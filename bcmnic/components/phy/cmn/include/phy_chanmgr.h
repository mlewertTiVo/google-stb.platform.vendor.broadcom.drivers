/*
 * Channel manager internal interface (to other PHY modules).
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
 * $Id: phy_chanmgr.h 635757 2016-05-05 05:18:39Z vyass $
 */

#ifndef _phy_chanmgr_h_
#define _phy_chanmgr_h_

#include <typedefs.h>
#include <phy_api.h>

/* forward declaration */
typedef struct phy_chanmgr_info phy_chanmgr_info_t;

/* attach/detach */
phy_chanmgr_info_t *phy_chanmgr_attach(phy_info_t *pi);
void phy_chanmgr_detach(phy_chanmgr_info_t *ri);

int wlc_phy_chanspec_bandrange_get(phy_info_t*, chanspec_t);

#if defined(WLTEST)
int phy_chanmgr_get_smth(phy_info_t *pi, int32 *ret_int_ptr);
int phy_chanmgr_set_smth(phy_info_t *pi, int8 int_val);
#endif /* defined(WLTEST) */

#endif /* _phy_chanmgr_h_ */
