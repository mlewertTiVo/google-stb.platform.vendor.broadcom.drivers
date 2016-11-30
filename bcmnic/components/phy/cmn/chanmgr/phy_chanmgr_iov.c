/*
 * Channel Manager module implementation - iovar handlers & registration
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
 * $Id: phy_chanmgr_iov.c 642720 2016-06-09 18:56:12Z vyass $
 */

#include <phy_chanmgr_iov.h>
#include <phy_chanmgr.h>
#include <phy_type_chanmgr.h>
#include <wlc_iocv_reg.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
/* TODO: all these are going away... > */
#endif

/* iovar ids */
enum {
	IOV_BAND_RANGE = 1,
	IOV_BAND_RANGE_SUB = 2,
	IOV_SMTH = 3
};

static const bcm_iovar_t phy_chanmgr_iovars[] = {
	{"band_range", IOV_BAND_RANGE, 0, 0, IOVT_INT8, 0},
	{"subband_idx", IOV_BAND_RANGE_SUB, 0, 0, IOVT_INT8, 0},
#if defined(WLTEST)
	{"smth_enable", IOV_SMTH, (IOVF_SET_UP|IOVF_GET_UP), 0, IOVT_UINT8, 0},
#endif /* defined(WLTEST) */
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_chanmgr_doiovar(void *ctx, uint32 aid,

	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int32 int_val = 0;
	int err = BCME_OK;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
	case IOV_GVAL(IOV_BAND_RANGE_SUB):
		int_val = wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec);
		bcopy(&int_val, a, vsize);
		break;

	case IOV_GVAL(IOV_BAND_RANGE):
		int_val = wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec);
		bcopy(&int_val, a, vsize);
		break;

#if defined(WLTEST)
	case IOV_SVAL(IOV_SMTH):
	{
		err = phy_chanmgr_set_smth(pi, (int8) int_val);
		break;
	}
	case IOV_GVAL(IOV_SMTH):
	{
		err = phy_chanmgr_get_smth(pi, (int32 *) a);
		break;
	}
#endif /* defined(WLTEST) */

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_chanmgr_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_chanmgr_iovars,
	                   NULL, NULL,
	                   phy_chanmgr_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
