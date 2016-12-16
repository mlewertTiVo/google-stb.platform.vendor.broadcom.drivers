/*
 * Sample play module internal interface - iovar table registration
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
 * $Id: $
 */

#ifndef _phy_samp_iov_h_
#define _phy_samp_iov_h_

#include <phy_api.h>
/* iovar ids */

/* register iovar table/handlers */
int phy_samp_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

#endif /* _phy_rssi_iov_h_ */
