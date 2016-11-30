/*
 * ACPHY Napping module implementation
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
 * $Id$
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_nap.h"
#include <phy_utils_var.h>
#include <phy_ac.h>
#include <phy_ac_nap.h>
#include <phy_utils_reg.h>
#include <phy_ac_info.h>
#include <wlc_phyreg_ac.h>
#include <phy_rstr.h>
#include <wlioctl.h>

/* Napping parameters */
typedef struct {
	uint16 nap2rx_seq[16];
	uint16 nap2rx_seq_dly[16];
	uint16 rx2nap_seq[16];
	uint16 rx2nap_seq_dly[16];
	uint16 nap_lo_th_adj[5];
	uint16 nap_hi_th_adj[5];
	uint16 nap_wait_in_cs_len;
	uint16 nap_len;
	uint16 nap2cs_wait_in_reset;
	uint16 pktprocResetLen;
	uint8 ed_thresh_cal_start_stage;
} phy_ac_nap_params_t;

/* module private states */
struct phy_ac_nap_info {
	phy_info_t		*pi;
	phy_ac_info_t		*aci;
	phy_nap_info_t		*cmn_info;
	phy_ac_nap_params_t	*nap_params;
	uint16	nap_disable_reqs;
	/* Parameter to enable Napping feature */
	uint8	nap_en;
};

/* Napping related table ID's */
typedef enum {
	RX2NAP_SEQ_TBL,
	RX2NAP_SEQ_DLY_TBL,
	NAP2RX_SEQ_TBL,
	NAP2RX_SEQ_DLY_TBL,
	NAP_LO_TH_ADJ_TBL,
	NAP_HI_TH_ADJ_TBL
} phy_ac_nap_param_tbl_t;

/* Local functions */
static int BCMATTACHFN(phy_ac_populate_nap_params)(phy_ac_nap_info_t *nap_info);
static void* BCMRAMFN(phy_ac_get_nap_param_tbl)(phy_info_t *pi,
		phy_ac_nap_param_tbl_t tbl_id);

#ifdef WL_NAP
static void phy_ac_set_nap_params(phy_info_t *pi);
static void phy_ac_reset_nap_params(phy_info_t *pi);
static void phy_ac_load_nap_sequencer_28nm_ulp(phy_info_t *pi);
#endif /* WL_NAP */

/* local functions */
static void phy_ac_nap_nvram_attach(phy_ac_nap_info_t *nap_info);
#ifdef WL_NAP
static void phy_ac_nap_get_status(phy_type_nap_ctx_t *ctx, uint16 *reqs, bool *nap_en);
static void phy_ac_nap_set_disable_req(phy_type_nap_ctx_t *ctx, uint16 req,
	bool disable, bool agc_reconfig, uint8 req_id);
#endif /* WL_NAP */

/* register phy type specific implementation */
phy_ac_nap_info_t *
BCMATTACHFN(phy_ac_nap_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_nap_info_t *cmn_info)
{
	phy_ac_nap_info_t *nap_info;
	phy_type_nap_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((nap_info = phy_malloc(pi, sizeof(phy_ac_nap_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	nap_info->pi = pi;
	nap_info->aci = aci;
	nap_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
#ifdef WL_NAP
	fns.get_status = phy_ac_nap_get_status;
	fns.set_disable_req = phy_ac_nap_set_disable_req;
#endif /* WL_NAP */
	fns.ctx = nap_info;

	/* Populate nap params */
	if (phy_ac_populate_nap_params(nap_info) != BCME_OK) {
		goto fail;
	}

	/* Read srom params from nvram */
	phy_ac_nap_nvram_attach(nap_info);

	if (phy_nap_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_nap_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return nap_info;

fail:
	if ((nap_info != NULL) && (nap_info->nap_params != NULL)) {
		phy_mfree(pi, nap_info->nap_params, sizeof(phy_ac_nap_params_t));
	}

	/* error handling */
	if (nap_info != NULL) {
		phy_mfree(pi, nap_info, sizeof(phy_ac_nap_info_t));
	}

	return NULL;
}

void
BCMATTACHFN(phy_ac_nap_unregister_impl)(phy_ac_nap_info_t *nap_info)
{
	phy_info_t *pi = nap_info->pi;
	phy_nap_info_t *cmn_info = nap_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_nap_unregister_impl(cmn_info);

	if (nap_info->nap_params) {
		phy_mfree(pi, nap_info->nap_params, sizeof(phy_ac_nap_params_t));
	}

	phy_mfree(pi, nap_info, sizeof(phy_ac_nap_info_t));
}

bool
phy_ac_nap_is_enabled(phy_ac_nap_info_t *nap_info)
{
	if (nap_info != NULL) {
		return nap_info->nap_en;
	} else {
		return FALSE;
	}
}

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */

static int
BCMATTACHFN(phy_ac_populate_nap_params)(phy_ac_nap_info_t *nap_info)
{
	phy_ac_nap_params_t *nap_params;
	phy_info_t *pi;

	pi = nap_info->pi;

	if ((nap_params = phy_malloc(pi, sizeof(phy_ac_nap_params_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		/* Copy NAP2RX and RX2NAP seq */
		memcpy(nap_params->rx2nap_seq,
				phy_ac_get_nap_param_tbl(pi, RX2NAP_SEQ_TBL),
				sizeof(uint16) * 16);
		memcpy(nap_params->rx2nap_seq_dly,
				phy_ac_get_nap_param_tbl(pi, RX2NAP_SEQ_DLY_TBL),
				sizeof(uint16) * 16);
		memcpy(nap_params->nap2rx_seq,
				phy_ac_get_nap_param_tbl(pi, NAP2RX_SEQ_TBL),
				sizeof(uint16) * 16);
		memcpy(nap_params->nap2rx_seq_dly,
				phy_ac_get_nap_param_tbl(pi, NAP2RX_SEQ_DLY_TBL),
				sizeof(uint16) * 16);

		/* Nap ED threshold computation adjust factors */
		memcpy(nap_params->nap_lo_th_adj,
				phy_ac_get_nap_param_tbl(pi, NAP_LO_TH_ADJ_TBL),
				sizeof(uint16) * 5);
		memcpy(nap_params->nap_hi_th_adj,
				phy_ac_get_nap_param_tbl(pi, NAP_HI_TH_ADJ_TBL),
				sizeof(uint16) * 5);

		nap_params->nap_wait_in_cs_len = 10;
		nap_params->nap_len = 64;
		nap_params->nap2cs_wait_in_reset = 2;
		nap_params->pktprocResetLen = 112;

		/* Start stage to do ED threshold cal */
		nap_params->ed_thresh_cal_start_stage = 5;
	}

	/* setup ptr */
	nap_info->nap_params = nap_params;

	return (BCME_OK);

fail:
	if (nap_params != NULL) {
		phy_mfree(pi, nap_params, sizeof(phy_ac_nap_params_t));
	}

	return (BCME_NOMEM);
}

static void *
BCMRAMFN(phy_ac_get_nap_param_tbl)(phy_info_t *pi, phy_ac_nap_param_tbl_t tbl_id)
{
	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		switch (tbl_id) {
			case RX2NAP_SEQ_TBL:
				return rx2nap_seq_maj36;

			case RX2NAP_SEQ_DLY_TBL:
				return rx2nap_seq_dly_maj36;

			case NAP2RX_SEQ_TBL:
				return nap2rx_seq_maj36;

			case NAP2RX_SEQ_DLY_TBL:
				return nap2rx_seq_dly_maj36;

			case NAP_LO_TH_ADJ_TBL:
				return nap_lo_th_adj_maj36;

			case NAP_HI_TH_ADJ_TBL:
				return nap_hi_th_adj_maj36;

			default:
				return NULL;
		}
	} else {
		return NULL;
	}
}

static void
BCMATTACHFN(phy_ac_nap_nvram_attach)(phy_ac_nap_info_t *nap_info)
{
	nap_info->nap_en = (uint8)PHY_GETINTVAR_DEFAULT(nap_info->pi, rstr_nap_en, 0x0);

	/* nvram to control init ocl_disable state */
	if (nap_info->nap_en) {
		nap_info->nap_disable_reqs = 0;
	} else {
		nap_info->nap_disable_reqs = NAP_DISABLED_HOST;
	}
}
/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */

#ifdef WL_NAP
/* proc acphy_load_nap_sequencer_28nm_ulp */
static void
phy_ac_load_nap_sequencer_28nm_ulp(phy_info_t *pi)
{
	/* Bundle commands to toggle afe_rst_clk, afediv_adc_en, afediv_dac_en, afediv_rst_en */
	uint16 rfseq_bundle_48[3] = {0x0, 0x1000, 0x0};
	uint16 rfseq_bundle_49[3] = {0x0, 0x1100, 0x0};
	uint16 rfseq_bundle_50[3] = {0x0, 0x1000, 0x0};
	uint16 rfseq_bundle_51[3] = {0x0, 0x0, 0x0};
	uint16 rfseq_bundle_52[3] = {0x0, 0x1400, 0x0};
	uint16 rfseq_bundle_53[3] = {0x0, 0x1500, 0x0};
	uint16 rfseq_bundle_54[3] = {0x0, 0x1400, 0x0};
	uint16 rfseq_bundle_55[3] = {0x0, 0x0400, 0x0};
	uint16 temp_var1, temp_var2;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_nap_params_t *nap_params = pi_ac->napi->nap_params;

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 48, 48, rfseq_bundle_48);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 49, 48, rfseq_bundle_49);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 50, 48, rfseq_bundle_50);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 51, 48, rfseq_bundle_51);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 52, 48, rfseq_bundle_52);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 53, 48, rfseq_bundle_53);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 54, 48, rfseq_bundle_54);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 55, 48, rfseq_bundle_55);

	/* To avoid pulsing of oclS_set_bias_reset1 (0xe4) and oclW_set_bias_reset1 (0xe2) */
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0xe4, 16, &temp_var1);
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0xe2, 16, &temp_var2);

	temp_var1 = temp_var1 & 0x77;
	temp_var2 = temp_var2 & 0x77;

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0xe4, 16, &temp_var1);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0xe2, 16, &temp_var2);

	if (!ACMINORREV_0(pi)) {
		/* Changes related to SW43012-1540 */
		/* Program oclW_rx_lpf_ctl_lut_enrx1(55:54) = 3
		    RFSeqTbl[0x1AA] => oclW_rx_lpf_ctl_lut_enrx1(0)(58:50)
		*/
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1aa, 16, &temp_var1);
		temp_var1 = temp_var1 | (3 << 4);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1aa, 16, &temp_var1);

		/* Program oclS_rx_lpf_ctl_lut_disrx1(55:54) = 0
		    RFSeqTbl[0x1A8] => oclS_rx_lpf_ctl_lut_disrx1(0)(58:50)
		*/
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1a8, 16, &temp_var1);
		temp_var1 = temp_var1 & 0xffcf;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1a8, 16, &temp_var1);
	}

	/* Load RX2NAP sequence and delay values */
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x400, 16,
			nap_params->rx2nap_seq);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x420, 16,
			nap_params->rx2nap_seq_dly);

	/* Load NAP2RX sequence and delay values */
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x410, 16,
			nap_params->nap2rx_seq);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x430, 16,
			nap_params->nap2rx_seq_dly);
}

/* proc acphy_enable_napping_28nm_ulp */
void
phy_ac_config_napping_28nm_ulp(phy_info_t *pi)
{
	/* Load napping related RFSeq entries */
	phy_ac_load_nap_sequencer_28nm_ulp(pi);

	/* Enable napping */
	phy_ac_nap_enable(pi, pi->u.pi_acphy->napi->nap_en, FALSE);
}

/* proc acphy_set_nap_params */
static void
phy_ac_set_nap_params(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_nap_params_t *nap_params = pi_ac->napi->nap_params;
	uint16 spare_reg;

	/* Napping related counters */
	MOD_PHYREG(pi, nap_len, nap_len, nap_params->nap_len);
	MOD_PHYREG(pi, nap2cs_wait_in_reset_len, nap2cs_wait_in_reset,
			nap_params->nap2cs_wait_in_reset);
	WRITE_PHYREG(pi, nap_wait_in_cs_len, nap_params->nap_wait_in_cs_len);

	/* SW43012-1024 : Fix for tx turn around delay when napping enabled */
	WRITE_PHYREG(pi, pktprocResetLen, nap_params->pktprocResetLen);

	ACPHY_REG_LIST_START
		/* Enable napping */
		MOD_PHYREG_ENTRY(pi, NapCtrl, enAdcFERstonNap, 1)
		MOD_PHYREG_ENTRY(pi, NapCtrl, disable_nap_on_aci_detect, 1)
		MOD_PHYREG_ENTRY(pi, NapCtrl, enFineStrRstonNap, 1)
		MOD_PHYREG_ENTRY(pi, NapCtrl, enFreqEstRstonNap, 1)
		MOD_PHYREG_ENTRY(pi, NapCtrl, enFrontEndRstonNap, 0)
		MOD_PHYREG_ENTRY(pi, NapCtrl, napCrsRstAccDis, 1)
		MOD_PHYREG_ENTRY(pi, NapCtrl, nap_en, 1)
		WRITE_PHYREG_ENTRY(pi, nap_rfctrl_prog_bits, 0x7)

		/* Enable clock gating */
		MOD_PHYREG_ENTRY(pi, NapCtrl, enAdcFEClkGateonNap, 0x1)
		MOD_PHYREG_ENTRY(pi, NapCtrl, enFEClkGateonNap, 0x1)

		/* Setting use_fr_reset to force only data reset and avoid resetting
		 * the fifo pointers when nap is enabled.
		 */
		MOD_PHYREG_ENTRY(pi, RxFeCtrl1, use_fr_reset, 1)

		/* Disable bg_pulse in NAP2RX and RX2NAP */
		MOD_PHYREG_ENTRY(pi, RfBiasControl, ocls_bg_pulse_val, 0)
		MOD_PHYREG_ENTRY(pi, RfBiasControl, oclw_bg_pulse_val, 0)
	ACPHY_REG_LIST_EXECUTE(pi);

	if (ACMAJORREV_36(pi->pubpi->phy_rev) && (!ACMINORREV_0(pi))) {
		/* Fix to pktproc stuck in nap state on nap disable : CRDOT11ACPHY-2102
		    SpareRegB0(7) = 0
		*/
		spare_reg = READ_PHYREG(pi, SpareRegB0) & 0xff7f;
		WRITE_PHYREG(pi, SpareRegB0, spare_reg);

		/* Fix to nap tx issue : CRDOT11ACPHY-2234
		    SpareReg(11) = 1
		*/
		spare_reg = READ_PHYREG(pi, SpareReg) | (1 << 11);
		WRITE_PHYREG(pi, SpareReg, spare_reg);
	}
}

static void
phy_ac_reset_nap_params(phy_info_t *pi)
{
	/* Reset napping related registers */
	ACPHY_REG_LIST_START
		WRITE_PHYREG_ENTRY(pi, NapCtrl, 0)
		WRITE_PHYREG_ENTRY(pi, nap_rfctrl_prog_bits, 0)
		MOD_PHYREG_ENTRY(pi, RxFeCtrl1, use_fr_reset, 0)
		MOD_PHYREG_ENTRY(pi, RfBiasControl, ocls_bg_pulse_val, 1)
		MOD_PHYREG_ENTRY(pi, RfBiasControl, oclw_bg_pulse_val, 1)
		WRITE_PHYREG_ENTRY(pi, pktprocResetLen, 112)
	ACPHY_REG_LIST_EXECUTE(pi);
}

void
phy_ac_nap_enable(phy_info_t *pi, bool enable, bool agc_reconfig)
{
	bool suspend = FALSE;

	/* Suspend MAC if haven't done so */
	wlc_phy_conditional_suspend(pi, &suspend);

	if (enable) {
		/* Set napping related params */
		phy_ac_set_nap_params(pi);
	} else {
		/* Reset napping related params */
		phy_ac_reset_nap_params(pi);
	}

	/* Indicate Ucode Napping feature enabled/disabled using host flag */
	(enable == TRUE) ? wlapi_mhf(pi->sh->physhim, MHF4, MHF4_NAPPING_ENABLE,
			MHF4_NAPPING_ENABLE, WLC_BAND_ALL) :  wlapi_mhf(pi->sh->physhim, MHF4,
			MHF4_NAPPING_ENABLE, 0, WLC_BAND_ALL);

	/* Reconfigure AGC parameters */
	if (agc_reconfig) {
		if (enable) {
			/* Configure SSAGC */
			phy_ac_agc_config(pi, SINGLE_SHOT_AGC);
		} else {
			/* Configure FastAGC */
			phy_ac_agc_config(pi, FAST_AGC);
		}
	}

	/* Resume MAC */
	wlc_phy_conditional_resume(pi, &suspend);
}

void
phy_ac_nap_ed_thresh_cal(phy_info_t *pi, int8 *cmplx_pwr_dBm)
{
	uint16 lo_thresh[] = {0x0, 0x0, 0x0, 0x0, 0x0};
	uint16 hi_thresh[] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
	uint8 i, j, core;
	uint16 stage_sig_energy, stage_noise_energy, min_sig_energy, max_noise_energy;
	uint16 adj_fact;
	int8 max_cmplx_pwr;
	int8 tbl_idx, tbl_sz;

	/* Obtained from TCL acphy_nap_ed_thresh_energy_compute proc
	   0.4 noise energy = 10^((iq_dbm - 33)/10)*2330168888.8885
	*/
	uint16 p4us_noise_energy_tbl[] = {233, 293, 369, 465, 585, 737, 928, 1168, 1470, 1851,
			2330, 2934, 3693, 4649, 5853, 7369, 9277, 11679, 14702, 18509, 23302
	};

	/* Obtained from TCL acphy_nap_ed_thresh_energy_compute proc
	   0.4 sig energy = 10^((iq_dbm + sig_pwr_del - 33)/10)*2330168888.8885
	*/
	uint16 p4us_sig_energy_tbl[] = {434, 546, 688, 866, 1090, 1372, 1727, 2175, 2738, 3447,
			4339, 5462, 6877, 8657, 10899, 13721, 17274, 21746, 27377, 34466, 43390
	};

	/* Register to configure LOW thresholds */
	uint16 lo_thresh_reg_addr[] = {
		ACPHY_REG(pi, nap_ed_thld_lo_1),
		ACPHY_REG(pi, nap_ed_thld_lo_2),
		ACPHY_REG(pi, nap_ed_thld_lo_3),
		ACPHY_REG(pi, nap_ed_thld_lo_4),
		ACPHY_REG(pi, nap_ed_thld_lo_5)
	};

	/* Register to configure HI thresholds */
	uint16 hi_thresh_reg_addr[] = {
		ACPHY_REG(pi, nap_ed_thld_hi_1),
		ACPHY_REG(pi, nap_ed_thld_hi_2),
		ACPHY_REG(pi, nap_ed_thld_hi_3),
		ACPHY_REG(pi, nap_ed_thld_hi_4),
		ACPHY_REG(pi, nap_ed_thld_hi_5)
	};

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_nap_params_t *nap_params = pi_ac->napi->nap_params;

	/* Find the maximum power seen across cores */
	max_cmplx_pwr = -100;
	FOREACH_CORE(pi, core) {
		/* DVGA gain is already accounted when computing power dBm */
		if (cmplx_pwr_dBm[core] > max_cmplx_pwr) {
			max_cmplx_pwr = cmplx_pwr_dBm[core];
		}
	}

	/* Find the index to LUT, first entry in table is for -37 dBm */
	tbl_idx = max_cmplx_pwr + 37;

	/* Out of bound */
	tbl_sz = ARRAYSIZE(p4us_sig_energy_tbl);
	if ((tbl_idx < 0) || (tbl_idx > (tbl_sz - 1))) {
		tbl_idx = (tbl_idx < 0) ? 0 : (tbl_sz - 1);
	}

	/* Compute ED thresholds (low and high) */
	for (i = nap_params->ed_thresh_cal_start_stage; i <= 5; i++) {
		j = i - 1;
		stage_sig_energy = p4us_sig_energy_tbl[tbl_idx] * i;
		stage_noise_energy = p4us_noise_energy_tbl[tbl_idx] * i;

		adj_fact = (stage_sig_energy * nap_params->nap_lo_th_adj[j]) / 100;
		min_sig_energy = stage_sig_energy - adj_fact;

		adj_fact = (stage_noise_energy * nap_params->nap_hi_th_adj[j]) / 100;
		max_noise_energy = stage_noise_energy + adj_fact;

		lo_thresh[j] = min_sig_energy;
		hi_thresh[j] = max_noise_energy;
	}

	/* since we are touching phy regs mac has to be suspended */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	/* Write thresholds to registers */
	for (i = 0; i < 5; i++) {
		phy_utils_write_phyreg(pi, lo_thresh_reg_addr[i], lo_thresh[i]);
		phy_utils_write_phyreg(pi, hi_thresh_reg_addr[i], hi_thresh[i]);
	}

	/* resume mac */
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}

static void
phy_ac_nap_get_status(phy_type_nap_ctx_t *ctx, uint16 *reqs, bool *nap_en)
{
	phy_ac_nap_info_t *napi = (phy_ac_nap_info_t *)ctx;
	phy_info_t *pi = napi->pi;
	bool suspend = FALSE;

	if (ISACPHY(pi)) {
		if (reqs != NULL) {
			*reqs = napi->nap_disable_reqs;
		}

		if (nap_en != NULL && pi->sh->clk) {
			/* Suspend MAC if haven't done so */
			wlc_phy_conditional_suspend(pi, &suspend);
			*nap_en = (READ_PHYREGFLD(pi, NapCtrl, nap_en) == 1);
			/* Resume MAC */
			wlc_phy_conditional_resume(pi, &suspend);
		}
	}
}

static void
phy_ac_nap_set_disable_req(phy_type_nap_ctx_t *ctx, uint16 req,
	bool disable, bool agc_reconfig, uint8 req_id)
{
	phy_ac_nap_info_t *napi = (phy_ac_nap_info_t *)ctx;
	phy_info_t *pi = napi->pi;

	if (disable) {
		napi->nap_disable_reqs |= req;
	} else {
		napi->nap_disable_reqs &= ~req;
	}

	if (pi->sh->clk) {
		phy_ac_nap_enable(pi, !napi->nap_disable_reqs, agc_reconfig);
	}
}
#endif /* WL_NAP */
