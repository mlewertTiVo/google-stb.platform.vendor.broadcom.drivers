/*
 * NPHY PHYTblInit module implementation - iovar handlers & registration
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
#include <phy_api.h>
#include <phy_tbl_iov.h>
#include <phy_n_tbl_iov.h>

#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>

#ifdef WLC_LOW
#ifndef ALL_NEW_PHY_MOD
#include <wlc_phyreg_n.h>
#include <wlc_phy_int.h>
#endif

#include <phy_utils_reg.h>

static int
phy_n_tbl_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize)
{
	return BCME_UNSUPPORTED;
}
#endif /* WLC_LOW */

/* register iovar table to the system */
int
BCMATTACHFN(phy_n_tbl_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_tbl_iovars,
	                   NULL, NULL,
	                   phy_n_tbl_doiovar, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
