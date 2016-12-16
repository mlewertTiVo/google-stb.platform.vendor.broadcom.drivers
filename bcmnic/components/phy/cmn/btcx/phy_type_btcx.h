/*
 * BT coex module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_btcx.h 657044 2016-08-30 21:37:55Z $
 */

#ifndef _phy_type_btcx_h_
#define _phy_type_btcx_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_btcx.h>

typedef struct phy_btcx_priv_info phy_btcx_priv_info_t;

typedef struct phy_btcx_data {
	uint16	bt_period;
	bool	bt_active;
	uint16	saved_clb_sw_ctrl_mask;
} phy_btcx_data_t;

struct phy_btcx_info {
    phy_btcx_priv_info_t *priv;
    phy_btcx_data_t *data;
};

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_btcx_ctx_t;

typedef int (*phy_type_btcx_init_fn_t)(phy_type_btcx_ctx_t *ctx);
typedef void (*phy_type_btcx_fn_t)(phy_type_btcx_ctx_t *ctx);
typedef void (*phy_type_btcx_adjust_fn_t)(phy_type_btcx_ctx_t *ctx, bool btactive);
typedef void (*phy_type_btcx_set_femctrl_fn_t)(phy_type_btcx_ctx_t *ctx, int8 state, bool set);
typedef int8 (*phy_type_btcx_get_femctrl_fn_t)(phy_type_btcx_ctx_t *ctx);
typedef int (*phy_type_btcx_dump_fn_t)(phy_type_btcx_ctx_t *ctx, struct bcmstrbuf *b);
typedef int (*phy_type_btcx_get_fn_t)(phy_type_btcx_ctx_t *ctx, int32 *ret_ptr);
typedef int (*phy_type_btcx_set_fn_t)(phy_type_btcx_ctx_t *ctx, int32 int_val);
typedef int (*phy_type_btcx_mode_set_fn_t)(phy_type_btcx_ctx_t *ctx, int btc_mode);

typedef struct {
	phy_type_btcx_init_fn_t init_btcx;
	phy_type_btcx_adjust_fn_t adjust;
	phy_type_btcx_set_femctrl_fn_t set_femctrl;
	phy_type_btcx_get_femctrl_fn_t get_femctrl;
	phy_type_btcx_fn_t override_enable;
	phy_type_btcx_fn_t override_disable;
	phy_type_btcx_fn_t femctrl_mask;
	phy_type_btcx_get_fn_t get_preemptstatus;
	phy_type_btcx_set_fn_t desense_ltecx;
	phy_type_btcx_set_fn_t desense_btc;
	phy_type_btcx_set_fn_t set_restage_rxgain;
	phy_type_btcx_get_fn_t get_restage_rxgain;
	phy_type_btcx_mode_set_fn_t mode_set;
	phy_type_btcx_ctx_t *ctx;
} phy_type_btcx_fns_t;

/*
 * Register/unregister PHY type implementation to the MultiPhaseCal module.
 * It returns BCME_XXXX.
 */
int phy_btcx_register_impl(phy_btcx_info_t *cmn_info, phy_type_btcx_fns_t *fns);
void phy_btcx_unregister_impl(phy_btcx_info_t *cmn_info);

#endif /* _phy_type_btcx_h_ */
