/*
 * Calibration Management module internal interface (to PHY specific implementation).
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
 * $Id: phy_type_calmgr.h 659967 2016-09-16 19:35:14Z $
 */

#ifndef _phy_type_calmgr_h_
#define _phy_type_calmgr_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_calmgr.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_calmgr_ctx_t;
typedef int (*phy_type_calmgr_fn_t)(phy_type_calmgr_ctx_t *ctx);
typedef bool (*phy_type_calmgr_bool_fn_t)(phy_type_calmgr_ctx_t *ctx);
typedef int (*phy_type_calmgr_prepare_fn_t)(phy_type_calmgr_ctx_t *ctx);
typedef void (*phy_type_calmgr_cleanup_fn_t)(phy_type_calmgr_ctx_t *ctx);
typedef void (*phy_type_calmgr_cals_fn_t)(phy_type_calmgr_ctx_t *ctx,
	uint8 legacy_caltype, uint8 searchmode);
typedef void (*phy_type_calmgr_add_timer_fn_t)(phy_type_calmgr_ctx_t *ctx, uint delay_val);
typedef void (*phy_type_calmgr_set_override_fn_t)(phy_type_calmgr_ctx_t *ctx, uint8 override);
typedef int (*phy_type_calmgr_enable_initcal_fn_t)(phy_type_calmgr_ctx_t *ctx, bool initcal);
typedef struct {
	phy_type_calmgr_fn_t init;
	phy_type_calmgr_bool_fn_t wd;
	phy_type_calmgr_enable_initcal_fn_t enable_initcal;
	phy_type_calmgr_prepare_fn_t prepare;
	phy_type_calmgr_cleanup_fn_t cleanup;
	phy_type_calmgr_cals_fn_t cals;
	phy_type_calmgr_add_timer_fn_t add_timer_special;
	phy_type_calmgr_set_override_fn_t set_override;
	phy_type_calmgr_ctx_t *ctx;
} phy_type_calmgr_fns_t;

/*
 * Register/unregister PHY type implementation to the common layer.
 * It returns BCME_XXXX.
 */
int phy_calmgr_register_impl(phy_calmgr_info_t *ci, phy_type_calmgr_fns_t *fns);
void phy_calmgr_unregister_impl(phy_calmgr_info_t *ci);

#endif /* _phy_type_calmgr_h_ */
