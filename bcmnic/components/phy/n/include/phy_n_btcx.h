/*
 * NPHY BT Coex module interface (to other PHY modules).
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
 * $Id: phy_n_btcx.h 632569 2016-04-19 20:52:56Z $
 */

#ifndef _phy_n_btcx_h_
#define _phy_n_btcx_h_

#include <phy_api.h>
#include <phy_n.h>
#include <phy_btcx.h>

/* forward declaration */
typedef struct phy_n_btcx_info phy_n_btcx_info_t;

/* register/unregister NPHY specific implementations to/from common */
phy_n_btcx_info_t *phy_n_btcx_register_impl(phy_info_t *pi,
	phy_n_info_t *ni, phy_btcx_info_t *ci);
void phy_n_btcx_unregister_impl(phy_n_btcx_info_t *info);

void phy_n_btcx_init(phy_n_btcx_info_t *n_info);
#endif /* _phy_n_btcx_h_ */
