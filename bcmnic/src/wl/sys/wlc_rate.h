/*
 * Common OS-independent driver header for rate management.
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
 * $Id: wlc_rate.h 668765 2016-11-04 21:59:07Z $
 */


#ifndef _WLC_RATE_H_
#define _WLC_RATE_H_

#include <typedefs.h>
#include <hndd11.h>
#include <wlioctl.h>
#include <wlc_types.h>
#include <bcmutils.h>

#include <wlc_rspec_defs.h>

extern const uint8 rate_info[];
extern const struct wlc_rateset cck_ofdm_mimo_rates;
extern const struct wlc_rateset ofdm_mimo_rates;
extern const struct wlc_rateset cck_ofdm_40bw_mimo_rates;
extern const struct wlc_rateset ofdm_40bw_mimo_rates;
extern const struct wlc_rateset cck_ofdm_rates;
extern const struct wlc_rateset ofdm_rates;
extern const struct wlc_rateset cck_rates;
extern const struct wlc_rateset gphy_legacy_rates;
extern const struct wlc_rateset wlc_lrs_rates;
extern const struct wlc_rateset rate_limit_1_2;

typedef struct mcs_info {
	uint32 phy_rate_20;	/**< phy rate in kbps [20Mhz] */
	uint32 phy_rate_40;	/**< phy rate in kbps [40Mhz] */
	uint32 phy_rate_20_sgi;	/**< phy rate in kbps [20Mhz] with SGI */
	uint32 phy_rate_40_sgi;	/**< phy rate in kbps [40Mhz] with SGI */
	uint8  tx_phy_ctl3;	/**< phy ctl byte 3, code rate, modulation type, # of streams */
	uint8  leg_ofdm;	/**< matching legacy ofdm rate in 500bkps */
} mcs_info_t;

#if defined(WLPROPRIETARY_11N_RATES) /* Broadcom proprietary rate support for 11n */

#define WLC_MAXMCS	102	/**< max valid mcs index */

#define IS_PROPRIETARY_11N_MCS(mcs) \
	((mcs) == 87 || (mcs) == 88 || (mcs) == 99 || (mcs) == 100 || (mcs) == 101 || (mcs) == 102)

#define IS_PROPRIETARY_11N_SS_MCS(mcs) \
	((mcs) == 87 || (mcs) == 88)

#else /* WLPROPRIETARY_11N_RATES */

#define WLC_MAXMCS	32	/**< max valid mcs index */

#define IS_PROPRIETARY_11N_MCS(mcs)	FALSE
#define IS_PROPRIETARY_11N_SS_MCS(mcs)	FALSE /**< is proprietary HT single stream MCS */

#endif /* WLPROPRIETARY_11N_RATES */

#define N_11N_MCS_VALUES	33	/**< Number of standardized MCS values in 802.11n (HT) */
#define MCS_TABLE_SIZE		N_11N_MCS_VALUES

#define PROP11N_2_VHT_MCS(rspec) (((rspec) & 0x1) ? 8 : 9) /* odd propr HT rates use 3/4 coding */

extern const mcs_info_t mcs_table[];

#define MCS_INVALID	0xFF
#define MCS_CR_MASK	0x07	/**< Code Rate bit mask */
#define MCS_MOD_MASK	0x38	/**< Modulation bit shift */
#define MCS_MOD_SHIFT	3	/**< MOdulation bit shift */
#define MCS_TXS_MASK   	0xc0	/**< num tx streams - 2 bit mask */
#define MCS_TXS_SHIFT	6	/**< num tx streams - 2 bit shift */
#define MCS_CR(_mcs)	(mcs_table[_mcs].tx_phy_ctl3 & MCS_CR_MASK)
#define MCS_MOD(_mcs)	((mcs_table[_mcs].tx_phy_ctl3 & MCS_MOD_MASK) >> MCS_MOD_SHIFT)
#define MCS_TXS(_mcs)	((mcs_table[_mcs].tx_phy_ctl3 & MCS_TXS_MASK) >> MCS_TXS_SHIFT)
#define MCS_RATE(_mcs, _is40, _sgi)	(_sgi ? \
	(_is40 ? mcs_table[_mcs].phy_rate_40_sgi : mcs_table[_mcs].phy_rate_20_sgi) : \
	(_is40 ? mcs_table[_mcs].phy_rate_40 : mcs_table[_mcs].phy_rate_20))

#if defined(WLPROPRIETARY_11N_RATES) /* Broadcom proprietary rate support for 11n */
#define VALID_MCS(_mcs)	(((_mcs) < N_11N_MCS_VALUES) || IS_PROPRIETARY_11N_MCS(_mcs))
#else
#define VALID_MCS(_mcs)	(((_mcs) < N_11N_MCS_VALUES))
#endif

#define	WLC_RATE_FLAG	0x80	/**< Rate flag: basic or ofdm */

/* Macros to use the rate_info table */
#define	RATE_MASK	0x7f		/* Rate value mask w/o basic rate flag */
#define	RATE_MASK_FULL	0xff		/* Rate value mask with basic rate flag */

/* DSSS, CCK and 802.11n rates in [500kbps] units */
#define WLC_MAXRATE	108	/**< in 500kbps units */


#define WLC_RATE_500K_TO_BPS(rate)	((rate) * 500000)	/**< convert 500kbps to bps */

#define BPS_TO_500K(rate)		((rate)/500)		/**< Convert in uinits of 500kbps */
#define MCS_TO_RSPEC(mcs, bw, plcp)	((HT_RSPEC(mcs)) | ((bw) << RSPEC_BW_SHIFT)| \
		((PLCP3_ISSGI(plcp)) ? RSPEC_SHORT_GI : 0))
#define WLC_STD_MAX_VHT_MCS	9	/**< 11ac std VHT MCS 0-9 */
#define WLC_MAX_VHT_MCS	11	/**< Std VHT MCS 0-9 plus prop VHT MCS 10-11 */
#define WLC_MAX_HE_MCS	11	/**< Std HE MCS 0-11 */

#define IS_PROPRIETARY_VHT_MCS_10_11(mcs) ((mcs) == 10 || (mcs) == 11)

/* mcsmap with MCS0-9 for Nss = 4 */
#define VHT_CAP_MCS_MAP_0_9_NSS4 \
	        ((VHT_CAP_MCS_MAP_0_9 << VHT_MCS_MAP_GET_SS_IDX(1)) | \
	         (VHT_CAP_MCS_MAP_0_9 << VHT_MCS_MAP_GET_SS_IDX(2)) | \
	         (VHT_CAP_MCS_MAP_0_9 << VHT_MCS_MAP_GET_SS_IDX(3)) | \
	         (VHT_CAP_MCS_MAP_0_9 << VHT_MCS_MAP_GET_SS_IDX(4)))

#define VHT_PROP_MCS_MAP_10_11_NSS4 \
	        ((VHT_PROP_MCS_MAP_10_11 << VHT_MCS_MAP_GET_SS_IDX(1)) | \
	         (VHT_PROP_MCS_MAP_10_11 << VHT_MCS_MAP_GET_SS_IDX(2)) | \
	         (VHT_PROP_MCS_MAP_10_11 << VHT_MCS_MAP_GET_SS_IDX(3)) | \
	         (VHT_PROP_MCS_MAP_10_11 << VHT_MCS_MAP_GET_SS_IDX(4)))

/* max no. of vht rates supported (0-9 and prop 10-11) */
#define MAX_VHT_RATES		12

#define NTXRATE		64	/**< # tx MPDUs rate is reported for */
#define NVITXRATE	32	/**< # Video tx MPDUs rate is reported for */

/* bw define used by rate modules */
#define BW_20MHZ                (RSPEC_BW_20MHZ >> RSPEC_BW_SHIFT)
#define BW_40MHZ                (RSPEC_BW_40MHZ >> RSPEC_BW_SHIFT)
#define BW_80MHZ                (RSPEC_BW_80MHZ >> RSPEC_BW_SHIFT)
#define BW_160MHZ               (RSPEC_BW_160MHZ >> RSPEC_BW_SHIFT)

#define BW_MAXMHZ	BW_160MHZ
#define BW_MINMHZ	BW_20MHZ

#define RSPEC2BW(rspec)		((RSPEC_BW_MASK & (rspec)) >> RSPEC_BW_SHIFT)

#define WLC_HTPHY		127		/**< HT PHY Membership */

#define RSPEC_ACTIVE(rspec) \
(((rspec) & (RSPEC_OVERRIDE_RATE | RSPEC_OVERRIDE_MODE | RSPEC_RATE_MASK)) ||\
RSPEC_ISHT(rspec))

#ifdef WL11N
#define IS_MCS(rspec)     	(((rspec) & RSPEC_ENCODING_MASK) != RSPEC_ENCODE_RATE)
#define IS_STBC(rspec)     	(((((rspec) & RSPEC_ENCODING_MASK) == RSPEC_ENCODE_HT) ||	\
	(((rspec) & RSPEC_ENCODING_MASK) == RSPEC_ENCODE_VHT)) &&	\
	(((rspec) & RSPEC_STBC) == RSPEC_STBC))
#define RSPEC2RATE(rspec)	(RSPEC_ISLEGACY(rspec) ? \
				 ((rspec) & RSPEC_RATE_MASK) : wlc_rate_rspec2rate(rspec))
/* return rate in unit of 500Kbps -- for internal use in wlc_rate_sel.c */
#define RSPEC2KBPS(rspec)	wlc_rate_rspec2rate(rspec)
#else /* WL11N */
#define IS_MCS(rspec)     	0
#define IS_STBC(rspec)		0
#define RSPEC2RATE(rspec)      	((rspec) & RSPEC_RATE_MASK)
#define RSPEC2KBPS(rspec)	(((rspec) & RSPEC_RATE_MASK) * 500)
#endif /* WL11N */

#define PLCP3_ISLDPC(plcp)	(((plcp) & PLCP3_LDPC) != 0)
#define PLCP3_ISSGI(plcp)	(((plcp) & PLCP3_SGI) != 0)
#define PLCP3_ISSTBC(plcp)	(((plcp) & PLCP3_STC_MASK) != 0)
#define PLCP3_STC_MASK          0x30
#define PLCP3_STC_SHIFT         4
#define PLCP3_LDPC              0x40
#define PLCP3_SGI               0x80


#define VHT_PLCP3_ISSGI(plcp)	(((plcp) & VHT_SIGA2_GI_SHORT) != 0)
#define VHT_PLCP0_ISSTBC(plcp)	(((plcp) & VHT_SIGA1_STBC) != 0)
#define VHT_PLCP0_BW(plcp)	((plcp) & VHT_SIGA1_BW_MASK)

/* Macros to check for OFDM or DSSS/CCK ratespec.
 *
 * Note, the parameter should not be an 802.11 rate (1 byte with
 * high bit for basic rate designation).
 * When you have an 802.11 rate byte, use the RATE_ISOFDM() and RATE_ISCCK() macros.
 */
#define	RSPEC_ISOFDM(r) (RSPEC_ISLEGACY(r) && (rate_info[(r) & RSPEC_RATE_MASK] & WLC_RATE_FLAG))
#define	RSPEC_ISCCK(r)	(RSPEC_ISLEGACY(r) &&			\
			 (((r) & RATE_MASK) == WLC_RATE_1M ||	\
			  ((r) & RATE_MASK) == WLC_RATE_2M ||	\
			  ((r) & RATE_MASK) == WLC_RATE_5M5 ||	\
			  ((r) & RATE_MASK) == WLC_RATE_11M))

/* Legacy rate info; takes 802.11 rate byte in 500Kbps units, ignoring the Basic bit 0x80 */
#define	RATE_ISOFDM(r)  ((rate_info[(r) & RATE_MASK] & WLC_RATE_FLAG) != 0)
#define	RATE_ISCCK(r)	(((r) & RATE_MASK) == WLC_RATE_1M ||	\
			 ((r) & RATE_MASK) == WLC_RATE_2M ||	\
			 ((r) & RATE_MASK) == WLC_RATE_5M5 ||	\
			 ((r) & RATE_MASK) == WLC_RATE_11M)


#if defined(WLPROPRIETARY_11N_RATES) /* Broadcom proprietary rate support for 11n */
#ifdef OEM_ANDROID
#define GET_PROPRIETARY_11N_MCS_NSS(mcs) (1 + ((mcs) - 85) / 8)

#define GET_11N_MCS_NSS(mcs) ((mcs) < 32 ? (1 + ((mcs) / 8)) \
				: ((mcs) == 32 ? 1 : GET_PROPRIETARY_11N_MCS_NSS(mcs)))
#endif /* OEM_ANDROID */
#define IS_SINGLE_STREAM(mcs)	(GET_11N_MCS_NSS(mcs) == 1)
#else
#define IS_SINGLE_STREAM(mcs)	(((mcs) <= HIGHEST_SINGLE_STREAM_MCS) || ((mcs) == 32))
#endif /* WLPROPRIETARY_11N_RATES */

/* create ratespecs */
#define CCK_RSPEC(cck)		LEGACY_RSPEC(cck)
#define OFDM_RSPEC(ofdm)	LEGACY_RSPEC(ofdm)

#define HT_RSPEC_BW(mcs, bw)	(HT_RSPEC(mcs) | (((bw) << RSPEC_BW_SHIFT) & RSPEC_BW_MASK))
#define VHT_RSPEC_BW(mcs, nss, bw) (VHT_RSPEC(mcs, nss) | (((bw) << RSPEC_BW_SHIFT)&RSPEC_BW_MASK))

/* Convert encoded rate value in plcp header to numerical rates in 500 KHz increments */
extern const uint8 *wlc_phy_get_ofdm_rate_lookup(void);
#define OFDM_PHY2MAC_RATE(rlpt)         ((wlc_phy_get_ofdm_rate_lookup())[(rlpt) & 0x7])
#define CCK_PHY2MAC_RATE(signal)	((signal)/5)

/* Rates specified in wlc_rateset_filter() */
#define WLC_RATES_CCK_OFDM	0
#define WLC_RATES_CCK		1
#define WLC_RATES_OFDM		2

/* mcsallow flags in wlc_rateset_filter() and wlc_filter_rateset */
#define WLC_MCS_ALLOW				0x1
#define WLC_MCS_ALLOW_VHT			0x2
#define WLC_MCS_ALLOW_PROP_HT		0x4 /**< Broadcom proprietary 11n rates */
#define WLC_MCS_ALLOW_1024QAM			0x8 /**< Proprietary VHT rates */
#define WLC_MCS_ALLOW_HE			0x10

/* per-bw sgi enable */
#define SGI_BW20		1
#define SGI_BW40		2
#define SGI_BW80		4
#define SGI_BW160		8

/* Macro for overriding AUTO mode switch */
#define WLC_RSPEC_OVERRIDE_ANY(wlc)	(wlc->bandstate[BAND_2G_INDEX]->rspec_override || \
	wlc->bandstate[BAND_2G_INDEX]->mrspec_override || \
	wlc->bandstate[BAND_5G_INDEX]->rspec_override || \
	wlc->bandstate[BAND_5G_INDEX]->mrspec_override)

/* ************* HE definitions. ************* */

#define HE_IS_GI_0_8us(gi)	((gi) == RSPEC_HE_GI_0_8us)
#define HE_IS_GI_1_6us(gi)	((gi) == RSPEC_HE_GI_1_6us)
#define HE_IS_GI_3_2us(gi)	((gi) == RSPEC_HE_GI_3_2us)

#define HE_PLCP_B0_DL_UL		(0x00)
#define HE_PLCP_B0_FORMAT_MASK		(0x02)
#define HE_PLCP_B0_FORMAT_SHIFT		(1)
#define HE_PLCP_B0_BW_MASK		(0x0C)
#define HE_PLCP_B0_BW_SHIFT		(2)

#define HE_PLCP_B2_TXBF_MASK		(0x10)
#define HE_PLCP_B2_TXBF_SHIFT		(4)
#define HE_PLCP_B2_CODING_MASK		(0x80)
#define HE_PLCP_B2_CODING_SHIFT		(7)

#define HE_PLCP_B3_STBC_MASK		(0x02)
#define HE_PLCP_B3_STBC_SHIFT		(1)
#define HE_PLCP_B3_CPLTF_MASK		(0x0C)
#define HE_PLCP_B3_CPLTF_SHIFT		(2)
#define HE_PLCP_B3_MCS_MASK		(0xF0)
#define HE_PLCP_B3_MCS_SHIFT		(4)

#define HE_PLCP_B4_DCM_MASK		(0x01)
#define HE_PLCP_B4_DCM_SHIFT		(0)
#define HE_PLCP_B4_NSTS_MASK		(0x0E)
#define HE_PLCP_B4_NSTS_SHIFT		(1)

/* HE Rates BITMAP */
#define HE_CAP_MCS_0_7_MAP		0x00ff
#define HE_CAP_MCS_0_8_MAP		0x01ff
#define HE_CAP_MCS_0_9_MAP		0x03ff
#define HE_CAP_MCS_0_10_MAP		0x07ff
#define HE_CAP_MCS_0_11_MAP		0x0fff
#define HE_CAP_MCS_FULL_RATEMAP		HE_CAP_MCS_0_11_MAP

/* Map the mcs bitmap to mcs code */
#define HE_MCS_MAP_TO_MCS_CODE(mcs_map) \
	((mcs_map == HE_CAP_MCS_0_7_MAP) ? HE_CAP_MCS_CODE_0_7 : \
	 (mcs_map == HE_CAP_MCS_0_8_MAP) ? HE_CAP_MCS_CODE_0_8 : \
	 (mcs_map == HE_CAP_MCS_0_9_MAP) ? HE_CAP_MCS_CODE_0_9 : \
	 (mcs_map == HE_CAP_MCS_0_10_MAP) ? HE_CAP_MCS_CODE_0_10 : \
	 (mcs_map == HE_CAP_MCS_0_11_MAP) ? HE_CAP_MCS_CODE_0_11 : HE_CAP_MCS_CODE_NONE)

/* Map the mcs code to mcs bitmap */
#define HE_MCS_CODE_TO_MCS_MAP(mcs_map) \
	((mcs_map == HE_CAP_MCS_CODE_0_7) ? HE_CAP_MCS_0_7_MAP : \
	 (mcs_map == HE_CAP_MCS_CODE_0_8) ? HE_CAP_MCS_0_8_MAP : \
	 (mcs_map == HE_CAP_MCS_CODE_0_9) ? HE_CAP_MCS_0_9_MAP : \
	 (mcs_map == HE_CAP_MCS_CODE_0_10) ? HE_CAP_MCS_0_10_MAP : \
	 (mcs_map == HE_CAP_MCS_CODE_0_11) ? HE_CAP_MCS_0_11_MAP : 0)

/* use the stuct form instead of typedef to fix dependency problems */
struct wlc_rateset;

/* ratespec utils */

/* Number of spatial streams, Nss, specified by the ratespec */
extern uint wlc_ratespec_nss(ratespec_t rspec);
/* Modulation without spatial stream info */
extern uint8 wlc_ratespec_mcs(ratespec_t rspec);

/* Number of space time streams, Nss + Nstbc expansion, specified by the ratespec */
extern uint wlc_ratespec_nsts(ratespec_t rspec);

/* Number of Tx chains, NTx, specified by the ratespec */
extern uint wlc_ratespec_ntx(ratespec_t rspec);

/* take a well formed ratespec_t arg and return phy rate in Kbps */
extern int wlc_rate_rspec2rate(ratespec_t rspec);
extern uint wlc_rate_mcs2rate(uint mcs, uint nss, uint bw, int sgi);
extern uint wlc_rate_he_rspec2rate(ratespec_t rspec);

#if defined(WL11N) || defined(WL11AC)
#define RSPEC_REFERENCE_RATE(rspec) wlc_rate_rspec_reference_rate(rspec)
#else
#define RSPEC_REFERENCE_RATE(rspec) ((uint)(rspec) & RSPEC_RATE_MASK)
#endif

/* Calculate the reference rate for response frame basic rate calculation */
extern uint wlc_rate_rspec_reference_rate(ratespec_t rspec);

/* Legacy reference rate for HT/VHT rates */
extern uint wlc_rate_ht_basic_reference(uint ht_mcs);
extern uint wlc_rate_vht_basic_reference(uint vht_mcs);

/* sanitize, and sort a rateset with the basic bit(s) preserved, validate rateset */
extern bool wlc_rate_hwrs_filter_sort_validate(struct wlc_rateset *rs,
                                               const struct wlc_rateset *hw_rs,
                                               bool check_brate, uint8 txstreams);
/* copy rateset src to dst as-is (no masking or sorting) */
extern void wlc_rateset_copy(const struct wlc_rateset *src, struct wlc_rateset *dst);

uint8 wlc_vht_get_rate_from_plcp(uint8 *plcp);
extern ratespec_t wlc_vht_get_rspec_from_plcp(uint8 *plcp);
/* would be nice to have these documented ... */
extern ratespec_t wlc_recv_compute_rspec(d11_info_t *d11_info, wlc_d11rxhdr_t *wrxh,
	uint8 *plcp);
extern void wlc_rateset_filter(struct wlc_rateset *src, struct wlc_rateset *dst,
	bool basic_only, uint8 rates, uint xmask, uint8 mcsallow);
extern void wlc_rateset_ofdm_fixup(struct wlc_rateset *rs);
extern void wlc_rateset_default(struct wlc_rateset *rs_tgt, const struct wlc_rateset *rs_hw,
	uint phy_type, int bandtype, bool cck_only, uint rate_mask,
	uint8 mcsallow, uint8 bw, uint8 txstreams);
extern int16 wlc_rate_legacy_phyctl(uint rate);

extern int wlc_dump_rateset(const char *name, struct wlc_rateset *rateset, struct bcmstrbuf *b);
extern void wlc_dump_rspec(void *context, ratespec_t rspec, struct bcmstrbuf *b);

extern int wlc_dump_mcsset(const char *name, uchar *mcs, struct bcmstrbuf *b);
extern void wlc_dump_vht_mcsmap(const char *name, uint16 mcsmap, struct bcmstrbuf *b);
extern int wlc_dump_vht_mcsmap_prop(const char *name, uint16 mcsmap, struct bcmstrbuf *b);

extern ratespec_t wlc_get_highest_rate(struct wlc_rateset *rateset, uint8 bw, bool sgi,
	bool vht_ldpc, uint8 vht_ratemask, uint8 txstreams);

#ifdef WL11N
extern void wlc_rateset_mcs_upd(struct wlc_rateset *rs, uint8 txstreams);
extern void wlc_rateset_mcs_clear(struct wlc_rateset *rateset);
extern void wlc_rateset_mcs_build(struct wlc_rateset *rateset, uint8 txstreams);
extern void wlc_rateset_bw_mcs_filter(struct wlc_rateset *rateset, uint8 bw);
extern uint8 wlc_rate_ht2vhtmcs(uint8 mcs);
extern uint16 wlc_get_valid_vht_mcsmap(uint8 mcscode, uint8 prop_mcscode, uint8 bw, bool ldpc,
	uint8 nss, uint8 ratemask);
#ifdef WL11AC
extern void wlc_rateset_vhtmcs_upd(struct wlc_rateset *rs, uint8 nstreams);
extern void wlc_rateset_vhtmcs_build(struct wlc_rateset *rateset, uint8 txstreams);
extern void wlc_rateset_prop_vhtmcs_upd(wlc_rateset_t *rs, uint8 nstreams);
extern void wlc_rateset_prop_vhtmcs_build(struct wlc_rateset *rateset, uint8 txstreams);
extern uint16 wlc_rateset_filter_mcsmap(uint16 vht_mcsmap, uint8 nstreams, uint8 mcsallow);
#endif /* WL11AC */
#else
#define wlc_rateset_mcs_upd(a, b)
#endif /* WL11N */

extern bool wlc_bss_membership_filter(struct wlc_rateset *rs);
extern void wlc_rateset_merge(struct wlc_rateset *rs1, struct wlc_rateset *rs2);
extern uint wlc_rate_mcs2rate(uint mcs, uint nss, uint bw, int sgi);
extern uint8 wlc_rate_get_single_stream_mcs(uint mcs);
extern void wlc_rate_clear_prop_11n_mcses(uint8 mcs_bitmap[]);

extern ratespec_t wlc_he_get_rspec_from_plcp(uint8 *plcp);

#endif	/* _WLC_RATE_H_ */
