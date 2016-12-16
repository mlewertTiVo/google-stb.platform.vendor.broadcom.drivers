/*
 * TxPowerCap module implementation - iovar table
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
 * $Id: phy_txpwrcap_iov.c 661168 2016-09-23 11:34:45Z $
 */

#if defined(WLC_TXPWRCAP)

#include <phy_txpwrcap.h>
#include <phy_type_txpwrcap.h>
#include <phy_txpwrcap_iov.h>
#include <phy_txpwrcap_api.h>
#include <wlc_iocv_reg.h>
#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>
#endif

enum {
	IOV_PHY_CELLSTATUS,
	IOV_PHY_TXPWRCAP,
	IOV_PHY_TXPWRCAP_TBL
};

/* iovar table */
static const bcm_iovar_t phy_txpwrcap_iovars[] = {
	{"phy_cellstatus", IOV_PHY_CELLSTATUS, IOVF_SET_UP | IOVF_GET_UP, 0, IOVT_INT8, 0},
	{"phy_txpwrcap", IOV_PHY_TXPWRCAP, IOVF_GET_UP, 0, IOVT_UINT32, 0},
	{NULL, 0, 0, 0, 0, 0}
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

static int
phy_txpwrcap_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	int err = BCME_OK;
	phy_info_t *pi = (phy_info_t *)ctx;
	bool bool_val = FALSE;
	int int_val = 0;
	int32 *ret_int_ptr = (int32 *)a;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	/* bool conversion to avoid duplication below */
	bool_val = int_val != 0;

	(void)pi;
	(void)bool_val;

	switch (aid) {
	case IOV_GVAL(IOV_PHY_CELLSTATUS):
		if (PHYTXPWRCAP_ENAB(pi->txpwrcapi))
			err = wlc_phyhal_txpwrcap_get_cellstatus((wlc_phy_t*)pi,
					ret_int_ptr);
		else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_SVAL(IOV_PHY_CELLSTATUS):
		if (PHYTXPWRCAP_ENAB(pi->txpwrcapi))
			err = phy_txpwrcap_cellstatus_override_set(pi, int_val);
		else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_GVAL(IOV_PHY_TXPWRCAP):
		if (PHYTXPWRCAP_ENAB(pi->txpwrcapi)) {
			if (!pi->sh->clk) {
				err = BCME_NOCLK;
				break;
			}
			*ret_int_ptr = phy_txpwrcap_get_caps_inuse(pi);
		} else
			err = BCME_UNSUPPORTED;
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_txpwrcap_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_txpwrcap_iovars,
	                   NULL, NULL,
	                   phy_txpwrcap_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}

#endif /* WLC_TXPWRCAP */
