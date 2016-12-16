/*
 * HTPHY RadarDetect module implementation
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
 * $Id: phy_ht_radar.c 657811 2016-09-02 17:48:43Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_radar.h"
#include "phy_radar_shared.h"
#include <phy_ht.h>
#include <phy_ht_radar.h>

#include <phy_utils_reg.h>

#include <wlc_phyreg_ht.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_ht_radar_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_radar_info_t *ri;
};

/* local functions */
static int phy_ht_radar_init(phy_type_radar_ctx_t *ctx, bool on);
static void _phy_ht_radar_update(phy_type_radar_ctx_t *ctx);
static void phy_ht_radar_set_mode(phy_type_radar_ctx_t *ctx, phy_radar_detect_mode_t mode);
static uint8 phy_ht_radar_run(phy_type_radar_ctx_t *ctx,
	radar_detected_info_t *radar_detected, bool sec_pll, bool bw80_80_mode);
static int phy_ht_radar_set_thresholds(phy_type_radar_ctx_t *ctx, wl_radar_thr_t *thresholds);
static void phy_radar_init_st(phy_info_t *pi, phy_radar_st_t *st);


/* Register/unregister HTPHY specific implementation to common layer. */
phy_ht_radar_info_t *
BCMATTACHFN(phy_ht_radar_register_impl)(phy_info_t *pi, phy_ht_info_t *hti, phy_radar_info_t *ri)
{
	phy_ht_radar_info_t *info;
	phy_type_radar_fns_t fns;
	phy_radar_st_t *st;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_ht_radar_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_ht_radar_info_t));
	info->pi = pi;
	info->hti = hti;
	info->ri = ri;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.init = phy_ht_radar_init;
	fns.update = _phy_ht_radar_update;
	fns.mode = phy_ht_radar_set_mode;
	fns.run = phy_ht_radar_run;
	fns.set_thresholds = phy_ht_radar_set_thresholds;
	fns.ctx = info;

	if (phy_radar_register_impl(ri, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_radar_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* init radar states */
	st = phy_radar_get_st(ri);
	ASSERT(st != NULL);

	phy_radar_init_st(pi, st);

	return info;
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_ht_radar_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_radar_unregister_impl)(phy_ht_radar_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_radar_info_t *ri = info->ri;


	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_radar_unregister_impl(ri);

	phy_mfree(pi, info, sizeof(phy_ht_radar_info_t));
}

static void
_phy_ht_radar_init(phy_ht_radar_info_t *info, bool on)
{
	phy_radar_info_t *ri = info->ri;
	phy_info_t *pi = info->pi;
	phy_radar_st_t *st;

	PHY_TRACE(("%s: init %d\n", __FUNCTION__, on));

	st = phy_radar_get_st(ri);
	ASSERT(st != NULL);

	if (on) {
		if (CHSPEC_CHANNEL(pi->radio_chanspec) <= WL_THRESHOLD_LO_BAND) {
			if (CHSPEC_IS40(pi->radio_chanspec)) {
				st->rparams.radar_args.thresh0 =
					st->rparams.radar_thrs.thresh0_40_lo;
				st->rparams.radar_args.thresh1 =
					st->rparams.radar_thrs.thresh1_40_lo;
			} else {
				st->rparams.radar_args.thresh0 =
					st->rparams.radar_thrs.thresh0_20_lo;
				st->rparams.radar_args.thresh1 =
					st->rparams.radar_thrs.thresh1_20_lo;
			}
		} else {
			if (CHSPEC_IS40(pi->radio_chanspec)) {
				st->rparams.radar_args.thresh0 =
					st->rparams.radar_thrs.thresh0_40_hi;
				st->rparams.radar_args.thresh1 =
					st->rparams.radar_thrs.thresh1_40_hi;
			} else {
				st->rparams.radar_args.thresh0 =
					st->rparams.radar_thrs.thresh0_20_hi;
				st->rparams.radar_args.thresh1 =
					st->rparams.radar_thrs.thresh1_20_hi;
			}
		}
		phy_utils_write_phyreg(pi, HTPHY_RadarBlankCtrl,
			(st->rparams.radar_args.blank));

		phy_utils_write_phyreg(pi, HTPHY_RadarThresh0,
			(uint16)((int16)st->rparams.radar_args.thresh0));
		phy_utils_write_phyreg(pi, HTPHY_RadarThresh1,
			(uint16)((int16)st->rparams.radar_args.thresh1));
		phy_utils_write_phyreg(pi, HTPHY_Radar_t2_min, 0);

		phy_utils_write_phyreg(pi, HTPHY_StrAddress2u,
			st->rparams.radar_args.st_level_time);
		phy_utils_write_phyreg(pi, HTPHY_StrAddress2l,
			st->rparams.radar_args.st_level_time);
		phy_utils_write_phyreg(pi, HTPHY_FMDemodConfig,
			st->rparams.radar_args.fmdemodcfg);

		wlapi_bmac_write_shm(pi->sh->physhim,
			M_RADAR_REG, st->rparams.radar_args.thresh1);

		PHY_REG_LIST_START
			PHY_REG_WRITE_ENTRY(HTPHY, RadarThresh0R, 0x7a8)
			PHY_REG_WRITE_ENTRY(HTPHY, RadarThresh1R, 0x7d0)
		PHY_REG_LIST_EXECUTE(pi);

#ifdef NPHYREV7_HTPHY_DFS_WAR
		if (CHSPEC_IS20(pi->radio_chanspec)) {
			PHY_REG_LIST_START
				PHY_REG_WRITE_ENTRY(HTPHY, RadarMaLength, 0x08)
				PHY_REG_WRITE_ENTRY(HTPHY, RadarT3Timeout, 200)
				PHY_REG_WRITE_ENTRY(HTPHY, RadarResetBlankingDelay, 25)
			PHY_REG_LIST_EXECUTE(pi);
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			PHY_REG_LIST_START
				PHY_REG_WRITE_ENTRY(HTPHY, RadarMaLength, 0x10)
				PHY_REG_WRITE_ENTRY(HTPHY, RadarT3Timeout, 400)
				PHY_REG_WRITE_ENTRY(HTPHY, RadarResetBlankingDelay, 50)
			PHY_REG_LIST_EXECUTE(pi);
		}
#endif

		/* percal_mask to disable radar detection during selected period cals */
		pi->radar_percal_mask = st->rparams.radar_args.percal_mask;

		PHY_REG_LIST_START
			PHY_REG_WRITE_ENTRY(HTPHY, RadarSearchCtrl, 1)
#ifdef NPHYREV7_HTPHY_DFS_WAR
			PHY_REG_WRITE_ENTRY(HTPHY, RadarDetectConfig1, 0x3206)
#else
			PHY_REG_WRITE_ENTRY(HTPHY, RadarDetectConfig1, 0x3204)
#endif
			PHY_REG_WRITE_ENTRY(HTPHY, RadarT3BelowMin, 0)
		PHY_REG_LIST_EXECUTE(pi);
	}

	wlapi_bmac_mhf(pi->sh->physhim, MHF1, MHF1_RADARWAR, (on ? MHF1_RADARWAR : 0), FALSE);
}

static int
WLBANDINITFN(phy_ht_radar_init)(phy_type_radar_ctx_t *ctx, bool on)
{
	PHY_TRACE(("%s: init %d\n", __FUNCTION__, on));

	_phy_ht_radar_init((phy_ht_radar_info_t *)ctx, on);
	_phy_ht_radar_update(ctx);

	return BCME_OK;
}

static uint8
phy_ht_radar_run(phy_type_radar_ctx_t *ctx, radar_detected_info_t *radar_detected,
bool sec_pll, bool bw80_80_mode)
{
	phy_ht_radar_info_t *info = (phy_ht_radar_info_t *)ctx;
	phy_info_t *pi = info->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	return phy_radar_run_nphy(pi, radar_detected, sec_pll, bw80_80_mode);
}

static void
_phy_ht_radar_update(phy_type_radar_ctx_t *ctx)
{
	phy_ht_radar_info_t *info = (phy_ht_radar_info_t *)ctx;
	phy_radar_info_t *ri = info->ri;
	phy_info_t *pi = info->pi;
	phy_radar_st_t *st;
	uint16 st_level;

	PHY_TRACE(("%s\n", __FUNCTION__));

	st = phy_radar_get_st(ri);
	ASSERT(st != NULL);

	/* Set back the default st_level_time for non-RADAR
	 * 2G channels.
	 */
	st_level = 0x1591;

	/* For 5G, RADAR channels set the st_level_time based on
	 * FCC/EU modes. Change of st_level_time value based on FCC/EU
	 * is taken care in wlc_phy_radar_detect_mode_set().
	 */
	if (CHSPEC_IS5G(pi->radio_chanspec) &&
	    CHANNEL_ISRADAR(CHSPEC_CHANNEL(pi->radio_chanspec)))
		st_level = st->rparams.radar_args.st_level_time;

	phy_utils_write_phyreg(pi, HTPHY_StrAddress2u, st_level);
	phy_utils_write_phyreg(pi, HTPHY_StrAddress2l, st_level);
}

void
phy_ht_radar_upd(phy_ht_radar_info_t *ri)
{
	_phy_ht_radar_update(ri);
}

static const wl_radar_thr_t BCMATTACHDATA(wlc_phy_radar_thresh_htphy) = {
	WL_RADAR_THR_VERSION,
	0x6b8, 0x30, 0x6b8, 0x30, 0, 0, 0x6b8, 0x30, 0x6b8, 0x30, 0, 0,
	0, 0, 0, 0
};

static void
phy_radar_init_st(phy_info_t *pi, phy_radar_st_t *st)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
	BCM_REFERENCE(pi);

	/* 20Mhz channel radar thresholds */
	st->rparams.radar_thrs = wlc_phy_radar_thresh_htphy;

	/* 20Mhz channel radar params */
	st->rparams.min_deltat_lp = 19000; /* 1e-3*20e6 - small error */
	st->rparams.max_deltat_lp = 130000;  /* 2*2e-3*20e6 + small error */
	st->rparams.radar_args.nskip_rst_lp = 2;
	st->rparams.radar_args.min_burst_intv_lp = 20000000;
	st->rparams.radar_args.max_burst_intv_lp = 70000000;
#ifdef BIN5_RADAR_DETECT_WAR
	st->rparams.radar_args.nskip_rst_lp = 3;
#endif
#ifdef BIN5_RADAR_DETECT_WAR_J28
	st->rparams.radar_args.nskip_rst_lp = 3;
#endif
	st->rparams.radar_args.quant = 128;
	st->rparams.radar_args.npulses = 5;
	st->rparams.radar_args.ncontig = 37411;

	/* [100 011 1000 100011]=[1000 1110 0010 0011]=0x8e23 = 36387
	 * bits 15-13: JP2_1, JP4 npulses = 4
	 * bits 12-10: JP1_2_JP2_3 npulses = 3
	 * bits 9-6: EU-t4 fm tol = 8, (8/16)
	 * bit 5-0: max detection index = 35
	 * [100 100 1000 100011]=[1001 0010 0010 0011]=0x9223 = 37411
	 * bits 15-13: JP2_1, JP4 npulses = 4
	 * bits 12-10: JP1_2_JP2_3 npulses = 4
	 * bits 9-6: EU-t4 fm tol = 8, (8/16)
	 * bit 5-0: max detection index = 35
	 * [101 100 1000 110000]=[1011 0010 0011 0000]=0xb230 = 45596
	 * bits 15-13: JP2_1, JP4 npulses = 5
	 * bits 12-10: FCC_1, JP1_2_JP2_3 npulses = 4
	 * bits 9-6: EU-t4 fm tol = 8, (8/16)
	 * bit 5-0: max detection index = 48
	 */

	st->rparams.radar_args.max_pw = 690;  /* 30us + 15% */
	st->rparams.radar_args.thresh0 = st->rparams.radar_thrs.thresh0_20_lo;
	st->rparams.radar_args.thresh1 = st->rparams.radar_thrs.thresh1_20_lo;
	st->rparams.radar_args.fmdemodcfg = 0x7f09;
	st->rparams.radar_args.autocorr = 0x1e;
	st->rparams.radar_args.st_level_time = 0x1591;
	st->rparams.radar_args.min_pw = 6;
	st->rparams.radar_args.max_pw_tol = 12;
#ifdef NPHYREV7_HTPHY_DFS_WAR
	st->rparams.radar_args.npulses_lp = 9;
#else
	st->rparams.radar_args.npulses_lp = 11;
#endif
	st->rparams.radar_args.t2_min = 31552;	/* 0x7b40 */
#ifdef BIN5_RADAR_DETECT_WAR
	st->rparams.radar_args.npulses_lp = 6;
	st->rparams.radar_args.t2_min = 31488;
#endif
#ifdef BIN5_RADAR_DETECT_WAR_J28
	st->rparams.radar_args.npulses_lp = 8;
	st->rparams.radar_args..st_level_time = 0x0190;
#endif
	/* t2_min[15:12] = x; if n_non_single >= x && lp_length >
	 * npulses_lp => bin5 detected
	 * t2_min[11:10] = # times combining adjacent pulses < min_pw_lp
	 * t2_min[9] = fm_tol enable
	 * t2_min[8] = skip_type 5 enable
	 * t2_min[7:4] = y; bin5 remove pw <= 10*y
	 * t2_min[3:0] = t; non-bin5 remove pw <= 5*y
	 * st_level_time[11:0] =  pw criterion for short pluse noise filter
	 * st_level_time[15:12] =  2^x-1 as FMOFFSET
	 */
	st->rparams.radar_args.min_pw_lp = 700;
#ifdef BIN5_RADAR_DETECT_WAR
	st->rparams.radar_args.min_pw_lp = 50;
#endif
	st->rparams.radar_args.max_pw_lp = 2000;
#ifdef NPHYREV7_HTPHY_DFS_WAR
	st->rparams.radar_args.min_fm_lp = 25;
#else
	st->rparams.radar_args.min_fm_lp = 45;
#endif

#ifdef BIN5_RADAR_DETECT_WAR_J28
	st->rparams.radar_args.min_fm_lp  = 20;
#endif
	st->rparams.radar_args.max_span_lp = 63568;   /* 0xf850; 15, 8, 80 */
	/* max_span_lp[15:12] = skip_tot max */
	/* max_span_lp[11:8] = x, x/16 = % alowed fm tollerance bin5 */
	/* max_span_lp[7:0] = alowed pw tollerance bin5 */

	st->rparams.radar_args.min_deltat = 2000;
	st->rparams.radar_args.version = WL_RADAR_ARGS_VERSION;

	st->rparams.radar_args.fra_pulse_err = 4098; /* 0x1002, */
		/* bits 15-8: EU-t4 min_fm = 16 */
		/* bits 7-0: time from last det = 2 minute */

	/* 0x8444, bits 15:14 low_intv_eu_t2 */
	/* bits 13:12 low_intv_eu_t1; npulse -- bit 11:8 for EU type 4, */
	/* bits 7:4 = 4 for EU type 2, bits 3:0= 4 for EU type 1 */
	/* 11 11 0100 0100 0100 */
	st->rparams.radar_args.npulses_fra = 33860;
	st->rparams.radar_args.percal_mask = 0x31;
	st->rparams.radar_args.feature_mask = RADAR_FEATURE_USE_MAX_PW | RADAR_FEATURE_FCC_DETECT;
#ifdef NPHYREV7_HTPHY_DFS_WAR
	st->rparams.radar_args.blank = 0x2c19;
#else
	st->rparams.radar_args.blank = 0x6419;
#endif
}

static void
phy_ht_radar_set_mode(phy_type_radar_ctx_t *ctx, phy_radar_detect_mode_t mode)
{
	phy_ht_radar_info_t *info = (phy_ht_radar_info_t *)ctx;
	phy_radar_info_t *ri = info->ri;
	phy_radar_st_t *st;

	PHY_TRACE(("%s: mode %d\n", __FUNCTION__, mode));

	st = phy_radar_get_st(ri);
	ASSERT(st != NULL);

	/* Change radar params based on radar detect mode for
	 * both 20Mhz (index 0) and 40Mhz (index 1) aptly
	 * feature_mask bit-11 is FCC-enable
	 * feature_mask bit-12 is EU-enable
	 */
	if (mode == RADAR_DETECT_MODE_FCC) {
		st->rparams.radar_args.st_level_time = 0x1591;
	} else if (mode == RADAR_DETECT_MODE_EU) {
		st->rparams.radar_args.st_level_time = 0x1591;
	}
}

static int
phy_ht_radar_set_thresholds(phy_type_radar_ctx_t *ctx, wl_radar_thr_t *thresholds)
{
	phy_ht_radar_info_t *radari = (phy_ht_radar_info_t *)ctx;
	phy_radar_st_t *st = phy_radar_get_st(radari->ri);
	st->rparams.radar_thrs.thresh0_40_lo = thresholds->thresh0_40_lo;
	st->rparams.radar_thrs.thresh1_40_lo = thresholds->thresh1_40_lo;
	st->rparams.radar_thrs.thresh0_40_hi = thresholds->thresh0_40_hi;
	st->rparams.radar_thrs.thresh1_40_hi = thresholds->thresh1_40_hi;
	return BCME_OK;
}
