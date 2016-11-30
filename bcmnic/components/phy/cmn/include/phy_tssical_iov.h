/*
 * TSSI CAL module internal interface - iovar table/handlers
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
 * $Id: phy_tssical_iov.h 632569 2016-04-19 20:52:56Z vyass $
 */

#ifndef _phy_tssical_iov_t_
#define _phy_tssical_iov_t_

#include <phy_api.h>

/* iovar ids */
enum {
	IOV_TSSIVISI_THRESH = 1
};

/* register iovar table/handlers */
int phy_tssical_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

#endif /* _phy_tssical_iov_t_ */
