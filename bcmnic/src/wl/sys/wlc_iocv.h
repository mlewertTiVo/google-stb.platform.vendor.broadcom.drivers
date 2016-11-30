/**
 * ioctl/iovar HIGH driver wrapper module interface
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
 * $Id: wlc_iocv.h 614277 2016-01-21 20:28:50Z $
 */
#ifndef _wlc_iocv_h_
#define _wlc_iocv_h_

#include <wlc_types.h>
#include <wlc_iocv_reg.h>
#include <wlc_iocv_high.h>

/* attach/detach as a wlc module */
wlc_iocv_info_t *wlc_iocv_attach(wlc_info_t *wlc);
void wlc_iocv_detach(wlc_iocv_info_t *iocvi);

/* register iovar/ioctl table & dispatcher */
#ifdef WLC_PATCH_IOCTL
#define wlc_iocv_add_iov_fn(iocvi, tbl, fn, ctx) \
	wlc_iocv_high_register_iovt(iocvi, tbl, fn, IOV_PATCH_TBL, IOV_PATCH_FN, ctx)
#else /* !WLC_PATCH_IOCTL */
#define wlc_iocv_add_iov_fn(iocvi, tbl, fn, ctx) \
	wlc_iocv_high_register_iovt(iocvi, tbl, fn, ctx)
#endif /* !WLC_PATCH_IOCTL */
#define wlc_iocv_add_ioc_fn(iocvi, tbl, tbl_sz, fn, ctx) \
	wlc_iocv_high_register_ioct(iocvi, tbl, tbl_sz, fn, ctx)

/* ioctl compatibility interface */
#define wlc_module_add_ioctl_fn(pub, hdl, fn, sz, tbl) \
	wlc_iocv_add_ioc_fn((pub)->wlc->iocvi, tbl, sz, fn, hdl)
#define wlc_module_remove_ioctl_fn(pub, hdl) BCME_OK

/* ioctl utilities */
int wlc_ioctl(wlc_info_t *wlc, int cmd, void *arg, int len, wlc_if_t *wlcif);
int wlc_set(wlc_info_t *wlc, int cmd, int arg);
int wlc_get(wlc_info_t *wlc, int cmd, int *arg);

int wlc_iocregchk(wlc_info_t *wlc, uint band);
int wlc_iocbandchk(wlc_info_t *wlc, int *arg, int len, uint *bands, bool clkchk);
int wlc_iocpichk(wlc_info_t *wlc, uint phytype);

/* iovar utilities */
int wlc_iovar_op(wlc_info_t *wlc, const char *name, void *params, int p_len, void *arg,
	int len, bool set, wlc_if_t *wlcif);
int wlc_iovar_getint(wlc_info_t *wlc, const char *name, int *arg);
#define wlc_iovar_getuint(wlc, name, arg) wlc_iovar_getint(wlc, name, (int *)(arg))
int wlc_iovar_setint(wlc_info_t *wlc, const char *name, int arg);
#define wlc_iovar_setuint(wlc, name, arg) wlc_iovar_setint(wlc, name, (int)(arg))
int wlc_iovar_getint8(wlc_info_t *wlc, const char *name, int8 *arg);
#define wlc_iovar_getuint8(wlc, name, arg) wlc_iovar_getint8(wlc, name, (int8 *)(arg))
int wlc_iovar_getbool(wlc_info_t *wlc, const char *name, bool *arg);

#endif /* _wlc_iocv_h_ */
