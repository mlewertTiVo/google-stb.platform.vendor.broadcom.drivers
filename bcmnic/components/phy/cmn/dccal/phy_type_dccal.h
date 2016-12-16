/*
 * dccal module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_dccal.h 606042 2015-12-14 06:21:23Z $
 */

#ifndef _phy_type_dccal_h_
#define _phy_type_dccal_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_dccal.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_dccal_ctx_t;

typedef struct {
	phy_type_dccal_ctx_t *ctx;
} phy_type_dccal_fns_t;

int phy_dccal_register_impl(phy_dccal_info_t *dccali, phy_type_dccal_fns_t *fns);
void phy_dccal_unregister_impl(phy_dccal_info_t *dccali);

#endif /* _phy_type_dccal_h_ */
