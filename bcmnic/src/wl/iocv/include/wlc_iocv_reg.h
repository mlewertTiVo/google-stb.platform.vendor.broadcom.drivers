/*
 * IOCV module interface - ioctl/iovar table registration.
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
 * $Id: wlc_iocv_reg.h 660317 2016-09-20 05:54:54Z $
 */

#ifndef _wlc_iocv_reg_h_
#define _wlc_iocv_reg_h_

#include <typedefs.h>
#include <bcmutils.h>

#include <wlc_iocv_desc.h>
#include <wlc_iocv_types.h>

#ifdef WLC_PATCH_IOCTL
/* If we are building for NON-FULL ROM software, by default IOVAR patch
 * handling is disabled. These macros are overriden by the generated
 * IOVAR patch handler file if patching is enabled.
 * But if we are building FULL ROM software,
 * then these macros will be undef'd and redefined in 'wlc_patch.h' with global variables,
 * so that we can avoid abandoning of attach functions if patch is generated.
 */
#define IOV_PATCH_TBL NULL
#define IOV_PATCH_FN NULL
#define IOC_PATCH_FN NULL
#endif

/* ==== For WLC modules use ==== */

/* register wlc iovar table & dispatcher */
int wlc_iocv_high_register_iovt(wlc_iocv_info_t *ii,
	const bcm_iovar_t *iovt, wlc_iov_disp_fn_t disp_fn,
#ifdef WLC_PATCH_IOCTL
	const bcm_iovar_t *patch_iovt, wlc_iov_disp_fn_t patch_disp_fn,
#endif
	void *ctx);
/* register wlc ioctl table & dispatcher */
int wlc_iocv_high_register_ioct(wlc_iocv_info_t *ii,
	const wlc_ioctl_cmd_t *ioct, uint num_cmds, wlc_ioc_disp_fn_t disp_fn,
#ifdef WLC_PATCH_IOCTL
    wlc_ioc_disp_fn_t ioc_patch_fn,
#endif
	void *ctx);

/* ioctl state validate function */
typedef int (*wlc_ioc_vld_fn_t)(void *ctx, uint32 cid,
	void *a, uint alen);


/* ==== For PHY/BMAC modules use ==== */

/* iovar table descriptor */
typedef struct {
	/* table pointer */
	const bcm_iovar_t *iovt;
	const bcm_iovar_t *patch_iovt;
	/* dispatch callback */
	wlc_iov_disp_fn_t disp_fn;
	wlc_iov_disp_fn_t patch_disp_fn;
	void *ctx;
} wlc_iovt_desc_t;

/* ioctl table descriptor */
typedef struct {
	/* table pointer */
	const wlc_ioctl_cmd_t *ioct;
	uint num_cmds;
	wlc_ioc_vld_fn_t st_vld_fn;
	/* dispatch callback */
	wlc_ioc_disp_fn_t disp_fn;
	wlc_ioc_disp_fn_t ioc_patch_fn;
	void *ctx;
} wlc_ioct_desc_t;

/* init iovar table desc */
#define _wlc_iocv_init_iovd_(\
	_iovt_, _cmdfn_, _resfn_, _dispfn_, _ptchdispfn_, _ptchtbl_, _ctx_, _iovd_) do { \
		(_iovd_)->iovt = _iovt_;				\
		(_iovd_)->patch_iovt = _ptchtbl_;			\
		(_iovd_)->disp_fn = _dispfn_;				\
		(_iovd_)->patch_disp_fn = _ptchdispfn_;			\
		(_iovd_)->ctx = _ctx_;					\
	} while (FALSE)

/* init iovar table descriptor */
#define wlc_iocv_init_iovd(iovt, cmdfn, resfn, dispfn, ptchdispfn, ptchtbl, ctx, iovd) \
	_wlc_iocv_init_iovd_(iovt, cmdfn, resfn, dispfn, ptchdispfn, ptchtbl, ctx, iovd)
/* register bmac/phy iovar table & callbacks */
int wlc_iocv_register_iovt(wlc_iocv_info_t *ii, wlc_iovt_desc_t *iovd);

/* init ioctl table desc */
#define _wlc_iocv_init_iocd_(_ioct_, _sz_, _vldfn_, _cmdfn_, _resfn_, _dispfn_, _ctx_, _iocd_) do {\
		(_iocd_)->ioct = _ioct_;				\
		(_iocd_)->num_cmds = _sz_;				\
		(_iocd_)->st_vld_fn = _vldfn_;				\
		(_iocd_)->disp_fn = _dispfn_;				\
		(_iocd_)->ctx = _ctx_;					\
	} while (FALSE)

/* init ioctl table descriptor */
#define wlc_iocv_init_iocd(ioct, sz, vldfn, cmdfn, resfn, dispfn, ctx, iocd) \
	_wlc_iocv_init_iocd_(ioct, sz, vldfn, cmdfn, resfn, dispfn, ctx, iocd)
/* register bmac/phy ioctl table & callbacks */
int wlc_iocv_register_ioct(wlc_iocv_info_t *ii, wlc_ioct_desc_t *iocd);

#endif /* wlc_iocv_reg_h_ */
