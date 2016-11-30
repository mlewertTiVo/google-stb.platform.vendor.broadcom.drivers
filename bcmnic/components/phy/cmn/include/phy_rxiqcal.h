/*
 * RXIQ CAL module interface (to other PHY modules).
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
 * $Id: phy_rxiqcal.h 624514 2016-03-11 23:15:58Z vyass $
 */

#ifndef _phy_rxiqcal_h_
#define _phy_rxiqcal_h_

#include <phy_api.h>

/* forward declaration */
typedef struct phy_rxiqcal_info phy_rxiqcal_info_t;

/* Definitions shared by different modules */
typedef struct _phy_iq_comp {
	int16  a;
	int16  b;
} phy_iq_comp_t;

typedef struct {
	uint32 iq_prod;
	uint32 i_pwr;
	uint32 q_pwr;
} phy_iq_est_t;

/* attach/detach */
phy_rxiqcal_info_t *phy_rxiqcal_attach(phy_info_t *pi);
void phy_rxiqcal_detach(phy_rxiqcal_info_t *cmn_info);

/* init */
/* phy rxcal coeffs structure */
typedef struct rxcal_coeffs {
	uint16 rxa;
	uint16 rxb;
} rxcal_coeffs_t;

void phy_rxiqcal_scanroam_cache(phy_info_t *pi, bool set);
#endif /* _phy_rxiqcal_h_ */
