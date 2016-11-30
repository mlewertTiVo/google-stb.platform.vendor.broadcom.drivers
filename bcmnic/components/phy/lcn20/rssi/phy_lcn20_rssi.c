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
 * $Id: phy_lcn20_rssi.c 646150 2016-06-28 17:04:40Z jqliu $
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
#if defined(BCMINTERNAL) || defined(WLTEST)
static int phy_lcn20_rssi_get_pkteng_stats(phy_type_rssi_ctx_t *ctx, void *a, int alen,
	wl_pkteng_stats_t stats);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#ifdef WLTEST
static int phy_lcn20_rssi_set_gain_delta_2gb(phy_type_rssi_ctx_t *ctx, uint32 aid,
	int8 *deltaValues);
static int phy_lcn20_rssi_get_gain_delta_2gb(phy_type_rssi_ctx_t *ctx, uint32 aid,
	int8 *deltaValues);
static int phy_lcn20_rssi_set_cal_freq_2g(phy_type_rssi_ctx_t *ctx, int8 *nvramValues);
static int phy_lcn20_rssi_get_cal_freq_2g(phy_type_rssi_ctx_t *ctx, int8 *nvramValues);
#endif /* WLTEST */

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
#if defined(BCMINTERNAL) || defined(WLTEST)
	fns.get_pkteng_stats = phy_lcn20_rssi_get_pkteng_stats;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#ifdef WLTEST
	fns.set_gain_delta_2gb = phy_lcn20_rssi_set_gain_delta_2gb;
	fns.get_gain_delta_2gb = phy_lcn20_rssi_get_gain_delta_2gb;
	fns.set_cal_freq_2g = phy_lcn20_rssi_set_cal_freq_2g;
	fns.get_cal_freq_2g = phy_lcn20_rssi_get_cal_freq_2g;
#endif /* WLTEST */
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
		wrxh->do_rssi_ma = 1;      /* skip calc rssi MA */
		goto end;
	}
	/* intermediate mpdus in a AMPDU do not have a valid phy status */
	if ((pi->sh->corerev >= 11) && !(ltoh16(rxh->lt80.RxStatus2) & RXS_PHYRXST_VALID)) {
		rssi = WLC_RSSI_INVALID;
		rssi_qdb = 0;
		wrxh->do_rssi_ma = 1;		/* skip calc rssi MA */
		goto end;
	}

	/* The below is WAR to avoid bad rssi values seen  in 11n rate */
	if (((ltoh16(rxh->lt80.PhyRxStatus_0) & PRXS0_FT_MASK) != PRXS0_CCK)) {
		if (rssi == LCN20_BAD_RSSI) {
			rssi = WLC_RSSI_INVALID;
			rssi_qdb = 0;
			/* skip calc rssi MA */
			wrxh->do_rssi_ma = 1;
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

	rssi = wlc_lcn20phy_rxpath_rssicorr(pi, rssi, &rssi_gain_params);

	wrxh->rxpwr[0] = (int8)(rssi >> LCN20_QDB_SHIFT);
	rssi_qdb = (int8)(rssi & LCN20_QDB_MASK);
	wrxh->do_rssi_ma = 0;

end:
	wrxh->rssi = (int8)(rssi >> 2);
	wrxh->rssi_qdb = (int8)rssi_qdb;

	PHY_INFORM(("%s: Final: rssi %d rssi_qdb: %d\n",
		__FUNCTION__, (int8)wrxh->rssi, wrxh->rssi_qdb));
}

#if defined(BCMINTERNAL) || defined(WLTEST)
#define LCN40_QDB_MASK	0x3
#define LCN40_QDB_SHIFT	2
#define RSSI_IQEST_DEBUG 0
static int
phy_lcn20_rssi_get_pkteng_stats(phy_type_rssi_ctx_t *ctx, void *a, int alen,
	wl_pkteng_stats_t stats)
{
	phy_lcn20_rssi_info_t *rssii = (phy_lcn20_rssi_info_t *)ctx;
	phy_info_t *pi = rssii->pi;
	uint16 rxstats_base;
	int i;

	int16 rssi_lcn[4];
	int16 snr_a_lcn[4];
	int16 snr_b_lcn[4];
	int16 rssi_qdb_lcn[4];
	uint16 jssi_addr[4], snr_a_addr[4], snr_b_addr[4];
	uint16 lcnphyregs_shm_addr =
		2 * wlapi_bmac_read_shm(pi->sh->physhim, M_LCN40PHYREGS_PTR(pi));
	uint16 elna_bypass;
	uint8 elna_bypass_flg;
	uint16 rssi_qdb_addr[4];
	uint8 lna1_bypass;
	uint8 lna1_rout;
	uint8 lna1_gain;
	uint8 lna2_gain;
	uint8 tia_amp2_bypass;
	uint8 aci_tbl_ind;
	lcn20phy_rssi_gain_params_t rssi_gain_params;
	uint16 tia_R1_val;
	uint16 tia_R2_val;
	uint16 tia_R3_val;
	uint8 dvga1_val;

	jssi_addr[0] = lcnphyregs_shm_addr + M_SSLPN_RSSI_0_OFFSET(pi);
	jssi_addr[1] = lcnphyregs_shm_addr + M_SSLPN_RSSI_1_OFFSET(pi);
	jssi_addr[2] = lcnphyregs_shm_addr + M_SSLPN_RSSI_2_OFFSET(pi);
	jssi_addr[3] = lcnphyregs_shm_addr + M_SSLPN_RSSI_3_OFFSET(pi);

	rssi_qdb_addr[0] = lcnphyregs_shm_addr + M_RSSI_QDB_0_OFFSET(pi);
	rssi_qdb_addr[1] = lcnphyregs_shm_addr + M_RSSI_QDB_1_OFFSET(pi);
	rssi_qdb_addr[2] = lcnphyregs_shm_addr + M_RSSI_QDB_2_OFFSET(pi);
	rssi_qdb_addr[3] = lcnphyregs_shm_addr + M_RSSI_QDB_3_OFFSET(pi);

	/* SNR */
	snr_a_addr[0] = lcnphyregs_shm_addr + M_SSLPN_SNR_0_logchPowAccOut_OFFSET(pi);
	snr_a_addr[1] = lcnphyregs_shm_addr + M_SSLPN_SNR_1_logchPowAccOut_OFFSET(pi);
	snr_a_addr[2] = lcnphyregs_shm_addr + M_SSLPN_SNR_2_logchPowAccOut_OFFSET(pi);
	snr_a_addr[3] = lcnphyregs_shm_addr + M_SSLPN_SNR_3_logchPowAccOut_OFFSET(pi);

	snr_b_addr[0] = lcnphyregs_shm_addr + M_SSLPN_SNR_0_errAccOut_OFFSET(pi);
	snr_b_addr[1] = lcnphyregs_shm_addr + M_SSLPN_SNR_1_errAccOut_OFFSET(pi);
	snr_b_addr[2] = lcnphyregs_shm_addr + M_SSLPN_SNR_2_errAccOut_OFFSET(pi);
	snr_b_addr[3] = lcnphyregs_shm_addr + M_SSLPN_SNR_3_errAccOut_OFFSET(pi);

	stats.rssi = 0;

	for (i = 0; i < 4; i++) {
		rssi_lcn[i] = (int8)(wlapi_bmac_read_shm(pi->sh->physhim,
			jssi_addr[i]) & 0xFF);

		rssi_qdb_lcn[i] = (int16)(wlapi_bmac_read_shm(pi->sh->physhim,
			rssi_qdb_addr[i]));

		if (rssi_lcn[i] > 127)
			rssi_lcn[i] -= 256;

		rssi_lcn[i] = (rssi_lcn[i] << LCN40_QDB_SHIFT);

		aci_tbl_ind = (rssi_qdb_lcn[i] >> 2) & 0x1;
		elna_bypass = rssi_qdb_lcn[i] & 0x3;
		tia_R2_val = (rssi_qdb_lcn[i] >> 3)& 0xFF;
		dvga1_val = (rssi_qdb_lcn[i] >> 11)& 0xF;

		if (elna_bypass ==
			((pi->u.pi_lcn20phy->swctrlmap_2g[LCN20PHY_I_WL_RX_ATTN] &
			LCN20PHY_I_WL_RX_ATTN_MASK) >> LCN20PHY_I_WL_RX_ATTN_SHIFT))
			elna_bypass_flg = 1;
		else
			elna_bypass_flg = 0;

		snr_a_lcn[i] = wlapi_bmac_read_shm(pi->sh->physhim, snr_a_addr[i]);
		lna1_bypass = (snr_a_lcn[i] >> 1) & 0x1;
		lna1_rout = (snr_a_lcn[i] >> 4) & 0xf;
		lna1_gain = (snr_a_lcn[i] >> 8) & 0x7;
		lna2_gain = (snr_a_lcn[i] >> 11) & 0x7;
		tia_amp2_bypass = (snr_a_lcn[i] >> 14) & 0x1;

		snr_b_lcn[i] = wlapi_bmac_read_shm(pi->sh->physhim, snr_b_addr[i]);
		tia_R1_val = (snr_b_lcn[i] >> 0) & 0x3F;
		tia_R3_val = (snr_b_lcn[i] >> 7) & 0x1FF;

		rssi_gain_params.elna_bypass = elna_bypass_flg;
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

		rssi_lcn[i] = wlc_lcn20phy_rxpath_rssicorr(pi, rssi_lcn[i],
			&rssi_gain_params);

		stats.rssi += rssi_lcn[i];
	}
	stats.rssi = stats.rssi >> 2;

	/* convert into dB and save qdB portion */
	stats.rssi_qdb = stats.rssi & LCN40_QDB_MASK;
	stats.rssi = stats.rssi >> LCN40_QDB_SHIFT;

	PHY_INFORM(("%s: RSSI = %d, RSSI_QDB = %d \n",
			__FUNCTION__, stats.rssi, stats.rssi_qdb));

	stats.snr = 0;

#if RSSI_IQEST_DEBUG
	stats.rssi = rssi_iqpwr_dB;
	stats.lostfrmcnt = rssi_iqpwr;
	stats.snr = board_atten_dbg;
#endif

	/* rx pkt stats */
	rxstats_base = wlapi_bmac_read_shm(pi->sh->physhim, M_RXSTATS_BLK_PTR(pi));
	for (i = 0; i <= NUM_80211_RATES; i++) {
		stats.rxpktcnt[i] =
			wlapi_bmac_read_shm(pi->sh->physhim, 2*(rxstats_base+i));
	}

	bcopy(&stats, a,
		(sizeof(wl_pkteng_stats_t) < (uint)alen) ? sizeof(wl_pkteng_stats_t) : (uint)alen);
	return BCME_OK;
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

#ifdef WLTEST
static int
phy_lcn20_rssi_set_gain_delta_2gb(phy_type_rssi_ctx_t *ctx, uint32 aid, int8 *deltaValues)
{
	phy_lcn20_rssi_info_t *rssii = (phy_lcn20_rssi_info_t *)ctx;
	int8 *p_rssi_delta_2g = rssii->lcn20i->rssi_corr_gain_delta_2g_sub;
	uint8 prm;

	if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB0)) {
		p_rssi_delta_2g += 0;
	} else if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB1)) {
		p_rssi_delta_2g += LCN20PHY_GAIN_DELTA_2G_PARAMS;
	} else if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB2)) {
		p_rssi_delta_2g += (2*LCN20PHY_GAIN_DELTA_2G_PARAMS);
	} else if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB3)) {
		p_rssi_delta_2g += (3*LCN20PHY_GAIN_DELTA_2G_PARAMS);
	} else {
		p_rssi_delta_2g += (4*LCN20PHY_GAIN_DELTA_2G_PARAMS);
	}

	for (prm = 0; prm < LCN20PHY_GAIN_DELTA_2G_PARAMS; prm++)
		p_rssi_delta_2g[prm] = deltaValues[prm];

	return BCME_OK;
}

static int
phy_lcn20_rssi_get_gain_delta_2gb(phy_type_rssi_ctx_t *ctx, uint32 aid, int8 *deltaValues)
{
	phy_lcn20_rssi_info_t *rssii = (phy_lcn20_rssi_info_t *)ctx;
	int8 *p_rssi_delta_2g = rssii->lcn20i->rssi_corr_gain_delta_2g_sub;
	uint8 prm;

	if (aid == IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB0)) {
		p_rssi_delta_2g += 0;
	} else if (aid == IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB1)) {
		p_rssi_delta_2g += LCN20PHY_GAIN_DELTA_2G_PARAMS;
	} else if (aid == IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB2)) {
		p_rssi_delta_2g += (2*LCN20PHY_GAIN_DELTA_2G_PARAMS);
	} else if (aid == IOV_GVAL(IOV_PHY_RSSI_GAIN_DELTA_2GB3)) {
		p_rssi_delta_2g += (3*LCN20PHY_GAIN_DELTA_2G_PARAMS);
	} else {
		p_rssi_delta_2g += (4*LCN20PHY_GAIN_DELTA_2G_PARAMS);
	}

	for (prm = 0; prm < LCN20PHY_GAIN_DELTA_2G_PARAMS; prm++) {
		deltaValues[prm] = p_rssi_delta_2g[prm];
	}

	return BCME_OK;
}

static int
phy_lcn20_rssi_set_cal_freq_2g(phy_type_rssi_ctx_t *ctx, int8 *nvramValues)
{
	phy_lcn20_rssi_info_t *rssii = (phy_lcn20_rssi_info_t *)ctx;
	int i;

	for (i = 0; i < 14; i++) {
		rssii->lcn20i->rssi_cal_freq_grp[i] = nvramValues[i];
	}
	return BCME_OK;
}

static int
phy_lcn20_rssi_get_cal_freq_2g(phy_type_rssi_ctx_t *ctx, int8 *nvramValues)
{
	phy_lcn20_rssi_info_t *rssii = (phy_lcn20_rssi_info_t *)ctx;
	int i;

	for (i = 0; i < 14; i++) {
		nvramValues[i] = rssii->lcn20i->rssi_cal_freq_grp[i];
	}
	return BCME_OK;
}
#endif /* WLTEST */
