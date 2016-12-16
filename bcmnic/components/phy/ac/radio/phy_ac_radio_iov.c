/*
 * ACPHY RADIO control module implementation - iovar handlers & registration
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

#include <phy_ac_radio.h>
#include <phy_ac_info.h>
#include <phy_ac_radio_iov.h>

#include <wlc_iocv_reg.h>
#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>
#endif
#include <phy_ac_info.h>

/* iovar ids */
enum {
	IOV_LP_MODE = 1,
	IOV_LP_VCO_2G = 2,
	IOV_RADIO_PD = 3
};

static const bcm_iovar_t phy_ac_radio_iovars[] = {
	{"lp_mode", IOV_LP_MODE, (IOVF_SET_UP|IOVF_GET_UP), 0, IOVT_UINT8, 0},
	{"lp_vco_2g", IOV_LP_VCO_2G, (IOVF_SET_UP|IOVF_GET_UP), 0, IOVT_UINT8, 0},
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_ac_radio_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int err = BCME_OK;
	int int_val = 0;
	int32 *ret_int_ptr = (int32 *)a;
	phy_ac_radio_info_t *radioi = pi->u.pi_acphy->radioi;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
	case IOV_SVAL(IOV_LP_MODE):
	{
		if ((int_val > 3) || (int_val < 1)) {
			PHY_ERROR(("LP MODE %d is not supported \n", (uint16)int_val));
			err = BCME_RANGE;
		} else {
			wlc_phy_lp_mode(radioi, (int8) int_val);
		}
		break;
	}
	case IOV_GVAL(IOV_LP_MODE):
	{
		*ret_int_ptr = phy_ac_radio_get_data(radioi)->acphy_lp_status;
		break;
	}
	case IOV_SVAL(IOV_LP_VCO_2G):
	{
		if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
			if ((int_val != 0) && (int_val != 1)) {
				PHY_ERROR(("LP MODE %d is not supported \n", (uint16)int_val));
				err = BCME_RANGE;
			} else {
				wlc_phy_force_lpvco_2G(pi, (int8) int_val);
			}
		} else {
			PHY_ERROR(("LP MODE is not supported for this chip \n"));
			err = BCME_UNSUPPORTED;
		}
		break;
	}
	case IOV_GVAL(IOV_LP_VCO_2G):
	{
		if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
			*ret_int_ptr = phy_ac_radio_get_data(radioi)->acphy_force_lpvco_2G;
		} else {
			PHY_ERROR(("LP MODE is not supported for this chip \n"));
			err = BCME_UNSUPPORTED;
		}
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
BCMATTACHFN(phy_ac_radio_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_ac_radio_iovars,
	                   NULL, NULL,
	                   phy_ac_radio_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
