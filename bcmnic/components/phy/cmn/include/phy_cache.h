/*
 * CACHE module internal interface (to other PHY modules).
 *
 * This cache is dedicated to operating chanspec contexts (see phy_chanmgr_notif.h).
 *
 * Each cache entry once 'used' has a corresponding chanspec context (or context).
 * The current chanspec context is the chanspec context whose chanspec is programmed
 * as the current radio chanspec. The cache entry that is associated with the current
 * chanspec context is called the current cache entry.
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
 * $Id: phy_cache.h 658638 2016-09-08 19:31:07Z $
 */

#ifndef _phy_cache_h_
#define _phy_cache_h_

#include <typedefs.h>
#include <phy_api.h>
#include <wlc_chctx_reg.h>

/* forward declaration */
typedef struct phy_cache_info phy_cache_info_t;

/* ================== interface for attach/detach =================== */

/* attach/detach */
phy_cache_info_t *phy_cache_attach(phy_info_t *pi);
void phy_cache_detach(phy_cache_info_t *ci);

/* ================== interface for calibration modules =================== */

/* forward declaration */
typedef uint phy_cache_cubby_id_t;

/*
 * Reserve a cubby in each cache entry and register the client callbacks and context.
 * - 'init' callback is mandatory and is invoked whenver a cache entry is made current
 *   but before any 'save' operation is performed to the cubby. It gives the cubby
 *   client a chance to initialize its relevant states in the entry to known ones.
 * - 'save' callback is optional and is invoked to save client states to the entry.
 *   It's done when the cache entry is made non-current.
 * - 'restore' callback is mandatory and is invoked to restore client states from
 *   the entry. It happens when an cache entry is made current.
 * - 'dump' callback is optional and is for debugging and dumpping the entry.
 * - 'ccid' is the cubby ID when the function is successfully called. It is used
 *   to call other cache module functions. It is also used to register a calibration
 *   callback to the calibration management (calmgr) module.
 */
typedef wlc_chctx_client_ctx_t phy_cache_ctx_t;
typedef wlc_chctx_init_fn_t phy_cache_init_fn_t;
typedef wlc_chctx_save_fn_t phy_cache_save_fn_t;
typedef wlc_chctx_restore_fn_t phy_cache_restore_fn_t;
typedef wlc_chctx_dump_fn_t phy_cache_dump_fn_t;

int phy_cache_reserve_cubby(phy_cache_info_t *ci, phy_cache_init_fn_t init,
	phy_cache_save_fn_t save, phy_cache_restore_fn_t restore, phy_cache_dump_fn_t dump,
	phy_cache_ctx_t *ctx, uint16 size, phy_cache_cubby_id_t *ccid);

/* ================== interface for calibration mgmt. module =================== */

/*
 * Save results after a calibration phase or module is finished.
 */
#ifdef NEW_PHY_CAL_ARCH
int phy_cache_save(phy_cache_info_t *ci, phy_cache_cubby_id_t ccid);
#endif /* NEW_PHY_CAL_ARCH */

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

#endif /* _phy_cache_h_ */
