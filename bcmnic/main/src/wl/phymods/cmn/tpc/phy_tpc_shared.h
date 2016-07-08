/*
 * TxPowerCtrl module internal interface (functions sharde by PHY type specific implementations).
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

#ifndef _phy_tpc_shared_h_
#define _phy_tpc_shared_h_

#include <typedefs.h>
#include <phy_api.h>

void phy_tpc_upd_shm(phy_info_t *pi);

#endif /* _phy_tpc_shared_h_ */
