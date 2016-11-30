/*
 * ACPHY ANTennaDIVersity module interface (to other PHY modules).
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
 * $Id: phy_ac_antdiv.h 644039 2016-06-17 01:06:20Z kentryu $
 */

#ifndef _phy_ac_antdiv_h_
#define _phy_ac_antdiv_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_antdiv.h>
#include <phy_type_antdiv.h>

#define ANTDIV_RFSWCTRLPIN_UNDEFINED	255
#ifdef WLC_SW_DIVERSITY
#define ANTDIV_ANTMAP_NOCHANGE	(0xFF)	/* antmap is not specified */
#define ACPHY_PHYCORE_ANY	(0xFF)	/* any phycore is fine */
#endif /* WLC_SW_DIVERSITY */


/* forward declaration */
typedef struct phy_ac_antdiv_info phy_ac_antdiv_info_t;

/* register/unregister phy type specific implementation */
phy_ac_antdiv_info_t *phy_ac_antdiv_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_antdiv_info_t *di);
extern void phy_ac_antdiv_unregister_impl(phy_ac_antdiv_info_t *di);
extern bool wlc_phy_check_antdiv_enable_acphy(phy_info_t *pi);
extern void wlc_phy_antdiv_acphy(phy_info_t *pi, uint8 val);

#ifdef WLC_SW_DIVERSITY
void wlc_phy_swdiv_antmap_init(phy_type_antdiv_ctx_t *ctx);
void wlc_phy_set_femctrl_control_reg(phy_type_antdiv_ctx_t *ctx);
bool phy_ac_swdiv_is_supported(phy_type_antdiv_ctx_t *ctx, uint8 core, bool inanyband);
uint8 phy_ac_swdiv_get_rxant_bycoreidx(phy_type_antdiv_ctx_t *ctx, uint core);
#ifdef WLC_TXPWRCAP
void phy_ac_swdiv_txpwrcap_shmem_set(phy_type_antdiv_ctx_t *ctx,
	uint core, uint8 cap_tx, uint8 cap_rx, uint16 txantmap, uint16 rxantmap);
#endif /* WLC_TXPWRCAP */
#endif /* WLC_SW_DIVERSITY */
void wlc_phy_write_regtbl_fc_from_nvram(phy_info_t *pi);

#endif /* _phy_ac_antdiv_h_ */
