/*
 * lcn20PHY TxPowerCtrl module interface (to other PHY modules).
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
 * $Id: phy_lcn20_tpc.h 639978 2016-05-25 16:03:11Z $
 */

#ifndef _phy_lcn20_tpc_h_
#define _phy_lcn20_tpc_h_

#include <phy_api.h>
#include <phy_lcn20.h>
#include <phy_tpc.h>

#define LCN20PHY_TX_PWR_CTRL_RATE_OFFSET	832
#define LCN20PHY_TX_PWR_CTRL_MAC_OFFSET		128
#define BTC_POWER_CLAMP 20 /* 10dBm in 1/2dBm */
#define LCN20_TARGET_PWR 60
#define TWO_POWER_RANGE_TXPWR_CTRL 1

/* forward declaration */
typedef struct phy_lcn20_tpc_info phy_lcn20_tpc_info_t;

/* register/unregister lcn20PHY specific implementations to/from common */
phy_lcn20_tpc_info_t *phy_lcn20_tpc_register_impl(phy_info_t *pi,
	phy_lcn20_info_t *lcn20i, phy_tpc_info_t *ri);
void phy_lcn20_tpc_unregister_impl(phy_lcn20_tpc_info_t *info);

void wlc_lcn20phy_txpwrctrl_init(phy_info_t *pi);
void wlc_lcn20phy_clear_tx_power_offsets(phy_info_t *pi);
void wlc_lcn20phy_idle_tssi_est(phy_info_t *pi);
void wlc_lcn20phy_set_txpwr_clamp(phy_info_t *pi);

#define wlc_lcn20phy_get_target_tx_pwr(pi) \
	((phy_utils_read_phyreg(pi, LCN20PHY_TxPwrCtrlTargetPwr) & \
		LCN20PHY_TxPwrCtrlTargetPwr_targetPwr0_MASK) >> \
		LCN20PHY_TxPwrCtrlTargetPwr_targetPwr0_SHIFT)
#define wlc_lcn20phy_get_tx_pwr_ctrl(pi) \
	(phy_utils_read_phyreg((pi), LCN20PHY_TxPwrCtrlCmd) & \
		(LCN20PHY_TxPwrCtrlCmd_txPwrCtrl_en_MASK | \
		LCN20PHY_TxPwrCtrlCmd_hwtxPwrCtrl_en_MASK | \
		LCN20PHY_TxPwrCtrlCmd_use_txPwrCtrlCoefs_MASK))
#define wlc_lcn20phy_tx_gain_override_enabled(pi) \
	(0 != (phy_utils_read_phyreg((pi), LCN20PHY_AfeCtrlOvr) & \
		LCN20PHY_AfeCtrlOvr_dacattctrl_ovr_MASK))
#define wlc_lcn20phy_set_target_tx_pwr(pi, target) \
		phy_utils_mod_phyreg(pi, LCN20PHY_TxPwrCtrlTargetPwr, \
			LCN20PHY_TxPwrCtrlTargetPwr_targetPwr0_MASK, \
			(uint16)(target) << LCN20PHY_TxPwrCtrlTargetPwr_targetPwr0_SHIFT)
#endif /* _phy_lcn20_tpc_h_ */
