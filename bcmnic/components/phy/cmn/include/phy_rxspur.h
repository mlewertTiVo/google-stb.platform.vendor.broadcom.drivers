/*
 * Rx Spur canceller module interface (to other PHY modules).
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
 * $Id: phy_rxspur.h 642720 2016-06-09 18:56:12Z $
 */

#ifndef _phy_rxspur_h_
#define _phy_rxspur_h_

#include <phy_api.h>

/* forward declaration */
typedef struct phy_rxspur_info phy_rxspur_info_t;

/* attach/detach */
phy_rxspur_info_t *phy_rxspur_attach(phy_info_t *pi);
void phy_rxspur_detach(phy_rxspur_info_t *cmn_info);

/* up/down */
int phy_rxspur_init(phy_rxspur_info_t *cmn_info);
int phy_rxspur_down(phy_rxspur_info_t *cmn_info);

/* force spurmode iovar functions */

#endif /* _phy_rxspur_h_ */
