/*
 * IOCV module interface - iovar subcommand facility.
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
 * $Id: wlc_iocv_cmd.h 622560 2016-03-03 02:41:41Z $
 */

#ifndef _wlc_iocv_cmd_t_
#define _wlc_iocv_cmd_t_

struct wlc_if;

typedef int (wlc_iov_cmd_fn_t)(void *ctx, uint8 *params, uint16 plen,
	uint8 *result, uint16 *rlen, bool set, struct wlc_if *wlcif);

/*  each mesh submodule defines list of "wl mesh" <cmd> handlers  */
typedef struct {
	uint16 cmd;
	uint16 flags;    /* See IOVF_XXXX flags in wlc_iocv_desc.h */
	uint16 min_len;  /* for result buffer length validation */
	wlc_iov_cmd_fn_t *hdlr;
} wlc_iov_cmd_t;

int wlc_iocv_iov_cmd_proc(void *ctx, const wlc_iov_cmd_t *tbl, uint sz, bool set,
	void *in_buf, uint in_len, void *out_buf, uint out_len, struct wlc_if *wlcif);

#endif /* _wlc_iocv_cmd_t_ */
