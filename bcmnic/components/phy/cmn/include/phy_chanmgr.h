/*
 * Channel manager internal interface (to other PHY modules).
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
 * $Id: phy_chanmgr.h 659938 2016-09-16 16:47:54Z $
 */

#ifndef _phy_chanmgr_h_
#define _phy_chanmgr_h_

#include <typedefs.h>
#include <phy_api.h>

#if defined(WLSRVSDB)
#define SR_MEMORY_BANK	2
#endif /* end WLSRVSDB */

/* forward declaration */
typedef struct phy_chanmgr_info phy_chanmgr_info_t;

/* attach/detach */
phy_chanmgr_info_t *phy_chanmgr_attach(phy_info_t *pi);
void phy_chanmgr_detach(phy_chanmgr_info_t *ri);

bool phy_chanmgr_skip_band_init(phy_chanmgr_info_t *chanmgri);
int wlc_phy_chanspec_bandrange_get(phy_info_t*, chanspec_t);


int phy_chanmgr_set_shm(phy_info_t *pi, chanspec_t chanspec);

/* VSDB, RVSDB Module realted declaration */
void phy_sr_vsdb_reset(wlc_phy_t *pi);
void phy_reset_srvsdb_engine(wlc_phy_t *pi);
#endif /* _phy_chanmgr_h_ */
