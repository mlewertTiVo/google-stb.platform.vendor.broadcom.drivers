/*
 * HTPHY ACI module implementation
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
 * $Id: phy_ht_noise.c 659938 2016-09-16 16:47:54Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_wd.h>
#include "phy_type_noise.h"
#include <phy_ht.h>
#include <phy_ht_noise.h>
#include <wlc_phyreg_ht.h>
#include <phy_ocl_api.h>
#include <phy_noise.h>
#include <wlc_phy_int.h>
#include <phy_noise_api.h>

/* module private states */
struct phy_ht_noise_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_noise_info_t *ii;
};

/* local functions */
static int phy_ht_noise_attach_modes(phy_info_t *pi);
static void phy_ht_noise_calc_fine_resln(phy_type_noise_ctx_t *ctx, int16 cmplx_pwr_dbm[],
	uint8 extra_gain_1dB, int16 *tot_gain);
static void phy_ht_noise_set_mode(phy_type_noise_ctx_t *ctx, int wanted_mode, bool init);
static int phy_ht_noise_reset(phy_type_noise_ctx_t *ctx);
static bool phy_ht_noise_noise_wd(phy_wd_ctx_t *ctx);
static bool phy_ht_noise_aci_wd(phy_wd_ctx_t *ctx);
#ifndef WLC_DISABLE_ACI
static void phy_ht_noise_interf_rssi_update(phy_type_noise_ctx_t *ctx,
	chanspec_t chanspec, int8 leastRSSI);
static void phy_ht_noise_upd_aci(phy_type_noise_ctx_t * ctx);
static void phy_ht_noise_update_aci_ma(phy_type_noise_ctx_t *pi);
#endif
static void phy_ht_noise_interf_mode_set(phy_type_noise_ctx_t *ctx, int val);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int phy_ht_noise_dump(phy_type_noise_ctx_t *ctx, struct bcmstrbuf *b);
#else
#define phy_ht_noise_dump NULL
#endif
static void phy_ht_noise_request_sample(phy_type_noise_ctx_t *ctx, uint8 reason, uint8 ch);
static int8 phy_ht_noise_avg(phy_type_noise_ctx_t *ctx);
static void phy_ht_noise_sample_intr(phy_type_noise_ctx_t *ctx);
static int8 phy_ht_noise_lte_avg(phy_type_noise_ctx_t *ctx);
static int phy_ht_noise_get_srom_level(phy_type_noise_ctx_t *ctx, int32 *ret_int_ptr);
static int8 phy_ht_noise_read_shmem(phy_type_noise_ctx_t *ctx,
	uint8 *lte_on, uint8 *crs_high);

/* register phy type specific implementation */
phy_ht_noise_info_t *
BCMATTACHFN(phy_ht_noise_register_impl)(phy_info_t *pi, phy_ht_info_t *hti, phy_noise_info_t *ii)
{
	phy_ht_noise_info_t *info;
	phy_type_noise_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((info = phy_malloc(pi, sizeof(phy_ht_noise_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_ht_noise_info_t));
	info->pi = pi;
	info->hti = hti;
	info->ii = ii;

	if (phy_ht_noise_attach_modes(pi) != BCME_OK) {
		goto fail;
	}

	/* register watchdog fn */
	if (phy_wd_add_fn(pi->wdi, phy_ht_noise_noise_wd, info,
	                  PHY_WD_PRD_1TICK, PHY_WD_1TICK_INTF_NOISE,
	                  PHY_WD_FLAG_NONE) != BCME_OK) {
		PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
		goto fail;
	}
	if (phy_wd_add_fn(pi->wdi, phy_ht_noise_aci_wd, info,
	                  PHY_WD_PRD_1TICK, PHY_WD_1TICK_NOISE_ACI,
	                  PHY_WD_FLAG_NONE) != BCME_OK) {
		PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
		goto fail;
	}

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.calc_fine = phy_ht_noise_calc_fine_resln;
	fns.mode = phy_ht_noise_set_mode;
	fns.reset = phy_ht_noise_reset;
#ifndef WLC_DISABLE_ACI
	fns.interf_rssi_update = phy_ht_noise_interf_rssi_update;
	fns.aci_upd = phy_ht_noise_upd_aci;
	fns.ma_upd = phy_ht_noise_update_aci_ma;
#endif
	fns.interf_mode_set = phy_ht_noise_interf_mode_set;
	fns.dump = phy_ht_noise_dump;
	fns.request_noise_sample = phy_ht_noise_request_sample;
	fns.avg = phy_ht_noise_avg;
	fns.sample_intr = phy_ht_noise_sample_intr;
	fns.lte_avg = phy_ht_noise_lte_avg;
	fns.get_srom_level = phy_ht_noise_get_srom_level;
	fns.read_shm = phy_ht_noise_read_shmem;
	fns.ctx = info;

	phy_noise_register_impl(ii, &fns);

	return info;

	/* error handling */
fail:
	phy_ht_noise_unregister_impl(info);

	return NULL;
}

static int
BCMATTACHFN(phy_ht_noise_attach_modes)(phy_info_t *pi)
{
	pi->interf->aci.nphy = (nphy_aci_interference_info_t *)
		phy_malloc(pi, sizeof(*(pi->interf->aci.nphy)));

	if (pi->interf->aci.nphy == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));

		return BCME_NOMEM;
	}

	if (IS_X12_BOARDTYPE(pi)) {
		pi->sh->interference_mode_2G = INTERFERE_NONE;
		pi->sh->interference_mode_5G = INTERFERE_NONE;
	} else if (IS_X29B_BOARDTYPE(pi)) {
		pi->sh->interference_mode_2G = NON_WLAN;
		pi->sh->interference_mode_5G = NON_WLAN;
	} else {
		if (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardflags) & BFL_EXTLNA) {
			pi->sh->interference_mode_2G = NON_WLAN;
		} else {
			pi->sh->interference_mode_2G = WLAN_AUTO_W_NOISE;
		}
	}

	return BCME_OK;
}

void
BCMATTACHFN(phy_ht_noise_unregister_impl)(phy_ht_noise_info_t *info)
{

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (info != NULL) {
		phy_info_t *pi = info->pi;
		phy_noise_info_t *ii = info->ii;

		/* unregister from common */
		phy_noise_unregister_impl(ii);

		if (pi->interf->aci.nphy != NULL)
			phy_mfree(pi, pi->interf->aci.nphy, sizeof(*(pi->interf->aci.nphy)));

		phy_mfree(pi, info, sizeof(phy_ht_noise_info_t));
	}
}

static void
phy_ht_noise_calc_fine_resln(phy_type_noise_ctx_t *ctx, int16 cmplx_pwr_dbm[],
	uint8 extra_gain_1dB, int16 *tot_gain)
{
	phy_ht_noise_info_t *info = (phy_ht_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint8 i;
	int16 assumed_gain;

	wlc_phy_get_rxiqest_gain_htphy(pi, &assumed_gain);
	assumed_gain += extra_gain_1dB;
	FOREACH_CORE(pi, i) {
		cmplx_pwr_dbm[i] += (int16) ((HTPHY_NOISE_SAMPLEPWR_TO_DBM -
				assumed_gain) << 2);
	}
}

/* set mode */
static void
phy_ht_noise_set_mode(phy_type_noise_ctx_t *ctx, int wanted_mode, bool init)
{
	phy_ht_noise_info_t *info = (phy_ht_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;

	PHY_TRACE(("%s: mode %d init %d\n", __FUNCTION__, wanted_mode, init));

	/* initialize interference algorithms */
	if (pi->sh->interference_mode_override == TRUE) {
		/* keep the same values */
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			pi->sh->interference_mode = pi->sh->interference_mode_2G_override;
		} else {
			/* for 5G, only values 0 and 1 are valid options */
			if (pi->sh->interference_mode_5G_override == 0 ||
			    pi->sh->interference_mode_5G_override == 1) {
				pi->sh->interference_mode = pi->sh->interference_mode_5G_override;
			} else {
				/* used invalid value. so default to 0 */
				pi->sh->interference_mode = 0;
			}
		}
	} else {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			pi->sh->interference_mode = pi->sh->interference_mode_2G;
		} else {
			pi->sh->interference_mode = pi->sh->interference_mode_5G;
		}
	}

	if (init) {
		pi->interference_mode_crs_time = 0;
		pi->crsglitch_prev = 0;

		/* clear out all the state */
		wlc_phy_noisemode_reset_htphy(pi);
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			wlc_phy_acimode_reset_htphy(pi);
		}
	}

	/* NPHY 5G, supported for NON_WLAN and INTERFERE_NONE only */
	if (CHSPEC_IS2G(pi->radio_chanspec) ||
	    (CHSPEC_IS5G(pi->radio_chanspec) &&
	     (wanted_mode == NON_WLAN || wanted_mode == INTERFERE_NONE))) {
		if (wanted_mode == INTERFERE_NONE) {	/* disable */
			wlc_phy_noise_raise_MFthresh_htphy(pi, FALSE);

			switch (pi->cur_interference_mode) {
			case WLAN_AUTO:
			case WLAN_AUTO_W_NOISE:
			case WLAN_MANUAL:
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					wlc_phy_acimode_set_htphy(pi, FALSE,
						PHY_ACI_PWR_NOTPRESENT);
				}
				pi->aci_state &= ~ACI_ACTIVE;
				break;
			case NON_WLAN:
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					wlc_phy_acimode_set_htphy(pi,
						FALSE,
						PHY_ACI_PWR_NOTPRESENT);
					pi->aci_state &= ~ACI_ACTIVE;
				} else {
					pi->interference_mode_crs = 0;

				}
				break;
			}
		} else {	/* Enable */
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				wlc_phy_noise_raise_MFthresh_htphy(pi, TRUE);
			}

			switch (wanted_mode) {
			case NON_WLAN:
				pi->interference_mode_crs = 1;
				break;
			case WLAN_AUTO:
			case WLAN_AUTO_W_NOISE:
				/* fall through */
				break;
			case WLAN_MANUAL:
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					wlc_phy_acimode_set_htphy(pi, TRUE,
						PHY_ACI_PWR_HIGH);
				}
				break;
			}
		}
	}
}

static int
phy_ht_noise_reset(phy_type_noise_ctx_t *ctx)
{
	phy_ht_noise_info_t *info = (phy_ht_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

#ifndef WLC_DISABLE_ACI
	/* update interference mode before INIT process start */
	if (!(SCAN_INPROG_PHY(pi))) {
		pi->sh->interference_mode = CHSPEC_IS2G(pi->radio_chanspec) ?
			pi->sh->interference_mode_2G : pi->sh->interference_mode_5G;
	}
#endif

	ASSERT(pi->interf->aci.nphy != NULL);

	/* Reset ACI internals if not scanning and not in aci_detection */
	if (!(SCAN_INPROG_PHY(pi) ||
	      pi->interf->aci.nphy->detection_in_progress)) {
		wlc_phy_aci_sw_reset_htphy(pi);
	}

	return BCME_OK;
}

/* watchdog callback */
static bool
phy_ht_noise_noise_wd(phy_wd_ctx_t *ctx)
{
	phy_ht_noise_info_t *ii = (phy_ht_noise_info_t *)ctx;
	phy_info_t *pi = ii->pi;

	if (pi->tunings[0]) {
		pi->interf->noise.nphy_noise_assoc_enter_th = pi->tunings[0];
		pi->interf->noise.nphy_noise_noassoc_enter_th = pi->tunings[0];
	}

	if (pi->tunings[2]) {
		pi->interf->noise.nphy_noise_assoc_glitch_th_dn = pi->tunings[2];
		pi->interf->noise.nphy_noise_noassoc_glitch_th_dn = pi->tunings[2];
	}

	if (pi->tunings[1]) {
		pi->interf->noise.nphy_noise_noassoc_glitch_th_up = pi->tunings[1];
		pi->interf->noise.nphy_noise_assoc_glitch_th_up = pi->tunings[1];
	}

	return TRUE;
}

static bool
phy_ht_noise_aci_wd(phy_wd_ctx_t *ctx)
{
	phy_ht_noise_info_t *ii = (phy_ht_noise_info_t *)ctx;
	phy_info_t *pi = ii->pi;
	phy_noise_info_t *noisei = ii->ii;

	/* defer interference checking, scan and update if RM is progress */
	if (!SCAN_RM_IN_PROGRESS(pi)) {
		/* interf.scanroamtimer counts transient time coming out of scan */
		if (pi->interf->scanroamtimer != 0)
			pi->interf->scanroamtimer -= 1;

		wlc_phy_aci_upd(noisei);

	} else {
		/* in a scan/radio meas, don't update moving average when we
		 * first come out of scan or roam
		 */
		pi->interf->scanroamtimer = 2;
	}

	return TRUE;
}

#ifndef WLC_DISABLE_ACI
static void
phy_ht_noise_interf_rssi_update(phy_type_noise_ctx_t *ctx, chanspec_t chanspec, int8 leastRSSI)
{
	phy_ht_noise_info_t *info = (phy_ht_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;
	if ((CHSPEC_CHANNEL(chanspec) == pi->interf->curr_home_channel)) {
		pi->interf->rssi = leastRSSI;
	}
}

static void
phy_ht_noise_upd_aci(phy_type_noise_ctx_t * ctx)
{
	phy_ht_noise_info_t *noise_info = (phy_ht_noise_info_t *) ctx;
	phy_info_t *pi = noise_info->pi;

	switch (pi->sh->interference_mode) {
		case NON_WLAN:
			/* run noise mitigation only */
			wlc_phy_noisemode_upd_htphy(pi);
			break;
		case WLAN_AUTO:
			if (ASSOC_INPROG_PHY(pi)) {
				break;
			}

#ifdef NOISE_CAL_LCNXNPHY
			if ((NREV_IS(pi->pubpi->phy_rev, LCNXN_BASEREV) ||
				NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))) {
					wlc_phy_aci_noise_measure_nphy(pi, TRUE);
			}
			else
#endif
			{
				/* 5G band not supported yet */
				if (CHSPEC_IS5G(pi->radio_chanspec))
					break;

				if (PUB_NOT_ASSOC(pi)) {
					/* not associated:  do not run aci routines */
					break;
				}
				PHY_ACI(("Interf Mode 3,"
					" pi->interf->aci.glitch_ma = %d\n",
					pi->interf->aci.glitch_ma));

				/* Attempt to enter ACI mode if not already active */
				/* only run this code if associated */
				if (!(pi->aci_state & ACI_ACTIVE)) {
					if ((pi->sh->now  % NPHY_ACI_CHECK_PERIOD) == 0) {

						if ((pi->interf->aci.glitch_ma +
							pi->interf->badplcp_ma) >=
							pi->interf->aci.enter_thresh) {
							wlc_phy_acimode_upd_htphy(pi);
						}
					}
				} else {
					if (((pi->sh->now - pi->aci_start_time) %
						pi->aci_exit_check_period) == 0) {
						wlc_phy_acimode_upd_htphy(pi);
					}
				}
			}
			break;
		case WLAN_AUTO_W_NOISE:
#ifdef NOISE_CAL_LCNXNPHY
			if ((NREV_IS(pi->pubpi->phy_rev, LCNXN_BASEREV) ||
				NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3))) {
					wlc_phy_aci_noise_measure_nphy(pi, TRUE);
			}
			else
#endif
			{
				/* 5G band not supported yet */
				if (CHSPEC_IS5G(pi->radio_chanspec)) {
					break;
				}
				wlc_phy_aci_noise_upd_htphy(pi);
			}
			break;

		default:
			break;
	}
}

static void
phy_ht_noise_update_aci_ma(phy_type_noise_ctx_t *ctx)
{
	phy_ht_noise_info_t *noise_info = (phy_ht_noise_info_t *) ctx;
	phy_info_t *pi = noise_info->pi;

	int32 delta = 0;
	int32 bphy_delta = 0;
	int32 ofdm_delta = 0;
	int32 badplcp_delta = 0;
	int32 bphy_badplcp_delta = 0;
	int32 ofdm_badplcp_delta = 0;

	if ((pi->interf->cca_stats_func_called == FALSE) || pi->interf->cca_stats_mbsstime <= 0) {
		uint16 cur_glitch_cnt;
		uint16 bphy_cur_glitch_cnt = 0;
		uint16 cur_badplcp_cnt = 0;
		uint16 bphy_cur_badplcp_cnt = 0;

#ifdef WLSRVSDB
		uint8 bank_offset = 0;
		uint8 vsdb_switch_failed = 0;
		uint8 vsdb_split_cntr = 0;

		if (CHSPEC_CHANNEL(pi->radio_chanspec) == pi->srvsdb_state->sr_vsdb_channels[0]) {
			bank_offset = 0;
		} else if (CHSPEC_CHANNEL(pi->radio_chanspec) ==
			pi->srvsdb_state->sr_vsdb_channels[1]) {
			bank_offset = 1;
		}
		/* Assume vsdb switch failed, if no switches were recorded for both the channels */
		vsdb_switch_failed = !(pi->srvsdb_state->num_chan_switch[0] &
		pi->srvsdb_state->num_chan_switch[1]);

		/* use split counter for each channel
		* if vsdb is active and vsdb switch was successfull
		*/
		/* else use last 1 sec delta counter
		* for current channel for calculations
		*/
		vsdb_split_cntr = (!vsdb_switch_failed) && (pi->srvsdb_state->srvsdb_active);

#endif /* WLSRVSDB */
		/* determine delta number of rxcrs glitches */
		cur_glitch_cnt =
			wlapi_bmac_read_shm(pi->sh->physhim, MACSTAT_ADDR(pi, MCSTOFF_RXCRSGLITCH));
		delta = cur_glitch_cnt - pi->interf->aci.pre_glitch_cnt;
		pi->interf->aci.pre_glitch_cnt = cur_glitch_cnt;

#ifdef WLSRVSDB
		if (vsdb_split_cntr) {
			delta =  pi->srvsdb_state->sum_delta_crsglitch[bank_offset];
		}
#endif /* WLSRVSDB */

		/* compute the rxbadplcp  */
		cur_badplcp_cnt = wlapi_bmac_read_shm(pi->sh->physhim,
			MACSTAT_ADDR(pi, MCSTOFF_RXBADPLCP));
		badplcp_delta = cur_badplcp_cnt - pi->interf->pre_badplcp_cnt;
		pi->interf->pre_badplcp_cnt = cur_badplcp_cnt;

#ifdef WLSRVSDB
		if (vsdb_split_cntr) {
			badplcp_delta = pi->srvsdb_state->sum_delta_prev_badplcp[bank_offset];
		}
#endif /* WLSRVSDB */
		/* determine delta number of bphy rx crs glitches */
		bphy_cur_glitch_cnt = wlapi_bmac_read_shm(pi->sh->physhim,
			MACSTAT_ADDR(pi, MCSTOFF_BPHYGLITCH));
		bphy_delta = bphy_cur_glitch_cnt - pi->interf->noise.bphy_pre_glitch_cnt;
		pi->interf->noise.bphy_pre_glitch_cnt = bphy_cur_glitch_cnt;

#ifdef WLSRVSDB
		if (vsdb_split_cntr) {
			bphy_delta = pi->srvsdb_state->sum_delta_bphy_crsglitch[bank_offset];
		}
#endif /* WLSRVSDB */
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			/* ofdm glitches is what we will be using */
			ofdm_delta = delta - bphy_delta;
		} else {
			ofdm_delta = delta;
		}

		/* compute bphy rxbadplcp */
		bphy_cur_badplcp_cnt = wlapi_bmac_read_shm(pi->sh->physhim,
			MACSTAT_ADDR(pi, MCSTOFF_BPHY_BADPLCP));
		bphy_badplcp_delta = bphy_cur_badplcp_cnt -
			pi->interf->bphy_pre_badplcp_cnt;
		pi->interf->bphy_pre_badplcp_cnt = bphy_cur_badplcp_cnt;

#ifdef WLSRVSDB
		if (vsdb_split_cntr) {
			bphy_badplcp_delta =
				pi->srvsdb_state->sum_delta_prev_bphy_badplcp[bank_offset];
		}
#endif /* WLSRVSDB */

		/* ofdm bad plcps is what we will be using */
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			ofdm_badplcp_delta = badplcp_delta - bphy_badplcp_delta;
		} else {
			ofdm_badplcp_delta = badplcp_delta;
		}

		/* if we aren't suppose to update yet, don't */
		if (pi->interf->scanroamtimer != 0) {
			return;
		}

	} else {
		pi->interf->cca_stats_func_called = FALSE;
		/* Normalizing the statistics per second */
		delta = pi->interf->cca_stats_total_glitch * 1000 /
		        pi->interf->cca_stats_mbsstime;
		bphy_delta = pi->interf->cca_stats_bphy_glitch * 1000 /
		        pi->interf->cca_stats_mbsstime;
		ofdm_delta = delta - bphy_delta;
		badplcp_delta = pi->interf->cca_stats_total_badplcp * 1000 /
		        pi->interf->cca_stats_mbsstime;
		bphy_badplcp_delta = pi->interf->cca_stats_bphy_badplcp * 1000 /
		        pi->interf->cca_stats_mbsstime;
		ofdm_badplcp_delta = badplcp_delta - bphy_badplcp_delta;
	}

	if (delta >= 0) {
		/* evict old value */
		pi->interf->aci.ma_total -= pi->interf->aci.ma_list[pi->interf->aci.ma_index];

		/* admit new value */
		pi->interf->aci.ma_total += (uint16) delta;
		pi->interf->aci.glitch_ma_previous = pi->interf->aci.glitch_ma;
		pi->interf->aci.glitch_ma = pi->interf->aci.ma_total /
			PHY_NOISE_MA_WINDOW_SZ;
		pi->interf->aci.ma_list[pi->interf->aci.ma_index++] = (uint16) delta;
		if (pi->interf->aci.ma_index >= PHY_NOISE_MA_WINDOW_SZ) {
			pi->interf->aci.ma_index = 0;
		}
	}

	if (badplcp_delta >= 0) {
		pi->interf->badplcp_ma_total -=
			pi->interf->badplcp_ma_list[pi->interf->badplcp_ma_index];
		pi->interf->badplcp_ma_total += (uint16) badplcp_delta;
		pi->interf->badplcp_ma_previous = pi->interf->badplcp_ma;
		pi->interf->badplcp_ma =
			pi->interf->badplcp_ma_total / PHY_NOISE_MA_WINDOW_SZ;
		pi->interf->badplcp_ma_list[pi->interf->badplcp_ma_index++] =
			(uint16) badplcp_delta;
		if (pi->interf->badplcp_ma_index >= PHY_NOISE_MA_WINDOW_SZ) {
			pi->interf->badplcp_ma_index = 0;
		}
	}

	if ((CHSPEC_IS5G(pi->radio_chanspec) && (ofdm_delta >= 0)) ||
		(CHSPEC_IS2G(pi->radio_chanspec) && (delta >= 0) && (bphy_delta >= 0))) {
		pi->interf->noise.ofdm_ma_total -= pi->interf->noise.
				ofdm_glitch_ma_list[pi->interf->noise.ofdm_ma_index];
		pi->interf->noise.ofdm_ma_total += (uint16) ofdm_delta;
		pi->interf->noise.ofdm_glitch_ma_previous =
			pi->interf->noise.ofdm_glitch_ma;
		pi->interf->noise.ofdm_glitch_ma =
			pi->interf->noise.ofdm_ma_total / PHY_NOISE_MA_WINDOW_SZ;
		pi->interf->noise.ofdm_glitch_ma_list[pi->interf->noise.ofdm_ma_index++] =
			(uint16) ofdm_delta;
		if (pi->interf->noise.ofdm_ma_index >= PHY_NOISE_MA_WINDOW_SZ)
			pi->interf->noise.ofdm_ma_index = 0;
	}

	if (bphy_delta >= 0) {
		pi->interf->noise.bphy_ma_total -= pi->interf->noise.
			bphy_glitch_ma_list[pi->interf->noise.bphy_ma_index];
		pi->interf->noise.bphy_ma_total += (uint16) bphy_delta;
		pi->interf->noise.bphy_glitch_ma_previous =
			pi->interf->noise.bphy_glitch_ma;
		pi->interf->noise.bphy_glitch_ma =
			pi->interf->noise.bphy_ma_total / PHY_NOISE_MA_WINDOW_SZ;
		pi->interf->noise.bphy_glitch_ma_list[pi->interf->noise.bphy_ma_index++] =
			(uint16) bphy_delta;
		if (pi->interf->noise.bphy_ma_index >= PHY_NOISE_MA_WINDOW_SZ)
			pi->interf->noise.bphy_ma_index = 0;
	}

	if ((CHSPEC_IS5G(pi->radio_chanspec) && (ofdm_badplcp_delta >= 0)) ||
		(CHSPEC_IS2G(pi->radio_chanspec) && (badplcp_delta >= 0) &&
		(bphy_badplcp_delta >= 0))) {
		pi->interf->noise.ofdm_badplcp_ma_total -= pi->interf->noise.
			ofdm_badplcp_ma_list[pi->interf->noise.ofdm_badplcp_ma_index];
		pi->interf->noise.ofdm_badplcp_ma_total += (uint16) ofdm_badplcp_delta;
		pi->interf->noise.ofdm_badplcp_ma_previous =
			pi->interf->noise.ofdm_badplcp_ma;
		pi->interf->noise.ofdm_badplcp_ma =
			pi->interf->noise.ofdm_badplcp_ma_total / PHY_NOISE_MA_WINDOW_SZ;
		pi->interf->noise.ofdm_badplcp_ma_list
			[pi->interf->noise.ofdm_badplcp_ma_index++] =
			(uint16) ofdm_badplcp_delta;
		if (pi->interf->noise.ofdm_badplcp_ma_index >= PHY_NOISE_MA_WINDOW_SZ)
			pi->interf->noise.ofdm_badplcp_ma_index = 0;
	}

	if (bphy_badplcp_delta >= 0) {
		pi->interf->noise.bphy_badplcp_ma_total -= pi->interf->noise.
			bphy_badplcp_ma_list[pi->interf->noise.bphy_badplcp_ma_index];
		pi->interf->noise.bphy_badplcp_ma_total += (uint16) bphy_badplcp_delta;
		pi->interf->noise.bphy_badplcp_ma_previous = pi->interf->noise.
			bphy_badplcp_ma;
		pi->interf->noise.bphy_badplcp_ma =
			pi->interf->noise.bphy_badplcp_ma_total / PHY_NOISE_MA_WINDOW_SZ;
		pi->interf->noise.bphy_badplcp_ma_list
				[pi->interf->noise.bphy_badplcp_ma_index++] =
				(uint16) bphy_badplcp_delta;
		if (pi->interf->noise.bphy_badplcp_ma_index >= PHY_NOISE_MA_WINDOW_SZ)
			pi->interf->noise.bphy_badplcp_ma_index = 0;
	}

	if (pi->sh->interference_mode == WLAN_AUTO_W_NOISE ||
		pi->sh->interference_mode == NON_WLAN) {
		PHY_ACI(("phy_ht_noise_update_aci_ma: ACI= %s, rxglitch_ma= %d,"
			" badplcp_ma= %d, ofdm_glitch_ma= %d, bphy_glitch_ma=%d,"
			" ofdm_badplcp_ma= %d, bphy_badplcp_ma=%d, crsminpwr index= %d,"
			" init gain= 0x%x, channel= %d\n",
			(pi->aci_state & ACI_ACTIVE) ? "Active" : "Inactive",
			pi->interf->aci.glitch_ma,
			pi->interf->badplcp_ma,
			pi->interf->noise.ofdm_glitch_ma,
			pi->interf->noise.bphy_glitch_ma,
			pi->interf->noise.ofdm_badplcp_ma,
			pi->interf->noise.bphy_badplcp_ma,
			pi->interf->crsminpwr_index,
			pi->interf->init_gain_core1, CHSPEC_CHANNEL(pi->radio_chanspec)));
	}
#ifdef WLSRVSDB
	/* Clear out cumulatiove cntrs after 1 sec */
	/* reset both chan info becuase its a fresh start after every 1 sec */
	if (pi->srvsdb_state->srvsdb_active) {
		bzero(pi->srvsdb_state->num_chan_switch, 2 * sizeof(uint8));
		bzero(pi->srvsdb_state->sum_delta_crsglitch, 2 * sizeof(uint32));
		bzero(pi->srvsdb_state->sum_delta_bphy_crsglitch, 2 * sizeof(uint32));
		bzero(pi->srvsdb_state->sum_delta_prev_badplcp, 2 * sizeof(uint32));
		bzero(pi->srvsdb_state->sum_delta_prev_bphy_badplcp, 2 * sizeof(uint32));
	}
#endif /* WLSRVSDB */
}

#endif /* WLC_DISABLE_ACI */

static void
phy_ht_noise_interf_mode_set(phy_type_noise_ctx_t *ctx, int val)
{
	phy_ht_noise_info_t *info = (phy_ht_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;
	pi->sh->interference_mode_2G_override = val;
	pi->sh->interference_mode_5G_override = val;
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		/* for 2G, all values 0 thru 4 are valid */
		pi->sh->interference_mode = pi->sh->interference_mode_2G_override;
	} else {
		/* for 5G, only values 0 and 1 are valid options */
		if (val == 0 || val == 1) {
			pi->sh->interference_mode = pi->sh->interference_mode_5G_override;
		} else {
			/* default 5G interference value to 0 */
			pi->sh->interference_mode = 0;
		}
	}
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int
phy_ht_noise_dump(phy_type_noise_ctx_t *ctx, struct bcmstrbuf *b)
{
	phy_ht_noise_info_t *info = (phy_ht_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;

	return phy_noise_dump_common(pi, b);
}
#endif

#define PHY_NOISE_MAX_IDLETIME		30

static void
phy_ht_noise_request_sample(phy_type_noise_ctx_t *ctx, uint8 reason, uint8 ch)
{
	phy_ht_noise_info_t *info = (phy_ht_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;
	phy_noise_info_t *noisei = pi->noisei;

	int8 noise_dbm = PHY_NOISE_FIXED_VAL_NPHY;
	bool sampling_in_progress = (pi->phynoise_state != 0);
	bool wait_for_intr = TRUE;

	PHY_NONE(("phy_ht_noise_request_sample: state %d reason %d, channel %d\n",
		pi->phynoise_state, reason, ch));

	/* since polling is atomic, sampling_in_progress equals to interrupt sampling ongoing
	 *  In these collision cases, always yield and wait interrupt to finish, where the results
	 *  maybe be sharable if channel matches in common callback progressing.
	 */
	if (sampling_in_progress) {
		return;
	}

	switch (reason) {
	case PHY_NOISE_SAMPLE_MON:

		pi->phynoise_chan_watchdog = ch;
		pi->phynoise_state |= PHY_NOISE_STATE_MON;

		break;

	case PHY_NOISE_SAMPLE_EXTERNAL:

		pi->phynoise_state |= PHY_NOISE_STATE_EXTERNAL;
		break;

	case PHY_NOISE_SAMPLE_CRSMINCAL:

	default:
		ASSERT(0);
		break;
	}

	/* start test, save the timestamp to recover in case ucode gets stuck */
	pi->phynoise_now = pi->sh->now;

	/* Fixed noise, don't need to do the real measurement */
	if (pi->phy_fixed_noise) {
		uint8 i;
		FOREACH_CORE(pi, i) {
			pi->phy_noise_win[i][pi->phy_noise_index] =
				PHY_NOISE_FIXED_VAL_NPHY;
		}
		pi->phy_noise_index = MODINC_POW2(pi->phy_noise_index,
			PHY_NOISE_WINDOW_SZ);
		/* legacy noise is the max of antennas */
		noise_dbm = PHY_NOISE_FIXED_VAL_NPHY;

		wait_for_intr = FALSE;
		goto done;
	}

	if (!pi->phynoise_polling || (reason == PHY_NOISE_SAMPLE_EXTERNAL) ||
		(reason == PHY_NOISE_SAMPLE_CRSMINCAL)) {

		/* If noise mmt disabled due to LPAS active, do not initiate one */
		if (pi->phynoise_disable)
			return;

		/* Do not do noise mmt if in PM State */
		if (phy_noise_pmstate_get(pi) &&
			!wlapi_phy_awdl_is_on(pi->sh->physhim)) {
			/* Make sure that you do it every PHY_NOISE_MAX_IDLETIME atleast */
			if ((pi->sh->now - pi->phynoise_lastmmttime)
					< PHY_NOISE_MAX_IDLETIME)
				return;
		}
		/* Note current noise request time */
		pi->phynoise_lastmmttime = pi->sh->now;

		/* ucode assumes these shmems start with 0
		 * and ucode will not touch them in case of sampling expires
		 */
		wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP0(pi), 0);
		wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP1(pi), 0);
		wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP2(pi), 0);
		wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP3(pi), 0);

		if (D11REV_GE(pi->sh->corerev, 40)) {
			wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP4(pi), 0);
			wlapi_bmac_write_shm(pi->sh->physhim, M_PWRIND_MAP5(pi), 0);
		}

		W_REG(pi->sh->osh, &pi->regs->maccommand, MCMD_BG_NOISE);

	} else {

		/* polling mode */
		phy_iq_est_t est[PHY_CORE_MAX];
		uint32 cmplx_pwr[PHY_CORE_MAX];
		int8 noise_dbm_ant[PHY_CORE_MAX];
		uint16 log_num_samps, num_samps, classif_state = 0;
		uint8 wait_time = 32;
		uint8 wait_crs = 0;
		uint8 i;

		bzero((uint8 *)est, sizeof(est));
		bzero((uint8 *)cmplx_pwr, sizeof(cmplx_pwr));
		bzero((uint8 *)noise_dbm_ant, sizeof(noise_dbm_ant));

		/* choose num_samps to be some power of 2 */
		log_num_samps = PHY_NOISE_SAMPLE_LOG_NUM_NPHY;
		num_samps = 1 << log_num_samps;

		/* suspend MAC, get classifier settings, turn it off
		 * get IQ power measurements
		 * restore classifier settings and reenable MAC ASAP
		*/
		wlapi_suspend_mac_and_wait(pi->sh->physhim);

		classif_state = wlc_phy_classifier_htphy(pi, 0, 0);
		wlc_phy_classifier_htphy(pi, 3, 0);
		wlc_phy_rx_iq_est_htphy(pi, est, num_samps, wait_time, wait_crs);
		wlc_phy_classifier_htphy(pi,
			HTPHY_ClassifierCtrl_classifierSel_MASK, classif_state);

		wlapi_enable_mac(pi->sh->physhim);

		/* sum I and Q powers for each core, average over num_samps */
		FOREACH_CORE(pi, i)
			cmplx_pwr[i] = (est[i].i_pwr + est[i].q_pwr) >> log_num_samps;

		/* pi->phy_noise_win per antenna is updated inside */
		wlc_phy_noise_calc(pi, cmplx_pwr, noise_dbm_ant, 0);
		wlc_phy_noise_save(pi, noise_dbm_ant, &noise_dbm);

		wait_for_intr = FALSE;
	}

done:
	/* if no interrupt scheduled, populate noise results now */
	if (!wait_for_intr) {
		phy_noise_invoke_callbacks(noisei, ch, noise_dbm);
	}
}

static int8
phy_ht_noise_avg(phy_type_noise_ctx_t *ctx)
{
	phy_ht_noise_info_t *noise_info = (phy_ht_noise_info_t *) ctx;
	phy_info_t *pi = noise_info->pi;

	int tot = 0;
	int i = 0;

	for (i = 0; i < MA_WINDOW_SZ; i++)
		tot += pi->sh->phy_noise_window[i];

	tot /= MA_WINDOW_SZ;

	return (int8)tot;
}

/* ucode finished phy noise measurement and raised interrupt */
static void
phy_ht_noise_sample_intr(phy_type_noise_ctx_t *ctx)
{
	phy_ht_noise_info_t *noise_info = (phy_ht_noise_info_t *) ctx;
	phy_info_t *pi = noise_info->pi;
	phy_noise_info_t *noisei = pi->noisei;

	uint16 jssi_aux;
	uint8 channel = 0;
	int8 noise_dbm = PHY_NOISE_FIXED_VAL_NPHY;
	uint8 lte_on, crs_high;

	/* copy of the M_CURCHANNEL, just channel number plus 2G/5G flag */
	jssi_aux = wlapi_bmac_read_shm(pi->sh->physhim, M_JSSI_AUX(pi));
	channel = jssi_aux & D11_CURCHANNEL_MAX;
	noise_dbm = phy_noise_read_shmem(noisei, &lte_on, &crs_high);
	PHY_INFORM(("%s: noise_dBm = %d, lte_on=%d, crs_high=%d\n",
		__FUNCTION__, noise_dbm, lte_on, crs_high));

	/* rssi dbm computed, invoke all callbacks */
	phy_noise_invoke_callbacks(noisei, channel, noise_dbm);
}

static int8
phy_ht_noise_lte_avg(phy_type_noise_ctx_t *ctx)
{
	return 0;
}

static int
phy_ht_noise_get_srom_level(phy_type_noise_ctx_t *ctx, int32 *ret_int_ptr)
{
	phy_ht_noise_info_t *noisei = (phy_ht_noise_info_t *) ctx;
	phy_info_t *pi = noisei->pi;
	int8 noiselvl[PHY_CORE_MAX] = {0};
	uint8 core;
	uint16 channel;
	uint32 pkd_noise = 0;

	channel = CHSPEC_CHANNEL(pi->radio_chanspec);
	FOREACH_CORE(pi, core) {

		if (channel <= 14) {
			/* 2G */
			noiselvl[core] = pi->noiselvl_2g[core];
		} else {
#ifdef BAND5G
			/* 5G */
			if (channel <= 48) {
				/* 5G-low: channels 36 through 48 */
				noiselvl[core] = pi->noiselvl_5gl[core];
			} else if (channel <= 64) {
				/* 5G-mid: channels 52 through 64 */
				noiselvl[core] = pi->noiselvl_5gm[core];
			} else if (channel <= 128) {
				/* 5G-high: channels 100 through 128 */
				noiselvl[core] = pi->noiselvl_5gh[core];
			} else {
				/* 5G-upper: channels 132 and above */
				noiselvl[core] = pi->noiselvl_5gu[core];
			}
#endif /* BAND5G */
		}
	}

	for (core = PHYCORENUM(pi->pubpi->phy_corenum); core >= 1; core--) {
		pkd_noise = (pkd_noise << 8) | (uint8)(noiselvl[core-1]);
	}
	*ret_int_ptr = pkd_noise;

	return BCME_OK;
}

static int8
phy_ht_noise_read_shmem(phy_type_noise_ctx_t *ctx, uint8 *lte_on, uint8 *crs_high)
{
	phy_ht_noise_info_t *noise_info = (phy_ht_noise_info_t *)ctx;
	phy_info_t *pi = noise_info->pi;
	phy_noise_info_t *noisei = pi->noisei;

	uint32 cmplx_pwr[PHY_CORE_MAX];
	int8 noise_dbm_ant[PHY_CORE_MAX];
	uint16 lo, hi;
	uint32 cmplx_pwr_tot = 0;
	int8 noise_dbm = PHY_NOISE_FIXED_VAL_NPHY;
	uint8 core;

	bzero((uint8 *)cmplx_pwr, sizeof(cmplx_pwr));
	bzero((uint8 *)noise_dbm_ant, sizeof(noise_dbm_ant));

	if (phy_noise_abort_shmem_read(noisei)) {
		return -1;
	}

	/* read SHM, reuse old corerev PWRIND since we are tight on SHM pool */
	FOREACH_CORE(pi, core) {
		lo = wlapi_bmac_read_shm(pi->sh->physhim, M_PWRIND_MAP(pi, 2*core));
		hi = wlapi_bmac_read_shm(pi->sh->physhim, M_PWRIND_MAP(pi, 2*core+1));
		cmplx_pwr[core] = (hi << 16) + lo;
	}
#ifdef OCL
	if (PHY_OCL_ENAB(pi->sh->physhim)) {
		uint8 active_cm, ocl_en;
		phy_ocl_status_get((wlc_phy_t *)pi, NULL, &active_cm, &ocl_en);
		/* copying active core noise to inactive core when ocl_en=1 */
		if (ocl_en)
			cmplx_pwr[!(active_cm >> 1)] = cmplx_pwr[(active_cm >> 1)];
	}
#endif
	FOREACH_CORE(pi, core) {
		cmplx_pwr_tot += cmplx_pwr[core];
		if (cmplx_pwr[core] == 0) {
			noise_dbm_ant[core] = PHY_NOISE_FIXED_VAL_NPHY;
		} else {
			/* Compute rounded average complex power */
			cmplx_pwr[core] = (cmplx_pwr[core] >>
				PHY_NOISE_SAMPLE_LOG_NUM_UCODE) +
				((cmplx_pwr[core] >>
				(PHY_NOISE_SAMPLE_LOG_NUM_UCODE - 1)) & 0x1);
		}
	}

	if (cmplx_pwr_tot == 0) {
		PHY_INFORM(("wlc_phy_noise_sample_nphy_compute: timeout in ucode\n"));
	} else {
		wlc_phy_noise_calc(pi, cmplx_pwr, noise_dbm_ant, 0);
	}

	wlc_phy_noise_save(pi, noise_dbm_ant, &noise_dbm);

	/* legacy noise is the max of antennas */
	*lte_on = 0;
	*crs_high = 0;
	return noise_dbm;
}
