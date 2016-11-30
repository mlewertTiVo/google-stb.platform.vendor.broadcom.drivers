/*
 * TXIQLO CAL module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_txiqlocal.h 635707 2016-05-05 00:32:31Z vyass $
 */

#ifndef _phy_type_txiqlocal_h_
#define _phy_type_txiqlocal_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_txiqlocal.h>


typedef struct phy_txiqlocal_priv_info phy_txiqlocal_priv_info_t;

typedef struct phy_txiqlocal_data {
	int8	txiqcalidx2g;
	int8	txiqcalidx5g;
} phy_txiqlocal_data_t;

struct phy_txiqlocal_info {
	phy_txiqlocal_priv_info_t *priv;
	phy_txiqlocal_data_t *data;
};

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_txiqlocal_ctx_t;

typedef int (*phy_type_txiqlocal_init_fn_t)(phy_type_txiqlocal_ctx_t *ctx);
typedef int (*phy_type_txiqlocal_dump_fn_t)(phy_type_txiqlocal_ctx_t *ctx, struct bcmstrbuf *b);
typedef void (*phy_type_txiqlocal_txiqccget_fn_t)(phy_type_txiqlocal_ctx_t *ctx, void *a);
typedef void (*phy_type_txiqlocal_txiqccset_fn_t)(phy_type_txiqlocal_ctx_t *ctx, void *b);
typedef void (*phy_type_txiqlocal_txloccget_fn_t)(phy_type_txiqlocal_ctx_t *ctx, void *a);
typedef void (*phy_type_txiqlocal_txloccset_fn_t)(phy_type_txiqlocal_ctx_t *ctx, void *b);
typedef void (*phy_type_txiqlocal_scanroam_cache_fn_t)(phy_type_txiqlocal_ctx_t *ctx, bool set);
typedef struct {
	phy_type_txiqlocal_txiqccget_fn_t	txiqccget;
	phy_type_txiqlocal_txiqccset_fn_t	txiqccset;
	phy_type_txiqlocal_txloccget_fn_t	txloccget;
	phy_type_txiqlocal_txloccset_fn_t	txloccset;
	phy_type_txiqlocal_scanroam_cache_fn_t scanroam_cache;
	phy_type_txiqlocal_ctx_t	*ctx;
} phy_type_txiqlocal_fns_t;

/*
 * Register/unregister PHY type implementation to the txiqlocal module.
 * It returns BCME_XXXX.
 */
int phy_txiqlocal_register_impl(phy_txiqlocal_info_t *cmn_info, phy_type_txiqlocal_fns_t *fns);
void phy_txiqlocal_unregister_impl(phy_txiqlocal_info_t *cmn_info);

#endif /* _phy_type_txiqlocal_h_ */
