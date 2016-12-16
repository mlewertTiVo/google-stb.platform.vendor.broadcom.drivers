/*
 * Link Power Control module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_lpc.h 666101 2016-10-19 23:52:54Z $
 */

#ifndef _phy_type_lpc_h_
#define _phy_type_lpc_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_lpc.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_lpc_ctx_t;

typedef int (*phy_type_lpc_init_fn_t)(phy_type_lpc_ctx_t *ctx);
typedef int (*phy_type_lpc_dump_fn_t)(phy_type_lpc_ctx_t *ctx, struct bcmstrbuf *b);
typedef uint8 (*phy_type_lpc_getminidx_fn_t)(void);
typedef void (*phy_type_lpc_setmode_fn_t)(phy_type_lpc_ctx_t *ctx, bool enable);
typedef uint8 (*phy_type_lpc_getpwros_fn_t)(uint8 index);
typedef uint8 (*phy_type_lpc_gettxcpwrval_fn_t)(uint16 phytxctrlword);
typedef void (*phy_type_lpc_settxcpwrval_fn_t)(uint16 *phytxctrlword, uint8 txcpwrval);
typedef uint8 (*phy_type_lpc_calcpwroffset_fn_t) (uint8 total_offset, uint8 rate_offset);
typedef uint8 (*phy_type_lpc_getpwridx_fn_t) (uint8 pwr_offset);
typedef uint8 * (*phy_type_lpc_getpwrlevelptr_fn_t) (void);

typedef struct {
	phy_type_lpc_ctx_t *ctx;

	phy_type_lpc_getminidx_fn_t		getminidx;
	phy_type_lpc_getpwros_fn_t		getpwros;
	phy_type_lpc_gettxcpwrval_fn_t		gettxcpwrval;
	phy_type_lpc_settxcpwrval_fn_t		settxcpwrval;
	phy_type_lpc_setmode_fn_t		setmode;
#ifdef WL_LPC_DEBUG
	phy_type_lpc_getpwrlevelptr_fn_t	getpwrlevelptr;
#endif

} phy_type_lpc_fns_t;

/*
 * Register/unregister PHY type implementation to the MultiPhaseCal module.
 * It returns BCME_XXXX.
 */
int phy_lpc_register_impl(phy_lpc_info_t *cmn_info, phy_type_lpc_fns_t *fns);
void phy_lpc_unregister_impl(phy_lpc_info_t *cmn_info);

#endif /* _phy_type_lpc_h_ */
