/*
 * RadarDetect module internal interface (to other PHY modules).
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
 * $Id: phy_radar.h 583048 2015-08-31 16:43:34Z jqliu $
 */

#ifndef _phy_radar_h_
#define _phy_radar_h_

#include <typedefs.h>
#include <phy_api.h>

/* forward declaration */
typedef struct phy_radar_info phy_radar_info_t;

/* attach/detach */
phy_radar_info_t *phy_radar_attach(phy_info_t *pi);
void phy_radar_detach(phy_radar_info_t *ri);

/* update h/w */
void phy_radar_upd(phy_radar_info_t *ri);

void phy_radar_first_indicator_set(phy_radar_info_t *ri);

#endif /* _phy_radar_h_ */
