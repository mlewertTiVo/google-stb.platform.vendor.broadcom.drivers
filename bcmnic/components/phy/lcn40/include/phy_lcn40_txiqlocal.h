/*
 * LCN40 TXIQLO CAL module interface (to other PHY modules).
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
 * $Id: phy_lcn40_txiqlocal.h $
 */

#ifndef _phy_lcn40_txiqlocal_h_
#define _phy_lcn40_txiqlocal_h_

#include <phy_api.h>
#include <phy_lcn40.h>
#include <phy_txiqlocal.h>

/* forward declaration */
typedef struct phy_lcn40_txiqlocal_info phy_lcn40_txiqlocal_info_t;

/* register/unregister LCN40 specific implementations to/from common */
phy_lcn40_txiqlocal_info_t *phy_lcn40_txiqlocal_register_impl(phy_info_t *pi,
	phy_lcn40_info_t *lcn40i, phy_txiqlocal_info_t *mi);
void phy_lcn40_txiqlocal_unregister_impl(phy_lcn40_txiqlocal_info_t *info);

/* Internal API for other PHY Modules */
void wlc_lcn40phy_get_tx_iqcc(phy_info_t *pi, uint16 *a, uint16 *b);
void wlc_lcn40phy_set_tx_iqcc(phy_info_t *pi, uint16 a, uint16 b);
uint16 wlc_lcn40phy_get_tx_locc(phy_info_t *pi);
void wlc_lcn40phy_set_tx_locc(phy_info_t *pi, uint16 didq);
#endif /* _phy_lcn40_txiqlocal_h_ */
