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
 * $Id: phy_type_txpwrcap.h 656306 2016-08-26 03:32:45Z $
 */

#ifndef _phy_type_txpwrcap_h_
#define _phy_type_txpwrcap_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_txpwrcap.h>
#include <phy_txpwrcap_api.h>

typedef struct phy_txpwrcap_priv_info phy_txpwrcap_priv_info_t;

typedef struct phy_txpwrcap_data {
	/* Tx Power cap vars */
	wl_txpwrcap_tbl_t *txpwrcap_tbl;
	bool	txpwrcap_cellstatus;
	bool	_txpwrcap;	/* enable/disable Tx power cap feature */
} phy_txpwrcap_data_t;

struct phy_txpwrcap_info {
	phy_txpwrcap_priv_info_t *priv;
	phy_txpwrcap_data_t *data;
};


/* Tx Pwr Cap Support */
#ifdef WLC_TXPWRCAP
#if defined(WL_ENAB_RUNTIME_CHECK)
	#define PHYTXPWRCAP_ENAB(txpwrcapi)   ((txpwrcapi)->data->_txpwrcap)
#elif defined(WLC_TXPWRCAP_DISABLED)
	#define PHYTXPWRCAP_ENAB(txpwrcapi)   0
#else
	#define PHYTXPWRCAP_ENAB(txpwrcapi)   ((txpwrcapi)->data->_txpwrcap)
#endif
#else
	#define PHYTXPWRCAP_ENAB(txpwrcapi)   0
#endif	/* WLC_TXPWRCAP */


/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_txpwrcap_ctx_t;

typedef int (*phy_type_txpwrcap_init_fn_t)(phy_type_txpwrcap_ctx_t *ctx);
typedef int (*phy_type_txpwrcap_tbl_set_fn_t)(phy_type_txpwrcap_ctx_t *ctx);
typedef void (*phy_type_txpwrcap_to_shm_fn_t)(phy_type_txpwrcap_ctx_t *ctx,
	uint16 tx_ant, uint16 rx_ant);
typedef uint32 (*phy_type_txpwrcap_in_use_fn_t)(phy_type_txpwrcap_ctx_t *ctx);
typedef void (*phy_type_txpwrcap_set_fn_t)(phy_type_txpwrcap_ctx_t *ctx);

typedef struct {
	/* init module including h/w */
	phy_type_txpwrcap_init_fn_t init;
	/* Txpwrcap Table Set */
	phy_type_txpwrcap_tbl_set_fn_t txpwrcap_tbl_set;
	/* Txpwrcap Set */
	phy_type_txpwrcap_set_fn_t txpwrcap_set;
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
