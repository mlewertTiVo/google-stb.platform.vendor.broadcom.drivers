/*
 * NPHY NOISE module interface (to other PHY modules).
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
 * $Id: phy_n_noise.h 659938 2016-09-16 16:47:54Z $
 */

#ifndef _phy_n_noise_h_
#define _phy_n_noise_h_

#include <phy_api.h>
#include <phy_n.h>
#include <phy_noise.h>

/* Noise Cal related */
#define NOISE_CAL_LCNXNPHY
#define M_LCNXN_BLK_PTR         (71 * 2)

/* forward declaration */
typedef struct phy_n_noise_info phy_n_noise_info_t;

/* register/unregister phy type specific implementation */
phy_n_noise_info_t *phy_n_noise_register_impl(phy_info_t *pi,
	phy_n_info_t *ai, phy_noise_info_t *ii);
void phy_n_noise_unregister_impl(phy_n_noise_info_t *info);

#endif /* _phy_n_noise_h_ */
