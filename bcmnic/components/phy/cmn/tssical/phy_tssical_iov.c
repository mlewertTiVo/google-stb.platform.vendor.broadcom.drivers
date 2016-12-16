/*
 * TSSICAL module implementation - iovar handlers & registration
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
 * $Id: phy_tssical_iov.c 666527 2016-10-21 18:26:20Z $
 */

#include <phy_tssical_iov.h>
#include <phy_tssical_api.h>
#include <wlc_iocv_reg.h>
#include <phy_tssical.h>
#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>
#endif
#include <phy_type_tssical.h>

static const bcm_iovar_t phy_tssical_iovars[] = {

#ifdef WLC_TXCAL
	{"phy_adj_tssi", IOV_PHY_ADJUSTED_TSSI,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0
	},
	{"txcal_ver", IOV_PHY_TXCALVER,
	(IOVF_MFG), 0, IOVT_INT32,	0
	},
	{"txcal_pwr_tssi_tbl", IOV_PHY_TXCAL_PWR_TSSI_TBL,
	(0), 0, IOVT_BUFFER, OFFSETOF(wl_txcal_power_tssi_ncore_t, tssi_percore) +
	PHY_CORE_MAX * sizeof(wl_txcal_power_tssi_percore_t)
	},
	{"txcal_apply_pwr_tssi_tbl", IOV_PHY_TXCAL_APPLY_PWR_TSSI_TBL,
	(0), 0, IOVT_UINT32, 0
	},
	{"phy_read_estpwrlut", IOV_PHY_READ_ESTPWR_LUT,
	(IOVF_GET_UP | IOVF_MFG), 0, IOVT_BUFFER, MAX_NUM_TXCAL_MEAS * sizeof(uint16)
	},
	{"olpc_anchoridx", IOV_OLPC_ANCHOR_IDX,
	(IOVF_MFG), 0, IOVT_BUFFER, sizeof(wl_olpc_pwr_t)
	},
	{"olpc_idx_valid", IOV_OLPC_IDX_VALID,
	(IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"txcal_status", IOV_PHY_TXCAL_STATUS,
	(IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"olpc_anchor2g", IOV_OLPC_ANCHOR_2G,
	0, 0, IOVT_INT8, 0
	},
	{"olpc_anchor5g", IOV_OLPC_ANCHOR_5G,
	0, 0, IOVT_INT8, 0
	},
	{"olpc_thresh", IOV_OLPC_THRESH,
	0, 0, IOVT_INT8, 0
	},
	{"disable_olpc", IOV_DISABLE_OLPC,
	0, 0, IOVT_INT8, 0
	},
#endif /* WLC_TXCAL */

	{NULL, 0, 0, 0, 0, 0}
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

static int
phy_tssical_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	int err = BCME_OK;
	phy_info_t *pi = (phy_info_t *)ctx;
	bool bool_val = FALSE;
	int int_val = 0;
	int32 *ret_int_ptr = (int32 *)a;
	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));
	/* bool conversion to avoid duplication below */
	bool_val = int_val != 0;
	(void)pi;
	(void)bool_val;
	(void)ret_int_ptr;

	switch (aid) {

#ifdef WLC_TXCAL
	case IOV_GVAL(IOV_OLPC_ANCHOR_2G):
		int_val = phy_tssical_get_olpc_anchor2g(pi->tssicali);
		bcopy(&int_val, a, sizeof(int_val));
		break;
	case IOV_SVAL(IOV_OLPC_ANCHOR_2G):
		phy_tssical_set_olpc_anchor2g(pi->tssicali, (int8)int_val);
		break;
	case IOV_GVAL(IOV_OLPC_ANCHOR_5G):
		int_val = phy_tssical_get_olpc_anchor5g(pi->tssicali);
		bcopy(&int_val, a, sizeof(int_val));
		break;
	case IOV_SVAL(IOV_OLPC_ANCHOR_5G):
		phy_tssical_set_olpc_anchor5g(pi->tssicali, (int8)int_val);
		break;
	case IOV_GVAL(IOV_OLPC_THRESH):
		int_val = phy_tssical_get_olpc_threshold(pi->tssicali);
		bcopy(&int_val, a, sizeof(int_val));
		break;
	case IOV_SVAL(IOV_OLPC_THRESH):
		phy_tssical_iov_set_olpc_threshold(pi->tssicali, (int8)int_val);
		break;
	case IOV_GVAL(IOV_DISABLE_OLPC):
		int_val = phy_tssical_get_disable_olpc(pi->tssicali);
		bcopy(&int_val, a, sizeof(int_val));
		break;
	case IOV_SVAL(IOV_DISABLE_OLPC):
		phy_tssical_set_disable_olpc(pi->tssicali, (int8)int_val);
		break;
	case IOV_GVAL(IOV_OLPC_OFFSET):
		phy_tssical_get_set_olpc_offset(pi->tssicali, a, 0);
		break;
	case IOV_SVAL(IOV_OLPC_OFFSET):
		phy_tssical_get_set_olpc_offset(pi->tssicali, p, 1);
		break;
	case IOV_GVAL(IOV_OLPC_IDX_VALID):
		int_val = phy_tssical_get_olpc_idx_valid(pi->tssicali);
		bcopy(&int_val, a, sizeof(int_val));
		break;
	case IOV_GVAL(IOV_PHY_ADJUSTED_TSSI):
		if (!pi->sh->clk)
			err = BCME_NOCLK;
		else
			err = phy_tssical_iovar_adjusted_tssi(pi->tssicali, ret_int_ptr,
				(uint8) int_val);
		break;

	case IOV_GVAL(IOV_PHY_TXCALVER):
	{
		*ret_int_ptr = TXCAL_IOVAR_VERSION;
		break;
	}
	case IOV_SVAL(IOV_PHY_TXCAL_PWR_TSSI_TBL):
	{
		if ((err = phy_tssical_set_pwr_tssi_tbl(pi->tssicali, p)) != BCME_OK)
			break;
		/* apply new values */
		err = phy_tssical_store_pwr_tssi_tbl(pi->tssicali);
		break;
	}
	case IOV_GVAL(IOV_PHY_TXCAL_PWR_TSSI_TBL):
	{
		wl_txcal_power_tssi_ncore_t * txcal_tssi;
		uint16 buf_size = OFFSETOF(wl_txcal_power_tssi_ncore_t, tssi_percore) +
			PHY_CORE_MAX * sizeof(wl_txcal_power_tssi_percore_t);
		txcal_tssi = (wl_txcal_power_tssi_ncore_t *)p;
		/* check for txcal version */
		if (txcal_tssi->version != TXCAL_IOVAR_VERSION) {
			err = BCME_VERSION;
			break;
		}
		phy_tssical_get_pwr_tssi_tbl(pi->tssicali, (uint8)txcal_tssi->channel);
		if (alen < buf_size)
			return BCME_BADLEN;
		txcal_tssi = (wl_txcal_power_tssi_ncore_t *)a;
		phy_tssical_copy_get_pwr_tssi_tbl(pi->tssicali, txcal_tssi);
		break;
	}
	case IOV_GVAL(IOV_OLPC_ANCHOR_IDX):
	{
		err = phy_tssical_get_olpc_pwr(pi->tssicali, p, a);
		break;
	}
	case IOV_SVAL(IOV_OLPC_ANCHOR_IDX):
	{
		err = phy_tssical_set_olpc_pwr(pi->tssicali, p, a);
		break;
	}
	case IOV_SVAL(IOV_PHY_TXCAL_APPLY_PWR_TSSI_TBL):
	{
		err = phy_tssical_iov_apply_pwr_tssi_tbl(pi->tssicali, int_val);
		break;
	}
	case IOV_GVAL(IOV_PHY_TXCAL_APPLY_PWR_TSSI_TBL):
	{
		int_val = phy_tssical_get_pwr_tssi_tbl_in_use(pi->tssicali);
		break;
	}
	case IOV_GVAL(IOV_PHY_READ_ESTPWR_LUT):
	{
		err = phy_tssical_iov_read_est_pwr_lut(pi->tssicali, a, int_val);
		break;
	}
	case IOV_GVAL(IOV_PHY_TXCAL_STATUS):
	{
		if (!pi->sh->up) {
			err = BCME_NOTUP;
			break;
		} else {
			*ret_int_ptr = (int32) phy_tssical_get_txcal_status(pi->tssicali);
		}
		break;
	}
#endif /* WLC_TXCAL */

	default:
#if defined(WLC_TXCAL)
		err = BCME_UNSUPPORTED;
#else
		err = BCME_OK;
#endif
		break;
	}
	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_tssical_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_tssical_iovars,
	                   NULL, NULL,
	                   phy_tssical_doiovar, NULL, NULL, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
