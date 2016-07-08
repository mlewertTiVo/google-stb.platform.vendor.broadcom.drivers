/*
 * RadarDetect module public interface (to MAC driver).
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#ifndef _phy_radar_api_h_
#define _phy_radar_api_h_

#include <typedefs.h>
#include <phy_api.h>

void phy_radar_detect_enable(phy_info_t *pi, bool on);
int phy_radar_detect_run(phy_info_t *pi, int PLL_idx);


typedef enum  phy_radar_detect_mode {
	RADAR_DETECT_MODE_FCC,
	RADAR_DETECT_MODE_EU,
	RADAR_DETECT_MODE_MAX
} phy_radar_detect_mode_t;

void phy_radar_detect_mode_set(phy_info_t *pi, phy_radar_detect_mode_t mode);

#endif /* _phy_radar_api_h_ */
