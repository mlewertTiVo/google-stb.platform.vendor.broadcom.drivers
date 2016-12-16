/*
 * VCO CAL module interface (to other PHY modules).
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
 * $Id: phy_vcocal.h 656120 2016-08-25 08:17:57Z $
 */

#ifndef _phy_vcocal_h_
#define _phy_vcocal_h_

#include <phy_api.h>

/* forward declaration */
typedef struct phy_vcocal_info phy_vcocal_info_t;

/* vco cal status */
#define RADIO2069X_VCOCAL_NOT_DONE	0
#define RADIO2069X_VCOCAL_IS_DONE	1
#define RADIO2069X_VCOCAL_NO_HWSUPPORT	2
#define RADIO2069X_VCOCAL_UNSUPPORTED_MODE	2

/* attach/detach */
phy_vcocal_info_t *phy_vcocal_attach(phy_info_t *pi);
void phy_vcocal_detach(phy_vcocal_info_t *cmn_info);
#if defined(RADIO_HEALTH_CHECK)
void phy_vcocal_force(phy_info_t *pi);
#endif 
int phy_vcocal_status(phy_vcocal_info_t *vcocali);

/* init */

#endif /* _phy_vcocal_h_ */
