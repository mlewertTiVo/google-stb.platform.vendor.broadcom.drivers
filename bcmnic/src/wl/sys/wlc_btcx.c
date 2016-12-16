/*
 * BT Coex module
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
 * $Id: wlc_btcx.c 665208 2016-10-16 23:44:23Z $
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <sbchipc.h>
#include <sbgci.h>
#include <bcmendian.h>
#include <proto/802.11.h>
#include <wlioctl.h>
#include <bcmwpa.h>
#include <bcmwifi_channels.h>
#include <bcmdevs.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_key.h>
#include <wlc_channel.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_btcx.h>
#include <wlc_scan.h>
#include <wlc_assoc.h>
#include <wlc_bmac.h>
#include <wlc_ap.h>
#include <wlc_stf.h>
#include <wlc_ampdu.h>
#include <wlc_ampdu_rx.h>
#include <wlc_ampdu_cmn.h>
#ifdef WLMCNX
#include <wlc_mcnx.h>
#endif
#include <wlc_hw_priv.h>
#ifdef BCMLTECOEX
#include <wlc_ltecx.h>
#endif /* BCMLTECOEX */
#ifdef WLRSDB
#include <wlc_rsdb.h>
#endif /* WLRSDB */
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_ie_mgmt_vs.h>
#ifdef WLAIBSS
#include <wlc_aibss.h>
#endif
#include <wlc_rspec.h>
#include <wlc_lq.h>
#include <wlc_dump.h>
#include <wlc_iocv.h>

#ifdef BCMULP
#include <ulp.h>
#include <wlc_ulp.h>
#endif /* BCMULP */

/* Defines */
#define BTC_BTRSSI_THRESH	-70 /* btrssi thresh for implibit mode switch */
/* actual btrssi = -1 * (btrssi_from_ucode * btrssi_step + btrssi_offset) */
#define BTC_BTRSSI_STEP		5
#define BTC_BTRSSI_OFFSET	10
#define BTC_BTRSSI_INVALID	0   /* invalid btrssi */
#define BTC_BTRSSI_MAX_SAMP		16  /* max number of btrssi samples for moving avg. */
#define BTC_BTRSSI_MIN_SAMP	4   /* min number of btrssi samples for moving avg. */


#ifndef MSECS_PER_SEC
#define MSECS_PER_SEC 1000
#endif /* MSECS_PER_SEC */

#define BLE_DEBOUNCE_ON_SECS	3
#define BLE_DEBOUNCE_OFF_SECS	20
#define A2DP_DEBOUNCE_ON_SECS	1
#define A2DP_DEBOUNCE_OFF_SECS	10

#define BT_AMPDU_THRESH		21000	/* if BT period < this threshold, turn off ampdu */
/* RX aggregation size (high/low) is decided based on this threshold */
#define BT_AMPDU_RESIZE_THRESH         7500

#ifdef WLRSDB
static void wlc_btcx_write_core_specific_mask(wlc_info_t *wlc);
#endif
#ifdef WLAIBSS
/* Actions supported by wlc_btc_aibss_get_state () */
#define BT_CHECK_AIBSS_STALE_BEACON	0x1
#define BT_CHECK_AIBSS_BT_SYNC_STATE	0x2

/* Periods, intervals related to BT */
#define BT_ESCO_PERIOD			7500 /* us */
#define BT_SCO_PERIOD			3750 /* us */
#define BT_STROBE_INTERVAL		30000 /* us */
#define BT_ESCO_OFF_DELAY_CNT		30 /* Delay reporting eSCO Off count */
#define BT_STALE_BEACON_CHK_PERIOD	5 /* watchdog period: ~ 1 sec */
#define BT_MAX_ACL_GRANT_CNT		10 /* Max value in secs */

#define BT_AIBSS_RX_AGG_SIZE		8

/* BT-WLAN information (via GCI) related bits */
#define BT_IS_SLAVE			0x8000
#define BT_CLK_SYNC_STS			0x40
#define BT_CLK_SYNC_STS_SHIFT_BITS	6

/* defines related to BT info */
#define BT_INFO_TASK_TYPE_SCO		0x1
#define BT_INFO_TASK_TYPE_ESCO		0x2
#define BT_INFO_BT_CLK_SYNCED		0x4

#define WLC_BTCX_STROBE_REG_MASK	(ALLONES_32) /* 0xFFFFFFFF */

/* defines for IBSS TSF SHM locations */
#define BT_IBSS_TSF_LOW_OFFSET		(0)
#define BT_IBSS_TSF_HIGH_OFFSET		(2)
#define BT_IBSS_TSF_ESCO_OFFSET		(4)
#define BT_AIBSS_TSF_JUMP_THRESH	1100000 // TSF jump threshold in us
#define BT_ACL_TASK_ID			(1)
#define BT_ACL_TASK_BIT			(0x1 << BT_ACL_TASK_ID)
#endif /* WLAIBSS */

/* ENUMs */
enum {
	IOV_BTC_MODE =			0,	/* BT Coexistence mode */
	IOV_BTC_WIRE =			1,	/* BTC number of wires */
	IOV_BTC_STUCK_WAR =		2,	/* BTC stuck WAR */
	IOV_BTC_FLAGS =			3,	/* BT Coex ucode flags */
	IOV_BTC_PARAMS =		4,	/* BT Coex shared memory parameters */
	IOV_COEX_DEBUG_GPIO_ENABLE =	5,
	IOV_BTC_SISO_ACK =		7,	/* Specify SISO ACK antenna, disabled when 0 */
	IOV_BTC_RXGAIN_THRESH =		8,	/* Specify restage rxgain thresholds */
	IOV_BTC_RXGAIN_FORCE =		9,	/* Force for BTC restage rxgain */
	IOV_BTC_RXGAIN_LEVEL =		10,	/* Set the BTC restage rxgain level */
	IOV_BTC_DYNCTL =		11,	/* get/set coex dynctl profile */
	IOV_BTC_DYNCTL_STATUS =		12,	/* get dynctl status: dsns, btpwr,
						   wlrssi, btc_mode, etc
						*/
	IOV_BTC_DYNCTL_SIM =		13,	/* en/dis & config dynctl algo simulation mode */
	IOV_BTC_STATUS =		14,	/* bitmap of current bt-coex state */
	IOV_BTC_AIBSS_STATUS =		15,
	IOV_BTC_SET_STROBE =		16,	/* Get/Set BT Strobe */
	IOV_BTC_SELECT_PROFILE =	17,
	IOV_BTC_PROFILE =		18,	/* BTC profile for UCM */
	IOV_LAST
};

enum {
	BTCX_STATUS_BTC_ENABLE_SHIFT =	0,	/* btc is enabled */
	BTCX_STATUS_ACT_PROT_SHIFT =	1,	/* active protection */
	BTCX_STATUS_SLNA_SHIFT =	2,	/* sim rx with shared fem */
	BTCX_STATUS_SISO_ACK_SHIFT =	3,	/* siso ack */
	BTCX_STATUS_SIM_RSP_SHIFT =	4,	/* limited low power tx when BT active */
	BTCX_STATUS_PS_PROT_SHIFT =	5,	/* PS mode is allowed for protection */
	BTCX_STATUS_SIM_TX_LP_SHIFT =	6,	/* use low power for simultaneous tx */
	BTCX_STATUS_DYN_AGG_SHIFT =	7,	/* dynamic aggregation is enabled */
	BTCX_STATUS_ANT_WLAN_SHIFT =	8,	/* prisel always to WL */
	BTCX_STATUS_DEF_BT_SHIFT =	9,	/* BT granted over non-critical WLAN */
	BTCX_STATUS_DEFANT_SHIFT =	10	/* default shared antenna position */
};

/*
 * Various Aggregation states that BTCOEX can request,
 * saved as a variable for improved visibility.
 */
enum {
	BTC_AGGOFF = 0,
	BTC_CLAMPTXAGG_RXAGGOFFEXCEPTAWDL = 1,
	BTC_DYAGG = 2,
	BTC_AGGON = 3,
	BTC_LIMAGG = 4
};

/* Constants */
const bcm_iovar_t btc_iovars[] = {
	{"btc_mode", IOV_BTC_MODE, 0, 0, IOVT_UINT32, 0},
	{"btc_stuck_war", IOV_BTC_STUCK_WAR, 0, 0, IOVT_BOOL, 0 },
	{"btc_flags", IOV_BTC_FLAGS, (IOVF_SET_UP | IOVF_GET_UP), 0, IOVT_BUFFER, 0 },
	{"btc_params", IOV_BTC_PARAMS, 0, 0, IOVT_BUFFER, 0 },
	{"btc_siso_ack", IOV_BTC_SISO_ACK, 0, 0, IOVT_INT16, 0
	},
	{"btc_rxgain_thresh", IOV_BTC_RXGAIN_THRESH, 0, 0, IOVT_UINT32, 0
	},
	{"btc_rxgain_force", IOV_BTC_RXGAIN_FORCE, 0, 0, IOVT_UINT32, 0
	},
	{"btc_rxgain_level", IOV_BTC_RXGAIN_LEVEL, 0, 0, IOVT_UINT32, 0
	},
#ifdef WL_BTCDYN
	/* set dynctl profile, get status , etc */
	{"btc_dynctl", 0, IOV_BTC_DYNCTL, 0, IOVT_BUFFER, 0
	},
	/* set dynctl status */
	{"btc_dynctl_status", IOV_BTC_DYNCTL_STATUS, 0, 0, IOVT_BUFFER, 0
	},
	/* enable & configure dynctl simulation mode (aka dryrun) */
	{"btc_dynctl_sim", IOV_BTC_DYNCTL_SIM, 0, 0, IOVT_BUFFER, 0
	},
#endif
	{"btc_status", IOV_BTC_STATUS, (IOVF_GET_UP), 0, IOVT_UINT32, 0},
#ifdef WLAIBSS
#if defined(BCMDBG)
	{"btc_set_strobe", IOV_BTC_SET_STROBE, 0, 0, IOVT_UINT32, 0
	},
#endif /* BCMDBG */
	{"btc_aibss_status", IOV_BTC_AIBSS_STATUS, 0, 0, IOVT_BUFFER, 0
	},
#endif /* WLAIBSS */
#ifdef STA
#ifdef WLBTCPROF
	{"btc_select_profile", IOV_BTC_SELECT_PROFILE, 0, 0, IOVT_BUFFER, 0},
#endif
#endif /* STA */
#ifdef WL_UCM
	{"btc_profile", IOV_BTC_PROFILE, 0, 0, IOVT_BUFFER, sizeof(wlc_btcx_profile_t)
	},
#endif /* WL_UCM */
	{NULL, 0, 0, 0, 0, 0}
};

#ifdef WL_BTCDYN
/*  btcdyn nvram variables to initialize the profile  */
static const char BCMATTACHDATA(rstr_btcdyn_flags)[] = "btcdyn_flags";
static const char BCMATTACHDATA(rstr_btcdyn_dflt_dsns_level)[] = "btcdyn_dflt_dsns_level";
static const char BCMATTACHDATA(rstr_btcdyn_low_dsns_level)[] = "btcdyn_low_dsns_level";
static const char BCMATTACHDATA(rstr_btcdyn_mid_dsns_level)[] = "btcdyn_mid_dsns_level";
static const char BCMATTACHDATA(rstr_btcdyn_high_dsns_level)[] = "btcdyn_high_dsns_level";
static const char BCMATTACHDATA(rstr_btcdyn_default_btc_mode)[] = "btcdyn_default_btc_mode";
static const char BCMATTACHDATA(rstr_btcdyn_msw_rows)[] = "btcdyn_msw_rows";
static const char BCMATTACHDATA(rstr_btcdyn_dsns_rows)[] = "btcdyn_dsns_rows";
static const char BCMATTACHDATA(rstr_btcdyn_msw_row0)[] = "btcdyn_msw_row0";
static const char BCMATTACHDATA(rstr_btcdyn_msw_row1)[] = "btcdyn_msw_row1";
static const char BCMATTACHDATA(rstr_btcdyn_msw_row2)[] = "btcdyn_msw_row2";
static const char BCMATTACHDATA(rstr_btcdyn_msw_row3)[] = "btcdyn_msw_row3";
static const char BCMATTACHDATA(rstr_btcdyn_dsns_row0)[] = "btcdyn_dsns_row0";
static const char BCMATTACHDATA(rstr_btcdyn_dsns_row1)[] = "btcdyn_dsns_row1";
static const char BCMATTACHDATA(rstr_btcdyn_dsns_row2)[] = "btcdyn_dsns_row2";
static const char BCMATTACHDATA(rstr_btcdyn_dsns_row3)[] = "btcdyn_dsns_row3";
static const char BCMATTACHDATA(rstr_btcdyn_btrssi_hyster)[] = "btcdyn_btrssi_hyster";
#endif /* WL_BTCDYN */

/* BT RSSI threshold for implict mode switching */
static const char BCMATTACHDATA(rstr_prot_btrssi_thresh)[] = "prot_btrssi_thresh";

/* structure definitions */
struct wlc_btc_debounce_info {
	uint8 on_timedelta;		/*
					 * #secs of continuous inputs
					 * of 1 to result in setting
					 * debounce to 1.
					 */
	uint8 off_timedelta;		/* #secs of continuous inputs
					 * of 0 to result in clearing
					 * debounce to 0.
					 */
	uint16 status_inprogress;	/* pre-debounced value */
	uint16 status_debounced;		/* Final debounced value */
	int status_timestamp;		/* Time since last consecutive
					 * value seen in current_status
					 */
};
typedef struct wlc_btc_debounce_info wlc_btc_debounce_info_t;

/* BTCDYN info */
struct wlc_btcdyn_info {
	/* BTCOEX extension: adds dynamic desense & modes witching feature */
	uint16	bt_pwr_shm;	/* last raw/per task bt_pwr read from ucode */
	int8	bt_pwr;		/* current bt power  */
	int8	wl_rssi;	/* last wl rssi */
	uint8	cur_dsns;	/* current desense level */
	dctl_prof_t *dprof;	/* current board dynctl profile  */
	btcx_dynctl_calc_t desense_fn;  /* calculate desense level  */
	btcx_dynctl_calc_t mswitch_fn;  /* calculate mode switch */
	/*  stimuli for dynctl dry runs(fake BT & WL activity) */
	int8	sim_btpwr;
	int8	sim_wlrssi;
	int8	sim_btrssi;
	/* mode switching hysteresis */
	int8 msw_btrssi_hyster;	/* from bt rssi */
	bool	dynctl_sim_on;  /* enable/disable simulation mode */
	uint32	prev_btpwr_ts;	/* timestamp of last call to btc_dynctl() */
	int8	prev_btpwr;		/* prev btpwr reading to filter out false invalids */
};
typedef struct wlc_btcdyn_info wlc_btcdyn_info_t;

/*  used by BTCDYN BT pwr debug code */
typedef struct pwrs {
	int8 pwr;
	uint32 ts;
} pwrs_t;

/* BTC stuff */
struct wlc_btc_info {
	wlc_info_t *wlc;
	uint16  bth_period;             /* bt coex period. read from shm. */
	bool    bth_active;             /* bt active session */
	uint8   prot_rssi_thresh;       /* rssi threshold for forcing protection */
	uint8   ampdutx_rssi_thresh;    /* rssi threshold to turn off ampdutx */
	uint8   ampdurx_rssi_thresh;    /* rssi threshold to turn off ampdurx */
	uint8   high_threshold;         /* threshold to switch to btc_mode 4 */
	uint8   low_threshold;          /* threshold to switch to btc_mode 1 */
	uint8   host_requested_pm;      /* saved pm state specified by host */
	uint8   mode_overridden;        /* override btc_mode for long range */
	/* cached value for btc in high driver to avoid frequent RPC calls */
	int     mode;
	int     wire;
	int16   siso_ack;               /* txcoremask for siso ack (e.g., 1: use core 1 for ack) */
	int     restage_rxgain_level;
	int     restage_rxgain_force;
	int     restage_rxgain_active;
	uint8   restage_rxgain_on_rssi_thresh;  /* rssi threshold to turn on rxgain restaging */
	uint8   restage_rxgain_off_rssi_thresh; /* rssi threshold to turn off rxgain restaging */
	uint16	agg_off_bm;
	bool    siso_ack_ovr;           /* siso_ack set 0: automatically 1: by iovar */
	wlc_btc_prev_connect_t *btc_prev_connect; /* btc previous connection info */
	wlc_btc_select_profile_t *btc_profile; /* User selected profile for 2G and 5G params */
	wlc_btcdyn_info_t *btcdyn; /* btcdyn info */
	int8	*btrssi_sample;        /* array of recent BT RSSI values */
	uint8   btrssi_maxsamp;          /* maximum number of btrssi samples to average */
	uint8   btrssi_minsamp;          /* minimum number of btrssi samples to average */
	int16	btrssi_sum;	/* bt rssi MA sum */
	uint8   btrssi_cnt;     /* number of btrssi samples */
	uint8	btrssi_idx;	/* index to bt_rssi sample array */
	int8	bt_rssi;	/* averaged bt rssi */
	uint16	bt_shm_addr;
	uint8	run_cnt;
	int8	prot_btrssi_thresh; /* used by implicit mode switching */
	int8    dyagg;                  /* dynamic tx agg (1: on, 0: off, -1: auto) */
	bool    simrx_slna;             /* simultaneous rx with shared fem/lna */
	int8	btrssi_avg;		/* average btrssi */
	uint8	btrssi_hyst;		/* btrssi hysteresis */
	uint8	wlrssi_hyst;		/* wlrssi hysteresis */
	uint16	acl_last_ts;
	uint16	a2dp_last_ts;
	uint8	agg_state_req;	/* Keeps state of the requested Aggregation state
				 * requested by BTC
				 */
	wlc_btc_debounce_info_t *ble_debounce; /* Used to debounce shmem for ble activity. */
	wlc_btc_debounce_info_t *a2dp_debounce; /* Used to debounce shmem for last_a2dp activity. */
	int8	sco_threshold_set;
	int32	scb_handle;	/* SCB CUBBY OFFSET */
	wlc_btc_aibss_info_t *aibss_info; // AIBSS related cfg & status info
	uint8	rxagg_resized;			/* Flag to indicate rx agg size was resized */
	uint8	rxagg_size;				/* New Rx agg size when SCO is on */
	uint8	pad;		/* can be re-used */
	wlc_btcx_profile_t	**profile_array;	/* UCM BTCX profile array */
};

#ifdef WLAIBSS
/* BTCX info structure for Oxygen */
typedef struct {
	uint32 btinfo;
	uint32 bcn_rx_cnt;
} btcx_oxygen_info_t;
#endif // WLAIBSS


/* Function prototypes */
static int wlc_btc_mode_set(wlc_info_t *wlc, int int_val);
static int wlc_btc_wire_set(wlc_info_t *wlc, int int_val);
static int wlc_btc_flags_idx_set(wlc_info_t *wlc, int int_val, int int_val2);
static int wlc_btc_flags_idx_get(wlc_info_t *wlc, int int_val);
static void wlc_btc_stuck_war50943(wlc_info_t *wlc, bool enable);
static void wlc_btc_rssi_threshold_get(wlc_info_t *wlc);
static int16 wlc_btc_siso_ack_get(wlc_info_t *wlc);
static int wlc_btc_wlc_up(void *ctx);
static int wlc_btc_wlc_down(void *ctx);
int wlc_btcx_desense(wlc_btc_info_t *btc, int band);
#ifdef WL_BTCDYN
static int wlc_btc_dynctl_profile_set(wlc_info_t *wlc, void *parambuf);
static int wlc_btc_dynctl_profile_get(wlc_info_t *wlc, void *resbuf);
static int wlc_btc_dynctl_status_get(wlc_info_t *wlc, void *resbuf);
static int wlc_btc_dynctl_sim_get(wlc_info_t *wlc, void *resbuf);
static int wlc_btc_dynctl_sim_set(wlc_info_t *wlc, void *parambuf);
static int wlc_btcdyn_attach(wlc_btc_info_t *btc);
static void wlc_btcx_dynctl(wlc_btc_info_t *btc);
#endif /* WL_BTCDYN */
static void wlc_btc_update_btrssi(wlc_btc_info_t *btc);
static void wlc_btc_reset_btrssi(wlc_btc_info_t *btc);

#if defined(STA) && defined(BTCX_PM0_IDLE_WAR)
static void wlc_btc_pm_adjust(wlc_info_t *wlc,  bool bt_active);
#endif
static int wlc_btc_doiovar(void *hdl, uint32 actionid,
        void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif);
static void wlc_btcx_watchdog(void *arg);
static int wlc_dump_btcx(void *ctx, struct bcmstrbuf *b);
static int wlc_clr_btcxdump(void *ctx);
static uint32 wlc_btcx_get_btc_status(wlc_info_t *wlc);

#ifdef STA
static bool wlc_btcx_check_port_open(wlc_info_t *wlc);
#endif

static wlc_btc_debounce_info_t *btc_debounce_init(wlc_info_t *wlc, uint8 on_seconds,
	uint8 off_seconds);
#ifdef WLAMPDU
static int wlc_btcx_rssi_for_ble_agg_decision(wlc_info_t *wlc);
static void wlc_btcx_evaluate_agg_state(wlc_btc_info_t *btc);
static uint16 wlc_btcx_debounce(wlc_btc_debounce_info_t *btc_debounce, uint16 curr);
static bool wlc_btcx_is_hybrid_in_TDM(wlc_info_t *wlc);
#endif
static void wlc_btc_update_sco_params(wlc_info_t *wlc);

#ifdef WLAIBSS
static uint wlc_btcx_aibss_ie_len(void *ctx, wlc_iem_calc_data_t *calc);
static int wlc_btcx_aibss_write_ie(void *ctx, wlc_iem_build_data_t *build);
static int wlc_btcx_aibss_parse_ie(void *ctx, wlc_iem_parse_data_t *parse);
static void wlc_btcx_strobe_enable(wlc_btc_info_t *btc, bool on);
static int wlc_btcx_scb_init(void *ctx, struct scb *scb);
static void wlc_btcx_scb_deinit(void *ctx, struct scb *scb);
static void wlc_btcx_bss_updn(void *ctx, bsscfg_up_down_event_data_t *evt);
static void wlc_btcx_aibss_set_prot(wlc_btc_info_t *btc, bool prot_on, bool cmn_cts2self);
static void wlc_btcx_aibss_check_state(wlc_btc_info_t *btc);
static void wlc_btcx_aibss_read_bt_sync_state(wlc_btc_info_t *btc);
static void wlc_btcx_aibss_get_local_bt_state(wlc_btc_info_t *btc);
static void wlc_btcx_aibss_update_agg_state(wlc_btc_info_t *btc);
static void wlc_btc_aibss_get_state(wlc_btc_info_t *btc, uint8 action);
static void wlc_btcx_aibss_calc_tsf(wlc_btc_info_t *btc, uint32 *new_tsf_l,
	uint32 mod_factor, uint16 add_delta);
static void wlc_btcx_aibss_update_agg_size(wlc_btc_info_t *btc);
static void wlc_btcx_aibss_chk_clk_sync(wlc_btc_info_t *btc);
static int wlc_btcx_aibss_set_strobe(wlc_btc_info_t *btc, int strobe_on);
static int wlc_btcx_aibss_get_status(wlc_info_t *wlc, void *buf);
#endif /* WLAIBSS */

#ifdef WLAMPDU
static void wlc_btcx_rx_ba_window_modify(wlc_info_t *wlc, bool *rx_agg);
static void wlc_btcx_rx_ba_window_reset(wlc_info_t *wlc);
#endif /* WLAMPDU */

#ifdef WL_UCM
static int wlc_btc_prof_get(wlc_info_t *wlc, int32 idx, void *resbuf, uint len);
static int wlc_btc_prof_set(wlc_info_t *wlc, void *parambuf, uint len);
static int wlc_btc_ucm_attach(wlc_btc_info_t *btc);
static void wlc_btc_ucm_detach(wlc_btc_info_t *btc);
#endif /* WL_UCM */

#ifdef BCMULP
static int wlc_ulp_btcx_enter_cb(void *handle, ulp_ext_info_t *einfo);

/* p1 context for phase1 store/retrieve/restore. This is used to register to
 * the ulp baselevel [ulp.c/h] for phase1. In phase1 storage, the
 * values/iovar's gets stored in shm separate from wowl/ulp-ucode-shm space
 * and dedicated to the driver, [called "driver-cache-shm"] which is NOT used
 * by wowl/ulp-ucode.
 */
static const ulp_p1_module_pubctx_t wlc_ulp_btcx_p1_ctx = {
	MODCBFL_CTYPE_STATIC,
	NULL,
	NULL,
	NULL,
	NULL,
	wlc_ulp_btcx_enter_cb
};
#endif /* BCMULP */

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>


wlc_btc_info_t *
BCMATTACHFN(wlc_btc_attach)(wlc_info_t *wlc)
{
	wlc_btc_info_t *btc;
#ifdef WLBTCPROF
	wlc_btc_profile_t *select_profile;
#endif /* WLBTCPROF */
	wlc_hw_info_t *wlc_hw = wlc->hw;

	if ((btc = (wlc_btc_info_t*)
		MALLOCZ(wlc->osh, sizeof(wlc_btc_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	btc->wlc = wlc;

	/* register module */
	if (wlc_module_register(wlc->pub, btc_iovars, "btc", btc, wlc_btc_doiovar,
		wlc_btcx_watchdog, wlc_btc_wlc_up, wlc_btc_wlc_down) != BCME_OK) {
		WL_ERROR(("wl%d: %s: btc register err\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#ifdef BCMULP
	if ((ulp_p1_module_register(ULP_MODULE_ID_WLC_ULP_BTCX,
			&wlc_ulp_btcx_p1_ctx, (void*)btc)) != BCME_OK) {
		WL_ERROR(("%s: ulp_p1_module_register failed\n", __FUNCTION__));
		goto fail;
	}
#endif

	/* register dump stats for btcx */
	wlc_dump_add_fns(wlc->pub, "btcx", wlc_dump_btcx, wlc_clr_btcxdump, wlc);

#if defined(BCMCOEXNOA) && !defined(BCMCOEXNOA_DISABLED)
	wlc->pub->_cxnoa = TRUE;
#endif /* defined(BCMCOEXNOA) && !defined(BCMCOEXNOA_DISABLED) */

#ifdef WLBTCPROF
	if ((btc->btc_prev_connect = (wlc_btc_prev_connect_t *)
		MALLOCZ(wlc->osh, sizeof(wlc_btc_prev_connect_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	if ((btc->btc_profile = (wlc_btc_select_profile_t *)
		MALLOCZ(wlc->osh, sizeof(wlc_btc_select_profile_t) * BTC_SUPPORT_BANDS)) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	bzero(btc->btc_prev_connect, sizeof(wlc_btc_prev_connect_t));
	bzero(btc->btc_profile, sizeof(wlc_btc_profile_t) * BTC_SUPPORT_BANDS);

	memset(&btc->btc_prev_connect->prev_2G_profile, 0, sizeof(struct wlc_btc_profile));
	memset(&btc->btc_prev_connect->prev_5G_profile, 0, sizeof(struct wlc_btc_profile));

	btc->btc_prev_connect->prev_band = WLC_BAND_ALL;

	select_profile = &btc->btc_profile[BTC_PROFILE_2G].select_profile;
	select_profile->btc_wlrssi_thresh = BTC_WL_RSSI_DEFAULT;
	select_profile->btc_btrssi_thresh = BTC_BT_RSSI_DEFAULT;
	if (CHIPID(wlc->pub->sih->chip) == BCM4350_CHIP_ID) {
		select_profile->btc_num_desense_levels = MAX_BT_DESENSE_LEVELS_4350;
		select_profile->btc_wlrssi_hyst = BTC_WL_RSSI_HYST_DEFAULT_4350;
		select_profile->btc_btrssi_hyst = BTC_BT_RSSI_HYST_DEFAULT_4350;
		select_profile->btc_max_siso_resp_power[0] =
			BTC_WL_MAX_SISO_RESP_POWER_TDD_DEFAULT_4350;
		select_profile->btc_max_siso_resp_power[1] =
			BTC_WL_MAX_SISO_RESP_POWER_HYBRID_DEFAULT_4350;
	}
#endif /* WLBTCPROF */

	btc->siso_ack_ovr = FALSE;

#if defined(WL_BTCDYN) && !defined(WL_BTCDYN_DISABLED)
	/* btcdyn attach */
	if (wlc_btcdyn_attach(btc) != BCME_OK) {
		goto fail;
	}
	wlc->pub->_btcdyn = TRUE;
#endif

	if ((btc->btrssi_sample =
		(int8*)MALLOCZ(wlc->osh, sizeof(int8) * BTC_BTRSSI_MAX_SAMP)) == NULL) {
		WL_ERROR(("wl%d: %s: MALLOC for btrssi_sample failed, %d bytes\n",
		    wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	btc->btrssi_maxsamp = BTC_BTRSSI_MAX_SAMP;
	btc->btrssi_minsamp = BTC_BTRSSI_MIN_SAMP;
	wlc_btc_reset_btrssi(btc);

	if (getvar(wlc_hw->vars, rstr_prot_btrssi_thresh) != NULL) {
		btc->prot_btrssi_thresh =
			(int8)getintvar(wlc_hw->vars, rstr_prot_btrssi_thresh);
	} else {
		btc->prot_btrssi_thresh = BTC_BTRSSI_THRESH;
	}

	btc->dyagg = AUTO;

	if ((btc->ble_debounce = btc_debounce_init(wlc, BLE_DEBOUNCE_ON_SECS,
			BLE_DEBOUNCE_OFF_SECS)) == NULL) {
		goto fail;
	}
	if ((btc->a2dp_debounce = btc_debounce_init(wlc, A2DP_DEBOUNCE_ON_SECS,
			A2DP_DEBOUNCE_OFF_SECS)) == NULL) {
		goto fail;
	}
	btc->simrx_slna = FALSE;
	// Default BTCX agg state is ON.
	btc->agg_state_req = BTC_AGGON;

#ifdef WLAIBSS
	if ((btc->aibss_info = (wlc_btc_aibss_info_t *)
			MALLOCZ(wlc->osh, sizeof(wlc_btc_aibss_info_t))) == NULL) {
			WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			goto fail;
	}

	/* reserve cubby in the scb container to monitor per SCB BT info */
	if ((btc->scb_handle = wlc_scb_cubby_reserve(wlc,
		sizeof(btcx_oxygen_info_t *), wlc_btcx_scb_init, wlc_btcx_scb_deinit,
		NULL, btc)) < 0) {
		WL_ERROR(("wl%d: %s: btc scb cubby err\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* bsscfg up/down callback */
	if (wlc_bsscfg_updown_register(wlc, wlc_btcx_bss_updn, btc) != BCME_OK) {
		WL_ERROR(("wl%d: %s: bss updown err\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* IE mgmt callbacks for AIBSS */
	/* bcn */
	if (wlc_iem_vs_add_build_fn(wlc->iemi, FC_BEACON, WLC_IEM_VS_IE_PRIO_BRCM_BTCX,
	                            wlc_btcx_aibss_ie_len, wlc_btcx_aibss_write_ie,
	                            btc) != BCME_OK) {
		WL_ERROR(("wl%d: %s btc ie add err\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	if (wlc_iem_vs_add_parse_fn(wlc->iemi, FC_BEACON, WLC_IEM_VS_IE_PRIO_BRCM_BTCX,
	                            wlc_btcx_aibss_parse_ie, btc) != BCME_OK) {
		WL_ERROR(("wl%d: %s btc parse err\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#endif /* WLAIBSS */

#if defined(WL_UCM) && !defined(WL_UCM_DISABLED)
	/* all the UCM related attach activities go here */
	if (wlc_btc_ucm_attach(btc) != BCME_OK) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	wlc->pub->_ucm = TRUE;
#endif /* WL_UCM && !WL_UCM_DISABLED */
	return btc;

	/* error handling */
fail:
	MODULE_DETACH(btc, wlc_btc_detach);
	return NULL;
}

static wlc_btc_debounce_info_t *
btc_debounce_init(wlc_info_t *wlc, uint8 on_seconds, uint8 off_seconds)
{
	/* Initialize debounce variables */
	wlc_btc_debounce_info_t *btc_debounce;
	if ((btc_debounce = (wlc_btc_debounce_info_t *)
		MALLOCZ(wlc->osh, sizeof(wlc_btc_debounce_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}
	btc_debounce->on_timedelta = on_seconds;
	btc_debounce->off_timedelta = off_seconds;
	btc_debounce->status_inprogress = 0;
	btc_debounce->status_debounced = 0;
	btc_debounce->status_timestamp = 0;
	return btc_debounce;
}

void
BCMATTACHFN(wlc_btc_detach)(wlc_btc_info_t *btc)
{
	wlc_info_t *wlc;

	if (btc == NULL)
		return;

	wlc = btc->wlc;
	wlc_module_unregister(wlc->pub, "btc", btc);

#ifdef WL_BTCDYN
	if (BTCDYN_ENAB(wlc->pub)) {
		if (btc->btcdyn != NULL) {
			if (btc->btcdyn->dprof)
				MFREE(wlc->osh, btc->btcdyn->dprof, sizeof(dctl_prof_t));
			btc->btcdyn->dprof = NULL;
			MFREE(wlc->osh, btc->btcdyn, sizeof(wlc_btcdyn_info_t));
		}
	}
#endif /* WL_BTCDYN */

#if defined(BCMCOEXNOA) && !defined(BCMCOEXNOA_DISABLED)
	wlc->pub->_cxnoa = FALSE;
#endif /* defined(BCMCOEXNOA) && !defined(BCMCOEXNOA_DISABLED) */

#ifdef WLBTCPROF
	if (btc->btc_prev_connect != NULL)
		MFREE(wlc->osh, btc->btc_prev_connect, sizeof(wlc_btc_prev_connect_t));
	if (btc->btc_profile != NULL)
		MFREE(wlc->osh, btc->btc_profile, sizeof(wlc_btc_select_profile_t));
#endif /* WLBTCPROF */
	if (btc->btrssi_sample != NULL)
		MFREE(wlc->osh, btc->btrssi_sample, sizeof(int8) * BTC_BTRSSI_MAX_SAMP);

	if (btc->a2dp_debounce != NULL)
		MFREE(wlc->osh, btc->a2dp_debounce, sizeof(wlc_btc_debounce_info_t));
	if (btc->ble_debounce != NULL)
		MFREE(wlc->osh, btc->ble_debounce, sizeof(wlc_btc_debounce_info_t));

#ifdef WLAIBSS
	if (btc->aibss_info) {
		MFREE(wlc->osh, btc->aibss_info, sizeof(wlc_btc_aibss_info_t));
		btc->aibss_info = NULL;
	}
#endif /* WLAIBSS */

#ifdef WL_UCM
	/* All the UCM related detach activities go here */
	wlc_btc_ucm_detach(btc);
#endif /* WL_UCM */

	MFREE(wlc->osh, btc, sizeof(wlc_btc_info_t));
}

#ifdef WL_UCM
/* All UCM related attach activities go here */
static int
BCMATTACHFN(wlc_btc_ucm_attach)(wlc_btc_info_t *btc)
{
	wlc_info_t *wlc = btc->wlc;
	int idx, idx2;
	size_t ucm_profile_sz;
	size_t ucm_array_sz = MAX_UCM_PROFILES*sizeof(*btc->profile_array);
	uint8 chain_attr_count = WLC_BITSCNT(wlc->stf->hw_txchain);
	wlc_btcx_chain_attr_t *chain_attr;

	/* Check from obj registry if common info is allocated */
	btc->profile_array = obj_registry_get(wlc->objr, OBJR_BTC_PROFILE_INFO);
	if (btc->profile_array == NULL) {
		/* first create an array of pointers to btc profile */
		btc->profile_array = (wlc_btcx_profile_t **)MALLOCZ(wlc->osh, ucm_array_sz);
		if (btc->profile_array == NULL) {
			WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			return BCME_NOMEM;
		}
		/* Update registry after allocating profile array */
		obj_registry_set(wlc->objr, OBJR_BTC_PROFILE_INFO, btc->profile_array);

		/* malloc individual profiles and insert their address in the btc profile array */
		for (idx = 0; idx < MAX_UCM_PROFILES; idx++) {
			/* the profile size is determined by the number of tx chains */
			ucm_profile_sz = sizeof(*btc->profile_array[idx])
				+ chain_attr_count*sizeof(btc->profile_array[idx]->chain_attr[0]);
			btc->profile_array[idx] =
				(wlc_btcx_profile_t *)MALLOCZ(wlc->osh, ucm_profile_sz);
			if (btc->profile_array[idx] == NULL) {
				WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
					wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
				return BCME_NOMEM;
			}
			btc->profile_array[idx]->profile_index = idx;
			/* last element of the profile is a struct with explicit padding */
			btc->profile_array[idx]->length = ucm_profile_sz;
			/* whenever changing the profile struct, update the last fixed entry here */
			btc->profile_array[idx]->fixed_length =
				STRUCT_SIZE_THROUGH(btc->profile_array[idx],
				tx_pwr_wl_lo_hi_rssi_thresh);
			btc->profile_array[idx]->version = UCM_PROFILE_VERSION;
			btc->profile_array[idx]->chain_attr_count = chain_attr_count;
			/* whenever changing the attributes struct, update last fixed entry here */
			for (idx2 = 0; idx2 < chain_attr_count; idx2++) {
				chain_attr = &(btc->profile_array[idx]->chain_attr[idx2]);
				chain_attr->length =
					STRUCT_SIZE_THROUGH(chain_attr, tx_pwr_weak_rssi);
			}
		}
	}

	(void)obj_registry_ref(wlc->objr, OBJR_BTC_PROFILE_INFO);
	return BCME_OK;
}

/* All UCM related detach activities go here */
static void
BCMATTACHFN(wlc_btc_ucm_detach)(wlc_btc_info_t *btc)
{
	int idx;
	wlc_info_t *wlc = btc->wlc;
	wlc_btcx_profile_t *profile;
	size_t ucm_profile_sz;
	size_t ucm_array_sz = MAX_UCM_PROFILES*sizeof(*btc->profile_array);
	/* if profile_array was not allocated, then nothing to do */
	if (btc->profile_array) {
		if (obj_registry_unref(wlc->objr, OBJR_BTC_PROFILE_INFO) == 0) {
			obj_registry_set(wlc->objr, OBJR_BTC_PROFILE_INFO, NULL);
			for (idx = 0; idx < MAX_UCM_PROFILES; idx++) {
				profile = btc->profile_array[idx];
				if (profile) {
					ucm_profile_sz = sizeof(*profile)
						+(profile->chain_attr_count)
						*sizeof(profile->chain_attr[0]);
					MFREE(wlc->osh, profile, ucm_profile_sz);
				}
			}
			MFREE(wlc->osh, btc->profile_array, ucm_array_sz);
		}
	}
}
#endif /* WL_UCM */

/* BTCX Wl up callback */
static int
wlc_btc_wlc_up(void *ctx)
{
	wlc_btc_info_t *btc = (wlc_btc_info_t *)ctx;
	wlc_info_t *wlc = btc->wlc;

	if (!btc->bt_shm_addr)
		btc->bt_shm_addr = 2 * wlc_bmac_read_shm(wlc->hw, M_BTCX_BLK_PTR(wlc));

	return BCME_OK;
}

/* BTCX Wl down callback */
static int
wlc_btc_wlc_down(void *ctx)
{
	wlc_btc_info_t *btc = (wlc_btc_info_t *)ctx;
	wlc_info_t *wlc = btc->wlc;

	BCM_REFERENCE(wlc);
	btc->bt_shm_addr = 0;



	return BCME_OK;
}

static int
wlc_btc_mode_set(wlc_info_t *wlc, int int_val)
{
	int err = wlc_bmac_btc_mode_set(wlc->hw, int_val);
	wlc->btch->mode = wlc_bmac_btc_mode_get(wlc->hw);
	return err;
}

int
wlc_btc_mode_get(wlc_info_t *wlc)
{
	return wlc->btch->mode;
}

int
wlc_btcx_desense(wlc_btc_info_t *btc, int band)
{
	int i;
	wlc_info_t *wlc = (wlc_info_t *)btc->wlc;
	int btc_mode = wlc_btc_mode_get(wlc);

	/* Dynamic restaging of rxgain for BTCoex */
	if (!SCAN_IN_PROGRESS(wlc->scan) &&
	    btc->bth_active &&
	    (wlc->cfg->link->rssi != WLC_RSSI_INVALID)) {
		if (!btc->restage_rxgain_active &&
		    ((BAND_5G(band) &&
		      ((btc->restage_rxgain_force &
			BTC_RXGAIN_FORCE_5G_MASK) == BTC_RXGAIN_FORCE_5G_ON)) ||
		     (BAND_2G(band) &&
		      ((btc->restage_rxgain_force &
			BTC_RXGAIN_FORCE_2G_MASK) == BTC_RXGAIN_FORCE_2G_ON) &&
		      (!btc->restage_rxgain_on_rssi_thresh ||
		       (btc_mode == WL_BTC_DISABLE) ||
		       (btc->restage_rxgain_on_rssi_thresh &&
		       ((btc_mode == WL_BTC_HYBRID) || (btc_mode == WL_BTC_FULLTDM)) &&
			(-wlc->cfg->link->rssi < btc->restage_rxgain_on_rssi_thresh)))))) {
			if ((i = wlc_iovar_setint(wlc, "phy_btc_restage_rxgain",
				btc->restage_rxgain_level)) == BCME_OK) {
				btc->restage_rxgain_active = 1;
			}
			WL_BTCPROF(("wl%d: BTC restage rxgain (%x) ON: RSSI %d "
				"Thresh -%d, bt %d, (err %d)\n",
				wlc->pub->unit, wlc->stf->rxchain, wlc->cfg->link->rssi,
				btc->restage_rxgain_on_rssi_thresh,
				btc->bth_active, i));
		}
		else if (btc->restage_rxgain_active &&
			((BAND_5G(band) &&
			((btc->restage_rxgain_force &
			BTC_RXGAIN_FORCE_5G_MASK) == BTC_RXGAIN_FORCE_OFF)) ||
			(BAND_2G(band) &&
			(((btc->restage_rxgain_force &
			BTC_RXGAIN_FORCE_2G_MASK) == BTC_RXGAIN_FORCE_OFF) ||
			(btc->restage_rxgain_off_rssi_thresh &&
			((btc_mode == WL_BTC_HYBRID) || (btc_mode == WL_BTC_FULLTDM)) &&
			(-wlc->cfg->link->rssi > btc->restage_rxgain_off_rssi_thresh)))))) {
			  if ((i = wlc_iovar_setint(wlc, "phy_btc_restage_rxgain", 0)) == BCME_OK) {
				btc->restage_rxgain_active = 0;
			  }
			  WL_BTCPROF(("wl%d: BTC restage rxgain (%x) OFF: RSSI %d "
				"Thresh -%d, bt %d, (err %d)\n",
				wlc->pub->unit, wlc->stf->rxchain, wlc->cfg->link->rssi,
				btc->restage_rxgain_off_rssi_thresh,
				btc->bth_active, i));
		}
	} else if (btc->restage_rxgain_active) {
		if ((i = wlc_iovar_setint(wlc, "phy_btc_restage_rxgain", 0)) == BCME_OK) {
			btc->restage_rxgain_active = 0;
		}
		WL_BTCPROF(("wl%d: BTC restage rxgain (%x) OFF: RSSI %d bt %d (err %d)\n",
			wlc->pub->unit, wlc->stf->rxchain, wlc->cfg->link->rssi,
			btc->bth_active, i));
	}

	return BCME_OK;
}

#ifdef WLBTCPROF
#ifdef WLBTCPROF_EXT
int
wlc_btcx_set_ext_profile_param(wlc_info_t *wlc)
{
	wlc_btc_info_t *btch = wlc->btch;
	int err = BCME_ERROR;
	wlc_btc_profile_t *select_profile;

	select_profile = &btch->btc_profile[BTC_PROFILE_2G].select_profile;
	/* program bt and wlan threshold and hysteresis data */
	btch->prot_btrssi_thresh = -1 * select_profile->btc_btrssi_thresh;
	btch->btrssi_hyst = select_profile->btc_btrssi_hyst;
	btch->prot_rssi_thresh = -1 * select_profile->btc_wlrssi_thresh;
	btch->wlrssi_hyst = select_profile->btc_wlrssi_hyst;
	if (wlc->clk) {
		wlc_btc_set_ps_protection(wlc, wlc->cfg); /* enable */

		if (select_profile->btc_num_desense_levels == MAX_BT_DESENSE_LEVELS_4350) {
			err = wlc_phy_btc_set_max_siso_resp_pwr(WLC_PI(wlc),
				&select_profile->btc_max_siso_resp_power[0],
				MAX_BT_DESENSE_LEVELS_4350);
		}
	}

	return err;
}
#endif /* WLBTCPROF_EXT */
int
wlc_btcx_set_btc_profile_param(struct wlc_info *wlc, chanspec_t chanspec, bool force)
{
	int err = BCME_OK;
	wlc_btc_profile_t *btc_cur_profile, *btc_prev_profile;
	wlc_btc_info_t *btch = wlc->btch;
	int btc_inactive_offset[WL_NUM_TXCHAIN_MAX] = {0, 0, 0, 0};

	int band = CHSPEC_IS2G(chanspec) ? WLC_BAND_2G : WLC_BAND_5G;

	/* "Disable Everything" profile for scanning */
	static wlc_btc_profile_t btc_scan_profile;
	int btc_scan_profile_init_done = 0;
	if (btc_scan_profile_init_done == 0) {
		btc_scan_profile_init_done = 1;
		bzero(&btc_scan_profile, sizeof(btc_scan_profile));
		btc_scan_profile.mode = WL_BTC_HYBRID;
		btc_scan_profile.desense_level = 0;
		btc_scan_profile.chain_power_offset[0] = btc_inactive_offset;
		btc_scan_profile.chain_ack[0] = wlc->stf->hw_txchain;
		btc_scan_profile.num_chains = WLC_BITSCNT(wlc->stf->hw_txchain);
	}

	if (!btch)
	{
		WL_INFORM(("%s invalid btch\n", __FUNCTION__));
		err = BCME_ERROR;
		goto finish;
	}

	if (!btch->btc_prev_connect)
	{
		WL_INFORM(("%s invalid btc_prev_connect\n", __FUNCTION__));
		err = BCME_ERROR;
		goto finish;
	}

	if (!btch->btc_profile)
	{
		WL_INFORM(("%s invalid btc_profile\n", __FUNCTION__));
		err = BCME_ERROR;
		goto finish;
	}

	if (band == WLC_BAND_2G)
	{
		if (!force) {
			if (btch->btc_profile[BTC_PROFILE_2G].enable == BTC_PROFILE_OFF)
				goto finish;

			if (btch->btc_prev_connect->prev_band == WLC_BAND_2G)
				goto finish;

			if ((btch->btc_prev_connect->prev_2G_mode == BTC_PROFILE_DISABLE) &&
			(btch->btc_profile[BTC_PROFILE_2G].enable == BTC_PROFILE_DISABLE))
				goto finish;
		}

		btc_cur_profile = &btch->btc_profile[BTC_PROFILE_2G].select_profile;
		btc_prev_profile = &btch->btc_prev_connect->prev_2G_profile;
		if (btch->btc_profile[BTC_PROFILE_2G].enable == BTC_PROFILE_DISABLE)
		{
			btch->btc_prev_connect->prev_2G_mode = BTC_PROFILE_DISABLE;
			memset(btc_cur_profile, 0, sizeof(wlc_btc_profile_t));
			btc_cur_profile->btc_wlrssi_thresh = BTC_WL_RSSI_DEFAULT;
			btc_cur_profile->btc_btrssi_thresh = BTC_BT_RSSI_DEFAULT;
			if (CHIPID(wlc->pub->sih->chip) == BCM4350_CHIP_ID) {
				btc_cur_profile->btc_wlrssi_hyst = BTC_WL_RSSI_HYST_DEFAULT_4350;
				btc_cur_profile->btc_btrssi_hyst = BTC_BT_RSSI_HYST_DEFAULT_4350;
				btc_cur_profile->btc_max_siso_resp_power[0] =
					BTC_WL_MAX_SISO_RESP_POWER_TDD_DEFAULT_4350;
				btc_cur_profile->btc_max_siso_resp_power[1] =
					BTC_WL_MAX_SISO_RESP_POWER_HYBRID_DEFAULT_4350;
			}
		}
		else
		{
			btch->btc_prev_connect->prev_2G_mode = BTC_PROFILE_ENABLE;
		}

		btch->btc_prev_connect->prev_band = WLC_BAND_2G;
	}
	else
	{
		if (!force) {
			if (btch->btc_profile[BTC_PROFILE_5G].enable == BTC_PROFILE_OFF)
				goto finish;

			if (btch->btc_prev_connect->prev_band == WLC_BAND_5G)
				goto finish;

			if ((btch->btc_prev_connect->prev_5G_mode == BTC_PROFILE_DISABLE) &&
			(btch->btc_profile[BTC_PROFILE_5G].enable == BTC_PROFILE_DISABLE))
				goto finish;
		}

		btc_cur_profile = &btch->btc_profile[BTC_PROFILE_5G].select_profile;
		btc_prev_profile = &btch->btc_prev_connect->prev_5G_profile;


		if (btch->btc_profile[BTC_PROFILE_5G].enable == BTC_PROFILE_DISABLE)
		{
			btch->btc_prev_connect->prev_5G_mode = BTC_PROFILE_DISABLE;
			memset(&btch->btc_profile[BTC_PROFILE_5G].select_profile,
				0, sizeof(wlc_btc_profile_t));
		}
		else
		{
			btch->btc_prev_connect->prev_5G_mode = BTC_PROFILE_ENABLE;
		}

		btch->btc_prev_connect->prev_band = WLC_BAND_5G;
		btc_cur_profile = &btch->btc_profile[BTC_PROFILE_5G].select_profile;
		btc_prev_profile = &btch->btc_prev_connect->prev_5G_profile;
	}


	/* New request to disable btc profiles during scan */
	if (SCAN_IN_PROGRESS(wlc->scan)) {
		btc_cur_profile = &btc_scan_profile;
		/* During Scans set btcmode to hybrid
		*  but zero all other coex params
		*/
		btc_cur_profile->mode = WL_BTC_HYBRID;
	}


	/* New request to disable btc profiles during scan */
	if (SCAN_IN_PROGRESS(wlc->scan) && (band == WLC_BAND_2G)) {
		btc_cur_profile = &btc_scan_profile;
	}


	WL_BTCPROF(("%s chanspec 0x%x\n", __FUNCTION__, chanspec));

	/* setBTCOEX_MODE */
	err = wlc_btc_mode_set(wlc, btc_cur_profile->mode);
	WL_BTCPROF(("btc mode %d\n", btc_cur_profile->mode));
	if (err)
	{
		err = BCME_ERROR;
		goto finish;
	}

	/* setDESENSE_LEVEL */
	btch->restage_rxgain_level = btc_cur_profile->desense_level;
	WL_BTCPROF(("btc desense level %d\n", btc_cur_profile->desense_level));

	/* setDESENSE */
	btch->restage_rxgain_force =
		(btch->btc_profile[BTC_PROFILE_2G].select_profile.desense)?
		BTC_RXGAIN_FORCE_2G_ON : 0;
	btch->restage_rxgain_force |=
		(btch->btc_profile[BTC_PROFILE_5G].select_profile.desense)?
		BTC_RXGAIN_FORCE_5G_ON : 0;
	WL_BTCPROF(("btc rxgain force 0x%x\n", btch->restage_rxgain_force));

	/* setting 2G thresholds, 5G thresholds are not used */
	btch->restage_rxgain_on_rssi_thresh =
		(uint8)((btch->btc_profile[BTC_PROFILE_2G].select_profile.desense_thresh_high *
		-1) & 0xFF);
	btch->restage_rxgain_off_rssi_thresh =
		(uint8)((btch->btc_profile[BTC_PROFILE_2G].select_profile.desense_thresh_low *
		-1) & 0xFF);
	WL_BTCPROF(("btc rxgain on 0x%x rxgain off 0x%x\n",
		btch->restage_rxgain_on_rssi_thresh,
		btch->restage_rxgain_off_rssi_thresh));

	/* check the state of bt_active signal */
		if (BT3P_HW_COEX(wlc) && wlc->clk) {
			wlc_bmac_btc_period_get(wlc->hw, &btch->bth_period,
				&btch->bth_active, &btch->agg_off_bm, &wlc->btch->acl_last_ts,
				&wlc->btch->a2dp_last_ts);
		}

	/* apply desense settings */
	wlc_btcx_desense(btch, band);

	/* setChain_Ack */
	wlc_btc_siso_ack_set(wlc, (int16)btc_cur_profile->chain_ack[0], TRUE);
	WL_BTCPROF(("btc chain ack 0x%x num chains %d\n",
		btc_cur_profile->chain_ack[0],
		btc_cur_profile->num_chains));

	/* setTX_CHAIN_POWER */
	if (btch->bth_active) {
	wlc_channel_set_tx_power(wlc, band, btc_cur_profile->num_chains,
		&btc_cur_profile->chain_power_offset[0],
		&btc_prev_profile->chain_power_offset[0]);
	*btc_prev_profile = *btc_cur_profile;
	} else {
		wlc_channel_set_tx_power(wlc, band, btc_cur_profile->num_chains,
			&btc_inactive_offset[0], &btc_prev_profile->chain_power_offset[0]);
	}

#ifdef WLBTCPROF_EXT
	/* set hybrid params */
	if ((band == WLC_BAND_2G) && (CHIPID(wlc->pub->sih->chip) == BCM4350_CHIP_ID)) {
		if ((err = wlc_btcx_set_ext_profile_param(wlc)) == BCME_ERROR) {
			WL_INFORM(("%s Unable to program siso ack powers\n", __FUNCTION__));
		}
	} else {
		for (i = 0; i < MAX_BT_DESENSE_LEVELS; i++) {
			btc_cur_profile->btc_max_siso_resp_power[i] =
				BTC_WL_MAX_SISO_RESP_POWER_TDD_DEFAULT;
		}

		if ((btc_cur_profile->btc_num_desense_levels == MAX_BT_DESENSE_LEVELS_4350) &&
			wlc->clk) {
			err = wlc_phy_btc_set_max_siso_resp_pwr(WLC_PI(wlc),
				&btc_cur_profile->btc_max_siso_resp_power[0],
				MAX_BT_DESENSE_LEVELS_4350);
		}
	}
#endif /* WLBTCPROF_EXT */

finish:
	return err;
}

int
wlc_btcx_select_profile_set(wlc_info_t *wlc, uint8 *pref, int len)
{
	wlc_btc_info_t *btch;

	btch = wlc->btch;
	if (!btch)
	{
		WL_INFORM(("%s invalid btch\n", __FUNCTION__));
		return BCME_ERROR;
	}

	if (pref)
	{
		if (!bcmp(pref, btch->btc_profile, len))
			return BCME_OK;

		bcopy(pref, btch->btc_profile, len);

		if (wlc_btcx_set_btc_profile_param(wlc, wlc->chanspec, TRUE))
		{
			WL_ERROR(("wl%d: %s: setting btc profile first time error: chspec %d!\n",
				wlc->pub->unit, __FUNCTION__, CHSPEC_CHANNEL(wlc->chanspec)));
		}

		return BCME_OK;
	}

	return BCME_ERROR;
}

int
wlc_btcx_select_profile_get(wlc_info_t *wlc, uint8 *pref, int len)
{
	wlc_btc_info_t *btch = wlc->btch;

	if (!btch)
	{
		WL_INFORM(("%s invalid btch\n", __FUNCTION__));
		return BCME_ERROR;
	}

	if (pref)
	{
		bcopy(btch->btc_profile, pref, len);
		return BCME_OK;
	}

	return BCME_ERROR;
}
#endif /* WLBTCPROF */

int
wlc_btc_siso_ack_set(wlc_info_t *wlc, int16 siso_ack, bool force)
{
	wlc_btc_info_t *btch = wlc->btch;

	if (!btch)
		return BCME_ERROR;

	if (force) {
		if (siso_ack == AUTO)
			btch->siso_ack_ovr = FALSE;
		else {
			/* sanity check forced value */
			if (!(siso_ack & TXCOREMASK))
				return BCME_BADARG;
			btch->siso_ack = siso_ack;
			btch->siso_ack_ovr = TRUE;
		}
	}

	if (!btch->siso_ack_ovr) {
		/* no override, set siso_ack according to btc_mode/chipids/boardflag etc. */
		if (siso_ack == AUTO) {
			siso_ack = 0;
			if (wlc->pub->boardflags & BFL_FEM_BT) {
				/* check boardflag: antenna shared w BT */
				/* futher check srom, nvram */
				if (wlc->hw->btc->btcx_aa == 0x3) { /* two antenna */
					if (wlc->pub->boardflags2 &
					    BFL2_BT_SHARE_ANT0) { /* core 0 shared */
						siso_ack = 0x2;
					} else {
						siso_ack = 0x1;
					}
				} else if (wlc->hw->btc->btcx_aa == 0x7) { /* three antenna */
					; /* not supported yet */
				}
			}
		}
		btch->siso_ack = siso_ack;
	}

	if (wlc->clk) {
		wlc_bmac_write_shm(wlc->hw, M_COREMASK_BTRESP(wlc), (uint16)siso_ack);
	}

	return BCME_OK;
}

int16
wlc_btc_siso_ack_get(wlc_info_t *wlc)
{
	return wlc->btch->siso_ack;
}

static int
wlc_btc_wire_set(wlc_info_t *wlc, int int_val)
{
	int err;
	err = wlc_bmac_btc_wire_set(wlc->hw, int_val);
	wlc->btch->wire = wlc_bmac_btc_wire_get(wlc->hw);
	return err;
}

int
wlc_btc_wire_get(wlc_info_t *wlc)
{
	return wlc->btch->wire;
}

void wlc_btc_mode_sync(wlc_info_t *wlc)
{
	wlc->btch->mode = wlc_bmac_btc_mode_get(wlc->hw);
	wlc->btch->wire = wlc_bmac_btc_wire_get(wlc->hw);
}

uint8 wlc_btc_save_host_requested_pm(wlc_info_t *wlc, uint8 val)
{
	return (wlc->btch->host_requested_pm = val);
}

bool wlc_btc_get_bth_active(wlc_info_t *wlc)
{
	return wlc->btch->bth_active;
}

uint16 wlc_btc_get_bth_period(wlc_info_t *wlc)
{
	return wlc->btch->bth_period;
}

static int
wlc_btc_flags_idx_set(wlc_info_t *wlc, int int_val, int int_val2)
{
	return wlc_bmac_btc_flags_idx_set(wlc->hw, int_val, int_val2);
}

static int
wlc_btc_flags_idx_get(wlc_info_t *wlc, int int_val)
{
	return wlc_bmac_btc_flags_idx_get(wlc->hw, int_val);
}

int
wlc_btc_params_set(wlc_info_t *wlc, int int_val, int int_val2)
{
	return wlc_bmac_btc_params_set(wlc->hw, int_val, int_val2);
}

int
wlc_btc_params_get(wlc_info_t *wlc, int int_val)
{
	return wlc_bmac_btc_params_get(wlc->hw, int_val);
}

#ifdef WLBTCPROF_EXT
int wlc_btc_profile_set(wlc_info_t *wlc, int int_val, int iovar_id)
{
	wlc_btc_info_t *btch = wlc->btch;
	wlc_btc_profile_t *select_profile;

	select_profile = &btch->btc_profile[BTC_PROFILE_2G].select_profile;
	switch (iovar_id)
	{
	case IOV_BTC_WLRSSI_THRESH:
		select_profile->btc_wlrssi_thresh = (int8) int_val;
		break;
	case IOV_BTC_WLRSSI_HYST:
		select_profile->btc_wlrssi_hyst = (uint8) int_val;
		break;
	case IOV_BTC_BTRSSI_THRESH:
		select_profile->btc_btrssi_thresh = (int8) int_val;
		break;
	case IOV_BTC_BTRSSI_HYST:
		select_profile->btc_btrssi_hyst = (uint8) int_val;
		break;
	default:
		break;
	}
	return 1;
}

int wlc_btc_profile_get(wlc_info_t *wlc, int iovar_id)
{
	wlc_btc_info_t *btch = wlc->btch;
	wlc_btc_profile_t *select_profile;

	select_profile = &btch->btc_profile[BTC_PROFILE_2G].select_profile;
	switch (iovar_id)
	{
	case IOV_BTC_WLRSSI_THRESH:
		return select_profile->btc_wlrssi_thresh;
		break;
	case IOV_BTC_WLRSSI_HYST:
		return select_profile->btc_wlrssi_hyst;
		break;
	case IOV_BTC_BTRSSI_THRESH:
		return select_profile->btc_btrssi_thresh;
		break;
	case IOV_BTC_BTRSSI_HYST:
		return select_profile->btc_btrssi_hyst;
		break;
	default:
		break;
	}
	return 1;
}
#endif /* WLBTCPROF_EXT */

static void
wlc_btc_stuck_war50943(wlc_info_t *wlc, bool enable)
{
	wlc_bmac_btc_stuck_war50943(wlc->hw, enable);
}

static void
wlc_btc_rssi_threshold_get(wlc_info_t *wlc)
{
	wlc_bmac_btc_rssi_threshold_get(wlc->hw,
		&wlc->btch->prot_rssi_thresh,
		&wlc->btch->high_threshold,
		&wlc->btch->low_threshold);
}

#ifdef WLAMPDU
static void
wlc_ampdu_agg_state_update_rx_all_but_AWDL(wlc_info_t *wlc, bool aggr)
{
	int idx;
	wlc_bsscfg_t *cfg;

	FOREACH_BSS(wlc, idx, cfg) {
		if (BSSCFG_AWDL(wlc, cfg)) {
			continue;
		}
		wlc_ampdu_rx_set_bsscfg_aggr_override(wlc->ampdu_rx, cfg, aggr);
	}
}
#endif /* WLAMPDU */

uint
wlc_btc_frag_threshold(wlc_info_t *wlc, struct scb *scb)
{
	ratespec_t rspec;
	uint rate, thresh;
	wlc_bsscfg_t *cfg;

	/* Make sure period is known */
	if (wlc->btch->bth_period == 0)
		return 0;

	ASSERT(scb != NULL);

	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg != NULL);

	/* if BT SCO is ongoing, packet length should not exceed 1/2 of SCO period */
	rspec = wlc_get_rspec_history(cfg);
	rate = RSPEC2KBPS(rspec);

	/*  use one half of the duration as threshold.  convert from usec to bytes */
	/* thresh = (bt_period * rate) / 1000 / 8 / 2  */
	thresh = (wlc->btch->bth_period * rate) >> 14;

	if (thresh < DOT11_MIN_FRAG_LEN)
		thresh = DOT11_MIN_FRAG_LEN;
	return thresh;
}

#if defined(STA) && defined(BTCX_PM0_IDLE_WAR)
static void
wlc_btc_pm_adjust(wlc_info_t *wlc,  bool bt_active)
{
	wlc_bsscfg_t *cfg = wlc->cfg;
	/* only bt is not active, set PM to host requested mode */
	if (wlc->btch->host_requested_pm != PM_FORCE_OFF) {
		if (bt_active) {
				if (PM_OFF == wlc->btch->host_requested_pm &&
				cfg->pm->PM != PM_FAST)
				wlc_set_pm_mode(wlc, PM_FAST, cfg);
		} else {
			if (wlc->btch->host_requested_pm != cfg->pm->PM)
				wlc_set_pm_mode(wlc, wlc->btch->host_requested_pm, cfg);
		}
	}
}
#endif /* STA */

#ifdef STA
static bool
wlc_btcx_check_port_open(wlc_info_t *wlc)
{
	uint idx;
	wlc_bsscfg_t *cfg;

	FOREACH_AS_STA(wlc, idx, cfg) {
		if (!cfg->BSS)
			continue;
		if (!WLC_PORTOPEN(cfg))
			return (FALSE);
	}
	return (TRUE);
}
#endif  /* STA */

void
wlc_btc_set_ps_protection(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	BCM_REFERENCE(bsscfg);
	if (MCHAN_ACTIVE(wlc->pub))
		return;

	if ((wlc_btc_wire_get(wlc) >= WL_BTC_3WIRE) &&
		wlc_btc_mode_get(wlc)) {
		wlc_bsscfg_t *pc;
		int btc_flags = wlc_bmac_btc_flags_get(wlc->hw);
		uint16 protections;
		uint16 active = 0;
		uint16 ps;

		pc = wlc->cfg;
		BCM_REFERENCE(pc);

		/* if radio is disable, driver may be down, quit here */
		if (wlc->pub->radio_disabled || !wlc->pub->up)
			return;

#if defined(STA) && !defined(BCMNODOWN)
		/* ??? if ismpc, driver should be in down state if up/down is allowed */
		if (wlc->mpc && wlc_ismpc(wlc))
			return;
#endif

#ifdef WL_BTCDYN
		if (BTCDYN_ENAB(wlc->pub) && !IS_DYNCTL_ON(wlc->btch->btcdyn->dprof)) {
#endif
			/* enable protection for hybrid mode when rssi below certain threshold */
			/* the implicit switching does not change btc_mode or PS protection */
			if (wlc->btch->prot_rssi_thresh &&
				-wlc->cfg->link->rssi > wlc->btch->prot_rssi_thresh) {
				active = MHF3_BTCX_ACTIVE_PROT;
			}

			/* enable protection if bt rssi < threshold */
			if (D11REV_IS(wlc->pub->corerev, 48) &&
				wlc->btch->prot_btrssi_thresh &&
				wlc->btch->bt_rssi <
				wlc->btch->prot_btrssi_thresh) {
				active = MHF3_BTCX_ACTIVE_PROT;
			}
#ifdef WL_BTCDYN
		}
#endif

		if (btc_flags & WL_BTC_FLAG_ACTIVE_PROT) {
			active = MHF3_BTCX_ACTIVE_PROT;
		}

		ps = (btc_flags & WL_BTC_FLAG_PS_PROTECT) ? MHF3_BTCX_PS_PROTECT : 0;
		BCM_REFERENCE(ps);

#ifdef STA
		/* Enable PS protection when there is only one STA/GC connection */
		if ((wlc->stas_connected == 1) && (wlc->aps_associated == 0) &&
			(wlc_ap_stas_associated(wlc->ap) == 0) &&
			(wlc->ibss_bsscfgs == 0) && !AWDL_ENAB(wlc->pub)) {
				/* when WPA/PSK security is enabled
				  * wait until WLC_PORTOPEN() is TRUE
				  */
				if (wlc_btcx_check_port_open(wlc)) {
					protections = active | ps;
				} else {
					protections = 0;
				}
		}
		else if (wlc->stas_connected > 0)
			protections = active;
		else
#endif /* STA */
#ifdef AP
		if (wlc->aps_associated > 0 && wlc_ap_stas_associated(wlc->ap) > 0)
			protections = active;
		/* No protection */
		else
#endif /* AP */
			if (AWDL_ENAB(wlc->pub))
				protections = active;
			else
				protections = 0;

		wlc_mhf(wlc, MHF3, MHF3_BTCX_ACTIVE_PROT | MHF3_BTCX_PS_PROTECT,
		        protections, WLC_BAND_2G);
#ifdef WLMCNX
		/*
		For non-VSDB the only time we turn on PS protection is when there is only
		one STA associated - primary or GC. In this case, set the BSS index in
		designated SHM location as well.
		*/
		if ((MCNX_ENAB(wlc->pub)) && (protections & ps)) {
			uint idx;
			wlc_bsscfg_t *cfg;
			int bss_idx;

			FOREACH_AS_STA(wlc, idx, cfg) {
				if (!cfg->BSS)
					continue;
				bss_idx = wlc_mcnx_BSS_idx(wlc->mcnx, cfg);
				wlc_mcnx_shm_bss_idx_set(wlc->mcnx, bss_idx);
				break;
			}
		}
#endif /* WLMCNX */
	}
}

static int wlc_dump_btcx(void *ctx, struct bcmstrbuf *b)
{
	wlc_info_t *wlc = ctx;
	uint8 idx, offset;
	uint16 hi, lo;
	uint32 buff[C_BTCX_DBGBLK_SZ/2];
	uint16 base;

	if (!wlc->clk) {
		return BCME_NOCLK;
	}

	base = D11REV_LT(wlc->pub->corerev, 40) ?
	        M_BTCXDBG_BLK(wlc) :
	        ((wlc_bmac_read_shm(wlc->hw, M_PSM2HOST_EXT_PTR(wlc)) + 0xd) << 1);

	for (idx = 0; idx < C_BTCX_DBGBLK_SZ; idx += 2) {
		offset = idx*2;
		lo = wlc_bmac_read_shm(wlc->hw, base+offset);
		hi = wlc_bmac_read_shm(wlc->hw, base+offset+2);
		buff[idx>>1] = (hi<<16) | lo;
	}

	bcm_bprintf(b, "nrfact: %u, ntxconf: %u (%u%%), txconf_durn(us): %u\n",
		buff[0], buff[1], buff[0] ? (buff[1]*100)/buff[0]: 0, buff[2]);
	return 0;
}

static int wlc_clr_btcxdump(void *ctx)
{
	wlc_info_t *wlc = ctx;
	uint16 base;

	if (!wlc->clk) {
		return BCME_NOCLK;
	}

	base = D11REV_LT(wlc->pub->corerev, 40) ?
	        M_BTCXDBG_BLK(wlc) :
	        ((wlc_bmac_read_shm(wlc->hw, M_PSM2HOST_EXT_PTR(wlc)) + 0xd) << 1);
	wlc_bmac_set_shm(wlc->hw, base, 0, C_BTCX_DBGBLK_SZ*2);
	return BCME_OK;
}

/* Read relevant BTC params to determine if aggregation has to be enabled/disabled */
void
wlc_btcx_read_btc_params(wlc_info_t *wlc)
{
	wlc_btc_info_t *btch = wlc->btch;

	if (BT3P_HW_COEX(wlc) && wlc->clk) {
		wlc_bmac_btc_period_get(wlc->hw, &btch->bth_period,
			&btch->bth_active, &btch->agg_off_bm, &btch->acl_last_ts,
			&btch->a2dp_last_ts);
		if (!btch->bth_active && btch->btrssi_cnt) {
			wlc_btc_reset_btrssi(btch);
		}
	}
}

#ifdef WLRSDB
void
wlc_btcx_update_coex_iomask(wlc_info_t *wlc)
{
}

bool
wlc_btcx_rsdb_update_bt_chanspec(wlc_info_t *wlc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	wlc_cmn_info_t* wlc_cmn = wlc->cmn;
	wlc_info_t *other_wlc;
	int idx;

	if (wlc_rsdb_mode(wlc) == PHYMODE_RSDB) {
		FOREACH_WLC(wlc_cmn, idx, other_wlc) {
			if (wlc != other_wlc) {
				if (CHSPEC_IS5G(wlc_hw->chanspec) &&
					CHSPEC_IS2G(other_wlc->hw->chanspec)) {
					/* don't update chanspec to bt since one core is in 2G
					  * and other core in 5G.
					  * 2G core's chanspec should get updated to BT
					  */
					return FALSE;
				} else {
					/* update chanspec to bt */
					return TRUE;
				}
			}
		}
	} else {
		/* update chanspec to bt */
		return TRUE;
	}
	return TRUE;
}

#endif /* WLRSDB */

static void
wlc_btc_update_sco_params(wlc_info_t *wlc)
{
	uint16 holdsco_limit, grant_ratio;
	bool low_threshold = TRUE;

	if (BT3P_HW_COEX(wlc) && BAND_2G(wlc->band->bandtype)) {
		if ((wlc->btch->bth_period) && (!wlc->btch->sco_threshold_set)) {
			uint16 sco_thresh = (uint16)wlc_btc_params_get(wlc,
				BTC_FW_HOLDSCO_HI_THRESH);
			uint16 btcx_config = (uint16)wlc_btc_params_get(wlc,
				M_BTCX_CONFIG_OFFSET(wlc) >> 1);

			// Use High thresholds if BT period is higher than configured
			// threshold or if Wlan RSSI is low
			if ((sco_thresh && (wlc->btch->bth_period > sco_thresh)) ||
				(btcx_config & C_BTCX_CONFIG_LOW_RSSI)) {
				low_threshold = FALSE;
			}
			wlc->btch->sco_threshold_set = TRUE;
		} else if ((!wlc->btch->bth_period) && (wlc->btch->sco_threshold_set)) {
			wlc->btch->sco_threshold_set = FALSE;
		} else {
			return;
		}

		if (low_threshold) {
			holdsco_limit = (uint16)wlc_btc_params_get(wlc, BTC_FW_HOLDSCO_LIMIT);
			grant_ratio = (uint16)wlc_btc_params_get(wlc,
				BTC_FW_SCO_GRANT_HOLD_RATIO);
		} else {
			holdsco_limit = (uint16)wlc_btc_params_get(wlc, BTC_FW_HOLDSCO_LIMIT_HI);
			grant_ratio = (uint16)wlc_btc_params_get(wlc,
				BTC_FW_SCO_GRANT_HOLD_RATIO_HI);
		}
		wlc_btc_params_set(wlc, M_BTCX_HOLDSCO_LIMIT_OFFSET(wlc) >> 1, holdsco_limit);
		wlc_btc_params_set(wlc, M_BTCX_SCO_GRANT_HOLD_RATIO_OFFSET(wlc) >> 1, grant_ratio);
	}
}

#ifdef WLAMPDU
/*
 * Function: wlc_btcx_debounce
 * Description: COEX status bits from shmem may be inconsistent when sampled once a second
 * and may require debouncing.  An example is BLE bit in agg_off_bm, which is '1' when a
 * BLE transaction occurred within 20ms of the last transaction, and '0' when the BLE
 * transactions were more than 20ms apart.  Rather than put debounce logic in ucode, it
 * is more efficient to put in in FW.

 * This routine requires a structure to be passed in with the following arguments:
 * inputs: current, on_timedelta, off_timedelta.
 * outputs: status_debounced, status_cntr, timeStamp.
 *
 * If current is consistently set to '1' for 5 seconds, then status_debounced will
 * bet set.  Once status_debounced is '1', then current will need to be '0' consistently
 * for 5 seconds in order for status_debounced to be cleared to '0'.  Note that the '5 second'
 * is configurable via the timedelta arguments passed in.
 */
static uint16
wlc_btcx_debounce(wlc_btc_debounce_info_t *btc_debounce, uint16 curr)
{
	uint8 on_timedelta = btc_debounce->on_timedelta;
	uint8 off_timedelta = btc_debounce->off_timedelta;
	uint32 curr_time = OSL_SYSUPTIME() / MSECS_PER_SEC;

	switch ((btc_debounce->status_debounced << 2) |
		(btc_debounce->status_inprogress << 1) | (curr & 1)) {
		case 0x0:
		case 0x7:
			btc_debounce->status_timestamp = curr_time;
			break;
		case 0x1:
		case 0x5:
		case 0x2:
		case 0x6:
			btc_debounce->status_inprogress = curr;
			btc_debounce->status_timestamp = curr_time;
			break;
		case 0x3:
			if ((curr_time - btc_debounce->status_timestamp) > on_timedelta) {
				btc_debounce->status_debounced = 1;
				btc_debounce->status_timestamp = curr_time;
			}
			break;
		case 0x4:
			if ((curr_time - btc_debounce->status_timestamp) > off_timedelta) {
				btc_debounce->status_debounced = 0;
				btc_debounce->status_timestamp = curr_time;
			}
			break;
		default:
			break;
	}
	return btc_debounce->status_debounced;
}

/* Function: wlc_btcx_evaluate_agg_state
 * Description: Determine proper aggregation settings based on the
 * BT/LTE COEX Scenario. There are 2 possible scenarios related to ucode.
 * 1. The corresponding change is present in ucode. In this case the SHM
 * parameters M_BTCX_HOLDSCO_LIMIT & M_BTCX_SCO_GRANT_HOLD_RATIO are initialized
 * to the low thresholds and will get reset based on BT Period once this function
 * runs.
 * 2. The corresponding change is not present in ucode. In this case, ucode uses
 * either low or high threshold parameters as needed. This function will set
 * the low threshold parameters to low or high values as needed. When it sets
 * the low threshold parameters to high values, it has no side-effect since ucode
 * will use the high threshold parameters.
 */
static void
wlc_btcx_evaluate_agg_state(wlc_btc_info_t *btc)
{
	wlc_info_t *wlc = (wlc_info_t *)btc->wlc;
	int btc_mode = wlc_btc_mode_get(wlc);
	uint16 ble_debounced = 0, a2dp_debounced = 0, sco_active = 0, siso_mode = 0;
	uint8 hybrid_mode = 0; // btc_mode 5
	uint8 explicit_tdm_mode = 0; // btc_mode 1
	uint8 implicit_tdm_mode = 0; // btc_mode 5 operating in tdd mode
	uint8 implicit_hybrid_mode = 0; // btc_mode 5 with simrx and limited tx

	if (BT3P_HW_COEX(wlc) && BAND_2G(wlc->band->bandtype)) {
		uint8 agg_state_req;

		if (btc_mode && btc_mode != WL_BTC_PARALLEL) {

			/* Make sure STA is on the home channel to avoid changing AMPDU
			 * state during scanning
			 */
			if (AMPDU_ENAB(wlc->pub) && !SCAN_IN_PROGRESS(wlc->scan) &&
				wlc->pub->associated) {
				/* Debounce ble status, which flaps frequently depending on
				 * whether the previous BLE request was within 20ms
				 */
				ble_debounced = wlc_btcx_debounce(btc->ble_debounce,
					(wlc->btch->agg_off_bm & C_BTCX_AGGOFF_BLE));

				a2dp_debounced = wlc_btcx_debounce(btc->a2dp_debounce,
					((wlc->btch->a2dp_last_ts) ? 1 : 0));
				sco_active = (wlc->btch->bth_period &&
					(wlc->btch->bth_period < BT_AMPDU_THRESH)) ? 1 : 0;
				/* txstreams==1 indicates chip operating in SISO mode.  */
				siso_mode = (wlc->stf->txstreams == 1) ? 1 : 0;
				hybrid_mode = (btc_mode == WL_BTC_HYBRID) ? 1 : 0;
				explicit_tdm_mode = IS_BTCX_FULLTDM(btc_mode);
				implicit_tdm_mode = wlc_btcx_is_hybrid_in_TDM(wlc);
				implicit_hybrid_mode = (btc_mode == WL_BTC_HYBRID) &&
					!implicit_tdm_mode;

				/* process all bt related disabling/enabling here */
				if (sco_active || a2dp_debounced || ble_debounced) {
					if (sco_active && (explicit_tdm_mode ||
						implicit_tdm_mode)) {
						/* If configured, dynamically adjust Rx
						* aggregation window sizes
						*/
						if (wlc_bmac_btc_params_get(wlc->hw,
							BTC_FW_MOD_RXAGG_PKT_SZ_FOR_SCO)) {
							agg_state_req = BTC_LIMAGG;
						} else {
						/* Turn off Agg for TX & RX for SCO/LEA COEX */
							agg_state_req = BTC_AGGOFF;
						}
					} else if (sco_active && implicit_hybrid_mode) {
						agg_state_req = BTC_DYAGG;

					} else if (((ble_debounced || a2dp_debounced) &&
						((explicit_tdm_mode && siso_mode) ||
						(!siso_mode && explicit_tdm_mode &&
						(wlc_btcx_rssi_for_ble_agg_decision(wlc) < 0)))) ||
						((ble_debounced || a2dp_debounced) &&
						implicit_tdm_mode)) {
						/* There are 3 conditions where we disable AMPDU-RX
						 * except for AWDL:
						 * 1) SISO w/btc_mode 1 and A2DP/BLE are active
						 * 2) MIMO w/btc_mode 1 and A2DP/BLE are active
						 * and WL RSSI is weak
						 * 3) Hybrid when coex mode has switched to TDM
						 */
						agg_state_req = BTC_CLAMPTXAGG_RXAGGOFFEXCEPTAWDL;
					} else if (((ble_debounced || a2dp_debounced) &&
						!siso_mode) &&
						(hybrid_mode || (explicit_tdm_mode &&
						wlc_btcx_rssi_for_ble_agg_decision(wlc) > 0))) {
						/*
						* There are 2 conditions where we enable aggregation
						* 1) MIMO chip in Hybrid mode and A2DP/BLE are
						* active
						* 2) MIMO chip /w btc_mode 1 and A2DP/BLE are active
						* and WL RSSI is strong
						*/
						agg_state_req = BTC_AGGON;
					} else {
						agg_state_req = wlc->btch->agg_state_req;
					}
				} else {
					agg_state_req = BTC_AGGON;
				}

				/* Apply Aggregation modes found above for btc_mode != PARALLEL */
				wlc->btch->agg_state_req = agg_state_req;
			} else {
				agg_state_req = BTC_AGGON;
			}
		} else {
			/* Dynamic BTC mode requires this */
			agg_state_req = BTC_AGGON;
		}
	}
}

/* Function: wlc_btcx_ltecx_apply_agg_state
 * Description: This function also applies the AMPDU settings,
 * as determined by wlc_btcx_evaluate_agg_state() & LTECX states
 */
void
wlc_btcx_ltecx_apply_agg_state(wlc_info_t *wlc)
{
	wlc_btc_info_t *btc = wlc->btch;
	uint16 btc_reagg_en;
	bool btcx_ampdu_dur = FALSE;
	bool dyagg = FALSE;
	bool rx_agg = TRUE, tx_agg = TRUE;
	bool awdl_rx_agg = FALSE;

	if (AMPDU_ENAB(wlc->pub) && !SCAN_IN_PROGRESS(wlc->scan) &&
		wlc->pub->associated) {
		if (BT3P_HW_COEX(wlc) && BAND_2G(wlc->band->bandtype)) {
			switch (btc->agg_state_req) {
				case BTC_AGGOFF:
					tx_agg = FALSE;
					rx_agg = FALSE;
				break;
				case BTC_CLAMPTXAGG_RXAGGOFFEXCEPTAWDL:
					btcx_ampdu_dur = TRUE;
					rx_agg = FALSE;
					awdl_rx_agg = TRUE;
				break;
				case BTC_DYAGG:
					/* IOVAR can override dynamic settting */
					dyagg = TRUE;
				break;
				case BTC_AGGON:
					btc_reagg_en = (uint16)wlc_btc_params_get(wlc,
						BTC_FW_RX_REAGG_AFTER_SCO);
					if (!btc_reagg_en) {
						rx_agg = FALSE;
					}
				break;
				case BTC_LIMAGG:
					wlc_btcx_rx_ba_window_modify(wlc, &rx_agg);
					tx_agg = TRUE;
					dyagg = TRUE;
				break;
				default:
				break;
			}
			wlc_ampdu_btcx_tx_dur(wlc, btcx_ampdu_dur);
			if (wlc->btch->dyagg != AUTO) {
				dyagg = wlc->btch->dyagg; /* IOVAR override */
			}
			wlc_btc_hflg(wlc, dyagg, BTCX_HFLG_DYAGG);
			wlc_btc_params_set(wlc, BTC_FW_AGG_STATE_REQ, wlc->btch->agg_state_req);

			if ((btc->agg_state_req != BTC_LIMAGG) && (wlc->btch->rxagg_resized)) {
				wlc_ampdu_agg_state_update_rx_all(wlc, OFF);
				wlc_btcx_rx_ba_window_reset(wlc);
				WL_INFORM(("BTCX: Revert Rx agg\n"));
			}
		}
		if ((rx_agg) &&
#ifdef BCMLTECOEX
		   (!wlc_ltecx_rx_agg_off(wlc)) &&
#endif
		   (TRUE)) {
			wlc_ampdu_agg_state_update_rx_all(wlc, ON);
		} else {
			if (awdl_rx_agg)
				wlc_ampdu_agg_state_update_rx_all_but_AWDL(wlc, OFF);
			else
				wlc_ampdu_agg_state_update_rx_all(wlc, OFF);
		}
		if ((tx_agg) &&
#ifdef BCMLTECOEX
		   (!wlc_ltecx_tx_agg_off(wlc)) &&
#endif
		   (TRUE)) {
			wlc_ampdu_agg_state_update_tx_all(wlc, ON);
		} else
			wlc_ampdu_agg_state_update_tx_all(wlc, OFF);
	}
}
static bool
wlc_btcx_is_hybrid_in_TDM(wlc_info_t *wlc)
{
	if ((wlc->btch->mode == WL_BTC_HYBRID) && !wlc->btch->simrx_slna &&
		wlc->btch->btrssi_avg) {
		return TRUE;
	}
	return FALSE;
}

/*
 * Function helps determine when to disable aggregation for BLE COEX scenario on MIMO chips
 * Returns negative value when RSSI is weaker than low-threshold (i.e. rssi -80, and
 * low_thresh=-70, return -10)
 * Returns positive value when RSSI is stronger than high-threshold (i.e. rssi -50 and
 * high_thresh=-65, return 15)
 * Returns 0 when rssi is inbetween thresholds.	 (i.e. rssi -68, return 0)
 * If SoftAP, then this funtion always returns 0.
 * if thresh = -70, then a new variables are created in this routine called high_thresh, at -65.
 */
static int
wlc_btcx_rssi_for_ble_agg_decision(wlc_info_t *wlc)
{
	int retval;
	int rssi = wlc->cfg->link->rssi;
	uint16 btc_rssi_low_thresh, btc_rssi_high_thresh;

	btc_rssi_low_thresh = (uint16)wlc_btc_params_get(wlc,
		BTC_FW_RSSI_THRESH_BLE);
	if (btc_rssi_low_thresh) {
		/* If low is 70, high will be 65, since thesholds stored
		 * as positive numbers
		 */
		btc_rssi_high_thresh = btc_rssi_low_thresh - 5;
		retval = rssi + btc_rssi_low_thresh;
		if (retval < 0) {
			return retval;
		}
		retval = rssi + btc_rssi_high_thresh;
		if (retval > 0) {
			return retval;
		}
	}
	return 0;
}

/*
This function is called by AMPDU Rx to get modified Rx aggregation size.
*/
uint8 wlc_btcx_get_ba_rx_wsize(wlc_info_t *wlc)
{
	if (BAND_2G(wlc->band->bandtype)) {
#ifdef WLAIBSS
		if ((AIBSS_ENAB(wlc->pub)) && wlc->btch->aibss_info->rx_agg_change)
			return (BT_AIBSS_RX_AGG_SIZE);
		else
#endif
		if (wlc->btch->rxagg_resized && wlc->btch->rxagg_size) {
			return wlc->btch->rxagg_size;
		}
	}
	return (0);
}
#endif /* WLAMPDU */

#ifdef	WLAIBSS
/* bsscfg up/down */
static void
wlc_btcx_bss_updn(void *ctx, bsscfg_up_down_event_data_t *evt)
{
	wlc_btc_info_t *btc = (wlc_btc_info_t *)ctx;
	wlc_bsscfg_t *cfg = (evt ? evt->bsscfg : NULL);

	BCM_REFERENCE(btc);
	BCM_REFERENCE(cfg);
	if (!AIBSS_ENAB(btc->wlc->pub) || (!cfg) || !BSSCFG_IBSS(cfg))
		return;

	/* Enable/Disable BT Strobe */
	wlc_btcx_strobe_enable(btc, evt->up);
}
/* scb cubby */
static int
wlc_btcx_scb_init(void *ctx, struct scb *scb)
{
	wlc_btc_info_t *btc = (wlc_btc_info_t *)ctx;
	btcx_oxygen_info_t **pbt_info = (btcx_oxygen_info_t **)SCB_CUBBY(scb, btc->scb_handle);
	btcx_oxygen_info_t *bt_info;

	if (SCB_INTERNAL(scb) || !AIBSS_ENAB(btc->wlc->pub)) {
		return BCME_OK;
	}

	if ((bt_info = MALLOCZ(btc->wlc->osh, sizeof(btcx_oxygen_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: no mem\n", btc->wlc->pub->unit, __FUNCTION__));
		return BCME_NOMEM;
	}
	*pbt_info = bt_info;

	return BCME_OK;
}

static void
wlc_btcx_scb_deinit(void *ctx, struct scb *scb)
{
	wlc_btc_info_t *btc = (wlc_btc_info_t *)ctx;
	btcx_oxygen_info_t **pbt_info = (btcx_oxygen_info_t **)SCB_CUBBY(scb, btc->scb_handle);
	btcx_oxygen_info_t *bt_info = *pbt_info;

	if (bt_info) {
		MFREE(btc->wlc->osh, bt_info, sizeof(btcx_oxygen_info_t));
		*pbt_info = NULL;
	}
}

/* Return the length of the BTCX IE in beacon */
static uint
wlc_btcx_aibss_ie_len(void *ctx, wlc_iem_calc_data_t *calc)
{
	wlc_btc_info_t *btc = (wlc_btc_info_t *)ctx;

	/* To avoid compiler warning when AIBSS_ENAB is '0' */
	BCM_REFERENCE(btc);

	if (AIBSS_ENAB(btc->wlc->pub)) {
		return (sizeof(btc_brcm_prop_ie_t));
	} else {
		return (0);
	}
}

/*
The following function builds the BTCX IE. This gets called when
beacon is being built
*/
static int
wlc_btcx_aibss_write_ie(void *ctx, wlc_iem_build_data_t *build)
{
	wlc_btc_info_t *btc = (wlc_btc_info_t *)ctx;
	btc_brcm_prop_ie_t *btc_brcm_prop_ie = (btc_brcm_prop_ie_t *)build->buf;
	int status = BCME_OK;

	if (((build->ft != FC_BEACON) || !AIBSS_ENAB(btc->wlc->pub)) ||
	    (build->buf_len < sizeof(btc_brcm_prop_ie_t))) {
		btc->aibss_info->write_ie_err_cnt++;
		status = BCME_UNSUPPORTED;
		goto error;
	}

	btc_brcm_prop_ie->id = DOT11_MNG_PROPR_ID;
	btc_brcm_prop_ie->len = BRCM_BTC_INFO_TYPE_LEN;
	memcpy(&btc_brcm_prop_ie->oui[0], BRCM_PROP_OUI, DOT11_OUI_LEN);
	btc_brcm_prop_ie->type = BTC_INFO_BRCM_PROP_IE_TYPE;

	/* Read out the latest BT state */
	wlc_btcx_aibss_get_local_bt_state(btc);

	/* Copy the BT status */
	btc_brcm_prop_ie->info = btc->aibss_info->local_btinfo;

error:
	return status;
}

/*
This function gets called during beacon parse when the BTCX IE is
present in the beacon
*/
static int
wlc_btcx_aibss_parse_ie(void *ctx, wlc_iem_parse_data_t *parse)
{
	wlc_btc_info_t *btc = (wlc_btc_info_t *)ctx;
	int status = BCME_OK;

	if (!btc) {
		status = BCME_NOTFOUND;
		goto error;
	}

	if (!AIBSS_ENAB(btc->wlc->pub)) {
		// Nothing to do
		goto error;
	}

	if ((parse->ie == NULL) || !BSSCFG_IBSS(parse->cfg)) {
		btc->aibss_info->parse_ie_err_cnt++;
		status = BCME_UNSUPPORTED;
		goto error;
	}

	switch (parse->ft) {
	case FC_BEACON:
		/* Beacon contains the Oxygen IE */
		if (parse->pparm->ft->bcn.scb != NULL) {
			btcx_oxygen_info_t **pbt_info =
				SCB_CUBBY((parse->pparm->ft->bcn.scb), (btc->scb_handle));
			btcx_oxygen_info_t *bt_info = *pbt_info;

			btc_brcm_prop_ie_t *btc_brcm_prop_ie = (btc_brcm_prop_ie_t *)parse->ie;

#if defined(BCMDBG)
			if (bt_info->btinfo != btc_brcm_prop_ie->info) {
				char eabuf[ETHER_ADDR_STR_LEN];

				bcm_ether_ntoa(&parse->pparm->ft->bcn.scb->ea, eabuf);
				WL_INFORM(("wl%d: Bcn: eth: %s BT Info chg: 0x%x 0x%x\n",
					btc->wlc->pub->unit, eabuf, bt_info->btinfo,
					btc_brcm_prop_ie->info));
			}
#endif
			bt_info->btinfo = btc_brcm_prop_ie->info;
			bt_info->bcn_rx_cnt++;
			wlc_btcx_aibss_check_state(btc);
		}

		break;
	}
error:
	return status;
}

/*
This function does the following:
1. Update beacon (Vendor specific BTCX IE) if local BT status has changed
since the last check.
2. Turn "fake eSCO" BT protection scheme on/off as follows:
	- If at least one other device in the IBSS has eSCO and local device
	  does not, turn protection on
	- If local device has eSCO on, turn protection off
	- If none of the devices in the IBSS has eSCO on, turn protection off
*/

static void
wlc_btcx_aibss_check_state(wlc_btc_info_t *btc)
{
	bool prot_on = FALSE, cmn_cts2self = FALSE;

	/* AIBSS_ENAB is checked by the caller */
	/* Update the local BT state */
	wlc_btcx_aibss_get_local_bt_state(btc);
	/* Check BT sync state in all the other devices in this IBSS */
	wlc_btc_aibss_get_state(btc, BT_CHECK_AIBSS_BT_SYNC_STATE);
	/* Update aggregation state */
	wlc_btcx_aibss_update_agg_state(btc);

	/*
	If BT info in local device (currently only SCO or eSCO indication and BT sync status)
	has changed, update beacon
	*/
	if (btc->aibss_info->local_btinfo != btc->aibss_info->last_btinfo) {
		int i;
		wlc_bsscfg_t *cfg;

#if defined(BCMDBG)
		WL_ERROR(("wl%d: BTCX: Local BT Info chg: 0x%x to 0x%x prd: %d sync: %d\n",
			btc->wlc->pub->unit, btc->aibss_info->last_btinfo,
			btc->aibss_info->local_btinfo, btc->bth_period,
			btc->aibss_info->local_bt_in_sync));
#endif
		/* Update Beacon since BT status has changed */
		FOREACH_BSS(btc->wlc, i, cfg) {
			if (cfg->associated && BSSCFG_IBSS(cfg)) {
				wlc_bss_update_beacon(btc->wlc, cfg);
			}
		}
		btc->aibss_info->last_btinfo = btc->aibss_info->local_btinfo;
	}


	if (!(btc->aibss_info->local_btinfo & BT_INFO_TASK_TYPE_ESCO)) {
		/* Local device does not have eSCO on */
		if (btc->aibss_info->other_esco_present && btc->aibss_info->other_bt_in_sync) {
			/* At least one other device in the IBSS has eSCO and
			   clock is synced at all devices
			*/
			prot_on = cmn_cts2self = TRUE;
		}
		/* else default values */
	} else {
		/* Local device has eSCO */
		/* prot_on is the default value */
		cmn_cts2self = btc->aibss_info->other_bt_in_sync &
			btc->aibss_info->local_bt_in_sync;
	}
	wlc_btcx_aibss_set_prot(btc, prot_on, cmn_cts2self);
}

/*
Use a low pass filter to weed out any spurious reads of BT sync state.
This function only sets state and does not return any status.
*/
static void
wlc_btcx_aibss_read_bt_sync_state(wlc_btc_info_t *btc)
{
	uint16 eci_m;
	uint8 tmp;

#define BTCX_OUT_OF_SYNC_CNT 5
	/* First check if the local device is the master. If not, sync state
	   does not apply
	*/
	eci_m = wlc_bmac_read_eci_data_reg(btc->wlc->hw, 2);

	if ((!AIBSS_ENAB(btc->wlc->pub)) || (eci_m & BT_IS_SLAVE)) {
		btc->aibss_info->local_bt_in_sync = FALSE;
		btc->aibss_info->local_bt_is_master = FALSE;
		goto exit;
	}

	btc->aibss_info->local_bt_is_master = TRUE;
	eci_m = wlc_bmac_read_eci_data_reg(btc->wlc->hw, 0);
	tmp = ((eci_m & BT_CLK_SYNC_STS) >> BT_CLK_SYNC_STS_SHIFT_BITS) & 0x1;
	if (!tmp) {
		btc->aibss_info->bt_out_of_sync_cnt++;
		if (btc->aibss_info->bt_out_of_sync_cnt <= BTCX_OUT_OF_SYNC_CNT) {
			tmp = btc->aibss_info->local_bt_in_sync;
		} else {
			btc->aibss_info->bt_out_of_sync_cnt = BTCX_OUT_OF_SYNC_CNT;
		}
	}
	else
		btc->aibss_info->bt_out_of_sync_cnt = 0;
	btc->aibss_info->local_bt_in_sync = tmp;
exit:
	return;
}

/* This function enables/disables BT strobe */
void
wlc_btcx_strobe_enable(wlc_btc_info_t *btc, bool on)
{
	if (BT3P_HW_COEX(btc->wlc) && BAND_2G(btc->wlc->band->bandtype)) {
		btc->aibss_info->strobe_enabled = on;
	} else {
		btc->aibss_info->strobe_enabled = FALSE;
	}
}


/* This function calculates the mod of the current TSF with the input factor */
static void
wlc_btcx_aibss_calc_tsf(wlc_btc_info_t *btc, uint32 *new_tsf_l,
	uint32 mod_factor, uint16 add_delta)
{
	uint32 tsf_l, tsf_h;
	uint32 new_tsf_h;
	uint32 temp;

	/* Read current TSF */
	wlc_read_tsf(btc->wlc, &tsf_l, &tsf_h);
	/* Divide by factor */
	bcm_uint64_divide(&temp, tsf_h, tsf_l, mod_factor);
	/* Multiply by factor and add factor * 2 */
	bcm_uint64_multiple_add(&new_tsf_h, new_tsf_l, temp, mod_factor, mod_factor * 2);
	/* Add delta */
	wlc_uint64_add(&new_tsf_h, new_tsf_l, 0, add_delta);
}

/*
This function enables/disables Oxygen protection (if local device does not have
eSCO or if BT clock is not synchronized in one or more devices in the IBSS)
*/
static void
wlc_btcx_aibss_set_prot(wlc_btc_info_t *btc, bool prot_on, bool cmn_cts2self)
{
	if (AIBSS_ENAB(btc->wlc->pub) && BT3P_HW_COEX(btc->wlc) &&
	    BAND_2G(btc->wlc->band->bandtype) && btc->aibss_info->strobe_on) {
		uint16 btcx_config;

		btcx_config = wlc_read_shm(btc->wlc, M_BTCX_CONFIG(btc->wlc));
		if (!btc->aibss_info->sco_prot_on && prot_on) {
			uint32 new_tsf_l;

			/* Read current TSF, divide and multiply by BT_ESCO_PERIOD
			and add 2 * BT_ESCO_PERIOD (15000).
			Setup the time for pretend eSCO protection:
			   Add 6175 (6175 ms = BT_ESCO_PERIOD (7500) - 750(to send CTS2SELF) -
			   625 (eSCO Tx slot)
			*/
			wlc_btcx_aibss_calc_tsf(btc, &new_tsf_l, BT_ESCO_PERIOD, 6175);

			/* Write out the eSCO protection start time in SHM */
			wlc_write_shm(btc->wlc, btc->aibss_info->ibss_tsf_shm +
				BT_IBSS_TSF_ESCO_OFFSET, (new_tsf_l & 0xFFFF));
			btcx_config |= C_BTCX_CONFIG_SCO_PROT;
			btc->aibss_info->sco_prot_on = TRUE;
		} else if (btc->aibss_info->sco_prot_on && !prot_on) {
			btcx_config &= ~C_BTCX_CONFIG_SCO_PROT;
			btc->aibss_info->sco_prot_on = FALSE;
		}
		if (cmn_cts2self)
			btcx_config |= C_BTCX_CFG_CMN_CTS2SELF;
		else
			btcx_config &= ~C_BTCX_CFG_CMN_CTS2SELF;
		wlc_write_shm(btc->wlc, M_BTCX_CONFIG(btc->wlc), btcx_config);
	}
}

/* Enable/disable strobing to BT */
static int
wlc_btcx_aibss_set_strobe(wlc_btc_info_t *btc, int strobe_on)
{
	if (AIBSS_ENAB(btc->wlc->pub) && BT3P_HW_COEX(btc->wlc) &&
		BAND_2G(btc->wlc->band->bandtype)) {
		uint32 new_tsf_l = 0;
		uint16 btcx_config;
		uint32 strobe_val = 0;
		uint32 secimcr = SECI_UART_MCR_BAUD_ADJ_EN | SECI_UART_MCR_HIGHRATE_EN |
			SECI_UART_MCR_TX_EN;
		uint32 seci_loopback = 0;
		si_t *sih = btc->wlc->hw->sih;

		btcx_config = wlc_read_shm(btc->wlc, M_BTCX_CONFIG(btc->wlc));
		if (!btc->aibss_info->strobe_on && strobe_on) {

			/* Enable WCI-2 interface for strobing */
			if (!si_btcx_wci2_init(sih)) {
				WL_ERROR(("BTCX Strobe:WCI2 Init failed\n"));
				btc->aibss_info->wci2_fail_cnt++;
				return BCME_NODEVICE;
			}
			/* Read current TSF, divide by BT_STROBE_INTERVAL,
			multiply by BT_STROBE_INTERVAL and add (2 * BT_STROBE_INTERVAL)
			*/
			wlc_btcx_aibss_calc_tsf(btc, &new_tsf_l, BT_STROBE_INTERVAL, 0);

			/* Read out current TSF to check for any TSF jumps in the watchdog
			   routine
			*/
			wlc_read_tsf(btc->wlc, &btc->aibss_info->prev_tsf_l,
				&btc->aibss_info->prev_tsf_h);

			/* Enable ucode */
			btcx_config |= C_BTCX_CONFIG_BT_STROBE;

			strobe_val = GCI_WL_STROBE_BIT_MASK;
			secimcr |= SECI_UART_MCR_LOOPBK_EN;
			seci_loopback = 1;
		} else if (btc->aibss_info->strobe_on && !strobe_on) {
			/* Disable WCI-2 interface */
			si_gci_reset(sih);
#if defined(BCMDBG)
			WL_INFORM(("wl%d: BTCX STROBE: Off\n", btc->wlc->pub->unit));
#endif

			/* Disable strobing in ucode */
			btcx_config &= ~(C_BTCX_CONFIG_BT_STROBE | C_BTCX_CONFIG_SCO_PROT);
			btc->aibss_info->sco_prot_on = FALSE;
		} else {
			WL_ERROR(("BTCX STROBE: strobe %s\n", (strobe_on) ? "already enabled" :
					"not enabled"));
			btc->aibss_info->strobe_enable_err_cnt++;
			return BCME_UNSUPPORTED;
		}
		/* Write out the starting strobe TS into SHM */
		wlc_write_shm(btc->wlc, btc->aibss_info->ibss_tsf_shm + BT_IBSS_TSF_LOW_OFFSET,
			(new_tsf_l & 0xFFFF));
		wlc_write_shm(btc->wlc, btc->aibss_info->ibss_tsf_shm + BT_IBSS_TSF_HIGH_OFFSET,
			(new_tsf_l >> 16) & 0xFFFF);

		btc->aibss_info->strobe_on = strobe_on;

		wlc_write_shm(btc->wlc, M_BTCX_CONFIG(btc->wlc), btcx_config);

		/* Indicate to BT that strobe is on/off */
		si_gci_direct(sih, GCI_OFFSETOF(sih, gci_control_1),
			GCI_WL_STROBE_BIT_MASK, strobe_val);
		si_gci_direct(sih, GCI_OFFSETOF(sih, gci_output[1]),
			GCI_WL_STROBE_BIT_MASK, strobe_val);

		if ((GCIREV(sih->gcirev) >= 1) && (GCIREV(sih->gcirev) < 4)) {
			/* Baud rate: loop back mode on/off */
			si_gci_direct(sih, GCI_OFFSETOF(sih, gci_secimcr),
				WLC_BTCX_STROBE_REG_MASK, secimcr);
		} else if (GCIREV(sih->gcirev) >= 4) {
			si_gci_indirect(sih, GCI_LTECX_SECI_ID,
				GCI_OFFSETOF(sih, gci_seciout_ctrl),
				(1 << GCI_SECIOUT_LOOPBACK_OFFSET),
				(seci_loopback << GCI_SECIOUT_LOOPBACK_OFFSET));
		} else {
			return (BCME_ERROR);
		}
		return (BCME_OK);
	}
	WL_ERROR(("BTCX STROBE: AIBSS Not enabled\n"));
	btc->aibss_info->strobe_init_err_cnt++;
	return (BCME_BADOPTION);
}

/*
This function checks to see if the TSF has jumped and if so stop and restart strobe.
To restart a flag is set so that any pending protection state completes before the
strobe is started again. This function also checks BT sync state and if not synced
grants ACL requests to speed up the synchronization process.
*/

static void
wlc_btcx_aibss_chk_clk_sync(wlc_btc_info_t *btc)
{
	uint16 pri_map_lo;

	pri_map_lo = wlc_read_shm(btc->wlc, M_BTCX_PRI_MAP_LO(btc->wlc));

	/* Check to see if TSF has jumped (because of sync to the network TSF). If it
	   has jumped, disable and enable strobing
	*/
	if (btc->aibss_info->strobe_enabled) {
		if (btc->aibss_info->strobe_on) {
			uint32 tsf_l, tsf_h;

			if ((btc->aibss_info->prev_tsf_l == 0) &&
				(btc->aibss_info->prev_tsf_h == 0))
				wlc_read_tsf(btc->wlc, &btc->aibss_info->prev_tsf_l,
					&btc->aibss_info->prev_tsf_h);

			wlc_read_tsf(btc->wlc, &tsf_l, &tsf_h);

			if (wlc_uint64_lt(btc->aibss_info->prev_tsf_h, btc->aibss_info->prev_tsf_l,
				tsf_h, tsf_l)) {
				uint32 temp_tsf_l = tsf_l, temp_tsf_h = tsf_h;

				wlc_uint64_sub(&temp_tsf_h, &temp_tsf_l,
					btc->aibss_info->prev_tsf_h,
					btc->aibss_info->prev_tsf_l);
				/*
				TSF Jump detected - turn strobe off. It gets restarted in
				watchdog next time around - that way ucode exits protection, etc.
				gracefully.
				*/
				if (temp_tsf_h || (temp_tsf_l > BT_AIBSS_TSF_JUMP_THRESH)) {
					wlc_btcx_aibss_set_strobe(btc, FALSE);
					btc->aibss_info->tsf_jump_cnt++;
				}
			}
			btc->aibss_info->prev_tsf_l = tsf_l;
			btc->aibss_info->prev_tsf_h = tsf_h;
		} else {
			wlc_btcx_aibss_set_strobe(btc, TRUE);
		}
		/* Check to see if BT clock is synchronized (only if eSCO is on) */
		if ((btc->aibss_info->strobe_on) &&
			(btc->aibss_info->local_btinfo & BT_INFO_TASK_TYPE_ESCO)) {

			/* Sync state applies only if the device is master */
			if (btc->aibss_info->local_bt_is_master) {

				if (!(btc->aibss_info->local_btinfo & BT_INFO_BT_CLK_SYNCED) &&
					(btc->aibss_info->acl_grant_cnt < BT_MAX_ACL_GRANT_CNT)) {
					/* If not synchronized, grant ACL requests to speed up
					   synchronization (as denied requests prevent master
					   from clock dragging)
					*/
					if (!(pri_map_lo & BT_ACL_TASK_BIT)) {
						pri_map_lo |= BT_ACL_TASK_BIT;
						wlc_write_shm(btc->wlc, M_BTCX_PRI_MAP_LO(btc->wlc),
							pri_map_lo);
						btc->aibss_info->acl_grant_set = TRUE;
						btc->aibss_info->acl_grant_cnt = 0;
#if defined(BCMDBG)
						WL_INFORM(("wl%d: BT Not in Sync\n",
							btc->wlc->pub->unit));
#endif
					} else {
						btc->aibss_info->acl_grant_cnt++;
					}
				}
				else if ((btc->aibss_info->local_btinfo & BT_INFO_BT_CLK_SYNCED) ||
				         (btc->aibss_info->acl_grant_cnt >= BT_MAX_ACL_GRANT_CNT)) {
					if (btc->aibss_info->acl_grant_set) {
						pri_map_lo &= ~BT_ACL_TASK_BIT;
						wlc_write_shm(btc->wlc, M_BTCX_PRI_MAP_LO(btc->wlc),
							pri_map_lo);
						btc->aibss_info->acl_grant_set = FALSE;
#if defined(BCMDBG)
						WL_INFORM(("wl%d: BT in Sync\n",
							btc->wlc->pub->unit));
#endif
					}
				}
				return;
			} else {
				btc->aibss_info->acl_grant_cnt = 0;
			}
		} else {
			btc->aibss_info->acl_grant_cnt = 0;
		}
	} else {
		wlc_btcx_aibss_set_strobe(btc, FALSE);
	}

	/* If Strobe is off or eSCO is off, disable acl grants if enabled */
	if (btc->aibss_info->acl_grant_set) {
		pri_map_lo &= ~BT_ACL_TASK_BIT;
		wlc_write_shm(btc->wlc, M_BTCX_PRI_MAP_LO(btc->wlc), pri_map_lo);
		btc->aibss_info->acl_grant_set = FALSE;
		btc->aibss_info->acl_grant_cnt = 0;
	}
}

/* Check AIBSS state (by traversing through the list of all devices) as per input
request. Currently supports 2 actions:
- Check for BT sync status in all devices in the IBSS (based on received beacons).
- Check for stale beacons - i.e., if no beacon has been received from a device since
the last check, clear out its BT info (in its SCB cubby).
*/
static void
wlc_btc_aibss_get_state(wlc_btc_info_t *btc, uint8 action)
{
	/* Check BT status in all the other devices in this IBSS */
	if (AIBSS_ENAB(btc->wlc->pub)) {
		int i;
		wlc_bsscfg_t *cfg;
		wlc_info_t *wlc = btc->wlc;

		btc->aibss_info->other_bt_in_sync = TRUE;
		wlc->btch->aibss_info->other_esco_present =  FALSE;
		FOREACH_AS_STA(wlc, i, cfg) {
			if (cfg->associated && BSSCFG_IBSS(cfg)) {
				struct scb_iter scbiter;
				struct scb *scb;

				FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
					btcx_oxygen_info_t **pbt_info =
						SCB_CUBBY((scb), (wlc->btch->scb_handle));
					btcx_oxygen_info_t *bt_info = *pbt_info;

					if (action & BT_CHECK_AIBSS_STALE_BEACON) {
						if (!bt_info->bcn_rx_cnt) {
							/* No beacons received since the last check,
							   clear out the BT info.
							*/
							bt_info->btinfo = 0;
						}
						bt_info->bcn_rx_cnt = 0;
					}
					if (action & BT_CHECK_AIBSS_BT_SYNC_STATE) {
						if (bt_info->btinfo & BT_INFO_TASK_TYPE_ESCO) {
							/* Atleast one device with BT eSCO on */
							wlc->btch->aibss_info->other_esco_present =
								TRUE;
							if (!(bt_info->btinfo &
								BT_INFO_BT_CLK_SYNCED)) {
								/* Atleast one BT is not synced */
								btc->aibss_info->other_bt_in_sync =
									FALSE;
							}
						}
					}
				}
			}
		}
	}
}

/* Get the latest local BT state */
static void
wlc_btcx_aibss_get_local_bt_state(wlc_btc_info_t *btc)
{
	if (AIBSS_ENAB(btc->wlc->pub)) {
		wlc_btcx_read_btc_params(btc->wlc);
		wlc_btcx_aibss_read_bt_sync_state(btc);

		btc->aibss_info->local_btinfo = 0;
		if (btc->bth_period >= BT_ESCO_PERIOD) {
			btc->aibss_info->local_btinfo = BT_INFO_TASK_TYPE_ESCO;
			if (btc->aibss_info->local_bt_in_sync)
				btc->aibss_info->local_btinfo |= BT_INFO_BT_CLK_SYNCED;
		}
	}
}

/*
Update Rx aggregation state based on if eSCO is enabled/disabled in the
entire AIBSS. This function sets the appropriate state based on
certain conditions
*/
static void
wlc_btcx_aibss_update_agg_state(wlc_btc_info_t *btc)
{
#if defined(BCMDBG)
	bool prev_rx_agg = btc->aibss_info->rx_agg_change;
#endif

	if (AIBSS_ENAB(btc->wlc->pub)) {
		if ((!btc->aibss_info->other_esco_present) &&
		     !(btc->aibss_info->local_btinfo & BT_INFO_TASK_TYPE_ESCO)) {
			/* BT eSCO is off in all devices in the IBSS */
			if (btc->aibss_info->rx_agg_change) {
				btc->aibss_info->esco_off_cnt++;
				/* Adding a low-pass filter to weed out spurious reads */
				if (btc->aibss_info->esco_off_cnt >= BT_ESCO_OFF_DELAY_CNT) {
					btc->aibss_info->esco_off_cnt = BT_ESCO_OFF_DELAY_CNT;
					// Revert aggregation size change
					btc->aibss_info->rx_agg_change = FALSE;
#if defined(BCMDBG)
					if (prev_rx_agg != btc->aibss_info->rx_agg_change) {
						WL_INFORM(("BTCX: Rx Agg revert cnt: %d esco: %d\n",
							btc->aibss_info->esco_off_cnt,
							btc->aibss_info->other_esco_present));
					}
#endif
				}
			}
			goto exit;
		}
		// eSCO is on in at least one of the devices in the IBSS
		btc->aibss_info->esco_off_cnt = 0;
		// Indicate aggregation size change is needed.
		btc->aibss_info->rx_agg_change = TRUE;
#if defined(BCMDBG)
		if (prev_rx_agg != btc->aibss_info->rx_agg_change) {
			WL_INFORM(("BTCX: Rx Agg chg cnt: %d esco: %d\n",
				btc->aibss_info->esco_off_cnt,
				btc->aibss_info->other_esco_present));
		}
#endif
	} else {
		btc->aibss_info->esco_off_cnt = BT_ESCO_OFF_DELAY_CNT;
		btc->aibss_info->rx_agg_change = FALSE;
	}
exit:
	return;
}

/* Update Rx aggregation size if needed as follows:
	- Turn off Rx agg
	- Turn it back on
	- AMPDU Rx will call wlc_btcx_get_ba_rx_wsize (from wlc_ampdu_recv_addba_req_resp())
		to get the BTCX size
*/
static void
wlc_btcx_aibss_update_agg_size(wlc_btc_info_t *btc)
{
	if (AIBSS_ENAB(btc->wlc->pub)) {
		if (btc->aibss_info->rx_agg_change && !btc->aibss_info->rx_agg_modified) {
			/* Agg change needed */
			btc->aibss_info->rx_agg_modified = TRUE;
		} else if (!btc->aibss_info->rx_agg_change && btc->aibss_info->rx_agg_modified) {
			/* Agg change needs to be reverted */
			btc->aibss_info->rx_agg_modified = FALSE;
		} else
			goto exit;
	} else {
		goto exit;
	}

#ifdef WLAMPDU
	/* If agg size needs to be modified or reverted, turn it off and back on */
	wlc_ampdu_agg_state_update_rx_all(btc->wlc, OFF);
	wlc_ampdu_agg_state_update_rx_all(btc->wlc, ON);
#endif /* WLAMPDU */

exit:
	return;
}

/*
Copy AIBSS status and configuration information into user-supplied buffer (IOVAR)
*/
static int
wlc_btcx_aibss_get_status(wlc_info_t *wlc, void *buf)
{
	wlc_btc_aibss_status_t *btc_status = (wlc_btc_aibss_status_t *)buf;
	int status = BCME_OK;

	if (!btc_status) {
		status = BCME_BUFTOOSHORT;
		goto error;
	}

	memset(btc_status, 0, sizeof(wlc_btc_aibss_status_t));

	btc_status->version = WLC_BTC_AIBSS_STATUS_VER;
	btc_status->len = WLC_BTC_AIBSS_STATUS_LEN;
	btc_status->mode = wlc->btch->mode;
	btc_status->bth_period = wlc->btch->bth_period;
	btc_status->agg_off_bm = wlc->btch->agg_off_bm;
	btc_status->bth_active = wlc->btch->bth_active;
	btc_status->aibss_info = *wlc->btch->aibss_info;

error:
	return (status);
}
#endif /* WLAIBSS */
#ifdef WLRSDB
static void
wlc_btcx_write_core_specific_mask(wlc_info_t *wlc)
{
	/* Mask value can change for other chips */
	if (CHIPID(wlc->pub->sih->chip) == BCM4364_CHIP_ID)
	{
		if (wlc->hw->macunit == 0) {
			/* Mask 1x1 core */
			si_gci_chipcontrol(wlc->pub->sih, CC_GCI_CHIPCTRL_11,
				GCI_CHIP_CTRL_PRISEL_MASK, GCI_CHIP_CTRL_PRISEL_MASK_CORE1);
		} else {
			/* Mask 3x3 core */
			si_gci_chipcontrol(wlc->pub->sih, CC_GCI_CHIPCTRL_11,
				GCI_CHIP_CTRL_PRISEL_MASK, GCI_CHIP_CTRL_PRISEL_MASK_CORE0);
		}
	} else if (BCM4347_CHIP(wlc->pub->sih->chip)) {
		if (wlc->hw->macunit == 0) {
			/* Mask core 1 */
			si_gci_chipcontrol(wlc->pub->sih, CC_GCI_CHIPCTRL_08,
			GCI_CHIP_CTRL_PRISEL_MASK_4347, GCI_CHIP_CTRL_PRISEL_MASK_CORE1_4347);
		} else {
			/* Mask core 0 */
			si_gci_chipcontrol(wlc->pub->sih, CC_GCI_CHIPCTRL_08,
			GCI_CHIP_CTRL_PRISEL_MASK_4347, GCI_CHIP_CTRL_PRISEL_MASK_CORE0_4347);
		}
	}
}

/* 4364: GCI mask for COEX */
void
wlc_btcx_update_gci_mask(wlc_info_t *wlc)
{
	wlc_info_t *other_wlc;
	other_wlc = wlc_rsdb_get_other_wlc(wlc);

	ASSERT(WLC_DUALMAC_RSDB(wlc->cmn));

	if (wlc_rsdb_is_other_chain_idle(wlc) == TRUE) {
		/* Only one core is active */
		/* Mask based on the MAC CORE */
		wlc_btcx_write_core_specific_mask(wlc);
	} else if (!wlc->pub->up) {
		wlc_btcx_write_core_specific_mask(other_wlc);
	} else {
		/* RSDB is active */
		if (CHSPEC_IS2G(wlc->hw->chanspec) && CHSPEC_IS5G(other_wlc->hw->chanspec)) {
			/* Mask based on the MAC CORE */
			wlc_btcx_write_core_specific_mask(wlc);
		} else if (CHSPEC_IS2G(other_wlc->hw->chanspec) &&
			CHSPEC_IS5G(wlc->hw->chanspec)) {
			/* Mask based on the MAC CORE */
			wlc_btcx_write_core_specific_mask(other_wlc);
		} else {
			/* Both cores in 5G leave both the mask to 0 for both the cores */
			if (CHIPID(wlc->pub->sih->chip) == BCM4364_CHIP_ID) {
				si_gci_chipcontrol(wlc->pub->sih, CC_GCI_CHIPCTRL_11,
					GCI_CHIP_CTRL_PRISEL_MASK, 0x00000000);
			} else if (BCM4347_CHIP(wlc->pub->sih->chip)) {
				si_gci_chipcontrol(wlc->pub->sih, CC_GCI_CHIPCTRL_08,
					GCI_CHIP_CTRL_PRISEL_MASK_4347, 0x00000000);
			}
		}
	}
}

#endif /* WLRSDB */
bool
wlc_btc_mode_not_parallel(int btc_mode)
{
	return (btc_mode && (btc_mode != WL_BTC_PARALLEL));
}


#ifdef WLAMPDU
static void
wlc_btcx_rx_ba_window_modify(wlc_info_t *wlc, bool *rx_agg)
{
	uint8 window_sz;
	if (!wlc->btch->rxagg_resized) {
		if (wlc->btch->bth_period >= BT_AMPDU_RESIZE_THRESH) {
			window_sz = (uint8) wlc_bmac_btc_params_get(wlc->hw,
				BTC_FW_AGG_SIZE_HIGH);
		} else {
			window_sz = (uint8) wlc_bmac_btc_params_get(wlc->hw,
				BTC_FW_AGG_SIZE_LOW);
		}
		if (!window_sz) {
			*rx_agg = FALSE;
		}
		else {
			/* To modify Rx agg size, turn off agg
			 * which will result in the other side
			 * sending BA Request. The new size will
			 * be indicated at that time
			 */
			wlc_ampdu_agg_state_update_rx_all(wlc, OFF);

			/* New size will be set when BA request
			 * is received
			 */
			wlc->btch->rxagg_resized = TRUE;
			wlc->btch->rxagg_size = window_sz;
			*rx_agg = TRUE;
			WL_INFORM(("BTCX: Resize Rx agg: %d\n",
				window_sz));
		}
	}
}

static void
wlc_btcx_rx_ba_window_reset(wlc_info_t *wlc)
{
	wlc->btch->rxagg_resized = FALSE;
	wlc->btch->rxagg_size = 0;
}
#endif /* WLAMPDU */


static void
wlc_btcx_watchdog(void *arg)
{
	wlc_btc_info_t *btc = (wlc_btc_info_t *)arg;
	wlc_info_t *wlc = (wlc_info_t *)btc->wlc;
	si_t *sih = wlc->pub->sih;
	int btc_mode = wlc_btc_mode_get(wlc);
	ASSERT(wlc->pub->tunables->maxscb <= 255);

	// This feature is only supported in GCI enabled chips
	if (BCMGCICOEX_ENAB_BMAC(wlc->hw)) {
		if (MCHAN_ACTIVE(wlc->pub)) {
			/* Indicate to BT that multi-channel is enabled */
			si_gci_direct(sih, GCI_OFFSETOF(sih, gci_control_1),
				GCI_WL_MCHAN_BIT_MASK, GCI_WL_MCHAN_BIT_MASK);
			si_gci_direct(sih, GCI_OFFSETOF(sih, gci_output[1]),
				GCI_WL_MCHAN_BIT_MASK, GCI_WL_MCHAN_BIT_MASK);
		} else {
			/* Indicate to BT that multi-channel is disabled */
			si_gci_direct(sih, GCI_OFFSETOF(sih, gci_control_1),
				GCI_WL_MCHAN_BIT_MASK, 0);
			si_gci_direct(sih, GCI_OFFSETOF(sih, gci_output[1]),
				GCI_WL_MCHAN_BIT_MASK, 0);
		}
	}

	/* update critical BT state, only for 2G band */
	if (btc_mode && BAND_2G(wlc->band->bandtype)) {
#if defined(STA) && defined(BTCX_PM0_IDLE_WAR)
		wlc_btc_pm_adjust(wlc, wlc->btch->bth_active);
#endif /* STA */

		wlc_btc_update_btrssi(btc);

#ifdef WL_BTCDYN
		if (BTCDYN_ENAB(wlc->pub) && IS_DYNCTL_ON(btc->btcdyn->dprof)) {
			/* new dynamic btcoex algo */
			wlc_btcx_dynctl(btc);
		} else {
#endif
			/* legacy mode switching */
			wlc_btc_rssi_threshold_get(wlc);

			/* if rssi too low, switch to TDM */
			if (wlc->btch->low_threshold &&
				-wlc->cfg->link->rssi >
				wlc->btch->low_threshold) {
				if (!IS_BTCX_FULLTDM(btc_mode)) {
					wlc->btch->mode_overridden = (uint8)btc_mode;
					wlc_btc_mode_set(wlc, WL_BTC_FULLTDM);
				}
			} else if (wlc->btch->high_threshold &&
				-wlc->cfg->link->rssi <
				wlc->btch->high_threshold) {
				if (btc_mode != WL_BTC_PARALLEL) {
					wlc->btch->mode_overridden = (uint8)btc_mode;
					wlc_btc_mode_set(wlc, WL_BTC_PARALLEL);
				}
			} else {
				if (wlc->btch->mode_overridden) {
					wlc_btc_mode_set(wlc, wlc->btch->mode_overridden);
					wlc->btch->mode_overridden = 0;
				}
			}

			/* enable protection in ucode */
			wlc_btc_set_ps_protection(wlc, wlc->cfg); /* enable */
#ifdef WL_BTCDYN
		}
#endif
	}


#if defined(WLAIBSS)
	if (AIBSS_ENAB(wlc->pub)) {
		/* Check and enable/disable Oxygen protection as needed */
		/* Check for any stale beacons every 5 seconds */
		if ((wlc->btch->run_cnt % BT_STALE_BEACON_CHK_PERIOD) == 0)
			wlc_btc_aibss_get_state(btc, BT_CHECK_AIBSS_STALE_BEACON);
		wlc_btcx_aibss_check_state(btc);
		wlc_btcx_aibss_chk_clk_sync(btc);
		if (btc->aibss_info->rx_agg_change) {
			wlc_btcx_aibss_update_agg_size(btc);
		}
	}
#endif // WLAIBSS

	wlc_btc_update_sco_params(wlc);

#ifdef WLAMPDU
	/* Adjust WL AMPDU settings based on BT/LTE COEX Profile */
	wlc_btcx_evaluate_agg_state(btc);
	wlc_btcx_ltecx_apply_agg_state(wlc);
#endif /* WLAMPDU */

#ifdef WL_BTCDYN
	if (BTCDYN_ENAB(wlc->pub) && (!IS_DYNCTL_ON(btc->btcdyn->dprof)))
#endif
	/* legacy desense coex  */
	{
		/* Dynamic restaging of rxgain for BTCoex */
		wlc_btcx_desense(btc, wlc->band->bandtype);
	}

	if (wlc->clk && (wlc->pub->sih->boardvendor == VENDOR_APPLE) &&
	    ((CHIPID(wlc->pub->sih->chip) == BCM4331_CHIP_ID) ||
	     (CHIPID(wlc->pub->sih->chip) == BCM4360_CHIP_ID))) {
		wlc_write_shm(wlc, M_COREMASK_BTRESP(wlc), (uint16)btc->siso_ack);
	}
}


/* handle BTC related iovars */

static int
wlc_btc_doiovar(void *ctx, uint32 actionid,
        void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif)
{
	wlc_btc_info_t *btc = (wlc_btc_info_t *)ctx;
	wlc_info_t *wlc = (wlc_info_t *)btc->wlc;
	int32 int_val = 0;
	int32 int_val2 = 0;
	int32 *ret_int_ptr;
	bool bool_val;
	int err = 0;

	BCM_REFERENCE(len);
	BCM_REFERENCE(val_size);
	BCM_REFERENCE(wlcif);

	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	if (p_len >= (int)sizeof(int_val) * 2)
		bcopy((void*)((uintptr)params + sizeof(int_val)), &int_val2, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	bool_val = (int_val != 0) ? TRUE : FALSE;

	switch (actionid) {

	case IOV_SVAL(IOV_BTC_FLAGS):
		err = wlc_btc_flags_idx_set(wlc, int_val, int_val2);
		break;

	case IOV_GVAL(IOV_BTC_FLAGS): {
		*ret_int_ptr = wlc_btc_flags_idx_get(wlc, int_val);
		break;
		}

	case IOV_SVAL(IOV_BTC_PARAMS):
		err = wlc_btc_params_set(wlc, int_val, int_val2);
		break;

	case IOV_GVAL(IOV_BTC_PARAMS):
		*ret_int_ptr = wlc_btc_params_get(wlc, int_val);
		break;

	case IOV_SVAL(IOV_BTC_MODE):
		err = wlc_btc_mode_set(wlc, int_val);
		break;

	case IOV_GVAL(IOV_BTC_MODE):
		*ret_int_ptr = wlc_btc_mode_get(wlc);
		break;

	case IOV_SVAL(IOV_BTC_WIRE):
		err = wlc_btc_wire_set(wlc, int_val);
		break;

	case IOV_GVAL(IOV_BTC_WIRE):
		*ret_int_ptr = wlc_btc_wire_get(wlc);
		break;

	case IOV_SVAL(IOV_BTC_STUCK_WAR):
		wlc_btc_stuck_war50943(wlc, bool_val);
		break;


	case IOV_GVAL(IOV_BTC_SISO_ACK):
		*ret_int_ptr = wlc_btc_siso_ack_get(wlc);
		break;

	case IOV_SVAL(IOV_BTC_SISO_ACK):
		wlc_btc_siso_ack_set(wlc, (int16)int_val, TRUE);
		break;

	case IOV_GVAL(IOV_BTC_RXGAIN_THRESH):
		*ret_int_ptr = ((uint32)btc->restage_rxgain_on_rssi_thresh |
			((uint32)btc->restage_rxgain_off_rssi_thresh << 8));
		break;

	case IOV_SVAL(IOV_BTC_RXGAIN_THRESH):
		if (int_val == 0) {
			err = wlc_iovar_setint(wlc, "phy_btc_restage_rxgain", 0);
			if (err == BCME_OK) {
				btc->restage_rxgain_on_rssi_thresh = 0;
				btc->restage_rxgain_off_rssi_thresh = 0;
				btc->restage_rxgain_active = 0;
				WL_BTCPROF(("wl%d: BTC restage rxgain disabled\n", wlc->pub->unit));
			} else {
				err = BCME_NOTREADY;
			}
		} else {
			btc->restage_rxgain_on_rssi_thresh = (uint8)(int_val & 0xFF);
			btc->restage_rxgain_off_rssi_thresh = (uint8)((int_val >> 8) & 0xFF);
			WL_BTCPROF(("wl%d: BTC restage rxgain enabled\n", wlc->pub->unit));
		}
		WL_BTCPROF(("wl%d: BTC restage rxgain thresh ON: -%d, OFF -%d\n",
			wlc->pub->unit,
			btc->restage_rxgain_on_rssi_thresh,
			btc->restage_rxgain_off_rssi_thresh));
		break;

	case IOV_GVAL(IOV_BTC_RXGAIN_FORCE):
		*ret_int_ptr = btc->restage_rxgain_force;
		break;

	case IOV_SVAL(IOV_BTC_RXGAIN_FORCE):
		btc->restage_rxgain_force = int_val;
		break;

	case IOV_GVAL(IOV_BTC_RXGAIN_LEVEL):
		*ret_int_ptr = btc->restage_rxgain_level;
		break;

	case IOV_SVAL(IOV_BTC_RXGAIN_LEVEL):
		btc->restage_rxgain_level = int_val;
		if (btc->restage_rxgain_active) {
			if ((err = wlc_iovar_setint(wlc, "phy_btc_restage_rxgain",
				btc->restage_rxgain_level)) != BCME_OK) {
				/* Need to apply new level on next update */
				btc->restage_rxgain_active = 0;
				err = BCME_NOTREADY;
			}
			WL_BTCPROF(("wl%d: set BTC rxgain level %d (active %d)\n",
				wlc->pub->unit,
				btc->restage_rxgain_level,
				btc->restage_rxgain_active));
		}
		break;

#ifdef	WL_BTCDYN
	/* get/set profile for bcoex dyn.ctl (desense, mode, etc) */
	case IOV_SVAL(IOV_BTC_DYNCTL):
		if (BTCDYN_ENAB(wlc->pub)) {
			err = wlc_btc_dynctl_profile_set(wlc, params);
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;

	case IOV_GVAL(IOV_BTC_DYNCTL):
		if (BTCDYN_ENAB(wlc->pub)) {
			err = wlc_btc_dynctl_profile_get(wlc, arg);
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;

	case IOV_GVAL(IOV_BTC_DYNCTL_STATUS):
		if (BTCDYN_ENAB(wlc->pub)) {
			err = wlc_btc_dynctl_status_get(wlc, arg);
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;

	case IOV_GVAL(IOV_BTC_DYNCTL_SIM):
		if (BTCDYN_ENAB(wlc->pub)) {
			err = wlc_btc_dynctl_sim_get(wlc, arg);
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;

	case IOV_SVAL(IOV_BTC_DYNCTL_SIM):
		if (BTCDYN_ENAB(wlc->pub)) {
			err = wlc_btc_dynctl_sim_set(wlc, arg);
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;
#endif /* WL_BTCDYN */

#ifdef WLBTCPROF_EXT
	case IOV_SVAL(IOV_BTC_BTRSSI_THRESH):
#ifdef WLBTCPROF
		wlc_btc_profile_set(wlc, int_val, IOV_BTC_BTRSSI_THRESH);
#endif /* WLBTCPROF */
		btc->prot_btrssi_thresh = (uint8)int_val;
		break;

	case IOV_SVAL(IOV_BTC_BTRSSI_HYST):
#ifdef WLBTCPROF
		wlc_btc_profile_set(wlc, int_val, IOV_BTC_BTRSSI_HYST);
#endif /* WLBTCPROF */
		btc->btrssi_hyst = (uint8)int_val;
		break;

	case IOV_SVAL(IOV_BTC_WLRSSI_HYST):
#ifdef WLBTCPROF
		wlc_btc_profile_set(wlc, int_val, IOV_BTC_WLRSSI_HYST);
#endif /* WLBTCPROF */
		btc->wlrssi_hyst = (uint8)int_val;
		break;

	case IOV_SVAL(IOV_BTC_WLRSSI_THRESH):
#ifdef WLBTCPROF
		wlc_btc_profile_set(wlc, int_val, IOV_BTC_WLRSSI_THRESH);
#endif /* WLBTCPROF */
		err = wlc_btc_params_set(wlc, (M_BTCX_PROT_RSSI_THRESH_OFFSET(wlc) >> 1), int_val);
		break;
#endif /* WLBTCPROF_EXT */

	case IOV_GVAL(IOV_BTC_STATUS):
		*ret_int_ptr = wlc_btcx_get_btc_status(wlc);
		break;

#ifdef WLAIBSS
#if defined(BCMDBG)
	case IOV_SVAL(IOV_BTC_SET_STROBE):
		if (AIBSS_ENAB(wlc->pub)) {
			wlc_btcx_strobe_enable(btc, int_val);
		}
		break;

	case IOV_GVAL(IOV_BTC_SET_STROBE):
		*ret_int_ptr = btc->aibss_info->strobe_enabled;
		break;
#endif /* BCMDBG */
	case IOV_GVAL(IOV_BTC_AIBSS_STATUS):
		if (AIBSS_ENAB(wlc->pub)) {
			if (p_len >= sizeof(wlc_btc_aibss_status_t))
				err = wlc_btcx_aibss_get_status(wlc, arg);
			else
				err = BCME_BUFTOOSHORT;
		} else
			err = BCME_UNSUPPORTED;
		break;
#endif /* WLAIBSS */

#ifdef STA
#ifdef WLBTCPROF
	case IOV_GVAL(IOV_BTC_SELECT_PROFILE):
		err = wlc_btcx_select_profile_get(wlc, arg, len);
		break;

	case IOV_SVAL(IOV_BTC_SELECT_PROFILE):
		err = wlc_btcx_select_profile_set(wlc, arg, len);
		break;
#endif /* WLBTCPROF */
#endif /* STA */

#ifdef WL_UCM
	case IOV_GVAL(IOV_BTC_PROFILE):
		if (UCM_ENAB(wlc->pub))
			err = wlc_btc_prof_get(wlc, int_val, arg, len);
		else
			err = BCME_UNSUPPORTED;
	break;

	case IOV_SVAL(IOV_BTC_PROFILE):
		if (UCM_ENAB(wlc->pub))
			err = wlc_btc_prof_set(wlc, arg, len);
		else
			err = BCME_UNSUPPORTED;
	break;
#endif /* WL_UCM */

	default:
		err = BCME_UNSUPPORTED;
	}
	return err;
}

/* E.g., To set BTCX_HFLG_SKIPLMP, wlc_btc_hflg(wlc, 1, BTCX_HFLG_SKIPLMP) */
void
wlc_btc_hflg(wlc_info_t *wlc, bool set, uint16 val)
{
	uint16 btc_hflg;

	if (!wlc->clk)
		return;

	btc_hflg = wlc_bmac_read_shm(wlc->hw, M_BTCX_HOST_FLAGS(wlc));

	if (set)
		btc_hflg |= val;
	else
		btc_hflg &= ~val;

	wlc_bmac_write_shm(wlc->hw, M_BTCX_HOST_FLAGS(wlc), btc_hflg);
}

uint32
wlc_btcx_get_btc_status(wlc_info_t *wlc)
{
	uint32 status;
	uint16 mhf1 = wlc_mhf_get(wlc, MHF1, WLC_BAND_AUTO);
	uint16 mhf3 = wlc_mhf_get(wlc, MHF3, WLC_BAND_AUTO);
	uint16 mhf5 = wlc_mhf_get(wlc, MHF5, WLC_BAND_AUTO);
	uint16 btc_hf = (uint16)wlc_btc_params_get(wlc, M_BTCX_HOST_FLAGS_OFFSET(wlc)/2);

	status = (((mhf1 & MHF1_BTCOEXIST) ? 1 << BTCX_STATUS_BTC_ENABLE_SHIFT : 0) |
		((mhf3 & MHF3_BTCX_ACTIVE_PROT) ? 1 << BTCX_STATUS_ACT_PROT_SHIFT : 0) |
		((wlc->btch->simrx_slna) ? 1 << BTCX_STATUS_SLNA_SHIFT : 0) |
		((wlc->btch->siso_ack) ? 1 << BTCX_STATUS_SISO_ACK_SHIFT : 0) |
		((mhf3 & MHF3_BTCX_SIM_RSP) ? 1 << BTCX_STATUS_SIM_RSP_SHIFT : 0) |
		((mhf3 & MHF3_BTCX_PS_PROTECT) ? 1 << BTCX_STATUS_PS_PROT_SHIFT : 0) |
		((mhf3 & MHF3_BTCX_SIM_TX_LP) ? 1 << BTCX_STATUS_SIM_TX_LP_SHIFT : 0) |
		((btc_hf & BTCX_HFLG_DYAGG) ? 1 << BTCX_STATUS_DYN_AGG_SHIFT : 0) |
		((btc_hf & BTCX_HFLG_ANT2WL) ? 1 << BTCX_STATUS_ANT_WLAN_SHIFT : 0) |
		((mhf3 & MHF3_BTCX_DEF_BT) ? 1 << BTCX_STATUS_DEF_BT_SHIFT : 0) |
		((mhf5 & MHF5_BTCX_DEFANT) ? 1 << BTCX_STATUS_DEFANT_SHIFT : 0));

	return status;
}

#ifdef WL_UCM
static int
wlc_btc_prof_get(wlc_info_t *wlc, int32 idx, void *resbuf, uint len)
{
	wlc_btcx_profile_t *profile, **profile_array = wlc->btch->profile_array;
	if (idx >= MAX_UCM_PROFILES || idx < 0) {
		return BCME_RANGE;
	}
	profile = profile_array[idx];
	/* check if the IO buffer has provided enough space to send the profile */
	if (profile->length > len) {
		return BCME_BUFTOOSHORT;
	}
	/* Although wlu does not know in advance, we copy over the profile with right sized array */
	memcpy(resbuf, profile, sizeof(*profile)
		+(profile->chain_attr_count)*sizeof(profile->chain_attr[0]));
	return BCME_OK;
}

static int
wlc_btc_prof_set(wlc_info_t *wlc, void *parambuf, uint len)
{
	uint8 idx, prof_attr;
	uint16 prof_len, prof_ver;
	wlc_btcx_profile_t *profile, **profile_array = wlc->btch->profile_array;
	size_t ucm_prof_sz;

	/* extract the profile index, so we know which one needs to be updated */
	idx = ((wlc_btcx_profile_t *)parambuf)->profile_index;
	if (idx >= MAX_UCM_PROFILES || idx < 0) {
		return BCME_RANGE;
	}
	profile = profile_array[idx];
	ucm_prof_sz = sizeof(*profile)
		+(profile->chain_attr_count)*sizeof(profile->chain_attr[0]);
	/* check if the io buffer has enough data */
	prof_len = ((wlc_btcx_profile_t *)parambuf)->length;
	if (prof_len > len) {
		return BCME_BUFTOOSHORT;
	}
	/* check if the version returned by the utility is correct */
	prof_ver = ((wlc_btcx_profile_t *)parambuf)->version;
	if (prof_ver != UCM_PROFILE_VERSION) {
		return BCME_VERSION;
	}
	/* check if the profile length is consistent */
	if (prof_len != profile->length) {
		return BCME_BADLEN;
	}
	prof_attr = ((wlc_btcx_profile_t *)parambuf)->chain_attr_count;
	/* check if the number of tx chains is consistent */
	if (prof_attr != profile->chain_attr_count) {
		return BCME_BADLEN;
	}

	/* wlc will copy over the var array sized to number of attributes */
	memcpy(profile, parambuf, ucm_prof_sz);

	/* indicate that this profile or a part of this profile has been initialized */
	profile->init = 1;
	return BCME_OK;
}
#endif /* WL_UCM */

#ifdef WL_BTCDYN
/* dynamic BTCOEX wl densense & coex mode switching */

void wlc_btcx_chspec_change_notify(wlc_info_t *wlc, chanspec_t chanspec)
{
	BTCDBG(("%s:wl%d: old chspec:0x%x new chspec:0x%x\n",
		__FUNCTION__, wlc->pub->unit,
		old_chanspec, chanspec));
	/* chip specific WAR code */
}

/* extract bf, apply mask check if invalid */
static void btcx_extract_pwr(int16 btpwr, uint8 shft, int8 *pwr8)
{
	int8 tmp;
	*pwr8 = BT_INVALID_TX_PWR;

	if ((tmp = (btpwr >> shft) & SHM_BTC_MASK_TXPWR)) {
		*pwr8 = (tmp * BT_TX_PWR_STEP) + BT_TX_PWR_OFFSET;
	}
}

/*
	checks for BT power of each active task, converts to dBm and
	returns the highest power level if there is > 1 task detected.
*/
static int8 wlc_btcx_get_btpwr(wlc_btc_info_t *btc)
{
	wlc_info_t *wlc = btc->wlc;

	int8 pwr_sco, pwr_a2dp, pwr_sniff, pwr_acl;
	int8 result_pwr = BT_INVALID_TX_PWR;
	uint16 txpwr_shm;
	int8 pwr_tmp;

	/* read btpwr  */
	txpwr_shm = wlc_read_shm(wlc, M_BTCX_BT_TXPWR(wlc));
	btc->btcdyn->bt_pwr_shm = txpwr_shm; /* keep a copy for dbg & status */

	/* clear the shm after read, ucode will refresh with a new value  */
	wlc_write_shm(wlc, M_BTCX_BT_TXPWR(wlc), 0);

	btcx_extract_pwr(txpwr_shm, SHM_BTC_SHFT_TXPWR_SCO, &pwr_sco);
	btcx_extract_pwr(txpwr_shm, SHM_BTC_SHFT_TXPWR_A2DP, &pwr_a2dp);
	btcx_extract_pwr(txpwr_shm, SHM_BTC_SHFT_TXPWR_SNIFF, &pwr_sniff);
	btcx_extract_pwr(txpwr_shm, SHM_BTC_SHFT_TXPWR_ACL, &pwr_acl);

	/*
	  although rare, both a2dp & sco may be active,
	  pick the highest one. if both are invalid, check sniff
	*/
	if (pwr_sco != BT_INVALID_TX_PWR ||
		pwr_a2dp != BT_INVALID_TX_PWR) {

		BTCDBG(("%s: shmem_val:%x, BT tasks pwr: SCO:%d, A2DP:%d, SNIFF:%d\n",
			__FUNCTION__, txpwr_shm, pwr_sco, pwr_a2dp, pwr_sniff));

		result_pwr = pwr_sco;
		if (pwr_a2dp > pwr_sco)
			result_pwr = pwr_a2dp;

	} else if (pwr_acl != BT_INVALID_TX_PWR) {
		result_pwr = pwr_acl;
	} else if (pwr_sniff != BT_INVALID_TX_PWR) {
		result_pwr = pwr_sniff;
	}

#ifdef DBG_BTPWR_HOLES
	btcdyn_detect_btpwrhole(btc, result_pwr);
#endif

	/* protect from single invalid pwr reading ("pwr hole") */
	if (result_pwr == BT_INVALID_TX_PWR) {
		BTCDBG(("cur btpwr invalid, use prev value:%d\n",
			btc->prev_btpwr));
		pwr_tmp = btc->btcdyn->prev_btpwr;
	} else {
		pwr_tmp = result_pwr;
	}

	btc->btcdyn->prev_btpwr = result_pwr;
	result_pwr = pwr_tmp;
	return result_pwr;
}

/*
	At a given BT TX PWR level (typically > 7dbm)
	there is a certain WL RSSI level range at which WL performance in Hybrid
	COEX mode is actually lower than in Full TDM.
	The algorithm below selects the right mode based on tabulated data points.
	The profile table is specific for each board and needs to be calibrated
	for every new board + BT+WIFI antenna design.
*/
static uint8 btcx_dflt_mode_switch(wlc_info_t *wlc, int8 wl_rssi, int8 bt_pwr, int8 bt_rssi)
{
	wlc_btc_info_t *btc = wlc->btch;
	dctl_prof_t *profile = btc->btcdyn->dprof;
	uint8 row, new_mode;

	new_mode = profile->default_btc_mode;

#ifdef BCMLTECOEX
	/* No mode switch is required if ltecx is ON and Not in desense mode */
	if (BCMLTECOEX_ENAB(wlc->pub) && wlc_ltecx_get_lte_status(wlc->ltecx) &&
		!wlc->ltecx->mws_elna_bypass) {
		return WL_BTC_FULLTDM;
	}
#endif

	/* no active BT task when bt_pwr is invalid */
	if	(bt_pwr == BT_INVALID_TX_PWR) {
		return new_mode;
	}

	/* keep current coex mode  if: */
	if ((btc->bt_rssi == BTC_BTRSSI_INVALID) ||
		(btc->btcdyn->wl_rssi == WLC_RSSI_INVALID)) {
		return btc->mode;
	}

	for (row = 0; row < profile->msw_rows; row++) {
		if ((bt_pwr >= profile->msw_data[row].bt_pwr) &&
			(bt_rssi < profile->msw_data[row].bt_rssi +
			btc->btcdyn->msw_btrssi_hyster) &&
			(wl_rssi > profile->msw_data[row].wl_rssi_low) &&
			(wl_rssi < profile->msw_data[row].wl_rssi_high)) {
			/* new1: fallback mode is now per {btpwr + bt_rssi + wl_rssi range} */
				new_mode = profile->msw_data[row].mode;
			break;
		}
	}

	if (new_mode != profile->default_btc_mode) {
		/* the new mode is a downgrade from the default one,
		 for wl & bt signal conditions have deteriorated.
		 Apply hysteresis to stay in this mode until
		 the conditions get better by >= hyster values
		*/
		/*  positive for bt rssi  */
		btc->btcdyn->msw_btrssi_hyster = profile->msw_btrssi_hyster;
	} else {
		/* in or has switched to default, turn off h offsets */
		btc->btcdyn->msw_btrssi_hyster = 0;
	}

	return new_mode;
}

/*
* calculates new desense level using
* current btcmode, bt_pwr, wl_rssi,* and board profile data points
*/
static uint8 btcx_dflt_get_desense_level(wlc_info_t *wlc, int8 wl_rssi, int8 bt_pwr, int8 bt_rssi)
{
	wlc_btc_info_t *btc = wlc->btch;
	dctl_prof_t *profile = btc->btcdyn->dprof;
	uint8 row, new_level;

	new_level = profile->dflt_dsns_level;

	/* BT "No tasks" -> use default desense */
	if	(bt_pwr == BT_INVALID_TX_PWR) {
		return new_level;
	}

	/*  keep current desense level if: */
	if ((btc->bt_rssi == BTC_BTRSSI_INVALID) ||
		(btc->btcdyn->wl_rssi == WLC_RSSI_INVALID)) {
		return btc->btcdyn->cur_dsns;
	}

	for (row = 0; row < profile->dsns_rows; row++) {
		if (btc->mode == profile->dsns_data[row].mode &&
			bt_pwr >= profile->dsns_data[row].bt_pwr) {

			if (wl_rssi > profile->dsns_data[row].wl_rssi_high) {
				new_level = profile->high_dsns_level;
			}
			else if (wl_rssi > profile->dsns_data[row].wl_rssi_low) {

				new_level = profile->mid_dsns_level;
			} else {
				new_level = profile->low_dsns_level;
			}
			break;
		}
	}
	return  new_level;
}

/* set external desense handler */
int btcx_set_ext_desense_calc(wlc_info_t *wlc, btcx_dynctl_calc_t fn)
{
	wlc_btcdyn_info_t *btcdyn = wlc->btch->btcdyn;

	ASSERT(fn);
	btcdyn->desense_fn = fn;
	return BCME_OK;
}

/* set external mode switch handler */
int btcx_set_ext_mswitch_calc(wlc_info_t *wlc, btcx_dynctl_calc_t fn)
{
	wlc_btcdyn_info_t *btcdyn = wlc->btch->btcdyn;

	ASSERT(fn);
	btcdyn->mswitch_fn = fn;
	return BCME_OK;
}

/*  wrapper for btcx code portability */
static int8 btcx_get_wl_rssi(wlc_btc_info_t *btc)
{
	return btc->wlc->cfg->link->rssi;
}
/*
	dynamic COEX CTL (desense & switching) called from btc_wtacdog()
*/
static void wlc_btcx_dynctl(wlc_btc_info_t *btc)
{
	wlc_info_t *wlc = btc->wlc;
	uint8 btc_mode = wlc->btch->mode;
	dctl_prof_t *ctl_prof = btc->btcdyn->dprof;
	wlc_btcdyn_info_t *btcdyn = btc->btcdyn;
	uint16 btcx_blk_ptr = wlc->hw->btc->bt_shm_addr;
	uint32 cur_ts = OSL_SYSUPTIME();

	/* protection against too frequent calls from btc watchdog context */
	if ((cur_ts - btcdyn->prev_btpwr_ts) < DYNCTL_MIN_PERIOD) {
		btcdyn->prev_btpwr_ts = cur_ts;
		return;
	}
	btcdyn->prev_btpwr_ts = cur_ts;

	btcdyn->bt_pwr = wlc_btcx_get_btpwr(btc);
	btcdyn->wl_rssi = btcx_get_wl_rssi(btc);

	if (btcdyn->dynctl_sim_on) {
	/* simulation mode is on  */
		btcdyn->bt_pwr = btcdyn->sim_btpwr;
		btcdyn->wl_rssi = btcdyn->sim_wlrssi;
		btc->bt_rssi = btcdyn->sim_btrssi;
	}

#ifdef BCMLTECOEX
	/* No mode switch is required if ltecx is ON */
	if (IS_MSWITCH_ON(ctl_prof) && !wlc_ltecx_get_lte_status(wlc->ltecx)) {
	/* 1st check if we need to switch the btc_mode */
#else
	if (IS_MSWITCH_ON(ctl_prof)) {
#endif /* BCMLTECOEX */
		uint8 new_mode;

		ASSERT(btcdyn->mswitch_fn);
		new_mode = btcdyn->mswitch_fn(wlc, btcdyn->wl_rssi,
			btcdyn->bt_pwr, btc->bt_rssi);

		if (new_mode != btc_mode) {
			wlc_btc_mode_set(wlc, new_mode);

			BTCDBG(("%s mswitch mode:%d -> mode:%d,"
				" bt_pwr:%d, wl_rssi:%d, cur_dsns:%d, BT hstr[rssi:%d]\n",
				__FUNCTION__, btc_mode, new_mode, btcdyn->bt_pwr,
				btcdyn->wl_rssi, btcdyn->cur_dsns,
				ctl_prof->msw_btrssi_hyster));

			if ((wlc->hw->boardflags & BFL_FEM_BT) == 0 && /* dLNA chip */
				btcx_blk_ptr != 0) {
				/* update btcx_host_flags  based on btc_mode */
				if (new_mode == WL_BTC_FULLTDM) {
					wlc_bmac_update_shm(wlc->hw, M_BTCX_HOST_FLAGS(wlc),
						BTCX_HFLG_DLNA_TDM_VAL, BTCX_HFLG_DLNA_MASK);
				} else {
					/* mainly for hybrid and parallel */
					wlc_bmac_update_shm(wlc->hw, M_BTCX_HOST_FLAGS(wlc),
						BTCX_HFLG_DLNA_DFLT_VAL, BTCX_HFLG_DLNA_MASK);
				}
			}
		}
	}

	/* enable protection after mode switching */
	wlc_btc_set_ps_protection(wlc, wlc->cfg); /* enable */

	/* check if we need to switch the desense level */
	if (IS_DESENSE_ON(ctl_prof)) {
		uint8 new_level;

		ASSERT(btcdyn->desense_fn);
		new_level = btcdyn->desense_fn(wlc, btcdyn->wl_rssi,
			btcdyn->bt_pwr, btc->bt_rssi);

		if (new_level != btcdyn->cur_dsns) {
			/* apply new desense level */
			if ((wlc_iovar_setint(wlc, "phy_btc_restage_rxgain",
				new_level)) == BCME_OK) {

				BTCDBG(("%s: set new desense:%d, prev was:%d btcmode:%d,"
					" bt_pwr:%d, wl_rssi:%d\n",
					__FUNCTION__, new_level, btcdyn->cur_dsns, btc->mode,
					btcdyn->bt_pwr, btcdyn->wl_rssi));

				btcdyn->cur_dsns = new_level;
			} else
				WL_ERROR(("%s desense apply error\n",
					__FUNCTION__));
		}
	}
}

static int wlc_btc_dynctl_profile_set(wlc_info_t *wlc, void *parambuf)
{
	wlc_btcdyn_info_t *btcdyn = wlc->btch->btcdyn;
	bcopy(parambuf, btcdyn->dprof, sizeof(dctl_prof_t));
	return BCME_OK;
}

static int wlc_btc_dynctl_profile_get(wlc_info_t *wlc, void *resbuf)
{
	wlc_btcdyn_info_t *btcdyn = wlc->btch->btcdyn;
	bcopy(btcdyn->dprof, resbuf, sizeof(dctl_prof_t));
	return BCME_OK;
}

/* get dynctl status iovar handler */
static int wlc_btc_dynctl_status_get(wlc_info_t *wlc, void *resbuf)
{
	wlc_btc_info_t *btc = wlc->btch;
	dynctl_status_t dynctl_status;

	/* agg. stats into local stats var */
	dynctl_status.sim_on = btc->btcdyn->dynctl_sim_on;
	dynctl_status.bt_pwr_shm  = btc->btcdyn->bt_pwr_shm;
	dynctl_status.bt_pwr = btc->btcdyn->bt_pwr;
	dynctl_status.bt_rssi = btc->bt_rssi;
	dynctl_status.wl_rssi = btc->btcdyn->wl_rssi;
	dynctl_status.dsns_level = btc->btcdyn->cur_dsns;
	dynctl_status.btc_mode = btc->mode;

	/* return it */
	bcopy(&dynctl_status, resbuf, sizeof(dynctl_status_t));
	return BCME_OK;
}

/*   get dynctl sim parameters */
static int wlc_btc_dynctl_sim_get(wlc_info_t *wlc, void *resbuf)
{
	dynctl_sim_t sim;
	wlc_btcdyn_info_t *btcdyn = wlc->btch->btcdyn;

	sim.sim_on = btcdyn->dynctl_sim_on;
	sim.btpwr = btcdyn->sim_btpwr;
	sim.btrssi = btcdyn->sim_btrssi;
	sim.wlrssi = btcdyn->sim_wlrssi;

	bcopy(&sim, resbuf, sizeof(dynctl_sim_t));
	return BCME_OK;
}

/*   set dynctl sim parameters */
static int wlc_btc_dynctl_sim_set(wlc_info_t *wlc, void *parambuf)
{
	dynctl_sim_t sim;

	wlc_btcdyn_info_t *btcdyn = wlc->btch->btcdyn;
	bcopy(parambuf, &sim, sizeof(dynctl_sim_t));

	btcdyn->dynctl_sim_on = sim.sim_on;
	btcdyn->sim_btpwr = sim.btpwr;
	btcdyn->sim_btrssi = sim.btrssi;
	btcdyn->sim_wlrssi = sim.wlrssi;

	return BCME_OK;
}

/*
* initialize one row btc_thr_data_t * from a named nvram var
*/
static int
BCMATTACHFN(wlc_btc_dynctl_init_trow)(wlc_btc_info_t *btc,
	btc_thr_data_t *trow, const char *varname, uint16 xpected_sz)
{
	wlc_info_t *wlc = btc->wlc;
	wlc_hw_info_t *wlc_hw = wlc->hw;
	uint16 j = 0;

	/* read mode switching table */
	int array_sz = getintvararraysize(wlc_hw->vars, varname);
	if (!array_sz)
		return 0; /* var is not present */

	/* mk sure num of items in the var is OK */
	if (array_sz != xpected_sz)
		return -1;

	trow->mode = getintvararray(wlc_hw->vars, varname, j++);
	trow->bt_pwr = getintvararray(wlc_hw->vars, varname, j++);
	trow->bt_rssi = getintvararray(wlc_hw->vars, varname, j++);
	trow->wl_rssi_high = getintvararray(wlc_hw->vars, varname, j++);
	trow->wl_rssi_low = getintvararray(wlc_hw->vars, varname, j++);
	return 1;
}

static int
BCMATTACHFN(wlc_btcdyn_attach)(wlc_btc_info_t *btc)
{
	wlc_info_t *wlc = btc->wlc;
	wlc_hw_info_t *wlc_hw = wlc->hw;
	wlc_btcdyn_info_t *btcdyn;
	dctl_prof_t *dprof;

	if ((btcdyn = (wlc_btcdyn_info_t *)
			MALLOCZ(wlc->osh, sizeof(wlc_btcdyn_info_t))) == NULL) {
			WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			return BCME_NOMEM;
	}

	btc->btcdyn = btcdyn;

	if ((dprof = MALLOCZ(wlc->osh, sizeof(dctl_prof_t))) != NULL) {

		/*  default desnense & mode switching tables (2 rows each) */
		btc_thr_data_t dflt_msw_data[2] = {
			{1, 12, -73, -30, -90},
			{1, 8, -73, -30, -60},
		};
		/*  default desense datatable  */
		btc_thr_data_t dflt_dsns_data[2] = {
			{5, 4, 0, -55, -65},
			{5, -16, 0, -50, -65}
		};

		btcdyn->dprof = dprof;
		/* dynctl profile struct ver */
		dprof->version = DCTL_PROFILE_VER;
		/* default WL desense & mode switch handlers  */
		btcdyn->desense_fn = btcx_dflt_get_desense_level;
		btcdyn->mswitch_fn = btcx_dflt_mode_switch;
		btc->bt_rssi = BTC_BTRSSI_INVALID;
		dprof->msw_btrssi_hyster = BTCDYN_DFLT_BTRSSI_HYSTER;

		/*
		 * try loading btcdyn profile from nvram,
		 * use "btcdyn_flags" var as a presense indication
		 */
		if (getvar(wlc_hw->vars, rstr_btcdyn_flags) != NULL) {

			uint16 i;

			/* read int params 1st */
			dprof->flags =	getintvar(wlc_hw->vars, rstr_btcdyn_flags);
			dprof->dflt_dsns_level =
				getintvar(wlc_hw->vars, rstr_btcdyn_dflt_dsns_level);
			dprof->low_dsns_level =
				getintvar(wlc_hw->vars, rstr_btcdyn_low_dsns_level);
			dprof->mid_dsns_level =
				getintvar(wlc_hw->vars, rstr_btcdyn_mid_dsns_level);
			dprof->high_dsns_level =
				getintvar(wlc_hw->vars, rstr_btcdyn_high_dsns_level);
			dprof->default_btc_mode =
				getintvar(wlc_hw->vars, rstr_btcdyn_default_btc_mode);
			dprof->msw_btrssi_hyster =
				getintvar(wlc_hw->vars, rstr_btcdyn_btrssi_hyster);

			/* these two are used for data array sz check */
			dprof->msw_rows =
				getintvar(wlc_hw->vars, rstr_btcdyn_msw_rows);
			dprof->dsns_rows =
				getintvar(wlc_hw->vars, rstr_btcdyn_dsns_rows);

			/* sanity check on btcdyn nvram table sz */
			if ((dprof->msw_rows > DCTL_TROWS_MAX) ||
				(((dprof->flags & DCTL_FLAGS_MSWITCH) == 0) !=
				(dprof->msw_rows == 0))) {
				BTCDBG(("btcdyn invalid mode switch config\n"));
				goto rst2_dflt;
			}
			if ((dprof->dsns_rows > DCTL_TROWS_MAX) ||
				(((dprof->flags & DCTL_FLAGS_DESENSE) == 0) !=
				(dprof->dsns_rows == 0))) {
				BTCDBG(("btcdyn invalid dynamic desense config\n"));
				goto rst2_dflt;
			}

			/*  initialize up to 4 rows in msw table */
			i = wlc_btc_dynctl_init_trow(btc, &dprof->msw_data[0],
				rstr_btcdyn_msw_row0, sizeof(btc_thr_data_t));
			i += wlc_btc_dynctl_init_trow(btc, &dprof->msw_data[1],
				rstr_btcdyn_msw_row1, sizeof(btc_thr_data_t));
			i += wlc_btc_dynctl_init_trow(btc, &dprof->msw_data[2],
				rstr_btcdyn_msw_row2, sizeof(btc_thr_data_t));
			i += wlc_btc_dynctl_init_trow(btc, &dprof->msw_data[3],
				rstr_btcdyn_msw_row3, sizeof(btc_thr_data_t));

			/* number of initialized table rows must match to specified in nvram */
			if (i != dprof->msw_rows) {
				BTCDBG(("btcdyn incorrect nr of mode switch rows (%d)\n", i));
				goto rst2_dflt;
			}

			/*  initialize up to 4 rows in desense sw table */
			i = wlc_btc_dynctl_init_trow(btc, &dprof->dsns_data[0],
				rstr_btcdyn_dsns_row0, sizeof(btc_thr_data_t));
			i += wlc_btc_dynctl_init_trow(btc, &dprof->dsns_data[1],
				rstr_btcdyn_dsns_row1, sizeof(btc_thr_data_t));
			i += wlc_btc_dynctl_init_trow(btc, &dprof->dsns_data[2],
				rstr_btcdyn_dsns_row2, sizeof(btc_thr_data_t));
			i += wlc_btc_dynctl_init_trow(btc, &dprof->dsns_data[3],
				rstr_btcdyn_dsns_row3, sizeof(btc_thr_data_t));

			/* number of initialized table rows must match to specified in nvram */
			if (i != dprof->dsns_rows) {
				BTCDBG(("btcdyn incorrect nr of dynamic desense rows (%d)\n", i));
				goto rst2_dflt;
			}

			BTCDBG(("btcdyn profile has been loaded from nvram - Ok\n"));
		} else {
			rst2_dflt:
			WL_ERROR(("wl%d: %s: nvram.txt: missing or bad btcdyn profile vars."
				" do init from default\n", wlc->pub->unit, __FUNCTION__));

			/* all dynctl features are disabled until changed by iovars */
			dprof->flags = DCTL_FLAGS_DISABLED;

			/* initialize default profile */
			dprof->dflt_dsns_level = DESENSE_OFF;
			dprof->low_dsns_level = DESENSE_OFF;
			dprof->mid_dsns_level = DFLT_DESENSE_MID;
			dprof->high_dsns_level = DFLT_DESENSE_HIGH;
			dprof->default_btc_mode = WL_BTC_HYBRID;
			dprof->msw_rows = DCTL_TROWS;
			dprof->dsns_rows = DCTL_TROWS;
			btcdyn->sim_btpwr = BT_INVALID_TX_PWR;
			btcdyn->sim_wlrssi = WLC_RSSI_INVALID;

			/*  sanity check for the table sizes */
			ASSERT(sizeof(dflt_msw_data) <=
				(DCTL_TROWS_MAX * sizeof(btc_thr_data_t)));
			bcopy(dflt_msw_data, dprof->msw_data, sizeof(dflt_msw_data));
			ASSERT(sizeof(dflt_dsns_data) <=
				(DCTL_TROWS_MAX * sizeof(btc_thr_data_t)));
			bcopy(dflt_dsns_data,
				dprof->dsns_data, sizeof(dflt_dsns_data));
		}
		/* set btc_mode to default value */
		wlc_hw->btc->mode = dprof->default_btc_mode;
	} else {
		WL_ERROR(("wl%d: %s: MALLOC for dprof failed, %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			MFREE(wlc->osh, btcdyn, sizeof(wlc_btcdyn_info_t));
			return BCME_NOMEM;
	}
	return BCME_OK;
}

#ifdef DBG_BTPWR_HOLES
static void btcdyn_detect_btpwrhole(wlc_btc_info_t *btc, int8 cur_pwr)
{
	static pwrs_t pwr_smpl[4] = {{-127, 0}, {-127, 0}, {-127, 0}, {-127, 0}};
	static uint8 pwr_idx = 0;
	int32 cur_ts;

	cur_ts =  OSL_SYSUPTIME();
	pwr_smpl[pwr_idx].pwr = cur_pwr;
	pwr_smpl[pwr_idx].ts = cur_ts;

	/* detect a hole (an abnormality) in PWR sampling sequence */
	if ((pwr_smpl[pwr_idx].pwr != BT_INVALID_TX_PWR) &&
		(pwr_smpl[(pwr_idx-1) & 0x3].pwr == BT_INVALID_TX_PWR) &&
		(pwr_smpl[(pwr_idx-2) & 0x3].pwr != BT_INVALID_TX_PWR)) {

		DYNCTL_ERROR(("BTPWR hole at T-1:%d, delta from T-2:%d\n"
			" btpwr:[t, t-1, t-2]:%d,%d,%d\n", pwr_smpl[(pwr_idx-1) & 0x3].ts,
			(pwr_smpl[(pwr_idx-1) & 0x3].ts - pwr_smpl[(pwr_idx-2) & 0x3].ts),
			pwr_smpl[pwr_idx].pwr, pwr_smpl[(pwr_idx-1) & 0x3].pwr,
			pwr_smpl[(pwr_idx-2) & 0x3].pwr));

	}
	pwr_idx = (pwr_idx + 1) & 0x3;
}
#endif /* DBG_BTPWR_HOLES */
#endif /* WL_BTCDYN */

/* Read bt rssi from shm and do moving average */
static void
wlc_btc_update_btrssi(wlc_btc_info_t *btc)
{
	wlc_info_t *wlc = btc->wlc;
	uint16 btrssi_shm;
	int16 btrssi_avg = 0;
	int8 cur_rssi = 0, old_rssi;

	if (!btc->bth_active) {
		wlc_btc_reset_btrssi(btc);
		return;
	}

	/* read btrssi idx from shm */
	btrssi_shm = wlc_bmac_read_shm(wlc->hw, M_BTCX_RSSI(wlc));

	if (btrssi_shm) {
		/* clear shm because ucode keeps max btrssi idx */
		wlc_bmac_write_shm(wlc->hw, M_BTCX_RSSI(wlc), 0);

		/* actual btrssi = -1 x (btrssi x BT_RSSI_STEP + BT_RSSI_OFFSET) */
		cur_rssi = (-1) * (int8)(btrssi_shm * BTC_BTRSSI_STEP + BTC_BTRSSI_OFFSET);

		/* # of samples max out at btrssi_maxsamp */
		if (btc->btrssi_cnt < btc->btrssi_maxsamp)
			btc->btrssi_cnt++;

		/* accumulate & calc moving average */
		old_rssi = btc->btrssi_sample[btc->btrssi_idx];
		btc->btrssi_sample[btc->btrssi_idx] = cur_rssi;
		/* sum = -old one, +new  */
		btc->btrssi_sum = btc->btrssi_sum - old_rssi + cur_rssi;
		ASSERT(btc->btrssi_cnt);
		btrssi_avg = btc->btrssi_sum / btc->btrssi_cnt;

		btc->btrssi_idx = MODINC_POW2(btc->btrssi_idx, btc->btrssi_maxsamp);

		if (btc->btrssi_cnt < btc->btrssi_minsamp) {
			btc->bt_rssi = BTC_BTRSSI_INVALID;
			return;
		}

		btc->bt_rssi = (int8)btrssi_avg;
	}
}

static void
wlc_btc_reset_btrssi(wlc_btc_info_t *btc)
{
	memset(btc->btrssi_sample, 0, sizeof(int8)*BTC_BTRSSI_MAX_SAMP);
	btc->btrssi_cnt = 0;
	btc->btrssi_idx = 0;
	btc->btrssi_sum = 0;
	btc->bt_rssi = BTC_BTRSSI_INVALID;
}

#ifdef BCMULP
static int wlc_ulp_btcx_enter_cb(void *handle, ulp_ext_info_t *einfo)
{
	wlc_btc_info_t *btc = (wlc_btc_info_t *)handle;
	wlc_info_t *wlc = btc->wlc;
	bool btcx_en = 0;
	uint16 host_flags;
	int btc_mode =  wlc_btc_mode_get(wlc);
	if (!wlc->clk) {
		return BCME_NOCLK;
	}
	/* Check if DS1 BTCX needs to be enabled */
	if (btc_mode != WL_BTC_DISABLE)
	{
		if (CHSPEC_IS2G(wlc->chanspec)) {
			btcx_en = 1;
		}
	}
	wlc_ulp_configure_ulp_features(wlc->ulp, C_BT_COEX, (btcx_en)? TRUE: FALSE);
	host_flags = wlc_bmac_read_shm(wlc->hw, M_HOST_FLAGS5(wlc));

#ifdef BTCX_PRISEL_WAR_43012
	/* 43012: WAR for BTCX issue in 5G
	* Keep the shared antenna to WLAN always if BTCX is disabled
	*/
	host_flags &= ~(MHF5_BTCX_DEFANT);
#else
	/* Configure BTC GPIOs as bands change */
	if (CHSPEC_IS5G(wlc->chanspec)) {
		host_flags |= (MHF5_BTCX_DEFANT);
	} else {
		host_flags &= ~(MHF5_BTCX_DEFANT);
	}
#endif

	wlc_bmac_write_shm(wlc->hw, M_HOST_FLAGS5(wlc), host_flags);
	return BCME_OK;
}
#endif /* BCMULP */
