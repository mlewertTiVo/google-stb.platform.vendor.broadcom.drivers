/*
 * Rx Spur canceller module public interface (to MAC driver)
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
 * $Id: phy_rxspur_api.h 657044 2016-08-30 21:37:55Z $
 */

#ifndef _phy_rxspur_api_h_
#define _phy_rxspur_api_h_

#include <typedefs.h>
#include <phy_api.h>

void phy_rxspur_change_block_bbpll(wlc_phy_t *pih, bool block, bool going_down);

#endif /* _phy_rxspur_api_h_ */
