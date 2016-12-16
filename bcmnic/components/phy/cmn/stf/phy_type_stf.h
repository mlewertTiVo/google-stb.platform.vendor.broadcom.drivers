/*
 * STF phy module internal interface (to PHY specific implementation).
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
 * $Id: phy_type_stf.h 659995 2016-09-16 22:23:48Z $
 */

#ifndef _phy_type_stf_h_
#define _phy_type_stf_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <d11.h>
#include <phy_stf.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_stf_ctx_t;
typedef int (*phy_type_stf_set_stf_chain_fn_t)(phy_type_stf_ctx_t *ctx,
	uint8 txchain, uint8 rxchain);
typedef int (*phy_type_stf_chain_init_fn_t)(phy_type_stf_ctx_t *ctx,
		bool txrxchain_mask, uint8 txchain, uint8 rxchain);

typedef struct {
	phy_type_stf_set_stf_chain_fn_t	set_stf_chain;
	phy_type_stf_chain_init_fn_t chain_init;

	phy_type_stf_ctx_t *ctx;
} phy_type_stf_fns_t;

/*
 * Register/unregister PHY type implementation.
 * Register returns BCME_XXXX.
 */
int phy_stf_register_impl(phy_stf_info_t *ri, phy_type_stf_fns_t *fns);
void phy_stf_unregister_impl(phy_stf_info_t *ri);

#endif /* _phy_type_stf_h_ */
