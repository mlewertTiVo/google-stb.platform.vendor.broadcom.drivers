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
 * $Id: phy_ac_tpc.h 657820 2016-09-02 18:26:33Z $
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
extern int8 wlc_phy_tone_pwrctrl(phy_info_t *pi, int8 tx_idx, uint8 core);

extern void wlc_phy_txpwrctrl_set_target_acphy(phy_info_t *pi, uint8 pwr_qtrdbm, uint8 core);
extern void wlc_phy_txpwrctrl_config_acphy(phy_info_t *pi);

int8 wlc_phy_txpwrctrl_update_minpwr_acphy(phy_info_t *pi);
void wlc_phy_txpwrctrl_set_baseindex(phy_info_t *pi, uint8 core, uint8 baseindex, bool frame_type);
#if (defined(WLOLPC) && !defined(WLOLPC_DISABLED)) || defined(BCMDBG)
void chanspec_clr_olpc_dbg_mode(phy_ac_tpc_info_t *info);
#endif 

#ifdef WLC_TXCAL
extern void wlc_phy_txcal_olpc_idx_recal_acphy(phy_info_t *pi, bool compute_idx);
#endif	/* WLC_TXCAL */
#ifdef PHYCAL_CACHING
void phy_ac_tpc_save_cache(phy_ac_tpc_info_t *ti, ch_calcache_t *ctx);
#endif /* PHYCAL_CACHING */
extern int8
wlc_phy_calc_ppr_pwr_cap_acphy(phy_info_t *pi, uint8 core, int8 maxpwr);
extern int16 wlc_phy_calc_adjusted_cap_rgstr_acphy(phy_info_t *pi, uint8 core);
extern void phy_ac_tpc_stf_chain_get_valid(phy_ac_tpc_info_t *tpci, uint8 *txchain, uint8 *rxchain);
extern uint16 wlc_phy_set_txpwr_by_index_acphy(phy_info_t *pi, uint8 core_mask, int8 txpwrindex);
#ifdef RADIO_HEALTH_CHECK
extern int phy_ac_tpc_force_fail_baseindex(phy_ac_tpc_info_t *tpci);
#endif /* RADIO_HEALTH_CHECK */
#endif /* _phy_ac_tpc_h_ */
