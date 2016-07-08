/*
 * ACPHY RSSI Compute module interface
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

#ifndef _phy_ac_rssi_h_
#define _phy_ac_rssi_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_rssi.h>

/* forward declaration */
typedef struct phy_ac_rssi_info phy_ac_rssi_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_rssi_info_t *phy_ac_rssi_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_rssi_info_t *ri);
void phy_ac_rssi_unregister_impl(phy_ac_rssi_info_t *info);

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
void phy_ac_rssi_init_gain_err(phy_ac_rssi_info_t *info);


#endif /* _phy_ac_rssi_h_ */
