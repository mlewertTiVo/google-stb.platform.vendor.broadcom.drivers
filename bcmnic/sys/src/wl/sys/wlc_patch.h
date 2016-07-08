/*
 * PATCH routines common hdr file
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_patch.h 590510 2015-10-05 06:54:19Z $
 */

#ifndef _wlc_patch_h_
#define _wlc_patch_h_

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmutils.h>
#include <wlc_types.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_pub.h>
#include <wlc_key.h>
#include <wlc.h>
#include <wlc_bsscfg.h>

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

#ifdef WLC_PATCH_IOCTL

extern int wlc_ioctl_patchmod(void *ctx, int cmd, void *arg, int len, struct wlc_if *wlcif);

/* Defaultly all patches will be defined to 'NULL', But this will be overridden by
 * actual patch generated during ROM OFFLOAD build from respective modules patch files.
 */
#define ROM_AUTO_IOCTL_PATCH_DOIOVAR NULL
#define ROM_AUTO_IOCTL_PATCH_IOVARS NULL

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file. It must be
 * included after the prototypes above. The name of the included source file (WLC_PATCH_IOCTL_FILE)
 * is defined by the build environment.
 */
#if defined(WLC_PATCH_IOCTL_FILE)
	#include WLC_PATCH_IOCTL_FILE
#endif

#define PATCH_IOVAR_FUNC_EXP(X)     X##_patch_func
#define PATCH_IOVAR_FUNC(X)         PATCH_IOVAR_FUNC_EXP(X)

#define PATCH_IOVAR_TABLE_EXP(X)    X##_patch_table
#define PATCH_IOVAR_TABLE(X)        PATCH_IOVAR_TABLE_EXP(X)

/* For FULL ROM SW, redefine iovar patch macros with global patch pointers,
 * initialized with new patch table/func.
 * For NON-FULL ROM SW, redefine iovar patch macros with new patch table/func.
 */
#undef MODULE_IOVAR_PATCH_FUNC
#undef MODULE_IOVAR_PATCH_TABLE
#if defined(ROM_AUTO_IOCTL_PATCH_GLOBAL_PTRS)
	#define MODULE_IOVAR_PATCH_FUNC		PATCH_IOVAR_FUNC(__FILENAME_NOEXTN__)
	#define MODULE_IOVAR_PATCH_TABLE	PATCH_IOVAR_TABLE(__FILENAME_NOEXTN__)

	void* MODULE_IOVAR_PATCH_FUNC  = (void*)ROM_AUTO_IOCTL_PATCH_DOIOVAR;
	bcm_iovar_t* MODULE_IOVAR_PATCH_TABLE = (bcm_iovar_t*)ROM_AUTO_IOCTL_PATCH_IOVARS;
#else /* !ROM_AUTO_IOCTL_PATCH_GLOBAL_PTRS */
	#define MODULE_IOVAR_PATCH_FUNC		ROM_AUTO_IOCTL_PATCH_DOIOVAR
	#define MODULE_IOVAR_PATCH_TABLE	ROM_AUTO_IOCTL_PATCH_IOVARS
#endif /* ROM_AUTO_IOCTL_PATCH_GLOBAL_PTRS */

#endif /* WLC_PATCH_IOCTL */

#endif /* _wlc_patch_h_ */
