/*
 * Envelope Tracking module internal interface (to other PHY modules).
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
 * $Id: phy_et.h 623369 2016-03-07 20:07:03Z vyass $
 */

#ifndef _phy_et_h_
#define _phy_et_h_

#include <typedefs.h>
#include <phy_api.h>

/* forward declaration */
typedef struct phy_et_info phy_et_info_t;

/* attach/detach */
phy_et_info_t *phy_et_attach(phy_info_t *pi);
void phy_et_detach(phy_et_info_t *info);

#endif /* _phy_et_h_ */
