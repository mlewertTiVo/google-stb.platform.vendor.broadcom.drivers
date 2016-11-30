/*
 * IOCV module interface.
 * For WLC.
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
 * $Id: wlc_iocv_high.h 614277 2016-01-21 20:28:50Z $
 */

#ifndef _wlc_iocv_high_h_
#define _wlc_iocv_high_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <osl_decl.h>

#include <wlc_iocv_types.h>

/* attach/detach */
wlc_iocv_info_t *wlc_iocv_high_attach(osl_t *osh, uint16 iovt_cnt, uint16 ioct_cnt,
	void *high_ctx, void *low_ctx);
void wlc_iocv_high_detach(wlc_iocv_info_t *ii);

/* dump function - need to be registered to dump facility by a caller */
int wlc_iocv_high_dump(wlc_iocv_info_t *ii, struct bcmstrbuf *b);

/*
 * TODO: Remove unnecessary interface
 * once integrated with existing iovar table handling.
 */

/* lookup iovar and return iovar entry and table id if found */
const bcm_iovar_t *wlc_iocv_high_find_iov(wlc_iocv_info_t *ii, const char *name, uint16 *tid);

/* forward iovar to registered table dispatcher */
int wlc_iocv_high_fwd_iov(wlc_iocv_info_t *ii, uint16 tid, uint32 aid, const bcm_iovar_t *vi,
	uint8 *p, uint p_len, uint8 *a, uint a_len, uint var_sz, struct wlc_if *wlcif);

/* lookup ioctl and return ioctl entry and table id if found */
const wlc_ioctl_cmd_t *wlc_iocv_high_find_ioc(wlc_iocv_info_t *ii, uint32 cid, uint16 *tid);

/* forward ioctl to registered table dispatcher */
int wlc_iocv_high_fwd_ioc(wlc_iocv_info_t *ii, uint16 tid, const wlc_ioctl_cmd_t *ci,
	uint8 *a, uint a_len, struct wlc_if *wlcif);

/* dispatch ioctl that doesn't have a table registered */
int wlc_iocv_high_proc_ioc(wlc_iocv_info_t *ii, uint32 cid,
	uint8 *a, uint a_len, struct wlc_if *wlcif);

/* query all iovar names */
int wlc_iocv_high_iov_names(wlc_iocv_info_t *ii, uint name_bytes, uint var_start, uint out_max,
	uint8 *buf, uint len);

/* validate ioctl */
int wlc_iocv_high_vld_ioc(wlc_iocv_info_t *ii, uint16 tid, const wlc_ioctl_cmd_t *ci,
	void *a, uint a_len);

#endif /* _wlc_iocv_high_h_ */
