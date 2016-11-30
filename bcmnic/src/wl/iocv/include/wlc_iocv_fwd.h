/*
 * IOCV module interface - forwarder interface abstraction.
 *
 * The forwarder is responsible for forwarding the iovar descriptor
 * (result of table lookup) in WLC driver to the iovar table dispatcher
 * in the BMAC/PHY driver.
 *
 * In a full driver the forwarder is just a thin veneer of wlc_iocv_low_dispatch_ioc|iov();
 * in a split drivr the forwarder is a rpc call to the LOW driver which in turn calls
 * wlc_iocv_low_dispatch_ioc|iov(). wlc_iocv_low_dispatch_ioc|iov are declared in
 * wlc_iocv_low.h.
 *
 * The user who instantiates the instance for the BMAC driver (who has the handle)
 * must implement these two interfaces.
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
 * $Id: wlc_iocv_fwd.h 614277 2016-01-21 20:28:50Z $
 */

#ifndef _wlc_iocv_fwd_h_
#define _wlc_iocv_fwd_h_

#include <typedefs.h>
#include <bcmutils.h>

int wlc_iocv_fwd_disp_iov(void *ctx, uint16 tid, uint32 aid, uint16 type,
	void *p, uint p_len, void *a, uint a_len, uint v_sz, struct wlc_if *wlcif);
int wlc_iocv_fwd_disp_ioc(void *ctx, uint16 tid, uint32 cid, uint16 type,
	void *a, uint alen, struct wlc_if *wlcif);

int wlc_iocv_fwd_dump(void *ctx, struct bcmstrbuf *b);

#endif /* _wlc_iocv_fwd_h_ */
