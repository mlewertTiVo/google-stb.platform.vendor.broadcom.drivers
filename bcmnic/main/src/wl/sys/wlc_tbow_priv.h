/*
* Tunnel BT Traffic Over Wlan private header file
*
* Copyright (C) 2016, Broadcom Corporation
* All Rights Reserved.
* 
* This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
* the contents of this file may not be disclosed to third parties, copied
* or duplicated in any form, in whole or in part, without the prior
* written permission of Broadcom Corporation.
*
* $Id: wlc_tbow_priv.h 487741 2014-06-26 22:33:48Z $
*
*/
#ifndef _wlc_tbow_priv_h_
#define _wlc_tbow_priv_h_

/* #define GO_LOOP_SCO */
/* #define GC_LOOP_SCO */
/* #define GC_LOOP_USE_RX_LBUF */
/* #define TBOW_FULLLOG */

#ifdef TBOW_FULLLOG
#ifdef WL_ERROR
#undef WL_ERROR
#endif
#define WL_ERROR(x) printf x

#ifdef WL_TRACE
#undef WL_TRACE
#endif
#define WL_TRACE(x) printf x
#endif /* TBOW_FULLLOG */

#define ETHER_TYPE_TBOW 0xC022

#ifdef GO_LOOP_SCO
#define TBOW_ETH_HDR_LEN ROUNDUP(ETHER_HDR_LEN, 4)
#else
#define TBOW_ETH_HDR_LEN ETHER_HDR_LEN
#endif

#define MAX_INTRASNIT_COUNT (tbow_info->wlc->pub->tunables->maxpktcb/2)

#define TBOW_SEQNUM_MAX (1 << 16)
#define TBOW_IS_SEQ_ADVANCED(a, b) (MODSUB_POW2((a), (b), TBOW_SEQNUM_MAX) < (TBOW_SEQNUM_MAX >> 1))

#define TBOW_REORDER_TIMEOUT 200 /* ms */
#define TBOW_STOPMSG_TIMEOUT 500 /* ms */
#define TBOW_LINKDOWN_TIMEOUT 500 /* ms */

typedef enum tbow_ho_msg_type {
	TBOW_HO_MSG_BT_READY = 0x1,
	TBOW_HO_MSG_GO_STA_SETUP = 0x2,
	TBOW_HO_MSG_SETUP_ACK = 0x3,
	TBOW_HO_MSG_START = 0x4,
	TBOW_HO_MSG_WLAN_READY = 0x5,
	TBOW_HO_MSG_S1 = 0x6,
	TBOW_HO_MSG_S2 = 0x7,
	TBOW_HO_MSG_S3 = 0x8,
	TBOW_HO_MSG_STOP = 0x9,
	TBOW_HO_MSG_BT_DROP = 0xa,
	TBOW_HO_MSG_BT_DROP_ACK = 0xb,
	TBOW_HO_MSG_WLAN_DROP = 0xc,
	TBOW_HO_MSG_WLAN_DROP_ACK = 0xd,
	TBOW_HO_MSG_BT_SUSPEND_FAILED = 0xe,
	TBOW_HO_MSG_BT_RESUME_FAILED = 0xf,
	TBOW_HO_MSG_WLAN_RSSI_QUERY = 0x10,
	TBOW_HO_MSG_WLAN_RSSI_RESP = 0x11,
	TBOW_HO_MSG_BT_RESET = 0x12,
	TBOW_HO_MSG_BT_RESET_ACK = 0x13,
	TBOW_HO_MSG_TEAR_WLAN = 0x14,

	TBOW_HO_MSG_INIT_START = 0xfe,
	TBOW_HO_MSG_INIT_STOP = 0xff
} tbow_ho_msg_type_t;

/* put all data types before other type */
typedef enum {
	TBOW_TYPE_DATA_ACL = 0,
	TBOW_TYPE_DATA_SCO,
	TBOW_TYPE_DATA_CNT,
	TBOW_TYPE_CTL = TBOW_TYPE_DATA_CNT,
	TBOW_TYPE_CNT
} tbow_pkt_type_t;

#define TBOW_PKTQ_CNT	1024

typedef struct {
	uint16 tx_seq_num;

	uint16 exp_rx_seq_num;
	uint16 pend_eos_seq;
	bool wait_for_pkt;
} tbow_datachan_state_t;

struct tbow_info {
	/* init one time */
	wlc_info_t* wlc;
	struct wl_timer *ist_timer;
	struct wl_timer *data_acl_timer;
	struct wl_timer *data_sco_timer;
	struct wl_timer *stopmsg_timer;
	struct wl_timer *linkdown_timer;

	/* init every link */
	int bsscfg_idx;
	uchar* msg;
	int msg_len;
	struct pktq send_pq;
	struct pktq recv_pq;
	uint16 intransit_pkt_cnt;
	struct ether_addr local_addr;
	struct ether_addr remote_addr;
	uint8 tx_wlan_ho_stop;
	uint8 rx_wlan_ho_stop;
	uint8 pend_eos_chan;
	bool pull_bt_stop;
	bool tunnel_stop;
	bool push_bt_stop;
	bool ist_timer_set;
	bool isGO;
	bool linkdown_timer_set;
	/* handover manager related info */
	int ho_rate;
	int ho_state;
	struct ether_addr go_BSSID; /* used only for GC */
	tbow_setup_netinfo_t goinfo;

	tbow_datachan_state_t datachan_state[TBOW_TYPE_DATA_CNT];
};

#define TBOW_TAIL_FLAG_MAC_ACK		1
#define TBOW_TAIL_FLAG_STACK_ACK	2
typedef struct {
	uint16 flag; /* txstatus call back */
	uint16 tx_cnt; /* tx times for one packet */
	uint32 timeout_val;
	struct wl_timer *timer;
} tbow_tail_t;

typedef struct {
	uint8 pkt_type;
	uint8 reserved;
	uint16 seq_num; /* tx sequence num for one packet */
	uint32 tx_ts;
#ifdef GC_LOOP_USE_RX_LBUF
	tbow_tail_t tail;
#endif
} tbow_header_t;

typedef struct {
	uint8 msg_type;
	uint8 reserved;
	uint16 last_seq[TBOW_TYPE_DATA_CNT];
} tbow_ho_stop_msg_t;

typedef struct tbow_ho_setupmsg {
	uint8 type;
	uint8 opmode;
	uint8 channel;
	uint8 sender_mac[ETHER_ADDR_LEN];
	uint8 ssid_len;
	uint8 ssid[32];
	uint8 pph_len;
	uint8 pph[63];
} tbow_ho_setupmsg_t;

typedef struct tbow_ho_ack_setupmsg {
	uint8 type;
	struct ether_addr recv_mac;
} tbow_ho_ack_setupmsg_t;

typedef enum tbow_hostate {
	TBOW_HO_IDLE = 0,
	TBOW_HO_START,
	TBOW_HO_FINISH
} tbow_hostate_t;

int tbow_ho_parse_ctrlmsg(tbow_info_t *tbow_info, uchar *msg, int len);
int tbow_ho_stop(tbow_info_t *tbow_info);

void tbow_set_mac(tbow_info_t *tbow_info, struct ether_addr *p_local, struct ether_addr *p_remote);
int tbow_send_bt_msg(tbow_info_t *tbow_info, uchar* msg_buf, int msg_len);
void tbow_send_goinfo(tbow_info_t *tbow_info);
int tbow_start_ho(tbow_info_t *tbow_info);
#endif /* _wlc_tbow_priv_h_ */
