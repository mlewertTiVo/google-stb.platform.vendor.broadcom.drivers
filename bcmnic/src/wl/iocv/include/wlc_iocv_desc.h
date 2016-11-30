/*
 * IOCV module interface - ioctl/iovar entry descriptor.
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
 * $Id: wlc_iocv_desc.h 624223 2016-03-11 00:04:24Z $
 */

#ifndef _wlc_iocv_desc_h_
#define _wlc_iocv_desc_h_

/* IOVar flags for common error checks */
#define IOVF_BSSCFG_STA_ONLY	(1<<0)	/**< flag for BSSCFG_STA() only iovars */
#define IOVF_BSSCFG_AP_ONLY	(1<<1)	/**< flag for BSSCFG_AP() only iovars */
#define IOVF_BSS_SET_DOWN (1<<2)	/**< set requires BSS to be down */

#define IOVF_MFG	(1<<3)  /* flag for mfgtest iovars */
#define IOVF_WHL	(1<<4)	/**< value must be whole (0-max) */
#define IOVF_NTRL	(1<<5)	/**< value must be natural (1-max) */

#define IOVF_SET_UP	(1<<6)	/**< set requires driver be up */
#define IOVF_SET_DOWN	(1<<7)	/**< set requires driver be down */
#define IOVF_SET_CLK	(1<<8)	/**< set requires core clock */
#define IOVF_SET_BAND	(1<<9)	/**< set requires fixed band */

#define IOVF_GET_UP	(1<<10)	/**< get requires driver be up */
#define IOVF_GET_DOWN	(1<<11)	/**< get requires driver be down */
#define IOVF_GET_CLK	(1<<12)	/**< get requires core clock */
#define IOVF_GET_BAND	(1<<13)	/**< get requires fixed band */

#define IOVF_OPEN_ALLOW	(1<<14)	/**< set allowed iovar for opensrc */
#define IOVF_RSDB_SET 	(1<<15)  /* set iovar across all WLCs */

#ifdef WL_DUALMAC_RSDB
#define IOVF_RSDB_DEFAULT 0
#else
#define IOVF_RSDB_DEFAULT IOVF_RSDB_SET
#endif

/* IOVAR flag used by ROM patch tables. Indicates that the IOVAR has been removed from the latest
 * code. Or may be unsupported based upon #ifdefs. (Note that all of the IOVF_xxx bits have been
 * used. Since some of the bits are mutually exclusive, it is impossible for all bits to be set in a
 * real IOVAR table. Therefore IOVF_REMOVED can be set to 0xffff, instead of increasing the width
 * of the flags bit-field).
 */
#define IOVF_REMOVED	(0xffff)

/* IOVar flags (flags2) */
#define IOVF2_RSDB_CORE_OVERRIDE (1<<0)  /* To specify "-w" option usage for any iovar */


/* IOCTL flags for common error checks */
#define WLC_IOCF_BSSCFG_STA_ONLY (1<<0)	/**< flag for BSSCFG_STA() only ioctls */
#define WLC_IOCF_BSSCFG_AP_ONLY	(1<<1)	/**< flag for BSSCFG_AP() only ioctls */

#define WLC_IOCF_MFG		(1<<2)  /* flag for mfgtest ioctls */

#define WLC_IOCF_DRIVER_UP	(1<<3)	/**< requires driver be up */
#define WLC_IOCF_DRIVER_DOWN	(1<<4)	/**< requires driver be down */
#define WLC_IOCF_CORE_CLK	(1<<5)	/**< requires core clock */
#define WLC_IOCF_FIXED_BAND	(1<<6)	/**< requires fixed band */
#define WLC_IOCF_OPEN_ALLOW	(1<<7)	/**< allowed ioctl for opensrc */

#define WLC_IOCF_REG_CHECK	(1<<8)	/**< band specific h/w register access check */
#define WLC_IOCF_REG_CHECK_AUTO	(1<<9)	/**< auto band h/w register access check */
#define WLC_IOCF_BAND_CHECK_AUTO (1<<10) /**< check band specifications */

#endif /* _wlc_iocv_desc_h_ */
