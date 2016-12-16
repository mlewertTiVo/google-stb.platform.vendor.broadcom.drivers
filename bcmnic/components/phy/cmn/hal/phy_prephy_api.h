/*
 * Prephy PHY module public interface (to MAC driver).
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
 * $Id: phy_prephy_api.h 658179 2016-09-06 21:33:14Z $
 */

#ifndef _phy_prephy_api_h_
#define _phy_prephy_api_h_

#include <phy_api.h>

#ifndef VASIP_NOVERSION
#define VASIP_NOVERSION 0xFF
#endif

/* vasip version read without pi */
void phy_prephy_vasip_ver_get(prephy_info_t *pi, d11regs_t *regs, uint32 *vasipver);
/* vasip reset without pi */
void phy_prephy_vasip_proc_reset(prephy_info_t *pi, d11regs_t *regs, bool reset);
/* vasip clk set without pi */
void phy_prephy_vasip_clk_set(prephy_info_t *pi, d11regs_t *regs, bool set);
/* get preattach phy caps */
uint32 phy_prephy_phy_caps(prephy_info_t *pi, uint32 *caps);

#endif /* _phy_prephy_api_h_ */
