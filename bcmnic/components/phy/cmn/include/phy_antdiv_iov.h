/*
 * ANTennaDIVersity module internal interface - iovar table registration
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
 * $Id: phy_antdiv_iov.h 642720 2016-06-09 18:56:12Z vyass $
 */

#ifndef _phy_antdiv_iov_h_
#define _phy_antdiv_iov_h_

#include <phy_api.h>

/* register iovar table/handlers */
int phy_antdiv_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

#endif /* _phy_antdiv_iov_h_ */
