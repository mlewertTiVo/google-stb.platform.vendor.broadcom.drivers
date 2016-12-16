/*
 * VASIP module interface (to other PHY modules).
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
 * $Id: phy_vasip.h 659421 2016-09-14 06:45:22Z $
 */

#ifndef _phy_vasip_h_
#define _phy_vasip_h_

#include <phy_api.h>

/* forward declaration */
typedef struct phy_vasip_info phy_vasip_info_t;

/* attach/detach */
phy_vasip_info_t *phy_vasip_attach(phy_info_t *pi);
void phy_vasip_detach(phy_vasip_info_t *cmn_info);
#endif /* _phy_vasip_h_ */
