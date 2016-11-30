/*
 * FCBS PHY module public interface (to MAC driver).
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
 * $Id: $
 */

#ifndef _phy_fcbs_api_h_
#define _phy_fcbs_api_h_

#include <phy_api.h>

/* Fast channel/band switch  (FCBS) function prototypes */
#ifdef ENABLE_FCBS
bool wlc_phy_fcbs_init(wlc_phy_t *ppi, int chanidx);
bool wlc_phy_fcbs_arm(wlc_phy_t *ppi, chanspec_t chanspec, int chanidx);
void wlc_phy_fcbs_exit(wlc_phy_t *ppi);
int wlc_phy_fcbs(wlc_phy_t *ppi, int chanidx, bool set);
bool wlc_phy_fcbs_uninit(wlc_phy_t *ppi, chanspec_t chanspec);
#endif /* ENABLE_FCBS */

#endif /* _phy_fcbs_api_h_ */
