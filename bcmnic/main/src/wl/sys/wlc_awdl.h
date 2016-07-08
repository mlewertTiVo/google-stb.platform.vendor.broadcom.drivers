/*
 * AWDL offloading.
 *
 *	This feature implements periodic AWDL.
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
 * <<Broadcom-WL-IPTag/Private1743:http://confluence.broadcom.com/display/WLAN/BuildIPValidation>>
 *
 * $Id: wlc_awdl.h,v 1.1 jaganlv Exp $
 */

#ifndef _wlc_awdl_h
#define _wlc_awdl_h

#define wlc_awdl_attach(a)		(awdl_info_t *)0x0dadbeef
#define wlc_awdl_detach(a)		do {} while (0)
#define wl_awdl_down(a)		((void)(0))
#define wlc_awdl_recv_process_prbresp(a, b, c, d, e) FALSE
#define wlc_awdl_pkt_inc(a, b, c, d)		do {} while (0)
#define wlc_awdl_scb_create(a, b, c, d) NULL
#define wlc_awdl_tx_complete(a, b, c) do {} while (0)
#define awdl_af_hdr(a)		NULL
#define wlc_awdl_enq_mcast_pkt(a, b) do {} while (0)
#define wlc_awdl_in_aw(a) FALSE
#define wlc_awdl_in_pre_aw(a) FALSE
#define wlc_awdl_valid_channel(a) FALSE
#define wlc_awdl_is_near_aw(a)	FALSE
#define wlc_awdl_tx_ready(a, b) do {} while (0)
#define awdl_in_chan_seq(a, b) FALSE
#define wlc_awdl_mon_bssid_match(a, b) FALSE
#define wlc_awdl_bssid_dst_match(a, b, c, d) FALSE
#define wlc_awdl_scb_cleanup_unused(a) do {} while (0)
#endif	/* _wlc_awdl_h */
