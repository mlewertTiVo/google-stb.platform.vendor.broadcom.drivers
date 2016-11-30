/*
 * NPHY Rx Gain Control and Carrier Sense module implementation
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
 * $Id: phy_n_rxgcrs.c 644994 2016-06-22 06:23:44Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_rxgcrs.h"
#include <phy_utils_reg.h>
#include <phy_n.h>
#include <phy_n_info.h>
#include <phy_n_rxgcrs.h>
#include <wlc_phyreg_n.h>
#include <phy_type_rxgcrs.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */

/* module private states */
struct phy_n_rxgcrs_info {
	phy_info_t *pi;
	phy_n_info_t *ni;
	phy_rxgcrs_info_t *cmn_info;
};

static void wlc_phy_adjust_ed_thres_nphy(phy_type_rxgcrs_ctx_t * ctx, int32 *assert_threshold,
	bool set_threshold);
#if defined(RXDESENS_EN)
static int wlc_nphy_get_rxdesens(phy_type_rxgcrs_ctx_t *ctx, int32 *ret_int_ptr);
static int phy_n_rxgcrs_set_rxdesens(phy_type_rxgcrs_ctx_t *ctx, int32 int_val);
#endif /* defined(RXDESENS_EN) */
#ifndef ATE_BUILD
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG)
static int phy_n_rxgcrs_forcecal_noise(phy_type_rxgcrs_ctx_t *ctx, void *a, bool set);
#endif /* BCMINTERNAL || WLTEST || ATE_BUILD */
#endif /* !ATE_BUILD */

/* register phy type specific implementation */
phy_n_rxgcrs_info_t *
BCMATTACHFN(phy_n_rxgcrs_register_impl)(phy_info_t *pi, phy_n_info_t *ni,
	phy_rxgcrs_info_t *cmn_info)
{
	phy_n_rxgcrs_info_t *n_info;
	phy_type_rxgcrs_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((n_info = phy_malloc(pi, sizeof(phy_n_rxgcrs_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	n_info->pi = pi;
	n_info->ni = ni;
	n_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = n_info;
	fns.adjust_ed_thres = wlc_phy_adjust_ed_thres_nphy;
#if defined(RXDESENS_EN)
	fns.adjust_get_rxdesens = wlc_nphy_get_rxdesens;
	fns.adjust_set_rxdesens = phy_n_rxgcrs_set_rxdesens;
#endif /* defined(RXDESENS_EN) */
#ifndef ATE_BUILD
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG)
	fns.forcecal_noise = phy_n_rxgcrs_forcecal_noise;
#endif /* BCMINTERNAL || WLTEST || ATE_BUILD */
#endif /* !ATE_BUILD */

	if (phy_rxgcrs_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_rxgcrs_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return n_info;

	/* error handling */
fail:
	if (n_info != NULL)
		phy_mfree(pi, n_info, sizeof(phy_n_rxgcrs_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_n_rxgcrs_unregister_impl)(phy_n_rxgcrs_info_t *n_info)
{
	phy_info_t *pi = n_info->pi;
	phy_rxgcrs_info_t *cmn_info = n_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_rxgcrs_unregister_impl(cmn_info);

	phy_mfree(pi, n_info, sizeof(phy_n_rxgcrs_info_t));
}

static void wlc_phy_adjust_ed_thres_nphy(phy_type_rxgcrs_ctx_t * ctx, int32 *assert_thresh_dbm,
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

		phy_utils_write_phyreg(pi, NPHY_ed_crs20LAssertThresh0, (uint16)assert_thres_val);
		phy_utils_write_phyreg(pi, NPHY_ed_crs20LAssertThresh1, (uint16)assert_thres_val);
		phy_utils_write_phyreg(pi, NPHY_ed_crs20UAssertThresh0, (uint16)assert_thres_val);
		phy_utils_write_phyreg(pi, NPHY_ed_crs20UAssertThresh1, (uint16)assert_thres_val);
		phy_utils_write_phyreg(pi, NPHY_ed_crs20LDeassertThresh0,
		                       (uint16)de_assert_thresh_val);
		phy_utils_write_phyreg(pi, NPHY_ed_crs20LDeassertThresh1,
		                       (uint16)de_assert_thresh_val);
		phy_utils_write_phyreg(pi, NPHY_ed_crs20UDeassertThresh0,
		                       (uint16)de_assert_thresh_val);
		phy_utils_write_phyreg(pi, NPHY_ed_crs20UDeassertThresh1,
		                       (uint16)de_assert_thresh_val);
	} else {
		assert_thres_val = phy_utils_read_phyreg(pi, NPHY_ed_crs20LAssertThresh0);
		*assert_thresh_dbm = ((((assert_thres_val - 832)*30103)) - 48000000)/640000;
	}
}

#if defined(RXDESENS_EN)
static int
wlc_nphy_get_rxdesens(phy_type_rxgcrs_ctx_t *ctx, int32 *ret_int_ptr)
{
	phy_n_rxgcrs_info_t *info = (phy_n_rxgcrs_info_t *)ctx;
	phy_info_t *pi = info->pi;
	phy_info_nphy_t *pi_nphy = pi->u.pi_nphy;
	uint16 regval, x1, x2, y;

	if (!pi->sh->up)
		return BCME_NOTUP;

	if (pi_nphy->ntd_crs_adjusted == FALSE)
		*ret_int_ptr = 0;
	else {
		regval =  phy_utils_read_phyreg(pi, NPHY_Core1InitGainCodeB2057);
		x1 = (((pi_nphy->ntd_initgain>>8) & 0xf) - ((regval>>8) & 0xf)) * 3;
		x2 = (((pi_nphy->ntd_initgain>>4) & 0xf) - ((regval>>4) & 0xf)) * 3;

		regval =  phy_utils_read_phyreg(pi, NPHY_crsminpoweru0);
		y = ((regval & 0xff) - pi_nphy->ntd_crsminpwr[0]) * 3;
		y = (y >> 3) + ((y & 0x4) >> 2);
		*ret_int_ptr = x1 + x2 + y;
	}

	return BCME_OK;
}

static int
phy_n_rxgcrs_set_rxdesens(phy_type_rxgcrs_ctx_t *ctx, int32 int_val)
{
	phy_n_rxgcrs_info_t *info = (phy_n_rxgcrs_info_t *)ctx;
	phy_info_t *pi = info->pi;
	if (pi->sh->interference_mode == INTERFERE_NONE)) {
		return wlc_nphy_set_rxdesens((wlc_phy_t *)pi, int32 int_val);
	} else {
		return BCME_UNSUPPORTED;
	}
}
#endif /* defined(RXDESENS_EN) */

#ifndef ATE_BUILD
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG)
/* JIRA: SWWLAN-32606, RB: 12975: function to do only Noise cal & read crsmin power of
 * core 0 & core 1
*/
static int
phy_n_rxgcrs_forcecal_noise(phy_type_rxgcrs_ctx_t *ctx, void *a, bool set)
{
	phy_n_rxgcrs_info_t *rxgcrsi = (phy_n_rxgcrs_info_t *) ctx;
	phy_info_t *pi = rxgcrsi->pi;
	int err = BCME_OK;
	uint8 wait_ctr = 0;
	int val = 1;
	uint16 crsmin[4];

	if (!set) {
		crsmin[0] = phy_utils_read_phyreg(pi, NPHY_crsminpowerl0);
		crsmin[1] = phy_utils_read_phyreg(pi, NPHY_crsminpoweru0);
		crsmin[2] = phy_utils_read_phyreg(pi, NPHY_crsminpowerl0_core1);
		crsmin[3] = phy_utils_read_phyreg(pi, NPHY_crsminpoweru0_core1);
		bcopy(crsmin, a, sizeof(uint16)*4);
	}
	else {
		wlc_phy_cal_perical_mphase_reset(pi);

		pi->cal_info->cal_phase_id = MPHASE_CAL_STATE_NOISECAL;
		pi->trigger_noisecal = TRUE;

		while (wait_ctr < 50) {
			val = ((pi->cal_info->cal_phase_id !=
			MPHASE_CAL_STATE_IDLE)? 1 : 0);

			if (val == 0) {
				err = BCME_OK;
				break;
			}
			else {
				wlc_phy_cal_perical_nphy_run(pi, PHY_PERICAL_PARTIAL);
				wait_ctr++;
			}
		}

		if (wait_ctr >= 50) {
			return BCME_ERROR;
		}
	}
	return err;
}
#endif /* BCMINTERNAL || WLTEST || ATE_BUILD */
#endif /* !ATE_BUILD */
