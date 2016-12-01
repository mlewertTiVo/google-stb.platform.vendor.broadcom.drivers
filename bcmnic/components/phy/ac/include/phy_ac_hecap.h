/*
 * ACPHY HE module interface
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */

#ifndef _phy_ac_hecap_h_
#define _phy_ac_hecap_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_type_hecap.h>

/* forward declaration */

typedef struct phy_ac_hecap_info phy_ac_hecap_info_t;

#ifdef WL11AX
/* register/unregister ACPHY specific implementations to/from common */
phy_ac_hecap_info_t *phy_ac_hecap_register_impl(phy_info_t *pi,
	phy_ac_info_t *ac_info, phy_hecap_info_t *ri);
void phy_ac_hecap_unregister_impl(phy_ac_hecap_info_t *info);

void wlc_phy_hecap_enable_acphy(phy_info_t *pi, bool enable);

/* External Declarations */
extern uint32 wlc_phy_ac_hecaps(phy_info_t *pi);
extern uint32 wlc_phy_ac_hecaps1(phy_info_t *pi);

#endif /* WL11AX */

#endif /* _phy_ac_hecap_h_ */
