/**
 * @file
 * @brief
 * ratespec related routines
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
 * $Id: wlc_rspec.c 655322 2016-08-19 00:34:25Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_stf.h>
#include <wlc_rsdb.h>
#include <wlc_prot_g.h>
#include <wlc_vht.h>
#include <wlc_txbf.h>
#include <wlc_txc.h>
#include <wlc_ht.h>
#include <wlc_ulb.h>
#include <wlc_rspec.h>
#if defined(WLAMPDU) && defined(WLATF)
#include <wlc_ampdu.h>
#endif


/* module info */
struct wlc_rspec_info {
	wlc_info_t *wlc;
};

/* iovar table */
enum {
	IOV_5G_RATE = 1,
	IOV_5G_MRATE = 2,
	IOV_2G_RATE = 3,
	IOV_2G_MRATE = 4,
	IOV_LAST
};

static const bcm_iovar_t rspec_iovars[] = {
	{"5g_rate", IOV_5G_RATE, IOVF_OPEN_ALLOW, 0, IOVT_UINT32, 0},
	{"5g_mrate", IOV_5G_MRATE, IOVF_OPEN_ALLOW, 0, IOVT_UINT32, 0},
	{"2g_rate", IOV_2G_RATE, IOVF_OPEN_ALLOW, 0, IOVT_UINT32, 0},
	{"2g_mrate", IOV_2G_MRATE, IOVF_OPEN_ALLOW, 0, IOVT_UINT32, 0},
	{NULL, 0, 0, 0, 0, 0}
};

static int wlc_wlrspec2ratespec(wlc_bsscfg_t *bsscfg, uint32 wlrspec, ratespec_t *pratespec);
static int wlc_ratespec2wlrspec(wlc_bsscfg_t *bsscfg, ratespec_t ratespec, uint32 *pwlrspec);
static ratespec_t wlc_rspec_to_rts_rspec_ex(wlc_info_t *wlc, ratespec_t rspec, bool use_rspec,
	bool g_protection);
static void wlc_get_rate_histo_bsscfg(wlc_bsscfg_t *bsscfg, wl_mac_ratehisto_res_t *rhist,
	ratespec_t *most_used_ratespec, ratespec_t *highest_used_ratespec);
static int wlc_set_ratespec_override(wlc_info_t *wlc, int band_id, ratespec_t rspec, bool mcast);

ratespec_t
wlc_lowest_basic_rspec(wlc_info_t *wlc, wlc_rateset_t *rs)
{
	ratespec_t lowest_basic_rspec;
	uint i;

	/* Use the lowest basic rate */
	lowest_basic_rspec = LEGACY_RSPEC(rs->rates[0]);
	for (i = 0; i < rs->count; i++) {
		if (rs->rates[i] & WLC_RATE_FLAG) {

			lowest_basic_rspec = LEGACY_RSPEC(rs->rates[i]);

			break;
		}
	}

#ifdef WL11N
	wlc_rspec_txexp_upd(wlc, &lowest_basic_rspec);
#endif

	return (lowest_basic_rspec);
}

void
wlc_rspec_txexp_upd(wlc_info_t *wlc, ratespec_t *rspec)
{
	if (RSPEC_ISOFDM(*rspec)) {
		if (WLCISNPHY(wlc->band) &&
		    wlc->stf->ss_opmode == PHY_TXC1_MODE_CDD) {
			*rspec |= (1 << RSPEC_TXEXP_SHIFT);
		}
		else if (WLCISHTPHY(wlc->band) || WLCISACPHY(wlc->band)) {
			uint ntx = wlc_stf_txchain_get(wlc, *rspec);
			uint nss = wlc_ratespec_nss(*rspec);
			*rspec |= ((ntx - nss) << RSPEC_TXEXP_SHIFT);
		}
	}
}

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */

#include <wlc_patch.h>

/* iovar dispatcher */
static int
wlc_rspec_doiovar(void *hdl, uint32 actionid,
	void *params, uint p_len, void *arg, uint a_len, uint val_size, struct wlc_if *wlcif)
{
	wlc_rspec_info_t *rsi = hdl;
	wlc_info_t *wlc = rsi->wlc;
	int err = BCME_OK;
	int32 int_val = 0;
	wlc_bsscfg_t *bsscfg;
	int32 *ret_int_ptr;

	/* update bsscfg w/provided interface context */
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);

	/* convenience int and bool vals for first 8 bytes of buffer */
	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	/* Do the actual parameter implementation */
	switch (actionid) {

	case IOV_GVAL(IOV_5G_RATE): {
		uint32 wlrspec;
		ratespec_t ratespec = wlc->bandstate[BAND_5G_INDEX]->rspec_override;

		err = wlc_ratespec2wlrspec(bsscfg, ratespec, &wlrspec);
		*ret_int_ptr = wlrspec;
		break;
	}

	case IOV_SVAL(IOV_5G_RATE):
		err = wlc_set_iovar_ratespec_override(wlc, bsscfg, WLC_BAND_5G, (uint32)int_val,
				FALSE);
		break;

	case IOV_GVAL(IOV_5G_MRATE): {
		uint32 wlrspec;
		ratespec_t ratespec = wlc->bandstate[BAND_5G_INDEX]->mrspec_override;

		err = wlc_ratespec2wlrspec(bsscfg, ratespec, &wlrspec);
		*ret_int_ptr = wlrspec;
		break;
	}

	case IOV_SVAL(IOV_5G_MRATE):
		err = wlc_set_iovar_ratespec_override(wlc, bsscfg, WLC_BAND_5G, (uint32)int_val,
				TRUE);
		break;

	case IOV_GVAL(IOV_2G_RATE): {
		uint32 wlrspec;
		ratespec_t ratespec = wlc->bandstate[BAND_2G_INDEX]->rspec_override;

		err = wlc_ratespec2wlrspec(bsscfg, ratespec, &wlrspec);
		*ret_int_ptr = wlrspec;
		break;
	}

	case IOV_SVAL(IOV_2G_RATE):
		err = wlc_set_iovar_ratespec_override(wlc, bsscfg, WLC_BAND_2G, (uint32)int_val,
				FALSE);
		break;

	case IOV_GVAL(IOV_2G_MRATE): {
		uint32 wlrspec;
		ratespec_t ratespec = wlc->bandstate[BAND_2G_INDEX]->mrspec_override;

		err = wlc_ratespec2wlrspec(bsscfg, ratespec, &wlrspec);
		*ret_int_ptr = wlrspec;
		break;
	}

	case IOV_SVAL(IOV_2G_MRATE):
		err = wlc_set_iovar_ratespec_override(wlc, bsscfg, WLC_BAND_2G, (uint32)int_val,
				TRUE);
		break;


	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
} /* wlc_rspec_doiovar */

#if defined(MBSS) || defined(WLTXMONITOR)
/** convert rate in OFDM PLCP to ratespec */
ratespec_t
ofdm_plcp_to_rspec(uint8 rate)
{
	ratespec_t rspec = 0;


	switch (rate & 0x0F) {
	case 0x0B: /* 6Mbps */
		rspec = 6*2;
		break;
	case 0x0F: /* 9Mbps */
		rspec = 9*2;
		break;
	case 0x0A: /* 12 Mbps */
		rspec = 12*2;
		break;
	case 0x0E: /* 18 Mbps */
		rspec = 18*2;
		break;
	case 0x09: /* 24 Mbps */
		rspec = 24*2;
		break;
	case 0x0D: /* 36 Mbps */
		rspec = 36*2;
		break;
	case 0x08: /* 48 Mbps */
		rspec = 48*2;
		break;
	case 0x0C: /* 54 Mbps */
		rspec = 54*2;
		break;
	default:
		/* Not a valid OFDM rate */
		ASSERT(0);
		break;
	}

	return rspec;
} /* ofdm_plcp_to_rspec */
#endif /* MBSS || WLTXMONITOR */

/** transmit related */
ratespec_t
wlc_rspec_to_rts_rspec(wlc_bsscfg_t *cfg, ratespec_t rspec, bool use_rspec)
{
	wlc_info_t *wlc = cfg->wlc;
	bool g_prot = wlc->band->gmode && WLC_PROT_G_CFG_G(wlc->prot_g, cfg);
	return wlc_rspec_to_rts_rspec_ex(wlc, rspec, use_rspec, g_prot);
}

static ratespec_t
wlc_rspec_to_rts_rspec_ex(wlc_info_t *wlc, ratespec_t rspec, bool use_rspec, bool g_prot)
{
	ratespec_t rts_rspec = 0;

	if (use_rspec) {
		/* use frame rate as rts rate */
		rts_rspec = rspec;

	} else if (g_prot && !RSPEC_ISCCK(rspec)) {
		/* Use 11Mbps as the g protection RTS target rate and fallback.
		 * Use the WLC_BASIC_RATE() lookup to find the best basic rate under the
		 * target in case 11 Mbps is not Basic.
		 * 6 and 9 Mbps are not usually selected by rate selection, but even
		 * if the OFDM rate we are protecting is 6 or 9 Mbps, 11 is more robust.
		 */
		rts_rspec = WLC_BASIC_RATE(wlc, WLC_RATE_11M);
	} else {
		/* calculate RTS rate and fallback rate based on the frame rate
		 * RTS must be sent at a basic rate since it is a
		 * control frame, sec 9.6 of 802.11 spec
		 */
		rts_rspec = WLC_BASIC_RATE(wlc, rspec);
	}

#ifdef WL11N
	if (WLC_PHY_11N_CAP(wlc->band)) {
		/* set rts txbw to correct side band */
		rts_rspec &= ~RSPEC_BW_MASK;

		/* if rspec/rspec_fallback is 40MHz, then send RTS on both 20MHz channel
		 * (DUP), otherwise send RTS on control channel
		 */
		if (RSPEC_IS40MHZ(rspec) && !RSPEC_ISCCK(rts_rspec))
			rts_rspec |= RSPEC_BW_40MHZ;
		else
			rts_rspec |= RSPEC_BW_20MHZ;

		 if ((RSPEC_IS40MHZ(rts_rspec) &&
		        (CHSPEC_IS20(wlc->chanspec))) ||
		        (RSPEC_ISCCK(rts_rspec))) {
			rts_rspec &= ~RSPEC_BW_MASK;
			rts_rspec |= RSPEC_BW_20MHZ;
		}

		/* pick siso/cdd as default for ofdm */
		if (RSPEC_ISOFDM(rts_rspec)) {
			rts_rspec &= ~RSPEC_TXEXP_MASK;
			if (wlc->stf->ss_opmode == PHY_TXC1_MODE_CDD)
				rts_rspec |= (1 << RSPEC_TXEXP_SHIFT);
		}
	}
#else
	/* set rts txbw to control channel BW if 11 n feature is not turned on */
	rts_rspec &= ~RSPEC_BW_MASK;
	rts_rspec |= RSPEC_BW_20MHZ;
#endif /* WL11N */

	return rts_rspec;
} /* wlc_rspec_to_rts_rspec_ex */

uint32
wlc_get_current_highest_rate(wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = cfg->wlc;
	/* return a default value for rate */
	ratespec_t reprspec;
	uint8 bw = BW_20MHZ;
	bool sgi = (WLC_HT_GET_SGI_TX(wlc->hti)) ? TRUE : FALSE;
	wlcband_t *cur_band = wlc->band;
	wlc_bss_info_t *current_bss = cfg->current_bss;

	if (cfg->associated) {
		uint8 vht_ratemask = 0;
		bool vht_ldpc = FALSE;

		if (N_ENAB(wlc->pub) && CHSPEC_IS40(current_bss->chanspec))
			bw = BW_40MHZ;
#ifdef WL11AC
		if (VHT_ENAB_BAND(wlc->pub, cur_band->bandtype)) {
			if (CHSPEC_IS80(current_bss->chanspec)) {
				bw = BW_80MHZ;
			}

			vht_ldpc = ((wlc->stf->ldpc_tx == AUTO) || (wlc->stf->ldpc_tx == ON));
			vht_ratemask = BAND_5G(wlc->band->bandtype) ?
				WLC_VHT_FEATURES_RATES_5G(wlc->pub) :
				WLC_VHT_FEATURES_RATES_2G(wlc->pub);
			/*
			* STA Mode  Look for scb corresponding to the bss.
			* APSTA Mode default to LDPC setting for AP mode.
			*/
			if (!AP_ENAB(wlc->pub)) {
				struct scb *scb = wlc_scbfind(wlc, cfg, &current_bss->BSSID);
				if (scb) {
					vht_ratemask = wlc_vht_get_scb_ratemask(wlc->vhti, scb);
					vht_ldpc &= (SCB_VHT_LDPC_CAP(wlc->vhti, scb));
				}
			}
		}
#endif /* WL11AC */
		/* associated, so return the max supported rate */
		reprspec = wlc_get_highest_rate(&current_bss->rateset, bw, sgi,
			vht_ldpc, vht_ratemask, wlc->stf->txstreams);
	} else {
		if (N_ENAB(wlc->pub)) {
			if (wlc_ht_is_40MHZ_cap(wlc->hti) &&
				((wlc_channel_locale_flags_in_band(wlc->cmi, cur_band->bandunit) &
				WLC_NO_40MHZ) == 0) &&
				WL_BW_CAP_40MHZ(cur_band->bw_cap)) {
				bw = BW_40MHZ;
			}
		}
		if (VHT_ENAB_BAND(wlc->pub, cur_band->bandtype)) {
			if (((wlc_channel_locale_flags_in_band(wlc->cmi, cur_band->bandunit) &
				WLC_NO_80MHZ) == 0) && WL_BW_CAP_80MHZ(cur_band->bw_cap)) {
				bw = BW_80MHZ;
			}
		}

		/* not associated, so return the max rate we can send */
		reprspec = wlc_get_highest_rate(&cur_band->hw_rateset, bw, sgi,
			(wlc->stf->ldpc_tx == AUTO), (BAND_5G(cur_band->bandtype)?
			WLC_VHT_FEATURES_RATES_5G(wlc->pub) : WLC_VHT_FEATURES_RATES_2G(wlc->pub)),
			wlc->stf->txstreams);

		/* Report the highest CCK rate if gmode == GMODE_LEGACY_B */
		if (BAND_2G(cur_band->bandtype) && (cur_band->gmode == GMODE_LEGACY_B)) {
			wlc_rateset_t rateset;
			wlc_rateset_filter(&cur_band->hw_rateset /* src */, &rateset, FALSE,
				WLC_RATES_CCK, RATE_MASK_FULL, wlc_get_mcsallow(wlc, cfg));
			reprspec = rateset.rates[rateset.count - 1];
		}
	}
	return reprspec;
} /* wlc_get_current_highest_rate */

/** return the "current tx rate" as a ratespec */
uint32
wlc_get_rspec_history(wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = cfg->wlc;
	wl_mac_ratehisto_res_t rate_histo;
	ratespec_t reprspec, highest_used_ratespec = 0;
	wlcband_t *cur_band = wlc->band;
	wlc_bss_info_t *current_bss = cfg->current_bss;
	chanspec_t chspec;

	bzero(&rate_histo, sizeof(wl_mac_ratehisto_res_t));

	if (cfg->associated)
		cur_band = wlc->bandstate[CHSPEC_IS2G(current_bss->chanspec) ?
			BAND_2G_INDEX : BAND_5G_INDEX];

	reprspec = cur_band->rspec_override ? cur_band->rspec_override : 0;
	if (reprspec) {
		if (RSPEC_ISHT(reprspec) || RSPEC_ISVHT(reprspec)) {
			/* If bw in override spec not specified, use bw from chanspec */
			if (RSPEC_BW(reprspec) == RSPEC_BW_UNSPECIFIED) {

				if (cfg->associated) {
					chspec = current_bss->chanspec;
				} else {
					chspec = WLC_BAND_PI_RADIO_CHANSPEC;
				}

				if (WLC_ULB_CHSPEC_ISLE20(wlc, chspec)) {
					reprspec |= RSPEC_BW_20MHZ;
				} else if (CHSPEC_IS40(chspec)) {
					reprspec |= RSPEC_BW_40MHZ;
				} else if (CHSPEC_IS80(chspec)) {
					reprspec |= RSPEC_BW_80MHZ;
				} else if (CHSPEC_IS8080(chspec) || CHSPEC_IS160(chspec)) {
					reprspec |= RSPEC_BW_160MHZ;
				}

			}
			if (WLC_HT_GET_SGI_TX(wlc->hti) == ON)
				reprspec |= RSPEC_SHORT_GI;
#if defined(WL_BEAMFORMING)
			if (TXBF_ENAB(wlc->pub) && wlc_txbf_get_applied2ovr(wlc->txbf))
				reprspec |= RSPEC_TXBF;
#endif
		}
		return reprspec;
	}

	/*
	 * Loop over txrspec history, looking up rate bins, and summing
	 * nfrags into appropriate supported rate bin.
	 */
	wlc_get_rate_histo_bsscfg(cfg, &rate_histo, &reprspec, &highest_used_ratespec);

	/* check for an empty history */
	if (reprspec == 0)
		return wlc_get_current_highest_rate(cfg);

	return (wlc->rpt_hitxrate ? highest_used_ratespec : reprspec);
} /* wlc_get_rspec_history */

/** Create the internal ratespec_t from the wl_ratespec */
static int
wlc_wlrspec2ratespec(wlc_bsscfg_t *bsscfg, uint32 wlrspec, ratespec_t *pratespec)
{
	bool islegacy = ((wlrspec & WL_RSPEC_ENCODING_MASK) == WL_RSPEC_ENCODE_RATE);
	bool isht     = ((wlrspec & WL_RSPEC_ENCODING_MASK) == WL_RSPEC_ENCODE_HT);
	bool isvht    = ((wlrspec & WL_RSPEC_ENCODING_MASK) == WL_RSPEC_ENCODE_VHT);
	uint8 rate    =  (wlrspec & WL_RSPEC_RATE_MASK);
	uint32 bw     =  (wlrspec & WL_RSPEC_BW_MASK);
	uint txexp    = ((wlrspec & WL_RSPEC_TXEXP_MASK) >> WL_RSPEC_TXEXP_SHIFT);
	bool isstbc   = ((wlrspec & WL_RSPEC_STBC) != 0);
	bool issgi    = ((wlrspec & WL_RSPEC_SGI) != 0);
	bool isldpc   = ((wlrspec & WL_RSPEC_LDPC) != 0);
	ratespec_t rspec;

	*pratespec = 0;

	/* check for an uspecified rate */
	if (wlrspec == 0) {
		/* pratespec has already been cleared, return OK */
		return BCME_OK;
	}

	/* set the encoding to legacy, HT, or VHT as specified, and set the rate/mcs field */
	if (islegacy) {
		rspec = LEGACY_RSPEC(rate);

		/* It is not a real legacy rate if the specified bw is not 20MHz (e.g. OFDM DUP40)
		 * clear the bandwidth bits so it can be correctly applied later.
		 */
		if (bw && (bw != WL_RSPEC_BW_20MHZ)) {
			rspec &= ~RSPEC_BW_MASK;
		}
	} else if (isht) {
		rspec = HT_RSPEC(rate);
	} else if (isvht) {
		uint mcs = (WL_RSPEC_VHT_MCS_MASK & wlrspec);
		uint nss = ((WL_RSPEC_VHT_NSS_MASK & wlrspec) >> WL_RSPEC_VHT_NSS_SHIFT);
		if (IS_PROPRIETARY_VHT_MCS_10_11(mcs)) {
			isldpc = TRUE;
		}
		rspec = VHT_RSPEC(mcs, nss);
	} else {
		return BCME_BADARG;
	}

	/* Tx chain expansion */
	rspec |= (txexp << RSPEC_TXEXP_SHIFT);

	/* STBC, LDPC and Short GI */
	if (isstbc) {
		rspec |= RSPEC_STBC;
	}

	if (isldpc) {
		 rspec |= RSPEC_LDPC_CODING;
	}

	if (issgi) {
		 rspec |= RSPEC_SHORT_GI;
	}

	/* Bandwidth */
#ifdef WL11ULB
	/* In ULB Mode BW is forced to 20MHz always */
	if (WLC_ULB_MODE_ENABLED(bsscfg->wlc) &&
		((bw == WL_RSPEC_BW_10MHZ) || (bw == WL_RSPEC_BW_5MHZ) ||
		(bw == WL_RSPEC_BW_2P5MHZ))) {
		bw = WL_RSPEC_BW_20MHZ;
	}
#endif /* WL11ULB */

	if (bw == WL_RSPEC_BW_20MHZ) {
		rspec |= RSPEC_BW_20MHZ;
	} else if (bw == WL_RSPEC_BW_40MHZ) {
		rspec |= RSPEC_BW_40MHZ;
	} else if (bw == WL_RSPEC_BW_80MHZ) {
		rspec |= RSPEC_BW_80MHZ;
	} else if (bw == WL_RSPEC_BW_160MHZ) {
		rspec |= RSPEC_BW_160MHZ;
	}

	*pratespec = rspec;

	return BCME_OK;
} /* wlc_wlrspec2ratespec */

/** Create the internal ratespec_t from the wl_ratespec */
static int
wlc_ratespec2wlrspec(wlc_bsscfg_t *bsscfg, ratespec_t ratespec, uint32 *pwlrspec)
{
	uint8 rate = (ratespec & RSPEC_RATE_MASK);
	ratespec_t wlrspec;
#ifdef WL11ULB
	chanspec_t min_bw = WLC_ULB_GET_BSS_MIN_BW(bsscfg->wlc, bsscfg);
#endif /* WL11ULB */

	*pwlrspec = 0;

	/* set the encoding to legacy, HT, or VHT as specified, and set the rate/mcs field */
	if (RSPEC_ISLEGACY(ratespec)) {
		wlrspec = (WL_RSPEC_ENCODE_RATE | (rate & WL_RSPEC_RATE_MASK));
	} else if (RSPEC_ISHT(ratespec)) {
		wlrspec = (WL_RSPEC_ENCODE_HT | (rate & WL_RSPEC_RATE_MASK));
	} else if (RSPEC_ISVHT(ratespec)) {
		uint mcs = (RSPEC_VHT_MCS_MASK & ratespec);
		uint nss = ((RSPEC_VHT_NSS_MASK & ratespec) >> RSPEC_VHT_NSS_SHIFT);

		wlrspec = (WL_RSPEC_ENCODE_VHT |
		           ((nss << WL_RSPEC_VHT_NSS_SHIFT) & WL_RSPEC_VHT_NSS_MASK) |
		           (mcs & WL_RSPEC_VHT_MCS_MASK));
	} else {
		return BCME_BADARG;
	}

	/* Tx chain expansion, STBC, LDPC and Short GI */
	wlrspec |= RSPEC_TXEXP(ratespec) << WL_RSPEC_TXEXP_SHIFT;
	wlrspec |= RSPEC_ISSTBC(ratespec) ? WL_RSPEC_STBC : 0;
	wlrspec |= RSPEC_ISLDPC(ratespec) ? WL_RSPEC_LDPC : 0;
	wlrspec |= RSPEC_ISSGI(ratespec) ? WL_RSPEC_SGI : 0;
#ifdef WL_BEAMFORMING
	if (RSPEC_ISTXBF(ratespec)) {
		wlrspec |= WL_RSPEC_TXBF;
	}
#endif
	/* Bandwidth */
	if (RSPEC_IS20MHZ(ratespec)) {
#ifdef WL11ULB
		if (WLC_ULB_MODE_ENABLED(bsscfg->wlc) && (min_bw != WL_CHANSPEC_BW_20)) {
			if (min_bw == WL_CHANSPEC_BW_10)
				wlrspec |= WL_RSPEC_BW_10MHZ;
			else if (min_bw == WL_CHANSPEC_BW_5)
				wlrspec |= WL_RSPEC_BW_5MHZ;
			else if (min_bw == WL_CHANSPEC_BW_2P5)
				wlrspec |= WL_RSPEC_BW_2P5MHZ;

		} else
#endif /* WL11ULB */
			wlrspec |= WL_RSPEC_BW_20MHZ;
	} else if (RSPEC_IS40MHZ(ratespec)) {
		wlrspec |= WL_RSPEC_BW_40MHZ;
	} else if (RSPEC_IS80MHZ(ratespec)) {
		wlrspec |= WL_RSPEC_BW_80MHZ;
	} else if (RSPEC_IS160MHZ(ratespec)) {
		wlrspec |= WL_RSPEC_BW_160MHZ;
	}

	if (ratespec & RSPEC_OVERRIDE_RATE)
		wlrspec |= WL_RSPEC_OVERRIDE_RATE;
	if (ratespec & RSPEC_OVERRIDE_MODE)
		wlrspec |= WL_RSPEC_OVERRIDE_MODE;

	*pwlrspec = wlrspec;

	return BCME_OK;
} /* wlc_ratespec2wlrspec */

int
wlc_set_iovar_ratespec_override(wlc_info_t *wlc, wlc_bsscfg_t *cfg, int band_id, uint32 wl_rspec,
	bool mcast)
{
	ratespec_t rspec;
	int err;

	WL_TRACE(("wl%d: %s: band %d wlrspec 0x%08X mcast %d\n", wlc->pub->unit, __FUNCTION__,
	          band_id, wl_rspec, mcast));

	err = wlc_wlrspec2ratespec(cfg, wl_rspec, &rspec);

	if (!err) {
		err = wlc_set_ratespec_override(wlc, band_id, rspec, mcast);
	} else {
		WL_TRACE(("wl%d: %s: rspec translate err %d\n",
		          wlc->pub->unit, __FUNCTION__, err));
	}
#ifdef WLRSDB
	if (RSDB_ENAB(wlc->pub) && (!err)) {
		wlc_rsdb_config_auto_mode_switch(wlc, WLC_RSDB_AUTO_OVERRIDE_RATE,
			(wl_rspec));
	}
#endif /* WLRSDB */
	return err;
}

/** transmit related, called when e.g. the user configures a fixed tx rate using the wl utility */
static int
wlc_set_ratespec_override(wlc_info_t *wlc, int band_id, ratespec_t rspec, bool mcast)
{
	wlcband_t *band;
	bool islegacy = RSPEC_ISLEGACY(rspec) && (rspec != 0); /* pre 11n */
	bool isht = RSPEC_ISHT(rspec);	/* 11n */
	bool isvht = RSPEC_ISVHT(rspec);
	uint8 rate = (rspec & RSPEC_RATE_MASK);
	uint32 bw = RSPEC_BW(rspec);
	uint txexp = RSPEC_TXEXP(rspec);
	bool isstbc = RSPEC_ISSTBC(rspec);
	bool issgi = RSPEC_ISSGI(rspec);
	bool isldpc = RSPEC_ISLDPC(rspec);
	uint Nss, Nsts;
	int bcmerror = 0;

	/* Default number of space-time streams for all legacy OFDM/DSSS/CCK is 1 */
	Nsts = 1;

	if (band_id == WLC_BAND_2G) {
		band = wlc->bandstate[BAND_2G_INDEX];
	} else {
		band = wlc->bandstate[BAND_5G_INDEX];
	}

	WL_TRACE(("wl%d: %s: band %d rspec 0x%08X mcast %d\n", wlc->pub->unit, __FUNCTION__,
	          band_id, rspec, mcast));

	/* check for HT/VHT mode items being set on a legacy rate */
	if (islegacy &&
	    (isldpc || issgi || isstbc)) {
		bcmerror = BCME_BADARG;
		WL_NONE(("wl%d: %s: err, legacy rate with ldpc:%d sgi:%d stbc:%d\n",
		         wlc->pub->unit, __FUNCTION__, isldpc, issgi, isstbc));
		goto done;
	}

	/* validate the combination of rate/mcs/stf is allowed */
	if (VHT_ENAB_BAND(wlc->pub, band_id) && isvht) {
		uint8 mcs = rate & WL_RSPEC_VHT_MCS_MASK;
		uint8 mcs_limit = WLC_STD_MAX_VHT_MCS;

		Nss = (rate & WL_RSPEC_VHT_NSS_MASK) >> WL_RSPEC_VHT_NSS_SHIFT;

		WL_NONE(("wl%d: %s: VHT, mcs:%d Nss:%d\n",
		         wlc->pub->unit, __FUNCTION__, mcs, Nss));

		/* VHT (11ac) supports MCS 0-11 */
		if (WLC_1024QAM_CAP_PHY(wlc))
			mcs_limit = WLC_MAX_VHT_MCS;

		if (mcs > mcs_limit) {
			bcmerror = BCME_BADARG;
			goto done;
		}

		/* VHT (11ac) supports Nss 1-8 */
		if (Nss == 0 || Nss > 8) {
			bcmerror = BCME_BADARG;
			goto done;
		}

		/* STBC only supported by some phys */
		if (isstbc && !WLC_STBC_CAP_PHY(wlc)) {
			bcmerror = BCME_RANGE;
			goto done;
		}

		/* STBC expansion doubles the Nss */
		if (isstbc) {
			Nsts = Nss * 2;
		} else {
			Nsts = Nss;
		}
	} else if (N_ENAB(wlc->pub) && isht) {
		/* mcs only allowed when nmode */
		uint8 mcs = rate;

		/* none of the 11n phys support the defined HT MCS 33-76 */
		if (mcs > 32) {
			if (!WLPROPRIETARY_11N_RATES_ENAB(wlc->pub) ||
			    !WLC_HT_PROP_RATES_CAP_PHY(wlc) || !IS_PROPRIETARY_11N_MCS(mcs) ||
			    wlc->pub->ht_features == WLC_HT_FEATURES_PROPRATES_DISAB) {
				bcmerror = BCME_RANGE;
				goto done;
			}
		}

		/* calculate the number of spatial streams, Nss.
		 * MCS 32 is a 40 MHz single stream, otherwise
		 * 0-31 follow a pattern of 8 MCS per Nss.
		 */
		if (WLPROPRIETARY_11N_RATES_ENAB(wlc->pub)) {
			Nss = GET_11N_MCS_NSS(mcs);
		} else {
			Nss = (mcs == 32) ? 1 : (1 + (mcs / 8));
		}

		/* STBC only supported by some phys */
		if (isstbc && !WLC_STBC_CAP_PHY(wlc)) {
			bcmerror = BCME_RANGE;
			goto done;
		}

		/* BRCM 11n phys only support STBC expansion doubling the num spatial streams */
		if (isstbc) {
			Nsts = Nss * 2;
		} else {
			Nsts = Nss;
		}

		/* mcs 32 is a special case, DUP mode 40 only */
		if (mcs == 32) {
			if (WLC_ULB_CHSPEC_ISLE20(wlc, wlc->home_chanspec) ||
				(isstbc && (txexp > 1))) {
				bcmerror = BCME_RANGE;
				goto done;
			}
		}
	} else if (islegacy) {
		if (RATE_ISCCK(rate)) {
			/* DSSS/CCK only allowed in 2.4G */
			if (band->bandtype != WLC_BAND_2G) {
				bcmerror = BCME_RANGE;
				goto done;
			}
		} else if (BAND_2G(band->bandtype) && (band->gmode == GMODE_LEGACY_B)) {
			/* allow only CCK if gmode == GMODE_LEGACY_B */
			bcmerror = BCME_RANGE;
			goto done;
		} else if (!RATE_ISOFDM(rate)) {
			bcmerror = BCME_RANGE;
			goto done;
		}
	} else if (rspec != 0) {
		bcmerror = BCME_RANGE;
		goto done;
	}

	/* Validate a BW override if given
	 *
	 * Make sure the phy is capable
	 * Only VHT for 80MHz/80+80Mhz/160Mhz
	 * VHT/HT/OFDM for 40MHz, not CCK/DSSS
	 * MCS32 cannot be 20MHz, always 40MHz
	 */

	if (bw == RSPEC_BW_160MHZ) {
		if (!(WLC_8080MHZ_CAP_PHY(wlc) || WLC_160MHZ_CAP_PHY(wlc))) {
			bcmerror = BCME_RANGE;
			goto done;
		}
		/* only VHT & legacy mode can be specified for >40MHz */
		if (!isvht) {
			bcmerror = BCME_BADARG;
			goto done;
		}
	} else if (bw == RSPEC_BW_80MHZ) {
		if (!WLC_80MHZ_CAP_PHY(wlc)) {
			bcmerror = BCME_RANGE;
			goto done;
		}
		/* only VHT & legacy mode can be specified for >40MHz */
		if (!isvht && !islegacy) {
			bcmerror = BCME_BADARG;
			goto done;
		}
	} else if (bw == RSPEC_BW_40MHZ) {
		if (!WLC_40MHZ_CAP_PHY(wlc)) {
			bcmerror = BCME_RANGE;
			goto done;
		}
		/* DSSS/CCK are only 20MHz */
		if (RSPEC_ISCCK(rspec)) {
			bcmerror = BCME_BADARG;
			goto done;
		}
	} else if (bw == RSPEC_BW_20MHZ) {
		/* MCS 32 is 40MHz, cannot be forced to 20MHz */
		if (isht && rate == 32) {
			bcmerror = BCME_BADARG;
		}
	}

	/* more error checks if the rate is not 0 == 'auto' */
	if (rspec != 0) {
		/* add the override flags */
		rspec = (rspec | RSPEC_OVERRIDE_RATE | RSPEC_OVERRIDE_MODE);

		/* make sure there are enough tx streams available for what is specified */
		if ((Nsts + txexp) > wlc->stf->txstreams) {
			WL_TRACE(("wl%d: %s: err, (Nsts %d + txexp %d) > txstreams %d\n",
			          wlc->pub->unit, __FUNCTION__, Nsts, txexp, wlc->stf->txstreams));
			bcmerror = BCME_RANGE;
			goto done;
		}

		if ((rspec != 0) && !wlc_valid_rate(wlc, rspec, band->bandtype, TRUE)) {
			WL_TRACE(("wl%d: %s: err, failed wlc_valid_rate()\n",
			          wlc->pub->unit, __FUNCTION__));
			bcmerror = BCME_RANGE;
			goto done;
		}
	}

	/* set the rspec_override/mrspec_override for the given band */
	if (mcast) {
		band->mrspec_override = rspec;
	} else {
		band->rspec_override = rspec;
#if defined(WLAMPDU) && defined(WLATF)
	/* Let ATF know we are overriding the rate */
		wlc_ampdu_atf_rate_override(wlc, rspec, band);
#endif
	}

	if (!mcast) {
		wlc_reprate_init(wlc); /* reported rate initialization */
	}

	/* invalidate txcache as transmit rate has changed */
	if (WLC_TXC_ENAB(wlc))
		wlc_txc_inv_all(wlc->txc);

done:
	return bcmerror;
} /* wlc_set_ratespec_override */

/** per bsscfg init tx reported rate mechanism */
void
wlc_bsscfg_reprate_init(wlc_bsscfg_t *bsscfg)
{
	bsscfg->txrspecidx = 0;
	bzero((char*)bsscfg->txrspec, sizeof(bsscfg->txrspec));
}

/**
 * Loop over bsscfg specific txrspec history, looking up rate bins, and summing
 * nfrags into appropriate supported rate bin. Return pointers to
 * most used ratespec and highest used ratespec.
 */
static void
wlc_get_rate_histo_bsscfg(wlc_bsscfg_t *bsscfg, wl_mac_ratehisto_res_t *rhist,
	ratespec_t *most_used_ratespec, ratespec_t *highest_used_ratespec)
{
	int i;
	ratespec_t rspec;
	uint max_frags = 0;
	uint rate, mcs, nss;
	uint high_frags = 0;

	*most_used_ratespec = 0x0;
	*highest_used_ratespec = 0x0;

	/* [0] = rspec, [1] = nfrags */
	for (i = 0; i < NTXRATE; i++) {
		rspec = bsscfg->txrspec[i][0]; /* circular buffer of prev MPDUs tx rates */
		/* skip empty rate specs */
		if (rspec == 0)
			continue;
		if (RSPEC_ISVHT(rspec)) {
			mcs = rspec & RSPEC_VHT_MCS_MASK;
			nss = ((rspec & RSPEC_VHT_NSS_MASK) >> RSPEC_VHT_NSS_SHIFT) - 1;
			ASSERT(mcs < WL_RATESET_SZ_VHT_MCS_P);
			ASSERT(nss < WL_TX_CHAINS_MAX);
			rhist->vht[mcs][nss] += bsscfg->txrspec[i][1]; /* [1] is for fragments */
			if (rhist->vht[mcs][nss] > max_frags) {
				max_frags = rhist->vht[mcs][nss];
				*most_used_ratespec = rspec;
			}
		} else if (IS_MCS(rspec)) {
			mcs = rspec & RSPEC_RATE_MASK;
#if defined(WLPROPRIETARY_11N_RATES) /* avoid ROM abandoning of this function */
			if (IS_PROPRIETARY_11N_MCS(mcs)) {
				mcs -= WLC_11N_FIRST_PROP_MCS;
				rhist->prop11n_mcs[mcs] += bsscfg->txrspec[i][1];
				if (rhist->prop11n_mcs[mcs] > max_frags) {
					max_frags = rhist->prop11n_mcs[mcs];
					*most_used_ratespec = rspec;
				}
			} else
#endif /* WLPROPRIETARY_11N_RATES */
			{
			/* ASSERT(mcs < WL_RATESET_SZ_HT_IOCTL * WL_TX_CHAINS_MAX); */
			if (mcs >= WL_RATESET_SZ_HT_IOCTL * WL_TX_CHAINS_MAX)
				continue;  /* Ignore mcs 32 if it ever comes through. */
			rhist->mcs[mcs] += bsscfg->txrspec[i][1];
			if (rhist->mcs[mcs] > max_frags) {
				max_frags = rhist->mcs[mcs];
				*most_used_ratespec = rspec;
			}
			}
		} else {
			rate = rspec & RSPEC_RATE_MASK;
			ASSERT(rate < (WLC_MAXRATE + 1));
			rhist->rate[rate] += bsscfg->txrspec[i][1];
			if (rhist->rate[rate] > max_frags) {
				max_frags = rhist->rate[rate];
				*most_used_ratespec = rspec;
			}
		}

		rate = RSPEC2KBPS(rspec);
		if (rate > high_frags) {
			high_frags = rate;
			*highest_used_ratespec = rspec;
		}
	}
	return;
}

wlc_rspec_info_t *
BCMATTACHFN(wlc_rspec_attach)(wlc_info_t *wlc)
{
	wlc_rspec_info_t *rsi;

	/* allocate module info */
	if ((rsi = MALLOCZ(wlc->osh, sizeof(*rsi))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	rsi->wlc = wlc;

	if (wlc_module_register(wlc->pub, rspec_iovars, "rspec", rsi, wlc_rspec_doiovar,
			NULL, NULL, NULL)) {
		WL_ERROR(("wl%d: %s wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	return rsi;

fail:
	return NULL;
}

void
BCMATTACHFN(wlc_rspec_detach)(wlc_rspec_info_t *rsi)
{
	wlc_info_t *wlc;

	if (rsi == NULL)
		return;

	wlc = rsi->wlc;

	(void)wlc_module_unregister(wlc->pub, "rspec", rsi);

	MFREE(wlc->osh, rsi, sizeof(*rsi));
}
