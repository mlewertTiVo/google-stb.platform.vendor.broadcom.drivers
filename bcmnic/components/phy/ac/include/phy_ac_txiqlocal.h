/*
 * ACPHY TXIQLO CAL module interface (to other PHY modules).
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
 * $Id: phy_ac_txiqlocal.h 633369 2016-04-22 08:36:32Z changbo $
 */

#ifndef _phy_ac_txiqlocal_h_
#define _phy_ac_txiqlocal_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_txiqlocal.h>

/* forward declaration */
typedef struct phy_ac_txiqlocal_info phy_ac_txiqlocal_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_txiqlocal_info_t *phy_ac_txiqlocal_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_txiqlocal_info_t *mi);
void phy_ac_txiqlocal_unregister_impl(phy_ac_txiqlocal_info_t *info);

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

/* ************************************************************************* */
/* The following definitions shared between TSSI, Cache, RxIQCal and TxIQLOCal			*/
/* ************************************************************************* */
#define ACPHY_IQCAL_TONEFREQ_80MHz 8000
#define ACPHY_IQCAL_TONEFREQ_40MHz 4000
#define ACPHY_IQCAL_TONEFREQ_20MHz 2000
#ifdef WLC_TXFDIQ
#define ACPHY_TXCAL_MAX_NUM_FREQ 4
#endif

int8 phy_ac_txiqlocal_get_fdiqi_slope(phy_ac_txiqlocal_info_t *txiqlocali, uint8 core);
void phy_ac_txiqlocal_set_fdiqi_slope(phy_ac_txiqlocal_info_t *txiqlocali, uint8 core, int8 slope);
bool phy_ac_txiqlocal_is_fdiqi_enabled(phy_ac_txiqlocal_info_t *txiqlocali);
extern void wlc_phy_poll_samps_WAR_acphy(phy_info_t *pi, int16 *samp, bool is_tssi,
                                         bool for_idle, txgain_setting_t *target_gains,
                                         bool for_iqcal, bool init_adc_inside, uint16 core,
                                         bool champ);
extern void wlc_phy_cal_txiqlo_coeffs_acphy(phy_info_t *pi, uint8 rd_wr, uint16 *coeff_vals,
	uint8 select, uint8 core);
extern void wlc_phy_ipa_set_bbmult_acphy(phy_info_t *pi, uint16 *m0, uint16 *m1,
	uint16 *m2, uint16 *m3, uint8 coremask);
extern void wlc_phy_precal_txgain_acphy(phy_info_t *pi, txgain_setting_t *target_gains);
extern int wlc_phy_cal_txiqlo_acphy(phy_info_t *pi, uint8 searchmode, uint8 mphase, uint8 Biq2byp);
uint8 wlc_phy_get_tbl_id_iqlocal(phy_info_t *pi, uint16 core);
extern void wlc_phy_txcal_phy_setup_acphy_core_sd_adc(phy_info_t *pi, uint8 core,
uint16 sdadc_config);
#ifdef WLC_TXFDIQ
extern void wlc_phy_tx_fdiqi_comp_acphy(phy_info_t *pi, bool enable, int fdiq_data_valid);
#endif
void phy_ac_txiqlocal(phy_info_t *pi, uint8 phase_id, uint8 searchmode);
void phy_ac_txiqlocal_prerx(phy_info_t *pi, uint8 searchmode);
void wlc_phy_txcal_coeffs_upd(phy_info_t *pi, txcal_coeffs_t *txcal_cache);
#if !defined(PHYCAL_CACHING)
void wlc_phy_scanroam_cache_txcal_acphy(void *ctx, bool set);
#endif
#endif /* _phy_ac_txiqlocal_h_ */
