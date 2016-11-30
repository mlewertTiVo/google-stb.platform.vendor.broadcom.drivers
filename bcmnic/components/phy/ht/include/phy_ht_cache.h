/*
 * HTPHY Calibration Cache module interface (to other PHY modules).
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
 * $Id: phy_ht_cache.h 606042 2015-12-14 06:21:23Z jqliu $
 */

#ifndef _phy_ht_cache_h_
#define _phy_ht_cache_h_

#include <phy_api.h>
#include <phy_ht.h>
#include <phy_cache.h>

#include <bcmutils.h>
#include <wlc_phy_int.h> /* *** !!! To be removed !!! *** */

/* forward declaration */
typedef struct phy_ht_cache_info phy_ht_cache_info_t;

/* register/unregister HT specific implementations to/from common */
phy_ht_cache_info_t *phy_ht_cache_register_impl(phy_info_t *pi,
	phy_ht_info_t *hti, phy_cache_info_t *cmn_info);
void phy_ht_cache_unregister_impl(phy_ht_cache_info_t *ht_info);


/*
 * !!! To be removed !!!
 * TODO: When the following function is moved to proper module, include module header
 * and remove following function declaration
 */
extern void
wlc_phy_cal_txiqlo_coeffs_htphy(phy_info_t *pi, uint8 rd_wr, uint16 *coeff_vals,
                                uint8 select, uint8 core);

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
#endif /* _phy_ht_cache_h_ */
