/*
 * HT PHY RXIQ CAL module implementation
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
 * $Id: phy_ht_rxiqcal.c 639978 2016-05-25 16:03:11Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_type_rxiqcal.h>
#include <phy_ht.h>
#include <phy_ht_rxiqcal.h>
#include <phy_utils_reg.h>
#include <wlc_phyreg_ht.h>
#include <phy_ht_info.h>
/* module private states */
struct phy_ht_rxiqcal_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_rxiqcal_info_t *cmn_info;
	/* cache coeffs */
	rxcal_coeffs_t rxcal_cache[PHY_CORE_MAX];
/* add other variable size variables here at the end */
};

#if !defined(PHYCAL_CACHING) || !defined(WLMCHAN)
static void wlc_phy_scanroam_cache_rxcal_htphy(phy_type_rxiqcal_ctx_t *ctx, bool set);
#endif
/* register phy type specific implementation */
phy_ht_rxiqcal_info_t *
BCMATTACHFN(phy_ht_rxiqcal_register_impl)(phy_info_t *pi, phy_ht_info_t *hti,
	phy_rxiqcal_info_t *cmn_info)
{
	phy_ht_rxiqcal_info_t *ht_info;
	phy_type_rxiqcal_fns_t fns;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ht_info = phy_malloc(pi, sizeof(phy_ht_rxiqcal_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize params */
	bzero(ht_info, sizeof(phy_ht_rxiqcal_info_t));
	ht_info->pi = pi;
	ht_info->hti = hti;
	ht_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = ht_info;
#if !defined(PHYCAL_CACHING) || !defined(WLMCHAN)
	fns.scanroam_cache = wlc_phy_scanroam_cache_rxcal_htphy;
#endif
	if (phy_rxiqcal_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_rxiqcal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return ht_info;

	/* error handling */
fail:
	if (ht_info != NULL)
		phy_mfree(pi, ht_info, sizeof(phy_ht_rxiqcal_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_rxiqcal_unregister_impl)(phy_ht_rxiqcal_info_t *ht_info)
{
	phy_rxiqcal_info_t *cmn_info = ht_info->cmn_info;
	phy_info_t *pi = ht_info->pi;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_rxiqcal_unregister_impl(cmn_info);

	phy_mfree(pi, ht_info, sizeof(phy_ht_rxiqcal_info_t));
}

#if !defined(PHYCAL_CACHING) || !defined(WLMCHAN)
static void
wlc_phy_scanroam_cache_rxcal_htphy(phy_type_rxiqcal_ctx_t *ctx, bool set)
{
	phy_ht_rxiqcal_info_t *info = (phy_ht_rxiqcal_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint16 tbl_cookie;
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
			PHY_CAL(("wl%d: %s: save the rxcal for scan/roam\n",
				pi->sh->unit, __FUNCTION__));
			/* save the txcal to cache */
			FOREACH_CORE(pi, core) {
				info->rxcal_cache[core].rxa =
					phy_utils_read_phyreg(pi, HTPHY_RxIQCompA(core));
				info->rxcal_cache[core].rxb =
					phy_utils_read_phyreg(pi, HTPHY_RxIQCompB(core));
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
				phy_utils_write_phyreg(pi, HTPHY_RxIQCompA(core),
					info->rxcal_cache[core].rxa);
				phy_utils_write_phyreg(pi, HTPHY_RxIQCompB(core),
					info->rxcal_cache[core].rxb);
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

/* Override/Restore routine for Rx Digital LPF:
 * 1) Override: Save digital LPF config and set new LPF configuration
 * 2) Restore: Restore digital LPF config
 */
void
wlc_phy_dig_lpf_override_htphy(phy_info_t *pi, uint8 dig_lpf_ht)
{
	phy_rxiqcal_data_t *data = pi->rxiqcali->data;
	if ((dig_lpf_ht > 0) && !data->phy_rx_diglpf_default_coeffs_valid) {

		data->phy_rx_diglpf_default_coeffs[0] =
		        phy_utils_read_phyreg(pi, HTPHY_RxStrnFilt40Num00);
		data->phy_rx_diglpf_default_coeffs[1] =
		        phy_utils_read_phyreg(pi, HTPHY_RxStrnFilt40Num01);
		data->phy_rx_diglpf_default_coeffs[2] =
		        phy_utils_read_phyreg(pi, HTPHY_RxStrnFilt40Num02);
		data->phy_rx_diglpf_default_coeffs[3] =
		        phy_utils_read_phyreg(pi, HTPHY_RxStrnFilt40Den00);
		data->phy_rx_diglpf_default_coeffs[4] =
		        phy_utils_read_phyreg(pi, HTPHY_RxStrnFilt40Den01);
		data->phy_rx_diglpf_default_coeffs[5] =
		        phy_utils_read_phyreg(pi, HTPHY_RxStrnFilt40Num10);
		data->phy_rx_diglpf_default_coeffs[6] =
		        phy_utils_read_phyreg(pi, HTPHY_RxStrnFilt40Num11);
		data->phy_rx_diglpf_default_coeffs[7] =
		        phy_utils_read_phyreg(pi, HTPHY_RxStrnFilt40Num12);
		data->phy_rx_diglpf_default_coeffs[8] =
		        phy_utils_read_phyreg(pi, HTPHY_RxStrnFilt40Den10);
		data->phy_rx_diglpf_default_coeffs[9] =
		        phy_utils_read_phyreg(pi, HTPHY_RxStrnFilt40Den11);
		data->phy_rx_diglpf_default_coeffs_valid = TRUE;

	}

	switch (dig_lpf_ht) {
	case 0:  /* restore rx dig lpf */

		/* ASSERT(pi->phy_rx_diglpf_default_coeffs_valid); */
		if (!data->phy_rx_diglpf_default_coeffs_valid) {
			break;
		}
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num00,
		                       data->phy_rx_diglpf_default_coeffs[0]);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num01,
		                       data->phy_rx_diglpf_default_coeffs[1]);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num02,
		                       data->phy_rx_diglpf_default_coeffs[2]);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den00,
		                       data->phy_rx_diglpf_default_coeffs[3]);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den01,
		                       data->phy_rx_diglpf_default_coeffs[4]);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num10,
		                       data->phy_rx_diglpf_default_coeffs[5]);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num11,
		                       data->phy_rx_diglpf_default_coeffs[6]);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num12,
		                       data->phy_rx_diglpf_default_coeffs[7]);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den10,
		                       data->phy_rx_diglpf_default_coeffs[8]);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den11,
		                       data->phy_rx_diglpf_default_coeffs[9]);

		data->phy_rx_diglpf_default_coeffs_valid = FALSE;
		break;
	case 1:  /* set rx dig lpf to ltrn-lpf mode */

		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num00,
		                       phy_utils_read_phyreg(pi, HTPHY_RxFilt40Num00));
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num01,
		                       phy_utils_read_phyreg(pi, HTPHY_RxFilt40Num01));
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num02,
		                       phy_utils_read_phyreg(pi, HTPHY_RxFilt40Num02));
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num10,
		                       phy_utils_read_phyreg(pi, HTPHY_RxFilt40Num10));
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num11,
		                       phy_utils_read_phyreg(pi, HTPHY_RxFilt40Num11));
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num12,
		                       phy_utils_read_phyreg(pi, HTPHY_RxFilt40Num12));
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den00,
		                       phy_utils_read_phyreg(pi, HTPHY_RxFilt40Den00));
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den01,
		                       phy_utils_read_phyreg(pi, HTPHY_RxFilt40Den01));
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den10,
		                       phy_utils_read_phyreg(pi, HTPHY_RxFilt40Den10));
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den11,
		                       phy_utils_read_phyreg(pi, HTPHY_RxFilt40Den11));

		break;
	case 2:  /* bypass rx dig lpf */
		/* 0x2d4 = sqrt(2) * 512 */
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num00, 0x2d4);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num01, 0);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num02, 0);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den00, 0);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den01, 0);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num10, 0x2d4);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num11, 0);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Num12, 0);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den10, 0);
		phy_utils_write_phyreg(pi, HTPHY_RxStrnFilt40Den11, 0);

		break;

	default:
		ASSERT((dig_lpf_ht == 2) || (dig_lpf_ht == 1) || (dig_lpf_ht == 0));
		break;
	}
}
