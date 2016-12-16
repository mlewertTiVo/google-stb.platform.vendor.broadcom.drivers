/*
 * HiRSSI eLNA Bypass module implementation - iovar handlers & registration
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
 * $Id: phy_hirssi_iov.c 663404 2016-10-05 07:00:28Z $
 */

#include <phy_hirssi_iov.h>
#include <phy_hirssi.h>
#include <wlc_iocv_reg.h>

static const bcm_iovar_t phy_hirssi_iovars[] = {
#if defined(BCMDBG) || defined(PHYCAL_CHNG_CS)
	{"hirssi_period", IOV_HIRSSI_PERIOD, (IOVF_MFG), 0, IOVT_INT16, 0},
	{"hirssi_en", IOV_HIRSSI_EN, 0, 0, IOVT_UINT8, 0},
	{"hirssi_byp_rssi", IOV_HIRSSI_BYP_RSSI, (IOVF_MFG), 0, IOVT_INT8, 0},
	{"hirssi_res_rssi", IOV_HIRSSI_RES_RSSI, (IOVF_MFG), 0, IOVT_INT8, 0},
	{"hirssi_byp_w1cnt", IOV_HIRSSI_BYP_CNT, (IOVF_NTRL | IOVF_MFG), 0, IOVT_UINT16, 0},
	{"hirssi_res_w1cnt", IOV_HIRSSI_RES_CNT, (IOVF_NTRL | IOVF_MFG), 0, IOVT_UINT16, 0},
	{"hirssi_status", IOV_HIRSSI_STATUS, 0, 0, IOVT_UINT8, 0},
#endif 
	{NULL, 0, 0, 0, 0, 0}
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */

#include <wlc_patch.h>

static int
phy_hirssi_doiovar(void *ctx, uint32 aid,
		void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int32 int_val = 0;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;

	BCM_REFERENCE(pi);
	BCM_REFERENCE(ret_int_ptr);

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
#if defined(BCMDBG) || defined(PHYCAL_CHNG_CS)
		case IOV_GVAL(IOV_HIRSSI_PERIOD):
			err = phy_hirssi_get_period(pi, ret_int_ptr);
			break;
		case IOV_SVAL(IOV_HIRSSI_PERIOD):
			err = phy_hirssi_set_period(pi, int_val);
			break;
		case IOV_GVAL(IOV_HIRSSI_EN):
			err = phy_hirssi_get_en(pi, ret_int_ptr);
			break;
		case IOV_SVAL(IOV_HIRSSI_EN):
			err = phy_hirssi_set_en(pi, int_val);
			break;
		case IOV_GVAL(IOV_HIRSSI_BYP_RSSI):
			err = phy_hirssi_get_rssi(pi, ret_int_ptr, PHY_HIRSSI_BYP);
			break;
		case IOV_SVAL(IOV_HIRSSI_BYP_RSSI):
			err = phy_hirssi_set_rssi(pi, int_val, PHY_HIRSSI_BYP);
			break;
		case IOV_GVAL(IOV_HIRSSI_RES_RSSI):
			err = phy_hirssi_get_rssi(pi, ret_int_ptr, PHY_HIRSSI_RES);
			break;
		case IOV_SVAL(IOV_HIRSSI_RES_RSSI):
			err = phy_hirssi_set_rssi(pi, int_val, PHY_HIRSSI_RES);
			break;
		case IOV_GVAL(IOV_HIRSSI_BYP_CNT):
			err = phy_hirssi_get_cnt(pi, ret_int_ptr, PHY_HIRSSI_BYP);
			break;
		case IOV_SVAL(IOV_HIRSSI_BYP_CNT):
			err = phy_hirssi_set_cnt(pi, int_val, PHY_HIRSSI_BYP);
			break;
		case IOV_GVAL(IOV_HIRSSI_RES_CNT):
			err = phy_hirssi_get_cnt(pi, ret_int_ptr, PHY_HIRSSI_RES);
			break;
		case IOV_SVAL(IOV_HIRSSI_RES_CNT):
			err = phy_hirssi_set_cnt(pi, int_val, PHY_HIRSSI_RES);
			break;
		case IOV_GVAL(IOV_HIRSSI_STATUS):
			err = phy_hirssi_get_status(pi, ret_int_ptr);
			break;
#endif 
	default:
		err = BCME_UNSUPPORTED;
		break;
	}
	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_hirssi_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;
#if defined(WLC_PATCH_IOCTL)
    wlc_iov_disp_fn_t disp_fn = IOV_PATCH_FN;
    const bcm_iovar_t* patch_table = IOV_PATCH_TBL;
#else
    wlc_iov_disp_fn_t disp_fn = NULL;
    const bcm_iovar_t* patch_table = NULL;
#endif /* WLC_PATCH_IOCTL */


	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_hirssi_iovars,
	                   NULL, NULL,
	                   phy_hirssi_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
