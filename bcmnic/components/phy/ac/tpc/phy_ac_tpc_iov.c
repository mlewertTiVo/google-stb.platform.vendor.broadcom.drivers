/*
 * ACPHY TxPowerControl module implementation - iovar handlers & registration
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
 * $Id: phy_ac_tpc_iov.c 658512 2016-09-08 07:03:22Z $
 */

#include <phy_ac_tpc.h>
#include <phy_ac_info.h>
#include <phy_ac_tpc_iov.h>
#include <phy_tpc_iov.h>
#include <phy_type_tpc.h>
#include <wlc_iocv_reg.h>

/* iovar ids */
enum {
	IOV_OVRINITBASEIDX = 1,
	IOV_PHY_TONE_TXPWR = 2
};

static const bcm_iovar_t phy_ac_tpc_iovars[] = {
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_ac_tpc_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int err = BCME_OK;
	int int_val = 0;
	int32 *ret_int_ptr = (int32 *)a;

	BCM_REFERENCE(*ret_int_ptr);
	BCM_REFERENCE(*pi);

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
	default:
		err = BCME_OK;
		break;
	}
	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_ac_tpc_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;
#if defined(WLC_PATCH_IOCTL)
	wlc_iov_disp_fn_t disp_fn = IOV_PATCH_FN;
	bcm_iovar_t* patch_table = IOV_PATCH_TBL;
#else
	wlc_iov_disp_fn_t disp_fn = NULL;
	bcm_iovar_t* patch_table = NULL;
#endif /* WLC_PATCH_IOCTL */

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_ac_tpc_iovars,
	                   NULL, NULL,
	                   phy_ac_tpc_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
