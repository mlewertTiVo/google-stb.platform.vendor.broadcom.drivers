/*
 * PHY and RADIO specific portion of Broadcom BCM43XX 802.11abg
 * PHY ioctl processing of Broadcom BCM43XX 802.11abg
 * Networking Device Driver.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

/*
 * This file contains high portion PHY ioctl processing and table.
 */

#include <wlc_cfg.h>

#ifdef WLC_HIGH
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wl_dbg.h>
#include <wlc.h>
#include <bcmwifi_channels.h>

static const wlc_ioctl_cmd_t phy_ioctls[] = {
	{WLC_RESTART, 0, 0},
	{WLC_GET_INTERFERENCE_MODE, 0, 0},
	{WLC_SET_INTERFERENCE_MODE, 0, 0},
	{WLC_GET_INTERFERENCE_OVERRIDE_MODE, 0, 0},
	{WLC_SET_INTERFERENCE_OVERRIDE_MODE, 0, 0},
};

static int
phy_legacy_vld_proc(wlc_info_t *wlc, uint16 cmd, void *arg, uint len)
{
	int bcmerror = BCME_OK;


	switch (cmd) {



	}

	return bcmerror;
}

#ifdef WLC_HIGH_ONLY
#include <bcm_xdr.h>

static bool
phy_legacy_cmd_proc(wlc_info_t *wlc, uint16 cmd, void *arg, uint len, bcm_xdr_buf_t *b)
{
	switch (cmd) {
	}

	return FALSE;
}

static bool
phy_legacy_result_proc(wlc_info_t *wlc, uint16 cmd, bcm_xdr_buf_t *b, void *arg, uint len)
{
	if ((cmd == WLC_EVM) || (cmd == WLC_FREQ_ACCURACY) ||
	    (cmd == WLC_LONGTRAIN) || (cmd == WLC_CARRIER_SUPPRESS)) {
		if (arg != NULL)
			wlc->pub->phytest_on = !!(*(uint32 *)arg);
	}

	return FALSE;
}
#endif /* WLC_HIGH_ONLY */
#endif /* WLC_HIGH */

#ifdef WLC_LOW
#include <typedefs.h>
#include <wlc_phy_int.h>

static int
phy_legacy_doioctl(void *ctx, uint16 cmd, void *arg, uint len, bool *ta_ok)
{
	return wlc_phy_ioctl_dispatch((phy_info_t *)ctx, (int)cmd, (int)len, arg, ta_ok);
}
#endif /* WLC_LOW */

/* register ioctl table to the system */
#include <phy_api.h>

#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>

int phy_legacy_register_ioct(phy_info_t *pi, wlc_iocv_info_t *ii);

int
BCMATTACHFN(phy_legacy_register_ioct)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_ioct_desc_t iocd;

	ASSERT(ii != NULL);

	wlc_iocv_init_iocd(phy_ioctls, ARRAYSIZE(phy_ioctls),
	                   phy_legacy_vld_proc,
	                   phy_legacy_cmd_proc, phy_legacy_result_proc,
	                   phy_legacy_doioctl, pi,
	                   &iocd);

	return wlc_iocv_register_ioct(ii, &iocd);
}
