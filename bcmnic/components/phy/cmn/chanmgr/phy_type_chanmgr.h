/*
 * Channel manager interface (to PHY specific implementations).
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
 * $Id: phy_type_chanmgr.h 635757 2016-05-05 05:18:39Z vyass $
 */

#ifndef _phy_type_chanmgr_h_
#define _phy_type_chanmgr_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_chanmgr.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_chanmgr_ctx_t;
typedef int (*phy_type_chanmgr_init_fn_t)(phy_type_chanmgr_ctx_t *ctx);
typedef bool (*phy_type_chanmgr_is_txbfcal_fn_t)(phy_type_chanmgr_ctx_t *ctx);
typedef int (*phy_type_chanmgr_get_chanspec_bandrange_fn_t)(phy_type_chanmgr_ctx_t *ctx,
    chanspec_t chanspec);
typedef void (*phy_type_chanmgr_chanspec_set_fn_t)(phy_type_chanmgr_ctx_t *ctx,
    chanspec_t chanspec);
typedef void (*phy_type_chanmgr_upd_interf_mode_fn_t)(phy_type_chanmgr_ctx_t *ctx,
    chanspec_t chanspec);
typedef void (*phy_type_chanmgr_preempt_fn_t)(phy_type_chanmgr_ctx_t *ctx, bool enable_preempt,
    bool EnablePostRxFilter_Proc);
typedef int (*phy_type_chanmgr_get_fn_t)(phy_type_chanmgr_ctx_t *ctx, int32 *ret_int_ptr);
typedef int (*phy_type_chanmgr_set_fn_t)(phy_type_chanmgr_ctx_t *ctx, int8 int_val);
typedef int (*phy_type_chanmgr_dump_fn_t)(phy_type_chanmgr_ctx_t *ctx, struct bcmstrbuf *b);
typedef struct {
	/* Chanmgr Init */
	phy_type_chanmgr_init_fn_t init_chanspec;
	/* TXBF cal? */
	phy_type_chanmgr_is_txbfcal_fn_t is_txbfcal;
	/* get channel frequency band range */
	phy_type_chanmgr_get_chanspec_bandrange_fn_t get_bandrange;
	/* set channel specification */
	phy_type_chanmgr_chanspec_set_fn_t chanspec_set;
	/* update interference mode */
	phy_type_chanmgr_upd_interf_mode_fn_t interfmode_upd;
	/* pre-empt */
	phy_type_chanmgr_preempt_fn_t preempt;
	/* get smth */
	phy_type_chanmgr_get_fn_t get_smth;
	/* set smth */
	phy_type_chanmgr_set_fn_t set_smth;
	/* context */
	phy_type_chanmgr_ctx_t *ctx;
} phy_type_chanmgr_fns_t;

/*
 * Register/unregister PHY type implementation to the TxPowerCtrl module.
 * It returns BCME_XXXX.
 */
int phy_chanmgr_register_impl(phy_chanmgr_info_t *cmn_info, phy_type_chanmgr_fns_t *fns);
void phy_chanmgr_unregister_impl(phy_chanmgr_info_t *cmn_info);

#endif /* _phy_type_chanmgr_h_ */
