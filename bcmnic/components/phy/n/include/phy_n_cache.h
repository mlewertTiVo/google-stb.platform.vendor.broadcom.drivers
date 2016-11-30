/*
 * NPHY Cache module interface (to other PHY modules).
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
 * $Id: phy_n_cache.h 606042 2015-12-14 06:21:23Z jqliu $
 */

#ifndef _phy_n_cache_h_
#define _phy_n_cache_h_

#include <phy_api.h>
#include <phy_n.h>
#include <phy_cache.h>

#include <bcmutils.h>
#include <wlc_phy_int.h> /* *** !!! To be removed !!! *** */

/* forward declaration */
typedef struct phy_n_cache_info phy_n_cache_info_t;

/* register/unregister N specific implementations to/from common */
phy_n_cache_info_t *phy_n_cache_register_impl(phy_info_t *pi,
	phy_n_info_t *ni, phy_cache_info_t *cmn_info);
void phy_n_cache_unregister_impl(phy_n_cache_info_t *n_info);


/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
#endif /* _phy_n_cache_h_ */
