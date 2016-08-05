/*
 * PHY and RADIO specific portion of Broadcom BCM43XX 802.11abg
 * PHY iovar processing of Broadcom BCM43XX 802.11abg
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
 * $Id: wlc_phy_iovar.c 455451 2014-02-14 01:22:35Z $
 */

/*
 * This file contains high portion PHY iovar processing and table.
 */

#include <wlc_cfg.h>

#ifdef WLC_HIGH
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#ifndef PHYMOD3_TRUNK_MERGE
#include <wlc_key.h>
#endif
#include <wlc_pub.h>
#ifndef PHYMOD3_TRUNK_MERGE
#include <wlc_bsscfg.h>
#endif
#include <wl_dbg.h>
#include <wlc.h>
#include <bcmwifi_channels.h>

const bcm_iovar_t phy_iovars[] = {
	/* OLD, PHYTYPE specific iovars, to phase out, use unified ones at the end of this array */
#if NCONF	/* move some to internal ?? */
#endif /* NCONF */


#if LPCONF
	{"lpphy_tempsense", IOV_LPPHY_TEMPSENSE,
	IOVF_GET_UP, IOVT_INT32, 0
	},
	{"lpphy_cal_delta_temp", IOV_LPPHY_CAL_DELTA_TEMP,
	0, IOVT_INT32, 0
	},
	{"lpphy_vbatsense", IOV_LPPHY_VBATSENSE,
	IOVF_GET_UP, IOVT_INT32, 0
	},
	{"lpphy_rx_gain_temp_adj_tempsense", IOV_LPPHY_RX_GAIN_TEMP_ADJ_TEMPSENSE,
	(0), IOVT_INT8, 0
	},
	{"lpphy_rx_gain_temp_adj_thresh", IOV_LPPHY_RX_GAIN_TEMP_ADJ_THRESH,
	(0), IOVT_UINT32, 0
	},
	{"lpphy_rx_gain_temp_adj_metric", IOV_LPPHY_RX_GAIN_TEMP_ADJ_METRIC,
	(0), IOVT_INT16, 0
	},
#endif /* LPCONF */

	/* ==========================================
	 * unified phy iovar, independent of PHYTYPE
	 * ==========================================
	 */
#ifndef PHYMOD3_TRUNK_MERGE
#if defined(AP) && defined(RADAR)
	{"radarargs", IOV_RADAR_ARGS,
	(0), IOVT_BUFFER, sizeof(wl_radar_args_t)
	},
	{"radarargs40", IOV_RADAR_ARGS_40MHZ,
	(0), IOVT_BUFFER, sizeof(wl_radar_args_t)
	},
	{"radarthrs", IOV_RADAR_THRS,
	(IOVF_SET_UP), IOVT_BUFFER, sizeof(wl_radar_thr_t)
	},
	{"radar_status", IOV_RADAR_STATUS,
	(0), IOVT_BUFFER, sizeof(wl_radar_status_t)
	},
	{"clear_radar_status", IOV_CLEAR_RADAR_STATUS,
	(IOVF_SET_UP), IOVT_BUFFER, sizeof(wl_radar_status_t)
	},
#if defined(RADAR)
	{"phy_dfs_lp_buffer", IOV_PHY_DFS_LP_BUFFER,
	0, IOVT_UINT8, 0
	},
#endif 
#endif /* AP && RADAR */
#endif /* PHYMOD3_TRUNK_MERGE */
	{"cal_period", IOV_CAL_PERIOD,
	0, IOVT_UINT32, 0
	},
#if defined(WLMEDIA_CALDBG) || defined(PHYCAL_CHNG_CS) || defined(WLMEDIA_FLAMES) || \
	defined(DBG_PHY_IOV)
	{"glacial_timer", IOV_GLACIAL_TIMER,
	IOVF_NTRL, IOVT_UINT32, 0
	},
#endif  
#if defined(WLMEDIA_CALDBG) || defined(PHYCAL_CHNG_CS) || defined(WLMEDIA_FLAMES)
	{"pkteng_gainindex", IOV_PKTENG_GAININDEX,
	IOVF_GET_UP, IOVT_UINT8, 0
	},
	{"hirssi_period", IOV_HIRSSI_PERIOD,
	(IOVF_MFG), IOVT_INT16, 0
	},
	{"hirssi_en", IOV_HIRSSI_EN,
	0, IOVT_UINT8, 0
	},
	{"hirssi_byp_rssi", IOV_HIRSSI_BYP_RSSI,
	(IOVF_MFG), IOVT_INT8, 0
	},
	{"hirssi_res_rssi", IOV_HIRSSI_RES_RSSI,
	(IOVF_MFG), IOVT_INT8, 0
	},
	{"hirssi_byp_w1cnt", IOV_HIRSSI_BYP_CNT,
	(IOVF_NTRL | IOVF_MFG), IOVT_UINT16, 0
	},
	{"hirssi_res_w1cnt", IOV_HIRSSI_RES_CNT,
	(IOVF_NTRL | IOVF_MFG), IOVT_UINT16, 0
	},
	{"hirssi_status", IOV_HIRSSI_STATUS,
	0, IOVT_UINT8, 0
	},
#endif 


#ifdef SAMPLE_COLLECT
	{"sample_collect", IOV_PHY_SAMPLE_COLLECT,
	(IOVF_GET_CLK), IOVT_BUFFER, WLC_SAMPLECOLLECT_MAXLEN
	},
	{"sample_data", IOV_PHY_SAMPLE_DATA,
	(IOVF_GET_CLK), IOVT_BUFFER, WLC_SAMPLECOLLECT_MAXLEN
	},
	{"sample_collect_gainadj", IOV_PHY_SAMPLE_COLLECT_GAIN_ADJUST,
	0, IOVT_INT8, 0
	},
	{"mac_triggered_sample_collect", IOV_PHY_MAC_TRIGGERED_SAMPLE_COLLECT,
	0, IOVT_BUFFER, WLC_SAMPLECOLLECT_MAXLEN
	},
	{"mac_triggered_sample_data", IOV_PHY_MAC_TRIGGERED_SAMPLE_DATA,
	0, IOVT_BUFFER, WLC_SAMPLECOLLECT_MAXLEN
	},
	{"sample_collect_gainidx", IOV_PHY_SAMPLE_COLLECT_GAIN_INDEX,
	0, IOVT_UINT8, 0
	},
	{"iq_metric_data", IOV_IQ_IMBALANCE_METRIC_DATA,
	(IOVF_GET_DOWN | IOVF_GET_CLK), IOVT_BUFFER, WLC_SAMPLECOLLECT_MAXLEN
	},
	{"iq_metric", IOV_IQ_IMBALANCE_METRIC,
	(IOVF_GET_DOWN | IOVF_GET_CLK), IOVT_BUFFER, 0
	},
	{"iq_metric_pass", IOV_IQ_IMBALANCE_METRIC_PASS,
	(IOVF_GET_DOWN | IOVF_GET_CLK), IOVT_BUFFER, 0
	},
#endif /* SAMPLE COLLECT */
	{"phy_muted", IOV_PHY_MUTED,
	0, IOVT_UINT8, 0
	},
	{"pavars", IOV_PAVARS,
	(IOVF_SET_DOWN), IOVT_BUFFER, WL_PHY_PAVARS_LEN * sizeof(uint16)
	},
	{"paparambwver", IOV_PA_BW_VER,
	(IOVF_SET_DOWN), IOVT_UINT8, 0
	},
#if defined(WLMEDIA_CALDBG)
	{"phy_cal_disable", IOV_PHY_CAL_DISABLE,
	(IOVF_MFG), IOVT_UINT8, 0
	},
#endif 
#if defined(WLMEDIA_N2DBG) || defined(WLMEDIA_N2DEV) || defined(DBG_PHY_IOV)
	{"phy_watchdog", IOV_PHY_WATCHDOG,
	(IOVF_MFG), IOVT_UINT8, 0
	},
#endif
#if defined(DBG_PHY_IOV)
	{"phytable", IOV_PHYTABLE,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), IOVT_BUFFER, 4*4
	},
	{"phy_dynamic_ml", IOV_PHY_DYN_ML,
	(IOVF_SET_UP | IOVF_MFG), IOVT_UINT8, 0
	},
	{"aci_nams", IOV_PHY_ACI_NAMS,
	(IOVF_SET_UP | IOVF_MFG), IOVT_UINT8, 0
	},
	{"phy_rf_swctrl_toggle", IOV_PHY_RF_SWCTRL_TOGGLE,
	(IOVF_SET_UP | IOVF_MFG), IOVT_UINT8, 0
	},
#endif  
#if defined(WLMEDIA_N2DBG) || defined(WLMEDIA_N2DEV) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG) || defined(ATE_BUILD)
	{"phy_forcecal", IOV_PHY_FORCECAL,
#ifdef WFD_PHY_LL_DEBUG
	IOVF_SET_UP, IOVT_UINT8, 0
#else
	(IOVF_SET_UP | IOVF_MFG), IOVT_UINT8, 0
#endif /* WFD_PHY_LL_DEBUG */
	},
#ifndef ATE_BUILD
	{"phy_forcecal_obt", IOV_PHY_FORCECAL_OBT,
	(IOVF_SET_UP | IOVF_MFG), IOVT_UINT8, 0
	},
	{"phy_forcecal_noise", IOV_PHY_FORCECAL_NOISE,
	(IOVF_SET_UP | IOVF_MFG), IOVT_BUFFER, sizeof(uint16)
	},
	{"phy_skippapd", IOV_PHY_SKIPPAPD,
	(IOVF_SET_DOWN | IOVF_MFG), IOVT_UINT8, 0
	},
#endif /* !ATE_BUILD */
#endif 
	{"phy_glitchthrsh", IOV_PHY_GLITCHK,
	(IOVF_MFG), IOVT_UINT8, 0
	},
	{"phy_noise_up", IOV_PHY_NOISE_UP,
	(IOVF_MFG), IOVT_UINT8, 0
	},
	{"phy_noise_dwn", IOV_PHY_NOISE_DWN,
	(IOVF_MFG), IOVT_UINT8, 0
	},
#if defined(ATE_BUILD)
	{"phy_txpwrctrl", IOV_PHY_TXPWRCTRL,
	(IOVF_MFG), IOVT_UINT8, 0
	},
	{"phy_txpwrindex", IOV_PHY_TXPWRINDEX,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), IOVT_BUFFER, 0
	},
	{"phy_txiqcc", IOV_PHY_TXIQCC,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), IOVT_BUFFER,  2*sizeof(int32)
	},
	{"phy_txlocc", IOV_PHY_TXLOCC,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), IOVT_BUFFER, 6
	},
	{"phy_tx_tone", IOV_PHY_TX_TONE,
	(IOVF_SET_UP | IOVF_MFG), IOVT_UINT32, 0
	},
	{"phy_tx_tone_hz", IOV_PHY_TX_TONE_HZ,
	(IOVF_SET_UP | IOVF_MFG), IOVT_UINT32, 0
	},
	{"phy_tx_tone_stop", IOV_PHY_TX_TONE_STOP,
	(IOVF_SET_UP | IOVF_MFG), IOVT_UINT8, 0
	},
#endif 
#ifdef PHYMON
	{"phycal_state", IOV_PHYCAL_STATE,
	IOVF_GET_UP, IOVT_UINT32, 0,
	},
#endif /* PHYMON */
#if defined(AP) || defined(WLAWDL)
	{"phy_percal", IOV_PHY_PERICAL,
	(0), IOVT_UINT8, 0
	},
#endif 
	{"phy_percal_delay", IOV_PHY_PERICAL_DELAY,
	(0), IOVT_UINT16, 0
	},
	{"phy_force_crsmin", IOV_PHY_FORCE_CRSMIN,
	IOVF_SET_UP, IOVT_BUFFER, 4*sizeof(int8)
	},
	{"phy_rxiqest", IOV_PHY_RXIQ_EST,
	IOVF_SET_UP, IOVT_UINT32, IOVT_UINT32
	},
#if ACCONF
	{"edcrs", IOV_EDCRS,
	(IOVF_SET_UP|IOVF_GET_UP), IOVT_UINT8, 0
	},
	{"lp_mode", IOV_LP_MODE,
	(IOVF_SET_UP|IOVF_GET_UP), IOVT_UINT8, 0
	},
	{"lp_vco_2g", IOV_LP_VCO_2G,
	(IOVF_SET_UP|IOVF_GET_UP), IOVT_UINT8, 0
	},
	{"smth_enable", IOV_SMTH,
	(IOVF_SET_UP|IOVF_GET_UP), IOVT_UINT8, 0
	},
	{"radio_pd", IOV_RADIO_PD,
	(IOVF_SET_UP|IOVF_GET_UP), IOVT_UINT8, 0
	},
#endif /* If defined ACPHY */
	{"phynoise_srom", IOV_PHYNOISE_SROM,
	IOVF_GET_UP, IOVT_UINT32, 0
	},
	{"num_stream", IOV_NUM_STREAM,
	(0), IOVT_INT32, 0
	},
	{"band_range", IOV_BAND_RANGE,
	0, IOVT_INT8, 0
	},
	{"subband5gver", IOV_PHY_SUBBAND5GVER,
	0, IOVT_INT8, 0
	},
	{"min_txpower", IOV_MIN_TXPOWER,
	0, IOVT_UINT32, 0
	},
	{"ant_diversity_sw_core0", IOV_ANT_DIV_SW_CORE0,
	(IOVF_SET_UP|IOVF_GET_UP), IOVT_UINT8, 0
	},
	{"ant_diversity_sw_core1", IOV_ANT_DIV_SW_CORE1,
	(IOVF_SET_UP|IOVF_GET_UP), IOVT_UINT8, 0
	},
#if defined(ATE_BUILD)
	{"phy_tempsense", IOV_PHY_TEMPSENSE,
	IOVF_GET_UP, IOVT_INT16, 0
	},
#endif
#if defined(WLMEDIA_N2DEV) || defined(WLMEDIA_N2DBG) || defined(RXDESENS_EN)
	{"phy_rxdesens", IOV_PHY_RXDESENS,
	IOVF_GET_UP, IOVT_INT32, 0
	},
#endif
#if defined(WLMEDIA_N2DEV) || defined(WLMEDIA_N2DBG)
	{"ntd_gds_lowtxpwr", IOV_NTD_GDS_LOWTXPWR,
	IOVF_GET_UP, IOVT_UINT8, 0
	},
	{"papdcal_indexdelta", IOV_PAPDCAL_INDEXDELTA,
	IOVF_GET_UP, IOVT_UINT8, 0
	},
#endif
	{"phy_oclscdenable", IOV_PHY_OCLSCDENABLE,
	(IOVF_MFG), IOVT_UINT8, 0
	},
	{"lnldo2", IOV_LNLDO2,
	(IOVF_MFG), IOVT_UINT8, 0
	},
	{"phy_rxantsel", IOV_PHY_RXANTSEL,
	(0), IOVT_UINT8, 0
	},
#ifdef ENABLE_FCBS
	{"phy_fcbs_init", IOV_PHY_FCBSINIT,
	(IOVF_SET_UP), IOVT_INT8, 0
	},
	{"phy_fcbs", IOV_PHY_FCBS,
	(IOVF_SET_UP | IOVF_GET_UP), IOVT_UINT8, 0
	},
	{"phy_fcbs_arm", IOV_PHY_FCBSARM,
	(IOVF_SET_UP), IOVT_UINT8, 0
	},
	{"phy_fcbs_exit", IOV_PHY_FCBSEXIT,
	(IOVF_SET_UP), IOVT_UINT8, 0
	},
#endif /* ENABLE_FCBS */
	{"phy_crs_war", IOV_PHY_CRS_WAR,
	(0), IOVT_INT8, 0
	},
	{"subband_idx", IOV_BAND_RANGE,
	0, IOVT_INT8, 0
	},
#if LCN40CONF
	{"noise_measure_disable", IOV_NOISE_MEASURE_DISABLE,
	(IOVF_MFG), IOVT_INT32, 0
	},
#endif
	{"pavars2", IOV_PAVARS2,
	(IOVF_SET_DOWN | IOVF_MFG), IOVT_BUFFER, sizeof(wl_pavars2_t)
	},
	{"phy_btc_restage_rxgain", IOV_PHY_BTC_RESTAGE_RXGAIN,
	IOVF_SET_UP, IOVT_UINT32, 0
	},
	{"phy_dssf", IOV_PHY_DSSF,
	IOVF_SET_UP, IOVT_UINT32, 0
	},
	{"phy_ed_thresh", IOV_ED_THRESH,
	(IOVF_SET_UP | IOVF_GET_UP), IOVT_INT32, 0
	},
#ifdef WL_SARLIMIT
	{"phy_sarlimit", IOV_PHY_SAR_LIMIT,
	0, IOVT_UINT32, 0
	},
	{"phy_txpwr_core", IOV_PHY_TXPWR_CORE,
	0, IOVT_UINT32, 0
	},
#endif /* WL_SARLIMIT */
	{"phy_txswctrlmap", IOV_PHY_TXSWCTRLMAP,
	0, IOVT_INT8, 0
	},
#ifdef WFD_PHY_LL
	{"phy_wfd_ll_enable", IOV_PHY_WFD_LL_ENABLE,
	0, IOVT_UINT8, 0
	},
#endif /* WFD_PHY_LL */
	{"phy_sromtempsense", IOV_PHY_SROM_TEMPSENSE,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), IOVT_INT16, 0
	},
	{"rxg_rssi", IOV_PHY_RXGAIN_RSSI,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), IOVT_INT16, 0
	},
	{"rssi_cal_rev", IOV_PHY_RSSI_CAL_REV,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), IOVT_INT16, 0
	},
	{"rud_agc_enable", IOV_PHY_RUD_AGC_ENABLE,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), IOVT_INT16, 0
	},
	{"gain_cal_temp", IOV_PHY_GAIN_CAL_TEMP,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), IOVT_INT16, 0
	},
	{"temp_comp_trloss", IOV_PHY_TEMP_COMP_TRLOSS,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), IOVT_INT16, 0
	},
#if ACCONF
	{"phy_afeoverride", IOV_PHY_AFE_OVERRIDE,
	(0), IOVT_UINT8, 0
	},
#endif
	{"phy_ocl_force_core0", IOV_PHY_OCL_FORCE_CORE0,
	0, IOVT_INT8, 0
	},
	/* terminating element, only add new before this */
	{NULL, 0, 0, 0, 0 }
};

#ifndef PHYMOD3_TRUNK_MERGE
#include <wlc_phy_iovar.h>
#include <wlc_bmac.h>
static int
wlc_phy_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *p, uint plen, void *a, int alen, int vsize, struct wlc_if *wlcif)
{
	wlc_info_t *wlc = (wlc_info_t *)hdl;

	return wlc_bmac_phy_iovar_dispatch(wlc->hw, actionid, vi->type, p, plen, a, alen, vsize);
}

/* PHY is no longer a wlc.c module. It's slave to wlc_bmac.c
 * still use module_register to move iovar processing to HIGH driver to save dongle space
 */
int
BCMATTACHFN(wlc_phy_iovar_attach)(void *pub)
{
	/* register module */
	if (wlc_module_register((wlc_pub_t*)pub, phy_iovars, "phy", ((wlc_pub_t*)pub)->wlc,
		wlc_phy_doiovar, NULL, NULL, NULL)) {
		WL_ERROR(("%s: wlc_phy_iovar_attach failed!\n", __FUNCTION__));
		return -1;
	}
	return 0;
}

void
BCMATTACHFN(wlc_phy_iovar_detach)(void *pub)
{
	wlc_module_unregister(pub, "phy", ((wlc_pub_t*)pub)->wlc);
}
#endif /* PHYMOD3_TRUNK_MERGE */
#endif /* WLC_HIGH */

#ifdef PHYMOD3_TRUNK_MERGE
#ifdef WLC_LOW
#include <typedefs.h>
#include <wlc_phy_int.h>

static int
phy_legacy_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize)
{
	return wlc_phy_iovar_dispatch((phy_info_t *)ctx, aid, -1, p, plen, a, alen, vsize);
}
#endif /* WLC_LOW */

#ifdef WLC_HIGH_ONLY
#include <bcm_xdr.h>

static bool
phy_legacy_pack_iov(wlc_info_t *wlc, uint32 aid, void *p, uint p_len, bcm_xdr_buf_t *b)
{
	int err;

	/* Decide the buffer is 16-bit or 32-bit buffer */
	switch (IOV_ID(aid)) {
	case IOV_PKTENG_STATS:
	case IOV_PHYTABLE:
	case IOV_POVARS:
		p_len &= ~3;
		err = bcm_xdr_pack_uint32(b, p_len);
		ASSERT(!err);
		err = bcm_xdr_pack_uint32_vec(b, p_len, p);
		ASSERT(!err);
		return TRUE;
	case IOV_PAVARS:
		p_len &= ~1;
		err = bcm_xdr_pack_uint32(b, p_len);
		ASSERT(!err);
		err = bcm_xdr_pack_uint16_vec(b, p_len, p);
		ASSERT(!err);
		return TRUE;
	}
	return FALSE;
}

static bool
phy_legacy_unpack_iov(wlc_info_t *wlc, uint32 aid, bcm_xdr_buf_t *b, void *a, uint a_len)
{
	/* Dealing with all the structures/special cases */
	switch (aid) {
	case IOV_GVAL(IOV_PHY_SAMPLE_COLLECT):
		WL_ERROR(("%s: nphy_sample_collect need endianess conversion code\n",
		          __FUNCTION__));
		break;
	case IOV_GVAL(IOV_PHY_SAMPLE_DATA):
		WL_ERROR(("%s: nphy_sample_data need endianess conversion code\n",
		          __FUNCTION__));
		break;
	}
	return FALSE;
}
#endif /* WLC_HIGH_ONLY */

/* register iovar table to the system */
#include <phy_api.h>

#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>

int phy_legacy_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

int
BCMATTACHFN(phy_legacy_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_iovars,
	                   phy_legacy_pack_iov, phy_legacy_unpack_iov,
	                   phy_legacy_doiovar, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
#endif /* PHYMOD3_TRUNK_MERGE */
