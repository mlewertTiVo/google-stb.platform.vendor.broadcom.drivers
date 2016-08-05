/*
 * WiFi multi channel source file
 * Broadcom 802.11abg Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_mchan.c 524316 2015-01-06 13:54:20Z $
 */


#include <wlc_cfg.h>

#ifdef WLMCHAN

#ifndef WLMCNX
#error "WLMCNX must be defined when WLMCHAN is defined!"
#endif

#ifndef WLP2P
#error "WLP2P must be defined when WLMCHAN is defined!"
#endif

#ifndef WL_MULTIQUEUE
#error "WL_MULTIQUEUE must be defined when WLMCHAN is defined!"
#endif


#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <bcmwpa.h>
#include <bcmdevs.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_key.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_scan.h>
#include <wlc_bmac.h>
#include <wl_export.h>
#include <wlc_utils.h>
#include <wlc_mcnx.h>
#include <wlc_tbtt.h>
#include <wlc_p2p.h>
#include <wlc_mchan.h>
#ifdef WLTDLS
#include <wlc_tdls.h>
#endif /* WLTDLS */
#include <wlc_hrt.h>
#ifdef PROP_TXSTATUS
#include <wlfc_proto.h>
#include <wlc_ampdu.h>
#include <wlc_apps.h>
#endif
#include <wlc_btcx.h>
#include <wlc_scb_ratesel.h>
#if defined(WL_MODESW)
#include <wlc_modesw.h>
#endif
#if defined(WL_PROT_OBSS)
#include <wlc_prot_obss.h>
#endif

/* iovar table */
enum {
	IOV_MCHAN,		/* enable/disable multi channel feature */
	IOV_MCHAN_STAGO_DISAB,	/* enable/disable multi channel sta-go feature */
	IOV_MCHAN_PRETBTT,	/* get/set mchan pretbtt time */
	IOV_MCHAN_PROC_TIME,	/* get/set mchan proc time */
	IOV_MCHAN_CHAN_SWITCH_TIME,	/* get/set mchan chan switch time */
	IOV_MCHAN_BLOCKING_ENAB,	/* get/set mchan blocking enab */
	IOV_MCHAN_BLOCKING_THRESH,	/* get/set mchan blocking thresh */
	IOV_MCHAN_DELAY_FOR_PM,	/* get/set mchan delay for pm feature */
	IOV_MCHAN_BYPASS_PM,	/* get/set mchan bypass pm feature */
	IOV_MCHAN_BYPASS_DTIM,	/* get/set mchan bypass dtim rule feature for chan sw */
	IOV_MCHAN_TEST,		/* JHCHIANG test iovar */
	IOV_MCHAN_SCHED_MODE,	/* get/set mchan sched mode */
	IOV_MCHAN_MIN_DUR,	/* get/set mchan minimum duration in a channel */
	IOV_MCHAN_SWTIME,
	IOV_MCHAN_ALGO,
	IOV_MCHAN_BW,
	IOV_MCHAN_BCNPOS,
	IOV_MCHAN_BCN_OVLP_OPT,
	IOV_MCHAN_WAIT_PM_TO
};

static const bcm_iovar_t mchan_iovars[] = {
	{"mchan", IOV_MCHAN, 0, IOVT_BOOL, 0},
	{"mchan_stago_disab", IOV_MCHAN_STAGO_DISAB, 0, IOVT_BOOL, 0},
	{"mchan_pretbtt", IOV_MCHAN_PRETBTT, 0, IOVT_UINT16, 0},
	{"mchan_proc_time", IOV_MCHAN_PROC_TIME, 0, IOVT_UINT16, 0},
	{"mchan_chan_switch_time", IOV_MCHAN_CHAN_SWITCH_TIME, 0, IOVT_UINT16, 0},
	{"mchan_blocking_enab", IOV_MCHAN_BLOCKING_ENAB, 0, IOVT_BOOL, 0},
	{"mchan_blocking_thresh", IOV_MCHAN_BLOCKING_THRESH, 0, IOVT_UINT8, 0},
	{"mchan_delay_for_pm", IOV_MCHAN_DELAY_FOR_PM, 0, IOVT_BOOL, 0},
	{"mchan_bypass_pm", IOV_MCHAN_BYPASS_PM, 0, IOVT_BOOL, 0},
	{"mchan_bypass_dtim", IOV_MCHAN_BYPASS_DTIM, 0, IOVT_BOOL, 0},
	{"mchan_test", IOV_MCHAN_TEST, 0, IOVT_UINT32, 0},
	{"mchan_sched_mode", IOV_MCHAN_SCHED_MODE, 0, IOVT_UINT32, 0},
	{"mchan_min_dur", IOV_MCHAN_MIN_DUR, 0, IOVT_UINT32, 0},
	{"mchan_swtime", IOV_MCHAN_SWTIME, 0, IOVT_UINT32, 0},
	{"mchan_algo", IOV_MCHAN_ALGO, 0, IOVT_UINT32, 0},
	{"mchan_bw", IOV_MCHAN_BW, 0, IOVT_UINT32, 0},
	{"mchan_bcnpos", IOV_MCHAN_BCNPOS, 0, IOVT_UINT32, 0},
	{"mchan_bcnovlpopt", IOV_MCHAN_BCN_OVLP_OPT, 0, IOVT_BOOL, 0},
	{"mchan_pm_timeout", IOV_MCHAN_WAIT_PM_TO, 0, IOVT_UINT32, 0},
	{NULL, 0, 0, 0, 0}
};

#define MCHAN_MIN_BCN_RX_TIME_US	3000U
#define MCHAN_MIN_DTIM_RX_TIME_US	12000U
#define MCHAN_MIN_RX_TX_TIME		12000U
#define MCHAN_MIN_CHANNEL_PERIOD(m)	(WLC_MCHAN_PRETBTT_TIME_US(m) + MCHAN_MIN_RX_TX_TIME)
#define MCHAN_MIN_TBTT_GAP_US_NEW(m)	(MCHAN_MIN_RX_TX_TIME + WLC_MCHAN_PRETBTT_TIME_US(m))
#define MCHAN_MIN_CHANNEL_TIME(m)	(MCHAN_MIN_RX_TX_TIME + WLC_MCHAN_PRETBTT_TIME_US(m))

/* local constants */
#define MCHAN_MAX_PM_NULL_TX_TIME_US	5000 /* max time for sending pm null data pkt to AP */

#define MCHAN_WAIT_FOR_PM_TO		5000 /* time to delay chan switch by for pm transition */

/* NOTE: Preclose is available only for full-dongle:mchan+proptxstatus */
/* Need to have different values for SDIO3.0  */
#define MCHAN_PRE_INTERFACE_CLOSE       6000 /* time to advance closing interface */
/* MchanPreClose enab macro; local to mchan only */
#ifdef WLMCHANPRECLOSE
#define MCHAN_PRECLOSE_ENAB(m) ((m)->pre_interface_close_enable)
#define MCHAN_PRECLOSE_ENABLE(m, val) ((m)->pre_interface_close_enable = val)
#else
#define MCHAN_PRECLOSE_ENAB(m) (0)
#endif

#define MCHAN_TBTT_SINCE_BCN_THRESHOLD	10
#define MCHAN_CFG_IDX_INVALID		(-1)
#define MCHAN_MIN_AP_TBTT_GAP_US	(30000)
#define MCHAN_FAIR_SCHED_DUR_ADJ_US	(100)

#ifdef WLC_HIGH_ONLY
#define MCHAN_MIN_STA_TBTT_GAP_US	(52000)
#define MCHAN_MIN_TBTT_TX_TIME_US	(8000)
#define MCHAN_MIN_TBTT_RX_TIME_US	(8000)
/* #define MCHAN_TIMER_DELAY_US		(5000) */
#else
#define MCHAN_MIN_STA_TBTT_GAP_US	(30000)
#define MCHAN_MIN_TBTT_TX_TIME_US	(12000)
#define MCHAN_MIN_TBTT_RX_TIME_US	(12000)
/* #define MCHAN_TIMER_DELAY_US		(200) */
#endif /* WLC_HIGH_ONLY */

#define MCHAN_MAX_TBTT_GAP_DRIFT_MS	(1000)
#define MCHAN_MIN_DUR_US		(5000)
#define MCHAN_MAX_FREE_SCHED_ELEM	(10)

#define MCHAN_TIMING_ALIGN(time)	((time) & (~((1 << P2P_UCODE_TIME_SHIFT) - 1)))

#define MCHAN_DEFAULT_ALGO 0
#define MCHAN_BANDWIDTH_ALGO 1
#define MCHAN_SI_ALGO 2
#define MCHAN_DYNAMIC_BW_ALGO 3

#define SI_DYN_SW_INTVAL (25 * 1024)
#define MCHAN_BCN_INVALID 0
#define MCHAN_BCN_OVERLAP 1
#define MCHAN_BCN_NONOVERLAP 2
#define MCHAN_BLOCKING_CNT 3 /* Value to be changed for enhancement  */
#define MCHAN_MIN_PM_WAIT_TO 1 /* value for Minimum PM timeout */
#define MCHAN_MAX_PM_WAIT_TO 5000 /* value for Maximum PM timeout */

#define DEFAULT_SI_TIME (25 * 1024)


#define SWAP(a, b) do {  a ^= b; b ^= a; a ^= b; } while (0)
#define SWAP_PTR(T, a, b) do \
	{ \
		T* temp; \
		temp = a; a = b; b = temp; \
	} while (0)

#define DYN_ENAB(mchan) (mchan->switch_algo == MCHAN_DYNAMIC_BW_ALGO)

#define MCHAN_DYN_ALGO_SAMPLE	10 /* No of sample in case of dynamic algo */
#define MCHAN_DYN_ALGO_MAX_IF	2
#define IF_PRIMARY	0
#define IF_SECONDARY	1
#define MCHAN_DYN_ALGO_STEP_SIZE	4
#define MCHAN_DYN_ALGO_MAX_BW	80
#define MCHAN_DYN_ALGO_MIN_BW	20
#define MCHAN_DYN_INI_BW		50
#define MCHAN_DYN_LOAD_INVALID	(-1)
#define MIN_THRESHOLD 5000

typedef struct _mchan_if_bytes {
	uint32 txbytes[MCHAN_DYN_ALGO_SAMPLE];	/* Total Number of TX bytes per if */
	uint32 rxbytes[MCHAN_DYN_ALGO_SAMPLE];	/* Total Number of RX bytes per if */
	uint32 tot_txbytes;				/* Cached TX bytes */
	uint32 tot_rxbytes;				/* Cached RX bytes */
	uint8 w;
} mchan_if_bytes_t;

typedef struct _mchan_fw_time {
	uint32 period[MCHAN_DYN_ALGO_SAMPLE];	/* Allocated time */
	uint32 w;
} mchan_fw_time_t;

typedef struct mchan_dyn_algo {
	mchan_if_bytes_t if_bytes[MCHAN_DYN_ALGO_MAX_IF];	/* Holds value for
							* actual byte count (RX-TX) per interface
							*/
	int32 maxload[MCHAN_DYN_ALGO_MAX_IF];
	int8 bw_dyn; /* from primary IF perspective */
	int8 bw_dyn_saved;
	int8 bw_dyn_diff;
	uint8 bw_dyn_wait_count;
} mchan_dyn_algo_t;

/* local enumeration types */
typedef enum
{
	FORCE_ADOPT,
	NORMAL_ADOPT
} wlc_mchan_adopt_type;

/* local structure defines */

/**
 * this structure is used to type cast structures used in
 * mchan module as a list of some sort to allow re-use of
 * add and delete code.
 */
typedef struct _mchan_list_elem
{
	struct _mchan_list_elem *next;
} mchan_list_elem_t;

typedef struct _mchan_sched_elem
{
	struct _mchan_sched_elem *next;  /* Keep this as first member of structure
				       * See comments for mchan_list_elem_t struct
				       */
	int8 bss_idx;
	uint32 duration;
	uint32 end_tsf_l;		/* ideal end(expire) time for this timer element */
} mchan_sched_elem_t;

/** mchan module specific state */
struct mchan_info {
	wlc_info_t *wlc;				/* wlc structure */
	mchan_sched_elem_t *mchan_sched;		/* ptr to mchan schedule elements */
	mchan_sched_elem_t *mchan_sched_free;		/* ptr to mchan free schedule elements */
	mchan_sched_elem_t *mchan_sched_free_list;	/* used for freeing the memory allocated */
	wlc_hrt_to_t *mchan_abs_to;			/* used for putting gc in ps mode b4 abs */
	wlc_hrt_to_t *mchan_to;			/* mchan timeout object for use with
							 * gptimer.
							 */
	wlc_hrt_to_t *mchan_special_to;		/* mchan timeout object for use with delayed
							 * channel switch to accommodate AP bug
							 * of not immediately honoring PM mode
							 * for STA.
							 */
	wlc_mchan_context_t *mchan_context_list;	/* mchan context list */
	int8 trigger_cfg_idx;				/* bsscfg idx used for scheduling */
	int8 other_cfg_idx;
	int8 last_missed_cfg_idx;			/* last bsscfg tbtt missed index */

	int8 special_bss_idx;				/* bss idx saved for delay pm chan switch */
	int8 abs_bss_idx;				/* bss idx saved for abs ps mode */
	int8 pm_pending_bss_idx;			/* bss index to go to after completion */
	wlc_mchan_context_t *pm_pending_ctx;		/* context to go to after completion */
	wlc_mchan_context_t *pm_pending_curr_chan_ctxt;	/* current context we're going away from */
	bool pm_pending;				/* pending PM transition */
	uint8 tsf_adj_wait_cnt;				/* cnt to wait for after tsf adj */
	uint32 tbtt_gap_gosta;				/* saved tbtt gap info between go and sta */
	uint32 noa_dur;					/* saved noa_dur for sta-go operation */
	uint16 pretbtt_time_us;				/* pretbtt time in uS */
	uint16 proc_time_us;				/* processing time for mchan in uS
							 * includes channel switch time and
							 * pm transition time
							 */
	uint16 chan_switch_time_us;			/* channel switch time in uS */
	bool disable_stago_mchan;			/* disable sta go multi-channel operation */
	uint8 tbtt_since_bcn_thresh;			/* threshold for num tbtt w/o beacon */
	wlc_bsscfg_t *blocking_bsscfg;			/* bsscfg that is blocking channel switch */
	bool blocking_enab;
	bool delay_for_pm;				/* whether we delay for pm transition */
	bool bypass_pm;					/* whether we wake sta when returing to
							 * sta's channel.
							 */
	bool bypass_dtim;				/* whether we ignore dtim rules when
							 * performing channel switches.
							 */
	wlc_mchan_sched_mode_t sched_mode;		/* schedule mode: fair,
	                                                 * favor sta, favor p2p
							 */
	uint32 min_dur;					/* minimum duration to stay in a channel */
	int8 last_missed_DTIM_idx;
	uint8 missed_cnt[2];
	uint32 switch_interval;
	uint32 switch_algo;
	uint32 percentage_bw;
	uint32 bcn_position;
	uint8 trigg_algo;         /* trigger algo's only if there is a change in parameters */
	dot11_quiet_t	mchan_quiet_cmd;
	uint32 si_algo_last_switch_time;
#ifdef WLMCHANPRECLOSE
	uint8 pre_interface_close_enable; /* mchan timeout object for closing */
	uint16 pre_interface_close_time;
	wlc_hrt_to_t *mchan_pre_interface_close; /* the proptxstatus interface */
							/* before switching channels */
#endif /* WLMCHANPRECLOSE */
	uint32 prep_pm_start_time;
	bool bcnovlpopt;		/* Variable to store ioctl value  */
	bool overlap_algo_switch;	/* whether overlap beacon caused a switch to SI algo. */
	uint32 noa_start_ref;		/* follows current tsf for dynamic pclose and regular to */
	uint32 abs_cnt;		/* The number of abscences within a given beacon int */
	uint8 blocking_cnt;
	uint32 chansw_timestamp;
	mchan_dyn_algo_t *dyn_algo;
	wlc_hrt_to_t *mchan_pm_wait_to;
	uint32 pm_wait_to; /* Time needed to wait before forcing the sta to PM */
	int wait_sta_valid;
	bcm_notif_h mchan_notif_hdl;
};

/* local macros */
#define MCHAN_AS_IN_PROGRESS(wlc)	((wlc)->block_datafifo & DATA_BLOCK_JOIN)
#define MCHAN_ADOPT_CONTEXT_ALLOWED(wlc) \
((!MCHAN_AS_IN_PROGRESS((wlc)) && !wlc_mchan_as_sta_waiting_for_bcn((wlc))) || \
(wlc)->mchan->pm_pending)

#define MCHAN_SI_ALGO_BCN_LOW_PSC_1 (28 * 1024)	/* Lower limit for GO beacon in 1st psc */
#define MCHAN_SI_ALGO_BCN_LOW_PSC_2 (47 * 1024)	/* Lower limit for GO beacon in 2nd psc */

/*  Optimum pos for GO bcn */
/* - beacon 25 to 50ms - psc  */
#define MCHAN_SI_ALGO_BCN_OPT_PSC_1 (38 * 1024)

#define MAX_WAIT_STA_VALID			10

/* local prototypes */
static int wlc_mchan_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid,
                             const char *name, void *p, uint plen, void *a,
                             int alen, int vsize, struct wlc_if *wlcif);
static int wlc_mchan_watchdog(void *context);
static int wlc_mchan_up(void *context);
static int wlc_mchan_down(void *context);
static void wlc_mchan_enab(mchan_info_t *mchan, bool enable);
static void wlc_mchan_list_add_head(mchan_list_elem_t **list, mchan_list_elem_t *elem);
static void wlc_mchan_list_del(mchan_list_elem_t **list, mchan_list_elem_t *elem);
static int8 wlc_mchan_set_other_cfg_idx(mchan_info_t *mchan);
static void wlc_mchan_multichannel_upd(wlc_info_t *wlc, mchan_info_t *mchan);
static void wlc_mchan_timer(void *arg);
static void wlc_mchan_timer_wait(void *arg);
static void wlc_mchan_timer_special(void *arg);
#ifdef WLMCHANPRECLOSE
static void wlc_mchan_timer_preclose(void *arg);
#endif
static bool wlc_mchan_qi_exist(wlc_info_t *wlc, wlc_txq_info_t *qi);
static wlc_txq_info_t *wlc_mchan_get_qi_with_no_context(wlc_info_t *wlc, mchan_info_t *mchan);
static wlc_mchan_context_t *wlc_mchan_context_alloc(mchan_info_t *mchan, chanspec_t chanspec);
static void wlc_mchan_context_free(mchan_info_t *mchan, wlc_mchan_context_t *chan_ctxt);
static bool wlc_mchan_as_sta_waiting_for_bcn(wlc_info_t *wlc);
static wlc_mchan_context_t *wlc_mchan_get_chan_context(mchan_info_t *mchan, chanspec_t chanspec);
static int wlc_mchan_sched_init(mchan_info_t *mchan);
static void wlc_mchan_sched_deinit(mchan_info_t *mchan);
static mchan_sched_elem_t *wlc_mchan_sched_alloc(mchan_info_t *mchan);
static void wlc_mchan_sched_free(mchan_info_t *mchan, mchan_sched_elem_t *sched);
static mchan_sched_elem_t *wlc_mchan_sched_get(mchan_info_t *mchan);
static void wlc_mchan_sched_delete(mchan_info_t *mchan, mchan_sched_elem_t* sched);
static void wlc_mchan_calc_fair_chan_switch_sched(uint32 next_tbtt1,
                                                  uint32 curr_tbtt2,
                                                  uint32 next_tbtt2,
                                                  uint32 bcn_per,
                                                  bool tx,
                                                  uint32 *start_time1,
                                                  uint32 *duration1);
static int wlc_mchan_noa_setup(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint32 bcn_per,
                               uint32 curr_ap_tbtt, uint32 next_ap_tbtt,
                               uint32 next_sta_tbtt, uint32 tbtt_gap, bool create);
static int
wlc_mchan_noa_setup_multiswitch(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint32 interval,
                    uint32 tbtt_gap, uint32 sel_dur, uint32 startswitch_for_abs, uint8 cnt);
static bool wlc_mchan_dtim_chan_select(wlc_info_t *wlc, bool sel_is_dtim, bool oth_is_dtim,
                                       wlc_bsscfg_t *bsscfg_sel, wlc_bsscfg_t *bsscfg_oth);
static void wlc_mchan_adopt_bss_chan_context(wlc_info_t *wlc, wlc_mchan_context_t *chan_ctxt,
                                             wlc_mchan_adopt_type adopt_type);
static int wlc_mchan_ap_pretbtt_proc(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
                                     uint32 tsf_l, uint32 tsf_h);
static int wlc_mchan_sta_pretbtt_proc(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
                                      uint32 tsf_l, uint32 tsf_h);
static void wlc_mchan_pm_block_all(mchan_info_t *mchan, mbool val, bool set);
static void wlc_mchan_cancel_pm_mode(mchan_info_t *mchan, wlc_mchan_context_t *ctx);
static void wlc_mchan_delay_for_pm(mchan_info_t *mchan);
static bool wlc_mchan_context_all_cfg_chanspec_is_same(mchan_info_t *mchan,
                                                       wlc_mchan_context_t *chan_ctxt,
                                                       chanspec_t chanspec);
static int _wlc_mchan_delete_bss_chan_context(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
                                              uint *oldqi_info);
static void wlc_mchan_psc_cleanup(mchan_info_t *mchan, wlc_bsscfg_t *cfg);

static void wlc_mchan_pretbtt_cb(void *ctx, wlc_mcnx_intr_data_t *notif_data);
static void _wlc_mchan_pretbtt_cb(void *ctx, wlc_mcnx_intr_data_t *notif_data);

static uint16 wlc_mchan_pretbtt_time_us(mchan_info_t *mchan);
#ifndef WLC_HIGH_ONLY
static uint16 wlc_mchan_proc_time_us(mchan_info_t *mchan);
#endif
static void wlc_mchan_dyn_algo(mchan_info_t *mchan);
static void wlc_mchan_dyn_algo_reset(mchan_info_t *mchan);

#ifdef WLC_HIGH_ONLY
static uint16 wlc_mchan_proc_time_bmac_us(mchan_info_t *mchan);
#endif /* WLC_HIGH_ONLY */
static void
wlc_mchan_context_set_chan(mchan_info_t *mchan, wlc_mchan_context_t *ctx,
	chanspec_t chanspec, bool destroy_old);
static void wlc_mchan_if_byte(wlc_info_t *wlc, uint8 idx);
#if defined(WL_PROT_OBSS) && defined(WL_MODESW)
static void wlc_mchan_notif_cb_notif(mchan_info_t *mchan, wlc_bsscfg_t *cfg,
	int status, int signal);
#endif

uint32 mchan_test_var = 0x0401;

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>


void *wlc_mchan_get_blocking_bsscfg(mchan_info_t *mchan)
{
	return (void *)mchan->blocking_bsscfg;
}

void wlc_mchan_reset_blocking_bsscfg(mchan_info_t *mchan)
{
	mchan->blocking_bsscfg = NULL;
	mchan->blocking_cnt = 0;
}

bool wlc_mchan_blocking_enab(mchan_info_t *mchan)
{
	return (mchan->blocking_enab);
}

void wlc_mchan_blocking_set(mchan_info_t *mchan, bool enable)
{
	mchan->blocking_enab = enable;
}

bool wlc_mchan_delay_for_pm_set(mchan_info_t *mchan, bool enable)
{
	bool prev_delay_for_pm = mchan->delay_for_pm;

	mchan->delay_for_pm = enable;
	if (mchan->delay_for_pm != prev_delay_for_pm) {
		/* clear the saved tbtt_gap value to force go to recreate noa sched */
		mchan->tbtt_gap_gosta = 0;
	}

	return prev_delay_for_pm;
}


uint8 wlc_mchan_tbtt_since_bcn_thresh(mchan_info_t *mchan)
{
	return (mchan->tbtt_since_bcn_thresh);
}

/** get value for mchan stago feature */
bool wlc_mchan_stago_is_disabled(mchan_info_t *mchan)
{
	return (mchan->disable_stago_mchan);
}

/* module attach/detach */
mchan_info_t *
BCMATTACHFN(wlc_mchan_attach)(wlc_info_t *wlc)
{
	mchan_info_t *mchan;
	bcm_notif_module_t *notif;
	/* sanity check */

	/* module states */
	if ((mchan = (mchan_info_t *)MALLOC(wlc->osh, sizeof(mchan_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	bzero((char *)mchan, sizeof(mchan_info_t));
	/* setup tbtt_since_bcn threshold */
	mchan->tbtt_since_bcn_thresh = MCHAN_TBTT_SINCE_BCN_THRESHOLD;
	/* enable chansw blocking */
	mchan->blocking_enab = TRUE;
	/* invalidate the cfg idx */
	mchan->trigger_cfg_idx = MCHAN_CFG_IDX_INVALID;
	mchan->other_cfg_idx = MCHAN_CFG_IDX_INVALID;
	mchan->last_missed_cfg_idx = MCHAN_CFG_IDX_INVALID;
	mchan->special_bss_idx = MCHAN_CFG_IDX_INVALID;

	mchan->wlc = wlc;

	/* dynamic  algo */
	if ((mchan->dyn_algo = (mchan_dyn_algo_t *)MALLOC(wlc->osh, sizeof(mchan_dyn_algo_t)))
			== NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	wlc_mchan_dyn_algo_reset(mchan);

	/* initialize pretbtt time */
	mchan->pretbtt_time_us = WLC_MCHAN_PRETBTT_TIME_US(mchan);
	/* pretbtt_time_us must be integer times of (1<<P2P_UCODE_TIME_SHIFT)us */
	mchan->pretbtt_time_us = MCHAN_TIMING_ALIGN(mchan->pretbtt_time_us);
	mchan->pm_wait_to = 2000; /* default value */

	/* initialize mchan proc time */
	if ((wlc->pub->sih->boardvendor == VENDOR_APPLE && wlc->pub->sih->boardtype == 0x0093)) {
		mchan->proc_time_us = WLC_MCHAN_PROC_TIME_LONG_US(mchan);
	}
	else {
#ifdef WLC_HIGH_ONLY
		mchan->proc_time_us = WLC_MCHAN_PROC_TIME_BMAC_US(mchan);
#else
		mchan->proc_time_us = WLC_MCHAN_PROC_TIME_US(mchan) + mchan->pm_wait_to;
#endif
	}
	/* proc_time_us must be integer times of (1<<P2P_UCODE_TIME_SHIFT)us */
	mchan->proc_time_us = MCHAN_TIMING_ALIGN(mchan->proc_time_us);

	/* initialize mchan channel switch time */
	mchan->chan_switch_time_us = WLC_MCHAN_CHAN_SWITCH_TIME_US(mchan);
	/* channel switch time must be integer times of (1<<P2P_UCODE_TIME_SHIFT)us */
	mchan->chan_switch_time_us = MCHAN_TIMING_ALIGN(mchan->chan_switch_time_us);


#ifdef WLC_HIGH_ONLY
	mchan->bypass_dtim = TRUE;
#endif

	/* schedule mode is fair by default */
	mchan->sched_mode = WLC_MCHAN_SCHED_MODE_FAIR;
	mchan->min_dur = MCHAN_MIN_DUR_US;

	if (wlc_module_register(wlc->pub, mchan_iovars, "mchan", mchan, wlc_mchan_doiovar,
	                        wlc_mchan_watchdog, wlc_mchan_up, wlc_mchan_down)) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}


	/* allocate timeout */
	if (!(mchan->mchan_to = wlc_hrt_alloc_timeout(wlc->hrti))) {
		WL_ERROR(("wl%d: wlc_hrt_alloc_timeout for mchan_to failed\n", wlc->pub->unit));
		goto fail;
	}
	if (!(mchan->mchan_special_to = wlc_hrt_alloc_timeout(wlc->hrti))) {
		WL_ERROR(("wl%d: wlc_hrt_alloc_timeout for mchan_special_to failed\n",
		          wlc->pub->unit));
		goto fail;
	}
	if (!(mchan->mchan_abs_to = wlc_hrt_alloc_timeout(wlc->hrti))) {
		WL_ERROR(("wl%d: wlc_hrt_alloc_timeout for mchan_abs_to failed\n",
		          wlc->pub->unit));
		goto fail;
	}
	if (!(mchan->mchan_pm_wait_to = wlc_hrt_alloc_timeout(wlc->hrti))) {
		WL_ERROR(("wl%d: wlc_hwtimer_alloc_timeout for mchan_wait_to failed\n",
		          wlc->pub->unit));
		goto fail;
	}
	/* pre-allocate mchan schedule free list */
	if (wlc_mchan_sched_init(mchan) != BCME_OK) {
		WL_ERROR(("wl%d: wlc_mchan_sched_init failed\n", wlc->pub->unit));
		goto fail;
	}
#ifdef WLMCHANPRECLOSE
	if (!(mchan->mchan_pre_interface_close = wlc_hrt_alloc_timeout(wlc->hrti))) {
		WL_ERROR(("wl%d: wlc_hrt_alloc_timeout for mchan_pre_interface_close failed\n",
			wlc->pub->unit));
		goto fail;
	}

	MCHAN_PRECLOSE_ENABLE(mchan, TRUE);
	mchan->pre_interface_close_time = MCHAN_PRE_INTERFACE_CLOSE;
#endif

#ifdef WLMCNX
	/* register preTBTT callback */
	if (wlc->mcnx) {
		if ((wlc_mcnx_intr_register(wlc->mcnx, wlc_mchan_pretbtt_cb, wlc)) != BCME_OK) {
			WL_ERROR(("wl%d: wlc_mcnx_intr_register failed (mchan)\n",
			          wlc->pub->unit));
			goto fail;
		}
	}
#endif

	/* enable multi channel */
	wlc_mchan_enab(mchan, TRUE);
	mchan->switch_interval = 50 * 1024;
	mchan->switch_algo = MCHAN_DEFAULT_ALGO;
	mchan->percentage_bw = 50;
	/* create notification list update. */
	notif = wlc->notif;
	ASSERT(notif != NULL);
	if (bcm_notif_create_list(notif, &mchan->mchan_notif_hdl) != BCME_OK) {
		WL_ERROR(("wl%d: %s: mchan bcm_notif_create_list() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	return mchan;

fail:
	/* error handling */
	wlc_mchan_detach(mchan);

	return NULL;
}

void
BCMATTACHFN(wlc_mchan_detach)(mchan_info_t *mchan)
{
	wlc_info_t *wlc;

	if (mchan == NULL)
	{
		return;
	}

	wlc = mchan->wlc;

	/* disable multi channel */
	wlc_mchan_enab(mchan, FALSE);

	/* Bring it down */
	wlc_mchan_down((void*)mchan);
	/* free mchan timeout object */
	if (mchan->mchan_to) {
		wlc_mchan_stop_timer(mchan, MCHAN_TO_REGULAR);
		wlc_hrt_free_timeout(mchan->mchan_to);
	}
	if (mchan->mchan_special_to) {
		wlc_mchan_stop_timer(mchan, MCHAN_TO_SPECIAL);
		wlc_hrt_free_timeout(mchan->mchan_special_to);
	}
	if (mchan->mchan_abs_to) {
		wlc_mchan_stop_timer(mchan, MCHAN_TO_ABS);
		wlc_hrt_free_timeout(mchan->mchan_abs_to);
	}
	if (mchan->mchan_pm_wait_to) {
		wlc_mchan_stop_timer(mchan, MCHAN_TO_WAITPMTIMEOUT);
		wlc_hrt_free_timeout(mchan->mchan_pm_wait_to);
	}
#ifdef WLMCHANPRECLOSE
	if (mchan->mchan_pre_interface_close) {
		wlc_mchan_stop_timer(mchan, MCHAN_TO_PRECLOSE);
		wlc_hrt_free_timeout(mchan->mchan_pre_interface_close);
	}
#endif

	/* delete all mchan schedule elements */
	wlc_mchan_sched_delete_all(mchan);

	/* Free memory used for mchan dynamic algo */
	MFREE(wlc->osh, mchan->dyn_algo, sizeof(mchan_dyn_algo_t));

	/* delete all mchan contexts */
	while (mchan->mchan_context_list) {
		wlc_mchan_context_free(mchan, mchan->mchan_context_list);
	}

	/* free memory for free mchan schedule list */
	wlc_mchan_sched_deinit(mchan);

	/* unregister callback */
#ifdef WLMCNX
	if (wlc->mcnx)
		wlc_mcnx_intr_unregister(wlc->mcnx, wlc_mchan_pretbtt_cb, wlc);
#endif

	/* unregister module */
	wlc_module_unregister(wlc->pub, "mchan", mchan);
	if (mchan->mchan_notif_hdl != NULL) {
		bcm_notif_delete_list(&mchan->mchan_notif_hdl);
	}
	MFREE(wlc->osh, mchan, sizeof(mchan_info_t));
}

static void wlc_mchan_dyn_algo_reset(mchan_info_t *mchan)
{
	bzero((char *)mchan->dyn_algo, sizeof(mchan_dyn_algo_t));
	mchan->dyn_algo->bw_dyn = MCHAN_DYN_INI_BW;
}

static void wlc_mchan_dyn_algo(mchan_info_t *mchan)
{

	uint32 primary_load = 0, secondary_load = 0;
	int i, j;
	uint32 if_txbytes[MCHAN_DYN_ALGO_MAX_IF] = {0, 0};
	uint32 if_rxbytes[MCHAN_DYN_ALGO_MAX_IF] = {0, 0};
	uint32 if_totbytes[MCHAN_DYN_ALGO_MAX_IF] = {0, 0};
	uint32 sta_possible_change = 0;
	uint32 p2p_possible_change = 0;

	if (mchan->dyn_algo->bw_dyn_wait_count > 0) {
		mchan->dyn_algo->bw_dyn_wait_count--;
		return;
	}

	/* Find avg values of data collected over last MCHAN_DYN_ALGO_SAMPLE samples */
	for (j = 0; j <= 1; j++) {
		for (i = 0; i < MCHAN_DYN_ALGO_SAMPLE; i++) {
			if_txbytes[j] += (mchan->dyn_algo->if_bytes[j].txbytes[i]
				/ MCHAN_DYN_ALGO_SAMPLE);
			if_rxbytes[j] += (mchan->dyn_algo->if_bytes[j].rxbytes[i]
				/ MCHAN_DYN_ALGO_SAMPLE);
		}
		/* Calculate Total Bytes on each interface as RX+TX */
		if_totbytes[j] += (if_txbytes[j] + if_rxbytes[j]);
	}

	primary_load = if_totbytes[IF_PRIMARY];
	secondary_load = if_totbytes[IF_SECONDARY];

	if (primary_load < MIN_THRESHOLD)
		mchan->dyn_algo->maxload[IF_PRIMARY] = 0;
	if (secondary_load < MIN_THRESHOLD)
		mchan->dyn_algo->maxload[IF_SECONDARY] = 0;

	if ((secondary_load < MIN_THRESHOLD) && (primary_load < MIN_THRESHOLD)) {
		return;
	}

	if (secondary_load >= primary_load) {
		if (mchan->dyn_algo->maxload[IF_PRIMARY] > 0) {
			int8 saved_bw = mchan->dyn_algo->bw_dyn_saved;
			int8 expected_bw = saved_bw + mchan->dyn_algo->bw_dyn_diff;
			if (mchan->dyn_algo->bw_dyn_diff > 0) {
				sta_possible_change = (((mchan->dyn_algo->maxload[IF_PRIMARY] *
					expected_bw) / saved_bw) -
					mchan->dyn_algo->maxload[IF_PRIMARY]);
			}
			else {
				sta_possible_change = (mchan->dyn_algo->maxload[IF_PRIMARY] -
					((mchan->dyn_algo->maxload[IF_PRIMARY] *
					expected_bw) / saved_bw));
			}
			sta_possible_change /= 8;
		}
	}
	else {
		if (mchan->dyn_algo->maxload[IF_SECONDARY] >
			0) {
			int8 saved_bw = 100 - mchan->dyn_algo->bw_dyn_saved;
			int8 expected_bw =  saved_bw - mchan->dyn_algo->bw_dyn_diff;
			if (mchan->dyn_algo->bw_dyn_diff < 0) {
				p2p_possible_change =
					(((mchan->dyn_algo->maxload[IF_SECONDARY] *
					expected_bw) / saved_bw) -
					mchan->dyn_algo->maxload[IF_SECONDARY]);
			}
			else {
				p2p_possible_change = (mchan->dyn_algo->maxload[IF_SECONDARY] -
					((mchan->dyn_algo->maxload[IF_SECONDARY] *
					expected_bw) / saved_bw));
			}
			p2p_possible_change /= 8;
		}
	}

	WL_MCHAN(("wlc_mchan_dyn_algo() : primary_load %d secondary_load %d\n",
		primary_load, secondary_load));

	if ((mchan->dyn_algo->maxload[IF_PRIMARY] != MCHAN_DYN_LOAD_INVALID) &&
		(mchan->dyn_algo->maxload[IF_SECONDARY] != MCHAN_DYN_LOAD_INVALID)) {
		if (secondary_load >= primary_load) {
			if (primary_load > MIN_THRESHOLD) {
				if (mchan->dyn_algo->maxload[IF_PRIMARY] > 0) {
					if (primary_load > (mchan->dyn_algo->maxload[IF_PRIMARY] +
						sta_possible_change)) {
						mchan->dyn_algo->maxload[IF_PRIMARY] = primary_load;
						mchan->dyn_algo->bw_dyn_saved =
							(int8)mchan->percentage_bw;
						mchan->dyn_algo->bw_dyn =
							mchan->dyn_algo->bw_dyn_saved +
							MCHAN_DYN_ALGO_STEP_SIZE;
						mchan->dyn_algo->bw_dyn_diff =
							MCHAN_DYN_ALGO_STEP_SIZE;
					}
					else if (primary_load <
						(mchan->dyn_algo->maxload[IF_PRIMARY] -
						sta_possible_change)) {
						mchan->dyn_algo->maxload[IF_PRIMARY] = primary_load;
						mchan->dyn_algo->bw_dyn_saved =
							(int8)mchan->percentage_bw;
						mchan->dyn_algo->bw_dyn =
							mchan->dyn_algo->bw_dyn_saved -
							(MCHAN_DYN_ALGO_STEP_SIZE/2);
						mchan->dyn_algo->bw_dyn_diff =
							-1*(MCHAN_DYN_ALGO_STEP_SIZE/2);
					}
					else {
						int8 temp = mchan->dyn_algo->bw_dyn;
						mchan->dyn_algo->maxload[IF_PRIMARY] = primary_load;
						mchan->dyn_algo->bw_dyn =
							mchan->dyn_algo->bw_dyn_saved;
						mchan->dyn_algo->bw_dyn_saved = temp;
						mchan->dyn_algo->bw_dyn_diff =
							mchan->dyn_algo->bw_dyn - temp;
					}
				} else {
					mchan->dyn_algo->maxload[IF_PRIMARY] =
						MCHAN_DYN_LOAD_INVALID;
					mchan->dyn_algo->bw_dyn = MCHAN_DYN_INI_BW;
					mchan->dyn_algo->bw_dyn_saved = MCHAN_DYN_INI_BW;
					mchan->dyn_algo->bw_dyn_diff = 0;
					mchan->dyn_algo->bw_dyn_wait_count = 2;
				}
				mchan->dyn_algo->maxload[IF_SECONDARY] = MIN_THRESHOLD + 1;
			} else {
				mchan->dyn_algo->maxload[IF_SECONDARY] = 0;
				mchan->dyn_algo->bw_dyn = MCHAN_DYN_ALGO_MIN_BW;
				mchan->dyn_algo->bw_dyn_saved = MCHAN_DYN_ALGO_MIN_BW;
				mchan->dyn_algo->bw_dyn_diff = 0;
			}
		} else {
			if (secondary_load > MIN_THRESHOLD) {
				if (mchan->dyn_algo->maxload[IF_SECONDARY] > 0) {
					if (secondary_load >
						(mchan->dyn_algo->maxload[IF_SECONDARY] +
						p2p_possible_change)) {
						mchan->dyn_algo->maxload[IF_SECONDARY] =
							secondary_load;
						mchan->dyn_algo->bw_dyn_saved =
							(int8)mchan->percentage_bw;
						mchan->dyn_algo->bw_dyn =
							mchan->dyn_algo->bw_dyn_saved -
							MCHAN_DYN_ALGO_STEP_SIZE;
						mchan->dyn_algo->bw_dyn_diff =
							-1*MCHAN_DYN_ALGO_STEP_SIZE;
					}
					else if (secondary_load <
						(mchan->dyn_algo->maxload[IF_SECONDARY] -
						p2p_possible_change)) {
						mchan->dyn_algo->maxload[IF_SECONDARY] =
							secondary_load;
						mchan->dyn_algo->bw_dyn_saved =
							(int8)mchan->percentage_bw;
						mchan->dyn_algo->bw_dyn =
							mchan->dyn_algo->bw_dyn_saved +
							(MCHAN_DYN_ALGO_STEP_SIZE/2);
						mchan->dyn_algo->bw_dyn_diff =
							MCHAN_DYN_ALGO_STEP_SIZE/2;
					}
					else {
						int8 temp = mchan->dyn_algo->bw_dyn;
						mchan->dyn_algo->maxload[IF_SECONDARY] =
							secondary_load;
						mchan->dyn_algo->bw_dyn =
							mchan->dyn_algo->bw_dyn_saved;
						mchan->dyn_algo->bw_dyn_saved = temp;
						mchan->dyn_algo->bw_dyn_diff =
							mchan->dyn_algo->bw_dyn - temp;
					}
				} else {
					mchan->dyn_algo->maxload[IF_SECONDARY] =
						MCHAN_DYN_LOAD_INVALID;
					mchan->dyn_algo->bw_dyn = MCHAN_DYN_INI_BW;
					mchan->dyn_algo->bw_dyn_saved = MCHAN_DYN_INI_BW;
					mchan->dyn_algo->bw_dyn_diff = 0;
					mchan->dyn_algo->bw_dyn_wait_count = 2;
				}
				mchan->dyn_algo->maxload[IF_PRIMARY] = MIN_THRESHOLD + 1;
			} else {
				mchan->dyn_algo->maxload[IF_PRIMARY] = 0;
				mchan->dyn_algo->bw_dyn = MCHAN_DYN_ALGO_MAX_BW;
				mchan->dyn_algo->bw_dyn_saved = MCHAN_DYN_ALGO_MAX_BW;
				mchan->dyn_algo->bw_dyn_diff = 0;
			}
		}
	} else {
		uint8 idx;
		wlc_bsscfg_t *cfg;
		ratespec_t rspec[MCHAN_DYN_ALGO_MAX_IF] = {0, 0};
		FOREACH_BSS(mchan->wlc, idx, cfg) {
			rspec[BSS_P2P_ENAB(mchan->wlc, cfg) ?
				IF_SECONDARY : IF_PRIMARY] =
				wlc_get_rspec_history(cfg);
		}
		rspec[IF_PRIMARY] = RSPEC2KBPS(rspec[IF_PRIMARY]);
		rspec[IF_SECONDARY] = RSPEC2KBPS(rspec[IF_SECONDARY]);

		if (mchan->dyn_algo->maxload[IF_PRIMARY] == MCHAN_DYN_LOAD_INVALID) {
			mchan->dyn_algo->maxload[IF_PRIMARY] = primary_load;
			if ((secondary_load / (RSPEC2KBPS(rspec[IF_SECONDARY]))) >=
				(primary_load / (RSPEC2KBPS(rspec[IF_PRIMARY]))))
				mchan->dyn_algo->bw_dyn = MCHAN_DYN_ALGO_MIN_BW;
			else
				mchan->dyn_algo->bw_dyn = MCHAN_DYN_ALGO_MAX_BW;
			mchan->dyn_algo->bw_dyn_saved = mchan->dyn_algo->bw_dyn;
			mchan->dyn_algo->bw_dyn_diff = 0;
		} else if (mchan->dyn_algo->maxload[IF_SECONDARY] == MCHAN_DYN_LOAD_INVALID) {
			mchan->dyn_algo->maxload[IF_SECONDARY] = secondary_load;
			if ((secondary_load / (RSPEC2KBPS(rspec[IF_SECONDARY]))) >=
				(primary_load / (RSPEC2KBPS(rspec[IF_PRIMARY]))))
				mchan->dyn_algo->bw_dyn = MCHAN_DYN_ALGO_MIN_BW;
			else
				mchan->dyn_algo->bw_dyn = MCHAN_DYN_ALGO_MAX_BW;
			mchan->dyn_algo->bw_dyn_saved = mchan->dyn_algo->bw_dyn;
			mchan->dyn_algo->bw_dyn_diff = 0;
		}
		mchan->dyn_algo->bw_dyn_wait_count = 2;
	}

	if (mchan->dyn_algo->bw_dyn >= MCHAN_DYN_ALGO_MAX_BW)
		mchan->dyn_algo->bw_dyn = MCHAN_DYN_ALGO_MAX_BW;
	else if (mchan->dyn_algo->bw_dyn <= MCHAN_DYN_ALGO_MIN_BW)
		mchan->dyn_algo->bw_dyn = MCHAN_DYN_ALGO_MIN_BW;

	if (mchan->dyn_algo->bw_dyn != mchan->percentage_bw) {
		/* Update the mchan->percentage_bw with newly calculated val */
		mchan->percentage_bw = mchan->dyn_algo->bw_dyn;
		/* Re-Trigger Bandwith Algorithm with new BW */
		mchan->trigg_algo = TRUE;
	}
}

static int
wlc_mchan_watchdog(void *context)
{
	mchan_info_t *mchan = (mchan_info_t *)context;
	wlc_info_t *wlc = mchan->wlc;
	int idx;
	wlc_bsscfg_t *cfg;

	FOREACH_AS_STA(wlc, idx, cfg) {
		if (BSS_P2P_ENAB(wlc, cfg) && cfg->last_psc_intr_time &&
		    ((wlc->pub->now - cfg->last_psc_intr_time) >= 2)) {
			/* noa schedule probably went away, cleanup */
			wlc_mchan_client_noa_clear(mchan, cfg);
		}
	}
	/* If DYN_ALGO is active then invoke wlc_mchan_dyn_algo */
	if (DYN_ENAB(mchan) && MCHAN_ACTIVE(wlc->pub))
		wlc_mchan_dyn_algo(mchan);

	return 0;
}

static int
wlc_mchan_up(void *context)
{
	mchan_info_t *mchan = (mchan_info_t *)context;
	wlc_info_t *wlc = mchan->wlc;

	(void)wlc;

	if (!MCHAN_ENAB(wlc->pub))
		return BCME_OK;

	/* clear all mchan timer and schedule elements */
	wlc_mchan_stop_timer(mchan, MCHAN_TO_REGULAR);
	wlc_mchan_stop_timer(mchan, MCHAN_TO_SPECIAL);
	wlc_mchan_stop_timer(mchan, MCHAN_TO_ABS);
#ifdef WLMCHANPRECLOSE
	wlc_mchan_stop_timer(mchan, MCHAN_TO_PRECLOSE);
#endif
	wlc_mchan_sched_delete_all(mchan);

	return BCME_OK;
}

static int
wlc_mchan_down(void *context)
{
	mchan_info_t *mchan = (mchan_info_t *)context;
	uint callbacks = 0;

	/* cancel the mchan timeout */
	wlc_mchan_stop_timer(mchan, MCHAN_TO_REGULAR);
	wlc_mchan_stop_timer(mchan, MCHAN_TO_SPECIAL);
	wlc_mchan_stop_timer(mchan, MCHAN_TO_ABS);
#ifdef WLMCHANPRECLOSE
	wlc_mchan_stop_timer(mchan, MCHAN_TO_PRECLOSE);
#endif

	if (mchan->pm_pending)
		wlc_mchan_cancel_pm_mode(mchan, mchan->pm_pending_ctx);

	return (callbacks);
}

/** handle mchan related iovars */
static int
wlc_mchan_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *p, uint plen, void *a, int alen, int vsize, struct wlc_if *wlcif)
{
	mchan_info_t *mchan = (mchan_info_t *)hdl;
	wlc_info_t *wlc = mchan->wlc;
	int32 int_val = 0;
	int err = BCME_OK;

	ASSERT(mchan == wlc->mchan);

	if (plen >= (int)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	/* all iovars require p2p being enabled */
	switch (actionid) {
	case IOV_GVAL(IOV_MCHAN):
	case IOV_SVAL(IOV_MCHAN):
		break;
	default:
		if (MCHAN_ENAB(wlc->pub))
			break;

		WL_ERROR(("wl%d: %s: iovar %s get/set requires mchan enabled\n",
		          wlc->pub->unit, __FUNCTION__, name));
		return BCME_ERROR;
	}

	switch (actionid) {
	case IOV_GVAL(IOV_MCHAN):
		*((uint32*)a) = wlc->pub->_mchan;
		break;
	case IOV_SVAL(IOV_MCHAN):
		wlc_mchan_enab(mchan, int_val != 0);
		break;
	case IOV_GVAL(IOV_MCHAN_STAGO_DISAB):
		*((uint32*)a) = mchan->disable_stago_mchan;
		break;
	case IOV_SVAL(IOV_MCHAN_STAGO_DISAB):
	        mchan->disable_stago_mchan = (int_val != 0);
		break;
	case IOV_GVAL(IOV_MCHAN_PRETBTT):
	        *((uint32*)a) = (uint32)mchan->pretbtt_time_us;
		break;
	case IOV_SVAL(IOV_MCHAN_PRETBTT):
	        mchan->pretbtt_time_us = MCHAN_TIMING_ALIGN(int_val);
		break;
	case IOV_GVAL(IOV_MCHAN_PROC_TIME):
	        *((uint32*)a) = (uint32)mchan->proc_time_us;
		break;
	case IOV_SVAL(IOV_MCHAN_PROC_TIME): {
		uint32 prev_proc_time = mchan->proc_time_us;

	        mchan->proc_time_us = MCHAN_TIMING_ALIGN(int_val);
		if (mchan->proc_time_us != prev_proc_time) {
			/* clear the saved tbtt_gap value to force go to recreate noa sched */
			mchan->tbtt_gap_gosta = 0;
		}
		break;
	}
	case IOV_GVAL(IOV_MCHAN_CHAN_SWITCH_TIME):
		*((uint32*)a) = (uint32)mchan->chan_switch_time_us;
		break;
	case IOV_SVAL(IOV_MCHAN_CHAN_SWITCH_TIME): {
		uint32 prev_chan_switch_time = mchan->chan_switch_time_us;

		mchan->chan_switch_time_us = MCHAN_TIMING_ALIGN(int_val);
		if (mchan->chan_switch_time_us != prev_chan_switch_time) {
			/* clear the saved tbtt_gap value to force go to recreate noa sched */
			mchan->tbtt_gap_gosta = 0;
		}
		break;
	}
	case IOV_GVAL(IOV_MCHAN_BLOCKING_ENAB):
	        *((uint32*)a) = (uint32)mchan->blocking_enab;
	        break;
	case IOV_SVAL(IOV_MCHAN_BLOCKING_ENAB):
	        mchan->blocking_enab = (int_val != 0);
	        break;
	case IOV_GVAL(IOV_MCHAN_BLOCKING_THRESH):
	        *((uint32*)a) = (uint32)mchan->tbtt_since_bcn_thresh;
	        break;
	case IOV_SVAL(IOV_MCHAN_BLOCKING_THRESH):
	        mchan->tbtt_since_bcn_thresh = (uint8)int_val;
	        break;
	case IOV_GVAL(IOV_MCHAN_DELAY_FOR_PM):
	        *((uint32*)a) = (uint32)mchan->delay_for_pm;
	        break;
	case IOV_SVAL(IOV_MCHAN_DELAY_FOR_PM): {
		bool prev_delay_for_pm = mchan->delay_for_pm;

	        mchan->delay_for_pm = (int_val != 0);
		if (mchan->delay_for_pm != prev_delay_for_pm) {
			/* clear the saved tbtt_gap value to force go to recreate noa sched */
			mchan->tbtt_gap_gosta = 0;
		}
	        break;
	}
	case IOV_GVAL(IOV_MCHAN_BYPASS_PM):
	        *((uint32*)a) = (uint32)mchan->bypass_pm;
	        break;
	case IOV_SVAL(IOV_MCHAN_BYPASS_PM):
	        mchan->bypass_pm = (int_val != 0);
		break;
	case IOV_GVAL(IOV_MCHAN_BYPASS_DTIM):
	        *((uint32*)a) = (uint32)mchan->bypass_dtim;
	        break;
	case IOV_SVAL(IOV_MCHAN_BYPASS_DTIM):
	        mchan->bypass_dtim = (int_val != 0);
		break;
	case IOV_GVAL(IOV_MCHAN_SCHED_MODE):
		*((uint32*)a) = (uint32)mchan->sched_mode;
		break;
	case IOV_SVAL(IOV_MCHAN_SCHED_MODE): {
		wlc_mchan_sched_mode_t prev_mode = mchan->sched_mode;

		if (int_val >= WLC_MCHAN_SCHED_MODE_MAX) {
			WL_ERROR(("wl%d: %s: iovar %s set invalid value\n",
				wlc->pub->unit, __FUNCTION__, name));
			err = BCME_ERROR;
			break;
		}
		mchan->sched_mode = (wlc_mchan_sched_mode_t)int_val;

		if (mchan->sched_mode != prev_mode) {
			/* clear the saved tbtt_gap value to force go to recreate noa sched */
			mchan->tbtt_gap_gosta = 0;
		}
		break;
	}
	case IOV_GVAL(IOV_MCHAN_MIN_DUR):
		*((uint32*)a) = (uint32)mchan->min_dur;
		break;
	case IOV_SVAL(IOV_MCHAN_MIN_DUR):
		mchan->min_dur = (uint32)int_val;
		break;
	case IOV_GVAL(IOV_MCHAN_TEST):
		*((uint32*)a) = mchan_test_var;
		break;
	case IOV_SVAL(IOV_MCHAN_TEST):
		mchan_test_var = (uint32)int_val;
	        /* code below is used for tx flow control testing */
	        if (mchan_test_var & 0x20) {
			if (wlc->bsscfg[mchan_test_var & 0xF] == NULL ||
			    wlc->bsscfg[mchan_test_var & 0xF]->associated == 0) {
				break;
			}
			wlc_txflowcontrol_override(wlc,
			                           wlc->bsscfg[mchan_test_var & 0xF]->wlcif->qi,
			                           (mchan_test_var & 0x10), TXQ_STOP_FOR_PKT_DRAIN);
		}
		break;
	case IOV_GVAL(IOV_MCHAN_SWTIME):
	        *((uint32*)a) = mchan->switch_interval;
		break;
	case IOV_SVAL(IOV_MCHAN_SWTIME):
		if ((uint32)int_val > (MCHAN_MIN_RX_TX_TIME + WLC_MCHAN_PRETBTT_TIME_US(mchan)) &&
			(int_val < 50000))
			mchan->switch_interval = (uint32)int_val;
		else
			err = BCME_RANGE;
		mchan->trigg_algo = TRUE;
		break;

	case IOV_GVAL(IOV_MCHAN_ALGO):
	        *((uint32*)a) = mchan->switch_algo;
		break;
	case IOV_SVAL(IOV_MCHAN_ALGO):
		if ((int_val >= 0) && (int_val < 4)) {
			if (int_val != mchan->switch_algo) {
				if (mchan->switch_algo == 3) {
					/* reset dyn algo components before leaving */
					wlc_mchan_dyn_algo_reset(mchan);
				}
				mchan->switch_algo = (uint32)int_val;
				mchan->trigg_algo = TRUE;
			}
		}
		else
			err = BCME_RANGE;
		break;
	case IOV_GVAL(IOV_MCHAN_BW):
		*((uint32 *)a) = mchan->percentage_bw;
		break;
	case IOV_SVAL(IOV_MCHAN_BW):
		if (DYN_ENAB(mchan))
			break;
		if ((int_val >= 0) && (int_val <= 100))
			mchan->percentage_bw = (uint32)int_val;
		else
			err = BCME_RANGE;
		mchan->trigg_algo = TRUE;
		break;
	case IOV_GVAL(IOV_MCHAN_WAIT_PM_TO):
		*((uint32 *)a) = mchan->pm_wait_to;
		break;
	case IOV_SVAL(IOV_MCHAN_WAIT_PM_TO):
		{
			uint32 old_state = mchan->pm_wait_to;
			if (int_val != old_state) {
				if (int_val) {
					if (int_val < MCHAN_MIN_PM_WAIT_TO) {
						mchan->pm_wait_to = (uint32)MCHAN_MIN_PM_WAIT_TO;
					}
					else if (int_val > MCHAN_MAX_PM_WAIT_TO) {
						mchan->pm_wait_to = (uint32)MCHAN_MAX_PM_WAIT_TO;
					}
					else {
						mchan->pm_wait_to = (uint32)int_val;
					}
#ifdef WLC_HIGH_ONLY
					mchan->proc_time_us = WLC_MCHAN_PROC_TIME_BMAC_US(mchan) +
						mchan->pm_wait_to;
#else
					mchan->proc_time_us = WLC_MCHAN_PROC_TIME_US(mchan) +
						mchan->pm_wait_to;
#endif
					mchan->proc_time_us =
						MCHAN_TIMING_ALIGN(mchan->proc_time_us);
				}
				else {
#ifdef WLC_HIGH_ONLY
					mchan->proc_time_us = WLC_MCHAN_PROC_TIME_BMAC_US(mchan);
#else
					mchan->proc_time_us = WLC_MCHAN_PROC_TIME_US(mchan);
#endif
					mchan->proc_time_us =
						MCHAN_TIMING_ALIGN(mchan->proc_time_us);
					wlc_mchan_stop_timer(mchan, MCHAN_TO_WAITPMTIMEOUT);
					mchan->pm_wait_to = 0;
				}
				mchan->trigg_algo = TRUE;
			}
		}
		break;
	case IOV_GVAL(IOV_MCHAN_BCNPOS):
		*((uint32 *)a) = mchan->bcn_position;
		break;
	case IOV_GVAL(IOV_MCHAN_BCN_OVLP_OPT):
		*((uint32 *)a) = mchan->bcnovlpopt;
		break;
	case IOV_SVAL(IOV_MCHAN_BCN_OVLP_OPT):
		if (int_val) {
			if (mchan->switch_algo == MCHAN_DEFAULT_ALGO)
				mchan->bcnovlpopt = TRUE;
		}
		else {
			if (mchan->overlap_algo_switch == TRUE) {
				mchan->bcnovlpopt = FALSE;
				mchan->overlap_algo_switch = FALSE;
				mchan->switch_algo = MCHAN_DEFAULT_ALGO;
			}
		}
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

static void
wlc_mchan_enab(mchan_info_t *mchan, bool enable)
{
	wlc_info_t *wlc = mchan->wlc;

	wlc->pub->_mchan = enable;
}


uint16 wlc_mchan_get_pretbtt_time(mchan_info_t *mchan)
{
	wlc_info_t *wlc = mchan->wlc;

	(void)wlc;

	return MCHAN_ENAB(wlc->pub) && MCHAN_ACTIVE(wlc->pub) ? mchan->pretbtt_time_us : 0;
}

/** adds elem to head of list */
static void
wlc_mchan_list_add_head(mchan_list_elem_t **list, mchan_list_elem_t *elem)
{
	elem->next = *list;
	*list = elem;
}

/** adds elem to tail of list */
static void
wlc_mchan_list_add_tail(mchan_list_elem_t **list, mchan_list_elem_t *elem)
{
	mchan_list_elem_t *curr_elem;

	/* first setup elem's next pointer to be NULL */
	elem->next = NULL;

	/* list is null, so point list to elem and we're done */
	if (*list == NULL) {
		*list = elem;
		return;
	}

	/* list is not null, traverse to end of list and add elem */
	curr_elem = *list;
	while (curr_elem->next != NULL) {
		curr_elem = curr_elem->next;
	}
	curr_elem->next = elem;
}

/** deletes elem from list */
static void
wlc_mchan_list_del(mchan_list_elem_t **list, mchan_list_elem_t *elem)
{
	mchan_list_elem_t *curr_elem, *prev_elem;

	curr_elem = *list;
	prev_elem = NULL;
	while (curr_elem != NULL) {
		/* found the one we want to delete, remove it from list */
		if (curr_elem == elem) {
			/* item to be removed is head of list */
			if (prev_elem == NULL) {
				/* reassign head of list */
				*list = elem->next;
			}
			else {
				prev_elem->next = elem->next;
			}
			break;
		}
		else {
			prev_elem = curr_elem;
			curr_elem = curr_elem->next;
		}
	}
}

static int8
wlc_mchan_set_other_cfg_idx(mchan_info_t *mchan)
{
	wlc_info_t *wlc = mchan->wlc;
	wlc_bsscfg_t *local_cfg;
	int idx;
	int8 cfgidx;

	if (mchan->trigger_cfg_idx == MCHAN_CFG_IDX_INVALID) {
		return (MCHAN_CFG_IDX_INVALID);
	}

	cfgidx = MCHAN_CFG_IDX_INVALID;
	FOREACH_BSS(wlc, idx, local_cfg) {
		if (local_cfg->chan_context ==
			wlc->bsscfg[mchan->trigger_cfg_idx]->chan_context) {
			continue;
		}

		if (local_cfg->chan_context != NULL) {
			cfgidx = local_cfg->_idx;
			break;
		}
	}

	return (cfgidx);
}

/**
 * Called on association or disassociation. If operation went from single to multichannel or vice
 * versa, ucode and firmware administration have to be updated.
 */
static void
wlc_mchan_multichannel_upd(wlc_info_t *wlc, mchan_info_t *mchan)
{
	bool oldstate = wlc->pub->_mchan_active;
	wlc_bsscfg_t *cfg;
	int idx;

	wlc->pub->_mchan_active = (mchan->mchan_context_list && mchan->mchan_context_list->next);

	/* need to reset mchan->wait_sta_valid upon update */
	mchan->wait_sta_valid = 0;

	if (!wlc->pub->_mchan_active && DYN_ENAB(mchan)) {
		wlc_mchan_dyn_algo_reset(mchan);
	}

	FOREACH_AS_BSS(wlc, idx, cfg) {
		wlc_tbtt_ent_init(wlc->tbtt, cfg);
	}
	if (SRHWVSDB_ENAB(wlc->pub)) {
		/* reset the engine so that previously saved stuff will be gone */
		wlc_srvsdb_reset_engine(wlc);

		/* Attach SRVSDB module */
		if (wlc->pub->_mchan_active) {
			struct wlc_srvsdb_info *srvsdb = wlc->srvsdb_info;

			srvsdb->vsdb_chans[0] = mchan->mchan_context_list->chanspec;
			srvsdb->vsdb_chans[1] = mchan->mchan_context_list->next->chanspec;
			if (wlc_bmac_activate_srvsdb(wlc->hw,
				mchan->mchan_context_list->chanspec,
				mchan->mchan_context_list->next->chanspec) != BCME_OK) {
				wlc_bmac_deactivate_srvsdb(wlc->hw);
			}
		} else {
			wlc_bmac_deactivate_srvsdb(wlc->hw);
		}
	}

	if (oldstate == wlc->pub->_mchan_active)
		return;

#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
		if (!wlc->pub->_mchan_active && mchan->mchan_context_list) {
			wlc_bsscfg_t *cfg;
			int idx;
			FOREACH_BSS(wlc, idx, cfg) {
				if (cfg->chan_context == mchan->mchan_context_list)
					break;
			}
			wlc_wlfc_mchan_interface_state_update(wlc, cfg,
				WLFC_CTL_TYPE_INTERFACE_OPEN, TRUE);
		}
	}
#endif

	wlc_mcnx_tbtt_adj_all(wlc->mcnx, 0, 0);

	wlc_bmac_enable_tbtt(wlc->hw, TBTT_MCHAN_MASK, wlc->pub->_mchan_active? TBTT_MCHAN_MASK:0);

	/* if we bypassed PM, we would like to restore it upon existing mchan operation */
	if (mchan->bypass_pm && !wlc->pub->_mchan_active) {
		int idx;
		wlc_bsscfg_t *cfg;

		FOREACH_AS_STA(wlc, idx, cfg) {
			wlc_pm_st_t *pm = cfg->pm;

			if (!pm->PMenabled) {
				if (pm->PM == PM_FAST) {
					/* Start the PM2 tick timer for the next tick */
					wlc_pm2_sleep_ret_timer_start(cfg);
				}
				else if (pm->PM == PM_MAX) {
					/* reenable ps mode */
					wlc_set_pmstate(cfg, TRUE);
				}
			}
		}
	}
}

static void
wlc_mchan_prepare_pm_mode(mchan_info_t *mchan, wlc_bsscfg_t *target_cfg, uint32 tsf_l)
{
	uint idx;
	wlc_bsscfg_t *cfg;
	wlc_info_t *wlc = mchan->wlc;
	wlc_mchan_context_t *curr_chan_ctxt;


#if defined(WLC_LOW) && defined(WLC_HIGH) && defined(MCHAN_MINIDUMP)
	wlc->mchan->prep_pm_start_time = R_REG(wlc->osh, &wlc->regs->tsf_timerlow);
#endif
	WL_MCHAN(("wl%d.%d: %s: 0x%x pm_pending: %d wlc->chanspec: 0x%x\n",
	          mchan->wlc->pub->unit,
	          WLC_BSSCFG_IDX(target_cfg),
	          __FUNCTION__,
	          target_cfg->chan_context->chanspec, mchan->pm_pending, wlc->chanspec));

	if (!MCHAN_ADOPT_CONTEXT_ALLOWED(mchan->wlc))
		return;

	/* If one is already pending then cancel it */
	if (mchan->pm_pending)
		return;

	/* Already on that channel or already off channel? */
	/* During scan, we can be off channel but the home_chanspec remains.
	 * Thus, it is better to check against wlc->home_chanspec instead of wlc->chanspec.
	 */
	if (WLC_MCHAN_SAME_CTLCHAN(target_cfg->chan_context->chanspec, wlc->home_chanspec) ||
	    !WLC_MCHAN_SAME_CTLCHAN(wlc->home_chanspec, wlc->chanspec)) {
		wlc_mchan_adopt_bss_chan_context(mchan->wlc, target_cfg->chan_context,
		                                 FORCE_ADOPT);
		return;
	}

	/* get the current context */
	curr_chan_ctxt = wlc_mchan_get_chan_context(mchan, wlc->chanspec);
	if (curr_chan_ctxt == NULL) {
		WL_MCHAN(("%s: current chanspec 0x%x does not have chan context!\n", __FUNCTION__,
		          wlc->chanspec));
		return;
	}

	/* check for blocking_bsscfg */
	if ((mchan->blocking_bsscfg) && (mchan->blocking_bsscfg != target_cfg)) {
			mchan->blocking_cnt--;
			if (mchan->blocking_cnt == 0) {
				WL_MCHAN(("wl%d.%d %s: chan switch blocked by bss %d\n",
					wlc->pub->unit, WLC_BSSCFG_IDX(target_cfg),
					__FUNCTION__, WLC_BSSCFG_IDX(mchan->blocking_bsscfg)));
				mchan->blocking_bsscfg = NULL;
			}
			return;
	}
	/* For all non-AP contexts that are not on the same channel as this BSS
	 * set pm state and wait for pm transition
	 */
	FOREACH_AS_STA(wlc, idx, cfg) {
		/* Skip the ones that are not on curr channel */
		if (cfg->chan_context != curr_chan_ctxt)
			continue;
		/* PM block any cfg with chan context that matches curr_chan_ctxt */
		mboolset(cfg->pm->PMblocked, WLC_PM_BLOCK_MCHAN_CHANSW);

		/* Also skip the ones that are already in PM mode
		 * with PMpending already acknowledged.
		 */
		if (cfg->pm->PMenabled && !cfg->pm->PMpending)
			continue;
		WL_MCHAN(("wl%d.%d: %s: "
		          "entering PM mode for off channel operation for 0x%x\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
		          target_cfg->chan_context->chanspec));
		if (!cfg->pm->PMenabled) {
			if (cfg->in_abs) {
				WL_ERROR(("wl%d.%d %s: in abs period, skip pmstate set!!!\n",
				          wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__));
			} else {
				wlc_module_set_pmstate(cfg, TRUE, WLC_BSSCFG_PS_REQ_MCHAN);
				/* Start timer for PM Timeout */
				if (mchan->pm_wait_to) {
					wlc_mchan_stop_timer(mchan, MCHAN_TO_WAITPMTIMEOUT);
					wlc_mchan_start_timer(mchan, mchan->pm_wait_to,
						MCHAN_TO_WAITPMTIMEOUT);
				}
			}
		}
		else {
			WL_MCHAN(("wl%d.%d: %s: already PMenabled %d, PMpending %d\n",
			          wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
			          cfg->pm->PMenabled, cfg->pm->PMpending));
		}
		/* Returns TRUE if PM mode transition would be pending for any of the BSSes
		 * that are not on the target_cfg context
		 */

		mchan->pm_pending_curr_chan_ctxt = curr_chan_ctxt;
		/* Only set mchan->pm_pending if our chan_context->PMpending is set */
		mchan->pm_pending = curr_chan_ctxt->pm_pending;
		if (mchan->pm_pending) {
			mchan->pm_pending_ctx = target_cfg->chan_context;
			mchan->pm_pending_bss_idx = WLC_BSSCFG_IDX(target_cfg);
		}
	}

	/* We are supposed to wait for PM0->PM1 transition to finish but in case
	 * we failed to send PM indications or failed to receive ACKs fake a PM0->PM1
	 * transition so that anything depending on the transition to finish can move
	 * forward i.e. scan engine can continue.
	 */
	if (mchan->pm_pending) {
		wlc_pm_pending_complete(mchan->wlc);
	}
	else if (mchan->delay_for_pm) {
		mchan->pm_pending_bss_idx = WLC_BSSCFG_IDX(target_cfg);
		wlc_mchan_delay_for_pm(mchan);
	}
	else
		wlc_mchan_adopt_bss_chan_context(mchan->wlc, target_cfg->chan_context,
		                                 FORCE_ADOPT);
}

static void
wlc_mchan_pm_block_all(mchan_info_t *mchan, mbool val, bool set)
{
	wlc_info_t *wlc = (wlc_info_t *)mchan->wlc;
	wlc_bsscfg_t *cfg;
	int idx;

	FOREACH_STA(wlc, idx, cfg) {
		if (set) {
			mboolset(cfg->pm->PMblocked, val);
		}
		else {
			mboolclr(cfg->pm->PMblocked, val);
		}
	}
}

/*
 * Based on the chan_context passed in, leave the PS mode for an AS STA if
 * the user setting says so
 */
static void
wlc_mchan_pm_mode_leave(mchan_info_t *mchan, wlc_mchan_context_t *chan_context)
{
	wlc_info_t *wlc = mchan->wlc;
	uint idx;
	wlc_bsscfg_t *cfg;

	FOREACH_AS_STA(wlc, idx, cfg) {
		/* Skip the ones that are not on our channel */
		if (cfg->chan_context != chan_context)
			continue;
		/* PM unblock any cfg with chan context that matches chan_ctxt */
		mboolclr(cfg->pm->PMblocked, WLC_PM_BLOCK_MCHAN_CHANSW);

		/* come out of PS mode if appropriate */
		/* come out of PS if last time STA did not 'complete' the transition */
		/* Logic is as follows:
		 * pmenabled is true &&
		 *	1. PM mode is OFF or
		 *	2. WME PM blocked or
		 *	3. bypass_pm feature is on
		 */
		if ((cfg->pm->PM == PM_OFF || cfg->pm->WME_PM_blocked || mchan->bypass_pm) &&
		    cfg->pm->PMenabled) {
			/* More checks for mchan active gc stas with noa schedule */
			if (!MCHAN_ACTIVE(wlc->pub) || !BSS_P2P_ENAB(wlc, cfg) || !cfg->in_abs) {
				wlc_set_pmstate(cfg, FALSE);
			}
		}
	}
}

/*
For MCHAN, only one of the interfaces is active at any given time. This provdes the opportunity
to setup BTCX protection mode according to the interface type - AP, STA, etc.
*/
static void
wlc_mchan_set_btcx_prot_mode(wlc_info_t *wlc, wlc_mchan_context_t *chan_context)
{
	uint idx;
	wlc_bsscfg_t *cfg;

	FOREACH_BSS(wlc, idx, cfg) {
		/* Skip the ones that are not on our channel */
		if (cfg->chan_context == chan_context) {
			int btc_flags = wlc_bmac_btc_flags_get(wlc->hw);
			uint16 protections = 0;
			uint16 ps = 0;
			uint16 active = 0;
#ifdef WLMCNX
			int bss_idx;
#endif /* WLMCNX */


			if (btc_flags & WL_BTC_FLAG_ACTIVE_PROT)
				active = MHF3_BTCX_ACTIVE_PROT;
			ps = (btc_flags & WL_BTC_FLAG_PS_PROTECT) ? MHF3_BTCX_PS_PROTECT : 0;

			if (BSSCFG_STA(cfg) && (cfg->BSS))
			{
				protections = ps | active;
			}
			else if (BSSCFG_AP(cfg) && (cfg->up))
			{
				protections = active;
			}
#ifdef WLMCNX
			if (MCNX_ENAB(wlc->pub)) {
				/*
				Set the current BSS's Index in SHM (bits 10:8 of M_P2P_GO_IND_BMP
				contain the current BSS Index
				*/
				bss_idx = wlc_mcnx_BSS_idx(wlc->mcnx, cfg);
				wlc_mcnx_shm_bss_idx_set(wlc->mcnx, bss_idx);
			}
#endif /* WLMCNX */

			if (MCHAN_ACTIVE(wlc->pub))
			{
				wlc_mhf(wlc, MHF3, MHF3_BTCX_ACTIVE_PROT | MHF3_BTCX_PS_PROTECT,
					protections, WLC_BAND_2G);
			}
			else
			{
				wlc_enable_btc_ps_protection(wlc, cfg, TRUE);
			}
			break;
		}
	}
}

static void
wlc_mchan_cancel_pm_mode(mchan_info_t *mchan, wlc_mchan_context_t *ctx)
{
	wlc_bsscfg_t *cfg;
	uint idx;
	wlc_info_t *wlc = mchan->wlc;

	if (ctx != mchan->pm_pending_ctx)
		return;

	/* Cancel pm state transition if there is one in progress */
	FOREACH_AS_STA(wlc, idx, cfg) {
		/* Skip the ones that are on our channel */
		if (cfg->chan_context == ctx)
			continue;

		/* come out of PS mode if appropriate */
		if (cfg->pm->PMpending)
			wlc_update_pmstate(cfg, TX_STATUS_BE);
	}

	mchan->pm_pending = FALSE;
	mchan->pm_pending_ctx = NULL;
}

static void
wlc_mchan_delay_for_pm(mchan_info_t *mchan)
{
	wlc_info_t *wlc = mchan->wlc;

	wlc_mchan_pm_block_all(mchan, WLC_PM_BLOCK_MCHAN_CHANSW, TRUE);
	wlc_mchan_start_timer(mchan, MCHAN_WAIT_FOR_PM_TO, MCHAN_TO_SPECIAL);
	mchan->special_bss_idx = mchan->pm_pending_bss_idx;
	wlc_txflowcontrol_override(wlc, wlc->bsscfg[mchan->special_bss_idx]->wlcif->qi,
	                           ON, TXQ_STOP_FOR_MCHAN_FLOW_CNTRL);
	WL_MCHAN(("wl%d: %s: wait for %d uS before changing channel to bss %d\n",
	          wlc->pub->unit, __FUNCTION__, MCHAN_WAIT_FOR_PM_TO,
	          mchan->special_bss_idx));
}

void
wlc_mchan_bss_prepare_pm_mode(mchan_info_t *mchan, wlc_bsscfg_t *target_cfg)
{
	wlc_mchan_prepare_pm_mode(mchan, target_cfg, 0);
}

void
wlc_mchan_pm_pending_complete(mchan_info_t *mchan)
{
	wlc_info_t *wlc = mchan->wlc;
	wlc_mchan_context_t *chan_ctxt;
	wlc_mchan_context_t *curr_chan_ctxt;

	if (!mchan->pm_pending) {
		ASSERT(mchan->pm_pending_ctx == NULL);
		return;
	}

	curr_chan_ctxt = mchan->pm_pending_curr_chan_ctxt;
	if (curr_chan_ctxt) {
		WL_MCHAN(("wl%d: %s: pending: 0x%x, curr: 0x%x, "
			"wlc->PMpending: %d, curr_chan_ctxt pm_pending = %d\n",
			wlc->pub->unit,
			__FUNCTION__,
			mchan->pm_pending_ctx->chanspec, curr_chan_ctxt->chanspec,
			wlc->PMpending, curr_chan_ctxt->pm_pending));
		if (curr_chan_ctxt->pm_pending)
			return;
	}

	ASSERT(mchan->pm_pending_ctx != NULL);
	/* Stop the PM timeout counter when an ACk is received */
	wlc_mchan_stop_timer(mchan, MCHAN_TO_WAITPMTIMEOUT);
	mchan->pm_pending = FALSE;
	chan_ctxt = mchan->pm_pending_ctx;
	mchan->pm_pending_ctx = NULL;
	mchan->pm_pending_curr_chan_ctxt = NULL;

	if (mchan->delay_for_pm) {
		wlc_mchan_delay_for_pm(mchan);
	}
	else {
		wlc_mchan_adopt_bss_chan_context(wlc, chan_ctxt, NORMAL_ADOPT);
		wlc_mchan_pm_block_all(mchan, WLC_PM_BLOCK_MCHAN_CHANSW, FALSE);
	}
}

static void
wlc_mchan_timer(void *arg)
{
	wlc_info_t *wlc = (wlc_info_t*)arg;
	mchan_sched_elem_t *mchan_sched;
	int8 bss_idx;
	wlc_bsscfg_t *bsscfg;
	wlc_mchan_context_t *chan_ctxt;
	mchan_info_t *mchan;
	bool stop_timer = TRUE;
	uint32 tsf_l = 0;

	if (!wlc->pub->up)
		return;

	if (DEVICEREMOVED(wlc)) {
		WL_ERROR(("wl%d: %s: dead chip\n", wlc->pub->unit, __FUNCTION__));
		wl_down(wlc->wl);
		return;
	}


	mchan = wlc->mchan;
	mchan_sched = wlc_mchan_sched_get(mchan);
	if (mchan_sched == NULL) {
		wlc_mchan_stop_timer(mchan, MCHAN_TO_REGULAR);
		return;
	}


	/* Need to go through some checks as this timer can fire after
	 * bsscfg or chan_context has been deleted.
	 */

	/* validate the bss_idx in schedule element */
	bss_idx = mchan_sched->bss_idx;
	if ((bss_idx < 0) || (bss_idx >= WLC_MAXBSSCFG)) {
		goto timer_finish;
	}
	/* validate bsscfg exists */
	bsscfg = wlc->bsscfg[bss_idx];
	if (bsscfg == NULL) {
		goto timer_finish;
	}
	/* validate chan_context exists */
	chan_ctxt = bsscfg->chan_context;
	if (chan_ctxt == NULL) {
		goto timer_finish;
	}
	/* adopt to the next context */
#ifdef WLTDLS
	if (BSS_TDLS_ENAB(wlc, bsscfg)) {
		wlc_tdls_do_chsw(wlc->tdls, bsscfg, TRUE);
#ifdef PROP_TXSTATUS
		wlc_wlfc_mchan_interface_state_update(wlc,
			bsscfg, WLFC_CTL_TYPE_MAC_OPEN, FALSE);
		if (!wlc_tdls_is_chsw_enabled(wlc->tdls)) {
			wlc_bsscfg_t *parent;
			parent = wlc_tdls_get_parent_bsscfg(wlc, bsscfg->tdls_scb);
			wlc_wlfc_mchan_interface_state_update(wlc,
			parent, WLFC_CTL_TYPE_INTERFACE_OPEN, FALSE);
		}
#endif
	}
	else
#endif /* WLTDLS */
	wlc_mchan_prepare_pm_mode(wlc->mchan, bsscfg, tsf_l);

	if (mchan_sched->duration) {
		uint32 oth_dur;

		oth_dur = R_REG(wlc->osh, &wlc->regs->tsf_timerlow);
		oth_dur = (oth_dur > mchan_sched->end_tsf_l) ?
		        (oth_dur - mchan_sched->end_tsf_l) : 0;
		oth_dur += MCHAN_TIMER_DELAY_US;
		WL_MCHAN(("%s: dur = %d, oth_dur = %d\n", __FUNCTION__,
		          mchan_sched->duration, oth_dur));
		oth_dur = (mchan_sched->duration > oth_dur) ?
		        (mchan_sched->duration - oth_dur) : 1;
		WL_MCHAN(("%s: new_dur = %d\n", __FUNCTION__, oth_dur));
		wlc_mchan_start_timer(mchan, oth_dur, MCHAN_TO_REGULAR);
		/* mark to not stop timer because we just restarted it */
		stop_timer = FALSE;
	}

timer_finish:
	/* no more timers needed, explicitly stop timers */
	if (stop_timer == TRUE) {
		wlc_mchan_stop_timer(mchan, MCHAN_TO_REGULAR);
	}
	/* delete the schedule element */
	wlc_mchan_sched_delete(mchan, mchan_sched);

}

static void
wlc_mchan_timer_special(void *arg)
{
	wlc_info_t *wlc = (wlc_info_t *)arg;
	wlc_bsscfg_t *bsscfg;
	wlc_mchan_context_t *chan_ctxt;
	int8 bss_idx;

	/* validate the bss_idx in schedule element */
	bss_idx = wlc->mchan->special_bss_idx;

	if ((bss_idx < 0) || (bss_idx >= WLC_MAXBSSCFG)) {
		WL_ERROR(("%s: bss_idx %d out of range!\n", __FUNCTION__, bss_idx));
		goto timer_finish_s;
	}
	/* validate bsscfg exists */
	bsscfg = wlc->bsscfg[bss_idx];
	if (bsscfg == NULL) {
		WL_ERROR(("%s: bsscfg is NULL!\n", __FUNCTION__));
		goto timer_finish_s;
	}
	/* validate chan_context exists */
	chan_ctxt = bsscfg->chan_context;
	if (chan_ctxt == NULL) {
		WL_ERROR(("%s: chan_ctxt is NULL!\n", __FUNCTION__));
		goto timer_finish_s;
	}

	WL_MCHAN(("%s: timer fired for chanspec 0x%x\n", __FUNCTION__,
	          chan_ctxt->chanspec));
	wlc_mchan_adopt_bss_chan_context(wlc, chan_ctxt, NORMAL_ADOPT);
	wlc_txflowcontrol_override(wlc, bsscfg->wlcif->qi, OFF, TXQ_STOP_FOR_MCHAN_FLOW_CNTRL);
timer_finish_s:
	wlc_mchan_stop_timer(wlc->mchan, MCHAN_TO_SPECIAL);

}

#ifdef WLMCHANPRECLOSE
static void
wlc_mchan_timer_preclose(void *arg)
{
	wlc_info_t *wlc = (wlc_info_t *)arg;

	wlc_mchan_stop_timer(wlc->mchan, MCHAN_TO_PRECLOSE);
#ifdef PROP_TXSTATUS
	wlc_wlfc_mchan_interface_state_update(wlc,
		wlc_mchan_get_cfg_frm_q(wlc, wlc->primary_queue),
		WLFC_CTL_TYPE_INTERFACE_CLOSE, FALSE);
#endif
}
#endif /* WLMCHANPRECLOSE */

static void
wlc_mchan_timer_wait(void *arg)
{
	wlc_info_t *wlc = (wlc_info_t *)arg;
	mchan_info_t *mchan = wlc->mchan;
	wlc_mchan_stop_timer(mchan, MCHAN_TO_WAITPMTIMEOUT);
	if (mchan->pm_pending) {
		wlc_bsscfg_t* pending_cfg;
		uint fake_txstatus;
		fake_txstatus = TX_STATUS_ACK_RCV;
		fake_txstatus |= TX_STATUS_PMINDCTD;
		if (mchan->pm_pending_curr_chan_ctxt) {
			pending_cfg = wlc_mchan_get_cfg_frm_q(wlc,
			mchan->pm_pending_curr_chan_ctxt->qi);
			wlc_update_pmstate(pending_cfg, fake_txstatus);
		}
	}
}

static void
wlc_mchan_timer_abs(void *arg)
{
	wlc_info_t *wlc = (wlc_info_t *)arg;
	mchan_info_t *mchan = wlc->mchan;
	wlc_bsscfg_t *bsscfg;
	wlc_mchan_context_t *chan_ctxt;
	int8 bss_idx;

	/* validate the bss_idx in schedule element */
	bss_idx = mchan->abs_bss_idx;

	if ((bss_idx < 0) || (bss_idx >= WLC_MAXBSSCFG)) {
		WL_ERROR(("%s: bss_idx %d out of range!\n", __FUNCTION__, bss_idx));
		goto timer_finish_a;
	}
	/* validate bsscfg exists */
	bsscfg = wlc->bsscfg[bss_idx];
	if (bsscfg == NULL) {
		WL_ERROR(("%s: bsscfg is NULL!\n", __FUNCTION__));
		goto timer_finish_a;
	}
	/* validate chan_context exists */
	chan_ctxt = bsscfg->chan_context;
	if (chan_ctxt == NULL) {
		WL_ERROR(("%s: chan_ctxt is NULL!\n", __FUNCTION__));
		goto timer_finish_a;
	}

	WL_MCHAN(("%s: timer fired for chanspec 0x%x\n", __FUNCTION__,
	          chan_ctxt->chanspec));
	if (bsscfg->in_abs) {
		WL_ERROR(("wl%d.%d %s: in abs period, skip pmstate set!!!\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
	} else
	/* make sure we're on channel and not PMenabled */
	if (!bsscfg->pm->PMenabled &&
	    (bsscfg->chan_context->chanspec == WLC_BAND_PI_RADIO_CHANSPEC)) {
		wlc_module_set_pmstate(bsscfg, TRUE, WLC_BSSCFG_PS_REQ_MCHAN);
	}
	mboolset(bsscfg->pm->PMblocked, WLC_PM_BLOCK_MCHAN_ABS);
timer_finish_a:
	wlc_mchan_stop_timer(mchan, MCHAN_TO_ABS);
}

void wlc_mchan_start_timer(mchan_info_t *mchan, uint32 to, mchan_to_type type)
{
	wlc_info_t *wlc = mchan->wlc;
	wlc_hrt_to_t *to_obj;
	wlc_hrt_to_cb_fn to_func;
	mbool requestor;

	switch (type) {
		case MCHAN_TO_SPECIAL:
			to_obj = mchan->mchan_special_to;
			to_func = wlc_mchan_timer_special;
			requestor = WLC_GPTIMER_AWAKE_MCHAN_SPECIAL;
			break;
		case MCHAN_TO_ABS:
			to_obj = mchan->mchan_abs_to;
			to_func = wlc_mchan_timer_abs;
			requestor = WLC_GPTIMER_AWAKE_MCHAN_ABS;
			break;
		case MCHAN_TO_WAITPMTIMEOUT:
			to_obj = mchan->mchan_pm_wait_to;
			to_func = wlc_mchan_timer_wait;
			requestor = WLC_GPTIMER_AWAKE_MCHAN_WAIT;
			break;
#ifdef WLMCHANPRECLOSE
		case MCHAN_TO_PRECLOSE:
			to_obj = mchan->mchan_pre_interface_close;
			to_func = wlc_mchan_timer_preclose;
			requestor = WLC_GPTIMER_AWAKE_MCHAN_PRECLOSE;
			break;
#endif
		case MCHAN_TO_REGULAR:
#ifdef WLMCHANPRECLOSE
			if (MCHAN_PRECLOSE_ENAB(mchan)) {
				to_obj = mchan->mchan_pre_interface_close;
				to_func = wlc_mchan_timer_preclose;
				wlc_hrt_del_timeout(to_obj);
				if (to < MCHAN_PRE_INTERFACE_CLOSE)
					to = MCHAN_PRE_INTERFACE_CLOSE;
				wlc_hrt_add_timeout(to_obj,
					to - mchan->pre_interface_close_time, to_func, wlc);
			}
#endif
			to_obj = mchan->mchan_to;
			to_func = wlc_mchan_timer;
			requestor = WLC_GPTIMER_AWAKE_MCHAN;
			break;

		default:
			to_obj = mchan->mchan_to;
			to_func = wlc_mchan_timer;
			requestor = WLC_GPTIMER_AWAKE_MCHAN;
			break;
	}
	wlc_hrt_del_timeout(to_obj);
	wlc_hrt_add_timeout(to_obj, to, to_func, wlc);
	wlc_gptimer_wake_upd(wlc, requestor, TRUE);
}

void wlc_mchan_stop_timer(mchan_info_t *mchan, mchan_to_type type)
{
	wlc_info_t *wlc = mchan->wlc;
	wlc_hrt_to_t *to_obj;
	mbool requestor;

	switch (type) {
		case MCHAN_TO_SPECIAL:
			to_obj = mchan->mchan_special_to;
			requestor = WLC_GPTIMER_AWAKE_MCHAN_SPECIAL;
			mchan->special_bss_idx = MCHAN_CFG_IDX_INVALID;
			wlc_mchan_pm_block_all(mchan, WLC_PM_BLOCK_MCHAN_CHANSW, FALSE);
			break;
		case MCHAN_TO_ABS:
			to_obj = mchan->mchan_abs_to;
			requestor = WLC_GPTIMER_AWAKE_MCHAN_ABS;
			mchan->abs_bss_idx = MCHAN_CFG_IDX_INVALID;
			break;
		case MCHAN_TO_WAITPMTIMEOUT:
			to_obj = mchan->mchan_pm_wait_to;
			requestor = WLC_GPTIMER_AWAKE_MCHAN_WAIT;
			break;
#ifdef WLMCHANPRECLOSE
		case MCHAN_TO_PRECLOSE:
			to_obj = mchan->mchan_pre_interface_close;
			requestor = WLC_GPTIMER_AWAKE_MCHAN_PRECLOSE;
			break;
#endif
		default:
			to_obj = mchan->mchan_to;
			requestor = WLC_GPTIMER_AWAKE_MCHAN;
			break;
	}
	wlc_hrt_del_timeout(to_obj);
	wlc_gptimer_wake_upd(wlc, requestor, FALSE);
}

bool
wlc_mchan_context_in_use(mchan_info_t *mchan, wlc_mchan_context_t *chan_ctxt,
	wlc_bsscfg_t *filter_cfg)
{
	wlc_info_t *wlc = mchan->wlc;
	int idx;
	wlc_bsscfg_t *cfg;

	ASSERT(chan_ctxt != NULL);

	FOREACH_BSS(wlc, idx, cfg) {
		if ((cfg != filter_cfg) && (cfg->chan_context == chan_ctxt)) {
			return (TRUE);
		}
	}

	return (FALSE);
}

static bool
wlc_mchan_qi_exist(wlc_info_t *wlc, wlc_txq_info_t *qi)
{
	wlc_txq_info_t *qi_list;

	/* walk through all queues */
	for (qi_list = wlc->tx_queues; qi_list; qi_list = qi_list->next) {
		if (qi == qi_list) {
			/* found it in the list, return true */
			return TRUE;
		}
	}

	return FALSE;
}

static wlc_txq_info_t *
wlc_mchan_get_qi_with_no_context(wlc_info_t *wlc, mchan_info_t *mchan)
{
	wlc_mchan_context_t *context_list;
	wlc_txq_info_t *qi_list;
	bool found;

	/* walk through all queues */
	for (qi_list = wlc->tx_queues; qi_list; qi_list = qi_list->next) {
		found = FALSE;
		/* walk through all context */
		context_list = mchan->mchan_context_list;
		for (; context_list; context_list = context_list->next) {
			if (context_list->qi == qi_list) {
				found = TRUE;
				break;
			}
		}
		/* make sure it is not excursion_queue */
		if (qi_list == wlc->excursion_queue) {
			found = TRUE;
		}
		/* if found is false, current qi is without context */
		if (!found) {
			WL_MCHAN(("found qi %p with no associated ctxt\n", qi_list));
			return (qi_list);
		}
	}

	return (NULL);
}

/* Take care of all the stuff needed when a ctx channel is set */
static void
wlc_mchan_context_set_chan(mchan_info_t *mchan, wlc_mchan_context_t *ctx,
	chanspec_t chanspec, bool destroy_old)
{
	if (destroy_old) {
		/* delete phy context for calibration */
		wlc_phy_destroy_chanctx(mchan->wlc->band->pi, ctx->chanspec);
	}
	ctx->chanspec = chanspec;

	/* create phy context for calibration */
	wlc_phy_create_chanctx(mchan->wlc->band->pi, chanspec);
}

static wlc_mchan_context_t *
wlc_mchan_context_alloc(mchan_info_t *mchan, chanspec_t chanspec)
{
	wlc_info_t *wlc = mchan->wlc;
	osl_t *osh = wlc->osh;
	wlc_mchan_context_t *chan_ctxt;
	wlc_txq_info_t *qi;
	wlc_mchan_context_t **mchan_context_list = &mchan->mchan_context_list;

	chan_ctxt = (wlc_mchan_context_t *)MALLOCZ(osh, sizeof(wlc_mchan_context_t));
	if (chan_ctxt == NULL) {
		WL_ERROR(("%s failed to allocate mem for chan context\n", __FUNCTION__));
		return (NULL);
	}

	qi = wlc_mchan_get_qi_with_no_context(wlc, mchan);
	/* if qi with no context found, then use it */
	if (qi) {
		chan_ctxt->qi = qi;
		WL_MCHAN(("use existing qi = %p\n", qi));
	}
	/* else allocate a new queue */
	else {
		chan_ctxt->qi = wlc_txq_alloc(wlc, osh);
		WL_MCHAN(("allocate new qi = %p\n", chan_ctxt->qi));
	}

	ASSERT(chan_ctxt->qi != NULL);
	wlc_mchan_context_set_chan(mchan, chan_ctxt, chanspec, FALSE);

	chan_ctxt->pm_pending = FALSE;

	/* add chan_ctxt to head of context list */
	wlc_mchan_list_add_head((mchan_list_elem_t **)mchan_context_list,
	                        (mchan_list_elem_t *)chan_ctxt);

	return (chan_ctxt);
}

static void
wlc_mchan_context_free(mchan_info_t *mchan, wlc_mchan_context_t *chan_ctxt)
{
	wlc_info_t *wlc = mchan->wlc;
	osl_t *osh = wlc->osh;
	wlc_txq_info_t *oldqi;
	chanspec_t chanspec = chan_ctxt->chanspec;
	wlc_mchan_context_t **mchan_context_list = &mchan->mchan_context_list;
	int qi_cfg_idx;
	wlc_bsscfg_t *qi_cfg;

	(void)osh;

	ASSERT(chan_ctxt != NULL);
	ASSERT(chan_ctxt != mchan->pm_pending_ctx);	/* should already be canceled */

	/* save the qi pointer, delete from context list and free context */
	oldqi = chan_ctxt->qi;
	wlc_mchan_list_del((mchan_list_elem_t **)mchan_context_list,
	                   (mchan_list_elem_t *)chan_ctxt);
	if (mchan->pm_pending_curr_chan_ctxt == chan_ctxt)
		mchan->pm_pending_curr_chan_ctxt = NULL;
	MFREE(osh, chan_ctxt, sizeof(wlc_mchan_context_t));

	/* delete phy context for calibration */
	wlc_phy_destroy_chanctx(wlc->band->pi, chanspec);

	/* continue with deleting this queue only if there are more contexts */
	if (mchan->mchan_context_list == NULL) {
		/* flush the queue */
		WL_MCHAN(("%s: our queue is only context queue, don't delete, "
			"simply flush the queue, qi 0x%p len %d\n",
			__FUNCTION__, oldqi, oldqi->q.len));
		pktq_flush(wlc->osh, &oldqi->q, TRUE, NULL, 0);
		ASSERT(pktq_empty(&oldqi->q));
		return;
	}

	/* if this queue is the primary_queue, detach it first */
	if (oldqi == wlc->primary_queue) {
		wlc_mchan_context_t *ctxt = mchan->mchan_context_list;
		wlc_mchan_adopt_bss_chan_context(wlc, ctxt, FORCE_ADOPT);
	}

	/* Before freeing this queue, any bsscfg that is using this queue
	 * should now use the primary queue.
	 * Active queue can be the excursion queue if we're scanning so using
	 * the primary queue is more appropriate.  When scan is done, active
	 * queue will be switched back to primary queue.
	 * This can be true for bsscfg's that never had contexts.
	 */
	FOREACH_BSS(wlc, qi_cfg_idx, qi_cfg) {
		if (qi_cfg->wlcif->qi == oldqi) {
			WL_MCHAN(("wl%d.%d: %s: cfg's qi %p is about to get deleted, "
			          "move to primary queue %p\n", wlc->pub->unit, qi_cfg->_idx,
			          __FUNCTION__, oldqi, wlc->primary_queue));
			qi_cfg->wlcif->qi = wlc->primary_queue;
		}
	}

	/* flush the queue */
	/* pktq_flush(wlc->osh, &oldqi->q, TRUE, NULL, 0); */
	{
		int prec;
		void * pkt;
		while ((pkt = pktq_deq(&oldqi->q, &prec))) {
#ifdef PROP_TXSTATUS
			if (PROP_TXSTATUS_ENAB(wlc->pub)) {
				wlc_process_wlhdr_txstatus(wlc,
					WLFC_CTL_PKTFLAG_DISCARD, pkt, FALSE);
			}
#endif /* PROP_TXSTATUS */
			PKTFREE(wlc->pub->osh, pkt, TRUE);
		}
	}
	ASSERT(pktq_empty(&oldqi->q));
	/* delete the queue */
	wlc_txq_free(wlc, wlc->osh, oldqi);
	WL_MCHAN(("%s, flush and free q\n", __FUNCTION__));
}

/* This function checks to see if the cfg needs an updated chan context
* Updates the chan context if so -- needed eg for roam
*/
int
wlc_mchan_update_bss_chan_context(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	chanspec_t chanspec)
{
	if (cfg->chan_context) {
		if (cfg->chan_context->chanspec != chanspec) {
			wlc_mchan_create_bss_chan_context(wlc, cfg, chanspec);
		}
	} else {
		ASSERT(cfg->chan_context);
	}
	return BCME_OK;

}

/**
 * This function will determine if any associated station on the current
 * channel is in the AS_WAIT_RCV_BCN state.
 * Returns true if such station exists, else returns false.
 */
static bool
wlc_mchan_as_sta_waiting_for_bcn(wlc_info_t *wlc)
{
	wlc_bsscfg_t *cfg;
	int i;
	wlc_mchan_context_t *curr_chan_ctxt;

	/* get the current context */
	curr_chan_ctxt = wlc_mchan_get_chan_context(wlc->mchan, wlc->chanspec);
	/* if current channel does not have context, return FALSE */
	if (curr_chan_ctxt == NULL) {
		return FALSE;
	}
	FOREACH_AS_STA(wlc, i, cfg) {
		if (cfg->chan_context && (cfg->chan_context == curr_chan_ctxt) &&
		    (cfg->assoc->state == AS_WAIT_RCV_BCN)) {
			WL_ERROR(("wl%d.%d: %s, cfg in AS_WAIT_RCV_BCN state, no chan switch\n",
			          wlc->pub->unit, cfg->_idx, __FUNCTION__));
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * This function returns channel context pointer based on chanspec passed in
 * If no context associated to chanspec, will return NULL.
 */
static wlc_mchan_context_t *
wlc_mchan_get_chan_context(mchan_info_t *mchan, chanspec_t chanspec)
{
	wlc_mchan_context_t *chan_ctxt = mchan->mchan_context_list;

	while (chan_ctxt) {
		if (WLC_MCHAN_SAME_CTLCHAN(chan_ctxt->chanspec, chanspec)) {
			break;
		}
		chan_ctxt = chan_ctxt->next;
	}

	return (chan_ctxt);
}

/**
 * Checks the chan_ctxt passed in for any bsscfg using this chan_ctxt that
 * is operating on a 40MHz chanspec.  Returns true is one is found.
 * Returns false if none is found.
 * chspec20 is used to get the 20MHz chanspec if there is a bsscfg using this
 * chan_ctxt that is operating in 20MHz mode.  ONLY USEFUL when all bsscfg
 * using our chan_ctxt are operating in 20MHz mode.
 */
bool mchan_chan_context_has_40mhz_cfg(wlc_info_t *wlc,
                                      wlc_mchan_context_t *chan_ctxt,
                                      chanspec_t *chspec20)
{
	int idx;
	wlc_bsscfg_t *cfg;

	FOREACH_BSS(wlc, idx, cfg) {
		if (chan_ctxt != cfg->chan_context) {
			continue;
		}
		if (CHSPEC_IS40(cfg->chanspec)) {
			/* we found a bsscfg that shares our chan context and this
			 * bsscfg is operating in 40MHz mode.
			 */
			return TRUE;
		} else if (chspec20 != NULL) {
			/* save the 20MHz chanspec value */
			*chspec20 = cfg->chanspec;
		}
	}
	return FALSE;
}

/**
 * Given a chan_context and chanspec as input, looks at all bsscfg using this
 * chan_context and see if bsscfg->chanspec matches exactly with chanspec.
 * Returns TRUE if all bsscfg->chanspec matches chanspec.  Else returns FALSE.
 */
static bool
wlc_mchan_context_all_cfg_chanspec_is_same(mchan_info_t *mchan,
                                           wlc_mchan_context_t *chan_ctxt,
                                           chanspec_t chanspec)
{
	wlc_info_t *wlc = mchan->wlc;
	int idx;
	wlc_bsscfg_t *cfg;

	ASSERT(chan_ctxt != NULL);

	FOREACH_BSS(wlc, idx, cfg) {
		if ((cfg->chan_context == chan_ctxt) &&
			!WLC_MCHAN_SAME_CTLCHAN(cfg->chanspec, chanspec)) {
			return (FALSE);
		}
	}

	return (TRUE);
}

/** when bsscfg becomes associated, call this function */
int
wlc_mchan_create_bss_chan_context(wlc_info_t *wlc, wlc_bsscfg_t *cfg, chanspec_t chanspec)
{
	mchan_info_t *mchan = wlc->mchan;
	wlc_mchan_context_t *chan_ctxt = cfg->chan_context;
	wlc_mchan_context_t *new_chan_ctxt;
	bool flowcontrol;
	wlc_txq_info_t *oldqi = cfg->wlcif->qi;
	uint oldqi_stopped_flag = 0;

	WL_MCHAN(("%s called on %s cfg %d chanspec 0x%x channel %d\n",
		__FUNCTION__, BSSCFG_AP(cfg) ? "AP" : "STA", WLC_BSSCFG_IDX(cfg),
		chanspec, CHSPEC_CHANNEL(chanspec)));

	/* fetch context for this chanspec */
	new_chan_ctxt = wlc_mchan_get_chan_context(mchan, chanspec);

	/* Assign chanspec to cfg */
	cfg->chanspec = chanspec;

	/* check old qi to see if we need to perform flow control on new qi.
	 * Only need to check TXQ_STOP_FOR_PKT_DRAIN because it is cfg specific.
	 * All other TXQ_STOP types are queue specific.
	 * Existing flow control setting only pertinent if we already have chan_ctxt.
	 */
	flowcontrol = ((chan_ctxt != NULL) &&
	               wlc_txflowcontrol_override_isset(wlc, cfg->wlcif->qi,
	                                                TXQ_STOP_FOR_PKT_DRAIN));

	/* This cfg has an existing context, do some checks to determine what to do */
	if (chan_ctxt) {
		WL_MCHAN(("%s, cfg alredy has context!\n", __FUNCTION__));
		if (WLC_MCHAN_SAME_CTLCHAN(chan_ctxt->chanspec, chanspec)) {
			/* we know control channel is same but if chanspecs are not
			 * identical, will want chan context to adopt to new chanspec,
			 * if no other cfg's using it or all cfg's using that context
			 * are using new chanspec.
			 */
			if ((chan_ctxt->chanspec != chanspec) &&
			    (!wlc_mchan_context_in_use(mchan, chan_ctxt, cfg) ||
			     wlc_mchan_context_all_cfg_chanspec_is_same(mchan, chan_ctxt,
			                                                chanspec)) &&
			     (chanspec == wlc_get_cur_wider_chanspec(wlc, cfg))) {

				WL_MCHAN(("context is the same but need to adopt "
				          "from chanspec 0x%x to 0x%x\n",
				          chan_ctxt->chanspec, chanspec));
#ifdef ENABLE_FCBS
				wlc_phy_fcbs_uninit(wlc->band->pi, chan_ctxt->chanspec);
				wlc_phy_fcbs_arm(wlc->band->pi, chanspec, 0);
#endif
				wlc_mchan_context_set_chan(mchan, chan_ctxt, chanspec,
				!BSSCFG_IS_MODESW_BWSW(cfg));
			}
			else {
				WL_MCHAN(("context is the same as new one, do nothing\n"));
			}
			/* already has context on the same chanspec, do nothing */
			return (BCME_OK);
		}
		/* Requested nonoverlapping channel. */
		else {
			if (!wlc_mchan_context_in_use(mchan, chan_ctxt, cfg)) {
				_wlc_mchan_delete_bss_chan_context(wlc, cfg, &oldqi_stopped_flag);
			}
		}
	}

	/* check to see if a context for this chanspec already exist */
	if (new_chan_ctxt == NULL) {
		/* context for this chanspec doesn't exist, create a new one */
		WL_MCHAN(("%s, allocate new context\n", __FUNCTION__));
		new_chan_ctxt = wlc_mchan_context_alloc(mchan, chanspec);
#ifdef ENABLE_FCBS
		if (!wlc_phy_fcbs_arm(wlc->band->pi, chanspec, 0))
			WL_ERROR(("%s: wlc_phy_fcbs_arm FAILed\n", __FUNCTION__));
#endif
	}

	ASSERT(new_chan_ctxt != NULL);

	if (new_chan_ctxt == NULL)
		return (BCME_NOMEM);

	/* assign context to cfg */
	cfg->wlcif->qi = new_chan_ctxt->qi;
	cfg->chan_context = new_chan_ctxt;

	/* if we switched queue/context, need to take care of flow control */
	if (oldqi != new_chan_ctxt->qi) {
		/* evaluate to see if we need to enable flow control for PKT_DRAIN
		 * If flowcontrol is true, this means our old chan_ctxt has flow control
		 * of type TXQ_STOP_FOR_PKT_DRAIN enabled.  Transfer this setting to new qi.
		 */
		if (flowcontrol) {
			WL_MCHAN(("wl%d.%d: %s: turn ON flow control for "
			          "TXQ_STOP_FOR_PKT_DRAIN on new qi %p\n",
			          wlc->pub->unit, cfg->_idx, __FUNCTION__, new_chan_ctxt->qi));
			wlc_txflowcontrol_override(wlc, new_chan_ctxt->qi,
			                           ON, TXQ_STOP_FOR_PKT_DRAIN);
			/* Check our oldqi, if it is different and exists,
			 * then we need to disable flow control on it
			 * since we just moved queue.
			 */
			if (wlc_mchan_qi_exist(wlc, oldqi)) {
				WL_MCHAN(("wl%d.%d: %s: turn OFF flow control for "
				          "TXQ_STOP_FOR_PKT_DRAIN on old qi %p\n",
				          wlc->pub->unit, cfg->_idx, __FUNCTION__, oldqi));
				wlc_txflowcontrol_override(wlc, oldqi, OFF,
				                           TXQ_STOP_FOR_PKT_DRAIN);
			}
		}
		else {
			/* This is to take care of cases where old context was deleted
			 * while flow control was on without TXQ_STOP_FOR_PKT_DRAIN being set.
			 * This means our cfg's interface is flow controlled at the wl layer.
			 * Need to disable flow control now that we have a new queue.
			 */
			if (oldqi_stopped_flag) {
				WL_MCHAN(("wl%d.%d: %s: turn OFF flow control for "
				          "TXQ_STOP_FOR_PKT_DRAIN on new qi %p\n",
				          wlc->pub->unit, cfg->_idx, __FUNCTION__, oldqi));
				wlc_txflowcontrol_override(wlc, new_chan_ctxt->qi, OFF,
				                           TXQ_STOP_FOR_PKT_DRAIN);
			}
		}
	}

	if (CHSPEC_BW(chanspec) > CHSPEC_BW(new_chan_ctxt->chanspec)) {
		WL_MCHAN(("wl%d.%d: Upgrade chan_context chanspec from 0x%x to 0x%x\n",
		          wlc->pub->unit, cfg->_idx, new_chan_ctxt->chanspec, chanspec));
#ifdef ENABLE_FCBS
		wlc_phy_fcbs_uninit(wlc->band->pi, new_chan_ctxt->chanspec);
		wlc_phy_fcbs_arm(wlc->band->pi, chanspec, 0);
#endif
		wlc_mchan_context_set_chan(mchan, new_chan_ctxt, chanspec,
		!BSSCFG_IS_MODESW_BWSW(cfg));
	}

	WL_MCHAN(("%s: cfg->chan_context set to %p\n", __FUNCTION__, cfg->chan_context));
	WL_MCHAN(("%s: cfg chan_context chanspec set to 0x%x\n", __FUNCTION__,
		cfg->chan_context->chanspec));

	/* update mchan active status */
	wlc_mchan_multichannel_upd(wlc, mchan);

	/* select trigger idx */
	if (BSSCFG_AP(cfg)) {
		mchan->trigger_cfg_idx = cfg->_idx;
	}
	else if ((mchan->trigger_cfg_idx == MCHAN_CFG_IDX_INVALID) ||
	 (cfg->current_bss->beacon_period <
		  wlc->bsscfg[mchan->trigger_cfg_idx]->current_bss->beacon_period)) {
		mchan->trigger_cfg_idx = cfg->_idx;
	}
	/* set the other idx */
	mchan->other_cfg_idx = wlc_mchan_set_other_cfg_idx(mchan);

	/* since in pm mode primary interface does not give pretbtt
	 * interrupt every beacon interval, trigger cfg should always
	 * be for p2p interface
	 */

	if ((mchan->other_cfg_idx != MCHAN_CFG_IDX_INVALID) &&
		(mchan->trigger_cfg_idx != MCHAN_CFG_IDX_INVALID)) {
		if (BSS_P2P_ENAB(wlc, WLC_BSSCFG(wlc, mchan->other_cfg_idx))) {
				SWAP(mchan->other_cfg_idx, mchan->trigger_cfg_idx);
		}
	}

	return (BCME_OK);
}

/** Everytime bsscfg becomes disassociated, call this function */
int
wlc_mchan_delete_bss_chan_context(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	return (_wlc_mchan_delete_bss_chan_context(wlc, cfg, NULL));
}

static int
_wlc_mchan_delete_bss_chan_context(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint *oldqi_info)
{
	mchan_info_t *mchan = wlc->mchan;
	wlc_mchan_context_t *chan_ctxt = cfg->chan_context;
	bool saved_state = cfg->associated;
	wlc_bsscfg_t *apcfg;
	wlc_bsscfg_t *other_bsscfg;

	WL_MCHAN(("%s called channel %d\n", __FUNCTION__, CHSPEC_CHANNEL(chan_ctxt->chanspec)));

	/* if context was not created due to AP up being deferred and we called down,
	 * can have a chan_ctxt that is NULL.
	 */
	if (chan_ctxt == NULL) {
		return (BCME_OK);
	}

	/* temporarily clear the cfg associated state.
	 * during roam, we are associated so when we do roam,
	 * we can potentially be deleting our old context first
	 * before creating our new context.  Just want to make sure
	 * that when we delete context, we're not associated.
	 */
	cfg->associated = FALSE;
	/* clear the cfg's context pointer */
	cfg->chan_context = NULL;
	/* if txq currently in use by cfg->wlcif->qi is deleted in
	 * wlc_mchan_context_free(), cfg->wlcif->qi will be set to the primary queue
	 */

	/* Take care of flow control for this cfg.
	 * If it was turned on for the queue, then this cfg has flow control on.
	 * Turn it off for this cfg only.
	 * Allocate a stale queue and temporarily attach cfg to this queue.
	 * Copy over the qi->stopped flag and turn off flow control.
	 */
	if (chan_ctxt->qi->stopped) {
		if (oldqi_info != NULL) {
			*oldqi_info = chan_ctxt->qi->stopped;
		}
		else
		{
			wlc_txq_info_t *qi = wlc_txq_alloc(wlc, wlc->osh);
			wlc_txq_info_t *orig_qi = cfg->wlcif->qi;

			WL_MCHAN(("wl%d.%d: %s: flow control on, turn it off!\n", wlc->pub->unit,
			          cfg->_idx, __FUNCTION__));
			/* Use this new q to drain the wl layer packets to make sure flow control
			 * is turned off for this interface.
			 */
			qi->stopped = chan_ctxt->qi->stopped;
			cfg->wlcif->qi = qi;
			/* reset_qi() below will not allow tx flow control to turn back on */
			wlc_txflowcontrol_reset_qi(wlc, qi);
			/* if txq currently in use by cfg->wlcif->qi is deleted in
			 * wlc_mchan_context_free(), cfg->wlcif->qi will be set to the primary queue
			 */
			pktq_flush(wlc->osh, &qi->q, TRUE, NULL, 0);
			ASSERT(pktq_empty(&qi->q));
			wlc_txq_free(wlc, wlc->osh, qi);
			cfg->wlcif->qi = orig_qi;
		}
	}

	/* no one else using this context, safe to delete it */
	if (!wlc_mchan_context_in_use(mchan, chan_ctxt, cfg)) {
#ifdef ENABLE_FCBS
		if (!wlc_phy_fcbs_uninit(wlc->band->pi, chan_ctxt->chanspec))
			WL_ERROR(("%s: wlc_phy_fcbs_uninit() FAILed\n", __FUNCTION__));
#endif
		/* If we were in the middle of transitioning to PM state because of this
		 * BSS, then cancel it
		 */
		wlc_mchan_cancel_pm_mode(mchan, chan_ctxt);
		wlc_mchan_context_free(mchan, chan_ctxt);
	}

	/* restore cfg associated state */
	cfg->associated = saved_state;

	other_bsscfg = wlc_mchan_get_other_cfg_frm_q(wlc, cfg->wlcif->qi);

	/* update mchan active status */
	wlc_mchan_multichannel_upd(wlc, mchan);
	/* reset the last_witch_time, in case the device is again in vsdb mode, */
	/* use fresh last_witch_time */
	mchan->si_algo_last_switch_time = 0;

	/* if mchan noa sched present, trigger index points to ap bsscfg */
	apcfg = wlc->bsscfg[mchan->trigger_cfg_idx];
	if (apcfg && P2P_GO(wlc, apcfg) && !MCHAN_ACTIVE(wlc->pub)) {
		/* if mchan noa present, remove it if mchan not active */
		int err;
		/* cancel the mchan noa schedule */
		err = wlc_mchan_noa_setup(wlc, apcfg, 0, 0, 0, 0, 0, FALSE);
		BCM_REFERENCE(err);
		WL_MCHAN(("%s: cancel noa schedule, err = %d\n", __FUNCTION__, err));
		/* Unmute - AP */
		wlc_ap_mute(wlc, FALSE, apcfg, -1);
	}

	/* if packets present for the other bsscfg (which is not getting deleted) in PSQ,  */
	/* flush them out now,	since we will not be able to get a chance to toucg PSQ */
	/* once we are out	of MCHAN mode */
	if (other_bsscfg)
		wlc_bsscfg_tx_start(other_bsscfg);

	/* clear trigger idx */
	if (mchan->trigger_cfg_idx == cfg->_idx) {
		wlc_bsscfg_t *bsscfg;
		int bssidx;
		int8 selected_idx = -1;
		uint16 shortest_bi = 0xFFFF;

		FOREACH_BSS(wlc, bssidx, bsscfg) {
			if ((bsscfg->chan_context == NULL) || (bsscfg == cfg)) {
				continue;
			}

			if (BSSCFG_AP(bsscfg)) {
				selected_idx = bsscfg->_idx;
				break;
			}

			if (bsscfg->current_bss->beacon_period < shortest_bi) {
				selected_idx = bsscfg->_idx;
				shortest_bi = bsscfg->current_bss->beacon_period;
			}
		}
		if (selected_idx != -1) {
			mchan->trigger_cfg_idx = selected_idx;
		}
		else {
			mchan->trigger_cfg_idx = MCHAN_CFG_IDX_INVALID;
		}
	}
	/* set the other idx */
	mchan->other_cfg_idx = wlc_mchan_set_other_cfg_idx(mchan);

#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
		/* Change the state of interface to OPEN so that pkts will be drained */
		wlc_wlfc_mchan_interface_state_update(wlc, cfg, WLFC_CTL_TYPE_INTERFACE_OPEN, TRUE);
		wlc_wlfc_mchan_interface_state_update(wlc, other_bsscfg,
			WLFC_CTL_TYPE_INTERFACE_OPEN, TRUE);
	}
#endif
	return BCME_OK;
}

static void
wlc_mchan_adopt_bss_chan_context(wlc_info_t *wlc, wlc_mchan_context_t *chan_ctxt,
                                 wlc_mchan_adopt_type adopt_type)
{
	chanspec_t home_chanspec;
	chanspec_t current_chanspec;
	wlc_txq_info_t *qi;
#if defined(WLC_LOW) && defined(WLC_HIGH) && defined(MCHAN_MINIDUMP)
	uint32 now;
	uint32 tsf_l;
#endif

#ifdef WL_PROT_OBSS
	wlc_bsscfg_t *bsscfg = NULL;
	wlc_bsscfg_t *bsscfg_prev = NULL;
	bool bw_update = FALSE;
	BCM_REFERENCE(bw_update);
	BCM_REFERENCE(bsscfg);
	BCM_REFERENCE(bsscfg_prev);
#endif

	WL_CHANLOG(wlc, __FUNCTION__, TS_ENTER, 0);
	ASSERT(chan_ctxt != NULL);
	/* only adopt context if allowed */
	if ((adopt_type != FORCE_ADOPT) && !MCHAN_ADOPT_CONTEXT_ALLOWED(wlc)) {
		WL_MCHAN(("wl%d: %s blocked scan/assoc/rm: %d/%d/%d\n",
		          wlc->pub->unit, __FUNCTION__, SCAN_IN_PROGRESS(wlc->scan),
		          AS_IN_PROGRESS(wlc), wlc_rminprog(wlc)));
		WL_CHANLOG(wlc, __FUNCTION__, TS_EXIT, 0);
		return;
	}

	current_chanspec = wlc->chanspec;
	home_chanspec = chan_ctxt->chanspec;
	qi = chan_ctxt->qi;


	wlc->home_chanspec = home_chanspec;

	/* If the driver is on an excursion, we just need to update
	 * wlc->primary_queue so the excursion will return to our
	 * new primary.
	 * Otherwise, we will both update wlc->primary_queue and switch
	 * to the new home channel.
	 */
	if (wlc->excursion_active) {
		WL_MCHAN(("%s, excursion active, not changing channel\n", __FUNCTION__));
		wlc_primary_queue_set(wlc, qi);
	} else {
		/* there is no excursion active, so switch to the new home channel
		* and update the primary queue.
		*/
#if defined(WLC_LOW) && defined(WLC_HIGH) && defined(MCHAN_MINIDUMP)
		tsf_l = R_REG(wlc->osh, &wlc->regs->tsf_timerlow);
#endif
		if (wlc->primary_queue == qi &&
			WLC_MCHAN_SAME_CTLCHAN(current_chanspec, home_chanspec)) {
			WL_MCHAN(("%s: already on chanspec 0x%x and primary_queue set to %p\n",
			          __FUNCTION__, current_chanspec, qi));
		} else {
			if (wlc->primary_queue == qi &&
			    !WLC_MCHAN_SAME_CTLCHAN(current_chanspec, home_chanspec)) {
				WL_MCHAN(("%s, context queue is primary, but current_chanspec %s "
				          "is not home_chanspec %s\n", __FUNCTION__,
				          wf_chspec_ntoa(current_chanspec, chanbuf),
				          wf_chspec_ntoa(home_chanspec, chanbuf2)));
			}
#ifdef WL_PROT_OBSS
			if (WLC_PROT_OBSS_ENAB(wlc->pub)) {
				bsscfg = wlc_mchan_get_cfg_frm_q(wlc, qi);
				bsscfg_prev	 = wlc_mchan_get_other_cfg_frm_q(wlc, qi);
				/* Check whether modesw BW update pending. */
				if (BSSCFG_STA(bsscfg))
					bw_update = wlc_modesw_update_bw_pending(wlc, bsscfg);
			}
			if (!bw_update)
#endif /* WL_PROT_OBSS */
				wlc_suspend_mac_and_wait(wlc);
			if (wlc->primary_queue != qi) {
#ifdef PROP_TXSTATUS
				wlc_bsscfg_t *cfg;
				wlc_txq_info_t *oldq = wlc->primary_queue;
				if (PROP_TXSTATUS_ENAB(wlc->pub)) {
					/* Close the non-active interface so that
					* pkts wont be send to dongle
					*/
					wlc_wlfc_mchan_interface_state_update(wlc,
					wlc_mchan_get_other_cfg_frm_q(wlc, qi),
					WLFC_CTL_TYPE_INTERFACE_CLOSE, FALSE);

					/* Open the active interface so that host
					* start sending pkts to dongle
					*/
					wlc_wlfc_mchan_interface_state_update(wlc,
					wlc_mchan_get_cfg_frm_q(wlc, qi),
					WLFC_CTL_TYPE_INTERFACE_OPEN, FALSE);
				}
#endif /* PROP_TXSTATUS */

				/* Peform queue switch and flush if necessary */
				wlc_primary_queue_set(wlc, qi);

#ifdef PROP_TXSTATUS
				if (PROP_TXSTATUS_ENAB(wlc->pub)) {
#ifdef WLAMPDU
					/* send_bar on interface being opened */
					if (AMPDU_ENAB(wlc->pub)) {
						struct scb_iter scbiter;
						struct scb *scb = NULL;
						cfg = wlc_mchan_get_cfg_frm_q(wlc, qi);
						if (cfg) {
							FOREACHSCB(wlc->scbstate, &scbiter, scb) {
								if (scb->bsscfg == cfg) {
									wlc_ampdu_send_bar_cfg(
										wlc->ampdu_tx, scb);
								}
							}
						}
					}
#endif /* WLAMPDU */
					/* flush interface being closed */
					cfg = wlc_mchan_get_cfg_frm_q(wlc, oldq);
					if (cfg) {
						wlc_wlfc_flush_pkts_to_host(wlc, cfg);
					}
					wlfc_sendup_ctl_info_now(wlc->wl);
				}
#endif /* PROP_TXSTATUS */
			}
			/* change channel if necessary */
#ifdef WL_PROT_OBSS
			if (WLC_PROT_OBSS_ENAB(wlc->pub)) {
				/* Update prev bsscfg stattistics */
				wlc_proto_obss_update_multiintf(wlc, bsscfg_prev);
			}
#endif /* WL_PROT_OBSS */
			if (current_chanspec != home_chanspec) {
				WL_MCHAN(("%s, changing channel to %s\n", __FUNCTION__,
				          wf_chspec_ntoa(home_chanspec, chanbuf)));
#ifdef WL_PROT_OBSS
				if (WLC_PROT_OBSS_ENAB(wlc->pub)) {
					if (BSSCFG_STA(bsscfg) && bw_update)
						bw_update = wlc_modesw_update_bandwidth(wlc, bsscfg,
						chan_ctxt->chanspec);
				}
				if (!bw_update)
#endif /* WL_PROT_OBSS */
				{
					/* No cals during VSDB channel switch */
					wlc_phy_hold_upd(wlc->band->pi, PHY_HOLD_FOR_VSDB, TRUE);
					wlc_set_chanspec(wlc, home_chanspec);
					wlc_phy_hold_upd(wlc->band->pi, PHY_HOLD_FOR_VSDB, FALSE);
				}

				if (DYN_ENAB(wlc->mchan)) {
					wlc_bsscfg_t *cfg = wlc_mchan_get_cfg_frm_q(wlc, qi);
					if (cfg) {
						bool is_p2p_cfg = BSS_P2P_ENAB(wlc, cfg);
						uint8 idx = 1 - is_p2p_cfg;
						wlc_mchan_if_byte(wlc, idx);
					}
				}
				if (AP_ACTIVE(wlc)) {
					wlc_bsscfg_t *cfg;
					cfg = wlc_mchan_get_cfg_frm_q(wlc, qi);
					/* Mute AP when not in GO channel
					* Unmute AP when in correct GO channel
					*/
					if (cfg && BSSCFG_AP(cfg)) {
						/* Unmute - AP when in GO channel */
						wlc_ap_mute(wlc, FALSE, cfg, -1);
					}
					else if ((cfg = wlc_mchan_get_other_cfg_frm_q(wlc, qi))) {
						/* Mute - AP when not in GO channel */
						wlc_ap_mute(wlc, TRUE, cfg, -1);
					}
				}

				/* Update PS modes for clients on new channel if their
				* user settings so desire
				*/
				wlc_mchan_pm_mode_leave(wlc->mchan, chan_ctxt);
			} else {
			WL_MCHAN(("%s, already on target channel %s\n", __FUNCTION__,
				wf_chspec_ntoa(home_chanspec, chanbuf)));
		}
		/*
		Setup BTCX protection mode according to the BSS that is
		being switched in.
		*/
		wlc_mchan_set_btcx_prot_mode(wlc, chan_ctxt);
#if defined(WL_PROT_OBSS)
		if (!bw_update)
#endif /* WL_PROT_OBSS */
			wlc_enable_mac(wlc);
#if defined(WL_PROT_OBSS)
		if (WLC_PROT_OBSS_ENAB(wlc->pub)) {
			/* Init new BSS OBSS statistics */
			wlc_proto_obss_stats_init_multiintf(wlc, bsscfg);
			if (bw_update) {
#if defined(WL_MODESW)
				if (WLC_MODESW_ENAB(wlc->pub)) {
					wlc_mchan_notif_cb_notif(wlc->mchan, bsscfg,
						BCME_OK, MCHAN_BW_UPDATE_COMPLETE);
				}
#endif /* WL_MODESW */
			}
		}
#endif /* WL_PROT_OBSS */
			/* Restart send after switching the queue */
		if (!pktq_empty(&wlc->active_queue->q))
			wlc_send_q(wlc, wlc->active_queue);
		}
#if defined(WLC_LOW) && defined(WLC_HIGH) && defined(MCHAN_MINIDUMP)
		if (wf_chspec_ctlchan(current_chanspec) != wf_chspec_ctlchan(home_chanspec)) {
			now = R_REG(wlc->osh, &wlc->regs->tsf_timerlow);
			chanswitch_history(wlc, current_chanspec, home_chanspec,
				wlc->mchan->prep_pm_start_time, tsf_l, CHANSWITCH_PREPPM_CONTEXT);
			chanswitch_history(wlc, current_chanspec, home_chanspec, tsf_l,
			now, CHANSWITCH_ADOPT_CONTEXT);
		}
#endif
	}
	WL_CHANLOG(wlc, __FUNCTION__, TS_EXIT, 0);
}

static int
BCMATTACHFN(wlc_mchan_sched_init)(mchan_info_t *mchan)
{
	mchan_sched_elem_t *sched_free_list;
	int i;

	sched_free_list = (mchan_sched_elem_t *)MALLOC(mchan->wlc->osh,
	                                               (sizeof(mchan_sched_elem_t) *
	                                                MCHAN_MAX_FREE_SCHED_ELEM));
	if (sched_free_list == NULL) {
		WL_ERROR(("%s: failed to allocate memory for mchan free schedule list\n",
		          __FUNCTION__));
		return (BCME_NOMEM);
	}

	/* save list to local global, used for freeing memory later */
	mchan->mchan_sched_free_list = sched_free_list;

	/* save list to mchan_sched_free pointer */
	mchan->mchan_sched_free = sched_free_list;

	/* initialize next pointers for each sched elem in list */
	for (i = 1; i < MCHAN_MAX_FREE_SCHED_ELEM; i++, sched_free_list++) {
		sched_free_list->next = sched_free_list + 1;
	}
	sched_free_list->next = NULL;

	return (BCME_OK);
}

static void
BCMATTACHFN(wlc_mchan_sched_deinit)(mchan_info_t *mchan)
{
	/* free memory allocated for mchan sched free list */
	if (mchan->mchan_sched_free_list != NULL) {
		MFREE(mchan->wlc->osh, mchan->mchan_sched_free_list,
		      (sizeof(mchan_sched_elem_t) * MCHAN_MAX_FREE_SCHED_ELEM));
		mchan->mchan_sched_free_list = NULL;
	}
}

static mchan_sched_elem_t *
wlc_mchan_sched_alloc(mchan_info_t *mchan)
{
	mchan_sched_elem_t *sched = mchan->mchan_sched_free;
	mchan_sched_elem_t **mchan_sched_free = &mchan->mchan_sched_free;

	wlc_mchan_list_del((mchan_list_elem_t **)mchan_sched_free,
	                   (mchan_list_elem_t *)sched);

	return (sched);
}

static void
wlc_mchan_sched_free(mchan_info_t *mchan, mchan_sched_elem_t *sched)
{
	mchan_sched_elem_t **mchan_sched_free = &mchan->mchan_sched_free;

	wlc_mchan_list_add_head((mchan_list_elem_t **)mchan_sched_free,
	                        (mchan_list_elem_t *)sched);
}

static mchan_sched_elem_t *
wlc_mchan_sched_get(mchan_info_t *mchan)
{
	return (mchan->mchan_sched);
}

int
wlc_mchan_sched_add(mchan_info_t *mchan, uint8 bss_idx, uint32 duration, uint32 end_tsf_l)
{
	mchan_sched_elem_t *sched;
	mchan_sched_elem_t **mchan_sched = &mchan->mchan_sched;

	sched = wlc_mchan_sched_alloc(mchan);
	if (sched == NULL) {
		WL_ERROR(("%s: failed to allocate memory for mchan schedule element\n",
		          __FUNCTION__));
		return (BCME_NOMEM);
	}
	sched->bss_idx = bss_idx;
	sched->duration = duration;
	sched->end_tsf_l = end_tsf_l;

	wlc_mchan_list_add_tail((mchan_list_elem_t **)mchan_sched,
	                        (mchan_list_elem_t *)sched);

	return BCME_OK;
}

static void
wlc_mchan_sched_delete(mchan_info_t *mchan, mchan_sched_elem_t* sched)
{
	mchan_sched_elem_t **mchan_sched = &mchan->mchan_sched;

	wlc_mchan_list_del((mchan_list_elem_t **)mchan_sched,
	                   (mchan_list_elem_t *)sched);
	wlc_mchan_sched_free(mchan, sched);
}

void wlc_mchan_sched_delete_all(mchan_info_t *mchan)
{
	/* delete all mchan schedule elements */
	while (mchan->mchan_sched) {
		wlc_mchan_sched_delete(mchan, mchan->mchan_sched);
	}
}

/**
 * Given a bcn_per and tbtt times from two entities, calculate a fair schedule and provide the
 * schedule start time and duration for the first entity.  The schedule start time and duration
 * for the second entity can be inferred from start_time1, duration1, and bcn_per passed in.
 *
 * start_time2 = start_time1 + duration1.
 * duration2 = bcn_per - duration1.
 *
 * NOTE: tbtt1 < tbtt2 in time.
 */
static void
wlc_mchan_calc_fair_chan_switch_sched(uint32 next_tbtt1,
                                      uint32 curr_tbtt2,
                                      uint32 next_tbtt2,
                                      uint32 bcn_per,
                                      bool tx,
                                      uint32 *start_time1,
                                      uint32 *duration1)
{
	uint32 start1, dur1, end1, fair_dur, min_start1, min_end1, overhead;

	/* calculate a fair schedule */
	start1 = next_tbtt1;
	end1 = next_tbtt2;
	dur1 = end1 - start1;
	/* fair time is half of the bcn_per */
	fair_dur = bcn_per >> 1;
	overhead = tx ? MCHAN_MIN_TBTT_TX_TIME_US : MCHAN_MIN_TBTT_RX_TIME_US;
	min_start1 = curr_tbtt2 + overhead;
	min_end1 = next_tbtt1 + overhead;
	/* This is when entity 1 gets too little time, correct it by moving start1
	 * closer to min_start1.
	 */
	if (dur1 < (fair_dur - MCHAN_FAIR_SCHED_DUR_ADJ_US)) {
		dur1 = fair_dur - dur1;
		start1 -= dur1;
		if (start1 < min_start1) {
			start1 = min_start1;
		}
	}
	/* This is when entity 2 gets too little time, correct it by moving
	 * end1 closer to min_end1.
	 */
	else if (dur1 > (fair_dur + MCHAN_FAIR_SCHED_DUR_ADJ_US)) {
		dur1 = dur1 - fair_dur;
		end1 -= dur1;
		if (end1 < min_end1) {
			end1 = min_end1;
		}
	}

	/* re-calculate duration */
	dur1 = end1 - start1;

	/* make duration to be integer times of (1<<P2P_UCODE_TIME_SHIFT)us */
	dur1 = MCHAN_TIMING_ALIGN(dur1);

	/* set the return values */
	*start_time1 = start1;
	*duration1 = dur1;
}

#ifdef WLC_HIGH_ONLY
/**
 * Given a bcn_per and tbtt times from two entities, calculate a schedule based on
 * schedule mode and provide the schedule start time and duration for the
 * first entity.  The schedule start time and duration
 * for the second entity can be inferred from start_time1, duration1, and bcn_per passed in.
 *
 * start_time2 = start_time1 + duration1.
 * duration2 = bcn_per - duration1.
 *
 * NOTE: tbtt1 < tbtt2 in time.
 */
static void
wlc_mchan_calc_chan_switch_sched(mchan_info_t *mchan,
                                 uint32 next_tbtt1,
                                 uint32 curr_tbtt2,
                                 uint32 next_tbtt2,
                                 uint32 bcn_per,
                                 bool tx,
                                 uint32 *start_time1,
                                 uint32 *duration1)
{
	uint32 start1, dur1, end1, fair_dur, min_start1, min_end1;
	uint32 overhead, max_start1;

	/* calculate a fair schedule */
	start1 = next_tbtt1;
	end1 = next_tbtt2;
	dur1 = end1 - start1;

	overhead = tx ? MCHAN_MIN_TBTT_TX_TIME_US : MCHAN_MIN_TBTT_RX_TIME_US;
	overhead += mchan->pretbtt_time_us;
	min_start1 = curr_tbtt2 + overhead;
	min_end1 = next_tbtt1 + overhead;
	max_start1 = next_tbtt1 - mchan->chan_switch_time_us;

	switch (mchan->sched_mode) {
	case WLC_MCHAN_SCHED_MODE_STA:
		start1 = curr_tbtt2 + mchan->min_dur;
		if (start1 < min_start1)
			start1 = min_start1;
		end1 = end1 - mchan->proc_time_us;
		break;

	case WLC_MCHAN_SCHED_MODE_P2P:
		end1 = next_tbtt1 + mchan->min_dur;
		if (end1 < min_end1)
			end1 = min_end1;
		if (start1 > max_start1)
			start1 = max_start1;
		break;

	case WLC_MCHAN_SCHED_MODE_FAIR:
	default:
		/* fair time is half of the bcn_per */
		fair_dur = bcn_per >> 1;

		/* This is when entity 1 gets too little time, correct it by moving start1
		 * closer to min_start1.
		 */
		if (dur1 < (fair_dur - MCHAN_FAIR_SCHED_DUR_ADJ_US)) {
			dur1 = fair_dur - dur1;
			start1 -= dur1;
		}
		/* This is when entity 2 gets too little time, correct it by moving
		 * end1 closer to min_end1.
		 */
		else if (dur1 > (fair_dur + MCHAN_FAIR_SCHED_DUR_ADJ_US)) {
			dur1 = dur1 - fair_dur;
			end1 -= dur1;
		}

		/* pull back start1 and end1 */
		start1 = start1 - mchan->proc_time_us;
		if (start1 < min_start1)
			start1 = min_start1;
		end1 = end1 - mchan->proc_time_us;
		if (end1 < min_end1)
			end1 = min_end1;
		break;
	}
	/* re-calculate noa_dur */
	dur1 = end1 - start1;

	/* make duration to be integer times of (1<<P2P_UCODE_TIME_SHIFT)us */
	dur1 = MCHAN_TIMING_ALIGN(dur1);

	/* set the return values */
	*start_time1 = start1;
	*duration1 = dur1;
}
#endif /* WLC_HIGH_ONLY */

static int
wlc_mchan_noa_setup(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint32 bcn_per,
                    uint32 curr_ap_tbtt, uint32 next_ap_tbtt, uint32 next_sta_tbtt,
                    uint32 tbtt_gap, bool create)
{
	wl_p2p_sched_t *noa_sched;
	wl_p2p_sched_desc_t *desc_ptr;
	int noa_sched_len;
	uint32 noa_start, noa_dur, noa_cnt;
	int err;
	mchan_info_t *mchan = wlc->mchan;

	/* default value for single abs period per beacon interval */
	/* Save actual abs count value */
	mchan->abs_cnt = 1;

	/* allocate memory for schedule */
	noa_sched_len = sizeof(wl_p2p_sched_t);
	if ((noa_sched = (wl_p2p_sched_t *)MALLOC(wlc->osh, noa_sched_len)) == NULL) {
		return (BCME_NOMEM);
	}
	ASSERT(noa_sched != NULL);
	/* set schedule type to abs */
	noa_sched->type = WL_P2P_SCHED_TYPE_ABS;

	/* cancel schedule first */
	noa_sched->action = WL_P2P_SCHED_ACTION_RESET;
	err = wlc_p2p_mchan_noa_set(wlc->p2p, bsscfg, noa_sched, WL_P2P_SCHED_FIXED_LEN);

	WL_MCHAN(("%s: cancel noa schedule, err = %d\n", __FUNCTION__, err));

	if (create) {
		uint32 proc_time = mchan->proc_time_us;

		if (mchan->delay_for_pm) {
			proc_time += MCHAN_TIMING_ALIGN(MCHAN_WAIT_FOR_PM_TO);
		}

#ifdef WLC_HIGH_ONLY
		/* calculate a schedule with different schedule mode */
		wlc_mchan_calc_chan_switch_sched(mchan, next_sta_tbtt, curr_ap_tbtt, next_ap_tbtt,
		                                 bcn_per, TRUE, &noa_start, &noa_dur);
#else
		/* calculate a fair schedule */
		wlc_mchan_calc_fair_chan_switch_sched(next_sta_tbtt, curr_ap_tbtt, next_ap_tbtt,
		                                      bcn_per, TRUE, &noa_start, &noa_dur);
#endif /* WLC_HIGH_ONLY */

		noa_cnt = WL_P2P_SCHED_REPEAT;

		/* save the actual noa_dur */
		mchan->noa_dur = noa_dur;
		/* include channel switch time in noa_dur for p2p setup */
		noa_dur += proc_time;
		/* pull back noa_start by proc_time */
		noa_start -= proc_time;
		mchan->noa_start_ref = noa_start;

		/* noa_dur must be integer times of (1<<P2P_UCODE_TIME_SHIFT)us */
		noa_dur = MCHAN_TIMING_ALIGN(noa_dur);

		WL_MCHAN(("%s: noa_start = 0x%x, noa_dur = 0x%x, noa_end = 0x%x\n",
		          __FUNCTION__, noa_start, noa_dur, (noa_start + noa_dur)));
		if (bsscfg->pm->PMenabled)
			noa_sched->action = WL_P2P_SCHED_ACTION_DOZE;
		else
			noa_sched->action = WL_P2P_SCHED_ACTION_NONE;

		noa_sched->option = WL_P2P_SCHED_OPTION_NORMAL;
		desc_ptr = noa_sched->desc;
		desc_ptr->start = ltoh32_ua(&noa_start);
		desc_ptr->interval = ltoh32_ua(&bcn_per);
		desc_ptr->duration = ltoh32_ua(&noa_dur);
		desc_ptr->count = ltoh32_ua(&noa_cnt);

		err = wlc_p2p_mchan_noa_set(wlc->p2p, bsscfg, noa_sched, noa_sched_len);
		if (err != BCME_OK) {
			WL_ERROR(("%s: wlc_p2p_noa_set returned err = %d\n", __FUNCTION__, err));
		}
		/* save the tbtt_gap value passed in */
		mchan->tbtt_gap_gosta = tbtt_gap;
	}
	else {
		/* clear the saved tbtt_gap value */
		mchan->tbtt_gap_gosta = 0;
		mchan->noa_dur = 0;
	}

	MFREE(wlc->osh, noa_sched, noa_sched_len);

	return (err);
}

static int
wlc_mchan_noa_setup_multiswitch(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint32 interval,
                    uint32 tbtt_gap, uint32 sel_dur, uint32 startswitch_for_abs, uint8 cnt)
{
	wl_p2p_sched_t *noa_sched;
	wl_p2p_sched_desc_t *desc_ptr;
	int noa_sched_len;
	uint32 noa_start, noa_dur, noa_cnt;
	int err;
	mchan_info_t *mchan = wlc->mchan;
	uint8 abs_cnt = cnt/2;

	/* Save actual abs count value */
	mchan->abs_cnt = abs_cnt;

	/* allocate memory for schedule */
	noa_sched_len = sizeof(wl_p2p_sched_t);
	if ((noa_sched = (wl_p2p_sched_t *)MALLOC(wlc->osh, noa_sched_len)) == NULL) {
		return (BCME_NOMEM);
	}
	ASSERT(noa_sched != NULL);
	ASSERT(sel_dur != 0);
	ASSERT(startswitch_for_abs != 0);

	/* set schedule type to abs */
	noa_sched->type = WL_P2P_SCHED_TYPE_ABS;

	/* cancel schedule first */
	noa_sched->action = WL_P2P_SCHED_ACTION_RESET;
	err = wlc_p2p_mchan_noa_set(wlc->p2p, bsscfg, noa_sched, WL_P2P_SCHED_FIXED_LEN);

	WL_MCHAN(("%s: cancel noa schedule, err = %d\n", __FUNCTION__, err));

	/* for unequal bandwidth cases, modify the noa_dur */
	noa_dur = sel_dur;
	/* For handling multiple abscence periods in one beacon interval */
	interval /= abs_cnt;

	noa_start = startswitch_for_abs;
	noa_cnt = WL_P2P_SCHED_REPEAT;

	/* save the actual noa_dur */
	mchan->noa_dur = noa_dur;
	/* include channel switch time in noa_dur for p2p setup */
	noa_dur += mchan->proc_time_us;
	/* pull back noa_start by proc_time */
	noa_start -= mchan->proc_time_us;

	mchan->noa_start_ref = noa_start;

	/* noa_dur must be integer times of (1<<P2P_UCODE_TIME_SHIFT)us */
	noa_dur = MCHAN_TIMING_ALIGN(noa_dur);

	WL_MCHAN(("noa_start = %d, noa_dur = %d, noa_end = %d\n",
		(((1<<21)-1)&noa_start), noa_dur, (((1<<21)-1)&(noa_start + noa_dur))));

	noa_sched->action = WL_P2P_SCHED_ACTION_NONE;
	noa_sched->option = WL_P2P_SCHED_OPTION_NORMAL;

	desc_ptr = noa_sched->desc;
	desc_ptr->start = ltoh32_ua(&noa_start);
	desc_ptr->interval = ltoh32_ua(&interval);
	desc_ptr->duration = ltoh32_ua(&noa_dur);
	desc_ptr->count = ltoh32_ua(&noa_cnt);

	err = wlc_p2p_mchan_noa_set(wlc->p2p, bsscfg, noa_sched, noa_sched_len);
	if (err != BCME_OK) {
		WL_ERROR(("%s: wlc_p2p_noa_set returned err = %d\n", __FUNCTION__, err));
	}

	/* save the tbtt_gap value passed in */
	mchan->tbtt_gap_gosta = tbtt_gap;
	MFREE(wlc->osh, noa_sched, noa_sched_len);
	return (err);
}

/** Returns 0 if other channel picked, 1 if selected channel picked. */
static bool
wlc_mchan_dtim_chan_select(wlc_info_t *wlc, bool sel_is_dtim, bool oth_is_dtim,
                           wlc_bsscfg_t *bsscfg_sel, wlc_bsscfg_t *bsscfg_oth)
{
	bool use_sel_ctxt;
	mchan_info_t *mchan = wlc->mchan;

	/* current tbtts too close to each other */
	/* Need to use special dtim based rules to schedule channel switch */

	/* if dtim count for bsscfg is 1, then don't use special dtim rules */
	if (bsscfg_sel->current_bss->dtim_period == 1) {
		sel_is_dtim = FALSE;
	}
	if (bsscfg_oth->current_bss->dtim_period == 1) {
		oth_is_dtim = FALSE;
	}

	if (mchan->bypass_dtim == TRUE) {
		sel_is_dtim = FALSE;
		oth_is_dtim = FALSE;
	}

	WL_MCHAN(("%s: sel_is_dtim = %d, oth_is_dtim = %d\n",
	          __FUNCTION__, sel_is_dtim, oth_is_dtim));

	/* DTIM rules */
	if (sel_is_dtim && !oth_is_dtim) {
		use_sel_ctxt = TRUE;
	}
	else if (!sel_is_dtim && oth_is_dtim) {
		use_sel_ctxt = FALSE;
	}
	else { /* NON-DTIM rules (when both are DTIMs, treated as neither being DTIMs */
		/* go to the channel whose beacon was last missed */
		if (mchan->last_missed_cfg_idx == bsscfg_sel->_idx) {
			/* go to selected chan context */
			use_sel_ctxt = TRUE;
		}
		else if (mchan->last_missed_cfg_idx == bsscfg_oth->_idx) {
			/* go to other chan context */
			use_sel_ctxt = FALSE;
		}
		else { /* randomly select channel */
			use_sel_ctxt = TRUE;
		}
	}

	WL_MCHAN(("%s: use_sel_ctxt = %d, last_missed_cfg_idx = %d\n",
	          __FUNCTION__, use_sel_ctxt, mchan->last_missed_cfg_idx));

	return (use_sel_ctxt);
}

static int
wlc_mchan_ap_pretbtt_proc(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint32 tsf_l, uint32 tsf_h)
{
	/* This is code that uses NOA to schedule channel switch */
	mchan_info_t *mchan = wlc->mchan;
	uint32 next_sta_tbtt, next_ap_tbtt, curr_sta_tbtt, curr_ap_tbtt;
	uint32 sta_bcn_per, ap_bcn_per;
	uint32 tbtt_gap;
	wlc_bsscfg_t *bsscfg_sta;
	int err = BCME_OK;
	uint32 tbtt_h, tbtt_l;

	/* Don't do anything if not up */
	if (!bsscfg->up) {
		return (BCME_NOTUP);
	}

	if (mchan->tsf_adj_wait_cnt) {
		WL_MCHAN(("%s: tbtt_gap, wait %d after tsf_adj\n",
		          __FUNCTION__, mchan->tsf_adj_wait_cnt));
		mchan->tsf_adj_wait_cnt--;
		return (BCME_NOTREADY);
	}

	ASSERT(wlc->mchan->other_cfg_idx != MCHAN_CFG_IDX_INVALID);

	/* find the sta_bsscfg */
	bsscfg_sta = WLC_BSSCFG(wlc, wlc->mchan->other_cfg_idx);

	ASSERT(bsscfg_sta != NULL);
	if (!bsscfg_sta) {
		return (BCME_ERROR);
	}

	/* if sta bsscfg not associated, return */
	if (!bsscfg_sta->associated) {
		return (BCME_NOTASSOCIATED);
	}

	if (!wlc_mcnx_tbtt_valid(wlc->mcnx, bsscfg_sta)) {
		if (mchan->wait_sta_valid < MAX_WAIT_STA_VALID) {
			wlc_mchan_adopt_bss_chan_context(wlc, bsscfg_sta->chan_context,
				FORCE_ADOPT);
			mchan->wait_sta_valid++;
		}
		else {
			/* STA's AP not found after repeated trials.
			* switch back to the GO channel
			*/
			wlc_mchan_adopt_bss_chan_context(wlc, bsscfg->chan_context,
				FORCE_ADOPT);
		}
		mchan->tbtt_gap_gosta = 0;
		return (BCME_NOTREADY);
	}

	mchan->wait_sta_valid = 0;

	/* First, switch to AP's chan context if allowed */
	/* if speciall_bss_idx is valid, means chan switch already in progress */
	if (mchan->special_bss_idx == MCHAN_CFG_IDX_INVALID) {
		wlc_mchan_prepare_pm_mode(wlc->mchan, bsscfg, tsf_l);
	}

	/* get sta tbtt info */
	sta_bcn_per = bsscfg_sta->current_bss->beacon_period << 10;
	wlc_mcnx_l2r_tsf64(wlc->mcnx, bsscfg_sta, tsf_h, tsf_l, &tbtt_h, &tbtt_l);
	wlc_tsf64_to_next_tbtt64(bsscfg_sta->current_bss->beacon_period, &tbtt_h, &tbtt_l);
	next_sta_tbtt = wlc_mcnx_r2l_tsf32(wlc->mcnx, bsscfg_sta, tbtt_l);
	/* get ap tbtt info */
	ap_bcn_per = bsscfg->current_bss->beacon_period << 10;
	wlc_mcnx_l2r_tsf64(wlc->mcnx, bsscfg, tsf_h, tsf_l, &tbtt_h, &tbtt_l);
	wlc_tsf64_to_next_tbtt64(bsscfg->current_bss->beacon_period, &tbtt_h, &tbtt_l);
	curr_ap_tbtt = wlc_mcnx_r2l_tsf32(wlc->mcnx, bsscfg, tbtt_l);

	/* calculate the next tbtt */
	curr_sta_tbtt = next_sta_tbtt - sta_bcn_per;
	next_ap_tbtt = curr_ap_tbtt + ap_bcn_per;
	BCM_REFERENCE(curr_sta_tbtt);

	/* we got tbbt times. Make them pre_tbtt */
	curr_ap_tbtt -= WLC_MCHAN_PRETBTT_TIME_US(mchan);
	curr_sta_tbtt -= WLC_MCHAN_PRETBTT_TIME_US(mchan);
	next_ap_tbtt -= WLC_MCHAN_PRETBTT_TIME_US(mchan);
	next_sta_tbtt -= WLC_MCHAN_PRETBTT_TIME_US(mchan);

	/* figure out gap between the 2 to make sure it is enough for operation */
	tbtt_gap = MIN(ABS((int32)(next_sta_tbtt - curr_ap_tbtt)),
	               ABS((int32)(next_ap_tbtt - next_sta_tbtt)));
	WL_MCHAN(("tbtt_gap = %d, n_ap_tbtt = 0x%x, c_ap_tbtt = 0x%x, "
	          "n_sta_tbtt = 0x%x, c_sta_tbtt = 0x%x\n",
	          tbtt_gap, next_ap_tbtt, curr_ap_tbtt, next_sta_tbtt, curr_sta_tbtt));
	if (tbtt_gap <= MCHAN_MIN_AP_TBTT_GAP_US && mchan->switch_algo != MCHAN_SI_ALGO) {
		int32 tsf_adj;
		uint32 tgt_h, tgt_l;

		/* reform ap interface */
		WL_MCHAN(("%s: tbtt_gap = %d, too close!\n", __FUNCTION__, tbtt_gap));
		tsf_adj = (int32)(next_sta_tbtt - curr_ap_tbtt);
		if (!((tsf_adj == (int32)tbtt_gap) || (tsf_adj == -((int32)tbtt_gap)))) {
			tsf_adj = (int32)(next_sta_tbtt - next_ap_tbtt);
		}
		/* Moving the tsf means moving the sta tbtt wrt our local tsf.
		 * AP's tbtt stays the same wrt our local tsf.
		 * Thus, if sta tbtt is > ap tbtt, move by (ap_bcn_per/2)-tbtt_gap.
		 * Otherwise, move by (ap_bcn_per/2)+tbtt_gap.
		 * tsf_adj > 0 == sta_tbtt > ap_tbtt
		 */
		tsf_adj = (tsf_adj > 0) ? (ap_bcn_per>>1)-tbtt_gap : (ap_bcn_per>>1)+tbtt_gap;
		/* update the GO's tsf */
		WL_MCHAN(("%s: (tbtt_gap) adjust tsf by %d\n", __FUNCTION__, tsf_adj));
		tgt_h = tsf_h;
		tgt_l = tsf_l;
		wlc_uint64_add(&tgt_h, &tgt_l, 0, tsf_adj);
		wlc_mcnx_l2r_tsf64(wlc->mcnx, bsscfg, tgt_h, tgt_l, &tbtt_h, &tbtt_l);
		wlc_tsf64_to_next_tbtt64(bsscfg->current_bss->beacon_period, &tbtt_h, &tbtt_l);
		wlc_mcnx_r2l_tsf64(wlc->mcnx, bsscfg, tbtt_h, tbtt_l, &tbtt_h, &tbtt_l);
		wlc_tsf_adj(wlc, bsscfg, tgt_h, tgt_l, tsf_h, tsf_l, tbtt_l, ap_bcn_per, FALSE);
		mchan->tsf_adj_wait_cnt = 2;
		return (BCME_BADARG);
	}

	/* if tbtt_gap meets requirement, we are guaranteed next_sta_tbtt < next_ap_tbtt */

	/* if GO, setup noa */
	if (BSS_P2P_ENAB(wlc, bsscfg)) {
		WL_MCHAN(("tbtt_drift = %d\n", ABS((int32)(mchan->tbtt_gap_gosta - tbtt_gap))));
		if (wlc_p2p_noa_valid(wlc->p2p, bsscfg) && mchan->tbtt_gap_gosta &&
		    (ABS((int32)(mchan->tbtt_gap_gosta - tbtt_gap)) <=
		     MCHAN_MAX_TBTT_GAP_DRIFT_MS) &&
		     (mchan->trigg_algo == FALSE)) {
			/* schedule already setup */
			return (BCME_OK);
		}

		mchan->trigg_algo = FALSE;
		if ((mchan->switch_algo == MCHAN_BANDWIDTH_ALGO) || DYN_ENAB(mchan)) {
			uint32 switchtime_1, sel_dur, switchtime_2;
			uint32 primary_ch_time;
			uint32 availtime_1, availtime_2;
			uint32 adj_remtime = 0;
			uint32 secondary_ch_percentage = 100 - mchan->percentage_bw;

			/* Putting min-max limits for the BW range (25%-75%) only */
			/* Beacon (failure to send beacon) losses if primary BW>75% */
			if (secondary_ch_percentage > 75)
				secondary_ch_percentage = 75;
			else if (secondary_ch_percentage < 25)
				secondary_ch_percentage = 25;

			primary_ch_time = sta_bcn_per * (100 - secondary_ch_percentage) / 100;

			WL_MCHAN(("tbtt_gap:%d, tsf:%d\n", tbtt_gap, (((1<<21)-1)&tsf_l)));
			WL_MCHAN(("cur_ap:%d cur_sta:%d next_ap:%d next_sta:%d\n",
				(((1<<21)-1)& curr_ap_tbtt), (((1<<21)-1)& curr_sta_tbtt),
				(((1<<21)-1)& next_ap_tbtt), (((1<<21)-1)& next_sta_tbtt)));
			{

			/*
			 *   Graphical representation of the diffrent tbtt's in time
			 *
			 *                  swtime_1         swtime_2
			 *          ^ ^       |      ^         |         ^
			 *   ^      | |       |      |         |         |
			 *   |      | |       |      |         |         |
			 * -------------------------------------------------------
			 *curr_sta_tbtt curr time(tsf) next_sta_tbtt         next_ap_tbtt
			 *         ^
			 *         |
			 *         curr_ap_tbtt
			 */

			availtime_1 = next_sta_tbtt - curr_ap_tbtt;
			availtime_2 = next_ap_tbtt - next_sta_tbtt;

			/*
			* we need to handle availtime_1 > availtime_2 in a different manner
			* compared to availtime_1 <= availtime_2. The default algorithm was failing
			* in the former case when say for ex. secondary_ch_time = 80ms and
			* primary_ch_time = 20ms. One can fit in the values and check assuming
			* availtime_1 = 65ms and availtime_2 = 35ms
			*/

			if (availtime_1 <= availtime_2) {
				switchtime_1 = next_sta_tbtt;
				switchtime_2 = switchtime_1 + primary_ch_time;
				if (switchtime_2 > next_ap_tbtt)
					adj_remtime = switchtime_2 - next_ap_tbtt;
				if (adj_remtime) {
					switchtime_1 -= adj_remtime;
					switchtime_2 -= adj_remtime;
				}
			}
			else {
				switchtime_2 = next_ap_tbtt;
				switchtime_1 = switchtime_2 - primary_ch_time;
				if (switchtime_1 > next_sta_tbtt)
					adj_remtime = switchtime_1 - next_sta_tbtt;

				if (adj_remtime) {
					switchtime_1 -= adj_remtime;
					switchtime_2 -= adj_remtime;
				}
			}

			sel_dur = switchtime_2 - switchtime_1;
			WL_MCHAN(("swtime_1:%d swtime_2:%d sel_dur:%d\n",
				(((1<<21)-1)& switchtime_1),
				(((1<<21)-1)&switchtime_2), sel_dur));
			err = wlc_mchan_noa_setup_multiswitch(wlc, bsscfg, ap_bcn_per, tbtt_gap,
			                                      sel_dur, switchtime_1, 2);
			}
		}
		else if (mchan->switch_algo == MCHAN_SI_ALGO) {
			/* This only Works for 25ms service interval */
			uint32 si_time = DEFAULT_SI_TIME;
			uint8 interval_slots;
			bool adjusted = FALSE;
			int32 tsf_adj;
			mchan->switch_interval = DEFAULT_SI_TIME;

			/* figure out gap between the 2 to make sure it is enough for operation */
			tbtt_gap = MIN(ABS((int32)(next_sta_tbtt - curr_ap_tbtt)),
				ABS((int32)(next_ap_tbtt - next_sta_tbtt)));

			/* Adjusting the GO beacon according to switch times.
			* Moving the tsf means moving the sta tbtt wrt our local tsf.
			* AP's tbtt stays the same wrt our local tsf.
			* Thus, if sta tbtt is > ap tbtt, move by (ap_bcn_per/2)-tbtt_gap.
			* Otherwise, move by (ap_bcn_per/2)+tbtt_gap.
			* tsf_adj > 0 == sta_tbtt > ap_tbtt
			*/

			tsf_adj = (int32)(next_sta_tbtt - curr_ap_tbtt);

			/* Checking is the GO beacon is within the bounds - 33 to 42
			* Adjusting to the centre of the presence period ~ 38ms
			*/

			if (tsf_adj < MCHAN_SI_ALGO_BCN_LOW_PSC_1) {
				tsf_adj = MCHAN_SI_ALGO_BCN_OPT_PSC_1 - tsf_adj;
				adjusted = TRUE;
			}
			else if (tsf_adj > MCHAN_SI_ALGO_BCN_LOW_PSC_2) {
				tsf_adj = MCHAN_SI_ALGO_BCN_OPT_PSC_1 - tsf_adj
					+ ap_bcn_per;
				adjusted = TRUE;
			}

			/* If the beacon is found to be outside the limits
			* then adjust the beacon accordingly
			*/
			if (adjusted) {
				uint32 tgt_h, tgt_l;
				/* update the GO's tsf */
				WL_MCHAN(("%s: (tbtt_gap) adjust tsf by %d\n", __FUNCTION__,
					tsf_adj));
				tgt_h = tsf_h;
				tgt_l = tsf_l;
				wlc_uint64_add(&tgt_h, &tgt_l, 0, tsf_adj);
				wlc_mcnx_l2r_tsf64(wlc->mcnx, bsscfg, tgt_h, tgt_l, &tbtt_h,
					&tbtt_l);
				wlc_tsf64_to_next_tbtt64(bsscfg->current_bss->beacon_period,
					&tbtt_h, &tbtt_l);
				wlc_mcnx_r2l_tsf64(wlc->mcnx, bsscfg, tbtt_h, tbtt_l, &tbtt_h,
					&tbtt_l);
				wlc_tsf_adj(wlc, bsscfg, tgt_h, tgt_l, tsf_h, tsf_l, tbtt_l,
					ap_bcn_per, FALSE);
				mchan->tsf_adj_wait_cnt = 2;
			}

			/* Number of Interval slots in one beacon period */
			interval_slots = ap_bcn_per/si_time;

			/* Here we are sure of correct beacon positioning */
			/* Create schedule and dispatch */
			WL_MCHAN(("tbtt_gap:%d, tsf:%d\n", tbtt_gap, (((1<<21)-1)&tsf_l)));
			WL_MCHAN(("cur_ap:%d cur_sta:%d next_ap:%d next_sta:%d\n",
				(((1<<21)-1)& curr_ap_tbtt), (((1<<21)-1)& curr_sta_tbtt),
				(((1<<21)-1)& next_ap_tbtt), (((1<<21)-1)& next_sta_tbtt)));
			err = wlc_mchan_noa_setup_multiswitch(wlc, bsscfg, ap_bcn_per,
				tbtt_gap, si_time, curr_ap_tbtt + si_time, interval_slots);

		}
		else {
			err = wlc_mchan_noa_setup(wlc, bsscfg, ap_bcn_per, curr_ap_tbtt,
				next_ap_tbtt, next_sta_tbtt, tbtt_gap, TRUE);
		}
	}
#ifdef WLMCHAN_SOFT_AP /* Not supported */
	/* Soft AP case. There is no NOA. Use 802.11g QUITE IE */
	else {
		uint32 other_start, other_dur, sel_dur, now, offset;
		wlc_mchan_calc_fair_chan_switch_sched(next_sta_tbtt, curr_ap_tbtt, next_ap_tbtt,
			ap_bcn_per, TRUE, &other_start, &other_dur);

		/* Update quiet IE */
		mchan->mchan_quiet_cmd.count = 1;
		mchan->mchan_quiet_cmd.period = 1;
		mchan->mchan_quiet_cmd.duration = (other_dur + mchan->pretbtt_time_us) >> 10;

		now = R_REG(wlc->osh, &wlc->regs->tsf_timerlow);
		offset = now + (ap_bcn_per >>10);
		mchan->mchan_quiet_cmd.offset = (other_start - offset) >> 10;

		/* Update the beacon/probe response */
		/* Need to see the use for this */
		/* wlc->spect_state |= NEED_TO_UPDATE_BCN; */
		wlc_update_beacon(wlc);
		wlc_update_probe_resp(wlc, TRUE);

		if (!(mchan->tbtt_gap_gosta &&
			(ABS((int32)(mchan->tbtt_gap_gosta - tbtt_gap)) <=
			MCHAN_MAX_TBTT_GAP_DRIFT_MS))) {
			mchan->tbtt_gap_gosta = tbtt_gap;
		}
		if (mchan->mchan_sched) {
			/* in case timer fires after pretbtt interrupt, clean up */
			wlc_mchan_stop_timer(mchan, MCHAN_TO_REGULAR);
#ifdef WLMCHANPRECLOSE
			wlc_mchan_stop_timer(mchan, MCHAN_TO_PRECLOSE);
#endif
			/* delete all mchan schedule elements */
			while (mchan->mchan_sched) {
				wlc_mchan_sched_delete(mchan, mchan->mchan_sched);
			}
		}
		sel_dur = R_REG(wlc->osh, &wlc->regs->tsf_timerlow) + MCHAN_TIMER_DELAY_US;
		sel_dur = (sel_dur >= other_start) ? 1 : (other_start - sel_dur);
		wlc_mchan_start_timer(mchan, sel_dur, MCHAN_TO_REGULAR);
		/* add mchan sched element for other bsscfg */
		wlc_mchan_sched_add(mchan, bsscfg_sta->_idx,
			other_dur, other_start);
		wlc_mchan_sched_add(mchan, bsscfg->_idx, 0,
			(other_start + other_dur));
		return (BCME_OK);
	}
#endif /* WLMCHAN_SOFT_AP */

	return (err);
}

#ifdef WLMCHAN_SOFT_AP
int wlc_mchan_write_ie_quiet_len(wlc_info_t *wlc)
{
	int len = 0;
	mchan_info_t *mchan = wlc->mchan;
	if (mchan->mchan_quiet_cmd.count)
		len = sizeof(dot11_quiet_t);
	return len;
}

int wlc_mchan_write_ie_quiet(wlc_info_t *wlc, uint8 *buf)
{
	int len = 0;
	mchan_info_t *mchan = wlc->mchan;
	if (mchan->mchan_quiet_cmd.count) {
		dot11_quiet_t *quiet = (dot11_quiet_t *)buf;
		quiet->id = DOT11_MNG_QUIET_ID;
		quiet->len = 6;
		quiet->count = mchan->mchan_quiet_cmd.count--;
		quiet->period = mchan->mchan_quiet_cmd.period;
		quiet->duration = htol16(mchan->mchan_quiet_cmd.duration);
		quiet->offset = htol16(mchan->mchan_quiet_cmd.offset);
		len = wlc_mchan_write_ie_quiet_len(wlc);
	}
	return len;
}
#endif /* WLMCHAN_SOFT_AP */

#ifdef PROP_TXSTATUS
int
wlc_wlfc_mchan_interface_state_update(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint8 open_close,
 bool force_open)
{
	int rc = BCME_OK;
	uint8 if_id;
	struct scb_iter scbiter;
	struct scb *scb;
	/* space for type(1), length(1) and value */
	uint8 results[1+1+WLFC_CTL_VALUE_LEN_MAC];

	results[1] = WLFC_CTL_VALUE_LEN_MAC;

	if (bsscfg == NULL)
		return rc;

	if (BSSCFG_AP(bsscfg)) {
		FOREACHSCB(wlc->scbstate, &scbiter, scb) {
			if (SCB_BSSCFG(scb) == bsscfg && SCB_ASSOCIATED(scb)) {
				results[2] = scb->mac_address_handle;
				if (open_close == WLFC_CTL_TYPE_INTERFACE_OPEN) {
					if (BSS_P2P_ENAB(wlc, bsscfg) && force_open == FALSE)
						return rc;
					if (!scb->PS) {
						results[0] = WLFC_CTL_TYPE_MAC_OPEN;
						if (PROP_TXSTATUS_ENAB(wlc->pub)) {
							rc = wlfc_push_signal_data(wlc->wl, results,
								sizeof(results), FALSE);
						}
						if (rc != BCME_OK)
							break;
					}
				}
				else {
						results[0] = WLFC_CTL_TYPE_MAC_CLOSE;
						if (PROP_TXSTATUS_ENAB(wlc->pub)) {
							rc = wlfc_push_signal_data(wlc->wl, results,
								sizeof(results), FALSE);
						}
						if (rc != BCME_OK)
							break;
				}
			}
		}
	}
	else {
		if_id = bsscfg->wlcif->index;
		results[0] = open_close;
		results[2] = if_id;
		if ((open_close == WLFC_CTL_TYPE_MAC_OPEN) ||
			(open_close == WLFC_CTL_TYPE_MAC_CLOSE))
		{
			FOREACHSCB(wlc->scbstate, &scbiter, scb) {
				if ((SCB_BSSCFG(scb) == bsscfg) && BSS_TDLS_ENAB(wlc, bsscfg)) {
					results[2] = scb->mac_address_handle;
					break;
				}
			}
		}
		if (PROP_TXSTATUS_ENAB(wlc->pub)) {
			rc = wlfc_push_signal_data(wlc->wl, results, sizeof(results), FALSE);
		}
	}
	if (open_close == WLFC_CTL_TYPE_INTERFACE_OPEN) {
		wlfc_sendup_ctl_info_now(wlc->wl);
	}
	return rc;
}
#endif /* PROP_TXSTATUS */

wlc_bsscfg_t *
wlc_mchan_get_other_cfg_frm_q(wlc_info_t *wlc, wlc_txq_info_t *qi)
{
	wlc_bsscfg_t *cfg;
	if (MCHAN_ACTIVE(wlc->pub)) {
		cfg = WLC_BSSCFG(wlc, wlc->mchan->trigger_cfg_idx);
		if (cfg->wlcif->qi == qi)
			return WLC_BSSCFG(wlc, wlc->mchan->other_cfg_idx);

		cfg = WLC_BSSCFG(wlc, wlc->mchan->other_cfg_idx);
		if (cfg && cfg->wlcif->qi == qi)
			return WLC_BSSCFG(wlc, wlc->mchan->trigger_cfg_idx);
	}
	return NULL;
}

wlc_bsscfg_t *
wlc_mchan_get_cfg_frm_q(wlc_info_t *wlc, wlc_txq_info_t *qi)
{
	wlc_bsscfg_t *cfg;
	if (MCHAN_ACTIVE(wlc->pub)) {
		cfg = WLC_BSSCFG(wlc, wlc->mchan->trigger_cfg_idx);
		if (cfg && cfg->wlcif->qi == qi)
			return cfg;

		cfg = WLC_BSSCFG(wlc, wlc->mchan->other_cfg_idx);
		if (cfg && cfg->wlcif->qi == qi)
			return cfg;
	}
	return NULL;
}

static int wlc_mchan_alternate_switching(wlc_info_t *wlc,
	wlc_bsscfg_t *bsscfg_sel,
	wlc_bsscfg_t *bsscfg_oth,
	uint32 curr_sel_tbtt,
	uint32 curr_oth_tbtt,
	uint32 sel_dtim_cnt,
	uint32 oth_dtim_cnt,
	uint32 tsf_l);
extern bool wlc_mchan_find_switch_idx(wlc_info_t *wlc,
	bool sel_is_dtim,
	bool oth_is_dtim,
	wlc_bsscfg_t *bsscfg_sel,
	wlc_bsscfg_t *bsscfg_oth);

static int
wlc_mchan_sta_pretbtt_proc(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg_sel, uint32 tsf_l, uint32 tsf_h)
{
	int err = BCME_OK;
	mchan_info_t *mchan = wlc->mchan;
	uint32 next_sel_tbtt, next_oth_tbtt, curr_sel_tbtt, curr_oth_tbtt;
	uint32 sel_bcn_per, oth_bcn_per;
	uint32 other_start, other_dur;
	uint32 tbtt_gap;
	wlc_bsscfg_t *bsscfg_oth;
	uint16 sel_dtim_cnt, oth_dtim_cnt;
	uint32 sel_dur;
	bool special_return;
	bool use_sel_ctxt;
	uint32 next_toggle_switch_time = 0;
	uint32 proc_time = (uint32)mchan->proc_time_us;
	uint32 start1, start2, tbtt_h, tbtt_l;
	uint8 cur_idx;

	/* Don't do anything if not up */
	if (!bsscfg_sel->up) {
		return (BCME_NOTUP);
	}

	/* if selected bsscfg not associated, return */
	if (!bsscfg_sel->associated) {
		return (BCME_NOTASSOCIATED);
	}

	ASSERT(mchan->other_cfg_idx != MCHAN_CFG_IDX_INVALID);

	/* find the selected and other bss */
	bsscfg_oth = WLC_BSSCFG(wlc, mchan->other_cfg_idx);

	ASSERT(bsscfg_oth != NULL);
	if (!bsscfg_oth) {
		return (BCME_ERROR);
	}

	/* if other bsscfg not associated, return */
	if (!BSS_TDLS_ENAB(wlc, bsscfg_oth) && !bsscfg_oth->associated) {
		WL_ERROR(("%s: bss idx %d not associated.\n", __FUNCTION__,
			WLC_BSSCFG_IDX(bsscfg_oth)));
		return (BCME_NOTASSOCIATED);
	}

	if (!wlc_mcnx_tbtt_valid(wlc->mcnx, bsscfg_oth) &&
		!BSS_TDLS_ENAB(wlc, bsscfg_oth)) {
		WL_NONE(("%s: bss idx %d WLC_P2P_INFO_VAL_TBTT not set\n", __FUNCTION__,
			WLC_BSSCFG_IDX(bsscfg_oth)));
		return (BCME_NOTREADY);
	}

#ifdef WLTDLS
	if (BSS_TDLS_ENAB(wlc, bsscfg_oth)) {
		/* get selected tbtt info */
		sel_bcn_per = bsscfg_sel->current_bss->beacon_period << 10;
		sel_dur = sel_bcn_per >> 1; /* off-channel 50% of the beacon period */
		WL_TDLS(("wl%d.%d:%s() return to base channel.\n",
		wlc->pub->unit, bsscfg_sel->_idx, __FUNCTION__));
		wlc_tdls_do_chsw(wlc->tdls, bsscfg_oth, FALSE);
		if (bsscfg_oth->tdls->chsw_req_enabled && wlc_tdls_is_chsw_enabled(wlc->tdls)) {
			wlc_mchan_sched_add(mchan, bsscfg_oth->_idx, 0, sel_dur);
			wlc_mchan_start_timer(mchan, sel_dur, MCHAN_TO_REGULAR);
			WL_MCHAN(("%s:wl%d.%d schedule for BSS%d, sel_dur = %d\n",
				__FUNCTION__, wlc->pub->unit, bsscfg_sel->_idx,
				bsscfg_oth->_idx,  sel_dur));
#ifdef PROP_TXSTATUS
			wlc_wlfc_mchan_interface_state_update(wlc,
				bsscfg_oth, WLFC_CTL_TYPE_MAC_CLOSE, FALSE);
			wlc_wlfc_flush_pkts_to_host(wlc, bsscfg_oth);
		} else {
			wlc_wlfc_mchan_interface_state_update(wlc,
				bsscfg_oth, WLFC_CTL_TYPE_MAC_OPEN, FALSE);
#endif
		}

#ifdef PROP_TXSTATUS
		wlc_wlfc_mchan_interface_state_update(wlc,
			bsscfg_sel, WLFC_CTL_TYPE_INTERFACE_OPEN, FALSE);
#endif
		return BCME_OK;
	}
#endif /* WLTDLS */

	/* Add extra processing time for delay_for_pm feature */
	if (mchan->delay_for_pm) {
		proc_time += MCHAN_TIMING_ALIGN(MCHAN_WAIT_FOR_PM_TO);
	}

	/* get selected tbtt info */
	sel_bcn_per = bsscfg_sel->current_bss->beacon_period << 10;
	wlc_mcnx_l2r_tsf64(wlc->mcnx, bsscfg_sel, tsf_h, tsf_l, &tbtt_h, &tbtt_l);
	wlc_tsf64_to_next_tbtt64(bsscfg_sel->current_bss->beacon_period, &tbtt_h, &tbtt_l);
	curr_sel_tbtt = wlc_mcnx_r2l_tsf32(wlc->mcnx, bsscfg_sel, tbtt_l);
	/* get other tbtt info */
	oth_bcn_per = bsscfg_oth->current_bss->beacon_period << 10;
	wlc_mcnx_l2r_tsf64(wlc->mcnx, bsscfg_oth, tsf_h, tsf_l, &tbtt_h, &tbtt_l);
	wlc_tsf64_to_next_tbtt64(bsscfg_oth->current_bss->beacon_period, &tbtt_h, &tbtt_l);
	next_oth_tbtt = wlc_mcnx_r2l_tsf32(wlc->mcnx, bsscfg_oth, tbtt_l);

	/* calculate the next tbtt */
	next_sel_tbtt = curr_sel_tbtt + sel_bcn_per;
	curr_oth_tbtt = next_oth_tbtt - oth_bcn_per;

	/* we got tbbt times. Make them pre_tbtt */
	curr_sel_tbtt -= WLC_MCHAN_PRETBTT_TIME_US(mchan);
	curr_oth_tbtt -= WLC_MCHAN_PRETBTT_TIME_US(mchan);
	next_sel_tbtt -= WLC_MCHAN_PRETBTT_TIME_US(mchan);
	next_oth_tbtt -= WLC_MCHAN_PRETBTT_TIME_US(mchan);

	/*
	 *	Graphical representation of the different tbtt's in time
	 *
	 *			^				^
	 *	^		|		^		|
	 *	|		|		|		|
	 * -------------------------------------------------------
	 *  curr_oth_tbtt   curr_sel_tbtt   next_oth_tbtt   next_sel_tbtt
	 *			^
	 *			|
	 *			curr time (tsf)
	 */

	/* Look for special case to correct curr_oth and next_oth tbtt.
	 * Always assume curr_sel_tbtt and next_sel_tbtt are correct.
	 */
	if (curr_sel_tbtt < curr_oth_tbtt) {
		/* By the time we got here, next_oth_tbtt already happened.
		 * This means, curr_oth_tbtt is really next_oth_tbtt and
		 * next_oth_tbtt is really next next_oth_tbtt.
		 * Correct for this by subtracting oth_bcn_per from them.
		 */
		curr_oth_tbtt -= oth_bcn_per;
		next_oth_tbtt -= oth_bcn_per;
	}

	/* update sel_bsscfg sw_dtim_cnt */
	bsscfg_sel->sw_dtim_cnt = (bsscfg_sel->sw_dtim_cnt) ?
	        (bsscfg_sel->sw_dtim_cnt - 1) : (bsscfg_sel->current_bss->dtim_period - 1);

	/* figure out gap between the 2 to determine what to do next */
	tbtt_gap = MIN(ABS((int32)(next_oth_tbtt - curr_sel_tbtt)),
	               ABS((int32)(curr_sel_tbtt - curr_oth_tbtt)));

	if (sel_bcn_per == oth_bcn_per) {
		if (tbtt_gap <= MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {
			mchan->bcn_position = MCHAN_BCN_OVERLAP;
			if ((mchan->switch_algo == MCHAN_DEFAULT_ALGO) && (mchan->bcnovlpopt) &&
				(mchan->overlap_algo_switch == FALSE)) {
				mchan->overlap_algo_switch = TRUE;
				mchan->switch_algo = MCHAN_SI_ALGO;
				mchan->si_algo_last_switch_time = 0;
				mchan->switch_interval = SI_DYN_SW_INTVAL;
			}
		}
		else {
			mchan->bcn_position = MCHAN_BCN_NONOVERLAP;
			if ((mchan->overlap_algo_switch) && (mchan->bcnovlpopt)) {
				mchan->overlap_algo_switch = FALSE;
				mchan->switch_algo = MCHAN_DEFAULT_ALGO;
			}
		}
	} else {
		mchan->bcn_position = MCHAN_BCN_INVALID;
	}

	if (sel_bcn_per > oth_bcn_per) {
		/* swap the two indices - In case of different BI use the one with
		 * smallest BI as the trigger idx
		 */
		mchan->trigger_cfg_idx ^= mchan->other_cfg_idx;
		mchan->other_cfg_idx ^= mchan->trigger_cfg_idx;
		mchan->trigger_cfg_idx ^= mchan->other_cfg_idx;

		/* clear last_missed idx */
		mchan->last_missed_cfg_idx = MCHAN_CFG_IDX_INVALID;
		mchan->last_missed_DTIM_idx = MCHAN_CFG_IDX_INVALID;
		return (BCME_NOTREADY);
	}

	if (sel_bcn_per == oth_bcn_per) {
		if ((ABS((int32)(curr_sel_tbtt - curr_oth_tbtt)) <=
			(int32)MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) &&
			(curr_sel_tbtt > curr_oth_tbtt)) {
			WL_MCHAN(("trigger idx change from %d to %d",
				mchan->trigger_cfg_idx, mchan->other_cfg_idx));
			/* swap the two indices */
			mchan->trigger_cfg_idx ^= mchan->other_cfg_idx;
			mchan->other_cfg_idx ^= mchan->trigger_cfg_idx;
			mchan->trigger_cfg_idx ^= mchan->other_cfg_idx;
			/* clear last_missed idx */
			mchan->last_missed_cfg_idx = MCHAN_CFG_IDX_INVALID;
			mchan->last_missed_DTIM_idx = MCHAN_CFG_IDX_INVALID;
			return (BCME_NOTREADY);
		}
	}

	/* Make sure we have no unserviced schedule elements.  If so, remove them. */
	if (mchan->mchan_sched) {
		WL_MCHAN(("something is wrong, mchan_sched not empty!\n"));
		/* in case timer fires after pretbtt interrupt, clean up */
		wlc_mchan_stop_timer(mchan, MCHAN_TO_REGULAR);
#ifdef WLMCHANPRECLOSE
		wlc_mchan_stop_timer(mchan, MCHAN_TO_PRECLOSE);
#endif
		/* delete all mchan schedule elements */
		wlc_mchan_sched_delete_all(mchan);
	}

	cur_idx = (WLC_MCHAN_SAME_CTLCHAN(wlc->home_chanspec, bsscfg_sel->chanspec)) ?
		mchan->trigger_cfg_idx : mchan->other_cfg_idx;

	if (mchan->switch_algo == MCHAN_DEFAULT_ALGO) {

		/* figure out gap between the 2 to determine what to do next */
		special_return = 0;
		if (sel_bcn_per == oth_bcn_per) {
		/* (next_oth_tbtt - curr_sel_tbtt) and
		 * (curr_sel_tbtt - curr_oth_tbtt) are periodic here.
		 */
		/* update oth_bsscfg sw_dtim_cnt */
		bsscfg_oth->sw_dtim_cnt = (bsscfg_oth->sw_dtim_cnt) ?
			(bsscfg_oth->sw_dtim_cnt - 1) :
			(bsscfg_oth->current_bss->dtim_period - 1);

		if (tbtt_gap == (uint32)ABS((int32)(next_oth_tbtt - curr_sel_tbtt))) {
			next_toggle_switch_time = next_sel_tbtt;
		}
		else {
			next_toggle_switch_time = next_oth_tbtt;
		}
	}
	else {
		uint32 next_next_oth_tbtt = next_oth_tbtt + oth_bcn_per;

		/* For cases of different bcn_per, need to look at
		 * (next_sel_tbtt - next_oth_tbtt) and (next_next_oth_tbtt - next_sel_tbtt)
		 */
		if (next_oth_tbtt >= next_sel_tbtt) {
				if ((next_oth_tbtt - next_sel_tbtt) <=
					MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {
				special_return = 1;
			}
			/* reset next_oth_tbtt as if it is midway between sel_bcn_per */
			next_oth_tbtt = next_sel_tbtt - (sel_bcn_per >> 1);
			tbtt_gap = next_sel_tbtt - next_oth_tbtt;
		}
		else {
			/* update oth_bsscfg sw_dtim_cnt */
			bsscfg_oth->sw_dtim_cnt = (bsscfg_oth->sw_dtim_cnt) ?
			        (bsscfg_oth->sw_dtim_cnt - 1) :
			        (bsscfg_oth->current_bss->dtim_period - 1);

				if ((next_oth_tbtt - curr_sel_tbtt) <=
					MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {
				if ((next_next_oth_tbtt - next_sel_tbtt) <=
					MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {
					tbtt_gap = next_next_oth_tbtt - next_sel_tbtt;
					next_toggle_switch_time = next_sel_tbtt;
				}
				else {
					tbtt_gap = (uint32)(-1);
				}
			}
			else {
				tbtt_gap = next_oth_tbtt - curr_sel_tbtt;
					if ((next_sel_tbtt - next_oth_tbtt) <=
						MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {
					special_return = 1;
				}
			}
		}
	}
	WL_MCHAN(("tbtt_gap = %d, n_sel_tbtt = 0x%x, c_sel_tbtt = 0x%x, "
	          "n_oth_tbtt = 0x%x, c_oth_tbtt = 0x%x, next_toggle_switch_time = 0x%x\n",
	          tbtt_gap, next_sel_tbtt, curr_sel_tbtt, next_oth_tbtt, curr_oth_tbtt,
	          next_toggle_switch_time));

	if (tbtt_gap == (uint32)(-1)) {
		return (err);
	}

/* Not using DTIM based logic as of now */
	use_sel_ctxt = wlc_mchan_dtim_chan_select(wlc, 0, 0,
	                                          bsscfg_sel, bsscfg_oth);
	/* mark the bsscfg that was skipped this time */
		mchan->last_missed_cfg_idx = use_sel_ctxt ?
			bsscfg_oth->_idx : bsscfg_sel->_idx;

		if (sel_bcn_per != oth_bcn_per) {
			if (tbtt_gap <= MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {
				/* perform context switch */
				sel_dur = R_REG(wlc->osh, &wlc->regs->tsf_timerlow);
				sel_dur += MCHAN_TIMER_DELAY_US;
				sel_dur += proc_time;
				sel_dur = ((sel_dur >= next_toggle_switch_time) ? 1 :
					(next_toggle_switch_time - sel_dur));
				wlc_mchan_start_timer(mchan, sel_dur, MCHAN_TO_REGULAR);
						WL_MCHAN(("%s: less than mgap, sel_dur = %d\n",
							__FUNCTION__, sel_dur));

				/* add mchan sched element for bsscfg */
				wlc_mchan_sched_add(mchan, (use_sel_ctxt ? bsscfg_sel->_idx :
					bsscfg_oth->_idx), 0, next_toggle_switch_time-proc_time);
			}
			else {
				wlc_mchan_calc_fair_chan_switch_sched(next_oth_tbtt,
					curr_sel_tbtt, next_sel_tbtt, sel_bcn_per, FALSE,
					&other_start, &other_dur);
				WL_MCHAN(("sel_dur = %d, other_dur = %d\n",
					sel_bcn_per - other_dur, other_dur));
				WL_MCHAN(("oth_start = 0x%x, curr_sel_tbtt = 0x%x, sel_dur=%d\n",
				          other_start, curr_sel_tbtt, (other_start-curr_sel_tbtt)));
				/* adopt context and set mchan timer to fire at other_start time
				* switch to sel chan context if allowed
				* if speciall_bss_idx is valid, means chan switch
				* already in progress
				*/
				if (mchan->special_bss_idx == MCHAN_CFG_IDX_INVALID) {
					wlc_mchan_prepare_pm_mode(wlc->mchan, bsscfg_sel, tsf_l);
			}
			/* set timer to kick off at other_start time */
			sel_dur = R_REG(wlc->osh, &wlc->regs->tsf_timerlow);
			sel_dur = (sel_dur >= other_start) ? 1 : (other_start - sel_dur);
			sel_dur = ((proc_time + MCHAN_TIMER_DELAY_US) < sel_dur) ?
				(sel_dur - proc_time - MCHAN_TIMER_DELAY_US) :
				sel_dur;

			wlc_mchan_start_timer(mchan, sel_dur, MCHAN_TO_REGULAR);
				WL_MCHAN(("%s: greater than mgap, sel_dur = %d\n",
					__FUNCTION__, sel_dur));

				/* This case means that we're switching
				* midway to other channel and staying there
				*/
				if (special_return && !use_sel_ctxt) {
						wlc_mchan_sched_add(mchan, bsscfg_oth->_idx, 0,
							other_start-proc_time);
				}
				/* This case means we switch midway and
				* go back to our channel again
				*/
				else {
					/* add mchan sched element for other bsscfg */
					wlc_mchan_sched_add(mchan, bsscfg_oth->_idx, other_dur,
						other_start-proc_time);
					wlc_mchan_sched_add(mchan, bsscfg_sel->_idx, 0,
						(other_start + other_dur - proc_time));
					if (!special_return) {
						/* clear last_missed_idx */
						mchan->last_missed_cfg_idx = MCHAN_CFG_IDX_INVALID;
					}
				}
			}
		}
		else {
			if (tbtt_gap <= (uint32)MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {

				/* perform context switch */
				if ((cur_idx != mchan->trigger_cfg_idx) && use_sel_ctxt)
					wlc_mchan_prepare_pm_mode(wlc->mchan, bsscfg_sel, tsf_l);
				else if ((cur_idx == mchan->trigger_cfg_idx) && !use_sel_ctxt)
					wlc_mchan_prepare_pm_mode(wlc->mchan, bsscfg_oth, tsf_l);

				sel_dur = R_REG(wlc->osh, &wlc->regs->tsf_timerlow);
				sel_dur += MCHAN_TIMER_DELAY_US;
				sel_dur = ((sel_dur >= next_toggle_switch_time) ? 1 :
					(next_toggle_switch_time - sel_dur));
				sel_dur = (proc_time < sel_dur) ?
					(sel_dur - proc_time) : sel_dur;

				wlc_mchan_start_timer(mchan, sel_dur, MCHAN_TO_REGULAR);

				WL_MCHAN(("%s: less than mgap, sel_dur = %d\n",
					__FUNCTION__, sel_dur));

				/* add mchan sched element for bsscfg */
				wlc_mchan_sched_add(mchan, use_sel_ctxt ?
					bsscfg_oth->_idx : bsscfg_sel->_idx, 0,
					next_toggle_switch_time - proc_time);
			} else {
				mchan->last_missed_cfg_idx = MCHAN_CFG_IDX_INVALID;
				mchan->last_missed_DTIM_idx = MCHAN_CFG_IDX_INVALID;

				if (cur_idx != mchan->trigger_cfg_idx)
					wlc_mchan_prepare_pm_mode(wlc->mchan, bsscfg_sel, tsf_l);
				sel_dur = R_REG(wlc->osh, &wlc->regs->tsf_timerlow);
				if ((sel_dur + (sel_bcn_per / 2)) <
					next_oth_tbtt) {
					start1 = curr_sel_tbtt + (sel_bcn_per / 2);
					start2 = start1 + (oth_bcn_per / 2);
					sel_dur = (sel_dur >= start1) ? 1 :
						(start1 - sel_dur);
					sel_dur = ((proc_time + MCHAN_TIMER_DELAY_US) < sel_dur) ?
						(sel_dur - proc_time - MCHAN_TIMER_DELAY_US) :
						sel_dur;
					wlc_mchan_start_timer(mchan, sel_dur, MCHAN_TO_REGULAR);
					wlc_mchan_sched_add(mchan, mchan->other_cfg_idx,
						start2 - start1, start1 - proc_time);
					wlc_mchan_sched_add(mchan, mchan->trigger_cfg_idx,
						0, start2 - proc_time);
				} else {
					start1 = next_oth_tbtt;
					start2 = start1 + (oth_bcn_per / 2);
					sel_dur = (sel_dur >= start1) ? 1 :
						(start1 - sel_dur);
					sel_dur = ((proc_time + MCHAN_TIMER_DELAY_US) < sel_dur) ?
						(sel_dur - proc_time - MCHAN_TIMER_DELAY_US) :
						sel_dur;
					wlc_mchan_start_timer(mchan, sel_dur, MCHAN_TO_REGULAR);
					wlc_mchan_sched_add(mchan, mchan->other_cfg_idx,
						(oth_bcn_per / 2), start1 - proc_time);
					wlc_mchan_sched_add(mchan, mchan->trigger_cfg_idx,
						0, start2 - proc_time);
				}
			}
		}
		mchan->si_algo_last_switch_time = 0;
	} else if ((sel_bcn_per == oth_bcn_per) &&
		((mchan->switch_algo == MCHAN_BANDWIDTH_ALGO) ||
		(mchan->switch_algo == MCHAN_DYNAMIC_BW_ALGO))) {

		bsscfg_oth->sw_dtim_cnt = (bsscfg_oth->sw_dtim_cnt) ?
			(bsscfg_oth->sw_dtim_cnt - 1) :
			(bsscfg_oth->current_bss->dtim_period - 1);

		sel_dtim_cnt = bsscfg_sel->sw_dtim_cnt;
		oth_dtim_cnt = bsscfg_oth->sw_dtim_cnt;

		if (sel_bcn_per < MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {
				ASSERT(0);
		} else if (sel_bcn_per < 2* MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {
			err = wlc_mchan_alternate_switching(wlc, bsscfg_sel, bsscfg_oth,
				curr_sel_tbtt, curr_oth_tbtt, sel_dtim_cnt, oth_dtim_cnt, tsf_l);
		} else {
			uint32 trigger_ch_time, other_ch_time;
			bool switch_ctx_idx;
			uint32 switchtime_1, switchtime_2, switchtime_3, switchtime_4;
			uint32 trigger_ch_percetage = (mchan->trigger_cfg_idx ==
				wlc->bsscfg[0]->_idx) ?
				mchan->percentage_bw :
				100 - mchan->percentage_bw;

			if (tbtt_gap <= MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {
				if (trigger_ch_percetage < 30)
					trigger_ch_percetage = 30;
				if (trigger_ch_percetage > 70)
					trigger_ch_percetage = 70;
			}

			trigger_ch_time = sel_bcn_per * trigger_ch_percetage / 100;
			other_ch_time = sel_bcn_per - trigger_ch_time;
			if (tbtt_gap < MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {
				/* correcting tbtt from pre-tbtt */
				curr_sel_tbtt += WLC_MCHAN_PRETBTT_TIME_US(mchan);
				next_sel_tbtt += WLC_MCHAN_PRETBTT_TIME_US(mchan);
				next_oth_tbtt += WLC_MCHAN_PRETBTT_TIME_US(mchan);

/* Not using DTIM logic as of now */

				/* Get the index to whcih it has to switch */
				/* Removed dependancy on next DTIM */
				/* DTIM gets missed alternately in any case */
				/* Now the focus is to just switch, irrespective of the DTIM */
				switch_ctx_idx = (wlc_mchan_find_switch_idx(wlc, 0,
					0, bsscfg_sel, bsscfg_oth)) ?
					mchan->trigger_cfg_idx :
					mchan->other_cfg_idx;

				/* Based on the above, set the last missed idx
				* to be used in next iteration
				*/
				mchan->last_missed_cfg_idx = (switch_ctx_idx ==
					mchan->trigger_cfg_idx) ?
					mchan->other_cfg_idx :
					mchan->trigger_cfg_idx;

				/* check if switch context index can be honored */
				if (tbtt_gap <= (uint32)WLC_MCHAN_PRETBTT_TIME_US(mchan) &&
					(switch_ctx_idx != cur_idx)) {
					mchan->last_missed_cfg_idx = switch_ctx_idx;
					switch_ctx_idx = cur_idx;
				}

				if (MIN(trigger_ch_time, other_ch_time) / 2 >
					(uint32)MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) {
					/* Try to centre around the tbtt */
					switchtime_1 = ((switch_ctx_idx == mchan->trigger_cfg_idx) ?
						(curr_sel_tbtt - trigger_ch_time/2) :
						(next_oth_tbtt - other_ch_time/2));
					if (switchtime_1 < tsf_l)
					switchtime_1 = tsf_l;
					switchtime_2 = (curr_sel_tbtt +
						((switch_ctx_idx == mchan->trigger_cfg_idx) ?
						trigger_ch_time : other_ch_time)/2);
					switchtime_3 = (switchtime_2 +
						((switch_ctx_idx == mchan->trigger_cfg_idx) ?
						other_ch_time : trigger_ch_time)/2);
					switchtime_4 = (switchtime_3 +
						((switch_ctx_idx == mchan->trigger_cfg_idx) ?
						trigger_ch_time : other_ch_time)/2);
					sel_dur = switchtime_2 - switchtime_1 -
						MCHAN_TIMER_DELAY_US - proc_time;
					/* set switch time to 1 us if sel_dur */
					/* calculated as negative */
					if ((int)sel_dur <= 0)
						sel_dur = 1;
					wlc_mchan_start_timer(mchan, sel_dur,
						MCHAN_TO_REGULAR);
					wlc_mchan_sched_add(mchan,
						((switch_ctx_idx == mchan->trigger_cfg_idx) ?
						mchan->other_cfg_idx: mchan->trigger_cfg_idx),
						switchtime_3 - switchtime_2,
						switchtime_2 - proc_time);
					wlc_mchan_sched_add(mchan,
						((switch_ctx_idx == mchan->trigger_cfg_idx) ?
						mchan->trigger_cfg_idx: mchan->other_cfg_idx),
						switchtime_4 - switchtime_3,
						switchtime_3 - proc_time);
					wlc_mchan_sched_add(mchan,
						((switch_ctx_idx == mchan->trigger_cfg_idx) ?
						mchan->other_cfg_idx: mchan->trigger_cfg_idx),
						0, switchtime_4 - proc_time);
				} else {
					/* Try to centre around the tbtt */
					switchtime_1 = ((switch_ctx_idx == mchan->trigger_cfg_idx) ?
						(curr_sel_tbtt - trigger_ch_time/2) :
						(next_oth_tbtt - other_ch_time/2));
					if (switchtime_1 < tsf_l)
					switchtime_1 = tsf_l;
					switchtime_2 = curr_sel_tbtt +
						((switch_ctx_idx == mchan->trigger_cfg_idx) ?
						trigger_ch_time : other_ch_time);
					sel_dur = switchtime_2 - switchtime_1 -
						MCHAN_TIMER_DELAY_US - proc_time;
					/* set switch time to 1 us if sel_dur */
					/* calculated as negative */
					if ((int)sel_dur <= 0)
						sel_dur = 1;
					wlc_mchan_start_timer(mchan, sel_dur,
						MCHAN_TO_REGULAR);
					wlc_mchan_sched_add(mchan,
						((switch_ctx_idx == mchan->trigger_cfg_idx) ?
						mchan->other_cfg_idx: mchan->trigger_cfg_idx),
						0, switchtime_2 - proc_time);
				}
			} else {
				mchan->last_missed_cfg_idx = MCHAN_CFG_IDX_INVALID;
				mchan->last_missed_DTIM_idx = MCHAN_CFG_IDX_INVALID;
				if (cur_idx != mchan->trigger_cfg_idx)
					wlc_mchan_prepare_pm_mode(wlc->mchan,
					bsscfg_sel, tsf_l);
				sel_dur = R_REG(wlc->osh, &wlc->regs->tsf_timerlow);
				if (sel_dur + trigger_ch_time < next_oth_tbtt) {
						switchtime_1 = curr_sel_tbtt + trigger_ch_time;
					switchtime_2 = switchtime_1 + other_ch_time;
						sel_dur = (sel_dur >= switchtime_1) ?
							1 : (switchtime_1 - sel_dur);
					sel_dur = ((proc_time + MCHAN_TIMER_DELAY_US) < sel_dur) ?
						(sel_dur - proc_time - MCHAN_TIMER_DELAY_US) :
						sel_dur;
					wlc_mchan_start_timer(mchan, sel_dur, MCHAN_TO_REGULAR);
						wlc_mchan_sched_add(mchan, mchan->other_cfg_idx,
						switchtime_2 - switchtime_1, switchtime_1 -
						proc_time);
					wlc_mchan_sched_add(mchan, mchan->trigger_cfg_idx, 0,
						switchtime_2 - proc_time);
				} else {
					switchtime_1 = next_oth_tbtt;
					switchtime_2 = switchtime_1 + other_ch_time;
					sel_dur = (sel_dur >= switchtime_1) ? 1 :
						(switchtime_1 - sel_dur);
					sel_dur = ((proc_time + MCHAN_TIMER_DELAY_US) < sel_dur) ?
						(sel_dur - proc_time - MCHAN_TIMER_DELAY_US) :
						sel_dur;
					wlc_mchan_start_timer(mchan, sel_dur, MCHAN_TO_REGULAR);
					wlc_mchan_sched_add(mchan, mchan->other_cfg_idx,
						other_ch_time, switchtime_1 - proc_time);
					wlc_mchan_sched_add(mchan, mchan->trigger_cfg_idx,
						0, switchtime_2 - proc_time);
				}
			}
		}
		mchan->si_algo_last_switch_time = 0;
	} else if (mchan->switch_algo == MCHAN_SI_ALGO) {
		uint32 last_switch_time = 0;
		uint32 next_tbtt_gap, next_switch_time, sel_dur;
		uint32 now = tsf_l;
		bool is_dtim, low_gap;
		uint32 rx_wait_time;
		bool same_bcn_inside, other_bcn_inside;
		bool cur_ch_isdtim, oth_ch_isdtim;
		uint8 cur_ch_idx, oth_ch_idx;
		uint32 cur_ch_tbtt, oth_ch_tbtt;
		uint32 next_switch_time_old;
		uint8 next_switch_idx_old, next_switch_idx;
		uint32 use_sel_ctxt;
		bool init_timer = FALSE;
		wlc_bsscfg_t *bsscfg_cur, *bsscfg_othr, *bsscfg_tmp;
		uint32 sel_tbtt, oth_tbtt;

		sel_tbtt = curr_sel_tbtt;
		if (curr_oth_tbtt > now) {
			oth_tbtt = curr_oth_tbtt;
		} else {
			oth_tbtt = next_oth_tbtt;
		}

		if (((curr_sel_tbtt < curr_oth_tbtt) && (curr_oth_tbtt < next_sel_tbtt)) ||
			((curr_sel_tbtt < next_oth_tbtt) && (next_oth_tbtt < next_sel_tbtt)))
		bsscfg_oth->sw_dtim_cnt = (bsscfg_oth->sw_dtim_cnt) ?
		(bsscfg_oth->sw_dtim_cnt - 1) : (bsscfg_oth->current_bss->dtim_period - 1);

		/* Check whether the last_switch_time value is valid or not -
		 * The value is expected to be greater than the tbtt time
		 */
		last_switch_time =  mchan->si_algo_last_switch_time;
		if (last_switch_time == 0 || last_switch_time < now)
			last_switch_time = now;

		next_switch_time = last_switch_time;

		if (last_switch_time > now && last_switch_time < next_sel_tbtt) {
			next_switch_idx = (mchan->trigger_cfg_idx == cur_idx) ?
				mchan->other_cfg_idx : mchan->trigger_cfg_idx;
			if (!init_timer) {
				sel_dur = R_REG(wlc->osh, &wlc->regs->tsf_timerlow) +
					MCHAN_TIMER_DELAY_US;
				sel_dur = (sel_dur >= next_switch_time) ?
					1 : (next_switch_time - sel_dur);
				wlc_mchan_start_timer(mchan, sel_dur, MCHAN_TO_REGULAR);
				init_timer = TRUE;
				if (sel_tbtt <= next_switch_time)
					sel_tbtt += sel_bcn_per;
				if (oth_tbtt <= next_switch_time)
					oth_tbtt += oth_bcn_per;
			}
		} else {
			next_switch_idx = cur_idx;
		}

		next_switch_time_old = next_switch_time;
		next_switch_idx_old = next_switch_idx;

		cur_ch_tbtt = (next_switch_idx == mchan->trigger_cfg_idx) ? sel_tbtt : oth_tbtt;
		cur_ch_idx = (next_switch_idx == mchan->trigger_cfg_idx) ?
			mchan->trigger_cfg_idx : mchan->other_cfg_idx;
		oth_ch_tbtt = (next_switch_idx == mchan->trigger_cfg_idx) ? oth_tbtt : sel_tbtt;
		oth_ch_idx = (next_switch_idx == mchan->trigger_cfg_idx) ?
			mchan->other_cfg_idx : mchan->trigger_cfg_idx;
		bsscfg_cur =  (next_switch_idx == mchan->trigger_cfg_idx) ?
			bsscfg_sel : bsscfg_oth;
		bsscfg_othr =  (next_switch_idx == mchan->trigger_cfg_idx) ?
			bsscfg_oth : bsscfg_sel;

		while (TRUE) {
			next_switch_idx = (mchan->trigger_cfg_idx == next_switch_idx) ?
				mchan->other_cfg_idx : mchan->trigger_cfg_idx;

			next_tbtt_gap = ABS((int32)(oth_ch_tbtt - cur_ch_tbtt));

			cur_ch_isdtim = FALSE; /* To be done */
			oth_ch_isdtim = FALSE; /* To be done */

			is_dtim = (cur_ch_tbtt < oth_ch_tbtt) ? cur_ch_isdtim : oth_ch_isdtim;

			rx_wait_time = is_dtim ?
				MCHAN_MIN_DTIM_RX_TIME_US: MCHAN_MIN_BCN_RX_TIME_US;

			rx_wait_time += WLC_MCHAN_PRETBTT_TIME_US(mchan);

			low_gap = (next_tbtt_gap < (rx_wait_time));

			use_sel_ctxt = wlc_mchan_find_switch_idx(wlc, cur_ch_isdtim,
				oth_ch_isdtim, bsscfg_cur, bsscfg_othr);

			same_bcn_inside = (cur_ch_tbtt > next_switch_time) &&
				(cur_ch_tbtt < (next_switch_time + mchan->switch_interval +
				MCHAN_MIN_CHANNEL_PERIOD(mchan)));
			other_bcn_inside = (oth_ch_tbtt > next_switch_time) &&
				(oth_ch_tbtt < (next_switch_time + mchan->switch_interval));

			if (low_gap) {
				if (!(same_bcn_inside || other_bcn_inside)) {
					next_switch_time = MAX(next_switch_time +
						mchan->switch_interval, now);
				} else if (use_sel_ctxt) {
					next_switch_time = MAX(next_switch_time +
						mchan->switch_interval,
						cur_ch_tbtt + rx_wait_time);
					mchan->last_missed_cfg_idx = oth_ch_idx;
				} else {
					if ((oth_ch_tbtt - next_switch_time) >
						MCHAN_MIN_CHANNEL_PERIOD(mchan)) {
						next_switch_time = MAX(MIN(next_switch_time +
							mchan->switch_interval, oth_ch_tbtt),
							R_REG(wlc->osh, &wlc->regs->tsf_timerlow));
						mchan->last_missed_cfg_idx = cur_ch_idx;
					} else {
						next_switch_time = MAX(next_switch_time +
							mchan->switch_interval,
							cur_ch_tbtt + rx_wait_time);
						mchan->last_missed_cfg_idx = oth_ch_idx;
					}
				}
			} else {
				if (!(same_bcn_inside || other_bcn_inside)) {
					next_switch_time = MAX(mchan->switch_interval +
						next_switch_time, now);
				} else if (other_bcn_inside && !same_bcn_inside) {
					if (oth_ch_tbtt > next_switch_time +
						MCHAN_MIN_CHANNEL_PERIOD(mchan)) {
						next_switch_time = oth_ch_tbtt;
					} else {
						next_switch_time = next_switch_time +
							mchan->switch_interval;
						mchan->last_missed_cfg_idx = oth_ch_idx;
					}
				} else if (!other_bcn_inside && same_bcn_inside) {
					if (oth_ch_tbtt > cur_ch_tbtt) {
						next_switch_time = MIN(oth_ch_tbtt,
							MAX(next_switch_time +
							mchan->switch_interval,
							cur_ch_tbtt + rx_wait_time));
					} else {
						next_switch_time = MAX(next_switch_time +
							mchan->switch_interval,
							cur_ch_tbtt + rx_wait_time);
					}
				} else {
					if (oth_ch_tbtt < next_switch_time +
						MCHAN_MIN_CHANNEL_PERIOD(mchan)) {
						next_switch_time = MAX(next_switch_time +
							mchan->switch_interval,
							cur_ch_tbtt + rx_wait_time);
						mchan->last_missed_cfg_idx = oth_ch_idx;
					} else {
						next_switch_time = oth_ch_tbtt;
					}
				}
			}

			if (cur_ch_tbtt > next_switch_time_old && cur_ch_tbtt < next_switch_time)
				cur_ch_tbtt = cur_ch_tbtt +
					(bsscfg_cur->current_bss->beacon_period << 10);
			if (oth_ch_tbtt > next_switch_time_old && oth_ch_tbtt < next_switch_time)
				oth_ch_tbtt = oth_ch_tbtt +
					(bsscfg_othr->current_bss->beacon_period << 10);

			if (next_switch_time > next_sel_tbtt) {
				mchan->si_algo_last_switch_time = next_switch_time;
				break;
			}

			if (!init_timer) {
				sel_dur = R_REG(wlc->osh, &wlc->regs->tsf_timerlow) +
					MCHAN_TIMER_DELAY_US;
				sel_dur = (sel_dur >= next_switch_time) ?
					1 : (next_switch_time - sel_dur);
				wlc_mchan_start_timer(mchan, sel_dur, MCHAN_TO_REGULAR);
				init_timer = TRUE;
			} else {
				wlc_mchan_sched_add(mchan, next_switch_idx_old,
					next_switch_time - next_switch_time_old,
					next_switch_time_old);
			}
			next_switch_time_old = next_switch_time;
			next_switch_idx_old = next_switch_idx;
			last_switch_time = next_switch_time;

			cur_ch_tbtt ^= oth_ch_tbtt;
			oth_ch_tbtt ^= cur_ch_tbtt;
			cur_ch_tbtt ^= oth_ch_tbtt;

			cur_ch_idx ^= oth_ch_idx;
			oth_ch_idx ^= cur_ch_idx;
			cur_ch_idx ^= oth_ch_idx;

			bsscfg_tmp = bsscfg_cur;
			bsscfg_cur = bsscfg_othr;
			bsscfg_othr = bsscfg_tmp;
		}
		wlc_mchan_sched_add(mchan, next_switch_idx_old, 0, next_switch_time_old);
	}
	return err;
}

static int wlc_mchan_alternate_switching(wlc_info_t *wlc,
	wlc_bsscfg_t *bsscfg_sel,
	wlc_bsscfg_t *bsscfg_oth,
	uint32 curr_sel_tbtt,
	uint32 curr_oth_tbtt,
	uint32 sel_dtim_cnt,
	uint32 oth_dtim_cnt,
	uint32 tsf_l) {

	bool sel_is_dtim, oth_is_dtim, use_sel_ctxt;
	mchan_info_t *mchan = wlc->mchan;

	/* make sure the tbtt that comes first in time is used for trigger idx
	 * The first ABS() check can evaluate to true eventhough our selected tbtt
	 * is already the earlier tbtt if the two tbtts are very close together and
	 * we did not service our selected tbtt interrupt until after the curr_oth_tbtt
	 * has already fired.  Thus, we need to put a second check to make sure that
	 * curr_sel_tbtt is indeed later than curr_oth_tbtt.
	 */
	if ((ABS((int32)(curr_sel_tbtt - curr_oth_tbtt)) <=
		(int32) MCHAN_MIN_TBTT_GAP_US_NEW(mchan)) &&
		(curr_sel_tbtt > curr_oth_tbtt)) {
		WL_MCHAN(("trigger idx change from %d to %d",
		mchan->trigger_cfg_idx, mchan->other_cfg_idx));
		/* swap the two indices */
		mchan->trigger_cfg_idx ^= mchan->other_cfg_idx;
		mchan->other_cfg_idx ^= mchan->trigger_cfg_idx;
		mchan->trigger_cfg_idx ^= mchan->other_cfg_idx;
		/* clear last_missed idx */
		mchan->last_missed_cfg_idx = MCHAN_CFG_IDX_INVALID;
		return (BCME_NOTREADY);
}

	/* current tbtts too close to each other */
	/* Need to use special dtim based rules to schedule channel switch */

	/* figure out which tbtt is currently a dtim */
	sel_is_dtim = (sel_dtim_cnt == 0) ? TRUE : FALSE;
	oth_is_dtim = (oth_dtim_cnt == 0) ? TRUE : FALSE;

	/* if dtim count for bsscfg is 1, then don't use special dtim rules */
	if (bsscfg_sel->current_bss->dtim_period == 1) {
		sel_is_dtim = FALSE;
	}

	if (bsscfg_oth->current_bss->dtim_period == 1) {
		oth_is_dtim = FALSE;
	}

	WL_MCHAN(("%s: sel_is_dtim = %d, sel_dtim_cnt = %d, "
			  "oth_is_dtim = %d, oth_dtim_cnt = %d\n",
			  __FUNCTION__, sel_is_dtim, sel_dtim_cnt,
			  oth_is_dtim, oth_dtim_cnt));

	use_sel_ctxt = wlc_mchan_find_switch_idx(wlc, sel_is_dtim, oth_is_dtim,
		bsscfg_sel, bsscfg_oth);


	WL_MCHAN(("%s: use_sel_ctxt = %d, last_missed_cfg_idx = %d\n",
		__FUNCTION__, use_sel_ctxt, mchan->last_missed_cfg_idx));

	/* perform context switch */
	wlc_mchan_prepare_pm_mode(wlc->mchan, (use_sel_ctxt ? bsscfg_sel : bsscfg_oth), tsf_l);
	/* mark the bsscfg that was skipped this time */
	{
		uint8 missed_idx = use_sel_ctxt ? bsscfg_oth->_idx : bsscfg_sel->_idx;
		uint8 used_idx = use_sel_ctxt ? bsscfg_sel->_idx : bsscfg_oth->_idx;

		if (mchan->missed_cnt[used_idx]) {
			mchan->missed_cnt[used_idx]--;
			if (mchan->missed_cnt[used_idx] == 0) {
				mchan->missed_cnt[missed_idx]++;
				mchan->last_missed_cfg_idx = missed_idx;
			} else
				mchan->last_missed_cfg_idx = used_idx;
		} else {
			if (mchan->missed_cnt[missed_idx] < 2)
				mchan->missed_cnt[missed_idx]++;
			mchan->last_missed_cfg_idx = missed_idx;
		}
	}
	return BCME_OK;
}


bool wlc_mchan_find_switch_idx(wlc_info_t *wlc,
	bool sel_is_dtim,
	bool oth_is_dtim,
	wlc_bsscfg_t *bsscfg_sel,
	wlc_bsscfg_t *bsscfg_oth) {

	bool use_sel_ctxt = FALSE;
	mchan_info_t *mchan = wlc->mchan;

	if (sel_is_dtim && !oth_is_dtim) {
		use_sel_ctxt = TRUE;
		if (mchan->last_missed_DTIM_idx == bsscfg_sel->_idx)
			mchan->last_missed_DTIM_idx = MCHAN_CFG_IDX_INVALID;
	} else if (!sel_is_dtim && oth_is_dtim) {
		if (mchan->last_missed_DTIM_idx == bsscfg_oth->_idx)
			mchan->last_missed_DTIM_idx = MCHAN_CFG_IDX_INVALID;
	} else if (sel_is_dtim && oth_is_dtim) {
		if (mchan->last_missed_DTIM_idx == bsscfg_sel->_idx) {
			/* go to selected chan context */
			use_sel_ctxt = TRUE;
		} else if (mchan->last_missed_DTIM_idx != bsscfg_oth->_idx) {
			if (mchan->last_missed_cfg_idx != MCHAN_CFG_IDX_INVALID) {
				if (mchan->last_missed_cfg_idx == bsscfg_sel->_idx) {
					use_sel_ctxt = TRUE;
				}
			}
			else{ /* randomly select channel */
				use_sel_ctxt = R_REG(wlc->osh,
					&wlc->regs->u.d11regs.tsf_random) & 0x1;
			}
		}
		mchan->last_missed_DTIM_idx = use_sel_ctxt ? bsscfg_oth->_idx : bsscfg_sel->_idx;
	} else {
		/* NON-DTIM rules (when both are DTIMs, treated as neither being DTIMs */
		/* go to the channel whose beacon was last missed */
		if (mchan->last_missed_cfg_idx == bsscfg_sel->_idx) {
			/* go to selected chan context */
			use_sel_ctxt = TRUE;
		} else if (mchan->last_missed_cfg_idx == bsscfg_oth->_idx) {
			/* go to other chan context */
			use_sel_ctxt = FALSE;
		} else { /* randomly select channel */
			use_sel_ctxt = R_REG(wlc->osh, &wlc->regs->u.d11regs.tsf_random) & 0x1;
		}
	}
	return use_sel_ctxt;
}

static void
wlc_mchan_pretbtt_cb(void *ctx, wlc_mcnx_intr_data_t *notif_data)
{
	if (notif_data->intr != M_P2P_I_PRE_TBTT)
		return;

	/* prevent unnecessary local variable initialization
	 *in case not M_P2P_I_PRE_TBTT
	 */
	_wlc_mchan_pretbtt_cb(ctx, notif_data);
}


static void
_wlc_mchan_pretbtt_cb(void *ctx, wlc_mcnx_intr_data_t *notif_data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	mchan_info_t *mchan = wlc->mchan;
	wlc_bsscfg_t *bsscfg = notif_data->cfg;
	uint32 tsf_h = notif_data->tsf_h;
	uint32 tsf_l = notif_data->tsf_l;


	if (!(MCHAN_ENAB(wlc->pub) && MCHAN_ACTIVE(wlc->pub))) {
		return;
	}

	if (!wlc_mcnx_tbtt_valid(wlc->mcnx, bsscfg)) {
		return;
	}

	/* if bsscfg missed too many beacons, block 1 chan switch to help
	 * with bcn reception.
	 * Only do this if there are no AP's active.
	 * If AP's active, then we don't want to block channel switch to AP channel
	 */
	if (!AP_ACTIVE(wlc) && (bsscfg->mchan_tbtt_since_bcn >
		mchan->tbtt_since_bcn_thresh) && (mchan->blocking_bsscfg == NULL)) {
		WL_MCHAN(("wl%d.%d: need to block chansw, mchan_tbtt_since_bcn = %d\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),	bsscfg->mchan_tbtt_since_bcn));
		mchan->blocking_bsscfg = bsscfg;
		mchan->blocking_cnt = MCHAN_BLOCKING_CNT;
	}

#ifdef STA
	/* Start monitoring at tick 1 */
	if (BSSCFG_STA(bsscfg) && bsscfg->BSS && mchan->blocking_enab) {
		bsscfg->mchan_tbtt_since_bcn++;
	}
#endif

	if (mchan->trigger_cfg_idx != bsscfg->_idx) {
		return;
	}

	if (BSSCFG_AP(bsscfg)) {
		wlc_mchan_ap_pretbtt_proc(wlc, bsscfg, tsf_l, tsf_h);
	}
	else {
		wlc_mchan_sta_pretbtt_proc(wlc, bsscfg, tsf_l, tsf_h);
	}
}

void
wlc_mchan_recv_process_beacon(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct scb *scb,
	wlc_d11rxhdr_t *wrxh, uint8 *plcp, uint8 *body)
{
	mchan_info_t *mchan = wlc->mchan;

	ASSERT(bsscfg != NULL);
	ASSERT(BSSCFG_STA(bsscfg) && bsscfg->BSS);
	ASSERT(scb != NULL);

	if (bsscfg->mchan_tbtt_since_bcn > mchan->tbtt_since_bcn_thresh) {
		/* force p2p module to update tbtt */
		wlc_mcnx_tbtt_inv(wlc->mcnx, bsscfg);
		WL_MCHAN(("wl%d.%d: force tbtt upd, mchan_tbtt_since_bcn = %d\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
			bsscfg->mchan_tbtt_since_bcn));
	}
	if (bsscfg == wlc_mchan_get_blocking_bsscfg(wlc->mchan)) {
		wlc_mchan_reset_blocking_bsscfg(wlc->mchan);
	}
	bsscfg->mchan_tbtt_since_bcn = 0;
}

void
wlc_mchan_client_noa_clear(mchan_info_t *mchan, wlc_bsscfg_t *cfg)
{
	WL_MCHAN(("wl%d.%d: %s: clean up mchan abs/psc states\n", mchan->wlc->pub->unit,
	          WLC_BSSCFG_IDX(cfg), __FUNCTION__));
	wlc_mchan_stop_timer(mchan, MCHAN_TO_ABS);
	cfg->last_psc_intr_time = 0;
	wlc_mchan_psc_cleanup(mchan, cfg);
	cfg->in_psc = FALSE;
	cfg->in_abs = FALSE;
}

static void wlc_mchan_if_byte(wlc_info_t *wlc, uint8 idx)
{
	mchan_info_t *mchan = wlc->mchan;
	uint32 new_txbytes, new_rxbytes, old_txbytes, old_rxbytes;
	uint8 w;

#ifdef WLCNT
	new_txbytes = wlc->pub->_cnt->txbyte;
	new_rxbytes = wlc->pub->_cnt->rxbyte;
#else
	new_txbytes = new_rxbytes = 0;
#endif
	old_txbytes = mchan->dyn_algo->if_bytes[1 - idx].tot_txbytes;
	old_rxbytes = mchan->dyn_algo->if_bytes[1 - idx].tot_rxbytes;
	w = mchan->dyn_algo->if_bytes[idx].w;

	mchan->dyn_algo->if_bytes[idx].txbytes[w] = new_txbytes - old_txbytes;
	mchan->dyn_algo->if_bytes[idx].rxbytes[w] = new_rxbytes - old_rxbytes;
	mchan->dyn_algo->if_bytes[idx].w = ((mchan->dyn_algo->if_bytes[idx].w +
		1) % MCHAN_DYN_ALGO_SAMPLE);

#ifdef WLCNT
	/* Cache the new TX-RX byte values */
	mchan->dyn_algo->if_bytes[idx].tot_txbytes = wlc->pub->_cnt->txbyte;
	mchan->dyn_algo->if_bytes[idx].tot_rxbytes = wlc->pub->_cnt->rxbyte;
#else
	mchan->dyn_algo->if_bytes[idx].tot_txbytes = 0;
	mchan->dyn_algo->if_bytes[idx].tot_rxbytes = 0;
#endif
}

void
wlc_mchan_abs_proc(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint32 tsf_l)
{
	wlc_bsscfg_t *bsscfg;
	mchan_info_t *mchan = wlc->mchan;
	uint32 end_time = 0;


	/* return if not mchan not active or cfg is not AP */
	if (!(MCHAN_ENAB(wlc->pub) && MCHAN_ACTIVE(wlc->pub))) {
		return;
	}

	if (!BSSCFG_AP(cfg)) {
		cfg->in_psc = FALSE;
		cfg->in_abs = TRUE;
		return;
	}

	ASSERT(wlc->mchan->other_cfg_idx != MCHAN_CFG_IDX_INVALID);

	bsscfg = WLC_BSSCFG(wlc, wlc->mchan->other_cfg_idx);
	if (!bsscfg) {
		return;
	}

	if (mchan->noa_dur) {
		/* Make sure we have no unserviced schedule elements.  If so, remove them. */
		if (mchan->mchan_sched) {
			WL_MCHAN(("something is wrong, mchan_sched not empty!\n"));
			/* in case timer fires after abs interrupt, clean up */
			wlc_mchan_stop_timer(mchan, MCHAN_TO_REGULAR);
			/* delete all mchan schedule elements */
			wlc_mchan_sched_delete_all(mchan);
		}
		/* add timer to schedule channel switch for end of abs period */
		wlc_mchan_start_timer(mchan, mchan->noa_dur, MCHAN_TO_REGULAR);
		/* add mchan sched element for ap, which is cfg */
		wlc_mchan_sched_add(mchan, cfg->_idx, 0, end_time);
	}
	/* switch to STA's chan context if allowed */
	wlc_mchan_prepare_pm_mode(wlc->mchan, bsscfg, tsf_l);
}

static void
wlc_mchan_psc_cleanup(mchan_info_t *mchan, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = mchan->wlc;

	if (mboolisset(cfg->pm->PMblocked, WLC_PM_BLOCK_MCHAN_ABS)) {
		/* unblock pm for abs */
		mboolclr(cfg->pm->PMblocked, WLC_PM_BLOCK_MCHAN_ABS);

		/* on channel and pm still enabled, get us out of pm enabled state */
		if ((cfg->pm->PM == PM_OFF || cfg->pm->WME_PM_blocked || mchan->bypass_pm) &&
		    cfg->pm->PMenabled && (wlc->chanspec == cfg->chan_context->chanspec)) {
			wlc_set_pmstate(cfg, FALSE);
			WL_MCHAN(("wl%d.%d: in psc, bring cfg out of ps mode\n", wlc->pub->unit,
			          WLC_BSSCFG_IDX(cfg)));
		}
	}
}

void
wlc_mchan_psc_proc(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint32 tsf_l)
{
#ifdef WLMCHANPRECLOSE
	 uint32 ap_bcn_per;
#endif

	if (BSSCFG_STA(cfg)) {
		cfg->last_psc_intr_time	= wlc->pub->now;
		cfg->in_psc = TRUE;
		cfg->in_abs = FALSE;
		wlc_mchan_psc_cleanup(wlc->mchan, cfg);
	}

	/* return if mchan not active */
	if (!(MCHAN_ENAB(wlc->pub) && MCHAN_ACTIVE(wlc->pub))) {
		return;
	}

	if (!WLC_MCHAN_SAME_CTLCHAN(cfg->current_bss->chanspec, wlc->chanspec)) {
		WL_MCHAN(("%s: channel is still %d, should have switched to %d\n",
			__FUNCTION__, CHSPEC_CHANNEL(wlc->chanspec),
			CHSPEC_CHANNEL(cfg->current_bss->chanspec)));
	}
	/* switch to GO's chan context if allowed */
	/* if speciall_bss_idx is valid, means chan switch already in progress */
	if (BSSCFG_AP(cfg) && wlc->mchan->special_bss_idx == MCHAN_CFG_IDX_INVALID) {
		wlc_mchan_prepare_pm_mode(wlc->mchan, cfg, tsf_l);
	}
	else if (BSSCFG_STA(cfg)) {
		mchan_info_t *mchan = wlc->mchan;
		uint32 next_abs_time;
		int32 timeout;
		uint32 proc_time = MCHAN_MAX_PM_NULL_TX_TIME_US;

		/* For GC, we need to put GC in PS mode just before abs comes */
		next_abs_time = wlc_mcnx_read_shm(wlc->mcnx,
			M_P2P_BSS_N_NOA(wlc_mcnx_BSS_idx(wlc->mcnx, cfg)));
		next_abs_time <<= P2P_UCODE_TIME_SHIFT;
		wlc_tbtt_ucode_to_tbtt32(tsf_l, &next_abs_time);
		mchan->abs_bss_idx = WLC_BSSCFG_IDX(cfg);
		timeout = (int)(next_abs_time - tsf_l - MCHAN_TIMER_DELAY_US - proc_time);
		if (timeout < 0) {
			WL_MCHAN(("wl%d.%d: %s, no time left: next_abs_time = 0x%x, tsf_l = 0x%x\n"
			          "MCHAN_TIMER_DELAY_US = %d, proc_time = %d, rawdur = %d\n",
			          wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
			          next_abs_time, tsf_l, MCHAN_TIMER_DELAY_US, proc_time,
			          next_abs_time-tsf_l));
			timeout = 1;
		}
		wlc_mchan_start_timer(mchan, (uint32)timeout, MCHAN_TO_ABS);
	}
#ifdef WLMCHANPRECLOSE
	if (BSSCFG_AP(cfg)) {
		uint32 close_time;
		uint32 ap_int_per;
		ap_bcn_per = cfg->current_bss->beacon_period << 10;

		/* Change the AP beacon period depending on no. of abs */
		/* period for diff algorithms */
		ap_int_per = ap_bcn_per/wlc->mchan->abs_cnt;

		close_time = ap_int_per - wlc->mchan->noa_dur -
			wlc->mchan->pre_interface_close_time - wlc->mchan->proc_time_us -
			MCHAN_TIMER_DELAY_US;
		wlc_mchan_start_timer(wlc->mchan, close_time, MCHAN_TO_PRECLOSE);
	}
#endif /* WLMCHANPRECLOSE */

}

static uint16 wlc_mchan_pretbtt_time_us(mchan_info_t *mchan)
{
	if (SRHWVSDB_ENAB(mchan->wlc->pub)) {
		return 6000;
	}
	return 2000;
}

#ifndef WLC_HIGH_ONLY
static uint16 wlc_mchan_proc_time_us(mchan_info_t *mchan)
{
	uint16 default_proc_time = 10000 - wlc_mchan_pretbtt_time_us(mchan);
#ifdef BCM4354
	default_proc_time += 3000;
#endif
	return default_proc_time;
}
#endif /* WLC_HIGH_ONLY */

#ifdef WLC_HIGH_ONLY
static uint16 wlc_mchan_proc_time_bmac_us(mchan_info_t *mchan)
{
	uint16 us = 15000;

	if (SRHWVSDB_ENAB(mchan->wlc->pub)) {
		us = 9000;
	}
	return us - wlc_mchan_pretbtt_time_us(mchan);
}
#endif /* WLC_HIGH_ONLY */
/* These functions register/unregister a callback that wlc_mchan_notif_cb_notif may invoke. */
int
wlc_mchan_notif_cb_register(mchan_info_t *mchan, wlc_mchan_notif_cb_fn_t cb, void *arg)
{
	bcm_notif_h hdl = mchan->mchan_notif_hdl;
	return bcm_notif_add_interest(hdl, (bcm_notif_client_callback)cb, arg);
}

int
wlc_mchan_notif_cb_unregister(mchan_info_t *mchan, wlc_mchan_notif_cb_fn_t cb, void *arg)
{
	bcm_notif_h hdl = mchan->mchan_notif_hdl;
	return bcm_notif_remove_interest(hdl, (bcm_notif_client_callback)cb, arg);
}

#if defined(WL_PROT_OBSS) && defined(WL_MODESW)
static void
wlc_mchan_notif_cb_notif(mchan_info_t *mchan, wlc_bsscfg_t *cfg, int status, int signal)
{
	wlc_mchan_notif_cb_data_t notif_data;
	bcm_notif_h hdl = mchan->mchan_notif_hdl;
	notif_data.cfg = cfg;
	notif_data.status = status;
	notif_data.signal = signal;
	bcm_notif_signal(hdl, &notif_data);
	return;
}
#endif
#endif /* WLMCHAN */
