/*
 * IOCV module implementation - ioctl/iovar registry switcher.
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
 * $Id: wlc_iocv_reg.c 614277 2016-01-21 20:28:50Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>

#include "wlc_iocv_if.h"
#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>

/* forward iovar table & callbacks registration to implementation */
int
BCMATTACHFN(wlc_iocv_register_iovt)(wlc_iocv_info_t *ii, wlc_iovt_desc_t *iovd)
{
	ASSERT(ii->iovt_reg_fn != NULL);

	return (ii->iovt_reg_fn)(ii, iovd);
}

/* forward ioctl table & callbacks registration implementation */
int
BCMATTACHFN(wlc_iocv_register_ioct)(wlc_iocv_info_t *ii, wlc_ioct_desc_t *iocd)
{
	ASSERT(ii->ioct_reg_fn != NULL);

	return (ii->ioct_reg_fn)(ii, iocd);
}
