/*
 * HTPHY NOISE module interface (to other PHY modules).
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#ifndef _phy_ht_noise_h_
#define _phy_ht_noise_h_

#include <phy_api.h>
#include <phy_ht.h>
#include <phy_noise.h>

/* forward declaration */
typedef struct phy_ht_noise_info phy_ht_noise_info_t;

/* register/unregister HTPHY specific implementations to/from common */
phy_ht_noise_info_t *phy_ht_noise_register_impl(phy_info_t *pi,
	phy_ht_info_t *hti, phy_noise_info_t *ii);
void phy_ht_noise_unregister_impl(phy_ht_noise_info_t *info);

#endif /* _phy_ht_noise_h_ */
