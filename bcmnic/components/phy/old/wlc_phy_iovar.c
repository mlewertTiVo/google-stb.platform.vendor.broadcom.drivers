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
 * $Id: wlc_phy_iovar.c 665670 2016-10-18 19:20:52Z $
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
#include <phy_mem.h>
#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>
#include <phy_calmgr_api.h>
#include <phy_stf.h>
#include <wlc_phy_iovar.h>
#include <phy_utils_pmu.h>
#include <wlc_phy_n.h>

#if defined(DBG_PHY_IOV) || defined(BCMDBG)
static void wlc_phy_iovar_set_papd_offset(phy_info_t *pi, int16 int_val);
static int wlc_phy_iovar_get_papd_offset(phy_info_t *pi);
#endif

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

#endif /* NCONF */
#if defined(BCMDBG)
	{"fast_timer", IOV_FAST_TIMER,
	(IOVF_NTRL | IOVF_MFG), 0, IOVT_UINT32, 0
	},
	{"slow_timer", IOV_SLOW_TIMER,
	(IOVF_NTRL | IOVF_MFG), 0, IOVT_UINT32, 0
	},
#endif 
#if defined(BCMDBG) || defined(PHYCAL_CHNG_CS)
	{"glacial_timer", IOV_GLACIAL_TIMER,
	IOVF_NTRL, 0, IOVT_UINT32, 0
	},
#endif
#if defined(DBG_PHY_IOV)
	{"phy_watchdog", IOV_PHY_WATCHDOG,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif
	{"cal_period", IOV_CAL_PERIOD,
	0, 0, IOVT_UINT32, 0
	},
	{"phy_rx_gainindex", IOV_PHY_RXGAININDEX,
	IOVF_GET_UP | IOVF_SET_UP, 0, IOVT_UINT16, 0
	},
	{"phy_forcecal", IOV_PHY_FORCECAL,
	IOVF_SET_UP, 0, IOVT_UINT8, 0
	},
#if defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG)
	{"phy_forcecal_obt", IOV_PHY_FORCECAL_OBT,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif 
	{"num_stream", IOV_NUM_STREAM,
	(0), 0, IOVT_INT32, 0
	},
	{"min_txpower", IOV_MIN_TXPOWER,
	0, 0, IOVT_UINT32, 0
	},
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
	{"phy_bphymrc", IOV_PHY_BPHYMRC,
	(IOVF_SET_DOWN | IOVF_SET_BAND), 0, IOVT_UINT8, 0
	},
#if defined(BCMDBG)
	{"phy_txpwr_core", IOV_PHY_TXPWR_CORE,
	0, 0, IOVT_UINT32, 0
	},
#endif 
#if defined(DBG_PHY_IOV) || defined(BCMDBG)
	{"phy_papd_eps_offset", IOV_PAPD_EPS_OFFSET,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_INT16, 0
	},
#endif 
	{"phydump_page", IOV_PHY_DUMP_PAGE,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_INT8, 0
	},
	{"phydump_entry", IOV_PHY_DUMP_ENTRY,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_INT16, 0
	},
	/* ************************************************************************************ */
	/* [PHY_REARCH] Do not add any variables here. Add them to the individual modules */
	/* ************************************************************************************ */
#if defined(BCMDBG) || defined(BCMDBG_TEMPSENSE)
	{"phy_tempsense", IOV_PHY_TEMPSENSE,
	IOVF_GET_UP, 0, IOVT_INT16, 0
	},
#endif 
	{"phy_sromtempsense", IOV_PHY_SROM_TEMPSENSE,
	(IOVF_SET_UP | IOVF_GET_UP | IOVF_MFG), 0, IOVT_INT16, 0
	},
	{"gain_cal_temp", IOV_PHY_GAIN_CAL_TEMP,
	(0), 0, IOVT_INT16, 0
	},
#ifdef PHYMON
	{"phycal_state", IOV_PHYCAL_STATE,
	IOVF_GET_UP, 0, IOVT_UINT32, 0,
	},
#endif /* PHYMON */
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
	{"phy_oclscdenable", IOV_PHY_OCLSCDENABLE,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"lnldo2", IOV_LNLDO2,
	(IOVF_MFG), 0, IOVT_UINT8, 0
	},
#if defined(DBG_PHY_IOV)
	{"phy_dynamic_ml", IOV_PHY_DYN_ML,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
	{"aci_nams", IOV_PHY_ACI_NAMS,
	(IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0
	},
#endif 
#if defined(BCMDBG)
	{"lcnphy_txclampcck", IOV_LCNPHY_TXPWRCLAMP_CCK,
	IOVF_GET_UP, 0, IOVT_INT32, 0
	},
#endif

	{"sromrev", IOV_SROM_REV,
	(IOVF_SET_DOWN), 0, IOVT_UINT8, 0
	},
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
#if defined(BCMDBG)
	{"phy_master_override", IOV_PHY_MASTER_OVERRIDE,
	(IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG), 0, IOVT_INT8, 0
	},
#endif 
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
#include <wlc_phytbl_n.h>
#include <wlc_phytbl_ht.h>
#include <wlc_phytbl_ac.h>
#include <wlc_phytbl_20691.h>
#include <wlc_phytbl_20693.h>
#include <wlc_phytbl_20694.h>
#include <wlc_phy_radio.h>
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
#include <phy_cache_api.h>
#include <phy_misc_api.h>
#include <phy_btcx.h>
#include <phy_calmgr.h>
#include <phy_temp.h>
#include <phy_tpc.h>
#include <phy_ac_calmgr.h>
#include <phy_noise.h>



/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/*  macro, typedef, enum, structure, global variable		*/
/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

/* Decode OFDM PLCP SIGNAL field RATE sub-field bits 0:2 (labeled R1-R3) into
 * 802.11 MAC rate in 500kbps units
 *
 * Table from 802.11-2012, sec 18.3.4.2.
 */
const uint8 ofdm_rate_lookup[] = {
	DOT11_RATE_48M, /* 8: 48Mbps */
	DOT11_RATE_24M, /* 9: 24Mbps */
	DOT11_RATE_12M, /* A: 12Mbps */
	DOT11_RATE_6M,  /* B:  6Mbps */
	DOT11_RATE_54M, /* C: 54Mbps */
	DOT11_RATE_36M, /* D: 36Mbps */
	DOT11_RATE_18M, /* E: 18Mbps */
	DOT11_RATE_9M   /* F:  9Mbps */
};

/* Modularise and clean up attach functions */
static void wlc_phy_cal_perical_mphase_schedule(phy_info_t *pi, uint delay);
static int wlc_phy_iovar_dispatch_old(phy_info_t *pi, uint32 actionid, void *p, void *a, int vsize,
	int32 int_val, bool bool_val);
static int wlc_phy_iovar_set_dssf(phy_info_t *pi, int32 set_val);
static int wlc_phy_iovar_get_dssf(phy_info_t *pi, int32 *ret_val);
static int wlc_phy_iovar_set_bphymrc(phy_info_t *pi, int32 set_val);
static int wlc_phy_iovar_get_bphymrc(phy_info_t *pi, int32 *get_val);
#if defined(BCMDBG)
static int
wlc_phy_iovar_tempsense_paldosense(phy_info_t *pi, int32 *ret_int_ptr, uint8 tempsense_paldosense);
#endif
static int
wlc_phy_iovar_forcecal(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, int vsize, bool set);
#if defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG)
static int
wlc_phy_iovar_forcecal_obt(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, int vsize, bool set);
#endif 

#if defined(DBG_PHY_IOV)
static int
wlc_phy_aci_nams(phy_info_t *pi, int32 int_val,	int32 *ret_int_ptr, int vsize, bool set);
static int
wlc_phy_dynamic_ml(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, int vsize, bool set);
#endif 
int phy_iovars_wrapper_sample_collect(phy_info_t *pi, uint32 actionid, uint16 type,
		void *p, uint plen, void *a, int alen, int vsize);

#if defined(DBG_PHY_IOV)
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
#endif 

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

	phy_temp_info_t *ti = pi->tempi;
	phy_txcore_temp_t *temp = phy_temp_get_st(ti);

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

				/* If it was pending last time, just restart it */
				if (PHY_PERICAL_MPHASE_PENDING(pi)) {
					/* Delete any existing timer just in case */
					PHY_CAL(("%s: Restarting calibration for 0x%x phase %d\n",
						__FUNCTION__, pi->radio_chanspec,
						pi->cal_info->cal_phase_id));
					wlapi_del_timer(pi->sh->physhim, pi->phycal_timer);
					wlapi_add_timer(pi->sh->physhim, pi->phycal_timer, 0, 0);
				} else if (phy_cache_restore_cal(pi) != BCME_ERROR) {
					break;
				}
			}
			else
#endif /* PHYCAL_CACHING */
			{
				if (PHY_PERICAL_MPHASE_PENDING(pi)) {
					phy_calmgr_mphase_reset(pi->calmgri);
				}

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

		if ((pi->phy_cal_mode == PHY_PERICAL_MPHASE) && PHY_PERICAL_MPHASE_PENDING(pi)) {
			phy_calmgr_mphase_reset(pi->calmgri);
		}

		/* Always do idle TSSI measurement at the end of NPHY cal
		 * while starting/joining a BSS/IBSS
		 */
		pi->first_cal_after_assoc = TRUE;

		if (ISNPHY(pi))
			pi->u.pi_nphy->cal_type_override = PHY_PERICAL_FULL; /* not used in htphy */


		/* Update last cal temp to current tempsense reading */
		if (temp->phycal_tempdelta) {
			phy_temp_get_tempsense_degree(pi->tempi, &pi->cal_info->last_cal_temp);
		}

		/* Attempt cal cache restore if ctx is valid */
#if defined(PHYCAL_CACHING)
		if (ctx)
		{
			PHY_CAL(("wl%d: %s: Attempting to restore cals on JOIN...\n",
				pi->sh->unit, __FUNCTION__));

			if (phy_cache_restore_cal(pi) == BCME_ERROR) {
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
			temp->phycal_tempdelta));

		if (temp->phycal_tempdelta && (ISNPHY(pi) || ISHTPHY(pi) || ISACPHY(pi))) {

			int cal_chanspec = 0;
			phy_temp_get_tempsense_degree(pi->tempi, &current_temp);
			if (ISNPHY(pi)) {
				cal_chanspec = pi->cal_info->u.ncal.txiqlocal_chanspec;
			} else if (ISHTPHY(pi)) {
				cal_chanspec = pi->cal_info->u.htcal.chanspec;
			} else if (ISACPHY(pi)) {
				if (pi->sh->now - pi->cal_info->last_temp_cal_time >=
					pi->sh->glacial_timer) {
					pi->cal_info->last_temp_cal_time = pi->sh->now;
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
				current_temp, delta_temp, temp->phycal_tempdelta,
				cal_chanspec, pi->radio_chanspec));

			delta_time = pi->sh->now - pi->cal_info->last_cal_time;

			/* cal_period = 0 implies only temperature based cals */
			if (((delta_temp < temp->phycal_tempdelta) &&
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
						phy_calmgr_mphase_reset(pi->calmgri);
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
				phy_calmgr_mphase_reset(pi->calmgri);

				if (temp->phycal_tempdelta) {
					current_temp = wlc_phy_tempsense_nphy(pi);
					pi->cal_info->last_cal_temp = current_temp;
				}
			} else if (temp->phycal_tempdelta) {

				current_temp = wlc_phy_tempsense_nphy(pi);

				delta_temp = ((current_temp > pi->cal_info->last_cal_temp) ?
					(current_temp - pi->cal_info->last_cal_temp) :
					(pi->cal_info->last_cal_temp - current_temp));

				if ((delta_temp < (int16)temp->phycal_tempdelta)) {
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

/* ************************************************************* */
/* ************************************************************* */
/*		Functions to be modularized			 */
/* ************************************************************* */
/* ************************************************************* */

/* wlc_awdl non-AC */
void
wlc_phy_cal_mode(wlc_phy_t *ppi, uint mode)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	if (pi->pi_fptr->calibmodes)
		pi->pi_fptr->calibmodes(pi, mode);
}


uint32
wlc_phy_cap_get(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t *)pih;
	uint32	cap = 0;

	switch (pi->pubpi->phy_type) {
	case PHY_TYPE_N:
		cap |= PHY_CAP_40MHZ;
		cap |= (PHY_CAP_SGI | PHY_CAP_STBC);
		break;

	case PHY_TYPE_HT:
		cap |= (PHY_CAP_40MHZ | PHY_CAP_SGI | PHY_CAP_STBC | PHY_CAP_LDPC);
		break;

#if (ACCONF || ACCONF2)
	case PHY_TYPE_AC:
		cap |= wlc_phy_ac_caps(pi);
		break;
#endif /* ACCONF || ACCONF2 */

	case PHY_TYPE_LCN20:
	/*                 A0     non-A0
	 * FW default:     OFF    ON
	 * nvram: ldpc=0   OFF    OFF
	 * nvram: ldpc=1   ON     ON
	 * nvram: ldpc=2   ON     OFF
	 * nvram: ldpc=3   OFF    ON
	 */

#if (defined(LCN20CONF) && (LCN20CONF != 0))
#ifdef WL11ULB
		cap |= (wlc_phy_lcn20_ulb_10_capable(pi) ? PHY_CAP_10MHZ : 0);
		cap |= (wlc_phy_lcn20_ulb_5_capable(pi) ? PHY_CAP_5MHZ : 0);
#endif /* WL11ULB */
#endif /* (defined(LCN20CONF) && (LCN20CONF != 0)) */

		if  ((pi->ldpc_en == 1) ||
		     ((CHIPREV(pi->sh->chiprev) == 0) && (pi->ldpc_en == 2)) ||
		     (!(CHIPREV(pi->sh->chiprev) == 0) && (pi->ldpc_en == 3)))
			cap |= (PHY_CAP_HT_PROP_RATES | PHY_CAP_SGI | PHY_CAP_STBC | PHY_CAP_LDPC);
		else
			cap |= (PHY_CAP_HT_PROP_RATES | PHY_CAP_SGI | PHY_CAP_STBC);
		break;

	default:
		break;
	}
	return cap;
}

const uint8 *
BCMRAMFN(wlc_phy_get_ofdm_rate_lookup)(void)
{
	return ofdm_rate_lookup;
}

/* ************************************************************* */
/* ************************************************************* */
/*		Functions shared by HT and N PHY		 */
/* ************************************************************* */
/* ************************************************************* */
int
wlc_phy_acimode_noisemode_reset(wlc_phy_t *pih, chanspec_t chanspec,
	bool clear_aci_state, bool clear_noise_state, bool disassoc)
{
	phy_info_t *pi = (phy_info_t *)pih;
	uint channel = CHSPEC_CHANNEL(chanspec);

	pi->interf->cca_stats_func_called = FALSE;

	if (!(ISNPHY(pi) || ISHTPHY(pi)))
		return BCME_UNSUPPORTED;

	if (pi->sh->interference_mode_override == TRUE)
		return BCME_NOTREADY;

	PHY_TRACE(("%s: CurCh %d HomeCh %d disassoc %d\n",
		__FUNCTION__, channel,
		pi->interf->curr_home_channel, disassoc));

	if ((disassoc) ||
		((channel != pi->interf->curr_home_channel) &&
		(disassoc == FALSE))) {
		/* not home channel... reset */
		if (ISNPHY(pi)) {
#ifndef WLC_DISABLE_ACI
			wlc_phy_aci_noise_reset_nphy(pi, channel,
				clear_aci_state, clear_noise_state, disassoc);
#endif /* Compiling out ACI code for 4324 */
		} else if (ISHTPHY(pi)) {
			wlc_phy_aci_noise_reset_htphy(pi, channel,
				clear_aci_state, clear_noise_state, disassoc);
		}
	}

	return BCME_OK;
}

int
wlc_phy_interference_set(wlc_phy_t *pih, bool init)
{
	int wanted_mode;
	phy_info_t *pi = (phy_info_t *)pih;

	if (!(ISNPHY(pi) || ISHTPHY(pi)))
		return BCME_UNSUPPORTED;

	if (pi->sh->interference_mode_override == TRUE) {
		/* keep the same values */
#ifdef BAND5G
		if (CHSPEC_IS5G(pi->radio_chanspec)) {
			if (pi->sh->interference_mode_5G_override == 0 ||
				pi->sh->interference_mode_5G_override == 1) {
				wanted_mode = pi->sh->interference_mode_5G_override;
			} else {
				wanted_mode = 0;
			}
		} else
#endif /* BAND5G */
		{
			wanted_mode = pi->sh->interference_mode_2G_override;
		}
	} else {

#ifdef BAND5G
		if (CHSPEC_IS5G(pi->radio_chanspec)) {
			wanted_mode = pi->sh->interference_mode_5G;
		} else
#endif /* BAND5G */
		{
			wanted_mode = pi->sh->interference_mode_2G;
		}
	}

	if (CHSPEC_CHANNEL(pi->radio_chanspec) != pi->interf->curr_home_channel) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);

#ifndef WLC_DISABLE_ACI
		phy_noise_set_mode(pi->noisei, wanted_mode, init);
#endif
		pi->sh->interference_mode = wanted_mode;

		wlapi_enable_mac(pi->sh->physhim);
	}

	return BCME_OK;
}

/* ************************************************************* */
/* ************************************************************* */

void
wlc_phy_trigger_cals_for_btc_adjust(phy_info_t *pi)
{
	phy_calmgr_mphase_reset(pi->calmgri);
	if (ISNPHY(pi)) {
		pi->u.pi_nphy->cal_type_override = PHY_PERICAL_FULL;
	}
	wlc_phy_cal_perical_mphase_schedule(pi, PHY_PERICAL_NODELAY);
}


#if defined(BCMDBG) || defined(BCMDBG_TEMPSENSE)
static int
wlc_phy_iovar_tempsense_paldosense(phy_info_t *pi, int32 *ret_int_ptr, uint8 tempsense_paldosense)
{
	int err = BCME_OK;

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
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	} else if (ISLCN20PHY(pi)) {
		int32 int_val = wlc_lcn20phy_tempsense(pi, TEMPER_VBAT_TRIGGER_NEW_MEAS);

		bcopy(&int_val, ret_int_ptr, sizeof(int_val));
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
	} else
		err = BCME_UNSUPPORTED;

	return err;
}
#endif	/* defined(BCMDBG) || defined(WLTEST) || defined(ATE_BUILD) ||
		* defined(BCMDBG_TEMPSENSE)
		*/


static int
wlc_phy_iovar_forcecal(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, int vsize, bool set)
{
	int err = BCME_OK;

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
		phy_calmgr_mphase_reset(pi->calmgri);
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
		phy_ac_rxgcrs_clean_noise_array(pi->u.pi_acphy->rxgcrsi);

		/* call sphase or schedule mphase cal */
		phy_calmgr_mphase_reset(pi->calmgri);
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
			phy_calmgr_mphase_reset(pi->calmgri);
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

	} else if (ISLCN20PHY(pi)) {
		pi->pi_fptr->calibmodes(pi, PHY_FULLCAL);
	}

	return err;
}

#if defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG)
static int
wlc_phy_iovar_forcecal_obt(phy_info_t *pi, int32 int_val, int32 *ret_int_ptr, int vsize, bool set)
{
	int err = BCME_OK;
	uint8 wait_ctr = 0;
	int val = 1;
	if (ISNPHY(pi)) {
			phy_calmgr_mphase_reset(pi->calmgri);

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
#endif 


int
wlc_phy_iovar_txpwrindex_set(phy_info_t *pi, void *p)
{
	int err = BCME_OK;
	uint32 txpwridx[PHY_CORE_MAX] = { 0x30 };
	int8 idx, core;
	phy_info_nphy_t *pi_nphy = NULL;

	if (ISNPHY(pi))
		pi_nphy = pi->u.pi_nphy;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	bcopy(p, txpwridx, PHY_CORE_MAX * sizeof(uint32));

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
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	} else if (ISLCN20PHY(pi)) {
		int8 siso_int_val = (int8)(txpwridx[0] & 0xff);

		wlc_lcn20phy_set_tx_pwr_by_index(pi, siso_int_val);
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
	}

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	return err;
}


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

static int
wlc_phy_iovar_set_bphymrc(phy_info_t *pi, int32 set_val)
{
	if (ISACPHY(pi) && (ACPHY_bphymrcCtrl(pi->pubpi->phy_rev) != INVALID_ADDRESS)) {
	pi->sh->bphymrc_en = (uint8) set_val;
	return BCME_OK;
	}
	return BCME_UNSUPPORTED;
}

static int
wlc_phy_iovar_get_bphymrc(phy_info_t *pi, int32 *ret_val)
{
	if (ISACPHY(pi) && (ACPHY_bphymrcCtrl(pi->pubpi->phy_rev) != INVALID_ADDRESS)) {
		*ret_val = pi->sh->bphymrc_en;
		return BCME_OK;
	}
	return BCME_UNSUPPORTED;
}

#if defined(DBG_PHY_IOV) || defined(BCMDBG)
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
#endif 

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
		phy_utils_pmu_regcontrol_access(pi, 5, &reg_value, 0);
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
			phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);

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

			phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
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

#endif /* NCONF */

	default:
		err = BCME_UNSUPPORTED;
	}

	return err;
}

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
#include <wlc_patch.h>

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
#if defined(BCMDBG)
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

#endif 
#if defined(BCMDBG) || defined(PHYCAL_CHNG_CS)
	case IOV_GVAL(IOV_GLACIAL_TIMER):
		*ret_int_ptr = (int32)pi->sh->glacial_timer;
		break;

	case IOV_SVAL(IOV_GLACIAL_TIMER):
		pi->sh->glacial_timer = (uint32)int_val;
		break;
#endif 
#if defined(DBG_PHY_IOV)
	case IOV_GVAL(IOV_PHY_WATCHDOG):
		*ret_int_ptr = (int32)pi->phywatchdog_override;
		break;

	case IOV_SVAL(IOV_PHY_WATCHDOG):
		phy_wd_override(pi, bool_val);
		break;
#endif 
	case IOV_GVAL(IOV_CAL_PERIOD):
	        *ret_int_ptr = (int32)pi->cal_period;
	        break;

	case IOV_SVAL(IOV_CAL_PERIOD):
	        pi->cal_period = (uint32)int_val;
	        break;
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
#if defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG)
	case IOV_GVAL(IOV_PHY_FORCECAL_OBT):
		err = wlc_phy_iovar_forcecal_obt(pi, int_val, ret_int_ptr, vsize, FALSE);
		break;

	case IOV_SVAL(IOV_PHY_FORCECAL_OBT):
		err = wlc_phy_iovar_forcecal_obt(pi, int_val, ret_int_ptr, vsize, FALSE);
		break;
#endif 
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

	case IOV_GVAL(IOV_PHY_BPHYMRC):
		err = wlc_phy_iovar_get_bphymrc(pi, ret_int_ptr);
		break;

	case IOV_SVAL(IOV_PHY_BPHYMRC):
		err = wlc_phy_iovar_set_bphymrc(pi, int_val);
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
#if defined(BCMDBG)
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
#endif 
#endif /* WL_SARLIMIT */
#if defined(DBG_PHY_IOV) || defined(BCMDBG)
		case IOV_SVAL(IOV_PAPD_EPS_OFFSET): {
			wlc_phy_iovar_set_papd_offset(pi, (int16)int_val);
			break;
		}

		case IOV_GVAL(IOV_PAPD_EPS_OFFSET): {
			*ret_int_ptr = wlc_phy_iovar_get_papd_offset(pi);
			break;
		}
#endif 

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

#if defined(BCMDBG) || defined(BCMDBG_TEMPSENSE)
	case IOV_GVAL(IOV_PHY_TEMPSENSE):
		err = wlc_phy_iovar_tempsense_paldosense(pi, ret_int_ptr, 0);
		break;
#endif 
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
	case IOV_GVAL(IOV_PHY_PAPD_DEBUG):
		break;

	case IOV_GVAL(IOV_NOISE_MEASURE):
		int_val = 0;
		bcopy(&int_val, a, sizeof(int_val));
		break;


	case IOV_GVAL(IOV_SROM_REV): {
			*ret_int_ptr = pi->sh->sromrev;
	}
	break;

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
#if defined(DBG_PHY_IOV)
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
#endif 

#if defined(BCMDBG)
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
#endif 

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
