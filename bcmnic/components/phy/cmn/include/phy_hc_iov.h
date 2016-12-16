/*
 * Health check module.
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
 * $Id: phy_hc_iov.h 656120 2016-08-25 08:17:57Z $
 */
#ifndef _phy_hc_iov_t_
#define _phy_hc_iov_t_

#include <phy_api.h>

/* register iovar table/handlers */
int phy_hc_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

#endif /* _phy_hc_iov_t_ */
