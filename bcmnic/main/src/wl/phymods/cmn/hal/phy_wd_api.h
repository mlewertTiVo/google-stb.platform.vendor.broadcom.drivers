/*
 * WatchDog module public interface (to MAC driver).
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

#ifndef _phy_wd_api_h_
#define _phy_wd_api_h_

#include <typedefs.h>
#include <phy_api.h>

int phy_watchdog(phy_info_t *pi);

#endif /* _phy_wd_api_h_ */
