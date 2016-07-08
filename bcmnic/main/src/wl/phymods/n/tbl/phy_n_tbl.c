/*
 * NPHY PHYTableInit module implementation
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_tbl.h"
#include <phy_n.h>
#include <phy_n_tbl.h>
#include <phy_utils_reg.h>

#include "wlc_phytbl_n.h"

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_n.h>
#include <wlc_phyreg_n.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_n_tbl_info {
	phy_info_t *pi;
	phy_n_info_t *ni;
	phy_tbl_info_t *ti;
};

/* local functions */
static int phy_n_tbl_init(phy_type_tbl_ctx_t *ctx);
#ifndef BCMNODOWN
static int phy_n_tbl_down(phy_type_tbl_ctx_t *ctx);
#else
#define phy_n_tbl_down NULL
#endif
#define phy_n_tbl_dump_tblfltr NULL
#define phy_n_tbl_dump_addrfltr NULL
#define phy_n_tbl_read_table NULL
#define phy_n_tbl_dump1 NULL
#define phy_n_tbl_dump2 NULL

/* Register/unregister NPHY specific implementation to common layer */
phy_n_tbl_info_t *
BCMATTACHFN(phy_n_tbl_register_impl)(phy_info_t *pi, phy_n_info_t *ni, phy_tbl_info_t *ti)
{
	phy_n_tbl_info_t *info;
	phy_type_tbl_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_n_tbl_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_n_tbl_info_t));
	info->pi = pi;
	info->ni = ni;
	info->ti = ti;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.init = phy_n_tbl_init;
	fns.down = phy_n_tbl_down;
	fns.ctx = info;

	phy_tbl_register_impl(ti, &fns);

	return info;
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_n_tbl_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_n_tbl_unregister_impl)(phy_n_tbl_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_tbl_info_t *ti = info->ti;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_tbl_unregister_impl(ti);

	phy_mfree(pi, info, sizeof(phy_n_tbl_info_t));
}

/* h/w init/down */
static int
WLBANDINITFN(phy_n_tbl_init)(phy_type_tbl_ctx_t *ctx)
{
	phy_n_tbl_info_t *ti = (phy_n_tbl_info_t *)ctx;
	phy_info_t *pi = ti->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	wlc_phy_init_nphy(pi);

	return BCME_OK;
}

#ifndef BCMNODOWN

/* down h/w */
static int
BCMUNINITFN(phy_n_tbl_down)(phy_type_tbl_ctx_t *ctx)
{
	phy_n_tbl_info_t *ti = (phy_n_tbl_info_t *)ctx;
	phy_n_info_t *ni = ti->ni;

	PHY_TRACE(("%s\n", __FUNCTION__));

	ni->nphy_iqcal_chanspec_2G = 0;
	ni->nphy_iqcal_chanspec_5G = 0;

	return 0;
}
#endif /* BCMNODOWN */
