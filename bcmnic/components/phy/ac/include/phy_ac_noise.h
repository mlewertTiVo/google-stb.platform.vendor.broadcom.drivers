/*
 * ACPHY Noise module interface (to other PHY modules).
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
 * $Id: phy_ac_noise.h 673547 2016-12-02 10:42:12Z $
 */

#ifndef _phy_ac_noise_h_
#define _phy_ac_noise_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_noise.h>
#include <d11.h>

#include <phy_ac_rxspur.h>

/* forward declaration */
typedef struct phy_ac_noise_info phy_ac_noise_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_noise_info_t *phy_ac_noise_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_noise_info_t *cmn_info);
void phy_ac_noise_unregister_impl(phy_ac_noise_info_t *ac_info);

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

/* Number of tones(spurwar+nvshp) to be written */
#define ACPHY_SPURWAR_NV_NTONES                  32

/* ACI (start) */
#define ACPHY_ACI_CHAN_LIST_SZ 3

/* Number of channels affected by RSDB/DDR spur in 4349-Router chip */
#define ACPHY_NUM_SPUR_CHANS_ROUTER4349 16

/* In 53573 because of spur at max 2 tones are affected */
#define ACPHY_NSHAPETBL_MAX_TONES_ROUTER4349 2

#define MIMO_SPUR 1
#define RSDB_SPUR 2

#define ACPHY_ACI_MAX_DESENSE_BPHY_DB 24
#define ACPHY_ACI_MAX_DESENSE_OFDM_DB 48
#define ACPHY_ACI_COARSE_DESENSE_UP 4
#define ACPHY_ACI_COARSE_DESENSE_DN 4

#define ACPHY_ACI_NUM_MAX_GLITCH_AVG 2
#define ACPHY_ACI_WAIT_POST_MITIGATION 1
#define ACPHY_ACI_OFDM_HI_GLITCH_THRESH 600
#define ACPHY_ACI_OFDM_LO_GLITCH_THRESH 300
#define ACPHY_ACI_BPHY_HI_GLITCH_THRESH 300
#define ACPHY_ACI_BPHY_LO_GLITCH_THRESH 100
#define ACPHY_ACI_BORDER_GLITCH_SLEEP 12
#define ACPHY_ACI_MD_GLITCH_SLEEP 8
#define ACPHY_ACI_LO_GLITCH_SLEEP 4
#define ACPHY_ACI_GLITCH_BUFFER_SZ 4

#define ACPHY_ACI_OFDM_HI_GLITCH_THRESH_TINY 300
#define ACPHY_ACI_OFDM_LO_GLITCH_THRESH_TINY 100

/* hw aci */
#define ACPHY_HWACI_MAX_STATES 5       /* min 1 for default */
#define ACPHY_HWACI_NOACI_WAIT_TIME  8 /* sec */
#define ACPHY_HWACI_SLEEP_TIME 2       /* After a change, sleep for 2s for hwaci stats refresh */

#define HWACI_DISABLE		0
#define HWACI_AUTO_FCBS		1
#define HWACI_FORCED_MITOFF	2
#define HWACI_FORCED_MITON	3
#define HWACI_AUTO_SW		4

#define HWACI_DETECT_ENERGY_TH_HARPOON ((CHSPEC_IS5G(pi->radio_chanspec) && \
				(CHSPEC_IS40(pi->radio_chanspec))) ? 5000 : \
				(CHSPEC_IS80(pi->radio_chanspec) ? 2500 : \
				(CHSPEC_IS5G(pi->radio_chanspec) ? 5500 : 6000)))

#define HWACI_DETECT_ENERGY_TH_GODZILLA ((CHSPEC_IS5G(pi->radio_chanspec) && \
				(CHSPEC_IS40(pi->radio_chanspec))) ? 4000 : \
				(CHSPEC_IS80(pi->radio_chanspec) ? 2500 : \
				(CHSPEC_IS5G(pi->radio_chanspec) ? 6200 : 3000)))

/* ACI (end) */

/*
 * ACPHY_ENABLE_FCBS_HWACI  enables  HW ACI detection and HW mitigation thru use of the FCBS
 * This allows for the AGC to be in two states ACI and Normal.
 * This mode of operation is not compatible with the pre-exisiting
 * schemes in particular SW based desense.
 * ACPHY_ENABLE_FCBS_HWACI also disables a selection of existing ACI code.
 */
#ifndef WLC_DISABLE_ACI
#define ACPHY_ENABLE_FCBS_HWACI(pi) \
	(ACMAJORREV_3((pi)->pubpi->phy_rev) || ACMAJORREV_4((pi)->pubpi->phy_rev) || \
	ACMAJORREV_36(pi->pubpi->phy_rev) || ACMAJORREV_40(pi->pubpi->phy_rev))
#define ACPHY_HWACI_WITH_DESENSE_ENG(pi) (ACMAJORREV_4((pi)->pubpi->phy_rev) || \
	ACMAJORREV_36(pi->pubpi->phy_rev) || ACMAJORREV_40(pi->pubpi->phy_rev))
#define ACPHY_HWACI_HWTBL_MITIGATION(pi) (ACMAJORREV_33((pi)->pubpi->phy_rev))
#else
#define ACPHY_ENABLE_FCBS_HWACI(pi) 0
#define ACPHY_HWACI_WITH_DESENSE_ENG(pi) (0)
#define ACPHY_HWACI_HWTBL_MITIGATION(pi) (0)
#endif

#ifndef WLC_DISABLE_ACI
/* This Macro is for conditioning HW ACI code on all variants after 4350 c0/c1/c2 */
#define ACHWACIREV(pi) \
		((ACMAJORREV_2((pi)->pubpi->phy_rev) && \
		(ACMINORREV_0(pi) || ACMINORREV_1(pi) || ACMINORREV_4(pi))) || \
		ACMAJORREV_5((pi)->pubpi->phy_rev))

#define AC4354REV(pi) \
		(ACMAJORREV_2((pi)->pubpi->phy_rev) && !ACMINORREV_0((pi)) && \
		!ACMINORREV_1((pi)) && !ACMINORREV_4((pi)))

#else
#define ACHWACIREV(pi) 0
#define AC4354REV(pi) 0

#endif

/* Debug crash support functions */
#define PHY_AC_OFFSET_WEAKEST_RSSI		15
#define PHY_AC_DESENSE_WEAKEST_RSSI_OFDM	96
#define PHY_AC_DESENSE_WEAKEST_RSSI_BPHY	100

typedef struct acphy_desense_values
{
	uint8 clipgain_desense[4]; /* in dBs */
	uint8 ofdm_desense, bphy_desense;      /* in dBs */
	uint8 lna1_tbl_desense, lna2_tbl_desense;   /* in ticks */
	uint8 lna1_gainlmt_desense, lna2_gainlmt_desense;   /* in ticks */
	uint8 lna1rout_gainlmt_desense;
	uint8 elna_bypass;
	uint8 mixer_setting_desense;
	uint8 nf_hit_lna12; /* mostly to adjust nb/w1 clip for bt cases */
	bool on;
	bool forced;
	uint8 analog_gain_desense_ofdm, analog_gain_desense_bphy; /* in dBs */
	uint8 lna1_idx_min, lna1_idx_max;   /* in ticks */
	uint8 lna2_idx_min, lna2_idx_max;   /* in ticks */
	uint8 mix_idx_min, mix_idx_max;   /* in ticks */
}  acphy_desense_values_t;

typedef struct desense_history {
	uint32 glitches[ACPHY_ACI_GLITCH_BUFFER_SZ];
	uint8 hi_glitch_dB;
	uint8 lo_glitch_dB;
	uint8 no_desense_change_time_cnt;
} desense_history_t;

typedef struct
{
	uint16 energy_thresh;
	uint8 lna1_pktg_lmt, lna2_pktg_lmt, lna1rout_pktg_lmt;
	uint8 w2_sel, w2_thresh, nb_thresh;
	uint16 energy_thresh_w2;
	uint8 lna1_idx_min, lna1_idx_max;
	uint8 lna2_idx_min, lna2_idx_max;
	uint8 mix_idx_min, mix_idx_max;
} acphy_hwaci_state_t;

typedef struct {
	/* Saving the default gain settings in these variables */
	uint8 lna1_gainlim_ofdm_def[6];
	uint8 lna1_gainlim_cck_def[6];

	uint8 lna2_gainlim_ofdm_def[6];
	uint8 lna2_gainlim_cck_def[6];
	uint8 lna2_gainbits_def[6];
	uint8 lna2_gaindb_def[6];

} acphy_hwaci_defgain_settings_t;

/* Monitor for the Modified Entries - nvshapingtbl */
typedef struct _acphy_nshapetbl_mon {
	uint8 mod_flag;
	uint8 offset[ACPHY_SPURWAR_NV_NTONES];
} acphy_nshapetbl_mon_t;

typedef struct _acphy_router_4349_nvshptbl {
	uint16 freq;
	uint8 bw;
	uint8 offset;
	uint8 num_tones;
	uint8 nv_val;
	uint8 spur_mode;
} acphy_router_4349_nvshptbl_t;

typedef struct _aci_reg_list_entry {
	uint16 regaddraci;
	uint16 regaddr;
} aci_reg_list_entry;

typedef struct _aci_tbl_list_entry {
	uint16 tblidaci;
	uint16 tblid;
	uint16 start_offset;
	uint16 tbl_len;
	uint16 tbl_width;
} aci_tbl_list_entry;

typedef struct phy_ac_noise_data {
	aci_reg_list_entry *hwaci_phyreg_list;
	aci_tbl_list_entry *hwaci_phytbl_list;
	/* this data is shared between noise and rxgcrs */
	int8	phy_noise_all_core[PHY_CORE_MAX]; /* noise power in dB for all cores */
	uint16	gain_idx_forced;
	/* this data is shared between noise and btcx */
	uint16	pktabortctl;
	bool	current_preemption_status;
	/* this data is shared between noise, rxgcrs and btcx */
	bool	hw_aci_status;
	/* this data is shared between noise and rxgcrs */
	bool	trigger_crsmin_cal;
} phy_ac_noise_data_t;

#ifndef WLC_DISABLE_ACI
/* ACI, BT Desense (start) */
extern void wlc_phy_hwaci_init_acphy(phy_ac_noise_info_t *pi);
extern void wlc_phy_save_def_gain_settings_acphy(phy_info_t *pi);
#endif /* !WLC_DISABLE_ACI */

/* inter-module data API */
phy_ac_noise_data_t *phy_ac_noise_get_data(phy_ac_noise_info_t *noisei);
/* this is used by rxgcrs */
void phy_ac_noise_set_gainidx(phy_ac_noise_info_t *noisei, uint16 gainidx);
void phy_ac_noise_set_trigger_crsmin_cal(phy_ac_noise_info_t *noisei, bool trigger_crsmin_cal);

void chanspec_noise(phy_info_t *pi);
void wlc_phy_aci_w2nb_setup_acphy(phy_info_t *pi, bool on);
extern void wlc_phy_hwaci_setup_acphy(phy_info_t *pi, bool on, bool init);
extern uint8 wlc_phy_disable_hwaci_fcbs_trig(phy_info_t *pi);
extern void wlc_phy_restore_hwaci_fcbs_trig(phy_info_t *pi, uint8 trig_disable);
extern void wlc_phy_hwaci_mitigation_enable_acphy(phy_info_t *pi, uint8 hwaci_mode, bool init);
extern void wlc_phy_enable_hwaci_rev40(phy_info_t *pi);
extern void wlc_phy_desense_aci_engine_acphy(phy_info_t *pi);
extern void wlc_phy_reset_noise_var_shaping_acphy(phy_info_t *pi);
extern void wlc_phy_hwaci_override_acphy(phy_info_t *pi, int state);
extern void wlc_phy_hwaci_engine_acphy(phy_info_t *pi);
extern void wlc_phy_hwaci_mitigate_acphy(phy_info_t *pi, bool aci_status);
acphy_desense_values_t* phy_ac_noise_get_desense(phy_ac_noise_info_t *noisei);
uint8 phy_ac_noise_get_desense_state(phy_ac_noise_info_t *noisei);
int8 phy_ac_noise_get_weakest_rssi(phy_ac_noise_info_t *noisei);
void phy_ac_noise_hwaci_mitigation(phy_ac_noise_info_t *ni, int8 desense_state);
extern void phy_ac_noise_hwaci_switching_regs_tbls_list_init(phy_info_t *pi);
extern void wlc_phy_desense_aci_reset_params_acphy(phy_info_t *pi,
	bool call_gainctrl, bool all2g, bool all5g);
void wlc_phy_aci_updsts_acphy(phy_info_t *pi);
extern void wlc_phy_set_aci_regs_acphy(phy_info_t *pi);
extern void wlc_phy_reset_noise_var_shaping_acphy(phy_info_t *pi);
extern void wlc_phy_noise_var_shaping_acphy(phy_info_t *pi, uint8 core_nv, uint8 core_sp,
	int8 *tone_id, uint8 noise_var[][ACPHY_SPURWAR_NV_NTONES], uint8 reset);
extern void wlc_phy_switch_preemption_settings(phy_info_t *pi, uint8 state);
void phy_ac_noise_preempt(phy_ac_noise_info_t *ni, bool enable_preempt,
	bool EnablePostRxFilter_Proc);
#ifdef RADIO_HEALTH_CHECK
int phy_ac_noise_force_fail_desense(phy_ac_noise_info_t *noisei);
#endif /* RADIO_HEALTH_CHECK */
#endif /* _phy_ac_noise_h_ */
