/*
 * HTPHY Channel Manager module implementation
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
 * $Id: phy_ht_chanmgr.c 610412 2016-01-06 23:43:14Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_chanmgr.h>
#include "phy_type_chanmgr.h"
#include <phy_ht.h>
#include <phy_ht_chanmgr.h>
#include <phy_ht_radio.h>
#include <phy_utils_reg.h>
#include <bcmwifi_channels.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_ht_chanmgr_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_chanmgr_info_t *ci;
};

/* local functions */
static int phy_ht_chanmgr_get_chanspec_bandrange(phy_type_chanmgr_ctx_t *ctx, chanspec_t);
static void phy_ht_chanmgr_chanspec_set(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec);

/* register phy type specific implementation */
phy_ht_chanmgr_info_t *
BCMATTACHFN(phy_ht_chanmgr_register_impl)(phy_info_t *pi, phy_ht_info_t *hti,
	phy_chanmgr_info_t *ci)
{
	phy_ht_chanmgr_info_t *info;
	phy_type_chanmgr_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((info = phy_malloc(pi, sizeof(phy_ht_chanmgr_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_ht_chanmgr_info_t));
	info->pi = pi;
	info->hti = hti;
	info->ci = ci;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.get_bandrange = phy_ht_chanmgr_get_chanspec_bandrange;
	fns.chanspec_set = phy_ht_chanmgr_chanspec_set;
	fns.ctx = info;

	if (phy_chanmgr_register_impl(ci, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_chanmgr_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return info;

	/* error handling */
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_ht_chanmgr_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_chanmgr_unregister_impl)(phy_ht_chanmgr_info_t *info)
{
	phy_info_t *pi;

	ASSERT(info);
	pi = info->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_chanmgr_unregister_impl(info->ci);

	phy_mfree(pi, info, sizeof(phy_ht_chanmgr_info_t));
}

static int
phy_ht_chanmgr_get_chanspec_bandrange(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec)
{
	phy_ht_chanmgr_info_t *info = (phy_ht_chanmgr_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint channel = CHSPEC_CHANNEL(chanspec);
	return wlc_phy_get_chan_freq_range_htphy(pi, channel);
}

static void
phy_ht_chanmgr_chanspec_set(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec)
{
	phy_ht_chanmgr_info_t *info = (phy_ht_chanmgr_info_t *)ctx;
	phy_info_t *pi = info->pi;
	wlc_phy_chanspec_set_htphy(pi, chanspec);
}
