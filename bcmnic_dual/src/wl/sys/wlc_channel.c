/*
 * Common interface to channel definitions.
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_channel.c 571308 2015-07-14 22:07:08Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <proto/802.11.h>
#include <proto/wpa.h>
#include <sbconfig.h>
#include <pcicfg.h>
#include <bcmsrom.h>
#include <wlioctl.h>
#include <epivers.h>
#ifdef BCMSUP_PSK
#include <proto/eapol.h>
#include <bcmwpa.h>
#endif /* BCMSUP_PSK */
#include <bcmdevs.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_key.h>
#include <wlc.h>
#include <wlc_bmac.h>
#include <wlc_phy_hal.h>
#include <wl_export.h>
#include <wlc_stf.h>
#include <wlc_channel.h>
#include "wlc_clm_data.h"
#include <wlc_11h.h>
#include <wlc_tpc.h>
#include <wlc_dfs.h>
#include <wlc_11d.h>
#include <wlc_cntry.h>
#include <wlc_prot_g.h>
#include <wlc_prot_n.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_ie_reg.h>
#ifdef WLOFFLD
#include <wlc_offloads.h>
#endif /* WLOFFLD */
#ifdef WLOLPC
#include <wlc_olpc_engine.h>
#endif /* WLOLPC */


typedef struct wlc_cm_band {
	uint8		locale_flags;		/* locale_info_t flags */
	chanvec_t	valid_channels;		/* List of valid channels in the country */
	chanvec_t	*radar_channels;	/* List of radar sensitive channels */
	struct wlc_channel_txchain_limits chain_limits;	/* per chain power limit */
	uint8		PAD[4];
} wlc_cm_band_t;

struct wlc_cm_info {
	wlc_pub_t	*pub;
	wlc_info_t	*wlc;
	char		srom_ccode[WLC_CNTRY_BUF_SZ];	/* Country Code in SROM */
	uint		srom_regrev;			/* Regulatory Rev for the SROM ccode */
	clm_country_t country;			/* Current country iterator for the CLM data */
	char		ccode[WLC_CNTRY_BUF_SZ];	/* current internal Country Code */
	uint		regrev;				/* current Regulatory Revision */
	char		country_abbrev[WLC_CNTRY_BUF_SZ];	/* current advertised ccode */
	wlc_cm_band_t	bandstate[MAXBANDS];	/* per-band state (one per phy/radio) */
	/* quiet channels currently for radar sensitivity or 11h support */
	chanvec_t	quiet_channels;		/* channels on which we cannot transmit */

	struct clm_data_header* clm_base_dataptr;
	int clm_base_data_len;

	void* clm_inc_dataptr;
	void* clm_inc_headerptr;
	int clm_inc_data_len;

	/* List of radar sensitive channels for the current locale */
	chanvec_t locale_radar_channels;

	/* restricted channels */
	chanvec_t	restricted_channels; 	/* copy of the global restricted channels of the */
						/* current local */
	bool		has_restricted_ch;

	/* regulatory class */
	rcvec_t		valid_rcvec;	/* List of valid regulatory class in the country */
	const rcinfo_t	*rcinfo_list[MAXRCLIST];	/* regulatory class info list */
	bool		bandedge_filter_apply;
	bool		sar_enable;		/* Use SAR as part of regulatory power calc */
#ifdef WL_SARLIMIT
	sar_limit_t	sarlimit;		/* sar limit per band/sub-band */
#endif
	/* List of valid regulatory class in the country */
	chanvec_t	allowed_5g_20channels;	/* List of valid 20MHz channels in the country */
	chanvec_t	allowed_5g_4080channels;	/* List of valid 40 and 80MHz channels */

};

static int wlc_channels_init(wlc_cm_info_t *wlc_cm, clm_country_t country);
static void wlc_set_country_common(
	wlc_cm_info_t *wlc_cm, const char* country_abbrev, const char* ccode, uint regrev,
	clm_country_t country);
static int wlc_country_aggregate_map(
	wlc_cm_info_t *wlc_cm, const char *ccode, char *mapped_ccode, uint *mapped_regrev);
static clm_result_t wlc_countrycode_map(wlc_cm_info_t *wlc_cm, const char *ccode,
	char *mapped_ccode, uint *mapped_regrev, clm_country_t *country);
static void wlc_channels_commit(wlc_cm_info_t *wlc_cm);
static void wlc_chanspec_list(wlc_info_t *wlc, wl_uint32_list_t *list, chanspec_t chanspec_mask);
static bool wlc_buffalo_map_locale(struct wlc_info *wlc, const char* abbrev);
static bool wlc_japan_ccode(const char *ccode);
static bool wlc_us_ccode(const char *ccode);
static void wlc_rcinfo_init(wlc_cm_info_t *wlc_cm);
static void wlc_regclass_vec_init(wlc_cm_info_t *wlc_cm);
static void wlc_upd_restricted_chanspec_flag(wlc_cm_info_t *wlc_cm);
static int wlc_channel_update_txchain_offsets(wlc_cm_info_t *wlc_cm, ppr_t *txpwr);

/* IE mgmt callbacks */
#ifdef WLTDLS
static uint wlc_channel_tdls_calc_rc_ie_len(void *ctx, wlc_iem_calc_data_t *calc);
static int wlc_channel_tdls_write_rc_ie(void *ctx, wlc_iem_build_data_t *build);
#endif

static void wlc_channel_spurwar_locale(wlc_cm_info_t *wlc_cm, chanspec_t chanspec);


#define	COPY_LIMITS(src, index, dst, cnt)	\
		bcopy(&src.limit[index], txpwr->dst, cnt)
#define	COPY_DSSS_LIMS(src, index, dst)	\
		bcopy(&src.limit[index], txpwr->dst, WL_NUM_RATES_CCK)
#define	COPY_OFDM_LIMS(src, index, dst)	\
		bcopy(&src.limit[index], txpwr->dst, WL_NUM_RATES_OFDM)
#define	COPY_MCS_LIMS(src, index, dst)	\
		bcopy(&src.limit[index], txpwr->dst, WL_NUM_RATES_MCS_1STREAM)
#ifdef WL11AC
#define	COPY_VHT_LIMS(src, index, dst)	\
		bcopy(&src.limit[index], txpwr->dst, WL_NUM_RATES_EXTRA_VHT)
#else
#define	COPY_VHT_LIMS(src, index, dst)
#endif

#define CLM_DSSS_RATESET(src) ((const ppr_dsss_rateset_t*)&src.limit[WL_RATE_1X1_DSSS_1])
#define CLM_OFDM_1X1_RATESET(src) ((const ppr_ofdm_rateset_t*)&src.limit[WL_RATE_1X1_OFDM_6])
#define CLM_MCS_1X1_RATESET(src) ((const ppr_vht_mcs_rateset_t*)&src.limit[WL_RATE_1X1_MCS0])

#define CLM_DSSS_1X2_MULTI_RATESET(src) \
	((const ppr_dsss_rateset_t*)&src.limit[WL_RATE_1X2_DSSS_1])
#define CLM_OFDM_1X2_CDD_RATESET(src) \
	((const ppr_ofdm_rateset_t*)&src.limit[WL_RATE_1X2_CDD_OFDM_6])
#define CLM_MCS_1X2_CDD_RATESET(src) \
	((const ppr_vht_mcs_rateset_t*)&src.limit[WL_RATE_1X2_CDD_MCS0])

#define CLM_DSSS_1X3_MULTI_RATESET(src) \
	((const ppr_dsss_rateset_t*)&src.limit[WL_RATE_1X3_DSSS_1])
#define CLM_OFDM_1X3_CDD_RATESET(src) \
	((const ppr_ofdm_rateset_t*)&src.limit[WL_RATE_1X3_CDD_OFDM_6])
#define CLM_MCS_1X3_CDD_RATESET(src) \
	((const ppr_vht_mcs_rateset_t*)&src.limit[WL_RATE_1X3_CDD_MCS0])

#define CLM_MCS_2X2_SDM_RATESET(src) \
	((const ppr_vht_mcs_rateset_t*)&src.limit[WL_RATE_2X2_SDM_MCS8])
#define CLM_MCS_2X2_STBC_RATESET(src) \
	((const ppr_vht_mcs_rateset_t*)&src.limit[WL_RATE_2X2_STBC_MCS0])

#define CLM_MCS_2X3_STBC_RATESET(src) \
	((const ppr_vht_mcs_rateset_t*)&src.limit[WL_RATE_2X3_STBC_MCS0])
#define CLM_MCS_2X3_SDM_RATESET(src) \
	((const ppr_vht_mcs_rateset_t*)&src.limit[WL_RATE_2X3_SDM_MCS8])

#define CLM_MCS_3X3_SDM_RATESET(src) \
	((const ppr_vht_mcs_rateset_t*)&src.limit[WL_RATE_3X3_SDM_MCS16])

#define CLM_OFDM_1X2_TXBF_RATESET(src) \
	((const ppr_ofdm_rateset_t*)&src.limit[WL_RATE_1X2_TXBF_OFDM_6])
#define CLM_MCS_1X2_TXBF_RATESET(src) \
	((const ppr_vht_mcs_rateset_t*)&src.limit[WL_RATE_1X2_TXBF_MCS0])
#define CLM_OFDM_1X3_TXBF_RATESET(src) \
	((const ppr_ofdm_rateset_t*)&src.limit[WL_RATE_1X3_TXBF_OFDM_6])
#define CLM_MCS_1X3_TXBF_RATESET(src) \
	((const ppr_vht_mcs_rateset_t*)&src.limit[WL_RATE_1X3_TXBF_MCS0])
#define CLM_MCS_2X2_TXBF_RATESET(src) \
	((const ppr_ht_mcs_rateset_t*)&src.limit[WL_RATE_2X2_TXBF_SDM_MCS8])
#define CLM_MCS_2X3_TXBF_RATESET(src) \
	((const ppr_vht_mcs_rateset_t*)&src.limit[WL_RATE_2X3_TXBF_SDM_MCS8])
#define CLM_MCS_3X3_TXBF_RATESET(src) \
	((const ppr_ht_mcs_rateset_t*)&src.limit[WL_RATE_3X3_TXBF_SDM_MCS16])

#ifdef WLTXPWR_CACHE
static chanspec_t last_chanspec = 0;
#endif /* WLTXPWR_CACHE */

clm_result_t clm_aggregate_country_lookup(const ccode_t cc, unsigned int rev,
	clm_agg_country_t *agg);
clm_result_t clm_aggregate_country_map_lookup(const clm_agg_country_t agg,
	const ccode_t target_cc, unsigned int *rev);

static clm_result_t clm_power_limits(
	const clm_country_locales_t *locales, clm_band_t band,
	unsigned int chan, int ant_gain, clm_limits_type_t limits_type,
	const clm_limits_params_t *params, clm_power_limits_t *limits);

/* QDB() macro takes a dB value and converts to a quarter dB value */
#ifdef QDB
#undef QDB
#endif
#define QDB(n) ((n) * WLC_TXPWR_DB_FACTOR)

/* Regulatory Matrix Spreadsheet (CLM) MIMO v3.8.6.4
 * + CLM v4.1.3
 * + CLM v4.2.4
 * + CLM v4.3.1 (Item-1 only EU/9 and Q2/4).
 * + CLM v4.3.4_3x3 changes(Skip changes for a13/14).
 * + CLMv 4.5.3_3x3 changes for Item-5(Cisco Evora (change AP3500i to Evora)).
 * + CLMv 4.5.3_3x3 changes for Item-3(Create US/61 for BCM94331HM, based on US/53 power levels).
 * + CLMv 4.5.5 3x3 (changes from Create US/63 only)
 * + CLMv 4.4.4 3x3 changes(Create TR/4 (locales Bn7, 3tn), EU/12 (locales 3s, 3sn) for Airties.)
 */

/*
 * Some common channel sets
 */

/* All 2.4 GHz HW channels */
const chanvec_t chanvec_all_2G = {
	{0xfe, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00}
};

/* All 5 GHz HW channels */
const chanvec_t chanvec_all_5G = {
	/* 35,36,38/40,42,44,46/48,52/56,60 */
	{0x00, 0x00, 0x00, 0x00, 0x54, 0x55, 0x11, 0x11,
	/* 64/-/-/-/100/104,108/112,116/120,124 */
	0x01, 0x00, 0x00, 0x00, 0x10, 0x11, 0x11, 0x11,
#ifdef WL11AC
	/* /128,132/136,140/144,149/153,157/161,165... */
	0x11, 0x11, 0x21, 0x22, 0x22, 0x00, 0x00, 0x11,
#else
	/* /128,132/136,140/149/153,157/161,165... */
	0x11, 0x11, 0x20, 0x22, 0x22, 0x00, 0x00, 0x11,
#endif
	0x11, 0x11, 0x11, 0x01}
};

/*
 * Radar channel sets
 */

#ifdef BAND5G
static const chanvec_t radar_set1 = { /* Channels 52 - 64, 100 - 140 */
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11,	/* 52 - 60 */
	0x01, 0x00, 0x00, 0x00, 0x10, 0x11, 0x11, 0x11, 	/* 64, 100 - 124 */
	0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		/* 128 - 140 */
	0x00, 0x00, 0x00, 0x00}
};
#endif	/* BAND5G */

/*
 * Restricted channel sets
 */

#define WLC_REGCLASS_USA_2G_20MHZ	12
#define WLC_REGCLASS_EUR_2G_20MHZ	4
#define WLC_REGCLASS_JPN_2G_20MHZ	30
#define WLC_REGCLASS_JPN_2G_20MHZ_CH14	31

#ifdef WL11N
/*
 * bit map of supported regulatory class for USA, Europe & Japan
 */
static const rcvec_t rclass_us = { /* 1-5, 12, 22-25, 27-30, 32-33 */
	{0x3e, 0x10, 0xc0, 0x7b, 0x03}
};
static const rcvec_t rclass_eu = { /* 1-4, 5-12 */
	{0xfe, 0x1f, 0x00, 0x00, 0x00}
};
static const rcvec_t rclass_jp = { /* 1, 30-32 */
	{0x01, 0x00, 0x00, 0xc0, 0x01}
};
#endif /* WL11N */

#ifdef BAND5G
/*
 * channel to regulatory class map for USA
 */
static const rcinfo_t rcinfo_us_20 = {
	24,
	{
	{ 36,  1}, { 40,  1}, { 44,  1}, { 48,  1}, { 52,  2}, { 56,  2}, { 60,  2}, { 64,  2},
	{100,  4}, {104,  4}, {108,  4}, {112,  4}, {116,  4}, {120,  4}, {124,  4}, {128,  4},
	{132,  4}, {136,  4}, {140,  4}, {149,  3}, {153,  3}, {157,  3}, {161,  3}, {165,  5}
	}
};
#endif /* BAND5G */

#ifdef WL11N
/* control channel at lower sb */
static const rcinfo_t rcinfo_us_40lower = {
	18,
	{
	{  1, 32}, {  2, 32}, {  3, 32}, {  4, 32}, {  5, 32}, {  6, 32}, {  7, 32}, { 36, 22},
	{ 44, 22}, { 52, 23}, { 60, 23}, {100, 24}, {108, 24}, {116, 24}, {124, 24}, {132, 24},
	{149, 25}, {157, 25}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}
	}
};
/* control channel at upper sb */
static const rcinfo_t rcinfo_us_40upper = {
	18,
	{
	{  5, 33}, {  6, 33}, {  7, 33}, {  8, 33}, {  9, 33}, { 10, 33}, { 11, 33}, { 40, 27},
	{ 48, 27}, { 56, 28}, { 64, 28}, {104, 29}, {112, 29}, {120, 29}, {128, 29}, {136, 29},
	{153, 30}, {161, 30}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}
	}
};
#endif /* WL11N */

#ifdef BAND5G
/*
 * channel to regulatory class map for Europe
 */
static const rcinfo_t rcinfo_eu_20 = {
	19,
	{
	{ 36,  1}, { 40,  1}, { 44,  1}, { 48,  1}, { 52,  2}, { 56,  2}, { 60,  2}, { 64,  2},
	{100,  3}, {104,  3}, {108,  3}, {112,  3}, {116,  3}, {120,  3}, {124,  3}, {128,  3},
	{132,  3}, {136,  3}, {140,  3}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}
	}
};
#endif /* BAND5G */

#ifdef WL11N
static const rcinfo_t rcinfo_eu_40lower = {
	18,
	{
	{  1, 11}, {  2, 11}, {  3, 11}, {  4, 11}, {  5, 11}, {  6, 11}, {  7, 11}, {  8, 11},
	{  9, 11}, { 36,  5}, { 44,  5}, { 52,  6}, { 60,  6}, {100,  7}, {108,  7}, {116,  7},
	{124,  7}, {132,  7}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}
	}
};
static const rcinfo_t rcinfo_eu_40upper = {
	18,
	{
	{  5, 12}, {  6, 12}, {  7, 12}, {  8, 12}, {  9, 12}, { 10, 12}, { 11, 12}, { 12, 12},
	{ 13, 12}, { 40,  8}, { 48,  8}, { 56,  9}, { 64,  9}, {104, 10}, {112, 10}, {120, 10},
	{128, 10}, {136, 10}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}
	}
};
#endif /* WL11N */

#ifdef BAND5G
/*
 * channel to regulatory class map for Japan
 */
static const rcinfo_t rcinfo_jp_20 = {
	8,
	{
	{ 34,  1}, { 38,  1}, { 42,  1}, { 46,  1}, { 52, 32}, { 56, 32}, { 60, 32}, { 64, 32},
	{  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0},
	{  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}
	}
};
#endif /* BAND5G */

#ifdef WL11N
static const rcinfo_t rcinfo_jp_40 = {
	0,
	{
	{  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0},
	{  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0},
	{  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}, {  0,  0}
	}
};
#endif


/* iovar table */
enum {
	IOV_RCLASS = 1, /* read rclass */
	IOV_LAST
};

static const bcm_iovar_t cm_iovars[] = {
	{"rclass", IOV_RCLASS, 0, IOVT_UINT16, 0},
	{NULL, 0, 0, 0, 0}
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>


clm_result_t
wlc_locale_get_channels(clm_country_locales_t *locales, clm_band_t band,
	chanvec_t *channels, chanvec_t *restricted)
{
	bzero(channels, sizeof(chanvec_t));
	bzero(restricted, sizeof(chanvec_t));

	return clm_country_channels(locales, band, (clm_channels_t *)channels,
		(clm_channels_t *)restricted);
}

clm_result_t wlc_get_flags(clm_country_locales_t *locales, clm_band_t band, uint8 *flags)
{
	unsigned long clm_flags = 0;

	clm_result_t result = clm_country_flags(locales, band, &clm_flags);

	*flags = 0;
	if (result == CLM_RESULT_OK) {
		switch (clm_flags & CLM_FLAG_DFS_MASK) {
		case CLM_FLAG_DFS_EU:
			*flags |= WLC_DFS_EU;
			break;
		case CLM_FLAG_DFS_US:
			*flags |= WLC_DFS_FCC;
			break;
		case CLM_FLAG_DFS_NONE:
		default:
			break;
		}

		if (clm_flags & CLM_FLAG_FILTWAR1)
			*flags |= WLC_FILT_WAR;

		if (clm_flags & CLM_FLAG_TXBF)
			*flags |= WLC_TXBF;

		if (clm_flags & CLM_FLAG_NO_MIMO)
			*flags |= WLC_NO_MIMO;
		else {
			if (clm_flags & CLM_FLAG_NO_40MHZ)
				*flags |= WLC_NO_40MHZ;
			if (clm_flags & CLM_FLAG_NO_80MHZ)
				*flags |= WLC_NO_80MHZ;
		}

		if ((band == CLM_BAND_2G) && (clm_flags & CLM_FLAG_HAS_DSSS_EIRP))
			*flags |= WLC_EIRP;
		if ((band == CLM_BAND_5G) && (clm_flags & CLM_FLAG_HAS_OFDM_EIRP))
			*flags |= WLC_EIRP;
	}

	return result;
}


clm_result_t wlc_get_locale(clm_country_t country, clm_country_locales_t *locales)
{
	return clm_country_def(country, locales);
}


/* 20MHz channel info for 40MHz pairing support */

struct chan20_info {
	uint8	sb;
	uint8	adj_sbs;
};

/* indicates adjacent channels that are allowed for a 40 Mhz channel and
 * those that permitted by the HT
 */
const struct chan20_info chan20_info[] = {
	/* 11b/11g */
/* 0 */		{1,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 1 */		{2,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 2 */		{3,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 3 */		{4,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 4 */		{5,	(CH_UPPER_SB | CH_LOWER_SB | CH_EWA_VALID)},
/* 5 */		{6,	(CH_UPPER_SB | CH_LOWER_SB | CH_EWA_VALID)},
/* 6 */		{7,	(CH_UPPER_SB | CH_LOWER_SB | CH_EWA_VALID)},
/* 7 */		{8,	(CH_UPPER_SB | CH_LOWER_SB | CH_EWA_VALID)},
/* 8 */		{9,	(CH_UPPER_SB | CH_LOWER_SB | CH_EWA_VALID)},
/* 9 */		{10,	(CH_LOWER_SB | CH_EWA_VALID)},
/* 10 */	{11,	(CH_LOWER_SB | CH_EWA_VALID)},
/* 11 */	{12,	(CH_LOWER_SB)},
/* 12 */	{13,	(CH_LOWER_SB)},
/* 13 */	{14,	(CH_LOWER_SB)},

/* 11a japan high */
/* 14 */	{34,	(CH_UPPER_SB)},
/* 15 */	{38,	(CH_LOWER_SB)},
/* 16 */	{42,	(CH_LOWER_SB)},
/* 17 */	{46,	(CH_LOWER_SB)},

/* 11a usa low */
/* 18 */	{36,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 19 */	{40,	(CH_LOWER_SB | CH_EWA_VALID)},
/* 20 */	{44,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 21 */	{48,	(CH_LOWER_SB | CH_EWA_VALID)},
/* 22 */	{52,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 23 */	{56,	(CH_LOWER_SB | CH_EWA_VALID)},
/* 24 */	{60,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 25 */	{64,	(CH_LOWER_SB | CH_EWA_VALID)},

/* 11a Europe */
/* 26 */	{100,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 27 */	{104,	(CH_LOWER_SB | CH_EWA_VALID)},
/* 28 */	{108,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 29 */	{112,	(CH_LOWER_SB | CH_EWA_VALID)},
/* 30 */	{116,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 31 */	{120,	(CH_LOWER_SB | CH_EWA_VALID)},
/* 32 */	{124,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 33 */	{128,	(CH_LOWER_SB | CH_EWA_VALID)},
/* 34 */	{132,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 35 */	{136,	(CH_LOWER_SB | CH_EWA_VALID)},

#ifdef WL11AC
/* 36 */	{140,   (CH_UPPER_SB | CH_EWA_VALID)},
/* 37 */	{144,   (CH_LOWER_SB)},

/* 11a usa high, ref5 only */
/* The 0x80 bit in pdiv means these are REF5, other entries are REF20 */
/* 38 */	{149,   (CH_UPPER_SB | CH_EWA_VALID)},
/* 39 */	{153,   (CH_LOWER_SB | CH_EWA_VALID)},
/* 40 */	{157,   (CH_UPPER_SB | CH_EWA_VALID)},
/* 41 */	{161,   (CH_LOWER_SB | CH_EWA_VALID)},
/* 42 */	{165,   (CH_LOWER_SB)},

/* 11a japan */
/* 43 */	{184,   (CH_UPPER_SB)},
/* 44 */	{188,   (CH_LOWER_SB)},
/* 45 */	{192,   (CH_UPPER_SB)},
/* 46 */	{196,   (CH_LOWER_SB)},
/* 47 */	{200,   (CH_UPPER_SB)},
/* 48 */	{204,   (CH_LOWER_SB)},
/* 49 */	{208,   (CH_UPPER_SB)},
/* 50 */	{212,   (CH_LOWER_SB)},
/* 51 */	{216,   (CH_LOWER_SB)}
};

#else

/* 36 */	{140,	(CH_LOWER_SB)},

/* 11a usa high, ref5 only */
/* The 0x80 bit in pdiv means these are REF5, other entries are REF20 */
/* 37 */	{149,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 38 */	{153,	(CH_LOWER_SB | CH_EWA_VALID)},
/* 39 */	{157,	(CH_UPPER_SB | CH_EWA_VALID)},
/* 40 */	{161,	(CH_LOWER_SB | CH_EWA_VALID)},
/* 41 */	{165,	(CH_LOWER_SB)},

/* 11a japan */
/* 42 */	{184,	(CH_UPPER_SB)},
/* 43 */	{188,	(CH_LOWER_SB)},
/* 44 */	{192,	(CH_UPPER_SB)},
/* 45 */	{196,	(CH_LOWER_SB)},
/* 46 */	{200,	(CH_UPPER_SB)},
/* 47 */	{204,	(CH_LOWER_SB)},
/* 48 */	{208,	(CH_UPPER_SB)},
/* 49 */	{212,	(CH_LOWER_SB)},
/* 50 */	{216,	(CH_LOWER_SB)}
};
#endif /* WL11AC */


/* country code mapping for SPROM rev 1 */
static const char def_country[][WLC_CNTRY_BUF_SZ] = {
	"AU",   /* Worldwide */
	"TH",   /* Thailand */
	"IL",   /* Israel */
	"JO",   /* Jordan */
	"CN",   /* China */
	"JP",   /* Japan */
	"US",   /* USA */
	"DE",   /* Europe */
	"US",   /* US Low Band, use US */
	"JP",   /* Japan High Band, use Japan */
};

/* autocountry default country code list */
static const char def_autocountry[][WLC_CNTRY_BUF_SZ] = {
	"XY",
	"XA",
	"XB",
	"X0",
	"X1",
	"X2",
	"X3",
	"XS",
	"XV"
};

static const char BCMATTACHDATA(rstr_ccode)[] = "ccode";
static const char BCMATTACHDATA(rstr_cc)[] = "cc";
static const char BCMATTACHDATA(rstr_regrev)[] = "regrev";

static bool
wlc_autocountry_lookup(char *cc)
{
	uint i;

	for (i = 0; i < ARRAYSIZE(def_autocountry); i++)
		if (!strcmp(def_autocountry[i], cc))
			return TRUE;

	return FALSE;
}

static bool
wlc_lookup_advertised_cc(char* ccode, const clm_country_t country)
{
	ccode_t advertised_cc;
	bool rv = FALSE;
	if (CLM_RESULT_OK == clm_country_advertised_cc(country, advertised_cc)) {
		memcpy(ccode, advertised_cc, 2);
		ccode[2] = '\0';
		rv = TRUE;
	}

	return rv;
}


static int
wlc_channel_init_ccode(wlc_cm_info_t *wlc_cm, char* country_abbrev, int ca_len)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	int result = BCME_OK;
	clm_country_t country;
	int rev = -1;
#ifdef PCOEM_LINUXSTA
	bool use_row = TRUE;
	wlc_pub_t *pub = wlc->pub;
#endif

	result = wlc_country_lookup(wlc, country_abbrev, &country);

	/* default to US if country was not specified or not found */
	if (result != CLM_RESULT_OK) {
		/* set specific country code for 43162 */
		if (BCM4350_CHIP(wlc->pub->sih->chip) &&
			(wlc->pub->sih->bustype == PCI_BUS)) {
			strncpy(country_abbrev, "XV", ca_len - 1);
			rev = 1;
		} else
			strncpy(country_abbrev, "US", ca_len - 1);

		result = wlc_country_lookup(wlc, country_abbrev, &country);
	}

	ASSERT(result == CLM_RESULT_OK);

	/* save default country for exiting 11d regulatory mode */
	wlc_cntry_set_default(wlc->cntry, country_abbrev);

	/* initialize autocountry_default to driver default */
	if (wlc_autocountry_lookup(country_abbrev))
		wlc_11d_set_autocountry_default(wlc->m11d, country_abbrev);
	else
		wlc_11d_set_autocountry_default(wlc->m11d, "XV");

#ifdef PCOEM_LINUXSTA
	if ((CHIPID(pub->sih->chip) != BCM4311_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM4312_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM4313_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM4321_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM4322_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM43224_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM43225_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM43421_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM4342_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM43131_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM43217_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM43227_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM43228_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM4331_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM43142_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM43428_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM43602_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM4350_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM4354_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM4356_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM4360_CHIP_ID) &&
	    (CHIPID(pub->sih->chip) != BCM4352_CHIP_ID)) {
		printf("Broadcom vers %s: Unsupported Chip (%x)\n",
			EPI_VERSION_STR, pub->sih->chip);
		return BCME_ERROR;
	}

	if ((pub->sih->boardvendor == VENDOR_HP) && (!bcmp(country_abbrev, "US", 2) ||
		!bcmp(country_abbrev, "JP", 2) || !bcmp(country_abbrev, "IL", 2)))
		use_row = FALSE;

	/* use RoW locale if set */
	if (use_row && bcmp(country_abbrev, "X", 1)) {
		bzero(country_abbrev, WLC_CNTRY_BUF_SZ);
		strncpy(country_abbrev, "XW", WLC_CNTRY_BUF_SZ);
	}

	/* Enable Auto Country Discovery */
	wlc_11d_set_autocountry_default(wlc->m11d, country_abbrev);

#endif /* PCOEM_LINUXSTA */

	/* Calling set_countrycode() once does not generate any event, if called more than
	 * once generates COUNTRY_CODE_CHANGED event which will cause the driver to crash
	 * at startup since bsscfg structure is still not initialized.
	 */
	if (rev == -1)
		wlc_set_countrycode(wlc_cm, country_abbrev);
	else
		wlc_set_countrycode_rev(wlc_cm, country_abbrev, rev);

	return result;
}

static int
wlc_cm_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int val_size, struct wlc_if *wlcif)
{
	wlc_cm_info_t *wlc_cm = (wlc_cm_info_t *)hdl;
	int err = BCME_OK;
	int32 int_val = 0;
	int32 *ret_int_ptr;

	/* convenience int and bool vals for first 8 bytes of buffer */
	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	switch (actionid) {
	case IOV_GVAL(IOV_RCLASS):
		*ret_int_ptr = wlc_get_regclass(wlc_cm, (chanspec_t)int_val);
		WL_INFORM(("chspec:%x rclass:%d\n", (chanspec_t)int_val, *ret_int_ptr));

		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

wlc_cm_info_t *
BCMATTACHFN(wlc_channel_mgr_attach)(wlc_info_t *wlc)
{
	clm_result_t result;
	wlc_cm_info_t *wlc_cm;
	char country_abbrev[WLC_CNTRY_BUF_SZ];
	wlc_pub_t *pub = wlc->pub;
#ifdef WLCLMINC
	uint32 clm_inc_size;
	uint32 clm_inc_hdr_offset;
#endif

	WL_TRACE(("wl%d: wlc_channel_mgr_attach\n", wlc->pub->unit));

	if ((wlc_cm = (wlc_cm_info_t *)MALLOC(pub->osh, sizeof(wlc_cm_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes", pub->unit,
			__FUNCTION__, MALLOCED(pub->osh)));
		return NULL;
	}
	bzero((char *)wlc_cm, sizeof(wlc_cm_info_t));
	wlc_cm->pub = pub;
	wlc_cm->wlc = wlc;
	wlc->cmi = wlc_cm;

	/* init the per chain limits to max power so they have not effect */
	memset(&wlc_cm->bandstate[BAND_2G_INDEX].chain_limits, WLC_TXPWR_MAX,
	       sizeof(struct wlc_channel_txchain_limits));
	memset(&wlc_cm->bandstate[BAND_5G_INDEX].chain_limits, WLC_TXPWR_MAX,
	       sizeof(struct wlc_channel_txchain_limits));

	/* get the SPROM country code or local, required to initialize channel set below */
	bzero(country_abbrev, WLC_CNTRY_BUF_SZ);
	if (wlc->pub->sromrev > 1) {
		/* get country code */
		const char *ccode = getvar(wlc->pub->vars, rstr_ccode);
		if (ccode) {

#ifndef ATE_BUILD
#ifndef OPENSRC_IOV_IOCTL
			int err;
#endif /* OPENSRC_IOV_IOCTL */
#endif /* #ifndef ATE_BUILD */
			strncpy(country_abbrev, ccode, WLC_CNTRY_BUF_SZ - 1);

#ifndef ATE_BUILD
#ifndef OPENSRC_IOV_IOCTL
			err = wlc_cntry_external_to_internal(country_abbrev,
				sizeof(country_abbrev));
			if (err != BCME_OK) {
				strncpy(country_abbrev, "__", WLC_CNTRY_BUF_SZ - 1);
			}
#endif /* OPENSRC_IOV_IOCTL */
#endif /* #ifndef ATE_BUILD */
		}
	} else {
		uint locale_num;

		/* get locale */
		locale_num = (uint)getintvar(wlc->pub->vars, rstr_cc);
		/* get mapped country */
		if (locale_num < ARRAYSIZE(def_country))
			strncpy(country_abbrev, def_country[locale_num],
			        sizeof(country_abbrev) - 1);
	}

#if defined(ATE_BUILD)
	/* convert "ALL/0" country code to #a/0 */
	if (!strncmp(country_abbrev, "ALL", WLC_CNTRY_BUF_SZ)) {
		strncpy(country_abbrev, "#a", sizeof(country_abbrev) - 1);
	}
#endif 

	strncpy(wlc_cm->srom_ccode, country_abbrev, WLC_CNTRY_BUF_SZ - 1);
	wlc_cm->srom_regrev = getintvar(wlc->pub->vars, rstr_regrev);

	/* For "some" apple boards with KR2, make them as KR3 as they have passed the
	 * FCC test but with wrong SROM contents
	 */
	if ((pub->sih->boardvendor == VENDOR_APPLE) &&
	    ((pub->sih->boardtype == BCM943224M93) ||
	     (pub->sih->boardtype == BCM943224M93A))) {
		if ((wlc_cm->srom_regrev == 2) &&
		    !strncmp(country_abbrev, "KR", WLC_CNTRY_BUF_SZ)) {
			wlc_cm->srom_regrev = 3;
		}
	}

	/* Correct SROM contents of an Apple board */
	if ((pub->sih->boardvendor == VENDOR_APPLE) &&
	    (pub->sih->boardtype == 0x93) &&
	    !strncmp(country_abbrev, "JP", WLC_CNTRY_BUF_SZ) &&
	    (wlc_cm->srom_regrev == 4)) {
		wlc_cm->srom_regrev = 6;
	}

	if ((pub->sih->boardvendor == VENDOR_APPLE) &&
	    (pub->sih->boardtype == BCM94360X51A)) {
		if ((!strncmp(country_abbrev, "X0", WLC_CNTRY_BUF_SZ)) ||
		    (!strncmp(country_abbrev, "X2", WLC_CNTRY_BUF_SZ)) ||
		    (!strncmp(country_abbrev, "X3", WLC_CNTRY_BUF_SZ)))
		        wlc_cm->srom_regrev = 19;
	}

	result = clm_init(&clm_header);
	ASSERT(result == CLM_RESULT_OK);

	/* these are initialised to zero until they point to malloced data */
	wlc_cm->clm_base_dataptr = NULL;
	wlc_cm->clm_base_data_len = 0;


	wlc_cm->clm_inc_dataptr = NULL;
	wlc_cm->clm_inc_headerptr = NULL;
	wlc_cm->clm_inc_data_len = 0;

#ifdef WLCLMINC
	/* Need to malloc memory for incremental data so existing stuff can be reclaimed.
	   How do we know the size?
	*/

	extern char _clmincstart;
	extern char _clmincend;

	clm_inc_size = (void*)&_clmincend - (void*)&_clmincstart;

	clm_inc_hdr_offset = (uint32)&clm_inc_header - (uint32)&_clmincstart;

	if ((wlc_cm->clm_inc_dataptr = (void*)MALLOC(wlc_cm->pub->osh, clm_inc_size)) != NULL) {
		bcopy((char*)&_clmincstart, (char*)wlc_cm->clm_inc_dataptr, clm_inc_size);
		wlc_cm->clm_inc_data_len = clm_inc_size;
		wlc_cm->clm_inc_headerptr = (struct clm_data_header*)((char*)wlc_cm->clm_inc_dataptr
			+ clm_inc_hdr_offset);
		result = clm_set_inc_data(wlc_cm->clm_inc_headerptr);
		ASSERT(result == CLM_RESULT_OK);

		/* Immediately reclaim CLM incremental data after copying data to malloc'ed
		 * buffer. This avoids holding onto 2 copies of the incremental data during
		 * attach time, and frees up more heap space for attach time allocations.
		 */
		bzero((void*)&_clmincstart, clm_inc_size);
		OSL_ARENA_ADD((uint32)&_clmincstart, clm_inc_size);
	}
#endif /* WLCLMINC */

	wlc_cm->bandedge_filter_apply = (CHIPID(pub->sih->chip) == BCM4331_CHIP_ID);

	result = wlc_channel_init_ccode(wlc_cm, country_abbrev, sizeof(country_abbrev));

#ifdef PCOEM_LINUXSTA
	if (result != BCME_OK) {
		wlc->cmi = NULL;
		wlc_channel_mgr_detach(wlc_cm);
		return NULL;
	}
#else
	BCM_REFERENCE(result);
#endif /* PCOEM_LINUXSTA */

#ifdef WLTDLS
	/* setupreq */
	if (TDLS_SUPPORT(wlc->pub)) {
		if (wlc_ier_add_build_fn(wlc->ier_tdls_srq, DOT11_MNG_REGCLASS_ID,
			wlc_channel_tdls_calc_rc_ie_len, wlc_channel_tdls_write_rc_ie, wlc_cm)
			!= BCME_OK) {
			WL_ERROR(("wl%d: %s wlc_ier_add_build_fn failed, "
				"reg class in tdls setupreq\n", wlc->pub->unit, __FUNCTION__));
			goto fail;
		}
		/* setupresp */
		if (wlc_ier_add_build_fn(wlc->ier_tdls_srs, DOT11_MNG_REGCLASS_ID,
			wlc_channel_tdls_calc_rc_ie_len, wlc_channel_tdls_write_rc_ie, wlc_cm)
			!= BCME_OK) {
			WL_ERROR(("wl%d: %s wlc_ier_add_build_fn failed, "
				"reg class in tdls setupresp\n", wlc->pub->unit, __FUNCTION__));
			goto fail;
		}
	}
#endif /* WLTDLS */

	/* register module */
	if (wlc_module_register(wlc->pub, cm_iovars, "cm", wlc_cm, wlc_cm_doiovar,
	    NULL, NULL, NULL)) {
		WL_ERROR(("wl%d: %s wlc_module_register() failed\n", wlc->pub->unit, __FUNCTION__));

		goto fail;
	}


	return wlc_cm;

	goto fail;

fail:	/* error handling */
	wlc->cmi = NULL;
	wlc_channel_mgr_detach(wlc_cm);
	return NULL;
}


static struct clm_data_header* download_dataptr = NULL;
static int download_data_len = 0;

int
wlc_handle_clm_dload(wlc_cm_info_t *wlc_cm, void* data_chunk, int clm_offset, int chunk_len,
	int clm_total_len, int ds_id)
{
	int result = BCME_OK;

	clm_country_t country;
	char country_abbrev[WLC_CNTRY_BUF_SZ];

	if (clm_offset == 0) {
		/* Clean up if a previous download didn't complete */
		if (download_dataptr != NULL) {
			MFREE(wlc_cm->pub->osh, download_dataptr, download_data_len);
			download_dataptr = NULL;
			download_data_len = 0;
		}

		if ((clm_total_len != 0) &&
			(download_dataptr = (struct clm_data_header*)MALLOC(wlc_cm->pub->osh,
			clm_total_len)) == NULL) {
			WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", wlc_cm->pub->unit,
				__FUNCTION__, MALLOCED(wlc_cm->pub->osh)));
			return BCME_ERROR;
		}
		download_data_len = clm_total_len;
	} else if (download_data_len == 0) {
		WL_ERROR(("wl%d: %s: No memory allocated for chunk at %d\n", wlc_cm->pub->unit,
			__FUNCTION__, clm_offset));
		return BCME_ERROR;
	}

	if ((clm_offset + chunk_len) <= download_data_len) {
		if (download_dataptr != NULL)
			bcopy(data_chunk, (char*)download_dataptr + clm_offset, chunk_len);

		if ((clm_offset + chunk_len) == download_data_len) {
			/* Download complete. Install the new data */
			if (ds_id != 0) {
				/* Incremental CLM data */
				if (wlc_cm->clm_inc_dataptr != NULL) {
					MFREE(wlc_cm->pub->osh,
						wlc_cm->clm_inc_dataptr, wlc_cm->clm_inc_data_len);
				}
				if (clm_set_inc_data(download_dataptr) != CLM_RESULT_OK) {
					WL_ERROR(("wl%d: %s: Error loading incremental CLM data."
						" Revert to default data\n",
						wlc_cm->pub->unit, __FUNCTION__));
					result = BCME_ERROR;
					clm_set_inc_data(NULL);
					wlc_cm->clm_inc_data_len = 0;
					wlc_cm->clm_inc_dataptr = NULL;
					wlc_cm->clm_inc_headerptr = NULL;
					MFREE(wlc_cm->pub->osh, download_dataptr,
						download_data_len);
				} else {
					wlc_cm->clm_inc_dataptr = download_dataptr;
					wlc_cm->clm_inc_headerptr = download_dataptr;
					wlc_cm->clm_inc_data_len = download_data_len;
				}
			} else {	/* Replacement CLM base data */
				clm_result_t clm_result = CLM_RESULT_OK;
				/* Clear out incremental data */
				if (wlc_cm->clm_inc_dataptr != NULL) {
					clm_set_inc_data(NULL);
					MFREE(wlc_cm->pub->osh, wlc_cm->clm_inc_dataptr,
						wlc_cm->clm_inc_data_len);
					wlc_cm->clm_inc_data_len = 0;
					wlc_cm->clm_inc_dataptr = NULL;
					wlc_cm->clm_inc_headerptr = NULL;
				}
				/* Free any previously downloaded base data */
				if (wlc_cm->clm_base_dataptr != NULL) {
					MFREE(wlc_cm->pub->osh, wlc_cm->clm_base_dataptr,
						wlc_cm->clm_base_data_len);
				}
				if (download_dataptr != NULL) {
					WL_NONE(("wl%d: Pointing API at new base data: v%s\n",
						wlc_cm->pub->unit, download_dataptr->clm_version));
					clm_result = clm_init(download_dataptr);
					if (clm_result != CLM_RESULT_OK) {
						WL_ERROR(("wl%d: %s: Error loading new base CLM"
							" data.\n",
							wlc_cm->pub->unit, __FUNCTION__));
						result = BCME_ERROR;
						MFREE(wlc_cm->pub->osh, download_dataptr,
							download_data_len);
					}
				}
				if ((download_dataptr == NULL) || (clm_result != CLM_RESULT_OK)) {
					WL_NONE(("wl%d: %s: Reverting to base data.\n",
						wlc_cm->pub->unit, __FUNCTION__));
					clm_init(&clm_header);
					wlc_cm->clm_base_data_len = 0;
					wlc_cm->clm_base_dataptr = NULL;
				} else {
					wlc_cm->clm_base_dataptr = download_dataptr;
					wlc_cm->clm_base_data_len = download_data_len;
				}
			}
			if (wlc_country_lookup_direct(wlc_cm->ccode, wlc_cm->regrev, &country) ==
				CLM_RESULT_OK)
				wlc_cm->country = country;
			else
				wlc_cm->country = 0;

			bzero(country_abbrev, WLC_CNTRY_BUF_SZ);
			strncpy(country_abbrev, wlc_cm->srom_ccode, WLC_CNTRY_BUF_SZ - 1);
			result = wlc_channel_init_ccode(wlc_cm, country_abbrev,
				sizeof(country_abbrev));

			/* download complete - tidy up */
			download_dataptr = NULL;
			download_data_len = 0;

		}
	} else {
		WL_ERROR(("wl%d: %s: incremental download too big for allocated memory"
			"(clm_offset %d, clm_offset + chunk_len %d, download_data_len %d)\n",
			wlc_cm->pub->unit, __FUNCTION__, clm_offset, clm_offset + chunk_len,
			download_data_len));
		MFREE(wlc_cm->pub->osh, download_dataptr, download_data_len);
		download_dataptr = NULL;
		download_data_len = 0;
		result = BCME_ERROR;
	}
	return result;
}


void
BCMATTACHFN(wlc_channel_mgr_detach)(wlc_cm_info_t *wlc_cm)
{
	wlc_info_t *wlc;

	if (wlc_cm) {
		wlc = wlc_cm->wlc;

		wlc_module_unregister(wlc->pub, "cm", wlc_cm);
		if (wlc_cm->clm_inc_dataptr != NULL) {
			MFREE(wlc_cm->pub->osh, wlc_cm->clm_inc_dataptr, wlc_cm->clm_inc_data_len);
		}
		if (wlc_cm->clm_base_dataptr != NULL) {
			MFREE(wlc_cm->pub->osh, wlc_cm->clm_base_dataptr,
				wlc_cm->clm_base_data_len);
		}
		MFREE(wlc_cm->pub->osh, wlc_cm, sizeof(wlc_cm_info_t));
	}
}

const char* wlc_channel_country_abbrev(wlc_cm_info_t *wlc_cm)
{
	return wlc_cm->country_abbrev;
}

const char* wlc_channel_ccode(wlc_cm_info_t *wlc_cm)
{
	return wlc_cm->ccode;
}

uint wlc_channel_regrev(wlc_cm_info_t *wlc_cm)
{
	return wlc_cm->regrev;
}

uint8 wlc_channel_locale_flags(wlc_cm_info_t *wlc_cm)
{
	wlc_info_t *wlc = wlc_cm->wlc;

	return wlc_cm->bandstate[wlc->band->bandunit].locale_flags;
}

uint8 wlc_channel_locale_flags_in_band(wlc_cm_info_t *wlc_cm, uint bandunit)
{
	return wlc_cm->bandstate[bandunit].locale_flags;
}

/*
 * return the first valid chanspec for the locale, if one is not found and hw_fallback is true
 * then return the first h/w supported chanspec.
 */
chanspec_t
wlc_default_chanspec(wlc_cm_info_t *wlc_cm, bool hw_fallback)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	chanspec_t  chspec;

	chspec = wlc_create_chspec(wlc, 0);
	/* try to find a chanspec that's valid in this locale */
	if ((chspec = wlc_next_chanspec(wlc_cm, chspec, CHAN_TYPE_ANY, 0)) == INVCHANSPEC)
		/* try to find a chanspec valid for this hardware */
		if (hw_fallback)
			chspec = wlc_phy_chanspec_band_firstch(wlc_cm->wlc->band->pi,
				wlc->band->bandtype);

	return chspec;
}

/*
 * Return the next channel's chanspec.
 */
chanspec_t
wlc_next_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t cur_chanspec, int type, bool any_band)
{
	uint8 ch;
	uint8 cur_chan = CHSPEC_CHANNEL(cur_chanspec);
	chanspec_t chspec;

	/* 0 is an invalid chspec, routines trying to find the first available channel should
	 * now be using wlc_default_chanspec (above)
	 */
	ASSERT(cur_chanspec);

	/* Try all channels in current band */
	ch = cur_chan + 1;
	for (; ch <= MAXCHANNEL; ch++) {
		if (ch == MAXCHANNEL)
			ch = 0;
		if (ch == cur_chan)
			break;
		/* create the next channel spec */
		chspec = cur_chanspec & ~WL_CHANSPEC_CHAN_MASK;
		chspec |= ch;
		if (wlc_valid_chanspec(wlc_cm, chspec)) {
			if ((type == CHAN_TYPE_ANY) ||
			(type == CHAN_TYPE_CHATTY && !wlc_quiet_chanspec(wlc_cm, chspec)) ||
			(type == CHAN_TYPE_QUIET && wlc_quiet_chanspec(wlc_cm, chspec)))
				return chspec;
		}
	}

	if (!any_band)
		return ((chanspec_t)INVCHANSPEC);

	/* Couldn't find any in current band, try other band */
	ch = cur_chan + 1;
	for (; ch <= MAXCHANNEL; ch++) {
		if (ch == MAXCHANNEL)
			ch = 0;
		if (ch == cur_chan)
			break;

		/* create the next channel spec */
		chspec = cur_chanspec & ~(WL_CHANSPEC_CHAN_MASK | WL_CHANSPEC_BAND_MASK);
		chspec |= ch;
		if (CHANNEL_BANDUNIT(wlc, ch) == BAND_2G_INDEX)
			chspec |= WL_CHANSPEC_BAND_2G;
		else
			chspec |= WL_CHANSPEC_BAND_5G;
		if (wlc_valid_chanspec_db(wlc_cm, chspec)) {
			if ((type == CHAN_TYPE_ANY) ||
			(type == CHAN_TYPE_CHATTY && !wlc_quiet_chanspec(wlc_cm, chspec)) ||
			(type == CHAN_TYPE_QUIET && wlc_quiet_chanspec(wlc_cm, chspec)))
				return chspec;
		}
	}

	return ((chanspec_t)INVCHANSPEC);
}

/* return chanvec for a given country code and band */
bool
wlc_channel_get_chanvec(struct wlc_info *wlc, const char* country_abbrev,
	int bandtype, chanvec_t *channels)
{
	clm_band_t band;
	clm_result_t result = CLM_RESULT_ERR;
	clm_country_t country;
	clm_country_locales_t locale;
	chanvec_t unused;
	uint i;
	chanvec_t channels_5g20, channels_5g4080;

	result = wlc_country_lookup(wlc, country_abbrev, &country);
	if (result != CLM_RESULT_OK)
		return FALSE;

	result = wlc_get_locale(country, &locale);
	if (bandtype != WLC_BAND_2G && bandtype != WLC_BAND_5G)
		return FALSE;

	band = (bandtype == WLC_BAND_2G) ? CLM_BAND_2G : CLM_BAND_5G;

	wlc_locale_get_channels(&locale, band, channels, &unused);
	clm_valid_channels_5g(&locale, (clm_channels_t*)&channels_5g20,
	(clm_channels_t*)&channels_5g4080);

	/* don't mask 2GHz channels, but check 5G channels */
	for (i = (CH_MAX_2G_CHANNEL / 8) + 1; i < sizeof(chanvec_t); i++)
	channels->vec[i] &= channels_5g20.vec[i];
	return TRUE;
}

/* set the driver's current country and regulatory information using a country code
 * as the source. Lookup built in country information found with the country code.
 */
int
wlc_set_countrycode(wlc_cm_info_t *wlc_cm, const char* ccode)
{
	WL_NONE(("wl%d: %s: ccode \"%s\"\n", wlc_cm->wlc->pub->unit, __FUNCTION__, ccode));
	return wlc_set_countrycode_rev(wlc_cm, ccode, -1);
}

int
wlc_set_countrycode_rev(wlc_cm_info_t *wlc_cm, const char* ccode, int regrev)
{
	clm_result_t result = CLM_RESULT_ERR;
	clm_country_t country;
	char mapped_ccode[WLC_CNTRY_BUF_SZ];
	uint mapped_regrev = 0;
	char country_abbrev[WLC_CNTRY_BUF_SZ] = { 0 };

	WL_NONE(("wl%d: %s: (country_abbrev \"%s\", ccode \"%s\", regrev %d) SPROM \"%s\"/%u\n",
	         wlc->pub->unit, __FUNCTION__,
	         country_abbrev, ccode, regrev, wlc_cm->srom_ccode, wlc_cm->srom_regrev));

	/* if regrev is -1, lookup the mapped country code,
	 * otherwise use the ccode and regrev directly
	 */
	if (regrev == -1) {
		/* map the country code to a built-in country code, regrev, and country_info */
		result = wlc_countrycode_map(wlc_cm, ccode, mapped_ccode, &mapped_regrev, &country);
		if (result == CLM_RESULT_OK)
			WL_NONE(("wl%d: %s: mapped to \"%s\"/%u\n",
			         wlc->pub->unit, __FUNCTION__, ccode, mapped_regrev));
		else
			WL_NONE(("wl%d: %s: failed lookup\n",
			        wlc->pub->unit, __FUNCTION__));
	} else {
		/* find the matching built-in country definition */
		result = wlc_country_lookup_direct(ccode, regrev, &country);
		strncpy(mapped_ccode, ccode, WLC_CNTRY_BUF_SZ-1);
		mapped_ccode[WLC_CNTRY_BUF_SZ-1] = '\0';
		mapped_regrev = regrev;
	}

	if (result != CLM_RESULT_OK)
		return BCME_BADARG;

	/* Set the driver state for the country.
	 * Getting the advertised country code from CLM.
	 * Else use the one comes from ccode.
	 */
	if (wlc_lookup_advertised_cc(country_abbrev, country))
		wlc_set_country_common(wlc_cm, country_abbrev,
		mapped_ccode, mapped_regrev, country);
	else
		wlc_set_country_common(wlc_cm, ccode,
		mapped_ccode, mapped_regrev, country);

#ifdef WLOLPC
	if (OLPC_ENAB(wlc_cm->wlc)) {
		WL_RATE(("olpc process: ccrev=%s regrev=%d\n", ccode, regrev));
		/* olpc needs to flush any stored chan info and cal if needed */
		wlc_olpc_eng_reset(wlc_cm->wlc->olpc_info);
	}
#endif /* WLOLPC */

	return 0;
}

/* set the driver's current country and regulatory information using a country code
 * as the source. Look up built in country information found with the country code.
 */
static void
wlc_set_country_common(wlc_cm_info_t *wlc_cm,
                       const char* country_abbrev,
                       const char* ccode, uint regrev, clm_country_t country)
{
	clm_result_t result = CLM_RESULT_ERR;
	clm_country_locales_t locale;
	uint8 flags;
	unsigned long clm_flags = 0;

	wlc_info_t *wlc = wlc_cm->wlc;
	char prev_country_abbrev[WLC_CNTRY_BUF_SZ];

#ifdef WLTXPWR_CACHE
	wlc_phy_txpwr_cache_invalidate(wlc_phy_get_txpwr_cache(wlc->band->pi));
#endif	/* WLTXPWR_CACHE */
	/* save current country state */
	wlc_cm->country = country;

	bzero(&prev_country_abbrev, WLC_CNTRY_BUF_SZ);
	strncpy(prev_country_abbrev, wlc_cm->country_abbrev, WLC_CNTRY_BUF_SZ - 1);

	strncpy(wlc_cm->country_abbrev, country_abbrev, WLC_CNTRY_BUF_SZ-1);
	strncpy(wlc_cm->ccode, ccode, WLC_CNTRY_BUF_SZ-1);
	wlc_cm->regrev = regrev;

	result = wlc_get_locale(country, &locale);
	ASSERT(result == CLM_RESULT_OK);

	result = wlc_get_flags(&locale, CLM_BAND_2G, &flags);
	ASSERT(result == CLM_RESULT_OK);
	BCM_REFERENCE(result);

	result = clm_country_flags(&locale,  CLM_BAND_2G, &clm_flags);
	ASSERT(result == CLM_RESULT_OK);
	if (clm_flags & CLM_FLAG_EDCRS_EU) {
		wlc_phy_set_locale(wlc->band->pi, REGION_EU);
	} else {
		wlc_phy_set_locale(wlc->band->pi, REGION_OTHER);
	}

#ifdef WL_BEAMFORMING
	if (TXBF_ENAB(wlc->pub)) {
		if (flags & WLC_TXBF) {
			wlc_stf_set_txbf(wlc, TRUE);
		} else {
			wlc_stf_set_txbf(wlc, FALSE);
		}
	}
#endif

#ifdef WL11N
	/* disable/restore nmode based on country regulations */
	if ((flags & WLC_NO_MIMO) && ((NBANDS(wlc) == 2) || IS_SINGLEBAND_5G(wlc->deviceid))) {
		result = wlc_get_flags(&locale, CLM_BAND_5G, &flags);
		ASSERT(result == CLM_RESULT_OK);
	}
	if (flags & WLC_NO_MIMO) {
		wlc_set_nmode(wlc, OFF);
		wlc->stf->no_cddstbc = TRUE;
	} else {
		wlc->stf->no_cddstbc = FALSE;
		wlc_prot_n_mode_reset(wlc->prot_n, FALSE);
	}

	wlc_stf_ss_update(wlc, wlc->bandstate[BAND_2G_INDEX]);
	wlc_stf_ss_update(wlc, wlc->bandstate[BAND_5G_INDEX]);
#endif /* WL11N */

#if defined(AP) && defined(RADAR)
	if ((NBANDS(wlc) == 2) || IS_SINGLEBAND_5G(wlc->deviceid)) {
		phy_radar_detect_mode_t mode;
		result = wlc_get_flags(&locale, CLM_BAND_5G, &flags);

		mode = ISDFS_EU(flags) ? RADAR_DETECT_MODE_EU:RADAR_DETECT_MODE_FCC;
		wlc_phy_radar_detect_mode_set(wlc->band->pi, mode);
	}
#endif /* AP && RADAR */

	wlc_channels_init(wlc_cm, country);

	/* Country code changed */
	if (strlen(prev_country_abbrev) > 1 &&
	    strncmp(wlc_cm->country_abbrev, prev_country_abbrev,
	            strlen(wlc_cm->country_abbrev)) != 0) {
		/* need to reset afe_override */
		wlc_channel_spurwar_locale(wlc_cm, wlc->chanspec);

		wlc_mac_event(wlc, WLC_E_COUNTRY_CODE_CHANGED, NULL,
		              0, 0, 0, wlc_cm->country_abbrev, strlen(wlc_cm->country_abbrev) + 1);
	}

	return;
}

#ifdef RADAR
extern bool
wlc_is_european_weather_radar_channel(struct wlc_info *wlc, chanspec_t chanspec)
{
	clm_result_t result;
	clm_country_locales_t locale;
	clm_country_t country;
	uint8 flags;

	if (!((NBANDS(wlc) == 2) || IS_SINGLEBAND_5G(wlc->deviceid)))
		return FALSE;

	result = wlc_country_lookup_direct(wlc->cmi->ccode, wlc->cmi->regrev, &country);
	if (result != CLM_RESULT_OK)
		return FALSE;

	result = wlc_get_locale(country, &locale);
	if (result != CLM_RESULT_OK)
		return FALSE;

	result = wlc_get_flags(&locale, CLM_BAND_5G, &flags);
	if (result != CLM_RESULT_OK)
		return FALSE;

	if (!ISDFS_EU(flags))
		return FALSE;

	if (CHSPEC_IS80(chanspec)) {
		if ((chanspec == CH80MHZ_CHSPEC(122, WL_CHANSPEC_CTL_SB_LL)) || /* 116-80 */
			(chanspec == CH80MHZ_CHSPEC(122, WL_CHANSPEC_CTL_SB_LU)) || /* 120-80 */
			(chanspec == CH80MHZ_CHSPEC(122, WL_CHANSPEC_CTL_SB_UL)) || /* 124-80 */
			(chanspec == CH80MHZ_CHSPEC(122, WL_CHANSPEC_CTL_SB_UU))) /* 128-80 */
			{
				return TRUE;
			}
	}
	else if (CHSPEC_IS40(chanspec)) {
		if ((chanspec == CH40MHZ_CHSPEC(118, WL_CHANSPEC_CTL_SB_LOWER)) || /* 118l */
			(chanspec == CH40MHZ_CHSPEC(118, WL_CHANSPEC_CTL_SB_UPPER)) || /* 118u */
			(chanspec == CH40MHZ_CHSPEC(126, WL_CHANSPEC_CTL_SB_LOWER)) || /* 126l */
			(chanspec == CH40MHZ_CHSPEC(126, WL_CHANSPEC_CTL_SB_UPPER))) /* 126u */
			{
				return TRUE;
			}
	}
	else if (CHSPEC_IS20(chanspec)) {
		if ((CHSPEC_CHANNEL(chanspec) == 120) ||
			(CHSPEC_CHANNEL(chanspec) == 124) ||
			(CHSPEC_CHANNEL(chanspec) == 128))
			{
				return TRUE;
			}
	}
	return FALSE;
}
#endif /* RADAR */

/* Lookup a country info structure from a null terminated country code
 * The lookup is case sensitive.
 */
clm_result_t
wlc_country_lookup(struct wlc_info *wlc, const char* ccode, clm_country_t *country)
{
	clm_result_t result = CLM_RESULT_ERR;
	char mapped_ccode[WLC_CNTRY_BUF_SZ];
	uint mapped_regrev;

	WL_NONE(("wl%d: %s: ccode \"%s\", SPROM \"%s\"/%u\n",
	         wlc->pub->unit, __FUNCTION__, ccode, wlc->cmi->srom_ccode, wlc->cmi->srom_regrev));

	/* map the country code to a built-in country code, regrev, and country_info struct */
	result = wlc_countrycode_map(wlc->cmi, ccode, mapped_ccode, &mapped_regrev, country);

	if (result == CLM_RESULT_OK)
		WL_NONE(("wl%d: %s: mapped to \"%s\"/%u\n",
		         wlc->pub->unit, __FUNCTION__, mapped_ccode, mapped_regrev));
	else
		WL_NONE(("wl%d: %s: failed lookup\n",
		         wlc->pub->unit, __FUNCTION__));

	return result;
}

static clm_result_t
wlc_countrycode_map(wlc_cm_info_t *wlc_cm, const char *ccode,
	char *mapped_ccode, uint *mapped_regrev, clm_country_t *country)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	clm_result_t result = CLM_RESULT_ERR;
	uint srom_regrev = wlc_cm->srom_regrev;
	const char *srom_ccode = wlc_cm->srom_ccode;
	int mapped;

	/* check for currently supported ccode size */
	if (strlen(ccode) > (WLC_CNTRY_BUF_SZ - 1)) {
		WL_ERROR(("wl%d: %s: ccode \"%s\" too long for match\n",
		          wlc->pub->unit, __FUNCTION__, ccode));
		return CLM_RESULT_ERR;
	}

	/* default mapping is the given ccode and regrev 0 */
	strncpy(mapped_ccode, ccode, WLC_CNTRY_BUF_SZ);
	*mapped_regrev = 0;

	/* Map locale for buffalo if needed */
	if (wlc_buffalo_map_locale(wlc, ccode)) {
		strncpy(mapped_ccode, "J10", WLC_CNTRY_BUF_SZ);
		result = wlc_country_lookup_direct(mapped_ccode, *mapped_regrev, country);
		if (result == CLM_RESULT_OK)
			return result;
	}

	/* If the desired country code matches the srom country code,
	 * then the mapped country is the srom regulatory rev.
	 * Otherwise look for an aggregate mapping.
	 */
	if (!strcmp(srom_ccode, ccode)) {
		WL_NONE(("wl%d: %s: srom ccode and ccode \"%s\" match\n",
		         wlc->pub->unit, __FUNCTION__, ccode));
		*mapped_regrev = srom_regrev;
		mapped = 0;
	} else {
		mapped = wlc_country_aggregate_map(wlc_cm, ccode, mapped_ccode, mapped_regrev);
		if (mapped)
			WL_NONE(("wl%d: %s: found aggregate mapping \"%s\"/%u\n",
			         wlc->pub->unit, __FUNCTION__, mapped_ccode, *mapped_regrev));
	}

	/* CLM 8.2, JAPAN
	 * Use the regrev=1 Japan country definition by default for chips newer than
	 * our d11 core rev 5 4306 chips, or for AP's of any vintage.
	 * For STAs with a 4306, use the legacy Japan country def "JP/0".
	 * Only do the conversion if JP/0 was not specified by a mapping or by an
	 * sprom with a regrev:
	 * Sprom has no specific info about JP's regrev if it's less than rev 3 or it does
	 * not specify "JP" as its country code =>
	 * (strcmp("JP", srom_ccode) || (wlc->pub->sromrev < 3))
	 */
	if (!strcmp("JP", mapped_ccode) && *mapped_regrev == 0 &&
	    !mapped && (strcmp("JP", srom_ccode) || (wlc->pub->sromrev < 3)) &&
	    (AP_ENAB(wlc->pub) || D11REV_GT(wlc->pub->corerev, 5))) {
		*mapped_regrev = 1;
		WL_NONE(("wl%d: %s: Using \"JP/1\" instead of legacy \"JP/0\" since we %s\n",
		         wlc->pub->unit, __FUNCTION__,
		         AP_ENAB(wlc->pub) ? "are operating as an AP" : "are newer than 4306"));
	}

	WL_NONE(("wl%d: %s: searching for country using ccode/rev \"%s\"/%u\n",
	         wlc->pub->unit, __FUNCTION__, mapped_ccode, *mapped_regrev));

	/* find the matching built-in country definition */
	result = wlc_country_lookup_direct(mapped_ccode, *mapped_regrev, country);

	/* if there is not an exact rev match, default to rev zero */
	if (result != CLM_RESULT_OK && *mapped_regrev != 0) {
		*mapped_regrev = 0;
		WL_NONE(("wl%d: %s: No country found, use base revision \"%s\"/%u\n",
		         wlc->pub->unit, __FUNCTION__, mapped_ccode, *mapped_regrev));
		result = wlc_country_lookup_direct(mapped_ccode, *mapped_regrev, country);
	}

	if (result != CLM_RESULT_OK)
		WL_NONE(("wl%d: %s: No country found, failed lookup\n",
		         wlc->pub->unit, __FUNCTION__));

	return result;
}

clm_result_t
clm_aggregate_country_lookup(const ccode_t cc, unsigned int rev, clm_agg_country_t *agg)
{
	return clm_agg_country_lookup(cc, rev, agg);
}

clm_result_t
clm_aggregate_country_map_lookup(const clm_agg_country_t agg, const ccode_t target_cc,
	unsigned int *rev)
{
	return clm_agg_country_map_lookup(agg, target_cc, rev);
}

static int
wlc_country_aggregate_map(wlc_cm_info_t *wlc_cm, const char *ccode,
                          char *mapped_ccode, uint *mapped_regrev)
{
	clm_result_t result;
	clm_agg_country_t agg = 0;
	const char *srom_ccode = wlc_cm->srom_ccode;
	uint srom_regrev = (uint8)wlc_cm->srom_regrev;

	/* Use "ww", WorldWide, for the lookup value for '\0\0' */
	if (srom_ccode[0] == '\0')
		srom_ccode = "ww";

	/* Check for a match in the aggregate country list */
	WL_NONE(("wl%d: %s: searching for agg map for srom ccode/rev \"%s\"/%u\n",
	         wlc->pub->unit, __FUNCTION__, srom_ccode, srom_regrev));

	result = clm_aggregate_country_lookup(srom_ccode, srom_regrev, &agg);

	if (result != CLM_RESULT_OK)
		WL_NONE(("wl%d: %s: no map for \"%s\"/%u\n",
		         wlc->pub->unit, __FUNCTION__, srom_ccode, srom_regrev));
	else
		WL_NONE(("wl%d: %s: found map for \"%s\"/%u\n",
		         wlc->pub->unit, __FUNCTION__, srom_ccode, srom_regrev));


	if (result == CLM_RESULT_OK) {
		result = clm_aggregate_country_map_lookup(agg, ccode, mapped_regrev);
		strncpy(mapped_ccode, ccode, WLC_CNTRY_BUF_SZ);
	}

	return (result == CLM_RESULT_OK);
}




/* Lookup a country info structure from a null terminated country
 * abbreviation and regrev directly with no translation.
 */
clm_result_t
wlc_country_lookup_direct(const char* ccode, uint regrev, clm_country_t *country)
{
	return clm_country_lookup(ccode, regrev, country);
}


#if defined(STA) && defined(WL11D)
/* Lookup a country info structure considering only legal country codes as found in
 * a Country IE; two ascii alpha followed by " ", "I", or "O".
 * Do not match any user assigned application specifc codes that might be found
 * in the driver table.
 */
clm_result_t
wlc_country_lookup_ext(wlc_info_t *wlc, const char *ccode, clm_country_t *country)
{
	clm_result_t result = CLM_RESULT_NOT_FOUND;
	char country_str_lookup[WLC_CNTRY_BUF_SZ] = { 0 };

	/* only allow ascii alpha uppercase for the first 2 chars, and " ", "I", "O" for the 3rd */
	if (!((0x80 & ccode[0]) == 0 && bcm_isupper(ccode[0]) &&
	      (0x80 & ccode[1]) == 0 && bcm_isupper(ccode[1]) &&
	      (ccode[2] == ' ' || ccode[2] == 'I' || ccode[2] == 'O')))
		return result;

	/* for lookup in the driver table of country codes, only use the first
	 * 2 chars, ignore the 3rd character " ", "I", "O" qualifier
	 */
	country_str_lookup[0] = ccode[0];
	country_str_lookup[1] = ccode[1];

	/* do not match ISO 3166-1 user assigned country codes that may be in the driver table */
	if (!strcmp("AA", country_str_lookup) ||	/* AA */
	    !strcmp("ZZ", country_str_lookup) ||	/* ZZ */
	    country_str_lookup[0] == 'X' ||		/* XA - XZ */
	    (country_str_lookup[0] == 'Q' &&		/* QM - QZ */
	     (country_str_lookup[1] >= 'M' && country_str_lookup[1] <= 'Z')))
		return result;


	return wlc_country_lookup(wlc, country_str_lookup, country);
}
#endif /* STA && WL11D */

static int
wlc_channels_init(wlc_cm_info_t *wlc_cm, clm_country_t country)
{
	clm_country_locales_t locale;
	uint8 flags;
	wlc_info_t *wlc = wlc_cm->wlc;
	uint i, j;
	wlcband_t * band;
	chanvec_t sup_chan, temp_chan;

	if (wlc->dfs)
		wlc_dfs_reset_all(wlc->dfs);

	bzero(&wlc_cm->restricted_channels, sizeof(chanvec_t));
	bzero(&wlc_cm->locale_radar_channels, sizeof(chanvec_t));
	bzero(&wlc_cm->allowed_5g_20channels, sizeof(chanvec_t));
	bzero(&wlc_cm->allowed_5g_4080channels, sizeof(chanvec_t));

	band = wlc->band;
	for (i = 0; i < NBANDS(wlc); i++, band = wlc->bandstate[OTHERBANDUNIT(wlc)]) {
		clm_result_t result = wlc_get_locale(country, &locale);
		clm_band_t tmp_band;
		if (result == CLM_RESULT_OK) {
			if (BAND_5G(band->bandtype)) {
				tmp_band = CLM_BAND_5G;
			} else {
				tmp_band = CLM_BAND_2G;
			}
			result = wlc_get_flags(&locale, tmp_band, &flags);
			wlc_cm->bandstate[band->bandunit].locale_flags = flags;

			wlc_locale_get_channels(&locale, tmp_band,
				&wlc_cm->bandstate[band->bandunit].valid_channels,
				&temp_chan);
			/* initialize restricted channels */
			for (j = 0; j < sizeof(chanvec_t); j++) {
				wlc_cm->restricted_channels.vec[j] |= temp_chan.vec[j];
			}
#ifdef BAND5G /* RADAR */
			wlc_cm->bandstate[band->bandunit].radar_channels =
				&wlc_cm->locale_radar_channels;
			if (BAND_5G(band->bandtype) && (flags & WLC_DFS_TPC)) {
				for (j = 0; j < sizeof(chanvec_t); j++) {
					wlc_cm->bandstate[band->bandunit].radar_channels->vec[j] =
						radar_set1.vec[j] &
						wlc_cm->bandstate[band->bandunit].
						valid_channels.vec[j];
				}
			}
			if (BAND_5G(band->bandtype)) {
			  clm_valid_channels_5g(&locale,
			  (clm_channels_t*)&wlc_cm->allowed_5g_20channels,
			  (clm_channels_t*)&wlc_cm->allowed_5g_4080channels);
			 }
#endif /* BAND5G */

			/* set the channel availability,
			 * masking out the channels that may not be supported on this phy
			 */
			wlc_phy_chanspec_band_validch(band->pi, band->bandtype, &sup_chan);
			wlc_locale_get_channels(&locale, tmp_band,
				&wlc_cm->bandstate[band->bandunit].valid_channels,
				&temp_chan);
			for (j = 0; j < sizeof(chanvec_t); j++)
				wlc_cm->bandstate[band->bandunit].valid_channels.vec[j] &=
					sup_chan.vec[j];
		}
	}

	wlc_upd_restricted_chanspec_flag(wlc_cm);
	wlc_quiet_channels_reset(wlc_cm);
	wlc_channels_commit(wlc_cm);
	wlc_rcinfo_init(wlc_cm);
	wlc_regclass_vec_init(wlc_cm);

	return (0);
}

/* Update the radio state (enable/disable) and tx power targets
 * based on a new set of channel/regulatory information
 */
static void
wlc_channels_commit(wlc_cm_info_t *wlc_cm)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	uint chan;
	ppr_t* txpwr;
	/* search for the existence of any valid channel */
	for (chan = 0; chan < MAXCHANNEL; chan++) {
		if (VALID_CHANNEL20_DB(wlc, chan)) {
			break;
		}
	}
	if (chan == MAXCHANNEL)
		chan = INVCHANNEL;

	/* based on the channel search above, set or clear WL_RADIO_COUNTRY_DISABLE */
	if (chan == INVCHANNEL) {
		/* country/locale with no valid channels, set the radio disable bit */
		mboolset(wlc->pub->radio_disabled, WL_RADIO_COUNTRY_DISABLE);
		WL_ERROR(("wl%d: %s: no valid channel for \"%s\" nbands %d bandlocked %d\n",
		          wlc->pub->unit, __FUNCTION__,
		          wlc_cm->country_abbrev, NBANDS(wlc), wlc->bandlocked));
	} else if (mboolisset(wlc->pub->radio_disabled, WL_RADIO_COUNTRY_DISABLE)) {
		/* country/locale with valid channel, clear the radio disable bit */
		mboolclr(wlc->pub->radio_disabled, WL_RADIO_COUNTRY_DISABLE);
	}

	/* Now that the country abbreviation is set, if the radio supports 2G, then
	 * set channel 14 restrictions based on the new locale.
	 */
	if (NBANDS(wlc) > 1 || BAND_2G(wlc->band->bandtype)) {
		wlc_phy_chanspec_ch14_widefilter_set(wlc->band->pi, wlc_japan(wlc) ? TRUE : FALSE);
	}

	if (wlc->pub->up && chan != INVCHANNEL) {
		/* recompute tx power for new country info */


		/* Where do we get a good chanspec? wlc, phy, set it ourselves? */

		if ((txpwr = ppr_create(wlc_cm->pub->osh, PPR_CHSPEC_BW(wlc->chanspec))) == NULL) {
			return;
		}

		wlc_channel_reg_limits(wlc_cm, wlc->chanspec, txpwr);

		ppr_apply_max(txpwr, WLC_TXPWR_MAX);
		/* Where do we get a good chanspec? wlc, phy, set it ourselves? */
		wlc_phy_txpower_limit_set(wlc->band->pi, txpwr, wlc->chanspec);

		ppr_delete(wlc_cm->pub->osh, txpwr);
	}
}
chanvec_t *
wlc_quiet_chanvec_get(wlc_cm_info_t *wlc_cm)
{
	return &wlc_cm->quiet_channels;
}

chanvec_t *
wlc_valid_chanvec_get(wlc_cm_info_t *wlc_cm, uint bandunit)
{
	return &wlc_cm->bandstate[bandunit].valid_channels;
}

/* reset the quiet channels vector to the union of the restricted and radar channel sets */
void
wlc_quiet_channels_reset(wlc_cm_info_t *wlc_cm)
{
#ifdef BAND5G
	wlc_info_t *wlc = wlc_cm->wlc;
	uint i;
	wlcband_t *band;
#endif /* BAND5G */

	/* initialize quiet channels for restricted channels */
	bcopy(&wlc_cm->restricted_channels, &wlc_cm->quiet_channels, sizeof(chanvec_t));

#ifdef BAND5G /* RADAR */
	band = wlc->band;
	for (i = 0; i < NBANDS(wlc); i++, band = wlc->bandstate[OTHERBANDUNIT(wlc)]) {
		/* initialize quiet channels for radar if we are in spectrum management mode */
		if (WL11H_ENAB(wlc)) {
			uint j;
			const chanvec_t *chanvec;

			chanvec = wlc_cm->bandstate[band->bandunit].radar_channels;
			for (j = 0; j < sizeof(chanvec_t); j++)
				wlc_cm->quiet_channels.vec[j] |= chanvec->vec[j];
		}
	}
#endif /* BAND5G */
}

bool
wlc_quiet_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec)
{
	wlc_pub_t *pub = wlc_cm->wlc->pub;
	uint8 channel = CHSPEC_CHANNEL(chspec);

	if (VHT_ENAB(pub) && CHSPEC_IS80(chspec)) {
		return (
		isset(wlc_cm->quiet_channels.vec, LL_20_SB(channel)) ||
		isset(wlc_cm->quiet_channels.vec, LU_20_SB(channel)) ||
		isset(wlc_cm->quiet_channels.vec, UL_20_SB(channel)) ||
		isset(wlc_cm->quiet_channels.vec, UU_20_SB(chspec)));
	} else if ((VHT_ENAB(pub) || N_ENAB(pub)) && CHSPEC_IS40(chspec)) {
		return (
		isset(wlc_cm->quiet_channels.vec, LOWER_20_SB(channel)) ||
		isset(wlc_cm->quiet_channels.vec, UPPER_20_SB(channel)));
	} else {
		return isset(wlc_cm->quiet_channels.vec, channel);
	}
}

void
wlc_set_quiet_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec)
{
	wlc_pub_t *pub = wlc_cm->wlc->pub;
	uint8 channel = CHSPEC_CHANNEL(chspec);

	if (VHT_ENAB(pub) && CHSPEC_IS80(chspec)) {
		setbit(wlc_cm->quiet_channels.vec, LL_20_SB(channel));
		setbit(wlc_cm->quiet_channels.vec, LU_20_SB(channel));
		setbit(wlc_cm->quiet_channels.vec, UL_20_SB(channel));
		setbit(wlc_cm->quiet_channels.vec, UU_20_SB(channel));
	} else if ((VHT_ENAB(pub) || N_ENAB(pub)) && CHSPEC_IS40(chspec)) {
		setbit(wlc_cm->quiet_channels.vec, LOWER_20_SB(channel));
		setbit(wlc_cm->quiet_channels.vec, UPPER_20_SB(channel));
	} else {
		setbit(wlc_cm->quiet_channels.vec, channel);
	}
}

void
wlc_clr_quiet_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec)
{
	wlc_pub_t *pub = wlc_cm->wlc->pub;
	uint8 channel = CHSPEC_CHANNEL(chspec);

	if (VHT_ENAB(pub) && CHSPEC_IS80(chspec)) {
		clrbit(wlc_cm->quiet_channels.vec, LL_20_SB(channel));
		clrbit(wlc_cm->quiet_channels.vec, LU_20_SB(channel));
		clrbit(wlc_cm->quiet_channels.vec, UL_20_SB(channel));
		clrbit(wlc_cm->quiet_channels.vec, UU_20_SB(channel));
	} else if ((VHT_ENAB(pub) || N_ENAB(pub)) && CHSPEC_IS40(chspec)) {
		clrbit(wlc_cm->quiet_channels.vec, LOWER_20_SB(channel));
		clrbit(wlc_cm->quiet_channels.vec, UPPER_20_SB(channel));
	} else {
		clrbit(wlc_cm->quiet_channels.vec, channel);
	}
}

/* Is the channel valid for the current locale? (but don't consider channels not
 *   available due to bandlocking)
 */
bool
wlc_valid_channel20_db(wlc_cm_info_t *wlc_cm, uint val)
{
	wlc_info_t *wlc = wlc_cm->wlc;

	return (VALID_CHANNEL20(wlc, val) ||
		(!wlc->bandlocked && VALID_CHANNEL20_IN_BAND(wlc, OTHERBANDUNIT(wlc), val)));
}

/* Is the channel valid for the current locale and specified band? */
bool
wlc_valid_channel20_in_band(wlc_cm_info_t *wlc_cm, uint bandunit, uint val)
{
	return ((val < MAXCHANNEL) && isset(wlc_cm->bandstate[bandunit].valid_channels.vec, val));
}

/* Is the channel valid for the current locale and current band? */
bool
wlc_valid_channel20(wlc_cm_info_t *wlc_cm, uint val)
{
	wlc_info_t *wlc = wlc_cm->wlc;

	return ((val < MAXCHANNEL) &&
		isset(wlc_cm->bandstate[wlc->band->bandunit].valid_channels.vec, val));
}

/* Is the 40 MHz allowed for the current locale and specified band? */
bool
wlc_valid_40chanspec_in_band(wlc_cm_info_t *wlc_cm, uint bandunit)
{
	wlc_info_t *wlc = wlc_cm->wlc;

	return (((wlc_cm->bandstate[bandunit].locale_flags & (WLC_NO_MIMO | WLC_NO_40MHZ)) == 0) &&
	        WL_BW_CAP_40MHZ(wlc->bandstate[bandunit]->bw_cap));
}

/* Is 80 MHz allowed for the current locale and specified band? */
bool
wlc_valid_80chanspec_in_band(wlc_cm_info_t *wlc_cm, uint bandunit)
{
	wlc_info_t *wlc = wlc_cm->wlc;

	return (((wlc_cm->bandstate[bandunit].locale_flags & (WLC_NO_MIMO | WLC_NO_80MHZ)) == 0) &&
	        WL_BW_CAP_80MHZ(wlc->bandstate[bandunit]->bw_cap));
}

/* Is chanspec allowed to use 80Mhz bandwidth for the current locale? */
bool
wlc_valid_80chanspec(struct wlc_info *wlc, chanspec_t chanspec)
{
	uint8 locale_flags;

	locale_flags = wlc_channel_locale_flags_in_band((wlc)->cmi, CHSPEC_WLCBANDUNIT(chanspec));

	return (VHT_ENAB_BAND((wlc)->pub, (CHSPEC2WLC_BAND(chanspec))) &&
	 !(locale_flags & WLC_NO_80MHZ) &&
	 WL_BW_CAP_80MHZ((wlc)->bandstate[BAND_5G_INDEX]->bw_cap) &&
	 wlc_valid_chanspec_db((wlc)->cmi, (chanspec)));
}


static void
wlc_channel_txpower_limits(wlc_cm_info_t *wlc_cm, ppr_t *txpwr)
{
	uint8 local_constraint;
	wlc_info_t *wlc = wlc_cm->wlc;

	local_constraint = wlc_tpc_get_local_constraint_qdbm(wlc->tpc);

	wlc_channel_reg_limits(wlc_cm, wlc->chanspec, txpwr);

	ppr_apply_constraint_total_tx(txpwr, local_constraint);
}

static void
wlc_channel_spurwar_locale(wlc_cm_info_t *wlc_cm, chanspec_t chanspec)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	int override;
	uint rev;
	bool isCN, isX2, isX51A;

	isX51A = (wlc->pub->sih->boardtype == BCM94360X51A) ? TRUE : FALSE;
	isCN = bcmp("CN", wlc_channel_country_abbrev(wlc->cmi), 2) ? FALSE : TRUE;
	isX2 = bcmp("X2", wlc_channel_country_abbrev(wlc->cmi), 2) ? FALSE : TRUE;
	rev = wlc_channel_regrev(wlc->cmi);

	wlc_iovar_getint(wlc, "phy_afeoverride", &override);
	override &= ~PHY_AFE_OVERRIDE_DRV;
	if (CHSPEC_IS5G(chanspec) && isX51A && ((isCN && rev == 40) || (isX2 && rev == 19)))
		override |= PHY_AFE_OVERRIDE_DRV;

	wlc_iovar_setint(wlc, "phy_afeoverride", override);
	if (CHSPEC_IS20(chanspec))
		wlc->stf->coremask_override = override ? TRUE : FALSE;
	else
		wlc->stf->coremask_override = FALSE;
}


void
wlc_channel_set_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chanspec, uint8 local_constraint_qdbm)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	ppr_t* txpwr;

#ifdef WLTXPWR_CACHE
	tx_pwr_cache_entry_t* cacheptr = wlc_phy_get_txpwr_cache(wlc->band->pi);

	if (wlc_phy_txpwr_cache_is_cached(cacheptr, chanspec) != TRUE) {
		int result;
		chanspec_t kill_chan = 0;

		BCM_REFERENCE(result);

		if (last_chanspec != 0)
			kill_chan = wlc_phy_txpwr_cache_find_other_cached_chanspec(cacheptr,
				last_chanspec);

		if (kill_chan != 0) {
#ifndef WLC_LOW
			ppr_t* txpwr_offsets;
			txpwr_offsets = wlc_phy_get_cached_pwr(cacheptr, kill_chan,
				TXPWR_CACHE_STF_OFFSETS);
			/* allow stf to delete its offsets */
			if ((txpwr_offsets != NULL) && (txpwr_offsets == wlc->stf->txpwr_ctl)) {
				wlc_phy_uncache_pwr(cacheptr, TXPWR_CACHE_STF_OFFSETS,
					txpwr_offsets);
			}
#else
			ppr_t* txpwr_offsets;
			/* for dualband chips, we have to set pi->tx_power_offset to NULL before
			 * delete the memory in cache. So released tx_power_offset won't
			 * be referenced during band switching
			 */
			txpwr_offsets = wlc_phy_get_cached_pwr(cacheptr, kill_chan,
				TXPWR_CACHE_POWER_OFFSETS);
			if (NBANDS(wlc) > 1)
				wlc_bmac_clear_band_pwr_offset(txpwr_offsets, wlc->hw);
#endif /* !WLC_LOW */
			wlc_phy_txpwr_cache_clear(wlc_cm->pub->osh, cacheptr, kill_chan);
		}
		result = wlc_phy_txpwr_setup_entry(cacheptr, chanspec);
		ASSERT(result == BCME_OK);
	}

	last_chanspec = chanspec;

	if ((wlc_phy_get_cached_txchain_offsets(cacheptr, chanspec, 0) != WL_RATE_DISABLED) &&
#ifndef WLTXPWR_CACHE_PHY_ONLY
		(wlc_phy_get_cached_pwr(cacheptr, chanspec, TXPWR_CACHE_POWER_OFFSETS) != NULL) &&
#endif
		TRUE) {
		wlc_channel_update_txchain_offsets(wlc_cm, NULL);
		wlc_bmac_set_chanspec(wlc->hw, chanspec,
			(wlc_quiet_chanspec(wlc_cm, chanspec) != 0), NULL);
		return;
	}
#endif /* WLTXPWR_CACHE */

	WL_CHANLOG(wlc, __FUNCTION__, TS_ENTER, 0);

	if ((txpwr = ppr_create(wlc_cm->pub->osh, PPR_CHSPEC_BW(chanspec))) == NULL) {
		WL_CHANLOG(wlc, __FUNCTION__, TS_EXIT, 0);
		return;
	}
#ifdef SRHWVSDB
	/* If txpwr shms are saved, no need to go through power init fns again */
#ifdef WLMCHAN
	if ((MCHAN_ENAB(wlc->pub) && MCHAN_ACTIVE(wlc->pub)) ||
		wlc->srvsdb_info->iovar_force_vsdb) {
#else
	if (wlc->srvsdb_info->iovar_force_vsdb) {
#endif /* WLMCHAN */
		/* Check if power offsets are already saved */
		uint8 offset = (chanspec == wlc->srvsdb_info->vsdb_chans[0]) ? 0 : 1;

		if (wlc->srvsdb_info->vsdb_save_valid[offset]) {
			goto apply_chanspec;
		}
	}
#endif /* SRHWVSDB */


	wlc_channel_reg_limits(wlc_cm, chanspec, txpwr);

	ppr_apply_constraint_total_tx(txpwr, local_constraint_qdbm);

	wlc_channel_update_txchain_offsets(wlc_cm, txpwr);

	WL_CHANLOG(wlc, "After wlc_channel_update_txchain_offsets", 0, 0);

#ifdef SRHWVSDB
apply_chanspec:
#endif /* SRHWVSDB */

	wlc_bmac_set_chanspec(wlc->hw, chanspec, (wlc_quiet_chanspec(wlc_cm, chanspec) != 0),
		txpwr);

	ppr_delete(wlc_cm->pub->osh, txpwr);
	WL_CHANLOG(wlc, __FUNCTION__, TS_EXIT, 0);
}

int
wlc_channel_set_txpower_limit(wlc_cm_info_t *wlc_cm, uint8 local_constraint_qdbm)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	ppr_t *txpwr;



	if ((txpwr = ppr_create(wlc_cm->pub->osh, PPR_CHSPEC_BW(wlc->chanspec))) == NULL) {
		return BCME_ERROR;
	}

	wlc_channel_reg_limits(wlc_cm, wlc->chanspec, txpwr);

#ifdef WLTXPWR_CACHE
	wlc_phy_txpwr_cache_invalidate(wlc_phy_get_txpwr_cache(wlc->band->pi));
#endif	/* WLTXPWR_CACHE */

	ppr_apply_constraint_total_tx(txpwr, local_constraint_qdbm);

	wlc_channel_update_txchain_offsets(wlc_cm, txpwr);

	wlc_phy_txpower_limit_set(wlc->band->pi, txpwr, wlc->chanspec);

	ppr_delete(wlc_cm->pub->osh, txpwr);
	return 0;
}


#ifdef WL11AC
static clm_limits_type_t clm_sideband_to_limits_type(uint sb)
{
	clm_limits_type_t lt = CLM_LIMITS_TYPE_CHANNEL;

	switch (sb) {
	case WL_CHANSPEC_CTL_SB_LL:
		lt = CLM_LIMITS_TYPE_SUBCHAN_LL;
		break;
	case WL_CHANSPEC_CTL_SB_LU:
		lt = CLM_LIMITS_TYPE_SUBCHAN_LU;
		break;
	case WL_CHANSPEC_CTL_SB_UL:
		lt = CLM_LIMITS_TYPE_SUBCHAN_UL;
		break;
	case WL_CHANSPEC_CTL_SB_UU:
		lt = CLM_LIMITS_TYPE_SUBCHAN_UU;
		break;
	default:
		break;
	}
	return lt;
}
#endif /* WL11AC */

#ifdef WL_SARLIMIT
#define MAXNUM_SAR_ENTRY	(sizeof(sar_default)/sizeof(sar_default[0]))
const struct {
	uint16	boardtype;
	sar_limit_t sar;
} sar_default[] = {
	{BCM94331X29B, {{QDB(17)+2, QDB(16), QDB(17)+2, WLC_TXPWR_MAX}, /* 2g SAR limits */
	{{QDB(14)+2, QDB(14)+2, QDB(15)+2, WLC_TXPWR_MAX}, /* 5g subband 0 SAR limits */
	{QDB(15), QDB(14)+2, QDB(15), WLC_TXPWR_MAX}, /* 5g subband 1 SAR limits */
	{QDB(17), QDB(15), QDB(17), WLC_TXPWR_MAX}, /* 5g subband 2 SAR limits */
	{QDB(18), QDB(15), QDB(18), WLC_TXPWR_MAX}  /* 5g subband 3 SAR limits */
	}}},
	{BCM94331X29D, {{QDB(18), QDB(18), QDB(18), WLC_TXPWR_MAX}, /* 2g SAR limits */
	{{QDB(16)+2, QDB(16)+2, QDB(16)+2, WLC_TXPWR_MAX}, /* 5g subband 0 SAR limits */
	{QDB(16), QDB(17), QDB(17), WLC_TXPWR_MAX}, /* 5g subband 1 SAR limits */
	{QDB(16)+2, QDB(16)+2, QDB(16)+2, WLC_TXPWR_MAX}, /* 5g subband 2 SAR limits */
	{QDB(17)+2, QDB(16)+2, QDB(17)+2, WLC_TXPWR_MAX}  /* 5g subband 3 SAR limits */
	}}},
	{BCM94360X29C, {{QDB(17)+2, QDB(16)+1, QDB(16)+2, WLC_TXPWR_MAX}, /* 2g SAR limits */
	{{QDB(15)+2, QDB(15)+2, QDB(15)+2, WLC_TXPWR_MAX}, /* 5g subband 0 SAR limits */
	{QDB(15)+2, QDB(15)+2, QDB(15)+2, WLC_TXPWR_MAX}, /* 5g subband 1 SAR limits */
	{QDB(15)+2, QDB(15)+2, QDB(15)+2, WLC_TXPWR_MAX}, /* 5g subband 2 SAR limits */
	{QDB(16)+1, QDB(16)+1, QDB(16)+2, WLC_TXPWR_MAX}  /* 5g subband 3 SAR limits */
	}}},
	{BCM94360X29CP2, {{QDB(16), QDB(17)+1, QDB(17)+2, WLC_TXPWR_MAX}, /* 2g SAR limits */
	{{QDB(15)+2, QDB(16)+2, QDB(16)+2, WLC_TXPWR_MAX}, /* 5g subband 0 SAR limits */
	{QDB(15), QDB(16)+2, QDB(18), WLC_TXPWR_MAX}, /* 5g subband 1 SAR limits */
	{QDB(15)+2, QDB(16)+3, QDB(17)+2, WLC_TXPWR_MAX}, /* 5g subband 2 SAR limits */
	 {QDB(16)+2, QDB(18)+2, QDB(18)+2, WLC_TXPWR_MAX}  /* 5g subband 3 SAR limits */
	}}},
	{BCM94360X52C, {{QDB(19), QDB(20), WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 2g SAR limits */
	{{QDB(16), QDB(15)+2, WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 5g subband 0 SAR limits */
	{QDB(17)+2, QDB(17)+2, WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 5g subband 1 SAR limits */
	{QDB(17), QDB(18), WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 5g subband 2 SAR limits */
	{QDB(17)+2, QDB(18), WLC_TXPWR_MAX, WLC_TXPWR_MAX}  /* 5g subband 3 SAR limits */
	}}},
	{BCM94360X52D, {{QDB(19), QDB(20), WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 2g SAR limits */
	{{QDB(16), QDB(15)+2, WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 5g subband 0 SAR limits */
	{QDB(17)+2, QDB(17)+2, WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 5g subband 1 SAR limits */
	{QDB(17), QDB(18), WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 5g subband 2 SAR limits */
	{QDB(17)+2, QDB(18), WLC_TXPWR_MAX, WLC_TXPWR_MAX}  /* 5g subband 3 SAR limits */
	}}},
	{BCM943602X87, {{QDB(17)+2, QDB(16)+1, QDB(16)+2, WLC_TXPWR_MAX}, /* 2g SAR limits */
	{{QDB(15)+2, QDB(15)+2, QDB(15)+2, WLC_TXPWR_MAX}, /* 5g subband 0 SAR limits */
	{QDB(15)+2, QDB(15)+2, QDB(15)+2, WLC_TXPWR_MAX}, /* 5g subband 1 SAR limits */
	{QDB(15)+2, QDB(15)+2, QDB(15)+2, WLC_TXPWR_MAX}, /* 5g subband 2 SAR limits */
	{QDB(16)+1, QDB(16)+1, QDB(16)+2, WLC_TXPWR_MAX}  /* 5g subband 3 SAR limits */
	}}},
	{BCM94350X14, {{QDB(19), QDB(20), WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 2g SAR limits */
	{{QDB(16), QDB(15)+2, WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 5g subband 0 SAR limits */
	{QDB(17)+2, QDB(17)+2, WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 5g subband 1 SAR limits */
	{QDB(17), QDB(18), WLC_TXPWR_MAX, WLC_TXPWR_MAX}, /* 5g subband 2 SAR limits */
	{QDB(17)+2, QDB(18), WLC_TXPWR_MAX, WLC_TXPWR_MAX}  /* 5g subband 3 SAR limits */
	}}},
};

static void
wlc_channel_sarlimit_get_default(wlc_cm_info_t *wlc_cm, sar_limit_t *sar)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	uint idx;

	for (idx = 0; idx < MAXNUM_SAR_ENTRY; idx++) {
		if (sar_default[idx].boardtype == wlc->pub->sih->boardtype) {
			memcpy((uint8 *)sar, (uint8 *)&(sar_default[idx].sar), sizeof(sar_limit_t));
			break;
		}
	}
}

void
wlc_channel_sar_init(wlc_cm_info_t *wlc_cm)
{
	wlc_info_t *wlc = wlc_cm->wlc;

	memset((uint8 *)wlc_cm->sarlimit.band2g,
	       wlc->bandstate[BAND_2G_INDEX]->sar,
	       WLC_TXCORE_MAX);
	memset((uint8 *)wlc_cm->sarlimit.band5g,
	       wlc->bandstate[BAND_5G_INDEX]->sar,
	       (WLC_TXCORE_MAX * WLC_SUBBAND_MAX));

	wlc_channel_sarlimit_get_default(wlc_cm, &wlc_cm->sarlimit);
}

int
wlc_channel_sarlimit_get(wlc_cm_info_t *wlc_cm, sar_limit_t *sar)
{
	memcpy((uint8 *)sar, (uint8 *)&wlc_cm->sarlimit, sizeof(sar_limit_t));
	return 0;
}

int
wlc_channel_sarlimit_set(wlc_cm_info_t *wlc_cm, sar_limit_t *sar)
{
	memcpy((uint8 *)&wlc_cm->sarlimit, (uint8 *)sar, sizeof(sar_limit_t));
	return 0;
}

/* given chanspec and return the subband index */
static uint
wlc_channel_sarlimit_subband_idx(wlc_cm_info_t *wlc_cm, chanspec_t chanspec)
{
	uint8 chan = CHSPEC_CHANNEL(chanspec);
	if (chan < CHANNEL_5G_MID_START)
		return 0;
	else if (chan >= CHANNEL_5G_MID_START && chan < CHANNEL_5G_HIGH_START)
		return 1;
	else if (chan >= CHANNEL_5G_HIGH_START && chan < CHANNEL_5G_UPPER_START)
		return 2;
	else
		return 3;
}

/* Get the sar limit for the subband containing this channel */
void
wlc_channel_sarlimit_subband(wlc_cm_info_t *wlc_cm, chanspec_t chanspec, uint8 *sar)
{
	int idx = 0;

	if (CHSPEC_IS5G(chanspec)) {
		idx = wlc_channel_sarlimit_subband_idx(wlc_cm, chanspec);
		memcpy((uint8 *)sar, (uint8 *)wlc_cm->sarlimit.band5g[idx], WLC_TXCORE_MAX);
	} else {
		memcpy((uint8 *)sar, (uint8 *)wlc_cm->sarlimit.band2g, WLC_TXCORE_MAX);
	}
}
#endif /* WL_SARLIMIT */

bool
wlc_channel_sarenable_get(wlc_cm_info_t *wlc_cm)
{
	return (wlc_cm->sar_enable);
}

void
wlc_channel_sarenable_set(wlc_cm_info_t *wlc_cm, bool state)
{
	wlc_cm->sar_enable = state ? TRUE : FALSE;
}

void
wlc_channel_reg_limits(wlc_cm_info_t *wlc_cm, chanspec_t chanspec, ppr_t *txpwr)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	unsigned int chan;
	clm_country_t country;
	clm_result_t result = CLM_RESULT_ERR;
	clm_country_locales_t locale;
	clm_power_limits_t limits;
	uint8 flags;
	clm_band_t bandtype;
	wlcband_t * band;
	bool filt_war = FALSE;
	int ant_gain;
	clm_limits_params_t lim_params;
#ifdef WL_SARLIMIT
	uint8 sarlimit[WLC_TXCORE_MAX];
#endif

	ppr_clear(txpwr);

	if (clm_limits_params_init(&lim_params) != CLM_RESULT_OK)
		return;

	band = wlc->bandstate[CHSPEC_WLCBANDUNIT(chanspec)];
	bandtype = BAND_5G(band->bandtype) ? CLM_BAND_5G : CLM_BAND_2G;
	/* Lookup channel in autocountry_default if not in current country */
	if ((WLC_LOCALE_PRIORITIZATION_2G_ENABLED(wlc) &&
		bandtype == CLM_BAND_2G && !WLC_CNTRY_DEFAULT_ENAB(wlc)) ||
		!wlc_valid_chanspec_db(wlc_cm, chanspec)) {
		if (WLC_AUTOCOUNTRY_ENAB(wlc)) {
			const char *def = wlc_11d_get_autocountry_default(wlc->m11d);
			result = wlc_country_lookup(wlc, def, &country);
		}
		if (result != CLM_RESULT_OK)
			return;
	} else
		country = wlc_cm->country;

	chan = CHSPEC_CHANNEL(chanspec);
	ant_gain = band->antgain;
	lim_params.sar = WLC_TXPWR_MAX;
	band->sar = band->sar_cached;
	if (wlc_cm->sar_enable) {
#ifdef WL_SARLIMIT
		/* when WL_SARLIMIT is enabled, update band->sar = MAX(sarlimit[i]) */
		wlc_channel_sarlimit_subband(wlc_cm, chanspec, sarlimit);
		if ((CHIPID(wlc->pub->sih->chip) != BCM4360_CHIP_ID) &&
		    (CHIPID(wlc->pub->sih->chip) != BCM4350_CHIP_ID) &&
		    (CHIPID(wlc->pub->sih->chip) != BCM43602_CHIP_ID)) {
			uint i;
			band->sar = 0;
			for (i = 0; i < WLC_BITSCNT(wlc->stf->hw_txchain); i++)
				band->sar = MAX(band->sar, sarlimit[i]);

			WL_NONE(("%s: in %s Band, SAR %d apply\n", __FUNCTION__,
				(BAND_5G(wlc->band->bandtype) ? "5G" : "2G"), band->sar));
		}
		/* Don't write sarlimit to registers when called for reporting purposes */
		if (chanspec == wlc->chanspec)
			wlc_phy_sar_limit_set(band->pi,
			                      (uint32)(sarlimit[0] | sarlimit[1] << 8 |
			                               sarlimit[2] << 16 | sarlimit[3] << 24));
#endif /* WL_SARLIMIT */
		lim_params.sar = band->sar;
	}

	result = wlc_get_locale(country, &locale);
	if (result != CLM_RESULT_OK)
		return;

	result = wlc_get_flags(&locale, bandtype, &flags);
	if (result != CLM_RESULT_OK)
		return;

	if (wlc_cm->bandedge_filter_apply &&
	    (flags & WLC_FILT_WAR) &&
	    (chan == 1 || chan == 13))
		filt_war = TRUE;
	wlc_bmac_filter_war_upd(wlc->hw, filt_war);

	/* Need to set the txpwr_local_max to external reg max for
	 * this channel as per the locale selected for AP.
	 */
#ifdef AP
	if (AP_ONLY(wlc->pub)) {
		uint8 ch = wf_chspec_ctlchan(wlc->chanspec);
		uint8 pwr = wlc_get_reg_max_power_for_channel(wlc->cmi, ch, TRUE);
		wlc_tpc_set_local_max(wlc->tpc, pwr);
	}
#endif

	/* Get 20MHz limits */
	if (CHSPEC_IS20(chanspec)) {
		lim_params.bw = CLM_BW_20;
		if (clm_power_limits(&locale, bandtype, chan, ant_gain, CLM_LIMITS_TYPE_CHANNEL,
			&lim_params, &limits) == CLM_RESULT_OK) {

			/* Port the 20MHz values */
			ppr_set_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_1, CLM_DSSS_RATESET(limits));

			ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
				CLM_OFDM_1X1_RATESET(limits));

#ifdef WL11N
			ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_NONE,
				WL_TX_CHAINS_1, CLM_MCS_1X1_RATESET(limits));


			if (PHYCORENUM(wlc->stf->txstreams) > 1) {
				ppr_set_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_2,
					CLM_DSSS_1X2_MULTI_RATESET(limits));

				ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_CDD_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_CDD,
					WL_TX_CHAINS_2, CLM_MCS_1X2_CDD_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_STBC,
					WL_TX_CHAINS_2, CLM_MCS_2X2_STBC_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_NONE,
					WL_TX_CHAINS_2, CLM_MCS_2X2_SDM_RATESET(limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_3,
						CLM_DSSS_1X3_MULTI_RATESET(limits));

					ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_CDD,
						WL_TX_CHAINS_3,	CLM_OFDM_1X3_CDD_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1,
						WL_TX_MODE_CDD, WL_TX_CHAINS_3,
						CLM_MCS_1X3_CDD_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2,
						WL_TX_MODE_STBC, WL_TX_CHAINS_3,
						CLM_MCS_2X3_STBC_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_2X3_SDM_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_3,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_3X3_SDM_RATESET(limits));
				}
			}
#if defined(WL_BEAMFORMING)
			if (TXBF_ENAB(wlc->pub) && (PHYCORENUM(wlc->stf->txstreams) > 1)) {
				ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_TXBF_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2, CLM_MCS_1X2_TXBF_RATESET(limits));

				ppr_set_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2, CLM_MCS_2X2_TXBF_RATESET(limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_TXBF,
						WL_TX_CHAINS_3,	CLM_OFDM_1X3_TXBF_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_1X3_TXBF_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_2X3_TXBF_RATESET(limits));

					ppr_set_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_3,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_3X3_TXBF_RATESET(limits));
				}
			}
#endif /* defined(WL_BEAMFORMING) */
#endif /* WL11N */
		}

	} else if (CHSPEC_IS40(chanspec)) {
	/* Get 40MHz and 20in40MHz limits */
		clm_power_limits_t bw20in40_limits;

		lim_params.bw = CLM_BW_40;

		if ((clm_power_limits(&locale, bandtype, chan, ant_gain, CLM_LIMITS_TYPE_CHANNEL,
			&lim_params, &limits) == CLM_RESULT_OK) &&

			(clm_power_limits(&locale, bandtype, chan, ant_gain,
			CHSPEC_SB_UPPER(chanspec) ?
			CLM_LIMITS_TYPE_SUBCHAN_U : CLM_LIMITS_TYPE_SUBCHAN_L,
			&lim_params, &bw20in40_limits) == CLM_RESULT_OK)) {

			/* Port the 40MHz values */


			ppr_set_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
				CLM_OFDM_1X1_RATESET(limits));

#ifdef WL11N
			/* Skip WL_RATE_1X1_DSSS_1 - not valid for 40MHz */
			ppr_set_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
				CLM_OFDM_1X1_RATESET(limits));

			ppr_set_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_NONE,
				WL_TX_CHAINS_1, CLM_MCS_1X1_RATESET(limits));

			if (PHYCORENUM(wlc->stf->txstreams) > 1) {
				/* Skip WL_RATE_1X2_DSSS_1 - not valid for 40MHz */
				ppr_set_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_CDD_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_CDD,
					WL_TX_CHAINS_2, CLM_MCS_1X2_CDD_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2, WL_TX_MODE_STBC,
					WL_TX_CHAINS_2, CLM_MCS_2X2_STBC_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2, WL_TX_MODE_NONE,
					WL_TX_CHAINS_2, CLM_MCS_2X2_SDM_RATESET(limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					/* Skip WL_RATE_1X3_DSSS_1 - not valid for 40MHz */
					ppr_set_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_CDD,
						WL_TX_CHAINS_3,	CLM_OFDM_1X3_CDD_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1,
						WL_TX_MODE_CDD,	WL_TX_CHAINS_3,
						CLM_MCS_1X3_CDD_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2,
						WL_TX_MODE_STBC, WL_TX_CHAINS_3,
						CLM_MCS_2X3_STBC_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_2X3_SDM_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_3,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_3X3_SDM_RATESET(limits));
				}
			}
#if defined(WL_BEAMFORMING)
			if (TXBF_ENAB(wlc->pub) && (PHYCORENUM(wlc->stf->txstreams) > 1)) {
				ppr_set_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_TXBF_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2, CLM_MCS_1X2_TXBF_RATESET(limits));

				ppr_set_ht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2, CLM_MCS_2X2_TXBF_RATESET(limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_TXBF,
						WL_TX_CHAINS_3,	CLM_OFDM_1X3_TXBF_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_1X3_TXBF_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_2X3_TXBF_RATESET(limits));

					ppr_set_ht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_3,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_3X3_TXBF_RATESET(limits));
				}
			}
#endif /* defined(WL_BEAMFORMING) */
			/* Port the 20in40 values */


			ppr_set_dsss(txpwr, WL_TX_BW_20,
				WL_TX_CHAINS_1, CLM_DSSS_RATESET(bw20in40_limits));

			ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_NONE,
				WL_TX_CHAINS_1, CLM_OFDM_1X1_RATESET(bw20in40_limits));

			ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_NONE,
				WL_TX_CHAINS_1, CLM_MCS_1X1_RATESET(bw20in40_limits));

			if (PHYCORENUM(wlc->stf->txstreams) > 1) {
				ppr_set_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_2,
					CLM_DSSS_1X2_MULTI_RATESET(bw20in40_limits));

				ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_CDD_RATESET(bw20in40_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_CDD,
					WL_TX_CHAINS_2, CLM_MCS_1X2_CDD_RATESET(bw20in40_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_STBC,
					WL_TX_CHAINS_2, CLM_MCS_2X2_STBC_RATESET(bw20in40_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_NONE,
					WL_TX_CHAINS_2, CLM_MCS_2X2_SDM_RATESET(bw20in40_limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_3,
						CLM_DSSS_1X3_MULTI_RATESET(bw20in40_limits));

					ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_CDD,
						WL_TX_CHAINS_3,
						CLM_OFDM_1X3_CDD_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1,
						WL_TX_MODE_CDD, WL_TX_CHAINS_3,
						CLM_MCS_1X3_CDD_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2,
						WL_TX_MODE_STBC, WL_TX_CHAINS_3,
						CLM_MCS_2X3_STBC_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_2X3_SDM_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_3,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_3X3_SDM_RATESET(bw20in40_limits));
				}
			}

			ppr_set_dsss(txpwr, WL_TX_BW_20IN40,
				WL_TX_CHAINS_1, CLM_DSSS_RATESET(bw20in40_limits));

			ppr_set_ofdm(txpwr, WL_TX_BW_20IN40, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
				CLM_OFDM_1X1_RATESET(bw20in40_limits));

			ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1, WL_TX_MODE_NONE,
				WL_TX_CHAINS_1, CLM_MCS_1X1_RATESET(bw20in40_limits));

			if (PHYCORENUM(wlc->stf->txstreams) > 1) {
				ppr_set_dsss(txpwr, WL_TX_BW_20IN40, WL_TX_CHAINS_2,
					CLM_DSSS_1X2_MULTI_RATESET(bw20in40_limits));

				ppr_set_ofdm(txpwr, WL_TX_BW_20IN40, WL_TX_MODE_CDD,
					WL_TX_CHAINS_2,	CLM_OFDM_1X2_CDD_RATESET(bw20in40_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1,
					WL_TX_MODE_CDD,	WL_TX_CHAINS_2,
					CLM_MCS_1X2_CDD_RATESET(bw20in40_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2,
					WL_TX_MODE_STBC, WL_TX_CHAINS_2,
					CLM_MCS_2X2_STBC_RATESET(bw20in40_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2,
					WL_TX_MODE_NONE, WL_TX_CHAINS_2,
					CLM_MCS_2X2_SDM_RATESET(bw20in40_limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_dsss(txpwr, WL_TX_BW_20IN40, WL_TX_CHAINS_3,
						CLM_DSSS_1X3_MULTI_RATESET(bw20in40_limits));

					ppr_set_ofdm(txpwr, WL_TX_BW_20IN40, WL_TX_MODE_CDD,
						WL_TX_CHAINS_3,
						CLM_OFDM_1X3_CDD_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1,
						WL_TX_MODE_CDD,	WL_TX_CHAINS_3,
						CLM_MCS_1X3_CDD_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2,
						WL_TX_MODE_STBC, WL_TX_CHAINS_3,
						CLM_MCS_2X3_STBC_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_2X3_SDM_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_3,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_3X3_SDM_RATESET(bw20in40_limits));
				}
			}

#if defined(WL_BEAMFORMING)
			if (TXBF_ENAB(wlc->pub) && (PHYCORENUM(wlc->stf->txstreams) > 1)) {
				ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_TXBF_RATESET(bw20in40_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2, CLM_MCS_1X2_TXBF_RATESET(bw20in40_limits));

				ppr_set_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2, CLM_MCS_2X2_TXBF_RATESET(bw20in40_limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_TXBF,
						WL_TX_CHAINS_3,
						CLM_OFDM_1X3_TXBF_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_1X3_TXBF_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_2X3_TXBF_RATESET(bw20in40_limits));

					ppr_set_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_3,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_3X3_TXBF_RATESET(bw20in40_limits));
				}

				ppr_set_ofdm(txpwr, WL_TX_BW_20IN40, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2,
					CLM_OFDM_1X2_TXBF_RATESET(bw20in40_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1,
					WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
					CLM_MCS_1X2_TXBF_RATESET(bw20in40_limits));

				ppr_set_ht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2,
					WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
					CLM_MCS_2X2_TXBF_RATESET(bw20in40_limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_ofdm(txpwr, WL_TX_BW_20IN40, WL_TX_MODE_TXBF,
						WL_TX_CHAINS_3,
						CLM_OFDM_1X3_TXBF_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_1X3_TXBF_RATESET(bw20in40_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_2X3_TXBF_RATESET(bw20in40_limits));

					ppr_set_ht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_3,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_3X3_TXBF_RATESET(bw20in40_limits));
				}
			}
#endif /* defined(WL_BEAMFORMING) */
#endif /* WL11N */
		}

	} else {
#ifdef WL11AC
	/* Get 80MHz, 40in80MHz and 20in80MHz limits */
		clm_power_limits_t bw20in80_limits;
		clm_power_limits_t bw40in80_limits;
		clm_limits_type_t sb40 = CLM_LIMITS_TYPE_SUBCHAN_L;

		uint sb = CHSPEC_CTL_SB(chanspec);

		lim_params.bw = CLM_BW_80;

		if ((sb == WL_CHANSPEC_CTL_SB_UU) || (sb == WL_CHANSPEC_CTL_SB_UL))
			/* Primary 40MHz is on upper side */
			sb40 = CLM_LIMITS_TYPE_SUBCHAN_U;


		if ((clm_power_limits(&locale, bandtype, chan, ant_gain, CLM_LIMITS_TYPE_CHANNEL,
			&lim_params, &limits) == CLM_RESULT_OK) &&

			(clm_power_limits(&locale, bandtype, chan, ant_gain,
			sb40, &lim_params, &bw40in80_limits) == CLM_RESULT_OK) &&

			(clm_power_limits(&locale, bandtype, chan, ant_gain,
			clm_sideband_to_limits_type(sb), &lim_params,
			&bw20in80_limits) == CLM_RESULT_OK)) {

			/* Port the 80MHz values */

			ppr_set_ofdm(txpwr, WL_TX_BW_80, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
				CLM_OFDM_1X1_RATESET(limits));

#ifdef WL11N
			/* Skip WL_RATE_1X1_DSSS_1 - not valid for 80MHz */
			ppr_set_ofdm(txpwr, WL_TX_BW_80, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
				CLM_OFDM_1X1_RATESET(limits));

			ppr_set_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_1, WL_TX_MODE_NONE,
				WL_TX_CHAINS_1, CLM_MCS_1X1_RATESET(limits));

			if (PHYCORENUM(wlc->stf->txstreams) > 1) {
				/* Skip WL_RATE_1X2_DSSS_1 - not valid for 80MHz */
				ppr_set_ofdm(txpwr, WL_TX_BW_80, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_CDD_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_1, WL_TX_MODE_CDD,
					WL_TX_CHAINS_2, CLM_MCS_1X2_CDD_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2, WL_TX_MODE_STBC,
					WL_TX_CHAINS_2, CLM_MCS_2X2_STBC_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2, WL_TX_MODE_NONE,
					WL_TX_CHAINS_2, CLM_MCS_2X2_SDM_RATESET(limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					/* Skip WL_RATE_1X3_DSSS_1 - not valid for 80MHz */
					ppr_set_ofdm(txpwr, WL_TX_BW_80, WL_TX_MODE_CDD,
						WL_TX_CHAINS_3,
						CLM_OFDM_1X3_CDD_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_1,
						WL_TX_MODE_CDD,	WL_TX_CHAINS_3,
						CLM_MCS_1X3_CDD_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2,
						WL_TX_MODE_STBC, WL_TX_CHAINS_3,
						CLM_MCS_2X3_STBC_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_2X3_SDM_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_3,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_3X3_SDM_RATESET(limits));
				}
			}

#if defined(WL_BEAMFORMING)
			if (TXBF_ENAB(wlc->pub) && (PHYCORENUM(wlc->stf->txstreams) > 1)) {
				ppr_set_ofdm(txpwr, WL_TX_BW_80, WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_TXBF_RATESET(limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_1, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2, CLM_MCS_1X2_TXBF_RATESET(limits));

				ppr_set_ht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2, CLM_MCS_2X2_TXBF_RATESET(limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_ofdm(txpwr, WL_TX_BW_80, WL_TX_MODE_TXBF,
						WL_TX_CHAINS_3,	CLM_OFDM_1X3_TXBF_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_1,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_1X3_TXBF_RATESET(limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_2X3_TXBF_RATESET(limits));

					ppr_set_ht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_3,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_3X3_TXBF_RATESET(limits));
				}
			}
#endif /* defined(WL_BEAMFORMING) */
			/* Port the 40in80 values */

			/* Skip WL_RATE_1X1_DSSS_1 - not valid for 40MHz */
			ppr_set_ofdm(txpwr, WL_TX_BW_40IN80, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
				CLM_OFDM_1X1_RATESET(bw40in80_limits));

			ppr_set_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_1, WL_TX_MODE_NONE,
				WL_TX_CHAINS_1, CLM_MCS_1X1_RATESET(bw40in80_limits));


			if (PHYCORENUM(wlc->stf->txstreams) > 1) {
				/* Skip WL_RATE_1X2_DSSS_1 - not valid for 40MHz */
				ppr_set_ofdm(txpwr, WL_TX_BW_40IN80, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_CDD_RATESET(bw40in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_1, WL_TX_MODE_CDD,
					WL_TX_CHAINS_2, CLM_MCS_1X2_CDD_RATESET(bw40in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2,
					WL_TX_MODE_STBC, WL_TX_CHAINS_2,
					CLM_MCS_2X2_STBC_RATESET(bw40in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2,
					WL_TX_MODE_NONE, WL_TX_CHAINS_2,
					CLM_MCS_2X2_SDM_RATESET(bw40in80_limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					/* Skip WL_RATE_1X3_DSSS_1 - not valid for 40MHz */
					ppr_set_ofdm(txpwr, WL_TX_BW_40IN80, WL_TX_MODE_CDD,
						WL_TX_CHAINS_3,
						CLM_OFDM_1X3_CDD_RATESET(bw40in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_1,
						WL_TX_MODE_CDD,	WL_TX_CHAINS_3,
						CLM_MCS_1X3_CDD_RATESET(bw40in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2,
						WL_TX_MODE_STBC, WL_TX_CHAINS_3,
						CLM_MCS_2X3_STBC_RATESET(bw40in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_2X3_SDM_RATESET(bw40in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_3,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_3X3_SDM_RATESET(bw40in80_limits));
				}
			}

#if defined(WL_BEAMFORMING)
			if (TXBF_ENAB(wlc->pub) && (PHYCORENUM(wlc->stf->txstreams) > 1)) {
				ppr_set_ofdm(txpwr, WL_TX_BW_40IN80, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2,
					CLM_OFDM_1X2_TXBF_RATESET(bw40in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_1,
					WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
					CLM_MCS_1X2_TXBF_RATESET(bw40in80_limits));

				ppr_set_ht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2,
					WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
					CLM_MCS_2X2_TXBF_RATESET(bw40in80_limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_ofdm(txpwr, WL_TX_BW_40IN80, WL_TX_MODE_TXBF,
						WL_TX_CHAINS_3,
						CLM_OFDM_1X3_TXBF_RATESET(bw40in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_1,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_1X3_TXBF_RATESET(bw40in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_2X3_TXBF_RATESET(bw40in80_limits));

					ppr_set_ht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_3,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_3X3_TXBF_RATESET(bw40in80_limits));
				}
			}
#endif /* defined(WL_BEAMFORMING) */
			/* Port the 20in80 values */

			ppr_set_dsss(txpwr, WL_TX_BW_20,
				WL_TX_CHAINS_1, CLM_DSSS_RATESET(bw20in80_limits));

			ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
				CLM_OFDM_1X1_RATESET(bw20in80_limits));

			ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_NONE,
				WL_TX_CHAINS_1, CLM_MCS_1X1_RATESET(bw20in80_limits));


			if (PHYCORENUM(wlc->stf->txstreams) > 1) {
				ppr_set_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_2,
					CLM_DSSS_1X2_MULTI_RATESET(bw20in80_limits));

				ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_CDD_RATESET(bw20in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_CDD,
					WL_TX_CHAINS_2, CLM_MCS_1X2_CDD_RATESET(bw20in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_STBC,
					WL_TX_CHAINS_2, CLM_MCS_2X2_STBC_RATESET(bw20in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_NONE,
					WL_TX_CHAINS_2, CLM_MCS_2X2_SDM_RATESET(bw20in80_limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_3,
						CLM_DSSS_1X3_MULTI_RATESET(bw20in80_limits));

					ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_CDD,
						WL_TX_CHAINS_3,
						CLM_OFDM_1X3_CDD_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1,
						WL_TX_MODE_CDD,	WL_TX_CHAINS_3,
						CLM_MCS_1X3_CDD_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2,
						WL_TX_MODE_STBC, WL_TX_CHAINS_3,
						CLM_MCS_2X3_STBC_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_2X3_SDM_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_3,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_3X3_SDM_RATESET(bw20in80_limits));
				}
			}

			ppr_set_dsss(txpwr, WL_TX_BW_20IN80,
				WL_TX_CHAINS_1, CLM_DSSS_RATESET(bw20in80_limits));

			ppr_set_ofdm(txpwr, WL_TX_BW_20IN80, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
				CLM_OFDM_1X1_RATESET(bw20in80_limits));

			ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_1, WL_TX_MODE_NONE,
				WL_TX_CHAINS_1, CLM_MCS_1X1_RATESET(bw20in80_limits));


			if (PHYCORENUM(wlc->stf->txstreams) > 1) {
				ppr_set_dsss(txpwr, WL_TX_BW_20IN80, WL_TX_CHAINS_2,
					CLM_DSSS_1X2_MULTI_RATESET(bw20in80_limits));

				ppr_set_ofdm(txpwr, WL_TX_BW_20IN80, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_CDD_RATESET(bw20in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_1, WL_TX_MODE_CDD,
					WL_TX_CHAINS_2, CLM_MCS_1X2_CDD_RATESET(bw20in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2,
					WL_TX_MODE_STBC, WL_TX_CHAINS_2,
					CLM_MCS_2X2_STBC_RATESET(bw20in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2,
					WL_TX_MODE_NONE, WL_TX_CHAINS_2,
					CLM_MCS_2X2_SDM_RATESET(bw20in80_limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_dsss(txpwr, WL_TX_BW_20IN80, WL_TX_CHAINS_3,
						CLM_DSSS_1X3_MULTI_RATESET(bw20in80_limits));

					ppr_set_ofdm(txpwr, WL_TX_BW_20IN80, WL_TX_MODE_CDD,
						WL_TX_CHAINS_3,
						CLM_OFDM_1X3_CDD_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_1,
						WL_TX_MODE_CDD,	WL_TX_CHAINS_3,
						CLM_MCS_1X3_CDD_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2,
						WL_TX_MODE_STBC, WL_TX_CHAINS_3,
						CLM_MCS_2X3_STBC_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_2X3_SDM_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_3,
						WL_TX_MODE_NONE, WL_TX_CHAINS_3,
						CLM_MCS_3X3_SDM_RATESET(bw20in80_limits));
				}
			}
#if defined(WL_BEAMFORMING)
			if (TXBF_ENAB(wlc->pub) && (PHYCORENUM(wlc->stf->txstreams) > 1)) {
				ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
					CLM_OFDM_1X2_TXBF_RATESET(bw20in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2, CLM_MCS_1X2_TXBF_RATESET(bw20in80_limits));

				ppr_set_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2, CLM_MCS_2X2_TXBF_RATESET(bw20in80_limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_TXBF,
						WL_TX_CHAINS_3,
						CLM_OFDM_1X3_TXBF_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_1X3_TXBF_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_2X3_TXBF_RATESET(bw20in80_limits));

					ppr_set_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_3,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_3X3_TXBF_RATESET(bw20in80_limits));
				}
				ppr_set_ofdm(txpwr, WL_TX_BW_20IN80, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2,	CLM_OFDM_1X2_TXBF_RATESET(bw20in80_limits));

				ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_1,
					WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
					CLM_MCS_1X2_TXBF_RATESET(bw20in80_limits));

				ppr_set_ht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2, WL_TX_MODE_TXBF,
					WL_TX_CHAINS_2, CLM_MCS_2X2_TXBF_RATESET(bw20in80_limits));

				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_set_ofdm(txpwr, WL_TX_BW_20IN80, WL_TX_MODE_TXBF,
						WL_TX_CHAINS_3,
						CLM_OFDM_1X3_TXBF_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_1,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_1X3_TXBF_RATESET(bw20in80_limits));

					ppr_set_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_2X3_TXBF_RATESET(bw20in80_limits));

					ppr_set_ht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_3,
						WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
						CLM_MCS_3X3_TXBF_RATESET(bw20in80_limits));
				}
			}
#endif /* defined(WL_BEAMFORMING) */
#endif /* WL11N */
		}
#endif /* WL11AC */

	}

	WL_NONE(("Channel(chanspec) %d (0x%4.4x)\n", chan, chanspec));
#ifdef WLC_LOW
	/* Convoluted WL debug conditional execution of function to avoid warnings. */
	WL_NONE(("%s", (wlc_phy_txpower_limits_dump(txpwr, WLCISHTPHY(wlc->band)), "")));
#endif /* WLC_LOW */

}
static clm_result_t
clm_power_limits(
	const clm_country_locales_t *locales, clm_band_t band,
	unsigned int chan, int ant_gain, clm_limits_type_t limits_type,
	const clm_limits_params_t *params, clm_power_limits_t *limits)
{
	return clm_limits(locales, band, chan, ant_gain, limits_type, params, limits);
}


/* Returns TRUE if currently set country is Japan or variant */
bool
wlc_japan(struct wlc_info *wlc)
{
	return wlc_japan_ccode(wlc->cmi->country_abbrev);
}

/* JP, J1 - J10 are Japan ccodes */
static bool
wlc_japan_ccode(const char *ccode)
{
	return (ccode[0] == 'J' &&
		(ccode[1] == 'P' || (ccode[1] >= '1' && ccode[1] <= '9')));
}

/* Q2 and Q1 are alternate USA ccode */
static bool
wlc_us_ccode(const char *ccode)
{
	return (!strncmp("US", ccode, 3) ||
		!strncmp("Q1", ccode, 3) ||
		!strncmp("Q2", ccode, 3) ||
		!strncmp("ALL", ccode, 3));
}

void
wlc_rcinfo_init(wlc_cm_info_t *wlc_cm)
{
	if (wlc_us_ccode(wlc_cm->country_abbrev)) {
#ifdef BAND5G
		wlc_cm->rcinfo_list[WLC_RCLIST_20] = &rcinfo_us_20;
#endif
#ifdef WL11N
		if (N_ENAB(wlc_cm->wlc->pub)) {
			wlc_cm->rcinfo_list[WLC_RCLIST_40L] = &rcinfo_us_40lower;
			wlc_cm->rcinfo_list[WLC_RCLIST_40U] = &rcinfo_us_40upper;
		}
#endif
	} else if (wlc_japan_ccode(wlc_cm->country_abbrev)) {
#ifdef BAND5G
		wlc_cm->rcinfo_list[WLC_RCLIST_20] = &rcinfo_jp_20;
#endif
#ifdef WL11N
		if (N_ENAB(wlc_cm->wlc->pub)) {
			wlc_cm->rcinfo_list[WLC_RCLIST_40L] = &rcinfo_jp_40;
			wlc_cm->rcinfo_list[WLC_RCLIST_40U] = &rcinfo_jp_40;
		}
#endif
	} else {
#ifdef BAND5G
		wlc_cm->rcinfo_list[WLC_RCLIST_20] = &rcinfo_eu_20;
#endif
#ifdef WL11N
		if (N_ENAB(wlc_cm->wlc->pub)) {
			wlc_cm->rcinfo_list[WLC_RCLIST_40L] = &rcinfo_eu_40lower;
			wlc_cm->rcinfo_list[WLC_RCLIST_40U] = &rcinfo_eu_40upper;
		}
#endif
	}
}

static void
wlc_regclass_vec_init(wlc_cm_info_t *wlc_cm)
{
	uint8 i, idx;
	chanspec_t chanspec;
#ifdef WL11N
	wlc_info_t *wlc = wlc_cm->wlc;
	bool saved_cap_40, saved_db_cap_40 = TRUE;
#endif
	rcvec_t *rcvec = &wlc_cm->valid_rcvec;

#ifdef WL11N
	/* save 40 MHz cap */
	saved_cap_40 = WL_BW_CAP_40MHZ(wlc->band->bw_cap);
	wlc->band->bw_cap |= WLC_BW_40MHZ_BIT;
	if (NBANDS(wlc) > 1) {
		saved_db_cap_40 = WL_BW_CAP_40MHZ(wlc->bandstate[OTHERBANDUNIT(wlc)]->bw_cap);
		wlc->bandstate[OTHERBANDUNIT(wlc)]->bw_cap |= WLC_BW_40MHZ_BIT;
	}
#endif

	bzero(rcvec, MAXRCVEC);
	for (i = 0; i < MAXCHANNEL; i++) {
		chanspec = CH20MHZ_CHSPEC(i);
		if (wlc_valid_chanspec_db(wlc_cm, chanspec)) {
			if ((idx = wlc_get_regclass(wlc_cm, chanspec)))
				setbit((uint8 *)rcvec, idx);
		}
#if defined(WL11N) && !defined(WL11N_20MHZONLY)
		if (N_ENAB(wlc->pub)) {
			chanspec = CH40MHZ_CHSPEC(i, WL_CHANSPEC_CTL_SB_LOWER);
			if (wlc_valid_chanspec_db(wlc_cm, chanspec)) {
				if ((idx = wlc_get_regclass(wlc_cm, chanspec)))
					setbit((uint8 *)rcvec, idx);
			}
			chanspec = CH40MHZ_CHSPEC(i, WL_CHANSPEC_CTL_SB_UPPER);
			if (wlc_valid_chanspec_db(wlc_cm, chanspec)) {
				if ((idx = wlc_get_regclass(wlc_cm, chanspec)))
					setbit((uint8 *)rcvec, idx);
			}
		}
#endif /* defined(WL11N) && !defined(WL11N_20MHZONLY) */
	}
#ifdef WL11N
	/* restore 40 MHz cap */
	if (saved_cap_40) {
		wlc->band->bw_cap |= WLC_BW_40MHZ_BIT;
	} else {
		wlc->band->bw_cap &= ~WLC_BW_40MHZ_BIT;
	}

	if (NBANDS(wlc) > 1) {
		if (saved_db_cap_40) {
			wlc->bandstate[OTHERBANDUNIT(wlc)]->bw_cap |= WLC_BW_40MHZ_BIT;
		} else {
			wlc->bandstate[OTHERBANDUNIT(wlc)]->bw_cap &= ~WLC_BW_40MHZ_BIT;
		}
	}
#endif
}

#ifdef WL11N
uint8
wlc_rclass_extch_get(wlc_cm_info_t *wlc_cm, uint8 rclass)
{
	const rcinfo_t *rcinfo;
	uint8 i, extch = DOT11_EXT_CH_NONE;

	if (!isset(wlc_cm->valid_rcvec.vec, rclass)) {
		WL_ERROR(("wl%d: %s %d regulatory class not supported\n",
			wlc_cm->wlc->pub->unit, wlc_cm->country_abbrev, rclass));
		return extch;
	}

	/* rcinfo consist of control channel at lower sb */
	rcinfo = wlc_cm->rcinfo_list[WLC_RCLIST_40L];
	for (i = 0; rcinfo && i < rcinfo->len; i++) {
		if (rclass == rcinfo->rctbl[i].rclass) {
			/* ext channel is opposite of control channel */
			extch = DOT11_EXT_CH_UPPER;
			goto exit;
		}
	}

	/* rcinfo consist of control channel at upper sb */
	rcinfo = wlc_cm->rcinfo_list[WLC_RCLIST_40U];
	for (i = 0; rcinfo && i < rcinfo->len; i++) {
		if (rclass == rcinfo->rctbl[i].rclass) {
			/* ext channel is opposite of control channel */
			extch = DOT11_EXT_CH_LOWER;
			break;
		}
	}
exit:
	WL_INFORM(("wl%d: %s regulatory class %d has ctl chan %s\n",
		wlc_cm->wlc->pub->unit, wlc_cm->country_abbrev, rclass,
		((!extch) ? "NONE" : (((extch == DOT11_EXT_CH_LOWER) ? "LOWER" : "UPPER")))));

	return extch;
}
#endif /* WL11N */

#if defined(WLTDLS)
/* get the ordered list of supported reg class, with current reg class
 * as first element
 */
uint8
wlc_get_regclass_list(wlc_cm_info_t *wlc_cm, uint8 *rclist, uint lsize,
	chanspec_t chspec, bool ie_order)
{
	uint8 i, cur_rc = 0, idx = 0;

	ASSERT(rclist != NULL);
	ASSERT(lsize > 1);

	if (ie_order) {
		cur_rc = wlc_get_regclass(wlc_cm, chspec);
		if (!cur_rc) {
			WL_ERROR(("wl%d: current regulatory class is not found\n",
				wlc_cm->wlc->pub->unit));
			return 0;
		}
		rclist[idx++] = cur_rc;	/* first element is current reg class */
	}

	for (i = 0; i < MAXREGCLASS && idx < lsize; i++) {
		if (i != cur_rc && isset(wlc_cm->valid_rcvec.vec, i))
			rclist[idx++] = i;
	}

	if (i < MAXREGCLASS && idx == lsize) {
		WL_ERROR(("wl%d: regulatory class list full %d\n", wlc_cm->wlc->pub->unit, idx));
		ASSERT(0);
	}

	return idx;
}
#endif 

static uint8
wlc_get_2g_regclass(wlc_cm_info_t *wlc_cm, uint8 chan)
{
	if (wlc_us_ccode(wlc_cm->country_abbrev))
		return WLC_REGCLASS_USA_2G_20MHZ;
	else if (wlc_japan_ccode(wlc_cm->country_abbrev)) {
		if (chan < 14)
			return WLC_REGCLASS_JPN_2G_20MHZ;
		else
			return WLC_REGCLASS_JPN_2G_20MHZ_CH14;
	} else
		return WLC_REGCLASS_EUR_2G_20MHZ;
}

uint8
wlc_get_regclass(wlc_cm_info_t *wlc_cm, chanspec_t chanspec)
{
	const rcinfo_t *rcinfo = NULL;
	uint8 i;
	uint8 chan;

#ifdef WL11AC
	if (CHSPEC_IS80(chanspec) || CHSPEC_IS160(chanspec) ||
		CHSPEC_IS8080(chanspec)) {
		chan = wf_chspec_ctlchan(chanspec);
		if (CHSPEC_SB_UPPER(wf_chspec_primary40_chspec(chanspec)))
			rcinfo = wlc_cm->rcinfo_list[WLC_RCLIST_40U];
		else
			rcinfo = wlc_cm->rcinfo_list[WLC_RCLIST_40L];
	} else
#endif /* WL11AC */

#ifdef WL11N
	if (CHSPEC_IS40(chanspec)) {
		chan = wf_chspec_ctlchan(chanspec);
		if (CHSPEC_SB_UPPER(chanspec))
			rcinfo = wlc_cm->rcinfo_list[WLC_RCLIST_40U];
		else
			rcinfo = wlc_cm->rcinfo_list[WLC_RCLIST_40L];
	} else
#endif /* WL11N */
	{
		chan = CHSPEC_CHANNEL(chanspec);
		if (CHSPEC_IS2G(chanspec))
			return (wlc_get_2g_regclass(wlc_cm, chan));
		rcinfo = wlc_cm->rcinfo_list[WLC_RCLIST_20];
	}

	for (i = 0; rcinfo != NULL && i < rcinfo->len; i++) {
		if (chan == rcinfo->rctbl[i].chan)
			return (rcinfo->rctbl[i].rclass);
	}

	WL_INFORM(("wl%d: No regulatory class assigned for %s channel %d\n",
		wlc_cm->wlc->pub->unit, wlc_cm->country_abbrev, chan));

	return 0;
}


/*
 * 	if (wlc->country_list_extended) all country listable.
 *	else J1 - J10 is excluded.
 */
static bool
wlc_country_listable(struct wlc_info *wlc, const char *countrystr)
{
	bool listable = TRUE;

	if (wlc->country_list_extended == FALSE) {
		if (countrystr[0] == 'J' &&
			(countrystr[1] >= '1' && countrystr[1] <= '9'))
			listable = FALSE;
	}

	return listable;
}

static bool
wlc_buffalo_map_locale(struct wlc_info *wlc, const char* abbrev)
{
	if ((wlc->pub->sih->boardvendor == VENDOR_BUFFALO) &&
	    D11REV_GT(wlc->pub->corerev, 5) && !strcmp("JP", abbrev))
		return TRUE;
	else
		return FALSE;
}

clm_country_t
wlc_get_country(struct wlc_info *wlc)
{
	return wlc->cmi->country;
}

int
wlc_get_channels_in_country(struct wlc_info *wlc, void *arg)
{
	chanvec_t channels;
	wl_channels_in_country_t *cic = (wl_channels_in_country_t *)arg;
	chanvec_t sup_chan;
	uint count, need, i;

	if ((cic->band != WLC_BAND_5G) && (cic->band != WLC_BAND_2G)) {
		WL_ERROR(("Invalid band %d\n", cic->band));
		return BCME_BADBAND;
	}

	if ((NBANDS(wlc) == 1) && (cic->band != (uint)wlc->band->bandtype)) {
		WL_ERROR(("Invalid band %d for card\n", cic->band));
		return BCME_BADBAND;
	}

	if (wlc_channel_get_chanvec(wlc, cic->country_abbrev, cic->band, &channels) == FALSE) {
		WL_ERROR(("Invalid country %s\n", cic->country_abbrev));
		return BCME_NOTFOUND;
	}

	wlc_phy_chanspec_band_validch(wlc->band->pi, cic->band, &sup_chan);
	for (i = 0; i < sizeof(chanvec_t); i++)
		sup_chan.vec[i] &= channels.vec[i];

	/* find all valid channels */
	for (count = 0, i = 0; i < sizeof(sup_chan.vec)*NBBY; i++) {
		if (isset(sup_chan.vec, i))
			count++;
	}

	need = sizeof(wl_channels_in_country_t) + count*sizeof(cic->channel[0]);

	if (need > cic->buflen) {
		/* too short, need this much */
		WL_ERROR(("WLC_GET_COUNTRY_LIST: Buffer size: Need %d Received %d\n",
			need, cic->buflen));
		cic->buflen = need;
		return BCME_BUFTOOSHORT;
	}

	for (count = 0, i = 0; i < sizeof(sup_chan.vec)*NBBY; i++) {
		if (isset(sup_chan.vec, i))
			cic->channel[count++] = i;
	}

	cic->count = count;
	return 0;
}

int
wlc_get_country_list(struct wlc_info *wlc, void *arg)
{
	chanvec_t channels;
	chanvec_t unused;
	wl_country_list_t *cl = (wl_country_list_t *)arg;
	clm_country_locales_t locale;
	chanvec_t sup_chan;
	uint count, need, j;
	clm_country_t country_iter;

	ccode_t cc;
	unsigned int regrev;

	if (cl->band_set == FALSE) {
		/* get for current band */
		cl->band = wlc->band->bandtype;
	}

	if ((cl->band != WLC_BAND_5G) && (cl->band != WLC_BAND_2G)) {
		WL_ERROR(("Invalid band %d\n", cl->band));
		return BCME_BADBAND;
	}

	if ((NBANDS(wlc) == 1) && (cl->band != (uint)wlc->band->bandtype)) {
		WL_INFORM(("Invalid band %d for card\n", cl->band));
		cl->count = 0;
		return 0;
	}

	wlc_phy_chanspec_band_validch(wlc->band->pi, cl->band, &sup_chan);

	need = sizeof(wl_country_list_t);
	count = 0;
	if (clm_iter_init(&country_iter) == CLM_RESULT_OK) {
		ccode_t prev_cc = "";
		while (clm_country_iter(&country_iter, cc, &regrev) == CLM_RESULT_OK) {
			if (wlc_get_locale(country_iter, &locale) == CLM_RESULT_OK) {
				if (cl->band == WLC_BAND_5G) {
					wlc_locale_get_channels(&locale, CLM_BAND_5G, &channels,
						&unused);
				} else {
					wlc_locale_get_channels(&locale, CLM_BAND_2G, &channels,
						&unused);
				}
				for (j = 0; j < sizeof(sup_chan.vec); j++) {
					if (sup_chan.vec[j] & channels.vec[j]) {
						if ((wlc_country_listable(wlc, cc)) &&
							memcmp(cc, prev_cc,
							sizeof(ccode_t))) {
							memcpy(prev_cc, cc, sizeof(ccode_t));
							need += WLC_CNTRY_BUF_SZ;
							if (need > cl->buflen) {
								continue;
							}
							strncpy(&cl->country_abbrev
								[count*WLC_CNTRY_BUF_SZ],
								cc, sizeof(ccode_t));
							/* terminate the string */
							cl->country_abbrev[count*WLC_CNTRY_BUF_SZ +
								sizeof(ccode_t)] = 0;
							count++;
						}
						break;
					}
				}
			}
		}
	}

	if (need > cl->buflen) {
		/* too short, need this much */
		WL_ERROR(("WLC_GET_COUNTRY_LIST: Buffer size: Need %d Received %d\n",
			need, cl->buflen));
		cl->buflen = need;
		return BCME_BUFTOOSHORT;
	}

	cl->count = count;
	return 0;
}

/* Get regulatory max power for a given channel in a given locale.
 * for external FALSE, it returns limit for brcm hw
 * ---- for 2.4GHz channel, it returns cck limit, not ofdm limit.
 * for external TRUE, it returns 802.11d Country Information Element -
 * 	Maximum Transmit Power Level.
 */
int8
wlc_get_reg_max_power_for_channel(wlc_cm_info_t *wlc_cm, int chan, bool external)
{
	int8 maxpwr = WL_RATE_DISABLED;

	clm_country_locales_t locales;
	clm_country_t country;
	wlc_info_t *wlc = wlc_cm->wlc;

	clm_result_t result = wlc_country_lookup_direct(wlc_cm->ccode, wlc_cm->regrev, &country);

	if (result != CLM_RESULT_OK) {
		return WL_RATE_DISABLED;
	}

	result = wlc_get_locale(country, &locales);
	if (result != CLM_RESULT_OK) {
		return WL_RATE_DISABLED;
	}

	if (external) {
		int int_limit;
		if (clm_regulatory_limit(&locales, BAND_5G(wlc->band->bandtype) ?
			CLM_BAND_5G : CLM_BAND_2G, chan, &int_limit) == CLM_RESULT_OK) {
			maxpwr = (uint8)int_limit;
		}
	} else {
		clm_power_limits_t limits;
		clm_limits_params_t lim_params;

		if (clm_limits_params_init(&lim_params) == CLM_RESULT_OK) {
			if (clm_limits(&locales,
				chan <= CH_MAX_2G_CHANNEL ? CLM_BAND_2G : CLM_BAND_5G,
				chan, 0, CLM_LIMITS_TYPE_CHANNEL, &lim_params,
				&limits) == CLM_RESULT_OK) {
				int i;

				for (i = 0; i < WL_NUMRATES; i++) {
					if (maxpwr < limits.limit[i])
						maxpwr = limits.limit[i];
				}
			}
		}
	}

	return (maxpwr);
}


static bool
wlc_channel_defined_80MHz_channel(uint channel)
{
	int i;
	/* 80MHz channels in 5GHz band */
	static const uint8 defined_5g_80m_chans[] =
	        {42, 58, 106, 122, 138, 155};

	for (i = 0; i < (int)ARRAYSIZE(defined_5g_80m_chans); i++) {
		if (channel == defined_5g_80m_chans[i]) {
			return TRUE;
		}
	}
	return FALSE;
}

static bool
wlc_channel_clm_chanspec_valid(wlc_cm_info_t *wlc_cm, chanspec_t chspec)
{

	return (CHSPEC_IS2G(chspec) ||
	(CHSPEC_IS20(chspec) &&
	 isset(wlc_cm->allowed_5g_20channels.vec, CHSPEC_CHANNEL(chspec))) ||
	 isset(wlc_cm->allowed_5g_4080channels.vec, CHSPEC_CHANNEL(chspec)));
}
/*
 * Validate the chanspec for this locale, for 40MHz we need to also check that the sidebands
 * are valid 20MHz channels in this locale and they are also a legal HT combination
 */
static bool
wlc_valid_chanspec_ext(wlc_cm_info_t *wlc_cm, chanspec_t chspec, bool dualband)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	uint8 channel = CHSPEC_CHANNEL(chspec);

	/* check the chanspec */
	if (wf_chspec_malformed(chspec)) {
		WL_ERROR(("wl%d: malformed chanspec 0x%x\n", wlc->pub->unit, chspec));
		ASSERT(0);
		return FALSE;
	}

	/* check channel range is in band range */
	if (CHANNEL_BANDUNIT(wlc_cm->wlc, channel) != CHSPEC_WLCBANDUNIT(chspec))
		return FALSE;


	/* Check a 20Mhz channel */
	if (CHSPEC_IS20(chspec)) {
		if (dualband)
			return (VALID_CHANNEL20_DB(wlc_cm->wlc, channel));
		else
			return (VALID_CHANNEL20(wlc_cm->wlc, channel));
	}

	/* Check a 40Mhz channel */
	if (CHSPEC_IS40(chspec)) {
		uint8 upper_sideband = 0, idx;
		uint8 num_ch20_entries = sizeof(chan20_info)/sizeof(struct chan20_info);

		if (!wlc->pub->phy_bw40_capable) {
			return FALSE;
		}

		if (!VALID_40CHANSPEC_IN_BAND(wlc, CHSPEC_WLCBANDUNIT(chspec)))
			return FALSE;

		if (dualband) {
			if (!VALID_CHANNEL20_DB(wlc, LOWER_20_SB(channel)) ||
			    !VALID_CHANNEL20_DB(wlc, UPPER_20_SB(channel)))
				return FALSE;
		} else {
			if (!VALID_CHANNEL20(wlc, LOWER_20_SB(channel)) ||
			    !VALID_CHANNEL20(wlc, UPPER_20_SB(channel)))
				return FALSE;
		}

		if (!wlc_channel_clm_chanspec_valid(wlc_cm, chspec))
			return FALSE;

		/* find the lower sideband info in the sideband array */
		for (idx = 0; idx < num_ch20_entries; idx++) {
			if (chan20_info[idx].sb == LOWER_20_SB(channel))
				upper_sideband = chan20_info[idx].adj_sbs;
		}
		/* check that the lower sideband allows an upper sideband */
		if ((upper_sideband & (CH_UPPER_SB | CH_EWA_VALID)) == (CH_UPPER_SB | CH_EWA_VALID))
			return TRUE;
		return FALSE;
	}

	/* Check a 80MHz channel - only 5G band supports 80MHz */

	if (CHSPEC_IS80(chspec)) {
		chanspec_t chspec40;

		/* Only 5G supports 80MHz
		 * Check the chanspec band with BAND_5G() instead of the more straightforward
		 * CHSPEC_IS5G() since BAND_5G() is conditionally compiled on BAND5G support. This
		 * check will turn into a constant check when compiling without BAND5G support.
		 */
		if (!BAND_5G(CHSPEC2WLC_BAND(chspec))) {
			return FALSE;
		}

		/* Make sure that the phy is 80MHz capable and that
		 * we are configured for 80MHz on the band
		 */
		if (!wlc->pub->phy_bw80_capable ||
		    !WL_BW_CAP_80MHZ(wlc->bandstate[BAND_5G_INDEX]->bw_cap)) {
			return FALSE;
		}

		/* Ensure that vhtmode is enabled if applicable */
		if (!VHT_ENAB_BAND(wlc->pub, WLC_BAND_5G)) {
			return FALSE;
		}

		if (!VALID_80CHANSPEC_IN_BAND(wlc, CHSPEC_WLCBANDUNIT(chspec)))
			return FALSE;

		/* Check that the 80MHz center channel is a defined channel */
		if (!wlc_channel_defined_80MHz_channel(channel)) {
			return FALSE;
		}

		if (!wlc_channel_clm_chanspec_valid(wlc_cm, chspec))
			return FALSE;

		/* Make sure both 40 MHz side channels are valid
		 * Create a chanspec for each 40MHz side side band and check
		 */
		chspec40 = (chanspec_t)((channel - CH_20MHZ_APART) |
		                        WL_CHANSPEC_CTL_SB_L |
		                        WL_CHANSPEC_BW_40 |
		                        WL_CHANSPEC_BAND_5G);
		if (!wlc_valid_chanspec_ext(wlc_cm, chspec40, dualband)) {
			WL_TMP(("wl%d: %s: 80MHz: chanspec %0X -> chspec40 %0X "
			        "failed valid check\n",
			        wlc->pub->unit, __FUNCTION__, chspec, chspec40));

			return FALSE;
		}

		chspec40 = (chanspec_t)((channel + CH_20MHZ_APART) |
		                        WL_CHANSPEC_CTL_SB_L |
		                        WL_CHANSPEC_BW_40 |
		                        WL_CHANSPEC_BAND_5G);
		if (!wlc_valid_chanspec_ext(wlc_cm, chspec40, dualband)) {
			WL_TMP(("wl%d: %s: 80MHz: chanspec %0X -> chspec40 %0X "
			        "failed valid check\n",
			        wlc->pub->unit, __FUNCTION__, chspec, chspec40));

			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

bool
wlc_valid_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec)
{
	return wlc_valid_chanspec_ext(wlc_cm, chspec, FALSE);
}

bool
wlc_valid_chanspec_db(wlc_cm_info_t *wlc_cm, chanspec_t chspec)
{
	return wlc_valid_chanspec_ext(wlc_cm, chspec, TRUE);
}

/*
 *  Fill in 'list' with validated chanspecs, looping through channels using the chanspec_mask.
 */
static void
wlc_chanspec_list(wlc_info_t *wlc, wl_uint32_list_t *list, chanspec_t chanspec_mask)
{
	uint8 channel;
	chanspec_t chanspec;

	for (channel = 0; channel < MAXCHANNEL; channel++) {
		chanspec = (chanspec_mask | channel);
		if (!wf_chspec_malformed(chanspec) &&
		    ((NBANDS(wlc) > 1) ? wlc_valid_chanspec_db(wlc->cmi, chanspec) :
		     wlc_valid_chanspec(wlc->cmi, chanspec))) {
			list->element[list->count] = chanspec;
			list->count++;
		}
	}
}

/*
 * Returns a list of valid chanspecs meeting the provided settings
 */
void
wlc_get_valid_chanspecs(wlc_cm_info_t *wlc_cm, wl_uint32_list_t *list, uint bw, bool band2G,
                        const char *abbrev)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	chanspec_t chanspec;
	clm_country_t country;
	clm_result_t result = CLM_RESULT_ERR;
	clm_result_t flag_result = CLM_RESULT_ERR;
	uint8 flags;
	clm_country_locales_t locale;
	chanvec_t saved_valid_channels, saved_db_valid_channels, unused;
#ifdef WL11N
	uint8 saved_locale_flags = 0,  saved_db_locale_flags = 0;
	bool saved_cap_40 = TRUE, saved_db_cap_40 = TRUE;
#endif /* WL11N */
#ifdef WL11AC
	bool saved_cap_80;
#endif /* WL11AC */

	/* for AC, we modified the bw definition, not previous bool bw20 anymore */
	ASSERT((bw != TRUE) && (bw != FALSE));

	/* Check if this is a valid band for this card */
	if ((NBANDS(wlc) == 1) &&
	    (BAND_5G(wlc->band->bandtype) == band2G))
		return;

	/* see if we need to look up country. Else, current locale */
	if (strcmp(abbrev, "")) {
		result = wlc_country_lookup(wlc, abbrev, &country);
		if (result != CLM_RESULT_OK) {
			WL_ERROR(("Invalid country \"%s\"\n", abbrev));
			return;
		}
		result = wlc_get_locale(country, &locale);

		flag_result = wlc_get_flags(&locale, band2G ? CLM_BAND_2G : CLM_BAND_5G, &flags);
		BCM_REFERENCE(flag_result);
	}

	/* Save current locales */
	if (result == CLM_RESULT_OK) {
		clm_band_t tmp_band = band2G ? CLM_BAND_2G : CLM_BAND_5G;
		bcopy(&wlc->cmi->bandstate[wlc->band->bandunit].valid_channels,
			&saved_valid_channels, sizeof(chanvec_t));
		wlc_locale_get_channels(&locale, tmp_band,
			&wlc->cmi->bandstate[wlc->band->bandunit].valid_channels, &unused);
		if (NBANDS(wlc) > 1) {
			bcopy(&wlc->cmi->bandstate[OTHERBANDUNIT(wlc)].valid_channels,
			      &saved_db_valid_channels, sizeof(chanvec_t));
			wlc_locale_get_channels(&locale, tmp_band,
			      &wlc->cmi->bandstate[OTHERBANDUNIT(wlc)].valid_channels, &unused);
		}
	}

#ifdef WL11N
	if (result == CLM_RESULT_OK) {
		saved_locale_flags = wlc_cm->bandstate[wlc->band->bandunit].locale_flags;
		wlc_cm->bandstate[wlc->band->bandunit].locale_flags = flags;

		if (NBANDS(wlc) > 1) {
			saved_db_locale_flags = wlc_cm->bandstate[OTHERBANDUNIT(wlc)].locale_flags;
			wlc_cm->bandstate[OTHERBANDUNIT(wlc)].locale_flags = flags;
		}
	}

	/* save 40 MHz cap */
	saved_cap_40 = WL_BW_CAP_40MHZ(wlc->band->bw_cap);
	wlc->band->bw_cap |= WLC_BW_40MHZ_BIT;
	if (NBANDS(wlc) > 1) {
		saved_db_cap_40 = WL_BW_CAP_40MHZ(wlc->bandstate[OTHERBANDUNIT(wlc)]->bw_cap);
		wlc->bandstate[OTHERBANDUNIT(wlc)]->bw_cap |= WLC_BW_40MHZ_BIT;
	}

#ifdef WL11AC
	/* save 80 MHz cap */
	saved_cap_80 = WL_BW_CAP_80MHZ(wlc->bandstate[BAND_5G_INDEX]->bw_cap);
	wlc->bandstate[BAND_5G_INDEX]->bw_cap |= WLC_BW_80MHZ_BIT;
#endif /* WL11AC */

#endif /* WL11N */

	/* Go through 2G 20MHZ chanspecs */
	if (band2G && bw == WL_CHANSPEC_BW_20) {
		chanspec = WL_CHANSPEC_BAND_2G | WL_CHANSPEC_BW_20;
		wlc_chanspec_list(wlc, list, chanspec);
	}

	/* Go through 5G 20 MHZ chanspecs */
	if (!band2G && bw == WL_CHANSPEC_BW_20) {
		chanspec = WL_CHANSPEC_BAND_5G | WL_CHANSPEC_BW_20;
		wlc_chanspec_list(wlc, list, chanspec);
	}

#ifdef WL11N
	/* Go through 2G 40MHZ chanspecs only if N mode and PHY is capable of 40MHZ */
	if (band2G && bw == WL_CHANSPEC_BW_40 &&
	    N_ENAB(wlc->pub) && wlc->pub->phy_bw40_capable) {
		chanspec = WL_CHANSPEC_BAND_2G | WL_CHANSPEC_BW_40 | WL_CHANSPEC_CTL_SB_UPPER;
		wlc_chanspec_list(wlc, list, chanspec);
		chanspec = WL_CHANSPEC_BAND_2G | WL_CHANSPEC_BW_40 | WL_CHANSPEC_CTL_SB_LOWER;
		wlc_chanspec_list(wlc, list, chanspec);
	}

	/* Go through 5G 40MHZ chanspecs only if N mode and PHY is capable of 40MHZ  */
	if (!band2G && bw == WL_CHANSPEC_BW_40 &&
	    N_ENAB(wlc->pub) && ((NBANDS(wlc) > 1) || IS_SINGLEBAND_5G(wlc->deviceid)) &&
	    wlc->pub->phy_bw40_capable) {
		chanspec = WL_CHANSPEC_BAND_5G | WL_CHANSPEC_BW_40 | WL_CHANSPEC_CTL_SB_UPPER;
		wlc_chanspec_list(wlc, list, chanspec);
		chanspec = WL_CHANSPEC_BAND_5G | WL_CHANSPEC_BW_40 | WL_CHANSPEC_CTL_SB_LOWER;
		wlc_chanspec_list(wlc, list, chanspec);
	}

#ifdef WL11AC
	/* Go through 5G 80MHZ chanspecs only if VHT mode and PHY is capable of 80MHZ  */
	if (!band2G && bw == WL_CHANSPEC_BW_80 && VHT_ENAB_BAND(wlc->pub, WLC_BAND_5G) &&
		((NBANDS(wlc) > 1) || IS_SINGLEBAND_5G(wlc->deviceid)) &&
		wlc->pub->phy_bw80_capable) {

		int i;
		uint16 ctl_sb[] = {
			WL_CHANSPEC_CTL_SB_LL,
			WL_CHANSPEC_CTL_SB_LU,
			WL_CHANSPEC_CTL_SB_UL,
			WL_CHANSPEC_CTL_SB_UU
		};

		for (i = 0; i < 4; i++) {
			chanspec = WL_CHANSPEC_BAND_5G | WL_CHANSPEC_BW_80 | ctl_sb[i];
			wlc_chanspec_list(wlc, list, chanspec);
		}
	}

	/* restore 80 MHz cap */
	if (saved_cap_80)
		wlc->bandstate[BAND_5G_INDEX]->bw_cap |= WLC_BW_80MHZ_BIT;
	else
		wlc->bandstate[BAND_5G_INDEX]->bw_cap &= ~WLC_BW_80MHZ_BIT;
#endif /* WL11AC */

	/* restore 40 MHz cap */
	if (saved_cap_40)
		wlc->band->bw_cap |= WLC_BW_40MHZ_BIT;
	else
		wlc->band->bw_cap &= ~WLC_BW_40MHZ_BIT;

	if (NBANDS(wlc) > 1) {
		if (saved_db_cap_40) {
			wlc->bandstate[OTHERBANDUNIT(wlc)]->bw_cap |= WLC_BW_CAP_40MHZ;
		} else {
			wlc->bandstate[OTHERBANDUNIT(wlc)]->bw_cap &= ~WLC_BW_CAP_40MHZ;
		}
	}

	if (result == CLM_RESULT_OK) {
		wlc_cm->bandstate[wlc->band->bandunit].locale_flags = saved_locale_flags;
		if ((NBANDS(wlc) > 1))
			wlc_cm->bandstate[OTHERBANDUNIT(wlc)].locale_flags = saved_db_locale_flags;
	}
#endif /* WL11N */

	/* Restore the locales if switched */
	if (result == CLM_RESULT_OK) {
		bcopy(&saved_valid_channels,
			&wlc->cmi->bandstate[wlc->band->bandunit].valid_channels,
			sizeof(chanvec_t));
		if ((NBANDS(wlc) > 1))
			bcopy(&saved_db_valid_channels,
			      &wlc->cmi->bandstate[OTHERBANDUNIT(wlc)].valid_channels,
			      sizeof(chanvec_t));
	}
}

/* query the channel list given a country and a regulatory class */
uint8
wlc_rclass_get_channel_list(wlc_cm_info_t *cmi, const char *abbrev, uint8 rclass,
	bool bw20, wl_uint32_list_t *list)
{
	const rcinfo_t *rcinfo = NULL;
	uint8 ch2g_start = 0, ch2g_end = 0;
	int i;

	if (wlc_us_ccode(abbrev)) {
		if (rclass == WLC_REGCLASS_USA_2G_20MHZ) {
			ch2g_start = 1;
			ch2g_end = 11;
		}
#ifdef BAND5G
		else
			rcinfo = &rcinfo_us_20;
#endif
	} else if (wlc_japan_ccode(abbrev)) {
		if (rclass == WLC_REGCLASS_JPN_2G_20MHZ) {
			ch2g_start = 1;
			ch2g_end = 13;
		}
		else if (rclass == WLC_REGCLASS_JPN_2G_20MHZ_CH14) {
			ch2g_start = 14;
			ch2g_end = 14;
		}
#ifdef BAND5G
		else
			rcinfo = &rcinfo_jp_20;
#endif
	} else {
		if (rclass == WLC_REGCLASS_EUR_2G_20MHZ) {
			ch2g_start = 1;
			ch2g_end = 13;
		}
#ifdef BAND5G
		else
			rcinfo = &rcinfo_eu_20;
#endif
	}

	list->count = 0;
	if (rcinfo == NULL) {
		for (i = ch2g_start; i <= ch2g_end; i ++)
			list->element[list->count ++] = i;
	}
	else {
		for (i = 0; i < rcinfo->len; i ++) {
			if (rclass == rcinfo->rctbl[i].rclass)
				list->element[list->count ++] = rcinfo->rctbl[i].chan;
		}
	}

	return (uint8)list->count;
}

/* Return true if the channel is a valid channel that is radar sensitive
 * in the current country/locale
 */
bool
wlc_radar_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec)
{
#ifdef BAND5G /* RADAR */
	uint channel = CHSPEC_CHANNEL(chspec);
	const chanvec_t *radar_channels;

	/* The radar_channels chanvec may be a superset of valid channels,
	 * so be sure to check for a valid channel first.
	 */

	if (!chspec || !wlc_valid_chanspec_db(wlc_cm, chspec)) {
		return FALSE;
	}

	if (CHSPEC_IS5G(chspec)) {
		radar_channels = wlc_cm->bandstate[BAND_5G_INDEX].radar_channels;

		if (CHSPEC_IS80(chspec)) {
			int i;

			/* start at the lower edge 20MHz channel */
			channel = LOWER_40_SB(channel); /* low 40MHz sb of the 80 */
			channel = LOWER_20_SB(channel); /* low 20MHz sb of the 40 */

			/* work through each 20MHz channel in the 80MHz */
			for (i = 0; i < 4; i++, channel += CH_20MHZ_APART) {
				if (isset(radar_channels->vec, channel)) {
					return TRUE;
				}
			}
		} else if (CHSPEC_IS40(chspec)) {
			if (isset(radar_channels->vec, LOWER_20_SB(channel)) ||
			    isset(radar_channels->vec, UPPER_20_SB(channel)))
				return TRUE;
		} else if (isset(radar_channels->vec, channel)) {
			return TRUE;
		}
	}
#endif	/* BAND5G */
	return FALSE;
}

/* Return true if the channel is a valid channel that is radar sensitive
 * in the current country/locale
 */
bool
wlc_restricted_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec)
{
	uint channel = CHSPEC_CHANNEL(chspec);
	chanvec_t *restricted_channels;

	/* The restriced_channels chanvec may be a superset of valid channels,
	 * so be sure to check for a valid channel first.
	 */

	if (!chspec || !wlc_valid_chanspec_db(wlc_cm, chspec)) {
		return FALSE;
	}

	restricted_channels = &wlc_cm->restricted_channels;

	if (CHSPEC_IS80(chspec)) {
		if (isset(restricted_channels->vec, LL_20_SB(channel)) ||
		    isset(restricted_channels->vec, LU_20_SB(channel)) ||
			isset(restricted_channels->vec, UL_20_SB(channel)) ||
		    isset(restricted_channels->vec, UU_20_SB(channel)))
			return TRUE;
	} else if (CHSPEC_IS40(chspec)) {
		if (isset(restricted_channels->vec, LOWER_20_SB(channel)) ||
		    isset(restricted_channels->vec, UPPER_20_SB(channel)))
			return TRUE;
	} else if (isset(restricted_channels->vec, channel)) {
		return TRUE;
	}

	return FALSE;
}

void
wlc_clr_restricted_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec)
{
	if (CHSPEC_IS40_UNCOND(chspec)) {
		clrbit(wlc_cm->restricted_channels.vec, LOWER_20_SB(CHSPEC_CHANNEL(chspec)));
		clrbit(wlc_cm->restricted_channels.vec, UPPER_20_SB(CHSPEC_CHANNEL(chspec)));
	} else
		clrbit(wlc_cm->restricted_channels.vec, CHSPEC_CHANNEL(chspec));

	wlc_upd_restricted_chanspec_flag(wlc_cm);
}

static void
wlc_upd_restricted_chanspec_flag(wlc_cm_info_t *wlc_cm)
{
	uint j;

	for (j = 0; j < (int)sizeof(chanvec_t); j++)
		if (wlc_cm->restricted_channels.vec[j]) {
			wlc_cm->has_restricted_ch = TRUE;
			return;
		}

	wlc_cm->has_restricted_ch = FALSE;
}

bool
wlc_has_restricted_chanspec(wlc_cm_info_t *wlc_cm)
{
	return wlc_cm->has_restricted_ch;
}


#ifndef INT8_MIN
#define INT8_MIN 0x80
#endif
#ifndef INT8_MAX
#define INT8_MAX 0x7F
#endif

static void
wlc_channel_margin_summary_mapfn(void *context, uint8 *a, uint8 *b)
{
	uint8 margin;
	uint8 *pmin = (uint8*)context;

	if (*a > *b)
		margin = *a - *b;
	else
		margin = 0;

	*pmin = MIN(*pmin, margin);
}

/* Map the given function with its context value over the power targets
 * appropriate for the given band and bandwidth in two txppr structs.
 * If the band is 2G, DSSS/CCK rates will be included.
 * If the bandwidth is 20MHz, only 20MHz targets are included.
 * If the bandwidth is 40MHz, both 40MHz and 20in40 targets are included.
 */
static void
wlc_channel_map_txppr_binary(ppr_mapfn_t fn, void* context, uint bandtype, uint bw,
	ppr_t *a, ppr_t *b, wlc_info_t *wlc)
{
	if (bw == WL_CHANSPEC_BW_20) {
		if (bandtype == WL_CHANSPEC_BAND_2G)
			ppr_map_vec_dsss(fn, context, a, b, WL_TX_BW_20, WL_TX_CHAINS_1);
		ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20, WL_TX_MODE_NONE, WL_TX_CHAINS_1);
	}

#ifdef WL11N
	/* map over 20MHz rates for 20MHz channels */
	if (bw == WL_CHANSPEC_BW_20) {
		if (PHYCORENUM(wlc->stf->txstreams) > 1) {
			if (bandtype == WL_CHANSPEC_BAND_2G) {
				ppr_map_vec_dsss(fn, context, a, b, WL_TX_BW_20, WL_TX_CHAINS_2);
				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_map_vec_dsss(fn, context, a, b,
						WL_TX_BW_20, WL_TX_CHAINS_3);
				}
			}
		}
		ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_1,
			WL_TX_MODE_NONE, WL_TX_CHAINS_1);

		if (PHYCORENUM(wlc->stf->txstreams) > 1) {
			ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20, WL_TX_MODE_CDD,
				WL_TX_CHAINS_2);
			ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_1,
				WL_TX_MODE_CDD, WL_TX_CHAINS_2);
			ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_2,
				WL_TX_MODE_STBC, WL_TX_CHAINS_2);
			ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_2,
				WL_TX_MODE_NONE, WL_TX_CHAINS_2);

			if (PHYCORENUM(wlc->stf->txstreams) > 2) {
				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_1,
					WL_TX_MODE_CDD, WL_TX_CHAINS_3);
				ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20, WL_TX_MODE_CDD,
					WL_TX_CHAINS_3);

				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_2,
					WL_TX_MODE_STBC, WL_TX_CHAINS_3);
				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_2,
					WL_TX_MODE_NONE, WL_TX_CHAINS_3);

				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_3,
					WL_TX_MODE_NONE, WL_TX_CHAINS_3);
			}
		}
	} else
	/* map over 40MHz and 20in40 rates for 40MHz channels */
	{
		ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_40, WL_TX_MODE_NONE, WL_TX_CHAINS_1);
		ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_NONE,
			WL_TX_CHAINS_1);

		if (PHYCORENUM(wlc->stf->txstreams) > 1) {
			ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_40, WL_TX_MODE_CDD,
				WL_TX_CHAINS_2);
			ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_1,
				WL_TX_MODE_CDD, WL_TX_CHAINS_2);
			ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_2,
				WL_TX_MODE_STBC, WL_TX_CHAINS_2);
			ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_2,
				WL_TX_MODE_NONE, WL_TX_CHAINS_2);

			if (PHYCORENUM(wlc->stf->txstreams) > 2) {
				ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_40, WL_TX_MODE_CDD,
					WL_TX_CHAINS_3);
				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_1,
					WL_TX_MODE_CDD, WL_TX_CHAINS_3);

				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_2,
					WL_TX_MODE_STBC, WL_TX_CHAINS_3);
				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_2,
					WL_TX_MODE_NONE, WL_TX_CHAINS_3);

				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_3,
					WL_TX_MODE_NONE, WL_TX_CHAINS_3);
			}
		}
		/* 20in40 legacy */
		if (bandtype == WL_CHANSPEC_BAND_2G) {
			ppr_map_vec_dsss(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_CHAINS_1);
			if (PHYCORENUM(wlc->stf->txstreams) > 1) {
				ppr_map_vec_dsss(fn, context, a, b, WL_TX_BW_20IN40,
					WL_TX_CHAINS_2);
				if (PHYCORENUM(wlc->stf->txstreams) > 2) {
					ppr_map_vec_dsss(fn, context, a, b, WL_TX_BW_20IN40,
						WL_TX_CHAINS_3);
				}
			}
		}

		ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_MODE_NONE,
			WL_TX_CHAINS_1);

		/* 20in40 HT */
		ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_1, WL_TX_MODE_NONE,
			WL_TX_CHAINS_1);

		if (PHYCORENUM(wlc->stf->txstreams) > 1) {
			ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_1,
				WL_TX_MODE_CDD,	WL_TX_CHAINS_2);
			ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_MODE_CDD,
				WL_TX_CHAINS_2);
			ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_2,
				WL_TX_MODE_STBC, WL_TX_CHAINS_2);
			ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_2,
				WL_TX_MODE_NONE, WL_TX_CHAINS_2);

			if (PHYCORENUM(wlc->stf->txstreams) > 2) {
				ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_MODE_CDD,
					WL_TX_CHAINS_3);
				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_1,
					WL_TX_MODE_CDD, WL_TX_CHAINS_3);

				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_2,
					WL_TX_MODE_STBC, WL_TX_CHAINS_3);
				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_2,
					WL_TX_MODE_NONE, WL_TX_CHAINS_3);
				ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_3,
					WL_TX_MODE_NONE, WL_TX_CHAINS_3);
			}
		}
	}
#endif /* WL11N */
}

/* calculate the offset from each per-rate power target in txpwr to the supplied
 * limit (or zero if txpwr[x] is less than limit[x]), and return the smallest
 * offset of relevant rates for bandtype/bw.
 */
static uint8
wlc_channel_txpwr_margin(ppr_t *txpwr, ppr_t *limit, uint bandtype, uint bw, wlc_info_t *wlc)
{
	uint8 margin = 0xff;

	wlc_channel_map_txppr_binary(wlc_channel_margin_summary_mapfn, &margin,
	                             bandtype, bw, txpwr, limit, wlc);
	return margin;
}

/* return a ppr_t struct with the phy srom limits for the given channel */
static void
wlc_channel_srom_limits(wlc_cm_info_t *wlc_cm, chanspec_t chanspec,
	ppr_t *srommin, ppr_t *srommax)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	wlc_phy_t *pi = wlc->band->pi;
	uint8 min_srom;

	if (srommin != NULL)
		ppr_clear(srommin);
	if (srommax != NULL)
		ppr_clear(srommax);

	if (!PHYTYPE_HT_CAP(wlc_cm->wlc->band))
		return;

	wlc_phy_txpower_sromlimit(pi, chanspec, &min_srom, srommax, 0);
	if (srommin != NULL)
		ppr_set_cmn_val(srommin, min_srom);
}

/* Set a per-chain power limit for the given band
 * Per-chain offsets will be used to make sure the max target power does not exceed
 * the per-chain power limit
 */
int
wlc_channel_band_chain_limit(wlc_cm_info_t *wlc_cm, uint bandtype,
                             struct wlc_channel_txchain_limits *lim)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	ppr_t* txpwr;
	int bandunit = (bandtype == WLC_BAND_2G) ? BAND_2G_INDEX : BAND_5G_INDEX;

	if (!PHYTYPE_HT_CAP(wlc_cm->wlc->band))
		return BCME_UNSUPPORTED;

	wlc_cm->bandstate[bandunit].chain_limits = *lim;

	if (CHSPEC_WLCBANDUNIT(wlc->chanspec) != bandunit)
		return 0;

	if ((txpwr = ppr_create(wlc_cm->pub->osh, PPR_CHSPEC_BW(wlc->chanspec))) == NULL) {
		return 0;
	}

	/* update the current tx chain offset if we just updated this band's limits */
	wlc_channel_txpower_limits(wlc_cm, txpwr);
	wlc_channel_update_txchain_offsets(wlc_cm, txpwr);

	ppr_delete(wlc_cm->pub->osh, txpwr);
	return 0;
}

/* update the per-chain tx power offset given the current power targets to implement
 * the correct per-chain tx power limit
 */
static int
wlc_channel_update_txchain_offsets(wlc_cm_info_t *wlc_cm, ppr_t *txpwr)
{
	wlc_info_t *wlc = wlc_cm->wlc;
	struct wlc_channel_txchain_limits *lim;
	wl_txchain_pwr_offsets_t offsets;
	chanspec_t chanspec;
	ppr_t* srompwr;
	int i, err;
	int max_pwr;
	int band, bw;
	int limits_present = FALSE;
	uint8 delta, margin, err_margin;
	wl_txchain_pwr_offsets_t cur_offsets;
#if defined WLTXPWR_CACHE && defined(WLC_LOW) && defined(WL11N)
	tx_pwr_cache_entry_t* cacheptr = wlc_phy_get_txpwr_cache(wlc->band->pi);
#endif

	if (!PHYTYPE_HT_CAP(wlc->band))
		return BCME_UNSUPPORTED;

	chanspec = wlc->chanspec;

#if defined WLTXPWR_CACHE && defined(WLC_LOW) && defined(WL11N)
	if ((wlc_phy_txpwr_cache_is_cached(cacheptr, chanspec) == TRUE) &&
		(wlc_phy_get_cached_txchain_offsets(cacheptr, chanspec, 0) != WL_RATE_DISABLED)) {

		for (i = 0; i < WLC_CHAN_NUM_TXCHAIN; i++) {
			offsets.offset[i] =
				wlc_phy_get_cached_txchain_offsets(cacheptr, chanspec, i);
		}

	/* always set, at least for the moment */
		err = wlc_stf_txchain_pwr_offset_set(wlc, &offsets);

		if (err) {
			WL_ERROR(("wl%d: txchain_pwr_offset failed: error %d\n",
				wlc->pub->unit, err));
		}

		return err;
	}
#endif /* WLTXPWR_CACHE && WLC_LOW && WL11N */

	band = CHSPEC_BAND(chanspec);
	bw = CHSPEC_BW(chanspec);

	if ((srompwr = ppr_create(wlc_cm->pub->osh, PPR_CHSPEC_BW(chanspec))) == NULL) {
		return BCME_ERROR;
	}
	/* initialize the offsets to a default of no offset */
	memset(&offsets, 0, sizeof(wl_txchain_pwr_offsets_t));

	lim = &wlc_cm->bandstate[CHSPEC_WLCBANDUNIT(chanspec)].chain_limits;

	/* see if there are any chain limits specified */
	for (i = 0; i < WLC_CHAN_NUM_TXCHAIN; i++) {
		if (lim->chain_limit[i] < WLC_TXPWR_MAX) {
			limits_present = TRUE;
			break;
		}
	}

	/* if there are no limits, we do not need to do any calculations */
	if (limits_present) {

#ifdef WLTXPWR_CACHE
		ASSERT(txpwr != NULL);
#endif
		/* find the max power target for this channel and impose
		 * a txpwr delta per chain to meet the specified chain limits
		 * Bound the delta by the tx power margin
		 */

		/* get the srom min powers */
		wlc_channel_srom_limits(wlc_cm, wlc->chanspec, srompwr, NULL);

		/* find the dB margin we can use to adjust tx power */
		margin = wlc_channel_txpwr_margin(txpwr, srompwr, band, bw, wlc);

		/* reduce the margin by the error margin 1.5dB backoff */
		err_margin = 6;	/* 1.5 dB in qdBm */
		margin = (margin >= err_margin) ? margin - err_margin : 0;

		/* get the srom max powers */
		wlc_channel_srom_limits(wlc_cm, wlc->chanspec, NULL, srompwr);

		/* combine the srom limits with the given regulatory limits
		 * to find the actual channel max
		 */
		/* wlc_channel_txpwr_vec_combine_min(srompwr, txpwr); */
		ppr_apply_vector_ceiling(srompwr, txpwr);

		/* max_pwr = (int)wlc_channel_txpwr_max(srompwr, band, bw, wlc); */
		max_pwr = (int)ppr_get_max_for_bw(srompwr, PPR_CHSPEC_BW(chanspec));
		WL_NONE(("wl%d: %s: channel %s max_pwr %d margin %d\n",
		         wlc->pub->unit, __FUNCTION__,
		         wf_chspec_ntoa(wlc->chanspec, chanbuf), max_pwr, margin));

		/* for each chain, calculate an offset that keeps the max tx power target
		 * no greater than the chain limit
		 */
		for (i = 0; i < WLC_CHAN_NUM_TXCHAIN; i++) {
			WL_NONE(("wl%d: %s: chain_limit[%d] %d",
			         wlc->pub->unit, __FUNCTION__,
			         i, lim->chain_limit[i]));
			if (lim->chain_limit[i] < max_pwr) {
				delta = max_pwr - lim->chain_limit[i];

				WL_NONE((" desired delta -%u lim delta -%u",
				         delta, MIN(delta, margin)));

				/* limit to the margin allowed for our adjustmets */
				delta = MIN(delta, margin);

				offsets.offset[i] = -delta;
			}
			WL_NONE(("\n"));
		}
	} else {
		WL_NONE(("wl%d: %s skipping limit calculation since limits are MAX\n",
		         wlc->pub->unit, __FUNCTION__));
	}

#ifdef WLTXPWR_CACHE
	if (wlc_phy_txpwr_cache_is_cached(cacheptr, chanspec) == TRUE) {
		for (i = 0; i < WLC_CHAN_NUM_TXCHAIN; i++) {
			wlc_phy_set_cached_txchain_offsets(cacheptr, chanspec, i,
				offsets.offset[i]);
		}
	}
#endif

	err = wlc_iovar_op(wlc, "txchain_pwr_offset", NULL, 0,
	                   &cur_offsets, sizeof(wl_txchain_pwr_offsets_t), IOV_GET, NULL);

	if (!err && bcmp(&cur_offsets.offset, &offsets.offset, WL_NUM_TXCHAIN_MAX)) {

		err = wlc_iovar_op(wlc, "txchain_pwr_offset", NULL, 0,
			&offsets, sizeof(wl_txchain_pwr_offsets_t), IOV_SET, NULL);
	}

	if (err) {
		WL_ERROR(("wl%d: txchain_pwr_offset failed: error %d\n",
		          wlc->pub->unit, err));
	}

	if (srompwr != NULL)
		ppr_delete(wlc_cm->pub->osh, srompwr);
	return err;
}


void
wlc_dump_clmver(wlc_cm_info_t *wlc_cm, struct bcmstrbuf *b)
{
	struct clm_data_header* clm_inc_headerptr = wlc_cm->clm_inc_headerptr;
	const struct clm_data_header* clm_base_headerptr = wlc_cm->clm_base_dataptr;
	const char* verstrptr;

	if (clm_base_headerptr == NULL)
		clm_base_headerptr = &clm_header;

	bcm_bprintf(b, "API: %d.%d\nData: %s\nCompiler: %s\n%s\n",
		clm_base_headerptr->format_major, clm_base_headerptr->format_minor,
		clm_base_headerptr->clm_version, clm_base_headerptr->compiler_version,
		clm_base_headerptr->generator_version);
	verstrptr = clm_get_base_app_version_string();
	if (verstrptr != NULL)
		bcm_bprintf(b, "Customization: %s\n", verstrptr);

	if (clm_inc_headerptr != NULL) {
		bcm_bprintf(b, "Inc Data: %s\nInc Compiler: %s\nInc %s\n",
			clm_inc_headerptr->clm_version,
			clm_inc_headerptr->compiler_version, clm_inc_headerptr->generator_version);
		verstrptr = clm_get_inc_app_version_string();
		if (verstrptr != NULL)
			bcm_bprintf(b, "Inc Customization: %s\n", verstrptr);
	}
}

void wlc_channel_update_txpwr_limit(wlc_info_t *wlc)
{
	ppr_t *txpwr;
	if ((txpwr = ppr_create(wlc->osh, PPR_CHSPEC_BW(wlc->chanspec))) == NULL) {
		return;
	}
	wlc_channel_reg_limits(wlc->cmi, wlc->chanspec, txpwr);
	wlc_phy_txpower_limit_set(wlc->band->pi, txpwr, wlc->chanspec);
	ppr_delete(wlc->osh, txpwr);
}

#ifdef WLTDLS
/* Regulatory Class IE in TDLS Setup frames */
static uint
wlc_channel_tdls_calc_rc_ie_len(void *ctx, wlc_iem_calc_data_t *calc)
{
	wlc_cm_info_t *cmi = (wlc_cm_info_t *)ctx;
	wlc_iem_ft_cbparm_t *ftcbparm = calc->cbparm->ft;
	chanspec_t chanspec = ftcbparm->tdls.chspec;
	uint8 rclen;		/* regulatory class length */
	uint8 rclist[32];	/* regulatory class list */

	if (!isset(ftcbparm->tdls.cap, DOT11_TDLS_CAP_CH_SW))
		return 0;

	rclen = wlc_get_regclass_list(cmi, rclist, MAXRCLISTSIZE, chanspec, TRUE);

	return TLV_HDR_LEN + rclen;
}

static int
wlc_channel_tdls_write_rc_ie(void *ctx, wlc_iem_build_data_t *build)
{
	wlc_cm_info_t *cmi = (wlc_cm_info_t *)ctx;
	wlc_iem_ft_cbparm_t *ftcbparm = build->cbparm->ft;
	chanspec_t chanspec = ftcbparm->tdls.chspec;
	uint8 rclen;		/* regulatory class length */
	uint8 rclist[32];	/* regulatory class list */

	if (!isset(ftcbparm->tdls.cap, DOT11_TDLS_CAP_CH_SW)) {
		return BCME_OK;
	}

	rclen = wlc_get_regclass_list(cmi, rclist, MAXRCLISTSIZE, chanspec, TRUE);

	bcm_write_tlv_safe(DOT11_MNG_REGCLASS_ID, rclist, rclen,
		build->buf, build->buf_len);

	return BCME_OK;
}
#endif /* WLTDLS */
