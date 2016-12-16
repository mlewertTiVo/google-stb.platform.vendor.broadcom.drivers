/*
 * Calibration Cache module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_cache.h 595511 2015-10-27 23:12:01Z $
 */

#ifndef _phy_type_cache_h_
#define _phy_type_cache_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_cache.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_cache_ctx_t;

typedef int (*phy_type_cache_init_fn_t)(phy_type_cache_ctx_t *ctx);
typedef int (*phy_type_cache_dump_fn_t)(phy_type_cache_ctx_t *ctx, struct bcmstrbuf *b);
typedef void (*phy_type_cal_cache_fn_t)(phy_type_cache_ctx_t * cache_ctx);
typedef int  (*phy_type_cal_cache_restore_fn_t)(phy_type_cache_ctx_t * cache_ctx);
typedef void (*phy_type_cal_dump_fn_t)(phy_type_cache_ctx_t * cache_ctx, struct bcmstrbuf *b);
typedef void (*phy_type_dump_chanctx_fn_t)(phy_type_cache_ctx_t * cache_ctx, ch_calcache_t *ctx,
	struct bcmstrbuf *b);

typedef struct {
	phy_type_cal_cache_fn_t cal_cache;
	phy_type_cal_cache_restore_fn_t restore;
	phy_type_cal_dump_fn_t dump_cal;
	phy_type_dump_chanctx_fn_t dump_chanctx;
	phy_type_cache_ctx_t *ctx;
} phy_type_cache_fns_t;

/*
 * Register/unregister PHY type implementation to the MultiPhaseCal module.
 * It returns BCME_XXXX.
 */
int phy_cache_register_impl(phy_cache_info_t *cmn_info, phy_type_cache_fns_t *fns);
void phy_cache_unregister_impl(phy_cache_info_t *cmn_info);

#endif /* _phy_type_cache_h_ */
