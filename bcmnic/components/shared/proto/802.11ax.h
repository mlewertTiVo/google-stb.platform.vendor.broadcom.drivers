/*
 * Basic types and constants relating to 802.11ax/HE STA
 * This is a portion of 802.11ax definition. The rest are in 802.11.h.
 *
 * Copyright (C) 2016, Broadcom. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: 802.11ax.h 643473 2016-06-14 22:31:56Z $
 */

#ifndef _802_11ax_h_
#define _802_11ax_h_

#include <typedefs.h>

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>

/**
 * HE Capabilites element (sec 8.4.2.213)
 */

/* HE Capabilities Information field */
typedef uint16 he_cap_info_t;

#define HE_INFO_PPE_THRESH_PRESENT	0x0001	/* PPE Thresholds Present */
#define HE_INFO_TWT_REQ_SUPPORT		0x0002	/* TWT Requestor Support */
#define HE_INFO_TWT_RESP_SUPPORT	0x0004	/* TWT Responder Support */
#define HE_INFO_FRAG_SUPPORT_MASK	0x0018	/* Fragmentation Support */
#define HE_INFO_FRAG_SUPPORT_SHIFT	3
#define HE_INFO_AMPDU_MULTI_TID_CAP	0x0020	/* A-MPDU with Multiple TIDs Capable */
#define HE_INFO_DL_OFDMA_CAP		0x0040	/* DL OFDMA Capable */
#define HE_INFO_UL_OFDMA_CAP		0x0080	/* UL OFDMA Capable */
#define HE_INFO_DL_MU_MIMO_CAP		0x0100	/* DL MU-MIMO Capable */
#define HE_INFO_UL_MU_MIMO_CAP		0x0200	/* UL MU-MIMO Capable */
#define HE_INFO_DL_MU_MIMO_STS_MASK	0x1c00	/* DL MU-MIMO STS Capability */
#define HE_INFO_DL_MU_MIMO_STS_SHIFT	10
#define HE_INFO_UL_MU_MIMO_STS_MASK	0xe000	/* UL MU-MIMO STS Capability */
#define HE_INFO_UL_MU_MIMO_STS_SHIFT	13

/* Fragmentation Support field (table 8-554ae) */
#define HE_FRAG_SUPPORT_NO_FRAG		0	/* no frag */
#define HE_FRAG_SUPPORT_VHT_SINGLE_MPDU	1	/* VHT single MPDU only */
#define HE_FRAG_SUPPORT_ONE_PER_MPDU	2	/* 1 frag per MPDU in A-MPDU */
#define HE_FRAG_SUPPORT_MULTI_PER_MPDU	3	/* 2+ frag per MPDU in A-MPDU */

#define HE_CAP_MCS_MAP_NSS_MAX	8
#define HE_MAX_RU_COUNT	4 /* Max number of RU allocation possible */

#define HE_NSSM1_LEN 3 /* length of NSSM1 field in bits */
#define HE_RU_COUNT_LEN 2 /* length of RU count field in bits */

/* HE Capabilities element */
BWL_PRE_PACKED_STRUCT struct he_cap_ie {
	uint8 id;
	uint8 len;
	he_cap_info_t info;
} BWL_POST_PACKED_STRUCT;

typedef struct he_cap_ie he_cap_ie_t;

/* PPE Threshold field (figure 8-554ac) */
#define HE_PPE_THRESH_NSS_M1_MASK	0x0007
#define HE_PPE_THRESH_RU_CNT_MASK	0x0018
#define HE_PPE_THRESH_RU_CNT_SHIFT	3

/* PPE Threshold Info field (figure 8-554ae) */
/* ruc: RU Count; NSSnM1: NSSn - 1; RUmM1: RUm - 1 */
#define HE_PPET16_BIT_OFFSET(ruc, NSSnM1, RUmM1) \
	(5 + ((NSSnM1) * (ruc) + (RUmM1)) * 6)		/* bit offset in PPE Threshold field */
#define HE_PPET16_BYTE_OFFSET(ruc, NSSnM1, RUmM1) \
	(HE_PPET16_BIT_OFFSET(ruc, NSSnM1, RUmM1) >> 3)	/* byte offset in PPE Threshold field,
							 * 3 bits may spin across 2 bytes.
							 */
#define HE_PPET16_MASK(ruc, NSSnM1, RUmM1) \
	(7 << HE_PPET16_SHIFT(ruc, NSSnM1, RUmM1))	/* bitmask in above 2 bytes */
#define HE_PPET16_SHIFT(ruc, NSSnM1, RUmM1) \
	(HE_PPET16_BIT_OFFSET(ruc, NSSnM1, RUmM1) - \
	 (HE_PPET16_BYTE_OFFSET(ruc, NSSnM1, RUmM1) << 3))	/* bit shift in above 2 bytes */
#define HE_PPET8_BIT_OFFSET(ruc, NSSnM1, RUmM1) \
	(5 + ((NSSnM1) * (ruc) + (RUmM1)) * 6 + 3)	/* bit offset in PPE Threshold field */
#define HE_PPET8_BYTE_OFFSET(ruc, NSSnM1, RUmM1) \
	(HE_PPET8_BIT_OFFSET(ruc, NSSnM1, RUmM1) >> 3)	/* byte offset in PPE Threshold field,
							 * 6 bits may spin across 2 bytes.
							 */
#define HE_PPET8_MASK(ruc, NSSnM1, RUmM1) \
	(7 << HE_PPET8_SHIFT(ruc, NSSnM1, RUmM1))	/* bitmask in above 2 bytes */
#define HE_PPET8_SHIFT(ruc, NSSnM1, RUmM1) \
	(HE_PPET8_BIT_OFFSET(ruc, NSSnM1, RUmM1) - \
	 (HE_PPET8_BYTE_OFFSET(ruc, NSSnM1, RUmM1) << 3))	/* bit shift in above 2 bytes */

/* Total PPE Threshold field byte length */
#define HE_PPE_THRESH_LEN(nss, ruc)	(5 + (((nss) * (ruc) * 6) >> 3))

/* RU Allocation Index encoding (table 8-554ag) */
#define HE_RU_ALLOC_IDX_2x996	0	/* RU alloc: 2x996 tones - 80p80/160Mhz */
#define HE_RU_ALLOC_IDX_996	1	/* RU alloc: 996 tones - 80Mhz */
#define HE_RU_ALLOC_IDX_484	2	/* RU alloc: 484 tones - 40Mhz */
#define HE_RU_ALLOC_IDX_RSVD	3	/* Reserved */

/* Constellation Index encoding (table 8-554ah) */
#define HE_CONST_IDX_BPSK	0
#define HE_CONST_IDX_QPSK	1
#define HE_CONST_IDX_16QAM	2
#define HE_CONST_IDX_64QAM	3
#define HE_CONST_IDX_256QAM	4
#define HE_CONST_IDX_1024QAM	5
#define HE_CONST_IDX_RSVD	6
#define HE_CONST_IDX_NONE	7

/**
 * HE Operation IE (sec 8.4.2.214)
 */

/* HE Operation Parameters field */
typedef uint16 he_op_parms_t;

#define HE_OP_BSS_COLOR_MASK	0x00ff	/* TODO: BSS Color Mask - TBD in spec */

/* HE Operation elemebt */
BWL_PRE_PACKED_STRUCT struct he_op_ie {
	uint8 id;
	uint8 len;
	he_op_parms_t parms;
} BWL_POST_PACKED_STRUCT;

typedef struct he_op_ie he_op_ie_t;

/* This marks the end of a packed structure section. */
#include <packed_section_end.h>

/* HE Action Frame */
#define HE_AF_CAT_OFF	0
#define HE_AF_ACT_OFF	1

/* TWT Setup */
#define HE_AF_TWT_SETUP_TOKEN_OFF	2
#define HE_AF_TWT_SETUP_TWT_IE_OFF	3

/* TWT Teardown */
#define HE_AF_TWT_TEARDOWN_FLOW_OFF	2

/* TWT Information */
#define HE_AF_TWT_INFO_OFF	2

/* HE Action ID */
#define HE_ACTION_TWT_SETUP	1
#define HE_ACTION_TWT_TEARDOWN	2
#define HE_ACTION_TWT_INFO	3

#endif /* _802_11ax_h_ */
