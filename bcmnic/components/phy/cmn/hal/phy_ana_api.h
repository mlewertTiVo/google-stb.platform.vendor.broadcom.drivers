/*
 * ANAcore control module public interface (to MAC driver).
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
 * $Id: phy_ana_api.h 583048 2015-08-31 16:43:34Z $
 */

#ifndef _phy_ana_api_h_
#define _phy_ana_api_h_

#include <typedefs.h>
#include <phy_api.h>

void phy_ana_switch(phy_info_t *pi, bool on);
void phy_ana_reset(phy_info_t *pi);

#endif /* _phy_ana_api_h_ */
