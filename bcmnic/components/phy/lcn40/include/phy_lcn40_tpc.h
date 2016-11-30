/*
 * LCN40PHY TxPowerCtrl module interface (to other PHY modules).
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
 * $Id: phy_lcn40_tpc.h 639978 2016-05-25 16:03:11Z vyass $
 */

#ifndef _phy_lcn40_tpc_h_
#define _phy_lcn40_tpc_h_

#include <phy_api.h>
#include <phy_lcn40.h>
#include <phy_tpc.h>

#define LCN40PHY_TX_PWR_CTRL_RATE_OFFSET	832
#define BTC_POWER_CLAMP 20	/* 10dBm in 1/2dBm */

/* forward declaration */
typedef struct phy_lcn40_tpc_info phy_lcn40_tpc_info_t;

/* register/unregister LCN40PHY specific implementations to/from common */
phy_lcn40_tpc_info_t *phy_lcn40_tpc_register_impl(phy_info_t *pi,
	phy_lcn40_info_t *lcn40i, phy_tpc_info_t *ri);
void phy_lcn40_tpc_unregister_impl(phy_lcn40_tpc_info_t *info);

#define wlc_lcn40phy_get_target_tx_pwr(pi) \
	((phy_utils_read_phyreg(pi, LCN40PHY_TxPwrCtrlTargetPwr) & \
		LCN40PHY_TxPwrCtrlTargetPwr_targetPwr0_MASK) >> \
		LCN40PHY_TxPwrCtrlTargetPwr_targetPwr0_SHIFT)
void wlc_lcn40phy_set_txpwr_clamp(phy_info_t *pi);
#endif /* _phy_lcn40_tpc_h_ */
