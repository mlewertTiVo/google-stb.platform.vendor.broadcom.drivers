/*
 * Health check module.
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
 * $Id: phy_hc_iov.c 656120 2016-08-25 08:17:57Z $
 */
#ifdef RADIO_HEALTH_CHECK
#include <phy_hc_iov.h>
#include <phy_hc.h>
#include <phy_type_hc.h>
#include <wlc_iocv_reg.h>
#include <phy_hc_api.h>
#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>
#endif

/* iovar ids */
enum {
	IOV_HEALTHCHECK_PHYRADIO = 1
};

/* iovar table */
static const bcm_iovar_t phy_hc_iovars[] = {
	{"radio_health_check", IOV_HEALTHCHECK_PHYRADIO,
	(IOVF_GET_UP | IOVF_SET_UP), 0, IOVT_INT8, 0
	},
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_hc_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	int err = BCME_OK;
	phy_info_t *pi = (phy_info_t *)ctx;
	int int_val = 0;
	int32 *ret_int_ptr = (int32 *)a;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	(void)pi;
	(void)ret_int_ptr;

	switch (aid) {
		case IOV_GVAL(IOV_HEALTHCHECK_PHYRADIO):
			*ret_int_ptr = phy_hc_iovar_hc_mode(pi->hci, TRUE, 0);
			break;
		case IOV_SVAL(IOV_HEALTHCHECK_PHYRADIO):
			err = phy_hc_iovar_hc_mode(pi->hci, FALSE, int_val);
			break;
		default:
			err = BCME_UNSUPPORTED;
			break;
	}
	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_hc_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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
	wlc_iocv_init_iovd(phy_hc_iovars,
	                   NULL, NULL,
	                   phy_hc_doiovar, disp_fn, patch_table, pi,
	                   &iovd);
	return wlc_iocv_register_iovt(ii, &iovd);
}
#endif /* RADIO_HEALTH_CHECK */
