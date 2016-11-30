/*
 * HTPHY Rx Gain Control and Carrier Sense module implementation
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
 * $Id: phy_ht_rxgcrs.c 606042 2015-12-14 06:21:23Z jqliu $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_rxgcrs.h"
#include <phy_utils_reg.h>
#include <phy_ht.h>
#include <phy_ht_rxgcrs.h>
#include <wlc_phyreg_ht.h>
#include <phy_type_rxgcrs.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */

/* module private states */
struct phy_ht_rxgcrs_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_rxgcrs_info_t *cmn_info;
};

static void wlc_phy_adjust_ed_thres_htphy(phy_type_rxgcrs_ctx_t * ctx, int32 *assert_threshold,
	bool set_threshold);

/* register phy type specific implementation */
phy_ht_rxgcrs_info_t *
BCMATTACHFN(phy_ht_rxgcrs_register_impl)(phy_info_t *pi, phy_ht_info_t *hti,
	phy_rxgcrs_info_t *cmn_info)
{
	phy_ht_rxgcrs_info_t *ht_info;
	phy_type_rxgcrs_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ht_info = phy_malloc(pi, sizeof(phy_ht_rxgcrs_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	ht_info->pi = pi;
	ht_info->hti = hti;
	ht_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = ht_info;
	fns.adjust_ed_thres = wlc_phy_adjust_ed_thres_htphy;

	if (phy_rxgcrs_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_rxgcrs_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return ht_info;

	/* error handling */
fail:
	if (ht_info != NULL)
		phy_mfree(pi, ht_info, sizeof(phy_ht_rxgcrs_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_rxgcrs_unregister_impl)(phy_ht_rxgcrs_info_t *ht_info)
{
	phy_info_t *pi = ht_info->pi;
	phy_rxgcrs_info_t *cmn_info = ht_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_rxgcrs_unregister_impl(cmn_info);

	phy_mfree(pi, ht_info, sizeof(phy_ht_rxgcrs_info_t));
}

static void wlc_phy_adjust_ed_thres_htphy(phy_type_rxgcrs_ctx_t * ctx, int32 *assert_thresh_dbm,
	bool set_threshold)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	/* Set the EDCRS Assert and De-assert Threshold
	The de-assert threshold is set to 6dB lower then the assert threshold
	Accurate Formula:64*log2(round((10.^((THRESHOLD_dBm +65-30)./10).*50).*(2^9./0.4).^2))
	Simplified Accurate Formula: 64*(THRESHOLD_dBm + 75)/(10*log10(2)) + 832;
	Implemented Approximate Formula: 640000*(THRESHOLD_dBm + 75)/30103 + 832;
	*/
	int32 assert_thres_val, de_assert_thresh_val;

	if (set_threshold == TRUE) {
		assert_thres_val = (640000*(*assert_thresh_dbm + 75) + 25045696)/30103;
		de_assert_thresh_val = (640000*(*assert_thresh_dbm + 69) + 25045696)/30103;

		phy_utils_write_phyreg(pi, HTPHY_ed_crs20LAssertThresh0, (uint16)assert_thres_val);
		phy_utils_write_phyreg(pi, HTPHY_ed_crs20LAssertThresh1, (uint16)assert_thres_val);
		phy_utils_write_phyreg(pi, HTPHY_ed_crs20UAssertThresh0, (uint16)assert_thres_val);
		phy_utils_write_phyreg(pi, HTPHY_ed_crs20UAssertThresh1, (uint16)assert_thres_val);
		phy_utils_write_phyreg(pi, HTPHY_ed_crs20LDeassertThresh0,
		                       (uint16)de_assert_thresh_val);
		phy_utils_write_phyreg(pi, HTPHY_ed_crs20LDeassertThresh1,
		                       (uint16)de_assert_thresh_val);
		phy_utils_write_phyreg(pi, HTPHY_ed_crs20UDeassertThresh0,
		                       (uint16)de_assert_thresh_val);
		phy_utils_write_phyreg(pi, HTPHY_ed_crs20UDeassertThresh1,
		                       (uint16)de_assert_thresh_val);
	} else {
		assert_thres_val = phy_utils_read_phyreg(pi, HTPHY_ed_crs20LAssertThresh0);
		*assert_thresh_dbm = ((((assert_thres_val - 832)*30103)) - 48000000)/640000;
	}
}
