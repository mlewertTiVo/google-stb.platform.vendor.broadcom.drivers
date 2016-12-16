/*
 * Health check module.
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
 * $Id: phy_type_hc.h 656120 2016-08-25 08:17:57Z $
 */
#ifndef _phy_type_hc_h_
#define _phy_type_hc_h_

#include <typedefs.h>
#include <phy_hc.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_hc_ctx_t;

typedef int (*phy_type_hc_force_fail_fn_t)(phy_type_hc_ctx_t *ctx, phy_crash_reason_t dctype);
typedef struct {
	 phy_type_hc_ctx_t *ctx;
	 phy_type_hc_force_fail_fn_t force_fail;
} phy_type_hc_fns_t;

/*
 * Register/unregister PHY type implementation to the health check module.
 * It returns BCME_XXXX.
 */
int phy_hc_register_impl(phy_hc_info_t *di, phy_type_hc_fns_t *fns);
void phy_hc_unregister_impl(phy_hc_info_t *di);

#endif /* _phy_type_hc_h_ */
