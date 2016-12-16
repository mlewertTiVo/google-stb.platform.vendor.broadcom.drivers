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
 * $Id: phy_type_tpc.h 659961 2016-09-16 18:46:01Z $
 */

#ifndef _phy_type_tpc_h_
#define _phy_type_tpc_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_tpc.h>
#include "phy_tpc_st.h"
#include <phy_tpc_api.h>

typedef struct phy_tpc_priv_info phy_tpc_priv_info_t;

typedef struct phy_tpc_config_info {
	uint32	dynamic_sarctrl_2g; /* #ifdef WL_SAR_SIMPLE_CONTROL */
	uint32	dynamic_sarctrl_5g; /* #ifdef WL_SAR_SIMPLE_CONTROL */
	uint32	dynamic_sarctrl_2g_2; /* #ifdef WL_SAR_SIMPLE_CONTROL */
	uint32	dynamic_sarctrl_5g_2; /* #ifdef WL_SAR_SIMPLE_CONTROL */
	uint16	bphy_scale; /* force bphy_scale register */
	int8	cckpwroffset[PHY_CORE_MAX];	/* cck power offset */
	int8    cckulbpwroffset[PHY_CORE_MAX]; /* cck ULB power offset */
	int8	fccpwrch12; /* nvram var, ch12 limit value */
	int8	fccpwrch13; /* nvram var, ch13 limit value */
	int8	fccpwroverride; /* nvram var, the power limit will be activated always */
	/* pdet_range_id */
	uint8	srom_2g_pdrange_id;
	uint8	srom_5g_pdrange_id;
	/* Init base index */
	uint8	initbaseidx2govrval;
	uint8	initbaseidx5govrval;
	/* 2 range tssi */
	bool	srom_tworangetssi2g;
	bool	srom_tworangetssi5g;
} phy_tpc_config_info_t;

typedef struct phy_tpc_data {
	phy_tpc_config_info_t *cfg;
	uint32	base_index_init_invalid_frame_cnt;
	uint32	txallfrm_cnt_ref; /* #ifdef PREASSOC_PWRCTRL */
	int16	radiopwr_override; /* phy PWR_CTL override, -1=default */
	int8	deltapwrdamp;
	int8	tx_pwr_ctrl_damping_en;
	int8	tx_user_target;
	int8	sarlimit[PHY_MAX_CORES]; /* #ifdef WL_SARLIMIT || WL_SAR_SIMPLE_CONTROL */
	int8	txpwr_max_boardlim_percore[PHY_CORE_MAX];
	int8    adjusted_pwr_cap[PHY_CORE_MAX];
	uint8	base_index_init[PHY_CORE_MAX];
	uint8	base_index_cck_init[PHY_CORE_MAX];
	uint8	txpwr_percent; /* power output percentage */
	uint8	curpower_display_core;
	bool	fcc_pwr_limit_2g; /* IOVAR, enable/disable */
	bool	ovrinitbaseidx;
	bool	txpwroverride; /* override */
	bool	txpwroverrideset; /* override */
	bool	txpwrnegative;
	bool	channel_short_window; /* #ifdef PREASSOC_PWRCTRL */
	uint16 chanspec_array[8];
	char ccode[24];
	uint16 minpwrlimit_fail;
	const char* ccode_ptr;
} phy_tpc_data_t;

struct phy_tpc_info {
	phy_tpc_priv_info_t *priv;
	phy_tpc_data_t *data;
};

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_tpc_ctx_t;

typedef int (*phy_type_tpc_init_fn_t)(phy_type_tpc_ctx_t *ctx);
typedef void (*phy_type_tpc_update_olpc_cal_fn_t)(phy_type_tpc_ctx_t *ctx, bool set, bool dbg);
typedef void (*phy_type_tpc_set_fcc_pwr_limit_fn_t)(phy_type_tpc_ctx_t *ctx, bool enable);
typedef void (*phy_type_tpc_set_sarlimit_fn_t)(phy_type_tpc_ctx_t *ctx);
typedef void (*phy_type_tpc_recalc_fn_t)(phy_type_tpc_ctx_t *ctx);
typedef void (*phy_type_tpc_recalc_target_fn_t)(phy_type_tpc_ctx_t *ctx, ppr_t *tx_pwr_target,
    ppr_t *srom_max_txpwr, ppr_t *reg_txpwr_limit, ppr_t *txpwr_targets);
typedef void (*phy_type_tpc_store_setting_fn_t)(phy_type_tpc_ctx_t *ctx,
    chanspec_t previous_channel);
typedef void (*phy_type_tpc_shortwindow_upd_fn_t)(phy_type_tpc_ctx_t *ctx, bool new_channel);
typedef void (*phy_type_tpc_sromlimit_get_fn_t)(phy_type_tpc_ctx_t *ctx, chanspec_t chanspec,
    ppr_t *max_pwr, uint8 core);
typedef void (*phy_type_tpc_fn_t)(phy_type_tpc_ctx_t *ctx);
typedef bool (*phy_type_tpc_bool_fn_t)(phy_type_tpc_ctx_t *ctx);
typedef void (*phy_type_tpc_reglimit_calc_fn_t)(phy_type_tpc_ctx_t *ctx, ppr_t *txpwr,
    ppr_t *txpwr_limit, ppr_ht_mcs_rateset_t *mcs_limits);
typedef void (*phy_type_tpc_set_fn_t)(phy_type_tpc_ctx_t *ctx, ppr_t *reg_pwr);
typedef void (*phy_type_tpc_set_flags_fn_t)(phy_type_tpc_ctx_t *ctx, phy_tx_power_t *power);
typedef void (*phy_type_tpc_set_max_fn_t)(phy_type_tpc_ctx_t *ctx, phy_tx_power_t *power);
typedef int (*phy_type_tpc_dump_fn_t)(phy_type_tpc_ctx_t *ctx, struct bcmstrbuf *b);
typedef int (*phy_type_tpc_txcorepwroffset_fn_t)(phy_type_tpc_ctx_t *ctx,
	struct phy_txcore_pwr_offsets*);
typedef void (*phy_type_tpc_set_avvmid_fn_t)(phy_type_tpc_ctx_t *ctx,
	wlc_phy_avvmid_txcal_t *avvmidinfo, bool set);
typedef int (*phy_type_tpc_pavars_fn_t)(phy_type_tpc_ctx_t *ctx, void* a, void* p);
typedef int8 (*phy_type_tpc_get_maxtxpwr_lowlimit_fn_t)(phy_type_tpc_ctx_t *ctx);
typedef phy_crash_reason_t (*phy_type_tpc_health_check_baseindex_fn_t)(phy_type_tpc_ctx_t *ctx);
typedef int (*phy_type_tpc_get_est_pout_fn_t)(phy_type_tpc_ctx_t *ctx,
	uint8* est_Pout, uint8* est_Pout_adj, uint8* est_Pout_cck);
typedef void (*phy_type_tpc_dump_txpower_limits_fn_t)(phy_type_tpc_ctx_t *ctx,
	ppr_t *tx_pwr_target);
typedef struct {
	/* get TX core power offset */
	phy_type_tpc_txcorepwroffset_fn_t txcorepwroffsetget;
	/* set TX core power offset */
	phy_type_tpc_txcorepwroffset_fn_t txcorepwroffsetset;
	/* init module including h/w */
	phy_type_tpc_init_fn_t init;
	/* update cal */
	phy_type_tpc_update_olpc_cal_fn_t update_cal;
	/* set fcc power limit */
	phy_type_tpc_set_fcc_pwr_limit_fn_t set_pwr_limit;
	/* set sar limit */
	phy_type_tpc_set_sarlimit_fn_t set_sarlimit;
	/* set avvmid */
	phy_type_tpc_set_avvmid_fn_t set_avvmid;
	/* recalc target txpwr & apply to h/w */
	phy_type_tpc_recalc_fn_t recalc;
	/* recalc target txpwr - bigger outer function */
	phy_type_tpc_recalc_target_fn_t recalc_target;
	/* store tx power control setting */
	phy_type_tpc_store_setting_fn_t store_setting;
	/* update tx power control short window  */
	phy_type_tpc_shortwindow_upd_fn_t shortwindow_upd;
	/* get SROM limit */
	phy_type_tpc_sromlimit_get_fn_t get_sromlimit;
	/* check txpwr limit */
	phy_type_tpc_fn_t check;
	/* tx power ipa upd */
	phy_type_tpc_fn_t ipa_upd;
	/* tx power ipa is on */
	phy_type_tpc_bool_fn_t ipa_ison;
	/* calculate regulatory limit */
	phy_type_tpc_reglimit_calc_fn_t reglimit_calc;
	/* get hardware control */
	phy_type_tpc_bool_fn_t get_hwctrl;
	/* set txpwr */
	phy_type_tpc_set_fn_t set;
	/* set power flags */
	phy_type_tpc_set_flags_fn_t setflags;
	/* set max target power */
	phy_type_tpc_set_max_fn_t setmax;
	/* recalculate sw power control */
	phy_type_tpc_bool_fn_t recalc_sw;
	/* set  pavars */
	phy_type_tpc_pavars_fn_t set_pavars;
	/* get  pavars */
	phy_type_tpc_pavars_fn_t get_pavars;
	/* get maxpower low limit */
	phy_type_tpc_get_maxtxpwr_lowlimit_fn_t get_maxtxpwr_lowlimit;
	/* health check base index */
	phy_type_tpc_health_check_baseindex_fn_t baseindex;
	/* get_est_pout */
	phy_type_tpc_get_est_pout_fn_t get_est_pout;
	/* dump tx power limits */
	phy_type_tpc_dump_txpower_limits_fn_t dump_txpower_limits;
	/* set hardware control */
	phy_type_tpc_set_fcc_pwr_limit_fn_t set_hwctrl;
	/* context */
	phy_type_tpc_ctx_t *ctx;
} phy_type_tpc_fns_t;

/*
 * Register/unregister PHY type implementation to the TxPowerCtrl module.
 * It returns BCME_XXXX.
 */
int phy_tpc_register_impl(phy_tpc_info_t *mi, phy_type_tpc_fns_t *fns);
void phy_tpc_unregister_impl(phy_tpc_info_t *mi);

#endif /* _phy_type_tpc_h_ */
