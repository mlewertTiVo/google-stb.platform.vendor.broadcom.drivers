/*
 * prephy modules interface (to other PHY modules).
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
 * $Id: phy_ac_prephy.h 657104 2016-08-31 03:30:45Z $
 */

#ifndef _phy_ac_prephy_h_
#define _phy_ac_prephy_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_prephy.h>


/* vasip version read without pi */
extern void
phy_ac_prephy_vasip_ver_get(prephy_info_t *pi, d11regs_t *regs, uint32 *vasipver);
/* vasip reset without pi */
extern void
phy_ac_prephy_vasip_proc_reset(prephy_info_t *pi, d11regs_t *regs, bool reset);
/* vasip clock set */
extern void
phy_ac_prephy_vasip_clk_set(prephy_info_t *pi, d11regs_t *regs, bool set);
/* getting phy cap */
extern uint32 phy_ac_prephy_caps(prephy_info_t *pi, uint32 *pacaps);
#endif /* _phy_ac_prephy_h_ */
