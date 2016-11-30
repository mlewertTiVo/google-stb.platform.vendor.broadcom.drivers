/*
 * CHanSPEC manipulation module implementation.
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
 * $Id: phy_chanmgr.c 639713 2016-05-24 18:02:57Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_init.h>
#include "phy_type_chanmgr.h"
#include <phy_chanmgr_notif.h>
#include <phy_chanmgr_notif_priv.h>
#include <phy_chanmgr_api.h>
#include <phy_chanmgr.h>
#include <phy_tpc.h>

/* forward declaration */
typedef struct phy_chanmgr_mem phy_chanmgr_mem_t;

/* module private states */
struct phy_chanmgr_info {
	phy_info_t				*pi;		/* PHY info ptr */
	phy_type_chanmgr_fns_t	*fns;		/* PHY specific function ptrs */
	phy_chanmgr_mem_t		*mem;		/* Memory layout ptr */
	uint16	home_chanspec;				/* holds operating/home chanspec */
};

/* module private states memory layout */
struct phy_chanmgr_mem {
	phy_chanmgr_info_t		info;
	phy_type_chanmgr_fns_t	fns;
/* add other variable size variables here at the end */
};

/* local function declaration */
static int phy_chanmgr_set_bw(phy_init_ctx_t *ctx);
static int phy_chanmgr_init_chanspec(phy_init_ctx_t *ctx);

/* attach/detach */
phy_chanmgr_info_t *
BCMATTACHFN(phy_chanmgr_attach)(phy_info_t *pi)
{
	phy_chanmgr_mem_t	*mem = NULL;
	phy_chanmgr_info_t	*cmn_info = NULL;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((mem = phy_malloc(pi, sizeof(phy_chanmgr_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	/* Initialize infra params */
	cmn_info = &(mem->info);
	cmn_info->pi = pi;
	cmn_info->fns = &(mem->fns);
	cmn_info->mem = mem;

	/* Initialize chanmgr params */

	/* register init fns */
	if (phy_init_add_init_fn(pi->initi, phy_chanmgr_set_bw,
		cmn_info, PHY_INIT_CHBW) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn set_bw failed\n", __FUNCTION__));
		goto fail;
	}

	if (phy_init_add_init_fn(pi->initi, phy_chanmgr_init_chanspec,
		cmn_info, PHY_INIT_CHSPEC) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn init_chanspec failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register callbacks */

	return cmn_info;

	/* error */
fail:
	phy_chanmgr_detach(cmn_info);
	return NULL;
}

void
BCMATTACHFN(phy_chanmgr_detach)(phy_chanmgr_info_t *cmn_info)
{
	phy_chanmgr_mem_t *mem;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* Clean up module related states */

	/* Clean up infra related states */
	if (!cmn_info)
		return;

	/* Memory associated with cmn_info must be cleaned up. */
	mem = cmn_info->mem;

	if (mem == NULL) {
		PHY_INFORM(("%s: null chanmgr module\n", __FUNCTION__));
		return;
	}
	phy_mfree(cmn_info->pi, mem, sizeof(phy_chanmgr_mem_t));
}

/* init bandwidth in h/w */
static int
WLBANDINITFN(phy_chanmgr_set_bw)(phy_init_ctx_t *ctx)
{
	phy_chanmgr_info_t *ti = (phy_chanmgr_info_t *)ctx;
	phy_info_t *pi = ti->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* sanitize bw here to avoid later mess. wlc_set_bw will invoke phy_reset,
	 *  but phy_init recursion is avoided by using init_in_progress
	 */
	if (CHSPEC_BW(pi->radio_chanspec) != pi->bw)
		wlapi_bmac_bw_set(pi->sh->physhim, CHSPEC_BW(pi->radio_chanspec));

	if (wlapi_bmac_bw_check(pi->sh->physhim) == BCME_BADCHAN) {
		PHY_ERROR(("PANIC! CHANSPEC_BW doesn't match with si_core_cflags BW bits\n"));
		ASSERT(0);
	}

	return BCME_OK;
}

/* Channel Manager Channel Specification Initialization */
static int
WLBANDINITFN(phy_chanmgr_init_chanspec)(phy_init_ctx_t *ctx)
{
	phy_chanmgr_info_t *chanmgri = (phy_chanmgr_info_t *)ctx;
	phy_type_chanmgr_fns_t *fns = chanmgri->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->init_chanspec != NULL) {
		return (fns->init_chanspec)(fns->ctx);
	}
	return BCME_OK;
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_chanmgr_register_impl)(phy_chanmgr_info_t *ti, phy_type_chanmgr_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));


	*ti->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_chanmgr_unregister_impl)(phy_chanmgr_info_t *ti)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

/*
 * Create/Destroy an operating chanspec context for 'chanspec'.
 */
int
phy_chanmgr_create_ctx(phy_info_t *pi, chanspec_t chanspec)
{
	phy_chanmgr_notif_data_t data;

	PHY_TRACE(("%s: chanspec 0x%x\n", __FUNCTION__, chanspec));

	data.event = PHY_CHANMGR_NOTIF_OPCHCTX_OPEN;
	data.new = chanspec;

	return phy_chanmgr_notif_signal(pi->chanmgr_notifi, &data, TRUE);
}

void
phy_chanmgr_destroy_ctx(phy_info_t *pi, chanspec_t chanspec)
{
	phy_chanmgr_notif_data_t data;

	PHY_TRACE(("%s: chanspec 0x%x\n", __FUNCTION__, chanspec));

	data.event = PHY_CHANMGR_NOTIF_OPCHCTX_CLOSE;
	data.new = chanspec;

	(void)phy_chanmgr_notif_signal(pi->chanmgr_notifi, &data, FALSE);
}

bool
wlc_phy_is_txbfcal(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_chanmgr_info_t *chanmgri = pi->chanmgri;
	phy_type_chanmgr_fns_t *fns = chanmgri->fns;

	if (fns->is_txbfcal != NULL) {
		return (fns->is_txbfcal)(fns->ctx);
	} else {
		return FALSE;
	}
}

int
wlc_phy_chanspec_bandrange_get(phy_info_t *pi, chanspec_t chanspec)
{
	phy_chanmgr_info_t *chanmgri = pi->chanmgri;
	phy_type_chanmgr_fns_t *fns = chanmgri->fns;
	int range = -1;

	if (fns->get_bandrange != NULL) {
		range = (fns->get_bandrange)(fns->ctx, chanspec);
	} else
		ASSERT(0);

	return range;
}

/*
 * Use the operating chanspec context associated with the 'chanspec' as
 * the current operating chanspec context.
 *
 * This function changes the current radio chanspec and applies
 * s/w properties to related h/w.
 */
int
phy_chanmgr_set_oper(phy_info_t *pi, chanspec_t chanspec)
{
	phy_chanmgr_notif_data_t data;

	PHY_TRACE(("%s: chanspec 0x%x\n", __FUNCTION__, chanspec));

	data.event = PHY_CHANMGR_NOTIF_OPCH_CHG;
	data.new = chanspec;
	data.old = pi->radio_chanspec;

	return phy_chanmgr_notif_signal(pi->chanmgr_notifi, &data, TRUE);
}

/*
 * Set the radio chanspec to the 'chanspec' and invalidate the current
 * operating chanspec context if any.
 *
 * This function only changes the current radio chanspec.
 */
int
phy_chanmgr_set(phy_info_t *pi, chanspec_t chanspec)
{
	phy_chanmgr_notif_data_t data;

	PHY_TRACE(("%s: chanspec 0x%x\n", __FUNCTION__, chanspec));

	data.event = PHY_CHANMGR_NOTIF_CH_CHG;
	data.new = chanspec;
	data.old = pi->radio_chanspec;

	return phy_chanmgr_notif_signal(pi->chanmgr_notifi, &data, FALSE);
}

/*
 * Update the radio chanspec.
 */
void
wlc_phy_chanspec_radio_set(wlc_phy_t *ppi, chanspec_t newch)
{
	phy_info_t *pi = (phy_info_t*)ppi;
#ifdef PREASSOC_PWRCTRL
	phy_preassoc_pwrctrl_upd(pi, newch);
#endif /* PREASSOC_PWRCTRL */
	/* update radio_chanspec */
	pi->radio_chanspec = newch;
}

/*
 * Set the chanspec.
 */

void
wlc_phy_chanspec_set(wlc_phy_t *ppi, chanspec_t chanspec)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_chanmgr_info_t *chanmgri = pi->chanmgri;
	phy_type_chanmgr_fns_t *fns = chanmgri->fns;

#if defined(PHYCAL_CACHING)
	ch_calcache_t *ctx = NULL;
	ctx = wlc_phy_get_chanctx(pi, chanspec);
#endif
	if (!SCAN_RM_IN_PROGRESS(pi) &&	(chanmgri->home_chanspec != chanspec))
		chanmgri->home_chanspec = chanspec;

	PHY_TRACE(("wl%d: %s: chanspec %x\n", pi->sh->unit, __FUNCTION__, chanspec));

	ASSERT(!wf_chspec_malformed(chanspec));

	PHY_CHANLOG(pi, __FUNCTION__, TS_ENTER, 0);

#ifdef WLSRVSDB
	pi->srvsdb_state->prev_chanspec = chanspec;
#endif /* WLSRVSDB */

	/* Update ucode channel value */
	wlc_phy_chanspec_shm_set(pi, chanspec);

#if defined(AP) && defined(RADAR)
	/* indicate first time radar detection */
	if (pi->radari != NULL)
		phy_radar_first_indicator_set(pi->radari);
#endif
	/* Update interference mode for ACPHY, as now init is not called on band/bw change */
	if ((!SCAN_RM_IN_PROGRESS(pi)) && (fns->interfmode_upd != NULL)) {
		fns->interfmode_upd(fns->ctx, chanspec);
	}

#if defined(PHYCAL_CACHING)
	/* If a channel context exists, retrieve the multi-phase info from there, else use
	 * the default one
	 */
	/* A context has to be explicitly created */
	if (ctx)
		pi->cal_info = &ctx->cal_info;
	else
		pi->cal_info = pi->def_cal_info;
#endif

	ASSERT(pi->cal_info);

	if (fns->chanspec_set != NULL) {
#ifdef ENABLE_FCBS
		if (IS_FCBS(pi)) {
			int chanidx;

			if (wlc_phy_is_fcbs_chan(pi, chanspec, &chanidx) &&
				!(SCAN_INPROG_PHY(pi) || RM_INPROG_PHY(pi) || PLT_INPROG_PHY(pi))) {
				wlc_phy_fcbs((wlc_phy_t*)pi, chanidx, 1);
				/* Need to set this to indicate that we have switched to new chan
				 * not needed for SW-based FCBS ???
				 */
				pi->phy_fcbs->FCBS_INPROG = 1;
				wlc_phy_chanspec_radio_set((wlc_phy_t *)pi, chanspec);
			} else {
				/* The conditioning here matches that below for FCBS init. Hence, if
				 * there is a pending FCBS init on this channel and if HW_FCBS, then
				 * we want to setup the channel-indicator bit, etc. appropriately
				 * before we call phy_specific chanspec_set
				 */
				if (wlc_phy_is_fcbs_pending(pi, chanspec, &chanidx) &&
				!(SCAN_INPROG_PHY(pi) || RM_INPROG_PHY(pi) || PLT_INPROG_PHY(pi))) {
						phy_fcbs_preinit(pi, chanidx);
				}

				(fns->chanspec_set)(fns->ctx, chanspec);

				/* Now that we are on new channel, check for a pending request */
				if (wlc_phy_is_fcbs_pending(pi, chanspec, &chanidx) &&
				!(SCAN_INPROG_PHY(pi) || RM_INPROG_PHY(pi) || PLT_INPROG_PHY(pi))) {
					wlc_phy_fcbs_init((wlc_phy_t*)pi, chanidx);
				} else {
					chanidx = wlc_phy_channelindicator_obtain(pi);
					pi->phy_fcbs->initialized[chanidx] = FALSE;
				}
			}
		} else {
			(fns->chanspec_set)(fns->ctx, chanspec);
		}
#else
		(fns->chanspec_set)(fns->ctx, chanspec);
#endif /* ENABLE_FCBS */
	}
	if (fns->preempt != NULL) {
		(fns->preempt)(fns->ctx, ((pi->sh->interference_mode & ACPHY_ACI_PREEMPTION) != 0),
			FALSE);
	}

#if defined(PHYCAL_CACHING)
	/* Switched the context so restart a pending MPHASE cal, else clear the state */
	if (ctx) {
		if (wlc_phy_cal_cache_restore(pi) == BCME_ERROR) {
			PHY_CAL(("%s cal cache restore on chanspec 0x%x Failed\n",
				__FUNCTION__, pi->radio_chanspec));
		}

		if (CHIPID(pi->sh->chip) != BCM43237_CHIP_ID) {
			/* Calibrate if now > last_cal_time + glacial */
			if (PHY_PERICAL_MPHASE_PENDING(pi)) {
				PHY_CAL(("%s: Restarting calibration for 0x%x phase %d\n",
					__FUNCTION__, chanspec, pi->cal_info->cal_phase_id));
				/* Delete any existing timer just in case */
				wlapi_del_timer(pi->sh->physhim, pi->phycal_timer);
				wlapi_add_timer(pi->sh->physhim, pi->phycal_timer, 0, 0);
			} else if ((pi->phy_cal_mode != PHY_PERICAL_DISABLE) &&
				(pi->phy_cal_mode != PHY_PERICAL_MANUAL) &&
				((pi->sh->now - pi->cal_info->last_cal_time) >=
				pi->sh->glacial_timer)) {
				wlc_phy_cal_perical((wlc_phy_t *)pi, PHY_PERICAL_WATCHDOG);
			}
		} else {
			if (PHY_PERICAL_MPHASE_PENDING(pi))
				wlapi_del_timer(pi->sh->physhim, pi->phycal_timer);
		}
	}
#endif /* PHYCAL_CACHING */

	wlapi_update_bt_chanspec(pi->sh->physhim, chanspec,
		SCAN_INPROG_PHY(pi), RM_INPROG_PHY(pi));
	PHY_CHANLOG(pi, __FUNCTION__, TS_EXIT, 0);
}

#if defined(WLTEST)
int
phy_chanmgr_get_smth(phy_info_t *pi, int32* ret_int_ptr)
{
	phy_chanmgr_info_t *chanmgri = pi->chanmgri;
	phy_type_chanmgr_fns_t *fns = chanmgri->fns;
	if (fns->get_smth != NULL) {
		return (fns->get_smth)(fns->ctx, ret_int_ptr);
	} else {
		return BCME_UNSUPPORTED;
	}
}

int
phy_chanmgr_set_smth(phy_info_t *pi, int8 int_val)
{
	phy_chanmgr_info_t *chanmgri = pi->chanmgri;
	phy_type_chanmgr_fns_t *fns = chanmgri->fns;
	if (fns->set_smth != NULL) {
		return (fns->set_smth)(fns->ctx, int_val);
	} else {
		return BCME_UNSUPPORTED;
	}
}
#endif /* defined(WLTEST) */
