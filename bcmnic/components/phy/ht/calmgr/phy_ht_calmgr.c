/*
 * HTPHY Calibration Manager module implementation
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
 * $Id: phy_ht_calmgr.c 657373 2016-09-01 01:08:38Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_calmgr.h"
#include <phy_ht.h>
#include <phy_ht_calmgr.h>
#include <phy_utils_reg.h>

/* module private states */
struct phy_ht_calmgr_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_calmgr_info_t *ci;
};

/* local functions */
static int phy_ht_calmgr_init(phy_type_calmgr_ctx_t *ctx);
static int phy_ht_calmgr_prepare(phy_type_calmgr_ctx_t *ctx);
static void phy_ht_calmgr_cleanup(phy_type_calmgr_ctx_t *ctx);
static void phy_ht_calmgr_cals(phy_type_calmgr_ctx_t *ctx, uint8 legacy_caltype, uint8 searchmode);

/* register phy type specific implementation */
phy_ht_calmgr_info_t *
BCMATTACHFN(phy_ht_calmgr_register_impl)(phy_info_t *pi, phy_ht_info_t *hti,
	phy_calmgr_info_t *ci)
{
	phy_ht_calmgr_info_t *info;
	phy_type_calmgr_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((info = phy_malloc(pi, sizeof(phy_ht_calmgr_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_ht_calmgr_info_t));
	info->pi = pi;
	info->hti = hti;
	info->ci = ci;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.init = phy_ht_calmgr_init;
	fns.prepare = phy_ht_calmgr_prepare;
	fns.cleanup = phy_ht_calmgr_cleanup;
	fns.cals = phy_ht_calmgr_cals;
	fns.ctx = info;

	if (phy_calmgr_register_impl(ci, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_calmgr_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return info;

	/* error handling */
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_ht_calmgr_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_calmgr_unregister_impl)(phy_ht_calmgr_info_t *info)
{
	phy_info_t *pi;

	ASSERT(info);
	pi = info->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_calmgr_unregister_impl(info->ci);

	phy_mfree(pi, info, sizeof(phy_ht_calmgr_info_t));
}

static int
phy_ht_calmgr_init(phy_type_calmgr_ctx_t *ctx)
{
	phy_ht_calmgr_info_t *calmgri = (phy_ht_calmgr_info_t *) ctx;
	phy_info_t * pi = calmgri->pi;
	pi->interf->aci.ma_total = PHY_NOISE_MA_WINDOW_SZ * ACI_INIT_MA;
	pi->interf->badplcp_ma_total = PHY_NOISE_GLITCH_INIT_MA_BADPlCP *
		PHY_NOISE_MA_WINDOW_SZ;
	wlc_phy_aci_init_htphy(pi);
	wlc_phy_aci_sw_reset_htphy(pi);
	return BCME_OK;
}

static int
phy_ht_calmgr_prepare(phy_type_calmgr_ctx_t *ctx)
{
	PHY_TRACE(("%s\n", __FUNCTION__));

	return BCME_OK;
}

static void
phy_ht_calmgr_cleanup(phy_type_calmgr_ctx_t *ctx)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

static void
phy_ht_calmgr_cals(phy_type_calmgr_ctx_t *ctx, uint8 legacy_caltype, uint8 searchmode)
{
	phy_ht_calmgr_info_t *info = (phy_ht_calmgr_info_t *)ctx;
	phy_info_t *pi = info->pi;
	wlc_phy_cals_htphy(pi, searchmode);
}
