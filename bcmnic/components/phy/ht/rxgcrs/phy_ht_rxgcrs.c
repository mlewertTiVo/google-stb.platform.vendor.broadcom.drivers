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
 * $Id: phy_ht_rxgcrs.c 657351 2016-08-31 23:00:22Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_rxgcrs.h"
#include <phy_utils_reg.h>
#include <phy_ht.h>
#include <phy_ht_info.h>
#include <phy_ht_rxgcrs.h>
#include <wlc_phyreg_ht.h>
#include <phy_type_rxgcrs.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
/* TODO: all these are going away... > */
#endif

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
static void
phy_ht_rxgcrs_stay_in_carriersearch(phy_type_rxgcrs_ctx_t * ctx, bool enable);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void
phy_ht_rxgcrs_phydump_chanest(phy_type_rxgcrs_ctx_t * ctx, struct bcmstrbuf *b);
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */
#if defined(DBG_BCN_LOSS)
static int
phy_ht_rxgcrs_dump_phycal_rx_min(phy_type_rxgcrs_ctx_t * ctx, struct bcmstrbuf *b);
#endif /* DBG_BCN_LOSS */

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
	fns.stay_in_carriersearch = phy_ht_rxgcrs_stay_in_carriersearch;
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	fns.phydump_chanest = phy_ht_rxgcrs_phydump_chanest;
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */
#if defined(DBG_BCN_LOSS)
	fns.phydump_phycal_rxmin = phy_ht_rxgcrs_dump_phycal_rx_min;
#endif /* DBG_BCN_LOSS */

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

static void
phy_ht_rxgcrs_stay_in_carriersearch(phy_type_rxgcrs_ctx_t * ctx, bool enable)
{
	phy_ht_rxgcrs_info_t *rxgcrs_info = (phy_ht_rxgcrs_info_t *)ctx;
	phy_info_t *pi = rxgcrs_info->pi;
	phy_info_htphy_t *pi_ht = (phy_info_htphy_t *)pi->u.pi_htphy;
	uint16 clip_off[] = {0xffff, 0xffff, 0xffff, 0xffff};

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* MAC should be suspended before calling this function */
	ASSERT(!(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC));

	if (enable) {
		if (pi_ht->deaf_count == 0) {
			pi_ht->classifier_state = wlc_phy_classifier_htphy(pi, 0, 0);
			wlc_phy_classifier_htphy(pi, HTPHY_ClassifierCtrl_classifierSel_MASK, 4);
			wlc_phy_clip_det_htphy(pi, 0, pi_ht->clip_state);
			wlc_phy_clip_det_htphy(pi, 1, clip_off);
		}

		pi_ht->deaf_count++;

		wlc_phy_resetcca_htphy(pi);

	} else {
		ASSERT(pi_ht->deaf_count > 0);

		pi_ht->deaf_count--;

		if (pi_ht->deaf_count == 0) {
			wlc_phy_classifier_htphy(pi, HTPHY_ClassifierCtrl_classifierSel_MASK,
			pi_ht->classifier_state);
			wlc_phy_clip_det_htphy(pi, 1, pi_ht->clip_state);
		}
	}
}
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void
phy_ht_rxgcrs_phydump_chanest(phy_type_rxgcrs_ctx_t * ctx, struct bcmstrbuf *b)
{
	phy_ht_rxgcrs_info_t *rxgcrs_info = (phy_ht_rxgcrs_info_t *)ctx;
	phy_info_t *pi = rxgcrs_info->pi;
	uint16 num_rx, num_sts, num_tones, start_tone;
	uint16 k, r, t, fftk;
	uint32 ch;
	uint16 ch_re_ma, ch_im_ma;
	uint8  ch_re_si, ch_im_si;
	int16  ch_re, ch_im;
	int8   ch_exp;
	uint8  dump_tones;

	num_rx = (uint8)PHYCORENUM(pi->pubpi->phy_corenum);
	num_sts = 4;

	if (CHSPEC_IS40(pi->radio_chanspec)) {
		num_tones = 128;
#ifdef CHSPEC_IS80
	} else if (CHSPEC_IS80(pi->radio_chanspec)) {
		num_tones = 256;
#endif /* CHSPEC_IS80 */
	} else {
		num_tones = 64;
	}

	bcm_bprintf(b, "num_tones=%d\n", num_tones);

	/* Dump only 16 sub-carriers at a time */
	dump_tones = 16;
	/* Reset the dump counter */
	if (pi->phy_chanest_dump_ctr > (num_tones/dump_tones - 1))
		pi->phy_chanest_dump_ctr = 0;

	start_tone = pi->phy_chanest_dump_ctr * dump_tones;
	pi->phy_chanest_dump_ctr++;

	for (r = 0; r < num_rx; r++) {
		bcm_bprintf(b, "rx=%d\n", r);
		for (t = 0; t < num_sts; t++) {
			bcm_bprintf(b, "sts=%d\n", t);
			for (k = start_tone; k < (start_tone + dump_tones); k++) {
				wlc_phy_table_read_htphy(pi, HTPHY_TBL_ID_CHANEST(r), 1,
								 t*128 + k, 32, &ch);

				/* Q11 FLP (12,12,6) */
				ch_re_ma  = ((ch >> 18) & 0x7ff);
				ch_re_si  = ((ch >> 29) & 0x001);
				ch_im_ma  = ((ch >>  6) & 0x7ff);
				ch_im_si  = ((ch >> 17) & 0x001);
				ch_exp	  = ((int8)((ch << 2) & 0xfc)) >> 2;
				ch_re = (ch_re_si > 0) ? -ch_re_ma : ch_re_ma;
				ch_im = (ch_im_si > 0) ? -ch_im_ma : ch_im_ma;

				fftk = ((k < num_tones/2) ? (k + num_tones/2) : (k - num_tones/2));

				bcm_bprintf(b, "chan(%d,%d,%d)=(%d+i*%d)*2^%d;\n",
					    r+1, t+1, fftk+1, ch_re, ch_im, ch_exp);
			}
		}
	}
}
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */
#if defined(DBG_BCN_LOSS)
static int
phy_ht_rxgcrs_dump_phycal_rx_min(phy_type_rxgcrs_ctx_t * ctx, struct bcmstrbuf *b)
{
	phy_ht_rxgcrs_info_t *rxgcrs_info = (phy_ht_rxgcrs_info_t *)ctx;
	phy_info_t *pi = rxgcrs_info->pi;

	wlc_phy_cal_dump_htphy_rx_min(pi, b);

	return BCME_OK;
}
#endif /* DBG_BCN_LOSS */
