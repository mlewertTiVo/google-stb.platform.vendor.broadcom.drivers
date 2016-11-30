/*
 * txiqlocal module implementation - iovar table/handlers & registration
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

#include <typedefs.h>
#include <phy_api.h>
#include <phy_txiqlocal.h>
#include <phy_txiqlocal_iov.h>

#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>

#include <phy_dbg.h>

/* iovar table */

static const bcm_iovar_t phy_txiqlocal_iovars[] = {
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD)
	{"phy_txiqcc", IOV_PHY_TXIQCC,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER,  2*sizeof(int32)
	},
	{"phy_txlocc", IOV_PHY_TXLOCC,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, 6
	},
#endif /* BCMINTERNAL || WLTEST || ATE_BUILD */
	{NULL, 0, 0, 0, 0, 0}
};

#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>
#endif

static int
phy_txiqlocal_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	int err = BCME_OK;
	int int_val = 0;
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD)
	phy_info_t *pi = (phy_info_t *)ctx;
#endif /* BCMINTERNAL || WLTEST || ATE_BUILD */

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD)
	case IOV_GVAL(IOV_PHY_TXIQCC):
	{
		phy_txiqlocal_txiqccget(pi, a);
		break;
	}
	case IOV_SVAL(IOV_PHY_TXIQCC):
	{
		phy_txiqlocal_txiqccset(pi, p);
		break;
	}
	case IOV_GVAL(IOV_PHY_TXLOCC):
	{
		phy_txiqlocal_txloccget(pi, a);
		break;
	}
	case IOV_SVAL(IOV_PHY_TXLOCC):
	{
		phy_txiqlocal_txloccset(pi, p);
		break;
	}
#endif /* BCMINTERNAL || WLTEST || ATE_BUILD */
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_txiqlocal_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_txiqlocal_iovars,
	                   NULL, NULL,
	                   phy_txiqlocal_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
