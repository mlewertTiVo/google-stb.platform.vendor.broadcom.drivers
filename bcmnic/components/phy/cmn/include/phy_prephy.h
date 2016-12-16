/*
 * prephy module interface (to other PHY modules).
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
 * $Id: phy_prephy.h 656026 2016-08-25 00:28:18Z $
 */

#ifndef _phy_prephy_h_
#define _phy_prephy_h_

#include <phy_api.h>
#include <phy_dbg.h>

/* forward declaration */
typedef struct phy_prephy_info phy_prephy_info_t;

/* attach/detach */
phy_prephy_info_t *phy_prephy_attach(prephy_info_t *pi);
void phy_prephy_detach(phy_prephy_info_t *cmn_info);

#endif /* _phy_prephy_h_ */
