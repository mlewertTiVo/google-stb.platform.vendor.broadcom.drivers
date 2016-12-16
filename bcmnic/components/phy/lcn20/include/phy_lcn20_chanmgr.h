/*
 * lcn20PHY Channel Manager module interface (to other PHY modules).
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
 * $Id: phy_lcn20_chanmgr.h 610412 2016-01-06 23:43:14Z $
 */

#ifndef _phy_lcn20_chanmgr_h_
#define _phy_lcn20_chanmgr_h_

#include <phy_api.h>
#include <phy_lcn20.h>
#include <phy_chanmgr.h>

/* forward declaration */
typedef struct phy_lcn20_chanmgr_info phy_lcn20_chanmgr_info_t;

/* register/unregister lcn20PHY specific implementations to/from common */
phy_lcn20_chanmgr_info_t *phy_lcn20_chanmgr_register_impl(phy_info_t *pi,
	phy_lcn20_info_t *lcn20i, phy_chanmgr_info_t *chanmgri);
void phy_lcn20_chanmgr_unregister_impl(phy_lcn20_chanmgr_info_t *info);

#endif /* _phy_lcn20_chanmgr_h_ */
