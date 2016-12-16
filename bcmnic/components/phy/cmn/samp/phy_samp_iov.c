/*
 * CMN SampCollect module iovars
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: phy_samp_iov.c 655756 2016-08-23 12:40:40Z $
 */
#include <wlc_iocv_reg.h>
#include <wlc_phy_int.h>
#ifdef IQPLAY_DEBUG
#include <phy_samp_iov.h>
#endif /* IQPLAY_DEBUG */
#include <phy_ac_samp.h>

enum {
	IOV_PHY_PLAY_IQSAMPLES = 1
};

static const bcm_iovar_t phy_samp_iovars[] = {
#ifdef IQPLAY_DEBUG
	{"phy_sample_play", IOV_PHY_PLAY_IQSAMPLES,
	(IOVF_OPEN_ALLOW), 0, IOVT_BUFFER, sizeof(uint32),
	},
#endif /* IQPLAY_DEBUG */
	{NULL, 0, 0, 0, 0, 0 }
};

#include <wlc_patch.h>

#ifdef IQPLAY_DEBUG
static int
phy_samp_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	int err = BCME_OK;
	switch (aid) {
		case IOV_GVAL(IOV_PHY_PLAY_IQSAMPLES): {
			phy_info_t *pi = (phy_info_t *)ctx;
			err = phy_samp_prep_IQplay(pi);
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
BCMATTACHFN(phy_samp_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_samp_iovars,
		NULL, NULL,
		phy_samp_doiovar, disp_fn, patch_table, pi,
		&iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
#endif /* IQPLAY_DEBUG */
