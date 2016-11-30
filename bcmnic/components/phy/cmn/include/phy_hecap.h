/*
 * High Efficiency (HE) (802.11ax) phy module internal interface (to other PHY modules).
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
 * $Id: $
 */

#ifndef _phy_hecap_h_
#define _phy_hecap_h_

#include <typedefs.h>
#include <phy_api.h>


/* forward declaration */
typedef struct phy_hecap_info phy_hecap_info_t;

#ifdef WL11AX
/* attach/detach */
phy_hecap_info_t* phy_hecap_attach(phy_info_t *pi);
void phy_hecap_detach(phy_hecap_info_t *ri);
void wlc_phy_hecap_enable(phy_info_t *pi, bool enable);
#endif /* WL11AX */

#endif /* _phy_hecap_h_ */
