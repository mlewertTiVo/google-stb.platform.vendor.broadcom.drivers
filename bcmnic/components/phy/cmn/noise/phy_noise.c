/*
 * NOISEmeasure module implementation.
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
 * $Id: phy_noise.c 635450 2016-05-03 23:49:04Z junseok $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_init.h>
#include "phy_type_noise.h"
#include <phy_noise.h>
#include <phy_noise_api.h>

/* module private states */
struct phy_noise_info {
	phy_info_t *pi;
	phy_type_noise_fns_t *fns;
	int bandtype;
};

/* module private states memory layout */
typedef struct {
	phy_noise_info_t info;
	phy_type_noise_fns_t fns;
/* add other variable size variables here at the end */
} phy_noise_mem_t;

/* local function declaration */
static int phy_noise_reset(phy_init_ctx_t *ctx);
static int phy_noise_init(phy_init_ctx_t *ctx);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int phy_noise_dump(void *ctx, struct bcmstrbuf *b);
#endif
#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(WLTEST)
static int phy_aci_dump(void *ctx, struct bcmstrbuf *b);
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(WLTEST) */
static bool phy_noise_start_wd(phy_wd_ctx_t *ctx);
static bool phy_noise_stop_wd(phy_wd_ctx_t *ctx);
static bool phy_noise_reset_wd(phy_wd_ctx_t *ctx);

/* attach/detach */
phy_noise_info_t *
BCMATTACHFN(phy_noise_attach)(phy_info_t *pi, int bandtype)
{
	phy_noise_info_t *info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((info = phy_malloc(pi, sizeof(phy_noise_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	info->pi = pi;
	info->fns = &((phy_noise_mem_t *)info)->fns;

	/* Initialize noise params */
	info->bandtype = bandtype;

	pi->interf->curr_home_channel = CHSPEC_CHANNEL(pi->radio_chanspec);
	pi->sh->interference_mode_override = FALSE;
	pi->sh->interference_mode_2G = INTERFERE_NONE;
	pi->sh->interference_mode_5G = INTERFERE_NONE;

	/* Default to 60. Each PHY specific attach should initialize it
	 * to PHY/chip specific.
	 */
	pi->aci_exit_check_period = 60;
	pi->aci_enter_check_period = 16;
	pi->aci_state = 0;

	pi->interf->scanroamtimer = 0;
	pi->interf->rssi_index = 0;
	pi->interf->rssi = 0;

	/* register watchdog fn */
	if (phy_wd_add_fn(pi->wdi, phy_noise_start_wd, info,
	                  PHY_WD_PRD_1TICK, PHY_WD_1TICK_NOISE_START,
	                  PHY_WD_FLAG_SCAN_SKIP|PHY_WD_FLAG_PLT_SKIP) != BCME_OK) {
		PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
		goto fail;
	}
	if (phy_wd_add_fn(pi->wdi, phy_noise_stop_wd, info,
	                  PHY_WD_PRD_1TICK, PHY_WD_1TICK_NOISE_STOP,
	                  PHY_WD_FLAG_NONE) != BCME_OK) {
		PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
		goto fail;
	}
	if (phy_wd_add_fn(pi->wdi, phy_noise_reset_wd, info,
	                  PHY_WD_PRD_1TICK, PHY_WD_1TICK_NOISE_RESET,
	                  PHY_WD_FLAG_NONE) != BCME_OK) {
		PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
		goto fail;
	}

	/* register reset fn */
	if (phy_init_add_init_fn(pi->initi, phy_noise_reset, info, PHY_INIT_NOISERST)
			!= BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn failed\n", __FUNCTION__));
		goto fail;
	}

	/* register init fn */
	if (phy_init_add_init_fn(pi->initi, phy_noise_init, info, PHY_INIT_NOISE) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn failed\n", __FUNCTION__));
		goto fail;
	}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	/* register dump callback */
	phy_dbg_add_dump_fn(pi, "phynoise", phy_noise_dump, info);
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */

#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(WLTEST)
	phy_dbg_add_dump_fn(pi, "phyaci", phy_aci_dump, info);
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(WLTEST) */

	return info;

	/* error */
fail:
	phy_noise_detach(info);
	return NULL;
}

void
BCMATTACHFN(phy_noise_detach)(phy_noise_info_t *info)
{
	phy_info_t *pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (info == NULL) {
		PHY_INFORM(("%s: null noise module\n", __FUNCTION__));
		return;
	}

	pi = info->pi;

	phy_mfree(pi, info, sizeof(phy_noise_mem_t));
}

/* watchdog callbacks */
static bool
phy_noise_start_wd(phy_wd_ctx_t *ctx)
{
	phy_noise_info_t *nxi = (phy_noise_info_t *)ctx;
	phy_info_t *pi = nxi->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* update phy noise moving average only if no scan or rm in progress */
	wlc_phy_noise_sample_request(pi, PHY_NOISE_SAMPLE_MON,
	                             CHSPEC_CHANNEL(pi->radio_chanspec));

	return TRUE;
}

static bool
phy_noise_stop_wd(phy_wd_ctx_t *ctx)
{
	phy_noise_info_t *nxi = (phy_noise_info_t *)ctx;
	phy_type_noise_fns_t *fns = nxi->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->stop != NULL)
		(fns->stop)(fns->ctx);

	return TRUE;
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_noise_register_impl)(phy_noise_info_t *nxi, phy_type_noise_fns_t *fns)
{
	phy_info_t *pi = nxi->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));


	*nxi->fns = *fns;

	/* N.B.: This is the signal of module attach completion. */
	/* Summarize the PHY type setting... */
	if (BAND_2G(nxi->bandtype)) {
		pi->sh->interference_mode =
			pi->sh->interference_mode_2G;
	} else {
		pi->sh->interference_mode =
			pi->sh->interference_mode_5G;
	}

	return BCME_OK;
}

void
BCMATTACHFN(phy_noise_unregister_impl)(phy_noise_info_t *ti)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

/* change mode */
int
phy_noise_set_mode(phy_noise_info_t *ii, int wanted_mode, bool init)
{
	phy_type_noise_fns_t *fns = ii->fns;
	phy_info_t *pi = ii->pi;

	PHY_TRACE(("%s: mode %d init %d\n", __FUNCTION__, wanted_mode, init));

	if (fns->mode != NULL)
		(fns->mode)(fns->ctx, wanted_mode, init);

	pi->cur_interference_mode = wanted_mode;
	return BCME_OK;
}

static bool
phy_noise_reset_wd(phy_wd_ctx_t *ctx)
{
	phy_noise_info_t *nxi = (phy_noise_info_t *)ctx;
	phy_info_t *pi = nxi->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* reset phynoise state if ucode interrupt doesn't arrive for so long */
	if (pi->phynoise_state && (pi->sh->now - pi->phynoise_now) > 5) {
		PHY_TMP(("wlc_phy_watchdog: ucode phy noise sampling overdue\n"));
		pi->phynoise_state = 0;
	}

	return TRUE;
}

/* noise calculation */
void
wlc_phy_noise_calc(phy_info_t *pi, uint32 *cmplx_pwr, int8 *pwr_ant, uint8 extra_gain_1dB)
{
	int8 cmplx_pwr_dbm[PHY_CORE_MAX];
	uint8 i;
	phy_noise_info_t *noisei = pi->noisei;
	phy_type_noise_fns_t *fns = noisei->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	bzero((uint8 *)cmplx_pwr_dbm, sizeof(cmplx_pwr_dbm));
	ASSERT(PHYCORENUM(pi->pubpi->phy_corenum) <= PHY_CORE_MAX);

	PHY_INFORM(("--> RXGAIN: %d\n", wlapi_bmac_read_shm(pi->sh->physhim,
		M_PWRIND_BLKS(pi)+0xC)));

	phy_utils_computedB(cmplx_pwr, cmplx_pwr_dbm, PHYCORENUM(pi->pubpi->phy_corenum));

	if (fns->calc != NULL)
		(fns->calc)(fns->ctx, cmplx_pwr_dbm, extra_gain_1dB);
	FOREACH_CORE(pi, i) {
		pwr_ant[i] = cmplx_pwr_dbm[i];
		PHY_INFORM(("wlc_phy_noise_calc_phy: pwr_ant[%d] = %d\n", i, pwr_ant[i]));
	}

	PHY_INFORM(("%s: samples %d ant %d\n", __FUNCTION__, pi->phy_rxiq_samps,
		pi->phy_rxiq_antsel));
}

void
wlc_phy_noise_calc_fine_resln(phy_info_t *pi, uint32 *cmplx_pwr, uint16 *crsmin_pwr,
		int16 *pwr_ant, uint8 extra_gain_1dB, int16 *tot_gain)
{
	int16 cmplx_pwr_dbm[PHY_CORE_MAX];
	uint8 i;
	phy_noise_info_t *noisei = pi->noisei;
	phy_type_noise_fns_t *fns = noisei->fns;

	/* lookup table for computing the dB contribution from the first
	 * 4 bits after MSB (most significant NONZERO bit) in cmplx_pwr[core]
	 * (entries in multiples of 0.25dB):
	 */
	uint8 dB_LUT[] = {0, 1, 2, 3, 4, 5, 6, 6, 7, 8, 8, 9,
		10, 10, 11, 11};
	uint8 LUT_correction[] = {13, 12, 12, 13, 16, 20, 25,
		5, 12, 19, 2, 11, 20, 5, 15, 1};

	bzero((uint16 *)cmplx_pwr_dbm, sizeof(cmplx_pwr_dbm));
	ASSERT(PHYCORENUM(pi->pubpi->phy_corenum) <= PHY_CORE_MAX);

	/* Convert sample-power to dB scale: */
	FOREACH_CORE(pi, i) {
		uint8 shift_ct, lsb, msb_loc;
		uint8 msb2345 = 0x0;
		uint32 tmp;
		tmp = cmplx_pwr[i];
		shift_ct = msb_loc = 0;
		while (tmp != 0) {
			tmp = tmp >> 1;
			shift_ct++;
			lsb = (uint8)(tmp & 1);
			if (lsb == 1)
				msb_loc = shift_ct;
		}

		/* Store first 4 bits after MSB: */
		if (msb_loc <= 4) {
			msb2345 = (cmplx_pwr[i] << (4-msb_loc)) & 0xf;
		} else {
			/* Need to first round cmplx_pwr to 5 MSBs: */
			tmp = cmplx_pwr[i] + (1U << (msb_loc-5));
			/* Check if MSB has shifted in the process: */
			if (tmp & (1U << (msb_loc+1))) {
				msb_loc++;
			}
			msb2345 = (tmp >> (msb_loc-4)) & 0xf;
		}

		/* Power in 0.25 dB steps: */
		cmplx_pwr_dbm[i] = ((3*msb_loc) << 2) + dB_LUT[msb2345];

		/* Apply a possible +0.25dB (1 step) correction depending
		 * on MSB location in cmplx_pwr[core]:
		 */
		cmplx_pwr_dbm[i] += (int16)((msb_loc >= LUT_correction[msb2345]) ? 1 : 0);
		/* crsmin_pwr = 8*log2(16*cmplx_pwr)
		   = 32 + 8*(10*log10(cmplx_pwr))/(10*log10(2))
		   = 32 + (cmplx_pwr_db * round(8/(10*log10(2)) * 2^10)) >> (10+2),
		   where additional >>2 is to account for quarter dB resolution
		*/
		crsmin_pwr[i] = 32 + ((cmplx_pwr_dbm[i] * 2721) >> 12);
	}

	if (fns->calc_fine != NULL)
		(fns->calc_fine)(fns->ctx, cmplx_pwr_dbm, extra_gain_1dB, tot_gain);
	FOREACH_CORE(pi, i) {
		pwr_ant[i] = cmplx_pwr_dbm[i];
		PHY_INFORM(("In %s: pwr_ant[%d] = %d\n", __FUNCTION__, i, pwr_ant[i]));
	}
}

/* driver up/init processing */
static int
WLBANDINITFN(phy_noise_init)(phy_init_ctx_t *ctx)
{
	phy_noise_info_t *ii = (phy_noise_info_t *)ctx;
	phy_info_t *pi = ii->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	    /* do not reinitialize interference mode, could be scanning */
	if ((ISNPHY(pi) || ISHTPHY(pi) || ISACPHY(pi)) && SCAN_INPROG_PHY(pi))
		return BCME_OK;

	/* initialize interference algorithms */
	if (pi->sh->interference_mode_override == TRUE) {
		/* keep the same values */
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			pi->sh->interference_mode = pi->sh->interference_mode_2G_override;
		} else {
			/* for 5G, only values 0 and 1 are valid options */
			if (pi->sh->interference_mode_5G_override == 0 ||
			    pi->sh->interference_mode_5G_override == 1) {
				pi->sh->interference_mode = pi->sh->interference_mode_5G_override;
			} else {
				/* used invalid value. so default to 0 */
				pi->sh->interference_mode = 0;
			}
		}
	} else {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			pi->sh->interference_mode = pi->sh->interference_mode_2G;
		} else {
			pi->sh->interference_mode = pi->sh->interference_mode_5G;
		}
	}

	return 	phy_noise_set_mode(ii, pi->sh->interference_mode, FALSE);
}

/* Reset */
static int
WLBANDINITFN(phy_noise_reset)(phy_init_ctx_t *ctx)
{
	phy_noise_info_t *ii = (phy_noise_info_t *)ctx;
	phy_type_noise_fns_t *fns = ii->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* Reset aci on band-change */
	if (fns->reset != NULL)
		(fns->reset)(fns->ctx);
	return BCME_OK;
}

#ifndef WLC_DISABLE_ACI
void
wlc_phy_interf_rssi_update(wlc_phy_t *pih, chanspec_t chanspec, int8 leastRSSI)
{
	phy_info_t *pi = (phy_info_t *)pih;
	phy_type_noise_fns_t *fns = pi->noisei->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->interf_rssi_update != NULL) {
		(fns->interf_rssi_update)(fns->ctx, chanspec, leastRSSI);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}
#endif /* Compiling out ACI code for 4324 */

/* Implements core functionality of WLC_SET_INTERFERENCE_OVERRIDE_MODE */
int
wlc_phy_set_interference_override_mode(phy_info_t* pi, int val)
{
	int bcmerror = 0;
	phy_type_noise_fns_t *fns = pi->noisei->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));

	if (val == INTERFERE_OVRRIDE_OFF) {
		/* this is a reset */
		pi->sh->interference_mode_override = FALSE;
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			pi->sh->interference_mode = pi->sh->interference_mode_2G;
		} else {
			pi->sh->interference_mode = pi->sh->interference_mode_5G;
		}
	} else {
		pi->sh->interference_mode_override = TRUE;
		/* push to sw state */
		if (fns->interf_mode_set != NULL) {
			(fns->interf_mode_set)(fns->ctx, val);
		} else {
			pi->sh->interference_mode_2G_override = val;
			pi->sh->interference_mode_5G_override = val;
			pi->sh->interference_mode = val;
		}
	}

	if (!pi->sh->up)
		return BCME_NOTUP;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);

#ifndef WLC_DISABLE_ACI
	/* turn interference mode to off before entering another mode */
	if (val != INTERFERE_NONE)
		phy_noise_set_mode(pi->noisei, INTERFERE_NONE, TRUE);

	if (phy_noise_set_mode(pi->noisei, pi->sh->interference_mode, TRUE) != BCME_OK)
		bcmerror = BCME_BADOPTION;
#endif

	wlapi_enable_mac(pi->sh->physhim);
	return bcmerror;
}

/* ucode detected ACI, apply mitigation settings */
void
wlc_phy_hwaci_mitigate_intr(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t*)pih;
	phy_noise_info_t *noisei = pi->noisei;
	phy_type_noise_fns_t *fns = noisei->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->aci_mitigate != NULL)
		(fns->aci_mitigate)(fns->ctx);
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
/* common dump functions for non-ac phy */
int
phy_noise_dump_common(phy_info_t *pi, struct bcmstrbuf *b)
{
	int channel, indx;
	int val;
	char *ptr;

	val = pi->sh->interference_mode;
	if (pi->aci_state & ACI_ACTIVE)
		val |= AUTO_ACTIVE;

	if (val & AUTO_ACTIVE)
		bcm_bprintf(b, "ACI is Active\n");
	else {
		bcm_bprintf(b, "ACI is Inactive\n");
		return BCME_OK;
	}

	for (channel = 0; channel < ACI_LAST_CHAN; channel++) {
		bcm_bprintf(b, "Channel %d : ", channel + 1);
		for (indx = 0; indx < 50; indx++) {
			ptr = &(pi->interf->aci.rssi_buf[channel][indx]);
			if (*ptr == (char)-1)
				break;
			bcm_bprintf(b, "%d ", *ptr);
		}
		bcm_bprintf(b, "\n");
	}

	return BCME_OK;
}

/* Dump rssi values from aci scans */
static int
phy_noise_dump(void *ctx, struct bcmstrbuf *b)
{
	phy_noise_info_t *nxi = (phy_noise_info_t *)ctx;
	phy_info_t *pi = nxi->pi;
	uint32 i, idx, antidx;
	int32 tot;

	if (!pi->sh->up)
		return BCME_NOTUP;

	bcm_bprintf(b, "History and average of latest %d noise values:\n",
		PHY_NOISE_WINDOW_SZ);

	FOREACH_CORE(pi, antidx) {
		tot = 0;
		bcm_bprintf(b, "Ant%d: [", antidx);

		idx = pi->phy_noise_index;
		for (i = 0; i < PHY_NOISE_WINDOW_SZ; i++) {
			bcm_bprintf(b, "%4d", pi->phy_noise_win[antidx][idx]);
			tot += pi->phy_noise_win[antidx][idx];
			idx = MODINC_POW2(idx, PHY_NOISE_WINDOW_SZ);
		}
		bcm_bprintf(b, "]");

		tot /= PHY_NOISE_WINDOW_SZ;
		bcm_bprintf(b, " [%4d]\n", tot);
	}

	return BCME_OK;
}
#endif /* BCMDBG || BCMDBG_DUMP */

#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(WLTEST)
/* Dump desense values */
static int
phy_aci_dump(void *ctx, struct bcmstrbuf *b)
{
	phy_noise_info_t *nxi = (phy_noise_info_t *)ctx;
	phy_type_noise_fns_t *fns = nxi->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->dump != NULL)
		(fns->dump)(fns->ctx, b);

	return BCME_OK;
}
#endif /* BCMDBG || BCMDBG_DUMP || WLTEST */
