/*
 * Napping module interface (to other PHY modules).
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
 * $Id$
 */

#ifndef _phy_nap_h_
#define _phy_nap_h_

#include <phy_api.h>

/* forward declaration */
typedef struct phy_nap_info phy_nap_info_t;

/* attach/detach */
phy_nap_info_t *phy_nap_attach(phy_info_t *pi);
void phy_nap_detach(phy_nap_info_t *cmn_info);

#endif /* _phy_nap_h_ */
