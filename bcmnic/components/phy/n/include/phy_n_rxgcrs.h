/*
 * NPHY Rx Gain Control and Carrier Sense module interface (to other PHY modules).
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
 * $Id: phy_n_rxgcrs.h 657373 2016-09-01 01:08:38Z $
 */

#ifndef _phy_n_rxgcrs_h_
#define _phy_n_rxgcrs_h_

#include <phy_api.h>
#include <phy_n.h>
#include <phy_rxgcrs.h>
#include <phy_type_rxgcrs.h>

/* forward declaration */
typedef struct phy_n_rxgcrs_info phy_n_rxgcrs_info_t;

/* register/unregister NPHY specific implementations to/from common */
phy_n_rxgcrs_info_t *phy_n_rxgcrs_register_impl(phy_info_t *pi,
	phy_n_info_t *ni, phy_rxgcrs_info_t *cmn_info);
void phy_n_rxgcrs_unregister_impl(phy_n_rxgcrs_info_t *n_info);
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

#endif /* _phy_n_rxgcrs_h_ */
