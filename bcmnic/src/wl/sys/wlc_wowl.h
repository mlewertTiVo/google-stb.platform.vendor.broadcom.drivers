/*
 * Wake-on-Wireless related header file
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
 * $Id: wlc_wowl.h 665388 2016-10-17 21:59:15Z $
*/


#ifndef _wlc_wowl_h_
#define _wlc_wowl_h_

#ifdef WOWL

extern wowl_info_t *wlc_wowl_attach(wlc_info_t *wlc);
extern void wlc_wowl_detach(wowl_info_t *wowl);
extern bool wlc_wowl_cap(struct wlc_info *wlc);
extern bool wlc_wowl_enable(wowl_info_t *wowl);
#ifdef BCMULP
extern bool wlc_wowl_pm2_to_pm1(wowl_info_t *wowl);
#endif /* BCMULP */
extern uint32 wlc_wowl_clear(wowl_info_t *wowl);
extern uint32 wlc_wowl_clear_bmac(wowl_info_t *wowl);
extern int wlc_wowl_wake_reason_process(wowl_info_t *wowl);
#ifdef WOWL_OS_OFFLOADS
extern void wlc_wowl_set_wpa_m1(wowl_info_t *wowl);
extern void wlc_wowl_set_eapol_id(wowl_info_t *wowl);
extern int wlc_wowl_set_key_info(wowl_info_t *wowl, uint32 offload_id, void *kek,
	int kek_len,	void* kck, int kck_len, void *replay_counter, int replay_counter_len);
extern int wlc_wowl_add_offload_ipv4_arp(wowl_info_t *wowl, uint32 offload_id,
	uint8 * RemoteIPv4Address, uint8 *HostIPv4Address, uint8 * MacAddress);
extern int wlc_wowl_add_offload_ipv6_ns(wowl_info_t *wowl, uint32 offload_id,
	uint8 * RemoteIPv6Address, uint8 *SolicitedNodeIPv6Address,
	uint8 * MacAddress, uint8 * TargetIPv6Address1, uint8 * TargetIPv6Address2);
extern int wlc_wowl_remove_offload(wowl_info_t *wowl, uint32 offload_id, uint32 * type);
extern int wlc_wowl_get_replay_counter(wowl_info_t *wowl, void *replay_counter, int *len);
#endif /* WOWL_OS_OFFLOADS */
extern int wlc_wowl_verify_d3_exit(wlc_info_t * wlc);
#else	/* stubs */
#define wlc_wowl_attach(a) (wowl_info_t *)0x0dadbeef
#define	wlc_wowl_detach(a) do {} while (0)
#define wlc_wowl_cap(a) FALSE
#define wlc_wowl_enable(a) FALSE
#define wlc_wowl_clear(b) (0)
#define wlc_wowl_clear_bmac(b) (0)
#define wlc_wowl_wake_reason_process(a) do {} while (0)
#ifdef WOWL_OS_OFFLOADS
#define wlc_wowl_set_wpa_m1(a)
#define wlc_wowl_set_eapol_id(a)
#define wlc_wowl_set_key_info(a, b, c, d, e, f, g, h) (0)
#define wlc_wowl_add_offload_ipv4_arp(a, b, c, d, e) (0)
#define wlc_wowl_add_offload_ipv6_ns(a, b, c, d, e, f, g) (0)
#define wlc_wowl_remove_offload(a, b, c) (0)
#define wlc_wowl_get_replay_counter(a, b, c) (0)
#endif /* WLC_WOWL_OFFLOADS */
#define wlc_wowl_verify_d3_exit(BCME_OK)
#endif /* WOWL */

#define WOWL_IPV4_ARP_TYPE			0
#define WOWL_IPV6_NS_TYPE			1
#define WOWL_DOT11_RSN_REKEY_TYPE	2
#define WOWL_OFFLOAD_INVALID_TYPE	3

#define	WOWL_IPV4_ARP_IDX		0
#define	WOWL_IPV6_NS_0_IDX		1
#define	WOWL_IPV6_NS_1_IDX		2
#define	WOWL_DOT11_RSN_REKEY_IDX	3
#define	WOWL_OFFLOAD_INVALID_IDX	4

#define	MAX_WOWL_OFFLOAD_ROWS		4
#define	MAX_WOWL_IPV6_ARP_PATTERNS	1
#define	MAX_WOWL_IPV6_NS_PATTERNS	2	/* Number of NS offload address patterns allowed */
#define	MAX_WOWL_IPV6_NS_OFFLOADS	1	/* Number of NS offload requests supported */

/* Reserve most	significant bits for internal usage for patterns */
#define	WOWL_INT_RESERVED_MASK      0xFF000000  /* Reserved for internal use */
#define	WOWL_INT_DATA_MASK          0x00FFFFFF  /* Mask for user data  */
#define	WOWL_INT_PATTERN_FLAG       0x80000000  /* OS-generated pattern */
#define	WOWL_INT_NS_TA2_FLAG        0x40000000  /* Pattern from NS TA2 of offload pattern */
#define	WOWL_INT_PATTERN_IDX_MASK   0x0F000000  /* Mask for pattern index in offload table */
#define	WOWL_INT_PATTERN_IDX_SHIFT  24          /* Note: only used for NS patterns */

/* number of WOWL patterns supported */
#define MAXPATTERNS(wlc) \
	(wlc_wowl_cap(wlc) ?									\
	(D11REV_LT((wlc)->pub->corerev, 40) ? 12 : 4)	: 0)

#define WOWL_KEEPALIVE_FIXED_PARAM	11

#endif /* _wlc_wowl_h_ */
