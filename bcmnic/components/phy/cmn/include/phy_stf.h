/*
 * STF phy module internal interface (to other PHY modules).
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
 * $Id: phy_stf.h 659995 2016-09-16 22:23:48Z $
 */

#ifndef _phy_stf_h_
#define _phy_stf_h_

#include <typedefs.h>
#include <phy_api.h>
#include <wlc_phy_shim.h>

/* forward declaration */
typedef struct phy_stf_info phy_stf_info_t;

typedef struct phy_stf_data {
	uint8	hw_phytxchain;		/* HW tx chain cfg */
	uint8	hw_phyrxchain;		/* HW rx chain cfg */
	uint8	phytxchain;			/* tx chain being used */
	uint8	phyrxchain;			/* rx chain being used */
} phy_stf_data_t;

/* attach/detach */
phy_stf_info_t* phy_stf_attach(phy_info_t *pi);
void phy_stf_detach(phy_stf_info_t *stf_info);

/* The below STF functions are called from PHY itself */
phy_stf_data_t *phy_stf_get_data(phy_stf_info_t *stfi);
int phy_stf_set_phyrxchain(phy_stf_info_t *stfi, uint8 phyrxchain);
int phy_stf_set_phytxchain(phy_stf_info_t *stfi, uint8 phytxchain);
int phy_stf_set_hwphyrxchain(phy_stf_info_t *stfi, uint8 hwphyrxchain);
int phy_stf_set_hwphytxchain(phy_stf_info_t *stfi, uint8 hwphytxchain);
#endif /* _phy_stf_h_ */
