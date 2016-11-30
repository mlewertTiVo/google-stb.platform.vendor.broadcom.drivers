/*
 * LCN20 TXIQLO CAL module interface (to other PHY modules).
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
 * $Id: phy_lcn20_txiqlocal.h $
 */

#ifndef _phy_lcn20_txiqlocal_h_
#define _phy_lcn20_txiqlocal_h_

#include <phy_api.h>
#include <phy_lcn20.h>
#include <phy_txiqlocal.h>

/* forward declaration */
typedef struct phy_lcn20_txiqlocal_info phy_lcn20_txiqlocal_info_t;

/* register/unregister lcn20PHY specific implementations to/from common */
phy_lcn20_txiqlocal_info_t *phy_lcn20_txiqlocal_register_impl(phy_info_t *pi,
	phy_lcn20_info_t *lcn20i, phy_txiqlocal_info_t *txiqlocali);
void phy_lcn20_txiqlocal_unregister_impl(phy_lcn20_txiqlocal_info_t *info);

/* Inter-module API to other PHY modules */
void wlc_phy_set_tx_iqcc_lcn20phy(phy_info_t *pi, uint16 a, uint16 b);
void wlc_phy_set_tx_locc_lcn20phy(phy_info_t *pi, uint16 didq);

#endif /* _phy_lcn20_txiqlocal_h_ */
