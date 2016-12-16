/*
 * RSSICompute module implementation - iovar handlers & registration
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

#include <phy_rssi_iov.h>
#include <phy_rssi.h>
#include <wlc_iocv_reg.h>
#include <wlc_phy_int.h>

static const bcm_iovar_t phy_rssi_iovars[] = {
	{"phy_rssi_gain_delta_2g", IOV_PHY_RSSI_GAIN_DELTA_2G,
	(0), 0, IOVT_BUFFER, 18*sizeof(int8)},
	{"phy_rssi_gain_delta_5gl", IOV_PHY_RSSI_GAIN_DELTA_5GL,
	(0), 0, IOVT_BUFFER, 6*sizeof(int8)},
	{"phy_rssi_gain_delta_5gml", IOV_PHY_RSSI_GAIN_DELTA_5GML,
	(0), 0, IOVT_BUFFER, 6*sizeof(int8)},
	{"phy_rssi_gain_delta_5gmu", IOV_PHY_RSSI_GAIN_DELTA_5GMU,
	(0), 0, IOVT_BUFFER, 6*sizeof(int8)},
	{"phy_rssi_gain_delta_5gh", IOV_PHY_RSSI_GAIN_DELTA_5GH,
	(0), 0, IOVT_BUFFER, 6*sizeof(int8)},
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_rssi_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int err = BCME_OK;
	int8 *setValues = p, *getValues = a;

	switch (aid) {
		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2G):
			err = phy_rssi_set_gain_delta_2g(pi->rssii, aid, setValues);
			break;

		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2G):
			err = phy_rssi_get_gain_delta_2g(pi->rssii, aid, getValues);
			break;

		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_5GL):
		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_5GML):
		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_5GMU):
		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_5GH):
			err = phy_rssi_set_gain_delta_5g(pi->rssii, aid, setValues);
			break;

		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_5GL):
		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_5GML):
		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_5GMU):
		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_5GH):
			err = phy_rssi_get_gain_delta_5g(pi->rssii, aid, getValues);
			break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_rssi_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_rssi_iovars,
	                   NULL, NULL,
	                   phy_rssi_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
