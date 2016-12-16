/*
 * PAPD CAL module internal interface - iovar table registration
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
 * $Id: phy_papdcal_iov.h 639713 2016-05-24 18:02:57Z $
 */

#ifndef _phy_papdcal_iov_t_
#define _phy_papdcal_iov_t_

#include <phy_api.h>

/* iovar ids */
enum {
	IOV_PHY_ENABLE_EPA_DPD_2G = 1,
	IOV_PHY_ENABLE_EPA_DPD_5G = 2,
	IOV_PHY_PACALIDX0 = 3,
	IOV_PHY_PACALIDX1 = 4,
	IOV_PHY_PACALIDX = 5,
	IOV_PAPD_EN_WAR = 6,
	IOV_PHY_SKIPPAPD = 7,
	IOV_PHY_EPACAL2GMASK = 8,
	IOV_PHY_WFD_LL_ENABLE = 9
};

/* register iovar table/handlers */
int phy_papdcal_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

#endif /* _phy_papdcal_iov_t_ */
