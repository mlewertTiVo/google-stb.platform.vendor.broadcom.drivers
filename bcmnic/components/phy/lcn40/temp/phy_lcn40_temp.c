/*
 * LCN40PHY TEMPerature sense module implementation
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
 * $Id: phy_lcn40_temp.c 630449 2016-04-09 00:27:18Z vyass $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_temp.h>
#include "phy_type_temp.h"
#include <phy_lcn40.h>
#include <phy_lcn40_temp.h>

#include <wlc_phy_lcn40.h>

/* module private states */
struct phy_lcn40_temp_info {
	phy_info_t *pi;
	phy_lcn40_info_t *lcn40i;
	phy_temp_info_t *tempi;
};

/* local functions */
#ifdef WLTEST
static void wlc_lcn40phy_rxgaincal_tempadj(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj);
#endif /* WLTEST */
/* Register/unregister LCN40PHY specific implementation to common layer */
phy_lcn40_temp_info_t *
BCMATTACHFN(phy_lcn40_temp_register_impl)(phy_info_t *pi, phy_lcn40_info_t *lcn40i,
	phy_temp_info_t *tempi)
{
	phy_lcn40_temp_info_t *info;
	phy_type_temp_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_lcn40_temp_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_lcn40_temp_info_t));
	info->pi = pi;
	info->lcn40i = lcn40i;
	info->tempi = tempi;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
#ifdef WLTEST
	fns.upd_gain = wlc_lcn40phy_rxgaincal_tempadj;
#endif /* WLTEST */
	fns.ctx = info;

	phy_temp_register_impl(tempi, &fns);

	return info;
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_lcn40_temp_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn40_temp_unregister_impl)(phy_lcn40_temp_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_temp_info_t *tempi = info->tempi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_temp_unregister_impl(tempi);

	phy_mfree(pi, info, sizeof(phy_lcn40_temp_info_t));
}

#ifdef WLTEST
/* switch anacore on/off */
static void
wlc_lcn40phy_rxgaincal_tempadj(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj)
{
	phy_lcn40_temp_info_t *info = (phy_lcn40_temp_info_t *)ctx;
	phy_info_t *pi = info->pi;
	int16 curr_temp, temp_coeff;
	phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);

	curr_temp = wlc_lcn40phy_tempsense(pi, 1);

	switch (wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec)) {
		case WL_CHAN_FREQ_RANGE_2G:
			temp_coeff = pi_lcn->rxgain_tempadj_2g;
			break;
#ifdef BAND5G
		case WL_CHAN_FREQ_RANGE_5GL:
			/* 5 GHz low */
			temp_coeff = pi_lcn->rxgain_tempadj_5gl;
			break;

		case WL_CHAN_FREQ_RANGE_5GM:
			/* 5 GHz middle */
			temp_coeff = pi_lcn->rxgain_tempadj_5gm;
			break;

		case WL_CHAN_FREQ_RANGE_5GH:
			/* 5 GHz high */
			temp_coeff = pi_lcn->rxgain_tempadj_5gh;
			break;
#endif /* BAND5G */
		default:
			temp_coeff = 0;
			break;
	}

	/* temp_coeff in nvram is multiplied by 2^10 */
	/* gain_err_temp_adj is on 0.25dB steps */
	*gain_err_temp_adj = (curr_temp * temp_coeff) >> 8;
}
#endif /* WLTEST */
