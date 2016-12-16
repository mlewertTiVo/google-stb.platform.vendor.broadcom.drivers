/*
 * OCL module public interface (to MAC driver).
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: phy_stf_api.h 659995 2016-09-16 22:23:48Z $
 */

#ifndef _phy_stf_api_h_
#define _phy_stf_api_h_

#include <typedefs.h>
#include <phy_api.h>

int phy_stf_chain_set(wlc_phy_t *pih, uint8 txchain, uint8 rxchain);
int phy_stf_chain_init(wlc_phy_t *pih, uint8 txchain, uint8 rxchain);
int phy_stf_chain_get(wlc_phy_t *pih, uint8 *txchain, uint8 *rxchain);
uint16 phy_stf_duty_cycle_chain_active_get(wlc_phy_t *pih);
#endif /* _phy_stf_api_h_ */
