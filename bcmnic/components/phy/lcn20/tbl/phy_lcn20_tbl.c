/*
 * lcn20PHY PHYTableInit module implementation
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
 * $Id: phy_lcn20_tbl.c 583048 2015-08-31 16:43:34Z jqliu $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_tbl.h"
#include <phy_lcn20.h>
#include <phy_lcn20_tbl.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_lcn20.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_lcn20_tbl_info {
	phy_info_t *pi;
	phy_lcn20_info_t *lcn20i;
	phy_tbl_info_t *ii;
};

/* local functions */
static int phy_lcn20_tbl_init(phy_type_tbl_ctx_t *ctx);

/* Register/unregister lcn20PHY specific implementation to common layer */
phy_lcn20_tbl_info_t *
BCMATTACHFN(phy_lcn20_tbl_register_impl)(phy_info_t *pi, phy_lcn20_info_t *lcn20i,
	phy_tbl_info_t *ii)
{
	phy_lcn20_tbl_info_t *info;
	phy_type_tbl_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_lcn20_tbl_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_lcn20_tbl_info_t));
	info->pi = pi;
	info->lcn20i = lcn20i;
	info->ii = ii;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.init = phy_lcn20_tbl_init;
	fns.ctx = info;

	phy_tbl_register_impl(ii, &fns);

	return info;
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_lcn20_tbl_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn20_tbl_unregister_impl)(phy_lcn20_tbl_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_tbl_info_t *ii = info->ii;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_tbl_unregister_impl(ii);

	phy_mfree(pi, info, sizeof(phy_lcn20_tbl_info_t));
}

/* h/w init/down */
static int
WLBANDINITFN(phy_lcn20_tbl_init)(phy_type_tbl_ctx_t *ctx)
{
	phy_lcn20_tbl_info_t *lcn20ii = (phy_lcn20_tbl_info_t *)ctx;
	phy_info_t *pi = lcn20ii->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	wlc_phy_init_lcn20phy(pi);

	return BCME_OK;
}
