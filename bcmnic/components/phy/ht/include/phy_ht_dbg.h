/*
 * HTPHY Debug module interface
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
 * $Id: phy_ht_dbg.h 589186 2015-09-28 14:24:26Z jqliu $
 */

#ifndef _phy_ht_dbg_h_
#define _phy_ht_dbg_h_

#include <phy_api.h>
#include <phy_ht.h>
#include <phy_dbg.h>

/* forward declaration */
typedef struct phy_ht_dbg_info phy_ht_dbg_info_t;

/* register/unregister HTPHY specific implementations to/from common */
phy_ht_dbg_info_t *phy_ht_dbg_register_impl(phy_info_t *pi,
	phy_ht_info_t *hti, phy_dbg_info_t *di);
void phy_ht_dbg_unregister_impl(phy_ht_dbg_info_t *info);

#endif /* _phy_ht_dbg_h_ */
