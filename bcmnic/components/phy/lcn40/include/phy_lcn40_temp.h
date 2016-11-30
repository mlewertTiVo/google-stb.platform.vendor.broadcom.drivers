/*
 * LCN40PHY TEMPerature sense module interface (to other PHY modules).
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
 * $Id: phy_lcn40_temp.h 630449 2016-04-09 00:27:18Z vyass $
 */

#ifndef _phy_lcn40_temp_h_
#define _phy_lcn40_temp_h_

#include <phy_api.h>
#include <phy_lcn40.h>
#include <phy_temp.h>

/* forward declaration */
typedef struct phy_lcn40_temp_info phy_lcn40_temp_info_t;

/* register/unregister LCN40PHY specific implementations to/from common */
phy_lcn40_temp_info_t *phy_lcn40_temp_register_impl(phy_info_t *pi,
	phy_lcn40_info_t *lcn40i, phy_temp_info_t *tempi);
void phy_lcn40_temp_unregister_impl(phy_lcn40_temp_info_t *info);

#endif /* _phy_lcn40_temp_h_ */
