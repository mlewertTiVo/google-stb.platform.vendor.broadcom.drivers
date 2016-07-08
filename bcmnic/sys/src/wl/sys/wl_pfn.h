/*
 * Preferred network header file
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
 * $Id: wl_pfn.h 543582 2015-03-24 19:48:01Z $
 */


#ifndef _wl_pfn_h_
#define _wl_pfn_h_

/* This define is to help mogrify the code */
#ifdef WLPFN

/* forward declaration */
typedef struct wl_pfn_info wl_pfn_info_t;
typedef struct wl_pfn_bdcast_list wl_pfn_bdcast_list_t;

/* Function prototype */
wl_pfn_info_t * wl_pfn_attach(wlc_info_t * wlc);
int wl_pfn_detach(wl_pfn_info_t * data);
extern void wl_pfn_event(wl_pfn_info_t * data, wlc_event_t * e);
extern int wl_pfn_scan_in_progress(wl_pfn_info_t * data);
extern void wl_pfn_process_scan_result(wl_pfn_info_t * data, wlc_bss_info_t * bi);
extern bool wl_pfn_scan_state_enabled(wlc_info_t *wlc);
extern bool wl_pfn_is_ch_bucket_flag_enabled(wl_pfn_info_t *pfn_info, uint16 bssid_channel,
             uint32 flag_type);
extern void wl_pfn_inform_mac_availability(wlc_info_t *wlc);
#endif /* WLPFN */

#endif /* _wl_pfn_h_ */
