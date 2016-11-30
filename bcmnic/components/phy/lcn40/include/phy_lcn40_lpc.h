/*
 * LCN40PHY Link Power Control module interface (to other PHY modules)
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
 * $Id: phy_lcn40_lpc.h $
 */

#ifndef _phy_lcn40_lpc_h_
#define _phy_lcn40_lpc_h_

#include <phy_api.h>
#include <phy_lcn40.h>
#include <phy_lpc.h>

/* forward declaration */
typedef struct phy_lcn40_lpc_info phy_lcn40_lpc_info_t;

/* register/unregister LCN40PHY specific implementations to/from common */
phy_lcn40_lpc_info_t *phy_lcn40_lpc_register_impl(phy_info_t *pi,
	phy_lcn40_info_t *lcn40i, phy_lpc_info_t *ri);
void phy_lcn40_lpc_unregister_impl(phy_lcn40_lpc_info_t *info);

void wlc_lcn40phy_lpc_write_maclut(phy_info_t *pi);

void wlc_lcn40phy_lpc_setmode(phy_info_t *pi, bool enable);
#endif /* _phy_lcn40_lpc_h_ */
