/*
 * NPHY Rx Spur canceller module interface (to other PHY modules).
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
 * $Id: phy_n_rxspur.h 657044 2016-08-30 21:37:55Z $
 */

#ifndef _phy_n_rxspur_h_
#define _phy_n_rxspur_h_

#include <phy_api.h>
#include <phy_n.h>
#include <phy_rxspur.h>
#include <phy_type_rxspur.h>

/* forward declaration */
typedef struct phy_n_rxspur_info phy_n_rxspur_info_t;

/* register/unregister NPHY specific implementations to/from common */
phy_n_rxspur_info_t *phy_n_rxspur_register_impl(phy_info_t *pi,
	phy_n_info_t *ni, phy_rxspur_info_t *cmn_info);
void phy_n_rxspur_unregister_impl(phy_n_rxspur_info_t *n_info);

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

#endif /* _phy_n_rxspur_h_ */
