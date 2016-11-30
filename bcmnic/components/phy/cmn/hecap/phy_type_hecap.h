/*
 * HE phy module internal interface (to PHY specific implementation).
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

#ifndef _phy_type_hecap_h_
#define _phy_type_hecap_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <d11.h>
#include <phy_hecap.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */

typedef void phy_type_hecap_ctx_t;
typedef void (*phy_type_hecap_fn_t)(phy_info_t *pi, bool enable);
typedef struct {
	phy_type_hecap_fn_t hecap;
	phy_type_hecap_ctx_t *ctx;
} phy_type_hecap_fns_t;

/*
 * Register/unregister PHY type implementation.
 * Register returns BCME_XXXX.
 */
#ifdef WL11AX

int phy_hecap_register_impl(phy_hecap_info_t *ri, phy_type_hecap_fns_t *fns);
void phy_hecap_unregister_impl(phy_hecap_info_t *ri);

#endif /* WL11AX */

#endif /* _phy_type_hecap_h_ */
