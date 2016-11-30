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
 * $Id: phy_ac_tpc_iov.c 642720 2016-06-09 18:56:12Z vyass $
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
#if (defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD))
#if (defined(BCMINTERNAL) || defined(WLTEST))
	{"phy_txpwr_ovrinitbaseidx", IOV_OVRINITBASEIDX, (IOVF_SET_UP|IOVF_GET_UP), 0,
	IOVT_UINT8, 0},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
	{"phy_tone_txpwr", IOV_PHY_TONE_TXPWR, (IOVF_SET_UP), 0, IOVT_INT8, 0},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD) */
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_ac_tpc_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
#if (defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD))
	phy_info_t *pi = (phy_info_t *)ctx;
	int err = BCME_OK;
	int int_val = 0;
	int32 *ret_int_ptr = (int32 *)a;

	BCM_REFERENCE(*ret_int_ptr);

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
#if (defined(BCMINTERNAL) || defined(WLTEST))
	case IOV_GVAL(IOV_OVRINITBASEIDX):
		*ret_int_ptr = pi->tpci->data->ovrinitbaseidx;
		break;
	case IOV_SVAL(IOV_OVRINITBASEIDX):
		pi->tpci->data->ovrinitbaseidx = (bool)int_val;
		wlc_phy_txpwr_ovrinitbaseidx(pi);
		break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
	case IOV_SVAL(IOV_PHY_TONE_TXPWR):
		if (!pi->sh->clk) {
		   err = BCME_NOCLK;
		   break;
		}
		wlc_phy_tone_pwrctrl_loop(pi, (int8)int_val);
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}
	return err;
#else /* defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD) */
	return BCME_UNSUPPORTED;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD) */
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
