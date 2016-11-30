/*
 * ACPHY RadarDetect module implementation
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
 * $Id: phy_ac_radar.c 619072 2016-02-14 23:04:06Z chihap $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_radar.h"
#include "phy_radar_shared.h"
#include <phy_ac.h>
#include <phy_ac_radar.h>

#include <phy_utils_reg.h>

#include <wlc_phyreg_ac.h>
#include <phy_ac_info.h>

/* module private states */
struct phy_ac_radar_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_radar_info_t *ri;
};

/* local functions */
static int phy_ac_radar_init(phy_type_radar_ctx_t *ctx, bool on);
static uint8 phy_ac_radar_run(phy_type_radar_ctx_t *ctx, radar_detected_info_t *radar_detected);
static void phy_radar_init_st(phy_info_t *pi, phy_radar_st_t *st);

/* Register/unregister ACPHY specific implementation to common layer. */
phy_ac_radar_info_t *
BCMATTACHFN(phy_ac_radar_register_impl)(phy_info_t *pi, phy_ac_info_t *aci, phy_radar_info_t *ri)
{
	phy_ac_radar_info_t *info;
	phy_type_radar_fns_t fns;
	phy_radar_st_t *st;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_ac_radar_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	info->pi = pi;
	info->aci = aci;
	info->ri = ri;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.init = phy_ac_radar_init;
	fns.run = phy_ac_radar_run;
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
		phy_mfree(pi, info, sizeof(phy_ac_radar_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_radar_unregister_impl)(phy_ac_radar_info_t *info)
{
	phy_info_t *pi;
	phy_radar_info_t *ri;

	ASSERT(info);
	pi = info->pi;
	ri = info->ri;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_radar_unregister_impl(ri);

	phy_mfree(pi, info, sizeof(phy_ac_radar_info_t));
}

static const wl_radar_thr_t BCMATTACHDATA(wlc_phy_radar_thresh_acphy_2cores) = {
	WL_RADAR_THR_VERSION,
	0x6a8, 0x30, 0x6a8, 0x30, 0x6a8, 0x30, 0x6a8, 0x30, 0x6a8, 0x30, 0x6a8, 0x30
};

static const wl_radar_thr_t BCMATTACHDATA(wlc_phy_radar_thresh_acphy_1core) = {
	WL_RADAR_THR_VERSION,
	0x6a8, 0x30, 0x6a8, 0x30, 0x6a8, 0x30, 0x6a8, 0x30, 0x6a8, 0x30, 0x6a8, 0x30
};

static const wl_radar_thr_t BCMATTACHDATA(wlc_phy_radar_thresh_acphy_1core_4350) = {
	WL_RADAR_THR_VERSION,
	0x6a8, 0x30, 0x6a4, 0x30, 0x6a0, 0x30, 0x6ac, 0x30, 0x6a8, 0x30, 0x6a8, 0x30
	/* Assume 3dB antenna gain, targeting -64dBm at input of antenna port */
};

static const wl_radar_thr_t BCMATTACHDATA(wlc_phy_radar_thresh_acphy_43602) = {
	WL_RADAR_THR_VERSION,
	0x698, 0x18, 0x698, 0x18, 0x698, 0x18, 0x698, 0x18, 0x698, 0x18, 0x698, 0x18
};

static void
BCMATTACHFN(phy_radar_init_st)(phy_info_t *pi, phy_radar_st_t *st)
{
	PHY_TRACE(("%s\n", __FUNCTION__));

	/* 20Mhz channel radar thresholds */
	st->rparams.radar_thrs = (GET_RDR_NANTENNAS(pi) == 1)
							? wlc_phy_radar_thresh_acphy_1core
							: wlc_phy_radar_thresh_acphy_2cores;
	if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
		st->rparams.radar_thrs = wlc_phy_radar_thresh_acphy_1core_4350;
	} else if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
		st->rparams.radar_thrs = wlc_phy_radar_thresh_acphy_43602;
	}

	/* 20Mhz channel radar params */
	st->rparams.min_deltat_lp = 19000;	/* 1e-3*20e6 - small error	*/
	st->rparams.max_deltat_lp = 90000;	/* 2*2e-3*20e6 + small error	*/
	st->rparams.radar_args.nskip_rst_lp = 2;
	st->rparams.radar_args.min_burst_intv_lp = 12000000;
	st->rparams.radar_args.max_burst_intv_lp = 50000000;
	st->rparams.radar_args.quant = 16;
	st->rparams.radar_args.npulses = 7;
	st->rparams.radar_args.ncontig = 54832; /* 0xd630; */

	/* [100 100 1000 100011]=[1001 0010 0010 0011]=0x9223 = 37411
	 * bits 15-13: JP2_1, JP4 npulses = 4
	 * bits 12-10: JP1_2_JP2_3 npulses = 4
	 * bits 9-6: EU-t4 fm tol = 8, (8/16)
	 * bit 5-0: max detection index = 35
	 */

	st->rparams.radar_args.max_pw = 690;  /* 30us + 15% */
	st->rparams.radar_args.thresh0 = st->rparams.radar_thrs.thresh0_20_lo;
	st->rparams.radar_args.thresh1 = st->rparams.radar_thrs.thresh1_20_lo;
	st->rparams.radar_args.fmdemodcfg = 0x7f09;
	st->rparams.radar_args.autocorr = 0x1e;
	/* it is used to check pw for acphy. if pw > 20.1us, then check fm */
	st->rparams.radar_args.st_level_time = 0x8192;
	st->rparams.radar_args.min_pw = 0;
	st->rparams.radar_args.max_pw_tol = 12;
	st->rparams.radar_args.npulses_lp = 10;
	st->rparams.radar_args.t2_min = 30528;	/* 0x7740 */
#ifdef BIN5_RADAR_DETECT_WAR
	st->rparams.radar_args.npulses_lp = 6;
	st->rparams.radar_args.t2_min = 31488;
#endif
#ifdef BIN5_RADAR_DETECT_WAR_J28
	st->rparams.radar_args.npulses_lp = 8;
	st->rparams.radar_args.st_level_time = 0x0192;
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
	if (TONEDETECTION)
		st->rparams.radar_args.min_fm_lp = 500 - 256;
	else
#ifdef NPHYREV7_HTPHY_DFS_WAR
		st->rparams.radar_args.min_fm_lp = 25;
#else
		st->rparams.radar_args.min_fm_lp = 45;
#endif

#ifdef BIN5_RADAR_DETECT_WAR_J28
	st->rparams.radar_args.min_fm_lp  = 20;
#endif

	if (ISACPHY(pi) && TONEDETECTION)
		st->rparams.radar_args.max_span_lp = 61708;  /* 0xf20c; 15, 1, 12 */
	else
		st->rparams.radar_args.max_span_lp = 62476;  /* 0xf40c; 15, 4, 12 */
	/* max_span_lp[15:12] = skip_tot max */
	/* max_span_lp[11:8] = x, x/16 = % alowed fm tollerance bin5 */
	/* max_span_lp[7:0] = alowed pw tollerance bin5 */

	st->rparams.radar_args.min_deltat = 2000;
	st->rparams.radar_args.version = WL_RADAR_ARGS_VERSION;
	if (TONEDETECTION) {
		st->rparams.radar_args.fra_pulse_err = 65282; /* 0x1002, */
		/* bits 15-8: EU-t4 min_fm = 255 */
		/* bits 7-0: time from last det = 2 minute */
	} else {
		st->rparams.radar_args.fra_pulse_err = 4098; /* 0x1002, */
		/* bits 15-8: EU-t4 min_fm = 16 */
		/* bits 7-0: time from last det = 2 minute */
	}

	/* 0x8444, bits 15:14 low_intv_eu_t2 */
	/* bits 13:12 low_intv_eu_t1; npulse -- bit 11:8 for EU type 4, */
	/* bits 7:4 = 4 for EU type 2, bits 3:0= 4 for EU type 1 */
	/* 10 00 1100 1100 1100 */
	st->rparams.radar_args.npulses_fra = 34406;  /* 0x8666, bits 15:14 low_intv_eu_t2 */
	st->rparams.radar_args.npulses_stg2 = 7;
	st->rparams.radar_args.npulses_stg3 = 7;
	st->rparams.radar_args.percal_mask = 0x31;
	st->rparams.radar_args.feature_mask = RADAR_FEATURE_USE_MAX_PW | RADAR_FEATURE_FCC_DETECT;

	/* RadarBlankCtrl has been hard coded to 0x2c19/0x2c32/0x2464 for acphy in init */
	/* bits 15:8: remove bin5 pulses < this value in combined pusles from 2 antenna */
	/* bits 7:0: remove short pulses < this value in combined pusles from 2 antenna */
	st->rparams.radar_args.blank = 0x7f14;
}


static int
WLBANDINITFN(phy_ac_radar_init)(phy_type_radar_ctx_t *ctx, bool on)
{
	phy_ac_radar_info_t *info = (phy_ac_radar_info_t *)ctx;
	phy_radar_info_t *ri = info->ri;
	phy_info_t *pi = info->pi;
	phy_radar_st_t *st;

	PHY_TRACE(("%s: init %d\n", __FUNCTION__, on));

	st = phy_radar_get_st(ri);
	ASSERT(st != NULL);

	if (on) {
		if (CHSPEC_CHANNEL(pi->radio_chanspec) <= WL_THRESHOLD_LO_BAND) {
			if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
				st->rparams.radar_args.thresh0 =
					st->rparams.radar_thrs.thresh0_20_lo;
				st->rparams.radar_args.thresh1 =
					st->rparams.radar_thrs.thresh1_20_lo;
				ACPHY_REG_LIST_START
					WRITE_PHYREG_ENTRY(pi, RadarMaLength, 0x08)
					WRITE_PHYREG_ENTRY(pi, RadarT3BelowMin, 0)
					WRITE_PHYREG_ENTRY(pi, RadarT3Timeout, 200)
					WRITE_PHYREG_ENTRY(pi, RadarResetBlankingDelay, 25)
					WRITE_PHYREG_ENTRY(pi, RadarBlankCtrl, 0xac19)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				st->rparams.radar_args.thresh0 =
					st->rparams.radar_thrs.thresh0_40_lo;
				st->rparams.radar_args.thresh1 =
					st->rparams.radar_thrs.thresh1_40_lo;
				ACPHY_REG_LIST_START
					WRITE_PHYREG_ENTRY(pi, RadarMaLength, 0x10)
					WRITE_PHYREG_ENTRY(pi, RadarT3BelowMin, 0)
					WRITE_PHYREG_ENTRY(pi, RadarT3Timeout, 400)
					WRITE_PHYREG_ENTRY(pi, RadarResetBlankingDelay, 50)
					WRITE_PHYREG_ENTRY(pi, RadarBlankCtrl, 0xac32)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else {
				st->rparams.radar_args.thresh0 =
					st->rparams.radar_thrs.thresh0_80_lo;
				st->rparams.radar_args.thresh1 =
					st->rparams.radar_thrs.thresh1_80_lo;
				ACPHY_REG_LIST_START
					WRITE_PHYREG_ENTRY(pi, RadarMaLength, 0x1f)
					WRITE_PHYREG_ENTRY(pi, RadarT3BelowMin, 0)
					WRITE_PHYREG_ENTRY(pi, RadarT3Timeout, 800)
					WRITE_PHYREG_ENTRY(pi, RadarResetBlankingDelay, 100)
					WRITE_PHYREG_ENTRY(pi, RadarBlankCtrl, 0xac64)
				ACPHY_REG_LIST_EXECUTE(pi);
			}
		} else {
			if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
				st->rparams.radar_args.thresh0 =
					st->rparams.radar_thrs.thresh0_20_hi;
				st->rparams.radar_args.thresh1 =
					st->rparams.radar_thrs.thresh1_20_hi;
				ACPHY_REG_LIST_START
					WRITE_PHYREG_ENTRY(pi, RadarMaLength, 0x08)
					WRITE_PHYREG_ENTRY(pi, RadarT3BelowMin, 0)
					WRITE_PHYREG_ENTRY(pi, RadarT3Timeout, 200)
					WRITE_PHYREG_ENTRY(pi, RadarResetBlankingDelay, 25)
					WRITE_PHYREG_ENTRY(pi, RadarBlankCtrl, 0xac19)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				st->rparams.radar_args.thresh0 =
					st->rparams.radar_thrs.thresh0_40_hi;
				st->rparams.radar_args.thresh1 =
					st->rparams.radar_thrs.thresh1_40_hi;
				ACPHY_REG_LIST_START
					WRITE_PHYREG_ENTRY(pi, RadarMaLength, 0x10)
					WRITE_PHYREG_ENTRY(pi, RadarT3BelowMin, 0)
					WRITE_PHYREG_ENTRY(pi, RadarT3Timeout, 400)
					WRITE_PHYREG_ENTRY(pi, RadarResetBlankingDelay, 50)
					WRITE_PHYREG_ENTRY(pi, RadarBlankCtrl, 0xac32)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else {
				st->rparams.radar_args.thresh0 =
					st->rparams.radar_thrs.thresh0_80_hi;
				st->rparams.radar_args.thresh1 =
					st->rparams.radar_thrs.thresh1_80_hi;
				ACPHY_REG_LIST_START
					WRITE_PHYREG_ENTRY(pi, RadarMaLength, 0x1f)
					WRITE_PHYREG_ENTRY(pi, RadarT3BelowMin, 0)
					WRITE_PHYREG_ENTRY(pi, RadarT3Timeout, 800)
					WRITE_PHYREG_ENTRY(pi, RadarResetBlankingDelay, 100)
					WRITE_PHYREG_ENTRY(pi, RadarBlankCtrl, 0xac64)
				ACPHY_REG_LIST_EXECUTE(pi);
			}
		}
		/* phy_utils_write_phyreg(pi, ACPHY_RadarBlankCtrl, */
		/*   (st->rparams.radar_args.blank | (0x0000))); */
		phy_utils_write_phyreg(pi, ACPHY_RadarThresh0(pi->pubpi->phy_rev),
			(uint16)((int16)st->rparams.radar_args.thresh0));
		phy_utils_write_phyreg(pi, ACPHY_RadarThresh1(pi->pubpi->phy_rev),
			(uint16)((int16)st->rparams.radar_args.thresh1));
		phy_utils_write_phyreg(pi, ACPHY_Radar_t2_min(pi->pubpi->phy_rev), 0);
		phy_utils_write_phyreg(pi, ACPHY_crsControlu(pi->pubpi->phy_rev),
			(uint8)st->rparams.radar_args.autocorr);
		phy_utils_write_phyreg(pi, ACPHY_crsControll(pi->pubpi->phy_rev),
			(uint8)st->rparams.radar_args.autocorr);
		phy_utils_write_phyreg(pi, ACPHY_FMDemodConfig(pi->pubpi->phy_rev),
			st->rparams.radar_args.fmdemodcfg);
		phy_utils_write_phyreg(pi, ACPHY_RadarBlankCtrl2(pi->pubpi->phy_rev), 0x5f88);

		wlapi_bmac_write_shm(pi->sh->physhim,
			M_RADAR_REG, st->rparams.radar_args.thresh1);

		/* percal_mask to disable radar detection during selected period cals */
		pi->radar_percal_mask = st->rparams.radar_args.percal_mask;

		if (TONEDETECTION) {
			ACPHY_REG_LIST_START
				WRITE_PHYREG_ENTRY(pi, RadarSearchCtrl, 1)
				WRITE_PHYREG_ENTRY(pi, RadarDetectConfig1, 0x3206)
				/* enable new fm */
				WRITE_PHYREG_ENTRY(pi, RadarDetectConfig2, 0x141)
				/* WRITE_PHYREG_ENTRY(pi, RadarT3BelowMin, 0) */
			ACPHY_REG_LIST_EXECUTE(pi);
		}
		else {
			ACPHY_REG_LIST_START
				WRITE_PHYREG_ENTRY(pi, RadarSearchCtrl, 1)
				WRITE_PHYREG_ENTRY(pi, RadarDetectConfig1, 0x3206)
				/* enable new fm */
				WRITE_PHYREG_ENTRY(pi, RadarDetectConfig2, 0x40)
				/* WRITE_PHYREG_ENTRY(pi, RadarT3BelowMin, 0) */
			ACPHY_REG_LIST_EXECUTE(pi);
		}
	}

	if (TINY_RADIO(pi)) {
		phy_utils_write_phyreg(pi, ACPHY_Radar_adc_to_dbm(pi->pubpi->phy_rev),
			(ROUTER_4349(pi) && CHSPEC_CHANNEL(pi->radio_chanspec) <=
			WL_THRESHOLD_LO_BAND) ? 0x4a4 : 0x4ac);
		phy_utils_write_phyreg(pi, ACPHY_RadarDetectConfig1(pi->pubpi->phy_rev), 0x2c06);
	}

	wlapi_bmac_mhf(pi->sh->physhim, MHF1, MHF1_RADARWAR, (on ? MHF1_RADARWAR : 0), FALSE);

	return BCME_OK;
}

static uint8
phy_ac_radar_run(phy_type_radar_ctx_t *ctx, radar_detected_info_t *radar_detected)
{
	phy_ac_radar_info_t *info = (phy_ac_radar_info_t *)ctx;
	phy_info_t *pi = info->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	return phy_radar_run_nphy(pi, radar_detected);
}
