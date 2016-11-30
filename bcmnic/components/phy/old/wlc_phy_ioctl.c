/*
 * PHY and RADIO specific portion of Broadcom BCM43XX 802.11abg
 * PHY ioctl processing of Broadcom BCM43XX 802.11abg
 * Networking Device Driver.
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
 * $Id: wlc_phy_ioctl.c 614654 2016-01-22 21:50:32Z jqliu $
 */

/*
 * This file contains high portion PHY ioctl processing and table.
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmutils.h>
#include <wlioctl.h>

#include <wlc_iocv_types.h>
#include <wlc_iocv_desc.h>
#include <wlc_iocv_reg.h>

static const wlc_ioctl_cmd_t phy_ioctls[] = {
	{WLC_RESTART, 0, 0},
#if defined(BCMINTERNAL) || defined(BCMDBG) || defined(WLTEST)
	{WLC_GET_RADIOREG, WLC_IOCF_REG_CHECK, 0},
	{WLC_SET_RADIOREG, WLC_IOCF_REG_CHECK, 0},
#endif
#if defined(BCMDBG)
	{WLC_GET_TX_PATH_PWR, WLC_IOCF_BAND_CHECK_AUTO, 0},
	{WLC_SET_TX_PATH_PWR, WLC_IOCF_BAND_CHECK_AUTO, 0},
#endif
#if defined(BCMINTERNAL) || defined(BCMDBG) || defined(WLTEST) || \
	defined(WL_EXPORT_GET_PHYREG)
	{WLC_GET_PHYREG, WLC_IOCF_REG_CHECK, 0},
#endif
#if defined(BCMINTERNAL) || defined(BCMDBG) || defined(WLTEST)
	{WLC_SET_PHYREG, WLC_IOCF_REG_CHECK, 0},
#endif
#if defined(BCMDBG) || defined(WLTEST)
	{WLC_GET_TSSI, WLC_IOCF_REG_CHECK_AUTO, 0},
	{WLC_GET_ATTEN, WLC_IOCF_REG_CHECK_AUTO, 0},
	{WLC_SET_ATTEN, WLC_IOCF_BAND_CHECK_AUTO, 0},
	{WLC_GET_PWRIDX, WLC_IOCF_BAND_CHECK_AUTO, 0},
	{WLC_SET_PWRIDX, WLC_IOCF_BAND_CHECK_AUTO, 0},
	{WLC_LONGTRAIN, WLC_IOCF_REG_CHECK_AUTO, 0},
	{WLC_EVM, WLC_IOCF_REG_CHECK, 0},
	{WLC_FREQ_ACCURACY, WLC_IOCF_REG_CHECK, 0},
	{WLC_CARRIER_SUPPRESS, WLC_IOCF_REG_CHECK_AUTO, 0},
#endif /* BCMDBG || WLTEST  */
#ifdef BCMINTERNAL
	{WLC_GET_ACI_ARGS, 0, 0},
	{WLC_SET_ACI_ARGS, 0, 0},
#endif
	{WLC_GET_INTERFERENCE_MODE, 0, 0},
	{WLC_SET_INTERFERENCE_MODE, 0, 0},
	{WLC_GET_INTERFERENCE_OVERRIDE_MODE, 0, 0},
	{WLC_SET_INTERFERENCE_OVERRIDE_MODE, 0, 0},
};

#include <wlc_phy_shim.h>
#include <wlc_phy_int.h>

static int
phy_legacy_doioctl(void *ctx, uint32 cmd, void *arg, uint len, struct wlc_if *wlcif)
{
	phy_info_t *pi = ctx;
	bool ta = FALSE;
	int err = wlc_phy_ioctl_dispatch(pi, (int)cmd, (int)len, arg, &ta);
	wlapi_taclear(pi->sh->physhim, ta);
	return err;
}

/* register ioctl table to the system */
int phy_legacy_register_ioct(phy_info_t *pi, wlc_iocv_info_t *ii);

int
BCMATTACHFN(phy_legacy_register_ioct)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_ioct_desc_t iocd;

	ASSERT(ii != NULL);

	wlc_iocv_init_iocd(phy_ioctls, ARRAYSIZE(phy_ioctls),
	                   NULL,
	                   phy_legacy_cmd_proc, phy_legacy_result_proc,
	                   phy_legacy_doioctl, pi,
	                   &iocd);

	return wlc_iocv_register_ioct(ii, &iocd);
}
