/*
 * TOF module internal interface (to other PHY modules).
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
 * $Id: phy_tof.h 606042 2015-12-14 06:21:23Z jqliu $
 */
#ifndef _phy_tof_h_
#define _phy_tof_h_

#include <typedefs.h>
#include <phy_api.h>

/* forward declaration */
typedef struct phy_tof_info phy_tof_info_t;

/* ================== interface for attach/detach =================== */

/* attach/detach */
phy_tof_info_t *phy_tof_attach(phy_info_t *pi);
void phy_tof_detach(phy_tof_info_t *ti);

#endif /* _phy_tof_h_ */
