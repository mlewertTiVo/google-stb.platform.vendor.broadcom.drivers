/*
 * NPHY MISC module implementation
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
 * $Id: phy_n_misc.c 624514 2016-03-11 23:15:58Z vyass $
 */
#include <typedefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_btcx.h>
#include <phy_misc.h>
#include <phy_type_misc.h>
#include <phy_n.h>
#include <phy_n_misc.h>
#include <wlc_phyreg_n.h>
#include <wlc_phy_int.h>
#include <bcmdevs.h>
#include <phy_utils_reg.h>
#include <phy_n_info.h>
/* module private states */
struct phy_n_misc_info {
	phy_info_t *pi;
	phy_n_info_t *ni;
	phy_misc_info_t *cmn_info;
};

/*  Local Functions */
#if defined(BCMDBG) || defined(WLTEST)
static int phy_n_misc_test_freq_accuracy(phy_type_misc_ctx_t *ctx, int channel);
static void phy_n_misc_test_stop(phy_type_misc_ctx_t *ctx);
#endif
static uint32 phy_n_rx_iq_est(phy_type_misc_ctx_t *ctx, uint8 samples, uint8 antsel,
	uint8 resolution, uint8 lpf_hpc, uint8 dig_lpf, uint8 gain_correct,
                                uint8 extra_gain_3dB, uint8 wait_for_crs, uint8 force_gain_type);
static void phy_n_iovar_tx_tone(phy_type_misc_ctx_t *ctx, int32 int_val);
static void phy_n_misc_deaf_mode(phy_type_misc_ctx_t *ctx, bool user_flag);
static void phy_n_iovar_txlo_tone(phy_type_misc_ctx_t *ctx);
static int phy_n_iovar_get_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 *ret_int_ptr,
	int32 int_val, int err);
static int phy_n_iovar_set_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 int_val, int err);

/* register phy type specific implementation */
phy_n_misc_info_t *
BCMATTACHFN(phy_n_misc_register_impl)(phy_info_t *pi, phy_n_info_t *ni,
	phy_misc_info_t *cmn_info)
{
	phy_n_misc_info_t *n_info;
	phy_type_misc_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((n_info = phy_malloc(pi, sizeof(phy_n_misc_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	n_info->pi = pi;
	n_info->ni = ni;
	n_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = n_info;
	fns.phy_type_misc_rx_iq_est = phy_n_rx_iq_est;
	fns.phy_type_misc_set_deaf = phy_n_misc_deaf_mode;
	fns.phy_type_misc_clear_deaf = phy_n_misc_deaf_mode;
#if defined(BCMDBG) || defined(WLTEST)
	fns.phy_type_misc_test_freq_accuracy = phy_n_misc_test_freq_accuracy;
	fns.phy_type_misc_test_stop = phy_n_misc_test_stop;
#endif
	fns.phy_type_misc_iovar_tx_tone = phy_n_iovar_tx_tone;
	fns.phy_type_misc_iovar_txlo_tone = phy_n_iovar_txlo_tone;
	fns.phy_type_misc_iovar_get_rx_iq_est = phy_n_iovar_get_rx_iq_est;
	fns.phy_type_misc_iovar_set_rx_iq_est = phy_n_iovar_set_rx_iq_est;

	if (phy_misc_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_misc_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return n_info;

	/* error handling */
fail:
	if (n_info != NULL)
		phy_mfree(pi, n_info, sizeof(phy_n_misc_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_n_misc_unregister_impl)(phy_n_misc_info_t *n_info)
{
	phy_info_t *pi;
	phy_misc_info_t *cmn_info;

	ASSERT(n_info);
	pi = n_info->pi;
	cmn_info = n_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_misc_unregister_impl(cmn_info);

	phy_mfree(pi, n_info, sizeof(phy_n_misc_info_t));
}

#if defined(BCMDBG) || defined(WLTEST)
static int
phy_n_misc_test_freq_accuracy(phy_type_misc_ctx_t *ctx, int channel)
{
	phy_n_misc_info_t *info = (phy_n_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	phy_info_nphy_t *pi_nphy = pi->u.pi_nphy;
	int bcmerror = BCME_OK;

	if (channel == 0) {
		wlc_phy_stopplayback_nphy(pi);
		/* restore the old BBconfig, to restore resampler setting */
		phy_utils_write_phyreg(pi, NPHY_BBConfig, pi_nphy->nphy_saved_bbconf);
		wlc_phy_resetcca_nphy(pi);
	} else {
		/* Disable the re-sampler (in case we are in spur avoidance mode) */
		pi_nphy->nphy_saved_bbconf = phy_utils_read_phyreg(pi, NPHY_BBConfig);
		phy_utils_mod_phyreg(pi, NPHY_BBConfig, NPHY_BBConfig_resample_clk160_MASK, 0);
		/* use 151 since that should correspond to nominal tx output power */
		bcmerror = wlc_phy_tx_tone_nphy(pi, 0, 151, 0, 0, TRUE);
    }

	return bcmerror;
}

static void
phy_n_misc_test_stop(phy_type_misc_ctx_t *ctx)
{
	phy_n_misc_info_t *info = (phy_n_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	BCM_REFERENCE(pi);
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		phy_utils_and_phyreg(pi, (NPHY_TO_BPHY_OFF + BPHY_TEST), 0xfc00);
		/* BPHY_DDFS_ENABLE is removed in mimophy rev 3 */
		phy_utils_write_phyreg(pi, NPHY_bphytestcontrol, 0x0);
	}
}
#endif /* defined(BCMDBG) || defined(WLTEST) */

/* ********************************************** */
/* Function table registred function */
/* ********************************************** */
static uint32 phy_n_rx_iq_est(phy_type_misc_ctx_t *ctx, uint8 samples, uint8 antsel,
	uint8 resolution,
	uint8 lpf_hpc, uint8 dig_lpf, uint8 gain_correct,
                                uint8 extra_gain_3dB, uint8 wait_for_crs, uint8 force_gain_type) {
	phy_n_misc_info_t *info = (phy_n_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;

	phy_iq_est_t est[PHY_CORE_MAX];
	uint32 cmplx_pwr[PHY_CORE_MAX];
	int8 noise_dbm_ant[PHY_CORE_MAX];
	int16	tot_gain[PHY_CORE_MAX];
	int16 noise_dBm_ant_fine[PHY_CORE_MAX];
	uint16 log_num_samps, num_samps;
	uint8 wait_time = 32;
	bool sampling_in_progress = (pi->phynoise_state != 0);
	uint8 i, extra_gain_1dB = 0;
	uint32 result = 0;
	uint16 crsmin_pwr[PHY_CORE_MAX];
	uint16  phy_clip_state[2];
	uint16 clip_off[] = {0xffff, 0xffff};
	uint16 classif_state = 0;

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
	/* choose num_samps to be some power of 2 */
	log_num_samps = samples;
	num_samps = 1 << log_num_samps;

	bzero((uint8 *)est, sizeof(est));
	bzero((uint8 *)cmplx_pwr, sizeof(cmplx_pwr));
	bzero((uint8 *)noise_dbm_ant, sizeof(noise_dbm_ant));
	bzero((uint16 *)crsmin_pwr, sizeof(crsmin_pwr));
	bzero((uint16 *)noise_dBm_ant_fine, sizeof(noise_dBm_ant_fine));
	bzero((int16 *)tot_gain, sizeof(tot_gain));

	/* get IQ power measurements */

	classif_state = wlc_phy_classifier_nphy(pi, 0, 0);
	wlc_phy_classifier_nphy(pi, 3, 0);
	wlc_phy_clip_det_nphy(pi, 0, &phy_clip_state[0]);
	wlc_phy_clip_det_nphy(pi, 1, clip_off);

	/* get IQ power measurements */
	wlc_phy_rx_iq_est_nphy(pi, est, num_samps, wait_time, wait_for_crs);

	wlc_phy_clip_det_nphy(pi, 1, &phy_clip_state[0]);
	/* restore classifier settings and reenable MAC ASAP */
	wlc_phy_classifier_nphy(pi, NPHY_ClassifierCtrl_classifierSel_MASK, classif_state);

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

		if (CHIPID_4324X_EPA_FAMILY(pi)) {

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
				4*HTPHY_NOISE_FLOOR_40M : 4*HTPHY_NOISE_FLOOR_20M;

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
		else {
			PHY_ERROR(("%s: Fine-resolution reporting not supported\n", __FUNCTION__));
			pi->phynoise_state &= ~PHY_NOISE_STATE_MON;
			return 0;
		}
	}

	pi->phynoise_state &= ~PHY_NOISE_STATE_MON;
	return 0;
}

static void phy_n_iovar_tx_tone(phy_type_misc_ctx_t *ctx, int32 int_val)
{

	phy_n_misc_info_t *info = (phy_n_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;

	pi->phy_tx_tone_freq = (int32) int_val;

	if (pi->phy_tx_tone_freq == 0) {

		/* Restore back PAPD settings after stopping the tone */
		if (NREV_IS(pi->pubpi->phy_rev, LCNXN_BASEREV))
			wlc_phy_papd_enable_nphy(pi, TRUE);
		/* FIXME4324. Dont know why only 4324 hangs if mac is not suspended */
		if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
		wlc_phy_stopplayback_nphy(pi);
		wlc_phy_stay_in_carriersearch_nphy(pi, FALSE);
		if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))
			wlapi_enable_mac(pi->sh->physhim);
	} else {

		/* use 151 since that should correspond to nominal tx output power */
		/* Can not play tone with papd bit enabled */
		if (NREV_IS(pi->pubpi->phy_rev, LCNXN_BASEREV))
			wlc_phy_papd_enable_nphy(pi, FALSE);
		if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
		wlc_phy_stay_in_carriersearch_nphy(pi, TRUE);
		wlc_phy_tx_tone_nphy(pi, (uint32)int_val, 151, 0, 0, TRUE); /* play tone */
		if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))
			wlapi_enable_mac(pi->sh->physhim);

	}

}

static void phy_n_iovar_txlo_tone(phy_type_misc_ctx_t *ctx)
{
	phy_n_misc_info_t *info = (phy_n_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;

	pi->phy_tx_tone_freq = 0;
		/* use 151 since that should correspond to nominal tx output power */
	/* Can not play tone with papd bit enabled */
	if (NREV_IS(pi->pubpi->phy_rev, LCNXN_BASEREV))
		wlc_phy_papd_enable_nphy(pi, FALSE);
	if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
	wlc_phy_stay_in_carriersearch_nphy(pi, TRUE);
	wlc_phy_tx_tone_nphy(pi, 0, 151, 0, 0, TRUE); /* play tone */
	if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))
		wlapi_enable_mac(pi->sh->physhim);
}

static int phy_n_iovar_get_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 *ret_int_ptr,
	int32 int_val, int err)
{
	phy_n_misc_info_t *info = (phy_n_misc_info_t *)ctx;
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

static int phy_n_iovar_set_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 int_val, int err)
{
	phy_n_misc_info_t *info = (phy_n_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint8 samples, antenna, resolution, lpf_hpc, dig_lpf;
	uint8 gain_correct, extra_gain_3dB, force_gain_type;
	uint8 index;
	phy_info_nphy_t *pi_nphy;

	extra_gain_3dB = (int_val >> 28) & 0xf;
	gain_correct = (int_val >> 24) & 0xf;
	lpf_hpc = (int_val >> 20) & 0x3;
	dig_lpf = (int_val >> 22) & 0x3;
	resolution = (int_val >> 16) & 0xf;
	samples = (int_val >> 8) & 0xff;
	antenna = int_val & 0xf;
	force_gain_type = (int_val >> 4) & 0xf;

	pi_nphy = pi->u.pi_nphy;
	index = (((int_val >> 28) & 0xF) << 3) | ((int_val >> 21) & 0x7);
	pi_nphy->gainindex = index;

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
phy_n_misc_deaf_mode(phy_type_misc_ctx_t *ctx, bool user_flag)
{
	phy_n_misc_info_t *info = (phy_n_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	wlc_nphy_deaf_mode(pi, user_flag);
}
