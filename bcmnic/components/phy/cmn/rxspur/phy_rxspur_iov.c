/*
 * Rx Spur canceller module implementation - iovar handlers & registration
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
 * $Id: $
 */

#include <phy_rxspur_iov.h>
#include <phy_rxspur.h>
#include <wlc_iocv_reg.h>
#include <wlc_phy_int.h>

/* iovar ids */
enum {
	IOV_PHY_FORCE_SPURMODE = 1
};

static const bcm_iovar_t phy_rxspur_iovars[] = {
#if defined(WLTEST)
	{"phy_force_spurmode", IOV_PHY_FORCE_SPURMODE, IOVF_SET_UP, 0, IOVT_UINT32, 0},
#endif /* WLTEST */
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_rxspur_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
#if defined(WLTEST)
	phy_info_t *pi = (phy_info_t *)ctx;
	int32 int_val = 0;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
	case IOV_SVAL(IOV_PHY_FORCE_SPURMODE):
		err = phy_rxspur_set_force_spurmode(pi->rxspuri, (int16) int_val);
		break;

	case IOV_GVAL(IOV_PHY_FORCE_SPURMODE):
		err = phy_rxspur_get_force_spurmode(pi->rxspuri, ret_int_ptr);
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
#else /* WLTEST */
	return BCME_UNSUPPORTED;
#endif /* WLTEST */
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_rxspur_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;
#if defined(WLC_PATCH_IOCTL)
	wlc_iov_disp_fn_t disp_fn = IOV_PATCH_FN;
	const bcm_iovar_t *patch_table = IOV_PATCH_TBL;
#else
	wlc_iov_disp_fn_t disp_fn = NULL;
	const bcm_iovar_t* patch_table = NULL;
#endif /* WLC_PATCH_IOCTL */

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_rxspur_iovars,
	                   NULL, NULL,
	                   phy_rxspur_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
