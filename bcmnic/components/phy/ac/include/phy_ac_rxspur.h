/*
 * ACPHY Rx Spur canceller module interface (to other PHY modules).
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
 * $Id: phy_ac_rxspur.h 657044 2016-08-30 21:37:55Z $
 */

#ifndef _phy_ac_rxspur_h_
#define _phy_ac_rxspur_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_rxspur.h>

/* forward declaration */
typedef struct phy_ac_rxspur_info phy_ac_rxspur_info_t;
typedef struct acphy_dssf_values acphy_dssf_values_t;
typedef struct acphy_dssfB_values acphy_dssfB_values_t;
typedef struct acphy_spurcan_values acphy_spurcan_values_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_rxspur_info_t
*phy_ac_rxspur_register_impl(phy_info_t *pi, phy_ac_info_t *aci, phy_rxspur_info_t *cmn_info);
void phy_ac_rxspur_unregister_impl(phy_ac_rxspur_info_t *ac_info);

/* ************************************************************************* */

#define ACPHY_SPURWAR_NTONES	8 /* Numver of tones for spurwar */
/* Number of tones(spurwar+nvshp) to be written */
#define ACPHY_SPURWAR_NV_NTONES	32

/* ************************************************************************* */

extern void
phy_ac_spurwar(phy_ac_rxspur_info_t *rxspuri, uint8 noise_var[][ACPHY_SPURWAR_NV_NTONES],
                   int8 *tone_id, uint8 *core_sp);
extern void phy_ac_dssf(phy_ac_rxspur_info_t *rxspuri, bool on);
extern void phy_ac_dssfB(phy_ac_rxspur_info_t *rxspuri, bool on);
extern void phy_ac_spurcan(phy_ac_rxspur_info_t *rxspuri, bool enable);
void chanspec_bbpll_parr(phy_ac_rxspur_info_t *rxspuri, uint32 *bbpll_parr_in, bool state);

#endif /* _phy_ac_rxspur_h_ */
