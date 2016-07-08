/*
 * Header for the common Pktfetch use cases in WLC
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
 * $Id: wlc_pktfetch.h 613060 2016-01-16 01:32:36Z $
 */

#ifndef _wlc_pktfetch_h_
#define _wlc_pktfetch_h_


#ifdef BCMSPLITRX
#include <wlc_types.h>
#include <rte_pktfetch.h>
#include <wlc_frmutil.h>
#include <bcmendian.h>
#ifdef WLNDOE
#include <proto/bcmipv6.h>
#endif
#include <wl_export.h>
#ifdef TKO
#include <wl_tko.h>
#endif

typedef struct wlc_eapol_pktfetch_ctx {
	wlc_info_t *wlc;
	wlc_frminfo_t f;
	bool ampdu_path;
	bool ordered;
	struct pktfetch_info *pinfo;
	bool promisc;
	uint32 scb_assoctime;
	uint32 flags;
	struct ether_addr ea; /* Ethernet address of SCB */
} wlc_eapol_pktfetch_ctx_t;
/* this bit shifting is as per RcvHdrConvCtrlSts register */
#define ETHER_HDR_CONV_TYPE     (1<<9)
#define ETHER_TYPE_2_OFFSET 14 /* DA(6) + SA(6)+ PAD(2) */
#define ETHER_TYPE_1_OFFSET 20 /* DA(6) + SA(6)+ PAD(2) +6(aa aa 03 00 00 00) */
#define DOT3HDR_OFFSET  16 /* DA(6) + SA(6) + Type(2) */
#define LLC_HDR_LEN             8
#define PKTBODYOFFSZ            4
#define PKTFETCH_FLAG_AMSDU_SUBMSDUS  0x01 /* AMSDU non-first MSDU"S */


#define LLC_SNAP_HEADER_CHECK(lsh) \
			(lsh->dsap == 0xaa && \
			lsh->ssap == 0xaa && \
			lsh->ctl == 0x03 && \
			lsh->oui[0] == 0 && \
			lsh->oui[1] == 0 && \
			lsh->oui[2] == 0)

#define EAPOL_PKTFETCH_REQUIRED(lsh) \
	(ntoh16(lsh->type) == ETHER_TYPE_802_1X && \
		LLC_SNAP_HEADER_CHECK(lsh))

#ifdef WLNDOE
#define ICMP6_MIN_BODYLEN	(DOT11_LLC_SNAP_HDR_LEN + sizeof(struct ipv6_hdr)) + \
				sizeof(((struct icmp6_hdr *)0)->icmp6_type)
#define ICMP6_NEXTHDR_OFFSET	(sizeof(struct dot11_llc_snap_header) + \
				OFFSETOF(struct ipv6_hdr, nexthdr))
#define ICMP6_TYPE_OFFSET	(sizeof(struct dot11_llc_snap_header) + \
				sizeof(struct ipv6_hdr) + \
				OFFSETOF(struct icmp6_hdr, icmp6_type))

#define NDOE_PKTFETCH_REQUIRED(wlc, lsh, pbody, body_len) \
	(lsh->type == hton16(ETHER_TYPE_IPV6) && \
	LLC_SNAP_HEADER_CHECK(lsh) && \
	body_len >= (((uint8 *)lsh - (uint8 *)pbody) + ICMP6_MIN_BODYLEN) && \
	*((uint8 *)lsh + ICMP6_NEXTHDR_OFFSET) == ICMPV6_HEADER_TYPE && \
	(*((uint8 *)lsh + ICMP6_TYPE_OFFSET) == ICMPV6_PKT_TYPE_NS || \
	*((uint8 *)lsh + ICMP6_TYPE_OFFSET) == ICMPV6_PKT_TYPE_RA) && \
	NDOE_ENAB(wlc->pub))
#endif /* WLNDOE */

#ifdef WLTDLS
#define	WLTDLS_PKTFETCH_REQUIRED(wlc, lsh)	\
	(TDLS_ENAB(wlc->pub) && ntoh16(lsh->type) == ETHER_TYPE_89_0D)
#endif

#ifdef BDO
#define BDO_PKTFETCH_REQUIRED(wlc, lsh, f) \
	((BDO_SUPPORT(wlc->pub) && BDO_ENAB(wlc->pub)) && \
	(lsh->type == hton16(ETHER_TYPE_IP) || lsh->type == hton16(ETHER_TYPE_IPV6)) && \
	LLC_SNAP_HEADER_CHECK(lsh))
#endif /* BDO */

#ifdef TKO
#define TKO_PKTFETCH_REQUIRED(wlc, lsh) \
	(TKO_ENAB(wlc->pub) && wl_tko_is_running(wl_get_tko(wlc->wl, 0)) && \
	(lsh->type == hton16(ETHER_TYPE_IP) || lsh->type == hton16(ETHER_TYPE_IPV6)) && \
	LLC_SNAP_HEADER_CHECK(lsh))
#endif /* TKO */

#ifdef WL_TBOW
#define WLTBOW_PKTFETCH_REQUIRED(wlc, bsscfg, eth_type) \
	(TBOW_ENAB((wlc)->pub) && (bsscfg)->associated && \
	WLC_TBOW_ETHER_TYPE_MATCH(eth_type) && \
	BSSCFG_IS_TBOW_ACTIVE(bsscfg))
#endif


extern int wlc_recvdata_schedule_pktfetch(wlc_info_t *wlc, struct scb *scb,
        wlc_frminfo_t *f, bool promisc_frame, bool ordered, bool amsdu_sub_msdus);
extern bool wlc_pktfetch_required(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct wlc_frminfo *f,
        wlc_key_info_t *key_info, bool skip_iv);

#if defined(PKTC) || defined(PKTC_DONGLE)
extern void wlc_sendup_schedule_pktfetch(wlc_info_t *wlc, void *pkt);
#endif
#endif /* BCMSPLITRX */

#endif	/* _wlc_pktfetch_h_ */
