/*
 * HiRSSI eLNA Bypass module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_hirssi.h 633216 2016-04-21 20:17:37Z vyass $
 */

#ifndef _phy_type_hirssi_h_
#define _phy_type_hirssi_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_hirssi.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_hirssi_ctx_t;
typedef int (*phy_type_hirssi_fn_t)(phy_type_hirssi_ctx_t *ctx);
typedef int (*phy_type_hirssi_get_period_fn_t)(phy_type_hirssi_ctx_t *ctx, int32 *period);
typedef int (*phy_type_hirssi_set_period_fn_t)(phy_type_hirssi_ctx_t *ctx, int32 period);
typedef int (*phy_type_hirssi_get_en_fn_t)(phy_type_hirssi_ctx_t *ctx, int32 *enable);
typedef int (*phy_type_hirssi_set_en_fn_t)(phy_type_hirssi_ctx_t *ctx, int32 enable);
typedef int (*phy_type_hirssi_get_rssi_fn_t)(phy_type_hirssi_ctx_t *ctx, int32 *rssi, phy_hirssi_t opt);
typedef int (*phy_type_hirssi_set_rssi_fn_t)(phy_type_hirssi_ctx_t *ctx, int32 rssi, phy_hirssi_t opt);
typedef int (*phy_type_hirssi_get_cnt_fn_t)(phy_type_hirssi_ctx_t *ctx, int32 *cnt, phy_hirssi_t opt);
typedef int (*phy_type_hirssi_set_cnt_fn_t)(phy_type_hirssi_ctx_t *ctx, int32 cnt, phy_hirssi_t opt);
typedef int (*phy_type_hirssi_get_status_fn_t)(phy_type_hirssi_ctx_t *ctx, int32 *status);

typedef struct {
	/* init */
	phy_type_hirssi_fn_t init_hirssi;
	/* get period */
	phy_type_hirssi_get_period_fn_t getperiod;
	/* set period */
	phy_type_hirssi_set_period_fn_t setperiod;
	/* get enable flag */
	phy_type_hirssi_get_en_fn_t geten;
	/* set enable flag */
	phy_type_hirssi_set_en_fn_t seten;
	/* get rssi */
	phy_type_hirssi_get_rssi_fn_t getrssi;
	/* set rssi */
	phy_type_hirssi_set_rssi_fn_t setrssi;
	/* get count */
	phy_type_hirssi_get_cnt_fn_t getcnt;
	/* set count */
	phy_type_hirssi_set_cnt_fn_t setcnt;
	/* get status */
	phy_type_hirssi_get_status_fn_t getstatus;
	/* context */
	phy_type_hirssi_ctx_t *ctx;
} phy_type_hirssi_fns_t;

/*
 * Register/unregister PHY type implementation to the HIRSSI module.
 *
 * It returns BCME_XXXX.
 */
int phy_hirssi_register_impl(phy_hirssi_info_t *hirssi, phy_type_hirssi_fns_t *fns);
void phy_hirssi_unregister_impl(phy_hirssi_info_t *hirssi);

#endif /* _phy_type_hirssi_h_ */
