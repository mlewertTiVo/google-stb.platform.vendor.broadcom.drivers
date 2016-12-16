/*
 * N PHY PAPD CAL module interface (to other PHY modules).
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
 * $Id: phy_n_papdcal.h 583048 2015-08-31 16:43:34Z $
 */

#ifndef _phy_n_papdcal_h_
#define _phy_n_papdcal_h_

#include <phy_api.h>
#include <phy_n.h>
#include <phy_papdcal.h>

/* forward declaration */
typedef struct phy_n_papdcal_info phy_n_papdcal_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_n_papdcal_info_t *phy_n_papdcal_register_impl(phy_info_t *pi,
	phy_n_info_t *aci, phy_papdcal_info_t *mi);
void phy_n_papdcal_unregister_impl(phy_n_papdcal_info_t *info);

#endif /* _phy_n_papdcal_h_ */
