/*
 * HTPHY Rx Gain Control and Carrier Sense module interface (to other PHY modules).
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
 * $Id: phy_ht_rxgcrs.h 606042 2015-12-14 06:21:23Z jqliu $
 */

#ifndef _phy_ht_rxgcrs_h_
#define _phy_ht_rxgcrs_h_

#include <phy_api.h>
#include <phy_ht.h>
#include <phy_rxgcrs.h>

/* forward declaration */
typedef struct phy_ht_rxgcrs_info phy_ht_rxgcrs_info_t;

/* register/unregister NPHY specific implementations to/from common */
phy_ht_rxgcrs_info_t *phy_ht_rxgcrs_register_impl(phy_info_t *pi,
	phy_ht_info_t *hti, phy_rxgcrs_info_t *cmn_info);
void phy_ht_rxgcrs_unregister_impl(phy_ht_rxgcrs_info_t *ht_info);

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

#endif /* _phy_ht_rxgcrs_h_ */
