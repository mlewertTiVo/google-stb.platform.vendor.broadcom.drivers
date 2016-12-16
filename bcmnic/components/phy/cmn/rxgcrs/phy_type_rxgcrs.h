/*
 * Rx Gain Control and Carrier Sense module internal interface
 * (to PHY specific implementations).
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
 * $Id: phy_type_rxgcrs.h 658925 2016-09-11 16:42:42Z $
 */

#ifndef _phy_type_rxgcrs_h_
#define _phy_type_rxgcrs_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_rxgcrs.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_rxgcrs_ctx_t;

typedef int (*phy_type_rxgcrs_init_fn_t)(phy_type_rxgcrs_ctx_t *ctx);
typedef int (*phy_type_rxgcrs_dump_fn_t)(phy_type_rxgcrs_ctx_t *ctx, struct bcmstrbuf *b);
typedef void (*phy_type_rxgcrs_locale_eu_fn_t)(phy_type_rxgcrs_ctx_t *ctx, uint8 region_group);
typedef void (*phy_type_rxgcrs_adjust_ed_thres_fn_t)(phy_type_rxgcrs_ctx_t *pi,
	int32 *assert_thresh_dbm, bool set_threshold);
typedef int (*phy_type_rxgcrs_get_fn_t)(phy_type_rxgcrs_ctx_t *ctx, int32 *ret_int_ptr);
typedef int (*phy_type_rxgcrs_set_fn_t)(phy_type_rxgcrs_ctx_t *ctx, int32 int_val);
typedef int (*phy_type_rxgcrs_forcecal_noise_fn_t)(phy_type_rxgcrs_ctx_t *ctx, void *a, bool set);
typedef uint16 (*phy_type_rxgcrs_sel_classifier_fn_t)(phy_type_rxgcrs_ctx_t *ctx, uint16 val);
typedef void (*phy_type_rxgcrs_stay_in_carriersearch_fn_t)(phy_type_rxgcrs_ctx_t *ctx,
	bool enable);
typedef void (*phy_type_rxgcrs_phydump_chanest_fn_t)(phy_type_rxgcrs_ctx_t *ctx,
	struct bcmstrbuf *b);
typedef void (*phy_type_rxgcrs_get_chanest_fn_t)(phy_type_rxgcrs_ctx_t *ctx, uint16 fftk,
	uint16 idx, int16 *ch_re, int16 *ch_im);
typedef int (*phy_type_rxgcrs_phydump_phycal_rx_min_fn_t)(phy_type_rxgcrs_ctx_t *ctx,
	struct bcmstrbuf *b);

typedef struct {
	phy_type_rxgcrs_ctx_t *ctx;
	phy_type_rxgcrs_locale_eu_fn_t set_locale;
	phy_type_rxgcrs_adjust_ed_thres_fn_t adjust_ed_thres;
	phy_type_rxgcrs_init_fn_t init_rxgcrs;
	phy_type_rxgcrs_get_fn_t get_rxdesens;
	phy_type_rxgcrs_set_fn_t set_rxdesens;
	phy_type_rxgcrs_get_fn_t get_rxgainindex;
	phy_type_rxgcrs_set_fn_t set_rxgainindex;
	phy_type_rxgcrs_forcecal_noise_fn_t forcecal_noise;
	phy_type_rxgcrs_sel_classifier_fn_t sel_classifier;
	phy_type_rxgcrs_stay_in_carriersearch_fn_t stay_in_carriersearch;
	phy_type_rxgcrs_phydump_chanest_fn_t	phydump_chanest;
	phy_type_rxgcrs_get_chanest_fn_t	get_chanest;
	phy_type_rxgcrs_phydump_phycal_rx_min_fn_t	phydump_phycal_rxmin;
} phy_type_rxgcrs_fns_t;

/*
 * Register/unregister PHY type implementation to the MultiPhaseCal module.
 * It returns BCME_XXXX.
 */
int phy_rxgcrs_register_impl(phy_rxgcrs_info_t *mi, phy_type_rxgcrs_fns_t *fns);
void phy_rxgcrs_unregister_impl(phy_rxgcrs_info_t *cmn_info);

#endif /* _phy_type_rxgcrs_h_ */
