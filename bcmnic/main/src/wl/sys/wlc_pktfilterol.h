/*
 * Broadcom 802.11 Packet Filter offload header
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
 * $Id: wlc_pktfilterol.h 467328 2014-04-03 01:23:40Z $
 */

#ifndef _wlc_pktfilterol_h_
#define _wlc_pktfilterol_h_

extern wlc_dngl_ol_pkt_filter_info_t *wlc_pkt_filter_ol_attach(
		wlc_dngl_ol_info_t *wlc_dngl_ol);
extern void wlc_pkt_filter_ol_clear(
		wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol,
		wlc_dngl_ol_info_t *wlc_dngl_ol);
extern void wlc_pkt_filter_ol_send_proc(
		wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol,
		void *buf,
		int len);
extern int wlc_pkt_filter_ol_process(wlc_info_t *wlc, void *pkt);
extern bool wlc_pkt_filter_ol_enabled(wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol);
extern void wlc_pkt_filter_ol_save_header(
		wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol,
		void *p);
extern void wlc_pkt_filter_ol_event_handler(
		wlc_dngl_ol_info_t	*wlc_dngl_ol,
		uint32			event,
		void			*event_data);
#endif /* _wlc_pktfilterol_h_ */
