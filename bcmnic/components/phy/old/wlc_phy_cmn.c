/*
 * PHY and RADIO specific portion of Broadcom BCM43XX 802.11 Networking Device Driver.
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
 * $Id: wlc_phy_cmn.c 649330 2016-07-15 16:17:13Z mvermeid $
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <qmath.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <bitfuncs.h>
#include <bcmdevs.h>
#include <bcmnvram.h>
#include <proto/802.11.h>
#include <sbchipc.h>
#include <hndpmu.h>
#include <bcmsrom_fmt.h>
#include <sbsprom.h>
#include <bcmutils.h>
#include <d11.h>
#include <phy_mem.h>

/* *********************************************** */
#include <phy_calmgr.h>
#include <phy_dbg.h>
#include <phy_tpc.h>
#include <phy_noise.h>
#include <phy_antdiv_api.h>
#include <phy_tpc_api.h>
#include <phy_radar.h>
#include <phy_temp.h>
#include <phy_misc.h>
#include <phy_ac_rxiqcal.h>
#include <phy_lcn40_rssi.h>
#include <phy_ac_rssi.h>
#include <phy_ac_tpc.h>
/* *********************************************** */

#include <wlc_phy_hal.h>
#include <wlc_phy_int.h>
#include <wlc_phyreg_n.h>
#include <wlc_phyreg_ht.h>
#include <wlc_phyreg_ac.h>
#include <wlc_phyreg_lcn.h>  /* lcn40 needs this */
#include <wlc_phyreg_lcn40.h>
#include <wlc_phytbl_n.h>
#include <wlc_phytbl_ht.h>
#include <wlc_phytbl_ac.h>
#include <wlc_phytbl_20691.h>
#include <wlc_phytbl_20694.h>
#include <wlc_phytbl_20693.h>
#include <wlc_phy_radio.h>
#if (defined(LCN20CONF) && (LCN20CONF != 0))
#include <wlc_phy_lcn20.h>
#include <wlc_phyreg_lcn20.h>
#endif /* #if ((defined(LCN20CONF) && (LCN20CONF != 0))) */
#include <wlc_phy_lcn40.h>
#include <wlc_phy_n.h>
#include <wlc_phy_ht.h>
#if (ACCONF != 0) || (ACCONF2 != 0)
#include <phy_ac_info.h>
#endif /* (ACCONF != 0) || (ACCONF2 != 0) */
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
#include <phy_ac_rxgcrs.h>
#if defined(PHYCAL_CACHING)
#include <phy_cache.h>
#endif
#include <phy_rstr.h>
/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/*  macro, typedef, enum, structure, global variable		*/
/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/* channel info structure */
typedef struct _chan_info_basic {
	uint16	chan;		/* channel number */
	uint16	freq;		/* in Mhz */
} chan_info_basic_t;

uint16 ltrn_list[PHY_LTRN_LIST_LEN] = {
	0x18f9, 0x0d01, 0x00e4, 0xdef4, 0x06f1, 0x0ffc,
	0xfa27, 0x1dff, 0x10f0, 0x0918, 0xf20a, 0xe010,
	0x1417, 0x1104, 0xf114, 0xf2fa, 0xf7db, 0xe2fc,
	0xe1fb, 0x13ee, 0xff0d, 0xe91c, 0x171a, 0x0318,
	0xda00, 0x03e8, 0x17e6, 0xe9e4, 0xfff3, 0x1312,
	0xe105, 0xe204, 0xf725, 0xf206, 0xf1ec, 0x11fc,
	0x14e9, 0xe0f0, 0xf2f6, 0x09e8, 0x1010, 0x1d01,
	0xfad9, 0x0f04, 0x060f, 0xde0c, 0x001c, 0x0dff,
	0x1807, 0xf61a, 0xe40e, 0x0f16, 0x05f9, 0x18ec,
	0x0a1b, 0xff1e, 0x2600, 0xffe2, 0x0ae5, 0x1814,
	0x0507, 0x0fea, 0xe4f2, 0xf6e6
};

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

#ifdef LP_P2P_SOFTAP
uint8 pwr_lvl_qdB[LCNPHY_TX_PWR_CTRL_MACLUT_LEN];
#endif /* LP_P2P_SOFTAP */

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/*  local prototype						*/
/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

/* %%%%%% major operations */

/* %%%%%% Calibration, ACI, noise/rssi measurement */
#ifndef WLC_DISABLE_ACI
static void wlc_phy_noisemode_upd(phy_info_t *pi);
static int8 wlc_phy_cmn_noisemode_glitch_chk_adj(phy_info_t *pi, uint16 glitch_badplcp_sum_ma,
	noise_thresholds_t *thresholds);
static void wlc_phy_cmn_noise_limit_desense(phy_info_t *pi);
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(WL_PHYACIARGS)
static int  wlc_phy_aci_args(phy_info_t *pi, wl_aci_args_t *params, bool get, int len);
#endif /* defined (BCMINTERNAL) || defined (WLTEST) || defined (WL_PHYACIARGS) */
static void wlc_phy_aci_update_ma(phy_info_t *pi);
void wlc_phy_aci_noise_reset_nphy(phy_info_t *pi, uint channel, bool clear_aci_state,
	bool clear_noise_state, bool disassoc);
#endif /* Compiling out ACI code for 4324 */
void wlc_phy_aci_noise_reset_htphy(phy_info_t *pi, uint channel, bool clear_aci_state,
	bool clear_noise_state, bool disassoc);
void wlc_phy_rxgainctrl_gainctrl_acphy_tiny(phy_info_t *pi, uint8 init_desense);

/* %%%%%% testing */
#if defined(BCMDBG) || defined(WLTEST)
static int wlc_phy_test_carrier_suppress(phy_info_t *pi, int channel);
static int wlc_phy_test_evm(phy_info_t *pi, int channel, uint rate, int txpwr);
#endif


void
BCMATTACHFN(wlc_phy_txpwr_srom11_read_pwrdet)(phy_info_t *pi, srom11_pwrdet_t * pwrdet,
	uint8 param, uint8 band, uint8 offset,  const char * name);

#ifdef WLC_TXCAL
int
wlc_phy_txcal_generate_estpwr_lut(wl_txcal_power_tssi_t *txcal_pwr_tssi,
	uint16 *estpwr, uint8 core);
int
wlc_phy_txcal_apply_pwr_tssi_tbl(phy_info_t *pi, wl_txcal_power_tssi_t *txcal_pwr_tssi);
int
wlc_phy_txcal_apply_pa_params(phy_info_t *pi);
#endif /* WLC_TXCAL */

#define PHY_TXFIFO_END_BLK_REV35	(0x7900 >> 2)

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/*  function implementation                                     */
/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/* returns a pointer to per interface instance data */
shared_phy_t *
BCMATTACHFN(wlc_phy_shared_attach)(shared_phy_params_t *shp)
{
	shared_phy_t *sh;
	int ref_count = 0;

#ifdef EVENT_LOG_COMPILE
	/* First thing to do.. initialize the PHY_ERROR tag's attributes. */
	/* This is the attach function for the PHY component. */
	event_log_tag_start(EVENT_LOG_TAG_PHY_ERROR, EVENT_LOG_SET_ERROR,
		EVENT_LOG_TAG_FLAG_LOG);
#endif

	/* allocate wlc_info_t state structure */
	if ((sh = (shared_phy_t*) MALLOCZ(shp->osh, sizeof(shared_phy_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
			shp->unit, __FUNCTION__, MALLOCED(shp->osh)));
		goto fail;
	}

	/* OBJECT REGISTRY: check if shared key has value already stored */
	sh->sromi = (phy_srom_info_t *)wlapi_obj_registry_get(shp->physhim, OBJR_PHY_CMN_SROM_INFO);
	if (sh->sromi == NULL) {
		if ((sh->sromi = (phy_srom_info_t *)MALLOCZ(shp->osh,
			sizeof(phy_srom_info_t))) == NULL) {

			PHY_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
				shp->unit, __FUNCTION__, MALLOCED(shp->osh)));
			goto fail;
		}

		/* OBJECT REGISTRY: We are the first instance, store value for key */
		wlapi_obj_registry_set(shp->physhim, OBJR_PHY_CMN_SROM_INFO, sh->sromi);
	}
	/* OBJECT REGISTRY: Reference the stored value in both instances */
	ref_count = wlapi_obj_registry_ref(shp->physhim, OBJR_PHY_CMN_SROM_INFO);
	ASSERT(ref_count <= MAX_RSDB_MAC_NUM);
	BCM_REFERENCE(ref_count);

	sh->osh		= shp->osh;
	sh->sih		= shp->sih;
	sh->physhim	= shp->physhim;
	sh->unit	= shp->unit;
	sh->corerev	= shp->corerev;
	sh->vid		= shp->vid;
	sh->did		= shp->did;
	sh->chip	= shp->chip;
	sh->chiprev	= shp->chiprev;
	sh->chippkg	= shp->chippkg;
	sh->sromrev	= shp->sromrev;
	sh->boardtype	= shp->boardtype;
	sh->boardrev	= shp->boardrev;
	sh->boardvendor	= shp->boardvendor;
	sh->boardflags	= shp->boardflags;
	sh->boardflags2	= shp->boardflags2;
	sh->boardflags4	= shp->boardflags4;
	sh->bustype	= shp->bustype;
	sh->buscorerev	= shp->buscorerev;
	strncpy(sh->vars_table_accessor, shp->vars_table_accessor, sizeof(sh->vars_table_accessor));
	sh->vars_table_accessor[sizeof(sh->vars_table_accessor)-1] = '\0';

	/* create our timers */
	sh->fast_timer	= PHY_SW_TIMER_FAST;
	sh->slow_timer	= PHY_SW_TIMER_SLOW;
	sh->glacial_timer = PHY_SW_TIMER_GLACIAL;

	/* reset cal scheduler */
	sh->scheduled_cal_time = 0;

	/* ACI mitigation mode is auto by default */
	sh->interference_mode = WLAN_AUTO;
	/* sh->snr_mode = SNR_ANT_MERGE_MAX; */
	sh->rssi_mode = RSSI_ANT_MERGE_MAX;

	return sh;
fail:
	wlc_phy_shared_detach(sh);
	return NULL;
}

void
BCMATTACHFN(wlc_phy_shared_detach)(shared_phy_t *sh)
{
	if (sh != NULL) {
		/* phy_head must have been all detached */
		if (sh->phy_head) {
			PHY_ERROR(("wl%d: %s non NULL phy_head\n", sh->unit, __FUNCTION__));
			ASSERT(!sh->phy_head);
		}
		if (sh->sromi != NULL) {
			if (wlapi_obj_registry_unref(sh->physhim, OBJR_PHY_CMN_SROM_INFO) == 0) {
				wlapi_obj_registry_set(sh->physhim, OBJR_PHY_CMN_SROM_INFO, NULL);
				MFREE(sh->osh, sh->sromi, sizeof(phy_srom_info_t));
			}
		}
		MFREE(sh->osh, sh, sizeof(shared_phy_t));
	}
}

static const char BCMATTACHDATA(rstr_phycal)[] = "phycal";

/*
 * Read the phy calibration temperature delta parameters from NVRAM.
 */
void
BCMATTACHFN(wlc_phy_read_tempdelta_settings)(phy_info_t *pi, int maxtempdelta)
{
	/* Read the temperature delta from NVRAM */
	pi->phycal_tempdelta = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_phycal_tempdelta, 0);

	/* if nvram entry is bogus */
	if (pi->phycal_tempdelta == 0) {
	    if (ISACPHY(pi))
		pi->phycal_tempdelta_default = ACPHY_DEFAULT_CAL_TEMPDELTA;
	    else
		pi->phycal_tempdelta_default = pi->phycal_tempdelta;
	}

	/* Range check, disable if incorrect configuration parameter */
	/* Preserve default, in case someone wants to use it. */
	if (pi->phycal_tempdelta > maxtempdelta)
		pi->phycal_tempdelta = pi->phycal_tempdelta_default;
	else
		pi->phycal_tempdelta_default = pi->phycal_tempdelta;
}

/* Break a lengthy algorithm into smaller pieces using 0-length timer */
void
wlc_phy_timercb_phycal(phy_info_t *pi)
{
	phy_info_nphy_t *pi_nphy = pi->u.pi_nphy;
	uint delay_val = pi->phy_cal_delay;
#if defined(PHYCAL_CACHING)
	ch_calcache_t *ctx = NULL;
	ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);
#endif

	/* Increase delay between phases to be longer than 2 video frames interval 16.7*2 */
#if !defined(PHYCAL_SPLIT_4324x) && !defined(WFD_PHY_LL)
	if (CHIPID(pi->sh->chip) == BCM43237_CHIP_ID)
#endif
		delay_val = 40;

	if (PHY_PERICAL_MPHASE_PENDING(pi)) {

		PHY_CAL(("wlc_phy_timercb_phycal: phase_id %d\n", pi->cal_info->cal_phase_id));

		if (!pi->sh->up) {
			wlc_phy_cal_perical_mphase_reset(pi);
			return;
		}

		if (SCAN_RM_IN_PROGRESS(pi) || PLT_INPROG_PHY(pi) || PHY_MUTED(pi)) {
			/* delay percal until scan completed */
			PHY_CAL(("wlc_phy_timercb_phycal: scan in progress, delay 1 sec\n"));
			delay_val = 1000;	/* delay 1 sec */
			/* PHYCAL_CACHING does not interact with mphase */
#if defined(PHYCAL_CACHING)
			if (!ctx)
#endif
				wlc_phy_cal_perical_mphase_restart(pi);
		} else {
			phy_calmgr_cals(pi, PHY_PERICAL_AUTO, pi->cal_info->cal_searchmode);
		}

		if (ISNPHY(pi)) {
			if (!(pi_nphy->ntd_papdcal_dcs == TRUE &&
				pi->cal_info->cal_phase_id == MPHASE_CAL_STATE_RXCAL))
				wlapi_add_timer(pi->sh->physhim, pi->phycal_timer, delay_val, 0);
			else {
				pi_nphy->ntd_papdcal_dcs = FALSE;
				wlc_phy_cal_perical_mphase_reset(pi);
			}
		} else {
			if (!pi->cal_info->cal_phase_id == MPHASE_CAL_STATE_IDLE) {
				wlapi_add_timer(pi->sh->physhim, pi->phycal_timer, delay_val, 0);
			} else {
				wlc_phy_cal_perical_mphase_reset(pi);
			}
		}
		return;
	}

	PHY_CAL(("wlc_phy_timercb_phycal: mphase phycal is done\n"));
}

void
WLBANDINITFN(wlc_phy_por_inform)(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	pi->phy_init_por = TRUE;
}

void
wlc_phy_btclock_war(wlc_phy_t *ppi, bool enable)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	pi->btclock_tune = enable;
}

void
wlc_phy_preamble_override_set(wlc_phy_t *ppi, int8 val)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	if ((ISACPHY(pi)) && (val != WLC_N_PREAMBLE_MIXEDMODE)) {
		PHY_ERROR(("wl%d:%s: AC Phy: Ignore request to set preamble mode %d\n",
			pi->sh->unit, __FUNCTION__, val));
		return;
	}

	pi->n_preamble_override = val;
}

int8
wlc_phy_preamble_override_get(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	return pi->n_preamble_override;
}

/* increase the threshold to avoid false edcrs detection, non-11n only */
void
wlc_phy_edcrs_lock(wlc_phy_t *pih, bool lock)
{
	phy_info_t *pi = (phy_info_t *)pih;
	pi->edcrs_threshold_lock = lock;

	/* assertion: -59dB, deassertion: -67dB */
	PHY_REG_LIST_START
		PHY_REG_WRITE_ENTRY(NPHY, ed_crs20UAssertThresh0, 0x46b)
		PHY_REG_WRITE_ENTRY(NPHY, ed_crs20UAssertThresh1, 0x46b)
		PHY_REG_WRITE_ENTRY(NPHY, ed_crs20UDeassertThresh0, 0x3c0)
		PHY_REG_WRITE_ENTRY(NPHY, ed_crs20UDeassertThresh1, 0x3c0)
	PHY_REG_LIST_EXECUTE(pi);
}

void
wlc_phy_initcal_enable(wlc_phy_t *pih, bool initcal)
{
	phy_info_t *pi = (phy_info_t *)pih;
	if (ISNPHY(pi))
		pi->u.pi_nphy->do_initcal = initcal;
}

void
wlc_phy_hw_clk_state_upd(wlc_phy_t *pih, bool newstate)
{
	phy_info_t *pi = (phy_info_t *)pih;

	if (!pi || !pi->sh)
		return;

	pi->sh->clk = newstate;
}

void
wlc_phy_hw_state_upd(wlc_phy_t *pih, bool newstate)
{
	phy_info_t *pi = (phy_info_t *)pih;

	if (!pi || !pi->sh)
		return;

	pi->sh->up = newstate;
}

#ifdef WFD_PHY_LL
void
wlc_phy_wfdll_chan_active(wlc_phy_t * ppi, bool chan_active)
{
	phy_info_t *pi = (phy_info_t*)ppi;

	if (ISNPHY(pi) || ISACPHY(pi))
		pi->wfd_ll_chan_active = chan_active;
}
#endif /* WFD_PHY_LL */

/*
 * Do one-time phy initializations and calibration.
 *
 * Note: no register accesses allowed; we have not yet waited for PLL
 * since the last corereset.
 */
void
BCMINITFN(wlc_phy_cal_init)(wlc_phy_t *pih)
{
	int i;
	phy_info_t *pi = (phy_info_t *)pih;
	initfn_t cal_init = NULL;

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	ASSERT((R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC) == 0);

	if (!pi->initialized) {
		/* glitch counter init */
		/* detection is called only if high glitches are observed */
		pi->interf->aci.glitch_ma = ACI_INIT_MA;
		pi->interf->aci.glitch_ma_previous = ACI_INIT_MA;
		pi->interf->aci.pre_glitch_cnt = 0;
		if ((ISNPHY(pi) && NREV_LE(pi->pubpi->phy_rev, 15)) || ISHTPHY(pi) || ISACPHY(pi)) {
			pi->interf->aci.ma_total = PHY_NOISE_MA_WINDOW_SZ * ACI_INIT_MA;
		} else {
			pi->interf->aci.ma_total = MA_WINDOW_SZ * ACI_INIT_MA;
		}
		for (i = 0; i < MA_WINDOW_SZ; i++)
			pi->interf->aci.ma_list[i] = ACI_INIT_MA;

		for (i = 0; i < PHY_NOISE_MA_WINDOW_SZ; i++) {
			pi->interf->noise.ofdm_glitch_ma_list[i] = PHY_NOISE_GLITCH_INIT_MA;
			pi->interf->noise.ofdm_badplcp_ma_list[i] =
				PHY_NOISE_GLITCH_INIT_MA_BADPlCP;
			pi->interf->noise.bphy_glitch_ma_list[i] = PHY_NOISE_GLITCH_INIT_MA;
			pi->interf->noise.bphy_badplcp_ma_list[i] =
				PHY_NOISE_GLITCH_INIT_MA_BADPlCP;
		}

		pi->interf->noise.ofdm_glitch_ma = PHY_NOISE_GLITCH_INIT_MA;
		pi->interf->noise.bphy_glitch_ma = PHY_NOISE_GLITCH_INIT_MA;
		pi->interf->noise.ofdm_glitch_ma_previous = PHY_NOISE_GLITCH_INIT_MA;
		pi->interf->noise.bphy_glitch_ma_previous = PHY_NOISE_GLITCH_INIT_MA;
		pi->interf->noise.bphy_pre_glitch_cnt = 0;
		pi->interf->noise.ofdm_ma_total = PHY_NOISE_GLITCH_INIT_MA * PHY_NOISE_MA_WINDOW_SZ;
		pi->interf->noise.bphy_ma_total = PHY_NOISE_GLITCH_INIT_MA * PHY_NOISE_MA_WINDOW_SZ;

		pi->interf->badplcp_ma = PHY_NOISE_GLITCH_INIT_MA_BADPlCP;
		pi->interf->badplcp_ma_previous = PHY_NOISE_GLITCH_INIT_MA_BADPlCP;
		if ((ISNPHY(pi) && NREV_LE(pi->pubpi->phy_rev, 15)) || ISHTPHY(pi) || ISACPHY(pi)) {
			pi->interf->badplcp_ma_total = PHY_NOISE_GLITCH_INIT_MA_BADPlCP *
			        PHY_NOISE_MA_WINDOW_SZ;
		} else {
			pi->interf->badplcp_ma_total = PHY_NOISE_GLITCH_INIT_MA_BADPlCP *
			        MA_WINDOW_SZ;
		}
		pi->interf->pre_badplcp_cnt = 0;
		pi->interf->bphy_pre_badplcp_cnt = 0;

		pi->interf->noise.ofdm_badplcp_ma = PHY_NOISE_GLITCH_INIT_MA_BADPlCP;
		pi->interf->noise.bphy_badplcp_ma = PHY_NOISE_GLITCH_INIT_MA_BADPlCP;

		pi->interf->noise.ofdm_badplcp_ma_previous = PHY_NOISE_GLITCH_INIT_MA_BADPlCP;
		pi->interf->noise.bphy_badplcp_ma_previous = PHY_NOISE_GLITCH_INIT_MA_BADPlCP;
		pi->interf->noise.ofdm_badplcp_ma_total =
			PHY_NOISE_GLITCH_INIT_MA_BADPlCP * PHY_NOISE_MA_WINDOW_SZ;
		pi->interf->noise.bphy_badplcp_ma_total =
			PHY_NOISE_GLITCH_INIT_MA_BADPlCP * PHY_NOISE_MA_WINDOW_SZ;

		pi->interf->noise.ofdm_badplcp_ma_index = 0;
		pi->interf->noise.bphy_badplcp_ma_index = 0;

		pi->interf->cca_stats_func_called = FALSE;
		pi->interf->cca_stats_total_glitch = 0;
		pi->interf->cca_stats_bphy_glitch = 0;
		pi->interf->cca_stats_total_badplcp = 0;
		pi->interf->cca_stats_bphy_badplcp = 0;
		pi->interf->cca_stats_mbsstime = 0;

		cal_init = pi->pi_fptr->calinit;
		if (cal_init)
			(*cal_init)(pi);

		pi->initialized = TRUE;
	}
}

#ifndef WLC_DISABLE_ACI
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(WL_PHYACIARGS)
static int
wlc_phy_aci_args(phy_info_t *pi, wl_aci_args_t *params, bool get, int len)
{
	if (len != WL_ACI_ARGS_LEGACY_LENGTH && len != sizeof(wl_aci_args_t))
		return BCME_BUFTOOSHORT;

	if (get) {
		params->enter_aci_thresh = pi->interf->aci.enter_thresh;
		params->exit_aci_thresh = pi->interf->aci.exit_thresh;
		params->usec_spin = pi->interf->aci.usec_spintime;
		params->glitch_delay = pi->interf->aci.glitch_delay;
	} else {
		if (params->enter_aci_thresh > 0)
			pi->interf->aci.enter_thresh = params->enter_aci_thresh;
		if (params->exit_aci_thresh > 0)
			pi->interf->aci.exit_thresh = params->exit_aci_thresh;
		if (params->usec_spin > 0)
			pi->interf->aci.usec_spintime = params->usec_spin;
		if (params->glitch_delay > 0)
			pi->interf->aci.glitch_delay = params->glitch_delay;
	}

	if (len == sizeof(wl_aci_args_t)) {
		if (!ISNPHY(pi) && !ISHTPHY(pi))
			return BCME_UNSUPPORTED;

		ASSERT(pi->interf->aci.nphy != NULL);

		if (get) {
			params->nphy_adcpwr_enter_thresh =
				pi->interf->aci.nphy->adcpwr_enter_thresh;
			params->nphy_adcpwr_exit_thresh = pi->interf->aci.nphy->adcpwr_exit_thresh;
			params->nphy_repeat_ctr = pi->interf->aci.nphy->detect_repeat_ctr;
			params->nphy_num_samples = pi->interf->aci.nphy->detect_num_samples;
			params->nphy_undetect_window_sz = pi->interf->aci.nphy->undetect_window_sz;
			params->nphy_b_energy_lo_aci = pi->interf->aci.nphy->b_energy_lo_aci;
			params->nphy_b_energy_md_aci = pi->interf->aci.nphy->b_energy_md_aci;
			params->nphy_b_energy_hi_aci = pi->interf->aci.nphy->b_energy_hi_aci;

			params->nphy_noise_noassoc_glitch_th_up =
				pi->interf->noise.nphy_noise_noassoc_glitch_th_up;
			params->nphy_noise_noassoc_glitch_th_dn =
				pi->interf->noise.nphy_noise_noassoc_glitch_th_dn;
			params->nphy_noise_assoc_glitch_th_up =
				pi->interf->noise.nphy_noise_assoc_glitch_th_up;
			params->nphy_noise_assoc_glitch_th_dn =
				pi->interf->noise.nphy_noise_assoc_glitch_th_dn;
			params->nphy_noise_assoc_aci_glitch_th_up =
				pi->interf->noise.nphy_noise_assoc_aci_glitch_th_up;
			params->nphy_noise_assoc_aci_glitch_th_dn =
				pi->interf->noise.nphy_noise_assoc_aci_glitch_th_dn;
			params->nphy_noise_assoc_enter_th =
				pi->interf->noise.nphy_noise_assoc_enter_th;
			params->nphy_noise_noassoc_enter_th =
				pi->interf->noise.nphy_noise_noassoc_enter_th;
			params->nphy_noise_assoc_rx_glitch_badplcp_enter_th=
				pi->interf->noise.nphy_noise_assoc_rx_glitch_badplcp_enter_th;
			params->nphy_noise_noassoc_crsidx_incr=
				pi->interf->noise.nphy_noise_noassoc_crsidx_incr;
			params->nphy_noise_assoc_crsidx_incr=
				pi->interf->noise.nphy_noise_assoc_crsidx_incr;
			params->nphy_noise_crsidx_decr=
				pi->interf->noise.nphy_noise_crsidx_decr;

		} else {
			pi->interf->aci.nphy->adcpwr_enter_thresh =
				params->nphy_adcpwr_enter_thresh;
			pi->interf->aci.nphy->adcpwr_exit_thresh = params->nphy_adcpwr_exit_thresh;
			pi->interf->aci.nphy->detect_repeat_ctr = params->nphy_repeat_ctr;
			pi->interf->aci.nphy->detect_num_samples = params->nphy_num_samples;
			pi->interf->aci.nphy->undetect_window_sz =
				MIN(params->nphy_undetect_window_sz,
				ACI_MAX_UNDETECT_WINDOW_SZ);
			pi->interf->aci.nphy->b_energy_lo_aci = params->nphy_b_energy_lo_aci;
			pi->interf->aci.nphy->b_energy_md_aci = params->nphy_b_energy_md_aci;
			pi->interf->aci.nphy->b_energy_hi_aci = params->nphy_b_energy_hi_aci;

			pi->interf->noise.nphy_noise_noassoc_glitch_th_up =
				params->nphy_noise_noassoc_glitch_th_up;
			pi->interf->noise.nphy_noise_noassoc_glitch_th_dn =
				params->nphy_noise_noassoc_glitch_th_dn;
			pi->interf->noise.nphy_noise_assoc_glitch_th_up =
				params->nphy_noise_assoc_glitch_th_up;
			pi->interf->noise.nphy_noise_assoc_glitch_th_dn =
				params->nphy_noise_assoc_glitch_th_dn;
			pi->interf->noise.nphy_noise_assoc_aci_glitch_th_up =
				params->nphy_noise_assoc_aci_glitch_th_up;
			pi->interf->noise.nphy_noise_assoc_aci_glitch_th_dn =
				params->nphy_noise_assoc_aci_glitch_th_dn;
			pi->interf->noise.nphy_noise_assoc_enter_th =
				params->nphy_noise_assoc_enter_th;
			pi->interf->noise.nphy_noise_noassoc_enter_th =
				params->nphy_noise_noassoc_enter_th;
			pi->interf->noise.nphy_noise_assoc_rx_glitch_badplcp_enter_th =
				params->nphy_noise_assoc_rx_glitch_badplcp_enter_th;
			pi->interf->noise.nphy_noise_noassoc_crsidx_incr =
				params->nphy_noise_noassoc_crsidx_incr;
			pi->interf->noise.nphy_noise_assoc_crsidx_incr =
				params->nphy_noise_assoc_crsidx_incr;
			pi->interf->noise.nphy_noise_crsidx_decr =
				params->nphy_noise_crsidx_decr;
		}
	}

	return BCME_OK;
}
#endif /* defined (BCMINTERNAL) || defined (WLTEST) || defined (WL_PHYACIARGS) */
#endif /* Compiling out ACI code for 4324 */

static void wlc_phy_table_lock_lcnphy(phy_info_t *pi);
static void wlc_phy_table_unlock_lcnphy(phy_info_t *pi);

void
wlc_phy_table_read_lcnphy(phy_info_t *pi, const lcnphytbl_info_t *ptbl_info)
{
	uint    idx;
	uint    tbl_id     = ptbl_info->tbl_id;
	uint    tbl_offset = ptbl_info->tbl_offset;
	uint32  u32temp;

	uint8  *ptbl_8b    = (uint8  *)(uintptr)ptbl_info->tbl_ptr;
	uint16 *ptbl_16b   = (uint16 *)(uintptr)ptbl_info->tbl_ptr;
	uint32 *ptbl_32b   = (uint32 *)(uintptr)ptbl_info->tbl_ptr;

	uint16 tblAddr = LCNPHY_TableAddress;
	uint16 tblDataHi = LCNPHY_TabledataHi;
	uint16 tblDatalo = LCNPHY_TabledataLo;

	ASSERT((ptbl_info->tbl_phywidth == 8) || (ptbl_info->tbl_phywidth == 16) ||
		(ptbl_info->tbl_phywidth == 32));
	ASSERT((ptbl_info->tbl_width == 8) || (ptbl_info->tbl_width == 16) ||
		(ptbl_info->tbl_width == 32));

	wlc_phy_table_lock_lcnphy(pi);

	phy_utils_write_phyreg(pi, tblAddr, (tbl_id << 10) | tbl_offset);

	for (idx = 0; idx < ptbl_info->tbl_len; idx++) {

		/* get the element from phy according to the phy table element
		 * address space width
		 */
		if (ptbl_info->tbl_phywidth == 32) {
			/* phy width is 32-bit */
			u32temp  =  phy_utils_read_phyreg(pi, tblDatalo);
			u32temp |= (phy_utils_read_phyreg(pi, tblDataHi) << 16);
		} else if (ptbl_info->tbl_phywidth == 16) {
			/* phy width is 16-bit */
			u32temp  =  phy_utils_read_phyreg(pi, tblDatalo);
		} else {
			/* phy width is 8-bit */
			u32temp   =  (uint8)phy_utils_read_phyreg(pi, tblDatalo);
		}

		/* put the element into the table according to the table element width
		 * Note that phy table width is some times more than necessary while
		 * table width is always optimal.
		 */
		if (ptbl_info->tbl_width == 32) {
			/* tbl width is 32-bit */
			ptbl_32b[idx]  =  u32temp;
		} else if (ptbl_info->tbl_width == 16) {
			/* tbl width is 16-bit */
			ptbl_16b[idx]  =  (uint16)u32temp;
		} else {
			/* tbl width is 8-bit */
			ptbl_8b[idx]   =  (uint8)u32temp;
		}
	}
	wlc_phy_table_unlock_lcnphy(pi);
}

/* prevent simultaneous phy table access by driver and ucode */
static void
wlc_phy_table_lock_lcnphy(phy_info_t *pi)
{
	uint32 mc = R_REG(pi->sh->osh, &pi->regs->maccontrol);

	if (mc & MCTL_EN_MAC) {
		wlapi_bmac_mctrl(pi->sh->physhim, MCTL_PHYLOCK,  MCTL_PHYLOCK);
		(void)R_REG(pi->sh->osh, &pi->regs->maccontrol);
		OSL_DELAY(5);
	}
}

static void
wlc_phy_table_unlock_lcnphy(phy_info_t *pi)
{
	wlapi_bmac_mctrl(pi->sh->physhim, MCTL_PHYLOCK,  0);
}

uint
wlc_phy_init_radio_regs_allbands(phy_info_t *pi, const radio_20xx_regs_t *radioregs)
{
	uint i;

	for (i = 0; radioregs[i].address != 0xffff; i++) {
		if (radioregs[i].do_init) {
			phy_utils_write_radioreg(pi, radioregs[i].address,
			                (uint16)radioregs[i].init);
		}
	}

	return i;
}
uint
wlc_phy_init_radio_regs_allbands_20671(phy_info_t *pi, radio_20671_regs_t *radioregs)
{
	uint i;

	for (i = 0; radioregs[i].address != 0xffff; i++) {
		if (radioregs[i].do_init) {
			phy_utils_write_radioreg(pi, radioregs[i].address,
			                (uint16)radioregs[i].init);
		}
	}

	return i;
}
uint
wlc_phy_init_radio_prefregs_allbands(phy_info_t *pi, const radio_20xx_prefregs_t *radioregs)
{
	uint i;

	for (i = 0; radioregs[i].address != 0xffff; i++) {
		phy_utils_write_radioreg(pi, radioregs[i].address,
		                (uint16)radioregs[i].init);
		if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
			OSL_DELAY(100);
		}
	}

	return i;
}

uint
wlc_phy_init_radio_regs(phy_info_t *pi, radio_regs_t *radioregs, uint16 core_offset)
{
	uint i;

	for (i = 0; radioregs[i].address != 0xffff; i++) {
#ifdef BAND5G
		if (CHSPEC_IS5G(pi->radio_chanspec)) {
			if (radioregs[i].do_init_a) {
				phy_utils_write_radioreg(pi, radioregs[i].address | core_offset,
					(uint16)radioregs[i].init_a);
			}
		} else
#endif /* BAND5G */
		{
			if (radioregs[i].do_init_g) {
				phy_utils_write_radioreg(pi, radioregs[i].address | core_offset,
					(uint16)radioregs[i].init_g);
			}
		}
	}

	return i;
}

void
wlc_phy_do_dummy_tx(phy_info_t *pi, bool ofdm, bool pa_on)
{
#define	DUMMY_PKT_LEN	20 /* Dummy packet's length */
	d11regs_t *regs = pi->regs;
	int	i, count;
	uint8	ofdmpkt[DUMMY_PKT_LEN] = {
		0xcc, 0x01, 0x02, 0x00, 0x00, 0x00, 0xd4, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00
	};
	uint8	cckpkt[DUMMY_PKT_LEN] = {
		0x6e, 0x84, 0x0b, 0x00, 0x00, 0x00, 0xd4, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00
	};
	uint32 *dummypkt;

	ASSERT((R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC) == 0);

	dummypkt = (uint32 *)(ofdm ? ofdmpkt : cckpkt);
	wlapi_bmac_write_template_ram(pi->sh->physhim, 0, DUMMY_PKT_LEN, dummypkt);

	/* set up the TXE transfer */

	W_REG(pi->sh->osh, &regs->PHYREF_XMTSEL, 0);
	/* Assign the WEP to the transmit path */
	W_REG(pi->sh->osh, &regs->PHYREF_WEPCTL, 0x100);

	/* Set/clear OFDM bit in PHY control word */
	W_REG(pi->sh->osh, &regs->txe_phyctl, (ofdm ? 1 : 0) | PHY_TXC_ANT_0);
	if (ISNPHY(pi) || ISHTPHY(pi) || ISLCNCOMMONPHY(pi) || ISLCN20PHY(pi)) {
		ASSERT(ofdm);
		W_REG(pi->sh->osh, &regs->txe_phyctl1, 0x1A02);
	}

	W_REG(pi->sh->osh, &regs->txe_wm_0, 0);		/* No substitutions */
	W_REG(pi->sh->osh, &regs->txe_wm_1, 0);

	/* Set transmission from the TEMPLATE where we loaded the frame */
	if (D11REV_IS(pi->sh->corerev, 35)) {
		W_REG(pi->sh->osh, &regs->PHYREF_XMTTPLATETXPTR, PHY_TXFIFO_END_BLK_REV35);
	} else
		W_REG(pi->sh->osh, &regs->PHYREF_XMTTPLATETXPTR, 0);
	W_REG(pi->sh->osh, &regs->PHYREF_XMTTXCNT, DUMMY_PKT_LEN);

	/* Set Template as source, length specified as a count and destination
	 * as Serializer also set "gen_eof"
	 */
	W_REG(pi->sh->osh, &regs->PHYREF_XMTSEL, ((8 << 8) | (1 << 5) | (1 << 2) | 2));

	/* Instruct the MAC to not calculate FCS, we'll supply a bogus one */
	W_REG(pi->sh->osh, &regs->txe_ctl, 0);

	if (!pa_on) {
		if (ISNPHY(pi))
			wlc_phy_pa_override_nphy(pi, OFF);
		else if (ISHTPHY(pi)) {
			wlc_phy_pa_override_htphy(pi, OFF);
		}
	}

	/* Start transmission and wait until sendframe goes away */
	/* Set TX_NOW in AUX along with MK_CTLWRD */
	if (ISNPHY(pi) || ISHTPHY(pi) || ISLCNCOMMONPHY(pi) || ISLCN20PHY(pi))
		W_REG(pi->sh->osh, &regs->txe_aux, 0xD0);
	else
		W_REG(pi->sh->osh, &regs->txe_aux, ((1 << 5) | (1 << 4)));

	(void)R_REG(pi->sh->osh, &regs->txe_aux);

	/* Wait for 10 x ack time, enlarge it for vsim of QT */
	i = 0;
	count = ofdm ? 30 : 250;

#ifndef BCMQT_CPU
	if (ISSIM_ENAB(pi->sh->sih)) {
		count *= 100;
	}
#endif
	/* wait for txframe to be zero */
	while ((i++ < count) && (R_REG(pi->sh->osh, &regs->txe_status) & (1 << 7))) {
		OSL_DELAY(10);
	}
	if (i >= count)
		PHY_ERROR(("wl%d: %s: Waited %d uS for %s txframe\n",
		          pi->sh->unit, __FUNCTION__, 10 * i, (ofdm ? "ofdm" : "cck")));

	/* Wait for the mac to finish (this is 10x what is supposed to take) */
	i = 0;
	/* wait for txemend */
	while ((i++ < 10) && ((R_REG(pi->sh->osh, &regs->txe_status) & (1 << 10)) == 0)) {
		OSL_DELAY(10);
	}
	if (i >= 10)
		PHY_ERROR(("wl%d: %s: Waited %d uS for txemend\n",
		          pi->sh->unit, __FUNCTION__, 10 * i));

	/* Wait for the phy to finish */
	i = 0;
	/* wait for txcrs */
	while ((i++ < 500) && ((R_REG(pi->sh->osh, &regs->PHYREF_IFSSTAT) & (1 << 8)))) {
		OSL_DELAY(10);
	}
	if (i >= 500)
		PHY_ERROR(("wl%d: %s: Waited %d uS for txcrs\n",
		          pi->sh->unit, __FUNCTION__, 10 * i));
	if (!pa_on) {
		if (ISNPHY(pi))
			wlc_phy_pa_override_nphy(pi, ON);
		else if (ISHTPHY(pi)) {
			wlc_phy_pa_override_htphy(pi, ON);
		}
	}
}

void
wlc_phy_clear_tssi(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t *)pih;

	if (ISNPHY(pi) || ISHTPHY(pi) || ISACPHY(pi)) {
		/* NPHY/HTPHY doesn't use sw or ucode powercontrol */
		return;
	} else {
		wlapi_bmac_write_shm(pi->sh->physhim, M_TSSI_0(pi), NULL_TSSI_W);
		wlapi_bmac_write_shm(pi->sh->physhim, M_TSSI_1(pi), NULL_TSSI_W);
		wlapi_bmac_write_shm(pi->sh->physhim, M_TSSI_OFDM_0(pi), NULL_TSSI_W);
		wlapi_bmac_write_shm(pi->sh->physhim, M_TSSI_OFDM_1(pi), NULL_TSSI_W);
	}
}

void
wlc_phy_chanspec_shm_set(phy_info_t *pi, chanspec_t chanspec)
{
	uint16 curchannel;

	/* Update ucode channel value */
	if (D11REV_LT(pi->sh->corerev, 40)) {
		/* d11 rev < 40: compose a channel info value */
		curchannel = CHSPEC_CHANNEL(chanspec);
#ifdef BAND5G
		if (CHSPEC_IS5G(chanspec))
			curchannel |= D11_CURCHANNEL_5G;
#endif /* BAND5G */
		if (CHSPEC_IS40(chanspec))
			curchannel |= D11_CURCHANNEL_40;
	} else {
		/* d11 rev >= 40: store the chanspec */
		curchannel = chanspec;
	}

	PHY_TRACE(("wl%d: %s: M_CURCHANNEL %x\n", pi->sh->unit, __FUNCTION__, curchannel));
	wlapi_bmac_write_shm(pi->sh->physhim, M_CURCHANNEL(pi), curchannel);
}

int32
wlc_phy_min_txpwr_limit_get(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	return (int32)pi->min_txpower;
}

void
wlc_phy_set_filt_war(wlc_phy_t *ppi, bool filt_war)
{
	phy_info_t *pi = (phy_info_t*)ppi;

	if (ISHTPHY(pi))
		wlc_phy_set_filt_war_htphy(pi, filt_war);
}

int
wlc_phy_lo_gain_nbcal(wlc_phy_t *ppi, bool lo_gain)
{
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	phy_info_t *pi = (phy_info_t*)ppi;

	if (ISLCN20PHY(pi)) {
		wlc_phy_set_lo_gain_nbcal_lcn20phy(pi, lo_gain);
		return BCME_OK;
	} else
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
	{
		return BCME_UNSUPPORTED;
	}
}


bool
wlc_phy_get_filt_war(wlc_phy_t *ppi)
{
	bool ret = FALSE;
	phy_info_t *pi = (phy_info_t*)ppi;

	if (ISHTPHY(pi))
		ret = wlc_phy_get_filt_war_htphy(pi);
	return ret;
}

#if defined(BCMTSTAMPEDLOGS)
void
phy_log(phy_info_t *pi, const char* str, uint32 p1, uint32 p2)
{
	/* Read a timestamp from the TSF timer register */
	uint32 tstamp = R_REG(pi->sh->osh, &pi->regs->tsf_timerlow);

	/* Store the timestamp and the log message in the log buffer */
	bcmtslog(tstamp, str, p1, p2);
}
#endif

void
wlc_phy_chanspec_ch14_widefilter_set(wlc_phy_t *ppi, bool wide_filter)
{


}

uint8
wlc_phy_txpower_get_target_min(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t*)ppi;

	uint8 core;
	uint8 tx_pwr_min = WLC_TXPWR_MAX;

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
		tx_pwr_min = MIN(tx_pwr_min, pi->tx_power_min_per_core[core]);
	}

	return tx_pwr_min;
}

uint8
wlc_phy_txpower_get_target_max(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t*)ppi;

	uint8 core;
	uint8 tx_pwr_max = 0;

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
		tx_pwr_max = MAX(tx_pwr_max, pi->tx_power_max_per_core[core]);
	}

	return tx_pwr_max;
}

#ifdef BCMDBG
void
wlc_phy_txpower_limits_dump(ppr_t* txpwr, bool ishtphy)
{
	int i;
	char fraction[4][4] = {"   ", ".25", ".5 ", ".75"};
	ppr_dsss_rateset_t dsss_limits;
	ppr_ofdm_rateset_t ofdm_limits;
	ppr_ht_mcs_rateset_t mcs_limits;

	printf("CCK                  ");
	ppr_get_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_1, &dsss_limits);
	for (i = 0; i < WL_RATESET_SZ_DSSS; i++) {
		printf(" %2d%s", dsss_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[dsss_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("20MHz OFDM SISO      ");
	ppr_get_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
	for (i = 0; i < WL_RATESET_SZ_OFDM; i++) {
		printf(" %2d%s", ofdm_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[ofdm_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("20MHz OFDM CDD       ");
	ppr_get_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_CDD, WL_TX_CHAINS_2, &ofdm_limits);
	for (i = 0; i < WL_RATESET_SZ_OFDM; i++) {
		printf(" %2d%s", ofdm_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[ofdm_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("%s", ishtphy ? "20MHz 1 Nsts to 1 Tx " : "20MHz MCS 0-7 SISO   ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("%s", ishtphy ? "20MHz 1 Nsts to 2 Tx " : "20MHz MCS 0-7 CDD    ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	if (ishtphy) {
		printf("20MHz 1 Nsts to 3 Tx ");
		ppr_get_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
			&mcs_limits);
	} else {
		printf("20MHz MCS 0-7 STBC   ");
		ppr_get_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_STBC, WL_TX_CHAINS_2,
			&mcs_limits);
	}
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("%s", ishtphy ? "20MHz 2 Nsts to 2 Tx " : "20MHz MCS 8-15 SDM   ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_2,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	if (ishtphy) {
		printf("20MHz 2 Nsts to 3 Tx ");
		ppr_get_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_3,
			&mcs_limits);
		for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
			printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
				fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
		}
		printf("\n");

		printf("20MHz 3 Nsts to 3 Tx ");
		ppr_get_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_3, WL_TX_MODE_NONE, WL_TX_CHAINS_3,
			&mcs_limits);
		for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
			printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
				fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
		}
		printf("\n");
	}


	printf("40MHz OFDM SISO      ");
	ppr_get_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
	for (i = 0; i < WL_RATESET_SZ_OFDM; i++) {
		printf(" %2d%s", ofdm_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[ofdm_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("40MHz OFDM CDD       ");
	ppr_get_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_CDD, WL_TX_CHAINS_2, &ofdm_limits);
	for (i = 0; i < WL_RATESET_SZ_OFDM; i++) {
		printf(" %2d%s", ofdm_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[ofdm_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("%s", ishtphy ? "40MHz 1 Nsts to 1 Tx " : "40MHz MCS 0-7 SISO   ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("%s", ishtphy ? "40MHz 1 Nsts to 2 Tx " : "40MHz MCS 0-7 CDD    ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("%s", ishtphy ? "40MHz 1 Nsts to 3 Tx " : "40MHz MCS 0-7 CDD    ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("%s", ishtphy ? "40MHz 2 Nsts to 2 Tx " : "40MHz MCS8-15 SDM    ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_2,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	if (ishtphy) {
		printf("40MHz 2 Nsts to 3 Tx ");
		ppr_get_ht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_3,
			&mcs_limits);
		for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
			printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
				fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
		}
		printf("\n");

		printf("40MHz 3 Nsts to 3 Tx ");
		ppr_get_ht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_3, WL_TX_MODE_NONE, WL_TX_CHAINS_3,
			&mcs_limits);
		for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
			printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
				fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
		}
		printf("\n");
	}

	if (!ishtphy)
		return;

	printf("20MHz UL CCK         ");
	ppr_get_dsss(txpwr, WL_TX_BW_20IN40, WL_TX_CHAINS_1, &dsss_limits);
	for (i = 0; i < WL_RATESET_SZ_DSSS; i++) {
		printf(" %2d%s", dsss_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[dsss_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("20MHz UL OFDM SISO   ");
	ppr_get_ofdm(txpwr, WL_TX_BW_20IN40, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
	for (i = 0; i < WL_RATESET_SZ_OFDM; i++) {
		printf(" %2d%s", ofdm_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[ofdm_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("20MHz UL OFDM CDD    ");
	ppr_get_ofdm(txpwr, WL_TX_BW_20IN40, WL_TX_MODE_CDD, WL_TX_CHAINS_2, &ofdm_limits);
	for (i = 0; i < WL_RATESET_SZ_OFDM; i++) {
		printf(" %2d%s", ofdm_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[ofdm_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("20UL 1 Nsts to 1 Tx  ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("20UL 1 Nsts to 2 Tx  ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("20UL 1 Nsts to 3 Tx  ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("20UL 2 Nsts to 2 Tx  ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_2,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("20UL 2 Nsts to 3 Tx  ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_3,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");

	printf("20UL 3 Nsts to 3 Tx  ");
	ppr_get_ht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_3, WL_TX_MODE_NONE, WL_TX_CHAINS_3,
		&mcs_limits);
	for (i = 0; i < WL_RATESET_SZ_HT_MCS; i++) {
		printf(" %2d%s", mcs_limits.pwr[i]/ WLC_TXPWR_DB_FACTOR,
			fraction[mcs_limits.pwr[i] % WLC_TXPWR_DB_FACTOR]);
	}
	printf("\n");
}

#endif /* BCMDBG */

/* Map Legacy OFDM powers-per-rate to MCS 0-7 powers-per-rate by matching the
 * constellation and coding rate of the corresponding Legacy OFDM and MCS rates. The power
 * of 9 Mbps Legacy OFDM is used for MCS-0 (same as 6 Mbps power) since no equivalent
 * of 9 Mbps exists in the 11n standard in terms of constellation and coding rate.
 */

void
wlc_phy_copy_ofdm_to_mcs_powers(ppr_ofdm_rateset_t* ofdm_limits, ppr_ht_mcs_rateset_t* mcs_limits)
{
	uint rate1;
	uint rate2;

	for (rate1 = 0, rate2 = 1; rate1 < WL_RATESET_SZ_OFDM-1; rate1++, rate2++) {
		mcs_limits->pwr[rate1] = ofdm_limits->pwr[rate2];
	}
	mcs_limits->pwr[rate1] = mcs_limits->pwr[rate1 - 1];
}


/* Map MCS 0-7 powers-per-rate to Legacy OFDM powers-per-rate by matching the
 * constellation and coding rate of the corresponding Legacy OFDM and MCS rates. The power
 * of 9 Mbps Legacy OFDM is set to the power of MCS-0 (same as 6 Mbps power) since no equivalent
 * of 9 Mbps exists in the 11n standard in terms of constellation and coding rate.
 */

void
wlc_phy_copy_mcs_to_ofdm_powers(ppr_ht_mcs_rateset_t* mcs_limits, ppr_ofdm_rateset_t* ofdm_limits)
{
	uint rate1;
	uint rate2;

	ofdm_limits->pwr[0] = mcs_limits->pwr[0];
	for (rate1 = 1, rate2 = 0; rate1 < WL_RATESET_SZ_OFDM; rate1++, rate2++) {
		ofdm_limits->pwr[rate1] = mcs_limits->pwr[rate2];
	}
}

void
BCMATTACHFN(wlc_phy_machwcap_set)(wlc_phy_t *ppi, uint32 machwcap)
{
	phy_info_t *pi = (phy_info_t*)ppi;

	pi->sh->machwcap = machwcap;
}

void
wlc_phy_runbist_config(wlc_phy_t *ppi, bool start_end)
{
	phy_info_t *pi = (phy_info_t*)ppi;

	if (start_end == ON) {
		if (!ISNPHY(pi) || ISHTPHY(pi))
			return;
	} else {
		wlc_phy_por_inform(ppi);
	}
}

#ifdef FCC_PWR_LIMIT_2G
void
wlc_phy_prev_chanspec_set(wlc_phy_t *ppi, chanspec_t prev_chanspec)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	pi->previous_chanspec = prev_chanspec;
}

bool
wlc_phy_isvalid_fcc_pwr_limit(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	uint prev_ch = CHSPEC_CHANNEL(pi->previous_chanspec);
	uint target_ch = CHSPEC_CHANNEL(pi->radio_chanspec);

	/* channel validity check */
	if ((prev_ch == 12 || prev_ch == 13 ||
		target_ch == 12 || target_ch == 13)) {
		return TRUE;
	}
	return FALSE;
}
#endif /* FCC_PWR_LIMIT_2G */

void
wlc_phy_ofdm_rateset_war(wlc_phy_t *pih, bool war)
{
	phy_info_t *pi = (phy_info_t*)pih;

	pi->ofdm_rateset_war = war;
}

void
wlc_phy_bf_preempt_enable(wlc_phy_t *pih, bool bf_preempt)
{
}

void
wlc_phy_txpower_hw_ctrl_set(wlc_phy_t *ppi, bool hwpwrctrl)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	bool cur_hwpwrctrl = pi->hwpwrctrl;
	bool suspend;

	/* validate if hardware power control is capable */
	if (!pi->hwpwrctrl_capable) {
		PHY_ERROR(("wl%d: hwpwrctrl not capable\n", pi->sh->unit));
		return;
	}

	PHY_INFORM(("wl%d: setting the hwpwrctrl to %d\n", pi->sh->unit, hwpwrctrl));
	pi->hwpwrctrl = hwpwrctrl;
	pi->nphy_txpwrctrl = hwpwrctrl;
	pi->txpwrctrl = hwpwrctrl;

	/* if power control mode is changed, propagate it */
	if (ISNPHY(pi)) {
		suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);
		if (!suspend)
			wlapi_suspend_mac_and_wait(pi->sh->physhim);

		/* turn on/off power control */
		wlc_phy_txpwrctrl_enable_nphy(pi, pi->nphy_txpwrctrl);
		if (pi->nphy_txpwrctrl == PHY_TPC_HW_OFF) {
			wlc_phy_txpwr_fixpower_nphy(pi);
		} else {
			/* restore the starting txpwr index */
			phy_utils_mod_phyreg(pi, NPHY_TxPwrCtrlCmd,
				NPHY_TxPwrCtrlCmd_pwrIndex_init_MASK, pi->saved_txpwr_idx);
		}

		if (!suspend)
			wlapi_enable_mac(pi->sh->physhim);
	} else if (ISHTPHY(pi)) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);

		/* turn on/off power control */
		wlc_phy_txpwrctrl_enable_htphy(pi, pi->txpwrctrl);

		wlapi_enable_mac(pi->sh->physhim);
	} else if (hwpwrctrl != cur_hwpwrctrl) {
		/* save, change and restore tx power control */

		if (ISLCNCOMMONPHY(pi)) {
			return;

		}
	}
}

void
wlc_phy_txpower_ipa_upd(phy_info_t *pi)
{
	/* this should be expanded to work with all new PHY capable of iPA */
	if (ISACPHY(pi)) {
		pi->ipa2g_on = ((pi->epagain2g == 2) || (pi->extpagain2g == 2));
		pi->ipa5g_on = ((pi->epagain5g == 2) || (pi->extpagain5g == 2));
	} else if (ISNPHY(pi)) {
		pi->ipa2g_on = (pi->fem2g->extpagain == 2);
		pi->ipa5g_on = (pi->fem5g->extpagain == 2);
	} else {
		pi->ipa2g_on = FALSE;
		pi->ipa5g_on = FALSE;
	}
	PHY_INFORM(("wlc_phy_txpower_ipa_upd: ipa 2g %d, 5g %d\n", pi->ipa2g_on, pi->ipa5g_on));
}

int wlc_phy_get_est_pout(wlc_phy_t *ppi, uint8* est_Pout, uint8* est_Pout_adj, uint8* est_Pout_cck)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	*est_Pout_cck = 0;

	if (!pi->sh->up) {
		return BCME_NOTUP;
	}

	/* fill the est_Pout array */
	if (ISNPHY(pi)) {
		uint32 est_pout;

		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))
			est_pout = wlc_phy_txpower_est_power_lcnxn_rev3(pi);
		else
			est_pout = wlc_phy_txpower_est_power_nphy(pi);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);

		/* Store the adjusted  estimated powers */
		est_Pout_adj[0] = (est_pout >> 8) & 0xff;
		est_Pout_adj[1] = est_pout & 0xff;

		/* Store the actual estimated powers without adjustment */
		est_Pout[0] = est_pout >> 24;
		est_Pout[1] = (est_pout >> 16) & 0xff;

		/* if invalid, return 0 */
		if (est_Pout[0] == 0x80)
			est_Pout[0] = 0;
		if (est_Pout[1] == 0x80)
			est_Pout[1] = 0;

		/* if invalid, return 0 */
		if (est_Pout_adj[0] == 0x80)
			est_Pout_adj[0] = 0;
		if (est_Pout_adj[1] == 0x80)
			est_Pout_adj[1] = 0;


	} else if (ISHTPHY(pi)) {
		/* Get power estimates */
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		wlc_phy_txpwr_est_pwr_htphy(pi, est_Pout, est_Pout_adj);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);

	} else if (ISACPHY(pi)) {
		/* Get power estimates */
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		wlc_phy_txpwr_est_pwr_acphy(pi, est_Pout, est_Pout_adj);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);

	} else if (!pi->hwpwrctrl) {
	} else if (pi->sh->up) {
		if (ISLCN40PHY(pi)) {
			/* Get power estimates */
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
			phy_utils_phyreg_enter(pi);
			wlc_lcn40phy_get_tssi(pi, (int8*)&est_Pout[0], (int8*)est_Pout_cck);
			phy_utils_phyreg_exit(pi);
			wlapi_enable_mac(pi->sh->physhim);
			est_Pout_adj[0] = est_Pout[0];
#if (defined(LCN20CONF) && (LCN20CONF != 0))
		} else if (ISLCN20PHY(pi)) {
			/* Get power estimates */
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
			phy_utils_phyreg_enter(pi);
			wlc_lcn20phy_get_tssi(pi, (int8*)&est_Pout[0], (int8*)est_Pout_cck);
			phy_utils_phyreg_exit(pi);
			wlapi_enable_mac(pi->sh->physhim);
			est_Pout_adj[0] = est_Pout[0];
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
		}
	}
	return BCME_OK;
}

#if defined(BCMDBG) || defined(WLTEST)
/*
 * Rate is number of 500 Kb units.
 */
static int
wlc_phy_test_evm(phy_info_t *pi, int channel, uint rate, int txpwr)
{
	d11regs_t *regs = pi->regs;
	uint16 reg = 0;
	int bcmerror = 0;

	/* stop any test in progress */
	wlc_phy_test_stop(pi);

	/* channel 0 means restore original contents and end the test */
	if (channel == 0) {
		if (ISNPHY(pi)) {
			phy_utils_write_phyreg(pi, (NPHY_TO_BPHY_OFF + BPHY_TEST),
			              pi->evm_phytest);
		} else if (ISLCN40PHY(pi)) {
			phy_utils_write_phyreg(pi, LCN40PHY_bphyTest, pi->evm_phytest);
			phy_utils_write_phyreg(pi, LCN40PHY_ClkEnCtrl, 0);
			wlc_lcn40phy_tx_pu(pi, 0);
		} else 	if (ISHTPHY(pi))
			wlc_phy_bphy_testpattern_htphy(pi, HTPHY_TESTPATTERN_BPHY_EVM, reg, FALSE);
		 else
			W_REG(pi->sh->osh, &regs->phytest, pi->evm_phytest);

		pi->evm_phytest = 0;

		if (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardflags) & BFL_PACTRL) {
			W_REG(pi->sh->osh, &pi->regs->psm_gpio_out, pi->evm_o);
			W_REG(pi->sh->osh, &pi->regs->psm_gpio_oe, pi->evm_oe);
			OSL_DELAY(1000);
		}
		return 0;
	}

	if (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardflags) & BFL_PACTRL) {
		PHY_INFORM(("wl%d: %s: PACTRL boardflag set, clearing gpio 0x%04x\n",
			pi->sh->unit, __FUNCTION__, BOARD_GPIO_PACTRL));
		/* Store initial values */
		pi->evm_o = R_REG(pi->sh->osh, &pi->regs->psm_gpio_out);
		pi->evm_oe = R_REG(pi->sh->osh, &pi->regs->psm_gpio_oe);
		AND_REG(pi->sh->osh, &regs->psm_gpio_out, ~BOARD_GPIO_PACTRL);
		OR_REG(pi->sh->osh, &regs->psm_gpio_oe, BOARD_GPIO_PACTRL);
		OSL_DELAY(1000);
	}

	if ((bcmerror = wlc_phy_test_init(pi, channel, TRUE)))
		return bcmerror;

	reg = TST_TXTEST_RATE_2MBPS;
	switch (rate) {
	case 2:
		reg = TST_TXTEST_RATE_1MBPS;
		break;
	case 4:
		reg = TST_TXTEST_RATE_2MBPS;
		break;
	case 11:
		reg = TST_TXTEST_RATE_5_5MBPS;
		break;
	case 22:
		reg = TST_TXTEST_RATE_11MBPS;
		break;
	}
	reg = (reg << TST_TXTEST_RATE_SHIFT) & TST_TXTEST_RATE;

	PHY_INFORM(("wlc_evm: rate = %d, reg = 0x%x\n", rate, reg));

	/* Save original contents */
	if (pi->evm_phytest == 0 && !ISHTPHY(pi)) {
		if (ISNPHY(pi)) {
			pi->evm_phytest = phy_utils_read_phyreg(pi,
			                               (NPHY_TO_BPHY_OFF + BPHY_TEST));
		} else if (ISLCN40PHY(pi)) {
			pi->evm_phytest = phy_utils_read_phyreg(pi, LCN40PHY_bphyTest);
			phy_utils_write_phyreg(pi, LCN40PHY_ClkEnCtrl, 0xffff);
		} else
			pi->evm_phytest = R_REG(pi->sh->osh, &regs->phytest);
	}

	/* Set EVM test mode */
	if (ISHTPHY(pi)) {
		wlc_phy_bphy_testpattern_htphy(pi, NPHY_TESTPATTERN_BPHY_EVM, reg, TRUE);
	} else if (ISNPHY(pi)) {
		phy_utils_and_phyreg(pi, (NPHY_TO_BPHY_OFF + BPHY_TEST),
		            ~(TST_TXTEST_ENABLE|TST_TXTEST_RATE|TST_TXTEST_PHASE));
		phy_utils_or_phyreg(pi, (NPHY_TO_BPHY_OFF + BPHY_TEST), TST_TXTEST_ENABLE | reg);
	} else if ISLCN40PHY(pi) {
		wlc_lcn40phy_tx_pu(pi, 1);
		phy_utils_or_phyreg(pi, LCN40PHY_bphyTest, 0x128);
	} else {
		AND_REG(pi->sh->osh, &regs->phytest,
		        ~(TST_TXTEST_ENABLE|TST_TXTEST_RATE|TST_TXTEST_PHASE));
		OR_REG(pi->sh->osh, &regs->phytest, TST_TXTEST_ENABLE | reg);
	}
	return 0;
}

static int
wlc_phy_test_carrier_suppress(phy_info_t *pi, int channel)
{
	d11regs_t *regs = pi->regs;
	int bcmerror = 0;

	/* stop any test in progress */
	wlc_phy_test_stop(pi);

	/* channel 0 means restore original contents and end the test */
	if (channel == 0) {
		if (ISNPHY(pi)) {
			phy_utils_write_phyreg(pi, (NPHY_TO_BPHY_OFF + BPHY_TEST),
			              pi->car_sup_phytest);
		} else if (ISLCN40PHY(pi)) {
			/* Disable carrier suppression */
			wlc_lcn40phy_crsuprs(pi, channel);
		} else 	if (ISHTPHY(pi)) {
			wlc_phy_bphy_testpattern_htphy(pi, HTPHY_TESTPATTERN_BPHY_RFCS, 0, FALSE);
		} else
			W_REG(pi->sh->osh, &regs->phytest, pi->car_sup_phytest);

		pi->car_sup_phytest = 0;
		return 0;
	}

	if ((bcmerror = wlc_phy_test_init(pi, channel, TRUE)))
		return bcmerror;

	/* Save original contents */
	if (pi->car_sup_phytest == 0 && !ISHTPHY(pi)) {
	        if (ISNPHY(pi)) {
			pi->car_sup_phytest = phy_utils_read_phyreg(pi,
			                                   (NPHY_TO_BPHY_OFF + BPHY_TEST));
		} else
			pi->car_sup_phytest = R_REG(pi->sh->osh, &regs->phytest);
	}

	/* set carrier suppression test mode */
	if (ISHTPHY(pi)) {
		wlc_phy_bphy_testpattern_htphy(pi, HTPHY_TESTPATTERN_BPHY_RFCS, 0, TRUE);
	} else if (ISNPHY(pi)) {
		PHY_REG_LIST_START
			PHY_REG_AND_RAW_ENTRY(NPHY_TO_BPHY_OFF + BPHY_TEST, 0xfc00)
			PHY_REG_OR_RAW_ENTRY(NPHY_TO_BPHY_OFF + BPHY_TEST, 0x0228)
		PHY_REG_LIST_EXECUTE(pi);
	} else {
		AND_REG(pi->sh->osh, &regs->phytest, 0xfc00);
		OR_REG(pi->sh->osh, &regs->phytest, 0x0228);
	}

	return 0;
}
#endif /* BCMDBG || WLTEST  */

void
wlc_phy_antsel_type_set(wlc_phy_t *ppi, uint8 antsel_type)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	pi->antsel_type = antsel_type;

	/* initialize flag to init HW Rx antsel if the board supports it */
	if ((pi->antsel_type == ANTSEL_2x3_HWRX) || (pi->antsel_type == ANTSEL_1x2_HWRX))
		pi->nphy_enable_hw_antsel = TRUE;
	else
		pi->nphy_enable_hw_antsel = FALSE;
}

bool
wlc_phy_test_ison(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	return (pi->phytest_on);
}

void
wlc_phy_interference_set(wlc_phy_t *pih, bool init)
{
	int wanted_mode;
	phy_info_t *pi = (phy_info_t *)pih;

	if (!(ISNPHY(pi) || ISHTPHY(pi) || (CHIPID(pi->sh->chip) == BCM43142_CHIP_ID)))
		return;

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
}

#ifndef WLC_DISABLE_ACI
/* %%%%%% interference */
/* update aci rx carrier sense glitch moving average */
static void
wlc_phy_aci_update_ma(phy_info_t *pi)
{

	int32 delta = 0;
	int32 bphy_delta = 0;
	int32 ofdm_delta = 0;
	int32 badplcp_delta = 0;
	int32 bphy_badplcp_delta = 0;
	int32 ofdm_badplcp_delta = 0;

	if ((pi->interf->cca_stats_func_called == FALSE) || pi->interf->cca_stats_mbsstime <= 0) {
	  uint16 cur_glitch_cnt;
	  uint16 bphy_cur_glitch_cnt = 0;
	  uint16 cur_badplcp_cnt = 0;
	  uint16 bphy_cur_badplcp_cnt = 0;

#ifdef WLSRVSDB
	  uint8 bank_offset = 0;
	  uint8 vsdb_switch_failed = 0;
	  uint8 vsdb_split_cntr = 0;

	  if (CHSPEC_CHANNEL(pi->radio_chanspec) == pi->srvsdb_state->sr_vsdb_channels[0]) {
	    bank_offset = 0;
	  } else if (CHSPEC_CHANNEL(pi->radio_chanspec) ==
	             pi->srvsdb_state->sr_vsdb_channels[1]) {
	    bank_offset = 1;
	  }
	  /* Assume vsdb switch failed, if no switches were recorded for both the channels */
	  vsdb_switch_failed = !(pi->srvsdb_state->num_chan_switch[0] &
	  pi->srvsdb_state->num_chan_switch[1]);

	  /* use split counter for each channel if vsdb is active and vsdb switch was successfull */
	  /* else use last 1 sec delta counter for current channel for calculations */
	  vsdb_split_cntr = (!vsdb_switch_failed) && (pi->srvsdb_state->srvsdb_active);

#endif /* WLSRVSDB */
	  /* determine delta number of rxcrs glitches */
		cur_glitch_cnt =
			wlapi_bmac_read_shm(pi->sh->physhim, MACSTAT_ADDR(pi, MCSTOFF_RXCRSGLITCH));
	  delta = cur_glitch_cnt - pi->interf->aci.pre_glitch_cnt;
	  pi->interf->aci.pre_glitch_cnt = cur_glitch_cnt;

#ifdef WLSRVSDB
	  if (vsdb_split_cntr) {
	    delta =  pi->srvsdb_state->sum_delta_crsglitch[bank_offset];
	  }
#endif /* WLSRVSDB */
	  if (ISNPHY(pi) || ISHTPHY(pi) || ISACPHY(pi)) {

		/* compute the rxbadplcp  */
		cur_badplcp_cnt = wlapi_bmac_read_shm(pi->sh->physhim,
			MACSTAT_ADDR(pi, MCSTOFF_RXBADPLCP));
	    badplcp_delta = cur_badplcp_cnt - pi->interf->pre_badplcp_cnt;
	    pi->interf->pre_badplcp_cnt = cur_badplcp_cnt;

#ifdef WLSRVSDB
	    if (vsdb_split_cntr) {
	      badplcp_delta = pi->srvsdb_state->sum_delta_prev_badplcp[bank_offset];
	    }
#endif /* WLSRVSDB */
	    /* determine delta number of bphy rx crs glitches */
		bphy_cur_glitch_cnt = wlapi_bmac_read_shm(pi->sh->physhim,
			MACSTAT_ADDR(pi, MCSTOFF_BPHYGLITCH));
	    bphy_delta = bphy_cur_glitch_cnt - pi->interf->noise.bphy_pre_glitch_cnt;
	    pi->interf->noise.bphy_pre_glitch_cnt = bphy_cur_glitch_cnt;

#ifdef WLSRVSDB
	    if (vsdb_split_cntr) {
	      bphy_delta = pi->srvsdb_state->sum_delta_bphy_crsglitch[bank_offset];
	    }
#endif /* WLSRVSDB */
	    if (CHSPEC_IS2G(pi->radio_chanspec)) {
	      /* ofdm glitches is what we will be using */
	      ofdm_delta = delta - bphy_delta;
	    } else {
	      ofdm_delta = delta;
	    }

	    /* compute bphy rxbadplcp */
			bphy_cur_badplcp_cnt = wlapi_bmac_read_shm(pi->sh->physhim,
				MACSTAT_ADDR(pi, MCSTOFF_BPHY_BADPLCP));
	    bphy_badplcp_delta = bphy_cur_badplcp_cnt -
	      pi->interf->bphy_pre_badplcp_cnt;
	    pi->interf->bphy_pre_badplcp_cnt = bphy_cur_badplcp_cnt;

#ifdef WLSRVSDB
	    if (vsdb_split_cntr) {
	      bphy_badplcp_delta = pi->srvsdb_state->sum_delta_prev_bphy_badplcp[bank_offset];
	    }

#endif /* WLSRVSDB */

	    /* ofdm bad plcps is what we will be using */
	    if (CHSPEC_IS2G(pi->radio_chanspec)) {
	      ofdm_badplcp_delta = badplcp_delta - bphy_badplcp_delta;
	    } else {
	      ofdm_badplcp_delta = badplcp_delta;
	    }
	  }

	  if (ISNPHY(pi) || ISHTPHY(pi)) {
	    /* if we aren't suppose to update yet, don't */
	    if (pi->interf->scanroamtimer != 0) {
	      return;
	    }

	  }
	} else {
		pi->interf->cca_stats_func_called = FALSE;
		/* Normalizing the statistics per second */
		delta = pi->interf->cca_stats_total_glitch * 1000 /
		        pi->interf->cca_stats_mbsstime;
		bphy_delta = pi->interf->cca_stats_bphy_glitch * 1000 /
		        pi->interf->cca_stats_mbsstime;
		ofdm_delta = delta - bphy_delta;
		badplcp_delta = pi->interf->cca_stats_total_badplcp * 1000 /
		        pi->interf->cca_stats_mbsstime;
		bphy_badplcp_delta = pi->interf->cca_stats_bphy_badplcp * 1000 /
		        pi->interf->cca_stats_mbsstime;
		ofdm_badplcp_delta = badplcp_delta - bphy_badplcp_delta;
	}

	if (delta >= 0) {
		/* evict old value */
		pi->interf->aci.ma_total -= pi->interf->aci.ma_list[pi->interf->aci.ma_index];

		/* admit new value */
		pi->interf->aci.ma_total += (uint16) delta;
		pi->interf->aci.glitch_ma_previous = pi->interf->aci.glitch_ma;
		if ((ISNPHY(pi) && NREV_LE(pi->pubpi->phy_rev, 15)) || ISHTPHY(pi) || ISACPHY(pi)) {
			pi->interf->aci.glitch_ma = pi->interf->aci.ma_total /
			        PHY_NOISE_MA_WINDOW_SZ;
		} else {
			pi->interf->aci.glitch_ma = pi->interf->aci.ma_total / MA_WINDOW_SZ;
		}

		pi->interf->aci.ma_list[pi->interf->aci.ma_index++] = (uint16) delta;
		if ((ISNPHY(pi) && NREV_LE(pi->pubpi->phy_rev, 15)) || ISHTPHY(pi) || ISACPHY(pi)) {
			if (pi->interf->aci.ma_index >= PHY_NOISE_MA_WINDOW_SZ)
				pi->interf->aci.ma_index = 0;
		} else {
			if (pi->interf->aci.ma_index >= MA_WINDOW_SZ)
				pi->interf->aci.ma_index = 0;
		}
	}


	if (ISNPHY(pi) || ISHTPHY(pi) || ISACPHY(pi)) {
		if (badplcp_delta >= 0) {
			pi->interf->badplcp_ma_total -=
				pi->interf->badplcp_ma_list[pi->interf->badplcp_ma_index];
			pi->interf->badplcp_ma_total += (uint16) badplcp_delta;
			pi->interf->badplcp_ma_previous = pi->interf->badplcp_ma;

			if ((ISNPHY(pi) && NREV_LE(pi->pubpi->phy_rev, 15)) ||
				ISHTPHY(pi) || ISACPHY(pi)) {
				pi->interf->badplcp_ma =
				        pi->interf->badplcp_ma_total / PHY_NOISE_MA_WINDOW_SZ;
			} else {
				pi->interf->badplcp_ma =
					pi->interf->badplcp_ma_total / MA_WINDOW_SZ;
			}

			pi->interf->badplcp_ma_list[pi->interf->badplcp_ma_index++] =
				(uint16) badplcp_delta;
			if ((ISNPHY(pi) && NREV_LE(pi->pubpi->phy_rev, 15)) ||
				ISHTPHY(pi) || ISACPHY(pi)) {
				if (pi->interf->badplcp_ma_index >= PHY_NOISE_MA_WINDOW_SZ)
					pi->interf->badplcp_ma_index = 0;
			} else {
				if (pi->interf->badplcp_ma_index >= MA_WINDOW_SZ)
					pi->interf->badplcp_ma_index = 0;
			}
		}

		if ((CHSPEC_IS5G(pi->radio_chanspec) && (ofdm_delta >= 0)) ||
			(CHSPEC_IS2G(pi->radio_chanspec) && (delta >= 0) && (bphy_delta >= 0))) {
			pi->interf->noise.ofdm_ma_total -= pi->interf->noise.
					ofdm_glitch_ma_list[pi->interf->noise.ofdm_ma_index];
			pi->interf->noise.ofdm_ma_total += (uint16) ofdm_delta;
			pi->interf->noise.ofdm_glitch_ma_previous =
				pi->interf->noise.ofdm_glitch_ma;
			pi->interf->noise.ofdm_glitch_ma =
				pi->interf->noise.ofdm_ma_total / PHY_NOISE_MA_WINDOW_SZ;
			pi->interf->noise.ofdm_glitch_ma_list[pi->interf->noise.ofdm_ma_index++] =
				(uint16) ofdm_delta;
			if (pi->interf->noise.ofdm_ma_index >= PHY_NOISE_MA_WINDOW_SZ)
				pi->interf->noise.ofdm_ma_index = 0;
		}

		if (bphy_delta >= 0) {
			pi->interf->noise.bphy_ma_total -= pi->interf->noise.
					bphy_glitch_ma_list[pi->interf->noise.bphy_ma_index];
			pi->interf->noise.bphy_ma_total += (uint16) bphy_delta;
			pi->interf->noise.bphy_glitch_ma_previous =
				pi->interf->noise.bphy_glitch_ma;
			pi->interf->noise.bphy_glitch_ma =
				pi->interf->noise.bphy_ma_total / PHY_NOISE_MA_WINDOW_SZ;
			pi->interf->noise.bphy_glitch_ma_list[pi->interf->noise.bphy_ma_index++] =
				(uint16) bphy_delta;
			if (pi->interf->noise.bphy_ma_index >= PHY_NOISE_MA_WINDOW_SZ)
				pi->interf->noise.bphy_ma_index = 0;
		}

		if ((CHSPEC_IS5G(pi->radio_chanspec) && (ofdm_badplcp_delta >= 0)) ||
			(CHSPEC_IS2G(pi->radio_chanspec) && (badplcp_delta >= 0) &&
			(bphy_badplcp_delta >= 0))) {
			pi->interf->noise.ofdm_badplcp_ma_total -= pi->interf->noise.
				ofdm_badplcp_ma_list[pi->interf->noise.ofdm_badplcp_ma_index];
			pi->interf->noise.ofdm_badplcp_ma_total += (uint16) ofdm_badplcp_delta;
			pi->interf->noise.ofdm_badplcp_ma_previous =
				pi->interf->noise.ofdm_badplcp_ma;
			pi->interf->noise.ofdm_badplcp_ma =
				pi->interf->noise.ofdm_badplcp_ma_total / PHY_NOISE_MA_WINDOW_SZ;
			pi->interf->noise.ofdm_badplcp_ma_list
				[pi->interf->noise.ofdm_badplcp_ma_index++] =
				(uint16) ofdm_badplcp_delta;
			if (pi->interf->noise.ofdm_badplcp_ma_index >= PHY_NOISE_MA_WINDOW_SZ)
				pi->interf->noise.ofdm_badplcp_ma_index = 0;
		}

		if (bphy_badplcp_delta >= 0) {
			pi->interf->noise.bphy_badplcp_ma_total -= pi->interf->noise.
				bphy_badplcp_ma_list[pi->interf->noise.bphy_badplcp_ma_index];
			pi->interf->noise.bphy_badplcp_ma_total += (uint16) bphy_badplcp_delta;
			pi->interf->noise.bphy_badplcp_ma_previous = pi->interf->noise.
				bphy_badplcp_ma;
			pi->interf->noise.bphy_badplcp_ma =
				pi->interf->noise.bphy_badplcp_ma_total / PHY_NOISE_MA_WINDOW_SZ;
			pi->interf->noise.bphy_badplcp_ma_list
					[pi->interf->noise.bphy_badplcp_ma_index++] =
					(uint16) bphy_badplcp_delta;
			if (pi->interf->noise.bphy_badplcp_ma_index >= PHY_NOISE_MA_WINDOW_SZ)
				pi->interf->noise.bphy_badplcp_ma_index = 0;
		}
	}

	if (((ISNPHY(pi) && NREV_GE(pi->pubpi->phy_rev, 16)) || ISHTPHY(pi)) &&
	    (pi->sh->interference_mode == WLAN_AUTO_W_NOISE ||
	     pi->sh->interference_mode == NON_WLAN)) {
		PHY_ACI(("wlc_phy_aci_update_ma: ACI= %s, rxglitch_ma= %d,"
			" badplcp_ma= %d, ofdm_glitch_ma= %d, bphy_glitch_ma=%d,"
			" ofdm_badplcp_ma= %d, bphy_badplcp_ma=%d, crsminpwr index= %d,"
			" init gain= 0x%x, channel= %d\n",
			(pi->aci_state & ACI_ACTIVE) ? "Active" : "Inactive",
			pi->interf->aci.glitch_ma,
			pi->interf->badplcp_ma,
			pi->interf->noise.ofdm_glitch_ma,
			pi->interf->noise.bphy_glitch_ma,
			pi->interf->noise.ofdm_badplcp_ma,
			pi->interf->noise.bphy_badplcp_ma,
			pi->interf->crsminpwr_index,
			pi->interf->init_gain_core1, CHSPEC_CHANNEL(pi->radio_chanspec)));
	} else {
		PHY_ACI(("wlc_phy_aci_update_ma: ave glitch %d, ACI is %s, delta is %d\n",
		pi->interf->aci.glitch_ma,
		(pi->aci_state & ACI_ACTIVE) ? "Active" : "Inactive", delta));
	}
#ifdef WLSRVSDB
	/* Clear out cumulatiove cntrs after 1 sec */
	/* reset both chan info becuase its a fresh start after every 1 sec */
	if (pi->srvsdb_state->srvsdb_active) {
		bzero(pi->srvsdb_state->num_chan_switch, 2 * sizeof(uint8));
		bzero(pi->srvsdb_state->sum_delta_crsglitch, 2 * sizeof(uint32));
		bzero(pi->srvsdb_state->sum_delta_bphy_crsglitch, 2 * sizeof(uint32));
		bzero(pi->srvsdb_state->sum_delta_prev_badplcp, 2 * sizeof(uint32));
		bzero(pi->srvsdb_state->sum_delta_prev_bphy_badplcp, 2 * sizeof(uint32));
	}
#endif /* WLSRVSDB */
}


void
wlc_phy_aci_upd(phy_info_t *pi)
{
	bool desense_gt_4;
	int glit_plcp_sum;
#ifdef WLSRVSDB
	uint8 offset = 0;
	uint8 vsdb_switch_failed = 0;

	if (pi->srvsdb_state->srvsdb_active) {
		/* Assume vsdb switch failure, if no chan switches were recorded in last 1 sec */
		vsdb_switch_failed = !(pi->srvsdb_state->num_chan_switch[0] &
			pi->srvsdb_state->num_chan_switch[1]);
		if (CHSPEC_CHANNEL(pi->radio_chanspec) ==
			pi->srvsdb_state->sr_vsdb_channels[0]) {
			offset = 0;
		} else if (CHSPEC_CHANNEL(pi->radio_chanspec) ==
			pi->srvsdb_state->sr_vsdb_channels[1]) {
			offset = 1;
		}

		/* return if vsdb switching was active and time spent in current channel */
		/* is less than 1 sec */
		/* If vsdb switching had failed, it could be in a  deadlock */
		/* situation because of noise/aci */
		/* So continue with aci mitigation even though delta timers show less than 1 sec */

		if ((pi->srvsdb_state->sum_delta_timer[offset] < (1000 * 1000)) &&
			!vsdb_switch_failed) {

			bzero(pi->srvsdb_state->num_chan_switch, 2 * sizeof(uint8));
			return;
		}
		PHY_INFORM(("Enter ACI mitigation for chan %x  since %d ms of time has expired\n",
			pi->srvsdb_state->sr_vsdb_channels[offset],
			pi->srvsdb_state->sum_delta_timer[offset]/1000));

		/* reset the timers after an effective 1 sec duration in the channel */
		bzero(pi->srvsdb_state->sum_delta_timer, 2 * sizeof(uint32));

		/* If enetering aci mitigation scheme, we need a save of */
		/* previous pi structure while doing switch */
		pi->srvsdb_state->swbkp_snapshot_valid[offset] = 0;
	}
#endif /* WLSRVSDB */
	wlc_phy_aci_update_ma(pi);


	if (ISACPHY(pi)) {
		if (!(ACPHY_ENABLE_FCBS_HWACI(pi)) || ACPHY_HWACI_WITH_DESENSE_ENG(pi)) {
			if ((pi->sh->interference_mode & ACPHY_ACI_GLITCHBASED_DESENSE) != 0) {
				wlc_phy_desense_aci_engine_acphy(pi);
			}
		}
		if (!ACPHY_ENABLE_FCBS_HWACI(pi)) {
			if ((pi->sh->interference_mode & (ACPHY_ACI_HWACI_PKTGAINLMT |
				ACPHY_ACI_W2NB_PKTGAINLMT)) != 0) {
				wlc_phy_hwaci_engine_acphy(pi);
			}
		}
	} else {
		switch (pi->sh->interference_mode) {
		case NON_WLAN:
			/* NON_WLAN NPHY */
			if (ISNPHY(pi) && NREV_GE(pi->pubpi->phy_rev, 16)) {
				/* run noise mitigation only */
				wlc_phy_noisemode_upd_nphy(pi);
			} else if (ISHTPHY(pi)) {
				/* run noise mitigation only */
				wlc_phy_noisemode_upd_htphy(pi);
			} else if (ISNPHY(pi) && NREV_LE(pi->pubpi->phy_rev, 15)) {
				wlc_phy_noisemode_upd(pi);
			}
			break;
		case WLAN_AUTO:
			if (ISLCN40PHY(pi) && ((CHIPID(pi->sh->chip) == BCM43142_CHIP_ID) ||
				(CHIPID(pi->sh->chip) == BCM43340_CHIP_ID) ||
				(CHIPID(pi->sh->chip) == BCM43341_CHIP_ID))) {
				wlc_lcn40phy_aci_upd(pi);
			}
			else if (ISNPHY(pi) || ISHTPHY(pi)) {
				if (ASSOC_INPROG_PHY(pi))
					break;
#ifdef NOISE_CAL_LCNXNPHY
				if ((NREV_IS(pi->pubpi->phy_rev, LCNXN_BASEREV) ||
					NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))) {
						wlc_phy_aci_noise_measure_nphy(pi, TRUE);
				}
				else
#endif
				{
					/* 5G band not supported yet */
					if (CHSPEC_IS5G(pi->radio_chanspec))
						break;

					if (PUB_NOT_ASSOC(pi)) {
						/* not associated:  do not run aci routines */
						break;
					}
					PHY_ACI(("Interf Mode 3,"
						" pi->interf->aci.glitch_ma = %d\n",
						pi->interf->aci.glitch_ma));

					/* Attempt to enter ACI mode if not already active */
					/* only run this code if associated */
					if (!(pi->aci_state & ACI_ACTIVE)) {
						if ((pi->sh->now  % NPHY_ACI_CHECK_PERIOD) == 0) {

							if ((pi->interf->aci.glitch_ma +
								pi->interf->badplcp_ma) >=
								pi->interf->aci.enter_thresh) {
								if (ISNPHY(pi)) {
								wlc_phy_acimode_upd_nphy(pi);
								} else if (ISHTPHY(pi)) {
								wlc_phy_acimode_upd_htphy(pi);
								}
							}
						}
					} else {
						if (((pi->sh->now - pi->aci_start_time) %
							pi->aci_exit_check_period) == 0) {
							if (ISNPHY(pi)) {
								wlc_phy_acimode_upd_nphy(pi);
							} else if (ISHTPHY(pi)) {
								wlc_phy_acimode_upd_htphy(pi);
							}
						}
					}
				}
			}
			break;
		case WLAN_AUTO_W_NOISE:
			if (ISNPHY(pi) || ISHTPHY(pi)) {
#ifdef NOISE_CAL_LCNXNPHY
				if ((NREV_IS(pi->pubpi->phy_rev, LCNXN_BASEREV) ||
					NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))) {
						wlc_phy_aci_noise_measure_nphy(pi, TRUE);
				}
				else
#endif
				{
					/* 5G band not supported yet */
					if (CHSPEC_IS5G(pi->radio_chanspec))
						break;

					/* only do this for 4322 and future revs */
					if (ISNPHY(pi) && NREV_GE(pi->pubpi->phy_rev, 16)) {
						/* Attempt to enter ACI mode if
						 * not already active
						 */
						wlc_phy_aci_noise_upd_nphy(pi);
					} else if (ISNPHY(pi) && NREV_LE(pi->pubpi->phy_rev, 15)) {
					  PHY_ACI(("Interf Mode 4\n"));
					  desense_gt_4 = (pi->interf->noise.ofdm_desense >= 4 ||
					      pi->interf->noise.bphy_desense >= 4);
					  glit_plcp_sum = pi->interf->aci.glitch_ma +
					        pi->interf->badplcp_ma;
					  if (!(pi->aci_state & ACI_ACTIVE)) {
					    if ((pi->sh->now  % NPHY_ACI_CHECK_PERIOD) == 0) {
					      if ((glit_plcp_sum >=
					        pi->interf->aci.enter_thresh) || desense_gt_4) {
						if (ISNPHY(pi)) {
						  wlc_phy_acimode_upd_nphy(pi);
						}
					      }
					    }
					  } else {
					    if (((pi->sh->now - pi->aci_start_time) %
						 pi->aci_exit_check_period) == 0) {
					      if (ISNPHY(pi)) {
						wlc_phy_acimode_upd_nphy(pi);
					      }
					    }
					  }

					  wlc_phy_noisemode_upd(pi);

					} else if (ISHTPHY(pi)) {
						wlc_phy_aci_noise_upd_htphy(pi);
					}
				}
			} else if (ISLCN40PHY(pi) && ((CHIPID(pi->sh->chip) == BCM43142_CHIP_ID) ||
				(CHIPID(pi->sh->chip) == BCM43340_CHIP_ID) ||
				(CHIPID(pi->sh->chip) == BCM43341_CHIP_ID))) {
				wlc_lcn40phy_aci_upd(pi);
			}
			break;

		default:
			break;
	}
	}
}

void
wlc_phy_noisemode_upd(phy_info_t *pi)
{

	int8 bphy_desense_delta = 0, ofdm_desense_delta = 0;
	uint16 glitch_badplcp_sum;

	if (CHSPEC_CHANNEL(pi->radio_chanspec) != pi->interf->curr_home_channel)
		return;

	if (pi->interf->scanroamtimer != 0) {
		/* have not updated moving averages */
		return;
	}

	/* BPHY desense. Sets  pi->interf->noise.bphy_desense in side the func */
	/* Should be run only if associated */
	if (!PUB_NOT_ASSOC(pi) && CHSPEC_IS2G(pi->radio_chanspec)) {

		glitch_badplcp_sum = pi->interf->noise.bphy_glitch_ma +
			pi->interf->noise.bphy_badplcp_ma;

		bphy_desense_delta = wlc_phy_cmn_noisemode_glitch_chk_adj(pi, glitch_badplcp_sum,
			&pi->interf->noise.bphy_thres);

		pi->interf->noise.bphy_desense += bphy_desense_delta;
	} else {
		pi->interf->noise.bphy_desense = 0;
	}

	glitch_badplcp_sum = pi->interf->noise.ofdm_glitch_ma + pi->interf->noise.ofdm_badplcp_ma;
	/* OFDM desense. Sets  pi->interf->noise.ofdm_desense in side the func */
	ofdm_desense_delta = wlc_phy_cmn_noisemode_glitch_chk_adj(pi, glitch_badplcp_sum,
		&pi->interf->noise.ofdm_thres);
	pi->interf->noise.ofdm_desense += ofdm_desense_delta;

	wlc_phy_cmn_noise_limit_desense(pi);


	if (bphy_desense_delta || ofdm_desense_delta) {
		if (ISNPHY(pi)) {
			wlc_phy_bphy_ofdm_noise_hw_set_nphy(pi);
		}
	}

	PHY_ACI(("PHY_CMN:  {GL_MA, BP_MA} OFDM {%d, %d},"
	         " BPHY {%d, %d}, Desense - (OFDM, BPHY) = {%d, %d} MAX_Desense {%d, %d},"
	         "channel is %d rssi = %d\n",
	         pi->interf->noise.ofdm_glitch_ma, pi->interf->noise.ofdm_badplcp_ma,
	         pi->interf->noise.bphy_glitch_ma, pi->interf->noise.bphy_badplcp_ma,
	         pi->interf->noise.ofdm_desense, pi->interf->noise.bphy_desense,
	         pi->interf->noise.max_poss_ofdm_desense, pi->interf->noise.max_poss_bphy_desense,
	         CHSPEC_CHANNEL(pi->radio_chanspec), pi->interf->rssi));

}

static int8
wlc_phy_cmn_noisemode_glitch_chk_adj(phy_info_t *pi, uint16 total_glitch_badplcp,
	noise_thresholds_t *thresholds)
{
	int8 desense_delta = 0;

	if (total_glitch_badplcp > thresholds->glitch_badplcp_low_th) {
		/* glitch count is high, could be due to inband noise */
		thresholds->high_detect_total++;
		thresholds->low_detect_total = 0;
	} else {
		/* glitch count not high */
		thresholds->high_detect_total = 0;
		thresholds->low_detect_total++;
	}

	if (thresholds->high_detect_total >= thresholds->high_detect_thresh) {
		/* we have more than glitch_th_up bphy
		 * glitches in a row. so, let's try raising the
		 * inband noise immunity
		 */
		if (total_glitch_badplcp < thresholds->glitch_badplcp_high_th) {
			/* Desense by less */
			desense_delta = thresholds->desense_lo_step;
		} else {
			/* Desense by more */
			desense_delta = thresholds->desense_hi_step;
		}
		thresholds->high_detect_total = 0;
	} else if (thresholds->low_detect_total > 0) {
		/* check to see if we can lower noise immunity */
		uint16 low_detect_total, undesense_wait, undesense_window;

		low_detect_total = thresholds->low_detect_total;
		undesense_wait = thresholds->undesense_wait;
		undesense_window = thresholds->undesense_window;

		/* Reduce the wait time to undesense if glitch count has been low for longer */
		while (undesense_wait > 1) {
			if (low_detect_total <=  undesense_window) {
				break;
			}
			low_detect_total -= undesense_window;
			/* Halve the wait time */
			undesense_wait /= 2;
		}

		if ((low_detect_total % undesense_wait) == 0) {
			/* Undesense */
			desense_delta = -1;
		}
	}
	PHY_ACI(("In %s: recomended desense = %d glitch_ma + badplcp_ma = %d,"
		"th_lo = %d, th_hi = %d \n", __FUNCTION__, desense_delta,
		total_glitch_badplcp, thresholds->glitch_badplcp_low_th,
		thresholds->glitch_badplcp_high_th));
	return desense_delta;
}

void
wlc_phy_cmn_noise_limit_desense(phy_info_t *pi)
{
	if (pi->interf->rssi != 0) {
		int8 max_desense_rssi = PHY_NOISE_DESENSE_RSSI_MAX;
		int8 desense_margin = PHY_NOISE_DESENSE_RSSI_MARGIN;

		int8 max_desense_dBm, bphy_desense_dB, ofdm_desense_dB;

		pi->interf->noise.bphy_desense = MAX(pi->interf->noise.bphy_desense, 0);
		pi->interf->noise.ofdm_desense = MAX(pi->interf->noise.ofdm_desense, 0);

		max_desense_dBm = MIN(pi->interf->rssi, max_desense_rssi) - desense_margin;

		bphy_desense_dB = max_desense_dBm - pi->interf->bphy_min_sensitivity;
		ofdm_desense_dB = max_desense_dBm - pi->interf->ofdm_min_sensitivity;

		bphy_desense_dB = MAX(bphy_desense_dB, 0);
		ofdm_desense_dB = MAX(ofdm_desense_dB, 0);

		pi->interf->noise.max_poss_bphy_desense = bphy_desense_dB;
		pi->interf->noise.max_poss_ofdm_desense = ofdm_desense_dB;

		pi->interf->noise.bphy_desense =
			MIN(bphy_desense_dB, pi->interf->noise.bphy_desense);
		pi->interf->noise.ofdm_desense =
			MIN(ofdm_desense_dB, pi->interf->noise.ofdm_desense);
	} else {
		pi->interf->noise.max_poss_bphy_desense = 0;
		pi->interf->noise.max_poss_ofdm_desense = 0;

		pi->interf->noise.bphy_desense = 0;
		pi->interf->noise.ofdm_desense = 0;
	}
}

void
wlc_phy_interf_chan_stats_update(wlc_phy_t *ppi, chanspec_t chanspec, uint32 crsglitch,
        uint32 bphy_crsglitch, uint32 badplcp, uint32 bphy_badplcp, uint32 mbsstime)
{
	phy_info_t *pi = (phy_info_t*)ppi;
	/* Doing interference update of chan stats here  */

	if (pi->interf->curr_home_channel == (CHSPEC_CHANNEL(chanspec))) {
		pi->interf->cca_stats_func_called = TRUE;
		pi->interf->cca_stats_total_glitch = crsglitch;
		pi->interf->cca_stats_bphy_glitch = bphy_crsglitch;
		pi->interf->cca_stats_total_badplcp = badplcp;
		pi->interf->cca_stats_bphy_badplcp = bphy_badplcp;
		pi->interf->cca_stats_mbsstime = mbsstime;
	}
}

#endif /* Compiling out ACI code for 4324 */

void
wlc_phy_acimode_noisemode_reset(wlc_phy_t *pih, chanspec_t chanspec,
	bool clear_aci_state, bool clear_noise_state, bool disassoc)
{
	phy_info_t *pi = (phy_info_t *)pih;
	uint channel = CHSPEC_CHANNEL(chanspec);

	pi->interf->cca_stats_func_called = FALSE;

	if (!(ISNPHY(pi) || ISHTPHY(pi)))
		return;

	if (pi->sh->interference_mode_override == TRUE)
		return;

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
}

static void
wlc_phy_noise_save(phy_info_t *pi, int8 *noise_dbm_ant, int8 *max_noise_dbm)
{
	uint8 i;

	FOREACH_CORE(pi, i) {
		/* save noise per core */
		pi->phy_noise_win[i][pi->phy_noise_index] = noise_dbm_ant[i];

		/* save the MAX for all cores */
		if (noise_dbm_ant[i] > *max_noise_dbm)
			*max_noise_dbm = noise_dbm_ant[i];
	}
	pi->phy_noise_index = MODINC_POW2(pi->phy_noise_index, PHY_NOISE_WINDOW_SZ);

	pi->sh->phy_noise_window[pi->sh->phy_noise_index] = *max_noise_dbm;
	pi->sh->phy_noise_index = MODINC_POW2(pi->sh->phy_noise_index, MA_WINDOW_SZ);
}

#ifdef NOISE_CAL_LCNXNPHY
void wlc_phy_noise_trigger_ucode(phy_info_t *pi)
{
	/* ucode assumes these shmems start with 0
	 * and ucode will not touch them in case of sampling expires
	 */
	wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP0(pi), 0);
	wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP1(pi), 0);
	wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP2(pi), 0);
	wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP3(pi), 0);
	if (D11REV_GE(pi->sh->corerev, 40)) {
		wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP4(pi), 0);
		wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP5(pi), 0);
	}

	W_REG(pi->sh->osh, &pi->regs->maccommand, MCMD_BG_NOISE);
}
#endif /* NOISE_CAL_LCNXNPHY */

void
wlc_phy_noise_sched_set(wlc_phy_t *pih, phy_bgnoise_schd_mode_t reason, bool upd)
{
	phy_info_t *pi = (phy_info_t*)pih;

	BCM_REFERENCE(reason);
	PHY_NONE(("%s: reason %d, upd %d\n", __FUNCTION__, reason, upd));

	pi->phynoise_disable = upd;
}

bool
wlc_phy_noise_sched_get(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t*)pih;

	return (pi->phynoise_disable);
}

void
wlc_phy_noise_pmstate_set(wlc_phy_t *pih, bool pmstate)
{
	phy_info_t *pi = (phy_info_t*)pih;
	pi->phynoise_pmstate = pmstate;
}

bool
wlc_phy_noise_pmstate_get(phy_info_t *pi)
{
	return pi->phynoise_pmstate;
}

#define PHY_NOISE_MAX_IDLETIME		30

void
wlc_phy_noise_sample_request(phy_info_t *pi, uint8 reason, uint8 ch)
{
	int8 noise_dbm = PHY_NOISE_FIXED_VAL_NPHY;
	bool sampling_in_progress = (pi->phynoise_state != 0);
	bool wait_for_intr = TRUE;

	PHY_NONE(("wlc_phy_noise_sample_request: state %d reason %d, channel %d\n",
	          pi->phynoise_state, reason, ch));


	/* This is needed to make sure that the crsmin cal happens
	*   even if sampling is in progress
	*/
	if (ISACPHY(pi)) {
		pi->u.pi_acphy->trigger_crsmin_cal = FALSE;
		if (reason == PHY_NOISE_STATE_CRSMINCAL && sampling_in_progress) {
			pi->u.pi_acphy->trigger_crsmin_cal = TRUE;
		}
	}

	/* since polling is atomic, sampling_in_progress equals to interrupt sampling ongoing
	 *  In these collision cases, always yield and wait interrupt to finish, where the results
	 *  maybe be sharable if channel matches in common callback progressing.
	 */
	if (sampling_in_progress)
		return;

	switch (reason) {
	case PHY_NOISE_SAMPLE_MON:

		pi->phynoise_chan_watchdog = ch;
		pi->phynoise_state |= PHY_NOISE_STATE_MON;

		break;

	case PHY_NOISE_SAMPLE_EXTERNAL:

		pi->phynoise_state |= PHY_NOISE_STATE_EXTERNAL;
		break;

	case PHY_NOISE_SAMPLE_CRSMINCAL:

		if (ISACPHY(pi)) {
			pi->phynoise_state |= PHY_NOISE_STATE_CRSMINCAL;
			break;
		}

	default:
		ASSERT(0);
		break;
	}

	/* start test, save the timestamp to recover in case ucode gets stuck */
	pi->phynoise_now = pi->sh->now;

	/* Fixed noise, don't need to do the real measurement */
	if (pi->phy_fixed_noise) {
		if (ISNPHY(pi) || ISHTPHY(pi)) {
			uint8 i;
			FOREACH_CORE(pi, i) {
				pi->phy_noise_win[i][pi->phy_noise_index] =
					PHY_NOISE_FIXED_VAL_NPHY;
			}
			pi->phy_noise_index = MODINC_POW2(pi->phy_noise_index,
				PHY_NOISE_WINDOW_SZ);
			/* legacy noise is the max of antennas */
			noise_dbm = PHY_NOISE_FIXED_VAL_NPHY;
		} else {
			/* all other PHY */
			noise_dbm = PHY_NOISE_FIXED_VAL;
		}

		wait_for_intr = FALSE;
		goto done;
	}

	if (ISLCN40PHY(pi)) {
		wlc_lcn40phy_noise_measure_start(pi, FALSE);
	} else if (ISLCN20PHY(pi)) {
		/* LCN20 don't have noise cal yet */
		wait_for_intr = FALSE;
		goto done;
	} else if (ISNPHY(pi) || ISHTPHY(pi) || ISACPHY(pi)) {
		if (!pi->phynoise_polling || (reason == PHY_NOISE_SAMPLE_EXTERNAL) ||
			(reason == PHY_NOISE_SAMPLE_CRSMINCAL)) {

			/* If noise mmt disabled due to LPAS active, do not initiate one */
			if (pi->phynoise_disable)
				return;

			/* Do not do noise mmt if in PM State */
			if (wlc_phy_noise_pmstate_get(pi) &&
				!wlapi_phy_awdl_is_on(pi->sh->physhim)) {
				/* Make sure that you do it every PHY_NOISE_MAX_IDLETIME atleast */
				if ((pi->sh->now - pi->phynoise_lastmmttime)
						< PHY_NOISE_MAX_IDLETIME)
					return;
			}
			/* Note current noise request time */
			pi->phynoise_lastmmttime = pi->sh->now;

			/* ucode assumes these shmems start with 0
			 * and ucode will not touch them in case of sampling expires
			 */
			wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP0(pi), 0);
			wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP1(pi), 0);
			wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP2(pi), 0);
			wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP3(pi), 0);

			if (D11REV_GE(pi->sh->corerev, 40)) {
				wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP4(pi), 0);
				wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP5(pi), 0);
			}

			W_REG(pi->sh->osh, &pi->regs->maccommand, MCMD_BG_NOISE);

		} else {

			/* polling mode */
			phy_iq_est_t est[PHY_CORE_MAX];
			uint32 cmplx_pwr[PHY_CORE_MAX];
			int8 noise_dbm_ant[PHY_CORE_MAX];
			uint16 log_num_samps, num_samps, classif_state = 0;
			uint8 wait_time = 32;
			uint8 wait_crs = 0;
			uint8 i;

			bzero((uint8 *)est, sizeof(est));
			bzero((uint8 *)cmplx_pwr, sizeof(cmplx_pwr));
			bzero((uint8 *)noise_dbm_ant, sizeof(noise_dbm_ant));

			/* choose num_samps to be some power of 2 */
			log_num_samps = PHY_NOISE_SAMPLE_LOG_NUM_NPHY;
			num_samps = 1 << log_num_samps;

			/* suspend MAC, get classifier settings, turn it off
			 * get IQ power measurements
			 * restore classifier settings and reenable MAC ASAP
			*/
			wlapi_suspend_mac_and_wait(pi->sh->physhim);

			if (ISNPHY(pi)) {
				classif_state = wlc_phy_classifier_nphy(pi, 0, 0);
				wlc_phy_classifier_nphy(pi, 3, 0);
				wlc_phy_rx_iq_est_nphy(pi, est, num_samps, wait_time, wait_crs);
				wlc_phy_classifier_nphy(pi, NPHY_ClassifierCtrl_classifierSel_MASK,
					classif_state);
			} else if (ISHTPHY(pi)) {
				classif_state = wlc_phy_classifier_htphy(pi, 0, 0);
				wlc_phy_classifier_htphy(pi, 3, 0);
				wlc_phy_rx_iq_est_htphy(pi, est, num_samps, wait_time, wait_crs);
				wlc_phy_classifier_htphy(pi,
					HTPHY_ClassifierCtrl_classifierSel_MASK, classif_state);
			} else if (ISACPHY(pi)) {
			  classif_state = wlc_phy_classifier_acphy(pi, 0, 0);
			  wlc_phy_classifier_acphy(pi, ACPHY_ClassifierCtrl_classifierSel_MASK, 0);
			  wlc_phy_rx_iq_est_acphy(pi, est, num_samps, wait_time, wait_crs, FALSE);
				wlc_phy_classifier_acphy(pi,
					ACPHY_ClassifierCtrl_classifierSel_MASK, classif_state);
			} else {
				ASSERT(0);
			}

			wlapi_enable_mac(pi->sh->physhim);

			/* sum I and Q powers for each core, average over num_samps */
			FOREACH_CORE(pi, i)
				cmplx_pwr[i] = (est[i].i_pwr + est[i].q_pwr) >> log_num_samps;

			/* pi->phy_noise_win per antenna is updated inside */
			wlc_phy_noise_calc(pi, cmplx_pwr, noise_dbm_ant, 0);
			wlc_phy_noise_save(pi, noise_dbm_ant, &noise_dbm);

			wait_for_intr = FALSE;
		}
	} else {
		PHY_TRACE(("Fallthru nophy\n"));
		/* No phy ?? */
		return;
	}

done:
	/* if no interrupt scheduled, populate noise results now */
	if (!wait_for_intr)
		wlc_phy_noise_cb(pi, ch, noise_dbm);
}

void
wlc_phy_noise_sample_request_external(wlc_phy_t *pih)
{
	uint8  channel;

	channel = CHSPEC_CHANNEL(phy_utils_get_chanspec((phy_info_t *)pih));

	wlc_phy_noise_sample_request((phy_info_t *)pih, PHY_NOISE_SAMPLE_EXTERNAL, channel);
}

void
wlc_phy_noise_sample_request_crsmincal(wlc_phy_t *pih)
{
	uint8  channel;

	channel = CHSPEC_CHANNEL(phy_utils_get_chanspec((phy_info_t *)pih));

	wlc_phy_noise_sample_request((phy_info_t *)pih, PHY_NOISE_SAMPLE_CRSMINCAL, channel);
}

void
wlc_phy_noise_cb(phy_info_t *pi, uint8 channel, int8 noise_dbm)
{
	if (!pi->phynoise_state)
		return;

	PHY_NONE(("wlc_phy_noise_cb: state %d noise %d channel %d\n",
	          pi->phynoise_state, noise_dbm, channel));
	if (pi->phynoise_state & PHY_NOISE_STATE_MON) {
		if (pi->phynoise_chan_watchdog == channel) {
			pi->sh->phy_noise_window[pi->sh->phy_noise_index] = noise_dbm;
			pi->sh->phy_noise_index = MODINC(pi->sh->phy_noise_index, MA_WINDOW_SZ);
		}
		pi->phynoise_state &= ~PHY_NOISE_STATE_MON;
	}

	if (pi->phynoise_state & PHY_NOISE_STATE_EXTERNAL) {
		pi->phynoise_state &= ~PHY_NOISE_STATE_EXTERNAL;
		wlapi_noise_cb(pi->sh->physhim, channel, noise_dbm);
	}

	if (ISACPHY(pi)) {
		if (pi->phynoise_state & PHY_NOISE_STATE_CRSMINCAL)
			pi->phynoise_state &= ~PHY_NOISE_STATE_CRSMINCAL;
	}
}

int8
wlc_phy_noise_read_shmem(phy_info_t *pi)
{
	uint32 cmplx_pwr[PHY_CORE_MAX];
	int8 noise_dbm_ant[PHY_CORE_MAX];
	uint16 lo, hi;
	uint32 cmplx_pwr_tot = 0;
	int8 noise_dbm = PHY_NOISE_FIXED_VAL_NPHY;
	uint8 core;
	uint16 map1 = 0;
	uint8 noise_during_crs = 0;
	uint8 noise_during_lte = 0;

	ASSERT(PHYCORENUM(pi->pubpi->phy_corenum) <= PHY_CORE_MAX);
	bzero((uint8 *)cmplx_pwr, sizeof(cmplx_pwr));
	bzero((uint8 *)noise_dbm_ant, sizeof(noise_dbm_ant));

	map1 = wlapi_bmac_read_shm(pi->sh->physhim, M_PWRIND_MAP1(pi));

	/* If PHYCRS was seen during noise measurement, skip this measurement */
	if ((map1 & 0x8000)) {
		pi->phynoise_state = 0;
		PHY_INFORM(("%s: PHY CRS seen during noise measurement\n", __FUNCTION__));
		return -1;
	}

	/* If IQ_Est time out was seen */
	if ((map1 & 0x2000)) {
		PHY_INFORM(("%s: IQ Est timeout during noise measurement\n", __FUNCTION__));
		return -1;
	}

	/* read SHM, reuse old corerev PWRIND since we are tight on SHM pool */
	FOREACH_CORE(pi, core) {
		lo = wlapi_bmac_read_shm(pi->sh->physhim, M_PWRIND_MAP(pi, 2*core));
		hi = wlapi_bmac_read_shm(pi->sh->physhim, M_PWRIND_MAP(pi, 2*core+1));
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
		    ACMAJORREV_37(pi->pubpi->phy_rev)) {
		  noise_during_crs =
		    (wlapi_bmac_read_shm(pi->sh->physhim, M_PWRIND_MAP1(pi)) >> 15) & 0x1;
		  noise_during_lte =
		    (wlapi_bmac_read_shm(pi->sh->physhim, M_PWRIND_MAP1(pi)) >> 14) & 0x1;
		  if (core == 0) {
		    hi = hi & ~(3<<14); /* Clear bit 14 and 15 */
		  }
		}
		cmplx_pwr[core] = (hi << 16) + lo;
		cmplx_pwr_tot += cmplx_pwr[core];
		if (cmplx_pwr[core] == 0) {
			noise_dbm_ant[core] = PHY_NOISE_FIXED_VAL_NPHY;
		} else
			cmplx_pwr[core] >>= PHY_NOISE_SAMPLE_LOG_NUM_UCODE;
			/* FIX ME: this is temp WAR for the 4th core crsmin cal */
			if (core == 3) {
				cmplx_pwr[core] = cmplx_pwr[core-1];
			}
	}

	if (cmplx_pwr_tot == 0)
		PHY_INFORM(("wlc_phy_noise_sample_nphy_compute: timeout in ucode\n"));
	else {
#ifdef NOISE_CAL_LCNXNPHY
		if ((NREV_IS(pi->pubpi->phy_rev, LCNXN_BASEREV) ||
			NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 2)) &&
			ISNPHY(pi)) {
			wlc_phy_noisepwr_nphy(pi, cmplx_pwr);
		}
#endif
		wlc_phy_noise_calc(pi, cmplx_pwr, noise_dbm_ant, 0);
		/* Putting check here to see if crsmin cal needs to be triggered */
		if (ISACPHY(pi)) {
			if ((pi->phynoise_state == PHY_NOISE_STATE_CRSMINCAL) ||
			pi->u.pi_acphy->trigger_crsmin_cal) {
				wlc_phy_crs_min_pwr_cal_acphy(pi, PHY_CRS_RUN_AUTO);
			}
		}
	}

	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
	    ACMAJORREV_37(pi->pubpi->phy_rev)) {
	  if ((!noise_during_lte) && (!noise_during_crs)) {
		  wlc_phy_noise_save(pi, noise_dbm_ant, &noise_dbm);
		} else {
		  noise_dbm	= noise_dbm_ant[0];
		}
	} else {
		wlc_phy_noise_save(pi, noise_dbm_ant, &noise_dbm);
	}

	/* legacy noise is the max of antennas */
	return noise_dbm;

}

/* ucode finished phy noise measurement and raised interrupt */
void
wlc_phy_noise_sample_intr(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t*)pih;
	uint16 jssi_aux;
	uint8 channel = 0;
	int8 noise_dbm = PHY_NOISE_FIXED_VAL_NPHY;
	if (ISLCN40PHY(pi)) {
	  wlc_lcn40phy_noise_measure(pi);
	} else if (ISNPHY(pi) || ISHTPHY(pi) || ISACPHY(pi)) {
		/* copy of the M_CURCHANNEL, just channel number plus 2G/5G flag */
		jssi_aux = wlapi_bmac_read_shm(pi->sh->physhim, M_JSSI_AUX(pi));
		channel = jssi_aux & D11_CURCHANNEL_MAX;
		noise_dbm = wlc_phy_noise_read_shmem(pi);
			} else {
		PHY_ERROR(("%s, unsupported phy type\n", __FUNCTION__));
		ASSERT(0);
	}

	if (!ISLCNCOMMONPHY(pi))
	  {
	    /* rssi dbm computed, invoke all callbacks */
	    wlc_phy_noise_cb(pi, channel, noise_dbm);
	  }
}

int8
wlc_phy_noise_avg(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t *)pih;
	int tot = 0;
	int i = 0;

	if (ISLCN40PHY(pi)) {
		phy_info_lcn40phy_t *pi_lcn40 = pi->u.pi_lcn40phy;
		int8 gain_adjust = 0;
		if (!pi->sh->clk)
			return 0;
		if (pi_lcn40->noise_iqest_en) {
			uint16 lcn40phyregs_shm_addr =
				2 * wlapi_bmac_read_shm(pi->sh->physhim, M_LCN40PHYREGS_PTR(pi));
			int16 noise_dBm_ucode, noise_dBm;
			wlc_lcn40phy_trigger_noise_iqest(pi);
			noise_dBm_ucode = (int16)wlapi_bmac_read_shm(pi->sh->physhim,
				lcn40phyregs_shm_addr + M_NOISE_IQPWR_DB_OFFSET(pi));
			if (noise_dBm_ucode)
				gain_adjust = wlc_lcn40phy_get_noise_iqest_gainadjust(pi);
			noise_dBm = noise_dBm_ucode + gain_adjust;
			tot = noise_dBm;
			PHY_INFORM(("noise_dBm=%d, noise_iq=%d, noise_dBm_ucode=%d\n",
				noise_dBm, (int16)wlapi_bmac_read_shm(pi->sh->physhim,
				lcn40phyregs_shm_addr + M_NOISE_IQPWR_OFFSET(pi)),
					noise_dBm_ucode));
		}
	} else {
		for (i = 0; i < MA_WINDOW_SZ; i++)
			tot += pi->sh->phy_noise_window[i];

		tot /= MA_WINDOW_SZ;
	}
	return (int8)tot;
}

int8
wlc_phy_noise_avg_per_antenna(wlc_phy_t *pih, int coreidx)
{
	phy_info_t *pi = (phy_info_t *)pih;
	uint8 i, idx;
	int32 tot = 0;
	int8 result = 0;

	if (!pi->sh->up)
		return 0;

	/* checking coreidx to prevent overrunning
	 * phy_noise_win array
	 */
	if (((uint)coreidx) >= PHY_CORE_MAX)
		return 0;

	IF_ACTV_CORE(pi, pi->sh->phyrxchain, coreidx) {
		tot = 0;
		idx = pi->phy_noise_index;
		for (i = 0; i < PHY_NOISE_WINDOW_SZ; i++) {
			tot += pi->phy_noise_win[coreidx][idx];
			idx = MODINC_POW2(idx, PHY_NOISE_WINDOW_SZ);
		}

		result = (int8)(tot/PHY_NOISE_WINDOW_SZ);
	}

	return result;
}

int8
wlc_phy_noise_lte_avg(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t *)pih;
	int tot = 0;

	if (ISLCN40PHY(pi)) {
		phy_info_lcn40phy_t *pi_lcn40 = pi->u.pi_lcn40phy;
		int8 gain_adjust = 0;
		if (!pi->sh->clk)
			return 0;
		if (pi_lcn40->noise_iqest_en) {
			uint16 lcn40phyregs_shm_addr =
				2 * wlapi_bmac_read_shm(pi->sh->physhim, M_LCN40PHYREGS_PTR(pi));
			int16 noise_dBm_ucode, noise_dBm;
			wlc_lcn40phy_trigger_noise_iqest(pi);
			noise_dBm_ucode = (int16)wlapi_bmac_read_shm(pi->sh->physhim,
				lcn40phyregs_shm_addr + M_NOISE_LTE_IQPWR_DB_OFFSET(pi));
			if (noise_dBm_ucode)
				gain_adjust = wlc_lcn40phy_get_noise_iqest_gainadjust(pi);
			noise_dBm = noise_dBm_ucode + gain_adjust;
			tot = noise_dBm;
			PHY_INFORM(("noise_dBm=%d, noise_iq=%d, noise_dBm_ucode=%d\n",
				noise_dBm, (int16)wlapi_bmac_read_shm(pi->sh->physhim,
				lcn40phyregs_shm_addr + M_NOISE_IQPWR_OFFSET(pi)),
					noise_dBm_ucode));
		}
	}
	return (int8)tot;
}


#define LCN40_RXSTAT0_BRD_ATTN	12
#define LCN40_RXSTAT0_ACITBL_IDX_MSB	11
#define LCN40_BIT1_QDB_POS	10
#define LCN40_BIT0_QDB_POS	13

/* Increase the loop bandwidth of the PLL in the demodulator.
 * Note that although this allows the demodulator to track the
 * received carrier frequency over a wider bandwidth, it may
 * cause the Rx sensitivity to decrease
 */
void
wlc_phy_freqtrack_start(wlc_phy_t *pih)
{
}


/* Restore the loop bandwidth of the PLL in the demodulator to the original value */
void
wlc_phy_freqtrack_end(wlc_phy_t *pih)
{
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

	case PHY_TYPE_LCN40:
		cap |= (PHY_CAP_SGI | PHY_CAP_40MHZ | PHY_CAP_STBC);
		break;
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

/* %%%%%% IOCTL */
int
wlc_phy_ioctl_dispatch(phy_info_t *pi, int cmd, int len, void *arg, bool *ta_ok)
{
	wlc_phy_t *pih = (wlc_phy_t *)pi;
	int bcmerror = 0;
	int val, *pval;
	bool bool_val;
	uint8 max_aci_mode;
	bool suspend;

	UNUSED_PARAMETER(suspend);
	(void)pih;

	/* default argument is generic integer */
	pval = (int*)arg;

	/* This will prevent the misaligned access */
	if (pval && (uint32)len >= sizeof(val))
		bcopy(pval, &val, sizeof(val));
	else
		val = 0;

	/* bool conversion to avoid duplication below */
	bool_val = (val != 0);
	BCM_REFERENCE(bool_val);

	switch (cmd) {
	case WLC_RESTART:
		break;
	default:
		if ((arg == NULL) || (len <= 0)) {
			PHY_ERROR(("wl%d: %s: Command %d needs arguments\n",
			          pi->sh->unit, __FUNCTION__, cmd));
			return BCME_BADARG;
		}
		break;
	}

	switch (cmd) {

	case WLC_GET_PHY_NOISE:
		ASSERT(pval != NULL);
		*pval = wlc_phy_noise_avg(pih);
		break;

	case WLC_RESTART:
		/* Reset calibration results to uninitialized state in order to
		 * trigger recalibration next time wlc_init() is called.
		 */
		if (pi->sh->up) {
			bcmerror = BCME_NOTDOWN;
			break;
		}
		phy_type_reset_impl(pi->typei);
		break;

#if defined(BCMINTERNAL) || defined(BCMDBG)|| defined(WLTEST) || defined(DBG_PHY_IOV)
	case WLC_GET_RADIOREG:
		/* Suspend MAC if haven't done so */
#if (ACCONF != 0) || (ACCONF2 != 0)
		wlc_phy_conditional_suspend(pi, &suspend);
#endif
		*ta_ok = TRUE;

		if (!pi->sh->clk) {
			bcmerror = BCME_NOCLK;
			break;
		}
		ASSERT(pval != NULL);

		phy_utils_phyreg_enter(pi);
		phy_utils_radioreg_enter(pi);
		if (val == RADIO_IDCODE)
			*pval = phy_radio_query_idcode(pi->radioi);
		else
			*pval = phy_utils_read_radioreg(pi, (uint16)val);
		phy_utils_radioreg_exit(pi);
		phy_utils_phyreg_exit(pi);
#if (ACCONF != 0) || (ACCONF2 != 0)
		/* Resume MAC */
		wlc_phy_conditional_resume(pi, &suspend);
#endif
		break;

	case WLC_SET_RADIOREG:
#if (ACCONF != 0) || (ACCONF2 != 0)
		/* Suspend MAC if haven't done so */
		wlc_phy_conditional_suspend(pi, &suspend);
#endif
		*ta_ok = TRUE;

		if (!pi->sh->clk) {
			bcmerror = BCME_NOCLK;
			break;
		}

		phy_utils_phyreg_enter(pi);
		phy_utils_radioreg_enter(pi);
		phy_utils_write_radioreg(pi, (uint16)val, (uint16)(val >> NBITS(uint16)));
		phy_utils_radioreg_exit(pi);
		phy_utils_phyreg_exit(pi);
#if (ACCONF != 0) || (ACCONF2 != 0)
		/* Resume MAC */
		wlc_phy_conditional_resume(pi, &suspend);
#endif
		break;
#endif /* BCMINTERNAL || BCMDBG */

#if defined(BCMDBG)
	case WLC_GET_TX_PATH_PWR:

		*pval = (phy_utils_read_radioreg(pi, RADIO_2050_PU_OVR) & 0x84) ? 1 : 0;
		break;

	case WLC_SET_TX_PATH_PWR:

		if (!pi->sh->clk) {
			bcmerror = BCME_NOCLK;
			break;
		}

		phy_utils_phyreg_enter(pi);
		phy_utils_radioreg_enter(pi);
		if (bool_val) {
			/* Enable overrides */
			phy_utils_write_radioreg(pi, RADIO_2050_PU_OVR,
				0x84 | (phy_utils_read_radioreg(pi, RADIO_2050_PU_OVR) &
				0xf7));
		} else {
			/* Disable overrides */
			phy_utils_write_radioreg(pi, RADIO_2050_PU_OVR,
				phy_utils_read_radioreg(pi, RADIO_2050_PU_OVR) & ~0x84);
		}
		phy_utils_radioreg_exit(pi);
		phy_utils_phyreg_exit(pi);
		break;
#endif /* BCMDBG */

#if defined(BCMINTERNAL) || defined(BCMDBG) || defined(WLTEST) || defined(DBG_PHY_IOV) \
	|| defined(WL_EXPORT_GET_PHYREG)
	case WLC_GET_PHYREG:
#if (ACCONF != 0) || (ACCONF2 != 0)
		/* Suspend MAC if haven't done so */
		wlc_phy_conditional_suspend(pi, &suspend);
#endif
		*ta_ok = TRUE;

		if (!pi->sh->clk) {
			bcmerror = BCME_NOCLK;
			break;
		}

		phy_utils_phyreg_enter(pi);

		ASSERT(pval != NULL);
		*pval = phy_utils_read_phyreg(pi, (uint16)val);

		phy_utils_phyreg_exit(pi);
#if (ACCONF != 0) || (ACCONF2 != 0)
		/* Resume MAC */
		wlc_phy_conditional_resume(pi, &suspend);
#endif
		break;
#endif /* BCMINTERNAL || BCMDBG || WLTEST || DBG_PHY_IOV || WL_EXPORT_GET_PHYREG */

#if defined(BCMINTERNAL) || defined(BCMDBG) || defined(WLTEST) || defined(DBG_PHY_IOV)
	case WLC_SET_PHYREG:
#if (ACCONF != 0) || (ACCONF2 != 0)
		/* Suspend MAC if haven't done so */
		wlc_phy_conditional_suspend(pi, &suspend);
#endif
		*ta_ok = TRUE;

		if (!pi->sh->clk) {
			bcmerror = BCME_NOCLK;
			break;
		}

		phy_utils_phyreg_enter(pi);
		phy_utils_write_phyreg(pi, (uint16)val, (uint16)(val >> NBITS(uint16)));
		phy_utils_phyreg_exit(pi);
#if (ACCONF != 0) || (ACCONF2 != 0)
		/* Resume MAC */
		wlc_phy_conditional_resume(pi, &suspend);
#endif
		break;
#endif /* BCMINTERNAL || BCMDBG || WLTEST */

#if defined(BCMDBG) || defined(WLTEST)
	case WLC_GET_TSSI: {

		if (!pi->sh->clk) {
			bcmerror = BCME_NOCLK;
			break;
		}

		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		ASSERT(pval != NULL);
		*pval = 0;
		switch (pi->pubpi->phy_type) {
		case PHY_TYPE_LCN40:
			PHY_TRACE(("%s:***CHECK***\n", __FUNCTION__));
			CASECHECK(PHYTYPE, PHY_TYPE_LCN40);
			{
				int8 ofdm_pwr = 0, cck_pwr = 0;

				wlc_lcn40phy_get_tssi(pi, &ofdm_pwr, &cck_pwr);
				*pval =  ((uint16)ofdm_pwr << 8) | (uint16)cck_pwr;
				break;
			}
		case PHY_TYPE_LCN20:
			PHY_TRACE(("%s:***CHECK***\n", __FUNCTION__));
			CASECHECK(PHYTYPE, PHY_TYPE_LCN20);
			{
				int8 ofdm_pwr = 0, cck_pwr = 0;
#if (defined(LCN20CONF) && (LCN20CONF != 0))
				wlc_lcn20phy_get_tssi(pi, &ofdm_pwr, &cck_pwr);
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
				*pval =  ((uint16)ofdm_pwr << 8) | (uint16)cck_pwr;
				break;
			}
		case PHY_TYPE_N:
			CASECHECK(PHYTYPE, PHY_TYPE_N);
			{
			*pval = (phy_utils_read_phyreg(pi, NPHY_TSSIBiasVal1) & 0xff) << 8;
			*pval |= (phy_utils_read_phyreg(pi, NPHY_TSSIBiasVal2) & 0xff);
			break;
			}
		}

		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
		break;
	}

	case WLC_GET_ATTEN: {
		bcmerror = BCME_UNSUPPORTED;
		break;
	}

	case WLC_SET_ATTEN: {
		bcmerror = BCME_UNSUPPORTED;
		break;
	}

	case WLC_GET_PWRIDX:
		bcmerror = BCME_UNSUPPORTED;
		break;

	case WLC_SET_PWRIDX:	/* set A band radio/baseband power index */
		bcmerror = BCME_UNSUPPORTED;
		break;

	case WLC_LONGTRAIN:
		{
		longtrnfn_t long_train_fn = NULL;

		if (pi->sh->up) {
			bcmerror = BCME_NOTDOWN;
			break;
		}

		long_train_fn = pi->pi_fptr->longtrn;
		if (long_train_fn)
			bcmerror = (*long_train_fn)(pi, val);
		else
			PHY_ERROR(("WLC_LONGTRAIN: unsupported phy type\n"));

			break;
		}

	case WLC_EVM:
		ASSERT(arg != NULL);
		if (pi->sh->up) {
			bcmerror = BCME_NOTDOWN;
			break;
		}

		bcmerror = wlc_phy_test_evm(pi, val, *(((uint *)arg) + 1), *(((int *)arg) + 2));
		break;

	case WLC_FREQ_ACCURACY:
		/* SSLPNCONF transmits a few frames before running PAPD Calibration
		 * it does papd calibration each time it enters a new channel
		 * We cannot be down for this reason
		 */
		if (pi->sh->up) {
			bcmerror = BCME_NOTDOWN;
			break;
		}

		bcmerror = wlc_phy_test_freq_accuracy(pi, val);
		break;

	case WLC_CARRIER_SUPPRESS:
		if (pi->sh->up) {
			bcmerror = BCME_NOTDOWN;
			break;
		}
		bcmerror = wlc_phy_test_carrier_suppress(pi, val);
		break;
#endif /* BCMDBG || WLTEST  */

#ifndef WLC_DISABLE_ACI
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(WL_PHYACIARGS)
	case WLC_GET_ACI_ARGS:
		ASSERT(arg != NULL);
		bcmerror = wlc_phy_aci_args(pi, arg, TRUE, len);
		break;

	case WLC_SET_ACI_ARGS:
		ASSERT(arg != NULL);
		bcmerror = wlc_phy_aci_args(pi, arg, FALSE, len);
		break;

#endif /* defined (BCMINTERNAL) || defined (WLTEST) || defined (WL_PHYACIARGS) */
#endif /* Compiling out ACI code for 4324 */

	case WLC_GET_INTERFERENCE_MODE:
		ASSERT(pval != NULL);
		*pval = pi->sh->interference_mode;
		if (pi->aci_state & ACI_ACTIVE) {
			*pval |= AUTO_ACTIVE;
			*pval |= (pi->aci_active_pwr_level << 4);
		}
		break;

	case WLC_SET_INTERFERENCE_MODE:
		max_aci_mode = ISACPHY(pi) ? ACPHY_ACI_MAX_MODE : WLAN_AUTO_W_NOISE;
		if (val < INTERFERE_NONE || val > max_aci_mode) {
			bcmerror = BCME_RANGE;
			break;
		}

		if (pi->sh->interference_mode == val)
			break;

		/* push to sw state */
		pi->sh->interference_mode = val;

		if (!pi->sh->up) {
			bcmerror = BCME_NOTUP;
			break;
		}

		wlapi_suspend_mac_and_wait(pi->sh->physhim);

#ifndef WLC_DISABLE_ACI
		/* turn interference mode to off before entering another mode */
		if (val != INTERFERE_NONE)
			phy_noise_set_mode(pi->noisei, INTERFERE_NONE, TRUE);

#if defined(RXDESENS_EN)
		if (ISNPHY(pi))
			wlc_nphy_set_rxdesens((wlc_phy_t *)pi, 0);
#endif
		if (phy_noise_set_mode(pi->noisei, pi->sh->interference_mode, TRUE) != BCME_OK)
			bcmerror = BCME_BADOPTION;
#endif /* !defined(WLC_DISABLE_ACI) */

		wlapi_enable_mac(pi->sh->physhim);
		break;

	case WLC_GET_INTERFERENCE_OVERRIDE_MODE:
		if (!(ISLCN20PHY(pi) || ISACPHY(pi) || ISNPHY(pi) || ISHTPHY(pi))) {
			break;
		}

		ASSERT(pval != NULL);
		if (pi->sh->interference_mode_override == FALSE) {
			*pval = INTERFERE_OVRRIDE_OFF;
		} else {
			*pval = pi->sh->interference_mode;
		}
		break;

	case WLC_SET_INTERFERENCE_OVERRIDE_MODE:
		max_aci_mode = ISACPHY(pi) ? ACPHY_ACI_MAX_MODE : WLAN_AUTO_W_NOISE;
		if (!(ISLCN20PHY(pi) || ISACPHY(pi) || ISNPHY(pi) || ISHTPHY(pi))) {
			break;
		}

		if (val < INTERFERE_OVRRIDE_OFF || val > max_aci_mode) {
			bcmerror = BCME_RANGE;
			break;
		}

		bcmerror = wlc_phy_set_interference_override_mode(pi, val);

		break;

	default:
		bcmerror = BCME_UNSUPPORTED;
	}

	return bcmerror;
}

/* WARNING: check ISSIM_ENAB() before doing any radio related calibrations */
int32
wlc_phy_watchdog(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t*)pih;

	/* abort if call should be blocked (e.g. not in home channel) */
	if ((SCAN_RM_IN_PROGRESS(pi) || PLT_INPROG_PHY(pi) || ASSOC_INPROG_PHY(pi)))
		return BCME_OK;

	if (ISACPHY(pi))
		wlc_phy_adjust_preempt_on_bt_activity_acphy(pih);

	/* PHY specific watchdog callbacks */
	if (pi->pi_fptr->phywatchdog)
		(*pi->pi_fptr->phywatchdog)(pi);

	/* PHY calibration is suppressed until this counter becomes 0 */
	if (pi->cal_info->cal_suppress_count > 0)
		pi->cal_info->cal_suppress_count--;

	return BCME_OK;
}

void
wlc_phy_BSSinit(wlc_phy_t *pih, bool bonlyap, int noise)
{
	phy_info_t *pi = (phy_info_t*)pih;
	uint i;
	uint core;

	if (bonlyap) {
	}

	/* watchdog idle phy noise */
	for (i = 0; i < MA_WINDOW_SZ; i++) {
		pi->sh->phy_noise_window[i] = (int8)(noise & 0xff);
	}
	pi->sh->phy_noise_index = 0;

	if ((pi->sh->interference_mode == WLAN_AUTO) &&
	     (pi->aci_state & ACI_ACTIVE)) {
		/* Reset the clock to check again after the moving average buffer has filled
		 */
		pi->aci_start_time = pi->sh->now + MA_WINDOW_SZ;
	}

	for (i = 0; i < PHY_NOISE_WINDOW_SZ; i++) {
		FOREACH_CORE(pi, core)
			pi->phy_noise_win[core][i] = PHY_NOISE_FIXED_VAL_NPHY;
	}
	pi->phy_noise_index = 0;
}


/* Convert epsilon table value to complex number */
void
wlc_phy_papd_decode_epsilon(uint32 epsilon, int32 *eps_real, int32 *eps_imag)
{
	if ((*eps_imag = (epsilon>>13)) > 0xfff)
		*eps_imag -= 0x2000; /* Sign extend */
	if ((*eps_real = (epsilon & 0x1fff)) > 0xfff)
		*eps_real -= 0x2000; /* Sign extend */
}

#if defined(PHYCAL_CACHING)
int
wlc_phy_cal_cache_init(wlc_phy_t *ppi)
{
	return 0;
}

void
wlc_phy_cal_cache_deinit(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	ch_calcache_t *ctx = pi->phy_calcache;

	while (ctx) {
		pi->phy_calcache = ctx->next;
		if (ctx->u_ram_cache.acphy_ram_calcache) {
			phy_mfree(pi, ctx->u_ram_cache.acphy_ram_calcache,
				sizeof(*ctx->u_ram_cache.acphy_ram_calcache));
			ctx->u_ram_cache.acphy_ram_calcache = NULL;
		}
		phy_mfree(pi, ctx,
		      sizeof(ch_calcache_t));
		ctx = pi->phy_calcache;
	}

	pi->phy_calcache = NULL;

	/* No more per-channel contexts, switch in the default one */
	pi->cal_info = pi->def_cal_info;
	ASSERT(pi->cal_info != NULL);
	if (pi->cal_info != NULL) {
		/* Reset the parameters */
		pi->cal_info->last_cal_temp = -50;
		pi->cal_info->last_cal_time = 0;
		pi->cal_info->last_temp_cal_time = 0;
	}
}
#endif /* defined(PHYCAL_CACHING) */

#if defined(PHYCAL_CACHING) || defined(PHYCAL_CACHE_SMALL)
void
wlc_phy_cal_cache_set(wlc_phy_t *ppi, bool state)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	pi->phy_calcache_on = state;
	if (!state)
		wlc_phy_cal_cache_deinit(ppi);
}

bool
wlc_phy_cal_cache_get(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	return pi->phy_calcache_on;
}
#endif /* defined(PHYCAL_CACHING) || defined(PHYCALCACHE_SMALL) */

void
wlc_phy_cal_mode(wlc_phy_t *ppi, uint mode)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	if (pi->pi_fptr->calibmodes)
		pi->pi_fptr->calibmodes(pi, mode);
}

void
wlc_phy_set_glacial_timer(wlc_phy_t *ppi, uint period)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	pi->sh->glacial_timer = period;
}

void
wlc_phy_cal_perical_mphase_reset(phy_info_t *pi)
{
	phy_cal_info_t *cal_info = pi->cal_info;

#if defined(PHYCAL_CACHING)
	PHY_CAL(("wlc_phy_cal_perical_mphase_reset chanspec 0x%x ctx: %p\n",
		pi->radio_chanspec, wlc_phy_get_chanctx(pi, pi->radio_chanspec)));
#else
	PHY_CAL(("wlc_phy_cal_perical_mphase_reset\n"));
#endif

	if (pi->phycal_timer)
		wlapi_del_timer(pi->sh->physhim, pi->phycal_timer);

	if (ISNPHY(pi))
		pi->u.pi_nphy->cal_type_override = PHY_PERICAL_AUTO;

	/* resets cal engine */
	cal_info->cal_phase_id = MPHASE_CAL_STATE_IDLE;

	cal_info->txcal_cmdidx = 0; /* needed in nphy only */
}

void
wlc_phy_cal_perical_mphase_restart(phy_info_t *pi)
{
	PHY_CAL(("wlc_phy_cal_perical_mphase_restart\n"));
	pi->cal_info->cal_phase_id = MPHASE_CAL_STATE_INIT;
	pi->cal_info->txcal_cmdidx = 0;
}

void
wlc_phy_stf_chain_init(wlc_phy_t *pih, uint8 txchain, uint8 rxchain)
{
	phy_info_t *pi = (phy_info_t*)pih;

	pi->sh->hw_phytxchain = txchain;
	pi->sh->hw_phyrxchain = rxchain;
	pi->sh->phytxchain = txchain;
	pi->sh->phyrxchain = rxchain;
}

void
wlc_phy_stf_chain_set(wlc_phy_t *pih, uint8 txchain, uint8 rxchain)
{
	phy_info_t *pi = (phy_info_t*)pih;

	PHY_TRACE(("wlc_phy_stf_chain_set, new phy chain tx %d, rx %d", txchain, rxchain));

	pi->sh->phytxchain = txchain;
	if (ISACPHY(pi)) {
		pi->u.pi_acphy->both_txchain_rxchain_eq_1 =
			((rxchain == 1) && (txchain == 1)) ? TRUE : FALSE;
	}
	if (ISNPHY(pi)) {
		if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV))
			wlc_phy_rxcore_setstate_nphy(pih, rxchain, 1);
		else
			wlc_phy_rxcore_setstate_nphy(pih, rxchain, 0);

	} else if (ISHTPHY(pi)) {
		wlc_phy_rxcore_setstate_htphy(pih, rxchain);
	} else if (ISACPHY(pi)) {
		wlc_phy_rxcore_setstate_acphy(pih, rxchain, pi->sh->phytxchain);
	}
}

void
wlc_phy_stf_chain_get(wlc_phy_t *pih, uint8 *txchain, uint8 *rxchain)
{
	phy_info_t *pi = (phy_info_t*)pih;

	*txchain = pi->sh->phytxchain;
	*rxchain = pi->sh->phyrxchain;
}

uint16
wlc_phy_stf_duty_cycle_chain_active_get(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t*)pih;

	ASSERT(pi->tempi != NULL);
	return phy_temp_throttle(pi->tempi);
}

int8
wlc_phy_stf_ssmode_get(wlc_phy_t *pih, chanspec_t chanspec)
{
	phy_info_t *pi = (phy_info_t*)pih;
	int8* mcs_limits_1;
	int8* mcs_limits_2;

	PHY_TRACE(("wl%d: %s: chanspec %x\n", pi->sh->unit, __FUNCTION__, chanspec));

	/* criteria to choose stf mode */

	/* the "+3dbm (12 0.25db units)" is to account for the fact that with CDD, tx occurs
	 * on both chains
	 */
	if (CHSPEC_IS40(chanspec)) {
		mcs_limits_1 = &pi->b40_1x1mcs0;
		mcs_limits_2 = &pi->b40_1x2cdd_mcs0;
	} else {
		mcs_limits_1 = &pi->b20_1x1mcs0;
		mcs_limits_2 = &pi->b20_1x2cdd_mcs0;
	}
	if (*mcs_limits_1 > *mcs_limits_2 + 12)
		return PHY_TXC1_MODE_SISO;
	else
		return PHY_TXC1_MODE_CDD;
}

#ifdef WLTEST
void
wlc_phy_boardflag_upd(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t *)pih;

	if (ISNPHY(pi)) {
		/* Check if A-band spur WAR should be enabled for this board */
		if (BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2) & BFL2_SPUR_WAR) {
			PHY_ERROR(("%s: aband_spurwar on\n", __FUNCTION__));
			pi->u.pi_nphy->nphy_aband_spurwar_en = TRUE;
		} else {
			PHY_ERROR(("%s: aband_spurwar off\n", __FUNCTION__));
			pi->u.pi_nphy->nphy_aband_spurwar_en = FALSE;
		}

		/* Check if extra G-band spur WAR for 40 MHz channels 3 through 10
		 * should be enabled for this board
		 */
		if (BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2) & BFL2_2G_SPUR_WAR) {
			PHY_ERROR(("%s: gband_spurwar2 on\n", __FUNCTION__));
			pi->u.pi_nphy->nphy_gband_spurwar2_en = TRUE;
		} else {
			PHY_ERROR(("%s: gband_spurwar2 off\n", __FUNCTION__));
			pi->u.pi_nphy->nphy_gband_spurwar2_en = FALSE;
		}
	}

	pi->nphy_txpwrctrl = PHY_TPC_HW_OFF;
	pi->txpwrctrl = PHY_TPC_HW_OFF;
	pi->phy_5g_pwrgain = FALSE;

	if ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2) & BFL2_TXPWRCTRL_EN) &&
		(pi->sh->sromrev >= 4)) {

		pi->nphy_txpwrctrl = PHY_TPC_HW_ON;

	} else if ((pi->sh->sromrev >= 4) &&
		(BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2) & BFL2_5G_PWRGAIN)) {
		pi->phy_5g_pwrgain = TRUE;
	}
}
#endif /* WLTEST */

#if (defined(WLTEST) || defined(BCMINTERNAL))
int
wlc_phy_set_po_htphy(phy_info_t *pi, wl_po_t *inpo)
{
	int err = BCME_OK;

	if ((inpo->band > WL_CHAN_FREQ_RANGE_5G_BAND3))
		err  = BCME_BADARG;
	else
	{
		if (inpo->band == WL_CHAN_FREQ_RANGE_2G)
		{
			pi->ppr->u.sr9.cckbw202gpo = inpo->cckpo;
			pi->ppr->u.sr9.cckbw20ul2gpo = inpo->cckpo;
		}
		pi->ppr->u.sr9.ofdm[inpo->band].bw20 = inpo->ofdmpo;
		pi->ppr->u.sr9.ofdm[inpo->band].bw20ul = inpo->ofdmpo;
		pi->ppr->u.sr9.ofdm[inpo->band].bw40 = 0;
		pi->ppr->u.sr9.mcs[inpo->band].bw20 =
			(inpo->mcspo[1] << 16) | inpo->mcspo[0];
		pi->ppr->u.sr9.mcs[inpo->band].bw20ul =
			(inpo->mcspo[3] << 16) | inpo->mcspo[2];
		pi->ppr->u.sr9.mcs[inpo->band].bw40 =
			(inpo->mcspo[5] << 16) | inpo->mcspo[4];
	}
	return err;
}

int
wlc_phy_get_po_htphy(phy_info_t *pi, wl_po_t *outpo)
{
	int err = BCME_OK;

	if ((outpo->band > WL_CHAN_FREQ_RANGE_5G_BAND3))
		err  = BCME_BADARG;
	else
	{
		if (outpo->band == WL_CHAN_FREQ_RANGE_2G)
			outpo->cckpo = pi->ppr->u.sr9.cckbw202gpo;

		outpo->ofdmpo = pi->ppr->u.sr9.ofdm[outpo->band].bw20;
		outpo->mcspo[0] = (uint16)pi->ppr->u.sr9.mcs[outpo->band].bw20;
		outpo->mcspo[1] = (uint16)(pi->ppr->u.sr9.mcs[outpo->band].bw20 >>16);
		outpo->mcspo[2] = (uint16)pi->ppr->u.sr9.mcs[outpo->band].bw20ul;
		outpo->mcspo[3] = (uint16)(pi->ppr->u.sr9.mcs[outpo->band].bw20ul >>16);
		outpo->mcspo[4] = (uint16)pi->ppr->u.sr9.mcs[outpo->band].bw40;
		outpo->mcspo[5] = (uint16)(pi->ppr->u.sr9.mcs[outpo->band].bw40 >>16);
	}

	return err;
}

#endif /* WLTEST || BCMINTERNAL */
/* %%%%%% dump */

#if defined(BCMDBG) || defined(BCMDBG_DUMP)

int
wlc_phydump_papd(void *ctx, struct bcmstrbuf *b)
{
	phy_info_t *pi = ctx;
	uint32 val, i, j;
	int32 eps_real, eps_imag;

	eps_real = eps_imag = 0;

	if (!pi->sh->up)
		return BCME_NOTUP;

	if (!(ISNPHY(pi) || ISLCNCOMMONPHY(pi)))
		return BCME_UNSUPPORTED;



	if (ISNPHY(pi))
	{
		bcm_bprintf(b, "papd eps table:\n [core 0]\t\t[core 1] \n");
		for (j = 0; j < 64; j++) {
			for (i = 0; i < 2; i++) {
				wlc_phy_table_read_nphy(pi, ((i == 0) ? NPHY_TBL_ID_EPSILONTBL0 :
					NPHY_TBL_ID_EPSILONTBL1), 1, j, 32, &val);
				wlc_phy_papd_decode_epsilon(val, &eps_real, &eps_imag);
				bcm_bprintf(b, "{%d\t%d}\t\t", eps_real, eps_imag);
			}
		bcm_bprintf(b, "\n");
		}
		bcm_bprintf(b, "\n\n");
	}
	else if (ISLCN40PHY(pi))
		wlc_lcn40phy_read_papdepstbl(pi, b);

	return BCME_OK;
}

int
wlc_phydump_state(void *ctx, struct bcmstrbuf *b)
{
#ifndef PPR_API
	phy_info_t *pi = ctx;
	char fraction[4][4] = {"  ", ".25", ".5", ".75"};
	int i;
	bool isphyhtcap = ISPHY_HT_CAP(pi);
	uint offset;

#define QDB_FRAC(x)	(x) / WLC_TXPWR_DB_FACTOR, fraction[(x) % WLC_TXPWR_DB_FACTOR]

	bcm_bprintf(b, "phy_type %d phy_rev %d ana_rev %d radioid 0x%x radiorev 0x%x\n",
	               pi->pubpi->phy_type, pi->pubpi->phy_rev, pi->pubpi->ana_rev,
	               pi->pubpi->radioid, pi->pubpi->radiorev);

	bcm_bprintf(b, "hw_power_control %d\n",
	               pi->hwpwrctrl);

	bcm_bprintf(b, "Power targets: ");
	/* CCK Power/Rate */
	bcm_bprintf(b, "\n\tCCK: ");
	for (i = 0; i < WL_NUM_RATES_CCK; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_CCK]));
	/* OFDM Power/Rate */
	bcm_bprintf(b, "\n\tOFDM 20MHz SISO: ");
	for (i = 0; i < WL_NUM_RATES_OFDM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_OFDM]));
	bcm_bprintf(b, "\n\tOFDM 20MHz CDD: ");
	for (i = 0; i < WL_NUM_RATES_OFDM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_OFDM_20_CDD]));
	/* 20MHz MCS Power/Rate */
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS0-7 20MHz 1 Tx: " : "\n\tMCS0-7 20MHz SISO: ");
	offset = isphyhtcap ? TXP_FIRST_MCS_20_S1x1 : TXP_FIRST_MCS_20_SISO;
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+offset]));
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS0-7 20MHz 2 Tx: " : "\n\tMCS0-7 20MHz CDD: ");
	offset = isphyhtcap ? TXP_FIRST_MCS_20_S1x2 : TXP_FIRST_MCS_20_CDD;
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+offset]));
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS0-7 20MHz 3 Tx: " : "\n\tMCS0-7 20MHz STBC: ");
	offset = isphyhtcap ? TXP_FIRST_MCS_20_S1x3 : TXP_FIRST_MCS_20_STBC;
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+offset]));
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS8-15 20MHz 2 Tx: " : "\n\tMCS0-7 20MHz SDM: ");
	offset = isphyhtcap ? TXP_FIRST_MCS_20_S2x2 : TXP_FIRST_MCS_20_SDM;
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i]));
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS8-15 20MHz 3 Tx: " : "");
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM && isphyhtcap; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_MCS_20_S2x3]));
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS16-23 20MHz 3 Tx: " : "");
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM && isphyhtcap; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_MCS_20_S3x3]));

	/* 40MHz OFDM Power/Rate */
	bcm_bprintf(b, "\n\tOFDM 40MHz SISO: ");
	for (i = 0; i < WL_NUM_RATES_OFDM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_OFDM_40_SISO]));
	bcm_bprintf(b, "\n\tOFDM 40MHz CDD: ");
	for (i = 0; i < WL_NUM_RATES_OFDM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_OFDM_40_CDD]));
	/* 40MHz MCS Power/Rate */
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS0-7 40MHz 1 Tx: " : "\n\tMCS0-7 40MHz SISO: ");
	offset = isphyhtcap ? TXP_FIRST_MCS_40_S1x1 : TXP_FIRST_MCS_40_SISO;
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+offset]));
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS0-7 40MHz 2 Tx: " : "\n\tMCS0-7 40MHz CDD: ");
	offset = isphyhtcap ? TXP_FIRST_MCS_40_S1x2 : TXP_FIRST_MCS_40_CDD;
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+offset]));
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS0-7 40MHz 3 Tx: " : "\n\tMCS0-7 40MHz STBC: ");
	offset = isphyhtcap ? TXP_FIRST_MCS_40_S1x3 : TXP_FIRST_MCS_40_STBC;
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+offset]));
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS8-15 40MHz 2 Tx: " : "\n\tMCS0-7 40MHz SDM: ");
	offset = isphyhtcap ? TXP_FIRST_MCS_40_S2x2 : TXP_FIRST_MCS_40_SDM;
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i]));
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS8-15 40MHz 3 Tx: " : "");
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM && isphyhtcap; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_MCS_40_S2x3]));
	bcm_bprintf(b, isphyhtcap ? "\n\tMCS16-23 40MHz 3 Tx: " : "");
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM && isphyhtcap; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_MCS_40_S3x3]));

	/* MCS 32 Power */
	bcm_bprintf(b, "\n\tMCS32: %2d%s\n\n", QDB_FRAC(pi->tx_power_target[i]));

	if (isphyhtcap)
		goto next;

	/* CCK Power/Rate */
#if HTCONF
	bcm_bprintf(b, "\n\tCCK 20UL: ");
	for (i = 0; i < WL_NUM_RATES_CCK; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_20UL_CCK]));

	/* 20 in 40MHz OFDM Power/Rate */
	bcm_bprintf(b, "\n\tOFDM 20UL SISO: ");
	for (i = 0; i < WL_NUM_RATES_OFDM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_20UL_OFDM]));
	bcm_bprintf(b, "\n\tOFDM 40MHz CDD: ");
	for (i = 0; i < WL_NUM_RATES_OFDM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_20UL_OFDM_CDD]));

	/* 20 in 40MHz MCS Power/Rate */
	bcm_bprintf(b, "\n\tMCS0-7 20UL 1 Tx: ");
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_20UL_S1x1]));
	bcm_bprintf(b, "\n\tMCS0-7 20UL 2 Tx: ");
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_20UL_S1x2]));
	bcm_bprintf(b, "\n\tMCS0-7 20UL 3 Tx: ");
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_FIRST_20UL_S1x3]));
	bcm_bprintf(b, "\n\tMCS8-15 20UL 2 Tx: ");
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_LAST_20UL_S2x2]));
	bcm_bprintf(b, "\n\tMCS8-15 20UL 3 Tx: ");
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM && isphyhtcap; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_LAST_20UL_S2x3]));
	bcm_bprintf(b, "\n\tMCS16-23 20UL 3 Tx: ");
	for (i = 0; i < WL_NUM_RATES_MCS_1STREAM && isphyhtcap; i++)
		bcm_bprintf(b, "%2d%s ", QDB_FRAC(pi->tx_power_target[i+TXP_LAST_20UL_S3x3]));
#endif /* HTCONF */

next:
	if (ISNPHY(pi)) {
		bcm_bprintf(b, "antsel_type %d\n", pi->antsel_type);
		bcm_bprintf(b, "ipa2g %d ipa5g %d\n", pi->ipa2g_on, pi->ipa5g_on);

	} else if (ISLCN40PHY(pi) || ISHTPHY(pi)) {
		return;
	}

	bcm_bprintf(b, "\ninterference_mode %d intf_crs %d\n",
		pi->sh->interference_mode, pi->interference_mode_crs);
#endif /* !PPR_API */

	return BCME_OK;
}

int
wlc_phydump_lnagain(void *ctx, struct bcmstrbuf *b)
{
	phy_info_t *pi = ctx;
	int core;
	uint16 lnagains[2][4];
	uint16 mingain[2];
	phy_info_nphy_t *pi_nphy = NULL;

	if (pi->u.pi_nphy)
		pi_nphy = pi->u.pi_nphy;

	if (!ISNPHY(pi))
		return BCME_UNSUPPORTED;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	if ((pi_nphy) && (pi_nphy->phyhang_avoid))
		wlc_phy_stay_in_carriersearch_nphy(pi, TRUE);

	/* Now, read the gain table */
	for (core = 0; core < 2; core++) {
		wlc_phy_table_read_nphy(pi, core, 4, 8, 16, &lnagains[core][0]);
	}

	mingain[0] =
		(phy_utils_read_phyreg(pi, NPHY_Core1MinMaxGain) &
		NPHY_CoreMinMaxGain_minGainValue_MASK) >>
		NPHY_CoreMinMaxGain_minGainValue_SHIFT;
	mingain[1] =
		(phy_utils_read_phyreg(pi, NPHY_Core2MinMaxGain) &
		NPHY_CoreMinMaxGain_minGainValue_MASK) >>
		NPHY_CoreMinMaxGain_minGainValue_SHIFT;

	if ((pi_nphy) && (pi_nphy->phyhang_avoid))
		wlc_phy_stay_in_carriersearch_nphy(pi, FALSE);

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	bcm_bprintf(b, "Core 0: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
		lnagains[0][0], lnagains[0][1], lnagains[0][2], lnagains[0][3]);
	bcm_bprintf(b, "Core 1: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n\n",
		lnagains[1][0], lnagains[1][1], lnagains[1][2], lnagains[1][3]);
	bcm_bprintf(b, "Min Gain: Core 0=0x%02x,   Core 1=0x%02x\n\n",
		mingain[0], mingain[1]);

	return BCME_OK;
}

int
wlc_phydump_initgain(void *ctx, struct bcmstrbuf *b)
{
	phy_info_t *pi = ctx;
	uint8 ctr;
	uint16 regval[2], tblregval[4];
	uint16 lna_gain[2], hpvga1_gain[2], hpvga2_gain[2];
	uint16 tbl_lna_gain[4], tbl_hpvga1_gain[4], tbl_hpvga2_gain[4];
	phy_info_nphy_t *pi_nphy = NULL;

	if (pi->u.pi_nphy)
		pi_nphy = pi->u.pi_nphy;

	if (!ISNPHY(pi))
		return BCME_UNSUPPORTED;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	if ((pi_nphy) && (pi_nphy->phyhang_avoid))
		wlc_phy_stay_in_carriersearch_nphy(pi, TRUE);

	regval[0] = phy_utils_read_phyreg(pi, NPHY_Core1InitGainCode);
	regval[1] = phy_utils_read_phyreg(pi, NPHY_Core2InitGainCode);

	wlc_phy_table_read_nphy(pi, 7, PHYCORENUM(pi->pubpi->phy_corenum), 0x106, 16, tblregval);

	if ((pi_nphy) && (pi_nphy->phyhang_avoid))
		wlc_phy_stay_in_carriersearch_nphy(pi, FALSE);

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	lna_gain[0] = (regval[0] & NPHY_CoreInitGainCode_initLnaIndex_MASK) >>
		NPHY_CoreInitGainCode_initLnaIndex_SHIFT;
	hpvga1_gain[0] = (regval[0] & NPHY_CoreInitGainCode_initHpvga1Index_MASK) >>
		NPHY_CoreInitGainCode_initHpvga1Index_SHIFT;
	hpvga2_gain[0] = (regval[0] & NPHY_CoreInitGainCode_initHpvga2Index_MASK) >>
		NPHY_CoreInitGainCode_initHpvga2Index_SHIFT;


	lna_gain[1] = (regval[1] & NPHY_CoreInitGainCode_initLnaIndex_MASK) >>
		NPHY_CoreInitGainCode_initLnaIndex_SHIFT;
	hpvga1_gain[1] = (regval[1] & NPHY_CoreInitGainCode_initHpvga1Index_MASK) >>
		NPHY_CoreInitGainCode_initHpvga1Index_SHIFT;
	hpvga2_gain[1] = (regval[1] & NPHY_CoreInitGainCode_initHpvga2Index_MASK) >>
		NPHY_CoreInitGainCode_initHpvga2Index_SHIFT;

	for (ctr = 0; ctr < 4; ctr++) {
		tbl_lna_gain[ctr] = (tblregval[ctr] >> 2) & 0x3;
	}

	for (ctr = 0; ctr < 4; ctr++) {
		tbl_hpvga1_gain[ctr] = (tblregval[ctr] >> 4) & 0xf;
	}

	for (ctr = 0; ctr < 4; ctr++) {
		tbl_hpvga2_gain[ctr] = (tblregval[ctr] >> 8) & 0x1f;
	}

	bcm_bprintf(b, "Core 0 INIT gain: HPVGA2=%d, HPVGA1=%d, LNA=%d\n",
		hpvga2_gain[0], hpvga1_gain[0], lna_gain[0]);
	bcm_bprintf(b, "Core 1 INIT gain: HPVGA2=%d, HPVGA1=%d, LNA=%d\n",
		hpvga2_gain[1], hpvga1_gain[1], lna_gain[1]);
	bcm_bprintf(b, "\n");
	bcm_bprintf(b, "INIT gain table:\n");
	bcm_bprintf(b, "----------------\n");
	for (ctr = 0; ctr < 4; ctr++) {
		bcm_bprintf(b, "Core %d: HPVGA2=%d, HPVGA1=%d, LNA=%d\n",
			ctr, tbl_hpvga2_gain[ctr], tbl_hpvga1_gain[ctr], tbl_lna_gain[ctr]);
	}

	return BCME_OK;
}

int
wlc_phydump_hpf1tbl(void *ctx, struct bcmstrbuf *b)
{
	phy_info_t *pi = ctx;
	uint8 ctr, core;
	uint16 gain[2][NPHY_MAX_HPVGA1_INDEX+1];
	uint16 gainbits[2][NPHY_MAX_HPVGA1_INDEX+1];
	phy_info_nphy_t *pi_nphy = NULL;

	if (pi->u.pi_nphy)
		pi_nphy = pi->u.pi_nphy;

	if (!ISNPHY(pi))
		return BCME_UNSUPPORTED;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	if ((pi_nphy) && (pi_nphy->phyhang_avoid))
		wlc_phy_stay_in_carriersearch_nphy(pi, TRUE);

	/* Read from the HPVGA1 gaintable */
	wlc_phy_table_read_nphy(pi, 0, NPHY_MAX_HPVGA1_INDEX, 16, 16, &gain[0][0]);
	wlc_phy_table_read_nphy(pi, 1, NPHY_MAX_HPVGA1_INDEX, 16, 16, &gain[1][0]);
	wlc_phy_table_read_nphy(pi, 2, NPHY_MAX_HPVGA1_INDEX, 16, 16, &gainbits[0][0]);
	wlc_phy_table_read_nphy(pi, 3, NPHY_MAX_HPVGA1_INDEX, 16, 16, &gainbits[1][0]);

	if ((pi_nphy) && (pi_nphy->phyhang_avoid))
		wlc_phy_stay_in_carriersearch_nphy(pi, FALSE);

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	for (core = 0; core < 2; core++) {
		bcm_bprintf(b, "Core %d gain: ", core);
		for (ctr = 0; ctr <= NPHY_MAX_HPVGA1_INDEX; ctr++)  {
			bcm_bprintf(b, "%2d ", gain[core][ctr]);
		}
		bcm_bprintf(b, "\n");
	}

	bcm_bprintf(b, "\n");
	for (core = 0; core < 2; core++) {
		bcm_bprintf(b, "Core %d gainbits: ", core);
		for (ctr = 0; ctr <= NPHY_MAX_HPVGA1_INDEX; ctr++)  {
			bcm_bprintf(b, "%2d ", gainbits[core][ctr]);
		}
		bcm_bprintf(b, "\n");
	}

	return BCME_OK;
}

int
wlc_phydump_chanest(void *ctx, struct bcmstrbuf *b)
{
	phy_info_t *pi = ctx;
	uint16 num_rx, num_sts, num_tones, start_tone;
	uint16 k, r, t, fftk;
	uint32 ch;
	uint16 ch_re_ma, ch_im_ma;
	uint8  ch_re_si, ch_im_si;
	int16  ch_re, ch_im;
	int8   ch_exp;
	uint8  dump_tones;

	if (!(ISHTPHY(pi) || ISACPHY(pi) || ISNPHY(pi)))
		return BCME_UNSUPPORTED;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	/* Go deaf to prevent PHY channel writes while doing reads */
	if (ISNPHY(pi))
		wlc_phy_stay_in_carriersearch_nphy(pi, TRUE);
	else if (ISHTPHY(pi))
		wlc_phy_stay_in_carriersearch_htphy(pi, TRUE);
	else if (ISACPHY(pi))
		wlc_phy_stay_in_carriersearch_acphy(pi, TRUE);

	num_rx = (uint8)PHYCORENUM(pi->pubpi->phy_corenum);
	if (ACMAJORREV_2(pi->pubpi->phy_rev) ||
		ACMAJORREV_4(pi->pubpi->phy_rev)) {
		/* number of spatial streams supported */
		num_sts = 2;
	} else {
		num_sts = 4;
	}
	if (CHSPEC_IS40(pi->radio_chanspec)) {
		num_tones = 128;
#ifdef CHSPEC_IS80
	} else if (CHSPEC_IS80(pi->radio_chanspec)) {
		num_tones = 256;
#endif /* CHSPEC_IS80 */
	} else {
		num_tones = 64;
	}

	bcm_bprintf(b, "num_tones=%d\n", num_tones);

	/* Dump only 16 sub-carriers at a time */
	dump_tones = 16;
	/* Reset the dump counter */
	if (pi->phy_chanest_dump_ctr > (num_tones/dump_tones - 1))
		pi->phy_chanest_dump_ctr = 0;

	start_tone = pi->phy_chanest_dump_ctr * dump_tones;
	pi->phy_chanest_dump_ctr++;

	for (r = 0; r < num_rx; r++) {
		bcm_bprintf(b, "rx=%d\n", r);
		for (t = 0; t < num_sts; t++) {
			bcm_bprintf(b, "sts=%d\n", t);
			for (k = start_tone; k < (start_tone + dump_tones); k++) {
				if (ISNPHY(pi)) {
					wlc_phy_table_read_nphy(pi, NPHY_TBL_ID_CHANEST, 1,
					                         t*128 + k, 32, &ch);

					/* Q11 FLP (12,12,6) */
					ch_re_ma  = ((ch >> 18) & 0x7ff);
					ch_re_si  = ((ch >> 29) & 0x001);
					ch_im_ma  = ((ch >>  6) & 0x7ff);
					ch_im_si  = ((ch >> 17) & 0x001);
					ch_exp    = ((int8)((ch << 2) & 0xfc)) >> 2;
					ch_re = (ch_re_si > 0) ? -ch_re_ma : ch_re_ma;
					ch_im = (ch_im_si > 0) ? -ch_im_ma : ch_im_ma;

				} else if (ISHTPHY(pi)) {
					wlc_phy_table_read_htphy(pi, HTPHY_TBL_ID_CHANEST(r), 1,
					                         t*128 + k, 32, &ch);

					/* Q11 FLP (12,12,6) */
					ch_re_ma  = ((ch >> 18) & 0x7ff);
					ch_re_si  = ((ch >> 29) & 0x001);
					ch_im_ma  = ((ch >>  6) & 0x7ff);
					ch_im_si  = ((ch >> 17) & 0x001);
					ch_exp    = ((int8)((ch << 2) & 0xfc)) >> 2;
					ch_re = (ch_re_si > 0) ? -ch_re_ma : ch_re_ma;
					ch_im = (ch_im_si > 0) ? -ch_im_ma : ch_im_ma;

				} else {
					ASSERT(ISACPHY(pi));
					wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_CHANEST(r), 1,
					                         t*256 + k, 32, &ch);

					if (ACMAJORREV_0(pi->pubpi->phy_rev) ||
						ACMAJORREV_1(pi->pubpi->phy_rev) ||
						(ACMAJORREV_2(pi->pubpi->phy_rev) &&
						SW_ACMINORREV_2(pi->pubpi->phy_rev)) ||
						ACMAJORREV_5(pi->pubpi->phy_rev) ||
						ACMAJORREV_32(pi->pubpi->phy_rev) ||
						ACMAJORREV_33(pi->pubpi->phy_rev) ||
						ACMAJORREV_37(pi->pubpi->phy_rev)) {
						/* Q11 FLP (12,12,6) */
						ch_re_ma  = ((ch >> 18) & 0x7ff);
						ch_re_si  = ((ch >> 29) & 0x001);
						ch_im_ma  = ((ch >>  6) & 0x7ff);
						ch_im_si  = ((ch >> 17) & 0x001);
						ch_exp    = ((int8)((ch << 2) & 0xfc)) >> 2;
						ch_re = (ch_re_si > 0) ? -ch_re_ma : ch_re_ma;
						ch_im = (ch_im_si > 0) ? -ch_im_ma : ch_im_ma;
					} else if (ACMAJORREV_2(pi->pubpi->phy_rev) ||
							ACMAJORREV_4(pi->pubpi->phy_rev) ||
							ACMAJORREV_40(pi->pubpi->phy_rev)) {
						/* Q8 FLP (9,9,5) */
						ch_re_ma  = ((ch >> 14) & 0xff);
						ch_re_si  = ((ch >> 22) & 0x01);
						ch_im_ma  = ((ch >>  5) & 0xff);
						ch_im_si  = ((ch >> 13) & 0x01);
						ch_exp    = ((int8)((ch << 3) & 0xf8)) >> 3;
						ch_re = (ch_re_si > 0) ? -ch_re_ma : ch_re_ma;
						ch_im = (ch_im_si > 0) ? -ch_im_ma : ch_im_ma;
					} else {
						/* Q13 FXP (14,14) */
						ch_re_ma  = ((ch >> 14) & 0x3fff);
						ch_im_ma  = ((ch >>  0) & 0x3fff);
						ch_exp = 0;
						ch_re = ((int16)((ch_re_ma << 2) & 0xfffc)) >> 2;
						ch_im = ((int16)((ch_im_ma << 2) & 0xfffc)) >> 2;
					}
				}
				fftk = ((k < num_tones/2) ? (k + num_tones/2) : (k - num_tones/2));

				bcm_bprintf(b, "chan(%d,%d,%d)=(%d+i*%d)*2^%d;\n",
				            r+1, t+1, fftk+1, ch_re, ch_im, ch_exp);
			}
		}
	}

	/* Return from deaf */
	if (ISNPHY(pi))
		wlc_phy_stay_in_carriersearch_nphy(pi, FALSE);
	else if (ISHTPHY(pi))
		wlc_phy_stay_in_carriersearch_htphy(pi, FALSE);
	else if (ISACPHY(pi))
		wlc_phy_stay_in_carriersearch_acphy(pi, FALSE);

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	return BCME_OK;
}

int
wlc_phydump_txv0(void *ctx, struct bcmstrbuf *b)
{
	phy_info_t *pi = ctx;
	uint16 k;
	uint32 tbl_val;
	uint8 stall_val;

	if (!ISACPHY(pi))
		return BCME_UNSUPPORTED;

	phy_utils_phyreg_enter(pi);

	/* disable stall */
	stall_val = (phy_utils_read_phyreg(pi, ACPHY_RxFeCtrl1(pi->pubpi->phy_rev)) &
	             ACPHY_RxFeCtrl1_disable_stalls_MASK(pi->pubpi->phy_rev))
	        >> ACPHY_RxFeCtrl1_disable_stalls_SHIFT(pi->pubpi->phy_rev);
	phy_utils_mod_phyreg(pi, ACPHY_RxFeCtrl1(pi->pubpi->phy_rev),
	            ACPHY_RxFeCtrl1_disable_stalls_MASK(pi->pubpi->phy_rev),
	            1 << ACPHY_RxFeCtrl1_disable_stalls_SHIFT(pi->pubpi->phy_rev));

	/* id=16, depth=1952 words for usr 0, width=32 */
	for (k = 0; k < 1952; k++) {
		wlc_phy_table_read_acphy(pi, 16, 1, k, 32, &tbl_val);
		bcm_bprintf(b, "0x%08x\n", tbl_val);
	}

	/* restore stall value */
	phy_utils_mod_phyreg(pi, ACPHY_RxFeCtrl1(pi->pubpi->phy_rev),
	            ACPHY_RxFeCtrl1_disable_stalls_MASK(pi->pubpi->phy_rev),
	            stall_val << ACPHY_RxFeCtrl1_disable_stalls_SHIFT(pi->pubpi->phy_rev));

	phy_utils_phyreg_exit(pi);

	return BCME_OK;
}
#endif /* BCMDBG || BCMDBG_DUMP */

#ifdef WLTEST
int
wlc_phydump_ch4rpcal(void *ctx, struct bcmstrbuf *b)
{
	phy_info_t *pi = ctx;
	uint16 num_tones;
	uint16 k, r, fftk, fft_size;
	uint32 ch, tbl_id;
	uint16 ch_re_ma, ch_im_ma;
	uint8  ch_re_si, ch_im_si;
	int16  ch_re, ch_im;
	uint16 *tone_idx_tbl;
	uint16 tone_idx_20[52]  = {
		4,  5,  6,  7,  8,  9, 10, 12, 13, 14, 15, 16, 17,
		18, 19, 20, 21, 22, 23, 24, 26, 27, 28, 29, 30, 31,
		33, 34, 35, 36, 37, 38, 40, 41, 42, 43, 44, 45, 46,
		47, 48, 49, 50, 51, 52, 54, 55, 56, 57, 58, 59, 60};

	uint16 tone_idx_40[108] = {
		6,  7,  8,  9, 10, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
		25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 40, 41, 42, 43,
		44, 45, 46, 47, 48, 49, 50, 51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62,
		66, 67, 68, 69, 70, 71, 72, 73, 74, 76, 77, 78, 79, 80, 81, 82, 83, 84,
		85, 86, 87, 88, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102,
		103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
		118, 119, 120, 121, 122};

#ifdef CHSPEC_IS80
	uint16 tone_idx_80[234] = {
		6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
		24, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
		43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 54, 55, 56, 57, 58, 59, 60, 61,
		62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87, 88, 90, 91, 92, 93, 94, 95, 96, 97, 98,
		99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
		114, 115, 116, 118, 119, 120, 121, 122, 123, 124, 125, 126, 130, 131, 132,
		133, 134, 135, 136, 137, 138, 140, 141, 142, 143, 144, 145, 146, 147, 148,
		149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163,
		164, 165, 166, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
		180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
		195, 196, 197, 198, 199, 200, 201, 202, 204, 205, 206, 207, 208, 209, 210,
		211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225,
		226, 227, 228, 229, 230, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241,
		242, 243, 244, 245, 246, 247, 248, 249, 250};
#endif /* CHSPEC_IS80 */

	if (!(ISHTPHY(pi) || ISACPHY(pi) || ISNPHY(pi)))
		return BCME_UNSUPPORTED;

	if (!pi->sh->up)
		return BCME_NOTUP;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	/* Go deaf to prevent PHY channel writes while doing reads */
	if (ISNPHY(pi))
		wlc_phy_stay_in_carriersearch_nphy(pi, TRUE);
	else if (ISHTPHY(pi))
		wlc_phy_stay_in_carriersearch_htphy(pi, TRUE);
	else if (ISACPHY(pi))
		wlc_phy_stay_in_carriersearch_acphy(pi, TRUE);

	if (CHSPEC_IS40(pi->radio_chanspec)) {
		num_tones = 108;
		fft_size = 128;
		tone_idx_tbl = tone_idx_40;
#ifdef CHSPEC_IS80
	} else if (CHSPEC_IS80(pi->radio_chanspec)) {
		num_tones = 234;
		fft_size = 256;
		tone_idx_tbl = tone_idx_80;
#endif /* CHSPEC_IS80 */
	} else {
		num_tones = 52;
		fft_size = 64;
		tone_idx_tbl = tone_idx_20;
	}

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, r) {
		for (k = 0; k < num_tones; k++) {
			fftk = tone_idx_tbl[k];
			fftk = (fftk < fft_size/2) ? (fftk + fft_size/2) : (fftk - fft_size/2);

			if (ISNPHY(pi)) {
				tbl_id = NPHY_TBL_ID_CHANEST;
				wlc_phy_table_read_nphy(pi, tbl_id, 1, fftk, 32, &ch);
			} else if (ISHTPHY(pi)) {
				tbl_id = HTPHY_TBL_ID_CHANEST(r);
				wlc_phy_table_read_htphy(pi, tbl_id, 1, fftk, 32, &ch);
			} else if (ISACPHY(pi)) {
				tbl_id = ACPHY_TBL_ID_CHANEST(r);
				wlc_phy_table_read_acphy(pi, tbl_id, 1, fftk, 32, &ch);
			}
			if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev) ||
			    ACMAJORREV_32(pi->pubpi->phy_rev) ||
			    ACMAJORREV_33(pi->pubpi->phy_rev) ||
			    ACMAJORREV_37(pi->pubpi->phy_rev)) {
				ch_re_ma  = ((ch >> 18) & 0x7ff);
				ch_re_si  = ((ch >> 29) & 0x001);
				ch_im_ma  = ((ch >>  6) & 0x7ff);
				ch_im_si  = ((ch >> 17) & 0x001);
			} else {
				ch_re_ma  = ((ch >> 14) & 0xff);
				ch_re_si  = ((ch >> 22) & 0x01);
				ch_im_ma  = ((ch >>  5) & 0xff);
				ch_im_si  = ((ch >> 13) & 0x01);
			}
			ch_re = (ch_re_si > 0) ? -ch_re_ma : ch_re_ma;
			ch_im = (ch_im_si > 0) ? -ch_im_ma : ch_im_ma;

			bcm_bprintf(b, "%d\n%d\n", ch_re, ch_im);
		}
	}

	/* Return from deaf */
	if (ISNPHY(pi))
		wlc_phy_stay_in_carriersearch_nphy(pi, FALSE);
	else if (ISHTPHY(pi))
		wlc_phy_stay_in_carriersearch_htphy(pi, FALSE);
	else if (ISACPHY(pi))
		wlc_phy_stay_in_carriersearch_acphy(pi, FALSE);

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	return BCME_OK;
}
#endif /* WLTEST */

#if defined(DBG_BCN_LOSS)
int
wlc_phydump_phycal_rx_min(void *ctx, struct bcmstrbuf *b)
{
	phy_info_t *pi = ctx;
	nphy_iq_comp_t rxcal_coeffs;
	int time_elapsed;
	phy_info_nphy_t *pi_nphy = NULL;

	if (!pi->sh->up) {
		PHY_ERROR(("wl%d: %s: Not up, cannot dump \n", pi->sh->unit, __FUNCTION__));
		return BCME_NOTUP;
	}

	if (ISACPHY(pi)) {
		PHY_ERROR(("wl%d: %s: AC Phy not yet supported\n", pi->sh->unit, __FUNCTION__));
		return BCME_UNSUPPORTED;
	}
	else if (ISHTPHY(pi)) {
		wlc_phy_cal_dump_htphy_rx_min(pi, b);
	}
	else if (ISNPHY(pi)) {
		pi_nphy = pi->u.pi_nphy;
		if (!pi_nphy) {
			PHY_ERROR(("wl%d: %s: NPhy null, cannot dump \n",
				pi->sh->unit, __FUNCTION__));
			return BCME_UNSUPPORTED;
		}

		time_elapsed = pi->sh->now - pi->cal_info->last_cal_time;
		if (time_elapsed < 0)
			time_elapsed = 0;

		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);

		if (pi_nphy->phyhang_avoid)
			wlc_phy_stay_in_carriersearch_nphy(pi, TRUE);

		/* Read Rx calibration co-efficients */
		wlc_phy_rx_iq_coeffs_nphy(pi, 0, &rxcal_coeffs);

		if (pi_nphy->phyhang_avoid)
			wlc_phy_stay_in_carriersearch_nphy(pi, FALSE);

		/* reg access is done, enable the mac */
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);

		bcm_bprintf(b, "time since last cal: %d (sec), mphase_cal_id: %d\n\n",
			time_elapsed, pi->cal_info->cal_phase_id);

		bcm_bprintf(b, "rx cal  a0=%d, b0=%d, a1=%d, b1=%d\n\n",
			rxcal_coeffs.a0, rxcal_coeffs.b0, rxcal_coeffs.a1, rxcal_coeffs.b1);
	}

	return BCME_OK;
}
#endif /* DBG_BCN_LOSS */

void
wlc_beacon_loss_war_lcnxn(wlc_phy_t *ppi, uint8 enable)
{
	 phy_info_t *pi = (phy_info_t*)ppi;
		phy_utils_mod_phyreg(pi, NPHY_reset_cca_frame_cond_ctrl_1,
		NPHY_reset_cca_frame_cond_ctrl_1_resetCCA_frame_cond_en_MASK,
		enable << NPHY_reset_cca_frame_cond_ctrl_1_resetCCA_frame_cond_en_SHIFT);
		phy_utils_mod_phyreg(pi, NPHY_EngCtrl1,
		NPHY_EngCtrl1_resetBphyEn_MASK,
		enable << NPHY_EngCtrl1_resetBphyEn_SHIFT);
}

const uint8 *
BCMRAMFN(wlc_phy_get_ofdm_rate_lookup)(void)
{
	return ofdm_rate_lookup;
}

void
wlc_phy_ldpc_override_set(wlc_phy_t *ppi, bool ldpc)
{
	phy_info_t *pi = (phy_info_t *)ppi;
#if (defined(LCN20CONF) && (LCN20CONF != 0))
	if (ISLCN20PHY(pi))
		wlc_phy_update_rxldpc_lcn20phy(pi, ldpc);
#endif /* #if (defined(LCN20CONF) && (LCN20CONF != 0)) */
	if (ISHTPHY(pi))
		wlc_phy_update_rxldpc_htphy(pi, ldpc);
	if (ISACPHY(pi))
		wlc_phy_update_rxldpc_acphy(pi, ldpc);
	return;
}

void
wlc_phy_tkip_rifs_war(wlc_phy_t *ppi, uint8 rifs)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	if (ISNPHY(pi))
		wlc_phy_nphy_tkip_rifs_war(pi, rifs);
}

void
wlc_phy_get_pwrdet_offsets(phy_info_t *pi, int8 *cckoffset, int8 *ofdmoffset)
{
	*cckoffset = 0;
	*ofdmoffset = 0;
}

/* update the cck power detector offset */
int8
wlc_phy_upd_rssi_offset(phy_info_t *pi, int8 rssi, chanspec_t chanspec)
{
	return rssi;
}

bool
wlc_phy_txpower_ipa_ison(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	if (ISNPHY(pi))
		return (wlc_phy_n_txpower_ipa_ison(pi));
	else
		return 0;
}

int
BCMRAMFN(wlc_phy_create_chanctx)(wlc_phy_t *ppi, chanspec_t chanspec)
{
#ifdef PHYCAL_CACHING
	ch_calcache_t *ctx;
	phy_info_t *pi = (phy_info_t *)ppi;

	/* Check for existing */
	if (wlc_phy_reuse_chanctx(ppi, chanspec) != 0)
		return BCME_OK;

	ctx = phy_malloc_fatal(pi, sizeof(*ctx));

	ctx->u_ram_cache.acphy_ram_calcache = phy_malloc_fatal(pi,
		sizeof(*ctx->u_ram_cache.acphy_ram_calcache));

	ctx->chanspec = chanspec;
	ctx->creation_time = pi->sh->now;
	ctx->cal_info.last_cal_temp = -50;
	ctx->cal_info.txcal_numcmds = pi->def_cal_info->txcal_numcmds;
	ctx->in_use = TRUE;

	/* Add it to the list */
	ctx->next = pi->phy_calcache;

	/* For the first context, switch out the default context */
	if (pi->phy_calcache == NULL &&
	    (pi->radio_chanspec == chanspec))
		pi->cal_info = &ctx->cal_info;
	pi->phy_calcache = ctx;
	pi->phy_calcache_num++;

	PHY_INFORM(("wl%d: %s ctx %d created for Ch %d\n",
		PI_INSTANCE(pi), __FUNCTION__,
		pi->phy_calcache_num,
		CHSPEC_CHANNEL(chanspec)));
#endif /* PHYCAL_CACHING */
	return BCME_OK;
}

void
BCMRAMFN(wlc_phy_destroy_chanctx)(wlc_phy_t *ppi, chanspec_t chanspec)
{
#ifdef PHYCAL_CACHING
	phy_info_t *pi = (phy_info_t *)ppi;
	ch_calcache_t *ctx;

	ctx = wlc_phy_get_chanctx(pi, chanspec);
	if (ctx) {
		ctx->valid = FALSE;
		ctx->in_use = FALSE;
	}

	PHY_INFORM(("wl%d: %s for Ch %d\n",
		PI_INSTANCE(pi), __FUNCTION__,
		CHSPEC_CHANNEL(chanspec)));
#endif /* PHYCAL_CACHING */
}

#ifndef PPR_API
static void
BCMNMIATTACHFN(wlc_phy_txpwr_srom9_convert)(phy_info_t *pi, uint8 *srom_max, uint32 pwr_offset,
	uint8 tmp_max_pwr, uint8 rate_start, uint8 rate_end, bool shift)
{
	uint8 rate;
	uint8 nibble;

	if (pi->sh->sromrev < 9) {
		ASSERT(0 && "SROMREV < 9");
		return;
	}

	for (rate = rate_start; rate <= rate_end; rate++) {
		nibble = (uint8)(pwr_offset & 0xf);
		if (shift)
			pwr_offset >>= 4;
		/* nibble info indicates offset in 0.5dB units convert to 0.25dB */
		srom_max[rate] = tmp_max_pwr - (nibble << 1);
	}
}
void
BCMNMIATTACHFN(wlc_phy_txpwr_apply_srom9)(phy_info_t *pi)
{

	srom_pwrdet_t	*pwrdet  = pi->pwrdet;
	uint8 tmp_max_pwr = 0;
	uint8 *tx_srom_max_rate = NULL;
	uint32 ppr_offsets[PWR_OFFSET_SIZE];
	uint32 pwr_offsets;
	uint rate_cnt, rate;
	int band_num;
	bool shift;

	for (band_num = 0; band_num < NUMSUBBANDS(pi); band_num++) {
		bzero((uint8 *)ppr_offsets, PWR_OFFSET_SIZE * sizeof(uint32));
		if (ISNPHY(pi)) {
			/* find MIN of 2  cores, board limits */
			tmp_max_pwr = MIN(pwrdet->max_pwr[0][band_num],
				pwrdet->max_pwr[1][band_num]);
		}
		if (ISPHY_HT_CAP(pi)) {
			tmp_max_pwr =
				MIN(pwrdet->max_pwr[0][band_num], pwrdet->max_pwr[1][band_num]);
			tmp_max_pwr =
				MIN(tmp_max_pwr, pwrdet->max_pwr[2][band_num]);
		}

		ppr_offsets[OFDM_20_PO] = pi->ppr->u.sr9.ofdm[band_num].bw20;
		ppr_offsets[OFDM_20UL_PO] = pi->ppr->u.sr9.ofdm[band_num].bw20ul;
		ppr_offsets[OFDM_40DUP_PO] = pi->ppr->u.sr9.ofdm[band_num].bw40;
		ppr_offsets[MCS_20_PO] = pi->ppr->u.sr9.mcs[band_num].bw20;
		ppr_offsets[MCS_20UL_PO] = pi->ppr->u.sr9.mcs[band_num].bw20ul;
		ppr_offsets[MCS_40_PO] = pi->ppr->u.sr9.mcs[band_num].bw40;
		tx_srom_max_rate = (uint8*)(&(pi->tx_srom_max_rate[band_num][0]));

		switch (band_num) {
			case WL_CHAN_FREQ_RANGE_2G:
				ppr_offsets[MCS32_PO] = (uint32)(pi->ppr->u.sr9.mcs32po & 0xf);
				wlc_phy_txpwr_srom9_convert(pi, tx_srom_max_rate,
					pi->ppr->u.sr9.cckbw202gpo, tmp_max_pwr,
					TXP_FIRST_CCK, TXP_LAST_CCK, TRUE);
				wlc_phy_txpwr_srom9_convert(pi, tx_srom_max_rate,
					pi->ppr->u.sr9.cckbw202gpo, tmp_max_pwr,
					TXP_FIRST_CCK_CDD_S1x2, TXP_LAST_CCK_CDD_S1x2, TRUE);
				wlc_phy_txpwr_srom9_convert(pi, tx_srom_max_rate,
					pi->ppr->u.sr9.cckbw202gpo, tmp_max_pwr,
					TXP_FIRST_CCK_CDD_S1x3, TXP_LAST_CCK_CDD_S1x3, TRUE);
				wlc_phy_txpwr_srom9_convert(pi, tx_srom_max_rate,
					pi->ppr->u.sr9.cckbw20ul2gpo, tmp_max_pwr,
					TXP_FIRST_20UL_CCK, TXP_LAST_20UL_CCK, TRUE);
				wlc_phy_txpwr_srom9_convert(pi, tx_srom_max_rate,
					pi->ppr->u.sr9.cckbw20ul2gpo, tmp_max_pwr,
					TXP_FIRST_CCK_20U_CDD_S1x2,
					TXP_LAST_CCK_20U_CDD_S1x2, TRUE);
				wlc_phy_txpwr_srom9_convert(pi, tx_srom_max_rate,
					pi->ppr->u.sr9.cckbw20ul2gpo, tmp_max_pwr,
					TXP_FIRST_CCK_20U_CDD_S1x3,
					TXP_LAST_CCK_20U_CDD_S1x3, TRUE);
				break;
#ifdef BAND5G
			case WL_CHAN_FREQ_RANGE_5G_BAND0:
				ppr_offsets[MCS32_PO] = (uint32)(pi->ppr->u.sr9.mcs32po >> 4) & 0xf;
				break;
			case WL_CHAN_FREQ_RANGE_5G_BAND1:
				ppr_offsets[MCS32_PO] = (uint32)(pi->ppr->u.sr9.mcs32po >> 8) & 0xf;
				break;
			case WL_CHAN_FREQ_RANGE_5G_BAND2:
				ppr_offsets[MCS32_PO] =
					(uint32)(pi->ppr->u.sr9.mcs32po >> 12) & 0xf;
				break;
			case WL_CHAN_FREQ_RANGE_5G_BAND3:
				ppr_offsets[MCS32_PO] =
					(uint32)(pi->ppr->u.sr9.mcs32po >> 12) & 0xf;
				break;
#endif /* BAND5G */
			default:
				break;
		}


		for (rate = TXP_FIRST_CCK; rate < TXP_NUM_RATES; rate += rate_cnt) {
			shift = TRUE;
			pwr_offsets = 0;
			switch (rate) {
				case TXP_FIRST_CCK:
				case TXP_FIRST_20UL_CCK:
				case TXP_FIRST_CCK_CDD_S1x2:
				case TXP_FIRST_CCK_CDD_S1x3:
				case TXP_FIRST_CCK_20U_CDD_S1x2:
				case TXP_FIRST_CCK_20U_CDD_S1x3:
					rate_cnt = WL_NUM_RATES_CCK;
					continue;
				case TXP_FIRST_OFDM:
				case TXP_FIRST_OFDM_20_CDD:
					pwr_offsets = ppr_offsets[OFDM_20_PO];
					rate_cnt = WL_NUM_RATES_OFDM;
					break;
				case TXP_FIRST_MCS_20_S1x1:
				case TXP_FIRST_MCS_20_S1x2:
				case TXP_FIRST_MCS_20_S1x3:
				case TXP_FIRST_MCS_20_S2x2:
				case TXP_FIRST_MCS_20_S2x3:
				case TXP_FIRST_MCS_20_S3x3:
					pwr_offsets = ppr_offsets[MCS_20_PO];
					rate_cnt = WL_NUM_RATES_MCS_1STREAM;
					break;
				case TXP_FIRST_OFDM_40_SISO:
				case TXP_FIRST_OFDM_40_CDD:
					pwr_offsets = ppr_offsets[OFDM_40DUP_PO];
					rate_cnt = WL_NUM_RATES_OFDM;
					break;
				case TXP_FIRST_MCS_40_S1x1:
				case TXP_FIRST_MCS_40_S1x2:
				case TXP_FIRST_MCS_40_S1x3:
				case TXP_FIRST_MCS_40_S2x2:
				case TXP_FIRST_MCS_40_S2x3:
				case TXP_FIRST_MCS_40_S3x3:
					pwr_offsets = ppr_offsets[MCS_40_PO];
					rate_cnt = WL_NUM_RATES_MCS_1STREAM;
					break;
				case TXP_MCS_32:
					pwr_offsets = ppr_offsets[MCS32_PO];
					rate_cnt = WL_NUM_RATES_MCS32;
					break;
				case TXP_FIRST_20UL_OFDM:
				case TXP_FIRST_20UL_OFDM_CDD:
					pwr_offsets = ppr_offsets[OFDM_20UL_PO];
					rate_cnt = WL_NUM_RATES_OFDM;
					break;
				case TXP_FIRST_20UL_S1x1:
				case TXP_FIRST_20UL_S1x2:
				case TXP_FIRST_20UL_S1x3:
				case TXP_FIRST_20UL_S2x2:
				case TXP_FIRST_20UL_S2x3:
				case TXP_FIRST_20UL_S3x3:
					pwr_offsets = ppr_offsets[MCS_20UL_PO];
					rate_cnt = WL_NUM_RATES_MCS_1STREAM;
					break;
				default:
					PHY_ERROR(("Invalid rate %d\n", rate));
					rate_cnt = 1;
					ASSERT(0);
					break;
			}
			wlc_phy_txpwr_srom9_convert(pi, tx_srom_max_rate, pwr_offsets,
				tmp_max_pwr, (uint8)rate, (uint8)rate+rate_cnt-1, shift);
		}
	}
}

#endif /* !PPR_API */

#ifdef PPR_API
/* add 1MSB to represent 5bit-width ppr value, for mcs8 and mcs9 only */
void
wlc_phy_txpwr_ppr_bit_ext_mcs8and9(ppr_vht_mcs_rateset_t* vht, uint8 msb)
{
	/* this added 1MSB is the 4th bit, so left shift 4 bits
	 * then left shift 1 more bit since boardlimit is 0.5dB format
	 */
	vht->pwr[8] -= ((msb & 0x1) << 4) << 1;
	vht->pwr[9] -= ((msb & 0x2) << 3) << 1;
}

void
ppr_dsss_printf(ppr_t *p)
{
	int chain, bitN;
	ppr_dsss_rateset_t dsss_boardlimits;

	for (chain = WL_TX_CHAINS_1; chain <= WL_TX_CHAINS_3; chain++) {
		ppr_get_dsss(p, WL_TX_BW_20, chain, &dsss_boardlimits);
		PHY_ERROR(("--------DSSS-BW_20-S1x%d-----\n", chain));
		for (bitN = 0; bitN < WL_RATESET_SZ_DSSS; bitN++)
			PHY_ERROR(("max-pwr = %d\n", dsss_boardlimits.pwr[bitN]));

		ppr_get_dsss(p, WL_TX_BW_20IN40, chain, &dsss_boardlimits);
		PHY_ERROR(("--------DSSS-BW_20IN40-S1x%d-----\n", chain));
		for (bitN = 0; bitN < WL_RATESET_SZ_DSSS; bitN++)
			PHY_ERROR(("max-pwr = %d\n", dsss_boardlimits.pwr[bitN]));

	}
}

void
ppr_ofdm_printf(ppr_t *p)
{

	int chain, bitN;
	ppr_ofdm_rateset_t ofdm_boardlimits;
	wl_tx_mode_t mode = WL_TX_MODE_NONE;

	for (chain = WL_TX_CHAINS_1; chain <= WL_TX_CHAINS_3; chain++) {
		ppr_get_ofdm(p, WL_TX_BW_20, mode, chain, &ofdm_boardlimits);
		PHY_ERROR(("--------OFDM-BW_20-S1x%d-----\n", chain));
		for (bitN = 0; bitN < WL_RATESET_SZ_OFDM; bitN++)
			PHY_ERROR(("max-pwr = %d\n", ofdm_boardlimits.pwr[bitN]));

		ppr_get_ofdm(p, WL_TX_BW_20IN40, mode, chain, &ofdm_boardlimits);
		PHY_ERROR(("--------OFDM-BW_20IN40-S1x%d-----\n", chain));
		for (bitN = 0; bitN < WL_RATESET_SZ_OFDM; bitN++)
			PHY_ERROR(("max-pwr = %d\n", ofdm_boardlimits.pwr[bitN]));

		ppr_get_ofdm(p, WL_TX_BW_20IN80, mode, chain, &ofdm_boardlimits);
		PHY_ERROR(("--------OFDM-BW_20IN80-S1x%d-----\n", chain));
		for (bitN = 0; bitN < WL_RATESET_SZ_OFDM; bitN++)
			PHY_ERROR(("max-pwr = %d\n", ofdm_boardlimits.pwr[bitN]));

		ppr_get_ofdm(p, WL_TX_BW_40, mode, chain, &ofdm_boardlimits);
		PHY_ERROR(("--------OFDM-BW_DUP40-S1x%d-----\n", chain));
		for (bitN = 0; bitN < WL_RATESET_SZ_OFDM; bitN++)
			PHY_ERROR(("max-pwr = %d\n", ofdm_boardlimits.pwr[bitN]));
		mode = WL_TX_MODE_CDD;
	}
}

void
ppr_mcs_printf(ppr_t* tx_srom_max_pwr)
{

	int bitN, bwtype;
	ppr_vht_mcs_rateset_t mcs_boardlimits;
#if defined(BCMDBG) || defined(PHYDBG)
	char* bw[6] = { "20IN20", "40IN40", "80IN80", "20IN40", "20IN80", "40IN80" };
#endif /* BCMDBG || PHYDBG */
	for (bwtype = 0; bwtype < 6; bwtype++) {
		ppr_get_vht_mcs(tx_srom_max_pwr, bwtype, 1, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
			&mcs_boardlimits);
		PHY_INFORM(("--------MCS-%s-S1x1-----\n", bw[bwtype]));
		for (bitN = 0; bitN < WL_RATESET_SZ_VHT_MCS; bitN++)
			PHY_INFORM(("max-pwr = %d\n", mcs_boardlimits.pwr[bitN]));
		/* for ht_20IN20_S1x2_CDD */
		ppr_get_vht_mcs(tx_srom_max_pwr, bwtype, 1, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
			&mcs_boardlimits);
		PHY_INFORM(("--------MCS-%s-S1x2-CDD-----\n", bw[bwtype]));
		for (bitN = 0; bitN < WL_RATESET_SZ_VHT_MCS; bitN++)
			PHY_INFORM(("max-pwr = %d\n", mcs_boardlimits.pwr[bitN]));
		/* for ht_20IN20_S1x3_CDD */
		ppr_get_vht_mcs(tx_srom_max_pwr, bwtype, 1, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
			&mcs_boardlimits);
		PHY_INFORM(("--------MCS-%s-S1x3-CDD-----\n", bw[bwtype]));
		for (bitN = 0; bitN < WL_RATESET_SZ_VHT_MCS; bitN++)
			PHY_INFORM(("max-pwr = %d\n", mcs_boardlimits.pwr[bitN]));
		/* for ht_20IN20_S2x2_STBC */
		ppr_get_vht_mcs(tx_srom_max_pwr, bwtype, 2, WL_TX_MODE_STBC, WL_TX_CHAINS_2,
			&mcs_boardlimits);
		PHY_INFORM(("--------MCS-%s-S2x2-STBC------\n", bw[bwtype]));
		for (bitN = 0; bitN < WL_RATESET_SZ_VHT_MCS; bitN++)
			PHY_INFORM(("max-pwr = %d\n", mcs_boardlimits.pwr[bitN]));
		/* for ht_20IN20_S2x3_STBC */
		ppr_get_vht_mcs(tx_srom_max_pwr, bwtype, 2, WL_TX_MODE_STBC, WL_TX_CHAINS_3,
			&mcs_boardlimits);
		PHY_INFORM(("--------MCS-%s-S2x3-STBC------\n", bw[bwtype]));
		for (bitN = 0; bitN < WL_RATESET_SZ_VHT_MCS; bitN++)
			PHY_INFORM(("max-pwr = %d\n", mcs_boardlimits.pwr[bitN]));
		/* for ht_20IN20_S2x2_SDM */
		ppr_get_vht_mcs(tx_srom_max_pwr, bwtype, 2, WL_TX_MODE_NONE, WL_TX_CHAINS_2,
			&mcs_boardlimits);
		PHY_INFORM(("--------MCS-%s-S2x2-SDM------\n", bw[bwtype]));
		for (bitN = 0; bitN < WL_RATESET_SZ_VHT_MCS; bitN++)
			PHY_INFORM(("max-pwr = %d\n", mcs_boardlimits.pwr[bitN]));
		/* for ht_20IN20_S2x3_SDM */
		ppr_get_vht_mcs(tx_srom_max_pwr, bwtype, 2, WL_TX_MODE_NONE, WL_TX_CHAINS_3,
			&mcs_boardlimits);
		PHY_INFORM(("--------MCS-%s-S2x3-SDM------\n", bw[bwtype]));
		for (bitN = 0; bitN < WL_RATESET_SZ_VHT_MCS; bitN++)
			PHY_INFORM(("max-pwr = %d\n", mcs_boardlimits.pwr[bitN]));
		/* for ht_20IN20_S3x3_SDM */
		ppr_get_vht_mcs(tx_srom_max_pwr, bwtype, 3, WL_TX_MODE_NONE, WL_TX_CHAINS_3,
			&mcs_boardlimits);
		PHY_INFORM(("--------MCS-%s-S3x3-SDM------\n", bw[bwtype]));
		for (bitN = 0; bitN < WL_RATESET_SZ_VHT_MCS; bitN++)
			PHY_INFORM(("max-pwr = %d\n", mcs_boardlimits.pwr[bitN]));
	}
}

uint8
wlc_phy_get_band_from_channel(phy_info_t *pi, uint channel)
{
	/* NOTE: At present this function has only been validated for
	 * LCN and LCN40 phys. It may apply to others, but
	 * that is left as an exercise to the reader.
	 */
	uint8 band = 0;

#ifdef BAND5G
	if ((channel >= FIRST_LOW_5G_CHAN_SSLPNPHY) &&
		(channel <= LAST_LOW_5G_CHAN_SSLPNPHY)) {
		band = WL_CHAN_FREQ_RANGE_5GL;
	} else if ((channel >= FIRST_MID_5G_CHAN_SSLPNPHY) &&
		(channel <= LAST_MID_5G_CHAN_SSLPNPHY)) {
		band = WL_CHAN_FREQ_RANGE_5GM;
	} else if ((channel >= FIRST_HIGH_5G_CHAN_SSLPNPHY) &&
		(channel <= LAST_HIGH_5G_CHAN_SSLPNPHY)) {
		band = WL_CHAN_FREQ_RANGE_5GH;
	} else
#endif /* BAND5G */
	if (channel <= CH_MAX_2G_CHANNEL) {
		band = WL_CHAN_FREQ_RANGE_2G;
	} else {
		PHY_ERROR(("%s: invalid channel %d\n", __FUNCTION__, channel));
		ASSERT(0);
	}

	return band;
}
#endif /* PPR_API */

static const char BCMATTACHDATA(rstr_legofdmbw202gpo)[] = "legofdmbw202gpo";
static const char BCMATTACHDATA(rstr_legofdmbw20ul2gpo)[] = "legofdmbw20ul2gpo";
static const char BCMATTACHDATA(rstr_legofdmbw205glpo)[] = "legofdmbw205glpo";
static const char BCMATTACHDATA(rstr_legofdmbw20ul5glpo)[] = "legofdmbw20ul5glpo";
static const char BCMATTACHDATA(rstr_legofdmbw205gmpo)[] = "legofdmbw205gmpo";
static const char BCMATTACHDATA(rstr_legofdmbw20ul5gmpo)[] = "legofdmbw20ul5gmpo";
static const char BCMATTACHDATA(rstr_legofdmbw205ghpo)[] = "legofdmbw205ghpo";
static const char BCMATTACHDATA(rstr_legofdmbw20ul5ghpo)[] = "legofdmbw20ul5ghpo";
static const char BCMATTACHDATA(rstr_mcsbw20ul2gpo)[] = "mcsbw20ul2gpo";
static const char BCMATTACHDATA(rstr_mcsbw20ul5glpo)[] = "mcsbw20ul5glpo";
static const char BCMATTACHDATA(rstr_mcsbw20ul5gmpo)[] = "mcsbw20ul5gmpo";
static const char BCMATTACHDATA(rstr_mcsbw20ul5ghpo)[] = "mcsbw20ul5ghpo";
static const char BCMATTACHDATA(rstr_legofdm40duppo)[] = "legofdm40duppo";
static const char BCMATTACHDATA(rstr_mcsbw20ul5gx1po)[] = "mcsbw20ul5gx1po";
static const char BCMATTACHDATA(rstr_mcsbw20ul5gx2po)[] = "mcsbw20ul5gx2po";

#ifdef BAND5G
static int8
wlc_phy_txpwr40Moffset_srom_convert(uint8 offset)
{
	if (offset == 0xf)
		return 0;
	else if (offset > 0x7) {
		PHY_ERROR(("ILLEGAL 40MHZ PWRCTRL OFFSET VALUE, APPLYING 0 OFFSET \n"));
		return 0;
	} else {
		if (offset < 4)
			return offset;
		else
			return (-8+offset);
	}
}
#endif /* BAND5G */

static const char BCMATTACHDATA(rstr_bw40po)[] = "bw40po";
static const char BCMATTACHDATA(rstr_cddpo)[] = "cddpo";
static const char BCMATTACHDATA(rstr_stbcpo)[] = "stbcpo";
static const char BCMATTACHDATA(rstr_bwduppo)[] = "bwduppo";
static const char BCMATTACHDATA(rstr_txpid2ga0)[] = "txpid2ga0";
static const char BCMATTACHDATA(rstr_txpid2ga1)[] = "txpid2ga1";
static const char BCMATTACHDATA(rstr_pa2gw0a2)[] = "pa2gw0a2";
static const char BCMATTACHDATA(rstr_pa2gw1a2)[] = "pa2gw1a2";
static const char BCMATTACHDATA(rstr_pa2gw2a2)[] = "pa2gw2a2";
static const char BCMATTACHDATA(rstr_maxp5gla2)[] = "maxp5gla2";
static const char BCMATTACHDATA(rstr_pa5glw0a2)[] = "pa5glw0a2";
static const char BCMATTACHDATA(rstr_pa5glw1a2)[] = "pa5glw1a2";
static const char BCMATTACHDATA(rstr_pa5glw2a2)[] = "pa5glw2a2";

static const char BCMATTACHDATA(rstr_pa5gw0a2)[] = "pa5gw0a2";
static const char BCMATTACHDATA(rstr_pa5gw1a2)[] = "pa5gw1a2";
static const char BCMATTACHDATA(rstr_pa5gw2a2)[] = "pa5gw2a2";
static const char BCMATTACHDATA(rstr_maxp5gha2)[] = "maxp5gha2";
static const char BCMATTACHDATA(rstr_pa5ghw0a2)[] = "pa5ghw0a2";
static const char BCMATTACHDATA(rstr_pa5ghw1a2)[] = "pa5ghw1a2";
static const char BCMATTACHDATA(rstr_pa5ghw2a2)[] = "pa5ghw2a2";
static const char BCMATTACHDATA(rstr_pa2gw0a0)[] = "pa2gw0a0";
static const char BCMATTACHDATA(rstr_pa2gw0a1)[] = "pa2gw0a1";
static const char BCMATTACHDATA(rstr_pa2gw1a0)[] = "pa2gw1a0";
static const char BCMATTACHDATA(rstr_pa2gw1a1)[] = "pa2gw1a1";
static const char BCMATTACHDATA(rstr_pa2gw2a0)[] = "pa2gw2a0";
static const char BCMATTACHDATA(rstr_pa2gw2a1)[] = "pa2gw2a1";
static const char BCMATTACHDATA(rstr_itt2ga0)[] = "itt2ga0";
static const char BCMATTACHDATA(rstr_itt2ga1)[] = "itt2ga1";
static const char BCMATTACHDATA(rstr_cck2gpo)[] = "cck2gpo";
static const char BCMATTACHDATA(rstr_ofdm2gpo)[] = "ofdm2gpo";
static const char BCMATTACHDATA(rstr_mcs2gpo0)[] = "mcs2gpo0";
static const char BCMATTACHDATA(rstr_mcs2gpo1)[] = "mcs2gpo1";
static const char BCMATTACHDATA(rstr_mcs2gpo2)[] = "mcs2gpo2";
static const char BCMATTACHDATA(rstr_mcs2gpo3)[] = "mcs2gpo3";
static const char BCMATTACHDATA(rstr_mcs2gpo4)[] = "mcs2gpo4";
static const char BCMATTACHDATA(rstr_mcs2gpo5)[] = "mcs2gpo5";
static const char BCMATTACHDATA(rstr_mcs2gpo6)[] = "mcs2gpo6";
static const char BCMATTACHDATA(rstr_mcs2gpo7)[] = "mcs2gpo7";
static const char BCMATTACHDATA(rstr_txpid5gla0)[] = "txpid5gla0";
static const char BCMATTACHDATA(rstr_txpid5gla1)[] = "txpid5gla1";
static const char BCMATTACHDATA(rstr_maxp5gla0)[] = "maxp5gla0";
static const char BCMATTACHDATA(rstr_maxp5gla1)[] = "maxp5gla1";
static const char BCMATTACHDATA(rstr_pa5glw0a0)[] = "pa5glw0a0";
static const char BCMATTACHDATA(rstr_pa5glw0a1)[] = "pa5glw0a1";
static const char BCMATTACHDATA(rstr_pa5glw1a0)[] = "pa5glw1a0";
static const char BCMATTACHDATA(rstr_pa5glw1a1)[] = "pa5glw1a1";
static const char BCMATTACHDATA(rstr_pa5glw2a0)[] = "pa5glw2a0";
static const char BCMATTACHDATA(rstr_pa5glw2a1)[] = "pa5glw2a1";
static const char BCMATTACHDATA(rstr_ofdm5glpo)[] = "ofdm5glpo";
static const char BCMATTACHDATA(rstr_mcs5glpo0)[] = "mcs5glpo0";
static const char BCMATTACHDATA(rstr_mcs5glpo1)[] = "mcs5glpo1";
static const char BCMATTACHDATA(rstr_mcs5glpo2)[] = "mcs5glpo2";
static const char BCMATTACHDATA(rstr_mcs5glpo3)[] = "mcs5glpo3";
static const char BCMATTACHDATA(rstr_mcs5glpo4)[] = "mcs5glpo4";
static const char BCMATTACHDATA(rstr_mcs5glpo5)[] = "mcs5glpo5";
static const char BCMATTACHDATA(rstr_mcs5glpo6)[] = "mcs5glpo6";
static const char BCMATTACHDATA(rstr_mcs5glpo7)[] = "mcs5glpo7";
static const char BCMATTACHDATA(rstr_txpid5ga0)[] = "txpid5ga0";
static const char BCMATTACHDATA(rstr_txpid5ga1)[] = "txpid5ga1";


static const char BCMATTACHDATA(rstr_pa5gw0a0)[] = "pa5gw0a0";
static const char BCMATTACHDATA(rstr_pa5gw0a1)[] = "pa5gw0a1";
static const char BCMATTACHDATA(rstr_pa5gw1a0)[] = "pa5gw1a0";
static const char BCMATTACHDATA(rstr_pa5gw1a1)[] = "pa5gw1a1";
static const char BCMATTACHDATA(rstr_pa5gw2a0)[] = "pa5gw2a0";
static const char BCMATTACHDATA(rstr_pa5gw2a1)[] = "pa5gw2a1";
static const char BCMATTACHDATA(rstr_itt5ga0)[] = "itt5ga0";
static const char BCMATTACHDATA(rstr_itt5ga1)[] = "itt5ga1";
static const char BCMATTACHDATA(rstr_ofdm5gpo)[] = "ofdm5gpo";
static const char BCMATTACHDATA(rstr_mcs5gpo0)[] = "mcs5gpo0";
static const char BCMATTACHDATA(rstr_mcs5gpo1)[] = "mcs5gpo1";
static const char BCMATTACHDATA(rstr_mcs5gpo2)[] = "mcs5gpo2";
static const char BCMATTACHDATA(rstr_mcs5gpo3)[] = "mcs5gpo3";
static const char BCMATTACHDATA(rstr_mcs5gpo4)[] = "mcs5gpo4";
static const char BCMATTACHDATA(rstr_mcs5gpo5)[] = "mcs5gpo5";
static const char BCMATTACHDATA(rstr_mcs5gpo6)[] = "mcs5gpo6";
static const char BCMATTACHDATA(rstr_mcs5gpo7)[] = "mcs5gpo7";
static const char BCMATTACHDATA(rstr_txpid5gha0)[] = "txpid5gha0";
static const char BCMATTACHDATA(rstr_txpid5gha1)[] = "txpid5gha1";
static const char BCMATTACHDATA(rstr_maxp5gha0)[] = "maxp5gha0";
static const char BCMATTACHDATA(rstr_maxp5gha1)[] = "maxp5gha1";
static const char BCMATTACHDATA(rstr_pa5ghw0a0)[] = "pa5ghw0a0";
static const char BCMATTACHDATA(rstr_pa5ghw0a1)[] = "pa5ghw0a1";
static const char BCMATTACHDATA(rstr_pa5ghw1a0)[] = "pa5ghw1a0";
static const char BCMATTACHDATA(rstr_pa5ghw1a1)[] = "pa5ghw1a1";
static const char BCMATTACHDATA(rstr_pa5ghw2a0)[] = "pa5ghw2a0";
static const char BCMATTACHDATA(rstr_pa5ghw2a1)[] = "pa5ghw2a1";
static const char BCMATTACHDATA(rstr_ofdm5ghpo)[] = "ofdm5ghpo";
static const char BCMATTACHDATA(rstr_mcs5ghpo0)[] = "mcs5ghpo0";
static const char BCMATTACHDATA(rstr_mcs5ghpo1)[] = "mcs5ghpo1";
static const char BCMATTACHDATA(rstr_mcs5ghpo2)[] = "mcs5ghpo2";
static const char BCMATTACHDATA(rstr_mcs5ghpo3)[] = "mcs5ghpo3";
static const char BCMATTACHDATA(rstr_mcs5ghpo4)[] = "mcs5ghpo4";
static const char BCMATTACHDATA(rstr_mcs5ghpo5)[] = "mcs5ghpo5";
static const char BCMATTACHDATA(rstr_mcs5ghpo6)[] = "mcs5ghpo6";
static const char BCMATTACHDATA(rstr_mcs5ghpo7)[] = "mcs5ghpo7";

static void
BCMATTACHFN(wlc_phy_txpwr_srom8_read_ppr)(phy_info_t *pi)
{
		uint16 bw40po, cddpo, stbcpo, bwduppo;
		int band_num;
		phy_info_nphy_t *pi_nphy = pi->u.pi_nphy;
		srom_pwrdet_t	*pwrdet  = pi->pwrdet;

		/* Read bw40po value
		 * each nibble corresponds to 2g, 5g, 5gl, 5gh specific offset respectively
		 */
		bw40po = (uint16)PHY_GETINTVAR(pi, rstr_bw40po);
		pi->ppr->u.sr8.bw40[WL_CHAN_FREQ_RANGE_2G] = bw40po & 0xf;
#ifdef BAND5G
		pi->ppr->u.sr8.bw40[WL_CHAN_FREQ_RANGE_5G_BAND0] = (bw40po & 0xf0) >> 4;
		pi->ppr->u.sr8.bw40[WL_CHAN_FREQ_RANGE_5G_BAND1] = (bw40po & 0xf00) >> 8;
		pi->ppr->u.sr8.bw40[WL_CHAN_FREQ_RANGE_5G_BAND2] = (bw40po & 0xf000) >> 12;
		if (pi->sromi->subband5Gver == PHY_SUBBAND_4BAND)
			pi->ppr->u.sr8.bw40[WL_CHAN_FREQ_RANGE_5G_BAND3] =
				(bw40po & 0xf000) >> 12;
#endif /* BAND5G */
		/* Read cddpo value
		 * each nibble corresponds to 2g, 5g, 5gl, 5gh specific offset respectively
		 */
		cddpo = (uint16)PHY_GETINTVAR(pi, rstr_cddpo);
		pi->ppr->u.sr8.cdd[WL_CHAN_FREQ_RANGE_2G] = cddpo & 0xf;
#ifdef BAND5G
		pi->ppr->u.sr8.cdd[WL_CHAN_FREQ_RANGE_5G_BAND0]  = (cddpo & 0xf0) >> 4;
		pi->ppr->u.sr8.cdd[WL_CHAN_FREQ_RANGE_5G_BAND1]  = (cddpo & 0xf00) >> 8;
		pi->ppr->u.sr8.cdd[WL_CHAN_FREQ_RANGE_5G_BAND2]  = (cddpo & 0xf000) >> 12;
		if (pi->sromi->subband5Gver == PHY_SUBBAND_4BAND)
			pi->ppr->u.sr8.cdd[WL_CHAN_FREQ_RANGE_5G_BAND3]  = (cddpo & 0xf000) >> 12;
#endif /* BAND5G */

		/* Read stbcpo value
		 * each nibble corresponds to 2g, 5g, 5gl, 5gh specific offset respectively
		 */
		stbcpo = (uint16)PHY_GETINTVAR(pi, rstr_stbcpo);
		pi->ppr->u.sr8.stbc[WL_CHAN_FREQ_RANGE_2G] = stbcpo & 0xf;
#ifdef BAND5G
		pi->ppr->u.sr8.stbc[WL_CHAN_FREQ_RANGE_5G_BAND0]  = (stbcpo & 0xf0) >> 4;
		pi->ppr->u.sr8.stbc[WL_CHAN_FREQ_RANGE_5G_BAND1]  = (stbcpo & 0xf00) >> 8;
		pi->ppr->u.sr8.stbc[WL_CHAN_FREQ_RANGE_5G_BAND2]  = (stbcpo & 0xf000) >> 12;
		if (pi->sromi->subband5Gver == PHY_SUBBAND_4BAND)
			pi->ppr->u.sr8.stbc[WL_CHAN_FREQ_RANGE_5G_BAND3]  = (stbcpo & 0xf000) >> 12;
#endif /* BAND5G */

		/* Read bwduppo value
		 * each nibble corresponds to 2g, 5g, 5gl, 5gh specific offset respectively
		 */
		bwduppo = (uint16)PHY_GETINTVAR(pi, rstr_bwduppo);
		pi->ppr->u.sr8.bwdup[WL_CHAN_FREQ_RANGE_2G] = bwduppo & 0xf;
#ifdef BAND5G
		pi->ppr->u.sr8.bwdup[WL_CHAN_FREQ_RANGE_5G_BAND0]  = (bwduppo & 0xf0) >> 4;
		pi->ppr->u.sr8.bwdup[WL_CHAN_FREQ_RANGE_5G_BAND1]  = (bwduppo & 0xf00) >> 8;
		pi->ppr->u.sr8.bwdup[WL_CHAN_FREQ_RANGE_5G_BAND2]  = (bwduppo & 0xf000) >> 12;
		if (pi->sromi->subband5Gver == PHY_SUBBAND_4BAND)
			pi->ppr->u.sr8.bwdup[WL_CHAN_FREQ_RANGE_5G_BAND3]  =
			(bwduppo & 0xf000) >> 12;
#endif /* BAND5G */

		for (band_num = 0; band_num < NUMSUBBANDS(pi); band_num++) {
			switch (band_num) {
				case WL_CHAN_FREQ_RANGE_2G:
					/* 2G band */
					if (ISNPHY(pi)) {
						pi_nphy->nphy_txpid2g[PHY_CORE_0]  =
						(uint8)PHY_GETINTVAR(pi, rstr_txpid2ga0);
						pi_nphy->nphy_pwrctrl_info[PHY_CORE_0].
						idle_targ_2g =
						(int8)PHY_GETINTVAR(pi, rstr_itt2ga0);
					}
					pwrdet->max_pwr[PHY_CORE_0][band_num] =
						(int8)PHY_GETINTVAR(pi, rstr_maxp2ga0);
					pwrdet->pwrdet_a1[PHY_CORE_0][band_num] =
						(int16)PHY_GETINTVAR(pi, rstr_pa2gw0a0);
					pwrdet->pwrdet_b0[PHY_CORE_0][band_num] =
						(int16)PHY_GETINTVAR(pi, rstr_pa2gw1a0);
					pwrdet->pwrdet_b1[PHY_CORE_0][band_num] =
						(int16)PHY_GETINTVAR(pi, rstr_pa2gw2a0);

					if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
						if (ISNPHY(pi)) {
							pi_nphy->nphy_txpid2g[PHY_CORE_1]  =
							(uint8)PHY_GETINTVAR(pi, rstr_txpid2ga1);
							pi_nphy->nphy_pwrctrl_info[PHY_CORE_1].
							idle_targ_2g =
							(int8)PHY_GETINTVAR(pi, rstr_itt2ga1);
						}
						pwrdet->max_pwr[PHY_CORE_1][band_num] =
							(int8)PHY_GETINTVAR(pi, rstr_maxp2ga1);
						pwrdet->pwrdet_a1[PHY_CORE_1][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa2gw0a1);
						pwrdet->pwrdet_b0[PHY_CORE_1][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa2gw1a1);
						pwrdet->pwrdet_b1[PHY_CORE_1][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa2gw2a1);
					}
					if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
						pwrdet->max_pwr[PHY_CORE_2][band_num] =
							(int8)PHY_GETINTVAR(pi, rstr_maxp2ga2);
						pwrdet->pwrdet_a1[PHY_CORE_2][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa2gw0a2);
						pwrdet->pwrdet_b0[PHY_CORE_2][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa2gw1a2);
						pwrdet->pwrdet_b1[PHY_CORE_2][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa2gw2a2);
					}

					/* 2G CCK */
					pi->ppr->u.sr8.cck2gpo =
						(uint16)PHY_GETINTVAR(pi, rstr_cck2gpo);

					/* 2G ofdm2gpo power offsets */
					pi->ppr->u.sr8.ofdm[band_num] =
						(uint32)PHY_GETINTVAR(pi, rstr_ofdm2gpo);

					/* 2G mcs2gpo power offsets */
					pi->ppr->u.sr8.mcs[band_num][0] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs2gpo0);
					pi->ppr->u.sr8.mcs[band_num][1] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs2gpo1);
					pi->ppr->u.sr8.mcs[band_num][2] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs2gpo2);
					pi->ppr->u.sr8.mcs[band_num][3] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs2gpo3);
					pi->ppr->u.sr8.mcs[band_num][4] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs2gpo4);
					pi->ppr->u.sr8.mcs[band_num][5] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs2gpo5);
					pi->ppr->u.sr8.mcs[band_num][6] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs2gpo6);
					pi->ppr->u.sr8.mcs[band_num][7] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs2gpo7);
					break;
#ifdef BAND5G
				case WL_CHAN_FREQ_RANGE_5G_BAND0:
					/* 5G lowband */
					if (ISNPHY(pi)) {
						pi_nphy->nphy_txpid5g[PHY_CORE_0][band_num] =
						(uint8)PHY_GETINTVAR(pi, rstr_txpid5gla0);
					}
					pwrdet->max_pwr[PHY_CORE_0][band_num]  =
						(int8)PHY_GETINTVAR(pi, rstr_maxp5gla0);
					pwrdet->pwrdet_a1[PHY_CORE_0][band_num]	=
						(int16)PHY_GETINTVAR(pi, rstr_pa5glw0a0);
					pwrdet->pwrdet_b0[PHY_CORE_0][band_num]	=
						(int16)PHY_GETINTVAR(pi, rstr_pa5glw1a0);
					pwrdet->pwrdet_b1[PHY_CORE_0][band_num] =
						(int16)PHY_GETINTVAR(pi, rstr_pa5glw2a0);

					if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
						if (ISNPHY(pi)) {
							pi_nphy->nphy_txpid5g[PHY_CORE_1][band_num]
							= (uint8)PHY_GETINTVAR(pi, rstr_txpid5gla1);
						}
						pwrdet->pwrdet_a1[PHY_CORE_1][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5glw0a1);
						pwrdet->max_pwr[PHY_CORE_1][band_num]  =
							(int8)PHY_GETINTVAR(pi, rstr_maxp5gla1);
						pwrdet->pwrdet_b0[PHY_CORE_1][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5glw1a1);
						pwrdet->pwrdet_b1[PHY_CORE_1][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa5glw2a1);
					}
					if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
						pwrdet->pwrdet_a1[PHY_CORE_2][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5glw0a2);
						pwrdet->max_pwr[PHY_CORE_2][band_num]  =
							(int8)PHY_GETINTVAR(pi, rstr_maxp5gla2);
						pwrdet->pwrdet_b0[PHY_CORE_2][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5glw1a2);
						pwrdet->pwrdet_b1[PHY_CORE_2][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa5glw2a2);
					}

					if (ISNPHY(pi)) {
						pi_nphy->nphy_pwrctrl_info[0].
						idle_targ_5g[band_num] = 0;
						pi_nphy->nphy_pwrctrl_info[1].
						idle_targ_5g[band_num] = 0;
					}

					/* 5G lowband ofdm5glpo power offsets */
					pi->ppr->u.sr8.ofdm[band_num] =
						(uint32)PHY_GETINTVAR(pi, rstr_ofdm5glpo);

					/* 5G lowband mcs5glpo power offsets */
					pi->ppr->u.sr8.mcs[band_num][0] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5glpo0);
					pi->ppr->u.sr8.mcs[band_num] [1] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5glpo1);
					pi->ppr->u.sr8.mcs[band_num] [2] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5glpo2);
					pi->ppr->u.sr8.mcs[band_num] [3] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5glpo3);
					pi->ppr->u.sr8.mcs[band_num] [4] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5glpo4);
					pi->ppr->u.sr8.mcs[band_num] [5] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5glpo5);
					pi->ppr->u.sr8.mcs[band_num] [6] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5glpo6);
					pi->ppr->u.sr8.mcs[band_num] [7] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5glpo7);
					break;
				case WL_CHAN_FREQ_RANGE_5G_BAND1:
					/* 5G band, mid */
					if (ISNPHY(pi)) {
						pi_nphy->nphy_txpid5g[PHY_CORE_0][band_num]  =
						(uint8)PHY_GETINTVAR(pi, rstr_txpid5ga0);
					}
					pwrdet->max_pwr[PHY_CORE_0][band_num] =
						(int8)PHY_GETINTVAR(pi, rstr_maxp5ga0);
					pwrdet->pwrdet_a1[PHY_CORE_0][band_num]	=
						(int16)PHY_GETINTVAR(pi, rstr_pa5gw0a0);
					pwrdet->pwrdet_b0[PHY_CORE_0][band_num]	=
						(int16)PHY_GETINTVAR(pi, rstr_pa5gw1a0);
					pwrdet->pwrdet_b1[PHY_CORE_0][band_num] =
						(int16)PHY_GETINTVAR(pi, rstr_pa5gw2a0);

					if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
						if (ISNPHY(pi)) {
							pi_nphy->nphy_txpid5g[PHY_CORE_1][band_num]
							= (uint8)PHY_GETINTVAR(pi, rstr_txpid5ga1);
						}
						pwrdet->pwrdet_a1[PHY_CORE_1][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5gw0a1);
						pwrdet->max_pwr[PHY_CORE_1][band_num]  =
							(int8)PHY_GETINTVAR(pi, rstr_maxp5ga1);
						pwrdet->pwrdet_b0[PHY_CORE_1][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5gw1a1);
						pwrdet->pwrdet_b1[PHY_CORE_1][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa5gw2a1);
					}
					if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
						pwrdet->pwrdet_a1[PHY_CORE_2][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5gw0a2);
						pwrdet->max_pwr[PHY_CORE_2][band_num]  =
							(int8)PHY_GETINTVAR(pi, rstr_maxp5ga2);
						pwrdet->pwrdet_b0[PHY_CORE_2][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5gw1a2);
						pwrdet->pwrdet_b1[PHY_CORE_2][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa5gw2a2);
					}

					if (ISNPHY(pi)) {
						pi_nphy->nphy_pwrctrl_info[PHY_CORE_0].
						idle_targ_5g[band_num] =
						(int8)PHY_GETINTVAR(pi, rstr_itt5ga0);
						pi_nphy->nphy_pwrctrl_info[PHY_CORE_1].
						idle_targ_5g[band_num] =
						(int8)PHY_GETINTVAR(pi, rstr_itt5ga1);
					}

					/* 5G midband ofdm5gpo power offsets */
					pi->ppr->u.sr8.ofdm[band_num] =
						(uint32)PHY_GETINTVAR(pi, rstr_ofdm5gpo);

					/* 5G midband mcs5gpo power offsets */
					pi->ppr->u.sr8.mcs[band_num] [0] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5gpo0);
					pi->ppr->u.sr8.mcs[band_num] [1] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5gpo1);
					pi->ppr->u.sr8.mcs[band_num] [2] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5gpo2);
					pi->ppr->u.sr8.mcs[band_num] [3] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5gpo3);
					pi->ppr->u.sr8.mcs[band_num] [4] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5gpo4);
					pi->ppr->u.sr8.mcs[band_num] [5] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5gpo5);
					pi->ppr->u.sr8.mcs[band_num] [6] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5gpo6);
					pi->ppr->u.sr8.mcs[band_num] [7] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5gpo7);
					break;
				case WL_CHAN_FREQ_RANGE_5G_BAND2:
					/* 5G highband */
					if (ISNPHY(pi)) {
						pi_nphy->nphy_txpid5g[PHY_CORE_0][band_num] =
						(uint8)PHY_GETINTVAR(pi, rstr_txpid5gha0);
					}
					pwrdet->max_pwr[PHY_CORE_0][band_num]  =
						(int8)PHY_GETINTVAR(pi, rstr_maxp5gha0);
					pwrdet->pwrdet_a1[PHY_CORE_0][band_num]	=
						(int16)PHY_GETINTVAR(pi, rstr_pa5ghw0a0);
					pwrdet->pwrdet_b0[PHY_CORE_0][band_num]	=
						(int16)PHY_GETINTVAR(pi, rstr_pa5ghw1a0);
					pwrdet->pwrdet_b1[PHY_CORE_0][band_num] =
						(int16)PHY_GETINTVAR(pi, rstr_pa5ghw2a0);

					if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
						if (ISNPHY(pi)) {
							pi_nphy->nphy_txpid5g[PHY_CORE_1][band_num]
							= (uint8)PHY_GETINTVAR(pi, rstr_txpid5gha1);
						}
						pwrdet->max_pwr[PHY_CORE_1][band_num]  =
							(int8)PHY_GETINTVAR(pi, rstr_maxp5gha1);
						pwrdet->pwrdet_a1[PHY_CORE_1][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw0a1);
						pwrdet->pwrdet_b0[PHY_CORE_1][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw1a1);
						pwrdet->pwrdet_b1[PHY_CORE_1][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw2a1);
					}
					if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
						pwrdet->max_pwr[PHY_CORE_2][band_num]  =
							(int8)PHY_GETINTVAR(pi, rstr_maxp5gha2);
						pwrdet->pwrdet_a1[PHY_CORE_2][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw0a2);
						pwrdet->pwrdet_b0[PHY_CORE_2][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw1a2);
						pwrdet->pwrdet_b1[PHY_CORE_2][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw2a2);
					}

					if (ISNPHY(pi)) {
						pi_nphy->nphy_pwrctrl_info[PHY_CORE_0].
						idle_targ_5g[band_num] = 0;
						pi_nphy->nphy_pwrctrl_info[PHY_CORE_1].
						idle_targ_5g[band_num] = 0;
					}

					/* 5G highband ofdm5ghpo power offsets */
					pi->ppr->u.sr8.ofdm[band_num] =
						(uint32)PHY_GETINTVAR(pi, rstr_ofdm5ghpo);

					/* 5G highband mcs5ghpo power offsets */
					pi->ppr->u.sr8.mcs[band_num][0] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo0);
					pi->ppr->u.sr8.mcs[band_num][1] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo1);
					pi->ppr->u.sr8.mcs[band_num][2] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo2);
					pi->ppr->u.sr8.mcs[band_num][3] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo3);
					pi->ppr->u.sr8.mcs[band_num][4] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo4);
					pi->ppr->u.sr8.mcs[band_num][5] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo5);
					pi->ppr->u.sr8.mcs[band_num][6] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo6);
					pi->ppr->u.sr8.mcs[band_num][7] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo7);
					break;

				case WL_CHAN_FREQ_RANGE_5G_BAND3:
					/* 5G highband */
					if (ISNPHY(pi)) {
						pi_nphy->nphy_txpid5g[PHY_CORE_0][band_num] =
						(uint8)PHY_GETINTVAR(pi, rstr_txpid5gha0);
					}
					pwrdet->max_pwr[PHY_CORE_0][band_num]  =
						(int8)PHY_GETINTVAR(pi, rstr_maxp5gha0);
					pwrdet->pwrdet_a1[PHY_CORE_0][band_num]	=
						(int16)PHY_GETINTVAR(pi, rstr_pa5ghw0a0);
					pwrdet->pwrdet_b0[PHY_CORE_0][band_num]	=
						(int16)PHY_GETINTVAR(pi, rstr_pa5ghw1a0);
					pwrdet->pwrdet_b1[PHY_CORE_0][band_num] =
						(int16)PHY_GETINTVAR(pi, rstr_pa5ghw2a0);

					if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
						if (ISNPHY(pi)) {
							pi_nphy->nphy_txpid5g[PHY_CORE_1][band_num]
							= (uint8)PHY_GETINTVAR(pi, rstr_txpid5gha1);
						}
						pwrdet->max_pwr[PHY_CORE_1][band_num]  =
							(int8)PHY_GETINTVAR(pi, rstr_maxp5gha1);
						pwrdet->pwrdet_a1[PHY_CORE_1][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw0a1);
						pwrdet->pwrdet_b0[PHY_CORE_1][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw1a1);
						pwrdet->pwrdet_b1[PHY_CORE_1][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw2a1);
					}
					if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
						pwrdet->max_pwr[PHY_CORE_2][band_num]  =
							(int8)PHY_GETINTVAR(pi, rstr_maxp5gha2);
						pwrdet->pwrdet_a1[PHY_CORE_2][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw0a2);
						pwrdet->pwrdet_b0[PHY_CORE_2][band_num]	=
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw1a2);
						pwrdet->pwrdet_b1[PHY_CORE_2][band_num] =
							(int16)PHY_GETINTVAR(pi, rstr_pa5ghw2a2);
					}

					if (ISNPHY(pi)) {
						pi_nphy->nphy_pwrctrl_info[PHY_CORE_0].
						idle_targ_5g[band_num] = 0;
						pi_nphy->nphy_pwrctrl_info[PHY_CORE_1].
						idle_targ_5g[band_num] = 0;
					}

					/* 5G highband ofdm5ghpo power offsets */
					pi->ppr->u.sr8.ofdm[band_num] =
						(uint32)PHY_GETINTVAR(pi, rstr_ofdm5ghpo);

					/* 5G highband mcs5ghpo power offsets */
					pi->ppr->u.sr8.mcs[band_num][0] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo0);
					pi->ppr->u.sr8.mcs[band_num][1] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo1);
					pi->ppr->u.sr8.mcs[band_num][2] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo2);
					pi->ppr->u.sr8.mcs[band_num][3] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo3);
					pi->ppr->u.sr8.mcs[band_num][4] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo4);
					pi->ppr->u.sr8.mcs[band_num][5] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo5);
					pi->ppr->u.sr8.mcs[band_num][6] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo6);
					pi->ppr->u.sr8.mcs[band_num][7] =
						(uint16)PHY_GETINTVAR(pi, rstr_mcs5ghpo7);
					break;
#endif /* BAND5G */
				}
			}

		/* Finished reading from SROM, calculate and apply powers */
}

static void
BCMATTACHFN(wlc_phy_txpwr_srom9_read_ppr)(phy_info_t *pi)
{

	/* Read and interpret the power-offset parameters from the SROM for each band/subband */
	if (pi->sh->sromrev >= 9) {
		int i, j;

		PHY_INFORM(("Get SROM 9 Power Offset per rate\n"));
		/* 2G CCK */
		pi->ppr->u.sr9.cckbw202gpo = (uint16)PHY_GETINTVAR(pi, rstr_cckbw202gpo);
		pi->ppr->u.sr9.cckbw20ul2gpo = (uint16)PHY_GETINTVAR(pi, rstr_cckbw20ul2gpo);
		/* 2G OFDM power offsets */
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_2G].bw20 =
			(uint32)PHY_GETINTVAR(pi, rstr_legofdmbw202gpo);
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_2G].bw20ul =
			(uint32)PHY_GETINTVAR(pi, rstr_legofdmbw20ul2gpo);
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_2G].bw20 =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw202gpo);
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_2G].bw20ul =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw20ul2gpo);
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_2G].bw40 =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw402gpo);

#ifdef BAND5G
		/* 5G power offsets */
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND0].bw20 =
			(uint32)PHY_GETINTVAR(pi, rstr_legofdmbw205glpo);
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND0].bw20ul =
			(uint32)PHY_GETINTVAR(pi, rstr_legofdmbw20ul5glpo);
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND1].bw20 =
			(uint32)PHY_GETINTVAR(pi, rstr_legofdmbw205gmpo);
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND1].bw20ul =
			(uint32)PHY_GETINTVAR(pi, rstr_legofdmbw20ul5gmpo);
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND2].bw20 =
			(uint32)PHY_GETINTVAR(pi, rstr_legofdmbw205ghpo);
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND2].bw20ul =
			(uint32)PHY_GETINTVAR(pi, rstr_legofdmbw20ul5ghpo);

		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND0].bw20 =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw205glpo);
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND0].bw20ul =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw20ul5glpo);
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND0].bw40 =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw405glpo);
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND1].bw20 =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw205gmpo);
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND1].bw20ul =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw20ul5gmpo);
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND1].bw40 =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw405gmpo);
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND2].bw20 =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw205ghpo);
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND2].bw20ul =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw20ul5ghpo);
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND2].bw40 =
			(uint32)PHY_GETINTVAR(pi, rstr_mcsbw405ghpo);
		if (pi->sromi->subband5Gver == PHY_SUBBAND_4BAND)
		{
			pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND3].bw20 =
				(uint32)PHY_GETINTVAR(pi, rstr_legofdmbw205ghpo);
			pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND3].bw20ul =
				(uint32)PHY_GETINTVAR(pi, rstr_legofdmbw20ul5ghpo);

			pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND3].bw20 =
				(uint32)PHY_GETINTVAR(pi, rstr_mcsbw205ghpo);
			pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND3].bw20ul =
				(uint32)PHY_GETINTVAR(pi, rstr_mcsbw20ul5ghpo);
			pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_5G_BAND3].bw40 =
				(uint32)PHY_GETINTVAR(pi, rstr_mcsbw405ghpo);
		}
#endif /* BAND5G */

		/* 40 Dups */
		pi->ppr->u.sr9.ofdm40duppo = (uint16)PHY_GETINTVAR(pi, rstr_legofdm40duppo);
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_2G].bw40 =
			pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_2G].bw20ul;
		for (i = 0; i < NUMSUBBANDS(pi); i++) {
			uint32 nibble, dup40_offset = 0;
			nibble = pi->ppr->u.sr9.ofdm40duppo & 0xf;
			for (j = 0; j < WL_NUM_RATES_OFDM; j++) {
				dup40_offset |= nibble;
				nibble <<= 4;
			}
			if (i == 0)
				pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_2G].bw40 =
				pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_2G].bw20ul +
				dup40_offset;
#ifdef BAND5G
			else if (i == 1)
				pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND0].bw40 =
				pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND0].bw20ul +
				dup40_offset;
			else if (i == 2)
				pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND1].bw40 =
				pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND1].bw20ul +
				dup40_offset;
			else if (i == 3)
				pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND2].bw40 =
				pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND2].bw20ul +
				dup40_offset;
			else if (i == 4)
				pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND3].bw40 =
				pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_5G_BAND3].bw20ul +
				dup40_offset;

#endif /* BAND5G */
		}
	}

	PHY_INFORM(("CCK: 0x%04x 0x%04x\n", pi->ppr->u.sr9.cckbw202gpo,
		pi->ppr->u.sr9.cckbw202gpo));
	PHY_INFORM(("OFDM: 0x%08x 0x%08x 0x%02x\n",
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_2G].bw20,
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_2G].bw20ul,
		pi->ppr->u.sr9.ofdm[WL_CHAN_FREQ_RANGE_2G].bw40));
	PHY_INFORM(("MCS: 0x%08x 0x%08x 0x%08x\n",
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_2G].bw20,
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_2G].bw20ul,
		pi->ppr->u.sr9.mcs[WL_CHAN_FREQ_RANGE_2G].bw40));
}

static const char BCMATTACHDATA(rstr_elna2g)[] = "elna2g";
static const char BCMATTACHDATA(rstr_elna5g)[] = "elna5g";
static const char BCMATTACHDATA(rstr_antswitch)[] = "antswitch";
static const char BCMATTACHDATA(rstr_aa2g)[] = "aa2g";
static const char BCMATTACHDATA(rstr_aa5g)[] = "aa5g";
static const char BCMATTACHDATA(rstr_tssipos2g)[] = "tssipos2g";
static const char BCMATTACHDATA(rstr_pdetrange2g)[] = "pdetrange2g";
static const char BCMATTACHDATA(rstr_triso2g)[] = "triso2g";
static const char BCMATTACHDATA(rstr_antswctl2g)[] = "antswctl2g";
static const char BCMATTACHDATA(rstr_tssipos5g)[] = "tssipos5g";
static const char BCMATTACHDATA(rstr_pdetrange5g)[] = "pdetrange5g";
static const char BCMATTACHDATA(rstr_triso5g)[] = "triso5g";
static const char BCMATTACHDATA(rstr_antswctl5g)[] = "antswctl5g";
static const char BCMATTACHDATA(rstr_pcieingress_war)[] = "pcieingress_war";
static const char BCMATTACHDATA(rstr_pa2gw0a3)[] = "pa2gw0a3";
static const char BCMATTACHDATA(rstr_pa2gw1a3)[] = "pa2gw1a3";
static const char BCMATTACHDATA(rstr_pa2gw2a3)[] = "pa2gw2a3";

bool
BCMATTACHFN(wlc_phy_txpwr_srom8_read)(phy_info_t *pi)
{

	/* read in antenna-related config */
	pi->antswitch = (uint8) PHY_GETINTVAR(pi, rstr_antswitch);
	pi->aa2g = (uint8) PHY_GETINTVAR(pi, rstr_aa2g);

#ifdef BAND5G
	pi->aa5g = (uint8) PHY_GETINTVAR(pi, rstr_aa5g);
#endif /* BAND5G */

	/* read in FEM stuff */
	pi->fem2g->tssipos = (uint8)PHY_GETINTVAR(pi, rstr_tssipos2g);
	pi->fem2g->extpagain = (uint8)PHY_GETINTVAR(pi, rstr_extpagain2g);
	pi->fem2g->pdetrange = (uint8)PHY_GETINTVAR(pi, rstr_pdetrange2g);
	pi->fem2g->triso = (uint8)PHY_GETINTVAR(pi, rstr_triso2g);
	pi->fem2g->antswctrllut = (uint8)PHY_GETINTVAR(pi, rstr_antswctl2g);

#ifdef BAND5G
	pi->fem5g->tssipos = (uint8)PHY_GETINTVAR(pi, rstr_tssipos5g);
	pi->fem5g->extpagain = (uint8)PHY_GETINTVAR(pi, rstr_extpagain5g);
	pi->fem5g->pdetrange = (uint8)PHY_GETINTVAR(pi, rstr_pdetrange5g);
	pi->fem5g->triso = (uint8)PHY_GETINTVAR(pi, rstr_triso5g);

	/* If antswctl5g entry exists, use it.
	 * Fallback to antswctl2g value if 5g entry does not exist.
	 * Previous code used 2g value only, thus...
	 * this is a WAR for any legacy NVRAMs that only had a 2g entry.
	 */
	pi->fem5g->antswctrllut = (uint8)PHY_GETINTVAR_DEFAULT(pi, rstr_antswctl5g,
		PHY_GETINTVAR(pi, rstr_antswctl2g));
#endif /* BAND5G */

	if (PHY_GETVAR(pi, rstr_elna2g)) {
		/* extlnagain2g entry exists, so use it. */
		if (ISNPHY(pi)) {
			pi->u.pi_nphy->elna2g = (uint8)PHY_GETINTVAR(pi, rstr_elna2g);
		}
	}
#ifdef BAND5G
	if (PHY_GETVAR(pi, rstr_elna5g)) {
		/* extlnagain5g entry exists, so use it. */
		if (ISNPHY(pi)) {
			pi->u.pi_nphy->elna5g = (uint8)PHY_GETINTVAR(pi, rstr_elna5g);
		}
	}
#endif /* BAND5G */

	/* srom_fem2/5g.extpagain changed */
	wlc_phy_txpower_ipa_upd(pi);

	pi->phy_tempsense_offset = (int8)PHY_GETINTVAR(pi, rstr_tempoffset);
	if (pi->phy_tempsense_offset != 0) {
		if (pi->phy_tempsense_offset >
			(NPHY_SROM_TEMPSHIFT + NPHY_SROM_MAXTEMPOFFSET)) {
			pi->phy_tempsense_offset = NPHY_SROM_MAXTEMPOFFSET;
		} else if (pi->phy_tempsense_offset < (NPHY_SROM_TEMPSHIFT +
			NPHY_SROM_MINTEMPOFFSET)) {
			pi->phy_tempsense_offset = NPHY_SROM_MINTEMPOFFSET;
		} else {
			pi->phy_tempsense_offset -= NPHY_SROM_TEMPSHIFT;
		}
	}

	wlc_phy_read_tempdelta_settings(pi, NPHY_CAL_MAXTEMPDELTA);

	/* Power per Rate */
	wlc_phy_txpwr_srom8_read_ppr(pi);

	return TRUE;

}

static const char BCMATTACHDATA(rstr_maxp5ga3)[] = "maxp5ga3";
static const char BCMATTACHDATA(rstr_maxp5gla3)[] = "maxp5gla3";
static const char BCMATTACHDATA(rstr_pa5gw0a3)[] = "pa5gw0a3";
static const char BCMATTACHDATA(rstr_pa5glw0a3)[] = "pa5glw0a3";
static const char BCMATTACHDATA(rstr_pa5gw1a3)[] = "pa5gw1a3";
static const char BCMATTACHDATA(rstr_pa5glw1a3)[] = "pa5glw1a3";
static const char BCMATTACHDATA(rstr_pa5gw2a3)[] = "pa5gw2a3";
static const char BCMATTACHDATA(rstr_pa5glw2a3)[] = "pa5glw2a3";
static const char BCMATTACHDATA(rstr_maxp5gha3)[] = "maxp5gha3";
static const char BCMATTACHDATA(rstr_pa5ghw0a3)[] = "pa5ghw0a3";
static const char BCMATTACHDATA(rstr_pa5ghw1a3)[] = "pa5ghw1a3";
static const char BCMATTACHDATA(rstr_pa5ghw2a3)[] = "pa5ghw2a3";

/* */
bool
BCMATTACHFN(wlc_phy_txpwr_srom9_read)(phy_info_t *pi)
{
	srom_pwrdet_t	*pwrdet  = pi->pwrdet;
#ifdef BAND5G
	uint32 offset_40MHz[PHY_MAX_CORES] = {0};
#endif /* BAND5G */
	int b;

	if (PHY_GETVAR(pi, rstr_elna2g)) {
		/* extlnagain2g entry exists, so use it. */
		if (ISNPHY(pi)) {
			pi->u.pi_nphy->elna2g = (uint8)PHY_GETINTVAR(pi, rstr_elna2g);
		}
	}
#ifdef BAND5G
	if (PHY_GETVAR(pi, rstr_elna5g)) {
		/* extlnagain5g entry exists, so use it. */
		if (ISNPHY(pi)) {
			pi->u.pi_nphy->elna5g = (uint8)PHY_GETINTVAR(pi, rstr_elna5g);
		}
	}
#endif /* BAND5G */

	/* read in antenna-related config */
	pi->antswitch = (uint8) PHY_GETINTVAR(pi, rstr_antswitch);
	pi->aa2g = (uint8) PHY_GETINTVAR(pi, rstr_aa2g);
#ifdef BAND5G
	pi->aa5g = (uint8) PHY_GETINTVAR(pi, rstr_aa5g);
#endif /* BAND5G */

	/* read in FEM stuff */
	pi->fem2g->tssipos = (uint8)PHY_GETINTVAR(pi, rstr_tssipos2g);
	pi->fem2g->extpagain = (uint8)PHY_GETINTVAR(pi, rstr_extpagain2g);
	pi->fem2g->pdetrange = (uint8)PHY_GETINTVAR(pi, rstr_pdetrange2g);
	pi->fem2g->triso = (uint8)PHY_GETINTVAR(pi, rstr_triso2g);
	pi->fem2g->antswctrllut = (uint8)PHY_GETINTVAR(pi, rstr_antswctl2g);

#ifdef BAND5G
	pi->fem5g->tssipos = (uint8)PHY_GETINTVAR(pi, rstr_tssipos5g);
	pi->fem5g->extpagain = (uint8)PHY_GETINTVAR(pi, rstr_extpagain5g);
	pi->fem5g->pdetrange = (uint8)PHY_GETINTVAR(pi, rstr_pdetrange5g);
	pi->fem5g->triso = (uint8)PHY_GETINTVAR(pi, rstr_triso5g);
	pi->fem5g->antswctrllut = (uint8)PHY_GETINTVAR(pi, rstr_antswctl5g);
#endif /* BAND5G */

	/* srom_fem2/5g.extpagain changed */
	if (ISNPHY(pi))
		wlc_phy_txpower_ipa_upd(pi);


#ifdef BAND5G
	offset_40MHz[PHY_CORE_0] = PHY_GETINTVAR(pi, rstr_pa2gw0a3);
	if (PHYCORENUM(pi->pubpi->phy_corenum) > 1)
		offset_40MHz[PHY_CORE_1] = PHY_GETINTVAR(pi, rstr_pa2gw1a3);
	if (PHYCORENUM(pi->pubpi->phy_corenum) > 2)
		offset_40MHz[PHY_CORE_2] = PHY_GETINTVAR(pi, rstr_pa2gw2a3);
#endif /* BAND5G */

	/* read pwrdet params for each band/subband */
	for (b = 0; b < NUMSUBBANDS(pi); b++) {
		switch (b) {
		case WL_CHAN_FREQ_RANGE_2G: /* 0 */
			/* 2G band */
			pwrdet->max_pwr[PHY_CORE_0][b] = (int8)PHY_GETINTVAR(pi, rstr_maxp2ga0);
			pwrdet->pwrdet_a1[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa2gw0a0);
			pwrdet->pwrdet_b0[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa2gw1a0);
			pwrdet->pwrdet_b1[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa2gw2a0);
			pwrdet->pwr_offset40[PHY_CORE_0][b] = 0;

			if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
				pwrdet->max_pwr[PHY_CORE_1][b] =
					(int8)PHY_GETINTVAR(pi, rstr_maxp2ga1);
				pwrdet->pwrdet_a1[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa2gw0a1);
				pwrdet->pwrdet_b0[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa2gw1a1);
				pwrdet->pwrdet_b1[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa2gw2a1);
				pwrdet->pwr_offset40[PHY_CORE_1][b] = 0;
			}
			if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
				pwrdet->max_pwr[PHY_CORE_2][b] =
					(int8)PHY_GETINTVAR(pi, rstr_maxp2ga2);
				pwrdet->pwrdet_a1[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa2gw0a2);
				pwrdet->pwrdet_b0[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa2gw1a2);
				pwrdet->pwrdet_b1[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa2gw2a2);
				pwrdet->pwr_offset40[PHY_CORE_2][b] = 0;
			}
			break;
#ifdef BAND5G
		case WL_CHAN_FREQ_RANGE_5G_BAND0: /* 1 */
			pwrdet->max_pwr[PHY_CORE_0][b] = (int8)PHY_GETINTVAR(pi, rstr_maxp5gla0);
			pwrdet->pwrdet_a1[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5glw0a0);
			pwrdet->pwrdet_b0[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5glw1a0);
			pwrdet->pwrdet_b1[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5glw2a0);
			pwrdet->pwr_offset40[PHY_CORE_0][b] = wlc_phy_txpwr40Moffset_srom_convert(
				(offset_40MHz[0] & PWROFFSET40_MASK_0)
					>> PWROFFSET40_SHIFT_0);
			if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
				pwrdet->max_pwr[PHY_CORE_1][b] =
					(int8)PHY_GETINTVAR(pi, rstr_maxp5gla1);
				pwrdet->pwrdet_a1[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5glw0a1);
				pwrdet->pwrdet_b0[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5glw1a1);
				pwrdet->pwrdet_b1[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5glw2a1);
				pwrdet->pwr_offset40[PHY_CORE_1][b] =
					wlc_phy_txpwr40Moffset_srom_convert(
					(offset_40MHz[1] & PWROFFSET40_MASK_0)
						>> PWROFFSET40_SHIFT_0);
			}
			if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
				pwrdet->max_pwr[PHY_CORE_2][b] =
					(int8)PHY_GETINTVAR(pi, rstr_maxp5gla2);
				pwrdet->pwrdet_a1[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5glw0a2);
				pwrdet->pwrdet_b0[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5glw1a2);
				pwrdet->pwrdet_b1[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5glw2a2);
				pwrdet->pwr_offset40[PHY_CORE_2][b] =
					wlc_phy_txpwr40Moffset_srom_convert(
					(offset_40MHz[2] & PWROFFSET40_MASK_0)
						>> PWROFFSET40_SHIFT_0);
			}
			break;

		case WL_CHAN_FREQ_RANGE_5G_BAND1: /* 2 */
			pwrdet->max_pwr[PHY_CORE_0][b] = (int8)PHY_GETINTVAR(pi, rstr_maxp5ga0);
			pwrdet->pwrdet_a1[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5gw0a0);
			pwrdet->pwrdet_b0[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5gw1a0);
			pwrdet->pwrdet_b1[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5gw2a0);
			pwrdet->pwr_offset40[PHY_CORE_0][b] = wlc_phy_txpwr40Moffset_srom_convert(
				(offset_40MHz[0] & PWROFFSET40_MASK_1) >> PWROFFSET40_SHIFT_1);
			if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
				pwrdet->max_pwr[PHY_CORE_1][b] =
					(int8)PHY_GETINTVAR(pi, rstr_maxp5ga1);
				pwrdet->pwrdet_a1[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5gw0a1);
				pwrdet->pwrdet_b0[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5gw1a1);
				pwrdet->pwrdet_b1[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5gw2a1);
				pwrdet->pwr_offset40[PHY_CORE_1][b] =
					wlc_phy_txpwr40Moffset_srom_convert(
					(offset_40MHz[1] & PWROFFSET40_MASK_1)
						>> PWROFFSET40_SHIFT_1);
			}

			if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
				pwrdet->max_pwr[PHY_CORE_2][b] =
					(int8)PHY_GETINTVAR(pi, rstr_maxp5ga2);
				pwrdet->pwrdet_a1[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5gw0a2);
				pwrdet->pwrdet_b0[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5gw1a2);
				pwrdet->pwrdet_b1[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5gw2a2);
				pwrdet->pwr_offset40[PHY_CORE_2][b] =
					wlc_phy_txpwr40Moffset_srom_convert(
					(offset_40MHz[2] & PWROFFSET40_MASK_1)
						>> PWROFFSET40_SHIFT_1);
			}
			break;

		case WL_CHAN_FREQ_RANGE_5G_BAND2: /* 3 */
			pwrdet->max_pwr[PHY_CORE_0][b] = (int8)PHY_GETINTVAR(pi, rstr_maxp5gha0);
			pwrdet->pwrdet_a1[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5ghw0a0);
			pwrdet->pwrdet_b0[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5ghw1a0);
			pwrdet->pwrdet_b1[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5ghw2a0);
			pwrdet->pwr_offset40[0][b] = wlc_phy_txpwr40Moffset_srom_convert(
				(offset_40MHz[PHY_CORE_0] & PWROFFSET40_MASK_2) >>
					PWROFFSET40_SHIFT_2);
			if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
				pwrdet->max_pwr[PHY_CORE_1][b] =
					(int8)PHY_GETINTVAR(pi, rstr_maxp5gha1);
				pwrdet->pwrdet_a1[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5ghw0a1);
				pwrdet->pwrdet_b0[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5ghw1a1);
				pwrdet->pwrdet_b1[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5ghw2a1);
				pwrdet->pwr_offset40[1][b] = wlc_phy_txpwr40Moffset_srom_convert(
					(offset_40MHz[PHY_CORE_1] & PWROFFSET40_MASK_2) >>
						PWROFFSET40_SHIFT_2);
			}

			if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
				pwrdet->max_pwr[PHY_CORE_2][b] =
					(int8)PHY_GETINTVAR(pi, rstr_maxp5gha2);
				pwrdet->pwrdet_a1[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5ghw0a2);
				pwrdet->pwrdet_b0[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5ghw1a2);
				pwrdet->pwrdet_b1[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5ghw2a2);
				pwrdet->pwr_offset40[2][b] =
					wlc_phy_txpwr40Moffset_srom_convert(
					(offset_40MHz[PHY_CORE_2] & PWROFFSET40_MASK_2)
						>> PWROFFSET40_SHIFT_2);
			}
			break;

		case WL_CHAN_FREQ_RANGE_5G_BAND3: /* 4 */
			pwrdet->max_pwr[PHY_CORE_0][b] = (int8)PHY_GETINTVAR(pi, rstr_maxp5ga3);
			pwrdet->pwrdet_a1[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5gw0a3);
			pwrdet->pwrdet_b0[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5gw1a3);
			pwrdet->pwrdet_b1[PHY_CORE_0][b] = (int16)PHY_GETINTVAR(pi, rstr_pa5gw2a3);
			pwrdet->pwr_offset40[PHY_CORE_0][b] = wlc_phy_txpwr40Moffset_srom_convert(
				(offset_40MHz[0] & PWROFFSET40_MASK_3) >> PWROFFSET40_SHIFT_3);
			if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
				pwrdet->max_pwr[PHY_CORE_1][b] =
					(int8)PHY_GETINTVAR(pi, rstr_maxp5gla3);
				pwrdet->pwrdet_a1[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5glw0a3);
				pwrdet->pwrdet_b0[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5glw1a3);
				pwrdet->pwrdet_b1[PHY_CORE_1][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5glw2a3);
				pwrdet->pwr_offset40[PHY_CORE_1][b] =
					wlc_phy_txpwr40Moffset_srom_convert(
					(offset_40MHz[1] & PWROFFSET40_MASK_3)
						>> PWROFFSET40_SHIFT_3);
			}

			if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
				pwrdet->max_pwr[PHY_CORE_2][b] =
					(int8)PHY_GETINTVAR(pi, rstr_maxp5gha3);
				pwrdet->pwrdet_a1[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5ghw0a3);
				pwrdet->pwrdet_b0[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5ghw1a3);
				pwrdet->pwrdet_b1[PHY_CORE_2][b] =
					(int16)PHY_GETINTVAR(pi, rstr_pa5ghw2a3);
				pwrdet->pwr_offset40[PHY_CORE_2][b] =
					wlc_phy_txpwr40Moffset_srom_convert(
					(offset_40MHz[2] & PWROFFSET40_MASK_3)
						>> PWROFFSET40_SHIFT_3);
			}
			break;
#endif /* BAND5G */
		}
	}
	wlc_phy_txpwr_srom9_read_ppr(pi);
	return TRUE;
}

void
wlc_phy_antsel_init(wlc_phy_t *ppi, bool lut_init)
{
	UNUSED_PARAMETER(ppi);
	UNUSED_PARAMETER(lut_init);

	BCM_REFERENCE(rstr_interference);
	BCM_REFERENCE(rstr_txpwrbckof);
	BCM_REFERENCE(rstr_tssilimucod);
	BCM_REFERENCE(rstr_rssicorrnorm);
	BCM_REFERENCE(rstr_rssicorratten);
	BCM_REFERENCE(rstr_rssicorrnorm5g);
	BCM_REFERENCE(rstr_rssicorratten5g);
	BCM_REFERENCE(rstr_rssicorrperrg2g);
	BCM_REFERENCE(rstr_rssicorrperrg5g);
	BCM_REFERENCE(rstr_5g_cga);
	BCM_REFERENCE(rstr_2g_cga);
	BCM_REFERENCE(rstr_phycal);
	BCM_REFERENCE(rstr_mintxpower);
	BCM_REFERENCE(rstr_mcsbw205gx1po);
	BCM_REFERENCE(rstr_mcsbw20ul5gx1po);
	BCM_REFERENCE(rstr_mcsbw405gx1po);
	BCM_REFERENCE(rstr_mcsbw205gx2po);
	BCM_REFERENCE(rstr_mcsbw20ul5gx2po);
	BCM_REFERENCE(rstr_mcsbw405gx2po);
	BCM_REFERENCE(rstr_mcslr5gx1po);
	BCM_REFERENCE(rstr_mcslr5gx2po);
	BCM_REFERENCE(rstr_mcsbw805gx1po);
	BCM_REFERENCE(rstr_mcsbw805gx2po);
	BCM_REFERENCE(rstr_sb20in80and160lr5gx1po);
	BCM_REFERENCE(rstr_sb20in80and160hr5gx1po);
	BCM_REFERENCE(rstr_sb20in80and160lr5gx2po);
	BCM_REFERENCE(rstr_sb20in80and160hr5gx2po);
	BCM_REFERENCE(rstr_sb40and80lr5gx1po);
	BCM_REFERENCE(rstr_sb40and80hr5gx1po);
	BCM_REFERENCE(rstr_sb40and80lr5gx2po);
	BCM_REFERENCE(rstr_sb40and80hr5gx2po);
	BCM_REFERENCE(rstr_pcieingress_war);
}

bool
wlc_phy_no_cal_possible(phy_info_t *pi)
{
	return (SCAN_RM_IN_PROGRESS(pi));
}

/* a simple implementation of gcd(greatest common divisor)
 * assuming argument 1 is bigger than argument 2, both of them
 * are positive numbers.
 */
uint32
wlc_phy_gcd(uint32 bigger, uint32 smaller)
{
	uint32 remainder;

	do {
		remainder = bigger % smaller;
		if (remainder) {
			bigger = smaller;
			smaller = remainder;
		} else {
			return smaller;
		}
	} while (TRUE);
}

#if defined(LCNCONF) || defined(LCN40CONF) || (defined(LCN20CONF) && (LCN20CONF != 0))
void
wlc_phy_get_paparams_for_band(phy_info_t *pi, int32 *a1, int32 *b0, int32 *b1)
{
	/* On lcnphy, estPwrLuts0/1 table entries are in S6.3 format */
	switch (wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec)) {
	case WL_CHAN_FREQ_RANGE_2G:
			/* 2.4 GHz */
			ASSERT((pi->txpa_2g[0] != -1) && (pi->txpa_2g[1] != -1) &&
				(pi->txpa_2g[2] != -1));
			*b0 = pi->txpa_2g[0];
			*b1 = pi->txpa_2g[1];
			*a1 = pi->txpa_2g[2];
			break;
#ifdef BAND5G
	case WL_CHAN_FREQ_RANGE_5GL:
			/* 5 GHz low */
			ASSERT((pi->txpa_5g_low[0] != -1) &&
				(pi->txpa_5g_low[1] != -1) &&
				(pi->txpa_5g_low[2] != -1));
			*b0 = pi->txpa_5g_low[0];
			*b1 = pi->txpa_5g_low[1];
			*a1 = pi->txpa_5g_low[2];
			break;

		case WL_CHAN_FREQ_RANGE_5GM:
			/* 5 GHz middle */
			ASSERT((pi->txpa_5g_mid[0] != -1) &&
				(pi->txpa_5g_mid[1] != -1) &&
				(pi->txpa_5g_mid[2] != -1));
			*b0 = pi->txpa_5g_mid[0];
			*b1 = pi->txpa_5g_mid[1];
			*a1 = pi->txpa_5g_mid[2];
			break;

		case WL_CHAN_FREQ_RANGE_5GH:
			/* 5 GHz high */
			ASSERT((pi->txpa_5g_hi[0] != -1) &&
				(pi->txpa_5g_hi[1] != -1) &&
				(pi->txpa_5g_hi[2] != -1));
			*b0 = pi->txpa_5g_hi[0];
			*b1 = pi->txpa_5g_hi[1];
			*a1 = pi->txpa_5g_hi[2];
			break;
#endif /* BAND5G */
		default:
			ASSERT(FALSE);
			break;
	}
	return;
}
#endif /* lcn40 || lcn */

/* --------------------------------------------- */
/* this will evetually be moved to lcncommon.c */
phy_info_lcnphy_t *
wlc_phy_getlcnphy_common(phy_info_t *pi)
{
	if (ISLCN40PHY(pi))
		return (phy_info_lcnphy_t *)pi->u.pi_lcn40phy;
	else {
		ASSERT(FALSE);
		return NULL;
	}
}

uint16
wlc_txpwrctrl_lcncommon(phy_info_t *pi)
{
	if (ISLCN40PHY(pi))
		return LCN40PHY_TX_PWR_CTRL_HW;
	else {
		ASSERT(FALSE);
		return 0;
	}
}

void
wlc_phy_get_SROMnoiselvl_phy(phy_info_t *pi, int8 *noiselvl)
{
	/* Returns noise level (read from srom) for current channel */
	uint8 core, core_freq_segment_map;
	uint16 channel;

	channel = CHSPEC_CHANNEL(pi->radio_chanspec);
	FOREACH_CORE(pi, core) {

		/* For 80P80, retrieve Primary/Secondary based on the mapping */
		if (CHSPEC_IS8080(pi->radio_chanspec)) {
			core_freq_segment_map = pi->u.pi_acphy->core_freq_mapping[core];
			if (PRIMARY_FREQ_SEGMENT == core_freq_segment_map)
				channel = wf_chspec_primary80_channel(pi->radio_chanspec);

			if (SECONDARY_FREQ_SEGMENT == core_freq_segment_map)
				channel = wf_chspec_secondary80_channel(pi->radio_chanspec);
		}

		if (channel <= 14) {
			/* 2G */
			noiselvl[core] = pi->noiselvl_2g[core];

		} else {
#ifdef BAND5G
			/* 5G */
			if (channel <= 48) {
				/* 5G-low: channels 36 through 48 */
				noiselvl[core] = pi->noiselvl_5gl[core];
			} else if (channel <= 64) {
				/* 5G-mid: channels 52 through 64 */
				noiselvl[core] = pi->noiselvl_5gm[core];
			} else if (channel <= 128) {
				/* 5G-high: channels 100 through 128 */
				noiselvl[core] = pi->noiselvl_5gh[core];
			} else {
				/* 5G-upper: channels 132 and above */
				noiselvl[core] = pi->noiselvl_5gu[core];
			}
#endif /* BAND5G */
		}
	}
}

#ifdef WLTEST
void
wlc_phy_pkteng_boostackpwr(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	if (ISLCN40PHY(pi) && (pi->boostackpwr == 1)) {
		pi->pi_fptr->settxpwrbyindex(pi, (int)0);
	}
}
#endif /* ifdef WLTEST */

#ifdef LP_P2P_SOFTAP
uint8
BCMATTACHFN(wlc_lcnphy_get_index)(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	if (ISLCN40PHY(pi)) {
		phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);
		int8 offset = pi_lcn->pwr_offset_val;
		uint8 i;

		if (offset) {
			/* Search for the offset */
			for (i = 0;
				(i < LCNPHY_TX_PWR_CTRL_MACLUT_LEN) && (-offset >= pwr_lvl_qdB[i]);
				i++);
			return --i;
		}
	}

	/* For other PHYs return 0: Nominal power */
	return 0;
}
#endif /* LP_P2P_SOFTAP */


#if defined(WLC_LOWPOWER_BEACON_MODE)
void
wlc_phy_lowpower_beacon_mode(wlc_phy_t *pih, int lowpower_beacon_mode)
{
	phy_info_t *pi = (phy_info_t *)pih;
	if (pi->pi_fptr->lowpowerbeaconmode)
		pi->pi_fptr->lowpowerbeaconmode(pi, lowpower_beacon_mode);
}
#endif /* WLC_LOWPOWER_BEACON_MODE */

/* block bbpll change if ILP cal is in progress */
void
wlc_phy_block_bbpll_change(wlc_phy_t *pih, bool block, bool going_down)
{
	phy_info_t *pi = (phy_info_t *)pih;

	if (block) {
		pi->block_for_slowcal = TRUE;
	} else {
		pi->block_for_slowcal = FALSE;

		if (pi->blocked_freq_for_slowcal && ISACPHY(pi) && !going_down) {
			phy_ac_set_spurmode(pi->u.pi_acphy->rxspuri,
				pi->blocked_freq_for_slowcal);
		} else if (pi->blocked_freq_for_slowcal && ISNPHY(pi) && !going_down) {
			wlc_phy_set_spurmode_nphy(pi, pi->blocked_freq_for_slowcal);
		}
	}

}


#ifdef WLTXPWR_CACHE
void wlc_phy_clear_tx_power_offset(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t*)ppi;

/*	if ((pi->tx_power_offset != NULL) &&
		(!wlc_phy_is_pwr_cached(pi->txpwr_cache, TXPWR_CACHE_POWER_OFFSETS,
		pi->tx_power_offset))) {
			ppr_delete(pi->sh->osh, pi->tx_power_offset);
	}
*/
	pi->tx_power_offset = NULL;
}
#endif /* WLTXPWR_CACHE */

#ifdef WLSRVSDB
#ifdef WFD_PHY_LL
int
wlc_phy_sr_vsdb_invalidate_active(wlc_phy_t * ppi)
{
	phy_info_t *pi = (phy_info_t*)ppi;

	if (pi->srvsdb_state->sr_vsdb_channels[0] ==
	    CHSPEC_CHANNEL(pi->srvsdb_state->prev_chanspec)) {
		pi->srvsdb_state->swbkp_snapshot_valid[0] = 0;
		return 0;
	}
	else if (pi->srvsdb_state->sr_vsdb_channels[1] ==
	         CHSPEC_CHANNEL(pi->srvsdb_state->prev_chanspec)) {
		pi->srvsdb_state->swbkp_snapshot_valid[1] = 0;
		return 1;
	}

	return -1;
}
#endif /* WFD_PHY_LL */

void
wlc_phy_sr_vsdb_reset(wlc_phy_t * ppi)
{
	phy_info_t *pi = (phy_info_t*)ppi;

	pi->srvsdb_state->swbkp_snapshot_valid[0] = 0;
	pi->srvsdb_state->swbkp_snapshot_valid[1] = 0;
	pi->srvsdb_state->sr_vsdb_bank_valid[0] = FALSE;
	pi->srvsdb_state->sr_vsdb_bank_valid[1] = FALSE;
	pi->srvsdb_state->prev_chanspec = CHANNEL_UNDEFINED;
	pi->srvsdb_state->vsdb_trig_cnt = 0;
}

/** reset SR VSDB hw */
void
wlc_phy_reset_srvsdb_engine(wlc_phy_t *ppi)
{
	uint origidx;
	chipcregs_t *cc;
	si_t *sih;
	phy_info_t *pi = (phy_info_t*)ppi;

	sih = (si_t*)pi->sh->sih;

	origidx = si_coreidx(sih);

	/* setcore to chipcmn */
	cc = si_setcoreidx(sih, SI_CC_IDX);

	/* Jira-SWWLAN-28550: A fix of Sr related changes from B0 to B1 created a problem for VSDB
	For VSDB, Bit 9 of PMU chip control 2 gates off the clock to the SR engine, in VSDB
	mode this Bit is always SET.
	Fix: RESET the Bit Before SRVSDB, and SET it back after SRVSDB, this is done presently
	only for 4324B4 chips
	*/
	if (CHIP_4324_B4(pi)) {
		uint32 temp, temp1;
		W_REG(si_osh(sih), &cc->chipcontrol_addr, 2);
		temp = R_REG(si_osh(sih), &cc->chipcontrol_data);
		temp1 = temp & 0xFFFFFDFF;
		W_REG(si_osh(sih), &cc->chipcontrol_data, temp1);
	}

	/* Reset 2nd bit */
	sr_chipcontrol(sih, 0x4, 0x0);
	/* Set 2nd bit */
	sr_chipcontrol(sih, 0x4, 0x4);

	/* Jira-SWWLAN-28550: A fix of Sr related changes from B0 to B1 created a problem for VSDB
	For VSDB, Bit 9 of PMU chip control 2 gates off the clock to the SR engine, in VSDB
	mode this Bit is always SET.
	Fix: RESET the Bit Before SRVSDB, and SET it back after SRVSDB, this is done presently
	only for 4324B4 chips
	*/
	if (CHIP_4324_B4(pi)) {
		uint32 temp, temp1;
		W_REG(si_osh(sih), &cc->chipcontrol_addr, 2);
		temp = R_REG(si_osh(sih), &cc->chipcontrol_data);
		temp1 = temp | 0x200;
		W_REG(si_osh(sih), &cc->chipcontrol_data, temp1);
	}

	/* Set core orig core */
	si_setcoreidx(sih, origidx);

}

void
wlc_phy_force_vsdb_chans(wlc_phy_t *ppi, uint16* vsdb_chans, uint8 set)
{
	uint16 chans[2];
	phy_info_t *pi = (phy_info_t*)ppi;

	if (set) {
		pi->srvsdb_state->force_vsdb = 1;

		bcopy(vsdb_chans, chans, 2*sizeof(uint16));

		wlc_phy_attach_srvsdb_module(ppi, chans[0], chans[1]);


		pi->srvsdb_state->prev_chanspec = chans[0];
		pi->srvsdb_state->force_vsdb_chans[0] = chans[0];
		pi->srvsdb_state->force_vsdb_chans[1] = chans[1];

	} else {
		wlc_phy_detach_srvsdb_module(ppi);

		/* Reset force vsdb chans */
		pi->srvsdb_state->force_vsdb_chans[0] = 0;
		pi->srvsdb_state->force_vsdb_chans[1] = 0;
		pi->srvsdb_state->force_vsdb = 0;
	}
}

void
wlc_phy_detach_srvsdb_module(wlc_phy_t *ppi)
{
	uint8 i;
	phy_info_t *pi = (phy_info_t*)ppi;

	/* Disable the flags */
	wlc_phy_sr_vsdb_reset(ppi);

	for (i = 0; i < SR_MEMORY_BANK; i++) {
		if (pi->vsdb_bkp[i] != NULL) {
			if (pi->vsdb_bkp[i]->pi_nphy != NULL) {
				phy_mfree(pi, pi->vsdb_bkp[i]->pi_nphy, sizeof(phy_info_nphy_t));
				pi->vsdb_bkp[i]->pi_nphy = NULL;
			}

			if (pi->vsdb_bkp[i]->tx_power_offset != NULL)
				ppr_delete(pi->sh->osh, pi->vsdb_bkp[i]->tx_power_offset);
			phy_mfree(pi, pi->vsdb_bkp[i], sizeof(vsdb_backup_t));
			pi->vsdb_bkp[i] = NULL;
			PHY_INFORM(("de allocate %d of mem \n", (sizeof(vsdb_backup_t) +
				sizeof(phy_info_nphy_t))));
		}
	}
	pi->srvsdb_state->sr_vsdb_channels[0] = 0;
	pi->srvsdb_state->sr_vsdb_channels[1] = 0;
	pi->srvsdb_state->srvsdb_active = 0;

	pi->srvsdb_state->acimode_noisemode_reset_done[0] = FALSE;
	pi->srvsdb_state->acimode_noisemode_reset_done[1] = FALSE;

	/* srvsdb switch status */
	pi->srvsdb_state->switch_successful = FALSE;

	/* Timers */
	bzero(pi->srvsdb_state->prev_timer, 2 * sizeof(uint32));
	bzero(pi->srvsdb_state->sum_delta_timer, 2 * sizeof(uint32));

	/* counter for no of switch iterations */
	bzero(pi->srvsdb_state->num_chan_switch, 2 * sizeof(uint8));

	/* crsglitch */
	bzero(pi->srvsdb_state->prev_crsglitch_cnt, 2 * sizeof(uint32));
	bzero(pi->srvsdb_state->sum_delta_crsglitch, 2 * sizeof(uint32));
	/* bphy_crsglitch */
	bzero(pi->srvsdb_state->prev_bphy_rxcrsglitch_cnt, 2 * sizeof(uint32));
	bzero(pi->srvsdb_state->sum_delta_bphy_crsglitch, 2 * sizeof(uint32));
	/* badplcp */
	bzero(pi->srvsdb_state->prev_badplcp_cnt, 2 * sizeof(uint32));
	bzero(pi->srvsdb_state->sum_delta_prev_badplcp, 2 * sizeof(uint32));
	/* bphy_badplcp */
	bzero(pi->srvsdb_state->prev_bphy_badplcp_cnt, 2 * sizeof(uint32));
	bzero(pi->srvsdb_state->sum_delta_prev_bphy_badplcp, 2 * sizeof(uint32));
}

/**
 * Despite the 'attach' in its name: is not meant to be called in the 'attach' phase.
 * Returns TRUE on success. Caller supplied arguments chan0 and chan1 may not reside in the same
 * band.
 */
uint8
wlc_phy_attach_srvsdb_module(wlc_phy_t *ppi, chanspec_t chan0, chanspec_t chan1)
{

	uint8 i;
	phy_info_t *pi = (phy_info_t*)ppi;

	/* Detach allready existing structire */
	wlc_phy_detach_srvsdb_module(ppi);

	/* reset srvsdb enigne */
	wlc_phy_reset_srvsdb_engine(ppi);

	/* Alloc mem for sw backup structures */
	for (i = 0; i < SR_MEMORY_BANK; i++) {
		pi->vsdb_bkp[i] = phy_malloc_fatal(pi, sizeof(vsdb_backup_t));
		pi->vsdb_bkp[i]->pi_nphy = phy_malloc_fatal(pi, sizeof(phy_info_nphy_t));
		PHY_INFORM(("allocate %d of mem \n", (sizeof(vsdb_backup_t) +
			sizeof(phy_info_nphy_t))));
	}


	pi->srvsdb_state->sr_vsdb_channels[0] = CHSPEC_CHANNEL(chan0);
	pi->srvsdb_state->sr_vsdb_channels[1] = CHSPEC_CHANNEL(chan1);
	pi->srvsdb_state->srvsdb_active = 1;

	return TRUE;
}
#endif /* WLSRVSDB */

/**
 * Reduce channel switch time by attempting to use hardware acceleration.
 */
uint8
wlc_set_chanspec_sr_vsdb(wlc_phy_t *ppi, chanspec_t chanspec, uint8 *last_chan_saved)
{
	uint8 switched = FALSE;

#ifdef WLSRVSDB
	phy_info_t *pi = (phy_info_t*)ppi;

	if (ISNPHY(pi)) {
		switched = wlc_set_chanspec_sr_vsdb_nphy(pi, chanspec, last_chan_saved);
	}
#endif /* WLSRVSDB */
	return switched;
}

void wlc_phy_clear_match_tx_offset(wlc_phy_t *ppi, ppr_t *pprptr)
{
	if (ppi != NULL) {
		phy_info_t *pi = (phy_info_t*)ppi;
		if (pprptr == pi->tx_power_offset)
			pi->tx_power_offset = NULL;
	}
}

#ifdef ATE_BUILD
void
wlc_ate_gpiosel_disable(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t*)ppi;

	if (ISACPHY(pi) && ACMAJORREV_3(pi->pubpi->phy_rev)) {
		phy_utils_phyreg_enter(pi);
		phy_utils_write_phyreg(pi, ACPHY_gpioLoOutEn(pi->pubpi->phy_rev), 0);
		phy_utils_write_phyreg(pi, ACPHY_gpioHiOutEn(pi->pubpi->phy_rev), 0);
		phy_utils_phyreg_exit(pi);
	} else {
		/* Not supported for other PHYs yet */
		ASSERT(FALSE);
	}
}
#endif /* ATE_BUILD */

#if defined(RXDESENS_EN)
/* Main wrapper for dynamic phy rxdesens */
void
wlc_phy_rxdesens(wlc_phy_t *ppi, bool densens_en)
{
	phy_info_t *pi = (phy_info_t *)ppi;
#ifdef WLSRVSDB
	bool invalidate_swbkp = FALSE;
#endif

	if (CHSPEC_IS5G(pi->radio_chanspec))
		return;

	if (pi->sromi->phyrxdesens == 0)
		return;

	if (densens_en) {
#ifndef WLC_DISABLE_ACI
		if (pi->sh->interference_mode != INTERFERE_NONE) {
			/* disable interference */
			phy_noise_set_mode(pi, INTERFERE_NONE, TRUE);
			pi->sromi->saved_interference_mode = pi->sh->interference_mode;
			pi->sh->interference_mode = INTERFERE_NONE;
#ifdef WLSRVSDB
			invalidate_swbkp = TRUE;
#endif
		}
#endif /* !defined(WLC_DISABLE_ACI) */
		/* apply densens */
		wlc_nphy_set_rxdesens((wlc_phy_t *)pi, pi->sromi->phyrxdesens);
		pi->u.pi_nphy->ntd_rxdesens_active = TRUE;
		PHY_INFORM(("%s: applied desens on Ch %d |"
			" phyreg 0x21 %x | 0x283: %x | 0x4aa: %x\n",
			__FUNCTION__, CHSPEC_CHANNEL(pi->radio_chanspec),
			phy_utils_read_phyreg(pi, 0x21), phy_utils_read_phyreg(pi, 0x283),
			phy_utils_read_phyreg(pi, 0x4aa)));
	} else {
		/* remove desens */
		wlc_nphy_set_rxdesens((wlc_phy_t *)pi, 0);
		pi->u.pi_nphy->ntd_rxdesens_active = FALSE;
		PHY_INFORM(("%s: removed desens on Ch %d |"
			" phyreg 0x21: %x | 0x283: %x | 0x4aa: %x\n",
			__FUNCTION__, CHSPEC_CHANNEL(pi->radio_chanspec),
			phy_utils_read_phyreg(pi, 0x21), phy_utils_read_phyreg(pi, 0x283),
			phy_utils_read_phyreg(pi, 0x4aa)));
#ifndef WLC_DISABLE_ACI
		/* enable interference */
		pi->sh->interference_mode = pi->sromi->saved_interference_mode;
		phy_noise_set_mode(pi, pi->sh->interference_mode, TRUE);
#ifdef WLSRVSDB
		invalidate_swbkp = TRUE;
#endif
#endif /* !defined(WLC_DISABLE_ACI) */
	}

#ifdef WLSRVSDB
	if (invalidate_swbkp) {
		/* invalidate swbkp to update per cfg phyrxdesens & interference_mode */
		if (pi->srvsdb_state->sr_vsdb_channels[0] ==
			CHSPEC_CHANNEL(pi->srvsdb_state->prev_chanspec)) {

			pi->srvsdb_state->swbkp_snapshot_valid[0] = 0;
			PHY_INFORM(("invalidating swbkp for Ch %d\n",
				CHSPEC_CHANNEL(pi->srvsdb_state->prev_chanspec)));

		} else if (pi->srvsdb_state->sr_vsdb_channels[1] ==
			CHSPEC_CHANNEL(pi->srvsdb_state->prev_chanspec)) {

			pi->srvsdb_state->swbkp_snapshot_valid[1] = 0;
			PHY_INFORM(("invalidating swbkp for Ch %d\n",
				CHSPEC_CHANNEL(pi->srvsdb_state->prev_chanspec)));
		}
	}
#endif /* defined(WLSRVSDB) */
}
#ifdef WLSRVSDB
/* Returns a flag to defer dynamic rxdesens if srvsdb engine
 * is not ready or SW backup is not taken.
 */
bool
wlc_phy_rxdesens_defer(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	int i = 0;
	bool defer = FALSE;

	for (i = 0; i < 2; i++) {
		if (pi->srvsdb_state->srvsdb_active &&
			!pi->srvsdb_state->swbkp_snapshot_valid[i]) {
			defer = TRUE;
			break;
		}
	}
	return defer;
}
#endif /* WLSRVSDB */
#endif /* RXDESENS_EN */

#if defined(WLMCHAN)
/* Returns a flag to reset aci and noise mitigation states (in vsdb mode) when back in home chanspec
 * to ensure corruption free sw backup and hardware state save/restore.
 */
bool
wlc_phy_acimode_noisemode_reset_allowed(wlc_phy_t *ppi, uint16 home_channel)
{
	bool allow = FALSE;
#if !defined(WLC_DISABLE_ACI) && defined(WLSRVSDB)
	phy_info_t *pi = (phy_info_t *)ppi;

	int i = 0;
	for (i = 0; i < 2; i++)
		if ((pi->srvsdb_state->sr_vsdb_channels[i] == home_channel) &&
			(!pi->srvsdb_state->swbkp_snapshot_valid[i]) &&
			(!pi->srvsdb_state->acimode_noisemode_reset_done[i])) {
			allow = TRUE;
			pi->srvsdb_state->acimode_noisemode_reset_done[i] = TRUE;
			break;
		}
#endif
	return allow;
}

#ifdef WLSRVSDB
bool
wlc_phy_srvsdb_switch_status(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	return pi->srvsdb_state->switch_successful;
}
#endif /* WLSRVSDB */
#endif /* defined(WLMCHAN) */

#ifdef WLC_TXDIVERSITY
/*
 * Returns Oclscd Enable status
 */
uint8
wlc_phy_get_oclscdenable_status(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t *)pih;
	return pi->nphy_oclscd;
}
#endif /* WLC_TXDIVERSITY */

void
wlc_phy_update_link_rssi(wlc_phy_t *pih, int8 rssi)
{
	phy_info_t *pi = (phy_info_t*)pih;

	pi->interf->link_rssi = rssi;
}

void
wlc_phy_watchdog_override(phy_info_t *pi, bool val)
{
	pi->phywatchdog_override = val;
}

#ifdef WLTXPWR_CACHE
tx_pwr_cache_entry_t* wlc_phy_get_txpwr_cache(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	return (tx_pwr_cache_entry_t*)pi->txpwr_cache;
}

#if !defined(WLTXPWR_CACHE_PHY_ONLY)

void wlc_phy_set_txpwr_cache(wlc_phy_t *ppi, tx_pwr_cache_entry_t* cacheptr)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	pi->txpwr_cache = cacheptr;
}

#endif

#endif	/* WLTXPWR_CACHE */

uint8
wlc_phy_get_txpwr_backoff(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	return pi->tx_pwr_backoff;

}

void
wlc_phy_get_txpwr_min(wlc_phy_t *ppi, uint8 *min_pwr)
{
#if defined(PHYCAL_CACHING)
	phy_info_t *pi = (phy_info_t *)ppi;

	if (ISACPHY(pi))
		*min_pwr = pi->min_txpower;
#endif /* PHYCAL_CACHING */

}

/*
 * if bfe capable then return max no. of streams that sta can receive in a VHT
 * NDP minus 1.
 */
uint8
wlc_phy_get_bfe_ndp_recvstreams(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;

	if (ISACPHY(pi)) {
		/* AC major 4, 32 and 40 can recv 3 */
		if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev) || ACMAJORREV_37(pi->pubpi->phy_rev) ||
			ACMAJORREV_40(pi->pubpi->phy_rev))
			return 3;
		else
			return 2;
	} else {
		ASSERT(0);
		return 0;
	}
}

uint32
wlc_phy_get_cal_dur(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t*)pih;
	return pi->cal_dur;
}

void
wlc_si_pmu_regcontrol_access(phy_info_t *pi, uint8 addr, uint32* val, bool write)
{
	si_t *sih;
	chipcregs_t *cc;
	uint origidx, intr_val;

	/* shared pi handler */
	sih = (si_t*)pi->sh->sih;

	/* Remember original core before switch to chipc */
	cc = (chipcregs_t *)si_switch_core(sih, CC_CORE_ID, &origidx, &intr_val);
	ASSERT(cc != NULL);

	if (write) {
		W_REG(si_osh(sih), &cc->regcontrol_addr, addr);
		W_REG(si_osh(sih), &cc->regcontrol_data, *val);
		/* read back to confirm */
		*val = R_REG(si_osh(sih), &cc->regcontrol_data);
	} else {
		W_REG(si_osh(sih), &cc->regcontrol_addr, addr);
		*val = R_REG(si_osh(sih), &cc->regcontrol_data);
	}

	/* Return to original core */
	si_restore_core(sih, origidx, intr_val);
}

void
wlc_si_pmu_chipcontrol_access(phy_info_t *pi, uint8 addr, uint32* val, bool write)
{
	si_t *sih;
	chipcregs_t *cc;
	uint origidx, intr_val;

	/* shared pi handler */
	sih = (si_t*)pi->sh->sih;

	/* Remember original core before switch to chipc */
	cc = (chipcregs_t *)si_switch_core(sih, CC_CORE_ID, &origidx, &intr_val);
	ASSERT(cc != NULL);

	if (write) {
		W_REG(si_osh(sih), &cc->chipcontrol_addr, addr);
		W_REG(si_osh(sih), &cc->chipcontrol_data, *val);
		/* read back to confirm */
		*val = R_REG(si_osh(sih), &cc->chipcontrol_data);
	} else {
		W_REG(si_osh(sih), &cc->chipcontrol_addr, addr);
		*val = R_REG(si_osh(sih), &cc->chipcontrol_data);
	}

	/* Return to original core */
	si_restore_core(sih, origidx, intr_val);
}

void wlc_phy_clean_noise_array(phy_info_t *pi)
{
	uint8 indx, i;

	if (ISACPHY(pi)) {
		FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, i) {
			for (indx = 0; indx < PHY_SIZE_NOISE_ARRAY; indx++) {
				pi->u.pi_acphy->phy_noise_pwr_array[indx][i] = 0;
			}
		}
		pi->u.pi_acphy->phy_noise_counter = 0;
	}
}

#ifdef OCL
void
wlc_phy_ocl_coremask_change(wlc_phy_t *ppi, uint8 coremask)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_info_acphy_t *pi_ac;

	ASSERT(pi != NULL);
	pi_ac = pi->u.pi_acphy;

	if (ISACPHY(pi)) {
		pi_ac->ocl_coremask = coremask;
		wlc_phy_ocl_enable(pi, FALSE);
		if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
			wlc_phy_ocl_apply_coremask_28nm(pi);
		} else {
			wlc_phy_ocl_apply_coremask_acphy(pi);
		}
		wlc_phy_ocl_enable(pi, !pi_ac->ocl_disable_reqs);
	}
}
uint8
wlc_phy_ocl_get_coremask(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_info_acphy_t *pi_ac;

	ASSERT(pi != NULL);
	pi_ac = pi->u.pi_acphy;

	if (ISACPHY(pi))
		return  pi_ac->ocl_coremask;
	else
		return 0;
}
void
wlc_phy_ocl_status_get(wlc_phy_t *ppi, uint16 *reqs, uint8 *coremask, bool *ocl_en)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_info_acphy_t *pi_ac;

	ASSERT(pi != NULL);
	pi_ac = pi->u.pi_acphy;

	if (ISACPHY(pi)) {
		if (reqs != NULL)
			*reqs = pi_ac->ocl_disable_reqs;
		if (coremask != NULL)
			*coremask = pi_ac->ocl_coremask;
		if (ocl_en != NULL && pi->sh->clk)
			*ocl_en = (READ_PHYREGFLD(pi, OCLControl1, ocl_mode_enable) == 1);
	}
}

void
wlc_phy_ocl_disable_req_set(wlc_phy_t *ppi, uint16 req, bool disable, uint8 req_id)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_info_acphy_t *pi_ac;
	uint16 disable_req_old;
	uint8 cm_hw = 0;
	bool ocl_en = FALSE;
	bool log = TRUE;

	ASSERT(pi != NULL);
	pi_ac = pi->u.pi_acphy;

	if (ISACPHY(pi)) {
		disable_req_old = pi_ac->ocl_disable_reqs;

		if (disable) {
			pi_ac->ocl_disable_reqs |= req;
		} else {
			pi_ac->ocl_disable_reqs &= ~req;
		}

		if (pi->sh->clk) {
			wlc_phy_ocl_enable_acphy(pi, !pi_ac->ocl_disable_reqs);
		}

#if defined(EVENT_LOG_COMPILE)
		if (!EVENT_LOG_IS_ON(EVENT_LOG_TAG_OCL_INFO)) {
			log = FALSE;
		}
#endif /* EVENT_LOG_COMPILE */

		if (log) {
			if (pi->sh->clk) {
				cm_hw = READ_PHYREGFLD(pi, OCLControl1, ocl_rx_core_mask);
				ocl_en = (READ_PHYREGFLD(pi, OCLControl1, ocl_mode_enable) == 1);
			}

			WLC_PHY_OCL_INFO(("Request from %d: disable new|old %08x "
			                  "chan|cm_sw_hw|ocl_clk %08x, tx|rx %08x\n",
			                  req_id,
			                  ((pi_ac->ocl_disable_reqs << 16) | disable_req_old),
			                  ((pi->radio_chanspec << 16)|
			                   ((pi_ac->ocl_coremask & 0x03) << 12) |
			                   ((cm_hw & 0x3) << 8) | (ocl_en << 4) | pi->sh->clk),
			                  ((pi->sh->hw_phytxchain << 24) |
			                   (pi->sh->hw_phyrxchain << 16) |
			                   (pi->sh->phytxchain << 8) | pi->sh->phyrxchain)));
		}
	}
}
#endif /* OCL */

void
wlc_phy_btc_mode_set(wlc_phy_t *ppi, int btc_mode)
{
	phy_info_t *pi = (phy_info_t*)ppi;

	BCM_REFERENCE(btc_mode);

	if (ISACPHY(pi)) {
		wlc_phy_btc_dyn_preempt(pi);
	}
}
