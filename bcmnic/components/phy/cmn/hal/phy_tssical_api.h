/*
 * TSSI Cal module public interface (to MAC driver).
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
 * $Id: phy_tssical_api.h 610412 2016-01-06 23:43:14Z vyass $
 */

#ifndef _phy_tssical_api_h_
#define _phy_tssical_api_h_

#include <typedefs.h>
#include <phy_api.h>

int wlc_phy_tssivisible_thresh(wlc_phy_t *ppi);
void wlc_phy_get_tssi_sens_min(wlc_phy_t *ppi, int8 *tssiSensMinPwr);

#endif /* _phy_tssical_api_h_ */
