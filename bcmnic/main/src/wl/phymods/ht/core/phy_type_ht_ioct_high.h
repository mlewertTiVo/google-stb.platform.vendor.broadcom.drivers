/*
 * HTPHY Core module interface (to PHY Core module).
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

#ifndef _phy_type_ht_ioct_high_h_
#define _phy_type_ht_ioct_high_h_

#include <wlc_iocv_types.h>

/* register HTPHY specific ioctl tables/handlers to system */
int phy_ht_high_register_ioct(wlc_iocv_info_t *ii);

#endif /* _phy_type_ht_ioct_high_h_ */
