/*
 * Common interface to channel definitions.
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
 * $Id: bcmwifi_rclass.c 660004 2016-09-16 23:08:05Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmwifi_rclass.h>

static bcmwifi_rclass_bvec_t g_bvec = {
	BCMWIFI_MAX_VEC_SIZE
};

/* op class 81 */
static const uint8 chan_set_2g_20[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
};

/* op class 83 */
static const uint8 chan_set_2g_40l[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9
};

/* op class 84 */
static const uint8 chan_set_2g_40u[] = {
	5, 6, 7, 8, 9, 10, 11, 12, 13
};

/* op class 112 */
static const uint8 chan_set_5g_20_1[] = {
	8, 12, 16
};

/* op class 115 */
static const uint8 chan_set_5g_20_2[] = {
	36, 40, 44, 48
};

/* op class 116 */
static const uint8 chan_set_5g_40l_1[] = {
	36, 44
};

/* op class 117 */
static const uint8 chan_set_5g_40u_1[] = {
	40, 48
};

/* op class 118 */
static const uint8 chan_set_5g_20_3[] = {
	52, 56, 60, 64
};

/* op class 119 */
static const uint8 chan_set_5g_40l_2[] = {
	52, 60
};

/* op class 120 */
static const uint8 chan_set_5g_40u_2[] = {
	56, 64
};

/* op class 121 */
static const uint8 chan_set_5g_20_4[] = {
	100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140
};

/* op class 122 */
static const uint8 chan_set_5g_40l_3[] = {
	100, 108, 116, 124, 132
};

/* op class 123 */
static const uint8 chan_set_5g_40u_3[] = {
	104, 112, 120, 128, 136
};

/* op class 124 */
static const uint8 chan_set_5g_20_5[] = {
	149, 153, 157, 161
};

/* op class 125 */
static const uint8 chan_set_5g_20_6[] = {
	149, 153, 157, 161, 165, 169
};

/* op class 126 */
static const uint8 chan_set_5g_40l_4[] = {
	149, 157
};

/* op class 127 */
static const uint8 chan_set_5g_40u_4[] = {
	153, 161
};

/* op class 128 */
static const uint8 chan_set_5g_80_1[] = {
	42, 58, 106, 122, 138, 155
};

/* op class 129 */
static const uint8 chan_set_5g_160_1[] = {
	50, 114
};

/* op class 130 */
static const uint8 chan_set_5g_80_2[] = {
	42, 58, 106, 122, 138, 155
};

static const bcmwifi_rclass_info_t rcinfo_global[] = {
	{81, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY, BCMWIFI_BW_20, BCMWIFI_BAND_2G,
	BCMWIFI_RCLASS_FLAGS_NONE, ARRAYSIZE(chan_set_2g_20), chan_set_2g_20},
	{83, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY, BCMWIFI_BW_40, BCMWIFI_BAND_2G,
	BCMWIFI_RCLASS_FLAGS_PRIMARY_LOWER, ARRAYSIZE(chan_set_2g_40l), chan_set_2g_40l},
	{84, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY, BCMWIFI_BW_40, BCMWIFI_BAND_2G,
	BCMWIFI_RCLASS_FLAGS_PRIMARY_UPPER, ARRAYSIZE(chan_set_2g_40u), chan_set_2g_40u},
	{112, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY,  BCMWIFI_BW_20, BCMWIFI_BAND_5G,
	BCMWIFI_RCLASS_FLAGS_NONE, ARRAYSIZE(chan_set_5g_20_1), chan_set_5g_20_1},
	{115, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY,  BCMWIFI_BW_20, BCMWIFI_BAND_5G,
	BCMWIFI_RCLASS_FLAGS_EIRP, ARRAYSIZE(chan_set_5g_20_2), chan_set_5g_20_2},
	{116, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY, BCMWIFI_BW_40, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_EIRP | BCMWIFI_RCLASS_FLAGS_PRIMARY_LOWER),
	ARRAYSIZE(chan_set_5g_40l_1), chan_set_5g_40l_1},
	{117, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY, BCMWIFI_BW_40, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_EIRP | BCMWIFI_RCLASS_FLAGS_PRIMARY_UPPER),
	ARRAYSIZE(chan_set_5g_40u_1), chan_set_5g_40u_1},
	{118, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY,  BCMWIFI_BW_20, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_DFS | BCMWIFI_RCLASS_FLAGS_EIRP),
	ARRAYSIZE(chan_set_5g_20_3), chan_set_5g_20_3},
	{119, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY, BCMWIFI_BW_40, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_DFS | BCMWIFI_RCLASS_FLAGS_EIRP | BCMWIFI_RCLASS_FLAGS_PRIMARY_LOWER),
	ARRAYSIZE(chan_set_5g_40l_2), chan_set_5g_40l_2},
	{120, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY, BCMWIFI_BW_40, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_DFS | BCMWIFI_RCLASS_FLAGS_EIRP | BCMWIFI_RCLASS_FLAGS_PRIMARY_UPPER),
	ARRAYSIZE(chan_set_5g_40u_2), chan_set_5g_40u_2},
	{121, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY,  BCMWIFI_BW_20, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_DFS | BCMWIFI_RCLASS_FLAGS_EIRP),
	ARRAYSIZE(chan_set_5g_20_4), chan_set_5g_20_4},
	{122, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY, BCMWIFI_BW_40, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_DFS | BCMWIFI_RCLASS_FLAGS_EIRP | BCMWIFI_RCLASS_FLAGS_PRIMARY_LOWER),
	ARRAYSIZE(chan_set_5g_40l_3), chan_set_5g_40l_3},
	{123, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY, BCMWIFI_BW_40, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_DFS | BCMWIFI_RCLASS_FLAGS_EIRP | BCMWIFI_RCLASS_FLAGS_PRIMARY_UPPER),
	ARRAYSIZE(chan_set_5g_40u_3), chan_set_5g_40u_3},
	{124, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY,  BCMWIFI_BW_20, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_NOMADIC | BCMWIFI_RCLASS_FLAGS_EIRP),
	ARRAYSIZE(chan_set_5g_20_5), chan_set_5g_20_5},
	{125, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY,  BCMWIFI_BW_20, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_LIC_EXMPT | BCMWIFI_RCLASS_FLAGS_EIRP),
	ARRAYSIZE(chan_set_5g_20_6), chan_set_5g_20_6},
	{126, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY, BCMWIFI_BW_40, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_PRIMARY_LOWER | BCMWIFI_RCLASS_FLAGS_EIRP),
	ARRAYSIZE(chan_set_5g_40l_4), chan_set_5g_40l_4},
	{127, BCMWIFI_RCLASS_CHAN_TYPE_CHAN_PRIMARY, BCMWIFI_BW_40, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_PRIMARY_UPPER | BCMWIFI_RCLASS_FLAGS_EIRP),
	ARRAYSIZE(chan_set_5g_40u_4), chan_set_5g_40u_4},
	{128, BCMWIFI_RCLASS_CHAN_TYPE_CNTR_FREQ,  BCMWIFI_BW_80, BCMWIFI_BAND_5G,
	BCMWIFI_RCLASS_FLAGS_EIRP, ARRAYSIZE(chan_set_5g_80_1), chan_set_5g_80_1},
	{129, BCMWIFI_RCLASS_CHAN_TYPE_CNTR_FREQ,  BCMWIFI_BW_160, BCMWIFI_BAND_5G,
	BCMWIFI_RCLASS_FLAGS_EIRP, ARRAYSIZE(chan_set_5g_160_1), chan_set_5g_160_1},
	{130, BCMWIFI_RCLASS_CHAN_TYPE_CNTR_FREQ,  BCMWIFI_BW_80, BCMWIFI_BAND_5G,
	(BCMWIFI_RCLASS_FLAGS_EIRP | BCMWIFI_RCLASS_FLAGS_80PLUS),
	ARRAYSIZE(chan_set_5g_80_2), chan_set_5g_80_2},
};

static int
bcmwifi_rclass_get_rclass_table(bcmwifi_rclass_type_t type,
	const bcmwifi_rclass_info_t **rcinfo, uint8 *rct_len)
{
	int ret = BCME_OK;

	switch (type) {
	case BCMWIFI_RCLASS_TYPE_GBL:
		*rcinfo = rcinfo_global;
		*rct_len = (uint8)ARRAYSIZE(rcinfo_global);
		break;
	case BCMWIFI_RCLASS_TYPE_US:
	case BCMWIFI_RCLASS_TYPE_EU:
	case BCMWIFI_RCLASS_TYPE_JP:
	case BCMWIFI_RCLASS_TYPE_CH:
		/* Not supported */
		ret = BCME_UNSUPPORTED;
		break;
	default:
		/* Invalid argument */
		ret = BCME_BADARG;
	}

	return ret;
}

static bcmwifi_rclass_bw_t
cspec_bw(bcmwifi_rclass_bw_t bw)
{
	bcmwifi_rclass_bw_t cs_bw = 0;
	switch (bw) {
	case BCMWIFI_BW_20:
		cs_bw = WL_CHANSPEC_BW_20;
		break;
	case BCMWIFI_BW_40:
		cs_bw = WL_CHANSPEC_BW_40;
		break;
	case BCMWIFI_BW_80:
		cs_bw = WL_CHANSPEC_BW_80;
		break;
	case BCMWIFI_BW_160:
		cs_bw = WL_CHANSPEC_BW_160;
		break;
	default:
		/* should never happen */
		break;
	}

	return cs_bw;
}

static bool
lookup_chan_in_chan_set(const bcmwifi_rclass_info_t *entry,
	bcmwifi_rclass_bw_t bw, uint8 chan)
{
	bool ret = FALSE;
	uint8 idx = 0;
	bcmwifi_rclass_bw_t entry_cspec_bw = cspec_bw(entry->bw);

	for (idx = 0; idx < entry->chan_set_len; idx++) {
		if (entry->chan_set[idx] == chan && entry_cspec_bw == bw) {
			ret = TRUE;
			goto done;
		}
	}
done:
	return ret;
}

/*
 * Return the operating class for a given chanspec
 * Input: chanspec
 *        type (op class type is US, EU, JP, GBL, or CH). Currently only GBL
 *        is supported.
 * Output: rclass (op class)
 * On success return status BCME_OK.
 * On error return status BCME_UNSUPPORTED, BCME_BADARG.
 */
int
bcmwifi_rclass_get_opclass(bcmwifi_rclass_type_t type, chanspec_t chanspec,
	bcmwifi_rclass_opclass_t *rclass)
{
	int ret = BCME_ERROR;
	const bcmwifi_rclass_info_t *rcinfo = NULL;
	bcmwifi_rclass_bw_t bw;
	uint8 rct_len = 0;
	uint8 i;
	uint8 chan;

	if ((ret = bcmwifi_rclass_get_rclass_table(type, &rcinfo, &rct_len)) != BCME_OK) {
		goto done;
	}

	bw = CHSPEC_BW(chanspec);

	if (CHSPEC_IS80(chanspec) || CHSPEC_IS160(chanspec)) {
		chan = CHSPEC_CHANNEL(chanspec);
	} else if ((bw == WL_CHANSPEC_BW_40) || (bw == WL_CHANSPEC_BW_20)) {
		chan = wf_chspec_ctlchan(chanspec);
	} else {
		goto done;
	}

	for (i = 0; rcinfo != NULL && i < rct_len; i++) {
		const bcmwifi_rclass_info_t *entry = &rcinfo[i];
		if (lookup_chan_in_chan_set(entry, bw, chan)) {
			*rclass = entry->rclass;
			ret = BCME_OK;
			goto done;
		}
	}
done:
	return ret;
}

/*
 * Return op class info (starting freq, bandwidth, sideband, channel set and behavior)
 * for given op class and type.
 * Input(s): type (op class type - US, EU, JP, GBL, or CH)
 *           rclass (op class)
 * Output: rcinfo (op class info entry)
 * Return status BCME_OK when no error.
 * On error, BCME_ERROR, BCME_UNSUPPORTED, or BCME_BADARG.
 */
int
bcmwifi_rclass_get_rclass_info(bcmwifi_rclass_type_t type, bcmwifi_rclass_opclass_t rclass,
	const bcmwifi_rclass_info_t **rcinfo)
{
	int ret = BCME_ERROR;
	const bcmwifi_rclass_info_t *rct = NULL;
	uint8 rct_len = 0;
	uint8 idx = 0;

	if ((ret = bcmwifi_rclass_get_rclass_table(type, &rct, &rct_len)) != BCME_OK) {
		goto done;
	}

	for (idx = 0; idx < rct_len; idx++) {
		const bcmwifi_rclass_info_t *entry = &rct[idx];
		if (entry->rclass == rclass) {
			*rcinfo = entry;
			ret = BCME_OK;
			goto done;
		}
	}
	*rcinfo = NULL;
done:
	return ret;
}

/*
 * Make chanspec from channel index
 */
static int
bcmwifi_rclass_get_chanspec_from_chan_idx(const bcmwifi_rclass_info_t *entry, uint8 chn_idx,
	chanspec_t *cspec)
{
	int ret = BCME_OK;
	uint16 band = WL_CHANSPEC_BAND_2G;
	uint16 sb = WL_CHANSPEC_CTL_SB_NONE;
	uint8 chan = 0;

	if (chn_idx > entry->chan_set_len) {
		ret = BCME_BADARG;
		goto done;
	}

	chan = entry->chan_set[chn_idx];
	if (entry->band == BCMWIFI_BAND_5G) {
		band = WL_CHANSPEC_BAND_5G;
	}

	if (entry->bw > BCMWIFI_BW_20) {
		/* Upper and lower flags are only set for 40MHz */
		if (entry->flags & BCMWIFI_RCLASS_FLAGS_PRIMARY_LOWER) {
			sb = WL_CHANSPEC_CTL_SB_L;
			chan += CH_10MHZ_APART;
		} else if (entry->flags & BCMWIFI_RCLASS_FLAGS_PRIMARY_UPPER) {
			sb = WL_CHANSPEC_CTL_SB_U;
			chan -= CH_10MHZ_APART;
		}
	}

	*cspec = (band | sb | cspec_bw(entry->bw) | chan);

done:
	return ret;
}

/*
 * Return chanspec given op class type, op class and channel index
 * Input: type (op class type US, EU, JP, GBL, CH)
 *        rclass (op class)
 *        chn_idx (index in channel list)
 * Output: chanspec
 * Return BCME_OK on success and BCME_ERROR on error
 */
int
bcmwifi_rclass_get_chanspec(bcmwifi_rclass_type_t type, bcmwifi_rclass_opclass_t rclass,
	uint8 chn_idx, chanspec_t *cspec)
{
	int ret = BCME_ERROR;
	const bcmwifi_rclass_info_t *rct = NULL;
	uint8 rct_len = 0;
	uint8 idx = 0;

	if ((ret = bcmwifi_rclass_get_rclass_table(type, &rct, &rct_len)) != BCME_OK) {
		goto done;
	}

	for (idx = 0; idx < rct_len; idx++) {
		const bcmwifi_rclass_info_t *entry = &rct[idx];
		if (entry->rclass != rclass) {
			continue;
		} else {
			ret = bcmwifi_rclass_get_chanspec_from_chan_idx(entry,
					chn_idx, cspec);
			goto done;
		}
	}
done:
	return ret;
}

/*
 * Return supported op class encoded as a bitvector.
 * Input: type (op class type US, EU, JP, GBL, or CH)
 *
 * Output: rclass_bvec
 * Return status BCME_OK when no error.
 * On error, return status BCME_ERROR, BCME_BADARG, or BCME_UNSUPPORTED
 */
int
bcmwifi_rclass_get_supported_opclasses(bcmwifi_rclass_type_t type,
	const bcmwifi_rclass_bvec_t **rclass_bvec)
{
	int ret = BCME_ERROR;
	const bcmwifi_rclass_info_t *rct = NULL;
	bcmwifi_rclass_bvec_t *bvec = &g_bvec;
	uint8 rct_len = 0;
	uint8 *vec;
	uint8 idx = 0;

	if ((ret = bcmwifi_rclass_get_rclass_table(type, &rct, &rct_len)) != BCME_OK) {
		goto done;
	}

	for (idx = 0; idx < rct_len; idx++) {
		const bcmwifi_rclass_info_t *entry = &rct[idx];
		vec = &bvec->bvec[entry->rclass / 8];
		*vec |= (1 << (entry->rclass % 8));
	}
	*rclass_bvec = (const bcmwifi_rclass_bvec_t *)&g_bvec;
done:
	return ret;
}
