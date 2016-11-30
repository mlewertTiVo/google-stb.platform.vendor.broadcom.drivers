/*
 * PAPD CAL module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_papdcal.h 639713 2016-05-24 18:02:57Z vyass $
 */

#ifndef _phy_type_papdcal_h_
#define _phy_type_papdcal_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_papdcal.h>

#ifdef WLPHY_IPA_ONLY
	#define PHY_EPAPD(pi)   0
#elif defined(EPAPD_SUPPORT)
	#define PHY_EPAPD(pi)   1
#elif defined(EPAPD_SUPPORT_NVRAMCTL)
	#define PHY_EPAPD(pi)							\
			(((pi)->papdcali->data->epacal2g && CHSPEC_IS2G((pi)->radio_chanspec) && \
			(((pi)->papdcali->data->epacal2g_mask >> \
			(CHSPEC_CHANNEL(pi->radio_chanspec) - 1)) & 1)) || \
			((pi)->papdcali->data->epacal5g && CHSPEC_IS5G((pi)->radio_chanspec)))
#elif defined(DONGLEBUILD)
	#define PHY_EPAPD(pi)   0
#else
	#define PHY_EPAPD(pi)							\
			(((pi)->papdcali->data->epacal2g && CHSPEC_IS2G((pi)->radio_chanspec) && \
			(((pi)->papdcali->data->epacal2g_mask >> \
			(CHSPEC_CHANNEL(pi->radio_chanspec) - 1)) & 1)) || \
			((pi)->papdcali->data->epacal5g && CHSPEC_IS5G((pi)->radio_chanspec)))
#endif /* WLPHY_IPA_ONLY */

typedef struct phy_papdcal_priv_info phy_papdcal_priv_info_t;

typedef struct phy_papdcal_data {
	uint8	epacal2g;
	uint16	epacal2g_mask;
	uint8	epacal5g;
	/* #ifdef WFD_PHY_LL */
	uint8	wfd_ll_enable;
	uint8	wfd_ll_enable_pending;
	uint8	wfd_ll_chan_active;
	uint8	wfd_ll_chan_active_force;
	/* #endif */
} phy_papdcal_data_t;

struct phy_papdcal_info {
    phy_papdcal_priv_info_t *priv;
    phy_papdcal_data_t *data;
};

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_papdcal_ctx_t;

typedef int (*phy_type_papdcal_init_fn_t)(phy_type_papdcal_ctx_t *ctx);
typedef void (*phy_type_papdcal_epa_dpd_set_fn_t)(phy_type_papdcal_ctx_t *ctx, uint8 enab_epa_dpd,
	bool in_2g_band);
typedef int (*phy_type_papdcal_dump_fn_t)(phy_type_papdcal_ctx_t *ctx, struct bcmstrbuf *b);
typedef bool (*phy_type_papdcal_isperratedpden_fn_t) (phy_type_papdcal_ctx_t *ctx);
typedef void (*phy_type_papdcal_perratedpdset_fn_t) (phy_type_papdcal_ctx_t *ctx, bool enable);
typedef int (*phy_type_papdcal_set_uint_fn_t) (phy_type_papdcal_ctx_t *ctx, uint8 var);
typedef int (*phy_type_papdcal_get_var_fn_t) (phy_type_papdcal_ctx_t *ctx, int32 *var);
typedef int (*phy_type_papdcal_set_int_fn_t) (phy_type_papdcal_ctx_t *ctx, int8 var);
typedef void (*phy_type_papdcal_void_fn_t) (phy_type_papdcal_ctx_t *ctx);
typedef struct {
	phy_type_papdcal_epa_dpd_set_fn_t epa_dpd_set;
	phy_type_papdcal_isperratedpden_fn_t	isperratedpden;
	phy_type_papdcal_perratedpdset_fn_t	perratedpdset;
	phy_type_papdcal_get_var_fn_t get_idx0;
	phy_type_papdcal_get_var_fn_t get_idx1;
	phy_type_papdcal_set_int_fn_t set_idx;
	phy_type_papdcal_set_uint_fn_t set_skip;
	phy_type_papdcal_get_var_fn_t get_skip;
	phy_type_papdcal_void_fn_t wd_wfd_ll;
	phy_type_papdcal_set_uint_fn_t set_wfd_ll_enable;
	phy_type_papdcal_get_var_fn_t get_wfd_ll_enable;
	phy_type_papdcal_ctx_t *ctx;
} phy_type_papdcal_fns_t;

/*
 * Register/unregister PHY type implementation to the papdcal module.
 * It returns BCME_XXXX.
 */
int phy_papdcal_register_impl(phy_papdcal_info_t *cmn_info, phy_type_papdcal_fns_t *fns);
void phy_papdcal_unregister_impl(phy_papdcal_info_t *cmn_info);

#endif /* _phy_type_papdcal_h_ */
