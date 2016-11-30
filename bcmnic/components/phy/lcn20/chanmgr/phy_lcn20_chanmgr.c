/*
 * lcn20PHY Channel Manager module implementation
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
 * $Id: phy_lcn20_chanmgr.c 610412 2016-01-06 23:43:14Z vyass $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_chanmgr.h>
#include "phy_type_chanmgr.h"
#include <phy_lcn20.h>
#include <phy_lcn20_chanmgr.h>
#include <phy_utils_channel.h>
#include <bcmwifi_channels.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_lcn20.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_lcn20_chanmgr_info {
	phy_info_t *pi;
	phy_lcn20_info_t *lcn20i;
	phy_chanmgr_info_t *chanmgri;
};

/* local functions */
static int wlc_phy_chanspec_freq2bandrange_lcnphy20(phy_type_chanmgr_ctx_t *ctx, chanspec_t);
static void phy_lcn20_chanmgr_chanspec_set(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec);

/* Register/unregister lcn20PHY specific implementation to common layer */
phy_lcn20_chanmgr_info_t *
BCMATTACHFN(phy_lcn20_chanmgr_register_impl)(phy_info_t *pi, phy_lcn20_info_t *lcn20i,
	phy_chanmgr_info_t *chanmgri)
{
	phy_lcn20_chanmgr_info_t *info;
	phy_type_chanmgr_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_lcn20_chanmgr_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_lcn20_chanmgr_info_t));
	info->pi = pi;
	info->lcn20i = lcn20i;
	info->chanmgri = chanmgri;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.get_bandrange = wlc_phy_chanspec_freq2bandrange_lcnphy20;
	fns.chanspec_set = phy_lcn20_chanmgr_chanspec_set;
	fns.ctx = info;

	if (phy_chanmgr_register_impl(chanmgri, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_chanmgr_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return info;
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_lcn20_chanmgr_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn20_chanmgr_unregister_impl)(phy_lcn20_chanmgr_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_chanmgr_info_t *chanmgri = info->chanmgri;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_chanmgr_unregister_impl(chanmgri);

	phy_mfree(pi, info, sizeof(phy_lcn20_chanmgr_info_t));
}

static int
wlc_phy_chanspec_freq2bandrange_lcnphy20(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec)
{
	int range = -1;
	uint channel = CHSPEC_CHANNEL(chanspec);
	uint freq = phy_utils_channel2freq(channel);
	if (freq < 2500)
		range = WL_CHAN_FREQ_RANGE_2G;
#ifdef BAND5G
	else if (freq <= 5320)
		range = WL_CHAN_FREQ_RANGE_5GL;
	else if (freq <= 5700)
		range = WL_CHAN_FREQ_RANGE_5GM;
	else
		range = WL_CHAN_FREQ_RANGE_5GH;
#endif /* BAND5G */

	return range;
}

static void
phy_lcn20_chanmgr_chanspec_set(phy_type_chanmgr_ctx_t *ctx, chanspec_t chanspec)
{
	phy_lcn20_chanmgr_info_t *info = (phy_lcn20_chanmgr_info_t *)ctx;
	phy_info_t *pi = info->pi;
	wlc_phy_chanspec_set_lcn20phy(pi, chanspec);
}
