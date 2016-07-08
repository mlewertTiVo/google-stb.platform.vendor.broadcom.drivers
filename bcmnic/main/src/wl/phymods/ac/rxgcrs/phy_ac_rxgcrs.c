/*
 * ACPHY Rx Gain Control and Carrier Sense module implementation
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_rxgcrs.h"
#include <phy_utils_var.h>
#include <phy_ac.h>
#include <phy_ac_rxgcrs.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_radioreg_20691.h>
#include <wlc_radioreg_20693.h>
#include <wlc_radioreg_20696.h>
#include <wlc_phy_radio.h>
#include <wlc_phyreg_ac.h>

#include <phy_utils_reg.h>
#include <phy_ac_info.h>

#include <phy_rstr.h>

/* Rx Gain Control params */
typedef struct {
	int8  lna12_gain_2g_tbl[2][N_LNA12_GAINS];
	int8  lna12_gain_5g_tbl[2][N_LNA12_GAINS];
	uint8 lna12_gainbits_2g_tbl[2][N_LNA12_GAINS];
	uint8 lna12_gainbits_5g_tbl[2][N_LNA12_GAINS];
	uint8 lna1Rout_2g_tbl[N_LNA12_GAINS];
	uint8 lna1Rout_5g_tbl[N_LNA12_GAINS];
	uint8 lna2_gm_ind_2g_tbl[N_LNA12_GAINS];
	uint8 lna2_gm_ind_5g_tbl[N_LNA12_GAINS];
	int8 gainlimit_tbl[RXGAIN_CONF_ELEMENTS][MAX_RX_GAINS_PER_ELEM];
	int8 tia_gain_tbl[N_TIA_GAINS];
	int8 tia_gainbits_tbl[N_TIA_GAINS];
	int8 biq01_gain_tbl[2][N_BIQ01_GAINS];
	int8 biq01_gainbits_tbl[2][N_BIQ01_GAINS];
	int8 fast_agc_clip_gains[4];
	int16 maxgain[ACPHY_MAX_RX_GAIN_STAGES];
	uint8 lna1_min_indx;
	uint8 lna1_max_indx;
	uint8 lna1_byp_indx;
	uint8 lna2_min_indx;
	uint8 lna2_max_indx;
	uint8 lna2_2g_min_indx;
	uint8 lna2_5g_min_indx;
	uint8 lna2_2g_max_indx;
	uint8 lna2_5g_max_indx;
	uint8 farrow_shift;
	uint8 tia_max_indx;
	uint8 max_analog_gain;
	int8 lna1_byp_gain_2g;
	int8 lna1_byp_gain_5g;
	uint8 lna1_byp_rout_2g;
	uint8 lna1_byp_rout_5g;
	int8 lna1_sat_2g;
	int8 lna1_sat_5g;
	uint8 high_sen_adjust_2g;
	uint8 high_sen_adjust_5g;

	/* SSAGC Specific */
	int8 ssagc_clip_gains[SSAGC_CLIP1_GAINS_CNT];
	uint8 ssagc_clip2_tbl[SSAGC_CLIP2_TBL_IDX_CNT];
	uint8 ssagc_rssi_thresh_2g[SSAGC_N_RSSI_THRESH];
	uint8 ssagc_rssi_thresh_5g[SSAGC_N_RSSI_THRESH];
	uint8 ssagc_clipcnt_thresh_2g[SSAGC_N_CLIPCNT_THRESH];
	uint8 ssagc_clipcnt_thresh_5g[SSAGC_N_CLIPCNT_THRESH];
	bool ssagc_lna1byp_flag[SSAGC_CLIP1_GAINS_CNT];
	bool ssagc_en;

	/* MclipAGC */
	bool mclip_agc_en;

	/* SSAGC related */
	uint8 ssagc_lna_byp_gain_thresh;

	/* Low NF ELNA mode */
	int8 fast_agc_lonf_clip_gains[4];
	uint8 lna2_lonf_force_indx_2g;
	uint8 lna2_lonf_force_indx_5g;
	uint8 lna1_lonf_max_indx;
	uint8 max_analog_gain_lonf;
	uint8 high_sen_adjust_lonf_2g;
	uint8 high_sen_adjust_lonf_5g;

	int8 lna1_sat_2g_elna;
	int8 lna1_sat_5g_elna;
} phy_ac_rxg_params_t;

/* Rx Gain Control params table ID's */
typedef enum {
	LNA12_GAIN_TBL_2G,
	LNA12_GAIN_TBL_5G,
	LNA12_GAIN_BITS_TBL_2G,
	LNA12_GAIN_BITS_TBL_5G,
	TIA_GAIN_TBL,
	TIA_GAIN_BITS_TBL,
	BIQ01_GAIN_TBL,
	BIQ01_GAIN_BITS_TBL,
	GAIN_LIMIT_TBL,
	LNA1_ROUTMAP_TBL_2G,
	LNA1_ROUTMAP_TBL_5G,
	LNA1_GAINMAP_TBL_2G,
	LNA1_GAINMAP_TBL_5G,
	LNA2_GAINMAP_TBL_2G,
	LNA2_GAINMAP_TBL_5G,
	MAX_GAIN_TBL,
	FAST_AGC_CLIP_GAIN_TBL,
	SSAGC_CLIP_GAIN_TBL,
	SSAGC_LNA1BYP_TBL,
	SSAGC_CLIP2_TBL,
	SSAGC_CLIPCNT_THRESH_TBL_2G,
	SSAGC_RSSI_THRESH_TBL_2G,
	SSAGC_CLIPCNT_THRESH_TBL_5G,
	SSAGC_RSSI_THRESH_TBL_5G,
	FAST_AGC_LONF_CLIP_GAIN_TBL
} phy_ac_rxg_param_tbl_t;

/* module private states */
struct phy_ac_rxgcrs_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_rxgcrs_info_t *cmn_info;
	acphy_desense_values_t *lte_desense;
	acphy_fem_rxgains_t fem_rxgains[PHY_CORE_MAX];
	// acphy_rxgainctrl_t rxgainctrl_params[PHY_CORE_MAX];
	phy_ac_rxg_params_t *rxg_params;
	int16	rxgainctrl_maxout_gains[ACPHY_MAX_RX_GAIN_STAGES];
	uint16	clip1_th, edcrs_en;
	uint16	initGain_codeA;
	uint16	initGain_codeB;
	uint8	phy_crs_th_from_crs_cal;
#ifndef BCM7271 // Remove when porting to Trunk
	uint8	rxgainctrl_stage_len[ACPHY_MAX_RX_GAIN_STAGES];
#endif // BCM7271
	int8	phy_noise_cache_crsmin[PHY_SIZE_NOISE_CACHE_ARRAY][PHY_CORE_MAX];
	int8	rx_elna_bypass_gain_th[PHY_CORE_MAX];
	/* Asymmetric AWGN noise jammer fix */
	uint8	srom_asymmetricjammermod;
	int8	lna2_complete_gaintbl[12];
	uint8	crsmincal_run;
	/* Parameter to enable subband_cust in 43012A0 */
	int8	enable_subband_cust;
	bool	thresh_noise_cal; /* enable/disable additional entries in noise cal table */
};

static const char BCMATTACHDATA(rstr_subband_cust)[]            = "subband_cust";
static const char BCMATTACHDATA(rstr_ssagc_en)[]                = "ssagc_en";
static const char BCMATTACHDATA(rstr_rxgains2gelnagainaD)[]     = "rxgains2gelnagaina%d";
static const char BCMATTACHDATA(rstr_rxgains2gtrelnabypaD)[]    = "rxgains2gtrelnabypa%d";
static const char BCMATTACHDATA(rstr_rxgains2gtrisoaD)[]        = "rxgains2gtrisoa%d";
static const char BCMATTACHDATA(rstr_rxgains5gelnagainaD)[]     = "rxgains5gelnagaina%d";
static const char BCMATTACHDATA(rstr_rxgains5gtrelnabypaD)[]    = "rxgains5gtrelnabypa%d";
static const char BCMATTACHDATA(rstr_rxgains5gtrisoaD)[]        = "rxgains5gtrisoa%d";
static const char BCMATTACHDATA(rstr_rxgains5gmelnagainaD)[]    = "rxgains5gmelnagaina%d";
static const char BCMATTACHDATA(rstr_rxgains5gmtrelnabypaD)[]   = "rxgains5gmtrelnabypa%d";
static const char BCMATTACHDATA(rstr_rxgains5gmtrisoaD)[]       = "rxgains5gmtrisoa%d";
static const char BCMATTACHDATA(rstr_rxgains5ghelnagainaD)[]    = "rxgains5ghelnagaina%d";
static const char BCMATTACHDATA(rstr_rxgains5ghtrelnabypaD)[]   = "rxgains5ghtrelnabypa%d";
static const char BCMATTACHDATA(rstr_rxgains5ghtrisoaD)[]       = "rxgains5ghtrisoa%d";

static const uint8 tiny4365_g_lna_rout_map[] = {9, 9, 9, 9, 6, 0};
static const uint8 tiny4365_g_lna_gain_map[] = {2, 3, 4, 5, 5, 5};
static const uint8 tiny4365_a_lna_rout_map[] = {9, 9, 9, 9, 6, 0};
static const uint8 tiny4365_a_lna_gain_map[] = {4, 5, 6, 7, 7, 7};

/* local functions */
static void phy_ac_rxgcrs_std_params_attach(phy_ac_rxgcrs_info_t *info);
static void phy_ac_rxgcrs_set_locale(phy_type_rxgcrs_ctx_t *ctx, uint8 region_group);
static void phy_ac_upd_lna1_bypass(phy_info_t *pi, uint8 core);
static void BCMATTACHFN(wlc_phy_srom_read_gainctrl_acphy)(phy_ac_rxgcrs_info_t *rxgcrs_info);
static int BCMATTACHFN(phy_ac_populate_rxg_params)(phy_ac_rxgcrs_info_t *rxgcrs_info);
static void phy_ac_set_ssagc_RSSI_detectors(phy_info_t *pi, uint8 core, uint8 *thresholds);
static void phy_ac_set_20696_RSSI_detectors(phy_info_t *pi, uint8 core, uint8 *thresholds);
static void* phy_ac_get_rxg_param_tbl(phy_info_t *pi, phy_ac_rxg_param_tbl_t tbl_id,
		uint8 ind_2d_tbl);

static void wlc_phy_mclip_agc(phy_info_t *pi, bool init, bool band_change, bool bw_change);
static void wlc_phy_mclip_agc_rssi_setup(phy_info_t *pi, bool init, bool band_change,
		bool bw_change,	int8 mclip_rssi[][9]);
static void wlc_phy_mclip_agc_rxgainctrl(phy_info_t *pi, int8 mclip_rssi[][9]);
static void wlc_phy_mclip_agc_clipcnt_thresh(phy_info_t *pi, uint8 core,
		const uint8 thresh_list[]);
static uint8 wlc_phy_mclip_agc_calc_lna_bypass_rssi(phy_info_t *pi, int8 *mclip_rssi,
		int8 sens_lna1Byp_lo, int8 sens_hi);
static uint32 wlc_phy_mclip_agc_pack_gains(phy_info_t *pi, uint8 *gain_indx, uint8 trtx,
		uint8 lna1byp);
static void wlc_phy_update_curr_desense_acphy(phy_info_t *pi);

// Macro's defined in Trunk but not for Eagle
#ifdef BCM7271
#define CCT_INIT(pi_ac) (pi_ac)->init
#define CCT_BAND_CHG(pi_ac) TRUE	// TODO: fix
#define CCT_BW_CHG(pi_ac) TRUE		// TODO: fix
#endif // BCM7271

/* register phy type specific implementation */
phy_ac_rxgcrs_info_t *
BCMATTACHFN(phy_ac_rxgcrs_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_rxgcrs_info_t *cmn_info)
{
	phy_ac_rxgcrs_info_t *rxgcrs_info;
	acphy_desense_values_t *lte_desense = NULL;
	phy_type_rxgcrs_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((rxgcrs_info = phy_malloc(pi, sizeof(phy_ac_rxgcrs_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	if ((lte_desense = phy_malloc(pi, sizeof(acphy_desense_values_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc lte_desense failed\n", __FUNCTION__));
		goto fail;
	}

	rxgcrs_info->pi = pi;
	rxgcrs_info->aci = aci;
	rxgcrs_info->cmn_info = cmn_info;
	rxgcrs_info->lte_desense = lte_desense;
	rxgcrs_info->thresh_noise_cal = (bool)PHY_GETINTVAR_DEFAULT(pi, rstr_thresh_noise_cal, 1);

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = rxgcrs_info;
	fns.set_locale = phy_ac_rxgcrs_set_locale;
	/* Read rxgainctrl srom entries - elna gain, trloss */
	wlc_phy_srom_read_gainctrl_acphy(rxgcrs_info);

	if (phy_ac_populate_rxg_params(rxgcrs_info) != BCME_OK) {
		goto fail;
	}

	phy_ac_rxgcrs_std_params_attach(rxgcrs_info);

	if (phy_rxgcrs_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_rxgcrs_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return rxgcrs_info;

	/* error handling */
fail:
	if (lte_desense != NULL)
		phy_mfree(pi, lte_desense, sizeof(acphy_desense_values_t));
	if (rxgcrs_info != NULL)
		phy_mfree(pi, rxgcrs_info, sizeof(phy_ac_rxgcrs_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_rxgcrs_unregister_impl)(phy_ac_rxgcrs_info_t *ac_info)
{
	phy_info_t *pi = ac_info->pi;
	phy_rxgcrs_info_t *cmn_info = ac_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_rxgcrs_unregister_impl(cmn_info);

	phy_mfree(pi, ac_info->lte_desense, sizeof(acphy_desense_values_t));

	if (ac_info->rxg_params) {
		phy_mfree(pi, ac_info->rxg_params, sizeof(phy_ac_rxg_params_t));
	}

	phy_mfree(pi, ac_info, sizeof(phy_ac_rxgcrs_info_t));
}

static void
BCMATTACHFN(wlc_phy_srom_read_gainctrl_acphy)(phy_ac_rxgcrs_info_t *rxgcrs_info)
{
	phy_info_t *pi = rxgcrs_info->pi;
	uint8 core, srom_rx, ant;
	char srom_name[30];
	phy_info_acphy_t *pi_ac;
	uint8 raw_elna, raw_trloss, raw_bypass;

	pi_ac = rxgcrs_info->aci;
	BCM_REFERENCE(pi_ac);

#ifndef BOARD_FLAGS
	BF_ELNA_2G(pi_ac) = (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardflags) &
	                              BFL_SROM11_EXTLNA) != 0;
	BF_ELNA_5G(pi_ac) = (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardflags) &
	                              BFL_SROM11_EXTLNA_5GHz) != 0;
#endif /* BOARD_FLAGS */

	FOREACH_CORE(pi, core) {
		ant = phy_get_rsdbbrd_corenum(pi, core);
		pi_ac->sromi->femrx_2g[ant].elna = 0;
		pi_ac->sromi->femrx_2g[ant].trloss = 0;
		pi_ac->sromi->femrx_2g[ant].elna_bypass_tr = 0;

		pi_ac->sromi->femrx_5g[ant].elna = 0;
		pi_ac->sromi->femrx_5g[ant].trloss = 0;
		pi_ac->sromi->femrx_5g[ant].elna_bypass_tr = 0;

		pi_ac->sromi->femrx_5gm[ant].elna = 0;
		pi_ac->sromi->femrx_5gm[ant].trloss = 0;
		pi_ac->sromi->femrx_5gm[ant].elna_bypass_tr = 0;

		pi_ac->sromi->femrx_5gh[ant].elna = 0;
		pi_ac->sromi->femrx_5gh[ant].trloss = 0;
		pi_ac->sromi->femrx_5gh[ant].elna_bypass_tr = 0;

		/*  -------  2G -------  */
		if (BF_ELNA_2G(pi_ac)) {
			snprintf(srom_name, sizeof(srom_name),  rstr_rxgains2gelnagainaD, ant);
			if (PHY_GETVAR(pi, srom_name) != NULL) {
				srom_rx = (uint8)PHY_GETINTVAR(pi, srom_name);
				pi_ac->sromi->femrx_2g[ant].elna = (2 * srom_rx) + 6;
			}

			snprintf(srom_name, sizeof(srom_name),  rstr_rxgains2gtrelnabypaD, ant);
			if (PHY_GETVAR(pi, srom_name) != NULL) {
				pi_ac->sromi->femrx_2g[ant].elna_bypass_tr =
				        (uint8)PHY_GETINTVAR(pi, srom_name);
			}
		}

		snprintf(srom_name, sizeof(srom_name),  rstr_rxgains2gtrisoaD, ant);
		if (PHY_GETVAR(pi, srom_name) != NULL) {
			srom_rx = (uint8)PHY_GETINTVAR(pi, srom_name);
			pi_ac->sromi->femrx_2g[ant].trloss = (2 * srom_rx) + 8;
		}


		/*  -------  5G -------  */
		if (BF_ELNA_5G(pi_ac)) {
			snprintf(srom_name, sizeof(srom_name),  rstr_rxgains5gelnagainaD, ant);
			if (PHY_GETVAR(pi, srom_name) != NULL) {
				srom_rx = (uint8)PHY_GETINTVAR(pi, srom_name);
				pi_ac->sromi->femrx_5g[ant].elna = (2 * srom_rx) + 6;
			}

			snprintf(srom_name, sizeof(srom_name),  rstr_rxgains5gtrelnabypaD, ant);
			if (PHY_GETVAR(pi, srom_name) != NULL) {
				pi_ac->sromi->femrx_5g[ant].elna_bypass_tr =
				        (uint8)PHY_GETINTVAR(pi, srom_name);
			}
		}

		snprintf(srom_name, sizeof(srom_name),  rstr_rxgains5gtrisoaD, ant);
		if (PHY_GETVAR(pi, srom_name) != NULL) {
			srom_rx = (uint8)PHY_GETINTVAR(pi, srom_name);
			pi_ac->sromi->femrx_5g[ant].trloss = (2 * srom_rx) + 8;
		}

		/*  -------  5G (mid) -------  */
		raw_elna = 0; raw_trloss = 0; raw_bypass = 0;
		if (BF_ELNA_5G(pi_ac)) {
			snprintf(srom_name, sizeof(srom_name),  rstr_rxgains5gmelnagainaD, ant);
			if (PHY_GETVAR(pi, srom_name) != NULL)
				raw_elna = (uint8)PHY_GETINTVAR(pi, srom_name);

			snprintf(srom_name, sizeof(srom_name),  rstr_rxgains5gmtrelnabypaD, ant);
			if (PHY_GETVAR(pi, srom_name) != NULL)
				raw_bypass = (uint8)PHY_GETINTVAR(pi, srom_name);
		}
		snprintf(srom_name, sizeof(srom_name),  rstr_rxgains5gmtrisoaD, ant);
		if (PHY_GETVAR(pi, srom_name) != NULL)
			raw_trloss = (uint8)PHY_GETINTVAR(pi, srom_name);

		if (((raw_elna == 0) && (raw_trloss == 0) && (raw_bypass == 0)) ||
		    ((raw_elna == 7) && (raw_trloss == 0xf) && (raw_bypass == 1))) {
			/* No entry in SROM, use generic 5g ones */
			pi_ac->sromi->femrx_5gm[ant].elna = pi_ac->sromi->femrx_5g[ant].elna;
			pi_ac->sromi->femrx_5gm[ant].elna_bypass_tr =
			        pi_ac->sromi->femrx_5g[ant].elna_bypass_tr;
			pi_ac->sromi->femrx_5gm[ant].trloss = pi_ac->sromi->femrx_5g[ant].trloss;
		} else {
			if (BF_ELNA_5G(pi_ac)) {
				pi_ac->sromi->femrx_5gm[ant].elna = (2 * raw_elna) + 6;
				pi_ac->sromi->femrx_5gm[ant].elna_bypass_tr = raw_bypass;
			}
			pi_ac->sromi->femrx_5gm[ant].trloss = (2 * raw_trloss) + 8;
		}

		/*  -------  5G (high) -------  */
		raw_elna = 0; raw_trloss = 0; raw_bypass = 0;
		if (BF_ELNA_5G(pi_ac)) {
			snprintf(srom_name, sizeof(srom_name),  rstr_rxgains5ghelnagainaD, ant);
			if (PHY_GETVAR(pi, srom_name) != NULL)
				raw_elna = (uint8)PHY_GETINTVAR(pi, srom_name);

			snprintf(srom_name, sizeof(srom_name),  rstr_rxgains5ghtrelnabypaD, ant);
			if (PHY_GETVAR(pi, srom_name) != NULL)
				raw_bypass = (uint8)PHY_GETINTVAR(pi, srom_name);
		}
		snprintf(srom_name, sizeof(srom_name),  rstr_rxgains5ghtrisoaD, ant);
		if (PHY_GETVAR(pi, srom_name) != NULL)
			raw_trloss = (uint8)PHY_GETINTVAR(pi, srom_name);

		if (((raw_elna == 0) && (raw_trloss == 0) && (raw_bypass == 0)) ||
		    ((raw_elna == 7) && (raw_trloss == 0xf) && (raw_bypass == 1))) {
			/* No entry in SROM, use generic 5g ones */
			pi_ac->sromi->femrx_5gh[ant].elna = pi_ac->sromi->femrx_5gm[ant].elna;
			pi_ac->sromi->femrx_5gh[ant].elna_bypass_tr =
			        pi_ac->sromi->femrx_5gm[ant].elna_bypass_tr;
			pi_ac->sromi->femrx_5gh[ant].trloss = pi_ac->sromi->femrx_5gm[ant].trloss;
		} else {
			if (BF_ELNA_5G(pi_ac)) {
				pi_ac->sromi->femrx_5gh[ant].elna = (2 * raw_elna) + 6;
				pi_ac->sromi->femrx_5gh[ant].elna_bypass_tr = raw_bypass;
			}
			pi_ac->sromi->femrx_5gh[ant].trloss = (2 * raw_trloss) + 8;

		}
	}

}

static int
BCMATTACHFN(phy_ac_populate_rxg_params)(phy_ac_rxgcrs_info_t *rxgcrs_info)
{
	phy_ac_rxg_params_t *rxg_params;
	phy_info_t *pi;
	uint8 i;
	uint8 *lna1_rout_map_2g, *lna1_rout_map_5g;
	uint8 *lna1_gain_map_2g, *lna1_gain_map_5g;

	pi = rxgcrs_info->pi;

	if ((rxg_params = phy_malloc(pi, sizeof(phy_ac_rxg_params_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	if (IS_28NM_RADIO(pi)) {
		/* Copy LNA12 gain params to structure element */
		for (i = 0; i < 2; i++) {
			memcpy(rxg_params->lna12_gain_2g_tbl[i],
					phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_2G, i),
					sizeof(int8) * N_LNA12_GAINS);
			memcpy(rxg_params->lna12_gainbits_2g_tbl[i],
					phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_BITS_TBL_2G, i),
					sizeof(int8) * N_LNA12_GAINS);

			/* If A band supported */
			if (PHY_BAND5G_ENAB(pi)) {
				memcpy(rxg_params->lna12_gain_5g_tbl[i],
						phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_5G, i),
						sizeof(int8) * N_LNA12_GAINS);
				memcpy(rxg_params->lna12_gainbits_5g_tbl[i],
					phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_BITS_TBL_5G, i),
					sizeof(int8) * N_LNA12_GAINS);
			}
		}

		/* Copy gain limit table */
		for (i = 0; i < RXGAIN_CONF_ELEMENTS; i++) {
			memcpy(rxg_params->gainlimit_tbl[i],
					phy_ac_get_rxg_param_tbl(pi, GAIN_LIMIT_TBL, i),
					sizeof(int8) * MAX_RX_GAINS_PER_ELEM);
		}

		/* LNA1 Rout table (2G) */
		lna1_rout_map_2g = phy_ac_get_rxg_param_tbl(pi, LNA1_ROUTMAP_TBL_2G, 0);
		lna1_gain_map_2g = phy_ac_get_rxg_param_tbl(pi, LNA1_GAINMAP_TBL_2G, 0);
		for (i = 0; i < N_LNA12_GAINS; i++) {
			rxg_params->lna1Rout_2g_tbl[i] = (lna1_rout_map_2g[i] << 3) |
					lna1_gain_map_2g[i];
		}

		/* LNA1 Rout table (5G), if A band supported */
		if (PHY_BAND5G_ENAB(pi)) {
			lna1_rout_map_5g = phy_ac_get_rxg_param_tbl(pi, LNA1_ROUTMAP_TBL_5G, 0);
			lna1_gain_map_5g = phy_ac_get_rxg_param_tbl(pi, LNA1_GAINMAP_TBL_5G, 0);

			for (i = 0; i < N_LNA12_GAINS; i++) {
				rxg_params->lna1Rout_5g_tbl[i] = (lna1_rout_map_5g[i] << 3) |
					lna1_gain_map_5g[i];
			}
		}

		/* TIA Gain and GainBits table */
		memcpy(rxg_params->tia_gain_tbl,
				phy_ac_get_rxg_param_tbl(pi, TIA_GAIN_TBL, 0),
				sizeof(int8) * N_TIA_GAINS);
		memcpy(rxg_params->tia_gainbits_tbl,
				phy_ac_get_rxg_param_tbl(pi, TIA_GAIN_BITS_TBL, 0),
				sizeof(int8) * N_TIA_GAINS);

		/* Copy BIQ01 gain params to structure element */
		for (i = 0; i < 2; i++) {
			memcpy(rxg_params->biq01_gain_tbl[i],
				phy_ac_get_rxg_param_tbl(pi, BIQ01_GAIN_TBL, i),
				sizeof(int8) * N_BIQ01_GAINS);
			memcpy(rxg_params->biq01_gainbits_tbl[i],
				phy_ac_get_rxg_param_tbl(pi, BIQ01_GAIN_BITS_TBL, i),
				sizeof(int8) * N_BIQ01_GAINS);
		}

	}

	if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
		/* LNA2 Gm table */
		memcpy(rxg_params->lna2_gm_ind_2g_tbl,
				phy_ac_get_rxg_param_tbl(pi, LNA2_GAINMAP_TBL_2G, 0),
				sizeof(int8) * N_LNA12_GAINS);

		if (PHY_BAND5G_ENAB(pi)) {
			memcpy(rxg_params->lna2_gm_ind_5g_tbl,
				phy_ac_get_rxg_param_tbl(pi, LNA2_GAINMAP_TBL_5G, 0),
				sizeof(int8) * N_LNA12_GAINS);
		}

		/* Max gain table */
		memcpy(rxg_params->maxgain, phy_ac_get_rxg_param_tbl(pi, MAX_GAIN_TBL, 0),
				sizeof(int16) * ACPHY_MAX_RX_GAIN_STAGES);

		/* Fast AGC clip gains */
		memcpy(rxg_params->fast_agc_clip_gains,
				phy_ac_get_rxg_param_tbl(pi, FAST_AGC_CLIP_GAIN_TBL, 0),
				sizeof(int8) * 4);

		rxg_params->lna1_min_indx = 0;
		rxg_params->lna1_max_indx = ACPHY_MAX_LNA1_IDX;
		rxg_params->lna2_min_indx = 1;
		rxg_params->lna2_max_indx = ACPHY_20695_MAX_LNA2_IDX;
		rxg_params->lna2_2g_min_indx = 1;
		rxg_params->lna2_5g_min_indx = 2;
		rxg_params->lna2_2g_max_indx = ACPHY_20695_MAX_LNA2_IDX;
		rxg_params->lna2_5g_max_indx = 3;
		rxg_params->lna1_byp_indx = 6;
		rxg_params->farrow_shift = 2;
		rxg_params->tia_max_indx = 9;
		rxg_params->max_analog_gain = MAX_ANALOG_RX_GAIN_28NM_ULP;
		rxg_params->lna1_byp_gain_2g = LNA1_BYPASS_GAIN_2G_20695;
		rxg_params->lna1_byp_gain_5g = LNA1_BYPASS_GAIN_5G_20695;
		rxg_params->lna1_byp_rout_2g = LNA1_BYPASS_ROUT_2G_20695;
		rxg_params->lna1_byp_rout_5g = LNA1_BYPASS_ROUT_5G_20695;
		rxg_params->lna1_sat_2g = -12;
		rxg_params->lna1_sat_5g = -12;
		rxg_params->lna1_sat_2g_elna = -12;
		rxg_params->lna1_sat_5g_elna = -8;
		rxg_params->high_sen_adjust_2g = 14;
		rxg_params->high_sen_adjust_5g = 10;

		/* Single shot specific configs */
		memcpy(rxg_params->ssagc_clip_gains,
				phy_ac_get_rxg_param_tbl(pi, SSAGC_CLIP_GAIN_TBL, 0),
				SSAGC_CLIP1_GAINS_CNT * sizeof(int8));
		memcpy(rxg_params->ssagc_lna1byp_flag,
				phy_ac_get_rxg_param_tbl(pi, SSAGC_LNA1BYP_TBL, 0),
				SSAGC_CLIP1_GAINS_CNT * sizeof(bool));
		memcpy(rxg_params->ssagc_clip2_tbl,
				phy_ac_get_rxg_param_tbl(pi, SSAGC_CLIP2_TBL, 0),
				SSAGC_CLIP2_TBL_IDX_CNT * sizeof(uint8));
		memcpy(rxg_params->ssagc_rssi_thresh_2g,
				phy_ac_get_rxg_param_tbl(pi, SSAGC_RSSI_THRESH_TBL_2G, 0),
				SSAGC_N_RSSI_THRESH * sizeof(uint8));
		memcpy(rxg_params->ssagc_clipcnt_thresh_2g,
				phy_ac_get_rxg_param_tbl(pi, SSAGC_CLIPCNT_THRESH_TBL_2G, 0),
				SSAGC_N_CLIPCNT_THRESH * sizeof(uint8));

		if (PHY_BAND5G_ENAB(pi)) {
			memcpy(rxg_params->ssagc_rssi_thresh_5g,
				phy_ac_get_rxg_param_tbl(pi, SSAGC_RSSI_THRESH_TBL_5G, 0),
				SSAGC_N_RSSI_THRESH * sizeof(uint8));
			memcpy(rxg_params->ssagc_clipcnt_thresh_5g,
				phy_ac_get_rxg_param_tbl(pi, SSAGC_CLIPCNT_THRESH_TBL_5G, 0),
				SSAGC_N_CLIPCNT_THRESH * sizeof(uint8));
		}

		rxg_params->ssagc_lna_byp_gain_thresh = 15;

		/* Low noise figure ELNA mode */
		memcpy(rxg_params->fast_agc_lonf_clip_gains,
				phy_ac_get_rxg_param_tbl(pi, FAST_AGC_LONF_CLIP_GAIN_TBL, 0),
				sizeof(int8) * 4);
		rxg_params->lna2_lonf_force_indx_2g = 1;
		rxg_params->lna2_lonf_force_indx_5g = 1;
		rxg_params->lna1_lonf_max_indx = 5;
		rxg_params->max_analog_gain_lonf = MAX_ANALOG_RX_GAIN_28NM_ULP_LONF;
		rxg_params->high_sen_adjust_lonf_2g = 14;
		rxg_params->high_sen_adjust_lonf_5g = 14;
	} else if (ACMAJORREV_25_40(pi->pubpi.phy_rev) ||
	           (ACMAJORREV_33(pi->pubpi.phy_rev) && ACMINORREV_4(pi))) {
		/*
		if (CHSPEC_IS2G(pi->radio_chanspec) ? BF_ELNA_2G(pi->u.pi_acphy)
				: BF_ELNA_5G(pi->u.pi_acphy))
			rxg_params->lna1_max_indx = 5;
		else
			rxg_params->lna1_max_indx = 5;
		*/
		rxg_params->lna1_max_indx = 5;

		rxg_params->lna1_byp_gain_2g = -12;
		rxg_params->lna1_byp_gain_5g = -5;
		rxg_params->lna1_byp_rout_2g = 10;
		rxg_params->lna1_byp_rout_5g = 8;
		rxg_params->lna1_byp_indx = 6;

		rxg_params->lna1_min_indx = 0;
		rxg_params->lna2_min_indx = 3;
		rxg_params->lna2_max_indx = 3;
		rxg_params->lna2_2g_min_indx = 3;
		rxg_params->lna2_2g_max_indx = 3;
		rxg_params->lna2_5g_min_indx = 3;
		rxg_params->lna2_5g_max_indx = 3;
		/* mclip agc is default to on for 4347 */
		rxg_params->mclip_agc_en = (bool)PHY_GETINTVAR_DEFAULT(pi, "mclip_agc_en", 1);
	}

	/* SSAGC enable control */
	rxg_params->ssagc_en = (bool)PHY_GETINTVAR_DEFAULT(pi, rstr_ssagc_en, 0);

	/* setup ptr */
	rxgcrs_info->rxg_params = rxg_params;

	return (BCME_OK);

fail:
	if (rxg_params != NULL) {
		phy_mfree(pi, rxg_params, sizeof(phy_ac_rxg_params_t));
	}

	return (BCME_NOMEM);
}

static void
BCMATTACHFN(phy_ac_rxgcrs_std_params_attach)(phy_ac_rxgcrs_info_t *ri)
{
	uint8 gain_len[] = {2, 6, 7, 10, 8, 8, 11}; /* elna, lna1, lna2, mix, bq0, bq1, dvga */
	uint8 i, core_num;
	phy_info_t *pi = ri->pi;

	if (TINY_RADIO(pi)) {
		gain_len[3] = 12; /* tia */
		gain_len[5] = 3;  /* farrow */
	} else if (ACMAJORREV_32(pi->pubpi.phy_rev)) {
		gain_len[TIA_ID] = 12;
		gain_len[BQ1_ID] = 3;
		gain_len[DVGA_ID] = 15;
	} else if (IS_28NM_RADIO(pi)) {
		if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
			gain_len[2] = 4; /* lna2 */
			gain_len[3] = 12; /* tia */
			gain_len[4] = 6; /* BIQ0 */
			gain_len[5] = 6; /* BIQ1 - Not present */
		} else {
			gain_len[2] = 4;
			gain_len[3] = 16;
			gain_len[4] = 6;
			gain_len[5] = 3; /* BIQ1 - Not present */
		}
	}
	for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++) {
#ifdef BCM7271 // Remove when porting to Trunk
		pi->u.pi_acphy->rxgainctrl_stage_len[i] = gain_len[i];
#else
		ri->rxgainctrl_stage_len[i] = gain_len[i];
#endif
	}

	ri->aci->crsmincal_enable = TRUE;
	ri->aci->force_crsmincal  = FALSE;
	ri->crsmincal_run = 0;
	ri->phy_crs_th_from_crs_cal = ACPHY_CRSMIN_DEFAULT;
	/* default clip1_th & edcrs_en */
	ri->clip1_th = 0x404e;
	ri->edcrs_en = 0xfff;
	bzero((uint8 *)ri->aci->phy_noise_pwr_array, sizeof(ri->aci->phy_noise_pwr_array));
	bzero((uint8 *)ri->aci->phy_noise_in_crs_min, sizeof(ri->aci->phy_noise_in_crs_min));
	ri->aci->phy_debug_crscal_counter = 0;
	ri->aci->phy_debug_crscal_channel = 0;
	ri->aci->phy_noise_counter = 0;

	if (ACMAJORREV_0(pi->pubpi.phy_rev)) {
		uint8 subband_num;
		for (subband_num = 0; subband_num <= 4; subband_num++)
			FOREACH_CORE(pi, core_num)
				ri->phy_noise_cache_crsmin[subband_num][core_num] = -30;
	} else {
		FOREACH_CORE(pi, core_num) {
			ri->phy_noise_cache_crsmin[0][core_num] = -30;
			if (ACMAJORREV_2(pi->pubpi.phy_rev) ||
			(!(BF_ELNA_5G(ri->aci)) && ACMAJORREV_4(pi->pubpi.phy_rev))) {
				ri->phy_noise_cache_crsmin[1][core_num] = -32;
				ri->phy_noise_cache_crsmin[2][core_num] = -32;
				ri->phy_noise_cache_crsmin[3][core_num] = -31;
				ri->phy_noise_cache_crsmin[4][core_num] = -31;
			} else {
				ri->phy_noise_cache_crsmin[1][core_num] = -28;
				ri->phy_noise_cache_crsmin[2][core_num] = -28;
				ri->phy_noise_cache_crsmin[3][core_num] = -26;
				ri->phy_noise_cache_crsmin[4][core_num] = -25;
			}
		}
	}
}

static void *
phy_ac_get_rxg_param_tbl(phy_info_t *pi, phy_ac_rxg_param_tbl_t tbl_id, uint8 ind_2d_tbl)
{
	if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
		return NULL;
	} else if (ACMAJORREV_25_40(pi->pubpi.phy_rev) ||
	           (ACMAJORREV_33(pi->pubpi.phy_rev) && ACMINORREV_4(pi))) {
		switch (tbl_id) {
			case LNA12_GAIN_TBL_2G:
				return (void *)lna12_gain_tbl_2g_maj40[ind_2d_tbl];

			case LNA12_GAIN_TBL_5G:
				return (void *)lna12_gain_tbl_5g_maj40[ind_2d_tbl];

			case LNA12_GAIN_BITS_TBL_2G:
				return (void *)lna12_gainbits_tbl_2g_maj40[ind_2d_tbl];

			case LNA12_GAIN_BITS_TBL_5G:
				return (void *)lna12_gainbits_tbl_5g_maj40[ind_2d_tbl];

			case TIA_GAIN_TBL:
				return (void *)tia_gain_tbl_maj40;

			case TIA_GAIN_BITS_TBL:
				return (void *)tia_gainbits_tbl_maj40;

			case BIQ01_GAIN_TBL:
				return (void *)biq01_gain_tbl_maj40[ind_2d_tbl];

			case BIQ01_GAIN_BITS_TBL:
				return (void *)biq01_gainbits_tbl_maj40[ind_2d_tbl];

			case GAIN_LIMIT_TBL:
				return (void *)gainlimit_tbl_maj40[ind_2d_tbl];

			case LNA1_ROUTMAP_TBL_2G:
				return (void *)lna1_rout_map_2g_maj40;

			case LNA1_ROUTMAP_TBL_5G:
				return (void *)lna1_rout_map_5g_maj40;

			case LNA1_GAINMAP_TBL_2G:
				return (void *)lna1_gain_map_2g_maj40;

			case LNA1_GAINMAP_TBL_5G:
				return (void *)lna1_gain_map_5g_maj40;

			default:
				return NULL;
		}
	} else {
		return NULL;
	}
}

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */
static const uint8 ac_lna1_2g[]       = {0xf6, 0xff, 0x6, 0xc, 0x12, 0x19};
static const uint8 ac_lna1_2g_tiny[]	= {0xfa, 0x00, 0x6, 0xc, 0x12, 0x18};
static const uint8 ac_tiny_g_lna_rout_map[] = {9, 9, 9, 9, 6, 0};
static const uint8 ac_tiny_g_lna_gain_map[] = {2, 3, 4, 5, 5, 5};
static const uint8 ac_tiny_a_lna_rout_map[] = {11, 11, 11, 11, 7, 0};
static const uint8 ac_tiny_a_lna_gain_map[] = {4, 5, 6, 7, 7, 7};
static const uint8 ac_4365_a_lna_rout_map[] = {9, 9, 9, 9, 6, 0};
static const uint8 ac_lna1_2g_ltecoex[]       = {0xf6, 0xff, 0x6, 0xc, 0x12, 0x16};
static const uint8 ac_lna1_5g_tiny[]	= {0xfb, 0x00, 0x6, 0xc, 0x12, 0x18};
static const uint8 ac_lna1_5g_4365[]	= {0xfe, 0x03, 0x9, 0x10, 0x14, 0x18};
static const uint8 ac_lna1_5g[] = {0xf9, 0xfe, 0x4, 0xa, 0x10, 0x17};
static const uint8 ac_lna1_2g_43352_ilna[]    = {0xff, 0xff, 0x6, 0xc, 0x12, 0x19};
static const uint8 ac_lna2_2g_ltecoex[] = {0xf4, 0xf8, 0xfc, 0xff, 0xff, 0x5, 0x9};
static const uint8 ac_lna2_2g_43352_ilna[] = {0xfa, 0xfa, 0xfe, 0x01, 0x4, 0x7, 0xb};
static const uint8 ac_lna2_5g[] = {0xf5, 0xf8, 0xfb, 0xfe, 0x2, 0x5, 0x9};
static const uint8 ac_lna2_2g_gm2[] = {0xf4, 0xf8, 0xfc, 0xff, 0x2, 0x5, 0x9};
static const uint8 ac_lna2_2g_gm3[] = {0xf6, 0xfa, 0xfe, 0x01, 0x4, 0x7, 0xb};
static const uint8 ac_lna2_tiny[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
static const uint8 ac_lna2_tiny_4349[] = {0xEE, 0xf4, 0xfa, 0x0, 0x0, 0x0, 0x0};
static const uint8 ac_lna2_tiny_4365_2g[] = {0xf4, 0xfa, 0, 6, 6, 6, 6};
static const uint8 ac_lna2_tiny_4365_5g[] = {0xf1, 0xf7, 0xfd, 3, 3, 3, 3};
static const uint8 ac_lna2_tiny_ilna_dcc_comp[] = {0, 20, 16, 12, 8, 4, 0};
static const uint8 ac_lna1_rout_delta_2g[] = {0, 1, 2, 3, 5, 7, 8, 11, 13, 15, 18, 20};
static const uint8 ac_lna1_rout_delta_5g[] = {10, 7, 4, 2, 0};

#ifndef WLC_DISABLE_ACI
static void wlc_phy_desense_mf_high_thresh_acphy(phy_info_t *pi, bool on);
static void wlc_phy_desense_print_phyregs_acphy(phy_info_t *pi, const char str[]);
#endif

static int8 wlc_phy_rxgainctrl_calc_low_sens_acphy(phy_info_t *pi, int8 clipgain,
                                                   bool trtx, bool lna1byp, uint8 core);
static void wlc_phy_set_analog_rxgain(phy_info_t *pi, uint8 clipgain,
                                      uint8 *gain_idx, bool trtx, bool lna1byp, uint8 core);
static int8 wlc_phy_rxgainctrl_calc_high_sens_acphy(phy_info_t *pi, int8 clipgain,
                                                    bool trtx, bool lna1byp, uint8 core);
static uint8 wlc_phy_rxgainctrl_set_init_clip_gain_acphy(phy_info_t *pi, uint8 clipgain,
	int8 gain_dB, bool trtx, bool lna1byp, uint8 core);
static void wlc_phy_rxgainctrl_nbclip_acphy(phy_info_t *pi, uint8 core,
	int8 rxpwr_dBm);
static void wlc_phy_rxgainctrl_w1clip_acphy(phy_info_t *pi, uint8 core,
	int8 rxpwr_dBm);
static void wlc_phy_rxgainctrl_w1clip_acphy_28nm_ulp(phy_info_t *pi, uint8 core,
	int8 rxpwr_dBm);
#ifndef WLC_DISABLE_ACI
static void wlc_phy_set_crs_min_pwr_higain_acphy(phy_info_t *pi, uint8 thresh);
#endif
static void wlc_phy_limit_rxgaintbl_acphy(uint8 gaintbl[], uint8 gainbitstbl[], uint8 sz,
	const uint8 default_gaintbl[], uint8 min_idx, uint8 max_idx);
static void wlc_phy_rxgainctrl_nbclip_acphy_tiny(phy_info_t *pi, uint8 core, int8 rxpwr_dBm);
static void wlc_phy_rxgainctrl_nbclip_acphy_28nm_ulp(phy_info_t *pi, uint8 core, int8 rxpwr_dBm);
#define ACPHY_NUM_NB_THRESH 8
#define ACPHY_NUM_NB_THRESH_TINY 9
#define ACPHY_NUM_W1_THRESH 12

#define NB_LOW 0
#define NB_MID 1
#define NB_HIGH 2
#ifndef WLC_DISABLE_ACI

/*
Default - High MF thresholds are used only if pktgain < 81dB.
To always use high mf thresholds, change this to 98dBs
*/
static void
wlc_phy_desense_mf_high_thresh_acphy(phy_info_t *pi, bool on)
{
	uint16 val;

	if (on) {
		val = 0x5f62;
	} else {
		val = (TINY_RADIO(pi)) ? 0x454b : 0x4e51;
	}

	if (ACREV_GE(pi->pubpi.phy_rev, 32)) {
		/* Use default values for now */
	} else {
		WRITE_PHYREG(pi, crshighlowpowThresholdl, val);
		WRITE_PHYREG(pi, crshighlowpowThresholdu, val);
		WRITE_PHYREG(pi, crshighlowpowThresholdlSub1, val);
		WRITE_PHYREG(pi, crshighlowpowThresholduSub1, val);
	}
}

static void
wlc_phy_desense_print_phyregs_acphy(phy_info_t *pi, const char str[])
{
}

#endif /* #ifndef WLC_DISABLE_ACI */

static int8
wlc_phy_rxgainctrl_calc_low_sens_acphy(phy_info_t *pi, int8 clipgain, bool trtx,
                                       bool lna1byp, uint8 core)
{
	int sens, sens_bw_c9[] = {-66, -63, -60};   /* c9s1 1% sensitivity for 20/40/80 mhz */
	int sens_bw_c11[]= {-63, -60, -57};   /* c11s1_ldpc 1% sensitivity for 20/40/80 mhz */
	uint8 low_sen_adjust[] = {25, 22, 19};   /* low_end_sens = -clip_gain - low_sen_adjust */
	uint8 bw_idx, elna_idx, trloss, elna_bypass_tr;
	int8 elna, detection, demod;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	acphy_desense_values_t *desense = pi_ac->total_desense;
	uint8 idx, max_lna2;
	int8 extra_loss = 0;

	ASSERT(core < PHY_CORE_MAX);
	bw_idx = CHSPEC_IS20(pi->radio_chanspec) ? 0 : CHSPEC_IS40(pi->radio_chanspec) ? 1 : 2;
	if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev))
		sens =  sens_bw_c11[bw_idx];
	else
		sens =  sens_bw_c9[bw_idx];

	if (lna1byp) {
		sens += 7;  /* djain - HACK, analyze this more */
	}

	elna_idx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
	elna = pi_ac->rxgainctrl_params[core].gaintbl[0][elna_idx];
	trloss = pi_ac->fem_rxgains[core].trloss;
	elna_bypass_tr = pi_ac->fem_rxgains[core].elna_bypass_tr;

	if (TINY_RADIO(pi))
		detection = -6 - (clipgain + low_sen_adjust[bw_idx]);
	else
		detection = 0 - (clipgain + low_sen_adjust[bw_idx]);

	demod = trtx ? (sens + trloss - (elna_bypass_tr * elna)) : sens;
	demod += desense->nf_hit_lna12;

	if ((ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) &&
	    TINY_RADIO(pi)) {
		/* sens is worse if lower lna2 gains are used */
		if (trtx) {
			idx = pi_ac->rxgainctrl_stage_len[LNA2_ID] - 1;
			max_lna2 = pi_ac->rxgainctrl_params[core].gainbitstbl[LNA2_ID][idx];
			extra_loss = 3 * (3 - max_lna2);
			demod += MAX(0, extra_loss);
		}
	}

	return MAX(detection, demod);
}

static void
wlc_phy_set_analog_rxgain(phy_info_t *pi, uint8 clipgain, uint8 *gain_idx, bool trtx,
                          bool lna1byp, uint8 core)
{
	uint8 lna1, lna2, mix, bq0, bq1, tx, rx, dvga;
	uint16 gaincodeA, gaincodeB, final_gain;

	ASSERT(core < PHY_CORE_MAX);

	lna1 = gain_idx[1];
	lna2 = gain_idx[2];
	mix = gain_idx[3];
	bq0 = (TINY_RADIO(pi)) ? 0 : gain_idx[4];
	bq1 = gain_idx[5];
	dvga = (TINY_RADIO(pi) || IS_28NM_RADIO(pi)) ? gain_idx[6] : 0;

	if (lna1byp) trtx = 0;  /* force in rx mode is using lna1byp */
	tx = trtx;
	rx = !trtx;

	gaincodeA = ((mix << 7) | (lna2 << 4) | (lna1 << 1));
	gaincodeB = (dvga<<12) | (bq1 << 8) | (bq0 << 4) | (tx << 3) | (rx << 2) | (lna1byp << 1);

	if (clipgain == 0) {
		WRITE_PHYREGC(pi, InitGainCodeA, core, gaincodeA);
		WRITE_PHYREGC(pi, InitGainCodeB, core, gaincodeB);
		final_gain = ((bq1 << 13) | (bq0 << 10) | (mix << 6) | (lna2 << 3) | (lna1 << 0));
		if (core == 3)
		    wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x509, 16,
		                          &final_gain);
		else
		    wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0xf9 + core), 16,
		                          &final_gain);
		if (TINY_RADIO(pi) || IS_28NM_RADIO(pi)) {
			uint8 gmrout;
			uint16 rfseq_init_aux;
			uint8 offset = ACPHY_LNAROUT_BAND_OFFSET(pi,
				pi->radio_chanspec)	+ lna1 +
				ACPHY_LNAROUT_CORE_RD_OFST(pi, core);

			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, offset, 8, &gmrout);

			if (ACMAJORREV_40(pi->pubpi.phy_rev)) {
				rfseq_init_aux = dvga;
			} else {
				rfseq_init_aux = (((0xf & (gmrout >> 3)) << 4) | dvga);
			}
			if (core == 3)
				wlc_phy_table_write_acphy(
					pi, ACPHY_TBL_ID_RFSEQ, 1, 0x506, 16,
					&rfseq_init_aux);
			else
				wlc_phy_table_write_acphy(
					pi, ACPHY_TBL_ID_RFSEQ, 1, (0xf6 + core),
					16, &rfseq_init_aux);
			/* elna index always zero, not required */
		}
	} else if (clipgain == 1) {
		WRITE_PHYREGC(pi, clipHiGainCodeA, core, gaincodeA);
		WRITE_PHYREGC(pi, clipHiGainCodeB, core, gaincodeB);
	} else if (clipgain == 2) {
		WRITE_PHYREGC(pi, clipmdGainCodeA, core, gaincodeA);
		WRITE_PHYREGC(pi, clipmdGainCodeB, core, gaincodeB);
	} else if (clipgain == 3) {
		WRITE_PHYREGC(pi, cliploGainCodeA, core, gaincodeA);
		WRITE_PHYREGC(pi, cliploGainCodeB, core, gaincodeB);
	} else if (clipgain == 4) {
		WRITE_PHYREGC(pi, clip2GainCodeA, core, gaincodeA);
		WRITE_PHYREGC(pi, clip2GainCodeB, core, gaincodeB);
	}

	if (lna1byp) {
		MOD_PHYREGC(pi, cliploGainCodeB, core, clip1loLna1Byp, 1);
		if (ACMAJORREV_4(pi->pubpi.phy_rev)) {
			WRITE_PHYREGC(pi, _lna1BypVals, core, 0xfa1);
			MOD_PHYREG(pi, AfePuCtrl, lna1_pd_during_byp, 1);
		}
	} else {
		MOD_PHYREGC(pi, cliploGainCodeB, core, clip1loLna1Byp, 0);
	}

}

static int8
wlc_phy_rxgainctrl_calc_high_sens_acphy(phy_info_t *pi, int8 clipgain, bool trtx,
                                        bool lna1byp, uint8 core)
{
	uint8 high_sen_adjust = 23;  /* high_end_sens = high_sen_adjust - clip_gain */
	uint8 elna_idx, trloss;
	int8 elna, saturation, clipped, lna1_sat;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	if (TINY_RADIO(pi)) {
		if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) {
			lna1_sat = CHSPEC_IS2G(pi->radio_chanspec) ? -12 : -10;
			high_sen_adjust = 20;
		} else {
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				lna1_sat = -10;
				high_sen_adjust = 17;
			} else {
				lna1_sat = -2;
				high_sen_adjust = 19;
			}
		}
	} else {
		lna1_sat = -16;
		high_sen_adjust = 23;
	}

	if (lna1byp)
		lna1_sat += CHSPEC_IS2G(pi->radio_chanspec) ? 10 : 7;

	ASSERT(core < PHY_CORE_MAX);
	elna_idx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
	elna = pi_ac->rxgainctrl_params[core].gaintbl[0][elna_idx];
	trloss = pi_ac->fem_rxgains[core].trloss;

	/* c9 needs lna1 input to be below -lna1_sat */
	saturation = trtx ? (trloss + lna1_sat - elna) : 0 + lna1_sat - elna;
	clipped = high_sen_adjust - clipgain;

	return MIN(saturation, clipped);
}

/* Wrapper to call encode_gain & set init/clip gains */
static uint8
wlc_phy_rxgainctrl_set_init_clip_gain_acphy(phy_info_t *pi, uint8 clipgain, int8 gain_dB,
	bool trtx, bool lna1byp, uint8 core)
{
	uint8 gain_idx[ACPHY_MAX_RX_GAIN_STAGES];
	uint8 gain_applied;

	gain_applied = wlc_phy_rxgainctrl_encode_gain_acphy(pi, clipgain, core, gain_dB, trtx,
	                                                    lna1byp, gain_idx);
	wlc_phy_set_analog_rxgain(pi, clipgain, gain_idx, trtx, lna1byp, core);

	return gain_applied;
}

static void
wlc_phy_max_anagain_to_initgain_lesi_acphy(phy_info_t *pi, uint8 i, uint8 core, uint8 gidx)
{
	uint8 l, offset = 0, gainbits[15];
	int8 gains[15];
	uint16 gainbits_tblid, gains_tblid;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint8 gainbit_tbl_entry_size = sizeof(pi_ac->rxgainctrl_params[0].gainbitstbl[0][0]);
	uint8 gain_tbl_entry_size = sizeof(pi_ac->rxgainctrl_params[0].gaintbl[0][0]);

	if (i == 1) {
		offset = 8;
	} else if (i == 2) {
		offset = 16;
	} else if (i == 3) {
		offset = 32;
	} else {
		ASSERT(0);
	}

	/* gainBits */
	for (l = 0; l < pi_ac->rxgainctrl_stage_len[i]; l++)
		gainbits[l] = (l > gidx) ? gidx : pi_ac->rxgainctrl_params[core].gainbitstbl[i][l];
	gainbits_tblid = (core == 0) ? ACPHY_TBL_ID_GAINBITS0 : (core == 1) ?
	        ACPHY_TBL_ID_GAINBITS1 : (core == 2) ?
	        ACPHY_TBL_ID_GAINBITS2 : ACPHY_TBL_ID_GAINBITS3;
	wlc_phy_table_write_acphy(
		pi, gainbits_tblid, pi_ac->rxgainctrl_stage_len[i],
		offset, ACPHY_GAINBITS_TBL_WIDTH, gainbits);
	memcpy(pi_ac->rxgainctrl_params[core].gainbitstbl[i], gainbits,
	       gainbit_tbl_entry_size * pi_ac->rxgainctrl_stage_len[i]);

	/* gaintbl */
	for (l = 0; l < pi_ac->rxgainctrl_stage_len[i]; l++)
		gains[l] = (l > gidx) ?
		        pi_ac->rxgainctrl_params[core].gaintbl[i][gidx] :
		        pi_ac->rxgainctrl_params[core].gaintbl[i][l];
	gains_tblid = (core == 0) ? ACPHY_TBL_ID_GAIN0 : (core == 1) ?
	        ACPHY_TBL_ID_GAIN1 : (core == 2) ?
	        ACPHY_TBL_ID_GAIN2 : ACPHY_TBL_ID_GAIN3;
	wlc_phy_table_write_acphy(
		pi, gains_tblid, pi_ac->rxgainctrl_stage_len[i],
		offset, ACPHY_GAINDB_TBL_WIDTH, gains);
	memcpy(pi_ac->rxgainctrl_params[core].gaintbl[i], gains,
	       gain_tbl_entry_size * pi_ac->rxgainctrl_stage_len[i]);
}

uint8
wlc_phy_rxgainctrl_encode_gain_acphy(phy_info_t *pi, uint8 clipgain, uint8 core, int8 gain_dB,
	bool trloss, bool lna1byp, uint8 *gidx)
{
	int16 min_gains[ACPHY_MAX_RX_GAIN_STAGES], max_gains[ACPHY_MAX_RX_GAIN_STAGES];
	int8 k, idx, maxgain_this_stage;
	int16 sum_min_gains, gain_needed, tr = 0;
	uint8 i, j;
	int8 *gaintbl_this_stage, gain_this_stage = -128;
	int16 total_gain = 0;
	int16 gain_applied = 0;
	uint8 *gainbitstbl_this_stage;
	uint8 gaintbl_len, lowest_idx;
	int8 lna1mingain, lna1maxgain;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint8 rnd_stage = (TINY_RADIO(pi) || ACMAJORREV_36(pi->pubpi.phy_rev))?
			ACPHY_MAX_RX_GAIN_STAGES - 4 : IS_28NM_RADIO(pi) ?
			ACPHY_MAX_RX_GAIN_STAGES-2 : ACPHY_MAX_RX_GAIN_STAGES - 3;
	int8 lna1_byp_gain = 0;
	uint8 lna1_byp_ind = 0;
	acphy_rxgainctrl_t gainctrl_params;

	PHY_TRACE(("%s: TARGET %d\n", __FUNCTION__, gain_dB));
	ASSERT(core < PHY_CORE_MAX);

	memcpy(&gainctrl_params, &pi_ac->rxgainctrl_params[core], sizeof(acphy_rxgainctrl_t));

	/* Over-write tia table to all same if using tia_high_mode */
	if ((ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) &&
		TINY_RADIO(pi) && (clipgain == 0)) {
		idx = 7;
		for (j = 0; j < pi_ac->rxgainctrl_stage_len[TIA_ID]; j++) {
			gainctrl_params.gainbitstbl[TIA_ID][j] = idx;
			gainctrl_params.gaintbl[TIA_ID][j] =
			        pi_ac->rxgainctrl_params[core].gaintbl[TIA_ID][idx];
		}
	}

	if (lna1byp) {
		if ((ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) &&
		    TINY_RADIO(pi)) {
		lna1_byp_gain = (READ_PHYREGC(pi, _lna1BypVals, core) &
		                ACPHY_Core0_lna1BypVals_lna1BypGain0_MASK(pi->pubpi.phy_rev))
		        >> ACPHY_Core0_lna1BypVals_lna1BypGain0_SHIFT(pi->pubpi.phy_rev);
			for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
				max_gains[i] = pi_ac->rxgainctrl_maxout_gains[i] + tr;
		} else if (IS_28NM_RADIO(pi)) {
			tr = 0;
			for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
				max_gains[i] = pi_ac->rxgainctrl_maxout_gains[i];
		} else {
			tr = pi_ac->fem_rxgains[core].trloss;
			lna1mingain = gainctrl_params.gaintbl[1][0];
			lna1maxgain = gainctrl_params.gaintbl[1][5];
			tr = tr - (lna1maxgain - lna1mingain);
			PHY_INFORM(("lna1mingain=%d ,lna1maxgain=%d, new tr=%d \n",
			            lna1mingain, lna1maxgain, tr));
			for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
				max_gains[i] = pi_ac->rxgainctrl_maxout_gains[i] + tr;
		}
	} else if (trloss) {
		tr =  pi_ac->fem_rxgains[core].trloss;
		for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
			max_gains[i] = pi_ac->rxgainctrl_maxout_gains[i] +
			        pi_ac->fem_rxgains[core].trloss;
	} else {
		tr = 0;
		for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
			max_gains[i] = pi_ac->rxgainctrl_maxout_gains[i];
	}

	gain_needed = gain_dB + tr;
	for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++) {
		min_gains[i] = gainctrl_params.gaintbl[i][0];
	}

	for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++) {
		if (i == rnd_stage) {
			if ((gain_needed % 3) == 2)
				++gain_needed;
			if ((!TINY_RADIO(pi)) && (gain_needed > 30))
				gain_needed = 30;
		}
		sum_min_gains = 0;

		for (j = i + 1; j < ACPHY_MAX_RX_GAIN_STAGES; j++) {
			if (TINY_RADIO(pi) && i < 5 && j >= 5)
				break;
			sum_min_gains += min_gains[j];
		}

		maxgain_this_stage = gain_needed - sum_min_gains;
		gaintbl_this_stage = gainctrl_params.gaintbl[i];
		gainbitstbl_this_stage = gainctrl_params.gainbitstbl[i];
		gaintbl_len = pi_ac->rxgainctrl_stage_len[i];
		if (lna1byp && (i == 1)) {
			gaintbl_len = 1;
			lna1_byp_gain = READ_PHYREGFLD(pi, Core0_lna1BypVals, lna1BypGain0);
			lna1_byp_ind = READ_PHYREGFLD(pi, Core0_lna1BypVals, lna1BypIndex0);
		}

		/* Limit TIA to max tia index programmed in case of SSAGC */
		if (IS_28NM_RADIO(pi) && !ACMAJORREV_36(pi->pubpi.phy_rev)) {
			if (i == 3) {
				if (clipgain == INIT_GAIN) {
					gidx[i] = 15;
					gain_this_stage =  28;
					gain_applied += gain_this_stage;
					total_gain += gain_this_stage;
					gain_needed = gain_needed - gain_this_stage;
					gaintbl_len = 0;
					/* TIA in INIT_GAIN is special mode, bypass calc below */
				} else {
					gaintbl_len = 7;
				}
			}
		}
		/* Limit Mixer Gain to 2 in 5G band */
		if (ACMAJORREV_36(pi->pubpi.phy_rev) && !CHSPEC_IS2G(pi->radio_chanspec)) {
			if (i == 2) {
				gaintbl_len = 3;
			}
		}
		if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
			if (i == 3) {
				if (clipgain == INIT_GAIN) {
					gidx[i] = 9;
					gain_this_stage =  33;
					gain_applied += gain_this_stage;
					gain_needed = gain_needed - gain_this_stage;
					gaintbl_len = 0;
					/* TIA in INIT_GAIN is special mode, bypass calc below */
				}
			}
		}

		for (k = gaintbl_len - 1; k >= 0; k--) {
			if (lna1byp && (i == 1)) {
				gain_this_stage = lna1_byp_gain;
			} else {
				gain_this_stage = gaintbl_this_stage[k];
			}
			total_gain = gain_this_stage + gain_applied;
			lowest_idx = 0;

			if (gainbitstbl_this_stage[k] == gainbitstbl_this_stage[0])
				lowest_idx = 1;
			if ((lowest_idx == 1) ||
			    ((gain_this_stage <= maxgain_this_stage) && (total_gain
			                                                 <= max_gains[i]))) {
				if (lna1byp && (i == 1)) {
					gidx[i] = lna1_byp_ind;
				} else {
					gidx[i] = gainbitstbl_this_stage[k];
				}
				gain_applied += gain_this_stage;
				gain_needed = gain_needed - gain_this_stage;
				break;
			}
		}

		/* Same output as for TCL with verbose = 1 */
		PHY_INFORM(("Core %d, gain stage %d, idx = %d/%d, gain_this_state = %d, "
			    "total_gain_till_now = %d, gain_needed = %d\n",
			    core, i, gidx[i], gaintbl_len, gain_this_stage,
			    total_gain, gain_needed));

		if (!IS_28NM_RADIO(pi)) {	// Doing badness for 7271

			/* If we want to fix max_tia_gain = initgain (for lesi) */
			if ((clipgain == 0) && (i > 0) && (i <= 3) && pi_ac->tia_idx_max_eq_init) {
				wlc_phy_max_anagain_to_initgain_lesi_acphy(pi, i, core, gidx[i]);
			}
		}
	}
	PHY_INFORM(("gain_applied = %d, tr = %d \n", gain_applied, tr));

	return (gain_applied - tr);
}


static void
wlc_phy_rxgainctrl_nbclip_acphy(phy_info_t *pi, uint8 core, int8 rxpwr_dBm)
{
	/* Multuply all pwrs by 10 to avoid floating point math */
	int rxpwrdBm_60mv, pwr;
	int pwr_60mv[] = {-40, -40, -40};     /* 20, 40, 80 */
	uint8 nb_thresh[] = {0, 35, 60, 80, 95, 120, 140, 156}; /* nb_thresh*10 to avoid float */
	const char *reg_name[ACPHY_NUM_NB_THRESH] = {"low", "low", "mid", "mid", "mid",
					       "mid", "high", "high"};
	uint8 mux_sel[] = {0, 0, 1, 1, 1, 1, 2, 2};
	uint8 reg_val[] = {1, 0, 1, 2, 0, 3, 1, 0};
	uint8 nb, i;
	int nb_thresh_bq[ACPHY_NUM_NB_THRESH];
	int v1, v2, vdiff1, vdiff2;
	uint8 idx[ACPHY_MAX_RX_GAIN_STAGES];
	uint16 initgain_codeA;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	ASSERT(core < PHY_CORE_MAX);
	ASSERT(RADIOID(pi->pubpi.radioid) == BCM2069_ID);

	rxpwrdBm_60mv = (CHSPEC_IS80(pi->radio_chanspec) ||
		PHY_AS_80P80(pi, pi->radio_chanspec)) ? pwr_60mv[2]
		: CHSPEC_IS160(pi->radio_chanspec) ? 0
		: (CHSPEC_IS40(pi->radio_chanspec)) ? pwr_60mv[1] : pwr_60mv[0];

	for (i = 0; i < ACPHY_NUM_NB_THRESH; i++) {
		nb_thresh_bq[i] = rxpwrdBm_60mv + nb_thresh[i];
	}

	/* Get the INITgain code */
	initgain_codeA = READ_PHYREGC(pi, InitGainCodeA, core);

	idx[0] = (initgain_codeA &
		ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initExtLnaIndex)) >>
		ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initExtLnaIndex);
	idx[1] = (initgain_codeA &
		ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initLnaIndex)) >>
		ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initLnaIndex);
	idx[2] = (initgain_codeA &
		ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initlna2Index)) >>
		ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initlna2Index);
	idx[3] = (initgain_codeA &
		ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initmixergainIndex)) >>
		ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initmixergainIndex);
	idx[4] = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitBiQ0Index);
	idx[5] = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitBiQ1Index);
	idx[6] = READ_PHYREGFLDC(pi, InitGainCodeB, core, initvgagainIndex);

	pwr = rxpwr_dBm;
	for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES - 2; i++)
		pwr += pi_ac->rxgainctrl_params[core].gaintbl[i][idx[i]];
	if (pi_ac->curr_desense->elna_bypass == 1)
		pwr = pwr - pi_ac->fem_rxgains[core].trloss;
	pwr = pwr * 10;

	nb = 0;
	if (pwr < nb_thresh_bq[0]) {
		nb = 0;
	} else if (pwr > nb_thresh_bq[ACPHY_NUM_NB_THRESH - 1]) {
		nb = ACPHY_NUM_NB_THRESH - 1;

		/* Reduce the bq0 gain, if can't achieve nbclip with highest nbclip thresh */
		if ((pwr - nb_thresh_bq[ACPHY_NUM_NB_THRESH - 1]) > 20) {
			if ((idx[4] > 0) && (idx[5] < 7)) {
				MOD_PHYREGC(pi, InitGainCodeB, core, InitBiQ0Index, idx[4] - 1);
				MOD_PHYREGC(pi, InitGainCodeB, core, InitBiQ1Index, idx[5] + 1);
			}
		}
	} else {
		for (i = 0; i < ACPHY_NUM_NB_THRESH - 1; i++) {
			v1 = nb_thresh_bq[i];
			v2 = nb_thresh_bq[i + 1];
			if ((pwr >= v1) && (pwr <= v2)) {
				vdiff1 = pwr > v1 ? (pwr - v1) : (v1 - pwr);
				vdiff2 = pwr > v2 ? (pwr - v2) : (v2 - pwr);

				if (vdiff1 < vdiff2)
					nb = i;
				else
					nb = i+1;
				break;
			}
		}
	}

	MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcNbClipMuxSel, mux_sel[nb]);

	if (strcmp(reg_name[nb], "low") == 0) {
		MOD_RADIO_REGC(pi, NBRSSI_CONFG, core, nbrssi_Refctrl_low, reg_val[nb]);
	} else if (strcmp(reg_name[nb], "mid") == 0) {
		MOD_RADIO_REGC(pi, NBRSSI_CONFG, core, nbrssi_Refctrl_mid, reg_val[nb]);
	} else {
		MOD_RADIO_REGC(pi, NBRSSI_CONFG, core, nbrssi_Refctrl_high, reg_val[nb]);
	}
}

static void
wlc_phy_rxgainctrl_w1clip_acphy_28nm_ulp(phy_info_t *pi, uint8 core, int8 rxpwr_dBm)
{
	/* Multuply all pwrs by 10 to avoid floating point math */

	int lna1_rxpwrdBm_lo4;
	uint8 *w1_hi;

	uint8 w1_delta_low[] = {0, 12, 23, 37, 45, 56, 63, 72, 78, 86, 93, 97};
	uint8 w1_delta_mid[] = {0, 17, 32, 45, 56, 67, 76, 84, 92, 99, 106, 113};
	uint8 w1_delta_hi2g[] = {0, 18, 33, 47, 59, 71, 81, 90, 98, 106, 111, 107};
	uint8 w1_delta_hi5g[] = {0, 18, 33, 47, 59, 71, 81, 90, 98, 106, 111, 107};
	/* 5g hi values are to be tuned later */
	int w1_thresh_low[ACPHY_NUM_W1_THRESH], w1_thresh_mid[ACPHY_NUM_W1_THRESH];
	int w1_thresh_high[ACPHY_NUM_W1_THRESH];
	int *w1_thresh;
	uint8 i, w1_muxsel, w1;
	uint8 elna, lna1_idx;
	int v1, v2, vdiff1, vdiff2, pwr, lna1_diff;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;
	int8 lna1_gain_idx5;

	ASSERT(core < PHY_CORE_MAX);
	w1_hi = CHSPEC_IS2G(pi->radio_chanspec) ? &w1_delta_hi2g[0] : &w1_delta_hi5g[0];

	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		lna1_rxpwrdBm_lo4 = -380;
		lna1_gain_idx5 = rxg_params->lna12_gain_2g_tbl[0][5];
	} else {
		lna1_rxpwrdBm_lo4 = -340;
		lna1_gain_idx5 = rxg_params->lna12_gain_5g_tbl[0][5];
	}

	/* mid is 5dB higher than low, and high is 6dB higher than mid */
	for (i = 0; i < ACPHY_NUM_W1_THRESH; i++) {
		w1_thresh_low[i] = lna1_rxpwrdBm_lo4 + w1_delta_low[i];
		w1_thresh_mid[i] = 50 + lna1_rxpwrdBm_lo4 + w1_delta_mid[i];
		w1_thresh_high[i] = 110 + lna1_rxpwrdBm_lo4 + w1_hi[i];
	}

	elna = pi_ac->rxgainctrl_params[core].gaintbl[0][0];
	lna1_idx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initLnaIndex);
	lna1_diff = lna1_gain_idx5 -
			pi_ac->rxgainctrl_params[core].gaintbl[1][lna1_idx];

	pwr = rxpwr_dBm + elna - lna1_diff;
	if (pi_ac->curr_desense->elna_bypass == 1) {
		pwr = pwr - pi_ac->rxgcrsi->fem_rxgains[core].trloss;
	}
	pwr = pwr * 10;

	if (pwr <= w1_thresh_low[0]) {
		w1 = 0;
		w1_muxsel = 0;
	} else if (pwr >= w1_thresh_high[ACPHY_NUM_W1_THRESH - 1]) {
		w1 = 11;
		w1_muxsel = 2;
	} else {
		/* Prefering W1 high instead mid, it is better choice for HWACI */
		/* HWACI can detect till -40dbm inthis case as W1 high is at -30dbm,low is -40dbm */
		if (pwr >= w1_thresh_high[0]) {
			w1_thresh = w1_thresh_high;
			w1_muxsel = 2;
		} else if (pwr >= w1_thresh_mid[0]) {
			w1_thresh = w1_thresh_mid;
			w1_muxsel = 1;
		} else {
			w1_thresh = w1_thresh_low;
			w1_muxsel = 0;
		}

		for (w1 = 0; w1 < ACPHY_NUM_W1_THRESH - 1; w1++) {
			v1 = w1_thresh[w1];
			v2 = w1_thresh[w1 + 1];
			if ((pwr >= v1) && (pwr <= v2)) {
				vdiff1 = pwr > v1 ? (pwr - v1) : (v1 - pwr);
				vdiff2 = pwr > v2 ? (pwr - v2) : (v2 - pwr);

				if (vdiff2 <= vdiff1)
					w1 = w1 + 1;

				break;
			}
		}
	}

	MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW1ClipMuxSel, w1_muxsel);
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		MOD_RADIO_REG_20696(pi, LNA2G_REG3, core, lna2g_dig_wrssi1_threshold, w1+4);
	} else {
		MOD_RADIO_REG_20696(pi, RX5G_WRSSI1, core, rx5g_wrssi_threshold, w1+4);
	}
}

static void
wlc_phy_rxgainctrl_w1clip_acphy(phy_info_t *pi, uint8 core, int8 rxpwr_dBm)
{
	/* Multuply all pwrs by 10 to avoid floating point math */

	int lna1_rxpwrdBm_lo4;
	int lna1_pwrs_w1clip[] = {-340, -340, -340};   /* 20, 40, 80 */
	uint8 *w1_hi, w1_delta[] = {0, 19, 35, 49, 60, 70, 80, 88, 95, 102, 109, 115};
	uint8 w1_delta_hi2g[] = {0, 19, 35, 49, 60, 70, 80, 92, 105, 120, 130, 140};
	uint8 w1_delta_hi5g[] = {0, 19, 35, 49, 60, 70, 80, 96, 113, 130, 155, 180};
	int w1_thresh_low[ACPHY_NUM_W1_THRESH], w1_thresh_mid[ACPHY_NUM_W1_THRESH];
	int w1_thresh_high[ACPHY_NUM_W1_THRESH];
	int *w1_thresh;
	uint8 i, w1_muxsel, w1;
	uint8 elna, lna1_idx;
	int v1, v2, vdiff1, vdiff2, pwr, lna1_diff;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	ASSERT(core < PHY_CORE_MAX);
	w1_hi = CHSPEC_IS2G(pi->radio_chanspec) ? &w1_delta_hi2g[0] : &w1_delta_hi5g[0];

	if (TINY_RADIO(pi)) {
		if (CHSPEC_IS2G(pi->radio_chanspec))
			lna1_rxpwrdBm_lo4 = (ACMAJORREV_32(pi->pubpi.phy_rev) ||
				ACMAJORREV_33(pi->pubpi.phy_rev))? -360 : -370;
		else
			lna1_rxpwrdBm_lo4 = -310;
	} else
		lna1_rxpwrdBm_lo4 = (CHSPEC_IS80(pi->radio_chanspec) ||
				PHY_AS_80P80(pi, pi->radio_chanspec)) ? lna1_pwrs_w1clip[2]
				: CHSPEC_IS160(pi->radio_chanspec) ? 0
		      : (CHSPEC_IS40(pi->radio_chanspec)) ? lna1_pwrs_w1clip[1]
				: lna1_pwrs_w1clip[0];

	/* mid is 6dB higher than low, and high is 6dB higher than mid */
	for (i = 0; i < ACPHY_NUM_W1_THRESH; i++) {
		w1_thresh_low[i] = lna1_rxpwrdBm_lo4 + w1_delta[i];
		w1_thresh_mid[i] = 60 + w1_thresh_low[i];
		w1_thresh_high[i] = 120 + lna1_rxpwrdBm_lo4 + w1_hi[i];
	}

	elna = pi_ac->rxgainctrl_params[core].gaintbl[0][0];

	lna1_idx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initLnaIndex);

	if (TINY_RADIO(pi)) {
		lna1_diff = 24 - pi_ac->rxgainctrl_params[core].gaintbl[1][lna1_idx];
	} else if (CHSPEC_IS2G(pi->radio_chanspec) &&
	    (ACMAJORREV_2(pi->pubpi.phy_rev) && ACMINORREV_3(pi))) {
		lna1_diff = 25 - pi_ac->rxgainctrl_params[core].gaintbl[1][lna1_idx];
	} else {
		lna1_diff = pi_ac->rxgainctrl_params[core].gaintbl[1][5] -
		            pi_ac->rxgainctrl_params[core].gaintbl[1][lna1_idx];
	}

	pwr = rxpwr_dBm + elna - lna1_diff;
	if (pi_ac->curr_desense->elna_bypass == 1)
		pwr = pwr - pi_ac->fem_rxgains[core].trloss;
	pwr = pwr * 10;

	if (pwr <= w1_thresh_low[0]) {
		w1 = 0;
		w1_muxsel = 0;
	} else if (pwr >= w1_thresh_high[ACPHY_NUM_W1_THRESH - 1]) {
		w1 = 11;
		w1_muxsel = 2;
	} else {
		if (pwr > w1_thresh_mid[ACPHY_NUM_W1_THRESH - 1]) {
			w1_thresh = w1_thresh_high;
			w1_muxsel = 2;
		} else if (pwr < w1_thresh_mid[0]) {
			w1_thresh = w1_thresh_low;
			w1_muxsel = 0;
		} else {
			w1_thresh = w1_thresh_mid;
			w1_muxsel = 1;
		}

		for (w1 = 0; w1 < ACPHY_NUM_W1_THRESH - 1; w1++) {
			v1 = w1_thresh[w1];
			v2 = w1_thresh[w1 + 1];
			if ((pwr >= v1) && (pwr <= v2)) {
				vdiff1 = pwr > v1 ? (pwr - v1) : (v1 - pwr);
				vdiff2 = pwr > v2 ? (pwr - v2) : (v2 - pwr);

				if (vdiff2 <= vdiff1)
					w1 = w1 + 1;

				break;
			}
		}
	}

	if (TINY_RADIO(pi)) {
		MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW1ClipMuxSel, w1_muxsel);

		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			MOD_RADIO_REG_TINY(pi, LNA2G_RSSI1, core, lna2g_dig_wrssi1_threshold, w1+4);
		} else {
			MOD_RADIO_REG_TINY(pi, LNA5G_RSSI1, core, lna5g_dig_wrssi1_threshold, w1+4);
		}
	} else {
		/* the w1 thresh array is wrt w1 code = 4 */
		/*	MOD_PHYREGC(pi, FastAgcClipCntTh, core, fastAgcW1ClipCntTh, w1 + 4); */
		MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW1ClipMuxSel, w1_muxsel);
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			MOD_RADIO_REGC(pi, LNA2G_RSSI, core, dig_wrssi1_threshold, w1 + 4);
		} else {
			MOD_RADIO_REGC(pi, LNA5G_RSSI, core, dig_wrssi1_threshold, w1 + 4);
		}
	}
}

#ifndef WLC_DISABLE_ACI
static void
wlc_phy_set_crs_min_pwr_higain_acphy(phy_info_t *pi, uint8 thresh)
{
	MOD_PHYREG(pi, crsminpoweru0, crsminpower1, thresh);
	MOD_PHYREG(pi, crsmfminpoweru0, crsmfminpower1, thresh);
	MOD_PHYREG(pi, crsminpowerl0, crsminpower1, thresh);
	MOD_PHYREG(pi, crsmfminpowerl0, crsmfminpower1, thresh);
	MOD_PHYREG(pi, crsminpoweruSub10, crsminpower1, thresh);
	MOD_PHYREG(pi, crsmfminpoweruSub10, crsmfminpower1,  thresh);
	MOD_PHYREG(pi, crsminpowerlSub10, crsminpower1, thresh);
	MOD_PHYREG(pi, crsmfminpowerlSub10, crsmfminpower1,  thresh);
}
#endif /* !WLC_DISABLE_ACI */

static void
wlc_phy_limit_rxgaintbl_acphy(uint8 gaintbl[], uint8 gainbitstbl[], uint8 sz,
	const uint8 default_gaintbl[], uint8 min_idx, uint8 max_idx)
{
	uint8 i;

	for (i = 0; i < sz; i++) {
		if (i < min_idx) {
			gaintbl[i] = default_gaintbl[min_idx];
			gainbitstbl[i] = min_idx;
		} else if (i > max_idx) {
			gaintbl[i] = default_gaintbl[max_idx];
			gainbitstbl[i] = max_idx;
		} else {
			gaintbl[i] = default_gaintbl[i];
			gainbitstbl[i] = i;
		}
	}
}

static void
wlc_phy_rxgainctrl_nbclip_acphy_28nm_ulp(phy_info_t *pi, uint8 core, int8 rxpwr_dBm)
{
	/* Multuply all pwrs by 10 to avoid floating point math */
	int16 rxpwrdBm_bw, pwr;
	int16 pwr_dBm[] = {-40, -40, -40};	/* 20, 40, 80 */
	int16 nb_thresh[] = { 0, 20, 40, 60, 80, 100, 120, 140, 160}; /* nb_thresh*10 avoid float */
	int8 reg_id[ACPHY_NUM_NB_THRESH_TINY] = { NB_LOW, NB_LOW, NB_LOW, NB_MID, NB_MID, NB_HIGH,
		NB_HIGH, NB_HIGH, NB_HIGH };
	uint8 mux_sel[] = {0, 0, 0, 1, 1, 2, 2, 2, 2};
	uint8 reg_val[] = {0, 2, 4, 2, 4, 1, 3, 5, 7};
	uint8 nb, i;
	int v1, v2, vdiff1, vdiff2;
	int8 idx[ACPHY_MAX_RX_GAIN_STAGES] = {-1, -1, -1, -1, -1, -1, -1};
	uint16 initgain_codeA;
	bool use_w3_detector = FALSE; /* To chhose betwen NB(T2) and W3(T1) detectors */
	int16 t2_delta_dB = 0; /* the power delta between T2 and T1 detectors */
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	int16 nb_thresh_end;

	ASSERT(core < PHY_CORE_MAX);
	ASSERT(ACMAJORREV_36(pi->pubpi.phy_rev) ||
			ACMAJORREV_25(pi->pubpi.phy_rev) ||
			ACMAJORREV_40(pi->pubpi.phy_rev));

	rxpwrdBm_bw = (CHSPEC_IS80(pi->radio_chanspec)) ? pwr_dBm[2] :
		(CHSPEC_IS40(pi->radio_chanspec)) ? pwr_dBm[1] : pwr_dBm[0];

	PHY_TRACE(("%s: adc pwr %d \n", __FUNCTION__, rxpwrdBm_bw));

	nb_thresh_end = nb_thresh[ACPHY_NUM_NB_THRESH_TINY - 1] + rxpwrdBm_bw;

	/* Get the INITgain code */
	initgain_codeA = READ_PHYREGC(pi, InitGainCodeA, core);

	idx[0] = (initgain_codeA &
			ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initExtLnaIndex)) >>
		ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initExtLnaIndex);
	idx[1] = (initgain_codeA &
			ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initLnaIndex)) >>
		ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initLnaIndex);
	idx[2] = (initgain_codeA &
			ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initlna2Index)) >>
		ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initlna2Index);
	idx[3] = (initgain_codeA &
			ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initmixergainIndex)) >>
		ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initmixergainIndex);

	pwr = rxpwr_dBm;

	for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES - 2; i++)
		if (idx[i] >= 0)
			pwr += pi_ac->rxgainctrl_params[core].gaintbl[i][idx[i]];

	if (pi_ac->curr_desense->elna_bypass == 1)
		pwr -= pi_ac->rxgcrsi->fem_rxgains[core].trloss;

	pwr *= 10;

	/* Use T1 detector bank if T2 bank is not covering pwr */
	if (pwr > nb_thresh_end) {
		use_w3_detector = TRUE;

		if (CHSPEC_IS80(pi->radio_chanspec)) {
			if (idx[3] >= 8) {
				t2_delta_dB = 75;
			} else if (idx[3] >= 4) {
				t2_delta_dB = 60;
			} else {
				t2_delta_dB = 0;
			}
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			if (idx[3] >= 8) {
				t2_delta_dB = 75;
			} else if (idx[3] >= 5) {
				t2_delta_dB = 60;
			} else {
				t2_delta_dB = 0;
			}
		} else {
			if (idx[3] >= 5) {
				if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
					/* 12.0 * 10 to avoid float */
					t2_delta_dB = 120;
				} else {
					t2_delta_dB = 60;
				}
			} else {
				t2_delta_dB = 0;
			}
		}
	}

	use_w3_detector = use_w3_detector && (t2_delta_dB > 0);

	for (i = 0; i < ACPHY_NUM_NB_THRESH_TINY; i++) {
		nb_thresh[i] += (rxpwrdBm_bw + t2_delta_dB);
	}

	if (pwr < nb_thresh[0]) {
		nb = 0;
	} else if (pwr > nb_thresh[ACPHY_NUM_NB_THRESH_TINY - 1]) {
		nb = ACPHY_NUM_NB_THRESH_TINY - 1;
	} else {
		nb = 0;
		for (i = 0; i < ACPHY_NUM_NB_THRESH_TINY - 1; i++) {
			v1 = nb_thresh[i];
			v2 = nb_thresh[i + 1];
			if ((pwr >= v1) && (pwr <= v2)) {
				vdiff1 = pwr > v1 ? (pwr - v1) : (v1 - pwr);
				vdiff2 = pwr > v2 ? (pwr - v2) : (v2 - pwr);

				nb = (vdiff1 < vdiff2) ? i : i + 1;
				break;
			}
		}
	}

	MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcNbClipMuxSel, mux_sel[nb]);

	if (use_w3_detector) {
		MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW3ClipMuxSel, 1);

#ifdef BCM7271
		MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG1, 0, tia_nbrssi1_threshold_high, 0);
		MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG1, 0, tia_nbrssi1_threshold_mid, 0);
		MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG1, 0, tia_nbrssi1_threshold_low, 0);
#else // BCM7271
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_NBRSSI_REG1, 0,
					tia_nbrssi1_threshold0, 0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_NBRSSI_REG1, 0,
					tia_nbrssi1_threshold1, 0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_NBRSSI_REG1, 0,
					tia_nbrssi1_threshold2, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
#endif // BCM7271

#ifdef BCM7271
		if (reg_id[nb] == NB_LOW) {
			MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG1, core,
					tia_nbrssi1_threshold_low, reg_val[nb]);
		} else if (reg_id[nb] == NB_MID) {
			MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG1, core,
					tia_nbrssi1_threshold_mid, reg_val[nb]);
		} else {
			MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG1, core,
					tia_nbrssi1_threshold_high, reg_val[nb]);
		}
#else // BCM7271
		if (reg_id[nb] == NB_LOW) {
			MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG1, core,
					tia_nbrssi1_threshold0, reg_val[nb]);
		} else if (reg_id[nb] == NB_MID) {
			MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG1, core,
					tia_nbrssi1_threshold1, reg_val[nb]);
		} else {
			MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG1, core,
					tia_nbrssi1_threshold2, reg_val[nb]);
		}
#endif // BCM7271
	} else {
		MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW3ClipMuxSel, 0);

#ifdef BCM7271
		MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG2, 0, tia_nbrssi2_threshold_high, 0);
		MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG2, 0, tia_nbrssi2_threshold_mid, 0);
		MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG2, 0, tia_nbrssi2_threshold_low, 0);
#else // BCM7271
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_NBRSSI_REG2, 0,
					tia_nbrssi2_threshold0, 0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_NBRSSI_REG2, 0,
					tia_nbrssi2_threshold1, 0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_NBRSSI_REG2, 0,
					tia_nbrssi2_threshold2, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
#endif // BCM7271

#ifdef BCM7271
		if (reg_id[nb] == NB_LOW) {
			MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG2, core,
					tia_nbrssi2_threshold_low, reg_val[nb]);
		} else if (reg_id[nb] == NB_MID) {
			MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG2, core,
					tia_nbrssi2_threshold_mid, reg_val[nb]);
		} else {
			MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG2, core,
					tia_nbrssi2_threshold_high, reg_val[nb]);
		}
#else // BCM7271
		if (reg_id[nb] == NB_LOW) {
			MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG2, core,
					tia_nbrssi2_threshold0, reg_val[nb]);
		} else if (reg_id[nb] == NB_MID) {
			MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG2, core,
					tia_nbrssi2_threshold1, reg_val[nb]);
		} else {
			MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG2, core,
					tia_nbrssi2_threshold2, reg_val[nb]);
		}
#endif // BCM7271
	}
}

static void
wlc_phy_rxgainctrl_nbclip_acphy_tiny(phy_info_t *pi, uint8 core, int8 rxpwr_dBm)
{
	/* Multuply all pwrs by 10 to avoid floating point math */
	int16 rxpwrdBm_bw, pwr, tmp_pwr;
	int16 pwr_dBm[] = {-40, -40, -40};	/* 20, 40, 80 */
	int16 nb_thresh[] = { 0, 20, 40, 60, 80, 100, 120, 140, 160}; /* nb_thresh*10 avoid float */
	const char * const reg_name[ACPHY_NUM_NB_THRESH_TINY] = {"low", "low", "low", "mid", "mid",
	                                                    "high", "high", "high", "high"};
	uint8 mux_sel[] = {0, 0, 0, 1, 1, 2, 2, 2, 2};
	uint8 reg_val[] = {0, 2, 4, 2, 4, 1, 3, 5, 7};
	uint8 nb, i;
	int v1, v2, vdiff1, vdiff2;
	int8 idx[ACPHY_MAX_RX_GAIN_STAGES] = {-1, -1, -1, -1, -1, -1, -1};
	uint16 initgain_codeA;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	bool use_w3_detector = FALSE; /* To chhose betwen NB(T2) and W3(T1) detectors */
	int16 t2_delta_dB = 0; /* the power delta between T2 and T1 detectors */
	int16 nb_thresh_end;

	ASSERT(core < PHY_CORE_MAX);
	ASSERT(TINY_RADIO(pi));

	rxpwrdBm_bw = (CHSPEC_IS80(pi->radio_chanspec) ||
		PHY_AS_80P80(pi, pi->radio_chanspec)) ? pwr_dBm[2]
		: (CHSPEC_IS160(pi->radio_chanspec)) ? 0
		: (CHSPEC_IS40(pi->radio_chanspec)) ? pwr_dBm[1] : pwr_dBm[0];

	PHY_TRACE(("%s: adc pwr %d \n", __FUNCTION__, rxpwrdBm_bw));

	nb_thresh_end = nb_thresh[ACPHY_NUM_NB_THRESH_TINY - 1] + rxpwrdBm_bw;

	/* Get the INITgain code */
	initgain_codeA = READ_PHYREGC(pi, InitGainCodeA, core);

	idx[0] = (initgain_codeA &
		ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initExtLnaIndex)) >>
		ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initExtLnaIndex);
	idx[1] = (initgain_codeA &
		ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initLnaIndex)) >>
		ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initLnaIndex);
	idx[3] = (initgain_codeA &
		ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initmixergainIndex)) >>
		ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initmixergainIndex);

	pwr = rxpwr_dBm;

	for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES - 2; i++) {
		if (idx[i] >= 0) {
			tmp_pwr = pi_ac->rxgainctrl_params[core].gaintbl[i][idx[i]];
			pwr += tmp_pwr;
		}
	}

	if (pi_ac->curr_desense->elna_bypass == 1)
		pwr -= pi_ac->fem_rxgains[core].trloss;

	pwr *= 10;

	/* Use T1 detector bank if T2 bank is not covering pwr, and in 5G only (for the */
	/* time being). As it can be seen for T1 vs. T2 delta to be > 0 TIA index >4 is */
	/* needed, typically for the current 2G gain line up TIA index = 4 (BIQ 2 is off), */
	/* and T1 doesn't help. To make this work for 2G, it is needed to decresse LNA */
	/* index which can lower sensitivity. */
	if (pwr > nb_thresh_end) {
	  use_w3_detector = TRUE;

		if (CHSPEC_IS80(pi->radio_chanspec) ||
				PHY_AS_80P80(pi, pi->radio_chanspec)) {
	    if (idx[3] >= 8)
	      t2_delta_dB = 75;
	    else if (idx[3] >= 4)
	      t2_delta_dB = 60;
	    else
	      t2_delta_dB = 0;
	  } else if (CHSPEC_IS160(pi->radio_chanspec)) {
		  ASSERT(0);
	  } else if (CHSPEC_IS40(pi->radio_chanspec)) {
	    if (idx[3] >= 8)
	      t2_delta_dB = 75;
	    else if (idx[3] >= 5)
	      t2_delta_dB = 60;
	    else
	      t2_delta_dB = 0;
	  } else {
	    if (idx[3] >= 5)
	      t2_delta_dB = 60;
	    else
	      t2_delta_dB = 0;
	  }

	}

	use_w3_detector = use_w3_detector && (t2_delta_dB > 0);
	for (i = 0; i < ACPHY_NUM_NB_THRESH_TINY; i++) {
	  nb_thresh[i] += (rxpwrdBm_bw + t2_delta_dB);
	}

	if (pwr < nb_thresh[0]) {
		nb = 0;
	} else if (pwr > nb_thresh[ACPHY_NUM_NB_THRESH_TINY - 1]) {
		nb = ACPHY_NUM_NB_THRESH_TINY - 1;
	} else {
		nb = 0;
		for (i = 0; i < ACPHY_NUM_NB_THRESH_TINY - 1; i++) {
			v1 = nb_thresh[i];
			v2 = nb_thresh[i + 1];
			if ((pwr >= v1) && (pwr <= v2)) {
				vdiff1 = pwr > v1 ? (pwr - v1) : (v1 - pwr);
				vdiff2 = pwr > v2 ? (pwr - v2) : (v2 - pwr);

				nb = (vdiff1 < vdiff2) ? i : i + 1;
				break;
			}
		}
	}

	if (ACMAJORREV_4(pi->pubpi.phy_rev)) {
		MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, wrssi3_ref_low_sel, 0);
		MOD_RADIO_REG_TINY(pi, TIA_CFG12, core, wrssi3_ref_mid_sel, 0);
		MOD_RADIO_REG_TINY(pi, TIA_CFG12, core, wrssi3_ref_high_sel, 0);
		MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_low_sel, 0);
		MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_mid_sel, 0);
		MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_high_sel, 0);
	}

	MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcNbClipMuxSel, mux_sel[nb]);
	if (use_w3_detector) {
	  MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW3ClipMuxSel, 1);
	  if (strcmp(reg_name[nb], "low") == 0) {
	    MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, wrssi3_ref_low_sel, reg_val[nb]);
	  } else if (strcmp(reg_name[nb], "mid") == 0) {
	    MOD_RADIO_REG_TINY(pi, TIA_CFG12, core, wrssi3_ref_mid_sel, reg_val[nb]);
	  } else {
	    MOD_RADIO_REG_TINY(pi, TIA_CFG12, core, wrssi3_ref_high_sel, reg_val[nb]);
	  }
	} else {
	  MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW3ClipMuxSel, 0);
	  if (strcmp(reg_name[nb], "low") == 0) {
	    MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_low_sel, reg_val[nb]);
	  } else if (strcmp(reg_name[nb], "mid") == 0) {
	    MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_mid_sel, reg_val[nb]);
	  } else {
	    MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_high_sel, reg_val[nb]);
	  }
	}
}


/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */
/**********  DESENSE : ACI, NOISE, BT (start)  ******** */

void
wlc_phy_rxgainctrl_gainctrl_acphy_tiny(phy_info_t *pi, uint8 init_desense)
{
	/* at present this is just a place holder for
	 * 'static' ELNA configuration. Eventually both TCL and
	 * driver should be changes to follow the auto-calc
	 * routine used in wlc_phy_rxgainctrl_gainctrl_acphy
	 */
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	bool elna_present = (CHSPEC_IS2G(pi->radio_chanspec)) ? BF_ELNA_2G(pi_ac)
	                                                      : BF_ELNA_5G(pi_ac);
	uint8 max_analog_gain;
	int16 maxgain[ACPHY_MAX_RX_GAIN_STAGES] = {0, 0, 0, 0, -100, 125, 125};
	uint8 i, core;
	bool init_trtx, hi_trtx, md_trtx, lo_trtx, clip2_trtx;
	bool lna1byp_fem, lo_lna1byp_core, lo_lna1byp = FALSE, md_lna1byp = FALSE;
	int8 md_low_end, hi_high_end, lo_low_end, md_high_end;
	int8 clip2_gain;
	int8 hi_gain1, mid_gain1, lo_gain1;
	int8 nbclip_pwrdBm, w1clip_pwrdBm;
	int8 clip_gain[] = {61, 41, 19, 1};

	pi_ac->mdgain_trtx_allowed = FALSE;

	MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);

	max_analog_gain = READ_PHYREGFLD(pi, Core0HpFBw, maxAnalogGaindb);

	/* fill in gain limits for analog stages */
	maxgain[0] = maxgain[1] = maxgain[2] = maxgain[3] = max_analog_gain;

	for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
		pi_ac->rxgainctrl_maxout_gains[i] = maxgain[i];

	/* Keep track of it (used in interf_mitigation) */
	pi_ac->curr_desense->elna_bypass = wlc_phy_get_max_lna_index_acphy(pi, ELNA_ID);
	init_trtx = elna_present & pi_ac->curr_desense->elna_bypass;
	hi_trtx = elna_present & pi_ac->curr_desense->elna_bypass;
	md_trtx = elna_present & (pi_ac->mdgain_trtx_allowed | pi_ac->curr_desense->elna_bypass);
	lo_trtx = elna_present;
	clip2_trtx = md_trtx;

	if ((ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) &&
	    TINY_RADIO(pi)) {
		clip_gain[0] = CHSPEC_IS2G(pi->radio_chanspec) ?
		        ACPHY_INIT_GAIN_4365_2G : ACPHY_INIT_GAIN_4365_5G;

#ifdef WL11ULB
		if (CHSPEC_IS10(pi->radio_chanspec) || CHSPEC_IS5(pi->radio_chanspec))
			clip_gain[0] += 3;
		PHY_CAL(("---%s: ulb: init_gain = %d\n", __FUNCTION__, clip_gain[0]));
#endif /* WL11ULB */

		clip_gain[1] = CHSPEC_IS2G(pi->radio_chanspec) ? 47 : 44;
		clip_gain[2] = 31;
		clip_gain[3] = 21;

		clip2_trtx = lo_trtx;
		if (!elna_present) {
			lo_lna1byp = TRUE;
		}
	} else if (!elna_present || md_trtx) {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			clip_gain[2] = 19;
			clip_gain[3] = 1;
		} else {
			clip_gain[2] = 19;
			clip_gain[3] = 1;
		}
	} else {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			clip_gain[2] = 24;
			clip_gain[3] = 15;
		} else {
			clip_gain[2] = 24;
			clip_gain[3] = 15;
		}
	}

	if (ACPHY_ENABLE_FCBS_HWACI(pi))
		/* Apply desense */
		for (i = 0; i < 4; i++)
			clip_gain[i] -= pi_ac->curr_desense->clipgain_desense[i];

	/* with elna if md_gain uses TR != T, then LO_gain needs to be higher */
	clip2_gain = clip2_trtx ? clip_gain[3] : (clip_gain[2] + clip_gain[3]) >> 1;

	FOREACH_CORE(pi, core) {
		lna1byp_fem = pi_ac->fem_rxgains[core].lna1byp;
		lo_lna1byp_core = lo_lna1byp | lna1byp_fem;

		if (lna1byp_fem && ACMAJORREV_4(pi->pubpi.phy_rev)) {
			WRITE_PHYREGC(pi, _lna1BypVals, core, 0xfa1);
			MOD_PHYREG(pi, AfePuCtrl, lna1_pd_during_byp, 1);
		}

		/* 0,1,2,3 for Init, hi, md and lo gains respectively */
		wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 0, clip_gain[0] - init_desense,
		                                            init_trtx, FALSE, core);
		hi_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 1, clip_gain[1],
		                                                       hi_trtx, FALSE, core);
		mid_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 2, clip_gain[2],
		                                                        md_trtx, md_lna1byp, core);
		wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 4, clip2_gain, clip2_trtx,
		                                            FALSE, core);
		lo_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 3, clip_gain[3],
		                                                       lo_trtx,
		                                                       lo_lna1byp_core, core);

		/* NB_CLIP */
		md_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, mid_gain1, md_trtx,
		                                                    md_lna1byp, core);
		hi_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, hi_gain1, hi_trtx,
		                                                      FALSE, core);
		lo_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, lo_gain1, lo_trtx,
		                                                    lo_lna1byp_core, core);
		md_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, mid_gain1, md_trtx,
		                                                      md_lna1byp, core);

		w1clip_pwrdBm = (lo_low_end + md_high_end) >> 1;

		/* -1 times pwr to avoid rounding off error */
		if (CHSPEC_IS20(pi->radio_chanspec)) {
			/*
			 * use mid-point, as late clip2 is causing minor humps,
			 * move nb/w1 clip to a lower pwr
			 */
			nbclip_pwrdBm = (md_low_end + hi_high_end) >> 1;
		} else {
			/*
			 * (0.4*md/lo + 0.6*hi/md) fixed point. On fading channels,
			 * low-end sensitivity degrades, keep more margin there.
			 */
			nbclip_pwrdBm = (((-2 * md_low_end) + (-3 * hi_high_end)) * 13) >> 6;
			nbclip_pwrdBm = -nbclip_pwrdBm;
		}

		wlc_phy_rxgainctrl_nbclip_acphy_tiny(pi, core, nbclip_pwrdBm);
		wlc_phy_rxgainctrl_w1clip_acphy(pi, core, w1clip_pwrdBm);
	}

	/* Saving the values of Init gain to be used
	 * in "wlc_phy_get_rxgain_acphy" function (used in rxiqest)
	 */
	pi_ac->initGain_codeA = READ_PHYREGC(pi, InitGainCodeA, 0);
	pi_ac->initGain_codeB = READ_PHYREGC(pi, InitGainCodeB, 0);
}

void
wlc_phy_rxgainctrl_gainctrl_acphy_28nm_ulp(phy_info_t *pi)
{
	/* at present this is just a place holder for
	 * 'static' ELNA configuration. Eventually both TCL and
	 * driver should be changes to follow the auto-calc
	 * routine used in wlc_phy_rxgainctrl_gainctrl_acphy
	 */
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;
	bool elna_present = (CHSPEC_IS2G(pi->radio_chanspec)) ? BF_ELNA_2G(pi_ac)
	                                                      : BF_ELNA_5G(pi_ac);
	uint8 max_analog_gain;
	uint8 i, core;
	bool init_trtx, hi_trtx, md_trtx, lo_trtx, lna1byp;
	int8 md_low_end, hi_high_end, lo_low_end, md_high_end;
	int8 clip2_gain;
	int8 hi_gain1, mid_gain1, lo_gain1;
	int8 nbclip_pwrdBm, w1clip_pwrdBm;
	int8 clip_gain[4];

	pi_ac->mdgain_trtx_allowed = FALSE;

	/* Copy clip gains from params */
	if (ACPHY_LO_NF_MODE_ELNA_28NM(pi)) {
		memcpy(clip_gain, rxg_params->fast_agc_lonf_clip_gains, ARRAYSIZE(clip_gain));
	} else {
		memcpy(clip_gain, rxg_params->fast_agc_clip_gains, ARRAYSIZE(clip_gain));
	}

	MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);

	max_analog_gain = READ_PHYREGFLD(pi, Core0HpFBw, maxAnalogGaindb);
	/* fill in gain limits for analog stages */
	for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++) {
		if (rxg_params->maxgain[i] == 0) {
			pi_ac->rxgainctrl_maxout_gains[i] = max_analog_gain;
		} else {
			pi_ac->rxgainctrl_maxout_gains[i] = rxg_params->maxgain[i];
		}
	}

	/* Keep track of it (used in interf_mitigation) */
	pi_ac->curr_desense->elna_bypass = wlc_phy_get_max_lna_index_acphy(pi, ELNA_ID);
	init_trtx = elna_present & pi_ac->curr_desense->elna_bypass;
	hi_trtx = elna_present & pi_ac->curr_desense->elna_bypass;
	md_trtx = elna_present & (pi_ac->mdgain_trtx_allowed | pi_ac->curr_desense->elna_bypass);
	lo_trtx = elna_present;

	/* This needs to be checkde */
	if (!elna_present || md_trtx) {
	 /* Add relevant changes later */
	} else {
	 /* Add relevant changes later */
	}

	if (ACPHY_ENABLE_FCBS_HWACI(pi)) {
		/* Apply desense */
		for (i = 0; i < 4; i++) {
			clip_gain[i] -= pi_ac->curr_desense->clipgain_desense[i];
		}
	}

	/* with elna if md_gain uses TR != T, then LO_gain needs to be higher */
	clip2_gain = md_trtx ? clip_gain[3] : (clip_gain[2] + clip_gain[3]) >> 1;

	FOREACH_CORE(pi, core) {
		lna1byp = pi_ac->rxgcrsi->fem_rxgains[core].lna1byp;

		/* 0,1,2,3 for Init, hi, md and lo gains respectively */
		wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, INIT_GAIN, clip_gain[0], init_trtx,
		                                            FALSE, core);

		hi_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, HIGH_GAIN, clip_gain[1],
		                                                       hi_trtx, FALSE, core);

		mid_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, MID_GAIN, clip_gain[2],
		                                                        md_trtx, FALSE, core);

		wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, CLIP2_GAIN, clip2_gain, md_trtx,
		                                            FALSE, core);

		lo_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, LOW_GAIN, clip_gain[3],
		                                                       lo_trtx, lna1byp, core);

		/* NB_CLIP */
		md_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, mid_gain1, md_trtx, 0,
				core);
		hi_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, hi_gain1, hi_trtx, 0,
				core);
		lo_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, lo_gain1, lo_trtx, 0,
				core);
		md_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, mid_gain1, md_trtx, 0,
				core);

		w1clip_pwrdBm = (lo_low_end + md_high_end) >> 1;
		/* -1 times pwr to avoid rounding off error */
		if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
			/*
			 * use mid-point, as late clip2 is causing minor humps,
			 * move nb/w1 clip to a lower pwr
			 */
			nbclip_pwrdBm = (md_low_end + hi_high_end) >> 1;
		} else {
			/*
			 * (0.4*md/lo + 0.6*hi/md) fixed point. On fading channels,
			 * low-end sensitivity degrades, keep more margin there.
			 */
			nbclip_pwrdBm = (((-2 * md_low_end) + (-3 * hi_high_end)) * 13) >> 6;
			nbclip_pwrdBm = -nbclip_pwrdBm;
		}

		wlc_phy_rxgainctrl_nbclip_acphy_28nm_ulp(pi, core, nbclip_pwrdBm);
		wlc_phy_rxgainctrl_w1clip_acphy_28nm_ulp(pi, core, w1clip_pwrdBm);
	}

	/* Saving the values of Init gain to be used
	 * in "wlc_phy_get_rxgain_acphy" function (used in rxiqest)
	 */
	pi_ac->initGain_codeA = READ_PHYREGC(pi, InitGainCodeA, 0);
	pi_ac->initGain_codeB = READ_PHYREGC(pi, InitGainCodeB, 0);
}


uint8
wlc_phy_get_max_lna_index_acphy(phy_info_t *pi, uint8 lna)
{
	uint8 max_idx, desense_state;
	acphy_desense_values_t *desense = NULL;
	uint8 elna_bypass, lna1_backoff, lna2_backoff;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;

	desense = pi_ac->total_desense;
	elna_bypass = desense->elna_bypass;
	lna1_backoff = desense->lna1_tbl_desense;
	lna2_backoff = desense->lna2_tbl_desense;

	if (pi_ac->aci != NULL) {
		desense_state = pi_ac->aci->hwaci_desense_state;
	} else {
		desense_state = 0;
	}

	/* Find default max_idx */
	if (lna == ELNA_ID) {
		max_idx = elna_bypass;       /* elna */
	} else if (lna == LNA1_ID) {               /* lna1 */
		if (IS_28NM_RADIO(pi)) {
			max_idx = ACPHY_LO_NF_MODE_ELNA_28NM(pi) ? rxg_params->lna1_lonf_max_indx :
					rxg_params->lna1_max_indx;
			max_idx = MAX(0, max_idx  - lna1_backoff);
		} else if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) {
			max_idx = (desense_state != 0) ? 
					desense->lna1_idx_max: ACPHY_4365_MAX_LNA1_IDX;
			max_idx = MAX(0, max_idx - lna1_backoff);
		} else {
			max_idx = MAX(0, ACPHY_MAX_LNA1_IDX  - lna1_backoff);
		}
	} else {                             /* lna2 */
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			if (BF_ELNA_2G(pi_ac) && (elna_bypass == 0)) {
				uint8 core = 0;
				if (pi_ac->sromi->femrx_2g[core].elna > 10) {
					max_idx = ACPHY_ELNA2G_MAX_LNA2_IDX;
				} else if (pi_ac->sromi->femrx_2g[core].elna > 8) {
					max_idx = ACPHY_ELNA2G_MAX_LNA2_IDX_L;
				} else {
					max_idx = ACPHY_ILNA2G_MAX_LNA2_IDX;
				}
			} else {
				max_idx = ACPHY_ILNA2G_MAX_LNA2_IDX;
			}
		} else {
			max_idx = (BF_ELNA_5G(pi_ac) && (elna_bypass == 0)) ?
			        ACPHY_ELNA5G_MAX_LNA2_IDX : ACPHY_ILNA5G_MAX_LNA2_IDX;
		}

		if (ACMAJORREV_4(pi->pubpi.phy_rev)) {
			/* Fix max lna2 idx for 4349 to 3 */
			max_idx = ACPHY_20693_MAX_LNA2_IDX;
		} else if (IS_28NM_RADIO(pi)) {
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				max_idx = ACPHY_LO_NF_MODE_ELNA_28NM(pi) ?
					rxg_params->lna2_lonf_force_indx_2g :
					rxg_params->lna2_2g_max_indx;
			} else {
				max_idx = ACPHY_LO_NF_MODE_ELNA_28NM(pi) ?
					rxg_params->lna2_lonf_force_indx_5g :
					rxg_params->lna2_5g_max_indx;
			}
		} else if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) {
			/* Fix max lna2 idx for 4365 to 1 */
			max_idx = desense_state != 0 ? desense->lna2_idx_max:
				ACPHY_4365_MAX_LNA2_IDX;
		}

		max_idx = MAX(0, max_idx - lna2_backoff);
	}

	return max_idx;
}

#ifndef WLC_DISABLE_ACI

/*********** Desnese (geneal) ********** */
/* IMP NOTE: make sure whatever regs are chagned here are either:
1. reset back to defualt below OR
2. Updated in gainctrl()
*/
void
wlc_phy_desense_apply_acphy(phy_info_t *pi, bool apply_desense)
{
	/* reset:
	   1 --> clear aci settings (the ones that gainctrl does not clear)
	   0 --> apply aci_noise_bt mitigation
	*/

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	acphy_desense_values_t *desense, *curr_desense;
	uint8 ofdm_desense, bphy_desense, initgain_desense;
	uint8 crsmin_thresh, crsmin_init;
	int8 crsmin_high;
	uint16 digigainlimit;
	uint8 bphy_minshiftbits[] = {0x77, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04,
	     0x04, 0x04, 0x04};
	uint16 bphy_peakenergy[]  = {0x10, 0x60, 0x10, 0x4c, 0x60, 0x30, 0x40, 0x40, 0x38, 0x2e,
	     0x40, 0x34, 0x40};
	uint8 bphy_initgain_backoff[] = {0, 0, 0, 0, 0, 0, 0, 3, 6, 9, 9, 12, 12};
	uint8 max_bphy_shiftbits = sizeof(bphy_minshiftbits) / sizeof(uint8);

	uint8 max_initgain_desense = 12, max_anlg_desense = 9 ;   /* only desnese bq0 */
	uint8 core, bphy_idx = 0;
	int8 zeros[] = {0, 0, 0, 0};

	bool call_gainctrl = FALSE;
	uint8 init_gain;

	if (TINY_RADIO(pi)) {
		if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) {
			init_gain = CHSPEC_IS2G(pi->radio_chanspec) ?
			        ACPHY_INIT_GAIN_4365_2G : ACPHY_INIT_GAIN_4365_5G;
		} else {
			init_gain = ACPHY_INIT_GAIN_TINY;
		}
	} else {
		init_gain = ACPHY_INIT_GAIN;
	}

	desense = pi_ac->total_desense;
	curr_desense = pi_ac->curr_desense;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);

	if (!apply_desense && !desense->forced) {
		/* when channel is changed, and the current channel is in mitigatation, then
		   we need to restore the values. wlc_phy_rxgainctrl_gainctrl_acphy() takes
		   care of all the gainctrl part, but we need to still restore back bphy regs
		*/

		wlc_phy_set_crs_min_pwr_higain_acphy(pi, ACPHY_CRSMIN_GAINHI);

		wlc_phy_desense_mf_high_thresh_acphy(pi, FALSE);
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			digigainlimit = READ_PHYREG(pi, DigiGainLimit0) & 0x8000;
			WRITE_PHYREG(pi, DigiGainLimit0, digigainlimit | 0x4477);
			WRITE_PHYREG(pi, PeakEnergyL, 0x10);
		}

		wlc_phy_desense_print_phyregs_acphy(pi, "restore");
		/* turn on LESI */
		wlc_phy_lesi_acphy(pi, TRUE);

		/* channal update reset for default */
		if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) {
		  wlc_phy_mlua_adjust_acphy(pi, FALSE);
		}
	} else {
		/* Get total desense based on aci & bt & lte */
		wlc_phy_desense_calc_total_acphy(pi);

		bphy_desense = MIN(ACPHY_ACI_MAX_DESENSE_BPHY_DB, desense->bphy_desense);
		ofdm_desense = MIN(ACPHY_ACI_MAX_DESENSE_OFDM_DB, desense->ofdm_desense);
		if (pi_ac->lesi == 1) {
		  if (ofdm_desense > 0 || bphy_desense > 12) {
		    wlc_phy_lesi_acphy(pi, FALSE);
		  } else {
		    wlc_phy_lesi_acphy(pi, TRUE);
		  }
		}

		/* channal update set to interference mode */
		if (ofdm_desense > 0) {
		  if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) {
		    wlc_phy_mlua_adjust_acphy(pi, TRUE);
		  }
		} else {
		  if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) {
		    wlc_phy_mlua_adjust_acphy(pi, FALSE);
		  }

		}

		/* Update total desense */
		if (ACMAJORREV_4(pi->pubpi.phy_rev)) {
			desense->ofdm_desense = ofdm_desense;
			desense->bphy_desense = bphy_desense;
		}

		/* Initgain can change due to bphy desense */
		if ((ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) &&
		    (curr_desense->bphy_desense != bphy_desense))
			call_gainctrl = TRUE;

		/* Update current desense */
		curr_desense->ofdm_desense = ofdm_desense;
		curr_desense->bphy_desense = bphy_desense;

		/* if any ofdm desense is needed, first start using higher
		   mf thresholds (1dB sens loss)
		*/
		wlc_phy_desense_mf_high_thresh_acphy(pi, (ofdm_desense > 0));
		if (ofdm_desense > 0)
			ofdm_desense -= 1;

		/* Distribute desense between INITgain & crsmin(ofdm) & digigain(bphy) */
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			/* round to 2, as bphy desnese table is in 2dB steps */
			bphy_idx = MIN((bphy_desense + 1) >> 1, max_bphy_shiftbits - 1);
			initgain_desense = bphy_initgain_backoff[bphy_idx];
		} else {
			initgain_desense = 0;
		}

#ifdef BCMLTECOEX
		if (pi_ac->ltecx_mode == 1)
			initgain_desense = ofdm_desense;
		else
#endif
		initgain_desense = MIN(initgain_desense, max_initgain_desense);
		desense->analog_gain_desense_ofdm = MAX(desense->analog_gain_desense_ofdm,
			MIN(max_anlg_desense, ofdm_desense));

		if (ACPHY_HWACI_WITH_DESENSE_ENG(pi) && CHSPEC_IS2G(pi->radio_chanspec)) {
			desense->clipgain_desense[0] = MAX(desense->clipgain_desense[0],
				initgain_desense);
			if (curr_desense->clipgain_desense[0] != desense->clipgain_desense[0])
			{
				curr_desense->clipgain_desense[0] =
					desense->clipgain_desense[0];
				call_gainctrl = TRUE;
			}
		}

		/* OFDM Desense */
		/* With init gain, max crsmin desense = 30dB, after which ADC will start clipping */
		/* With HI gain, crsmin desense = new_sens - old_sens
		   = (-96 + desense) - (0 - (higain + 27))
		   = -69 + desense + higain
		*/

		/*  In TINY, clipping happens close to -60 dBm. SO Max desense of crsmin_init
		 *  should be increased from 30 to 35 dB.
		 *
		 *  crsmin_high is changed to take care of any change in default HI or INIT gain.
		*/
		if (ACMAJORREV_4(pi->pubpi.phy_rev) || ACMAJORREV_32(pi->pubpi.phy_rev) ||
			ACMAJORREV_33(pi->pubpi.phy_rev)) {
			crsmin_init = MIN(35, MAX(0, ofdm_desense - desense->clipgain_desense[0]));
			crsmin_high = ofdm_desense + ACPHY_HI_GAIN_TINY -
			        init_gain - desense->clipgain_desense[1];
		} else {
			crsmin_init = MIN(30, MAX(0, ofdm_desense - initgain_desense));
			crsmin_high = ofdm_desense + ACPHY_HI_GAIN - 69;
		}
		PHY_ACI(("aci_mode1, desense, init = %d, bphy_idx = %d, crsmin = {%d %d}\n",
		         initgain_desense, bphy_idx, crsmin_init, crsmin_high));

		if ((curr_desense->elna_bypass != desense->elna_bypass) ||
		    (curr_desense->lna1_gainlmt_desense != desense->lna1_gainlmt_desense) ||
		    (curr_desense->lna1_tbl_desense != desense->lna1_tbl_desense) ||
		    (curr_desense->lna2_tbl_desense != desense->lna2_tbl_desense) ||
		    (curr_desense->lna2_gainlmt_desense != desense->lna2_gainlmt_desense) ||
		    (curr_desense->mixer_setting_desense != desense->mixer_setting_desense) ||
		    (curr_desense->analog_gain_desense_ofdm != desense->analog_gain_desense_ofdm) ||
		    (curr_desense->analog_gain_desense_bphy != desense->analog_gain_desense_bphy)) {
			if (ACPHY_ENABLE_FCBS_HWACI(pi)) {
				/* Update the current structure to hold the new desense values */
				memcpy(curr_desense, desense, sizeof(acphy_desense_values_t));
				wlc_phy_rxgainctrl_set_gaintbls_acphy(pi, TRUE, TRUE, TRUE);
			} else {
				wlc_phy_upd_lna1_lna2_gains_acphy(pi);
			}
			call_gainctrl = TRUE;
		}

		/* if lna1/lna2 gaintable has changed, call gainctrl as it effects all clip gains */
		if (call_gainctrl) {
			if (TINY_RADIO(pi)) {
				if (ACMAJORREV_32(pi->pubpi.phy_rev) ||
					ACMAJORREV_33(pi->pubpi.phy_rev))
					wlc_phy_rxgainctrl_gainctrl_acphy_tiny(pi,
					                                       initgain_desense);
				else
					wlc_phy_rxgainctrl_gainctrl_acphy_tiny(pi, 0);
			} else if (IS_28NM_RADIO(pi)) {
				if (pi_ac->rxgcrsi->rxg_params->mclip_agc_en) {
					wlc_phy_mclip_agc(pi, CCT_INIT(pi_ac), CCT_BAND_CHG(pi_ac),
						CCT_BW_CHG(pi_ac));
				} else {
					PHY_ERROR(("%s: Fast AGC needed for ACMAJORREV_25_40\n",
						__FUNCTION__));
					return;
				}
			} else {
				wlc_phy_rxgainctrl_gainctrl_acphy(pi);
			}
		}

		if (!TINY_RADIO(pi)) {
			/* Update INITgain */
			FOREACH_CORE(pi, core) {
				wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 0,
				                                            ACPHY_INIT_GAIN
				                                            - initgain_desense,
				                                            desense->elna_bypass,
				                                            FALSE, core);
			}
		}

		/* adjust crsmin threshold, 8 ticks increase gives 3dB rejection */
		crsmin_thresh = ACPHY_CRSMIN_DEFAULT + ((88 * crsmin_init) >> 5);  /* init gain */

#ifdef BCMLTECOEX
		if (pi_ac->ltecx_mode == 1)
			wlc_phy_set_crs_min_pwr_acphy(pi,
				MAX(crsmin_thresh, ACPHY_CRSMIN_DEFAULT), zeros);
		else
#endif
		wlc_phy_set_crs_min_pwr_acphy(pi,
			MAX(crsmin_thresh, pi->u.pi_acphy->phy_crs_th_from_crs_cal), zeros);

		/* crs high_gain */
		crsmin_thresh = MAX(ACPHY_CRSMIN_GAINHI,
		                    ACPHY_CRSMIN_DEFAULT + ((88 * crsmin_high) >> 5));
		wlc_phy_set_crs_min_pwr_higain_acphy(pi, crsmin_thresh);

		/* bphy desense */
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			digigainlimit = READ_PHYREG(pi, DigiGainLimit0) & 0x8000;
			WRITE_PHYREG(pi, DigiGainLimit0,
			             digigainlimit | 0x4400 | bphy_minshiftbits[bphy_idx]);
			WRITE_PHYREG(pi, PeakEnergyL, bphy_peakenergy[bphy_idx]);
		}
		wlc_phy_desense_print_phyregs_acphy(pi, "apply");
	}

	/* Inform rate contorl to slow down is mitigation is on */
	wlc_phy_aci_updsts_acphy(pi);

	wlapi_enable_mac(pi->sh->physhim);
}

void
wlc_phy_desense_calc_total_acphy(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

#ifdef BCMLTECOEX
	acphy_desense_values_t *bt, *aci, *lte;
#else
	acphy_desense_values_t *bt, *aci;
#endif
	uint8 i;

	acphy_desense_values_t *total = pi_ac->total_desense;

	aci = (pi_ac->aci == NULL) ? pi_ac->zero_desense : &pi_ac->aci->desense;

	/* if desense is forced, then skip without calculation and just use the forced value */
	if (total->forced)
		return;

#ifdef BCMLTECOEX
	if ((pi_ac->btc_mode == 0 && pi_ac->ltecx_mode == 0) || wlc_phy_is_scan_chan_acphy(pi) ||
	    CHSPEC_IS5G(pi->radio_chanspec) || ACMAJORREV_32(pi->pubpi.phy_rev) ||
	    ACMAJORREV_33(pi->pubpi.phy_rev)) {
		/* only consider aci desense */
		memcpy(total, aci, sizeof(acphy_desense_values_t));
	} else {
		/* Merge BT & ACI & LTE desense, take max */
		bt  = pi_ac->bt_desense;
		lte = pi_ac->rxgcrsi->lte_desense;
		total->ofdm_desense = MAX(MAX(aci->ofdm_desense, bt->ofdm_desense),
			lte->ofdm_desense);
		total->bphy_desense = MAX(MAX(aci->bphy_desense, bt->bphy_desense),
			lte->bphy_desense);
		total->lna1_tbl_desense = MAX(MAX(aci->lna1_tbl_desense, bt->lna1_tbl_desense),
			lte->lna1_tbl_desense);
		total->analog_gain_desense_ofdm = MAX(MAX(aci->analog_gain_desense_ofdm,
			bt->analog_gain_desense_ofdm), lte->analog_gain_desense_ofdm);
		total->analog_gain_desense_bphy = MAX(MAX(aci->analog_gain_desense_bphy,
			bt->analog_gain_desense_bphy), lte->analog_gain_desense_bphy);
		total->lna2_tbl_desense = MAX(MAX(aci->lna2_tbl_desense, bt->lna2_tbl_desense),
			lte->lna2_tbl_desense);
		total->lna1_gainlmt_desense = MAX(MAX(aci->lna1_gainlmt_desense,
			bt->lna1_gainlmt_desense), lte->lna1_gainlmt_desense);
		total->lna2_gainlmt_desense = MAX(MAX(aci->lna2_gainlmt_desense,
			bt->lna2_gainlmt_desense), lte->lna2_gainlmt_desense);
		total->elna_bypass = MAX(MAX(aci->elna_bypass, bt->elna_bypass),
			lte->elna_bypass);
		total->mixer_setting_desense = MAX(MAX(aci->mixer_setting_desense,
			bt->mixer_setting_desense), lte->mixer_setting_desense);
		total->nf_hit_lna12 =  MAX(MAX(aci->nf_hit_lna12, bt->nf_hit_lna12),
			lte->nf_hit_lna12);
		total->on = aci->on | bt->on | lte->on;
		for (i = 0; i < 4; i++)
			total->clipgain_desense[i] = MAX(MAX(aci->clipgain_desense[i],
				bt->clipgain_desense[i]), lte->clipgain_desense[i]);
	}
#else
	if ((pi_ac->btc_mode == 0) || wlc_phy_is_scan_chan_acphy(pi) ||
	    CHSPEC_IS5G(pi->radio_chanspec) || ACMAJORREV_32(pi->pubpi.phy_rev) ||
	    ACMAJORREV_33(pi->pubpi.phy_rev)) {
		/* only consider aci desense */
		memcpy(total, aci, sizeof(acphy_desense_values_t));
	} else {
		/* Merge BT & ACI desense, take max */
		bt  = pi_ac->bt_desense;
		total->ofdm_desense = MAX(aci->ofdm_desense, bt->ofdm_desense);
		total->bphy_desense = MAX(aci->bphy_desense, bt->bphy_desense);
		total->analog_gain_desense_ofdm = MAX(aci->analog_gain_desense_ofdm,
			bt->analog_gain_desense_ofdm);
		total->analog_gain_desense_bphy = MAX(aci->analog_gain_desense_bphy,
			bt->analog_gain_desense_bphy);
		total->lna1_tbl_desense = MAX(aci->lna1_tbl_desense, bt->lna1_tbl_desense);
		total->lna2_tbl_desense = MAX(aci->lna2_tbl_desense, bt->lna2_tbl_desense);
		total->lna1_gainlmt_desense =
		  MAX(aci->lna1_gainlmt_desense, bt->lna1_gainlmt_desense);
		total->lna2_gainlmt_desense =
		  MAX(aci->lna2_gainlmt_desense, bt->lna2_gainlmt_desense);
		total->mixer_setting_desense =
		  MAX(aci->mixer_setting_desense, bt->mixer_setting_desense);
		total->elna_bypass = MAX(aci->elna_bypass, bt->elna_bypass);
		total->nf_hit_lna12 =  MAX(aci->nf_hit_lna12, bt->nf_hit_lna12);
		total->on = aci->on | bt->on;
		for (i = 0; i < 4; i++)
			total->clipgain_desense[i] = MAX(aci->clipgain_desense[i],
				bt->clipgain_desense[i]);
	}
#endif /* BCMLTECOEX */

	/* uCode detected high pwr RSSI. Time to save ilna1 */
	if (CHSPEC_IS2G(pi->radio_chanspec) && (pi_ac->hirssi_timer2g > PHY_SW_HIRSSI_OFF))
		total->elna_bypass = TRUE;
	if (CHSPEC_IS5G(pi->radio_chanspec) && (pi_ac->hirssi_timer5g > PHY_SW_HIRSSI_OFF))
		total->elna_bypass = TRUE;
}

/********** Desnese BT  ******** */
void
wlc_phy_desense_btcoex_acphy(phy_info_t *pi, int32 mode)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	acphy_desense_values_t *desense = pi_ac->bt_desense;
	int32 old_mode = pi_ac->btc_mode;

	if (ACPHY_ENABLE_FCBS_HWACI(pi) && !ACPHY_HWACI_WITH_DESENSE_ENG(pi))
		return;

	/* Start with everything at 0 */
	bzero(desense, sizeof(acphy_desense_values_t));
	pi_ac->btc_mode = mode;
	desense->on = (mode > 0);

	switch (mode) {
	case 1: /* BT power =  -30dBm, -35dBm */
		desense->lna1_gainlmt_desense = 1;   /* 4 */
		desense->lna2_gainlmt_desense = 3;   /* 3 */
		desense->elna_bypass = 0;
		break;
	case 2: /* BT power = -20dBm , -25dB */
		desense->lna1_gainlmt_desense = 0;   /* 5 */
		desense->lna2_gainlmt_desense = 0;   /* 6 */
		desense->elna_bypass = 1;
		break;
	case 3: /* BT power = -15dBm */
		desense->lna1_gainlmt_desense = 0;   /* 5 */
		desense->lna2_gainlmt_desense = 2;   /* 4 */
		desense->elna_bypass = 1;
		desense->nf_hit_lna12 = 2;
		break;
	case 4: /* BT power = -10dBm */
		desense->lna1_gainlmt_desense = 1;   /* 4 */
		desense->lna2_gainlmt_desense = 2;   /* 4 */
		desense->elna_bypass = 1;
		desense->nf_hit_lna12 = 3;
		break;
	case 5: /* BT power = -5dBm */
		desense->lna1_gainlmt_desense = 3;   /* 2 */
		desense->lna2_gainlmt_desense = 0;   /* 6 */
		desense->elna_bypass = 1;
		desense->nf_hit_lna12 = 13;
		break;
	case 6: /* BT power = 0dBm */
		desense->lna1_gainlmt_desense = 3;   /* 2 */
		desense->lna2_gainlmt_desense = 4;   /* 2 */
		desense->elna_bypass = 1;
		desense->nf_hit_lna12 = 24;
		break;
	case 7: /* Case added for 4359 */
		desense->lna1_tbl_desense = 3;	/* 1 */
		desense->mixer_setting_desense = 7; /* Value should be take between 2-11 for Tiny */
		break;

	default:
		break;
	}

	/* Apply these settings if this is called while on an active 2g channel */
	if (CHSPEC_IS2G(pi->radio_chanspec) && !SCAN_RM_IN_PROGRESS(pi)) {
		/* If bt desense changed, then reset aci params. But, keep the aci settings intact
		   if bt is switched off (as you will still need aci desense)
		*/
		if ((mode != old_mode) && (mode > 0))
			wlc_phy_desense_aci_reset_params_acphy(pi, FALSE, FALSE, FALSE);
		wlc_phy_desense_apply_acphy(pi, TRUE);
	}
}

/********** Desense LTE  ******** */
#ifdef BCMLTECOEX
void
wlc_phy_desense_ltecx_acphy(phy_info_t *pi, int32 mode)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	acphy_desense_values_t *desense = pi_ac->rxgcrsi->lte_desense;

	/* Start with everything at 0 */
	bzero(desense, sizeof(acphy_desense_values_t));
	pi_ac->ltecx_mode = mode;
	if (pi_ac->ltecx_mode == 0) {
			pi->u.pi_acphy->ltecx_elna_bypass_status = 0;
		}
	desense->on = (mode > 0);
	switch (mode) {
	case 1: /* LTE - Add new cases in the future */
		desense->ofdm_desense = 24;
		desense->bphy_desense = 24;
		desense->elna_bypass = 1;
		desense->nf_hit_lna12 = 24;
		pi->u.pi_acphy->ltecx_elna_bypass_status = 1;
		break;
	default:
		break;
	}

	/* Apply these settings if this is called while on an active 2g channel */
	if (CHSPEC_IS2G(pi->radio_chanspec) && !SCAN_RM_IN_PROGRESS(pi))
		wlc_phy_desense_apply_acphy(pi, TRUE);
}
#endif /* BCMLTECOEX */

#endif /* #ifndef WLC_DISABLE_ACI */

void
wlc_phy_rxgainctrl_gainctrl_acphy(phy_info_t *pi)
{
	bool elna_present;
	bool init_trtx, hi_trtx, md_trtx, lo_trtx, lna1byp;
	uint8 init_gain = ACPHY_INIT_GAIN, hi_gain = ACPHY_HI_GAIN;
	uint8 mid_gain = 35, lo_gain, clip2_gain;
	uint8 hi_gain1, mid_gain1, lo_gain1;

	/* 1% PER point used for all the PER numbers */

	/* For bACI/ACI: max output pwrs {elna, lna1, lna2, mix, bq0, bq1, dvga} */
	uint8 maxgain_2g[] = {43, 43, 43, 52, 52, 100, 0};
	uint8 maxgain_5g[] = {47, 47, 47, 52, 52, 100, 0};

	uint8 i, core, elna_idx;
	int8 md_low_end, hi_high_end, lo_low_end, md_high_end, max_himd_hi_end;
	int8 nbclip_pwrdBm, w1clip_pwrdBm;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	bool elnabyp_en = (CHSPEC_IS2G(pi->radio_chanspec) & pi_ac->hirssi_elnabyp2g_en) ||
	        (CHSPEC_IS5G(pi->radio_chanspec) & pi_ac->hirssi_elnabyp5g_en);

	pi_ac->mdgain_trtx_allowed = TRUE;
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
			pi_ac->rxgainctrl_maxout_gains[i] = maxgain_2g[i];
		elna_present = BF_ELNA_2G(pi_ac);
	} else {
		for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
			pi_ac->rxgainctrl_maxout_gains[i] = maxgain_5g[i];
		elna_present = BF_ELNA_5G(pi_ac);
	}

	/* Keep track of it (used in interf_mitigation) */
	pi_ac->curr_desense->elna_bypass = wlc_phy_get_max_lna_index_acphy(pi, ELNA_ID);
	init_trtx = elna_present & pi_ac->curr_desense->elna_bypass;
	hi_trtx = elna_present & pi_ac->curr_desense->elna_bypass;
	md_trtx = elna_present & (pi_ac->mdgain_trtx_allowed | pi_ac->curr_desense->elna_bypass);
	lo_trtx = TRUE;

	/* with elna if md_gain uses TR != T, then LO_gain needs to be higher */
	if (elnabyp_en)
		lo_gain = ((!elna_present) || md_trtx) ? 15 : 30;
	else
		lo_gain = ((!elna_present) || md_trtx) ? 20 : 30;
	clip2_gain = md_trtx ? lo_gain : (mid_gain + lo_gain) >> 1;

	FOREACH_CORE(pi, core) {
		lna1byp = pi_ac->fem_rxgains[core].lna1byp;
		if (lna1byp) {
			mid_gain = 35;
			lo_gain = 15;
			PHY_INFORM((" iPa chip, set lo_gain = 15 \n"));
			hi_gain = 48;
		}
		elna_idx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
		max_himd_hi_end = - 16 - pi_ac->rxgainctrl_params[core].gaintbl[0][elna_idx];
		if (pi_ac->curr_desense->elna_bypass == 1)
			max_himd_hi_end += pi_ac->fem_rxgains[core].trloss;

		/* 0,1,2,3 for Init, hi, md and lo gains respectively */
		wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 0, init_gain, init_trtx,
			FALSE, core);
		hi_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 1, hi_gain,
			hi_trtx, FALSE, core);
		mid_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 2, mid_gain,
			md_trtx, FALSE, core);
		wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 4, clip2_gain, md_trtx,
			FALSE, core);
		lo_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 3, lo_gain,
			lo_trtx, lna1byp, core);

		/* NB_CLIP */
		md_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, mid_gain1, md_trtx,
		                                                    FALSE, core);
		hi_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, hi_gain1, hi_trtx,
		                                                      FALSE, core);
		lo_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, lo_gain1, lo_trtx,
		                                                    lna1byp, core);
		md_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, mid_gain1, md_trtx,
		                                                      FALSE, core);

		/* -1 times pwr to avoid rounding off error */
		if (CHSPEC_IS20(pi->radio_chanspec)) {
			/* use mid-point, as late clip2 is causing minor humps,
			   move nb/w1 clip to a lower pwr
			*/
			nbclip_pwrdBm = (md_low_end + hi_high_end) >> 1;
		} else {
			/* (0.4*md/lo + 0.6*hi/md) fixed point. On fading channels,
			   low-end sensitivity degrades, keep more margin there.
			*/
			nbclip_pwrdBm = (((-2*md_low_end)+(-3*hi_high_end)) * 13) >> 6;
			nbclip_pwrdBm *= -1;
			if (CHSPEC_IS80(pi->radio_chanspec) &&
			    ((ACMAJORREV_2(pi->pubpi.phy_rev) && ACMINORREV_1(pi)) ||
			     ACMAJORREV_5(pi->pubpi.phy_rev))) {
				/* solves the 20ll PER hump issue : JIRA SWWLAN-37167 */
				nbclip_pwrdBm -= 4;
			}
		}

		if (CHSPEC_IS20(pi->radio_chanspec) ||
		    (ACMAJORREV_1(pi->pubpi.phy_rev) && ACMINORREV_2(pi) && !PHY_ILNA(pi))) {
			if (elnabyp_en) {
				w1clip_pwrdBm = (((-2*lo_low_end) + (-3*md_high_end)) * 13) >> 6;
				w1clip_pwrdBm *= -1;
			} else {
				w1clip_pwrdBm = (lo_low_end + md_high_end) >> 1;
			}
		} else {
			w1clip_pwrdBm = (((-2*lo_low_end) + (-3*md_high_end)) * 13) >> 6;
			w1clip_pwrdBm *= -1;
		}

		wlc_phy_rxgainctrl_nbclip_acphy(pi, core, nbclip_pwrdBm);
		wlc_phy_rxgainctrl_w1clip_acphy(pi, core, w1clip_pwrdBm);

		/* 2G/5G VHT20 hump in eLNA, WAR for 43162 */
		if (ACMAJORREV_1(pi->pubpi.phy_rev) && ACMINORREV_2(pi) &&
		     !PHY_ILNA(pi) && CHSPEC_IS20(pi->radio_chanspec) && pi->xtalfreq == 40000000) {
			if (CHSPEC_IS5G(pi->radio_chanspec))
				nbclip_pwrdBm = ((md_low_end + hi_high_end) >> 1) - 5;
			else
				nbclip_pwrdBm = ((md_low_end + hi_high_end) >> 1) - 4;

			wlc_phy_rxgainctrl_nbclip_acphy(pi, core, nbclip_pwrdBm);
		}
	}
	/* After comming out of this loop, gain ctrl is done. So, values of Init gain in the phyregs
	 * are the correct/default ones. These should be the ones that should be used in
	 * "wlc_phy_get_rxgain_acphy" function (used in rxiqest). So, we have to save the default
	 * Init gain phyregs in some variables. We assume, that both core's init gains will be same
	 * and thus save only core 0's init gain and this should be fine for single core chips too.
	 */
	pi_ac->initGain_codeA = READ_PHYREGC(pi, InitGainCodeA, 0);
	pi_ac->initGain_codeB = READ_PHYREGC(pi, InitGainCodeB, 0);
}

void
wlc_phy_upd_lna1_lna2_gains_acphy(phy_info_t *pi)
{
	wlc_phy_upd_lna1_lna2_gainlimittbls_acphy(pi, 1);
	wlc_phy_upd_lna1_lna2_gainlimittbls_acphy(pi, 2);
	wlc_phy_upd_lna1_lna2_gaintbls_acphy(pi, 1);
	wlc_phy_upd_lna1_lna2_gaintbls_acphy(pi, 2);
}

void
wlc_phy_upd_lna1_lna2_gaintbls_acphy(phy_info_t *pi, uint8 lna12)
{
	uint8 offset, sz, core;
	uint8 gaintbl[10], gainbitstbl[10];
	uint8 max_idx, min_idx, desense_state;
	const uint8 *default_gaintbl = NULL;
	uint16 gain_tblid, gainbits_tblid;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	acphy_desense_values_t *desense = pi_ac->total_desense;
	phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;
	uint8 lna1Rout = 0x25;
	uint8 lna2Rout = 0x44;
	uint8 lna1rout_tbl[6], lna1rout_offset;
	uint8 lna1, lna1_idx, lna1_rout;
	uint8 stall_val;

	if ((lna12 < 1) || (lna12 > 2)) return;

	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	sz = pi_ac->rxgainctrl_stage_len[lna12];


	if (pi_ac->aci != NULL) {
		desense_state = pi_ac->aci->hwaci_desense_state;
	} else {
		desense_state = 0;
	}

	if (lna12 == LNA1_ID) {          /* lna1 */
		offset = 8;
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			if (IS_28NM_RADIO(pi)) {
				default_gaintbl = (uint8 *)(rxg_params->lna12_gain_2g_tbl[0]);
			} else	if (!TINY_RADIO(pi)) {
				if (PHY_ILNA(pi)) { /* iLNA only chip */
					default_gaintbl = ac_lna1_2g_43352_ilna;
				} else {
					if (BF3_LTECOEX_GAINTBL_EN(pi_ac) == 1 &&
					    ACMAJORREV_1(pi->pubpi.phy_rev)) {
						default_gaintbl = ac_lna1_2g_ltecoex;
					} else {
						default_gaintbl = ac_lna1_2g;
					}
				}
			} else {
				default_gaintbl = ac_lna1_2g_tiny;
			}
		} else if (IS_28NM_RADIO(pi)) {
			default_gaintbl = (uint8 *)(rxg_params->lna12_gain_5g_tbl[0]);
		} else {
			if (ACMAJORREV_32(pi->pubpi.phy_rev) ||
			    (ACMAJORREV_33(pi->pubpi.phy_rev) && !ACMINORREV_4(pi)))
				default_gaintbl = ac_lna1_5g_4365;
			else
				default_gaintbl = (TINY_RADIO(pi)) ? ac_lna1_5g_tiny : ac_lna1_5g;
		}
	} else {  /* lna2 */
		offset = 16;
		if (TINY_RADIO(pi)) {
			if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) {
				default_gaintbl = CHSPEC_IS2G(pi->radio_chanspec) ?
				        ac_lna2_tiny_4365_2g : ac_lna2_tiny_4365_5g;
			} else if (ACMAJORREV_4(pi->pubpi.phy_rev)) {
				default_gaintbl = ac_lna2_tiny_4349;
			} else if (ACMAJORREV_3(pi->pubpi.phy_rev) &&
				ACREV_GE(pi->pubpi.phy_rev, 11) &&
				PHY_ILNA(pi)) {
				default_gaintbl = ac_lna2_tiny_ilna_dcc_comp;
			} else {
				default_gaintbl = ac_lna2_tiny;
			}
		} else if (IS_28NM_RADIO(pi)) {
			default_gaintbl = (uint8 *)(CHSPEC_IS2G(pi->radio_chanspec) ?
					rxg_params->lna12_gain_2g_tbl[1] :
					rxg_params->lna12_gain_5g_tbl[1]);
		} else if (CHSPEC_IS2G(pi->radio_chanspec)) {
			if (PHY_ILNA(pi)) { /* iLNA only chip */
				default_gaintbl = ac_lna2_2g_43352_ilna;
			} else {
				if (BF3_LTECOEX_GAINTBL_EN(pi_ac) == 1 &&
				    ACMAJORREV_1(pi->pubpi.phy_rev)) {
					default_gaintbl = ac_lna2_2g_ltecoex;
				} else {
					default_gaintbl = (desense->elna_bypass)
						? ac_lna2_2g_gm3 : ac_lna2_2g_gm2;
				}
			}
		} else {
			default_gaintbl = ac_lna2_5g;
		}
		memcpy(pi_ac->lna2_complete_gaintbl, default_gaintbl, sizeof(uint8)*sz);
	}

	max_idx = wlc_phy_get_max_lna_index_acphy(pi, lna12);
	if ((ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) &&
	    TINY_RADIO(pi)) {
		if (desense_state != 0)
			min_idx = (lna12 == 1) ? desense->lna1_idx_min: desense->lna2_idx_min;
		else
			min_idx = (lna12 == 1) ? 0 : max_idx;
	} else if (ACMAJORREV_33(pi->pubpi.phy_rev) && ACMINORREV_4(pi)) {
		if (lna12 == LNA1_ID) {
			min_idx = rxg_params->lna1_min_indx;
		} else {
			// Fix LNA2
			min_idx = 3;
			max_idx = 3;
		}
	} else if (IS_28NM_RADIO(pi)) {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			min_idx = (lna12 == LNA1_ID) ? rxg_params->lna1_min_indx :
				(ACPHY_LO_NF_MODE_ELNA_28NM(pi) ?
					rxg_params->lna2_lonf_force_indx_2g :
					rxg_params->lna2_2g_min_indx);
		} else {
			min_idx = (lna12 == LNA1_ID) ? rxg_params->lna1_min_indx :
				(ACPHY_LO_NF_MODE_ELNA_28NM(pi) ?
					rxg_params->lna2_lonf_force_indx_5g :
					rxg_params->lna2_5g_min_indx);
		}
	} else {
		min_idx = ACMAJORREV_4(pi->pubpi.phy_rev) ? max_idx :
		        ((pi->pubpi.phy_rev == 0) ? 1 : 0);
	}

	wlc_phy_limit_rxgaintbl_acphy(gaintbl, gainbitstbl, sz, default_gaintbl, min_idx, max_idx);

	/* Update pi_ac->curr_desense (used in interf_mitigation) */
	if (lna12 == 1) {
		pi_ac->curr_desense->lna1_tbl_desense = desense->lna1_tbl_desense;
	} else {
		pi_ac->curr_desense->lna2_tbl_desense = desense->lna2_tbl_desense;
	}

	if (BF3_LTECOEX_GAINTBL_EN(pi_ac) == 1 && ACMAJORREV_1(pi->pubpi.phy_rev)) {
		/* for now changing rout of lna1 and lna2 to achieve */
		/* lower gain of 3dB for the init gain index only */
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, 5, 8, &lna1Rout);
		MOD_RADIO_REG(pi, RFX, OVR6, ovr_lna2g_lna1_Rout, 0);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, 20, 8, &lna2Rout);
		MOD_RADIO_REG(pi, RFX, OVR6, ovr_lna2g_lna2_Rout, 0);
	} else if (TINY_RADIO(pi)) {
		uint8 x;
		uint8 lnarout_val, lnarout_val2G, lnarout_val5G;

		if (ACMAJORREV_4(pi->pubpi.phy_rev) || ACMAJORREV_32(pi->pubpi.phy_rev) ||
			ACMAJORREV_33(pi->pubpi.phy_rev)) {
			FOREACH_CORE(pi, core) {
				for (x = 0; x < 6; x++) {
					lnarout_val2G = (ac_tiny_g_lna_rout_map[x] << 3) |
						ac_tiny_g_lna_gain_map[x];
					if (ACMAJORREV_32(pi->pubpi.phy_rev) ||
						ACMAJORREV_33(pi->pubpi.phy_rev))
						lnarout_val5G = (ac_4365_a_lna_rout_map[x] << 3) |
						        ac_tiny_a_lna_gain_map[x];
					else
						lnarout_val5G = (ac_tiny_a_lna_rout_map[x] << 3) |
						        ac_tiny_a_lna_gain_map[x];
					lnarout_val = CHSPEC_IS5G(pi->radio_chanspec) ?
						lnarout_val5G : lnarout_val2G;
					wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1,
						(ACPHY_LNAROUT_BAND_OFFSET(pi,
						pi->radio_chanspec)	+ x +
						ACPHY_LNAROUT_CORE_WRT_OFST(pi->pubpi.phy_rev,
						core)),	8, &lnarout_val);
				}

				if (ACMINORREV_0(pi) || ACMINORREV_1(pi))
					MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_lna2_gain,
						wlc_phy_get_max_lna_index_acphy(pi, LNA2_ID));
			}
		} else {
			/* 2G index is 0->5 */
			for (x = 0; x < 6; x++) {
				lnarout_val = (ac_tiny_g_lna_rout_map[x] << 3) |
					ac_tiny_g_lna_gain_map[x];
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, x, 8,
					&lnarout_val);
			}

			/* 5G index is 8->13 */
			for (x = 0; x < 6; x++) {
				lnarout_val =  (ac_tiny_a_lna_rout_map[x] << 3) |
					ac_tiny_a_lna_gain_map[x];
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, 8 + x, 8,
				                          &lnarout_val);
			}
		}
	} else if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
		uint8 i, idx;
		uint8 *lna2_gm_ind = CHSPEC_IS2G(pi->radio_chanspec) ?
				rxg_params->lna2_gm_ind_2g_tbl : rxg_params->lna2_gm_ind_5g_tbl;

		/* 2G index is 0->5 */
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 6, 0, 8,
				rxg_params->lna1Rout_2g_tbl);

		/* 5G index is 8->13 */
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 6, 8, 8,
				rxg_params->lna1Rout_5g_tbl);

		for (i = 0; i < 7; i++) {
			/* LNA2 gm indexes 16->22 */
			idx = ACPHY_LO_NF_MODE_ELNA_28NM(pi) ?
				(CHSPEC_IS2G(pi->radio_chanspec) ?
				rxg_params->lna2_lonf_force_indx_2g :
				rxg_params->lna2_lonf_force_indx_5g) : i;
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 7, (16 + i), 8,
					&lna2_gm_ind[idx]);
		}
	} else if (IS_28NM_RADIO(pi) && (lna12 == LNA1_ID)) {
		FOREACH_CORE(pi, core) {
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 6,
				(0 + ACPHY_LNAROUT_CORE_WRT_OFST(pi->pubpi.phy_rev, core)), 8,
				rxg_params->lna1Rout_2g_tbl);
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 6,
				(8 + ACPHY_LNAROUT_CORE_WRT_OFST(pi->pubpi.phy_rev, core)), 8,
				rxg_params->lna1Rout_5g_tbl);
		}
	}

	/* Update gaintbl */
	FOREACH_CORE(pi, core) {
		if (core == 0) {
			gain_tblid =  ACPHY_TBL_ID_GAIN0;
			gainbits_tblid =  ACPHY_TBL_ID_GAINBITS0;
		} else if (core == 1) {
			gain_tblid =  ACPHY_TBL_ID_GAIN1;
			gainbits_tblid =  ACPHY_TBL_ID_GAINBITS1;
		} else if (core == 2) {
			gain_tblid =  ACPHY_TBL_ID_GAIN2;
			gainbits_tblid =  ACPHY_TBL_ID_GAINBITS2;
		} else {
			gain_tblid =  ACPHY_TBL_ID_GAIN3;
			gainbits_tblid =  ACPHY_TBL_ID_GAINBITS3;
		}

		if (!TINY_RADIO(pi) && !IS_28NM_RADIO(pi)) {
			lna1rout_offset = core * 24;
			if (CHSPEC_IS5G(pi->radio_chanspec))
				lna1rout_offset += 8;

			if (lna12 == 1) {
				uint8 i;
				wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT, 6,
				                         lna1rout_offset, 8, lna1rout_tbl);
				for (i = 0; i < 6; i++) {
					lna1 = lna1rout_tbl[gainbitstbl[i]];
					lna1_idx = lna1 & 0x7;
					lna1_rout = (lna1 >> 3) & 0xf;
					gaintbl[i] = CHSPEC_IS2G(pi->radio_chanspec) ?
					        default_gaintbl[lna1_idx] -
					        ac_lna1_rout_delta_2g[lna1_rout]:
					        default_gaintbl[lna1_idx] -
					        ac_lna1_rout_delta_5g[lna1_rout];
				}
			}
		}

		memcpy(pi_ac->rxgainctrl_params[core].gaintbl[lna12], gaintbl, sizeof(uint8)*sz);
		wlc_phy_table_write_acphy(pi, gain_tblid, sz, offset, 8, gaintbl);
		memcpy(pi_ac->rxgainctrl_params[core].gainbitstbl[lna12], gainbitstbl,
			sizeof(uint8)*sz);
		wlc_phy_table_write_acphy(pi, gainbits_tblid, sz, offset, 8, gainbitstbl);
	}

	ACPHY_ENABLE_STALL(pi, stall_val);
}

void
wlc_phy_upd_lna1_lna2_gainlimittbls_acphy(phy_info_t *pi, uint8 lna12)
{
	uint8 i, sz, max_idx, core, lna2_limit_size, lna1_rout, lna1rout_tbl[5];
	uint8 max_idx_pktgain_limit, max_idx_non_init_lna1rout_tbl, lna1rout_offset;
	uint8 lna1_tbl[] = {11, 12, 14, 32, 36, 40};
	uint8 lna2_tbl[] = {0, 0, 0, 3, 3, 3, 3};
	uint8 lna2_gainlmt_tiny[] = {127, 127, 127, 127};
	uint8 tia_gainlmt_tiny[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 127};
	uint8 lna2_gainlmt_4365[] = {0, 0, 0, 0, 0, 0, 0};
	uint8 tia_gainlmt_4365[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127};
	uint8 *tiny_gainlmt_lna2, *tiny_gainlmt_tia;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	acphy_desense_values_t *desense = pi_ac->total_desense;
	uint16 gainlimitid;

	if (!TINY_RADIO(pi)) {
		sz = (lna12 == 1) ? 6 : 7;

		/* Limit based on desense mitigation mode */
		if (lna12 == 1) {
			max_idx = MAX(0, (sz - 1) - desense->lna1_gainlmt_desense);
			pi_ac->curr_desense->lna1_gainlmt_desense = desense->lna1_gainlmt_desense;

			max_idx_pktgain_limit = max_idx;
			/* if rout desense is active, we have to change the non-init gain entries of
			 * lna1_rout tbl and modify pkt gain limit tbl to not use init-gain instead
			 * of just limiting LNA1 gain in the pkt gain limit tbls
			 */
			max_idx_non_init_lna1rout_tbl = (desense->lna1rout_gainlmt_desense > 0) ?
			        max_idx : (sz - 2);

			/* LNA1 gain in pktgain limit tbl has to be capped
			   only to not use init gain
			*/
			max_idx_pktgain_limit = (desense->lna1rout_gainlmt_desense > 0) ?
			        (sz - 2) : max_idx;
			lna1_rout = CHSPEC_IS2G(pi->radio_chanspec) ?
			        0 + desense->lna1rout_gainlmt_desense :
			        4 - desense->lna1rout_gainlmt_desense;
			for (i = 0; i <= sz - 2; i++) {
				lna1rout_tbl[i] = (lna1_rout << 3) |
				        MAX(0, max_idx_non_init_lna1rout_tbl - (sz - 2 - i));
			}

			if (!(ACREV_IS(pi->pubpi.phy_rev, 0) || TINY_RADIO(pi))) {
				/* WRITE the lna1 rout table (only first 5 entries) */
				FOREACH_CORE(pi, core) {
					lna1rout_offset = core * 24;
					if (CHSPEC_IS5G(pi->radio_chanspec))
						lna1rout_offset += 8;
					wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, sz-1,
					                          lna1rout_offset, 8, lna1rout_tbl);
				}
			}
		} else {
			max_idx_pktgain_limit = MAX(0, (sz - 1) - desense->lna2_gainlmt_desense);
			pi_ac->curr_desense->lna2_gainlmt_desense = desense->lna2_gainlmt_desense;
		}

		/* Write 0x7f to entries not to be used */
		for (i = (max_idx_pktgain_limit + 1); i < sz; i++) {
			if (lna12 == 1) {
				lna1_tbl[i] = 0x7f;
			} else {
				lna2_tbl[i] = 0x7f;
			}
		}

		if (lna12 == 1) {
		    if (ACMAJORREV_5(pi->pubpi.phy_rev)) {
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT0, sz,
					8, 8,  lna1_tbl);
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT1, sz,
					8, 8,  lna1_tbl);
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT2, sz,
					8, 8,  lna1_tbl);
			} else
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT, sz,
					8, 8, lna1_tbl);

			/* 4335C0: This is for DSSS_CCK packet gain limit */
			if (ACMAJORREV_1(pi->pubpi.phy_rev) && ACMINORREV_2(pi))
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT, sz, 72, 8,
				                          lna1_tbl);
		}
		else {
			if (ACMAJORREV_5(pi->pubpi.phy_rev)) {
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT0, sz,
					16, 8, lna2_tbl);
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT1, sz,
					16, 8, lna2_tbl);
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT2, sz,
					16, 8, lna2_tbl);
			} else
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT, sz,
					16, 8, lna2_tbl);

			/* 4335C0: This is for DSSS_CCK packet gain limit */
			if (ACMAJORREV_1(pi->pubpi.phy_rev) && ACMINORREV_2(pi))
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT, sz,
					80, 8, lna2_tbl);
		}
	} else {
		tiny_gainlmt_lna2 = lna2_gainlmt_tiny;
		tiny_gainlmt_tia = tia_gainlmt_tiny;

		FOREACH_CORE(pi, core) {
			if (ACMAJORREV_4(pi->pubpi.phy_rev)) {
				gainlimitid = (core) ? ACPHY_TBL_ID_GAINLIMIT1
					: ACPHY_TBL_ID_GAINLIMIT0;
				/* get max lna2 indx */
				lna2_limit_size = wlc_phy_get_max_lna_index_acphy(pi, LNA2_ID) + 1;
			} else if (ACMAJORREV_32(pi->pubpi.phy_rev) ||
				ACMAJORREV_33(pi->pubpi.phy_rev)) {
				/* Use different for 4365 */
				tiny_gainlmt_lna2 = lna2_gainlmt_4365;
				tiny_gainlmt_tia = tia_gainlmt_4365;
				if (core == 0)
					gainlimitid = ACPHY_TBL_ID_GAINLIMIT0;
				else if (core == 1)
					gainlimitid = ACPHY_TBL_ID_GAINLIMIT1;
				else if (core == 2)
					gainlimitid = ACPHY_TBL_ID_GAINLIMIT2;
				else /* (core == 3) */
					gainlimitid = ACPHY_TBL_ID_GAINLIMIT3;
				/* get max lna2 indx */
				lna2_limit_size = wlc_phy_get_max_lna_index_acphy(pi, LNA2_ID) + 1;
				lna2_limit_size = 7;
			} else {
				gainlimitid = ACPHY_TBL_ID_GAINLIMIT;
				lna2_limit_size = 2;
			}
			wlc_phy_table_write_acphy(pi, gainlimitid, lna2_limit_size,
				16, 8, tiny_gainlmt_lna2);
			wlc_phy_table_write_acphy(pi, gainlimitid, lna2_limit_size,
				16+64, 8, tiny_gainlmt_lna2);
			wlc_phy_table_write_acphy(pi, gainlimitid, 13, 32,    8, tiny_gainlmt_tia);
			wlc_phy_table_write_acphy(pi, gainlimitid, 13, 32+64, 8, tiny_gainlmt_tia);
		}
	}
}

void wlc_phy_rfctrl_override_rxgain_acphy(phy_info_t *pi, uint8 restore,
                                           rxgain_t rxgain[], rxgain_ovrd_t rxgain_ovrd[])
{
	uint8 core, lna1_Rout, lna2_Rout;
	uint16 reg_rxgain, reg_rxgain2, reg_lpfgain;
	uint8 stall_val;
	uint8 lna1_gm;
	uint8 offset;

	if (restore == 1) {
		/* restore the stored values */
		FOREACH_CORE(pi, core) {
			WRITE_PHYREGCE(pi, RfctrlOverrideGains, core, rxgain_ovrd[core].rfctrlovrd);
			WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxgain_ovrd[core].rxgain);
			WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN2, core, rxgain_ovrd[core].rxgain2);
			WRITE_PHYREGCE(pi, RfctrlCoreLpfGain, core, rxgain_ovrd[core].lpfgain);
			PHY_INFORM(("%s, Restoring RfctrlOverride(rxgain) values\n", __FUNCTION__));
		}
	} else {
		stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
		ACPHY_DISABLE_STALL(pi);
		FOREACH_CORE(pi, core) {
			/* Save the original values */
			rxgain_ovrd[core].rfctrlovrd = READ_PHYREGCE(pi, RfctrlOverrideGains, core);
			rxgain_ovrd[core].rxgain = READ_PHYREGCE(pi, RfctrlCoreRXGAIN1, core);
			rxgain_ovrd[core].rxgain2 = READ_PHYREGCE(pi, RfctrlCoreRXGAIN2, core);
			rxgain_ovrd[core].lpfgain = READ_PHYREGCE(pi, RfctrlCoreLpfGain, core);

			offset = TINY_RADIO(pi) ? (rxgain[core].lna1 +
				ACPHY_LNAROUT_BAND_OFFSET(pi, pi->radio_chanspec)) :
				(5 + ACPHY_LNAROUT_BAND_OFFSET(pi, pi->radio_chanspec));

			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT,
				1, offset + ACPHY_LNAROUT_CORE_RD_OFST(pi, core),
				8, &lna1_Rout);
			/* LnaRoutLUT WAR for 4349A2 */
			if ((ACMAJORREV_4((pi)->pubpi.phy_rev)) && (ACMINORREV_1(pi))) {
				wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT,
					1, 0 + ACPHY_LNAROUT_CORE_RD_OFST(pi, core),
					8, &lna2_Rout);
			} else {
				wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT,
					1, 22 + ACPHY_LNAROUT_CORE_RD_OFST(pi, core),
					8, &lna2_Rout);
			}

			/* Write the rxgain override registers */
			lna1_gm = TINY_RADIO(pi) ? (lna1_Rout & 0x7) : rxgain[core].lna1;

			WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core,
			              (rxgain[core].dvga << 10) | (rxgain[core].mix << 6) |
			              (rxgain[core].lna2 << 3) | lna1_gm);

			WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN2, core,
			              (((lna2_Rout >> 3) & 0xf) << 4 | ((lna1_Rout >> 3) & 0xf)));
			WRITE_PHYREGCE(pi, RfctrlCoreLpfGain, core,
			              (rxgain[core].lpf1 << 3) | rxgain[core].lpf0);

			MOD_PHYREGCE(pi, RfctrlOverrideGains, core, rxgain, 1);
			MOD_PHYREGCE(pi, RfctrlOverrideGains, core, lpf_bq1_gain, 1);
			MOD_PHYREGCE(pi, RfctrlOverrideGains, core, lpf_bq2_gain, 1);

			reg_rxgain = READ_PHYREGCE(pi, RfctrlCoreRXGAIN1, core);
			reg_rxgain2 = READ_PHYREGCE(pi, RfctrlCoreRXGAIN2, core);
			reg_lpfgain = READ_PHYREGCE(pi, RfctrlCoreLpfGain, core);
			PHY_INFORM(("%s, core %d. rxgain_ovrd = 0x%x, lpf_ovrd = 0x%x\n",
			            __FUNCTION__, core, reg_rxgain, reg_lpfgain));
			PHY_INFORM(("%s, core %d. rxgain_rout_ovrd = 0x%x\n",
			            __FUNCTION__, core, reg_rxgain2));
			BCM_REFERENCE(reg_rxgain);
			BCM_REFERENCE(reg_rxgain2);
			BCM_REFERENCE(reg_lpfgain);
		}
		ACPHY_ENABLE_STALL(pi, stall_val);
	}
}

uint8 wlc_phy_calc_extra_init_gain_acphy(phy_info_t *pi, uint8 extra_gain_3dB,
	rxgain_t rxgain[])
{
	uint16 init_gain_code[4];
	uint8 core, MAX_DVGA, MAX_LPF, MAX_MIX;
	uint8 dvga, mix, lpf0, lpf1;
	uint8 dvga_inc, lpf0_inc, lpf1_inc;
	uint8 max_inc, gain_ticks = extra_gain_3dB;

	MAX_DVGA = 4; MAX_LPF = 10; MAX_MIX = 4;
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 3, 0xf9, 16, &init_gain_code);

	/* Find if the requested gain increase is possible */
	FOREACH_CORE(pi, core) {
		dvga = 0;
		mix = (init_gain_code[core] >> 6) & 0xf;
		lpf0 = (init_gain_code[core] >> 10) & 0x7;
		lpf1 = (init_gain_code[core] >> 13) & 0x7;
		max_inc = MAX(0, MAX_DVGA - dvga) + MAX(0, MAX_LPF - lpf0 - lpf1) +
		        MAX(0, MAX_MIX - mix);
		gain_ticks = MIN(gain_ticks, max_inc);
	}
	if (gain_ticks != extra_gain_3dB) {
		PHY_INFORM(("%s: Unable to find enough extra gain. Using extra_gain = %d\n",
		            __FUNCTION__, 3 * gain_ticks));
	}
		/* Do nothing if no gain increase is required/possible */
	if (gain_ticks == 0) {
		return gain_ticks;
	}
	/* Find the mix, lpf0, lpf1 gains required for extra INITgain */
	FOREACH_CORE(pi, core) {
		uint8 gain_inc = gain_ticks;
		dvga = 0;
		mix = (init_gain_code[core] >> 6) & 0xf;
		lpf0 = (init_gain_code[core] >> 10) & 0x7;
		lpf1 = (init_gain_code[core] >> 13) & 0x7;
		dvga_inc = MIN((uint8) MAX(0, MAX_DVGA - dvga), gain_inc);
		dvga += dvga_inc;
		gain_inc -= dvga_inc;
		lpf1_inc = MIN((uint8) MAX(0, MAX_LPF - lpf1 - lpf0), gain_inc);
		lpf1 += lpf1_inc;
		gain_inc -= lpf1_inc;
		lpf0_inc = MIN((uint8) MAX(0, MAX_LPF - lpf1 - lpf0), gain_inc);
		lpf0 += lpf0_inc;
		gain_inc -= lpf0_inc;
		mix += MIN((uint8) MAX(0, MAX_MIX - mix), gain_inc);
		rxgain[core].lna1 = init_gain_code[core] & 0x7;
		rxgain[core].lna2 = (init_gain_code[core] >> 3) & 0x7;
		rxgain[core].mix  = mix;
		rxgain[core].lpf0 = lpf0;
		rxgain[core].lpf1 = lpf1;
		rxgain[core].dvga = dvga;
	}
	return gain_ticks;
}

/* ************************************* */
/*		Carrier Sense related definitions		*/
/* ************************************* */
static void wlc_phy_ofdm_crs_acphy(phy_info_t *pi, bool enable);
static void wlc_phy_clip_det_acphy(phy_info_t *pi, bool enable);

static void
wlc_phy_ofdm_crs_acphy(phy_info_t *pi, bool enable)
{
	uint8 core;
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* MAC should be suspended before calling this function */
	ASSERT((R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC) == 0);

	if (enable) {
		if (ACREV_GE(pi->pubpi.phy_rev, 32)) {
		  FOREACH_CORE(pi, core) {
		    MOD_PHYREGCE(pi, crsControlu, core, totEnable, 1);
		    MOD_PHYREGCE(pi, crsControll, core, totEnable, 1);
		    MOD_PHYREGCE(pi, crsControluSub1, core, totEnable, 1);
		    MOD_PHYREGCE(pi, crsControllSub1, core, totEnable, 1);
		  }
		} else {
			MOD_PHYREG(pi, crsControlu, totEnable, 1);
			MOD_PHYREG(pi, crsControll, totEnable, 1);
			MOD_PHYREG(pi, crsControluSub1, totEnable, 1);
			MOD_PHYREG(pi, crsControllSub1, totEnable, 1);
		}
	} else {
		if (ACREV_GE(pi->pubpi.phy_rev, 32)) {
		  FOREACH_CORE(pi, core) {
		    MOD_PHYREGCE(pi, crsControlu, core, totEnable, 0);
		    MOD_PHYREGCE(pi, crsControll, core, totEnable, 0);
		    MOD_PHYREGCE(pi, crsControluSub1, core, totEnable, 0);
		    MOD_PHYREGCE(pi, crsControllSub1, core, totEnable, 0);
		  }
		} else {
			MOD_PHYREG(pi, crsControlu, totEnable, 0);
			MOD_PHYREG(pi, crsControll, totEnable, 0);
			MOD_PHYREG(pi, crsControluSub1, totEnable, 0);
			MOD_PHYREG(pi, crsControllSub1, totEnable, 0);
		}
	}
}

static void
wlc_phy_clip_det_acphy(phy_info_t *pi, bool enable)
{
	uint8 core;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

	/* Make clip detection difficult (impossible?) */
	/* don't change this loop to active core loop, gives 100% per, why? */
	FOREACH_CORE(pi, core) {
		if (ACREV_IS(pi->pubpi.phy_rev, 0)) {
			if (enable) {
				WRITE_PHYREGC(pi, Clip1Threshold, core,
					pi_ac->clip1_th);
			} else {
				WRITE_PHYREGC(pi, Clip1Threshold, core, 0xffff);
			}
		} else {
			if (enable) {
				phy_utils_and_phyreg(pi, ACPHYREGC(pi, computeGainInfo, core),
					(uint16)~ACPHY_REG_FIELD_MASK(pi, computeGainInfo, core,
					disableClip1detect));
			} else {
				phy_utils_or_phyreg(pi, ACPHYREGC(pi, computeGainInfo, core),
					ACPHY_REG_FIELD_MASK(pi, computeGainInfo, core,
					disableClip1detect));
			}
		}
	}

}

void wlc_phy_force_crsmin_acphy(phy_info_t *pi, void *p)
{
	int8 *set_crs_thr = p;
	/* Local copy of phyrxchains & EnTx bits before overwrite */
	uint8 enRx = 0;
	uint8 enTx = 0;
	int8 zeros[] = {0, 0, 0, 0};
	/* Prepare Mac and Phregs */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	/* Save and overwrite Rx chains */
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx);

	if (set_crs_thr[0] == -1) {
		/* Auto crsmin power mode */
		PHY_CAL(("Setting auto crsmin power mode\n"));
		wlc_phy_noise_sample_request_crsmincal((wlc_phy_t*)pi);
		pi->u.pi_acphy->crsmincal_enable = TRUE;
	} else if (set_crs_thr[0] == 0) {
		/* Default crsmin value */
		PHY_CAL(("Setting default crsmin: %d\n", ACPHY_CRSMIN_DEFAULT));
		wlc_phy_set_crs_min_pwr_acphy(pi, ACPHY_CRSMIN_DEFAULT, zeros);
		pi->u.pi_acphy->crsmincal_enable = FALSE;
	} else {
		/* Set the crsmin power value to be 'set_crs_thr' */
		PHY_CAL(("Setting crsmin: %d %d %d %d\n",
			set_crs_thr[0], set_crs_thr[1], set_crs_thr[2], set_crs_thr[3]));
		wlc_phy_set_crs_min_pwr_acphy(pi, set_crs_thr[0], set_crs_thr);
		pi->u.pi_acphy->crsmincal_enable = FALSE;
	}

	/* Restore Rx chains */
	wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

	/* Enable mac */
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}

void wlc_phy_force_lesiscale_acphy(phy_info_t *pi, void *p)
{
	int8  *set_lesi_scale = p;
	/* Local copy of phyrxchains & EnTx bits before overwrite */
	uint8 enRx = 0;
	uint8 enTx = 0;
	int8 zerodBs[] = {64, 64, 64, 64};
	/* Prepare Mac and Phregs */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	/* Save and overwrite Rx chains */
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx);

	if (set_lesi_scale[0] == -1) {
		/* Auto crsmin power mode */
		PHY_CAL(("Setting auto crsmin power mode\n"));
		wlc_phy_noise_sample_request_crsmincal((wlc_phy_t*)pi);
		pi->u.pi_acphy->lesiscalecal_enable = TRUE;
	} else if (set_lesi_scale[0] == 0) {
		/* Default crsmin value */
		PHY_CAL(("Setting default crsmin: %d\n", ACPHY_LESISCALE_DEFAULT));
		wlc_phy_set_lesiscale_acphy(pi, zerodBs);
		pi->u.pi_acphy->lesiscalecal_enable = FALSE;
	} else {
		/* Set the crsmin power value to be 'set_crs_thr' */
		printf("Setting LESI scale: %d %d %d %d\n",
			set_lesi_scale[0], set_lesi_scale[1], set_lesi_scale[2], set_lesi_scale[3]);
		wlc_phy_set_lesiscale_acphy(pi, set_lesi_scale);
		pi->u.pi_acphy->lesiscalecal_enable = FALSE;
	}

	/* Restore Rx chains */
	wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

	/* Enable mac */
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}

void
wlc_phy_crs_min_pwr_cal_acphy(phy_info_t *pi, uint8 crsmin_cal_mode)
{
	int8   cmplx_pwr_dbm[PHY_CORE_MAX], cnt, tmp_diff, cache_up_th = 2;
	int8   offset[PHY_CORE_MAX];
	int8   thresh_20[] = {39, 42, 45, 48, 51, 53, 54, 57, 60, 63,
			      66, 68, 70, 72, 75, 78, 80, 83, 86, 90}; /* idx 0 --> -36 dBm */
	int8   thresh_40[] = {41, 44, 46, 48, 50, 52, 54, 56, 58, 60,
			      63, 66, 69, 71, 74, 76, 79, 82, 86, 89}; /* idx 0 --> -34 dBm */
	int8   thresh_80[] = {41, 44, 46, 48, 50, 52, 55, 57, 60, 63,
			      65, 68, 70, 72, 74, 77, 80, 84, 87, 90}; /* idx 0 --> -30 dBm */
#ifdef WL11ULB
	int8   thresh_ULB[] = {33, 36, 39, 42, 45, 48, 51, 53, 54, 57,
			60, 63, 66, 68, 70, 72, 75, 78, 80, 83, 86, 90}; /* idx 0 --> -38 dBm */
#endif /* WL11ULB */
	int8   scale_lesi[] = {64, 57, 50, 45, 40, 35, 32, 28, 25, 22,
		20, 18, 16, 14, 12, 11, 10,  9,  8,  7,  6,  5,  5};
	uint8  thresh_sz = 15;	/* i.e. not the full size of the array */
	uint8  thresh_sz_lesi = 18;	/* i.e. not the full size of the array */
	uint8  i;
	uint8  fp[PHY_CORE_NUM_4];
	int8   lesi_scale[PHY_CORE_MAX];
	uint8  chan_freq_range, run_cal = 0, abs_diff_cache_curr = 0;
	int16  acum_noise_pwr;
	uint8  dvga;
	uint8  min_of_all_cores = 54;
	struct Tidxlist {
		int8 idx;
		int8 core;
	} idxlists[PHY_CORE_MAX];

	struct Tidxlist_lesi {
		int8 idx;
		int8 core;
	} idxlists_lesi[PHY_CORE_MAX];

	uint core_count = 0;

	/* Local copy of phyrxchains & EnTx bits before overwrite */
	uint8 enRx = 0;
	uint8 enTx = 0;
	uint8 chans[NUM_CHANS_IN_CHAN_BONDING] = {0, 0};

	bzero((uint8 *)cmplx_pwr_dbm, sizeof(cmplx_pwr_dbm));
	bzero((uint8 *)fp, sizeof(fp));

	/* Increase crsmin by 1.5dB for 4360/43602. Helps with Zero-pkt-loss (less fall triggers) */
	if (ACMAJORREV_0(pi->pubpi.phy_rev) || ACMAJORREV_5(pi->pubpi.phy_rev)) {
		for (i = 0; i < thresh_sz; i++) {
			thresh_20[i] += 4;
			thresh_40[i] += 4;
			thresh_80[i] += 4;
		}
	} else if (ACMAJORREV_3(pi->pubpi.phy_rev) && pi->u.pi_acphy->rxgcrsi != NULL &&
	           pi->u.pi_acphy->rxgcrsi->thresh_noise_cal) {
		thresh_sz = sizeof(thresh_20);
	}

	/* Initialize */
	fp[0] = 0x36;

	/* Save and overwrite Rx chains */
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx);

	/* check the freq range of the current channel */
	/* 2G, 5GL, 5GM, 5GH, 5GX */
	if (ACMAJORREV_33(pi->pubpi.phy_rev) && PHY_AS_80P80(pi, pi->radio_chanspec)) {
		wlc_phy_get_chan_freq_range_80p80_acphy(pi, pi->radio_chanspec, chans);
		chan_freq_range = chans[0];
	} else {
		chan_freq_range = wlc_phy_get_chan_freq_range_acphy(pi, 0);
	}
	ASSERT(chan_freq_range < PHY_SIZE_NOISE_CACHE_ARRAY);

	/* upate the noise pwr array with most recent noise pwr */
	if (crsmin_cal_mode == PHY_CRS_RUN_AUTO) {
		FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, i)  {
			pi->u.pi_acphy->phy_noise_pwr_array
					[pi->u.pi_acphy->phy_noise_counter][i]
					=  pi->u.pi_acphy->phy_noise_all_core[i]+1;
		}
		pi->u.pi_acphy->phy_noise_counter++;
		pi->u.pi_acphy->phy_noise_counter
				= pi->u.pi_acphy->phy_noise_counter%(PHY_SIZE_NOISE_ARRAY);
	}

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, i)  {
		if (crsmin_cal_mode == PHY_CRS_RUN_AUTO) {
			/* Enter here only from ucode noise interrupt */
			acum_noise_pwr = 0;
			/* average of the most recent noise pwrs */
			for (cnt = 0; cnt < PHY_SIZE_NOISE_ARRAY; cnt++) {
				if (pi->u.pi_acphy->phy_noise_pwr_array[cnt][i] != 0)
				{
					acum_noise_pwr += pi->u.pi_acphy->
					        phy_noise_pwr_array[cnt][i];
				} else {
					break;
				}
			}

			if (cnt != 0) {
				cmplx_pwr_dbm[i] = acum_noise_pwr/cnt;
			} else {
				/* Restore Rx chains */
				wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);
#ifdef WL_PSMX
				/* trigger MU resound due to the resetcca
				 * triggered by updating rxchains
				 */
				if (D11REV_GE(pi->sh->corerev, 64)) {
					OR_REG(pi->sh->osh, &pi->regs->maccommand_x, MCMDX_SND);
				}
#endif /* WL_PSMX */
				return;
			}

			/* cache_update threshold: 2 4335 and 4350, and 3 for 4360 */
			if (ACMAJORREV_0(pi->pubpi.phy_rev))
				cache_up_th = 3;

			/* update noisecal_cache with valid noise power */
			/* check if the new noise pwr reading is same as cache */
			tmp_diff = 0;
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				tmp_diff = pi->u.pi_acphy->phy_noise_cache_crsmin
				        [chan_freq_range][i] - cmplx_pwr_dbm[i];
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				tmp_diff = (pi->u.pi_acphy->phy_noise_cache_crsmin
				            [chan_freq_range][i] + 3) - cmplx_pwr_dbm[i];
			} else {
				tmp_diff = (pi->u.pi_acphy->phy_noise_cache_crsmin
				            [chan_freq_range][i]+7) - cmplx_pwr_dbm[i];
			}

			abs_diff_cache_curr = tmp_diff < 0 ? (0 - tmp_diff) : tmp_diff;

			/* run crscal with the current noise pwr if the call comes from phy_cals */
			if (abs_diff_cache_curr >= cache_up_th ||
				pi->u.pi_acphy->force_crsmincal)  {
				run_cal++;
				if (CHSPEC_IS20(pi->radio_chanspec)) {
					pi->u.pi_acphy->phy_noise_cache_crsmin[chan_freq_range][i]
					        = cmplx_pwr_dbm[i];
				} else if (CHSPEC_IS40(pi->radio_chanspec)) {
					pi->u.pi_acphy->phy_noise_cache_crsmin[chan_freq_range][i] =
					        cmplx_pwr_dbm[i] - 3;
				} else {
					pi->u.pi_acphy->phy_noise_cache_crsmin[chan_freq_range][i]
					        = cmplx_pwr_dbm[i] - 7;
				}
			}
		} else {
			/* Enter here only from chan_change */

			/* During chan_change, read back the noise pwr from cache */
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				cmplx_pwr_dbm[i] = pi->u.pi_acphy->phy_noise_cache_crsmin
				        [chan_freq_range][i];
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				cmplx_pwr_dbm[i] = pi->u.pi_acphy->phy_noise_cache_crsmin
				        [chan_freq_range][i] + 3;
			} else {
				cmplx_pwr_dbm[i] = pi->u.pi_acphy->phy_noise_cache_crsmin
				        [chan_freq_range][i] + 7;
			}
		}

		dvga = READ_PHYREGFLDC(pi, InitGainCodeB, i, initvgagainIndex);
		/* get the index number for crs_th table */
		if (CHSPEC_IS20(pi->radio_chanspec)) {
			idxlists[core_count].idx = cmplx_pwr_dbm[i] + 36;
			idxlists_lesi[core_count].idx = cmplx_pwr_dbm[i] - 3*dvga + 40;
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			idxlists[core_count].idx = cmplx_pwr_dbm[i] + 34;
			idxlists_lesi[core_count].idx = cmplx_pwr_dbm[i] - 3*dvga + 37;
		} else {
			idxlists[core_count].idx = cmplx_pwr_dbm[i] + 30;
			idxlists_lesi[core_count].idx = cmplx_pwr_dbm[i] - 3*dvga + 34;
		}

#ifdef WL11ULB
		if (CHSPEC_IS10(pi->radio_chanspec) || CHSPEC_IS5(pi->radio_chanspec) ||
			CHSPEC_IS2P5(pi->radio_chanspec)) {
			idxlists[core_count].idx = cmplx_pwr_dbm[i] + 38;
		}
#endif /* WL11ULB */

		PHY_CAL(("%s: cmplx_pwr (%d) =======  %d\n", __FUNCTION__, i, cmplx_pwr_dbm[i]));

		/* out of bound */
		if ((idxlists[core_count].idx < 0) ||
		    (idxlists[core_count].idx > (thresh_sz - 1))) {
			idxlists[core_count].idx = idxlists[core_count].idx < 0 ?
			        0 : (thresh_sz - 1);
		}
		if ((idxlists_lesi[core_count].idx < 0) ||
		    (idxlists[core_count].idx > (thresh_sz_lesi - 1))) {
			idxlists_lesi[core_count].idx = idxlists_lesi[core_count].idx < 0 ?
			        0 : (thresh_sz_lesi - 1);
		}
		idxlists[core_count].core = i;
		idxlists_lesi[core_count].core = i;
		if (ISSIM_ENAB(pi->sh->sih)) {
			fp[i] = fp[0];
		} else {
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				fp[i] = thresh_20[idxlists[core_count].idx];
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				fp[i] = thresh_40[idxlists[core_count].idx];
			} else if (CHSPEC_IS80(pi->radio_chanspec) ||
					PHY_AS_80P80(pi, pi->radio_chanspec)) {
				fp[i] = thresh_80[idxlists[core_count].idx];
			} else if (CHSPEC_IS160(pi->radio_chanspec)) {
				ASSERT(0);
			}
		}
#ifdef WL11ULB
		if (CHSPEC_IS10(pi->radio_chanspec) || CHSPEC_IS5(pi->radio_chanspec) ||
			CHSPEC_IS2P5(pi->radio_chanspec)) {
			fp[i] = thresh_ULB[idxlists[core_count].idx];
		}
#endif /* WL11ULB */
		if (i == 0)
			min_of_all_cores = fp[0];
		else
			min_of_all_cores = MIN(min_of_all_cores, fp[i]);

		lesi_scale[i] = scale_lesi[idxlists_lesi[core_count].idx];
		++core_count;
	}

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, i)  {
		offset[i] = fp[i] - min_of_all_cores;
	}

	fp[0] = min_of_all_cores;

	/* check if current noise pwr is different from the one in cache */
	if ((run_cal == 0) && (crsmin_cal_mode == PHY_CRS_RUN_AUTO)) {
		/* Restore Rx chains */
		wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);
#ifdef WL_PSMX
		/* trigger MU resound due to the resetcca triggered by updating rxchains */
		if (D11REV_GE(pi->sh->corerev, 64)) {
			OR_REG(pi->sh->osh, &pi->regs->maccommand_x, MCMDX_SND);
		}
#endif /* WL_PSMX */
		return;
	}

	/* Below flag is set from phy_cals only */
	if (crsmin_cal_mode == PHY_CRS_RUN_AUTO) {
		pi->u.pi_acphy->force_crsmincal = FALSE;
	}

	pi->u.pi_acphy->crsmincal_run = 1;


	/* if noise desense is on, then the below variable will be used for comparison */
	pi->u.pi_acphy->phy_crs_th_from_crs_cal = MAX(MAX(fp[0], fp[1]), fp[2]);

	if (!ACPHY_ENABLE_FCBS_HWACI(pi) || ACPHY_HWACI_WITH_DESENSE_ENG(pi)) {
		/* if desense is forced, then reset the variable below to default */
		if (pi->u.pi_acphy->total_desense->forced) {
			pi->u.pi_acphy->phy_crs_th_from_crs_cal = ACPHY_CRSMIN_DEFAULT;
			pi->u.pi_acphy->crsmincal_run = 2;

			/* Restore Rx chains */
			wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);
#ifdef WL_PSMX
			/* trigger MU resound due to the resetcca triggered by updating rxchains */
			if (D11REV_GE(pi->sh->corerev, 64)) {
				OR_REG(pi->sh->osh, &pi->regs->maccommand_x, MCMDX_SND);
			}
#endif /* WL_PSMX */
			return;
		}

		/* Don't update the crsmin registers if any desense(aci/bt) is on */
		if (pi->u.pi_acphy->total_desense->on) {
			pi->u.pi_acphy->crsmincal_run = 2;
			/* Restore Rx chains */
			wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);
#ifdef WL_PSMX
			/* trigger MU resound due to the resetcca triggered by updating rxchains */
			if (D11REV_GE(pi->sh->corerev, 64)) {
				OR_REG(pi->sh->osh, &pi->regs->maccommand_x, MCMDX_SND);
			}
#endif /* WL_PSMX */
			return;
		}
	}

	/* Debug: keep count of all calls to crsmin_cal  */
	/* Debug: store the channel info  */
	/* Debug: store the noise pwr used for updating crs thresholds */
	pi->u.pi_acphy->phy_debug_crscal_counter++;
	pi->u.pi_acphy->phy_debug_crscal_channel = CHSPEC_CHANNEL(pi->radio_chanspec);
	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, i)  {
		/* Debug info: for dumping the noise pwr used in crsmin_cal */
		pi->u.pi_acphy->phy_noise_in_crs_min[i] = cmplx_pwr_dbm[i];
	}

	/* since we are touching phy regs mac has to be suspended */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	/* call for updating the crsmin thresholds */
	if (pi->u.pi_acphy->crsmincal_enable)
	  wlc_phy_set_crs_min_pwr_acphy(pi, fp[0], offset);
	if (pi->u.pi_acphy->lesiscalecal_enable)
	  wlc_phy_set_lesiscale_acphy(pi, lesi_scale);

	/* resume mac */
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	/* Restore Rx chains */
	wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

#ifdef WL_PSMX
	/* trigger MU resound due to the resetcca triggered by updating rxchains */
	if (D11REV_GE(pi->sh->corerev, 64)) {
		OR_REG(pi->sh->osh, &pi->regs->maccommand_x, MCMDX_SND);
	}
#endif /* WL_PSMX */
}

void
wlc_phy_set_crs_min_pwr_acphy(phy_info_t *pi, uint8 ac_th, int8 *offset)
{
	uint8 core;
	uint8 mfcrs_1bit = 0; /* match filter carrier sense 1 bit mode */
	uint8 mf_th = ac_th;
	int8  mf_off1 = offset[1];
	/* 1-bit MF: 4335A0, 4335B0, 4350 */
	/* 6-bit MF: 4335C0 */

	if (ACMAJORREV_1(pi->pubpi.phy_rev) || ACMAJORREV_2(pi->pubpi.phy_rev)) {
		mfcrs_1bit = 1;
	} else {
		mfcrs_1bit = 0;
	}

	/* did not see any positive pwr consumption impact of 1 bit mode, so increase sensitivity */
	if (ACMAJORREV_1(pi->pubpi.phy_rev) && ACMINORREV_2(pi)) {
		mfcrs_1bit = 0;
	}

	if (ACMAJORREV_2(pi->pubpi.phy_rev) && !ACMINORREV_0(pi)) {
		mfcrs_1bit = 0; /* >= v2.1 */
	}

	if (ac_th == 0) {
		if (mfcrs_1bit == 1) {
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				mf_th = 60;
				ac_th = 58;
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				mf_th = 60;
				ac_th = 58;
			} else if (CHSPEC_IS80(pi->radio_chanspec) ||
					PHY_AS_80P80(pi, pi->radio_chanspec)) {
				mf_th = 67;
				ac_th = 60;
			} else if (CHSPEC_IS160(pi->radio_chanspec)) {
				ASSERT(0);
			}
		} else {
			mf_th = ACPHY_CRSMIN_DEFAULT;
			ac_th = ACPHY_CRSMIN_DEFAULT;
		}
		pi->u.pi_acphy->phy_crs_th_from_crs_cal = ac_th;
	} else {
		if (mfcrs_1bit == 1) {
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				mf_th = ((ac_th*101)/100) + 2;
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				mf_th = ((ac_th*109)/100) - 2;
			} else if (CHSPEC_IS80(pi->radio_chanspec) ||
					PHY_AS_80P80(pi, pi->radio_chanspec)) {
				mf_th = ((ac_th*105)/100) + 2;
			} if (CHSPEC_IS160(pi->radio_chanspec)) {
				ASSERT(0);
			}
		}
	}

	/* Adjust offset values for 1-bit MF */
	/* Not needed for 4335 and 4360, will be needed for 4350, disabled for now */

	if (ACMAJORREV_2(pi->pubpi.phy_rev)) {
		FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
			if (core == 1) {
				if (CHSPEC_IS5G(pi->radio_chanspec)) {
					if (CHSPEC_IS20(pi->radio_chanspec))
					{
						mf_off1 = ((ac_th+offset[core])*101/100 + 2)-mf_th;
					} else if (CHSPEC_IS40(pi->radio_chanspec)) {
						mf_off1 = ((ac_th+offset[core])*109/100 - 2)-mf_th;
					} else if (CHSPEC_IS80(pi->radio_chanspec)) {
						mf_off1 = ((ac_th+offset[core])*105/100 + 2)-mf_th;
					}
				}
			}
		}
	}

#ifdef WL11ULB
	if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) {
		if (CHSPEC_IS10(pi->radio_chanspec)) {
			ac_th -= 0;
			mf_th -= 0;
		} else if (CHSPEC_IS5(pi->radio_chanspec)) {
			ac_th -= 8;
			mf_th -= 8;
		}
	}
#endif /* WL11ULB */

	PHY_CAL(("%s: (AC_th, MF_th) = (%d, %d)\n", __FUNCTION__, ac_th, mf_th));

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {

		switch (core) {
			case 0:
				MOD_PHYREG(pi, crsminpoweru0, crsminpower0, ac_th);
				MOD_PHYREG(pi, crsmfminpoweru0, crsmfminpower0, mf_th);
				MOD_PHYREG(pi, crsminpowerl0, crsminpower0, ac_th);
				MOD_PHYREG(pi, crsmfminpowerl0, crsmfminpower0, mf_th);
				MOD_PHYREG(pi, crsminpoweruSub10, crsminpower0, ac_th);
				MOD_PHYREG(pi, crsmfminpoweruSub10, crsmfminpower0,  mf_th);
				MOD_PHYREG(pi, crsminpowerlSub10, crsminpower0, ac_th);
				MOD_PHYREG(pi, crsmfminpowerlSub10, crsmfminpower0,  mf_th);
				/* Force the offsets for core-0 */
				/* Core 0 */
				MOD_PHYREG(pi, crsminpoweroffset0,
				           crsminpowerOffsetu, offset[core]);
				MOD_PHYREG(pi, crsminpoweroffset0,
				           crsminpowerOffsetl, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffset0,
				           crsmfminpowerOffsetu, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffset0,
				           crsmfminpowerOffsetl, offset[core]);
				MOD_PHYREG(pi, crsminpoweroffsetSub10,
				           crsminpowerOffsetlSub1, offset[core]);
				MOD_PHYREG(pi, crsminpoweroffsetSub10,
				           crsminpowerOffsetuSub1, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffsetSub10,
				           crsmfminpowerOffsetlSub1, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffsetSub10,
				           crsmfminpowerOffsetuSub1, offset[core]);
				break;
			case 1:
				/* Force the offsets for core-1 */
				/* Core 1 */
				MOD_PHYREG(pi, crsminpoweroffset1,
				           crsminpowerOffsetu, offset[core]);
				MOD_PHYREG(pi, crsminpoweroffset1,
				           crsminpowerOffsetl, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffset1,
				           crsmfminpowerOffsetu, mf_off1);
				MOD_PHYREG(pi, crsmfminpoweroffset1,
				           crsmfminpowerOffsetl, mf_off1);
				MOD_PHYREG(pi, crsminpoweroffsetSub11,
				           crsminpowerOffsetlSub1, offset[core]);
				MOD_PHYREG(pi, crsminpoweroffsetSub11,
				           crsminpowerOffsetuSub1, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffsetSub11,
				           crsmfminpowerOffsetlSub1, mf_off1);
				MOD_PHYREG(pi, crsmfminpoweroffsetSub11,
				           crsmfminpowerOffsetuSub1, mf_off1);
				break;
			case 2:
				/* Force the offsets for core-2 */
				/* Core 2 */
				MOD_PHYREG(pi, crsminpoweroffset2,
				           crsminpowerOffsetu, offset[core]);
				MOD_PHYREG(pi, crsminpoweroffset2,
				           crsminpowerOffsetl, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffset2,
				           crsmfminpowerOffsetu, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffset2,
				           crsmfminpowerOffsetl, offset[core]);
				MOD_PHYREG(pi, crsminpoweroffsetSub12,
				           crsminpowerOffsetlSub1, offset[core]);
				MOD_PHYREG(pi, crsminpoweroffsetSub12,
				           crsminpowerOffsetuSub1, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffsetSub12,
				           crsmfminpowerOffsetlSub1, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffsetSub12,
				           crsmfminpowerOffsetuSub1, offset[core]);
				break;
			case 3:
				/* Force the offsets for core-3 */
				/* Core 3 */
				MOD_PHYREG(pi, crsminpoweroffset3,
					crsminpowerOffsetu, offset[core]);
				MOD_PHYREG(pi, crsminpoweroffset3,
					crsminpowerOffsetl, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffset3,
					crsmfminpowerOffsetu, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffset3,
					crsmfminpowerOffsetl, offset[core]);
				MOD_PHYREG(pi, crsminpoweroffsetSub13,
					crsminpowerOffsetlSub1, offset[core]);
				MOD_PHYREG(pi, crsminpoweroffsetSub13,
					crsminpowerOffsetuSub1, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffsetSub13,
					crsmfminpowerOffsetlSub1, offset[core]);
				MOD_PHYREG(pi, crsmfminpoweroffsetSub13,
					crsmfminpowerOffsetuSub1, offset[core]);
				break;
		default:
			break;
		}
	}
}


static void
wlc_phy_rxgainctrl_set_gaintbls_acphy_2069(phy_info_t *pi,
	uint8 core, uint16 gain_tblid, uint16 gainbits_tblid)
{
	uint16 gmsz;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	acphy_desense_values_t *desense = pi_ac->total_desense;

	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		uint8 mixbits_2g[] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

		if (PHY_ILNA(pi)) {
			/* 2g settings */
			uint8 mix_2g_43352_ilna[] = {0x9, 0x9, 0x9, 0x9, 0x9,
			                             0x9, 0x9, 0x9, 0x9, 0x9};

			/* Use lna2_gm_sz = 3 (for ACI), mix/tia_gm_sz = 2 */
			ACPHYREG_BCAST(pi, RfctrlCoreLowPwr0, 0x2c);
			ACPHYREG_BCAST(pi, RfctrlOverrideLowPwrCfg0, 0xc);

			wlc_phy_table_write_acphy(pi, gain_tblid, 10, 32, 8, mix_2g_43352_ilna);
			wlc_phy_table_write_acphy(pi, gainbits_tblid, 10, 32, 8, mixbits_2g);

			/* copying values into gaintbl arrays
				to avoid reading from table
			*/
			memcpy(pi_ac->rxgainctrl_params[core].gaintbl[3], mix_2g_43352_ilna,
				sizeof(uint8)*pi_ac->rxgainctrl_stage_len[3]);
		} else {
			uint8 mix_2g[]  = {0x3, 0x3, 0x3, 0x3, 0x3,
			                   0x3, 0x3, 0x3, 0x3, 0x3};

			/* Use lna2_gm_sz = 2 (for ACI), mix/tia_gm_sz = 1 */
			gmsz = desense->elna_bypass ? 0x1c : 0x18;
			ACPHYREG_BCAST(pi, RfctrlCoreLowPwr0, gmsz);
			ACPHYREG_BCAST(pi, RfctrlOverrideLowPwrCfg0, 0xc);
			wlc_phy_table_write_acphy(pi, gain_tblid, 10, 32, 8, mix_2g);
			wlc_phy_table_write_acphy(pi, gainbits_tblid, 10, 32, 8, mixbits_2g);

			/* copying values into gaintbl arrays to avoid reading from table */
			memcpy(pi_ac->rxgainctrl_params[core].gaintbl[3], mix_2g,
				sizeof(uint8)*pi_ac->rxgainctrl_stage_len[3]);
		}
	} else {
		/* 5g settings */
		uint8 mix5g_elna[]  = {0x7, 0x7, 0x7, 0x7, 0x7,
		                       0x7, 0x7, 0x7, 0x7, 0x7};
		uint8 mixbits5g_elna[] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
		uint8 mix5g_ilna[]  = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
		uint8 mixbits5g_ilna[] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
		uint8 *mix_5g;
		uint8 *mixbits_5g;

		/* Mixer tables based on elna/ilna */
		if (BF_ELNA_5G(pi_ac)) {
			mix_5g = mix5g_elna;
			mixbits_5g = mixbits5g_elna;
		} else {
			mix_5g = mix5g_ilna;
			mixbits_5g = mixbits5g_ilna;
		}

		/* Use lna2_gm_sz = 3, mix/tia_gm_sz = 2 */
		ACPHYREG_BCAST(pi, RfctrlCoreLowPwr0, 0x2c);
		ACPHYREG_BCAST(pi, RfctrlOverrideLowPwrCfg0, 0xc);

		wlc_phy_table_write_acphy(pi, gain_tblid, 10, 32, 8, mix_5g);
		wlc_phy_table_write_acphy(pi, gainbits_tblid, 10, 32, 8, mixbits_5g);

		/* copying values into gaintbl arrays to avoid reading from table */
		memcpy(pi_ac->rxgainctrl_params[core].gaintbl[3], mix_5g,
		       sizeof(uint8)*pi_ac->rxgainctrl_stage_len[3]);
	}
}


void
wlc_phy_rxgainctrl_set_gaintbls_acphy(phy_info_t *pi, bool init, bool band_change,
                                     bool bw_change)
{
	uint8 elna[2], ant;

	/* lna1 GainLimit */
	uint8 stall_val, core, i;
	uint16 save_forclks, gain_tblid, gainbits_tblid;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	/* Disable stall before writing tables */
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	/* If reciever is in active demod, it will NOT update the Gain tables */
	save_forclks = READ_PHYREG(pi, fineRxclockgatecontrol);
	MOD_PHYREG(pi, fineRxclockgatecontrol, forcegaingatedClksOn, 1);

	/* LNA1/2 (always do this, as the previous channel could have been in ACI mitigation) */
	wlc_phy_upd_lna1_lna2_gains_acphy(pi);

	FOREACH_CORE(pi, core) {
		if (core == 0) {
			gain_tblid = ACPHY_TBL_ID_GAIN0;
			gainbits_tblid = ACPHY_TBL_ID_GAINBITS0;
		} else if (core == 1) {
			gain_tblid = ACPHY_TBL_ID_GAIN1;
			gainbits_tblid = ACPHY_TBL_ID_GAINBITS1;
		} else if (core == 2) {
			gain_tblid = ACPHY_TBL_ID_GAIN2;
			gainbits_tblid = ACPHY_TBL_ID_GAINBITS2;
		} else {
			gain_tblid = ACPHY_TBL_ID_GAIN3;
			gainbits_tblid = ACPHY_TBL_ID_GAINBITS3;
		}

		/* FEM - elna, trloss (from srom) */
		ant = phy_get_rsdbbrd_corenum(pi, core);
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			elna[0] = elna[1] = pi_ac->sromi->femrx_2g[ant].elna;
			pi_ac->fem_rxgains[core].elna = elna[0];
			pi_ac->fem_rxgains[core].trloss = pi_ac->sromi->femrx_2g[ant].trloss;
			pi_ac->fem_rxgains[core].elna_bypass_tr =
			        pi_ac->sromi->femrx_2g[ant].elna_bypass_tr;
			pi_ac->fem_rxgains[core].lna1byp =
				((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
				& BFL2_LNA1BYPFORTR2G) != 0);
		} else if (CHSPEC_CHANNEL(pi->radio_chanspec) < 100) {
			elna[0] = elna[1] = pi_ac->sromi->femrx_5g[ant].elna;
			pi_ac->fem_rxgains[core].elna = elna[0];
			pi_ac->fem_rxgains[core].trloss = pi_ac->sromi->femrx_5g[ant].trloss;
			pi_ac->fem_rxgains[core].elna_bypass_tr =
			        pi_ac->sromi->femrx_5g[ant].elna_bypass_tr;
			pi_ac->fem_rxgains[core].lna1byp =
				((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
				& BFL2_LNA1BYPFORTR5G) != 0);
		} else {
			elna[0] = elna[1] = pi_ac->sromi->femrx_5gh[ant].elna;
			pi_ac->fem_rxgains[core].elna = elna[0];
			pi_ac->fem_rxgains[core].trloss = pi_ac->sromi->femrx_5gh[ant].trloss;
			pi_ac->fem_rxgains[core].elna_bypass_tr =
			        pi_ac->sromi->femrx_5gh[ant].elna_bypass_tr;
			pi_ac->fem_rxgains[core].lna1byp =
				((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
				& BFL2_LNA1BYPFORTR5G) != 0);
		}

		wlc_phy_set_trloss_reg_acphy(pi, core);

		wlc_phy_table_write_acphy(pi, gain_tblid, 2, 0, 8, elna);
		memcpy(pi_ac->rxgainctrl_params[core].gaintbl[0],
		       elna, sizeof(uint8)*pi_ac->rxgainctrl_stage_len[0]);

		/* MIX, LPF / TIA */
		if (TINY_RADIO(pi)) {
			if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) {
				wlc_phy_rxgainctrl_set_gaintbls_acphy_wave2(
					pi, core, gain_tblid, gainbits_tblid);
			} else {
				wlc_phy_rxgainctrl_set_gaintbls_acphy_tiny(
					pi, core, gain_tblid, gainbits_tblid);
			}
		} else if (init || band_change) {
			if (ACMAJORREV_33(pi->pubpi.phy_rev) && ACMINORREV_4(pi)) {
				wlc_phy_rxgainctrl_set_gaintbls_acphy_28nm(
					pi,core, gain_tblid, gainbits_tblid);
			} else {
				wlc_phy_rxgainctrl_set_gaintbls_acphy_2069(pi, core, gain_tblid,
			                                           gainbits_tblid);
			}
		}

		if (init) {
			/* Store gainctrl info (to be used for Auto-Gainctrl)
			 * lna1,2 taken care in wlc_phy_upd_lna1_lna2_gaintbls_acphy()
			 */
			wlc_phy_table_read_acphy(pi, gainbits_tblid, 1, 0, 8,
			                         pi_ac->rxgainctrl_params[core].gainbitstbl[0]);
			wlc_phy_table_read_acphy(pi, gainbits_tblid, 10, 32, 8,
			                         pi_ac->rxgainctrl_params[core].gainbitstbl[3]);
			wlc_phy_table_read_acphy(pi, gain_tblid, 8, 96, 8,
			                         pi_ac->rxgainctrl_params[core].gaintbl[4]);
			wlc_phy_table_read_acphy(pi, gain_tblid, 8, 112, 8,
			                         pi_ac->rxgainctrl_params[core].gaintbl[5]);
			wlc_phy_table_read_acphy(pi, gainbits_tblid, 8, 96, 8,
			                         pi_ac->rxgainctrl_params[core].gainbitstbl[4]);
			wlc_phy_table_read_acphy(pi, gainbits_tblid, 8, 112, 8,
			                         pi_ac->rxgainctrl_params[core].gainbitstbl[5]);

			if (TINY_RADIO(pi) || IS_28NM_RADIO(pi)) {
				/* initialise DVGA table */
				for (i = 0; i < pi_ac->rxgainctrl_stage_len[6]; i++) {
					pi_ac->rxgainctrl_params[core].gaintbl[6][i] = 3 * i;
					pi_ac->rxgainctrl_params[core].gainbitstbl[6][i] = i;
				}
			}
		}
	}

	/* Restore */
	WRITE_PHYREG(pi, fineRxclockgatecontrol, save_forclks);
	ACPHY_ENABLE_STALL(pi, stall_val);
}

void
wlc_phy_set_lna1byp_reg_acphy(phy_info_t *pi, int8 core)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	int8 lna1_gain_offset = 0, core_freq_segment_map, ant;
	int8  bw_idx, subband_idx, gain_lna1_byp;
	int8 elna_off_index;
	bool suspend = FALSE;
	wlc_phy_conditional_suspend(pi, &suspend);

	if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
		elna_off_index = ACPHY_GAIN_DELTA_ELNA_OFF_43012;
	} else {
		elna_off_index = ACPHY_GAIN_DELTA_ELNA_OFF;
	}
	bw_idx = (CHSPEC_IS80(pi->radio_chanspec)) ? 2 :
	        (CHSPEC_IS40(pi->radio_chanspec)) ? 1 : 0;
	core_freq_segment_map = pi->u.pi_acphy->core_freq_mapping[core];
	ant = phy_get_rsdbbrd_corenum(pi, core);

	if (pi_ac->rssi_cal_rev == FALSE) {
		subband_idx = wlc_phy_get_chan_freq_range_acphy(pi,
		  pi->radio_chanspec) - 1;
	} else {
		subband_idx = wlc_phy_rssi_get_chan_freq_range_acphy(pi, core_freq_segment_map);
	}
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
	  if (pi->u.pi_acphy->rssi_cal_rev == FALSE) {
	    lna1_gain_offset =
	      pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g[ant]
	        [elna_off_index][bw_idx]-
	        pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g[ant]
	        [ACPHY_GAIN_DELTA_ELNA_ON][bw_idx];
	  } else {
	    lna1_gain_offset =
	      pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g_sub
	        [ant][elna_off_index][bw_idx][subband_idx]-
	        pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g_sub
	        [ant][ACPHY_GAIN_DELTA_ELNA_ON][bw_idx][subband_idx];
	  }
	} else {
	  if (pi->u.pi_acphy->rssi_cal_rev == FALSE) {
	    lna1_gain_offset =
	      pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g[ant]
	        [elna_off_index][bw_idx][subband_idx]-
	        pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g[ant]
	        [ACPHY_GAIN_DELTA_ELNA_ON][bw_idx][subband_idx];
	  } else {
	    lna1_gain_offset =
	      pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g_sub
	        [ant][elna_off_index][bw_idx][subband_idx]-
	        pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g_sub
	        [ant][ACPHY_GAIN_DELTA_ELNA_ON][bw_idx][subband_idx];
	  }
	}
	if (pi->u.pi_acphy->rssi_cal_rev == TRUE) {
	  /* With new scheme, rssi gain delta's are in qdB steps */
	  lna1_gain_offset = (lna1_gain_offset + 2) >> 2;
	}
	if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			gain_lna1_byp = LNA1_BYPASS_GAIN_2G_20695 - lna1_gain_offset;
		} else {
			gain_lna1_byp = LNA1_BYPASS_GAIN_5G_20695 - lna1_gain_offset;
		}
		MOD_PHYREG(pi, Core0_lna1BypVals, lna1BypGain0, gain_lna1_byp);
	}
	wlc_phy_conditional_resume(pi, &suspend);
}


/* Additional settings to use LNA1 bypass instead of T/R */
static void
phy_ac_upd_lna1_bypass(phy_info_t *pi, uint8 core)
{
	uint8 rout_2g = 0;
	int8  gain_2g = 0;
	uint8 rout_5g = 0;
	int8  gain_5g = 0;
	uint8 idx = 6;	/* Rout table index to be used when in bypass */
	uint8 rout;
	int8 gain;
	uint16 byp_val;
	uint8 rout_val;
	uint32 rout_offset;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;

	// if (ACMAJORREV_36(pi->pubpi.phy_rev) || ACMAJORREV_25_40(pi->pubpi.phy_rev)) {
	if (IS_28NM_RADIO(pi)) {
		/* 43012 bypass settings */
		gain_2g = rxg_params->lna1_byp_gain_2g;
		gain_5g = rxg_params->lna1_byp_gain_5g;
		rout_2g = rxg_params->lna1_byp_rout_2g;
		rout_5g = rxg_params->lna1_byp_rout_5g;
		idx = rxg_params->lna1_byp_indx;
	}

	/* Make sure the AGC knows the gain when LNA1 is in bypass */
	gain = CHSPEC_IS2G(pi->radio_chanspec) ? gain_2g: gain_5g;
	byp_val = (0xff & gain) << ACPHY_Core0_lna1BypVals_lna1BypGain0_SHIFT(pi->pubpi.phy_rev);

	/* Gain/Rout table index to be used when in bypass */
	byp_val |= idx << ACPHY_Core0_lna1BypVals_lna1BypIndex0_SHIFT(pi->pubpi.phy_rev);

	/* Have the AGC use the gain set in lna1BypGain */
	byp_val |= 1 << ACPHY_Core0_lna1BypVals_lna1BypEn0_SHIFT(pi->pubpi.phy_rev);

	WRITE_PHYREGC(pi, _lna1BypVals, core, byp_val);

	/* Set the Rout to be used for index 6 */
	rout = CHSPEC_IS2G(pi->radio_chanspec) ? rout_2g: rout_5g;
	rout_val = (rout << 3) + idx;
	// if (ACMAJORREV_36(pi->pubpi.phy_rev) || ACMAJORREV_40(pi->pubpi.phy_rev)) {
	if (IS_28NM_RADIO(pi)) {
		rout_offset = (core * 24) + idx + (CHSPEC_IS2G(pi->radio_chanspec) ? 0 : 8);
	} else {
		rout_offset = (core * 24) + idx + (CHSPEC_IS2G(pi->radio_chanspec) ? 0 : 16);
	}
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, rout_offset, 8, &rout_val);

	/* Power down LNA1 when bypassed */
	MOD_PHYREG(pi, AfePuCtrl, lna1_pd_during_byp, 1);

	/* Only for chips with a dedicated BT-LNA, no shared LNA */
	ASSERT(READ_PHYREGFLD(pi, SlnaControl, SlnaEn) == 0);

	/* WAR for LNA1 bypass sticky on Core 0, CRDOT11ACPHY-1161 */
	MOD_PHYREG(pi, SlnaControl, SlnaCore, 3);

	if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
		wlc_phy_set_lna1byp_reg_acphy(pi, core);
	}
}


void
wlc_phy_set_lesiscale_acphy(phy_info_t *pi, int8 *lesi_scale)
{
	uint8 core;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	if (pi_ac->lesi) {
	  FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
	    if (ACMAJORREV_32(pi->pubpi.phy_rev)) {
		MOD_PHYREGCE(pi, lesiPathControl0, core, inpScalingFactor, lesi_scale[core]);
	      } else {
		MOD_PHYREGCE(pi, lesiInputScaling0_, core, inpScalingFactor_0, lesi_scale[core]);
		MOD_PHYREGCE(pi, lesiInputScaling1_, core, inpScalingFactor_1, lesi_scale[core]);
		MOD_PHYREGCE(pi, lesiInputScaling2_, core, inpScalingFactor_2, lesi_scale[core]);
		MOD_PHYREGCE(pi, lesiInputScaling3_, core, inpScalingFactor_3, lesi_scale[core]);
	      }
	  }
	}
}

void
wlc_phy_stay_in_carriersearch_acphy(phy_info_t *pi, bool enable)
{
	uint8 class_mask;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* MAC should be suspended before calling this function */
	ASSERT((R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC) == 0);
	if (enable) {
		if (pi_ac->deaf_count == 0) {
			wlc_phy_classifier_acphy(pi, ACPHY_ClassifierCtrl_classifierSel_MASK, 4);
			wlc_phy_ofdm_crs_acphy(pi, FALSE);
			wlc_phy_clip_det_acphy(pi, FALSE);
			WRITE_PHYREG(pi, ed_crsEn, 0);
		}

		pi_ac->deaf_count++;
#ifdef WL_MUPKTENG
		if (!(D11REV_IS(pi->sh->corerev, 64) && wlapi_is_mutx_pkteng_on(pi->sh->physhim)))
#endif
		{
		        wlc_phy_resetcca_acphy(pi);
		}
	} else {
	  ASSERT(pi_ac->deaf_count > 0);

		pi_ac->deaf_count--;
		if (pi_ac->deaf_count == 0) {
			class_mask = CHSPEC_IS2G(pi->radio_chanspec) ? 7 : 6;   /* No bphy in 5g */
			wlc_phy_classifier_acphy(pi, ACPHY_ClassifierCtrl_classifierSel_MASK,
			                         class_mask);
			wlc_phy_ofdm_crs_acphy(pi, TRUE);
			wlc_phy_clip_det_acphy(pi, TRUE);
			WRITE_PHYREG(pi, ed_crsEn, pi_ac->edcrs_en);
		}
	}
}

uint8 wlc_phy_get_lna_gain_rout(phy_info_t *pi, uint8 idx, acphy_lna_gain_rout_t type)
{
	uint8 ret_val;
	if (type == GET_LNA_GAINCODE) {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			ret_val = ac_tiny_g_lna_gain_map[idx];
		} else {
			ret_val = ac_tiny_a_lna_gain_map[idx];
		}
	} else {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			ret_val = ac_tiny_g_lna_rout_map[idx];
		} else {
			ret_val = ac_tiny_a_lna_rout_map[idx];
		}
	}
	return (ret_val);
}

/* This function tells you locale info, e.g EU, so that correct edcrs setting could be done */
static void
phy_ac_rxgcrs_set_locale(phy_type_rxgcrs_ctx_t *ctx, uint8 region_group)
{
	/* USAGE: if (region_group == LOCALE_EU) */
	/* Nothing for now - just a template */
}

void wlc_phy_set_srom_eu_edthresh_acphy(phy_info_t *pi)
{
	int32 eu_edthresh;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	eu_edthresh = CHSPEC_IS2G(pi->radio_chanspec) ?
	        pi->srom_eu_edthresh2g : pi->srom_eu_edthresh5g;
	if (eu_edthresh < -10) /* 0 & 0xff(-1) are invalid values */
		wlc_phy_adjust_ed_thres_acphy(pi, &eu_edthresh, TRUE);
	else
		wlc_phy_adjust_ed_thres_acphy(pi, &pi_ac->sromi->ed_thresh_default, TRUE);
}

/* For 7271, see phy_ac_set_20696_RSSI_detectors below */
static void
phy_ac_set_ssagc_RSSI_detectors(phy_info_t *pi, uint8 core, uint8 *thresholds)
{
#ifdef BCM7271		// Remove when merging to Trunk
	ASSERT(0);
#else // BCM7271
	uint8 w1 = thresholds[0];
	uint8 w3_low = thresholds[1];
	uint8 w3_mid = thresholds[2];
	uint8 w3_hi = thresholds[3];
	uint8 nb_low = thresholds[4];
	uint8 nb_mid = thresholds[5];
	uint8 nb_hi = thresholds[6];
	/* Configure W1 threshold */
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		if (ACMAJORREV_40(pi->pubpi.phy_rev)) {
			MOD_RADIO_REG_20694(pi, RF, LNA2G_REG3, core,
					lna2g_dig_wrssi1_threshold, w1);
		} else {
			MOD_RADIO_REG_28NM(pi, RF, LNA2G_REG3, core, rx2g_wrssi1_threshold, w1);
		}
	} else {
		if (ACMAJORREV_40(pi->pubpi.phy_rev)) {
			MOD_RADIO_REG_20694(pi, RF, RX5G_WRSSI1, core, rx5g_wrssi_threshold, w1);
		} else {
			MOD_RADIO_REG_28NM(pi, RF, RX5G_WRSSI1, core, rx5g_wrssi_threshold, w1);
		}
	}

	if (ACMAJORREV_40(pi->pubpi.phy_rev)) {
		MOD_RADIO_REG_20694(pi, RF, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold2, w3_hi);
		MOD_RADIO_REG_20694(pi, RF, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold1, w3_mid);
		MOD_RADIO_REG_20694(pi, RF, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold0, w3_low);

		MOD_RADIO_REG_20694(pi, RF, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold2, nb_hi);
		MOD_RADIO_REG_20694(pi, RF, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold1, nb_mid);
		MOD_RADIO_REG_20694(pi, RF, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold0, nb_low);
	} else {
		/* Configure W3 thresholds */
		MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold2, w3_hi);
		MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold1, w3_mid);
		MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold0, w3_low);

		/* Configure NB thresholds */
		MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold2, nb_hi);
		MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold1, nb_mid);
		MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold0, nb_low);
	}
#endif // BCM7271
}

/* Same functionality as phy_ac_set_ssagc_RSSI_detectors In Trunk/Iguana */
static void
phy_ac_set_20696_RSSI_detectors(phy_info_t *pi, uint8 core, uint8 *thresholds)
{
	uint8 w1 = thresholds[0];
	uint8 w3_low = thresholds[1];
	uint8 w3_mid = thresholds[2];
	uint8 w3_hi = thresholds[3];
	uint8 nb_low = thresholds[4];
	uint8 nb_mid = thresholds[5];
	uint8 nb_hi = thresholds[6];

	ASSERT(RADIOID_IS(pi->pubpi.radioid, BCM20696_ID));

	/* Configure W1 threshold */
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		MOD_RADIO_REG_20696(pi, LNA2G_REG3, core, lna2g_dig_wrssi1_threshold, w1);
	} else {
		MOD_RADIO_REG_20696(pi, RX5G_WRSSI1, core, rx5g_wrssi_threshold, w1);
	}

	/* Configure W3 thresholds */
	MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_high, w3_hi);
	MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_mid, w3_mid);
	MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_low, w3_low);

	/* Configure NB thresholds */
	MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_high, nb_hi);
	MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_mid, nb_mid);
	MOD_RADIO_REG_20696(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_low, nb_low);
}

void
chanspec_setup_rxgcrs(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;

	/* Rx gainctrl (if not QT) */
	if (ISSIM_ENAB(pi->sh->sih)) {
		if (!ACMAJORREV_36(pi->pubpi.phy_rev)) {
			return;
		}
	}

#ifndef BCM7271 // Code in Trunk we do not want/need
	/*
	 For initial bringup need to call subband cust
	 Flag is read from 'subband_cust' NVRAM param
	*/
	if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
		if (pi->u.pi_acphy->enable_subband_cust == 1) {
			phy_ac_subband_cust_28nm_ulp(pi);
			return;
		}
	}
#endif // BCM7271

	pi_ac->aci = NULL;

	if (!wlc_phy_is_scan_chan_acphy(pi)) {
		pi->interf->curr_home_channel = CHSPEC_CHANNEL(pi->radio_chanspec);
#ifndef WLC_DISABLE_ACI
		/* Get pointer to current aci channel list */
		if (!ACPHY_ENABLE_FCBS_HWACI(pi) || ACPHY_HWACI_WITH_DESENSE_ENG(pi)) {
			pi_ac->aci = wlc_phy_desense_aci_getset_chanidx_acphy(pi,
				pi->radio_chanspec, TRUE);
		}
#endif /* !WLC_DISABLE_ACI */
	}

#ifndef WLC_DISABLE_ACI
	/* Merge ACI & BT params into one */
	if (!ACPHY_ENABLE_FCBS_HWACI(pi) || ACPHY_HWACI_WITH_DESENSE_ENG(pi)) {
		wlc_phy_desense_calc_total_acphy(pi);
		wlc_phy_update_curr_desense_acphy(pi);
	}
#endif /* !WLC_DISABLE_ACI */

	wlc_phy_rxgainctrl_set_gaintbls_acphy(pi,
		CCT_INIT(pi_ac), CCT_BAND_CHG(pi_ac), CCT_BW_CHG(pi_ac));

#ifndef WLC_DISABLE_ACI
	if (ACPHY_ENABLE_FCBS_HWACI(pi) && (CCT_INIT(pi_ac) || CCT_BAND_CHG(pi_ac)))
		wlc_phy_hwaci_mitigation_enable_acphy_tiny(pi,
			((pi->sh->interference_mode & ACPHY_HWACI_MITIGATION) > 0)
#ifdef WL_ACI_FCBS
			? HWACI_AUTO_FCBS : 0,
#else
			? HWACI_AUTO_SW : 0,
#endif
			TRUE);

#endif /* WLC_DISABLE_ACI */

	if (TINY_RADIO(pi)) {
		wlc_phy_rxgainctrl_gainctrl_acphy_tiny(pi, 0);
		if (ACMAJORREV_3(pi->pubpi.phy_rev) && ACREV_GE(pi->pubpi.phy_rev, 11))
			wlc_phy_enable_lna_dcc_comp_20691(pi, 0);
	} else if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
	} else if (IS_28NM_RADIO(pi)) {
		if (rxg_params->mclip_agc_en) {
			wlc_phy_mclip_agc(pi, CCT_INIT(pi_ac), CCT_BAND_CHG(pi_ac),
				CCT_BW_CHG(pi_ac));
		} else {
			PHY_ERROR(("%s: Fast AGC needed for ACMAJORREV_25_40\n", __FUNCTION__));
			return;
		}
	} else {
		/* Set INIT, Clip gains, clip thresh (srom based) */
		wlc_phy_rxgainctrl_gainctrl_acphy(pi);
	}

#ifndef WLC_DISABLE_ACI
	if (!ACPHY_ENABLE_FCBS_HWACI(pi) || ACPHY_HWACI_WITH_DESENSE_ENG(pi)) {
		/*
		 * Desense on top of default gainctrl, if desense on
		 * (otherwise restore defaults)
		 */
		wlc_phy_desense_apply_acphy(pi, pi_ac->total_desense->on);
	}
#endif /* !WLC_DISABLE_ACI */
	if (ACMAJORREV_40(pi->pubpi.phy_rev)) {
		/* force a reset2rx seq to let gain changes take effects */
		wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RESET2RX);
	}
}

void
wlc_phy_rxgainctrl_set_gaintbls_acphy_28nm(phy_info_t *pi,
	uint8 core, uint16 gain_tblid, uint16 gainbits_tblid)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	//acphy_desense_values_t *desense = pi_ac->total_desense;
	phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;

	const uint16 gainlimit_tblids[PHY_CORE_MAX] =
		{ACPHY_TBL_ID_GAINLIMIT0, ACPHY_TBL_ID_GAINLIMIT1,
	         ACPHY_TBL_ID_GAINLIMIT2, ACPHY_TBL_ID_GAINLIMIT3};

	/* TIA */
	/* 2g & 5g settings */
	uint8 tia[16];
	uint8 tiabits[16];
	uint8 tialimit[16];
	uint8 i;
	//bool elna_present = CHSPEC_IS2G(pi->radio_chanspec) ? BF_ELNA_2G(pi_ac)
	//                                                    : BF_ELNA_5G(pi_ac);
	uint8 elna_gainbits[] = {0, 0};
	uint8 elna_gainlimits[] = {0, 0};
	uint8 lna1_byp_bit = 6;

	uint16 gainlimit_tblid = gainlimit_tblids[core];

	/* ELNA setting */
	wlc_phy_table_write_acphy(pi, gainbits_tblid, pi_ac->rxgainctrl_stage_len[ELNA_ID],
		EXT_LNA_OFFSET, ACPHY_GAINDB_TBL_WIDTH, elna_gainbits);
	wlc_phy_table_write_acphy(pi, gainlimit_tblid, pi_ac->rxgainctrl_stage_len[ELNA_ID],
		EXT_LNA_OFFSET, ACPHY_GAINDB_TBL_WIDTH, elna_gainlimits);

	/* LNA1 settting already taken care of by upd_lna1_lna2_gains_acphy */

	/* Put LNA1 bypass index in gainbits */
	wlc_phy_table_write_acphy(pi, gainbits_tblid, 1,
		(LNA1_OFFSET + 6), ACPHY_GAINBITS_TBL_WIDTH, &lna1_byp_bit);
	/* LNA1 bypass related config */
	//if (pi_ac->rxgcrsi->fem_rxgains[core].lna1byp) {
	phy_ac_upd_lna1_bypass(pi, core);
	//}

	/* TIA setting */
	memcpy(tia, rxg_params->tia_gain_tbl, sizeof(int8) * N_TIA_GAINS);
	memcpy(tiabits, rxg_params->tia_gainbits_tbl, sizeof(int8) * N_TIA_GAINS);
	memcpy(tialimit, rxg_params->gainlimit_tbl[TIA_TBL_IND], sizeof(int8) * N_TIA_GAINS);
	for (i = 0; i < 4; i++) {
		tia[N_TIA_GAINS+i] = rxg_params->tia_gain_tbl[N_TIA_GAINS-1];
		tiabits[N_TIA_GAINS+i] = rxg_params->tia_gainbits_tbl[N_TIA_GAINS-1];
		tialimit[N_TIA_GAINS+i] = rxg_params->gainlimit_tbl[TIA_TBL_IND][N_TIA_GAINS-1];
	}

	/* program tia table */
	wlc_phy_table_write_acphy(pi, gain_tblid, 16, TIA_OFFSET, ACPHY_GAINDB_TBL_WIDTH, tia);

	wlc_phy_table_write_acphy(pi, gainbits_tblid, 16, TIA_OFFSET, ACPHY_GAINBITS_TBL_WIDTH,
		tiabits);

	wlc_phy_table_write_acphy(pi, gainlimit_tblid, 16, TIA_OFFSET, ACPHY_GAINLMT_TBL_WIDTH,
		tialimit);
	wlc_phy_table_write_acphy(pi, gainlimit_tblid, 16, 64+TIA_OFFSET, ACPHY_GAINLMT_TBL_WIDTH,
		tialimit);

	/* copying values into gaintbl arrays to avoid reading from table */
	memcpy(pi_ac->rxgainctrl_params[core].gaintbl[TIA_ID], tia,
		sizeof(pi_ac->rxgainctrl_params[core].gaintbl[0][0])
		* N_TIA_GAINS);

	memcpy(pi_ac->rxgainctrl_params[core].gainbitstbl[TIA_ID], tiabits,
		sizeof(pi_ac->rxgainctrl_params[core].gainbitstbl[0][0])
		* N_TIA_GAINS);

	/* BIQ0 setting */
	wlc_phy_table_write_acphy(pi, gain_tblid, N_BIQ01_GAINS, BIQ0_OFFSET,
		ACPHY_GAINDB_TBL_WIDTH, rxg_params->biq01_gain_tbl[0]);
	wlc_phy_table_write_acphy(pi, gainbits_tblid, N_BIQ01_GAINS, BIQ0_OFFSET,
		ACPHY_GAINBITS_TBL_WIDTH, rxg_params->biq01_gainbits_tbl[0]);
	wlc_phy_table_write_acphy(pi, gainlimit_tblid, N_BIQ01_GAINS, 48,
		ACPHY_GAINLMT_TBL_WIDTH, rxg_params->gainlimit_tbl[BIQ0_TBL_IND]);
	wlc_phy_table_write_acphy(pi, gainlimit_tblid, N_BIQ01_GAINS, 64+48,
		ACPHY_GAINLMT_TBL_WIDTH, rxg_params->gainlimit_tbl[BIQ0_TBL_IND]);


	/* BIQ1 setting */
	wlc_phy_table_write_acphy(pi, gain_tblid, 3, DVGA1_OFFSET, ACPHY_GAINDB_TBL_WIDTH,
		rxg_params->biq01_gain_tbl[1]);
	wlc_phy_table_write_acphy(pi, gainbits_tblid, 3, DVGA1_OFFSET, ACPHY_GAINBITS_TBL_WIDTH,
		rxg_params->biq01_gainbits_tbl[1]);

}

static void wlc_phy_mclip_agc(phy_info_t *pi, bool init, bool band_change, bool bw_change)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint8 core, i;
	int8 mclip_rssi[PHY_CORE_MAX][9];
	if (init) {
		if (ACMAJORREV_25(pi->pubpi.phy_rev)) {
			FOREACH_CORE(pi, core) {
				/* disable clip2 when mclip agc enabled for 4347TC2 */
				MOD_PHYREGC(pi, computeGainInfo, core, disableClip2detect, 1);
				WRITE_PHYREGC(pi, Clip2Threshold, core, 0xffff);
			}
		}
		FOREACH_CORE(pi, core) {
			/* set proper clip detector mux connection */
			WRITE_PHYREGC(pi, _RSSIMuxSel2, core,  0x06db);
		}
		//Enable Mclip
		MOD_PHYREG(pi, clip_detect_normpwr_var_mux, mClpAgcSdwTblEn, 1);
		MOD_PHYREG(pi, clip_detect_normpwr_var_mux, mClpAgcEn, 1);
	}

	if (init || band_change || bw_change) {
		/* program init gain */
		FOREACH_CORE(pi, core) {
			for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES-1; i++)
				pi_ac->rxgainctrl_maxout_gains[i] = 67;
			pi_ac->rxgainctrl_maxout_gains[ACPHY_MAX_RX_GAIN_STAGES-1] = 100;

			wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 0, 67, 0, 0, core);
		}
		wlc_phy_mclip_agc_rssi_setup(pi, init, band_change, bw_change, mclip_rssi);
		wlc_phy_mclip_agc_rxgainctrl(pi, mclip_rssi);
	}

}

static void wlc_phy_mclip_agc_rssi_setup(phy_info_t *pi, bool init, bool band_change,
	bool bw_change, int8 mclip_rssi[][9])
{

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint8 core;
	uint8 analog_threshold[7] = {8, 4, 4, 4, 2, 2, 2};
	uint8 ana_thld_2g_r40[7] = {14, 4, 4, 4, 2, 2, 2};
	const uint8 clipcnt_thresh_bw80[9] = {80, 80, 80, 120, 120, 120, 120, 120, 120};
	const uint8 clipcnt_thresh_bw40[9] = {40, 40, 40,  60,  60,  60,  60,  60,  60};
	const uint8 clipcnt_thresh_bw20[9] = {20, 20, 20,  30,  30,  30,  30,  30,  30};
	int8 nblo_ref = 0, w1md_ref = 0;
	uint8 elna_indx, lna1_indx, tia_indx;
	int8 elna_gain, lna1_gain, tia_gain, nb_gain, w1_gain;

	FOREACH_CORE(pi, core) {
		/* each core is programed the same threshold. can be different if necessary */
		if (init) {
			if ACMAJORREV_25_40(pi->pubpi.phy_rev) {
				if (CHSPEC_IS2G(pi->radio_chanspec) && (BF_ELNA_2G(pi_ac))) {
					phy_ac_set_ssagc_RSSI_detectors(pi, core, ana_thld_2g_r40);
					nblo_ref = -8;
					w1md_ref = -2;
				} else {
					phy_ac_set_ssagc_RSSI_detectors(pi, core, analog_threshold);
					nblo_ref = -6;
					w1md_ref = 0;
				}
			} else if (RADIOID_IS(pi->pubpi.radioid, BCM20696_ID)) {
				if (CHSPEC_IS2G(pi->radio_chanspec) && (BF_ELNA_2G(pi_ac))) {
					phy_ac_set_20696_RSSI_detectors(pi, core, ana_thld_2g_r40);
					nblo_ref = -8;
					w1md_ref = -2;
				} else {
					phy_ac_set_20696_RSSI_detectors(pi, core, analog_threshold);
					nblo_ref = -6;
					w1md_ref = 0;
				}
			}
		}

		if (band_change) {
			if (CHSPEC_IS80(pi->radio_chanspec)) {
				wlc_phy_mclip_agc_clipcnt_thresh(pi, core, clipcnt_thresh_bw80);
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				wlc_phy_mclip_agc_clipcnt_thresh(pi, core, clipcnt_thresh_bw40);
			} else {
				wlc_phy_mclip_agc_clipcnt_thresh(pi, core, clipcnt_thresh_bw20);
			}
		}
		elna_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
		lna1_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initLnaIndex);
		tia_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initmixergainIndex);
		if (tia_indx > 11) tia_indx = 11;

		elna_gain = pi_ac->rxgainctrl_params[core].gaintbl[ELNA_ID][elna_indx];
		lna1_gain = pi_ac->rxgainctrl_params[core].gaintbl[LNA1_ID][lna1_indx];
		tia_gain = pi_ac->rxgainctrl_params[core].gaintbl[TIA_ID][tia_indx];
		w1_gain = elna_gain + lna1_gain;
		nb_gain = w1_gain + tia_gain;

		if (ACMAJORREV_25_40(pi->pubpi.phy_rev) ||
		    (ACMAJORREV_33(pi->pubpi.phy_rev) && ACMINORREV_4(pi))) {
			mclip_rssi[core][0] = nblo_ref - nb_gain;
			mclip_rssi[core][1] = mclip_rssi[core][0] + 6;
			mclip_rssi[core][2] = mclip_rssi[core][1] + 6;
			mclip_rssi[core][3] = mclip_rssi[core][2] + 1;
			mclip_rssi[core][4] = mclip_rssi[core][3] + 6;
			mclip_rssi[core][5] = mclip_rssi[core][4] + 6;
			mclip_rssi[core][7] = w1md_ref - w1_gain;
			mclip_rssi[core][6] = mclip_rssi[core][7] - 6;
			mclip_rssi[core][8] = mclip_rssi[core][7] + 6;
		}
	}
}


static void wlc_phy_mclip_agc_rxgainctrl(phy_info_t *pi, int8 mclip_rssi[][9])
{

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	int8 maxgain[] = {100, 100, 100, 100, 100, 100, 100};
	int8 margin_low_sens = 15, margin_high_sens = 12;
	int8 det_adjust_list[] = {24, 22, 20};
	int8 high_sens_adjust_list_2g[] = {14, 14, 14};
	int8 high_sens_adjust_list_2g_rev40[] = {10, 10, 10};
	int8 high_sens_adjust_list_5g[] = {6, 8, 9};
	int8 *high_sens_adjust_list;
	int8 high_adjust, det_adjust;
	bool elna_present = CHSPEC_IS2G(pi->radio_chanspec) ? BF_ELNA_2G(pi_ac)
	                                                    : BF_ELNA_5G(pi_ac);
	uint8 i;
	uint8 bw_indx = CHSPEC_IS20(pi->radio_chanspec) ? 0 :
			CHSPEC_IS40(pi->radio_chanspec) ? 1 : 2;

	int8 sens_hi, sens_lna1Byp_lo, sens_lna1Byp_hi, sens_elnaByp_lo = -1;
	int8 lna1Byp_rssi_idx, lna1Byp_gain_idx;
	int8 elnaByp_rssi_idx = -1, elnaByp_gain_idx;
	int8 sens_high, sens_low;
	uint8 gain_indx[7];
	uint32 mclip_gaincode[10];
	uint8 core;
	int8 rssi, rssi_low, rssi_high, clipgain, min_gain, max_gain, trtx, lna1byp;
	uint8 mag;

	mag = READ_PHYREGFLD(pi, Core0HpFBw, maxAnalogGaindb);
	maxgain[0] = maxgain[1] = maxgain[3] = maxgain[4] = mag;
	maxgain[2] = -100;

	for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
		pi_ac->rxgainctrl_maxout_gains[i] = maxgain[i];

	high_sens_adjust_list = CHSPEC_IS2G(pi->radio_chanspec) ?
		((ACMAJORREV_40(pi->pubpi.phy_rev) || (ACMAJORREV_33(pi->pubpi.phy_rev) && ACMINORREV_4(pi))) ? high_sens_adjust_list_2g_rev40:
		high_sens_adjust_list_2g) : high_sens_adjust_list_5g;
	det_adjust = det_adjust_list[bw_indx];
	high_adjust = high_sens_adjust_list[bw_indx];

	MOD_PHYREG(pi, clip_detect_normpwr_var_mux, mClpAgcSdwTblSel, 1);

	FOREACH_CORE(pi, core) {
		sens_hi = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, 20, 0, 0, core);
		sens_lna1Byp_lo = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, 60, 0, 1, core);
		sens_lna1Byp_hi = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, 20, 0, 1, core);
		lna1Byp_rssi_idx = wlc_phy_mclip_agc_calc_lna_bypass_rssi(pi, mclip_rssi[core],
			sens_lna1Byp_lo, sens_hi);
		lna1Byp_gain_idx = lna1Byp_rssi_idx + 1;
		if  (ACMAJORREV_40(pi->pubpi.phy_rev) || (ACMAJORREV_33(pi->pubpi.phy_rev) && ACMINORREV_4(pi)))	lna1Byp_gain_idx = 100;
		elnaByp_gain_idx = 100;

		if (elna_present) {
			sens_elnaByp_lo = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, 60, 1, 0,
				core);
			elnaByp_rssi_idx = wlc_phy_mclip_agc_calc_lna_bypass_rssi(pi,
				mclip_rssi[core], sens_elnaByp_lo, sens_lna1Byp_hi);
			elnaByp_gain_idx = elnaByp_rssi_idx + 1;
		}
		PHY_INFORM(("sens_hi %d sens_lna1Byp_lo %d sens_lna1Byp_hi %d, lna1Byp_rssi_idx %d,"
			"sens_elnaByp_lo %d elnaByp_rssi_idx %d\n",
			sens_hi, sens_lna1Byp_lo, sens_lna1Byp_hi, lna1Byp_rssi_idx,
			sens_elnaByp_lo, elnaByp_rssi_idx));

		for (i = 0; i < 10; i++) {
			if (i == 0) {
				rssi = mclip_rssi[core][0];
				sens_high = rssi + margin_high_sens;
				clipgain = high_adjust - sens_high;
			} else if (i == 9) {
				rssi = mclip_rssi[core][8];
				sens_low = rssi - margin_low_sens;
				clipgain = 0 - sens_low - det_adjust;
			} else {
				rssi_low = mclip_rssi[core][i-1];
				rssi_high = mclip_rssi[core][i];
				sens_low = rssi_low - margin_low_sens;
				sens_high = rssi_high + margin_high_sens;
				min_gain = 0 - sens_low - det_adjust;
				max_gain = high_adjust - sens_high;
				clipgain = (min_gain + max_gain)>>1;
			}

			if (elna_present) {
				trtx = (i >= elnaByp_gain_idx) ? 1 : 0;
				lna1byp = (i >= lna1Byp_gain_idx) && (trtx == 0);
			} else {
				trtx = 0;
				lna1byp = 0;
			}

#ifdef BCM7271
			// Eagle: first clipgain then core
			wlc_phy_rxgainctrl_encode_gain_acphy(pi, 7, core, clipgain, trtx, lna1byp, gain_indx);
#else // BCM7271
			// Trunk: first core then clipgain
			wlc_phy_rxgainctrl_encode_gain_acphy(pi, core, clipgain, trtx, lna1byp, 7, gain_indx);
#endif // BCM7271
			mclip_gaincode[i] = wlc_phy_mclip_agc_pack_gains(pi, gain_indx, trtx,
				lna1byp);
			PHY_INFORM(("mclip_agc: clip%d code 0x%08x\n", i, mclip_gaincode[i]));
		}
		/* program clip gain */
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RSSICLIPGAIN0+core*32, 10, 1, 32,
			mclip_gaincode);
	}

	MOD_PHYREG(pi, clip_detect_normpwr_var_mux, mClpAgcSdwTblSel, 1);

}


static void wlc_phy_mclip_agc_clipcnt_thresh(phy_info_t *pi, uint8 core, const uint8 thresh_list[])
{
	/* W1 HI/MD/LW */
	MOD_PHYREGC(pi, mClpAgcW1ClipCntTh,  core, mClpAgcW1ClipCntThHi, thresh_list[0]);
	MOD_PHYREGC(pi, mClpAgcMDClipCntTh2, core, mClpAgcW1ClipCntThMd, thresh_list[1]);
	MOD_PHYREGC(pi, mClpAgcW1ClipCntTh,  core, mClpAgcW1ClipCntThLo, thresh_list[2]);
	/* W3 HI/MD/LW */
	MOD_PHYREGC(pi, mClpAgcW3ClipCntTh,  core, mClpAgcW3ClipCntThHi, thresh_list[3]);
	MOD_PHYREGC(pi, mClpAgcMDClipCntTh1, core, mClpAgcW3ClipCntThMd, thresh_list[4]);
	MOD_PHYREGC(pi, mClpAgcW3ClipCntTh,  core, mClpAgcW3ClipCntThLo, thresh_list[5]);
	/* NB HI/MD/LW */
	MOD_PHYREGC(pi, mClpAgcNbClipCntTh,  core, mClpAgcNbClipCntThHi, thresh_list[6]);
	MOD_PHYREGC(pi, mClpAgcMDClipCntTh1, core, mClpAgcNbClipCntThMd, thresh_list[7]);
	MOD_PHYREGC(pi, mClpAgcNbClipCntTh,  core, mClpAgcNbClipCntThLo, thresh_list[8]);

}

static uint8
wlc_phy_mclip_agc_calc_lna_bypass_rssi(phy_info_t *pi, int8 *mclip_rssi, int8 lo, int8 hi)
{
	int8 target_rssi, rssi_diff, min_rssi_diff = 30, min_rssi_idx = 0, i;
	target_rssi = (lo+hi)>>1;

	for (i = 0; i < 9; i++) {
		rssi_diff = ABS(mclip_rssi[i] - target_rssi);
		if (rssi_diff < min_rssi_diff) {
			min_rssi_diff = rssi_diff;
			min_rssi_idx = i;
		}
	}
	return min_rssi_idx;
}

static uint32
wlc_phy_mclip_agc_pack_gains(phy_info_t *pi, uint8 *gain_indx, uint8 trtx, uint8 lna1byp)
{
	uint32 gaincode;
	gaincode = ((lna1byp&0x1)<<23) + (1<<22) +
		((gain_indx[6]&0xF)<<18) +
		((gain_indx[5]&0x7)<<15) +
		((gain_indx[4]&0x7)<<12) +
		((gain_indx[3]&0xF)<< 8) +
		((gain_indx[2]&0x7)<< 5) +
		((gain_indx[1]&0x7)<< 2) +
		((gain_indx[0]&0x1)<< 1) + (trtx&0x1);

	return gaincode;

}

void
wlc_phy_lesi_acphy(phy_info_t *pi, bool on)
{
	  phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	  if (pi_ac->lesi > 0)  {
	    if (on) {
		MOD_PHYREG(pi, lesi_control, lesiFstrEn, 1);
		MOD_PHYREG(pi, lesi_control, lesiCrsEn, 1);
		MOD_PHYREG(pi, lesiCrsTypRxPowerPerCore, PowerLevelPerCore,
			CHSPEC_IS20(pi->radio_chanspec) ?
			0x15b : CHSPEC_IS40(pi->radio_chanspec) ? 0x228 : 0x308);
		MOD_PHYREG(pi, lesiCrsHighRxPowerPerCore, PowerLevelPerCore,
			CHSPEC_IS20(pi->radio_chanspec) ?
			0x76e : CHSPEC_IS40(pi->radio_chanspec) ? 0x9c1 : 0xAA2);
		MOD_PHYREG(pi, lesiCrsMinRxPowerPerCore, PowerLevelPerCore,
			CHSPEC_IS20(pi->radio_chanspec) ?
			0x5 : CHSPEC_IS40(pi->radio_chanspec) ? 0x18 : 0x4);
		MOD_PHYREG(pi, lesiCrs1stDetThreshold_1, crsDetTh1_1Core,
			CHSPEC_IS20(pi->radio_chanspec) ?
			0x41 : CHSPEC_IS40(pi->radio_chanspec) ? 0x2d : 0x23);
		MOD_PHYREG(pi, lesiCrs1stDetThreshold_1, crsDetTh1_2Core,
			CHSPEC_IS20(pi->radio_chanspec) ?
			0x30 : CHSPEC_IS40(pi->radio_chanspec) ? 0x20 : 0x1b);
		MOD_PHYREG(pi, lesiCrs1stDetThreshold_2, crsDetTh1_3Core,
			CHSPEC_IS20(pi->radio_chanspec) ?
			0x28 : CHSPEC_IS40(pi->radio_chanspec) ? 0x1b : 0x17);
		MOD_PHYREG(pi, lesiCrs1stDetThreshold_2, crsDetTh1_4Core,
			CHSPEC_IS20(pi->radio_chanspec) ?
			0x23 : CHSPEC_IS40(pi->radio_chanspec) ? 0x18 : 0x14);
		MOD_PHYREG(pi, lesiCrs2ndDetThreshold_1, crsDetTh1_1Core,
			CHSPEC_IS20(pi->radio_chanspec) ?
			0x41 : CHSPEC_IS40(pi->radio_chanspec) ? 0x2d : 0x23);
		MOD_PHYREG(pi, lesiCrs2ndDetThreshold_1, crsDetTh1_2Core,
			CHSPEC_IS20(pi->radio_chanspec) ?
			0x30 : CHSPEC_IS40(pi->radio_chanspec) ? 0x20 : 0x1b);
		MOD_PHYREG(pi, lesiCrs2ndDetThreshold_2, crsDetTh1_3Core,
			CHSPEC_IS20(pi->radio_chanspec) ?
			0x28 : CHSPEC_IS40(pi->radio_chanspec) ? 0x1b : 0x17);
		MOD_PHYREG(pi, lesiCrs2ndDetThreshold_2, crsDetTh1_4Core,
			CHSPEC_IS20(pi->radio_chanspec) ?
			0x23 : CHSPEC_IS40(pi->radio_chanspec) ? 0x18 : 0x14);
		MOD_PHYREG(pi, lesiFstrControl3, cCrsFftInpAdj, CHSPEC_IS20(pi->radio_chanspec) ?
			0x0 : CHSPEC_IS40(pi->radio_chanspec) ? 0x1 : 0x3);
		MOD_PHYREG(pi, lesiFstrClassifyThreshold0, MaxScaleHighValue, 0x5a);
		if (ACMAJORREV_33(pi->pubpi.phy_rev)) {
		    /* the default value caused the degradation on SGI for C1s4 */
		    MOD_PHYREG(pi, lesiFstrControl5, lesi_sgi_hw_adj, 0xf);
		  }

	    } else {
		    MOD_PHYREG(pi, lesi_control, lesiFstrEn, 0);
		    MOD_PHYREG(pi, lesi_control, lesiCrsEn, 0);
	    }
	  }

	  if (pi_ac->lesi > 0 && PHY_AS_80P80(pi, pi->radio_chanspec)) {
		    MOD_PHYREG(pi, lesi_control, lesiFstrEn, 0);
		    MOD_PHYREG(pi, lesi_control, lesiCrsEn, 0);
	  }
}

static void
wlc_phy_update_curr_desense_acphy(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	acphy_desense_values_t *total_desense, *curr_desense;
	total_desense = pi_ac->total_desense;
	curr_desense = pi_ac->curr_desense;
	memcpy(curr_desense, total_desense, sizeof(acphy_desense_values_t));
}
