/*
 * Miscellanous features interface.
 * For features that don't warrant separate wlc modules.
 *
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
 * $Id: wlc_misc.h 596126 2015-10-29 19:53:48Z $
 */

#ifndef _wlc_misc_h_
#define _wlc_misc_h_

#include <wlc_types.h>
#include <wlc_wpa.h>

wlc_misc_info_t *wlc_misc_attach(wlc_info_t *wlc);
void wlc_misc_detach(wlc_misc_info_t *misc);

bool wlc_is_4way_msg(wlc_info_t *wlc, void *pkt, int offset, wpa_msg_t msg);

void wlc_sr_fix_up(wlc_info_t *wlc);

#endif /* _wlc_misc_h_ */
