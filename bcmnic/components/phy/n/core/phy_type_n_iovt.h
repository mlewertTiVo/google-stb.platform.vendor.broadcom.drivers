/*
 * NPHY Core module internal interface
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
 * $Id: phy_type_n_iovt.h 583048 2015-08-31 16:43:34Z $
 */

#ifndef _phy_n_iovt_h_
#define _phy_n_iovt_h_

#include <phy_api.h>
#include "phy_type.h"
#include <wlc_iocv_types.h>

/* register NPHY specific iovar tables/handlers to system */
int phy_n_register_iovt(phy_info_t *pi, phy_type_info_t *ti, wlc_iocv_info_t *ii);

#endif /* _phy_n_iovt_h_ */
