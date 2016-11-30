/*
 * RSSI Compute module internal interface (to PHY specific implementation).
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
 * $Id: phy_type_rssi.h 642720 2016-06-09 18:56:12Z vyass $
 */

#ifndef _phy_type_rssi_h_
#define _phy_type_rssi_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <d11.h>
#include <phy_rssi.h>
#include <wlioctl.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_rssi_ctx_t;

typedef void (*phy_type_rssi_compute_fn_t)(phy_type_rssi_ctx_t *ctx, wlc_d11rxhdr_t *wrxh);
typedef void (*phy_type_rssi_init_gain_err_fn_t)(phy_type_rssi_ctx_t *ctx);
typedef int (*phy_type_rssi_dump_fn_t)(phy_type_rssi_ctx_t *ctx, struct bcmstrbuf *b);
typedef void (*phy_type_rssi_update_pkteng_rxstats_fn_t)(phy_type_rssi_ctx_t *ctx, uint8 statidx);
typedef int (*phy_type_rssi_get_pkteng_stats_fn_t)(phy_type_rssi_ctx_t *ctx, void *a, int alen,
	wl_pkteng_stats_t stats);
typedef int (*phy_type_rssi_gain_delta_fn_t)(phy_type_rssi_ctx_t *ctx, uint32 aid,
	int8 *deltaValues);
typedef int (*phy_type_rssi_int_fn_t)(phy_type_rssi_ctx_t *ctx, int8 *value);

typedef struct {
	phy_type_rssi_compute_fn_t compute;
	phy_type_rssi_init_gain_err_fn_t init_gain_err;
	phy_type_rssi_dump_fn_t dump;
	phy_type_rssi_update_pkteng_rxstats_fn_t update_pkteng_rxstats;
	phy_type_rssi_get_pkteng_stats_fn_t get_pkteng_stats;
	phy_type_rssi_gain_delta_fn_t set_gain_delta_2g;
	phy_type_rssi_gain_delta_fn_t get_gain_delta_2g;
	phy_type_rssi_gain_delta_fn_t set_gain_delta_5g;
	phy_type_rssi_gain_delta_fn_t get_gain_delta_5g;
	phy_type_rssi_gain_delta_fn_t set_gain_delta_2gb;
	phy_type_rssi_gain_delta_fn_t get_gain_delta_2gb;
	phy_type_rssi_int_fn_t set_cal_freq_2g;
	phy_type_rssi_int_fn_t get_cal_freq_2g;
	phy_type_rssi_ctx_t *ctx;
} phy_type_rssi_fns_t;

/*
 * Register/unregister PHY type implementation to the common of the RSSI Compute module.
 * It returns BCME_XXXX.
 */
int phy_rssi_register_impl(phy_rssi_info_t *ri, phy_type_rssi_fns_t *fns);
void phy_rssi_unregister_impl(phy_rssi_info_t *ri);

/*
 * Enable RSSI moving average algorithm.
 * It returns BCME_XXXX.
 */
int phy_rssi_enable_ma(phy_rssi_info_t *ri, bool enab);

#endif /* _phy_type_rssi_h_ */
