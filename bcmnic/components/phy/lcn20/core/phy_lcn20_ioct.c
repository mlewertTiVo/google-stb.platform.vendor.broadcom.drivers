/*
 * LCN20PHY Core module implementation
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
 * $Id: phy_lcn20_ioct.c 583048 2015-08-31 16:43:34Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <wlc_iocv_types.h>
#include "phy_type_lcn20.h"
#include "phy_type_lcn20_ioct.h"

/* local functions */

/* register ioctl tables/handlers to IOC module */
int
BCMATTACHFN(phy_lcn20_register_ioct)(phy_info_t *pi, phy_type_info_t *ti, wlc_iocv_info_t *ii)
{
	return BCME_OK;
}
