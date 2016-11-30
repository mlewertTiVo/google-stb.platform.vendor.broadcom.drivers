/*
 * Napping module internal interface
 * (to PHY specific implementations).
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
 * $Id$
 */

#ifndef _phy_type_nap_h_
#define _phy_type_nap_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_nap.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_nap_ctx_t;
typedef void (*phy_type_nap_get_status_fn_t)(phy_type_nap_ctx_t *ctx, uint16 *reqs,
	bool *nap_en);
typedef void (*phy_type_nap_set_disable_req_fn_t)(phy_type_nap_ctx_t *ctx, uint16 req,
	bool disable, bool agc_reconfig, uint8 req_id);

typedef struct {
	phy_type_nap_get_status_fn_t get_status;
	phy_type_nap_set_disable_req_fn_t set_disable_req;
	phy_type_nap_ctx_t *ctx;
} phy_type_nap_fns_t;

/*
 * Register/unregister PHY type implementation.
 * It returns BCME_XXXX.
 */
int phy_nap_register_impl(phy_nap_info_t *mi, phy_type_nap_fns_t *fns);
void phy_nap_unregister_impl(phy_nap_info_t *cmn_info);

#endif /* _phy_type_nap_h_ */
