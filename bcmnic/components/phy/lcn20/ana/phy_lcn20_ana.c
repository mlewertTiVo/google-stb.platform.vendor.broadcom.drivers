/*
 * lcn20PHY ANAcore contorl module implementation
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
 * $Id: phy_lcn20_ana.c 583048 2015-08-31 16:43:34Z jqliu $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_ana.h>
#include "phy_type_ana.h"
#include <phy_lcn20.h>
#include <phy_lcn20_ana.h>

#include <wlc_phyreg_lcn20.h>
#include <phy_utils_reg.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_lcn20.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_lcn20_ana_info {
	phy_info_t *pi;
	phy_lcn20_info_t *lcn20i;
	phy_ana_info_t *ani;
};

/* local functions */
static int phy_lcn20_ana_switch(phy_type_ana_ctx_t *ctx, bool on);

/* Register/unregister lcn20PHY specific implementation to common layer */
phy_lcn20_ana_info_t *
BCMATTACHFN(phy_lcn20_ana_register_impl)(phy_info_t *pi, phy_lcn20_info_t *lcn20i,
	phy_ana_info_t *ani)
{
	phy_lcn20_ana_info_t *info;
	phy_type_ana_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_lcn20_ana_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_lcn20_ana_info_t));
	info->pi = pi;
	info->lcn20i = lcn20i;
	info->ani = ani;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctrl = phy_lcn20_ana_switch;
	fns.ctx = info;

	phy_ana_register_impl(ani, &fns);

	return info;
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_lcn20_ana_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn20_ana_unregister_impl)(phy_lcn20_ana_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_ana_info_t *ani = info->ani;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_ana_unregister_impl(ani);

	phy_mfree(pi, info, sizeof(phy_lcn20_ana_info_t));
}

/* switch anacore on/off */
static int
phy_lcn20_ana_switch(phy_type_ana_ctx_t *ctx, bool on)
{
	phy_lcn20_ana_info_t *info = (phy_lcn20_ana_info_t *)ctx;
	phy_info_t *pi = info->pi;

	wlc_lcn20phy_anacore(pi, on);

	return BCME_OK;
}
