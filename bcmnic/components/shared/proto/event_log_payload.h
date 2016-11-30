/*
 * EVENT_LOG System Definitions
 *
 * This file describes the payloads of event log entries that are data buffers
 * rather than formatted string entries. The contents are generally XTLVs.
 *
 * Copyright (C) 2016, Broadcom. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: event_log_payload.h 641295 2016-06-02 03:29:43Z $
 */

#ifndef _EVENT_LOG_PAYLOAD_H_
#define _EVENT_LOG_PAYLOAD_H_

#include <typedefs.h>
#include <bcmutils.h>
#include <proto/ethernet.h>

#define EVENT_LOG_XTLV_ID_STR                   0  /**< XTLV ID for a string */
#define EVENT_LOG_XTLV_ID_TXQ_SUM               1  /**< XTLV ID for txq_summary_t */
#define EVENT_LOG_XTLV_ID_SCBDATA_SUM           2  /**< XTLV ID for cb_subq_summary_t */
#define EVENT_LOG_XTLV_ID_SCBDATA_AMPDU_TX_SUM  3  /**< XTLV ID for scb_ampdu_tx_summary_t */
#define EVENT_LOG_XTLV_ID_BSSCFGDATA_SUM        4  /**< XTLV ID for bsscfg_q_summary_t */
#define EVENT_LOG_XTLV_ID_UCTXSTATUS            5  /**< XTLV ID for ucode TxStatus array */
#define EVENT_LOG_XTLV_ID_TXQ_SUM_V2            6  /**< XTLV ID for txq_summary_v2_t */

/**
 * An XTLV holding a string
 * String is not null terminated, length is the XTLV len.
 */
typedef struct xtlv_string {
	uint16 id;              /* XTLV ID: EVENT_LOG_XTLV_ID_STR */
	uint16 len;             /* XTLV Len (String length) */
	char   str[1];          /* var len array characters */
} xtlv_string_t;

#define XTLV_STRING_FULL_LEN(str_len)     (BCM_XTLV_HDR_SIZE + (str_len) * sizeof(char))

/**
 * Summary for a single TxQ context
 * Two of these will be used per TxQ context---one for the high TxQ, and one for
 * the low txq that contains DMA prepared pkts. The high TxQ is a full multi-precidence
 * queue and also has a BSSCFG map to identify the BSSCFGS associated with the queue context.
 * The low txq counterpart does not populate the BSSCFG map.
 * The excursion queue will have no bsscfgs associated and is the first queue dumped.
 */
typedef struct txq_summary {
	uint16 id;              /* XTLV ID: EVENT_LOG_XTLV_ID_TXQ_SUM */
	uint16 len;             /* XTLV Len */
	uint32 bsscfg_map;      /* bitmap of bsscfg indexes associated with this queue */
	uint32 stopped;         /* flow control bitmap */
	uint8  prec_count;      /* count of precedences/fifos and len of following array */
	uint8  pad;
	uint16 plen[1];         /* var len array of lengths of each prec/fifo in the queue */
} txq_summary_t;

#define TXQ_SUMMARY_LEN                   (OFFSETOF(txq_summary_t, plen))
#define TXQ_SUMMARY_FULL_LEN(num_q)       (TXQ_SUMMARY_LEN + (num_q) * sizeof(uint16))

typedef struct txq_summary_v2 {
	uint16 id;              /* XTLV ID: EVENT_LOG_XTLV_ID_TXQ_SUM_V2 */
	uint16 len;             /* XTLV Len */
	uint32 bsscfg_map;      /* bitmap of bsscfg indexes associated with this queue */
	uint32 stopped;         /* flow control bitmap */
	uint32 hw_stopped;      /* flow control bitmap */
	uint8  prec_count;      /* count of precedences/fifos and len of following array */
	uint8  pad;
	uint16 plen[1];         /* var len array of lengths of each prec/fifo in the queue */
} txq_summary_v2_t;

#define TXQ_SUMMARY_V2_LEN                (OFFSETOF(txq_summary_v2_t, plen))
#define TXQ_SUMMARY_V2_FULL_LEN(num_q)    (TXQ_SUMMARY_V2_LEN + (num_q) * sizeof(uint16))

/**
 * Summary for tx datapath of an SCB cubby
 * This is a generic summary structure (one size fits all) with
 * a cubby ID and sub-ID to differentiate SCB cubby types and possible sub-queues.
 */
typedef struct scb_subq_summary {
	uint16 id;             /* XTLV ID: EVENT_LOG_XTLV_ID_SCBDATA_SUM */
	uint16 len;            /* XTLV Len */
	uint32 flags;          /* cubby specficic flags */
	uint8  cubby_id;       /* ID registered for cubby */
	uint8  sub_id;         /* sub ID if a cubby has more than one queue */
	uint8  prec_count;     /* count of precedences/fifos and len of following array */
	uint8  pad;
	uint16 plen[1];        /* var len array of lengths of each prec/fifo in the queue */
} scb_subq_summary_t;

#define SCB_SUBQ_SUMMARY_LEN              (OFFSETOF(scb_subq_summary_t, plen))
#define SCB_SUBQ_SUMMARY_FULL_LEN(num_q)  (SCB_SUBQ_SUMMARY_LEN + (num_q) * sizeof(uint16))

/* scb_subq_summary_t.flags for APPS */
#define SCBDATA_APPS_F_PS               0x00000001
#define SCBDATA_APPS_F_PSPEND           0x00000002
#define SCBDATA_APPS_F_INPVB            0x00000004
#define SCBDATA_APPS_F_APSD_USP         0x00000008
#define SCBDATA_APPS_F_TXBLOCK          0x00000010
#define SCBDATA_APPS_F_APSD_HPKT_TMR    0x00000020
#define SCBDATA_APPS_F_APSD_TX_PEND     0x00000040
#define SCBDATA_APPS_F_INTRANS          0x00000080
#define SCBDATA_APPS_F_OFF_PEND         0x00000100
#define SCBDATA_APPS_F_OFF_BLOCKED      0x00000200
#define SCBDATA_APPS_F_OFF_IN_PROG      0x00000400


/**
 * Summary for tx datapath AMPDU SCB cubby
 * This is a specific data structure to describe the AMPDU datapath state for an SCB
 * used instead of scb_subq_summary_t.
 * Info is for one TID, so one will be dumped per BA TID active for an SCB.
 */
typedef struct scb_ampdu_tx_summary {
	uint16 id;              /* XTLV ID: EVENT_LOG_XTLV_ID_SCBDATA_AMPDU_TX_SUM */
	uint16 len;             /* XTLV Len */
	uint32 flags;           /* misc flags */
	uint8  tid;             /* initiator TID (priority) */
	uint8  ba_state;        /* internal BA state */
	uint8  bar_cnt;         /* number of bars sent with no progress */
	uint8  retry_bar;       /* reason code if bar to be retried at watchdog */
	uint16 barpending_seq;  /* seqnum for bar */
	uint16 bar_ackpending_seq; /* seqnum of bar for which ack is pending */
	uint16 start_seq;       /* seqnum of the first unacknowledged packet */
	uint16 max_seq;         /* max unacknowledged seqnum sent */
	uint32 released_bytes_inflight; /* Number of bytes pending in bytes */
	uint32 released_bytes_target;
} scb_ampdu_tx_summary_t;

/* scb_ampdu_tx_summary.flags defs */
#define SCBDATA_AMPDU_TX_F_BAR_ACKPEND          0x00000001 /* bar_ackpending */

/** XTLV stuct to summarize a BSSCFG's packet queue */
typedef struct bsscfg_q_summary {
	uint16 id;               /* XTLV ID: EVENT_LOG_XTLV_ID_BSSCFGDATA_SUM */
	uint16 len;              /* XTLV Len */
	struct ether_addr BSSID; /* BSSID */
	uint8  bsscfg_idx;       /* bsscfg index */
	uint8  type;             /* bsscfg type enumeration: BSSCFG_TYPE_XXX */
	uint8  subtype;          /* bsscfg subtype enumeration: BSSCFG_SUBTYPE_XXX */
	uint8  prec_count;       /* count of precedences/fifos and len of following array */
	uint16 plen[1];          /* var len array of lengths of each prec/fifo in the queue */
} bsscfg_q_summary_t;

#define BSSCFG_Q_SUMMARY_LEN              (OFFSETOF(bsscfg_q_summary_t, plen))
#define BSSCFG_Q_SUMMARY_FULL_LEN(num_q)  (BSSCFG_Q_SUMMARY_LEN + (num_q) * sizeof(uint16))

/**
 * An XTLV holding a TxStats array
 * TxStatus entries are 8 or 16 bytes, size in words (2 or 4) givent in
 * entry_size field.
 * Array is uint32 words
 */
typedef struct xtlv_uc_txs {
	uint16 id;              /* XTLV ID: EVENT_LOG_XTLV_ID_UCTXSTATUS */
	uint16 len;             /* XTLV Len */
	uint8  entry_size;      /* num uint32 words per entry */
	uint8  pad[3];          /* reserved, zero */
	uint32 w[1];            /* var len array of words */
} xtlv_uc_txs_t;

#define XTLV_UCTXSTATUS_LEN                (OFFSETOF(xtlv_uc_txs_t, w))
#define XTLV_UCTXSTATUS_FULL_LEN(words)    (XTLV_UCTXSTATUS_LEN + (words) * sizeof(uint32))

#define SCAN_SUMMARY_VERSION	1
/* Scan flags */
#define SCAN_SUM_CHAN_INFO	0x1
/* Scan_sum flags */
#define BAND5G_SIB_ENAB	0x2
#define BAND2G_SIB_ENAB	0x4
#define PARALLEL_SCAN	0x8
#define SCAN_ABORT	0x10

/* scan_channel_info flags */
#define ACTIVE_SCAN_SCN_SUM	0x2
#define SCAN_SUM_WLC_CORE0	0x4
#define SCAN_SUM_WLC_CORE1	0x8
#define HOME_CHAN	0x10

typedef struct wl_scan_ssid_info
{
	uint8		ssid_len;	/* the length of SSID */
	uint8		ssid[32];	/* SSID string */
} wl_scan_ssid_info_t;

typedef struct wl_scan_channel_info {
	uint16 chanspec;	/* chanspec scanned */
	uint16 reserv;
	uint32 start_time;		/* Scan start time in
				* milliseconds for the chanspec
				* or home_dwell time start
				*/
	uint32 end_time;		/* Scan end time in
				* milliseconds for the chanspec
				* or home_dwell time end
				*/
	uint16 probe_count;	/* No of probes sent out. For future use
				*/
	uint16 scn_res_count;	/* Count of scan_results found per
				* channel. For future use
				*/
} wl_scan_channel_info_t;

typedef struct wl_scan_summary_info {
	uint32 total_chan_num;	/* Total number of channels scanned */
	uint32 scan_start_time;	/* Scan start time in milliseconds */
	uint32 scan_end_time;	/* Scan end time in milliseconds */
	wl_scan_ssid_info_t ssid[1];	/* SSID being scanned in current
				* channel. For future use
				*/
} wl_scan_summary_info_t;

struct wl_scan_summary {
	uint8 version;		/* Version */
	uint8 reserved;
	uint16 len;		/* Length of the data buffer including SSID
				 * list.
				 */
	uint16 sync_id;		/* Scan Sync ID */
	uint16 scan_flags;		/* flags [0] or SCAN_SUM_CHAN_INFO = */
				/* channel_info, if not set */
				/* it is scan_summary_info */
				/* when channel_info is used, */
				/* the following flag bits are overridden: */
				/* flags[1] or ACTIVE_SCAN_SCN_SUM = active channel if set */
				/* passive if not set */
				/* flags[2] or WLC_CORE0 = if set, represents wlc_core0 */
				/* flags[3] or WLC_CORE1 = if set, represents wlc_core1 */
				/* flags[4] or HOME_CHAN = if set, represents home-channel */
				/* flags[5:15] = reserved */
				/* when scan_summary_info is used, */
				/* the following flag bits are used: */
				/* flags[1] or BAND5G_SIB_ENAB = */
				/* allowSIBParallelPassiveScan on 5G band */
				/* flags[2] or BAND2G_SIB_ENAB = */
				/* allowSIBParallelPassiveScan on 2G band */
				/* flags[3] or PARALLEL_SCAN = Parallel scan enabled or not */
				/* flags[4] or SCAN_ABORT = SCAN_ABORTED scenario */
				/* flags[5:15] = reserved */
	union {
		wl_scan_channel_info_t scan_chan_info;	/* scan related information
							* for each channel scanned
							*/
		wl_scan_summary_info_t scan_sum_info;	/* Cumulative scan related
							* information.
							*/
	} u;
};

/* Channel switch log record structure
 * Host may map the following structure on channel switch event log record
 * received from dongle. Note that all payload entries in event log record are
 * uint32/int32.
 * THIS IS HTE SAME AS wl_chansw_event_log (as in src/include/wlioctl.h)!!!
 */
typedef struct wl_chansw_event_log {
	uint32 time;			/* Time in us */
	uint32 old_chanspec;		/* Old channel spec */
	uint32 new_chanspec;		/* New channel spec */
	uint32 chansw_reason;		/* Reason for channel change */
	int32 dwell_time;
} wl_chansw_event_log_t;

/* Sub-block type for EVENT_LOG_TAG_AMPDU_DUMP */
#define WL_AMPDU_STATS_CNT_RXMCSx1	0	/* RX MCS rate (Nss = 1) */
#define WL_AMPDU_STATS_CNT_RXMCSx2	1
#define WL_AMPDU_STATS_CNT_RXMCSx3	2
#define WL_AMPDU_STATS_CNT_RXMCSx4	3
#define WL_AMPDU_STATS_CNT_RXVHTx1	4	/* RX VHT rate (Nss = 1) */
#define WL_AMPDU_STATS_CNT_RXVHTx2	5
#define WL_AMPDU_STATS_CNT_RXVHTx3	6
#define WL_AMPDU_STATS_CNT_RXVHTx4	7
#define WL_AMPDU_STATS_CNT_TXMCSx1	8	/* TX MCS rate (Nss = 1) */
#define WL_AMPDU_STATS_CNT_TXMCSx2	9
#define WL_AMPDU_STATS_CNT_TXMCSx3	10
#define WL_AMPDU_STATS_CNT_TXMCSx4	11
#define WL_AMPDU_STATS_CNT_TXVHTx1	12	/* TX VHT rate (Nss = 1) */
#define WL_AMPDU_STATS_CNT_TXVHTx2	13
#define WL_AMPDU_STATS_CNT_TXVHTx3	14
#define WL_AMPDU_STATS_CNT_TXVHTx4	15
#define WL_AMPDU_STATS_CNT_RXMCSSGI	16	/* RX SGI usage (for all MCS rates) */
#define WL_AMPDU_STATS_CNT_TXMCSSGI	17	/* TX SGI usage (for all MCS rates) */
#define WL_AMPDU_STATS_CNT_RXVHTSGI	18	/* RX SGI usage (for all VHT rates) */
#define WL_AMPDU_STATS_CNT_TXVHTSGI	19	/* TX SGI usage (for all VHT rates) */
#define WL_AMPDU_STATS_CNT_RXMCSPER	20	/* RX PER (for all MCS rates) */
#define WL_AMPDU_STATS_CNT_TXMCSPER	21	/* TX PER (for all MCS rates) */
#define WL_AMPDU_STATS_CNT_RXVHTPER	22	/* RX PER (for all VHT rates) */
#define WL_AMPDU_STATS_CNT_TXVHTPER	23	/* TX PER (for all VHT rates) */
#define WL_AMPDU_STATS_CNT_RXDENS	24	/* RX AMPDU density */
#define WL_AMPDU_STATS_CNT_TXDENS	25	/* TX AMPDU density */
#define WL_AMPDU_STATS_CNT_RXMCSOK	26	/* RX all MCS rates */
#define WL_AMPDU_STATS_CNT_RXVHTOK	27	/* RX all VHT rates */
#define WL_AMPDU_STATS_CNT_TXMCSALL	28	/* TX all MCS rates */
#define WL_AMPDU_STATS_CNT_TXVHTALL	29	/* TX all VHT rates */
#define WL_AMPDU_STATS_CNT_TXMCSOK	30	/* TX all MCS rates */
#define WL_AMPDU_STATS_CNT_TXVHTOK	31	/* TX all VHT rates */

#define WL_AMPDU_STATS_CNT_MAX		64

typedef struct {
	uint16	type;		/* AMPDU statistics sub-type */
	uint16	len;		/* Number of 32-bit counters */
	uint32	counters[WL_AMPDU_STATS_CNT_MAX];
} wl_ampdu_stats_cnt_generic_t;

typedef struct {
	uint16	type;		/* AMPDU statistics sub-type */
	uint16	len;		/* Number of 32-bit counters + 2 */
	uint32	total_ampdu;
	uint32	total_mpdu;
	uint32	aggr_dist[WL_AMPDU_STATS_CNT_MAX + 1];
} wl_ampdu_stats_cnt_aggrsz_t;
#endif /* _EVENT_LOG_PAYLOAD_H_ */
