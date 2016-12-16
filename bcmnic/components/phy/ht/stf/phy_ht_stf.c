/*
 * HTPHY STF modules implementation
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
 * $Id: phy_ht_stf.c 656697 2016-08-29 19:19:37Z $
 */
#include <typedefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_btcx.h>
#include <phy_stf.h>
#include <phy_type_stf.h>
#include <phy_ht_stf.h>
#include <phy_ht_rxiqcal.h>
#include <wlc_phy_int.h>
#include <wlc_phyreg_ht.h>
#include <phy_utils_reg.h>
#include <phy_stf.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_ht.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_ht_stf_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_stf_info_t *stf_info;
};

/* Functions used by common layer as callbacks */
static int phy_ht_stf_set_stf_chain(phy_type_stf_ctx_t *ctx,
		uint8 txchain, uint8 rxchain);

/* register phy type specific implementation */
phy_ht_stf_info_t *
BCMATTACHFN(phy_ht_stf_register_impl)(phy_info_t *pi, phy_ht_info_t *hti,
	phy_stf_info_t *stf_info)
{
	phy_ht_stf_info_t *ht_info;
	phy_type_stf_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ht_info = phy_malloc(pi, sizeof(phy_ht_stf_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	ht_info->pi = pi;
	ht_info->hti = hti;
	ht_info->stf_info = stf_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.set_stf_chain = phy_ht_stf_set_stf_chain;
	fns.chain_init = NULL;
	fns.ctx = ht_info;

	if (phy_stf_register_impl(stf_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_stf_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return ht_info;

	/* error handling */
fail:
	if (ht_info != NULL)
		phy_mfree(pi, ht_info, sizeof(phy_ht_stf_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_stf_unregister_impl)(phy_ht_stf_info_t *ht_info)
{
	phy_info_t *pi;
	phy_stf_info_t *stf_info;

	ASSERT(ht_info);
	pi = ht_info->pi;
	stf_info = ht_info->stf_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_stf_unregister_impl(stf_info);

	phy_mfree(pi, ht_info, sizeof(phy_ht_stf_info_t));
}

/* ********************************************** */
/* Function table registred function */
/* ********************************************** */
static int
phy_ht_stf_set_stf_chain(phy_type_stf_ctx_t *ctx, uint8 txchain, uint8 rxchain)
{
	phy_ht_stf_info_t *stf_info = (phy_ht_stf_info_t *) ctx;
	phy_stf_info_t *cmn_stf_info = (phy_stf_info_t *) stf_info->stf_info;
	phy_info_t *pi = stf_info->pi;

	PHY_TRACE(("phy_ht_stf_set_stf_chain, new phy chain tx %d, rx %d", txchain, rxchain));

	phy_stf_set_phytxchain(cmn_stf_info, txchain);
	wlc_phy_rxcore_setstate_htphy((wlc_phy_t *)pi, rxchain);

	return BCME_OK;
}
