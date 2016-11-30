/*
 * NPHY Link Power Control module implementation
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
 * $Id: phy_n_lpc.c $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_lpc.h"
#include <phy_n.h>
#include <phy_n_lpc.h>

/* module private states */
struct phy_n_lpc_info {
	phy_info_t *pi;
	phy_n_info_t *ni;
	phy_lpc_info_t *cmn_info;
};

/* local functions */
static uint8 wlc_phy_lpc_getminidx_nphy(void);
static uint8 wlc_phy_lpc_getoffset_nphy(uint8 index);
static uint8 wlc_phy_lpc_get_txcpwrval_nphy(uint16 phytxctrlword);
static void wlc_phy_lpc_setmode_nphy(phy_type_lpc_ctx_t *ctx, bool enable);
static void wlc_phy_lpc_set_txcpwrval_nphy(uint16 *phytxctrlword, uint8 txcpwrval);

/* register phy type specific implementation */
phy_n_lpc_info_t *
BCMATTACHFN(phy_n_lpc_register_impl)(phy_info_t *pi, phy_n_info_t *ni, phy_lpc_info_t *cmn_info)
{
	phy_n_lpc_info_t *n_info;
	phy_type_lpc_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((n_info = phy_malloc(pi, sizeof(phy_n_lpc_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	n_info->pi = pi;
	n_info->ni = ni;
	n_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = n_info;
	fns.setmode = wlc_phy_lpc_setmode_nphy;
	fns.getpwros = wlc_phy_lpc_getoffset_nphy;
	fns.getminidx = wlc_phy_lpc_getminidx_nphy;
	fns.gettxcpwrval = wlc_phy_lpc_get_txcpwrval_nphy;
	fns.settxcpwrval = wlc_phy_lpc_set_txcpwrval_nphy;
	if (phy_lpc_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_lpc_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return n_info;

	/* error handling */
fail:
	if (n_info != NULL)
		phy_mfree(pi, n_info, sizeof(phy_n_lpc_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_n_lpc_unregister_impl)(phy_n_lpc_info_t *n_info)
{
	phy_info_t *pi;
	phy_lpc_info_t *cmn_info;

	ASSERT(n_info);
	pi = n_info->pi;
	cmn_info = n_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_lpc_unregister_impl(cmn_info);

	phy_mfree(pi, n_info, sizeof(phy_n_lpc_info_t));
}

/* ********************************************* */
/*				Callback Functions Table					*/
/* ********************************************* */
#define PWR_SEL_MIN_IDX 24
static uint8
wlc_phy_lpc_getminidx_nphy(void)
{
	return PWR_SEL_MIN_IDX;
}

static uint8
wlc_phy_lpc_getoffset_nphy(uint8 index)
{
	return index;
}

static uint8
wlc_phy_lpc_get_txcpwrval_nphy(uint16 phytxctrlword)
{
	return (phytxctrlword & PHY_TXC_PWR_MASK) >> PHY_TXC_PWR_SHIFT;
}

static void
wlc_phy_lpc_setmode_nphy(phy_type_lpc_ctx_t *ctx, bool enable)
{
	/* do nothing; just return */
	/* fn put to maintain modularity across PHYs */
	return;
}

static void
wlc_phy_lpc_set_txcpwrval_nphy(uint16 *phytxctrlword, uint8 txcpwrval)
{
	*phytxctrlword = (*phytxctrlword & ~PHY_TXC_PWR_MASK) |
		(txcpwrval << PHY_TXC_PWR_SHIFT);
	return;
}

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */

/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */
