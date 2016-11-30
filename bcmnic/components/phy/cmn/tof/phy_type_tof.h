/*
 * TOF module internal interface (to PHY specific implementation).
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
 * $Id: phy_type_tof.h 633216 2016-04-21 20:17:37Z vyass $
 */

#ifndef _phy_type_tof_h_
#define _phy_type_tof_h_

#include <phy_tof.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_tof_ctx_t;
typedef int (*phy_type_tof_init_fn_t)(phy_type_tof_ctx_t *ctx);
typedef int (*phy_type_tof_fn_t)(phy_type_tof_ctx_t *ctx, bool enter, bool tx, bool hw_adj,
	bool seq_en, int core);
typedef void (*phy_type_tof_cmd_fn_t)(phy_type_tof_ctx_t *ctx, bool seq);
typedef int (*phy_type_tof_info_fn_t)(phy_type_tof_ctx_t *ctx, int* p_frame_type, int* p_frame_bw,
	int* p_cfo, int8* p_rssi);
typedef int (*phy_type_tof_kvalue_fn_t)(phy_type_tof_ctx_t *ctx, chanspec_t chanspec,
	uint32 rspecidx, uint32 *kip, uint32 *ktp, uint8 flag);
typedef int (*phy_type_tof_seq_params_fn_t)(phy_type_tof_ctx_t *ctx);
typedef void (*phy_type_tof_seq_upd_dly_fn_t)(phy_type_tof_ctx_t *ctx, bool tx, uint8 core,
	bool mac_suspend);
typedef int (*phy_type_chan_freq_response_fn_t)(phy_type_tof_ctx_t *ctx, int len, int nbits,
	cint32* H, uint32* Hraw);
typedef int (*phy_type_chan_mag_sqr_impulse_response_fn_t)(phy_type_tof_ctx_t *ctx,
	int frame_type, int len, int offset, int nbits, int32* h, int* pgd, uint32* hraw,
	uint16 tof_shm_ptr);
typedef int (*phy_type_seq_ts_fn_t)(phy_type_tof_ctx_t *ctx, int n, cint32* p_buffer, int tx,
	int cfo, int adj, void* pparams, int32* p_ts, int32* p_seq_len, uint32* p_raw);
typedef int (*phy_type_tof_dbg_fn_t)(phy_type_tof_ctx_t *ctx, int arg);
typedef struct {
	phy_type_tof_init_fn_t init_tof;
	phy_type_tof_fn_t tof;
	phy_type_tof_cmd_fn_t cmd;
	phy_type_tof_info_fn_t info;
	phy_type_tof_kvalue_fn_t kvalue;
	phy_type_tof_seq_params_fn_t seq_params;
	phy_type_tof_seq_upd_dly_fn_t seq_upd_dly;
	phy_type_chan_freq_response_fn_t chan_freq_response;
	phy_type_chan_mag_sqr_impulse_response_fn_t chan_mag_sqr_impulse_response;
	phy_type_seq_ts_fn_t seq_ts;
	phy_type_tof_dbg_fn_t dbg;
	phy_type_tof_ctx_t *ctx;
} phy_type_tof_fns_t;

/*
 * Register/unregister PHY type implementation to the common of the TOF module.
 * It returns BCME_XXXX.
 */
int phy_tof_register_impl(phy_tof_info_t *ti, phy_type_tof_fns_t *fns);
void phy_tof_unregister_impl(phy_tof_info_t *ti);

#endif /* _phy_type_tof_h_ */
