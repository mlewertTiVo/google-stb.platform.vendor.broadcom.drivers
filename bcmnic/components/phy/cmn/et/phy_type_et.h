/*
 * Envelope Tracking module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_et.h 623369 2016-03-07 20:07:03Z $
 */

#ifndef _phy_type_et_h_
#define _phy_type_et_h_

#include <typedefs.h>
#include <phy_et.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_et_ctx_t;

typedef void (*phy_type_et_fn_t)(phy_type_et_ctx_t *ctx);
typedef struct {
	phy_type_et_ctx_t *ctx;
} phy_type_et_fns_t;

/*
 * Register/unregister PHY type implementation to the MultiPhaseCal module.
 * It returns BCME_XXXX.
 */
int phy_et_register_impl(phy_et_info_t *cmn_info, phy_type_et_fns_t *fns);
void phy_et_unregister_impl(phy_et_info_t *cmn_info);

#endif /* _phy_type_et_h_ */
