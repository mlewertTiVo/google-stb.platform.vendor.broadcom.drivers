/*
 * ACPhy Noise module implementation
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
 * $Id: phy_ac_noise.c 659242 2016-09-13 11:33:09Z kriedte $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_wd.h>
#include <phy_btcx.h>
#include "phy_type_noise.h"
#include <phy_ac.h>
#include <phy_ac_info.h>
#include <phy_ac_noise.h>
#include <phy_ac_chanmgr.h>
#include <phy_ac_rxgcrs.h>
/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_radioreg_20691.h>
#include <wlc_phy_radio.h>
#include <wlc_phyreg_ac.h>

#include <phy_utils_reg.h>
#include <phy_utils_status.h>
#include <phy_ac_info.h>
#include <phy_rstr.h>
#include <phy_utils_var.h>

#define ACPHY_SROM_NOISELVL_OFFSET (-70)

typedef struct acphy_aci_params {
	/* array is indexed by chan/bw */
	uint8 chan;
	uint16 bw;
	uint64 last_updated;

	acphy_desense_values_t desense;
	int8 weakest_rssi;

	desense_history_t bphy_hist;
	desense_history_t ofdm_hist;
	uint8 glitch_buff_idx, glitch_upd_wait;

	uint8 hwaci_setup_state, hwaci_desense_state;
	uint8 hwaci_noaci_timer;
	uint8 hwaci_sleep;
	int  hwaci_desense_state_ovr;
	bool engine_called;
} acphy_aci_params_t;

typedef struct {
	uint16 sample_time;
	uint16 energy_thresh;
	uint16 detect_thresh;
	uint16 wait_period;
	uint8 sliding_window;
	uint8 samp_cluster;
	uint8 nb_lo_th;
	uint8 w3_lo_th;
	uint8 w3_md_th;
	uint8 w3_hi_th;
	uint8 w2;
	uint16 energy_thresh_w2;
	uint8 aci_present_th;
	uint8 aci_present_select;
} acphy_hwaci_setup_t;

/* module private states */
struct phy_ac_noise_info {
	phy_info_t *pi;
	phy_ac_info_t *ac_info;
	phy_noise_info_t *cmn_info;
	/* nvnoiseshapingtbl monitor */
	acphy_nshapetbl_mon_t *nshapetbl_mon;
	acphy_hwaci_state_t hwaci_states_2g[ACPHY_HWACI_MAX_STATES];
	acphy_hwaci_state_t hwaci_states_5g[ACPHY_HWACI_MAX_STATES];
	/* aci params */
	acphy_aci_params_t aci_list2g[ACPHY_ACI_CHAN_LIST_SZ];
	acphy_aci_params_t aci_list5g[ACPHY_ACI_CHAN_LIST_SZ];
	acphy_hwaci_defgain_settings_t def_gains[PHY_CORE_MAX];
	acphy_aci_params_t	*aci;
	acphy_hwaci_setup_t	*hwaci_args;
	int		hwaci_desense_state_ovr;
	uint8	hwaci_max_states_2g, hwaci_max_states_5g;
};

#ifndef WLC_DISABLE_ACI
static uint32 wlc_phy_desense_aci_get_avg_max_glitches_acphy(uint32 glitches[]);
static uint8 wlc_phy_desense_aci_calc_acphy(phy_info_t *pi, desense_history_t *aci_desense,
	uint8 desense, uint32 glitch_cnt, uint16 glitch_th_lo, uint16 glitch_th_hi);
#ifdef WL_ACI_FCBS
static void wlc_phy_init_FCBS_hwaci(phy_info_t *pi);
#endif /* WL_ACI_FCBS */
static void wlc_phy_hwaci_write_table_acphy(phy_info_t *pi, uint16 table_id,
	uint16 start_off, uint8 *table_entry, bool check_5G);
static void wlc_phy_desense_aci_upd_chan_stats_acphy(phy_type_noise_ctx_t *ctx,
	chanspec_t chanspec, int8 leastRSSI);
static acphy_aci_params_t* wlc_phy_desense_aci_getset_chanidx_acphy(phy_info_t *pi,
	chanspec_t chanspec, bool create);
#endif /* !WLC_DISABLE_ACI */

/* local functions */
static void phy_ac_noise_attach_params(phy_ac_noise_info_t *noisei);
static void phy_ac_noise_attach_modes(phy_info_t *pi);
static void phy_ac_noise_calc(phy_type_noise_ctx_t *ctx, int8 cmplx_pwr_dbm[],
		uint8 extra_gain_1dB);
static void phy_ac_noise_calc_fine_resln(phy_type_noise_ctx_t *ctx, int16 cmplx_pwr_dbm[],
		uint8 extra_gain_1dB, int16 *tot_gain);
#ifndef WL_ACI_FCBS
static void phy_ac_noise_aci_mitigate(phy_type_noise_ctx_t *ctx);
#endif /* WL_ACI_FCBS */
static void phy_ac_noise_set_mode(phy_type_noise_ctx_t *ctx, int wanted_mode, bool init);
static bool phy_ac_noise_wd(phy_wd_ctx_t *ctx);
#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(WLTEST)
static int phy_ac_noise_dump(phy_type_noise_ctx_t *ctx, struct bcmstrbuf *b);
#else
#define phy_ac_noise_dump NULL
#endif

static void
BCMATTACHFN(phy_ac_srom_read_noiselvl)(phy_info_t *pi)
{
	/* read noise levels from SROM */
	uint8 core, ant;
	char phy_var_name[20];

	if (phy_get_phymode(pi) == PHYMODE_RSDB &&
		(phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE0))
	{
		/* update pi[0] to hold pwrdet params for all cores */
		/* This is required for mimo operation */
		pi->pubpi->phy_corenum <<= 1;
	}

	FOREACH_CORE(pi, core) {
		/* 2G: */
		ant = phy_get_rsdbbrd_corenum(pi, core);
		(void)snprintf(phy_var_name, sizeof(phy_var_name), rstr_noiselvl2gaD, ant);
		pi->noiselvl_2g[core] = ACPHY_SROM_NOISELVL_OFFSET -
		                             ((uint8)PHY_GETINTVAR(pi, phy_var_name));

		/* 5G low: */
		(void)snprintf(phy_var_name, sizeof(phy_var_name), rstr_noiselvl5gaD, ant);
		pi->noiselvl_5gl[core] = ACPHY_SROM_NOISELVL_OFFSET -
		                              ((uint8)getintvararray(pi->vars, phy_var_name, 0));

		/* 5G mid: */
		pi->noiselvl_5gm[core] = ACPHY_SROM_NOISELVL_OFFSET -
		                              ((uint8)getintvararray(pi->vars, phy_var_name, 1));

		/* 5G high: */
		pi->noiselvl_5gh[core] = ACPHY_SROM_NOISELVL_OFFSET -
		                              ((uint8)getintvararray(pi->vars, phy_var_name, 2));

		/* 5G upper: */
		pi->noiselvl_5gu[core] = ACPHY_SROM_NOISELVL_OFFSET -
		                              ((uint8)getintvararray(pi->vars, phy_var_name, 3));
	}
	if ((phy_get_phymode(pi) == PHYMODE_RSDB) &&
		(phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE0))
	{
		/* update pi[0] to hold pwrdet params for all cores */
		/* This is required for mimo operation */
		pi->pubpi->phy_corenum >>= 1;
	}
}


/* register phy type specific implementation */
phy_ac_noise_info_t *
BCMATTACHFN(phy_ac_noise_register_impl)(phy_info_t *pi, phy_ac_info_t *ac_info,
	phy_noise_info_t *cmn_info)
{
	phy_ac_noise_info_t *noise_info;
	phy_type_noise_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((noise_info = phy_malloc(pi, sizeof(phy_ac_noise_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	if ((noise_info->nshapetbl_mon = phy_malloc(pi, sizeof(acphy_nshapetbl_mon_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc nshapetbl_mon failed\n", __FUNCTION__));
		goto fail;
	}

#ifndef WLC_DISABLE_ACI
	if ((noise_info->hwaci_args = phy_malloc(pi, sizeof(acphy_hwaci_setup_t))) == NULL) {
		PHY_ERROR(("%s: hwaci_args malloc failed\n", __FUNCTION__));
		goto fail;
	}
#endif

	noise_info->pi = pi;
	noise_info->ac_info = ac_info;
	noise_info->cmn_info = cmn_info;

	phy_ac_noise_attach_modes(pi);
	phy_ac_noise_attach_params(noise_info);

#ifndef WLC_DISABLE_ACI
	/* hwaci params */
	if (!(ACPHY_ENABLE_FCBS_HWACI(pi)))
		wlc_phy_hwaci_init_acphy(noise_info);
#endif /* !WLC_DISABLE_ACI */

	/* register watchdog fn */
	if (phy_wd_add_fn(pi->wdi, phy_ac_noise_wd, noise_info,
		PHY_WD_PRD_1TICK, PHY_WD_1TICK_NOISE_ACI,
		PHY_WD_FLAG_NONE) != BCME_OK) {
		PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
		goto fail;
	}

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.mode = phy_ac_noise_set_mode;
	fns.calc = phy_ac_noise_calc;
	fns.calc_fine = phy_ac_noise_calc_fine_resln;
#ifndef WLC_DISABLE_ACI
	fns.interf_rssi_update = wlc_phy_desense_aci_upd_chan_stats_acphy;
#endif
#ifndef WL_ACI_FCBS
	fns.aci_mitigate = phy_ac_noise_aci_mitigate;
#endif /* !WL_ACI_FCBS */
	fns.dump = phy_ac_noise_dump;
	fns.ctx = noise_info;

	phy_ac_srom_read_noiselvl(pi);

	if (phy_noise_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_noise_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return noise_info;

	/* error handling */
fail:
	phy_ac_noise_unregister_impl(noise_info);
	return NULL;
}

void
BCMATTACHFN(phy_ac_noise_attach_params)(phy_ac_noise_info_t *noisei)
{
	phy_info_t *pi = noisei->pi;

	pi->sh->interference_mode = INTERFERE_NONE;

	bzero(noisei->aci_list2g, ACPHY_ACI_CHAN_LIST_SZ * sizeof(acphy_aci_params_t));
	bzero(noisei->aci_list5g, ACPHY_ACI_CHAN_LIST_SZ * sizeof(acphy_aci_params_t));

	noisei->aci = NULL;
}

static void
BCMATTACHFN(phy_ac_noise_attach_modes)(phy_info_t *pi)
{
	if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_1(pi->pubpi->phy_rev) ||
	    ACMAJORREV_5(pi->pubpi->phy_rev)) {
		/* ACPHY_ACI_GLITCHBASED_DESENSE or ACPHY_ACI_HWACI_PKTGAINLMT */
		pi->sh->interference_mode_2G = ACPHY_ACI_GLITCHBASED_DESENSE;
		pi->sh->interference_mode_5G = ACPHY_ACI_GLITCHBASED_DESENSE;
	} else if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
		/* ADD HW_ACI and preemption for 4350 */
		pi->sh->interference_mode_2G |= ACPHY_ACI_GLITCHBASED_DESENSE;
		pi->sh->interference_mode_5G |= ACPHY_ACI_GLITCHBASED_DESENSE;
	}
	if (ACREV_IS(pi->pubpi->phy_rev, 1) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
		/* 4360b0 */
		pi->sh->interference_mode_2G |= ACPHY_ACI_HWACI_PKTGAINLMT |
		        ACPHY_ACI_W2NB_PKTGAINLMT;
		pi->sh->interference_mode_5G |= ACPHY_ACI_HWACI_PKTGAINLMT |
		        ACPHY_ACI_W2NB_PKTGAINLMT;
	} else if (ACREV_IS(pi->pubpi->phy_rev, 0)) {
		/* 4360a0 */
		pi->sh->interference_mode_2G |= ACPHY_ACI_W2NB_PKTGAINLMT;
		pi->sh->interference_mode_5G |= ACPHY_ACI_W2NB_PKTGAINLMT;
	}
	if (ACMAJORREV_1(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) {
		/* 4335C0 */
		pi->sh->interference_mode_2G |= ACPHY_ACI_HWACI_PKTGAINLMT |
		        ACPHY_ACI_W2NB_PKTGAINLMT | ACPHY_ACI_PREEMPTION;
		pi->sh->interference_mode_5G |= ACPHY_ACI_HWACI_PKTGAINLMT |
		        ACPHY_ACI_W2NB_PKTGAINLMT | ACPHY_ACI_PREEMPTION;
	} else if (ACMAJORREV_3(pi->pubpi->phy_rev) && CHIPREV(pi->sh->chiprev) >= 4) {
		/* 4345 B0 onwards */
		pi->sh->interference_mode_2G |= ACPHY_ACI_PREEMPTION | ACPHY_HWACI_MITIGATION;
		pi->sh->interference_mode_5G |= ACPHY_ACI_PREEMPTION;
	} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		/* 4349 A0 onwards */
		pi->sh->interference_mode_2G |= ACPHY_ACI_PREEMPTION | ACPHY_HWACI_MITIGATION
			| ACPHY_ACI_GLITCHBASED_DESENSE;
		pi->sh->interference_mode_5G |= ACPHY_ACI_PREEMPTION | ACPHY_HWACI_MITIGATION
			| ACPHY_ACI_GLITCHBASED_DESENSE;
	} else if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		pi->sh->interference_mode_2G |= ACPHY_ACI_PREEMPTION |
			ACPHY_ACI_GLITCHBASED_DESENSE;
		pi->sh->interference_mode_5G |= ACPHY_ACI_PREEMPTION |
			ACPHY_ACI_GLITCHBASED_DESENSE;
	} else if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		/* 4365 */
		pi->sh->interference_mode_2G = ACPHY_ACI_GLITCHBASED_DESENSE |
				ACPHY_HWACI_MITIGATION;
		pi->sh->interference_mode_5G = ACPHY_ACI_GLITCHBASED_DESENSE |
				ACPHY_HWACI_MITIGATION;

		if (!ACMINORREV_0(pi)) {
			/* B0 and later */
			pi->sh->interference_mode_2G |= ACPHY_ACI_PREEMPTION;
			pi->sh->interference_mode_5G |= ACPHY_ACI_PREEMPTION;
		}
	} else if (ACMAJORREV_37(pi->pubpi->phy_rev)) {
		/* 7271 */
		/* pi->sh->interference_mode_2G = ACPHY_ACI_GLITCHBASED_DESENSE |
		        ACPHY_HWACI_MITIGATION;
		   pi->sh->interference_mode_5G = ACPHY_ACI_GLITCHBASED_DESENSE |
		        ACPHY_HWACI_MITIGATION;
		*/
		pi->sh->interference_mode_2G = ACPHY_ACI_GLITCHBASED_DESENSE;
		pi->sh->interference_mode_5G = ACPHY_ACI_GLITCHBASED_DESENSE;
		/* Enable preemption/LPD by default */
		pi->sh->interference_mode_2G |= ACPHY_ACI_PREEMPTION | ACPHY_LPD_PREEMPTION;
		pi->sh->interference_mode_5G |= ACPHY_ACI_PREEMPTION | ACPHY_LPD_PREEMPTION;
	}

	/* Enable preemption for 4364 slices */
	if (IS_4364_1x1(pi)) {
		pi->sh->interference_mode_2G = ACPHY_ACI_PREEMPTION | ACPHY_HWACI_MITIGATION;
		pi->sh->interference_mode_5G = ACPHY_ACI_PREEMPTION;
	}

	if (IS_4364_3x3(pi)) {
		pi->sh->interference_mode_2G = ACPHY_ACI_PREEMPTION | ACPHY_HWACI_MITIGATION
			| ACPHY_ACI_GLITCHBASED_DESENSE;
		pi->sh->interference_mode_5G = ACPHY_ACI_PREEMPTION | ACPHY_HWACI_MITIGATION
			| ACPHY_ACI_GLITCHBASED_DESENSE;
	}
}

void
BCMATTACHFN(phy_ac_noise_unregister_impl)(phy_ac_noise_info_t *noise_info)
{
	phy_info_t *pi;
	phy_noise_info_t *cmn_info;

	if (noise_info == NULL) {
		return;
	}

	pi = noise_info->pi;
	cmn_info = noise_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_noise_unregister_impl(cmn_info);

#ifndef WLC_DISABLE_ACI
	if (noise_info->hwaci_args != NULL) {
		phy_mfree(pi, noise_info->hwaci_args, sizeof(acphy_hwaci_setup_t));
	}
#endif
	if (noise_info->nshapetbl_mon != NULL) {
		phy_mfree(pi, noise_info->nshapetbl_mon, sizeof(acphy_nshapetbl_mon_t));
	}
	phy_mfree(pi, noise_info, sizeof(phy_ac_noise_info_t));
}

static void
phy_ac_noise_calc(phy_type_noise_ctx_t *ctx, int8 cmplx_pwr_dbm[], uint8 extra_gain_1dB)
{
	phy_ac_noise_info_t *noise_info = (phy_ac_noise_info_t *)ctx;
	phy_info_t *pi = noise_info->pi;
	/* init gain is fixed to -65 for acphy,
	 *-37 = 10*log10((0.4/512)*(0.4/512)*(16)*(1/50.0))
	*/
	/* knoise support, read gain value from shm, by Peyush */
	int16 assumed_gain;
	uint8 i;

	FOREACH_CORE(pi, i) {
		if (BFCTL(pi->u.pi_acphy) == 2) {
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				assumed_gain = (int16)(ACPHY_NOISE_INITGAIN_X29_2G);
			} else {
				assumed_gain = (int16)(ACPHY_NOISE_INITGAIN_X29_5G);
			}
		} else {
			assumed_gain = (int16)(ACPHY_NOISE_INITGAIN);
		}

		assumed_gain += extra_gain_1dB;

		/* JIRA:CRDOT11ACPHY-1542: rx_iq_est is increased to 10 bits */
		if (ACMAJORREV_33(pi->pubpi->phy_rev) || ACMAJORREV_37(pi->pubpi->phy_rev)) {
			cmplx_pwr_dbm[i] += (int16) ((ACPHY_NOISE_SAMPLEPWR_TO_DBM_10BIT -
				assumed_gain));
		} else if ((ACMAJORREV_36(pi->pubpi->phy_rev))) {
			cmplx_pwr_dbm[i] += (int16) ((ACPHY_NOISE_SAMPLEPWR_TO_DBM_43012
						- assumed_gain));
		} else {
			cmplx_pwr_dbm[i] += (int16) ((ACPHY_NOISE_SAMPLEPWR_TO_DBM - assumed_gain));
		}
		pi->u.pi_acphy->phy_noise_all_core[i] =  cmplx_pwr_dbm[i] + (assumed_gain);
	}
}

static void
phy_ac_noise_calc_fine_resln(phy_type_noise_ctx_t *ctx, int16 cmplx_pwr_dbm[],
	uint8 extra_gain_1dB, int16 *tot_gain)
{
	phy_ac_noise_info_t *noise_info = (phy_ac_noise_info_t *)ctx;
	phy_info_t *pi = noise_info->pi;
	int16 assumed_gain;
	uint8 i;

	FOREACH_CORE(pi, i) {
		assumed_gain = tot_gain[i];
		if (assumed_gain == ACPHY_NOISE_RXGAIN_UNSPECIFIED) {
			if (BFCTL(pi->u.pi_acphy) == 2) {
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					assumed_gain = (int16)(ACPHY_NOISE_INITGAIN_X29_2G);
				} else {
					assumed_gain = (int16)(ACPHY_NOISE_INITGAIN_X29_5G);
				}
			} else {
				assumed_gain = (int16)(ACPHY_NOISE_INITGAIN);
			}
		}
		assumed_gain += extra_gain_1dB;

		cmplx_pwr_dbm[i] += (int16) ((ACPHY_NOISE_SAMPLEPWR_TO_DBM -
				assumed_gain) << 2);
	}
}

/* set mode */
static void
phy_ac_noise_set_mode(phy_type_noise_ctx_t *ctx, int wanted_mode, bool init)
{
	phy_ac_noise_info_t *noise_info = (phy_ac_noise_info_t *)ctx;
	phy_info_t *pi = noise_info->pi;

	PHY_TRACE(("%s: mode %d init %d\n", __FUNCTION__, wanted_mode, init));

	if (init) {
		pi->interference_mode_crs_time = 0;
		pi->crsglitch_prev = 0;
	}

	/* Depending on interfernece modes, do whatever needs to be done */
	if (wanted_mode == INTERFERE_NONE) {
#ifndef WLC_DISABLE_ACI
		wlc_phy_desense_aci_reset_params_acphy(pi, TRUE, TRUE, TRUE);
		if (ACPHY_ENABLE_FCBS_HWACI(pi) || ((ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev) || ACMAJORREV_37(pi->pubpi->phy_rev)) &&
			((pi->sh->interference_mode & ACPHY_HWACI_MITIGATION) != 0)))
			wlc_phy_hwaci_mitigation_enable_acphy(pi, HWACI_DISABLE, FALSE);
		else
			wlc_phy_hwaci_setup_acphy(pi, FALSE, FALSE);
#endif
		wlc_phy_aci_w2nb_setup_acphy(pi, FALSE);
		wlc_phy_preempt(pi, FALSE, FALSE);
	}

	/* Nothing needs to be done for ACPHY_ACI_GLITCHBASED_DESENSE */
#ifndef WLC_DISABLE_ACI
	/* Switch on( & init) the required rssi settings */
	if ((pi->sh->interference_mode & ACPHY_ACI_HWACI_PKTGAINLMT) != 0) {
		if (!ACPHY_ENABLE_FCBS_HWACI(pi))
			wlc_phy_hwaci_setup_acphy(pi, TRUE, TRUE);
	} else if ((pi->sh->interference_mode & ACPHY_HWACI_MITIGATION) != 0) {
		if (!ACMAJORREV_32(pi->pubpi->phy_rev) && !ACMAJORREV_33(pi->pubpi->phy_rev) &&
			!ACMAJORREV_37(pi->pubpi->phy_rev) && !ACPHY_ENABLE_FCBS_HWACI(pi))
			wlc_phy_hwaci_setup_acphy(pi, TRUE, TRUE);
		else
#ifdef WL_ACI_FCBS
			wlc_phy_hwaci_mitigation_enable_acphy(pi, HWACI_AUTO_FCBS, TRUE);
#else
			wlc_phy_hwaci_mitigation_enable_acphy(pi, HWACI_AUTO_SW, TRUE);
#endif /* WL_ACI_FCBS */
	}
#endif /* WLC_DISABLE_ACI */
	if (!ACPHY_ENABLE_FCBS_HWACI(pi) &&
		(pi->sh->interference_mode & ACPHY_ACI_W2NB_PKTGAINLMT) != 0) {
		wlc_phy_aci_w2nb_setup_acphy(pi, TRUE);
	}
	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
	    ACMAJORREV_37(pi->pubpi->phy_rev)) {
#ifndef WLC_DISABLE_ACI
		if ((pi->sh->interference_mode & ACPHY_HWACI_MITIGATION) == 0) {
			wlc_phy_hwaci_mitigation_enable_acphy(pi, HWACI_DISABLE, FALSE);
		}
#endif
	}
	if ((pi->sh->interference_mode & ACPHY_ACI_PREEMPTION) != 0)
		/* Configured in Pre-Filter Processing mode */
		wlc_phy_preempt(pi, TRUE, FALSE);
}

/* watchdog callback */
static bool
phy_ac_noise_wd(phy_wd_ctx_t *ctx)
{
	phy_ac_noise_info_t *noise_info = (phy_ac_noise_info_t *)ctx;
	phy_info_t *pi = noise_info->pi;

	/* defer interference checking, scan and update if RM is progress */
	if (!SCAN_RM_IN_PROGRESS(pi)) {
		wlc_phy_aci_upd(pi);
	}

	return TRUE;
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(WLTEST)
static int
phy_ac_noise_dump(phy_type_noise_ctx_t *ctx, struct bcmstrbuf *b)
{
	phy_ac_noise_info_t *noise_info = (phy_ac_noise_info_t *)ctx;
	phy_info_t *pi = noise_info->pi;
	uint8 bw;
	acphy_desense_values_t desense;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	int8 rssi = 0;

	if (SCAN_RM_IN_PROGRESS(pi)) {
		bcm_bprintf(b, "Scanning is in progress. Can't dump aci mitigation info.\n");
		return BCME_ERROR;
	}

	if (noise_info->aci != NULL)
		rssi = noise_info->aci->weakest_rssi;

	bw = CHSPEC_BW_LE20(pi->radio_chanspec) ? 20 : (CHSPEC_IS40(pi->radio_chanspec) ? 40 : 80);

	/* Get total desense based on aci & bt */
	desense = phy_ac_rxgcrs_get_desense(pi_ac->rxgcrsi, TOTAL_DESENSE);
	bcm_bprintf(b, "\n");
	if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
	    ACMAJORREV_33(pi->pubpi->phy_rev) ||
	    ACMAJORREV_37(pi->pubpi->phy_rev)) {
		bcm_bprintf(b, "*** Channel = %d(%d mhz), Weakest RSSI = %d, Associated = %d ** \n",
		            CHSPEC_CHANNEL(pi->radio_chanspec), bw, rssi,
		            !(ASSOC_INPROG_PHY(pi) || PUB_NOT_ASSOC(pi)));
	} else {
		bcm_bprintf(b, "*** Channel = %d(%d mhz), Desense(mode 1) On = %d *** \n",
		            CHSPEC_CHANNEL(pi->radio_chanspec), bw, desense.on);
	}
	bcm_bprintf(b, "OFDM desense (dB) = %d\n", desense.ofdm_desense);
	bcm_bprintf(b, "BPHY desense (dB) = %d\n", desense.bphy_desense);
	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		if (noise_info->aci != NULL) {
			bcm_bprintf(b, "HWACI mitigation triggered = %d\n",
					noise_info->aci->hwaci_desense_state);
		} else {
			bcm_bprintf(b, "No mitigation information available: pi_ac->aci is NULL\n");
		}
	} else {
		bcm_bprintf(b, "lna1 tbl desense (ticks) = %d\n", desense.lna1_tbl_desense);
		bcm_bprintf(b, "lna2 tbl desense (ticks) = %d\n", desense.lna2_tbl_desense);
		bcm_bprintf(b, "lna1 pktgain limit (ticks) = %d\n", desense.lna1_gainlmt_desense);
		bcm_bprintf(b, "lna2 pktgain limit (ticks) = %d\n", desense.lna2_gainlmt_desense);
		bcm_bprintf(b, "elna bypass = %d\n", desense.elna_bypass);
		bcm_bprintf(b, "forced = %d\n", desense.forced);
	}
	if (ACHWACIREV(pi)) {
		bcm_bprintf(b, "hwaci detected = %d\n", pi_ac->hw_aci_status);
	}
	bcm_bprintf(b, "\n");

	return BCME_OK;
}
#endif /* BCMDBG || BCMDBG_DUMP || defined(WLTEST) */

void
chanspec_noise(phy_info_t *pi)
{
	pi->u.pi_acphy->noisei->aci = NULL;

	if (!wlc_phy_is_scan_chan_acphy(pi)) {
		pi->interf->curr_home_channel = CHSPEC_CHANNEL(pi->radio_chanspec);
#ifndef WLC_DISABLE_ACI
		/* Get pointer to current aci channel list */
		if (!ACPHY_ENABLE_FCBS_HWACI(pi) || ACPHY_HWACI_WITH_DESENSE_ENG(pi)) {
			pi->u.pi_acphy->noisei->aci = wlc_phy_desense_aci_getset_chanidx_acphy(pi,
				pi->radio_chanspec, TRUE);
		}
#endif /* !WLC_DISABLE_ACI */
	}

	if (ACMAJORREV_40(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev) || ACMAJORREV_37(pi->pubpi->phy_rev)) {
		return;
	}
	if (ACHWACIREV(pi)) {
		wlc_phy_hwaci_setup_acphy(pi, TRUE, TRUE);
		pi->u.pi_acphy->hw_aci_status = FALSE;
#ifndef WLC_DISABLE_ACI
		wlc_phy_save_def_gain_settings_acphy(pi);
#endif
	}
}

#ifndef WLC_DISABLE_ACI

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */
static uint32
wlc_phy_desense_aci_get_avg_max_glitches_acphy(uint32 glitches[])
{
	uint8 i, j;
	uint32 max_glitch, glitch_cnt = 0;
	uint8 max_glitch_idx;
	uint32 glitches_sort[ACPHY_ACI_GLITCH_BUFFER_SZ];

	for (i = 0; i < ACPHY_ACI_GLITCH_BUFFER_SZ; i++)
		glitches_sort[i] = glitches[i];

	/* Get 2 max from the list */
	for (j = 0; j < ACPHY_ACI_NUM_MAX_GLITCH_AVG; j++) {
		max_glitch_idx = 0;
		max_glitch = glitches_sort[0];
		for (i = 1; i <  ACPHY_ACI_GLITCH_BUFFER_SZ; i++) {
			if (glitches_sort[i] > max_glitch) {
				max_glitch_idx = i;
				max_glitch = glitches_sort[i];
			}
		}
		glitches_sort[max_glitch_idx] = 0;
		glitch_cnt += max_glitch;
	}

	/* avg */
	glitch_cnt /= ACPHY_ACI_NUM_MAX_GLITCH_AVG;
	return glitch_cnt;
}

static uint8
wlc_phy_desense_aci_calc_acphy(phy_info_t *pi, desense_history_t *aci_desense, uint8 desense,
	uint32 glitch_cnt, uint16 glitch_th_lo, uint16 glitch_th_hi)
{
	uint8 hi, lo, cnt, cnt_thresh = 0;

	hi = aci_desense->hi_glitch_dB;
	lo = aci_desense->lo_glitch_dB;
	cnt = aci_desense->no_desense_change_time_cnt;

	if (glitch_cnt > glitch_th_hi) {
		hi = desense;
		if (hi >= lo) desense += ACPHY_ACI_COARSE_DESENSE_UP;
		else desense = MAX(desense + 1, (hi + lo) >> 1);
		cnt = 0;
	} else {
		/* Sleep for different times under different conditions */
		if (desense - hi == 1) {
			cnt_thresh = ACPHY_ACI_BORDER_GLITCH_SLEEP;
		} else {
			cnt_thresh = (glitch_cnt >= glitch_th_lo) ? ACPHY_ACI_MD_GLITCH_SLEEP :
			        ACPHY_ACI_LO_GLITCH_SLEEP;
		}

		if (cnt > cnt_thresh) {
			lo = desense;
			if (lo <= hi) desense = MAX(0, desense - ACPHY_ACI_COARSE_DESENSE_DN);
			else desense = MAX(0, MIN(desense - 1, (hi + lo) >> 1));
			cnt = 0;
		}
	}

	PHY_ACI(("aci_mode1, lo = %d, hi = %d, desense = %d, cnt = %d(%d)\n",  lo, hi, desense,
	         cnt, cnt_thresh));

	/* Update the values */
	aci_desense->hi_glitch_dB = hi;
	aci_desense->lo_glitch_dB = lo;
	aci_desense->no_desense_change_time_cnt = MIN(cnt + 1, 255);

	return desense;
}


/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */

/********** DESENSE (ACI, CCI, Noise - glitch based) ******** */

acphy_desense_values_t*
phy_ac_noise_get_desense(phy_ac_noise_info_t *noisei)
{
	if (noisei->aci != NULL) {
		return &noisei->aci->desense;
	} else {
		return NULL;
	}
}

uint8 phy_ac_noise_get_desense_state(phy_ac_noise_info_t *noisei)
{
	if (noisei->aci != NULL) {
		return noisei->aci->hwaci_desense_state;
	} else {
		return 0;
	}
}

#ifndef WLC_DISABLE_ACI
void
wlc_phy_hwaci_init_acphy(phy_ac_noise_info_t *noisei)
{
	phy_info_t *pi = noisei->pi;
	phy_info_acphy_t *pi_ac = noisei->ac_info;

	BCM_REFERENCE(pi);

	/* common */
	noisei->hwaci_args->energy_thresh = 0x3e8;
	noisei->hwaci_args->detect_thresh = 0x1f4;
	noisei->hwaci_args->wait_period = 0x1;
	noisei->hwaci_args->sliding_window = 0xf;
	noisei->hwaci_args->samp_cluster = 0xf;
	noisei->hwaci_args->w3_lo_th = 0x0;
	noisei->hwaci_args->w3_md_th = 0x0;
	noisei->hwaci_args->w3_hi_th = 0x0;
	noisei->hwaci_args->w2 = 4;

	/* <= ACPHY_HWACI_MAX_STATES */
	noisei->hwaci_max_states_2g = 4;
	noisei->hwaci_max_states_5g = 4;

	/* 2g */
	bzero(noisei->hwaci_states_2g, ACPHY_HWACI_MAX_STATES * sizeof(acphy_hwaci_state_t));
	/* hwaci states: 0, no ACI settings */
	noisei->hwaci_states_2g[0].energy_thresh = 0xffff;   /* don't care */
	noisei->hwaci_states_2g[0].w2_sel = 0;
	noisei->hwaci_states_2g[0].w2_thresh = 30;
	noisei->hwaci_states_2g[0].nb_thresh = 4;
	noisei->hwaci_states_2g[0].lna1_pktg_lmt = 5;
	noisei->hwaci_states_2g[0].lna2_pktg_lmt = 6;
	noisei->hwaci_states_2g[0].lna1rout_pktg_lmt = 0;

	/* hwaci states: 1, check/desense for > -45dBm */
	noisei->hwaci_states_2g[1].w2_sel = 0;

	/* hwaci states: 2, check/desense for > -40dBm */
	noisei->hwaci_states_2g[2].w2_sel = 1;

	/* hwaci states: 3, check/desense for > -35dBm */
	noisei->hwaci_states_2g[3].w2_sel = 2;

	/* hwaci states: 4, check/desense */
	noisei->hwaci_states_5g[4].w2_sel = 2;

	/* 5g */
	bzero(noisei->hwaci_states_5g, ACPHY_HWACI_MAX_STATES * sizeof(acphy_hwaci_state_t));
	/* hwaci states: 0, no ACI settings */
	noisei->hwaci_states_5g[0].energy_thresh = 0xffff;   /* don't care */
	noisei->hwaci_states_5g[0].w2_sel = 0;
	noisei->hwaci_states_5g[0].w2_thresh = 30;
	noisei->hwaci_states_5g[0].nb_thresh = 4;
	noisei->hwaci_states_5g[0].lna1_pktg_lmt = 5;
	noisei->hwaci_states_5g[0].lna2_pktg_lmt = 6;
	noisei->hwaci_states_5g[0].lna1rout_pktg_lmt = 4;

	/* hwaci states: 1, check/desense */
	noisei->hwaci_states_5g[1].w2_sel = 0;

	/* hwaci states: 2, check/desense */
	noisei->hwaci_states_5g[2].w2_sel = 1;

	/* hwaci states: 3, check/desense */
	noisei->hwaci_states_5g[3].w2_sel = 2;

	/* hwaci states: 4, check/desense */
	noisei->hwaci_states_5g[4].w2_sel = 2;

	/* 4335C0 */
	if (ACMAJORREV_1(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) {
		/* hwaci params */
		noisei->hwaci_args->sample_time = 300;
		noisei->hwaci_args->nb_lo_th = 0x0;
		/* 2g */
		noisei->hwaci_states_2g[1].energy_thresh = 4000;
		noisei->hwaci_states_2g[1].w2_thresh = 30;
		noisei->hwaci_states_2g[1].nb_thresh = 10;
		noisei->hwaci_states_2g[1].lna1_pktg_lmt = 4;
		noisei->hwaci_states_2g[1].lna2_pktg_lmt = 4;
		noisei->hwaci_states_2g[1].lna1rout_pktg_lmt = 0;

		noisei->hwaci_states_2g[2].energy_thresh = 9000;
		noisei->hwaci_states_2g[2].w2_thresh = 16;
		noisei->hwaci_states_2g[2].nb_thresh = 10;
		noisei->hwaci_states_2g[2].lna1_pktg_lmt = 3;
		noisei->hwaci_states_2g[2].lna2_pktg_lmt = 4;
		noisei->hwaci_states_2g[2].lna1rout_pktg_lmt = 0;

		noisei->hwaci_states_2g[3].energy_thresh = 14000;
		noisei->hwaci_states_2g[3].w2_thresh = 2;
		noisei->hwaci_states_2g[3].nb_thresh = 10;
		noisei->hwaci_states_2g[3].lna1_pktg_lmt = 3;
		noisei->hwaci_states_2g[3].lna2_pktg_lmt = 3;
		noisei->hwaci_states_2g[3].lna1rout_pktg_lmt = 0;

		/* 5g */
		noisei->hwaci_states_5g[1].energy_thresh = 1000;
		noisei->hwaci_states_5g[1].w2_thresh = 30;
		noisei->hwaci_states_5g[1].nb_thresh = 4;
		noisei->hwaci_states_5g[1].lna1_pktg_lmt = 4;
		noisei->hwaci_states_5g[1].lna2_pktg_lmt = 4;
		noisei->hwaci_states_5g[1].lna1rout_pktg_lmt = 4;

		noisei->hwaci_states_5g[2].energy_thresh = 11000;
		noisei->hwaci_states_5g[2].w2_thresh = 25;
		noisei->hwaci_states_5g[2].nb_thresh = 4;
		noisei->hwaci_states_5g[2].lna1_pktg_lmt = 3;
		noisei->hwaci_states_5g[2].lna2_pktg_lmt = 4;
		noisei->hwaci_states_5g[2].lna1rout_pktg_lmt = 4;

		noisei->hwaci_states_5g[3].energy_thresh = 17000;
		noisei->hwaci_states_5g[3].w2_thresh = 15;
		noisei->hwaci_states_5g[3].nb_thresh = 8;
		noisei->hwaci_states_5g[3].lna1_pktg_lmt = 3;
		noisei->hwaci_states_5g[3].lna2_pktg_lmt = 3;
		noisei->hwaci_states_5g[3].lna1rout_pktg_lmt = 4;
	}

	/* 4360B0 */
	if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
		/* hwaci params */
		noisei->hwaci_args->sample_time = 300;
		noisei->hwaci_args->nb_lo_th = 0x1;
		/* 2g */
		noisei->hwaci_states_2g[1].energy_thresh = 4000;
		noisei->hwaci_states_2g[1].w2_thresh = 30;
		noisei->hwaci_states_2g[1].nb_thresh = 4;
		noisei->hwaci_states_2g[1].lna1_pktg_lmt = 5;
		noisei->hwaci_states_2g[1].lna2_pktg_lmt = 4;
		noisei->hwaci_states_2g[1].lna1rout_pktg_lmt = 0;

		noisei->hwaci_states_2g[2].energy_thresh = 8000;
		noisei->hwaci_states_2g[2].w2_thresh = 22;
		noisei->hwaci_states_2g[2].nb_thresh = 4;
		noisei->hwaci_states_2g[2].lna1_pktg_lmt = 4;
		noisei->hwaci_states_2g[2].lna2_pktg_lmt = 4;
		noisei->hwaci_states_2g[2].lna1rout_pktg_lmt = 0;

		noisei->hwaci_states_2g[3].energy_thresh = 11000;
		noisei->hwaci_states_2g[3].w2_thresh = 10;
		noisei->hwaci_states_2g[3].nb_thresh = 4;
		noisei->hwaci_states_2g[3].lna1_pktg_lmt = 3;
		noisei->hwaci_states_2g[3].lna2_pktg_lmt = 4;
		noisei->hwaci_states_2g[3].lna1rout_pktg_lmt = 0;

		/* 5g */
		if (ACREV_IS(pi->pubpi->phy_rev, 0)) {
			/* 4360 A0 */
			noisei->hwaci_states_5g[1].energy_thresh = 1000;
			noisei->hwaci_states_5g[1].w2_thresh = 30;
			noisei->hwaci_states_5g[1].nb_thresh = 4;
			noisei->hwaci_states_5g[1].lna1_pktg_lmt = 5;
			noisei->hwaci_states_5g[1].lna2_pktg_lmt = 4;
			noisei->hwaci_states_5g[1].lna1rout_pktg_lmt = 4;

			noisei->hwaci_states_5g[2].energy_thresh = 6000;
			noisei->hwaci_states_5g[2].w2_thresh = 25;
			noisei->hwaci_states_5g[2].nb_thresh = 4;
			noisei->hwaci_states_5g[2].lna1_pktg_lmt = 4;
			noisei->hwaci_states_5g[2].lna2_pktg_lmt = 4;
			noisei->hwaci_states_5g[2].lna1rout_pktg_lmt = 4;

			noisei->hwaci_states_5g[3].energy_thresh = 10000;
			noisei->hwaci_states_5g[3].w2_thresh = 15;
			noisei->hwaci_states_5g[3].nb_thresh = 4;
			noisei->hwaci_states_5g[3].lna1_pktg_lmt = 3;
			noisei->hwaci_states_5g[3].lna2_pktg_lmt = 4;
			noisei->hwaci_states_5g[3].lna1rout_pktg_lmt = 4;
		} else {
			/* 4360 B0, 43602 */
			noisei->hwaci_max_states_5g = 5;
			noisei->hwaci_states_5g[1].energy_thresh = 3000;
			noisei->hwaci_states_5g[1].w2_thresh = 30;
			noisei->hwaci_states_5g[1].nb_thresh = 4;
			noisei->hwaci_states_5g[1].lna1_pktg_lmt = 5;
			noisei->hwaci_states_5g[1].lna2_pktg_lmt = 4;
			noisei->hwaci_states_5g[1].lna1rout_pktg_lmt = 4;

			noisei->hwaci_states_5g[2].energy_thresh = 8000;
			noisei->hwaci_states_5g[2].w2_thresh = 15;
			noisei->hwaci_states_5g[2].nb_thresh = 4;
			noisei->hwaci_states_5g[2].lna1_pktg_lmt = 5;
			noisei->hwaci_states_5g[2].lna2_pktg_lmt = 4;
			noisei->hwaci_states_5g[2].lna1rout_pktg_lmt = 2;

			noisei->hwaci_states_5g[3].energy_thresh = 11000;
			noisei->hwaci_states_5g[3].w2_thresh = 8;
			noisei->hwaci_states_5g[3].nb_thresh = 4;
			noisei->hwaci_states_5g[3].lna1_pktg_lmt = 5;
			noisei->hwaci_states_5g[3].lna2_pktg_lmt = 4;
			noisei->hwaci_states_5g[3].lna1rout_pktg_lmt = 0;

			noisei->hwaci_states_5g[4].energy_thresh = 12000;
			noisei->hwaci_states_5g[4].w2_thresh = 25;
			noisei->hwaci_states_5g[4].nb_thresh = 4;
			noisei->hwaci_states_5g[4].lna1_pktg_lmt = 3;
			noisei->hwaci_states_5g[4].lna2_pktg_lmt = 4;
			noisei->hwaci_states_5g[4].lna1rout_pktg_lmt = 4;
		}
	}

	if (ACHWACIREV(pi)) {
		/* HW ACI Detection SW ACI Mitigation
		 * These settings have been tested well,
		 * with only LNA back off's and using no Routs.
		 */

		/* hwaci params */
		noisei->hwaci_args->energy_thresh = 3000;
		noisei->hwaci_args->energy_thresh_w2 = 3000;
		noisei->hwaci_args->detect_thresh = 1000;
		noisei->hwaci_args->nb_lo_th = 0x1;
		noisei->hwaci_args->wait_period = 0x0;
		noisei->hwaci_args->sliding_window = 0xf;
		noisei->hwaci_args->samp_cluster = 0x5;
		noisei->hwaci_args->w3_lo_th = 0x0;
		noisei->hwaci_args->w3_md_th = 0x0;
		noisei->hwaci_args->w3_hi_th = 0x1;
		noisei->hwaci_args->w2 = 10;
		noisei->hwaci_args->aci_present_th = 0;	/* 0 = 1/8; 1 = 1/4; */
		noisei->hwaci_args->aci_present_select = 2; /* 2 = W3 || W12  */
		noisei->hwaci_desense_state_ovr = -1;
		pi_ac->gain_idx_forced  = 0xffff;

		if (((pi->sh->interference_mode & ACPHY_HWACI_MITIGATION) != 0) ||
			((pi->sh->interference_mode_2G & ACPHY_HWACI_MITIGATION) != 0) ||
			((pi->sh->interference_mode_5G& ACPHY_HWACI_MITIGATION) != 0)) {
			/* HW Swithching makes a decision in 100 ms */
			noisei->hwaci_args->sample_time = 100;
		} else if (((pi->sh->interference_mode & ACPHY_ACI_HWACI_PKTGAINLMT) != 0) ||
			((pi->sh->interference_mode_2G & ACPHY_ACI_HWACI_PKTGAINLMT) != 0) ||
			((pi->sh->interference_mode_5G & ACPHY_ACI_HWACI_PKTGAINLMT) != 0)) {
			/* SW Swithching makes a decision in 990 ms */
			noisei->hwaci_args->sample_time = 990;
		}
	}
}


void
wlc_phy_save_def_gain_settings_acphy(phy_info_t *pi)
{
	uint8 core;

	acphy_hwaci_defgain_settings_t *pi_ac_dgain;

	ASSERT(pi->u.pi_acphy->noisei != NULL);
	ASSERT(pi->u.pi_acphy->noisei->hwaci_args != NULL);

	pi_ac_dgain = pi->u.pi_acphy->noisei->def_gains;

	FOREACH_CORE(pi, core) {
		if (!ACMAJORREV_5(pi->pubpi->phy_rev)) {
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_GAINLIMIT,
			6, 8, 8, pi_ac_dgain[core].lna1_gainlim_ofdm_def);
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_GAINLIMIT,
			6, 72, 8, pi_ac_dgain[core].lna1_gainlim_cck_def);
		} else {
			wlc_phy_table_read_acphy(pi, (core == 0) ? ACPHY_TBL_ID_GAINLIMIT0:
				(core == 1) ? ACPHY_TBL_ID_GAINLIMIT1: ACPHY_TBL_ID_GAINLIMIT2,
				6, 8, 8, pi_ac_dgain[core].lna1_gainlim_ofdm_def);
			wlc_phy_table_read_acphy(pi, (core == 0) ? ACPHY_TBL_ID_GAINLIMIT0:
				(core == 1) ? ACPHY_TBL_ID_GAINLIMIT1: ACPHY_TBL_ID_GAINLIMIT2,
				6, 72, 8, pi_ac_dgain[core].lna1_gainlim_cck_def);
		}

		if (!ACMAJORREV_5(pi->pubpi->phy_rev)) {
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_GAINLIMIT,
				6, 17, 8, pi_ac_dgain[core].lna2_gainlim_ofdm_def);
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_GAINLIMIT,
				6, 81, 8, pi_ac_dgain[core].lna2_gainlim_cck_def);
		} else {
			wlc_phy_table_read_acphy(pi, (core == 0) ? ACPHY_TBL_ID_GAINLIMIT0:
				(core == 1) ? ACPHY_TBL_ID_GAINLIMIT1: ACPHY_TBL_ID_GAINLIMIT2,
				6, 17, 8, pi_ac_dgain[core].lna2_gainlim_ofdm_def);
			wlc_phy_table_read_acphy(pi, (core == 0) ? ACPHY_TBL_ID_GAINLIMIT0:
				(core == 1) ? ACPHY_TBL_ID_GAINLIMIT1: ACPHY_TBL_ID_GAINLIMIT2,
				6, 81, 8, pi_ac_dgain[core].lna2_gainlim_cck_def);
		}
		wlc_phy_table_read_acphy(pi, (core == 0) ? ACPHY_TBL_ID_GAIN0:
			(core == 1) ? ACPHY_TBL_ID_GAIN1: ACPHY_TBL_ID_GAIN2,
			6, 17, 8, pi_ac_dgain[core].lna2_gaindb_def);
		wlc_phy_table_read_acphy(pi, (core == 0) ? ACPHY_TBL_ID_GAINBITS0:
			(core == 1) ? ACPHY_TBL_ID_GAINBITS1: ACPHY_TBL_ID_GAINBITS2,
			6, 17, 8, pi_ac_dgain[core].lna2_gainbits_def);
	}
}

void
wlc_phy_hwaci_mitigate_acphy(phy_info_t *pi, bool aci_status)
{
	acphy_hwaci_defgain_settings_t *pi_ac_dgain;

	/* These are HW ACI Mitigation settings (to be written to FCBS table) */
	uint8 *lna1_gainlim_ofdm;
	uint8 *lna1_gainlim_cck;

	uint8 *lna2_gainlim_ofdm;
	uint8 *lna2_gaindb;
	uint8 *lna2_gainbits;

	uint8 lna1_gainlim_ofdm_updated = 0;
	uint8 lna1_gainlim_cck_updated  = 0;

	uint8 lna2_gainlim_ofdm_updated = 0;
	uint8 lna2_gaindb_updated = 0;
	uint8 lna2_gainbits_updated = 0;


	uint8 core;
	uint16 table_id_core[PHY_CORE_MAX];
	uint16 start_off_core[PHY_CORE_MAX];

	/* If any of the 10 settings (7 for 5G) are changed,
	* they will be initialized to new values here
	* If they are not being changed, they will be read
	* from the phytable, so that they are initialized.
	*/
	uint8 lna1_gainlim_ofdm_2g_aci[6] = {0xb, 0xc, 0xe, 0x20, 0x7f, 0x7f};
	uint8 lna1_gainlim_cck_2g_aci[6] = {0xb, 0xc, 0xe, 0xf, 0x7f, 0x7f};

	uint8 lna2_gaindb_2g_aci[6] = {-8, -4, -1, 2, 5, 9};
	uint8 lna2_gainbits_2g_aci[6] = {1, 2, 3, 4, 5, 6};

	uint8 lna1_gainlim_5g_aci[6] = {0xb, 0xc, 0xe, 0x20, 0x7f, 0x7f};

	uint8 lna2_gainlim_5g_aci[6] = {0x0, 0x0, 0x3, 0x3, 0x3, 0x7f};

	bool suspend;
	uint8 stall_val;

	phy_ac_noise_info_t *noisei = pi->u.pi_acphy->noisei;
	ASSERT(noisei != NULL);
	ASSERT(noisei->hwaci_args != NULL);

	pi_ac_dgain = noisei->def_gains;

	ASSERT(pi_ac_dgain);

	suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);
	if (!suspend) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
	}
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	if (stall_val == 0)
		ACPHY_DISABLE_STALL(pi);

	FOREACH_CORE(pi, core) {
		/* Initialize to defaults */
		lna1_gainlim_ofdm =  pi_ac_dgain[core].lna1_gainlim_ofdm_def;
		lna1_gainlim_cck =  pi_ac_dgain[core].lna1_gainlim_cck_def;
		lna2_gainlim_ofdm = pi_ac_dgain[core].lna2_gainlim_ofdm_def;
		lna2_gaindb = pi_ac_dgain[core].lna2_gaindb_def;
		lna2_gainbits = pi_ac_dgain[core].lna2_gainbits_def;
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			lna1_gainlim_ofdm =   aci_status ? lna1_gainlim_ofdm_2g_aci :
				pi_ac_dgain[core].lna1_gainlim_ofdm_def;
			lna1_gainlim_cck =    aci_status ? lna1_gainlim_cck_2g_aci :
				pi_ac_dgain[core].lna1_gainlim_cck_def;
			lna2_gaindb =         aci_status ? lna2_gaindb_2g_aci:
				pi_ac_dgain[core].lna2_gaindb_def;
			lna2_gainbits =       aci_status ? lna2_gainbits_2g_aci :
				pi_ac_dgain[core].lna2_gainbits_def;
			lna1_gainlim_ofdm_updated = 1;
			lna1_gainlim_cck_updated = 1;
			lna2_gaindb_updated = 1;
			lna2_gainbits_updated = 1;
		} else {
			lna1_gainlim_ofdm =	  aci_status ? lna1_gainlim_5g_aci :
				pi_ac_dgain[core].lna1_gainlim_ofdm_def;
			lna2_gainlim_ofdm =	  aci_status ? lna2_gainlim_5g_aci :
				pi_ac_dgain[core].lna2_gainlim_ofdm_def;
			lna1_gainlim_ofdm_updated = 1;
			lna2_gainlim_ofdm_updated = 1;
		}

		if (IS_4364_3x3(pi)) {
			/* Dont limit lna2 for 3x3 slice */
			lna2_gainlim_ofdm_updated = 0;

			/* core2 on 3x3 slice has 4db less gain till lna1,
			   so change limit index to 4 instead of 3
			 */
			if ((pi->sh->boardflags4 & BFL4_4364_HARPOON) &&
				CHSPEC_IS5G(pi->radio_chanspec) &&
				(core == 2)) {
				uint8 lna1_gainlim_5g_aci_core2[6] =
				{0xb, 0xc, 0xe, 0x20, 0x24, 0x7f};
				lna1_gainlim_ofdm = aci_status ? lna1_gainlim_5g_aci_core2 :
					pi_ac_dgain[core].lna1_gainlim_ofdm_def;
			}

			if ((pi->sh->boardflags4 & BFL4_4364_GODZILLA) &&
				CHSPEC_IS80(pi->radio_chanspec)) {
				uint8 lna1_gainlim_5g_aci_4364[6] =
				{0xb, 0xc, 0xe, 0x7f, 0x7f, 0x7f};
				lna1_gainlim_ofdm =	  aci_status ? lna1_gainlim_5g_aci_4364 :
					pi_ac_dgain[core].lna1_gainlim_ofdm_def;
			}

		}

		if (!ACMAJORREV_5(pi->pubpi->phy_rev)) {
			/* LNA1 settings */
			if (lna1_gainlim_ofdm_updated) {
				table_id_core[core] = ACPHY_TBL_ID_GAINLIMIT;
				start_off_core[core] = (core == 0) ? 8 : 0xff;
				wlc_phy_hwaci_write_table_acphy(pi, table_id_core[core],
					start_off_core[core], lna1_gainlim_ofdm, 0);
			}
			if (lna1_gainlim_cck_updated) {
				table_id_core[core] = ACPHY_TBL_ID_GAINLIMIT;
				start_off_core[core] = (core == 0) ? 72 : 0xff;
				wlc_phy_hwaci_write_table_acphy(pi, table_id_core[core],
					start_off_core[core], lna1_gainlim_cck, 1);
			}
			/* LNA2 settings */
			if (lna2_gainlim_ofdm_updated) {
				table_id_core[core] = ACPHY_TBL_ID_GAINLIMIT;
				start_off_core[core] = (core == 0) ? 17 : 0xff;
				wlc_phy_hwaci_write_table_acphy(pi, table_id_core[core],
					start_off_core[core], lna2_gainlim_ofdm, 0);
			}
			if (lna2_gaindb_updated) {
				table_id_core[core] = (core == 0) ? ACPHY_TBL_ID_GAIN0 :
					ACPHY_TBL_ID_GAIN1;
				start_off_core[core] = 17;
				wlc_phy_hwaci_write_table_acphy(pi, table_id_core[core],
					start_off_core[core], lna2_gaindb, 0);
			}
			if (lna2_gainbits_updated) {
				table_id_core[core] = (core == 0) ? ACPHY_TBL_ID_GAINBITS0 :
					ACPHY_TBL_ID_GAINBITS1;
				start_off_core[core] = 17;
				wlc_phy_hwaci_write_table_acphy(pi, table_id_core[core],
					start_off_core[core], lna2_gainbits, 0);
			}
		} else {	/* 43602 */
			/* LNA1 settings */
			if (lna1_gainlim_ofdm_updated) {
				wlc_phy_hwaci_write_table_acphy(pi, (core == 0) ?
					ACPHY_TBL_ID_GAINLIMIT0:
					(core == 1) ? ACPHY_TBL_ID_GAINLIMIT1:
					ACPHY_TBL_ID_GAINLIMIT2,
					8, lna1_gainlim_ofdm, 0);
			}
			if (lna1_gainlim_cck_updated) {
				wlc_phy_hwaci_write_table_acphy(pi, (core == 0) ?
					ACPHY_TBL_ID_GAINLIMIT0:
					(core == 1) ? ACPHY_TBL_ID_GAINLIMIT1:
					ACPHY_TBL_ID_GAINLIMIT2,
					72, lna1_gainlim_cck, 1);
			}
			/* LNA2 settings */
			if (lna2_gainlim_ofdm_updated) {
				wlc_phy_hwaci_write_table_acphy(pi, (core == 0) ?
					ACPHY_TBL_ID_GAINLIMIT0:
					(core == 1) ? ACPHY_TBL_ID_GAINLIMIT1:
					ACPHY_TBL_ID_GAINLIMIT2,
					17, lna2_gainlim_ofdm, 0);
			}
			if (lna2_gaindb_updated) {
				wlc_phy_hwaci_write_table_acphy(pi, (core == 0) ?
					ACPHY_TBL_ID_GAIN0:
					(core == 1) ? ACPHY_TBL_ID_GAIN1:
					ACPHY_TBL_ID_GAIN2,
					17, lna2_gaindb, 0);
			}
			if (lna2_gainbits_updated) {
				wlc_phy_hwaci_write_table_acphy(pi, (core == 0) ?
					ACPHY_TBL_ID_GAINBITS0:
					(core == 1) ? ACPHY_TBL_ID_GAINBITS1:
					ACPHY_TBL_ID_GAINBITS2,
					17, lna2_gainbits, 0);
			}
			/* tighten FSTR to prevent bphy from passing FSTR */
			if (CHSPEC_IS2G(pi->radio_chanspec) && IS43602WLCSP) {
				MOD_PHYREG(pi, FSTRMetricTh, lowPwr_min_metric_th,
					aci_status ? 0x18 : 0x20);
			}
		}
	}

	wlc_phy_switch_preemption_settings(pi, aci_status);

	/* Update INIT/CLIP-HI etc with gain limits programmed */
	if (IS_4364_3x3(pi) && CHSPEC_IS2G(pi->radio_chanspec)) {
		if (aci_status == 0) {
			noisei->aci->desense.lna1_tbl_desense = 0;
		} else {
			noisei->aci->desense.lna1_tbl_desense = 1;
		}
		noisei->aci->desense.analog_gain_desense_ofdm = 0;
		noisei->aci->desense.analog_gain_desense_bphy = 0;
		noisei->aci->desense.lna2_tbl_desense = 0;
		noisei->aci->desense.clipgain_desense[0] = 0;
		noisei->aci->desense.clipgain_desense[1] = 0;
		noisei->aci->desense.clipgain_desense[2] = 0;
		noisei->aci->desense.clipgain_desense[3] = 0;
		/* Get total desense based on aci & bt & lte */
		wlc_phy_desense_calc_total_acphy(pi->u.pi_acphy->rxgcrsi);
		/* Update LNA1 gain table with limit values */
		wlc_phy_upd_lna1_lna2_gaintbls_acphy(pi, 1);
		/* Call gain control function to recompute INIT/CLIP-HI etc.
			based on gain limit table
		*/
		wlc_phy_rxgainctrl_gainctrl_acphy(pi);
	}

	ACPHY_ENABLE_STALL(pi, stall_val);
	if (!suspend) {
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
	}
}

static void
wlc_phy_hwaci_write_table_acphy(phy_info_t *pi, uint16 table_id,
uint16 start_off, uint8 *table_entry, bool check_5G)
{
	uint8 def_values[6];
	if ((start_off == 0xff) || (CHSPEC_IS5G(pi->radio_chanspec) && check_5G))
		return;
	wlc_phy_table_read_acphy(pi, table_id,
	6, start_off, 8, def_values);
	if (!memcmp(def_values, table_entry, sizeof(uint8)*6)) {
		start_off = 0xff;
	}
	if (start_off != 0xff) {
		wlc_phy_table_write_acphy(pi, table_id,
			6, start_off, 8, table_entry);
	}

}

void
wlc_phy_hwaci_setup_acphy(phy_info_t *pi, bool on, bool init)
{
	uint8 core, on_off;
	uint8 wrssi3_ib_Refbuf;
	uint8 nbrssi_Refctrl_low;
	uint8 wrssi3_Refctrl_low, wrssi3_Refctrl_mid, wrssi3_Refctrl_high;
	uint8 wrssi3_ib_powersaving, wrssi3_ib_Refladder;
	uint16 regval;

	/* For ACI_Detect_CTRL */
	uint8 aci_detect_window_size;
	uint8 aci_rpt_det_ctr_clren;
	uint8 aci_detect_enable;
	uint8 aci_detect_clkenable;
	uint8 aci_present_th;

	/* For ACPHY_ACI_Detect_collect_interval */
	uint8 aci_detect_collect_interval;

	/* ACI_Detect_wait_period 0x552 0x553 */
	uint16 aci_detect_wait_period;

	/* Energy and detect threshold 0x554 0x555 0x556 0x557 */
	uint16 aci_detect_energy_threshold;
	uint16 aci_detect_diff_threshold;
	uint16 aci_detect_energy_threshold_w2;
	uint16 aci_detect_diff_threshold_w2;

	/* ACI_Detect_MAX_COUNT 0x558 0x559 */
	uint32 max_count;
	uint16 sample_time_period_ms;
	acphy_hwaci_setup_t hwaci;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	on_off = on ? 1 : 0;
	if ((ACMAJORREV_1(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) || ACHWACIREV(pi)) {
		on_off = on ? 7 : 0;
	}
	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, RfctrlCoreTxPus, core, lpf_wrssi3_pwrup, on_off);
		MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, lpf_wrssi3_pwrup, 0x1);
	}
	if (ACHWACIREV(pi)) {
		FOREACH_CORE(pi, core) {
			MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_lna2_wrssi2_pwrup, 1);
			MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna2_wrssi2_pwrup, 0x1);
		}
	}

	if (!init) return;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
	ASSERT(pi_ac->noisei->hwaci_args != NULL);

	hwaci = *pi_ac->noisei->hwaci_args;

	/* These are the thresholds of comparators */
	wrssi3_ib_Refladder = 0x0;
	wrssi3_ib_Refbuf = 0x0;
	wrssi3_ib_powersaving = 0x0;
	nbrssi_Refctrl_low = hwaci.nb_lo_th;
	wrssi3_Refctrl_low = hwaci.w3_lo_th;
	wrssi3_Refctrl_mid = hwaci.w3_md_th;
	wrssi3_Refctrl_high = hwaci.w3_hi_th;

	/* For ACI_Detect_CTRL */
	aci_detect_window_size = hwaci.sliding_window;
	aci_rpt_det_ctr_clren = 1;
	aci_detect_enable = 1;
	aci_detect_clkenable = 1;
	aci_present_th = hwaci.aci_present_th;

	/* For ACPHY_ACI_Detect_collect_interval */
	aci_detect_collect_interval = hwaci.samp_cluster;

	/* ACI_Detect_wait_period 0x552 0x553 */
	aci_detect_wait_period = hwaci.wait_period;

	/* Energy and detect threshold 0x554 0x555 0x556 0x557 */
	aci_detect_energy_threshold = hwaci.energy_thresh;
	aci_detect_diff_threshold = hwaci.detect_thresh;
	aci_detect_energy_threshold_w2 = hwaci.energy_thresh_w2;
	aci_detect_diff_threshold_w2 = hwaci.detect_thresh;

	/* hwaci energy thresholds */
	if (IS_4364_3x3(pi)) {
			if (pi->sh->boardflags4 & BFL4_4364_GODZILLA) {
				/* GODZILLA MODULES */
				aci_detect_energy_threshold = HWACI_DETECT_ENERGY_TH_GODZILLA;
				aci_detect_energy_threshold_w2 = HWACI_DETECT_ENERGY_TH_GODZILLA;
			} else {
				/* REF BOARDS & HARPOON MODULES */
				aci_detect_energy_threshold = HWACI_DETECT_ENERGY_TH_HARPOON;
				aci_detect_energy_threshold_w2 = HWACI_DETECT_ENERGY_TH_HARPOON;
			}
	}

	/* ACI_Detect_MAX_COUNT 0x558 0x559 */
	/* For 1 sec, it is = 1000 */
	sample_time_period_ms = hwaci.sample_time;

	FOREACH_CORE(pi, core) {
		/*  Configure nb_low can't touch mid, hi as auto-gainctrl is going to pick those */
		MOD_RADIO_REGC(pi, NBRSSI_CONFG, core, nbrssi_Refctrl_low, nbrssi_Refctrl_low);

		/* Configure w3 */
		MOD_RADIO_REGC(pi, WRSSI3_CONFG, core, wrssi3_ib_Refladder, wrssi3_ib_Refladder);
		MOD_RADIO_REGC(pi, WRSSI3_CONFG, core, wrssi3_ib_Refbuf, wrssi3_ib_Refbuf);
		MOD_RADIO_REGC(pi, WRSSI3_CONFG, core,
		               wrssi3_ib_powersaving, wrssi3_ib_powersaving);
		MOD_RADIO_REGC(pi, WRSSI3_CONFG, core, wrssi3_Refctrl_low, wrssi3_Refctrl_low);
		MOD_RADIO_REGC(pi, WRSSI3_CONFG, core, wrssi3_Refctrl_mid, wrssi3_Refctrl_mid);
		MOD_RADIO_REGC(pi, WRSSI3_CONFG, core, wrssi3_Refctrl_high, wrssi3_Refctrl_high);

		if (ACHWACIREV(pi)) {
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
			  MOD_RADIO_REGC(pi, LNA2G_RSSI, core, dig_wrssi2_threshold, hwaci.w2);
			} else {
			  MOD_RADIO_REGC(pi, LNA5G_RSSI, core, dig_wrssi2_threshold, hwaci.w2);
			}
		}
	}

	/* ACPHY_ACI_Detect_CTRL */
	/* Reset ACI detection block */
	MOD_PHYREG(pi, ACI_Detect_CTRL, aci_detect_enable, 0);
	regval = READ_PHYREG(pi, ACI_Detect_CTRL);
	regval = (regval & ~ACPHY_ACI_Detect_CTRL_aci_detect_enable_MASK(pi->pubpi->phy_rev)) |
	        (aci_detect_enable <<
		 ACPHY_ACI_Detect_CTRL_aci_detect_enable_SHIFT(pi->pubpi->phy_rev));
	regval = (regval & ~ACPHY_ACI_Detect_CTRL_aci_report_ctr_clren_MASK(pi->pubpi->phy_rev)) |
	        (aci_rpt_det_ctr_clren <<
		 ACPHY_ACI_Detect_CTRL_aci_report_ctr_clren_SHIFT(pi->pubpi->phy_rev));
	regval = (regval & ~ACPHY_ACI_Detect_CTRL_aci_detected_ctr_clren_MASK(pi->pubpi->phy_rev)) |
	        (aci_rpt_det_ctr_clren <<
		 ACPHY_ACI_Detect_CTRL_aci_detected_ctr_clren_SHIFT(pi->pubpi->phy_rev));
	regval = (regval &
		~ACPHY_ACI_Detect_CTRL_aci_detect_window_size_1_MASK(pi->pubpi->phy_rev)) |
	        (aci_detect_window_size <<
		 ACPHY_ACI_Detect_CTRL_aci_detect_window_size_1_SHIFT(pi->pubpi->phy_rev));
	regval = (regval &
		~ACPHY_ACI_Detect_CTRL_aci_detect_window_size_2_MASK(pi->pubpi->phy_rev)) |
	        (aci_detect_window_size <<
		 ACPHY_ACI_Detect_CTRL_aci_detect_window_size_2_SHIFT(pi->pubpi->phy_rev));
	if (ACMAJORREV_1(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) {
		regval = (regval &
			~ACPHY_ACI_Detect_CTRL_aci_detect_clkenable_MASK(pi->pubpi->phy_rev)) |
		        (aci_detect_clkenable <<
			 ACPHY_ACI_Detect_CTRL_aci_detect_clkenable_SHIFT(pi->pubpi->phy_rev));
	}
	WRITE_PHYREG(pi, ACI_Detect_CTRL, regval);

	/* ACPHY_ACI_Detect_collect_interval */
	MOD_PHYREG(pi, ACI_Detect_collect_interval, aci_detect_collect_interval_1,
	           aci_detect_collect_interval);
	MOD_PHYREG(pi, ACI_Detect_collect_interval, aci_detect_collect_interval_2,
	           aci_detect_collect_interval);

	/* ACI_Detect_wait_period */
	WRITE_PHYREG(pi, ACI_Detect_wait_period_1, aci_detect_wait_period);
	WRITE_PHYREG(pi, ACI_Detect_wait_period_2, aci_detect_wait_period);

	/* Energy threshold */
	WRITE_PHYREG(pi, ACI_Detect_energy_threshold_1, aci_detect_energy_threshold);
	WRITE_PHYREG(pi, ACI_Detect_energy_threshold_2, aci_detect_energy_threshold);

	/* Detect threshold */
	WRITE_PHYREG(pi, ACI_Detect_detect_threshold_1, aci_detect_diff_threshold);
	WRITE_PHYREG(pi, ACI_Detect_detect_threshold_2, aci_detect_diff_threshold);

	if (AC4354REV(pi) || ACHWACIREV(pi)) {
		/* Energy threshold */
		WRITE_PHYREG(pi, ACI_Detect_energy_threshold_1_w12, aci_detect_energy_threshold_w2);

		WRITE_PHYREG(pi, ACI_Detect_energy_threshold_2_w12, aci_detect_energy_threshold_w2);


		/* Detect threshold */
		WRITE_PHYREG(pi, ACI_Detect_detect_threshold_1_w12, aci_detect_diff_threshold_w2);

		WRITE_PHYREG(pi, ACI_Detect_detect_threshold_2_w12, aci_detect_diff_threshold_w2);
	}

	/* max count = time/0.8 us */
	max_count = sample_time_period_ms * 1000 * 10/8;
	WRITE_PHYREG(pi, ACI_Detect_MAX_COUNT_LO, (uint16) (max_count & 0xffff));
	WRITE_PHYREG(pi, ACI_Detect_MAX_COUNT_HI, (uint16) (max_count >> 16));

	/* SlnaControl bt prisel to be inv for 2G HWACI to work 4335C0 */
	if ((ACMAJORREV_1(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) ||
	    ACMAJORREV_3(pi->pubpi->phy_rev) ||
		(IS_4364_3x3(pi))) {
		MOD_PHYREG(pi, SlnaControl, inv_btcx_prisel_polarity, 1);
	}

	if (AC4354REV(pi) || ACHWACIREV(pi)) {
		if (!ACMAJORREV_5(pi->pubpi->phy_rev)) {
			MOD_PHYREG(pi, ACI_Mitigation_CTRL1, aci_present_th_mit_on_w12,
					aci_present_th);
			MOD_PHYREG(pi, ACI_Mitigation_CTRL1, aci_present_th_mit_off_w12,
					aci_present_th);
			MOD_PHYREG(pi, ACI_Mitigation_CTRL, aci_present_th_mit_on_w3,
					aci_present_th);
			MOD_PHYREG(pi, ACI_Mitigation_CTRL, aci_present_th_mit_off_w3,
					aci_present_th);
			MOD_PHYREG(pi, ACI_Detect_CTRL1, aci_present_select,
					hwaci.aci_present_select);
		} else {
			MOD_PHYREG(pi, ACI_Mitigation_CTRL2, aci_present_th_mit_on_w12,
					aci_present_th);
			MOD_PHYREG(pi, ACI_Mitigation_CTRL2, aci_present_th_mit_off_w12,
					aci_present_th);
			MOD_PHYREG(pi, ACI_Mitigation_CTRL, aci_present_th_mit_on_w3,
					aci_present_th);
			MOD_PHYREG(pi, ACI_Mitigation_CTRL, aci_present_th_mit_off_w3,
					aci_present_th);
			MOD_PHYREG(pi, ACI_Detect_CTRL1, aci_present_select,
					hwaci.aci_present_select);
		}

		if (((pi->sh->interference_mode & ACPHY_HWACI_MITIGATION) == 0)) {
		  /* HW ACI Detection and SW Mitigation */
		  ACPHY_REG_LIST_START
		    MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_hwtrig_disable, 1)
		    MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_hw_enable, 1)
		    MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_sw_enable, 1)
		    MOD_PHYREG_ENTRY(pi, ACI_Detect_report_ctr_threshold_lo, aci_report_ctr_th_lo,
		      30)
		  ACPHY_REG_LIST_EXECUTE(pi)
		} else {
		  /* HW ACI Detection and HW Mitigation */
		  ACPHY_REG_LIST_START
		    MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_hwtrig_disable, 1)
		    MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, ACIMitigationIndicatorBit, 0)
		    MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, ACIMitigationONShadowBit, 0)
		    MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, Disable_ChannelIndicator, 1)
		    MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_hw_enable, 2)
		    MOD_PHYREG_ENTRY(pi, ACI_Mitigation_timeout_LO, aci_mitigation_timeout_lo, 0)
		    MOD_PHYREG_ENTRY(pi, ACI_Mitigation_timeout_HI, aci_mitigation_timeout_hi, 0)
		    MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_sw_enable, 0)
		    MOD_PHYREG_ENTRY(pi, ACI_Detect_report_ctr_threshold_lo, aci_report_ctr_th_lo,
		      30)
		  ACPHY_REG_LIST_EXECUTE(pi);
		}
	}
}
#endif /* WLC_DISABLE_ACI */

void
wlc_phy_aci_w2nb_setup_acphy(phy_info_t *pi, bool on)
{
	uint8 core, on_off;

	on_off = on ? 1 : 0;
	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_lna2_wrssi2_pwrup, on_off);
		MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna2_wrssi2_pwrup, 0x1);

#ifndef WLC_DISABLE_ACI
		if (TINY_RADIO(pi) || ACMAJORREV_36(pi->pubpi->phy_rev))
			PHY_INFORM(("%s: HWACI not yet implemented for Tiny Radio chips\n",
				__FUNCTION__));
		else if (on) {
			MOD_RADIO_REGC(pi, LNA5G_RSSI, core, dig_wrssi2_threshold,
			               pi->u.pi_acphy->noisei->hwaci_args->w2);
		}
#endif /* !WLC_DISABLE_ACI */
	}
}

typedef struct _hwaci_fcbs_phytbl_list_entry hwaci_fcbs_phytbl_list_entry;

#ifdef WL_ACI_FCBS
static uint16 hwaci_fcbs_rfregs[5];
static uint16 hwaci_fcbs_phyregs[14];
#define FCBS_1DATA_PER_ADDR  0x2000
static  hwaci_fcbs_phytbl_list_entry  hwaci_fcbs_phytbls [ ] = {
	{ ACPHY_TBL_ID_RFSEQ,       0x0f6, 1 },
	{ ACPHY_TBL_ID_RFSEQ,		 0x0f9, 1 },
	{ ACPHY_TBL_ID_GAINLIMIT,	 0x008, 7 },
	{ ACPHY_TBL_ID_GAINLIMIT,	 0x048, 7 },
	{ ACPHY_TBL_ID_GAIN0,		 0x008, 8 },
	{ ACPHY_TBL_ID_GAINBITS0,	 0x008, 8 },
	{ 0xFFFF,	 0x000, 0 }
};

static void
wlc_phy_hwaci_fcbsinit_acphy(phy_info_t *pi)
{
	hwaci_fcbs_rfregs[0] = RF0_20691_LNA2G_RSSI1(pi->pubpi->radiorev);
	hwaci_fcbs_rfregs[1] = RF0_20691_LNA5G_RSSI1(pi->pubpi->radiorev);
	hwaci_fcbs_rfregs[2] = RF0_20691_TIA_CFG13(pi->pubpi->radiorev);
	hwaci_fcbs_rfregs[3] = RF0_20691_TIA_CFG12(pi->pubpi->radiorev);
	hwaci_fcbs_rfregs[4] = 0xFFFF;

	hwaci_fcbs_phyregs[0] = ACPHY_Core0clip2GainCodeA(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[1] = ACPHY_Core0clipHiGainCodeA(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[2] = ACPHY_Core0cliploGainCodeA(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[3] = ACPHY_Core0clipmdGainCodeA(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[4] = ACPHY_Core0InitGainCodeA(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[5] = ACPHY_Core0clip2GainCodeB(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[6] = ACPHY_Core0clipHiGainCodeB(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[7] = ACPHY_Core0cliploGainCodeB(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[8] = ACPHY_Core0clipmdGainCodeB(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[9] = ACPHY_Core0InitGainCodeB(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[10] = ACPHY_Core0DSSScckPktGain(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[11] = ACPHY_Core0HpFBw(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[12] = ACPHY_Core0RssiClipMuxSel(pi->pubpi->phy_rev);
	hwaci_fcbs_phyregs[13] = 0xFFFF;
}
#endif /* WL_ACI_FCBS */

uint8
wlc_phy_disable_hwaci_fcbs_trig(phy_info_t *pi)
{
#ifdef WL_ACI_FCBS
	uint8 curr_val = READ_PHYREGFLD(pi, ACI_Mitigation_CTRL, aci_mitigation_hwtrig_disable);

	MOD_PHYREG(pi, ACI_Mitigation_CTRL, aci_mitigation_hwtrig_disable, 1);

	while ((READ_PHYREG(pi, FastChanSW_Status) & 0xe) != 0x0) {
		OSL_DELAY(5);   /* Expected time for FCBS(for HWACI) complete */
	}

	return curr_val;
#else
	return 0;
#endif /* WL_ACI_FCBS */
}

void
wlc_phy_restore_hwaci_fcbs_trig(phy_info_t *pi, uint8 trig_disable)
{
#ifdef WL_ACI_FCBS
	MOD_PHYREG(pi, ACI_Mitigation_CTRL, aci_mitigation_hwtrig_disable, trig_disable);
#else
	return;
#endif
}

/* From proc acphy_hwaci_mitigation_enable {{ enable 0 }} { */
void
wlc_phy_hwaci_mitigation_enable_acphy(phy_info_t *pi, uint8 hwaci_mode, bool init)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint16 m_hwaci_st_offset =
		((wlapi_bmac_read_shm(pi->sh->physhim, M_PSM2HOST_EXT_PTR(pi)) + 0x20) << 1);
	uint8 band = CHSPEC_IS2G(pi->radio_chanspec) ? 2 : 5;
	acphy_hwaci_state_t  *hwaci_states = (band == 2) ? pi_ac->noisei->hwaci_states_2g :
	        pi_ac->noisei->hwaci_states_5g;

	ASSERT(ACPHY_ENABLE_FCBS_HWACI(pi) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev) || ACMAJORREV_37(pi->pubpi->phy_rev));
	/* Reset to the High Sense mode */
	wlapi_bmac_write_shm(pi->sh->physhim, m_hwaci_st_offset, 0);

	if (ACMAJORREV_4(pi->pubpi->phy_rev) && CHSPEC_IS5G(pi->radio_chanspec)) {
		/* Disable HW ACI in 5G for 4349 */
		hwaci_mode = HWACI_DISABLE;
	}

	switch (hwaci_mode) {
	case HWACI_DISABLE:
		PHY_ACI(("Tiny HWACI: Disable HW-ACI-Mitigation\n"));
		wlc_phy_disable_hwaci_fcbs_trig(pi);
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
		    ACMAJORREV_37(pi->pubpi->phy_rev)) {
			ACPHY_REG_LIST_START
				MOD_PHYREG_ENTRY(pi, ACI_Detect_collect_interval,
					aci_detect_clkenable, 0)
			ACPHY_REG_LIST_EXECUTE(pi);
		} else {
			ACPHY_REG_LIST_START
				MOD_PHYREG_ENTRY(pi, ACI_Detect_CTRL, aci_detect_clkenable, 0)
			ACPHY_REG_LIST_EXECUTE(pi);
		}
		ACPHY_REG_LIST_START
			MOD_PHYREG_ENTRY(pi, ACI_Detect_CTRL, aci_detect_enable, 0)
			MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_hw_enable, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
		break;
	case HWACI_AUTO_FCBS:
	case HWACI_FORCED_MITON:
	case HWACI_FORCED_MITOFF:
	case HWACI_AUTO_SW:

		PHY_ACI(("Tiny HWACI: Enable clocks, preempt \n"));
		if (init) {
			/* Disable HWACI while updating FCBS */
			wlc_phy_disable_hwaci_fcbs_trig(pi);
			if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
			    ACMAJORREV_33(pi->pubpi->phy_rev) ||
			    ACMAJORREV_37(pi->pubpi->phy_rev)) {
				if ((pi->sh->interference_mode & ACPHY_ACI_PREEMPTION) != 0)
					wlc_phy_preempt(pi, TRUE, FALSE);
			} else {
				wlc_phy_preempt(pi, TRUE, FALSE);
			}

#ifdef WL_ACI_FCBS
			wlc_phy_hwaci_fcbsinit_acphy(pi);
			wlc_phy_init_FCBS_hwaci(pi);
#endif /* WL_ACI_FCBS */
		}
		MOD_PHYREG(pi, SlnaControl, inv_btcx_prisel_polarity, 1);
		MOD_PHYREG(pi, ACI_Mitigation_CTRL, aci_mitigation_hw_enable, 2);
		break;

	default:
		break;
	}


	switch (hwaci_mode) {
	case HWACI_AUTO_FCBS:
	case HWACI_AUTO_SW:

		PHY_ACI(("Tiny HWACI: Enable clocks, preempt \n"));
		if (init) {
			PHY_ACI(("Tiny HWACI: AUTO mode\n"));
			/*
			 * From proc do_aci_setup Optimise by
			 * amalgamating writes by using WRITE_PHYREG
			 */
			if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
				ACPHY_REG_LIST_START
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_mitigation_sw_enable, 0)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_present_th_mit_off, 0)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_present_th_mit_on, 5)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else if (ACMAJORREV_32(pi->pubpi->phy_rev)) {
				ACPHY_REG_LIST_START
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_mitigation_sw_enable, 0)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_present_th_mit_off, 2)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_present_th_mit_on, 2)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else if (ACMAJORREV_33(pi->pubpi->phy_rev) ||
			           ACMAJORREV_37(pi->pubpi->phy_rev)) {
				ACPHY_REG_LIST_START
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_mitigation_sw_enable, 0)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_present_th_mit_off_1, 2)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_present_th_mit_on_1, 2)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else {
				ACPHY_REG_LIST_START
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_mitigation_sw_enable, 0)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_present_th_mit_off, 5)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_present_th_mit_on, 5)
				ACPHY_REG_LIST_EXECUTE(pi);
			}
			if (ACMAJORREV_4(pi->pubpi->phy_rev) ||
			    ACMAJORREV_32(pi->pubpi->phy_rev) ||
			    ACMAJORREV_33(pi->pubpi->phy_rev) ||
			    ACMAJORREV_37(pi->pubpi->phy_rev)) {
				WRITE_PHYREG(pi, ACI_Detect_report_ctr_threshold_lo, 0);
			} else if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
				WRITE_PHYREG(pi, ACI_Detect_report_ctr_threshold_lo, 0);
				MOD_PHYREG(pi, ACI_Detect_CTRL1, aci_detect_w1w2_select, 1);
			} else {
				MOD_PHYREG(pi, ACI_Detect_report_ctr_threshold,
						aci_report_ctr_th, 0);
			}

			if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
			    ACMAJORREV_33(pi->pubpi->phy_rev) ||
			    ACMAJORREV_37(pi->pubpi->phy_rev)) {
				ACPHY_REG_LIST_START
					MOD_PHYREG_ENTRY(pi, ACI_Detect_collect_interval,
						aci_detect_collect_interval_1, 15)
					MOD_PHYREG_ENTRY(pi, ACI_Detect_collect_interval,
						aci_detect_collect_interval_2, 15)
					MOD_PHYREG_ENTRY(pi, ACI_Detect_CTRL,
						aci_detect_window_size_1, 15)
					MOD_PHYREG_ENTRY(pi, ACI_Detect_CTRL,
						aci_detect_window_size_2, 15)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else {
				ACPHY_REG_LIST_START
					MOD_PHYREG_ENTRY(pi, ACI_Detect_collect_interval,
						aci_detect_collect_interval_1, 2)
					MOD_PHYREG_ENTRY(pi, ACI_Detect_collect_interval,
						aci_detect_collect_interval_2, 2)
					MOD_PHYREG_ENTRY(pi, ACI_Detect_wait_period_1,
						aci_detect_wait_period_1, 1)
					MOD_PHYREG_ENTRY(pi, ACI_Detect_wait_period_2,
						aci_detect_wait_period_2, 0xff)
					MOD_PHYREG_ENTRY(pi, ACI_Detect_CTRL,
						aci_detect_window_size_1, 8)
					MOD_PHYREG_ENTRY(pi, ACI_Detect_CTRL,
						aci_detect_window_size_2, 8)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_status,
							aci_detect_direct_output, 1)
				ACPHY_REG_LIST_EXECUTE(pi);
			}

			if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
				MOD_PHYREG(pi, ACI_Mitigation_status, aci_T_RSSI_select, 2);
			} else if (ACMAJORREV_32(pi->pubpi->phy_rev)) {
				MOD_PHYREG(pi, ACI_Mitigation_status, aci_T_RSSI_select, 2);
				MOD_PHYREG(pi, ACI_Detect_CTRL1, aci_detect_w1w2_select, 1);
				MOD_PHYREG(pi, ACI_Mitigation_status, aci_detect_direct_output, 1);
			} else if (ACMAJORREV_33(pi->pubpi->phy_rev) ||
			           ACMAJORREV_37(pi->pubpi->phy_rev)) {
				MOD_PHYREG(pi, ACI_Mitigation_InputSelect, aci_A_select_1, 0);
				MOD_PHYREG(pi, ACI_Mitigation_InputSelect, aci_B_select_1, 2);
				MOD_PHYREG(pi, ACI_Mitigation_status, aci_detect_direct_output, 1);
			} else {
				MOD_PHYREG(pi, ACI_Mitigation_status, aci_T_RSSI_select, 0);
			}
			if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
				ACPHY_REG_LIST_START
					/* 4349 eLNA boards HWACI settings */
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_status,
						aci_pwr_input_shift, 6)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_status,
						aci_pwr_block_shift, 4)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config1, 1300)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config2, 0x0)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config3, 200)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config4, 0x004B)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config5, 0x0)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config6, 0x0)
					MOD_PHYREG_ENTRY(pi, ACI_Detect_MAX_COUNT_HI,
						aci_detect_max_count_hi, 6)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_present_th_mit_off, 0)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
						aci_present_th_mit_on, 5)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
				/* 43012 HWACI settings */
				ACPHY_REG_LIST_START
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_status,
						aci_pwr_input_shift, 6)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_status,
						aci_pwr_block_shift, 4)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config1, 0x50)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config2, 0xffff)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config3, 0x50)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config4, 0x50)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config5, 0xffff)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config6, 0x50)
					MOD_PHYREG_ENTRY(pi, ACI_Detect_MAX_COUNT_HI,
						aci_detect_max_count_hi, 6)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else if (ACMAJORREV_32(pi->pubpi->phy_rev)) {
				MOD_PHYREG(pi, ACI_Mitigation_status, aci_pwr_input_shift, 6);
				MOD_PHYREG(pi, ACI_Mitigation_status, aci_pwr_block_shift, 4);
				WRITE_PHYREG(pi, Tiny_ACI_config1, 0x100);
				WRITE_PHYREG(pi, Tiny_ACI_config2, 0x8000);
				WRITE_PHYREG(pi, Tiny_ACI_config3, 0x300);
				WRITE_PHYREG(pi, Tiny_ACI_config4, 0x100);
			} else if (ACMAJORREV_33(pi->pubpi->phy_rev) ||
			           ACMAJORREV_37(pi->pubpi->phy_rev)) {
				MOD_PHYREG(pi, ACI_Mitigation_status, aci_pwr_input_shift, 6);
				MOD_PHYREG(pi, ACI_Mitigation_status, aci_pwr_block_shift, 4);
				ACPHYREG_BCAST(pi, Tiny_ACI_config10, 0x100);
				ACPHYREG_BCAST(pi, Tiny_ACI_config20, 0x8000);
				ACPHYREG_BCAST(pi, Tiny_ACI_config30, 0x300);
				ACPHYREG_BCAST(pi, Tiny_ACI_config40, 0x100);
			} else {
				/* these are for eLNA boards only */
				if (IS_4364_1x1(pi)) {
				/* ACI detection time is modified to 100ms for 4364.
				 0x1e848 * 0.8us = 100ms.
				*/
					WRITE_PHYREG(pi, ACI_Detect_MAX_COUNT_LO, 0xe848);
					WRITE_PHYREG(pi, ACI_Detect_MAX_COUNT_HI, 1);
				}
				ACPHY_REG_LIST_START
					/* these are for eLNA boards only */
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_status,
						aci_pwr_input_shift, 6)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_status,
						aci_pwr_block_shift, 4)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config1,
						((CHSPEC_IS2G(pi->radio_chanspec) &&
						(IS_4364_1x1(pi))) ? 0x514 : 0x3e8))
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config2, 0x0)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config3, 0x00c8)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config4, 0x0064)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config5, 0x0)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config6, 0x0)
				ACPHY_REG_LIST_EXECUTE(pi);
			}
			if (ACPHY_LO_NF_MODE_ELNA_TINY(pi) &&
			    !(ACMAJORREV_32(pi->pubpi->phy_rev) ||
			      ACMAJORREV_33(pi->pubpi->phy_rev) ||
			      ACMAJORREV_37(pi->pubpi->phy_rev))) {
				ACPHY_REG_LIST_START
					MOD_PHYREG_ENTRY(pi, ACI_Detect_collect_interval,
						aci_detect_collect_interval_1, 1)
					MOD_PHYREG_ENTRY(pi, ACI_Detect_wait_period_1,
						aci_detect_wait_period_1, 0)
					MOD_PHYREG_ENTRY(pi, ACI_Mitigation_status,
						aci_pwr_input_shift, 7)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config1, 1000)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config3, 100)
				ACPHY_REG_LIST_EXECUTE(pi);
			}

			if (ACMAJORREV_32(pi->pubpi->phy_rev)) {
				WRITE_PHYREG(pi, Tiny_ACI_config5, 0x8000);
				WRITE_PHYREG(pi, Tiny_ACI_config6, 0x300);
			} else if (ACMAJORREV_33(pi->pubpi->phy_rev) ||
			           ACMAJORREV_37(pi->pubpi->phy_rev)) {
				ACPHYREG_BCAST(pi, Tiny_ACI_config50, 0x8000);
				ACPHYREG_BCAST(pi, Tiny_ACI_config60, 0x300);
			} else {
				ACPHY_REG_LIST_START
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config7, 0x0)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config8, 0x0)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config9, 0x0)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config10, 0x0)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config11, 0x0451)
					WRITE_PHYREG_ENTRY(pi, Tiny_ACI_config12, 0x0534)
				ACPHY_REG_LIST_EXECUTE(pi);
			}
		}
		if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
		    ACMAJORREV_33(pi->pubpi->phy_rev) ||
		    ACMAJORREV_37(pi->pubpi->phy_rev)) {
			ACPHY_REG_LIST_START
				MOD_PHYREG_ENTRY(pi, ACI_Detect_collect_interval,
					aci_detect_clkenable, 1)
			ACPHY_REG_LIST_EXECUTE(pi);
		} else {
			ACPHY_REG_LIST_START
				MOD_PHYREG_ENTRY(pi, ACI_Detect_CTRL, aci_detect_clkenable, 1)
			ACPHY_REG_LIST_EXECUTE(pi);
		}
		ACPHY_REG_LIST_START
			MOD_PHYREG_ENTRY(pi, ACI_Detect_CTRL, aci_detect_enable, 1)
			MOD_PHYREG_ENTRY(pi, aci_detector_reset, aci_soft_reset, 1)
			MOD_PHYREG_ENTRY(pi, aci_detector_reset, aci_soft_reset, 0)
#ifdef WL_ACI_FCBS
			MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_hwtrig_disable, 0)
#else
			MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_hwtrig_disable, 1)
#endif
		ACPHY_REG_LIST_EXECUTE(pi);
#ifndef WL_ACI_FCBS
		/* 4365 C0 :: have to remove this for future chips to enable hw switching */
		if (ACMAJORREV_33(pi->pubpi->phy_rev) || ACMAJORREV_37(pi->pubpi->phy_rev)) {
			ACPHY_REG_LIST_START
				MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL,
					aci_mitigation_hw_switching_disable, 1)
			ACPHY_REG_LIST_EXECUTE(pi);
		}
#endif
		break;

	case HWACI_FORCED_MITOFF:
		PHY_ACI(("Tiny HWACI: Forced NORMAL mode \n"));
		ACPHY_REG_LIST_START
			MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_sw_enable, 0)
			MOD_PHYREG_ENTRY(pi, ACI_Detect_CTRL, aci_sel, 0)
			MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_sw_enable, 1)
		ACPHY_REG_LIST_EXECUTE(pi);
		break;

	case HWACI_FORCED_MITON:
		PHY_ACI(("Tiny HWACI: Forced ACI mode \n"));
		ACPHY_REG_LIST_START
			MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_sw_enable, 0)
			MOD_PHYREG_ENTRY(pi, ACI_Detect_CTRL, aci_sel, 1)
			MOD_PHYREG_ENTRY(pi, ACI_Mitigation_CTRL, aci_mitigation_sw_enable, 1)
		ACPHY_REG_LIST_EXECUTE(pi);
		break;

	default:
		break;
	}
	if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
	    ACMAJORREV_33(pi->pubpi->phy_rev) ||
	    ACMAJORREV_37(pi->pubpi->phy_rev)) {
		hwaci_states[0].lna1_idx_min = 0;
		hwaci_states[0].lna1_idx_max = ACPHY_4365_MAX_LNA1_IDX;
		hwaci_states[0].lna2_idx_min = ACPHY_4365_MAX_LNA2_IDX;
		hwaci_states[0].lna2_idx_max = ACPHY_4365_MAX_LNA2_IDX;
		hwaci_states[0].mix_idx_min = 0;
		hwaci_states[0].mix_idx_max = 7;
		hwaci_states[1].lna1_idx_min = 0;
		hwaci_states[1].lna1_idx_max = 4;
		hwaci_states[1].lna2_idx_min = 0;
		hwaci_states[1].lna2_idx_max = 1;
		hwaci_states[1].mix_idx_min = 7;
		hwaci_states[1].mix_idx_max = 7;
	}
}


#ifdef WL_ACI_FCBS

/*  From proc init_fastcsTbl_for_aci_detector {} { */
static void
wlc_phy_init_FCBS_hwaci(phy_info_t *pi)
{

/*
 * Initialise FCBS table with ACI mitigation settings.
 * These are generated by running AGC config code
 * and then copying values to FCBS.
 *
 */

	uint16 val;
	uint16 *ptr_val;
	int i;

	uint16 fastchswTableIndex, startoffset;

	uint16 *reg_list_ptr;
	hwaci_fcbs_phytbl_list_entry  *tbl_list_ptr;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	const uint16 fcbs_table_delim = 0xffff;
	uint16 tia_cfg12_wrssi3_ref_high_sel;
	uint16 tia_cfg12_wrssi3_ref_mid_sel;
	uint16 tia_cfg13_wrssi3_ref_low_sel;
	acphy_desense_values_t curr_desense;

	ASSERT(ACPHY_ENABLE_FCBS_HWACI(pi));

	ptr_val = &val;

	fastchswTableIndex = startoffset =
		READ_PHYREGFLD(pi, ACI_Mitigation_CTRL1, ACIMitigation_iniraddr);

	curr_desense = phy_ac_rxgcrs_get_desense(pi_ac->rxgcrsi, CURR_DESENSE);
	curr_desense.analog_gain_desense_ofdm = 0;
	curr_desense.analog_gain_desense_bphy = 0;
	curr_desense.lna1_tbl_desense = 0;
	curr_desense.clipgain_desense[0] = 0;
	curr_desense.clipgain_desense[1] = 0;
	curr_desense.clipgain_desense[2] = 0;
	curr_desense.clipgain_desense[3] = 0;
	phy_ac_rxgcrs_set_desense(pi_ac->rxgcrsi, &curr_desense, CURR_DESENSE);

	wlc_phy_rxgainctrl_set_gaintbls_acphy_tiny(pi, 0, ACPHY_TBL_ID_GAIN0,
		ACPHY_TBL_ID_GAINBITS0);
	wlc_phy_rxgainctrl_gainctrl_acphy_tiny(pi, 0);

/* First write the Normal values, reg addresses, tbl id and delimiters */
	for (reg_list_ptr = hwaci_fcbs_rfregs; *reg_list_ptr != 0xFFFF;  reg_list_ptr++) {
		val = phy_utils_read_radioreg(pi, *reg_list_ptr);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex,
			16, reg_list_ptr);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex+1,
			16, ptr_val);
		fastchswTableIndex += 3;
	}

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex,
		16, &fcbs_table_delim);
	fastchswTableIndex += 1;

	for (reg_list_ptr = hwaci_fcbs_phyregs; *reg_list_ptr != 0xFFFF;  reg_list_ptr++) {
		val = phy_utils_read_phyreg(pi, *reg_list_ptr);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex,
			16, reg_list_ptr);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex+1,
			16, ptr_val);
		fastchswTableIndex += 3;
	}

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex,
		16, &fcbs_table_delim);
	fastchswTableIndex += 1;


	for (tbl_list_ptr = hwaci_fcbs_phytbls; tbl_list_ptr->tbl_id != 0xFFFF;  tbl_list_ptr++) {
		uint16 tbl_id = tbl_list_ptr->tbl_id | FCBS_1DATA_PER_ADDR;

		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex,
			16, &tbl_id);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex+1,
			16, &tbl_list_ptr->tbl_offset);
		/* calc end offset */
		val = tbl_list_ptr->tbl_offset + tbl_list_ptr->num_entries - 1;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex+2,
			16, ptr_val);

		fastchswTableIndex += 3; /* point to first normal entry */
		for (i = 0; i < tbl_list_ptr->num_entries;  i++) {
			wlc_phy_table_read_acphy(pi, tbl_list_ptr->tbl_id, 1,
				tbl_list_ptr->tbl_offset + i, 16, ptr_val);
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1,
				fastchswTableIndex, 16, ptr_val);
/* set to next normal entry OR table id  OR delimiter */
			fastchswTableIndex += 2;
		}
	}

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex,
		16, &fcbs_table_delim);

/* Write the ACI values */

	fastchswTableIndex = startoffset;

	/* save TIA wrssi3 ref values */
	tia_cfg12_wrssi3_ref_high_sel = READ_RADIO_REGFLD_20691(pi, TIA_CFG12, 0,
	                                                        wrssi3_ref_high_sel);
	tia_cfg12_wrssi3_ref_mid_sel = READ_RADIO_REGFLD_20691(pi, TIA_CFG12, 0,
	                                                       wrssi3_ref_mid_sel);
	tia_cfg13_wrssi3_ref_low_sel = READ_RADIO_REGFLD_20691(pi, TIA_CFG13, 0,
	                                                       wrssi3_ref_low_sel);

	curr_desense = phy_ac_rxgcrs_get_desense(pi_ac->rxgcrsi, CURR_DESENSE);
	curr_desense.analog_gain_desense_ofdm = HWACI_OFDM_DESENSE;
	curr_desense.analog_gain_desense_bphy = HWACI_BPHY_DESENSE;
	curr_desense.lna1_tbl_desense = HWACI_LNA1_DESENSE;
	curr_desense.clipgain_desense[0] = HWACI_CLIP_INIT_DESENSE;
	curr_desense.clipgain_desense[1] = HWACI_CLIP_HIGH_DESENSE;
	curr_desense.clipgain_desense[2] = HWACI_CLIP_MED_DESENSE;
	curr_desense.clipgain_desense[3] = HWACI_CLIP_LO_DESENSE;
	phy_ac_rxgcrs_set_desense(pi_ac->rxgcrsi, &curr_desense, CURR_DESENSE);

	wlc_phy_rxgainctrl_set_gaintbls_acphy_tiny(pi, 0, ACPHY_TBL_ID_GAIN0,
		ACPHY_TBL_ID_GAINBITS0);
	wlc_phy_rxgainctrl_gainctrl_acphy_tiny(pi, 0);

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_20691_ENTRY(pi, TIA_CFG12, 0, wrssi3_ref_high_sel, 0)
		MOD_RADIO_REG_20691_ENTRY(pi, TIA_CFG12, 0, wrssi3_ref_mid_sel, 0)
		MOD_RADIO_REG_20691_ENTRY(pi, TIA_CFG13, 0, wrssi3_ref_low_sel, 0)
	ACPHY_REG_LIST_EXECUTE(pi);

	for (reg_list_ptr = hwaci_fcbs_rfregs; *reg_list_ptr != 0xFFFF;  reg_list_ptr++) {
		val = phy_utils_read_radioreg(pi, *reg_list_ptr);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex+2,
			16, ptr_val);
		fastchswTableIndex += 3;
	}

	fastchswTableIndex += 1; /* skip delimiter */

	for (reg_list_ptr = hwaci_fcbs_phyregs; *reg_list_ptr != 0xFFFF;  reg_list_ptr++) {
		val = phy_utils_read_phyreg(pi, *reg_list_ptr);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex+2,
			16, ptr_val);
		fastchswTableIndex += 3;
	}

	fastchswTableIndex += 1;  /* skip delimiter */

	for (tbl_list_ptr = hwaci_fcbs_phytbls; tbl_list_ptr->tbl_id != 0xFFFF;  tbl_list_ptr++) {
		fastchswTableIndex += 4;   /* point to first aci entry */
		for (i = 0; i < tbl_list_ptr->num_entries;  i++) {
			wlc_phy_table_read_acphy(pi, tbl_list_ptr->tbl_id,  1,
				tbl_list_ptr->tbl_offset + i, 16, ptr_val);
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1,
				fastchswTableIndex,	16, ptr_val);
			/* point to next aci entry, next table, or delimiter */
			fastchswTableIndex += 2;
		}
		fastchswTableIndex -= 1;  /* back up one to point to Table ID entry */
	}

	fastchswTableIndex += 1; /* skip delimiter */
	while ((fastchswTableIndex) < 800) {
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_FASTCHSWITCH, 1, fastchswTableIndex,
			16, &fcbs_table_delim);
		fastchswTableIndex += 1;
	}

	curr_desense = phy_ac_rxgcrs_get_desense(pi_ac->rxgcrsi, CURR_DESENSE);
	curr_desense.analog_gain_desense_ofdm = 0;
	curr_desense.analog_gain_desense_bphy = 0;
	curr_desense.lna1_tbl_desense = 0;
	curr_desense.clipgain_desense[0] = 0;
	curr_desense.clipgain_desense[1] = 0;
	curr_desense.clipgain_desense[2] = 0;
	curr_desense.clipgain_desense[3] = 0;
	phy_ac_rxgcrs_set_desense(pi_ac->rxgcrsi, &curr_desense, CURR_DESENSE);

	/* restore TIA wrssi3 ref values */
	MOD_RADIO_REG_20691(pi, TIA_CFG12, 0, wrssi3_ref_high_sel, tia_cfg12_wrssi3_ref_high_sel);
	MOD_RADIO_REG_20691(pi, TIA_CFG12, 0, wrssi3_ref_mid_sel, tia_cfg12_wrssi3_ref_mid_sel);
	MOD_RADIO_REG_20691(pi, TIA_CFG13, 0, wrssi3_ref_low_sel, tia_cfg13_wrssi3_ref_low_sel);

	wlc_phy_rxgainctrl_set_gaintbls_acphy_tiny(pi, 0, ACPHY_TBL_ID_GAIN0,
		ACPHY_TBL_ID_GAINBITS0);
	wlc_phy_rxgainctrl_gainctrl_acphy_tiny(pi, 0);

}
#endif /* WL_ACI_FCBS */

/* Update chan stats offline, i.e. we might not be on this channel currently */

static acphy_aci_params_t*
wlc_phy_desense_aci_getset_chanidx_acphy(phy_info_t *pi, chanspec_t chanspec, bool create)
{
	uint8 idx, oldest_idx;
	uint64 oldest_time;
	acphy_aci_params_t *ret = NULL;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	acphy_aci_params_t *aci_list;

	aci_list = CHSPEC_IS2G(chanspec) ? pi_ac->noisei->aci_list2g : pi_ac->noisei->aci_list5g;

	/* Find if this chan/bw already exists */
	for (idx = 0; idx < ACPHY_ACI_CHAN_LIST_SZ; idx++) {
		if ((aci_list[idx].chan == CHSPEC_CHANNEL(chanspec)) &&
		    (aci_list[idx].bw == CHSPEC_BW(chanspec))) {
			ret = &aci_list[idx];
			PHY_ACI(("aci_debug. *** old_chan. idx = %d, chan = %d, bw = %d\n",
			         idx, ret->chan, ret->bw));
			break;
		}
	}

	/* If doesn't exist & don't want to create one */
	if ((ret == NULL) && !create) return ret;

	if (ret == NULL) {
		/* Chan/BW does not exist on in the list of ACI channels.
		   Create a new one (based on oldest timestamp)
		*/
		oldest_idx = 0; oldest_time = aci_list[oldest_idx].last_updated;
		for (idx = 1; idx < ACPHY_ACI_CHAN_LIST_SZ; idx++) {
			if (aci_list[idx].last_updated < oldest_time) {
				oldest_time = aci_list[idx].last_updated;
				oldest_idx = idx;
			}
		}

		/* Clear the new aciinfo data */
		ret = &aci_list[oldest_idx];
		bzero(ret, sizeof(acphy_aci_params_t));
		ret->chan =  CHSPEC_CHANNEL(pi->radio_chanspec);
		ret->bw = pi->bw;
		ret->glitch_upd_wait = 2;
		PHY_ACI(("aci_debug, *** new_chan = %d %d, idx = %d\n",
		         CHSPEC_CHANNEL(pi->radio_chanspec), pi->bw, oldest_idx));
	}

	/* Only if the request came for creation */
	if (create) {
		ret->last_updated = phy_utils_get_time_usec(pi);
	}

	return ret;
}

void
wlc_phy_desense_aci_engine_acphy(phy_info_t *pi)
{

	uint8 ma_idx, badplcp_idx, i, glitch_idx;
	bool call_mitigation = FALSE;
	uint32 ofdm_glitches, bphy_glitches;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint32 avg_glitch_ofdm, avg_glitch_bphy;
	uint8 new_bphy_desense, new_ofdm_desense;
	acphy_desense_values_t *desense;
	acphy_desense_values_t zero_desense;
	acphy_aci_params_t *aci;

	if (pi_ac->noisei->aci == NULL) {
		pi_ac->noisei->aci = wlc_phy_desense_aci_getset_chanidx_acphy(pi,
			pi->radio_chanspec, TRUE);
	}

	aci = pi_ac->noisei->aci;
	desense = &aci->desense;

	if (aci->glitch_upd_wait > 0) {
		aci->glitch_upd_wait--;
		return;
	}

	/* bphy glitches/badplcp */
	ma_idx = (pi->interf->noise.bphy_ma_index == 0) ? PHY_NOISE_MA_WINDOW_SZ - 1 :
	        pi->interf->noise.bphy_ma_index - 1;
	badplcp_idx = (pi->interf->noise.bphy_badplcp_ma_index == 0) ? PHY_NOISE_MA_WINDOW_SZ - 1 :
	        pi->interf->noise.bphy_badplcp_ma_index - 1;
	bphy_glitches =  pi->interf->noise.bphy_glitch_ma_list[ma_idx] +
	        (2 * pi->interf->noise.bphy_badplcp_ma_list[badplcp_idx]);
	PHY_ACI(("aci_mode1, bphy(glitch, badplcp) = %d %d \n",
	         pi->interf->noise.bphy_glitch_ma_list[ma_idx],
	         pi->interf->noise.bphy_badplcp_ma_list[badplcp_idx]));

	/* Ofdm glitches/badplcp */
	ma_idx = (pi->interf->noise.ofdm_ma_index == 0) ? PHY_NOISE_MA_WINDOW_SZ - 1 :
	        pi->interf->noise.ofdm_ma_index - 1;
	badplcp_idx = (pi->interf->noise.ofdm_badplcp_ma_index == 0) ? PHY_NOISE_MA_WINDOW_SZ - 1 :
	        pi->interf->noise.ofdm_badplcp_ma_index - 1;
	ofdm_glitches =  pi->interf->noise.ofdm_glitch_ma_list[ma_idx] +
	        (2 * pi->interf->noise.ofdm_badplcp_ma_list[badplcp_idx]);
	PHY_ACI(("aci_mode1, ofdm(glitch, badplcp) = %d %d \n",
	         pi->interf->noise.ofdm_glitch_ma_list[ma_idx],
	         pi->interf->noise.ofdm_badplcp_ma_list[badplcp_idx]));

	/* Update glitch history */
	glitch_idx = aci->glitch_buff_idx;
	aci->ofdm_hist.glitches[glitch_idx] = ofdm_glitches;
	aci->bphy_hist.glitches[glitch_idx] = bphy_glitches;
	aci->glitch_buff_idx = (glitch_idx + 1) % ACPHY_ACI_GLITCH_BUFFER_SZ;

	PHY_ACI(("aci_mode1, bphy idx = %d, glitch + (2 * badplcp) = ", glitch_idx));
	for (i = 0; i < ACPHY_ACI_GLITCH_BUFFER_SZ; i++)
		PHY_ACI(("%d ", aci->bphy_hist.glitches[i]));
	PHY_ACI(("\n"));

	PHY_ACI(("aci_mode1, ofdm idx = %d, glitch + (2 * badplcp) = ", glitch_idx));
	for (i = 0; i < ACPHY_ACI_GLITCH_BUFFER_SZ; i++)
		PHY_ACI(("%d ", aci->ofdm_hist.glitches[i]));
	PHY_ACI(("\n"));

	/* Find AVG of Max glitches in last N seconds */
	avg_glitch_bphy =
	        wlc_phy_desense_aci_get_avg_max_glitches_acphy(aci->bphy_hist.glitches);
	avg_glitch_ofdm =
	        wlc_phy_desense_aci_get_avg_max_glitches_acphy(aci->ofdm_hist.glitches);

	PHY_ACI(("aci_mode1, max {bphy, ofdm} = {%d %d}, rssi = %d, aci_on = %d\n",
	         avg_glitch_bphy, avg_glitch_ofdm, aci->weakest_rssi, desense->on));

	/* Don't need to do anything is interference mitigation is off & glitches < thresh */
	/* Using different MACROS for 4349
	 */
	if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev) || ACMAJORREV_37(pi->pubpi->phy_rev) ||
		ACMAJORREV_36(pi->pubpi->phy_rev)) {
		if (!(desense->on || (avg_glitch_bphy > ACPHY_ACI_BPHY_HI_GLITCH_THRESH) ||
			(avg_glitch_ofdm > ACPHY_ACI_OFDM_HI_GLITCH_THRESH_TINY)))
			return;
	} else {
		if (!(desense->on || (avg_glitch_bphy > ACPHY_ACI_BPHY_HI_GLITCH_THRESH) ||
			(avg_glitch_ofdm > ACPHY_ACI_OFDM_HI_GLITCH_THRESH)))
			return;
	}

	new_bphy_desense = wlc_phy_desense_aci_calc_acphy(pi, &aci->bphy_hist,
	                                             desense->bphy_desense,
	                                             avg_glitch_bphy,
	                                             ACPHY_ACI_BPHY_LO_GLITCH_THRESH,
	                                             ACPHY_ACI_BPHY_HI_GLITCH_THRESH);

	if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev) || ACMAJORREV_37(pi->pubpi->phy_rev) ||
		ACMAJORREV_36(pi->pubpi->phy_rev)) {
		new_ofdm_desense = wlc_phy_desense_aci_calc_acphy(pi, &aci->ofdm_hist,
			desense->ofdm_desense,
			avg_glitch_ofdm,
			ACPHY_ACI_OFDM_LO_GLITCH_THRESH_TINY,
			ACPHY_ACI_OFDM_HI_GLITCH_THRESH_TINY);
	} else {
		new_ofdm_desense = wlc_phy_desense_aci_calc_acphy(pi, &aci->ofdm_hist,
			desense->ofdm_desense,
			avg_glitch_ofdm,
			ACPHY_ACI_OFDM_LO_GLITCH_THRESH,
			ACPHY_ACI_OFDM_HI_GLITCH_THRESH);
	}


	/* Limit desnese */
	new_bphy_desense = MIN(new_bphy_desense, ACPHY_ACI_MAX_DESENSE_BPHY_DB);
	new_ofdm_desense = MIN(new_ofdm_desense, ACPHY_ACI_MAX_DESENSE_OFDM_DB);

	if (ASSOC_INPROG_PHY(pi) || PUB_NOT_ASSOC(pi)) {
		new_bphy_desense = 0;
		new_ofdm_desense = 0;
	}

	new_bphy_desense = MIN(new_bphy_desense, MAX(0, aci->weakest_rssi + 85));
	new_ofdm_desense = MIN(new_ofdm_desense, MAX(0, aci->weakest_rssi + 80));

	PHY_ACI(("aci_mode1, old desense = {%d %d}, new = {%d %d}\n",
	         desense->bphy_desense,
	         desense->ofdm_desense,
	         new_bphy_desense, new_ofdm_desense));

	if (new_bphy_desense != desense->bphy_desense) {
		call_mitigation = TRUE;
		desense->bphy_desense = new_bphy_desense;

		/* Clear old glitch history when desnese changed */
		for (i = 0; i <  ACPHY_ACI_GLITCH_BUFFER_SZ; i++)
			aci->bphy_hist.glitches[i] = ACPHY_ACI_BPHY_LO_GLITCH_THRESH;
	}

	if (new_ofdm_desense != desense->ofdm_desense) {
		call_mitigation = TRUE;
		desense->ofdm_desense = new_ofdm_desense;

		/* Clear old glitch history when desnese changed */
		if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
		    ACMAJORREV_33(pi->pubpi->phy_rev) || ACMAJORREV_37(pi->pubpi->phy_rev) ||
		    ACMAJORREV_36(pi->pubpi->phy_rev)) {
			for (i = 0; i <  ACPHY_ACI_GLITCH_BUFFER_SZ; i++)
				aci->ofdm_hist.glitches[i] = ACPHY_ACI_OFDM_LO_GLITCH_THRESH_TINY;
		} else {
			for (i = 0; i <  ACPHY_ACI_GLITCH_BUFFER_SZ; i++)
				aci->ofdm_hist.glitches[i] = ACPHY_ACI_OFDM_LO_GLITCH_THRESH;
		}
	}

	desense->on = FALSE;
	zero_desense = phy_ac_rxgcrs_get_desense(pi_ac->rxgcrsi, ZERO_DESENSE);
	desense->on = (memcmp(&zero_desense, desense, sizeof(acphy_desense_values_t)) != 0);

	if (call_mitigation) {
		PHY_ACI(("aci_mode1 : desense = %d %d\n",
		         desense->bphy_desense, desense->ofdm_desense));
		wlc_phy_desense_apply_acphy(pi, TRUE);

		/* After gain change, it takes a while for updated glitches to show up */
		aci->glitch_upd_wait = ACPHY_ACI_WAIT_POST_MITIGATION;
	}
}

#endif /* WLC_DISABLE_ACI */

#ifndef WL_ACI_FCBS
static void
phy_ac_noise_aci_mitigate(phy_type_noise_ctx_t *ctx)
{
	phy_ac_noise_info_t *info = (phy_ac_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;
	phy_info_acphy_t *pi_ac = info->ac_info;

	uint16 m_hwaci_st_offset =
		((wlapi_bmac_read_shm(pi->sh->physhim,
			M_PSM2HOST_EXT_PTR(pi)) + 0x20) << 1);

	pi_ac->hw_aci_status = wlapi_bmac_read_shm(pi->sh->physhim, m_hwaci_st_offset) & 1;

#ifndef WLC_DISABLE_ACI
	if	((ACMAJORREV_5(pi->pubpi->phy_rev) &&
	  (phy_ac_btcx_get_btc_mode(pi_ac->btcxi) == 0)) || !ACMAJORREV_5(pi->pubpi->phy_rev)) {
		if ((pi_ac->gain_idx_forced == 0xffff) &&
			(info->hwaci_desense_state_ovr == -1)) {
				if (info->aci == NULL) {
					info->aci = wlc_phy_desense_aci_getset_chanidx_acphy(pi,
						pi->radio_chanspec, TRUE);
				}
			wlc_phy_hwaci_mitigate_acphy(pi, pi_ac->hw_aci_status);
		}
	}

	/*
	 * Currently this feature is enabled in ucode for CoreRevIds 47 and 48. In the
	 * driver it is functional only for MajorRev3 (4345b1) and not for 4350c2
	 * (Phyrev 14).
	 */
	if ((ACMAJORREV_3(pi->pubpi->phy_rev) ||
		ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_36(pi->pubpi->phy_rev)) &&
		(pi->sh->interference_mode & ACPHY_HWACI_MITIGATION)) {
		if (info->aci == NULL) {
			info->aci = wlc_phy_desense_aci_getset_chanidx_acphy(pi,
				pi->radio_chanspec, TRUE);
		}

		if (pi_ac->hw_aci_status == 0) {
			info->aci->desense.analog_gain_desense_ofdm = 0;
			info->aci->desense.analog_gain_desense_bphy = 0;
			info->aci->desense.lna1_tbl_desense = 0;
			info->aci->desense.lna2_tbl_desense = 0;
			info->aci->desense.clipgain_desense[0] = 0;
			info->aci->desense.clipgain_desense[1] = 0;
			info->aci->desense.clipgain_desense[2] = 0;
			info->aci->desense.clipgain_desense[3] = 0;
			wlc_phy_desense_apply_acphy(pi, TRUE);
		} else {
			info->aci->desense.analog_gain_desense_ofdm = HWACI_OFDM_DESENSE;
			info->aci->desense.analog_gain_desense_bphy = HWACI_BPHY_DESENSE;
			info->aci->desense.lna1_tbl_desense = HWACI_LNA1_DESENSE;
			if (PHY_ILNA(pi))
				info->aci->desense.lna2_tbl_desense = HWACI_LNA2_DESENSE;
			else
				info->aci->desense.lna2_tbl_desense = 0;
			info->aci->desense.clipgain_desense[0] = HWACI_CLIP_INIT_DESENSE;
			info->aci->desense.clipgain_desense[1] = HWACI_CLIP_HIGH_DESENSE;
			info->aci->desense.clipgain_desense[2] = HWACI_CLIP_MED_DESENSE;
			info->aci->desense.clipgain_desense[3] = HWACI_CLIP_LO_DESENSE;
			wlc_phy_desense_apply_acphy(pi, TRUE);
		}
	} else if ((ACMAJORREV_32(pi->pubpi->phy_rev) ||
	            ACMAJORREV_33(pi->pubpi->phy_rev) ||
	            ACMAJORREV_37(pi->pubpi->phy_rev)) &&
			((pi->sh->interference_mode & ACPHY_HWACI_MITIGATION) != 0)) {
			phy_ac_noise_hwaci_mitigation_tiny(info);
	}
#endif /* WLC_DISABLE_ACI */
}
#endif /* !WL_ACI_FCBS */

void
wlc_phy_reset_noise_var_shaping_acphy(phy_info_t *pi)
{
	uint8 i;
	uint32 zeroval = 0;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_nshapetbl_mon_t* nshapetbl_mon = pi_ac->noisei->nshapetbl_mon;
	uint8* offset = nshapetbl_mon->offset;
	uint8 stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	/* Do table writes only if table has been modified */
	if (nshapetbl_mon->mod_flag) {

		/* Reset only already-modified entries */
		for (i = 0; i < ACPHY_SPURWAR_NV_NTONES; i++) {
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_NVNOISESHAPINGTBL,
			   1, offset[i], 32, &zeroval);
			PHY_INFORM(("wlc_phy_reset_noise_var_shaping_acphy: offset %d; val 0x00\n",
			            offset[i]));
		}

		/* Invalidate the monitor upon reseting the nvar shaping table */
		nshapetbl_mon->mod_flag = 0;
	}
	ACPHY_ENABLE_STALL(pi, stall_val);
}

void
wlc_phy_noise_var_shaping_acphy(phy_info_t *pi, uint8 core_nv, uint8 core_sp, int8 *tone_id,
                                        uint8 noise_var[][ACPHY_SPURWAR_NV_NTONES], uint8 reset)
{
	uint8 i, core;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_nshapetbl_mon_t* nshapetbl_mon = pi_ac->noisei->nshapetbl_mon;
	uint8* offset = nshapetbl_mon->offset;
	uint32 tbllen;
	uint32 nvar;
	uint8 stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	if (CHSPEC_IS80(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec)) {
		/* 80mhz */
		tbllen = 256;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		/* 40mhz */
		tbllen = 128;
	} else {
		/* 20mhz */
		tbllen = 64;
	}

	/* total tones should be equal to (nvshp + sp) tones */
	ASSERT(ACPHY_SPURWAR_NV_NTONES == ACPHY_NV_NTONES + ACPHY_SPURWAR_NTONES);

	for (i = 0; i < ACPHY_SPURWAR_NV_NTONES; i++) {
		nvar = 0;
		/* Wrap around up to tbllen */
		offset[i] = (tone_id[i] >= 0)? tone_id[i] : (tbllen + tone_id[i]);
		/* Using separate core value to have flexibility
		 * of doing nvshp & spurwar on different cores
		 * for multiple core chips without increasing
		 * number of table writes
		 */
		FOREACH_CORE(pi, core) {
			if (i < ACPHY_NV_NTONES) {
				nvar |= (core_nv & (0x1 << core))? ((noise_var[core][i] << 8*core) &
					(0xFF << 8*core)): 0x0; /* nvshp tones */
			} else {
				nvar |= (core_sp & (0x1 << core))? ((noise_var[core][i] << 8*core) &
					(0xFF << 8*core)): 0x0; /* spurwar tones */
			}
		}

		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_NVNOISESHAPINGTBL,
		                          1, offset[i], 32, &nvar);

		PHY_INFORM(("wlc_phy_reset_noise_var_shaping_acphy: offset %d; val 0x%x\n",
		            offset[i], nvar));
	}
	/* activate monitor flag */
	nshapetbl_mon->mod_flag = 1;
	ACPHY_ENABLE_STALL(pi, stall_val);
}

#ifndef WLC_DISABLE_ACI

void
wlc_phy_hwaci_override_acphy(phy_info_t *pi, int state)
{

	if ((pi->sh->interference_mode & ACPHY_HWACI_MITIGATION) != 0) {
		/* This part of function is for HW Mitigation */
		if (state == 0) {
			if (TINY_RADIO(pi) || ACMAJORREV_37(pi->pubpi->phy_rev)) {
				/* Force Normal gain table */
				wlc_phy_hwaci_mitigation_enable_acphy(pi, 2, TRUE);
			} else {
				ACPHY_REG_LIST_START
					/* Make sure HWACI never triggers. */
					WRITE_PHYREG_ENTRY(pi, ACI_Detect_energy_threshold_1_w3,
						0xfffe)
					WRITE_PHYREG_ENTRY(pi, ACI_Detect_energy_threshold_2_w3,
						0xfffe)
					WRITE_PHYREG_ENTRY(pi, ACI_Detect_energy_threshold_1_w12,
						0xfffe)
					WRITE_PHYREG_ENTRY(pi, ACI_Detect_energy_threshold_2_w12,
						0xfffe)
				ACPHY_REG_LIST_EXECUTE(pi);
				pi->u.pi_acphy->hw_aci_status = FALSE;
				wlc_phy_hwaci_mitigate_acphy(pi, FALSE);
			}
		} else if (state == 1) {
			if (TINY_RADIO(pi) || ACMAJORREV_37(pi->pubpi->phy_rev)) {
				/* Force ACI gain table */
				wlc_phy_hwaci_mitigation_enable_acphy(pi, 3, TRUE);
			} else {
				ACPHY_REG_LIST_START
					/* Make sure HWACI is ALWAYS detected. */
					WRITE_PHYREG_ENTRY(pi, ACI_Detect_energy_threshold_1_w3,
						0x1)
					WRITE_PHYREG_ENTRY(pi, ACI_Detect_energy_threshold_2_w3,
						0x1)
					WRITE_PHYREG_ENTRY(pi, ACI_Detect_energy_threshold_1_w12,
						0x1)
					WRITE_PHYREG_ENTRY(pi, ACI_Detect_energy_threshold_2_w12,
						0x1)
				ACPHY_REG_LIST_EXECUTE(pi);
				pi->u.pi_acphy->hw_aci_status = TRUE;
				wlc_phy_hwaci_mitigate_acphy(pi, TRUE);
			}
		} else if (state == -1) {
			/* Default Mode */
			if (TINY_RADIO(pi) || ACMAJORREV_37(pi->pubpi->phy_rev)) {
				wlc_phy_hwaci_mitigation_enable_acphy(pi, 5, TRUE);
			} else {
				wlc_phy_hwaci_setup_acphy(pi, TRUE, TRUE);
				pi->u.pi_acphy->hw_aci_status = FALSE;
				wlc_phy_hwaci_mitigate_acphy(pi, 0);
			}
		}

	} else if ((pi->sh->interference_mode & ACPHY_ACI_HWACI_PKTGAINLMT) && (state >= 0)) {
		/* This part of function is for SW Mitigation */
		int lna1_idx, lna2_idx;

		phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
		acphy_aci_params_t *aci;
		acphy_desense_values_t *desense;
		acphy_desense_values_t total_desense;
		uint8 band = CHSPEC_IS2G(pi->radio_chanspec) ? 2 : 5;
		acphy_hwaci_state_t  *hwaci_states;

		/* Get current channels ACI structure */
		if (pi_ac->noisei->aci == NULL)
			pi_ac->noisei->aci = wlc_phy_desense_aci_getset_chanidx_acphy(pi,
				pi->radio_chanspec, TRUE);
		aci = pi_ac->noisei->aci;

		desense = &aci->desense;

		pi_ac->noisei->aci->hwaci_desense_state = (uint8)state;

		/* Update Desense settings */
		ASSERT(pi_ac->noisei->hwaci_args != NULL);
		hwaci_states = (band == 2)
			? pi_ac->noisei->hwaci_states_2g : pi_ac->noisei->hwaci_states_5g;
		lna1_idx = hwaci_states[state].lna1_pktg_lmt;
		lna2_idx = hwaci_states[state].lna2_pktg_lmt;

		desense->lna1_gainlmt_desense = MAX(0, 5 - lna1_idx);
		desense->lna2_gainlmt_desense = MAX(0, 6 - lna2_idx);
		total_desense = phy_ac_rxgcrs_get_desense(pi_ac->rxgcrsi, TOTAL_DESENSE);
		total_desense.lna1_gainlmt_desense = desense->lna1_gainlmt_desense;
		total_desense.lna2_gainlmt_desense = desense->lna2_gainlmt_desense;
		phy_ac_rxgcrs_set_desense(pi_ac->rxgcrsi, &total_desense, TOTAL_DESENSE);

		/* Update the tables. If other things are updated may need to call gainctrl */
		wlc_phy_upd_lna1_lna2_gainlimittbls_acphy(pi, 1);
		wlc_phy_upd_lna1_lna2_gainlimittbls_acphy(pi, 2);
	}
}

void
phy_ac_noise_hwaci_mitigation_tiny(phy_ac_noise_info_t *ni)
{
	phy_info_t *pi = ni->pi;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	acphy_aci_params_t *aci;
	acphy_desense_values_t *desense;
	uint8 band = CHSPEC_IS2G(pi->radio_chanspec) ? 2 : 5;
	uint8 prev_desense_state;
	bool hwaci_present = FALSE, aci_present;
	bool hwaci;
	acphy_hwaci_state_t  *hwaci_states = (band == 2) ? ni->hwaci_states_2g :
	        ni->hwaci_states_5g;

	hwaci = (pi->sh->interference_mode & ACPHY_HWACI_MITIGATION) != 0;
	if (!hwaci) return;

	/* Get current channels ACI structure */
	if (ni->aci == NULL) {
		ni->aci = wlc_phy_desense_aci_getset_chanidx_acphy(pi, pi->radio_chanspec, TRUE);
	}
	aci = ni->aci;
	desense = &aci->desense;

	PHY_ACI(("desense_state = %d, hwaci_sleep = %d\n", aci->hwaci_desense_state,
		aci->hwaci_sleep));

	prev_desense_state = aci->hwaci_desense_state;
	if (hwaci) {
		/* HW ACI Output */
		hwaci_present = pi_ac->hw_aci_status;
	}

	aci_present = hwaci_present;
	aci->hwaci_desense_state = hwaci_present;

	if (prev_desense_state != aci->hwaci_desense_state) {
		/* Suspend mac before accessing phyregs */
		wlapi_suspend_mac_and_wait(pi->sh->physhim);

		/* Update Desense settings */
		desense->lna1_idx_min = hwaci_states[aci_present].lna1_idx_min;
		desense->lna1_idx_max = hwaci_states[aci_present].lna1_idx_max;
		desense->lna2_idx_min = hwaci_states[aci_present].lna2_idx_min;
		desense->lna2_idx_max = hwaci_states[aci_present].lna2_idx_max;
		desense->mix_idx_min = hwaci_states[aci_present].mix_idx_min;
		desense->mix_idx_max = hwaci_states[aci_present].mix_idx_max;

		/* Update the tables. If other things are updated may need to call gainctrl */
		wlc_phy_desense_calc_total_acphy(pi_ac->rxgcrsi); /* Update in total desense */
		wlc_phy_upd_lna1_lna2_gains_acphy(pi);

		phy_ac_rxgcrs_upd_mix_gains(pi_ac->rxgcrsi);
		/* Inform rate contorl to slow down is mitigation is on */
		wlc_phy_aci_updsts_acphy(pi);

		wlc_phy_rxgainctrl_gainctrl_acphy_tiny(pi, 0);
		PHY_ACI(("DESENSE ST CHG. state = %d --- LNA1_minmax = (%d, %d), "
				"LNA2_minmax = (%d, %d), TIA_minmax = (%d, %d)\n",
				aci->hwaci_desense_state, desense->lna1_idx_min,
				desense->lna1_idx_max, desense->lna2_idx_min,
				desense->lna2_idx_max, desense->mix_idx_min,
				desense->mix_idx_max));
	}

	wlapi_enable_mac(pi->sh->physhim);
}

void
wlc_phy_hwaci_engine_acphy(phy_info_t *pi)
{
	uint32 hw_detect, hw_report, sw_detect, sw_report;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	acphy_aci_params_t *aci;
	acphy_desense_values_t *desense;
	uint8 band = CHSPEC_IS2G(pi->radio_chanspec) ? 2 : 5;
	uint8 core, core_ctr, lna1_idx, lna2_idx, lna1_rout;
	uint16 thresh;
	uint8 state, prev_setup_state, prev_desense_state;
	bool hwaci_present = FALSE, w2aci_present = FALSE, aci_present;
	uint8 w2sel, w2thresh, w2[3], nb, max_states;
	bool hwaci, w2aci;
	acphy_hwaci_state_t  *hwaci_states = (band == 2) ? pi_ac->noisei->hwaci_states_2g :
	        pi_ac->noisei->hwaci_states_5g;
	uint8 shft = CHSPEC_BW_LE20(pi->radio_chanspec) ? 0 :
	        (CHSPEC_IS40(pi->radio_chanspec) ? 1 : 2);

	ASSERT(!ACPHY_ENABLE_FCBS_HWACI(pi));

	hwaci = (pi->sh->interference_mode & ACPHY_ACI_HWACI_PKTGAINLMT) != 0;
	w2aci = (pi->sh->interference_mode & ACPHY_ACI_W2NB_PKTGAINLMT) != 0;
	if (!(hwaci | w2aci)) return;

	/* Get current channels ACI structure */
	if (pi_ac->noisei->aci == NULL) {
		pi_ac->noisei->aci = wlc_phy_desense_aci_getset_chanidx_acphy(pi,
			pi->radio_chanspec, TRUE);
	}
	aci = pi_ac->noisei->aci;
	desense = &aci->desense;

	if (aci->hwaci_sleep > 0) {
		aci->hwaci_sleep--;
		return;
	}

	/* Don't use highest desense level based on SOI RSSi */
	max_states = (band == 2) ? pi_ac->noisei->hwaci_max_states_2g :
		pi_ac->noisei->hwaci_max_states_5g;
	if (aci->weakest_rssi < -75)
		max_states = MAX(1, max_states - 1);

	aci->hwaci_noaci_timer = MAX(0, aci->hwaci_noaci_timer - 1);
	state = aci->hwaci_setup_state;

	PHY_ACI(("aci_mode2_4. setup_state = %d, timer = %d\n", state, aci->hwaci_noaci_timer));

	/* Suspend mac before accessing phyregs */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);

	/* Disable BT as it affects rssi counters used for w2aci */
	wlc_btcx_override_enable(pi);

	if (hwaci) {
		/* HW ACI Output */
		hw_detect = 0; hw_report = 0; sw_detect = 0; sw_report = 0; core_ctr = 0;
		FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
			if (core == 0) {
				hw_detect += READ_PHYREG(pi, ACI_Detect_aci_detected_ctr0);
				hw_report += READ_PHYREG(pi, ACI_Detect_aci_report_ctr0);
				sw_detect += READ_PHYREG(pi, ACI_Detect_sw_aci_detected_ctr0);
				sw_report += READ_PHYREG(pi, ACI_Detect_sw_aci_report_ctr0);
				core_ctr++;
			}
			if (core == 1) {
				hw_detect += READ_PHYREG(pi, ACI_Detect_aci_detected_ctr1);
				hw_report += READ_PHYREG(pi, ACI_Detect_aci_report_ctr1);
				sw_detect += READ_PHYREG(pi, ACI_Detect_sw_aci_detected_ctr1);
				sw_report += READ_PHYREG(pi, ACI_Detect_sw_aci_report_ctr1);
				core_ctr++;
			}
			if (core == 2) {
				hw_detect += READ_PHYREG(pi, ACI_Detect_aci_detected_ctr2);
				hw_report += READ_PHYREG(pi, ACI_Detect_aci_report_ctr2);
				sw_detect += READ_PHYREG(pi, ACI_Detect_sw_aci_detected_ctr2);
				sw_report += READ_PHYREG(pi, ACI_Detect_sw_aci_report_ctr2);
				core_ctr++;
			}
		}

		/* Average over cores */
		if (core_ctr > 1) {
			hw_detect /= core_ctr;
			hw_report /= core_ctr;
			sw_detect /= core_ctr;
			sw_report /= core_ctr;
		}

		if (sw_report < 200) {
			/* Try to think of doing something here */
			if (hw_report < 200) {
				/* invalid value, ignore */
				sw_detect = 0;
			} else {
				sw_detect = hw_detect;
				sw_report = hw_report;
			}
		}

		/* Here, if the ratio of report/detect < 2 declare ACI */
		hwaci_present = (sw_detect > 0) & (sw_report < 4 * sw_detect);

		PHY_ACI(("aci_mode2_4(hwaci). hw_detect = %d, hw_report = %d, "
		         "sw_detect = %d, sw_report = %d, state {setup, desense} = {%d, %d}, "
		         "aci_present = %d\n",
		         hw_detect, hw_report, sw_detect, sw_report, aci->hwaci_setup_state,
		         aci->hwaci_desense_state, hwaci_present));
	}

	/* w2, nb pair */
	if (w2aci) {
		nb = READ_PHYREGFLD(pi, NbClipCnt1, NbClipCntAccum1_i) >> shft;
		w2[0] = READ_PHYREGFLD(pi, W2W1ClipCnt1, W2ClipCntAccum1) >> shft;
		w2[1] = READ_PHYREGFLD(pi, W2W1ClipCnt2, W2ClipCntAccum2) >> shft;
		w2[2] = READ_PHYREGFLD(pi, W2W1ClipCnt3, W2ClipCntAccum3) >> shft;

		w2sel = hwaci_states[state].w2_sel;
		w2thresh = hwaci_states[state].w2_thresh;
		w2aci_present = (nb <= hwaci_states[state].nb_thresh);
		if (w2sel == 0) {
			/* lo */
			w2aci_present &= (w2[0] >= w2thresh) ||
			        (w2[1] > 0) || (w2[2] > 0);
		} else if (w2sel == 1) {
			/* md */
			w2aci_present &= (w2[1] >= w2thresh) || (w2[2] > 0);
		} else {
			/* hi */
			w2aci_present &= (w2[2] >= w2thresh);
		}

		PHY_ACI(("aci_mode2_4(w2nb). w2 = {%d %d %d}, nb_lo = %d, w2aci = %d\n",
		         w2[0], w2[1], w2[2], nb, w2aci_present));
	}

	aci_present = hwaci_present | w2aci_present;

	/* ***** HWACI State Machine & Apply Settings ******* */
	prev_setup_state = aci->hwaci_setup_state;
	prev_desense_state = aci->hwaci_desense_state;
	if (aci->hwaci_setup_state == 0) {
		/* Coming here for the first time */
		aci->hwaci_setup_state = 1;
	} else {
		if (aci_present) {
			/* ACI found */
			aci->hwaci_noaci_timer = ACPHY_HWACI_NOACI_WAIT_TIME;
			aci->hwaci_desense_state = aci->hwaci_setup_state;
			aci->hwaci_setup_state++;
		} else if (aci->hwaci_noaci_timer == 0) {
			/* No ACI found */
			aci->hwaci_setup_state = MAX(0, aci->hwaci_setup_state - 1);
			if (aci->hwaci_setup_state < aci->hwaci_desense_state) {
				aci->hwaci_desense_state = aci->hwaci_setup_state;
			}
		}
	}

	/* keep setup state between 1 & max. 0 is noACI state, for setup use atleast state 1 */
	aci->hwaci_setup_state = MAX(1, MIN(aci->hwaci_setup_state, max_states - 1));
	if (ASSOC_INPROG_PHY(pi) || PUB_NOT_ASSOC(pi)) {
		aci->hwaci_setup_state = 1;
		aci->hwaci_desense_state = 0;
	}

	/* Update new settings & desense */
	if (prev_setup_state != aci->hwaci_setup_state) {
		state = aci->hwaci_setup_state;
		aci->hwaci_noaci_timer = ACPHY_HWACI_NOACI_WAIT_TIME;
		aci->hwaci_sleep = ACPHY_HWACI_SLEEP_TIME;           /* let hwaci get refreshed */

	if (AC4354REV(pi) && ((pi->sh->interference_mode & ACPHY_ACI_PREEMPTION) != 0)) {
		wlc_phy_switch_preemption_settings(pi, aci->hwaci_setup_state);
	}
		/* Update HWACI settings */
		if (hwaci) {
			thresh = hwaci_states[state].energy_thresh;
			WRITE_PHYREG(pi, ACI_Detect_energy_threshold_1, thresh);
			WRITE_PHYREG(pi, ACI_Detect_energy_threshold_2, thresh);
		}

		PHY_ACI(("aci_mode2_4. SETUP STATE CHANGED. state = {%d --> %d}\n",
		         prev_setup_state, aci->hwaci_setup_state));
	}

	if (prev_desense_state != aci->hwaci_desense_state) {
		state = aci->hwaci_desense_state;
		/* Update Desense settings */
		lna1_idx = hwaci_states[state].lna1_pktg_lmt;
		lna2_idx = hwaci_states[state].lna2_pktg_lmt;
		lna1_rout = hwaci_states[state].lna1rout_pktg_lmt;
		desense->lna1_gainlmt_desense = MAX(0, 5 - lna1_idx);
		desense->lna2_gainlmt_desense = MAX(0, 6 - lna2_idx);

		if (band == 5)
			desense->lna1rout_gainlmt_desense = MAX(0, 4 - lna1_rout);
		else
			desense->lna1rout_gainlmt_desense = lna1_rout;

		/* Update the tables. If other things are updated may need to call gainctrl */
		wlc_phy_desense_calc_total_acphy(pi_ac->rxgcrsi); /* Update in total desense */
		wlc_phy_upd_lna1_lna2_gains_acphy(pi);

		/* Inform rate contorl to slow down is mitigation is on */
		wlc_phy_aci_updsts_acphy(pi);

		PHY_ACI(("aci_mode2_4. DESENSE ST CHG. state = %d --> %d, lna1 = %d, lna2 = %d\n",
		         prev_desense_state, aci->hwaci_desense_state, lna1_idx, lna2_idx));
	}

	/* Disabling BTCX Override */
	wlc_phy_btcx_override_disable(pi);

	wlapi_enable_mac(pi->sh->physhim);
}

#endif /* WLC_DISABLE_ACI */

void
wlc_phy_set_aci_regs_acphy(phy_info_t *pi)
{
	uint16 aci_th;
	uint8 core;

	if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
		PHY_ERROR(("FIXME: 4347A0 Bypass set_aci_regs for the moment\n"));
		return;
	}
	if (ACMAJORREV_37(pi->pubpi->phy_rev)) {
		PHY_ERROR(("FIXME: 7271 Bypass set_aci_regs for the moment\n"));
		return;
	}

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
	if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_3(pi->pubpi->phy_rev) ||
		ACMAJORREV_5(pi->pubpi->phy_rev) || ACMAJORREV_4(pi->pubpi->phy_rev) ||
		ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_37(pi->pubpi->phy_rev)) {
			/* Disable aci_absent, as it casues issues SOI pkts
			   to be missed when ACI is playing, eg channel 36 & 40, pwr = -40 dBm
			 */
			aci_th = 0x1ff;
	} else {
			if (CHSPEC_IS5G(pi->radio_chanspec)) {
					aci_th = 0xbf;	/* 5GHz, 40/20MHz BW */
			} else {
					aci_th = (ACMAJORREV_1(pi->pubpi->phy_rev) ||
					ACMAJORREV_36(pi->pubpi->phy_rev)) ? 0x80 : 0xff;
			}
	}

	if (CHSPEC_IS8080(pi->radio_chanspec) || ACMAJORREV_40(pi->pubpi->phy_rev) ||
		ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_37(pi->pubpi->phy_rev)) {
		FOREACH_CORE(pi, core) {
			WRITE_PHYREGCE(pi, crsacidetectThreshl, core, aci_th);
			WRITE_PHYREGCE(pi, crsacidetectThreshu, core, aci_th);
			WRITE_PHYREGCE(pi, crsacidetectThreshlSub1, core, aci_th);
			WRITE_PHYREGCE(pi, crsacidetectThreshuSub1, core, aci_th);
		}
	} else {
		WRITE_PHYREG(pi, crsacidetectThreshl, aci_th);
		WRITE_PHYREG(pi, crsacidetectThreshu, aci_th);
		WRITE_PHYREG(pi, crsacidetectThreshlSub1, aci_th);
		WRITE_PHYREG(pi, crsacidetectThreshuSub1, aci_th);
	}

	if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
	    ACMAJORREV_33(pi->pubpi->phy_rev) ||
	    ACMAJORREV_37(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, RxControl, bphyacidetEn, 0);
		/* CRDOT11ACPHY-280 : enabled bphy aci det is causing hangs */
		if (CHSPEC_IS2G(pi->radio_chanspec) && CHSPEC_IS20(pi->radio_chanspec)) {
			ACPHY_REG_LIST_START
				/* Enable bphy ACI Detection HW */
				WRITE_PHYREG_ENTRY(pi, bphyaciThresh0, 0)
				WRITE_PHYREG_ENTRY(pi, bphyaciThresh1, 0)
				WRITE_PHYREG_ENTRY(pi, bphyaciThresh2, 0)
				WRITE_PHYREG_ENTRY(pi, bphyaciThresh3, 0x9F)
				WRITE_PHYREG_ENTRY(pi, bphyaciPwrThresh0, 0)
				WRITE_PHYREG_ENTRY(pi, bphyaciPwrThresh1, 0)
				WRITE_PHYREG_ENTRY(pi, bphyaciPwrThresh2, 0)
			ACPHY_REG_LIST_EXECUTE(pi);
		}
	}
}

#ifndef WLC_DISABLE_ACI
void
wlc_phy_desense_aci_reset_params_acphy(phy_info_t *pi, bool call_gainctrl, bool all2g, bool all5g)
{
	phy_ac_noise_info_t *noisei = pi->u.pi_acphy->noisei;

	if (all2g)
		bzero(noisei->aci_list2g,
			ACPHY_ACI_CHAN_LIST_SZ * sizeof(acphy_aci_params_t));
	if (all5g)
		bzero(noisei->aci_list5g,
			ACPHY_ACI_CHAN_LIST_SZ * sizeof(acphy_aci_params_t));

	if (all2g || all5g) {
		noisei->aci = NULL;
	} else if (noisei->aci != NULL) {
		bzero(&noisei->aci->bphy_hist, sizeof(desense_history_t));
		bzero(&noisei->aci->ofdm_hist, sizeof(desense_history_t));
		noisei->aci->glitch_buff_idx = 0;
		noisei->aci->glitch_upd_wait = 1;
		bzero(&noisei->aci->desense, sizeof(acphy_desense_values_t));
	}

	/* Call gainctrl to reset all the phy regs */
	if (call_gainctrl)
		wlc_phy_desense_apply_acphy(pi, TRUE);
}

/* Update chan stats offline, i.e. we might not be on this channel currently */
static void
wlc_phy_desense_aci_upd_chan_stats_acphy(phy_type_noise_ctx_t *ctx, chanspec_t chanspec, int8 rssi)
{
	phy_ac_noise_info_t *info = (phy_ac_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;
	acphy_aci_params_t *aci;

	aci = (acphy_aci_params_t *)
	        wlc_phy_desense_aci_getset_chanidx_acphy(pi, chanspec, FALSE);

	if (aci == NULL) return;   /* not found in phy list of channels */

	aci->weakest_rssi = rssi;
}

void
wlc_phy_aci_updsts_acphy(phy_info_t *pi)
{
	phy_ac_noise_info_t *noisei = pi->u.pi_acphy->noisei;
	acphy_aci_params_t *aci;
	uint32 phy_mode = 0;

	if (noisei->aci != NULL) {
		aci = noisei->aci;
		if (aci->desense.on || aci->hwaci_desense_state > 0)
			phy_mode = PHY_MODE_ACI;
	}

	wlapi_high_update_phy_mode(pi->sh->physhim, phy_mode);
}
#endif /* !WLC_DISABLE_ACI */

void
wlc_phy_switch_preemption_settings(phy_info_t *pi, uint8 state)
{
	/* 4354A0/A1:dynamic preemption settings based on hwaci state:
	   Aggressive settings for no_aci or aci < -40 dbm
	   & conservative settings for aci > -40 dbm
	 */
	if (!AC4354REV(pi) && !ACHWACIREV(pi)) return;

	if (IS_4364_3x3(pi)) return;

	if ((AC4354REV(pi) && ((state == 0) || (state == 1))) ||
	    (ACHWACIREV(pi) && (state == 0))) {
		/* no aci or aci < -40 dbm : Aggressive settings */
		ACPHYREG_BCAST(pi, PREMPT_per_pkt_en0, 0x31);
			if (CHSPEC_IS20(pi->radio_chanspec)) {
					ACPHYREG_BCAST(pi, PREMPT_ofdm_nominal_clip_cnt_th0, 0x24);
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
					ACPHYREG_BCAST(pi, PREMPT_ofdm_nominal_clip_cnt_th0, 0x48);
			} else {
				 ACPHYREG_BCAST(pi, PREMPT_ofdm_nominal_clip_cnt_th0, 0xa0);
			}
	} else {
	/* state is 2 or 3:aci > -40 dbm */
		ACPHYREG_BCAST(pi, PREMPT_per_pkt_en0, 0x21);
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				ACPHYREG_BCAST(pi, PREMPT_ofdm_nominal_clip_cnt_th0, 0x2d);
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				if (CHSPEC_IS5G(pi->radio_chanspec)) {
					ACPHYREG_BCAST(pi, PREMPT_ofdm_nominal_clip_cnt_th0, 0x64);
				} else {
					ACPHYREG_BCAST(pi, PREMPT_ofdm_nominal_clip_cnt_th0, 0x48);
				}
			} else {
					ACPHYREG_BCAST(pi, PREMPT_ofdm_nominal_clip_cnt_th0, 0xdc);
			}
	}
}
