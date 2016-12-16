/*
 * RadarDetect module internal interface (functions sharde by PHY type specific implementation).
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
 * $Id: phy_radar_utils.h 601782 2015-11-24 06:04:42Z $
 */

#ifndef _phy_radar_utils_h_
#define _phy_radar_utils_h_

#include <typedefs.h>

/* utilities */
int wlc_phy_radar_select_nfrequent(int *inlist, int length, int n, int *value,
	int *position, int *frequency, int *vlist, int *flist);

#endif /* _phy_radar_utils_h_ */
