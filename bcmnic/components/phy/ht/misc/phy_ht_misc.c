/*
 * HTPHY Miscellaneous modules implementation
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
 * $Id: phy_ht_misc.c 658200 2016-09-07 01:03:02Z $
 */
#include <typedefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_btcx.h>
#include <phy_misc.h>
#include <phy_type_misc.h>
#include <phy_ht_misc.h>
#include <phy_ht_rxiqcal.h>
#include <wlc_phy_int.h>
#include <wlc_phyreg_ht.h>
#include <phy_utils_reg.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_ht.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_ht_misc_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_misc_info_t *cmn_info;
};

/* Local functions */
#if defined(BCMDBG)
static int phy_ht_misc_test_freq_accuracy(phy_type_misc_ctx_t *ctx, int channel);
static void phy_ht_misc_test_stop(phy_type_misc_ctx_t *ctx);
#endif
static uint32 phy_ht_rx_iq_est(phy_type_misc_ctx_t *ctx, uint8 samples, uint8 antsel,
	uint8 resolution, uint8 lpf_hpc, uint8 dig_lpf, uint8 gain_correct,
                                uint8 extra_gain_3dB, uint8 wait_for_crs, uint8 force_gain_type);
static void wlc_phy_deaf_htphy(phy_type_misc_ctx_t *ctx, bool mode);
static void phy_ht_iovar_tx_tone(phy_type_misc_ctx_t *ctx, int32 int_val);
static void phy_ht_iovar_txlo_tone(phy_type_misc_ctx_t *ctx);
static int phy_ht_iovar_get_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 *ret_int_ptr,
	int32 int_val, int err);
static int phy_ht_iovar_set_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 int_val, int err);
static bool phy_ht_misc_get_rxgainerr(phy_type_misc_ctx_t *ctx, int16 *gainerr);
static void phy_ht_misc_set_ldpc_override(phy_type_misc_ctx_t *ctx, bool ldpc);
/* WAR */
static int phy_ht_misc_set_filt_war(phy_type_misc_ctx_t *ctx, bool war);
static bool phy_ht_misc_get_filt_war(phy_type_misc_ctx_t *ctx);

/* register phy type specific implementation */
phy_ht_misc_info_t *
BCMATTACHFN(phy_ht_misc_register_impl)(phy_info_t *pi, phy_ht_info_t *hti,
	phy_misc_info_t *cmn_info)
{
	phy_ht_misc_info_t *ht_info;
	phy_type_misc_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ht_info = phy_malloc(pi, sizeof(phy_ht_misc_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	ht_info->pi = pi;
	ht_info->hti = hti;
	ht_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = ht_info;
	fns.phy_type_misc_rx_iq_est = phy_ht_rx_iq_est;
	fns.phy_type_misc_set_deaf = wlc_phy_deaf_htphy;
	fns.phy_type_misc_clear_deaf = wlc_phy_deaf_htphy;
#if defined(BCMDBG)
	fns.phy_type_misc_test_freq_accuracy = phy_ht_misc_test_freq_accuracy;
	fns.phy_type_misc_test_stop = phy_ht_misc_test_stop;
#endif
	fns.phy_type_misc_iovar_tx_tone = phy_ht_iovar_tx_tone;
	fns.phy_type_misc_iovar_txlo_tone = phy_ht_iovar_txlo_tone;
	fns.phy_type_misc_iovar_get_rx_iq_est = phy_ht_iovar_get_rx_iq_est;
	fns.phy_type_misc_iovar_set_rx_iq_est = phy_ht_iovar_set_rx_iq_est;
	fns.phy_type_misc_get_rxgainerr = phy_ht_misc_get_rxgainerr;
	fns.set_ldpc_override = phy_ht_misc_set_ldpc_override;
	fns.set_filt_war = phy_ht_misc_set_filt_war;
	fns.get_filt_war = phy_ht_misc_get_filt_war;

	if (phy_misc_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_misc_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return ht_info;

	/* error handling */
fail:
	if (ht_info != NULL)
		phy_mfree(pi, ht_info, sizeof(phy_ht_misc_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_misc_unregister_impl)(phy_ht_misc_info_t *ht_info)
{
	phy_info_t *pi;
	phy_misc_info_t *cmn_info;

	ASSERT(ht_info);
	pi = ht_info->pi;
	cmn_info = ht_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_misc_unregister_impl(cmn_info);

	phy_mfree(pi, ht_info, sizeof(phy_ht_misc_info_t));
}


/* ********************************************** */
/* Function table registred function */
/* ********************************************** */

static uint8 phy_ht_calc_extra_init_gain(phy_info_t *pi, uint8 extra_gain_3dB,
                                         rxgain_t rxgain[])
{
	uint16 init_gain_code[4];
	uint8 core, MAX_DVGA, MAX_LPF, MAX_MIX;
	uint8 dvga, mix, lpf0, lpf1;
	uint8 dvga_inc, lpf0_inc, lpf1_inc;
	uint8 max_inc, gain_ticks = extra_gain_3dB;

	MAX_DVGA = 4; MAX_LPF = 10; MAX_MIX = 4;
	wlc_phy_table_read_htphy(pi, HTPHY_TBL_ID_RFSEQ, 3, 0x106, 16, &init_gain_code);

	/* Find if the requested gain increase is possible */
	FOREACH_CORE(pi, core) {
		dvga = 0;
		mix = (init_gain_code[core] >> 4) & 0xf;
		lpf0 = (init_gain_code[core] >> 8) & 0xf;
		lpf1 = (init_gain_code[core] >> 12) & 0xf;
		max_inc = MAX(0, MAX_DVGA - dvga) + MAX(0, MAX_LPF - lpf0 - lpf1) +
		        MAX(0, MAX_MIX - mix);
		gain_ticks = MIN(gain_ticks, max_inc);
	}
	if (gain_ticks != extra_gain_3dB) {
		PHY_INFORM(("%s: Unable to find enough extra gain. Using extra_gain = %d\n",
		            __FUNCTION__, 3 * gain_ticks));
	}

	/* Do nothing if no gain increase is required/possible */
	if (gain_ticks == 0) {
		return gain_ticks;
	}

	/* Find the mix, lpf0, lpf1 gains required for extra INITgain */
	FOREACH_CORE(pi, core) {
		uint8 gain_inc = gain_ticks;

		dvga = 0;
		mix = (init_gain_code[core] >> 4) & 0xf;
		lpf0 = (init_gain_code[core] >> 8) & 0xf;
		lpf1 = (init_gain_code[core] >> 12) & 0xf;

		dvga_inc = MIN((uint8) MAX(0, MAX_DVGA - dvga), gain_inc);
		dvga += dvga_inc;
		gain_inc -= dvga_inc;

		lpf1_inc = MIN((uint8) MAX(0, MAX_LPF - lpf1 - lpf0), gain_inc);
		lpf1 += lpf1_inc;
		gain_inc -= lpf1_inc;

		lpf0_inc = MIN((uint8) MAX(0, MAX_LPF - lpf1 - lpf0), gain_inc);
		lpf0 += lpf0_inc;
		gain_inc -= lpf0_inc;

		mix += MIN((uint8) MAX(0, MAX_MIX - mix), gain_inc);

		rxgain[core].lna1 = init_gain_code[core] & 0x3;
		rxgain[core].lna2 = (init_gain_code[core] >> 2) & 0x3;
		rxgain[core].mix  = mix;
		rxgain[core].lpf0 = lpf0;
		rxgain[core].lpf1 = lpf1;
		rxgain[core].dvga = dvga;
	}

	return gain_ticks;
}

#if defined(BCMDBG)
static int
phy_ht_misc_test_freq_accuracy(phy_type_misc_ctx_t *ctx, int channel)
{
	phy_ht_misc_info_t *info = (phy_ht_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	return wlc_phy_freq_accuracy_htphy(pi, channel);
}

static void
phy_ht_misc_test_stop(phy_type_misc_ctx_t *ctx)
{
	phy_ht_misc_info_t *info = (phy_ht_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
	PHY_REG_LIST_START
		PHY_REG_AND_RAW_ENTRY(HTPHY_TO_BPHY_OFF + BPHY_TEST, 0xfc00)
		PHY_REG_WRITE_RAW_ENTRY(HTPHY_bphytestcontrol, 0x0)
	PHY_REG_LIST_EXECUTE(pi);
	}
}
#endif 

static uint32 phy_ht_rx_iq_est(phy_type_misc_ctx_t *ctx, uint8 samples, uint8 antsel,
	uint8 resolution, uint8 lpf_hpc, uint8 dig_lpf, uint8 gain_correct,
                                uint8 extra_gain_3dB, uint8 wait_for_crs, uint8 force_gain_type) {
	phy_ht_misc_info_t *info = (phy_ht_misc_info_t *)ctx;
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
	rxgain_t rxgain[PHY_CORE_MAX];
	rxgain_ovrd_t rxgain_ovrd[PHY_CORE_MAX];

	if (sampling_in_progress) {
		PHY_ERROR(("%s: sampling_in_progress\n", __FUNCTION__));

		return 0;
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


	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
	if (lpf_hpc) {
		/* Override the LPF high pass corners to their lowest values (0x1) */
		wlc_phy_lpf_hpc_override_htphy(pi, TRUE);
	}

	/* Overide the digital LPF */
	if (dig_lpf) {
		wlc_phy_dig_lpf_override_htphy(pi, dig_lpf);
	}

	/* Increase INITgain if requested */
	if (extra_gain_3dB > 0) {
		extra_gain_3dB = phy_ht_calc_extra_init_gain(pi, extra_gain_3dB, rxgain);

		/* Override higher INITgain if possible */
		if (extra_gain_3dB > 0) {
			wlc_phy_rfctrl_override_rxgain_htphy(pi, 0, rxgain, rxgain_ovrd);
		}
	}

	/* get IQ power measurements */
	wlc_phy_rx_iq_est_htphy(pi, est, num_samps, wait_time, wait_for_crs);

	/* Disable the overrides if they were set */
	if (extra_gain_3dB > 0) {
		wlc_phy_rfctrl_override_rxgain_htphy(pi, 1, NULL, rxgain_ovrd);
	}

	if (lpf_hpc) {
		/* Restore LPF high pass corners to their original values */
		wlc_phy_lpf_hpc_override_htphy(pi, FALSE);
	}

	/* Restore the digital LPF */
	if (dig_lpf) {
		wlc_phy_dig_lpf_override_htphy(pi, 0);
	}
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);


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

		if (PHYCORENUM(pi->pubpi->phy_corenum) == 3)  {

			int16 noisefloor;

			wlc_phy_noise_calc_fine_resln(pi, cmplx_pwr, crsmin_pwr, noise_dBm_ant_fine,
			                              extra_gain_1dB, tot_gain);

			if ((gain_correct == 1) || (gain_correct == 2) || gain_correct == 3) {
				int16 gainerr[PHY_CORE_MAX];
				int16 gain_err_temp_adj;
				wlc_phy_get_rxgainerr_phy(pi, gainerr);

				/* make and apply temperature correction */

				/* Read and (implicitly) store current temperature */
				wlc_phy_tempsense_htphy(pi);

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

static void phy_ht_iovar_tx_tone(phy_type_misc_ctx_t *ctx, int32 int_val)
{

	phy_ht_misc_info_t *info = (phy_ht_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;

	pi->phy_tx_tone_freq = (int32) int_val;

	if (pi->phy_tx_tone_freq == 0) {
		wlc_phy_stopplayback_htphy(pi);
		phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	} else {
		phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
		wlc_phy_tx_tone_htphy(pi, (uint32)int_val, 151, 0, 0, TRUE); /* play tone */
	}
}

static void phy_ht_iovar_txlo_tone(phy_type_misc_ctx_t *ctx)
{
	phy_ht_misc_info_t *info = (phy_ht_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	pi->phy_tx_tone_freq = 0;
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
	wlc_phy_tx_tone_htphy(pi, 0, 151, 0, 0, TRUE); /* play tone */
}

static int phy_ht_iovar_get_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 *ret_int_ptr,
	int32 int_val, int err)
{
	phy_ht_misc_info_t *info = (phy_ht_misc_info_t *)ctx;
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
static int phy_ht_iovar_set_rx_iq_est(phy_type_misc_ctx_t *ctx, int32 int_val, int err)
{
	phy_ht_misc_info_t *info = (phy_ht_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint8 samples, antenna, resolution, lpf_hpc, dig_lpf;
	uint8 gain_correct, extra_gain_3dB, force_gain_type;

	extra_gain_3dB = (int_val >> 28) & 0xf;
	gain_correct = (int_val >> 24) & 0xf;
	lpf_hpc = (int_val >> 20) & 0x3;
	dig_lpf = (int_val >> 22) & 0x3;
	resolution = (int_val >> 16) & 0xf;
	samples = (int_val >> 8) & 0xff;
	antenna = int_val & 0xf;
	force_gain_type = (int_val >> 4) & 0xf;

	if (gain_correct > 4) {
		err = BCME_RANGE;
		return err;
	}

	if ((lpf_hpc != 0) && (lpf_hpc != 1)) {
		err = BCME_RANGE;
		return err;
	}
	if (dig_lpf > 2) {
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
wlc_phy_deaf_htphy(phy_type_misc_ctx_t *ctx, bool mode)
{
	phy_ht_misc_info_t *info = (phy_ht_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	phy_info_htphy_t *pi_ht = (phy_info_htphy_t *)pi->u.pi_htphy;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	if (mode) {
		if (pi_ht->deaf_count == 0)
			phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
		else
			PHY_ERROR(("%s: Deafness already set\n", __FUNCTION__));
	}
	else {
		if (pi_ht->deaf_count > 0)
			phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
		else
			PHY_ERROR(("%s: Deafness already cleared\n", __FUNCTION__));
	}
	wlapi_enable_mac(pi->sh->physhim);
}

static bool
phy_ht_misc_get_rxgainerr(phy_type_misc_ctx_t *ctx, int16 *gainerr)
{
	phy_ht_misc_info_t *info = (phy_ht_misc_info_t *)ctx;
	phy_info_t *pi = info->pi;
	bool srom_isempty[PHY_CORE_MAX] = { 0 };
	uint8 core;
	uint16 channel;
	chanspec_t chanspec = pi->radio_chanspec;
	channel = CHSPEC_CHANNEL(chanspec);
	FOREACH_CORE(pi, core) {
		if (channel <= 14) {
			/* 2G */
			gainerr[core] = (int16) pi->rxgainerr_2g[core];
			srom_isempty[core] = pi->rxgainerr2g_isempty;
#ifdef BAND5G
		} else if (channel <= 48) {
				/* 5G-low: channels 36 through 48 */
				gainerr[core] = (int16) pi->rxgainerr_5gl[core];
				srom_isempty[core] = pi->rxgainerr5gl_isempty;
		} else if (channel <= 64) {
			/* 5G-mid: channels 52 through 64 */
			gainerr[core] = (int16) pi->rxgainerr_5gm[core];
			srom_isempty[core] = pi->rxgainerr5gm_isempty;
		} else if (channel <= 128) {
			/* 5G-high: channels 100 through 128 */
			gainerr[core] = (int16) pi->rxgainerr_5gh[core];
			srom_isempty[core] = pi->rxgainerr5gh_isempty;
		} else {
			/* 5G-upper: channels 132 and above */
			gainerr[core] = (int16) pi->rxgainerr_5gu[core];
			srom_isempty[core] = pi->rxgainerr5gu_isempty;
#endif /* BAND5G */
		}
	}
	/* For 80P80, retrun only primary channel value */
	return srom_isempty[0];
}

static void
phy_ht_misc_set_ldpc_override(phy_type_misc_ctx_t *ctx, bool ldpc)
{
	phy_ht_misc_info_t *misc_info = (phy_ht_misc_info_t *) ctx;
	phy_info_t *pi = misc_info->pi;
	wlc_phy_update_rxldpc_htphy(pi, ldpc);
	return;
}

/* WAR */
static int
phy_ht_misc_set_filt_war(phy_type_misc_ctx_t *ctx, bool war)
{
	phy_ht_misc_info_t *misc_info = (phy_ht_misc_info_t *) ctx;
	phy_info_t *pi = misc_info->pi;
	phy_info_htphy_t *pi_ht = misc_info->hti;
	if (pi_ht->ht_ofdm20_filt_war_req != war) {
		pi_ht->ht_ofdm20_filt_war_req = war;
		if (pi->sh->up)
			wlc_phy_tx_digi_filts_htphy_war(pi, TRUE);
	}
	return BCME_OK;
}

static bool
phy_ht_misc_get_filt_war(phy_type_misc_ctx_t *ctx)
{
	phy_ht_misc_info_t *misc_info = (phy_ht_misc_info_t *) ctx;
	phy_info_htphy_t *pi_ht = misc_info->hti;
	return pi_ht->ht_ofdm20_filt_war_req;
}
