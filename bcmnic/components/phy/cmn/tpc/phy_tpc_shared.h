/*
 * TxPowerCtrl module internal interface (functions sharde by PHY type specific implementations).
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
 * $Id: phy_tpc_shared.h 583048 2015-08-31 16:43:34Z jqliu $
 */

#ifndef _phy_tpc_shared_h_
#define _phy_tpc_shared_h_

#include <typedefs.h>
#include <phy_api.h>

void phy_tpc_upd_shm(phy_info_t *pi);

#endif /* _phy_tpc_shared_h_ */
