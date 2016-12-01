/*
 * RADIO control module public interface (to MAC driver).
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
 * $Id: phy_radio_api.h 583048 2015-08-31 16:43:34Z jqliu $
 */

#ifndef _phy_radio_api_h_
#define _phy_radio_api_h_

#include <typedefs.h>
#include <phy_api.h>

/* switch the radio on/off */
void phy_radio_switch(phy_info_t *pi, bool on);

/* switch the radio off when switching band */
void phy_radio_xband(phy_info_t *pi);

/* switch the radio off when initializing */
void phy_radio_init(phy_info_t *pi);

#endif /* _phy_radio_api_h_ */
