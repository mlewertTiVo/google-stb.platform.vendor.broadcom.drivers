/*
 * Monitor Mode routines.
 * This header file housing the Monitor Mode routines implementation.
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
 * $Id: bcmwifi_monitor.c 512698 2016-02-11 13:12:15Z $
 */

#ifndef WL11N
#define WL11N
#endif
#ifndef WL11AC
#define WL11AC
#endif

#include <bcmutils.h>
#include <bcmendian.h>
#include <bcmwifi_channels.h>
#include <proto/monitor.h>
#include <bcmwifi_radiotap.h>
#include <bcmwifi_monitor.h>
#include <hndd11.h>

/* channel bandwidth */
#define WLC_10_MHZ	10	/**< 10Mhz channel bandwidth */
#define WLC_20_MHZ	20	/**< 20Mhz channel bandwidth */
#define WLC_40_MHZ	40	/**< 40Mhz channel bandwidth */
#define WLC_80_MHZ	80	/**< 80Mhz channel bandwidth */
#define WLC_160_MHZ	160	/**< 160Mhz channel bandwidth */

struct monitor_info {
	ratespec_t	ampdu_rspec;	/* spec value for AMPDU sniffing */
	uint16		ampdu_counter;
	uint16		amsdu_len;
	uint8*		amsdu_pkt;
	int8		headroom;
};

#ifdef BCMDONGLEHOST
/**
 * Rate info per rate: tells for *pre* 802.11n rates whether a given rate is OFDM or not and its
 * phy_rate value. Table index is a rate in [500Kbps] units, from 0 to 54Mbps.
 * Contents of a table element:
 *     d[7] : 1=OFDM rate, 0=DSSS/CCK rate
 *     d[3:0] if DSSS/CCK rate:
 *         index into the 'M_RATE_TABLE_B' table maintained by ucode in shm
 *     d[3:0] if OFDM rate: encode rate per 802.11a-1999 sec 17.3.4.1, with lsb transmitted first.
 *         index into the 'M_RATE_TABLE_A' table maintained by ucode in shm
 */

const uint8 rate_info[WLC_MAXRATE + 1] = {
	/*  0     1     2     3     4     5     6     7     8     9 */
/*   0 */ 0x00, 0x00, 0x0a, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  10 */ 0x00, 0x37, 0x8b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x00,
/*  20 */ 0x00, 0x00, 0x6e, 0x00, 0x8a, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  30 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x00, 0x00,
/*  40 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0x00,
/*  50 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  60 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  70 */ 0x00, 0x00, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  80 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  90 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00,
/* 100 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8c
};

/* Decode OFDM PLCP SIGNAL field RATE sub-field bits 0:2 (labeled R1-R3) into
 * 802.11 MAC rate in 500kbps units
 *
 * Table from 802.11-2012, sec 18.3.4.2.
 */
const uint8 ofdm_rate_lookup[] = {
    DOT11_RATE_48M, /* 8: 48Mbps */
    DOT11_RATE_24M, /* 9: 24Mbps */
    DOT11_RATE_12M, /* A: 12Mbps */
    DOT11_RATE_6M,  /* B:  6Mbps */
    DOT11_RATE_54M, /* C: 54Mbps */
    DOT11_RATE_36M, /* D: 36Mbps */
    DOT11_RATE_18M, /* E: 18Mbps */
    DOT11_RATE_9M   /* F:  9Mbps */
};

struct ieee_80211_mcs_rate_info {
	uint8 constellation_bits;
	uint8 coding_q;
	uint8 coding_d;
};

static const struct ieee_80211_mcs_rate_info wlc_mcs_info[] = {
	{ 1, 1, 2 }, /* MCS  0: MOD: BPSK,   CR 1/2 */
	{ 2, 1, 2 }, /* MCS  1: MOD: QPSK,   CR 1/2 */
	{ 2, 3, 4 }, /* MCS  2: MOD: QPSK,   CR 3/4 */
	{ 4, 1, 2 }, /* MCS  3: MOD: 16QAM,  CR 1/2 */
	{ 4, 3, 4 }, /* MCS  4: MOD: 16QAM,  CR 3/4 */
	{ 6, 2, 3 }, /* MCS  5: MOD: 64QAM,  CR 2/3 */
	{ 6, 3, 4 }, /* MCS  6: MOD: 64QAM,  CR 3/4 */
	{ 6, 5, 6 }, /* MCS  7: MOD: 64QAM,  CR 5/6 */
	{ 8, 3, 4 }, /* MCS  8: MOD: 256QAM, CR 3/4 */
	{ 8, 5, 6 }, /* MCS  9: MOD: 256QAM, CR 5/6 */
	{ 10, 3, 4 }, /* MCS 10: MOD: 1024QAM, CR 3/4 */
	{ 10, 5, 6 }  /* MCS 11: MOD: 1024QAM, CR 5/6 */
};

const uint8 *
BCMRAMFN(wlc_phy_get_ofdm_rate_lookup)(void)
{
	return ofdm_rate_lookup;
}

uint8
wlc_vht_get_rate_from_plcp(uint8 *plcp)
{
	uint8 rate;
	uint nss;
	uint32 plcp0 = plcp[0] + (plcp[1] << 8); /* don't need plcp[2] */
	rate = (plcp[3] >> VHT_SIGA2_MCS_SHIFT);
	nss = ((plcp0 & VHT_SIGA1_NSTS_SHIFT_MASK_USER0) >>
		VHT_SIGA1_NSTS_SHIFT) + 1;
	if (plcp0 & VHT_SIGA1_STBC)
		nss = nss >> 1;
	rate |= ((nss << RSPEC_VHT_NSS_SHIFT) | 0x80);
	return rate;
}

ratespec_t
wlc_vht_get_rspec_from_plcp(uint8 *plcp)
{
	uint8 rate;
	uint vht_sig_a1, vht_sig_a2;
	ratespec_t rspec;

	ASSERT(plcp);

	rate = (wlc_vht_get_rate_from_plcp(plcp) & 0x7f);

	vht_sig_a1 = plcp[0] | (plcp[1] << 8);
	vht_sig_a2 = plcp[3] | (plcp[4] << 8);

	rspec = VHT_RSPEC((rate & RSPEC_VHT_MCS_MASK),
		(rate >> RSPEC_VHT_NSS_SHIFT));
#if ((((VHT_SIGA1_20MHZ_VAL + 1) << RSPEC_BW_SHIFT)  != RSPEC_BW_20MHZ) || \
	(((VHT_SIGA1_40MHZ_VAL + 1) << RSPEC_BW_SHIFT)  != RSPEC_BW_40MHZ) || \
	(((VHT_SIGA1_80MHZ_VAL + 1) << RSPEC_BW_SHIFT)  != RSPEC_BW_80MHZ) || \
	(((VHT_SIGA1_160MHZ_VAL + 1) << RSPEC_BW_SHIFT)  != RSPEC_BW_160MHZ))
#error "VHT SIGA BW mapping to RSPEC BW needs correction"
#endif
	rspec |= ((vht_sig_a1 & VHT_SIGA1_160MHZ_VAL) + 1) << RSPEC_BW_SHIFT;
	if (vht_sig_a1 & VHT_SIGA1_STBC)
		rspec |= RSPEC_STBC;
	if (vht_sig_a2 & VHT_SIGA2_GI_SHORT)
		rspec |= RSPEC_SHORT_GI;
	if (vht_sig_a2 & VHT_SIGA2_CODING_LDPC)
		rspec |= RSPEC_LDPC_CODING;

	return rspec;
}

/**
 * Returns the rate in [Kbps] units for a caller supplied MCS/bandwidth/Nss/Sgi combination.
 *     'mcs' : a *single* spatial stream MCS (11n or 11ac)
 */
uint
wlc_rate_mcs2rate(uint mcs, uint nss, uint bw, int sgi)
{
	const int ksps = 250; /* kilo symbols per sec, 4 us sym */
	const int Nsd_20MHz = 52;
	const int Nsd_40MHz = 108;
	const int Nsd_80MHz = 234;
	const int Nsd_160MHz = 468;
	uint rate;

	if (mcs == 32) {
		/* just return fixed values for mcs32 instead of trying to parametrize */
		rate = (sgi == 0) ? 6000 : 6778;
	} else if (mcs <= WLC_MAX_VHT_MCS || IS_PROPRIETARY_11N_SS_MCS(mcs)) {
		/* This calculation works for 11n HT and 11ac VHT if the HT mcs values
		 * are decomposed into a base MCS = MCS % 8, and Nss = 1 + MCS / 8.
		 * That is, HT MCS 23 is a base MCS = 7, Nss = 3
		 */

#if defined(WLPROPRIETARY_11N_RATES)
			switch (mcs) {
				case 87:
					mcs = 8; /* MCS  8: MOD: 256QAM, CR 3/4 */
					break;
				case 88:
					mcs = 9; /* MCS  9: MOD: 256QAM, CR 5/6 */
					break;
			default:
				break;
		}
#endif /* WLPROPRIETARY_11N_RATES */

		/* find the number of complex numbers per symbol */
		if (RSPEC_IS20MHZ(bw)) {
			rate = Nsd_20MHz;
		} else if (RSPEC_IS40MHZ(bw)) {
			rate = Nsd_40MHz;
		} else if (bw == RSPEC_BW_80MHZ) {
			rate = Nsd_80MHz;
		} else if (bw == RSPEC_BW_160MHZ) {
			rate = Nsd_160MHz;
		} else {
			rate = 0;
		}

		/* multiply by bits per number from the constellation in use */
		rate = rate * wlc_mcs_info[mcs].constellation_bits;

		/* adjust for the number of spatial streams */
		rate = rate * nss;

		/* adjust for the coding rate given as a quotient and divisor */
		rate = (rate * wlc_mcs_info[mcs].coding_q) / wlc_mcs_info[mcs].coding_d;

		/* multiply by Kilo symbols per sec to get Kbps */
		rate = rate * ksps;

		/* adjust the symbols per sec for SGI
		 * symbol duration is 4 us without SGI, and 3.6 us with SGI,
		 * so ratio is 10 / 9
		 */
		if (sgi) {
			/* add 4 for rounding of division by 9 */
			rate = ((rate * 10) + 4) / 9;
		}
	} else {
		rate = 0;
	}

	return rate;
} /* wlc_rate_mcs2rate */


/** take a well formed ratespec_t arg and return phy rate in [Kbps] units */
int
wlc_rate_rspec2rate(ratespec_t rspec)
{
	int rate = -1;

	if (RSPEC_ISLEGACY(rspec)) {
		rate = 500 * (rspec & RSPEC_RATE_MASK);
	} else if (RSPEC_ISHT(rspec)) {
		uint mcs = (rspec & RSPEC_RATE_MASK);

		ASSERT(mcs <= 32 || IS_PROPRIETARY_11N_MCS(mcs));

		if (mcs == 32) {
			rate = wlc_rate_mcs2rate(mcs, 1, RSPEC_BW_40MHZ, RSPEC_ISSGI(rspec));
		} else {
#if defined(WLPROPRIETARY_11N_RATES)
			uint nss = GET_11N_MCS_NSS(mcs);
			mcs = wlc_rate_get_single_stream_mcs(mcs);
#else /* this ifdef prevents ROM abandons */
			uint nss = 1 + (mcs / 8);
			mcs = mcs % 8;
#endif /* WLPROPRIETARY_11N_RATES */

			rate = wlc_rate_mcs2rate(mcs, nss, RSPEC_BW(rspec), RSPEC_ISSGI(rspec));
		}
	} else if (RSPEC_ISVHT(rspec)) {
		uint mcs = (rspec & RSPEC_VHT_MCS_MASK);
		uint nss = (rspec & RSPEC_VHT_NSS_MASK) >> RSPEC_VHT_NSS_SHIFT;

		ASSERT(mcs <= WLC_MAX_VHT_MCS);
		ASSERT(nss <= 8);

		rate = wlc_rate_mcs2rate(mcs, nss, RSPEC_BW(rspec), RSPEC_ISSGI(rspec));
	} else {
		ASSERT(0);
	}

	return (rate == 0) ? -1 : rate;
}

#endif /* BCMDONGLEHOST */

/** Calculate the rate of a received frame and return it as a ratespec (monitor mode) */
static ratespec_t BCMFASTPATH
wlc_recv_mon_compute_rspec(wlc_d11rxhdr_t *wrxh, uint8 *plcp)
{
	d11rxhdr_t *rxh = &wrxh->rxhdr;
	ratespec_t rspec;
	uint16 phy_ft;

	phy_ft = rxh->lt80.PhyRxStatus_0 & PRXS0_FT_MASK_REV_LT80;

	switch (phy_ft) {
		case PRXS0_CCK:
			rspec = CCK_RSPEC(CCK_PHY2MAC_RATE(((cck_phy_hdr_t *)plcp)->signal));
			rspec |= RSPEC_BW_20MHZ;
			break;
		case PRXS0_OFDM:
			rspec = OFDM_RSPEC(OFDM_PHY2MAC_RATE(((ofdm_phy_hdr_t *)plcp)->rlpt[0]));
			rspec |= RSPEC_BW_20MHZ;
			break;
		case PRXS0_PREN: {
			uint ht_sig1, ht_sig2;
			uint8 stbc;

			ht_sig1 = plcp[0];	/* only interested in low 8 bits */
			ht_sig2 = plcp[3] | (plcp[4] << 8); /* only interested in low 10 bits */

			rspec = HT_RSPEC((ht_sig1 & HT_SIG1_MCS_MASK));
			if (ht_sig1 & HT_SIG1_CBW) {
				/* indicate rspec is for 40 MHz mode */
				rspec |= RSPEC_BW_40MHZ;
			} else {
				/* indicate rspec is for 20 MHz mode */
				rspec |= RSPEC_BW_20MHZ;
			}
			if (ht_sig2 & HT_SIG2_SHORT_GI)
				rspec |= RSPEC_SHORT_GI;
			if (ht_sig2 & HT_SIG2_FEC_CODING)
				rspec |= RSPEC_LDPC_CODING;
			stbc = ((ht_sig2 & HT_SIG2_STBC_MASK) >> HT_SIG2_STBC_SHIFT);
			if (stbc != 0) {
				rspec |= RSPEC_STBC;
			}
			break;
		}
		case PRXS0_STDN:
			rspec = wlc_vht_get_rspec_from_plcp(plcp);
			break;
		default:
			/* return a valid rspec if not a debug/assert build */
			rspec = OFDM_RSPEC(6) | RSPEC_BW_20MHZ;
			break;
	}

	return rspec;
} /* wlc_recv_compute_rspec */

/* recover 32bit TSF value from the 16bit TSF value */
/* assumption is time in rxh is within 65ms of the current tsf */
/* local TSF inserted in the rxh is at RxStart which is before 802.11 header */
static uint32
wlc_recover_tsf32(uint16 rxh_tsf, uint32 ts_tsf)
{
	uint16 rfdly;

	/* adjust rx dly added in RxTSFTime */

	/* TODO: add PHY type specific value here... */
	rfdly = M_BPHY_PLCPRX_DLY;

	rxh_tsf -= rfdly;

	return (((ts_tsf - rxh_tsf) & 0xFFFF0000) | rxh_tsf);
}

static uint8
wlc_vht_get_gid(uint8 *plcp)
{
	uint32 plcp0 = plcp[0] | (plcp[1] << 8);
	return (plcp0 & VHT_SIGA1_GID_MASK) >> VHT_SIGA1_GID_SHIFT;
}

static uint16
wlc_vht_get_aid(uint8 *plcp)
{
	uint32 plcp0 = plcp[0] | (plcp[1] << 8) | (plcp[2] << 16);
	return (plcp0 & VHT_SIGA1_PARTIAL_AID_MASK) >> VHT_SIGA1_PARTIAL_AID_SHIFT;
}

static bool
wlc_vht_get_txop_ps_not_allowed(uint8 *plcp)
{
	return !!(plcp[2] & (VHT_SIGA1_TXOP_PS_NOT_ALLOWED >> 16));
}

static bool
wlc_vht_get_sgi_nsym_da(uint8 *plcp)
{
	return !!(plcp[3] & VHT_SIGA2_GI_W_MOD10);
}

static bool
wlc_vht_get_ldpc_extra_symbol(uint8 *plcp)
{
	return !!(plcp[3] & VHT_SIGA2_LDPC_EXTRA_OFDM_SYM);
}

static bool
wlc_vht_get_beamformed(uint8 *plcp)
{
	return !!(plcp[4] & (VHT_SIGA2_BEAMFORM_ENABLE >> 8));
}
/* Convert htflags and mcs values to
* rate in units of 500kbps
*/
static uint16
wlc_ht_phy_get_rate(uint8 htflags, uint8 mcs)
{

	ratespec_t rspec = HT_RSPEC(mcs);

	if (htflags & WL_RXS_HTF_40)
		rspec |= RSPEC_BW_40MHZ;

	if (htflags & WL_RXS_HTF_SGI)
		rspec |= RSPEC_SHORT_GI;

	return RSPEC2KBPS(rspec)/500;
}

uint16
wl_rxsts_to_rtap(struct wl_rxsts* rxsts, void *payload, uint16 len, void* pout)
{
	uint16 rtap_len = 0;
	struct dot11_header* mac_header;
	uint8* p = payload;

	ASSERT(p && rxsts);

	if (rxsts->phytype == WL_RXS_PHY_N) {
		if (rxsts->encoding == WL_RXS_ENCODING_HT)
			rtap_len = sizeof(wl_radiotap_ht_t);
		else if (rxsts->encoding == WL_RXS_ENCODING_VHT)
			rtap_len = sizeof(wl_radiotap_vht_t);
		else
			rtap_len = sizeof(wl_radiotap_legacy_t);
	} else {
		rtap_len = sizeof(wl_radiotap_legacy_t);
	}

	if (!(rxsts->nfrmtype & WL_RXS_NFRM_AMSDU_FIRST) &&
			!(rxsts->nfrmtype & WL_RXS_NFRM_AMSDU_SUB)) {
		memcpy((uint8*)pout + rtap_len, (uint8*)p, len);
	}

	mac_header = (struct dot11_header *)(p);

	len += rtap_len;

	if ((rxsts->phytype != WL_RXS_PHY_N) ||
		((rxsts->encoding != WL_RXS_ENCODING_HT) &&
		(rxsts->encoding != WL_RXS_ENCODING_VHT))) {
		wl_radiotap_rx_legacy(mac_header, rxsts,
			(wl_radiotap_legacy_t *)pout);
	}
	else if (rxsts->encoding == WL_RXS_ENCODING_VHT) {
		wl_radiotap_rx_vht(mac_header, rxsts,
			(wl_radiotap_vht_t *)pout);
	}
	else if (rxsts->encoding == WL_RXS_ENCODING_HT) {
		wl_radiotap_rx_ht(mac_header, rxsts,
			(wl_radiotap_ht_t *)pout);
	}

	return len;
}


/* Convert RX hardware status to standard format and send to wl_monitor
 * assume p points to plcp header
 */
static uint16
wl_d11rx_to_rxsts(monitor_info_t* info, monitor_pkt_info_t* pkt_info,
	wlc_d11rxhdr_t *wrxh, void *pkt, uint16 len, void* pout)
{
	struct wl_rxsts sts;
	uint32 rx_tsf_l;
	ratespec_t rspec;
	uint16 chan_num;
	uint16 len_out = 0;
	uint8 *plcp;
	uint8 *p = (uint8*)pkt;
	uint8 hwrxoff = 0;
	uint32 ts_tsf = 0;
	int8 rssi = 0;

	ASSERT(p);
	ASSERT(info);
	BCM_REFERENCE(chan_num);

	rssi = (pkt_info->marker >> 8) & 0xff;
	hwrxoff = (pkt_info->marker >> 16) & 0xff;

	plcp = (uint8*)p + hwrxoff;

	if (wrxh->rxhdr.lt80.RxStatus1 & RXS_PBPRES) {
		plcp += 2;
	}

	ts_tsf = pkt_info->ts.ts_low;
	rx_tsf_l = wlc_recover_tsf32(wrxh->rxhdr.lt80.RxTSFTime, ts_tsf);

	bzero((void *)&sts, sizeof(wl_rxsts_t));

	sts.mactime = rx_tsf_l;
	sts.antenna = (wrxh->rxhdr.lt80.PhyRxStatus_0 & PRXS0_RXANT_UPSUBBAND) ? 1 : 0;
	sts.signal = rssi;
	sts.noise = (int8)pkt_info->marker;
	sts.chanspec = wrxh->rxhdr.lt80.RxChan;

	if (wf_chspec_malformed(sts.chanspec))
		return 0;

	chan_num = CHSPEC_CHANNEL(sts.chanspec);

	rspec = wlc_recv_mon_compute_rspec(wrxh, plcp);
	{
		struct dot11_header *h;
		uint16 subtype;

		h = (struct dot11_header *)(plcp + D11_PHY_HDR_LEN);
		subtype = (ltoh16(h->fc) & FC_SUBTYPE_MASK) >> FC_SUBTYPE_SHIFT;

		if ((subtype == FC_SUBTYPE_QOS_DATA) || (subtype == FC_SUBTYPE_QOS_NULL)) {
			/* A-MPDU parsing */
			if ((wrxh->rxhdr.lt80.PhyRxStatus_0 & PRXS0_FT_MASK) == PRXS0_PREN) {
				if (WLC_IS_MIMO_PLCP_AMPDU(plcp)) {
					sts.nfrmtype |= WL_RXS_NFRM_AMPDU_FIRST;
					/* Save the rspec for later */
					info->ampdu_rspec = rspec;
				} else if (!(plcp[0] | plcp[1] | plcp[2])) {
					sts.nfrmtype |= WL_RXS_NFRM_AMPDU_SUB;
					/* Use the saved rspec */
					rspec = info->ampdu_rspec;
				}
			}
			else if ((wrxh->rxhdr.lt80.PhyRxStatus_0 & PRXS0_FT_MASK) == FT_VHT) {
				if ((plcp[0] | plcp[1] | plcp[2]) &&
					!(wrxh->rxhdr.lt80.RxStatus2 & RXS_PHYRXST_VALID)) {
					/* First MPDU:
					 * PLCP header is valid, Phy RxStatus is not valid
					 */
					sts.nfrmtype |= WL_RXS_NFRM_AMPDU_FIRST;
					/* Save the rspec for later */
					info->ampdu_rspec = rspec;
					info->ampdu_counter++;
				} else if (!(plcp[0] | plcp[1] | plcp[2]) &&
					!(wrxh->rxhdr.lt80.RxStatus2 & RXS_PHYRXST_VALID)) {
					/* Sub MPDU:
					 * PLCP header is not valid, Phy RxStatus is not valid
					 */
					sts.nfrmtype |= WL_RXS_NFRM_AMPDU_SUB;
					/* Use the saved rspec */
					rspec = info->ampdu_rspec;
				} else if ((plcp[0] | plcp[1] | plcp[2]) &&
					(wrxh->rxhdr.lt80.RxStatus2 & RXS_PHYRXST_VALID)) {
					/* MPDU is not a part of A-MPDU:
					 * PLCP header is valid and Phy RxStatus is valid
					 */
					info->ampdu_counter++;
				} else {
					/* Last MPDU */
					rspec = info->ampdu_rspec;
				}

				sts.ampdu_counter = info->ampdu_counter;
			}
		}
		/* A-MSDU parsing */
		if (wrxh->rxhdr.lt80.RxStatus2 & RXS_AMSDU_MASK) {
			/* it's chained buffer, break it if necessary */
			sts.nfrmtype |= WL_RXS_NFRM_AMSDU_FIRST | WL_RXS_NFRM_AMSDU_SUB;
		}
	}

	if (PRXS5_ACPHY_DYNBWINNONHT(&wrxh->rxhdr))
		sts.vhtflags |= WL_RXS_VHTF_DYN_BW_NONHT;
	else
		sts.vhtflags &= ~WL_RXS_VHTF_DYN_BW_NONHT;

	switch (PRXS5_ACPHY_CHBWINNONHT(&wrxh->rxhdr)) {
	default: case PRXS5_ACPHY_CHBWINNONHT_20MHZ:
		sts.bw_nonht = WLC_20_MHZ;
		break;
	case PRXS5_ACPHY_CHBWINNONHT_40MHZ:
		sts.bw_nonht = WLC_40_MHZ;
		break;
	case PRXS5_ACPHY_CHBWINNONHT_80MHZ:
		sts.bw_nonht = WLC_80_MHZ;
		break;
	case PRXS5_ACPHY_CHBWINNONHT_160MHZ:
		sts.bw_nonht = WLC_160_MHZ;
		break;
	}

	/* prepare rate/modulation info */
	if (RSPEC_ISVHT(rspec)) {
		uint32 bw = RSPEC_BW(rspec);
		/* prepare VHT rate/modulation info */
		sts.nss = (rspec & RSPEC_VHT_NSS_MASK) >> RSPEC_VHT_NSS_SHIFT;
		sts.mcs = (rspec & RSPEC_VHT_MCS_MASK);

		if (CHSPEC_IS80(sts.chanspec)) {
			if (bw == RSPEC_BW_20MHZ) {
				switch (CHSPEC_CTL_SB(sts.chanspec)) {
					default:
					case WL_CHANSPEC_CTL_SB_LL:
						sts.bw = WL_RXS_VHT_BW_20LL;
						break;
					case WL_CHANSPEC_CTL_SB_LU:
						sts.bw = WL_RXS_VHT_BW_20LU;
						break;
					case WL_CHANSPEC_CTL_SB_UL:
						sts.bw = WL_RXS_VHT_BW_20UL;
						break;
					case WL_CHANSPEC_CTL_SB_UU:
						sts.bw = WL_RXS_VHT_BW_20UU;
						break;
				}
			} else if (bw == RSPEC_BW_40MHZ) {
				switch (CHSPEC_CTL_SB(sts.chanspec)) {
					default:
					case WL_CHANSPEC_CTL_SB_L:
						sts.bw = WL_RXS_VHT_BW_40L;
						break;
					case WL_CHANSPEC_CTL_SB_U:
						sts.bw = WL_RXS_VHT_BW_40U;
						break;
				}
			} else {
				sts.bw = WL_RXS_VHT_BW_80;
			}
		} else if (CHSPEC_IS40(sts.chanspec)) {
			if (bw == RSPEC_BW_20MHZ) {
				switch (CHSPEC_CTL_SB(sts.chanspec)) {
					default:
					case WL_CHANSPEC_CTL_SB_L:
						sts.bw = WL_RXS_VHT_BW_20L;
						break;
					case WL_CHANSPEC_CTL_SB_U:
						sts.bw = WL_RXS_VHT_BW_20U;
						break;
				}
			} else if (bw == RSPEC_BW_40MHZ) {
				sts.bw = WL_RXS_VHT_BW_40;
			}
		} else {
			sts.bw = WL_RXS_VHT_BW_20;
		}

		if (RSPEC_ISSTBC(rspec))
			sts.vhtflags |= WL_RXS_VHTF_STBC;
		if (wlc_vht_get_txop_ps_not_allowed(plcp))
			sts.vhtflags |= WL_RXS_VHTF_TXOP_PS;
		if (RSPEC_ISSGI(rspec)) {
			sts.vhtflags |= WL_RXS_VHTF_SGI;
			if (wlc_vht_get_sgi_nsym_da(plcp))
				sts.vhtflags |= WL_RXS_VHTF_SGI_NSYM_DA;
		}
		if (RSPEC_ISLDPC(rspec)) {
			sts.coding = WL_RXS_VHTF_CODING_LDCP;
			if (wlc_vht_get_ldpc_extra_symbol(plcp))
				sts.vhtflags |= WL_RXS_VHTF_LDPC_EXTRA;
		}
		if (wlc_vht_get_beamformed(plcp))
			sts.vhtflags |= WL_RXS_VHTF_BF;

		sts.gid = wlc_vht_get_gid(plcp);
		sts.aid = wlc_vht_get_aid(plcp);
		sts.datarate = RSPEC2KBPS(rspec)/500;
	} else
	if (RSPEC_ISHT(rspec)) {
		/* prepare HT rate/modulation info */
		sts.mcs = (rspec & RSPEC_RATE_MASK);

		if (CHSPEC_IS40(sts.chanspec) || CHSPEC_IS80(sts.chanspec)) {
			uint32 bw = RSPEC_BW(rspec);

			if (bw == RSPEC_BW_20MHZ) {
				if (CHSPEC_CTL_SB(sts.chanspec) == WL_CHANSPEC_CTL_SB_L) {
					sts.htflags = WL_RXS_HTF_20L;
				} else {
					sts.htflags = WL_RXS_HTF_20U;
				}
			} else if (bw == RSPEC_BW_40MHZ) {
				sts.htflags = WL_RXS_HTF_40;
			}
		}

		if (RSPEC_ISSGI(rspec))
			sts.htflags |= WL_RXS_HTF_SGI;
		if (RSPEC_ISLDPC(rspec))
			sts.htflags |= WL_RXS_HTF_LDPC;
		if (RSPEC_ISSTBC(rspec))
			sts.htflags |= (1 << WL_RXS_HTF_STBC_SHIFT);

		sts.datarate = wlc_ht_phy_get_rate(sts.htflags, sts.mcs);
	} else {
		/* round non-HT data rate to nearest 500bkps unit */
		sts.datarate = RSPEC2KBPS(rspec)/500;
	}

	sts.pktlength = wrxh->rxhdr.lt80.RxFrameSize - D11_PHY_HDR_LEN;
	sts.sq = ((wrxh->rxhdr.lt80.PhyRxStatus_1 & PRXS1_SQ_MASK) >> PRXS1_SQ_SHIFT);

	sts.phytype = WL_RXS_PHY_N;

	if (RSPEC_ISCCK(rspec)) {
		sts.encoding = WL_RXS_ENCODING_DSSS_CCK;
		sts.preamble = (wrxh->rxhdr.lt80.PhyRxStatus_0 & PRXS0_SHORTH) ?
			WL_RXS_PREAMBLE_SHORT : WL_RXS_PREAMBLE_LONG;
	} else if (RSPEC_ISOFDM(rspec)) {
		sts.encoding = WL_RXS_ENCODING_OFDM;
		sts.preamble = WL_RXS_PREAMBLE_SHORT;
	}
	else {	/* MCS rate */
		sts.encoding = WL_RXS_ENCODING_HT;
		if (RSPEC_ISVHT(rspec)) {
			sts.encoding = WL_RXS_ENCODING_VHT;
		}
		sts.preamble = (uint32)((D11HT_MMPLCPLen(&wrxh->rxhdr) != 0) ?
			WL_RXS_PREAMBLE_HT_MM : WL_RXS_PREAMBLE_HT_GF);
	}
	/* translate error code */
	if (wrxh->rxhdr.lt80.RxStatus1 & RXS_DECERR)
		sts.pkterror |= WL_RXS_DECRYPT_ERR;
	if (wrxh->rxhdr.lt80.RxStatus1 & RXS_FCSERR)
		sts.pkterror |= WL_RXS_CRC_ERROR;

	if (wrxh->rxhdr.lt80.RxStatus1 & RXS_PBPRES) {
		p += 2; len -= 2;
	}

	p += (hwrxoff + D11_PHY_HDR_LEN);
	len -= (hwrxoff + D11_PHY_HDR_LEN);

	len_out = wl_rxsts_to_rtap(&sts, p, len, pout);
	return len_out;
}

static uint16
wl_monitor_amsdu(monitor_info_t* info, monitor_pkt_info_t* pkt_info, wlc_d11rxhdr_t *wrxh,
	void *pkt, uint16 len, void* pout, uint16* offset)
{
	uint8 *p = pkt;
	uint8 hwrxoff = (pkt_info->marker >> 16) & 0xff;
	uint16 frame_len = 0;
	uint16 aggtype = (wrxh->rxhdr.lt80.RxStatus2 & RXS_AGGTYPE_MASK) >> RXS_AGGTYPE_SHIFT;

	switch (aggtype) {
	case RXS_AMSDU_FIRST:
	case RXS_AMSDU_N_ONE:
		/* Flush any previously collected */
		if (info->amsdu_len) {
			info->amsdu_len = 0;
		}

		info->headroom = MAX_RADIOTAP_SIZE - D11_PHY_HDR_LEN - hwrxoff;
		info->headroom -= (wrxh->rxhdr.lt80.RxStatus1 & RXS_PBPRES) ? 2 : 0;

		/* Save the new starting AMSDU subframe */
		info->amsdu_len = len;
		info->amsdu_pkt = (uint8*)pout + (info->headroom > 0 ?
			info->headroom : 0);

		memcpy(info->amsdu_pkt, p, len);

		if (aggtype == RXS_AMSDU_N_ONE) {
			/* all-in-one AMSDU subframe */
			frame_len = wl_d11rx_to_rxsts(info, pkt_info, wrxh, p,
				len, info->amsdu_pkt - info->headroom);

			*offset = ABS(info->headroom);
			frame_len += *offset;

			info->amsdu_len = 0;
		}
		break;

	case RXS_AMSDU_INTERMEDIATE:
	case RXS_AMSDU_LAST:
	default:
			/* Check for previously collected */
		if (info->amsdu_len) {
			/* Append next AMSDU subframe */
			p += hwrxoff; len -= hwrxoff;

			if (wrxh->rxhdr.lt80.RxStatus1 & RXS_PBPRES) {
				p += 2;	len -= 2;
			}

			memcpy(info->amsdu_pkt + info->amsdu_len, p, len);
			info->amsdu_len += len;

			/* complete AMSDU frame */
			if (aggtype == RXS_AMSDU_LAST) {
				frame_len = wl_d11rx_to_rxsts(info, pkt_info, wrxh, info->amsdu_pkt,
					info->amsdu_len, info->amsdu_pkt - info->headroom);

				*offset = ABS(info->headroom);
				frame_len += *offset;

				info->amsdu_len = 0;
			}
		}
		break;
	}

	return frame_len;
}

uint16 bcmwifi_monitor_create(monitor_info_t** info)
{
	*info = MALLOC(NULL, sizeof(struct monitor_info));

	if (!(*info))
		return FALSE;

	(*info)->amsdu_len = 0;

	return TRUE;
}

void
bcmwifi_monitor_delete(monitor_info_t* info)
{
	if (info)
		MFREE(NULL, info, sizeof(struct monitor_info));
}

uint16
bcmwifi_monitor(monitor_info_t* info, monitor_pkt_info_t* pkt_info,
	void *pkt, uint16 len, void* pout, uint16* offset)
{
	wlc_d11rxhdr_t *wrxh = (wlc_d11rxhdr_t*)pkt;

	if ((wrxh->rxhdr.lt80.RxStatus2 & RXS_AMSDU_MASK)) {
		return wl_monitor_amsdu(info, pkt_info, wrxh, pkt, len, pout, offset);
	} else {
		info->amsdu_len = 0; /* reset amsdu */
		*offset = 0;
		return wl_d11rx_to_rxsts(info, pkt_info, wrxh, pkt, len, pout);
	}
}
