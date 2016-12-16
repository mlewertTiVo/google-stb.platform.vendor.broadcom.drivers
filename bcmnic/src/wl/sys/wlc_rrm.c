/*
 * 802.11k protocol implementation for
 * Broadcom 802.11bang Networking Device Driver
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
 * $Id: wlc_rrm.c 661109 2016-09-23 07:54:52Z $
 */

/**
 * @file
 * @brief
 * Radio Resource Management
 * Wireless LAN (WLAN) Radio Measurements enable STAs to understand the radio environment in which
 * they exist. WLAN Radio Measurements enable STAs to observe and gather data on radio link
 * performance and on the radio environment. A STA may choose to make measurements locally, request
 * a measurement from another STA, or may be requested by another STA to make one or more
 * measurements and return the results. Radio Measurement data is made available to STA management
 * and upper protocol layers where it may be used for a range of applications. The measurements
 * enable adjustment of STA operation to better suit the radio environment.
 */



#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <proto/802.11.h>
#include <wlioctl.h>
#include <sbhndpio.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_channel.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_scan.h>
#include <wlc_rrm.h>
#include <bcmwpa.h>
#include <wlc_assoc.h>
#include <wl_dbg.h>
#include <wlc_tpc.h>
#include <wlc_11h.h>
#include <wl_export.h>
#include <wlc_utils.h>
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_ie_helper.h>
#include <wlc_hw.h>
#include <wlc_pm.h>
#include <wlc_scan_utils.h>
#include <wlc_event_utils.h>
#include <phy_misc_api.h>
#include <wlc_iocv.h>
#if defined(WL_PROXDETECT) && defined(WL_FTM)
#include <wlc_ftm.h>
#include <wlc_ht.h>
#include <wlc_vht.h>
#endif /* WL_PROXDETECT && WL_FTM */

#ifdef WLMCNX
#include <wlc_mcnx.h>
#endif

#define WLC_RRM_MS_IN_SEC 1000

#define WLC_RRM_UPDATE_TOKEN(i) \
	{ ++i; if (i == 0) ++i; }

#define WLC_RRM_ELEMENT_LIST_ADD(s, e) \
	do { \
		(e)->next = (s); \
		(s) = (e); \
	} while (0)

/* Beacon report modes. */
#define WLC_RRM_CHANNEL_REPORT_MODE(a) (a->channel == 255)
#define WLC_RRM_REPORT_ALL_CHANNELS(a) (a->channel == 0)

#define WLC_RRM_MIN_TIMER   20 /* (ms) min time for a measure */
#define WLC_RRM_PREP_MARGIN 30 /* (ms) time to prepare for a measurement on a different channel */
#define WLC_RRM_HOME_TIME   40 /* (ms) min time on home channel between off-channel measurements */
#define WLC_RRM_MAX_REP_NUM 65535 /* Max number of repetitions */
#define WLC_RRM_REQUEST_ID_MAX 20

#define MAX_NEIGHBORS 64
#define WLC_RRM_BCNREQ_INTERVAL 180000 /* 3 minutes */
#define WLC_RRM_BCNREQ_INTERVAL_DUR 20
#define MAX_BEACON_NEIGHBORS 5
#define MAX_CHANNEL_REPORT_IES 8
#define WLC_RRM_BCNREQ_NUM_REQ_ELEM 8 /* number of requested elements for reporting detail 1 */
#define WLC_RRM_WILDCARD_SSID_IND   "*" /* indicates the wildcard SSID (SSID len of 0) */

#define RRM_NREQ_FLAGS_LCI_PRESENT		0x1
#define RRM_NREQ_FLAGS_CIVICLOC_PRESENT	0x2
#define RRM_NREQ_FLAGS_OWN_REPORT		0x4

/* Radio Measurement states */
#define WLC_RRM_IDLE                     0 /* Idle */
#define WLC_RRM_ABORT                    1 /* Abort */
#define WLC_RRM_WAIT_START_SET           2 /* Wait Start set */
#define WLC_RRM_WAIT_STOP_TRAFFIC        3 /* Wait Stop traffic */
#define WLC_RRM_WAIT_TX_SUSPEND          4 /* Wait Tx Suspend */
#define WLC_RRM_WAIT_PS_ANNOUNCE         5 /* Wait PS Announcement */
#define WLC_RRM_WAIT_BEGIN_MEAS          6 /* Wait Begin Measurement */
#define WLC_RRM_WAIT_END_MEAS            7 /* Wait End Measurement */
#define WLC_RRM_WAIT_END_CCA             8 /* Wait End CCA */
#define WLC_RRM_WAIT_END_SCAN            9  /* Wait End Scan */
#define WLC_RRM_WAIT_END_FRAME           10 /* Wait End Frame Measurement */

#define WLC_RRM_NOISE_SUPPORTED(rrm_info) TRUE /* except obsolete bphy, all current phy support */

/* a radio measurement is in progress unless is it IDLE,
 * ABORT, or waiting to start or channel switch for a set
 */
#define WLC_RRM_IN_PROGRESS(wlc) (WL11K_ENAB(wlc->pub) && \
		!(((wlc)->rrm_info)->rrm_state->step == WLC_RRM_IDLE ||	\
		((wlc)->rrm_info)->rrm_state->step == WLC_RRM_ABORT ||	\
		((wlc)->rrm_info)->rrm_state->step == WLC_RRM_WAIT_STOP_TRAFFIC || \
		((wlc)->rrm_info)->rrm_state->step == WLC_RRM_WAIT_START_SET))

#define RRM_BSSCFG_CUBBY(rrm_info, cfg) ((rrm_bsscfg_cubby_t *)BSSCFG_CUBBY(cfg, (rrm_info)->cfgh))

/* iovar table */
enum {
	IOV_RRM,
	IOV_RRM_NBR_REQ,
	IOV_RRM_BCN_REQ,
	IOV_RRM_CHLOAD_REQ,
	IOV_RRM_NOISE_REQ,
	IOV_RRM_FRAME_REQ,
	IOV_RRM_STAT_REQ,
	IOV_RRM_STAT_RPT,
	IOV_RRM_LM_REQ,
	IOV_RRM_NBR_LIST,
	IOV_RRM_NBR_DEL_NBR,
	IOV_RRM_NBR_ADD_NBR,
	IOV_RRM_BCN_REQ_THRTL_WIN,
	IOV_RRM_BCN_REQ_MAX_OFF_CHAN_TIME,
	IOV_RRM_BCN_REQ_TRAFF_MEAS_PER,
	IOV_RRM_CONFIG,
	IOV_RRM_FRNG
};

static const bcm_iovar_t rrm_iovars[] = {
	{"rrm", IOV_RRM, IOVF_SET_DOWN, 0, IOVT_INT32, 0},
	{"rrm_nbr_req", IOV_RRM_NBR_REQ, (IOVF_SET_UP), 0, IOVT_BUFFER, 0},
	{"rrm_bcn_req", IOV_RRM_BCN_REQ, (IOVF_SET_UP), 0, IOVT_BUFFER, 0},
	{"rrm_lm_req", IOV_RRM_LM_REQ, (IOVF_SET_UP), 0, IOVT_BUFFER, 0},
	{"rrm_chload_req", IOV_RRM_CHLOAD_REQ, (IOVF_SET_UP), 0, IOVT_BUFFER, 0},
	{"rrm_noise_req", IOV_RRM_NOISE_REQ, (IOVF_SET_UP), 0, IOVT_BUFFER, 0},
	{"rrm_frame_req", IOV_RRM_FRAME_REQ, (IOVF_SET_UP), 0, IOVT_BUFFER, 0},
	{"rrm_stat_req", IOV_RRM_STAT_REQ, (IOVF_SET_UP), 0, IOVT_BUFFER, 0},
	{"rrm_stat_rpt", IOV_RRM_STAT_RPT, (IOVF_SET_UP), 0, IOVT_BUFFER, sizeof(statrpt_t)},
	{"rrm_nbr_list", IOV_RRM_NBR_LIST, (IOVF_GET_UP), 0, IOVT_BUFFER, 0},
	{"rrm_nbr_del_nbr", IOV_RRM_NBR_DEL_NBR, 0, 0, IOVT_BUFFER, 0},
	{"rrm_nbr_add_nbr", IOV_RRM_NBR_ADD_NBR, 0, 0, IOVT_BUFFER, 0},
	{"rrm_bcn_req_thrtl_win", IOV_RRM_BCN_REQ_THRTL_WIN, (0), 0, IOVT_UINT32, 0},
	{"rrm_bcn_req_max_off_chan_time", IOV_RRM_BCN_REQ_MAX_OFF_CHAN_TIME,
	(0), 0, IOVT_UINT32, 0
	},
	{"rrm_bcn_req_traff_meas_per", IOV_RRM_BCN_REQ_TRAFF_MEAS_PER,
	(IOVF_SET_UP), 0, IOVT_UINT32, 0
	},
	{"rrm_config", IOV_RRM_CONFIG, 0, 0, IOVT_BUFFER, 0},
#ifdef WL_FTM_11K
#if defined(WL_PROXDETECT) && defined(WL_FTM)
	{"rrm_frng", IOV_RRM_FRNG, (IOVF_SET_UP), 0, IOVT_BUFFER, 0},
#endif /* WL_PROXDETECT && WL_FTM */
#endif /* WL_FTM_11K */
	{NULL, 0, 0, 0, 0, 0}
};

typedef struct rrm_bcnreq_ {
	uint8 token;
	uint8 reg;
	uint8 channel;

	uint8 rep_detail;			/* Reporting Detail */
	uint8 req_eid[WLC_RRM_REQUEST_ID_MAX];	/* Request elem id */

	uint8 req_eid_num;
	uint16 duration;
	chanspec_t chanspec_list[MAXCHANNEL];
	uint16 channel_num;
	uint32 start_tsf_l;
	uint32 start_tsf_h;
	uint32 scan_type;
	wlc_ssid_t ssid;
	struct ether_addr bssid;
	int scan_status;
	wlc_bss_list_t scan_results;
} rrm_bcnreq_t;

#ifdef WL_FTM_11K
typedef struct rrm_ftmreq {
	uint8 token;
	frngreq_t *frng_req;
	frngrep_t *frng_rep;
#if defined(WL_PROXDETECT) && defined(WL_FTM)
	wlc_ftm_ranging_ctx_t *rctx;
#endif
} rrm_ftmreq_t;
#endif /* WL_FTM_11K */

typedef struct wlc_rrm_req {
	int16 type;		/* type of measurement */
	int8 flags;
	uint8 token;		/* token for this particular measurement */
	chanspec_t chanspec;	/* channel for the measurement */
	uint32 tsf_h;		/* TSF high 32-bits of Measurement Request start time */
	uint32 tsf_l;		/* TSF low 32-bits */
	uint16 dur;		/* TUs */
	uint16 intval;		/* Randomization interval in TUs */
} wlc_rrm_req_t;

typedef struct wlc_rrm_req_state {
	int report_class;	/* type of report to generate */
	bool broadcast;		/* was the request DA broadcast */
	int token;		/* overall token for measurement set */
	uint step;		/* current state of RRM state machine */
	chanspec_t chanspec_return;	/* channel to return to after the measurements */
	bool ps_pending;	/* true if we need to wait for PS to be announced before
				 * off-channel measurement
				 */
	int dur;		/* TUs, min duration of current parallel set measurements */
	uint16 reps;		/* Number of repetitions */
	uint32 actual_start_h;	/* TSF high 32-bits of actual start time */
	uint32 actual_start_l;	/* TSF low 32-bits */
	int cur_req;		/* index of current measure request */
	int req_count;		/* number of measure requests */
	wlc_rrm_req_t *req;	/* array of requests */
	/* CCA measurements */
	bool cca_active;	/* true if measurement in progress */
	int cca_dur;		/* TU, specified duration */
	int cca_idle;		/* idle carrier time reported by ucode */
	uint8 cca_busy;		/* busy fraction */
	/* Beacon measurements */
	bool scan_active;

	/* RPI measurements */
	bool rpi_active;	/* true if measurement in progress */
	bool rpi_end;		/* signal last sample */
	int rpi_dur;		/* TU, specified duration */
	int rpi_sample_num;	/* number of samples collected */
	uint16 rpi[WL_RPI_REP_BIN_NUM];	/* rpi/rssi measurement values */
	int rssi_sample_num;	/* count of samples in averaging total */
	int rssi_sample;	/* current sample averaging total */
	void *cb;		/* completion callback fn: may be NULL */
	void *cb_arg;		/* single arg to callback function */
} wlc_rrm_req_state_t;

struct wlc_rrm_info {
	wlc_info_t *wlc;
	/* Unused. Keep for ROM compatibility. */
	bool state;
	/* The radio measurements that use state machine use global variables as follows */
	uint8 req_token;		/* token used in measure requests from us */
	uint8 dialog_token;		/* Dialog token received in measure req */
	struct ether_addr da;
	rrm_bcnreq_t *bcnreq;
	uint8 req_elt_token;           /* element token in measure requests */
	wlc_rrm_req_state_t *rrm_state;
	struct wl_timer *rrm_timer;     /* 11h radio resource measurement timer */
	int cfgh;                       /* rrm bsscfg cubby handle */
	wlc_bsscfg_t *cur_cfg;	/* current BSS in the rrm state machine */
	int scb_handle;			/* rrm scb cubby handle */
	uint32 bcn_req_thrtl_win; /* Window (secs) in which off-chan time is computed */
	uint32 bcn_req_thrtl_win_sec; /* Seconds in to throttle window */
	uint32 bcn_req_win_scan_cnt; /* Count of scans due to bcn_req in current window */
	uint32 bcn_req_off_chan_time_allowed; /* Max scan time allowed in window (ms) */
	uint32 bcn_req_off_chan_time; /* Total ms scan time completed in throttle window */
	uint32 bcn_req_scan_start_timestamp; /* Intermediate timestamp to mark scan times */
	uint32 bcn_req_traff_meas_prd; /* ms period for checking traffic */
	uint32 data_activity_ts; /* last data activity ts */
#ifdef WL_FTM_11K
#if defined(WL_PROXDETECT) && defined(WL_FTM)
	rrm_ftmreq_t *rrm_frng_req;
#endif /* WL_PROXDETECT && WL_FTM */
#endif /* WL_FTM_11K */
};

/* Neighbor Report element */
typedef struct rrm_nbr_rep {
	struct rrm_nbr_rep *next;
	nbr_rpt_elem_t nbr_elt;
#ifdef WL_FTM_11K
	uint8 *opt_lci;
	uint8 *opt_civic;
#endif /* WL_FTM_11K */
} rrm_nbr_rep_t;

/* AP Channel Report */
typedef struct reg_nbr_count {
	struct reg_nbr_count *next;
	uint8 reg;
	uint16 nbr_count_by_channel[MAXCHANNEL];
} reg_nbr_count_t;

typedef struct {
	wlc_info_t *wlc;
	wlc_rrm_info_t *rrm_info;
	uint8 req_token;			/* token used in measure requests from us */
	uint8 dialog_token;			/* Dialog token received in measure req */
	struct ether_addr da;
	rrm_nbr_rep_t *nbr_rep_head;		/* Neighbor Report element */
	reg_nbr_count_t *nbr_cnt_head;		/* AP Channel Report */
	uint8 rrm_cap[DOT11_RRM_CAP_LEN]; 	/* RRM Enabled Capability */
	uint32 bcn_req_timer;			/* Auto Beacon request timer */
	bool rrm_timer_set;
	rrm_nbr_rep_t *nbr_rep_self;		/* Neighbor Report element for self */
#ifdef WL_FTM_11K
	uint8 lci_token;                        /* LCI token received in measure req */
	uint8 civic_token;                      /* CIVIC loc token received in measure req */
#endif /* WL_FTM_11K */
} rrm_bsscfg_cubby_t;

typedef struct rrm_scb_info {
	uint32 timestamp;	/* timestamp of the report */
	uint16 flag;		/* flag */
	uint16 len;			/* length of payload data */
	unsigned char data[WL_RRM_RPT_MAX_PAYLOAD];
	uint8 rrm_capabilities[DOT11_RRM_CAP_LEN];	/* rrm capabilities of station */
	uint32 bcnreq_time;
	rrm_nbr_rep_t *nbr_rep_head;
} rrm_scb_info_t;

typedef struct rrm_scb_cubby {
	rrm_scb_info_t *scb_info;
} rrm_scb_cubby_t;
#define RRM_SCB_CUBBY(rrm_info, scb) (rrm_scb_cubby_t *)SCB_CUBBY(scb, (rrm_info->scb_handle))
#define RRM_SCB_INFO(rrm_info, scb) (RRM_SCB_CUBBY(rrm_info, scb))->scb_info


static int wlc_rrm_framerep_add(wlc_rrm_info_t *rrm_info, uint8 *bufptr, uint cnt);
static int wlc_rrm_bcnrep_add(wlc_rrm_info_t *rrm_info, wlc_bss_info_t *bi,
	uint8 *bufptr, uint buflen);
static void wlc_rrm_recv_rmreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, struct scb *scb,
	uint8 *body, int body_len);
static void wlc_rrm_recv_lmreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, struct scb *scb,
	uint8 *body, int body_len, int8 rssi, ratespec_t);
static void wlc_rrm_recv_rmrep(wlc_rrm_info_t *rrm_info, struct scb *scb, uint8 *body,
	int body_len);
#ifdef STA
static int wlc_rrm_recv_nrrep(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, uint8 *body,
	int body_len);
static int wlc_rrm_regclass_neighbor_count(wlc_rrm_info_t *rrm_info, rrm_nbr_rep_t *nbr_rep,
	wlc_bsscfg_t *cfg);
static bool wlc_rrm_regclass_match(wlc_rrm_info_t *rrm_info, rrm_nbr_rep_t *nbr_rep,
	wlc_bsscfg_t *cfg);
#endif /* STA */
static bool wlc_rrm_request_current_regdomain(wlc_rrm_info_t *rrm_info,
	dot11_rmreq_bcn_t *rmreq_bcn, rrm_bcnreq_t *bcn_req, wlc_rrm_req_t *req);
static unsigned int wlc_rrm_ap_chreps_to_chanspec(bcm_tlv_t *tlvs, int tlvs_len, rrm_bcnreq_t
	*bcn_req);
static int wlc_rrm_recv_bcnreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	dot11_rmreq_bcn_t *rmreq_bcn, wlc_rrm_req_t *req);
static void wlc_rrm_bcnreq_scancb(void *arg, int status, wlc_bsscfg_t *cfg);
static dot11_ap_chrep_t* wlc_rrm_get_ap_chrep(const wlc_bss_info_t *bi);
static uint8 wlc_rrm_get_ap_chrep_reg(const wlc_bss_info_t *bi);
static int wlc_rrm_add_empty_bcnrep(const wlc_rrm_info_t *rrm_info, uint8 *bufptr, uint buflen);
static void wlc_rrm_send_bcnrep(wlc_rrm_info_t *rrm_info, wlc_bss_list_t *bsslist);
static void wlc_rrm_send_chloadrep(wlc_rrm_info_t *rrm_info);
static void wlc_rrm_send_noiserep(wlc_rrm_info_t *rrm_info);
static void wlc_rrm_send_framerep(wlc_rrm_info_t *rrm_info);
static void wlc_rrm_send_statrep(wlc_rrm_info_t *rrm_info);
static void wlc_rrm_send_txstrmrep(wlc_rrm_info_t *rrm_info);
#ifdef WL_FTM_11K
static void wlc_rrm_send_lcirep(wlc_rrm_info_t *rrm_info, uint8 token);
static void wlc_rrm_send_civiclocrep(wlc_rrm_info_t *rrm_info, uint8 token);
#endif

static int wlc_rrm_doiovar(void *hdl, uint32 actionid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif);
static void wlc_rrm_rep_err(wlc_rrm_info_t *rrm_info, uint8 type, uint8 token, uint8 reason);
static void wlc_rrm_bcnreq_scancache(wlc_rrm_info_t *rrm_info, wlc_ssid_t *ssid,
	struct ether_addr *bssid);

static void wlc_rrm_state_upd(wlc_rrm_info_t  *rrm_info, uint state);
static void wlc_rrm_timer(void *arg);
static void wlc_rrm_recv_lmrep(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, uint8 *body,
	int body_len);
#ifdef WL11K_AP
static void wlc_rrm_recv_nrreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, struct scb *scb,
	uint8 *body, int body_len);
static int wlc_rrm_send_nbrrep(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, struct scb *scb,
	wlc_ssid_t *ssid);
static void wlc_rrm_get_neighbor_list(wlc_rrm_info_t *rrm_info, void *a, uint16 list_cnt,
	rrm_bsscfg_cubby_t *rrm_cfg);
static void wlc_rrm_del_neighbor(wlc_rrm_info_t *rrm_info, struct ether_addr *ea,
	struct wlc_if *wlcif, rrm_bsscfg_cubby_t *rrm_cfg);
static void wlc_rrm_remove_regclass_nbr_cnt(wlc_rrm_info_t *rrm_info,
	rrm_nbr_rep_t *nbr_rep, bool delete_empty_reg, bool *channel_removed,
	rrm_bsscfg_cubby_t *rrm_cfg);
static int wlc_rrm_add_neighbor(wlc_rrm_info_t *rrm_info, nbr_rpt_elem_t *nbr_elt,
	struct wlc_if *wlcif, rrm_bsscfg_cubby_t *rrm_cfg);
static int wlc_rrm_add_regclass_neighbor_count(wlc_rrm_info_t *rrm_info, rrm_nbr_rep_t *nbr_rep,
	bool *channel_added, rrm_bsscfg_cubby_t *rrm_cfg);
static uint16 wlc_rrm_get_neighbor_count(wlc_rrm_info_t *rrm_info, rrm_bsscfg_cubby_t *rrm_cfg);
static uint16 wlc_rrm_get_reg_count(wlc_rrm_info_t *rrm_info, rrm_bsscfg_cubby_t *rrm_cfg);
static uint wlc_rrm_ap_chrep_len(wlc_rrm_info_t *rrm, wlc_bsscfg_t *cfg);
static void wlc_rrm_add_ap_chrep(wlc_rrm_info_t *rrm, wlc_bsscfg_t *cfg, uint8 *bufstart);
static bool wlc_rrm_security_match(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, uint16 cap,
	bcm_tlv_t *wpaie, bcm_tlv_t *wpa2ie);
static uint16 wlc_rrm_get_bcnnbr_count(wlc_rrm_info_t *rrm_info, rrm_scb_info_t *scb_info);
static int wlc_rrm_add_bcnnbr(wlc_rrm_info_t *rrm_info, struct scb *scb,
	dot11_rmrep_bcn_t *rmrep_bcn, struct dot11_bcn_prb *bcn, bcm_tlv_t *htie,
	bcm_tlv_t *mdie, uint32 flags);
static void wlc_rrm_flush_bcnnbrs(wlc_rrm_info_t *rrm_info, struct scb *scb);
#endif /* WL11K_AP */

static void wlc_rrm_flush_neighbors(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *bsscfg);

#ifdef BCMDBG
static void wlc_rrm_dump_neighbors(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *bsscfg);
#endif /* BCMDBG */
static dot11_rm_ie_t *wlc_rrm_next_ie(dot11_rm_ie_t *ie, int *len, uint8 mea_id);
static void wlc_rrm_report_ioctl(wlc_rrm_info_t *rrm_info, wlc_rrm_req_t *req_block, int count);
static void wlc_rrm_next_set(wlc_rrm_info_t *rrm_info);
static void wlc_rrm_send_bcnreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, bcn_req_t *bcnreq);
static void wlc_rrm_send_lmreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	struct ether_addr *da);
static void wlc_rrm_send_noisereq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	rrmreq_t *noisereq);
static void wlc_rrm_send_chloadreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	rrmreq_t *chloadreq);
static void wlc_rrm_send_framereq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	framereq_t *framereq);
static void wlc_rrm_send_statreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	statreq_t *statreq);

static void wlc_rrm_process_rep(wlc_rrm_info_t *rrm_info, struct scb *scb,
	dot11_rm_action_t *rrm_rep, int rep_len);
static void wlc_rrm_parse_response(wlc_rrm_info_t *rrm_info, struct scb *scb, dot11_rm_ie_t* ie,
	int len, int count);
static void wlc_rrm_recv_bcnrep(wlc_rrm_info_t *rrm_info, struct scb *scb,
	dot11_rmrep_bcn_t *rmrep_bcn, int len);
static void wlc_rrm_recv_noiserep(wlc_rrm_info_t *rrm_info, dot11_rmrep_noise_t *rmrep_noise);
static void wlc_rrm_recv_chloadrep(wlc_rrm_info_t *rrm_info, dot11_rmrep_chanload_t *rmrep_chload);
static void wlc_rrm_recv_framerep(wlc_rrm_info_t *rrm_info, dot11_rmrep_frame_t *rmrep_frame);
static void wlc_rrm_recv_statrep(wlc_rrm_info_t *rrm_info, struct scb *scb,
	dot11_rmrep_stat_t *rmrep_stat, int len);

static void wlc_rrm_meas_end(wlc_info_t* wlc);
static void wlc_rrm_start(wlc_info_t* wlc);
static int wlc_rrm_abort(wlc_info_t* wlc);

static bool wlc_rrm_recv_rmreq_enabled(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, int body_len,
	uint8 *body);
static int wlc_rrm_bsscfg_init(void *context, wlc_bsscfg_t *bsscfg);
static void wlc_rrm_bsscfg_deinit(void *context, wlc_bsscfg_t *bsscfg);
static void wlc_rrm_scb_deinit(void *ctx, struct scb *scb);
static int wlc_rrm_scb_init(void *ctx, struct scb *scb);
#ifdef BCMDBG
static void wlc_rrm_bsscfg_dump(void *context, wlc_bsscfg_t *bsscfg, struct bcmstrbuf *b);
static void wlc_rrm_scb_dump(void *ctx, struct scb *scb, struct bcmstrbuf *b);
#else
#define wlc_rrm_bsscfg_dump NULL
#define wlc_rrm_scb_dump NULL
#endif /* BCMDBG */

static void wlc_rrm_watchdog(void *context);

/* IE mgmt */
#ifdef WL11K_AP
static uint wlc_rrm_calc_ap_ch_rep_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_rrm_write_ap_ch_rep_ie(void *ctx, wlc_iem_build_data_t *data);
static void wlc_rrm_ap_scb_state_upd(void *ctx, scb_state_upd_data_t *data);
static void wlc_rrm_add_timer(wlc_info_t *wlc, struct scb *scb);
static int wlc_assoc_parse_rrm_ie(void *ctx, wlc_iem_parse_data_t *data);
#endif /* WL11K_AP */

static uint wlc_rrm_calc_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_rrm_write_cap_ie(void *ctx, wlc_iem_build_data_t *data);

#ifdef STA
static uint wlc_rrm_calc_assoc_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_rrm_write_assoc_cap_ie(void *ctx, wlc_iem_build_data_t *data);
static int wlc_rrm_parse_rrmcap_ie(void *ctx, wlc_iem_parse_data_t *data);
static void wlc_rrm_send_nbrreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, wlc_ssid_t *ssid);
#ifdef WLASSOC_NBR_REQ
static void wlc_rrm_sta_scb_state_upd(void *ctx, scb_state_upd_data_t *data);
#endif /* WLASSOC_NBR_REQ */
#endif /* STA */

#ifdef WL11K_AP
#endif /* WL11K_AP */

#ifdef WL_FTM_11K
static int rrm_config_get(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	void *p, uint plen, void *a, int alen);
static int rrm_config_set(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	void *p, uint plen, void *a, int alen);
static int wlc_rrm_add_lci_opt(wlc_rrm_info_t *rrm_info, rrm_nbr_rep_t *nbr_rep,
	uint8 *opt_lci, int len);
static int wlc_rrm_set_self_lci(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *opt_lci,	int len);
static int wlc_rrm_get_self_lci(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *a, int alen);
static int wlc_rrm_add_civic_opt(wlc_rrm_info_t *rrm_info, rrm_nbr_rep_t *nbr_rep,
	uint8 *opt_civic, int len);
static int wlc_rrm_get_self_civic(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *a, int alen);
static int wlc_rrm_set_self_civic(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *opt_civic, int len);
static void wlc_rrm_free_self_report(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *bsscfg);

static void wlc_rrm_recv_lcireq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	dot11_meas_req_loc_t *rmreq_lci, wlc_rrm_req_t *req);
static void wlc_rrm_recv_civicreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	dot11_meas_req_loc_t *rmreq_civic, wlc_rrm_req_t *req);
#if defined(WL_PROXDETECT) && defined(WL_FTM)
static uint8 * wlc_rrm_create_lci_meas_element(uint8 *ptr, rrm_bsscfg_cubby_t *rrm_cfg,
	rrm_nbr_rep_t *nbr_rep, int *buflen);
static uint8 * wlc_rrm_create_civic_meas_element(uint8 *ptr, rrm_bsscfg_cubby_t *rrm_cfg,
	rrm_nbr_rep_t *nbr_rep, int *buflen);
static void wlc_rrm_create_nbrrep_element(rrm_bsscfg_cubby_t *rrm_cfg, uint8 **ptr,
	rrm_nbr_rep_t *nbr_rep, uint8 flags, uint8 *bufend);
static int rrm_frng_set(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	void *p, uint plen, void *a, int alen);
static int wlc_rrm_recv_frngreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	dot11_rmreq_ftm_range_t *rmreq_ftmrange, wlc_rrm_req_t *req);
static int wlc_rrm_recv_frngrep(wlc_rrm_info_t *rrm_info, struct scb *scb,
	dot11_rmrep_ftm_range_t *rmrep_ftmrange, int len);
static int wlc_rrm_send_frngreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *data, int len);
static int wlc_rrm_send_frngrep(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *data, int len);
static int wlc_rrm_create_frngrep_element(uint8 **ptr, rrm_bsscfg_cubby_t *rrm_cfg,
	frngrep_t *frngrep);
typedef int (*rrm_frng_fn_t)(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *data, int len);

static rrm_frng_fn_t rrmfrng_cmd_fn[] = {
	NULL, /* WL_RRM_FRNG_NONE */
	wlc_rrm_send_frngreq, /* WL_RRM_FRNG_SET_REQ */
	NULL /* WL_RRM_FRNG_MAX */
};
#endif /* WL_PROXDETECT && WL_FTM */
typedef int (*rrm_config_fn_t)(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *opt, int len);

static rrm_config_fn_t rrmconfig_cmd_fn[] = {
	NULL, /* WL_RRM_CONFIG_NONE */
	wlc_rrm_get_self_lci, /* WL_RRM_CONFIG_GET_LCI */
	wlc_rrm_set_self_lci, /* WL_RRM_CONFIG_SET_LCI */
	wlc_rrm_get_self_civic, /* WL_RRM_CONFIG_GET_CIVIC */
	wlc_rrm_set_self_civic, /* WL_RRM_CONFIG_SET_CIVIC */
	NULL /* WL_RRM_CONFIG_MAX */
};
#endif /* WL_FTM_11K */

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

wlc_rrm_info_t *
BCMATTACHFN(wlc_rrm_attach)(wlc_info_t *wlc)
{
	wlc_rrm_info_t *rrm_info = NULL;
#ifdef WL11K_AP
	uint16 bcnfstbmp = FT2BMP(FC_BEACON) | FT2BMP(FC_PROBE_RESP);
#endif /* WL11K_AP */
	uint16 rrmcapfstbmp =
#ifdef WL11K_AP
	        FT2BMP(FC_BEACON) |
	        FT2BMP(FC_PROBE_RESP) |
#endif /* WL11K_AP */
#ifdef WL11K_AP
	        FT2BMP(FC_ASSOC_RESP) |
	        FT2BMP(FC_REASSOC_RESP) |
#endif /* WL11K_AP */
	        0;
#ifdef STA
	uint16 assocrrmcapfstbmp = FT2BMP(FC_ASSOC_REQ) | FT2BMP(FC_REASSOC_REQ);
	uint16 arsfstbmp = FT2BMP(FC_ASSOC_RESP) | FT2BMP(FC_REASSOC_RESP);
#endif /* STA */

#ifdef WL11K_AP
	uint16 arqfstbmp = FT2BMP(FC_ASSOC_REQ) | FT2BMP(FC_REASSOC_REQ);
#endif /* WL11K_AP */

	if (!(rrm_info = (wlc_rrm_info_t *)MALLOCZ(wlc->osh, sizeof(wlc_rrm_info_t)))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}
	rrm_info->wlc = wlc;

	if ((rrm_info->rrm_state = (wlc_rrm_req_state_t *)MALLOCZ(wlc->osh,
		sizeof(wlc_rrm_req_state_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	if (!(rrm_info->rrm_timer = wl_init_timer(wlc->wl, wlc_rrm_timer, rrm_info, "rrm"))) {
		WL_ERROR(("rrm_timer init failed\n"));
		goto fail;
	}

	/* register module */
	if (wlc_module_register(wlc->pub, rrm_iovars, "rrm", rrm_info, wlc_rrm_doiovar,
	                        wlc_rrm_watchdog, NULL, NULL)) {
		WL_ERROR(("wl%d: %s wlc_module_register() failed\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* reserve cubby in the bsscfg container for per-bsscfg private data */
	if ((rrm_info->cfgh = wlc_bsscfg_cubby_reserve(wlc, sizeof(rrm_bsscfg_cubby_t),
		wlc_rrm_bsscfg_init, wlc_rrm_bsscfg_deinit, wlc_rrm_bsscfg_dump,
		(void *)rrm_info)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* reserve scb cubby */
	rrm_info->scb_handle = wlc_scb_cubby_reserve(wlc, sizeof(struct rrm_scb_cubby),
		wlc_rrm_scb_init, wlc_rrm_scb_deinit, wlc_rrm_scb_dump, (void *)rrm_info);

	if (rrm_info->scb_handle < 0) {
		WL_ERROR(("wl%d: %s wlc_scb_cubby_reserve() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register IE mgmt callbacks */
	/* calc/build */
#ifdef WL11K_AP
	/* bcn/prbrsp */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, bcnfstbmp, DOT11_MNG_AP_CHREP_ID,
	      wlc_rrm_calc_ap_ch_rep_ie_len, wlc_rrm_write_ap_ch_rep_ie, rrm_info) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, chrep in bcn\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#endif /* WL11K_AP */
	/* bcn/prbrsp/assocresp/reassocresp */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, rrmcapfstbmp, DOT11_MNG_RRM_CAP_ID,
	      wlc_rrm_calc_cap_ie_len, wlc_rrm_write_cap_ie, rrm_info) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, cap ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#ifdef STA
	/* assocreq/reassocreq */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, assocrrmcapfstbmp, DOT11_MNG_RRM_CAP_ID,
	      wlc_rrm_calc_assoc_cap_ie_len, wlc_rrm_write_assoc_cap_ie, rrm_info) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, cap ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	if (wlc_iem_add_parse_fn_mft(wlc->iemi, arsfstbmp, DOT11_MNG_RRM_CAP_ID,
	                         wlc_rrm_parse_rrmcap_ie, rrm_info) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, rrm cap ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#ifdef WLASSOC_NBR_REQ
	if (wlc_scb_state_upd_register(wlc,
		wlc_rrm_sta_scb_state_upd, (void*)rrm_info) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_scb_state_upd_register failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#endif /* WLASSOC_NBR_REQ */
#endif /* STA */

#ifdef WL11K_AP

	/* assocreq/reassocreq */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, arqfstbmp, DOT11_MNG_RRM_CAP_ID,
	                         wlc_assoc_parse_rrm_ie, wlc) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn_mft failed, RRM CAP ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	if (wlc_scb_state_upd_register(wlc,
		wlc_rrm_ap_scb_state_upd, (void*)rrm_info) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_scb_state_upd_register failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#endif /* WL11K_AP */

	return rrm_info;

fail:
	MODULE_DETACH(rrm_info, wlc_rrm_detach);
	return NULL;
}

void
BCMATTACHFN(wlc_rrm_detach)(wlc_rrm_info_t *rrm_info)
{
	wlc_info_t *wlc = rrm_info->wlc;

	wlc_module_unregister(rrm_info->wlc->pub, "rrm", rrm_info);

#ifdef STA
#ifdef WLASSOC_NBR_REQ
	wlc_scb_state_upd_unregister(wlc, wlc_rrm_sta_scb_state_upd, (void*)rrm_info);
#endif /* WLASSOC_NBR_REQ */
#endif /* STA */

#ifdef WL11K_AP
	wlc_scb_state_upd_unregister(wlc, wlc_rrm_ap_scb_state_upd, (void*)rrm_info);
#endif /* WL11K_AP */

	/* free radio measurement ioctl reports */
	if (rrm_info->rrm_timer) {
		wl_free_timer(wlc->wl, rrm_info->rrm_timer);
	}

	if (rrm_info->rrm_state) {
		MFREE(wlc->osh, rrm_info->rrm_state, sizeof(wlc_rrm_req_state_t));
	}

	MFREE(rrm_info->wlc->osh, rrm_info, sizeof(wlc_rrm_info_t));
}

void
wlc_frameaction_rrm(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, struct scb *scb,
	uint action_id, uint8 *body, int body_len, int8 rssi, ratespec_t rspec)
{
	wlc_info_t *wlc = rrm_info->wlc;

	(void)wlc;

	if (action_id == DOT11_RM_ACTION_RM_REQ)
		wlc_rrm_recv_rmreq(rrm_info, cfg, scb, body, body_len);
	else if (action_id == DOT11_RM_ACTION_LM_REQ)
		wlc_rrm_recv_lmreq(rrm_info, cfg, scb, body, body_len, rssi, rspec);
	else if (action_id == DOT11_RM_ACTION_LM_REP)
		wlc_rrm_recv_lmrep(rrm_info, cfg, body, body_len);
	else if (action_id == DOT11_RM_ACTION_RM_REP)
		wlc_rrm_recv_rmrep(rrm_info, scb, body, body_len);
#ifdef WL11K_AP
	else if (action_id == DOT11_RM_ACTION_NR_REQ)
		wlc_rrm_recv_nrreq(rrm_info, cfg, scb, body, body_len);
#endif /* WL11K_AP */

#ifdef STA
	else if (action_id == DOT11_RM_ACTION_NR_REP)
		wlc_rrm_recv_nrrep(rrm_info, cfg, body, body_len);
#endif /* STA */

	return;
}

static int
wlc_rrm_ie_count(dot11_rm_ie_t *ie, uint8 mea_id, uint8 mode_flag, int len)
{
	int count = 0;

	/* make sure this is a valid RRM IE */
	if (len < DOT11_RM_IE_LEN ||
	    len < TLV_HDR_LEN + ie->len) {
		ie = NULL;
		return count;
	}

	/* walk the req IEs counting valid RRM Requests,
	 * skipping unknown IEs or ones that just have autonomous report flags
	 */
	while (ie) {
		if (ie->id == mea_id &&
		    0 == (ie->mode & mode_flag)) {
			/* found a measurement request */
			count++;
		}
		ie = wlc_rrm_next_ie(ie, &len, mea_id);
	}
	return count;
}

static dot11_rm_ie_t *
wlc_rrm_next_ie(dot11_rm_ie_t *ie, int *len, uint8 mea_id)
{
	int buflen = *len;

	while (ie) {
		/* advance to the next IE */
		buflen -= TLV_HDR_LEN + ie->len;
		ie = (dot11_rm_ie_t *)((int8 *)ie + TLV_HDR_LEN + ie->len);

		/* make sure there is room for a valid RRM IE */
		if (buflen < DOT11_RM_IE_LEN ||
		    buflen < TLV_HDR_LEN + ie->len) {
			buflen = 0;
			ie = NULL;
			break;
		}

		if (ie->id == mea_id) {
			/* found valid measurement request/response */
			break;
		}
	}

	*len = buflen;
	return ie;
}

static void
wlc_rrm_parse_requests(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, dot11_rm_ie_t *ie,
	int len, wlc_rrm_req_t *req, int count)
{
	dot11_rmreq_bcn_t *rmreq_bcn = NULL;
	dot11_rmreq_chanload_t *rmreq_chanload = NULL;
	dot11_rmreq_noise_t *rmreq_noise = NULL;
	dot11_rmreq_frame_t *rmreq_frame = NULL;
	dot11_rmreq_stat_t *rmreq_stat = NULL;
	dot11_rmreq_tx_stream_t *rmreq_txstrm = NULL;
	char eabuf[ETHER_ADDR_STR_LEN];
	int idx = 0;
	int8 type = 0;
	uint32 parallel_flag = 0;
	rrm_bsscfg_cubby_t *rrm_cfg;

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, rrm_info->cur_cfg);
	ASSERT(rrm_cfg != NULL);

	/* convert each RRM IE into a wlc_rrm_req */
	for (; ie != NULL && idx < count;
		ie = wlc_rrm_next_ie(ie, &len, DOT11_MNG_MEASURE_REQUEST_ID)) {
		/* skip IE if we do not recongnized the request,
		 * or it was !enable (not a autonomous report req)
		 */
		if (ie->id != DOT11_MNG_MEASURE_REQUEST_ID ||
		    ie->len < (DOT11_RM_IE_LEN - TLV_HDR_LEN) ||
		    0 != (ie->mode & DOT11_RMREQ_MODE_ENABLE))
			continue;

		/* in case we skip measurement requests, keep track of the
		 * parallel bit flag separately. Clear here at the beginning of
		 * every set of measurements and set below after the first
		 * measurement req we actually send down.
		 */
		if (!(ie->mode & DOT11_RMREQ_MODE_PARALLEL)) {
			parallel_flag = 0;
		}

		switch (ie->type) {
		case DOT11_MEASURE_TYPE_BEACON:

			rmreq_bcn = (dot11_rmreq_bcn_t *)ie;
			if (rmreq_bcn->bcn_mode == DOT11_RMREQ_BCN_ACTIVE) {
				if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_ACTIVE))
					continue;
				type = DOT11_RMREQ_BCN_ACTIVE;
				req[idx].chanspec = CH20MHZ_CHSPEC(rmreq_bcn->channel);
			} else if (rmreq_bcn->bcn_mode == DOT11_RMREQ_BCN_PASSIVE) {
				if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_PASSIVE))
					continue;
				type = DOT11_RMREQ_BCN_PASSIVE;
				req[idx].chanspec = CH20MHZ_CHSPEC(rmreq_bcn->channel);
			} else if (rmreq_bcn->bcn_mode == DOT11_RMREQ_BCN_TABLE) {
				if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_TABLE))
					continue;
				type = DOT11_RMREQ_BCN_TABLE;
			} else {
				continue;
			}

			bcm_ether_ntoa(&rmreq_bcn->bssid, eabuf);
			WL_INFORM(("%s: TYPE = BEACON, subtype = %d, bssid: %s\n",
				__FUNCTION__, type, eabuf));
			wlc_rrm_recv_bcnreq(rrm_info, cfg, rmreq_bcn, &req[idx]);
			req[idx].dur = ltoh16_ua(&rmreq_bcn->duration);
			req[idx].intval = ltoh16_ua(&rmreq_bcn->interval);

			WL_INFORM(("%s: req.dur:%d, req_bcn->duration:%d\n",
				__FUNCTION__,
				req[idx].dur, rmreq_bcn->duration));
			break;
		case DOT11_MEASURE_TYPE_NOISE:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_NHM))
				continue;

			rmreq_noise = (dot11_rmreq_noise_t *)ie;
			req[idx].chanspec = CH20MHZ_CHSPEC(rmreq_noise->channel);
			type = ie->type;
			req[idx].dur = ltoh16_ua(&rmreq_noise->duration);
			req[idx].intval = ltoh16_ua(&rmreq_noise->interval);
			break;
		case DOT11_MEASURE_TYPE_CHLOAD:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_CLM))
				continue;

			rmreq_chanload = (dot11_rmreq_chanload_t *)ie;
			req[idx].chanspec = CH20MHZ_CHSPEC(rmreq_chanload->channel);
			type = ie->type;
			req[idx].dur = ltoh16_ua(&rmreq_chanload->duration);
			req[idx].intval = ltoh16_ua(&rmreq_chanload->interval);
			break;
		case DOT11_MEASURE_TYPE_FRAME:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_FM))
				continue;

			rmreq_frame = (dot11_rmreq_frame_t *)ie;
			req[idx].chanspec = CH20MHZ_CHSPEC(rmreq_frame->channel);
			type = ie->type;
			req[idx].dur = ltoh16_ua(&rmreq_frame->duration);
			req[idx].intval = ltoh16_ua(&rmreq_frame->interval);
			break;
		case DOT11_MEASURE_TYPE_STAT:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_SM))
				continue;

			rmreq_stat = (dot11_rmreq_stat_t *)ie;
			req[idx].dur = ltoh16_ua(&rmreq_stat->duration);
			req[idx].intval = ltoh16_ua(&rmreq_stat->interval);
			type = ie->type;
			break;
		case DOT11_MEASURE_TYPE_TXSTREAM:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_TSCM))
				continue;

			rmreq_txstrm = (dot11_rmreq_tx_stream_t *)ie;
			req[idx].dur = ltoh16_ua(&rmreq_txstrm->duration);
			req[idx].intval = ltoh16_ua(&rmreq_txstrm->interval);
			type = ie->type;
			break;

#ifdef WL_FTM_11K
		case DOT11_MEASURE_TYPE_LCI:
			type = ie->type;
			wlc_rrm_recv_lcireq(rrm_info, cfg, (dot11_meas_req_loc_t *)ie,
				&req[idx]);
			break;
		case DOT11_MEASURE_TYPE_CIVICLOC:
			type = ie->type;
			wlc_rrm_recv_civicreq(rrm_info, cfg, (dot11_meas_req_loc_t *)ie,
				&req[idx]);
			break;
#if defined(WL_PROXDETECT) && defined(WL_FTM)
		case DOT11_MEASURE_TYPE_FTMRANGE: {
			dot11_rmreq_ftm_range_t *rmreq_ftmrange = NULL;
			type = ie->type;
			if (!PROXD_ENAB(rrm_info->wlc->pub))
				break;
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_FTM_RANGE))
				break; /* respond even if incapable */
			rmreq_ftmrange = (dot11_rmreq_ftm_range_t *) ie;
			(void) wlc_rrm_recv_frngreq(rrm_info, cfg, rmreq_ftmrange, &req[idx]);
			req[idx].dur = 1; /* needs to be non-zero */
			req[idx].intval = ltoh16_ua(&rmreq_ftmrange->max_init_delay);
			break;
		}
#endif /* WL_PROXDETECT && WL_FTM */
#endif /* WL_FTM_11K */

		case DOT11_MEASURE_TYPE_PAUSE:

			type = ie->type;
			break;
		default:
			WL_ERROR(("%s: TYPE = Unknown TYPE, igore it\n", __FUNCTION__));

			/* unknown measurement type, do not reply */
			continue;
		}
		req[idx].type = type;
		req[idx].flags |= parallel_flag;
		req[idx].token = ltoh16(ie->token);
		/* special handling for beacon table */
		if (type == DOT11_RMREQ_BCN_TABLE) {
			req[idx].chanspec = 0;
			req[idx].dur = 0;
		}

		/* set the parallel bit now that we output a request item so
		 * that other reqs in the same parallel set will have the bit
		 * set. The flag will be cleared above when a new parallel set
		 * starts.
		 */
		parallel_flag = DOT11_RMREQ_MODE_PARALLEL;

		idx++;
	}
}

static int
wlc_rrm_state_init(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, uint16 rx_time,
	dot11_rmreq_t *rm_req, int req_len)
{
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	uint32 start_time_h;
	uint32 start_time_l;
	int rrm_req_size;
	int rrm_req_count;
	wlc_rrm_req_t *rrm_req;
	dot11_rm_ie_t *req_ie;
	uint8 *req_pkt;
	int body_len;
	wlc_info_t *wlc = rrm_info->wlc;

	if (rrm_state->req) {
		WL_ERROR(("wl%d: %s: Already servicing another RRM request\n", wlc->pub->unit,
			__FUNCTION__));
		return BCME_BUSY;
	}
	/* Along with rrm_state->req, wlc_rrm_free() also frees rrm_info->bcnreq so assert that is
	 * clear if rrm_state->req is NULL.
	 */
	ASSERT(rrm_info->bcnreq == NULL);
	body_len = req_len;
	req_pkt = (uchar *)rm_req;
	req_pkt += DOT11_RMREQ_LEN;
	body_len -= DOT11_RMREQ_LEN;

	req_ie = (dot11_rm_ie_t *)req_pkt;
	rrm_req_count = wlc_rrm_ie_count(req_ie, DOT11_MNG_MEASURE_REQUEST_ID,
		DOT11_RMREQ_MODE_ENABLE, body_len);

	if (rrm_req_count == 0) {
		WL_ERROR(("%s: request count == 0\n", __FUNCTION__));
		return BCME_ERROR;
	}
	if (rm_req->reps < WLC_RRM_MAX_REP_NUM)
		rrm_state->reps = rm_req->reps;

	rrm_req_size = rrm_req_count * sizeof(wlc_rrm_req_t);
	rrm_req = (wlc_rrm_req_t *)MALLOCZ(wlc->osh, rrm_req_size);
	if (rrm_req == NULL) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__, rrm_req_size,
			MALLOCED(wlc->osh)));
		return BCME_NORESOURCE;
	}

	rrm_state->report_class = WLC_RRM_CLASS_11K;
	rrm_state->token = ltoh16(rm_req->token);
	rrm_info->dialog_token = rrm_state->token;
	rrm_state->req_count = rrm_req_count;
	rrm_state->req = rrm_req;

	/* Fill out the request blocks */
	wlc_rrm_parse_requests(rrm_info, cfg, req_ie, body_len, rrm_req, rrm_req_count);

	/* Compute the start time of the measurements,
	 * Req frame has a delay in TBTT times, and an offset in TUs
	 * from the delay time.
	 */
	start_time_l = 0;
	start_time_h = 0;

	/* The measurement request frame has one start time for the set of
	 * measurements, but the driver state allows for a start time for each
	 * measurement. Set the measurement request start time into the
	 * first of the driver measurement requests, and leave the following
	 * start times as zero to indicate they happen as soon as possible.
	 */
	rrm_req[0].tsf_h = start_time_h;
	rrm_req[0].tsf_l = start_time_l;

	return BCME_OK;
}

static void
wlc_rrm_recv_rmreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, struct scb *scb, uint8 *body,
	int body_len)
{
	dot11_rmreq_t *rmreq;
	uint16 rxtime = 0;

	rmreq = (dot11_rmreq_t *)body;

	if (body_len <= DOT11_RMREQ_LEN) {
		WL_ERROR(("wl%d: %s invalid body len %d in rmreq\n", rrm_info->wlc->pub->unit,
		          __FUNCTION__, body_len));
		WLCNTINCR(rrm_info->wlc->pub->_cnt->rxbadproto);
		return;
	}

	if (!wlc_rrm_recv_rmreq_enabled(rrm_info, cfg, body_len, body)) {
		WL_ERROR(("wl%d: %s Features are Unsupported\n", rrm_info->wlc->pub->unit,
			__FUNCTION__));
		return;
	}

	WL_INFORM(("%s: abort current RM request due to the comming of new req\n", __FUNCTION__));

	wlc_rrm_abort(rrm_info->wlc);

	rrm_info->cur_cfg = cfg;
	bcopy(&scb->ea, &rrm_info->da, ETHER_ADDR_LEN);
	/* Parse measurement requests */
	if (wlc_rrm_state_init(rrm_info, cfg, rxtime, rmreq, body_len)) {
		WL_ERROR(("wl%s: failed to initialize radio "
			  "measurement state, dropping request\n", __FUNCTION__));
		return;
	}
	/* Start rrm state machine */
	wlc_rrm_start(rrm_info->wlc);
}

static void
wlc_rrm_parse_response(wlc_rrm_info_t *rrm_info, struct scb *scb, dot11_rm_ie_t *ie,
int len, int count)
{
	int idx = 0;
	bool incap = FALSE;
	bool refused = FALSE;
	dot11_rmrep_bcn_t *rmrep_bcn;
	dot11_rmrep_chanload_t *rmrep_chload;
	dot11_rmrep_noise_t *rmrep_noise;
	dot11_rmrep_frame_t *rmrep_frame;
	dot11_rmrep_stat_t *rmrep_stat;

	for (; ie != NULL && idx < count;
		ie = wlc_rrm_next_ie(ie, &len, DOT11_MNG_MEASURE_REPORT_ID)) {

		if (ie->id != DOT11_MNG_MEASURE_REPORT_ID ||
		    ie->len < (DOT11_RM_IE_LEN - TLV_HDR_LEN)) {
			WL_ERROR(("%s: ie->id: %d not DOT11_MNG_MEASURE_REPORT_ID\n",
				__FUNCTION__, ie->id));
			continue;
		}
		if (ie->mode & DOT11_RMREP_MODE_INCAPABLE)
			incap = TRUE;

		if (ie->mode & DOT11_RMREP_MODE_REFUSED)
			refused = TRUE;

		if (incap || refused) {
			WL_ERROR(("%s: type: %s\n",
				__FUNCTION__, incap ? "INCAPABLE" : "REFUSED"));
			continue;
		}
		switch (ie->type) {
			case DOT11_MEASURE_TYPE_BEACON:
				if (ie->len > (DOT11_RM_IE_LEN - TLV_HDR_LEN)) {
					rmrep_bcn = (dot11_rmrep_bcn_t *)&ie[1];
					wlc_rrm_recv_bcnrep(rrm_info, scb, rmrep_bcn, len);
				}
				else
					WL_ERROR(("%s: Null Beacon resp\n", __FUNCTION__));
				break;
			case DOT11_MEASURE_TYPE_NOISE:
				rmrep_noise = (dot11_rmrep_noise_t *)&ie[1];
				wlc_rrm_recv_noiserep(rrm_info, rmrep_noise);
				break;

			case DOT11_MEASURE_TYPE_CHLOAD:
				rmrep_chload = (dot11_rmrep_chanload_t *)&ie[1];
				wlc_rrm_recv_chloadrep(rrm_info, rmrep_chload);
				break;

			case DOT11_MEASURE_TYPE_FRAME:
				rmrep_frame = (dot11_rmrep_frame_t *)&ie[1];

				wlc_rrm_recv_framerep(rrm_info, rmrep_frame);
				break;

			case DOT11_MEASURE_TYPE_STAT:
				rmrep_stat = (dot11_rmrep_stat_t *)&ie[1];
				wlc_rrm_recv_statrep(rrm_info, scb, rmrep_stat, len);
				break;

			case DOT11_MEASURE_TYPE_TXSTREAM:
				break;

#ifdef WL_FTM_11K
#if defined(WL_PROXDETECT) && defined(WL_FTM)
			case DOT11_MEASURE_TYPE_FTMRANGE: {
				rrm_bsscfg_cubby_t *rrm_cfg;
				dot11_rmrep_ftm_range_t *rmrep_ftmrange;
				if (!PROXD_ENAB(rrm_info->wlc->pub))
					break;
				rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, scb->bsscfg);
				if (rrm_cfg != NULL && !isset(rrm_cfg->rrm_cap,
					DOT11_RRM_CAP_FTM_RANGE))
					continue;
				rmrep_ftmrange = (dot11_rmrep_ftm_range_t *) ie;
				(void) wlc_rrm_recv_frngrep(rrm_info, scb, rmrep_ftmrange, len);
				break;
			}
#endif /* WL_PROXDETECT && WL_FTM */
#endif /* WL_FTM_11K */

			case DOT11_MEASURE_TYPE_PAUSE:
				break;

			default:
				WL_ERROR(("%s: TYPE = Unknown TYPE, igore it\n", __FUNCTION__));
				/* unknown type */
				continue;
				break;
		}
	}
}

static void
wlc_rrm_process_rep(wlc_rrm_info_t *rrm_info, struct scb *scb, dot11_rm_action_t *rrm_rep,
int rep_len)
{
	uchar *rep_pkt;
	dot11_rm_ie_t *rep_ie;
	uint rrm_rep_count, body_len = rep_len;

	rep_pkt = (uchar *)rrm_rep;
	rep_pkt += DOT11_RM_ACTION_LEN;
	body_len -= DOT11_RM_ACTION_LEN;

	rep_ie = (dot11_rm_ie_t *)rep_pkt;
	rrm_rep_count = wlc_rrm_ie_count(rep_ie, DOT11_MNG_MEASURE_REPORT_ID,
		0xff, body_len);

	if (rrm_rep_count == 0) {
		WL_ERROR(("%s: response count == 0\n", __FUNCTION__));
		return;
	}

	wlc_rrm_parse_response(rrm_info, scb, rep_ie, body_len, rrm_rep_count);
}

static void
wlc_rrm_recv_rmrep(wlc_rrm_info_t *rrm_info, struct scb *scb, uint8 *body, int body_len)
{
	dot11_rm_action_t *rrm_rep;

	rrm_rep = (dot11_rm_action_t *)body;

	if (body_len <= DOT11_RM_ACTION_LEN) {
		WL_ERROR(("wl%d: %s invalid body len %d in rmrep\n", rrm_info->wlc->pub->unit,
		          __FUNCTION__, body_len));
		WLCNTINCR(rrm_info->wlc->pub->_cnt->rxbadproto);
		return;
	}

	wlc_rrm_process_rep(rrm_info, scb, rrm_rep, body_len);

	return;
}

/* Converts all current reg domain channels into chanspec list for scanning. */
static bool
wlc_rrm_request_current_regdomain(wlc_rrm_info_t *rrm_info, dot11_rmreq_bcn_t *rmreq_bcn,
	rrm_bcnreq_t *bcn_req, wlc_rrm_req_t *req)
{
	wlc_info_t *wlc = rrm_info->wlc;
	wl_uint32_list_t *chanlist;
	uint32 req_len;
	unsigned int i;

	req_len = OFFSETOF(wl_uint32_list_t, element) + (sizeof(chanlist->element[0]) * MAXCHANNEL);
	if ((chanlist = (wl_uint32_list_t *)MALLOCZ(wlc->osh, req_len)) == NULL) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__, (int)req_len,
			MALLOCED(wlc->osh)));
		req->flags |= DOT11_RMREP_MODE_REFUSED;
		return FALSE;
	}

	if ((wlc_rclass_get_channel_list(wlc->cmi,
		wlc_channel_country_abbrev(wlc->cmi), rmreq_bcn->reg, TRUE,
		chanlist)) != 0) {
		bcn_req->channel_num = (uint16)chanlist->count;
		for (i = 0; i < chanlist->count; i++)
			bcn_req->chanspec_list[i] = CH20MHZ_CHSPEC(chanlist->element[i]);
	}
	MFREE(wlc->osh, chanlist, req_len);

	return TRUE;
}

/* Searches TLVs for AP Channel Reports and adds channels to the beacon request chanspec. */
static unsigned int
wlc_rrm_ap_chreps_to_chanspec(bcm_tlv_t *tlvs, int tlvs_len, rrm_bcnreq_t *bcn_req)
{
	unsigned int channel_count = 0;
	dot11_ap_chrep_t *ap_chrep = (dot11_ap_chrep_t *)tlvs;

	while (ap_chrep) {
		if (ap_chrep->id == DOT11_MNG_AP_CHREP_ID) {
			/* Subtract 1 byte for regclass. */
			unsigned int channel_num = ap_chrep->len - 1;
			unsigned int i;

			/* validate channel against regclass */
			WL_NONE(("AP Chanrep found "));
			for (i = 0; i < channel_num; i++) {
				bcn_req->chanspec_list[channel_count++] =
					CH20MHZ_CHSPEC(ap_chrep->chanlist[i]);
				WL_NONE(("%d ", ap_chrep->chanlist[i]));
			}
			WL_NONE(("\n"));
		}
		ap_chrep = (dot11_ap_chrep_t *)bcm_next_tlv((bcm_tlv_t*)ap_chrep, &tlvs_len);
	}

	return channel_count;
}

static int
wlc_rrm_recv_bcnreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, dot11_rmreq_bcn_t *rmreq_bcn,
	wlc_rrm_req_t *req)
{
	wlc_info_t *wlc = rrm_info->wlc;
	int tlv_len;
	unsigned int scan_type;
	bcm_tlv_t *tlv = NULL;
	uint8 *tlvs;
	rrm_bcnreq_t *bcn_req;
	bool do_scan = TRUE;

	if (rmreq_bcn->mode & (DOT11_RMREQ_MODE_PARALLEL | DOT11_RMREQ_MODE_ENABLE)) {
		WL_ERROR(("wl%d: %s: Unsupported beacon request mode 0x%x\n",
			wlc->pub->unit, __FUNCTION__, rmreq_bcn->mode));
		req->flags |= DOT11_RMREP_MODE_INCAPABLE;
		return BCME_UNSUPPORTED;
	}

	switch (rmreq_bcn->bcn_mode) {
		case DOT11_RMREQ_BCN_PASSIVE:
			scan_type = DOT11_SCANTYPE_PASSIVE;
			break;
		case DOT11_RMREQ_BCN_ACTIVE:
			scan_type = DOT11_SCANTYPE_ACTIVE;
			break;
		case DOT11_RMREQ_BCN_TABLE:
			scan_type = DOT11_RMREQ_BCN_TABLE;
			do_scan = FALSE;
			break;
		default:
			req->flags |= DOT11_RMREP_MODE_INCAPABLE;
			WL_ERROR(("wl%d: %s: Invalid beacon mode 0x%x in rmreq\n",
				wlc->pub->unit, __FUNCTION__, rmreq_bcn->bcn_mode));
			return BCME_ERROR;
	}
	if ((rrm_info->bcnreq = (rrm_bcnreq_t *)MALLOCZ(wlc->osh, sizeof(rrm_bcnreq_t))) == NULL) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__, (int)sizeof(rrm_bcnreq_t),
			MALLOCED(wlc->osh)));
		req->flags |= DOT11_RMREP_MODE_REFUSED;
		return BCME_NOMEM;
	}

	bcn_req = rrm_info->bcnreq;
	bcn_req->token = rmreq_bcn->token;
	bcn_req->reg = rmreq_bcn->reg;
	bcn_req->channel = rmreq_bcn->channel;
	bcn_req->rep_detail = DOT11_RMREQ_BCN_REPDET_ALL;
	bcn_req->scan_type = scan_type;

	/* 802.11k measurement duration: 11.10.3
	 * Convert 802.11 Time Unit (TU) to ms for use in local scan timers.
	 */
	bcn_req->duration = (ltoh16_ua(&rmreq_bcn->duration) * DOT11_TU_TO_US) / 1000;
	if (bcn_req->duration > WLC_SCAN_AWAY_LIMIT) {
		if (rmreq_bcn->mode & DOT11_RMREQ_MODE_DURMAND) {
			WL_INFORM(("wl%d: %s: mandatory duration %d longer than"
				" WLC_SCAN_AWAY_LIMIT\n",
				wlc->pub->unit, __FUNCTION__, bcn_req->duration));
		} else
			bcn_req->duration = WLC_SCAN_AWAY_LIMIT;
	}

	if (do_scan) {
		if (WLC_RRM_REPORT_ALL_CHANNELS(rmreq_bcn)) {
			wlc_rrm_request_current_regdomain(rrm_info, rmreq_bcn, bcn_req, req);
		}
		else if (WLC_RRM_CHANNEL_REPORT_MODE(rmreq_bcn) == FALSE) {
			bcn_req->channel_num = 1;
			bcn_req->chanspec_list[0] = CH20MHZ_CHSPEC(rmreq_bcn->channel);
		}
		/* else: AP Channel Report mode is handled in subelement parsing section.
		 *
		 * We'll first search for the AP Channel Report IE in this frame, if none
		 * is found we'll fallback to the AP Channel Report in the beacon.
		 */
	}

	bcopy(&rmreq_bcn->bssid, &bcn_req->bssid, sizeof(struct ether_addr));
	bzero(&bcn_req->ssid, sizeof(wlc_ssid_t));

	/* parse (mandatory subset) optional subelements */
	tlv_len = rmreq_bcn->len - (DOT11_RMREQ_BCN_LEN - TLV_HDR_LEN);

	if (tlv_len > 0) {
		tlvs = (uint8 *)&rmreq_bcn[1];

		tlv = bcm_parse_tlvs(tlvs, tlv_len, DOT11_RMREQ_BCN_SSID_ID);
		if (tlv) {
			bcn_req->ssid.SSID_len = MIN(DOT11_MAX_SSID_LEN, tlv->len);
			bcopy(tlv->data, bcn_req->ssid.SSID, bcn_req->ssid.SSID_len);
		}

		tlv = bcm_parse_tlvs(tlvs, tlv_len, DOT11_RMREQ_BCN_REPDET_ID);
		if (tlv && tlv->len > 0)
			bcn_req->rep_detail = tlv->data[0];
		if (bcn_req->rep_detail == DOT11_RMREQ_BCN_REPDET_REQUEST) {
			tlv = bcm_parse_tlvs(tlvs, tlv_len, DOT11_RMREQ_BCN_REQUEST_ID);
			if (tlv) {
				bcn_req->req_eid_num = MIN(tlv->len, WLC_RRM_REQUEST_ID_MAX);
				WL_INFORM(("%s: Got DOT11_RMREQ_BCN_REQUEST_ID in request: "
					  "bcn_req->req_eid_num %d, tlv->len %d, "
					  "WLC_RRM_REQUEST_ID_MAX %d\n", __FUNCTION__,
					  bcn_req->req_eid_num, tlv->len, WLC_RRM_REQUEST_ID_MAX));
				bcopy(tlv->data, &bcn_req->req_eid[0], bcn_req->req_eid_num);
			} else
				WL_INFORM(("%s: No DOT11_RMREQ_BCN_REQUEST_ID in request\n",
					__FUNCTION__));
		} else
			WL_INFORM(("%s: bcn_req->rep_detail %d != DOT11_RMREQ_BCN_REPDET_REQUEST\n",
				__FUNCTION__, bcn_req->rep_detail));

		/* Only look for AP Channel Report IEs if we're scanning and in the correct mode. */
		if (do_scan && WLC_RRM_CHANNEL_REPORT_MODE(rmreq_bcn)) {
			/* Convert multiple AP chan report IEs to chanspec. */
			bcn_req->channel_num = wlc_rrm_ap_chreps_to_chanspec((bcm_tlv_t*)tlvs,
				tlv_len, bcn_req);
		}
	}

	if (do_scan && WLC_RRM_CHANNEL_REPORT_MODE(rmreq_bcn) && (bcn_req->channel_num == 0)) {
		/* Fallback to channels in AP Channel Report from Beacon Frame if no AP Channel
		 * Reports were included in this Beacon Request frame.
		 */
		/* Get beacon frame from current bss. */
		wlc_bss_info_t *bi = cfg->current_bss;
		dot11_ap_chrep_t *ap_chrep;

		ap_chrep = wlc_rrm_get_ap_chrep(bi);
		if (ap_chrep) {
			bcn_req->channel_num = wlc_rrm_ap_chreps_to_chanspec((bcm_tlv_t*)ap_chrep,
				tlv_len, bcn_req);
		}
		/* Fallback to all channels in the current regulatory domain as there were no AP
		 * Channel Reports in the Beacon Request or Beacon.
		 */
		if (bcn_req->channel_num == 0) {
			wlc_rrm_request_current_regdomain(rrm_info, rmreq_bcn, bcn_req, req);
		}
	}
	if (rmreq_bcn->bcn_mode < DOT11_RMREQ_BCN_TABLE &&
		rrm_info->bcn_req_thrtl_win) {
		bool refuse = FALSE;

		/* Fallback to 1 bcn req / N seconds mode if no off_chan_time allowed */
		if (!rrm_info->bcn_req_off_chan_time_allowed && rrm_info->bcn_req_win_scan_cnt) {
			refuse = TRUE;
		}

		/* If this req causes to exceed allowed time, refuse */
		if (rrm_info->bcn_req_off_chan_time_allowed && (rrm_info->bcn_req_off_chan_time +
			(bcn_req->duration * bcn_req->channel_num)) >
			rrm_info->bcn_req_off_chan_time_allowed) {
			refuse = TRUE;
		}
		if (refuse == TRUE) {
			WL_ERROR(("wl%d: %s: reject bcn req allowed:%d spent:%d scan_cnt:%d\n",
				wlc->pub->unit, __FUNCTION__,
				rrm_info->bcn_req_off_chan_time_allowed,
				rrm_info->bcn_req_off_chan_time,
				rrm_info->bcn_req_win_scan_cnt));
			req->flags |= DOT11_RMREP_MODE_REFUSED;

			/* rrm_info->bcn_req will be freed at the end of state machine */
			return BCME_ERROR;
		}
		rrm_info->bcn_req_scan_start_timestamp = OSL_SYSUPTIME();
	}
	return BCME_OK;
}

static void
wlc_rrm_recv_bcnrep(wlc_rrm_info_t *rrm_info, struct scb *scb,
	dot11_rmrep_bcn_t *rmrep_bcn, int len)
{
	char eabuf[ETHER_ADDR_STR_LEN];
#ifdef WL11K_AP
	bcm_tlv_t *tlv = NULL;
	int tlv_len = len - DOT11_RMREP_BCN_LEN;
	uint8 *tlvs = (uint8 *)&rmrep_bcn[1];
	struct dot11_bcn_prb bcn;
	wlc_ssid_t report_ssid;
	bcm_tlv_t *wpaie = NULL;
	bcm_tlv_t *wpa2ie = NULL;
	bcm_tlv_t *htie = NULL;
	bcm_tlv_t *mdie = NULL;
	uint32 flags = 0;
	bcm_tlv_t *wme_ie;
#endif /* WL11K_AP */

	bcm_ether_ntoa(&rmrep_bcn->bssid, eabuf);
#ifdef WL11K_AP
	bzero(&report_ssid, sizeof(wlc_ssid_t));
	bzero(&bcn, sizeof(bcn));

	/* look for sub elements */
	tlv = (bcm_tlv_t *)&rmrep_bcn[1];
	if (tlv->id == DOT11_RMREP_BCN_FRM_BODY && tlv_len >= (TLV_HDR_LEN + DOT11_BCN_PRB_LEN)) {
		tlvs += TLV_HDR_LEN;
		tlv_len -= TLV_HDR_LEN;
		/* copy fixed beacon params */
		bcopy(tlvs, &bcn, sizeof(bcn));
		bcopy(&bcn.timestamp[1], &rmrep_bcn->parent_tsf, sizeof(uint32));

		tlvs += DOT11_BCN_PRB_LEN;
		tlv_len -= DOT11_BCN_PRB_LEN;

		/* look for ies within bcn frame */
		tlv = bcm_parse_tlvs(tlvs, tlv_len, DOT11_MNG_SSID_ID);
		if (tlv) {
			report_ssid.SSID_len = MIN(DOT11_MAX_SSID_LEN, tlv->len);
			bcopy(tlv->data, report_ssid.SSID, report_ssid.SSID_len);
		}
		htie = bcm_parse_tlvs(tlvs, tlv_len, DOT11_MNG_HT_CAP);
		mdie = bcm_parse_tlvs(tlvs, tlv_len, DOT11_MNG_MDIE_ID);
		wpa2ie = bcm_parse_tlvs(tlvs, tlv_len, DOT11_MNG_RSN_ID);
		if (bcm_find_vendor_ie(tlvs, tlv_len, BRCM_OUI, NULL, 0) != NULL)
			flags |= WLC_BSS_BRCM;

		if ((wme_ie = wlc_find_wme_ie(tlvs, tlv_len)) != NULL) {
			flags |= WLC_BSS_WME;
		}
		wpaie = (bcm_tlv_t *)bcm_find_wpaie(tlvs, tlv_len);
	}
#endif /* WL11K_AP */
	WL_ERROR(("%s: channel :%d, duration: %d, frame info: %d, rcpi: %d,"
		"rsni: %d, bssid: %s, antenna id: %d, parent tsf: %d\n",
		__FUNCTION__, rmrep_bcn->channel, rmrep_bcn->duration, rmrep_bcn->frame_info,
		rmrep_bcn->rcpi, rmrep_bcn->rsni, eabuf,
		rmrep_bcn->antenna_id, rmrep_bcn->parent_tsf));
#ifdef WL11K_AP
	/* check if SSID and security match with this BSSID */
	if ((report_ssid.SSID_len == scb->bsscfg->SSID_len) &&
		(memcmp(&report_ssid.SSID, &scb->bsscfg->SSID, report_ssid.SSID_len) == 0) &&
		(memcmp(&rmrep_bcn->bssid, &scb->bsscfg->BSSID, ETHER_ADDR_LEN) != 0)) {

		if (wlc_rrm_security_match(rrm_info, scb->bsscfg, bcn.capability,
			wpaie, wpa2ie)) {
			WL_NONE(("%s: matched BSSID :%s, SSID: %s\n", __FUNCTION__, eabuf,
				report_ssid.SSID));
			wlc_rrm_add_bcnnbr(rrm_info, scb, rmrep_bcn, &bcn, htie, mdie, flags);
		}
	}
#endif /* WL11K_AP */
}

static void
wlc_rrm_recv_noiserep(wlc_rrm_info_t *rrm_info, dot11_rmrep_noise_t *rmrep_noise)
{
	WL_ERROR(("%s\n", __FUNCTION__));
}

static void
wlc_rrm_recv_chloadrep(wlc_rrm_info_t *rrm_info, dot11_rmrep_chanload_t *rmrep_chload)
{
	WL_ERROR(("%s: regulatory: %d, channel :%d, duration: %d, chan load: %d\n",
		__FUNCTION__, rmrep_chload->reg, rmrep_chload->channel,
		rmrep_chload->duration, rmrep_chload->channel_load));
}

static void
wlc_rrm_recv_framerep(wlc_rrm_info_t *rrm_info, dot11_rmrep_frame_t *rmrep_frame)
{
	WL_ERROR(("%s\n", __FUNCTION__));
}

static void
wlc_rrm_recv_statrep(wlc_rrm_info_t *rrm_info, struct scb *scb, dot11_rmrep_stat_t *rmrep_stat,
	int len)
{
	rrm_scb_info_t *scb_info;
	int header_len = sizeof(dot11_rmrep_stat_t) + DOT11_RM_IE_LEN;
	int stats_len;
	char *buf;
	bcm_tlv_t *vendor_ie;

	if (scb == NULL) {
		WL_ERROR(("wl%d: %s: scb==NULL\n",
			rrm_info->wlc->pub->unit, __FUNCTION__));
		return;
	}

	scb_info = RRM_SCB_INFO(rrm_info, scb);

	if (!scb_info) {
		WL_ERROR(("wl%d: %s: "MACF" scb_info is NULL\n",
			rrm_info->wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea)));
		return;
	}

	scb_info->timestamp = OSL_SYSUPTIME();
	scb_info->flag = WL_RRM_RPT_FALG_ERR;
	scb_info->len = 0;

	switch (rmrep_stat->group_id) {
	case DOT11_MNG_PROPR_ID:
		WL_INFORM(("wl%d: ie len:%d: min:%d max:%d\n", rrm_info->wlc->pub->unit, len,
			WL_RRM_RPT_MIN_PAYLOAD + header_len,
			WL_RRM_RPT_MAX_PAYLOAD + header_len));

		if ((len < WL_RRM_RPT_MIN_PAYLOAD + header_len) ||
			(len >= WL_RRM_RPT_MAX_PAYLOAD + header_len))
		{
			WL_ERROR(("wl%d: ie len:%d min:%d max:%d\n", rrm_info->wlc->pub->unit, len,
			WL_RRM_RPT_MIN_PAYLOAD + header_len,
			WL_RRM_RPT_MAX_PAYLOAD + header_len));
			break;
		}

		scb_info->flag = WL_RRM_RPT_FALG_GRP_ID_PROPR;
		scb_info->len = len - header_len;
		memcpy(scb_info->data, (void *)&rmrep_stat[1], scb_info->len);
		break;

	case DOT11_RRM_STATS_GRP_ID_0:
		stats_len = sizeof(dot11_rmrep_stat_t) + DOT11_RRM_STATS_RPT_LEN_GRP_ID_0;
		if (len <= stats_len) {
			WL_ERROR(("wl%d: len[%d] < stats_len[%d]\n",
				rrm_info->wlc->pub->unit, len, stats_len));
			break;
		}

		buf = (char *)rmrep_stat + stats_len;
		vendor_ie = (bcm_tlv_t *)buf;

		if (vendor_ie->id != DOT11_MNG_VS_ID) {
			WL_ERROR(("wl%d: %s: Not found Valid Vendor IE\n",
				rrm_info->wlc->pub->unit, __FUNCTION__));
		}
		else {
			scb_info->len = vendor_ie->len;
			memcpy(scb_info->data, (void *)vendor_ie->data, vendor_ie->len);
			scb_info->flag = WL_RRM_RPT_FALG_GRP_ID_0;
		}
		break;

	default:
		break;
	}
}

static void
wlc_rrm_begin_scan(wlc_rrm_info_t *rrm_info)
{
	rrm_bcnreq_t *bcn_req;
	uint32 ret;
	char eabuf[ETHER_ADDR_STR_LEN];
	wlc_info_t *wlc = rrm_info->wlc;
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;

	bcn_req = rrm_info->bcnreq;
	ASSERT(bcn_req != NULL);
	wlc_bss_list_free(wlc, &bcn_req->scan_results);
	bzero(bcn_req->scan_results.ptrs, MAXBSS * sizeof(wlc_bss_info_t *));

	wlc_read_tsf(wlc, &bcn_req->start_tsf_l, &bcn_req->start_tsf_h);

	if (bcn_req->scan_type == DOT11_RMREQ_BCN_TABLE) {
		WL_ERROR(("%s: type = BCN_TABLE, defer it to WLC_RRM_WAIT_END_MEAS\n",
		__FUNCTION__));
	}
	else {
		bcm_ether_ntoa(&bcn_req->bssid, eabuf);
		/* clear stale scan_results */
		wlc_bss_list_free(wlc, wlc->scan_results);
		WL_ERROR(("%s: duration: %d, channel num: %d, bssid: %s\n", __FUNCTION__,
			bcn_req->duration, bcn_req->channel_num,
			eabuf));
		ret = wlc_scan_request(wlc, DOT11_BSSTYPE_ANY, &bcn_req->bssid,
		                       1, &bcn_req->ssid,
		                       bcn_req->scan_type, 1, bcn_req->duration, bcn_req->duration,
		                       -1, bcn_req->chanspec_list, bcn_req->channel_num,
		                       TRUE, wlc_rrm_bcnreq_scancb, rrm_info);
		if (ret != BCME_OK)
			return;
		rrm_state->scan_active = TRUE;

		if (rrm_info->bcn_req_thrtl_win) {
			/* Start a new win if no win is running */
			if (!rrm_info->bcn_req_thrtl_win_sec) {
				rrm_info->bcn_req_thrtl_win_sec = rrm_info->bcn_req_thrtl_win;
			}
		}

	}
}

#ifdef WL11K_AP
static void
wlc_rrm_add_ap_chrep(wlc_rrm_info_t *rrm, wlc_bsscfg_t *cfg, uint8 *bufstart)
{
	wlc_info_t *wlc = rrm->wlc;
	reg_nbr_count_t *nbr_cnt;
	uint8 ap_reg;
	uint8 chan_count = 0;
	int ch;
	rrm_bsscfg_cubby_t *rrm_cfg;


	rrm_cfg = RRM_BSSCFG_CUBBY(rrm, cfg);
	ASSERT(rrm_cfg != NULL);

	/* Get AP current regulatory class */
	ap_reg = wlc_get_regclass(wlc->cmi, cfg->current_bss->chanspec);

	nbr_cnt = rrm_cfg->nbr_cnt_head;
	while (nbr_cnt) {
		uint8 buf[TLV_HDR_LEN + 255];
		dot11_ap_chrep_t *ap_chrep = (dot11_ap_chrep_t *)buf;

		bzero(ap_chrep, sizeof(buf));

		/* check if the regulatory matches AP's regulatory class */
		if (nbr_cnt->reg == ap_reg) {
			/* Parse the channels found in this regulatory class */
			for (ch = 0; ch < MAXCHANNEL; ch++) {
				if (nbr_cnt->nbr_count_by_channel[ch] > 0) {
					ap_chrep->chanlist[chan_count] = ch;
					chan_count++;
				}
			}

			if (chan_count > 0) {
				ap_chrep->reg = nbr_cnt->reg;

				bcm_write_tlv(DOT11_MNG_AP_CHREP_ID, &ap_chrep->reg,
					chan_count + 1, bufstart);
			}
			break;
		}
		/* Move to the next regulatory class */
		nbr_cnt = nbr_cnt->next;
	}
}

/* calculate length of AP Channel Report element */
static uint
wlc_rrm_ap_chrep_len(wlc_rrm_info_t *rrm, wlc_bsscfg_t *cfg)
{
	reg_nbr_count_t *nbr_cnt;
	rrm_bsscfg_cubby_t *rrm_cfg;
	wlc_info_t *wlc = rrm->wlc;
	uint8 ap_reg;
	int ch;
	uint len = 0;

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm, cfg);
	ASSERT(rrm_cfg != NULL);

	/* Get AP current regulatory class */
	ap_reg = wlc_get_regclass(wlc->cmi, cfg->current_bss->chanspec);

	nbr_cnt = rrm_cfg->nbr_cnt_head;
	while (nbr_cnt) {
		/* check if the regulatory matches AP's regulatory class */
		if (nbr_cnt->reg == ap_reg) {
			len += TLV_HDR_LEN + 1;
			for (ch = 0; ch < MAXCHANNEL; ch++) {
				if (nbr_cnt->nbr_count_by_channel[ch] > 0) {
					len++;
				}
			}
			break;
		}
		nbr_cnt = nbr_cnt->next;
	}
	return len;
}
#endif	/* WL11K_AP */

/* Return a pointer to the AP Channel Report IE if it's in the beacon for the bss. */
static dot11_ap_chrep_t*
wlc_rrm_get_ap_chrep(const wlc_bss_info_t *bi)
{
	dot11_ap_chrep_t *ap_chrep = NULL;
	bcm_tlv_t *tlv;

	if (bi->bcn_prb && (bi->bcn_prb_len >= DOT11_BCN_PRB_LEN)) {
		tlv = (bcm_tlv_t *)((uint8 *)bi->bcn_prb + DOT11_BCN_PRB_FIXED_LEN);
		ap_chrep = (dot11_ap_chrep_t*)bcm_parse_tlvs((uint8 *)tlv, bi->bcn_prb_len -
			DOT11_BCN_PRB_FIXED_LEN, DOT11_MNG_AP_CHREP_ID);
	}

	return ap_chrep;
}

/* Return the Regulatory Class from the AP Channel Report for the BSS if present, or
 * DOT11_OP_CLASS_NONE if not.
 */
static uint8
wlc_rrm_get_ap_chrep_reg(const wlc_bss_info_t *bi)
{
	uint8 reg = DOT11_OP_CLASS_NONE;
	dot11_ap_chrep_t *ap_chrep = NULL;

	ap_chrep = (dot11_ap_chrep_t*)wlc_rrm_get_ap_chrep(bi);
	if (ap_chrep)
		reg = ap_chrep->reg;

	return reg;
}

/* Adds an empty beacon report if there's room. */
static int
wlc_rrm_add_empty_bcnrep(const wlc_rrm_info_t *rrm_info, uint8 *bufptr, uint buflen)
{
	dot11_rm_ie_t *rmrep_ie;

	if (buflen < DOT11_RM_IE_LEN) {
		return 0;
	}

	rmrep_ie = (dot11_rm_ie_t *)bufptr;
	rmrep_ie->len = DOT11_RM_IE_LEN - 2;
	rmrep_ie->id = DOT11_MNG_MEASURE_REPORT_ID;
	rmrep_ie->token = rrm_info->bcnreq->token;
	rmrep_ie->mode = 0;
	rmrep_ie->type = DOT11_MEASURE_TYPE_BEACON;

	return DOT11_RM_IE_LEN;
}

static void
wlc_rrm_rep_err(wlc_rrm_info_t *rrm_info, uint8 type, uint8 token, uint8 reason)
{
	wlc_info_t *wlc = rrm_info->wlc;
	void *p;
	uint8 *pbody;
	dot11_rm_action_t *rm_rep;
	dot11_rm_ie_t *rmrep_ie;
	struct scb *scb;

	if ((p = wlc_frame_get_action(wlc, &rrm_info->da,
		&rrm_info->cur_cfg->cur_etheraddr, &rrm_info->cur_cfg->BSSID,
		(DOT11_RM_ACTION_LEN + DOT11_RM_IE_LEN), &pbody, DOT11_ACTION_CAT_RRM)) != NULL) {

		rm_rep = (dot11_rm_action_t *)pbody;
		rm_rep->category = DOT11_ACTION_CAT_RRM;
		rm_rep->action = DOT11_RM_ACTION_RM_REP;
		rm_rep->token = rrm_info->dialog_token;
		rmrep_ie = (dot11_rm_ie_t *)&rm_rep->data[0];
		rmrep_ie->id = DOT11_MNG_MEASURE_REPORT_ID;
		rmrep_ie->len = DOT11_RM_IE_LEN - TLV_HDR_LEN;
		rmrep_ie->token = token;
		rmrep_ie->mode = reason;
		rmrep_ie->type = type;

		scb = wlc_scbfind(wlc, rrm_info->cur_cfg, &rrm_info->da);
		wlc_sendmgmt(wlc, p, rrm_info->cur_cfg->wlcif->qi, scb);
	}
}

static void
wlc_rrm_bcnreq_scancb(void *arg, int status, wlc_bsscfg_t *cfg)
{
	wlc_rrm_info_t *rrm_info = arg;
	wlc_info_t *wlc = rrm_info->wlc;
	rrm_bcnreq_t *bcn_req;
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;

	if (rrm_info->bcnreq == NULL || rrm_state == NULL) {
		WL_ERROR(("%s: bcn_req or rrm_state is NULL\n", __FUNCTION__));
		return;
	}

	WL_ERROR(("%s: state: %d, status: %d\n", __FUNCTION__, rrm_state->step, status));
	bcn_req = rrm_info->bcnreq;

	bcn_req->scan_status = status;
	rrm_state->scan_active = FALSE;

	/* copy scan results to rrm_state for reporting */
	wlc_bss_list_xfer(wlc->scan_results, &bcn_req->scan_results);

	wl_add_timer(wlc->wl, rrm_info->rrm_timer, 0, 0);
	WL_ERROR(("%s: scan count: %d\n", __FUNCTION__,
		bcn_req->scan_results.count));

	rrm_info->bcn_req_win_scan_cnt++;
	if (rrm_info->bcn_req_off_chan_time_allowed) {
		rrm_info->bcn_req_off_chan_time += OSL_SYSUPTIME() -
			rrm_info->bcn_req_scan_start_timestamp;
	}
}

static void wlc_rrm_send_txstrmrep(wlc_rrm_info_t *rrm_info)
{}
static void wlc_rrm_send_statrep(wlc_rrm_info_t *rrm_info)
{}

static void
wlc_rrm_send_framerep(wlc_rrm_info_t *rrm_info)
{
	wlc_info_t *wlc = rrm_info->wlc;
	dot11_rm_action_t *rm_rep;
	dot11_rm_ie_t *rmrep_ie;
	dot11_rmrep_frame_t *rmrep_frame;
	void *p = NULL;
	uint8 *pbody, *buf;
	uint len, buflen, frame_cnt = 2;
	struct scb *scb;

	p = wlc_frame_get_action(wlc, &rrm_info->da,
		&rrm_info->cur_cfg->cur_etheraddr, &rrm_info->cur_cfg->BSSID,
		ETHER_MAX_DATA, &pbody, DOT11_ACTION_CAT_RRM);

	if (p == NULL) {
		WL_ERROR(("%s: failed to get mgmt frame\n", __FUNCTION__));
		return;
	}

	rm_rep = (dot11_rm_action_t *)pbody;
	rm_rep->category = DOT11_ACTION_CAT_RRM;
	rm_rep->action = DOT11_RM_ACTION_RM_REP;
	rm_rep->token = rrm_info->dialog_token;

	rmrep_ie = (dot11_rm_ie_t *)&rm_rep->data[0];
	rmrep_ie->id = DOT11_MNG_MEASURE_REPORT_ID;
	rmrep_ie->len = DOT11_RM_IE_LEN - TLV_HDR_LEN;
	rmrep_ie->token = 0;
	rmrep_ie->mode = 0;
	rmrep_ie->type = DOT11_MEASURE_TYPE_FRAME;
	rmrep_frame = (dot11_rmrep_frame_t *)&rmrep_ie[1];
	rmrep_frame->reg = 0;
	rmrep_frame->channel = 0;
	rmrep_frame->duration = 0;
	buf = (uint8 *)&rmrep_frame[1];
	rmrep_ie->len += DOT11_RMREP_FRAME_LEN;
	buflen = ETHER_MAX_DATA -
		(DOT11_MGMT_HDR_LEN + DOT11_RM_ACTION_LEN + DOT11_RM_IE_LEN
			+ DOT11_RMREP_FRAME_LEN);

	len = wlc_rrm_framerep_add(rrm_info, buf, frame_cnt);
	buflen -= len;
	rmrep_ie->len += len;

	if (p != NULL) {
		/* Fix up packet length */
		if (buflen > 0) {
			PKTSETLEN(wlc->osh, p, (ETHER_MAX_DATA - buflen));
		}
		scb = wlc_scbfind(wlc, rrm_info->cur_cfg, &rrm_info->da);
		wlc_sendmgmt(wlc, p, rrm_info->cur_cfg->wlcif->qi, scb);
	}
}

static void
wlc_rrm_send_noiserep(wlc_rrm_info_t *rrm_info)
{
	wlc_info_t *wlc = rrm_info->wlc;
	dot11_rm_action_t *rm_rep;
	dot11_rm_ie_t *rmrep_ie;
	dot11_rmrep_noise_t *rmrep_noise;
	void *p = NULL;
	uint8 *pbody;
	unsigned int len;
	struct scb *scb;

	len = DOT11_RM_ACTION_LEN + DOT11_RM_IE_LEN + DOT11_RMREP_NOISE_LEN;
	p = wlc_frame_get_action(wlc, &rrm_info->da,
		&rrm_info->cur_cfg->cur_etheraddr, &rrm_info->cur_cfg->BSSID,
		len, &pbody, DOT11_ACTION_CAT_RRM);

	if (p == NULL) {
		WL_ERROR(("%s: failed to get mgmt frame\n", __FUNCTION__));
		return;
	}
	rm_rep = (dot11_rm_action_t *)pbody;
	rm_rep->category = DOT11_ACTION_CAT_RRM;
	rm_rep->action = DOT11_RM_ACTION_RM_REP;
	rm_rep->token = rrm_info->dialog_token;

	rmrep_ie = (dot11_rm_ie_t *)&rm_rep->data[0];
	rmrep_ie->id = DOT11_MNG_MEASURE_REPORT_ID;
	rmrep_ie->len = DOT11_RM_IE_LEN + DOT11_RMREP_NOISE_LEN - TLV_HDR_LEN;
	rmrep_ie->token = 0;
	rmrep_ie->mode = 0;
	rmrep_ie->type = DOT11_MEASURE_TYPE_NOISE;

	rmrep_noise = (dot11_rmrep_noise_t *)&rmrep_ie[1];

	rmrep_noise->reg = 0;
	rmrep_noise->channel = 0;
	rmrep_noise->duration = 0;
	rmrep_noise->antid = 0;
	rmrep_noise->anpi = 0;
	if (p != NULL) {
		scb = wlc_scbfind(wlc, rrm_info->cur_cfg, &rrm_info->da);
		wlc_sendmgmt(wlc, p, rrm_info->cur_cfg->wlcif->qi, scb);
	}
}

static void
wlc_rrm_send_chloadrep(wlc_rrm_info_t *rrm_info)
{
	wlc_info_t *wlc = rrm_info->wlc;
	dot11_rm_action_t *rm_rep;
	dot11_rm_ie_t *rmrep_ie;
	dot11_rmrep_chanload_t *chload;
	void *p = NULL;
	uint8 *pbody;
	unsigned int len;
	struct scb *scb;

	len = DOT11_RM_ACTION_LEN + DOT11_RM_IE_LEN + DOT11_RMREP_CHANLOAD_LEN;
	p = wlc_frame_get_action(wlc, &rrm_info->da,
		&rrm_info->cur_cfg->cur_etheraddr, &rrm_info->cur_cfg->BSSID,
		len, &pbody, DOT11_ACTION_CAT_RRM);

	if (p == NULL) {
		WL_ERROR(("%s: failed to get mgmt frame\n", __FUNCTION__));
		return;
	}
	rm_rep = (dot11_rm_action_t *)pbody;
	rm_rep->category = DOT11_ACTION_CAT_RRM;
	rm_rep->action = DOT11_RM_ACTION_RM_REP;
	rm_rep->token = rrm_info->dialog_token;

	rmrep_ie = (dot11_rm_ie_t *)&rm_rep->data[0];
	rmrep_ie->id = DOT11_MNG_MEASURE_REPORT_ID;
	rmrep_ie->len = DOT11_RM_IE_LEN + DOT11_RMREP_CHANLOAD_LEN - TLV_HDR_LEN;

	rmrep_ie->token = 0;
	rmrep_ie->mode = 0;
	rmrep_ie->type = DOT11_MEASURE_TYPE_CHLOAD;

	chload = (dot11_rmrep_chanload_t *)&rmrep_ie[1];

	chload->reg = 0;
	chload->channel = 0;
	chload->duration = 0;
	chload->channel_load = 0;

	if (p != NULL) {
		scb = wlc_scbfind(wlc, rrm_info->cur_cfg, &rrm_info->da);
		wlc_sendmgmt(wlc, p, rrm_info->cur_cfg->wlcif->qi, scb);
	}
}



#ifdef WL_FTM_11K
static void
wlc_rrm_send_lcirep(wlc_rrm_info_t *rrm_info, uint8 token)
{
	wlc_info_t *wlc = rrm_info->wlc;
	dot11_rm_action_t *rm_rep;
	dot11_rm_ie_t *rmrep_ie;
	uint8 *rmrep_lci;
	void *p = NULL;
	uint8 *pbody;
	unsigned int len;
	uint8 lci_len = 0;
	uint8 *lci_data = NULL;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, rrm_info->cur_cfg);
	dot11_meas_rep_t *lci_rep;
	struct scb *scb;

	if (rrm_cfg->nbr_rep_self &&
			rrm_cfg->nbr_rep_self->opt_lci) {
		lci_rep = (dot11_meas_rep_t *)
			rrm_cfg->nbr_rep_self->opt_lci;
		lci_data = &lci_rep->rep.data[0];
		lci_len = lci_rep->len - DOT11_MNG_IE_MREP_FIXED_LEN;
	} else {
		/* unknown LCI */
		lci_len = DOT11_FTM_LCI_UNKNOWN_LEN;
	}

	len = DOT11_RM_ACTION_LEN + DOT11_RM_IE_LEN + lci_len;
	p = wlc_frame_get_mgmt(wlc, FC_ACTION, &rrm_info->da,
		&rrm_info->cur_cfg->cur_etheraddr, &rrm_info->cur_cfg->BSSID,
		len, &pbody);

	if (p == NULL) {
		WL_ERROR(("%s: failed to get mgmt frame\n", __FUNCTION__));
		return;
	}
	rm_rep = (dot11_rm_action_t *)pbody;
	rm_rep->category = DOT11_ACTION_CAT_RRM;
	rm_rep->action = DOT11_RM_ACTION_RM_REP;
	rm_rep->token = rrm_info->dialog_token;

	rmrep_ie = (dot11_rm_ie_t *)&rm_rep->data[0];
	rmrep_ie->id = DOT11_MNG_MEASURE_REPORT_ID;
	rmrep_ie->len = DOT11_RM_IE_LEN + lci_len - TLV_HDR_LEN;
	rmrep_ie->token = token;
	rmrep_ie->mode = 0;
	rmrep_ie->type = DOT11_MEASURE_TYPE_LCI;

	rmrep_lci = (uint8 *)&rmrep_ie[1];

	if (lci_data) {
		memcpy(rmrep_lci, lci_data, lci_len);
	} else {
		/* set output to unknown LCI */
		memset(rmrep_lci, 0, DOT11_FTM_LCI_UNKNOWN_LEN);
	}

	scb = wlc_scbfind(wlc, rrm_info->cur_cfg, &rrm_info->da);
	wlc_sendmgmt(wlc, p, rrm_info->cur_cfg->wlcif->qi, scb);
}

static void
wlc_rrm_send_civiclocrep(wlc_rrm_info_t *rrm_info, uint8 token)
{
	wlc_info_t *wlc = rrm_info->wlc;
	dot11_rm_action_t *rm_rep;
	dot11_rm_ie_t *rmrep_ie;
	uint8 *rmrep_civic;
	void *p = NULL;
	uint8 *pbody;
	unsigned int len;
	uint8 civic_len = 0;
	uint8 *civic_data = NULL;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, rrm_info->cur_cfg);
	dot11_meas_rep_t *civic_rep;
	struct scb *scb;

	if (rrm_cfg->nbr_rep_self &&
			rrm_cfg->nbr_rep_self->opt_civic) {
		civic_rep = (dot11_meas_rep_t *)
			rrm_cfg->nbr_rep_self->opt_civic;
		civic_data = &civic_rep->rep.data[0];
		civic_len = civic_rep->len - DOT11_MNG_IE_MREP_FIXED_LEN;
	} else {
		/* unknown civic */
		civic_len = DOT11_FTM_CIVIC_UNKNOWN_LEN;
	}

	len = DOT11_RM_ACTION_LEN + DOT11_RM_IE_LEN + civic_len;
	p = wlc_frame_get_mgmt(wlc, FC_ACTION, &rrm_info->da,
		&rrm_info->cur_cfg->cur_etheraddr, &rrm_info->cur_cfg->BSSID,
		len, &pbody);

	if (p == NULL) {
		WL_ERROR(("%s: failed to get mgmt frame\n", __FUNCTION__));
		return;
	}
	rm_rep = (dot11_rm_action_t *)pbody;
	rm_rep->category = DOT11_ACTION_CAT_RRM;
	rm_rep->action = DOT11_RM_ACTION_RM_REP;
	rm_rep->token = rrm_info->dialog_token;

	rmrep_ie = (dot11_rm_ie_t *)&rm_rep->data[0];
	rmrep_ie->id = DOT11_MNG_MEASURE_REPORT_ID;
	rmrep_ie->len = DOT11_RM_IE_LEN + civic_len - TLV_HDR_LEN;
	rmrep_ie->token = token;
	rmrep_ie->mode = 0;
	rmrep_ie->type = DOT11_MEASURE_TYPE_CIVICLOC;

	rmrep_civic = (uint8 *)&rmrep_ie[1];

	if (civic_data) {
		memcpy(rmrep_civic, civic_data, civic_len);
	} else {
		/* set output to unknown civic */
		memset(rmrep_civic, 0, DOT11_FTM_CIVIC_UNKNOWN_LEN);
	}

	scb = wlc_scbfind(wlc, rrm_info->cur_cfg, &rrm_info->da);
	wlc_sendmgmt(wlc, p, rrm_info->cur_cfg->wlcif->qi, scb);
}
#endif /* WL_FTM_11K */

static void
wlc_rrm_send_bcnrep(wlc_rrm_info_t *rrm_info, wlc_bss_list_t *bsslist)
{
	wlc_info_t *wlc = rrm_info->wlc;
	dot11_rm_action_t *rm_rep;
	wlc_bss_info_t *bi;
	void *p = NULL;
	uint8 *pbody, *buf = NULL;
	unsigned int i = 0, len, buflen = 0;
	unsigned int sent_packets = 0;
	unsigned int bss_added = 0;
	struct scb *scb;

	/* Use a do/while to ensure that a report packet is always allocated even if bsslist is
	 * empty.
	 */
	do {
		/* Allocate and initialise packet. */
		if (buflen <= 0) {
			p = wlc_frame_get_action(wlc, &rrm_info->da,
				&rrm_info->cur_cfg->cur_etheraddr, &rrm_info->cur_cfg->BSSID,
				ETHER_MAX_DATA, &pbody, DOT11_ACTION_CAT_RRM);

			if (p == NULL) {
				WL_ERROR(("%s: failed to get mgmt frame\n", __FUNCTION__));
				return;
			}

			rm_rep = (dot11_rm_action_t *)pbody;
			rm_rep->category = DOT11_ACTION_CAT_RRM;
			rm_rep->action = DOT11_RM_ACTION_RM_REP;
			rm_rep->token = rrm_info->dialog_token;

			buf = &rm_rep->data[0];
			buflen = ETHER_MAX_DATA - (DOT11_MGMT_HDR_LEN + DOT11_RM_ACTION_LEN);
		}

		/* Send, or add a BSS to the report. */
		if (i < bsslist->count) {
			bi = bsslist->ptrs[i];
			len = wlc_rrm_bcnrep_add(rrm_info, bi, buf, buflen);
			if (len == 0) {
				/* wlc_rrm_bcnrep_add returns 0 if another beacon report won't fit
				 * in the buffer. Immediately send what we have so next loop
				 * iteration can allocate frame and add this beacon report.
				 */
				/* Fixup packet length */
				if (buflen > 0)
					PKTSETLEN(wlc->osh, p, (ETHER_MAX_DATA-buflen));

				scb = wlc_scbfind(wlc, rrm_info->cur_cfg, &rrm_info->da);
				wlc_sendmgmt(wlc, p, rrm_info->cur_cfg->wlcif->qi, scb);
				/* Force allocation of new buffer */
				buflen = 0;
				bss_added = 0;
				sent_packets += 1;
			} else {
				i++;
				buflen -= len;
				buf += len;
				bss_added += 1;
			}
		}
	} while (i < bsslist->count);

	/* Send any remaining reports. */
	if (buflen > 0) {
		bool do_send = TRUE;

		if (sent_packets == 0 && bss_added == 0) {
			/* Make sure we send at least one beacon report even if it is empty. */
			len = wlc_rrm_add_empty_bcnrep(rrm_info, buf, buflen);
			buflen -= len;
		} else if (sent_packets > 0 && bss_added == 0) {
			/* No need to send anything else since we've already sent something. */
			do_send = FALSE;
		}

		if (do_send) {
			/* Fixup packet length */
			if (buflen > 0)
				PKTSETLEN(wlc->osh, p, (ETHER_MAX_DATA-buflen));

			scb = wlc_scbfind(wlc, rrm_info->cur_cfg, &rrm_info->da);
			wlc_sendmgmt(wlc, p, rrm_info->cur_cfg->wlcif->qi, scb);
		} else {
			/* Not sending anything, need to manually free packet. */
			PKTFREE(wlc->osh, p, TRUE);
			WLCNTINCR(wlc->pub->_cnt->txnobuf);
		}
	}

	wlc_bss_list_free(rrm_info->wlc, &rrm_info->bcnreq->scan_results);
}

static int
wlc_rrm_framerep_add(wlc_rrm_info_t *rrm_info, uint8 *bufptr, uint cnt)
{
	uint i;
	bcm_tlv_t *frm_body_tlv;
	dot11_rmrep_frmentry_t *frm_e;
	frm_body_tlv = (bcm_tlv_t *)bufptr;
	frm_body_tlv->id = DOT11_RMREP_FRAME_COUNT_REPORT;
	frm_body_tlv->len = cnt * DOT11_RMREP_FRMENTRY_LEN;

	frm_e = (dot11_rmrep_frmentry_t *)bufptr;
	for (i = 0; i < cnt; i++) {
		frm_e->phy_type = 0;
		frm_e->avg_rcpi = 0;
		frm_e->last_rsni = 0;
		frm_e->last_rcpi = 0;
		frm_e->ant_id = 0;
		frm_e->frame_cnt = 0;
		frm_e++;
	}
	return frm_body_tlv->len + TLV_HDR_LEN;
}

static int
wlc_rrm_bcnrep_add(wlc_rrm_info_t *rrm_info, wlc_bss_info_t *bi, uint8 *bufptr, uint buflen)
{
	dot11_rm_ie_t *rmrep_ie;
	dot11_rmrep_bcn_t *rmrep_bcn;
	bcm_tlv_t *frm_body_tlv;
	unsigned int elem_len, i;
	char eabuf[ETHER_ADDR_STR_LEN];

	elem_len = DOT11_RM_IE_LEN + DOT11_RMREP_BCN_LEN;
	if (buflen < elem_len) {
		return 0;
	}

	rmrep_ie = (dot11_rm_ie_t *)bufptr;
	rmrep_ie->id = DOT11_MNG_MEASURE_REPORT_ID;
	rmrep_ie->token = rrm_info->bcnreq->token;
	rmrep_ie->mode = 0;
	rmrep_ie->type = DOT11_MEASURE_TYPE_BEACON;

	rmrep_bcn = (dot11_rmrep_bcn_t *)&rmrep_ie[1];
	rmrep_bcn->reg = wlc_rrm_get_ap_chrep_reg(bi);
	if (rmrep_bcn->reg == DOT11_OP_CLASS_NONE) {
		rmrep_bcn->reg = rrm_info->bcnreq->reg;
	}
	rmrep_bcn->channel = wf_chspec_ctlchan(bi->chanspec);
	bcopy(&rrm_info->bcnreq->start_tsf_l, rmrep_bcn->starttime, (2*sizeof(uint32)));
	rmrep_bcn->duration = htol16(rrm_info->bcnreq->duration);
	rmrep_bcn->frame_info = 0;
	rmrep_bcn->rcpi = (uint8)bi->RSSI;
	rmrep_bcn->rsni = (uint8)bi->SNR;
	bcopy(&bi->BSSID, &rmrep_bcn->bssid, ETHER_ADDR_LEN);
	rmrep_bcn->antenna_id = 0;

	bcm_ether_ntoa(&rmrep_bcn->bssid, eabuf);
	WL_INFORM(("%s: token: %d, channel :%d, duration: %d, frame info: %d, rcpi: %d,"
		"rsni: %d, bssid: %s, antenna id: %d, parent tsf: %d\n",
		__FUNCTION__, rmrep_ie->token, rmrep_bcn->channel, rmrep_bcn->duration,
		rmrep_bcn->frame_info, rmrep_bcn->rcpi, rmrep_bcn->rsni,
		eabuf,
		rmrep_bcn->antenna_id, rmrep_bcn->parent_tsf));

	/* add bcn frame body subelement */
	if (rrm_info->bcnreq->rep_detail == DOT11_RMREQ_BCN_REPDET_ALL) {
		elem_len += TLV_HDR_LEN + bi->bcn_prb_len;
		if (buflen < elem_len)
			return 0;

		frm_body_tlv = (bcm_tlv_t *)&rmrep_bcn[1];
		frm_body_tlv->id = DOT11_RMREP_BCN_FRM_BODY;
		frm_body_tlv->len = (uint8)bi->bcn_prb_len;
		bcopy(bi->bcn_prb, &frm_body_tlv->data[0], bi->bcn_prb_len);
	}
	else if (rrm_info->bcnreq->rep_detail == DOT11_RMREQ_BCN_REPDET_REQUEST) {
		bcm_tlv_t *tlv;
		int tlvs_len;

		elem_len += TLV_HDR_LEN + DOT11_BCN_PRB_FIXED_LEN;
		if (buflen < elem_len)
			return 0;
		frm_body_tlv = (bcm_tlv_t *)&rmrep_bcn[1];
		frm_body_tlv->id = DOT11_RMREP_BCN_FRM_BODY;
		frm_body_tlv->len = DOT11_BCN_PRB_FIXED_LEN;
		bcopy(bi->bcn_prb, &frm_body_tlv->data[0], DOT11_BCN_PRB_FIXED_LEN);

		bufptr += elem_len;
		tlvs_len = bi->bcn_prb_len - DOT11_BCN_PRB_FIXED_LEN;
		if (tlvs_len) {
			tlv = (bcm_tlv_t *)((uint8 *)bi->bcn_prb + DOT11_BCN_PRB_FIXED_LEN);
			while (tlv) {
				/* Need to parse for multiple instances of the requested ie,
				 * for example multiple vendor ies
				 */
				for (i = 0; i < rrm_info->bcnreq->req_eid_num; i++) {
					if (tlv->id == rrm_info->bcnreq->req_eid[i]) {
						int tlv_len;

						tlv_len = TLV_HDR_LEN + tlv->len;
						elem_len += tlv_len;
						if (buflen < elem_len)
							return 0;
						bcopy(tlv, bufptr, tlv_len);
						bufptr += tlv_len;
						frm_body_tlv->len += (uint8)tlv_len;
						break;
					}
				}
				tlv = bcm_next_tlv(tlv, (int *)&tlvs_len);
			}
		}
	}

	rmrep_ie->len = elem_len - TLV_HDR_LEN;
	return (elem_len);
}

static void
wlc_rrm_bcnreq_scancache(wlc_rrm_info_t *rrm_info, wlc_ssid_t *ssid, struct ether_addr *bssid)
{
	wlc_info_t *wlc = rrm_info->wlc;
	wlc_bss_list_t bss_list;
#ifdef BCMDBG
	char eabuf[ETHER_ADDR_STR_LEN];
#endif /* BCMDBG */

	if (!SCANCACHE_ENAB(wlc->scan) || SCAN_IN_PROGRESS(wlc->scan)) {
		WL_ERROR(("%s: scancache: %d, scan_in_progress: %d\n",
			__FUNCTION__, SCANCACHE_ENAB(wlc->scan),
			SCAN_IN_PROGRESS(wlc->scan)));
		/* send a null beacon report */
		wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_BEACON, rrm_info->bcnreq->token, 0);
		return;
	}

	if (ETHER_ISNULLADDR(bssid)) {
		bssid = NULL;
	}
	wlc_scan_get_cache(wlc->scan, bssid, 1, ssid, DOT11_BSSTYPE_ANY, NULL, 0, &bss_list);

#ifdef BCMDBG
	if (bssid) {
		bcm_ether_ntoa(bssid, eabuf);
		WL_ERROR(("%s: ssid: %s, bssid: %s, bss_list.count: %d\n",
			__FUNCTION__, ssid->SSID, eabuf, bss_list.count));
	} else {
		WL_ERROR(("%s: ssid: %s, bssid: NULL, bss_list.count: %d\n",
			__FUNCTION__, ssid->SSID, bss_list.count));
	}
#endif /* BCMDBG */

	if (bss_list.count) {
		wlc_rrm_send_bcnrep(rrm_info, &bss_list);
		wlc_bss_list_free(wlc, &bss_list);
	}
	else
		wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_BEACON, rrm_info->bcnreq->token, 0);
}

static void
wlc_rrm_send_lmreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, struct ether_addr *da)
{
	void *p;
	uint8 *pbody;
	int buflen;
	dot11_lmreq_t *lmreq;
	struct scb *scb;

	wlc_info_t *wlc = rrm_info->wlc;

	/* lm request header */
	buflen = DOT11_LMREQ_LEN;


	if ((p = wlc_frame_get_action(wlc, da,
		&cfg->cur_etheraddr, &cfg->BSSID, buflen, &pbody, DOT11_ACTION_CAT_RRM)) == NULL) {
		return;
	}
	lmreq = (dot11_lmreq_t *)pbody;
	lmreq->category = DOT11_ACTION_CAT_RRM;
	lmreq->action = DOT11_RM_ACTION_LM_REQ;
	WLC_RRM_UPDATE_TOKEN(rrm_info->req_token);
	lmreq->token = rrm_info->req_token;
	lmreq->txpwr = 0;
	lmreq->maxtxpwr = 0;

	scb = wlc_scbfind(wlc, cfg, da);
	wlc_sendmgmt(wlc, p, cfg->wlcif->qi, scb);
}

static void
wlc_rrm_send_statreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, statreq_t *statreq)
{
	void *p;
	uint8 *pbody;
	int buflen;
	dot11_rmreq_t *rmreq;
	dot11_rmreq_stat_t *sreq;
	struct ether_addr *da;
	struct scb *scb;

	wlc_info_t *wlc = rrm_info->wlc;
	da = &statreq->da;

	/* rm frame action header + STAT request header */
	buflen = DOT11_RMREQ_LEN + DOT11_RMREQ_STAT_LEN;

	if ((p = wlc_frame_get_action(wlc, da,
		&cfg->cur_etheraddr, &cfg->BSSID, buflen, &pbody, DOT11_ACTION_CAT_RRM)) == NULL) {
		return;
	}

	rmreq = (dot11_rmreq_t *)pbody;
	rmreq->category = DOT11_ACTION_CAT_RRM;
	rmreq->action = DOT11_RM_ACTION_RM_REQ;
	WLC_RRM_UPDATE_TOKEN(rrm_info->req_token);
	rmreq->token = rrm_info->req_token;
	rmreq->reps = statreq->reps;

	sreq = (dot11_rmreq_stat_t *)&rmreq->data[0];
	sreq->id = DOT11_MNG_MEASURE_REQUEST_ID;
	sreq->len = DOT11_RMREQ_STAT_LEN - TLV_HDR_LEN;
	WLC_RRM_UPDATE_TOKEN(rrm_info->req_elt_token);
	sreq->token = rrm_info->req_elt_token;
	sreq->mode = 0;
	sreq->type = DOT11_MEASURE_TYPE_STAT;
	sreq->interval = statreq->random_int;
	sreq->duration = statreq->dur;
	sreq->group_id = statreq->group_id;
	bcopy(&statreq->peer, &sreq->peer, sizeof(struct ether_addr));

	scb = wlc_scbfind(wlc, cfg, da);
	wlc_sendmgmt(wlc, p, cfg->wlcif->qi, scb);
}

static void
wlc_rrm_send_framereq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, framereq_t *framereq)
{
	void *p;
	uint8 *pbody;
	int buflen;
	dot11_rmreq_t *rmreq;
	dot11_rmreq_frame_t *freq;
	struct ether_addr *da;
	struct scb *scb;

	wlc_info_t *wlc = rrm_info->wlc;
	da = &framereq->da;

	/* rm frame action header + frame request header */
	buflen = DOT11_RMREQ_LEN + DOT11_RMREQ_FRAME_LEN;

	if ((p = wlc_frame_get_action(wlc, da,
		&cfg->cur_etheraddr, &cfg->BSSID, buflen, &pbody, DOT11_ACTION_CAT_RRM)) == NULL) {
		return;
	}

	rmreq = (dot11_rmreq_t *)pbody;
	rmreq->category = DOT11_ACTION_CAT_RRM;
	rmreq->action = DOT11_RM_ACTION_RM_REQ;
	WLC_RRM_UPDATE_TOKEN(rrm_info->req_token);
	rmreq->token = rrm_info->req_token;
	rmreq->reps = framereq->reps;

	freq = (dot11_rmreq_frame_t *)&rmreq->data[0];
	freq->id = DOT11_MNG_MEASURE_REQUEST_ID;
	freq->len = DOT11_RMREQ_FRAME_LEN - TLV_HDR_LEN;
	freq->token = 0;
	freq->mode = 0;
	freq->type = DOT11_MEASURE_TYPE_FRAME;
	freq->reg = framereq->reg;
	freq->channel = framereq->chan;
	freq->interval = framereq->random_int;
	freq->duration = framereq->dur;
	/* Frame Count Report is requested */
	freq->req_type = 1;
	bcopy(&framereq->ta, &freq->ta, sizeof(struct ether_addr));

	scb = wlc_scbfind(wlc, cfg, da);
	wlc_sendmgmt(wlc, p, cfg->wlcif->qi, scb);
}

static void
wlc_rrm_send_noisereq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, rrmreq_t *noisereq)
{
	void *p;
	uint8 *pbody;
	int buflen;
	dot11_rmreq_t *rmreq;
	dot11_rmreq_noise_t *nreq;
	struct ether_addr *da;
	struct scb *scb;

	wlc_info_t *wlc = rrm_info->wlc;
	da = &noisereq->da;

	/* rm frame action header + noise request header */
	buflen = DOT11_RMREQ_LEN + DOT11_RMREQ_NOISE_LEN;

	if ((p = wlc_frame_get_action(wlc, da,
		&cfg->cur_etheraddr, &cfg->BSSID, buflen, &pbody, DOT11_ACTION_CAT_RRM)) == NULL) {
		return;
	}

	rmreq = (dot11_rmreq_t *)pbody;
	rmreq->category = DOT11_ACTION_CAT_RRM;
	rmreq->action = DOT11_RM_ACTION_RM_REQ;
	WLC_RRM_UPDATE_TOKEN(rrm_info->req_token);
	rmreq->token = rrm_info->req_token;
	rmreq->reps = noisereq->reps;

	nreq = (dot11_rmreq_noise_t *)&rmreq->data[0];
	nreq->id = DOT11_MNG_MEASURE_REQUEST_ID;
	nreq->len = DOT11_RMREQ_NOISE_LEN - TLV_HDR_LEN;
	WLC_RRM_UPDATE_TOKEN(rrm_info->req_elt_token);
	nreq->token = rrm_info->req_elt_token;
	nreq->mode = 0;
	nreq->type = DOT11_MEASURE_TYPE_NOISE;
	nreq->reg = noisereq->reg;
	nreq->channel = noisereq->chan;
	nreq->interval = noisereq->random_int;
	nreq->duration = noisereq->dur;

	scb = wlc_scbfind(wlc, cfg, da);
	wlc_sendmgmt(wlc, p, cfg->wlcif->qi, scb);
}

static void
wlc_rrm_send_chloadreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, rrmreq_t *chloadreq)
{
	void *p;
	uint8 *pbody;
	int buflen;
	dot11_rmreq_t *rmreq;
	dot11_rmreq_chanload_t *chreq;
	struct ether_addr *da;
	struct scb *scb;

	wlc_info_t *wlc = rrm_info->wlc;
	da = &chloadreq->da;

	/* rm frame action header + channel load request header */
	buflen = DOT11_RMREQ_LEN + DOT11_RMREQ_CHANLOAD_LEN;

	if ((p = wlc_frame_get_action(wlc, da,
		&cfg->cur_etheraddr, &cfg->BSSID, buflen, &pbody, DOT11_ACTION_CAT_RRM)) == NULL) {
		return;
	}

	rmreq = (dot11_rmreq_t *)pbody;
	rmreq->category = DOT11_ACTION_CAT_RRM;
	rmreq->action = DOT11_RM_ACTION_RM_REQ;
	WLC_RRM_UPDATE_TOKEN(rrm_info->req_token);
	rmreq->token = rrm_info->req_token;
	rmreq->reps = chloadreq->reps;

	chreq = (dot11_rmreq_chanload_t *)&rmreq->data[0];
	chreq->id = DOT11_MNG_MEASURE_REQUEST_ID;
	chreq->len = DOT11_RMREQ_CHANLOAD_LEN - TLV_HDR_LEN;
	WLC_RRM_UPDATE_TOKEN(rrm_info->req_elt_token);
	chreq->token = rrm_info->req_elt_token;
	chreq->mode = 0;
	chreq->type = DOT11_MEASURE_TYPE_CHLOAD;
	if (chloadreq->reg == 0) {
		chreq->reg = wlc_get_regclass(wlc->cmi, cfg->current_bss->chanspec);
	}
	else {
		chreq->reg = chloadreq->reg;
	}
	chreq->channel = chloadreq->chan;
	chreq->interval = chloadreq->random_int;
	chreq->duration = chloadreq->dur;

	scb = wlc_scbfind(wlc, cfg, da);
	wlc_sendmgmt(wlc, p, cfg->wlcif->qi, scb);
}

static void
wlc_rrm_send_bcnreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, bcn_req_t *bcnreq)
{
	void *p;
	uint8 *pbody, repdet;
	int buflen;
	int dur;
	int channel;
	uint16 interval;
	uint16 repcond;
	dot11_rmreq_t *rmreq;
	dot11_rmreq_bcn_t *rmreq_bcn;
	struct ether_addr *da;
	uint8 bcnmode;
	int len = 0;
	int i = 0;
	struct scb *scb;

	wlc_info_t *wlc = rrm_info->wlc;
	da = &bcnreq->da;
	dur = bcnreq->dur;
	channel = bcnreq->channel;
	interval = bcnreq->random_int;
	bcnmode = bcnreq->bcn_mode;

	/* rm frame action header + bcn request header */
	buflen = DOT11_RMREQ_LEN + DOT11_RMREQ_BCN_LEN;
	if (bcnreq->ssid.SSID_len == 1 &&
		memcmp(bcnreq->ssid.SSID, WLC_RRM_WILDCARD_SSID_IND, 1) == 0) {
		buflen += TLV_HDR_LEN;
	}
	else if (bcnreq->ssid.SSID_len)
		buflen += bcnreq->ssid.SSID_len + TLV_HDR_LEN;

	/* AP Channel Report */
	len = bcnreq->chspec_list.num;
	if (len) {
		buflen += len + TLV_HDR_LEN + 1;
	}

	/* buflen is length of Reporting Detail
	   and optional beacon request id if present + 3 * TLV_HDR_LEN
	*/
	if (bcnreq->req_elements) {
		if (bcnreq->reps)
			buflen += (WLC_RRM_BCNREQ_NUM_REQ_ELEM + TLV_HDR_LEN) +
				(1 + TLV_HDR_LEN) + (DOT11_RMREQ_BCN_REPINFO_LEN +
				TLV_HDR_LEN);
		else
			buflen += (WLC_RRM_BCNREQ_NUM_REQ_ELEM + TLV_HDR_LEN) +
				(1 + TLV_HDR_LEN);
	}
	else {
		if (bcnreq->reps)
			buflen += (1 + TLV_HDR_LEN) + (DOT11_RMREQ_BCN_REPINFO_LEN +
				TLV_HDR_LEN);
		else
			buflen += (1 + TLV_HDR_LEN);
	}
	if ((p = wlc_frame_get_action(wlc, da,
		&cfg->cur_etheraddr, &cfg->BSSID, buflen, &pbody, DOT11_ACTION_CAT_RRM)) == NULL) {
		return;
	}

	rmreq = (dot11_rmreq_t *)pbody;
	rmreq->category = DOT11_ACTION_CAT_RRM;
	rmreq->action = DOT11_RM_ACTION_RM_REQ;
	WLC_RRM_UPDATE_TOKEN(rrm_info->req_token);
	rmreq->token = rrm_info->req_token;
	rmreq->reps = bcnreq->reps;
	pbody += DOT11_RMREQ_LEN;

	rmreq_bcn = (dot11_rmreq_bcn_t *)&rmreq->data[0];

	rmreq_bcn->id = DOT11_MNG_MEASURE_REQUEST_ID;

	rmreq_bcn->len = DOT11_RMREQ_BCN_LEN - TLV_HDR_LEN;

	WLC_RRM_UPDATE_TOKEN(rrm_info->req_elt_token);
	rmreq_bcn->token = rrm_info->req_elt_token;

	rmreq_bcn->mode = 0;

	rmreq_bcn->type = DOT11_MEASURE_TYPE_BEACON;
	rmreq_bcn->reg = wlc_get_regclass(wlc->cmi, cfg->current_bss->chanspec);
	rmreq_bcn->channel = channel;
	rmreq_bcn->interval = interval;
	rmreq_bcn->duration = dur;
	rmreq_bcn->bcn_mode = bcnmode;

	pbody += DOT11_RMREQ_BCN_LEN;
	/* sub-element SSID */
	if (bcnreq->ssid.SSID_len) {
		if (bcnreq->ssid.SSID_len == 1 &&
		memcmp(bcnreq->ssid.SSID, WLC_RRM_WILDCARD_SSID_IND, 1) == 0) {
			bcnreq->ssid.SSID_len = 0;
		}

		pbody = bcm_write_tlv(DOT11_MNG_SSID_ID, (uint8 *)bcnreq->ssid.SSID,
			bcnreq->ssid.SSID_len, pbody);
		rmreq_bcn->len += bcnreq->ssid.SSID_len + TLV_HDR_LEN;
	}

	if (rmreq->reps) {
		/* Beacon Reporting Information subelement may be included only for
		   repeated measurements, includes reporting condition
		*/
		repcond = DOT11_RMREQ_BCN_REPCOND_DEFAULT << 1;
		pbody = bcm_write_tlv(DOT11_RMREQ_BCN_REPINFO_ID, &repcond,
			DOT11_RMREQ_BCN_REPINFO_LEN, pbody);
		rmreq_bcn->len += DOT11_RMREQ_BCN_REPINFO_LEN + TLV_HDR_LEN;
	}

	/* request specific IEs since default will return only up to 224 bytes of beacon */
	if (bcnreq->req_elements)
		repdet = DOT11_RMREQ_BCN_REPDET_REQUEST;
	else
		repdet = DOT11_RMREQ_BCN_REPDET_ALL;

	pbody = bcm_write_tlv(DOT11_RMREQ_BCN_REPDET_ID, &repdet, 1, pbody);
	rmreq_bcn->len += 1 + TLV_HDR_LEN;

	/* add specific IEs to request */
	if (bcnreq->req_elements) {
		*pbody++ = DOT11_RMREQ_BCN_REQUEST_ID;
		*pbody++ = WLC_RRM_BCNREQ_NUM_REQ_ELEM;
		*pbody++ = DOT11_MNG_SSID_ID;
		*pbody++ = DOT11_MNG_HT_CAP;
		*pbody++ = DOT11_MNG_RSN_ID;
		*pbody++ = DOT11_MNG_AP_CHREP_ID;
		*pbody++ = DOT11_MNG_MDIE_ID;
		*pbody++ = DOT11_MNG_BSS_AVAL_ADMISSION_CAP_ID;
		*pbody++ = DOT11_MNG_RRM_CAP_ID;
		*pbody++ = DOT11_MNG_VS_ID;
		rmreq_bcn->len += WLC_RRM_BCNREQ_NUM_REQ_ELEM + TLV_HDR_LEN;
	}

	if (len) {
		/* AP Channel Report */
		*pbody++ = DOT11_RMREQ_BCN_APCHREP_ID;
		/* Length */
		*pbody++ = len + 1;

		/* only report channels for a single operating class, per spec */
		*pbody++ = wlc_get_regclass(wlc->cmi, bcnreq->chspec_list.list[0]);

		for (i = 0; i < len; i++)		{
			*pbody++ = CHSPEC_CHANNEL(bcnreq->chspec_list.list[i]);
		}
		rmreq_bcn->len += len + TLV_HDR_LEN + 1;
	}
	WL_ERROR(("%s: id: %d, len: %d, type: %d, channel: %d, duration: %d, bcn_mode: %d\n",
		__FUNCTION__, rmreq_bcn->id, rmreq_bcn->len, rmreq_bcn->type, rmreq_bcn->channel,
		rmreq_bcn->duration, rmreq_bcn->bcn_mode));

	memset(&rmreq_bcn->bssid.octet, 0xff, ETHER_ADDR_LEN);
	scb = wlc_scbfind(wlc, cfg, da);
	wlc_sendmgmt(wlc, p, cfg->wlcif->qi, scb);
}

#ifdef STA
static void
wlc_rrm_send_nbrreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, wlc_ssid_t *ssid)
{
	void *p;
	uint8 *pbody;
	int buflen;
	dot11_rm_action_t *rmreq;
	wlc_info_t *wlc = rrm_info->wlc;
	bcm_tlv_t *ssid_ie;
	rrm_bsscfg_cubby_t *rrm_cfg;
	uint8 *req_data_ptr;
	dot11_meas_req_loc_t *ie;
	struct scb *scb;

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
	ASSERT(rrm_cfg != NULL);

	/* rm frame action header + optional ssid ie */
	buflen = DOT11_RM_ACTION_LEN;

	if (ssid)
		buflen += ssid->SSID_len + TLV_HDR_LEN;

	if (isset(cfg->ext_cap, DOT11_EXT_CAP_FTM_RESPONDER) ||
		isset(cfg->ext_cap, DOT11_EXT_CAP_FTM_INITIATOR)) {
		if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LCIM)) {
			buflen += TLV_HDR_LEN + DOT11_MNG_IE_MREQ_LCI_FIXED_LEN;
		}
		if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_CIVIC_LOC)) {
			buflen += TLV_HDR_LEN + DOT11_MNG_IE_MREQ_CIVIC_FIXED_LEN;
		}
	} else {
		WL_ERROR(("wl%d: %s: FTM not enabled, not including location elements\n",
			wlc->pub->unit, __FUNCTION__));
	}

	if ((p = wlc_frame_get_action(wlc, &cfg->BSSID,
		&cfg->cur_etheraddr, &cfg->BSSID, buflen, &pbody, DOT11_ACTION_CAT_RRM)) == NULL) {
		return;
	}

	rmreq = (dot11_rm_action_t *)pbody;
	rmreq->category = DOT11_ACTION_CAT_RRM;
	rmreq->action = DOT11_RM_ACTION_NR_REQ;
	WLC_RRM_UPDATE_TOKEN(rrm_cfg->req_token);
	rmreq->token = rrm_cfg->req_token;
	req_data_ptr = &rmreq->data[0];

	if (ssid != NULL) {
		ssid_ie = (bcm_tlv_t *)&rmreq->data[0];
		ssid_ie->id = DOT11_MNG_SSID_ID;
		ssid_ie->len = (uint8)ssid->SSID_len;
		bcopy(ssid->SSID, ssid_ie->data, ssid->SSID_len);
		req_data_ptr += (TLV_HDR_LEN + ssid_ie->len);
	}

	if (isset(cfg->ext_cap, DOT11_EXT_CAP_FTM_RESPONDER) ||
		isset(cfg->ext_cap, DOT11_EXT_CAP_FTM_INITIATOR)) {

		WLC_RRM_UPDATE_TOKEN(rrm_info->req_elt_token);
		if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LCIM)) {
			ie = (dot11_meas_req_loc_t *) req_data_ptr;
			ie->id = DOT11_MNG_MEASURE_REQUEST_ID;
			ie->len = DOT11_MNG_IE_MREQ_LCI_FIXED_LEN;
			ie->token = rrm_info->req_elt_token;
			ie->mode = 0; /* enable bit is 0 */
			ie->type = DOT11_MEASURE_TYPE_LCI;
			ie->req.lci.subject = DOT11_FTM_LOCATION_SUBJ_REMOTE;
			req_data_ptr += TLV_HDR_LEN + DOT11_MNG_IE_MREQ_LCI_FIXED_LEN;
		}
		if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_CIVIC_LOC)) {
			ie = (dot11_meas_req_loc_t *) req_data_ptr;
			ie->id = DOT11_MNG_MEASURE_REQUEST_ID;
			ie->len = DOT11_MNG_IE_MREQ_CIVIC_FIXED_LEN;
			ie->token = rrm_info->req_elt_token;
			ie->mode = 0; /* enable bit is 0 */
			ie->type = DOT11_MEASURE_TYPE_CIVICLOC;
			ie->req.civic.subject = DOT11_FTM_LOCATION_SUBJ_REMOTE;
			ie->req.civic.type = 0;
			ie->req.civic.siu = 0;
			ie->req.civic.si = 0;
			req_data_ptr += TLV_HDR_LEN + DOT11_MNG_IE_MREQ_CIVIC_FIXED_LEN;
		}
	}

	scb = wlc_scbfind(wlc, cfg, &cfg->BSSID);
	wlc_sendmgmt(wlc, p, cfg->wlcif->qi, scb);
}
#endif /* STA */

#ifdef WL11K_AP
static void
wlc_rrm_recv_nrreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, struct scb *scb, uint8 *body,
	int body_len)
{
	wlc_info_t *wlc;
	dot11_rm_action_t *rmreq;
	rrm_bsscfg_cubby_t *rrm_cfg = NULL;
	wlc_ssid_t ssid;
	int tlv_len;
	bcm_tlv_t *tlv = NULL;
	uint8 *tlvs;

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
	ASSERT(rrm_cfg != NULL);

	wlc = rrm_info->wlc;
	ASSERT(wlc);

	BCM_REFERENCE(wlc);

	if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_NEIGHBOR_REPORT)) {
		WL_ERROR(("wl%d: %s: Unsupported\n", wlc->pub->unit,
		          __FUNCTION__));
		return;
	}
	if (body_len < DOT11_RM_ACTION_LEN) {
		WL_ERROR(("wl%d: %s: invalid body len %d\n", wlc->pub->unit,
		          __FUNCTION__, body_len));
		return;
	}

	rmreq = (dot11_rm_action_t *)body;
	bcopy(&scb->ea, &rrm_cfg->da, ETHER_ADDR_LEN);
	rrm_cfg->dialog_token = rmreq->token;

	memset(&ssid, 0, sizeof(ssid));

	/* parse subelements */
	if (body_len > DOT11_RM_ACTION_LEN) {
		tlvs = (uint8 *)&rmreq->data[0];
		tlv_len = body_len - DOT11_RM_ACTION_LEN;

		if (tlv_len > 1) {
			tlv = bcm_parse_tlvs(tlvs, tlv_len, DOT11_RMREQ_BCN_SSID_ID);
			if (tlv) {
				ssid.SSID_len = MIN(DOT11_MAX_SSID_LEN, tlv->len);
				bcopy(tlv->data, ssid.SSID, ssid.SSID_len);
			}
		}
	}
#ifdef BCMDBG
	wlc_rrm_dump_neighbors(rrm_info, cfg);
#endif /* BCMDBG */

	/* Send Neighbor Report Response */
	wlc_rrm_send_nbrrep(rrm_info, cfg, scb, &ssid);
}

static int
wlc_rrm_send_nbrrep(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, struct scb *scb,
	wlc_ssid_t *ssid)
{
	wlc_info_t *wlc;
	uint rep_count = 0;
	dot11_neighbor_rep_ie_t *nbrrep;
	rrm_nbr_rep_t *nbr_rep;
	dot11_rm_action_t *rmrep;
	int rrm_req_len, i;
	uint8 *pbody;
	void *p;
	uint8 *ptr;
	dot11_ngbr_bsstrans_pref_se_t *pref;
	uint16 list_cnt = 0;
	rrm_scb_info_t *scb_info;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);

	ASSERT(rrm_cfg != NULL);

	wlc = rrm_info->wlc;

	nbr_rep = rrm_cfg->nbr_rep_head;
	while (nbr_rep) {
		rep_count++;
		nbr_rep = nbr_rep->next;
	}

	scb_info = RRM_SCB_INFO(rrm_info, scb);
	nbr_rep = scb_info->nbr_rep_head;
	while (nbr_rep) {
		nbr_rep = nbr_rep->next;
		list_cnt++;
	}

	if (rep_count == 0 && list_cnt == 0) {
		WL_ERROR(("wl%d: %s: Neighbor Report element is empty\n",
			wlc->pub->unit, __FUNCTION__));
		return BCME_ERROR;
	}

	rrm_req_len = DOT11_RM_ACTION_LEN +
		(rep_count + list_cnt) * (TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN +
		TLV_HDR_LEN + DOT11_NGBR_BSSTRANS_PREF_SE_LEN);

	p = wlc_frame_get_action(wlc, &rrm_cfg->da,
	                       &cfg->cur_etheraddr, &cfg->BSSID,
	                       rrm_req_len, &pbody, DOT11_ACTION_CAT_RRM);
	if (p == NULL) {
		WL_ERROR((WLC_BSS_MALLOC_ERR, WLCWLUNIT(wlc), WLC_BSSCFG_IDX(cfg), __FUNCTION__,
			rrm_req_len, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}

	rmrep = (dot11_rm_action_t *)pbody;
	rmrep->category = DOT11_ACTION_CAT_RRM;
	rmrep->action = DOT11_RM_ACTION_NR_REP;
	rmrep->token = rrm_cfg->dialog_token;

	ptr = rmrep->data;

	/* add neighbors obtained from beacon report */
	nbr_rep = scb_info->nbr_rep_head;

	for (i = 0; i < list_cnt; i++) {
		nbrrep = (dot11_neighbor_rep_ie_t *)ptr;

		if (ssid != NULL && ssid->SSID_len > 0) {
			/* if ssid is specified and does not match entry, skip */
			if ((ssid->SSID_len != nbr_rep->nbr_elt.ssid.SSID_len) ||
				(memcmp(&nbr_rep->nbr_elt.ssid.SSID, &ssid->SSID,
				ssid->SSID_len) != 0)) {
				/* Move to the next one */
				nbr_rep = nbr_rep->next;
				continue;
			}
		}

		nbrrep->id = nbr_rep->nbr_elt.id;
		nbrrep->len = DOT11_NEIGHBOR_REP_IE_FIXED_LEN +
			DOT11_NGBR_BSSTRANS_PREF_SE_LEN + TLV_HDR_LEN;

		memcpy(&nbrrep->bssid, &nbr_rep->nbr_elt.bssid, ETHER_ADDR_LEN);

		store32_ua((uint8 *)&nbrrep->bssid_info, nbr_rep->nbr_elt.bssid_info);

		nbrrep->reg = nbr_rep->nbr_elt.reg;
		nbrrep->channel = nbr_rep->nbr_elt.channel;
		nbrrep->phytype = nbr_rep->nbr_elt.phytype;

		/* Add preference subelement */
		pref = (dot11_ngbr_bsstrans_pref_se_t*) nbrrep->data;
		pref->sub_id = DOT11_NGBR_BSSTRANS_PREF_SE_ID;
		pref->len = DOT11_NGBR_BSSTRANS_PREF_SE_LEN;
		pref->preference = nbr_rep->nbr_elt.bss_trans_preference;

		/* Move to the next one */
		nbr_rep = nbr_rep->next;

		ptr += TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN +
			TLV_HDR_LEN + DOT11_NGBR_BSSTRANS_PREF_SE_LEN;
	}

	nbr_rep = rrm_cfg->nbr_rep_head;

	for (i = 0; i < rep_count; i++) {
		nbrrep = (dot11_neighbor_rep_ie_t *)ptr;

		if (ssid != NULL && ssid->SSID_len > 0) {
			/* if ssid is specified and does not match entry, skip */
			if ((ssid->SSID_len != nbr_rep->nbr_elt.ssid.SSID_len) ||
				(memcmp(&nbr_rep->nbr_elt.ssid.SSID, &ssid->SSID,
				ssid->SSID_len) != 0)) {
				/* Move to the next one */
				nbr_rep = nbr_rep->next;
				continue;
			}
		}
		nbrrep->id = nbr_rep->nbr_elt.id;
		nbrrep->len = DOT11_NEIGHBOR_REP_IE_FIXED_LEN +
			DOT11_NGBR_BSSTRANS_PREF_SE_LEN + TLV_HDR_LEN;

		memcpy(&nbrrep->bssid, &nbr_rep->nbr_elt.bssid, ETHER_ADDR_LEN);

		store32_ua((uint8 *)&nbrrep->bssid_info, nbr_rep->nbr_elt.bssid_info);

		nbrrep->reg = nbr_rep->nbr_elt.reg;
		nbrrep->channel = nbr_rep->nbr_elt.channel;
		nbrrep->phytype = nbr_rep->nbr_elt.phytype;

		/* Add preference subelement */
		pref = (dot11_ngbr_bsstrans_pref_se_t*) nbrrep->data;
		pref->sub_id = DOT11_NGBR_BSSTRANS_PREF_SE_ID;
		pref->len = DOT11_NGBR_BSSTRANS_PREF_SE_LEN;
		pref->preference = nbr_rep->nbr_elt.bss_trans_preference;

		/* Move to the next one */
		nbr_rep = nbr_rep->next;
		ptr += TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN +
			TLV_HDR_LEN + DOT11_NGBR_BSSTRANS_PREF_SE_LEN;
	}

	wlc_sendmgmt(wlc, p, cfg->wlcif->qi, scb);
	return BCME_OK;
}

static void
wlc_rrm_get_neighbor_list(wlc_rrm_info_t *rrm_info, void *a, uint16 list_cnt,
	rrm_bsscfg_cubby_t *rrm_cfg)
{
	rrm_nbr_rep_t *nbr_rep;
	nbr_element_t *nbr_elt;
	uint8 *ptr;
	int i;

	nbr_rep = rrm_cfg->nbr_rep_head;
	ptr = (uint8 *)a;

	for (i = 0; i < list_cnt; i++) {
		nbr_elt = (nbr_element_t *)ptr;
		nbr_elt->id = nbr_rep->nbr_elt.id;
		nbr_elt->len = nbr_rep->nbr_elt.len;

		memcpy(&nbr_elt->bssid, &nbr_rep->nbr_elt.bssid, ETHER_ADDR_LEN);

		store32_ua((uint8 *)&nbr_elt->bssid_info, nbr_rep->nbr_elt.bssid_info);

		nbr_elt->reg = nbr_rep->nbr_elt.reg;
		nbr_elt->channel = nbr_rep->nbr_elt.channel;
		nbr_elt->phytype = nbr_rep->nbr_elt.phytype;

		nbr_rep = nbr_rep->next;
		ptr += TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN;
	}
}

static void
wlc_rrm_del_neighbor(wlc_rrm_info_t *rrm_info, struct ether_addr *ea, struct wlc_if *wlcif,
	rrm_bsscfg_cubby_t *rrm_cfg)
{
	rrm_nbr_rep_t *nbr_rep, *nbr_rep_pre = NULL;
	struct ether_addr *bssid;
	wlc_info_t *wlc;
	wlc_bsscfg_t *bsscfg;
	bool channel_removed = FALSE;

	wlc = rrm_info->wlc;
	nbr_rep = rrm_cfg->nbr_rep_head;
	while (nbr_rep) {
		bssid = &nbr_rep->nbr_elt.bssid;
		if (memcmp(ea, bssid, ETHER_ADDR_LEN) == 0)
			break;
		nbr_rep_pre = nbr_rep;
		nbr_rep = nbr_rep->next;

	}
	if (nbr_rep == NULL)
		return;

	wlc_rrm_remove_regclass_nbr_cnt(rrm_info, nbr_rep, 1, &channel_removed, rrm_cfg);

	/* remove node from neighbor report list */
	if (nbr_rep_pre == NULL)
		rrm_cfg->nbr_rep_head = nbr_rep->next;
	else
		nbr_rep_pre->next = nbr_rep->next;

	MFREE(rrm_info->wlc->osh, nbr_rep, sizeof(rrm_nbr_rep_t));

	/* Update beacons and probe responses */
	if (!channel_removed) {
		bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
		ASSERT(bsscfg);

		if (bsscfg->up &&
		    (BSSCFG_AP(bsscfg) || (!bsscfg->BSS && !BSS_TDLS_ENAB(wlc, bsscfg)))) {
			/* Update AP or IBSS beacons */
			wlc_bss_update_beacon(wlc, bsscfg);
			/* Update AP or IBSS probe responses */
			wlc_bss_update_probe_resp(wlc, bsscfg, TRUE);
		}
	}
}

static void
wlc_rrm_remove_regclass_nbr_cnt(wlc_rrm_info_t *rrm_info, rrm_nbr_rep_t *nbr_rep,
	bool delete_empty_reg, bool *channel_removed, rrm_bsscfg_cubby_t *rrm_cfg)
{
	bool reg_became_empty = FALSE;
	reg_nbr_count_t *nbr_cnt, *nbr_cnt_pre = NULL;
	int i;

	*channel_removed = FALSE;

	nbr_cnt = rrm_cfg->nbr_cnt_head;
	while (nbr_cnt) {
		if (nbr_rep->nbr_elt.reg == nbr_cnt->reg) {
			nbr_cnt->nbr_count_by_channel[nbr_rep->nbr_elt.channel] -= 1;
			if (nbr_cnt->nbr_count_by_channel[nbr_rep->nbr_elt.channel] == 0)
				*channel_removed = TRUE;

			reg_became_empty = TRUE;
			for (i = 0; i < MAXCHANNEL; i++) {
				if (nbr_cnt->nbr_count_by_channel[i] > 0) {
					reg_became_empty = FALSE;
					break;
				}
			}
			if (delete_empty_reg && reg_became_empty) {
				/* remove node */
				if (nbr_cnt_pre == NULL)
					rrm_cfg->nbr_cnt_head = nbr_cnt->next;
				else
					nbr_cnt_pre->next = nbr_cnt->next;
				MFREE(rrm_info->wlc->osh, nbr_cnt, sizeof(reg_nbr_count_t));
			}
			break;
		}
		nbr_cnt_pre = nbr_cnt;
		nbr_cnt = nbr_cnt->next;
	}
}

static int
wlc_rrm_add_neighbor(wlc_rrm_info_t *rrm_info, nbr_rpt_elem_t *nbr_elt, struct wlc_if *wlcif,
	rrm_bsscfg_cubby_t *rrm_cfg)
{
	rrm_nbr_rep_t *nbr_rep;
	wlc_info_t *wlc;
	wlc_bsscfg_t *bsscfg;
	uint16 nbr_rep_cnt;
	bool channel_removed = FALSE;
	bool channel_added = FALSE;
	bool channels_same = FALSE;
	bool should_update_beacon = FALSE;
	bool del = FALSE;

	wlc = rrm_info->wlc;
	/* Find Neighbor Report element from list */
	nbr_rep = rrm_cfg->nbr_rep_head;
	while (nbr_rep) {
		if (memcmp(&nbr_rep->nbr_elt.bssid, &nbr_elt->bssid, ETHER_ADDR_LEN) == 0)
			break;
		nbr_rep = nbr_rep->next;
	}

	if (nbr_rep) {
		if (nbr_rep->nbr_elt.reg == nbr_elt->reg) {
			if (nbr_rep->nbr_elt.channel == nbr_elt->channel)
				channels_same = TRUE;
		} else {
			del = TRUE;
		}
		wlc_rrm_remove_regclass_nbr_cnt(rrm_info, nbr_rep, del, &channel_removed, rrm_cfg);
		nbr_rep->nbr_elt.id = nbr_elt->id;
		nbr_rep->nbr_elt.len = nbr_elt->len;
		nbr_rep->nbr_elt.reg = nbr_elt->reg;
		nbr_rep->nbr_elt.channel = nbr_elt->channel;
		nbr_rep->nbr_elt.phytype = nbr_elt->phytype;
		nbr_rep->nbr_elt.bssid_info = nbr_elt->bssid_info;
		nbr_rep->nbr_elt.bss_trans_preference =
			(nbr_elt->bss_trans_preference == 0) ? 1 : nbr_elt->bss_trans_preference;
	} else {
		nbr_rep_cnt = wlc_rrm_get_neighbor_count(rrm_info, rrm_cfg);
		if (nbr_rep_cnt >= MAX_NEIGHBORS) {
			WL_ERROR(("%s nbr_rep_cnt %d is over MAX_NEIGHBORS\n", __FUNCTION__,
				nbr_rep_cnt));
			return BCME_ERROR;
		}
		nbr_rep = (rrm_nbr_rep_t *)MALLOCZ(rrm_info->wlc->osh, sizeof(rrm_nbr_rep_t));
		if (nbr_rep == NULL) {
			WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(rrm_info->wlc), __FUNCTION__,
				(int)sizeof(rrm_nbr_rep_t), MALLOCED(wlc->osh)));
			return BCME_NOMEM;
		}
		nbr_rep->nbr_elt.id = nbr_elt->id;
		nbr_rep->nbr_elt.len = nbr_elt->len;
		nbr_rep->nbr_elt.reg = nbr_elt->reg;
		nbr_rep->nbr_elt.channel = nbr_elt->channel;
		nbr_rep->nbr_elt.phytype = nbr_elt->phytype;
		nbr_rep->nbr_elt.bssid_info = nbr_elt->bssid_info;
		nbr_rep->nbr_elt.bss_trans_preference =
			(nbr_elt->bss_trans_preference == 0) ? 1 : nbr_elt->bss_trans_preference;
		nbr_rep->next = rrm_cfg->nbr_rep_head;
		rrm_cfg->nbr_rep_head = nbr_rep;
	}

	wlc_rrm_add_regclass_neighbor_count(rrm_info, nbr_rep, &channel_added, rrm_cfg);

	should_update_beacon = !channels_same && (channel_added || channel_removed);

	/* Update beacons and probe responses  */
	if (should_update_beacon) {
		bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
		ASSERT(bsscfg);

		if (bsscfg->up &&
		    (BSSCFG_AP(bsscfg) || (!bsscfg->BSS && !BSS_TDLS_ENAB(wlc, bsscfg)))) {
			/* Update AP or IBSS beacons */
			wlc_bss_update_beacon(wlc, bsscfg);
			/* Update AP or IBSS probe responses */
			wlc_bss_update_probe_resp(wlc, bsscfg, TRUE);

		}
	}
	return BCME_OK;
}

static int
wlc_rrm_add_regclass_neighbor_count(wlc_rrm_info_t *rrm_info, rrm_nbr_rep_t *nbr_rep,
	bool *channel_added, rrm_bsscfg_cubby_t *rrm_cfg)
{
	reg_nbr_count_t *nbr_cnt;
	wlc_info_t *wlc;
	uint16 list_cnt;
	bool nbr_cnt_find = FALSE;

	wlc = rrm_info->wlc;
	*channel_added = FALSE;

	nbr_cnt = rrm_cfg->nbr_cnt_head;
	while (nbr_cnt) {
		if (nbr_rep->nbr_elt.reg == nbr_cnt->reg) {
			nbr_cnt->nbr_count_by_channel[nbr_rep->nbr_elt.channel] += 1;

			if (nbr_cnt->nbr_count_by_channel[nbr_rep->nbr_elt.channel] == 1)
				*channel_added = TRUE;

			nbr_cnt_find = TRUE;
			break;
		}
		nbr_cnt = nbr_cnt->next;
	}

	if (!nbr_cnt_find) {
		list_cnt = wlc_rrm_get_reg_count(rrm_info, rrm_cfg);
		if (list_cnt >= MAX_CHANNEL_REPORT_IES) {
			WL_ERROR(("%s over MAX_CHANNEL_REPORT_IES\n", __FUNCTION__));
			return BCME_ERROR;
		}
		nbr_cnt = (reg_nbr_count_t *)MALLOCZ(wlc->osh, sizeof(reg_nbr_count_t));
		if (nbr_cnt == NULL) {
			WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
				(int)sizeof(reg_nbr_count_t), MALLOCED(wlc->osh)));
			return BCME_NOMEM;
		}
		nbr_cnt->reg = nbr_rep->nbr_elt.reg;
		nbr_cnt->nbr_count_by_channel[nbr_rep->nbr_elt.channel] += 1;
		nbr_cnt->next = rrm_cfg->nbr_cnt_head;
		rrm_cfg->nbr_cnt_head = nbr_cnt;

		*channel_added = TRUE;
	}
	return BCME_OK;
}

static uint16
wlc_rrm_get_neighbor_count(wlc_rrm_info_t *rrm_info, rrm_bsscfg_cubby_t *rrm_cfg)
{
	uint16 list_cnt = 0;
	rrm_nbr_rep_t *nbr_rep;

	nbr_rep = rrm_cfg->nbr_rep_head;
	while (nbr_rep) {
		nbr_rep = nbr_rep->next;
		list_cnt++;
	}

	return list_cnt;
}

static uint16
wlc_rrm_get_reg_count(wlc_rrm_info_t *rrm_info, rrm_bsscfg_cubby_t *rrm_cfg)
{
	uint16 list_cnt = 0;
	reg_nbr_count_t *nbr_cnt;

	nbr_cnt = rrm_cfg->nbr_cnt_head;
	while (nbr_cnt) {
		nbr_cnt = nbr_cnt->next;
		list_cnt++;
	}
	return list_cnt;
}

static uint16
wlc_rrm_get_bcnnbr_count(wlc_rrm_info_t *rrm_info, rrm_scb_info_t *scb_info)
{
	uint16 list_cnt = 0;
	rrm_nbr_rep_t *nbr_rep;

	nbr_rep = scb_info->nbr_rep_head;
	while (nbr_rep) {
		nbr_rep = nbr_rep->next;
		list_cnt++;
	}
	return list_cnt;
}

static int
wlc_rrm_add_bcnnbr(wlc_rrm_info_t *rrm_info, struct scb *scb, dot11_rmrep_bcn_t *rmrep_bcn,
	struct dot11_bcn_prb *bcn, bcm_tlv_t *htie, bcm_tlv_t *mdie, uint32 flags)
{
	rrm_nbr_rep_t *nbr_rep;
	wlc_info_t *wlc;
	uint16 nbr_rep_cnt;
	rrm_scb_info_t *scb_info;
	uint32 apsd = 0;

	scb_info = RRM_SCB_INFO(rrm_info, scb);

	wlc = rrm_info->wlc;
	if ((flags & WLC_BSS_BRCM) && (flags & WLC_BSS_WME)) {
		apsd = DOT11_NGBR_BI_CAP_APSD; /* APSD is enabled by default if wme is enabled */
	}
	else {
		apsd = ((bcn->capability & DOT11_CAP_APSD)? DOT11_NGBR_BI_CAP_APSD : 0);
	}
	/* Find Neighbor Report element from list */
	nbr_rep = scb_info->nbr_rep_head;
	while (nbr_rep) {
		if (memcmp(&nbr_rep->nbr_elt.bssid, &rmrep_bcn->bssid, ETHER_ADDR_LEN) == 0)
			break;
		nbr_rep = nbr_rep->next;
	}

	if (nbr_rep) {
		nbr_rep->nbr_elt.id = DOT11_MNG_NEIGHBOR_REP_ID;
		nbr_rep->nbr_elt.len = DOT11_NEIGHBOR_REP_IE_FIXED_LEN +
			TLV_HDR_LEN + DOT11_NGBR_BSSTRANS_PREF_SE_LEN;
		memcpy(&nbr_rep->nbr_elt.bssid, &rmrep_bcn->bssid, ETHER_ADDR_LEN);

		nbr_rep->nbr_elt.ssid.SSID_len = scb->bsscfg->SSID_len;
		memcpy(&nbr_rep->nbr_elt.ssid.SSID, &scb->bsscfg->SSID,
			nbr_rep->nbr_elt.ssid.SSID_len);

		nbr_rep->nbr_elt.bssid_info = DOT11_NGBR_BI_REACHABILTY | DOT11_NGBR_BI_SEC |
			((bcn->capability & DOT11_CAP_SPECTRUM)? DOT11_NGBR_BI_CAP_SPEC_MGMT : 0) |
			((flags & WLC_BSS_WME)? DOT11_NGBR_BI_CAP_QOS : 0) |
			(apsd) |
			((bcn->capability & DOT11_CAP_RRM)? DOT11_NGBR_BI_CAP_RDIO_MSMT : 0) |
			((bcn->capability & DOT11_CAP_DELAY_BA)? DOT11_NGBR_BI_CAP_DEL_BA : 0) |
			((bcn->capability & DOT11_CAP_IMMEDIATE_BA)? DOT11_NGBR_BI_CAP_IMM_BA : 0);

		if (htie != NULL)
			nbr_rep->nbr_elt.bssid_info |= DOT11_NGBR_BI_HT;

		if (mdie != NULL)
			nbr_rep->nbr_elt.bssid_info |= DOT11_NGBR_BI_MOBILITY;

		nbr_rep->nbr_elt.reg = rmrep_bcn->reg;
		nbr_rep->nbr_elt.channel = rmrep_bcn->channel;
		nbr_rep->nbr_elt.phytype = (rmrep_bcn->frame_info >> 1);
		nbr_rep->nbr_elt.bss_trans_preference = rmrep_bcn->rsni; /* fix */
	}
	else {
		nbr_rep_cnt = wlc_rrm_get_bcnnbr_count(rrm_info, scb_info);
		if (nbr_rep_cnt >= MAX_BEACON_NEIGHBORS) {
			WL_ERROR(("%s nbr_rep_cnt: %d is over MAX_BEACON_NEIGHBORS\n",
				__FUNCTION__, nbr_rep_cnt));
			return BCME_ERROR;
		}
		nbr_rep = (rrm_nbr_rep_t *)MALLOCZ(wlc->osh, sizeof(rrm_nbr_rep_t));
		if (nbr_rep == NULL) {
			WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
				(int)sizeof(rrm_nbr_rep_t), MALLOCED(wlc->osh)));
			return BCME_NOMEM;
		}

		nbr_rep->nbr_elt.id = DOT11_MNG_NEIGHBOR_REP_ID;
		nbr_rep->nbr_elt.len = DOT11_NEIGHBOR_REP_IE_FIXED_LEN +
			TLV_HDR_LEN + DOT11_NGBR_BSSTRANS_PREF_SE_LEN;
		memcpy(&nbr_rep->nbr_elt.bssid, &rmrep_bcn->bssid, ETHER_ADDR_LEN);

		nbr_rep->nbr_elt.ssid.SSID_len = scb->bsscfg->SSID_len;
		memcpy(&nbr_rep->nbr_elt.ssid.SSID, &scb->bsscfg->SSID,
			nbr_rep->nbr_elt.ssid.SSID_len);

		nbr_rep->nbr_elt.bssid_info = DOT11_NGBR_BI_REACHABILTY | DOT11_NGBR_BI_SEC |
			((bcn->capability & DOT11_CAP_SPECTRUM)? DOT11_NGBR_BI_CAP_SPEC_MGMT : 0) |
			((flags & WLC_BSS_WME)? DOT11_NGBR_BI_CAP_QOS : 0) |
			(apsd) |
			((bcn->capability & DOT11_CAP_RRM)? DOT11_NGBR_BI_CAP_RDIO_MSMT : 0) |
			((bcn->capability & DOT11_CAP_DELAY_BA)? DOT11_NGBR_BI_CAP_DEL_BA : 0) |
			((bcn->capability & DOT11_CAP_IMMEDIATE_BA)? DOT11_NGBR_BI_CAP_IMM_BA : 0);

		if (htie != NULL)
			nbr_rep->nbr_elt.bssid_info |= DOT11_NGBR_BI_HT;

		if (mdie != NULL)
			nbr_rep->nbr_elt.bssid_info |= DOT11_NGBR_BI_MOBILITY;

		nbr_rep->nbr_elt.reg = rmrep_bcn->reg;
		nbr_rep->nbr_elt.channel = rmrep_bcn->channel;
		nbr_rep->nbr_elt.phytype = (rmrep_bcn->frame_info >> 1);
		nbr_rep->nbr_elt.bss_trans_preference = rmrep_bcn->rsni; /* fix */

		nbr_rep->next = scb_info->nbr_rep_head;
		scb_info->nbr_rep_head = nbr_rep;
	}

	return BCME_OK;
}

static void
wlc_rrm_flush_bcnnbrs(wlc_rrm_info_t *rrm_info, struct scb *scb)
{
	rrm_nbr_rep_t *nbr_rep, *nbr_rep_next;
	rrm_scb_info_t *scb_info = NULL;

	scb_info = RRM_SCB_INFO(rrm_info, scb);
	if (scb_info != NULL) {
		nbr_rep = scb_info->nbr_rep_head;
		while (nbr_rep) {
			nbr_rep_next = nbr_rep->next;
			MFREE(rrm_info->wlc->osh, nbr_rep, sizeof(rrm_nbr_rep_t));
			nbr_rep = nbr_rep_next;
		}
		scb_info->nbr_rep_head = NULL;
	}
}
#endif /* WL11K_AP */

#ifdef WL_FTM_11K
#if defined(WL_PROXDETECT) && defined(WL_FTM)
static uint8 *
wlc_rrm_create_lci_meas_element(uint8 *ptr, rrm_bsscfg_cubby_t *rrm_cfg,
	rrm_nbr_rep_t *nbr_rep, int *buflen)
{
	dot11_meas_rep_t *lci_rep = (dot11_meas_rep_t *) ptr;
	uint copy_len;

	if (*buflen < DOT11_RM_IE_LEN) {
		WL_ERROR(("%s: buf too small; buflen %d; leaving off LCI\n",
			__FUNCTION__, *buflen));
		goto done;
	}
	lci_rep->id = DOT11_MNG_MEASURE_REPORT_ID;
	lci_rep->type = DOT11_MEASURE_TYPE_LCI;
	lci_rep->mode = 0;
	lci_rep->token = rrm_cfg->lci_token;

	ptr += DOT11_RM_IE_LEN;
	*buflen -= DOT11_RM_IE_LEN;

	if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LCIM)) {
		/* Send incapable */
		lci_rep->mode |= DOT11_MEASURE_MODE_INCAPABLE;
		lci_rep->len = DOT11_MNG_IE_MREP_FIXED_LEN;
		goto done;
	}

	if (nbr_rep->opt_lci != NULL) {
		dot11_meas_rep_t *opt_lci_rep = (dot11_meas_rep_t *) nbr_rep->opt_lci;
		copy_len = opt_lci_rep->len - (DOT11_RM_IE_LEN - TLV_HDR_LEN);

		if (*buflen < copy_len) {
			WL_ERROR(("%s: buf too small; buflen %d, need %d; leaving off LCI\n",
				__FUNCTION__, *buflen, copy_len));
			ptr -= DOT11_RM_IE_LEN;
			*buflen += DOT11_RM_IE_LEN;
			goto done;
		}

		memcpy(ptr, nbr_rep->opt_lci+DOT11_RM_IE_LEN, copy_len);
		lci_rep->len = opt_lci_rep->len;
		ptr += copy_len;
		*buflen -= copy_len;
	} else {
		if (*buflen < DOT11_MNG_IE_MREP_LCI_FIXED_LEN - DOT11_MNG_IE_MREP_FIXED_LEN) {
			WL_ERROR(("%s: buf too small; buflen %d, need %d; leaving off LCI\n",
				__FUNCTION__, *buflen,
				DOT11_MNG_IE_MREP_LCI_FIXED_LEN - DOT11_MNG_IE_MREP_FIXED_LEN));
			ptr -= DOT11_RM_IE_LEN;
			*buflen += DOT11_RM_IE_LEN;
			goto done;
		}
		/* Send unknown */
		lci_rep->rep.lci.subelement = DOT11_FTM_LCI_SUBELEM_ID;
		lci_rep->rep.lci.length = 0;
		lci_rep->len = DOT11_MNG_IE_MREP_LCI_FIXED_LEN;

		ptr += (DOT11_MNG_IE_MREP_LCI_FIXED_LEN - TLV_HDR_LEN);
		*buflen -= (DOT11_MNG_IE_MREP_LCI_FIXED_LEN - TLV_HDR_LEN);
	}

done:
	return ptr;
}

static uint8 *
wlc_rrm_create_civic_meas_element(uint8 *ptr, rrm_bsscfg_cubby_t *rrm_cfg,
	rrm_nbr_rep_t *nbr_rep, int *buflen)
{
	dot11_meas_rep_t *civic_rep = (dot11_meas_rep_t *) ptr;
	uint copy_len;

	if (*buflen < DOT11_RM_IE_LEN) {
		WL_ERROR(("%s: buf too small; buflen %d; leaving off CIVIC\n",
			__FUNCTION__, *buflen));
		goto done;
	}
	civic_rep->id = DOT11_MNG_MEASURE_REPORT_ID;
	civic_rep->type = DOT11_MEASURE_TYPE_CIVICLOC;
	civic_rep->mode = 0;
	civic_rep->token = rrm_cfg->civic_token;

	ptr += DOT11_RM_IE_LEN;
	*buflen -= DOT11_RM_IE_LEN;

	if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_CIVIC_LOC)) {
		civic_rep->mode |= DOT11_MEASURE_MODE_INCAPABLE;
		civic_rep->len = DOT11_MNG_IE_MREP_FIXED_LEN;
		goto done;
	}

	if (nbr_rep->opt_civic != NULL) {
		dot11_meas_rep_t *opt_civic_rep = (dot11_meas_rep_t *) nbr_rep->opt_civic;
		copy_len = opt_civic_rep->len - (DOT11_RM_IE_LEN - TLV_HDR_LEN);

		if (*buflen < copy_len) {
			WL_ERROR(("%s: buf too small; buflen %d, need %d; leaving off CIVIC\n",
				__FUNCTION__, *buflen, copy_len));
			ptr -= DOT11_RM_IE_LEN;
			*buflen += DOT11_RM_IE_LEN;
			goto done;
		}

		memcpy(ptr, nbr_rep->opt_civic+DOT11_RM_IE_LEN, copy_len);
		civic_rep->len = opt_civic_rep->len;
		ptr += copy_len;
		*buflen -= copy_len;
	} else {
		if (*buflen < DOT11_MNG_IE_MREP_CIVIC_FIXED_LEN - DOT11_MNG_IE_MREP_FIXED_LEN) {
			WL_ERROR(("%s: buf too small; buflen %d, need %d; leaving off CIVIC\n",
				__FUNCTION__, *buflen,
				DOT11_MNG_IE_MREP_CIVIC_FIXED_LEN - DOT11_MNG_IE_MREP_FIXED_LEN));
			ptr -= DOT11_RM_IE_LEN;
			*buflen += DOT11_RM_IE_LEN;
			goto done;
		}
		/* Send unknown */
		civic_rep->rep.civic.type = DOT11_FTM_CIVIC_LOC_TYPE_RFC4776;
		civic_rep->rep.civic.subelement = DOT11_FTM_CIVIC_SUBELEM_ID;
		civic_rep->rep.civic.length = 0;
		civic_rep->len = DOT11_MNG_IE_MREP_CIVIC_FIXED_LEN;

		ptr += (DOT11_MNG_IE_MREP_CIVIC_FIXED_LEN - TLV_HDR_LEN);
		*buflen -= (DOT11_MNG_IE_MREP_CIVIC_FIXED_LEN - TLV_HDR_LEN);
	}
done:
	return ptr;
}

static uint8 *
wlc_write_wide_bw_chan_ie(chanspec_t chspec, uint8 *cp, int buflen)
{
	dot11_wide_bw_chan_ie_t *wide_bw_chan_ie;
	uint8 center_chan;

	/* perform buffer length check. */
	/* if not big enough, return buffer untouched */
	BUFLEN_CHECK_AND_RETURN((TLV_HDR_LEN + DOT11_WIDE_BW_IE_LEN), buflen, cp);

	wide_bw_chan_ie = (dot11_wide_bw_chan_ie_t *) cp;
	wide_bw_chan_ie->id = DOT11_NGBR_WIDE_BW_CHAN_SE_ID;
	wide_bw_chan_ie->len = DOT11_WIDE_BW_IE_LEN;

	if (CHSPEC_IS40(chspec))
		wide_bw_chan_ie->channel_width = WIDE_BW_CHAN_WIDTH_40;
	else if (CHSPEC_IS80(chspec))
		wide_bw_chan_ie->channel_width = WIDE_BW_CHAN_WIDTH_80;
	else if (CHSPEC_IS160(chspec))
		wide_bw_chan_ie->channel_width = WIDE_BW_CHAN_WIDTH_160;
	else if (CHSPEC_IS8080(chspec))
		wide_bw_chan_ie->channel_width = WIDE_BW_CHAN_WIDTH_80_80;
	else
		wide_bw_chan_ie->channel_width = WIDE_BW_CHAN_WIDTH_20;

	if (CHSPEC_IS8080(chspec)) {
		wide_bw_chan_ie->center_frequency_segment_0 =
			wf_chspec_primary80_channel(chspec);
		wide_bw_chan_ie->center_frequency_segment_1 =
			wf_chspec_secondary80_channel(chspec);
	}
	else {
		center_chan = CHSPEC_CHANNEL(chspec) >> WL_CHANSPEC_CHAN_SHIFT;
		wide_bw_chan_ie->center_frequency_segment_0 = center_chan;
		wide_bw_chan_ie->center_frequency_segment_1 = 0;
	}

	cp += (TLV_HDR_LEN + DOT11_WIDE_BW_IE_LEN);

	return cp;
}

static void
wlc_rrm_create_nbrrep_element(rrm_bsscfg_cubby_t *rrm_cfg, uint8 **ptr,
	rrm_nbr_rep_t *nbr_rep, uint8 flags, uint8 *bufend)
{
	dot11_neighbor_rep_ie_t *nbrrep;
	int buflen = MIN(TLV_BODY_LEN_MAX, BUFLEN(*ptr, bufend));
	uint8 *nrrep_start = *ptr;

	nbrrep = (dot11_neighbor_rep_ie_t *) *ptr;

	nbrrep->id = nbr_rep->nbr_elt.id;
	nbrrep->len = DOT11_NEIGHBOR_REP_IE_FIXED_LEN;

	memcpy(&nbrrep->bssid, &nbr_rep->nbr_elt.bssid, ETHER_ADDR_LEN);

	store32_ua((uint8 *)&nbrrep->bssid_info, nbr_rep->nbr_elt.bssid_info);

	nbrrep->reg = nbr_rep->nbr_elt.reg;
	nbrrep->channel = nbr_rep->nbr_elt.channel;
	nbrrep->phytype = nbr_rep->nbr_elt.phytype;

	/* Check for optional elements */
	*ptr += TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN;
	buflen -= TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN;

	if (wf_chspec_valid(nbr_rep->nbr_elt.chanspec)) {
		uint8 *wide_bw_ch;
		buflen = MIN(buflen, BUFLEN(*ptr, bufend));
		wide_bw_ch = wlc_write_wide_bw_chan_ie(nbr_rep->nbr_elt.chanspec, *ptr,
			buflen);
		buflen -= (wide_bw_ch - *ptr);
		*ptr = wide_bw_ch;
	}

	if (flags & RRM_NREQ_FLAGS_LCI_PRESENT) {
		buflen = MIN(buflen, BUFLEN(*ptr, bufend));
		*ptr = wlc_rrm_create_lci_meas_element(*ptr, rrm_cfg, nbr_rep,
				&buflen);
	}

	if (flags & RRM_NREQ_FLAGS_CIVICLOC_PRESENT) {
		buflen = MIN(buflen, BUFLEN(*ptr, bufend));
		*ptr = wlc_rrm_create_civic_meas_element(*ptr, rrm_cfg, nbr_rep,
				&buflen);
	}
	/* update neighbor report length */
	nbrrep->len = (*ptr - nrrep_start) - TLV_HDR_LEN;
}

/* rrm_frng SET iovar */
static int
rrm_frng_set(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	void *p, uint plen, void *a, int alen)
{
	int err = BCME_BADARG;
	rrm_config_fn_t fn = NULL;
	wl_rrm_frng_ioc_t *rrm_cfg_cmd = (wl_rrm_frng_ioc_t *)a;

	if ((a == NULL) || (alen < WL_RRM_FRNG_MIN_LENGTH)) {
		return BCME_BUFTOOSHORT;
	}

	if (rrm_cfg_cmd->id <= WL_RRM_FRNG_NONE ||
		rrm_cfg_cmd->id >= WL_RRM_FRNG_MAX)
		return BCME_BADARG;

	fn = rrmfrng_cmd_fn[rrm_cfg_cmd->id];
	if (fn) {
		err = fn(rrm_info, cfg, &rrm_cfg_cmd->data[0], (int)rrm_cfg_cmd->len);
	}

	return err;
}
#endif /* WL_PROXDETECT && WL_FTM */

/* rrm_config GET iovar */
static int
rrm_config_get(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	void *p, uint plen, void *a, int alen)
{
	int err = BCME_BADARG;
	rrm_config_fn_t fn = NULL;
	wl_rrm_config_ioc_t *param = (wl_rrm_config_ioc_t *)p;

	if ((a == NULL) || (alen < WL_RRM_CONFIG_MIN_LENGTH)) {
		return BCME_BUFTOOSHORT;
	}
	if ((p == NULL) || (plen < WL_RRM_CONFIG_MIN_LENGTH)) {
		return BCME_BADARG;
	}

	if (param->id <= WL_RRM_CONFIG_NONE ||
		param->id >= WL_RRM_CONFIG_MAX)
		return BCME_BADARG;

	fn = rrmconfig_cmd_fn[param->id];
	if (fn) {
		err = fn(rrm_info, cfg, a, alen);
	}

	return err;
}

/* rrm_config SET iovar */
static int
rrm_config_set(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	void *p, uint plen, void *a, int alen)
{
	int err = BCME_BADARG;
	rrm_config_fn_t fn = NULL;
	wl_rrm_config_ioc_t *rrm_cfg_cmd = (wl_rrm_config_ioc_t *)a;

	if ((a == NULL) || (alen < WL_RRM_CONFIG_MIN_LENGTH)) {
		return BCME_BUFTOOSHORT;
	}
	if (alen > (TLV_BODY_LEN_MAX + WL_RRM_CONFIG_MIN_LENGTH -
			DOT11_MNG_IE_MREP_FIXED_LEN)) {
		return BCME_BUFTOOLONG;
	}

	if (rrm_cfg_cmd->id <= WL_RRM_CONFIG_NONE ||
		rrm_cfg_cmd->id >= WL_RRM_CONFIG_MAX)
		return BCME_BADARG;

	fn = rrmconfig_cmd_fn[rrm_cfg_cmd->id];
	if (fn) {
		err = fn(rrm_info, cfg, &rrm_cfg_cmd->data[0], (int)rrm_cfg_cmd->len);
	}

	return err;
}

/*
 * 'len' param only includes what comes after the "type" field
 * of the measurement report.
*/
static int
wlc_rrm_add_lci_opt(wlc_rrm_info_t *rrm_info, rrm_nbr_rep_t *nbr_rep,
	uint8 *opt_lci, int len)
{
	dot11_meas_rep_t *lci_rep;

	if (nbr_rep->opt_lci) {
		lci_rep = (dot11_meas_rep_t *) nbr_rep->opt_lci;
		MFREE(rrm_info->wlc->osh, lci_rep, lci_rep->len + TLV_HDR_LEN);
		nbr_rep->opt_lci = NULL;
		lci_rep = NULL;
	}

	/* add 3 bytes for token, mode and type */
	len += len ? DOT11_MNG_IE_MREP_FIXED_LEN : DOT11_MNG_IE_MREP_LCI_FIXED_LEN;
	/* malloc enough to store the total measurement report starting with "meas rep id" */
	nbr_rep->opt_lci = MALLOCZ(rrm_info->wlc->osh, (TLV_HDR_LEN + len));

	if (nbr_rep->opt_lci != NULL) {
		lci_rep = (dot11_meas_rep_t *)(nbr_rep->opt_lci);

		lci_rep->id = DOT11_MNG_MEASURE_REPORT_ID;
		lci_rep->type = DOT11_MEASURE_TYPE_LCI;
		lci_rep->len = len;

		if (len == DOT11_MNG_IE_MREP_LCI_FIXED_LEN) {
			/* Use unknown for empty config */
			lci_rep->rep.lci.subelement = DOT11_FTM_LCI_SUBELEM_ID;
			lci_rep->rep.lci.length = 0;
		} else {
			memcpy(&lci_rep->rep.lci.subelement, opt_lci,
				(len - DOT11_MNG_IE_MREP_FIXED_LEN));
		}
		return BCME_OK;
	} else {
		WL_ERROR(("%s: out of memory, malloced %d bytes\n",
			__FUNCTION__, MALLOCED(rrm_info->wlc->osh)));
		return BCME_NOMEM;
	}
}

static int
wlc_rrm_set_self_lci(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *opt_lci, int len)
{
	rrm_nbr_rep_t *nbr_rep;
	int ret = BCME_OK;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
	wlc_info_t *wlc = rrm_info->wlc;

	if (len < DOT11_FTM_LCI_UNKNOWN_LEN) {
		return BCME_BUFTOOSHORT;
	}

	nbr_rep = rrm_cfg->nbr_rep_self;

	if (nbr_rep == NULL) {
		nbr_rep = (rrm_nbr_rep_t *)MALLOCZ(rrm_info->wlc->osh, sizeof(rrm_nbr_rep_t));
		if (nbr_rep == NULL) {
			ret = BCME_NOMEM;
			goto done;
		}
		rrm_cfg->nbr_rep_self = nbr_rep;
	}

	ret = wlc_rrm_add_lci_opt(rrm_info, nbr_rep, opt_lci, len);

done:
	if (ret != BCME_OK) {
		WL_ERROR(("%s: Failed to create self LCI of len %d, status %d\n",
			__FUNCTION__, len, ret));
	}
	else {
		wlc_bsscfg_set_ext_cap(cfg, DOT11_EXT_CAP_LCI, (len != DOT11_FTM_LCI_UNKNOWN_LEN));
		if (len != DOT11_FTM_LCI_UNKNOWN_LEN)	{
			setbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LCIM);
		}
		else {
			clrbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LCIM);
		}

		if (cfg->up &&
		    (BSSCFG_AP(cfg) || (!cfg->BSS && !BSS_TDLS_ENAB(wlc, cfg)))) {
			/* Update AP or IBSS beacons */
			wlc_bss_update_beacon(wlc, cfg);
			/* Update AP or IBSS probe responses */
			wlc_bss_update_probe_resp(wlc, cfg, TRUE);
		}
	}

	return ret;
}

static int
wlc_rrm_get_self_lci(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *a, int alen)
{
	wl_rrm_config_ioc_t *rrm_cfg_cmd = (wl_rrm_config_ioc_t *)a; /* output */
	dot11_meas_rep_t *lci_rep;
	int err = BCME_OK;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);

	if (rrm_cfg->nbr_rep_self &&
			rrm_cfg->nbr_rep_self->opt_lci) {
		lci_rep = (dot11_meas_rep_t *)
			rrm_cfg->nbr_rep_self->opt_lci;
		if (alen >= (lci_rep->len + WL_RRM_CONFIG_MIN_LENGTH -
				DOT11_MNG_IE_MREP_FIXED_LEN)) {
			memcpy(&rrm_cfg_cmd->data[0], &lci_rep->rep.data[0],
				(lci_rep->len - DOT11_MNG_IE_MREP_FIXED_LEN));
			rrm_cfg_cmd->len = lci_rep->len - DOT11_MNG_IE_MREP_FIXED_LEN;
		} else {
			err = BCME_BUFTOOSHORT;
		}
	} else {
		/* set output to unknown LCI */
		rrm_cfg_cmd->len = DOT11_FTM_LCI_UNKNOWN_LEN;
		memset(&rrm_cfg_cmd->data[0], 0, DOT11_FTM_LCI_UNKNOWN_LEN);
	}

	return err;
}

static int
wlc_rrm_get_self_civic(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *a, int alen)
{
	wl_rrm_config_ioc_t *rrm_cfg_cmd = (wl_rrm_config_ioc_t *)a; /* output */
	dot11_meas_rep_t *civic_rep;
	int err = BCME_OK;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);

	if (rrm_cfg->nbr_rep_self &&
			rrm_cfg->nbr_rep_self->opt_civic) {
		civic_rep =	(dot11_meas_rep_t *)
			rrm_cfg->nbr_rep_self->opt_civic;
		if (alen >= (civic_rep->len + WL_RRM_CONFIG_MIN_LENGTH -
				DOT11_MNG_IE_MREP_FIXED_LEN)) {
			memcpy(&rrm_cfg_cmd->data[0], &civic_rep->rep.data[0],
				(civic_rep->len - DOT11_MNG_IE_MREP_FIXED_LEN));
			rrm_cfg_cmd->len = civic_rep->len - DOT11_MNG_IE_MREP_FIXED_LEN;
		} else {
			err = BCME_BUFTOOSHORT;
		}
	} else {
		/* set output to unknown civic */
		rrm_cfg_cmd->len = DOT11_FTM_CIVIC_UNKNOWN_LEN;
		memset(&rrm_cfg_cmd->data[0], 0, DOT11_FTM_CIVIC_UNKNOWN_LEN);
	}

	return err;
}

/*
 * 'len' param only includes what comes after the "type" field
 * of the measurement report.
*/
static int
wlc_rrm_add_civic_opt(wlc_rrm_info_t *rrm_info, rrm_nbr_rep_t *nbr_rep,
	uint8 *opt_civic, int len)
{
	dot11_meas_rep_t *civic_rep;

	if (nbr_rep->opt_civic) {
		civic_rep = (dot11_meas_rep_t *) nbr_rep->opt_civic;
		MFREE(rrm_info->wlc->osh, civic_rep, civic_rep->len + TLV_HDR_LEN);
		nbr_rep->opt_civic = NULL;
		civic_rep = NULL;
	}

	/* add 3 bytes for token, mode and type */
	len += len ? DOT11_MNG_IE_MREP_FIXED_LEN : DOT11_MNG_IE_MREP_CIVIC_FIXED_LEN;
	/* malloc enough to store the total measurement report starting with "meas rep id" */
	nbr_rep->opt_civic = MALLOCZ(rrm_info->wlc->osh, (TLV_HDR_LEN + len));

	if (nbr_rep->opt_civic != NULL) {
		civic_rep = (dot11_meas_rep_t *)(nbr_rep->opt_civic);
		civic_rep->id = DOT11_MNG_MEASURE_REPORT_ID;
		civic_rep->type = DOT11_MEASURE_TYPE_CIVICLOC;
		civic_rep->len = len;

		if (len == DOT11_MNG_IE_MREP_CIVIC_FIXED_LEN) {
			/* Use unknown for empty config */
			civic_rep->rep.civic.type = DOT11_FTM_CIVIC_LOC_TYPE_RFC4776;
			civic_rep->rep.civic.subelement = DOT11_FTM_CIVIC_SUBELEM_ID;
			civic_rep->rep.civic.length = 0;
		} else {
			memcpy(&civic_rep->rep.civic.type, opt_civic,
				(len - DOT11_MNG_IE_MREP_FIXED_LEN));
		}
		return BCME_OK;
	} else {
		WL_ERROR(("%s: out of memory, malloced %d bytes\n",
			__FUNCTION__, MALLOCED(rrm_info->wlc->osh)));
		return BCME_NOMEM;
	}
}

static int
wlc_rrm_set_self_civic(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	uint8 *opt_civic, int len)
{
	rrm_nbr_rep_t *nbr_rep;
	int ret = BCME_OK;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
	wlc_info_t *wlc = rrm_info->wlc;

	if (len < DOT11_FTM_CIVIC_UNKNOWN_LEN) {
		return BCME_BUFTOOSHORT;
	}

	nbr_rep = rrm_cfg->nbr_rep_self;

	if (nbr_rep == NULL) {
		nbr_rep = (rrm_nbr_rep_t *)MALLOCZ(rrm_info->wlc->osh, sizeof(rrm_nbr_rep_t));
		if (nbr_rep == NULL) {
			ret = BCME_NOMEM;
			goto done;
		}
		rrm_cfg->nbr_rep_self = nbr_rep;
	}

	ret = wlc_rrm_add_civic_opt(rrm_info, nbr_rep, opt_civic, len);

done:
	if (ret != BCME_OK) {
		WL_ERROR(("%s: Failed to create self civic, len %d, status %d\n",
			__FUNCTION__, len, ret));
	}
	else {
		wlc_bsscfg_set_ext_cap(cfg, DOT11_EXT_CAP_CIVIC_LOC,
			(len != DOT11_FTM_CIVIC_UNKNOWN_LEN));

		if (len != DOT11_FTM_CIVIC_UNKNOWN_LEN)	{
			setbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_CIVIC_LOC);
		}
		else {
			clrbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_CIVIC_LOC);
		}
		if (cfg->up &&
		    (BSSCFG_AP(cfg) || (!cfg->BSS && !BSS_TDLS_ENAB(wlc, cfg)))) {
			/* Update AP or IBSS beacons */
			wlc_bss_update_beacon(wlc, cfg);
			/* Update AP or IBSS probe responses */
			wlc_bss_update_probe_resp(wlc, cfg, TRUE);
		}
	}

	return ret;
}

static void
wlc_rrm_free_self_report(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *bsscfg)
{
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, bsscfg);
	rrm_nbr_rep_t *nbr_rep = rrm_cfg->nbr_rep_self;
	dot11_meas_rep_t *lci_rep;
	dot11_meas_rep_t *civic_rep;

	if (nbr_rep) {
		if (nbr_rep->opt_lci) {
			lci_rep = (dot11_meas_rep_t *) nbr_rep->opt_lci;
			MFREE(rrm_info->wlc->osh, lci_rep,
				lci_rep->len + TLV_HDR_LEN);
		}
		if (nbr_rep->opt_civic) {
			civic_rep = (dot11_meas_rep_t *) nbr_rep->opt_civic;
			MFREE(rrm_info->wlc->osh, civic_rep,
				civic_rep->len + TLV_HDR_LEN);
		}
		MFREE(rrm_info->wlc->osh, nbr_rep, sizeof(rrm_nbr_rep_t));
		rrm_cfg->nbr_rep_self = NULL;
	}

	return;
}

static void
wlc_rrm_recv_lcireq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	dot11_meas_req_loc_t *rmreq_lci, wlc_rrm_req_t *req)
{
	wlc_info_t *wlc = rrm_info->wlc;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, rrm_info->cur_cfg);
	BCM_REFERENCE(wlc); /* in case BCMDBG off */

	if ((rmreq_lci->mode & (DOT11_RMREQ_MODE_PARALLEL | DOT11_RMREQ_MODE_ENABLE)) ||
		!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LCIM)) {
		WL_ERROR(("wl%d: %s: Unsupported LCI request; mode 0x%x\n",
			wlc->pub->unit, __FUNCTION__, rmreq_lci->mode));
		req->flags |= DOT11_RMREP_MODE_INCAPABLE;
		return;
	}

	if (rmreq_lci->req.lci.subject != DOT11_FTM_LOCATION_SUBJ_REMOTE) {
		WL_ERROR(("wl%d: %s: Unsupported LCI request subject %d\n",
			wlc->pub->unit, __FUNCTION__, rmreq_lci->req.lci.subject));
		req->flags |= DOT11_RMREP_MODE_INCAPABLE;
		return;
	}

}

static void
wlc_rrm_recv_civicreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	dot11_meas_req_loc_t *rmreq_civic, wlc_rrm_req_t *req)
{
	wlc_info_t *wlc = rrm_info->wlc;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, rrm_info->cur_cfg);
	BCM_REFERENCE(wlc); /* in case BCMDBG off */

	if ((rmreq_civic->mode & (DOT11_RMREQ_MODE_PARALLEL | DOT11_RMREQ_MODE_ENABLE)) ||
		!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_CIVIC_LOC)) {
		WL_ERROR(("wl%d: %s: Unsupported civic location request; mode 0x%x\n",
			wlc->pub->unit, __FUNCTION__, rmreq_civic->mode));
		req->flags |= DOT11_RMREP_MODE_INCAPABLE;
		return;
	}

	if (rmreq_civic->req.civic.subject != DOT11_FTM_LOCATION_SUBJ_REMOTE) {
		WL_ERROR(("wl%d: %s: Unsupported civic location request subject %d\n",
			wlc->pub->unit, __FUNCTION__, rmreq_civic->req.civic.subject));
		req->flags |= DOT11_RMREP_MODE_INCAPABLE;
		return;
	}

}
#if defined(WL_PROXDETECT) && defined(WL_FTM)
#if defined(WL11AC)
static uint
wide_channel_width_to_bw(uint8 channel_width)
{
	uint bw;

	if (channel_width == WIDE_BW_CHAN_WIDTH_40)
		bw = WL_CHANSPEC_BW_40;
	else if (channel_width == WIDE_BW_CHAN_WIDTH_80)
		bw = WL_CHANSPEC_BW_80;
	else if (channel_width == WIDE_BW_CHAN_WIDTH_160)
		bw = WL_CHANSPEC_BW_160;
	else if (channel_width == WIDE_BW_CHAN_WIDTH_80_80)
		bw = WL_CHANSPEC_BW_8080;
	else
		bw = WL_CHANSPEC_BW_20;

	return bw;
}
#endif /* WL11AC */

static uint
op_channel_width_to_bw(uint8 channel_width)
{
	uint bw;

	if (channel_width == VHT_OP_CHAN_WIDTH_80)
		bw = WL_CHANSPEC_BW_80;
	else if (channel_width == VHT_OP_CHAN_WIDTH_160)
		bw = WL_CHANSPEC_BW_160;
	else if (channel_width == VHT_OP_CHAN_WIDTH_80_80)
		bw = WL_CHANSPEC_BW_8080;
	else
		bw = WL_CHANSPEC_BW_40;

	return bw;
}

static void
wlc_rrm_parse_mr_tlvs(wlc_info_t *wlc, bcm_tlv_t *mr_tlvs, int *mr_tlv_len,
        rrm_nbr_rep_t *nbr_rep, wlc_bsscfg_t *cfg)
{
	while (mr_tlvs) {
		if (mr_tlvs->id == DOT11_MNG_MEASURE_REPORT_ID) {
			dot11_meas_rep_t *meas_rep = (dot11_meas_rep_t *) mr_tlvs;
			dot11_meas_rep_t *meas_elt;

			if (meas_rep->len < DOT11_MNG_IE_MREP_FIXED_LEN) {
				WL_ERROR(("wl%d: %s: malformed element id %d with length %d\n",
					wlc->pub->unit, __FUNCTION__, meas_rep->id, meas_rep->len));
				mr_tlvs = bcm_next_tlv(mr_tlvs, mr_tlv_len);
				continue;
			}

			if ((meas_rep->type == DOT11_MEASURE_TYPE_LCI ||
				meas_rep->type == DOT11_MEASURE_TYPE_CIVICLOC) &&
				!(meas_rep->mode & DOT11_MEASURE_MODE_INCAPABLE)) {

				meas_elt = (dot11_meas_rep_t *)MALLOCZ(wlc->osh,
					meas_rep->len + TLV_HDR_LEN);

				if (meas_elt == NULL) {
					WL_ERROR(("wl%d: %s: out of memory for"
						" measurement report, malloced %d bytes\n",
						wlc->pub->unit, __FUNCTION__,
						MALLOCED(wlc->osh)));
					return;
				} else {
					memcpy((uint8 *) meas_elt, (uint8 *) meas_rep,
						meas_rep->len + TLV_HDR_LEN);
				}
			} else {
				WL_ERROR(("wl%d: %s: Received UNEXPECTED or INCAPABLE"
					" Measurement report type %d\n", wlc->pub->unit,
					__FUNCTION__, meas_rep->type));
				mr_tlvs = bcm_next_tlv(mr_tlvs, mr_tlv_len);
				continue;
			}

			if (meas_rep->type == DOT11_MEASURE_TYPE_LCI) {
				nbr_rep->opt_lci = (uint8 *) meas_elt;
			} else if (meas_rep->type == DOT11_MEASURE_TYPE_CIVICLOC) {
				nbr_rep->opt_civic = (uint8 *) meas_elt;
			}
		}
		else if (mr_tlvs->id == DOT11_MNG_HT_ADD) {
			ht_add_ie_t	*ht_op_ie;
			chanspec_t chspec;

			ht_op_ie = wlc_read_ht_add_ie(wlc, (uint8 *)mr_tlvs, (uint8)*mr_tlv_len);

			if (ht_op_ie) {
				chspec = wlc_ht_chanspec(wlc, ht_op_ie->ctl_ch,
					ht_op_ie->byte1, cfg);
				if (chspec != INVCHANSPEC) {
					nbr_rep->nbr_elt.chanspec = chspec;
					/* also set channel in case there's a descrepancy */
					nbr_rep->nbr_elt.channel = ht_op_ie->ctl_ch;
				}
			}
			else {
				WL_ERROR(("wl%d: %s: Error parsing wlc_read_ht_add_ie id: %d, "
					"len %d\n", wlc->pub->unit, __FUNCTION__,
					mr_tlvs->id, mr_tlvs->len));
			}
		}
#ifdef WL11AC
		else if (mr_tlvs->id == DOT11_MNG_VHT_OPERATION_ID) {
			vht_op_ie_t *op_p;
			vht_op_ie_t op_ie;
			wlc_vht_info_t *vhti = wlc->vhti;

			if ((op_p = wlc_read_vht_op_ie(vhti, (uint8 *)mr_tlvs,
				(uint8)*mr_tlv_len, &op_ie)) != NULL) {
				/* determine the chanspec from VHT Operational IE */
				uint bw;
				uint8 primary_channel = nbr_rep->nbr_elt.channel;

				bw = op_channel_width_to_bw(op_p->chan_width);
				if (bw == WL_CHANSPEC_BW_8080) {
					nbr_rep->nbr_elt.chanspec =
						wf_chspec_get8080_chspec(primary_channel,
							op_p->chan1, op_p->chan2);
				}
				else {
					nbr_rep->nbr_elt.chanspec =
						wf_channel2chspec(primary_channel, bw);
				}
			}
			else {
				WL_ERROR(("wl%d: %s: Error parsing wlc_read_vht_op_ie id: %d, "
					"len %d\n", wlc->pub->unit, __FUNCTION__,
					mr_tlvs->id, mr_tlvs->len));
			}
		}
		/* check for optional neighbor report subelement - wide chan bw */
		else if (mr_tlvs->id == DOT11_NGBR_WIDE_BW_CHAN_SE_ID) {
			uint bw;
			uint8 chan1;
			uint8 chan2;
			uint8 primary_channel = nbr_rep->nbr_elt.channel;
			dot11_wide_bw_chan_ie_t *wide_bw_chan_ie =
				(dot11_wide_bw_chan_ie_t *)mr_tlvs;

			chan1 = wide_bw_chan_ie->center_frequency_segment_0;
			chan2 = wide_bw_chan_ie->center_frequency_segment_1;
			bw = wide_channel_width_to_bw(wide_bw_chan_ie->channel_width);
			if (bw == WL_CHANSPEC_BW_8080) {
				nbr_rep->nbr_elt.chanspec =
					wf_chspec_get8080_chspec(primary_channel, chan1, chan2);
			}
			else {
				nbr_rep->nbr_elt.chanspec = wf_channel2chspec(primary_channel, bw);
			}
		}
#endif /* WL11AC */
		else {
			WL_ERROR(("wl%d: %s: Received UNEXPECTED Element %d, "
				"len %d\n", wlc->pub->unit, __FUNCTION__,
				mr_tlvs->id, mr_tlvs->len));
		}

		mr_tlvs = bcm_next_tlv(mr_tlvs, mr_tlv_len);
	}
}

static void
rrm_ftm_get_meas_start(wlc_info_t *wlc, wlc_ftm_t *ftm, wlc_bsscfg_t *bsscfg,
	wl_proxd_ftm_session_info_t *si, uint32 *meas_start)
{
	uint64 tsf = 0, s_tsf;
	uint32 tsf_lo = 0, tsf_hi = 0;
	uint64 meas_tsf;
	int err = BCME_OK;

	/* convert session meas start to associated bsscfg reference */
#ifdef WLMCNX
	if (MCNX_ENAB(wlc->pub) && wlc_mcnx_tbtt_valid(wlc->mcnx, bsscfg)) {
		wlc_mcnx_read_tsf64(wlc->mcnx, bsscfg, &tsf_hi, &tsf_lo);
	} else
#else
	{
		if (wlc->clk)
			wlc_read_tsf(wlc, &tsf_lo, &tsf_hi);
		else
			err = BCME_NOCLK;
	}
#endif /* WLMCNX */

	if (err != BCME_OK)
		goto done;

	tsf = (uint64)tsf_hi << 32 | (uint64)tsf_lo; /* associated bss tsf */
	err = wlc_ftm_get_session_tsf(ftm, si->sid, &s_tsf);
	if (err != BCME_OK)
		goto done;
	meas_tsf = (uint64)si->meas_start_hi << 32 | (uint64)si->meas_start_lo;
	tsf -= (s_tsf - meas_tsf); /* convert by applying delta */

done:
	store32_ua(meas_start, (uint32)(tsf & 0xffffffffULL));
	WL_TMP(("wl%d.%d: %s: status %d meas start %d.%d\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__,
		err, (uint32)(tsf >> 32), (uint32)tsf));
}

static void
wlc_rrm_ftm_ranging_cb(wlc_info_t *wlc, wlc_ftm_t *ftm,
	wl_proxd_status_t status, wl_proxd_event_type_t event,
	wl_proxd_session_id_t sid, void *cb_ctx)
{
	wl_proxd_ranging_info_t info;
	wl_proxd_rtt_result_t res;
	wl_proxd_ftm_session_info_t session_inf;
	frngrep_t *frng_buf = NULL;
	wlc_rrm_info_t *rrm_info = (wlc_rrm_info_t *)cb_ctx;
	wlc_bsscfg_t *bsscfg;
	int err = BCME_OK;
	int i;
	wlc_ftm_ranging_ctx_t *rctx;
	uint8 err_cnt = 0;
	uint8 success_cnt = 0;
	frngreq_t *frng_req = NULL;

	if (cb_ctx == NULL) {
		return;
	}
	bsscfg = rrm_info->cur_cfg;

	if (bsscfg == NULL) {
		return;
	}

	rctx = rrm_info->rrm_frng_req->rctx;

	if (rctx == NULL) {
		WL_ERROR(("%s: rctx not valid, err %d \n", __FUNCTION__, err));
		goto done;
	}

	err = wlc_ftm_ranging_get_info(rctx, &info);
	if (err != BCME_OK)	{
		WL_ERROR(("%s: wlc_ftm_ranging_get_info failed, err %d \n", __FUNCTION__, err));
		goto done;
	}

	if (info.state != WL_PROXD_RANGING_STATE_DONE) {
		WL_ERROR(("%s: ignoring ranging state %d \n", __FUNCTION__, info.state));
		return;
	}

	frng_req = rrm_info->rrm_frng_req->frng_req;
	if (frng_req == NULL) {
		WL_ERROR(("wl%d: %s: frng_req is NULL\n",
				wlc->pub->unit, __FUNCTION__));
		return;
	}

	rrm_info->rrm_frng_req->frng_rep = (frngrep_t *) MALLOCZ(wlc->osh, sizeof(*frng_buf));
	frng_buf = rrm_info->rrm_frng_req->frng_rep;
	if (frng_buf == NULL) {
		WL_ERROR(("%s: MALLOC failed of %d bytes\n", __FUNCTION__, sizeof(*frng_buf)));
		goto done;
	}

	frng_buf->dialog_token = rrm_info->rrm_frng_req->token;
	memcpy(&frng_buf->da, &rrm_info->da, ETHER_ADDR_LEN);
	frng_buf->range_entry_count = 0;
	frng_buf->error_entry_count = 0;

	for (i = 0; i < frng_req->num_aps; i++) {
		memset(&res, 0, sizeof(res));
		memset(&session_inf, 0, sizeof(session_inf));

		err = wlc_ftm_ranging_get_result(rctx, frng_req->targets[i].sid, 0, &res);
		if (err != BCME_OK) {
			WL_ERROR(("%s: status %d getting result for session sid %d\n",
					__FUNCTION__, err, frng_req->targets[i].sid));
			res.status = err;
		}
		err = wlc_ftm_get_session_info(ftm, frng_req->targets[i].sid, &session_inf);
		if (err != BCME_OK) {
			WL_ERROR(("%s: status %d getting session info for session sid %d\n",
					__FUNCTION__, err, frng_req->targets[i].sid));
			res.status = err;
		}

		switch (res.status)
		{
			case WL_PROXD_E_INVALIDMEAS:
			case WL_PROXD_E_OK:
				memcpy(&frng_buf->range_entries[success_cnt].bssid,
					&frng_req->targets[i].bssid, ETHER_ADDR_LEN);
				if (res.status == WL_PROXD_E_INVALIDMEAS) {
					memset(&frng_buf->range_entries[success_cnt].range, 0xFF,
						sizeof(frng_buf->range_entries[success_cnt].range));
				}
				else {
					/* avg_dist is 1/256 meters, range is 1/4096 meters */
					frng_buf->range_entries[success_cnt].range =
						res.avg_dist * 16;
				}
				memset(&frng_buf->range_entries[success_cnt].max_err, 0,
					sizeof(frng_buf->range_entries[success_cnt].max_err));
				rrm_ftm_get_meas_start(wlc, ftm, bsscfg, &session_inf,
					&frng_buf->range_entries[success_cnt].start_tsf);

				frng_buf->range_entries[success_cnt].rsvd = 0;
				frng_buf->range_entry_count++;
				success_cnt++;
				break;

			case WL_PROXD_E_REMOTE_FAIL:
			case WL_PROXD_E_REMOTE_CANCEL:
				frng_buf->error_entries[err_cnt].code =
					DOT11_FTM_RANGE_ERROR_AP_FAILED;
				break;
			case WL_PROXD_E_REMOTE_INCAPABLE:
				frng_buf->error_entries[err_cnt].code =
					DOT11_FTM_RANGE_ERROR_AP_INCAPABLE;
				break;
			default:
				frng_buf->error_entries[err_cnt].code =
					DOT11_FTM_RANGE_ERROR_TX_FAILED;
				break;
		}

		if (res.status != WL_PROXD_E_OK && res.status != WL_PROXD_E_INVALIDMEAS) {
			frng_buf->error_entries[err_cnt].start_tsf = session_inf.meas_start_lo;
			rrm_ftm_get_meas_start(wlc, ftm, bsscfg, &session_inf,
				&frng_buf->error_entries[err_cnt].start_tsf);

			memcpy(&frng_buf->error_entries[err_cnt].bssid,
				&frng_req->targets[i].bssid, ETHER_ADDR_LEN);
			frng_buf->error_entry_count++;
			err_cnt++;
		}

		WL_TMP(("%s: Results: sid %d, dist %u, status %d, state %d, num_ftm %d \n",
			__FUNCTION__, res.sid, res.avg_dist, res.status, res.state, res.num_ftm));
	}

done:
	rrm_info->rrm_state->scan_active = FALSE;

	/* continue rrm state machine */
	wl_add_timer(wlc->wl, rrm_info->rrm_timer, 0, 0);
}

static int
wlc_rrm_recv_frngreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg,
	dot11_rmreq_ftm_range_t *rmreq_ftmrange, wlc_rrm_req_t *req)
{
	wlc_info_t *wlc = rrm_info->wlc;
	int tlv_len, i, req_len;
	bcm_tlv_t *tlvs = NULL;
	dot11_neighbor_rep_ie_t *nbr_rep_ie;
	dot11_ftm_range_subel_t *frng_subel;
	rrm_nbr_rep_t *nbr_rep, *nbr_rep_next;
	rrm_nbr_rep_t *nbr_rep_head = NULL; /* Neighbor Report element */
	frngreq_t *frng_req;
	char eabuf[ETHER_ADDR_STR_LEN];

	BCM_REFERENCE(wlc); /* in case BCMDBG off */

	if (rmreq_ftmrange->mode & (DOT11_RMREQ_MODE_PARALLEL | DOT11_RMREQ_MODE_ENABLE)) {
		WL_ERROR(("wl%d: %s: Unsupported FTM range request mode 0x%x\n",
			wlc->pub->unit, __FUNCTION__, rmreq_ftmrange->mode));
		req->flags |= DOT11_RMREP_MODE_INCAPABLE;
		return BCME_UNSUPPORTED;
	}

	if ((rrm_info->rrm_frng_req = (rrm_ftmreq_t *)MALLOCZ(wlc->osh,
		sizeof(rrm_ftmreq_t))) == NULL) {
		WL_ERROR(("wl%d: %s: MALLOC(%d) failed, malloced %d bytes\n", WLCWLUNIT(wlc),
			__FUNCTION__, (int)sizeof(frngreq_t), MALLOCED(wlc->osh)));
		req->flags |= DOT11_RMREP_MODE_REFUSED;
		return BCME_NOMEM;
	}

	if ((rrm_info->rrm_frng_req->frng_req = (frngreq_t *)MALLOCZ(wlc->osh,
			(sizeof(frngreq_t) + ((DOT11_FTM_RANGE_ENTRY_MAX_COUNT-1) *
			sizeof(frngreq_target_t))))) == NULL) {
		WL_ERROR(("wl%d: %s: MALLOC(%d) failed, malloced %d bytes\n", WLCWLUNIT(wlc),
			__FUNCTION__, (int)sizeof(frngreq_t), MALLOCED(wlc->osh)));
		req->flags |= DOT11_RMREP_MODE_REFUSED;
		return BCME_NOMEM;
	}

	frng_req = rrm_info->rrm_frng_req->frng_req;
	rrm_info->rrm_frng_req->token = rmreq_ftmrange->token;
	frng_req->event = WL_RRM_EVENT_FRNG_REQ;
	frng_req->max_init_delay = load16_ua(&rmreq_ftmrange->max_init_delay);
	frng_req->min_ap_count = rmreq_ftmrange->min_ap_count;

	tlvs = (bcm_tlv_t *)&rmreq_ftmrange->data[0];
	tlv_len = rmreq_ftmrange->len - DOT11_MNG_IE_MREQ_FRNG_FIXED_LEN;

	while (tlvs) {
		if (tlvs->id == DOT11_MNG_NEIGHBOR_REP_ID) {
			nbr_rep_ie = (dot11_neighbor_rep_ie_t *)tlvs;

			if (nbr_rep_ie->len < DOT11_NEIGHBOR_REP_IE_FIXED_LEN) {
				WL_ERROR(("wl%d: %s: malformed Neighbor Report element with"
					" length %d\n", wlc->pub->unit, __FUNCTION__,
					nbr_rep_ie->len));

				tlvs = bcm_next_tlv(tlvs, &tlv_len);
				continue;
			}

			nbr_rep = (rrm_nbr_rep_t *)MALLOCZ(wlc->osh, sizeof(rrm_nbr_rep_t));
			if (nbr_rep == NULL) {
				WL_ERROR(("wl%d: %s: out of memory for neighbor report,"
					" malloced %d bytes\n", wlc->pub->unit, __FUNCTION__,
					MALLOCED(wlc->osh)));
					return BCME_NOMEM;
			}

			memcpy(&nbr_rep->nbr_elt.bssid, &nbr_rep_ie->bssid, ETHER_ADDR_LEN);
			nbr_rep->nbr_elt.bssid_info = load32_ua(&nbr_rep_ie->bssid_info);

			nbr_rep->nbr_elt.reg = nbr_rep_ie->reg;
			nbr_rep->nbr_elt.channel = nbr_rep_ie->channel;
			nbr_rep->nbr_elt.phytype = nbr_rep_ie->phytype;
			nbr_rep->nbr_elt.len = nbr_rep_ie->len;
			frng_req->num_aps++;

			/* Add the new nbr_rep to the head of the list */
			WLC_RRM_ELEMENT_LIST_ADD(nbr_rep_head, nbr_rep);

			bcm_ether_ntoa(&nbr_rep_ie->bssid, eabuf);
			WL_TMP((" bssid %s, bssid_info: 0x%08x, channel %d, reg %d, phytype %d\n",
				eabuf, ntoh32_ua(&nbr_rep_ie->bssid_info), nbr_rep_ie->channel,
				nbr_rep_ie->reg, nbr_rep_ie->phytype));

			if (nbr_rep_ie->len > DOT11_NEIGHBOR_REP_IE_FIXED_LEN) {
				bcm_tlv_t *mr_tlvs = (bcm_tlv_t *) &nbr_rep_ie->data[0];
				int mr_tlv_len = nbr_rep_ie->len - DOT11_NEIGHBOR_REP_IE_FIXED_LEN;

				if (mr_tlv_len < TLV_HDR_LEN) {
					WL_ERROR(("wl%d: %s: malformed variable data with"
						" total length %d\n", wlc->pub->unit, __FUNCTION__,
						mr_tlv_len));
					tlvs = bcm_next_tlv(tlvs, &tlv_len);
					continue;
				}

				wlc_rrm_parse_mr_tlvs(wlc, mr_tlvs, &mr_tlv_len, nbr_rep, cfg);
			}

		} else if (tlvs->id == DOT11_FTM_RANGE_SUBELEM_ID) {
			frng_subel = (dot11_ftm_range_subel_t *) tlvs;

			if (frng_subel->len < DOT11_FTM_RANGE_SUBELEM_LEN) {
				WL_ERROR(("wl%d: %s: malformed FTM Range Subelement with length"
					" %d\n", wlc->pub->unit,
					__FUNCTION__, frng_subel->len));

				tlvs = bcm_next_tlv(tlvs, &tlv_len);
				continue;
			}

			frng_req->max_age = load16_ua(&frng_subel->max_age);
		} else {
			WL_ERROR(("wl%d: %s: Received Unknown Element %d, len %d\n",
				wlc->pub->unit, __FUNCTION__, tlvs->id, tlvs->len));
		}

		/*
		Although the spec specifies a neighbor report optional subelement type,
		it's use is not specified, so will be ignored for now.
		*/

		tlvs = bcm_next_tlv(tlvs, &tlv_len);
	}

	/*
	* Have all the neighbors; now need to send to host app to request
	* ranges from the ranging code
	*/
	req_len = sizeof(*frng_req) + ((frng_req->num_aps-1) * sizeof(frngreq_target_t));
	nbr_rep = nbr_rep_head;
	i = 0;

	while (nbr_rep) {
		nbr_rep_next = nbr_rep->next;

		memcpy(&frng_req->targets[i].bssid, &nbr_rep->nbr_elt.bssid, ETHER_ADDR_LEN);
		frng_req->targets[i].bssid_info = nbr_rep->nbr_elt.bssid_info;
		frng_req->targets[i].reg = nbr_rep->nbr_elt.reg;
		frng_req->targets[i].channel = nbr_rep->nbr_elt.channel;
		frng_req->targets[i].chanspec = nbr_rep->nbr_elt.chanspec;
		frng_req->targets[i].phytype = nbr_rep->nbr_elt.phytype;

		if (nbr_rep->nbr_elt.chanspec == 0) {
			uint8 extch = wlc_rclass_extch_get(wlc->cmi, nbr_rep->nbr_elt.reg);
			frng_req->targets[i].chanspec = wlc_ht_chanspec(wlc,
				nbr_rep->nbr_elt.channel, extch, rrm_info->cur_cfg);
		}

		if (!wf_chspec_valid(frng_req->targets[i].chanspec)) {
			WL_ERROR(("wl%d:%s:invalid chanspec defaulting to 20MHz\n",
				wlc->pub->unit, __FUNCTION__));
			frng_req->targets[i].chanspec = CH20MHZ_CHSPEC(nbr_rep->nbr_elt.channel);
		}

		if (nbr_rep->opt_lci != NULL) {
			dot11_meas_rep_t *lci_rep = (dot11_meas_rep_t *) nbr_rep->opt_lci;
			MFREE(rrm_info->wlc->osh, nbr_rep->opt_lci, lci_rep->len + TLV_HDR_LEN);
		}

		if (nbr_rep->opt_civic != NULL) {
			dot11_meas_rep_t *civic_rep = (dot11_meas_rep_t *) nbr_rep->opt_civic;
			MFREE(rrm_info->wlc->osh, nbr_rep->opt_civic, civic_rep->len + TLV_HDR_LEN);
		}

		MFREE(rrm_info->wlc->osh, nbr_rep, sizeof(rrm_nbr_rep_t));
		nbr_rep = nbr_rep_next;
		i++;
	}

	wlc_bss_mac_event(wlc, cfg, WLC_E_RRM, &rrm_info->da,
		0, 0, 0, (void *)frng_req, req_len);

	return BCME_OK;
}

static int
wlc_rrm_begin_ftm(wlc_rrm_info_t *rrm_info)
{
	int err = BCME_OK;
	int i;
	wlc_ftm_ranging_ctx_t *rctx = NULL;
	frngreq_t *frng_req = NULL;
	wlc_info_t *wlc = rrm_info->wlc;
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	wl_proxd_tlv_t *req_tlvs = NULL;
	wl_proxd_tlv_t *req_tlvs_start = NULL;
	uint16 req_len_total, req_len;
	wl_proxd_ranging_info_t info;
	memset(&info, 0, sizeof(info));

	frng_req = rrm_info->rrm_frng_req->frng_req;
	if (frng_req == NULL) {
		WL_ERROR(("wl%d: %s: frng_req is NULL\n",
				wlc->pub->unit, __FUNCTION__));
		goto done;
	}

	if (rrm_info->cur_cfg == NULL) {
		WL_ERROR(("wl%d: %s: rrm_info->cur_cfg is NULL\n",
				wlc->pub->unit, __FUNCTION__));
		goto done;
	}

	/* set up ranging */
	/* create a ranging context */
	err = wlc_ftm_ranging_create(wlc_ftm_get_handle(wlc), wlc_rrm_ftm_ranging_cb,
		(void *) rrm_info, &rctx);
	if (err != BCME_OK || rctx == NULL) {
		WL_ERROR(("wl%d: %s: status %d, wlc_ftm_ranging_create failed\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	rrm_info->rrm_frng_req->rctx = rctx;

	err = wlc_ftm_ranging_set_flags(rctx,	WL_PROXD_RANGING_FLAG_DEL_SESSIONS_ON_STOP,
		WL_PROXD_RANGING_FLAG_DEL_SESSIONS_ON_STOP);
	if (err != BCME_OK) {
		WL_ERROR(("wl%d: %s: status %d, wlc_ftm_ranging_set_flags failed\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	err = wlc_ftm_ranging_set_sid_range(rctx, WL_PROXD_SID_RRM_START, WL_PROXD_SID_RRM_END);
	if (err != BCME_OK) {
		WL_ERROR(("wl%d: %s: status %d, wlc_ftm_ranging_set_sid_range failed\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	req_len = BCM_TLV_MAX_DATA_SIZE; /* fix */
	req_len_total = req_len;

	req_tlvs_start = MALLOCZ(wlc->osh, req_len);
	if (req_tlvs_start == NULL) {
		WL_ERROR(("wl%d: %s: MALLOC failed of %d bytes\n",
				wlc->pub->unit, __FUNCTION__, req_len));
		goto done;
	}

	for (i = 0; i < frng_req->num_aps; i++)
	{
		uint32 flags = 0;
		uint32 flags_mask = 0;
		uint32 chanspec;
		uint16 num_burst = 0;

		wl_proxd_session_id_t sid = WL_PROXD_SESSION_ID_GLOBAL;

		if (!(frng_req->targets[i].bssid_info & DOT11_NGBR_BI_FTM)) {
			/* skip if FTM not advertised by target */
			WL_ERROR(("wl%d: %s: skipping BSSID with FTM not set in BSSID info\n",
				wlc->pub->unit, __FUNCTION__));
			continue;
		}
		/* real sid will be returned */
		err = wlc_ftm_ranging_add_sid(rctx, rrm_info->cur_cfg, &sid);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d: %s: status %d, wlc_ftm_ranging_add_sid failed\n",
				wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}

		frng_req->targets[i].sid = sid;

		req_tlvs = req_tlvs_start;

		err = bcm_pack_xtlv_entry((uint8 **)&req_tlvs, &req_len,
			WL_PROXD_TLV_ID_PEER_MAC, sizeof(struct ether_addr),
			(uint8 *)&frng_req->targets[i].bssid, BCM_XTLV_OPTION_ALIGN32);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d: %s: status %d, bcm_pack_xtlv_entry failed\n",
				wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}

		/* set flags */
		flags_mask = WL_PROXD_SESSION_FLAG_INITIATOR | WL_PROXD_SESSION_FLAG_TARGET;
		flags_mask = htol32(flags_mask);
		err = bcm_pack_xtlv_entry((uint8 **)&req_tlvs, &req_len,
			WL_PROXD_TLV_ID_SESSION_FLAGS_MASK,
			sizeof(uint32), (uint8 *)&flags_mask, BCM_XTLV_OPTION_ALIGN32);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d: %s: status %d, bcm_pack_xtlv_entry failed\n",
				wlc->pub->unit, __FUNCTION__, req_len));
			goto done;
		}

		flags = WL_PROXD_SESSION_FLAG_INITIATOR;
		flags = htol32(flags);
		err = bcm_pack_xtlv_entry((uint8 **)&req_tlvs, &req_len,
			WL_PROXD_TLV_ID_SESSION_FLAGS,
			sizeof(uint32), (uint8 *)&flags, BCM_XTLV_OPTION_ALIGN32);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d: %s: status %d, bcm_pack_xtlv_entry failed\n",
				wlc->pub->unit, __FUNCTION__, req_len));
			goto done;
		}

		num_burst = frng_req->reps + 1;
		num_burst = htol16(num_burst);
		err = bcm_pack_xtlv_entry((uint8 **)&req_tlvs, &req_len, WL_PROXD_TLV_ID_NUM_BURST,
			sizeof(uint16), (uint8 *)&num_burst, BCM_XTLV_OPTION_ALIGN32);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d: %s: status %d, bcm_pack_xtlv_entry failed\n",
				wlc->pub->unit, __FUNCTION__, req_len));
			goto done;
		}

		chanspec = htol32(frng_req->targets[i].chanspec);
		err = bcm_pack_xtlv_entry((uint8 **)&req_tlvs, &req_len, WL_PROXD_TLV_ID_CHANSPEC,
			sizeof(uint32), (uint8 *)&chanspec, BCM_XTLV_OPTION_ALIGN32);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d: %s: status %d, bcm_pack_xtlv_entry failed\n",
				wlc->pub->unit, __FUNCTION__, req_len));
			goto done;
		}

		err = wlc_ftm_set_iov(wlc_ftm_get_handle(wlc), rrm_info->cur_cfg,
			WL_PROXD_CMD_CONFIG, sid, req_tlvs_start, req_len_total-req_len);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d: %s: status %d, wlc_ftm_set_iov failed\n",
				wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}

		memset(req_tlvs_start, 0, req_len_total);
	}

	/* FTM may do a scan, plus we need to wait for FTM to do stuff anyway */
	rrm_state->scan_active = TRUE;

	err = wlc_ftm_ranging_start(rctx);
	if (err != BCME_OK) {
		WL_ERROR(("wl%d: %s: status %d, wlc_ftm_ranging_start failed\n",
				wlc->pub->unit, err, __FUNCTION__));
	}

	err = wlc_ftm_ranging_get_info(rctx, &info);
	if (err != BCME_OK) {
		WL_ERROR(("wl%d: %s: status %d, wlc_ftm_ranging_get_info failed\n",
				wlc->pub->unit, err, __FUNCTION__));
	}

done:
	if (req_tlvs_start) {
		MFREE(wlc->osh, req_tlvs_start, req_len_total);
	}

	if (info.state < WL_PROXD_RANGING_STATE_INPROGRESS && rctx != NULL)	{
		/* ranging did not start so cancel */
		rrm_state->scan_active = FALSE;
		wlc_ftm_ranging_cancel(rctx);
	}

	return err;
}

static int
wlc_rrm_recv_frngrep(wlc_rrm_info_t *rrm_info, struct scb *scb,
	dot11_rmrep_ftm_range_t *rmrep_ftmrange, int len)
{
	wlc_info_t *wlc = rrm_info->wlc;
	int tlv_len;
	bcm_tlv_t *tlvs = NULL;
	uint8 *ptr;
	int i;
	dot11_ftm_range_entry_t *range_entry;
	dot11_ftm_range_error_entry_t *error_entry;
	frngrep_t frng_rep;
	char eabuf[ETHER_ADDR_STR_LEN];

	BCM_REFERENCE(wlc); /* in case BCMDBG off */

	if (rmrep_ftmrange->mode & (DOT11_RMREQ_MODE_PARALLEL | DOT11_RMREQ_MODE_ENABLE)) {
		WL_ERROR(("wl%d: %s: Unsupported FTM range report mode 0x%x\n",
		wlc->pub->unit, __FUNCTION__, rmrep_ftmrange->mode));
		return BCME_UNSUPPORTED;
	}

	frng_rep.event = WL_RRM_EVENT_FRNG_REP;
	frng_rep.range_entry_count = rmrep_ftmrange->entry_count;
	frng_rep.dialog_token = rmrep_ftmrange->token;

	if (frng_rep.range_entry_count > DOT11_FTM_RANGE_ENTRY_CNT_MAX) {
		WL_ERROR(("wl%d: %s: FTM Range Report entry_count %d is too high,"
			" malformed frame\n", wlc->pub->unit, __FUNCTION__,
			frng_rep.range_entry_count));
		return BCME_ERROR;
	}

	ptr = &rmrep_ftmrange->data[0];

	if (len < DOT11_FTM_RANGE_REP_MIN_LEN +
		(frng_rep.range_entry_count * sizeof(dot11_ftm_range_entry_t)) + 1) {
		WL_ERROR(("wl%d: %s: FTM Range Report len %d is too short, malformed frame\n",
			wlc->pub->unit, __FUNCTION__, len));
		return BCME_ERROR;
	}

	/*
	 * Put all the FTM range report elements in a list
	 */
	for (i = 0; i < frng_rep.range_entry_count; i++) {
		range_entry = (dot11_ftm_range_entry_t *) ptr;

		frng_rep.range_entries[i].start_tsf = load32_ua(&range_entry->start_tsf);
		memcpy(&frng_rep.range_entries[i].bssid, &range_entry->bssid, ETHER_ADDR_LEN);

		frng_rep.range_entries[i].range = (((range_entry->range[0] & 0xff) << 16) |
			((range_entry->range[1] & 0xff) << 8) | (range_entry->range[2] & 0xff));
		frng_rep.range_entries[i].max_err = range_entry->max_err[0];

		frng_rep.range_entries[i].rsvd = range_entry->rsvd;
		bcm_ether_ntoa(&range_entry->bssid, eabuf);
		ptr += sizeof(dot11_ftm_range_entry_t);
	}

	frng_rep.error_entry_count = *ptr++; /* error count just a single byte */

	if (frng_rep.error_entry_count > DOT11_FTM_RANGE_ERROR_CNT_MAX) {
		WL_ERROR(("wl%d: %s: FTM Range Report error_count %d is too high,"
			" malformed frame\n", wlc->pub->unit, __FUNCTION__,
			frng_rep.error_entry_count));
		return BCME_ERROR;
	}

	/*
	 * Put all the FTM range error report elements in a list
	 */
	for (i = 0; i < frng_rep.error_entry_count; i++) {
		error_entry = (dot11_ftm_range_error_entry_t *) ptr;

		frng_rep.error_entries[i].start_tsf = load32_ua(&error_entry->start_tsf);
		memcpy(&frng_rep.error_entries[i].bssid, &error_entry->bssid, ETHER_ADDR_LEN);
		frng_rep.error_entries[i].code = error_entry->code;

		ptr += sizeof(dot11_ftm_range_error_entry_t);
	}

	wlc_bss_mac_event(wlc, scb->bsscfg, WLC_E_RRM, &rrm_info->da,
		0, 0, 0, (void *)&frng_rep, sizeof(frng_rep));

	/*
	 * tlvs will be used for optional subelements, but no non-vendor-specific
	 * optional subelements are currently defined
	 */
	BCM_REFERENCE(tlvs);
	BCM_REFERENCE(tlv_len);

	return BCME_OK;
}

static int
wlc_rrm_create_frngrep_element(uint8 **ptr, rrm_bsscfg_cubby_t *rrm_cfg, frngrep_t *frngrep)
{
	dot11_ftm_range_entry_t *ftm_range;
	dot11_ftm_range_error_entry_t *ftm_error;
	int copy_len = 0;
	int i = 0;
	dot11_meas_rep_t *frng_rep = (dot11_meas_rep_t *) *ptr;

	if (frngrep == NULL ||
		(!frngrep->range_entry_count && !frngrep->error_entry_count)) {
		WL_ERROR(("%s: no range entry data\n",
				__FUNCTION__));
		return 0;
	}

	frng_rep->id = DOT11_MNG_MEASURE_REPORT_ID;
	frng_rep->type = DOT11_MEASURE_TYPE_FTMRANGE;
	frng_rep->mode = 0;
	frng_rep->token = (uint8)frngrep->dialog_token;
	frng_rep->len = DOT11_MNG_IE_MREP_FRNG_FIXED_LEN;

	frng_rep->rep.ftm_range.entry_count = frngrep->range_entry_count;

	*ptr += TLV_HDR_LEN + DOT11_MNG_IE_MREP_FRNG_FIXED_LEN;

	/* Range Entries */
	for (i = 0; i < frngrep->range_entry_count; i++) {
		ftm_range = (dot11_ftm_range_entry_t *) *ptr;

		store32_ua((uint8 *)&ftm_range->start_tsf, frngrep->range_entries[i].start_tsf);
		memcpy(&ftm_range->bssid, &frngrep->range_entries[i].bssid, ETHER_ADDR_LEN);
		ftm_range->range[0] = (frngrep->range_entries[i].range >> 16) & 0xFF;
		ftm_range->range[1] = (frngrep->range_entries[i].range >> 8) & 0xFF;
		ftm_range->range[2] = frngrep->range_entries[i].range & 0xFF;

		ftm_range->max_err[0] = frngrep->range_entries[i].max_err;
		ftm_range->rsvd = 0;
		*ptr += sizeof(dot11_ftm_range_entry_t);
	}

	copy_len += (sizeof(*ftm_range) * frngrep->range_entry_count);
	**ptr = frngrep->error_entry_count;
	*ptr += sizeof(frngrep->error_entry_count);
	copy_len++;

	/* Error entries */
	for (i = 0; i < frngrep->error_entry_count; i++) {
		ftm_error = (dot11_ftm_range_error_entry_t *) *ptr;
		store32_ua((uint8 *)&ftm_error->start_tsf, frngrep->error_entries[i].start_tsf);
		memcpy(&ftm_error->bssid, &frngrep->error_entries[i].bssid, ETHER_ADDR_LEN);
		ftm_error->code = frngrep->error_entries[i].code;
		*ptr += sizeof(dot11_ftm_range_error_entry_t);
	}

	copy_len += (sizeof(*ftm_error) * frngrep->error_entry_count);

	/* Currently no optional subelements for FTM Range report */
	frng_rep->len += copy_len;
	return copy_len;
}

static int
wlc_rrm_send_frngrep(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, uint8 *data, int len)
{
	wlc_info_t *wlc = rrm_info->wlc;
	dot11_rm_action_t *rm_rep;
	void *p = NULL;
	uint8 *pbody;
	uint buflen;
	rrm_bsscfg_cubby_t *rrm_cfg = NULL;
	struct ether_addr *da;
	int err = BCME_OK;
	frngrep_t *frngrep = (frngrep_t *)data;
	struct scb *scb;

	if (len > sizeof(frngrep_t)) {
		return BCME_BUFTOOLONG;
	}
	if (len < (OFFSETOF(frngrep_t, range_entries) + sizeof(frngrep_error_t))) {
		return BCME_BUFTOOSHORT;
	}

	if (frngrep->range_entry_count > DOT11_FTM_RANGE_ENTRY_CNT_MAX) {
		WL_ERROR(("wl%d: %s IOV_RRM_FRNG: REP: range_entry_count %d"
			" invalid\n", rrm_info->wlc->pub->unit, __FUNCTION__,
			frngrep->range_entry_count));
		return BCME_BADARG;
	}

	if (frngrep->error_entry_count > DOT11_FTM_RANGE_ERROR_CNT_MAX) {
		WL_ERROR(("wl%d: %s IOV_RRM_FRNG_REP: error_entry_count %d"
			" invalid\n", rrm_info->wlc->pub->unit, __FUNCTION__,
			frngrep->error_entry_count));
		return BCME_BADARG;
	}

	if (!frngrep->range_entry_count && !frngrep->error_entry_count) {
		WL_ERROR(("wl%d: %s IOV_RRM_FRNG_REP: missing entries, range %d, error %d",
			rrm_info->wlc->pub->unit, __FUNCTION__,
			frngrep->range_entry_count,
			frngrep->error_entry_count));
		return BCME_BADARG;
	}

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
	ASSERT(rrm_cfg != NULL);

	buflen = DOT11_RM_ACTION_LEN + DOT11_RM_IE_LEN + DOT11_FTM_RANGE_REP_FIXED_LEN;
	buflen += frngrep->range_entry_count * sizeof(dot11_ftm_range_entry_t);
	buflen += sizeof(frngrep->error_entry_count);
	buflen += frngrep->error_entry_count * sizeof(dot11_ftm_range_error_entry_t);

	da = &frngrep->da;
	p = wlc_frame_get_action(wlc, da, &cfg->cur_etheraddr, &cfg->BSSID,
		buflen, &pbody, DOT11_ACTION_CAT_RRM);

	if (p == NULL) {
		WL_ERROR(("%s: failed to get mgmt frame\n", __FUNCTION__));
		return BCME_NOMEM;
	}

	rm_rep = (dot11_rm_action_t *)pbody;
	rm_rep->category = DOT11_ACTION_CAT_RRM;
	rm_rep->action = DOT11_RM_ACTION_RM_REP;
	rm_rep->token = rrm_info->dialog_token;

	pbody = rm_rep->data;
	wlc_rrm_create_frngrep_element(&pbody, rrm_cfg, frngrep);

	scb = wlc_scbfind(wlc, cfg, da);
	wlc_sendmgmt(wlc, p, cfg->wlcif->qi, scb);

	return err;
}

static int
wlc_rrm_send_frngreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, uint8 *data, int len)
{
	void *p;
	uint8 *pbody, *ptr;
	uint8 *bufend;
	int buflen;
	dot11_rmreq_t *rmreq;
	dot11_rmreq_ftm_range_t *freq;
	struct ether_addr *da;
	frngreq_target_t *targets;
	int i;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
	int err = BCME_OK;
	frngreq_t *frngreq = (frngreq_t *)data;
	wlc_info_t *wlc = rrm_info->wlc;
	struct scb *scb;

	if (len > ((sizeof(frngreq_t) +
		((DOT11_FTM_RANGE_ENTRY_CNT_MAX-1) * sizeof(frngreq_target_t))))) {
		return BCME_BUFTOOLONG;
	}
	if (len < sizeof(frngreq_t)) {
		return BCME_BUFTOOSHORT;
	}

	if (frngreq->num_aps < frngreq->min_ap_count) {
		WL_ERROR(("wl%d: %s IOV_RRM_FRNG: REQ: num_aps %d <"
				" min_ap_count %d\n", rrm_info->wlc->pub->unit,
				__FUNCTION__, frngreq->num_aps, frngreq->min_ap_count));
		return BCME_BADARG;
	}

	da = &frngreq->da;

	/* rm frame action header + ftm range request header */
	buflen = DOT11_RMREQ_LEN + DOT11_RMREQ_FTM_RANGE_LEN;
	targets = (frngreq_target_t *) frngreq->targets;

	/* neighbor reports -  at least 1 is required */
	for (i = 0; i < frngreq->num_aps; i++) {
		buflen += (DOT11_NEIGHBOR_REP_IE_FIXED_LEN + TLV_HDR_LEN);
		if (wf_chspec_valid(targets[i].chanspec)) {
			buflen += (TLV_HDR_LEN + DOT11_WIDE_BW_IE_LEN);
		}
	}

	if (frngreq->max_age != 0) {
		buflen += (DOT11_FTM_RANGE_SUBELEM_LEN + TLV_HDR_LEN);
	}

	if ((p = wlc_frame_get_action(wlc, da,
		&cfg->cur_etheraddr, &cfg->BSSID, buflen, &pbody, DOT11_ACTION_CAT_RRM)) == NULL) {
		return BCME_NOMEM;
	}

	bufend = pbody + buflen;

	rmreq = (dot11_rmreq_t *)pbody;
	rmreq->category = DOT11_ACTION_CAT_RRM;
	rmreq->action = DOT11_RM_ACTION_RM_REQ;
	WLC_RRM_UPDATE_TOKEN(rrm_info->req_token);
	rmreq->token = rrm_info->req_token;
	rmreq->reps = frngreq->reps;

	freq = (dot11_rmreq_ftm_range_t *)&rmreq->data[0];
	freq->id = DOT11_MNG_MEASURE_REQUEST_ID;
	freq->len = buflen - DOT11_RMREQ_LEN - TLV_HDR_LEN;
	WLC_RRM_UPDATE_TOKEN(rrm_info->req_elt_token);
	freq->token = rrm_info->req_elt_token;
	freq->mode = 0;
	freq->type = DOT11_MEASURE_TYPE_FTMRANGE;

	store16_ua((uint8 *)&freq->max_init_delay, frngreq->max_init_delay);
	freq->min_ap_count = frngreq->min_ap_count;

	ptr = (uint8 *) freq->data;

	/* add neighbor reports -  at least 1 is required */
	for (i = 0; i < frngreq->num_aps; i++) {
		rrm_nbr_rep_t nbr_rep;

		memset(&nbr_rep, 0, sizeof(nbr_rep));
		memcpy(&nbr_rep.nbr_elt.bssid, &targets[i].bssid, sizeof(nbr_rep.nbr_elt.bssid));
		nbr_rep.nbr_elt.bssid_info = targets[i].bssid_info;
		nbr_rep.nbr_elt.channel = targets[i].channel;
		nbr_rep.nbr_elt.phytype = targets[i].phytype;
		nbr_rep.nbr_elt.reg = targets[i].reg;
		nbr_rep.nbr_elt.chanspec = targets[i].chanspec;
		nbr_rep.nbr_elt.id = DOT11_MNG_NEIGHBOR_REP_ID;

		wlc_rrm_create_nbrrep_element(rrm_cfg, &ptr, &nbr_rep, 0, bufend);
	}

	/* add optional subelements in non-decreasing order */
	if (frngreq->max_age != 0) {
		dot11_ftm_range_subel_t *age_range_subel = (dot11_ftm_range_subel_t *) ptr;

		age_range_subel->id = DOT11_FTM_RANGE_SUBELEM_ID;
		age_range_subel->len = DOT11_FTM_RANGE_SUBELEM_LEN;
		store16_ua((uint8 *)&age_range_subel->max_age, frngreq->max_age);
		ptr += sizeof(*age_range_subel);
	}

	if ((bufend - ptr) > 0) {
		WL_ERROR(("%s: fixup FTM range request response len buflen %d, actual %d\n",
			__FUNCTION__, buflen, (bufend - ptr)));
		buflen += DOT11_MGMT_HDR_LEN;
		PKTSETLEN(wlc->osh, p, (buflen - (bufend - ptr)));
	}

	scb = wlc_scbfind(wlc, cfg, da);
	wlc_sendmgmt(wlc, p, cfg->wlcif->qi, scb);

	return err;
}

static void wlc_rrm_frngreq_free(wlc_rrm_info_t *rrm_info)
{
	wlc_info_t *wlc = rrm_info->wlc;
	rrm_ftmreq_t *rrm_frng_req;

	rrm_frng_req = rrm_info->rrm_frng_req;
	if (rrm_frng_req) {
		if (rrm_frng_req->frng_rep) {
			MFREE(wlc->osh, rrm_frng_req->frng_rep, sizeof(frngrep_t));
			rrm_frng_req->frng_rep = NULL;
		}
		if (rrm_frng_req->frng_req) {
			MFREE(wlc->osh, rrm_frng_req->frng_req, sizeof(frngreq_t));
			rrm_frng_req->frng_req = NULL;
		}

		if (rrm_frng_req->rctx) {
			WL_TMP(("%s: Destroying range context\n", __FUNCTION__));
			wlc_ftm_ranging_cancel(rrm_frng_req->rctx);
			wlc_ftm_ranging_destroy(&rrm_frng_req->rctx);
		}

		MFREE(wlc->osh, rrm_frng_req, sizeof(rrm_ftmreq_t));
	}
	rrm_info->rrm_frng_req = NULL;
}
#endif /* WL_PROXDETECT && WL_FTM */
#endif /* WL_FTM_11K */

static void
wlc_rrm_flush_neighbors(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *bsscfg)
{
	rrm_nbr_rep_t *nbr_rep, *nbr_rep_next;
	reg_nbr_count_t *nbr_cnt, *nbr_cnt_next;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, bsscfg);

	if (rrm_cfg == NULL)
		return;

	nbr_rep = rrm_cfg->nbr_rep_head;
	while (nbr_rep) {
		nbr_rep_next = nbr_rep->next;

#ifdef WL_FTM_11K
		if (nbr_rep->opt_lci != NULL) {
			dot11_meas_rep_t *lci_rep = (dot11_meas_rep_t *) nbr_rep->opt_lci;
			MFREE(rrm_info->wlc->osh, nbr_rep->opt_lci, lci_rep->len + TLV_HDR_LEN);
		}
		if (nbr_rep->opt_civic != NULL) {
			dot11_meas_rep_t *civic_rep = (dot11_meas_rep_t *) nbr_rep->opt_civic;
			MFREE(rrm_info->wlc->osh, nbr_rep->opt_civic, civic_rep->len + TLV_HDR_LEN);
		}
#endif /* WL_FTM_11K */

		MFREE(rrm_info->wlc->osh, nbr_rep, sizeof(rrm_nbr_rep_t));
		nbr_rep = nbr_rep_next;
	}
	rrm_cfg->nbr_rep_head = NULL;

	nbr_cnt = rrm_cfg->nbr_cnt_head;
	while (nbr_cnt) {
		nbr_cnt_next = nbr_cnt->next;
		MFREE(rrm_info->wlc->osh, nbr_cnt, sizeof(reg_nbr_count_t));
		nbr_cnt = nbr_cnt_next;
	}
	rrm_cfg->nbr_cnt_head = NULL;
}

#ifdef BCMDBG
static void
wlc_rrm_dump_neighbors(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *bsscfg)
{
	rrm_nbr_rep_t *nbr_rep;
	reg_nbr_count_t *nbr_cnt;
	int count = 0;
	char eabuf[ETHER_ADDR_STR_LEN];
	int i;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, bsscfg);

	ASSERT(rrm_cfg != NULL);

	WL_ERROR(("\nRRM Neighbor Report:\n"));
	nbr_rep = rrm_cfg->nbr_rep_head;
	while (nbr_rep) {
		count++;

		WL_ERROR(("AP %2d: ", count));
		WL_ERROR(("bssid %s ", bcm_ether_ntoa(&nbr_rep->nbr_elt.bssid, eabuf)));
		WL_ERROR(("bssid_info %08x ", load32_ua(&nbr_rep->nbr_elt.bssid_info)));
		WL_ERROR(("reg %2d channel %3d phytype %d pref %3d\n", nbr_rep->nbr_elt.reg,
			nbr_rep->nbr_elt.channel, nbr_rep->nbr_elt.phytype,
			nbr_rep->nbr_elt.bss_trans_preference));
#ifdef WL_FTM_11K
		if (nbr_rep->opt_lci != NULL) {
			dot11_meas_rep_t *lci_rep = (dot11_meas_rep_t *) nbr_rep->opt_lci;
			WL_ERROR(("LCI meas report present, len %d, LCI subelement len %d\n",
				lci_rep->len, lci_rep->rep.lci.length));
		}
		if (nbr_rep->opt_civic != NULL) {
			dot11_meas_rep_t *civic_rep = (dot11_meas_rep_t *) nbr_rep->opt_civic;
			WL_ERROR(("Civic meas report present, len %d,"
				" Civic subelement type %d, len %d\n", civic_rep->len,
				civic_rep->rep.civic.type, civic_rep->rep.civic.length));
		}
#endif /* WL_FTM_11K */

		nbr_rep = nbr_rep->next;
	}

	WL_ERROR(("\nRRM AP Channel Report:\n"));
	nbr_cnt = rrm_cfg->nbr_cnt_head;
	while (nbr_cnt) {
		WL_ERROR(("regulatory %d channel ", nbr_cnt->reg));

		for (i = 0; i < MAXCHANNEL; i++) {
			if (nbr_cnt->nbr_count_by_channel[i] > 0)
				WL_ERROR(("%d ", i));
		}
		WL_ERROR(("\n"));

		nbr_cnt = nbr_cnt->next;
	}
}
#endif /* BCMDBG */

#ifdef STA
static int
wlc_rrm_regclass_neighbor_count(wlc_rrm_info_t *rrm_info, rrm_nbr_rep_t *nbr_rep,
	wlc_bsscfg_t *cfg)
{
	bool found;
	wlc_info_t *wlc;
	reg_nbr_count_t *nbr_cnt;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);

	ASSERT(rrm_cfg != NULL);

	if ((found = wlc_rrm_regclass_match(rrm_info, nbr_rep, cfg)))
		return BCME_OK;

	wlc = rrm_info->wlc;

	/* There is no corresponding reg_nbr_count_t for this nbr_rep; create a new one */
	nbr_cnt = (reg_nbr_count_t *)MALLOCZ(wlc->osh, sizeof(reg_nbr_count_t));
	if (nbr_cnt == NULL) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
			(int)sizeof(reg_nbr_count_t), MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}

	nbr_cnt->reg = nbr_rep->nbr_elt.reg;
	nbr_cnt->nbr_count_by_channel[nbr_rep->nbr_elt.channel] += 1;

	/* Add this reg_nbr_count_t to the list */
	WLC_RRM_ELEMENT_LIST_ADD(rrm_cfg->nbr_cnt_head, nbr_cnt);
	return BCME_OK;
}

static bool
wlc_rrm_regclass_match(wlc_rrm_info_t *rrm_info, rrm_nbr_rep_t *nbr_rep, wlc_bsscfg_t *cfg)
{
	reg_nbr_count_t *nbr_cnt;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);

	ASSERT(rrm_cfg != NULL);
	nbr_cnt = rrm_cfg->nbr_cnt_head;

	while (nbr_cnt) {
		if (nbr_cnt->reg == nbr_rep->nbr_elt.reg) {
			nbr_cnt->nbr_count_by_channel[nbr_rep->nbr_elt.channel] += 1;
			return TRUE;
		}
		nbr_cnt = nbr_cnt->next;
	}

	return FALSE;
}

/* Configure the roam channel cache manually, for instance if you have
 * .11k neighbor information.
 */
static int
wlc_set_roam_channel_cache(wlc_bsscfg_t *cfg, const chanspec_t *chanspecs, int num_chanspecs)
{
	int i, num_channel = 0;
	uint8 channel;
	wlc_roam_t *roam = cfg->roam;

	/* The new information replaces previous contents. */
	memset(roam->roam_chn_cache.vec, 0, sizeof(chanvec_t));

	WL_SRSCAN(("wl%d: ROAM: Adding channels to cache:", WLCWLUNIT(cfg->wlc)));
	WL_ASSOC(("wl%d: ROAM: Adding channels to cache:\n", WLCWLUNIT(cfg->wlc)));

	/* Neighbor list from AP may contains more channels than necessay:
	 * Take the first wlc->roam_chn_cache_limit distinctive channels.
	 * (prioritize 5G over 2G channels)
	 */
	for (i = 0; (i < num_chanspecs) && (num_channel < roam->roam_chn_cache_limit); i++) {
		channel = wf_chspec_ctlchan(chanspecs[i]);

		if (CHSPEC_IS5G(chanspecs[i]) && isclr(roam->roam_chn_cache.vec, channel)) {
			WL_SRSCAN(("%u", channel));
			WL_ASSOC(("%u\n", channel));
			setbit(roam->roam_chn_cache.vec, channel);
			num_channel++;
		}
	}
	for (i = 0; (i < num_chanspecs) && (num_channel < roam->roam_chn_cache_limit); i++) {
		channel = wf_chspec_ctlchan(chanspecs[i]);

		if (CHSPEC_IS2G(chanspecs[i]) && isclr(roam->roam_chn_cache.vec, channel)) {
			WL_SRSCAN(("%u", channel));
			WL_ASSOC(("%u\n", channel));
			setbit(roam->roam_chn_cache.vec, channel);
			num_channel++;
		}
	}
	WL_ASSOC(("\n"));

	/* Don't let subsequent roam scans modify the cache. */
	roam->roam_chn_cache_locked = TRUE;

	return BCME_OK;
}

static int
wlc_rrm_recv_nrrep(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, uint8 *body, int body_len)
{
	wlc_info_t *wlc = rrm_info->wlc;
	dot11_rm_action_t *rm_rep;
	bcm_tlv_t *tlvs;
	int tlv_len;
	dot11_neighbor_rep_ie_t *nbr_rep_ie;
	rrm_nbr_rep_t *nbr_rep;
	dot11_ngbr_bsstrans_pref_se_t *pref;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
	chanspec_t chanspec_list[MAXCHANNEL], ch;
	int channel_num = 0;

	ASSERT(rrm_cfg != NULL);

	if (body_len < DOT11_RM_ACTION_LEN) {
		WL_ERROR(("wl%d: %s: Received Neighbor Report frame with incorrect length %d\n",
			wlc->pub->unit, __FUNCTION__, body_len));
		return BCME_ERROR;
	}

	rm_rep = (dot11_rm_action_t *)body;
	WL_SRSCAN(("received neighbor report (token = %d)",
		rm_rep->token));
	WL_ASSOC(("received neighbor report (token = %d)\n",
		rm_rep->token));

	if (rrm_cfg->req_token != rm_rep->token) {
		WL_ERROR(("wl%d: %s: unmatched token (%d, %d) on Neighbor Report frame\n",
			wlc->pub->unit, __FUNCTION__, rrm_cfg->req_token, rm_rep->token));
		return BCME_ERROR;
	}

	wlc_rrm_flush_neighbors(rrm_info, cfg);

	tlvs = (bcm_tlv_t *)&rm_rep->data[0];

	tlv_len = body_len - DOT11_RM_ACTION_LEN;

	while (tlvs && tlvs->id == DOT11_MNG_NEIGHBOR_REP_ID) {
		nbr_rep_ie = (dot11_neighbor_rep_ie_t *)tlvs;

		if (nbr_rep_ie->len < DOT11_NEIGHBOR_REP_IE_FIXED_LEN) {
			WL_ERROR(("wl%d: %s: malformed Neighbor Report element with length %d\n",
				wlc->pub->unit, __FUNCTION__, nbr_rep_ie->len));

			tlvs = bcm_next_tlv(tlvs, &tlv_len);

			continue;
		}

		nbr_rep = (rrm_nbr_rep_t *)MALLOCZ(wlc->osh, sizeof(rrm_nbr_rep_t));
		if (nbr_rep == NULL) {
			WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
				(int)sizeof(rrm_nbr_rep_t), MALLOCED(wlc->osh)));
			return BCME_NOMEM;
		}

		memcpy(&nbr_rep->nbr_elt.bssid, &nbr_rep_ie->bssid, ETHER_ADDR_LEN);
		nbr_rep->nbr_elt.bssid_info = load32_ua(&nbr_rep_ie->bssid_info);
		nbr_rep->nbr_elt.reg = nbr_rep_ie->reg;
		nbr_rep->nbr_elt.channel = nbr_rep_ie->channel;
		nbr_rep->nbr_elt.phytype = nbr_rep_ie->phytype;

		pref = (dot11_ngbr_bsstrans_pref_se_t*) bcm_parse_tlvs(nbr_rep_ie->data,
			nbr_rep_ie->len - DOT11_NEIGHBOR_REP_IE_FIXED_LEN,
			DOT11_NGBR_BSSTRANS_PREF_SE_ID);
		if (pref) {
			nbr_rep->nbr_elt.bss_trans_preference = pref->preference;
		}
		/* Add the new nbr_rep to the head of the list */
		WLC_RRM_ELEMENT_LIST_ADD(rrm_cfg->nbr_rep_head, nbr_rep);

		/* AP Channel Report */
		wlc_rrm_regclass_neighbor_count(rrm_info, nbr_rep, cfg);

		ch = CH20MHZ_CHSPEC(nbr_rep_ie->channel);
		WL_SRSCAN(("  bssid %02x:%02x:%02x",
			nbr_rep_ie->bssid.octet[0], nbr_rep_ie->bssid.octet[1],
			nbr_rep_ie->bssid.octet[2]));
		WL_SRSCAN(("        %02x:%02x:%02x",
			nbr_rep_ie->bssid.octet[3], nbr_rep_ie->bssid.octet[4],
			nbr_rep_ie->bssid.octet[5]));
		WL_SRSCAN((
			"    bssinfo: 0x%x: reg %d: channel %d: phytype %d",
			ntoh32_ua(&nbr_rep_ie->bssid_info), nbr_rep_ie->reg,
			nbr_rep_ie->channel, nbr_rep_ie->phytype));
		WL_ASSOC(("  bssid %02x:%02x:%02x\n",
			nbr_rep_ie->bssid.octet[0], nbr_rep_ie->bssid.octet[1],
			nbr_rep_ie->bssid.octet[2]));
		WL_ASSOC(("        %02x:%02x:%02x\n",
			nbr_rep_ie->bssid.octet[3], nbr_rep_ie->bssid.octet[4],
			nbr_rep_ie->bssid.octet[5]));
		WL_ASSOC((
			"    bssinfo: 0x%x: reg %d: channel %d: phytype %d\n",
			ntoh32_ua(&nbr_rep_ie->bssid_info), nbr_rep_ie->reg,
			nbr_rep_ie->channel, nbr_rep_ie->phytype));

		/* Prepare the hot channel list with 11k information */
		if ((channel_num < MAXCHANNEL) && wlc_valid_chanspec_db(wlc->cmi, ch))
			chanspec_list[channel_num++] = ch;

		tlvs = bcm_next_tlv(tlvs, &tlv_len);
	}

	/* Push the list for consumption */
	(void) wlc_set_roam_channel_cache(cfg, chanspec_list, channel_num);

#ifdef BCMDBG
	wlc_rrm_dump_neighbors(rrm_info, cfg);
#endif /* BCMDBG */
	return BCME_OK;
}
#endif /* STA */

static int
wlc_rrm_doiovar(void *hdl, uint32 actionid,
        void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	wlc_rrm_info_t *rrm_info = hdl;
	wlc_info_t * wlc = rrm_info->wlc;
	rrm_bsscfg_cubby_t *rrm_cfg;
	wlc_bsscfg_t *cfg;
	int err = BCME_OK;

	cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(cfg != NULL);

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);

	switch (actionid) {
	case IOV_SVAL(IOV_RRM):
	{
		wlc_bss_info_t *current_bss;
		dot11_rrm_cap_ie_t *rrm_cap = (dot11_rrm_cap_ie_t *)a;

		current_bss = cfg->current_bss;

		bcopy(rrm_cap->cap, rrm_cfg->rrm_cap, DOT11_RRM_CAP_LEN);

		if (!wlc_rrm_enabled(rrm_info, cfg)) {
			wlc_bsscfg_t *bsscfg;
			int idx;
			bool _rrm = FALSE;

			current_bss->capability &= ~DOT11_CAP_RRM;

			FOREACH_BSS(wlc, idx, bsscfg) {
				if (bsscfg->current_bss->capability & DOT11_CAP_RRM) {
					_rrm = TRUE;
					break;
				}
			}
			wlc->pub->_rrm = _rrm;

			wlc_rrm_flush_neighbors(rrm_info, cfg);
		} else {
			current_bss->capability |= DOT11_CAP_RRM;
			wlc->pub->_rrm = TRUE;
		}

		if (cfg->up &&
		    (BSSCFG_AP(cfg) || (!cfg->BSS && !BSS_TDLS_ENAB(wlc, cfg)))) {
			/* Update AP or IBSS beacons */
			wlc_bss_update_beacon(wlc, cfg);
			/* Update AP or IBSS probe responses */
			wlc_bss_update_probe_resp(wlc, cfg, TRUE);
		}

		break;
	}
	case IOV_GVAL(IOV_RRM):
	{
		dot11_rrm_cap_ie_t *rrm_cap = (dot11_rrm_cap_ie_t *)a;

		bcopy(rrm_cfg->rrm_cap, rrm_cap->cap, DOT11_RRM_CAP_LEN);

		break;
	}
	/* IOVAR SET neighbor request */
	case IOV_SVAL(IOV_RRM_NBR_REQ):
	{
#ifdef STA
		wlc_ssid_t ssid;

		if (!cfg->associated)
			return BCME_NOTASSOCIATED;

		if (alen == sizeof(wlc_ssid_t)) {
			bcopy(a, &ssid, sizeof(wlc_ssid_t));
			wlc_rrm_send_nbrreq(rrm_info, cfg, &ssid);
		}
		else {
			wlc_rrm_send_nbrreq(rrm_info, cfg, NULL);
		}
#endif /* STA */
		break;
	}
	/* IOVAR SET beacon request */
	case IOV_SVAL(IOV_RRM_BCN_REQ):
	{
		char eabuf[ETHER_ADDR_STR_LEN];
		bcn_req_t *bcnreq = NULL;
		bcnreq_t bcnreq_v0;

		if (!cfg->associated)
			return BCME_NOTASSOCIATED;

		if (alen >= sizeof(bcn_req_t)) {
			bcnreq = (bcn_req_t *)MALLOCZ(wlc->osh, alen);
			if (bcnreq == NULL) {
				WL_ERROR(("%s: out of memory, malloced %d bytes\n",
					__FUNCTION__, MALLOCED(wlc->osh)));
				err = BCME_ERROR;
				break;
			}
			memcpy(bcnreq, a, alen);
		}
		else if (alen == sizeof(bcnreq_t)) {
			bcnreq = (bcn_req_t *)MALLOCZ(wlc->osh, alen);
			if (bcnreq == NULL) {
				WL_ERROR(("%s: out of memory, malloced %d bytes\n",
					__FUNCTION__, MALLOCED(wlc->osh)));
				err = BCME_ERROR;
				break;
			}
			/* convert from old struct */
			memcpy(&bcnreq_v0, a, sizeof(bcnreq_t));
			bcnreq->bcn_mode = bcnreq_v0.bcn_mode;
			bcnreq->dur = bcnreq_v0.dur;
			bcnreq->channel = bcnreq_v0.channel;
			memcpy(&bcnreq->da, &bcnreq_v0.da, sizeof(struct ether_addr));
			memcpy(&bcnreq->ssid, &bcnreq_v0.ssid, sizeof(wlc_ssid_t));
			bcnreq->reps = bcnreq_v0.reps;
			bcnreq->version = WL_RRM_BCN_REQ_VER;
		}
		else {
			WL_ERROR(("wl%d: %s IOV_RRM_BCN_REQ: len of req: %d not match\n",
				rrm_info->wlc->pub->unit, __FUNCTION__, alen));
			err = BCME_ERROR;
			break;
		}
		if (bcnreq->version == WL_RRM_BCN_REQ_VER) {
			if (bcnreq->bcn_mode == DOT11_RMREQ_BCN_PASSIVE) {
				if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_PASSIVE))
					err =  BCME_UNSUPPORTED;
			} else if (bcnreq->bcn_mode == DOT11_RMREQ_BCN_ACTIVE) {
				if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_ACTIVE))
					err =  BCME_UNSUPPORTED;
			} else if (bcnreq->bcn_mode == DOT11_RMREQ_BCN_TABLE) {
				if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_TABLE))
					err =  BCME_UNSUPPORTED;
			} else {
				err = BCME_ERROR;
			}
			if (err == BCME_OK)
			{
				wlc_rrm_send_bcnreq(rrm_info, cfg, bcnreq);
				bcm_ether_ntoa(&bcnreq->da, eabuf);
				WL_ERROR(("IOV_RRM_BCN_REQ: da: %s, mode: %d, dur: %d, chan: %d\n",
				eabuf, bcnreq->bcn_mode, bcnreq->dur,
				bcnreq->channel));
			}
		}
		else {
			WL_ERROR(("wl%d: %s IOV_RRM_BCN_REQ: len of req: %d not match\n",
				rrm_info->wlc->pub->unit, __FUNCTION__, alen));
		}
		if (bcnreq)
			MFREE(wlc->osh, bcnreq, sizeof(*bcnreq));
		break;
	}

	/* IOVAR SET channel load request */
	case IOV_SVAL(IOV_RRM_CHLOAD_REQ):
	{
		rrmreq_t chloadreq;

		if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_CLM))
			return BCME_UNSUPPORTED;

		bcopy(a, &chloadreq, sizeof(rrmreq_t));
		wlc_rrm_send_chloadreq(rrm_info, cfg, &chloadreq);
		break;
	}

	/* IOVAR SET noise request */
	case IOV_SVAL(IOV_RRM_NOISE_REQ):
	{
		rrmreq_t noisereq;

		if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_NHM))
			return BCME_UNSUPPORTED;

		bcopy(a, &noisereq, sizeof(rrmreq_t));
		wlc_rrm_send_noisereq(rrm_info, cfg, &noisereq);
		break;
	}

	/* IOVAR SET frame request */
	case IOV_SVAL(IOV_RRM_FRAME_REQ):
	{
		framereq_t framereq;

		if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_FM))
			return BCME_UNSUPPORTED;

		bcopy(a, &framereq, sizeof(framereq_t));
		wlc_rrm_send_framereq(rrm_info, cfg, &framereq);
		break;
	}

	/* IOVAR SET statistics request */
	case IOV_SVAL(IOV_RRM_STAT_REQ):
	{
		statreq_t statreq;

		if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_SM))
			return BCME_UNSUPPORTED;

		bcopy(a, &statreq, sizeof(statreq_t));
		wlc_rrm_send_statreq(rrm_info, cfg, &statreq);
		break;
	}

	/* IOVAR GET statistics report */
	case IOV_GVAL(IOV_RRM_STAT_RPT):
	{
		statrpt_t *param, *rpt;
		struct scb *scb;
		rrm_scb_info_t *scb_info;

		if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_SM))
			return BCME_UNSUPPORTED;

		if (alen < sizeof(statrpt_t)) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		rpt = (statrpt_t*)a;
		param = (statrpt_t*)p;

		if ((param == NULL) || ETHER_ISMULTI(&param->addr)) {
			err = BCME_BADARG;
			break;
		}

		if (param->ver != WL_RRM_RPT_VER) {
			WL_ERROR(("ver[%d] mismatch[%d]\n", param->ver, WL_RRM_RPT_VER));
			err = BCME_VERSION;
			break;
		}

		if ((scb = wlc_scbfind(wlc, cfg, &param->addr)) == NULL) {
			WL_ERROR(("wl%d: %s: non-associated station"MACF"\n",
				wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(param->addr)));
			err = BCME_NOTASSOCIATED;
			break;
		}
		scb_info = RRM_SCB_INFO(rrm_info, scb);

		if (!scb_info) {
			WL_ERROR(("wl%d: %s: "MACF" scb_info is NULL\n",
				wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(param->addr)));
			err = BCME_ERROR;
			break;
		}

		rpt->ver = WL_RRM_RPT_VER;
		rpt->flag = scb_info->flag;
		rpt->len = scb_info->len;
		rpt->timestamp = scb_info->timestamp;
		memcpy(rpt->data, scb_info->data, rpt->len);

		WL_INFORM(("ver:%d timestamp:%u flag:%d len:%d\n",
			rpt->ver, rpt->timestamp, rpt->flag, rpt->len));

		break;
	}

	/* IOVAR SET link measurement request */
	case IOV_SVAL(IOV_RRM_LM_REQ):
	{
		struct ether_addr da;
		char eabuf[ETHER_ADDR_STR_LEN];

		if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LINK))
			return BCME_UNSUPPORTED;

		if (alen == sizeof(struct ether_addr)) {
			bcopy(a, &da, sizeof(struct ether_addr));
			wlc_rrm_send_lmreq(rrm_info, cfg, &da);

			bcm_ether_ntoa(&da, eabuf);
		}
		else {
			WL_ERROR(("wl%d: %s IOV_RRM_LM_REQ: len of req: %d not match\n",
				rrm_info->wlc->pub->unit, __FUNCTION__, alen));
		}
		break;
	}

	case IOV_GVAL(IOV_RRM_NBR_LIST):
	{
#ifdef WL11K_AP
		uint16 list_cnt;
#endif /* WL11K_AP */

		if (BSSCFG_STA(cfg))
			return BCME_UNSUPPORTED;

#ifdef WL11K_AP
		if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_NEIGHBOR_REPORT))
			return BCME_UNSUPPORTED;

		if (plen == 0) {
			list_cnt = wlc_rrm_get_neighbor_count(rrm_info, rrm_cfg);
			store16_ua((uint8 *)a, list_cnt);
			break;
		}

		list_cnt = load16_ua((uint8 *)p);
		wlc_rrm_get_neighbor_list(rrm_info, a, list_cnt, rrm_cfg);
#endif /* WL11K_AP */
		break;
	}

#ifdef WL11K_AP
	case IOV_SVAL(IOV_RRM_NBR_DEL_NBR):
	{
		struct ether_addr ea;

		if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_NEIGHBOR_REPORT))
			return BCME_UNSUPPORTED;

		if (alen == sizeof(struct ether_addr)) {
			/* Flush all neighbors if input address is broadcast */
			if (ETHER_ISBCAST(a)) {
				wlc_rrm_flush_neighbors(rrm_info, cfg);
				break;
			}
			bcopy(a, &ea, sizeof(struct ether_addr));
			wlc_rrm_del_neighbor(rrm_info, &ea, wlcif, rrm_cfg);
		}
		else {
			WL_ERROR(("wl%d: %s IOV_RRM_NBR_DEL_NBR: len of req: %d not match\n",
				rrm_info->wlc->pub->unit, __FUNCTION__, alen));
		}

		break;
	}

	case IOV_SVAL(IOV_RRM_NBR_ADD_NBR):
	{
		nbr_rpt_elem_t nbr_elt;
		memset(&nbr_elt, 0, sizeof(nbr_rpt_elem_t));

		if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_NEIGHBOR_REPORT))
			return BCME_UNSUPPORTED;

		if (alen == TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN) {
			memcpy(((uint8 *)&nbr_elt + OFFSETOF(nbr_rpt_elem_t, id)), a,
				TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN);
			nbr_elt.version = WL_RRM_NBR_RPT_VER;
		}
		else if ((alen > TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN) &&
			(alen <= sizeof(nbr_elt))) {
			memcpy(&nbr_elt, a, alen);
		}
		if (nbr_elt.version == WL_RRM_NBR_RPT_VER) {
			wlc_rrm_add_neighbor(rrm_info, &nbr_elt, wlcif, rrm_cfg);
		}
		else {
			WL_ERROR(("wl%d: %s IOV_RRM_NBR_ADD_NBR: len of req: %d not match\n",
				rrm_info->wlc->pub->unit, __FUNCTION__, alen));
		}
		break;
	}
#endif /* WL11K_AP */

#ifdef WL_FTM_11K
#if defined(WL_PROXDETECT) && defined(WL_FTM)
	case IOV_SVAL(IOV_RRM_FRNG):
	{
		if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_FTM_RANGE))
			return BCME_UNSUPPORTED;

		err = rrm_frng_set(rrm_info, cfg, p, plen, a, alen);
		break;
	}
#endif /* WL_PROXDETECT && WL_FTM */
	case IOV_SVAL(IOV_RRM_CONFIG):
	{
		err = rrm_config_set(rrm_info, cfg, p, plen, a, alen);
		break;
	}

	case IOV_GVAL(IOV_RRM_CONFIG):
	{
		err = rrm_config_get(rrm_info, cfg, p, plen, a, alen);
		break;
	}
#endif /* WL_FTM_11K */

	case IOV_SVAL(IOV_RRM_BCN_REQ_THRTL_WIN):
	{
		if (p && (plen >= (int)sizeof(rrm_info->bcn_req_thrtl_win))) {
			rrm_info->bcn_req_thrtl_win = *(uint32 *)p;
		}

		/* Sanity check: throttle window must always be > off chan time */
		if (rrm_info->bcn_req_thrtl_win <
			rrm_info->bcn_req_off_chan_time_allowed/WLC_RRM_MS_IN_SEC) {
			rrm_info->bcn_req_thrtl_win = 0;
			rrm_info->bcn_req_off_chan_time_allowed = 0;
		}
		break;
	}
	case IOV_GVAL(IOV_RRM_BCN_REQ_THRTL_WIN):
	{
		if (a && (alen >= (int)sizeof(rrm_info->bcn_req_thrtl_win))) {
			*((uint32 *)a) = rrm_info->bcn_req_thrtl_win;
		}
		break;
	}
	case IOV_SVAL(IOV_RRM_BCN_REQ_MAX_OFF_CHAN_TIME):
	{
		if (p && (plen >= (int)sizeof(rrm_info->bcn_req_off_chan_time_allowed))) {
			rrm_info->bcn_req_off_chan_time_allowed = *(uint32 *)p * WLC_RRM_MS_IN_SEC;
		}

		/* Sanity check: throttle window must always be > off chan time */
		if (rrm_info->bcn_req_thrtl_win <
			rrm_info->bcn_req_off_chan_time_allowed/WLC_RRM_MS_IN_SEC) {
			rrm_info->bcn_req_thrtl_win = 0;
			rrm_info->bcn_req_off_chan_time_allowed = 0;
		}
		break;
	}
	case IOV_GVAL(IOV_RRM_BCN_REQ_MAX_OFF_CHAN_TIME):
	{
		if (a && (alen >= (int)sizeof(rrm_info->bcn_req_off_chan_time_allowed))) {
			*((uint32 *)a) = rrm_info->bcn_req_off_chan_time_allowed/WLC_RRM_MS_IN_SEC;
		}
		break;
	}
	case IOV_SVAL(IOV_RRM_BCN_REQ_TRAFF_MEAS_PER):
	{
		if (p && (plen >= (int)sizeof(rrm_info->bcn_req_traff_meas_prd))) {
			rrm_info->bcn_req_traff_meas_prd = *((uint32 *)p);
		}
		break;
	}
	case IOV_GVAL(IOV_RRM_BCN_REQ_TRAFF_MEAS_PER):
	{
		if (a && (alen >= (int)sizeof(rrm_info->bcn_req_traff_meas_prd))) {
			*((uint32 *)a) = rrm_info->bcn_req_traff_meas_prd;
		}
		break;
	}
	default:
	{
		err = BCME_UNSUPPORTED;
		break;
	}
	}

	return (err);
}

static void
wlc_rrm_recv_lmreq(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, struct scb *scb, uint8 *body,
	int body_len, int8 rssi, ratespec_t rspec)
{
	wlc_info_t *wlc = rrm_info->wlc;
	dot11_lmreq_t *lmreq;
	dot11_lmrep_t *lmrep;
	uint8 *pbody;
	void *p;
	rrm_bsscfg_cubby_t *rrm_cfg;

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
	ASSERT(rrm_cfg != NULL);

	if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LINK)) {
		WL_ERROR(("wl%d: %s: Unsupported\n", wlc->pub->unit,
		          __FUNCTION__));
		return;
	}
	lmreq = (dot11_lmreq_t *)body;

	if (body_len < DOT11_LMREQ_LEN) {
		WL_ERROR(("wl%d: %s invalid body len %d in rmreq\n", rrm_info->wlc->pub->unit,
		          __FUNCTION__, body_len));
		return;
	}

	p = wlc_frame_get_action(wlc, &scb->ea,
	                       &cfg->cur_etheraddr, &cfg->BSSID,
	                       DOT11_LMREP_LEN, &pbody, DOT11_ACTION_CAT_RRM);
	if (p == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return;
	}

	lmrep = (dot11_lmrep_t *)pbody;
	lmrep->category = DOT11_ACTION_CAT_RRM;
	lmrep->action = DOT11_RM_ACTION_LM_REP;
	lmrep->token = lmreq->token;
	lmrep->rxant = 0;
	lmrep->txant = 0;
	lmrep->rsni = 0;
	lmrep->rcpi = (uint8)rssi;

	/* tpc report element */
	wlc_tpc_rep_build(wlc, rssi, rspec, &lmrep->tpc);

	wlc_sendmgmt(wlc, p, cfg->wlcif->qi, scb);
}

static void
wlc_rrm_recv_lmrep(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, uint8 *body, int body_len)
{
	rrm_bsscfg_cubby_t *rrm_cfg;

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
	ASSERT(rrm_cfg != NULL);

	if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LINK)) {
		WL_ERROR(("wl%d: %s: Unsupported\n", rrm_info->wlc->pub->unit,
		          __FUNCTION__));
		return;
	}

	if (body_len < DOT11_LMREP_LEN) {
		WL_ERROR(("wl%d: %s invalid body len %d in rmrep\n", rrm_info->wlc->pub->unit,
		          __FUNCTION__, body_len));
		return;
	}

	return;
}

static chanspec_t
wlc_rrm_chanspec(wlc_rrm_info_t *rrm_info)
{
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	wlc_rrm_req_t *req;
	int req_idx;
	chanspec_t chanspec = 0;

	for (req_idx = rrm_state->cur_req; req_idx < rrm_state->req_count; req_idx++) {
		req = &rrm_state->req[req_idx];

		/* check for end of parallel measurements */
		if (req_idx != rrm_state->cur_req &&
		    (req->flags & DOT11_RMREQ_MODE_PARALLEL) == 0) {
			break;
		}

		/* check for request that have already been marked to skip */
		if (req->flags & (DOT11_RMREP_MODE_INCAPABLE| DOT11_RMREP_MODE_REFUSED))
			continue;

		if (req->dur > 0) {
			/* found the first non-zero dur request that will not be skipped */
			chanspec = req->chanspec;
			break;
		}
	}

	return chanspec;
}

static void wlc_rrm_bcnreq_free(wlc_rrm_info_t *rrm_info)
{
	wlc_info_t *wlc = rrm_info->wlc;
	rrm_bcnreq_t *bcn_req;

	bcn_req = rrm_info->bcnreq;
	if (bcn_req) {
		wlc_bss_list_free(wlc, &bcn_req->scan_results);
		MFREE(wlc->osh, bcn_req, sizeof(rrm_bcnreq_t));
	}
	rrm_info->bcnreq = NULL;
}

static void
wlc_rrm_free(wlc_rrm_info_t *rrm_info)
{
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	wlc_info_t *wlc = rrm_info->wlc;

	WL_INFORM(("%s: free memory\n", __FUNCTION__));

	if (rrm_state->req)
		MFREE(wlc->osh, rrm_state->req, rrm_state->req_count * sizeof(wlc_rrm_req_t));

	wl_del_timer(wlc->wl, rrm_info->rrm_timer);
	wlc_rrm_state_upd(rrm_info, WLC_RRM_IDLE);
	WL_INFORM(("%s: state upd to %d\n", __FUNCTION__, rrm_state->step));

	rrm_state->scan_active = FALSE;
	wlc_rrm_bcnreq_free(rrm_info);
#ifdef WL_FTM_11K
#if defined(WL_PROXDETECT) && defined(WL_FTM)
	if (PROXD_ENAB(wlc->pub)) {
		wlc_rrm_frngreq_free(rrm_info);
	}
#endif /* WL_PROXDETECT && WL_FTM */
#endif /* WL_FTM_11K */
	/* update ps control */
	wlc_set_wake_ctrl(wlc);
	rrm_state->chanspec_return = 0;
	rrm_state->cur_req = 0;
	rrm_state->req_count = 0;
	rrm_state->req = NULL;
	rrm_state->reps = 0;
	rrm_info->dialog_token = 0;
	return;
}

void
wlc_rrm_terminate(wlc_rrm_info_t *rrm_info)
{
	wlc_info_t *wlc = rrm_info->wlc;
#ifdef STA
	int idx;
	wlc_bsscfg_t *cfg;
#endif /* STA */
	/* enable CFP & TSF update */
	wlc_mhf(wlc, MHF2, MHF2_SKIP_CFP_UPDATE, 0, WLC_BAND_ALL);
	wlc_skip_adjtsf(wlc, FALSE, NULL, WLC_SKIP_ADJTSF_RM, WLC_BAND_ALL);

#ifdef STA
	/* come out of PS mode if appropriate */
	FOREACH_BSS(wlc, idx, cfg) {
		if (!BSSCFG_STA(cfg))
			continue;
		/* un-block PSPoll operations and restore PS state */
		mboolclr(cfg->pm->PMblocked, WLC_PM_BLOCK_CHANSW);
		if (cfg->pm->PM != PM_MAX || cfg->pm->WME_PM_blocked) {
			WL_RTDC(wlc, "wlc_rrm_meas_end: exit PS", 0, 0);
			wlc_set_pmstate(cfg, FALSE);
			wlc_pm2_sleep_ret_timer_start(cfg, 0);
		}
	}
#endif /* STA */
	wlc_set_wake_ctrl(wlc);
}

static int
wlc_rrm_abort(wlc_info_t *wlc)
{
	wlc_rrm_info_t *rrm_info = wlc->rrm_info;
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	bool canceled = FALSE;

	if (rrm_state->step == WLC_RRM_IDLE) {
		return TRUE;
	}
	if (rrm_state->step == WLC_RRM_ABORT) {
		/* timer has been canceled, but not fired yet */
		return FALSE;
	}

	wlc_rrm_state_upd(rrm_info, WLC_RRM_ABORT);
	WL_ERROR(("%s: state upd to %d\n", __FUNCTION__, rrm_state->step));

	/* cancel any timers and clear state */
	if (wl_del_timer(wlc->wl, rrm_info->rrm_timer)) {
		wlc_rrm_state_upd(rrm_info, WLC_RRM_IDLE);
		WL_ERROR(("%s: state upd to %d\n", __FUNCTION__, rrm_state->step));
		canceled = TRUE;
	}

	/* Change the radio channel to the return channel */
	if ((rrm_state->chanspec_return != 0) &&
	(WLC_BAND_PI_RADIO_CHANSPEC != rrm_state->chanspec_return)) {
		wlc_suspend_mac_and_wait(wlc);
		WL_ERROR(("%s: return to the home channel\n", __FUNCTION__));
		wlc_set_chanspec(wlc, rrm_state->chanspec_return, CHANSW_IOVAR);
		wlc_enable_mac(wlc);
	}

	wlc_rrm_terminate(wlc->rrm_info);

	/* un-suspend the data fifos in case they were suspended
	 * for off channel measurements
	 */
	wlc_tx_resume(wlc);

	wlc_rrm_free(rrm_info);

	return canceled;
}

static void
wlc_rrm_report_11k(wlc_rrm_info_t *rrm_info, wlc_rrm_req_t *req_block, int count)
{
	int i, status;
	bool measured;
	wlc_rrm_req_t *req;
	rrm_bcnreq_t *bcn_req;
	rrm_bsscfg_cubby_t *rrm_cfg;
#ifdef WL_FTM_11K
#if defined(WL_PROXDETECT) && defined(WL_FTM)
	frngrep_t *frng_rep;
#endif
#endif /* WL_FTM_11K */

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, rrm_info->cur_cfg);
	ASSERT(rrm_cfg != NULL);

	for (i = 0; i < count; i++) {
		req = req_block + i;

		if ((req->flags & (DOT11_RMREP_MODE_INCAPABLE |
			DOT11_RMREP_MODE_REFUSED))) {
			measured = FALSE;
		} else {
			measured = TRUE;
		}

		switch (req->type) {
		case DOT11_RMREQ_BCN_ACTIVE:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_ACTIVE))
				break;
		case DOT11_RMREQ_BCN_PASSIVE:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_PASSIVE))
				break;
			if (req->flags & DOT11_RMREP_MODE_INCAPABLE) {
				wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_BEACON,
					req->token, DOT11_RMREP_MODE_INCAPABLE);
				break;
			}
			if (req->flags & DOT11_RMREP_MODE_REFUSED) {
				wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_BEACON,
					req->token, DOT11_RMREP_MODE_REFUSED);
				break;
			}
			bcn_req = rrm_info->bcnreq;
			if (bcn_req == NULL)
				break;
			status = bcn_req->scan_status;

			if (status == WLC_E_STATUS_ERROR) {
				WL_ERROR(("%s: scan status = error\n", __FUNCTION__));
				break;
			}

			if (status != WLC_E_STATUS_SUCCESS) {
				WL_ERROR(("%s: scan status != error\n", __FUNCTION__));
				wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_BEACON,
					req->token, DOT11_RMREP_MODE_REFUSED);
				break;
			}

			if (bcn_req->scan_results.count == 0) {
				WL_ERROR(("%s: scan results count = 0\n", __FUNCTION__));
				/* send a null beacon report */
				wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_BEACON,
					req->token, 0);
				break;
			}
			WL_ERROR(("%s: scan type: %d, scan results count = %d, sendout bcnrep\n",
				__FUNCTION__, req->type, bcn_req->scan_results.count));

			wlc_rrm_send_bcnrep(rrm_info, &bcn_req->scan_results);
			break;
		case DOT11_RMREQ_BCN_TABLE:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_TABLE))
				break;
			if (req->flags & DOT11_RMREP_MODE_INCAPABLE) {
				wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_BEACON,
					req->token, DOT11_RMREP_MODE_INCAPABLE);
				break;
			}
			if (req->flags & DOT11_RMREP_MODE_REFUSED) {
				wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_BEACON,
					req->token, DOT11_RMREP_MODE_REFUSED);
				break;
			}
			bcn_req = rrm_info->bcnreq;
			if (bcn_req == NULL)
				break;
			WL_INFORM(("%s: BCN_TABLE\n", __FUNCTION__));

			wlc_rrm_bcnreq_scancache(rrm_info, &bcn_req->ssid, &bcn_req->bssid);
			break;
		case DOT11_MEASURE_TYPE_CHLOAD:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_CLM))
				break;
			if (measured == FALSE)
				break;
			wlc_rrm_send_chloadrep(rrm_info);
			break;
		case DOT11_MEASURE_TYPE_NOISE:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_NHM))
				break;
			if (measured == FALSE)
				break;
			wlc_rrm_send_noiserep(rrm_info);
			break;
		case DOT11_MEASURE_TYPE_FRAME:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_FM))
				break;
			if (measured == FALSE)
				break;
			wlc_rrm_send_framerep(rrm_info);
			break;
		case DOT11_MEASURE_TYPE_STAT:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_SM))
				break;
			if (measured == FALSE)
				break;
			wlc_rrm_send_statrep(rrm_info);
			break;
		case DOT11_MEASURE_TYPE_LCI:
			if (req->flags & DOT11_RMREP_MODE_INCAPABLE) {
				wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_LCI,
					req->token, DOT11_RMREP_MODE_INCAPABLE);
				break;
			}
			if (measured == FALSE)
				break;
#ifdef WL_FTM_11K
			wlc_rrm_send_lcirep(rrm_info, req->token);
#endif /* WL_FTM_11K */
			break;
		case DOT11_MEASURE_TYPE_TXSTREAM:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_TSCM))
				break;
			if (measured == FALSE)
				break;
			wlc_rrm_send_txstrmrep(rrm_info);
			break;
#ifdef WL_FTM_11K
		case DOT11_MEASURE_TYPE_CIVICLOC:
			if (req->flags & DOT11_RMREP_MODE_INCAPABLE) {
				wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_CIVICLOC,
					req->token, DOT11_RMREP_MODE_INCAPABLE);
				break;
			}
			if (measured == FALSE)
				break;
			wlc_rrm_send_civiclocrep(rrm_info, req->token);
			break;
#if defined(WL_PROXDETECT) && defined(WL_FTM)
		case DOT11_MEASURE_TYPE_FTMRANGE:
			if (!isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_FTM_RANGE) ||
				req->flags & DOT11_RMREP_MODE_INCAPABLE) {
				wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_FTMRANGE,
					req->token, DOT11_RMREP_MODE_INCAPABLE);
				break;
			}
			if (req->flags & DOT11_RMREP_MODE_REFUSED) {
				wlc_rrm_rep_err(rrm_info, DOT11_MEASURE_TYPE_FTMRANGE,
					req->token, DOT11_RMREP_MODE_REFUSED);
				break;
			}
			if (measured == FALSE)
				break;
			frng_rep = rrm_info->rrm_frng_req->frng_rep;
			if (frng_rep == NULL)
				break;

			/* send ftm range report to requester */
			wlc_rrm_send_frngrep(rrm_info, rrm_info->cur_cfg,
				(uint8 *)frng_rep, sizeof(*frng_rep));
			if (rrm_info->rrm_frng_req->rctx) {
				wlc_ftm_ranging_cancel(rrm_info->rrm_frng_req->rctx);
				wlc_ftm_ranging_destroy(&rrm_info->rrm_frng_req->rctx);
			}
			break;
#endif /* WL_PROXDETECT && WL_FTM */
#endif /* WL_FTM_11K */
		default:
			/* set measured so that we do not respond, even if a refusal flag is set */
			break;
		}
	}

}

static void
wlc_rrm_meas_end(wlc_info_t *wlc)
{
	wlc_rrm_info_t *rrm_info = wlc->rrm_info;
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	wlc_rrm_req_t *req;
	int req_idx;
	int req_count;

	if (rrm_state->scan_active) {
		WL_ERROR(("wl%d: wlc_rrm_meas_end: waiting for scan\n",
			wlc->pub->unit));
		return;
	}

	/* return to the home channel if needed */
	if (rrm_state->chanspec_return != 0) {
		wlc_suspend_mac_and_wait(wlc);
		WL_ERROR(("%s: return to the home channel\n", __FUNCTION__));
		wlc_set_chanspec(wlc, rrm_state->chanspec_return, CHANSW_IOVAR);
		wlc_enable_mac(wlc);
	}
	wlc_rrm_terminate(rrm_info);
	/* un-suspend the data fifos in case they were suspended
	 * * for off channel measurements
	 */
	if (!SCAN_IN_PROGRESS(wlc->scan) || ((wlc->scan->state & SCAN_STATE_WSUSPEND) == 0))
		wlc_tx_resume(wlc);
	/* count the requests for this set */
	for (req_idx = rrm_state->cur_req; req_idx < rrm_state->req_count; req_idx++) {
		req = &rrm_state->req[req_idx];
		/* check for end of parallel measurements */
		if (req_idx != rrm_state->cur_req &&
		    (req->flags & DOT11_RMREQ_MODE_PARALLEL) == 0) {
			break;
		}
	}
	req_count = req_idx - rrm_state->cur_req;
	req = &rrm_state->req[rrm_state->cur_req];
	switch (rrm_state->report_class) {
		case WLC_RRM_CLASS_IOCTL:
			wlc_rrm_report_ioctl(rrm_info, req, req_count);
			break;
		case WLC_RRM_CLASS_11K:
			wlc_rrm_report_11k(rrm_info, req, req_count);
	default:
		break;
	}
	/* done with the current set of measurements,
	 * advance the pointers and start the next set
	 */
	rrm_state->cur_req += req_count;
	wlc_rrm_next_set(rrm_info);
}

static void
wlc_rrm_report_ioctl(wlc_rrm_info_t *rrm_info, wlc_rrm_req_t *req_block, int count)
{
	WL_ERROR(("wl%d: %s dummy function\n", rrm_info->wlc->pub->unit,
	          __FUNCTION__));
}

static void
wlc_rrm_begin(wlc_rrm_info_t *rrm_info)
{
	int dur_max;
	chanspec_t rrm_chanspec;
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	wlc_rrm_req_t *req;
	int req_idx;
	bool blocked = FALSE;
	wlc_info_t *wlc = rrm_info->wlc;

	rrm_chanspec = wlc_rrm_chanspec(rrm_info);

	WL_INFORM(("%s: rrm_chanspec: %d\n", __FUNCTION__, rrm_chanspec));

	if (rrm_chanspec != 0)
		blocked = (wlc_mac_request_entry(wlc, NULL, WLC_ACTION_RM) != BCME_OK);

	/* check for a channel change */
	if (!blocked &&
	    rrm_chanspec != 0 &&
	    rrm_chanspec != WLC_BAND_PI_RADIO_CHANSPEC) {
		/* has the PS announcement completed yet? */
		if (rrm_state->ps_pending) {
			/* wait for PS state to be communicated before switching channels */
			wlc_rrm_state_upd(rrm_info, WLC_RRM_WAIT_PS_ANNOUNCE);
			WL_ERROR(("%s, state upd to %d\n", __FUNCTION__, rrm_state->step));
			return;
		}
		/* has the suspend completed yet? */
		if (wlc->pub->associated && !wlc_tx_suspended(wlc)) {
			/* suspend tx data fifos for off channel measurements */
			wlc_tx_suspend(wlc);
			/* wait for the suspend before switching channels */
			wlc_rrm_state_upd(rrm_info, WLC_RRM_WAIT_TX_SUSPEND);
			WL_ERROR(("%s, state upd to %d\n", __FUNCTION__, rrm_state->step));
			return;
		}
		if (CHSPEC_CHANNEL(rrm_chanspec) != 0 &&
			CHSPEC_CHANNEL(rrm_chanspec) != 255) {
			rrm_state->chanspec_return = WLC_BAND_PI_RADIO_CHANSPEC;
			/* skip CFP & TSF update */
			wlc_mhf(wlc, MHF2, MHF2_SKIP_CFP_UPDATE, MHF2_SKIP_CFP_UPDATE,
				WLC_BAND_ALL);
			wlc_skip_adjtsf(wlc, TRUE, NULL, WLC_SKIP_ADJTSF_RM, WLC_BAND_ALL);

			wlc_suspend_mac_and_wait(wlc);
			WL_ERROR(("%s: calling wlc_set_chanspec\n", __FUNCTION__));
			wlc_set_chanspec(wlc, rrm_chanspec, CHANSW_IOVAR);
			wlc_enable_mac(wlc);
		}
	} else {
		rrm_state->chanspec_return = 0;
		if (rrm_chanspec != 0 && rrm_chanspec != WLC_BAND_PI_RADIO_CHANSPEC)
			WL_ERROR(("wl%d: %s: fail to set channel "
				"for rm due to rm request is blocked\n",
				wlc->pub->unit, __FUNCTION__));
	}

	/* record the actual start time of the measurements */
	wlc_read_tsf(wlc, &rrm_state->actual_start_l, &rrm_state->actual_start_h);

	dur_max = 0;

	for (req_idx = rrm_state->cur_req; req_idx < rrm_state->req_count; req_idx++) {
		req = &rrm_state->req[req_idx];

		/* check for end of parallel measurements */
		if (req_idx != rrm_state->cur_req &&
		    (req->flags & DOT11_RMREQ_MODE_PARALLEL) == 0) {
			WL_ERROR(("%s: req_idx: %d, NOT PARALLEL\n",
				__FUNCTION__, req_idx));
			break;
		}

		/* check for request that have already been marked to skip */
		if (req->flags & (DOT11_RMREP_MODE_INCAPABLE | DOT11_RMREP_MODE_REFUSED)) {
			WL_ERROR(("%s: req->flags: %d\n", __FUNCTION__, req->flags));
			continue;
		}

		/* mark all requests as refused if blocked from measurement */
		if (blocked) {
			req->flags |= DOT11_RMREP_MODE_REFUSED;
			WL_ERROR(("%s: blocked\n", __FUNCTION__));
			continue;
		}

		if (req->dur > dur_max)
			dur_max = req->dur;

		/* record the actual start time of the measurements */
		req->tsf_h = rrm_state->actual_start_h;
		req->tsf_l = rrm_state->actual_start_l;

		switch (req->type) {
		case DOT11_RMREQ_BCN_ACTIVE:
		case DOT11_RMREQ_BCN_PASSIVE:
			wlc_rrm_begin_scan(rrm_info);
			break;
		case DOT11_RMREQ_BCN_TABLE:
			break;
		case DOT11_MEASURE_TYPE_FRAME:
			break;
		case DOT11_MEASURE_TYPE_STAT:
			break;
		case DOT11_MEASURE_TYPE_LCI:
			break;
		case DOT11_MEASURE_TYPE_TXSTREAM:
			break;
		case DOT11_MEASURE_TYPE_PAUSE:
			break;
#ifdef WL_FTM_11K
		case DOT11_MEASURE_TYPE_CIVICLOC:
			break;
#if defined(WL_PROXDETECT) && defined(WL_FTM)
		case DOT11_MEASURE_TYPE_FTMRANGE:
			wlc_rrm_begin_ftm(rrm_info);
			break;
#endif /* WL_PROXDETECT && WL_FTM */
#endif /* WL_FTM_11K */
		default:
			WL_ERROR(("wl%d: %s: unknown measurement "
				"request type %d in request token %d\n",
				wlc->pub->unit, __FUNCTION__, req->type, rrm_state->token));
			break;
		}
	}

	wlc_rrm_state_upd(rrm_info, WLC_RRM_WAIT_END_MEAS);
	WL_INFORM(("%s: state upd to %d\n", __FUNCTION__, rrm_state->step));

	if (dur_max == 0) {
		WL_INFORM(("wl%d: wlc_rrm_begin: zero dur for measurements, "
			   "scheduling end timer now\n", wlc->pub->unit));
		wl_add_timer(wlc->wl, rrm_info->rrm_timer, 0, 0);
	}

	return;
}

void
wlc_rrm_pm_pending_complete(wlc_rrm_info_t *rrm_info)
{
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	wlc_info_t *wlc = rrm_info->wlc;
	if (!rrm_state->ps_pending)
		return;

	rrm_state->ps_pending = FALSE;
	/* if the RM state machine is waiting for the PS announcement,
	 * then schedule the timer
	 */
	if (rrm_state->step == WLC_RRM_WAIT_PS_ANNOUNCE)
		wl_add_timer(wlc->wl, rrm_info->rrm_timer, 0, 0);
}

static void
wlc_rrm_timer(void *arg)
{
	wlc_rrm_info_t *rrm_info = (wlc_rrm_info_t *)arg;
	wlc_info_t *wlc = rrm_info->wlc;
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	wlc_rrm_req_t *req;
	uint32 offset, intval;
	int idx;
	wlc_bsscfg_t *cfg;
#ifdef WL11K_AP
	struct scb_iter scbiter;
	struct scb *scb;
	bool rrm_timer_set = FALSE;
#endif /* WL11K_AP */

	if (!wlc->pub->up)
		return;
	if (DEVICEREMOVED(wlc)) {
		WL_ERROR(("wl%d: %s: dead chip\n", wlc->pub->unit, __FUNCTION__));
		wl_down(wlc->wl);
		return;
	}
	switch (rrm_state->step) {
	       case WLC_RRM_WAIT_STOP_TRAFFIC:
		/* announce PS mode to the AP if we are not already in PS mode */
#ifdef STA
		rrm_state->ps_pending = TRUE;
		FOREACH_AS_STA(wlc, idx, cfg) {
			/* block any PSPoll operations since we are just holding off AP traffic */
			mboolset(cfg->pm->PMblocked, WLC_PM_BLOCK_CHANSW);
			if (cfg->pm->PMenabled)
				continue;
			WL_ERROR(("wl%d.%d: wlc_rrm_timer: WAIT_STOP_TRAFFIC, "
				"entering PS mode for off channel measurement\n",
				wlc->pub->unit, WLC_BSSCFG_IDX(cfg)));
			wlc_set_pmstate(cfg, TRUE);
		}
		/* We are supposed to wait for PM0->PM1 transition to finish but in case
		 * * we failed to send PM indications or failed to receive ACKs fake a PM0->PM1
		 * * transition so that anything depending on the transition to finish can move
		 * * forward i.e. scan engine can continue.
		*/
		wlc_pm_pending_complete(wlc);
		if (!wlc->PMpending)
			rrm_state->ps_pending = FALSE;
		wlc_set_wake_ctrl(wlc);

		/* suspend tx data fifos here for off channel measurements
		 * * if we are not announcing PS mode
		*/
		if (!rrm_state->ps_pending)
			wlc_tx_suspend(wlc);
#endif /* STA */
		/* calculate how long until this measurement should start */
		req = &rrm_state->req[rrm_state->cur_req];
		intval = req->intval * DOT11_TU_TO_US / 1000;
		if (intval)
			offset = R_REG(wlc->osh, &wlc->regs->u.d11regs.tsf_random) % intval;
		else
			offset = 0;

		if (offset > 0) {
			wlc_rrm_state_upd(rrm_info, WLC_RRM_WAIT_BEGIN_MEAS);
			WL_ERROR(("%s: state upd to %d\n", __FUNCTION__, rrm_state->step));

			wl_add_timer(wlc->wl, rrm_info->rrm_timer, offset, 0);
			WL_INFORM(("wl%d: wlc_rrm_timer: WAIT_STOP_TRAFFIC, "
				"%d ms until measurement, waiting for measurement timer\n",
				wlc->pub->unit, offset));
		} else if (rrm_state->ps_pending) {
			wlc_rrm_state_upd(rrm_info, WLC_RRM_WAIT_PS_ANNOUNCE);
			WL_ERROR(("%s: state upd to %d\n", __FUNCTION__, rrm_state->step));
			/* wait for PS state to be communicated by
			 * waiting for the Null Data packet tx to
			 * complete, then start measurements
			 */
			WL_ERROR(("wl%d: wlc_rrm_timer: WAIT_STOP_TRAFFIC, "
				"%d ms until measurement, waiting for PS announcement\n",
				wlc->pub->unit, offset));
		} else {
			wlc_rrm_state_upd(rrm_info, WLC_RRM_WAIT_TX_SUSPEND);
			WL_ERROR(("%s: state upd to %d\n", __FUNCTION__, rrm_state->step));
			/* wait for the suspend interrupt to come back
			 * and start measurements
			 */
			WL_ERROR(("wl%d: wlc_rrm_timer: WAIT_STOP_TRAFFIC, "
				"%d ms until measurement, waiting for TX fifo suspend\n",
				wlc->pub->unit, offset));
		}
		break;
	case WLC_RRM_WAIT_PS_ANNOUNCE:
	case WLC_RRM_WAIT_TX_SUSPEND:
	case WLC_RRM_WAIT_BEGIN_MEAS:
		wlc_rrm_state_upd(rrm_info, WLC_RRM_WAIT_START_SET);
		/* Set step as WLC_RRM_WAIT_START_SET,
		 * fall through to case WLC_RRM_WAIT_START_SET.
		 */
	case WLC_RRM_WAIT_START_SET:
		WL_INFORM(("wl%d: wlc_rrm_timer: START_SET\n",
			wlc->pub->unit));
		wlc_rrm_begin(rrm_info);
		break;
	case WLC_RRM_WAIT_END_SCAN:
		WL_INFORM(("wl%d: wlc_rrm_timer: END_SCAN\n",
			wlc->pub->unit));
		break;
	case WLC_RRM_WAIT_END_FRAME:
		WL_INFORM(("wl%d: wlc_rrm_timer: END_FRAME\n",
			wlc->pub->unit));
		break;
	case WLC_RRM_WAIT_END_MEAS:
		WL_INFORM(("wl%d: wlc_rrm_timer: END_MEASUREMENTS\n",
			wlc->pub->unit));
		wlc_rrm_meas_end(wlc);
		break;
	case WLC_RRM_ABORT:
		wlc_rrm_state_upd(rrm_info, WLC_RRM_IDLE);
		WL_INFORM(("%s, state upd to %d\n", __FUNCTION__, rrm_state->step));
		break;
	case WLC_RRM_IDLE:
#ifdef WL11K_AP
		/* use a timer to send periodic beacon requests 11k enabled STAs */
		FOREACH_UP_AP(wlc, idx, cfg)
		{
			rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
			FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
				if (SCB_ASSOCIATED(scb)) {
					rrm_scb_info_t *scb_info;
					bcn_req_t bcnreq;
					uint32 current_time;

					memset(&bcnreq, 0, sizeof(bcn_req_t));
					scb_info = RRM_SCB_INFO(rrm_info, scb);
					if (!scb_info) {
						continue;
					}

					memcpy(&bcnreq.da, &scb->ea, ETHER_ADDR_LEN);
					memcpy(&bcnreq.ssid.SSID, &scb->bsscfg->SSID,
						scb->bsscfg->SSID_len);
					bcnreq.ssid.SSID_len = scb->bsscfg->SSID_len;
					bcnreq.bcn_mode = DOT11_RMREQ_BCN_PASSIVE;
					bcnreq.channel = 0;	 /* all channels */
					bcnreq.dur = WLC_RRM_BCNREQ_INTERVAL_DUR;
					bcnreq.random_int = 0;
					bcnreq.reps = 1;
					bcnreq.req_elements = 1;
					current_time = OSL_SYSUPTIME();

					if (isset(scb_info->rrm_capabilities,
						DOT11_RRM_CAP_BCN_PASSIVE) &&
						rrm_cfg->bcn_req_timer != 0) {
						if (((current_time - scb_info->bcnreq_time >=
							rrm_cfg->bcn_req_timer) ||
							(scb_info->bcnreq_time == 0))) {
							scb_info->bcnreq_time = current_time;
							wlc_rrm_flush_bcnnbrs(rrm_info, scb);
							wlc_rrm_send_bcnreq(rrm_info, cfg, &bcnreq);
						}
						if (!rrm_timer_set) {
							rrm_timer_set = TRUE;
							rrm_cfg->rrm_timer_set = FALSE;
							wlc_rrm_add_timer(wlc, scb);
						}
					}
				}
			} /* for each bss scb */
			rrm_timer_set = FALSE;
		}
#endif /* WL11K_AP */
		WL_INFORM(("wl%d: %s: error, in timer with state WLC_RRM_IDLE\n",
			wlc->pub->unit, __FUNCTION__));
		break;
	default:
		WL_ERROR(("wl%d: %s: Invalid rrm state:%d\n", WLCWLUNIT(wlc), __FUNCTION__,
			rrm_state->step));
		break;
	}
	/* update PS control */
	wlc_set_wake_ctrl(wlc);
}

static void
wlc_rrm_validate(wlc_rrm_info_t *rrm_info)
{
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	chanspec_t rrm_chanspec;
	wlc_rrm_req_t *req;
	int i;
	wlc_info_t *wlc = rrm_info->wlc;
	DBGONLY(char chanbuf[CHANSPEC_STR_LEN]; )

	rrm_chanspec = 0;

	for (i = 0; i < rrm_state->req_count; i++) {
		req = &rrm_state->req[i];
		if (!(req->flags & DOT11_RMREQ_MODE_PARALLEL)) {
			/* new set of parallel measurements */
			rrm_chanspec = 0;
		}

		/* check for an unsupported channel */
		if (CHSPEC_CHANNEL(req->chanspec) != 0 &&
			CHSPEC_CHANNEL(req->chanspec) != 255 &&
			!wlc_valid_chanspec_db(wlc->cmi, req->chanspec)) {
			if (req->type == DOT11_RMREQ_BCN_TABLE)
				WL_ERROR(("%s: DOT11_RMREP_MODE_INCAPABLE\n",
					__FUNCTION__));
			req->flags |= DOT11_RMREP_MODE_INCAPABLE;
			continue;
		}

		/* refuse zero dur measurements */
		if (req->type == DOT11_RMREQ_BCN_TABLE ||
			req->type == DOT11_MEASURE_TYPE_LCI ||
			req->type == DOT11_MEASURE_TYPE_CIVICLOC) {
		}
		else {
			uint32 activity_delta;
			if (req->dur == 0) {
				req->flags |= DOT11_RMREP_MODE_REFUSED;
				continue;
			}
			/* If data activity in past bcn_req_traff_meas_prd ms, refuse */
			activity_delta = OSL_SYSUPTIME() - rrm_info->data_activity_ts;
			if (activity_delta < rrm_info->bcn_req_traff_meas_prd) {
				WL_ERROR(("%s: Req refused due to data activity in last %d ms\n",
					__FUNCTION__, activity_delta));
				req->flags |= DOT11_RMREP_MODE_REFUSED;
				continue;
			}
		}
		/* pick up the channel for this parallel set */
		if (rrm_chanspec == 0)
			rrm_chanspec = req->chanspec;

		/* check for parallel measurements on different channels */
		if (req->chanspec && req->chanspec != rrm_chanspec) {
			WL_INFORM(("wl%d: wlc_rrm_validate: refusing parallel "
				   "measurement with different channel than "
				   "others, RM type %d token %d chanspec %s\n",
				   wlc->pub->unit, req->type, req->token,
				   wf_chspec_ntoa_ex(req->chanspec, chanbuf)));
			req->flags |= DOT11_RMREP_MODE_REFUSED;
			continue;
		}
	}
}

static void
wlc_rrm_end(wlc_rrm_info_t *rrm_info)
{
	/* clean up state */
	wlc_rrm_free(rrm_info);
	if (rrm_info->rrm_state->cb) {
		cb_fn_t cb = (cb_fn_t)(rrm_info->rrm_state->cb);
		void *cb_arg = rrm_info->rrm_state->cb_arg;

		(cb)(cb_arg);
		rrm_info->rrm_state->cb = NULL;
		rrm_info->rrm_state->cb_arg = NULL;
	}
}

/* start the radio measurement state machine working on
 * the next set of parallel measurements.
 */
static void
wlc_rrm_next_set(wlc_rrm_info_t *rrm_info)
{
	uint32 offset = 0, intval = 0;
	chanspec_t chanspec;
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	wlc_info_t *wlc = rrm_info->wlc;

	if (rrm_state->cur_req >= rrm_state->req_count) {
		if (rrm_state->reps == 0) {
			/* signal that all requests are done */
			wlc_rrm_end(rrm_info);
			WL_INFORM(("%s: all req are done\n", __FUNCTION__));
			return;
		} else {
			rrm_state->reps--;
			rrm_state->cur_req = 0;
		}
	}

	/* Start a timer for off-channel prep if measurements are off-channel,
	 * or for the measurements if on-channel
	 */
	if (wlc->pub->associated &&
		(chanspec = wlc_rrm_chanspec(rrm_info)) != 0 &&
		chanspec != wlc->home_chanspec) {
		if (rrm_state->chanspec_return != 0)
			offset = WLC_RRM_HOME_TIME;
		/* off-channel measurements, set a timer to prepare for the channel switch */
		wlc_rrm_state_upd(rrm_info, WLC_RRM_WAIT_STOP_TRAFFIC);
		WL_INFORM(("%s: state upd to %d\n", __FUNCTION__, rrm_state->step));

	} else {
		wlc_rrm_req_t *req = &rrm_state->req[rrm_state->cur_req];
		if (req->intval > 0) {
			/* calculate a random delay(ms) distributed uniformly in the range 0
			 * to the random internval
			 */
			intval = req->intval * DOT11_TU_TO_US / 1000;
			offset = R_REG(wlc->osh, &wlc->regs->u.d11regs.tsf_random)
					% intval;
		}
		/* either unassociated, so no channel prep even if we do change channels,
		 * or channel-less measurement set (all suppressed or internal state report),
		 * or associated and measurements on home channel,
		 * set the timer for WLC_RRM_WAIT_START_SET
		 */

		wlc_rrm_state_upd(rrm_info, WLC_RRM_WAIT_START_SET);
		WL_INFORM(("%s: state upd to %d, offset: %d\n",
			__FUNCTION__, rrm_state->step, offset));
	}

	if (offset > 0)
		wl_add_timer(wlc->wl, rrm_info->rrm_timer, offset, 0);
	else
		wlc_rrm_timer(rrm_info);

	return;
}

static void
wlc_rrm_start(wlc_info_t *wlc)
{
	wlc_rrm_info_t *rrm_info = wlc->rrm_info;
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	DBGONLY(char chanbuf[CHANSPEC_STR_LEN]; )
#ifdef BCMDBG
	wlc_rrm_req_t *req;
	const char *name;
	int i;
#endif /* BCMDBG */

	rrm_state->cur_req = 0;
	wlc_rrm_state_upd(rrm_info, WLC_RRM_IDLE);
	WL_INFORM(("%s: state upd to %d\n", __FUNCTION__, rrm_state->step));

#ifdef BCMDBG
	WL_INFORM(("wl%d: wlc_rrm_start(): %d RM Requests, token 0x%x (%d)\n",
		rrm_info->wlc->pub->unit, rrm_state->req_count,
		rrm_state->token, rrm_state->token));

	for (i = 0; i < rrm_state->req_count; i++) {
		req = &rrm_state->req[i];
		switch (req->type) {

		case DOT11_RMREQ_BCN_ACTIVE:
			name = "Active Scan";
			break;
		case DOT11_RMREQ_BCN_PASSIVE:
			name = "Passive Scan";
			break;
		case DOT11_RMREQ_BCN_TABLE:
			name = "Beacon Table";
			break;
		case DOT11_MEASURE_TYPE_FRAME:
			name = "Frame";
			break;
		case DOT11_MEASURE_TYPE_STAT:
			name = "STA statistics";
			break;
		case DOT11_MEASURE_TYPE_LCI:
			name = "Location";
			break;
		case DOT11_MEASURE_TYPE_TXSTREAM:
			name = "TX stream";
			break;
		case DOT11_MEASURE_TYPE_PAUSE:
			name = "Pause";
			break;
		case DOT11_MEASURE_TYPE_FTMRANGE:
			name = "FTM Range";
			break;
		case DOT11_MEASURE_TYPE_CIVICLOC:
			name = "Civic location";
			break;
		default:
			name = "";
			WL_ERROR(("wl%d: %s: Unsupported RRM Req/Measure type:%d\n", WLCWLUNIT(wlc),
				__FUNCTION__, req->type));
			break;
		}

		WL_ERROR(("RM REQ token 0x%02x (%2d), type %2d: %s, chanspec %s, "
			"tsf 0x%x:%08x dur %4d TUs\n",
			req->token, req->token, req->type, name,
			wf_chspec_ntoa_ex(req->chanspec, chanbuf), req->tsf_h, req->tsf_l,
			req->dur));
	}
#endif /* BCMDBG */
	wlc_rrm_validate(rrm_info);

	wlc_rrm_next_set(rrm_info);
}

static void
wlc_rrm_state_upd(wlc_rrm_info_t *rrm_info, uint state)
{
	bool was_in_progress;
	wlc_rrm_req_state_t *rrm_state = rrm_info->rrm_state;
	wlc_info_t *wlc = rrm_info->wlc;

	if (rrm_state->step == state)
		return;

	WL_INFORM(("wlc_rrm_state_upd; change from %d to %d\n", rrm_state->step, state));
	was_in_progress = WLC_RRM_IN_PROGRESS(wlc);
	rrm_state->step = state;
	if (WLC_RRM_IN_PROGRESS(wlc) != was_in_progress)
		wlc_phy_hold_upd(WLC_PI(wlc), PHY_HOLD_FOR_RM, !was_in_progress);

	return;
}

#ifdef WL11K_AP
/* AP Channel Report */
static uint
wlc_rrm_calc_ap_ch_rep_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_rrm_info_t *rrm = (wlc_rrm_info_t *)ctx;
	wlc_info_t *wlc = rrm->wlc;
	wlc_bsscfg_t *cfg = data->cfg;
	uint len = 0;

	if (WL11K_ENAB(wlc->pub) && BSSCFG_AP(cfg) && wlc_rrm_enabled(rrm, cfg))
		len = wlc_rrm_ap_chrep_len(rrm, cfg);

	return len;
}

static int
wlc_rrm_write_ap_ch_rep_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_rrm_info_t *rrm = (wlc_rrm_info_t *)ctx;
	wlc_info_t *wlc = rrm->wlc;
	wlc_bsscfg_t *cfg = data->cfg;

	if (WL11K_ENAB(wlc->pub) && BSSCFG_AP(cfg) && wlc_rrm_enabled(rrm, cfg))
		wlc_rrm_add_ap_chrep(rrm, cfg, data->buf);

	return BCME_OK;
}






#endif /* WL11K_AP */

/* RRM Cap */
static uint
wlc_rrm_calc_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_rrm_info_t *rrm = (wlc_rrm_info_t *)ctx;
	wlc_info_t *wlc = rrm->wlc;
	wlc_bsscfg_t *cfg = data->cfg;

	if (WL11K_ENAB(wlc->pub) && wlc_rrm_enabled(rrm, cfg))
		return TLV_HDR_LEN + DOT11_RRM_CAP_LEN;

	return 0;
}

static int
wlc_rrm_write_cap_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_rrm_info_t *rrm = (wlc_rrm_info_t *)ctx;
	wlc_info_t *wlc = rrm->wlc;
	wlc_bsscfg_t *cfg = data->cfg;
	rrm_bsscfg_cubby_t *rrm_cfg;

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm, cfg);
	ASSERT(rrm_cfg != NULL);

	if (WL11K_ENAB(wlc->pub) && wlc_rrm_enabled(rrm, cfg)) {
		/* RM Enabled Capabilities */
		bcm_write_tlv(DOT11_MNG_RRM_CAP_ID, rrm_cfg->rrm_cap, DOT11_RRM_CAP_LEN,
			data->buf);
	}

	return BCME_OK;
}

#ifdef STA
/* return TRUE if wlc_bss_info_t contains RRM IE, else FALSE */
static bool
wlc_rrm_is_rrm_ie(wlc_bss_info_t *bi)
{
	uint bcn_parse_len = bi->bcn_prb_len - sizeof(struct dot11_bcn_prb);
	uint8 *bcn_parse = (uint8*)bi->bcn_prb + sizeof(struct dot11_bcn_prb);

	if (bcm_parse_tlvs(bcn_parse, bcn_parse_len, DOT11_MNG_RRM_CAP_ID))
		return TRUE;

	return FALSE;
}

/* RRM Cap */
static uint
wlc_rrm_calc_assoc_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	/* include RRM cap only if target AP supports RRM cap */
	if (wlc_rrm_is_rrm_ie(wlc_iem_calc_get_assocreq_target(data)))
		return wlc_rrm_calc_cap_ie_len(ctx, data);

	return 0;
}

static int
wlc_rrm_write_assoc_cap_ie(void *ctx, wlc_iem_build_data_t *data)
{
	/* include RRM cap only if target AP supports RRM cap */
	if (wlc_rrm_is_rrm_ie(wlc_iem_build_get_assocreq_target(data)))
		return wlc_rrm_write_cap_ie(ctx, data);

	return BCME_OK;
}

static int
wlc_rrm_parse_rrmcap_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	wlc_rrm_info_t *rrm = (wlc_rrm_info_t *)ctx;
	wlc_info_t *wlc = rrm->wlc;
	wlc_bsscfg_t *cfg = data->cfg;
	wlc_assoc_t *as = cfg->assoc;
	wlc_iem_ft_pparm_t *ftpparm = data->pparm->ft;
	struct scb *scb = ftpparm->assocresp.scb;

	if (WL11K_ENAB(wlc->pub) && wlc_rrm_enabled(wlc->rrm_info, cfg)) {
		/* update the AP's scb for RRM if assoc response has RRM Cap bit set and
		 * RRM cap IE present.
		 */
		scb->flags3 &= ~SCB3_RRM;
		if ((ltoh16(as->resp->capability) & DOT11_CAP_RRM) &&
			data->ie != NULL) {
			scb->flags3 |= SCB3_RRM;
		}
	}
	return BCME_OK;
}

#ifdef WLASSOC_NBR_REQ
/* called from sta assocreq_done function via scb state notif */
static void
wlc_rrm_sta_scb_state_upd(void *ctx, scb_state_upd_data_t *data)
{
	wlc_rrm_info_t *rrm_info = (wlc_rrm_info_t *)ctx;
	struct scb *scb = data->scb;
	wlc_bsscfg_t *cfg = scb->bsscfg;
	wlc_ssid_t ssid;
	int err;

	if (!WL11K_ENAB(rrm_info->wlc->pub) || !BSSCFG_STA(cfg)) {
		return;
	}

	/* hndl transition from unassoc to assoc */
	if (!(data->oldstate & ASSOCIATED) && SCB_ASSOCIATED(scb) &&
			SCB_RRM(scb)) {
		/* Trigger a neighbor report to prepare hot channel list for roaming */
		if (cfg->BSS && cfg->associated && cfg->current_bss &&
				(cfg->current_bss->capability & DOT11_CAP_RRM)) {
			ssid.SSID_len = cfg->SSID_len;
			memcpy(ssid.SSID, cfg->SSID, cfg->SSID_len);
			err = wlc_iovar_op(rrm_info->wlc, "rrm_nbr_req", NULL, 0,
					&ssid, sizeof(wlc_ssid_t), IOV_SET, cfg->wlcif);
			if (err) {
				WL_ERROR(("wl%d: ERROR %d wlc_iovar_op failed: \"rrm_nbr_req\"\n",
						rrm_info->wlc->pub->unit, err));
			}
		}
	}
}
#endif /* WLASSOC_NBR_REQ */
#endif /* STA */

#ifdef WL11K_AP

#endif /* WL11K_AP */

void
wlc_rrm_stop(wlc_info_t *wlc)
{
	WL_ERROR(("%s: abort rrm processing\n", __FUNCTION__));
	/* just abort for now, eventually, send reports then stop */
	wlc_rrm_abort(wlc);
}

bool
wlc_rrm_inprog(wlc_info_t *wlc)
{
	return WLC_RRM_IN_PROGRESS(wlc);
}

#ifdef WL_FTM_11K
bool
wlc_rrm_ftm_inprog(wlc_info_t *wlc)
{
	return (WLC_RRM_IN_PROGRESS(wlc) &&
			wlc->rrm_info->rrm_state->scan_active &&
			wlc->rrm_info->rrm_frng_req->frng_req);
}
#endif /* WL_FTM_11K */

bool
wlc_rrm_wait_tx_suspend(wlc_info_t *wlc)
{
	return (wlc->rrm_info->rrm_state->step == WLC_RRM_WAIT_TX_SUSPEND);
}

void
wlc_rrm_start_timer(wlc_info_t *wlc)
{
	wl_add_timer(wlc->wl, wlc->rrm_info->rrm_timer, 0, 0);
}

#ifdef WL11K_AP
void
wlc_rrm_add_timer(wlc_info_t *wlc, struct scb *scb)
{
	rrm_bsscfg_cubby_t *rrm_cfg;
	wlc_bsscfg_t *bsscfg = scb->bsscfg;

	rrm_cfg = RRM_BSSCFG_CUBBY(wlc->rrm_info, bsscfg);
	if (rrm_cfg->rrm_timer_set)
		return;
	wl_add_timer(wlc->wl, wlc->rrm_info->rrm_timer, rrm_cfg->bcn_req_timer, 0);
	rrm_cfg->rrm_timer_set = TRUE;
}

static bool
wlc_rrm_security_match(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, uint16 cap,
	bcm_tlv_t *wpaie, bcm_tlv_t *wpa2ie)
{
	bool match = FALSE;

	if ((wpa2ie != NULL) && bcmwpa_is_wpa2_auth(cfg->WPA_auth) && WSEC_ENABLED(cfg->wsec)) {
		match = TRUE;
	}
	if ((wpaie != NULL) && bcmwpa_is_wpa_auth(cfg->WPA_auth) && WSEC_ENABLED(cfg->wsec)) {
		match = TRUE;
	}
	if ((wpa2ie == NULL) && (wpaie == NULL) &&
		WSEC_ENABLED(cfg->wsec) == (cap & DOT11_CAP_PRIVACY)) {
		match = TRUE;
	}

	return match;
}

/* called from ap assocreq_done function via scb state notif */
static void
wlc_rrm_ap_scb_state_upd(void *ctx, scb_state_upd_data_t *data)
{
	wlc_rrm_info_t *rrm_info = (wlc_rrm_info_t *)ctx;
	struct scb *scb = data->scb;
	wlc_bsscfg_t *cfg = scb->bsscfg;
	BCM_REFERENCE(cfg);

	if (!WL11K_ENAB(rrm_info->wlc->pub) || !BSSCFG_AP(cfg)) {
		return;
	}

	/* hndl transition from unassoc to assoc */
	if (!(data->oldstate & ASSOCIATED) && SCB_ASSOCIATED(scb) &&
		SCB_RRM(scb)) {
		wlc_rrm_add_timer(rrm_info->wlc, scb);
	}
}

static int
wlc_assoc_parse_rrm_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_iem_ft_pparm_t *ftpparm = data->pparm->ft;
	wlc_bsscfg_t *cfg = data->cfg;
	wlc_rrm_info_t *rrm_info = wlc->rrm_info;
	bcm_tlv_t *rrmie = (bcm_tlv_t *)data->ie;
	struct scb *scb = ftpparm->assocreq.scb;
	rrm_scb_info_t *rrm_scb = RRM_SCB_INFO(rrm_info, scb);

	if (WL11K_ENAB(wlc->pub) && BSSCFG_AP(cfg) && wlc_rrm_enabled(rrm_info, cfg)) {

		if (rrmie == NULL)
			return BCME_OK;

		if ((rrmie->len == DOT11_RRM_CAP_LEN) &&
			(rrm_scb != NULL)) {
			bcopy(rrmie->data, rrm_scb->rrm_capabilities, DOT11_RRM_CAP_LEN);
		}
	}

	return BCME_OK;
}
#endif /* WL11K_AP */

bool
wlc_rrm_enabled(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg)
{
	rrm_bsscfg_cubby_t *rrm_cfg;
	int i;

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
	ASSERT(rrm_cfg != NULL);

	for (i = 0; i < DOT11_RRM_CAP_LEN; i++) {
		if (rrm_cfg->rrm_cap[i] != 0)
			return TRUE;
	}
	return FALSE;
}

static bool
wlc_rrm_recv_rmreq_enabled(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *cfg, int body_len, uint8 *body)
{
	int len;
	uint8 *req_pkt;
	dot11_rm_ie_t *req_ie;
	rrm_bsscfg_cubby_t *rrm_cfg;
	dot11_rmreq_bcn_t *rmreq_bcn = NULL;

	rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
	ASSERT(rrm_cfg != NULL);

	len = body_len;
	req_pkt = body;
	req_pkt += DOT11_RMREQ_LEN;
	req_ie = (dot11_rm_ie_t *)req_pkt;

	while (req_ie) {
		if (req_ie->id == DOT11_MNG_MEASURE_REQUEST_ID) {
			switch (req_ie->type) {
			case DOT11_MEASURE_TYPE_BEACON:
				rmreq_bcn = (dot11_rmreq_bcn_t *)req_ie;
				if (rmreq_bcn->bcn_mode == DOT11_RMREQ_BCN_ACTIVE) {
					if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_ACTIVE))
						return TRUE;
				} else if (rmreq_bcn->bcn_mode == DOT11_RMREQ_BCN_PASSIVE) {
					if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_PASSIVE))
						return TRUE;
				} else if (rmreq_bcn->bcn_mode == DOT11_RMREQ_BCN_TABLE) {
					if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_TABLE))
						return TRUE;
				}
				break;
			case DOT11_MEASURE_TYPE_NOISE:
				if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_NHM))
					return TRUE;
				break;
			case DOT11_MEASURE_TYPE_CHLOAD:
				if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_CLM))
					return TRUE;
				break;
			case DOT11_MEASURE_TYPE_FRAME:
				if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_FM))
					return TRUE;
				break;
			case DOT11_MEASURE_TYPE_STAT:
				if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_SM))
					return TRUE;
				break;
			case DOT11_MEASURE_TYPE_TXSTREAM:
				if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_TSCM))
					return TRUE;
				break;
#ifdef WL_FTM_11K
			case DOT11_MEASURE_TYPE_LCI:
				if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LCIM))
					return TRUE;
				break;
			case DOT11_MEASURE_TYPE_CIVICLOC:
				if (isset(rrm_cfg->rrm_cap, DOT11_RRM_CAP_CIVIC_LOC))
					return TRUE;
				break;
#if defined(WL_PROXDETECT) && defined(WL_FTM)
			case DOT11_MEASURE_TYPE_FTMRANGE:
				/* respond to these requests even if incapable */
				return TRUE;
				break;
#endif /* WL_PROXDETECT && WL_FTM */
#endif /* WL_FTM_11K */
			default:
				WL_ERROR(("wl%d: %s: Invalid Measure type:%d\n",
					WLCWLUNIT(rrm_info->wlc), __FUNCTION__, req_ie->type));
				break;
			}
		}
		req_ie = wlc_rrm_next_ie(req_ie, &len, DOT11_MNG_MEASURE_REQUEST_ID);
	}
	return FALSE;
}

static int
wlc_rrm_bsscfg_init(void *context, wlc_bsscfg_t *bsscfg)
{
	wlc_rrm_info_t *rrm_info = (wlc_rrm_info_t *)context;
	wlc_info_t *wlc = rrm_info->wlc;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, bsscfg);
	int err = BCME_OK;

	rrm_cfg->wlc = wlc;
	rrm_cfg->rrm_info = rrm_info;
	rrm_cfg->rrm_timer_set = FALSE;

	/* Configure RRM enabled capability */
	bzero(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LEN);
#ifdef WL11K_AP
	setbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_NEIGHBOR_REPORT);
#endif /* WL11K_AP */
	setbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_LINK);
	setbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_AP_CHANREP);
	setbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_PASSIVE);
	setbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_ACTIVE);
	setbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_SM);
#ifdef WLSCANCACHE
	if (SCANCACHE_ENAB(wlc->scan))
		setbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_TABLE);
#endif /* WLSCANCACHE */

	wlc->pub->_rrm = TRUE;

	return err;
}

static void
wlc_rrm_bsscfg_deinit(void *context, wlc_bsscfg_t *bsscfg)
{
	wlc_rrm_info_t *rrm_info = (wlc_rrm_info_t *)context;
	rrm_bsscfg_cubby_t *rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, bsscfg);

	if (rrm_cfg == NULL)
		return;

	/* Free nbr_rep_head and nbr_cnt_head */
	wlc_rrm_flush_neighbors(rrm_info, bsscfg);

#ifdef WL_FTM_11K
	wlc_rrm_free_self_report(rrm_info, bsscfg);
#endif
}

#ifdef WLSCANCACHE
void
wlc_rrm_update_cap(wlc_rrm_info_t *rrm_info, wlc_bsscfg_t *bsscfg)
{
	rrm_bsscfg_cubby_t *rrm_cfg;
	ASSERT(bsscfg != NULL);
	rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, bsscfg);

	if (SCANCACHE_ENAB(rrm_info->wlc->scan))
		setbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_TABLE);
	else
		clrbit(rrm_cfg->rrm_cap, DOT11_RRM_CAP_BCN_TABLE);
}
#endif /* WLSCANCACHE */

#ifdef BCMDBG
static void
wlc_rrm_bsscfg_dump(void *context, wlc_bsscfg_t *bsscfg, struct bcmstrbuf *b)
{
	wlc_rrm_info_t *rrm_info = (wlc_rrm_info_t *)context;

	bcm_bprintf(b, "cfgh: %d\n", rrm_info->cfgh);
}
#endif

bool wlc_rrm_in_progress(wlc_info_t *wlc)
{
	return WLC_RRM_IN_PROGRESS(wlc);
}

/* scb cubby_init function */
static int wlc_rrm_scb_init(void *ctx, struct scb *scb)
{
	wlc_rrm_info_t *rrm_info = (wlc_rrm_info_t *)ctx;
	wlc_info_t *wlc = rrm_info->wlc;

	rrm_scb_cubby_t *scb_cubby = RRM_SCB_CUBBY(rrm_info, scb);
	rrm_scb_info_t *scb_info;

	scb_info = (rrm_scb_info_t *)MALLOCZ(wlc->osh, sizeof(rrm_scb_info_t));
	if (!scb_info) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
			(int)sizeof(rrm_scb_info_t), MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}


	scb_cubby->scb_info = scb_info;

	return BCME_OK;
}

/* scb cubby_deinit fucntion */
static void wlc_rrm_scb_deinit(void *ctx, struct scb *scb)
{
	wlc_rrm_info_t *rrm_info = (wlc_rrm_info_t *)ctx;
	wlc_info_t *wlc = rrm_info->wlc;
	rrm_scb_cubby_t *scb_cubby = RRM_SCB_CUBBY(rrm_info, scb);
	rrm_scb_info_t *scb_info = RRM_SCB_INFO(rrm_info, scb);
#ifdef WL11K_AP
	struct scb_iter scbiter;
	struct scb *tscb;
	bool delete_timer = TRUE;
	int idx;
	wlc_bsscfg_t *cfg;
	rrm_bsscfg_cubby_t *rrm_cfg = NULL;
	FOREACH_UP_AP(wlc, idx, cfg) {
		FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, tscb) {
			rrm_scb_info_t *tscb_info;
			rrm_cfg = RRM_BSSCFG_CUBBY(rrm_info, cfg);
			tscb_info = RRM_SCB_INFO(rrm_info, tscb);
			if (!scb_info) {
				continue;
			}

			if (isset(tscb_info->rrm_capabilities, DOT11_RMREQ_BCN_PASSIVE)) {
				delete_timer = FALSE;
				break;
			}
		}
	}
	if (delete_timer) {
		wl_del_timer(wlc->wl, rrm_info->rrm_timer);
		if (rrm_cfg)
			rrm_cfg->rrm_timer_set = FALSE;
	}
	wlc_rrm_flush_bcnnbrs(rrm_info, scb);
#endif /* WL11K_AP */

	if (scb_info)
		MFREE(wlc->osh, scb_info, sizeof(rrm_scb_info_t));

	scb_cubby->scb_info = NULL;
}

#ifdef BCMDBG
/* scb cubby_dump fucntion */
static void wlc_rrm_scb_dump(void *ctx, struct scb *scb, struct bcmstrbuf *b)
{
	wlc_rrm_info_t *rrm_info = (wlc_rrm_info_t *)ctx;
	rrm_scb_info_t *scb_info = RRM_SCB_INFO(rrm_info, scb);
	rrm_nbr_rep_t *nbr_rep;
	int count = 0;
	char eabuf[ETHER_ADDR_STR_LEN];

	bcm_bprintf(b, "     SCB: len:%d timestamp:%u\n",
		scb_info->len, scb_info->timestamp);

	bcm_bprintf(b, "RRM capability = %02x %02x %02x %02x %02x \n",
		scb_info->rrm_capabilities[0], scb_info->rrm_capabilities[1],
		scb_info->rrm_capabilities[2], scb_info->rrm_capabilities[3],
		scb_info->rrm_capabilities[4]);
	bcm_bprintf(b, "RRM Neighbor Report (from beacons):\n");
	nbr_rep = scb_info->nbr_rep_head;
	while (nbr_rep) {
		count++;
		bcm_bprintf(b, "AP %2d: ", count);
		bcm_bprintf(b, "bssid %s ", bcm_ether_ntoa(&nbr_rep->nbr_elt.bssid, eabuf));
		bcm_bprintf(b, "bssid_info %08x ", load32_ua(&nbr_rep->nbr_elt.bssid_info));
		bcm_bprintf(b, "reg %2d channel %3d phytype %d pref %3d\n", nbr_rep->nbr_elt.reg,
			nbr_rep->nbr_elt.channel, nbr_rep->nbr_elt.phytype,
			nbr_rep->nbr_elt.bss_trans_preference);
		nbr_rep = nbr_rep->next;
	}
}
#endif /* BCMDBG */

void
wlc_rrm_upd_data_activity_ts(wlc_rrm_info_t *ri)
{
	if (ri->bcn_req_traff_meas_prd)
		ri->data_activity_ts = OSL_SYSUPTIME();
}

static void
wlc_rrm_watchdog(void *context)
{
	wlc_rrm_info_t *ri = (wlc_rrm_info_t *) context;
	if (ri->bcn_req_thrtl_win_sec) {
		ri->bcn_req_thrtl_win_sec--;
		/* Reset params for next window (to be started at next bcn_req rx) */
		if (ri->bcn_req_thrtl_win_sec == 0) {
			ri->bcn_req_off_chan_time = 0;
			ri->bcn_req_win_scan_cnt = 0;
		}
	}
}
