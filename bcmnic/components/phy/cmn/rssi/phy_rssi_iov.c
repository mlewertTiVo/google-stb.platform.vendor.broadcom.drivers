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
	(IOVF_GET_UP | IOVF_SET_UP), 0, IOVT_BUFFER, 18*sizeof(int8)},
#ifdef WLTEST
	{"phy_rssi_gain_delta_2gh", IOV_PHY_RSSI_GAIN_DELTA_2GH,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, 18*sizeof(int8)},
	{"phy_rssi_gain_delta_2ghh", IOV_PHY_RSSI_GAIN_DELTA_2GHH,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, 18*sizeof(int8)},
	{"rssi_cal_freq_grp_2g", IOV_PHY_RSSI_CAL_FREQ_GRP_2G,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, 7*sizeof(int8)},
	{"phy_rssi_gain_delta_2gb0", IOV_PHY_RSSI_GAIN_DELTA_2GB0,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, sizeof(int8)},
	{"phy_rssi_gain_delta_2gb1", IOV_PHY_RSSI_GAIN_DELTA_2GB1,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, sizeof(int8)},
	{"phy_rssi_gain_delta_2gb2", IOV_PHY_RSSI_GAIN_DELTA_2GB2,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, sizeof(int8)},
	{"phy_rssi_gain_delta_2gb3", IOV_PHY_RSSI_GAIN_DELTA_2GB3,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, sizeof(int8)},
	{"phy_rssi_gain_delta_2gb4", IOV_PHY_RSSI_GAIN_DELTA_2GB4,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, sizeof(int8)},
#endif /* WLTEST */
	{"phy_rssi_gain_delta_5gl", IOV_PHY_RSSI_GAIN_DELTA_5GL,
	(IOVF_GET_UP | IOVF_SET_UP), 0, IOVT_BUFFER, 6*sizeof(int8)},
	{"phy_rssi_gain_delta_5gml", IOV_PHY_RSSI_GAIN_DELTA_5GML,
	(IOVF_GET_UP | IOVF_SET_UP), 0, IOVT_BUFFER, 6*sizeof(int8)},
	{"phy_rssi_gain_delta_5gmu", IOV_PHY_RSSI_GAIN_DELTA_5GMU,
	(IOVF_GET_UP | IOVF_SET_UP), 0, IOVT_BUFFER, 6*sizeof(int8)},
	{"phy_rssi_gain_delta_5gh", IOV_PHY_RSSI_GAIN_DELTA_5GH,
	(IOVF_GET_UP | IOVF_SET_UP), 0, IOVT_BUFFER, 6*sizeof(int8)},
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"pkteng_stats", IOV_PKTENG_STATS,
	(IOVF_GET_UP | IOVF_MFG), 0, IOVT_BUFFER, sizeof(wl_pkteng_stats_t)},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
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
		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GH):
		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GHH):
			err = phy_rssi_set_gain_delta_2g(pi->rssii, aid, setValues);
			break;

		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2G):
		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2GH):
		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2GHH):
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
#if defined(BCMINTERNAL) || defined(WLTEST)
		case IOV_GVAL(IOV_PKTENG_STATS):
			err = wlc_phy_pkteng_stats_get(pi->rssii, a, alen);
			break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#ifdef WLTEST
		case IOV_SVAL(IOV_PHY_RSSI_CAL_FREQ_GRP_2G):
			err = phy_rssi_set_cal_freq_2g(pi->rssii, setValues);
			break;

		case IOV_GVAL(IOV_PHY_RSSI_CAL_FREQ_GRP_2G):
			err = phy_rssi_get_cal_freq_2g(pi->rssii, getValues);
			break;

		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB0):
		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB1):
		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB2):
		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB3):
		case IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB4):
			err = phy_rssi_set_gain_delta_2gb(pi->rssii, aid, setValues);
			break;

		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB0):
		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB1):
		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB2):
		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB3):
		case IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB4):
			err = phy_rssi_get_gain_delta_2gb(pi->rssii, aid, getValues);
			break;
#endif /* WLTEST */
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
