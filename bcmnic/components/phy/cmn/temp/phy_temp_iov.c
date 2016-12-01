/*
 * TEMPsense module implementation - iovar table/handlers & registration
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
 * $Id: phy_temp_iov.c 642720 2016-06-09 18:56:12Z vyass $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <phy_api.h>
#include <phy_temp.h>
#include "phy_temp_st.h"
#include <phy_temp_iov.h>

#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>

/* iovar ids */
enum {
	IOV_PHY_TEMPTHRESH = 1,
	IOV_PHY_TEMP_HYSTERESIS = 2,
	IOV_PHY_TEMPOFFSET = 3,
	IOV_PHY_TEMPSENSE_OVERRIDE = 4
};

/* iovar table */
static const bcm_iovar_t phy_temp_iovars[] = {
	{"phy_tempthresh", IOV_PHY_TEMPTHRESH, 0, 0, IOVT_INT16, 0},
	{"phy_temp_hysteresis", IOV_PHY_TEMP_HYSTERESIS, 0, 0, IOVT_UINT8, 0},
#if defined(BCMDBG) || defined(WLTEST) || defined(MACOSX)
	{"phy_tempoffset", IOV_PHY_TEMPOFFSET, 0, 0, IOVT_INT8, 0},
	{"phy_tempsense_override", IOV_PHY_TEMPSENSE_OVERRIDE, 0, 0, IOVT_UINT16, 0},
#endif /* BCMDBG || WLTEST || MACOSX */
	{NULL, 0, 0, 0, 0, 0}
};

#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>
#endif

#include <wlc_patch.h>

static int
phy_temp_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	phy_temp_info_t *ti = pi->tempi;
	phy_txcore_temp_t *temp = phy_temp_get_st(ti);
	int err = BCME_OK;
	int32 *ret_int_ptr;
	int int_val = 0;
	BCM_REFERENCE(alen);
	BCM_REFERENCE(vsize);

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)a;

	(void)temp;
	(void)ret_int_ptr;

	switch (aid) {
#if defined(BCMDBG) || defined(WLTEST) || defined(DUTY_CYCLE_THROTTLING)
	case IOV_GVAL(IOV_PHY_TEMPTHRESH):
		*ret_int_ptr = (int32) temp->disable_temp;
		break;

	case IOV_SVAL(IOV_PHY_TEMPTHRESH):
		temp->disable_temp = (uint8) int_val;
		temp->enable_temp = temp->disable_temp - temp->hysteresis;
		break;
#endif /* defined(BCMDBG) || defined(WLTEST) || defined(DUTY_CYCLE_THROTTLING) */
#if defined(BCMDBG) || defined(WLTEST)
	case IOV_GVAL(IOV_PHY_TEMPOFFSET):
		*ret_int_ptr = (int32) pi->phy_tempsense_offset;
		break;

	case IOV_SVAL(IOV_PHY_TEMPOFFSET):
		pi->phy_tempsense_offset = (int8) int_val;
		break;

	case IOV_GVAL(IOV_PHY_TEMPSENSE_OVERRIDE):
		*ret_int_ptr = (int32)pi->tempsense_override;
		break;

	case IOV_SVAL(IOV_PHY_TEMPSENSE_OVERRIDE):
		pi->tempsense_override = (uint16)int_val;
		break;

	case IOV_GVAL(IOV_PHY_TEMP_HYSTERESIS):
		*ret_int_ptr = (int32)temp->hysteresis;
		break;

	case IOV_SVAL(IOV_PHY_TEMP_HYSTERESIS):
		temp->hysteresis = (uint8)int_val;
		temp->enable_temp = temp->disable_temp - temp->hysteresis;
		break;

#endif /* BCMDBG || WLTEST */

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_temp_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_temp_iovars,
	                   NULL, NULL,
	                   phy_temp_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
