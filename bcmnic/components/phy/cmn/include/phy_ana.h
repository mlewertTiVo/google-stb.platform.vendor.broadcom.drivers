/*
 * ANAcore control module internal interface (to other PHY modules).
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
 * $Id: phy_ana.h 583048 2015-08-31 16:43:34Z $
 */

#ifndef _phy_ana_h_
#define _phy_ana_h_

#include <typedefs.h>
#include <phy_api.h>

/* forward declaration */
typedef struct phy_ana_info phy_ana_info_t;

/* attach/detach */
phy_ana_info_t *phy_ana_attach(phy_info_t *pi);
void phy_ana_detach(phy_ana_info_t *ri);

#endif /* _phy_ana_h_ */
