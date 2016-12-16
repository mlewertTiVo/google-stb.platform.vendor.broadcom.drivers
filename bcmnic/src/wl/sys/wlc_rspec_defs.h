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
 * $Id: wlc_rspec_defs.h 661129 2016-09-23 09:41:42Z $
 */


#ifndef _wlc_rspec_defs_h_
#define _wlc_rspec_defs_h_

#include <typedefs.h>

/**
 * ===================================================================================
 * rate spec : holds rate and mode specific information required to generate a tx frame.
 * Legacy CCK and OFDM information is held in the same manner as was done in the past.
 * (in the lower byte) the upper 3 bytes primarily hold MIMO specific information
 * ===================================================================================
 */
typedef uint32	ratespec_t;

/* Rate spec. definitions */
#define RSPEC_RATE_MASK         0x000000FF      /**< Legacy rate or MIMO MCS + NSS */
#define RSPEC_TXEXP_MASK        0x00000300      /**< Tx chain expansion beyond Nsts */
#define RSPEC_TXEXP_SHIFT       8
#define RSPEC_BW_MASK           0x00070000      /**< Band width */
#define RSPEC_BW_SHIFT          16
#define RSPEC_HE_DCM_MASK	0x00080000      /**< DCM: for HE */
#define RSPEC_HE_DCM_SHIFT	19		/**< DCM shift: for HE */
#define RSPEC_STBC              0x00100000      /**< STBC expansion, Nsts = 2 * Nss */
#define RSPEC_TXBF              0x00200000      /**< TXBF */
#define RSPEC_LDPC_CODING       0x00400000      /**< adv coding in use */
#define RSPEC_SHORT_GI          0x00800000      /**< short GI in use : see RSPEC_HE_GI_EXTN also */
#define RSPEC_SHORT_GI_SHIFT	23		/**< short GI shift */
#define RSPEC_SHORT_PREAMBLE    0x00800000      /**< DSSS short preable - Encoding 0 */
#define RSPEC_ENCODING_MASK     0x03000000      /**< Encoding of RSPEC_RATE field */
#define RSPEC_HE_GI_EXTN	0x08000000      /**< HE GI extension: inerpretation in HE is
						 * RSPEC_HE_GI_EXTN	RSPEC_SHORT_GI	GI value
						 * 0			0		0.8uSec
						 * 0			1		0.4uSec
						 * 1			0		1.6uSec
						 * 1			1		3.2uSec
						 */
#define RSPEC_HE_GI_EXTN_SHIFT	27		/**< HE GI extension shift */
#define RSPEC_OVERRIDE_RATE     0x40000000      /**< override rate only */
#define RSPEC_OVERRIDE_MODE     0x80000000      /**< override both rate & mode */

/* GI for HE */
#define RSPEC_HE_GI(rspec)	((((rspec) & RSPEC_SHORT_GI) >> RSPEC_SHORT_GI_SHIFT) | \
					(((rspec) & RSPEC_HE_GI_EXTN) >> \
					(RSPEC_HE_GI_EXTN_SHIFT - 1)))

/* ======== RSPEC_GI fields for HE ======== */
#define RSPEC_HE_GI_0_8us	(0x0)
#define RSPEC_HE_GI_1_6us	(0x2)
#define RSPEC_HE_GI_3_2us	(0x3)

/* Encoding 0 - legacy rate */
/* DSSS, CCK, and OFDM rates in [500kbps] units */
#define RSPEC_LEGACY_RATE_MASK  0x0000007F
#define WLC_RATE_1M	2
#define WLC_RATE_2M	4
#define WLC_RATE_5M5	11
#define WLC_RATE_11M	22
#define WLC_RATE_6M	12
#define WLC_RATE_9M	18
#define WLC_RATE_12M	24
#define WLC_RATE_18M	36
#define WLC_RATE_24M	48
#define WLC_RATE_36M	72
#define WLC_RATE_48M	96
#define WLC_RATE_54M	108

/* Encoding 1 - HT MCS + NSS */
#define RSPEC_HT_MCS_MASK       0x00000007      /**< HT MCS value mask in rate field */
#define RSPEC_HT_PROP_MCS_MASK  0x0000007F      /**< more than 4 bits are required for prop MCS */
#define RSPEC_HT_NSS_MASK       0x00000078      /**< HT Nss value mask in rate field */
#define RSPEC_HT_NSS_SHIFT      3               /**< HT Nss value shift in rate field */

/* Encoding 2 - VHT MCS + NSS */
#define RSPEC_VHT_MCS_MASK      0x0000000F      /**< VHT MCS value mask in rate field */
#define RSPEC_VHT_NSS_MASK      0x000000F0      /**< VHT Nss value mask in rate field */
#define RSPEC_VHT_NSS_SHIFT     4               /**< VHT Nss value shift in rate field */

/* Encoding 3 - HE MCS + NSS */
#define RSPEC_HE_MCS_MASK      0x0000000F      /**< HE MCS value mask in rate field */
#define RSPEC_HE_NSS_MASK      0x000000F0      /**< HE Nss value mask in rate field */
#define RSPEC_HE_NSS_SHIFT     4               /**< HE Nss value shift in rate field */

/* ======== RSPEC_BW field ======== */

#define RSPEC_BW_UNSPECIFIED    0x00000000
#define RSPEC_BW_20MHZ          0x00010000
#define RSPEC_BW_40MHZ          0x00020000
#define RSPEC_BW_80MHZ          0x00030000
#define RSPEC_BW_160MHZ         0x00040000

/* ======== RSPEC_ENCODING field ======== */

#define RSPEC_ENCODE_RATE	0x00000000	/**< Legacy rate is stored in RSPEC_RATE */
#define RSPEC_ENCODE_HT		0x01000000	/**< HT MCS is stored in RSPEC_RATE */
#define RSPEC_ENCODE_VHT	0x02000000	/**< VHT MCS and NSS are stored in RSPEC_RATE */
#define RSPEC_ENCODE_HE		0x03000000	/**< HE MCS and NSS are stored in RSPEC_RATE */

/**
 * ===============================
 * Handy macros to parse rate spec
 * ===============================
 */
#define RSPEC_BW(rspec)         ((rspec) & RSPEC_BW_MASK)
#define RSPEC_IS20MHZ(rspec)	(RSPEC_BW(rspec) == RSPEC_BW_20MHZ)
#define RSPEC_IS40MHZ(rspec)	(RSPEC_BW(rspec) == RSPEC_BW_40MHZ)
#define RSPEC_IS80MHZ(rspec)    (RSPEC_BW(rspec) == RSPEC_BW_80MHZ)
#define RSPEC_IS160MHZ(rspec)   (RSPEC_BW(rspec) == RSPEC_BW_160MHZ)

#define RSPEC_ISSGI(rspec)      (((rspec) & RSPEC_SHORT_GI) != 0)
#define RSPEC_ISLDPC(rspec)     (((rspec) & RSPEC_LDPC_CODING) != 0)
#define RSPEC_ISSTBC(rspec)     (((rspec) & RSPEC_STBC) != 0)
#define RSPEC_ISTXBF(rspec)     (((rspec) & RSPEC_TXBF) != 0)

#define RSPEC_TXEXP(rspec)      (((rspec) & RSPEC_TXEXP_MASK) >> RSPEC_TXEXP_SHIFT)

#ifdef WL11N
#define RSPEC_ISHT(rspec)	(((rspec) & RSPEC_ENCODING_MASK) == RSPEC_ENCODE_HT)
#define RSPEC_ISLEGACY(rspec)   (((rspec) & RSPEC_ENCODING_MASK) == RSPEC_ENCODE_RATE)
#else /* WL11N */
#define RSPEC_ISHT(rspec)	0
#define RSPEC_ISLEGACY(rspec)   1
#endif /* WL11N */
#ifdef WL11AC
#define RSPEC_ISVHT(rspec)	(((rspec) & RSPEC_ENCODING_MASK) == RSPEC_ENCODE_VHT)
#else /* WL11AC */
#define RSPEC_ISVHT(rspec)	0
#endif /* WL11AC */
#ifdef WL11AX
#define RSPEC_ISHE(rspec)	(((rspec) & RSPEC_ENCODING_MASK) == RSPEC_ENCODE_HE)
#else /* WL11AX */
#define RSPEC_ISHE(rspec)	0
#endif /* WL11AX */

/**
 * Macros to extract Frametype TBL index from RSPEC
 */
#define RSPEC_TO_FT_TBL_IDX_SHIFT	24
#define RSPEC_TO_FT_TBL_IDX_MASK	((RSPEC_ENCODING_MASK) >> (RSPEC_TO_FT_TBL_IDX_SHIFT))
#define RSPEC_TO_FT_TBL_IDX(r)		(((r) >> RSPEC_TO_FT_TBL_IDX_SHIFT) & \
					(RSPEC_TO_FT_TBL_IDX_MASK))

/**
 * ================================
 * Handy macros to create rate spec
 * ================================
 */
/* create ratespecs */
#define LEGACY_RSPEC(rate)	(RSPEC_ENCODE_RATE | RSPEC_BW_20MHZ | \
				 ((rate) & RSPEC_LEGACY_RATE_MASK))
#define HT_RSPEC(mcs)		(RSPEC_ENCODE_HT | ((mcs) & RSPEC_RATE_MASK))
#define VHT_RSPEC(mcs, nss)	(RSPEC_ENCODE_VHT | \
				 (((nss) << RSPEC_VHT_NSS_SHIFT) & RSPEC_VHT_NSS_MASK) | \
				 ((mcs) & RSPEC_VHT_MCS_MASK))

#define HE_RSPEC(mcs, nss)	(RSPEC_ENCODE_HE | \
					 (((nss) << RSPEC_HE_NSS_SHIFT) & RSPEC_HE_NSS_MASK) | \
					 ((mcs) & RSPEC_HE_MCS_MASK))

#endif /* _wlc_rspec_defs_h_ */
