/*
 * LCN40PHY PHYTableInit module interface
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

#ifndef _phy_lcn40_tbl_h_
#define _phy_lcn40_tbl_h_

#include <phy_api.h>
#include <phy_lcn40.h>
#include <phy_tbl.h>

/* forward declaration */
typedef struct phy_lcn40_tbl_info phy_lcn40_tbl_info_t;

/* register/unregister LCN40PHY specific implementations to/from common */
phy_lcn40_tbl_info_t *phy_lcn40_tbl_register_impl(phy_info_t *pi,
	phy_lcn40_info_t *lcn40i, phy_tbl_info_t *ii);
void phy_lcn40_tbl_unregister_impl(phy_lcn40_tbl_info_t *info);

#endif /* _phy_lcn40_tbl_h_ */
