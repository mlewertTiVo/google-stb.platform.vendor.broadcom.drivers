/*
 * RadarDetect module implementation (shared by PHY implementations)
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
 * $Id: phy_radar_shared.c 639713 2016-05-24 18:02:57Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <phy_api.h>
#include <phy_radar.h>
#include "phy_radar_st.h"
#include <phy_radar_utils.h>
#include <phy_radar_shared.h>
#include <phy_radar_api.h>

#include <phy_utils_reg.h>

#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>

#include <wlc_phy_n.h>
#include <wlc_phyreg_n.h>

#include <wlc_phy_ht.h>
#include <wlc_phyreg_ht.h>

#include <phy_ac_info.h>
#include <wlc_phyreg_ac.h>
#endif /* !ALL_NEW_PHY_MOD */

#define MIN_PULSES		6	/* the minimum number of required consecutive pulses */
#define INTERVAL_TOLERANCE	16	/* in 1/20 us */
#define PULSE_WIDTH_TOLERANCE	30	/* in 1/20 us */
#define PULSE_WIDTH_DIFFERENCE	12	/* in 1/20 us */

typedef struct {
	uint8 radar_type;	/* one of RADAR_TYPE_XXX */
	uint8 num_pulses;	/* min number of pulses required */
	uint16 min_pw;		/* minimum pulse-width (usec * 20) */
	uint16 max_pw;		/* maximum pulse-width (usec * 20) */
	uint16 min_pri;		/* minimum pulse repetition interval (usec) */
	uint16 max_pri;		/* maximum pulse repetition interval (usec) */
} radar_spec_t;

/*
 * Define the specifications used to detect the different Radar types.
 * Note: These are the simple ones.  The long pulse and staggered Radars will be handled seperately.
 * Note: The number of pulses are currently not used, because due to noise we need to reduce the
 *       actual number of pulses to check for (i.e. MIN_PULSES).
 * Note: The order of these are important.  We could execute a sorting routine at startup, but
 *       that will just add to the code size, so instead we order them properly here.
 *       The order should go from smallest max_pw to largest and from smallest range to largest.
 *       This is so that we detect the correct Radar where there's overlap between them.
 */
static const radar_spec_t radar_spec[] = {
	/* Japan 2.1.1: PW 0.5us, PRI 1389us */
	{ RADAR_TYPE_JP2_1_1, 5, 10, 10, 1389, 1389 },	/* num_pulses reduced from 18 */

	/* FCC 6: PW 1us, PRI 333us - same as RADAR_TYPE_JP4 */
	{ RADAR_TYPE_FCC_6, 9, 20, 20, 333, 333 },

	/* Korean 4: PW 1us, PRI 333us (similar to JP 4) */
	{ RADAR_TYPE_KN4, 3, 20, 20, 333, 333 },

	/* Korean 2: PW 1us, PRI 556us */
	{ RADAR_TYPE_KN2, 10, 20, 20, 556, 556 },

	/* ETSI 0: PW 1us, PRI 1429us */
	{ RADAR_TYPE_ETSI_0, 5, 20, 20, 1428, 1429 },	/* num_pulses reduced from 18 */

	/* FCC 0: PW 1us, PRI 1428us - same RADAR_TYPE_JP1_1, RADAR_TYPE_JP2_2_1, RADAR_TYPE_KN1 */
	{ RADAR_TYPE_FCC_0, 5, 20, 20, 1428, 1429 },	/* num_pulses reduced from 18 */

	/* Japan 1.2: PW 2.5us, PRI 3846us */
	{ RADAR_TYPE_JP1_2, 5, 50, 50, 3846, 3846 },	/* num_pulses reduced from 18 */

	/* Japan 2.1.2: PW 2us, PRI 4000us */
	{ RADAR_TYPE_JP2_1_2, 5, 40, 40, 4000, 4000 },	/* num_pulses reduced from 18 */

	/* Korean 3: PW 2us, PRI 3030us */
	{ RADAR_TYPE_KN3, 70, 40, 40, 3030, 3030 },

	/* FCC 1: PW 1us, PRI 518-3066us */
	{ RADAR_TYPE_FCC_1, 5, 20, 20, 518, 3066 },	/* num_pulses reduced from 18 */

	/* FCC 2: PW 1-5us, PRI 150-230us */
	{ RADAR_TYPE_FCC_2, 23, 20, 100, 150, 230 },

	/* ETSI 1: PW 0.5-5us, PRI 1000-5000us */
	{ RADAR_TYPE_ETSI_1, 3, 10, 100, 1000, 5000 },	/* num_pulses reduced from 10 */

	/* Japan 2.2.2: same as FCC 2 */
	{ RADAR_TYPE_JP2_2_2, 23, 20, 100, 150, 230 },

	/* FCC 3: PW 6-10us, PRI 200-500us */
	{ RADAR_TYPE_FCC_3, 16, 120, 200, 200, 500 },

	/* Japan 2.2.3: PW 6-10us, PRI 250-500us (similar to FCC 3) */
	{ RADAR_TYPE_JP2_2_3, 16, 120, 200, 250, 500 },

	/* ETSI 2: PW 0.5-15us, PRI 625-5000us */
	{ RADAR_TYPE_ETSI_2, 5, 10, 300, 625, 5000 },	/* num_pulses reduced from 15 */

	/* ETSI 3: PW 0.5-15us, PRI 250-435us */
	{ RADAR_TYPE_ETSI_3, 25, 10, 300, 250, 435 },

	/* FCC 4: PW 11-20us, PRI 200-500us */
	{ RADAR_TYPE_FCC_4, 12, 220, 400, 200, 500 },

	/* Japan 2.2.4: PW 11-20us, PRI 250-500us (similar to FCC 4) */
	{ RADAR_TYPE_JP2_2_4, 12, 220, 400, 250, 500 },

	/* ETSI 4: PW 20-30us, PRI 250-500us */
	{ RADAR_TYPE_ETSI_4, 20, 400, 600, 250, 500 },

	{ RADAR_TYPE_NONE, 0, 0, 0, 0, 0 }
};

#define MAX_FIFO_SIZE	512

static void
wlc_phy_radar_read_table(phy_info_t *pi, phy_radar_st_t *st)
{
	int i;
	uint8 core;
	uint16 w0, w1, w2, w3;
	int FMOffset = 0;
	uint8 time_shift;
	uint16 RadarFifoCtrlReg[2];
	uint16 RadarFifoDataReg[2];

	bzero(st->radar_work.pulses_input, sizeof(st->radar_work.pulses_input));

	if (ISACPHY(pi)) {
		RadarFifoCtrlReg[0] = ACPHY_Antenna0_radarFifoCtrl(pi->pubpi->phy_rev);
		RadarFifoCtrlReg[1] = ACPHY_Antenna1_radarFifoCtrl(pi->pubpi->phy_rev);
		RadarFifoDataReg[0] = ACPHY_Antenna0_radarFifoData(pi->pubpi->phy_rev);
		RadarFifoDataReg[1] = ACPHY_Antenna1_radarFifoData(pi->pubpi->phy_rev);

		if (TONEDETECTION) {
			FMOffset = 256;
		}
	} else if (ISHTPHY(pi)) {
		RadarFifoCtrlReg[0] = HTPHY_Antenna0_radarFifoCtrl;
		RadarFifoCtrlReg[1] = HTPHY_Antenna1_radarFifoCtrl;
		RadarFifoDataReg[0] = HTPHY_Antenna0_radarFifoData;
		RadarFifoDataReg[1] = HTPHY_Antenna1_radarFifoData;
	} else {
		RadarFifoCtrlReg[0] = NPHY_Antenna0_radarFifoCtrl;
		RadarFifoCtrlReg[1] = NPHY_Antenna1_radarFifoCtrl;
		RadarFifoDataReg[0] = NPHY_Antenna0_radarFifoData;
		RadarFifoDataReg[1] = NPHY_Antenna1_radarFifoData;
	}

	if (IS20MHZ(pi)) {
		time_shift = 0;
	} else if (IS40MHZ(pi)) {
		time_shift = 1;
	} else {
		time_shift = 2;
	}

	for (core = 0; core < GET_RDR_NANTENNAS(pi); core++) {
		st->radar_work.length_input[core] =
			phy_utils_read_phyreg(pi, RadarFifoCtrlReg[core]) & 0x3ff;

		if (st->radar_work.length_input[core] > MAX_FIFO_SIZE) {
			PHY_RADAR(("FIFO LENGTH in ant %d is greater than max_fifo_size of %d\n",
			           core, MAX_FIFO_SIZE));
			st->radar_work.length_input[core] = 0;
		}

#ifdef BCMDBG
		/* enable pulses received at each antenna messages if feature_mask bit-2 is set */
		if (st->rparams.radar_args.feature_mask & RADAR_FEATURE_DEBUG_PULSES_PER_ANT &&
		    st->radar_work.length_input[core] > 5) {
			PHY_RADAR(("ant %d:%d\n", core, st->radar_work.length_input[core]));
		}
#endif /* BCMDBG */

		st->radar_work.length_input[core] /= 4;	/* 4 words per pulse */

		/* use the last sample for bin5 */
		st->radar_work.pulses_bin5[core][0] = st->radar_work.pulse_tail[core];

		for (i = 0; i < st->radar_work.length_input[core]; i++) {
			w0 = phy_utils_read_phyreg(pi, RadarFifoDataReg[core]);
			w1 = phy_utils_read_phyreg(pi, RadarFifoDataReg[core]);
			w2 = phy_utils_read_phyreg(pi, RadarFifoDataReg[core]);
			w3 = phy_utils_read_phyreg(pi, RadarFifoDataReg[core]);

			/* extract the timestamp and pulse widths from the 4 words */
			st->radar_work.pulses_input[core][i].interval =
				(uint32)(((w0 << 16) + ((w1 & 0x0fff) << 4) + (w3 & 0xf))
				>> time_shift);
			st->radar_work.pulses_input[core][i].pw =
				(((w3 & 0x10) << 8) + ((w2 & 0x00ff) << 4) + ((w1 >> 12) & 0x000f))
				>> time_shift;
			st->radar_work.pulses_input[core][i].fm =
				((w3 & 0x20) << 3) + ((w2 >> 8) & 0x00ff) - FMOffset;

			st->radar_work.pulses_bin5[core][i + 1] =
				st->radar_work.pulses_input[core][i];
		}
		/* save the last (tail) sample */
		st->radar_work.pulse_tail[core] = st->radar_work.pulses_bin5[core][i+1];
		st->radar_work.length_bin5[core] = st->radar_work.length_input[core] + 1;
	}
}

static bool
wlc_phy_radar_valid(uint16 feature_mask, uint8 radar_type)
{
	switch (radar_type) {
	case RADAR_TYPE_NONE:
	case RADAR_TYPE_UNCLASSIFIED:
		return FALSE;

	case RADAR_TYPE_ETSI_0:
	case RADAR_TYPE_ETSI_1:
	case RADAR_TYPE_ETSI_2:
	case RADAR_TYPE_ETSI_3:
	case RADAR_TYPE_ETSI_4:
	case RADAR_TYPE_ETSI_5_STG2:
	case RADAR_TYPE_ETSI_5_STG3:
	case RADAR_TYPE_ETSI_6_STG2:
	case RADAR_TYPE_ETSI_6_STG3:
		return (feature_mask & RADAR_FEATURE_ETSI_DETECT) ? TRUE : FALSE;

	default:
		;
	}

	return (feature_mask & RADAR_FEATURE_FCC_DETECT) ? TRUE : FALSE;
}

static const radar_spec_t *
wlc_phy_radar_detect_match(const pulse_data_t *pulse, uint16 feature_mask)
{
	const radar_spec_t *pRadar;
	const pulse_data_t *next_pulse = pulse + 1;

	/* scan the list of Radars for a possible match */
	for (pRadar = &radar_spec[0]; pRadar->radar_type != RADAR_TYPE_NONE; ++pRadar) {

		/* skip Radars not in our region */
		if (!wlc_phy_radar_valid(feature_mask, pRadar->radar_type))
			continue;

		/* need consistent intervals */
		if (ABS((int32)(pulse->interval - next_pulse->interval)) <
		    INTERVAL_TOLERANCE + (int32)pulse->interval / 4000) {
			uint16 max_pw = pRadar->max_pw + pRadar->max_pw/10 + PULSE_WIDTH_TOLERANCE;

			/* check against current radar specifications */

			/* Note: min_pw is currently not used due to a hardware fault causing
			 *       some valid pulses to be spikes of very small widths.  We need to
			 *       ignore the min_pw otherwise we could miss some valid Radar signals
			 */
			if (pulse->pw <= max_pw && next_pulse->pw <= max_pw &&
			    pulse->interval >=
			    (uint32)pRadar->min_pri * 20 - pRadar->min_pri/9 - INTERVAL_TOLERANCE &&
			    pulse->interval <=
			    (uint32)pRadar->max_pri * 20 + pRadar->max_pri/9 + INTERVAL_TOLERANCE) {
				/* match found */
				break;
			}
		}
	}

	return pRadar;	/* points to an entry in radar_spec[] */
}

/* remove outliers from a list */
static int
wlc_phy_radar_filter_list(pulse_data_t inlist[], int length, uint32 min_val, uint32 max_val)
{
	int i, j;
	j = 0;
	for (i = 0; i < length; i++) {
		if ((inlist[i].interval >= min_val) && (inlist[i].interval <= max_val)) {
			inlist[j] = inlist[i];
			j++;
		}
	}

	return j;
}

#define MIN_STAGGERED_INTERVAL		16667	/* 833 us */
#define MAX_STAGGERED_INTERVAL		66667	/* 3333 us */
#define MIN_STAGGERED_PULSES		7	/* the minimum number of required pulses */
#define MAX_STAGGERED_PULSE_WIDTH	40	/* 2 us */
#define	MAX_STAGGERED_ETSI_6_INTERVAL	50000	/* 2500 us */

static void phy_radar_check_staggered(int *nconseq, const pulse_data_t pulse[], uint16 j, int stg)
{
	if (ABS((int32)(pulse[j].interval - pulse[j - stg].interval)) <= INTERVAL_TOLERANCE &&
	    (int)pulse[j].pw <= MAX_STAGGERED_PULSE_WIDTH + PULSE_WIDTH_TOLERANCE &&
	    (int)pulse[j].interval >= MIN_STAGGERED_INTERVAL - INTERVAL_TOLERANCE &&
	    (int)pulse[j].interval <= MAX_STAGGERED_INTERVAL + INTERVAL_TOLERANCE) {
		int k;
		uint16 curr_pw = pulse[j].pw;
		uint16 min_pw = curr_pw;
		uint16 max_pw = curr_pw;

		++(*nconseq);

		/* check all alternate pulses for pulse width tolerance */
		for (k = 0; k < *nconseq; k++) {
			curr_pw = pulse[j - stg * k].pw;
			if (curr_pw < min_pw) {
				min_pw = curr_pw;
			}
			if (curr_pw > max_pw) {
				max_pw = curr_pw;
			}
		}
		if (max_pw - min_pw > PULSE_WIDTH_DIFFERENCE + curr_pw / 200) {
			/* alternate pulse widths inconsistent */
			*nconseq = 0;
		}
	} else {
		/* alternate intervals inconsistent */
		*nconseq = 0;
	}
}

static void phy_radar_check_staggered2(int *nconseq, const pulse_data_t pulse[], uint16 j)
{
	phy_radar_check_staggered(nconseq, pulse, j, 2);
}

static void phy_radar_check_staggered3(int *nconseq, const pulse_data_t pulse[], uint16 j)
{
	phy_radar_check_staggered(nconseq, pulse, j, 3);
}

#define MIN_INTERVAL	3000	/* 150 us */
#define MAX_INTERVAL	120000	/* 6 ms */

static void
get_min_max_pri(pulse_data_t pulses[], int idx, int num_pulses,
                uint32 *min_detected_interval_p, uint32 *max_detected_interval_p)
{
	int i;
	uint32 min_interval = MAX_INTERVAL;
	uint32 max_interval = MIN_INTERVAL;
	uint32 interval;

	for (i = idx; i < idx + num_pulses; ++i) {
		ASSERT(i < RDR_LIST_SIZE);
		interval = pulses[i].interval;
		if (interval < min_interval) {
			min_interval = interval;
		}
		if (interval > max_interval) {
			max_interval = interval;
		}
	}

	*min_detected_interval_p = min_interval;
	*max_detected_interval_p = max_interval;
}

static void
wlc_phy_radar_detect_run_epoch(phy_info_t *pi,
	radar_work_t *rt, radar_params_t *rparams,
	int epoch_length,
	uint8 *det_type_p, int *nconsecq_pulses_p,
	int *detected_pulse_index_p, int *min_detected_pw_p, int *max_detected_pw_p,
	uint32 *min_detected_interval_p, uint32 *max_detected_interval_p)
{
	uint16 j;
	bool radar_detected = FALSE;
	uint8 det_type;
	uint8 previous_det_type = RADAR_TYPE_NONE;
	int detected_pulse_index = 0;
	int nconsecq_pulses = 0;
	int nconseq2even, nconseq2odd;
	int nconseq3a, nconseq3b, nconseq3c;
	int first_interval;
	const radar_spec_t *pRadar;

	epoch_length = wlc_phy_radar_filter_list(rt->pulses, epoch_length,
	                                         MIN_INTERVAL, MAX_INTERVAL);

	/* Detect contiguous only pulses */
	detected_pulse_index = 0;
	nconsecq_pulses = 0;
	nconseq2even = 0;
	nconseq2odd = 0;
	nconseq3a = 0;
	nconseq3b = 0;
	nconseq3c = 0;

	first_interval = rt->pulses[0].interval;
	*min_detected_pw_p = rt->pulses[0].pw;
	*max_detected_pw_p = *min_detected_pw_p;

	for (j = 0; j < epoch_length - 1; j++) {
		uint16 curr_pw = rt->pulses[j].pw;
		uint32 curr_interval = rt->pulses[j].interval;

		/* contiguous pulse detection */
		pRadar = wlc_phy_radar_detect_match(&rt->pulses[j],
		                                    rparams->radar_args.feature_mask);
		det_type = pRadar->radar_type;

		if (det_type != RADAR_TYPE_NONE) {
			/* same interval detected as first ? */
			if (ABS((int32)(curr_interval - first_interval)) <
			    INTERVAL_TOLERANCE + (int32)curr_interval / 4000) {
				if (curr_pw < *min_detected_pw_p) {
					*min_detected_pw_p = curr_pw;
				}
				if (curr_pw > *max_detected_pw_p) {
					*max_detected_pw_p = curr_pw;
				}

				if (*max_detected_pw_p - *min_detected_pw_p <=
				    PULSE_WIDTH_DIFFERENCE + curr_pw / 200) {
					uint8 min_pulses = MIN(MIN_PULSES, pRadar->num_pulses);

					if (++nconsecq_pulses >= min_pulses) {
						/* requirements match a Radar */
						radar_detected = TRUE;
						*nconsecq_pulses_p = nconsecq_pulses;
						detected_pulse_index = j - min_pulses + 1;
						get_min_max_pri(rt->pulses,
								detected_pulse_index, min_pulses,
								min_detected_interval_p,
								max_detected_interval_p);
						PHY_RADAR(("Radar %d: detected_pulse_index=%d\n",
							det_type, detected_pulse_index));
						break;
					}
				} else {
					/* reset to current pulse */
					first_interval = rt->pulses[j].interval;
					*min_detected_pw_p = curr_pw;
					*max_detected_pw_p = curr_pw;
					previous_det_type = det_type;
					nconsecq_pulses = 0;
				}
			} else {
				/* reset to current pulse */
				first_interval = rt->pulses[j].interval;
				*min_detected_pw_p = curr_pw;
				*max_detected_pw_p = curr_pw;
				previous_det_type = det_type;
				nconsecq_pulses = 0;
			}
		} else {
			previous_det_type = RADAR_TYPE_NONE;
			first_interval = rt->pulses[j].interval;
			nconsecq_pulses = 0;
		}

		/* special case for ETSI 4, as it has a chirp of +/-2.5MHz */
		if (det_type == RADAR_TYPE_ETSI_4 && previous_det_type == RADAR_TYPE_ETSI_4) {
			int k;
			int fm_dif;
			int fm_tol;
			int fm_min = 1030;
			int fm_max = 0;

			/* not a valid ETSI 4 if fm doesn't vary */
			for (k = 0; k < nconsecq_pulses; k++) {
				if (ISACPHY(pi) && TONEDETECTION) {
					if (rt->pulses[j - k].fm < 240) {
						previous_det_type = RADAR_TYPE_NONE;
						nconsecq_pulses = 0;
						break;
					}
				} else {
					if (rt->pulses[j - k].fm < fm_min) {
						fm_min = rt->pulses[j - k].fm;
					}
					if (rt->pulses[j - k].fm > fm_max) {
						fm_max = rt->pulses[j - k].fm;
					}
					fm_dif = ABS(fm_max - fm_min);
					fm_tol = (fm_min + fm_max) / 4;
					if (fm_dif > fm_tol) {
						previous_det_type = RADAR_TYPE_NONE;
						nconsecq_pulses = 0;
						break;
					}
				}
			}
		}

		/* staggered 2/3 single filters */
		if (rparams->radar_args.feature_mask & RADAR_FEATURE_ETSI_DETECT) {
			/* staggered 2 even */
			if (j >= 2 && j % 2 == 0) {
				phy_radar_check_staggered2(&nconseq2even, rt->pulses, j);
			}

			/* staggered 2 odd */
			if (j >= 3 && j % 2 == 1) {
				phy_radar_check_staggered2(&nconseq2odd, rt->pulses, j);
			}

			if (nconseq2even >= MIN_STAGGERED_PULSES / 2 &&
			    nconseq2odd >= MIN_STAGGERED_PULSES / 2) {
				radar_detected = TRUE;
				detected_pulse_index = j - (MIN_STAGGERED_PULSES / 2) * 2 - 1;
				if (detected_pulse_index < 0)
					detected_pulse_index = 0;
				if (rt->pulses[detected_pulse_index].interval >=
						MAX_STAGGERED_ETSI_6_INTERVAL &&
				    rt->pulses[detected_pulse_index+1].interval >=
						MAX_STAGGERED_ETSI_6_INTERVAL) {
					det_type = RADAR_TYPE_ETSI_5_STG2;
				} else {
					det_type = RADAR_TYPE_ETSI_6_STG2;
				}
				*min_detected_interval_p =
					rt->pulses[detected_pulse_index].interval;
				*max_detected_interval_p = *min_detected_interval_p;
				*min_detected_pw_p = rt->pulses[detected_pulse_index].pw;
				*max_detected_pw_p = *min_detected_pw_p;
				*nconsecq_pulses_p = nconseq2odd + nconseq2even;
				PHY_RADAR(("j=%d detected_pulse_index=%d\n",
					j, detected_pulse_index));
				break;	/* radar detected */
			}

			/* staggered 3-a */
			if (j >= 3 && j % 3 == 0) {
				phy_radar_check_staggered3(&nconseq3a, rt->pulses, j);
			}

			/* staggered 3-b */
			if (j >= 4 && j % 3 == 1) {
				phy_radar_check_staggered3(&nconseq3b, rt->pulses, j);
			}

			/* staggered 3-c */
			if (j >= 5 && j % 3 == 2) {
				phy_radar_check_staggered3(&nconseq3c, rt->pulses, j);
			}

			if (nconseq3a >= MIN_STAGGERED_PULSES / 3 &&
			    nconseq3b >= MIN_STAGGERED_PULSES / 3 &&
			    nconseq3c >= MIN_STAGGERED_PULSES / 3) {
				radar_detected = TRUE;
				detected_pulse_index = j - (MIN_STAGGERED_PULSES / 3) * 3 - 2;
				if (detected_pulse_index < 0)
					detected_pulse_index = 0;
				if (rt->pulses[detected_pulse_index].interval >=
						MAX_STAGGERED_ETSI_6_INTERVAL &&
				    rt->pulses[detected_pulse_index+1].interval >=
						MAX_STAGGERED_ETSI_6_INTERVAL) {
					det_type = RADAR_TYPE_ETSI_5_STG3;
				} else {
					det_type = RADAR_TYPE_ETSI_6_STG3;
				}
				*min_detected_interval_p =
					rt->pulses[detected_pulse_index].interval;
				*max_detected_interval_p = *min_detected_interval_p;
				*min_detected_pw_p = rt->pulses[detected_pulse_index].pw;
				*max_detected_pw_p = *min_detected_pw_p;
				*nconsecq_pulses_p = nconseq3a + nconseq3b + nconseq3c;
				PHY_RADAR(("j=%d detected_pulse_index=%d\n",
					j, detected_pulse_index));
				break;	/* radar detected */
			}
		}
	}  /* for (j = 0; j < epoch_length - 2; j++) */

	if (radar_detected) {
		*det_type_p = det_type;
		*detected_pulse_index_p = detected_pulse_index;
	} else {
		*det_type_p = RADAR_TYPE_NONE;
		*min_detected_interval_p = 0;
		*max_detected_interval_p = 0;
		*nconsecq_pulses_p = 0;
		*detected_pulse_index_p = 0;
	}
}

#ifdef BCMDBG
static void
phy_radar_output_pulses(pulse_data_t pulses[], uint16 length, uint16 feature_mask)
{
	uint16 i;

	if (feature_mask & RADAR_FEATURE_DEBUG_PULSE_DATA) {
		PHY_RADAR(("\ntstamp=[ "));
		for (i = 0; i < length; i++) {
			PHY_RADAR(("%u ", pulses[i].interval));
		}
		PHY_RADAR(("];\n"));

		PHY_RADAR(("(Interval, PW, FM) = [ "));
		for (i = 1; i < length; i++) {
			PHY_RADAR(("(%u, %u, %d) ",
			           pulses[i].interval - pulses[i - 1].interval,
			           pulses[i].pw, pulses[i].fm));
		}
		PHY_RADAR(("];\n"));
	}
}
#else
#define phy_radar_output_pulses(a, b, c)
#endif /* BCMDBG */

static void
phy_radar_detect_fcc5(const phy_info_t *pi,
                      phy_radar_st_t *st, radar_detected_info_t *radar_detected)
{
	uint16 i;
	uint16 j;
	int k;
	uint8 ant;
	int wr_ptr;
	uint16 mlength;
	int32 deltat;
	int skip_type;
	uint16 width;
	int16 fm;
	bool valid_lp;
	int32 salvate_intv;
	int pw_dif, pw_tol, fm_dif, fm_tol;
	int FMCombineOffset;
	radar_work_t *rt = &st->radar_work;
	radar_params_t *rparams = &st->rparams;

	if (ISACPHY(pi) && TONEDETECTION) {
		FMCombineOffset =
			MAX((2 << (((rparams->radar_args.st_level_time>> 12) & 0xf) -1)) -1, 0);
	} else {
		FMCombineOffset = 0;
	}

	/* t2_min[15:12] = x; if n_non_single >= x && lp_length > npulses_lp => bin5 detected */
	/* t2_min[11:10] = # times combining adjacent pulses < min_pw_lp  */
	/* t2_min[9] = fm_tol enable */
	/* t2_min[8] = skip_type 5 enable */
	/* t2_min[7:4] = y; bin5 remove pw <= 10*y  */
	/* t2_min[3:0] = t; non-bin5 remove pw <= 5*y */

	/* remove "noise" pulses with small pw */
	for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) {
		wr_ptr = 0;
		mlength = rt->length_bin5[ant];
		for (i = 0; i < rt->length_bin5[ant]; i++) {
			if (rt->pulses_bin5[ant][i].pw >
			    10*((rparams->radar_args.t2_min >> 4) & 0xf) &&
			    rt->pulses_bin5[ant][i].fm >
			    ((ISACPHY(pi) && TONEDETECTION) ?
			     rparams->radar_args.min_fm_lp : rparams->radar_args.min_fm_lp/2)) {
				rt->pulses_bin5[ant][wr_ptr] = rt->pulses_bin5[ant][i];
				++wr_ptr;
			} else {
				mlength--;
			}
		}	/* for mlength loop */
		rt->length_bin5[ant] = mlength;
	}	/* for ant loop */

#ifdef BCMDBG
	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_SHORT_PULSE) == 0) { /* bin5 */
		for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) {
			PHY_RADAR(("\nBin5 after removing noise pulses with pw <= %d",
				((rparams->radar_args.t2_min >> 4) & 0xf) * 10));
			PHY_RADAR(("\nAnt %d: %d pulses, ", ant, rt->length_bin5[ant]));

			phy_radar_output_pulses(rt->pulses_bin5[ant], rt->length_bin5[ant],
			                        rparams->radar_args.feature_mask);
		}
	}
#endif /* BCMDBG */

	/* Combine pulses that are adjacent for ant 0 and ant 1 */
	for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) {
		rt->length =  rt->length_bin5[ant];
		for (k = 0; k < ((rparams->radar_args.t2_min >> 10) & 0x3); k++) {
			mlength = rt->length;
			if (mlength > 1) {
				for (i = 1; i < mlength; i++) {
					deltat = ABS((int32)(rt->pulses_bin5[ant][i].interval -
					             rt->pulses_bin5[ant][i-1].interval));

					if (deltat <= (int32)rparams->radar_args.max_pw_lp) {
						rt->pulses_bin5[ant][i-1].pw =
							deltat + rt->pulses_bin5[ant][i].pw;
						rt->pulses_bin5[ant][i-1].fm =
							(rt->pulses_bin5[ant][i-1].fm +
							rt->pulses_bin5[ant][i].fm) -
							FMCombineOffset;
						for (j = i; j < mlength - 1; j++) {
							rt->pulses_bin5[ant][j] =
								rt->pulses_bin5[ant][j+1];
						}
						mlength--;
						rt->length--;
					}	/* if deltat */
				}	/* for mlength loop */
			}	/* mlength > 1 */
		}
	} /* for ant loop */

	ant = 0;	/* Use data from one of the antenna */
	rt->length = 0;
	for (i = 0; i < rt->length_bin5[ant]; i++) {
		rt->pulses[rt->length] = rt->pulses_bin5[ant][i];
		rt->length++;
	}

#ifdef BCMDBG
	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_SHORT_PULSE) == 0) { /* bin5 */
		PHY_RADAR(("\nBin5 after use data from one of two antennas"));
		PHY_RADAR(("\n%d pulses, ", rt->length));

		phy_radar_output_pulses(rt->pulses, rt->length, rparams->radar_args.feature_mask);
	}
#endif /* BCMDBG */

	/* remove pulses that are spaced less than high byte of "blanlk" */
	for (i = 1; i < rt->length; i++) {
		deltat = ABS((int32)(rt->pulses[i].interval - rt->pulses[i-1].interval));
		if (deltat < (ISACPHY(pi) ?
			(((int32)rparams->radar_args.blank & 0xff00) >> 8) : 128)) {
			for (j = i - 1; j < (rt->length); j++) {
				rt->pulses[j] = rt->pulses[j + 1];
			}
			rt->length--;
		}
	}

#ifdef BCMDBG
	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_SHORT_PULSE) == 0) { /* bin5 */
		PHY_RADAR(("\nBin5 after removing pulses that are spaced < %d",
		           rparams->radar_args.quant >> 8));
		PHY_RADAR(("\n%d pulses, ", rt->length));

		phy_radar_output_pulses(rt->pulses, rt->length, rparams->radar_args.feature_mask);
	}
#endif /* BCMDBG */

	/* prune lp buffer */
	/* remove any entry outside the time max delta_t_lp */
	if (rt->lp_length > 1) {
		deltat = ABS((int32)(rt->lp_buffer[rt->lp_length - 1] - rt->lp_buffer[0]));
		i = 0;
		while ((i < (rt->lp_length - 1)) && (deltat > MAX_LP_BUFFER_SPAN_20MHZ)) {
			i++;
			deltat = ABS((int32)(rt->lp_buffer[rt->lp_length - 1] - rt->lp_buffer[i]));
		}

		if (i > 0) {
			for (j = i; j < rt->lp_length; j++) {
				rt->lp_buffer[j-i] = rt->lp_buffer[j];
			}

			rt->lp_length -= i;
		}
	}
	/* First perform FCC-5 detection */
	/* add new pulses */

	/* process each new pulse */
	for (i = 0; i < rt->length; i++) {
		deltat = ABS((int32)(rt->pulses[i].interval - rt->last_tstart));
		salvate_intv = ABS((int32)(rt->pulses[i].interval - rt->last_skipped_time));
		valid_lp = (rt->pulses[i].pw >= rparams->radar_args.min_pw_lp) &&
			(rt->pulses[i].pw <= rparams->radar_args.max_pw_lp) &&
			(rt->pulses[i].fm >= rparams->radar_args.min_fm_lp) &&
			(deltat >= rparams->min_deltat_lp);

		/* filter out: max_deltat_l < pburst_intv_lp < min_burst_intv_lp */
		/* this was skip-type = 2, now not skipping for this */
		valid_lp = valid_lp &&(deltat <= (int32) rparams->max_deltat_lp ||
			deltat >= (int32) rparams->radar_args.min_burst_intv_lp);

		if (rt->lp_length > 0 && rt->lp_length < LP_BUFFER_SIZE) {
			valid_lp = valid_lp &&
				(rt->pulses[i].interval != rt->lp_buffer[rt->lp_length]);
		}

		skip_type = 0;
		if (valid_lp && deltat >= (int32) rparams->radar_args.min_burst_intv_lp &&
			deltat < (int32) rparams->radar_args.max_burst_intv_lp) {
			rt->lp_cnt = 0;
		}

		/* skip the pulse if outside of pulse interval range (1-2ms), */
		/* burst to burst interval not within range, more than 3 pulses in a */
		/* burst, and not skip salvated */

		if ((valid_lp && ((rt->lp_length != 0)) &&
			((deltat < (int32) rparams->min_deltat_lp) ||
			(deltat > (int32) rparams->max_deltat_lp &&
			deltat < (int32) rparams->radar_args.min_burst_intv_lp) ||
			(deltat > (int32) rparams->radar_args.max_burst_intv_lp) ||
			(rt->lp_cnt > 2)))) {	/* possible skip lp */

			/* get skip type */
			if (deltat < (int32) rparams->min_deltat_lp) {
				skip_type = 1;
			} else if (deltat > (int32) rparams->max_deltat_lp &&
				deltat < (int32) rparams->radar_args.min_burst_intv_lp) {
				skip_type = 2;
			} else if (deltat > (int32) rparams->radar_args.max_burst_intv_lp) {
				skip_type = 3;
				rt->lp_cnt = 0;
			} else if (rt->lp_cnt > 2) {
				skip_type = 4;
			} else {
				skip_type = 999;
			}

			/* skip_salvate */
			if ((((salvate_intv > (int32) rparams->min_deltat_lp &&
				salvate_intv < (int32) rparams->max_deltat_lp)) ||
				((salvate_intv > (int32)rparams->radar_args.min_burst_intv_lp) &&
				(salvate_intv < (int32)rparams->radar_args.max_burst_intv_lp))) &&
				(skip_type != 4)) {
				/* note valid_lp is not reset in here */
				skip_type = -1;  /* salvated PASSED */
				if (salvate_intv >= (int32) rparams->radar_args.min_burst_intv_lp &&
					salvate_intv <
						(int32) rparams->radar_args.max_burst_intv_lp) {
					rt->lp_cnt = 0;
				}
			}
		} else {  /* valid lp not by skip salvate */
			skip_type = -2;
		}

		width = 0;
		fm = 0;
		pw_dif = 0;
		fm_dif = 0;
		pw_tol = rparams->radar_args.max_span_lp & 0xff;
		fm_tol = 0;
		/* monitor the number of pw and fm matching */
		/* max_span_lp[15:12] = skip_tot max */
		/* max_span_lp[11:8] = x, x/16 = % alowed fm tollerance */
		/* max_span_lp[7:0] = alowed pw tollerance */
		if (valid_lp && skip_type <= 0) {
			if (rt->lp_cnt == 0) {
				rt->lp_pw[0] = rt->pulses[i].pw;
				rt->lp_fm[0] = rt->pulses[i].fm;
			} else if (rt->lp_cnt == 1) {
				width = rt->lp_pw[0];
				fm = rt->lp_fm[0];
				pw_dif = ABS(rt->pulses[i].pw - width);
				fm_dif = ABS(rt->pulses[i].fm - fm);
				if (rparams->radar_args.t2_min & 0x200) {
					fm_tol = (fm*((rparams->radar_args.max_span_lp >> 8)
						& 0xf))/16;
				} else {
					fm_tol = 999;
				}
				if (pw_dif < pw_tol && fm_dif < fm_tol) {
					rt->lp_pw[1] = rt->pulses[i].pw;
					rt->lp_fm[1] = rt->pulses[i].fm;
					++rt->lp_n_non_single_pulses;
					++rt->lp_pw_fm_matched;
				} else if (rt->lp_just_skipped) {
					width = rt->lp_skipped_pw;
					fm = rt->lp_skipped_fm;
					pw_dif = ABS(rt->pulses[i].pw - width);
					fm_dif = ABS(rt->pulses[i].fm - fm);
					if (rparams->radar_args.t2_min & 0x200) {
						fm_tol = (fm*((rparams->radar_args.max_span_lp >> 8)
							& 0xf))/16;
					} else {
						fm_tol = 999;
					}
					if (pw_dif < pw_tol && fm_dif < fm_tol) {
						rt->lp_pw[1] = rt->pulses[i].pw;
						rt->lp_fm[1] = rt->pulses[i].fm;
						++rt->lp_n_non_single_pulses;
						++rt->lp_pw_fm_matched;
						skip_type = -1;  /* salvated PASSED */
					} else {
						if (rparams->radar_args.t2_min & 0x100) {
							skip_type = 5;
						}
					}
				} else {
					if (rparams->radar_args.t2_min & 0x100) {
						skip_type = 5;
					}
				}
			} else if (rt->lp_cnt == 2) {
				width = rt->lp_pw[1];
				fm = rt->lp_fm[1];
				pw_dif = ABS(rt->pulses[i].pw - width);
				fm_dif = ABS(rt->pulses[i].fm - fm);
				if (rparams->radar_args.t2_min & 0x200) {
					fm_tol = (fm*((rparams->radar_args.max_span_lp >> 8)
						& 0xf))/16;
				} else {
					fm_tol = 999;
				}
				if (pw_dif < pw_tol && fm_dif < fm_tol) {
					rt->lp_pw[2] = rt->pulses[i].pw;
					rt->lp_fm[2] = rt->pulses[i].fm;
					++rt->lp_n_non_single_pulses;
					++rt->lp_pw_fm_matched;
				} else if (rt->lp_just_skipped) {
					width = rt->lp_skipped_pw;
					fm = rt->lp_skipped_fm;
					pw_dif = ABS(rt->pulses[i].pw - width);
					fm_dif = ABS(rt->pulses[i].fm - fm);
					if (rparams->radar_args.t2_min & 0x200) {
						fm_tol = (fm*((rparams->radar_args.max_span_lp >> 8)
							& 0xf))/16;
					} else {
						fm_tol = 999;
					}
					if (pw_dif < pw_tol && fm_dif < fm_tol) {
						rt->lp_pw[2] = rt->pulses[i].pw;
						rt->lp_fm[2] = rt->pulses[i].fm;
						++rt->lp_n_non_single_pulses;
						++rt->lp_pw_fm_matched;
						skip_type = -1;  /* salvated PASSED */
					} else {
						if (rparams->radar_args.t2_min & 0x100) {
							skip_type = 5;
						}
					}
				} else {
					if (rparams->radar_args.t2_min & 0x100) {
						skip_type = 5;
					}
				}
			}
		}

		if (valid_lp && skip_type != -1 && skip_type != -2) {	/* skipped lp */
			valid_lp = FALSE;
			rt->lp_skip_cnt++;
			rt->lp_skip_tot++;
			rt->lp_just_skipped = TRUE;
			rt->lp_skipped_pw = rt->pulses[i].pw;
			rt->lp_skipped_fm = rt->pulses[i].fm;

			rt->last_skipped_time = rt->pulses[i].interval;

#ifdef BCMDBG
			/* print "SKIPPED LP" debug messages */
			PHY_RADAR(("Skipped LP:"
/*
				" KTstart=%u Klast_ts=%u Klskip=%u"
*/
				" nLP=%d nSKIP=%d KIntv=%u"
				" Ksalintv=%d PW=%d FM=%d"
				" Type=%d pulse#=%d skip_tot=%d csect_single=%d\n",
/*
			(rt->pulses[i].interval)/1000, rt->last_tstart/1000, tmp_uint32/1000,
*/
				rt->lp_length, rt->lp_skip_cnt, deltat/1000, salvate_intv/1000,
				rt->pulses[i].pw, rt->pulses[i].fm,
				skip_type, rt->lp_cnt, rt->lp_skip_tot, rt->lp_csect_single));
			if (skip_type == 5) {
				PHY_RADAR(("           "
					" pw2=%d pw_dif=%d pw_tol=%d fm2=%d fm_dif=%d fm_tol=%d\n",
					width, pw_dif, pw_tol, fm, fm_dif, fm_tol));
			}
			if (skip_type == 999) {
				PHY_RADAR(("UNKOWN SKIP TYPE: %d\n", skip_type));
			}
#endif /* BCMDBG */

			/* if a) 2 consecutive skips, or */
			/*    b) too many consective singles, or */
			/*    c) too many total skip so far */
			/*  then reset lp buffer ... */
			if (rt->lp_skip_cnt >= rparams->radar_args.nskip_rst_lp) {
				if (rt->lp_len_his_idx < LP_LEN_HIS_SIZE) {
					rt->lp_len_his[rt->lp_len_his_idx] = rt->lp_length;
					rt->lp_len_his_idx++;
				}
				rt->lp_length = 0;
				rt->lp_skip_tot = 0;
				rt->lp_skip_cnt = 0;
				rt->lp_csect_single = 0;
				rt->lp_pw_fm_matched = 0;
				rt->lp_n_non_single_pulses = 0;
				rt->lp_cnt = 0;
			}
		} else if (valid_lp && (rt->lp_length < LP_BUFFER_SIZE)) {	/* valid lp */
			/* reset consecutive singles counter if pulse # > 0 */
			if (rt->lp_cnt > 0) {
				rt->lp_csect_single = 0;
			} else {
				++rt->lp_csect_single;
			}

			rt->lp_just_skipped = FALSE;
			/* print "VALID LP" debug messages */
			rt->lp_skip_cnt = 0;
			PHY_RADAR(("Valid LP:"
/*
				" KTstart=%u KTlast_ts=%u Klskip=%u"
*/
				" KIntv=%u"
				" Ksalintv=%d PW=%d FM=%d pulse#=%d"
				" pw2=%d pw_dif=%d pw_tol=%d fm2=%d fm_dif=%d fm_tol=%d\n",
/*
				(rt->pulses[i].interval)/1000, rt->last_tstart/1000,
					rt->last_skipped_time/1000,
*/
				deltat/1000, salvate_intv/1000,
				rt->pulses[i].pw, rt->pulses[i].fm, rt->lp_cnt,
				width, pw_dif, pw_tol,
				fm, fm_dif, fm_tol));
			PHY_RADAR(("         "
				" nLP=%d nSKIP=%d skipped_salvate=%d"
				" pw_fm_matched=%d #non-single=%d skip_tot=%d csect_single=%d\n",
				rt->lp_length + 1, rt->lp_skip_cnt, (skip_type == -1 ? 1 : 0),
				rt->lp_pw_fm_matched,
				rt->lp_n_non_single_pulses, rt->lp_skip_tot, rt->lp_csect_single));

				rt->lp_buffer[rt->lp_length] = rt->pulses[i].interval;
				rt->lp_length++;
				rt->last_tstart = rt->pulses[i].interval;
				rt->last_skipped_time = rt->pulses[i].interval;

				rt->lp_cnt++;

			if (rt->lp_csect_single >= ((rparams->radar_args.t2_min >> 12) & 0xf)) {
				if (rt->lp_length > rparams->radar_args.npulses_lp / 2)
					rt->lp_length -= rparams->radar_args.npulses_lp / 2;
				else
					rt->lp_length = 0;
			}
		}
	}

	if (rt->lp_length > LP_BUFFER_SIZE)
		PHY_ERROR(("WARNING: LP buffer size is too long\n"));

#ifdef RADAR_DBG
	PHY_RADAR(("\n FCC-5 \n"));
	for (i = 0; i < rt->lp_length; i++) {
		PHY_RADAR(("%u  ", rt->lp_buffer[i]));
	}
	PHY_RADAR(("\n"));
#endif
	if (rt->lp_length >= rparams->radar_args.npulses_lp) {
		/* reject detection spaced more than 3 minutes */
		deltat = (uint32) (pi->sh->now - rt->last_detection_time_lp);
		PHY_RADAR(("last_detection_time_lp=%u, watchdog_timer=%u, deltat=%d,"
			" deltat_min=%d, deltat_sec=%d\n",
			rt->last_detection_time_lp, pi->sh->now, deltat, deltat/60, deltat%60));
		rt->last_detection_time_lp = pi->sh->now;
		if ((uint32) deltat < (rparams->radar_args.fra_pulse_err & 0xff)*60 ||
		    (st->first_radar_indicator == 1 && (uint32) deltat < 30 * 60)) {
			if (rt->lp_csect_single <= rparams->radar_args.npulses_lp - 2 &&
				rt->lp_skip_tot < ((rparams->radar_args.max_span_lp >> 12) & 0xf)) {
				PHY_RADAR(("FCC-5 Radar Detection. Time from last detection"
					" = %u, = %dmin %dsec\n",
					deltat, deltat / 60, deltat % 60));

				radar_detected->radar_type = RADAR_TYPE_FCC_5;
				st->radar_status.detected = TRUE;
				st->radar_status.count =
					st->radar_status.count + 1;
				st->radar_status.pretended = FALSE;
				st->radar_status.radartype = radar_detected->radar_type;
				st->radar_status.timenow = (uint32) (pi->sh->now);
				st->radar_status.timefromL = deltat;
				st->radar_status.ch = pi->radio_chanspec;
				st->radar_status.lp_csect_single = rt->lp_csect_single;
			}
		}
#ifdef BCMDBG
		else {
			if (rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_REJECTED_RADAR) {
				PHY_RADAR(("SKIPPED false FCC-5 Radar Detection."
					" Time from last detection = %u, = %dmin %dsec,"
					" ncsect_single=%d\n",
					deltat, deltat / 60, deltat % 60, rt->lp_csect_single));
			}
		}
#endif /* BCMDBG */
		if (rt->lp_len_his_idx < LP_LEN_HIS_SIZE) {
			rt->lp_len_his[rt->lp_len_his_idx] = rt->lp_length;
			rt->lp_len_his_idx++;
		}
		rt->lp_length = 0;
		rt->lp_pw_fm_matched = 0;
		rt->lp_n_non_single_pulses = 0;
		rt->lp_skip_tot = 0;
		rt->lp_csect_single = 0;
		st->first_radar_indicator = 0;
	}
}

/* [PHY_REARCH] Need to rename this function. It is used in AC, N and HT */
uint8
phy_radar_run_nphy(phy_info_t *pi, radar_detected_info_t *radar_detected)
{
	phy_radar_info_t *ri = pi->radari;
	phy_radar_st_t *st = phy_radar_get_st(ri);
/* NOTE: */
/* PLEASE UPDATE THE DFS_SW_VERSION #DEFINE'S IN FILE WLC_PHY_INT.H */
/* EACH TIME ANY DFS FUNCTION IS MODIFIED EXCEPT RADAR THRESHOLD CHANGES */
	uint16 i;
	uint16 j;
	int k;
	int wr_ptr;
	uint8 ant;
	uint16 mlength;
	int32 deltat;
	radar_work_t *rt;
	radar_params_t *rparams;
	uint8 det_type;
	int min_detected_pw;
	int max_detected_pw;
	uint32 min_detected_interval;
	uint32 max_detected_interval;
	int nconsecq_pulses = 0;
	int detected_pulse_index = 0;
	int32 deltat2 = 0;

	rt = &st->radar_work;
	rparams = &st->rparams;

	ASSERT(radar_detected != NULL);

	(void) memset(radar_detected, 0, sizeof(*radar_detected));

	/* clear LP buffer if requested, and print LP buffer count history */
	if (pi->dfs_lp_buffer_nphy != 0) {
		pi->dfs_lp_buffer_nphy = 0;
		st->first_radar_indicator = 1;
		PHY_RADAR(("DFS LP buffer =  "));
		for (i = 0; i < rt->lp_len_his_idx; i++) {
			PHY_RADAR(("%d, ", rt->lp_len_his[i]));
		}
		PHY_RADAR(("%d; now CLEARED\n", rt->lp_length));
		rt->lp_length = 0;
		rt->lp_pw_fm_matched = 0;
		rt->lp_n_non_single_pulses = 0;
		rt->lp_cnt = 0;
		rt->lp_skip_cnt = 0;
		rt->lp_skip_tot = 0;
		rt->lp_csect_single = 0;
		rt->lp_len_his_idx = 0;
		rt->last_detection_time = pi->sh->now;
		rt->last_detection_time_lp = pi->sh->now;
	}
	if (!rparams->radar_args.npulses) {
		PHY_ERROR(("%s: radar params not initialized\n", __FUNCTION__));
		return RADAR_TYPE_NONE;
	}

	min_detected_pw = rparams->radar_args.max_pw;
	max_detected_pw = rparams->radar_args.min_pw;

	/* suspend mac before reading phyregs */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	wlc_phy_radar_read_table(pi, st);

	/* restart mac after reading phyregs */
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	/* skip radar detect if doing periodic cal
	 * (the test-tones utilized during cal can trigger
	 * radar detect)
	 * NEED TO BE HERE AFTER READING DATA FROM (CLEAR) THE FIFO
	 */
	if (ISNPHY(pi)) {
	  if (pi->u.pi_nphy->nphy_rxcal_active) {
	    pi->u.pi_nphy->nphy_rxcal_active = FALSE;
	    PHY_RADAR(("DOING RXCAL, SKIP RADARS\n"));
	    return RADAR_TYPE_NONE;
	  }
	}
	if (ISHTPHY(pi)) {
	  if (pi->u.pi_htphy->radar_cal_active) {
	    pi->u.pi_htphy->radar_cal_active = FALSE;
	    PHY_RADAR(("DOING CAL, SKIP RADARS\n"));
	    return RADAR_TYPE_NONE;
	  }
	}
	if (ISACPHY(pi)) {
	  if (pi->u.pi_acphy->radar_cal_active) {
	    pi->u.pi_acphy->radar_cal_active = FALSE;
	    PHY_RADAR(("DOING CAL, SKIP RADARS\n"));
	    return RADAR_TYPE_NONE;
	  }
	}

	/*
	 * Reject if no pulses recorded
	 */
	if (GET_RDR_NANTENNAS(pi) == 2) {
	    if ((rt->length_input[0] < 1) && (rt->length_input[1] < 1)) {
			return RADAR_TYPE_NONE;
		}
	} else {
		if (rt->length_input[0] < 1) {
		return RADAR_TYPE_NONE;
	    }
	}

#ifdef BCMDBG
	for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) {
		PHY_RADAR(("\nPulses read from PHY FIFO"));
		PHY_RADAR(("\nAnt %d: %d pulses, ", ant, rt->length_input[ant]));

		phy_radar_output_pulses(rt->pulses_input[ant], rt->length_input[ant],
		                        rparams->radar_args.feature_mask);
	}
#endif /* BCMDBG */

	/* START LONG PULSES (BIN5) DETECTION */
	if (rparams->radar_args.feature_mask & RADAR_FEATURE_FCC_DETECT) {	/* if fcc */
		phy_radar_detect_fcc5(pi, st, radar_detected);

		if (radar_detected->radar_type == RADAR_TYPE_FCC_5) {
			return RADAR_TYPE_FCC_5;
		}
	}

	/* START SHORT PULSES (NON-BIN5) DETECTION */
	/* remove "noise" pulses with  pw > 400 and fm < 244 */
	if (ISACPHY(pi) && TONEDETECTION) {
	for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) {
		wr_ptr = 0;
		mlength = rt->length_input[ant];
		for (i = 0; i < rt->length_input[ant]; i++) {
		  if (rt->pulses_input[ant][i].pw  < (rparams->radar_args.st_level_time & 0x0fff) ||
		      rt->pulses_input[ant][i].fm > rparams->radar_args.min_fm_lp) {
				rt->pulses_input[ant][wr_ptr] = rt->pulses_input[ant][i];
				++wr_ptr;
			} else {
				mlength--;
			}
		}	/* for mlength loop */
		rt->length_input[ant] = mlength;
	}	/* for ant loop */
	}
	/* Combine pulses that are adjacent */
	for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) {
		for (k = 0; k < 2; k++) {
			mlength = rt->length_input[ant];
			if (mlength > 1) {
			for (i = 1; i < mlength; i++) {
				deltat = ABS((int32)(rt->pulses_input[ant][i].interval -
					rt->pulses_input[ant][i-1].interval));
				if ((deltat < (int32)rparams->radar_args.min_deltat && FALSE) ||
				    (deltat <= (int32)rparams->radar_args.max_pw && TRUE))
				{
					if (ISNPHY(pi) || ISHTPHY(pi) || ISACPHY(pi)) {
						if (((uint32)(rt->pulses_input[ant][i].interval -
							rt->pulses_input[ant][i-1].interval))
							<= (uint32) rparams->radar_args.max_pw) {
							rt->pulses_input[ant][i-1].pw =
							ABS((int32)(rt->pulses_input[ant][i].
							interval -
							rt->pulses_input[ant][i-1].interval)) +
							rt->pulses_input[ant][i].pw;
						}
					} else {
#ifdef BCMDBG
						/* print pulse combining debug messages */
						PHY_RADAR(("*%d,%d,%d ",
							rt->pulses_input[ant][i].interval -
							rt->pulses_input[ant][i-1].interval,
							rt->pulses_input[ant][i].pw,
							rt->pulses_input[ant][i-1].pw));
#endif
						if (rparams->radar_args.feature_mask &
								RADAR_FEATURE_USE_MAX_PW) {
							rt->pulses_input[ant][i-1].pw =
								(rt->pulses_input[ant][i-1].pw >
								rt->pulses_input[ant][i].pw ?
								rt->pulses_input[ant][i-1].pw :
								rt->pulses_input[ant][i].pw);
						} else {
							rt->pulses_input[ant][i-1].pw =
								rt->pulses_input[ant][i-1].pw +
								rt->pulses_input[ant][i].pw;
						}
					}

					/* Combine fm */
					if (ISACPHY(pi) && TONEDETECTION) {
						rt->pulses_input[ant][i-1].fm =
							(rt->pulses_input[ant][i-1].fm >
							rt->pulses_input[ant][i].fm) ?
							rt->pulses_input[ant][i-1].fm :
							rt->pulses_input[ant][i].fm;
					} else {
						rt->pulses_input[ant][i-1].fm =
							rt->pulses_input[ant][i-1].fm +
							rt->pulses_input[ant][i].fm;
					}

					for (j = i; j < mlength - 1; j++) {
						rt->pulses_input[ant][j] =
							rt->pulses_input[ant][j+1];
					}
					mlength--;
					rt->length_input[ant]--;
				}
			} /* for i < mlength */
			} /* mlength > 1 */
		}
	}
#ifdef BCMDBG
	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_SHORT_PULSE) != 0 &&
	    rt->length_input[0] > 0) {	/* short pulses */
		for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) {
			PHY_RADAR(("\nShort Pulse After combining adjacent pulses"));
			PHY_RADAR(("\nAnt %d: %d pulses, ", ant, rt->length_input[ant]));

			phy_radar_output_pulses(rt->pulses_input[ant], rt->length_input[ant],
			                        rparams->radar_args.feature_mask);
		}
	}
#endif /* BCMDBG */

	for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) {

	bzero(rt->pulses, sizeof(rt->pulses));
	rt->length = 0;

	/* Use data from one of the antenna */
	for (i = 0; i < rt->length_input[ant]; i++) {
		rt->pulses[rt->length] = rt->pulses_input[ant][i];
		rt->length++;
	}

#ifdef BCMDBG
	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_SHORT_PULSE) != 0 &&
	    rt->length > 0) {	/* short pulses */
		PHY_RADAR(("\nShort pulses after use data from one of two antennas"));
		PHY_RADAR(("\n%d pulses, ", rt->length));

		phy_radar_output_pulses(rt->pulses, rt->length, rparams->radar_args.feature_mask);
	}
#endif /* BCMDBG */

	/* remove pulses spaced less than lower bype of "blank" */
	for (i = 1; i < rt->length; i++) {
		deltat = (int32)(rt->pulses[i].interval - rt->pulses[i-1].interval);
		if (deltat < (ISACPHY(pi) ? ((int32)rparams->radar_args.blank & 0xff) : 20)) {
			for (j = i; j < (rt->length - 1); j++) {
				rt->pulses[j] = rt->pulses[j + 1];
			}
			rt->length--;
		}
	}

#ifdef BCMDBG
	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_SHORT_PULSE) != 0 &&
	    rt->length > 0) {	/* short pulses */
		PHY_RADAR(("\nShort pulses after removing pulses with interval < %d",
		          rparams->radar_args.blank & 0xff));
		PHY_RADAR(("\n%d pulses, ", rt->length));

		phy_radar_output_pulses(rt->pulses, rt->length, rparams->radar_args.feature_mask);
	}
#endif /* BCMDBG */

	/*
	 * filter based on pulse width
	 */
	j = 0;
	for (i = 0; i < rt->length; i++) {
		if ((rt->pulses[i].pw >= rparams->radar_args.min_pw) &&
		    (rt->pulses[i].pw <= rparams->radar_args.max_pw)) {
			rt->pulses[j] = rt->pulses[i];
			j++;
		}
	}
	rt->length = j;

	if (ISACPHY(pi) && TONEDETECTION) {
		j = 0;
		for (i = 0; i < rt->length; i++) {
			if ((rt->pulses[i].fm >= -50)) {
				rt->pulses[j] = rt->pulses[i];
				j++;
			}
		}
		rt->length = j;
	}

	if (rt->length == 0)
		continue;

#ifdef BCMDBG
	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_SHORT_PULSE) != 0) {
		PHY_RADAR(("\nShort pulses after removing pulses with pw outside [%d, %d]",
		           rparams->radar_args.min_pw, rparams->radar_args.max_pw));
		PHY_RADAR((" or fm < -50"));
		PHY_RADAR(("\n%d pulses, ", rt->length));

		phy_radar_output_pulses(rt->pulses, rt->length, rparams->radar_args.feature_mask);
	}
#endif /* BCMDBG */

/*
	ASSERT(rt->length <= RDR_LIST_SIZE);
*/
	if (rt->length > RDR_LIST_SIZE) {
		rt->length = RDR_LIST_SIZE;
		PHY_RADAR(("WARNING: radar rt->length=%d > list_size=%d\n",
			rt->length, RDR_LIST_SIZE));
	}

	det_type = 0;

	/* convert pulses[].interval from tstamps to intervals */
	--rt->length;
	for (j = 0; j < rt->length; j++) {
		rt->pulses[j].interval = ABS((int32)(rt->pulses[j + 1].interval -
		                                     rt->pulses[j].interval));
	}

	wlc_phy_radar_detect_run_epoch(pi, rt, rparams, rt->length,
		&det_type, &nconsecq_pulses, &detected_pulse_index,
		&min_detected_pw, &max_detected_pw,
		&min_detected_interval, &max_detected_interval);

#ifdef BCMDBG
	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_INTV_PW) != 0) {
		PHY_RADAR(("\nShort pulses after pruning (filtering)"));

		phy_radar_output_pulses(rt->pulses, rt->length, rparams->radar_args.feature_mask);

		PHY_RADAR(("nconsecq_pulses=%d max_pw_delta=%d min_pw=%d max_pw=%d\n",
			nconsecq_pulses, max_detected_pw - min_detected_pw, min_detected_pw,
			max_detected_pw));
	}
#endif /* BCMDBG */

	if (min_detected_interval != 0 || det_type != 0) {
#ifdef BCMDBG
		PHY_RADAR(("\nPruned %d pulses, ", rt->length));

		PHY_RADAR(("\n(Interval, PW, FM) = [ "));
		for (i = 0; i < rt->length; i++) {
			PHY_RADAR(("(%u, %u, %d) ",
			           rt->pulses[i].interval, rt->pulses[i].pw, rt->pulses[i].fm));
		}
		PHY_RADAR(("];\n"));

		PHY_RADAR(("det_idx=%d pw_delta=%d min_pw=%d max_pw=%d\n",
				detected_pulse_index,
				max_detected_pw - min_detected_pw, min_detected_pw,
				max_detected_pw));
#endif /* BCMDBG */

		bzero(st->radar_status.intv, sizeof(st->radar_status.intv));
		bzero(st->radar_status.pw, sizeof(st->radar_status.pw));
		bzero(st->radar_status.fm, sizeof(st->radar_status.fm));

		for (i = 0; i < rt->length; i++) {
			if (i >= detected_pulse_index && i < detected_pulse_index + 10) {
				st->radar_status.intv[i - detected_pulse_index] =
					rt->pulses[i].interval;
				st->radar_status.pw[i - detected_pulse_index] = rt->pulses[i].pw;
				st->radar_status.fm[i - detected_pulse_index] = rt->pulses[i].fm;
			}
		}

		deltat2 = (uint32) (pi->sh->now - rt->last_detection_time);
		/* detection not valid if detected pulse index too large */
		if (detected_pulse_index < ((rparams->radar_args.ncontig) & 0x3f)) {
			rt->last_detection_time = pi->sh->now;
		}
		/* reject detection spaced more than 3 minutes and detected pulse index too larg */
		if (((uint32) deltat2 < (rparams->radar_args.fra_pulse_err & 0xff)*60 ||
			(st->first_radar_indicator == 1 && (uint32) deltat2 < 30*60)) &&
			(detected_pulse_index < ((rparams->radar_args.ncontig) & 0x3f))) {
			PHY_RADAR(("Type %d Radar Detection. Detected pulse index=%d"
				" nconsecq_pulses=%d."
				" Time from last detection = %u, = %dmin %dsec \n",
				det_type, detected_pulse_index, nconsecq_pulses,
				deltat2, deltat2/60, deltat2%60));
			st->radar_status.detected = TRUE;
			st->radar_status.count = st->radar_status.count + 1;
			st->radar_status.pretended = FALSE;
			st->radar_status.radartype = det_type;
			st->radar_status.timenow = (uint32) pi->sh->now;
			st->radar_status.timefromL = (uint32) deltat2;
			st->radar_status.detected_pulse_index =  detected_pulse_index;
			st->radar_status.nconsecq_pulses = nconsecq_pulses;
			st->radar_status.ch = pi->radio_chanspec;
			st->first_radar_indicator = 0;

			radar_detected->radar_type = det_type;
			radar_detected->min_pw = (uint16)min_detected_pw;	/* in 20 * usec */
			radar_detected->max_pw = (uint16)max_detected_pw;	/* in 20 * usec */
			radar_detected->min_pri =
				(uint16)((min_detected_interval + 10) / 20);	/* in usec */
			radar_detected->max_pri =
				(uint16)((max_detected_interval + 10) / 20);	/* in usec */

			return det_type;
		} else {
			if (rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_REJECTED_RADAR) {
				PHY_RADAR(("SKIPPED false Type %d Radar Detection."
					" min_pw=%d pw_delta=%d pri=%u"
					" nconsecq_pulses=%d. Time from last"
					" detection = %u, = %dmin %dsec",
					det_type, min_detected_pw,
					max_detected_pw - max_detected_pw,
					min_detected_interval, nconsecq_pulses, deltat2,
					deltat2 / 60, deltat2 % 60));
				if (detected_pulse_index < ((rparams->radar_args.ncontig) & 0x3f) -
					rparams->radar_args.npulses)
					PHY_RADAR((". Detected pulse index: %d\n",
						detected_pulse_index));
				else
					PHY_ERROR((". Detected pulse index too high: %d\n",
						detected_pulse_index));
			}
			break;
		}
	}
	}	/* end for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) */
	return (RADAR_TYPE_NONE);
}
