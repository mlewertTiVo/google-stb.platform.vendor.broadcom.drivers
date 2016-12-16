/*
 * PMU control module internal interface - shared by PHY type specific implementations.
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
 * $Id: phy_utils_pmu.h 659421 2016-09-14 06:45:22Z $
 */

#ifndef _phy_utils_pmu_h_
#define _phy_utils_pmu_h_

#include <typedefs.h>
#include <phy_api.h>

void phy_utils_pmu_regcontrol_access(phy_info_t *pi, uint8 addr, uint32* val, bool write);
void phy_utils_pmu_chipcontrol_access(phy_info_t *pi, uint8 addr, uint32* val, bool write);
#endif /* _phy_utils_pmu_h_ */
