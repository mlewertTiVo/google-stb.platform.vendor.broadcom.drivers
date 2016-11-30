/*
 * LCNNPHY Cache module interface (to other PHY modules).
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
 * $Id: phy_lcn20_cache.h 606042 2015-12-14 06:21:23Z jqliu $
 */

#ifndef _phy_lcn20_cache_h_
#define _phy_lcn20_cache_h_

#include <phy_api.h>
#include <phy_lcn20.h>
#include <phy_cache.h>

#include <bcmutils.h>
#include <wlc_phy_int.h> /* *** !!! To be removed !!! *** */

/* forward declaration */
typedef struct phy_lcn20_cache_info phy_lcn20_cache_info_t;

/* register/unregister lcn20 specific implementations to/from common */
phy_lcn20_cache_info_t *phy_lcn20_cache_register_impl(phy_info_t *pi,
	phy_lcn20_info_t *lcn20i, phy_cache_info_t *cmn_info);
void phy_lcn20_cache_unregister_impl(phy_lcn20_cache_info_t *lcn20_info);

#endif /* _phy_lcn20_cache_h_ */
