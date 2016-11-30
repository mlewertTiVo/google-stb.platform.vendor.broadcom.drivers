/*
 * lcn20PHY Core module internal interface
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
 * $Id: phy_type_lcn20_ioct.h 583048 2015-08-31 16:43:34Z jqliu $
 */

#ifndef _phy_lcn20_ioct_h_
#define _phy_lcn20_ioct_h_

#include <phy_api.h>
#include "phy_type.h"
#include <wlc_iocv_types.h>

/* register lcn20PHY specific ioctl tables/handlers to system */
int phy_lcn20_register_ioct(phy_info_t *pi, phy_type_info_t *ti, wlc_iocv_info_t *ii);

#endif /* _phy_lcn20_ioct_h_ */
