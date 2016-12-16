/*
 * NPHY Channel Manager module implementation
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: phy_n_chanmgr.c 658200 2016-09-07 01:03:02Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_chanmgr.h>
#include "phy_type_chanmgr.h"
#include <phy_n.h>
#include <phy_n_chanmgr.h>
#include <bcmwifi_channels.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_n_chanmgr_info {
	phy_info_t *pi;
	phy_n_info_t *ni;
	phy_chanmgr_info_t *chanmgri;
};

/* Functions called using callback from Common layer */
static int phy_n_chanmgr_get_chanspec_bandrange(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec);
static void phy_n_chanmgr_chanspec_set(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec);
static void phy_n_chanmgr_upd_interf_mode(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec);
static uint8 phy_n_set_chanspec_sr_vsdb(phy_type_chanmgr_ctx_t *ctx,
		chanspec_t chanspec, uint8 *last_chan_saved);

/* Register/unregister NPHY specific implementation to common layer */
phy_n_chanmgr_info_t *
BCMATTACHFN(phy_n_chanmgr_register_impl)(phy_info_t *pi, phy_n_info_t *ni,
	phy_chanmgr_info_t *chanmgri)
{
	phy_n_chanmgr_info_t *info;
	phy_type_chanmgr_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_n_chanmgr_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_n_chanmgr_info_t));
	info->pi = pi;
	info->ni = ni;
	info->chanmgri = chanmgri;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.get_bandrange = phy_n_chanmgr_get_chanspec_bandrange;
	fns.chanspec_set = phy_n_chanmgr_chanspec_set;
	fns.interfmode_upd = phy_n_chanmgr_upd_interf_mode;
	fns.set_chanspec_sr_vsdb = phy_n_set_chanspec_sr_vsdb;

	fns.ctx = info;

	if (phy_chanmgr_register_impl(chanmgri, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_chanmgr_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return info;
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_n_chanmgr_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_n_chanmgr_unregister_impl)(phy_n_chanmgr_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_chanmgr_info_t *chanmgri = info->chanmgri;

	phy_chanmgr_unregister_impl(chanmgri);

	phy_mfree(pi, info, sizeof(phy_n_chanmgr_info_t));
}

static int
phy_n_chanmgr_get_chanspec_bandrange(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec)
{
	phy_n_chanmgr_info_t *info = (phy_n_chanmgr_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint channel = CHSPEC_CHANNEL(chanspec);
	return wlc_phy_get_chan_freq_range_nphy(pi, channel);
}

/* Set Channel Specification */
static void
phy_n_chanmgr_chanspec_set(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec)
{
	phy_n_chanmgr_info_t *info = (phy_n_chanmgr_info_t *)ctx;
	phy_info_t *pi = info->pi;
	wlc_phy_chanspec_set_nphy(pi, chanspec);
}

static void
phy_n_chanmgr_upd_interf_mode(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec)
{
	phy_n_chanmgr_info_t *info = (phy_n_chanmgr_info_t *)ctx;
	phy_info_t *pi = info->pi;
	if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 4))
	{
		if (pi->sh->interference_mode_override == TRUE) {
			pi->sh->interference_mode = CHSPEC_IS2G(chanspec) ?
			        pi->sh->interference_mode_2G_override :
			        pi->sh->interference_mode_5G_override;
		} else {
			pi->sh->interference_mode = CHSPEC_IS2G(chanspec) ?
			        pi->sh->interference_mode_2G :
			        pi->sh->interference_mode_5G;
		}
	}
}

/**
 * Attempts to switch to the caller supplied chanspec using hardware assisted band switching. There
 * are conditions that determine whether such as hardware switch is possible, if it is not possible
 * (or there was an other problem preventing the channel switch) the function will return FALSE,
 * giving the caller an opportunity to invoke a subsequent non-SRVSDB assisted channel switch.
 *
 * Output argument 'last_chan_saved' is set to 'TRUE' when the context of the 'old' channel was
 * saved by hardware, and can even be set to 'TRUE' when the channel switch itself failed.
 *
 * Note: the *caller* should set *last_chan_saved to 'FALSE' before calling this function.
 */
static uint8
phy_n_set_chanspec_sr_vsdb(phy_type_chanmgr_ctx_t *ctx,
		chanspec_t chanspec, uint8 * last_chan_saved)
{
	uint8 switch_done = FALSE;
#ifdef WLSRVSDB
	phy_n_chanmgr_info_t *info = (phy_n_chanmgr_info_t *)ctx;
	phy_info_t *pi = info->pi;

	/* check if malloc for SW backup structures have failed */
	if (SRVSDSB_MEM_ALLOC_FAIL(pi))  {
		printf("SRVSDB : Mem alloc fail \n");
		ASSERT(0);
		goto exit;
	}

	/* While coming out of roam, its fresh start. Make status = 0 */
	if (ROAM_SRVSDB_RESET(pi)) {
		phy_reset_srvsdb_engine((wlc_phy_t*)pi);
	}

	/* reset fifo, disable stall */
	wlc_phy_srvsdb_prepare(pi, ENTER);

	/* If channel pattern matches, enter HW VSDB */
	if (sr_vsdb_switch_allowed(pi, chanspec)) {
		if (!wlc_set_chanspec_vsdb_phy(pi, chanspec)) {
			goto exit;
		}
		*last_chan_saved = TRUE;
		/*
		 * vsdb_trig_cnt tracks hardware state: the first two times the SR engine is
		 * triggered, it will save and restore (in one operation) the same context, so it
		 * will not switch context.
		 */
		if (pi->srvsdb_state->vsdb_trig_cnt > 1)
			switch_done =  TRUE;
	} else {
		switch_done = FALSE;
		goto exit;
	}

	/* to restore radio state */
	wlc_phy_resetcca_nphy(pi);
	wlc_20671_vco_cal(pi, TRUE);

exit:
	wlc_phy_srvsdb_prepare(pi, EXIT);

	/* store present chanspec */
	pi->srvsdb_state->prev_chanspec = chanspec;
#endif /* WLSRVSDB */

	return switch_done;
}
