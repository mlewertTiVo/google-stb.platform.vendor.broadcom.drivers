/*
 * HTPHY PHYTableInit module implementation
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

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_tbl.h"
#include <phy_ht.h>
#include <phy_ht_tbl.h>
#include <phy_utils_reg.h>

#include "wlc_phytbl_ht.h"
#include "wlc_phyreg_ht.h"

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_ht_tbl_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_tbl_info_t *ti;
};

/* local functions */
static int phy_ht_tbl_init(phy_type_tbl_ctx_t *ctx);
#ifndef BCMNODOWN
static int phy_ht_tbl_down(phy_type_tbl_ctx_t *ctx);
#else
#define phy_ht_tbl_down NULL
#endif
#define phy_ht_tbl_read_table NULL
#define phy_ht_tbl_dump_addrfltr NULL
#define phy_ht_tbl_dump NULL

/* Register/unregister HTPHY specific implementation to common layer. */
phy_ht_tbl_info_t *
BCMATTACHFN(phy_ht_tbl_register_impl)(phy_info_t *pi, phy_ht_info_t *hti, phy_tbl_info_t *ti)
{
	phy_ht_tbl_info_t *info;
	phy_type_tbl_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_ht_tbl_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_ht_tbl_info_t));
	info->pi = pi;
	info->hti = hti;
	info->ti = ti;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.init = phy_ht_tbl_init;
	fns.down = phy_ht_tbl_down;
	fns.addrfltr = phy_ht_tbl_dump_addrfltr;
	fns.readtbl = phy_ht_tbl_read_table;
	fns.dump[0] = phy_ht_tbl_dump;
	fns.ctx = info;

	phy_tbl_register_impl(ti, &fns);

	return info;
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_ht_tbl_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_tbl_unregister_impl)(phy_ht_tbl_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_tbl_info_t *ti = info->ti;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_tbl_unregister_impl(ti);

	phy_mfree(pi, info, sizeof(phy_ht_tbl_info_t));
}

/* init h/w tables */
static int
WLBANDINITFN(phy_ht_tbl_init)(phy_type_tbl_ctx_t *ctx)
{
	phy_ht_tbl_info_t *ti = (phy_ht_tbl_info_t *)ctx;
	phy_info_t *pi = ti->pi;

	wlc_phy_init_htphy(pi);

	return BCME_OK;
}

#ifndef BCMNODOWN

/* down h/w */
static int
BCMUNINITFN(phy_ht_tbl_down)(phy_type_tbl_ctx_t *ctx)
{
	PHY_TRACE(("%s\n", __FUNCTION__));

	return 0;
}
#endif /* BCMNODOWN */
