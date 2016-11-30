/*
 * Rx Gain Control and Carrier Sense module implementation - iovar table
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
 * $Id: phy_rxgcrs_iov.c 644994 2016-06-22 06:23:44Z vyass $
 */

#include <phy_rxgcrs.h>
#include <phy_rxgcrs_iov.h>
#include <phy_rxgcrs_api.h>
#include <phy_rxgcrs.h>
#include <wlc_iocv_reg.h>
#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>
#endif

/* iovar ids */
enum {
	IOV_ED_THRESH = 1,
	IOV_PHY_RXDESENS = 2,
	IOV_PHY_FORCECAL_NOISE = 3
};

/* iovar table */
static const bcm_iovar_t phy_rxgcrs_iovars[] = {
	{"phy_ed_thresh", IOV_ED_THRESH, (IOVF_SET_UP | IOVF_GET_UP), 0, IOVT_INT32, 0},
#if defined(RXDESENS_EN)
	{"phy_rxdesens", IOV_PHY_RXDESENS, IOVF_GET_UP, 0, IOVT_INT32, 0},
#endif /* defined(RXDESENS_EN) */
#ifndef ATE_BUILD
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG)
	{"phy_forcecal_noise", IOV_PHY_FORCECAL_NOISE,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, sizeof(uint16)
	},
#endif /* BCMINTERNAL || WLTEST || DBG_PHY_IOV || WFD_PHY_LL_DEBUG */
#endif /* !ATE_BUILD */
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_rxgcrs_doiovar(void *ctx, uint32 aid, void *p, uint plen, void *a, uint alen, uint vsize,
	struct wlc_if *wlcif)
{
	int err = BCME_OK;
	phy_info_t *pi = (phy_info_t *)ctx;
	int int_val = 0;
	int32 *ret_int_ptr = (int32 *)a;

	switch (aid) {
	case IOV_SVAL(IOV_ED_THRESH):
		err = wlc_phy_adjust_ed_thres(pi, &int_val, TRUE);
		break;
	case IOV_GVAL(IOV_ED_THRESH):
		err = wlc_phy_adjust_ed_thres(pi, ret_int_ptr, FALSE);
		break;
#if defined(RXDESENS_EN)
	case IOV_GVAL(IOV_PHY_RXDESENS):
		err = phy_rxgcrs_get_rxdesens(pi, ret_int_ptr);
		break;

	case IOV_SVAL(IOV_PHY_RXDESENS):
		err = phy_rxgcrs_set_rxdesens(pi, int_val);
		break;
#endif /* defined(RXDESENS_EN) */
#ifndef ATE_BUILD
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG)
	/* JIRA:SWWLAN-32606, RB: 12975 */
	case IOV_GVAL(IOV_PHY_FORCECAL_NOISE): /* Get crsminpwr for core 0 & core 1 */
		err = wlc_phy_iovar_forcecal_noise(pi, a, FALSE);
		break;

	case IOV_SVAL(IOV_PHY_FORCECAL_NOISE): /* do only Noise Cal */
		err = wlc_phy_iovar_forcecal_noise(pi, a, TRUE);
		break;
#endif /* BCMINTERNAL || WLTEST || DBG_PHY_IOV || WFD_PHY_LL_DEBUG */
#endif /* !ATE_BUILD */
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_rxgcrs_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_rxgcrs_iovars,
	                   NULL, NULL,
	                   phy_rxgcrs_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
