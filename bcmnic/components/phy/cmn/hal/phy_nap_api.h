/*
 * Napping module public interface (to MAC driver).
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
 * $Id: phy_nap_api.h 630449 2016-04-09 00:27:18Z $
 */

#ifndef _phy_nap_api_h_
#define _phy_nap_api_h_

#include <typedefs.h>
#include <phy_api.h>

#ifdef WL_NAP
void wlc_phy_nap_status_get(wlc_phy_t *ppi, uint16 *reqs, bool *nap_en);
void wlc_phy_nap_disable_req_set(wlc_phy_t *ppi, uint16 req, bool disable,
		bool agc_reconfig, uint8 req_id);
#endif /* WL_NAP */

#endif /* _phy_nap_api_h_ */
