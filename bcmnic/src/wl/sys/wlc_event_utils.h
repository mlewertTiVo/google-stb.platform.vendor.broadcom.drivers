/*
 * event wrapper related routines
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
 * $Id: wlc_event_utils.h 617509 2016-02-05 15:01:32Z $
 */

#ifndef _wlc_event_utils_h_
#define _wlc_event_utils_h_

#include <typedefs.h>
#include <wlc_types.h>

extern void wlc_event_if(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wlc_event_t *e,
	const struct ether_addr *addr);

extern void wlc_mac_event(wlc_info_t* wlc, uint msg, const struct ether_addr* addr,
	uint result, uint status, uint auth_type, void *data, int datalen);
extern void wlc_bss_mac_event(wlc_info_t* wlc, wlc_bsscfg_t *bsscfg, uint msg,
	const struct ether_addr* addr, uint result, uint status, uint auth_type,
	void *data, int datalen);
extern void wlc_bss_mac_event_immediate(wlc_info_t* wlc, wlc_bsscfg_t *bsscfg, uint msg,
	const struct ether_addr* addr, uint result, uint status, uint auth_type,
	void *data, int datalen);
extern int wlc_bss_mac_rxframe_event(wlc_info_t* wlc, wlc_bsscfg_t *bsscfg, uint msg,
	const struct ether_addr* addr, uint result, uint status, uint auth_type,
	void *data, int datalen, wl_event_rx_frame_data_t *rxframe_data);
extern void wlc_recv_prep_event_rx_frame_data(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh, uint8 *plcp,
                                         wl_event_rx_frame_data_t *rxframe_data);
#if !defined(WLNOEIND)
void wlc_bss_eapol_event(wlc_info_t *wlc, wlc_bsscfg_t * bsscfg,
                                const struct ether_addr *ea, uint8 *data, uint32 len);
void wlc_eapol_event_indicate(wlc_info_t *wlc, void *p, struct scb *scb, wlc_bsscfg_t *bsscfg,
                              struct ether_header *eh);
#else
#define wlc_bss_eapol_event(wlc, cfg, ea, data, len) do {} while (0)
#define wlc_eapol_event_indicate(wlc, p, scb, cfg, ea) do {} while (0)
#endif /* !defined(WLNOEIND) */

extern void wlc_if_event(wlc_info_t *wlc, uint8 what, wlc_if_t *wlcif);
extern void wlc_process_event(wlc_info_t *wlc, wlc_event_t *e);

#endif /* _wlc_event_utils_h_ */
