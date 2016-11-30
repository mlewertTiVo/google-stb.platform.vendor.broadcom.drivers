/*
 * ACPHY Channel Manager module implementation - iovar handlers & registration
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
 * $Id: phy_ac_chanmgr_iov.c 642720 2016-06-09 18:56:12Z vyass $
 */

#include <phy_ac_chanmgr_iov.h>
#include <phy_ac_chanmgr.h>
#include <wlc_iocv_reg.h>

/* iovar ids */
enum {
	IOV_PHY_CLBPRIO_2G = 1,
	IOV_PHY_CLBPRIO_5G = 2
};

static const bcm_iovar_t phy_ac_chanmgr_iovars[] = {
	{"phy_clbprio2g", IOV_PHY_CLBPRIO_2G, 0, 0, IOVT_INT32, 0},
	{"phy_clbprio5g", IOV_PHY_CLBPRIO_5G, 0, 0, IOVT_INT32, 0},
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_ac_chanmgr_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int32 int_val = 0;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
		case IOV_GVAL(IOV_PHY_CLBPRIO_2G):
			*ret_int_ptr = wlc_phy_femctrl_clb_prio_2g_acphy(pi, FALSE, 0);
			break;
		case IOV_SVAL(IOV_PHY_CLBPRIO_2G):
			*ret_int_ptr = wlc_phy_femctrl_clb_prio_2g_acphy(pi, TRUE, int_val);
			break;
		case IOV_GVAL(IOV_PHY_CLBPRIO_5G):
			*ret_int_ptr = wlc_phy_femctrl_clb_prio_5g_acphy(pi, FALSE, 0);
			break;
		case IOV_SVAL(IOV_PHY_CLBPRIO_5G):
			*ret_int_ptr = wlc_phy_femctrl_clb_prio_5g_acphy(pi, TRUE, int_val);
			break;

		default:
			err = BCME_UNSUPPORTED;
			break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_ac_chanmgr_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_ac_chanmgr_iovars,
	                   NULL, NULL,
	                   phy_ac_chanmgr_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
