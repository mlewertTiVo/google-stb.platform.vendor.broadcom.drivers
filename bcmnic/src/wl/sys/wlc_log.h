/*
 * Event mechanism
 *
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
 * $Id:  $
 */

/**
 * @file
 * @brief
 * The WLAN driver currently has tight coupling between different components. In particular,
 * components know about each other, and call each others functions, access data, and invoke
 * callbacks. This means that maintenance and new features require changing these
 * relationships. This is fundamentally a tightly coupled system where everything touches
 * many other things.
 *
 * @brief
 * We can reduce the coupling between our features by reducing their need to directly call
 * each others functions, and access each others data. An mechanism for accomplishing this is
 * a generic event signaling mechanism. The event infrastructure enables modules to communicate
 * indirectly through events, rather than directly by calling each others routines and
 * callbacks.
 */



#ifndef _wlc_log_h_
#define _wlc_log_h_
#include  <wlc_types.h>
#include  <wl_dbg.h>
#include  <proto/802.11.h>

/* Debug error log helpers */
extern void wlc_log_unexpected_rx_frame_log_80211hdr(wlc_info_t *wlc,
	const struct dot11_header *h, int toss_reason);
extern void wlc_log_unexpected_tx_frame_log_80211hdr(wlc_info_t *wlc, const struct dot11_header *h);
extern void wlc_log_unexpected_tx_frame_log_8023hdr(wlc_info_t *wlc, const struct ether_header *eh);

#endif	/* _wlc_log_h_ */
