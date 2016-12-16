/*
 * MSDU aggregation protocol source file
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wlc_amsdu.c 663577 2016-10-06 01:25:43Z $
 */


/**
 * C preprocessor flags used within this file:
 * WLAMSDU       : if defined, enables support for AMSDU reception
 * WLAMSDU_TX    : if defined, enables support for AMSDU transmission
 * BCMDBG_AMSDU  : enable AMSDU debug/dump facilities
 * PKTC          : packet chaining support for NIC driver model
 * PKTC_DONGLE   : packet chaining support for dongle firmware
 * WLOVERTHRUSTER: TCP throughput enhancing feature
 * PROP_TXSTATUS : transmit flow control related feature
 * HNDCTF        : Cut Through Forwarding, router specific feature
 * BCM_GMAC3     : Atlas (4709) router chip specific Ethernet interface
 */

#include <wlc_cfg.h>

#if !defined(WLAMSDU) && !defined(WLAMSDU_TX)
#error "Neither WLAMSDU nor WLAMSDU_TX is defined"
#endif

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <proto/802.1d.h>
#include <proto/802.11.h>
#include <wlioctl.h>
#include <bcmwpa.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_phy_hal.h>
#include <wlc_frmutil.h>
#include <wlc_pcb.h>
#include <wlc_scb_ratesel.h>
#include <wlc_rate_sel.h>
#include <wlc_amsdu.h>
#ifdef PROP_TXSTATUS
#include <wlc_wlfc.h>
#endif
#if defined(WLAMPDU) && defined(WLOVERTHRUSTER)
/* headers to enable TCP ACK bypass for Overthruster */
#include <wlc_ampdu.h>
#include <proto/ethernet.h>
#include <proto/bcmip.h>
#include <proto/bcmtcp.h>
#endif /* WLAMPDU && WLOVERTHRUSTER */
#ifdef WL_DATAPATH_LOG_DUMP
#include <event_log.h>
#endif
#ifdef PSTA
#include <wlc_psta.h>
#endif
#ifdef WL11AC
#include <wlc_vht.h>
#endif /* WL11AC */
#include <wlc_ht.h>
#include <wlc_tx.h>
#include <wlc_rx.h>
#include <wlc_bmac.h>
#include <wlc_txmod.h>
#include <wlc_pktc.h>
#include <wlc_dump.h>
#include <wlc_hw.h>
#include <wlc_rsdb.h>

#ifdef BCMSPLITRX
#include <wlc_pktfetch.h>
#endif
#include <d11_cfg.h>
#include <wlc_log.h>

#ifdef WL_TXQ_STALL
#include <wlc_bmac.h>
#include <wl_export.h>
#endif

/*
 * A-MSDU agg flow control
 * if txpend/scb/prio >= WLC_AMSDU_HIGH_WATERMARK, start aggregating
 * if txpend/scb/prio <= WLC_AMSDU_LOW_WATERMARK, stop aggregating
 */
#define WLC_AMSDU_LOW_WATERMARK           1
#define WLC_AMSDU_HIGH_WATERMARK          1

/* default values for tunables, iovars */

#define CCKOFDM_PLCP_MPDU_MAX_LENGTH      8096  /**< max MPDU len: 13 bit len field in PLCP */
#define AMSDU_MAX_MSDU_PKTLEN             VHT_MAX_AMSDU	/**< max pkt length to be aggregated */
#define AMSDU_VHT_USE_HT_AGG_LIMITS_ENAB  1	    /**< use ht agg limits for vht */

#define AMSDU_AGGBYTES_MIN                500   /**< the lowest aggbytes allowed */
#define MAX_TX_SUBFRAMES_LIMIT            16    /**< the highest aggsf allowed */

#define MAX_RX_SUBFRAMES                  100  /**< max A-MSDU rx size/smallest frame bytes */
#define MAX_TX_SUBFRAMES                  14   /**< max num of MSDUs in one A-MSDU */
#define AMSDU_RX_SUBFRAMES_BINS           5    /**< number of counters for amsdu subframes */

#define MAX_TX_SUBFRAMES_ACPHY            2    /**< max num of MSDUs in one A-MSDU */

/* Statistics */

/* Number of length bins in length histogram */
#ifdef WL11AC
#define AMSDU_LENGTH_BINS                 12
#else
#define AMSDU_LENGTH_BINS                 8
#endif

/* Number of bytes in length represented by each bin in length histogram */
#define AMSDU_LENGTH_BIN_BYTES            1024

/* SW RX private states */
#define WLC_AMSDU_DEAGG_IDLE              0    /**< idle */
#define WLC_AMSDU_DEAGG_FIRST             1    /**< deagg first frame received */
#define WLC_AMSDU_DEAGG_LAST              3    /**< deagg last frame received */

/*
 * A-MSDU Header Packet headroom
 *
 * CKIP MIC encapsulation requires extra headroom for the CKIP MIC, CKIP sequence
 * and the difference between ckip_hdr and 802.3 hdr.
 * CCX_HEADROOM set to zero if BCMCCX is not defined.
 */
#define WLC_AMSDU_PKT_HEADROOM (TXOFF + ETHER_ADDR_LEN + CCX_HEADROOM)

/* A-MSDU aggregation passthrough flags */
#define MASK_COMMON (WLF_AMSDU | WLF_MPDU | WLF_8021X)

#define WLC_AMSDU_PASSTHROUGH_FLAGS (MASK_COMMON)

/* Minimum size for AMSDU subframe. In practice this its the TCP ACK */
#ifndef WLC_AMSDU_MIN_SFSIZE
#define WLC_AMSDU_MIN_SFSIZE	TCPACKSZSDU
#endif

/*
 * WLC_AMSDU_AGGDELTA_HIGHWATER is the minimum amount of usable space
 * In practice this its the TCP ACK
 */
#ifndef WLC_AMSDU_AGGDELTA_HIGHWATER
#define WLC_AMSDU_AGGDELTA_HIGHWATER	WLC_AMSDU_MIN_SFSIZE
#endif


#ifdef WLCNT
#define	WLC_AMSDU_CNT_VERSION	2	/**< current version of wlc_amsdu_cnt_t */

/** block ack related stats */
typedef struct wlc_amsdu_cnt {
	/* Transmit aggregation counters */
	uint16 version;               /**< WLC_AMSDU_CNT_VERSION */
	uint16 length;                /**< length of entire structure */

	uint32 agg_openfail;          /**< num of MSDU open failure */
	uint32 agg_passthrough;       /**< num of MSDU pass through w/o A-MSDU agg */
	uint32 agg_block;             /**< num of MSDU blocked in A-MSDU agg */
	uint32 agg_amsdu;             /**< num of A-MSDU released */
	uint32 agg_msdu;              /**< num of MSDU aggregated in A-MSDU */
	uint32 agg_stop_tailroom;     /**< num of MSDU aggs stopped for lack of tailroom */
	uint32 agg_stop_sf;           /**< num of MSDU aggs stopped for sub-frame count limit */
	uint32 agg_stop_len;          /**< num of MSDU aggs stopped for byte length limit */
	uint32 agg_stop_passthrough;  /**< num of MSDU aggs stopped for un-aggregated frame */
	/* TXQ _MUX */
	/* Aggregation stop reasons */
	uint32 agg_stop_qempty;       /**< Stop as input queue has run out of pkts */
	uint32 agg_stop_flags;        /**< Stop agg due to overide bits being set */
	uint32 agg_stop_hostsuppr;    /**< Stop due to PROPTXSTATUS suppress to host */
	uint32 agg_stop_pad_fail;     /**< Failed to get space for padding */
	uint32 agg_stop_headroom;     /**< Num of MSDU aggs stopped for lack of headroom */
	uint32 agg_alloc_newhdr;      /**< Increment when a new packet is used for AMSDU header */
	uint32 agg_alloc_newhdr_fail; /**< Num incremented when new header alloc failed */
	uint32 agg_stop_pktcallback;  /**< Stop due to change in packet callback fn */
	uint32 agg_stop_tid;          /**< Stop as packet TID has changed */
	uint32 agg_stop_ratelimit;    /**< Stop due to ratelimited A-MSDU length */

	/* Aggregation passthrough reasons, incremented during open of the A-MSDU session */
	uint32 passthrough_min_agglen;   /**< Minimum aggregation length not met */
	uint32 passthrough_min_aggsf;    /**< Max sf count alllowed < requested min sf */
	uint32 passthrough_rlimit_bytes; /**< Minimum agg size not met due to rate limits */
	uint32 passthrough_legacy_rate;  /**< Legacy rate supplied, skip aggregation */
	uint32 passthrough_invalid_rate; /**< Invalid rate, skip aggregation */

	/* Misc counters */
	uint32 tx_nprocessed;        /**< Total number of packets processed */
	uint32 tx_singleton;         /**< No agg because of a single packet only */
	uint32 tx_pkt_putback;       /**< Incremented on requeue to the inbound queue */
	uint32 rlimit_max_bytes;     /**< max_bytes is reduced due to specified rate */

	/* NEW_TXQ */
	uint32	agg_stop_lowwm;      /**< num of MSDU aggs stopped for tx low water mark */

	/* Receive Deaggregation counters */
	uint32 deagg_msdu;           /**< MSDU of deagged A-MSDU(in ucode) */
	uint32 deagg_amsdu;          /**< valid A-MSDU deagged(exclude bad A-MSDU) */
	uint32 deagg_badfmt;         /**< MPDU is bad */
	uint32 deagg_wrongseq;       /**< MPDU of one A-MSDU doesn't follow sequence */
	uint32 deagg_badsflen;       /**< MPDU of one A-MSDU has length mismatch */
	uint32 deagg_badsfalign;     /**< MPDU of one A-MSDU is not aligned to 4 byte boundary */
	uint32 deagg_badtotlen;      /**< A-MSDU tot length doesn't match summation of all sfs */
	uint32 deagg_openfail;       /**< A-MSDU deagg open failures */
	uint32 deagg_swdeagglong;    /**< A-MSDU sw_deagg doesn't handle long pkt */
	uint32 deagg_flush;          /**< A-MSDU deagg flush; deagg errors may result in this */
	uint32 tx_pkt_free_ignored;  /**< tx pkt free ignored due to invalid scb or !amsdutx */
	uint32 tx_padding_in_tail;   /**< 4Byte pad was placed in tail of packet */
	uint32 tx_padding_in_head;   /**< 4Byte pad was placed in head of packet */
	uint32 tx_padding_no_pad;    /**< 4Byte pad was not needed (4B aligned or last in agg) */
} wlc_amsdu_cnt_t;
#endif	/* WLCNT */

typedef struct {
	/* tx counters */
	uint32 tx_msdu_histogram[MAX_TX_SUBFRAMES_LIMIT]; /**< mpdus per amsdu histogram */
	uint32 tx_length_histogram[AMSDU_LENGTH_BINS]; /**< amsdu length histogram */
	/* rx counters */
	uint32 rx_msdu_histogram[AMSDU_RX_SUBFRAMES_BINS]; /**< mpdu per amsdu rx */
	uint32 rx_length_histogram[AMSDU_LENGTH_BINS]; /**< amsdu rx length */
} amsdu_dbg_t;

/** iovar table */
enum {
	IOV_AMSDU_SIM,

	IOV_AMSDU_HIWM,       /**< Packet release highwater mark */
	IOV_AMSDU_LOWM,       /**< Packet release lowwater mark */
	IOV_AMSDU_AGGSF,      /**< num of subframes in one A-MSDU for all tids */
	IOV_AMSDU_AGGBYTES,   /**< num of bytes in one A-MSDU for all tids */

	IOV_AMSDU_RXMAX,      /**< get/set HT_CAP_MAX_AMSDU in HT cap field */
	IOV_AMSDU_BLOCK,      /**< block amsdu agg */
	IOV_AMSDU_FLUSH,      /**< flush all amsdu agg queues */
	IOV_AMSDU_DEAGGDUMP,  /**< dump deagg pkt */
	IOV_AMSDU_COUNTERS,   /**< dump A-MSDU counters */
	IOV_AMSDUNOACK,
	IOV_AMSDU,            /**< Enable/disable A-MSDU, GET returns current state */
	IOV_RX_AMSDU_IN_AMPDU
};

/* Policy of allowed AMSDU priority items */
static const bool amsdu_scb_txpolicy[NUMPRIO] = {
	TRUE,  /* 0 BE - Best-effortBest-effort */
	FALSE, /* 1 BK - Background */
	FALSE, /* 2 None = - */
	FALSE, /* 3 EE - Excellent-effort */
	FALSE, /* 4 CL - Controlled Load */
	FALSE, /* 5 VI - Video */
	FALSE, /* 6 VO - Voice */
	FALSE, /* 7 NC - Network Control */
};

static const bcm_iovar_t amsdu_iovars[] = {
	{"amsdu", IOV_AMSDU, (IOVF_SET_DOWN|IOVF_RSDB_SET), 0, IOVT_BOOL, 0},
	{"rx_amsdu_in_ampdu", IOV_RX_AMSDU_IN_AMPDU, (IOVF_RSDB_SET), 0, IOVT_BOOL, 0},
	{"amsdu_noack", IOV_AMSDUNOACK, (IOVF_RSDB_SET), 0, IOVT_BOOL, 0},
	{"amsdu_aggsf", IOV_AMSDU_AGGSF, (IOVF_RSDB_SET), 0, IOVT_UINT16, 0},
	{"amsdu_aggbytes", IOV_AMSDU_AGGBYTES, (0), 0, IOVT_UINT32, 0},
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	{"amsdu_aggblock", IOV_AMSDU_BLOCK, (0), 0, IOVT_BOOL, 0},
#ifdef WLCNT
	{"amsdu_counters", IOV_AMSDU_COUNTERS, (0), 0, IOVT_BUFFER, sizeof(wlc_amsdu_cnt_t)},
#endif /* WLCNT */
#endif /* BCMDBG */
	{NULL, 0, 0, 0, 0, 0}
};

typedef struct amsdu_deagg {
	int amsdu_deagg_state;     /**< A-MSDU deagg statemachine per device */
	void *amsdu_deagg_p;       /**< pointer to first pkt buffer in A-MSDU chain */
	void *amsdu_deagg_ptail;   /**< pointer to last pkt buffer in A-MSDU chain */
	uint16  first_pad;         /**< front padding bytes of A-MSDU first sub frame */
} amsdu_deagg_t;

/** default/global settings for A-MSDU module */
typedef struct amsdu_ami_policy {
	amsdu_txpolicy_union_t u;
	uint amsdu_max_sframes;            /**< Maximum allowed subframes per A-MSDU */
	bool amsdu_agg_enable[NUMPRIO];    /**< TRUE:agg allowed, FALSE:agg disallowed */
	uint amsdu_max_agg_bytes[NUMPRIO]; /**< Maximum allowed payload bytes per A-MSDU */
} amsdu_ami_policy_t;

/** principal amsdu module local structure per device instance */
struct amsdu_info {
	wlc_info_t *wlc;             /**< pointer to main wlc structure */
	wlc_pub_t *pub;              /**< public common code handler */
	int scb_handle;              /**< scb cubby handle to retrieve data from scb */

	/* RX datapath bits */
	uint mac_rcvfifo_limit;    /**< max rx fifo in bytes */
	uint amsdu_rx_mtu;         /**< amsdu MTU, depend on rx fifo limit */
	bool amsdu_rxcap_big;        /**< TRUE: rx big amsdu capable (HT_MAX_AMSDU) */
	/* rx: streams per device */
	amsdu_deagg_t *amsdu_deagg;  /**< A-MSDU deagg */

	/* TX datapath bits */
	bool amsdu_agg_block;        /**< global override: disable amsdu tx */
	amsdu_ami_policy_t txpolicy; /**< Default ami release policy per prio */

	uint8   cubby_name_id;       /**< cubby ID (used by WL_DATAPATH_LOG_DUMP) */
#ifdef WLCNT
	wlc_amsdu_cnt_t *cnt;        /**< counters/stats */
#endif /* WLCNT */
#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(BCMDBG_AMSDU)
	amsdu_dbg_t *amdbg;
#endif
};

/* Per-prio SCB A-MSDU policy variables, contains a public and a private part */
typedef struct {
	/* This info is public */
	amsdu_txpolicy_t pub;          /**<public A-MSDU tx policy items */

	/*
	 * This are the private limits as established by the peer negotiation.
	 * The A-MSDU code may over-ride this in the case of TXQ_MUX
	 */
	uint amsdu_ht_agg_bytes_max;  /**< max ht AMSDU bytes negotiated */
	uint amsdu_vht_agg_bytes_max; /**< max vht AMSDU bytes negotiated */
} amsdu_scb_txpolicy_t;

/* Local private A-MSDU aggregation state variables */
#ifdef TXQ_MUX
typedef struct {
	amsdu_info_t *ami;         /**< A-MSDU module info handle */
	uint amsdu_agg_bytes;      /**< A-MSDU byte count */
	uint amsdu_agg_sframes;    /**< A-MSDU max allowed subframe count */
	uint amsdu_max_bytes;      /**< A-MSDU max allowed byte count */
	uint amsdu_max_sframes;    /**< A-MSDU subframe count */
	void *amsdu_head;          /**< A-MSDU pkt pointer to first MSDU */
	void *lastpkt_lastbuf;     /**< A-MSDU pkt pointer to last buffer of last MSDU */
	void *amsdu_hdr;           /**< A-MSDU header allocated as a separate packet */
	uint amsdu_tid;            /**< TID of amsdu aggregate */
	uint hdr_headroom;         /**< amount of allocated header HEADROOM */
	uint32 passthrough_flags;  /**< Cached A-MSDU passthrough flags */
	uint32 lastpkt_len;        /**< Length of last packet in chain */
	uint32 aggbytes_hwm;       /**< high watermark for agg len */
	bool ackRatioEnabled;      /**< Set if TCP ACK ratio feature is enabled */
	bool proptxstatus_suppr;   /**< Set if PROPTXSTATUS suppression handling is reqd */
	uint16 seq_suppr_state;    /**< PCIeFD seqno suppression state */
	uint8 callbackidx;         /**< Packet callback index for this A-MPDU */
	uint8 pcb_data;            /**< Packet class callback data */
	uint8 pcb_offset;          /**< pkt class callback offset within pkttag */
	osl_t *osh;                /**< osl handle for packet processing */
	uint timestamp;            /**< Timestamp of the first queued packet */
#ifdef WLCNT
	uint32 *stop_counter;     /**< stop or ratelimit counter */
	wlc_amsdu_cnt_t *cnt;	  /**< counters */
#endif
} amsdu_txaggstate_t;
#else
typedef struct {
	uint amsdu_agg_bytes;      /**< A-MSDU byte count */
	uint amsdu_agg_sframes;    /**< A-MSDU max allowed subframe count */
	void *amsdu_agg_p;         /**< A-MSDU pkt pointer to first MSDU */
	void *amsdu_agg_ptail;     /**< A-MSDU pkt pointer to last MSDU */
	uint amsdu_agg_padlast;    /**< pad bytes in the agg tail buffer */
	uint8 headroom_pad_need;   /**< # of bytes (0-3) need fr headroom for pad prev pkt */
	/** # of transmit AMSDUs forwarded to the next software layer but not yet returned */
	uint amsdu_agg_txpending;
	uint timestamp;            /**< Timestamp of the first queued packet */
} amsdu_txaggstate_t;
#endif /* TXQ_MUX */

/** per scb cubby info */
typedef struct scb_amsduinfo {
	/* Initialized when A-MSDU module for the SCB is plumbed */
	amsdu_info_t *ami;         /* A-MSDU module handle */
	struct scb *scb;           /* SCB of the A-MSDU-- optimization */

	/* NEW_TXQ buffers the frames in A-MSDU aggregate, this is the state metadata */
	amsdu_txaggstate_t aggstate[NUMPRIO];

	/*
	 * Default policy items in TXQ_MUX, STATE + POLICY items in NEW_TXQ
	 * This contains the default attach time values as well as any
	 * protocol dependent limits.
	 */
	amsdu_scb_txpolicy_t scb_txpolicy[NUMPRIO];
} scb_amsdu_t;

#define SCB_AMSDU_CUBBY(ami, scb) (scb_amsdu_t *)SCB_CUBBY((scb), (ami)->scb_handle)

/* A-MSDU general */
static int wlc_amsdu_doiovar(void *hdl, uint32 actionid,
        void *p, uint plen, void *arg, uint alen, uint val_size, struct wlc_if *wlcif);

static void wlc_amsdu_mtu_init(amsdu_info_t *ami);
static int wlc_amsdu_down(void *hdl);
static int wlc_amsdu_up(void *hdl);

#ifdef WLAMSDU_TX

#ifndef WLAMSDU_TX_DISABLED
static int wlc_amsdu_scb_update(void *context, struct scb *scb, wlc_bsscfg_t* new_cfg);
static int wlc_amsdu_scb_init(void *cubby, struct scb *scb);
static void wlc_amsdu_scb_deinit(void *cubby, struct scb *scb);
#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(BCMDBG_AMSDU)
static void wlc_amsdu_dump_scb(void *ctx, struct scb *scb, struct bcmstrbuf *b);
#else
#define wlc_amsdu_dump_scb NULL
#endif

#if defined(WL_DATAPATH_LOG_DUMP) && !defined(TXQ_MUX)
static void wlc_amsdu_datapath_summary(void *ctx, struct scb *scb, int tag);
#endif /* WL_DATAPATH_LOG_DUMP && !TXQ_MUX */
#endif /* WLAMSDU_TX_DISABLED */

/* A-MSDU aggregation */
#ifdef TXQ_MUX
static bool wlc_amsdu_can_agg(amsdu_txaggstate_t *amsdupolicy, void *p);
static void* wlc_amsdu_agg_open(amsdu_txinfo_t *txinfo, scb_amsdu_t *scb_ami,
	amsdu_txaggstate_t *amsdustate, void *p);
#else
static void  wlc_amsdu_agg(void *ctx, struct scb *scb, void *p, uint prec);
static void* wlc_amsdu_agg_open(amsdu_info_t *ami, wlc_bsscfg_t *bsscfg,
	struct ether_addr *ea, void *p);
static bool  wlc_amsdu_agg_append(amsdu_info_t *ami, struct scb *scb, void *p,
	uint tid, ratespec_t rspec);
static void  wlc_amsdu_agg_close(amsdu_info_t *ami, struct scb *scb, uint tid);
#endif /* TXQ_MUX */

#if defined(WLOVERTHRUSTER)
static bool wlc_amsdu_is_tcp_ack(amsdu_info_t *ami, void *p);
#endif

#ifndef TXQ_MUX
static void  wlc_amsdu_scb_deactive(void *ctx, struct scb *scb);
static uint  wlc_amsdu_txpktcnt(void *ctx);

static txmod_fns_t BCMATTACHDATA(amsdu_txmod_fns) = {
	wlc_amsdu_agg,
	wlc_amsdu_txpktcnt,
	wlc_amsdu_scb_deactive,
	NULL
};
#endif /* !TXQ_MUX */
#endif /* WLAMSDU_TX */

#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(BCMDBG_AMSDU)
static int wlc_amsdu_dump(void *ctx, struct bcmstrbuf *b);
#endif

/* A-MSDU deaggregation */
static bool wlc_amsdu_deagg_open(amsdu_info_t *ami, int fifo, void *p,
	struct dot11_header *h, uint32 pktlen);
static bool wlc_amsdu_deagg_verify(amsdu_info_t *ami, uint16 fc, void *h);
static void wlc_amsdu_deagg_flush(amsdu_info_t *ami, int fifo);
static int wlc_amsdu_tx_attach(amsdu_info_t *ami, wlc_info_t *wlc);

#if (defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(BCMDBG_AMSDU)) && \
	defined(WLCNT)
void wlc_amsdu_dump_cnt(amsdu_info_t *ami, struct bcmstrbuf *b);
#endif	/* defined(BCMDBG) && defined(WLCNT) */

#ifdef WLAMSDU_TX

static void
wlc_amsdu_set_scb_default_txpolicy(amsdu_info_t *ami, scb_amsdu_t *scb_ami)
{
	uint i;

	for (i = 0; i < NUMPRIO; i++) {
		amsdu_txpolicy_t *scb_txpolicy = &scb_ami->scb_txpolicy[i].pub;
#ifdef TXQ_MUX
		scb_txpolicy->aggbytes_hwm_delta = ami->txpolicy.aggbytes_hwm_delta;
		scb_txpolicy->tx_ackRatioEnabled = ami->txpolicy.tx_ackRatioEnabled;
		scb_txpolicy->tx_passthrough_flags = ami->txpolicy.tx_passthrough_flags;
		scb_txpolicy->packet_headroom = ami->txpolicy.packet_headroom;
#else
		scb_txpolicy->fifo_lowm = ami->txpolicy.fifo_lowm;
		scb_txpolicy->fifo_hiwm = ami->txpolicy.fifo_hiwm;
#endif /* TXQ_MUX */
		scb_txpolicy->amsdu_agg_enable = ami->txpolicy.amsdu_agg_enable[i];
		scb_txpolicy->amsdu_max_sframes = ami->txpolicy.amsdu_max_sframes;
		scb_txpolicy->amsdu_max_agg_bytes = ami->txpolicy.amsdu_max_agg_bytes[i];
	}
}

static void
wlc_amsdu_set_scb_default_txpolicy_all(amsdu_info_t *ami)
{
	struct scb *scb;
	struct scb_iter scbiter;

	FOREACHSCB(ami->wlc->scbstate, &scbiter, scb) {
		wlc_amsdu_set_scb_default_txpolicy(ami, SCB_AMSDU_CUBBY(ami, scb));
	}

	/* Update frag threshold on A-MSDU parameter changes */
	wlc_amsdu_agglimit_frag_upd(ami);
}

#ifdef TXQ_MUX
static void

/* Retrieve the default configured scb txpolicy for the specified priority */
wlc_amsdu_get_scb_cfg_txpolicy(scb_amsdu_t *scb_ami, uint prio,
	amsdu_scb_txpolicy_t *txpolicy_out)
{
	ASSERT(prio < NUMPRIO);
	ASSERT(txpolicy_out);
	memcpy(txpolicy_out, &scb_ami->scb_txpolicy[prio], sizeof(*txpolicy_out));
}

#else
static void wlc_amsdu_dotxstatus(amsdu_info_t *ami, struct scb *scb, void *pkt);

/** WLAMSDU_TX specific function, handles callbacks when pkts either tx-ed or freed */
static void BCMFASTPATH
wlc_amsdu_pkt_freed(wlc_info_t *wlc, void *pkt, uint txs)
{
	BCM_REFERENCE(txs);

	if (AMSDU_TX_ENAB(wlc->pub)) {
		int err;
		struct scb *scb = NULL;
		wlc_bsscfg_t *bsscfg = wlc_bsscfg_find(wlc, WLPKTTAGBSSCFGGET(pkt), &err);
		/* if bsscfg or scb are stale or bad, then ignore this pkt for acctg purposes */
		if (!err && bsscfg) {
			scb = WLPKTTAGSCBGET(pkt);
			if (scb && SCB_AMSDU(scb)) {
				wlc_amsdu_dotxstatus(wlc->ami, scb, pkt);
			} else {
				WL_AMSDU(("wl%d:%s: not count scb(%p) pkts\n",
					wlc->pub->unit, __FUNCTION__, OSL_OBFUSCATE_BUF(scb)));
#ifdef WLCNT
				WLCNTINCR(wlc->ami->cnt->tx_pkt_free_ignored);
#endif /* WLCNT */
			}
		} else {
			WL_AMSDU(("wl%d:%s: not count bsscfg (%p) pkts\n",
				wlc->pub->unit, __FUNCTION__, OSL_OBFUSCATE_BUF(bsscfg)));
#ifdef WLCNT
			WLCNTINCR(wlc->ami->cnt->tx_pkt_free_ignored);
#endif /* WLCNT */
		}
	}
	/* callback is cleared by default by calling function */
}
#endif /* !TXQ_MUX */
#endif /* WLAMSDU_TX */

#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(BCMDBG_AMSDU)
#ifdef WLCNT
static int
wlc_amsdu_dump_clr(void *ctx)
{
	amsdu_info_t *ami = ctx;

	bzero(ami->cnt, sizeof(*ami->cnt));
	bzero(ami->amdbg, sizeof(*ami->amdbg));

	return BCME_OK;
}
#else
#define wlc_amsdu_dump_clr NULL
#endif
#endif /* BCMDBG || BCMDBG_DUMP || BCMDBG_AMSDU */

/*
 * This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

/** attach function for receive AMSDU support */
amsdu_info_t *
BCMATTACHFN(wlc_amsdu_attach)(wlc_info_t *wlc)
{
	amsdu_info_t *ami;
	if (!(ami = (amsdu_info_t *)MALLOCZ(wlc->osh, sizeof(amsdu_info_t)))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}
	ami->wlc = wlc;
	ami->pub = wlc->pub;

	if (!(ami->amsdu_deagg = (amsdu_deagg_t *)MALLOCZ(wlc->osh,
		sizeof(amsdu_deagg_t) * RX_FIFO_NUMBER))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

#ifdef WLCNT
	if (!(ami->cnt = (wlc_amsdu_cnt_t *)MALLOCZ(wlc->osh, sizeof(wlc_amsdu_cnt_t)))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
#endif /* WLCNT */

	/* register module */
	if (wlc_module_register(ami->pub, amsdu_iovars, "amsdu", ami, wlc_amsdu_doiovar,
		NULL, wlc_amsdu_up, wlc_amsdu_down)) {
		WL_ERROR(("wl%d: %s: wlc_module_register failed\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(BCMDBG_AMSDU)
	if (!(ami->amdbg = (amsdu_dbg_t *)MALLOCZ(wlc->osh, sizeof(amsdu_dbg_t)))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	wlc_dump_add_fns(ami->pub, "amsdu", wlc_amsdu_dump, wlc_amsdu_dump_clr, ami);
#endif

#ifdef TXQ_MUX
	ami->txpolicy.packet_headroom = (uint16)WLC_AMSDU_PKT_HEADROOM;
	ami->txpolicy.tx_passthrough_flags = WLC_AMSDU_PASSTHROUGH_FLAGS;
	ami->txpolicy.aggbytes_hwm_delta = WLC_AMSDU_AGGDELTA_HIGHWATER;
#else
	ami->txpolicy.fifo_lowm = (uint16)WLC_AMSDU_LOW_WATERMARK;
	ami->txpolicy.fifo_hiwm = (uint16)WLC_AMSDU_HIGH_WATERMARK;
#endif

	if (wlc_amsdu_tx_attach(ami, wlc) < 0) {
		WL_ERROR(("wl%d: %s: Error initing the amsdu tx\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	wlc_amsdu_mtu_init(ami);

	/* to be compatible with spec limit */
	if (wlc->pub->tunables->nrxd < MAX_RX_SUBFRAMES) {
		WL_ERROR(("NRXD %d is too small to fit max amsdu rxframe\n",
		          (uint)wlc->pub->tunables->nrxd));
	}
#ifndef WLAMSDU_2G_DISABLED
	 wlc->pub->_amsdu_2g = TRUE;
#endif /* WLAMSDU 2G */
	return ami;
fail:
	MODULE_DETACH(ami, wlc_amsdu_detach);
	return NULL;
} /* wlc_amsdu_attach */

/** attach function for transmit AMSDU support */
static int
BCMATTACHFN(wlc_amsdu_tx_attach)(amsdu_info_t *ami, wlc_info_t *wlc)
{
#ifdef WLAMSDU_TX
#ifndef WLAMSDU_TX_DISABLED
	uint i;
	uint max_agg;
	scb_cubby_params_t cubby_params;
#ifndef TXQ_MUX
	int err;
#endif /* !TXQ_MUX */

	if (WLCISACPHY(wlc->band)) {
#if AMSDU_VHT_USE_HT_AGG_LIMITS_ENAB
		max_agg = HT_MIN_AMSDU;
#else
		max_agg = VHT_MAX_AMSDU;
#endif /* AMSDU_VHT_USE_HT_AGG_LIMITS_ENAB */
	} else {
		max_agg = HT_MAX_AMSDU;
	}

#ifndef TXQ_MUX
	/* register packet class callback */
	if ((err = wlc_pcb_fn_set(wlc->pcb, 2, WLF2_PCB3_AMSDU, wlc_amsdu_pkt_freed)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_pcb_fn_set err=%d\n", wlc->pub->unit, __FUNCTION__, err));
		return -1;
	}
#endif

#if defined(WL_DATAPATH_LOG_DUMP) && !defined(TXQ_MUX)
	ami->cubby_name_id = wlc_scb_cubby_name_register(wlc->scbstate, "AMSDU");
	if (ami->cubby_name_id == WLC_SCB_NAME_ID_INVALID) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_name_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		return -10;
	}
#endif /* WL_DATAPATH_LOG_DUMP && !TXQ_MUX */

	/* reserve cubby in the scb container for per-scb private data */
	bzero(&cubby_params, sizeof(cubby_params));

	cubby_params.context = ami;
	cubby_params.fn_init = wlc_amsdu_scb_init;
	cubby_params.fn_deinit = wlc_amsdu_scb_deinit;
#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(BCMDBG_AMSDU)
	cubby_params.fn_dump = wlc_amsdu_dump_scb;
#endif
#if defined(WL_DATAPATH_LOG_DUMP) && !defined(TXQ_MUX)
	cubby_params.fn_data_log_dump = wlc_amsdu_datapath_summary;
#endif
	cubby_params.fn_update = wlc_amsdu_scb_update;

	ami->scb_handle = wlc_scb_cubby_reserve_ext(wlc, sizeof(scb_amsdu_t), &cubby_params);

	if (ami->scb_handle < 0) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve failed\n",
		          wlc->pub->unit, __FUNCTION__));
		return -1;
	}

#ifndef TXQ_MUX
	/* register txmod call back */
	wlc_txmod_fn_register(wlc->txmodi, TXMOD_AMSDU, ami, amsdu_txmod_fns);
#endif

	WLCNTSET(ami->cnt->version, WLC_AMSDU_CNT_VERSION);
	WLCNTSET(ami->cnt->length, sizeof(*(ami->cnt)));

	/* init tunables */

	if (WLCISACPHY(wlc->band)) {
		ami->txpolicy.amsdu_max_sframes = MAX_TX_SUBFRAMES_ACPHY;
	} else {
		ami->txpolicy.amsdu_max_sframes = MAX_TX_SUBFRAMES;
	}

	/* DMA: leave empty room for DMA descriptor table */
	if (ami->txpolicy.amsdu_max_sframes >
		(uint)(wlc->pub->tunables->ntxd/3)) {
		WL_ERROR(("NTXD %d is too small to fit max amsdu txframe\n",
			(uint)wlc->pub->tunables->ntxd));
		ASSERT(0);
	}

	for (i = 0; i < NUMPRIO; i++) {
		uint fifo_size;

		ami->txpolicy.amsdu_agg_enable[i] = amsdu_scb_txpolicy[i];

		/* set agg_bytes_limit to standard maximum if hw fifo allows
		 *  this value can be changed via iovar or fragthreshold later
		 *  it can never exceed hw fifo limit since A-MSDU is not streaming
		 */
		fifo_size = wlc->xmtfifo_szh[prio2fifo[i]];
		/* blocks to bytes */
		fifo_size = fifo_size * TXFIFO_SIZE_UNIT(wlc->pub->corerev);
		ami->txpolicy.amsdu_max_agg_bytes[i] = MIN(max_agg, fifo_size);
		/* TODO: PIO */
	}

	wlc->pub->_amsdu_tx_support = TRUE;
#else
#ifndef TXQ_MUX
	/* Reference this to prevent Coverity erors if TXQ_MUX is not used */
	BCM_REFERENCE(amsdu_txmod_fns);
#endif /* !TXQ_MUX */
#endif /* WLAMSDU_TX_DISABLED */
#endif /* WLAMSDU_TX */

	return 0;
} /* wlc_amsdu_tx_attach */

void
BCMATTACHFN(wlc_amsdu_detach)(amsdu_info_t *ami)
{
	if (!ami)
		return;

	wlc_amsdu_down(ami);

	wlc_module_unregister(ami->pub, "amsdu", ami);

#ifdef BCMDBG
	if (ami->amdbg) {
		MFREE(ami->pub->osh, ami->amdbg, sizeof(amsdu_dbg_t));
		ami->amdbg = NULL;
	}
#endif

#ifdef WLCNT
	if (ami->cnt)
		MFREE(ami->pub->osh, ami->cnt, sizeof(wlc_amsdu_cnt_t));
#endif /* WLCNT */
	MFREE(ami->pub->osh, ami->amsdu_deagg, sizeof(amsdu_deagg_t) * RX_FIFO_NUMBER);
	MFREE(ami->pub->osh, ami, sizeof(amsdu_info_t));
}

static uint
wlc_rcvfifo_limit_get(wlc_info_t *wlc)
{
	uint rcvfifo;

	/* determine rx fifo. no register/shm, it's hardwired in RTL */

	if (D11REV_GE(wlc->pub->corerev, 40)) {
		rcvfifo = wlc_bmac_rxfifosz_get(wlc->hw);
	} else if (D11REV_GE(wlc->pub->corerev, 22)) {
		rcvfifo = ((wlc->machwcap & MCAP_RXFSZ_MASK) >> MCAP_RXFSZ_SHIFT) * 512;
	} else {
		/* EOL'd chips or chips that don't exist yet */
		ASSERT(0);
		return 0;
	}
	return rcvfifo;
} /* wlc_rcvfifo_limit_get */

static void
BCMATTACHFN(wlc_amsdu_mtu_init)(amsdu_info_t *ami)
{
	ami->mac_rcvfifo_limit = wlc_rcvfifo_limit_get(ami->wlc);
	if (D11REV_GE(ami->wlc->pub->corerev, 31) && D11REV_LE(ami->wlc->pub->corerev, 38))
		ami->amsdu_rxcap_big = FALSE;
	else
		ami->amsdu_rxcap_big =
		        ((int)(ami->mac_rcvfifo_limit - ami->wlc->hwrxoff - 100) >= HT_MAX_AMSDU);

	ami->amsdu_rx_mtu = ami->amsdu_rxcap_big ? HT_MAX_AMSDU : HT_MIN_AMSDU;

	/* For A/C enabled chips only */
	if (WLCISACPHY(ami->wlc->band) &&
	    ami->amsdu_rxcap_big &&
	    ((int)(ami->mac_rcvfifo_limit - ami->wlc->hwrxoff - 100) >= VHT_MAX_AMSDU)) {
		ami->amsdu_rx_mtu = VHT_MAX_AMSDU;
	}
	WL_AMSDU(("%s:ami->amsdu_rx_mtu=%d\n", __FUNCTION__, ami->amsdu_rx_mtu));
}

bool
wlc_amsdu_is_rxmax_valid(amsdu_info_t *ami)
{
	if (wlc_amsdu_mtu_get(ami) < HT_MAX_AMSDU) {
		return TRUE;
	} else {
		return FALSE;
	}
}

uint
wlc_amsdu_mtu_get(amsdu_info_t *ami)
{
	return ami->amsdu_rx_mtu;
}

/** Returns TRUE or FALSE. AMSDU tx is optional, sw can turn it on or off even HW supports */
bool
wlc_amsdutx_cap(amsdu_info_t *ami)
{
#if defined(WL11N) && defined(WLAMSDU_TX)
	if (AMSDU_TX_SUPPORT(ami->pub) && ami->pub->phy_11ncapable)
		return (TRUE);
#endif
	return (FALSE);
}

/** Returns TRUE or FALSE. AMSDU rx is mandatory for NPHY */
bool
wlc_amsdurx_cap(amsdu_info_t *ami)
{
#ifdef WL11N
	if (ami->pub->phy_11ncapable)
		return (TRUE);
#endif

	return (FALSE);
}

#ifdef WLAMSDU_TX

/** WLAMSDU_TX specific function to enable/disable AMSDU transmit */
int
wlc_amsdu_set(amsdu_info_t *ami, bool on)
{
	wlc_info_t *wlc = ami->wlc;

	WL_AMSDU(("wlc_amsdu_set val=%d\n", on));

	if (on) {
		if (!N_ENAB(wlc->pub)) {
			WL_AMSDU(("wl%d: driver not nmode enabled\n", wlc->pub->unit));
			return BCME_UNSUPPORTED;
		}
		if (!wlc_amsdutx_cap(ami)) {
			WL_AMSDU(("wl%d: device not amsdu capable\n", wlc->pub->unit));
			return BCME_UNSUPPORTED;
		} else if (AMPDU_ENAB(wlc->pub) &&
		           D11REV_LT(wlc->pub->corerev, 40)) {
			/* AMSDU + AMPDU ok for core-rev 40+ with AQM */
			WL_AMSDU(("wl%d: A-MSDU not supported with AMPDU on d11 rev %d\n",
			          wlc->pub->unit, wlc->pub->corerev));
			return BCME_UNSUPPORTED;
		}
	}

	/* This controls AMSDU agg only, AMSDU deagg is on by default per spec */
	wlc->pub->_amsdu_tx = on;
	wlc_update_brcm_ie(ami->wlc);

	/* tx descriptors should be higher -- AMPDU max when both AMSDU and AMPDU set */
	wlc_set_default_txmaxpkts(wlc);

	if (!wlc->pub->_amsdu_tx) {
		wlc_amsdu_agg_flush(ami);
	}

	return (0);
} /* wlc_amsdu_set */

#ifndef WLAMSDU_TX_DISABLED
static int
wlc_amsdu_scb_update(void *context, struct scb *scb, wlc_bsscfg_t* new_cfg)
{
#ifdef TXQ_MUX
	amsdu_info_t *ami = (amsdu_info_t *)context;
	wlc_info_t *new_wlc = new_cfg->wlc;
	scb_amsdu_t *scb_ami = SCB_AMSDU_CUBBY(ami, scb);

	scb_ami->ami = new_wlc->ami;
#endif
	return BCME_OK;
}

/** WLAMSDU_TX specific function, initializes each priority for a remote party (scb) */
static int
wlc_amsdu_scb_init(void *context, struct scb *scb)
{
	uint i;
	amsdu_info_t *ami = (amsdu_info_t *)context;
	scb_amsdu_t *scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	amsdu_scb_txpolicy_t *scb_txpolicy;

	WL_AMSDU(("wlc_amsdu_scb_init scb %p\n", OSL_OBFUSCATE_BUF(scb)));

	ASSERT(scb_ami);

#ifdef TXQ_MUX
	/* Module handle and scb info */
	scb_ami->ami = ami;
	scb_ami->scb = scb;
#endif
	/* Setup A-MSDU SCB policy defaults */
	for (i = 0; i < NUMPRIO; i++) {
		scb_txpolicy = &scb_ami->scb_txpolicy[i];

		memset(scb_txpolicy, 0, sizeof(*scb_txpolicy));

		/*
		 * These are negotiated defaults, this is not a public block
		 * The public info is rebuilt each time an iovar is used which
		 * would destroy the negotiated values here, so this is made private.
		 */
		scb_txpolicy->amsdu_vht_agg_bytes_max = AMSDU_MAX_MSDU_PKTLEN;
		scb_txpolicy->amsdu_ht_agg_bytes_max = HT_MAX_AMSDU;
	}

	/* Init the public part with the global ami defaults */
	wlc_amsdu_set_scb_default_txpolicy(ami, scb_ami);

	return 0;
}

/** WLAMSDU_TX specific function */
static void
wlc_amsdu_scb_deinit(void *context, struct scb *scb)
{
#ifdef TXQ_MUX
	/* The function stub should be retained for any future additions */
	return;
#else
	uint i;
	amsdu_info_t *ami = (amsdu_info_t *)context;
	scb_amsdu_t *scb_amsdu = SCB_AMSDU_CUBBY(ami, scb);

	WL_AMSDU(("wlc_amsdu_scb_deinit scb %p\n", OSL_OBFUSCATE_BUF(scb)));

	ASSERT(scb_amsdu);

	/* release tx agg pkts */
	for (i = 0; i < NUMPRIO; i++) {
		if (scb_amsdu->aggstate[i].amsdu_agg_p) {
			PKTFREE(ami->wlc->osh, scb_amsdu->aggstate[i].amsdu_agg_p, TRUE);
			/* needs clearing, so subsequent access to this cubby doesn't ASSERT */
			/* and/or access bad memory */
			scb_amsdu->aggstate[i].amsdu_agg_p = NULL;
		}
	}
#endif /* TXQ_MUX */
}

#endif /* !WLAMSDU_TX_DISABLED */
#endif /* WLAMSDU_TX */

/** handle AMSDU related items when going down */
static int
wlc_amsdu_down(void *hdl)
{
	int fifo;
	amsdu_info_t *ami = (amsdu_info_t *)hdl;

	WL_AMSDU(("wlc_amsdu_down: entered\n"));

	/* Flush the deagg Q, there may be packets there */
	for (fifo = 0; fifo < RX_FIFO_NUMBER; fifo++)
		wlc_amsdu_deagg_flush(ami, fifo);

	return 0;
}

static int
wlc_amsdu_up(void *hdl)
{
	/* limit max size pkt ucode lets through to what we use for dma rx descriptors */
	/* else rx of amsdu can cause dma rx errors and potentially impact performance */
	wlc_info_t *wlc = ((amsdu_info_t *)hdl)->wlc;
	int rxbufsz = wlc->pub->tunables->rxbufsz - wlc->hwrxoff;

	/* ensure tunable is a valid value which fits in a uint16 */
	ASSERT(rxbufsz > 0 && rxbufsz <= 0xffff);

	wlc_write_shm(wlc, M_MAXRXFRM_LEN(wlc), (uint16)rxbufsz);
	if (D11REV_GE(wlc->pub->corerev, 64)) {
		W_REG(wlc->osh, &wlc->regs->u.d11acregs.DAGG_LEN_THR, (uint16)rxbufsz);
	}

	return BCME_OK;
}

/** handle AMSDU related iovars */
static int
wlc_amsdu_doiovar(void *hdl, uint32 actionid,
	void *p, uint plen, void *a, uint alen, uint val_size, struct wlc_if *wlcif)
{
	amsdu_info_t *ami = (amsdu_info_t *)hdl;
	int32 int_val = 0;
	int32 *pval = (int32 *)a;
	bool bool_val;
	int err = 0;
	wlc_info_t *wlc;
#ifdef WLAMSDU_TX
	uint i; /* BCMREFERENCE() may be more convienient but we save
		 * 1 stack var if the feature is not used.
		*/
#endif

	BCM_REFERENCE(wlcif);
	BCM_REFERENCE(alen);

	if (plen >= (int)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	bool_val = (int_val != 0) ? TRUE : FALSE;
	wlc = ami->wlc;
	ASSERT(ami == wlc->ami);

	switch (actionid) {
	case IOV_GVAL(IOV_AMSDU):
		int_val = wlc->pub->_amsdu_tx;
		bcopy(&int_val, a, val_size);
		break;

#ifdef WLAMSDU_TX
	case IOV_SVAL(IOV_AMSDU):
		if (AMSDU_TX_SUPPORT(wlc->pub))
			err = wlc_amsdu_set(ami, bool_val);
		else
			err = BCME_UNSUPPORTED;
		break;
#endif

	case IOV_GVAL(IOV_AMSDUNOACK):
		*pval = (int32)wlc->_amsdu_noack;
		break;

	case IOV_SVAL(IOV_AMSDUNOACK):
		wlc->_amsdu_noack = bool_val;
		break;

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	case IOV_GVAL(IOV_AMSDU_BLOCK):
		*pval = (int32)ami->amsdu_agg_block;
		break;

	case IOV_SVAL(IOV_AMSDU_BLOCK):
		ami->amsdu_agg_block = bool_val;
		break;

#ifdef WLCNT
	case IOV_GVAL(IOV_AMSDU_COUNTERS):
		bcopy(&ami->cnt, a, sizeof(ami->cnt));
		break;
#endif /* WLCNT */
#endif /* BCMDBG */

#ifdef WLAMSDU_TX
	case IOV_GVAL(IOV_AMSDU_AGGBYTES):
		if (AMSDU_TX_SUPPORT(wlc->pub)) {
			/* TODO, support all priorities ? */
			*pval = (int32)ami->txpolicy.amsdu_max_agg_bytes[PRIO_8021D_BE];
		} else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_SVAL(IOV_AMSDU_AGGBYTES):
		if (AMSDU_TX_SUPPORT(wlc->pub)) {
			struct scb *scb;
			struct scb_iter scbiter;
			uint32 uint_val = (uint)int_val;

			if (WLCISACPHY(wlc->band) && uint_val > VHT_MAX_AMSDU) {
				err = BCME_RANGE;
				break;
			}
			if (!WLCISACPHY(wlc->band) && (uint_val > (uint)HT_MAX_AMSDU)) {
				err = BCME_RANGE;
				break;
			}

			if (uint_val < AMSDU_AGGBYTES_MIN) {
				err = BCME_RANGE;
				break;
			}

#ifndef TXQ_MUX
			/* if smaller, flush existing aggregation, care only BE for now */
			if (uint_val < ami->txpolicy.amsdu_max_agg_bytes[PRIO_8021D_BE])
				wlc_amsdu_agg_flush(ami);
#endif

			for (i = 0; i < NUMPRIO; i++) {
				uint fifo_size;

				fifo_size = wlc->xmtfifo_szh[prio2fifo[i]];
				/* blocks to bytes */
				fifo_size = fifo_size * TXFIFO_SIZE_UNIT(wlc->pub->corerev);
				ami->txpolicy.amsdu_max_agg_bytes[i] =
						MIN(uint_val, fifo_size);
			}
			/* update amsdu agg bytes for ALL scbs */
			FOREACHSCB(wlc->scbstate, &scbiter, scb) {
				wlc_amsdu_scb_agglimit_upd(ami, scb);
			}
		} else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_GVAL(IOV_AMSDU_AGGSF):
		if (AMSDU_TX_SUPPORT(wlc->pub)) {
			/* TODO, support all priorities ? */
			*pval = (int32)ami->txpolicy.amsdu_max_sframes;
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;

	case IOV_SVAL(IOV_AMSDU_AGGSF):
#if defined(PROP_TXSTATUS) && defined(BCMPCIEDEV)
		if (BCMPCIEDEV_ENAB() && PROP_TXSTATUS_ENAB(wlc->pub) &&
			WLFC_GET_REUSESEQ(wlfc_query_mode(wlc->wlfc))) {
			err = BCME_UNSUPPORTED;
		} else
#endif /* PROP_TXSTATUS && BCMPCIEDEV */
		if (AMSDU_TX_SUPPORT(wlc->pub)) {

			if ((int_val > MAX_TX_SUBFRAMES_LIMIT) ||
			    (int_val > wlc->pub->tunables->ntxd/2) ||
			    (int_val < 1)) {
				err = BCME_RANGE;
				break;
			}

			for (i = 0; i < NUMPRIO; i++) {
				ami->txpolicy.amsdu_max_sframes = int_val;
			}

			/* Update the scbs */
			wlc_amsdu_set_scb_default_txpolicy_all(ami);
		} else
			err = BCME_UNSUPPORTED;
		break;
#endif /* WLAMSDU_TX */

#ifdef WLAMSDU
	case IOV_GVAL(IOV_RX_AMSDU_IN_AMPDU):
		int_val = (int8)(wlc->_rx_amsdu_in_ampdu);
		bcopy(&int_val, a, val_size);
		break;

	case IOV_SVAL(IOV_RX_AMSDU_IN_AMPDU):
		if (bool_val && D11REV_LT(wlc->pub->corerev, 40)) {
			WL_AMSDU(("wl%d: Not supported < corerev (40)\n", wlc->pub->unit));
			err = BCME_UNSUPPORTED;
		} else {
			wlc->_rx_amsdu_in_ampdu = bool_val;
		}

		break;
#endif /* WLAMSDU */

	default:
		err = BCME_UNSUPPORTED;
	}

	return err;
} /* wlc_amsdu_doiovar */


#ifdef WLAMSDU_TX
/**
 * Helper function that updates the per-SCB A-MSDU state.
 *
 * @param ami			A-MSDU module handle.
 * @param scb			scb pointer.
 * @param amsdu_agg_enable[]	array of enable states, one for each priority.
 *
 */
static void
wlc_amsdu_scb_agglimit_upd_all(amsdu_info_t *ami, struct scb *scb, bool amsdu_agg_enable[])
{
	uint i;
	amsdu_scb_txpolicy_t *scb_txpolicy = &(SCB_AMSDU_CUBBY(ami, scb))->scb_txpolicy[0];

	for (i = 0; i < NUMPRIO; i++) {
		scb_txpolicy[i].pub.amsdu_agg_enable = amsdu_agg_enable[i];
	}
	wlc_amsdu_scb_agglimit_upd(ami, scb);
}

/**
 * WLAMSDU_TX specific function.
 * called from fragthresh changes ONLY: update agg bytes limit, toss buffered A-MSDU
 * This is expected to happen very rarely since user should use very standard 802.11 fragthreshold
 *  to "disabled" fragmentation when enable A-MSDU. We can even ignore that. But to be
 *  full spec compliant, we reserve this capability.
 *   ??? how to inform user the requirement that not changing FRAGTHRESHOLD to screw up A-MSDU
 */
void
wlc_amsdu_agglimit_frag_upd(amsdu_info_t *ami)
{
	uint i;
	wlc_info_t *wlc = ami->wlc;
	struct scb *scb;
	struct scb_iter scbiter;
	bool flush = FALSE;
	bool frag_disabled = FALSE;
	bool amsdu_agg_enable[NUMPRIO];

	WL_AMSDU(("wlc_amsdu_agg_limit_upd\n"));

	if (!AMSDU_TX_SUPPORT(wlc->pub))
		return;

	if (!(WLC_PHY_11N_CAP(wlc->band)))
		return;

	for (i = 0; i < NUMPRIO; i++) {
		frag_disabled = (wlc->fragthresh[WME_PRIO2AC(i)] == DOT11_MAX_FRAG_LEN);

		if (!frag_disabled &&
			wlc->fragthresh[WME_PRIO2AC(i)] < ami->txpolicy.amsdu_max_agg_bytes[i]) {
			flush = TRUE;
			ami->txpolicy.amsdu_max_agg_bytes[i] = wlc->fragthresh[WME_PRIO2AC(i)];

			WL_INFORM(("wlc_amsdu_agg_frag_upd: amsdu_aggbytes[%d] = %d due to frag!\n",
				i, ami->txpolicy.amsdu_max_agg_bytes[i]));
		} else if (frag_disabled ||
			wlc->fragthresh[WME_PRIO2AC(i)] > ami->txpolicy.amsdu_max_agg_bytes[i]) {
			uint max_agg;
			uint fifo_size;
			if (WLCISACPHY(wlc->band)) {
#if AMSDU_VHT_USE_HT_AGG_LIMITS_ENAB
				max_agg = HT_MIN_AMSDU;
#else
				max_agg = VHT_MAX_AMSDU;
#endif /* AMSDU_VHT_USE_HT_AGG_LIMITS_ENAB */
			} else {
				max_agg = HT_MAX_AMSDU;
			}
			fifo_size = wlc->xmtfifo_szh[prio2fifo[i]];
			/* blocks to bytes */
			fifo_size = fifo_size * TXFIFO_SIZE_UNIT(wlc->pub->corerev);

			if (frag_disabled &&
				ami->txpolicy.amsdu_max_agg_bytes[i] == MIN(max_agg, fifo_size)) {
				/*
				 * Copy the A-MSDU  enable state over to
				 * properly initialize amsdu_agg_enable[]
				 */
				amsdu_agg_enable[i] = ami->txpolicy.amsdu_agg_enable[i];
				/* Nothing else to be done. */
				continue;
			}
#ifdef BCMDBG
			if (wlc->fragthresh[WME_PRIO2AC(i)] > MIN(max_agg, fifo_size)) {
				WL_AMSDU(("wl%d:%s: MIN(max_agg=%d, fifo_sz=%d)=>amsdu_max_agg\n",
					ami->wlc->pub->unit, __FUNCTION__, max_agg, fifo_size));
			}
#endif /* BCMDBG */
			ami->txpolicy.amsdu_max_agg_bytes[i] = MIN(fifo_size, max_agg);
			/* if frag not disabled, then take into account the fragthresh */
			if (!frag_disabled) {
				ami->txpolicy.amsdu_max_agg_bytes[i] =
					MIN(ami->txpolicy.amsdu_max_agg_bytes[i],
					wlc->fragthresh[WME_PRIO2AC(i)]);
			}
		}
		amsdu_agg_enable[i] = ami->txpolicy.amsdu_agg_enable[i] &&
			(ami->txpolicy.amsdu_max_agg_bytes[i] > AMSDU_AGGBYTES_MIN);

		if (!amsdu_agg_enable[i]) {
			WL_INFORM(("wlc_amsdu_agg_frag_upd: fragthresh is too small for AMSDU %d\n",
				i));
		}
	}

	/* toss A-MSDU since bust it up is very expensive, can't push through */
	if (flush) {
		wlc_amsdu_agg_flush(ami);
	}

	/* update all scb limit */
	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		wlc_amsdu_scb_agglimit_upd_all(ami, scb, amsdu_agg_enable);
	}
} /* wlc_amsdu_agglimit_frag_upd */

/** deal with WME txop dynamically shrink.  WLAMSDU_TX specific function. */
void
wlc_amsdu_txop_upd(amsdu_info_t *ami)
{
	BCM_REFERENCE(ami);
}

/** WLAMSDU_TX specific function. */
void
wlc_amsdu_scb_agglimit_upd(amsdu_info_t *ami, struct scb *scb)
{
#ifdef WL11AC
	wlc_amsdu_scb_vht_agglimit_upd(ami, scb);
#endif /* WL11AC */
}

#ifdef WL11AC
/** WLAMSDU_TX specific function. */
void
wlc_amsdu_scb_vht_agglimit_upd(amsdu_info_t *ami, struct scb *scb)
{
	uint i;
	scb_amsdu_t *scb_ami;
	uint16 vht_pref = 0;
#ifdef BCMDBG
	char eabuf[ETHER_ADDR_STR_LEN];
#endif /* BCMDBG */

	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	vht_pref = wlc_vht_get_scb_amsdu_mtu_pref(ami->wlc->vhti, scb);
#ifdef BCMDBG
	WL_AMSDU(("wl%d: %s: scb=%s scb->vht_mtu_pref %d\n",
		ami->wlc->pub->unit, __FUNCTION__, bcm_ether_ntoa(&(scb->ea), eabuf),
		vht_pref));
#endif

	for (i = 0; i < NUMPRIO; i++) {
		WL_AMSDU(("old: scb_txpolicy[%d].vhtaggbytesmax = %d", i,
			scb_ami->scb_txpolicy[i].amsdu_vht_agg_bytes_max));
		scb_ami->scb_txpolicy[i].amsdu_vht_agg_bytes_max =
			MIN(vht_pref,
			ami->txpolicy.amsdu_max_agg_bytes[i]);
		WL_AMSDU((" new: scb_txpolicy[%d].vht_agg_bytes_max = %d\n", i,
			scb_ami->scb_txpolicy[i].amsdu_vht_agg_bytes_max));
	}
}
#endif /* WL11AC */

/** WLAMSDU_TX specific function. */
void
wlc_amsdu_scb_ht_agglimit_upd(amsdu_info_t *ami, struct scb *scb)
{
	uint i;
	scb_amsdu_t *scb_ami;
	uint16 ht_pref = wlc_ht_get_scb_amsdu_mtu_pref(ami->wlc->hti, scb);

#ifdef BCMDBG
	char eabuf[ETHER_ADDR_STR_LEN];

	WL_AMSDU(("wl%d: %s: scb=%s scb->amsdu_ht_mtu_pref %d\n",
		ami->wlc->pub->unit, __FUNCTION__, bcm_ether_ntoa(&(scb->ea), eabuf),
		ht_pref));
#endif

	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	for (i = 0; i < NUMPRIO; i++) {
		WL_AMSDU(("old: scb_txpolicy[%d].ht_agg_bytes_max = %d", i,
			scb_ami->scb_txpolicy[i].amsdu_ht_agg_bytes_max));
		scb_ami->scb_txpolicy[i].amsdu_ht_agg_bytes_max = MIN(ht_pref,
			ami->txpolicy.amsdu_max_agg_bytes[i]);
		WL_AMSDU((" new: scb_txpolicy[%d].ht_agg_bytes_max = %d\n", i,
			scb_ami->scb_txpolicy[i].amsdu_ht_agg_bytes_max));
	}
}

#ifndef TXQ_MUX
/**
 * A-MSDU admission control, per-scb-tid. WLAMSDU_TX specific function.
 * called from tx completion, to decrement agg_txpend, compare with LOWM/HIWM
 * - this is called regardless the tx frame is AMSDU or not. the amsdu_agg_txpending
 *   increment/decrement for any traffic for scb-tid.
 * - work on best-effort traffic only for now, can be expanded to other in the future
 * - amsdu_agg_txpending never go below 0
 * - amsdu_agg_txpending may not be accurate before/after A-MSDU agg is added to txmodule
 *   config/unconfig dynamically
 */
static void
wlc_amsdu_dotxstatus(amsdu_info_t *ami, struct scb *scb, void* p)
{

	uint tid;
	scb_amsdu_t *scb_ami;

	WL_AMSDU(("wlc_amsdu_dotxstatus\n"));

	ASSERT(scb && SCB_AMSDU(scb));

	tid = (uint8)PKTPRIO(p);

	if (PRIO_8021D_BE != tid) {
		WL_AMSDU(("wlc_amsdu_dotxstatus, tid %d\n", tid));
		return;
	}

	scb_ami = SCB_AMSDU_CUBBY(ami, scb);

	ASSERT(scb_ami);
	if (scb_ami->aggstate[tid].amsdu_agg_txpending > 0)
		scb_ami->aggstate[tid].amsdu_agg_txpending--;

	WL_AMSDU(("wlc_amsdu_dotxstatus: scb txpending reduce to %d\n",
		scb_ami->aggstate[tid].amsdu_agg_txpending));

	if ((scb_ami->aggstate[tid].amsdu_agg_txpending < ami->txpolicy.fifo_lowm)) {
		if (scb_ami->aggstate[tid].amsdu_agg_p && !(scb->mark & SCB_DEL_IN_PROG)) {
			WL_AMSDU(("wlc_amsdu_dotxstatus: release amsdu due to low watermark!!\n"));
			wlc_amsdu_agg_close(ami, scb, tid); /* close and send AMSDU */
			WLCNTINCR(ami->cnt->agg_stop_lowwm);
		}
	}
} /* wlc_amsdu_dotxstatus */
#endif /* TXQ_MUX */

/** centralize A-MSDU tx policy */
void
wlc_amsdu_txpolicy_upd(amsdu_info_t *ami)
{
	WL_AMSDU(("wlc_amsdu_txpolicy_upd\n"));

	if (PIO_ENAB(ami->pub))
		ami->amsdu_agg_block = TRUE;
	else {
		int idx;
		wlc_bsscfg_t *cfg;
		FOREACH_BSS(ami->wlc, idx, cfg) {
			if (!cfg->BSS)
				ami->amsdu_agg_block = TRUE;
		}
	}

	ami->amsdu_agg_block = FALSE;
}

#ifndef TXQ_MUX
/**
 * Return vht max if to send at vht rates
 * Return ht max if to send at ht rates
 * Return 0 otherwise
 * WLAMSDU_TX specific function.
 */
static uint
wlc_amsdu_get_max_agg_bytes(struct scb *scb, scb_amsdu_t *scb_ami,
	uint tid, void *p, ratespec_t rspec)
{
#define MAX_SUPPPLY_TIME_US	5000 /* 5ms max rate */
#define _RSPEC2RATE(r)  wlc_rate_rspec2rate(r) /* Make a static table later */
	uint byte_limit;
	BCM_REFERENCE(scb);
	BCM_REFERENCE(p);

	if (RSPEC_ISVHT(rspec)) {
		WL_AMSDU(("vht rate max agg used = %d\n",
			scb_ami->scb_txpolicy[tid].amsdu_vht_agg_bytes_max));
		byte_limit = scb_ami->scb_txpolicy[tid].amsdu_vht_agg_bytes_max;
	} else {
		WL_AMSDU(("ht rate max agg used = %d\n",
			scb_ami->scb_txpolicy[tid].amsdu_ht_agg_bytes_max));
		byte_limit = scb_ami->scb_txpolicy[tid].amsdu_ht_agg_bytes_max;
	}

	return byte_limit;

}
#endif /* TXQ_MUX */
#endif /* WLAMSDU_TX */

#if defined(PKTC) || defined(PKTC_TX_DONGLE)

/**
 * Do AMSDU aggregation for chained packets. Called by AMPDU module (strange?) as in:
 *     txmod -> wlc_ampdu_agg_pktc() -> wlc_amsdu_pktc_agg()
 *
 *     @param[in/out] *ami
 *     @param[in/out] *p    packet chain that is extended with subframes in parameter 'n'
 *     @param[in/out] *n    packet chain containing subframes to aggregate
 *     @param[in]     tid   QoS class
 *     @param[in]     lifetime
 */
void * BCMFASTPATH
wlc_amsdu_pktc_agg(amsdu_info_t *ami, struct scb *scb, void *p, void *n,
	uint8 tid, uint32 lifetime)
{
	wlc_info_t *wlc;
	int32 pad, lastpad = 0;
	uint32 i, totlen = 0;
	struct ether_header *eh = NULL;
	scb_amsdu_t *scb_ami;
	void *p1 = NULL;
	bool pad_at_head = FALSE;

#ifdef BCMDBG
	if (ami->amsdu_agg_block) {
		WLCNTINCR(ami->cnt->agg_passthrough);
		return (n);
	}
#endif /* BCMDBG */

	pad = 0;
	i = 1;
	wlc = ami->wlc;
	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	totlen = pkttotlen(wlc->osh, p);

	/* msdu's in packet chain 'n' are aggregated while in this loop */
	while (1) {
#ifdef WLOVERTHRUSTER
		if (OVERTHRUST_ENAB(wlc->pub)) {
			/* Check if the next subframe is possibly TCP ACK */
			if (pkttotlen(wlc->osh, n) <= TCPACKSZSDU)
				break;
		}
#endif /* WLOVERTHRUSTER */
		if (i >= ami->txpolicy.amsdu_max_sframes) {
			WLCNTINCR(ami->cnt->agg_stop_sf);
			break;
		} else if ((totlen + pkttotlen(wlc->osh, n))
			>= scb_ami->scb_txpolicy[tid].amsdu_vht_agg_bytes_max) {
			WLCNTINCR(ami->cnt->agg_stop_len);
			break;
		}
#if defined(PROP_TXSTATUS) && defined(BCMPCIEDEV)
		else if (BCMPCIEDEV_ENAB() && PROP_TXSTATUS_ENAB(wlc->pub) &&
			WLFC_GET_REUSESEQ(wlfc_query_mode(wlc->wlfc))) {
			/*
			 * This comparison does the following:
			 *  - For suppressed pkts in AMSDU, the mask should be same
			 *      so re-aggregate them
			 *  - For new pkts, pkttag->seq is zero so same
			 *  - Prevents suppressed pkt from being aggregated with non-suppressed pkt
			 *  - Prevents suppressed MPDU from being aggregated with suppressed MSDU
			 */
			if ((WLPKTTAG(p)->seq & WL_SEQ_AMSDU_SUPPR_MASK) !=
				(WLPKTTAG(n)->seq & WL_SEQ_AMSDU_SUPPR_MASK)) {
				break;
			}
		}
#endif /* PROP_TXSTATUS && BCMPCIEDEV */

		/* Padding of A-MSDU sub-frame to 4 bytes */
		pad = (uint)((-(int)(pkttotlen(wlc->osh, p) - lastpad)) & 3);

		if (i == 1) {
			/* Init ether header */
			eh = (struct ether_header *)PKTPUSH(wlc->osh, p, ETHER_HDR_LEN);
			eacopy(&scb->ea, eh->ether_dhost);
			eacopy(&(SCB_BSSCFG(scb)->cur_etheraddr), eh->ether_shost);
			WLPKTTAG(p)->flags |= WLF_AMSDU;
			totlen = pkttotlen(wlc->osh, p);
		}

#ifdef DMATXRC
		/* Add padding to next pkt if current is phdr */
		if (DMATXRC_ENAB(wlc->pub) && (WLPKTTAG(p)->flags & WLF_PHDR)) {
			/* Possible p is not phdr but is flagged as WLF_PHDR
			 * in which case there's no next pkt.
			 */
			if (PKTNEXT(wlc->osh, p))
				p = PKTNEXT(wlc->osh, p);
		}
#endif

		/* Add padding to next pkt (at headroom) if current is a lfrag */
		if (BCMLFRAG_ENAB() && PKTISTXFRAG(wlc->osh, p)) {
			/*
			 * Store the padding value. We need this to be accounted for, while
			 * calculating the padding for the subsequent packet.
			 * For example, if we need padding of 3 bytes for the first packet,
			 * we add those 3 bytes padding in the head of second packet. Now,
			 * to check if padding is required for the second packet, we should
			 * calculate the padding without considering these 3 bytes that
			 * we have already put in.
			 */
			lastpad = pad;

			if (pad) {
				/*
				 * Let's just mark that padding is required at the head of next
				 * packet. Actual padding needs to be done after the next sdu
				 * header has been copied from the current sdu.
				 */
				pad_at_head = TRUE;

#ifdef WLCNT
				WLCNTINCR(ami->cnt->tx_padding_in_head);
#endif /* WLCNT */
			}
		} else {
			PKTSETLEN(wlc->osh, p, pkttotlen(wlc->osh, p) + pad);
			totlen += pad;
#ifdef WLCNT
			WLCNTINCR(ami->cnt->tx_padding_in_tail);
#endif /* WLCNT */
		}

		/* Consumes MSDU from head of chain 'n' and append this MSDU to chain 'p' */
		p1 = n;
		n = PKTCLINK(p1); /* advance 'n' by one MSDU */
		PKTSETCLINK(p1, NULL);
		PKTCLRCHAINED(wlc->osh, p1);
		if (BCMLFRAG_ENAB()) {
			PKTSETNEXT(wlc->osh, pktlast(wlc->osh, p), p1);
			PKTSETNEXT(wlc->osh, pktlast(wlc->osh, p1), NULL);
		} else {
			PKTSETNEXT(wlc->osh, p, p1); /* extend 'p' with one MSDU */
			PKTSETNEXT(wlc->osh, p1, NULL);
		}
		totlen += pkttotlen(wlc->osh, p1);
		i++;

		/* End of pkt chain */
		if (n == NULL)
			break;

		wlc_pktc_sdu_prep(wlc, scb, p1, n, lifetime);

		if (pad_at_head == TRUE) {
			/* If a padding was required for the previous packet, apply it here */
			PKTPUSH(wlc->osh, p1, pad);
			totlen += pad;
			pad_at_head = FALSE;
		}
		p = p1;
	} /* while */

	WLCNTINCR(ami->cnt->agg_amsdu);
	WLCNTADD(ami->cnt->agg_msdu, i);

	if (pad_at_head == TRUE) {
		/* If a padding was required for the previous/last packet, apply it here */
		PKTPUSH(wlc->osh, p1, pad);
		totlen += pad;
		pad_at_head = FALSE;
	}

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	/* update statistics histograms */
	ami->amdbg->tx_msdu_histogram[MIN(i-1,
		MAX_TX_SUBFRAMES_LIMIT-1)]++;
	ami->amdbg->tx_length_histogram[MIN(totlen / AMSDU_LENGTH_BIN_BYTES,
		AMSDU_LENGTH_BINS-1)]++;
#endif /* BCMDBG */

	if (i > 1)
		eh->ether_type = HTON16(totlen - ETHER_HDR_LEN);

	return n;
} /* wlc_amsdu_pktc_agg */
#endif /* PKTC */

#ifdef WLAMSDU_TX
#ifdef TXQ_MUX
/**
 * TXQMUX Checks if the A-MSDU can proceed, once for each block of output.
 *
 * @param scb_ami   	SCB A-MSDU handle
 * @param session	A-MSDU session control block
 * @return              TRUE if can be aggregated, FALSE otherwise
 */
static bool
wlc_amsdu_agg_allowed(scb_amsdu_t *scb_ami, amsdu_txsession_t *session)
{
	struct scb *scb = scb_ami->scb;
	amsdu_info_t *ami = scb_ami->ami;

	if (!(SCB_AMSDU(scb) && SCB_QOS(scb))) {
		return FALSE;
	}

	if (ami->amsdu_agg_block) {
		WL_NONE(("%s:PASSTHRU ami->amsdu_agg_block\n", __FUNCTION__));
		return FALSE;
	}

	/* Admission control */
	if (scb_ami->scb_txpolicy[session->priority].pub.amsdu_agg_enable == FALSE) {
		WL_AMSDU(("AMSDU Explicitly disabled for TID:%d\n", session->priority));
		return FALSE;
	}

	if (WSEC_ENABLED(session->bsscfg->wsec) &&
		!WSEC_AES_ENABLED(session->bsscfg->wsec)) {
		WL_NONE(("%s: target scb "MACF" is has unsupported WSEC\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea)));
		return FALSE;
	}

	return TRUE;

}

/**
 * TXQMUX A-MSDU per-packet check to decide whether to aggregate.
 *
 * Determines of packet can be added to the aggregate
 *
 * @param amsdustate   	A-MSDU cached state metadata for this aggregate
 * @param p		Packet to be processed, must not be NULL.
 *
 * @return              TRUE if can be aggregated, FALSE otherwise
 */
static INLINE bool BCMFASTPATH
wlc_amsdu_can_agg(amsdu_txaggstate_t *amsdustate, void *p)
{
	wlc_pkttag_t *pkttag = WLPKTTAG(p);
#ifdef WLCNT
	wlc_amsdu_cnt_t *cnt = amsdustate->cnt;
#endif
	ASSERT(p);

	WLCNTINCR(cnt->tx_nprocessed);

	if (pkttag->flags & amsdustate->passthrough_flags) {
		WL_AMSDU(("%s:PASSTHRU flags:0x%x MASK:0x%x\n",
			__FUNCTION__, pkttag->flags, amsdustate->passthrough_flags));
		WLCNTINCR(cnt->agg_stop_flags);
		return FALSE;
	}

	/* A-MSDUs carry only frames with the same TID */
	if (amsdustate->amsdu_tid != (uint)PKTPRIO(p)) {
		WL_AMSDU(("%s:TID Passthrough\n", __FUNCTION__));
		WLCNTINCR(cnt->agg_stop_tid);
		return FALSE;
	}

	/* Different packet callback index implies different packet processing on txcompletion */
	if (pkttag->callbackidx != amsdustate->callbackidx) {
		WL_AMSDU(("%s:Callbackidx Passthrough\n", __FUNCTION__));
		WLCNTINCR(cnt->agg_stop_pktcallback);
		return FALSE;
	}

	/*
	 * Different packet class callback stops aggregation
	 * The packet class call back metadata is an 8bit quantity in the packet tag
	 * This is the location of the first valid active PCB class.
	 * In the future the function will have to be reworked to accommodate
	 * the aditional byte sized groups if more PCB classes are added.
	 */
	if (*((uint8*)pkttag + amsdustate->pcb_offset) != amsdustate->pcb_data) {
		WL_AMSDU(("%s:Calss Callback Passthrough\n", __FUNCTION__));
		WLCNTINCR(cnt->agg_stop_pktcallback);
		return FALSE;
	}

#if defined(PROP_TXSTATUS) && defined(BCMPCIEDEV)
	if (amsdustate->proptxstatus_suppr &&
		((pkttag->seq & WL_SEQ_AMSDU_SUPPR_MASK) != amsdustate->seq_suppr_state)) {
	/*
	 * This comparison does the following:
	 *  - For suppressed pkts in A-MSDU, the mask should be same
	 *      so re-aggregate them
	 *  - For new pkts, pkttag->seq is zero so same
	 *  - Prevents suppressed pkt from being aggregated with non-suppressed pkt
	 *  - Prevents suppressed MPDU from being aggregated with suppressed MSDU
	 */
		WL_AMSDU(("%s:WL_SEQ_AMSDU_SUPPR_MASK Passthrough\n", __FUNCTION__));
		WLCNTINCR(cnt->agg_stop_hostsuppr);
		return FALSE;
	}
#endif /* PROP_TXSTATUS && BCMPCIEDEV */

#ifdef WLOVERTHRUSTER
	/* Allow TCP ACKs to flow through to Overthruster if it is enabled */
	if (amsdustate->ackRatioEnabled && wlc_amsdu_is_tcp_ack(amsdustate->ami, p)) {
		WL_AMSDU(("%s:TCP_ACK Passthrough\n", __FUNCTION__));
		return FALSE;
	}
#endif /* WLOVERTHRUSTER */
	return  TRUE;
}

/**
 * TXQMUX A-MSDU aggregation initialization routine
 *
 * Attaches a header to start of A-MSDU aggregate and returns the start of A-MSDU chain
 * Converts per-SCB policy information in scb_ami->txpolicy to the
 * runtime aggregation variable cache.
 * Cache in reformed each time at the beginning of each aggregation cycle.
 *
 * @param txinfo	Session and policy info block from caller
 * @param scb_ami	SCB A-MSDU pointer block, optimixation as the caller already computed it.
 * @param amsdustate	A-MSDU cached state metadata for this aggregate
 * @param p		First packet of the A-MSDU, returned to caller if aggregation fails.
 *
 *
 * @return              Start of A-MSDU packet chain or NULL on error.
 */
static void *
wlc_amsdu_agg_open(amsdu_txinfo_t *txinfo, scb_amsdu_t *scb_ami,
		amsdu_txaggstate_t *amsdustate, void *p)
{
	osl_t *osh;
	wlc_info_t *wlc;
	uint pkt_headroom;
	bool alloc_hdr = FALSE;
	int pkt_totlen;
	wlc_pkttag_t *pkttag;
	amsdu_txpolicy_t *txpolicy;

	wlc = scb_ami->ami->wlc;
	osh = wlc->osh;
	/*
	 * Zero out amsdustate, and initialize the fixed stuff,
	 * this simplifies the error unwind of the caller
	 */
	memset(amsdustate, 0, sizeof(*amsdustate));
	amsdustate->ami = scb_ami->ami;
	amsdustate->osh = osh;
	amsdustate->cnt = scb_ami->ami->cnt;

	if (p == NULL) {
		WLCNTINCR(scb_ami->ami->cnt->agg_stop_qempty);
		return NULL;
	}

	txpolicy = &txinfo->txpolicy;
	pkt_headroom = (uint)PKTHEADROOM(osh, p);
	pkt_totlen = (int)(pkttotlen(osh, p));
	alloc_hdr = (txpolicy->packet_headroom > pkt_headroom);

	if (BCMLFRAG_ENAB() && alloc_hdr) {
		/*
		 * Policy decision on dongle, return if there is no headroom,
		 * additional packet for header will not be allocated.
		 */
		WLCNTINCR(scb_ami->ami->cnt->agg_stop_headroom);
		return NULL;
	}

	pkttag = WLPKTTAG(p);

	/*
	 * State cache: Reduces the amount of dereferencing
	 * of the various state variables, this is a space vs speed tradeoff
	 * when processing the imbound packet chain.
	 */
	amsdustate->amsdu_head = p;
	amsdustate->lastpkt_lastbuf = pktlast(osh, p);
	amsdustate->amsdu_tid = txinfo->txsession.priority;
	amsdustate->amsdu_agg_bytes = pkt_totlen;
	amsdustate->lastpkt_len = pkt_totlen;
	amsdustate->hdr_headroom = txpolicy->packet_headroom;
	amsdustate->callbackidx = pkttag->callbackidx;
	amsdustate->ackRatioEnabled = txpolicy->tx_ackRatioEnabled;
	amsdustate->passthrough_flags = txpolicy->tx_passthrough_flags;
	amsdustate->amsdu_agg_sframes = 1;
	amsdustate->aggbytes_hwm = txinfo->txsession.aggbytes_hwm;
	amsdustate->amsdu_max_bytes = txpolicy->amsdu_max_agg_bytes;
	amsdustate->amsdu_max_sframes = txpolicy->amsdu_max_sframes;
#ifdef WLCNT
	if (!txinfo->txsession.ratelimit) {
		amsdustate->stop_counter = &amsdustate->cnt->agg_stop_len;
	} else {
		amsdustate->stop_counter = &amsdustate->cnt->agg_stop_ratelimit;
	}
#endif

	/* Get the packet callback function bits */
	wlc_pcb_getdata(wlc->pcb, pkttag, &amsdustate->pcb_offset, &amsdustate->pcb_data);
	ASSERT(amsdustate->pcb_offset < sizeof(*pkttag));

#if defined(PROP_TXSTATUS) && defined(BCMPCIEDEV)
	amsdustate->proptxstatus_suppr =
			(BCMPCIEDEV_ENAB() &&
			PROP_TXSTATUS_ENAB(wlc->pub) &&
			WLFC_GET_REUSESEQ(wlfc_query_mode(wlc->wlfc)));
	amsdustate->seq_suppr_state = (pkttag->seq & WL_SEQ_AMSDU_SUPPR_MASK);
#endif

	if (!wlc_amsdu_can_agg(amsdustate, p)) {
		return NULL;
	}

	/*
	 * Allocate header early, if this fails abandon A-MSDU now so that we do not
	 * have to unwind and requeue the packets later
	 */
	if (alloc_hdr) {
		amsdustate->amsdu_hdr = PKTGET(osh, amsdustate->hdr_headroom, TRUE);
		if (amsdustate->amsdu_hdr == NULL) {
			return NULL;
		}
	}

	/* Note: no special handling for LFRAG case as there is an explicit check for headroom */

	return p;
} /* wlc_amsdu_agg_open */

/**
 * TXQMUX A-MSDU in A-MPDU policy function.
 *
 * @param wlc		wlc_info_t handle
 * @param scb		scb peer of this aggregation session
 * @param bsscfg	bss of the current scb
 * @param txinfo	Session and policy info block from caller
 * @param priority	FIFO priority
 * @param rspec		current primary ratespec
 * @param min_agglen	minimum acceptable aggregation length from caller.
 * @param min_aggsf	minimum number of supported subframes for aggregation from caller.
 *
 * @return              TRUE if A-MSDU is permitted, FALSE otherwise
 */
bool
wlc_amsdu_init_session(wlc_info_t *wlc, struct scb *scb,  wlc_bsscfg_t *bsscfg,
		amsdu_txinfo_t *txinfo, uint priority, ratespec_t rspec,
		uint min_agglen, uint min_aggsf)
{
#define MAX_MPDU_TIME_US 5000 /* Mixed Mode packets limited to max ~5ms */

	scb_amsdu_t *scb_ami = SCB_AMSDU_CUBBY(wlc->ami, scb);
	amsdu_scb_txpolicy_t scb_txpolicy;
	uint max_bytes_scb_default;
	uint ratebytes;
	int ratekbps;

	ASSERT(priority < NUMPRIO);


	if (RSPEC_ISLEGACY(rspec)) {
		WLCNTINCR(scb_ami->ami->cnt->passthrough_legacy_rate);
		WL_NONE(("%s() PASSTHROUGH:legacy rate\n", __FUNCTION__));
		return FALSE;
	}

	ratekbps =  wlc_rate_rspec2rate(rspec);

	/* The zero return is only for malformed rspecs, should get only good rspecs from rate */
	ASSERT(ratekbps > 0);

	/* Setup the A-MSDU minimum session info for the agg check below */
	txinfo->txsession.priority = priority;
	txinfo->txsession.rspec = rspec;
	txinfo->txsession.bsscfg = bsscfg;
	txinfo->txsession.ctx = scb_ami;
	txinfo->txsession.ratelimit = FALSE;

	if (wlc_amsdu_agg_allowed(scb_ami, &txinfo->txsession) == FALSE) {
		return FALSE;
	}

	/* Load the default txpolicy for the caller specified priority. */
	wlc_amsdu_get_scb_cfg_txpolicy(scb_ami, priority, &scb_txpolicy);

	/*
	 * Apply local overrides.
	 * for example: current rspec may be HT on a VHT capble device,
	 * this readjusts the limits accordingly.
	 *
	 * The amsdu agg byte limits are stored per tid for the highest possible rates,
	 * but are based on fifo size limits (per AC fifos).
	 * This function gets a prio/tid parameter to access the per tid byte limits,
	 * but really gets the same value for prios that map to the same data fifo.
	 *
	 * If the A-MSDU limits change, it will not take effect
	 * until this function is invoked again.
	 */

	if (RSPEC_ISVHT(rspec)) {
		scb_txpolicy.pub.amsdu_max_agg_bytes =
			MIN(scb_txpolicy.pub.amsdu_max_agg_bytes,
			scb_txpolicy.amsdu_vht_agg_bytes_max);

		WL_AMSDU(("vht rate max agg used = %d\n",
			scb_txpolicy.pub.amsdu_max_agg_bytes));
	} else {
		scb_txpolicy.pub.amsdu_max_agg_bytes =
			MIN(scb_txpolicy.pub.amsdu_max_agg_bytes,
			scb_txpolicy.amsdu_ht_agg_bytes_max);

		WL_AMSDU(("ht rate max agg used = %d\n",
			scb_txpolicy.pub.amsdu_max_agg_bytes));
	}
	max_bytes_scb_default = scb_txpolicy.pub.amsdu_max_agg_bytes;

	ratebytes = (uint)((MAX_MPDU_TIME_US >> 3)*(ratekbps/1000));

	scb_txpolicy.pub.amsdu_max_agg_bytes = MIN(scb_txpolicy.pub.amsdu_max_agg_bytes, ratebytes);

	if (scb_txpolicy.pub.amsdu_max_agg_bytes < max_bytes_scb_default) {
		WLCNTINCR(scb_ami->ami->cnt->rlimit_max_bytes);
		WL_NONE(("Adjusted length maxbytes: %u rate:%d  ratebytes:%u\n",
			scb_txpolicy.pub.amsdu_max_agg_bytes, ratekbps, ratebytes));
	}

	/* Negotiated max agglen is less than caller requested value */
	if (scb_txpolicy.pub.amsdu_max_agg_bytes < min_agglen) {
		WL_NONE(("%s() PASSTHROUGH:min_agglen not met\n", __FUNCTION__));
		WLCNTINCR(scb_ami->ami->cnt->passthrough_min_agglen);
		return FALSE;
	/* Allowed max subframes less than called specified minimum subframe count */
	} else if (scb_txpolicy.pub.amsdu_max_sframes < min_aggsf) {
		WL_NONE(("%s() PASSTHROUGH:min_aggssf not met\n", __FUNCTION__));
		WLCNTINCR(scb_ami->ami->cnt->passthrough_min_aggsf);
		return FALSE;
	/* The maxagglen of the specified rate is less than the mimimum payload length */
	} else if (scb_txpolicy.pub.amsdu_max_agg_bytes < (WLC_AMSDU_MIN_SFSIZE * min_aggsf)) {
		WL_NONE(("%s() PASSTHROUGH:min_agglen not met due to rate limit\n", __FUNCTION__));
		WLCNTINCR(scb_ami->ami->cnt->passthrough_rlimit_bytes);
		txinfo->txsession.ratelimit = TRUE;
		return FALSE;
	}

	/*
	 * max sub frames is set during SCB attach time, no further action required unless
	 * an override is required.
	 */
	txinfo->txsession.aggbytes_hwm =
		scb_txpolicy.pub.amsdu_max_agg_bytes - scb_txpolicy.pub.aggbytes_hwm_delta;
	txinfo->txpolicy = scb_txpolicy.pub;

	return TRUE;

}
/**
 * TXQMUX A-MSDU aggregation routine.
 *
 * This function return either and aggregated packet or a next single packet in the queue
 *
 *
 * @param pq        Inbound multiprecedence packet queue.
 * @param prec_map  Map of precedences to dequeue.
 * @param txinfo    A-MSDU Session and policy info block from caller.
 * @param p         Pointer to first packet of the potential aggregate.
 *
 * @return          The aggregated or next packet in the queue.
 */
void * BCMFASTPATH
wlc_amsdu_pktq_agg(struct pktq *pq, uint prec_map, amsdu_txinfo_t *txinfo, void *p)
{
	uint len, totlen = 0;
	void *ptid = NULL;
	amsdu_txaggstate_t amsdustate;
	uint8 pad = 0;
	struct ether_header *eh;
	int prec;
	scb_amsdu_t *scb_ami;
#ifdef WLCNT
	uint32 tx_padding_in_tail = 0;
	uint32 tx_padding_in_head = 0;
	uint32 tx_padding_no_pad = 0;
#endif /* WLCNT */

	ASSERT(txinfo);

	scb_ami = (scb_amsdu_t *)(txinfo->txsession.ctx);
	ASSERT(scb_ami);

	/* Open and initialize AMSDU, first packet comes from the A-MPDU module */
	if (wlc_amsdu_agg_open(txinfo, scb_ami, &amsdustate, p) == NULL) {
		WLCNTINCR(amsdustate.cnt->agg_openfail);
		ptid = p;
		WL_AMSDU(("%s:AGG open fail\n", __FUNCTION__));
		goto agg_passthrough;
	}

	/*
	 * Dequeue second packet, special handling to avoid making a singleton A-MSDU
	 * If p is null, skip subsequent A-MSDU processing, this is the most common case.
	 * Singletons can still happen for other reasons.
	 * like 802.1x packets but this is much less common.
	 *
	 * This clumsy arrangement of null pointer checks ensures.
	 * the empty queue agg stop condition is accurately tracked.
	 * and to separate this condition from other agg open failures.
	 *
	 * The empty queue condition is an indication of how efficiently
	 * the datapath is transmitting the data packets.
	 */
	if ((p = pktq_mdeq(pq, prec_map, &prec)) == NULL) {
		ptid = p;
		WL_AMSDU(("%s:Single packet in queue\n", __FUNCTION__));
		goto amsdu_close_agg;
	}

	while (wlc_amsdu_can_agg(&amsdustate, p)) {

		/* Calculate required padding of preceeding packet */
		pad = ((uint)((-(int)(amsdustate.lastpkt_len)) & 3));

		/*
		 * length of 802.3/LLC frame, equivalent to
		 * A-MSDU sub-frame length, DA/SA/Len/Data
		 */
		len = pkttotlen(amsdustate.osh, p);
		totlen = len + pad + amsdustate.amsdu_agg_bytes;

		if (totlen > amsdustate.amsdu_max_bytes) {
			WL_AMSDU(("%s:AGG stop len=%d sf=%d\n", __FUNCTION__,
					totlen, amsdustate.amsdu_agg_sframes));
			WLCNTINCR(*amsdustate.stop_counter);
			goto amsdu_close_agg;
		}

		/*
		 * This block decides where to put the pad bytes,
		 * either in the head of the current packet or the tail of the previous packet
		 * Relevant counters updated in the process.
		 */
		if (pad > 0) {
			void *pad_start;

			/*
			 * Try using tailroom first
			 * Tail pad not supported on dongle as the buffer
			 * is in the host, not easily accessible
			 */
			if (((uint)PKTTAILROOM(amsdustate.osh,
				amsdustate.lastpkt_lastbuf) >= pad) && !BCMLFRAG_ENAB()) {
				uint lastbuf_len =
					(uint)PKTLEN(amsdustate.osh, amsdustate.lastpkt_lastbuf);
				PKTSETLEN(amsdustate.osh, amsdustate.lastpkt_lastbuf,
					lastbuf_len + pad);
				pad_start = PKTDATA(amsdustate.osh, amsdustate.lastpkt_lastbuf) +
					lastbuf_len;
				WLCNTINCR(tx_padding_in_tail);
			/* Try headroom next -> default for dongle */
			} else if ((uint)PKTHEADROOM(amsdustate.osh, p) >= pad) {
				pad_start = PKTPUSH(amsdustate.osh, p, pad);
				WLCNTINCR(tx_padding_in_head);
			/*
			 * Stop agg, exit and increment error counter, if pkt has insufficient
			 * [head|tail]room.
			 */
			} else {
				WL_AMSDU(("%s:AGG stop nopad\n", __FUNCTION__));
				WLCNTINCR(amsdustate.cnt->agg_stop_pad_fail);
				goto amsdu_close_agg;
			}
			/* Zero out padbytes */
			memset(pad_start, 0, pad);
		} else {
			WLCNTINCR(tx_padding_no_pad);
		}

		/* Chain packet to A-MSDU output chain */
		PKTSETNEXT(amsdustate.osh, amsdustate.lastpkt_lastbuf, p);
		amsdustate.lastpkt_lastbuf = pktlast(amsdustate.osh, p);
		amsdustate.lastpkt_len = len;
		amsdustate.amsdu_agg_sframes++;
		amsdustate.amsdu_agg_bytes = totlen;

		/* Optimizations to avoid additional dequeue */
		if (amsdustate.amsdu_agg_sframes == amsdustate.amsdu_max_sframes) {
			WLCNTINCR(amsdustate.cnt->agg_stop_sf);
			/* NULL out p as we have not dequeued a new packet */
			p = NULL;
			goto amsdu_close_agg;
		}

		/*
		 * Stop, if length high watermark is reached.
		 * Beyond this point, the probablity of being able to include
		 * the next packet drops substantially.
		 * The limit is set when the amsdu module is instantiated.
		 */
		if (amsdustate.amsdu_agg_bytes > amsdustate.aggbytes_hwm) {
			WLCNTINCR(*amsdustate.stop_counter);
			/* NULL out p as we have not dequeued a new packet */
			p = NULL;
			goto amsdu_close_agg;
		}

		/* Dequeue next packet */
		if ((p = pktq_mdeq(pq, prec_map, &prec)) == NULL) {
			WLCNTINCR(amsdustate.cnt->agg_stop_qempty);
			goto amsdu_close_agg;
		}
	}

amsdu_close_agg:

	/*
	 * Close A-MSDU
	 * Counters are not updated here, updated by code doing the jump
	 * to this section
	 */

	ptid = amsdustate.amsdu_head;
	ASSERT(ptid);

	/* Pop back un-aggregated packet */
	if (p) {
		WL_AMSDU(("%s:Put back packet\n", __FUNCTION__));
		WLCNTINCR(amsdustate.cnt->tx_pkt_putback);
		pktq_penq_head(pq, prec, p);
	}

	/* Abandon A-MSDU on single packets and passthru */
	if (amsdustate.amsdu_agg_sframes == 1) {
		WL_AMSDU(("%s:Singleton packet. Abandon A-MSDU\n", __FUNCTION__));
		/*
		 * Abandoned A-MSDUs are not counted in the MSDU/A-MSDU counters
		 * Considered to be a passthrough event.
		 */
		WLCNTINCR(amsdustate.cnt->tx_singleton);
		goto agg_passthrough;
	}

	/* Populate externally allocated A-MSDU header if present */
	if (amsdustate.amsdu_hdr) {
		WLCNTINCR(amsdustate.cnt->agg_alloc_newhdr);
		ASSERT(ISALIGNED(PKTDATA(amsdustate.osh, amsdustate.amsdu_hdr), sizeof(uint32)));

		/* Adjust the data point for correct pkttotlength */
		PKTPULL(amsdustate.osh, amsdustate.amsdu_hdr, amsdustate.hdr_headroom);
		PKTSETLEN(amsdustate.osh, amsdustate.amsdu_hdr, 0);

		/* Link header to A-MSDU chain, transfer pkttag, and scb  */
		wlc_pkttag_info_move(amsdustate.ami->wlc, ptid, amsdustate.amsdu_hdr);
		PKTSETPRIO(ptid, amsdustate.amsdu_tid);
		WLPKTTAGSCBSET(amsdustate.amsdu_hdr, scb_ami->scb);
		PKTSETNEXT(amsdustate.osh, amsdustate.amsdu_hdr, ptid);
		ptid = amsdustate.amsdu_hdr;
	}

	/* Update the PKTTAG with A-MSDU flag */
	WLPKTTAG(ptid)->flags |= WLF_AMSDU;

	/*
	 * Prepend the |DA SA LEN| to the same packet. At the end of this
	 * operation, the packet would contain the following.
	 * +-----------------------------+-------------------+---------------------------+
	 * |A-MSDU header(ethernet like) | Subframe1 8023hdr | Subframe1 body(Host Frag) |
	 * +-----------------------------+-------------------+---------------------------+
	 */
	eh = (struct ether_header*) PKTPUSH(amsdustate.osh, ptid, ETHER_HDR_LEN);

	bcopy((char*)(&scb_ami->scb->ea), eh->ether_dhost, ETHER_ADDR_LEN);
	bcopy((char*)(&txinfo->txsession.bsscfg->cur_etheraddr), eh->ether_shost, ETHER_ADDR_LEN);
	eh->ether_type = hton16((uint16)amsdustate.amsdu_agg_bytes);

	WLCNTINCR(amsdustate.cnt->agg_amsdu);
	WLCNTADD(amsdustate.cnt->agg_msdu, amsdustate.amsdu_agg_sframes);
	WLCNTADD(amsdustate.cnt->tx_padding_no_pad, tx_padding_no_pad);
	WLCNTADD(amsdustate.cnt->tx_padding_in_head, tx_padding_in_head);
	WLCNTADD(amsdustate.cnt->tx_padding_in_tail, tx_padding_in_tail);

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	/* update statistics histograms */
	amsdustate.ami->amdbg->tx_msdu_histogram[
		MIN(amsdustate.amsdu_agg_sframes-1, MAX_TX_SUBFRAMES_LIMIT-1)]++;
	amsdustate.ami->amdbg->tx_length_histogram[
		MIN(amsdustate.amsdu_agg_bytes / AMSDU_LENGTH_BIN_BYTES, AMSDU_LENGTH_BINS-1)]++;
#endif /* BCMDBG */

	WL_AMSDU(("%s: valid AMSDU, add to txq %d bytes, scb %p\n",
		__FUNCTION__, amsdustate.amsdu_agg_bytes, scb_ami->scb));

	return ptid;

agg_passthrough:
	/* Free pre-allocated A-MSDU header if one was made */
	if (amsdustate.amsdu_hdr != NULL) {
		PKTFREE(amsdustate.osh, amsdustate.amsdu_hdr, FALSE);
	}
	WLCNTINCR(amsdustate.cnt->agg_passthrough);
	return ptid;

}
#else
static void
wlc_amsdu_agg(void *ctx, struct scb *scb, void *p, uint prec)
{
	amsdu_info_t *ami;
	osl_t *osh;
	uint tid = 0;
	scb_amsdu_t *scb_ami;
	wlc_bsscfg_t *bsscfg;
	uint totlen;
	uint maxagglen;
	ratespec_t rspec;

	ami = (amsdu_info_t *)ctx;
	osh = ami->pub->osh;
	scb_ami = SCB_AMSDU_CUBBY(ami, scb);

	if (ami->amsdu_agg_block) {
		WL_NONE(("%s:PASSTHROUGH ami->amsdu_agg_block\n", __FUNCTION__));
		goto passthrough;
	}

	/* doesn't handle MPDU,  */
	if (WLPKTTAG(p)->flags & WLF_MPDU) {
		WL_NONE(("%s:PASSTHROUGH WLPKTTAG(p)->flags & WLF_MPDU\n", __FUNCTION__));
		goto passthrough;
	}

	/* non best-effort, skip for now */
	tid = PKTPRIO(p);
	ASSERT(tid < NUMPRIO);
	if (tid != PRIO_8021D_BE) {
		WL_NONE(("%s:PASSTHROUGH tid != PRIO_8021D_BE\n", __FUNCTION__));
		goto passthrough;
	}

	/* admission control */
	if (!scb_ami->scb_txpolicy[tid].pub.amsdu_agg_enable) {
		WL_NONE(("%s:PASSTHROUGH !ami->amsdu_agg_enable[tid]\n", __FUNCTION__));
		goto passthrough;
	}

	if (WLPKTTAG(p)->flags & WLF_8021X) {
		WL_NONE(("%s:PASSTHROUGH WLPKTTAG(p)->flags & WLF_8021X\n", __FUNCTION__));
		goto passthrough;
	}



	scb = WLPKTTAGSCBGET(p);
	ASSERT(scb);

	/* the scb must be A-MSDU capable */
	ASSERT(SCB_AMSDU(scb));
	ASSERT(SCB_QOS(scb));

	rspec = wlc_scb_ratesel_get_primary(scb->bsscfg->wlc, scb, p);

#ifdef VHT_TESTBED
	if (RSPEC_ISLEGACY(rspec)) {
		WL_AMSDU(("PASSTHROUGH legacy rate\n"));
		goto passthrough;
	}
#else
	if (WLCISACPHY(scb->bsscfg->wlc->band)) {
		if (!RSPEC_ISVHT(rspec)) {
			WL_AMSDU(("PASSTHROUGH non-VHT on VHT phy\n"));
			goto passthrough;
		}
	}
	else {
		/* add for WNM support (r365273) */
		if (WLC_PHY_11N_CAP(scb->bsscfg->wlc->band) && !RSPEC_ISHT(rspec)) {
			WL_AMSDU(("PASSTHROUGH non-HT on HT phy\n"));
			goto passthrough;
		}
	}
#endif /* VHT_TESTBED */

	if (SCB_AMPDU(scb)) {
		/* Bypass AMSDU agg if the destination has AMPDU enabled but does not
		 * advertise AMSDU in AMPDU.
		 * This capability is advertised in the ADDBA response.
		 * This check is not taking into account whether or not the AMSDU will
		 * be encapsulated in an AMPDU, so it is possibly too conservative.
		 */
		if (!SCB_AMSDU_IN_AMPDU(scb)) {
			WL_NONE(("%s:PASSTHROUGH !SCB_AMSDU_IN_AMPDU(scb)\n", __FUNCTION__));
			goto passthrough;
		}

#ifdef WLOVERTHRUSTER
		if (OVERTHRUST_ENAB(ami->wlc->pub)) {
			/* Allow TCP ACKs to flow through to Overthruster if it is enabled */
			if (wlc_ampdu_tx_get_tcp_ack_ratio(ami->wlc->ampdu_tx) > 0 &&
				wlc_amsdu_is_tcp_ack(ami, p)) {
				WL_NONE(("%s:PASSTHROUGH TCP_ACK\n", __FUNCTION__));
				goto passthrough;
			}
		}
#endif /* WLOVERTHRUSTER */
	}

#ifdef WLAMPDU_PRECEDENCE
	/* check existing AMSDU (if there is one) has same WLF2_FAVORED attribute as
	 * new packet
	 */
	if (scb_ami->aggstate[tid].amsdu_agg_p) {
		if ((WLPKTTAG(p)->flags2 & WLF3_FAVORED) !=
		       (WLPKTTAG(scb_ami->aggstate[tid].amsdu_agg_p)->flags2 & WLF3_FAVORED)) {
			WL_AMSDU(("close amsdu due to change in favored status\n"));
			wlc_amsdu_agg_close(ami, scb, tid); /* close and send AMSDU */
		}
	}
#endif /* WLAMPDU_PRECEDENCE */

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);

	if (WSEC_ENABLED(bsscfg->wsec) && !WSEC_AES_ENABLED(bsscfg->wsec)) {
		WL_AMSDU(("%s: target scb %p is has wrong WSEC\n",
			__FUNCTION__, OSL_OBFUSCATE_BUF(scb)));
		goto passthrough;
	}

	WL_AMSDU(("%s: txpend %d\n", __FUNCTION__, scb_ami->aggstate[tid].amsdu_agg_txpending));

	if (scb_ami->aggstate[tid].amsdu_agg_txpending < ami->txpolicy.fifo_hiwm) {
		WL_NONE((
		"%s: scb_ami->aggstate[tid].amsdu_agg_txpending < ami->txpolicy.fifo_hiwm\n",
		__FUNCTION__));
		goto passthrough;
	} else {
		WL_AMSDU(("%s: Starts aggregation due to hiwm %d reached\n",
			__FUNCTION__, ami->txpolicy.fifo_hiwm));
	}

	totlen = pkttotlen(osh, p) + scb_ami->aggstate[tid].amsdu_agg_bytes +
		scb_ami->aggstate[tid].headroom_pad_need;

	maxagglen = wlc_amsdu_get_max_agg_bytes(scb, scb_ami, tid, p, rspec);

	if ((totlen > maxagglen - ETHER_HDR_LEN) ||
		(scb_ami->aggstate[tid].amsdu_agg_sframes + 1 >
		ami->txpolicy.amsdu_max_sframes)) {
		WL_AMSDU(("%s: terminte A-MSDU for txbyte %d or txframe %d\n",
			__FUNCTION__, maxagglen,
			ami->txpolicy.amsdu_max_sframes));
#ifdef WLCNT
		if (totlen > maxagglen - ETHER_HDR_LEN) {
			WLCNTINCR(ami->cnt->agg_stop_len);
		} else {
			WLCNTINCR(ami->cnt->agg_stop_sf);
		}
#endif /* WLCNT */

		wlc_amsdu_agg_close(ami, scb, tid); /* close and send AMSDU */

		/* if the new pkt itself is more than aggmax, can't continue with agg_append
		 *   add here to avoid per pkt checking for this rare case
		 */
		if (pkttotlen(osh, p) > maxagglen - ETHER_HDR_LEN) {
			WL_AMSDU(("%s: A-MSDU aggmax is smaller than pkt %d, pass\n",
				__FUNCTION__, pkttotlen(osh, p)));

			goto passthrough;
		}
	}

	BCM_REFERENCE(prec);

	/* agg this one and return on success */
	if (wlc_amsdu_agg_append(ami, scb, p, tid, rspec)) {
		return;
	}

passthrough:
	/* A-MSDU agg rejected, pass through to next tx module */

	/* release old first before passthrough new one to maintain sequence */
	if (scb_ami->aggstate[tid].amsdu_agg_p) {
		WL_AMSDU(("%s: release amsdu for passthough!!\n", __FUNCTION__));
		wlc_amsdu_agg_close(ami, scb, tid); /* close and send AMSDU */
		WLCNTINCR(ami->cnt->agg_stop_passthrough);
	}
	scb_ami->aggstate[tid].amsdu_agg_txpending++;
	WLCNTINCR(ami->cnt->agg_passthrough);
	WL_AMSDU(("%s: passthrough scb %p txpending %d\n",
		__FUNCTION__, OSL_OBFUSCATE_BUF(scb),
		scb_ami->aggstate[tid].amsdu_agg_txpending));
	WLF2_PCB3_REG(p, WLF2_PCB3_AMSDU);
	SCB_TX_NEXT(TXMOD_AMSDU, scb, p, WLC_PRIO_TO_PREC(tid));
} /* wlc_amsdu_agg */

/**
 * Finalizes a transmit A-MSDU and forwards the A-MSDU to the next transmit layer.
 */
static void
wlc_amsdu_agg_close(amsdu_info_t *ami, struct scb *scb, uint tid)
{
	/* ??? cck rate is not supported in hw, how to restrict rate algorithm later */
	scb_amsdu_t *scb_ami;
	void *ptid;
	osl_t *osh;
	struct ether_header *eh;
	amsdu_txaggstate_t *aggstate;
	uint16 amsdu_body_len;

	WL_AMSDU(("wlc_amsdu_agg_close\n"));

	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	osh = ami->pub->osh;
	aggstate = &scb_ami->aggstate[tid];
	ptid = aggstate->amsdu_agg_p;

	if (ptid == NULL)
		return;

	ASSERT(WLPKTFLAG_AMSDU(WLPKTTAG(ptid)));

	/* wlc_pcb_fn_register(wlc->pcb, wlc_amsdu_tx_complete, ami, ptid); */

	/* check */
	ASSERT(PKTLEN(osh, ptid) >= ETHER_HDR_LEN);
	ASSERT(tid == (uint)PKTPRIO(ptid));

	/* FIXUP lastframe pad --- the last subframe must not be padded,
	 * reset pktlen to the real length(strip off pad) using previous
	 * saved value.
	 * amsdupolicy->amsdu_agg_ptail points to the last buf(not last pkt)
	 */
	if (aggstate->amsdu_agg_padlast) {
		PKTSETLEN(osh, aggstate->amsdu_agg_ptail,
			PKTLEN(osh, aggstate->amsdu_agg_ptail) -
			aggstate->amsdu_agg_padlast);
		aggstate->amsdu_agg_bytes -= aggstate->amsdu_agg_padlast;
		WL_AMSDU(("wlc_amsdu_agg_close: strip off padlast %d\n",
			aggstate->amsdu_agg_padlast));
#ifdef WLCNT
		WLCNTDECR(ami->cnt->tx_padding_in_tail);
		WLCNTINCR(ami->cnt->tx_padding_no_pad);
#endif /* WLCNT */
	}

	amsdu_body_len = (uint16) aggstate->amsdu_agg_bytes;

	eh = (struct ether_header*) PKTDATA(osh, ptid);
	eh->ether_type = hton16(amsdu_body_len);

	WLCNTINCR(ami->cnt->agg_amsdu);
	WLCNTADD(ami->cnt->agg_msdu, aggstate->amsdu_agg_sframes);

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	/* update statistics histograms */
	ami->amdbg->tx_msdu_histogram[MIN(aggstate->amsdu_agg_sframes-1,
	                                  MAX_TX_SUBFRAMES_LIMIT-1)]++;
	ami->amdbg->tx_length_histogram[MIN(amsdu_body_len / AMSDU_LENGTH_BIN_BYTES,
	                                    AMSDU_LENGTH_BINS-1)]++;
#endif /* BCMDBG */

	aggstate->amsdu_agg_txpending++;

	WL_AMSDU(("wlc_amsdu_agg_close: valid AMSDU, add to txq %d bytes, scb %p, txpending %d\n",
		aggstate->amsdu_agg_bytes, OSL_OBFUSCATE_BUF(scb),
		scb_ami->aggstate[tid].amsdu_agg_txpending));

	/* clear state prior to calling next txmod, else crash may occur if next mod frees pkt */
	/* due to amsdu use of pcb to track pkt freeing */
	aggstate->amsdu_agg_p = NULL;
	aggstate->amsdu_agg_ptail = NULL;
	aggstate->amsdu_agg_sframes = 0;
	aggstate->amsdu_agg_bytes = 0;
	aggstate->headroom_pad_need = 0;
	WLF2_PCB3_REG(ptid, WLF2_PCB3_AMSDU);
	SCB_TX_NEXT(TXMOD_AMSDU, scb, ptid, WLC_PRIO_TO_PREC(tid)); /* forwards to next layer */

} /* wlc_amsdu_agg_close */

/**
 * 'opens' a new AMSDU for aggregation. Creates a pseudo ether_header, the len field will be updated
 * when aggregating more frames
 */
static void *
wlc_amsdu_agg_open(amsdu_info_t *ami, wlc_bsscfg_t *bsscfg, struct ether_addr *ea, void *p)
{
	struct ether_header* eh;
	uint headroom;
	void *pkt;
	osl_t *osh;
	wlc_info_t *wlc = ami->wlc;
	wlc_pkttag_t *pkttag;

	osh = wlc->osh;

	/* allocate enough room once for all cases */
	headroom = TXOFF;


	if (BCMLFRAG_ENAB() && PKTISTXFRAG(osh, p)) {
		/*
		 * LFRAGs have enough headroom to accomodate the MPDU headers
		 * in the same packet. We do not need a new packet to be allocated
		 * for this. Let's try to reuse the same packet.
		 */
		ASSERT(headroom <= (uint)PKTHEADROOM(osh, p));

		/* Update the PKTTAG with AMSDU flag */
		pkttag = WLPKTTAG(p);
		ASSERT(!WLPKTFLAG_AMSDU(pkttag));
		pkttag->flags |= WLF_AMSDU;

		/*
		 * Prepend the |DA SA LEN| to the same packet. At the end of this
		 * operation, the packet would contain the following.
		 * +-----------------------------+-------------------+---------------------------+
		 * |A-MSDU header(ethernet like) | Subframe1 8023hdr | Subframe1 body(Host Frag) |
		 * +-----------------------------+-------------------+---------------------------+
		 */
		eh = (struct ether_header*) PKTPUSH(osh, p, ETHER_HDR_LEN);
		bcopy((char*)ea, eh->ether_dhost, ETHER_ADDR_LEN);
		bcopy((char*)&bsscfg->cur_etheraddr, eh->ether_shost, ETHER_ADDR_LEN);
		eh->ether_type = hton16(0);	/* no payload bytes yet */
		return p;
	}

	/* alloc new frame buffer */
	if ((pkt = PKTGET(osh, headroom, TRUE)) == NULL) {
		WL_ERROR(("wl%d: %s, PKTGET headroom %d failed\n",
			wlc->pub->unit, __FUNCTION__, headroom));
		WLCNTINCR(wlc->pub->_cnt->txnobuf);
		return NULL;
	}

	ASSERT(ISALIGNED(PKTDATA(osh, pkt), sizeof(uint32)));

	/* construct AMSDU frame as
	 * | DA SA LEN | Sub-Frame1 | Sub-Frame2 | ...
	 * its header is not converted to 8023hdr, it will be replaced by dot11 hdr directly
	 * the len is the totlen of the whole aggregated AMSDU, including padding
	 * need special flag for later differentiation
	 */


	/* adjust the data point for correct pkttotlength */
	PKTPULL(osh, pkt, headroom);
	PKTSETLEN(osh, pkt, 0);

	/* init ether_header */
	eh = (struct ether_header*) PKTPUSH(osh, pkt, ETHER_HDR_LEN);

	bcopy((char*)ea, eh->ether_dhost, ETHER_ADDR_LEN);
	bcopy((char*)&bsscfg->cur_etheraddr, eh->ether_shost, ETHER_ADDR_LEN);
	eh->ether_type = hton16(0);	/* no payload bytes yet */

	/* transfer pkttag, scb, add AMSDU flag */
	/* ??? how many are valid and should be transferred */
	wlc_pkttag_info_move(wlc, p, pkt);
	PKTSETPRIO(pkt, PKTPRIO(p));
	WLPKTTAGSCBSET(pkt, WLPKTTAGSCBGET(p));
	pkttag = WLPKTTAG(pkt);
	ASSERT(!WLPKTFLAG_AMSDU(pkttag));
	pkttag->flags |= WLF_AMSDU;

	return pkt;
} /* wlc_amsdu_agg_open */

/**
 * called by wlc_amsdu_agg(). May close and forward AMSDU to next software layer.
 *
 * return true on consumed, false others if
 *      -- first header buffer allocation failed
 *      -- no enough tailroom for pad bytes
 *      -- tot size goes beyond A-MSDU limit
 *
 *  amsdu_agg_p[tid] points to the header lbuf, amsdu_agg_ptail[tid] points to the tail lbuf
 *
 * The A-MSDU format typically will be below
 *   | A-MSDU header(ethernet like) |
 *	|subframe1 8023hdr |
 *		|subframe1 body | pad |
 *			|subframe2 8023hdr |
 *				|subframe2 body | pad |
 *					...
 *						|subframeN 8023hdr |
 *							|subframeN body |
 * It's not required to have pad bytes on the last frame
*/
static bool
wlc_amsdu_agg_append(amsdu_info_t *ami, struct scb *scb, void *p, uint tid,
	ratespec_t rspec)
{
	uint len, totlen;
	bool pad_end_supported = TRUE;
	osl_t *osh;
	scb_amsdu_t *scb_ami;
	void *ptid;
	amsdu_txaggstate_t *aggstate;
	uint max_agg_bytes;

	uint pkt_tail_room, pkt_head_room;
	uint8 headroom_pad_need, pad;
	WL_AMSDU(("%s\n", __FUNCTION__));

	osh = ami->pub->osh;
	pkt_tail_room = (uint)PKTTAILROOM(osh, pktlast(osh, p));
	pkt_head_room = (uint)PKTHEADROOM(osh, p);
	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	aggstate = &scb_ami->aggstate[tid];
	headroom_pad_need = aggstate->headroom_pad_need;
	max_agg_bytes = wlc_amsdu_get_max_agg_bytes(scb, scb_ami, tid, p, rspec);


	/* length of 802.3/LLC frame, equivalent to A-MSDU sub-frame length, DA/SA/Len/Data */
	len = pkttotlen(osh, p);
	/* padding of A-MSDU sub-frame to 4 bytes */
	pad = (uint)((-(int)len) & 3);
	/* START: check ok to append p to queue */
	/* if we have a pkt being agged */
	/* ensure that we can stick any padding needed on front of pkt */
	/* check here, coz later on we will append p on agg_p after adding pad */
	if (aggstate->amsdu_agg_p) {
		if (pkt_head_room < headroom_pad_need ||
			(len + headroom_pad_need + aggstate->amsdu_agg_bytes > max_agg_bytes)) {
				/* start over if we cant' make it */
				/* danger here is if no tailroom or headroom in packets */
				/* we will send out only 1 sdu long amsdu -- slow us down */
#ifdef WLCNT
				if (pkt_head_room < headroom_pad_need) {
					WLCNTINCR(ami->cnt->agg_stop_tailroom);
				} else {
					WLCNTINCR(ami->cnt->agg_stop_len);
				}
#endif /* WLCNT */
				wlc_amsdu_agg_close(ami, scb, tid); /* closes and sends AMPDU */
		}
	}

	/* END: check ok to append pkt to queue */
	/* START: allocate header */
	/* alloc new pack tx buffer if necessary */
	if (aggstate->amsdu_agg_p == NULL) {
		/* catch a common case of a stream of incoming packets with no tailroom */
		/* also throw away if headroom doesn't look like can accomodate */
		/* the assumption is that current pkt headroom is good gauge of next packet */
		/* and if both look insufiicient now, it prob will be insufficient later */
		if (pad > pkt_tail_room && pad > pkt_head_room) {
			WLCNTINCR(ami->cnt->agg_stop_tailroom);
			goto amsdu_agg_false;
		}

		if ((ptid = wlc_amsdu_agg_open(ami, SCB_BSSCFG(scb), &scb->ea, p)) == NULL) {
			WLCNTINCR(ami->cnt->agg_openfail);
			goto amsdu_agg_false;
		}

		aggstate->amsdu_agg_p = ptid;
		aggstate->amsdu_agg_ptail = ptid;
		aggstate->amsdu_agg_sframes = 0;
		aggstate->amsdu_agg_bytes = 0;
		aggstate->amsdu_agg_padlast = 0;
#ifdef WL_TXQ_STALL
		aggstate->timestamp = ami->wlc->pub->now;
#endif
		WL_AMSDU(("%s: open a new AMSDU, hdr %d bytes\n",
			__FUNCTION__, aggstate->amsdu_agg_bytes));
	} else {
		/* step 1: mod cur pkt to take care of prev pkt */
		if (headroom_pad_need) {
			PKTPUSH(osh, p, aggstate->headroom_pad_need);
			len += headroom_pad_need;
			aggstate->headroom_pad_need = 0;
#ifdef WLCNT
			WLCNTINCR(ami->cnt->tx_padding_in_head);
#endif /* WLCNT */
		}
	}
	/* END: allocate header */

	/* use short name for convenience */
	ptid = aggstate->amsdu_agg_p;

	/* START: append packet */
	/* step 2: chain the pkts at the end of current one */
	ASSERT(aggstate->amsdu_agg_ptail != NULL);

	/* If the AMSDU header was prepended in the same packet, we have */
	/* only one packet to work with and do not need any linking. */
	if (p != aggstate->amsdu_agg_ptail) {
		PKTSETNEXT(osh, aggstate->amsdu_agg_ptail, p);
		aggstate->amsdu_agg_ptail = pktlast(osh, p);

		/* Append any packet callbacks from p to *ptid */
		wlc_pcb_fn_move(ami->wlc->pcb, ptid, p);
	}

	/* step 3: update total length in agg queue */
	totlen = len + aggstate->amsdu_agg_bytes;
	/* caller already makes sure this frame fits */
	ASSERT(totlen < max_agg_bytes);

	/* END: append pkt */

	/* START: pad current
	 * If padding for this pkt (for 4 bytes alignment) is needed
	 * and feasible(enough tailroom and
	 * totlen does not exceed limit), then add it, adjust length and continue;
	 * Otherwise, close A-MSDU
	 */
	aggstate->amsdu_agg_padlast = 0;
	if (pad != 0) {
		if (BCMLFRAG_ENAB() && PKTISTXFRAG(osh, aggstate->amsdu_agg_ptail))
			pad_end_supported = FALSE;

		/* first try using tailroom -- append pad immediately */
		if (((uint)PKTTAILROOM(osh, aggstate->amsdu_agg_ptail) >= pad) &&
		    (totlen + pad < max_agg_bytes) && (pad_end_supported)) {
			aggstate->amsdu_agg_padlast = pad;
			PKTSETLEN(osh, aggstate->amsdu_agg_ptail,
				PKTLEN(osh, aggstate->amsdu_agg_ptail) + pad);
			totlen += pad;
#ifdef WLCNT
			WLCNTINCR(ami->cnt->tx_padding_in_tail);
#endif /* WLCNT */
		} else if (totlen + pad < max_agg_bytes) {
			/* next try using headroom -- wait til next pkt to take care of padding */
			aggstate->headroom_pad_need = pad;
		} else {
			WL_AMSDU(("%s: terminate A-MSDU for tailroom/aggmax\n", __FUNCTION__));
			aggstate->amsdu_agg_sframes++;
			aggstate->amsdu_agg_bytes = totlen;

#ifdef WLCNT
			if (totlen + pad < max_agg_bytes) {
				WLCNTINCR(ami->cnt->agg_stop_len);
			} else {
				WLCNTINCR(ami->cnt->agg_stop_tailroom);
			}
#endif /* WLCNT */
			wlc_amsdu_agg_close(ami, scb, tid);
			goto amsdu_agg_true;
		}
	}
#ifdef WLCNT
	else {
		WLCNTINCR(ami->cnt->tx_padding_no_pad);
	}
#endif /* WLCNT */
	/* END: pad current */
	/* sync up agg counter */
	WL_AMSDU(("%s: add one more frame len %d pad %d\n", __FUNCTION__, len, pad));
	ASSERT(totlen == (pkttotlen(osh, ptid) - ETHER_HDR_LEN));
	aggstate->amsdu_agg_sframes++;
	aggstate->amsdu_agg_bytes = totlen;

amsdu_agg_true:
	return (TRUE);

amsdu_agg_false:
	return (FALSE);
} /* wlc_amsdu_agg_append */

void
wlc_amsdu_agg_flush(amsdu_info_t *ami)
{
	wlc_info_t *wlc;
	uint i;
	struct scb *scb;
	struct scb_iter scbiter;
	scb_amsdu_t *scb_ami;
	amsdu_txaggstate_t *aggstate;

	if (!AMSDU_TX_SUPPORT(ami->pub))
		return;

	WL_AMSDU(("wlc_amsdu_agg_flush\n"));

	wlc = ami->wlc;
	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		for (i = 0; i < NUMPRIO; i++) {
			scb_ami = SCB_AMSDU_CUBBY(ami, scb);
			aggstate = &scb_ami->aggstate[i];

			if (aggstate->amsdu_agg_p) {
				PKTFREE(ami->wlc->osh, scb_ami->aggstate[i].amsdu_agg_p, TRUE);
			}

			aggstate->amsdu_agg_p = NULL;
			aggstate->amsdu_agg_ptail = NULL;
			aggstate->amsdu_agg_sframes = 0;
			aggstate->amsdu_agg_bytes = 0;
			aggstate->amsdu_agg_padlast = 0;
			aggstate->headroom_pad_need = 0;
		}
	}
}
#endif /* TXQ_MUX */

#if defined(WLOVERTHRUSTER)
/**
 * Transmit throughput enhancement, identifies TCP ACK frames to skip AMSDU agg.
 * WLOVERTHRUSTER specific.
 */
static bool
wlc_amsdu_is_tcp_ack(amsdu_info_t *ami, void *p)
{
	uint8 *ip_header;
	uint8 *tcp_header;
	uint32 eth_len;
	uint32 ip_hdr_len;
	uint32 ip_len;
	uint32 pktlen;
	uint16 ethtype;
	wlc_info_t *wlc = ami->wlc;
	osl_t *osh = wlc->osh;

	pktlen = pkttotlen(osh, p);

	/* make sure we have enough room for a minimal IP + TCP header */
	if (pktlen < (ETHER_HDR_LEN +
	              DOT11_LLC_SNAP_HDR_LEN +
	              IPV4_MIN_HEADER_LEN +
	              TCP_MIN_HEADER_LEN)) {
		return FALSE;
	}

	/* find length of ether payload */
	eth_len = pktlen - (ETHER_HDR_LEN + DOT11_LLC_SNAP_HDR_LEN);

	/* bail out early if pkt is too big for an ACK
	 *
	 * A TCP ACK has only TCP header and no data.  Max size for both IP header and TCP
	 * header is 15 words, 60 bytes. So if the ether payload is more than 120 bytes, we can't
	 * possibly have a TCP ACK. This test optimizes an early exit for MTU sized TCP.
	 */
	if (eth_len > 120) {
		return FALSE;
	}

	ethtype = wlc_sdu_etype(wlc, p);
	if (ethtype != ntoh16(ETHER_TYPE_IP)) {
		return FALSE;
	}

	/* find protocol headers and actual IP lengths */
	ip_header = wlc_sdu_data(wlc, p);
	ip_hdr_len = IPV4_HLEN(ip_header);
	ip_len = ntoh16_ua(&ip_header[IPV4_PKTLEN_OFFSET]);

	/* check for IP VER4 and carrying TCP */
	if (IP_VER(ip_header) != IP_VER_4 ||
	    IPV4_PROT(ip_header) != IP_PROT_TCP) {
		return FALSE;
	}

	/* verify pkt len in case of ip hdr has options */
	if (eth_len < ip_hdr_len + TCP_MIN_HEADER_LEN) {
		return FALSE;
	}

	tcp_header = ip_header + ip_hdr_len;

	/* fail if no TCP ACK flag or payload bytes present
	 * (payload bytes are present if IP len is not eq to IP header + TCP header)
	 */
	if ((tcp_header[TCP_FLAGS_OFFSET] & TCP_FLAG_ACK) == 0 ||
	    ip_len != ip_hdr_len + 4 * TCP_HDRLEN(tcp_header[TCP_HLEN_OFFSET])) {
		return FALSE;
	}

	return TRUE;
} /* wlc_amsdu_is_tcp_ack */
#endif /* WLOVERTHRUSTER */

#ifndef TXQ_MUX
/* Part of TXMOD functions, not used by TXQ_MUX */

/** Return the transmit packets held by AMSDU */
static uint
wlc_amsdu_txpktcnt(void *ctx)
{
	amsdu_info_t *ami = (amsdu_info_t *)ctx;
	uint i;
	scb_amsdu_t *scb_ami;
	int pktcnt = 0;
	struct scb_iter scbiter;
	wlc_info_t *wlc = ami->wlc;
	struct scb *scb;

	FOREACHSCB(wlc->scbstate, &scbiter, scb)
		if (SCB_AMSDU(scb)) {
			scb_ami = SCB_AMSDU_CUBBY(ami, scb);
			for (i = 0; i < NUMPRIO; i++) {
				if (scb_ami->aggstate[i].amsdu_agg_p)
					pktcnt++;
			}
		}

	return pktcnt;
}

static void
wlc_amsdu_scb_deactive(void *ctx, struct scb *scb)
{
	amsdu_info_t *ami;
	uint i;
	scb_amsdu_t *scb_ami;

	WL_AMSDU(("wlc_amsdu_scb_deactive scb %p\n", OSL_OBFUSCATE_BUF(scb)));

	ami = (amsdu_info_t *)ctx;
	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	for (i = 0; i < NUMPRIO; i++) {

		if (scb_ami->aggstate[i].amsdu_agg_p)
			PKTFREE(ami->wlc->osh, scb_ami->aggstate[i].amsdu_agg_p, TRUE);

		scb_ami->aggstate[i].amsdu_agg_p = NULL;
		scb_ami->aggstate[i].amsdu_agg_ptail = NULL;
		scb_ami->aggstate[i].amsdu_agg_sframes = 0;
		scb_ami->aggstate[i].amsdu_agg_bytes = 0;
		scb_ami->aggstate[i].amsdu_agg_padlast = 0;
	}
}

#endif /* !TXQ_MUX */
#endif /* WLAMSDU_TX */


/**
 * We should not come here typically!!!!
 * if we are here indicates we received a corrupted packet which is tagged as AMSDU by the ucode.
 * So flushing invalid AMSDU chain. When anything other than AMSDU is received when AMSDU state is
 * not idle, flush the collected intermediate amsdu packets.
 */
void BCMFASTPATH
wlc_amsdu_flush(amsdu_info_t *ami)
{
	int fifo;
	amsdu_deagg_t *deagg;

	for (fifo = 0; fifo < RX_FIFO_NUMBER; fifo++) {
		deagg = &ami->amsdu_deagg[fifo];
		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_IDLE)
			wlc_amsdu_deagg_flush(ami, fifo);
	}
}

/**
 * return FALSE if filter failed
 *   caller needs to toss all buffered A-MSDUs and p
 *   Enhancement: in case of out of sequences, try to restart to
 *     deal with lost of last MSDU, which can occur frequently due to fcs error
 *   Assumes receive status is in host byte order at this point.
 *   PKTDATA points to start of receive descriptor when called.
 */
void * BCMFASTPATH
wlc_recvamsdu(amsdu_info_t *ami, wlc_d11rxhdr_t *wrxh, void *p, uint16 *padp, bool chained_sendup)
{
	osl_t *osh;
	amsdu_deagg_t *deagg;
	uint aggtype;
	int fifo;
	bool pad_present;               /* TRUE if headroom is padded for alignment */
	uint16 pad = 0;                 /* Number of bytes of pad */
	wlc_info_t *wlc = ami->wlc;
	uint16 *mrxs;

	/* packet length starting at 802.11 mac header (first frag) or eth header (others) */
	uint32 pktlen;

	uint32 pktlen_w_plcp;           /* packet length starting at PLCP */
	struct dot11_header *h;
#ifdef BCMDBG
	int msdu_cnt = -1;              /* just used for debug */
#endif

#if !defined(PKTC) && !defined(PKTC_DONGLE)
	BCM_REFERENCE(chained_sendup);
#endif

	osh = ami->pub->osh;

	ASSERT(padp != NULL);

	if (RXS_SHORT_ENAB(ami->pub->corerev) &&
		((D11RXHDR_ACCESS_VAL(&wrxh->rxhdr, ami->pub->corerev, dma_flags)) &
		RXS_SHORT_MASK)) {
		mrxs = wlc_get_mrxs(wlc, &wrxh->rxhdr);
		aggtype = (*mrxs & RXSS_AGGTYPE_MASK) >>
			RXSS_AGGTYPE_SHIFT;
		pad_present = ((*mrxs & RXSS_PBPRES) != 0);
#ifdef BCMDBG
		msdu_cnt = (*mrxs & RXSS_MSDU_CNT_MASK) >>
			RXSS_MSDU_CNT_SHIFT;
#endif  /* BCMDBG */
	} else {
		aggtype = (((D11RXHDR_ACCESS_VAL(&wrxh->rxhdr, ami->pub->corerev,
			RxStatus2)) & RXS_AGGTYPE_MASK) >> RXS_AGGTYPE_SHIFT);
		pad_present = (((D11RXHDR_ACCESS_VAL(&wrxh->rxhdr, ami->pub->corerev,
			RxStatus1)) & RXS_PBPRES) != 0);
	}
	if (pad_present) {
		pad = 2;
	}

	/* PKTDATA points to rxh. Get length of packet w/o rxh, but incl plcp */
	pktlen_w_plcp = PKTLEN(osh, p) - (ami->wlc->hwrxoff + pad);

	/* Packet length w/o PLCP */
	pktlen = pktlen_w_plcp - D11_PHY_HDR_LEN;

	fifo = (D11RXHDR_ACCESS_VAL(&wrxh->rxhdr, ami->pub->corerev, fifo));
	ASSERT((fifo < RX_FIFO_NUMBER));
	deagg = &ami->amsdu_deagg[fifo];

	WLCNTINCR(ami->cnt->deagg_msdu);


	WL_AMSDU(("wlc_recvamsdu: aggtype %d, msdu count %d\n", aggtype, msdu_cnt));

	h = (struct dot11_header *)(PKTDATA(osh, p) + ami->wlc->hwrxoff + pad + D11_PHY_HDR_LEN);

	switch (aggtype) {
	case RXS_AMSDU_FIRST:
		/* PKTDATA starts with PLCP */
		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_IDLE) {
			WL_AMSDU(("wlc_recvamsdu: wrong A-MSDU deagg sequence, cur_state=%d\n",
				deagg->amsdu_deagg_state));
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			wlc_amsdu_deagg_flush(ami, fifo);
			/* keep this valid one and reset to improve throughput */
		}

		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_FIRST;

		/* Store the frontpad value of the first subframe */
		deagg->first_pad = *padp;

		if (!wlc_amsdu_deagg_open(ami, fifo, p, h, pktlen_w_plcp)) {
			goto abort;
		}

		WL_AMSDU(("wlc_recvamsdu: first A-MSDU buffer\n"));
		break;

	case RXS_AMSDU_INTERMEDIATE:
		/* PKTDATA starts with subframe header */
		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_FIRST) {
			WL_AMSDU_ERROR(("%s: wrong A-MSDU deagg sequence, cur_state=%d\n",
				__FUNCTION__, deagg->amsdu_deagg_state));
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			goto abort;
		}

#ifdef ASSERT
		/* intermediate frames should have 2 byte padding if wlc->hwrxoff is aligned
		* on mod 4 address
		*/
		if ((ami->wlc->hwrxoff % 4) == 0)
			ASSERT(pad_present);
		else
			ASSERT(!pad_present);
#endif /* ASSERT */

		if ((pktlen) <= ETHER_HDR_LEN) {
			WL_AMSDU_ERROR(("%s: rxrunt\n", __FUNCTION__));
			WLCNTINCR(ami->pub->_cnt->rxrunt);
			goto abort;
		}

		ASSERT(deagg->amsdu_deagg_ptail);
		PKTSETNEXT(osh, deagg->amsdu_deagg_ptail, p);
		deagg->amsdu_deagg_ptail = p;
		WL_AMSDU(("wlc_recvamsdu:   mid A-MSDU buffer\n"));
		break;
	case RXS_AMSDU_LAST:
		/* PKTDATA starts with last subframe header */
		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_FIRST) {
			WL_AMSDU_ERROR(("%s: wrong A-MSDU deagg sequence, cur_state=%d\n",
				__FUNCTION__, deagg->amsdu_deagg_state));
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			goto abort;
		}

		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_LAST;

#ifdef ASSERT
		/* last frame should have 2 byte padding if wlc->hwrxoff is aligned
		* on mod 4 address
		*/
		if ((ami->wlc->hwrxoff % 4) == 0)
			ASSERT(pad_present);
		else
			ASSERT(!pad_present);
#endif /* ASSERT */

		if ((pktlen) < (ETHER_HDR_LEN + DOT11_FCS_LEN)) {
			WL_AMSDU_ERROR(("%s: rxrunt\n", __FUNCTION__));
			WLCNTINCR(ami->pub->_cnt->rxrunt);
			goto abort;
		}

		ASSERT(deagg->amsdu_deagg_ptail);
		PKTSETNEXT(osh, deagg->amsdu_deagg_ptail, p);
		deagg->amsdu_deagg_ptail = p;
		WL_AMSDU(("wlc_recvamsdu: last A-MSDU buffer\n"));
		break;


	case RXS_AMSDU_N_ONE:
		/* this frame IS AMSDU, checked by caller */

		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_IDLE) {
			WL_AMSDU(("wlc_recvamsdu: wrong A-MSDU deagg sequence, cur_state=%d\n",
				deagg->amsdu_deagg_state));
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			wlc_amsdu_deagg_flush(ami, fifo);

			/* keep this valid one and reset to improve throughput */
		}

		ASSERT((deagg->amsdu_deagg_p == NULL) && (deagg->amsdu_deagg_ptail == NULL));
		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_LAST;

		/* Store the frontpad value of this single subframe */
		deagg->first_pad = *padp;

		if (!wlc_amsdu_deagg_open(ami, fifo, p, h, pktlen_w_plcp)) {
			goto abort;
		}

		break;

	default:
		/* can't be here */
		ASSERT(0);
		goto abort;
	}

	/* Note that pkttotlen now includes the length of the rxh for each frag */
	WL_AMSDU(("wlc_recvamsdu: add one more A-MSDU buffer %d bytes, accumulated %d bytes\n",
	         pktlen_w_plcp, pkttotlen(osh, deagg->amsdu_deagg_p)));

	if (deagg->amsdu_deagg_state == WLC_AMSDU_DEAGG_LAST) {
		void *pp;
		pp = deagg->amsdu_deagg_p;
		deagg->amsdu_deagg_p = deagg->amsdu_deagg_ptail = NULL;
		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_IDLE;

		/* ucode/hw deagg happened */
		WLPKTTAG(pp)->flags |= WLF_HWAMSDU;

		/* First frame has fully defined Receive Frame Header,
		 * handle it to normal MPDU process.
		 */
		WLCNTINCR(ami->pub->_cnt->rxfrag);
		WLCNTINCR(ami->cnt->deagg_amsdu);

#ifdef WL_MU_RX
		/* Update the murate to the plcp
		 * last rxhdr has murate information
		 * plcp is in the first packet
		 */
		if (MU_RX_ENAB(ami->wlc)) {
			wlc_d11rxhdr_t *frag_wrxh;   /* rx status of an AMSDU frag in chain */
			d11rxhdr_t *rxh;
			uchar *plcp;
			uint16 pad_cnt;
			frag_wrxh = (wlc_d11rxhdr_t *)PKTDATA(osh, pp);
			rxh = &frag_wrxh->rxhdr;

			/* Check for short or long format */
			if ((D11RXHDR_ACCESS_VAL(rxh, ami->pub->corerev,
				dma_flags)) & RXS_SHORT_MASK) {
				/* short rx status received */
				mrxs = wlc_get_mrxs(wlc, rxh);
				pad_cnt = (*mrxs & RXSS_PBPRES) ? 2 : 0;
			} else {
				/* long rx status received */
				pad_cnt = (((D11RXHDR_ACCESS_VAL(rxh, ami->pub->corerev,
					RxStatus1)) & RXS_PBPRES) ? 2 : 0);
			}

			plcp = (uchar *)(PKTDATA(ami->wlc->osh, pp) + ami->wlc->hwrxoff + pad_cnt);

			wlc_bmac_upd_murate(ami->wlc, &wrxh->rxhdr, plcp);
		}
#endif /* WL_MU_RX */

#if defined(PKTC) || defined(PKTC_DONGLE)
		/* if chained sendup, return back head pkt and front padding of first sub-frame */
		/* if unchained wlc_recvdata takes pkt till bus layer */
		if (chained_sendup == TRUE) {
			*padp = deagg->first_pad;
			deagg->first_pad = 0;
			return (pp);
		} else
#endif
		{
			/* Strip rxh from all amsdu frags in amsdu chain before send up */
			void *np = pp;
			uint16 pad_cnt;
			wlc_d11rxhdr_t *frag_wrxh;   /* rx status of an AMSDU frag in chain */
			d11rxhdr_t *rxh;

			if (!RXS_SHORT_ENAB(ami->pub->corerev)) {
				/* Point to correct rx header instead of
				 * using last frames rx header
				 */
				wrxh = (wlc_d11rxhdr_t *)PKTDATA(osh, pp);
			}

			while (np) {
				frag_wrxh = (wlc_d11rxhdr_t*) PKTDATA(osh, np);
				rxh = &frag_wrxh->rxhdr;

				if (RXS_SHORT_ENAB(ami->pub->corerev) &&
					((D11RXHDR_ACCESS_VAL(rxh,
					ami->pub->corerev, dma_flags)) & RXS_SHORT_MASK)) {
					/* short rx status received */
					mrxs = wlc_get_mrxs(wlc, rxh);
					pad_cnt = (*mrxs & RXSS_PBPRES) ? 2 : 0;
				} else {
					/* long rx status received */
					pad_cnt = (((D11RXHDR_ACCESS_VAL(rxh,
					ami->pub->corerev, RxStatus1)) & RXS_PBPRES) ? 2 : 0);
				}
				PKTPULL(osh, np, ami->wlc->hwrxoff + pad_cnt);
				np = PKTNEXT(osh, np);
			}
			wlc_recvdata(ami->wlc, ami->pub->osh, wrxh, pp);
		}
		deagg->first_pad = 0;
	}

	/* all other cases needs no more action, just return */
	return  NULL;

abort:
	wlc_amsdu_deagg_flush(ami, fifo);
	PKTFREE(osh, p, FALSE);
	return  NULL;
} /* wlc_recvamsdu */

/** return FALSE if A-MSDU verification failed */
static bool BCMFASTPATH
wlc_amsdu_deagg_verify(amsdu_info_t *ami, uint16 fc, void *h)
{
	bool is_wds;
	uint16 *pqos;
	uint16 qoscontrol;

	BCM_REFERENCE(ami);

	/* it doesn't make sense to aggregate other type pkts, toss them */
	if ((fc & FC_KIND_MASK) != FC_QOS_DATA) {
		WL_AMSDU(("wlc_amsdu_deagg_verify fail: fc 0x%x is not QoS data type\n", fc));
		return FALSE;
	}

	is_wds = ((fc & (FC_TODS | FC_FROMDS)) == (FC_TODS | FC_FROMDS));
	pqos = (uint16*)((uchar*)h + (is_wds ? DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN));
	qoscontrol = ltoh16_ua(pqos);

	if (qoscontrol & QOS_AMSDU_MASK)
		return TRUE;

	WL_AMSDU_ERROR(("%s fail: qos field 0x%x\n", __FUNCTION__, *pqos));
	return FALSE;
}

/**
 * Start a new AMSDU receive chain. Verifies that the frame is a data frame
 * with QoS field indicating AMSDU, and that the frame is long enough to
 * include PLCP, 802.11 mac header, QoS field, and AMSDU subframe header.
 * Inputs:
 *   ami    - AMSDU state
 *   fifo   - queue on which current frame was received
 *   p      - first frag in a sequence of AMSDU frags. PKTDATA(p) points to
 *            start of receive descriptor
 *   h      - start of ethernet header in received frame
 *   pktlen - frame length, starting at PLCP
 *
 * Returns:
 *   TRUE if new AMSDU chain is started
 *   FALSE otherwise
 */
static bool BCMFASTPATH
wlc_amsdu_deagg_open(amsdu_info_t *ami, int fifo, void *p, struct dot11_header *h, uint32 pktlen)
{
	osl_t *osh = ami->pub->osh;
	amsdu_deagg_t *deagg = &ami->amsdu_deagg[fifo];
	uint16 fc;

	BCM_REFERENCE(osh);

	if (pktlen < D11_PHY_HDR_LEN + DOT11_MAC_HDR_LEN + DOT11_QOS_LEN + ETHER_HDR_LEN) {
		WL_AMSDU(("%s: rxrunt\n", __FUNCTION__));
		WLCNTINCR(ami->pub->_cnt->rxrunt);
		goto fail;
	}

	fc = ltoh16(h->fc);

	if (!wlc_amsdu_deagg_verify(ami, fc, h)) {
		WL_AMSDU(("%s: AMSDU verification failed, toss\n", __FUNCTION__));
		WLCNTINCR(ami->cnt->deagg_badfmt);
		goto fail;
	}

	/* explicitly test bad src address to avoid sending bad deauth */
	if ((ETHER_ISNULLADDR(&h->a2) || ETHER_ISMULTI(&h->a2))) {
		WL_AMSDU(("%s: wrong address 2\n", __FUNCTION__));
		WLCNTINCR(ami->pub->_cnt->rxbadsrcmac);
		goto fail;
	}

	deagg->amsdu_deagg_p = p;
	deagg->amsdu_deagg_ptail = p;
	return TRUE;

fail:
	WLCNTINCR(ami->cnt->deagg_openfail);
	return FALSE;
} /* wlc_amsdu_deagg_open */

static void BCMFASTPATH
wlc_amsdu_deagg_flush(amsdu_info_t *ami, int fifo)
{
	amsdu_deagg_t *deagg = &ami->amsdu_deagg[fifo];
	WL_AMSDU(("%s\n", __FUNCTION__));

	if (deagg->amsdu_deagg_p)
		PKTFREE(ami->pub->osh, deagg->amsdu_deagg_p, FALSE);

	deagg->first_pad = 0;
	deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_IDLE;
	deagg->amsdu_deagg_p = deagg->amsdu_deagg_ptail = NULL;
#ifdef WLCNT
	WLCNTINCR(ami->cnt->deagg_flush);
#endif /* WLCNT */
}

static void
wlc_amsdu_to_dot11(amsdu_info_t *ami, struct scb *scb, struct dot11_header *hdr, void *pkt)
{
	osl_t *osh;

	/* ptr to 802.3 or eh in incoming pkt */
	struct ether_header *eh;
	struct dot11_llc_snap_header *lsh;

	/* ptr to 802.3 or eh in new pkt */
	struct ether_header *neh;
	struct dot11_header *phdr;

	wlc_bsscfg_t *cfg = scb->bsscfg;
	osh = ami->pub->osh;

	/* If we discover an ethernet header, replace it by an 802.3 hdr + SNAP header */
	eh = (struct ether_header *)PKTDATA(osh, pkt);

	if (ntoh16(eh->ether_type) > ETHER_MAX_LEN) {
		neh = (struct ether_header *)PKTPUSH(osh, pkt, DOT11_LLC_SNAP_HDR_LEN);

		/* Avoid constructing 802.3 header as optimization.
		 * 802.3 header(14 bytes) is going to be overwritten by the 802.11 header.
		 * This will save writing 14-bytes for every MSDU.
		 */

		/* Construct LLC SNAP header */
		lsh = (struct dot11_llc_snap_header *)
			((char *)neh + ETHER_HDR_LEN);
		lsh->dsap = 0xaa;
		lsh->ssap = 0xaa;
		lsh->ctl = 0x03;
		lsh->oui[0] = 0;
		lsh->oui[1] = 0;
		lsh->oui[2] = 0;
		/* The snap type code is already in place, inherited from the ethernet header that
		 * is now overlaid.
		 */
	}
	else {
		neh = (struct ether_header *)PKTDATA(osh, pkt);
	}

	if (BSSCFG_AP(cfg)) {
		/* Force the 802.11 a2 address to be the ethernet source address */
		bcopy((char *)neh->ether_shost,
			(char *)&hdr->a2, ETHER_ADDR_LEN);
	} else {
		if (!cfg->BSS) {
			/* Force the 802.11 a3 address to be the ethernet source address
			 * IBSS has BSS as a3, so leave a3 alone for win7+
			 */
			bcopy((char *)neh->ether_shost,
				(char *)&hdr->a3, ETHER_ADDR_LEN);
		}
	}

	/* Replace the 802.3 header, if present, by an 802.11 header. The original 802.11 header
	 * was appended to the frame along with the receive data needed by Microsoft.
	 */
	phdr = (struct dot11_header *)
		PKTPUSH(osh, pkt, DOT11_A3_HDR_LEN - ETHER_HDR_LEN);

	bcopy((char *)hdr, (char *)phdr, DOT11_A3_HDR_LEN);

	/* Clear all frame control bits except version, type, data subtype & from-ds/to-ds */
	phdr->fc = htol16(ltoh16(phdr->fc) & (FC_FROMDS | FC_TODS | FC_TYPE_MASK |
		(FC_SUBTYPE_MASK & ~QOS_AMSDU_MASK) | FC_PVER_MASK));
} /* wlc_amsdu_to_dot11 */

/**
 * Called when the 'WLF_HWAMSDU' flag is set in the PKTTAG of a received frame.
 * A-MSDU decomposition: break A-MSDU(chained buffer) to individual buffers
 *
 *    | 80211 MAC HEADER | subFrame 1 |
 *			               --> | subFrame 2 |
 *			                                 --> | subFrame 3... |
 * where, 80211 MAC header also includes QOS and/or IV fields
 *        f->pbody points to beginning of subFrame 1,
 *        f->totlen is the total body len(chained, after mac/qos/iv header) w/o icv and FCS
 *
 *        each subframe is in the form of | 8023hdr | body | pad |
 *                subframe other than the last one may have pad bytes
*/
void
wlc_amsdu_deagg_hw(amsdu_info_t *ami, struct scb *scb, struct wlc_frminfo *f)
{
	osl_t *osh;
	void *sf[MAX_RX_SUBFRAMES], *newpkt;
	struct ether_header *eh;
	uint16 body_offset, sflen = 0, len = 0;
	uint num_sf = 0, i;
	int resid;
	wlc_pkttag_t * pkttag = WLPKTTAG(f->p);
	struct dot11_header hdr_copy;
	wlc_rx_status_t toss_reason = WLC_RX_STS_TOSS_UNKNOWN;

	if (WLEXTSTA_ENAB(ami->wlc->pub)) {
		/* Save the header before being overwritten */
		bcopy((char *)f->h, (char *)&hdr_copy, DOT11_A3_HDR_LEN);

		/* Assume f->h pointer is not valid any more */
		f->h = NULL;
	}

	ASSERT(pkttag->flags & WLF_HWAMSDU);
	osh = ami->pub->osh;

	/* strip mac header, move to start from A-MSDU body */
	body_offset = (uint)(f->pbody - (uchar*)PKTDATA(osh, f->p));
	PKTPULL(osh, f->p, body_offset);

	WL_AMSDU(("wlc_amsdu_deagg_hw: body_len(exclude icv and FCS) %d\n", f->totlen));

	resid = f->totlen;
	newpkt = f->p;
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	WLCNTINCR(ami->amdbg->rx_length_histogram[
		MIN(PKTLEN(osh, f->p)/AMSDU_LENGTH_BIN_BYTES,
		AMSDU_LENGTH_BINS-1)]);
#endif /* BCMDBG */

	/* break chained AMSDU into N independent MSDU */
	while (newpkt != NULL) {
		/* there must be a limit to stop in order to prevent memory/stack overflow */
		if (num_sf >= MAX_RX_SUBFRAMES) {
			WL_AMSDU_ERROR(("%s: more than %d MSDUs !\n", __FUNCTION__, num_sf));
			break;
		}

		/* each subframe is 802.3 frame */
		eh = (struct ether_header*) PKTDATA(osh, newpkt);

		len = (uint16)(PKTLEN(osh, newpkt) + PKTFRAGUSEDLEN(osh, newpkt));

#ifdef BCMPCIEDEV
		if (BCMPCIEDEV_ENAB() && PKTISHDRCONVTD(osh, newpkt)) {
			/* Skip header conversion */
		        goto skip_conv;
		}
#endif /* BCMPCIEDEV */

		sflen = ntoh16(eh->ether_type) + ETHER_HDR_LEN;

		if ((((uintptr)eh + (uint)ETHER_HDR_LEN) % 4)  != 0) {
			WL_AMSDU_ERROR(("%s: sf body is not 4 bytes aligned!\n", __FUNCTION__));
			WLCNTINCR(ami->cnt->deagg_badsfalign);
			toss_reason = WLC_RX_STS_TOSS_DEAGG_UNALIGNED;
			goto toss;
		}

		/* last MSDU: has FCS, but no pad, other MSDU: has pad, but no FCS */
		if (len != (PKTNEXT(osh, newpkt) ? ROUNDUP(sflen, 4) : sflen)) {
			WL_AMSDU_ERROR(("%s: len mismatch buflen %d sflen %d, sf %d\n",
				__FUNCTION__, len, sflen, num_sf));
			WLCNTINCR(ami->cnt->deagg_badsflen);
			toss_reason = WLC_RX_STS_TOSS_BAD_DEAGG_SF_LEN;
			goto toss;
		}

		/* strip trailing optional pad */
		if (PKTFRAGUSEDLEN(osh, newpkt)) {
			/* set total length to sflen */
			PKTSETFRAGUSEDLEN(osh, newpkt, (sflen - PKTLEN(osh, newpkt)));
		} else {
			PKTSETLEN(osh, newpkt, sflen);
		}

		if (WLEXTSTA_ENAB(ami->wlc->pub)) {
			/* convert 802.3 to 802.11 */
			wlc_amsdu_to_dot11(ami, scb, &hdr_copy, newpkt);

			if (f->p != newpkt) {
				wlc_pkttag_t * new_pkttag = WLPKTTAG(newpkt);

				new_pkttag->rxchannel = pkttag->rxchannel;
				new_pkttag->pktinfo.misc.rssi = pkttag->pktinfo.misc.rssi;
				new_pkttag->rspec = pkttag->rspec;
			}
		} else {
			/* convert 8023hdr to ethernet if necessary */
			wlc_8023_etherhdr(ami->wlc, osh, newpkt);
		}
#ifdef BCMPCIEDEV
skip_conv:
#endif /* BCMPCIEDEV */
		/* propagate prio, NO need to transfer other tags, it's plain stack packet now */
		PKTSETPRIO(newpkt, f->prio);

		WL_AMSDU(("wlc_amsdu_deagg_hw: deagg MSDU buffer %d, frame %d\n", len, sflen));

		sf[num_sf] = newpkt;
		num_sf++;
		newpkt = PKTNEXT(osh, newpkt);

		resid -= len;
	}

	if (resid != 0) {
		ASSERT(0);
		WLCNTINCR(ami->cnt->deagg_badtotlen);
		toss_reason = WLC_RX_STS_TOSS_BAD_DEAGG_SF_LEN;
		goto toss;
	}

	/* cut the chain: set PKTNEXT to NULL */
	for (i = 0; i < num_sf; i++)
		PKTSETNEXT(osh, sf[i], NULL);

	/* toss the remaining MSDU, which we couldn't handle */
	if (newpkt != NULL) {
		WL_AMSDU_ERROR(("%s: toss MSDUs > %d !\n", __FUNCTION__, num_sf));
		PKTFREE(osh, newpkt, FALSE);
	}

	/* forward received data in host direction */
	for (i = 0; i < num_sf; i++) {
		struct ether_addr * ea;

		WL_AMSDU(("wlc_amsdu_deagg_hw: sendup subframe %d\n", i));

		if (WLEXTSTA_ENAB(ami->wlc->pub)) {
			ea = &hdr_copy.a1;
		} else {
			ea = (struct ether_addr *) PKTDATA(osh, sf[i]);
		}
		eh = (struct ether_header*)PKTDATA(osh, sf[i]);
		if ((ntoh16(eh->ether_type) == ETHER_TYPE_802_1X)) {
#ifdef BCMSPLITRX
			if ((BCMSPLITRX_ENAB()) && i &&
				(PKTFRAGUSEDLEN(ami->wlc->osh, sf[i]) > 0)) {
				/* Fetch susequent subframes */
				f->p = sf[i];
				wlc_recvdata_schedule_pktfetch(ami->wlc, scb, f, FALSE, TRUE, TRUE);
				continue;
			}
#endif /* BCMSPLITRX */
			/* Call process EAP frames */
			if (wlc_process_eapol_frame(ami->wlc, scb->bsscfg, scb, f, sf[i])) {
				/* We have consumed the pkt drop and continue; */
				WL_AMSDU(("Processed First fetched msdu %p\n", (void *)sf[i]));
				PKTFREE(osh, sf[i], FALSE);
				continue;
			}
		}

		wlc_recvdata_sendup(ami->wlc, scb, f->wds, ea, sf[i]);
	}

	WL_AMSDU(("wlc_amsdu_deagg_hw: this A-MSDU has %d MSDU, done\n", num_sf));
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	WLCNTINCR(ami->amdbg->rx_msdu_histogram[MIN(num_sf, AMSDU_RX_SUBFRAMES_BINS-1)]);
#endif /* BCMDBG */

	return;

toss:
	wlc_log_unexpected_rx_frame_log_80211hdr(ami->wlc, f->h, toss_reason);
#ifdef WL_RX_STALL
	wlc_rx_healthcheck_update_counters(ami->wlc, f->ac, scb, scb->bsscfg, toss_reason, 1);
#endif
	/*
	 * toss the whole A-MSDU since we don't know where the error starts
	 *  e.g. a wrong subframe length for mid frame can slip through the ucode
	 *       and the only syptom may be the last MSDU frame has the mismatched length.
	 */
	for (i = 0; i < num_sf; i++)
		sf[i] = NULL;

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	WLCNTINCR(ami->amdbg->rx_msdu_histogram[0]);
#endif /* BCMDBG */
	WL_AMSDU(("%s: tossing amsdu in deagg -- error seen\n", __FUNCTION__));
	PKTFREE(osh, f->p, FALSE);
} /* wlc_amsdu_deagg_hw */


#if defined(PKTC) || defined(PKTC_DONGLE)
/** Packet chaining (pktc) specific AMSDU receive function */
int32 BCMFASTPATH
wlc_amsdu_pktc_deagg_hw(amsdu_info_t *ami, void **pp, wlc_rfc_t *rfc, uint16 *index,
                        bool *chained_sendup, struct scb *scb, uint16 sec_offset)
{
	osl_t *osh;
	wlc_info_t *wlc = ami->wlc;
	void *newpkt, *head, *tail, *tmp_next;
	struct ether_header *eh;
	uint16 sflen = 0, len = 0;
	uint16 num_sf = 0;
	int resid = 0;
	uint8 *da;
	struct dot11_header hdr_copy;
	char *start;
	wlc_pkttag_t * pkttag  = NULL;

	wlc_bsscfg_t *bsscfg = NULL;
	osh = ami->pub->osh;
	newpkt = tail = head = *pp;
	resid = pkttotlen(osh, head);

	/* converted frame doesnt have FCS bytes */
	if (!PKTISHDRCONVTD(osh, head))
		resid -= DOT11_FCS_LEN;

	bsscfg = SCB_BSSCFG(scb);

	if (bsscfg->wlcif->if_flags & WLC_IF_PKT_80211) {
		ASSERT((sizeof(hdr_copy) + sec_offset) < (uint16)PKTHEADROOM(osh, newpkt));
		start = (char *)(PKTDATA(osh, newpkt) - sizeof(hdr_copy) - sec_offset);
		pkttag = (wlc_pkttag_t *)start;
		/* Save the header before being overwritten */
		bcopy((char *)start + sizeof(rx_ctxt_t), (char *)&hdr_copy, DOT11_A3_HDR_LEN);

	}


#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	len = MIN(PKTLEN(osh, head)/AMSDU_LENGTH_BIN_BYTES, AMSDU_LENGTH_BINS-1);
	WLCNTINCR(ami->amdbg->rx_length_histogram[len]);
#endif

	/* insert MSDUs in to current packet chain */
	while (newpkt != NULL) {
		/* strip off FCS in last MSDU */
		if (PKTNEXT(osh, newpkt) == NULL)
			PKTFRAG_TRIM_TAILBYTES(osh, newpkt, DOT11_FCS_LEN, TAIL_BYTES_TYPE_FCS);

		/* there must be a limit to stop in order to prevent memory/stack overflow */
		if (num_sf >= MAX_RX_SUBFRAMES) {
			WL_AMSDU_ERROR(("%s: more than %d MSDUs !\n", __FUNCTION__, num_sf));
			break;
		}

		/* Frame buffer still points to the start of the receive descriptor. For each
		 * MPDU in chain, move pointer past receive descriptor.
		 */
		if ((WLPKTTAG(newpkt)->flags & WLF_HWAMSDU) == 0) {
			wlc_d11rxhdr_t *wrxh;
			uint pad = 0;
			/* determine whether packet has 2-byte pad */
			wrxh = (wlc_d11rxhdr_t*) PKTDATA(osh, newpkt);
			if (RXS_SHORT_ENAB(wlc->pub->corerev) &&
				((D11RXHDR_ACCESS_VAL(&wrxh->rxhdr,
				ami->pub->corerev, dma_flags)) & RXS_SHORT_MASK)) {
				uint16 *mrxs;
				mrxs = wlc_get_mrxs(wlc, &wrxh->rxhdr);
				pad = ((*mrxs & RXSS_PBPRES) != 0) ? 2 : 0;
			} else {
				pad = (((D11RXHDR_ACCESS_VAL(&wrxh->rxhdr,
					ami->pub->corerev, RxStatus1)) &
					RXS_PBPRES) != 0) ? 2 : 0;
			}
			PKTPULL(wlc->osh, newpkt, wlc->hwrxoff + pad);
			resid -= (wlc->hwrxoff + pad);
		}

		/* each subframe is 802.3 frame */
		eh = (struct ether_header *)PKTDATA(osh, newpkt);
		len = (uint16)PKTLEN(osh, newpkt) + (uint16)PKTFRAGUSEDLEN(osh, newpkt);

#ifdef BCMPCIEDEV
		if (BCMPCIEDEV_ENAB() && PKTISHDRCONVTD(osh, newpkt)) {
			/* Allready converted */
			goto skip_conv;
		}
#endif /* BCMPCIEDEV */

		sflen = NTOH16(eh->ether_type) + ETHER_HDR_LEN;

		if ((((uintptr)eh + (uint)ETHER_HDR_LEN) % 4) != 0) {
			WL_AMSDU_ERROR(("%s: sf body is not 4b aligned!\n", __FUNCTION__));
			WLCNTINCR(ami->cnt->deagg_badsfalign);
			goto toss;
		}

		/* last MSDU: has FCS, but no pad, other MSDU: has pad, but no FCS */
		if (len != (PKTNEXT(osh, newpkt) ? ROUNDUP(sflen, 4) : sflen)) {
			WL_AMSDU_ERROR(("%s: len mismatch buflen %d sflen %d, sf %d\n",
				__FUNCTION__, len, sflen, num_sf));
			WLCNTINCR(ami->cnt->deagg_badsflen);
			goto toss;
		}

		/* strip trailing optional pad */
		if (PKTFRAGUSEDLEN(osh, newpkt)) {
			PKTSETFRAGUSEDLEN(osh, newpkt, (sflen - PKTLEN(osh, newpkt)));
		} else {
			PKTSETLEN(osh, newpkt, sflen);
		}

		if (bsscfg->wlcif->if_flags & WLC_IF_PKT_80211) {
			/* convert 802.3 to 802.11 */
			wlc_amsdu_to_dot11(ami, scb, &hdr_copy, newpkt);
			if (*pp != newpkt) {
				wlc_pkttag_t * new_pkttag = WLPKTTAG(newpkt);

				new_pkttag->rxchannel = pkttag->rxchannel;
				new_pkttag->pktinfo.misc.rssi = pkttag->pktinfo.misc.rssi;
				new_pkttag->rspec = pkttag->rspec;
			}

		} else {
			/* convert 8023hdr to ethernet if necessary */
			wlc_8023_etherhdr(wlc, osh, newpkt);
		}
#ifdef BCMPCIEDEV
skip_conv:
#endif /* BCMPCIEDEV */

		eh = (struct ether_header *)PKTDATA(osh, newpkt);
#ifdef PSTA
		if (BSSCFG_STA(rfc->bsscfg) &&
		    PSTA_IS_REPEATER(wlc) &&
		    TRUE)
			da = (uint8 *)&rfc->ds_ea;
		else
#endif
			da = eh->ether_dhost;

		if (ETHER_ISNULLDEST(da))
			goto toss;

		if (*chained_sendup) {
			if (wlc->pktc_info->h_da == NULL)
				wlc->pktc_info->h_da = (struct ether_addr *)da;

			*chained_sendup = !ETHER_ISMULTI(da) &&
				!eacmp(wlc->pktc_info->h_da, da) &&
#if defined(HNDCTF)
				CTF_HOTBRC_CMP(wlc->pub->brc_hot, da, rfc->wlif_dev) &&
#endif 
				((eh->ether_type == HTON16(ETHER_TYPE_IP)) ||
				(eh->ether_type == HTON16(ETHER_TYPE_IPV6)));
		}

		WL_AMSDU(("%s: deagg MSDU buffer %d, frame %d\n",
		          __FUNCTION__, len, sflen));

		/* remove from AMSDU chain and insert in to MPDU chain. skip
		 * the head MSDU since it is already in chain.
		 */
		tmp_next = PKTNEXT(osh, newpkt);
		if (num_sf > 0) {
			/* remove */
			PKTSETNEXT(osh, head, tmp_next);
			PKTSETNEXT(osh, newpkt, NULL);
			/* insert */
			PKTSETCLINK(newpkt, PKTCLINK(tail));
			PKTSETCLINK(tail, newpkt);
			tail = newpkt;
			/* set prio */
			PKTSETPRIO(newpkt, PKTPRIO(head));
			WLPKTTAGSCBSET(newpkt, rfc->scb);
		}

		*pp = newpkt;
		PKTCADDLEN(head, len);
		PKTSETCHAINED(wlc->osh, newpkt);

		num_sf++;
		newpkt = tmp_next;
		resid -= len;
	}

	if (resid != 0) {
		ASSERT(0);
		WLCNTINCR(ami->cnt->deagg_badtotlen);
		goto toss;
	}

	/* toss the remaining MSDU, which we couldn't handle */
	if (newpkt != NULL) {
		WL_AMSDU_ERROR(("%s: toss MSDUs > %d !\n", __FUNCTION__, num_sf));
		PKTFREE(osh, newpkt, FALSE);
	}

	WL_AMSDU(("%s: this A-MSDU has %d MSDUs, done\n", __FUNCTION__, num_sf));
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	WLCNTINCR(ami->amdbg->rx_msdu_histogram[MIN(num_sf, AMSDU_RX_SUBFRAMES_BINS-1)]);
#endif

	(*index) += num_sf;

	return BCME_OK;

toss:
	/*
	 * toss the whole A-MSDU since we don't know where the error starts
	 *  e.g. a wrong subframe length for mid frame can slip through the ucode
	 *       and the only syptom may be the last MSDU frame has the mismatched length.
	 */
	if (PKTNEXT(osh, head)) {
		PKTFREE(osh, PKTNEXT(osh, head), FALSE);
		PKTSETNEXT(osh, head, NULL);
	}

	if (head != tail) {
		while ((tmp_next = PKTCLINK(head)) != NULL) {
			PKTSETCLINK(head, PKTCLINK(tmp_next));
			PKTFREE(osh, tmp_next, FALSE);
			if (tmp_next == tail) {
				/* assign *pp to head so that wlc_sendup_chain
				 * does not try to free tmp_next again
				 */
				*pp = head;
				break;
			}
		}
	}

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	WLCNTINCR(ami->amdbg->rx_msdu_histogram[0]);
#endif
	WL_AMSDU(("%s: tossing amsdu in deagg -- error seen\n", __FUNCTION__));
	return BCME_ERROR;
} /* wlc_amsdu_pktc_deagg_hw */
#endif /* PKTC */


#ifdef WLAMSDU_SWDEAGG
/**
 * A-MSDU sw deaggregation - for testing only due to lower performance to align payload.
 *
 *    | 80211 MAC HEADER | subFrame 1 | subFrame 2 | subFrame 3 | ... |
 * where, 80211 MAC header also includes WDS and/or QOS and/or IV fields
 *        f->pbody points to beginning of subFrame 1,
 *        f->body_len is the total length of all sub frames, exclude ICV and/or FCS
 *
 *        each subframe is in the form of | 8023hdr | body | pad |
 *                subframe other than the last one may have pad bytes
*/
/*
 * Note: This headroom calculation comes out to 10 byte.
 * Arithmetically, this amounts to two 4-byte blocks plus
 * 2. 2 bytes are needed anyway to achieve 4-byte alignment.
 */
#define HEADROOM  DOT11_A3_HDR_LEN-ETHER_HDR_LEN
void
wlc_amsdu_deagg_sw(amsdu_info_t *ami, struct scb *scb, struct wlc_frminfo *f)
{
	osl_t *osh;
	struct ether_header *eh;
	struct ether_addr *ea;
	uchar *data;
	void *newpkt;
	int resid;
	uint16 body_offset, sflen, len;
	wlc_pkttag_t * pkttag = WLPKTTAG(f->p);

	struct dot11_header hdr_copy;

	if (WLEXTSTA_ENAB(ami->wlc->pub)) {
		/* Save the header before being overwritten */
		bcopy((char *)f->h, (char *)&hdr_copy, DOT11_A3_HDR_LEN);

		/* Assume f->h pointer is not valid any more */
		f->h = NULL;
	}

	osh = ami->pub->osh;

	/* all in one buffer, no chain */
	ASSERT(PKTNEXT(osh, f->p) == NULL);

	/* throw away mac header all together, start from A-MSDU body */
	body_offset = (uint)(f->pbody - (uchar*)PKTDATA(osh, f->p));
	PKTPULL(osh, f->p, body_offset);
	ASSERT(f->pbody == (uchar *)PKTDATA(osh, f->p));
	data = f->pbody;
	resid = f->totlen;

	WL_AMSDU(("wlc_amsdu_deagg_sw: body_len(exclude ICV and FCS) %d\n", resid));

	/* loop over orig unpacking and copying frames out into new packet buffers */
	while (resid > 0) {
		if (resid < ETHER_HDR_LEN + DOT11_LLC_SNAP_HDR_LEN)
			break;

		/* each subframe is 802.3 frame */
		eh = (struct ether_header*) data;
		sflen = ntoh16(eh->ether_type) + ETHER_HDR_LEN;

		/* swdeagg is mainly for testing, not intended to support big buffer.
		 *  there are also the 2K hard limit for rx buffer we posted.
		 *  We can increase to 4K, but it wastes memory and A-MSDU often goes
		 *  up to 8K. HW deagg is the preferred way to handle large A-MSDU.
		 */
		if (sflen > ETHER_MAX_DATA + DOT11_LLC_SNAP_HDR_LEN + ETHER_HDR_LEN) {
			WL_AMSDU_ERROR(("%s: unexpected long pkt, toss!", __FUNCTION__));
			WLCNTINCR(ami->cnt->deagg_swdeagglong);
			goto done;
		}

		/*
		 * Alloc new rx packet buffer, add headroom bytes to
		 * achieve 4-byte alignment and to allow for changing
		 * the hdr from 802.3 to 802.11 (EXT STA only)
		 */
		if ((newpkt = PKTGET(osh, sflen + HEADROOM, FALSE)) == NULL) {
			WL_ERROR(("wl: %s: pktget error\n", __FUNCTION__));
			WLCNTINCR(ami->pub->_cnt->rxnobuf);
			goto done;
		}
		PKTPULL(osh, newpkt, HEADROOM);
		/* copy next frame into new rx packet buffer, pad bytes are dropped */
		bcopy(data, PKTDATA(osh, newpkt), sflen);
		PKTSETLEN(osh, newpkt, sflen);

		if (WLEXTSTA_ENAB(ami->wlc->pub)) {
			/* convert 802.3 to 802.11 */
			wlc_amsdu_to_dot11(ami, scb, &hdr_copy, newpkt);
			if (f->p != newpkt) {
				wlc_pkttag_t * new_pkttag = WLPKTTAG(newpkt);

				new_pkttag->rxchannel = pkttag->rxchannel;
				new_pkttag->pktinfo.misc.rssi = pkttag->pktinfo.misc.rssi;
				new_pkttag->rspec = pkttag->rspec;
			}

		} else {
			/* convert 8023hdr to ethernet if necessary */
			wlc_8023_etherhdr(ami->wlc, osh, newpkt);
		}

		if (WLEXTSTA_ENAB(ami->wlc->pub)) {
			ea = &hdr_copy.a1;
		}
		else {
			ea = (struct ether_addr *) PKTDATA(osh, newpkt);
		}

		/* transfer prio, NO need to transfer other tags, it's plain stack packet now */
		PKTSETPRIO(newpkt, f->prio);

		wlc_recvdata_sendup(ami->wlc, scb, f->wds, ea, newpkt);

		/* account padding bytes */
		len = ROUNDUP(sflen, 4);

		WL_AMSDU(("wlc_amsdu_deagg_sw: deagg one frame datalen=%d, buflen %d\n",
			sflen, len));

		data += len;
		resid -= len;

		/* last MSDU doesn't have pad, may overcount */
		if (resid < -4) {
			WL_AMSDU_ERROR(("wl: %s: error: resid %d\n", __FUNCTION__, resid));
			break;
		}
	}

done:
	/* all data are copied, free the original amsdu frame */
	PKTFREE(osh, f->p, FALSE);
} /* wlc_amsdu_deagg_sw */
#endif /* WLAMSDU_SWDEAGG */


#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(BCMDBG_AMSDU)
static int
wlc_amsdu_dump(void *ctx, struct bcmstrbuf *b)
{
	amsdu_info_t *ami = ctx;
	uint i, last;
	uint32 total = 0;

	bcm_bprintf(b, "amsdu_agg_block %d amsdu_rx_mtu %d rcvfifo_limit %d amsdu_rxcap_big %d\n",
		ami->amsdu_agg_block, ami->amsdu_rx_mtu,
		ami->mac_rcvfifo_limit, ami->amsdu_rxcap_big);

	for (i = 0; i < RX_FIFO_NUMBER; i++) {
		amsdu_deagg_t *deagg = &ami->amsdu_deagg[i];
		bcm_bprintf(b, "%d amsdu_deagg_state %d\n", i, deagg->amsdu_deagg_state);
	}

	for (i = 0; i < NUMPRIO; i++) {
		bcm_bprintf(b, "%d agg_allowprio %d agg_bytes_limit %d agg_sf_limit %d",
			i, ami->txpolicy.amsdu_agg_enable[i],
			ami->txpolicy.amsdu_max_agg_bytes[i],
			ami->txpolicy.amsdu_max_sframes);
#ifndef TXQ_MUX
		bcm_bprintf(b, " fifo_lowm %d fifo_hiwm %d",
			ami->txpolicy.fifo_lowm, ami->txpolicy.fifo_hiwm);
#endif
		bcm_bprintf(b, "\n");
	}

#ifdef WLCNT
	wlc_amsdu_dump_cnt(ami, b);
#endif

	for (i = 0, last = 0, total = 0; i < MAX_TX_SUBFRAMES_LIMIT; i++) {
		total += ami->amdbg->tx_msdu_histogram[i];
		if (ami->amdbg->tx_msdu_histogram[i])
			last = i;
	}
	bcm_bprintf(b, "TxMSDUdens:");
	for (i = 0; i <= last; i++) {
		bcm_bprintf(b, " %6u(%2d%%)", ami->amdbg->tx_msdu_histogram[i],
			(total == 0) ? 0 :
			(ami->amdbg->tx_msdu_histogram[i] * 100 / total));
		if (((i+1) % 8) == 0 && i != last)
			bcm_bprintf(b, "\n        :");
	}
	bcm_bprintf(b, "\n\n");

	for (i = 0, last = 0, total = 0; i < AMSDU_LENGTH_BINS; i++) {
		total += ami->amdbg->tx_length_histogram[i];
		if (ami->amdbg->tx_length_histogram[i])
			last = i;
	}
	bcm_bprintf(b, "TxAMSDU Len:");
	for (i = 0; i <= last; i++) {
		bcm_bprintf(b, " %2u-%uk%s %6u(%2d%%)", i, i+1, (i < 9)?" ":"",
		            ami->amdbg->tx_length_histogram[i],
		            (total == 0) ? 0 :
		            (ami->amdbg->tx_length_histogram[i] * 100 / total));
		if (((i+1) % 4) == 0 && i != last)
			bcm_bprintf(b, "\n         :");
	}
	bcm_bprintf(b, "\n");

	for (i = 0, last = 0, total = 0; i < AMSDU_RX_SUBFRAMES_BINS; i++) {
		total += ami->amdbg->rx_msdu_histogram[i];
		if (ami->amdbg->rx_msdu_histogram[i])
			last = i;
	}
	bcm_bprintf(b, "RxMSDUdens:");
	for (i = 0; i <= last; i++) {
		bcm_bprintf(b, " %6u(%2d%%)", ami->amdbg->rx_msdu_histogram[i],
			(total == 0) ? 0 :
			(ami->amdbg->rx_msdu_histogram[i] * 100 / total));
		if (((i+1) % 8) == 0 && i != last)
			bcm_bprintf(b, "\n        :");
	}
	bcm_bprintf(b, "\n\n");

	for (i = 0, last = 0, total = 0; i < AMSDU_LENGTH_BINS; i++) {
		total += ami->amdbg->rx_length_histogram[i];
		if (ami->amdbg->rx_length_histogram[i])
			last = i;
	}
	bcm_bprintf(b, "RxAMSDU Len:");
	for (i = 0; i <= last; i++) {
		bcm_bprintf(b, " %2u-%uk%s %6u(%2d%%)", i, i+1, (i < 9)?" ":"",
		            ami->amdbg->rx_length_histogram[i],
		            (total == 0) ? 0 :
		            (ami->amdbg->rx_length_histogram[i] * 100 / total));
		if (((i+1) % 4) == 0 && i != last)
			bcm_bprintf(b, "\n         :");
	}
	bcm_bprintf(b, "\n");


	return 0;
} /* wlc_amsdu_dump */

#ifdef WLCNT
void
wlc_amsdu_dump_cnt(amsdu_info_t *ami, struct bcmstrbuf *b)
{
	wlc_amsdu_cnt_t *cnt = ami->cnt;

#ifdef TXQ_MUX
	bcm_bprintf(b, "tx_nprocessed %u\n", cnt->tx_nprocessed);
#endif
	bcm_bprintf(b, "agg_openfail %u\n", cnt->agg_openfail);
	bcm_bprintf(b, "agg_passthrough %u\n", cnt->agg_passthrough);
	bcm_bprintf(b, "agg_block %u\n", cnt->agg_openfail);
	bcm_bprintf(b, "agg_amsdu %u\n", cnt->agg_amsdu);
	bcm_bprintf(b, "agg_msdu %u\n", cnt->agg_msdu);
	bcm_bprintf(b, "agg_stop_tailroom %u\n", cnt->agg_stop_tailroom);
	bcm_bprintf(b, "agg_stop_sf %u\n", cnt->agg_stop_sf);
	bcm_bprintf(b, "agg_stop_len %u\n", cnt->agg_stop_len);
#ifdef TXQ_MUX
	bcm_bprintf(b, "agg_stop_ratelimit %u\n", cnt->agg_stop_ratelimit);
	bcm_bprintf(b, "agg_stop_qempty %u\n", cnt->agg_stop_qempty);
	bcm_bprintf(b, "agg_stop_flags %u\n", cnt->agg_stop_flags);
	bcm_bprintf(b, "agg_stop_pktcallback %u\n", cnt->agg_stop_pktcallback);
	bcm_bprintf(b, "agg_stop_tid %u\n", cnt->agg_stop_tid);
	bcm_bprintf(b, "agg_stop_headroom %u\n", cnt->agg_stop_headroom);
	bcm_bprintf(b, "agg_stop_hostsuppr %u\n", cnt->agg_stop_hostsuppr);
	bcm_bprintf(b, "agg_stop_pad_fail %u\n", cnt->agg_stop_pad_fail);
	bcm_bprintf(b, "rlimit_max_bytes %u\n", cnt->rlimit_max_bytes);
	bcm_bprintf(b, "passthrough_min_agglen %u\n", cnt->passthrough_min_agglen);
	bcm_bprintf(b, "passthrough_min_aggsf %u\n", cnt->passthrough_min_aggsf);
	bcm_bprintf(b, "passthrough_rlimit_bytes %u\n", cnt->passthrough_rlimit_bytes);
	bcm_bprintf(b, "passthrough_legacy_rate %u\n", cnt->passthrough_legacy_rate);
	bcm_bprintf(b, "passthrough_invalid_rate %u\n", cnt->passthrough_invalid_rate);
#else
	bcm_bprintf(b, "agg_stop_lowwm %u\n", cnt->agg_stop_lowwm);
#endif /* TXQ_MUX */
	bcm_bprintf(b, "deagg_msdu %u\n", cnt->deagg_msdu);
	bcm_bprintf(b, "deagg_amsdu %u\n", cnt->deagg_amsdu);
	bcm_bprintf(b, "deagg_badfmt %u\n", cnt->deagg_badfmt);
	bcm_bprintf(b, "deagg_wrongseq %u\n", cnt->deagg_wrongseq);
	bcm_bprintf(b, "deagg_badsflen %u\n", cnt->deagg_badsflen);
	bcm_bprintf(b, "deagg_badsfalign %u\n", cnt->deagg_badsfalign);
	bcm_bprintf(b, "deagg_badtotlen %u\n", cnt->deagg_badtotlen);
	bcm_bprintf(b, "deagg_openfail %u\n", cnt->deagg_openfail);
	bcm_bprintf(b, "deagg_swdeagglong %u\n", cnt->deagg_swdeagglong);
	bcm_bprintf(b, "deagg_flush %u\n", cnt->deagg_flush);
	bcm_bprintf(b, "tx_pkt_free_ignored %u\n", cnt->tx_pkt_free_ignored);
	bcm_bprintf(b, "tx_padding_in_tail %u\n", cnt->tx_padding_in_tail);
	bcm_bprintf(b, "tx_padding_in_head %u\n", cnt->tx_padding_in_head);
	bcm_bprintf(b, "tx_padding_no_pad %u\n", cnt->tx_padding_no_pad);
#ifdef TXQ_MUX
	bcm_bprintf(b, "tx_singleton %u\n", cnt->tx_singleton);
	bcm_bprintf(b, "tx_pkt_putback %u\n", cnt->tx_pkt_putback);
	bcm_bprintf(b, "agg_alloc_newhdr %u\n", cnt->agg_alloc_newhdr);
	bcm_bprintf(b, "agg_alloc_newhdr_fail %u\n", cnt->agg_alloc_newhdr_fail);
#endif
}
#endif	/* WLCNT */

#if defined(WLAMSDU_TX) && !defined(WLAMSDU_TX_DISABLED)
static void
wlc_amsdu_dump_scb(void *ctx, struct scb *scb, struct bcmstrbuf *b)
{
	amsdu_info_t *ami = (amsdu_info_t *)ctx;
	scb_amsdu_t *scb_amsdu = SCB_AMSDU_CUBBY(ami, scb);
	amsdu_scb_txpolicy_t *amsdupolicy;
	/* Not allocating as a static or const, only needed during scope of the dump */
	char ac_name[NUMPRIO][3] = {"BE", "BK", "--", "EE", "CL", "VI", "VO", "NC"};
	uint i;
#ifndef TXQ_MUX
	amsdu_txaggstate_t *aggstate;
#endif

	if (!AMSDU_TX_SUPPORT(ami->pub) || !scb_amsdu || !SCB_AMSDU(scb))
		return;

	bcm_bprintf(b, "\n");
	for (i = 0; i < NUMPRIO; i++) {

		amsdupolicy = &scb_amsdu->scb_txpolicy[i];
		if (amsdupolicy->pub.amsdu_agg_enable == FALSE) {
			continue;
		}

		/* add \t to be aligned with other scb stuff */
		bcm_bprintf(b, "\tAMSDU scb prio: %s(%d)\n", ac_name[i], i);

#ifndef TXQ_MUX
		aggstate = &scb_amsdu->aggstate[i];
		bcm_bprintf(b, "\tamsdu_agg_sframes %u amsdu_agg_bytes %u amsdu_agg_txpending %u\n",
			aggstate->amsdu_agg_sframes, aggstate->amsdu_agg_bytes,
			aggstate->amsdu_agg_txpending);

#endif
		bcm_bprintf(b, "\tamsdu_ht_agg_bytes_max %d vht_agg_max %d amsdu_agg_enable %d\n",
			amsdupolicy->amsdu_ht_agg_bytes_max, amsdupolicy->amsdu_vht_agg_bytes_max,
			amsdupolicy->pub.amsdu_agg_enable);

		bcm_bprintf(b, "\n");
	}
}
#endif /* WLAMSDU_TX && !WLAMSDU_TX_DISABLED */
#endif	/* BCMDBG || BCMDBG_DUMP || BCMDBG_AMSDU */

#if defined(WL_DATAPATH_LOG_DUMP) && !defined(TXQ_MUX) && defined(WLAMSDU_TX) && \
	!defined(WLAMSDU_TX_DISABLED)
/**
 * Cubby datapath log callback fn
 * Use EVENT_LOG to dump a summary of the AMPDU datapath state
 * @param ctx   context pointer to amsdu_info_t state structure
 * @param scb   scb of interest for the dump
 * @param tag   EVENT_LOG tag for output
 */
static void
wlc_amsdu_datapath_summary(void *ctx, struct scb *scb, int tag)
{
	int buf_size;
	uint prio;
	uint num_prio;
	scb_subq_summary_t *sum;
	amsdu_info_t *ami = (amsdu_info_t *)ctx;
	scb_amsdu_t *scb_amsdu = SCB_AMSDU_CUBBY(ami, scb);
	osl_t *osh;

	if (!AMSDU_TX_SUPPORT(ami->pub) || !scb_amsdu || !SCB_AMSDU(scb)) {
		/* nothing to report on for AMSDU */
		return;
	}

	osh = ami->wlc->osh;

	/*
	 * allcate a scb_subq_summary struct to dump amsdu information to the EVENT_LOG
	 */

	/* AMSDU works on per-tid (prio) */
	num_prio = NUMPRIO;

	/* allocate a size large enough for supported prio */
	buf_size = SCB_SUBQ_SUMMARY_FULL_LEN(num_prio);

	sum = MALLOCZ(osh, buf_size);
	if (sum == NULL) {
		EVENT_LOG(tag,
		          "wlc_amsdu_datapath_summary(): MALLOC %d failed, malloced %d bytes\n",
		          buf_size, MALLOCED(osh));
		/* nothing to do if event cannot be allocated */
		return;
	}

	/* Report the multi-prio queue summary */
	sum->id = EVENT_LOG_XTLV_ID_SCBDATA_SUM;
	sum->len = SCB_SUBQ_SUMMARY_FULL_LEN(num_prio) - BCM_XTLV_HDR_SIZE;
	sum->cubby_id = ami->cubby_name_id;
	sum->prec_count = num_prio;
	for (prio = 0; prio < num_prio; prio++) {
		sum->plen[prio] = scb_amsdu->aggstate[prio].amsdu_agg_sframes;
	}

	EVENT_LOG_BUFFER(tag, (uint8*)sum, sum->len + BCM_XTLV_HDR_SIZE);

	MFREE(osh, sum, buf_size);
}
#endif /* WL_DATAPATH_LOG_DUMP && !TXQ_MUX && WLAMSDU_TX && !WLAMSDU_TX_DISABLED */

/* update amsdu states during mode switch */
void
wlc_amsdu_update_state(wlc_info_t *wlc)
{
	bool tx_amsdu = TRUE;
#ifdef WLAMSDU_TX
	struct scb_iter scbiter;
	struct scb *scb;
#endif

	/* Return if 11n not enabled */
	if (!N_ENAB(wlc->pub)) {
		return;
	}

	/* Return if amsdu not supported */
	if (!AMSDU_TX_SUPPORT(wlc->pub)) {
		return;
	}

#if defined(WLRSDB) && defined(AMSDU_OFF_IN_RSDB)
	/* Disable AMSDU in Dongle, since rxpost buffers are lesser for RSDB */
	if (RSDB_ENAB(wlc->pub) &&
		WLC_RSDB_DUAL_MAC_MODE(WLC_RSDB_CURR_MODE(wlc)) &&
		!WLC_DUALMAC_RSDB(wlc->cmn))
		tx_amsdu = FALSE;
#endif /* WLRSDB && AMSDU_OFF_IN_RSDB */

#ifdef WLAMSDU_TX
	/* Set amsdu state */
	wlc_amsdu_set(wlc->ami, tx_amsdu);

	/* set/reset amsdu txmod functions depending on cur state */
	/* reset scb amsdu flags if amsdu is disabled, set back from ampdu ba resp */
	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		if (AMSDU_TX_ENAB(wlc->pub)) {
			scb->flags |= SCB_AMSDUCAP;
		        wlc_txmod_config(wlc->txmodi, scb, TXMOD_AMSDU);
		} else {
			wlc_txmod_unconfig(wlc->txmodi, scb, TXMOD_AMSDU);
			scb->flags &= ~SCB_AMSDUCAP;
		}
	}
#endif

	/* Set rx amsdu state */
	if (D11REV_GE(wlc->pub->corerev, 40) &&
		(wlc->pub->tunables->nrxbufpost >= wlc->pub->tunables->amsdu_rxpost_threshold) &&
		tx_amsdu) {
		wlc->_rx_amsdu_in_ampdu = TRUE;
	} else {
		wlc->_rx_amsdu_in_ampdu = FALSE;
	}
}

/* Note: TXQ_MUX does not queue pkts for AMSDU, so there is only a tx healthcheck
 * on AMSDU when !TXQ_MUX.
 */
#if defined(WL_TXQ_STALL) && !defined(TXQ_MUX)
int
wlc_amsdu_tx_health_check(wlc_info_t * wlc)
{
	amsdu_info_t * ami;
	struct scb *scb;
	struct scb_iter scbiter;
	scb_amsdu_t *scb_ami;
	amsdu_txaggstate_t * aggstate;
	uint32 prio;
	char eabuf[ETHER_ADDR_STR_LEN];
	wlc_tx_stall_info_t * tx_stall;

	if (wlc == NULL) {
		return BCME_BADARG;
	}

	ami = wlc->ami;

	if ((ami == NULL) ||
		(!AMSDU_TX_SUPPORT(ami->pub))) {
		return BCME_UNSUPPORTED;
	}

	tx_stall = wlc->tx_stall;

	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		/* Skip unsupported scb */
		if (!SCB_AMSDU(scb) ||
			!(scb_ami = SCB_AMSDU_CUBBY(wlc->ami, scb))) {
			continue;
		}

		/* Check for stalled tx amsdu queue */
		for (prio = 0; prio < NUMPRIO; prio++) {
			aggstate = &scb_ami->aggstate[prio];
			/* No packets queued, continue */
			if (aggstate->amsdu_agg_p == NULL) {
				continue;
			}

			/* Fitst packet was queued before timeout seconds */
			if ((wlc->pub->now - aggstate->timestamp) >=
				tx_stall->timeout) {
				snprintf(tx_stall->error->reason,
					sizeof(tx_stall->error->reason) - 1,
					"txq:amsdu,cfg:%s,scb:%s,prio:%d,qts:%d,now:%d",
					(SCB_BSSCFG(scb) && SCB_BSSCFG(scb)->wlcif) ?
						wl_ifname(wlc->wl,
						SCB_BSSCFG(scb)->wlcif->wlif) : "",
					bcm_ether_ntoa(&scb->ea, eabuf), prio,
					aggstate->timestamp, wlc->pub->now);
				tx_stall->error->reason[sizeof(tx_stall->error->reason) - 1] = '\0';
				WL_ERROR(("AMSDU_TXQ stall: %s\n",
					tx_stall->error->reason));
				if (tx_stall->assert_on_error) {
					tx_stall->error->stall_reason =
						WLC_TX_STALL_REASON_TXQ_NO_PROGRESS;

					wlc_bmac_report_fatal_errors(wlc->hw, WL_REINIT_TX_STALL);
				} else {
					WL_ERROR(("Possible AMSDU TXQ stall: %s\n",
						tx_stall->error->reason));
					wl_log_system_state(wlc->wl, tx_stall->error->reason, TRUE);
				}
				return BCME_ERROR;
			}
		}
	}

	return BCME_OK;
}
#endif /* WL_TXQ_STALL */

#if defined(WLAMSDU_TX) && !defined(TXQ_MUX)
/* Return number of TX packets queued for given TID.
 * on non zero return timestamp of the first packet queued
 * in reorder queue will be returned
 */
int wlc_amsdu_tx_queued_pkts(amsdu_info_t *ami,
	struct scb * scb, int tid, uint * timestamp)
{
	scb_amsdu_t *scb_ami;

	if ((ami == NULL) ||
		(scb == NULL) ||
		(tid < 0) ||
		(tid >= NUMPRIO)) {
		return 0;
	}

	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
#ifdef WL_TXQ_STALL
	if (scb_ami->aggstate[tid].amsdu_agg_sframes && timestamp) {
		*timestamp = scb_ami->aggstate[tid].timestamp;
	}
#else
	*timestamp = 0;
#endif

	return scb_ami->aggstate[tid].amsdu_agg_sframes;
}
#endif /* WLAMSDU_TX && !TXQ_MUX */
