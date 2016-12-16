/*
 * NPHY STF modules implementation
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
 * $Id: phy_n_stf.c 656697 2016-08-29 19:19:37Z $
 */
#include <typedefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_btcx.h>
#include <phy_stf.h>
#include <phy_type_stf.h>
#include <phy_n_stf.h>
#include <phy_n_rxiqcal.h>
#include <wlc_phy_int.h>
#include <wlc_phyreg_n.h>
#include <phy_utils_reg.h>
#include <phy_stf.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_n.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_n_stf_info {
	phy_info_t *pi;
	phy_n_info_t *ni;
	phy_stf_info_t *stf_info;
};

/* Functions used by common layer as callbacks */
static int phy_n_stf_set_stf_chain(phy_type_stf_ctx_t *ctx,
		uint8 txchain, uint8 rxchain);

/* register phy type specific implementation */
phy_n_stf_info_t *
BCMATTACHFN(phy_n_stf_register_impl)(phy_info_t *pi, phy_n_info_t *ni,
	phy_stf_info_t *stf_info)
{
	phy_n_stf_info_t *n_stf_info;
	phy_type_stf_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((n_stf_info = phy_malloc(pi, sizeof(phy_n_stf_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	n_stf_info->pi = pi;
	n_stf_info->ni = ni;
	n_stf_info->stf_info = stf_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.set_stf_chain = phy_n_stf_set_stf_chain;
	fns.chain_init = NULL;
	fns.ctx = n_stf_info;

	if (phy_stf_register_impl(stf_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_stf_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return n_stf_info;

	/* error handling */
fail:
	if (n_stf_info != NULL)
		phy_mfree(pi, n_stf_info, sizeof(phy_n_stf_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_n_stf_unregister_impl)(phy_n_stf_info_t *n_stf_info)
{
	phy_info_t *pi = n_stf_info->pi;
	phy_stf_info_t *stf_info = n_stf_info->stf_info;
	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_stf_unregister_impl(stf_info);
	phy_mfree(pi, n_stf_info, sizeof(phy_n_stf_info_t));
}

/* ********************************************** */
/* Function table registered function                                  */
/* ********************************************** */
static int
phy_n_stf_set_stf_chain(phy_type_stf_ctx_t *ctx, uint8 txchain, uint8 rxchain)
{
	phy_n_stf_info_t *stf_info = (phy_n_stf_info_t *)ctx;
	phy_stf_info_t *cmn_stf_info = (phy_stf_info_t *) stf_info->stf_info;
	phy_info_t *pi = stf_info->pi;

	PHY_TRACE(("phy_n_stf_set_stf_chain, new phy chain tx %d, rx %d", txchain, rxchain));

	phy_stf_set_phytxchain(cmn_stf_info, txchain);

	if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV)) {
		wlc_phy_rxcore_setstate_nphy((wlc_phy_t *)pi, rxchain, 1);
	} else {
		wlc_phy_rxcore_setstate_nphy((wlc_phy_t *)pi, rxchain, 0);
	}

	return BCME_OK;
}
