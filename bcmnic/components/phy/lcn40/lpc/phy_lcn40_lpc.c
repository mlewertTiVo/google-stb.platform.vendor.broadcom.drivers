/*
 * LCN40PHY Link Power Control module implementation
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
 * $Id: phy_lcn40_lpc.c $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_lpc.h"
#include <phy_lcn40.h>
#include <phy_lcn40_lpc.h>

#define LCN40PHY_TX_PWR_CTRL_MAC_OFFSET 128
#define LPC_MIN_IDX 38

#define LPC_TOT_IDX (LPC_MIN_IDX + 1)
#define LCN40PHY_TX_PWR_CTRL_MACLUT_MAX_ENTRIES	64
#define LCN40PHY_TX_PWR_CTRL_MACLUT_WIDTH	8

static uint8 lpc_pwr_level[LPC_TOT_IDX] =
	{0, 2, 4, 6, 8, 10,
	12, 14, 16, 18, 20,
	22, 24, 26, 28, 30,
	32, 34, 36, 38, 40,
	42, 44, 46, 48, 50,
	52, 54, 56, 58, 60,
	61, 62, 63, 64, 65,
	66, 67, 68};

/* module private states */
struct phy_lcn40_lpc_info {
	phy_info_t *pi;
	phy_lcn40_info_t *ni;
	phy_lpc_info_t *cmn_info;
};

/* local functions */
static uint8 wlc_lcn40phy_lpc_getminidx(void);
static uint8 wlc_lcn40phy_lpc_getoffset(uint8 index);
static void phy_lcn40_lpc_setmode(phy_type_lpc_ctx_t *ctx, bool enable);
static uint8 wlc_lcn40phy_lpc_get_txcpwrval(uint16 phytxctrlword);
static void wlc_lcn40phy_lpc_set_txcpwrval(uint16 *phytxctrlword, uint8 txcpwrval);
#ifdef WL_LPC_DEBUG
static uint8 * wlc_lcn40phy_lpc_get_pwrlevelptr(void);
#endif

/* register phy type specific implementation */
phy_lcn40_lpc_info_t *
BCMATTACHFN(phy_lcn40_lpc_register_impl)(phy_info_t *pi, phy_lcn40_info_t *ni, phy_lpc_info_t *cmn_info)
{
	phy_lcn40_lpc_info_t *lcn40_info;
	phy_type_lpc_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((lcn40_info = phy_malloc(pi, sizeof(phy_lcn40_lpc_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	lcn40_info->pi = pi;
	lcn40_info->ni = ni;
	lcn40_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = lcn40_info;
	fns.getminidx = wlc_lcn40phy_lpc_getminidx;
	fns.getpwros = wlc_lcn40phy_lpc_getoffset;
	fns.gettxcpwrval = wlc_lcn40phy_lpc_get_txcpwrval;
	fns.settxcpwrval = wlc_lcn40phy_lpc_set_txcpwrval;
	fns.setmode = phy_lcn40_lpc_setmode;
#ifdef WL_LPC_DEBUG
	fns.getpwrlevelptr = wlc_lcn40phy_lpc_get_pwrlevelptr;
#endif

	if (phy_lpc_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_lpc_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return lcn40_info;

	/* error handling */
fail:
	if (lcn40_info != NULL)
		phy_mfree(pi, lcn40_info, sizeof(phy_lcn40_lpc_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn40_lpc_unregister_impl)(phy_lcn40_lpc_info_t *lcn40_info)
{
	phy_info_t *pi;
	phy_lpc_info_t *cmn_info;

	ASSERT(lcn40_info);
	pi = lcn40_info->pi;
	cmn_info = lcn40_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_lpc_unregister_impl(cmn_info);

	phy_mfree(pi, lcn40_info, sizeof(phy_lcn40_lpc_info_t));
}

/* ********************************************* */
/*				Callback Functions Table					*/
/* ********************************************* */
static uint8
wlc_lcn40phy_lpc_getminidx(void)
{
	return LPC_MIN_IDX;
}

static uint8
wlc_lcn40phy_lpc_getoffset(uint8 index)
{
	return index;
	/* return lpc_pwr_level[index]; for PHYs which expect the actual offset
	 * for example, HT 4331.
	 */
}

static void
phy_lcn40_lpc_setmode(phy_type_lpc_ctx_t *ctx, bool enable)
{
	phy_lcn40_lpc_info_t *info = (phy_lcn40_lpc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	BCM_REFERENCE(pi);
	wlc_lcn40phy_lpc_setmode(pi, enable);
}

static uint8
wlc_lcn40phy_lpc_get_txcpwrval(uint16 phytxctrlword)
{
	return (phytxctrlword & PHY_TXC_PWR_MASK) >> PHY_TXC_PWR_SHIFT;
}

static void
wlc_lcn40phy_lpc_set_txcpwrval(uint16 *phytxctrlword, uint8 txcpwrval)
{
	*phytxctrlword = (*phytxctrlword & ~PHY_TXC_PWR_MASK) |
		(txcpwrval << PHY_TXC_PWR_SHIFT);
	return;
}

#ifdef WL_LPC_DEBUG
static uint8 *
wlc_lcn40phy_lpc_get_pwrlevelptr(void)
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
void
wlc_lcn40phy_lpc_write_maclut(phy_info_t *pi)
{
	phytbl_info_t tab;

#if defined(LP_P2P_SOFTAP)
	uint8 i;
	/* Assign values from 0 to 63 qdB for now */
	for (i = 0; i < LCNPHY_TX_PWR_CTRL_MACLUT_LEN; i++)
		pwr_lvl_qdB[i] = i;
	tab.tbl_ptr = pwr_lvl_qdB;
	tab.tbl_len = LCNPHY_TX_PWR_CTRL_MACLUT_LEN;

#endif /* LP_P2P_SOFTAP */

	/* If not enabled, no need to clear out the table, just quit */
	if (!pi->lpc_algo)
		return;
	tab.tbl_ptr = lpc_pwr_level;
	tab.tbl_len = LPC_TOT_IDX;


	tab.tbl_id = LCN40PHY_TBL_ID_TXPWRCTL;
	tab.tbl_width = LCN40PHY_TX_PWR_CTRL_MACLUT_WIDTH;
	tab.tbl_offset = LCN40PHY_TX_PWR_CTRL_MAC_OFFSET;

	/* Write to it */
	wlc_lcn40phy_write_table(pi, &tab);
}

void
wlc_lcn40phy_lpc_setmode(phy_info_t *pi, bool enable)
{
	if (enable)
		wlc_lcn40phy_lpc_write_maclut(pi);
}
