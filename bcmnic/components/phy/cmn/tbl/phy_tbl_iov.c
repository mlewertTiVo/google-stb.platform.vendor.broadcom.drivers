/*
 * PHYTblInit module implementation - iovar table/handlers
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
 * $Id: phy_tbl_iov.c 619527 2016-02-17 05:54:49Z $
 */

#include <phy_cfg.h>

#include <wlc_iocv_reg.h>

/* iovar table */
#include "phy_tbl_iov.h"

const bcm_iovar_t phy_tbl_iovars[] = {
#if defined(DBG_PHY_IOV)
	{"phytable", IOV_PHYTABLE, IOVF_GET_UP | IOVF_SET_UP | IOVF_MFG, 0, IOVT_BUFFER, 4*4},
#endif
	{NULL, 0, 0, 0, 0, 0}
};
