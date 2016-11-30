/*
 * TxPowerCtrl module implementation.
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
 * $Id: phy_tpc.c 651513 2016-07-27 08:59:58Z mvermeid $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_init.h>
#include <phy_rstr.h>
#include "phy_type_tpc.h"
#include <phy_tpc_api.h>
#include <phy_tpc.h>
#include <phy_utils_channel.h>
#include <phy_utils_var.h>

/* ******* Local Functions ************ */

/* ********************************** */

/* module private states */
struct phy_tpc_priv_info {
	phy_info_t *pi;
	phy_type_tpc_fns_t *fns;
	uint8 ucode_tssi_limit_en;
};

/* module private states memory layout */
typedef struct {
	phy_tpc_info_t info;
	phy_type_tpc_fns_t fns;
	phy_tpc_priv_info_t priv;
	phy_tpc_data_t data;
/* add other variable size variables here at the end */
} phy_tpc_mem_t;

/* local function declaration */
static int phy_tpc_init(phy_init_ctx_t *ctx);

/* attach/detach */
phy_tpc_info_t *
BCMATTACHFN(phy_tpc_attach)(phy_info_t *pi)
{
	phy_tpc_info_t *info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((info = phy_malloc(pi, sizeof(phy_tpc_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	info->priv = &((phy_tpc_mem_t *)info)->priv;
	info->priv->pi = pi;
	info->priv->fns = &((phy_tpc_mem_t *)info)->fns;
	info->data = &((phy_tpc_mem_t *)info)->data;

#if defined(POWPERCHANNL) && !defined(POWPERCHANNL_DISABLED)
		pi->_powerperchan = TRUE;
#endif
	/* Initialize variables */
	info->priv->ucode_tssi_limit_en = (uint8)PHY_GETINTVAR_DEFAULT(pi, rstr_tssilimucod, 1);
	info->data->cfg.bphy_scale = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_bphyscale, 0);
	info->data->cfg.initbaseidx2govrval = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi,
		rstr_initbaseidx2govrval, 255);
	info->data->cfg.initbaseidx5govrval = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi,
		rstr_initbaseidx5govrval, 255);
	info->data->cfg.srom_tworangetssi2g = (bool)PHY_GETINTVAR_DEFAULT_SLICE(pi,
		rstr_tworangetssi2g, FALSE);
	info->data->cfg.srom_tworangetssi5g = (bool)PHY_GETINTVAR_DEFAULT_SLICE(pi,
		rstr_tworangetssi5g, FALSE);
	info->data->cfg.srom_2g_pdrange_id = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi,
		rstr_pdgain2g, 0);
	info->data->cfg.srom_5g_pdrange_id = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi,
		rstr_pdgain5g, 0);
#ifdef FCC_PWR_LIMIT_2G
	info->data->cfg.fccpwrch12 = (int8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_fccpwrch12, 0);
	info->data->cfg.fccpwrch13 = (int8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_fccpwrch13, 0);
	info->data->cfg.fccpwroverride =
		(int8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_fccpwroverride, 0);
#endif /* FCC_PWR_LIMIT_2G */
	/* set default power output percentage to 100 percent */
	info->data->txpwr_percent = 100;
	/* initialize our txpwr limit to a large value until we know what band/channel
	 * we settle on in wlc_up() set the txpwr user override to the max
	 */
	info->data->tx_user_target = WLC_TXPWR_MAX;
	/* default radio power */
	info->data->radiopwr_override = RADIOPWR_OVERRIDE_DEF;
#ifdef WL_SARLIMIT
	memset(info->data->sarlimit, WLC_TXPWR_MAX, PHY_MAX_CORES);
#endif /* WL_SARLIMIT */
#ifdef PREASSOC_PWRCTRL
	info->data->channel_short_window = TRUE;
#endif

	/* Register callbacks */

	/* register init fn */
	if (phy_init_add_init_fn(pi->initi, phy_tpc_init, info, PHY_INIT_TPC) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn failed\n", __FUNCTION__));
		goto fail;
	}

	return info;

	/* error */
fail:
	phy_tpc_detach(info);
	return NULL;
}

void
BCMATTACHFN(phy_tpc_detach)(phy_tpc_info_t *info)
{
	phy_info_t *pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (info == NULL) {
		PHY_INFORM(("%s: null tpc module\n", __FUNCTION__));
		return;
	}

	pi = info->priv->pi;

	phy_mfree(pi, info, sizeof(phy_tpc_mem_t));
}


/* TPC init */
static int
WLBANDINITFN(phy_tpc_init)(phy_init_ctx_t *ctx)
{
	phy_tpc_info_t *tpci = (phy_tpc_info_t *)ctx;
	phy_type_tpc_fns_t *fns = tpci->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->init != NULL) {
		return (fns->init)(fns->ctx);
	}
	return BCME_OK;
}

bool
phy_tpc_get_tworangetssi2g(phy_tpc_info_t *tpci)
{
	return tpci->data->cfg.srom_tworangetssi2g;
}

bool
phy_tpc_get_tworangetssi5g(phy_tpc_info_t *tpci)
{
	return tpci->data->cfg.srom_tworangetssi5g;
}

uint8
phy_tpc_get_2g_pdrange_id(phy_tpc_info_t *tpci)
{
	return tpci->data->cfg.srom_2g_pdrange_id;
}

uint8
phy_tpc_get_5g_pdrange_id(phy_tpc_info_t *tpci)
{
	return tpci->data->cfg.srom_5g_pdrange_id;
}

/* Translates the regulatory power limit array into an array of length TXP_NUM_RATES,
 * which can match the board limit array obtained using the SROM. Moreover, since the NPHY chips
 * currently do not distinguish between Legacy OFDM and MCS0-7, the SISO and CDD regulatory power
 * limits of these rates need to be combined carefully.
 * This internal/static function needs to be called whenever the chanspec or regulatory TX power
 * limits change.
 */
static void
wlc_phy_txpower_reg_limit_calc(phy_info_t *pi, ppr_t *txpwr, chanspec_t chanspec,
	ppr_t *txpwr_limit)
{
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;
	ppr_ht_mcs_rateset_t mcs_limits;

	ppr_copy_struct(txpwr, txpwr_limit);

#if defined(WLPROPRIETARY_11N_RATES)
	ppr_get_ht_mcs(txpwr_limit, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_NONE,
		WL_TX_CHAINS_1, &mcs_limits);
	if (mcs_limits.pwr[WL_RATE_GROUP_VHT_INDEX_MCS_87] == WL_RATE_DISABLED) {
		mcs_limits.pwr[WL_RATE_GROUP_VHT_INDEX_MCS_87] =
			mcs_limits.pwr[WL_RATE_GROUP_VHT_INDEX_MCS_7];
		mcs_limits.pwr[WL_RATE_GROUP_VHT_INDEX_MCS_88] =
			mcs_limits.pwr[WL_RATE_GROUP_VHT_INDEX_MCS_7];
		ppr_set_ht_mcs(txpwr_limit, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_NONE,
			WL_TX_CHAINS_1, &mcs_limits);
	}
#endif

	/* Obtain the regulatory limits for Legacy OFDM and HT-OFDM 11n rates in NPHY chips */
	if (fns->reglimit_calc != NULL) {
		(fns->reglimit_calc)(fns->ctx, txpwr, txpwr_limit, &mcs_limits);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}

#ifdef PREASSOC_PWRCTRL
void
phy_preassoc_pwrctrl_upd(phy_info_t *pi, chanspec_t chspec)
{
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;
	if ((pi->radio_chanspec != chspec) && fns->shortwindow_upd != NULL &&
	    fns->store_setting != NULL && pi->sh->up) {
		(fns->store_setting)(fns->ctx, pi->radio_chanspec);
		(fns->shortwindow_upd)(fns->ctx, TRUE);
	} else {
		ti->data->channel_short_window = TRUE;
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}
#endif /* PREASSOC_PWRCTRL */

#ifdef WLTXPWR_CACHE
/* Retrieve the cached ppr targets and pass them to the hardware function. */
static void
wlc_phy_txpower_retrieve_cached_target(phy_info_t *pi)
{
	uint8 core;

	FOREACH_CORE(pi, core) {
		pi->tpci->data->txpwr_max_boardlim_percore[core] =
			wlc_phy_get_cached_boardlim(pi->txpwr_cache, pi->radio_chanspec, core);
		pi->tx_power_max_per_core[core] =
			wlc_phy_get_cached_pwr_max(pi->txpwr_cache, pi->radio_chanspec, core);
		pi->tx_power_min_per_core[core] =
			wlc_phy_get_cached_pwr_min(pi->txpwr_cache, pi->radio_chanspec, core);
		pi->openlp_tx_power_min = pi->tx_power_min_per_core[core];
	}
	pi->tpci->data->txpwrnegative = 0;

#ifdef WL_SARLIMIT
	wlc_phy_sar_limit_set((wlc_phy_t*)pi,
		wlc_phy_get_cached_sar_lims(pi->txpwr_cache, pi->radio_chanspec));
#endif

	phy_tpc_recalc_tgt(pi->tpci);
}
#endif /* WLTXPWR_CACHE */

/* recalc target txpwr and apply to h/w */
void
phy_tpc_recalc_tgt(phy_tpc_info_t *ti)
{
	phy_type_tpc_fns_t *fns = ti->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	ASSERT(fns->recalc != NULL);
	(fns->recalc)(fns->ctx);
}

/* Recalc target power all phys.  This internal/static function needs to be called whenever
 * the chanspec or TX power values (user target, regulatory limits or SROM/HW limits) change.
 * This happens thorough calls to the PHY public API.
 */
/* TODO: The code could be optimized by moving the common code to phy/cmn */
/* [PHY_RE_ARCH] There are two functions: Bigger function wlc_phy_txpower_recalc_target
 * and smaller function phy_tpc_recalc_tgt which in turn call their phy specific functions
 * which are named in a haphazard manner. This needs to be cleaned up.
 */
void
wlc_phy_txpower_recalc_target(phy_info_t *pi, ppr_t *txpwr_reg, ppr_t *txpwr_targets)
{
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;
	chanspec_t chspec;
	ppr_t *srom_max_txpwr;
	ppr_t *tx_pwr_target;
	ppr_t *reg_txpwr_limit;

	PHY_CHANLOG(pi, __FUNCTION__, TS_ENTER, 0);
#ifdef WLTXPWR_CACHE
	if ((pi->tx_power_offset != NULL) && (!wlc_phy_is_pwr_cached(pi->txpwr_cache,
		TXPWR_CACHE_POWER_OFFSETS, pi->tx_power_offset))) {
			ppr_delete(pi->sh->osh, pi->tx_power_offset);
	}

	if ((!ti->data->txpwroverride) && ((pi->tx_power_offset =
		wlc_phy_get_cached_pwr(pi->txpwr_cache, pi->radio_chanspec,
		TXPWR_CACHE_POWER_OFFSETS)) != NULL) && (txpwr_targets == NULL)) {
		wlc_phy_txpower_retrieve_cached_target(pi);
		PHY_CHANLOG(pi, __FUNCTION__, TS_EXIT, 0);
		return;
	}
	pi->tx_power_offset = NULL;
#endif /* WLTXPWR_CACHE */

	if ((reg_txpwr_limit = ppr_create(pi->sh->osh,
		PPR_CHSPEC_BW(pi->radio_chanspec))) == NULL) {
		PHY_CHANLOG(pi, __FUNCTION__, TS_EXIT, 0);
		return;
	}

	if (txpwr_reg != NULL)
		wlc_phy_txpower_reg_limit_calc(pi, txpwr_reg, pi->radio_chanspec,
		reg_txpwr_limit);

	chspec = pi->radio_chanspec;

	if ((tx_pwr_target = ppr_create(pi->sh->osh, PPR_CHSPEC_BW(chspec))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", pi->sh->unit,
		     __FUNCTION__, MALLOCED(pi->sh->osh)));
		ppr_delete(pi->sh->osh, reg_txpwr_limit);
		PHY_CHANLOG(pi, __FUNCTION__, TS_EXIT, 0);
		return;
	}
	if ((srom_max_txpwr = ppr_create(pi->sh->osh, PPR_CHSPEC_BW(chspec))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", pi->sh->unit,
		     __FUNCTION__, MALLOCED(pi->sh->osh)));
		ppr_delete(pi->sh->osh, reg_txpwr_limit);
		ppr_delete(pi->sh->osh, tx_pwr_target);
		PHY_CHANLOG(pi, __FUNCTION__, TS_EXIT, 0);
		return;
	}

	if ((pi->tx_power_offset != NULL) &&
	    (ppr_get_ch_bw(pi->tx_power_offset) != PPR_CHSPEC_BW(chspec))) {
		ppr_delete(pi->sh->osh, pi->tx_power_offset);
		pi->tx_power_offset = NULL;
	}
	if (pi->tx_power_offset == NULL) {
		if ((pi->tx_power_offset = ppr_create(pi->sh->osh, PPR_CHSPEC_BW(chspec))) ==
			NULL) {
			PHY_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", pi->sh->unit,
				__FUNCTION__, MALLOCED(pi->sh->osh)));
			ppr_delete(pi->sh->osh, reg_txpwr_limit);
			ppr_delete(pi->sh->osh, tx_pwr_target);
			ppr_delete(pi->sh->osh, srom_max_txpwr);
			PHY_CHANLOG(pi, __FUNCTION__, TS_EXIT, 0);
			return;
		}
	}

	ppr_clear(pi->tx_power_offset);

	if (fns->recalc_target != NULL) {
		(fns->recalc_target)(fns->ctx, tx_pwr_target, srom_max_txpwr, reg_txpwr_limit,
		    txpwr_targets);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}

	/* Don't call the hardware specifics if we were just trying to retrieve the target powers */
	if (txpwr_targets == NULL) {
		phy_tpc_recalc_tgt(pi->tpci);
	}
	PHY_CHANLOG(pi, __FUNCTION__, TS_EXIT, 0);
}

#ifdef PPR_API
/* for CCK case, 4 rates only */
void
wlc_phy_txpwr_srom_convert_cck(uint16 po, uint8 max_pwr, ppr_dsss_rateset_t *dsss)
{
	uint8 i;
	/* Extract offsets for 4 CCK rates, convert from .5 to .25 dbm units. */
	for (i = 0; i < WL_RATESET_SZ_DSSS; i++) {
		dsss->pwr[i] = max_pwr - ((po & 0xf) * 2);
		po >>= 4;
	}
}

/* for OFDM cases, 8 rates only */
void
wlc_phy_txpwr_srom_convert_ofdm(uint32 po, uint8 max_pwr, ppr_ofdm_rateset_t *ofdm)
{
	uint8 i;
	for (i = 0; i < WL_RATESET_SZ_OFDM; i++) {
		ofdm->pwr[i] = max_pwr - ((po & 0xf) * 2);
		po >>= 4;
	}
}

void
wlc_phy_ppr_set_dsss(ppr_t* tx_srom_max_pwr, uint8 bwtype,
          ppr_dsss_rateset_t* pwr_offsets, phy_info_t *pi) {
	uint8 chain;
	for (chain = WL_TX_CHAINS_1; chain <= PHYCORENUM(pi->pubpi->phy_corenum); chain++)
		/* for 2g_dsss: S1x1, S1x2, S1x3 */
		ppr_set_dsss(tx_srom_max_pwr, bwtype, chain,
		      (const ppr_dsss_rateset_t*)pwr_offsets);
}

void
wlc_phy_ppr_set_ofdm(ppr_t* tx_srom_max_pwr, uint8 bwtype,
          ppr_ofdm_rateset_t* pwr_offsets, phy_info_t *pi) {

	uint8 chain;
	ppr_set_ofdm(tx_srom_max_pwr, bwtype, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
	       (const ppr_ofdm_rateset_t*)pwr_offsets);
	BCM_REFERENCE(chain);
	if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
		for (chain = WL_TX_CHAINS_2; chain <= PHYCORENUM(pi->pubpi->phy_corenum); chain++) {
			ppr_set_ofdm(tx_srom_max_pwr, bwtype, WL_TX_MODE_CDD, chain,
				(const ppr_ofdm_rateset_t*)pwr_offsets);
#ifdef WL_BEAMFORMING
			/* Add TXBF */
			ppr_set_ofdm(tx_srom_max_pwr, bwtype, WL_TX_MODE_TXBF, chain,
				(const ppr_ofdm_rateset_t*)pwr_offsets);
#endif
		}
	}
}
#endif /* PPR_API */

void
wlc_phy_txpwr_srom_convert_mcs_offset(uint32 po,
	uint8 offset, uint8 max_pwr, ppr_ht_mcs_rateset_t* mcs, int8 mcs7_15_offset)
{
	uint8 i;
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		mcs->pwr[i] = max_pwr - ((po & 0xF) * 2) - (offset * 2);
		po >>= 4;
	}

	mcs->pwr[WL_RATESET_SZ_HT_MCS - 1] -= mcs7_15_offset;
}

void
wlc_phy_txpwr_srom_convert_mcs(uint32 po, uint8 max_pwr, ppr_ht_mcs_rateset_t *mcs)
{
	wlc_phy_txpwr_srom_convert_mcs_offset(po, 0, max_pwr, mcs, 0);
}

static void
wlc_phy_txpwr_srom9_convert(phy_info_t *pi, int8 *srom_max,
                                            uint32 pwr_offset, uint8 tmp_max_pwr, uint8 rate_cnt)
{
	uint8 rate;
	uint8 nibble;

	if (pi->sh->sromrev < 9) {
		ASSERT(0 && "SROMREV < 9");
		return;
	}

	for (rate = 0; rate < rate_cnt; rate++) {
		nibble = (uint8)(pwr_offset & 0xf);
		pwr_offset >>= 4;
		/* nibble info indicates offset in 0.5dB units convert to 0.25dB */
		srom_max[rate] = (int8)(tmp_max_pwr - (nibble << 1));
	}
}

static void
wlc_phy_ppr_set_ht_mcs(ppr_t* tx_srom_max_pwr, uint8 bwtype,
         ppr_ht_mcs_rateset_t* pwr_offsets, phy_info_t *pi) {
	ppr_set_ht_mcs(tx_srom_max_pwr, bwtype, WL_TX_NSS_1, WL_TX_MODE_NONE,
		WL_TX_CHAINS_1, (const ppr_ht_mcs_rateset_t*)pwr_offsets);
	if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
		/* for ht_S1x2_CDD */
		ppr_set_ht_mcs(tx_srom_max_pwr, bwtype, WL_TX_NSS_1, WL_TX_MODE_CDD,
			WL_TX_CHAINS_2, (const ppr_ht_mcs_rateset_t*)pwr_offsets);
		/* for ht_S2x2_STBC */
		ppr_set_ht_mcs(tx_srom_max_pwr, bwtype, WL_TX_NSS_2, WL_TX_MODE_STBC,
			WL_TX_CHAINS_2, (const ppr_ht_mcs_rateset_t*)pwr_offsets);
		/* for ht_S2x2_SDM */
		ppr_set_ht_mcs(tx_srom_max_pwr, bwtype, WL_TX_NSS_2, WL_TX_MODE_NONE,
			WL_TX_CHAINS_2, (const ppr_ht_mcs_rateset_t*)pwr_offsets);
		if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
			/* for ht_S1x3_CDD */
			ppr_set_ht_mcs(tx_srom_max_pwr, bwtype, WL_TX_NSS_1, WL_TX_MODE_CDD,
				WL_TX_CHAINS_3, (const ppr_ht_mcs_rateset_t*)pwr_offsets);
			/* for ht_S2x3_STBC */
			ppr_set_ht_mcs(tx_srom_max_pwr, bwtype, WL_TX_NSS_2, WL_TX_MODE_STBC,
				WL_TX_CHAINS_3, (const ppr_ht_mcs_rateset_t*)pwr_offsets);
			/* for ht_S2x3_SDM */
			ppr_set_ht_mcs(tx_srom_max_pwr, bwtype, WL_TX_NSS_2, WL_TX_MODE_NONE,
				WL_TX_CHAINS_3, (const ppr_ht_mcs_rateset_t*)pwr_offsets);
			/* for ht_S3x3_SDM */
			ppr_set_ht_mcs(tx_srom_max_pwr, bwtype, WL_TX_NSS_3, WL_TX_MODE_NONE,
				WL_TX_CHAINS_3, (const ppr_ht_mcs_rateset_t*)pwr_offsets);
		}
	}
}

void
wlc_phy_txpwr_apply_srom9(phy_info_t *pi, uint8 band_num, chanspec_t chanspec,
	uint8 tmp_max_pwr, ppr_t *tx_srom_max_pwr)
{
	srom_pwrdet_t *pwrdet  = pi->pwrdet;
	ppr_dsss_rateset_t cck20_offset_ppr_api, cck20in40_offset_ppr_ppr_api;

	ppr_ofdm_rateset_t ofdm20_offset_ppr_api;
	ppr_ofdm_rateset_t ofdm20in40_offset_ppr_api;
	ppr_ofdm_rateset_t ofdmdup40_offset_ppr_api;

	ppr_ht_mcs_rateset_t mcs20_offset_ppr_api;
	ppr_ht_mcs_rateset_t mcs40_offset_ppr_api;
	ppr_ht_mcs_rateset_t mcs20in40_offset_ppr_api;

	ASSERT(tx_srom_max_pwr);

	tmp_max_pwr = pwrdet->max_pwr[0][band_num];

	if (PHYCORENUM(pi->pubpi->phy_corenum) > 1)
		tmp_max_pwr = MIN(tmp_max_pwr, pwrdet->max_pwr[1][band_num]);
	if (PHYCORENUM(pi->pubpi->phy_corenum) > 2)
		tmp_max_pwr = MIN(tmp_max_pwr, pwrdet->max_pwr[2][band_num]);

	switch (band_num) {
	case WL_CHAN_FREQ_RANGE_2G:
		if (CHSPEC_BW_LE20(chanspec)) {
			wlc_phy_txpwr_srom9_convert(pi, cck20_offset_ppr_api.pwr,
			                            pi->ppr->u.sr9.cckbw202gpo, tmp_max_pwr,
			                            WL_RATESET_SZ_DSSS);
			/* populating tx_srom_max_pwr = pi->tx_srom_max_pwr[band]
			   structure
			*/
			/* for 2g_dsss_20IN20: S1x1, S1x2, S1x3 */
			wlc_phy_ppr_set_dsss(tx_srom_max_pwr, WL_TX_BW_20,
			                     &cck20_offset_ppr_api, pi);
		}
		else if (CHSPEC_IS40(chanspec)) {
			wlc_phy_txpwr_srom9_convert(pi, cck20in40_offset_ppr_ppr_api.pwr,
			                            pi->ppr->u.sr9.cckbw20ul2gpo, tmp_max_pwr,
			                            WL_RATESET_SZ_DSSS);
			/* for 2g_dsss_20IN40: S1x1, S1x2, S1x3 */
			wlc_phy_ppr_set_dsss(tx_srom_max_pwr, WL_TX_BW_20IN40,
			                     &cck20in40_offset_ppr_ppr_api, pi);
		}
		/* Fall through to set OFDM and .11n rates for 2.4GHz band */
	case WL_CHAN_FREQ_RANGE_5G_BAND0:
	case WL_CHAN_FREQ_RANGE_5G_BAND1:
	case WL_CHAN_FREQ_RANGE_5G_BAND2:
	case WL_CHAN_FREQ_RANGE_5G_BAND3:
		/* OFDM srom conversion */
		/* ofdm_20IN20: S1x1, S1x2, S1x3 */
		/*  pwr_offsets = pi->ppr->u.sr9.ofdm[band_num].bw20; */
		if (CHSPEC_BW_LE20(chanspec)) {
			wlc_phy_txpwr_srom9_convert(pi, ofdm20_offset_ppr_api.pwr,
			                            pi->ppr->u.sr9.ofdm[band_num].bw20,
			                            tmp_max_pwr, WL_RATESET_SZ_OFDM);
			/* ofdm_20IN20: S1x1, S1x2, S1x3 */
			wlc_phy_ppr_set_ofdm(tx_srom_max_pwr, WL_TX_BW_20, &ofdm20_offset_ppr_api,
				pi);
			/* HT srom conversion  */
			/*  20MHz HT */
			/* rate_cnt = WL_RATESET_SZ_HT_MCS; */
			/* pwr_offsets = pi->ppr->u.sr9.mcs[band_num].bw20; */
			wlc_phy_txpwr_srom9_convert(pi, mcs20_offset_ppr_api.pwr,
			                            pi->ppr->u.sr9.mcs[band_num].bw20,
			                            tmp_max_pwr, WL_RATESET_SZ_HT_MCS);
			/* 20MHz HT  */
			wlc_phy_ppr_set_ht_mcs(tx_srom_max_pwr,
			                       WL_TX_BW_20, &mcs20_offset_ppr_api, pi);
		}
		else if (CHSPEC_IS40(chanspec)) {
			/* * ofdm 20 in 40 */
			/* pwr_offsets = pi->ppr->u.sr9.ofdm[band_num].bw20ul; */
			wlc_phy_txpwr_srom9_convert(pi, ofdm20in40_offset_ppr_api.pwr,
			                            pi->ppr->u.sr9.ofdm[band_num].bw20ul,
			                            tmp_max_pwr, WL_RATESET_SZ_OFDM);
			/*  ofdm dup */
			/*  pwr_offsets = pi->ppr->u.sr9.ofdm[band_num].bw40; */
			wlc_phy_txpwr_srom9_convert(pi, ofdmdup40_offset_ppr_api.pwr,
			                            pi->ppr->u.sr9.ofdm[band_num].bw40,
			                            tmp_max_pwr, WL_RATESET_SZ_OFDM);
			/* ofdm_20IN40: S1x1, S1x2, S1x3 */
			wlc_phy_ppr_set_ofdm(tx_srom_max_pwr, WL_TX_BW_20IN40,
			                     &ofdm20in40_offset_ppr_api, pi);
			/* ofdm DUP: S1x1, S1x2, S1x3 */
			wlc_phy_ppr_set_ofdm(tx_srom_max_pwr, WL_TX_BW_40,
			                     &ofdmdup40_offset_ppr_api, pi);




			/* 40MHz HT  */

			/* pwr_offsets = pi->ppr->u.sr9.mcs[band_num].bw20ul; */
			wlc_phy_txpwr_srom9_convert(pi, mcs20in40_offset_ppr_api.pwr,
			                            pi->ppr->u.sr9.mcs[band_num].bw20ul,
			                            tmp_max_pwr, WL_RATESET_SZ_HT_MCS);
			/* 20IN40MHz HT */
			/* pwr_offsets = pi->ppr->u.sr9.mcs[band_num].bw40; */
			wlc_phy_txpwr_srom9_convert(pi, mcs40_offset_ppr_api.pwr,
			                            pi->ppr->u.sr9.mcs[band_num].bw40,
			                            tmp_max_pwr, WL_RATESET_SZ_HT_MCS);
			/* 40MHz HT */
			wlc_phy_ppr_set_ht_mcs(tx_srom_max_pwr,
			                       WL_TX_BW_40, &mcs40_offset_ppr_api, pi);
			/* 20IN40MHz HT */
			wlc_phy_ppr_set_ht_mcs(tx_srom_max_pwr,
			                       WL_TX_BW_20IN40, &mcs20in40_offset_ppr_api, pi);
		}
			break;
		default:
			break;
		}
}

/* CCK Pwr Index Convergence Correction */
void
phy_tpc_cck_corr(phy_info_t *pi)
{
	ppr_dsss_rateset_t dsss;
	uint rate;
	int32 temp;
	ppr_get_dsss(pi->tx_power_offset, WL_TX_BW_20, WL_TX_CHAINS_1, &dsss);
	for (rate = 0; rate < WL_RATESET_SZ_DSSS; rate++) {
			temp = (int32)(dsss.pwr[rate]);
			temp += pi->sromi->cckPwrIdxCorr;
			dsss.pwr[rate] = (int8)((uint8)temp);
	}
	ppr_set_dsss(pi->tx_power_offset, WL_TX_BW_20, WL_TX_CHAINS_1, &dsss);
}

/* check limit? */
void
phy_tpc_check_limit(phy_info_t *pi)
{
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (!ti->priv->ucode_tssi_limit_en)
		return;

	if (fns->check == NULL)
		return;

	(fns->check)(fns->ctx);
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_tpc_register_impl)(phy_tpc_info_t *ti, phy_type_tpc_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));


	*ti->priv->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_tpc_unregister_impl)(phy_tpc_info_t *ti)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

#ifdef WLOLPC
void
wlc_phy_update_olpc_cal(wlc_phy_t *ppi, bool set, bool dbg)
{
	phy_info_t * pi = (phy_info_t *)ppi;
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->update_cal != NULL) {
		(fns->update_cal)(fns->ctx, set, dbg);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}

int8
wlc_phy_calc_ppr_pwr_cap(wlc_phy_t *ppi, uint8 core)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	if (core >= PHY_CORE_MAX) {
		PHY_ERROR(("%s: Invalid PHY core[0x%02x], setting cap to %d,"
			" hw_phytxchain[0x%02x] hw_phyrxchain[0x%02x]\n",
			__FUNCTION__, core, 127, pi->sh->hw_phytxchain, pi->sh->hw_phyrxchain));
		 return 127;
	}
	return pi->tpci->data->adjusted_pwr_cap[core];
}
#endif /* WLOLPC */

static bool
wlc_phy_cal_txpower_recalc_sw(phy_info_t *pi)
{
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* NPHY/HTPHY/ACPHY / LCN40 doesn't ever use SW power control */
	if (fns->recalc_sw != NULL) {
		return (fns->recalc_sw)(fns->ctx);
	} else {
		return FALSE;
	}
}

/* Set tx power limits */
/* BMAC_NOTE: this only needs a chanspec so that it can choose which 20/40 limits
 * to save in phy state. Would not need this if we ether saved all the limits and
 * applied them only when we were on the correct channel, or we restricted this fn
 * to be called only when on the correct channel.
 */
/* FTODO make sure driver functions are calling this version */
void
wlc_phy_txpower_limit_set(wlc_phy_t *ppi, ppr_t *txpwr, chanspec_t chanspec)
{
	phy_info_t *pi = (phy_info_t*)ppi;
#ifdef FCC_PWR_LIMIT_2G
	phy_tpc_data_t *data = pi->tpci->data;
#endif /* FCC_PWR_LIMIT_2G */

#ifdef TXPWR_TIMING
	int time1, time2;
	time1 = hnd_time_us();
#endif
	PHY_CHANLOG(pi, __FUNCTION__, TS_ENTER, 0);
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	wlc_phy_txpower_recalc_target(pi, txpwr, NULL);
	wlc_phy_cal_txpower_recalc_sw(pi);
	wlapi_enable_mac(pi->sh->physhim);
	PHY_CHANLOG(pi, __FUNCTION__, TS_EXIT, 0);

#ifdef FCC_PWR_LIMIT_2G
	if ((data->cfg.fccpwrch12 > 0 || data->cfg.fccpwrch13 > 0) &&
		(data->fcc_pwr_limit_2g || data->cfg.fccpwroverride)) {
		if (wlc_phy_isvalid_fcc_pwr_limit(ppi)) {
			wlc_phy_fcc_pwr_limit_set(ppi, TRUE);
		}
	}
#endif /* FCC_PWR_LIMIT_2G */

#ifdef TXPWR_TIMING
	time2 = hnd_time_us();
	wlc_phy_txpower_limit_set_time = time2 - time1;
#endif
}

/* user txpower limit: in qdbm units with override flag */
int
wlc_phy_txpower_set(wlc_phy_t *ppi, int8 qdbm, bool override, ppr_t *reg_pwr)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;

	/* No way for user to set maxpower on individual rates yet.
	 * Same max power is used for all rates
	 */
	ti->data->tx_user_target = qdbm;

	/* Restrict external builds to 100% Tx Power */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(WL_EXPORT_TXPOWER)
	ti->data->txpwroverride = override;
	ti->data->txpwroverrideset = override;
#else
	ti->data->txpwroverride = FALSE;
#endif


	if (pi->sh->up) {
		if (SCAN_INPROG_PHY(pi)) {
			PHY_TXPWR(("wl%d: Scan in progress, skipping txpower control\n",
				pi->sh->unit));
		} else {
			bool suspend;

			suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) &
			            MCTL_EN_MAC);

			if (!suspend)
				wlapi_suspend_mac_and_wait(pi->sh->physhim);

			if (fns->set != NULL) {
				(fns->set)(fns->ctx, reg_pwr);
			} else {
				wlc_phy_txpower_recalc_target(pi, reg_pwr, NULL);
				wlc_phy_cal_txpower_recalc_sw(pi);
			}

			if (!suspend)
				wlapi_enable_mac(pi->sh->physhim);
		}
	}
	return (0);
}

/* get sromlimit per rate for given channel. Routine does not account for ant gain */
void
wlc_phy_txpower_sromlimit(wlc_phy_t *ppi, chanspec_t chanspec, uint8 *min_pwr,
    ppr_t *max_pwr, uint8 core)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (min_pwr)
		*min_pwr = pi->min_txpower * WLC_TXPWR_DB_FACTOR;
	if (max_pwr) {
		if (fns->get_sromlimit != NULL) {
			(fns->get_sromlimit)(fns->ctx, chanspec, max_pwr, core);
		} else {
			ppr_set_cmn_val(max_pwr, (int8)WLC_TXPWR_MAX);
		}
	}
}

int
wlc_phy_txpower_get_current(wlc_phy_t *ppi, ppr_t *reg_pwr, phy_tx_power_t *power)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;
	uint8 min_pwr;
	int ret;

	if (!pi->sh->up) {
		ret = BCME_NOTUP;
		PHY_ERROR(("wl%d: %s: PHY not up: %d\n", pi->sh->unit,
			__FUNCTION__, ret));
		return ret;
	}

	if (fns->setflags != NULL) {
		(fns->setflags)(fns->ctx, power);
	} else {
		power->rf_cores = 1;
		if (ti->data->radiopwr_override == RADIOPWR_OVERRIDE_DEF)
			power->flags |= WL_TX_POWER_F_ENABLED;
		if (pi->hwpwrctrl)
			power->flags |= WL_TX_POWER_F_HW;
	}

	{
		ppr_t *txpwr_srom;

		if ((txpwr_srom = ppr_create(pi->sh->osh, PPR_CHSPEC_BW(pi->radio_chanspec))) !=
			NULL) {
			wlc_phy_txpower_sromlimit(ppi, pi->radio_chanspec, &min_pwr, txpwr_srom, 0);
			ppr_copy_struct(txpwr_srom, power->ppr_board_limits);
			ppr_delete(pi->sh->osh, txpwr_srom);
		}
	}
	/* reuse txpwr for target */
	wlc_phy_txpower_recalc_target(pi, reg_pwr, power->ppr_target_powers);

	power->display_core = ti->data->curpower_display_core;

	/* fill the est_Pout, max target power, and rate index corresponding to the max
	 * target power fields
	 */
	if ((ret = wlc_phy_get_est_pout(ppi, power->est_Pout,
			power->est_Pout_act, &power->est_Pout_cck)) != BCME_OK) {
			PHY_ERROR(("wl%d: %s: PHY func fail: %d\n", pi->sh->unit,
				__FUNCTION__, ret));
			goto fail;
	}

	if (fns->setmax != NULL) {
		(fns->setmax)(fns->ctx, power);
	} else {
		PHY_INFORM(("%s:setmax: No phy specific function\n", __FUNCTION__));
	}

fail:
	return ret;
}

bool
wlc_phy_txpower_hw_ctrl_get(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_tpc_fns_t *fns = pi->tpci->priv->fns;

	if (fns->get_hwctrl != NULL) {
		return (fns->get_hwctrl)(fns->ctx);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
		return pi->hwpwrctrl;
	}
}

#ifdef FCC_PWR_LIMIT_2G
void
wlc_phy_fcc_pwr_limit_set(wlc_phy_t *ppi, bool enable)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->set_pwr_limit != NULL) {
		(fns->set_pwr_limit)(fns->ctx, enable);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}
#endif /* FCC_PWR_LIMIT_2G */

#ifdef WL_SARLIMIT
static void
wlc_phy_sarlimit_set_int(phy_info_t *pi, int8 *sar)
{
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;
	uint core;

	PHY_TRACE(("%s\n", __FUNCTION__));

	FOREACH_CORE(pi, core) {
		ti->data->sarlimit[core] =
			MAX((sar[core] - pi->tx_pwr_backoff), pi->min_txpower);
	}
	if ((fns->set_sarlimit != NULL) && pi->sh->clk) {
		(fns->set_sarlimit)(fns->ctx);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}

void
wlc_phy_sar_limit_set(wlc_phy_t *ppi, uint32 int_val)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	uint core;
	int8 sar[PHY_MAX_CORES];

	FOREACH_CORE(pi, core) {
		sar[core] = (int8)(((int_val) >> (core * 8)) & 0xff);
	}
	/* internal */
	wlc_phy_sarlimit_set_int(pi, sar);
}
#endif /* WL_SARLIMIT */

#ifdef WL_SAR_SIMPLE_CONTROL
static void
wlc_phy_sar_limit_set_percore(wlc_phy_t *ppi, uint32 uint_val)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_tpc_info_t *ti = pi->tpci;
	phy_type_tpc_fns_t *fns = ti->priv->fns;
	uint core;

	PHY_TRACE(("%s\n", __FUNCTION__));

	for (core = 0; core < PHY_CORE_MAX; core++) {
		if (((uint_val) >> (core * SAR_VAL_LENG)) & SAR_ACTIVEFLAG_MASK) {
			ti->data->sarlimit[core] =
				(int8)(((uint_val) >> (core * SAR_VAL_LENG)) & SAR_VAL_MASK);
		} else {
			ti->data->sarlimit[core] = WLC_TXPWR_MAX;
		}
	}
	if ((fns->set_sarlimit != NULL) && pi->sh->clk) {
		(fns->set_sarlimit)(fns->ctx);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}

void
wlc_phy_dynamic_sarctrl_set(wlc_phy_t *pi, bool isctrlon)
{
	phy_info_t *piinfo = (phy_info_t*)pi;
	uint32 sarctrlmap = 0;

	if (isctrlon) {
		if (CHSPEC_IS2G(piinfo->radio_chanspec)) {
			if (piinfo->tpci->data->fcc_pwr_limit_2g) {
				sarctrlmap = piinfo->tpci->data->cfg.dynamic_sarctrl_2g_2;
			} else {
				sarctrlmap = piinfo->tpci->data->cfg.dynamic_sarctrl_2g;
			}
		} else {
			if (piinfo->tpci->data->fcc_pwr_limit_2g) {
				sarctrlmap = piinfo->tpci->data->cfg.dynamic_sarctrl_5g_2;
			} else {
				sarctrlmap = piinfo->tpci->data->cfg.dynamic_sarctrl_5g;
			}
		}
	} else {
		sarctrlmap = 0;
	}
	wlc_phy_sar_limit_set_percore(pi, sarctrlmap);
}
#endif /* WL_SAR_SIMPLE_CONTROL */

void
BCMRAMFN(wlc_phy_avvmid_txcal)(wlc_phy_t *ppi, wlc_phy_avvmid_txcal_t *val, bool set)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_tpc_fns_t *fns = pi->tpci->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->set_avvmid != NULL) {
		(fns->set_avvmid)(fns->ctx, val, set);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}

void
BCMATTACHFN(wlc_phy_txpwr_srom11_read_ppr)(phy_info_t *pi)
{

	/* Read and interpret the power-offset parameters from the SROM for each band/subband */
	if (pi->sh->sromrev >= 11) {
		uint16 _tmp;
		uint8 nibble, nibble01;

		PHY_INFORM(("Get SROM 11 Power Offset per rate\n"));
		/* --------------2G------------------- */
		/* 2G CCK */
		pi->ppr->u.sr11.cck.bw20 			=
		                (uint16)PHY_GETINTVAR(pi, rstr_cckbw202gpo);
		pi->ppr->u.sr11.cck.bw20in40 		=
		                (uint16)PHY_GETINTVAR(pi, rstr_cckbw20ul2gpo);

		pi->ppr->u.sr11.offset_2g			=
		                (uint16)PHY_GETINTVAR(pi, rstr_ofdmlrbw202gpo);
		/* 2G OFDM_20 */
		_tmp 		= (uint16)PHY_GETINTVAR(pi, rstr_dot11agofdmhrbw202gpo);
		nibble 		= pi->ppr->u.sr11.offset_2g & 0xf;
		nibble01 	= (nibble<<4)|nibble;
		nibble 		= (pi->ppr->u.sr11.offset_2g>>4)& 0xf;
		pi->ppr->u.sr11.ofdm_2g.bw20 		=
		                (((nibble<<8)|(nibble<<12))|(nibble01))&0xffff;
		pi->ppr->u.sr11.ofdm_2g.bw20 		|=
		                (_tmp << 16);
		/* 2G MCS_20 */
		pi->ppr->u.sr11.mcs_2g.bw20 		=
		                (uint32)PHY_GETINTVAR(pi, rstr_mcsbw202gpo);
		/* 2G MCS_40 */
		pi->ppr->u.sr11.mcs_2g.bw40 		=
		                (uint32)PHY_GETINTVAR(pi, rstr_mcsbw402gpo);

		pi->ppr->u.sr11.offset_20in40_l 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb20in40lrpo);
		pi->ppr->u.sr11.offset_20in40_h 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb20in40hrpo);

		pi->ppr->u.sr11.offset_dup_h 		=
		                (uint16)PHY_GETINTVAR(pi, rstr_dot11agduphrpo);
		pi->ppr->u.sr11.offset_dup_l 		=
		                (uint16)PHY_GETINTVAR(pi, rstr_dot11agduplrpo);

#ifdef BAND5G
		/* ---------------5G--------------- */
		/* 5G 11agnac_20IN20 */
		pi->ppr->u.sr11.ofdm_5g.bw20[0] 		=
		                (uint32)PHY_GETINTVAR(pi, rstr_mcsbw205glpo);
		pi->ppr->u.sr11.ofdm_5g.bw20[1] 		=
		                (uint32)PHY_GETINTVAR(pi, rstr_mcsbw205gmpo);
		pi->ppr->u.sr11.ofdm_5g.bw20[2] 		=
		                (uint32)PHY_GETINTVAR(pi, rstr_mcsbw205ghpo);

		pi->ppr->u.sr11.offset_5g[0]			=
		                (uint16)PHY_GETINTVAR(pi, rstr_mcslr5glpo);
		pi->ppr->u.sr11.offset_5g[1] 			=
		                (uint16)PHY_GETINTVAR(pi, rstr_mcslr5gmpo);
		pi->ppr->u.sr11.offset_5g[2] 			=
		                (uint16)PHY_GETINTVAR(pi, rstr_mcslr5ghpo);

		/* 5G 11nac 40IN40 */
		pi->ppr->u.sr11.mcs_5g.bw40[0] 		=
		                (uint32)PHY_GETINTVAR(pi, rstr_mcsbw405glpo);
		pi->ppr->u.sr11.mcs_5g.bw40[1] 		=
		                (uint32)PHY_GETINTVAR(pi, rstr_mcsbw405gmpo);
		pi->ppr->u.sr11.mcs_5g.bw40[2] 		=
		                (uint32)PHY_GETINTVAR(pi, rstr_mcsbw405ghpo);

		/* 5G 11nac 80IN80 */
		pi->ppr->u.sr11.mcs_5g.bw80[0] 		=
		                (uint32)PHY_GETINTVAR(pi, rstr_mcsbw805glpo);
		pi->ppr->u.sr11.mcs_5g.bw80[1] 		=
		                (uint32)PHY_GETINTVAR(pi, rstr_mcsbw805gmpo);
		pi->ppr->u.sr11.mcs_5g.bw80[2] 		=
		                (uint32)PHY_GETINTVAR(pi, rstr_mcsbw805ghpo);

		/* 5G 11agnac_20Ul/20LU */
		pi->ppr->u.sr11.offset_20in80_l[0] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb20in80and160lr5glpo);
		pi->ppr->u.sr11.offset_20in80_h[0] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb20in80and160hr5glpo);
		pi->ppr->u.sr11.offset_20in80_l[1] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb20in80and160lr5gmpo);
		pi->ppr->u.sr11.offset_20in80_h[1] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb20in80and160hr5gmpo);
		pi->ppr->u.sr11.offset_20in80_l[2] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb20in80and160lr5ghpo);
		pi->ppr->u.sr11.offset_20in80_h[2] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb20in80and160hr5ghpo);

		/* 5G 11nac_40IN80 */
		pi->ppr->u.sr11.offset_40in80_l[0] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb40and80lr5glpo);
		pi->ppr->u.sr11.offset_40in80_h[0] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb40and80hr5glpo);
		pi->ppr->u.sr11.offset_40in80_l[1] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb40and80lr5gmpo);
		pi->ppr->u.sr11.offset_40in80_h[1] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb40and80hr5gmpo);
		pi->ppr->u.sr11.offset_40in80_l[2] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb40and80lr5ghpo);
		pi->ppr->u.sr11.offset_40in80_h[2] 	=
		                (uint16)PHY_GETINTVAR(pi, rstr_sb40and80hr5ghpo);

#endif /* BAND5G */

#ifdef NO_PROPRIETARY_VHT_RATES
#else
#ifdef WL11AC
	    PHY_INFORM(("Get SROM <= 11 1024 QAM Power Offset per rate\n"));
	    pi->ppr->u.sr11.pp1024qam2g = (uint16)PHY_GETINTVAR(pi, rstr_mcs1024qam2gpo);

	    pi->ppr->u.sr11.ppmcsexp[0] = (uint32)PHY_GETINTVAR(pi, rstr_mcs8poexp);
	    pi->ppr->u.sr11.ppmcsexp[1] = (uint32)PHY_GETINTVAR(pi, rstr_mcs9poexp);
	    pi->ppr->u.sr11.ppmcsexp[2] = (uint32)PHY_GETINTVAR(pi, rstr_mcs10poexp);
	    pi->ppr->u.sr11.ppmcsexp[3] = (uint32)PHY_GETINTVAR(pi, rstr_mcs11poexp);
#ifdef BAND5G
	    /* 1024 qam fields for SROM <= 12 */
	    pi->ppr->u.sr11.pp1024qam5g[0] = (uint32)PHY_GETINTVAR(pi, rstr_mcs1024qam5glpo);
	    pi->ppr->u.sr11.pp1024qam5g[1] = (uint32)PHY_GETINTVAR(pi, rstr_mcs1024qam5gmpo);
	    pi->ppr->u.sr11.pp1024qam5g[2] = (uint32)PHY_GETINTVAR(pi, rstr_mcs1024qam5ghpo);
	    pi->ppr->u.sr11.pp1024qam5g[3] = (uint32)PHY_GETINTVAR(pi, rstr_mcs1024qam5gx1po);
	    pi->ppr->u.sr11.pp1024qam5g[4] = (uint32)PHY_GETINTVAR(pi, rstr_mcs1024qam5gx2po);

#endif /* BAND5G */
#endif /* WL11AC */
#endif /* NO_PROPRIETARY_VHT_RATES */
		if (0) {
			/* printf srom value for verification */
			PHY_ERROR(("		cckbw202gpo=%x\n", pi->ppr->u.sr11.cck.bw20));
			PHY_ERROR(("		cckbw20ul2gpo=%x\n", pi->ppr->u.sr11.cck.bw20in40));
			PHY_ERROR(("		ofdmlrbw202gpo=%x\n", pi->ppr->u.sr11.offset_2g));
			PHY_ERROR(("		dot11agofdmhrbw202gpo=%x\n", _tmp));
			PHY_ERROR(("		mcsbw202gpo=%x\n", pi->ppr->u.sr11.mcs_2g.bw20));
			PHY_ERROR(("		mcsbw402gpo=%x\n", pi->ppr->u.sr11.mcs_2g.bw40));
			PHY_ERROR(("		sb20in40lrpo=%x\n",
				pi->ppr->u.sr11.offset_20in40_l));
			PHY_ERROR(("		sb20in40hrpo=%x\n",
				pi->ppr->u.sr11.offset_20in40_h));
			PHY_ERROR(("		dot11agduphrpo=%x\n",
				pi->ppr->u.sr11.offset_dup_h));
			PHY_ERROR(("		dot11agduplrpo=%x\n",
				pi->ppr->u.sr11.offset_dup_l));
			PHY_ERROR(("		mcsbw205glpo=%x\n",
				pi->ppr->u.sr11.ofdm_5g.bw20[0]));
			PHY_ERROR(("		mcsbw205gmpo=%x\n",
				pi->ppr->u.sr11.ofdm_5g.bw20[1]));
			PHY_ERROR(("		mcsbw205ghpo=%x\n",
				pi->ppr->u.sr11.ofdm_5g.bw20[2]));
			PHY_ERROR(("		mcslr5glpo=%x\n", pi->ppr->u.sr11.offset_5g[0]));
			PHY_ERROR(("		mcslr5gmpo=%x\n", pi->ppr->u.sr11.offset_5g[1]));
			PHY_ERROR(("		mcslr5ghpo=%x\n", pi->ppr->u.sr11.offset_5g[2]));
			PHY_ERROR(("		mcsbw405glpo=%x\n",
				pi->ppr->u.sr11.mcs_5g.bw40[0]));
			PHY_ERROR(("		mcsbw405gmpo=%x\n",
				pi->ppr->u.sr11.mcs_5g.bw40[1]));
			PHY_ERROR(("		mcsbw405ghpo=%x\n",
				pi->ppr->u.sr11.mcs_5g.bw40[2]));
			PHY_ERROR(("		mcsbw805glpo=%x\n",
				pi->ppr->u.sr11.mcs_5g.bw80[0]));
			PHY_ERROR(("		mcsbw805gmpo=%x\n",
				pi->ppr->u.sr11.mcs_5g.bw80[1]));
			PHY_ERROR(("		mcsbw805ghpo=%x\n",
				pi->ppr->u.sr11.mcs_5g.bw80[2]));
			PHY_ERROR(("		sb20in80and160lr5glpo=%x\n",
			                    pi->ppr->u.sr11.offset_20in80_l[0]));
			PHY_ERROR(("		sb20in80and160hr5glpo=%x\n",
			                    pi->ppr->u.sr11.offset_20in80_h[0]));
			PHY_ERROR(("		sb20in80and160lr5gmpo=%x\n",
			                    pi->ppr->u.sr11.offset_20in80_l[1]));
			PHY_ERROR(("		sb20in80and160hr5gmpo=%x\n",
			                    pi->ppr->u.sr11.offset_20in80_h[1]));
			PHY_ERROR(("		sb20in80and160lr5ghpo=%x\n",
			                    pi->ppr->u.sr11.offset_20in80_l[2]));
			PHY_ERROR(("		sb20in80and160hr5ghpo=%x\n",
			                    pi->ppr->u.sr11.offset_20in80_h[2]));
			PHY_ERROR(("		sb40and80lr5glpo=%x\n",
			                    pi->ppr->u.sr11.offset_40in80_l[0]));
			PHY_ERROR(("		sb40and80hr5glpo=%x\n",
			                    pi->ppr->u.sr11.offset_40in80_h[0]));
			PHY_ERROR(("		sb40and80lr5gmpo=%x\n",
			                    pi->ppr->u.sr11.offset_40in80_l[1]));
			PHY_ERROR(("		sb40and80hr5gmpo=%x\n",
			                    pi->ppr->u.sr11.offset_40in80_h[1]));
			PHY_ERROR(("		sb40and80lr5ghpo=%x\n",
			                    pi->ppr->u.sr11.offset_40in80_l[2]));
			PHY_ERROR(("		sb40and80hr5ghpo=%x\n",
			                    pi->ppr->u.sr11.offset_40in80_h[2]));
		}
	}
}


void
BCMATTACHFN(wlc_phy_txpwr_srom12_read_ppr)(phy_info_t *pi)
{
	if (!(SROMREV(pi->sh->sromrev) < 12)) {
	    /* Read and interpret the power-offset parameters from the SROM for each band/subband */
	    uint8 nibble, nibble01;
	    ASSERT(pi->sh->sromrev >= 12);

	    PHY_INFORM(("Get SROM 12 Power Offset per rate\n"));
	    /* --------------2G------------------- */
	    /* 2G CCK */
	    pi->ppr->u.sr11.cck.bw20 = (uint16)PHY_GETINTVAR_SLICE(pi, rstr_cckbw202gpo);
	    pi->ppr->u.sr11.cck.bw20in40 = (uint16)PHY_GETINTVAR_SLICE(pi, rstr_cckbw20ul2gpo);

	    pi->ppr->u.sr11.offset_2g = (uint16)PHY_GETINTVAR_SLICE(pi, rstr_ofdmlrbw202gpo);
	    /* 2G OFDM_20 */
	    nibble = pi->ppr->u.sr11.offset_2g & 0xf;
	    nibble01 = (nibble<<4)|nibble;
	    nibble = (pi->ppr->u.sr11.offset_2g>>4)& 0xf;
	    pi->ppr->u.sr11.ofdm_2g.bw20 = (((nibble<<8)|(nibble<<12))|(nibble01))&0xffff;
	    pi->ppr->u.sr11.ofdm_2g.bw20 |=
	     (((uint16)PHY_GETINTVAR_SLICE(pi, rstr_dot11agofdmhrbw202gpo)) << 16);
	    /* 2G MCS_20 */
	    pi->ppr->u.sr11.mcs_2g.bw20 = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw202gpo);
	    /* 2G MCS_40 */
	    pi->ppr->u.sr11.mcs_2g.bw40 = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw402gpo);

	    pi->ppr->u.sr11.offset_20in40_l = (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in40lrpo);
	    pi->ppr->u.sr11.offset_20in40_h = (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in40hrpo);

	    pi->ppr->u.sr11.offset_dup_h = (uint16)PHY_GETINTVAR_SLICE(pi, rstr_dot11agduphrpo);
	    pi->ppr->u.sr11.offset_dup_l = (uint16)PHY_GETINTVAR_SLICE(pi, rstr_dot11agduplrpo);

#ifdef BAND5G
	    /* ---------------5G--------------- */
	    /* 5G 11agnac_20IN20 */
	    pi->ppr->u.sr11.ofdm_5g.bw20[0] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw205glpo);
	    pi->ppr->u.sr11.ofdm_5g.bw20[1] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw205gmpo);
	    pi->ppr->u.sr11.ofdm_5g.bw20[2] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw205ghpo);
	    pi->ppr->u.sr11.ofdm_5g.bw20[3] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw205gx1po);
	    pi->ppr->u.sr11.ofdm_5g.bw20[4] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw205gx2po);

	    pi->ppr->u.sr11.offset_5g[0]	= (uint16)PHY_GETINTVAR_SLICE(pi, rstr_mcslr5glpo);
	    pi->ppr->u.sr11.offset_5g[1] = (uint16)PHY_GETINTVAR_SLICE(pi, rstr_mcslr5gmpo);
	    pi->ppr->u.sr11.offset_5g[2] = (uint16)PHY_GETINTVAR_SLICE(pi, rstr_mcslr5ghpo);
	    pi->ppr->u.sr11.offset_5g[3] = (uint16)PHY_GETINTVAR_SLICE(pi, rstr_mcslr5gx1po);
	    pi->ppr->u.sr11.offset_5g[4] = (uint16)PHY_GETINTVAR_SLICE(pi, rstr_mcslr5gx2po);

	    /* 5G 11nac 40IN40 */
	    pi->ppr->u.sr11.mcs_5g.bw40[0] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw405glpo);
	    pi->ppr->u.sr11.mcs_5g.bw40[1] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw405gmpo);

	    pi->ppr->u.sr11.mcs_5g.bw40[2] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw405ghpo);
	    pi->ppr->u.sr11.mcs_5g.bw40[3] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw405gx1po);
	    pi->ppr->u.sr11.mcs_5g.bw40[4] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw405gx2po);

	    /* 5G 11nac 80IN80 */
	    pi->ppr->u.sr11.mcs_5g.bw80[0] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw805glpo);
	    pi->ppr->u.sr11.mcs_5g.bw80[1] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw805gmpo);
	    pi->ppr->u.sr11.mcs_5g.bw80[2] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw805ghpo);
	    pi->ppr->u.sr11.mcs_5g.bw80[3] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw805gx1po);
	    pi->ppr->u.sr11.mcs_5g.bw80[4] = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_mcsbw805gx2po);

	    pi->ppr->u.sr11.offset_20in80_l[0] =
	     (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in80and160lr5glpo);
	    pi->ppr->u.sr11.offset_20in80_h[0] =
	     (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in80and160hr5glpo);
	    pi->ppr->u.sr11.offset_20in80_l[1] =
	     (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in80and160lr5gmpo);
	    pi->ppr->u.sr11.offset_20in80_h[1] =
	     (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in80and160hr5gmpo);
	    pi->ppr->u.sr11.offset_20in80_l[2] =
	     (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in80and160lr5ghpo);
	    pi->ppr->u.sr11.offset_20in80_h[2] =
	     (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in80and160hr5ghpo);
	    pi->ppr->u.sr11.offset_20in80_l[3] =
	     (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in80and160lr5gx1po);
	    pi->ppr->u.sr11.offset_20in80_h[3] =
	     (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in80and160hr5gx1po);
	    pi->ppr->u.sr11.offset_20in80_l[4] =
	     (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in80and160lr5gx2po);
	    pi->ppr->u.sr11.offset_20in80_h[4] =
	     (uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb20in80and160hr5gx2po);

		pi->ppr->u.sr11.offset_40in80_l[0] =
			(uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb40and80lr5glpo);
		pi->ppr->u.sr11.offset_40in80_h[0] =
			(uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb40and80hr5glpo);
		pi->ppr->u.sr11.offset_40in80_l[1] =
			(uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb40and80lr5gmpo);
		pi->ppr->u.sr11.offset_40in80_h[1] =
			(uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb40and80hr5gmpo);
		pi->ppr->u.sr11.offset_40in80_l[2] =
			(uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb40and80lr5ghpo);
		pi->ppr->u.sr11.offset_40in80_h[2] =
			(uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb40and80hr5ghpo);
		pi->ppr->u.sr11.offset_40in80_l[3] =
			(uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb40and80lr5gx1po);
		pi->ppr->u.sr11.offset_40in80_h[3] =
			(uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb40and80hr5gx1po);
		pi->ppr->u.sr11.offset_40in80_l[4] =
			(uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb40and80lr5gx2po);
		pi->ppr->u.sr11.offset_40in80_h[4] =
			(uint16)PHY_GETINTVAR_SLICE(pi, rstr_sb40and80hr5gx2po);
#endif /* BAND5G */
	}
}

#ifdef NO_PROPRIETARY_VHT_RATES
#else
#ifdef WL11AC
/* for MCS10/11 cases, 2 rates only */
void
wlc_phy_txpwr_srom11_ext_1024qam_convert_mcs_5g(uint32 po, chanspec_t chanspec,
         uint8 tmp_max_pwr, ppr_vht_mcs_rateset_t* vht) {

	if (!(sizeof(*vht) > 10)) {
		PHY_ERROR(("%s: should not call me this file without VHT MCS10/11 supported!\n",
				__FUNCTION__));
		return;
	}

	if (CHSPEC_IS20(chanspec)) {
		vht->pwr[10] = tmp_max_pwr - ((po & 0xf) << 1);
		vht->pwr[11] = tmp_max_pwr - (((po >> 4) & 0xf) << 1);
	} else if (CHSPEC_IS40(chanspec)) {
		vht->pwr[10] = tmp_max_pwr - (((po >> 8) & 0xf) << 1);
		vht->pwr[11] = tmp_max_pwr - (((po >> 12) & 0xf) << 1);
	} else if (CHSPEC_IS80(chanspec)) {
		vht->pwr[10] = tmp_max_pwr - (((po >> 16) & 0xf) << 1);
		vht->pwr[11] = tmp_max_pwr - (((po >> 20) & 0xf) << 1);
	} else { /* when we are ready to BU 80p80 chanspec, settings have to be updated */
		vht->pwr[10] = tmp_max_pwr - (((po >> 24) & 0xf) << 1);
		vht->pwr[11] = tmp_max_pwr - (((po >> 28) & 0xf) << 1);
	}
}

void
wlc_phy_txpwr_srom11_ext_1024qam_convert_mcs_2g(uint16 po, chanspec_t chanspec,
         uint8 tmp_max_pwr, ppr_vht_mcs_rateset_t* vht) {
	if (!(sizeof(*vht) > 10)) {
		PHY_ERROR(("%s: should not call me this file without VHT MCS10/11 supported!\n",
			__FUNCTION__));
		return;
	}

	if (CHSPEC_IS20(chanspec)) {
		vht->pwr[10] = tmp_max_pwr - ((po & 0xf) << 1);
		vht->pwr[11] = tmp_max_pwr - (((po >> 4) & 0xf) << 1);
	} else if (CHSPEC_IS40(chanspec)) {
		vht->pwr[10] = tmp_max_pwr - (((po >> 8) & 0xf) << 1);
		vht->pwr[11] = tmp_max_pwr - (((po >> 12) & 0xf) << 1);
	}

}
#endif /* WL11AC */
#endif /* NO_PROPRIETARY_VHT_RATES */


void
wlc_phy_txpwr_percent_set(wlc_phy_t *ppi, uint8 txpwr_percent)
{
	phy_info_t *pi = (phy_info_t*)ppi;

	pi->tpci->data->txpwr_percent = txpwr_percent;
}

int
wlc_phy_txpower_core_offset_set(wlc_phy_t *ppi, struct phy_txcore_pwr_offsets *offsets)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	int err = BCME_UNSUPPORTED;
	phy_type_tpc_fns_t *fns = pi->tpci->priv->fns;

	if (fns->txcorepwroffsetset) {
		err = (fns->txcorepwroffsetset)(fns->ctx, offsets);
	}
	return err;
}

int
wlc_phy_txpower_core_offset_get(wlc_phy_t *ppi, struct phy_txcore_pwr_offsets *offsets)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	int err = BCME_UNSUPPORTED;
	phy_type_tpc_fns_t *fns = pi->tpci->priv->fns;

	if (fns->txcorepwroffsetget) {
		err = (fns->txcorepwroffsetget)(fns->ctx, offsets);
	}
	return err;
}

/* user txpower limit: in qdbm units with override flag */
int
wlc_phy_txpower_get(wlc_phy_t *ppi, int8 *qdbm, bool *override)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	ASSERT(qdbm != NULL);

	*qdbm = pi->tpci->data->tx_user_target;

	if (pi->openlp_tx_power_on) {
	  if (pi->tpci->data->txpwrnegative)
		*qdbm = (-1 * pi->openlp_tx_power_min) | WL_TXPWR_NEG;
	  else
		*qdbm = pi->openlp_tx_power_min;
	}

	if (override != NULL)
		*override = pi->tpci->data->txpwroverride;
	return (0);
}

/* user txpower limit: in qdbm units with override flag */
int
wlc_phy_neg_txpower_set(wlc_phy_t *ppi, uint qdbm)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	if (pi->sh->up) {
		if (SCAN_INPROG_PHY(pi)) {
			PHY_TXPWR(("wl%d: Scan in progress, skipping txpower control\n",
				pi->sh->unit));
		} else {
			bool suspend;
			suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);

			if (!suspend)
				wlapi_suspend_mac_and_wait(pi->sh->physhim);

			pi->openlp_tx_power_min = -1*qdbm;
			pi->tpci->data->txpwrnegative = 1;
			pi->tpci->data->txpwroverride = 1;

			phy_tpc_recalc_tgt(pi->tpci);

			if (!suspend)
				wlapi_enable_mac(pi->sh->physhim);

		}
	}
	return (0);
}

#if (defined(BCMINTERNAL) || defined(WLTEST))
int
phy_tpc_set_pavars(phy_tpc_info_t *tpci, void* a, void* p)
{
	phy_type_tpc_fns_t *fns = tpci->priv->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));
	if (fns->set_pavars != NULL) {
		return (fns->set_pavars)(fns->ctx, a, p);
	} else {
		PHY_ERROR(("Unsupported PHY type!\n"));
		return BCME_UNSUPPORTED;
	}
}

int
phy_tpc_get_pavars(phy_tpc_info_t *tpci, void* a, void* p)
{
	phy_type_tpc_fns_t *fns = tpci->priv->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));
	if (fns->get_pavars != NULL) {
		return (fns->get_pavars)(fns->ctx, a, p);
	} else {
		PHY_ERROR(("Unsupported PHY type!\n"));
		return BCME_UNSUPPORTED;
	}
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */


void
BCMRAMFN(wlc_phy_set_country)(wlc_phy_t *ppi, const char *val)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	pi->tpci->data->ccode_ptr = val;
}

int8
BCMRAMFN(wlc_phy_maxtxpwr_lowlimit)(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_tpc_fns_t *fns = pi->tpci->priv->fns;
	if (fns->get_maxtxpwr_lowlimit != NULL)
		return (fns->get_maxtxpwr_lowlimit)(fns->ctx);
	else
		return 0;
}
