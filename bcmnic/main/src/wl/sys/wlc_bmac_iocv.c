/*
 * BMAC iovar table and registration
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


#include <wlc_cfg.h>
#include <typedefs.h>

#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlioctl.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_bmac.h>

#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>
#include <wlc_bmac_iocv.h>

#ifdef WLC_HIGH
/* iovar table */
static const bcm_iovar_t
wlc_bmac_iovt[] = {
#ifdef WLDIAG
	{"diag", IOV_BMAC_DIAG, 0, IOVT_UINT32, 0},
#endif /* WLDIAG */
#ifdef WLLED
	{"gpiotimerval", IOV_BMAC_SBGPIOTIMERVAL, 0, IOVT_UINT32, sizeof(uint32)},
	{"leddc", IOV_BMAC_LEDDC, IOVF_OPEN_ALLOW, IOVT_UINT32, sizeof(uint32)},
#endif /* WLLED */
	{"wpsgpio", IOV_BMAC_WPSGPIO, 0, IOVT_UINT32, 0},
	{"wpsled", IOV_BMAC_WPSLED, 0, IOVT_UINT32, 0},
	{"btclock_tune_war", IOV_BMAC_BTCLOCK_TUNE_WAR, 0, IOVT_UINT32, 0},
	{"ccgpioin", IOV_BMAC_CCGPIOIN, 0, IOVT_UINT32, 0},
	{"bt_regs_read",	IOV_BMAC_BT_REGS_READ, 0, IOVT_BUFFER, 0},
	/* || defined (BCMINTERNAL)) */
	{"aspm", IOV_BMAC_PCIEASPM, 0, IOVT_INT16, 0},
	{"correrrmask", IOV_BMAC_PCIEADVCORRMASK, 0, IOVT_INT16, 0},
	{"pciereg", IOV_BMAC_PCIEREG, 0, IOVT_BUFFER, 0},
	{"pcieserdesreg", IOV_BMAC_PCIESERDESREG, 0, IOVT_BUFFER, 0},
	{"srom", IOV_BMAC_SROM, 0, IOVT_BUFFER, 0},
	{"customvar1", IOV_BMAC_CUSTOMVAR1, 0, IOVT_UINT32, 0},
	{"generic_dload", IOV_BMAC_GENERIC_DLOAD, 0, IOVT_BUFFER, 0},
	{"noise_metric", IOV_BMAC_NOISE_METRIC, 0, IOVT_UINT16, 0},
	{"avoidance_cnt", IOV_BMAC_AVIODCNT, 0, IOVT_UINT32, 0},
	{"btswitch", IOV_BMAC_BTSWITCH,	0, IOVT_INT32, 0},
	{"vcofreq_pcie2", IOV_BMAC_4360_PCIE2_WAR, IOVF_SET_DOWN, IOVT_INT32, 0},
	{"edcrs", IOV_BMAC_EDCRS, IOVF_SET_UP | IOVF_GET_UP, IOVT_UINT8, 0},
	{NULL, 0, 0, 0, 0}
};
#endif /* WLC_HIGH */

#ifdef WLC_HIGH_ONLY
/* fixup callbacks */
static bool
wlc_bmac_pack_iov(wlc_info_t *wlc, uint32 aid, void *p, uint p_len, bcm_xdr_buf_t *b)
{
	int err;

	BCM_REFERENCE(err);

	/* Decide the buffer is 16-bit or 32-bit buffer */
	switch (IOV_ID(aid)) {
	case IOV_BMAC_SBGPIOOUT:
	case IOV_BMAC_CCREG:
	case IOV_BMAC_PCIEREG:
	case IOV_BMAC_PCICFGREG:
	case IOV_BMAC_PCIESERDESREG:
		p_len &= ~3;
		err = bcm_xdr_pack_uint32(b, p_len);
		ASSERT(!err);
		err = bcm_xdr_pack_uint32_vec(b, p_len, p);
		ASSERT(!err);
		return TRUE;
	}
	return FALSE;
}
#endif /* WLC_HIGH_ONLY */

#ifdef WLC_LOW
static int
wlc_bmac_doiovar(void *ctx, uint32 aid, void *p, uint plen, void *a, uint alen, uint vsize)
{
	return wlc_bmac_iovar_dispatch((wlc_hw_info_t *)ctx, aid,
	                               p, plen, a, (int)alen, (int)vsize);
}
#endif /* WLC_LOW */

/* register iovar table/handlers to the system */
int
wlc_bmac_register_iovt(wlc_hw_info_t *hw, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(wlc_bmac_iovt,
	                   wlc_bmac_pack_iov, NULL,
	                   wlc_bmac_doiovar, hw,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
