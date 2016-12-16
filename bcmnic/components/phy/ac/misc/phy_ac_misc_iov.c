/*
 * ACPHY Miscellaneous module implementation - iovar handlers & registration
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

#include <phy_ac_misc_iov.h>
#include <phy_ac_misc.h>
#include <wlc_iocv_reg.h>
#include <phy_ac_info.h>

/* iovar ids */
enum {
	IOV_PHY_RXGAINERR_2G = 1,
	IOV_PHY_RXGAINERR_5GL = 2,
	IOV_PHY_RXGAINERR_5GM = 3,
	IOV_PHY_RXGAINERR_5GH = 4,
	IOV_PHY_RXGAINERR_5GU = 5,
	IOV_PHY_RUD_AGC_ENABLE = 6
};

static const bcm_iovar_t phy_ac_misc_iovars[] = {
	{"rud_agc_enable", IOV_PHY_RUD_AGC_ENABLE,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_INT16, 0},
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_ac_misc_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{

	phy_info_t *pi = (phy_info_t *)ctx;
	int err = BCME_OK;
	int8 *setDeltaValues = p, *getDeltaValues = a;
	uint8 core;

	BCM_REFERENCE(getDeltaValues);
	BCM_REFERENCE(setDeltaValues);
	BCM_REFERENCE(core);

	switch (aid) {
	case IOV_SVAL(IOV_PHY_RUD_AGC_ENABLE):
		{
			int32 int_val = 0;
			if (plen >= (uint)sizeof(int_val)) {
				bcopy(p, &int_val, sizeof(int_val));
			}
			err = phy_ac_misc_set_rud_agc_enable(pi->u.pi_acphy->misci, int_val);
		}
		break;

	case IOV_GVAL(IOV_PHY_RUD_AGC_ENABLE):
		{
			int32 *ret_int_ptr = (int32 *)a;
			err = phy_ac_misc_get_rud_agc_enable(pi->u.pi_acphy->misci, ret_int_ptr);
		}
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_ac_misc_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_ac_misc_iovars,
	                   NULL, NULL,
	                   phy_ac_misc_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
