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



#include <wlc_types.h>
#include <typedefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <wlc_log.h>
#include <wl_dbg.h>
#include <proto/ethernet.h>
#include <proto/802.11.h>
#include <wlioctl.h>
#include <wlc.h>
#include <wlc_rx.h>

void wlc_log_unexpected_rx_frame_log_80211hdr(wlc_info_t *wlc,
	const struct dot11_header *h, int toss_reason)
{
	BCM_REFERENCE(toss_reason);
	toss_reason = (toss_reason >= WLC_RX_STS_LAST)? WLC_RX_STS_TOSS_UNKNOWN : toss_reason;
	WL_ERROR(("Unexpected RX {if=wl%d fc=%04x seq=%04x "
		"A1=%02x:%02x:%02x:%02x:%02x:%02x A2=%02x:%02x:%02x:%02x:%02x:%02x}\n",
		WLCWLUNIT(wlc), ltoh16(h->fc), ltoh16(h->seq),
		h->a1.octet[0], h->a1.octet[1], h->a1.octet[2],
		h->a1.octet[3], h->a1.octet[4], h->a1.octet[5],
		h->a2.octet[0], h->a2.octet[1], h->a2.octet[2],
		h->a2.octet[3], h->a2.octet[4], h->a2.octet[5]));
}

void wlc_log_unexpected_tx_frame_log_80211hdr(wlc_info_t *wlc, const struct dot11_header *h)
{
	WL_ERROR(("Unexpected TX {if=wl%d fc=%04x seq=%04x "
		"A1=%02x:%02x:%02x:%02x:%02x:%02x A2=%02x:%02x:%02x:%02x:%02x:%02x}\n",
		WLCWLUNIT(wlc), ltoh16(h->fc), ltoh16(h->seq),
		h->a1.octet[0], h->a1.octet[1], h->a1.octet[2],
		h->a1.octet[3], h->a1.octet[4], h->a1.octet[5],
		h->a2.octet[0], h->a2.octet[1], h->a2.octet[2],
		h->a2.octet[3], h->a2.octet[4], h->a2.octet[5]));
}

void wlc_log_unexpected_tx_frame_log_8023hdr(wlc_info_t *wlc, const struct ether_header *eh)
{
	WL_ERROR(("Unexpected TX {if=wl%d "
		"DST=%02x:%02x:%02x:%02x:%02x:%02x SRC=%02x:%02x:%02x:%02x:%02x:%02x}\n",
		WLCWLUNIT(wlc),
		eh->ether_dhost[0], eh->ether_dhost[1], eh->ether_dhost[2],
		eh->ether_dhost[3], eh->ether_dhost[4], eh->ether_dhost[5],
		eh->ether_shost[0], eh->ether_shost[1], eh->ether_shost[2],
		eh->ether_shost[3], eh->ether_shost[4], eh->ether_shost[5]));
}
