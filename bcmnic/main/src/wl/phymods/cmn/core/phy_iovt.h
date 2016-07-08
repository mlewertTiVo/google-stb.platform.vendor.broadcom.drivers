/*
 * PHY Core internal interface (to other modules) - IOVarTable registration.
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

#ifndef _phy_iovt_h_
#define _phy_iovt_h_

#include <phy_api.h>

#include <wlc_iocv_types.h>

/* register all modules' iovar tables/handlers */
int phy_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

#endif /* _phy_iovt_h_ */
