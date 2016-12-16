/**
 * @file
 * @brief
 * scan related wrapper routines and scan results related functions
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
 * $Id: wlc_scan_utils.h 658917 2016-09-11 08:36:30Z $
 */

#ifndef _wlc_scan_utils_h_
#define _wlc_scan_utils_h_

#include <typedefs.h>
#include <proto/ethernet.h>
#include <proto/802.11.h>
#include <d11.h>
#include <wlc_cfg.h>
#include <wlc_types.h>
#include <wlc_scan.h>

/* For managing scan result lists */
struct wlc_bss_list {
	uint		count;
	wlc_bss_info_t*	ptrs[MAXBSS];
};

typedef struct scan_utl_scan_data {
	wlc_d11rxhdr_t *wrxh;
	uint8 *plcp;
	struct dot11_management_header *hdr;
	uint8 *body;
	int body_len;
	wlc_bss_info_t *bi;
} scan_utl_scan_data_t;

typedef void (*scan_utl_rx_scan_data_fn_t)(void *ctx, scan_utl_scan_data_t *data);
typedef void (*scan_utl_scan_start_fn_t)(void *ctx);

#define wlc_scan_request(wlc, bss_type, bssid, nssid, ssid, scan_type, nprobes, \
		active_time, passive_time, home_time, chanspec_list, chanspec_num, \
		save_prb, fn, arg) \
	wlc_scan_request_ex(wlc, bss_type, bssid, nssid, ssid, scan_type, nprobes, \
		active_time, passive_time, home_time, chanspec_list, chanspec_num, 0, \
		save_prb, fn, arg, WLC_ACTION_SCAN, FALSE, NULL, NULL, NULL)
int wlc_scan_request_ex(wlc_info_t *wlc, int bss_type, const struct ether_addr* bssid,
	int nssid, wlc_ssid_t *ssids, int scan_type, int nprobes,
	int active_time, int passive_time, int home_time,
	const chanspec_t* chanspec_list, int chanspec_num,
	chanspec_t chanspec_start, bool save_prb,
	scancb_fn_t fn, void* arg,
	int macreq, uint scan_flags, wlc_bsscfg_t *cfg,
	actcb_fn_t act_cb, void *act_cb_arg);

int wlc_custom_scan(wlc_info_t *wlc, char *arg, int arg_len,
	chanspec_t chanspec_start, int macreq, wlc_bsscfg_t *cfg);
void wlc_custom_scan_complete(void *arg, int status, wlc_bsscfg_t *cfg);

int wlc_recv_scan_parse(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh, uint8 *plcp,
	struct dot11_management_header *hdr, uint8 *body, int body_len);
int wlc_recv_scan_parse_bcn_prb(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh,
	struct ether_addr *bssid, bool beacon,
	uint8 *body, uint body_len, wlc_bss_info_t *bi);

#define ISCAN_IN_PROGRESS(wlc)  wlc_scan_utils_iscan_inprog(wlc)
bool wlc_scan_utils_iscan_inprog(wlc_info_t *wlc);

void wlc_scan_utils_set_chanspec(wlc_info_t *wlc, chanspec_t chanspec);
void wlc_scan_utils_set_syncid(wlc_info_t *wlc, uint16 syncid);

int wlc_scan_utils_rx_scan_register(wlc_info_t *wlc, scan_utl_rx_scan_data_fn_t fn, void *arg);
int wlc_scan_utils_rx_scan_unregister(wlc_info_t *wlc, scan_utl_rx_scan_data_fn_t fn, void *arg);

int wlc_scan_utils_scan_start_register(wlc_info_t *wlc, scan_utl_scan_start_fn_t fn, void *arg);
int wlc_scan_utils_scan_start_unregister(wlc_info_t *wlc, scan_utl_scan_start_fn_t fn, void *arg);

#ifdef WLRSDB
void wlc_scan_utils_scan_complete(wlc_info_t *wlc, wlc_bsscfg_t *scan_cfg);
#endif

/* attach/detach */
wlc_scan_utils_t *wlc_scan_utils_attach(wlc_info_t *wlc);
void wlc_scan_utils_detach(wlc_scan_utils_t *sui);

#endif /* _wlc_scan_utils_h_ */
