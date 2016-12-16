/**
 * Channel Context module WLC driver wrapper
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
 * $Id: wlc_chctx.h 658442 2016-09-08 01:17:56Z $
 */
#ifndef _wlc_chctx_h_
#define _wlc_chctx_h_

#include <wlc_types.h>
#include <wlc_chctx_reg.h>

/* pre-attach/post-detach */
wlc_chctx_reg_t *wlc_chctx_pre_attach(wlc_info_t *wlc);
void wlc_chctx_post_detach(wlc_chctx_reg_t *chctx);

/* attach/detach - use these instead of wlc_chctx_reg_xxx */
int wlc_chctx_attach(wlc_chctx_reg_t *chctx);
void wlc_chctx_detach(wlc_chctx_reg_t *chctx);

/* others - use other wlc_chctx_reg_xxx */
/*
wlc_chctx_reg_add_client
wlc_chctx_reg_add_cubby
wlc_chctx_reg_del_cubby
wlc_chctx_reg_notify
*/

#endif /* _wlc_chctx_h_ */
