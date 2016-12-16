/*
 * Miscellaneous module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_vasip.h 659421 2016-09-14 06:45:22Z $
 */

#ifndef _phy_type_vasip_h_
#define _phy_type_vasip_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_vasip.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_vasip_ctx_t;
typedef uint8 (*phy_type_vasip_get_ver_fn_t)(phy_type_vasip_ctx_t *ctx);
typedef void (*phy_type_vasip_reset_proc_fn_t)(phy_type_vasip_ctx_t *ctx, int reset);
typedef void (*phy_type_vasip_set_clk_t)(phy_type_vasip_ctx_t *ctx, bool val);
typedef void (*phy_type_vasip_write_bin_t)(phy_type_vasip_ctx_t *ctx, const uint32 vasip_code[],
		const uint nbytes);
#ifdef VASIP_SPECTRUM_ANALYSIS
typedef void (*phy_type_vasip_write_spectrum_tbl_t)(phy_type_vasip_ctx_t *ctx,
		const uint32 vasip_spectrum_tbl[], const uint nbytes_tbl);
#endif
typedef void (*phy_type_vasip_write_svmp_t)(phy_type_vasip_ctx_t *ctx,
		uint32 offset, uint16 val);
typedef void (*phy_type_vasip_read_svmp_t)(phy_type_vasip_ctx_t *ctx,
		const uint32 offset, uint16 *val);
typedef void (*phy_type_vasip_write_svmp_blk_t)(phy_type_vasip_ctx_t *ctx,
		uint32 offset, uint16 len, uint16 *val);
typedef void (*phy_type_vasip_read_svmp_blk_t)(phy_type_vasip_ctx_t *ctx,
		uint32 offset, uint16 len, uint16 *val);

typedef struct {
	phy_type_vasip_ctx_t *ctx;
	phy_type_vasip_get_ver_fn_t get_ver;
	phy_type_vasip_reset_proc_fn_t reset_proc;
	phy_type_vasip_set_clk_t set_clk;
	phy_type_vasip_write_bin_t write_bin;
#ifdef VASIP_SPECTRUM_ANALYSIS
	phy_type_vasip_write_spectrum_tbl_t write_spectrum_tbl;
#endif
	phy_type_vasip_write_svmp_t	write_svmp;
	phy_type_vasip_read_svmp_t	read_svmp;
	phy_type_vasip_write_svmp_blk_t	write_svmp_blk;
	phy_type_vasip_read_svmp_blk_t	read_svmp_blk;
} phy_type_vasip_fns_t;

/*
 * Register/unregister PHY type implementation to the vasip module.
 * It returns BCME_XXXX.
 */
int phy_vasip_register_impl(phy_vasip_info_t *mi, phy_type_vasip_fns_t *fns);
void phy_vasip_unregister_impl(phy_vasip_info_t *cmn_info);

#endif /* _phy_type_vasip_h_ */
