/*
 * TxPowerCtrl module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_txpwrcap.h 644299 2016-06-18 03:37:28Z gpasrija $
 */

#ifndef _phy_type_txpwrcap_h_
#define _phy_type_txpwrcap_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_txpwrcap.h>
#include <phy_txpwrcap_api.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_txpwrcap_ctx_t;

typedef int (*phy_type_txpwrcap_init_fn_t)(phy_type_txpwrcap_ctx_t *ctx);
typedef int (*phy_type_txpwrcap_tbl_set_fn_t)(phy_type_txpwrcap_ctx_t *ctx,
	wl_txpwrcap_tbl_t *txpwrcap_tbl);
typedef int (*phy_type_txpwrcap_tbl_get_fn_t)(phy_type_txpwrcap_ctx_t *ctx,
	wl_txpwrcap_tbl_t *txpwrcap_tbl);
typedef void (*phy_type_txpwrcap_cellstatus_set_fn_t)(phy_type_txpwrcap_ctx_t *ctx,
	int mask, int value);
typedef bool (*phy_type_txpwrcap_cellstatus_get_fn_t)(phy_type_txpwrcap_ctx_t *ctx);
typedef void (*phy_type_txpwrcap_to_shm_fn_t)(phy_type_txpwrcap_ctx_t *ctx,
	uint16 tx_ant, uint16 rx_ant);
typedef uint32 (*phy_type_txpwrcap_in_use_fn_t)(phy_type_txpwrcap_ctx_t *ctx);

typedef struct {
	/* init module including h/w */
	phy_type_txpwrcap_init_fn_t init;
	/* Txpwrcap Table Set */
	phy_type_txpwrcap_tbl_set_fn_t txpwrcap_tbl_set;
	/* Txpwrcap Table Get */
	phy_type_txpwrcap_tbl_get_fn_t txpwrcap_tbl_get;
	/* Txpwrcap Cellstatus Set */
	phy_type_txpwrcap_cellstatus_set_fn_t txpwrcap_set_cellstatus;
	/* Txpwrcap Cellstatus Get */
	phy_type_txpwrcap_cellstatus_get_fn_t txpwrcap_get_cellstatus;
	/* txpwrcap values in shm along with diversity */
	phy_type_txpwrcap_to_shm_fn_t txpwrcap_to_shm;
	/* txpwrcap values in shm along with diversity */
	phy_type_txpwrcap_in_use_fn_t txpwrcap_in_use;
	/* context */
	phy_type_txpwrcap_ctx_t *ctx;
} phy_type_txpwrcap_fns_t;

#if defined(WLC_TXPWRCAP)
/*
 * Register/unregister PHY type implementation to the TxPowerCtrl module.
 * It returns BCME_XXXX.
 */
int phy_txpwrcap_register_impl(phy_txpwrcap_info_t *mi, phy_type_txpwrcap_fns_t *fns);
void phy_txpwrcap_unregister_impl(phy_txpwrcap_info_t *mi);
#endif /* WLC_TXPWRCAP */

#endif /* _phy_type_txpwrcap_h_ */
