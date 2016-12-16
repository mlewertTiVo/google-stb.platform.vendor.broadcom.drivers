/*
 *  hostapd - IEEE 802.11r - Fast BSS Transition
 *  Copyright (c) 2004-2009, Jouni Malinen <j@w1.fi>
 *
 *  This software may be distributed under the terms of the BSD license.
 *  See README for more details.
 */
/* $ Copyright... $
 *
 * $Id: wpa_auth_ft.h 667908 2016-10-31 18:41:03Z $
 */

#ifdef WLHOSTFBT

#ifndef WPA_AUTH_FT_H_
#define WPA_AUTH_FT_H_

#include <common.h>

/* FT Action frame type */
#define WLAN_ACTION_FT 6

/* IEEE Std 802.11r-2008, 11A.10.3 - Remote request/response frame definition */
struct ft_rrb_frame {
	uint8 frame_type; /* RSN_REMOTE_FRAME_TYPE_FT_RRB */
	uint8 packet_type; /* FT_PACKET_REQUEST/FT_PACKET_RESPONSE */
	uint16 action_length; /* little endian length of action_frame */
	uint8 ap_address[ETH_ALEN];
	/*
	 * Followed by action_length bytes of FT Action frame (from Category
	 * field to the end of Action Frame body.
	 */
} STRUCT_PACKED;

#define RSN_REMOTE_FRAME_TYPE_FT_RRB 1

#define FT_PACKET_REQUEST 0
#define FT_PACKET_RESPONSE 1
/* Vendor-specific types for R0KH-R1KH protocol; not defined in 802.11r */
#define FT_PACKET_R0KH_R1KH_PULL 200
#define FT_PACKET_R0KH_R1KH_RESP 201
#define FT_PACKET_R0KH_R1KH_PUSH 202

#define FT_R0KH_R1KH_PULL_DATA_LEN 44
#define FT_R0KH_R1KH_RESP_DATA_LEN 76
#define FT_R0KH_R1KH_PUSH_DATA_LEN 88

struct ft_r0kh_r1kh_pull_frame {
	uint8 frame_type; /* RSN_REMOTE_FRAME_TYPE_FT_RRB */
	uint8 packet_type; /* FT_PACKET_R0KH_R1KH_PULL */
	uint16 data_length; /* little endian length of data (44) */
	uint8 ap_address[ETH_ALEN];

	uint8 nonce[16];
	uint8 pmk_r0_name[WPA_PMK_NAME_LEN];
	uint8 r1kh_id[FT_R1KH_ID_LEN];
	uint8 s1kh_id[ETH_ALEN];
	uint8 pad[4]; /* 8-octet boundary for AES key wrap */
	uint8 key_wrap_extra[8];
} STRUCT_PACKED;

struct ft_r0kh_r1kh_resp_frame {
	uint8 frame_type; /* RSN_REMOTE_FRAME_TYPE_FT_RRB */
	uint8 packet_type; /* FT_PACKET_R0KH_R1KH_RESP */
	uint16 data_length; /* little endian length of data (76) */
	uint8 ap_address[ETH_ALEN];

	uint8 nonce[16]; /* copied from pull */
	uint8 r1kh_id[FT_R1KH_ID_LEN]; /* copied from pull */
	uint8 s1kh_id[ETH_ALEN]; /* copied from pull */
	uint8 pmk_r1[PMK_LEN];
	uint8 pmk_r1_name[WPA_PMK_NAME_LEN];
	uint16 pairwise;
	uint8 pad[2]; /* 8-octet boundary for AES key wrap */
	uint8 key_wrap_extra[8];
} STRUCT_PACKED;

struct ft_r0kh_r1kh_push_frame {
	uint8 frame_type; /* RSN_REMOTE_FRAME_TYPE_FT_RRB */
	uint8 packet_type; /* FT_PACKET_R0KH_R1KH_PUSH */
	uint16 data_length; /* little endian length of data (88) */
	uint8 ap_address[ETH_ALEN];

	/* Encrypted with AES key-wrap */
	uint8 timestamp[4]; /* current time in seconds since unix epoch, little endian */
	uint8 r1kh_id[FT_R1KH_ID_LEN];
	uint8 s1kh_id[ETH_ALEN];
	uint8 pmk_r0_name[WPA_PMK_NAME_LEN];
	uint8 pmk_r1[PMK_LEN];
	uint8 pmk_r1_name[WPA_PMK_NAME_LEN];
	uint16 pairwise;
	uint8 pad[6]; /* 8-octet boundary for AES key wrap */
	uint8 key_wrap_extra[8];
} STRUCT_PACKED;

struct ft_remote_r0kh {
	struct ft_remote_r0kh *next;
	uint8 addr[ETH_ALEN];
	uint8 id[FT_R0KH_ID_MAX_LEN];
	size_t id_len;
	uint8 key[KH_KEY_LEN];
} STRUCT_PACKED;

struct ft_remote_r1kh {
	struct ft_remote_r1kh *next;
	uint8 addr[ETH_ALEN];
	uint8 id[FT_R1KH_ID_LEN];
	uint8 key[KH_KEY_LEN];
} STRUCT_PACKED;

struct wpa_ft_pmk_cache * wpa_ft_pmk_cache_init(void);
void wpa_ft_pmk_cache_deinit(struct wpa_ft_pmk_cache *cache);
void wpa_ft_r0kh_r1kh_init(wpa_t *wpa);
void wpa_ft_push_pmk_r1(wpa_t *wpa);
int wpa_auth_derive_ptk_ft(wpa_t *wpa, nas_sta_t *sta,
		const char *pmk, unsigned char *ptk, size_t ptk_len);
int wpa_ft_rrb_rx(wpa_t *wpa, const uint8 *src_addr, const uint8 *dest_addr,
		const uint8 *data, size_t data_len);
int wpa_write_mdie(wpa_t *wpa, nas_sta_t *sta, uint8 *buf, size_t len);
int wpa_write_ftie(wpa_t *wpa, nas_sta_t *sta, const uint8 *r0kh_id,
		size_t r0kh_id_len,
		const uint8 *anonce, const uint8 *snonce,
		uint8 *buf, size_t len, const uint8 *subelem,
		size_t subelem_len);
void wpa_ft_process_auth(wpa_t *wpa, bcm_event_t *dpkt, nas_sta_t *sta);
int setup_rrb_socket(wpa_t *wpa, char *ifname);
int deinit_rrb_socket(nas_t *nas);

#endif /* WPA_AUTH_FT_H_ */
#endif /* WLHOSTFBT */
