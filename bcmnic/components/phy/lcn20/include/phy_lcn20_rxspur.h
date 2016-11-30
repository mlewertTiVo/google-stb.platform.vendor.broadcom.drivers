/*
 * lcn20PHY Rx Spur canceller module interface (to other PHY modules).
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
 * $Id: $
 */

#ifndef _phy_lcn20_rxspur_h_
#define _phy_lcn20_rxspur_h_

#include <phy_api.h>
#include <phy_lcn20.h>
#include <phy_rxspur.h>

/* forward declaration */
typedef struct phy_lcn20_rxspur_info phy_lcn20_rxspur_info_t;

/* register/unregister lcn20PHY specific implementations to/from common */
phy_lcn20_rxspur_info_t *phy_lcn20_rxspur_register_impl(phy_info_t *pi,
	phy_lcn20_info_t *lcn20i, phy_rxspur_info_t *arxspuri);
void phy_lcn20_rxspur_unregister_impl(phy_lcn20_rxspur_info_t *info);

/* ------ Spur-Mode ------ */
#define	LCN20PHY_SPURMODE_FVCO_972   0
#define	LCN20PHY_SPURMODE_FVCO_980   1
#define	LCN20PHY_SPURMODE_FVCO_984   2
#define	LCN20PHY_SPURMODE_FVCO_326P4 3

#endif /* _phy_lcn20_rxspur_h_ */
