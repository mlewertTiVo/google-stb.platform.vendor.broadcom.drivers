/*
 * HTPHY STF module interface
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

#ifndef _phy_ht_stf_h_
#define _phy_ht_stf_h_

#include <phy_api.h>
#include <phy_ht.h>
#include <phy_stf.h>

/* forward declaration */

typedef struct phy_ht_stf_info phy_ht_stf_info_t;

/* register/unregister HTPHY specific implementations to/from common */
phy_ht_stf_info_t *phy_ht_stf_register_impl(phy_info_t *pi,
	phy_ht_info_t *ht_info, phy_stf_info_t *stf_info);
void phy_ht_stf_unregister_impl(phy_ht_stf_info_t *info);

/* Functions being used by other HT modules */

#endif /* _phy_ht_stf_h_ */
