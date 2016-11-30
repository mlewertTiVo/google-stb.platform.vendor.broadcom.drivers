/*
 * Miscellaneous module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_misc.h 627541 2016-03-25 01:49:14Z hou $
 */

#ifndef _phy_type_misc_h_
#define _phy_type_misc_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_misc.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_misc_ctx_t;

typedef int (*phy_type_misc_init_fn_t)(phy_type_misc_ctx_t *ctx);
typedef int (*phy_type_misc_dump_fn_t)(phy_type_misc_ctx_t *ctx, struct bcmstrbuf *b);
typedef int (*phy_type_misc_getlistandsize_fn_t)(phy_type_misc_ctx_t *ctx,
	phyradregs_list_t **phyreglist, uint16 *phyreglist_sz);
typedef uint8 (*phy_type_vasip_get_ver_fn_t)(phy_type_misc_ctx_t *ctx);
typedef void (*phy_type_vasip_proc_reset_fn_t)(phy_type_misc_ctx_t *ctx, int reset);
typedef void (*phy_type_vasip_clk_set_t)(phy_type_misc_ctx_t *ctx, bool val);
typedef void (*phy_type_vasip_bin_write_t)(phy_type_misc_ctx_t *ctx, const uint32 vasip_code[],
		const uint nbytes);
#ifdef VASIP_SPECTRUM_ANALYSIS
typedef void (*phy_type_vasip_spectrum_tbl_write_t)(phy_type_misc_ctx_t *ctx,
		const uint32 vasip_spectrum_tbl[], const uint nbytes_tbl);
#endif
typedef void (*phy_type_misc_test_init_fn_t)(phy_type_misc_ctx_t *ctx, bool encals);
typedef void (*phy_type_misc_test_stop_fn_t)(phy_type_misc_ctx_t *ctx);
typedef uint32 (*phy_type_misc_rx_iq_est_fn_t)(phy_type_misc_ctx_t *ctx, uint8 samples,
	uint8 antsel, uint8 resolution, uint8 lpf_hpc, uint8 dig_lpf, uint8 gain_correct,
                       uint8 extra_gain_3dB, uint8 wait_for_crs, uint8 force_gain_type);
typedef void (*phy_type_misc_deaf_mode_fn_t)(phy_type_misc_ctx_t *ctx, bool user_flag);
typedef int (*phy_type_misc_test_freq_accuracy_fn_t)(phy_type_misc_ctx_t *ctx, int channel);
typedef void (*phy_type_misc_iovar_tx_tone_fn_t)(phy_type_misc_ctx_t *ctx, int32 int_val);
typedef void (*phy_type_misc_iovar_txlo_tone_fn_t)(phy_type_misc_ctx_t *ctx);
typedef int (*phy_type_misc_iovar_get_rx_iq_est_fn_t)(phy_type_misc_ctx_t *ctx,
	int32 *ret_int_ptr, int32 int_val, int err);
typedef int (*phy_type_misc_iovar_set_rx_iq_est_fn_t)(phy_type_misc_ctx_t *ctx,
	int32 int_val, int err);
#ifdef ATE_BUILD
typedef void (*phy_type_misc_gpaioconfig_fn_t) (phy_type_misc_ctx_t *ctx,
	wl_gpaio_option_t option, int core);
#endif
typedef int (*phy_type_misc_txswctrlmapset_fn_t) (phy_type_misc_ctx_t *ctx,
	int32 int_val);
typedef void (*phy_type_misc_txswctrlmapget_fn_t) (phy_type_misc_ctx_t *ctx,
	int32 *ret_int_ptr);
typedef bool (*phy_type_misc_get_rxgainerr_fn_t)(phy_type_misc_ctx_t *ctx, int16 *gainerr);
typedef void (*phy_type_vasip_svmp_write_t)(phy_type_misc_ctx_t *ctx, uint32 offset, uint16 val);
typedef uint16 (*phy_type_vasip_svmp_read_t)(phy_type_misc_ctx_t *ctx, const uint32 offset);
typedef void (*phy_type_vasip_svmp_write_blk_t)(phy_type_misc_ctx_t *ctx,
		uint32 offset, uint16 len, uint16 *val);
typedef void (*phy_type_vasip_svmp_read_blk_t)(phy_type_misc_ctx_t *ctx,
		uint32 offset, uint16 len, uint16 *val);

typedef struct {
	phy_type_misc_ctx_t *ctx;
	phy_type_misc_getlistandsize_fn_t phy_type_misc_getlistandsize;
	phy_type_vasip_get_ver_fn_t phy_type_vasip_get_ver;
	phy_type_vasip_proc_reset_fn_t phy_type_vasip_proc_reset;
	phy_type_vasip_clk_set_t phy_type_vasip_clk_set;
	phy_type_vasip_bin_write_t phy_type_vasip_bin_write;
#ifdef VASIP_SPECTRUM_ANALYSIS
	phy_type_vasip_spectrum_tbl_write_t phy_type_vasip_spectrum_tbl_write;
#endif
	phy_type_vasip_svmp_write_t phy_type_vasip_svmp_write;
	phy_type_vasip_svmp_read_t phy_type_vasip_svmp_read;
	phy_type_vasip_svmp_write_blk_t phy_type_vasip_svmp_write_blk;
	phy_type_vasip_svmp_read_blk_t phy_type_vasip_svmp_read_blk;
	phy_type_misc_test_init_fn_t phy_type_misc_test_init;
	phy_type_misc_test_stop_fn_t phy_type_misc_test_stop;
	phy_type_misc_rx_iq_est_fn_t phy_type_misc_rx_iq_est;
	phy_type_misc_deaf_mode_fn_t phy_type_misc_set_deaf;
	phy_type_misc_deaf_mode_fn_t phy_type_misc_clear_deaf;
	phy_type_misc_test_freq_accuracy_fn_t phy_type_misc_test_freq_accuracy;
	phy_type_misc_iovar_tx_tone_fn_t phy_type_misc_iovar_tx_tone;
	phy_type_misc_iovar_txlo_tone_fn_t phy_type_misc_iovar_txlo_tone;
	phy_type_misc_iovar_get_rx_iq_est_fn_t phy_type_misc_iovar_get_rx_iq_est;
	phy_type_misc_iovar_set_rx_iq_est_fn_t phy_type_misc_iovar_set_rx_iq_est;
#ifdef ATE_BUILD
	phy_type_misc_gpaioconfig_fn_t	gpaioconfig;
#endif
	phy_type_misc_txswctrlmapset_fn_t	txswctrlmapset;
	phy_type_misc_txswctrlmapget_fn_t	txswctrlmapget;
	phy_type_misc_get_rxgainerr_fn_t phy_type_misc_get_rxgainerr;
} phy_type_misc_fns_t;

/*
 * Register/unregister PHY type implementation to the MultiPhaseCal module.
 * It returns BCME_XXXX.
 */
int phy_misc_register_impl(phy_misc_info_t *mi, phy_type_misc_fns_t *fns);
void phy_misc_unregister_impl(phy_misc_info_t *cmn_info);

#endif /* _phy_type_misc_h_ */
