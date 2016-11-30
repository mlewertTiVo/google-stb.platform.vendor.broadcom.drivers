/*
 * PHY and RADIO specific portion of Broadcom BCM43XX 802.11abg
 * PHY iovar processing of Broadcom BCM43XX 802.11abg
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_phy_iovar.c 650807 2016-07-22 15:01:33Z mvermeid $
 */

/*
 * This file contains high portion PHY iovar processing and table.
 */

#include <wlc_cfg.h>

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <phy_dbg.h>

#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>

/* ************************************************************************************ */
/* [PHY_REARCH] Do not add any variables here. Add them to the individual modules */
/* ************************************************************************************ */
/* OLD, PHYTYPE specific iovars, to phase out */
static const bcm_iovar_t phy_iovars[] = {
#if NCONF
#if defined(BCMDBG)
	{"nphy_initgain", IOV_NPHY_INITGAIN,
	IOVF_SET_UP, 0, IOVT_UINT16, 0
	},
	{"nphy_hpv1gain", IOV_NPHY_HPVGA1GAIN,
	IOVF_SET_UP, 0, IOVT_INT8, 0
	},
	{"nphy_tx_temp_tone", IOV_NPHY_TX_TEMP_TONE,
	IOVF_SET_UP, 0, IOVT_UINT32, 0
	},
	{"nphy_cal_reset", IOV_NPHY_CAL_RESET,
	IOVF_SET_UP, 0, IOVT_UINT32, 0
	},
	{"nphy_est_tonepwr", IOV_NPHY_EST_TONEPWR,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
	{"phy_est_tonepwr", IOV_PHY_EST_TONEPWR,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
	{"nphy_rfseq_txgain", IOV_NPHY_RFSEQ_TXGAIN,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
	{"phy_spuravoid", IOV_PHY_SPURAVOID,
	(IOVF_SET_DOWN | IOVF_MFG), 0, IOVT_INT8, 0
	},
#endif /* defined(BCMDBG) */

#if defined(BCMINTERNAL) || defined(WLTEST)
	{"nphy_cckpwr_offset", IOV_NPHY_CCK_PWR_OFFSET,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"nphy_cal_sanity", IOV_NPHY_CAL_SANITY,
	IOVF_SET_UP, 0, IOVT_UINT32, 0
	},
	{"nphy_txiqlocal", IOV_NPHY_TXIQLOCAL,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"nphy_rxiqcal", IOV_NPHY_RXIQCAL,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"nphy_rxcalparams", IOV_NPHY_RXCALPARAMS,
	(0), 0, IOVT_UINT32, 0
	},
	{"nphy_rssisel", IOV_NPHY_RSSISEL,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"nphy_rssical", IOV_NPHY_RSSICAL,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phy_gpiosel", IOV_PHY_GPIOSEL,
	(IOVF_MFG), 0, IOVT_UINT16, 0
	},
	{"nphy_gain_boost", IOV_NPHY_GAIN_BOOST,
	(IOVF_SET_DOWN | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"nphy_elna_gain_config", IOV_NPHY_ELNA_GAIN_CONFIG,
	(IOVF_SET_DOWN), 0, IOVT_UINT8, 0
	},
	{"nphy_aci_scan", IOV_NPHY_ACI_SCAN,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"nphy_pacaltype", IOV_NPHY_PAPDCALTYPE,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"nphy_papdcal", IOV_NPHY_PAPDCAL,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"nphy_skippapd", IOV_NPHY_SKIPPAPD,
	(IOVF_SET_DOWN | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"nphy_pacalindex", IOV_NPHY_PAPDCALINDEX,
	(IOVF_MFG), 0, IOVT_UINT16, 0
	},
	{"nphy_caltxgain", IOV_NPHY_CALTXGAIN,
	(IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"nphy_tbldump_minidx", IOV_NPHY_TBLDUMP_MINIDX,
	(IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"nphy_tbldump_maxidx", IOV_NPHY_TBLDUMP_MAXIDX,
	(IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"nphy_phyreg_skipdump", IOV_NPHY_PHYREG_SKIPDUMP,
	(IOVF_MFG), 0, IOVT_UINT16, 0
	},
	{"nphy_phyreg_skipcount", IOV_NPHY_PHYREG_SKIPCNT,
	(IOVF_MFG), 0, IOVT_INT8, 0
	},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#endif /* NCONF */
#if defined(BCMDBG) || defined(WLTEST)
	{"fast_timer", IOV_FAST_TIMER,
	(IOVF_NTRL | IOVF_MFG), 0, IOVT_UINT32, 0
	},
	{"slow_timer", IOV_SLOW_TIMER,
	(IOVF_NTRL | IOVF_MFG), 0, IOVT_UINT32, 0
	},
#endif /* BCMDBG || WLTEST */
#if defined(BCMDBG) || defined(WLTEST) || defined(PHYCAL_CHNG_CS)
	{"glacial_timer", IOV_GLACIAL_TIMER,
	IOVF_NTRL, 0, IOVT_UINT32, 0
	},
#endif
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(MACOSX) || defined(DBG_PHY_IOV)
	{"phy_watchdog", IOV_PHY_WATCHDOG,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif
	{"cal_period", IOV_CAL_PERIOD,
	0, 0, IOVT_UINT32, 0
	},
#if defined(BCMINTERNAL) || defined(WLTEST)
#ifdef BAND5G
	{"phy_cga_5g", IOV_PHY_CGA_5G,
	IOVF_SET_UP, 0, IOVT_BUFFER, 24*sizeof(int8)
	},
#endif /* BAND5G */
	{"phy_cga_2g", IOV_PHY_CGA_2G,
	IOVF_SET_UP, 0, IOVT_BUFFER, 14*sizeof(int8)
	},
	{"phymsglevel", IOV_PHYHAL_MSG,
	(0), 0, IOVT_UINT32, 0
	},
	{"phy_fixed_noise", IOV_PHY_FIXED_NOISE,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phynoise_polling", IOV_PHYNOISE_POLL,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"carrier_suppress", IOV_CARRIER_SUPPRESS,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
#ifdef BAND5G
	{"subband5gver", IOV_PHY_SUBBAND5GVER,
	0, 0, IOVT_INT8, 0
	},
#endif /* BAND5G */
	{"phy_txrx_chain", IOV_PHY_TXRX_CHAIN,
	(0), 0, IOVT_INT8, 0
	},
	{"phy_bphy_evm", IOV_PHY_BPHY_EVM,
	(IOVF_SET_DOWN | IOVF_SET_BAND | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phy_bphy_rfcs", IOV_PHY_BPHY_RFCS,
	(IOVF_SET_DOWN | IOVF_SET_BAND | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phy_scraminit", IOV_PHY_SCRAMINIT,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"phy_rfseq", IOV_PHY_RFSEQ,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phy_tx_tone_hz", IOV_PHY_TX_TONE_HZ,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT32, 0
	},
	{"phy_tx_tone_symm", IOV_PHY_TX_TONE_SYMM,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT32, 0
	},
	{"phy_tx_tone_stop", IOV_PHY_TX_TONE_STOP,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phy_test_tssi", IOV_PHY_TEST_TSSI,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"phy_test_tssi_offs", IOV_PHY_TEST_TSSI_OFFS,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"phy_test_idletssi", IOV_PHY_TEST_IDLETSSI,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"phy_setrptbl", IOV_PHY_SETRPTBL,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_VOID, 0
	},
	{"phy_forceimpbf", IOV_PHY_FORCEIMPBF,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_VOID, 0
	},
	{"phy_forcesteer", IOV_PHY_FORCESTEER,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
#ifdef BAND5G
	{"phy_5g_pwrgain", IOV_PHY_5G_PWRGAIN,
	(IOVF_SET_DOWN | IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif /* BAND5G */
	{"phy_enrxcore", IOV_PHY_ENABLERXCORE,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phy_activecal", IOV_PHY_ACTIVECAL,
	IOVF_GET_UP, 0, IOVT_UINT8, 0
	},
	{"phy_bbmult", IOV_PHY_BBMULT,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, 0
	},
	{"phy_lowpower_beacon_mode", IOV_PHY_LOWPOWER_BEACON_MODE,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT32, 0
	},
#endif /* BCMINTERNAL || WLTEST */
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"pkteng_gainindex", IOV_PKTENG_GAININDEX,
	IOVF_GET_UP, 0, IOVT_UINT8, 0
	},
#endif  /* BCMINTERNAL || WLTEST */
	{"phy_rx_gainindex", IOV_PHY_RXGAININDEX,
	IOVF_GET_UP | IOVF_SET_UP, 0, IOVT_UINT16, 0
	},
	{"phy_forcecal", IOV_PHY_FORCECAL,
	IOVF_SET_UP, 0, IOVT_UINT8, 0
	},
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG)
#ifndef ATE_BUILD
	{"phy_forcecal_obt", IOV_PHY_FORCECAL_OBT,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif /* !ATE_BUILD */
#endif /* BCMINTERNAL || WLTEST || DBG_PHY_IOV || WFD_PHY_LL_DEBUG */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(MACOSX)
	{"phy_deaf", IOV_PHY_DEAF,
	(IOVF_GET_UP | IOVF_SET_UP), 0, IOVT_UINT8, 0
	},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(MACOSX) */
	{"phynoise_srom", IOV_PHYNOISE_SROM,
	IOVF_GET_UP, 0, IOVT_UINT32, 0
	},
	{"num_stream", IOV_NUM_STREAM,
	(0), 0, IOVT_INT32, 0
	},
	{"min_txpower", IOV_MIN_TXPOWER,
	0, 0, IOVT_UINT32, 0
	},
#if defined(BCMINTERNAL) || defined(MACOSX)
	{"phywreg_limit", IOV_PHYWREG_LIMIT,
	0, 0, IOVT_UINT32, IOVT_UINT32
	},
#endif /* BCMINTERNAL */
	{"phy_muted", IOV_PHY_MUTED,
	0, 0, IOVT_UINT8, 0
	},
#if defined(WLMEDIA_N2DEV) || defined(WLMEDIA_N2DBG)
	{"ntd_gds_lowtxpwr", IOV_NTD_GDS_LOWTXPWR,
	IOVF_GET_UP, 0, IOVT_UINT8, 0
	},
	{"papdcal_indexdelta", IOV_PAPDCAL_INDEXDELTA,
	IOVF_GET_UP, 0, IOVT_UINT8, 0
	},
#endif /* defined(WLMEDIA_N2DEV) || defined(WLMEDIA_N2DBG) */
	{"phy_rxantsel", IOV_PHY_RXANTSEL,
	(0), 0, IOVT_UINT8, 0
	},
	{"phy_dssf", IOV_PHY_DSSF,
	IOVF_SET_UP, 0, IOVT_UINT32, 0
	},
#if defined(WLTEST) || defined(BCMDBG)
	{"phy_txpwr_core", IOV_PHY_TXPWR_CORE,
	0, 0, IOVT_UINT32, 0
	},
#endif /* WLTEST || BCMDBG */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || defined(BCMDBG)
	{"phy_papd_eps_offset", IOV_PAPD_EPS_OFFSET,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_INT16, 0
	},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || defined(BCMDBG) */
	{"phydump_page", IOV_PHY_DUMP_PAGE,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"phydump_entry", IOV_PHY_DUMP_ENTRY,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_INT16, 0
	},
	/* ************************************************************************************ */
	/* [PHY_REARCH] Do not add any variables here. Add them to the individual modules */
	/* ************************************************************************************ */
#if defined(BCMDBG) || defined(WLTEST) || defined(MACOSX) || defined(ATE_BUILD) || \
	defined(BCMDBG_TEMPSENSE)
	{"phy_tempsense", IOV_PHY_TEMPSENSE,
	IOVF_GET_UP, 0, IOVT_INT16, 0
	},
#endif /* BCMDBG || WLTEST || MACOSX || ATE_BUILD || BCMDBG_TEMPSENSE */
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"phy_cal_disable", IOV_PHY_CAL_DISABLE,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif  /* BCMINTERNAL || WLTEST */
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"phy_vbatsense", IOV_PHY_VBATSENSE,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
	{"phy_idletssi", IOV_PHY_IDLETSSI_REG,
	(IOVF_GET_UP | IOVF_SET_UP), 0, IOVT_BUFFER, 0
	},
	{"phy_tssi", IOV_PHY_AVGTSSI_REG,
	(IOVF_GET_UP), 0, IOVT_BUFFER, 0
	},
	{"phy_resetcca", IOV_PHY_RESETCCA,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phy_iqlocalidx", IOV_PHY_IQLOCALIDX,
	(IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0
	},
	{"phycal_tempdelta", IOV_PHYCAL_TEMPDELTA,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
	{"phy_sromtempsense", IOV_PHY_SROM_TEMPSENSE,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_INT16, 0
	},
	{"gain_cal_temp", IOV_PHY_GAIN_CAL_TEMP,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_INT16, 0
	},
#ifdef PHYMON
	{"phycal_state", IOV_PHYCAL_STATE,
	IOVF_GET_UP, 0, IOVT_UINT32, 0,
	},
#endif /* PHYMON */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(AP) || defined(BCMDBG_PHYCAL)
	{"phy_percal", IOV_PHY_PERICAL,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(AP) */
	{"phy_percal_delay", IOV_PHY_PERICAL_DELAY,
	(0), 0, IOVT_UINT16, 0
	},
#ifdef WLC_TXCAL
	{"phy_adj_tssi", IOV_PHY_ADJUSTED_TSSI,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0
	},
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"txcal_gainsweep", IOV_PHY_TXCAL_GAINSWEEP,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, sizeof(wl_txcal_params_t)
	},
	{"txcal_ver", IOV_PHY_TXCALVER,
	(IOVF_MFG), 0, IOVT_INT32,  0
	},
	{"txcal_gainsweep_meas", IOV_PHY_TXCAL_GAINSWEEP_MEAS,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER,
	OFFSETOF(wl_txcal_meas_ncore_t, txcal_percore) +
	PHY_CORE_MAX * sizeof(wl_txcal_meas_percore_t)
	},
	{"txcal_pwr_tssi_tbl", IOV_PHY_TXCAL_PWR_TSSI_TBL,
	(0), 0, IOVT_BUFFER, OFFSETOF(wl_txcal_power_tssi_ncore_t, tssi_percore) +
	PHY_CORE_MAX * sizeof(wl_txcal_power_tssi_percore_t)
	},
	{"txcal_apply_pwr_tssi_tbl", IOV_PHY_TXCAL_APPLY_PWR_TSSI_TBL,
	(0), 0, IOVT_UINT32, 0
	},
	{"phy_read_estpwrlut", IOV_PHY_READ_ESTPWR_LUT,
	(IOVF_GET_UP | IOVF_MFG), 0, IOVT_BUFFER, MAX_NUM_TXCAL_MEAS * sizeof(uint16)
	},
	{"olpc_anchoridx", IOV_OLPC_ANCHOR_IDX,
	(IOVF_MFG), 0, IOVT_BUFFER, sizeof(wl_olpc_pwr_t)
	},
	{"olpc_idx_valid", IOV_OLPC_IDX_VALID,
	(IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"txcal_status", IOV_PHY_TXCAL_STATUS,
	(IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif	/* BCMINTERNAL || WLTEST */
#endif /* WLC_TXCAL */
#if defined(WLC_TXCAL) || (defined(WLOLPC) && !defined(WLOLPC_DISABLED))
	{"olpc_anchor2g", IOV_OLPC_ANCHOR_2G,
	0, 0, IOVT_INT8, 0
	},
	{"olpc_anchor5g", IOV_OLPC_ANCHOR_5G,
	0, 0, IOVT_INT8, 0
	},
	{"olpc_thresh", IOV_OLPC_THRESH,
	0, 0, IOVT_INT8, 0
	},
	{"disable_olpc", IOV_DISABLE_OLPC,
	0, 0, IOVT_INT8, 0
	},
#endif	/* WLC_TXCAL || (WLOLPC && !WLOLPC_DISABLED) */
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"aci_exit_check_period", IOV_ACI_EXIT_CHECK_PERIOD,
	(IOVF_MFG), 0, IOVT_UINT32, 0
	},
#endif /* BCMINTERNAL || WLTEST */
#if defined(WLTEST)
	{"phy_glitchthrsh", IOV_PHY_GLITCHK,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phy_noise_up", IOV_PHY_NOISE_UP,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phy_noise_dwn", IOV_PHY_NOISE_DWN,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif /* #if defined(WLTEST) */
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"rpcalvars", IOV_RPCALVARS,
	(IOVF_SET_DOWN | IOVF_MFG), 0, IOVT_BUFFER, 2*WL_NUM_RPCALVARS * sizeof(wl_rpcal_t)
	},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if (defined(BCMINTERNAL) || defined(WLTEST))
	{"phy_tpc_av", IOV_TPC_AV,
	(IOVF_SET_UP|IOVF_GET_UP), 0, IOVT_BUFFER, 0
	},
	{"phy_tpc_vmid", IOV_TPC_VMID,
	(IOVF_SET_UP|IOVF_GET_UP), 0, IOVT_BUFFER, 0
	},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if defined(BCMDBG)
	{"phy_force_fdiqi", IOV_PHY_FORCE_FDIQI,
	IOVF_SET_UP, 0, IOVT_UINT32, 0
	},
#endif /* BCMDBG */
	{"edcrs", IOV_EDCRS,
	(IOVF_SET_UP|IOVF_GET_UP), 0, IOVT_UINT8, 0
	},
	{"phy_afeoverride", IOV_PHY_AFE_OVERRIDE,
	(0), 0, IOVT_UINT8, 0
	},
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"unmod_rssi", IOV_UNMOD_RSSI,
	(IOVF_MFG), 0, IOVT_INT32, 0
	},
#endif /* BCMINTERNAL || WLTEST */
#if (NCONF || LCN40CONF || LCN20CONF || ACCONF || ACCONF2) && defined(WLTEST)
	{"phy_rssi_gain_cal_temp", IOV_PHY_RSSI_GAIN_CAL_TEMP,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_INT32, 0
	},
#endif /* (NCONF || LCN40CONF || ACCONF || ACCONF2) && WLTEST */
	{"phy_oclscdenable", IOV_PHY_OCLSCDENABLE,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"lnldo2", IOV_LNLDO2,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV)
	{"phy_dynamic_ml", IOV_PHY_DYN_ML,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"aci_nams", IOV_PHY_ACI_NAMS,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) */
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"phy_auxpga", IOV_PHY_AUXPGA,
	(IOVF_SET_DOWN | IOVF_MFG), 0, IOVT_BUFFER, 6*sizeof(uint8)
	},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if defined(WLTEST)
#if defined(LCNCONF) || defined(LCN40CONF)
	{"lcnphy_rxiqgain", IOV_LCNPHY_RXIQGAIN,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
	{"lcnphy_rxiqgspower", IOV_LCNPHY_RXIQGSPOWER,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
	{"lcnphy_rxiqpower", IOV_LCNPHY_RXIQPOWER,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
	{"lcnphy_rxiqstatus", IOV_LCNPHY_RXIQSTATUS,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
	{"lcnphy_rxiqsteps", IOV_LCNPHY_RXIQSTEPS,
	IOVF_SET_UP, 0, IOVT_UINT8, 0
	},
	{"lcnphy_tssimaxpwr", IOV_LCNPHY_TSSI_MAXPWR,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
	{"lcnphy_tssiminpwr", IOV_LCNPHY_TSSI_MINPWR,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
#endif /* #if defined(LCNCONF) || defined(LCN40CONF) */
#if LCN40CONF || (defined(LCN20CONF) && (LCN20CONF != 0))
	{"lcnphy_txclampdis", IOV_LCNPHY_TXPWRCLAMP_DIS,
	IOVF_GET_UP, 0, IOVT_UINT8, 0
	},
	{"lcnphy_txclampofdm", IOV_LCNPHY_TXPWRCLAMP_OFDM,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
#endif /* LCN40CONF||(defined(LCN20CONF) && (LCN20CONF != 0)) */
#endif /* #if defined(WLTEST) */
#if defined(BCMDBG) || defined(BCMINTERNAL)
	{"lcnphy_txclampcck", IOV_LCNPHY_TXPWRCLAMP_CCK,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
#endif
	{"phy_crs_war", IOV_PHY_CRS_WAR,
	(0), 0, IOVT_INT8, 0
	},
#if defined(BCMDBG) || defined(BCMINTERNAL) || defined(WLTEST) || defined(BCMCCX)
	{"txinstpwr", IOV_TXINSTPWR,
	(IOVF_GET_CLK | IOVF_GET_BAND | IOVF_MFG), 0, IOVT_BUFFER, sizeof(tx_inst_power_t)
	},
#endif /* BCMDBG || BCMINTERNAL || WLTEST || BCMCCX */

#if defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD)
	{"phy_txpwrctrl", IOV_PHY_TXPWRCTRL,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"phy_txpwrindex", IOV_PHY_TXPWRINDEX,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_BUFFER, 0
	},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD) */
#if defined(BCMINTERNAL) || defined(WLTEST)
	{"patrim", IOV_PATRIM,
	(IOVF_MFG), 0, IOVT_INT32, 0
	},
	{"pavars2", IOV_PAVARS2,
	(IOVF_SET_DOWN | IOVF_MFG), 0, IOVT_BUFFER, sizeof(wl_pavars2_t)
	},
	{"povars", IOV_POVARS,
	(IOVF_SET_DOWN | IOVF_MFG), 0, IOVT_BUFFER, sizeof(wl_po_t)
	},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
	{"sromrev", IOV_SROM_REV,
	(IOVF_SET_DOWN), 0, IOVT_UINT8, 0
	},
#ifdef WLTEST
	{"maxpower", IOV_PHY_MAXP,
	(IOVF_SET_DOWN | IOVF_MFG), 0, IOVT_BUFFER, 0
	},
#endif /* WLTEST */
#ifdef SAMPLE_COLLECT
	{"sample_collect", IOV_PHY_SAMPLE_COLLECT,
	(IOVF_GET_CLK), 0, IOVT_BUFFER, WLC_IOCTL_SMLEN
	},
	{"sample_data", IOV_PHY_SAMPLE_DATA,
	(IOVF_GET_CLK), 0, IOVT_BUFFER, WLC_IOCTL_MEDLEN
	},
	{"sample_collect_gainadj", IOV_PHY_SAMPLE_COLLECT_GAIN_ADJUST,
	0, 0, IOVT_INT8, 0
	},
	{"mac_triggered_sample_collect", IOV_PHY_MAC_TRIGGERED_SAMPLE_COLLECT,
	0, 0, IOVT_BUFFER, WLC_IOCTL_SMLEN
	},
	{"mac_triggered_sample_data", IOV_PHY_MAC_TRIGGERED_SAMPLE_DATA,
	0, 0, IOVT_BUFFER, WLC_IOCTL_MEDLEN
	},
	{"sample_collect_gainidx", IOV_PHY_SAMPLE_COLLECT_GAIN_INDEX,
	0, 0, IOVT_UINT8, 0
	},
	{"iq_metric_data", IOV_IQ_IMBALANCE_METRIC_DATA,
	(IOVF_GET_DOWN | IOVF_GET_CLK), 0, IOVT_BUFFER, WLC_SAMPLECOLLECT_MAXLEN
	},
	{"iq_metric", IOV_IQ_IMBALANCE_METRIC,
	(IOVF_GET_DOWN | IOVF_GET_CLK), 0, IOVT_BUFFER, 0
	},
	{"iq_metric_pass", IOV_IQ_IMBALANCE_METRIC_PASS,
	(IOVF_GET_DOWN | IOVF_GET_CLK), 0, IOVT_BUFFER, 0
	},
#endif /* sample_collect */
#ifdef WLOLPC
	{"olpc_offset", IOV_OLPC_OFFSET,
	0, 0, IOVT_BUFFER, 0
	},
#endif /* WLOLPC */
#if defined(BCMDBG) || defined(WLTEST)
	{"phy_master_override", IOV_PHY_MASTER_OVERRIDE,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_INT8, 0
	},
#endif /* BCMDBG  || WLTEST */
	{NULL, 0, 0, 0, 0, 0 }
	/* ************************************************************************************ */
	/* [PHY_REARCH] Do not add any variables here. Add them to the individual modules */
	/* ************************************************************************************ */
};

#include <typedefs.h>
/* *********************************************** */
#include <phy_dbg.h>
#include <phy_misc.h>
/* *********************************************** */
#include <bcmdefs.h>
#include <wlc_phy_hal.h>
#include <wlc_phy_int.h>
#include <wlc_phyreg_n.h>
#include <wlc_phyreg_ht.h>
#include <wlc_phyreg_ac.h>
#include <wlc_phyreg_lcn.h>
#include <wlc_phyreg_lcn40.h>
#include <wlc_phytbl_n.h>
#include <wlc_phytbl_ht.h>
#include <wlc_phytbl_ac.h>
#include <wlc_phytbl_20691.h>
#include <wlc_phytbl_20693.h>
#include <wlc_phytbl_20694.h>
#include <wlc_phy_radio.h>
#include <wlc_phy_lcn40.h>
#include <wlc_phy_n.h>
#include <wlc_phy_ht.h>
#include <wlc_phy_lcn20.h>
#include <wlc_phyreg_lcn20.h>
#include <bcmwifi_channels.h>
#include <bcmotp.h>
#ifdef WLSRVSDB
#include <saverestore.h>
#endif
#include <phy_utils_math.h>
#include <phy_utils_var.h>
#include <phy_utils_status.h>
#include <phy_utils_reg.h>
#include <phy_utils_channel.h>
#include <phy_utils_api.h>
#include <phy_misc_api.h>
#include <phy_btcx.h>
#include <phy_calmgr.h>
#include <phy_tpc.h>
#include <phy_ac_calmgr.h>
#include <phy_noise.h>

#if defined(BCMINTERNAL) || defined(WLTEST)
#define BFECONFIGREF_FORCEVAL    0x9
#define BFMCON_FORCEVAL          0x8c03
#define BFMCON_RELEASEVAL        0x8c1d
#define REFRESH_THR_FORCEVAL     0xffff
#define REFRESH_THR_RELEASEVAL   0x186a
#define BFRIDX_POS_FORCEVAL      0x100
#define BFRIDX_POS_RELEASEVAL    0x0
#endif /* BCMINTERNAL || WLTEST */

#define LCN40_RX_GAIN_INDEX_MASK	0x7F00
#define LCN40_RX_GAIN_INDEX_SHIFT	8

/* Modularise and clean up attach functions */
static void wlc_phy_cal_perical_mphase_schedule(phy_info_t *pi, uint delay);
static int wlc_phy_iovar_dispatch_old(phy_info_t *pi, uint32 actionid, void *p, void *a, int vsize,
	int32 int_val, bool bool_val);
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(AP) || defined(BCMDBG_PHYCAL)
static int wlc_phy_iovar_perical_config(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr,
		bool set);
#endif
static int wlc_phy_iovar_set_dssf(phy_info_t *pi, int32 set_val);
static int wlc_phy_iovar_get_dssf(phy_info_t *pi, int32 *ret_val);
#if defined(BCMDBG) || defined(WLTEST) || defined(MACOSX) || defined(ATE_BUILD)
static int
wlc_phy_iovar_tempsense_paldosense(phy_info_t *pi, int32 *ret_int_ptr, uint8 tempsense_paldosense);
#endif
#if defined(BCMINTERNAL) || defined(WLTEST)
static int wlc_phy_iovar_idletssi(phy_info_t *pi, int32 *ret_int_ptr, bool type);
static int
wlc_phy_iovar_bbmult_get(phy_info_t *pi, int32 int_val, bool bool_val, int32 *ret_int_ptr);
static int wlc_phy_iovar_bbmult_set(phy_info_t *pi, void *p);
static int wlc_phy_iovar_vbatsense(phy_info_t *pi, int32 *ret_int_ptr);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if (defined(BCMINTERNAL) || defined(WLTEST))
static int
wlc_phy_iovar_avvmid_get(phy_info_t *pi, void *p,
	bool bool_val, int32 *ret_int_ptr, wlc_avvmid_t avvmid_type);
static int wlc_phy_iovar_avvmid_set(phy_info_t *pi, void *p,  wlc_avvmid_t avvmid_type);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if defined(BCMINTERNAL) || defined(WLTEST)
static int wlc_phy_iovar_idletssi_reg(phy_info_t *pi, int32 *ret_int_ptr, int32 int_val, bool set);
static int wlc_phy_iovar_avgtssi_reg(phy_info_t *pi, int32 *ret_int_ptr);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if defined(BCMINTERNAL) || defined(WLTEST)
static int wlc_phy_iovar_txrx_chain(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, bool set);
static void wlc_phy_iovar_bphy_testpattern(phy_info_t *pi, uint8 testpattern, bool enable);
static void wlc_phy_iovar_scraminit(phy_info_t *pi, int8 scraminit);
static int wlc_phy_iovar_force_rfseq(phy_info_t *pi, uint8 int_val);
static void wlc_phy_iovar_tx_tone_hz(phy_info_t *pi, int32 int_val);
static void wlc_phy_iovar_tx_tone_stop(phy_info_t *pi);
static int16 wlc_phy_iovar_test_tssi(phy_info_t *pi, uint8 val, uint8 pwroffset);
static int16 wlc_phy_iovar_test_idletssi(phy_info_t *pi, uint8 val);
static int16 wlc_phy_iovar_setrptbl(phy_info_t *pi);
static int16 wlc_phy_iovar_forceimpbf(phy_info_t *pi);
static int16 wlc_phy_iovar_forcesteer(phy_info_t *pi, uint8 enable);
static void
wlc_phy_iovar_rxcore_enable(phy_info_t *pi, int32 int_val, bool bool_val, int32 *ret_int_ptr,
	bool set);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(MACOSX)
static void wlc_phy_iovar_set_deaf(phy_info_t *pi, int32 int_val);
static int wlc_phy_iovar_get_deaf(phy_info_t *pi, int32 *ret_int_ptr);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(MACOSX) */
static int
wlc_phy_iovar_forcecal(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, int vsize, bool set);
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG) || defined(ATE_BUILD)
#ifndef ATE_BUILD
static int
wlc_phy_iovar_forcecal_obt(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, int vsize, bool set);
#endif /* !ATE_BUILD */
#endif /* BCMINTERNAL || WLTEST || WLMEDIA_N2DBG || WLMEDIA_N2DEV || ATE_BUILD */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD)
static int
wlc_phy_iovar_txpwrctrl(phy_info_t *pi, int32 int_val, bool bool_val, int32 *ret_int_ptr,
	bool set);
static int
wlc_phy_iovar_txpwrindex_get(phy_info_t *pi, int32 int_val, bool bool_val, int32 *ret_int_ptr);
static int wlc_phy_iovar_txpwrindex_set(phy_info_t *pi, void *p);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD) */
#if defined(BCMINTERNAL) || defined(WLTEST)
static int wlc_phy_pkteng_get_gainindex(phy_info_t *pi, int32 *gain_idx);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

#ifdef WLC_TXCAL
static int wlc_phy_iovar_adjusted_tssi(phy_info_t *pi, int32 *ret_int_ptr, uint8 int_val);
#if defined(BCMDBG) || defined(BCMINTERNAL) || defined(WLTEST) || defined(BCMCCX)
static void wlc_phy_txpower_get_instant(phy_info_t *pi, void *pwr);
#endif /* BCMDBG || BCMINTERNAL || WLTEST || BCMCCX */
#if defined(BCMINTERNAL) || defined(WLTEST)
static int wlc_phy_txcal_get_pwr_tssi_tbl(phy_info_t *pi, uint8 channel);
static int wlc_phy_txcal_store_pwr_tssi_tbl(phy_info_t *pi);
static int wlc_phy_txcal_gainsweep(phy_info_t *pi, wl_txcal_params_t *txcal_params);
static int wlc_phy_txcal_generate_pwr_tssi_tbl(phy_info_t *pi);
#endif /* BCMINTERNAL || WLTEST */
#endif /* WLC_TXCAL */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV)
static int
wlc_phy_aci_nams(phy_info_t *pi, int32 int_val,	int32 *ret_int_ptr, int vsize, bool set);
static int
wlc_phy_dynamic_ml(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, int vsize, bool set);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) */
int phy_iovars_wrapper_sample_collect(phy_info_t *pi, uint32 actionid, uint16 type,
		void *p, uint plen, void *a, int alen, int vsize);

#include <wlc_patch.h>

#if defined(BCMDBG) || defined(BCMINTERNAL) || defined(WLTEST) || defined(BCMCCX)
/* Return the current instantaneous est. power
 * For swpwr ctrl it's based on current TSSI value (as opposed to average)
 * Mainly used by mfg.
 */
static void
wlc_phy_txpower_get_instant(phy_info_t *pi, void *pwr)
{
	tx_inst_power_t *power = (tx_inst_power_t *)pwr;
	/* If sw power control, grab the instant value based on current TSSI Only
	 * If hw based, read the hw based estimate in realtime
	 */
	if (ISLCN40PHY(pi)) {
		if (!pi->hwpwrctrl)
			return;

		wlc_lcn40phy_get_tssi(pi, (int8*)&power->txpwr_est_Pout_gofdm,
			(int8*)&power->txpwr_est_Pout[0]);
		power->txpwr_est_Pout[1] = power->txpwr_est_Pout_gofdm;

	}

}
#endif /* BCMDBG || BCMINTERNAL || WLTEST || BCMCCX */

#if defined(BCMINTERNAL) || defined(WLTEST)
static int
wlc_phy_pkteng_get_gainindex(phy_info_t *pi, int32 *gain_idx)
{
	int i;

	if (!pi->sh->up) {
		return BCME_NOTUP;
	}

	PHY_INFORM(("wlc_phy_pkteng_get_gainindex Called\n"));

	if (ISLCNCOMMONPHY(pi)) {
		uint8 gidx[4];
		uint16 rssi_addr[4];

		uint16 lcnphyregs_shm_addr =
			2 * wlapi_bmac_read_shm(pi->sh->physhim, M_LCN40PHYREGS_PTR(pi));

		rssi_addr[0] = lcnphyregs_shm_addr +  M_SSLPN_RSSI_0_OFFSET(pi);
		rssi_addr[1] = lcnphyregs_shm_addr + M_SSLPN_RSSI_1_OFFSET(pi);
		rssi_addr[2] = lcnphyregs_shm_addr + M_SSLPN_RSSI_2_OFFSET(pi);
		rssi_addr[3] = lcnphyregs_shm_addr + M_SSLPN_RSSI_3_OFFSET(pi);

		for (i = 0; i < 4; i++) {
			gidx[i] =
				(wlapi_bmac_read_shm(pi->sh->physhim, rssi_addr[i])
				& LCN40_RX_GAIN_INDEX_MASK) >> LCN40_RX_GAIN_INDEX_SHIFT;

		}
		*gain_idx = (int32)gidx[0];
	}

	return BCME_OK;
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV)
static int
wlc_phy_dynamic_ml(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, int vsize, bool set)
{
	int err = BCME_OK;

	if (!pi->sh->clk)
		return BCME_NOCLK;

	if (!ISNPHY(pi))
		return BCME_UNSUPPORTED;
	else if (NREV_LT(pi->pubpi->phy_rev, LCNXN_BASEREV + 2))
		return BCME_UNSUPPORTED;

	if (!set) {
		if (ISNPHY(pi)) {
			wlc_phy_dynamic_ml_get(pi);
			*ret_int_ptr = pi->nphy_ml_type;
		}

	} else {
		if ((int_val > 4) || (int_val < 0))
			return BCME_RANGE;
		wlc_phy_dynamic_ml_set(pi, int_val);
	}
	return err;
}

/* aci_nams : ACI Non Assoc Mode Sanity */
static int
wlc_phy_aci_nams(phy_info_t *pi, int32 int_val,	int32 *ret_int_ptr, int vsize, bool set)
{
	int err = BCME_OK;

	if (!pi->sh->clk)
		return BCME_NOCLK;

	if (!ISNPHY(pi))
		return BCME_UNSUPPORTED;
	else if NREV_LT(pi->pubpi->phy_rev, LCNXN_BASEREV + 2)
		return BCME_UNSUPPORTED;

	if (!set) {
		if (ISNPHY(pi)) {
			*ret_int_ptr = pi->aci_nams;
		}

	} else {
		if ((int_val > 1) || (int_val < 0))
			return BCME_RANGE;
		pi->aci_nams = (uint8)int_val;
	}
	return err;
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) */

/* wrapper function for multi phase perical schedule */
void
wlc_phy_mphase_cal_schedule(wlc_phy_t *pih, uint delay_val)
{
	phy_info_t *pi = (phy_info_t*)pih;

	if (!ISNPHY(pi) && !ISHTPHY(pi) && !ISACPHY(pi))
		return;

	if (PHY_PERICAL_MPHASE_PENDING(pi)) {
		wlc_phy_cal_perical_mphase_reset(pi);
	}

	pi->cal_info->cal_searchmode = PHY_CAL_SEARCHMODE_RESTART;
	/* schedule mphase cal */
	wlc_phy_cal_perical_mphase_schedule(pi, delay_val);
}

static void
wlc_phy_cal_perical_mphase_schedule(phy_info_t *pi, uint delay_val)
{
	/* for manual mode, let it run */
	if ((pi->phy_cal_mode != PHY_PERICAL_MPHASE) &&
	    (pi->phy_cal_mode != PHY_PERICAL_MANUAL))
		return;

	PHY_CAL(("wlc_phy_cal_perical_mphase_schedule\n"));

	/* use timer to wait for clean context since this
	 * may be called in the middle of nphy_init
	 */
	wlapi_del_timer(pi->sh->physhim, pi->phycal_timer);

	pi->cal_info->cal_phase_id = MPHASE_CAL_STATE_INIT;
	wlapi_add_timer(pi->sh->physhim, pi->phycal_timer, delay_val, 0);
}

/* policy entry */
void
wlc_phy_cal_perical(wlc_phy_t *pih, uint8 reason)
{
	int16 current_temp = 0, delta_temp = 0;
	uint delta_time = 0;
	uint8 do_cals = 0;
	bool  suppress_cal = FALSE;

	phy_info_t *pi = (phy_info_t*)pih;

#if defined(PHYCAL_CACHING)
	ch_calcache_t *ctx = NULL;
#endif

	if (!ISNPHY(pi) && !ISHTPHY(pi) && !ISACPHY(pi))
		return;

	/* reset to default */
	pi->cal_info->cal_suppress_count = 0;

	/* do only init noisecal or trigger noisecal when STA
	 * joins to an AP (also trigger noisecal if AP roams)
	 */
	pi->trigger_noisecal = TRUE;

	/* reset periodic noise poll flag to avoid
	 * race-around condition with cal triggers
	 * between watchdog and others which could
	 * potentially cause sw corruption.
	 */
	pi->capture_periodic_noisestats = FALSE;

	if ((pi->phy_cal_mode == PHY_PERICAL_DISABLE) ||
	    (pi->phy_cal_mode == PHY_PERICAL_MANUAL))
		return;

	/* NPHY_IPA : disable PAPD cal for following calibration at least 4322A1? */

	PHY_CAL(("wlc_phy_cal_perical: reason %d chanspec 0x%x\n", reason,
	         pi->radio_chanspec));

	/* Update the Tx power per channel on ACPHY for 2GHz channels */
#ifdef POWPERCHANNL
	if (PWRPERCHAN_ENAB(pi)) {
		if (ISACPHY(pi) && !(ACMAJORREV_4(pi->pubpi->phy_rev)))
			wlc_phy_tx_target_pwr_per_channel_decide_run_acphy(pi);
	}
#endif /* POWPERCHANNL */

#if defined(PHYCAL_CACHING)
	ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);
#endif

	/* perical is enabled : Either single phase only, or mphase is allowed
	 * Dispatch to s-phase or m-phase based on reasons
	 */
	switch (reason) {

	case PHY_PERICAL_DRIVERUP:	/* always single phase ? */
		break;

	case PHY_PERICAL_PHYINIT:	/* always multi phase */
		if (pi->phy_cal_mode == PHY_PERICAL_MPHASE) {
#if defined(PHYCAL_CACHING)
			if (ctx)
			{
				/* Switched context so restart a pending MPHASE cal or
				 * restore stored calibration
				 */
				ASSERT(ctx->chanspec == pi->radio_chanspec);

				/* If it was pending last time, just restart it */
				if (PHY_PERICAL_MPHASE_PENDING(pi)) {
					/* Delete any existing timer just in case */
					PHY_CAL(("%s: Restarting calibration for 0x%x phase %d\n",
						__FUNCTION__, ctx->chanspec,
						pi->cal_info->cal_phase_id));
					wlapi_del_timer(pi->sh->physhim, pi->phycal_timer);
					wlapi_add_timer(pi->sh->physhim, pi->phycal_timer, 0, 0);
				} else if (wlc_phy_cal_cache_restore(pi) != BCME_ERROR) {
					break;
				}
			}
			else
#endif /* PHYCAL_CACHING */
			{
				if (PHY_PERICAL_MPHASE_PENDING(pi))
					wlc_phy_cal_perical_mphase_reset(pi);

				pi->cal_info->cal_searchmode = PHY_CAL_SEARCHMODE_RESTART;

				/* schedule mphase cal */
				wlc_phy_cal_perical_mphase_schedule(pi, PHY_PERICAL_INIT_DELAY);
			}
		}
		break;

	case PHY_PERICAL_JOIN_BSS:
	case PHY_PERICAL_START_IBSS:
	case PHY_PERICAL_UP_BSS:
	case PHY_PERICAL_PHYMODE_SWITCH:

		/* These must run in single phase to ensure clean Tx/Rx
		 * performance so the auto-rate fast-start is promising
		 */

		if ((pi->phy_cal_mode == PHY_PERICAL_MPHASE) && PHY_PERICAL_MPHASE_PENDING(pi))
			wlc_phy_cal_perical_mphase_reset(pi);

		/* Always do idle TSSI measurement at the end of NPHY cal
		 * while starting/joining a BSS/IBSS
		 */
		pi->first_cal_after_assoc = TRUE;

		if (ISNPHY(pi))
			pi->u.pi_nphy->cal_type_override = PHY_PERICAL_FULL; /* not used in htphy */


		/* Update last cal temp to current tempsense reading */
		if (pi->phycal_tempdelta) {
			if (ISNPHY(pi))
				pi->cal_info->last_cal_temp = wlc_phy_tempsense_nphy(pi);
			else if (ISHTPHY(pi))
				pi->cal_info->last_cal_temp = wlc_phy_tempsense_htphy(pi);
			else if (ISACPHY(pi))
				pi->cal_info->last_cal_temp =
					wlc_phy_tempsense_acphy(pi);
		}

		/* Attempt cal cache restore if ctx is valid */
#if defined(PHYCAL_CACHING)
		if (ctx)
		{
			PHY_CAL(("wl%d: %s: Attempting to restore cals on JOIN...\n",
				pi->sh->unit, __FUNCTION__));

			if (wlc_phy_cal_cache_restore(pi) == BCME_ERROR) {
				phy_calmgr_cals(pi, PHY_PERICAL_FULL,
					PHY_CAL_SEARCHMODE_RESTART);
			}
		}
		else
#endif /* PHYCAL_CACHING */
		{
			phy_calmgr_cals(pi, PHY_PERICAL_FULL, PHY_CAL_SEARCHMODE_RESTART);
		}
		break;

	case PHY_PERICAL_WATCHDOG:

		if (PUB_NOT_ASSOC(pi) && ISACPHY(pi))
			return;

		if ((ACMAJORREV_4(pi->pubpi->phy_rev)) && !(ROUTER_4349(pi))) {
			if ((pi->ldo3p3_voltage == -1) && (pi->paldo3p3_voltage == -1)) {
				do_cals = wlc_phy_vbat_monitoring_algorithm_decision(pi);
			}
		}
		/* Disable periodic noisecal trigger */
		pi->trigger_noisecal = FALSE;

		if (ISNPHY(pi) && NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))
			pi->capture_periodic_noisestats = TRUE;
		else
			pi->capture_periodic_noisestats = FALSE;

		PHY_CAL(("%s: %sPHY phycal_tempdelta=%d\n", __FUNCTION__,
			(ISNPHY(pi)) ? "N": (ISHTPHY(pi) ? "HT" : (ISACPHY(pi) ? "AC" : "some")),
			pi->phycal_tempdelta));

		if (pi->phycal_tempdelta && (ISNPHY(pi) || ISHTPHY(pi) || ISACPHY(pi))) {

			int cal_chanspec = 0;

			if (ISNPHY(pi)) {
				current_temp = wlc_phy_tempsense_nphy(pi);
				cal_chanspec = pi->cal_info->u.ncal.txiqlocal_chanspec;
			} else if (ISHTPHY(pi)) {
				current_temp = wlc_phy_tempsense_htphy(pi);
				cal_chanspec = pi->cal_info->u.htcal.chanspec;
			} else if (ISACPHY(pi)) {
				if (pi->sh->now - pi->cal_info->last_temp_cal_time >=
					pi->sh->glacial_timer) {
					pi->cal_info->last_temp_cal_time = pi->sh->now;
					current_temp = wlc_phy_tempsense_acphy(pi);
				} else {
					current_temp =  pi->cal_info->last_cal_temp;
				}
				cal_chanspec = pi->cal_info->u.accal.chanspec;
			}

			delta_temp = ((current_temp > pi->cal_info->last_cal_temp) ?
				(current_temp - pi->cal_info->last_cal_temp) :
				(pi->cal_info->last_cal_temp - current_temp));

			/* Only do WATCHDOG triggered (periodic) calibration if
			 * the channel hasn't changed and if the temperature delta
			 * is above the specified threshold
			 */
			PHY_CAL(("%sPHY temp is %d, delta %d, cal_delta %d, chanspec %04x/%04x\n",
				(ISNPHY(pi)) ? "N": (ISHTPHY(pi) ? "HT" :
				(ISACPHY(pi) ? "AC" : "some")),
				current_temp, delta_temp, pi->phycal_tempdelta,
				cal_chanspec, pi->radio_chanspec));

			delta_time = pi->sh->now - pi->cal_info->last_cal_time;

			/* cal_period = 0 implies only temperature based cals */
			if (((delta_temp < pi->phycal_tempdelta) &&
				(((delta_time < pi->cal_period) &&
				(pi->cal_period > 0)) || (pi->cal_period == 0)) &&
				(cal_chanspec == pi->radio_chanspec)) && (!do_cals)) {

				suppress_cal = TRUE;
				pi->cal_info->cal_suppress_count = pi->sh->glacial_timer;
				PHY_CAL(("Suppressing calibration.\n"));

			} else {
				pi->cal_info->last_cal_temp = current_temp;
			}
		}

		if (!suppress_cal) {
			/* if mphase is allowed, do it, otherwise, fall back to single phase */
			if (pi->phy_cal_mode == PHY_PERICAL_MPHASE) {
				/* only schedule if it's not in progress */
				if ((!PHY_PERICAL_MPHASE_PENDING(pi)) && (!do_cals)) {
					pi->cal_info->cal_searchmode = PHY_CAL_SEARCHMODE_REFINE;
					wlc_phy_cal_perical_mphase_schedule(pi,
						PHY_PERICAL_WDOG_DELAY);
				} else if (do_cals) {
					if (PHY_PERICAL_MPHASE_PENDING(pi)) {
						wlc_phy_cal_perical_mphase_reset(pi);
					}
					wlc_phy_cals_acphy(pi->u.pi_acphy->calmgri,
						PHY_PERICAL_UNDEF, PHY_CAL_SEARCHMODE_RESTART);
				}
			} else if (pi->phy_cal_mode == PHY_PERICAL_SPHASE) {
				phy_calmgr_cals(pi, PHY_PERICAL_AUTO, PHY_CAL_SEARCHMODE_RESTART);
			} else {
				ASSERT(0);
			}
		}
		break;

	case PHY_PERICAL_DCS:

		/* Only applicable for NPHYs */
		ASSERT(ISNPHY(pi));

		if (ISNPHY(pi)) {
			if (PHY_PERICAL_MPHASE_PENDING(pi)) {
				wlc_phy_cal_perical_mphase_reset(pi);

				if (pi->phycal_tempdelta) {
					current_temp = wlc_phy_tempsense_nphy(pi);
					pi->cal_info->last_cal_temp = current_temp;
				}
			} else if (pi->phycal_tempdelta) {

				current_temp = wlc_phy_tempsense_nphy(pi);

				delta_temp = ((current_temp > pi->cal_info->last_cal_temp) ?
					(current_temp - pi->cal_info->last_cal_temp) :
					(pi->cal_info->last_cal_temp - current_temp));

				if ((delta_temp < (int16)pi->phycal_tempdelta)) {
					suppress_cal = TRUE;
				} else {
					pi->cal_info->last_cal_temp = current_temp;
				}
			}

			if (suppress_cal) {
				wlc_phy_txpwr_papd_cal_nphy_dcs(pi);
			} else {
				/* only mphase is allowed */
				if (pi->phy_cal_mode == PHY_PERICAL_MPHASE) {
					pi->cal_info->cal_searchmode = PHY_CAL_SEARCHMODE_REFINE;
					wlc_phy_cal_perical_mphase_schedule(pi,
						PHY_PERICAL_WDOG_DELAY);
				} else {
					ASSERT(0);
				}
			}
		}
		break;
	default:
		ASSERT(0);
		break;
	}
}

void
wlc_phy_trigger_cals_for_btc_adjust(phy_info_t *pi)
{
	wlc_phy_cal_perical_mphase_reset(pi);
	if (ISNPHY(pi)) {
		pi->u.pi_nphy->cal_type_override = PHY_PERICAL_FULL;
	}
	wlc_phy_cal_perical_mphase_schedule(pi, PHY_PERICAL_NODELAY);
}

#if defined(BCMINTERNAL) || defined(WLTEST)

static int
wlc_phy_iovar_idletssi(phy_info_t *pi, int32 *ret_int_ptr, bool type)
{
	/* no argument or type = 1 will do full tx_pwr_ctrl_init */
	/* type = 0 will do just idle_tssi_est */
	int err = BCME_OK;
	if (ISLCN40PHY(pi))
		*ret_int_ptr = wlc_lcn40phy_idle_tssi_est_iovar(pi, type);
	else
		*ret_int_ptr = 0;

	return err;
}

static int
wlc_phy_iovar_bbmult_get(phy_info_t *pi, int32 int_val, bool bool_val, int32 *ret_int_ptr)
{
	int err = BCME_OK;

	if (ISNPHY(pi))
		wlc_phy_get_bbmult_nphy(pi, ret_int_ptr);
	else
		err = BCME_UNSUPPORTED;

	return err;
}

static int
wlc_phy_iovar_bbmult_set(phy_info_t *pi, void *p)
{
	int err = BCME_OK;
	uint16 bbmult[PHY_CORE_NUM_2] = { 0 };
	uint8 m0, m1;

	bcopy(p, bbmult, PHY_CORE_NUM_2 * sizeof(uint16));

	if (ISNPHY(pi)) {
		m0 = (uint8)(bbmult[0] & 0xff);
		m1 = (uint8)(bbmult[1] & 0xff);
		wlc_phy_set_bbmult_nphy(pi, m0, m1);
	} else
		err = BCME_UNSUPPORTED;

	return err;
}

#if (defined(BCMINTERNAL) || defined(WLTEST))
static int
wlc_phy_iovar_avvmid_get(phy_info_t *pi, void *p, bool bool_val,
	int32 *ret_int_ptr, wlc_avvmid_t avvmid_type)
{
	int err = BCME_OK;
	uint8 core_sub_band[2];
	bcopy(p, core_sub_band, 2*sizeof(uint8));
	BCM_REFERENCE(bool_val);
	if (ISACPHY(pi))
		wlc_phy_get_avvmid_acphy(pi, ret_int_ptr, avvmid_type, core_sub_band);
	else
		err = BCME_UNSUPPORTED;
	return err;
}

static int
wlc_phy_iovar_avvmid_set(phy_info_t *pi, void *p, wlc_avvmid_t avvmid_type)
{
	int err = BCME_OK;
	uint8 avvmid[3];
	bcopy(p, avvmid, 3*sizeof(uint8));
	if (ISACPHY(pi)) {
		wlc_phy_set_avvmid_acphy(pi, avvmid, avvmid_type);
	} else
		err = BCME_UNSUPPORTED;
	return err;
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

static int
wlc_phy_iovar_vbatsense(phy_info_t *pi, int32 *ret_int_ptr)
{
	int err = BCME_OK;
	int32 int_val;

	if (ISLCN40PHY(pi)) {
		int_val = wlc_lcn40phy_vbatsense(pi, TEMPER_VBAT_TRIGGER_NEW_MEAS);
		bcopy(&int_val, ret_int_ptr, sizeof(int_val));
	} else if (ISNPHY(pi)) {
		int_val = (int32)(wlc_phy_vbat_from_statusbyte_nphy_rev19(pi));
		bcopy(&int_val, ret_int_ptr, sizeof(int_val));
	} else
		err = BCME_UNSUPPORTED;

	return err;
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

#if defined(BCMINTERNAL) || defined(WLTEST) || defined(AP) || defined(BCMDBG_PHYCAL)
static int
wlc_phy_iovar_perical_config(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr,	bool set)
{
	int err = BCME_OK;

	if (!set) {
		if (!ISNPHY(pi) && !ISHTPHY(pi) && !ISACPHY(pi))
			/* supported for n, ht, ac and lcn phy only */
			return BCME_UNSUPPORTED;

		*ret_int_ptr =  pi->phy_cal_mode;
	} else {
		if (!ISNPHY(pi) && !ISHTPHY(pi) && !ISACPHY(pi))
			/* supported for n, ht, ac and lcn phy only */
			return BCME_UNSUPPORTED;

		if (int_val == 0) {
			pi->phy_cal_mode = PHY_PERICAL_DISABLE;
		} else if (int_val == 1) {
			pi->phy_cal_mode = PHY_PERICAL_SPHASE;
		} else if (int_val == 2) {
			if ((ISACPHY(pi) && ACREV_IS(pi->pubpi->phy_rev, 1)) ||
				ACMAJORREV_32(pi->pubpi->phy_rev) ||
				ACMAJORREV_33(pi->pubpi->phy_rev) ||
				ACMAJORREV_37(pi->pubpi->phy_rev)) {
				pi->phy_cal_mode = PHY_PERICAL_MPHASE;
				/* enabling MPHASE for 4360 B0 and 4366 */
			} else {
				pi->phy_cal_mode = PHY_PERICAL_SPHASE;
			}
		} else if (int_val == 3) {
			/* this mode is to disable normal periodic cal paths
			 *  only manual trigger(nphy_forcecal) can run it
			 */
			pi->phy_cal_mode = PHY_PERICAL_MANUAL;
		} else {
			err = BCME_RANGE;
			goto end;
		}
		wlc_phy_cal_perical_mphase_reset(pi);
	}
end:
	return err;
}
#endif	/* defined(BCMINTERNAL) || defined(WLTEST) || defined(AP) */

#if defined(BCMDBG) || defined(WLTEST) || defined(MACOSX) || defined(ATE_BUILD) || \
	defined(BCMDBG_TEMPSENSE)
static int
wlc_phy_iovar_tempsense_paldosense(phy_info_t *pi, int32 *ret_int_ptr, uint8 tempsense_paldosense)
{
	int err = BCME_OK;
	int32 int_val;

	*ret_int_ptr = 0;
	if (ISNPHY(pi)) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		*ret_int_ptr = (int32)wlc_phy_tempsense_nphy(pi);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
	} else if (ISHTPHY(pi)) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		*ret_int_ptr = (int32)wlc_phy_tempsense_htphy(pi);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
	} else if (ISACPHY(pi)) {
		/* No need to call suspend_mac and phyreg_enter since it
		* is done inside wlc_phy_tempsense_acphy
		*/
	  if (pi->radio_is_on)
	    *ret_int_ptr = (int32)wlc_phy_tempsense_acphy(pi);
	  else
	    err = BCME_RADIOOFF;
	} else if (ISLCN40PHY(pi)) {
		int_val = wlc_lcn40phy_tempsense(pi, TEMPER_VBAT_TRIGGER_NEW_MEAS);
		bcopy(&int_val, ret_int_ptr, sizeof(int_val));
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	} else if (ISLCN20PHY(pi)) {
		int_val = wlc_lcn20phy_tempsense(pi, TEMPER_VBAT_TRIGGER_NEW_MEAS);
		bcopy(&int_val, ret_int_ptr, sizeof(int_val));
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
	} else
		err = BCME_UNSUPPORTED;

	return err;
}
#endif	/* defined(BCMDBG) || defined(WLTEST) || defined(MACOSX) || defined(ATE_BUILD) ||
		* defined(BCMDBG_TEMPSENSE)
		*/

#if defined(BCMINTERNAL) || defined(WLTEST)
static int
wlc_phy_iovar_idletssi_reg(phy_info_t *pi, int32 *ret_int_ptr, int32 int_val, bool set)
{
	int err = BCME_OK;
	uint32 tmp;
	uint16 idle_tssi[NPHY_CORE_NUM];
	phy_info_nphy_t *pi_nphy = pi->u.pi_nphy;

	if (ISLCN40PHY(pi))
		*ret_int_ptr = wlc_lcn40phy_idle_tssi_reg_iovar(pi, int_val, set, &err);
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	else if (ISLCN20PHY(pi))
		*ret_int_ptr = wlc_lcn20phy_idle_tssi_reg_iovar(pi, int_val, set, &err);
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
	else if (ISHTPHY(pi))
		*ret_int_ptr = wlc_phy_idletssi_get_htphy(pi);
	else if (ISNPHY(pi)) {
		if (!(CHIP_4324_B0(pi) || CHIP_4324_B4(pi))) {
			wlc_phy_lcnxn_rx2tx_stallwindow_nphy(pi, 1);
			wlc_phy_txpwrctrl_idle_tssi_nphy(pi);
			wlc_phy_lcnxn_rx2tx_stallwindow_nphy(pi, 0);
		}
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			idle_tssi[0] = (uint16)pi_nphy->nphy_pwrctrl_info[0].idle_tssi_2g;
			idle_tssi[1] = (uint16)pi_nphy->nphy_pwrctrl_info[1].idle_tssi_2g;
		} else {
			idle_tssi[0] = (uint16)pi_nphy->nphy_pwrctrl_info[0].idle_tssi_5g;
			idle_tssi[1] = (uint16)pi_nphy->nphy_pwrctrl_info[1].idle_tssi_5g;
		}
		tmp = (idle_tssi[1] << 16) | idle_tssi[0];
		*ret_int_ptr = tmp;
	}

	return err;
}

static int
wlc_phy_iovar_avgtssi_reg(phy_info_t *pi, int32 *ret_int_ptr)
{
	int err = BCME_OK;
	if (ISLCN40PHY(pi))
		*ret_int_ptr = wlc_lcn40phy_avg_tssi_reg_iovar(pi);
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	else if (ISLCN20PHY(pi))
		*ret_int_ptr = wlc_lcn20phy_avg_tssi_reg_iovar(pi);
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
	else if (ISNPHY(pi)) {
		*ret_int_ptr = wlc_nphy_tssi_read_iovar(pi);
	}
	return err;
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

static int
wlc_phy_iovar_forcecal(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, int vsize, bool set)
{
	int err = BCME_OK;
	void *a;
#if defined(PHYCAL_CACHING)
	ch_calcache_t *ctx;
#endif /* PHYCAL_CACHING */

	a = (int32*)ret_int_ptr;

	if (!pi->sh->up)
		return BCME_NOTUP;

	if (ISHTPHY(pi)) {
		uint8 mphase = 0, searchmode = 0;

		/* for get with no argument, assume 0x00 */
		if (!set)
			int_val = 0x00;

		/* upper nibble: 0 = sphase,  1 = mphase */
		mphase = (((uint8) int_val) & 0xf0) >> 4;

		/* 0 = RESTART, 1 = REFINE, for Tx-iq/lo-cal */
		searchmode = ((uint8) int_val) & 0xf;

		PHY_CAL(("wlc_phy_iovar_forcecal (mphase = %d, refine = %d)\n",
			mphase, searchmode));

		/* call sphase or schedule mphase cal */
		wlc_phy_cal_perical_mphase_reset(pi);
		if (mphase) {
			pi->cal_info->cal_searchmode = searchmode;
			wlc_phy_cal_perical_mphase_schedule(pi, PHY_PERICAL_NODELAY);
		} else {
			wlc_phy_cals_htphy(pi, searchmode);
		}
	} else if (ISACPHY(pi)) {
		uint8 mphase = FALSE;
		uint8 searchmode = PHY_CAL_SEARCHMODE_RESTART;
		/* Vbat monitoring done before triggering cals */
		if ((ACMAJORREV_4(pi->pubpi->phy_rev)) && !(ROUTER_4349(pi))) {
			if ((pi->ldo3p3_voltage == -1) && (pi->paldo3p3_voltage == -1)) {
				wlc_phy_vbat_monitoring_algorithm_decision(pi);
			}
		}
		/* for get with no argument, assume 0x00 */
		if (!set)
			int_val = 0x00;

		/* only values in range [0-3] are valids */
		if (int_val > 3)
			return BCME_BADARG;

		/* 3 is mphase, anything else is single phase */
		if (int_val == 3) {
			mphase = TRUE;
		}
		else {
			/* Single phase, using 2 means sphase partial */
			if (int_val == 2)
				searchmode = PHY_CAL_SEARCHMODE_REFINE;
		}

		PHY_CAL(("wlc_phy_iovar_forcecal (mphase = %d, refine = %d)\n",
			mphase, searchmode));

		/* reset the noise array to clean the cache */
		wlc_phy_clean_noise_array(pi);

		/* call sphase or schedule mphase cal */
		wlc_phy_cal_perical_mphase_reset(pi);
		if (mphase) {
			pi->cal_info->cal_searchmode = searchmode;
			wlc_phy_cal_perical_mphase_schedule(pi, PHY_PERICAL_NODELAY);
		} else {
			wlc_phy_cals_acphy(pi->u.pi_acphy->calmgri, PHY_PERICAL_UNDEF,
					searchmode);
		}
	} else if (ISNPHY(pi)) {
		/* for get with no argument, assume 0x00 */
		if (!set)
			int_val = PHY_PERICAL_AUTO;

		if ((int_val == PHY_PERICAL_PARTIAL) ||
		    (int_val == PHY_PERICAL_AUTO) ||
		    (int_val == PHY_PERICAL_FULL)) {
			wlc_phy_cal_perical_mphase_reset(pi);
			pi->u.pi_nphy->cal_type_override = (uint8)int_val;
			wlc_phy_cal_perical_mphase_schedule(pi, PHY_PERICAL_NODELAY);
#ifdef WLOTA_EN
		} else if (int_val == PHY_FULLCAL_SPHASE) {
			wlc_phy_cal_perical((wlc_phy_t *)pi, PHY_FULLCAL_SPHASE);
#endif /* WLOTA_EN */
		} else
			err = BCME_RANGE;

		/* phy_forcecal will trigger noisecal */
		pi->trigger_noisecal = TRUE;

	} else if (ISLCNCOMMONPHY(pi) && pi->pi_fptr->calibmodes) {
#if defined(PHYCAL_CACHING)
		ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);
		if (ctx) {
			ctx->valid = FALSE;
		}
		/* null ctx is invalid by definition */
#else
		phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
		pi_lcn->lcnphy_full_cal_channel = 0;
#endif /* PHYCAL_CACHING */
		if (!set)
			*ret_int_ptr = 0;

		pi->pi_fptr->calibmodes(pi, PHY_FULLCAL);
		int_val = 0;
		bcopy(&int_val, a, vsize);
	} else if (ISLCN20PHY(pi)) {
		pi->pi_fptr->calibmodes(pi, PHY_FULLCAL);
	}

	return err;
}

#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG) || defined(ATE_BUILD)
#ifndef ATE_BUILD
static int
wlc_phy_iovar_forcecal_obt(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, int vsize, bool set)
{
	int err = BCME_OK;
	uint8 wait_ctr = 0;
	int val = 1;
	if (ISNPHY(pi)) {
			wlc_phy_cal_perical_mphase_reset(pi);

			pi->cal_info->cal_phase_id = MPHASE_CAL_STATE_INIT;
			pi->trigger_noisecal = TRUE;

			while (wait_ctr < 50) {
				val = ((pi->cal_info->cal_phase_id !=
					MPHASE_CAL_STATE_IDLE)? 1 : 0);
				if (val == 0) {
					err = BCME_OK;
					break;
				}
				else {
					wlc_phy_cal_perical_nphy_run(pi, PHY_PERICAL_FULL);
					wait_ctr++;
				}
			}
			if (wait_ctr >= 50) {
				return BCME_ERROR;
			}

		}

	return err;
}
#endif /* !ATE_BUILD */
#endif /* BCMINTERNAL || WLTEST || ATE_BUILD */

#if defined(BCMINTERNAL) || defined(WLTEST) || defined(MACOSX)
static void
wlc_phy_iovar_set_deaf(phy_info_t *pi, int32 int_val)
{
	if (int_val) {
		wlc_phy_set_deaf((wlc_phy_t *) pi, TRUE);
	} else {
		wlc_phy_clear_deaf((wlc_phy_t *) pi, TRUE);
	}
}

static int
wlc_phy_iovar_get_deaf(phy_info_t *pi, int32 *ret_int_ptr)
{
	if (ISHTPHY(pi)) {
		*ret_int_ptr = (int32)wlc_phy_get_deaf_htphy(pi);
		return BCME_OK;
	} else if (ISNPHY(pi)) {
		*ret_int_ptr = (int32)wlc_phy_get_deaf_nphy(pi);
		return BCME_OK;
	} else if (ISACPHY(pi)) {
	        *ret_int_ptr = (int32)wlc_phy_get_deaf_acphy(pi);
		return BCME_OK;
	} else {
		return BCME_UNSUPPORTED;
	}
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(MACOSX) */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD)
static int
wlc_phy_iovar_txpwrctrl(phy_info_t *pi, int32 int_val, bool bool_val, int32 *ret_int_ptr,
	bool set)
{
	int err = BCME_OK;

	if (!set) {
		if (ISACPHY(pi) || ISHTPHY(pi)) {
			*ret_int_ptr = pi->txpwrctrl;
		} else if (ISNPHY(pi)) {
			*ret_int_ptr = pi->nphy_txpwrctrl;
#if (defined(LCN20CONF) && (LCN20CONF != 0))
		} else if (ISLCN20PHY(pi)) {
				err = wlc_lcn20phy_iovar_isenabled_tpc(pi, ret_int_ptr);
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
		} else if (ISLCN40PHY(pi)) {
			*ret_int_ptr = wlc_phy_tpc_iovar_isenabled_lcn40phy(pi);
		}

	} else {
		if (ISNPHY(pi) || ISHTPHY(pi) || ISACPHY(pi)) {
			if ((int_val != PHY_TPC_HW_OFF) && (int_val != PHY_TPC_HW_ON)) {
				err = BCME_RANGE;
				goto end;
			}

			pi->nphy_txpwrctrl = (uint8)int_val;
			pi->txpwrctrl = (uint8)int_val;

			/* if not up, we are done */
			if (!pi->sh->up)
				goto end;

			wlapi_suspend_mac_and_wait(pi->sh->physhim);
			phy_utils_phyreg_enter(pi);
			if (ISNPHY(pi))
				wlc_phy_txpwrctrl_enable_nphy(pi, (uint8) int_val);
			else if (ISHTPHY(pi))
				wlc_phy_txpwrctrl_enable_htphy(pi, (uint8) int_val);
			else if (ISACPHY(pi))
				wlc_phy_txpwrctrl_enable_acphy(pi, (uint8) int_val);
			phy_utils_phyreg_exit(pi);
			wlapi_enable_mac(pi->sh->physhim);

		} else if (ISLCN40PHY(pi)) {
			wlc_lcn40phy_iovar_txpwrctrl(pi, int_val, ret_int_ptr);
#if (defined(LCN20CONF) && (LCN20CONF != 0))
		} else if (ISLCN20PHY(pi)) {
			err = wlc_lcn20phy_iovar_txpwrctrl(pi, int_val, ret_int_ptr);
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
		}
	}

end:
	return err;
}

static int
wlc_phy_iovar_txpwrindex_get(phy_info_t *pi, int32 int_val, bool bool_val, int32 *ret_int_ptr)
{
	int err = BCME_OK;

	if (ISNPHY(pi)) {
		*ret_int_ptr = wlc_phy_txpwr_idx_get_nphy(pi);
	} else if (ISHTPHY(pi)) {
		*ret_int_ptr = wlc_phy_txpwr_idx_get_htphy(pi);
	} else if (ISACPHY(pi)) {
		*ret_int_ptr = wlc_phy_txpwr_idx_get_acphy(pi);
	} else if (ISLCN40PHY(pi))
		*ret_int_ptr = wlc_lcn40phy_get_current_tx_pwr_idx(pi);
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	else if (ISLCN20PHY(pi))
		*ret_int_ptr = wlc_lcn20phy_get_current_tx_pwr_idx(pi);
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
	return err;
}


static int
wlc_phy_iovar_txpwrindex_set(phy_info_t *pi, void *p)
{
	int err = BCME_OK;
	uint32 txpwridx[PHY_CORE_MAX] = { 0x30 };
	int8 idx, core;
	int8 siso_int_val;
	phy_info_nphy_t *pi_nphy = NULL;
#if defined(PHYCAL_CACHING)
	ch_calcache_t *ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);
#endif

	if (ISNPHY(pi))
		pi_nphy = pi->u.pi_nphy;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	bcopy(p, txpwridx, PHY_CORE_MAX * sizeof(uint32));
	siso_int_val = (int8)(txpwridx[0] & 0xff);

	if (ISNPHY(pi)) {
		FOREACH_CORE(pi, core) {
			idx = (int8)(txpwridx[core] & 0xff);
			pi_nphy->nphy_txpwrindex[core].index_internal = idx;
			wlc_phy_store_txindex_nphy(pi);
			wlc_phy_txpwr_index_nphy(pi, (1 << core), idx, TRUE);
		}
	} else if (ISHTPHY(pi)) {
		FOREACH_CORE(pi, core) {
			idx = (int8)(txpwridx[core] & 0xff);
			wlc_phy_txpwr_by_index_htphy(pi, (1 << core), idx);
		}
	} else if (ISACPHY(pi)) {
		FOREACH_CORE(pi, core) {
			idx = (int8)(txpwridx[core] & 0xff);
			wlc_phy_txpwrctrl_enable_acphy(pi, PHY_TPC_HW_OFF);
			wlc_phy_txpwr_by_index_acphy(pi, (1 << core), idx);
		}
	} else if (ISLCNCOMMONPHY(pi)) {
#if defined(PHYCAL_CACHING)
		err = wlc_iovar_txpwrindex_set_lcncommon(pi, siso_int_val, ctx);
#else
		err = wlc_iovar_txpwrindex_set_lcncommon(pi, siso_int_val);
#endif
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	} else if (ISLCN20PHY(pi)) {
		wlc_lcn20phy_set_tx_pwr_by_index(pi, siso_int_val);
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
	}

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	return err;
}

#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD) */

#if defined(BCMINTERNAL) || defined(WLTEST)
static int
wlc_phy_iovar_txrx_chain(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, bool set)
{
	int err = BCME_OK;

	if (ISHTPHY(pi))
		return BCME_UNSUPPORTED;

	if (!set) {
		if (ISNPHY(pi)) {
			*ret_int_ptr = (int)pi->nphy_txrx_chain;
		}
	} else {
		if (ISNPHY(pi)) {
			if ((int_val != AUTO) && (int_val != WLC_N_TXRX_CHAIN0) &&
				(int_val != WLC_N_TXRX_CHAIN1)) {
				err = BCME_RANGE;
				goto end;
			}

			if (pi->nphy_txrx_chain != (int8)int_val) {
				pi->nphy_txrx_chain = (int8)int_val;
				if (pi->sh->up) {
					wlapi_suspend_mac_and_wait(pi->sh->physhim);
					phy_utils_phyreg_enter(pi);
					wlc_phy_stf_chain_upd_nphy(pi);
					wlc_phy_force_rfseq_nphy(pi, NPHY_RFSEQ_RESET2RX);
					phy_utils_phyreg_exit(pi);
					wlapi_enable_mac(pi->sh->physhim);
				}
			}
		}
	}
end:
	return err;
}

static void
wlc_phy_iovar_bphy_testpattern(phy_info_t *pi, uint8 testpattern, bool enable)
{
	bool existing_enable = FALSE;

	/* WL out check */
	if (pi->sh->up) {
		PHY_ERROR(("wl%d: %s: This function needs to be called after 'wl out'\n",
		          pi->sh->unit, __FUNCTION__));
		return;
	}

	/* confirm band is locked to 2G */
	if (!CHSPEC_IS2G(pi->radio_chanspec)) {
		PHY_ERROR(("wl%d: %s: Band needs to be locked to 2G (b)\n",
		          pi->sh->unit, __FUNCTION__));
		return;
	}

	if (ISHTPHY(pi)) {
		PHY_ERROR(("wl%d: %s: This function is supported only for NPHY\n",
		          pi->sh->unit, __FUNCTION__));
		return;
	}

	if (testpattern == NPHY_TESTPATTERN_BPHY_EVM) {    /* CW CCK for EVM testing */
		existing_enable = (bool) pi->phy_bphy_evm;
	} else if (testpattern == NPHY_TESTPATTERN_BPHY_RFCS) { /* RFCS testpattern */
		existing_enable = (bool) pi->phy_bphy_rfcs;
	} else {
		PHY_ERROR(("Testpattern needs to be between [0 (BPHY_EVM), 1 (BPHY_RFCS)]\n"));
		ASSERT(0);
	}

	if (ISNPHY(pi)) {
		wlc_phy_bphy_testpattern_nphy(pi, testpattern, enable, existing_enable);
	} else {
		PHY_ERROR(("support yet to be added\n"));
		ASSERT(0);
	}

	/* Return state of testpattern enables */
	if (testpattern == NPHY_TESTPATTERN_BPHY_EVM) {    /* CW CCK for EVM testing */
		pi->phy_bphy_evm = enable;
	} else if (testpattern == NPHY_TESTPATTERN_BPHY_RFCS) { /* RFCS testpattern */
		pi->phy_bphy_rfcs = enable;
	}
}

static void
wlc_phy_iovar_scraminit(phy_info_t *pi, int8 scraminit)
{
	pi->phy_scraminit = (int8)scraminit;
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	if (ISNPHY(pi)) {
		wlc_phy_test_scraminit_nphy(pi, scraminit);
	} else if (ISHTPHY(pi)) {
		wlc_phy_test_scraminit_htphy(pi, scraminit);
	} else if (ISACPHY(pi)) {
		wlc_phy_test_scraminit_acphy(pi, scraminit);
	} else {
		PHY_ERROR(("support yet to be added\n"));
		ASSERT(0);
	}

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}

static int
wlc_phy_iovar_force_rfseq(phy_info_t *pi, uint8 int_val)
{
	int err = BCME_OK;

	phy_utils_phyreg_enter(pi);
	if (ISNPHY(pi)) {
		wlc_phy_force_rfseq_nphy(pi, int_val);
	} else if (ISHTPHY(pi)) {
		wlc_phy_force_rfseq_htphy(pi, int_val);
	} else if (ISACPHY(pi)) {
		wlc_phy_force_rfseq_acphy(pi, int_val);
	} else {
		err = BCME_UNSUPPORTED;
	}
	phy_utils_phyreg_exit(pi);

	return err;
}

static void
wlc_phy_iovar_tx_tone_hz(phy_info_t *pi, int32 int_val)
{
	pi->phy_tx_tone_freq = (int32) int_val;

	if (ISLCN40PHY(pi)) {
		wlc_lcn40phy_set_tx_tone_and_gain_idx(pi);
	}
}

static void
wlc_phy_iovar_tx_tone_stop(phy_info_t *pi)
{
	if (ISLCN40PHY(pi)) {
		wlc_lcn40phy_stop_tx_tone(pi);
	}
}
static int
wlc_phy_iovar_tx_tone_symm(phy_info_t *pi, int32 int_val)
{
	int err = BCME_OK;
	pi->phy_tx_tone_freq = (int32) int_val;

	pi->phytxtone_symm = TRUE;

	if (ISACPHY(pi)) {
	        if (pi->phy_tx_tone_freq == 0) {
	                wlc_phy_stopplayback_acphy(pi);
	                wlc_phy_stay_in_carriersearch_acphy(pi, FALSE);
	                wlapi_enable_mac(pi->sh->physhim);
	        } else {
	                pi->phy_tx_tone_freq = pi->phy_tx_tone_freq * 1000; /* Covert to Hz */
	                wlapi_suspend_mac_and_wait(pi->sh->physhim);
	                wlc_phy_stay_in_carriersearch_acphy(pi, TRUE);
	                wlc_phy_tx_tone_acphy(pi, (int32)int_val, 151, 0, 0, TRUE);
	        }
	} else {
	        err = BCME_UNSUPPORTED;
	}

	/* Disable symmetrical tone, falling back to default setting */
	pi->phytxtone_symm = FALSE;

	return err;
}

static int16
wlc_phy_iovar_test_tssi(phy_info_t *pi, uint8 val, uint8 pwroffset)
{
	int16 tssi = 0;
	if (ISNPHY(pi)) {
		tssi = (int16) wlc_phy_test_tssi_nphy(pi, val, pwroffset);
	} else if (ISHTPHY(pi)) {
		tssi = (int16) wlc_phy_test_tssi_htphy(pi, val, pwroffset);
	} else if (ISACPHY(pi)) {
		tssi = (int16) wlc_phy_test_tssi_acphy(pi, val, pwroffset);
	} else if (ISLCN20PHY(pi)) {
		tssi = (int16) wlc_phy_test_tssi_lcn20phy(pi, val, pwroffset);
	}
	return tssi;
}

static int16
wlc_phy_iovar_test_idletssi(phy_info_t *pi, uint8 val)
{
	int16 idletssi = INVALID_IDLETSSI_VAL;
	if (ISACPHY(pi)) {
		idletssi = (int16) wlc_phy_test_idletssi_acphy(pi, val);
	} else if (ISLCN20PHY(pi)) {
		idletssi = (int16) wlc_phy_test_idletssi_lcn20phy(pi, val);
	}
	return idletssi;
}

static int16
wlc_phy_iovar_setrptbl(phy_info_t *pi)
{
	if (ISACPHY(pi) && (!ACMAJORREV_1(pi->pubpi->phy_rev))) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		wlc_phy_populate_recipcoeffs_acphy(pi);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
		return 0;
	}

	return BCME_UNSUPPORTED;
}

static int16
wlc_phy_iovar_forceimpbf(phy_info_t *pi)
{
	if (ISACPHY(pi) && (!ACMAJORREV_1(pi->pubpi->phy_rev))) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		phy_utils_write_phyreg(pi, ACPHY_BfeConfigReg0(pi->pubpi->phy_rev),
		                       BFECONFIGREF_FORCEVAL);

		if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
		    ACMAJORREV_33(pi->pubpi->phy_rev) ||
		    ACMAJORREV_37(pi->pubpi->phy_rev)) {
			WRITE_PHYREG(pi, BfeMuConfigReg1, 0x1000);
			WRITE_PHYREG(pi, BfeMuConfigReg2, 0x2000);
			WRITE_PHYREG(pi, BfeMuConfigReg3, 0x1000);
		}

		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
		return 0;
	}

	return BCME_UNSUPPORTED;
}

static int16
wlc_phy_iovar_forcesteer(phy_info_t *pi, uint8 enable)
{
#if (ACCONF || ACCONF2) && defined(WL_BEAMFORMING)
	uint16 bfmcon_val      = 0;
	uint16 bfridx_pos_val  = 0;
	uint16 refresh_thr_val = 0;
	uint16 shm_base, addr1, addr2, bfrctl = 0;

	if (ISACPHY(pi) && (!ACMAJORREV_1(pi->pubpi->phy_rev))) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);

		bfmcon_val      = enable ? BFMCON_FORCEVAL      : BFMCON_RELEASEVAL;
		bfridx_pos_val  = enable ? BFRIDX_POS_FORCEVAL  : BFRIDX_POS_RELEASEVAL;
		refresh_thr_val = enable ? REFRESH_THR_FORCEVAL : REFRESH_THR_RELEASEVAL;

		shm_base = wlapi_bmac_read_shm(pi->sh->physhim, M_BFCFG_PTR(pi));
		shm_base = wlapi_bmac_read_shm(pi->sh->physhim, shm_base * 2);

		/* NDP streams */
		if (PHY_BITSCNT(pi->sh->phytxchain) == 4) {
			/* 4 streams */
			bfrctl = (2 << C_BFI_BFRCTL_POS_NSTS_SHIFT);
		} else if (PHY_BITSCNT(pi->sh->phytxchain) == 3) {
			/* 3 streams */
			bfrctl = (1 << C_BFI_BFRCTL_POS_NSTS_SHIFT);
		} else if (PHY_BITSCNT(pi->sh->phytxchain) == 2) {
			/* 2 streams */
			bfrctl = 0;
		}
		bfrctl |= (pi->sh->phytxchain << C_BFI_BFRCTL_POS_BFM_SHIFT);
		wlapi_bmac_write_shm(pi->sh->physhim, (shm_base + C_BFI_BFRCTL_POS) * 2, bfrctl);

		addr1 = (shm_base + M_BFI_REFRESH_THR_OFFSET(pi))*2;
		addr2 = (shm_base + C_BFI_BFRIDX_POS)* 2;
		phy_utils_write_phyreg(pi, ACPHY_BfmCon(pi->pubpi->phy_rev), bfmcon_val);

		if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
		    ACMAJORREV_33(pi->pubpi->phy_rev) ||
		    ACMAJORREV_37(pi->pubpi->phy_rev)) {
			uint32 tmpaddr = 0x1000;
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_BFMUSERINDEX,
			1, 0x1000, 32, &tmpaddr);
			MOD_PHYREG(pi, BfeMuConfigReg0, useTxbfIndexAddr, 1);
		}

		wlapi_bmac_write_shm(pi->sh->physhim, addr1, refresh_thr_val);
		wlapi_bmac_write_shm(pi->sh->physhim, addr2, bfridx_pos_val);

		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
		return 0;
	}
#endif /* (ACCONF || ACCONF2) && WL_BEAMFORMING */

	return BCME_UNSUPPORTED;
}

static void
wlc_phy_iovar_rxcore_enable(phy_info_t *pi, int32 int_val, bool bool_val, int32 *ret_int_ptr,
	bool set)
{
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	if (set) {
		if (ISNPHY(pi)) {
			wlc_phy_rxcore_setstate_nphy((wlc_phy_t *)pi, (uint8) int_val, 0);
		} else if (ISHTPHY(pi)) {
			wlc_phy_rxcore_setstate_htphy((wlc_phy_t *)pi, (uint8) int_val);
		} else if (ISACPHY(pi)) {
			wlc_phy_rxcore_setstate_acphy((wlc_phy_t *)pi,
			    (uint8) int_val, pi->sh->phytxchain);
		}
	} else {
		if (ISNPHY(pi)) {
			*ret_int_ptr =  (uint32)wlc_phy_rxcore_getstate_nphy((wlc_phy_t *)pi);
		} else if (ISHTPHY(pi)) {
			*ret_int_ptr =  (uint32)wlc_phy_rxcore_getstate_htphy((wlc_phy_t *)pi);
		} else if (ISACPHY(pi)) {
			*ret_int_ptr =  (uint32)wlc_phy_rxcore_getstate_acphy((wlc_phy_t *)pi);
		}
	}

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}

#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

static int
wlc_phy_iovar_set_dssf(phy_info_t *pi, int32 set_val)
{
	if (ISACPHY(pi) && PHY_ILNA(pi)) {
	  phy_utils_write_phyreg(pi, ACPHY_DSSF_C_CTRL(pi->pubpi->phy_rev), (uint16) set_val);

		return BCME_OK;
	}

	return BCME_UNSUPPORTED;
}

static int
wlc_phy_iovar_get_dssf(phy_info_t *pi, int32 *ret_val)
{
	if (ISACPHY(pi) && PHY_ILNA(pi)) {
		*ret_val = (int32) phy_utils_read_phyreg(pi, ACPHY_DSSF_C_CTRL(pi->pubpi->phy_rev));

		return BCME_OK;
	}

	return BCME_UNSUPPORTED;
}

#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || defined(BCMDBG)
static void
wlc_phy_iovar_set_papd_offset(phy_info_t *pi, int16 int_val)
{
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	if (ISLCN20PHY(pi)) {
		wlc_phy_set_papd_offset_lcn20phy(pi, int_val);
	}
#endif /* #if ((defined(LCN20CONF) && (LCN20CONF != 0))) */
}

static int
wlc_phy_iovar_get_papd_offset(phy_info_t *pi)
{
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	if (ISLCN20PHY(pi)) {
		return wlc_phy_get_papd_offset_lcn20phy(pi);
	} else
#endif /* #if ((defined(LCN20CONF) && (LCN20CONF != 0))) */
		return BCME_UNSUPPORTED;
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || defined(BCMDBG) */

#if NCONF
static int
wlc_phy_iovar_oclscd(phy_info_t *pi, int32 int_val, bool bool_val, int32 *ret_int_ptr,
	bool set)
{
	int err = BCME_OK;

	if (!pi->sh->clk)
		return BCME_NOCLK;

	if (!ISNPHY(pi))
		return BCME_UNSUPPORTED;
	else if (NREV_LT(pi->pubpi->phy_rev, LCNXN_BASEREV + 2))
		return BCME_UNSUPPORTED;

	if (!set) {
		if (ISNPHY(pi)) {
			*ret_int_ptr = pi->nphy_oclscd;
		}

	} else {
		if (ISNPHY(pi)) {
			uint8 coremask = ((phy_utils_read_phyreg(pi, NPHY_CoreConfig) &
				NPHY_CoreConfig_CoreMask_MASK) >> NPHY_CoreConfig_CoreMask_SHIFT);

			if ((int_val > 3) || (int_val < 0))
				return BCME_RANGE;

			if (int_val == 2)
				return BCME_BADARG;

			if ((coremask < 3) && (int_val != 0))
				return BCME_NORESOURCE;

			pi->nphy_oclscd = (uint8)int_val;
			/* suspend mac */
			wlapi_suspend_mac_and_wait(pi->sh->physhim);

			wlc_phy_set_oclscd_nphy(pi);

			/* resume mac */
			wlapi_enable_mac(pi->sh->physhim);
		}
	}
	return err;
}

/* Debug functionality. Is called via an iovar. */
static int
wlc_phy_iovar_prog_lnldo2(phy_info_t *pi, int32 int_val, bool bool_val, int32 *ret_int_ptr,
	bool set)
{
	int err = BCME_OK;
	uint8 lnldo2_val = 0;
	uint32 reg_value = 0;

	if (!ISNPHY(pi))
		return BCME_UNSUPPORTED;
	else if (!CHIPID_4324X_IPA_FAMILY(pi))
		return BCME_UNSUPPORTED;

	if (!set) {
		/* READ */
		wlc_si_pmu_regcontrol_access(pi, 5, &reg_value, 0);
		*ret_int_ptr = (int32)((reg_value & 0xff) >> 1);
	} else {
		/* WRITE */
		lnldo2_val = (uint8)(int_val & 0xff);
		*ret_int_ptr = wlc_phy_lnldo2_war_nphy(pi, 1, lnldo2_val);
	}
	return err;
}
#endif /* NCONF */

static int
wlc_phy_iovar_dispatch_old(phy_info_t *pi, uint32 actionid, void *p, void *a, int vsize,
	int32 int_val, bool bool_val)
{
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;

	phy_info_nphy_t *pi_nphy;

	pi_nphy = pi->u.pi_nphy;
	BCM_REFERENCE(pi_nphy);
	BCM_REFERENCE(ret_int_ptr);

	switch (actionid) {
#if NCONF
#if defined(BCMDBG)
	case IOV_SVAL(IOV_NPHY_INITGAIN):
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		wlc_phy_setinitgain_nphy(pi, (uint16) int_val);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
		break;

	case IOV_SVAL(IOV_NPHY_HPVGA1GAIN):
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		wlc_phy_sethpf1gaintbl_nphy(pi, (int8) int_val);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
		break;

	case IOV_SVAL(IOV_NPHY_TX_TEMP_TONE): {
		uint16 orig_BBConfig;
		uint16 m0m1;
		nphy_txgains_t target_gain;

		if ((uint32)int_val > 0) {
			pi->phy_tx_tone_freq = (uint32) int_val;
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
			phy_utils_phyreg_enter(pi);
			wlc_phy_stay_in_carriersearch_nphy(pi, TRUE);

			/* Save the bbmult values,since it gets overwritten by mimophy_tx_tone() */
			wlc_phy_table_read_nphy(pi, 15, 1, 87, 16, &m0m1);

			/* Disable the re-sampler (in case we are in spur avoidance mode) */
			orig_BBConfig = phy_utils_read_phyreg(pi, NPHY_BBConfig);
			phy_utils_mod_phyreg(pi, NPHY_BBConfig,
			                     NPHY_BBConfig_resample_clk160_MASK, 0);

			/* read current tx gain and use as target_gain */
			wlc_phy_get_tx_gain_nphy(pi, &target_gain);

			PHY_ERROR(("Tx gain core 0: target gain: ipa = %d,"
			         " pad = %d, pga = %d, txgm = %d, txlpf = %d\n",
			         target_gain.ipa[0], target_gain.pad[0], target_gain.pga[0],
			         target_gain.txgm[0], target_gain.txlpf[0]));

			PHY_ERROR(("Tx gain core 1: target gain: ipa = %d,"
			         " pad = %d, pga = %d, txgm = %d, txlpf = %d\n",
			         target_gain.ipa[0], target_gain.pad[1], target_gain.pga[1],
			         target_gain.txgm[1], target_gain.txlpf[1]));

			/* play a tone for 10 secs and then stop it and return */
			wlc_phy_tx_tone_nphy(pi, (uint32)int_val, 250, 0, 0, FALSE);

			/* Now restore the original bbmult values */
			wlc_phy_table_write_nphy(pi, 15, 1, 87, 16, &m0m1);
			wlc_phy_table_write_nphy(pi, 15, 1, 95, 16, &m0m1);

			OSL_DELAY(10000000);
			wlc_phy_stopplayback_nphy(pi);

			/* Restore the state of the re-sampler
			   (in case we are in spur avoidance mode)
			*/
			phy_utils_write_phyreg(pi, NPHY_BBConfig, orig_BBConfig);

			wlc_phy_stay_in_carriersearch_nphy(pi, FALSE);
			phy_utils_phyreg_exit(pi);
			wlapi_enable_mac(pi->sh->physhim);
		}
		break;
	}
	case IOV_SVAL(IOV_NPHY_CAL_RESET):
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		wlc_phy_cal_reset_nphy(pi, (uint32) int_val);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
		break;

	case IOV_GVAL(IOV_NPHY_EST_TONEPWR):
	case IOV_GVAL(IOV_PHY_EST_TONEPWR): {
		int32 dBm_power[2];
		uint16 orig_BBConfig;
		uint16 m0m1;

		if (ISNPHY(pi)) {
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
			phy_utils_phyreg_enter(pi);

			/* Save the bbmult values, since it gets overwritten
			   by mimophy_tx_tone()
			*/
			wlc_phy_table_read_nphy(pi, 15, 1, 87, 16, &m0m1);

			/* Disable the re-sampler (in case we are in spur avoidance mode) */
			orig_BBConfig = phy_utils_read_phyreg(pi, NPHY_BBConfig);
			phy_utils_mod_phyreg(pi, NPHY_BBConfig,
			                     NPHY_BBConfig_resample_clk160_MASK, 0);
			pi->phy_tx_tone_freq = (uint32) 4000;

			/* play a tone for 10 secs */
			wlc_phy_tx_tone_nphy(pi, (uint32)4000, 250, 0, 0, FALSE);

			/* Now restore the original bbmult values */
			wlc_phy_table_write_nphy(pi, 15, 1, 87, 16, &m0m1);
			wlc_phy_table_write_nphy(pi, 15, 1, 95, 16, &m0m1);

			OSL_DELAY(10000000);
			wlc_phy_est_tonepwr_nphy(pi, dBm_power, 128);
			wlc_phy_stopplayback_nphy(pi);

			/* Restore the state of the re-sampler
			   (in case we are in spur avoidance mode)
			*/
			phy_utils_write_phyreg(pi, NPHY_BBConfig, orig_BBConfig);

			phy_utils_phyreg_exit(pi);
			wlapi_enable_mac(pi->sh->physhim);

			int_val = dBm_power[0]/4;
			bcopy(&int_val, a, vsize);
			break;
		} else {
			err = BCME_UNSUPPORTED;
			break;
		}
	}

	case IOV_GVAL(IOV_NPHY_RFSEQ_TXGAIN): {
		uint16 rfseq_tx_gain[2];
		wlc_phy_table_read_nphy(pi, NPHY_TBL_ID_RFSEQ, 2, 0x110, 16, rfseq_tx_gain);
		int_val = (((uint32) rfseq_tx_gain[1] << 16) | ((uint32) rfseq_tx_gain[0]));
		bcopy(&int_val, a, vsize);
		break;
	}

	case IOV_SVAL(IOV_PHY_SPURAVOID):
		if ((int_val != SPURAVOID_DISABLE) && (int_val != SPURAVOID_AUTO) &&
		    (int_val != SPURAVOID_FORCEON) && (int_val != SPURAVOID_FORCEON2)) {
			err = BCME_RANGE;
			break;
		}

		pi->phy_spuravoid = (int8)int_val;
		break;

	case IOV_GVAL(IOV_PHY_SPURAVOID):
		int_val = pi->phy_spuravoid;
		bcopy(&int_val, a, vsize);
		break;
#endif /* defined(BCMDBG) */

#if defined(BCMINTERNAL) || defined(WLTEST)
	case IOV_GVAL(IOV_NPHY_CCK_PWR_OFFSET):
		if (ISNPHY(pi)) {
			int_val =  pi_nphy->nphy_cck_pwr_err_adjust;
			bcopy(&int_val, a, vsize);
		}
		break;
	case IOV_GVAL(IOV_NPHY_CAL_SANITY):
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		*ret_int_ptr = (uint32)wlc_phy_cal_sanity_nphy(pi);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
		break;

	case IOV_GVAL(IOV_NPHY_BPHY_EVM):
		*ret_int_ptr = pi->phy_bphy_evm;
		break;


	case IOV_SVAL(IOV_NPHY_BPHY_EVM):
		wlc_phy_iovar_bphy_testpattern(pi, NPHY_TESTPATTERN_BPHY_EVM, (bool) int_val);
		break;

	case IOV_GVAL(IOV_NPHY_BPHY_RFCS):
		*ret_int_ptr = pi->phy_bphy_rfcs;
		break;

	case IOV_SVAL(IOV_NPHY_BPHY_RFCS):
		wlc_phy_iovar_bphy_testpattern(pi, NPHY_TESTPATTERN_BPHY_RFCS, (bool) int_val);
		break;

	case IOV_GVAL(IOV_NPHY_SCRAMINIT):
		*ret_int_ptr = pi->phy_scraminit;
		break;

	case IOV_SVAL(IOV_NPHY_SCRAMINIT):
		wlc_phy_iovar_scraminit(pi, pi->phy_scraminit);
		break;

	case IOV_SVAL(IOV_NPHY_RFSEQ):
		err = wlc_phy_iovar_force_rfseq(pi, (uint8)int_val);
		break;

	case IOV_GVAL(IOV_NPHY_TXIQLOCAL): {
		nphy_txgains_t target_gain;
		uint8 tx_pwr_ctrl_state;
		if (ISNPHY(pi)) {

			wlapi_suspend_mac_and_wait(pi->sh->physhim);
			phy_utils_phyreg_enter(pi);

			/* read current tx gain and use as target_gain */
			wlc_phy_get_tx_gain_nphy(pi, &target_gain);
			tx_pwr_ctrl_state = pi->nphy_txpwrctrl;
			wlc_phy_txpwrctrl_enable_nphy(pi, PHY_TPC_HW_OFF);

			err = wlc_phy_cal_txiqlo_nphy(pi, target_gain, TRUE, FALSE);
			if (err)
				break;
			wlc_phy_txpwrctrl_enable_nphy(pi, tx_pwr_ctrl_state);
			phy_utils_phyreg_exit(pi);
			wlapi_enable_mac(pi->sh->physhim);
		}
		*ret_int_ptr = 0;
		break;
	}
	case IOV_SVAL(IOV_NPHY_RXIQCAL): {
		nphy_txgains_t target_gain;
		uint8 tx_pwr_ctrl_state;


		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);

		/* read current tx gain and use as target_gain */
		wlc_phy_get_tx_gain_nphy(pi, &target_gain);
		tx_pwr_ctrl_state = pi->nphy_txpwrctrl;
		wlc_phy_txpwrctrl_enable_nphy(pi, PHY_TPC_HW_OFF);
#ifdef RXIQCAL_FW_WAR
		if (wlc_phy_cal_rxiq_nphy_fw_war(pi, target_gain, 0, (bool)int_val, 0x3) != BCME_OK)
#else
		if (wlc_phy_cal_rxiq_nphy(pi, target_gain, 0, (bool)int_val, 0x3) != BCME_OK)
#endif
		{
			break;
		}
		wlc_phy_txpwrctrl_enable_nphy(pi, tx_pwr_ctrl_state);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
		int_val = 0;
		bcopy(&int_val, a, vsize);
		break;
	}
	case IOV_GVAL(IOV_NPHY_RXCALPARAMS):
		if (ISNPHY(pi)) {
			*ret_int_ptr = pi_nphy->nphy_rxcalparams;
		}
		break;

	case IOV_SVAL(IOV_NPHY_RXCALPARAMS):
		if (ISNPHY(pi)) {
			pi_nphy->nphy_rxcalparams = (uint32)int_val;
		}
		break;

	case IOV_GVAL(IOV_NPHY_TXPWRCTRL):
		wlc_phy_iovar_txpwrctrl(pi, int_val, bool_val, ret_int_ptr, FALSE);
		break;

	case IOV_SVAL(IOV_NPHY_TXPWRCTRL):
		err = wlc_phy_iovar_txpwrctrl(pi, int_val, bool_val, ret_int_ptr, TRUE);
		break;

	case IOV_GVAL(IOV_NPHY_RSSISEL):
		*ret_int_ptr = pi->nphy_rssisel;
		break;

	case IOV_SVAL(IOV_NPHY_RSSISEL):
		pi->nphy_rssisel = (uint8)int_val;

		if (!pi->sh->up)
			break;

		if (pi->nphy_rssisel < 0) {
			phy_utils_phyreg_enter(pi);
			wlc_phy_rssisel_nphy(pi, RADIO_MIMO_CORESEL_OFF, 0);
			phy_utils_phyreg_exit(pi);
		} else {
			int32 rssi_buf[4];
			phy_utils_phyreg_enter(pi);
			wlc_phy_poll_rssi_nphy(pi, (uint8)int_val, rssi_buf, 1);
			phy_utils_phyreg_exit(pi);
		}
		break;

	case IOV_GVAL(IOV_NPHY_RSSICAL): {
		/* if down, return the value, if up, run the cal */
		if (!pi->sh->up) {
			int_val = pi->nphy_rssical;
			bcopy(&int_val, a, vsize);
			break;
		}

		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		/* run rssi cal */
		wlc_phy_rssi_cal_nphy(pi);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
		int_val = pi->nphy_rssical;
		bcopy(&int_val, a, vsize);
		break;
	}

	case IOV_SVAL(IOV_NPHY_RSSICAL): {
		pi->nphy_rssical = bool_val;
		break;
	}

	case IOV_GVAL(IOV_NPHY_GPIOSEL):
	case IOV_GVAL(IOV_PHY_GPIOSEL):
		*ret_int_ptr = pi->phy_gpiosel;
		break;

	case IOV_SVAL(IOV_NPHY_GPIOSEL):
	case IOV_SVAL(IOV_PHY_GPIOSEL):
		pi->phy_gpiosel = (uint16) int_val;

		if (!pi->sh->up)
			break;

		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		if (ISNPHY(pi))
			wlc_phy_gpiosel_nphy(pi, (uint16)int_val);
		else if (ISHTPHY(pi))
			wlc_phy_gpiosel_htphy(pi, (uint16)int_val);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
		break;

	case IOV_GVAL(IOV_NPHY_TX_TONE):
		*ret_int_ptr = pi->phy_tx_tone_freq;
		break;

	case IOV_SVAL(IOV_NPHY_TX_TONE):
		wlc_phy_iovar_tx_tone(pi, (uint32)int_val);
		break;

	case IOV_SVAL(IOV_NPHY_GAIN_BOOST):
		pi->nphy_gain_boost = bool_val;
		break;

	case IOV_GVAL(IOV_NPHY_GAIN_BOOST):
		*ret_int_ptr = (int32)pi->nphy_gain_boost;
		break;

	case IOV_SVAL(IOV_NPHY_ELNA_GAIN_CONFIG):
		pi->nphy_elna_gain_config = (int_val != 0) ? TRUE : FALSE;
		break;

	case IOV_GVAL(IOV_NPHY_ELNA_GAIN_CONFIG):
		*ret_int_ptr = (int32)pi->nphy_elna_gain_config;
		break;

	case IOV_GVAL(IOV_NPHY_TEST_TSSI):
		*((uint*)a) = wlc_phy_iovar_test_tssi(pi, (uint8)int_val, 0);
		break;

	case IOV_GVAL(IOV_NPHY_TEST_TSSI_OFFS):
		*((uint*)a) = wlc_phy_iovar_test_tssi(pi, (uint8)int_val, 12);
		break;

#ifdef BAND5G
	case IOV_SVAL(IOV_NPHY_5G_PWRGAIN):
		pi->phy_5g_pwrgain = bool_val;
		break;

	case IOV_GVAL(IOV_NPHY_5G_PWRGAIN):
		*ret_int_ptr = (int32)pi->phy_5g_pwrgain;
		break;
#endif /* BAND5G */

	case IOV_GVAL(IOV_NPHY_PERICAL):
		wlc_phy_iovar_perical_config(pi, int_val, ret_int_ptr, FALSE);
		break;

	case IOV_SVAL(IOV_NPHY_PERICAL):
		wlc_phy_iovar_perical_config(pi, int_val, ret_int_ptr, TRUE);
		break;

	case IOV_SVAL(IOV_NPHY_FORCECAL):
		err = wlc_phy_iovar_forcecal(pi, int_val, ret_int_ptr, vsize, TRUE);
		break;

#ifndef WLC_DISABLE_ACI
	case IOV_GVAL(IOV_NPHY_ACI_SCAN):
		if (SCAN_INPROG_PHY(pi)) {
			PHY_ERROR(("Scan in Progress, can execute %s\n", __FUNCTION__));
			*ret_int_ptr = -1;
		} else {
			if (pi->cur_interference_mode == INTERFERE_NONE) {
				PHY_ERROR(("interference mode is off\n"));
				*ret_int_ptr = -1;
				break;
			}

			wlapi_suspend_mac_and_wait(pi->sh->physhim);
			*ret_int_ptr = wlc_phy_aci_scan_nphy(pi);
			wlapi_enable_mac(pi->sh->physhim);
		}
		break;
#endif /* Compiling out ACI code for 4324 */
	case IOV_SVAL(IOV_NPHY_ENABLERXCORE):
		wlc_phy_iovar_rxcore_enable(pi, int_val, bool_val, ret_int_ptr, TRUE);
		break;

	case IOV_GVAL(IOV_NPHY_ENABLERXCORE):
		wlc_phy_iovar_rxcore_enable(pi, int_val, bool_val, ret_int_ptr, FALSE);
		break;

	case IOV_SVAL(IOV_NPHY_PAPDCALTYPE):
		if (ISNPHY(pi))
			pi_nphy->nphy_papd_cal_type = (int8) int_val;
		break;

	case IOV_GVAL(IOV_NPHY_PAPDCAL):
		if (ISNPHY(pi))
			pi_nphy->nphy_force_papd_cal = TRUE;
		int_val = 0;
		bcopy(&int_val, a, vsize);
		break;

	case IOV_SVAL(IOV_NPHY_SKIPPAPD):
		if ((int_val != 0) && (int_val != 1)) {
			err = BCME_RANGE;
			break;
		}
		if (ISNPHY(pi))
			pi_nphy->nphy_papd_skip = (uint8)int_val;
		break;

	case IOV_GVAL(IOV_NPHY_PAPDCALINDEX):
		if (ISNPHY(pi)) {
			*ret_int_ptr = (pi_nphy->nphy_papd_cal_gain_index[0] << 8) |
				pi_nphy->nphy_papd_cal_gain_index[1];
		}
		break;

	case IOV_SVAL(IOV_NPHY_CALTXGAIN): {
		uint8 tx_pwr_ctrl_state;

		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);

		if (ISNPHY(pi)) {
			pi_nphy->nphy_cal_orig_pwr_idx[0] =
			        (uint8) ((phy_utils_read_phyreg(pi,
			                  NPHY_Core0TxPwrCtrlStatus) >> 8) & 0x7f);
			pi_nphy->nphy_cal_orig_pwr_idx[1] =
				(uint8) ((phy_utils_read_phyreg(pi,
			                  NPHY_Core1TxPwrCtrlStatus) >> 8) & 0x7f);
		}

		tx_pwr_ctrl_state = pi->nphy_txpwrctrl;
		wlc_phy_txpwrctrl_enable_nphy(pi, PHY_TPC_HW_OFF);

		wlc_phy_cal_txgainctrl_nphy(pi, int_val, TRUE);

		wlc_phy_txpwrctrl_enable_nphy(pi, tx_pwr_ctrl_state);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);

		break;
	}

	case IOV_GVAL(IOV_NPHY_VCOCAL):
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		wlc_phy_radio205x_vcocal_nphy(pi);
		wlapi_enable_mac(pi->sh->physhim);
		*ret_int_ptr = 0;
		break;

	case IOV_GVAL(IOV_NPHY_TBLDUMP_MINIDX):
		*ret_int_ptr = (int32)pi->nphy_tbldump_minidx;
		break;

	case IOV_SVAL(IOV_NPHY_TBLDUMP_MINIDX):
		pi->nphy_tbldump_minidx = (int8) int_val;
		break;

	case IOV_GVAL(IOV_NPHY_TBLDUMP_MAXIDX):
		*ret_int_ptr = (int32)pi->nphy_tbldump_maxidx;
		break;

	case IOV_SVAL(IOV_NPHY_TBLDUMP_MAXIDX):
		pi->nphy_tbldump_maxidx = (int8) int_val;
		break;

	case IOV_SVAL(IOV_NPHY_PHYREG_SKIPDUMP):
		if (pi->nphy_phyreg_skipcnt < 127) {
			pi->nphy_phyreg_skipaddr[pi->nphy_phyreg_skipcnt++] = (uint) int_val;
		}
		break;

	case IOV_GVAL(IOV_NPHY_PHYREG_SKIPDUMP):
		*ret_int_ptr = (pi->nphy_phyreg_skipcnt > 0) ?
			(int32) pi->nphy_phyreg_skipaddr[pi->nphy_phyreg_skipcnt-1] : 0;
		break;

	case IOV_SVAL(IOV_NPHY_PHYREG_SKIPCNT):
		pi->nphy_phyreg_skipcnt = (int8) int_val;
		break;

	case IOV_GVAL(IOV_NPHY_PHYREG_SKIPCNT):
		*ret_int_ptr = (int32)pi->nphy_phyreg_skipcnt;
		break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#endif /* NCONF */

	default:
		err = BCME_UNSUPPORTED;
	}

	return err;
}

#ifdef WLC_TXCAL
static int
wlc_phy_iovar_adjusted_tssi(phy_info_t *pi, int32 *ret_int_ptr, uint8 int_val)
{
	if (ISACPHY(pi)) {
		if (int_val >= PHY_CORE_MAX)
			*ret_int_ptr = 0;
		else
			*ret_int_ptr = wlc_phy_adjusted_tssi_acphy(pi, int_val);
	}
	return BCME_OK;
}
#if defined(BCMINTERNAL) || defined(WLTEST)
static int
wlc_phy_txcal_copy_get_gainsweep_meas(phy_info_t * pi,
		wl_txcal_meas_ncore_t * txcal_meas) {
	uint16 i, j;
	wl_txcal_meas_percore_t * per_core;
	txcal_meas->valid_cnt = pi->txcali->txcal_meas->valid_cnt;
	txcal_meas->num_core = PHY_CORE_MAX;
	txcal_meas->version = TXCAL_IOVAR_VERSION;

	/* fillup per core info */
	per_core = txcal_meas->txcal_percore;
	for (i = 0; i < PHY_CORE_MAX; i++) {
		for (j = 0; j < pi->txcali->txcal_meas->valid_cnt; j++) {
			per_core->tssi[j] = pi->txcali->txcal_meas->tssi[i][j];
			per_core->pwr[j] = pi->txcali->txcal_meas->pwr[i][j];
		}
		per_core++;
	}
	return BCME_OK;
}

static int
wlc_phy_txcal_copy_set_pwr_tssi_tbl_gentbl(wl_txcal_power_tssi_t *pi_txcal_pwr_tssi,
		wl_txcal_power_tssi_ncore_t *txcal_tssi) {
	uint16 i;
	wl_txcal_power_tssi_percore_t * per_core;
	per_core = txcal_tssi->tssi_percore;
	pi_txcal_pwr_tssi->gen_tbl = txcal_tssi->gen_tbl;
	pi_txcal_pwr_tssi->channel = txcal_tssi->channel;
	for (i = 0; i < PHY_CORE_MAX; i++) {
		pi_txcal_pwr_tssi->pwr_start[i] = per_core->pwr_start;
		pi_txcal_pwr_tssi->num_entries[i] = per_core->num_entries;
		per_core++;
	}
	pi_txcal_pwr_tssi->set_core = txcal_tssi->set_core;
	return BCME_OK;
}

static int
wlc_phy_txcal_copy_set_pwr_tssi_tbl_storetbl(wl_txcal_power_tssi_t *pi_txcal_pwr_tssi,
		wl_txcal_power_tssi_ncore_t *txcal_tssi) {
	uint16 i;
	wl_txcal_power_tssi_percore_t * per_core;
	per_core = txcal_tssi->tssi_percore;
	for (i = 0; i < PHY_CORE_MAX; i++) {
		pi_txcal_pwr_tssi->set_core = txcal_tssi->set_core;
		pi_txcal_pwr_tssi->channel = txcal_tssi->channel;
		pi_txcal_pwr_tssi->gen_tbl = txcal_tssi->gen_tbl;
		pi_txcal_pwr_tssi->tempsense[i] = per_core->tempsense;
		pi_txcal_pwr_tssi->pwr_start[i] = per_core->pwr_start;
		pi_txcal_pwr_tssi->pwr_start_idx[i] = per_core->pwr_start_idx;
		pi_txcal_pwr_tssi->num_entries[i] = per_core->num_entries;
		pi_txcal_pwr_tssi->tempsense[i] = per_core->tempsense;
		bcopy(per_core->tssi, pi_txcal_pwr_tssi->tssi[i],
			MAX_NUM_PWR_STEP * sizeof(per_core->tssi[0]));
		per_core++;
	}
	return BCME_OK;
}

static int
wlc_phy_txcal_copy_get_pwr_tssi_tbl(wl_txcal_power_tssi_ncore_t * txcal_tssi,
		wl_txcal_power_tssi_t * in_buf) {
	uint16 i;
	wl_txcal_power_tssi_percore_t * per_core;
	txcal_tssi->set_core = in_buf->set_core;
	txcal_tssi->channel  = in_buf->channel;
	txcal_tssi->gen_tbl = in_buf->gen_tbl;
	txcal_tssi->num_core = PHY_CORE_MAX;
	txcal_tssi->version = TXCAL_IOVAR_VERSION;

	/* per core info */
	per_core = txcal_tssi->tssi_percore;
	for (i = 0; i < PHY_CORE_MAX; i++) {
		per_core->tempsense = in_buf->tempsense[i];
		per_core->pwr_start = in_buf->pwr_start[i];
		per_core->pwr_start_idx = in_buf->pwr_start_idx[i];
		per_core->num_entries = in_buf->num_entries[i];
		bcopy(in_buf->tssi[i], per_core->tssi,
			MAX_NUM_PWR_STEP * sizeof(in_buf->tssi[0][0]));
		per_core++;
	}
	return BCME_OK;
}

static int
wlc_phy_txcal_generate_pwr_tssi_tbl(phy_info_t *pi)
{
	wl_txcal_power_tssi_t *pi_txcal_pwr_tssi = pi->txcali->txcal_pwr_tssi;
	wl_txcal_meas_t *pi_txcal_meas = pi->txcali->txcal_meas;
	uint8 core = pi_txcal_pwr_tssi->set_core;
	int16 pwr_start = pi_txcal_pwr_tssi->pwr_start[core];
	uint8 num_entries = pi_txcal_pwr_tssi->num_entries[core];
	int32 pwr_val = pwr_start;
	int8 i = 0;
	uint8 j, k;
	int16 *pwr = pi_txcal_meas->pwr[core];
	uint16 *tssi = pi_txcal_meas->tssi[core];
	uint8 valid_cnt = pi_txcal_meas->valid_cnt;
	int32 tssi_val;
	uint8 flag;
	int32 num, den;
	for (j = 0; j < num_entries; j++) {
		flag = 0;
		for (k = 0; k < valid_cnt; k++) {
			if (pwr[k] <= pwr_val) {
				flag = 1;
				i = k-1;
				if (i < 0)
					i = 0;
				break;
			}
		}
		if (flag == 0) {
			i = valid_cnt - 2;
			if (i < 0) {
				return BCME_ERROR;
			}
		}
		/* Interpolate between i and i+1 to find tssi_val at known pwr */
		num = (pwr_val - pwr[i])*(tssi[i+1] - tssi[i]);
		den = pwr[i+1] - pwr[i];
		/* Limiting tssi_val if trying to find tssi for power higher than */
		/* that measured during gain sweep */
		if (den == 0 || i == 0)
			tssi_val = tssi[i];
		else {
			tssi_val = tssi[i] + (num + (num > 0 ? ABS(den) :
			-ABS(den))/2)/den;
		}
		/* Keep TSSI val in 8 bits to increase accuracy when */
		/* interpolated to 128 entries estpwrlut */
		if (ISLCN20PHY(pi)) {
			tssi_val = (tssi_val > 0 ? tssi_val+2 :	tssi_val-2)>>1;
		} else {
			tssi_val = (tssi_val > 0 ? tssi_val+2 :	tssi_val-2)>>2;
		}

		/* Limiting tssi_val in range */
		if (tssi_val < 0)
			tssi_val = 0;
		else if (tssi_val > 255)
			tssi_val = 255;

		pi->txcali->txcal_pwr_tssi->tssi[core][j] = (uint8) tssi_val;
		pwr_val = pwr_val + 8;
	}
	pi->txcali->txcal_pwr_tssi->gen_tbl = 0;
	return BCME_OK;
}

static int
wlc_phy_txcal_get_pwr_tssi_tbl(phy_info_t *pi, uint8 channel)
{
	/* Go over the list for inquiry of the pwr tssi tbl */
	txcal_pwr_tssi_lut_t *LUT_pt;
	txcal_root_pwr_tssi_t *pi_txcal_root_pwr_tssi_tbl = pi->txcali->txcal_root_pwr_tssi_tbl;
	if (channel < 15) {
		LUT_pt = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_2G;
	} else if ((channel & 0x06) == 0x06) {
		LUT_pt = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_5G40;
	} else if ((channel & 0x0A) == 0x0A) {
		LUT_pt = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_5G80;
	} else {
		LUT_pt = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_5G20;
	}
	while (LUT_pt->next_chan != 0) {
		if (LUT_pt->txcal_pwr_tssi->channel == channel) {
			bcopy(LUT_pt->txcal_pwr_tssi, pi->txcali->txcal_pwr_tssi,
			        sizeof(wl_txcal_power_tssi_t));
			break;
		} else {
			LUT_pt = LUT_pt->next_chan;
		}
	}
	if (LUT_pt->txcal_pwr_tssi->channel == channel) {
		bcopy(LUT_pt->txcal_pwr_tssi, pi->txcali->txcal_pwr_tssi,
			sizeof(wl_txcal_power_tssi_t));
	} else {
		memset(pi->txcali->txcal_pwr_tssi, 0, sizeof(wl_txcal_power_tssi_t));
	}
	return BCME_OK;
}

static int
wlc_phy_txcal_alloc_pwr_tssi_lut(phy_info_t *pi, txcal_pwr_tssi_lut_t** LUT)
{
	*LUT = phy_malloc_fatal(pi, sizeof(**LUT));
	(*LUT)->txcal_pwr_tssi = phy_malloc_fatal(pi, sizeof(*(*LUT)->txcal_pwr_tssi));
	return BCME_OK;
}

static void
wlc_phy_txcal_copy_rsdb_pwr_tssi_tbl(phy_info_t *pi)
{
	int tssi_lp_cnt;
	pi->txcali->txcal_pwr_tssi->tempsense[PHY_RSBD_PI_IDX_CORE0] =
			pi->txcali->txcal_pwr_tssi->tempsense[PHY_RSBD_PI_IDX_CORE1];
	pi->txcali->txcal_pwr_tssi->pwr_start[PHY_RSBD_PI_IDX_CORE0] =
			pi->txcali->txcal_pwr_tssi->pwr_start[PHY_RSBD_PI_IDX_CORE1];
	pi->txcali->txcal_pwr_tssi->pwr_start_idx[PHY_RSBD_PI_IDX_CORE0] =
			pi->txcali->txcal_pwr_tssi->pwr_start_idx[PHY_RSBD_PI_IDX_CORE1];
	pi->txcali->txcal_pwr_tssi->num_entries[PHY_RSBD_PI_IDX_CORE0] =
			pi->txcali->txcal_pwr_tssi->num_entries[PHY_RSBD_PI_IDX_CORE1];
	for (tssi_lp_cnt = 0; tssi_lp_cnt < MAX_NUM_PWR_STEP; tssi_lp_cnt++)
		pi->txcali->txcal_pwr_tssi->tssi[PHY_RSBD_PI_IDX_CORE0][tssi_lp_cnt] =
			pi->txcali->txcal_pwr_tssi->tssi[PHY_RSBD_PI_IDX_CORE1][tssi_lp_cnt];
	return;
}

static int
wlc_phy_txcal_store_pwr_tssi_tbl(phy_info_t *pi)
{
	txcal_pwr_tssi_lut_t *LUT_pt;
	txcal_pwr_tssi_lut_t *LUT_tmp;
	txcal_pwr_tssi_lut_t *LUT_root;
	txcal_root_pwr_tssi_t *pi_txcal_root_pwr_tssi_tbl = pi->txcali->txcal_root_pwr_tssi_tbl;
	if (pi->txcali->txcal_pwr_tssi->channel < 15) {
		LUT_root = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_2G;
	} else if ((pi->txcali->txcal_pwr_tssi->channel & 0x06) == 0x06) {
		LUT_root = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_5G40;
	} else if ((pi->txcali->txcal_pwr_tssi->channel & 0x0A) == 0x0A) {
		LUT_root = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_5G80;
	} else {
		LUT_root = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_5G20;
	}
	if (LUT_root->txcal_pwr_tssi->channel == 0) {
		/* First valid entry of the linked list */
		bcopy(pi->txcali->txcal_pwr_tssi, LUT_root->txcal_pwr_tssi,
		      sizeof(wl_txcal_power_tssi_t));
		LUT_root->next_chan = 0;
	} else {
		LUT_pt = LUT_root;
		if (LUT_pt->txcal_pwr_tssi->channel > pi->txcali->txcal_pwr_tssi->channel) {
			/* Add entry at the beginning of the linked list */
			wlc_phy_txcal_alloc_pwr_tssi_lut(pi, &LUT_tmp);
			bcopy(LUT_root->txcal_pwr_tssi, LUT_tmp->txcal_pwr_tssi,
			        sizeof(wl_txcal_power_tssi_t));
			LUT_tmp->next_chan = LUT_root->next_chan;
			bcopy(pi->txcali->txcal_pwr_tssi, LUT_root->txcal_pwr_tssi,
			        sizeof(wl_txcal_power_tssi_t));
			LUT_root->next_chan = LUT_tmp;
			pi->txcali->txcal_pwr_tssi_tbl_count++;
			return BCME_OK;
		}
		while (LUT_pt->next_chan != 0) {
			/* Go over all the entries in the list */
			if (LUT_pt->txcal_pwr_tssi->channel == pi->txcali->txcal_pwr_tssi->channel)
				break;
			if ((LUT_pt->txcal_pwr_tssi->channel <
				pi->txcali->txcal_pwr_tssi->channel) &&
				(LUT_pt->next_chan->txcal_pwr_tssi->channel >
				 pi->txcali->txcal_pwr_tssi->channel))
				break;
			LUT_pt = LUT_pt->next_chan;
		}
		if (LUT_pt->txcal_pwr_tssi->channel == pi->txcali->txcal_pwr_tssi->channel) {
			/* Channel found, override */
			bcopy(pi->txcali->txcal_pwr_tssi, LUT_pt->txcal_pwr_tssi,
			        sizeof(wl_txcal_power_tssi_t));
		} else {
			if (pi->txcali->txcal_pwr_tssi_tbl_count > 31)
				return BCME_NOMEM;
			if (LUT_pt->next_chan == 0) {
				/* Add to the end of the linked list */
				wlc_phy_txcal_alloc_pwr_tssi_lut(pi, &LUT_pt->next_chan);
				LUT_pt = LUT_pt->next_chan;
				bcopy(pi->txcali->txcal_pwr_tssi, LUT_pt->txcal_pwr_tssi,
				        sizeof(wl_txcal_power_tssi_t));
				LUT_pt->next_chan = 0;
			} else {
				/* Insert into the linked list */
				wlc_phy_txcal_alloc_pwr_tssi_lut(pi, &LUT_tmp);
				bcopy(pi->txcali->txcal_pwr_tssi, LUT_tmp->txcal_pwr_tssi,
				        sizeof(wl_txcal_power_tssi_t));
				LUT_tmp->next_chan = LUT_pt->next_chan;
				LUT_pt->next_chan = LUT_tmp;
			}
			pi->txcali->txcal_pwr_tssi_tbl_count++;
		}
	}
	return BCME_OK;
}
static int
wlc_phy_txcal_gainsweep(phy_info_t *pi, wl_txcal_params_t *txcal_params)
{
	int16 gidx;
	uint32 gidx_core[WLC_TXCORE_MAX];
	uint16 adj_tssi = 0;
	bool more_steps;
	uint8 cnt = 0;
	uint16 save_TxPwrCtrlCmd = 0;
	uint8 tx_pwr_ctrl_state = pi->txpwrctrl;
	uint8 core;
	wl_txcal_meas_t *pi_txcal_meas = pi->txcali->txcal_meas;
	bool suspend = FALSE;
	//suspend mac before accessing phy registers
	wlc_phy_conditional_suspend(pi, &suspend);

	if (ISACPHY(pi)) {
		save_TxPwrCtrlCmd = phy_utils_read_phyreg(pi,
			ACPHY_TxPwrCtrlCmd(pi->pubpi.phy_rev));
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	} else if (ISLCN20PHY(pi)) {
		save_TxPwrCtrlCmd = phy_utils_read_phyreg(pi, LCN20PHY_TxPwrCtrlCmd);
#endif /* #if ((defined(LCN20CONF) && (LCN20CONF != 0))) */
	}

	gidx = txcal_params->gidx_start;

	do {
		if (ISACPHY(pi)) {
			/* Set txpwrindex of all cores to be same */
			FOREACH_ACTV_CORE(pi, pi->sh->hw_phyrxchain, core) {
			   gidx_core[core] = (uint32) gidx;
			}
			wlc_phy_iovar_txpwrindex_set(pi, &gidx_core);
			/* Disable HWTXPwrCtrl */
			wlc_phy_txpwrctrl_enable_acphy(pi, PHY_TPC_HW_OFF);
#if (defined(LCN20CONF) && (LCN20CONF != 0))
		} else if (ISLCN20PHY(pi)) {
			/* Set txpwrindex */
			wlc_phy_iovar_txpwrindex_set(pi, &gidx);
			/* Setting index, disables both Txpwrctrl and HWTXPwrCtrl */
			/* Enabling only Txpwrctrl and leaving HWTxPwrctrl disabled */
			phy_utils_write_phyreg(pi, LCN20PHY_TxPwrCtrlCmd,
				(save_TxPwrCtrlCmd & (~LCN20PHY_TxPwrCtrlCmd_hwtxPwrCtrl_en_MASK)));
#endif /* #if ((defined(LCN20CONF) && (LCN20CONF != 0))) */
		}
		wlc_phy_conditional_resume(pi, &suspend);
		wlapi_bmac_pkteng_txcal(pi->sh->physhim, 0, 0, &(txcal_params->pkteng));
		wlc_phy_conditional_suspend(pi, &suspend);

		FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
			if (ISACPHY(pi)) {
				adj_tssi = wlc_phy_adjusted_tssi_acphy(pi, core);
#if (defined(LCN20CONF) && (LCN20CONF != 0))
			} else if (ISLCN20PHY(pi)) {
				adj_tssi = wlc_lcn20phy_adjusted_tssi(pi);
#endif /* #if ((defined(LCN20CONF) && (LCN20CONF != 0))) */
			}
			pi_txcal_meas->tssi[core][cnt] = adj_tssi;
		}
		cnt++;
		wlc_phy_conditional_resume(pi, &suspend);
		wlapi_bmac_pkteng_txcal(pi->sh->physhim, 0, 0, 0);
		wlapi_bmac_service_txstatus(pi->sh->physhim);
		wlc_phy_conditional_suspend(pi, &suspend);

		gidx = gidx + txcal_params->gidx_step;
		if ((gidx > 127) || (gidx < 0))
			more_steps = FALSE;
		else
			more_steps = (txcal_params->gidx_step > 0) ?
				(gidx <= txcal_params->gidx_stop) :
				(gidx >= txcal_params->gidx_stop);
	} while (more_steps);

	pi_txcal_meas->valid_cnt = cnt;

	if (ISACPHY(pi)) {
		phy_utils_write_phyreg(pi, ACPHY_TxPwrCtrlCmd(pi->pubpi.phy_rev),
			save_TxPwrCtrlCmd);
		wlc_phy_txpwrctrl_enable_acphy(pi, tx_pwr_ctrl_state);
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	} else if (ISLCN20PHY(pi)) {
		phy_utils_write_phyreg(pi, LCN20PHY_TxPwrCtrlCmd, save_TxPwrCtrlCmd);
#endif /* #if ((defined(LCN20CONF) && (LCN20CONF != 0))) */
	}
	wlc_phy_conditional_resume(pi, &suspend);
	return BCME_OK;
}

#endif	/* BCMINTERNAL || WLTEST */
#endif /* WLC_TXCAL */

#ifdef WLC_TXCAL
int
wlc_phy_txcal_generate_estpwr_lut(wl_txcal_power_tssi_t *txcal_pwr_tssi, uint16 *estpwr, uint8 core)
{
	uint8 tssi_val_idx;
	uint8 tssi_val;
	int8 i = 0, k;
	int16 pwr_start = txcal_pwr_tssi->pwr_start[core];
	uint8 num_entries = txcal_pwr_tssi->num_entries[core];
	uint8 *tssi = txcal_pwr_tssi->tssi[core];
	int16 pwr_i, pwr_i_1;
	int32 est_pwr_calc;
	int32 num, den;
	for (tssi_val_idx = 0; tssi_val_idx < 128; tssi_val_idx++) {
		tssi_val = tssi_val_idx * 2;
		for (k = num_entries-1; k >= 0; k--) {
			if (tssi[k] >= tssi_val) {
				i = k+1;
				if (i >= num_entries)
					i = num_entries-1;
				break;
			}
		}
		pwr_i = pwr_start + 8*i;
		pwr_i_1 = pwr_start + 8*(i-1);
		if (tssi[i-1] - tssi[i] == 0) {
			est_pwr_calc = pwr_i;
		} else {
			num = (tssi_val - tssi[i])*(pwr_i_1 - pwr_i);
			den = tssi[i-1] - tssi[i];
			est_pwr_calc = pwr_i + (num + (num > 0 ? ABS(den) :
				-ABS(den))/2)/den;
		}
		/* Convert to qdBm for writing to LUT */
		est_pwr_calc = (est_pwr_calc + 1) >> 1;
		if (est_pwr_calc > 126)
			est_pwr_calc = 126;
		else if (est_pwr_calc < -128)
			est_pwr_calc = -128;

		estpwr[tssi_val_idx] = (uint16) (est_pwr_calc & 0xFF);
	}
	return BCME_OK;
}

int
wlc_phy_txcal_apply_pwr_tssi_tbl(phy_info_t *pi, wl_txcal_power_tssi_t *txcal_pwr_tssi)
{
	uint32 tbl_len = 128;
	uint32 tbl_offset = 0;
	uint8 core;
	uint8 tx_pwr_ctrl_state;
	uint8 tssi_idx;
#if ACCONF || ACCONF2
	uint16 estpwr_acphy[128];
#endif
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	uint32 estpwr_lcn20[128];
	phytbl_info_t tab;
#endif
	if (ISACPHY(pi)) {
		tx_pwr_ctrl_state =  pi->txpwrctrl;
		bool suspend = FALSE;
		wlc_phy_conditional_suspend(pi, &suspend);
		wlc_phy_txpwrctrl_enable_acphy(pi, PHY_TPC_HW_OFF);
		wlc_phy_conditional_resume(pi, &suspend);
		FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
			if (txcal_pwr_tssi->num_entries[core] == 0) {
				/* sanity check */
				wlc_phy_txcal_apply_pa_params(pi);
			} else {
				wlc_phy_txcal_generate_estpwr_lut(txcal_pwr_tssi,
					estpwr_acphy, core);
				/* Program the upper 8-bits for CCK for 43012 */
				if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
					for (tssi_idx = 0; tssi_idx < 128; tssi_idx++) {
						estpwr_acphy[tssi_idx] = (uint16)
							(((estpwr_acphy[tssi_idx] & 0x00FF) << 8) |
							(estpwr_acphy[tssi_idx] & 0x00FF));
					}
				}
				wlc_phy_conditional_suspend(pi, &suspend);
				wlc_phy_table_write_acphy(pi,
					ACPHY_TBL_ID_ESTPWRLUTS(core),
					tbl_len, tbl_offset, 16, estpwr_acphy);
				wlc_phy_conditional_resume(pi, &suspend);
			}
		}
		wlc_phy_conditional_suspend(pi, &suspend);
		wlc_phy_txpwrctrl_enable_acphy(pi, tx_pwr_ctrl_state);
		wlc_phy_conditional_resume(pi, &suspend);
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	} else if (ISLCN20PHY(pi)) {
		core = 0;

		if (txcal_pwr_tssi->num_entries[core] == 0) {
			/* sanity check */
			wlc_phy_txcal_apply_pa_params(pi);
		} else {
			wlc_phy_txcal_generate_estpwr_lut_lcn20phy(txcal_pwr_tssi,
				estpwr_lcn20, core);
			tab.tbl_offset = 0;
			tab.tbl_id = LCN20PHY_TBL_ID_TXPWRCTL;
			tab.tbl_width = 32;
			tab.tbl_ptr = estpwr_lcn20;
			tab.tbl_len = 128;
			wlc_lcn20phy_write_table(pi,  &tab);
		}
#endif /* #if ((defined(LCN20CONF) && (LCN20CONF != 0))) */
	}
	return BCME_OK;
}

int
wlc_phy_txcal_apply_pa_params(phy_info_t *pi)
{
	uint32 tbl_len = 128;
	uint32 tbl_offset = 0;
	uint8 core;
	uint8 tx_pwr_ctrl_state;
	int16 a1[PHY_CORE_MAX];
	int16 b0[PHY_CORE_MAX];
	int16 b1[PHY_CORE_MAX];
	uint8 tssi_val;

	if (ISACPHY(pi)) {
		uint16 estpwr[128];
		tx_pwr_ctrl_state =  pi->txpwrctrl;
		bool suspend = FALSE;
		wlc_phy_conditional_suspend(pi, &suspend);

		wlc_phy_txpwrctrl_enable_acphy(pi, PHY_TPC_HW_OFF);
		wlc_phy_get_paparams_for_band_acphy(pi, a1, b0, b1);
		FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
			for (tssi_val = 0; tssi_val < 128; tssi_val++) {
				estpwr[tssi_val] = wlc_phy_tssi2dbm_acphy(pi,
				        tssi_val, a1[core], b0[core], b1[core]);
				/* Program the upper 8-bits for CCK for 43012 */
				if (ACMAJORREV_36(pi->pubpi->phy_rev))
					estpwr[tssi_val] = (int16) (estpwr[tssi_val]<< 8 |
						estpwr[tssi_val]);
			}
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_ESTPWRLUTS(core),
				tbl_len, tbl_offset, 16, estpwr);
		}
		wlc_phy_txpwrctrl_enable_acphy(pi, tx_pwr_ctrl_state);
		pi->txcali->txcal_status = 0;
		wlc_phy_conditional_resume(pi, &suspend);
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	} else if (ISLCN20PHY(pi)) {
		uint32 estpwr[128];
		int32 x1 = 0, y0 = 0, y1 = 0;
		phytbl_info_t tab;

		wlc_phy_get_paparams_for_band(pi, &x1, &y0, &y1);
		for (tssi_val = 0; tssi_val < 128; tssi_val++) {
			estpwr[tssi_val] = wlc_lcn20phy_tssi2dbm(tssi_val, x1, y0, y1);
		}
		tab.tbl_offset = tbl_offset;
		tab.tbl_id = LCN20PHY_TBL_ID_TXPWRCTL;
		tab.tbl_width = 32;
		tab.tbl_ptr = estpwr;
		tab.tbl_len = tbl_len;
		wlc_lcn20phy_write_table(pi,  &tab);
#endif /* #if ((defined(LCN20CONF) && (LCN20CONF != 0)))  */
	}
	return BCME_OK;
}
#endif /* WLC_TXCAL */

/* register iovar table to the system */
#include <phy_api.h>

int
phy_iovars_wrapper_sample_collect(phy_info_t *pi, uint32 actionid, uint16 type, void *p, uint plen,
	void *a, int alen, int vsize)
{
#ifdef SAMPLE_COLLECT
	return phy_iovars_sample_collect(pi, actionid, -1, p, plen, a, alen, vsize);
#endif /* SAMPLE_COLLECT */

	return BCME_UNSUPPORTED;
}

static int
phy_doiovar(void *ctx, uint32 actionid, void *p, uint plen, void *a, uint alen, uint vsize,
	struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	bool bool_val = FALSE;
	int32 int_val = 0;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	/* bool conversion to avoid duplication below */
	bool_val = int_val != 0;

	BCM_REFERENCE(*ret_int_ptr);
	BCM_REFERENCE(bool_val);

	switch (actionid) {
#if defined(BCMDBG) || defined(WLTEST)
	case IOV_GVAL(IOV_FAST_TIMER):
		*ret_int_ptr = (int32)pi->sh->fast_timer;
		break;

	case IOV_SVAL(IOV_FAST_TIMER):
		pi->sh->fast_timer = (uint32)int_val;
		break;

	case IOV_GVAL(IOV_SLOW_TIMER):
		*ret_int_ptr = (int32)pi->sh->slow_timer;
		break;

	case IOV_SVAL(IOV_SLOW_TIMER):
		pi->sh->slow_timer = (uint32)int_val;
		break;

#endif /* BCMDBG || WLTEST */
#if defined(BCMDBG) || defined(WLTEST) || defined(PHYCAL_CHNG_CS)
	case IOV_GVAL(IOV_GLACIAL_TIMER):
		*ret_int_ptr = (int32)pi->sh->glacial_timer;
		break;

	case IOV_SVAL(IOV_GLACIAL_TIMER):
		pi->sh->glacial_timer = (uint32)int_val;
		break;
#endif
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(MACOSX) || defined(DBG_PHY_IOV)
	case IOV_GVAL(IOV_PHY_WATCHDOG):
		*ret_int_ptr = (int32)pi->phywatchdog_override;
		break;

	case IOV_SVAL(IOV_PHY_WATCHDOG):
		wlc_phy_watchdog_override(pi, bool_val);
		break;
#endif
	case IOV_GVAL(IOV_CAL_PERIOD):
	        *ret_int_ptr = (int32)pi->cal_period;
	        break;

	case IOV_SVAL(IOV_CAL_PERIOD):
	        pi->cal_period = (uint32)int_val;
	        break;
#if defined(BCMINTERNAL) || defined(WLTEST)
#ifdef BAND5G
	case IOV_GVAL(IOV_PHY_CGA_5G):
		/* Pass on existing channel based offset into wl */
		bcopy(pi->phy_cga_5g, a, 24*sizeof(int8));
		break;
#endif /* BAND5G */
	case IOV_GVAL(IOV_PHY_CGA_2G):
		/* Pass on existing channel based offset into wl */
		bcopy(pi->phy_cga_2g, a, 14*sizeof(int8));
		break;

	case IOV_GVAL(IOV_PHYHAL_MSG):
		*ret_int_ptr = (int32)phyhal_msg_level;
		break;

	case IOV_SVAL(IOV_PHYHAL_MSG):
		phyhal_msg_level = (uint32)int_val;
		break;

	case IOV_SVAL(IOV_PHY_FIXED_NOISE):
		pi->phy_fixed_noise = bool_val;
		break;

	case IOV_GVAL(IOV_PHY_FIXED_NOISE):
		int_val = (int32)pi->phy_fixed_noise;
		bcopy(&int_val, a, vsize);
		break;

	case IOV_GVAL(IOV_PHYNOISE_POLL):
		*ret_int_ptr = (int32)pi->phynoise_polling;
		break;

	case IOV_SVAL(IOV_PHYNOISE_POLL):
		pi->phynoise_polling = bool_val;
		break;

	case IOV_GVAL(IOV_CARRIER_SUPPRESS):
		err = BCME_UNSUPPORTED;
		*ret_int_ptr = (pi->carrier_suppr_disable == 0);
		break;

	case IOV_SVAL(IOV_CARRIER_SUPPRESS):
	{
		initfn_t carr_suppr_fn = pi->pi_fptr->carrsuppr;
		if (carr_suppr_fn) {
			pi->carrier_suppr_disable = bool_val;
			if (pi->carrier_suppr_disable) {
				(*carr_suppr_fn)(pi);
			}
			PHY_INFORM(("Carrier Suppress Called\n"));
		} else
			err = BCME_UNSUPPORTED;
		break;
	}

#ifdef BAND5G
	case IOV_GVAL(IOV_PHY_SUBBAND5GVER):
		/* Retrieve 5G subband version */
		int_val = (uint8)(pi->sromi->subband5Gver);
		bcopy(&int_val, a, vsize);
		break;
#endif /* BAND5G */
	case IOV_GVAL(IOV_PHY_TXRX_CHAIN):
		wlc_phy_iovar_txrx_chain(pi, int_val, ret_int_ptr, FALSE);
		break;

	case IOV_SVAL(IOV_PHY_TXRX_CHAIN):
		err = wlc_phy_iovar_txrx_chain(pi, int_val, ret_int_ptr, TRUE);
		break;

	case IOV_GVAL(IOV_PHY_BPHY_EVM):
		*ret_int_ptr = pi->phy_bphy_evm;
		break;

	case IOV_SVAL(IOV_PHY_BPHY_EVM):
		wlc_phy_iovar_bphy_testpattern(pi, NPHY_TESTPATTERN_BPHY_EVM, (bool) int_val);
		break;

	case IOV_GVAL(IOV_PHY_BPHY_RFCS):
		*ret_int_ptr = pi->phy_bphy_rfcs;
		break;

	case IOV_SVAL(IOV_PHY_BPHY_RFCS):
		wlc_phy_iovar_bphy_testpattern(pi, NPHY_TESTPATTERN_BPHY_RFCS, (bool) int_val);
		break;

	case IOV_GVAL(IOV_PHY_SCRAMINIT):
		*ret_int_ptr = pi->phy_scraminit;
		break;

	case IOV_SVAL(IOV_PHY_SCRAMINIT):
		wlc_phy_iovar_scraminit(pi, (uint8)int_val);
		break;

	case IOV_SVAL(IOV_PHY_RFSEQ):
		err = wlc_phy_iovar_force_rfseq(pi, (uint8)int_val);
		break;

	case IOV_GVAL(IOV_PHY_TX_TONE_HZ):
	case IOV_GVAL(IOV_PHY_TX_TONE_SYMM):
		*ret_int_ptr = pi->phy_tx_tone_freq;
		break;

	case IOV_SVAL(IOV_PHY_TX_TONE_STOP):
		wlc_phy_iovar_tx_tone_stop(pi);
		break;

	case IOV_SVAL(IOV_PHY_TX_TONE_HZ):
		wlc_phy_iovar_tx_tone_hz(pi, (int32)int_val);
		break;

	case IOV_SVAL(IOV_PHY_TX_TONE_SYMM):
		err = wlc_phy_iovar_tx_tone_symm(pi, (int32)int_val);
		break;

	case IOV_GVAL(IOV_PHY_TEST_TSSI):
		*((uint*)a) = wlc_phy_iovar_test_tssi(pi, (uint8)int_val, 0);
		break;

	case IOV_GVAL(IOV_PHY_TEST_TSSI_OFFS):
		*((uint*)a) = wlc_phy_iovar_test_tssi(pi, (uint8)int_val, 12);
		break;

	case IOV_GVAL(IOV_PHY_TEST_IDLETSSI):
		*((uint*)a) = wlc_phy_iovar_test_idletssi(pi, (uint8)int_val);
		break;

	case IOV_SVAL(IOV_PHY_SETRPTBL):
		wlc_phy_iovar_setrptbl(pi);
		break;

	case IOV_SVAL(IOV_PHY_FORCEIMPBF):
		wlc_phy_iovar_forceimpbf(pi);
		break;

	case IOV_SVAL(IOV_PHY_FORCESTEER):
		wlc_phy_iovar_forcesteer(pi, (uint8)int_val);
		break;
#ifdef BAND5G
	case IOV_SVAL(IOV_PHY_5G_PWRGAIN):
		pi->phy_5g_pwrgain = bool_val;
		break;

	case IOV_GVAL(IOV_PHY_5G_PWRGAIN):
		*ret_int_ptr = (int32)pi->phy_5g_pwrgain;
		break;
#endif /* BAND5G */

	case IOV_SVAL(IOV_PHY_ENABLERXCORE):
		wlc_phy_iovar_rxcore_enable(pi, int_val, bool_val, ret_int_ptr, TRUE);
		break;

	case IOV_GVAL(IOV_PHY_ENABLERXCORE):
		wlc_phy_iovar_rxcore_enable(pi, int_val, bool_val, ret_int_ptr, FALSE);
		break;

	case IOV_GVAL(IOV_PHY_ACTIVECAL):
		*ret_int_ptr = (int32)((pi->cal_info->cal_phase_id !=
			MPHASE_CAL_STATE_IDLE)? 1 : 0);
		break;

	case IOV_SVAL(IOV_PHY_BBMULT):
		if (!pi->sh->clk) {
			err = BCME_NOCLK;
			break;
		}
		err = wlc_phy_iovar_bbmult_set(pi, p);
		break;

	case IOV_GVAL(IOV_PHY_BBMULT):
		if (!pi->sh->clk) {
			err = BCME_NOCLK;
			break;
		}
		wlc_phy_iovar_bbmult_get(pi, int_val, bool_val, ret_int_ptr);
		break;
#if (defined(BCMINTERNAL) || defined(WLTEST))
	case IOV_SVAL(IOV_TPC_AV):
		if (!pi->sh->clk) {
			err = BCME_NOCLK;
			break;
		}
		err = wlc_phy_iovar_avvmid_set(pi, p, AV);
		break;

	case IOV_GVAL(IOV_TPC_AV):
		if (!pi->sh->clk) {
			err = BCME_NOCLK;
			break;
		}
		wlc_phy_iovar_avvmid_get(pi, p, bool_val, ret_int_ptr, AV);
		break;

	case IOV_SVAL(IOV_TPC_VMID):
		if (!pi->sh->clk) {
			err = BCME_NOCLK;
			break;
		}
		err = wlc_phy_iovar_avvmid_set(pi, p, VMID);
		break;

	case IOV_GVAL(IOV_TPC_VMID):
		if (!pi->sh->clk) {
			err = BCME_NOCLK;
			break;
		}
		wlc_phy_iovar_avvmid_get(pi, p, bool_val, ret_int_ptr, VMID);
		break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

#if defined(WLC_LOWPOWER_BEACON_MODE)
	case IOV_GVAL(IOV_PHY_LOWPOWER_BEACON_MODE):
		if (ISLCN40PHY(pi)) {
			*ret_int_ptr = (pi->u.pi_lcn40phy)->lowpower_beacon_mode;
		}
		break;

	case IOV_SVAL(IOV_PHY_LOWPOWER_BEACON_MODE):
		wlc_phy_lowpower_beacon_mode(pih, int_val);
		break;
#endif /* WLC_LOWPOWER_BEACON_MODE */
#endif /* BCMINTERNAL || WLTEST */
#if defined(BCMINTERNAL) || defined(WLTEST)
	case IOV_GVAL(IOV_PKTENG_GAININDEX):
		if (!pi->sh->clk) {
			err = BCME_NOCLK;
			break;
		}
		err = wlc_phy_pkteng_get_gainindex(pi, ret_int_ptr);
		break;

#endif  /* BCMINTERNAL || WLTEST */
	case IOV_GVAL(IOV_PHY_RXGAININDEX):
		if (!pi->sh->clk) {
			err = BCME_NOCLK;
			break;
		}
		err = wlc_phy_get_rx_gainindex(pi, ret_int_ptr);
		break;

	case IOV_SVAL(IOV_PHY_RXGAININDEX):
		if (!pi->sh->clk) {
			err = BCME_NOCLK;
			break;
		}
		err = wlc_phy_set_rx_gainindex(pi, int_val);
		break;

	case IOV_GVAL(IOV_PHY_FORCECAL):
		err = wlc_phy_iovar_forcecal(pi, int_val, ret_int_ptr, vsize, FALSE);
		break;

	case IOV_SVAL(IOV_PHY_FORCECAL):
		err = wlc_phy_iovar_forcecal(pi, int_val, ret_int_ptr, vsize, TRUE);
		break;
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG)
#ifndef ATE_BUILD
	case IOV_GVAL(IOV_PHY_FORCECAL_OBT):
		err = wlc_phy_iovar_forcecal_obt(pi, int_val, ret_int_ptr, vsize, FALSE);
		break;

	case IOV_SVAL(IOV_PHY_FORCECAL_OBT):
		err = wlc_phy_iovar_forcecal_obt(pi, int_val, ret_int_ptr, vsize, FALSE);
		break;
#endif /* !ATE_BUILD */
#endif /* BCMINTERNAL || WLTEST || WFD_PHY_LL_DEBUG || ATE_BUILD */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(MACOSX)
	case IOV_SVAL(IOV_PHY_DEAF):
		wlc_phy_iovar_set_deaf(pi, int_val);
		break;
	case IOV_GVAL(IOV_PHY_DEAF):
		err = wlc_phy_iovar_get_deaf(pi, ret_int_ptr);
		break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(MACOSX) */
	case IOV_GVAL(IOV_PHYNOISE_SROM):
		if (ISHTPHY(pi) || ISACPHY(pi) || ISNPHY(pi)) {

			int8 noiselvl[PHY_CORE_MAX];
			uint8 core;
			uint32 pkd_noise = 0;
			if (!pi->sh->up) {
				err = BCME_NOTUP;
				break;
			}
			wlc_phy_get_SROMnoiselvl_phy(pi, noiselvl);
			for (core = PHYCORENUM(pi->pubpi->phy_corenum); core >= 1; core--) {
				pkd_noise = (pkd_noise << 8) | (uint8)(noiselvl[core-1]);
			}
			*ret_int_ptr = pkd_noise;
		} else {
			return BCME_UNSUPPORTED;        /* only htphy supported for now */
		}
		break;

	case IOV_GVAL(IOV_NUM_STREAM):
		if (ISNPHY(pi)) {
			int_val = 2;
		} else if (ISHTPHY(pi)) {
			int_val = 3;
		} else {
			int_val = -1;
		}
		bcopy(&int_val, a, vsize);
		break;

	case IOV_SVAL(IOV_MIN_TXPOWER):
		if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			pi->min_txpower_5g = (uint8)int_val;
		}
		pi->min_txpower = (uint8)int_val;
		break;

	case IOV_GVAL(IOV_MIN_TXPOWER):
		if (ACMAJORREV_4(pi->pubpi->phy_rev) && CHSPEC_IS5G(pi->radio_chanspec)) {
			int_val = pi->min_txpower_5g;
		} else {
			int_val = pi->min_txpower;
		}
		bcopy(&int_val, a, sizeof(int_val));
		break;

#if defined(BCMINTERNAL) || defined(MACOSX)
	case IOV_GVAL(IOV_PHYWREG_LIMIT):
		int_val = pi->phy_wreg_limit;
		bcopy(&int_val, a, vsize);
		break;

	case IOV_SVAL(IOV_PHYWREG_LIMIT):
		pi->phy_wreg_limit = (uint8)int_val;
		break;
#endif /* BCMINTERNAL */
	case IOV_GVAL(IOV_PHY_MUTED):
		*ret_int_ptr = PHY_MUTED(pi) ? 1 : 0;
		break;

	case IOV_GVAL(IOV_PHY_RXANTSEL):
		if (ISNPHY(pi))
			*ret_int_ptr = pi->nphy_enable_hw_antsel ? 1 : 0;
		break;

	case IOV_SVAL(IOV_PHY_RXANTSEL):
		if (ISNPHY(pi)) {
			pi->nphy_enable_hw_antsel = bool_val;
			/* make sure driver is up (so clks are on) before writing to PHY regs */
			if (pi->sh->up) {
				wlc_phy_init_hw_antsel(pi);
			}
		}
		break;
	case IOV_GVAL(IOV_PHY_DSSF):
	        err = wlc_phy_iovar_get_dssf(pi, ret_int_ptr);
		break;
	case IOV_SVAL(IOV_PHY_DSSF):
		err = wlc_phy_iovar_set_dssf(pi, int_val);
		break;
#ifdef WL_SARLIMIT
	case IOV_GVAL(IOV_PHY_TXPWR_CORE):
	{
		uint core;
		uint32 sar = 0;
		uint8 tmp;

		FOREACH_CORE(pi, core) {
			if (pi->txpwr_max_percore_override[core] != 0)
				tmp = pi->txpwr_max_percore_override[core];
			else
				tmp = pi->txpwr_max_percore[core];

			sar |= (tmp << (core * 8));
		}
		*ret_int_ptr = (int32)sar;
		break;
	}
#if defined(WLTEST) || defined(BCMDBG)
	case IOV_SVAL(IOV_PHY_TXPWR_CORE):
	{
		uint core;

		if (!pi->sh->up) {
			err = BCME_NOTUP;
			break;
		} else if (!ISHTPHY(pi)) {
			err = BCME_UNSUPPORTED;
			break;
		}

		FOREACH_CORE(pi, core) {
			pi->txpwr_max_percore_override[core] =
				(uint8)(((uint32)int_val) >> (core * 8) & 0x7f);
			if (pi->txpwr_max_percore_override[core] != 0) {
				pi->txpwr_max_percore_override[core] =
					MIN(WLC_TXPWR_MAX,
					MAX(pi->txpwr_max_percore_override[core],
					pi->min_txpower));
			}
		}
		phy_tpc_recalc_tgt(pi->tpci);
		break;
	}
#endif /* WLTEST || BCMDBG */
#endif /* WL_SARLIMIT */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || defined(BCMDBG)
		case IOV_SVAL(IOV_PAPD_EPS_OFFSET): {
			wlc_phy_iovar_set_papd_offset(pi, (int16)int_val);
			break;
		}

		case IOV_GVAL(IOV_PAPD_EPS_OFFSET): {
			*ret_int_ptr = wlc_phy_iovar_get_papd_offset(pi);
			break;
		}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) ||  defined(BCMDBG) */

		case IOV_GVAL(IOV_PHY_DUMP_PAGE): {
			*ret_int_ptr = pi->pdpi->page;
			break;
		}
		case IOV_SVAL(IOV_PHY_DUMP_PAGE): {
			if ((int_val < 0) || (int_val > 0xFF)) {
			        err = BCME_RANGE;
			        PHY_ERROR(("Value out of range\n"));
			        break;
			}
			*ret_int_ptr = pi->pdpi->page = (uint8)int_val;
			break;
		}
		case IOV_GVAL(IOV_PHY_DUMP_ENTRY): {
			*ret_int_ptr = pi->pdpi->entry;
			break;
		}
		case IOV_SVAL(IOV_PHY_DUMP_ENTRY): {
			if ((int_val < 0) || (int_val > 0xFFFF)) {
			        err = BCME_RANGE;
			        PHY_ERROR(("Value out of range\n"));
			        break;
			}
			*ret_int_ptr = pi->pdpi->entry = (uint16)int_val;
			break;
		}
#if defined(BCMINTERNAL) || defined(WLTEST)
	case IOV_SVAL(IOV_ACI_EXIT_CHECK_PERIOD):
		if (int_val == 0)
			err = BCME_RANGE;
		else
			pi->aci_exit_check_period = int_val;
		break;

	case IOV_GVAL(IOV_ACI_EXIT_CHECK_PERIOD):
		int_val = pi->aci_exit_check_period;
		bcopy(&int_val, a, vsize);
		break;

#endif /* BCMINTERNAL || WLTEST */
#if defined(WLTEST)
	case IOV_SVAL(IOV_PHY_GLITCHK):
		pi->tunings[0] = (uint16)int_val;
		break;

	case IOV_SVAL(IOV_PHY_NOISE_UP):
		pi->tunings[1] = (uint16)int_val;
		break;

	case IOV_SVAL(IOV_PHY_NOISE_DWN):
		pi->tunings[2] = (uint16)int_val;
		break;
#endif /* #if defined(WLTEST) */
#if defined(BCMINTERNAL) || defined(WLTEST)
	case IOV_GVAL(IOV_UNMOD_RSSI):
	{
		int32 input_power_db = 0;
		rxsigpwrfn_t rx_sig_pwr_fn = pi->pi_fptr->rxsigpwr;

		PHY_INFORM(("UNMOD RSSI Called\n"));

		if (!rx_sig_pwr_fn)
			return BCME_UNSUPPORTED;	/* lpphy and sslnphy support only for now */

		if (!pi->sh->up) {
			err = BCME_NOTUP;
			break;
		}

		input_power_db = (*rx_sig_pwr_fn)(pi, -1);

		*ret_int_ptr = input_power_db;
		break;
	}
#endif /* BCMINTERNAL || WLTEST */
#if (NCONF || LCN40CONF || LCN20CONF || ACCONF || ACCONF2) && defined(WLTEST)
	case IOV_GVAL(IOV_PHY_RSSI_GAIN_CAL_TEMP):
		if (ISLCN40PHY(pi)) {
			phy_info_lcn40phy_t *pi_lcn40 = pi->u.pi_lcn40phy;
			*ret_int_ptr = (int32)pi_lcn40->gain_cal_temp;
		} else if (CHIPID_4324X_EPA_FAMILY(pi)) {
			phy_info_nphy_t *pi_nphy = pi->u.pi_nphy;
			*ret_int_ptr = (int32)pi_nphy->gain_cal_temp;
		}
		break;

	case IOV_SVAL(IOV_PHY_RSSI_GAIN_CAL_TEMP):
		if (ISLCN40PHY(pi)) {
			phy_info_lcn40phy_t *pi_lcn40 = pi->u.pi_lcn40phy;
			pi_lcn40->gain_cal_temp = (int8)int_val;
		} else if (CHIPID_4324X_EPA_FAMILY(pi)) {
			phy_info_nphy_t *pi_nphy = pi->u.pi_nphy;
			pi_nphy->gain_cal_temp = (int8)int_val;
		}
		break;
#endif /* (NCONF || LCN40CONF || LCN20CONF || ACCONF || ACCONF2) && WLTEST */

#if defined(BCMDBG) || defined(WLTEST) || defined(MACOSX) || defined(ATE_BUILD) || \
	defined(BCMDBG_TEMPSENSE)
	case IOV_GVAL(IOV_PHY_TEMPSENSE):
		err = wlc_phy_iovar_tempsense_paldosense(pi, ret_int_ptr, 0);
		break;
#endif /* BCMDBG || WLTEST || MACOSX || ATE_BUILD || BCMDBG_TEMPSENSE */
#if defined(BCMINTERNAL) || defined(WLTEST)
	case IOV_GVAL(IOV_PHY_CAL_DISABLE):
		*ret_int_ptr = (int32)pi->disable_percal;
		break;

	case IOV_SVAL(IOV_PHY_CAL_DISABLE):
		pi->disable_percal = bool_val;
		break;

	case IOV_GVAL(IOV_PHY_IDLETSSI):
		wlc_phy_iovar_idletssi(pi, ret_int_ptr, TRUE);
		break;

	case IOV_SVAL(IOV_PHY_IDLETSSI):
		wlc_phy_iovar_idletssi(pi, ret_int_ptr, bool_val);
		break;

	case IOV_GVAL(IOV_PHY_VBATSENSE):
		wlc_phy_iovar_vbatsense(pi, ret_int_ptr);
		break;

	case IOV_GVAL(IOV_PHY_IDLETSSI_REG):
		if (!pi->sh->clk)
			err = BCME_NOCLK;
		else
			err = wlc_phy_iovar_idletssi_reg(pi, ret_int_ptr, int_val, FALSE);
		break;

	case IOV_SVAL(IOV_PHY_IDLETSSI_REG):
		if (!pi->sh->clk)
			err = BCME_NOCLK;
		else
			err = wlc_phy_iovar_idletssi_reg(pi, ret_int_ptr, int_val, TRUE);
		break;

	case IOV_GVAL(IOV_PHY_AVGTSSI_REG):
		if (!pi->sh->clk)
			err = BCME_NOCLK;
		else
			wlc_phy_iovar_avgtssi_reg(pi, ret_int_ptr);
		break;

	case IOV_SVAL(IOV_PHY_RESETCCA):
		if (ISNPHY(pi)) {
			wlc_phy_resetcca_nphy(pi);
		}
		else if (ISACPHY(pi)) {
			bool macSuspended;
			/* check if MAC already suspended */
			macSuspended = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);
			if (!macSuspended) {
				wlapi_suspend_mac_and_wait(pi->sh->physhim);
			}
			wlc_phy_resetcca_acphy(pi);
			if (!macSuspended)
				wlapi_enable_mac(pi->sh->physhim);
		}
		break;

	case IOV_SVAL(IOV_PHY_IQLOCALIDX):
		break;

	case IOV_GVAL(IOV_PHYCAL_TEMPDELTA):
		*ret_int_ptr = (int32)pi->phycal_tempdelta;
		break;

	case IOV_SVAL(IOV_PHYCAL_TEMPDELTA):
		if (int_val == -1)
			pi->phycal_tempdelta = pi->phycal_tempdelta_default;
		else
			pi->phycal_tempdelta = (uint8)int_val;
		break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
	case IOV_SVAL(IOV_PHY_SROM_TEMPSENSE):
	{
		pi->srom_rawtempsense = (int16)int_val;
		break;
	}

	case IOV_GVAL(IOV_PHY_SROM_TEMPSENSE):
	{
		*ret_int_ptr = pi->srom_rawtempsense;
		break;
	}
	case IOV_SVAL(IOV_PHY_GAIN_CAL_TEMP):
	{
		pi->srom_gain_cal_temp  = (int16)int_val;
		break;
	}
	case IOV_GVAL(IOV_PHY_GAIN_CAL_TEMP):
	{
		*ret_int_ptr = pi->srom_gain_cal_temp;
		break;
	}
#ifdef PHYMON
	case IOV_GVAL(IOV_PHYCAL_STATE): {
		if (alen < (int)sizeof(wl_phycal_state_t)) {
			err = BCME_BUFTOOSHORT;
			break;
		}
		if (ISNPHY(pi))
			err = wlc_phycal_state_nphy(pi, a, alen);
		else
			err = BCME_UNSUPPORTED;

		break;
	}
#endif /* PHYMON */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(AP) || defined(BCMDBG_PHYCAL)
	case IOV_GVAL(IOV_PHY_PERICAL):
		wlc_phy_iovar_perical_config(pi, int_val, ret_int_ptr, FALSE);
		break;

	case IOV_SVAL(IOV_PHY_PERICAL):
		wlc_phy_iovar_perical_config(pi, int_val, ret_int_ptr, TRUE);
		break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(AP) */

	case IOV_GVAL(IOV_PHY_PERICAL_DELAY):
		*ret_int_ptr = (int32)pi->phy_cal_delay;
		break;

	case IOV_SVAL(IOV_PHY_PERICAL_DELAY):
		if ((int_val >= PHY_PERICAL_DELAY_MIN) && (int_val <= PHY_PERICAL_DELAY_MAX))
			pi->phy_cal_delay = (uint16)int_val;
		else
			err = BCME_RANGE;
		break;

	case IOV_GVAL(IOV_PHY_PAPD_DEBUG):
		break;

	case IOV_GVAL(IOV_NOISE_MEASURE):
		int_val = 0;
		bcopy(&int_val, a, sizeof(int_val));
		break;
#if defined(WLC_TXCAL) || (defined(WLOLPC) && !defined(WLOLPC_DISABLED))
	case IOV_GVAL(IOV_OLPC_ANCHOR_2G):
		if (pi->olpci != NULL) {
			int_val = pi->olpci->olpc_anchor2g;
			bcopy(&int_val, a, sizeof(int_val));
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_SVAL(IOV_OLPC_ANCHOR_2G):
		if (pi->olpci != NULL) {
			pi->olpci->olpc_anchor2g = (int8) int_val;
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_GVAL(IOV_OLPC_ANCHOR_5G):
		if (pi->olpci != NULL) {
			int_val = pi->olpci->olpc_anchor5g;
			bcopy(&int_val, a, sizeof(int_val));
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_SVAL(IOV_OLPC_ANCHOR_5G):
		if (pi->olpci != NULL) {
			pi->olpci->olpc_anchor5g = (int8) int_val;
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_GVAL(IOV_OLPC_THRESH):
		if (pi->olpci != NULL) {
			int_val = pi->olpci->olpc_thresh;
			bcopy(&int_val, a, sizeof(int_val));
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_SVAL(IOV_OLPC_THRESH):
		if (pi->olpci != NULL) {
			/* iovar override for olpc_thresh */
			pi->olpci->olpc_thresh = (int8) int_val;
			pi->olpci->olpc_thresh_iovar_ovr = 1;
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_GVAL(IOV_DISABLE_OLPC):
		if (pi->olpci != NULL) {
			int_val = pi->olpci->disable_olpc;
			bcopy(&int_val, a, sizeof(int_val));
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_SVAL(IOV_DISABLE_OLPC):
		if (pi->olpci != NULL) {
			pi->olpci->disable_olpc = (int8) int_val;
			break;
		} else {
			err = BCME_UNSUPPORTED;
		}
	case IOV_GVAL(IOV_OLPC_OFFSET):
		if (pi->olpci) {
			bcopy(pi->olpci->olpc_offset, a, 5*sizeof(int8));
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_SVAL(IOV_OLPC_OFFSET):
		if (pi->olpci) {
			bcopy(p, pi->olpci->olpc_offset, 5*sizeof(int8));
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_GVAL(IOV_OLPC_IDX_VALID):
		if (pi->olpci != NULL) {
			int_val = pi->olpci->olpc_idx_valid;
			bcopy(&int_val, a, sizeof(int_val));
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
#endif	/* WLC_TXCAL || (WLOLPC && !WLOLPC_DISABLED) */
#ifdef WLC_TXCAL
	case IOV_GVAL(IOV_PHY_ADJUSTED_TSSI):
		if (!pi->sh->clk)
			err = BCME_NOCLK;
		else
			err = wlc_phy_iovar_adjusted_tssi(pi, ret_int_ptr,
				(uint8) int_val);
		break;
#if defined(BCMINTERNAL) || defined(WLTEST)
	case IOV_SVAL(IOV_PHY_TXCAL_GAINSWEEP):
	{
		wl_txcal_params_t txcal_params;
		bcopy(p, &txcal_params, sizeof(wl_txcal_params_t));
		wlc_phy_txcal_gainsweep(pi, &txcal_params);
		break;
	}
	case IOV_GVAL(IOV_PHY_TXCAL_GAINSWEEP_MEAS):
	{
		wl_txcal_meas_ncore_t * txcal_meas;
		uint16 size = OFFSETOF(wl_txcal_meas_ncore_t, txcal_percore) +
			PHY_CORE_MAX * sizeof(wl_txcal_meas_percore_t);

		txcal_meas = (wl_txcal_meas_ncore_t *)p;
		/* check for txcal version */
		if (txcal_meas->version != TXCAL_IOVAR_VERSION) {
			PHY_ERROR(("wl%d: %s: Version mismatch \n",
				pi->sh->unit, __FUNCTION__));
			err = BCME_VERSION;
			break;
		}
		if (alen < size)
		    return BCME_BADLEN;
		/* start filling up return buffer */
		txcal_meas = (wl_txcal_meas_ncore_t *)a;
		wlc_phy_txcal_copy_get_gainsweep_meas(pi, txcal_meas);

		break;
	}
	case IOV_SVAL(IOV_PHY_TXCAL_GAINSWEEP_MEAS):
	{
		wl_txcal_meas_ncore_t * txcal_meas;
		wl_txcal_meas_percore_t * per_core;
		uint16 i, j;

		txcal_meas = (wl_txcal_meas_ncore_t *)p;

		/* check for txcal version */
		if (txcal_meas->version != TXCAL_IOVAR_VERSION) {
			err = BCME_VERSION;
			break;
		}

		/* set per core info */
		per_core = txcal_meas->txcal_percore;
		for (i = 0; i < PHY_CORE_MAX; i++) {
			for (j = 0; j < pi->txcali->txcal_meas->valid_cnt; j++) {
				pi->txcali->txcal_meas->pwr[i][j] = per_core->pwr[j];
			}
			per_core++;
		}
		break;
	}
	case IOV_SVAL(IOV_PHY_TXCAL_PWR_TSSI_TBL):
	{
		wl_txcal_power_tssi_ncore_t * txcal_tssi;
		wl_txcal_power_tssi_t  *pi_txcal_pwr_tssi;

		txcal_tssi = (wl_txcal_power_tssi_ncore_t *)p;

		/* check for txcal version */
		if (txcal_tssi->version != TXCAL_IOVAR_VERSION) {
			err = BCME_VERSION;
			break;
		}
		/* FW copy */
		pi_txcal_pwr_tssi = pi->txcali->txcal_pwr_tssi;

		if (txcal_tssi->gen_tbl) {
			/* Generate table */
			wlc_phy_txcal_copy_set_pwr_tssi_tbl_gentbl(pi_txcal_pwr_tssi, txcal_tssi);
			wlc_phy_txcal_generate_pwr_tssi_tbl(pi);
		} else {
			wlc_phy_txcal_copy_set_pwr_tssi_tbl_storetbl(pi_txcal_pwr_tssi, txcal_tssi);
			/* 4355 supports both RSDB and MIMO modes.
			 * In RSDB mode they can use the tssi table generated (in MIMO mode)
			 * by copying core1 table to core0
			 * Currently for RSDB enabled builds
			 * tssi table of core1 will be copied to core0
			 * without checking the current mode,
			 * as for RSDB enabled builds default mode is MIMO
			 */
			if (ACMAJORREV_4(pi->pubpi->phy_rev) &&
					(phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1)) {
				wlc_phy_txcal_copy_rsdb_pwr_tssi_tbl(pi);
			}
		}
		/* apply new values */
		err = wlc_phy_txcal_store_pwr_tssi_tbl(pi);
		break;
	}
	case IOV_GVAL(IOV_PHY_TXCAL_PWR_TSSI_TBL):
	{
		wl_txcal_power_tssi_ncore_t * txcal_tssi;
		wl_txcal_power_tssi_t * in_buf = pi->txcali->txcal_pwr_tssi;
		uint16 buf_size = OFFSETOF(wl_txcal_power_tssi_ncore_t, tssi_percore) +
			PHY_CORE_MAX * sizeof(wl_txcal_power_tssi_percore_t);

		txcal_tssi = (wl_txcal_power_tssi_ncore_t *)p;

		/* check for txcal version */
		if (txcal_tssi->version != TXCAL_IOVAR_VERSION) {
			err = BCME_VERSION;
			break;
		}
		wlc_phy_txcal_get_pwr_tssi_tbl(pi, (uint8)txcal_tssi->channel);
		if (alen < buf_size)
		   return BCME_BADLEN;

		txcal_tssi = (wl_txcal_power_tssi_ncore_t *)a;
		wlc_phy_txcal_copy_get_pwr_tssi_tbl(txcal_tssi, in_buf);
		break;
	}
	case IOV_GVAL(IOV_OLPC_ANCHOR_IDX):
	{
		wl_olpc_pwr_t olpc_pwr;
		uint8 core;
		bcopy(p, &olpc_pwr, sizeof(wl_olpc_pwr_t));

		/* Check for txcal version */
		if (olpc_pwr.version != TXCAL_IOVAR_VERSION) {
			PHY_ERROR(("wl%d: %s  version mismatch",
				pi->sh->unit, __FUNCTION__));
			err = BCME_VERSION;
			break;
		}
		core = olpc_pwr.core;
		wl_txcal_power_tssi_t * in_buf = pi->txcali->txcal_pwr_tssi;

		wlc_phy_txcal_get_pwr_tssi_tbl(pi, olpc_pwr.channel);
		olpc_pwr.olpc_idx = in_buf->pwr_start_idx[core];
		olpc_pwr.tempsense = in_buf->tempsense[core];
		olpc_pwr.version = TXCAL_IOVAR_VERSION;

		bcopy(&olpc_pwr, a, sizeof(wl_olpc_pwr_t));

		break;
	}
	case IOV_SVAL(IOV_OLPC_ANCHOR_IDX):
	{
		wl_olpc_pwr_t olpc_pwr;
		uint8 core;
		bcopy(p, &olpc_pwr, sizeof(wl_olpc_pwr_t));

		/* Get the structure for the particular channel */
		wlc_phy_txcal_get_pwr_tssi_tbl(pi, olpc_pwr.channel);
		wl_txcal_power_tssi_t * in_buf = pi->txcali->txcal_pwr_tssi;


		/* Check for txcal version */
		if (olpc_pwr.version != TXCAL_IOVAR_VERSION) {
			err = BCME_VERSION;
			break;
		}

		core = olpc_pwr.core;

		in_buf->channel = olpc_pwr.channel;
		in_buf->pwr_start_idx[core] = olpc_pwr.olpc_idx;
		in_buf->tempsense[core] = olpc_pwr.tempsense;

		err = wlc_phy_txcal_store_pwr_tssi_tbl(pi);

		/* If in use, update olpc anchor info for curchan */
		if (ISACPHY(pi) && pi->olpci->olpc_idx_in_use)
			wlc_phy_set_olpc_anchor_acphy(pi);
		break;
	}
	case IOV_SVAL(IOV_PHY_TXCAL_APPLY_PWR_TSSI_TBL):
	{
		if (ISACPHY(pi)) {
			if (pi->sh->clk) {
				if (int_val) {
					/* Use Tx cal based pwr_tssi_tbl */
					wlc_phy_apply_pwr_tssi_tble_chan_acphy(pi);
				} else {
					/* Use PA Params */
					wlc_phy_txcal_apply_pa_params(pi);
				}
			}
			pi->txcali->txcal_pwr_tssi_tbl_in_use = (bool)int_val;
#if (defined(LCN20CONF) && (LCN20CONF != 0))
		} else if (ISLCN20PHY(pi)) {
			if (pi->sh->clk) {
				if (int_val) {
					/* Use Tx cal based pwr_tssi_tbl */
					if (wlc_phy_apply_pwr_tssi_tble_chan_lcn20phy(pi)
							!= BCME_OK)
						return BCME_ERROR;
				} else {
					/* Use PA Params */
					if (wlc_phy_txcal_apply_pa_params(pi) != BCME_OK)
						return BCME_ERROR;
				}
			}
			pi->txcali->txcal_pwr_tssi_tbl_in_use = (bool)int_val;
#endif /* #if ((defined(LCN20CONF) && (LCN20CONF != 0))) */
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	}
	case IOV_GVAL(IOV_PHY_TXCAL_APPLY_PWR_TSSI_TBL):
	{
		if (ISACPHY(pi) || ISLCN20PHY(pi)) {
			*ret_int_ptr = (int32)pi->txcali->txcal_pwr_tssi_tbl_in_use;
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	}
	case IOV_GVAL(IOV_PHY_READ_ESTPWR_LUT):
	{
		if (ISACPHY(pi)) {
			uint16 estpwr[128];
			uint8 tx_pwr_ctrl_state;
			uint32 tbl_len = 128;
			uint32 tbl_offset = 0;
			uint8 core;
			core = (uint8) int_val;
			bool suspend = FALSE;
			wlc_phy_conditional_suspend(pi, &suspend);
			tx_pwr_ctrl_state =  pi->txpwrctrl;
			wlc_phy_txpwrctrl_enable_acphy(pi, PHY_TPC_HW_OFF);
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_ESTPWRLUTS(core),
				tbl_len, tbl_offset, 16, estpwr);
			wlc_phy_txpwrctrl_enable_acphy(pi, tx_pwr_ctrl_state);
			bcopy(estpwr, a, 128*sizeof(uint16));
			wlc_phy_conditional_resume(pi, &suspend);
#if (defined(LCN20CONF) && (LCN20CONF != 0))
		} else if (ISLCN20PHY(pi)) {
			uint32 estpwr[128];
			phytbl_info_t tab;

			tab.tbl_offset = 0;
			tab.tbl_id = LCN20PHY_TBL_ID_TXPWRCTL;
			tab.tbl_width = 32;
			tab.tbl_ptr = estpwr;
			tab.tbl_len = 128;

			wlc_lcn20phy_read_table(pi,  &tab);
			bcopy(estpwr, a, 128*sizeof(uint32));
#endif /* #if ((defined(LCN20CONF) && (LCN20CONF != 0))) */
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	}
	case IOV_GVAL(IOV_PHY_TXCAL_STATUS):
	{
		if (!pi->sh->up) {
			err = BCME_NOTUP;
			break;
		} else {
			*ret_int_ptr = (int32) pi->txcali->txcal_status;
		}
		break;
	}
	case IOV_GVAL(IOV_PHY_TXCALVER):
	{
		*ret_int_ptr = TXCAL_IOVAR_VERSION;
		break;
	}
#endif	/* BCMINTERNAL || WLTEST */
#endif /* WLC_TXCAL */
#if defined(BCMDBG) || defined(BCMINTERNAL) || defined(WLTEST) || defined(BCMCCX)
	case IOV_GVAL(IOV_TXINSTPWR):
		phy_utils_phyreg_enter(pi);
		/* Return the current instantaneous est. power
		 * For swpwr ctrl it's based on current TSSI value (as opposed to average)
		 */
		wlc_phy_txpower_get_instant(pi, a);
		phy_utils_phyreg_exit(pi);
		break;
#endif /* BCMDBG || BCMINTERNAL || WLTEST || BCMCCX */

#if defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD)
	case IOV_GVAL(IOV_PHY_TXPWRCTRL):
		wlc_phy_iovar_txpwrctrl(pi, int_val, bool_val, ret_int_ptr, FALSE);
		break;

	case IOV_SVAL(IOV_PHY_TXPWRCTRL):
		err = wlc_phy_iovar_txpwrctrl(pi, int_val, bool_val, ret_int_ptr, TRUE);
		break;

	case IOV_SVAL(IOV_PHY_TXPWRINDEX):
		if (!pi->sh->clk) {
			err = BCME_NOCLK;
			break;
		}
		err = wlc_phy_iovar_txpwrindex_set(pi, p);
		break;

	case IOV_GVAL(IOV_PHY_TXPWRINDEX):
		if (!pi->sh->clk) {
			err = BCME_NOCLK;
			break;
		}
		wlc_phy_iovar_txpwrindex_get(pi, int_val, bool_val, ret_int_ptr);
		break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(ATE_BUILD) */
#if defined(BCMINTERNAL) || defined(WLTEST)
	case IOV_GVAL(IOV_PATRIM):
		if (ISACPHY(pi))
			wlc_phy_iovar_patrim_acphy(pi, ret_int_ptr);
		else
			*ret_int_ptr = 0;
	break;

	case IOV_GVAL(IOV_PAVARS2): {
			wl_pavars2_t *invar = (wl_pavars2_t *)p;
			wl_pavars2_t *outvar = (wl_pavars2_t *)a;
			uint16 *outpa = outvar->inpa;
			uint j = 0; /* PA parameters start from offset */

			if (invar->ver	!= WL_PHY_PAVAR_VER) {
				PHY_ERROR(("Incompatible version; use %d expected version %d\n",
					invar->ver, WL_PHY_PAVAR_VER));
				return BCME_BADARG;
			}

			outvar->ver = WL_PHY_PAVAR_VER;
			outvar->len = sizeof(wl_pavars2_t);
			if (wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec)
				== invar->bandrange)
				outvar->inuse = 1;
			else
				outvar->inuse = 0;

#ifdef BAND5G
			if (pi->sromi->subband5Gver == PHY_SUBBAND_5BAND) {
				if ((invar->bandrange == WL_CHAN_FREQ_RANGE_5GL) ||
					(invar->bandrange == WL_CHAN_FREQ_RANGE_5GM) ||
					(invar->bandrange == WL_CHAN_FREQ_RANGE_5GH)) {
					outvar->phy_type = PHY_TYPE_NULL;
					break;
				}
			}

			if (pi->sromi->subband5Gver == PHY_SUBBAND_3BAND_JAPAN) {
				if ((invar->bandrange == WL_CHAN_FREQ_RANGE_5GLL_5BAND) ||
					(invar->bandrange == WL_CHAN_FREQ_RANGE_5GLH_5BAND) ||
					(invar->bandrange == WL_CHAN_FREQ_RANGE_5GML_5BAND) ||
					(invar->bandrange == WL_CHAN_FREQ_RANGE_5GMH_5BAND) ||
					(invar->bandrange == WL_CHAN_FREQ_RANGE_5GH_5BAND)) {
					outvar->phy_type = PHY_TYPE_NULL;
					break;
				}
			}
#endif /* BAND5G */

			if (ISHTPHY(pi)) {
				if (invar->phy_type != PHY_TYPE_HT) {
					outvar->phy_type = PHY_TYPE_NULL;
					break;
				}

				if (invar->chain >= PHYCORENUM(pi->pubpi->phy_corenum))
					return BCME_BADARG;

				switch (invar->bandrange) {
				case WL_CHAN_FREQ_RANGE_2G:
				case WL_CHAN_FREQ_RANGE_5GL:
				case WL_CHAN_FREQ_RANGE_5GM:
				case WL_CHAN_FREQ_RANGE_5GH:
					wlc_phy_pavars_get_htphy(pi, &outpa[j], invar->bandrange,
						invar->chain);
					break;
				default:
					PHY_ERROR(("bandrange %d is out of scope\n",
						invar->bandrange));
					break;
				}
			} else if (ISLCNCOMMONPHY(pi)) {
				if ((invar->phy_type != PHY_TYPE_LCN) &&
					(invar->phy_type != PHY_TYPE_LCN40)) {
					outvar->phy_type = PHY_TYPE_NULL;
					break;
				}

				if (invar->chain != 0)
					return BCME_BADARG;

				switch (invar->bandrange) {
				case WL_CHAN_FREQ_RANGE_2G:
					outpa[j++] = pi->txpa_2g[0];		/* b0 */
					outpa[j++] = pi->txpa_2g[1];		/* b1 */
					outpa[j++] = pi->txpa_2g[2];		/* a1 */
					break;
#ifdef BAND5G
				case WL_CHAN_FREQ_RANGE_5GL:
					outpa[j++] = pi->txpa_5g_low[0];	/* b0 */
					outpa[j++] = pi->txpa_5g_low[1];	/* b1 */
					outpa[j++] = pi->txpa_5g_low[2];	/* a1 */
					break;

				case WL_CHAN_FREQ_RANGE_5GM:
					outpa[j++] = pi->txpa_5g_mid[0];	/* b0 */
					outpa[j++] = pi->txpa_5g_mid[1];	/* b1 */
					outpa[j++] = pi->txpa_5g_mid[2];	/* a1 */
					break;

				case WL_CHAN_FREQ_RANGE_5GH:
					outpa[j++] = pi->txpa_5g_hi[0]; /* b0 */
					outpa[j++] = pi->txpa_5g_hi[1]; /* b1 */
					outpa[j++] = pi->txpa_5g_hi[2]; /* a1 */
					break;
#endif /* BAND5G */
				default:
					PHY_ERROR(("bandrange %d is out of scope\n",
						invar->bandrange));
					break;
				}
			} else {
				PHY_ERROR(("Unsupported PHY type!\n"));
				err = BCME_UNSUPPORTED;
			}
		}
		break;

		case IOV_SVAL(IOV_PAVARS2): {
			wl_pavars2_t *invar = (wl_pavars2_t *)p;
			uint16 *inpa = invar->inpa;
			uint j = 0; /* PA parameters start from offset */

			if (invar->ver	!= WL_PHY_PAVAR_VER) {
				PHY_ERROR(("Incompatible version; use %d expected version %d\n",
					invar->ver, WL_PHY_PAVAR_VER));
				return BCME_BADARG;
			}

			if (ISHTPHY(pi)) {
				if (invar->phy_type != PHY_TYPE_HT) {
					break;
				}

				if (invar->chain >= PHYCORENUM(pi->pubpi->phy_corenum))
					return BCME_BADARG;

				switch (invar->bandrange) {
				case WL_CHAN_FREQ_RANGE_2G:
				case WL_CHAN_FREQ_RANGE_5GL:
				case WL_CHAN_FREQ_RANGE_5GM:
				case WL_CHAN_FREQ_RANGE_5GH:
				case WL_CHAN_FREQ_RANGE_5GLL_5BAND:
				case WL_CHAN_FREQ_RANGE_5GLH_5BAND:
				case WL_CHAN_FREQ_RANGE_5GML_5BAND:
				case WL_CHAN_FREQ_RANGE_5GMH_5BAND:
				case WL_CHAN_FREQ_RANGE_5GH_5BAND:
					if (invar->bandrange < (CH_2G_GROUP + CH_5G_4BAND)) {
						wlc_phy_pavars_set_htphy(pi, &inpa[j],
							invar->bandrange, invar->chain);
					} else {
						err = BCME_RANGE;
						PHY_ERROR(("bandrange %d is out of scope\n",
							invar->bandrange));
					}
					break;
				default:
					err = BCME_RANGE;
					PHY_ERROR(("bandrange %d is out of scope\n",
						invar->bandrange));
					break;
				}
			} else if (ISLCNCOMMONPHY(pi)) {
				if ((invar->phy_type != PHY_TYPE_LCN) &&
					(invar->phy_type != PHY_TYPE_LCN40))
					break;

				if (invar->chain != 0)
					return BCME_BADARG;

				switch (invar->bandrange) {
				case WL_CHAN_FREQ_RANGE_2G:
					pi->txpa_2g[0] = inpa[j++]; /* b0 */
					pi->txpa_2g[1] = inpa[j++]; /* b1 */
					pi->txpa_2g[2] = inpa[j++]; /* a1 */
					break;
#ifdef BAND5G
				case WL_CHAN_FREQ_RANGE_5GL:
					pi->txpa_5g_low[0] = inpa[j++]; /* b0 */
					pi->txpa_5g_low[1] = inpa[j++]; /* b1 */
					pi->txpa_5g_low[2] = inpa[j++]; /* a1 */
					break;

				case WL_CHAN_FREQ_RANGE_5GM:
					pi->txpa_5g_mid[0] = inpa[j++]; /* b0 */
					pi->txpa_5g_mid[1] = inpa[j++]; /* b1 */
					pi->txpa_5g_mid[2] = inpa[j++]; /* a1 */
					break;

				case WL_CHAN_FREQ_RANGE_5GH:
					pi->txpa_5g_hi[0] = inpa[j++];	/* b0 */
					pi->txpa_5g_hi[1] = inpa[j++];	/* b1 */
					pi->txpa_5g_hi[2] = inpa[j++];	/* a1 */
					break;
#endif /* BAND5G */
				default:
					PHY_ERROR(("bandrange %d is out of scope\n",
						invar->bandrange));
					break;
				}
			} else {
				PHY_ERROR(("Unsupported PHY type!\n"));
				err = BCME_UNSUPPORTED;
			}
		}
		break;

	case IOV_GVAL(IOV_POVARS): {
		wl_po_t tmppo;

		/* tmppo has the input phy_type and band */
		bcopy(p, &tmppo, sizeof(wl_po_t));
		if (ISHTPHY(pi)) {
			if ((tmppo.phy_type != PHY_TYPE_HT) && (tmppo.phy_type != PHY_TYPE_N))  {
				tmppo.phy_type = PHY_TYPE_NULL;
				break;
			}

			err = wlc_phy_get_po_htphy(pi, &tmppo);
			if (!err)
				bcopy(&tmppo, a, sizeof(wl_po_t));
			break;
		} else if (ISNPHY(pi)) {
			if (tmppo.phy_type != PHY_TYPE_N)  {
				tmppo.phy_type = PHY_TYPE_NULL;
				break;
			}

			/* Power offsets variables depend on the SROM revision */
			if (NREV_GE(pi->pubpi->phy_rev, 8) && (pi->sh->sromrev >= 9)) {
				err = wlc_phy_get_po_htphy(pi, &tmppo);

			} else {
				switch (tmppo.band) {
				case WL_CHAN_FREQ_RANGE_2G:
					tmppo.cckpo = pi->ppr->u.sr8.cck2gpo;
					tmppo.ofdmpo = pi->ppr->u.sr8.ofdm[tmppo.band];
					bcopy(&pi->ppr->u.sr8.mcs[tmppo.band][0], &tmppo.mcspo,
						8*sizeof(uint16));
					break;
#ifdef BAND5G
				case WL_CHAN_FREQ_RANGE_5G_BAND0:
					tmppo.ofdmpo = pi->ppr->u.sr8.ofdm[tmppo.band];
					bcopy(&pi->ppr->u.sr8.mcs[tmppo.band], &tmppo.mcspo,
						8*sizeof(uint16));
					break;

				case WL_CHAN_FREQ_RANGE_5G_BAND1:
					tmppo.ofdmpo = pi->ppr->u.sr8.ofdm[tmppo.band];
					bcopy(&pi->ppr->u.sr8.mcs[tmppo.band], &tmppo.mcspo,
						8*sizeof(uint16));
					break;

				case WL_CHAN_FREQ_RANGE_5G_BAND2:
					tmppo.ofdmpo = pi->ppr->u.sr8.ofdm[tmppo.band];
					bcopy(&pi->ppr->u.sr8.mcs[tmppo.band], &tmppo.mcspo,
						8*sizeof(uint16));
					break;

				case WL_CHAN_FREQ_RANGE_5G_BAND3:
					tmppo.ofdmpo = pi->ppr->u.sr8.ofdm[tmppo.band];
					bcopy(&pi->ppr->u.sr8.mcs[tmppo.band], &tmppo.mcspo,
						8*sizeof(uint16));
					break;
#endif /* BAND5G */
				default:
					PHY_ERROR(("bandrange %d is out of scope\n", tmppo.band));
					err = BCME_BADARG;
					break;
				}
			}

			if (!err)
				bcopy(&tmppo, a, sizeof(wl_po_t));
		} else if (ISLCNCOMMONPHY(pi)) {
			if ((tmppo.phy_type != PHY_TYPE_LCN) &&
				(tmppo.phy_type != PHY_TYPE_LCN40)) {
				tmppo.phy_type = PHY_TYPE_NULL;
				break;
			}

			switch (tmppo.band) {
			case WL_CHAN_FREQ_RANGE_2G:
				tmppo.cckpo = pi->ppr->u.sr8.cck2gpo;
				tmppo.ofdmpo = pi->ppr->u.sr8.ofdm[tmppo.band];
				bcopy(&pi->ppr->u.sr8.mcs[tmppo.band], &tmppo.mcspo,
					8*sizeof(uint16));

				break;

			default:
				PHY_ERROR(("bandrange %d is out of scope\n", tmppo.band));
				err = BCME_BADARG;
				break;
			}

			if (!err)
				bcopy(&tmppo, a, sizeof(wl_po_t));
		} else {
			PHY_ERROR(("Unsupported PHY type!\n"));
			err = BCME_UNSUPPORTED;
		}
	}
	break;

	case IOV_SVAL(IOV_POVARS): {
		wl_po_t inpo;

		bcopy(p, &inpo, sizeof(wl_po_t));

		if (ISHTPHY(pi)) {
			if ((inpo.phy_type == PHY_TYPE_HT) || (inpo.phy_type == PHY_TYPE_N))
				err = wlc_phy_set_po_htphy(pi, &inpo);
			break;
		} else if (ISNPHY(pi)) {
			if (inpo.phy_type != PHY_TYPE_N)
				break;

			/* Power offsets variables depend on the SROM revision */
			if (NREV_GE(pi->pubpi->phy_rev, 8) && (pi->sh->sromrev >= 9)) {
				err = wlc_phy_set_po_htphy(pi, &inpo);

			} else {

				switch (inpo.band) {
				case WL_CHAN_FREQ_RANGE_2G:
					pi->ppr->u.sr8.cck2gpo = inpo.cckpo;
					pi->ppr->u.sr8.ofdm[inpo.band]  = inpo.ofdmpo;
					bcopy(inpo.mcspo, &(pi->ppr->u.sr8.mcs[inpo.band][0]),
						8*sizeof(uint16));
					break;
#ifdef BAND5G
				case WL_CHAN_FREQ_RANGE_5G_BAND0:
					pi->ppr->u.sr8.ofdm[inpo.band] = inpo.ofdmpo;
					bcopy(inpo.mcspo, &(pi->ppr->u.sr8.mcs[inpo.band][0]),
						8*sizeof(uint16));
					break;

				case WL_CHAN_FREQ_RANGE_5G_BAND1:
					pi->ppr->u.sr8.ofdm[inpo.band] = inpo.ofdmpo;
					bcopy(inpo.mcspo, &(pi->ppr->u.sr8.mcs[inpo.band][0]),
						8*sizeof(uint16));
					break;

				case WL_CHAN_FREQ_RANGE_5G_BAND2:
					pi->ppr->u.sr8.ofdm[inpo.band] = inpo.ofdmpo;
					bcopy(inpo.mcspo, &(pi->ppr->u.sr8.mcs[inpo.band][0]),
						8*sizeof(uint16));
					break;

				case WL_CHAN_FREQ_RANGE_5G_BAND3:
					pi->ppr->u.sr8.ofdm[inpo.band] = inpo.ofdmpo;
					bcopy(inpo.mcspo, &(pi->ppr->u.sr8.mcs[inpo.band][0]),
						8*sizeof(uint16));
					break;
#endif /* BAND5G */
				default:
					PHY_ERROR(("bandrange %d is out of scope\n", inpo.band));
					err = BCME_BADARG;
					break;
				}

			}

		} else {
			PHY_ERROR(("Unsupported PHY type!\n"));
			err = BCME_UNSUPPORTED;
		}
	}
	break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

	case IOV_GVAL(IOV_SROM_REV): {
			*ret_int_ptr = pi->sh->sromrev;
	}
	break;

#ifdef WLTEST
	case IOV_GVAL(IOV_PHY_MAXP): {
		if (ISNPHY(pi)) {
			srom_pwrdet_t	*pwrdet  = pi->pwrdet;
			uint8*	maxp = (uint8*)a;

			maxp[0] = pwrdet->max_pwr[PHY_CORE_0][WL_CHAN_FREQ_RANGE_2G];
			maxp[1] = pwrdet->max_pwr[PHY_CORE_1][WL_CHAN_FREQ_RANGE_2G];
			maxp[2] = pwrdet->max_pwr[PHY_CORE_0][WL_CHAN_FREQ_RANGE_5G_BAND0];
			maxp[3] = pwrdet->max_pwr[PHY_CORE_1][WL_CHAN_FREQ_RANGE_5G_BAND0];
			maxp[4] = pwrdet->max_pwr[PHY_CORE_0][WL_CHAN_FREQ_RANGE_5G_BAND1];
			maxp[5] = pwrdet->max_pwr[PHY_CORE_1][WL_CHAN_FREQ_RANGE_5G_BAND1];
			maxp[6] = pwrdet->max_pwr[PHY_CORE_0][WL_CHAN_FREQ_RANGE_5G_BAND2];
			maxp[7] = pwrdet->max_pwr[PHY_CORE_1][WL_CHAN_FREQ_RANGE_5G_BAND2];
			if (pi->sromi->subband5Gver == PHY_SUBBAND_4BAND)
			{
				maxp[8] = pwrdet->max_pwr[PHY_CORE_0][WL_CHAN_FREQ_RANGE_5G_BAND3];
				maxp[9] = pwrdet->max_pwr[PHY_CORE_1][WL_CHAN_FREQ_RANGE_5G_BAND3];
			}
		}
		break;
	}
	case IOV_SVAL(IOV_PHY_MAXP): {
		if (ISNPHY(pi)) {
			uint8*	maxp = (uint8*)p;
			srom_pwrdet_t	*pwrdet  = pi->pwrdet;

			pwrdet->max_pwr[PHY_CORE_0][WL_CHAN_FREQ_RANGE_2G] = maxp[0];
			pwrdet->max_pwr[PHY_CORE_1][WL_CHAN_FREQ_RANGE_2G] = maxp[1];
			pwrdet->max_pwr[PHY_CORE_0][WL_CHAN_FREQ_RANGE_5G_BAND0] = maxp[2];
			pwrdet->max_pwr[PHY_CORE_1][WL_CHAN_FREQ_RANGE_5G_BAND0] = maxp[3];
			pwrdet->max_pwr[PHY_CORE_0][WL_CHAN_FREQ_RANGE_5G_BAND1] = maxp[4];
			pwrdet->max_pwr[PHY_CORE_1][WL_CHAN_FREQ_RANGE_5G_BAND1] = maxp[5];
			pwrdet->max_pwr[PHY_CORE_0][WL_CHAN_FREQ_RANGE_5G_BAND2] = maxp[6];
			pwrdet->max_pwr[PHY_CORE_1][WL_CHAN_FREQ_RANGE_5G_BAND2] = maxp[7];
			if (pi->sromi->subband5Gver == PHY_SUBBAND_4BAND)
			{
				pwrdet->max_pwr[PHY_CORE_0][WL_CHAN_FREQ_RANGE_5G_BAND3] = maxp[8];
				pwrdet->max_pwr[PHY_CORE_1][WL_CHAN_FREQ_RANGE_5G_BAND3] = maxp[9];
			}
		}
		break;
	}
#endif /* WLTEST */
#if defined(BCMINTERNAL) || defined(WLTEST)
	case IOV_SVAL(IOV_RPCALVARS): {
		const wl_rpcal_t *rpcal = p;
		if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
			PHY_ERROR(("Number of TX Chain has to be > 1!\n"));
			err = BCME_UNSUPPORTED;
		} else {
			pi->sromi->rpcal2g = rpcal[WL_CHAN_FREQ_RANGE_2G].update ?
			rpcal[WL_CHAN_FREQ_RANGE_2G].value : pi->sromi->rpcal2g;
			pi->sromi->rpcal5gb0 = rpcal[WL_CHAN_FREQ_RANGE_5G_BAND0].update ?
			rpcal[WL_CHAN_FREQ_RANGE_5G_BAND0].value : pi->sromi->rpcal5gb0;
			pi->sromi->rpcal5gb1 = rpcal[WL_CHAN_FREQ_RANGE_5G_BAND1].update ?
			rpcal[WL_CHAN_FREQ_RANGE_5G_BAND1].value : pi->sromi->rpcal5gb1;
			pi->sromi->rpcal5gb2 = rpcal[WL_CHAN_FREQ_RANGE_5G_BAND2].update ?
			rpcal[WL_CHAN_FREQ_RANGE_5G_BAND2].value : pi->sromi->rpcal5gb2;
			pi->sromi->rpcal5gb3 = rpcal[WL_CHAN_FREQ_RANGE_5G_BAND3].update ?
			rpcal[WL_CHAN_FREQ_RANGE_5G_BAND3].value : pi->sromi->rpcal5gb3;

			if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
				ACMAJORREV_33(pi->pubpi->phy_rev) ||
				ACMAJORREV_37(pi->pubpi->phy_rev)) {
				pi->sromi->rpcal2gcore3 =
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_2G].update ?
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_2G].value :
				pi->sromi->rpcal2gcore3;
				pi->sromi->rpcal5gb0core3 =
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND0].update ?
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND0].value :
				pi->sromi->rpcal5gb0core3;
				pi->sromi->rpcal5gb1core3 =
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND1].update ?
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND1].value :
				pi->sromi->rpcal5gb1core3;
				pi->sromi->rpcal5gb2core3 =
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND2].update ?
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND2].value :
				pi->sromi->rpcal5gb2core3;
				pi->sromi->rpcal5gb3core3 =
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND3].update ?
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND3].value :
				pi->sromi->rpcal5gb3core3;
			}
		}
		break;
	}

	case IOV_GVAL(IOV_RPCALVARS): {
		wl_rpcal_t *rpcal = a;
		if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
			PHY_ERROR(("Number of TX Chain has to be > 1!\n"));
			err = BCME_UNSUPPORTED;
		} else {
			rpcal[WL_CHAN_FREQ_RANGE_2G].value = pi->sromi->rpcal2g;
			rpcal[WL_CHAN_FREQ_RANGE_5G_BAND0].value = pi->sromi->rpcal5gb0;
			rpcal[WL_CHAN_FREQ_RANGE_5G_BAND1].value = pi->sromi->rpcal5gb1;
			rpcal[WL_CHAN_FREQ_RANGE_5G_BAND2].value = pi->sromi->rpcal5gb2;
			rpcal[WL_CHAN_FREQ_RANGE_5G_BAND3].value = pi->sromi->rpcal5gb3;
			if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
				ACMAJORREV_33(pi->pubpi->phy_rev) ||
				ACMAJORREV_37(pi->pubpi->phy_rev)) {
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_2G].value =
				pi->sromi->rpcal2gcore3;
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND0].value =
				pi->sromi->rpcal5gb0core3;
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND1].value =
				pi->sromi->rpcal5gb1core3;
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND2].value =
				pi->sromi->rpcal5gb2core3;
				rpcal[WL_NUM_RPCALVARS+WL_CHAN_FREQ_RANGE_5G_BAND3].value =
				pi->sromi->rpcal5gb3core3;
			}
		}
		break;
	}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if defined(BCMDBG)
	case IOV_SVAL(IOV_PHY_FORCE_FDIQI):
	{

		wlc_phy_force_fdiqi_acphy(pi, (uint16) int_val);

		break;
	}
#endif /* BCMDBG */

	case IOV_SVAL(IOV_EDCRS):
	{

		if (int_val == 0)
		{
			W_REG(pi->sh->osh, &pi->regs->PHYREF_IFS_CTL_SEL_PRICRS, 0x000F);
		}
		else
		{
			W_REG(pi->sh->osh, &pi->regs->PHYREF_IFS_CTL_SEL_PRICRS, 0x0F0F);
		}
		break;
	}
	case IOV_GVAL(IOV_EDCRS):
	{
		if (R_REG(pi->sh->osh, &pi->regs->PHYREF_IFS_CTL_SEL_PRICRS) == 0x000F)
		{
			*ret_int_ptr = 0;
		}
		else if (R_REG(pi->sh->osh, &pi->regs->PHYREF_IFS_CTL_SEL_PRICRS) == 0x0F0F)
		{
			*ret_int_ptr = 1;
		}
		break;
	}
	case IOV_GVAL(IOV_PHY_AFE_OVERRIDE):
		if (!ACMAJORREV_3(pi->pubpi->phy_rev)) {
			*ret_int_ptr = (int32)pi->afe_override;
		} else {
			PHY_ERROR(("PHY_AFE_OVERRIDE is not supported for this chip \n"));
		}
		break;
	case IOV_SVAL(IOV_PHY_AFE_OVERRIDE):
		if (!ACMAJORREV_3(pi->pubpi->phy_rev)) {
			pi->afe_override = (uint8)int_val;
		} else {
			PHY_ERROR(("PHY_AFE_OVERRIDE is not supported for this chip \n"));
		}
		break;
#if defined(BCMINTERNAL) || defined(WLTEST)
	case IOV_SVAL(IOV_PHY_AUXPGA):
			if (ISLCNCOMMONPHY(pi)) {
				phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
				uint8 *aug_pga_val = (uint8*)p;
				pi_lcn->lcnphy_rssi_vf = aug_pga_val[0];
				pi_lcn->lcnphy_rssi_vc = aug_pga_val[1];
				pi_lcn->lcnphy_rssi_gs = aug_pga_val[2];
#ifdef BAND5G
				pi_lcn->rssismf5g = aug_pga_val[3];
				pi_lcn->rssismc5g = aug_pga_val[4];
				pi_lcn->rssisav5g = aug_pga_val[5];
#endif
			}
		break;

	case IOV_GVAL(IOV_PHY_AUXPGA):
			if (ISLCNCOMMONPHY(pi)) {
				phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
				uint8 *aug_pga_val = (uint8*)a;
				aug_pga_val[0] = pi_lcn->lcnphy_rssi_vf;
				aug_pga_val[1] = pi_lcn->lcnphy_rssi_vc;
				aug_pga_val[2] = pi_lcn->lcnphy_rssi_gs;
#ifdef BAND5G
				aug_pga_val[3] = (uint8) pi_lcn->rssismf5g;
				aug_pga_val[4] = (uint8) pi_lcn->rssismc5g;
				aug_pga_val[5] = (uint8) pi_lcn->rssisav5g;
#endif
			}
		break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if defined(WLTEST)
#if defined(LCNCONF) || defined(LCN40CONF)
	case IOV_GVAL(IOV_LCNPHY_RXIQGAIN):
		{
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			*ret_int_ptr = (int32)pi_lcn->rxpath_gain;
		}
		break;
	case IOV_GVAL(IOV_LCNPHY_RXIQGSPOWER):
		{
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			*ret_int_ptr = (int32)pi_lcn->rxpath_gainselect_power;
		}
		break;
	case IOV_GVAL(IOV_LCNPHY_RXIQPOWER):
		{
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			*ret_int_ptr = (int32)pi_lcn->rxpath_final_power;
		}
		break;
	case IOV_GVAL(IOV_LCNPHY_RXIQSTATUS):
		{
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			*ret_int_ptr = (int32)pi_lcn->rxpath_status;
		}
		break;
	case IOV_GVAL(IOV_LCNPHY_RXIQSTEPS):
		{
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			*ret_int_ptr = (int32)pi_lcn->rxpath_steps;
		}
		break;
	case IOV_SVAL(IOV_LCNPHY_RXIQSTEPS):
		{
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			pi_lcn->rxpath_steps = (uint8)int_val;
		}
		break;
	case IOV_GVAL(IOV_LCNPHY_TSSI_MAXPWR):
		{
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			*ret_int_ptr = (int32)pi_lcn->tssi_maxpwr_limit;
		}
		break;
	case IOV_GVAL(IOV_LCNPHY_TSSI_MINPWR):
		{
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			*ret_int_ptr = (int32)pi_lcn->tssi_minpwr_limit;
		}
		break;
#endif /* #if defined(LCNCONF) || defined(LCN40CONF) */
#if LCN40CONF || (defined(LCN20CONF) && (LCN20CONF != 0))
	case IOV_GVAL(IOV_LCNPHY_TXPWRCLAMP_DIS):
		if (ISLCN40PHY(pi)) {
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			*ret_int_ptr = (int32)pi_lcn->txpwr_clamp_dis;
		}
#if (defined(LCN20CONF) && (LCN20CONF != 0))
		else if (ISLCN20PHY(pi)) {
			*ret_int_ptr = (int32)(int32)pi->u.pi_lcn20phy->txpwr_clamp_dis;
		}
#endif /* (defined(LCN20CONF) && (LCN20CONF != 0)) */
		break;
	case IOV_SVAL(IOV_LCNPHY_TXPWRCLAMP_DIS):
		if (ISLCN40PHY(pi)) {
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			pi_lcn->txpwr_clamp_dis = (uint8)int_val;
		}
#if (defined(LCN20CONF) && (LCN20CONF != 0))
		else if (ISLCN20PHY(pi)) {
			pi->u.pi_lcn20phy->txpwr_clamp_dis = (uint8)int_val;
		}
#endif /* (defined(LCN20CONF) && (LCN20CONF != 0)) */
		break;
	case IOV_GVAL(IOV_LCNPHY_TXPWRCLAMP_OFDM):
		if (ISLCN40PHY(pi)) {
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			*ret_int_ptr = (int32)pi_lcn->target_pwr_ofdm_max;
		}
#if (defined(LCN20CONF) && (LCN20CONF != 0))
		else if (ISLCN20PHY(pi)) {
			*ret_int_ptr = (int32)pi->u.pi_lcn20phy->target_pwr_ofdm_max;
		}
#endif /* (defined(LCN20CONF) && (LCN20CONF != 0)) */
		break;
	case IOV_GVAL(IOV_LCNPHY_TXPWRCLAMP_CCK):
		if (ISLCN40PHY(pi)) {
			phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
			*ret_int_ptr = (int32)pi_lcn->target_pwr_cck_max;
		}
#if (defined(LCN20CONF) && (LCN20CONF != 0))
		else if (ISLCN20PHY(pi)) {
			*ret_int_ptr = (int32)pi->u.pi_lcn20phy->target_pwr_cck_max;
		}
#endif /* (defined(LCN20CONF) && (LCN20CONF != 0)) */
		break;
#endif /* #if LCN40CONF */
#endif /* #if defined(WLTEST) */
	case IOV_SVAL(IOV_PHY_CRS_WAR):
		if (ISLCN40PHY(pi)) {
			pi->u.pi_lcn40phy->phycrs_war_en = (bool)int_val;
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
	case IOV_GVAL(IOV_PHY_CRS_WAR):
		if (ISLCN40PHY(pi)) {
			*ret_int_ptr = (int32) pi->u.pi_lcn40phy->phycrs_war_en;
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
#if NCONF
	case IOV_GVAL(IOV_PHY_OCLSCDENABLE):
		err = wlc_phy_iovar_oclscd(pi, int_val, bool_val, ret_int_ptr, FALSE);
		break;

	case IOV_SVAL(IOV_PHY_OCLSCDENABLE):
		err = wlc_phy_iovar_oclscd(pi, int_val, bool_val, ret_int_ptr, TRUE);
		break;

	case IOV_GVAL(IOV_LNLDO2):
		wlc_phy_iovar_prog_lnldo2(pi, int_val, bool_val, ret_int_ptr, FALSE);
		break;

	case IOV_SVAL(IOV_LNLDO2):
		err = wlc_phy_iovar_prog_lnldo2(pi, int_val, bool_val, ret_int_ptr, TRUE);
		break;
#endif /* NCONF */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV)
	case IOV_GVAL(IOV_PHY_DYN_ML):
		err =  wlc_phy_dynamic_ml(pi, int_val, ret_int_ptr, vsize, FALSE);
		break;

	case IOV_SVAL(IOV_PHY_DYN_ML):
		err =  wlc_phy_dynamic_ml(pi, int_val, ret_int_ptr, vsize, TRUE);
		break;

	case IOV_GVAL(IOV_PHY_ACI_NAMS):
		err =  wlc_phy_aci_nams(pi, int_val, ret_int_ptr, vsize, FALSE);
		break;

	case IOV_SVAL(IOV_PHY_ACI_NAMS):
		err =  wlc_phy_aci_nams(pi, int_val, ret_int_ptr, vsize, TRUE);
		break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) */

#if defined(BCMDBG) || defined(WLTEST)
	case IOV_GVAL(IOV_PHY_MASTER_OVERRIDE): {
		*ret_int_ptr = phy_get_master(pi);
		break;
	}

	case IOV_SVAL(IOV_PHY_MASTER_OVERRIDE): {
		if ((int_val < -1) || (int_val > 1)) {
		  err = BCME_RANGE;
		  PHY_ERROR(("Value out of range\n"));
		  break;
		}
		*ret_int_ptr = phy_set_master(pi, (int8)int_val);
		break;
	}
#endif /* defined(BCMDBG) ||  defined(WLTEST) */

	default:
		err = BCME_UNSUPPORTED;
	}

	if (err == BCME_UNSUPPORTED)
		err = wlc_phy_iovar_dispatch_old(pi, actionid, p, a, vsize, int_val, bool_val);

	if (err == BCME_UNSUPPORTED)
		err = phy_iovars_wrapper_sample_collect(pi, actionid, -1, p, plen, a, alen, vsize);

	return err;
}

int phy_legacy_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

int
BCMATTACHFN(phy_legacy_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;
#if defined(WLC_PATCH_IOCTL)
	wlc_iov_disp_fn_t disp_fn = IOV_PATCH_FN;
	const bcm_iovar_t* patch_table = IOV_PATCH_TBL;
#else
	wlc_iov_disp_fn_t disp_fn = NULL;
	const bcm_iovar_t* patch_table = NULL;
#endif /* WLC_PATCH_IOCTL */

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_iovars,
	                   phy_legacy_pack_iov, phy_legacy_unpack_iov,
	                   phy_doiovar, disp_fn, patch_table, pi,
	                   &iovd);
	return wlc_iocv_register_iovt(ii, &iovd);
}
