/*
 * LCN40PHY TXIQLO CAL module implementation
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
 * $Id: phy_lcn40_txiqlocal.c $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_type_txiqlocal.h>
#include <phy_lcn40.h>
#include <phy_lcn40_txiqlocal.h>
#include <phy_lcn40_radio.h>

#define LCN40PHY_TBL_ID_IQLOCAL			0x00

/* module private states */
struct phy_lcn40_txiqlocal_info {
	phy_info_t *pi;
	phy_lcn40_info_t *lcn40i;
	phy_txiqlocal_info_t *cmn_info;
};

/* local functions */
static void phy_lcn40_txiqlocal_txiqccget(phy_type_txiqlocal_ctx_t *ctx, void *a);
static void phy_lcn40_txiqlocal_txiqccset(phy_type_txiqlocal_ctx_t *ctx, void *b);
static void phy_lcn40_txiqlocal_txloccget(phy_type_txiqlocal_ctx_t *ctx, void *a);
static void phy_lcn40_txiqlocal_txloccset(phy_type_txiqlocal_ctx_t *ctx, void *b);

/* register phy type specific implementation */
phy_lcn40_txiqlocal_info_t *
BCMATTACHFN(phy_lcn40_txiqlocal_register_impl)(phy_info_t *pi, phy_lcn40_info_t *lcn40i,
	phy_txiqlocal_info_t *cmn_info)
{
	phy_lcn40_txiqlocal_info_t *lcn40_info;
	phy_type_txiqlocal_fns_t fns;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((lcn40_info = phy_malloc(pi, sizeof(phy_lcn40_txiqlocal_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* initialize ptrs */
	lcn40_info->pi = pi;
	lcn40_info->lcn40i = lcn40i;
	lcn40_info->cmn_info = cmn_info;


	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = lcn40_info;
	fns.txiqccget = phy_lcn40_txiqlocal_txiqccget;
	fns.txiqccset = phy_lcn40_txiqlocal_txiqccset;
	fns.txloccget = phy_lcn40_txiqlocal_txloccget;
	fns.txloccset = phy_lcn40_txiqlocal_txloccset;

	if (phy_txiqlocal_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_txiqlocal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return lcn40_info;

	/* error handling */
fail:
	if (lcn40_info) {
		phy_mfree(pi, lcn40_info, sizeof(phy_lcn40_txiqlocal_info_t));
	}
	return NULL;
}

void
BCMATTACHFN(phy_lcn40_txiqlocal_unregister_impl)(phy_lcn40_txiqlocal_info_t *lcn40_info)
{
	phy_txiqlocal_info_t *cmn_info;
	phy_info_t *pi;

	pi = lcn40_info->pi;
	cmn_info = lcn40_info->cmn_info;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_txiqlocal_unregister_impl(cmn_info);

	phy_mfree(pi, lcn40_info, sizeof(phy_lcn40_txiqlocal_info_t));
}

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */
static void phy_lcn40_txiqlocal_txiqccget(phy_type_txiqlocal_ctx_t *ctx, void *a)
{
	phy_lcn40_txiqlocal_info_t *info = (phy_lcn40_txiqlocal_info_t *)ctx;
	phy_info_t *pi = info->pi;
	int32 iqccValues[2];
	uint16 valuea = 0;
	uint16 valueb = 0;

	wlc_lcn40phy_get_tx_iqcc(pi, &valuea, &valueb);
	iqccValues[0] = valuea;
	iqccValues[1] = valueb;
	bcopy(iqccValues, a, 2*sizeof(int32));
}

static void phy_lcn40_txiqlocal_txiqccset(phy_type_txiqlocal_ctx_t *ctx, void *p)
{
	phy_lcn40_txiqlocal_info_t *info = (phy_lcn40_txiqlocal_info_t *)ctx;
	phy_info_t *pi = info->pi;
	int32 iqccValues[2];
	uint16 valuea, valueb;

	bcopy(p, iqccValues, 2*sizeof(int32));
	valuea = (uint16)(iqccValues[0]);
	valueb = (uint16)(iqccValues[1]);
	wlc_lcn40phy_set_tx_iqcc(pi, valuea, valueb);
}

static void phy_lcn40_txiqlocal_txloccget(phy_type_txiqlocal_ctx_t *ctx, void *a)
{
	phy_lcn40_txiqlocal_info_t *info = (phy_lcn40_txiqlocal_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint16 di0dq0;
	uint8 *loccValues = a;

	/* copy the 6 bytes to a */
	di0dq0 = wlc_lcn40phy_get_tx_locc(pi);
	loccValues[0] = (uint8)(di0dq0 >> 8);
	loccValues[1] = (uint8)(di0dq0 & 0xff);
	wlc_lcn40phy_get_radio_loft(pi, &loccValues[2], &loccValues[3],
		&loccValues[4], &loccValues[5]);
}
static void phy_lcn40_txiqlocal_txloccset(phy_type_txiqlocal_ctx_t *ctx, void *p)
{
	phy_lcn40_txiqlocal_info_t *info = (phy_lcn40_txiqlocal_info_t *)ctx;
	phy_info_t *pi = info->pi;
	/* copy 6 bytes from a to radio */
	uint16 di0dq0;
	uint8 *loccValues = p;

	di0dq0 = ((uint16)loccValues[0] << 8) | loccValues[1];
	wlc_lcn40phy_set_tx_locc(pi, di0dq0);
	wlc_lcn40phy_set_radio_loft(pi, loccValues[2],
		loccValues[3], loccValues[4], loccValues[5]);
}

/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */
uint16
wlc_lcn40phy_get_tx_locc(phy_info_t *pi)
{	phytbl_info_t tab;
	uint16 didq;

	/* Update iqloCaltbl */
	tab.tbl_id = LCN40PHY_TBL_ID_IQLOCAL; /* iqloCaltbl		*/
	tab.tbl_width = 16;	/* 16 bit wide	*/
	tab.tbl_ptr = &didq;
	tab.tbl_len = 1;
	tab.tbl_offset = 85;
	wlc_lcn40phy_read_table(pi, &tab);

	return didq;
}

void
wlc_lcn40phy_set_tx_locc(phy_info_t *pi, uint16 didq)
{
	phytbl_info_t tab;

	/* Update iqloCaltbl */
	tab.tbl_id = LCN40PHY_TBL_ID_IQLOCAL;			/* iqloCaltbl	*/
	tab.tbl_width = 16;	/* 16 bit wide	*/
	tab.tbl_ptr = &didq;
	tab.tbl_len = 1;
	tab.tbl_offset = 85;
	wlc_lcn40phy_write_table(pi, &tab);
}

void
wlc_lcn40phy_get_tx_iqcc(phy_info_t *pi, uint16 *a, uint16 *b)
{
	uint16 iqcc[2];
	phytbl_info_t tab;

	tab.tbl_ptr = iqcc; /* ptr to buf */
	tab.tbl_len = 2;        /* # values   */
	tab.tbl_id = LCN40PHY_TBL_ID_IQLOCAL; /* iqloCaltbl      */
	tab.tbl_offset = 80; /* tbl offset */
	tab.tbl_width = 16;     /* 16 bit wide */
	wlc_lcn40phy_read_table(pi, &tab);

	*a = iqcc[0];
	*b = iqcc[1];
}
void
wlc_lcn40phy_set_tx_iqcc(phy_info_t *pi, uint16 a, uint16 b)
{
	phytbl_info_t tab;
	uint16 iqcc[2];

	/* Fill buffer with coeffs */
	iqcc[0] = a;
	iqcc[1] = b;
	/* Update iqloCaltbl */
	tab.tbl_id = LCN40PHY_TBL_ID_IQLOCAL;			/* iqloCaltbl	*/
	tab.tbl_width = 16;	/* 16 bit wide	*/
	tab.tbl_ptr = iqcc;
	tab.tbl_len = 2;
	tab.tbl_offset = 80;
	wlc_lcn40phy_write_table(pi, &tab);
}
