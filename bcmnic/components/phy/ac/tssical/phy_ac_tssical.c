/*
 * ACPHY TSSI Cal module implementation
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
 * $Id: phy_ac_tssical.c 651872 2016-07-28 19:41:35Z ernst $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_btcx.h>
#include "phy_type_tssical.h"
#include <phy_ac.h>
#include <phy_ac_tssical.h>
#include <phy_cache_api.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_radioreg_20691.h>
#include <wlc_radioreg_20693.h>
#include <wlc_radioreg_20694.h>
#include <wlc_radioreg_20695.h>
#include <wlc_radioreg_20696.h>
#include <wlc_phy_radio.h>
#include <wlc_phyreg_ac.h>

#include <phy_utils_reg.h>
#include <phy_utils_channel.h>
#include <phy_utils_math.h>
#include <phy_ac_info.h>
#include <phy_rstr.h>
#include <phy_tpc.h>
#include <phy_utils_var.h>
#include <bcmdevs.h>

/* module private states */

typedef struct phy_ac_tssical_config_info {
	/* low range tssi */
	bool srom_lowpowerrange2g;
	bool srom_lowpowerrange5g;
} phy_ac_tssical_config_info_t;

struct phy_ac_tssical_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_tssical_info_t *cmn_info;
	int16 idle_tssi[PHY_CORE_MAX];
	phy_ac_tssical_config_info_t cfg;
};

/* local functions */
static int8 phy_ac_tssical_get_visible_thresh(phy_type_tssical_ctx_t *ctx);
static void wlc_phy_get_tssisens_min_acphy(phy_type_tssical_ctx_t *ctx, int8 *tssiSensMinPwr);
static void phy_ac_tssical_nvram_attach(phy_ac_tssical_info_t *ti);

/* register phy type specific implementation */
phy_ac_tssical_info_t *
BCMATTACHFN(phy_ac_tssical_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_tssical_info_t *cmn_info)
{
	phy_ac_tssical_info_t *tssical_info;
	phy_type_tssical_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((tssical_info = phy_malloc(pi, sizeof(phy_ac_tssical_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	tssical_info->pi = pi;
	tssical_info->aci = aci;
	tssical_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.visible_thresh = phy_ac_tssical_get_visible_thresh;
	fns.sens_min = wlc_phy_get_tssisens_min_acphy;
	fns.ctx = tssical_info;

	/* Read srom params from nvram */
	phy_ac_tssical_nvram_attach(tssical_info);

	if (phy_tssical_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_tssical_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return tssical_info;

	/* error handling */
fail:
	if (tssical_info != NULL)
		phy_mfree(pi, tssical_info, sizeof(phy_ac_tssical_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_tssical_unregister_impl)(phy_ac_tssical_info_t *tssical_info)
{
	phy_info_t *pi = tssical_info->pi;
	phy_tssical_info_t *cmn_info = tssical_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_tssical_unregister_impl(cmn_info);

	phy_mfree(pi, tssical_info, sizeof(phy_ac_tssical_info_t));
}

/* ************************* */
/*		Internal Functions		*/
/* ************************* */
#ifdef NOT_YET
static void wlc_phy_scanroam_tssical_cal_acphy(phy_info_t *pi, bool set)
#endif

#ifdef NOT_YET
static void
wlc_phy_scanroam_tssical_cal_acphy(phy_info_t *pi, bool set)
{
	uint16 ab_int[2];
	uint8 core;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	/* Prepare Mac and Phregs */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	PHY_TRACE(("wl%d: %s: in scan/roam set %d\n", pi->sh->unit, __FUNCTION__, set));

	if (set) {
		PHY_CAL(("wl%d: %s: save the txcal for scan/roam\n",
			pi->sh->unit, __FUNCTION__));
		/* save the txcal to tssical */
		FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
			wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ,
				ab_int, TB_OFDM_COEFFS_AB, core);
			pi->u.pi_acphy->txcal_tssical[core].txa = ab_int[0];
			pi->u.pi_acphy->txcal_tssical[core].txb = ab_int[1];
			wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ,
			&pi->u.pi_acphy->txcal_tssical[core].txd,
				TB_OFDM_COEFFS_D, core);
			pi->u.pi_acphy->txcal_tssical[core].txei =
				(uint8)READ_RADIO_REGC(pi, RF, TXGM_LOFT_FINE_I, core);
			pi->u.pi_acphy->txcal_tssical[core].txeq =
				(uint8)READ_RADIO_REGC(pi, RF, TXGM_LOFT_FINE_Q, core);
			pi->u.pi_acphy->txcal_tssical[core].txfi =
				(uint8)READ_RADIO_REGC(pi, RF, TXGM_LOFT_COARSE_I, core);
			pi->u.pi_acphy->txcal_tssical[core].txfq =
				(uint8)READ_RADIO_REGC(pi, RF, TXGM_LOFT_COARSE_Q, core);
			pi->u.pi_acphy->txcal_tssical[core].rxa =
				READ_PHYREGCE(pi, Core1RxIQCompA, core);
			pi->u.pi_acphy->txcal_tssical[core].rxb =
				READ_PHYREGCE(pi, Core1RxIQCompB, core);
			}

		/* mark the tssical as valid */
		pi->u.pi_acphy->txcal_tssical_cookie = TXCAL_tssical_VALID;
	} else {
		if (pi->u.pi_acphy->txcal_tssical_cookie == TXCAL_tssical_VALID) {
			PHY_CAL(("wl%d: %s: restore the txcal after scan/roam\n",
				pi->sh->unit, __FUNCTION__));
			/* restore the txcal from tssical */
			wlc_phy_cal_coeffs_upd(pi, pi->u.pi_acphy->txcal_tssical);
			/* This function has been split into two:
			 * wlc_phy_txcal_coeffs_upd and wlc_phy_rxcal_coeffs_upd
			 */
		}
	}

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}
#endif /* NOT_YET */


static void
BCMATTACHFN(phy_ac_tssical_nvram_attach)(phy_ac_tssical_info_t *ti)
{
	ti->cfg.srom_lowpowerrange2g = (bool)PHY_GETINTVAR_DEFAULT_SLICE(ti->pi,
		rstr_lowpowerrange2g, FALSE);
	ti->cfg.srom_lowpowerrange5g = (bool)PHY_GETINTVAR_DEFAULT_SLICE(ti->pi,
		rstr_lowpowerrange5g, FALSE);
	return;
}
/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */
/* measure idle TSSI by sending 0-magnitude tone */
void
wlc_phy_txpwrctrl_idle_tssi_meas_acphy(phy_info_t *pi)
{
	phy_ac_tssical_info_t *tssicali = (phy_ac_tssical_info_t *)pi->u.pi_acphy->tssicali;
	uint8  core;
	int16  idle_tssi[PHY_CORE_MAX] = {0};
	uint16 orig_RfseqCoreActv2059, orig_RxSdFeConfig6 = 0;
	bool suspend = TRUE;

	if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
		/* Let WLAN have FEMCTRL to ensure cal is done properly */
		suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);
		if (!suspend) {
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
		}
		wlc_btcx_override_enable(pi);
	}

	if ((SCAN_RM_IN_PROGRESS(pi) || PLT_INPROG_PHY(pi) || PHY_MUTED(pi)) && !TINY_RADIO(pi))
		/* skip idle tssi cal */
		return;

#ifdef ATE_BUILD
	printf("===> Running Idle TSSI cal\n");
#endif /* ATE_BUILD */

	/* prevent crs trigger */
	wlc_phy_stay_in_carriersearch_acphy(pi, TRUE);
	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		phy_ac_rfseq_mode_set(pi, 1);
	}

	/* we should not need this but just in case */
	phy_ac_tssi_loopback_path_setup(pi, LOOPBACK_FOR_TSSICAL);

	if (TINY_RADIO(pi)) {
		MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 1);
		orig_RxSdFeConfig6 = READ_PHYREG(pi, RxSdFeConfig6);
		MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0,
			READ_PHYREGFLD(pi, RxSdFeConfig1, farrow_rshift_tx));
	}

	// Additional settings for 4365 core3 GPIO WAR
	if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_37(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, DcFiltAddress,        dcBypass,     1);
		MOD_PHYREG(pi, RfctrlCoreRXGAIN13,   rxgain_dvga,  0);
		MOD_PHYREG(pi, RfctrlCoreLpfGain3,   lpf_bq2_gain, 0);
		MOD_PHYREG(pi, RfctrlOverrideGains3, rxgain,       1);
		MOD_PHYREG(pi, RfctrlOverrideGains3, lpf_bq2_gain, 1);
	}

	if (ACMAJORREV_37(pi->pubpi->phy_rev)) {
		/* Must always be on or tssi capture will fail */
		MOD_PHYREG(pi, TssiAccumCtrl, tssi_accum_en, 1);
		MOD_PHYREG(pi, TssiAccumCtrl, tssi_filter_pos, 1);
	}

	/* force all TX cores on */
	orig_RfseqCoreActv2059 = READ_PHYREG(pi, RfseqCoreActv2059);
	MOD_PHYREG(pi, RfseqCoreActv2059, EnTx,  pi->sh->hw_phyrxchain);
	MOD_PHYREG(pi, RfseqCoreActv2059, DisRx, pi->sh->hw_phyrxchain);

	FOREACH_ACTV_CORE(pi, pi->sh->hw_phyrxchain, core) {
		wlc_phy_poll_samps_WAR_acphy(pi, idle_tssi, TRUE, TRUE, NULL,
		                             FALSE, TRUE, core, 0);
		tssicali->idle_tssi[core] = idle_tssi[core];
		printf("7271  -----core = %d  idle_tssi = %d\n", core,  tssicali->idle_tssi[core]);	
		wlc_phy_txpwrctrl_set_idle_tssi_acphy(pi, idle_tssi[core], core);
		PHY_TRACE(("wl%d: %s: idle_tssi core%d: %d\n",
		           pi->sh->unit, __FUNCTION__, core, tssicali->idle_tssi[core]));
	}

	WRITE_PHYREG(pi, RfseqCoreActv2059, orig_RfseqCoreActv2059);

	if (TINY_RADIO(pi)) {
		MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 0);
		WRITE_PHYREG(pi, RxSdFeConfig6, orig_RxSdFeConfig6);
	}

	if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
		wlc_phy_btcx_override_disable(pi);
		if (!suspend) {
			wlapi_enable_mac(pi->sh->physhim);
		}
	}

	// Additional settings for 4365 core3 GPIO WAR
	if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_37(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, DcFiltAddress,        dcBypass,     0);
		MOD_PHYREG(pi, RfctrlOverrideGains3, rxgain,       0);
		MOD_PHYREG(pi, RfctrlOverrideGains3, lpf_bq2_gain, 0);
	}

	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		phy_ac_rfseq_mode_set(pi, 0);
	}
	/* prevent crs trigger */
	wlc_phy_stay_in_carriersearch_acphy(pi, FALSE);

#ifdef ATE_BUILD
	printf("===> Finished Idle TSSI cal\n");
#endif /* ATE_BUILD */

}

void
wlc_phy_tssi_phy_setup_acphy(phy_info_t *pi, uint8 for_iqcal)
{
	uint8 core;
	phy_ac_tssical_info_t *ti = pi->u.pi_acphy->tssicali;
	bool flag2rangeon =
		((CHSPEC_IS2G(pi->radio_chanspec) &&
		phy_tpc_get_tworangetssi2g(pi->tpci)) || (CHSPEC_IS5G(pi->radio_chanspec) &&
		phy_tpc_get_tworangetssi5g(pi->tpci))) && PHY_IPA(pi);
	bool flaglowrangeon =
		((CHSPEC_IS2G(pi->radio_chanspec) && ti->cfg.srom_lowpowerrange2g) ||
		(CHSPEC_IS5G(pi->radio_chanspec) && ti->cfg.srom_lowpowerrange5g)) &&
		PHY_IPA(pi);

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
		if (ACREV_IS(pi->pubpi->phy_rev, 4) && CHSPEC_IS2G(pi->radio_chanspec)) {
			MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
		} else if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
			wlc_phy_ac_set_tssi_params_maj36(pi);
			/* Select Q rail for TSSI on 43012A0, since IQ swap is enabled on RX */
			MOD_PHYREG(pi, TSSIMode, tssiADCSel, 1);
			MOD_PHYREG(pi, TSSIMode, tssiPosSlope, 1);
			MOD_PHYREG(pi, TSSIMode, tssiEn, 1);
		} else {
			MOD_PHYREG(pi, TSSIMode, tssiADCSel, 1);
		}

		if (TINY_RADIO(pi)) {
			if (!PHY_IPA(pi)) {
				if (ACREV_IS(pi->pubpi->phy_rev, 4)) {
					MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx, 0x0);
				} else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
					ACMAJORREV_33(pi->pubpi->phy_rev)) {
					MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx, 0x1);
				} else {
					MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx, 0x2);
				}
			} else {
				if (CHSPEC_IS2G(pi->radio_chanspec))
					if (pi->sromi->txpwr2gAdcScale != -1)
						MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx,
							pi->sromi->txpwr2gAdcScale);
					else if (ACMAJORREV_4(pi->pubpi->phy_rev))
						MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx,
							0x1);
					else
						MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx,
							0x0);
				else
					if (pi->sromi->txpwr5gAdcScale != -1)
						MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx,
							pi->sromi->txpwr5gAdcScale);
					else if (ACMAJORREV_4(pi->pubpi->phy_rev))
						MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx,
							0x1);
					else
						MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx,
							0x0);
			}

			if (IBOARD(pi)) {
				MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx, 0x1);
			}
		}
		if (!(ACMAJORREV_40(pi->pubpi->phy_rev))) {
			if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
				/* PingPongComp.sdadcloopback_sat = 0 would drop one lsb
				 * bit 1 would saturate 1 msb bit.
				 * Setting this to 0 to drop 1 bit.
				 */
				MOD_PHYREG(pi, PingPongComp, sdadcloopback_sat, 0x0);
				MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx, 0x1);

				if (for_iqcal == 1) {
					MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 0x1);
					MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0x2);
				}
			} else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
					ACMAJORREV_33(pi->pubpi->phy_rev) ||
					ACMAJORREV_37(pi->pubpi->phy_rev)) {
				MOD_PHYREG(pi, PingPongComp, sdadcloopback_sat, 0x0);
				MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx, 0x0);
			}

			if (!(ACMAJORREV_4(pi->pubpi->phy_rev))) {
				MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi, core, tssi_pu, 1);
			}

			if (!PHY_IPA(pi) && (!for_iqcal)) {
				MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1, core, tssi_pu, for_iqcal);
			} else {
				if (!(ACMAJORREV_4(pi->pubpi->phy_rev))) {
					MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1, core, tssi_pu, 1);
				}
				if (for_iqcal) {
					MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core,
						tssi_range, 1);
					MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core,
						tssi_range, 1);
				} else if (flag2rangeon) {
					MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core,
						tssi_range, 0);
					MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core,
						tssi_range, 0);
				} else if (flaglowrangeon) {
					MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core,
						tssi_range, 1);
					MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core,
						tssi_range, 0);
				} else {
					MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core,
						tssi_range, 1);
					MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core,
						tssi_range, ~(for_iqcal));
				}
			}
		}
	}
}

void
wlc_phy_tssi_radio_setup_acphy_20694(phy_info_t *pi, uint8 for_iqcal)
{
	/* save radio config before changing it */
	// phy_ac_reg_cache_save(ti->aci, RADIOREGS_TSSI);
	uint8 core;

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
		MOD_RADIO_REG_20694(pi, RF, IQCAL_OVR1, core,
			ovr_iqcal_PU_tssi, 1);
		if (PHY_IPA(pi) || for_iqcal == 1) {
			PHY_ERROR(("FIXME: 4347A0 doesn't suppot iPA tssi yet\n"));
			return;
		} else {
			if (CHSPEC_IS5G(pi->radio_chanspec)) {
				MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG1, core, iqcal_sel_sw, 0x3);
			} else {
				MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG1, core, iqcal_sel_sw, 0x1);
			}
			MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG1, core, iqcal_sel_ext_tssi, 0x1);
			MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG1, core, iqcal_PU_tssi, 0x0);
		}
		MOD_RADIO_REG_20694(pi, RF, LPF_REG7, core, lpf_sw_bq2_adc, 0x0);
		MOD_RADIO_REG_20694(pi, RF, LPF_REG7, core, lpf_sw_bq1_adc, 0x0);
		MOD_RADIO_REG_20694(pi, RF, LPF_REG7, core, lpf_sw_aux_adc, 0x1);
		MOD_RADIO_REG_20694(pi, RF, LPF_REG8, core, lpf_sw_adc_test, 0x0);

		MOD_RADIO_REG_20694(pi, RF, IQCAL_OVR1, core, ovr_iqcal_PU_iqcal, 0x1);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG1, core, iqcal_PU_iqcal, 0x0);

		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG1, core, iqcal_tssi_GPIO_ctrl, 0x0);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG2, core, iqcal_tssi_cm_center, 0x1);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_IDAC, core, iqcal_tssi_bias, 0x0);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_GAIN_RFB, core, iqcal_rfb, 0x200);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_GAIN_RIN, core, iqcal_rin, 0x800);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_OVR1, core, ovr_iqcal_PU_wbpga, 0x1);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG5, core, wbpga_pu, 0x0);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_OVR1, core, ovr_iqcal_PU_loopback_bias, 0x1);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG5, core, loopback_bias_pu, 0x1);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG5, core, wbpga_cmref_iqbuf, 0x0);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG5, core, wbpga_cmref_half, 0x1);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG6, core, wbpga_cmref, 0x14);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG6, core, wbpga_bias, 0x14);


		/* Selection between auxpga or iqcal(wbpga) */
		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG4, core, iqcal2adc, 0x0);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG4, core, auxpga2adc, 0x1);

		MOD_RADIO_REG_20694(pi, RF, TESTBUF_OVR1, core, ovr_testbuf_PU, 0x1);
		MOD_RADIO_REG_20694(pi, RF, TESTBUF_CFG1, core, testbuf_PU, 0x1);

		MOD_RADIO_REG_20694(pi, RF, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 0x1);
		MOD_RADIO_REG_20694(pi, RF, TESTBUF_CFG1, core, testbuf_sel_test_port, 0x2);

		MOD_RADIO_REG_20694(pi, RF, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1);
		MOD_RADIO_REG_20694(pi, RF, AUXPGA_CFG1, core, auxpga_i_pu, 0x1);

		MOD_RADIO_REG_20694(pi, RF, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 0x1);
		MOD_RADIO_REG_20694(pi, RF, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x2);
	}
}

void
wlc_phy_tssi_radio_setup_acphy_28nm(phy_info_t *pi, uint8 for_iqcal)
{

	if (PHY_IPA(pi) || for_iqcal == 1) {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
		ACPHY_REG_LIST_START
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, IQCAL_CFG1, 0, iqcal_sel_sw, 0x0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TX2G_CFG2_OVR, 0, ovr_pad2g_tssi_ctrl_sel, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TX2G_CFG1_OVR, 0, ovr_pad2g_tssi_ctrl_pu, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, PAD2G_CFG11, 0, pad2g_tssi_ctrl_pu, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TX2G_CFG2_OVR, 0, ovr_pad2g_tssi_ctrl_range, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, PAD2G_CFG11, 0, pad2g_tssi_ctrl_range, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, PAD2G_CFG11, 0,       pad2g_wrssi0_en, 0x0)
		ACPHY_REG_LIST_EXECUTE(pi);
		MOD_RADIO_REG_28NM(pi, RF, PAD2G_CFG11, 0, pad2g_tssi_ctrl_sel, for_iqcal);
		} else {
		ACPHY_REG_LIST_START
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, IQCAL_CFG1, 0, iqcal_sel_sw, 0x2)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TX5G_CFG2_OVR, 0, ovr_pad5g_tssi_ctrl_sel, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TX5G_CFG2_OVR, 0, ovr_pad5g_tssi_ctrl_pu, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, PAD5G_CFG14, 0, pad5g_tssi_ctrl_pu, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TX5G_CFG2_OVR, 0, ovr_pad5g_tssi_ctrl_range, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, PAD5G_CFG14, 0, pad5g_tssi_ctrl_range, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, PAD5G_CFG14, 0,     pad5g_wrssi0_en, 0x0)
		ACPHY_REG_LIST_EXECUTE(pi);
		MOD_RADIO_REG_28NM(pi, RF, PAD5G_CFG14, 0, pad5g_tssi_ctrl_sel, for_iqcal);
		}

		MOD_RADIO_REG_28NM(pi, RF, IQCAL_CFG1, 0, iqcal_sel_ext_tssi, 0x0);
		MOD_RADIO_REG_28NM(pi, RF, IQCAL_OVR1, 0, ovr_iqcal_PU_tssi, 1);
		MOD_RADIO_REG_28NM(pi, RF, IQCAL_CFG1, 0, iqcal_PU_tssi, 0x1);
	} else {

		if (CHSPEC_IS5G(pi->radio_chanspec)) {
			MOD_RADIO_REG_28NM(pi, RF, IQCAL_CFG1, 0, iqcal_sel_sw, 0x3); /* 0xf */
		} else {
			MOD_RADIO_REG_28NM(pi, RF, IQCAL_CFG1, 0, iqcal_sel_sw, 0x1); /* 0xd */
		}
		MOD_RADIO_REG_28NM(pi, RF, IQCAL_CFG1, 0, iqcal_sel_ext_tssi, 0x1);
		MOD_RADIO_REG_28NM(pi, RF, IQCAL_OVR1, 0, ovr_iqcal_PU_tssi, 1);
		MOD_RADIO_REG_28NM(pi, RF, IQCAL_CFG1, 0, iqcal_PU_tssi, 0x0);
	}

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, IQCAL_OVR1, 0, ovr_iqcal_PU_iqcal, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, IQCAL_CFG1, 0, iqcal_PU_iqcal, 0x0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, IQCAL_CFG1, 0, iqcal_tssi_GPIO_ctrl,	0x0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, IQCAL_CFG2, 0, iqcal_tssi_cm_center,	0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, IQCAL_IDAC, 0, iqcal_tssi_bias, 0x0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, IQCAL_OVR1, 0, ovr_iqcal_PU_loopback_bias, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, IQCAL_CFG5, 0, loopback_bias_pu, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TESTBUF_OVR1, 0, ovr_testbuf_sel_test_port, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TESTBUF_CFG1, 0, testbuf_sel_test_port, 0x2)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TESTBUF_OVR1, 0, ovr_testbuf_PU, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TESTBUF_CFG1, 0, testbuf_PU, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AUXPGA_OVR1, 0, ovr_auxpga_i_pu, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AUXPGA_CFG1, 0, auxpga_i_pu, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AUXPGA_OVR1, 0, ovr_auxpga_i_sel_input, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AUXPGA_CFG1, 0, auxpga_i_sel_input, 0x2)
	ACPHY_REG_LIST_EXECUTE(pi);

}

void
wlc_phy_tssi_radio_setup_acphy_tiny(phy_info_t *pi, uint8 core_mask, uint8 for_iqcal)
{
	uint8 core, pupd = 0;
	phy_ac_tssical_info_t *ti = pi->u.pi_acphy->tssicali;
	bool flag2rangeon =
		((CHSPEC_IS2G(pi->radio_chanspec) &&
		phy_tpc_get_tworangetssi2g(pi->tpci)) || (CHSPEC_IS5G(pi->radio_chanspec) &&
		phy_tpc_get_tworangetssi5g(pi->tpci))) && PHY_IPA(pi);
	bool flaglowrangeon =
		((CHSPEC_IS2G(pi->radio_chanspec) && ti->cfg.srom_lowpowerrange2g) ||
		(CHSPEC_IS5G(pi->radio_chanspec) && ti->cfg.srom_lowpowerrange5g)) &&
		PHY_IPA(pi);
	ASSERT(TINY_RADIO(pi));

	if (BF3_TSSI_DIV_WAR(pi->u.pi_acphy) && ACMAJORREV_4(pi->pubpi->phy_rev)) {
		/* DIV_WAR is priority between DIV WAR & two range */
		flag2rangeon = 0;
	}

	/* 20691_tssi_radio_setup */
	/* # Powerdown rcal otherwise it won't let any other test point go through */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID) ||
		(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3)) {
		/* 20691_gpaio # Powerup gpaio block, powerdown rcal,
		 * clear all test point selection
		 */
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_TINY_ENTRY(pi, GPAIO_SEL2, 0, gpaio_pu, 1)

			MOD_RADIO_REG_TINY_ENTRY(pi, GPAIO_SEL0, 0, gpaio_sel_0to15_port, 0x0)
			MOD_RADIO_REG_TINY_ENTRY(pi, GPAIO_SEL1, 0, gpaio_sel_16to31_port, 0x0)

			MOD_RADIO_REG_TINY_ENTRY(pi, GPAIO_SEL0, 0, gpaio_sel_0to15_port, 0x0)
			MOD_RADIO_REG_TINY_ENTRY(pi, GPAIO_SEL1, 0, gpaio_sel_16to31_port, 0x0)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
	FOREACH_ACTV_CORE(pi, core_mask, core) {
		if (PHY_IPA(pi) || IBOARD(pi) || for_iqcal) {
			/* #
			 *  # INT TSSI setup
			 *  #
			 */
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_sel_sw, 0x0);
				/* # B3: (1 = iqcal, 0 = tssi);
				 * # B2: (1 = ext-tssi, 0 = int-tssi)
				 * # B1: (1 = 5g, 0 = 2g)
				 * # B0: (1 = wo filter, 0 = w filter for ext-tssi)
				 */
				 /* # Select PA output (and not PA input) */
				MOD_RADIO_REG_TINY(pi, PA2G_CFG1, core, pa2g_tssi_ctrl_sel, 0);
				pupd = 1;
			} else {
				pupd = 0;
				MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_sel_sw, 0x2);
				/* # Select PA output (and not PA input) */
				MOD_RADIO_REG_TINY(pi, TX5G_MISC_CFG1, core,
					pa5g_tssi_ctrl_sel, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
					ovr_pa5g_tssi_ctrl_sel, 1);
			}
			if (!(flag2rangeon || flaglowrangeon)) {
				MOD_RADIO_REG_20693(pi, TX5G_MISC_CFG1, core,
					pa5g_tssi_ctrl_range, (1-pupd));
				MOD_RADIO_REG_20693(pi, TX_TOP_5G_OVR1, core,
					ovr_pa5g_tssi_ctrl_range, (1-pupd));
				MOD_RADIO_REG_20693(pi, TX2G_MISC_CFG1, core,
					pa2g_tssi_ctrl_range, pupd);
				MOD_RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST,
					core, ovr_pa2g_tssi_ctrl_range, pupd);

			} else {
				MOD_RADIO_REG_20693(pi, TX_TOP_5G_OVR1, core,
					ovr_pa5g_tssi_ctrl_range, pupd);
				MOD_RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST,
					core, ovr_pa2g_tssi_ctrl_range, 1-pupd);
			}

			/* # int-tssi select */
			MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_sel_ext_tssi, 0x0);

			if (for_iqcal == 1) {
				MOD_RADIO_REG_20693(pi, AUXPGA_OVR1, core,
					ovr_auxpga_i_sel_gain, 0x1);
				MOD_RADIO_REG_20693(pi, AUXPGA_OVR1, core,
					ovr_auxpga_i_sel_vmid, 0x1);
				MOD_RADIO_REG_20693(pi, AUXPGA_CFG1, core,
					auxpga_i_sel_gain, 0x3);
				phy_utils_write_radioreg(pi, RADIO_REG_20693(pi,
					AUXPGA_VMID, core), 0x9c);

				MOD_RADIO_REG_20693(pi, AUXPGA_OVR1, core,
					ovr_auxpga_i_sel_input, 0x1);
				MOD_RADIO_REG_20693(pi, AUXPGA_CFG1, core,
					auxpga_i_sel_input, 0x0);

				if (CHSPEC_IS5G(pi->radio_chanspec)) {
					MOD_RADIO_REG_20693(pi, TX_TOP_5G_OVR1, core,
						ovr_pa5g_tssi_ctrl_range, 1);
					MOD_RADIO_REG_20693(pi, TX5G_MISC_CFG1, core,
						pa5g_tssi_ctrl_range, 0);
					MOD_RADIO_REG_20693(pi, TX5G_MISC_CFG1, core,
						pa5g_tssi_ctrl_sel, 0);
				} else {
					MOD_RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST, core,
						ovr_pa2g_tssi_ctrl_range, 0x1);
					MOD_RADIO_REG_20693(pi, TX2G_MISC_CFG1, core,
						pa2g_tssi_ctrl_range, 0);
					MOD_RADIO_REG_20693(pi, PA2G_CFG1, core,
						pa2g_tssi_ctrl_sel, 0);
				}
			}
		} else {
			/* #
			 * # EPA TSSI setup
			 * #
			 */
			/* # Enabling and Muxing per band */
			if (CHSPEC_IS5G(pi->radio_chanspec)) {
			    MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_sel_sw, 0x3);
			} else {
				/* ### gband
				 * 47189 ACDBMR uses the same 5G eTSSI for 2G as well
				 * since both 2G and 5G FEM has a common power detector
				 * which is conencted to the 5G TSSI line
				 */

				MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_sel_sw,
					(ROUTER_4349(pi) ? 0x3 : 0x1));
			}
			/* # ext-tssi select */
			MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_sel_ext_tssi, 0x1);
			if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) &&
				RADIOMAJORREV(pi) == 3)) {
				MOD_RADIO_REG_TINY(pi, IQCAL_OVR1, core, ovr_iqcal_PU_tssi, 0x1);
				/* # power on tssi */
				MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_PU_tssi, 0x1);
			}
		}
		if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3)) {
			MOD_RADIO_REG_TINY(pi, IQCAL_OVR1, core, ovr_iqcal_PU_iqcal, 0x1);
			/* # power off iqlocal */
			MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_PU_iqcal, 0x0);
			MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_PU_tssi, 1);
			MOD_RADIO_REG_TINY(pi, IQCAL_OVR1, core, ovr_iqcal_PU_tssi, 1);
		} else {
			/* # power on tssi/iqcal */
			MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_PU_iqcal, 0x1);
		}

		if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
			MOD_RADIO_REG_TINY(pi, TIA_CFG9, core, txbb_dac2adc, 0x0);
			MOD_RADIO_REG_TINY(pi, TIA_CFG5, core, tia_out_test, 0x0);

			MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1);
			MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1);
		}
		/* MOD_RADIO_REG_20691(pi, ADC_CFG10, 0, adc_in_test, 0xF); */
		if (!(ACMAJORREV_4(pi->pubpi->phy_rev) && (for_iqcal == 1))) {
			MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 0x1);
			MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x2);
		}
	}
}

void
wlc_phy_tssi_radio_setup_acphy(phy_info_t *pi, uint8 core_mask, uint8 for_iqcal)
{
	uint8 core;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	/* 2069_gpaio(clear) to pwr up the GPAIO and clean up al lthe otehr test pins */
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		FOREACH_ACTV_CORE(pi, core_mask, core) {
			/* first powerup the CGPAIO block */
			MOD_RADIO_REGC(pi, GE32_CGPAIO_CFG1, core, cgpaio_pu, 1);
		}
		ACPHY_REG_LIST_START
		/* turn off all test points in cgpaio block to avoid conflict,disable tp0 to tp15 */
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG2, 0)
			/* disable tp16 to tp31 */
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG3, 0)
			/* Disable muxsel0 and muxsel1 test points */
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG4, 0)
			/* Disable muxsel2 and muxselgpaio test points */
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG5, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
	} else {
		ACPHY_REG_LIST_START
			/* first powerup the CGPAIO block */
			MOD_RADIO_REG_ENTRY(pi, RF2, CGPAIO_CFG1, cgpaio_pu, 1)
		/* turn off all test points in cgpaio block to avoid conflict,disable tp0 to tp15 */
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG2, 0)
			/* disable tp16 to tp31 */
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG3, 0)
			/* Disable muxsel0 and muxsel1 test points */
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG4, 0)
			/* Disable muxsel2 and muxselgpaio test points */
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG5, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
	/* Powerdown rcal. This is one of the enable pins to AND gate in cgpaio block */
	MOD_RADIO_REG(pi, RF2, RCAL_CFG, pu, 0);



	FOREACH_ACTV_CORE(pi, core_mask, core) {

		if (for_iqcal == 0) {
			if (PHY_IPA(pi)) {
				if (CHSPEC_IS5G(pi->radio_chanspec)) {
					MOD_RADIO_REGC(pi, IQCAL_CFG1, core, sel_sw, 0x2);
					MOD_RADIO_REGC(pi, IQCAL_CFG1, core, sel_ext_tssi, 0x0);
					MOD_RADIO_REGC(pi, GE16_OVR21, core,
						ovr_pa5g_ctrl_tssi_sel, 0x1);
					MOD_RADIO_REGC(pi, TX5G_TSSI, core,
						pa5g_ctrl_tssi_sel, 0x0);
				} else {
					MOD_RADIO_REGC(pi, IQCAL_CFG1, core, sel_sw, 0x0);
					MOD_RADIO_REGC(pi, IQCAL_CFG1, core, sel_ext_tssi, 0x0);
					MOD_RADIO_REGC(pi, GE16_OVR21, core,
						ovr_pa2g_ctrl_tssi_sel, 0x1);
					MOD_RADIO_REGC(pi, PA2G_TSSI, core,
						pa2g_ctrl_tssi_sel, 0x0);
				}
			} else  {
				if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) &&
				    CHSPEC_IS5G(pi->radio_chanspec)) {
					MOD_RADIO_REGC(pi, IQCAL_CFG1, core, sel_sw, 0x3);
				} else {
					MOD_RADIO_REGC(pi, IQCAL_CFG1, core, sel_sw, 0x1);
				}
			}
		} else {
			MOD_RADIO_REGC(pi, IQCAL_CFG1, core, sel_sw,
			               (CHSPEC_IS5G(pi->radio_chanspec)) ? 0x2 : 0x0);
		}
		if (!PHY_IPA(pi)) {
			MOD_RADIO_REGC(pi, IQCAL_CFG1, core, sel_ext_tssi, for_iqcal == 0);

			switch (core) {
			case 0:
				MOD_RADIO_REG(pi, RF2, CGPAIO_CFG4, cgpaio_tssi_muxsel0, 0x1);
				break;
			case 1:
				MOD_RADIO_REG(pi, RF2, CGPAIO_CFG4, cgpaio_tssi_muxsel1, 0x1);
				break;
			case 2:
				MOD_RADIO_REG(pi, RF2, CGPAIO_CFG5, cgpaio_tssi_muxsel2, 0x1);
				break;
			case 3:
				MOD_RADIO_REG(pi, RF2, CGPAIO_CFG5, cgpaio_tssi_muxsel2, 0x1);
				break;
			default:
				ASSERT(0);
			}
		}

		MOD_RADIO_REGC(pi, IQCAL_CFG1, core, tssi_GPIO_ctrl, 0);
		MOD_RADIO_REGC(pi, TESTBUF_CFG1, core, GPIO_EN, 0);

		/* Reg conflict with 2069 rev 16 */
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0) {
			MOD_RADIO_REGC(pi, TX5G_TSSI, core, pa5g_ctrl_tssi_sel, for_iqcal);
			MOD_RADIO_REGC(pi, OVR20, core, ovr_pa5g_ctrl_tssi_sel, 1);
		} else if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) ||
			(RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2)) {
			MOD_RADIO_REGC(pi, TX5G_TSSI, core, pa5g_ctrl_tssi_sel, for_iqcal);
			MOD_RADIO_REGC(pi, GE16_OVR21, core, ovr_pa5g_ctrl_tssi_sel, 1);
			MOD_RADIO_REGC(pi, PA2G_TSSI, core, pa2g_ctrl_tssi_sel, for_iqcal);
			MOD_RADIO_REGC(pi, GE16_OVR21, core, ovr_pa2g_ctrl_tssi_sel, 1);
		}

		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
			MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0x0);
			/* This bit is supposed to be controlled by phy direct control line.
			 * Please check: http://jira.broadcom.com/browse/HW11ACRADIO-45
			 */
			MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x2);
		}

	}
}

void
wlc_phy_tssi_radio_setup_acphy_20696(phy_info_t *pi, uint8 for_iqcal)
{
	/* ported the code from Iguana PHY_BRANCH_2_100 */
	/* save radio config before changing it */
	uint8 core;

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
		MOD_RADIO_REG_20696(pi, IQCAL_OVR1, core,
			ovr_iqcal_PU_tssi, 1);
		if (CHSPEC_IS5G(pi->radio_chanspec)) {
			MOD_RADIO_REG_20696(pi, IQCAL_CFG1, core, iqcal_sel_sw, 0x3);
		} else {
			MOD_RADIO_REG_20696(pi, IQCAL_CFG1, core, iqcal_sel_sw, 0x1);
		}
		MOD_RADIO_REG_20696(pi, IQCAL_CFG1, core, iqcal_sel_ext_tssi, 0x1);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG1, core, iqcal_PU_tssi, 0x0);
		MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0);
		MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0);
		MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x1);
		MOD_RADIO_REG_20696(pi, LPF_REG8, core, lpf_sw_adc_test, 0x0);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG1, core, iqcal_PU_iqcal, 0x0);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG1, core, iqcal_tssi_GPIO_ctrl, 0x0);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG2, core, iqcal_tssi_cm_center, 0x1);
		MOD_RADIO_REG_20696(pi, IQCAL_IDAC, core, iqcal_tssi_bias, 0x0);
		MOD_RADIO_REG_20696(pi, IQCAL_GAIN_RFB, core, iqcal_rfb, 0x200);
		MOD_RADIO_REG_20696(pi, IQCAL_GAIN_RIN, core, iqcal_rin, 0x800);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG5, core, wbpga_pu, 0x0);
		MOD_RADIO_REG_20696(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu, 0x1);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG5, core, loopback_bias_pu, 0x1);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG5, core, wbpga_cmref_iqbuf, 0x0);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG5, core, wbpga_cmref_half, 0x1);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG6, core, wbpga_cmref, 0x14);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG6, core, wbpga_bias, 0x14);

		/* Selection between auxpga or iqcal(wbpga) */
		MOD_RADIO_REG_20696(pi, IQCAL_CFG4, core, iqcal2adc, 0x0);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG4, core, auxpga2adc, 0x1);

		MOD_RADIO_REG_20696(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 0x1);
		MOD_RADIO_REG_20696(pi, TESTBUF_CFG1, core, testbuf_PU, 0x1);

		MOD_RADIO_REG_20696(pi, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 0x1);
		MOD_RADIO_REG_20696(pi, TESTBUF_CFG1, core, testbuf_sel_test_port, 0x2);

		MOD_RADIO_REG_20696(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1);
		MOD_RADIO_REG_20696(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1);

		MOD_RADIO_REG_20696(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 0x1);
		MOD_RADIO_REG_20696(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x2);
	}
}


void
phy_ac_tssi_loopback_path_setup(phy_info_t *pi, uint8 for_iqcal)
{
	wlc_phy_tssi_phy_setup_acphy(pi, for_iqcal);
	if (TINY_RADIO(pi)) {
		wlc_phy_tssi_radio_setup_acphy_tiny(pi, pi->sh->hw_phyrxchain, for_iqcal);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
		wlc_phy_tssi_radio_setup_acphy_20694(pi, for_iqcal);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
		wlc_phy_tssi_radio_setup_acphy_28nm(pi, for_iqcal);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20696_ID)) {
		wlc_phy_tssi_radio_setup_acphy_20696(pi, for_iqcal);
	} else {
		wlc_phy_tssi_radio_setup_acphy(pi, pi->sh->hw_phyrxchain, for_iqcal);
	}
}
void
wlc_phy_txpwrctrl_set_idle_tssi_acphy(phy_info_t *pi, int16 idle_tssi, uint8 core)
{
	bool flag2rangeon =
		((CHSPEC_IS2G(pi->radio_chanspec) &&
		phy_tpc_get_tworangetssi2g(pi->tpci)) || (CHSPEC_IS5G(pi->radio_chanspec) &&
		phy_tpc_get_tworangetssi5g(pi->tpci))) && PHY_IPA(pi);

	/* set idle TSSI in 2s complement format (max is 0x1ff) */
	switch (core) {
	case 0:
		MOD_PHYREG(pi, TxPwrCtrlIdleTssi_path0, idleTssi0, idle_tssi);
		if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
			MOD_PHYREG(pi, TxPwrCtrlIdleTssi_second_path0, idleTssi_second0, idle_tssi);
		}
		break;
	case 1:
		MOD_PHYREG(pi, TxPwrCtrlIdleTssi_path1, idleTssi1, idle_tssi);
		break;
	case 2:
		MOD_PHYREG(pi, TxPwrCtrlIdleTssi_path2, idleTssi2, idle_tssi);
		break;
	case 3:
		MOD_PHYREG(pi, TxPwrCtrlIdleTssi_path3, idleTssi3, idle_tssi);
		break;
	}

	/* Only 4335 and 4350 has 2nd idle-tssi */
	if (((ACMAJORREV_1(pi->pubpi->phy_rev) || ACMAJORREV_2(pi->pubpi->phy_rev) ||
		ACMAJORREV_4(pi->pubpi->phy_rev)) && BF3_TSSI_DIV_WAR(pi->u.pi_acphy)) ||
		flag2rangeon) {
		switch (core) {
		case 0:
			MOD_PHYREG(pi, TxPwrCtrlIdleTssi_second_path0,
			           idleTssi_second0, idle_tssi);
			break;
		case 1:
			MOD_PHYREG(pi, TxPwrCtrlIdleTssi_second_path1,
			           idleTssi_second1, idle_tssi);
			break;
		}
	} else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
	           ACMAJORREV_33(pi->pubpi->phy_rev) ||
	           ACMAJORREV_37(pi->pubpi->phy_rev)) {
		switch (core) {
		case 0:
			MOD_PHYREG(pi, TxPwrCtrlIdleTssi_second_path0, idleTssi_second0, idle_tssi);
			MOD_PHYREG(pi, TxPwrCtrlIdleTssi_third_path0, idleTssi_third0, idle_tssi);
			break;
		case 1:
			MOD_PHYREG(pi, TxPwrCtrlIdleTssi_second_path1, idleTssi_second1, idle_tssi);
			MOD_PHYREG(pi, TxPwrCtrlIdleTssi_third_path1, idleTssi_third1, idle_tssi);
			break;
		case 2:
			MOD_PHYREG(pi, TxPwrCtrlIdleTssi_second_path2, idleTssi_second2, idle_tssi);
			MOD_PHYREG(pi, TxPwrCtrlIdleTssi_third_path2, idleTssi_third2, idle_tssi);
			break;
		case 3:
			MOD_PHYREG(pi, TxPwrCtrlIdleTssi_second_path3, idleTssi_second3, idle_tssi);
			MOD_PHYREG(pi, TxPwrCtrlIdleTssi_third_path3, idleTssi_third3, idle_tssi);
			break;
		}
	}
}

static int8
phy_ac_tssical_get_visible_thresh(phy_type_tssical_ctx_t *ctx)
{
	phy_ac_tssical_info_t *info = (phy_ac_tssical_info_t *)ctx;
	phy_info_t *pi = info->pi;
	return wlc_phy_tssivisible_thresh_acphy(pi);
}

int8
wlc_phy_tssivisible_thresh_acphy(phy_info_t *pi)
{
	int8 visi_thresh_qdbm;
	uint16 channel = CHSPEC_CHANNEL(pi->radio_chanspec);

	if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
		/* J28 */
		if ((BFCTL(pi->u.pi_acphy) == 3) && (BF3_FEMCTRL_SUB(pi->u.pi_acphy) == 2)) {
			visi_thresh_qdbm = 30;  /* 7.5*4 */
		} else {
			if (IS_X52C_BOARDTYPE(pi))
				visi_thresh_qdbm = 5*4;
			else if (IS_X29C_BOARDTYPE(pi) && (channel >= 149))
				visi_thresh_qdbm = 22; /* 5.5 dBm */
			else
				visi_thresh_qdbm = 6*4;
		}
	}
	else if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
		if (pi->olpci->olpc_thresh != 0) {
			visi_thresh_qdbm = pi->olpci->olpc_thresh;
		} else {
			visi_thresh_qdbm = 7*4;
		}

		if (pi->u.pi_acphy->rx5ggainwar) {
			/* rx5ggainwar is a srom flag to indicate */
			/* whether the board is X14 for 4350 */
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				visi_thresh_qdbm = 8*4;
			} else {
				visi_thresh_qdbm = 6*4;
			}
		}

		if (pi->olpci->disable_olpc == 1)
			visi_thresh_qdbm = WL_RATE_DISABLED;
	}
	else if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
		if (phy_tpc_get_2g_pdrange_id(pi->tpci) == 24)
			visi_thresh_qdbm = 4*4;
		else
			visi_thresh_qdbm = 7*4;
	}
	else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		visi_thresh_qdbm = (CHSPEC_IS2G(pi->radio_chanspec) ? pi->min_txpower :
			pi->min_txpower_5g) * WLC_TXPWR_DB_FACTOR;
#if (!defined(WLOLPC) || defined(WLOLPC_DISABLED)) && defined(WLC_TXCAL)
		/* if training based OLPC is disabled
		 * but txcal based olpc idx is not valid
		 * disable OLPC
		 */
		if (!pi->olpci->olpc_idx_valid &&
			pi->olpci->olpc_idx_in_use) {
				visi_thresh_qdbm = WL_RATE_DISABLED;
		}
#endif	/* (!WLOLPC || WLOLPC_DISABLED) && WLC_TXCAL */
	}
	else if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
#if defined(WLC_TXCAL)
		if (pi->olpci->disable_olpc == 1)
			visi_thresh_qdbm = WL_RATE_DISABLED;
		else if (pi->olpci->olpc_idx_in_use && pi->olpci->olpc_idx_valid)
			visi_thresh_qdbm = pi->olpci->olpc_thresh;
		else
			visi_thresh_qdbm = pi->min_txpower * WLC_TXPWR_DB_FACTOR;
#else
		visi_thresh_qdbm = pi->min_txpower * WLC_TXPWR_DB_FACTOR;
#endif
	}
	else visi_thresh_qdbm = WL_RATE_DISABLED;
	if (ACMAJORREV_3(pi->pubpi->phy_rev) ||
			ACMAJORREV_5(pi->pubpi->phy_rev))
	   visi_thresh_qdbm = -128;
	return visi_thresh_qdbm;
}

#if defined(BCMINTERNAL) || defined(WLTEST)
int16
wlc_phy_test_tssi_acphy(phy_info_t *pi, int8 ctrl_type, int8 pwr_offs)
{
	int16 tssi = 0;
	int16 temp = 0;
	int Npt, Npt_log2, i;
	bool suspend = FALSE;
	wlc_phy_conditional_suspend(pi, &suspend);
	Npt_log2 = READ_PHYREGFLD(pi, TxPwrCtrlNnum, Npt_intg_log2);
	Npt = 1 << Npt_log2;

	switch (ctrl_type & 0x7) {
	case 0:
	case 1:
	case 2:
	case 3:
		for (i = 0; i < Npt; i++) {
			OSL_DELAY(10);
			temp = READ_PHYREGCE(pi, TssiVal_path, ctrl_type) & 0x3ff;
			temp -= (temp >= 512) ? 1024 : 0;
			tssi += temp;
		}
		tssi = tssi >> Npt_log2;
		break;
	default:
		tssi = -1024;
	}
	wlc_phy_conditional_resume(pi, &suspend);
	return (tssi);
}

int16
wlc_phy_test_idletssi_acphy(phy_info_t *pi, int8 ctrl_type)
{
	int16 idletssi = INVALID_IDLETSSI_VAL;
	bool suspend = FALSE;

	/* Suspend MAC if haven't done so */
	wlc_phy_conditional_suspend(pi, &suspend);

	switch (ctrl_type & 0x7) {
	case 0:
	case 1:
	case 2:
	case 3:
		idletssi = READ_PHYREGCE(pi, TxPwrCtrlIdleTssi_path, ctrl_type) & 0x3ff;
		idletssi -= (idletssi >= 512) ? 1024 : 0;
		break;
	default:
		idletssi = INVALID_IDLETSSI_VAL;
	}

	/* Resume MAC */
	wlc_phy_conditional_resume(pi, &suspend);

	return (idletssi);
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

static void
wlc_phy_get_tssisens_min_acphy(phy_type_tssical_ctx_t *ctx, int8 *tssiSensMinPwr)
{
	phy_ac_tssical_info_t *info = (phy_ac_tssical_info_t *)ctx;
	phy_info_t *pi = info->pi;
	tssiSensMinPwr[0] = READ_PHYREGFLD(pi, TxPwrCtrlCore0TSSISensLmt, tssiSensMinPwr);
	if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
		tssiSensMinPwr[1] = READ_PHYREGFLD(pi, TxPwrCtrlCore1TSSISensLmt, tssiSensMinPwr);
		if (PHYCORENUM(pi->pubpi->phy_corenum) > 2)
			tssiSensMinPwr[2] = READ_PHYREGFLD(pi, TxPwrCtrlCore2TSSISensLmt,
				tssiSensMinPwr);
	}
}

#ifdef WLC_TXCAL
uint16
wlc_phy_adjusted_tssi_acphy(phy_info_t *pi, uint8 core_num)
{
	uint16 adj_tssi = 0;
	int16 tssi_OB, idletssi_OB;
	uint8 pos_slope;
	int16 tssi_reg, idletssi_reg;
	tssi_reg = READ_PHYREGCE(pi, TssiVal_path, core_num) & 0x3ff;
	if (tssi_reg >= 512)
		tssi_OB = tssi_reg - 511;
	else
		tssi_OB = tssi_reg + 512;
	idletssi_reg = READ_PHYREGCE(pi, TxPwrCtrlIdleTssi_path, core_num) & 0x3ff;
	if (idletssi_reg >= 512)
		idletssi_OB = idletssi_reg - 511;
	else
		idletssi_OB = idletssi_reg + 512;
	pos_slope = READ_PHYREGFLD(pi, TSSIMode, tssiPosSlope);
	if (pos_slope)
		adj_tssi = idletssi_OB - tssi_OB  + ((1 << 10)-1);
	else
		adj_tssi = tssi_OB - idletssi_OB  + ((1 << 10)-1);
	return adj_tssi;
}
uint8
wlc_phy_apply_pwr_tssi_tble_chan_acphy(phy_info_t *pi)
{
	txcal_pwr_tssi_lut_t *LUT_pt;
	txcal_pwr_tssi_lut_t *LUT_root;
	uint8 chan_num = CHSPEC_CHANNEL(pi->radio_chanspec);
	uint8 flag_chan_found = 0;
	txcal_root_pwr_tssi_t *pi_txcal_root_pwr_tssi_tbl = pi->txcali->txcal_root_pwr_tssi_tbl;
	if (CHSPEC_IS2G(pi->radio_chanspec))
		LUT_root = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_2G;
	else if (CHSPEC_IS80(pi->radio_chanspec))
		LUT_root = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_5G80;
	else if (CHSPEC_IS40(pi->radio_chanspec))
		LUT_root = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_5G40;
	else
		LUT_root = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_5G20;
	if (LUT_root->txcal_pwr_tssi->channel == 0) {
		/* if no txcal table is present for 2G and 5G20, apply paparam */
		/* For 5G40/80, apply 20mhz txcal table */
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			wlc_phy_txcal_apply_pa_params(pi);
			return BCME_OK;
		} else {
			if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
				wlc_phy_txcal_apply_pa_params(pi);
				return BCME_OK;
			} else {
				LUT_root = pi_txcal_root_pwr_tssi_tbl->root_pwr_tssi_lut_5G20;
				if (LUT_root->txcal_pwr_tssi->channel == 0) {
					wlc_phy_txcal_apply_pa_params(pi);
					return BCME_OK;
				}
			}
		}
	}
	LUT_pt = LUT_root;
	while (LUT_pt->next_chan != 0) {
		/* Go over all the entries in the list */
		if (LUT_pt->txcal_pwr_tssi->channel == chan_num) {
			flag_chan_found = 1;
			break;
		}
		if ((LUT_pt->txcal_pwr_tssi->channel < chan_num) &&
		    (LUT_pt->next_chan->txcal_pwr_tssi->channel > chan_num)) {
			flag_chan_found = 2;
			break;
		}
		LUT_pt = LUT_pt->next_chan;
	}
	if (LUT_pt->txcal_pwr_tssi->channel == chan_num) {
		/* In case only one entry is in the list */
		flag_chan_found = 1;
	}
	switch (flag_chan_found) {
	case 0:
		/* Channel not found in linked list or not between two channels */
		/* Then pick the closest one */
		if (chan_num < LUT_root->txcal_pwr_tssi->channel)
			LUT_pt = LUT_root;
		wlc_phy_txcal_apply_pwr_tssi_tbl(pi, LUT_pt->txcal_pwr_tssi);
		pi->txcali->txcal_status = 2;
		break;
	case 1:
		/* Channel found */
		wlc_phy_txcal_apply_pwr_tssi_tbl(pi, LUT_pt->txcal_pwr_tssi);
		pi->txcali->txcal_status = 1;
		break;
	case 2:
		/* Channel is in between two channels, do interpolation */
		/* ---- need to verify goodness of interpolation */
		wlc_phy_estpwrlut_intpol_acphy(pi, chan_num,
			LUT_pt->txcal_pwr_tssi, LUT_pt->next_chan->txcal_pwr_tssi);
		pi->txcali->txcal_status = 2;
		break;
	}
	return BCME_OK;
}
#endif	/* WLC_TXCAL */

void
wlc_phy_set_tssisens_lim_acphy(phy_info_t *pi, uint8 override)
{
	uint16 tssi_limit;
	int8 visi_thresh_qdbm = WL_RATE_DISABLED;
#if defined(PHYCAL_CACHING)
	acphy_calcache_t *cache;
	ch_calcache_t *ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);
#endif
	if (override) {
		/* Override mode, always write tssi visible thresh */
		visi_thresh_qdbm = wlc_phy_tssivisible_thresh_acphy(pi);
	}
#if defined(WLC_TXCAL)
	else if (pi->olpci->olpc_idx_valid && pi->olpci->olpc_idx_in_use) {
		/* txcal based OLPC is in use, set tssi visible thresh to let OLPC work */
		visi_thresh_qdbm = wlc_phy_tssivisible_thresh_acphy(pi);
	}
#endif	/* WLC_TXCAL */
#if defined(PHYCAL_CACHING)
	else {
		if (ctx) {
			if (ctx->valid) {
				cache = &ctx->u.acphy_cache;
				if (cache->olpc_caldone) {
					/* If cal is done, write correct visibility thresh
					 * else, write -127
					 */
					visi_thresh_qdbm = wlc_phy_tssivisible_thresh_acphy(pi);
				}
			}
		}
	}
#endif
	tssi_limit = (127 << 8) + (visi_thresh_qdbm & 0xFF);
	ACPHYREG_BCAST(pi, TxPwrCtrlCore0TSSISensLmt, tssi_limit);
}

void
phy_ac_tssical_idle(phy_info_t *pi)
{
	/*
	 *     Idle TSSI & TSSI-to-dBm Mapping Setup
	 */
	const uint16 tbl_cookie = TXCAL_CACHE_VALID;
#if defined(PHYCAL_CACHING)
	ch_calcache_t *ctx = NULL;
	ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);
#endif
	wlc_phy_susp2tx_cts2self(pi, 1550);
	if ((pi->radar_percal_mask & 0x8) != 0)
		pi->u.pi_acphy->radar_cal_active = TRUE;

	/* Idle TSSI determination once right after join/up/assoc */
	wlc_phy_txpwrctrl_idle_tssi_meas_acphy(pi);

	/* done with multi-phase cal, reset phase */
	pi->first_cal_after_assoc = FALSE;

	wlc_phy_table_write_acphy(pi, wlc_phy_get_tbl_id_iqlocal(pi, 0), 1,
	  IQTBL_CACHE_COOKIE_OFFSET, 16, &tbl_cookie);

#if defined(PHYCAL_CACHING)
	if (ctx)
		wlc_phy_cal_cache((wlc_phy_t *)pi);
#endif

	wlc_phy_cal_perical_mphase_reset(pi);
}
