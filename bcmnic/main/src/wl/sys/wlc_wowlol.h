/*
 * Broadcom 802.11 WoWL offload header
 *
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_wowlol.h 467328 2014-04-03 01:23:40Z $
 */


#ifndef _wlc_wowlol_h_
#define _wlc_wowlol_h_

extern wlc_dngl_ol_wowl_info_t *wlc_wowl_ol_attach(wlc_dngl_ol_info_t * wlc_dngl_ol);
extern void wlc_wowl_ol_send_proc(wlc_dngl_ol_wowl_info_t *wowl_ol, void *buf, int len);
extern void wlc_wowl_ol_wake_host(wlc_dngl_ol_wowl_info_t *wowl_ol, void *wowlol_struct, uint32 len,
	uchar *pkt, uint32 pkt_len, uint32 wake_reason);

#endif	/* _wlc_wowl_ol_h_ */
