/*
 * LCN40PHY TxPowerCtrl module implementation
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
 * $Id: phy_lcn40_tpc.c 642720 2016-06-09 18:56:12Z vyass $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_tpc.h>
#include "phy_tpc_shared.h"
#include "phy_type_tpc.h"
#include <phy_lcn40.h>
#include <phy_lcn40_tpc.h>
#include <phy_tpc_api.h>
#include <phy_rstr.h>
#include <phy_utils_reg.h>
#include <phy_utils_var.h>
#include <phy_utils_channel.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_lcn40.h>
#include <wlc_phyreg_lcn40.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_lcn40_tpc_info {
	phy_info_t *pi;
	phy_lcn40_info_t *lcn40i;
	phy_tpc_info_t *ti;
};

/* local functions */
static int phy_lcn40_tpc_init(phy_type_tpc_ctx_t *ctx);
static void phy_lcn40_tpc_recalc_tgt(phy_type_tpc_ctx_t *ctx);
static void wlc_phy_txpower_recalc_target_lcn40_big(phy_type_tpc_ctx_t *ctx, ppr_t *tx_pwr_target,
    ppr_t *srom_max_txpwr, ppr_t *reg_txpwr_limit, ppr_t *txpwr_targets);
static void wlc_phy_txpower_sromlimit_get_lcn40phy(phy_type_tpc_ctx_t *ctx, chanspec_t chanspec,
    ppr_t *max_pwr, uint8 core);
static bool phy_lcn40_tpc_hw_ctrl_get(phy_type_tpc_ctx_t *ctx);
static void phy_lcn40_tpc_set_flags(phy_type_tpc_ctx_t *ctx, phy_tx_power_t *power);
static void phy_lcn40_tpc_set_max(phy_type_tpc_ctx_t *ctx, phy_tx_power_t *power);
#if (defined(BCMINTERNAL) || defined(WLTEST))
static int phy_lcn40_tpc_set_pavars(phy_type_tpc_ctx_t *ctx, void* a, void* p);
static int phy_lcn40_tpc_get_pavars(phy_type_tpc_ctx_t *ctx, void* a, void* p);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

/* Register/unregister LCN40PHY specific implementation to common layer. */
phy_lcn40_tpc_info_t *
BCMATTACHFN(phy_lcn40_tpc_register_impl)(phy_info_t *pi, phy_lcn40_info_t *lcn40i,
	phy_tpc_info_t *ti)
{
	phy_lcn40_tpc_info_t *info;
	phy_type_tpc_fns_t fns;
	int i;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_lcn40_tpc_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_lcn40_tpc_info_t));
	info->pi = pi;
	info->lcn40i = lcn40i;
	info->ti = ti;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.init = phy_lcn40_tpc_init;
	fns.recalc = phy_lcn40_tpc_recalc_tgt;
	fns.recalc_target = wlc_phy_txpower_recalc_target_lcn40_big;
	fns.get_sromlimit = wlc_phy_txpower_sromlimit_get_lcn40phy;
	fns.get_hwctrl = phy_lcn40_tpc_hw_ctrl_get;
	fns.setflags = phy_lcn40_tpc_set_flags;
	fns.setmax = phy_lcn40_tpc_set_max;
#if (defined(BCMINTERNAL) || defined(WLTEST))
	fns.set_pavars = phy_lcn40_tpc_set_pavars;
	fns.get_pavars = phy_lcn40_tpc_get_pavars;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
	fns.ctx = info;

	for (i = 0; i < 14; i++) {
		pi->phy_cga_2g[i] = (int8)PHY_GETINTVAR_ARRAY_SLICE(pi, rstr_2g_cga, i);
	}
#ifdef BAND5G
	for (i = 0; i < 24; i++) {
		pi->phy_cga_5g[i] = (int8)PHY_GETINTVAR_ARRAY_SLICE(pi, rstr_5g_cga, i);
	}
#endif /* BAND5G */

	phy_tpc_register_impl(ti, &fns);

	return info;

fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_lcn40_tpc_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn40_tpc_unregister_impl)(phy_lcn40_tpc_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_tpc_info_t *ti = info->ti;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_tpc_unregister_impl(ti);

	phy_mfree(pi, info, sizeof(phy_lcn40_tpc_info_t));
}

/* init module and h/w */
static int
WLBANDINITFN(phy_lcn40_tpc_init)(phy_type_tpc_ctx_t *ctx)
{
	phy_lcn40_tpc_info_t *info = (phy_lcn40_tpc_info_t *)ctx;
	phy_lcn40_info_t *lcn40i = info->lcn40i;
	phy_info_t *pi = info->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (lcn40i->lcnphycommon.txpwrindex5g_nvram ||
	    lcn40i->lcnphycommon.txpwrindex_nvram) {
		uint8 txpwrindex;
#ifdef BAND5G
		if (CHSPEC_IS5G(pi->radio_chanspec))
			txpwrindex = lcn40i->lcnphycommon.txpwrindex5g_nvram;
		else
#endif /* BAND5G */
			txpwrindex = lcn40i->lcnphycommon.txpwrindex_nvram;

		if (txpwrindex) {
#if defined(PHYCAL_CACHING)
			ch_calcache_t *ctxTmp = wlc_phy_get_chanctx(pi, pi->last_radio_chanspec);
			wlc_iovar_txpwrindex_set_lcncommon(pi, txpwrindex, ctxTmp);
#else
			wlc_iovar_txpwrindex_set_lcncommon(pi, txpwrindex);
#endif /* defined(PHYCAL_CACHING) */
		}
	}

	return BCME_OK;
}

static int8
wlc_phy_channel_gain_adjust(phy_info_t *pi)
{
	int8 pwr_correction = 0;
	uint8 channel;
	/* In case of 80P80, extract primary channel number */
	if (CHSPEC_IS8080(pi->radio_chanspec)) {
		channel = wf_chspec_primary80_channel(pi->radio_chanspec);
	} else {
		channel = CHSPEC_CHANNEL(pi->radio_chanspec);
	}

	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		pwr_correction = pi->phy_cga_2g[channel-1];
	}
#ifdef BAND5G
	else {
		uint freq = phy_utils_channel2freq(channel);
		uint8 i;
		uint16 chan_info_phy_cga_5g[24] = {
			5180, 5200, 5220, 5240, 5260, 5280, 5300, 5320,
			5500, 5520, 5540, 5560, 5580, 5600, 5620, 5640,
			5660, 5680, 5700, 5745, 5765, 5785, 5805, 5825,
		};

		for (i = 0; i < ARRAYSIZE(chan_info_phy_cga_5g); i++)
			if (freq <= chan_info_phy_cga_5g[i]) {
				pwr_correction = pi->phy_cga_5g[i];
				break;
			}
	}
#endif /* BAND5G */
	return pwr_correction;
}

/* recalc target txpwr and apply to h/w */
static void
phy_lcn40_tpc_recalc_tgt(phy_type_tpc_ctx_t *ctx)
{
	phy_lcn40_tpc_info_t *info = (phy_lcn40_tpc_info_t *)ctx;
	phy_info_t *pi = info->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	wlc_phy_txpower_recalc_target_lcn40phy(pi);
}

/* TODO: The code could be optimized by moving the common code to phy/cmn */
/* [PHY_RE_ARCH] There are two functions: Bigger function wlc_phy_txpower_recalc_target
 * and smaller function phy_tpc_recalc_tgt which in turn call their phy specific functions
 * which are named in a haphazard manner. This needs to be cleaned up.
 */
static void
wlc_phy_txpower_recalc_target_lcn40_big(phy_type_tpc_ctx_t *ctx, ppr_t *tx_pwr_target,
    ppr_t *srom_max_txpwr, ppr_t *reg_txpwr_limit, ppr_t *txpwr_targets)
{
	phy_lcn40_tpc_info_t *info = (phy_lcn40_tpc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	int8 tx_pwr_max = 0;
	int8 tx_pwr_min = 255;
	uint8 mintxpwr = 0;
	uint8 core;
	chanspec_t chspec = pi->radio_chanspec;
	int8 cga = wlc_phy_channel_gain_adjust(pi);
	/* Combine user target, regulatory limit, SROM/HW/board limit and power
	 * percentage to get a tx power target for each rate.
	 */
	FOREACH_CORE(pi, core) {
		/* The user target is the starting point for determining the transmit
		 * power.  If pi->txoverride is true, then use the user target as the
		 * tx power target.
		 */
		ppr_set_cmn_val(tx_pwr_target, info->ti->data->tx_user_target);

#if defined(BCMINTERNAL) || defined(WLTEST) || defined(WL_EXPORT_TXPOWER)
		/* Only allow tx power override for internal or test builds. */
		if (!info->ti->data->txpwroverride)
#endif
		{
			/* Get board/hw limit */
			wlc_phy_txpower_sromlimit((wlc_phy_t *)pi, chspec,
			    &mintxpwr, srom_max_txpwr, core);

			/* Adjust board limits based on environmental conditions */
			wlc_lcn40phy_apply_cond_chg(pi->u.pi_lcn40phy,
					srom_max_txpwr);

			/* Common Code Start */
			/* Choose minimum of provided regulatory and board/hw limits */
			ppr_compare_min(srom_max_txpwr, reg_txpwr_limit);

			/* Subtract 4 (1.0db) for 4313(IPA) as we are doing PA trimming
			 * otherwise subtract 6 (1.5 db)
			 * to ensure we don't go over
			 * the limit given a noisy power detector.  The result
			 * is now a target, not a limit.
			 */
			ppr_minus_cmn_val(srom_max_txpwr, pi->tx_pwr_backoff);

			/* Choose least of user and now combined regulatory/hw targets */
			ppr_compare_min(tx_pwr_target, srom_max_txpwr);

			/* Board specific fix reduction */

			/* Apply power output percentage */
			if (pi->tpci->data->txpwr_percent < 100) {
				ppr_multiply_percentage(tx_pwr_target,
					pi->tpci->data->txpwr_percent);
			}
			/* Common Code End */

			/* Enforce min power and save result as power target.
			 * LCNPHY references off the minimum so this is not appropriate for it.
			 */
			ppr_apply_min(tx_pwr_target, mintxpwr);

			/* Channel Gain Adjustment */
			ppr_minus_cmn_val(tx_pwr_target, cga);
		}

		tx_pwr_max = ppr_get_max(tx_pwr_target);

		if (tx_pwr_max < (pi->min_txpower * WLC_TXPWR_DB_FACTOR)) {
			tx_pwr_max = pi->min_txpower * WLC_TXPWR_DB_FACTOR;
		}
		tx_pwr_min = ppr_get_min(tx_pwr_target, WL_RATE_DISABLED);

		/* Common Code Start */
		/* Now calculate the tx_power_offset and update the hardware... */
		pi->tx_power_max_per_core[core] = tx_pwr_max;
		pi->tx_power_min_per_core[core] = tx_pwr_min;

#ifdef WLTXPWR_CACHE
		if (wlc_phy_txpwr_cache_is_cached(pi->txpwr_cache, pi->radio_chanspec) == TRUE) {
			wlc_phy_set_cached_pwr_min(pi->txpwr_cache, pi->radio_chanspec, core,
				tx_pwr_min);
			wlc_phy_set_cached_pwr_max(pi->txpwr_cache, pi->radio_chanspec, core,
				tx_pwr_max);
		}
#endif
		pi->openlp_tx_power_min = tx_pwr_min;
		info->ti->data->txpwrnegative = 0;

		PHY_NONE(("wl%d: %s: min %d max %d\n", pi->sh->unit, __FUNCTION__,
		    tx_pwr_min, tx_pwr_max));

		/* determinate the txpower offset by either of the following 2 methods:
		 * txpower_offset = txpower_max - txpower_target OR
		 * txpower_offset = txpower_target - txpower_min
		 * return curpower for last core loop since we are now checking
		 * MIN_cores(MAX_rates(power)) for rate disabling
		 * Only the last core loop info is valid
		 */
		if (core == PHYCORENUM((pi)->pubpi->phy_corenum) - 1) {
			info->ti->data->curpower_display_core =
				PHYCORENUM((pi)->pubpi->phy_corenum) - 1;
			if (txpwr_targets != NULL)
				ppr_copy_struct(tx_pwr_target, txpwr_targets);
		}
		/* Common Code End */

		if (!pi->hwpwrctrl) {
			ppr_cmn_val_minus(tx_pwr_target, pi->tx_power_max_per_core[core]);
		} else {
			ppr_minus_cmn_val(tx_pwr_target, pi->tx_power_min_per_core[core]);
		}

		ppr_compare_max(pi->tx_power_offset, tx_pwr_target);

		phy_tpc_cck_corr(pi);
	}	/* CORE loop */

	/* Common Code Start */
#ifdef WLTXPWR_CACHE
	if (wlc_phy_txpwr_cache_is_cached(pi->txpwr_cache, pi->radio_chanspec) == TRUE) {
		wlc_phy_set_cached_pwr(pi->sh->osh, pi->txpwr_cache, pi->radio_chanspec,
			TXPWR_CACHE_POWER_OFFSETS, pi->tx_power_offset);
	}
#endif
	/*
	 * PHY_ERROR(("#####The final power offset limit########\n"));
	 * ppr_mcs_printf(pi->tx_power_offset);
	 */
	ppr_delete(pi->sh->osh, reg_txpwr_limit);
	ppr_delete(pi->sh->osh, tx_pwr_target);
	ppr_delete(pi->sh->osh, srom_max_txpwr);
	/* Common Code End */
}

#ifdef BAND5G
static void
wlc_phy_txpwr_apply_srom_5g_subband(phy_info_t *pi, int8 max_pwr_ref, ppr_t *tx_srom_max_pwr,
	uint32 ofdm_20_offsets, uint32 mcs_20_offsets, uint32 mcs_40_offsets)
{
	ppr_ofdm_rateset_t ppr_ofdm;
	ppr_ht_mcs_rateset_t ppr_mcs;
	uint32 offset_mcs = 0;
	uint32 last_offset_mcs = 0;

	if (CHSPEC_IS20(pi->radio_chanspec)) {
		/* 5G-hi - OFDM_20 */
		wlc_phy_txpwr_srom_convert_ofdm(ofdm_20_offsets, max_pwr_ref, &ppr_ofdm);
		ppr_set_ofdm(tx_srom_max_pwr, WL_TX_BW_20, WL_TX_MODE_NONE,
			WL_TX_CHAINS_1, &ppr_ofdm);

		/* 5G-hi - MCS_20 */
		offset_mcs = mcs_20_offsets;
		wlc_phy_txpwr_srom_convert_mcs(offset_mcs, max_pwr_ref, &ppr_mcs);
		ppr_set_ht_mcs(tx_srom_max_pwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_NONE,
			WL_TX_CHAINS_1, &ppr_mcs);
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {

		/* 5G-hi - MCS_40 */
		last_offset_mcs = offset_mcs;
		offset_mcs = mcs_40_offsets;
		if (!offset_mcs)
			offset_mcs = last_offset_mcs;

		wlc_phy_txpwr_srom_convert_mcs(offset_mcs, max_pwr_ref, &ppr_mcs);
		ppr_set_ht_mcs(tx_srom_max_pwr, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_NONE,
			WL_TX_CHAINS_1, &ppr_mcs);
		/* Infer 20in40 MCS from this limit */
		ppr_set_ht_mcs(tx_srom_max_pwr, WL_TX_BW_20IN40, WL_TX_NSS_1, WL_TX_MODE_NONE,
			WL_TX_CHAINS_1, &ppr_mcs);
		/* Infer 20in40 OFDM from this limit */
		ppr_set_ofdm(tx_srom_max_pwr, WL_TX_BW_20IN40, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
			(ppr_ofdm_rateset_t*)&ppr_mcs);
		/* Infer 40MHz OFDM from this limit */
		ppr_set_ofdm(tx_srom_max_pwr, WL_TX_BW_40, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
			(ppr_ofdm_rateset_t*)&ppr_mcs);
	}
}
#endif /* BAND5G */

static void
wlc_phy_txpwr_apply_sromlcn40(phy_info_t *pi, uint8 band, ppr_t *tx_srom_max_pwr)
{
	srom_lcn40_ppr_t *sr_lcn40 = &pi->ppr->u.sr_lcn40;
	int8 max_pwr_ref = 0;

	ppr_dsss_rateset_t ppr_dsss;
	ppr_ofdm_rateset_t ppr_ofdm;
	ppr_ht_mcs_rateset_t ppr_mcs;

#ifdef BAND5G
	uint32 offset_ofdm20, offset_mcs20, offset_mcs40;
#endif

	switch (band)
	{
		case WL_CHAN_FREQ_RANGE_2G:
		{
			max_pwr_ref = pi->tx_srom_max_2g;

			/* 2G - CCK_20 */
			wlc_phy_txpwr_srom_convert_cck(sr_lcn40->cck202gpo, max_pwr_ref, &ppr_dsss);
			if (CHSPEC_IS20(pi->radio_chanspec))
				ppr_set_dsss(tx_srom_max_pwr, WL_TX_BW_20, WL_TX_CHAINS_1,
					&ppr_dsss);
			/* Infer 20in40 DSSS from this limit */
			else if (CHSPEC_IS40(pi->radio_chanspec))
				ppr_set_dsss(tx_srom_max_pwr, WL_TX_BW_20IN40, WL_TX_CHAINS_1,
					&ppr_dsss);

			if (CHSPEC_IS20(pi->radio_chanspec)) {
				/* 2G - OFDM_20 */
				wlc_phy_txpwr_srom_convert_ofdm(sr_lcn40->ofdmbw202gpo, max_pwr_ref,
					&ppr_ofdm);
				ppr_set_ofdm(tx_srom_max_pwr, WL_TX_BW_20, WL_TX_MODE_NONE,
					WL_TX_CHAINS_1, &ppr_ofdm);
			}

			if (CHSPEC_IS20(pi->radio_chanspec)) {
				/* 2G - MCS_20 */
				wlc_phy_txpwr_srom_convert_mcs(sr_lcn40->mcsbw202gpo, max_pwr_ref,
					&ppr_mcs);
				ppr_set_ht_mcs(tx_srom_max_pwr, WL_TX_BW_20, WL_TX_NSS_1,
					WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ppr_mcs);
			}


			if (CHSPEC_IS40(pi->radio_chanspec)) {
				/* 2G - MCS_40 */
				wlc_phy_txpwr_srom_convert_mcs(sr_lcn40->mcsbw402gpo, max_pwr_ref,
					&ppr_mcs);
				ppr_set_ht_mcs(tx_srom_max_pwr, WL_TX_BW_40, WL_TX_NSS_1,
					WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ppr_mcs);
				/* Infer MCS_20in40 from MCS_40 */
				ppr_set_ht_mcs(tx_srom_max_pwr, WL_TX_BW_20IN40, WL_TX_NSS_1,
					WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ppr_mcs);
				/* Infer OFDM_20in40 from MCS_40 */
				ppr_set_ofdm(tx_srom_max_pwr, WL_TX_BW_20IN40, WL_TX_MODE_NONE,
					WL_TX_CHAINS_1, (ppr_ofdm_rateset_t*)&ppr_mcs);
				/* Infer OFDM_40 from MCS_40 */
				ppr_set_ofdm(tx_srom_max_pwr, WL_TX_BW_40, WL_TX_MODE_NONE,
					WL_TX_CHAINS_1, (ppr_ofdm_rateset_t*)&ppr_mcs);
			}

			break;
		}
#ifdef BAND5G
		case WL_CHAN_FREQ_RANGE_5GM:
		{
			max_pwr_ref = pi->tx_srom_max_5g_mid;
			offset_ofdm20 = sr_lcn40->ofdm5gpo;
			offset_mcs20 = (sr_lcn40->mcs5gpo1 << 16) | sr_lcn40->mcs5gpo0;
			offset_mcs40 = (sr_lcn40->mcs5gpo3 << 16) | sr_lcn40->mcs5gpo2;

			wlc_phy_txpwr_apply_srom_5g_subband(pi, max_pwr_ref, tx_srom_max_pwr,
				offset_ofdm20, offset_mcs20, offset_mcs40);
			break;
		}
		case WL_CHAN_FREQ_RANGE_5GL:
		{
			max_pwr_ref = pi->tx_srom_max_5g_low;
			offset_ofdm20 = sr_lcn40->ofdm5glpo;
			offset_mcs20 = (sr_lcn40->mcs5glpo1 << 16) | sr_lcn40->mcs5glpo0;
			offset_mcs40 = (sr_lcn40->mcs5glpo3 << 16) | sr_lcn40->mcs5glpo2;

			wlc_phy_txpwr_apply_srom_5g_subband(pi, max_pwr_ref, tx_srom_max_pwr,
				offset_ofdm20, offset_mcs20, offset_mcs40);
			break;
		}
		case WL_CHAN_FREQ_RANGE_5GH:
		{
			max_pwr_ref = pi->tx_srom_max_5g_hi;
			offset_ofdm20 = sr_lcn40->ofdm5ghpo;
			offset_mcs20 = (sr_lcn40->mcs5ghpo1 << 16) | sr_lcn40->mcs5ghpo0;
			offset_mcs40 = (sr_lcn40->mcs5ghpo3 << 16) | sr_lcn40->mcs5ghpo2;

			wlc_phy_txpwr_apply_srom_5g_subband(pi, max_pwr_ref, tx_srom_max_pwr,
				offset_ofdm20, offset_mcs20, offset_mcs40);
			break;
		}
#endif /* #ifdef BAND5G */
	}
}

static void
wlc_phy_txpower_sromlimit_get_lcn40phy(phy_type_tpc_ctx_t *ctx, chanspec_t chanspec,
    ppr_t *max_pwr, uint8 core)
{
	phy_lcn40_tpc_info_t *info = (phy_lcn40_tpc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	srom_pwrdet_t *pwrdet  = pi->pwrdet;
	uint8 band;
	uint8 channel = CHSPEC_CHANNEL(chanspec);
	band = wlc_phy_get_band_from_channel(pi, channel);
	wlc_phy_txpwr_apply_sromlcn40(pi, band, max_pwr);
	ppr_apply_max(max_pwr, pwrdet->max_pwr[core][band]);
}

static bool
phy_lcn40_tpc_hw_ctrl_get(phy_type_tpc_ctx_t *ctx)
{
	phy_lcn40_tpc_info_t *info = (phy_lcn40_tpc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	return pi->hwpwrctrl;
}

static void
phy_lcn40_tpc_set_flags(phy_type_tpc_ctx_t *ctx, phy_tx_power_t *power)
{
	phy_lcn40_tpc_info_t *info = (phy_lcn40_tpc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	power->rf_cores = 1;
	power->flags |= (WL_TX_POWER_F_SISO);
	if (info->ti->data->radiopwr_override == RADIOPWR_OVERRIDE_DEF)
		power->flags |= WL_TX_POWER_F_ENABLED;
	if (pi->hwpwrctrl)
		power->flags |= WL_TX_POWER_F_HW;
}

static void
phy_lcn40_tpc_set_max(phy_type_tpc_ctx_t *ctx, phy_tx_power_t *power)
{
#ifdef WL_SARLIMIT
	uint8 core;
#endif
	phy_lcn40_tpc_info_t *info = (phy_lcn40_tpc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	/* Store the maximum target power among all rates */
	if (pi->hwpwrctrl && pi->sh->up) {
		/* If hw (ucode) based, read the hw based estimate in realtime */
		phy_utils_phyreg_enter(pi);
		/* Store the maximum target power among all rates */
		power->tx_power_max[0] = pi->tx_power_max_per_core[0];
		power->tx_power_max[1] = pi->tx_power_max_per_core[0];
		if (wlc_phy_tpc_iovar_isenabled_lcn40phy(pi))
			power->flags |= (WL_TX_POWER_F_HW | WL_TX_POWER_F_ENABLED);
		else
			power->flags &= ~(WL_TX_POWER_F_HW | WL_TX_POWER_F_ENABLED);
		phy_utils_phyreg_exit(pi);
	}
#ifdef WL_SARLIMIT
	FOREACH_CORE(pi, core) {
		power->SARLIMIT[core] = WLC_TXPWR_MAX;
	}
#endif
}

static void
wlc_lcn40phy_get_tssi_floor(phy_info_t *pi, uint16 *floor)
{
	phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);

	switch (wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec)) {
		case WL_CHAN_FREQ_RANGE_2G:
			*floor = pi_lcn->tssi_floor_2g;
			break;
#ifdef BAND5G
	case WL_CHAN_FREQ_RANGE_5GL:
			/* 5 GHz low */
			*floor = pi_lcn->tssi_floor_5glo;
			break;

		case WL_CHAN_FREQ_RANGE_5GM:
			/* 5 GHz middle */
			*floor = pi_lcn->tssi_floor_5gmid;
			break;

		case WL_CHAN_FREQ_RANGE_5GH:
			/* 5 GHz high */
			*floor = pi_lcn->tssi_floor_5ghi;
			break;
#endif /* BAND5G */
		default:
			ASSERT(FALSE);
			break;
	}
}

void
wlc_lcn40phy_set_txpwr_clamp(phy_info_t *pi)
{
	phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
	uint16 tssi_floor = 0, idle_tssi_shift, adj_tssi_min;
	uint16 idleTssi_2C, idleTssi_OB, target_pwr_reg, intended_target;
	phy_info_lcn40phy_t *pi_lcn40 = pi->u.pi_lcn40phy;
	int32 a1 = 0, b0 = 0, b1 = 0;
	int32 target_pwr_cck_max, target_pwr_ofdm_max, pwr, max_ovr_pwr;
	int32 fudge = 0*8; /* 1dB */
	phytbl_info_t tab;
	uint32 rate_table[WL_RATESET_SZ_DSSS + WL_RATESET_SZ_OFDM + WL_RATESET_SZ_HT_MCS];
	uint8 ii;
	uint16 perPktIdleTssi;

	if (pi_lcn->txpwr_clamp_dis || pi_lcn->txpwr_tssifloor_clamp_dis) {
		pi_lcn->target_pwr_ofdm_max = 0x7fffffff;
		pi_lcn->target_pwr_cck_max = 0x7fffffff;
		if (pi_lcn40->btc_clamp) {
			target_pwr_cck_max = BTC_POWER_CLAMP;
			target_pwr_ofdm_max = BTC_POWER_CLAMP;
		} else {
			return;
		}
	} else {

		wlc_lcn40phy_get_tssi_floor(pi, &tssi_floor);
		wlc_phy_get_paparams_for_band(pi, &a1, &b0, &b1);

		if (LCN40REV_GE(pi->pubpi->phy_rev, 4)) {
			perPktIdleTssi = PHY_REG_READ(pi, LCN40PHY, TxPwrCtrlIdleTssi2,
				perPktIdleTssiUpdate_en);
			if (perPktIdleTssi)
				idleTssi_2C = PHY_REG_READ(pi, LCN40PHY, TxPwrCtrlStatusNew6,
					avgidletssi);
			else
				idleTssi_2C = PHY_REG_READ(pi, LCN40PHY, TxPwrCtrlIdleTssi,
					idleTssi0);
		}
		else
			idleTssi_2C = PHY_REG_READ(pi, LCN40PHY, TxPwrCtrlIdleTssi, idleTssi0);

		if (idleTssi_2C >= 256)
			idleTssi_OB = idleTssi_2C - 256;
		else
			idleTssi_OB = idleTssi_2C + 256;

		idleTssi_OB = idleTssi_OB >> 2; /* Converting to 7 bits */
		idle_tssi_shift = (127 - idleTssi_OB) + 4;
		adj_tssi_min = MAX(tssi_floor, idle_tssi_shift);
		pwr = wlc_lcnphy_tssi2dbm(adj_tssi_min, a1, b0, b1);
		target_pwr_ofdm_max = (pwr - fudge) >> 1;
		target_pwr_cck_max = (MIN(pwr, (pwr + pi_lcn->cckPwrOffset)) - fudge) >> 1;
		PHY_TMP(("idleTssi_OB= %d, idle_tssi_shift= %d, adj_tssi_min= %d, "
				"pwr = %d, target_pwr_cck_max = %d, target_pwr_ofdm_max = %d\n",
				idleTssi_OB, idle_tssi_shift, adj_tssi_min, pwr,
				target_pwr_cck_max, target_pwr_ofdm_max));
		pi_lcn->target_pwr_ofdm_max = target_pwr_ofdm_max;
		pi_lcn->target_pwr_cck_max = target_pwr_cck_max;

		if (pi_lcn40->btc_clamp) {
			target_pwr_cck_max = MIN(target_pwr_cck_max, BTC_POWER_CLAMP);
			target_pwr_ofdm_max = MIN(target_pwr_ofdm_max, BTC_POWER_CLAMP);
		}
	}

	if (pi->tpci->data->txpwroverride) {
		max_ovr_pwr = MIN(target_pwr_ofdm_max, target_pwr_cck_max);
		{
			uint8 core;
			FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
				pi->tx_power_min_per_core[core] =
					MIN(pi->tx_power_min_per_core[core], max_ovr_pwr);
			}
		}
		return;
	}

	for (ii = 0; ii < ARRAYSIZE(rate_table); ii ++)
		rate_table[ii] = pi_lcn->rate_table[ii];

	/* Adjust Rate Offset Table to ensure intended tx power for every OFDM/CCK */
	/* rate is less than target_power_ofdm_max/target_power_cck_max */
	target_pwr_reg = wlc_lcn40phy_get_target_tx_pwr(pi);
	for (ii = 0; ii < WL_RATESET_SZ_DSSS; ii ++) {
		intended_target = target_pwr_reg - rate_table[ii];
		if (intended_target > target_pwr_cck_max)
			rate_table[ii] = rate_table[ii] + (intended_target - target_pwr_cck_max);
		PHY_TMP(("Rate: %d, maxtar = %d, target = %d, origoff: %d, clampoff: %d\n",
			ii, target_pwr_cck_max, intended_target,
			pi_lcn->rate_table[ii], rate_table[ii]));
	}
	for (ii = WL_RATESET_SZ_DSSS;
		ii < WL_RATESET_SZ_DSSS + WL_RATESET_SZ_OFDM + WL_RATESET_SZ_HT_MCS; ii ++) {
		intended_target = target_pwr_reg - rate_table[ii];
		if (intended_target > target_pwr_ofdm_max)
			rate_table[ii] = rate_table[ii] + (intended_target - target_pwr_ofdm_max);
		PHY_TMP(("Rate: %d, maxtar = %d, target = %d, origoff: %d, clampoff: %d\n",
			ii, target_pwr_ofdm_max, intended_target,
			pi_lcn->rate_table[ii], rate_table[ii]));
	}

	tab.tbl_id = LCN40PHY_TBL_ID_TXPWRCTL;
	tab.tbl_width = 32;	/* 32 bit wide	*/
	tab.tbl_len = ARRAYSIZE(rate_table); /* # values   */
	tab.tbl_ptr = rate_table; /* ptr to buf */
	tab.tbl_offset = LCN40PHY_TX_PWR_CTRL_RATE_OFFSET;
	wlc_lcn40phy_write_table(pi, &tab);
}

#if (defined(BCMINTERNAL) || defined(WLTEST))
static int
phy_lcn40_tpc_set_pavars(phy_type_tpc_ctx_t *ctx, void *a, void *p)
{
	phy_lcn40_tpc_info_t *tpci = (phy_lcn40_tpc_info_t *)ctx;
	uint16 inpa[WL_PHY_PAVARS_LEN];
#if defined(LCNCONF)
	phy_info_t *pi = tpci->pi;
	uint j = 3; /* PA parameters start from offset 3 */
#endif /* defined(LCNCONF) */

	bcopy(p, inpa, sizeof(inpa));

	if ((inpa[0] != PHY_TYPE_LCN) && (inpa[0] != PHY_TYPE_LCN40)) {
		return BCME_BADARG;
	}

	if (inpa[2] != 0) {
		return BCME_BADARG;
	}

#if defined(LCNCONF)
	switch (inpa[1]) {
	case WL_CHAN_FREQ_RANGE_2G:
		pi->txpa_2g[0] = inpa[j++]; /* b0 */
		pi->txpa_2g[1] = inpa[j++]; /* b1 */
		pi->txpa_2g[2] = inpa[j++]; /* a1 */
		break;
#ifdef BAND5G
	case WL_CHAN_FREQ_RANGE_5GL:
		pi->txpa_5g_low[0] = inpa[j++]; /* b0 */
		pi->txpa_5g_low[1] = inpa[j++]; /* b1 */
		pi->txpa_5g_low[2] = inpa[j++]; /* a1 */
		break;

	case WL_CHAN_FREQ_RANGE_5GM:
		pi->txpa_5g_mid[0] = inpa[j++]; /* b0 */
		pi->txpa_5g_mid[1] = inpa[j++]; /* b1 */
		pi->txpa_5g_mid[2] = inpa[j++]; /* a1 */
		break;

	case WL_CHAN_FREQ_RANGE_5GH:
		pi->txpa_5g_hi[0] = inpa[j++];	/* b0 */
		pi->txpa_5g_hi[1] = inpa[j++];	/* b1 */
		pi->txpa_5g_hi[2] = inpa[j++];	/* a1 */
		break;
#endif /* BAND5G */
	default:
		PHY_ERROR(("bandrange %d is out of scope\n", inpa[0]));
		return BCME_OUTOFRANGECHAN;
	}
	return BCME_OK;
#else
	return BCME_UNSUPPORTED;
#endif /* Older PHYs */
}

static int
phy_lcn40_tpc_get_pavars(phy_type_tpc_ctx_t *ctx, void *a, void *p)
{
	phy_lcn40_tpc_info_t *tpci = (phy_lcn40_tpc_info_t *)ctx;
	uint16 *outpa = a;
	uint16 inpa[WL_PHY_PAVARS_LEN];
#if defined(LCNCONF)
	phy_info_t *pi = tpci->pi;
	uint j = 3; /* PA parameters start from offset 3 */
#endif /* defined(LCNCONF) */

	bcopy(p, inpa, sizeof(inpa));

	outpa[0] = inpa[0]; /* Phy type */
	outpa[1] = inpa[1]; /* Band range */
	outpa[2] = inpa[2]; /* Chain */

	if (((inpa[0] != PHY_TYPE_LCN) && (inpa[0] != PHY_TYPE_LCN40))) {
		outpa[0] = PHY_TYPE_NULL;
		return BCME_OK;
	}

	if (inpa[2] != 0) {
		return BCME_BADARG;
	}

#if defined(LCNCONF)
	switch (inpa[1]) {
	case WL_CHAN_FREQ_RANGE_2G:
		outpa[j++] = pi->txpa_2g[0];		/* b0 */
		outpa[j++] = pi->txpa_2g[1];		/* b1 */
		outpa[j++] = pi->txpa_2g[2];		/* a1 */
		break;
#ifdef BAND5G
	case WL_CHAN_FREQ_RANGE_5GL:
		outpa[j++] = pi->txpa_5g_low[0];	/* b0 */
		outpa[j++] = pi->txpa_5g_low[1];	/* b1 */
		outpa[j++] = pi->txpa_5g_low[2];	/* a1 */
		break;

	case WL_CHAN_FREQ_RANGE_5GM:
		outpa[j++] = pi->txpa_5g_mid[0];	/* b0 */
		outpa[j++] = pi->txpa_5g_mid[1];	/* b1 */
		outpa[j++] = pi->txpa_5g_mid[2];	/* a1 */
		break;

	case WL_CHAN_FREQ_RANGE_5GH:
		outpa[j++] = pi->txpa_5g_hi[0]; /* b0 */
		outpa[j++] = pi->txpa_5g_hi[1]; /* b1 */
		outpa[j++] = pi->txpa_5g_hi[2]; /* a1 */
		break;
#endif /* BAND5G */
	default:
		PHY_ERROR(("bandrange %d is out of scope\n", inpa[0]));
		return BCME_OUTOFRANGECHAN;
	}
	return BCME_OK;
#else
	return BCME_UNSUPPORTED;
#endif /* older PHYs */
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
