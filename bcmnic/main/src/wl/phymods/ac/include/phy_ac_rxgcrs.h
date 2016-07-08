/*
 * ACPHY Rx Gain Control and Carrier Sense module interface (to other PHY modules).
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

#ifndef _phy_ac_rxgcrs_h_
#define _phy_ac_rxgcrs_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_rxgcrs.h>

/* forward declaration */
typedef struct phy_ac_rxgcrs_info phy_ac_rxgcrs_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_rxgcrs_info_t *phy_ac_rxgcrs_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_rxgcrs_info_t *cmn_info);
void phy_ac_rxgcrs_unregister_impl(phy_ac_rxgcrs_info_t *ac_info);


/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

/* ********************************* */
/*		Rx gain related definitions		*/
/* ********************************* */
/* gainctrl */
/* stages: elna, lna1, lna2, mix, bq0, bq1, dvga (tiny only) */
#define ACPHY_MAX_RX_GAIN_STAGES 7
#define ACPHY_INIT_GAIN 69
#define ACPHY_HI_GAIN 48
#define ACPHY_CRSMIN_DEFAULT 54
#define ACPHY_CRSMIN_GAINHI 32
#define ACPHY_MAX_LNA1_IDX 5
#define ACPHY_4365_MAX_LNA1_IDX 4
#define ACPHY_ILNA2G_MAX_LNA2_IDX 6
#define ACPHY_ELNA2G_MAX_LNA2_IDX 4
#define ACPHY_ELNA2G_MAX_LNA2_IDX_L 5
#define ACPHY_ILNA5G_MAX_LNA2_IDX 6
#define ACPHY_ELNA5G_MAX_LNA2_IDX 6
#define ACPHY_LESISCALE_DEFAULT 64
#define ACPHY_LO_NF_MODE_ELNA_28NM(pi) (IS_28NM_RADIO(pi) && \
	((CHSPEC_IS2G(pi->radio_chanspec)) ? BF_ELNA_2G(pi->u.pi_acphy) : \
	BF_ELNA_5G(pi->u.pi_acphy)) && pi->u.pi_acphy->lonf_elna_mode)

/* Following MACROS will be used by GBD only.
 * Tiny gain control has a local array of INIT and clip gains.
 */
#define ACPHY_INIT_GAIN_TINY 61
#define ACPHY_HI_GAIN_TINY 37
/* End of GBD used MACRO
 */
#define ACPHY_INIT_GAIN_28NM_ULP 65
#define ACPHY_HI_GAIN_28NM_ULP 44
#define ACPHY_INIT_GAIN_28NM 67

#define ACPHY_20695_MAX_LNA2_IDX 2
#define MAX_ANALOG_RX_GAIN_28NM_ULP 62
#define MAX_ANALOG_RX_GAIN_28NM_ULP_LONF 65

#define ACPHY_INIT_GAIN_4365_2G 67
#define ACPHY_INIT_GAIN_4365_5G 64

#define ACPHY_20693_MAX_LNA2_IDX 3
#define ACPHY_4365_MAX_LNA2_IDX 2
#define MAX_ANALOG_RX_GAIN_TINY 55
/* Programming this value into gainlimittable for an index prevents AGC from using it */
#define GAINLIMIT_MAX_OUT 127

#define ELNA_ID 0
#define LNA1_ID 1
#define LNA2_ID 2
#define TIA_ID	3
#define BQ0_ID 4
#define BQ1_ID 5
#define DVGA_ID 6

#define ELNA_OFFSET             0
#define EXT_LNA_OFFSET		0
#define LNA1_OFFSET		8
#define LNA2_OFFSET		16
#define TIA_OFFSET		32
#define BIQ0_OFFSET		96
#define BQ0_OFFSET              96
#define BQ1_OFFSET             112
#define DVGA1_OFFSET	       112

#define INIT_GAIN 0
#define HIGH_GAIN 1
#define MID_GAIN 2
#define LOW_GAIN 3
#define CLIP2_GAIN 4
#define SSAGC_CLIP_GAIN 5
#define GAINLMT_TBL_BAND_OFFSET 64

/* PAPD MODE CONFIGURATION */
#define PAPD_LMS 0
#define PAPD_ANALYTIC 1
#define PAPD_ANALYTIC_WO_YREF 2

/* LNA1 bypass settings for 43012 */
#define LNA1_BYPASS_GAIN_2G_20695	-10
#define LNA1_BYPASS_GAIN_5G_20695	-7
#define LNA1_BYPASS_ROUT_2G_20695	15
#define LNA1_BYPASS_ROUT_5G_20695	8
#define LNA1_BYPASS_INDEX	6

/* RX GAIN PARAMS */
#define LNA1_TBL_IND		0
#define LNA2_TBL_IND		1
#define TIA_TBL_IND		2
#define BIQ0_TBL_IND		3
#define BIQ1_TBL_IND		4
#define RXGAIN_CONF_ELEMENTS	5
#define MAX_RX_GAINS_PER_ELEM	12
#define N_LNA12_GAINS		7
#define N_TIA_GAINS		12
#define N_BIQ01_GAINS		6

/* SSAGC clip1 gains count */
#define SSAGC_CLIP1_GAINS_CNT 19

/* SSAGC clip2 index count */
#define SSAGC_CLIP2_IDX_CNT 13

/* SSAGC clip2 table entries */
#define SSAGC_CLIP2_TBL_IDX_CNT	(SSAGC_CLIP1_GAINS_CNT * SSAGC_CLIP2_IDX_CNT)

#define SSAGC_N_CLIPCNT_THRESH 6
#define SSAGC_N_RSSI_THRESH 7

typedef struct {
	uint8 elna;
	uint8 trloss;
	uint8 elna_bypass_tr;
	uint8 lna1byp;
} acphy_fem_rxgains_t;

typedef struct acphy_rxgainctrl_params {
	int8  gaintbl[ACPHY_MAX_RX_GAIN_STAGES][15];
	uint8 gainbitstbl[ACPHY_MAX_RX_GAIN_STAGES][15];
} acphy_rxgainctrl_t;

typedef enum {
	GET_LNA_GAINCODE,
	GET_LNA_ROUT
} acphy_lna_gain_rout_t;

/* Different AGC types */
typedef enum {
	FAST_AGC,
	SINGLE_SHOT_AGC
} acphy_agc_type_t;

extern uint8 wlc_phy_rxgainctrl_encode_gain_acphy(phy_info_t *pi, uint8 clipgain,
                                                  uint8 core, int8 gain_dB,
                                                  bool trloss, bool lna1byp, uint8 *gidx);
extern uint8 wlc_phy_get_lna_gain_rout(phy_info_t *pi, uint8 idx,
	acphy_lna_gain_rout_t type);

#ifndef WLC_DISABLE_ACI
extern void wlc_phy_desense_apply_acphy(phy_info_t *pi, bool apply_desense);
extern void wlc_phy_desense_calc_total_acphy(phy_info_t *pi);
extern void wlc_phy_desense_btcoex_acphy(phy_info_t *pi, int32 mode);
#ifdef BCMLTECOEX
extern void wlc_phy_desense_ltecx_acphy(phy_info_t * pi, int32 mode);
#endif /* BCMLTECOEX */
#endif /* WLC_DISABLE_ACI */

extern void wlc_phy_rxgainctrl_gainctrl_acphy(phy_info_t *pi);
extern void wlc_phy_upd_lna1_lna2_gains_acphy(phy_info_t *pi);
extern void wlc_phy_upd_lna1_lna2_gaintbls_acphy(phy_info_t *pi, uint8 lna12);
extern void wlc_phy_upd_lna1_lna2_gainlimittbls_acphy(phy_info_t *pi, uint8 lna12);
extern void wlc_phy_rfctrl_override_rxgain_acphy(phy_info_t *pi, uint8 restore,
	rxgain_t rxgain[], rxgain_ovrd_t rxgain_ovrd[]);
extern uint8 wlc_phy_calc_extra_init_gain_acphy(phy_info_t *pi, uint8 extra_gain_3dB,
	rxgain_t rxgain[]);
extern uint8 wlc_phy_get_max_lna_index_acphy(phy_info_t *pi, uint8 lna);
extern void wlc_phy_rxgainctrl_gainctrl_acphy_tiny(phy_info_t *pi, uint8 init_desense);
extern void wlc_phy_rxgainctrl_gainctrl_acphy_28nm_ulp(phy_info_t *pi);

extern void wlc_phy_rxgainctrl_set_gaintbls_acphy(phy_info_t *pi, bool init,
	bool band_change, bool bw_change);
extern void wlc_phy_rxgainctrl_set_gaintbls_acphy_wave2(phy_info_t *pi, uint8 core,
	uint16 gain_tblid, uint16  gainbits_tblid);
extern void wlc_phy_rxgainctrl_set_gaintbls_acphy_tiny(phy_info_t *pi, uint8 core,
	uint16 gain_tblid, uint16 gainbits_tblid);
extern void wlc_phy_rxgainctrl_set_gaintbls_acphy_28nm_ulp(phy_info_t *pi, uint8 core,
	uint16 gain_tblid, uint16 gainbits_tblid);
extern void wlc_phy_rxgainctrl_set_gaintbls_acphy_28nm(phy_info_t *pi, uint8 core,
	uint16 gain_tblid, uint16 gainbits_tblid);
extern void wlc_phy_set_trloss_reg_acphy(phy_info_t *pi, int8 core);
extern void wlc_phy_set_lna1byp_reg_acphy(phy_info_t *pi, int8 core);
extern void wlc_phy_set_agc_gaintbls_acphy(phy_info_t *pi, uint32 gain_tblid, const void *gain,
	uint32 gainbits_tblid, const void *gainbits, uint32 gainlimits_tblid,
	const void *gainlimits_ofdm, const void *gainlimits_cck, uint32 offset, uint32 len);

extern void phy_ac_subband_cust_28nm_ulp(phy_info_t *pi);
extern void phy_ac_agc_config(phy_info_t *pi, uint8 agc_type);
/* ************************************* */
/*		Carrier Sense related definitions		*/
/* ************************************* */

extern void wlc_phy_force_crsmin_acphy(phy_info_t *pi, void *p);
extern void wlc_phy_force_lesiscale_acphy(phy_info_t *pi, void *p);
extern void wlc_phy_crs_min_pwr_cal_acphy(phy_info_t *pi, uint8 crsmin_cal_mode);
extern void wlc_phy_set_crs_min_pwr_acphy(phy_info_t *pi, uint8 ac_th, int8 *offset);
extern void wlc_phy_set_lesiscale_acphy(phy_info_t *pi, int8 *lesi_scale);
extern void wlc_phy_stay_in_carriersearch_acphy(phy_info_t *pi, bool enable);
extern void wlc_phy_force_gainlevel_acphy(phy_info_t *pi, int16 int_val);
extern void wlc_phy_set_srom_eu_edthresh_acphy(phy_info_t *pi);
extern void chanspec_setup_rxgcrs(phy_info_t *pi);
extern void wlc_phy_lesi_acphy(phy_info_t *pi, bool on);
#endif /* _phy_ac_rxgcrs_h_ */
