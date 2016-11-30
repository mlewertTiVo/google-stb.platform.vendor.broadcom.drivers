/*
 * TSSICAL module implementation - iovar handlers & registration
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
 * $Id: phy_tssical_iov.c 632569 2016-04-19 20:52:56Z vyass $
 */

#include <phy_tssical_iov.h>
#include <phy_tssical_api.h>
#include <wlc_iocv_reg.h>

static const bcm_iovar_t phy_tssical_iovars[] = {
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"tssivisi_thresh", IOV_TSSIVISI_THRESH, 0, 0, IOVT_UINT32, 0},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
	{NULL, 0, 0, 0, 0, 0}
};

static int
phy_tssical_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
#if defined(BCMINTERNAL) || defined(WLTEST)
	phy_info_t *pi = (phy_info_t *)ctx;
	int32 int_val = 0;
	int err = BCME_OK;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
	case IOV_GVAL(IOV_TSSIVISI_THRESH):
		int_val = wlc_phy_tssivisible_thresh((wlc_phy_t *)pi);
		bcopy(&int_val, a, sizeof(int_val));
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
BCMATTACHFN(phy_tssical_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_tssical_iovars,
	                   NULL, NULL,
	                   phy_tssical_doiovar, NULL, NULL, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
