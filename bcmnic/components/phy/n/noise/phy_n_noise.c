/*
 * NPHY NOISE module implementation
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
 * $Id: phy_n_noise.c 635450 2016-05-03 23:49:04Z junseok $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_wd.h>
#include "phy_type_noise.h"
#include <phy_n.h>
#include <phy_n_noise.h>
#include <phy_n_info.h>

/* module private states */
struct phy_n_noise_info {
	phy_info_t *pi;
	phy_n_info_t *ni;
	phy_noise_info_t *ii;
};

/* local functions */
static int phy_n_noise_attach_modes(phy_info_t *pi);
static void phy_n_noise_calc(phy_type_noise_ctx_t *ctx, int8 cmplx_pwr_dbm[], uint8 extra_gain_1dB);
static void phy_n_noise_calc_fine_resln(phy_type_noise_ctx_t *ctx, int16 cmplx_pwr_dbm[],
	uint8 extra_gain_1dB, int16 *tot_gain);
static void phy_n_noise_set_mode(phy_type_noise_ctx_t *ctx, int wanted_mode, bool init);
static void phy_n_noise_reset(phy_type_noise_ctx_t *ctx);
static bool phy_n_noise_noise_wd(phy_wd_ctx_t *ctx);
static bool phy_n_noise_aci_wd(phy_wd_ctx_t *ctx);
#ifndef WLC_DISABLE_ACI
static void phy_n_noise_interf_rssi_update(phy_type_noise_ctx_t *ctx,
	chanspec_t chanspec, int8 leastRSSI);
#endif
static void phy_n_noise_interf_mode_set(phy_type_noise_ctx_t *ctx, int val);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int phy_n_noise_dump(phy_type_noise_ctx_t *ctx, struct bcmstrbuf *b);
#else
#define phy_n_noise_dump NULL
#endif

/* register phy type specific implementation */
phy_n_noise_info_t *
BCMATTACHFN(phy_n_noise_register_impl)(phy_info_t *pi, phy_n_info_t *ni, phy_noise_info_t *ii)
{
	phy_n_noise_info_t *info;
	phy_type_noise_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((info = phy_malloc(pi, sizeof(phy_n_noise_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_n_noise_info_t));
	info->pi = pi;
	info->ni = ni;
	info->ii = ii;

	if (phy_n_noise_attach_modes(pi) != BCME_OK) {
		goto fail;
	}

	/* register watchdog fn */
	if (phy_wd_add_fn(pi->wdi, phy_n_noise_noise_wd, info,
	                  PHY_WD_PRD_1TICK, PHY_WD_1TICK_INTF_NOISE,
	                  PHY_WD_FLAG_NONE) != BCME_OK) {
		PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
		goto fail;
	}
	if (phy_wd_add_fn(pi->wdi, phy_n_noise_aci_wd, info,
	                  PHY_WD_PRD_1TICK, PHY_WD_1TICK_NOISE_ACI,
	                  PHY_WD_FLAG_NONE) != BCME_OK) {
		PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
		goto fail;
	}

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.mode = phy_n_noise_set_mode;
	fns.reset = phy_n_noise_reset;
	fns.calc = phy_n_noise_calc;
	fns.calc_fine = phy_n_noise_calc_fine_resln;
#ifndef WLC_DISABLE_ACI
	fns.interf_rssi_update = phy_n_noise_interf_rssi_update;
#endif
	fns.interf_mode_set = phy_n_noise_interf_mode_set;
	fns.dump = phy_n_noise_dump;
	fns.ctx = info;

	phy_noise_register_impl(ii, &fns);

	return info;

	/* error handling */
fail:
	phy_n_noise_unregister_impl(info);

	return NULL;
}

static int
BCMATTACHFN(phy_n_noise_attach_modes)(phy_info_t *pi)
{
	pi->interf->aci.nphy = (nphy_aci_interference_info_t *)
		phy_malloc(pi, sizeof(*(pi->interf->aci.nphy)));

	if (pi->interf->aci.nphy == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));

		return BCME_NOMEM;
	}

	if (pi->pubpi->phy_rev == LCNXN_BASEREV) {
		pi->sh->interference_mode_2G = WLAN_AUTO;
		pi->sh->interference_mode_5G = WLAN_AUTO_W_NOISE;
	} else if (pi->pubpi->phy_rev == LCNXN_BASEREV + 1) {
		pi->sh->interference_mode_2G = WLAN_AUTO_W_NOISE;
		pi->sh->interference_mode_5G = NON_WLAN;
	} else if (pi->pubpi->phy_rev == LCNXN_BASEREV + 2) {
		pi->sh->interference_mode_2G = WLAN_AUTO_W_NOISE;
		pi->sh->interference_mode_5G = NON_WLAN;
	} else if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3)) {
		if (CHIP_4324_B0(pi) || CHIP_4324_B4(pi)) {
			pi->sh->interference_mode_2G = WLAN_AUTO_W_NOISE;
			pi->sh->interference_mode_5G = NON_WLAN;
		} else if (CHIP_4324_B1(pi) || CHIP_4324_B3(pi) ||
			CHIP_4324_B5(pi)) {
			pi->sh->interference_mode_2G = INTERFERE_NONE;
			pi->sh->interference_mode_5G = NON_WLAN;
		} else if (CHIPID_4324X_MEDIA_FAMILY(pi)) {
			pi->sh->interference_mode_2G = WLAN_AUTO;
			pi->sh->interference_mode_5G = INTERFERE_NONE;
		} else {
			pi->sh->interference_mode_2G = INTERFERE_NONE;
			pi->sh->interference_mode_5G = INTERFERE_NONE;
		}
	} else if ((CHIPID(pi->sh->chip) == BCM43236_CHIP_ID) ||
		(CHIPID(pi->sh->chip) == BCM43235_CHIP_ID) ||
		(CHIPID(pi->sh->chip) == BCM43234_CHIP_ID) ||
		(CHIPID(pi->sh->chip) == BCM43238_CHIP_ID) ||
		(CHIPID(pi->sh->chip) == BCM43237_CHIP_ID)) {
		/* assign 2G default interference mode for 4323x chips */
		pi->sh->interference_mode_2G = WLAN_AUTO_W_NOISE;
		pi->sh->interference_mode_5G = NON_WLAN;
	} else if (CHIPID(pi->sh->chip) == BCM43237_CHIP_ID) {
		/* Disable interference mode for 43237 chips */
		pi->sh->interference_mode_2G = WLAN_AUTO;
		pi->sh->interference_mode_5G = INTERFERE_NONE;
	} else {
		pi->sh->interference_mode_2G = WLAN_AUTO;
		pi->sh->interference_mode_5G = NON_WLAN;
	}

	return BCME_OK;
}

void
BCMATTACHFN(phy_n_noise_unregister_impl)(phy_n_noise_info_t *info)
{
	PHY_TRACE(("%s\n", __FUNCTION__));

	if (info != NULL) {
		phy_noise_info_t *ii = info->ii;
		phy_info_t *pi = info->pi;

		/* unregister from common */
		phy_noise_unregister_impl(ii);

		if (pi->interf->aci.nphy != NULL)
			phy_mfree(pi, pi->interf->aci.nphy, sizeof(*(pi->interf->aci.nphy)));

		phy_mfree(pi, info, sizeof(phy_n_noise_info_t));
	}
}

static void
phy_n_noise_calc(phy_type_noise_ctx_t *ctx, int8 cmplx_pwr_dbm[], uint8 extra_gain_1dB)
{
	phy_n_noise_info_t *info = (phy_n_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint8 i;
	uint16 gain = 0;

	gain = wlapi_bmac_read_shm(pi->sh->physhim, M_PWRIND_BLKS(pi) + 0xC);

	FOREACH_CORE(pi, i) {
		if (CHIPID_4324X_EPA_FAMILY(pi)) {
			phy_info_nphy_t *pi_nphy = pi->u.pi_nphy;
			gain = pi_nphy->iqestgain;
			cmplx_pwr_dbm[i] += (int8) (NPHY_NOISE_SAMPLEPWR_TO_DBM - gain);
		} else if (NREV_GE(pi->pubpi->phy_rev, 3)) {
			cmplx_pwr_dbm[i] += (int8) (PHY_NOISE_OFFSETFACT_4322 - gain);
		} else if (NREV_LT(pi->pubpi->phy_rev, 3)) {
			/* assume init gain 70 dB, 128 maps to 1V so
			 * 10*log10(128^2*2/128/128/50)+30=16 dBm
			 * WARNING: if the nphy init gain is ever changed,
			 * this formula needs to be updated
			*/
			cmplx_pwr_dbm[i] += (int8)(16 - (15) * 3 - (70 + extra_gain_1dB));
		}
	}
}

static void
phy_n_noise_calc_fine_resln(phy_type_noise_ctx_t *ctx, int16 cmplx_pwr_dbm[], uint8 extra_gain_1dB,
	int16 *tot_gain)
{
	phy_n_noise_info_t *info = (phy_n_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint8 i;
	uint16 gain;
	int8 offset[] = {0, 0};

	if (CHIPID_4324X_EPA_FAMILY(pi)) {
		/* Adding fine resolution option for 4324B1,B3,B5 */
		phy_info_nphy_t *pi_nphy = pi->u.pi_nphy;
		gain = pi_nphy->iqestgain;
		wlc_phy_rxgain_index_offset(pi, offset);
		FOREACH_CORE(pi, i) {
			cmplx_pwr_dbm[i] += (int16) ((NPHY_NOISE_SAMPLEPWR_TO_DBM - gain) << 2);
			cmplx_pwr_dbm[i] -= (offset[i]);
		}
	} else {
		FOREACH_CORE(pi, i) {
			/* assume init gain 70 dB, 128 maps to 1V so
			 * 10*log10(128^2*2/128/128/50)+30=16 dBm
			 *	WARNING: if the nphy init gain is ever changed,
			 * this formula needs to be updated
			 */
			cmplx_pwr_dbm[i] += ((int16)(16 << 2) - (int16)((15 << 2)*3)
					- (int16)((70 + extra_gain_1dB) << 2));
		}
	}
}

/* set mode */
static void
phy_n_noise_set_mode(phy_type_noise_ctx_t *ctx, int wanted_mode, bool init)
{
	phy_n_noise_info_t *info = (phy_n_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;

	PHY_TRACE(("%s: mode %d init %d\n", __FUNCTION__, wanted_mode, init));

	if (init) {
		pi->interference_mode_crs_time = 0;
		pi->crsglitch_prev = 0;
		/* clear out all the state */
		wlc_phy_noisemode_reset_nphy(pi);
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			wlc_phy_acimode_reset_nphy(pi);
		}
	}

	/* NPHY 5G, supported for NON_WLAN and INTERFERE_NONE only */
	if (CHSPEC_IS2G(pi->radio_chanspec) ||
	    (CHSPEC_IS5G(pi->radio_chanspec) &&
	     (wanted_mode == NON_WLAN || wanted_mode == INTERFERE_NONE))) {
		if (wanted_mode == INTERFERE_NONE) {	/* disable */
			switch (pi->cur_interference_mode) {
			case WLAN_AUTO:
			case WLAN_AUTO_W_NOISE:
			case WLAN_MANUAL:
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					wlc_phy_acimode_set_nphy(pi, FALSE,
						PHY_ACI_PWR_NOTPRESENT);
				}
				pi->aci_state &= ~ACI_ACTIVE;
				break;
			case NON_WLAN:
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					wlc_phy_acimode_set_nphy(pi,
						FALSE,
						PHY_ACI_PWR_NOTPRESENT);
					pi->aci_state &= ~ACI_ACTIVE;
				}
				break;
			}
		} else {	/* Enable */
			switch (wanted_mode) {
			case NON_WLAN:
			case WLAN_AUTO:
			case WLAN_AUTO_W_NOISE:
				/* fall through */
				break;
			case WLAN_MANUAL: {
				int aci_pwr = CHIPID_4324X_MEDIA_FAMILY(pi) ?
					PHY_ACI_PWR_MED : PHY_ACI_PWR_HIGH;
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					wlc_phy_acimode_set_nphy(pi, TRUE, aci_pwr);
				}
				break;
			}
			}
		}
	}
}

static void
phy_n_noise_reset(phy_type_noise_ctx_t *ctx)
{
	phy_n_noise_info_t *info = (phy_n_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	ASSERT(pi->interf->aci.nphy != NULL);

	/* Reset ACI internals if not scanning and not in aci_detection */
	if (!(SCAN_INPROG_PHY(pi) ||
	      pi->interf->aci.nphy->detection_in_progress)) {
		wlc_phy_aci_sw_reset_nphy(pi);
	}
}

/* watchdog callback */
static bool
phy_n_noise_noise_wd(phy_wd_ctx_t *ctx)
{
	phy_n_noise_info_t *ii = (phy_n_noise_info_t *)ctx;
	phy_info_t *pi = ii->pi;

	if (pi->tunings[0]) {
		pi->interf->noise.nphy_noise_assoc_enter_th = pi->tunings[0];
		pi->interf->noise.nphy_noise_noassoc_enter_th = pi->tunings[0];
	}

	if (pi->tunings[2]) {
		pi->interf->noise.nphy_noise_assoc_glitch_th_dn = pi->tunings[2];
		pi->interf->noise.nphy_noise_noassoc_glitch_th_dn = pi->tunings[2];
	}

	if (pi->tunings[1]) {
		pi->interf->noise.nphy_noise_noassoc_glitch_th_up = pi->tunings[1];
		pi->interf->noise.nphy_noise_assoc_glitch_th_up = pi->tunings[1];
	}

	return TRUE;
}

static bool
phy_n_noise_aci_wd(phy_wd_ctx_t *ctx)
{
	phy_n_noise_info_t *ii = (phy_n_noise_info_t *)ctx;
	phy_info_t *pi = ii->pi;

	/* defer interference checking, scan and update if RM is progress */
	if (!SCAN_RM_IN_PROGRESS(pi)) {
		/* interf.scanroamtimer counts transient time coming out of scan */
		if (pi->interf->scanroamtimer != 0)
			pi->interf->scanroamtimer -= 1;

		wlc_phy_aci_upd(pi);

	} else {
		/* in a scan/radio meas, don't update moving average when we
		 * first come out of scan or roam
		 */
		pi->interf->scanroamtimer = 2;
	}

	return TRUE;
}

#ifndef WLC_DISABLE_ACI
static void
phy_n_noise_interf_rssi_update(phy_type_noise_ctx_t *ctx, chanspec_t chanspec, int8 leastRSSI)
{
	phy_n_noise_info_t *info = (phy_n_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;
	if ((CHSPEC_CHANNEL(chanspec) == pi->interf->curr_home_channel)) {
		pi->u.pi_nphy->intf_rssi_avg = leastRSSI;
		pi->interf->rssi = leastRSSI;
	}
}
#endif

static void
phy_n_noise_interf_mode_set(phy_type_noise_ctx_t *ctx, int val)
{
	phy_n_noise_info_t *info = (phy_n_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;
	pi->sh->interference_mode_2G_override = val;
	pi->sh->interference_mode_5G_override = val;
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		/* for 2G, all values 0 thru 4 are valid */
		pi->sh->interference_mode = pi->sh->interference_mode_2G_override;
	} else {
		/* for 5G, only values 0 and 1 are valid options */
		if (val == 0 || val == 1) {
			pi->sh->interference_mode = pi->sh->interference_mode_5G_override;
		} else {
			/* default 5G interference value to 0 */
			pi->sh->interference_mode = 0;
		}
	}
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int
phy_n_noise_dump(phy_type_noise_ctx_t *ctx, struct bcmstrbuf *b)
{
	phy_n_noise_info_t *info = (phy_n_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;

	return phy_noise_dump_common(pi, b);
}
#endif /* BCMDBG || BCMDBG_DUMP */
