/*
 * DeepSleepInit module implementation - iovar table/handlers & registration
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
 * $Id: phy_dsi_iov.c 658512 2016-09-08 07:03:22Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <phy_api.h>
#include <phy_dsi.h>
#include <phy_dsi_iov.h>

#include "phy_dsi_st.h"

#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>

/* iovar table */
enum {
	IOV_PHY_DSI_TRIGGER = 1,
	IOV_PHY_DSI_SAVE = 2
};

static const bcm_iovar_t phy_dsi_iovars[] = {
	{"dsi_trigger", IOV_PHY_DSI_TRIGGER,
	IOVF_SET_DOWN, 0, IOVT_BOOL, 0
	},
	{"dsi_save", IOV_PHY_DSI_SAVE,
	IOVF_SET_UP, 0, IOVT_INT8, 0
	},
	/* terminating element, only add new before this */
	{NULL, 0, 0, 0, 0, 0}
};

#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>
#endif

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

static int
phy_dsi_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	int32 *ret_int_ptr;

	int int_val = 0;
	int err = BCME_OK;

	phy_info_t *pi = (phy_info_t *)ctx;
	phy_dsi_info_t *di = pi->dsii;
	phy_dsi_state_t *st = phy_dsi_get_st(di);

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)a;

	(void)st;
	(void)ret_int_ptr;

	switch (aid) {
		case IOV_GVAL(IOV_PHY_DSI_TRIGGER): {
			*ret_int_ptr = (uint32)st->trigger;
			break;
		}
		case IOV_SVAL(IOV_PHY_DSI_TRIGGER): {
			st->trigger = (bool)int_val;
			break;
		}
		case IOV_GVAL(IOV_PHY_DSI_SAVE): {
			*ret_int_ptr = (int)phy_dsi_save(pi);
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
BCMATTACHFN(phy_dsi_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_dsi_iovars,
	                   NULL, NULL,
	                   phy_dsi_doiovar, NULL, NULL, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
