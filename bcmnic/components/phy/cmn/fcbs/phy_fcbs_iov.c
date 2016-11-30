/*
 * FCBS module implementation - iovar table/handlers & registration
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

#include <wlc_cfg.h>
#include <typedefs.h>
#include <phy_api.h>
#include <phy_fcbs.h>
#include <phy_fcbs_iov.h>

#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>

#include <wlc_phy_shim.h>
#include <phy_utils_reg.h>
#include <phy_ac_info.h>
#include <wlc_phy_n.h>
#include <wlc_phy_lcn.h>
#include <phy_dbg.h>

/* iovar table */
static const bcm_iovar_t phy_fcbs_iovars[] = {
	{"phy_fcbs_init", IOV_PHY_FCBSINIT,
	(IOVF_SET_UP), 0, IOVT_INT8, 0
	},
	{"phy_fcbs", IOV_PHY_FCBS,
	(IOVF_SET_UP | IOVF_GET_UP), 0, IOVT_UINT8, 0
	},
	{"phy_fcbs_arm", IOV_PHY_FCBSARM,
	(IOVF_SET_UP), 0, IOVT_UINT8, 0
	},
	{"phy_fcbs_exit", IOV_PHY_FCBSEXIT,
	(IOVF_SET_UP), 0, IOVT_UINT8, 0
	},
	{NULL, 0, 0, 0, 0, 0}
};

#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>
#endif

static int
phy_fcbs_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	int err = BCME_OK;
	int32 *ret_int_ptr;
	int int_val = 0;
#ifdef ENABLE_FCBS
	phy_info_t *pi = (phy_info_t *)ctx;
#endif /* ENABLE_FCBS */

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)a;

	(void)ret_int_ptr;

	switch (aid) {
#ifdef ENABLE_FCBS
	case IOV_SVAL(IOV_PHY_FCBSINIT):
		if (ISHTPHY(pi)) {
			if ((int_val >= FCBS_CHAN_A) && (int_val <= FCBS_CHAN_B)) {
				wlc_phy_fcbs_init((wlc_phy_t*)pi, int_val);
			} else {
				err = BCME_RANGE;
			}
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_SVAL(IOV_PHY_FCBS):
		if (ISACPHY(pi)) {
			pi->FCBS = (bool)int_val;
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_GVAL(IOV_PHY_FCBS):
		if (ISACPHY(pi)) {
			*ret_int_ptr = pi->FCBS;
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_GVAL(IOV_PHY_FCBSARM):
		if (ISACPHY(pi)) {
			wlc_phy_fcbs_arm((wlc_phy_t*)pi, 0xFFFF, 0);
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_GVAL(IOV_PHY_FCBSEXIT):
		if (ISACPHY(pi)) {
			 wlc_phy_fcbs_exit((wlc_phy_t*)pi);
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
#endif /* ENABLE_FCBS */
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_fcbs_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;
#if defined(WLC_PATCH_IOCTL)
	wlc_iov_disp_fn_t disp_fn = IOV_PATCH_FN;
	bcm_iovar_t *patch_table = IOV_PATCH_TBL;
#else
	wlc_iov_disp_fn_t disp_fn = NULL;
	bcm_iovar_t* patch_table = NULL;
#endif /* WLC_PATCH_IOCTL */

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_fcbs_iovars,
	                   NULL, NULL,
	                   phy_fcbs_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
