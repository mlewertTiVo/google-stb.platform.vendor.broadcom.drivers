/*
 * OCL phy module internal interface (to PHY specific implementation).
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */

#ifndef _phy_type_ocl_h_
#define _phy_type_ocl_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <d11.h>
#include <phy_ocl.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */

typedef void phy_type_ocl_ctx_t;
typedef void (*phy_type_ocl_fn_t)(phy_info_t *pi, bool enable);
typedef struct {
	phy_type_ocl_fn_t ocl;
	phy_type_ocl_ctx_t *ctx;
} phy_type_ocl_fns_t;

/*
 * Register/unregister PHY type implementation.
 * Register returns BCME_XXXX.
 */
#ifdef OCL

int phy_ocl_register_impl(phy_ocl_info_t *ri, phy_type_ocl_fns_t *fns);
void phy_ocl_unregister_impl(phy_ocl_info_t *ri);

#endif /* OCL */

#endif /* _phy_type_ocl_h_ */
