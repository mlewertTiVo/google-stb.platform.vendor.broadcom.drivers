/*
 * ACPHY Calibration Manager module interface (to other PHY modules).
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
 * $Id: phy_ac_calmgr.h 666231 2016-10-20 09:42:37Z $
 */

#ifndef _phy_ac_calmgr_h_
#define _phy_ac_calmgr_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_calmgr.h>

#include <phy_utils_math.h>
#include <phy_txiqlocal.h>


/* forward declaration */
typedef struct phy_ac_calmgr_info phy_ac_calmgr_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_calmgr_info_t *phy_ac_calmgr_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_calmgr_info_t *cmn_info);
void phy_ac_calmgr_unregister_impl(phy_ac_calmgr_info_t *info);

/* Inter-module data API */
bool phy_ac_calmgr_get_enIndxCap(phy_ac_calmgr_info_t *calmgri);
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* mphase phases for ACPHY */
enum {
	ACPHY_CAL_PHASE_IDLE = 0,
	ACPHY_CAL_PHASE_INIT = 1,
	ACPHY_CAL_PHASE_TX0,
	ACPHY_CAL_PHASE_TX1,
	ACPHY_CAL_PHASE_TX2,
	ACPHY_CAL_PHASE_TX3,
	ACPHY_CAL_PHASE_TX4,
	ACPHY_CAL_PHASE_TX5,
	ACPHY_CAL_PHASE_TX6,
	ACPHY_CAL_PHASE_TX7,
	ACPHY_CAL_PHASE_TX8,
	ACPHY_CAL_PHASE_TX9,
	ACPHY_CAL_PHASE_TX_LAST,
	ACPHY_CAL_PHASE_PAPDCAL,	/* IPA */
	ACPHY_CAL_PHASE_TXPRERXCAL0,	/* bypass Biq2 pre rx cal */
	ACPHY_CAL_PHASE_TXPRERXCAL1,	/* bypass Biq2 pre rx cal */
	ACPHY_CAL_PHASE_TXPRERXCAL2,	/* bypass Biq2 pre rx cal */
	ACPHY_CAL_PHASE_RXCAL,
	ACPHY_CAL_PHASE_RSSICAL,
	ACPHY_CAL_PHASE_IDLETSSI
};

void phy_ac_calmgr_clean(phy_info_t *pi, bool *suspend);
void phy_ac_calmgr_singleshot(phy_info_t *pi, uint8 searchmode, acphy_cal_result_t *accal);
void phy_ac_calmgr_multiphase(phy_info_t *pi, uint8 phase_id, uint8 searchmode);
void wlc_phy_cals_acphy(void *ctx, uint8 legacy_caltype, uint8 searchmode);
extern void wlc_phy_low_rate_adc_enable_acphy(phy_info_t *pi, bool enable);

#endif /* _phy_ac_calmgr_h_ */
