/*
 * IOCV module interface - basic data type declarations.
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
 * $Id: wlc_iocv_types.h 614277 2016-01-21 20:28:50Z $
 */

#ifndef _wlc_iocv_types_h_
#define _wlc_iocv_types_h_

#include <typedefs.h>

/* forward declarations */
typedef struct wlc_iocv_info wlc_iocv_info_t;

#define WLC_IOCV_NEW_IF

struct wlc_if;

/* IOVar dispatcher
 *
 * ctx - a pointer value registered with the function
 * aid - action ID, calculated by IOV_GVAL() and IOV_SVAL() based on varid.
 * p/plen - parameters and length for a get, input only.
 * a/alen - buffer and length for value to be set or retrieved, input or output.
 * vsz - value size, valid for integer type only.
 * wlcif - interface context (wlc_if pointer)
 *
 * p/a pointers may point into the same buffer.
 */
/* iovar dispatch function */
typedef int (*wlc_iov_disp_fn_t)(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsz, struct wlc_if *wlcif);
/* IOCtl dispatcher
 *
 * ctx - a pointer value registered with the function
 * cid - command/ioctl ID defined in $components/shared/devctrl_if/wlioctl_defs.h
 * a/alen - parameters and length and buffer and length for a GET command;
 *          parameters and length for a SET command
 * wlcif - interface context (wlc_if pointer)
 *
 * p/a pointers may point into the same buffer.
 */
/* ioctl dispatch function */
typedef int (*wlc_ioc_disp_fn_t)(void *ctx, uint32 cid,
	void *a, uint alen, struct wlc_if *wlcif);

#endif /* _wlc_iocv_types_h_ */
