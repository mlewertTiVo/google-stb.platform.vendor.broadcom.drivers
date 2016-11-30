/*
 * Cache module public interface (to MAC driver).
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
 * $Id: phy_cache_api.h 606042 2015-12-14 06:21:23Z jqliu $
 */

#ifndef _phy_cache_api_h_
#define _phy_cache_api_h_

#include <typedefs.h>
#include <phy_api.h>

int wlc_phy_cal_cache_return(wlc_phy_t *ppi);
void wlc_phy_cal_cache(wlc_phy_t *ppi);

#endif /* _phy_cache_api_h_ */
