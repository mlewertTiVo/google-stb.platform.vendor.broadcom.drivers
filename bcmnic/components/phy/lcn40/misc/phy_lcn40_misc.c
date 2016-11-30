/*
 * LCN40PHY Miscellaneous modules implementation
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
 * $Id: phy_lcn40_misc.c 625314 2016-03-16 04:44:41Z vyass $
 */
#include <typedefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_btcx.h>
#include <phy_misc.h>
#include <phy_type_misc.h>
#include <phy_lcn40_misc.h>
#include <phy_antdiv_api.h>
#include <phy_utils_reg.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_lcn40.h>
#include <wlc_phyreg_lcn40.h>
/* TODO: all these are going away... > */
#endif


/* module private states */
struct phy_lcn40_misc_info {
	phy_info_t *pi;
	phy_lcn40_info_t *lcn40i;
	phy_misc_info_t *cmn_info;
};

/* Local Functions */
#if defined(BCMDBG) || defined(WLTEST)
static void phy_lcn40_init_test(phy_type_misc_ctx_t *ctx, bool encals);
static int phy_lcn40_misc_test_freq_accuracy(phy_type_misc_ctx_t *ctx, int channel);
#endif /* defined(BCMDBG) || defined(WLTEST) */
static uint32 phy_lcn40_rx_iq_est(phy_type_misc_ctx_t *ctx, uint8 samples, uint8 antsel,
	uint8 resolution, uint8 lpf_hpc, uint8 dig_lpf, uint8 gain_correct,
                                uint8 extra_gain_3dB, uint8 wait_for_crs, uint8 force_gain_type);
static void phy_lcn40_misc_set_deaf(phy_type_misc_ctx_t *ctx, bool user_flag);
static void phy_lcn40_misc_clear_deaf(phy_type_misc_ctx_t *ctx, bool user_flag);
static void phy_lcn40_iovar_tx_tone(phy_type_misc_ctx_t *ctx, int32 int_val);
static void phy_lcn40_iovar_txlo_tone(phy_type_misc_ctx_t *ctx);
static int phy_lcn40_iovar_get_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 *ret_int_ptr,
	int32 int_val, int err);
static int phy_lcn40_iovar_set_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 int_val, int err);
static bool wlc_phy_get_rxgainerr_lcn40phy(phy_type_misc_ctx_t *ctx, int16 *gainerr);

/* register phy type specific implementation */
phy_lcn40_misc_info_t *
BCMATTACHFN(phy_lcn40_misc_register_impl)(phy_info_t *pi, phy_lcn40_info_t *lcn40i,
	phy_misc_info_t *cmn_info)
{
	phy_lcn40_misc_info_t *lcn40_info;
	phy_type_misc_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((lcn40_info = phy_malloc(pi, sizeof(phy_lcn40_misc_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	lcn40_info->pi = pi;
	lcn40_info->lcn40i = lcn40i;
	lcn40_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = lcn40_info;
#if defined(BCMDBG) || defined(WLTEST)
	fns.phy_type_misc_test_init = phy_lcn40_init_test;
	fns.phy_type_misc_test_freq_accuracy = phy_lcn40_misc_test_freq_accuracy;
#endif /* defined(BCMDBG) || defined(WLTEST) */
	fns.phy_type_misc_rx_iq_est = phy_lcn40_rx_iq_est;
	fns.phy_type_misc_set_deaf = phy_lcn40_misc_set_deaf;
	fns.phy_type_misc_clear_deaf = phy_lcn40_misc_clear_deaf;
	fns.phy_type_misc_iovar_tx_tone = phy_lcn40_iovar_tx_tone;
	fns.phy_type_misc_iovar_txlo_tone = phy_lcn40_iovar_txlo_tone;
	fns.phy_type_misc_iovar_get_rx_iq_est = phy_lcn40_iovar_get_rx_iq_est;
	fns.phy_type_misc_iovar_set_rx_iq_est = phy_lcn40_iovar_set_rx_iq_est;
	fns.phy_type_misc_get_rxgainerr = wlc_phy_get_rxgainerr_lcn40phy;

	if (phy_misc_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_misc_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return lcn40_info;

	/* error handling */
fail:
	if (lcn40_info != NULL)
		phy_mfree(pi, lcn40_info, sizeof(phy_lcn40_misc_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn40_misc_unregister_impl)(phy_lcn40_misc_info_t *lcn40_info)
{
	phy_info_t *pi;
	phy_misc_info_t *cmn_info;

	ASSERT(lcn40_info);
	pi = lcn40_info->pi;
	cmn_info = lcn40_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_misc_unregister_impl(cmn_info);

	phy_mfree(pi, lcn40_info, sizeof(phy_lcn40_misc_info_t));
}


/* ********************************************** */
/* Function table registred function */
/* ********************************************** */
#if defined(BCMDBG) || defined(WLTEST)
static void
phy_lcn40_init_test(phy_type_misc_ctx_t *ctx, bool encals)
{
	phy_lcn40_misc_info_t *info = (phy_lcn40_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;

	/* Force WLAN antenna */
	wlc_btcx_override_enable(pi);
	/* Disable tx power control */
	wlc_lcn40phy_set_tx_pwr_ctrl(pi, LCN40PHY_TX_PWR_CTRL_OFF);
	/* Recalibrate for this channel */
	wlc_lcn40phy_calib_modes(pi, PHY_FULLCAL);
}

static int
phy_lcn40_misc_test_freq_accuracy(phy_type_misc_ctx_t *ctx, int channel)
{
	phy_lcn40_misc_info_t *info = (phy_lcn40_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;

	if (channel == 0) {
		/* Restore the 24dB scaler */
		PHY_REG_MOD(pi, LCN40PHY, bbmult0, bbmult0_enable, 1);
		PHY_REG_MOD(pi, LCN40PHY, bbmult0, bbmult0_coeff, 64);
		wlc_lcn40phy_set_bbmult(pi, 64);
		wlc_lcn40phy_stop_tx_tone(pi);
		wlc_lcn40phy_set_tx_pwr_ctrl(pi, LCN40PHY_TX_PWR_CTRL_HW);
	} else {
		/* Need to scale up tx tone by 24 dB */
		PHY_REG_MOD(pi, LCN40PHY, bbmult0, bbmult0_enable, 1);
		PHY_REG_MOD(pi, LCN40PHY, bbmult0, bbmult0_coeff, 255);
		wlc_lcn40phy_set_bbmult(pi, 255);
		wlc_lcn40phy_start_tx_tone(pi, 0, 112, 0);
		wlc_lcn40phy_set_tx_pwr_by_index(pi, (int)94);
	}

	return BCME_OK;
}
#endif /* defined(BCMDBG) || defined(WLTEST) */

static uint32 phy_lcn40_rx_iq_est(phy_type_misc_ctx_t *ctx, uint8 samples, uint8 antsel,
	uint8 resolution, uint8 lpf_hpc, uint8 dig_lpf, uint8 gain_correct,
                                uint8 extra_gain_3dB, uint8 wait_for_crs, uint8 force_gain_type) {
	phy_lcn40_misc_info_t *info = (phy_lcn40_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	phy_iq_est_t est[PHY_CORE_MAX];
	uint32 cmplx_pwr[PHY_CORE_MAX];
	int8 noise_dbm_ant[PHY_CORE_MAX];
	int16	tot_gain[PHY_CORE_MAX];
	int16 noise_dBm_ant_fine[PHY_CORE_MAX];
#if defined(WLTEST)
	uint16 log_num_samps;
	uint16 num_samps;
	uint8 wait_time = 32;
	uint8 i, extra_gain_1dB = 0;
	uint32 result = 0;
#endif /* WLTEST */
	bool sampling_in_progress = (pi->phynoise_state != 0);
	uint16 crsmin_pwr[PHY_CORE_MAX];
#if defined(WLTEST)
	uint8 save_antsel;
#endif /* WLTEST */
	if (sampling_in_progress) {
		PHY_ERROR(("%s: sampling_in_progress\n", __FUNCTION__));

		return 0;
	}

	/* Extra INITgain is supported only for HTPHY currently */
	if (extra_gain_3dB > 0) {
		extra_gain_3dB = 0;
		PHY_ERROR(("%s: Extra INITgain not supported for this phy.\n",
		           __FUNCTION__));
	}

	pi->phynoise_state |= PHY_NOISE_STATE_MON;
#if defined(WLTEST)
	/* choose num_samps to be some power of 2 */
	log_num_samps = samples;
	num_samps = 1 << log_num_samps;
#endif /* WLTEST */
	bzero((uint8 *)est, sizeof(est));
	bzero((uint8 *)cmplx_pwr, sizeof(cmplx_pwr));
	bzero((uint8 *)noise_dbm_ant, sizeof(noise_dbm_ant));
	bzero((uint16 *)crsmin_pwr, sizeof(crsmin_pwr));
	bzero((uint16 *)noise_dBm_ant_fine, sizeof(noise_dBm_ant_fine));
	bzero((int16 *)tot_gain, sizeof(tot_gain));

	/* get IQ power measurements */
#if defined(WLTEST)
	phy_antdiv_get_rx(pi, &save_antsel);
	if (antsel <= 1)
		phy_antdiv_set_rx(pi, antsel);
	wlc_lcn40phy_rx_power(pi, num_samps, wait_time, wait_for_crs, est);
	phy_antdiv_set_rx(pi, save_antsel);

	/* sum I and Q powers for each core, average over num_samps with rounding */
	ASSERT(PHYCORENUM(pi->pubpi->phy_corenum) <= PHY_CORE_MAX);
	FOREACH_CORE(pi, i) {
		cmplx_pwr[i] = ((est[i].i_pwr + est[i].q_pwr) +
			(1U << (log_num_samps-1))) >> log_num_samps;
	}

	/* convert in 1dB gain for gain adjustment */
	extra_gain_1dB = 3 * extra_gain_3dB;

	if (resolution == 0) {
		/* pi->phy_noise_win per antenna is updated inside */
		wlc_phy_noise_calc(pi, cmplx_pwr, noise_dbm_ant, extra_gain_1dB);

		pi->phynoise_state &= ~PHY_NOISE_STATE_MON;

		for (i = PHYCORENUM(pi->pubpi->phy_corenum); i >= 1; i--)
			result = (result << 8) | (noise_dbm_ant[i-1] & 0xff);

		return result;
	}
	else if (resolution == 1) {
		/* Reports power in finer resolution than 1 dB (currently 0.25 dB) */

		int16 noisefloor;

		wlc_phy_noise_calc_fine_resln(pi, cmplx_pwr, crsmin_pwr, noise_dBm_ant_fine,
		                              extra_gain_1dB, tot_gain);

		if ((gain_correct == 1) || (gain_correct == 2) || gain_correct == 3) {
			int16 gainerr[PHY_CORE_MAX];
			int16 gain_err_temp_adj;
			wlc_phy_get_rxgainerr_phy(pi, gainerr);

			wlc_phy_upd_gain_wrt_temp_phy(pi, &gain_err_temp_adj);

			FOREACH_CORE(pi, i) {
				/* gainerr is in 0.5dB units;
				 * need to convert to 0.25dB units
				 */
			    if (gain_correct == 1) {
			    gainerr[i] = gainerr[i] << 1;
				/* Apply gain correction */
				noise_dBm_ant_fine[i] -= gainerr[i];
				}
				noise_dBm_ant_fine[i] += gain_err_temp_adj;
			}
		}

		noisefloor = (CHSPEC_IS40(pi->radio_chanspec))?
			4*LCN40PHY_NOISE_FLOOR_40M : 4*LCN40PHY_NOISE_FLOOR_20M;

		FOREACH_CORE(pi, i) {
		if (noise_dBm_ant_fine[i] < noisefloor) {
					noise_dBm_ant_fine[i] = noisefloor;
			}
		}

		for (i = PHYCORENUM(pi->pubpi->phy_corenum); i >= 1; i--) {
			result = (result << 10) | (noise_dBm_ant_fine[i-1] & 0x3ff);
		}
		pi->phynoise_state &= ~PHY_NOISE_STATE_MON;
		return result;
	}
#endif /* #if defined(WLTEST) */

	pi->phynoise_state &= ~PHY_NOISE_STATE_MON;
	return 0;
}

static void phy_lcn40_iovar_tx_tone(phy_type_misc_ctx_t *ctx, int32 int_val)
{
	phy_lcn40_misc_info_t *info = (phy_lcn40_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;

	pi->phy_tx_tone_freq = (int32) int_val;

	if (pi->phy_tx_tone_freq == 0) {
		wlc_lcn40phy_stop_tx_tone(pi);
		wlapi_enable_mac(pi->sh->physhim);
	} else {
		pi->phy_tx_tone_freq = pi->phy_tx_tone_freq * 1000; /* Covert to Hz */
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		wlc_lcn40phy_set_tx_tone_and_gain_idx(pi);
	}
}

static void phy_lcn40_iovar_txlo_tone(phy_type_misc_ctx_t *ctx)
{
	phy_lcn40_misc_info_t *info = (phy_lcn40_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;

	pi->phy_tx_tone_freq = 0;
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	wlc_lcn40phy_set_tx_tone_and_gain_idx(pi);
}

static int phy_lcn40_iovar_get_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 *ret_int_ptr,
	int32 int_val, int err)
{
	phy_lcn40_misc_info_t *info = (phy_lcn40_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	bool suspend;

	if (!pi->sh->up) {
		err = BCME_NOTUP;
		return err;
	}

	/* make sure bt-prisel is on WLAN side */
	wlc_phy_btcx_wlan_critical_enter(pi);

	suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);
	if (!suspend) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
	}

	phy_utils_phyreg_enter(pi);

	/* get IQ power measurements */
	*ret_int_ptr = wlc_phy_rx_iq_est(pi, pi->phy_rxiq_samps, pi->phy_rxiq_antsel,
	                                 pi->phy_rxiq_resln, pi->phy_rxiq_lpfhpc,
	                                 pi->phy_rxiq_diglpf,
	                                 pi->phy_rxiq_gain_correct,
	                                 pi->phy_rxiq_extra_gain_3dB, 0,
	                                 pi->phy_rxiq_force_gain_type);

	phy_utils_phyreg_exit(pi);

	if (!suspend)
		wlapi_enable_mac(pi->sh->physhim);
	wlc_phy_btcx_wlan_critical_exit(pi);

	return err;
}

static int phy_lcn40_iovar_set_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 int_val, int err)
{
	phy_lcn40_misc_info_t *info = (phy_lcn40_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint8 samples, antenna, resolution, lpf_hpc, dig_lpf;
	uint8 gain_correct, extra_gain_3dB, force_gain_type;
#if defined(WLTEST)
	uint8 index, elna, index_valid;
	phy_info_lcnphy_t *pi_lcn;
#endif
	extra_gain_3dB = (int_val >> 28) & 0xf;
	gain_correct = (int_val >> 24) & 0xf;
	lpf_hpc = (int_val >> 20) & 0x3;
	dig_lpf = (int_val >> 22) & 0x3;
	resolution = (int_val >> 16) & 0xf;
	samples = (int_val >> 8) & 0xff;
	antenna = int_val & 0xf;
	force_gain_type = (int_val >> 4) & 0xf;
#if defined(WLTEST)
	pi_lcn = wlc_phy_getlcnphy_common(pi);
	antenna = int_val & 0x7f;
	elna =  (int_val >> 20) & 0x1;
	index = (((int_val >> 28) & 0xF) << 3) | ((int_val >> 21) & 0x7);
	index_valid = (int_val >> 7) & 0x1;
	if (index_valid)
		pi_lcn->rxpath_index = index;
	else
		pi_lcn->rxpath_index = 0xFF;
	pi_lcn->rxpath_elna = elna;
#endif

	if (gain_correct > 4) {
		err = BCME_RANGE;
		return err;
	}

	if ((resolution != 0) && (resolution != 1)) {
		err = BCME_RANGE;
		return err;
	}

	if (samples < 10 || samples > 15) {
		err = BCME_RANGE;
		return err;
	}

	/* Limit max number of samples to 2^14 since Lcnphy RXIQ Estimator
	 * takes too much and variable time for more than that.
	*/
	samples = MIN(14, samples);

	if ((antenna != ANT_RX_DIV_FORCE_0) &&
		(antenna != ANT_RX_DIV_FORCE_1) &&
		(antenna != ANT_RX_DIV_DEF)) {
			err = BCME_RANGE;
			return err;
	}
	pi->phy_rxiq_samps = samples;
	pi->phy_rxiq_antsel = antenna;
	pi->phy_rxiq_resln = resolution;
	pi->phy_rxiq_lpfhpc = lpf_hpc;
	pi->phy_rxiq_diglpf = dig_lpf;
	pi->phy_rxiq_gain_correct = gain_correct;
	pi->phy_rxiq_extra_gain_3dB = extra_gain_3dB;
	pi->phy_rxiq_force_gain_type = force_gain_type;
	return err;
}

static void
phy_lcn40_misc_set_deaf(phy_type_misc_ctx_t *ctx, bool user_flag)
{
	phy_lcn40_misc_info_t *info = (phy_lcn40_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	/* Before being deaf, Force digi_gain to 0dB */
	uint16 save_digi_gain_ovr_val;
	phy_info_lcn40phy_t *pi_lcn40 = pi->u.pi_lcn40phy;

	save_digi_gain_ovr_val =
		PHY_REG_READ(pi, LCN40PHY, radioCtrl, digi_gain_ovr_val);
	pi_lcn40->save_digi_gain_ovr =
		PHY_REG_READ(pi, LCN40PHY, radioCtrl, digi_gain_ovr);
	/* Force digi_gain to 0dB */

	PHY_REG_LIST_START
		PHY_REG_MOD_ENTRY(LCN40PHY, radioCtrl, digi_gain_ovr_val, 0)
		PHY_REG_MOD_ENTRY(LCN40PHY, radioCtrl, digi_gain_ovr, 1)

		PHY_REG_MOD_ENTRY(LCN40PHY, agcControl4, c_agc_fsm_en, 0)
		PHY_REG_MOD_ENTRY(LCN40PHY, agcControl4, c_agc_fsm_en, 1)
	PHY_REG_LIST_EXECUTE(pi);
	OSL_DELAY(2);
	PHY_REG_MOD(pi, LCN40PHY, agcControl4, c_agc_fsm_en, 0);

	wlc_lcn40phy_deaf_mode(pi, user_flag);

	PHY_REG_MOD(pi, LCN40PHY, radioCtrl, digi_gain_ovr_val,
		save_digi_gain_ovr_val);
}

static void
phy_lcn40_misc_clear_deaf(phy_type_misc_ctx_t *ctx, bool user_flag)
{
	phy_lcn40_misc_info_t *info = (phy_lcn40_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	phy_info_lcn40phy_t *pi_lcn40 = pi->u.pi_lcn40phy;

	PHY_REG_MOD(pi, LCN40PHY, radioCtrl, digi_gain_ovr,
		pi_lcn40->save_digi_gain_ovr);
	wlc_lcn40phy_deaf_mode(pi, user_flag);
	/* Setting the saved value to default just to be safe */
	pi_lcn40->save_digi_gain_ovr = 0;
}

static bool
wlc_phy_get_rxgainerr_lcn40phy(phy_type_misc_ctx_t *ctx, int16 *gainerr)
{
#if defined(WLTEST)
	/* Returns rxgain error (read from srom) for current channel */
	phy_lcn40_misc_info_t *info = (phy_lcn40_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	phy_info_lcnphy_t *pi_lcn = wlc_phy_getlcnphy_common(pi);

	switch (wlc_phy_chanspec_bandrange_get(pi, pi->radio_chanspec)) {
		case WL_CHAN_FREQ_RANGE_2G:
			*gainerr = pi_lcn->srom_rxgainerr_2g;
			break;
	#ifdef BAND5G
		case WL_CHAN_FREQ_RANGE_5GL:
			/* 5 GHz low */
			*gainerr = pi_lcn->srom_rxgainerr_5gl;
			break;

		case WL_CHAN_FREQ_RANGE_5GM:
			/* 5 GHz middle */
			*gainerr = pi_lcn->srom_rxgainerr_5gm;
			break;

		case WL_CHAN_FREQ_RANGE_5GH:
			/* 5 GHz high */
			*gainerr = pi_lcn->srom_rxgainerr_5gh;
			break;
	#endif /* BAND5G */
		default:
			*gainerr = 0;
			break;
	}
#endif /* WLTEST */
	return 1;
}
