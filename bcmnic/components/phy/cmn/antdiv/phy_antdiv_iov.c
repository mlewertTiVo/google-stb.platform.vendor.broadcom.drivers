/*
 * ANTennaDIVersity module implementation - iovar handlers & registration
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
 * $Id: phy_antdiv_iov.c 642720 2016-06-09 18:56:12Z vyass $
 */

#include <phy_antdiv_iov.h>
#include <phy_antdiv.h>
#include <wlc_iocv_reg.h>

/* iovar ids */
enum {
	IOV_ANT_DIV_SW_CORE0 = 1,
	IOV_ANT_DIV_SW_CORE1 = 2
};

static const bcm_iovar_t phy_antdiv_iovars[] = {
	{"ant_diversity_sw_core0", IOV_ANT_DIV_SW_CORE0, (IOVF_SET_UP|IOVF_GET_UP), 0,
	IOVT_UINT8, 0},
	{"ant_diversity_sw_core1", IOV_ANT_DIV_SW_CORE1, (IOVF_SET_UP|IOVF_GET_UP), 0,
	IOVT_UINT8, 0},
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_antdiv_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int32 int_val = 0;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
	case IOV_SVAL(IOV_ANT_DIV_SW_CORE0):
	{
		phy_antdiv_set_sw_control(pi, (int8) int_val, 0);
		break;
	}
	case IOV_GVAL(IOV_ANT_DIV_SW_CORE0):
	{
		phy_antdiv_get_sw_control(pi, ret_int_ptr, 0);
		break;
	}
	case IOV_SVAL(IOV_ANT_DIV_SW_CORE1):
	{
		phy_antdiv_set_sw_control(pi, (int8) int_val, 1);
		break;
	}
	case IOV_GVAL(IOV_ANT_DIV_SW_CORE1):
	{
		phy_antdiv_get_sw_control(pi, ret_int_ptr, 1);
		break;
	}
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_antdiv_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_antdiv_iovars,
	                   NULL, NULL,
	                   phy_antdiv_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
