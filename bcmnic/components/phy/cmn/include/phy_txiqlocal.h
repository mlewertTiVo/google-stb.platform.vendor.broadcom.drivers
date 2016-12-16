/*
 * TXIQLO CAL module interface (to other PHY modules).
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
 * $Id: phy_txiqlocal.h 633369 2016-04-22 08:36:32Z $
 */

#ifndef _phy_txiqlocal_h_
#define _phy_txiqlocal_h_

#include <phy_api.h>

#define CAL_COEFF_READ    0
#define CAL_COEFF_WRITE   1
#define IQTBL_CACHE_COOKIE_OFFSET	95
#define TXCAL_CACHE_VALID		0xACDC
#define RXCAL_CACHE_VALID		TXCAL_CACHE_VALID /* [PHY_REARCH] New value may be added */

/* defs for iqlo cal */
enum {  /* mode selection for reading/writing tx iqlo cal coefficients */
	TB_START_COEFFS_AB, TB_START_COEFFS_D, TB_START_COEFFS_E, TB_START_COEFFS_F,
	TB_BEST_COEFFS_AB,  TB_BEST_COEFFS_D,  TB_BEST_COEFFS_E,  TB_BEST_COEFFS_F,
	TB_OFDM_COEFFS_AB,  TB_OFDM_COEFFS_D,  TB_BPHY_COEFFS_AB,  TB_BPHY_COEFFS_D,
	PI_INTER_COEFFS_AB, PI_INTER_COEFFS_D, PI_INTER_COEFFS_E, PI_INTER_COEFFS_F,
	PI_FINAL_COEFFS_AB, PI_FINAL_COEFFS_D, PI_FINAL_COEFFS_E, PI_FINAL_COEFFS_F
};

/* forward declaration */
typedef struct phy_txiqlocal_info phy_txiqlocal_info_t;

/* attach/detach */
phy_txiqlocal_info_t *phy_txiqlocal_attach(phy_info_t *pi);
void phy_txiqlocal_detach(phy_txiqlocal_info_t *cmn_info);

/* init */

/* phy txcal coeffs structure */
typedef struct txcal_coeffs {
	uint16 txa;
	uint16 txb;
	uint16 txd;	/* contain di & dq */
	uint8 txei;
	uint8 txeq;
	uint8 txfi;
	uint8 txfq;
	uint16 rxa;
	uint16 rxb;
} txcal_coeffs_t;

/* Intra-module API for IOVAR */
void phy_txiqlocal_txiqccget(phy_info_t *pi, void *a);
void phy_txiqlocal_txiqccset(phy_info_t *pi, void *p);
void phy_txiqlocal_txloccget(phy_info_t *pi, void *a);
void phy_txiqlocal_txloccset(phy_info_t *pi, void *p);

void phy_txiqlocal_scanroam_cache(phy_info_t *pi, bool set);
#endif /* _phy_txiqlocal_h_ */
