/*
 * Miscellaneous module implementation.
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
 * $Id: phy_misc.c 666575 2016-10-21 23:07:43Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_misc.h"
#include <phy_rstr.h>
#include <phy_misc.h>
#include <phy_misc_api.h>
#include <phy_chanmgr_api.h>
#include <phy_utils_var.h>

/* forward declaration */
typedef struct phy_misc_mem phy_misc_mem_t;

/* module private states */
struct phy_misc_info {
	phy_info_t			*pi;	/* PHY info ptr */
	phy_type_misc_fns_t	*fns;	/* PHY specific function ptrs */
	phy_misc_mem_t		*mem;	/* Memory layout ptr */
	uint8 rxiqest_niter;	/* Number of averaging iterations for iqest */
	uint8 rxiqest_delay;	/* Averaging delay for iqest */
};

/* module private states memory layout */
struct phy_misc_mem {
	phy_misc_info_t cmn_info;
	phy_type_misc_fns_t fns;
/* add other variable size variables here at the end */
};

/* local function declaration */

/* attach/detach */
phy_misc_info_t *
BCMATTACHFN(phy_misc_attach)(phy_info_t *pi)
{
	phy_misc_mem_t	*mem = NULL;
	phy_misc_info_t	*cmn_info = NULL;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((mem = phy_malloc(pi, sizeof(phy_misc_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize infra params */
	cmn_info = &(mem->cmn_info);
	cmn_info->pi = pi;
	cmn_info->fns = &(mem->fns);
	cmn_info->mem = mem;

	/* Initialize misc params */
	pi->phy_scraminit = AUTO;

	/* Register callbacks */

	return cmn_info;

	/* error */
fail:
	phy_misc_detach(cmn_info);
	return NULL;
}

void
BCMATTACHFN(phy_misc_detach)(phy_misc_info_t *cmn_info)
{
	phy_misc_mem_t	*mem;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* Clean up module related states */

	/* Clean up infra related states */
	/* Malloc has failed. No cleanup is necessary here. */
	if (!cmn_info)
		return;

	/* Freeup memory associated with cmn_info */
	mem = cmn_info->mem;

	if (mem == NULL) {
		PHY_INFORM(("%s: null misc module\n", __FUNCTION__));
		return;
	}

	phy_mfree(cmn_info->pi, mem, sizeof(phy_misc_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_misc_register_impl)(phy_misc_info_t *mi, phy_type_misc_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));

	*mi->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_misc_unregister_impl)(phy_misc_info_t *mi)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

/* driver down processing */
int
phy_misc_down(phy_misc_info_t *mi)
{
	int callbacks = 0;

	PHY_TRACE(("%s\n", __FUNCTION__));

	return callbacks;
}

/* Inter-module interfaces and downward interfaces to PHY type specific implemenation */
#if defined(BCMDBG)
int
wlc_phy_test_init(phy_info_t *pi, int channel, bool txpkt)
{
	phy_type_misc_fns_t *fns = pi->misci->fns;

	if (channel > MAXCHANNEL)
		return BCME_OUTOFRANGECHAN;

	wlc_phy_chanspec_set((wlc_phy_t*)pi, CH20MHZ_CHSPEC(channel));

	/* stop any requests from the stack and prevent subsequent thread */
	pi->phytest_on = TRUE;

	/* the second argument to is used to enable cals only when called
	 * with TRUE in the case of AC layer; txpkt is FALSE for freq_accuracy
	 */
	if (fns->phy_type_misc_test_init != NULL) {
		fns->phy_type_misc_test_init(fns->ctx, txpkt);
	} else {
		return BCME_UNSUPPORTED;
	}

	/* Force WLAN antenna */
	wlc_btcx_override_enable(pi);

	return 0;
}

int
wlc_phy_test_stop(phy_info_t *pi)
{
	phy_type_misc_fns_t *fns = pi->misci->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));

	if (pi->phytest_on == FALSE)
		return 0;

	/* stop phytest mode */
	pi->phytest_on = FALSE;

	if (fns->phy_type_misc_test_stop != NULL) {
		(fns->phy_type_misc_test_stop)(fns->ctx);
	} else {
		PHY_TRACE(("%s:***CHECK***\n", __FUNCTION__));
	}
	return 0;
}

int
wlc_phy_test_freq_accuracy(phy_info_t *pi, int channel)
{
	int bcmerror = 0;
	phy_type_misc_fns_t *fns = pi->misci->fns;

	/* stop any test in progress */
	wlc_phy_test_stop(pi);

	/* channel 0 means this is a request to end the test */
	if (channel != 0) {
		if ((bcmerror = wlc_phy_test_init(pi, channel, FALSE)))
			return bcmerror;
	}

	/* Restore original values when channel 0 OR put out tone at LO for freq accuracy test */
	if (fns->phy_type_misc_test_freq_accuracy != NULL) {
		bcmerror = (fns->phy_type_misc_test_freq_accuracy)(fns->ctx, channel);
	}
	return bcmerror;
}
#endif	

uint32
wlc_phy_rx_iq_est(phy_info_t *pi, uint8 samples, uint8 antsel, uint8 resolution, uint8 lpf_hpc,
                  uint8 dig_lpf, uint8 gain_correct, uint8 extra_gain_3dB, uint8 wait_for_crs,
                  uint8 force_gain_type)
{
	phy_type_misc_fns_t *fns = pi->misci->fns;

	ASSERT(pi->misci->rxiqest_niter > 0);

	if (fns->phy_type_misc_rx_iq_est != NULL) {
		int iter, core_idx;
		uint32 rxiq_est;
		int32 rxiq_est_sum[PHY_CORE_MAX] = { 0 };
		uint16 rxiq_mask = 0xff;
		uint16 rxiq_bit_width = 8;

		if (pi->phy_rxiq_resln) {
			rxiq_mask = 0x3ff;
			rxiq_bit_width = 10;
		}

		if (pi->misci->rxiqest_niter == 1) {
			/* return results for single iqest */
			return fns->phy_type_misc_rx_iq_est(fns->ctx, samples, antsel,
				resolution, lpf_hpc, dig_lpf, gain_correct, extra_gain_3dB,
				wait_for_crs, force_gain_type);
		}

		for (iter = 0; iter < pi->misci->rxiqest_niter; ++iter) {
			rxiq_est = fns->phy_type_misc_rx_iq_est(fns->ctx, samples, antsel,
				resolution, lpf_hpc, dig_lpf, gain_correct, extra_gain_3dB,
				wait_for_crs, force_gain_type);
			FOREACH_CORE(pi, core_idx) {
				rxiq_est_sum[core_idx] += (int32)((rxiq_est & rxiq_mask)
					<< (32 - rxiq_bit_width)) >> (32 - rxiq_bit_width);
				rxiq_est >>= rxiq_bit_width;
			}
			if (pi->misci->rxiqest_delay && iter < pi->misci->rxiqest_niter - 1)
				OSL_DELAY(pi->misci->rxiqest_delay);
		}
		rxiq_est = 0;
		FOREACH_CORE(pi, core_idx) {
			rxiq_est |= (((uint32)(((rxiq_est_sum[core_idx] << 1) +
				pi->misci->rxiqest_niter) / (pi->misci->rxiqest_niter << 1))) &
				rxiq_mask) << (rxiq_bit_width * core_idx);
		}
		return rxiq_est;
} else {
		return 0; /* Fix me. It can only return non-negative number */
	}

}

void
wlc_phy_set_deaf(wlc_phy_t *ppi, bool user_flag)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_type_misc_fns_t *fns = pi->misci->fns;

	if (fns->phy_type_misc_set_deaf != NULL) {
		(fns->phy_type_misc_set_deaf)(fns->ctx, TRUE);
	} else {
		PHY_ERROR(("%s: Not yet supported\n", __FUNCTION__));
		ASSERT(0);
	}
}

void
wlc_phy_clear_deaf(wlc_phy_t  *ppi, bool user_flag)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_type_misc_fns_t *fns = pi->misci->fns;

	if (fns->phy_type_misc_clear_deaf != NULL) {
		(fns->phy_type_misc_clear_deaf)(fns->ctx, FALSE);
	} else {
		PHY_ERROR(("%s: Not yet supported\n", __FUNCTION__));
		ASSERT(0);
	}
}


int wlc_phy_iovar_get_rx_iq_est(phy_info_t *pi, int32 *ret_int_ptr, int32 int_val, int err)
{
	phy_misc_info_t *info;
	phy_type_misc_fns_t *fns;
	ASSERT(pi != NULL);
	info = pi->misci;
	fns = info->fns;

	if (fns->phy_type_misc_iovar_get_rx_iq_est != NULL)
		return fns->phy_type_misc_iovar_get_rx_iq_est(fns->ctx, ret_int_ptr, int_val, err);
	else
		return err;
}
int wlc_phy_iovar_set_rx_iq_est(phy_info_t *pi, void *p, int32 plen, int err)
{
	phy_misc_info_t *info;
	phy_type_misc_fns_t *fns;
	wl_iqest_params_t params;

	ASSERT(pi != NULL);
	info = pi->misci;
	fns = info->fns;

	memset(&params, 0, sizeof(params));
	params.niter = 1;
	params.delay = PHY_RXIQEST_AVERAGING_DELAY;

	if (plen > sizeof(uint32) && plen <  sizeof(params))
		return BCME_BUFTOOSHORT;
	if (plen > sizeof(params))
		plen = sizeof(params);
	bcopy(p, &params, plen);
	if (params.niter < 1)
		return BCME_BADARG;
	info->rxiqest_niter = params.niter;
	info->rxiqest_delay = params.delay;
	if (fns->phy_type_misc_iovar_set_rx_iq_est != NULL)
		return fns->phy_type_misc_iovar_set_rx_iq_est(fns->ctx, params.rxiq, err);
	else
		return err;
}


#ifdef PHY_DUMP_BINARY
int
phy_misc_getlistandsize(phy_info_t *pi, phyradregs_list_t **phyreglist, uint16 *phyreglist_sz)
{
	phy_type_misc_fns_t *fns = pi->misci->fns;

	if (fns->phy_type_misc_getlistandsize != NULL)
		return (fns->phy_type_misc_getlistandsize)(fns->ctx, phyreglist, phyreglist_sz);
	else {
		PHY_INFORM(("%s: wl%d: unsupported phy type %d\n",
			__FUNCTION__,  pi->sh->unit,  pi->pubpi->phy_type));
		return BCME_UNSUPPORTED;
	}
}
#endif /* PHY_DUMP_BINARY */


bool
wlc_phy_get_rxgainerr_phy(phy_info_t *pi, int16 *gainerr)
{
	/*
	 * Retrieves rxgain error (read from srom) for current channel;
	 * Returns TRUE if no gainerror was written to SROM, FALSE otherwise
	 */
	phy_type_misc_fns_t *fns = pi->misci->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->phy_type_misc_get_rxgainerr != NULL) {
		return (fns->phy_type_misc_get_rxgainerr)(fns->ctx, gainerr);
	} else {
		return 0;
	}
}

void
wlc_phy_hold_upd(wlc_phy_t *pih, mbool id, bool set)
{
	phy_info_t *pi = (phy_info_t *)pih;
	ASSERT(id);

	PHY_TRACE(("%s: id %d val %d old pi->measure_hold 0%x\n", __FUNCTION__, id, set,
		pi->measure_hold));

	PHY_CAL(("wl%d: %s: %s %s flag\n", pi->sh->unit, __FUNCTION__,
		set ? "SET" : "CLR",
		(id == PHY_HOLD_FOR_ASSOC) ? "ASSOC" :
		((id == PHY_HOLD_FOR_SCAN) ? "SCAN" :
		((id == PHY_HOLD_FOR_SCAN) ? "SCAN" :
		((id == PHY_HOLD_FOR_RM) ? "RM" :
		((id == PHY_HOLD_FOR_PLT) ? "PLT" :
		((id == PHY_HOLD_FOR_MUTE) ? "MUTE" :
		((id == PHY_HOLD_FOR_PKT_ENG) ? "PKTENG" :
		((id == PHY_HOLD_FOR_TOF) ? "TOF" :
		((id == PHY_HOLD_FOR_NOT_ASSOC) ? "NOT-ASSOC" : ""))))))))));

	if (set) {
		mboolset(pi->measure_hold, id);
	} else {
		mboolclr(pi->measure_hold, id);
	}
	if (id & PHY_HOLD_FOR_SCAN) {
		phy_txiqlocal_scanroam_cache(pi, set);
		phy_rxiqcal_scanroam_cache(pi, set);
	}
}

/** PHY mute is used to prevent emissions when calibrating */
void
wlc_phy_mute_upd(wlc_phy_t *pih, bool mute, mbool flags)
{
	phy_info_t *pi = (phy_info_t *)pih;

	PHY_TRACE(("wlc_phy_mute_upd: flags 0x%x mute %d\n", flags, mute));

	if (mute) {
		mboolset(pi->measure_hold, PHY_HOLD_FOR_MUTE);
	} else {
		mboolclr(pi->measure_hold, PHY_HOLD_FOR_MUTE);
	}

	/* check if need to schedule a phy cal */
	if (!mute && (flags & PHY_MUTE_FOR_PREISM)) {
		pi->cal_info->last_cal_time = (pi->sh->now > pi->sh->glacial_timer) ?
			(pi->sh->now - pi->sh->glacial_timer) : 0;
	}
	return;
}

bool
wlc_phy_ismuted(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t *)pih;

	return PHY_MUTED(pi);
}


/*
 * if bfe capable then return max no. of streams that sta can receive in a VHT
 * NDP minus 1.
 */
uint8
wlc_phy_get_bfe_ndp_recvstreams(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_misc_info_t *misci = pi->misci;
	phy_type_misc_fns_t *fns = misci->fns;
	phy_type_misc_ctx_t *ctx = fns->ctx;

	ASSERT(fns->get_bfe_ndp_recvstreams);
	return fns->get_bfe_ndp_recvstreams(ctx);
}

void
phy_misc_set_ldpc_override(wlc_phy_t *ppi, bool ldpc)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_misc_info_t *misci = pi->misci;
	phy_type_misc_fns_t *fns = misci->fns;
	phy_type_misc_ctx_t *ctx = fns->ctx;

	if (fns->set_ldpc_override) {
		fns->set_ldpc_override(ctx, ldpc);
	}
	return;
}

void
wlc_phy_preamble_override_set(wlc_phy_t *ppi, int8 val)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_misc_info_t *misci = pi->misci;
	phy_type_misc_fns_t *fns = misci->fns;
	phy_type_misc_ctx_t *ctx = fns->ctx;

	if (fns->set_preamble_override) {
		/* Phy specific implementation */
		fns->set_preamble_override(ctx, val);
	} else {
		/* Default behaviour */
		pi->n_preamble_override = val;
	}
}

#ifdef WFD_PHY_LL
void
wlc_phy_wfdll_chan_active(wlc_phy_t * ppi, bool chan_active)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_misc_info_t *misci = pi->misci;
	phy_type_misc_fns_t *fns = misci->fns;
	phy_type_misc_ctx_t *ctx = fns->ctx;

	if (fns->set_wfdll_chan_active) {
		fns->set_wfdll_chan_active(ctx, chan_active);
	}
}
#endif /* WFD_PHY_LL */

int
phy_misc_set_lo_gain_nbcal(wlc_phy_t *ppi, bool lo_gain)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_type_misc_fns_t *fns = pi->misci->fns;

	if (fns->set_lo_gain_nbcal != NULL) {
		return (fns->set_lo_gain_nbcal)(fns->ctx, lo_gain);
	} else {
		return BCME_UNSUPPORTED;
	}
}

/* WARs */
int
phy_misc_set_filt_war(wlc_phy_t *ppi, bool filt_war)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_type_misc_fns_t *fns = pi->misci->fns;

	if (fns->set_filt_war) {
		return (fns->set_filt_war)(fns->ctx, filt_war);
	} else {
		return BCME_UNSUPPORTED;
	}
}

bool
phy_misc_get_filt_war(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_type_misc_fns_t *fns = pi->misci->fns;

	if (fns->get_filt_war) {
		return (fns->get_filt_war)(fns->ctx);
	} else {
		return FALSE;
	}
}

int
phy_misc_tkip_rifs_war(wlc_phy_t *ppi, uint8 rifs)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_type_misc_fns_t *fns = pi->misci->fns;

	if (fns->tkip_rifs_war) {
		return (fns->tkip_rifs_war)(fns->ctx, rifs);
	} else {
		return BCME_UNSUPPORTED;
	}
}
