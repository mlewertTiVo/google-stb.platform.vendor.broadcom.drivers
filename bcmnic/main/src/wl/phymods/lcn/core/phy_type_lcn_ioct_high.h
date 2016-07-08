/*
 * LCNPHY Core module interface (to PHY Core module).
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

#ifndef _phy_type_lcn_ioct_high_h_
#define _phy_type_lcn_ioct_high_h_

#include <wlc_iocv_types.h>

/* register LCNPHY specific ioctl tables/handlers to system */
int phy_lcn_high_register_ioct(wlc_iocv_info_t *ii);

#endif /* _phy_type_lcn_ioct_high_h_ */
