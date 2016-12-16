/*
 * NPHY Cache module implementation
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
 * $Id: phy_ht_cache.c 656063 2016-08-25 03:56:25Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_cache.h"
#include <phy_ht.h>
#include <phy_ht_cache.h>
#include <bcmutils.h>

#include <wlc_phyreg_ht.h>
#include <phy_utils_reg.h>
/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_phy_radio.h>
#include <bcmwifi_channels.h> /* TODO: Do we include this ??? Needed for CHSPEC_CHANNEL */
/* #include <phy_ht_txiqlocal.h> */
#include <phy_utils_reg.h>
#include <phy_ht_info.h>

#define MOD_PHYREG(pi, reg, field, value)				\
	phy_utils_mod_phyreg(pi, reg,						\
	            reg##_##field##_MASK, (value) << reg##_##field##_##SHIFT);

#define MOD_PHYREGC(pi, reg, core, field, value)			\
	phy_utils_mod_phyreg(pi, (core == 0) ? \
	reg##_CR0 : ((core == 1) ? reg##_CR1 : reg##_CR2), \
	            reg##_##field##_MASK, (value) << reg##_##field##_##SHIFT);

#define READ_PHYREG(pi, reg, field)				\
	((phy_utils_read_phyreg(pi, reg)					\
	  & reg##_##field##_##MASK) >> reg##_##field##_##SHIFT)

#define READ_PHYREGC(pi, reg, core, field)				\
	((phy_utils_read_phyreg(pi, (core == 0) ? \
	 reg##_CR0 : ((core == 1) ? reg##_CR1 : reg##_CR2)) \
	  & reg##_##field##_##MASK) >> reg##_##field##_##SHIFT)

/* module private states */
struct phy_ht_cache_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_cache_info_t *ci;
};

/* local functions */
#if defined(PHYCAL_CACHING)
void wlc_phy_cal_cache_htphy(phy_type_cache_ctx_t * cache_ctx);
static void wlc_phy_cal_cache_dbg_htphy(phy_type_cache_ctx_t * cache_ctx, ch_calcache_t *ctx);
static int wlc_phy_cal_cache_restore_htphy(phy_type_cache_ctx_t * cache_ctx);
#endif /* PHYCAL_CACHING */

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void wlc_phydump_cal_cache_htphy(phy_type_cache_ctx_t * cache_ctx, ch_calcache_t *ctx,
		struct bcmstrbuf *b);
#endif

/* To be removed when moved to proper module */
extern void wlc_phy_txpwrctrl_set_idle_tssi_htphy(phy_info_t *pi, int8 idle_tssi, uint8 core);

/* register phy type specific implementation */
phy_ht_cache_info_t *
BCMATTACHFN(phy_ht_cache_register_impl)(phy_info_t *pi, phy_ht_info_t *hti,
	phy_cache_info_t *ci)
{
	phy_ht_cache_info_t *info;
	phy_type_cache_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((info = phy_malloc(pi, sizeof(phy_ht_cache_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_ht_cache_info_t));
	info->pi = pi;
	info->hti = hti;
	info->ci = ci;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = info;
#ifdef PHYCAL_CACHING
	fns.cal_cache = wlc_phy_cal_cache_htphy;
	fns.restore = wlc_phy_cal_cache_restore_htphy;
#endif /* PHYCAL_CACHING */

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	fns.dump_chanctx = wlc_phydump_cal_cache_htphy;
#endif

	if (phy_cache_register_impl(ci, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_cache_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return info;

	/* error handling */
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_ht_cache_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_cache_unregister_impl)(phy_ht_cache_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_cache_info_t *ci = info->ci;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_cache_unregister_impl(ci);

	phy_mfree(pi, info, sizeof(phy_ht_cache_info_t));
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void
wlc_phydump_cal_cache_htphy(phy_type_cache_ctx_t * cache_ctx, ch_calcache_t *ctx,
	struct bcmstrbuf *b)
{
	phy_info_t *pi = (phy_info_t *)cache_ctx;
	uint i;
	htphy_calcache_t *cache = NULL;
	uint16 chan = CHSPEC_CHANNEL(pi->radio_chanspec);

	if (ISHTPHY(pi)) {
		cache = &ctx->u.htphy_cache;
	} else
		return;

	FOREACH_CORE(pi, i) {
		bcm_bprintf(b, "CORE %d:\n", i);
		bcm_bprintf(b, "\tofdm_txa:0x%x  ofdm_txb:0x%x  ofdm_txd:0x%x\n",
		            cache->ofdm_txa[i], cache->ofdm_txb[i], cache->ofdm_txd[i]);
		bcm_bprintf(b, "\tbphy_txa:0x%x  bphy_txb:0x%x  bphy_txd:0x%x\n",
		            cache->bphy_txa[i], cache->bphy_txb[i], cache->bphy_txd[i]);
		bcm_bprintf(b, "\ttxei:0x%x  txeq:0x%x\n", cache->txei[i], cache->txeq[i]);
		bcm_bprintf(b, "\ttxfi:0x%x  txfq:0x%x\n", cache->txfi[i], cache->txfq[i]);
		bcm_bprintf(b, "\trxa:0x%x  rxb:0x%x\n", cache->rxa[i], cache->rxb[i]);
		if (pi->itssical || (pi->itssi_cap_low5g && chan <= 48 && chan >= 36)) {
			bcm_bprintf(b, "\tidletssi:0x%x\n", cache->idle_tssi[i]);
			bcm_bprintf(b, "\tvmid:0x%x\n", cache->Vmid[i]);
		}
	}
}
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */

#if defined(PHYCAL_CACHING)
static int
wlc_phy_cal_cache_restore_htphy(phy_type_cache_ctx_t * cache_ctx)
{
	phy_info_t *pi = (phy_info_t *)cache_ctx;
	ch_calcache_t *ctx;
	htphy_calcache_t *cache = NULL;
	bool suspend;
	uint8 core;
	uint16 tbl_cookie = TXCAL_CACHE_VALID;
	uint16 chan = CHSPEC_CHANNEL(pi->radio_chanspec);

	ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);

	if (!ctx) {
		PHY_ERROR(("wl%d: %s: Chanspec 0x%x not found in calibration cache\n",
		           pi->sh->unit, __FUNCTION__, pi->radio_chanspec));
		return BCME_ERROR;
	}

	if (!ctx->valid) {
		PHY_ERROR(("wl%d: %s: Chanspec 0x%x found, but not valid in phycal cache\n",
		           pi->sh->unit, __FUNCTION__, pi->radio_chanspec));
		return BCME_ERROR;
	}

	PHY_CAL(("wl%d: %s: Restoring all cal coeffs from calibration cache for chanspec 0x%x\n",
	           pi->sh->unit, __FUNCTION__, pi->radio_chanspec));

	cache = &ctx->u.htphy_cache;

	suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);
	if (!suspend) {
		/* suspend mac */
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
	}
	phy_utils_phyreg_enter(pi);

	/* restore the txcal from cache */
	FOREACH_CORE(pi, core) {
		uint16 ab_int[2];
		/* Restore OFDM Tx IQ Imb Coeffs A,B and Digital Loft Comp Coeffs */
		ab_int[0] = cache->ofdm_txa[core];
		ab_int[1] = cache->ofdm_txb[core];
		wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_WRITE,
		                                ab_int, TB_OFDM_COEFFS_AB, core);
		wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_WRITE,
		                                &cache->ofdm_txd[core], TB_OFDM_COEFFS_D, core);
		/* Restore BPHY Tx IQ Imb Coeffs A,B and Digital Loft Comp Coeffs */
		ab_int[0] = cache->bphy_txa[core];
		ab_int[1] = cache->bphy_txb[core];
		wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_WRITE,
		                                ab_int, TB_BPHY_COEFFS_AB, core);
		wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_WRITE,
		                                &cache->bphy_txd[core], TB_BPHY_COEFFS_D, core);
		/* Restore Analog Tx Loft Comp Coeffs */
		phy_utils_write_radioreg(pi, RADIO_2059_TX_LOFT_FINE_I(core),
		                cache->txei[core]);
		phy_utils_write_radioreg(pi, RADIO_2059_TX_LOFT_FINE_Q(core),
		                cache->txeq[core]);
		phy_utils_write_radioreg(pi, RADIO_2059_TX_LOFT_COARSE_I(core),
		                cache->txfi[core]);
		phy_utils_write_radioreg(pi, RADIO_2059_TX_LOFT_COARSE_Q(core),
		                cache->txfq[core]);
		/* Restore Rx IQ Imb Coeffs */
		phy_utils_write_phyreg(pi, HTPHY_RxIQCompA(core), cache->rxa[core]);
		phy_utils_write_phyreg(pi, HTPHY_RxIQCompB(core), cache->rxb[core]);
		if (pi->itssical || (pi->itssi_cap_low5g && chan <= 48 && chan >= 36)) {
			/* Restore Idle TSSI & Vmid values */
			wlc_phy_txpwrctrl_set_idle_tssi_htphy(pi, cache->idle_tssi[core], core);
			wlc_phy_table_write_htphy(pi, HTPHY_TBL_ID_AFECTRL, 1, 0x0b + 0x10*core,
			                          16, &cache->Vmid[core]);
		}
	}

	/* Validate the Calibration Results */
	wlc_phy_table_write_htphy(pi, HTPHY_TBL_ID_IQLOCAL, 1,
	                          IQTBL_CACHE_COOKIE_OFFSET, 16, &tbl_cookie);

	phy_utils_phyreg_exit(pi);

	/* unsuspend mac */
	if (!suspend) {
		wlapi_enable_mac(pi->sh->physhim);
	}

	PHY_CAL(("wl%d: %s: Restored values for chanspec 0x%x are:\n", pi->sh->unit,
	           __FUNCTION__, pi->radio_chanspec));
	wlc_phy_cal_cache_dbg_htphy((wlc_phy_t *)pi, ctx);
	return BCME_OK;
}

void wlc_phy_cal_cache_htphy(phy_type_cache_ctx_t * cache_ctx)
{
	phy_info_t *pi = (phy_info_t *) cache_ctx;
	ch_calcache_t *ctx;
	htphy_calcache_t *cache;
	uint8 core;
	uint16 tbl_cookie;
	uint16 chan = CHSPEC_CHANNEL(pi->radio_chanspec);

	ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);

	/* A context must have been created before reaching here */
	ASSERT(ctx != NULL);
	if (ctx == NULL)
		return;

	/* Ensure that the Callibration Results are valid */
	wlc_phy_table_read_htphy(pi, HTPHY_TBL_ID_IQLOCAL, 1,
		IQTBL_CACHE_COOKIE_OFFSET, 16, &tbl_cookie);
	if (tbl_cookie != TXCAL_CACHE_VALID) {
		return;
	}

	ctx->valid = TRUE;

	cache = &ctx->u.htphy_cache;

	/* save the callibration to cache */
	FOREACH_CORE(pi, core) {
		uint16 ab_int[2];
		/* Save OFDM Tx IQ Imb Coeffs A,B and Digital Loft Comp Coeffs */
		wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_READ,
		                                ab_int, TB_OFDM_COEFFS_AB, core);
		cache->ofdm_txa[core] = ab_int[0];
		cache->ofdm_txb[core] = ab_int[1];
		wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_READ,
		                                &cache->ofdm_txd[core], TB_OFDM_COEFFS_D, core);
		/* Save OFDM Tx IQ Imb Coeffs A,B and Digital Loft Comp Coeffs */
		wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_READ,
		                                ab_int, TB_BPHY_COEFFS_AB, core);
		cache->bphy_txa[core] = ab_int[0];
		cache->bphy_txb[core] = ab_int[1];
		wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_READ,
		                                &cache->bphy_txd[core], TB_BPHY_COEFFS_D, core);
		/* Save Analog Tx Loft Comp Coeffs */
		cache->txei[core] = (uint8)phy_utils_read_radioreg(pi,
			RADIO_2059_TX_LOFT_FINE_I(core));
		cache->txeq[core] = (uint8)phy_utils_read_radioreg(pi,
			RADIO_2059_TX_LOFT_FINE_Q(core));
		cache->txfi[core] = (uint8)phy_utils_read_radioreg(pi,
			RADIO_2059_TX_LOFT_COARSE_I(core));
		cache->txfq[core] = (uint8)phy_utils_read_radioreg(pi,
			RADIO_2059_TX_LOFT_COARSE_Q(core));
		/* Save Rx IQ Imb Coeffs */
		cache->rxa[core] = phy_utils_read_phyreg(pi, HTPHY_RxIQCompA(core));
		cache->rxb[core] = phy_utils_read_phyreg(pi, HTPHY_RxIQCompB(core));

		if (pi->itssical || (pi->itssi_cap_low5g && chan <= 48 && chan >= 36)) {
			/* Save Idle TSSI & Vmid values */
			/* Read idle TSSI in 2s complement format (max is 0x1f) */
			switch (core) {
			case 0:
				cache->idle_tssi[core] =
				        READ_PHYREG(pi, HTPHY_TxPwrCtrlIdleTssi, idleTssi0);
				break;
			case 1:
				cache->idle_tssi[core] =
				        READ_PHYREG(pi, HTPHY_TxPwrCtrlIdleTssi, idleTssi1);
				break;
			case 2:
				cache->idle_tssi[core] =
				        READ_PHYREG(pi, HTPHY_TxPwrCtrlIdleTssi1, idleTssi2);
				break;
			default:
				ASSERT(0);
			}

			/* Save Vmid values */
			wlc_phy_table_read_htphy(pi, HTPHY_TBL_ID_AFECTRL, 1, 0x0b + 0x10*core,
			                         16, &cache->Vmid[core]);
		}
	}

	PHY_CAL(("wl%d: %s: Cached cal values for chanspec 0x%x are:\n",
		pi->sh->unit, __FUNCTION__,  pi->radio_chanspec));
	wlc_phy_cal_cache_dbg_htphy(pi, ctx);
}

static void
wlc_phy_cal_cache_dbg_htphy(phy_type_cache_ctx_t * cache_ctx, ch_calcache_t *ctx)
{
	uint i;
	htphy_calcache_t *cache = NULL;
	phy_info_t *pi = (phy_info_t *) cache_ctx;
	uint16 chan = CHSPEC_CHANNEL(pi->radio_chanspec);

	if (ISHTPHY(pi)) {
		cache = &ctx->u.htphy_cache;
	} else
		return;

	BCM_REFERENCE(cache);
	FOREACH_CORE(pi, i) {
		PHY_CAL(("CORE %d:\n", i));
		PHY_CAL(("\tofdm_txa:0x%x  ofdm_txb:0x%x  ofdm_txd:0x%x\n",
		            cache->ofdm_txa[i], cache->ofdm_txb[i], cache->ofdm_txd[i]));
		PHY_CAL(("\tbphy_txa:0x%x  bphy_txb:0x%x  bphy_txd:0x%x\n",
		            cache->bphy_txa[i], cache->bphy_txb[i], cache->bphy_txd[i]));
		PHY_CAL(("\ttxei:0x%x  txeq:0x%x\n", cache->txei[i], cache->txeq[i]));
		PHY_CAL(("\ttxfi:0x%x  txfq:0x%x\n", cache->txfi[i], cache->txfq[i]));
		PHY_CAL(("\trxa:0x%x  rxb:0x%x\n", cache->rxa[i], cache->rxb[i]));
		if (pi->itssical || (pi->itssi_cap_low5g && chan <= 48 && chan >= 36)) {
			PHY_CAL(("\tidletssi:0x%x\n", cache->idle_tssi[i]));
			PHY_CAL(("\tvmid:0x%x\n", cache->Vmid[i]));
		}
	}
}

#endif /* PHYCAL_CACHING */
