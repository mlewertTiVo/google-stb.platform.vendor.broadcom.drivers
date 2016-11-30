/*
 * ACPHY RADIO control module interface (to other PHY modules).
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
 * $Id: phy_ac_radio.h 650087 2016-07-20 12:04:23Z luka $
 */

#ifndef _phy_ac_radio_h_
#define _phy_ac_radio_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_radio.h>

#include <wlc_phytbl_20691.h>
#include <wlc_phytbl_20693.h>
#include <wlc_phytbl_20694.h>
#include <wlc_phytbl_20695.h>
#include <wlc_phytbl_20696.h>


#ifdef ATE_BUILD
#include <wl_ate.h>
#endif /* ATE_BUILD */


/** 20695 dac buffer macros */
#define BUTTERWORTH 1
#define CHEBYSHEV   2
#define BIQUAD_BYPASS 3
#define DACBW_30 30
#define DACBW_60 60
#define DACBW_120 120
#define DACBW_150 150


/* 20693 Radio functions */
typedef enum {
	RADIO_20693_FAST_ADC,
	RADIO_20693_SLOW_ADC
} radio_20693_adc_modes_t;

typedef enum {
	ACPHY_LPMODE_NONE = -1,
	ACPHY_LPMODE_NORMAL_SETTINGS,
	ACPHY_LPMODE_LOW_PWR_SETTINGS_1,
	ACPHY_LPMODE_LOW_PWR_SETTINGS_2
} acphy_lp_modes_t;

typedef struct _acphy_pmu_core1_off_radregs_t {
	bool   is_orig;
	uint16 pll_xtalldo1[PHY_CORE_MAX];
	uint16 pll_xtal_ovr1[PHY_CORE_MAX];
	uint16 pll_cp1[PHY_CORE_MAX];
	uint16 vreg_cfg[PHY_CORE_MAX];
	uint16 vreg_ovr1[PHY_CORE_MAX];
	uint16 pmu_op[PHY_CORE_MAX];
	uint16 pmu_cfg4[PHY_CORE_MAX];
	uint16 logen_cfg2[PHY_CORE_MAX];
	uint16 logen_ovr1[PHY_CORE_MAX];
	uint16 logen_cfg3[PHY_CORE_MAX];
	uint16 logen_ovr2[PHY_CORE_MAX];
	uint16 clk_div_cfg1[PHY_CORE_MAX];
	uint16 clk_div_ovr1[PHY_CORE_MAX];
	uint16 spare_cfg2[PHY_CORE_MAX];
	uint16 auxpga_ovr1[PHY_CORE_MAX];
	uint16 tx_top_2g_ovr_east[PHY_CORE_MAX];
} acphy_pmu_core1_off_radregs_t;

typedef struct _acphy_pmu_mimo_lp_opt_radregs_t {
	bool   is_orig;
	uint16 pll_xtalldo1[PHY_CORE_MAX];
	uint16 pll_xtal_ovr1[PHY_CORE_MAX];
	uint16 pmu_op[PHY_CORE_MAX];
	uint16 pmu_ovr[PHY_CORE_MAX];
	uint16 pmu_cfg4[PHY_CORE_MAX];
} acphy_pmu_mimo_lp_opt_radregs_t;

typedef struct phy_ac_radio_data {
	/* this data is shared between radio and chanmgr */
	uint16	rccal_gmult;
	uint16	rccal_gmult_rc;
	uint8	rccal_dacbuf;
	uint8	vcodivmode;
	uint8	srom_txnospurmod2g; /* 2G Tx spur optimization */
	uint8	srom_txnospurmod5g; /* 5G Tx spur optimization (Only used in ac radio) */
	/* this data is shared between radio and dsi */
	int8	use_5g_pll_for_2g; /* Parameter to indicate Use 5G PLL for 2G */
} phy_ac_radio_data_t;

/* forward declaration */
typedef struct phy_ac_radio_info phy_ac_radio_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_radio_info_t *phy_ac_radio_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_radio_info_t *ri);
void phy_ac_radio_unregister_impl(phy_ac_radio_info_t *info);

/* inter-module data API */
phy_ac_radio_data_t *phy_ac_radio_get_data(phy_ac_radio_info_t *radioi);

/* query and parse idcode */
uint32 phy_ac_radio_query_idcode(phy_info_t *pi);
void phy_ac_radio_parse_idcode(phy_info_t *pi, uint32 idcode);
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
void wlc_phy_lp_mode(phy_ac_radio_info_t *ri, int8 lp_mode);
void acphy_set_lpmode(phy_info_t *pi, acphy_lp_opt_levels_t lp_opt_lvl);
void wlc_phy_force_lpvco_2G(phy_info_t *pi, int8 force_lpvco_2G);
bool wlc_phy_poweron_dac_clocks(phy_info_t *pi, uint8 core, uint16 *orig_dac_clk_pu,
	uint16 *orig_ovr_dac_clk_pu);
void wlc_phy_restore_dac_clocks(phy_info_t *pi, uint8 core, uint16 orig_dac_clk_pu,
	uint16 orig_ovr_dac_clk_pu);
void wlc_phy_radio20693_vco_opt(phy_info_t *pi, bool isVCOTUNE_5G);
extern void wlc_phy_radio20693_mimo_lpopt_restore(phy_info_t *pi);
extern void wlc_phy_radio20693_mimo_core1_pmu_restore_on_bandhcange(phy_info_t *pi);
extern void wlc_phy_radio2069_pwrdwn_seq(phy_ac_radio_info_t *ri);
extern void wlc_phy_radio2069_pwrup_seq(phy_ac_radio_info_t *ri);
extern void wlc_acphy_get_radio_loft(phy_info_t *pi, uint8 *ei0,
	uint8 *eq0, uint8 *fi0, uint8 *fq0);
extern void wlc_acphy_set_radio_loft(phy_info_t *pi, uint8, uint8, uint8, uint8);
int wlc_phy_chan2freq_acphy(phy_info_t *pi, uint8 channel, const void **chan_info);
int wlc_phy_chan2freq_20691(phy_info_t *pi, uint8 channel, const chan_info_radio20691_t
	**chan_info);
void wlc_phy_radio20693_config_bf_mode(phy_info_t *pi, uint8 core);
int wlc_phy_chan2freq_20693(phy_info_t *pi, uint8 channel,
	const chan_info_radio20693_pll_t **chan_info_pll,
	const chan_info_radio20693_rffe_t **chan_info_rffe,
	const chan_info_radio20693_pll_wave2_t **chan_info_pll_wave2);
extern void phy_ac_radio_20693_chanspec_setup(phy_info_t *pi, uint8 ch,
	uint8 toggle_logen_reset, uint8 pllcore, uint8 mode);
void wlc_phy_radio_tiny_lpf_tx_set(phy_info_t *pi, int8 bq_bw, int8 bq_gain,
	int8 rc_bw_ofdm, int8 rc_bw_cck);
int8 wlc_phy_tiny_radio_minipmu_cal(phy_info_t *pi);
int wlc_phy_radio20693_altclkpln_get_chan_row(phy_info_t *pi);
extern void phy_ac_radio_20693_pmu_pll_config_wave2(phy_info_t *pi, uint8 pll_mode);
void wlc_phy_set_regtbl_on_band_change_acphy_20691(phy_info_t *pi);
void wlc_phy_radio_vco_opt(phy_info_t *pi, uint8 vco_mode);
void wlc_phy_radio_afecal(phy_info_t *pi);
extern void phy_ac_dsi_radio_fns(phy_info_t *pi);
extern void wlc_phy_radio20695_afe_div_ratio(phy_info_t *pi, uint8 ulp_tx_mode, uint8 ulp_adc_mode);
extern int wlc_phy_chan2freq_20695(phy_info_t *pi, uint8 channel,
	const chan_info_radio20695_rffe_t **chan_info);
//extern int wlc_phy_chan2freq_20694(phy_info_t *pi, uint8 channel);
extern int wlc_phy_chan2freq_20694(phy_info_t *pi, uint8 channel,
	const chan_info_radio20694_rffe_t **chan_info);
extern int wlc_phy_chan2freq_20696(phy_info_t *pi, uint8 channel,
	const chan_info_radio20696_rffe_t **chan_info);
extern void wlc_phy_radio20694_afe_div_ratio(phy_info_t *pi, uint8 ulp_tx_mode, uint8 ulp_adc_mode);
extern void wlc_phy_radio20696_afe_div_ratio(phy_info_t *pi);
extern void
wlc_phy_radio20695_txdac_bw_setup(phy_info_t *pi, uint8 filter_type, uint8 dacbw);
extern void
wlc_phy_radio20694_txdac_bw_setup(phy_info_t *pi, uint8 filter_type, uint8 dacbw);
extern void
wlc_phy_radio20696_txdac_bw_setup(phy_info_t *pi, uint8 filter_type, uint8 dacbw);

/* Cleanup Chanspec */
extern void chanspec_setup_radio(phy_info_t *pi);
extern void chanspec_tune_radio(phy_info_t *pi);

/* Obtain DAC rate for different modes */
uint16 wlc_phy_get_dac_rate_from_mode(phy_info_t *pi, uint8 dac_rate_mode);
void wlc_phy_dac_rate_mode_acphy(phy_info_t *pi, uint8 dac_rate_mode);

void phy_ac_radio_cal_init(phy_info_t *pi);
void phy_ac_radio_cal_reset(phy_info_t *pi, int16 idac_i, int16 idac_q);

#endif /* _phy_ac_radio_h_ */
