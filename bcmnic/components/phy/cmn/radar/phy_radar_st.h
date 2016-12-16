/*
 * RadarDetect module internal interface.
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
 * $Id: phy_radar_st.h 656353 2016-08-26 06:50:51Z $
 */

#ifndef _phy_radar_st_
#define _phy_radar_st_

#include <wlioctl.h>
#include <phy_radar_api.h>
#include <phy_radar.h>

/* PLEASE UPDATE THE FOLLOWING REVISION HISTORY AND VALUES OF #DEFINE'S */
/* DFS_SW_VERSION, DFS_SW_SUB_VERSION, DFS_SW_DATE_MONTH, AND */
/* DFS_SW_YEAR EACH TIME THE DFS CODE IS CHANGED. */
/* NO NEED TO CHANGE SW VERSION FOR RADAR THRESHOLD CHANGES */
/* Revision history: */
/* ver 2.001, 0612 2011: Overhauled short pulse detection to address EU types 1, 2, 4 */
/*     detection with traffic. Also increased npulses_stg2 to 5 */
/*     Tested on 4331 to pass EU and FCC radars. No false detection in 24 hours */
/* ver 2.002, 0712 2011: renamed functions wlc_phy_radar_detect_uniform_pw_check(..) */
/*     and wlc_phy_radar_detect_pri_pw_filter(..) from previous names to */
/*     better reflect functionality. Change npulses_fra to 33860,  */
/*     making the effective npulses of EU type 4 from 3 to 4 */
/* ver 2.003, 0912 2011: changed min_fm_lp of 20MHz from 45 to 25 */
/*     added Japan types 1_2, 2_3, 2_1, 4 detections. Modified radar types */
/*     in wlc_phy_shim.h and wlc_ap.c */
/* ver 3.000, 0911 2013: update to improve false detection as in acphy */
/*     This include ucode to disable radar during Rx after RXstart, and */
/*     disable radar during Tx. Enable radar smoothing and turn off Reset */
/*     blanking (to not blank the smoothed sample). Set appropriate smoothing */
/*     lengths. Changed min_fm_lp to 25, npulses_lp to 9 for nphy and htphy. */
/* ver 3.001, 1020 2015: extensive change to reduce false detection */
/*     see RB http://wlan-rb.sj.broadcom.com/r/73810/ */
#define DFS_SW_VERSION	3
#define DFS_SW_SUB_VERSION	1
#define DFS_SW_DATE_MONTH	1020
#define DFS_SW_YEAR	2015

#define MAX_FIFO_SIZE	512

#define RDR_LIST_SIZE (MAX_FIFO_SIZE / 4 + 2)	/* 4 words per pulse */

#ifdef BCMPHYCORENUM
#  if BCMPHYCORENUM > 1
#    define RDR_NANTENNAS 2
#  else /* BCMPHYCORENUM == 1 */
#    define RDR_NANTENNAS 1
#  endif /* BCMPHYCORENUM > 1 */
#  define GET_RDR_NANTENNAS(pi) (BCM4350_CHIP((pi)->sh->chip) ? 1 : RDR_NANTENNAS)
#else /* !BCMPHYCORENUM */
#  define RDR_NANTENNAS 2
#  define GET_RDR_NANTENNAS(pi) ((((pi)->pubpi->phy_corenum > 1) && \
				  (!BCM4350_CHIP((pi)->sh->chip))) ? 2 : 1)
#endif /* BCMPHYCORENUM */

#define RDR_LP_BUFFER_SIZE 64
#define LP_LEN_HIS_SIZE 10
#define LP_BUFFER_SIZE 64
#define MAX_LP_BUFFER_SPAN_20MHZ 240000000
#define MAX_LP_BUFFER_SPAN_40MHZ 480000000
#define RDR_SDEPTH_EXTRA_PULSES 1
#define TONEDETECTION 1
#define LPQUANT 128

typedef struct {
	wl_radar_args_t radar_args;	/* radar detection parametners */
	wl_radar_thr_t radar_thrs;	/* radar thresholds */
	wl_radar_thr2_t radar_thrs2;	/* radar thresholds for subband and 3+1 */
	int min_deltat_lp;
	int max_deltat_lp;
} radar_params_t;

typedef struct {
	uint32 interval;	/* tstamp or interval (in 1/20 us) */
	uint16 pw;		/* pulse-width in 1/20 us */
	int16 fm;		/* autocorrelation */
	int16 fc;
	uint16 chirp;
	uint16 notradar;
} pulse_data_t;

typedef struct {
	/* pulses (in tstamps) read from PHY FIFO */
	uint16 length_input[RDR_NANTENNAS];
	pulse_data_t pulses_input[RDR_NANTENNAS][RDR_LIST_SIZE];

	/* pulses (in tstamps) used for checking FCC 5 Radar match */
	uint16 length_bin5[RDR_NANTENNAS];
	pulse_data_t pulses_bin5[RDR_NANTENNAS][RDR_LIST_SIZE];

	/* pulses (in intervals) to process for checking short-pulse Radar match */
	uint16 length;
	pulse_data_t pulses[RDR_LIST_SIZE];
} radar_work_t;

typedef struct {
	/* various fields needed for checking FCC 5 long-pulse Radar */
	uint8 lp_length;
	uint32 lp_buffer[RDR_LP_BUFFER_SIZE];
	uint32 last_tstart;
	uint8 lp_cnt;
	uint8 lp_skip_cnt;
	int lp_pw_fm_matched;
	uint16 lp_pw[3];
	int16 lp_fm[3];
	int lp_n_non_single_pulses;
	bool lp_just_skipped;
	uint16 lp_skipped_pw;
	int16 lp_skipped_fm;
	uint8 lp_skip_tot;
	uint8 lp_csect_single;
	uint32 last_detection_time;
	uint32 last_detection_time_lp;
	uint32 last_skipped_time;
	uint8 lp_len_his[LP_LEN_HIS_SIZE];
	uint8 lp_len_his_idx;
	int16 min_detected_fc_bin5;
	int16 max_detected_fc_bin5;
	int16 avg_detected_fc_bin5;
	pulse_data_t pulse_tail[RDR_NANTENNAS];
} radar_lp_info_t;

/* RADAR data structure */
typedef struct {
	radar_work_t	radar_work;	/* current radar fifo sample info */
	radar_lp_info_t	radar_lp_info;	/* radar persistent info */
	radar_params_t	rparams;
	phy_radar_detect_mode_t rdm;    /* current radar detect mode FCC/EU */
	wl_radar_status_t radar_status;	/* dump/clear radar status */
	bool first_radar_indicator;	/* first radar indicator */

	radar_lp_info_t	*radar_work_lp_sc;	/* scan core radar persistent info */
	wl_radar_status_t *radar_status_sc;	/* dump/clear radar status */
	bool first_radar_indicator_sc;	/* first radar indicator */
	uint16 subband_result;
} phy_radar_st_t;

/*
 * Query the radar states pointer.
 */
phy_radar_st_t *phy_radar_get_st(phy_radar_info_t *ri);

#endif /* _phy_radar_st_ */
