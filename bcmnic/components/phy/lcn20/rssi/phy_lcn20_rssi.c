/*
 * lcn20PHY RSSI Compute module implementation
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
 * $Id: phy_lcn20_rssi.c 663069 2016-10-04 01:20:17Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_rssi.h"
#include <phy_lcn20.h>
#include <phy_lcn20_rssi.h>
#include <phy_rssi_iov.h>
#include <phy_utils_reg.h>
#include <wlc_phyreg_lcn20.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_lcn20.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_lcn20_rssi_info {
	phy_info_t *pi;
	phy_lcn20_info_t *lcn20i;
	phy_rssi_info_t *ri;
};

/* local functions */
static void phy_lcn20_rssi_compute(phy_type_rssi_ctx_t *ctx, wlc_d11rxhdr_t *wrxh);

/* register phy type specific functions */
phy_lcn20_rssi_info_t *
BCMATTACHFN(phy_lcn20_rssi_register_impl)(phy_info_t *pi, phy_lcn20_info_t *lcn20i,
	phy_rssi_info_t *ri)
{
	phy_lcn20_rssi_info_t *info;
	phy_type_rssi_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_lcn20_rssi_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_lcn20_rssi_info_t));
	info->pi = pi;
	info->lcn20i = lcn20i;
	info->ri = ri;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.compute = phy_lcn20_rssi_compute;
	fns.ctx = info;

	phy_rssi_register_impl(ri, &fns);

	return info;

	/* error handling */
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_lcn20_rssi_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn20_rssi_unregister_impl)(phy_lcn20_rssi_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_rssi_info_t *ri = info->ri;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_rssi_unregister_impl(ri);

	phy_mfree(pi, info, sizeof(phy_lcn20_rssi_info_t));
}

/* calculate rssi */
#define LCN20_RXSTAT2_BRD_ATTN_SHIFT	14
#define LCN20_RXSTAT2_BRD_ATTN_MASK	(0x3) << LCN20_RXSTAT2_BRD_ATTN_SHIFT

#define LCN20_RXSTAT0_ACITBL_IND_SHIFT	12
#define LCN20_RXSTAT0_ACITBL_IND_MASK	(0x1) << LCN20_RXSTAT0_ACITBL_IND_SHIFT

#define LCN20_RXSTAT2_TIA_AMP2_BYP_SHIFT	13
#define LCN20_RXSTAT2_TIA_AMP2_BYP_MASK	(0x1) << LCN20_RXSTAT2_TIA_AMP2_BYP_SHIFT

#define LCN20_RXSTAT2_LNA1_GAIN_SHIFT	5
#define LCN20_RXSTAT2_LNA1_GAIN_MASK	(0x7) << LCN20_RXSTAT2_LNA1_GAIN_SHIFT

#define LCN20_RXSTAT2_LNA1_ROUT_SHIFT	1
#define LCN20_RXSTAT2_LNA1_ROUT_MASK	(0xF) << LCN20_RXSTAT2_LNA1_ROUT_SHIFT

#define LCN20_RXSTAT2_LNA2_GAIN_SHIFT	10
#define LCN20_RXSTAT2_LNA2_GAIN_MASK	(0x7) << LCN20_RXSTAT2_LNA2_GAIN_SHIFT

#define LCN20_RXSTAT2_LNA1_BYP_SHIFT	0
#define LCN20_RXSTAT2_LNA1_BYP_MASK	(0x1) << LCN20_RXSTAT2_LNA1_BYP_SHIFT

#define LCN20_RXSTAT4_TIA_R1_SHIFT	0
#define LCN20_RXSTAT4_TIA_R1_MASK	(0x3F) << LCN20_RXSTAT4_TIA_R1_SHIFT

#define LCN20_RXSTAT5_TIA_R2_SHIFT	0
#define LCN20_RXSTAT5_TIA_R2_MASK	(0xFF) << LCN20_RXSTAT5_TIA_R2_SHIFT

#define LCN20_RXSTAT4_TIA_R3_SHIFT	7
#define LCN20_RXSTAT4_TIA_R3_MASK	(0x1FF) << LCN20_RXSTAT4_TIA_R3_SHIFT

#define LCN20_RXSTAT5_DVGA1_SHIFT	8
#define LCN20_RXSTAT5_DVGA1_MASK	(0xF) << LCN20_RXSTAT5_DVGA1_SHIFT

#define LCN20_QDB_MASK	0x3
#define LCN20_QDB_SHIFT	2
#define LCN20_BIT1_QDB_POS	10
#define LCN20_BIT0_QDB_POS	13

#define LCN20_BAD_RSSI	157
#define LCN20_RXSTAT0_ACIDET	12
static void BCMFASTPATH
phy_lcn20_rssi_compute(phy_type_rssi_ctx_t *ctx, wlc_d11rxhdr_t *wrxh)
{
	phy_lcn20_rssi_info_t *info = (phy_lcn20_rssi_info_t *)ctx;
	phy_info_t *pi = info->pi;
	d11rxhdr_t *rxh = &wrxh->rxhdr;
	int16 rssi = ltoh16(rxh->lt80.PhyRxStatus_1) & PRXS1_JSSI_MASK;
	int8 rssi_qdb = 0;

	uint16 board_atten = (ltoh16(rxh->lt80.PhyRxStatus_2) & LCN20_RXSTAT2_BRD_ATTN_MASK) >>
			LCN20_RXSTAT2_BRD_ATTN_SHIFT;
	uint8 aci_tbl_ind = (ltoh16(rxh->lt80.PhyRxStatus_0) & LCN20_RXSTAT0_ACITBL_IND_MASK) >>
			LCN20_RXSTAT0_ACITBL_IND_SHIFT;
	uint8 tia_amp2_bypass = (ltoh16(rxh->lt80.PhyRxStatus_2) & LCN20_RXSTAT2_TIA_AMP2_BYP_MASK) >>
			LCN20_RXSTAT2_TIA_AMP2_BYP_SHIFT;
	uint8 lna1_gain = (ltoh16(rxh->lt80.PhyRxStatus_2) & LCN20_RXSTAT2_LNA1_GAIN_MASK) >>
			LCN20_RXSTAT2_LNA1_GAIN_SHIFT;
	uint8 lna1_rout = (ltoh16(rxh->lt80.PhyRxStatus_2) & LCN20_RXSTAT2_LNA1_ROUT_MASK) >>
			LCN20_RXSTAT2_LNA1_ROUT_SHIFT;
	uint8 lna2_gain = (ltoh16(rxh->lt80.PhyRxStatus_2) & LCN20_RXSTAT2_LNA2_GAIN_MASK) >>
			LCN20_RXSTAT2_LNA2_GAIN_SHIFT;
	uint8 lna1_bypass = (ltoh16(rxh->lt80.PhyRxStatus_2) & LCN20_RXSTAT2_LNA1_BYP_MASK) >>
			LCN20_RXSTAT2_LNA1_BYP_SHIFT;
	uint16 tia_R1_val = (ltoh16(rxh->lt80.PhyRxStatus_4) & LCN20_RXSTAT4_TIA_R1_MASK) >>
			LCN20_RXSTAT4_TIA_R1_SHIFT;
	uint16 tia_R2_val = (ltoh16(rxh->lt80.PhyRxStatus_5) & LCN20_RXSTAT5_TIA_R2_MASK) >>
			LCN20_RXSTAT5_TIA_R2_SHIFT;
	uint16 tia_R3_val = (ltoh16(rxh->lt80.PhyRxStatus_4) & LCN20_RXSTAT4_TIA_R3_MASK) >>
			LCN20_RXSTAT4_TIA_R3_SHIFT;
	uint8 dvga1_val = (ltoh16(rxh->lt80.PhyRxStatus_5) & LCN20_RXSTAT5_DVGA1_MASK) >>
			LCN20_RXSTAT5_DVGA1_SHIFT;
	uint8 elna_bypass;
	lcn20phy_rssi_gain_params_t rssi_gain_params;

	PHY_INFORM(("\n boardattn= %d, lna1_gain=%d, lna1_rout= %d, lna2_gain= %d, "
		"tia_amp2_bypass=%d, lna1_bypass= %d, aci_tbl_ind= %d\n", board_atten, lna1_gain,
		lna1_rout, lna2_gain, tia_amp2_bypass, lna1_bypass, aci_tbl_ind));

	if (ISSIM_ENAB(pi->sh->sih)) {
		rssi = WLC_RSSI_INVALID;
		rssi_qdb = 0;
		goto end;
	}
	/* intermediate mpdus in a AMPDU do not have a valid phy status */
	if ((pi->sh->corerev >= 11) && !(ltoh16(rxh->lt80.RxStatus2) & RXS_PHYRXST_VALID)) {
		rssi = WLC_RSSI_INVALID;
		rssi_qdb = 0;
		goto end;
	}

	/* The below is WAR to avoid bad rssi values seen  in 11n rate */
	if (((ltoh16(rxh->lt80.PhyRxStatus_0) & PRXS0_FT_MASK) != PRXS0_CCK)) {
		if (rssi == LCN20_BAD_RSSI) {
			rssi = WLC_RSSI_INVALID;
			rssi_qdb = 0;
			goto end;
		}
	}

	if (rssi > 127)
		rssi -= 256;

	rssi = (rssi << LCN20_QDB_SHIFT) + rssi_qdb;

	if (board_atten == ((pi->u.pi_lcn20phy->swctrlmap_2g[LCN20PHY_I_WL_RX_ATTN] &
		LCN20PHY_I_WL_RX_ATTN_MASK) >> LCN20PHY_I_WL_RX_ATTN_SHIFT))
		elna_bypass = 1;
	else
		elna_bypass = 0;

	rssi_gain_params.elna_bypass = elna_bypass;
	rssi_gain_params.lna1_bypass = lna1_bypass;
	rssi_gain_params.lna1_rout = lna1_rout;
	rssi_gain_params.lna1_gain = lna1_gain;
	rssi_gain_params.lna2_gain = lna2_gain;
	rssi_gain_params.tia_amp2_bypass = tia_amp2_bypass;
	rssi_gain_params.aci_tbl_ind = aci_tbl_ind;
	rssi_gain_params.tia_R1_val = tia_R1_val;
	rssi_gain_params.tia_R2_val = tia_R2_val;
	rssi_gain_params.tia_R3_val = tia_R3_val;
	rssi_gain_params.dvga1_val = dvga1_val;

	rssi = wlc_lcn20phy_rxpath_rssicorr(pi, rssi, &rssi_gain_params, 255, 255);

	PHY_INFORM(("%s : Corrected: rssiqdB= %d, boardattn= %d\n",
		__FUNCTION__, rssi, elna_bypass));

	wrxh->rxpwr[0] = (int8)(rssi >> LCN20_QDB_SHIFT);
	rssi_qdb = (int8)(rssi & LCN20_QDB_MASK);

end:
	wrxh->rssi = (int8)(rssi >> 2);
	wrxh->rssi_qdb = (int8)rssi_qdb;

	PHY_INFORM(("%s: Final: rssi %d rssi_qdb: %d\n",
		__FUNCTION__, (int8)wrxh->rssi, wrxh->rssi_qdb));
}
