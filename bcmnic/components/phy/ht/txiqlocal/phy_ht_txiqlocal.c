/*
 * HT PHY TXIQLO CAL module implementation
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
 * $Id: phy_ht_txiqlocal.c 625575 2016-03-17 00:46:03Z vyass $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_type_txiqlocal.h>
#include <phy_ht.h>
#include <phy_ht_txiqlocal.h>
#include <phy_utils_reg.h>
#include <wlc_phyreg_ht.h>
#include <wlc_phy_radio.h>
#include <phy_ht_info.h>
/* module private states */
struct phy_ht_txiqlocal_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_txiqlocal_info_t *cmn_info;
	/* cache coeffs */
	txcal_coeffs_t txcal_cache[PHY_CORE_MAX];
/* add other variable size variables here at the end */
};

#if !defined(PHYCAL_CACHING) || !defined(WLMCHAN)
static void wlc_phy_scanroam_cache_txcal_htphy(phy_type_txiqlocal_ctx_t *ctx, bool set);
#endif
/* register phy type specific implementation */
phy_ht_txiqlocal_info_t *
BCMATTACHFN(phy_ht_txiqlocal_register_impl)(phy_info_t *pi, phy_ht_info_t *hti,
	phy_txiqlocal_info_t *cmn_info)
{
	phy_ht_txiqlocal_info_t *ht_info;
	phy_type_txiqlocal_fns_t fns;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ht_info = phy_malloc(pi, sizeof(phy_ht_txiqlocal_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize params */
	bzero(ht_info, sizeof(phy_ht_txiqlocal_info_t));
	ht_info->pi = pi;
	ht_info->hti = hti;
	ht_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = ht_info;
#if !defined(PHYCAL_CACHING) || !defined(WLMCHAN)
	fns.scanroam_cache = wlc_phy_scanroam_cache_txcal_htphy;
#endif
	if (phy_txiqlocal_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_txiqlocal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return ht_info;

	/* error handling */
fail:
	if (ht_info != NULL)
		phy_mfree(pi, ht_info, sizeof(phy_ht_txiqlocal_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_txiqlocal_unregister_impl)(phy_ht_txiqlocal_info_t *ht_info)
{
	phy_txiqlocal_info_t *cmn_info = ht_info->cmn_info;
	phy_info_t *pi = ht_info->pi;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_txiqlocal_unregister_impl(cmn_info);

	phy_mfree(pi, ht_info, sizeof(phy_ht_txiqlocal_info_t));
}

/* Dont need scanroam caching if PHYCAL Caching is enabled */
#if !defined(PHYCAL_CACHING) || !defined(WLMCHAN)
static void
wlc_phy_scanroam_cache_txcal_htphy(phy_type_txiqlocal_ctx_t *ctx, bool set)
{
	phy_ht_txiqlocal_info_t *info = (phy_ht_txiqlocal_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint16 tbl_cookie;
	uint16 ab_int[2];
	uint8 core;
	bool suspend;

	PHY_TRACE(("wl%d: %s: in scan/roam set %d\n", pi->sh->unit, __FUNCTION__, set));

	suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);
	if (!suspend) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
	}

	wlc_phy_table_read_htphy(pi, HTPHY_TBL_ID_IQLOCAL, 1,
		IQTBL_CACHE_COOKIE_OFFSET, 16, &tbl_cookie);

	if (set) {
		if (tbl_cookie == TXCAL_CACHE_VALID) {
			PHY_CAL(("wl%d: %s: save the txcal for scan/roam\n",
				pi->sh->unit, __FUNCTION__));
			/* save the txcal to cache */
			FOREACH_CORE(pi, core) {
				wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_READ,
					ab_int, TB_OFDM_COEFFS_AB, core);
				info->txcal_cache[core].txa = ab_int[0];
				info->txcal_cache[core].txb = ab_int[1];
				wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_READ,
					&info->txcal_cache[core].txd,
					TB_OFDM_COEFFS_D, core);
				info->txcal_cache[core].txei =
				        (uint8)phy_utils_read_radioreg(pi,
					RADIO_2059_TX_LOFT_FINE_I(core));
				info->txcal_cache[core].txeq =
				        (uint8)phy_utils_read_radioreg(pi,
					RADIO_2059_TX_LOFT_FINE_Q(core));
				info->txcal_cache[core].txfi =
				        (uint8)phy_utils_read_radioreg(pi,
					RADIO_2059_TX_LOFT_COARSE_I(core));
				info->txcal_cache[core].txfq =
				        (uint8)phy_utils_read_radioreg(pi,
					RADIO_2059_TX_LOFT_COARSE_Q(core));
			}
			/* mark the cache as valid */
			pi->u.pi_htphy->txcal_cache_cookie = tbl_cookie;
		}
	} else {
		if (pi->u.pi_htphy->txcal_cache_cookie == TXCAL_CACHE_VALID &&
		   tbl_cookie != pi->u.pi_htphy->txcal_cache_cookie) {
			PHY_CAL(("wl%d: %s: restore the txcal after scan/roam\n",
				pi->sh->unit, __FUNCTION__));
			/* restore the txcal from cache */
			FOREACH_CORE(pi, core) {
				ab_int[0] = info->txcal_cache[core].txa;
				ab_int[1] = info->txcal_cache[core].txb;
				wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_WRITE,
					ab_int, TB_OFDM_COEFFS_AB, core);
				wlc_phy_cal_txiqlo_coeffs_htphy(pi, CAL_COEFF_WRITE,
					&info->txcal_cache[core].txd,
					TB_OFDM_COEFFS_D, core);
				phy_utils_write_radioreg(pi, RADIO_2059_TX_LOFT_FINE_I(core),
					info->txcal_cache[core].txei);
				phy_utils_write_radioreg(pi, RADIO_2059_TX_LOFT_FINE_Q(core),
					info->txcal_cache[core].txeq);
				phy_utils_write_radioreg(pi, RADIO_2059_TX_LOFT_COARSE_I(core),
					info->txcal_cache[core].txfi);
				phy_utils_write_radioreg(pi, RADIO_2059_TX_LOFT_COARSE_Q(core),
					info->txcal_cache[core].txfq);
				wlc_phy_table_write_htphy(pi, HTPHY_TBL_ID_IQLOCAL, 1,
					IQTBL_CACHE_COOKIE_OFFSET, 16,
					&pi->u.pi_htphy->txcal_cache_cookie);
			}
			pi->u.pi_htphy->txcal_cache_cookie = 0;
		}
	}

	if (!suspend) {
		wlapi_enable_mac(pi->sh->physhim);
	}
}
#endif /* !defined(PHYCAL_CACHING) || !defined(WLMCHAN) */
