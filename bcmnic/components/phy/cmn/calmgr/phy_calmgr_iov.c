/*
 * Calibration manager module implementation - iovar handlers & registration
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
 * $Id: phy_calmgr_iov.c $
 */

#include <phy_calmgr_iov.h>
#include <wlc_iocv_reg.h>
#include <phy_calmgr.h>
#include <phy_calmgr_api.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
/* TODO: all these are going away... > */
#endif

/* iovar ids */
enum {
	IOV_PHY_PERICAL = 1,
	IOV_PHY_PERICAL_DELAY = 2,
	IOV_SINGLE_PHASE_CAL_CNTR = 3,
	IOV_MULTI_PHASE_CAL_CNTR = 4
};

static const bcm_iovar_t phy_calmgr_iovars[] = {
	{"phy_percal", IOV_PHY_PERICAL, (IOVF_MFG), 0, IOVT_UINT8, 0 },
	{"phy_percal_delay", IOV_PHY_PERICAL_DELAY, (0), 0, IOVT_UINT16, 0 },
	{"cal_sp_cntr", IOV_SINGLE_PHASE_CAL_CNTR,
	(IOVF_GET_UP), 0, IOVT_UINT32, 0
	},
	{"cal_mp_cntr", IOV_MULTI_PHASE_CAL_CNTR,
	(IOVF_GET_UP), 0, IOVT_UINT32, 0
	},
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_calmgr_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int32 int_val = 0;
	int err = BCME_OK;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
	case IOV_GVAL(IOV_PHY_PERICAL):
#if defined(AP) || defined(BCMDBG_PHYCAL)
		int_val = phy_calmgr_get_calmode(pi);
		bcopy(&int_val, a, vsize);
#else
		err = BCME_UNSUPPORTED;
#endif
		break;
	case IOV_SVAL(IOV_PHY_PERICAL):
#if defined(AP) || defined(BCMDBG_PHYCAL)
		err = phy_calmgr_set_calmode(pi, int_val);
#else
		err = BCME_UNSUPPORTED;
#endif
		break;
	case IOV_GVAL(IOV_PHY_PERICAL_DELAY):
		int_val = (int32)pi->phy_cal_delay;
		bcopy(&int_val, a, vsize);
		break;

	case IOV_SVAL(IOV_PHY_PERICAL_DELAY):
		if ((int_val >= PHY_PERICAL_DELAY_MIN) && (int_val <= PHY_PERICAL_DELAY_MAX))
			pi->phy_cal_delay = (uint16)int_val;
		else
			err = BCME_RANGE;
		break;

	case IOV_GVAL(IOV_SINGLE_PHASE_CAL_CNTR):
		int_val = (int32)pi->cal_info->fullphycalcntr;
		bcopy(&int_val, a, vsize);
		break;

	case IOV_GVAL(IOV_MULTI_PHASE_CAL_CNTR):
		int_val = (int32)pi->cal_info->multiphasecalcntr;
		bcopy(&int_val, a, vsize);
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_calmgr_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_calmgr_iovars,
	                   NULL, NULL,
	                   phy_calmgr_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
