/*
 * Broadcom 802.11 Packet filter offload driver
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
 * $Id: wlc_pktfilterol.c 467328 2014-04-03 01:23:40Z $
 */

#include <osl.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <bcmendian.h>
#include <proto/802.3.h>
#include <proto/ethernet.h>
#include <bcm_ol_msg.h>
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_hw_priv.h>
#include <wlc_dngl_ol.h>
#include <wlc_wowlol.h>
#include <wlc_pktfilterol.h>
#include <wlc_pkt_filter.h>


/* Starting delimiter of a standard magic packet */
uchar magic_pkt_delimiter[ETHER_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

#define WLC_RX_CHANNEL(rxh)		(CHSPEC_CHANNEL((rxh)->RxChan))

/* Packet filter offload private info structure */
struct wlc_dngl_ol_pkt_filter_info {
	wlc_dngl_ol_info_t *wlc_dngl_ol;

	/* Received message from Host to enable the packet filter offload */
	bool			pkt_filter_ol_enable;

	/* Packet filter info block (defined in wlc_pkt_filter.c */
	wlc_pkt_filter_info_t	*pkt_filter_info;

	/* Cached 802.11 header data */
	struct dot11_header	d11_hdr;
	uint			d11_hdr_len;

	/* WoWL info */
	bool			wowl_pme_asserted;

	/* RSSI RATE AND CHANNEL */
	uint32              channel;
	uint32              rssi;
	uint32              rate;
	uint32              phy_type;
 };


/* Private functions */
static bool wlc_pkt_filter_ol_magic_packet(
		wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol,
		void *pkt);

static void wlc_pkt_filter_ol_wake_host(
		wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol,
		uint32 wake_reason,
		uint32 pattern_id,
		void *wake_pkt);


/*
 * Initialize packet filter private context.
 * Returns a pointer to the packet filter private context, NULL on failure.
 */
wlc_dngl_ol_pkt_filter_info_t *
BCMATTACHFN(wlc_pkt_filter_ol_attach)(wlc_dngl_ol_info_t *wlc_dngl_ol)
{
	wlc_dngl_ol_pkt_filter_info_t	*pkt_filter_ol;

	/* allocate packet filter private info struct */
	pkt_filter_ol =
	    (wlc_dngl_ol_pkt_filter_info_t *)MALLOC(wlc_dngl_ol->osh,
		sizeof(wlc_dngl_ol_pkt_filter_info_t));

	if (!pkt_filter_ol) {
		WL_ERROR(("pkt_filter_ol malloc failed: %s\n", __FUNCTION__));
		return NULL;
	}

	/* init packet filter private info struct */
	bzero(pkt_filter_ol, sizeof(wlc_dngl_ol_pkt_filter_info_t));

	pkt_filter_ol->wlc_dngl_ol = wlc_dngl_ol;

	/* allocate packet filter info block */
	pkt_filter_ol->pkt_filter_info = wlc_pkt_filter_attach(wlc_dngl_ol);
	if (!pkt_filter_ol->pkt_filter_info) {
	    WL_ERROR(("%s: wlc_pkt_filter_attach failed\n", __FUNCTION__));

	    MFREE(wlc_dngl_ol->osh, pkt_filter_ol, sizeof(wlc_dngl_ol_pkt_filter_info_t));
	}

	wlc_pkt_filter_ol_clear(pkt_filter_ol, wlc_dngl_ol);


	WL_TRACE(("Packet filter attached\n"));
	return pkt_filter_ol;
}

void wlc_pkt_filter_ol_clear(wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol,
	wlc_dngl_ol_info_t * wlc_dngl_ol)
{
	/* Clear all of the offloaded pkt filters. */
	if (pkt_filter_ol->pkt_filter_info != NULL)
		wlc_pkt_fitler_delete_filters(pkt_filter_ol->pkt_filter_info);
}

bool wlc_pkt_filter_ol_enabled(wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol)
{
	return pkt_filter_ol->pkt_filter_ol_enable;

}

void wlc_pkt_filter_ol_send_proc(wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol,
	void *buf, int len)
{
	olmsg_header *msg_hdr;
	wlc_dngl_ol_info_t *wlc_dngl_ol = pkt_filter_ol->wlc_dngl_ol;
	olmsg_pkt_filter_enable *pkt_filter_enable;
	olmsg_pkt_filter_add *filter;
	int err;

	msg_hdr = (olmsg_header *)buf;

	WL_TRACE(("%s: message type:%d\n", __FUNCTION__, msg_hdr->type));

	switch (msg_hdr->type) {
		case BCM_OL_PKT_FILTER_ENABLE:
			pkt_filter_enable = (olmsg_pkt_filter_enable *)buf;

			if (pkt_filter_ol->pkt_filter_ol_enable == FALSE) {
				wlc_pkt_filter_ol_clear(pkt_filter_ol, wlc_dngl_ol);

				bcopy(&pkt_filter_enable->host_mac,
				      &wlc_dngl_ol->cur_etheraddr,
				      sizeof(struct ether_addr));

				/*
				 * Initialize PME state. Don't initialize WoWL state since
				 * we get that when WoWL call our CB.
				 */
				pkt_filter_ol->d11_hdr_len	    = 0;
				pkt_filter_ol->wowl_pme_asserted    = FALSE;
				pkt_filter_ol->pkt_filter_ol_enable = TRUE;

				WL_TRACE(("%s: pkt_filter_ol enabled\n", __FUNCTION__));
			}

			break;

	case BCM_OL_PKT_FILTER_ADD:
			filter = (olmsg_pkt_filter_add *)buf;

			if (pkt_filter_ol->pkt_filter_ol_enable == TRUE) {
				/* Add a wowl packet filter from host driver */
				err = wlc_pkt_filter_add(
							pkt_filter_ol->pkt_filter_info,
							&filter->pkt_filter);

				if (err != BCME_OK) {
					WL_ERROR(("%s: wlc_pkt_filter_add failed. err = %d\n",
						__FUNCTION__, err));

					break;
				}

				WL_TRACE(("%s: added filter. ID = 0x%08x\n",
				    __FUNCTION__, filter->pkt_filter.id));

			} else {
				WL_ERROR(("%s: attempt to add pkt filter "
					    "when ol is not enabled\n",
					    __FUNCTION__));
			}

			break;

	case BCM_OL_PKT_FILTER_DISABLE:
			if (pkt_filter_ol->pkt_filter_ol_enable == TRUE) {
				wlc_pkt_filter_ol_clear(pkt_filter_ol, wlc_dngl_ol);

				pkt_filter_ol->pkt_filter_info = NULL;
				pkt_filter_ol->pkt_filter_ol_enable = FALSE;

				WL_TRACE(("%s: pkt_filter_ol disabled\n", __FUNCTION__));
			}

			break;

	default:
		WL_ERROR(("%s: INVALID message type:%d\n",
		    __FUNCTION__, msg_hdr->type));
	}
}


int
wlc_pkt_filter_ol_process(wlc_info_t *wlc, void *pkt)
{
	olmsg_pkt_filter_stats stats;
	bool			matched = FALSE;
	uint32			id;
	uint32			wake_reason = 0;
	wlc_dngl_ol_pkt_filter_info_t	*pkt_filter_ol;
	wlc_dngl_ol_info_t	*wlc_dngl_ol;
	wowl_cfg_t		*wowl_cfg;
	wlc_pkt_filter_info_t	*pkt_info = NULL;
	wlc_d11rxhdr_t *wrxh = NULL;
	uint8 *plcp = NULL;
	ratespec_t rspec;

	ENTER();

	pkt_filter_ol = wlc->wlc_dngl_ol->pkt_filter_ol;
	wlc_dngl_ol   = pkt_filter_ol->wlc_dngl_ol;
	wowl_cfg      = &wlc_dngl_ol->wowl_cfg;

	WL_TRACE(("%s: packet len = %d\n",
	    __FUNCTION__, PKTLEN(wlc_dngl_ol->osh, pkt)));

	if (!pkt_filter_ol->pkt_filter_ol_enable)
		return -1;

	WL_TRACE(("%s: wowl_enabled = %d, wowl_flags = 0x%08x\n",
	    __FUNCTION__, wowl_cfg->wowl_enabled, wowl_cfg->wowl_flags));

	/*
	 * Don't filter packets if WoWL isn't enabled, Wowl flags are not enabled for
	 * pkt filtering, or we have previously enabled PME.
	 */
	if ((pkt_filter_ol->wowl_pme_asserted == TRUE) ||
	    (wowl_cfg->wowl_enabled == FALSE) ||
	    !(wowl_cfg->wowl_flags & (WL_WOWL_MAGIC | WL_WOWL_NET)))
		    return -1;

	bzero(&stats, sizeof(olmsg_pkt_filter_stats));

	wrxh          = (wlc_d11rxhdr_t *)PKTDATA(wlc_dngl_ol->osh, pkt);
	plcp          = PKTDATA(wlc_dngl_ol->osh, pkt);
	rspec         = CCK_RSPEC(CCK_PHY2MAC_RATE(((cck_phy_hdr_t *)plcp)->signal));
	pkt_info      = pkt_filter_ol->pkt_filter_info;

	/* Total number of packets received to be filtered */
	RXOEINC(wlc_dngl_ol, rxoe_total_pkt_filter_cnt);
	stats.pkt_filtered = 1;


	pkt_filter_ol->channel = WLC_RX_CHANNEL(&wrxh->rxhdr);
	pkt_filter_ol->rssi = wrxh->rssi;
	pkt_filter_ol->rate = rspec;
	pkt_filter_ol->phy_type = wlc->hw->band->phytype;

	/*
	 * If  the wake packet is a broadcast/multicast frame, set the
	 * WL_WOWL_BCAST bit in wake reason.
	 */
	if (ETHER_ISMULTI(&pkt_filter_ol->d11_hdr.a1)) {
		wake_reason = WL_WOWL_BCAST;
	}

	/* Filter the packet */
	if (wowl_cfg->wowl_flags & WL_WOWL_NET) {
		matched = wlc_pkt_filter_recv_proc_ex(
					    pkt_filter_ol->pkt_filter_info,
					    pkt,
					    &id);

		if (matched) {
			WL_TRACE(("%s: matched filter!\n", __FUNCTION__));

			RXOEINC(wlc_dngl_ol, rxoe_total_matching_pattern_cnt);
			stats.matched_pattern = 1;
			stats.suppressed = 1;
			stats.pattern_id = id;

			WL_TRACE(("%s: waking the host for WL_WOWL_NET\n", __FUNCTION__));

			wake_reason |= WL_WOWL_NET;

			wlc_pkt_filter_ol_wake_host(pkt_filter_ol, wake_reason, id, pkt);
		}
	}

	if  ((matched == FALSE) && (wowl_cfg->wowl_flags & WL_WOWL_MAGIC)) {
		matched = wlc_pkt_filter_ol_magic_packet(pkt_filter_ol, pkt);

		if (matched) {
			WL_TRACE(("%s: matched magic packet\n", __FUNCTION__));

			RXOEINC(wlc_dngl_ol, rxoe_total_matching_magic_cnt);
			stats.suppressed = 1;
			stats.matched_magic = 1;

			WL_TRACE(("%s: waking the host for WL_WOWL_MAGIC\n", __FUNCTION__));

			wake_reason |= WL_WOWL_MAGIC;

			wlc_pkt_filter_ol_wake_host(pkt_filter_ol, wake_reason, 0, pkt);
		}
	}

	RXOEADD_PKT_FILTER_ENTRY(wlc_dngl_ol, stats);

	EXIT();

	return (matched ? 1: -1);
}

static void
wlc_pkt_filter_ol_wowl_enabled(
	wlc_dngl_ol_pkt_filter_info_t	*pkt_filter_ol,
	wowl_cfg_t			*wowl_cfg)
{
	if (wowl_cfg->wowl_enabled == TRUE) {
		/*
		 * NOTE: We currently don't support a WoWL disable state!
		 * That is because asserting PME will result in a PCIe reset,
		 * which stops the ARM.
		 */
		pkt_filter_ol->wowl_pme_asserted = FALSE;
		pkt_filter_ol->d11_hdr_len	 = 0;
	}
}

void
wlc_pkt_filter_ol_event_handler(
	wlc_dngl_ol_info_t	*wlc_dngl_ol,
	uint32			event,
	void			*event_data)
{
	wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol;

	ENTER();

	pkt_filter_ol = wlc_dngl_ol->pkt_filter_ol;

	switch (event) {
		case BCM_OL_E_WOWL_START:
			/* We don't need to explicitly handle this event */
			break;

		case BCM_OL_E_WOWL_COMPLETE:
			wlc_pkt_filter_ol_wowl_enabled(pkt_filter_ol, event_data);

			break;

		case BCM_OL_E_PME_ASSERTED:
			/*
			 * Either we or another module asserted PME, so save state
			 * so we don't try to asssert PME.
			 */
			pkt_filter_ol->wowl_pme_asserted = TRUE;

			break;
	}

	EXIT();
}

static bool
wlc_pkt_filter_ol_magic_packet(wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol, void *pkt)
{
	wlc_dngl_ol_info_t *wlc_dngl_ol = pkt_filter_ol->wlc_dngl_ol;
	osl_t *osh;
	uchar *pkt_ptr;
	uint32 pkt_len, i;

	ENTER();

	osh	= wlc_dngl_ol->osh;
	pkt_ptr = PKTDATA(osh, pkt) + ETHER_HDR_LEN;
	pkt_len = PKTLEN(osh, pkt) - ETHER_HDR_LEN;

	if (pkt_len <  MAGIC_PKT_MINLEN) {
		WL_INFORM(("%s: Packet too small = %d\n", __FUNCTION__, pkt_len));

		return FALSE;
	}

	while (pkt_len >= MAGIC_PKT_MINLEN) {
		/* Look for starting delimiter of 6 0xff's */
		if (bcmp(pkt_ptr, &magic_pkt_delimiter, ETHER_ADDR_LEN) == 0) {
			/* Found the delimiter so see if we have the MAC address pattern */
			WL_TRACE(("%s: found delimiter\n", __FUNCTION__));

			pkt_ptr += ETHER_ADDR_LEN;
			pkt_len -= ETHER_ADDR_LEN;

			for (i = 0; i < MAGIC_PKT_NUM_MAC_ADDRS; i++) {
				if (bcmp(pkt_ptr, &wlc_dngl_ol->cur_etheraddr, ETHER_ADDR_LEN) != 0)
					break;

				/* Found a MAC address, so advance and keep going */
				pkt_ptr += ETHER_ADDR_LEN;
				pkt_len -= ETHER_ADDR_LEN;
			}

			if (i >= MAGIC_PKT_NUM_MAC_ADDRS)
				return TRUE;
		} else {
			/* Haven't found it yet, so advance and keep going */
			pkt_ptr++;
			pkt_len--;
		}
	}

	EXIT();

	return FALSE;
}

/* Return wakeup packet info to host */
static void
wlc_pkt_filter_ol_wake_host(
		wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol,
		uint32 wake_reason,
		uint32 pattern_id,
		void *wake_pkt)
{
	wowl_cfg_t *wowl_cfg = &pkt_filter_ol->wlc_dngl_ol->wowl_cfg;
	wowl_wake_info_t		pattern_pkt_info;
	osl_t				*osh;
	uint8				*d11_snap_hdr;
	struct ether_header		*eth_hdr;
	uint16				ether_type;


	ENTER();
	if ((pkt_filter_ol->wowl_pme_asserted == TRUE) ||
	    ((wowl_cfg->wowl_flags & (WL_WOWL_MAGIC | WL_WOWL_NET)) == 0) ||
	    (wake_pkt == NULL)) {
		WL_ERROR(("%s: PME already asserted or no wake pkt\n", __FUNCTION__));

		return;
	}

	/* We caused the PME assetion so return the wake packet info */
	osh = pkt_filter_ol->wlc_dngl_ol->osh;


	eth_hdr	    = (struct ether_header *)PKTDATA(osh, wake_pkt);
	ether_type  = ntoh16(eth_hdr->ether_type);

	if (ether_type > ETHER_MAX_DATA) {
		/*
		 * Convert the packet back to 802.11 format from Ethernet II format.
		 * In this case, we need to add room for the 802.11 header and SNAP/LLC
		 * header.
		 *
		 * NOTE: We convert the wake packet to 802.11 format since this is
		 * an NDIS requirement. We may need to change this for other per-port
		 * interfaces.
		 */
		PKTPULL(osh, wake_pkt, sizeof(struct ether_header));
		PKTPUSH(osh, wake_pkt, pkt_filter_ol->d11_hdr_len +
			sizeof(struct dot11_llc_snap_header));

		/* Copy the original d11 header back */
		bcopy(
		    &pkt_filter_ol->d11_hdr,
		    PKTDATA(osh, wake_pkt),
		    pkt_filter_ol->d11_hdr_len);

		/*
		 * Add the snap header. We shouldn't need to update the EtherType
		 * if we did this right:)
		 */
		d11_snap_hdr = PKTDATA(osh, wake_pkt) + pkt_filter_ol->d11_hdr_len;

		d11_snap_hdr[0] = 0xAA;
		d11_snap_hdr[1] = 0xAA;
		d11_snap_hdr[2] = 0x03;
		d11_snap_hdr[3] = 0x00;
		d11_snap_hdr[4] = 0x00;
		d11_snap_hdr[5] = 0x00;
	} else {
		/*
		 * Convert the packet back to 802.11 format from 802.3 format. In this case,
		 * we only need to add the 802.11 header. We'll retain the original SNAP/LLC
		 * header
		 */
		PKTPULL(osh, wake_pkt, sizeof(struct ether_header));
		PKTPUSH(osh, wake_pkt, pkt_filter_ol->d11_hdr_len);

		/* Copy the original d11 header back */
		bcopy(
		    &pkt_filter_ol->d11_hdr,
		    PKTDATA(osh, wake_pkt),
		    pkt_filter_ol->d11_hdr_len);
	}

	pattern_pkt_info.pattern_id		= pattern_id;
	pattern_pkt_info.original_packet_size = PKTLEN(osh, wake_pkt);
	pattern_pkt_info.phy_type       = pkt_filter_ol->phy_type;
	pattern_pkt_info.channel		= pkt_filter_ol->channel;
	pattern_pkt_info.rate			= pkt_filter_ol->rate;
	pattern_pkt_info.rssi           = pkt_filter_ol->rssi;


	/* Save state so we don't process additional packets before the host resumes to D0 */
	pkt_filter_ol->wowl_pme_asserted = TRUE;

	wlc_wowl_ol_wake_host(
						pkt_filter_ol->wlc_dngl_ol->wowl_ol,
						&pattern_pkt_info, sizeof(pattern_pkt_info),
						PKTDATA(osh, wake_pkt),
						MIN(MAX_WAKE_PACKET_BYTES, PKTLEN(osh, wake_pkt)),
						wake_reason);

	EXIT();
}


void
wlc_pkt_filter_ol_save_header(wlc_dngl_ol_pkt_filter_info_t *pkt_filter_ol, void *p)
{
	wlc_dngl_ol_info_t *wlc_dngl_ol = pkt_filter_ol->wlc_dngl_ol;
	struct dot11_header *h;
	bool qos;
	uint16 type, subtype;
	wowl_cfg_t *wowl_cfg;

	ENTER();

	wowl_cfg = &wlc_dngl_ol->wowl_cfg;

	if ((pkt_filter_ol->pkt_filter_ol_enable) &&
	    (pkt_filter_ol->wowl_pme_asserted == FALSE) &&
	    (wowl_cfg->wowl_enabled == TRUE) &&
	    (wowl_cfg->wowl_flags & (WL_WOWL_MAGIC | WL_WOWL_NET))) {
		    h	    = (struct dot11_header *)(PKTDATA(pkt_filter_ol->wlc_dngl_ol->osh, p));
		    type    = FC_TYPE(h->fc);
		    subtype = (h->fc & FC_SUBTYPE_MASK) >> FC_SUBTYPE_SHIFT;
		    qos	    = (type == FC_TYPE_DATA && FC_SUBTYPE_ANY_QOS(subtype));

		    pkt_filter_ol->d11_hdr_len = DOT11_A3_HDR_LEN;

		    bcopy(h, &pkt_filter_ol->d11_hdr, DOT11_A3_HDR_LEN);

		    WL_TRACE(("%s: QoS enabled: %d\n", __FUNCTION__, qos));

		    if (qos) {
			    /* Save off the QoS header by overwriting A4 in the 802.11 header. */
			    pkt_filter_ol->d11_hdr_len += DOT11_QOS_LEN;

			    bcopy(
				    (uchar *)h + DOT11_A3_HDR_LEN,
				    (uchar *)&pkt_filter_ol->d11_hdr + DOT11_A3_HDR_LEN,
				    DOT11_QOS_LEN);
		    }

	}

	EXIT();
}
