/*
 * NPHY Core module implementation
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

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <wlc_iocv_types.h>
#include "phy_type_n_ioct.h"
#include "phy_type_n_ioct_high.h"

/* local functions */

/* register ioctl tables/handlers to IOC module */
int
BCMATTACHFN(phy_n_high_register_ioct)(wlc_iocv_info_t *ii)
{
	return phy_n_register_ioct(NULL, NULL, ii);
}
