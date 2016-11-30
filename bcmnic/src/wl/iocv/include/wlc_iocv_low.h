/*
 * IOCV module interface.
 * For BMAC/PHY.
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
 * $Id: wlc_iocv_low.h 614277 2016-01-21 20:28:50Z $
 */

#ifndef _wlc_iocv_low_h_
#define _wlc_iocv_low_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <osl_decl.h>

#include <wlc_iocv_types.h>

/* attach/detach */
wlc_iocv_info_t *wlc_iocv_low_attach(osl_t *osh, uint16 iovt_cnt, uint16 ioct_cnt);
void wlc_iocv_low_detach(wlc_iocv_info_t *ii);

/* dump function - need to be registered to dump facility by a caller */
int wlc_iocv_low_dump(wlc_iocv_info_t *ii, struct bcmstrbuf *b);

/*
 * Dispatch iovar with table id 'tid' and action id 'aid'
 * to a registered dispatch callback.
 *
 * Return BCME_XXXX.
 */
int wlc_iocv_low_dispatch_iov(wlc_iocv_info_t *ii, uint16 tid, uint32 aid,
	uint8 *p, uint p_len, uint8 *a, uint a_len, uint v_sz, struct wlc_if *wlcif);

/*
 * Dispatch ioctl with table id 'tid' and command id 'cid'
 * to a registered dispatch callback.
 *
 * Return BCME_XXXX.
 */
int wlc_iocv_low_dispatch_ioc(wlc_iocv_info_t *ii, uint16 tid, uint32 cid,
	uint8 *a, uint a_len, struct wlc_if *wlcif);

#endif /* _wlc_iocv_low_h_ */
