/*
 * HTPHY BlueToothCoExistence module implementation
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
 * $Id: $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_btcx.h>
#include "phy_type_btcx.h"
#include <phy_ht_info.h>
#include <phy_ht_btcx.h>
#include <wlc_phy_int.h>

/* module private states */
struct phy_ht_btcx_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_btcx_info_t *btcx_cmn;
};

/* local functions */
static int phy_ht_btcx_set_restage_rxgain(phy_type_btcx_ctx_t *ctx, int32 set_val);
static int phy_ht_btcx_get_restage_rxgain(phy_type_btcx_ctx_t *ctx, int32 *ret_val);

/* Register/unregister HTPHY specific implementation to common layer. */
phy_ht_btcx_info_t *
BCMATTACHFN(phy_ht_btcx_register_impl)(phy_info_t *pi, phy_ht_info_t *hti,
	phy_btcx_info_t *btcx_cmn)
{
	phy_ht_btcx_info_t *btcxi;
	phy_type_btcx_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((btcxi = phy_malloc(pi, sizeof(phy_ht_btcx_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(btcxi, sizeof(phy_ht_btcx_info_t));
	btcxi->pi = pi;
	btcxi->hti = hti;
	btcxi->btcx_cmn = btcx_cmn;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.set_restage_rxgain = phy_ht_btcx_set_restage_rxgain;
	fns.get_restage_rxgain = phy_ht_btcx_get_restage_rxgain;
	fns.ctx = btcxi;

	phy_btcx_register_impl(btcx_cmn, &fns);

	return btcxi;
fail:
	if (btcxi != NULL)
		phy_mfree(pi, btcxi, sizeof(phy_ht_btcx_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_btcx_unregister_impl)(phy_ht_btcx_info_t *btcxi)
{
	phy_info_t *pi = btcxi->pi;
	phy_btcx_info_t *btcx_cmn = btcxi->btcx_cmn;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_btcx_unregister_impl(btcx_cmn);

	phy_mfree(pi, btcxi, sizeof(phy_ht_btcx_info_t));
}

static int
phy_ht_btcx_set_restage_rxgain(phy_type_btcx_ctx_t *ctx, int32 set_val)
{
	phy_ht_btcx_info_t *btcxi = (phy_ht_btcx_info_t *)ctx;
	phy_info_t *pi = btcxi->pi;
	BCM_REFERENCE(pi);

	if (IS_X29B_BOARDTYPE(pi) || IS_X29D_BOARDTYPE(pi) || IS_X33_BOARDTYPE(pi)) {
		if ((set_val < 0) || (set_val > 1)) {
			return BCME_RANGE;
		}
		if (SCAN_RM_IN_PROGRESS(pi)) {
			return BCME_NOTREADY;
		}
		if (IS_X29B_BOARDTYPE(pi)) {
			if ((pi->sh->interference_mode != INTERFERE_NONE) &&
			     (pi->sh->interference_mode != NON_WLAN)) {
				return BCME_UNSUPPORTED;
			}
		}
		if ((IS_X29D_BOARDTYPE(pi) || IS_X33_BOARDTYPE(pi)) &&
		    !CHSPEC_IS2G(pi->radio_chanspec)) {
			return BCME_BADBAND;
		}
		if (((set_val == 0) && btcxi->hti->btc_restage_rxgain) ||
		    ((set_val == 1) && !btcxi->hti->btc_restage_rxgain)) {
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
			wlc_phy_btc_restage_rxgain_htphy(pi, (bool)set_val);
			wlapi_enable_mac(pi->sh->physhim);
		}
		return BCME_OK;
	} else {
		return BCME_UNSUPPORTED;
	}
}

static int
phy_ht_btcx_get_restage_rxgain(phy_type_btcx_ctx_t *ctx, int32 *ret_val)
{
	phy_ht_btcx_info_t *btcxi = (phy_ht_btcx_info_t *)ctx;
	phy_info_t *pi = btcxi->pi;
	BCM_REFERENCE(pi);

	if (IS_X29B_BOARDTYPE(pi) || IS_X29D_BOARDTYPE(pi) || IS_X33_BOARDTYPE(pi)) {
		if ((IS_X29D_BOARDTYPE(pi) || IS_X33_BOARDTYPE(pi)) &&
			!CHSPEC_IS2G(pi->radio_chanspec)) {
			return BCME_BADBAND;
		}
		*ret_val = (int32)btcxi->hti->btc_restage_rxgain;
		return BCME_OK;
	} else {
		return BCME_UNSUPPORTED;
	}
}
