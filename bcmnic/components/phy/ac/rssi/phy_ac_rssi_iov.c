/*
 * ACPHY RSSICompute module implementation - iovar handlers & registration
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

#include <phy_ac_rssi_iov.h>
#include <phy_ac_rssi.h>
#include <wlc_iocv_reg.h>
#include <phy_ac_info.h>

/* iovar ids */
enum {
	IOV_PHY_RSSI_CAL_REV = 1,
	IOV_PHY_RSSI_QDB_EN = 2,
	IOV_PHY_RXGAIN_RSSI = 3
};

static const bcm_iovar_t phy_ac_rssi_iovars[] = {
	{"rssi_cal_rev", IOV_PHY_RSSI_CAL_REV,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_INT16, 0},
	{"rssi_qdb_en", IOV_PHY_RSSI_QDB_EN,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_INT16, 0},
	{"rxg_rssi", IOV_PHY_RXGAIN_RSSI, (IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_INT16, 0},
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_ac_rssi_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int32 int_val = 0;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
		case IOV_SVAL(IOV_PHY_RSSI_CAL_REV):
			pi->u.pi_acphy->rssi_cal_rev = (bool)int_val;
			break;

		case IOV_GVAL(IOV_PHY_RSSI_CAL_REV):
			*ret_int_ptr = pi->u.pi_acphy->rssi_cal_rev;
			break;

		case IOV_SVAL(IOV_PHY_RSSI_QDB_EN):
			err = phy_ac_rssi_set_qdb_en(pi->u.pi_acphy->rssii, (bool) int_val);
			break;

		case IOV_GVAL(IOV_PHY_RSSI_QDB_EN):
			err = phy_ac_rssi_get_qdb_en(pi->u.pi_acphy->rssii, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_RXGAIN_RSSI):
			pi->u.pi_acphy->rxgaincal_rssical = (bool)int_val;
			break;

		case IOV_GVAL(IOV_PHY_RXGAIN_RSSI):
			*ret_int_ptr = pi->u.pi_acphy->rxgaincal_rssical;
			break;

		default:
			err = BCME_UNSUPPORTED;
			break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_ac_rssi_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_ac_rssi_iovars,
	                   NULL, NULL,
	                   phy_ac_rssi_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
