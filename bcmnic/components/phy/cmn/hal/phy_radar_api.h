/*
 * RadarDetect module public interface (to MAC driver).
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
 * $Id: phy_radar_api.h 604347 2015-12-07 00:05:52Z chihap $
 */

#ifndef _phy_radar_api_h_
#define _phy_radar_api_h_

#include <typedefs.h>
#include <phy_api.h>

/* Note: uncomment the commented-out Radar Types once the above are removed */
#define RADAR_TYPE_NONE		0   /* Radar type None */
#define RADAR_TYPE_ETSI_0	1   /* ETSI 0: PW 1us, PRI 1429us */
#define RADAR_TYPE_ETSI_1	2   /* ETSI 1: PW 0.5-5us, PRI 1000-5000us */
#define RADAR_TYPE_ETSI_2	3   /* ETSI 2: PW 0.5-15us, PRI 625-5000us */
#define RADAR_TYPE_ETSI_3	4   /* ETSI 3: PW 0.5-15us, PRI 250-435us */
#define RADAR_TYPE_ETSI_4	5   /* ETSI 4: PW 20-30us, PRI 250-500us */
#define RADAR_TYPE_ETSI_5_STG2	6   /* staggered-2 ETSI 5: PW 0.5-2us, PRI 2500-3333us */
#define RADAR_TYPE_ETSI_5_STG3	7   /* staggered-3 ETSI 5: PW 0.5-2us, PRI 2500-3333us */
#define RADAR_TYPE_ETSI_6_STG2	8   /* staggered-2 ETSI 6: PW 0.5-2us, PRI 833-2500us */
#define RADAR_TYPE_ETSI_6_STG3	9   /* staggered-3 ETSI 6: PW 0.5-2us, PRI 833-2500us */
#define RADAR_TYPE_ETSI_STG2	RADAR_TYPE_ETSI_5_STG2 /* staggered-2: PW 0.5-2us, PRI 833-3333us */
#define RADAR_TYPE_ETSI_STG3	RADAR_TYPE_ETSI_5_STG3 /* staggered-3: PW 0.5-2us, PRI 833-3333us */
#define RADAR_TYPE_FCC_0	10  /* FCC 0: PW 1us, PRI 1428us */
#define RADAR_TYPE_FCC_1	11  /* FCC 1: PW 1us, PRI 518-3066us */
#define RADAR_TYPE_FCC_2	12  /* FCC 2: PW 1-5us, PRI 150-230us */
#define RADAR_TYPE_FCC_3	13  /* FCC 3: PW 6-10us, PRI 200-500us */
#define RADAR_TYPE_FCC_4	14  /* FCC 4: PW 11-20us, PRI 200-500us */
#define RADAR_TYPE_FCC_5	15  /* FCC 5: PW 50-100us, PRI 1000-2000us */
#define RADAR_TYPE_FCC_6	16  /* FCC 6: PW 1us, PRI 333us */
#define RADAR_TYPE_JP1_1	17  /* Japan 1.1: same as FCC 0 */
#define RADAR_TYPE_JP1_2	18  /* Japan 1.2: PW 2.5us, PRI 3846us */
#define RADAR_TYPE_JP2_1_1	19  /* Japan 2.1.1: PW 0.5us, PRI 1389us */
#define RADAR_TYPE_JP2_1_2	20  /* Japan 2.1.2: PW 2us, PRI 4000us */
#define RADAR_TYPE_JP2_2_1	21  /* Japan 2.2.1: same as FCC 0 */
#define RADAR_TYPE_JP2_2_2	22  /* Japan 2.2.2: same as FCC 2 */
#define RADAR_TYPE_JP2_2_3	23  /* Japan 2.2.3: PW 6-10us, PRI 250-500us (similar to FCC 3) */
#define RADAR_TYPE_JP2_2_4	24  /* Japan 2.2.4: PW 11-20us, PRI 250-500us (similar to FCC 4) */
#define RADAR_TYPE_JP3		25  /* Japan 3: same as FCC 5 */
#define RADAR_TYPE_JP4		26  /* Japan 4: same as FCC 6 */
#define RADAR_TYPE_KN1		27  /* Korean 1: same as FCC 0 */
#define RADAR_TYPE_KN2		28  /* Korean 2: PW 1us, PRI 556us */
#define RADAR_TYPE_KN3		29  /* Korean 3: PW 2us, PRI 3030us */
#define RADAR_TYPE_KN4		30  /* Korean 4: PW 1us, PRI 333us (similar to JP 4) */
#define RADAR_TYPE_UNCLASSIFIED 255 /* Unclassified Radar type */

typedef struct {
	uint8 radar_type;	/* one of RADAR_TYPE_XXX */
	uint16 min_pw;		/* minimum pulse-width (usec * 20) */
	uint16 max_pw;		/* maximum pulse-width (usec * 20) */
	uint16 min_pri;		/* minimum pulse repetition interval (usec) */
	uint16 max_pri;		/* maximum pulse repetition interval (usec) */
	uint16 subband;		/* subband/frequency */
} radar_detected_info_t;

void phy_radar_detect_enable(phy_info_t *pi, bool on);

uint8 phy_radar_detect(phy_info_t *pi, radar_detected_info_t *radar_detected);

typedef enum  phy_radar_detect_mode {
	RADAR_DETECT_MODE_FCC,
	RADAR_DETECT_MODE_EU,
	RADAR_DETECT_MODE_MAX
} phy_radar_detect_mode_t;

void phy_radar_detect_mode_set(phy_info_t *pi, phy_radar_detect_mode_t mode);

#endif /* _phy_radar_api_h_ */
