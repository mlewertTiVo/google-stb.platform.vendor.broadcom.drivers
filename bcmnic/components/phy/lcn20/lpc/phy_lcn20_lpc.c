/*
 * LCN20PHY Link Power Control module implementation
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
 * $Id: phy_lcn20_lpc.c $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_lpc.h"
#include <phy_lcn20.h>
#include <phy_lcn20_lpc.h>

#define LPC_MIN_IDX 19

/* module private states */
struct phy_lcn20_lpc_info {
	phy_info_t *pi;
	phy_lcn20_info_t *ni;
	phy_lpc_info_t *cmn_info;
};

/* local functions */
static uint8 wlc_lcn20phy_lpc_getminidx(void);
static void wlc_lcn20phy_lpc_setmode(phy_type_lpc_ctx_t *ctx, bool enable);
static uint8 wlc_lcn20phy_lpc_getoffset(uint8 index);
static uint8 wlc_lcn20phy_lpc_get_txcpwrval(uint16 phytxctrlword);
static void wlc_lcn20phy_lpc_set_txcpwrval(uint16 *phytxctrlword, uint8 txcpwrval);
#ifdef WL_LPC_DEBUG
static uint8 * wlc_lcn20phy_lpc_get_pwrlevelptr(void);
#endif /* WL_LPC_DEBUG */

/* register phy type specific implementation */
phy_lcn20_lpc_info_t *
BCMATTACHFN(phy_lcn20_lpc_register_impl)(phy_info_t *pi, phy_lcn20_info_t *ni, phy_lpc_info_t *cmn_info)
{
	phy_lcn20_lpc_info_t *lcn20_info;
	phy_type_lpc_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((lcn20_info = phy_malloc(pi, sizeof(phy_lcn20_lpc_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	lcn20_info->pi = pi;
	lcn20_info->ni = ni;
	lcn20_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = lcn20_info;
	fns.getminidx = wlc_lcn20phy_lpc_getminidx;
	fns.getpwros = wlc_lcn20phy_lpc_getoffset;
	fns.gettxcpwrval = wlc_lcn20phy_lpc_get_txcpwrval;
	fns.settxcpwrval = wlc_lcn20phy_lpc_set_txcpwrval;
	fns.setmode = wlc_lcn20phy_lpc_setmode;
#ifdef WL_LPC_DEBUG
	fns.getpwrlevelptr = wlc_lcn20phy_lpc_get_pwrlevelptr;
#endif

	if (phy_lpc_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_lpc_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return lcn20_info;

	/* error handling */
fail:
	if (lcn20_info != NULL)
		phy_mfree(pi, lcn20_info, sizeof(phy_lcn20_lpc_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn20_lpc_unregister_impl)(phy_lcn20_lpc_info_t *lcn20_info)
{
	phy_info_t *pi;
	phy_lpc_info_t *cmn_info;

	ASSERT(lcn20_info);
	pi = lcn20_info->pi;
	cmn_info = lcn20_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_lpc_unregister_impl(cmn_info);

	phy_mfree(pi, lcn20_info, sizeof(phy_lcn20_lpc_info_t));
}

/* ********************************************* */
/*				Callback Functions Table					*/
/* ********************************************* */
uint8
wlc_lcn20phy_lpc_getminidx(void)
{
	return LPC_MIN_IDX;
}

void
wlc_lcn20phy_lpc_setmode(phy_type_lpc_ctx_t *ctx, bool enable)
{
	PHY_INFORM(("\n lpc mode set to : %d\n", enable));
}

uint8
wlc_lcn20phy_lpc_getoffset(uint8 index)
{
	return index;
}

uint8
wlc_lcn20phy_lpc_get_txcpwrval(uint16 phytxctrlword)
{
	return (phytxctrlword & PHY_TXC_PWR_MASK) >> PHY_TXC_PWR_SHIFT;
}

void
wlc_lcn20phy_lpc_set_txcpwrval(uint16 *phytxctrlword, uint8 txcpwrval)
{
	*phytxctrlword = (*phytxctrlword & ~PHY_TXC_PWR_MASK) |
		(txcpwrval << PHY_TXC_PWR_SHIFT);
	return;
}
#ifdef WL_LPC_DEBUG
uint8 *
wlc_lcn20phy_lpc_get_pwrlevelptr(void)
{
	return lpc_pwr_level;
}
#endif

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */

/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */
