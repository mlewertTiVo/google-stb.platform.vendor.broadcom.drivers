/*
 * RadarDetect module internal interface (functions sharde by PHY type specific implementations).
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
 * $Id: phy_radar_shared.h 603346 2015-12-01 23:05:11Z chihap $
 */

#ifndef _phy_radar_shared_h_
#define _phy_radar_shared_h_

#include <typedefs.h>
#include <phy_api.h>
#include <phy_radar_api.h>

/* radar_args.feature_mask bitfields */
#define RADAR_FEATURE_DEBUG_SHORT_PULSE		(1 << 0) /* when bit-3 is on,
						          * 0=>bin5 data, 1=>short pulse data
						          */
#define RADAR_FEATURE_DEBUG_INPUT_PULSE_DATA	(1 << 1) /* output before-fitlered pulse data */
#define RADAR_FEATURE_DEBUG_PULSES_PER_ANT	(1 << 2) /* output # pulses at each antenna
						          * (if # pulse > 5)
						          */
#define RADAR_FEATURE_DEBUG_PULSE_DATA		(1 << 3) /* output radar pulses data */
#define RADAR_FEATURE_DEBUG_PW_CHECK_INFO	(1 << 4) /* output PW checking debug messages */
#define RADAR_FEATURE_DEBUG_STAGGERED_RESET	(1 << 5) /* output staggered reset */
#define RADAR_FEATURE_DEBUG_FIFO_OUTPUT		(1 << 6) /* output fifo output */
#define RADAR_FEATURE_DEBUG_INTV_PW		(1 << 7) /* output intervals and pruned pw */
#define RADAR_FEATURE_DEBUG_EU_TYPE		(1 << 9) /* output EU type debug messages */
#define RADAR_FEATURE_FCC_DETECT		(1 << 11) /* enable FCC radar detection */
#define RADAR_FEATURE_ETSI_DETECT		(1 << 12) /* enable ETSI radar detection */
#define RADAR_FEATURE_USE_MAX_PW		(1 << 13) /* for combining pulse use max of pw(i)
						           * and pw(i-1) inlieu of pw(i-1) + pw(i)
						           */
#define RADAR_FEATURE_DEBUG_REJECTED_RADAR	(1 << 14) /* output the skipped large intervals */

/*
 * Run the radar detect algorithm.
 */
uint8 phy_radar_run_nphy(phy_info_t *pi, radar_detected_info_t *radar_detected);

#endif /* _phy_radar_shared_h_ */
