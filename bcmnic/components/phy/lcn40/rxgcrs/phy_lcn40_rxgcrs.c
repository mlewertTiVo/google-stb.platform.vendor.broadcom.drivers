/*
 * LCN40PHY Rx Gain Control and Carrier Sense module implementation
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
 * $Id: phy_lcn40_rxgcrs.c 606042 2015-12-14 06:21:23Z jqliu $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_rxgcrs.h"
#include <phy_utils_reg.h>
#include <phy_lcn40.h>
#include <phy_lcn40_rxgcrs.h>
#include <wlc_phyreg_n.h>
#include <wlc_phyreg_lcn.h>   /* lcn40 depends on lcn */
#include <wlc_phyreg_lcn40.h>
#include <phy_type_rxgcrs.h>
#include <wlc_phy_lcn40.h>

void wlc_lcn40phy_set_ed_thres(phy_info_t *pi);

/* ************************ */
/* Modules used by this module */
/* ************************ */

/* module private states */
struct phy_lcn40_rxgcrs_info {
	phy_info_t *pi;
	phy_lcn40_info_t *lcn40i;
	phy_rxgcrs_info_t *cmn_info;
};

static void wlc_phy_adjust_ed_thres_lcn40phy(phy_type_rxgcrs_ctx_t * ctx,
	int32 *assert_threshold, bool set_threshold);

/* register phy type specific implementation */
phy_lcn40_rxgcrs_info_t *
BCMATTACHFN(phy_lcn40_rxgcrs_register_impl)(phy_info_t *pi, phy_lcn40_info_t *lcn40i,
	phy_rxgcrs_info_t *cmn_info)
{
	phy_lcn40_rxgcrs_info_t *lcn40_info;
	phy_type_rxgcrs_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((lcn40_info = phy_malloc(pi, sizeof(phy_lcn40_rxgcrs_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	lcn40_info->pi = pi;
	lcn40_info->lcn40i = lcn40i;
	lcn40_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = lcn40_info;
	fns.adjust_ed_thres = wlc_phy_adjust_ed_thres_lcn40phy;

	if (phy_rxgcrs_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_rxgcrs_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return lcn40_info;

	/* error handling */
fail:
	if (lcn40_info != NULL)
		phy_mfree(pi, lcn40_info, sizeof(phy_lcn40_rxgcrs_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn40_rxgcrs_unregister_impl)(phy_lcn40_rxgcrs_info_t *lcn40_info)
{
	phy_info_t *pi = lcn40_info->pi;
	phy_rxgcrs_info_t *cmn_info = lcn40_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_rxgcrs_unregister_impl(cmn_info);

	phy_mfree(pi, lcn40_info, sizeof(phy_lcn40_rxgcrs_info_t));
}

static void wlc_phy_adjust_ed_thres_lcn40phy(phy_type_rxgcrs_ctx_t * ctx,
	int32 *assert_thresh_dbm, bool set_threshold)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	/* Set the EDCRS Assert and De-assert Threshold
	* The de-assert threshold is set to 6dB lower then the assert threshold
	* A 6dB offset is observed during measurements between value set in the register
	* and the actual value in dBm i.e. if the desired ED threshold is -65dBm,
	* the Edon register has to be set to -71
	*/
	phy_info_lcn40phy_t *pi_lcn40 = pi->u.pi_lcn40phy;
	int8 assert_thres_val, de_assert_thresh_val;

	if (set_threshold == TRUE) {
		assert_thres_val = *assert_thresh_dbm - 6;
		de_assert_thresh_val = assert_thres_val - 6;
		/* Set the EDCRS Assert Threshold */
		pi_lcn40->edonthreshold40 = assert_thres_val;
		pi_lcn40->edonthreshold20U = assert_thres_val;
		pi_lcn40->edonthreshold20L = assert_thres_val;

		/* Set the EDCRS De-assert Threshold */
		pi_lcn40->edoffthreshold40 = de_assert_thresh_val;
		pi_lcn40->edoffthreshold20UL = de_assert_thresh_val;

		wlc_lcn40phy_set_ed_thres(pi);
	}
	else {
		assert_thres_val = pi_lcn40->edonthreshold20L + 6;
		*assert_thresh_dbm = assert_thres_val;
	}
}

void wlc_lcn40phy_set_ed_thres(phy_info_t *pi)
{
	phy_info_lcn40phy_t *pi_lcn40 = pi->u.pi_lcn40phy;

	PHY_REG_MOD(pi, LCN40PHY, edthresh40, edonthreshold40,
		pi_lcn40->edonthreshold40);
	PHY_REG_MOD(pi, LCN40PHY, edthresh40, edoffthreshold40,
		pi_lcn40->edoffthreshold40);
	PHY_REG_MOD(pi, LCN40PHY, edthresh20ul, edonthreshold20U,
		pi_lcn40->edonthreshold20U);
	PHY_REG_MOD(pi, LCN40PHY, edthresh20ul, edonthreshold20L,
		pi_lcn40->edonthreshold20L);
	PHY_REG_MOD(pi, LCN40PHY, crsedthresh, edoffthreshold,
		pi_lcn40->edoffthreshold20UL);
}
