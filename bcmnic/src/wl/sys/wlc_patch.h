/*
 * PATCH routines common hdr file
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_patch.h 623137 2016-03-05 00:20:57Z $
 */

#ifndef _wlc_patch_h_
#define _wlc_patch_h_

#include <wlc_iocv_patch.h>

/* "Patch preambles" are assembly instructions corresponding to the first couple instructions
 * for each ROM function. These instructions are executed (in RAM) by manual patch functions prior
 * to branching to an offset within the patched ROM function. This avoids recursively hitting the
 * TCAM entry located at the beginning of the ROM function (in the absense of ROM function nop
 * preambles).
 */
#if defined(BCMROM_PATCH_PREAMBLE)
	#define CALLROM_ENTRY(a) a##__bcmromfn_preamble
#else
	#define CALLROM_ENTRY(a) a##__bcmromfn
#endif

#endif /* _wlc_patch_h_ */
