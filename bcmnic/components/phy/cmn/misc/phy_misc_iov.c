/*
 * Miscellaneous module implementation - iovar table/handlers & registration
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
 * $Id: phy_misc_iov.c 643558 2016-06-15 05:34:16Z changbo $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <phy_api.h>
#include <phy_misc.h>
#include <phy_misc_iov.h>

#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>

/* iovar table */

static const bcm_iovar_t phy_misc_iovars[] = {
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG) || defined(ATE_BUILD)
	{"phy_tx_tone", IOV_PHY_TX_TONE,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT32, 0
	},
	{"phy_txlo_tone", IOV_PHY_TXLO_TONE,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phy_rxiqest", IOV_PHY_RXIQ_EST,
	IOVF_SET_UP, 0, IOVT_UINT32, IOVT_UINT32
	},
#endif /* BCMINTERNAL || WLTEST || WFD_PHY_LL_DEBUG || ATE_BUILD */
	{"phy_txswctrlmap", IOV_PHY_TXSWCTRLMAP,
	0, 0, IOVT_INT8, 0
	},
	{NULL, 0, 0, 0, 0, 0}
};

#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>
#endif

static int
phy_misc_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int err = BCME_OK;
	int32 *ret_int_ptr;
	int int_val = 0;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)a;

	(void)ret_int_ptr;

	switch (aid) {
	case IOV_GVAL(IOV_PHY_RXIQ_EST):
		err = wlc_phy_iovar_get_rx_iq_est(pi, ret_int_ptr, int_val, err);
		break;

	case IOV_SVAL(IOV_PHY_RXIQ_EST):
	{
		err = wlc_phy_iovar_set_rx_iq_est(pi, int_val, err);
		break;
	}

#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG) || defined(ATE_BUILD)
	case IOV_GVAL(IOV_PHY_TX_TONE):
	case IOV_GVAL(IOV_PHY_TXLO_TONE):
		*ret_int_ptr = pi->phy_tx_tone_freq;
		break;

	case IOV_SVAL(IOV_PHY_TX_TONE):
		wlc_phy_iovar_tx_tone(pi, (int32)int_val);
		break;

	case IOV_SVAL(IOV_PHY_TXLO_TONE):
		wlc_phy_iovar_txlo_tone(pi);
		break;
#endif /* BCMINTERNAL || WLTEST || WFD_PHY_LL_DEBUG || ATE_BUILD */
	case IOV_GVAL(IOV_PHY_TXSWCTRLMAP): {
		err = phy_misc_txswctrlmapget(pi, ret_int_ptr);
		break;
	}
	case IOV_SVAL(IOV_PHY_TXSWCTRLMAP): {
		err = phy_misc_txswctrlmapset(pi, int_val);
		break;
	}
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_misc_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_misc_iovars,
	                   NULL, NULL,
	                   phy_misc_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
