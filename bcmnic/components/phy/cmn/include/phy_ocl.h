/*
 * OCL phy module internal interface (to other PHY modules).
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
 * $Id: $
 */

#ifndef _phy_ocl_h_
#define _phy_ocl_h_

#include <typedefs.h>
#include <phy_api.h>


/* forward declaration */
typedef struct phy_ocl_info phy_ocl_info_t;

#ifdef OCL
/* attach/detach */
phy_ocl_info_t* phy_ocl_attach(phy_info_t *pi);
void phy_ocl_detach(phy_ocl_info_t *ri);
void wlc_phy_ocl_enable(phy_info_t *pi, bool enable);
#endif /* OCL */

#endif /* _phy_ocl_h_ */
