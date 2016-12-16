/*
 * TSSI Cal module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_tssical.h 657820 2016-09-02 18:26:33Z $
 */

#ifndef _phy_type_tssical_h_
#define _phy_type_tssical_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_tssical.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_tssical_ctx_t;

typedef int (*phy_type_tssical_init_fn_t)(phy_type_tssical_ctx_t *ctx);
typedef int8 (*phy_type_tssical_get_visible_thresh_fn_t)(phy_type_tssical_ctx_t *ctx);
typedef void (*phy_type_tssical_sens_min_fn_t)(phy_type_tssical_ctx_t *ctx, int8 *tssiSensMinPwr);
typedef int (*phy_type_tssical_dump_fn_t)(phy_type_tssical_ctx_t *ctx, struct bcmstrbuf *b);

#ifdef WLC_TXCAL
typedef void (*phy_type_tssical_compute_olpc_idx_fn_t)(phy_type_tssical_ctx_t *ctx);
typedef uint16 (*phy_type_tssical_adjusted_tssi_fn_t)(phy_type_tssical_ctx_t *ctx, uint8 core_num);
typedef uint16 (*phy_type_tssical_txpwr_ctrl_cmd_fn_t)(phy_type_tssical_ctx_t *ctx);
typedef void (*phy_type_tssical_set_txpwrindex_fn_t)(phy_type_tssical_ctx_t *ctx, int16 gain_idx,
		uint16 save_TxPwrCtrlCmd);
typedef void (*phy_type_tssical_restore_txpwr_ctrl_cmd_fn_t)(phy_type_tssical_ctx_t *ctx,
		uint16 save_TxPwrCtrlCmd, uint8 txpwr_ctrl_state);
typedef void (*phy_type_tssical_set_olpc_anchor_fn_t)(phy_type_tssical_ctx_t *ctx);
typedef void (*phy_type_tssical_iov_apply_pwr_tssi_tbl_fn_t)(phy_type_tssical_ctx_t *ctx,
		int int_val);
typedef void (*phy_type_tssical_read_est_pwr_lut_fn_t)(phy_type_tssical_ctx_t *ctx,
		void *output_buff, uint8 core);
typedef void (*phy_type_tssical_apply_paparams_fn_t)(phy_type_tssical_ctx_t *ctx);
typedef int32 (*phy_type_tssical_get_tssi_val_fn_t)(phy_type_tssical_ctx_t *ctx, int32 tssi_val);
typedef void (*phy_type_tssical_apply_pwr_tssi_tbl_fn_t)(phy_type_tssical_ctx_t *ctx,
		wl_txcal_power_tssi_t* tssi_tbl);
#endif /* WLC_TXCAL */

typedef struct {
	phy_type_tssical_get_visible_thresh_fn_t visible_thresh;
	phy_type_tssical_sens_min_fn_t sens_min;

#ifdef WLC_TXCAL
	/* compute olpc index */
	phy_type_tssical_compute_olpc_idx_fn_t compute_olpc_idx;
	/* adjusted tssi */
	phy_type_tssical_adjusted_tssi_fn_t adjusted_tssi;
	/* TxPwrCtrlCmd */
	phy_type_tssical_txpwr_ctrl_cmd_fn_t txpwr_ctrl_cmd;
	/* set index */
	phy_type_tssical_set_txpwrindex_fn_t set_txpwrindex;
	/* restore  TxPwrCtrlCmd */
	phy_type_tssical_restore_txpwr_ctrl_cmd_fn_t restore_txpwr_ctrl_cmd;
	/* set olpc anchor */
	phy_type_tssical_set_olpc_anchor_fn_t set_olpc_anchor;
	/* apply pwr tssi tbl */
	phy_type_tssical_iov_apply_pwr_tssi_tbl_fn_t iov_apply_pwr_tssi_tbl;
	/* read est pwr lut */
	phy_type_tssical_read_est_pwr_lut_fn_t read_est_pwr_lut;
	/* apply paparams */
	phy_type_tssical_apply_paparams_fn_t apply_paparams;
	/* get tssi val */
	phy_type_tssical_get_tssi_val_fn_t get_tssi_val;
	/* apply pwr tssi tbl */
	phy_type_tssical_apply_pwr_tssi_tbl_fn_t apply_pwr_tssi_tbl;
#endif /* WLC_TXCAL */

	phy_type_tssical_ctx_t *ctx;
} phy_type_tssical_fns_t;

/*
 * Register/unregister PHY type implementation to the MultiPhaseCal module.
 * It returns BCME_XXXX.
 */
int phy_tssical_register_impl(phy_tssical_info_t *cmn_info, phy_type_tssical_fns_t *fns);
void phy_tssical_unregister_impl(phy_tssical_info_t *cmn_info);

#endif /* _phy_type_tssical_h_ */
