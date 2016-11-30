/*
 * ACPHY ACPHY VCO CAL module implementation - iovar handlers & registration
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
 * $Id: phy_vcocal_iov.c 619527 2016-02-17 05:54:49Z renukad $
 */

#include <phy_vcocal_iov.h>
#include <phy_vcocal.h>
#include <wlc_iocv_reg.h>

static const bcm_iovar_t phy_vcocal_iovars[] = {
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"phy_vcocal", IOV_PHY_VCOCAL, (IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
	{NULL, 0, 0, 0, 0, 0}
};

static int
phy_vcocal_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
#if defined(BCMINTERNAL) || defined(WLTEST)
	phy_info_t *pi = (phy_info_t *)ctx;
	int err = BCME_OK;

	switch (aid) {
	case IOV_GVAL(IOV_PHY_VCOCAL):
	case IOV_SVAL(IOV_PHY_VCOCAL):
		phy_vcocal_force(pi);
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
#else
	return BCME_UNSUPPORTED;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_vcocal_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_vcocal_iovars,
	                   NULL, NULL,
	                   phy_vcocal_doiovar, NULL, NULL, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
