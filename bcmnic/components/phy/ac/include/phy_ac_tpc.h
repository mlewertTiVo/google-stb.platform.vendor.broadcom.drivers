/*
 * ACPHY TxPowerCtrl module interface (to other PHY modules).
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
 * $Id: phy_ac_tpc.h 647804 2016-07-07 15:46:23Z ernst $
 */

#ifndef _phy_ac_tpc_h_
#define _phy_ac_tpc_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_tpc.h>

/* forward declaration */
typedef struct phy_ac_tpc_info phy_ac_tpc_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_tpc_info_t *phy_ac_tpc_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_tpc_info_t *ti);
void phy_ac_tpc_unregister_impl(phy_ac_tpc_info_t *info);

#define TSSI_DIVWAR_INDX (2)

void chanspec_setup_tpc(phy_info_t *pi);
extern uint8 wlc_phy_tssi2dbm_acphy(phy_info_t *pi, int32 tssi, int32 a1, int32 b0, int32 b1);
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD)
extern void wlc_phy_tone_pwrctrl_loop(phy_info_t *pi, int8 targetpwr_dBm);
#endif
extern void wlc_phy_get_paparams_for_band_acphy(phy_info_t *pi, int16 *a1, int16 *b0, int16 *b1);
extern void wlc_phy_read_txgain_acphy(phy_info_t *pi);
extern void wlc_phy_txpwr_by_index_acphy(phy_info_t *pi, uint8 core_mask, int8 txpwrindex);
extern void wlc_phy_get_txgain_settings_by_index_acphy(phy_info_t *pi,
	txgain_setting_t *txgain_settings, int8 txpwrindex);
extern void wlc_phy_get_tx_bbmult_acphy(phy_info_t *pi, uint16 *bb_mult, uint16 core);
extern void wlc_phy_set_tx_bbmult_acphy(phy_info_t *pi, uint16 *bb_mult, uint16 core);
extern uint32 wlc_phy_txpwr_idx_get_acphy(phy_info_t *pi);
extern void wlc_phy_txpwrctrl_enable_acphy(phy_info_t *pi, uint8 ctrl_type);
extern void wlc_phy_txpwr_fixpower_acphy(phy_info_t *pi);
extern void wlc_phy_txpwr_est_pwr_acphy(phy_info_t *pi, uint8 *Pout, uint8 *Pout_adj);
extern const uint16 *wlc_phy_get_tx_pwrctrl_tbl_2069(phy_info_t *pi);
extern const uint16 *wlc_phy_get_txgain_tbl_20695(phy_info_t *pi);
extern const uint16 *wlc_phy_get_txgain_tbl_20694(phy_info_t *pi);
extern const uint16 *wlc_phy_get_txgain_tbl_20696(phy_info_t *pi);
extern int8 wlc_phy_tone_pwrctrl(phy_info_t *pi, int8 tx_idx, uint8 core);
extern void wlc_phy_gaintbl_blanking(phy_info_t *pi, uint16 *tx_pwrctrl_tbl,
	uint8 txidxcap, bool is_max_cap);

extern void wlc_phy_txpwrctrl_set_target_acphy(phy_info_t *pi, uint8 pwr_qtrdbm, uint8 core);
extern void wlc_phy_txpwrctrl_config_acphy(phy_info_t *pi);

#if defined(BCMINTERNAL) || defined(WLTEST)
void wlc_phy_iovar_patrim_acphy(phy_info_t *pi, int32 *ret_int_ptr);
void wlc_phy_txpwr_ovrinitbaseidx(phy_info_t *pi);
#endif
int8 wlc_phy_txpwrctrl_update_minpwr_acphy(phy_info_t *pi);
void wlc_phy_txpwrctrl_set_baseindex(phy_info_t *pi, uint8 core, uint8 baseindex, bool frame_type);
#if (defined(WLOLPC) && !defined(WLOLPC_DISABLED)) || defined(BCMDBG) || \
	defined(WLTEST)
void chanspec_clr_olpc_dbg_mode(phy_ac_tpc_info_t *info);
#endif /* ((WLOLPC) && !(WLOLPC_DISABLED)) || (BCMDBG) || (WLTEST) */

#ifdef WLC_TXCAL
uint8 wlc_phy_estpwrlut_intpol_acphy(phy_info_t *pi, uint8 channel,
	wl_txcal_power_tssi_t *pwr_tssi_lut_ch1, wl_txcal_power_tssi_t *pwr_tssi_lut_ch2);
uint8 wlc_phy_set_olpc_anchor_acphy(phy_info_t *pi);
uint8 wlc_phy_olpc_idx_tempsense_comp_acphy(phy_info_t *pi, uint8 *iidx, uint8 core);
uint8 wlc_phy_txcal_olpc_idx_recal_acphy(phy_info_t *pi, bool compute_idx);
#endif	/* WLC_TXCAL */

extern int8
wlc_phy_calc_ppr_pwr_cap_acphy(phy_info_t *pi, uint8 core, int8 maxpwr);
extern int16 wlc_phy_calc_adjusted_cap_rgstr_acphy(phy_info_t *pi, uint8 core);
void phy_ac_tpc_ipa_upd(phy_info_t *pi);
extern uint16 wlc_phy_set_txpwr_by_index_acphy(phy_info_t *pi, uint8 core_mask, int8 txpwrindex);
#endif /* _phy_ac_tpc_h_ */
