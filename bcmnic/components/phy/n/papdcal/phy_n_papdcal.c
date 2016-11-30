/*
 * N PHY PAPD CAL module implementation
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
 * $Id: phy_n_papdcal.c 639713 2016-05-24 18:02:57Z vyass $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_type_papdcal.h>
#include <phy_n.h>
#include <phy_n_papdcal.h>

/* module private states */
struct phy_n_papdcal_info {
	phy_info_t			*pi;
	phy_n_info_t		*aci;
	phy_papdcal_info_t	*cmn_info;
/* add other variable size variables here at the end */
};

#ifdef WFD_PHY_LL
static void phy_ac_wd_wfd_ll(phy_type_papdcal_ctx_t *ctx);
static int phy_n_papdcal_set_wfd_ll_enable(phy_type_papdcal_ctx_t *ctx, uint8 int_val);
static int phy_n_papdcal_get_wfd_ll_enable(phy_type_papdcal_ctx_t *ctx, int32 *ret_int_ptr);
#endif /* WFD_PHY_LL */

/* register phy type specific implementation */
phy_n_papdcal_info_t *
BCMATTACHFN(phy_n_papdcal_register_impl)(phy_info_t *pi, phy_n_info_t *aci,
	phy_papdcal_info_t *cmn_info)
{
	phy_n_papdcal_info_t *ac_info;
	phy_type_papdcal_fns_t fns;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ac_info = phy_malloc(pi, sizeof(phy_n_papdcal_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize params */
	bzero(ac_info, sizeof(phy_n_papdcal_info_t));
	ac_info->pi = pi;
	ac_info->aci = aci;
	ac_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
#if defined(WFD_PHY_LL)
	fns.wd_wfd_ll = phy_n_wd_wfd_ll;
	fns.set_wfd_ll_enable = phy_n_papdcal_set_wfd_ll_enable;
	fns.get_wfd_ll_enable = phy_n_papdcal_get_wfd_ll_enable;
#endif /* WFD_PHY_LL */
	fns.ctx = ac_info;

	if (phy_papdcal_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_papdcal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return ac_info;

	/* error handling */
fail:
	if (ac_info != NULL)
		phy_mfree(pi, ac_info, sizeof(phy_n_papdcal_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_n_papdcal_unregister_impl)(phy_n_papdcal_info_t *ac_info)
{
	phy_papdcal_info_t *cmn_info = ac_info->cmn_info;
	phy_info_t *pi = ac_info->pi;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_papdcal_unregister_impl(cmn_info);

	phy_mfree(pi, ac_info, sizeof(phy_n_papdcal_info_t));
}

#ifdef WFD_PHY_LL
static void
phy_n_wd_wfd_ll(phy_type_papdcal_ctx_t *ctx)
{
	phy_n_papdcal_info_t *papdcali = (phy_n_papdcal_info_t *)ctx;
	phy_papdcal_data_t *data = papdcali->cmn_info->data;

	/* Be sure there is no cal in progress to enable/disable optimization */
	if (!PHY_PERICAL_MPHASE_PENDING(papdcali->pi)) {
		if (data->wfd_ll_enable != data->wfd_ll_enable_pending) {
			data->wfd_ll_enable = data->wfd_ll_enable_pending;
			if (!data->wfd_ll_enable) {
				/* Force a watchdog CAL when disabling WFD optimization
				 * As PADP CAL has not been executed since a long time
				 * a PADP CAL is executed at the next watchdog timeout
				 */
				papdcali->pi->cal_info->last_cal_time = 0;
			}
		}
	}
}
#endif /* WFD_PHY_LL */

#if defined(WFD_PHY_LL)
static int
phy_n_papdcal_set_wfd_ll_enable(phy_type_papdcal_ctx_t *ctx, uint8 int_val)
{
	phy_n_papdcal_info_t *papdcali = (phy_n_papdcal_info_t *)ctx;
	phy_papdcal_data_t *data = papdcali->cmn_info->data;

	/* Force the channel to be active */
	data->wfd_ll_chan_active_force = (int_val == 2) ? TRUE : FALSE;
	data->wfd_ll_enable_pending = int_val;
	if (!PHY_PERICAL_MPHASE_PENDING(papdcali->pi)) {
		/* Apply it since there is no CAL in progress */
		data->wfd_ll_enable = int_val;
		if (!int_val) {
			/* Force a watchdog CAL when disabling WFD optimization
			 * As PADP CAL has not been executed since a long time
			 * a PADP CAL is executed at the next watchdog timeout
			 */
			 papdcali->pi->cal_info->last_cal_time = 0;
		}
	}
	return BCME_OK;
}

static int
phy_n_papdcal_get_wfd_ll_enable(phy_type_papdcal_ctx_t *ctx, int32 *ret_int_ptr)
{
	phy_n_papdcal_info_t *papdcali = (phy_n_papdcal_info_t *)ctx;
	*ret_int_ptr = papdcali->cmn_info->data->wfd_ll_enable;
	return BCME_OK;
}
#endif /* WFD_PHY_LL */
