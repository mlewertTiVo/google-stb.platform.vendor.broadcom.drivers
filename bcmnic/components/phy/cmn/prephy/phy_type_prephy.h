/*
 * Prephy module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_prephy.h 657104 2016-08-31 03:30:45Z $
 */

#ifndef _phy_type_prephy_h_
#define _phy_type_prephy_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_prephy.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_prephy_ctx_t;

typedef void (*phy_type_prephy_vasip_ver_get_t)(prephy_info_t *pi,
	d11regs_t *regs, uint32 *vasipver);
typedef void (*phy_type_prephy_vasip_proc_reset_t)(prephy_info_t *pi,
	d11regs_t *regs, bool reset);
typedef void (*phy_type_prephy_vasip_clk_set_t)(prephy_info_t *pi,
	d11regs_t *regs, bool set);
typedef uint32 (*phy_type_prephy_caps_t)(prephy_info_t *pi, uint32 *pacaps);

typedef struct {
	phy_type_prephy_ctx_t *ctx;
	phy_type_prephy_vasip_ver_get_t	phy_type_prephy_vasip_ver_get;
	phy_type_prephy_vasip_proc_reset_t phy_type_prephy_vasip_proc_reset;
	phy_type_prephy_vasip_clk_set_t phy_type_prephy_vasip_clk_set;
	phy_type_prephy_caps_t phy_type_prephy_caps;
} phy_type_prephy_fns_t;

/*
 * Register/unregister PHY type implementation to the MultiPhaseCal module.
 * It returns BCME_XXXX.
 */
int phy_prephy_register_impl(phy_prephy_info_t *mi, phy_type_prephy_fns_t *fns);
void phy_prephy_unregister_impl(phy_prephy_info_t *cmn_info);

#endif /* _phy_type_prephy_h_ */
