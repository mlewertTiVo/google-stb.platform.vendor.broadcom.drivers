/*
 * LCN40PHY RSSI Compute module implementation
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
 * $Id: phy_lcn40_rssi.c 646150 2016-06-28 17:04:40Z jqliu $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_rssi.h"
#include <phy_lcn40.h>
#include <phy_lcn40_rssi.h>
#include <phy_rssi_iov.h>
#include <phy_utils_reg.h>
#include <wlc_phyreg_lcn40.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_lcn40.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_lcn40_rssi_info {
	phy_info_t *pi;
	phy_lcn40_info_t *lcn40i;
	phy_rssi_info_t *ri;
};

/* local functions */
static void phy_lcn40_rssi_compute(phy_type_rssi_ctx_t *ctx, wlc_d11rxhdr_t *wrxh);
#if defined(BCMINTERNAL) || defined(WLTEST)
static int phy_lcn40_rssi_get_pkteng_stats(phy_type_rssi_ctx_t *ctx, void *a, int alen,
	wl_pkteng_stats_t stats);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
static int phy_lcn40_rssi_set_gain_delta_2g(phy_type_rssi_ctx_t *ctx, uint32 aid,
	int8 *deltaValues);
static int phy_lcn40_rssi_get_gain_delta_2g(phy_type_rssi_ctx_t *ctx, uint32 aid,
	int8 *deltaValues);
static int phy_lcn40_rssi_set_gain_delta_5g(phy_type_rssi_ctx_t *ctx, uint32 aid,
	int8 *deltaValues);
static int phy_lcn40_rssi_get_gain_delta_5g(phy_type_rssi_ctx_t *ctx, uint32 aid,
	int8 *deltaValues);

/* register phy type specific functions */
phy_lcn40_rssi_info_t *
BCMATTACHFN(phy_lcn40_rssi_register_impl)(phy_info_t *pi, phy_lcn40_info_t *lcn40i,
	phy_rssi_info_t *ri)
{
	phy_lcn40_rssi_info_t *info;
	phy_type_rssi_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_lcn40_rssi_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_lcn40_rssi_info_t));
	info->pi = pi;
	info->lcn40i = lcn40i;
	info->ri = ri;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.compute = phy_lcn40_rssi_compute;
#if defined(BCMINTERNAL) || defined(WLTEST)
	fns.get_pkteng_stats = phy_lcn40_rssi_get_pkteng_stats;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
	fns.set_gain_delta_2g = phy_lcn40_rssi_set_gain_delta_2g;
	fns.get_gain_delta_2g = phy_lcn40_rssi_get_gain_delta_2g;
	fns.set_gain_delta_5g = phy_lcn40_rssi_set_gain_delta_5g;
	fns.get_gain_delta_5g = phy_lcn40_rssi_get_gain_delta_5g;
	fns.ctx = info;

	phy_rssi_register_impl(ri, &fns);

	return info;

	/* error handling */
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_lcn40_rssi_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn40_rssi_unregister_impl)(phy_lcn40_rssi_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_rssi_info_t *ri = info->ri;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_rssi_unregister_impl(ri);

	phy_mfree(pi, info, sizeof(phy_lcn40_rssi_info_t));
}

static const int8 lcnphy_gain_index_offset_for_pkt_rssi[] = {
	8,      /* 0 */
	8,      /* 1 */
	8,      /* 2 */
	8,      /* 3 */
	8,      /* 4 */
	8,      /* 5 */
	8,      /* 6 */
	9,      /* 7 */
	10,     /* 8 */
	8,      /* 9 */
	8,      /* 10 */
	7,      /* 11 */
	7,      /* 12 */
	1,      /* 13 */
	2,      /* 14 */
	2,      /* 15 */
	2,      /* 16 */
	2,      /* 17 */
	2,      /* 18 */
	2,      /* 19 */
	2,      /* 20 */
	2,      /* 21 */
	2,      /* 22 */
	2,      /* 23 */
	2,      /* 24 */
	2,      /* 25 */
	2,      /* 26 */
	2,      /* 27 */
	2,      /* 28 */
	2,      /* 29 */
	2,      /* 30 */
	2,      /* 31 */
	1,      /* 32 */
	1,      /* 33 */
	0,      /* 34 */
	0,      /* 35 */
	0,      /* 36 */
	0       /* 37 */
};


/* calculate rssi */
#define LCN40_RXSTAT0_BRD_ATTN	12
#define LCN40_RXSTAT0_ACITBL_IDX_MSB	11
#define LCN40_RX_GAIN_INDEX_MASK	0x7F00
#define LCN40_RX_GAIN_INDEX_SHIFT	8
#define LCN40_QDB_MASK	0x3
#define LCN40_QDB_SHIFT	2
#define LCN40_BIT1_QDB_POS	10
#define LCN40_BIT0_QDB_POS	13

static void BCMFASTPATH
phy_lcn40_rssi_compute(phy_type_rssi_ctx_t *ctx, wlc_d11rxhdr_t *wrxh)
{
	phy_lcn40_rssi_info_t *info = (phy_lcn40_rssi_info_t *)ctx;
	phy_info_t *pi = info->pi;
	phy_lcn40_info_t *lcn40i = info->lcn40i;
	d11rxhdr_t *rxh = &wrxh->rxhdr;
	int rssi = rxh->lt80.PhyRxStatus_1 & PRXS1_JSSI_MASK;
	uint16 board_atten = (rxh->lt80.PhyRxStatus_0 >> LCN40_RXSTAT0_BRD_ATTN) & 0x1;
	uint8 gidx = (rxh->lt80.PhyRxStatus_2 & 0xFC00) >> 10;
	int8 *corr2g;
#ifdef BAND5G
	int8 *corr5g;
#endif
	int8 *corrperrg;
	int8 po_reg;
	int16 po_nom = 0;
	int16 rxpath_gain;
	uint8 aci_tbl = 0;
	int8 rssi_qdb = 0;
	bool jssi_based_rssi = FALSE;
	uint8 tr_iso = 0;

	/* For bcm4334 do not use packets that were received when prisel_clr flag is set */
	if ((pi->sh->corerev == 35) && (rxh->lt80.RxStatus2 & RXS_PHYRXST_PRISEL_CLR)) {
		/* RXS_PHYRXST_PRISEL_CLR set, rssi_qdB = 0x0 => Use JSSI */
		/* RXS_PHYRXST_PRISEL_CLR set, rssi_qdB = 0x3 => WLC_RSSI_INVALID */
		if (lcn40i->rssi_iqest_en && lcn40i->rssi_iqest_jssi_en) {
			rssi_qdb = (rxh->lt80.PhyRxStatus_0
			            >> LCN40_BIT0_QDB_POS) & 0x1;
			rssi_qdb |= ((rxh->lt80.PhyRxStatus_0
			              & (1 << LCN40_BIT1_QDB_POS))
			             >> (LCN40_BIT1_QDB_POS - 1));
			if (!rssi_qdb) {
				jssi_based_rssi = TRUE;
			}
		}
		else {
			rssi = WLC_RSSI_INVALID;
			wrxh->do_rssi_ma = 1; /* skip calc rssi MA */
			goto end;
		}
	}

	wrxh->do_rssi_ma = 0;

	if (rssi > 127)
		rssi -= 256;

	aci_tbl = (rxh->lt80.PhyRxStatus_0 >> LCN40_RXSTAT0_ACITBL_IDX_MSB) & 0x1;
	if (aci_tbl)
		gidx = gidx + (1 << 6);

	if (lcn40i->rssi_iqest_en) {
		if (CHSPEC_IS2G(pi->radio_chanspec))
			tr_iso = lcn40i->lcnphycommon.lcnphy_tr_isolation_mid;
#ifdef BAND5G
		else
			tr_iso = lcn40i->lcnphycommon.triso5g[0];
#endif
		if (board_atten) {
			gidx = gidx + tr_iso;
		}
	}

	if (lcn40i->rssi_iqest_en && (!jssi_based_rssi)) {
		int rssi_in_qdb;

		rxpath_gain = wlc_lcn40phy_get_rxpath_gain_by_index(pi, gidx, board_atten);
		rssi_qdb = (rxh->lt80.PhyRxStatus_0 >> LCN40_BIT0_QDB_POS) & 0x1;
		rssi_qdb |= ((rxh->lt80.PhyRxStatus_0 & (1 << LCN40_BIT1_QDB_POS))
		             >> (LCN40_BIT1_QDB_POS - 1));
		rssi_in_qdb = (rssi << LCN40_QDB_SHIFT) + rssi_qdb - rxpath_gain +
		        (lcn40i->rssi_iqest_gain_adj << LCN40_QDB_SHIFT);
		rssi = (rssi_in_qdb >> LCN40_QDB_SHIFT);
		rssi_qdb = rssi_in_qdb & LCN40_QDB_MASK;
		PHY_INFORM(("rssidB= %d, rssi_qdB= %d, rssi_in_qdB= %d"
		            "boardattn= %d, rxpath_gain= %d, "
		            "gidx = %d, gain_adj = %d\n",
		            rssi, rssi_qdb, rssi_in_qdb, board_atten,
		            rxpath_gain, gidx,
		            lcn40i->rssi_iqest_gain_adj));
	}

	if ((!lcn40i->rssi_iqest_en) || jssi_based_rssi) {
		/* JSSI adjustment wrt power offset */
		if (CHSPEC_IS20(pi->radio_chanspec))
			po_reg = PHY_REG_READ(pi, LCN40PHY, SignalBlockConfigTable6_new,
			                      crssignalblk_input_pwr_offset_db);
		else
			po_reg = PHY_REG_READ(pi, LCN40PHY, SignalBlockConfigTable5_new,
			                      crssignalblk_input_pwr_offset_db_40mhz);
		switch (wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec)) {
		case WL_CHAN_FREQ_RANGE_2G:
			if (CHSPEC_IS20(pi->radio_chanspec))
				po_nom = lcn40i->lcnphycommon.noise.nvram_input_pwr_offset_2g;
			else
				po_nom = lcn40i->lcnphycommon.noise.nvram_input_pwr_offset_40_2g;
			break;
#ifdef BAND5G
		case WL_CHAN_FREQ_RANGE_5GL:
			/* 5 GHz low */
			if (CHSPEC_IS20(pi->radio_chanspec))
				po_nom = lcn40i->lcnphycommon.noise.nvram_input_pwr_offset_5g[0];
			else
				po_nom = lcn40i->lcnphycommon.noise.nvram_input_pwr_offset_40_5g[0];
			break;
		case WL_CHAN_FREQ_RANGE_5GM:
			/* 5 GHz middle */
			if (CHSPEC_IS20(pi->radio_chanspec))
				po_nom = lcn40i->lcnphycommon.noise.nvram_input_pwr_offset_5g[1];
			else
				po_nom = lcn40i->lcnphycommon.noise.nvram_input_pwr_offset_40_5g[1];
			break;
		case WL_CHAN_FREQ_RANGE_5GH:
			/* 5 GHz high */
			if (CHSPEC_IS20(pi->radio_chanspec))
				po_nom = lcn40i->lcnphycommon.noise.nvram_input_pwr_offset_5g[2];
			else
				po_nom = lcn40i->lcnphycommon.noise.nvram_input_pwr_offset_40_5g[2];
			break;
#endif /* BAND5G */
		default:
			po_nom = po_reg;
			break;
		}

		rssi += (po_nom - po_reg);

		/* RSSI adjustment and Adding the JSSI range specific corrections */
#ifdef BAND5G
		if (wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec) !=
		    WL_CHAN_FREQ_RANGE_2G) {
			if ((rssi < -60) && ((gidx > 0) && (gidx <= 37)))
				rssi += phy_lcn40_get_pkt_rssi_gain_index_offset_5g(gidx);
			corrperrg = pi->rssi_corr_perrg_5g;
		} else
#endif /* BAND5G */
		{
			if ((rssi < -60) && ((gidx > 0) && (gidx <= 37)))
				rssi += phy_lcn40_get_pkt_rssi_gain_index_offset_2g(gidx);
			corrperrg = pi->rssi_corr_perrg_2g;
		}

		if (rssi <= corrperrg[0])
			rssi += corrperrg[2];
		else if (rssi <= corrperrg[1])
			rssi += corrperrg[3];
		else
			rssi += corrperrg[4];

		corr2g = &(pi->rssi_corr_normal);
#ifdef BAND5G
		corr5g = &(pi->rssi_corr_normal_5g[0]);
#endif /* BAND5G */

		switch (wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec)) {
		case WL_CHAN_FREQ_RANGE_2G:
			rssi += *corr2g;
			break;
#ifdef BAND5G
		case WL_CHAN_FREQ_RANGE_5GL:
			/* 5 GHz low */
			rssi += corr5g[0];
			break;
		case WL_CHAN_FREQ_RANGE_5GM:
			/* 5 GHz middle */
			rssi += corr5g[1];
			break;
		case WL_CHAN_FREQ_RANGE_5GH:
			/* 5 GHz high */
			rssi += corr5g[2];
			break;
#endif /* BAND5G */
		default:
			rssi += 0;
			break;
		}
	}

	/* Temp sense based correction */
	rssi = (rssi << LCN40_QDB_SHIFT) + rssi_qdb;
	rssi += wlc_lcn40phy_rssi_tempcorr(pi, 0);
	if (lcn40i->rssi_iqest_en && !jssi_based_rssi)
		rssi += wlc_lcn40phy_iqest_rssi_tempcorr(pi, 0, board_atten);

	rssi_qdb = rssi & LCN40_QDB_MASK;
	rssi = (rssi >> LCN40_QDB_SHIFT);

	/* temperature compensation */
	rssi = rssi + lcn40i->lcnphycommon.lcnphy_pkteng_rssi_slope;

	/* Sign extend */
	if (rssi > 127)
		rssi -= 256;

	wrxh->rxpwr[0] = (int8)rssi;

	if (rssi > MAX_VALID_RSSI)
		rssi = MAX_VALID_RSSI;

end:
	wrxh->rssi = (int8)rssi;
	wrxh->rssi_qdb = rssi_qdb;

	PHY_TRACE(("%s: rssi %d\n", __FUNCTION__, (int8)rssi));
}
static const int8 lcn40phy_gain_index_offset_for_pkt_rssi_2g[] = {
	0,	/* 0 */
	0,	/* 1 */
	0,	/* 2 */
	0,	/* 3 */
	0,	/* 4 */
	0,	/* 5 */
	0,	/* 6 */
	0,	/* 7 */
	0,	/* 8 */
	0,	/* 9 */
	0,	/* 10 */
	0,	/* 11 */
	0,	/* 12 */
	0,	/* 13 */
	0,	/* 14 */
	0,	/* 15 */
	0,	/* 16 */
	0,	/* 17 */
	0,	/* 18 */
	0,	/* 19 */
	0,	/* 20 */
	0,	/* 21 */
	0,	/* 22 */
	0,	/* 23 */
	0,	/* 24 */
	1,	/* 25 */
	0,	/* 26 */
	1,	/* 27 */
	2,	/* 28 */
	2,	/* 29 */
	0,	/* 30 */
	0,	/* 31 */
	0,	/* 32 */
	0,	/* 33 */
	0,	/* 34 */
	0,	/* 35 */
	0,	/* 36 */
	0,	/* 37 */
};

/* don't ROM this variable (must disable qual-cast for this) */
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wcast-qual\"")
#endif
const int8 *phy_lcn_pkt_rssi_gain_index_offset = lcnphy_gain_index_offset_for_pkt_rssi;
const int8 *phy_lcn40_pkt_rssi_gain_index_offset_2g =
        lcn40phy_gain_index_offset_for_pkt_rssi_2g;
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic pop")
#endif

#ifdef BAND5G
static const  int8 lcn40phy_gain_index_offset_for_pkt_rssi_5g[] = {
	0,	/* 0 */
	0,	/* 1 */
	0,	/* 2 */
	0,	/* 3 */
	0,	/* 4 */
	0,	/* 5 */
	0,	/* 6 */
	0,	/* 7 */
	0,	/* 8 */
	0,	/* 9 */
	0,	/* 10 */
	0,	/* 11 */
	0,	/* 12 */
	0,	/* 13 */
	0,	/* 14 */
	0,	/* 15 */
	0,	/* 16 */
	0,	/* 17 */
	0,	/* 18 */
	0,	/* 19 */
	0,	/* 20 */
	0,	/* 21 */
	0,	/* 22 */
	0,	/* 23 */
	0,	/* 24 */
	0,	/* 25 */
	2,	/* 26 */
	2,	/* 27 */
	2,	/* 28 */
	2,	/* 29 */
	0,	/* 30 */
	0,	/* 31 */
	0,	/* 32 */
	0,	/* 33 */
	0,	/* 34 */
	0,	/* 35 */
	0,	/* 36 */
	0	/* 37 */
};

/* don't ROM this variable (must disable cast-qual for this) */
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wcast-qual\"")
#endif
const int8 *phy_lcn40_pkt_rssi_gain_index_offset_5g =
        lcn40phy_gain_index_offset_for_pkt_rssi_5g;
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic pop")
#endif
#endif /* BAND5G */

#if defined(BCMINTERNAL) || defined(WLTEST)
#define RSSI_CORR_EN 1
#define RSSI_IQEST_DEBUG 0
static int
phy_lcn40_rssi_get_pkteng_stats(phy_type_rssi_ctx_t *ctx, void *a, int alen,
	wl_pkteng_stats_t stats)
{
	phy_lcn40_rssi_info_t *rssii = (phy_lcn40_rssi_info_t *)ctx;
	phy_info_t *pi = rssii->pi;
	uint16 rxstats_base;
	int i;

	int16 rssi_lcn[4];
	int16 snr_a_lcn[4];
	int16 snr_b_lcn[4];
	uint8 gidx[4];
	int8 snr[4];
	int8 snr_lcn[4];
	phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
	phy_info_lcn40phy_t *pi_lcn40 = pi->u.pi_lcn40phy;
	uint16 rssi_addr[4], snr_a_addr[4], snr_b_addr[4];
	uint16 lcnphyregs_shm_addr =
		2 * wlapi_bmac_read_shm(pi->sh->physhim, M_LCN40PHYREGS_PTR(pi));
	int16 rxpath_gain;
	uint16 board_atten = 0;
	int16 rssi_lcn40_qdb[4];
	uint16 rssi_qdb_addr[4];
	uint8 gain_resp[4];
	uint8 gidx_calc[4];

#if RSSI_IQEST_DEBUG
	uint16 rssi_iqpwr;
	int16 rssi_iqpwr_dB;
	int wait_count = 0;
	uint16 board_atten_dbg;

	while (wlapi_bmac_read_shm(pi->sh->physhim, lcnphyregs_shm_addr +
			M_RSSI_LOCK_OFFSET(pi))) {
		/* Check for timeout */
		if (wait_count > 100) { /* 1 ms */
			PHY_ERROR(("wl%d: %s: Unable to get RSSI_LOCK\n",
				pi->sh->unit, __FUNCTION__));
			goto iqest_rssi_skip;
		}
		OSL_DELAY(10);
		wait_count++;
	}
	wlapi_bmac_write_shm(pi->sh->physhim, lcnphyregs_shm_addr +
		M_RSSI_LOCK_OFFSET(pi), 1);
	rssi_iqpwr = wlapi_bmac_read_shm(pi->sh->physhim,
		lcnphyregs_shm_addr + M_RSSI_IQPWR_DBG_OFFSET(pi));
	rssi_iqpwr_dB = wlapi_bmac_read_shm(pi->sh->physhim,
		lcnphyregs_shm_addr + M_RSSI_IQPWR_DB_DBG_OFFSET(pi));
	board_atten_dbg = wlapi_bmac_read_shm(pi->sh->physhim,
		lcnphyregs_shm_addr + M_RSSI_BOARDATTEN_DBG_OFFSET(pi));
	wlapi_bmac_write_shm(pi->sh->physhim, lcnphyregs_shm_addr +
		M_RSSI_LOCK_OFFSET(pi), 0);
	printf("iqpwr = %d, iqpwr_dB = %d, board_atten = %d\n",
		rssi_iqpwr, rssi_iqpwr_dB, board_atten_dbg);
iqest_rssi_skip:
#endif /* RSSI_IQEST_DEBUG */

	rssi_addr[0] = lcnphyregs_shm_addr + M_SSLPN_RSSI_0_OFFSET(pi);
	rssi_addr[1] = lcnphyregs_shm_addr + M_SSLPN_RSSI_1_OFFSET(pi);
	rssi_addr[2] = lcnphyregs_shm_addr + M_SSLPN_RSSI_2_OFFSET(pi);
	rssi_addr[3] = lcnphyregs_shm_addr + M_SSLPN_RSSI_3_OFFSET(pi);

	rssi_qdb_addr[0] = lcnphyregs_shm_addr + M_RSSI_QDB_0_OFFSET(pi);
	rssi_qdb_addr[1] = lcnphyregs_shm_addr + M_RSSI_QDB_1_OFFSET(pi);
	rssi_qdb_addr[2] = lcnphyregs_shm_addr + M_RSSI_QDB_2_OFFSET(pi);
	rssi_qdb_addr[3] = lcnphyregs_shm_addr + M_RSSI_QDB_3_OFFSET(pi);

	stats.rssi = 0;

	for (i = 0; i < 4; i++) {
		rssi_lcn[i] = (int8)(wlapi_bmac_read_shm(pi->sh->physhim,
			rssi_addr[i]) & 0xFF);
		snr_lcn[i] = (int8)((wlapi_bmac_read_shm(pi->sh->physhim,
			rssi_addr[i]) >> 8) & 0xFF);
		BCM_REFERENCE(snr_lcn[i]);
		gidx[i] =
			(wlapi_bmac_read_shm(pi->sh->physhim, rssi_addr[i])
			& LCN40_RX_GAIN_INDEX_MASK) >> LCN40_RX_GAIN_INDEX_SHIFT;

		{
#if RSSI_CORR_EN
			int8 *corr2g;
	#ifdef BAND5G
			int8 *corr5g;
	#endif /* BAND5G */
			int8 *corrperrg;
			int8 po_reg;
			int16 po_nom;
#endif /* RSSI_CORR_EN */
			uint8 tr_iso = 0;

			if (pi_lcn40->rssi_iqest_en) {
				board_atten = wlapi_bmac_read_shm(pi->sh->physhim,
				rssi_addr[i]) >> 15;

				gain_resp[i] =
					wlapi_bmac_read_shm(pi->sh->physhim,
					rssi_qdb_addr[i]) >> 2;
				gidx_calc[i] = ((gain_resp[i] + 18)* 85) >> 8;
				if (gidx[i] > 37)
					gidx_calc[i] = gidx_calc[i] + 38;
				if (CHSPEC_IS2G(pi->radio_chanspec))
					tr_iso = pi_lcn->lcnphy_tr_isolation_mid;
#ifdef BAND5G
				else
					tr_iso = pi_lcn->triso5g[0];
#endif
				if (board_atten) {
					gidx_calc[i] = gidx_calc[i] + tr_iso;
				}
				gidx[i] = gidx_calc[i];

				rxpath_gain =
					wlc_lcn40phy_get_rxpath_gain_by_index(pi,
					gidx[i], board_atten);
				PHY_INFORM(("iqdB= %d, boardattn= %d, rxpath_gain= %d, "
					"gidx = %d, rssi_iqest_gain_adj = %d\n",
					rssi_lcn[i], board_atten, rxpath_gain, gidx[i],
					pi_lcn40->rssi_iqest_gain_adj));
				rssi_lcn40_qdb[i] =
					wlapi_bmac_read_shm(pi->sh->physhim,
					rssi_qdb_addr[i]) & 0x3;
				rssi_lcn[i] = (rssi_lcn[i] << LCN40_QDB_SHIFT)
					+ rssi_lcn40_qdb[i] - rxpath_gain +
					(pi_lcn40->rssi_iqest_gain_adj
					<< LCN40_QDB_SHIFT);
			}

#if RSSI_CORR_EN
		if (!pi_lcn40->rssi_iqest_en) {
			/* JSSI adjustment wrt power offset */
			if (CHSPEC_BW_LE20(pi->radio_chanspec))
				po_reg =
				PHY_REG_READ(pi, LCN40PHY,
				SignalBlockConfigTable6_new,
				crssignalblk_input_pwr_offset_db);
			else
				po_reg =
				PHY_REG_READ(pi, LCN40PHY,
				SignalBlockConfigTable5_new,
				crssignalblk_input_pwr_offset_db_40mhz);

			switch (wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec)) {
			case WL_CHAN_FREQ_RANGE_2G:
				if (CHSPEC_BW_LE20(pi->radio_chanspec))
					po_nom = pi_lcn->noise.nvram_input_pwr_offset_2g;
				else
					po_nom = pi_lcn->noise.nvram_input_pwr_offset_40_2g;
				break;
		#ifdef BAND5G
			case WL_CHAN_FREQ_RANGE_5GL:
				/* 5 GHz low */
				if (CHSPEC_BW_LE20(pi->radio_chanspec))
					po_nom = pi_lcn->noise.nvram_input_pwr_offset_5g[0];
				else
					po_nom =
					pi_lcn->noise.nvram_input_pwr_offset_40_5g[0];
				break;
			case WL_CHAN_FREQ_RANGE_5GM:
				/* 5 GHz middle */
				if (CHSPEC_BW_LE20(pi->radio_chanspec))
					po_nom = pi_lcn->noise.nvram_input_pwr_offset_5g[1];
				else
					po_nom =
					pi_lcn->noise.nvram_input_pwr_offset_40_5g[1];
				break;
			case WL_CHAN_FREQ_RANGE_5GH:
				/* 5 GHz high */
				if (CHSPEC_BW_LE20(pi->radio_chanspec))
					po_nom = pi_lcn->noise.nvram_input_pwr_offset_5g[2];
				else
					po_nom =
					pi_lcn->noise.nvram_input_pwr_offset_40_5g[2];
				break;
		#endif /* BAND5G */
			default:
				po_nom = po_reg;
				break;
			}

			rssi_lcn[i] += ((po_nom - po_reg) << LCN40_QDB_SHIFT);

			/* RSSI adjustment and Adding the JSSI range specific corrections */
			#ifdef BAND5G
			if (wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec) !=
				WL_CHAN_FREQ_RANGE_2G) {
					if (((rssi_lcn[i] >> LCN40_QDB_SHIFT) < -60) &&
						((gidx[i] > 0) && (gidx[i] < 38)))
				    rssi_lcn[i] +=
				      (phy_lcn40_get_pkt_rssi_gain_index_offset_5g(gidx[i])
				            << LCN40_QDB_SHIFT);
					corrperrg = pi->rssi_corr_perrg_5g;
				} else
			#endif /* BAND5G */
				{
					if (((rssi_lcn[i] >> LCN40_QDB_SHIFT) < -60) &&
						((gidx[i] > 0) && (gidx[i] < 38)))
				    rssi_lcn[i] +=
				      (phy_lcn40_get_pkt_rssi_gain_index_offset_2g(gidx[i])
				            << LCN40_QDB_SHIFT);
					corrperrg = pi->rssi_corr_perrg_2g;
				}

				if ((rssi_lcn[i] << LCN40_QDB_SHIFT) <= corrperrg[0])
					rssi_lcn[i] += (corrperrg[2] << LCN40_QDB_SHIFT);
				else if ((rssi_lcn[i] << LCN40_QDB_SHIFT) <= corrperrg[1])
					rssi_lcn[i] += (corrperrg[3] << LCN40_QDB_SHIFT);
				else
					rssi_lcn[i] += (corrperrg[4] << LCN40_QDB_SHIFT);

				corr2g = &(pi->rssi_corr_normal);
#ifdef BAND5G
				corr5g = &(pi->rssi_corr_normal_5g[0]);
#endif /* BAND5G */

			switch (wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec)) {
					case WL_CHAN_FREQ_RANGE_2G:
						rssi_lcn[i] = rssi_lcn[i] +
							(*corr2g  << LCN40_QDB_SHIFT);
						break;
				#ifdef BAND5G
					case WL_CHAN_FREQ_RANGE_5GL:
						/* 5 GHz low */
						rssi_lcn[i] = rssi_lcn[i] +
						(corr5g[0] << LCN40_QDB_SHIFT);
						break;

					case WL_CHAN_FREQ_RANGE_5GM:
						/* 5 GHz middle */
						rssi_lcn[i] = rssi_lcn[i] +
						(corr5g[1] << LCN40_QDB_SHIFT);
						break;

					case WL_CHAN_FREQ_RANGE_5GH:
						/* 5 GHz high */
						rssi_lcn[i] = rssi_lcn[i] +
						(corr5g[2] << LCN40_QDB_SHIFT);
						break;
				#endif /* BAND5G */
					default:
						rssi_lcn[i] = rssi_lcn[i] + 0;
						break;
				}
			}

			/* Temp sense based correction */
			rssi_lcn[i] += wlc_lcn40phy_rssi_tempcorr(pi, 0);
			if (pi_lcn40->rssi_iqest_en)
				rssi_lcn[i] +=
				wlc_lcn40phy_iqest_rssi_tempcorr(pi, 0, board_atten);

#endif /* RSSI_CORR_EN */
		}
		stats.rssi += rssi_lcn[i];
	}

	stats.rssi = stats.rssi >> 2;

	/* temperature compensation */
	stats.rssi = stats.rssi + (pi_lcn->lcnphy_pkteng_rssi_slope << LCN40_QDB_SHIFT);

	/* convert into dB and save qdB portion */
	stats.rssi_qdb = stats.rssi & LCN40_QDB_MASK;
	stats.rssi = stats.rssi >> LCN40_QDB_SHIFT;

	/* SNR */
	snr_a_addr[0] = lcnphyregs_shm_addr + M_SSLPN_SNR_0_logchPowAccOut_OFFSET(pi);
	snr_a_addr[1] = lcnphyregs_shm_addr + M_SSLPN_SNR_1_logchPowAccOut_OFFSET(pi);
	snr_a_addr[2] = lcnphyregs_shm_addr + M_SSLPN_SNR_2_logchPowAccOut_OFFSET(pi);
	snr_a_addr[3] = lcnphyregs_shm_addr + M_SSLPN_SNR_3_logchPowAccOut_OFFSET(pi);

	snr_b_addr[0] = lcnphyregs_shm_addr + M_SSLPN_SNR_0_errAccOut_OFFSET(pi);
	snr_b_addr[1] = lcnphyregs_shm_addr + M_SSLPN_SNR_1_errAccOut_OFFSET(pi);
	snr_b_addr[2] = lcnphyregs_shm_addr + M_SSLPN_SNR_2_errAccOut_OFFSET(pi);
	snr_b_addr[3] = lcnphyregs_shm_addr + M_SSLPN_SNR_3_errAccOut_OFFSET(pi);

	stats.snr = 0;
	for (i = 0; i < 4; i++) {
		snr_a_lcn[i] = wlapi_bmac_read_shm(pi->sh->physhim, snr_a_addr[i]);
		snr_b_lcn[i] = wlapi_bmac_read_shm(pi->sh->physhim, snr_b_addr[i]);
		snr[i] = ((snr_a_lcn[i] - snr_b_lcn[i])* 3) >> 5;
		if (snr[i] > 31)
			snr[i] = 31;
		stats.snr += snr[i];
		PHY_INFORM(("i = %d, gidx = %d, snr = %d, snr_lcn = %d\n",
			i, phy_lcn_get_pkt_rssi_gain_index_offset(gidx[i]),
			snr[i], snr_lcn[i]));
	}
	stats.snr = stats.snr >> 2;

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

static int
phy_lcn40_rssi_set_gain_delta_2g(phy_type_rssi_ctx_t *ctx, uint32 aid, int8 *deltaValues)
{
	phy_lcn40_rssi_info_t *rssii = (phy_lcn40_rssi_info_t *)ctx;
	phy_info_lcn40phy_t *pi_lcn40 = rssii->lcn40i;
	uint8 i;
	int8 *rssi_gain_delta;

	if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2G)) {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_2g;
	} else if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GH)) {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_2gh;
	} else {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_2ghh;
	}

	for (i = 0; i < LCN40PHY_GAIN_DELTA_2G_PARAMS; i++) {
		rssi_gain_delta[i] = deltaValues[i];
	}
	return BCME_OK;
}

static int
phy_lcn40_rssi_get_gain_delta_2g(phy_type_rssi_ctx_t *ctx, uint32 aid, int8 *deltaValues)
{
	phy_lcn40_rssi_info_t *rssii = (phy_lcn40_rssi_info_t *)ctx;
	phy_info_lcn40phy_t *pi_lcn40 = rssii->lcn40i;
	uint8 i;
	int8 *rssi_gain_delta;

	if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2G)) {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_2g;
	} else if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_2GH)) {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_2gh;
	} else {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_2ghh;
	}

	for (i = 0; i < LCN40PHY_GAIN_DELTA_2G_PARAMS; i++)
		deltaValues[i] = rssi_gain_delta[i];

	return BCME_OK;
}

static int
phy_lcn40_rssi_set_gain_delta_5g(phy_type_rssi_ctx_t *ctx, uint32 aid, int8 *deltaValues)
{
	phy_lcn40_rssi_info_t *rssii = (phy_lcn40_rssi_info_t *)ctx;
	phy_info_lcn40phy_t *pi_lcn40 = rssii->lcn40i;
	uint8 i;
	int8 *rssi_gain_delta;

	if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_5GL)) {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_5gl;
	} else if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_5GML)) {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_5gml;
	} else if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_5GMU)) {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_5gmu;
	} else {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_5gh;
	}

	for (i = 0; i < LCN40PHY_GAIN_DELTA_5G_PARAMS; i++) {
		rssi_gain_delta[i] = deltaValues[i];
	}
	return BCME_OK;
}

static int
phy_lcn40_rssi_get_gain_delta_5g(phy_type_rssi_ctx_t *ctx, uint32 aid, int8 *deltaValues)
{
	phy_lcn40_rssi_info_t *rssii = (phy_lcn40_rssi_info_t *)ctx;
	phy_info_lcn40phy_t *pi_lcn40 = rssii->lcn40i;
	uint8 i;
	int8 *rssi_gain_delta;

	if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_5GL)) {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_5gl;
	} else if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_5GML)) {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_5gml;
	} else if (aid == IOV_SVAL(IOV_PHY_RSSI_GAIN_DELTA_5GMU)) {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_5gmu;
	} else {
		rssi_gain_delta = pi_lcn40->rssi_gain_delta_5gh;
	}

	for (i = 0; i < LCN40PHY_GAIN_DELTA_5G_PARAMS; i++) {
		deltaValues[i] = rssi_gain_delta[i];
	}
	return BCME_OK;
}
