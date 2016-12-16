/*
 * Debug module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_dbg.h 658374 2016-09-07 19:38:20Z $
 */

#ifndef _phy_type_dbg_h_
#define _phy_type_dbg_h_

#include <typedefs.h>
#include <phy_dbg.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_dbg_ctx_t;

typedef void (*phy_type_dbg_txerr_dump_fn_t)(phy_type_dbg_ctx_t *ctx, uint16 err);
typedef void (*phy_type_dbg_print_phydbg_regs_fn_t)(phy_type_dbg_ctx_t *ctx);
typedef void (*phy_type_dbg_gpio_out_enab_fn_t)(phy_type_dbg_ctx_t *ctx, bool enab);
typedef uint16 (*phy_type_dbg_get_phyreg_address_fn_t)(phy_type_dbg_ctx_t *ctx, uint16 addr);
typedef int (*phy_type_dbg_test_evm)(phy_type_dbg_ctx_t *ctx, int channel, uint rate, int txpwr);
typedef int (*phy_type_dbg_test_carrier_suppress)(phy_type_dbg_ctx_t *ctx, int channel);

typedef struct {
	phy_type_dbg_ctx_t *ctx;
	phy_type_dbg_txerr_dump_fn_t txerr_dump;
	phy_type_dbg_print_phydbg_regs_fn_t print_phydbg_regs;
	phy_type_dbg_gpio_out_enab_fn_t gpio_out_enab;
	phy_type_dbg_get_phyreg_address_fn_t phyregaddr;
	phy_type_dbg_test_evm test_evm;
	phy_type_dbg_test_carrier_suppress test_carrier_suppress;
} phy_type_dbg_fns_t;

/*
 * Register/unregister PHY type implementation to the Debug module.
 * It returns BCME_XXXX.
 */
int phy_dbg_register_impl(phy_dbg_info_t *di, phy_type_dbg_fns_t *fns);
void phy_dbg_unregister_impl(phy_dbg_info_t *di);

#endif /* _phy_type_dbg_h_ */
