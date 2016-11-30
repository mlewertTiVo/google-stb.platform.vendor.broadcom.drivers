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
 * $Id: phy_txpwrcap_iov.c 629423 2016-04-05 11:12:48Z pieterpg $
 */

#if defined(WLC_TXPWRCAP)

#include <phy_txpwrcap.h>
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
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"phy_txpwrcap_tbl", IOV_PHY_TXPWRCAP_TBL, 0, 0, IOVT_BUFFER, sizeof(wl_txpwrcap_tbl_t)},
#endif
	{"phy_cellstatus", IOV_PHY_CELLSTATUS, IOVF_SET_UP | IOVF_GET_UP, 0, IOVT_INT8, 0},
	{"phy_txpwrcap", IOV_PHY_TXPWRCAP, IOVF_GET_UP, 0, IOVT_UINT32, 0},
	{NULL, 0, 0, 0, 0, 0}
};

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
		if (PHYTXPWRCAP_ENAB(pi))
			err = wlc_phyhal_txpwrcap_get_cellstatus((wlc_phy_t*)pi,
					ret_int_ptr);
		else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_SVAL(IOV_PHY_CELLSTATUS):
		if (PHYTXPWRCAP_ENAB(pi))
			err = wlc_phy_cellstatus_override_set(pi, int_val);
		else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_GVAL(IOV_PHY_TXPWRCAP):
		if (PHYTXPWRCAP_ENAB(pi)) {
			if (!pi->sh->clk) {
				err = BCME_NOCLK;
				break;
			}
			*ret_int_ptr = wlc_phy_get_txpwrcap_inuse(pi);
		} else
			err = BCME_UNSUPPORTED;
		break;

#if defined(BCMINTERNAL) || defined(WLTEST)
	case IOV_GVAL(IOV_PHY_TXPWRCAP_TBL):
		if (PHYTXPWRCAP_ENAB(pi)) {
			wl_txpwrcap_tbl_t txpwrcap_tbl;
			err = wlc_phy_txpwrcap_tbl_get((wlc_phy_t*)pi, &txpwrcap_tbl);
			if (err == BCME_OK)
				bcopy(&txpwrcap_tbl, a, sizeof(wl_txpwrcap_tbl_t));
		} else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_SVAL(IOV_PHY_TXPWRCAP_TBL):
		if (PHYTXPWRCAP_ENAB(pi)) {
			wl_txpwrcap_tbl_t txpwrcap_tbl;
			bcopy(p, &txpwrcap_tbl, sizeof(wl_txpwrcap_tbl_t));
			err = wlc_phy_txpwrcap_tbl_set((wlc_phy_t*)pi, &txpwrcap_tbl);
		} else
			err = BCME_UNSUPPORTED;
		break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
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
	bcm_iovar_t *patch_table = IOV_PATCH_TBL;
#else
	wlc_iov_disp_fn_t disp_fn = NULL;
	bcm_iovar_t* patch_table = NULL;
#endif /* WLC_PATCH_IOCTL */

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_txpwrcap_iovars,
	                   NULL, NULL,
	                   phy_txpwrcap_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}

#endif /* WLC_TXPWRCAP */
