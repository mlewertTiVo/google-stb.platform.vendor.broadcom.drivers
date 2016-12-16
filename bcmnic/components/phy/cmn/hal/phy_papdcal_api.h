/*
 * PAPD CAL PHY module public interface (to MAC driver).
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

#ifndef _phy_papdcal_api_h_
#define _phy_papdcal_api_h_

#include <phy_api.h>

#if defined(WLPKTENG)
bool wlc_phy_isperratedpden(wlc_phy_t *ppi);
void wlc_phy_perratedpdset(wlc_phy_t *ppi, bool enable);
#endif

#endif /* _phy_papdcal_api_h_ */
