/*
 * ACPHY RXIQ CAL module interface (to other PHY modules).
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
 * $Id: phy_ac_rxiqcal.h 657664 2016-09-02 04:57:54Z $
 */

#ifndef _phy_ac_rxiqcal_h_
#define _phy_ac_rxiqcal_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_rxiqcal.h>

/* forward declaration */
typedef struct phy_ac_rxiqcal_info phy_ac_rxiqcal_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_rxiqcal_info_t *phy_ac_rxiqcal_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_rxiqcal_info_t *mi);
void phy_ac_rxiqcal_unregister_impl(phy_ac_rxiqcal_info_t *info);


/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
typedef struct _acphy_2069_rxcal_radioregs {
	bool   is_orig;
	uint16 rf_2069_txrx2g_cal_tx[PHY_CORE_MAX];
	uint16 rf_2069_txrx5g_cal_tx[PHY_CORE_MAX];
	uint16 rf_2069_txrx2g_cal_rx[PHY_CORE_MAX];
	uint16 rf_2069_txrx5g_cal_rx[PHY_CORE_MAX];
	uint16 rf_2069_rxrf2g_cfg2[PHY_CORE_MAX];
	uint16 rf_2069_rxrf5g_cfg2[PHY_CORE_MAX];
} acphy_2069_rxcal_radioregs_t;

typedef struct _acphy_tiny_rxcal_radioregs {
	bool   is_orig;
	uint16 rf_tiny_rfrx_top_2g_ovr_east[PHY_CORE_MAX];
} acphy_tiny_rxcal_radioregs_t;

typedef struct _acphy_rxcal_phyregs {
	bool   is_orig;
	uint16 RfctrlOverrideTxPus [PHY_CORE_MAX];
	uint16 RfctrlCoreTxPus [PHY_CORE_MAX];
	uint16 RfctrlOverrideRxPus [PHY_CORE_MAX];
	uint16 RfctrlCoreRxPus [PHY_CORE_MAX];
	uint16 RfctrlOverrideGains [PHY_CORE_MAX];
	uint16 Dac_gain [PHY_CORE_MAX];
	uint16 forceFront [PHY_CORE_MAX];
	uint16 RfctrlCoreTXGAIN1 [PHY_CORE_MAX];
	uint16 RfctrlCoreTXGAIN2 [PHY_CORE_MAX];
	uint16 RfctrlCoreRXGAIN1 [PHY_CORE_MAX];
	uint16 RfctrlCoreRXGAIN2 [PHY_CORE_MAX];
	uint16 RfctrlCoreLpfGain [PHY_CORE_MAX];
	uint16 RfctrlOverrideLpfCT [PHY_CORE_MAX];
	uint16 RfctrlCoreLpfCT [PHY_CORE_MAX];
	uint16 RfctrlCoreLpfGmult [PHY_CORE_MAX];
	uint16 RfctrlCoreRCDACBuf [PHY_CORE_MAX];
	uint16 RfctrlOverrideLpfSwtch [PHY_CORE_MAX];
	uint16 RfctrlCoreLpfSwtch [PHY_CORE_MAX];
	uint16 RfctrlOverrideAfeCfg [PHY_CORE_MAX];
	uint16 RfctrlCoreAfeCfg1 [PHY_CORE_MAX];
	uint16 RfctrlCoreAfeCfg2 [PHY_CORE_MAX];
	uint16 RfctrlOverrideLowPwrCfg [PHY_CORE_MAX];
	uint16 RfctrlCoreLowPwr [PHY_CORE_MAX];
	uint16 RfctrlOverrideAuxTssi [PHY_CORE_MAX];
	uint16 RfctrlCoreAuxTssi1 [PHY_CORE_MAX];
	uint16 RfctrlCoreAuxTssi2[PHY_CORE_MAX];
	uint16 RfctrlOverrideGlobalPus;
	uint16 RfctrlCoreGlobalPus;
	uint16 RxSdFeConfig1;
	uint16 RxSdFeConfig6;
	uint16 bbmult[PHY_CORE_MAX];
	uint16 rfseq_txgain[3 * PHY_CORE_NUM_4];
	uint16 RfseqCoreActv2059;
	uint16 RfctrlIntc[PHY_CORE_NUM_4];
	uint16 PapdEnable[PHY_CORE_MAX];
	uint16 AfePuCtrl;

	uint8 txpwridx[PHY_CORE_MAX];
	uint16 lbFarrowCtrl;
	uint16 spur_can_s1_en[PHY_CORE_MAX];
	uint16 spur_can_en[PHY_CORE_MAX];
} acphy_rxcal_phyregs_t;

int32 phy_ac_rxiqcal_get_fdiqi_slope(phy_ac_rxiqcal_info_t *rxiqcali, uint8 core);
void phy_ac_rxiqcal_set_fdiqi_slope(phy_ac_rxiqcal_info_t *rxiqcali, uint8 core, int32 slope);
bool phy_ac_rxiqcal_is_fdiqi_enabled(phy_ac_rxiqcal_info_t *rxiqcali);
void phy_ac_rxiqcal_set_fdiqi_enable(phy_ac_rxiqcal_info_t *rxiqcali, bool enable);

extern void wlc_phy_rx_iq_comp_acphy(phy_info_t *pi, uint8 write,
	phy_iq_comp_t *pcomp, uint8 rx_core);
extern void wlc_phy_rx_fdiqi_comp_acphy(phy_info_t *pi, bool enable);
extern int  wlc_phy_cal_rx_fdiqi_acphy(phy_info_t *pi);
extern void wlc_phy_turnon_rxlogen_20694(phy_info_t *pi, uint8 *sr_reg);
extern void wlc_phy_turnoff_rxlogen_20694(phy_info_t *pi, uint8 *sr_reg);
extern void wlc_phy_rx_iq_est_acphy(phy_info_t *pi, phy_iq_est_t *est, uint16 num_samps,
	uint8 wait_time, uint8 wait_for_crs, bool rxiq_cal);
#if defined(BCMDBG)
extern void wlc_phy_force_fdiqi_acphy(phy_info_t *pi, uint16 int_val);
#endif
extern void wlc_phy_dig_lpf_override_acphy(phy_info_t *pi, uint8 dig_lpf_ht);
void wlc_phy_rxcal_coeffs_upd(phy_info_t *pi, rxcal_coeffs_t *rxcal_cache);
void phy_ac_rxiqcal(phy_info_t *pi);
#ifdef PHYCAL_CACHING
void phy_ac_rxiqcal_save_cache(phy_ac_rxiqcal_info_t *rxiqcali, ch_calcache_t *ctx);
#else
void wlc_phy_scanroam_cache_rxcal_acphy(void *ctx, bool set);
#endif
#endif /* _phy_ac_rxiqcal_h_ */
