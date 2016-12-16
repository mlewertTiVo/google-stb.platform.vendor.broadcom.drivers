/*
 * NPHY Rx Spur canceller module implementation
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
 * $Id: phy_n_rxspur.c 657044 2016-08-30 21:37:55Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_mem.h>
#include "phy_type_rxspur.h"
#include <phy_n.h>
#include <phy_n_rxspur.h>
#include <wlc_phy_n.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */

/* module private states */
struct phy_n_rxspur_info {
	phy_info_t *pi;
	phy_n_info_t *ni;
	phy_rxspur_info_t *cmn_info;
};

/* local functions */
static void phy_n_set_spurmode(phy_type_rxspur_ctx_t *ctx, uint16 freq);

/* register phy type specific implementation */
phy_n_rxspur_info_t *
BCMATTACHFN(phy_n_rxspur_register_impl)(phy_info_t *pi, phy_n_info_t *ni,
	phy_rxspur_info_t *cmn_info)
{
	phy_n_rxspur_info_t *n_info;
	phy_type_rxspur_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((n_info = phy_malloc(pi, sizeof(phy_n_rxspur_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	n_info->pi = pi;
	n_info->ni = ni;
	n_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.set_spurmode = phy_n_set_spurmode;
	fns.ctx = n_info;

	if (phy_rxspur_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_rxspur_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return n_info;

	/* error handling */
fail:
	if (n_info != NULL)
		phy_mfree(pi, n_info, sizeof(phy_n_rxspur_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_n_rxspur_unregister_impl)(phy_n_rxspur_info_t *n_info)
{
	phy_info_t *pi = n_info->pi;
	phy_rxspur_info_t *cmn_info = n_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_rxspur_unregister_impl(cmn_info);

	phy_mfree(pi, n_info, sizeof(phy_n_rxspur_info_t));
}

static void
phy_n_set_spurmode(phy_type_rxspur_ctx_t *ctx, uint16 freq)
{
	phy_n_rxspur_info_t *rxspuri = (phy_n_rxspur_info_t *) ctx;
	wlc_phy_set_spurmode_nphy(rxspuri->pi, freq);
}
