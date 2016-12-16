/*
 * FCBS module interface (to other PHY modules).
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
 * $Id: phy_fcbs.h 612041 2016-01-12 22:04:08Z $
 */

#ifndef _phy_fcbs_h_
#define _phy_fcbs_h_

#include <phy_api.h>

/* Definition of FCBS macros that are used by AC specific layer and common layer */
#ifdef ENABLE_FCBS
#define IS_FCBS(pi) (((pi)->HW_FCBS) && ((pi)->FCBS))
#else
#define IS_FCBS(pi) 0
#endif /* ENABLE_FCBS */

/* Fast channel band switch (FCBS) structures and definitions */
#ifdef ENABLE_FCBS

/* FCBS cache id */
#define FCBS_CACHE_RADIOREG		1
#define FCBS_CACHE_PHYREG		2
#define FCBS_CACHE_PHYTBL16		3
#define FCBS_CACHE_PHYTBL32		4


/* Channel index of pseudo-simultaneous dual channels */
#define FCBS_CHAN_A	0
#define FCBS_CHAN_B 1

/* Flag to tell ucode to turn on/off BPHY core */
#define FCBS_BPHY_UPDATE		0x1
#define FCBS_BPHY_ON			(FCBS_BPHY_UPDATE | 0x2)
#define FCBS_BPHY_OFF			(FCBS_BPHY_UPDATE | 0x0)

/* FCBS TBL format */
/* bit shift for core info of radioregs */
#define FCBS_TBL_RADIOREG_CORE_SHIFT 0x9
/* ORed with the reg offset indicates an instruction */
#define FCBS_TBL_INST_INDICATOR 0x2000

#endif /* ENABLE_FCBS */

/* Number of pseudo-simultaneous channels that we support */
#define MAX_FCBS_CHANS	2

/* length of the HW FCBS TBL */
#define FCBS_HW_TBL_LEN 1024

/* forward declaration */
typedef struct phy_fcbs_info phy_fcbs_info_t;

/* attach/detach */
phy_fcbs_info_t *phy_fcbs_attach(phy_info_t *pi);
void phy_fcbs_detach(phy_fcbs_info_t *cmn_info);

/* up/down */
int phy_fcbs_init(phy_fcbs_info_t *cmn_info);

/* Intra-module API */
/* Fast Channel/Band Switch (FCBS) engine functions */
int phy_fcbs_preinit(phy_info_t *pi, int chanidx);
bool wlc_phy_is_fcbs_pending(phy_info_t *pi, chanspec_t chanspec, int *chanidx_ptr);
bool wlc_phy_is_fcbs_chan(phy_info_t *pi, chanspec_t chanspec, int *chanidx_ptr);
uint16 wlc_phy_channelindicator_obtain(phy_info_t *pi);
#endif /* _phy_fcbs_h_ */
