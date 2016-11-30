/*
 * BlueToothCoExistence module implementation - iovar handlers & registration
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
 * $Id: phy_btcx_iov.c 642720 2016-06-09 18:56:12Z vyass $
 */

#include <phy_btcx_iov.h>
#include <phy_btcx.h>
#include <wlc_iocv_reg.h>
#include <wlc_phy_int.h>

/* iovar ids */
enum {
	IOV_PHY_BTC_RESTAGE_RXGAIN = 1,
	IOV_PHY_LTECX_MODE = 2,
	IOV_PHY_BTC_PREEMPT_STATUS = 3,
	IOV_PHY_BTCOEX_DESENSE = 4
};

static const bcm_iovar_t phy_btcx_iovars[] = {
	{"phy_btc_restage_rxgain", IOV_PHY_BTC_RESTAGE_RXGAIN, IOVF_SET_UP, 0, IOVT_UINT32, 0},
#if (!defined(WLC_DISABLE_ACI) && defined(BCMLTECOEX) && defined(BCMINTERNAL))
	{"phy_ltecx_mode", IOV_PHY_LTECX_MODE, IOVF_SET_UP, 0, IOVT_INT32, 0},
#endif /* !defined (WLC_DISABLE_ACI) && defined (BCMLTECOEX) && defined (BCMINTERNAL) */
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"phy_btc_preempt_status", IOV_PHY_BTC_PREEMPT_STATUS, 0, 0, IOVT_INT8, 0},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if !defined(WLC_DISABLE_ACI) && defined(BCMDBG)
	{"phy_btcoex_desense", IOV_PHY_BTCOEX_DESENSE, IOVF_SET_UP, 0, IOVT_INT32, 0},
#endif /* !defined(WLC_DISABLE_ACI) && defined(BCMDBG) */
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_btcx_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int32 int_val = 0;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	BCM_REFERENCE(pi);
	BCM_REFERENCE(ret_int_ptr);

	switch (aid) {
	case IOV_GVAL(IOV_PHY_BTC_RESTAGE_RXGAIN):
		err = wlc_phy_iovar_get_btc_restage_rxgain(pi->btcxi, ret_int_ptr);
		break;
	case IOV_SVAL(IOV_PHY_BTC_RESTAGE_RXGAIN):
		err = wlc_phy_iovar_set_btc_restage_rxgain(pi->btcxi, int_val);
		break;
#if defined(BCMINTERNAL) || defined(WLTEST)
	case IOV_GVAL(IOV_PHY_BTC_PREEMPT_STATUS):
		err = phy_btcx_get_preemptstatus(pi, ret_int_ptr);
		break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if (!defined(WLC_DISABLE_ACI) && defined(BCMLTECOEX) && defined(BCMINTERNAL))
	case IOV_SVAL(IOV_PHY_LTECX_MODE):
		err = phy_btcx_desense_ltecx(pi, int_val);
		break;
#endif /* !defined (WLC_DISABLE_ACI) && defined (BCMLTECOEX) && defined (BCMINTERNAL) */
#if !defined(WLC_DISABLE_ACI) && defined(BCMDBG)
	case IOV_SVAL(IOV_PHY_BTCOEX_DESENSE):
	{
		err = phy_btcx_desense_btc(pi, int_val);
		break;
	}
#endif /* !defined(WLC_DISABLE_ACI) && defined(BCMDBG) */
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_btcx_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_btcx_iovars,
	                   NULL, NULL,
	                   phy_btcx_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
