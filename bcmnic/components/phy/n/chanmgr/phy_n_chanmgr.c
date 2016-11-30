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
 * $Id: phy_n_chanmgr.c 610412 2016-01-06 23:43:14Z vyass $
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

/* local functions */
static int phy_n_chanmgr_get_chanspec_bandrange(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec);
static void phy_n_chanmgr_chanspec_set(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec);
static void phy_n_chanmgr_upd_interf_mode(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec);

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
