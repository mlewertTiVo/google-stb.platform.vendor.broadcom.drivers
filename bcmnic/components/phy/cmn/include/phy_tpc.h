/*
 * TxPowerCtrl module internal interface (to other PHY modules).
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
 * $Id: phy_tpc.h 659961 2016-09-16 18:46:01Z $
 */

#ifndef _phy_tpc_h_
#define _phy_tpc_h_

#include <typedefs.h>
#include <phy_api.h>
#include <wlc_ppr.h>

/* forward declaration */
typedef struct phy_tpc_info phy_tpc_info_t;

/* attach/detach */
phy_tpc_info_t *phy_tpc_attach(phy_info_t *pi);
void phy_tpc_detach(phy_tpc_info_t *ri);

/* ******** interface for TPC module ******** */
/* tx gain settings */
typedef struct {
	uint16 rad_gain; /* Radio gains */
	uint16 rad_gain_mi; /* Radio gains [16:31] */
	uint16 rad_gain_hi; /* Radio gains [32:47] */
	uint16 dac_gain; /* DAC attenuation */
	uint16 bbmult;   /* BBmult */
} txgain_setting_t;

#ifdef WL_SAR_SIMPLE_CONTROL
#define SAR_VAL_LENG        (8) /* Number of bit for SAR target pwr val */
#define SAR_ACTIVEFLAG_MASK (0x80)  /* Bitmask of SAR limit active flag */
#define SAR_VAL_MASK        (0x7f)  /* Bitmask of SAR limit target */
#endif /* WL_SAR_SIMPLE_CONTROL */

#ifdef POWPERCHANNL
	#if defined(WL_ENAB_RUNTIME_CHECK)
		#define PWRPERCHAN_ENAB(pi)	(pi->_powerperchan)
	#elif defined(POWPERCHANNL_DISABLED)
		#define PWRPERCHAN_ENAB(pi)	(0)
	#else
		#define PWRPERCHAN_ENAB(pi)	(pi->_powerperchan)
	#endif
#else
	#define PWRPERCHAN_ENAB(pi)	(0)
#endif /* POWPERCHANNL */

/* recalc target txpwr and apply to h/w */
void phy_tpc_recalc_tgt(phy_tpc_info_t *ti);

/* srom9 : Used by HT and N PHY */
void wlc_phy_txpwr_apply_srom9(phy_info_t *pi, uint8 band_num, chanspec_t chanspec,
	uint8 tmp_max_pwr, ppr_t *tx_srom_max_pwr);

/* %%%%%% srom accessory functions */
void wlc_phy_txpwr_srom_convert_cck(uint16 po, uint8 max_pwr, ppr_dsss_rateset_t *dsss);
void wlc_phy_txpwr_srom_convert_ofdm(uint32 po, uint8 max_pwr, ppr_ofdm_rateset_t *ofdm);
void wlc_phy_ppr_set_dsss(ppr_t* tx_srom_max_pwr, uint8 bwtype, ppr_dsss_rateset_t* pwr_offsets,
    phy_info_t *pi);
void wlc_phy_ppr_set_ofdm(ppr_t* tx_srom_max_pwr, uint8 bwtype, ppr_ofdm_rateset_t* pwr_offsets,
    phy_info_t *pi);

void wlc_phy_txpwr_srom_convert_mcs(uint32 po, uint8 max_pwr, ppr_ht_mcs_rateset_t *mcs);
void wlc_phy_txpwr_srom_convert_mcs_offset(uint32 po, uint8 offset, uint8 max_pwr,
	ppr_ht_mcs_rateset_t* mcs, int8 mcs7_15_offset);

/* CCK Pwr Index Convergence Correction */
void phy_tpc_cck_corr(phy_info_t *pi);

/* check limit */
void phy_tpc_check_limit(phy_info_t *pi);

//#ifdef PREASSOC_PWRCTRL
void phy_preassoc_pwrctrl_upd(phy_info_t *pi, chanspec_t chspec);
//#endif /* PREASSOC_PWRCTRL */

/* set FCC power limit */
#ifdef FCC_PWR_LIMIT_2G
void wlc_phy_fcc_pwr_limit_set(wlc_phy_t *ppi, bool enable);
#endif /* FCC_PWR_LIMIT_2G */

void wlc_phy_txpwr_srom11_read_ppr(phy_info_t *pi);
void wlc_phy_txpwr_srom12_read_ppr(phy_info_t *pi);
/* two range tssi */
bool phy_tpc_get_tworangetssi2g(phy_tpc_info_t *tpci);
bool phy_tpc_get_tworangetssi5g(phy_tpc_info_t *tpci);
/* pdet_range_id */
uint8 phy_tpc_get_2g_pdrange_id(phy_tpc_info_t *tpci);
uint8 phy_tpc_get_5g_pdrange_id(phy_tpc_info_t *tpci);

uint8 phy_tpc_get_band_from_channel(phy_tpc_info_t *tpci, uint channel);

#ifdef NO_PROPRIETARY_VHT_RATES
#else
void wlc_phy_txpwr_read_1024qam_ppr(phy_info_t *pi);
void wlc_phy_txpwr_srom11_ext_1024qam_convert_mcs_5g(uint32 po,
		chanspec_t chanspec, uint8 tmp_max_pwr,
		ppr_vht_mcs_rateset_t* vht);
void wlc_phy_txpwr_srom11_ext_1024qam_convert_mcs_2g(uint16 po,
		chanspec_t chanspec, uint8 tmp_max_pwr,
		ppr_vht_mcs_rateset_t* vht);
#endif
void phy_tpc_ipa_upd(phy_tpc_info_t *tpci);

#ifdef RADIO_HEALTH_CHECK
phy_crash_reason_t phy_radio_health_check_baseindex(phy_info_t *pi);
#endif /* RADIO_HEALTH_CHECK */

bool wlc_phy_txpwr_srom9_read(phy_info_t *pi);

void phy_tpc_get_paparams_for_band(phy_info_t *pi, int32 *a1, int32 *b0, int32 *b1);

#endif /* _phy_tpc_h_ */
