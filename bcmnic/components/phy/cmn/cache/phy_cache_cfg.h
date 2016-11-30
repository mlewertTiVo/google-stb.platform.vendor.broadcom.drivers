/*
 * CACHE module - configuration
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
 * $Id: phy_cache_cfg.h 583048 2015-08-31 16:43:34Z jqliu $
 */

#ifndef _phy_cache_cfg_h_
#define _phy_cache_cfg_h_

/* default cache registry capacity */
#ifndef PHY_CACHE_REG_SZ
#define PHY_CACHE_REG_SZ 6
#endif

/* default cache capacity */
#ifndef PHY_CACHE_SZ
#define PHY_CACHE_SZ 2
#endif

/* default cache entry allocation method */
#ifndef PHY_CACHE_PREALLOC
#define PHY_CACHE_PREALLOC 1
#endif

#endif /* _phy_cache_cfg_h_ */
