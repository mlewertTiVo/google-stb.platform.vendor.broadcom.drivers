/*
 * PATCH routines common hdr file
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
 * $Id: wlc_iocv_patch.h 623137 2016-03-05 00:20:57Z $
 */

#ifndef _wlc_iocv_patch_h_
#define _wlc_iocv_patch_h_

#include <typedefs.h>
#include <bcmutils.h>

#include <wlc_iocv_types.h>

#ifdef WLC_PATCH_IOCTL

struct wlc_if;

int wlc_ioctl_patchmod(void *ctx, uint32 cmd, void *arg, uint len, struct wlc_if *wlcif);

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
#undef IOV_PATCH_TBL
#undef IOV_PATCH_FN
#if defined(ROM_AUTO_IOCTL_PATCH_GLOBAL_PTRS)
	#define IOV_PATCH_FN	PATCH_IOVAR_FUNC(__FILENAME_NOEXTN__)
	#define IOV_PATCH_TBL	PATCH_IOVAR_TABLE(__FILENAME_NOEXTN__)

	wlc_iov_disp_fn_t IOV_PATCH_FN = ROM_AUTO_IOCTL_PATCH_DOIOVAR;
	bcm_iovar_t *IOV_PATCH_TBL = (bcm_iovar_t *)ROM_AUTO_IOCTL_PATCH_IOVARS;
#else /* !ROM_AUTO_IOCTL_PATCH_GLOBAL_PTRS */
	#define IOV_PATCH_FN	ROM_AUTO_IOCTL_PATCH_DOIOVAR
	#define IOV_PATCH_TBL	ROM_AUTO_IOCTL_PATCH_IOVARS
#endif /* ROM_AUTO_IOCTL_PATCH_GLOBAL_PTRS */

#endif /* WLC_PATCH_IOCTL */

#endif /* _wlc_iocv_patch_h_ */
