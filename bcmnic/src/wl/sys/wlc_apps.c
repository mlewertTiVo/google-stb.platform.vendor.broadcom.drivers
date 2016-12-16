/*
 * Common interface to the 802.11 AP Power Save state per scb
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
 * $Id: wlc_apps.c 663543 2016-10-05 21:41:16Z $
 */

/**
 * @file
 * @brief
 * Twiki: [WlDriverPowerSave]
 */

/* Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */
#include <wlc_cfg.h>

#ifndef AP
#error "AP must be defined to include this module"
#endif  /* AP */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <proto/802.11.h>
#include <proto/wpa.h>
#ifdef WL_DATAPATH_LOG_DUMP
#include <event_log.h>
#endif
#include <wlioctl.h>
#include <epivers.h>

#include <sbhndpio.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_keymgmt.h>
#include <wlc_channel.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_ap.h>
#include <wlc_apps.h>
#include <wlc_scb.h>
#include <wlc_phy_hal.h>
#include <bcmwpa.h>
#include <wlc_bmac.h>
#ifdef PROP_TXSTATUS
#include <wlc_wlfc.h>
#endif
#ifdef WLTDLS
#include <wlc_tdls.h>
#endif
#include <wl_export.h>
#ifdef WLAMPDU
#include <wlc_ampdu.h>
#endif /* WLAMPDU */
#include <wlc_pcb.h>
#ifdef WLTOEHW
#include <wlc_tso.h>
#endif /* WLTOEHW */
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#ifdef WLWNM
#include <wlc_wnm.h>
#endif
#include "wlc_txc.h"
#include <wlc_tx.h>
#include <wlc_mbss.h>
#include <wlc_txmod.h>
#include <wlc_pspretend.h>
#include <wlc_qoscfg.h>
#ifdef WL_BEAMFORMING
#include <wlc_txbf.h>
#endif /* WL_BEAMFORMING */
#ifdef BCMFRWDPOOLREORG
#include <hnd_poolreorg.h>
#endif /* BCMFRWDPOOLREORG */
#ifdef TXQ_MUX
#include <wlc_bcmc_txq.h>
#endif

// Forward declarations
struct apps_scb_psinfo;


static void wlc_apps_ps_flush(wlc_info_t *wlc, struct scb *scb);
#if !defined(TXQ_MUX)
// Use a different PS aging mechanism for TXQ_MUX
static void wlc_apps_ps_timedout(wlc_info_t *wlc, struct scb *scb);
// using SCBQ output fn to send pspoll responses instead of the following
static bool wlc_apps_ps_send(wlc_info_t *wlc, struct scb *scb, uint prec_bmp, uint32 flags);
static bool wlc_apps_ps_enq_resp(wlc_info_t *wlc, struct scb *scb, void *pkt, int prec);
#endif /* TXQ_MUX */
static void wlc_apps_ps_enq(void *ctx, struct scb *scb, void *pkt, uint prec);
#if defined(TXQ_MUX) // new output mechansim for PSPoll response
static void wlc_apps_enable_ps_output(wlc_info_t *wlc, struct scb *scb);
static uint wlc_apps_nulldata_output(struct scb *scb, uint ac, struct spktq *output_q);
#endif /* TXQ_MUX */
#if defined(TXQ_MUX) && defined(PROP_TXSTATUS)
static uint wlc_apps_apsd_max_delv_aci(uint8 delv_bmp);
#endif /* TXQ_MUX && PROP_TXSTATUS */
static int wlc_apps_apsd_delv_count(wlc_info_t *wlc, struct scb *scb);
static int wlc_apps_apsd_ndelv_count(wlc_info_t *wlc, struct scb *scb);
#if !defined(TXQ_MUX)
// new output mechanism instead of the following
static void wlc_apps_apsd_send(wlc_info_t *wlc, struct scb *scb);
// new ps queue mgmt obsoletes these fns
static void wlc_apps_txq_to_psq(wlc_info_t *wlc, struct scb *scb);
static void wlc_apps_move_to_psq(wlc_info_t *wlc, struct pktq *txq, struct scb* scb);
#if defined(WLAMPDU) && defined(PROP_TXSTATUS)
static void wlc_apps_ampdu_txq_to_psq(wlc_info_t *wlc, struct scb *scb);
#endif
#endif /* !TXQ_MUX */
static int wlc_apps_down(void *hdl);
#if !defined(TXQ_MUX)
// new apsd output mechanism obsoletes this
static void wlc_apps_apsd_eosp_send(wlc_info_t *wlc, struct scb *scb);
#endif /* !TXQ_MUX */
static int wlc_apps_bss_init(void *ctx, wlc_bsscfg_t *cfg);
static int wlc_apps_cubby_get(void *ctx, wlc_bsscfg_t *cfg, uint8 *data, int *len);
static int wlc_apps_cubby_set(void *ctx, wlc_bsscfg_t *cfg, const uint8 *data, int len);
static void wlc_apps_bss_deinit(void *ctx, wlc_bsscfg_t *cfg);
static int wlc_apps_scb_update(void *context, struct scb *scb, wlc_bsscfg_t* new_cfg);


#if defined(MBSS)
static void wlc_apps_bss_ps_off_start(wlc_info_t *wlc, struct scb *bcmc_scb);
#else
#define wlc_apps_bss_ps_off_start(wlc, bcmc_scb)
#endif /* MBSS */

/* IE mgmt */
static uint wlc_apps_calc_tim_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_apps_write_tim_ie(void *ctx, wlc_iem_build_data_t *data);
static void wlc_apps_scb_state_upd_cb(void *ctx, scb_state_upd_data_t *notif_data);
#if !defined(TXQ_MUX) // Using a different null data pkt prep method
static int wlc_apps_send_psp_response_cb(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt, void *data);
#endif

#ifdef PROP_TXSTATUS
/* Maximum suppressed broadcast packets handled */
#define BCMC_MAX 4
#endif

/* Flags for psp_flags */
#define PS_MORE_DATA		0x01
#define PS_PSP_REQ_PEND		0x02
#define PS_PSP_ONRESP		0x04	/* a pspoll req is under handling (SWWLAN-42801) */

/* PS transition status flags of apps_scb_psinfo */
#define SCB_PS_TRANS_OFF_PEND		0x01	/* Pend PS off until txfifo draining is done */
#define SCB_PS_TRANS_OFF_BLOCKED	0x02	/* Block attempts to switch PS off */
#define SCB_PS_TRANS_OFF_IN_PROG	0x04	/* PS off transition is already in progress */

/* PS transition status flags of apps_bss_psinfo */
#define BSS_PS_TRANS_OFF_BLOCKED	0x01	/* Block attempts to switch PS of bcmc scb off */

#if defined(TXQ_MUX)

/*
 * PSQ pktq needs to hold any number of pkts suppressed from tx fifo
 */
#define PSQ_PKTQ_LEN_DEFAULT    PKTQ_LEN_MAX    /* hold any number of pkts */

#else /* TXQ_MUX */

#define PSQ_PKTQ_LEN_DEFAULT    512             /* Max 512 packets */

#endif /* TXQ_MUX */

/* power-save mode definitions */
#ifndef PSQ_PKTS_LO
#define PSQ_PKTS_LO		5	/* max # PS pkts scb can enq */
#endif
#ifndef PSQ_PKTS_HI
#define	PSQ_PKTS_HI		500
#endif /* PSQ_PKTS_HI */

struct apps_scb_psinfo {
#if defined(TXQ_MUX)
	struct pktq     psq;            /* PS suppression queue */
#else
	struct pktq     psq;            /* PS-defer queue */
	bool            psp_pending;    /* whether uncompleted PS-POLL response is pending */
#endif /* TXQ_MUX */
	uint8           psp_flags;      /* various ps mode bit flags defined below */
	bool            first_suppr_handled;    /* have we handled first supr'd frames ? */
	bool            in_pvb;         /* This STA already set in partial virtual bitmap */
	bool            apsd_usp;       /* APSD Unscheduled Service Period in progress */
	int             apsd_cnt;       /* Frames left in this service period */
	mbool           tx_block;       /* tx has been blocked */
#if !defined(TXQ_MUX)
	uint32          ps_discard;     /* cnt of PS pkts which were dropped */
	uint32		ps_queued;      /* cnt of PS pkts which were queued */
#endif
	bool            ext_qos_null;   /* Send extra QoS NULL frame to indicate EOSP
	                                 * if last frame sent in SP was MMPDU
	                                 */
	uint8           ps_trans_status; /* PS transition status flag */
#ifdef PROP_TXSTATUS
	bool            apsd_hpkt_timer_on;
	struct wl_timer *apsd_hpkt_timer;
	bool            apsd_tx_pending;
	struct scb      *scb;
	wlc_info_t      *wlc;
	int             in_transit;
#endif
	uint16		tbtt;		/**< count of tbtt intervals since last ageing event */
	uint16		listen;		/**< minimum # bcn's to buffer PS traffic */
	uint16		auxpmq_idx;
};

typedef struct apps_scb_psinfo apps_scb_psinfo_t;

typedef struct apps_bss_info
{
	char		pvb[251];		/* full partial virtual bitmap */
	uint16		aid_lo;			/* lowest aid with traffic pending */
	uint16		aid_hi;			/* highest aid with traffic pending */
#if !defined(TXQ_MUX)
	uint		ps_deferred;		/* cnt of all PS pkts buffered on unicast scbs */
#endif /* TXQ_MUX */
	uint32		ps_nodes;		/* num of STAs in PS-mode */
	uint8		ps_trans_status;	/* PS transition status flag */
#if defined(WLCNT)
	uint32		bcmc_pkts_seen;		/* Packets thru BC/MC SCB queue */
	uint32		bcmc_discards;		/* Packets discarded due to full queue */
#endif /* WLCNT */
} apps_bss_info_t;

struct apps_wlc_psinfo
{
	int		cfgh;			/* bsscfg cubby handle */
	osl_t		*osh;			/* pointer to os handle */
#if !defined(TXQ_MUX)
	uint		ps_deferred;		/* cnt of all PS pkts buffered on unicast scbs */
	uint32		ps_discard;		/* cnt of all PS pkts which were dropped */
	uint32		ps_aged;		/* cnt of all aged PS pkts */

	uint		psq_pkts_lo;		/* # ps pkts are always enq'able on scb */
	uint		psq_pkts_hi;		/* max # of ps pkts enq'able on a single scb */
#endif /* TXQ_MUX */
	int		scb_handle;		/* scb cubby handle to retrieve data from scb */
	uint32		ps_nodes_all;		/* Count of nodes in PS across all BBSes */
	uint8		cubby_name_id;		/* cubby ID (used by WL_DATAPATH_LOG_DUMP) */
};

/* AC bitmap to precedence bitmap mapping (constructed in wlc_attach) */
static uint wlc_acbitmap2precbitmap[16] = { 0 };

/* Map AC bitmap to precedence bitmap */
#define WLC_ACBITMAP_TO_PRECBITMAP(ab)  wlc_acbitmap2precbitmap[(ab) & 0xf]

#define SCB_PSINFO_LOC(psinfo, scb) ((apps_scb_psinfo_t **)SCB_CUBBY(scb, (psinfo)->scb_handle))
#define SCB_PSINFO(psinfo, scb) (*SCB_PSINFO_LOC(psinfo, scb))

/* apps info accessor */
#define APPS_BSSCFG_CUBBY_LOC(psinfo, cfg) ((apps_bss_info_t **)BSSCFG_CUBBY((cfg), (psinfo)->cfgh))
#define APPS_BSSCFG_CUBBY(psinfo, cfg) (*(APPS_BSSCFG_CUBBY_LOC(psinfo, cfg)))

#define APPS_CUBBY_CONFIG_SIZE	sizeof(apps_bss_info_t)

#define BSS_PS_NODES(psinfo, bsscfg) ((APPS_BSSCFG_CUBBY(psinfo, bsscfg))->ps_nodes)

static uint wlc_apps_ac2precbmp_info(uint8);
static int wlc_apps_scb_psinfo_init(void *context, struct scb *scb);
static void wlc_apps_scb_psinfo_deinit(void *context, struct scb *scb);
static uint wlc_apps_scb_psinfo_secsz(void *context, struct scb *scb);
#if !defined(TXQ_MUX)
static uint wlc_apps_txpktcnt(void *context);
#endif
#ifdef PROP_TXSTATUS
static void wlc_apps_apsd_hpkt_tmout(void *arg);
#endif

static void wlc_apps_apsd_complete(wlc_info_t *wlc, void *pkt, uint txs);
static void wlc_apps_psp_resp_complete(wlc_info_t *wlc, void *pkt, uint txs);

/* Accessor Function */
static uint BCMRAMFN(wlc_apps_ac2precbmp_info)(uint8 inf)
{
	return (WLC_ACBITMAP_TO_PRECBITMAP(inf));
}

#if !defined(TXQ_MUX)
static txmod_fns_t BCMATTACHDATA(apps_txmod_fns) = {
	wlc_apps_ps_enq,
	wlc_apps_txpktcnt,
	NULL,
	NULL
};
#else
static txmod_fns_t BCMATTACHDATA(apps_txmod_fns) = {
	wlc_apps_ps_enq,
	NULL,
	NULL,
	NULL
};
#endif /* TXQ_MUX */

#if defined(BCMDBG)
static void wlc_apps_scb_psinfo_dump(void *context, struct scb *scb, struct bcmstrbuf *b);
#else
/* Use NULL to pass as reference on init */
#define wlc_apps_scb_psinfo_dump NULL
#endif

#if defined(BCMDBG)
/* Limited dump routine for APPS SCB info */
static void
wlc_apps_scb_psinfo_dump(void *context, struct scb *scb, struct bcmstrbuf *b)
{
	wlc_info_t *wlc = (wlc_info_t *)context;
	struct apps_scb_psinfo *scb_psinfo;
	struct apps_bss_info *bss_info;
	wlc_bsscfg_t *bsscfg;

	if (scb == NULL)
		return;

	bsscfg = scb->bsscfg;
	if (bsscfg == NULL)
		return;

	if (!BSSCFG_AP(bsscfg))
		return;

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	if (scb_psinfo == NULL)
		return;

	bcm_bprintf(b, "     APPS psinfo on SCB %p is %p; scb-PS is %s; Listen is %d\n",
	            OSL_OBFUSCATE_BUF(scb), OSL_OBFUSCATE_BUF(scb_psinfo),
			SCB_PS(scb) ? "on" : "off", scb_psinfo->listen);

#if defined(TXQ_MUX)
	bcm_bprintf(b, "     tx_block %d ext_qos_null %d\n",
		scb_psinfo->tx_block, scb_psinfo->ext_qos_null);
#else
	bcm_bprintf(b, "     tx_block %d ext_qos_null %d psp_pending %d discards %d queued %d\n",
		scb_psinfo->tx_block, scb_psinfo->ext_qos_null, scb_psinfo->psp_pending,
		scb_psinfo->ps_discard, scb_psinfo->ps_queued);
#endif /* TXQ_MUX */

	if (SCB_ISMULTI(scb)) {
		bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, bsscfg);
		bcm_bprintf(b, "       SCB is multi. node count %d\n",
		            BSS_PS_NODES(wlc->psinfo, bsscfg));
		bcm_bprintf(b, "       BC/MC Seen %d Disc %d\n",
		            WLCNTVAL(bss_info->bcmc_pkts_seen), WLCNTVAL(bss_info->bcmc_discards));

	}

	{
		struct pktq *pktq;

		pktq = &scb_psinfo->psq;
		if (pktq == NULL) {
			bcm_bprintf(b, "       Packet queue is NULL\n");
			return;
		}

		bcm_bprintf(b, "       Pkt Q %p Que len %d Max %d Avail %d\n",
		            OSL_OBFUSCATE_BUF(pktq), pktq_n_pkts_tot(pktq), pktq_max(pktq),
		            pktq_avail(pktq));
	}

}
#endif /* BCMDBG */

#if defined(WL_DATAPATH_LOG_DUMP)
/**
 * Cubby datapath log callback fn
 * Use EVENT_LOG to dump a summary of the AMPDU datapath state
 * @param ctx   context pointer to wlc_info_t state structure
 * @param scb   scb of interest for the dump
 * @param tag   EVENT_LOG tag for output
 */
static void
wlc_apps_datapath_summary(void *ctx, struct scb *scb, int tag)
{
	int buf_size;
	uint prec;
	uint num_prec;
	uint32 flags;
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	struct apps_scb_psinfo *scb_psinfo;
	scb_subq_summary_t *sum;
	struct pktq *pktq;
	osl_t *osh;
	uint8 trans;

	osh = wlc->osh;

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	if (scb_psinfo == NULL) {
		return;
	}

	trans = scb_psinfo->ps_trans_status;

	pktq = &scb_psinfo->psq;

	/* skip summary if there are no packets and not in PS mode */
	if (pktq_n_pkts_tot(pktq) == 0 &&
	    !scb->PS &&
	    trans == 0) {
		return;
	}

	/*
	 * allcate a scb_subq_summary struct to dump ap ps information to the EVENT_LOG
	 */

	/* APPS works on the full prec count, but skip if no pkts */
	if (pktq_n_pkts_tot(pktq) > 0) {
		num_prec = WLC_PREC_COUNT;
	} else {
		num_prec = 0;
	}

	/* allocate a size large enough for queue info */
	buf_size = SCB_SUBQ_SUMMARY_FULL_LEN(num_prec);

	sum = MALLOCZ(osh, buf_size);
	if (sum == NULL) {
		EVENT_LOG(tag,
		          "wlc_apps_datapath_summary(): MALLOC %d failed, malloced %d bytes\n",
		          buf_size, MALLOCED(osh));
		/* nothing to do if event cannot be allocated */
		return;
	}

	/* Flags to contain interesting PS state */
	flags = 0;
	flags |= (scb->PS)                        ? SCBDATA_APPS_F_PS            : 0;
	flags |= (scb_psinfo->in_pvb)             ? SCBDATA_APPS_F_INPVB         : 0;
	flags |= (scb_psinfo->apsd_usp)           ? SCBDATA_APPS_F_APSD_USP      : 0;
	flags |= (scb_psinfo->tx_block)           ? SCBDATA_APPS_F_TXBLOCK       : 0;
#ifdef PROP_TXSTATUS
	flags |= (scb_psinfo->apsd_hpkt_timer_on) ? SCBDATA_APPS_F_APSD_HPKT_TMR : 0;
	flags |= (scb_psinfo->apsd_tx_pending)    ? SCBDATA_APPS_F_APSD_TX_PEND  : 0;
	flags |= (scb_psinfo->in_transit)         ? SCBDATA_APPS_F_INTRANS       : 0;
#endif
	flags |= (trans & SCB_PS_TRANS_OFF_PEND)    ? SCBDATA_APPS_F_OFF_PEND    : 0;
	flags |= (trans & SCB_PS_TRANS_OFF_BLOCKED) ? SCBDATA_APPS_F_OFF_BLOCKED : 0;
	flags |= (trans & SCB_PS_TRANS_OFF_IN_PROG) ? SCBDATA_APPS_F_OFF_IN_PROG : 0;

	/* Report the multi-prec queue summary */
	sum->id = EVENT_LOG_XTLV_ID_SCBDATA_SUM;
	sum->len = SCB_SUBQ_SUMMARY_FULL_LEN(num_prec) - BCM_XTLV_HDR_SIZE;
	sum->flags = flags;
	sum->cubby_id = wlc->psinfo->cubby_name_id;
	sum->prec_count = num_prec;
	for (prec = 0; prec < num_prec; prec++) {
		sum->plen[prec] = pktqprec_n_pkts(pktq, prec);
	}

	EVENT_LOG_BUFFER(tag, (uint8*)sum, sum->len + BCM_XTLV_HDR_SIZE);

	MFREE(osh, sum, buf_size);
}
#endif /* WL_DATAPATH_LOG_DUMP */

int
BCMATTACHFN(wlc_apps_attach)(wlc_info_t *wlc)
{
	scb_cubby_params_t cubby_params;
	apps_wlc_psinfo_t *wlc_psinfo;
	bsscfg_cubby_params_t bsscfg_cubby_params;
	int i;

	if (!(wlc_psinfo = MALLOCZ(wlc->osh, sizeof(apps_wlc_psinfo_t)))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		wlc->psinfo = NULL;
		return -1;
	}

	/* reserve cubby space in the bsscfg container for per-bsscfg private data */
	bzero(&bsscfg_cubby_params, sizeof(bsscfg_cubby_params));
	bsscfg_cubby_params.context = wlc_psinfo;
	bsscfg_cubby_params.fn_deinit = wlc_apps_bss_deinit;
	bsscfg_cubby_params.fn_init = wlc_apps_bss_init;
	bsscfg_cubby_params.fn_get = wlc_apps_cubby_get;
	bsscfg_cubby_params.fn_set = wlc_apps_cubby_set;
	bsscfg_cubby_params.config_size = APPS_CUBBY_CONFIG_SIZE;

	if ((wlc_psinfo->cfgh = wlc_bsscfg_cubby_reserve_ext(wlc, sizeof(apps_bss_info_t *),
	                                                      &bsscfg_cubby_params)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		return -1;
	}

#if !defined(TXQ_MUX)
	/* Set the psq watermarks */
	wlc_psinfo->psq_pkts_lo = PSQ_PKTS_LO;
	wlc_psinfo->psq_pkts_hi = PSQ_PKTS_HI;
#endif /* TXQ_MUX */

#if !defined(TXQ_MUX)
	/* calculate the total ps pkts required */
	wlc->pub->psq_pkts_total = wlc_psinfo->psq_pkts_hi +
	        (wlc->pub->tunables->maxscb * wlc_psinfo->psq_pkts_lo);
#else
	/* placeholder: TXQ_MUX does not need to add this to Q lens */
	wlc->pub->psq_pkts_total = 0;
	/* Perf impact: psq_pkts_totoal is added to all txq lens. For AP builds this means all txqs
	 * without TXQ_MUX are long (500 + 128 * 5) = 1140. Making the txq len shorter may result
	 * in more queue overflow drops for queues that still use the limit. TXQ_MUX does not limit
	 * in general, but for DONGLEBUILD the limit was kept because of a UTF failure (out of
	 * packets condition would happen)
	 */
#endif /* TXQ_MUX */

	/* reserve cubby in the scb container for per-scb private data */
	bzero(&cubby_params, sizeof(cubby_params));

	cubby_params.context = wlc;
	cubby_params.fn_init = wlc_apps_scb_psinfo_init;
	cubby_params.fn_deinit = wlc_apps_scb_psinfo_deinit;
	cubby_params.fn_secsz = wlc_apps_scb_psinfo_secsz;
	cubby_params.fn_dump = wlc_apps_scb_psinfo_dump;
#if defined(WL_DATAPATH_LOG_DUMP)
	cubby_params.fn_data_log_dump = wlc_apps_datapath_summary;
#endif
	cubby_params.fn_update = wlc_apps_scb_update;

	wlc_psinfo->scb_handle = wlc_scb_cubby_reserve_ext(wlc,
	                                                   sizeof(apps_scb_psinfo_t *),
	                                                   &cubby_params);

	if (wlc_psinfo->scb_handle < 0) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		wlc_apps_detach(wlc);
		return -2;
	}

	/* construct mapping from AC bitmap to precedence bitmap */
	for (i = 0; i < 16; i++) {
		wlc_acbitmap2precbitmap[i] = 0;
		if (AC_BITMAP_TST(i, AC_BE))
			wlc_acbitmap2precbitmap[i] |= WLC_PREC_BMP_AC_BE;
		if (AC_BITMAP_TST(i, AC_BK))
			wlc_acbitmap2precbitmap[i] |= WLC_PREC_BMP_AC_BK;
		if (AC_BITMAP_TST(i, AC_VI))
			wlc_acbitmap2precbitmap[i] |= WLC_PREC_BMP_AC_VI;
		if (AC_BITMAP_TST(i, AC_VO))
			wlc_acbitmap2precbitmap[i] |= WLC_PREC_BMP_AC_VO;
	}

	/* register module */
	if (wlc_module_register(wlc->pub, NULL, "apps", wlc, NULL, NULL, NULL, wlc_apps_down)) {
		WL_ERROR(("wl%d: %s wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		return -4;
	}

	/* register packet class callback */
	if (wlc_pcb_fn_set(wlc->pcb, 1, WLF2_PCB2_APSD, wlc_apps_apsd_complete) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_pcb_fn_set() failed\n", wlc->pub->unit, __FUNCTION__));
		return -5;
	}
	if (wlc_pcb_fn_set(wlc->pcb, 1, WLF2_PCB2_PSP_RSP, wlc_apps_psp_resp_complete) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_pcb_fn_set() failed\n", wlc->pub->unit, __FUNCTION__));
		return -6;
	}

	/* register IE mgmt callbacks */
	/* bcn */
	if (wlc_iem_add_build_fn(wlc->iemi, FC_BEACON, DOT11_MNG_TIM_ID,
			wlc_apps_calc_tim_ie_len, wlc_apps_write_tim_ie, wlc) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, tim in bcn\n",
		          wlc->pub->unit, __FUNCTION__));
		return -7;
	}

#if defined(PSPRETEND) && !defined(PSPRETEND_DISABLED)
	if ((wlc->pps_info = wlc_pspretend_attach(wlc)) == NULL) {
		WL_ERROR(("wl%d: %s: wlc_pspretend_attach failed\n",
		          wlc->pub->unit, __FUNCTION__));
		return -8;
	}
	wlc->pub->_pspretend = TRUE;
#endif /* PSPRETEND  && !PSPRETEND_DISABLED */
	wlc_txmod_fn_register(wlc->txmodi, TXMOD_APPS, wlc, apps_txmod_fns);

	/* Add client callback to the scb state notification list */
	if (wlc_scb_state_upd_register(wlc, wlc_apps_scb_state_upd_cb, wlc) != BCME_OK) {
		WL_ERROR(("wl%d: %s: unable to register callback %p\n",
		          wlc->pub->unit, __FUNCTION__,
			OSL_OBFUSCATE_BUF(wlc_apps_scb_state_upd_cb)));
		return -9;
	}

#if defined(WL_DATAPATH_LOG_DUMP)
	wlc_psinfo->cubby_name_id = wlc_scb_cubby_name_register(wlc->scbstate, "APPS");
	if (wlc_psinfo->cubby_name_id == WLC_SCB_NAME_ID_INVALID) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_name_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		return -10;
	}
#endif /* WL_DATAPATH_LOG_DUMP */

	wlc_psinfo->osh = wlc->osh;
	wlc->psinfo = wlc_psinfo;

	return 0;
}

void
BCMATTACHFN(wlc_apps_detach)(wlc_info_t *wlc)
{
	apps_wlc_psinfo_t *wlc_psinfo;

	ASSERT(wlc);

#ifdef PSPRETEND
	wlc_pspretend_detach(wlc->pps_info);
#endif /* PSPRETEND */

	wlc_psinfo = wlc->psinfo;

	if (!wlc_psinfo)
		return;

#if !defined(TXQ_MUX)
	/* All PS packets shall have been flushed */
	ASSERT(wlc_psinfo->ps_deferred == 0);
#endif /* TXQ_MUX */

	wlc_scb_state_upd_unregister(wlc, wlc_apps_scb_state_upd_cb, wlc);

	wlc_module_unregister(wlc->pub, "apps", wlc);

	MFREE(wlc->osh, wlc_psinfo, sizeof(apps_wlc_psinfo_t));
	wlc->psinfo = NULL;
}

static int
wlc_apps_down(void *hdl)
{
	wlc_info_t *wlc = (wlc_info_t *)hdl;
	struct apps_scb_psinfo *scb_psinfo;
	struct scb_iter scbiter;
	struct scb *tscb;

	FOREACHSCB(wlc->scbstate, &scbiter, tscb) {
		scb_psinfo = SCB_PSINFO(wlc->psinfo, tscb);

		if (scb_psinfo == NULL)
			continue;

		if (!pktq_empty(&scb_psinfo->psq))
			wlc_apps_ps_flush(wlc, tscb);
	}
	if (MBSS_ENAB(wlc->pub)) {
		int i;
		wlc_bsscfg_t *cfg;

		FOREACH_BSS(wlc, i, cfg) {
			if (wlc->bsscfg[i]->bcmc_scb)
				wlc_apps_ps_flush(wlc, wlc->bsscfg[i]->bcmc_scb);
		}
	}

	return 0;
}

/* bsscfg cubby copy */
static int
wlc_apps_cubby_get(void *ctx, wlc_bsscfg_t *cfg, uint8 *data, int *len)
{
	apps_wlc_psinfo_t *wlc_psinfo = (apps_wlc_psinfo_t *)ctx;
	apps_bss_info_t *apps_bss = NULL;
	ASSERT(cfg != NULL);
	ASSERT(data != NULL);
	if (!BSSCFG_HAS_PSINFO(cfg)) {
		return BCME_OK;
	}
	apps_bss = APPS_BSSCFG_CUBBY(wlc_psinfo, cfg);
	ASSERT(apps_bss != NULL);
	memcpy(data, (uint8*)apps_bss, sizeof(*apps_bss));
	*len = sizeof(*apps_bss);
	return BCME_OK;
}

/* bsscfg cubby copy */
static int
wlc_apps_cubby_set(void *ctx, wlc_bsscfg_t *cfg, const uint8 *data, int len)
{
	apps_wlc_psinfo_t *wlc_psinfo = (apps_wlc_psinfo_t *)ctx;
	apps_bss_info_t *apps_bss = NULL;
	ASSERT(cfg != NULL);
	ASSERT(data != NULL);
	if (!BSSCFG_HAS_PSINFO(cfg)) {
		return BCME_OK;
	}
	apps_bss = APPS_BSSCFG_CUBBY(wlc_psinfo, cfg);
	ASSERT(apps_bss != NULL);
	memcpy((uint8*)apps_bss, data, len);
	/* Increment the global count for all the ps nodes.
	* This function is called when the old bsscfg on old wlc
	* is being moved to a new bsscfg on new wlc. Therefore,
	* the count of ps nodes maintained at wlc level needs to
	* be updated.
	*/
	wlc_psinfo->ps_nodes_all += apps_bss->ps_nodes;
	return BCME_OK;
}

/* bsscfg cubby */
static int
wlc_apps_bss_init(void *ctx, wlc_bsscfg_t *cfg)
{
	apps_wlc_psinfo_t *wlc_psinfo = (apps_wlc_psinfo_t *)ctx;
	wlc_info_t *wlc = cfg->wlc;
	apps_bss_info_t **papps_bss = APPS_BSSCFG_CUBBY_LOC(wlc_psinfo, cfg);
	apps_bss_info_t *apps_bss = NULL;
	UNUSED_PARAMETER(wlc);

	/* Allocate only for AP || TDLS || AIBSS bsscfg */
	if (BSSCFG_HAS_PSINFO(cfg)) {
		if (!(apps_bss = (apps_bss_info_t *)MALLOCZ(wlc_psinfo->osh,
			sizeof(apps_bss_info_t)))) {
			WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
			          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc_psinfo->osh)));
			return BCME_NOMEM;
		}
	}
	*papps_bss = apps_bss;

	return BCME_OK;
}

static void
wlc_apps_bss_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
	apps_wlc_psinfo_t *wlc_psinfo = (apps_wlc_psinfo_t *)ctx;
	apps_bss_info_t **papps_bss = APPS_BSSCFG_CUBBY_LOC(wlc_psinfo, cfg);
	apps_bss_info_t *apps_bss = *papps_bss;

	if (apps_bss != NULL) {
		MFREE(wlc_psinfo->osh, apps_bss, sizeof(apps_bss_info_t));
		*papps_bss = NULL;
	}

	return;
}

#if !defined(TXQ_MUX)
/* TXQ_MUX does not store pkts by way of the TxModule */

/* Return the count of all the packets being held by APPS TxModule */
static uint
wlc_apps_txpktcnt(void *context)
{
	wlc_info_t *wlc = (wlc_info_t *)context;
	apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;

	return (wlc_psinfo->ps_deferred);
}
#endif /* TXQ_MUX */

/* Return the count of all the packets being held in scb psq */
uint
wlc_apps_scb_txpktcnt(wlc_info_t *wlc, struct scb *scb)
{
	struct apps_scb_psinfo *scb_psinfo;

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

	if (!scb_psinfo)
		return 0;

	return (pktq_n_pkts_tot(&scb_psinfo->psq));
}

static int
wlc_apps_scb_update(void *context, struct scb *scb, wlc_bsscfg_t* new_cfg)
{
#ifdef PROP_TXSTATUS
	wlc_info_t *wlc = (wlc_info_t *)context;
	wlc_info_t *new_wlc = new_cfg->wlc;
	apps_scb_psinfo_t *cubby_info;

	ASSERT(scb && new_cfg);
	cubby_info = SCB_PSINFO(wlc->psinfo, scb);
	if (cubby_info) {
		cubby_info->wlc = new_wlc;
	}
#endif /* PROP_TXSTATUS */
	return BCME_OK;
}

static int
wlc_apps_scb_psinfo_init(void *context, struct scb *scb)
{
	wlc_info_t *wlc = (wlc_info_t *)context;
	apps_scb_psinfo_t **cubby_info;
	wlc_bsscfg_t *bsscfg;

	cubby_info = SCB_PSINFO_LOC(wlc->psinfo, scb);
	ASSERT(*cubby_info == NULL);

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);

	if (BSSCFG_HAS_PSINFO(bsscfg)) {
		*cubby_info = wlc_scb_sec_cubby_alloc(wlc, scb, sizeof(apps_scb_psinfo_t));
		if (*cubby_info == NULL)
			return BCME_NOMEM;

#ifdef PROP_TXSTATUS
		if (!((*cubby_info)->apsd_hpkt_timer = wl_init_timer(wlc->wl,
			wlc_apps_apsd_hpkt_tmout, *cubby_info, "appsapsdhkpt"))) {
			WL_ERROR(("wl: apsd_hpkt_timer failed\n"));
			return 1;
		}
		(*cubby_info)->wlc = wlc;
		(*cubby_info)->scb = scb;
#endif
		/* PS state init */
		pktq_init(&(*cubby_info)->psq, WLC_PREC_COUNT, PSQ_PKTQ_LEN_DEFAULT);

		(*cubby_info)->auxpmq_idx = AUXPMQ_INVALID_IDX;
	}
	return 0;
}

static void
wlc_apps_scb_psinfo_deinit(void *context, struct scb *remove)
{
	wlc_info_t *wlc = (wlc_info_t *)context;
	apps_scb_psinfo_t **cubby_info = SCB_PSINFO_LOC(wlc->psinfo, remove);

	if (*cubby_info) {
#ifdef PROP_TXSTATUS
		wl_del_timer(wlc->wl, (*cubby_info)->apsd_hpkt_timer);
		(*cubby_info)->apsd_hpkt_timer_on = FALSE;
		wl_free_timer(wlc->wl, (*cubby_info)->apsd_hpkt_timer);
#endif

		if (AP_ENAB(wlc->pub) || (BSS_TDLS_ENAB(wlc, SCB_BSSCFG(remove)) &&
				SCB_PS(remove))) {
			uint8 ps = (wlc->block_datafifo & DATA_BLOCK_PS) ?
				PS_SWITCH_OFF : PS_SWITCH_FIFO_FLUSHED;
			if (!SCB_ISMULTI(remove)) {
				struct apps_bss_info *bss_info =
					APPS_BSSCFG_CUBBY(wlc->psinfo, SCB_BSSCFG(remove));
				if (bss_info &&
					(bss_info->ps_trans_status & BSS_PS_TRANS_OFF_BLOCKED)) {
					WL_ERROR(("wl%d.%d: "MACF" deinited."
						" bcmc_scb PS off blocked. PS %d\n",
						wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(remove)),
						ETHER_TO_MACF(remove->ea), SCB_PS(remove)));
				}
			}
			if (SCB_PS(remove) && !SCB_ISMULTI(remove))
				wlc_apps_scb_ps_off(wlc, remove, TRUE);
			else if (!pktq_empty(&(*cubby_info)->psq))
				wlc_apps_ps_flush(wlc, remove);
			wlc_bmac_process_ps_switch(wlc->hw, &remove->ea, ps |
				PS_SWITCH_STA_REMOVED,
				&((*cubby_info)->auxpmq_idx));
		}

		wlc_scb_sec_cubby_free(wlc, remove, *cubby_info);
		*cubby_info = NULL;
	}
}

static uint
wlc_apps_scb_psinfo_secsz(void *context, struct scb *scb)
{
	wlc_bsscfg_t *cfg;
	BCM_REFERENCE(context);

	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg != NULL);

	if (BSSCFG_HAS_PSINFO(cfg)) {
		return sizeof(apps_scb_psinfo_t);
	}

	return 0;
}

void
wlc_apps_dbg_dump(wlc_info_t *wlc, int hi, int lo)
{
	struct scb *scb;
	struct scb_iter scbiter;
	struct apps_bss_info *bss_info;
	struct apps_scb_psinfo *scb_psinfo;
	char* pvb;

#if !defined(TXQ_MUX)
	printf("WLC discards:%d, ps_deferred:%d\n",
		wlc->psinfo->ps_discard,
	        wlc->psinfo->ps_deferred);
#endif /* TXQ_MUX */

	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
		if (scb_psinfo == NULL)
			continue;

		printf("scb at %p for [%02x:%02x:%02x:%02x:%02x:%02x]\n",
			OSL_OBFUSCATE_BUF(scb),
			scb->ea.octet[0], scb->ea.octet[1], scb->ea.octet[2],
		        scb->ea.octet[3], scb->ea.octet[4], scb->ea.octet[5]);
		printf("  (psq_items,state)=(%d,%s), psq_bucket: %d items\n",
			scb_psinfo->psq.n_pkts_tot,
			((scb->PS == FALSE) ? " OPEN" : "CLOSE"),
			scb_psinfo->psq.n_pkts_tot);

		bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, scb->bsscfg);

		pvb = bss_info->pvb;
		printf("pvb: [%02x-%02x-%02x-%02x]\n", pvb[0], pvb[1], pvb[2], pvb[3]);
	}

#if defined(TXQ_MUX)
	UNUSED_PARAMETER(hi);
	UNUSED_PARAMETER(lo);
#else
	if (hi) {
		printf("setting psq (hi,lo) to (%d,%d)\n", hi, lo);
		wlc->psinfo->psq_pkts_hi = hi;
		wlc->psinfo->psq_pkts_lo = lo;
	} else {
		printf("leaving psq (hi,lo) as is (%d,%d)\n",
		       wlc->psinfo->psq_pkts_hi, wlc->psinfo->psq_pkts_lo);
	}
#endif /* TXQ_MUX */

	return;
}

static void
wlc_apps_apsd_usp_end(wlc_info_t *wlc, struct scb *scb)
{
	struct apps_scb_psinfo *scb_psinfo;

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

#ifdef WLTDLS
	if (BSS_TDLS_ENAB(wlc, scb->bsscfg)) {
		if (wlc_tdls_in_pti_interval(wlc->tdls, scb))
			return;
		wlc_tdls_apsd_usp_end(wlc->tdls, scb);
	}
#endif /* WLTDLS */

	scb_psinfo->apsd_usp = FALSE;

#ifdef WLTDLS
	if (BSS_TDLS_ENAB(wlc, scb->bsscfg) &&
		(wlc_apps_apsd_delv_count(wlc, scb) > 0)) {
		/* send PTI again */
		wlc_tdls_send_pti(wlc->tdls, scb);
	}
#endif /* WLTDLS */

}


/* This routine deals with all PS transitions from ON->OFF */
void
wlc_apps_scb_ps_off(wlc_info_t *wlc, struct scb *scb, bool discard)
{
	apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
	struct apps_scb_psinfo *scb_psinfo;
	struct scb *bcmc_scb;
	apps_bss_info_t *bss_info;
	wlc_bsscfg_t *bsscfg;
	struct ether_addr ea;

	/* sanity */
	ASSERT(scb);

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);

	bss_info = APPS_BSSCFG_CUBBY(wlc_psinfo, bsscfg);
	ASSERT(bss_info && bss_info->ps_nodes);

	/* process ON -> OFF PS transition */
	WL_PS(("wl%d.%d: wlc_apps_scb_ps_off, "MACF" aid %d\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
		ETHER_TO_MACF(scb->ea), AID2PVBMAP(scb->aid)));

#if defined(BCMDBG) && defined(PSPRETEND)
	if (SCB_PS_PRETEND(scb)) {
		wlc_pspretend_scb_time_upd(wlc->pps_info, scb);
	}
#endif /* PSPRETEND */

	/* update PS state info */
	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
	ASSERT(scb_psinfo);

	/* if already in this process but came here due to pkt callback then
	 * just return.
	 */
	if ((scb_psinfo->ps_trans_status & SCB_PS_TRANS_OFF_IN_PROG))
		return;
	scb_psinfo->ps_trans_status |= SCB_PS_TRANS_OFF_IN_PROG;

	bss_info->ps_nodes--;
	wlc_psinfo->ps_nodes_all--;
	scb->PS = FALSE;
#ifdef PSPRETEND
	if (wlc->pps_info != NULL) {
		wlc_pspretend_scb_ps_off(wlc->pps_info, scb);
	}
#endif /* PSPRETEND */
#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
		wlc_wlfc_scb_ps_off(wlc, scb);
	}
#endif /* PROP_TXSTATUS */

	/* Unconfigure the APPS from the txpath */
	wlc_txmod_unconfig(wlc->txmodi, scb, TXMOD_APPS);
#if defined(TXQ_MUX)
	WL_TMP(("TXQ: %s unconfig PS output for "MACF"\n", __FUNCTION__, ETHER_TO_MACF(scb->ea)));

	/* The scb can go back to non-PS delivery, unconfigure the PS output fn */
	wlc_txoutput_unconfig(wlc->tx_scbq, scb, TXOUT_APPS);

	WL_TMP(("TXQ: %s clearing SCBQ_FC_PS\n", __FUNCTION__));

	/* Clear the PS Stop reason for the SCBQ */
	wlc_scbq_scb_stop_flag_clear(wlc->tx_scbq, scb, SCBQ_FC_PS);
#endif /* TXQ_MUX */

	/* If this is last STA to leave PS mode,
	 * trigger BCMC FIFO drain and
	 * set BCMC traffic to go through regular fifo
	 */
	if (bss_info->ps_nodes == 0 && !(bsscfg->flags & WLC_BSSCFG_NOBCMC) &&
		BSSCFG_HAS_BCMC_SCB(bsscfg) &&
		!(bss_info->ps_trans_status & BSS_PS_TRANS_OFF_BLOCKED)) {
		WL_PS(("wl%d.%d: wlc_apps_scb_ps_off - bcmc off\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
		bcmc_scb = WLC_BCMCSCB_GET(wlc, bsscfg);

		if (MBSS_ENAB(wlc->pub)) { /* MBSS PS handling is a bit more complicated. */
			wlc_apps_bss_ps_off_start(wlc, bcmc_scb);
		} else
#if defined(WLAIBSS) && !defined(WLAIBSS_DISABLED)
		if (!BSSCFG_IBSS(bsscfg) || !AIBSS_ENAB(wlc->pub))
#endif /* WLAIBSS && !WLAIBSS_DISABLED */
		{
			bcmc_scb->PS = FALSE;
#ifdef PSPRETEND
			// Does PSPretend really apply to bcmc?
			bcmc_scb->ps_pretend &= ~PS_PRETEND_ON;
#endif /* PSPRETEND */
			/* If packets are pending in TX_BCMC_FIFO,
			 * then ask ucode to transmit them immediately
			 */
			if (!wlc->bcmcfifo_drain && TXPKTPENDGET(wlc, TX_BCMC_FIFO)) {
				wlc->bcmcfifo_drain = TRUE;
				wlc_mhf(wlc, MHF2, MHF2_TXBCMC_NOW, MHF2_TXBCMC_NOW, WLC_BAND_AUTO);
			}
#if defined(TXQ_MUX)
			WL_TMP(("TXQ: %s config PS-off output for BCMC\n", __FUNCTION__));

			/* reconfigure the bcmc MUXs for current PS mode */
			wlc_bcmc_set_powersave(bsscfg, FALSE);

#endif /* TXQ_MUX */
		}
	} else if ((bss_info->ps_trans_status & BSS_PS_TRANS_OFF_BLOCKED)) {
		WL_PS(("wl%d.%d: wlc_apps_scb_ps_off - bcmc off BLOCKED! %d\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), bss_info->ps_nodes));
	}

#if !defined(TXQ_MUX)
	scb_psinfo->psp_pending = FALSE;
#endif
	scb_psinfo->first_suppr_handled = FALSE;
	wlc_apps_apsd_usp_end(wlc, scb);
	scb_psinfo->apsd_cnt = 0;

	/* save ea before calling wlc_apps_ps_flush */
	ea = scb->ea;

	/* Note: We do not clear up any pending PS-POLL pkts
	 * which may be enq'd with the IGNOREPMQ bit set. The
	 * relevant STA should stay awake until it rx's these
	 * response pkts
	 */

#if defined(TXQ_MUX)
	if (discard == FALSE) {
		/* The PS Q should be empty since we only come to an ON->OFF transition after
		 * completing the OFF->ON transition and pushing the PS Q suppressed pkts
		 * onto the SCBQ.
		 */
		ASSERT(pktq_n_pkts_tot(&scb_psinfo->psq) == 0);
	} else {
		/* free any pending frames */
		wlc_apps_ps_flush(wlc, scb);
	}
#else
	/* Move pmq entries to Q1 (ctl) for immediate tx */
	if (discard == FALSE)
		while (wlc_apps_ps_send(wlc, scb, WLC_PREC_BMP_ALL, 0))
			;
	else /* free any pending frames */
		wlc_apps_ps_flush(wlc, scb);
#endif /* TXQ_MUX */

	/* clear the PVB entry since we are leaving PM mode */
	if (scb_psinfo->in_pvb) {
		wlc_apps_pvb_update(wlc, scb);
	}

	/* callbacks in wlc_apps_ps_flush may have freed scb */
	if (!ETHER_ISMULTI(&ea) && (wlc_scbfind(wlc, bsscfg, &ea) == NULL)) {
		WL_PS(("wl%d: %s: exiting, scb for "MACF" was freed\n",
			wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea)));
		return;
	}
	scb_psinfo->ps_trans_status &= ~SCB_PS_TRANS_OFF_IN_PROG;
#ifdef WL_BEAMFORMING
	if (TXBF_ENAB(wlc->pub)) {
		/* Notify txbf module of the scb's PS change */
		wlc_txbf_scb_ps_notify(wlc->txbf, scb, FALSE);
	}
#endif /* WL_BEAMFORMING */

#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
		wlc_check_txq_fc(wlc, SCB_WLCIFP(scb)->qi);
#ifdef WLAMPDU
		if (AMPDU_ENAB(wlc->pub)) {
			wlc_check_ampdu_fc(wlc->ampdu_tx, scb);
		}
#endif /* WLAMPDU */
#ifdef BCMPCIEDEV
		if (BCMPCIEDEV_ENAB()) {
			scb_psinfo->in_transit = 0;
		}
#endif
	}
#endif /* PROP_TXSTATUS */
}


static void
wlc_apps_bcmc_scb_ps_on(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, bool pmq_suppr_bcmc_pkt)
{
	apps_bss_info_t *bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, bsscfg);
	struct scb *bcmc_scb;
	/*  Use the bsscfg pointer of this scb to help us locate the
	 *  correct bcmc_scb so that we can turn on PS
	 */
	bcmc_scb = WLC_BCMCSCB_GET(wlc, bsscfg);
	ASSERT(bcmc_scb->bsscfg == bsscfg);

	if (pmq_suppr_bcmc_pkt) {
		bss_info->ps_trans_status |= BSS_PS_TRANS_OFF_BLOCKED;
		WL_PS(("wl%d.%d: %s: Req by bcmc suppr pkt!\n", wlc->pub->unit,
			WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
	} else if (SCB_PS(bcmc_scb)) {
		WL_PS(("wl%d.%d: %s: [bcmc_scb] Already in PS!\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
		/* ps on had already been processed by bcmc pmq suppressed pkt,
		 * and now we are here upon successive unicast pmq suppressed pkt
		 * or actual pmq entry addition interrupt.
		 * Therefore do nothing here and just get out.
		 */
		return;
	}
#if defined(MBSS) && (defined(BCMDBG) || defined(WLMSG_PS))
	if (MBSS_ENAB(wlc->pub)) {
		uint32 mc_fifo_pkts = wlc_mbss_get_bcmc_pkts_sent(wlc, bsscfg);

		if (mc_fifo_pkts != 0) {
			WL_PS(("wl%d.%d: START PS-ON; bcmc %d\n", wlc->pub->unit,
				WLC_BSSCFG_IDX(bsscfg), mc_fifo_pkts));
		}
	}
#endif /* MBSS && (BCMDBG || WLMSG_PS) */
#if defined(WLAIBSS) && !defined(WLAIBSS_DISABLED)
	if (!BSSCFG_IBSS(bsscfg) || !AIBSS_ENAB(wlc->pub))
#endif /* WLAIBSS && WLAIBSS_DISABLED */
	{
		WL_PS(("wl%d.%d: %s: bcmc SCB PS on!\n", wlc->pub->unit,
			WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
		bcmc_scb->PS = TRUE;
		if (wlc->bcmcfifo_drain) {
			wlc->bcmcfifo_drain = FALSE;
			wlc_mhf(wlc, MHF2, MHF2_TXBCMC_NOW, 0, WLC_BAND_AUTO);
		}
#if defined(TXQ_MUX)
		/* handle the switch of BCMC mux sources based on outstanding pkts in the fifo */
		if (TXPKTPENDTOT(wlc) > 0) {
			WL_TMP(("TXQ: %s suspend output for BCMC\n", __FUNCTION__));

			/* Suspend bcmc delivery until flush complete and
			 * we reorder suppressed packets
			 */
			wlc_bcmc_stop_mux_sources(bsscfg);
		} else {
			WL_TMP(("TXQ: %s config PS-on output for BCMC\n", __FUNCTION__));

			/* reconfigure output right now since there are no outstanding pkts in the
			 * the lower queues, so no reorder of suppressed pkts.
			 */
			wlc_bcmc_set_powersave(bsscfg, TRUE);
		}
#endif /* TXQ_MUX */
	}
}

/* This deals with all PS transitions from OFF->ON */
void
wlc_apps_scb_ps_on(wlc_info_t *wlc, struct scb *scb, uint8 flags)
{
	struct apps_scb_psinfo *scb_psinfo;
	apps_wlc_psinfo_t *wlc_psinfo;
	apps_bss_info_t *bss_info;
	wlc_bsscfg_t *bsscfg;

	ASSERT(scb);

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);

	if (BSSCFG_STA(bsscfg) && !BSS_TDLS_ENAB(wlc, bsscfg) && !BSSCFG_IBSS(bsscfg)) {
		WL_PS(("wlc_apps_scb_ps_on, "MACF" aid %d: BSSCFG_STA(bsscfg)=%s, "
			"BSS_TDLS_ENAB(wlc,bsscfg)=%s\n\n",
			ETHER_TO_MACF(scb->ea), AID2PVBMAP(scb->aid),
			BSSCFG_STA(bsscfg) ? "TRUE" : "FALSE",
			BSS_TDLS_ENAB(wlc, bsscfg) ? "TRUE" : "FALSE"));
		return;
	}

	/* process OFF -> ON PS transition */
	wlc_psinfo = wlc->psinfo;
	bss_info = APPS_BSSCFG_CUBBY(wlc_psinfo, bsscfg);
	ASSERT(bss_info);

	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
	ASSERT(scb_psinfo);

	WL_PS(("wl%d.%d wlc_apps_scb_ps_on, "MACF" aid %d in_pvb %d fifo pkts pending %d\n",
	       wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), ETHER_TO_MACF(scb->ea),
	       AID2PVBMAP(scb->aid), scb_psinfo->in_pvb, TXPKTPENDTOT(wlc)));

	scb_psinfo->first_suppr_handled = FALSE;

	/* update PS state info */
	bss_info->ps_nodes++;
	wlc_psinfo->ps_nodes_all++;
	scb->PS = TRUE;
	scb_psinfo->tbtt = 0;
	scb_psinfo->ps_trans_status &= ~SCB_PS_TRANS_OFF_PEND;

#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
		wlc_wlfc_scb_ps_on(wlc, scb);
	}
#endif	/* PROP_TXSTATUS */

	/* If this is first STA to enter PS mode, set BCMC traffic
	 * to go through BCMC Fifo. If bcmcfifo_drain is set, then clear
	 * the drain bit.
	 */
	if (bss_info->ps_nodes == 1 && !(bsscfg->flags & WLC_BSSCFG_NOBCMC)) {
		wlc_apps_bcmc_scb_ps_on(wlc, SCB_BSSCFG(scb), FALSE);
	}

	/* Add the APPS to the txpath for this SCB */
	wlc_txmod_config(wlc->txmodi, scb, TXMOD_APPS);
#if defined(TXQ_MUX)
	WL_TMP(("TXQ: %s config PS output for "MACF"\n", __FUNCTION__, ETHER_TO_MACF(scb->ea)));

	/* The scb will use PS delivery, configure the PS output fn */
	wlc_txoutput_config(wlc->tx_scbq, scb, TXOUT_APPS);

	WL_TMP(("TXQ: %s setting SCBQ_FC_PS\n", __FUNCTION__));

	/* Stop the SCBQ until PSPoll or APSD delivery */
	wlc_scbq_scb_stop_flag_set(wlc->tx_scbq, scb, SCBQ_FC_PS);
#endif /* TXQ_MUX */

#ifdef PROP_TXSTATUS
	/* suppress tx fifo first */
	wlc_txfifo_suppress(wlc, scb); // Need to examine what this does for MUX impact.
#ifdef BCMPCIEDEV
	if (BCMPCIEDEV_ENAB()) {
		scb_psinfo->in_transit = 0;
	}
#endif
#endif /* BCMPCIEDEV */

#if !defined(TXQ_MUX) // No TxQ or AMPDU Q for TXQ_MUX, so no move to psq

	/* ps enQ any pkts on the txq, ampduq */
	wlc_apps_txq_to_psq(wlc, scb);

#if defined(WLAMPDU) && defined(PROP_TXSTATUS)
	/* This causes problems for PSPRETEND */
	wlc_apps_ampdu_txq_to_psq(wlc, scb);
#endif

#endif /* TXQ_MUX */

#if defined(WLAIBSS) && !defined(WLAIBSS_DISABLED)
	if (!BSSCFG_IBSS(bsscfg) || !AIBSS_ENAB(wlc->pub))
#endif /* WLAIBSS && !WLAIBSS_DISABLED */
	{
		/* If there is anything in the data fifo then allow it to drain */
		if (TXPKTPENDTOT(wlc) > 0) {
			wlc_block_datafifo(wlc, DATA_BLOCK_PS, DATA_BLOCK_PS);
		}
	}

#ifdef WL_BEAMFORMING
	if (TXBF_ENAB(wlc->pub)) {
		/* Notify txbf module of the scb's PS change */
		wlc_txbf_scb_ps_notify(wlc->txbf, scb, TRUE);
	}
#endif /* WL_BEAMFORMING */
}

#if !defined(TXQ_MUX)

/* TXQ_MUX Note:
 * The wlc_pktq_supr_norm() fn below combs out the Suppressed flagged pkts and moves to the head of
 * the queue. This deals with pkts from the top dropping in mixed in with pkts being suppressed.
 * TXQ_MUX changes the suppress queue to be only for suppressed pkts. The "normalization"
 * step would only be to fold the suppressed queue to the head of the scbq.
 * A new fn needed to do the move to the scbq, wlc_apps_scb_merge_suppress().
 */

/* "normalize" a packet queue - move packets tagged with WLF3_SUPR flag
 * to the front and retain the order in case other packets were inserted
 * in the queue before.
 */
static void
wlc_pktq_supr_norm(wlc_info_t *wlc, struct pktq *pktq)
{
	struct spktq scratch;
	void *pkt;
	int prec;
	BCM_REFERENCE(wlc);

	if (pktq_n_pkts_tot(pktq) == 0)
		return;

	spktqinit(&scratch, pktq_n_pkts_tot(pktq));

	PKTQ_PREC_ITER(pktq, prec) {
		void *head_pkt = pktqprec_peek(pktq, prec);

		while ((pkt = pktq_pdeq_tail(pktq, prec)) != NULL) {
			if (WLPKTTAG(pkt)->flags3 & WLF3_SUPR) {
				WLPKTTAG(pkt)->flags3 &= ~WLF3_SUPR;
				spktenq_head(&scratch, pkt);
			}
			else {
				pktq_penq_head(pktq, prec, pkt);
			}
			if (pkt == head_pkt)
				break;
		}

		if (spktq_n_pkts(&scratch) == 0)
			continue;

		while ((pkt = spktdeq_tail(&scratch)) != NULL) {
			pktq_penq_head(pktq, prec, pkt);
		}

		ASSERT(spktq_empty(&scratch));
	}

	spktq_deinit(&scratch);
}

/* "normalize" the SCB's PS queue - move packets tagged with WLF3_SUPR flag
 * to the front and retain the order in case other packets were inserted
 * in the queue before.
 */
void
wlc_apps_scb_psq_norm(wlc_info_t *wlc, struct scb *scb)
{
	apps_wlc_psinfo_t *wlc_psinfo;
	struct apps_scb_psinfo *scb_psinfo;
	struct pktq *pktq;

	wlc_psinfo = wlc->psinfo;
	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
	ASSERT(scb_psinfo);

	pktq = &scb_psinfo->psq;

	wlc_pktq_supr_norm(wlc, pktq);
}

/* "normalize" the BSS's txq queue - move packets tagged with WLF3_SUPR flag
 * to the front and retain the order in case other packets were inserted
 * in the queue before.
 */
static void
wlc_apps_bsscfg_txq_norm(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	struct pktq *txq = WLC_GET_TXQ(cfg->wlcif->qi);

	wlc_pktq_supr_norm(wlc, txq);
}

#else /* TXQ_MUX */

/* Merge the suppressed pkts from the SCB's ps suppress queue to the head
 * of the SCB's data queue, preserving the order.
 * Note: similar to the wlc_apps_scb_psq_norm()/wlc_apps_bsscfg_txq_norm()
 * stage of processing from non-TXQ_MUX
 */
static void
wlc_apps_scb_merge_suppress(wlc_info_t *wlc, struct scb *scb)
{
	apps_wlc_psinfo_t *wlc_psinfo;
	struct apps_scb_psinfo *scb_psinfo;
	struct pktq *suppr_q;
	struct pktq *dest_q;
	void *pkt;
	int prec;

	wlc_psinfo = wlc->psinfo;
	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);

	suppr_q = &scb_psinfo->psq;
	dest_q = wlc_scbq_txq(wlc->tx_scbq, scb);

	/* if the source queue is empty, nothing to do */
	if (pktq_n_pkts_tot(suppr_q) == 0) {
		return;
	}

	WL_NONE(("TXQ: %s moving %u pkts from scb->psq to scbq (%u pkts)\n",
	         __FUNCTION__, pktq_n_pkts_tot(suppr_q), pktq_n_pkts_tot(dest_q)));

	/* move each prec q of pkts from suppr_q, in order,
	 * to the head of the SCBQ's pktq.
	 */
	PKTQ_PREC_ITER(suppr_q, prec) {
		while ((pkt = pktq_pdeq_tail(suppr_q, prec)) != NULL) {
			pktq_penq_head(dest_q, prec, pkt);
		}
	}

	/* the source q should be empty now */
	ASSERT(pktq_empty(suppr_q));

	return;
}

#endif /* TXQ_MUX */

/* Process any pending PS states */
void
wlc_apps_process_pend_ps(wlc_info_t *wlc)
{
	struct scb_iter scbiter;
	struct scb *scb;

	/* PMQ todo : we should keep track of pkt pending for each scb and wait for
	   individual drains, instead of blocking and draining the whole pipe.
	*/

	if ((wlc->block_datafifo & DATA_BLOCK_PS) && (TXPKTPENDTOT(wlc) == 0)) {
		int idx;
		wlc_bsscfg_t *cfg;
		WL_PS(("wlc_apps_process_pend_ps unblocking fifo\n"));
		wlc_block_datafifo(wlc, DATA_BLOCK_PS, 0);

		/* notify bmac to clear the PMQ */
		wlc_bmac_process_ps_switch(wlc->hw, NULL,
			(PS_SWITCH_FIFO_FLUSHED | PS_SWITCH_MAC_INVALID), NULL);

		FOREACH_BSS(wlc, idx, cfg) {
			scb = WLC_BCMCSCB_GET(wlc, cfg);
			if (scb && SCB_PS(scb)) {
#if !defined(TXQ_MUX)
				WL_PS(("wl%d.%d wlc_apps_process_pend_ps: Normalizing bcmc PSQ"
					" of BSSID "MACF"\n",
					wlc->pub->unit, idx, ETHER_TO_MACF(cfg->BSSID)));
				wlc_apps_bsscfg_txq_norm(wlc, cfg);
#else
				WL_PS(("wl%d.%d wlc_apps_process_pend_ps: Merging %u PS suppressed "
				       "pkts to BCMC SCBQ of BSSID "MACF"\n",
				       wlc->pub->unit, idx,
				       pktq_n_pkts_tot(&SCB_PSINFO(wlc->psinfo, scb)->psq),
				       ETHER_TO_MACF(cfg->BSSID)));
				wlc_apps_scb_merge_suppress(wlc, scb);

				WL_TMP(("TXQ: %s config PS-on output for BCMC\n", __FUNCTION__));

				/* reconfigure the bcmc MUXs for current PS mode */
				wlc_bcmc_start_mux_sources(cfg);
#endif /* TXQ_MUX */
			}
		}
		FOREACHSCB(wlc->scbstate, &scbiter, scb) {
#if !defined(TXQ_MUX)
			WL_PS(("wl%d.%d wlc_apps_process_pend_ps: Normalizing PSQ for STA "MACF"\n",
			       wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
			       ETHER_TO_MACF(scb->ea)));
#endif /* TXQ_MUX */
			if (SCB_PS(scb) && !SCB_ISMULTI(scb)) {
				struct apps_scb_psinfo *scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
				ASSERT(scb_psinfo);

#if !defined(TXQ_MUX)
				wlc_apps_scb_psq_norm(wlc, scb);
#else
				WL_PS(("wl%d.%d wlc_apps_process_pend_ps: Merging %u PS suppressed "
				       "pkts to SCBQ for STA "MACF"\n",
				       wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
				       pktq_n_pkts_tot(&SCB_PSINFO(wlc->psinfo, scb)->psq),
				       ETHER_TO_MACF(scb->ea)));
				wlc_apps_scb_merge_suppress(wlc, scb);
#endif /* TXQ_MUX */
				if ((scb_psinfo->ps_trans_status & SCB_PS_TRANS_OFF_PEND)) {
#ifdef PSPRETEND
					if (SCB_PS_PRETEND_BLOCKED(scb)) {
						WL_ERROR(
						("wl%d.%d: %s: SCB_PS_PRETEND_BLOCKED, "
						"expected to see PMQ PPS entry\n",
						wlc->pub->unit,
						WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
						__FUNCTION__));
					}
#endif /* PSPRETEND */
					WL_PS(("wl%d.%d wlc_apps_process_pend_ps: Allowing PS Off"
						" for STA "MACF"\n", wlc->pub->unit,
						WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
						ETHER_TO_MACF(scb->ea)));
					wlc_apps_scb_ps_off(wlc, scb, FALSE);
				}
			}
#ifdef PSPRETEND
			if (SCB_PS_PRETEND_PROBING(scb)) {
				wlc_pspretend_probe_sched(wlc->pps_info, scb);
			}
#endif /* PSPRETEND */
		}

		if (MBSS_ENAB(wlc->pub)) {
			wlc_apps_bss_ps_on_done(wlc);
		}
		/* If any suppressed BCMC packets at the head of txq,
		 * they need to be sent to hw fifo right now.
		 */
		if (wlc->active_queue != NULL && WLC_TXQ_OCCUPIED(wlc)) {
			wlc_send_q(wlc, wlc->active_queue);
		}
	}
}

/* wlc_apps_ps_flush()
 * Free any pending PS packets for this STA
 *
 * Called when APPS is handling a driver down transision, when an SCB is deleted,
 * when wlc_apps_scb_ps_off() is called with the discard param
 *	- from wlc_scb_disassoc_cleanup()
 *	- when a STA re-associates
 *	- from a deauth completion
 */
static void
wlc_apps_ps_flush(wlc_info_t *wlc, struct scb *scb)
{
#if defined(BCMDBG) || defined(WLMSG_PS)
	char eabuf[ETHER_ADDR_STR_LEN];
#endif
	void *pkt;
	struct ether_addr ea;
	struct apps_scb_psinfo *scb_psinfo;
	apps_wlc_psinfo_t *wlc_psinfo;
	wlc_bsscfg_t *bsscfg;

	ASSERT(scb);
	ASSERT(wlc);

	/* save ea and bsscfg before call wlc_pkt_flush */
	ea = scb->ea;
	bsscfg = scb->bsscfg;
	wlc_psinfo = wlc->psinfo;
	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
	if (scb_psinfo == NULL)
		return;

	WL_PS(("wl%d.%d: wlc_apps_ps_flush: flushing %d packets for %s aid %d\n",
	       wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), pktq_n_pkts_tot(&scb_psinfo->psq),
	       bcm_ether_ntoa(&scb->ea, eabuf), AID2PVBMAP(scb->aid)));

	/* Don't care about dequeue precedence */
	while ((pkt = pktq_deq(&scb_psinfo->psq, NULL))) {
#if !defined(TXQ_MUX)
		if (!SCB_ISMULTI(scb))
			wlc_psinfo->ps_deferred--;
#endif /* !TXQ_MUX */
		WLPKTTAG(pkt)->flags &= ~WLF_PSMARK; /* clear the timestamp */
		/* reclaim callbacks and free */
		PKTFREE(wlc->osh, pkt, TRUE);

		/* callback may have freed scb */
		if (!ETHER_ISMULTI(&ea) && (wlc_scbfind(wlc, bsscfg, &ea) == NULL)) {
			WL_PS(("wl%d.%d: wlc_apps_ps_flush: exiting, scb for %s was freed\n",
				wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
				bcm_ether_ntoa(&ea, eabuf)));
			return;
		}
	}

	ASSERT(pktq_empty(&scb_psinfo->psq));

	/* If there is a valid aid (the bcmc scb wont have one) then ensure
	 * the PVB is cleared.
	 */
	if (scb->aid && scb_psinfo->in_pvb)
		wlc_apps_pvb_update(wlc, scb);
}

#if !defined(TXQ_MUX)
void
wlc_apps_ps_flush_by_prio(wlc_info_t *wlc, struct scb *scb, int prec)
{
	struct apps_scb_psinfo *scb_psinfo;
	struct pktq *pktq;
	void *pkt;
#if defined(BCMDBG) || defined(WLMSG_PS)
	char eabuf[ETHER_ADDR_STR_LEN];
#endif
	wlc_bsscfg_t *bsscfg;

	ASSERT(scb);
	bsscfg = scb->bsscfg;
	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	pktq = &scb_psinfo->psq;

	while ((pkt = pktq_pdeq_tail(pktq, prec)) != NULL) {
		if (!SCB_ISMULTI(scb)) {
			wlc->psinfo->ps_deferred--;
		}
		WLPKTTAG(pkt)->flags &= ~WLF_PSMARK; /* clear the timestamp */
		/* reclaim callbacks and free */
		PKTFREE(wlc->osh, pkt, TRUE);

		/* callback may have freed scb */
		if (!ETHER_ISMULTI(&scb->ea) && (wlc_scbfind(wlc, bsscfg, &scb->ea) == NULL)) {
			WL_PS(("wl%d.%d: wlc_apps_ps_flush: exiting, scb for %s was freed",
				wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
				bcm_ether_ntoa(&scb->ea, eabuf)));
			return;
		}
	}
}
#endif /* !TXQ_MUX */

#ifdef PROP_TXSTATUS
void
wlc_apps_ps_flush_mchan(wlc_info_t *wlc, struct scb *scb)
{
	void *pkt;
	int prec;
	struct apps_scb_psinfo *scb_psinfo;
	apps_wlc_psinfo_t *wlc_psinfo;
	struct pktq tmp_q;

	ASSERT(scb);
	ASSERT(wlc);

	wlc_psinfo = wlc->psinfo;
	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
	if (scb_psinfo == NULL)
		return;
	pktq_init(&tmp_q, WLC_PREC_COUNT, PKTQ_LEN_DEFAULT);

	/* Don't care about dequeue precedence */
	while ((pkt = pktq_deq(&scb_psinfo->psq, &prec))) {
#if !defined(TXQ_MUX)
		if (!SCB_ISMULTI(scb)) {
			wlc_psinfo->ps_deferred--;
		}
#endif /* !TXQ_MUX */

		if (!(WL_TXSTATUS_GET_FLAGS(WLPKTTAG(pkt)->wl_hdr_information) &
			WLFC_PKTFLAG_PKTFROMHOST)) {
			pktq_penq(&tmp_q, prec, pkt);
			continue;
		}

		WLPKTTAG(pkt)->flags &= ~WLF_PSMARK; /* clear the timestamp */

		wlc_suppress_sync_fsm(wlc, scb, pkt, TRUE);
		wlc_process_wlhdr_txstatus(wlc, WLFC_CTL_PKTFLAG_WLSUPPRESS, pkt, FALSE);
		PKTFREE(wlc->osh, pkt, TRUE);


	 }
	/* Enqueue back the frames generated in dongle */
	while ((pkt = pktq_deq(&tmp_q, &prec))) {

#ifdef BCMFRWDPOOLREORG
		if (BCMFRWDPOOLREORG_ENAB() && PKTISFRWDPKT(wlc->pub->osh, pkt) &&
				PKTISFRAG(wlc->pub->osh, pkt)) {
			PKTFREE(wlc->osh, pkt, TRUE);
		} else
#endif
		{
			pktq_penq(&scb_psinfo->psq, prec, pkt);
#if !defined(TXQ_MUX)
			wlc_psinfo->ps_deferred++;
#endif /* !TXQ_MUX */
		}
	}

	/* If there is a valid aid (the bcmc scb wont have one) then ensure
	 * the PVB is cleared.
	 */
	if (scb->aid && scb_psinfo->in_pvb)
		wlc_apps_pvb_update(wlc, scb);
}
#endif /* defined(PROP_TXSTATUS) */


#if !defined(TXQ_MUX)
/* Return TRUE if packet has been enqueued on a ps queue, FALSE otherwise */
#define WLC_PS_APSD_HPKT_TIME 12 /* in ms */

bool
wlc_apps_psq(wlc_info_t *wlc, void *pkt, int prec)
{
	apps_wlc_psinfo_t *wlc_psinfo;
	struct apps_scb_psinfo *scb_psinfo;
	struct scb *scb;
	int psq_len;
	int psq_len_diff;

	scb = WLPKTTAGSCBGET(pkt);
	ASSERT(SCB_PS(scb));
	ASSERT(wlc);

	/* Do not enq bcmc pkts on a psq, also
	 * ageing out packets may have disassociated the STA, so return FALSE if so
	 * unless scb is wds
	 */
	if (!SCB_ASSOCIATED(scb) && !BSSCFG_IBSS(SCB_BSSCFG(scb)) && !SCB_WDS(scb)) {
		return FALSE;
	}

	ASSERT(!SCB_ISMULTI(scb));

	wlc_psinfo = wlc->psinfo;
	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
	ASSERT(scb_psinfo);

#if defined(PROP_TXSTATUS)
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
		/* If the host sets a flag marking the packet as "in response to
		   credit request for pspoll" then only the fimrware enqueues it.
		   Otherwise wlc drops it by sending a wlc_suppress.
		*/
		if ((WL_TXSTATUS_GET_FLAGS(WLPKTTAG(pkt)->wl_hdr_information) &
			WLFC_PKTFLAG_PKTFROMHOST) &&
			HOST_PROPTXSTATUS_ACTIVATED(wlc) &&
			(!(WL_TXSTATUS_GET_FLAGS(WLPKTTAG(pkt)->wl_hdr_information) &
			WLFC_PKTFLAG_PKT_REQUESTED))) {
			WLFC_DBGMESG(("R[0x%.8x]\n", (WLPKTTAG(pkt)->wl_hdr_information)));
			return FALSE;
		}
	}
#endif /* PROP_TXSTATUS */

	psq_len = pktq_n_pkts_tot(&scb_psinfo->psq);
	/* Deferred PS pkt flow control
	 * If this scb currently contains less than the minimum number of PS pkts
	 * per scb then enq it. If the total number of PS enq'd pkts exceeds the
	 * watermark and more than the minimum number of pkts are already enq'd
	 * for this STA then do not enq the pkt.
	 */
#ifdef PROP_TXSTATUS
	if (!PROP_TXSTATUS_ENAB(wlc->pub) || !HOST_PROPTXSTATUS_ACTIVATED(wlc) ||
#ifdef WL_NATOE
			(NATOE_ENAB(wlc->pub) && PKTISFRWDPKT(wlc->pub->osh, pkt)) ||
#endif /* WL_NATOE */
		FALSE)
#endif /* PROP_TXSTATUS */
	{
		if (psq_len > (int)wlc_psinfo->psq_pkts_lo &&
		    wlc_psinfo->ps_deferred > wlc_psinfo->psq_pkts_hi) {
			WL_PS(("wl%d.%d: wlc_apps_psq: can't buffer packet for "MACF" aid %d, %d "
			       "queued for scb, %d for WL\n", wlc->pub->unit,
			       WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), ETHER_TO_MACF(scb->ea),
			       AID2PVBMAP(scb->aid), pktq_n_pkts_tot(&scb_psinfo->psq),
			       wlc_psinfo->ps_deferred));
			return FALSE;
		}
	}

	WL_PS(("wl%d.%d:%s(): enq %p to PSQ, prec = 0x%x, scb_psinfo->apsd_usp = %s\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, pkt, prec,
		scb_psinfo->apsd_usp ? "TRUE" : "FALSE"));

	if (!wlc_prec_enq(wlc, &scb_psinfo->psq, pkt, prec)) {
		WL_PS(("wl%d.%d:%s(): wlc_prec_enq() failed\n", wlc->pub->unit,
			WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
		return FALSE;
	}

	/* increment total count of PS pkts enqueued in WL driver */
	psq_len_diff = pktq_n_pkts_tot(&scb_psinfo->psq) - psq_len;
	if (psq_len_diff == 1)
		wlc_psinfo->ps_deferred++;
	else if (psq_len_diff == 0)
		WL_PS(("wl%d.%d:%s(): wlc_prec_enq() dropped pkt.\n", wlc->pub->unit,
			WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));

#ifdef WLTDLS
	if (BSS_TDLS_ENAB(wlc, scb->bsscfg)) {
		if (!scb_psinfo->apsd_usp)
			wlc_tdls_send_pti(wlc->tdls, scb);
		else if (wlc_tdls_in_pti_interval(wlc->tdls, scb)) {
			scb_psinfo->apsd_cnt = wlc_apps_apsd_delv_count(wlc, scb);
			if (scb_psinfo->apsd_cnt)
				wlc_apps_apsd_send(wlc, scb);
			return TRUE;
		}
	}
#endif /* WLTDLS */

	/* Check if the PVB entry needs to be set */
	if (scb->aid && !scb_psinfo->in_pvb)
		wlc_apps_pvb_update(wlc, scb);

#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
		wlc_wlfc_scb_psq_resp(wlc, scb);

		if (scb_psinfo->apsd_hpkt_timer_on) {

			wl_del_timer(wlc->wl, scb_psinfo->apsd_hpkt_timer);

			if (!scb_psinfo->apsd_tx_pending && scb_psinfo->apsd_usp &&
			    scb_psinfo->apsd_cnt) {
				wlc_apps_apsd_send(wlc, scb);

				if (scb_psinfo->apsd_cnt > 1 &&
				    wlc_apps_apsd_delv_count(wlc, scb) == 1) {
					uint8 prec_bmp =
					        WLC_ACBITMAP_TO_PRECBITMAP(scb->apsd.ac_delv);
					wlc_wlfc_psmode_request(wlc->wlfc, scb,
						1, prec_bmp, WLFC_CTL_TYPE_MAC_REQUEST_PACKET);
					wl_add_timer(wlc->wl, scb_psinfo->apsd_hpkt_timer,
						WLC_PS_APSD_HPKT_TIME, FALSE);
				} else
					scb_psinfo->apsd_hpkt_timer_on = FALSE;
			} else
				scb_psinfo->apsd_hpkt_timer_on = FALSE;

		}
	}
#endif /* PROP_TXSTATUS */
	return (TRUE);
}
#endif /* TXQ_MUX */

#if !defined(TXQ_MUX)
// TXQ_MUX: wlc_apps_ps_send() replaced by SCBQ PS output fn
/*
 * Move a PS-buffered packet to the txq and send the txq.
 * Returns TRUE if a packet was available to dequeue and send.
 * extra_flags are added to packet flags (for SDU, only to last MPDU)
 */
static bool
wlc_apps_ps_send(wlc_info_t *wlc, struct scb *scb, uint prec_bmp, uint32 extra_flags)
{
	void *pkt = NULL;
	struct apps_scb_psinfo *scb_psinfo;
	apps_wlc_psinfo_t *wlc_psinfo;
	int prec;
	struct pktq *psq;

	ASSERT(wlc);
	wlc_psinfo = wlc->psinfo;

	ASSERT(scb);
	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
	ASSERT(scb_psinfo);

	psq = &scb_psinfo->psq;

	WL_PS(("wl%d.%d:%s\n", wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
	/* Dequeue the packet with highest precedence out of a given set of precedences */
	if (!(pkt = pktq_mdeq(psq, prec_bmp, &prec))) {
		return FALSE;		/* no traffic to send */
	}

	WL_PS(("wl%d.%d:%s: dequed pkt %p\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, pkt));
	/*
	 * If it's the first MPDU in a series of suppressed MPDUs that make up an SDU,
	 * enqueue all of them together before calling wlc_send_q.
	 */
	/*
	 * It's possible that hardware resources may be available for
	 * one fragment but not for another (momentarily).
	 */
	if (WLPKTTAG(pkt)->flags & WLF_TXHDR) {
		struct dot11_header *h;
		uint tsoHdrSize = 0;
		void *next_pkt;
		uint seq_num, next_seq_num;
		bool control;

#ifdef WLTOEHW
		tsoHdrSize = WLC_TSO_HDR_LEN(wlc, (d11ac_tso_t*)PKTDATA(wlc->osh, pkt));
#endif
		h = (struct dot11_header *)
			(PKTDATA(wlc->osh, pkt) + tsoHdrSize + D11_TXH_LEN_EX(wlc));
		control = FC_TYPE(ltoh16(h->fc)) == FC_TYPE_CTL;

		/* Control frames does not have seq field; directly queue
		 * them.
		 */
		if (!control) {
			seq_num = ltoh16(h->seq) >> SEQNUM_SHIFT;

			while ((next_pkt = pktqprec_peek(psq, prec)) != NULL) {
				/* Stop if different SDU */
				if (!(WLPKTTAG(next_pkt)->flags & WLF_TXHDR))
					break;

				/* Stop if different sequence number */
#ifdef WLTOEHW
				tsoHdrSize = WLC_TSO_HDR_LEN(wlc,
						(d11ac_tso_t*)PKTDATA(wlc->osh, next_pkt));
#endif
				h = (struct dot11_header *) (PKTDATA(wlc->osh, next_pkt) +
					tsoHdrSize + D11_TXH_LEN_EX(wlc));
				control = FC_TYPE(ltoh16(h->fc)) == FC_TYPE_CTL;

				/* stop if different ft; control frames does
				 * not have sequence control.
				 */
				if (control)
					break;

				next_seq_num = ltoh16(h->seq) >> SEQNUM_SHIFT;
				if (next_seq_num != seq_num)
					break;

				/* Enqueue the PS-Poll response at higher precedence level */
				wlc_apps_ps_enq_resp(wlc, scb, pkt,
					WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt)));

				/* Dequeue the peeked packet */
				pkt = pktq_pdeq(psq, prec);
				ASSERT(pkt == next_pkt);
			}
		}
	}

	/* Set additional flags on SDU or on final MPDU */
	WLPKTTAG(pkt)->flags |= extra_flags;

	WLPKTTAGBSSCFGSET(pkt, WLC_BSSCFG_IDX(scb->bsscfg));

	/* Enqueue the PS-Poll response at higher precedence level */
	if (!wlc_apps_ps_enq_resp(wlc, scb, pkt, WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt))))
		wlc_apps_apsd_usp_end(wlc, scb);

	/* Send to hardware (latency for first APSD-delivered frame is especially important) */
	wlc_send_q(wlc, SCB_WLCIFP(scb)->qi);


	/* Check if the PVB entry needs to be cleared */
	if (scb_psinfo->in_pvb)
		wlc_apps_pvb_update(wlc, scb);

#ifdef PROP_TXSTATUS
	if (extra_flags & WLF_APSD)
		scb_psinfo->apsd_tx_pending = TRUE;
#endif

	return TRUE;
}

static bool
wlc_apps_ps_enq_resp(wlc_info_t *wlc, struct scb *scb, void *pkt, int prec)
{
	apps_wlc_psinfo_t *wlc_psinfo;
	wlc_txq_info_t *qi = SCB_WLCIFP(scb)->qi;

	wlc_psinfo = wlc->psinfo;

	/* Decrement the global ps pkt cnt */
	if (!SCB_ISMULTI(scb))
		wlc_psinfo->ps_deferred--;

	/* register WLF2_PCB2_PSP_RSP for pkt */
	WLF2_PCB2_REG(pkt, WLF2_PCB2_PSP_RSP);

	/* Ensure the pkt marker (used for ageing) is cleared */
	WLPKTTAG(pkt)->flags &= ~WLF_PSMARK;

	WL_PS(("wl%d.%d: ps_enq_resp %p supr %d apsd %d\n",
	       wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), OSL_OBFUSCATE_BUF(pkt),
	       (WLPKTTAG(pkt)->flags & WLF_TXHDR) ? 1 : 0,
	       (WLPKTTAG(pkt)->flags & WLF_APSD) ? 1 : 0));

	/* Enqueue in order of precedence */
	if (!wlc_prec_enq(wlc, WLC_GET_TXQ(qi), pkt, prec)) {
		WL_ERROR(("wl%d.%d: %s: txq full, frame discarded\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
		PKTFREE(wlc->osh, pkt, TRUE);
		return FALSE;
	}

	return TRUE;
}
#endif /* !TXQ_MUX */

void
wlc_apps_set_listen_prd(wlc_info_t *wlc, struct scb *scb, uint16 listen)
{
	struct apps_scb_psinfo *scb_psinfo;
	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	if (scb_psinfo != NULL)
		scb_psinfo->listen = listen;
}

uint16
wlc_apps_get_listen_prd(wlc_info_t *wlc, struct scb *scb)
{
	struct apps_scb_psinfo *scb_psinfo;
	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	if (scb_psinfo != NULL)
		return scb_psinfo->listen;
	return 0;
}

#if !defined(TXQ_MUX)
// Need to integrate PS aging with SCBQ flow control and AQM
static bool
wlc_apps_psq_ageing_needed(wlc_info_t *wlc, struct scb *scb)
{
	wlc_bss_info_t *current_bss = scb->bsscfg->current_bss;
	uint16 interval;
	struct apps_scb_psinfo *scb_psinfo;

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

	/* Using scb->listen + 1 sec for ageing to avoid packet drop.
	 * In WMM-PS:Test Case 4.10(M.V) which is legacy mixed with wmmps.
	 * buffered frame will be dropped because ageing occurs.
	 */
	interval = scb_psinfo->listen + (1000/current_bss->beacon_period);

#ifdef WLWNM_AP
	if (WLWNM_ENAB(wlc->pub)) {
		uint32 wnm_scbcap = wlc_wnm_get_scbcap(wlc, scb);
		int sleep_interval = wlc_wnm_scb_sm_interval(wlc, scb);

		if (SCB_WNM_SLEEP(wnm_scbcap) && sleep_interval) {
			interval = MAX((current_bss->dtim_period * sleep_interval), interval);
		}
	}
#endif /* WLWNM_AP */

	return (scb_psinfo->tbtt >= interval);
}

/* Reclaim as many PS pkts as possible
 *	Reclaim from all STAs with pending traffic.
 */
void
wlc_apps_psq_ageing(wlc_info_t *wlc)
{
	apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
	struct apps_scb_psinfo *scb_psinfo;
	struct scb_iter scbiter;
	struct scb *tscb;

	if (wlc_psinfo->ps_nodes_all == 0) {
		return; /* No one in PS */
	}

	FOREACHSCB(wlc->scbstate, &scbiter, tscb) {
		if (!tscb->permanent && SCB_PS(tscb) && wlc_apps_psq_ageing_needed(wlc, tscb)) {
			scb_psinfo = SCB_PSINFO(wlc_psinfo, tscb);
			scb_psinfo->tbtt = 0;
			/* Initiate an ageing event per listen interval */
			if (!pktq_empty(&scb_psinfo->psq))
				wlc_apps_ps_timedout(wlc, tscb);
		}
	}
}

/**
 * Context structure used by wlc_apps_ps_timeout_filter() while filtering a ps pktq
 */
struct wlc_apps_ps_timeout_filter_info {
	uint              count;    /**< total num packets deleted */
};

/**
 * Pktq filter function to age-out pkts on an SCB psq.
 */
static pktq_filter_result_t
wlc_apps_ps_timeout_filter(void* ctx, void* pkt)
{
	struct wlc_apps_ps_timeout_filter_info *info;
	pktq_filter_result_t ret;

	info = (struct wlc_apps_ps_timeout_filter_info *)ctx;

	/* If not marked just move on */
	if ((WLPKTTAG(pkt)->flags & WLF_PSMARK) == 0) {
		WLPKTTAG(pkt)->flags |= WLF_PSMARK;
		ret = PKT_FILTER_NOACTION;
	} else {
		info->count++;
		ret = PKT_FILTER_DELETE;
	}

	return ret;
}

/* check if we should age pkts or not */
static void
wlc_apps_ps_timedout(wlc_info_t *wlc, struct scb *scb)
{
	struct ether_addr ea;
	struct apps_scb_psinfo *scb_psinfo;
	apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
	struct pktq *psq;
	wlc_bsscfg_t *bsscfg;
	struct wlc_apps_ps_timeout_filter_info info;
	uint16 aid;

	BCM_REFERENCE(aid);

	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
	ASSERT(scb_psinfo);

	psq = &scb_psinfo->psq;
	ASSERT(!pktq_empty(psq));

	/* save ea and bsscfg before call pkt flush */
	ea = scb->ea;
	bsscfg = SCB_BSSCFG(scb);
	aid = scb->aid;

	/* init the state for the pktq filter */
	info.count = 0;

	/* Age out all pkts that have been through one previous listen interval */
	wlc_txq_pktq_filter(wlc, psq, wlc_apps_ps_timeout_filter, &info);

	WL_PS(("wl%d.%d: wlc_apps_ps_timedout: timing out %d packet for "MACF" aid %d, "
	       "%d remain\n", wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), info.count,
	       ETHERP_TO_MACF(&ea), AID2PVBMAP(aid), pktq_n_pkts_tot(psq)));

	wlc_psinfo->ps_deferred -= info.count;
	wlc_psinfo->ps_aged += info.count;

	/* callback may have freed scb, exit early if so */
	if (wlc_scbfind(wlc, bsscfg, &ea) == NULL) {
		WL_PS(("wl%d.%d: wlc_apps_ps_timedout: exiting, scb for "MACF" was "
		       "freed after last packet timeout\n", wlc->pub->unit,
		       WLC_BSSCFG_IDX(bsscfg), ETHERP_TO_MACF(&ea)));
		return;
	}

	/* update the beacon PVB, but only if the SCB was not deleted */
	if (scb_psinfo->in_pvb) {
		wlc_apps_pvb_update(wlc, scb);
	}
}
#endif /* TXQ_MUX */

#if !defined(TXQ_MUX)
/* _wlc_apps_ps_enq()
 *
 * Try to PS enq a pkt, return false if we could not
 *
 * _wlc_apps_ps_enq() Called from:
 *	wlc_apps_ps_enq() TxMod
 *	wlc_apps_move_to_psq()
 *		wlc_apps_ampdu_txq_to_psq() --| from wlc_apps_scb_ps_on()
 *		wlc_apps_txq_to_psq() --------| from wlc_apps_scb_ps_on()
 * Implements TDLS PRI response handling to bypass PSQ buffering
 *
 * TXQ_MUX note: No need to migrate pkts from TxQ or AMPDUQ to psq,
 *     need to check TDLS PTI work.
 */
static void
_wlc_apps_ps_enq(void *ctx, struct scb *scb, void *pkt, uint prec)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;

	ASSERT(!SCB_ISMULTI(scb));
	ASSERT(!PKTISCHAINED(pkt));

	/* in case of PS mode */
	if ((WLPKTTAG(pkt)->flags & WLF_8021X)) {
		prec = WLC_PRIO_TO_HI_PREC(MAXPRIO);
	}

#ifdef WLTDLS
	/* for TDLS PTI resp, don't enq to PSQ, send right away */
	if (BSS_TDLS_ENAB(wlc, SCB_BSSCFG(scb)) && SCB_PS(scb) &&
		(WLPKTTAG(pkt)->flags & WLF_PSDONTQ)) {
		WL_PS(("wl%d.%d:%s(): skip enq to PSQ\n", wlc->pub->unit,
			WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
		SCB_TX_NEXT(TXMOD_APPS, scb, pkt, prec);
		return;
	}
#endif /* WLTDLS */

	if (!wlc_apps_psq(wlc, pkt, prec)) {
		struct apps_scb_psinfo *scb_psinfo;
		WL_PS(("wl%d.%d: wlc_apps_ps_enq: ps pkt discarded\n", wlc->pub->unit,
		       WLC_BSSCFG_IDX(SCB_BSSCFG(scb))));
		wlc_psinfo->ps_discard++;
		ASSERT(scb);
		scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
		ASSERT(scb_psinfo);
		scb_psinfo->ps_discard++;
#ifdef PROP_TXSTATUS
		if (PROP_TXSTATUS_ENAB(wlc->pub)) {
			/*
			   wlc decided to discard the packet, host should hold onto it,
			   this is effectively a "suppress by wl" instead of D11
			*/
			wlc_suppress_sync_fsm(wlc, scb, pkt, TRUE);
			wlc_process_wlhdr_txstatus(wlc, WLFC_CTL_PKTFLAG_WLSUPPRESS, pkt, FALSE);
		}
#endif /* PROP_TXSTATUS */

		PKTFREE(wlc->osh, pkt, TRUE);
	} else {
		struct apps_scb_psinfo *scb_psinfo;
		scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
		scb_psinfo->ps_queued++;
	}
}
#endif /* TXQ_MUX */

#if !defined(TXQ_MUX)
/* PS TxModule enqueue function */
static void
wlc_apps_ps_enq(void *ctx, struct scb *scb, void *pkt, uint prec) // Non-TXQ_MUX version
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;

	_wlc_apps_ps_enq(wlc, scb, pkt, prec);

#if defined(BCMPCIEDEV) && defined(PROP_TXSTATUS)
	if (PROP_TXSTATUS_ENAB(wlc->pub) && BCMPCIEDEV_ENAB()) {
		struct apps_wlc_psinfo *wlc_psinfo;
		struct apps_scb_psinfo *scb_psinfo;
		ASSERT(wlc);
		ASSERT(scb);
		wlc_psinfo = wlc->psinfo;
		scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
		if (scb_psinfo == NULL)
			return;
		/* we just recieved a packet from the host through wlc_sendpkt and
		 * the TxMod chain, so decrement the in_transit count of packets
		 * requested from the host
		 */
		if (scb_psinfo->in_transit > 0) {
			scb_psinfo->in_transit --;
		}
	}
#endif /* BCMPCIEDEV && PROP_TXSTATUS */
}
#else
/* PS TxModule enqueue function */
static void
wlc_apps_ps_enq(void *ctx, struct scb *scb, void *pkt, uint prec) // TXQ_MUX version
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	struct apps_scb_psinfo *scb_psinfo;

	/* send the pkt down the chain */
	SCB_TX_NEXT(TXMOD_APPS, scb, pkt, prec);

	/* the packet is passed on first so that the wlc_apps_pvb_update() can
	 * check the state of the scbq tx queue to determine if the AID bit
	 * in the PVB needs to be set.
	 */

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

	/* update the PVB if needed for a new tx packet in the queue */
	if (!scb_psinfo->in_pvb) {
		wlc_apps_pvb_update(wlc, scb);

		if (scb_psinfo->in_pvb) {
			WL_TMP(("TXQ: %s adding pvb entry %d\n",
			        __FUNCTION__, scb->aid));
		} else {
			WL_TMP(("TXQ: %s What?? did not add pvb entry %d\n",
			        __FUNCTION__, scb->aid));
		}
	}
}
#endif /* TXQ_MUX */

#if !defined(TXQ_MUX)
/* TXQ_MUX: No need to migrate pkts from TxQ or AMPDUQ to psq in MUX */

/* Try to ps enq the pkts on the txq */
static void
wlc_apps_txq_to_psq(wlc_info_t *wlc, struct scb *scb)
{
	struct pktq *txq = WLC_GET_TXQ(SCB_WLCIFP(scb)->qi);
	wlc_apps_move_to_psq(wlc, txq, scb);
}

#if defined(WLAMPDU) && defined(PROP_TXSTATUS)
/* This causes problems for PSPRETEND */
/* ps enq pkts on ampduq */
static void
wlc_apps_ampdu_txq_to_psq(wlc_info_t *wlc, struct scb *scb)
{
	if (AMPDU_ENAB(wlc->pub)) {
		struct pktq *txq = wlc_ampdu_txq(wlc->ampdu_tx, scb);
		if (txq) wlc_apps_move_to_psq(wlc, txq, scb);
	}
}
#endif /* WLAMPDU && PROP_TXSTATUS */

#endif /* TXQ_MUX */

#if !defined(TXQ_MUX)
/* TXQ_MUX: No need to migrate pkts from TxQ or AMPDUQ to psq in MUX */

static void
wlc_apps_move_to_psq(wlc_info_t *wlc, struct pktq *txq, struct scb* scb)
{
	void *head_pkt = NULL, *pkt;
	int prec;
#ifdef WLTDLS
	bool q_empty = TRUE;
	apps_wlc_psinfo_t *wlc_psinfo;
	struct apps_scb_psinfo *scb_psinfo;
#endif

	ASSERT(AP_ENAB(wlc->pub) || BSS_TDLS_BUFFER_STA(SCB_BSSCFG(scb)) ||
		BSSCFG_IBSS(SCB_BSSCFG(scb)));

	PKTQ_PREC_ITER(txq, prec) {
		head_pkt = NULL;
		/* PS enq all the pkts we can */
		while (pktqprec_peek(txq, prec) != head_pkt) {
			pkt = pktq_pdeq(txq, prec);
			ASSERT(pkt != NULL);
			if (scb != WLPKTTAGSCBGET(pkt)) {
				if (!head_pkt)
					head_pkt = pkt;
				pktq_penq(txq, prec, pkt);
				continue;
			}
			/* Enqueueing at the same prec may create a remote
			 * possibility of suppressed pkts being reordered.
			 * Needs to be investigated...
			 */
			_wlc_apps_ps_enq(wlc, scb, pkt, prec);

#if defined(WLTDLS)
			if (TDLS_ENAB(wlc->pub) && wlc_tdls_isup(wlc->tdls))
				q_empty = FALSE;
#endif /* defined(WLTDLS) */
		}
	}

#if defined(WLTDLS)
	if (TDLS_ENAB(wlc->pub)) {
		ASSERT(wlc);
		wlc_psinfo = wlc->psinfo;

		ASSERT(scb);
		ASSERT(scb->bsscfg);
		scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
		ASSERT(scb_psinfo);
		if (!q_empty && !scb_psinfo->apsd_usp)
			wlc_tdls_send_pti(wlc->tdls, scb);
	}
#endif /* defined(WLTDLS) */
}

#endif /* TXQ_MUX */

#ifdef PROP_TXSTATUS
/* update in_transit counter */
bool
wlc_apps_pvb_upd_in_transit(wlc_info_t *wlc, struct scb *scb, uint vqdepth, bool op)
{
#if defined(BCMPCIEDEV) && !defined(TXQ_MUX)
	if (PROP_TXSTATUS_ENAB(wlc->pub) && BCMPCIEDEV_ENAB()) {
		struct apps_wlc_psinfo *wlc_psinfo;
		struct apps_scb_psinfo *scb_psinfo;
		uint8 max_depth;

		wlc_psinfo = wlc->psinfo;
		scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
		ASSERT(scb_psinfo);

		max_depth = vqdepth;
#ifdef WLTDLS
		if (TDLS_ENAB(wlc->pub) && BSSCFG_IS_TDLS(SCB_BSSCFG(scb))) {
			max_depth = TDLS_WLFC_DEFAULT_FWQ_DEPTH;
		}
#endif /* WLTDLS */
		/* allow fecting one more packet if apsd session is running */
		max_depth += (scb_psinfo->apsd_usp == TRUE);
		/* fetch the host packets given the wlc->wlfc_vqdepth constraint
		*/
		if (pktq_n_pkts_tot(&scb_psinfo->psq) + scb_psinfo->in_transit < max_depth) {
			scb_psinfo->in_transit ++;
			return FALSE;
		}
	}
#endif /* BCMPCIEDEV && ! TXQ_MUX */
	return TRUE;
}
#endif /* PROP_TXSTATUS */

/* Set/clear PVB entry. Called before changing the TSC state to EST */
void
wlc_apps_pvb_update_now(wlc_info_t *wlc, struct scb *scb)
{
	wlc_apps_pvb_update(wlc, scb);
	return;
}

/* Set/clear PVB entry according to current state of power save queues */
void
wlc_apps_pvb_update(wlc_info_t *wlc, struct scb *scb)
{
	uint16 aid;
	apps_wlc_psinfo_t *wlc_psinfo;
	struct apps_scb_psinfo *scb_psinfo;
	int ps_count;
	int pktq_total;
	apps_bss_info_t *bss_info;

	ASSERT(wlc);
	wlc_psinfo = wlc->psinfo;

	ASSERT(scb);
	ASSERT(scb->bsscfg);
	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
	if (scb_psinfo == NULL)
		return;

	bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, scb->bsscfg);

	/* get the count of pkts buffered for the scb */
#if defined(TXQ_MUX)
	// for MUX, PS pkts are just on the regular scbq
	pktq_total = pktq_n_pkts_tot(wlc_scbq_txq(wlc->tx_scbq, scb));
#else
	// for pre-TXQ_MUX, PS pkts are in the scb_psinfo->psq
	pktq_total = pktq_n_pkts_tot(&scb_psinfo->psq);
#endif /* TXQ_MUX */

	aid = AID2PVBMAP(scb->aid);
	ASSERT(aid);

	/*
	 * WMM/APSD 3.6.1.4: if no ACs are delivery-enabled (legacy), or all ACs are
	 * delivery-enabled (special case), the PVB should indicate if any packet is
	 * buffered.  Otherwise, the PVB should indicate if any packets are buffered
	 * for non-delivery-enabled ACs only.
	 */

	ps_count = ((scb->apsd.ac_delv == AC_BITMAP_NONE ||
	             scb->apsd.ac_delv == AC_BITMAP_ALL) ?
	            pktq_total :
	            wlc_apps_apsd_ndelv_count(wlc, scb));

#ifdef PROP_TXSTATUS
	/*
	If there is no packet locally, check the traffic availability flags from the
	host.
	*/
	if (ps_count == 0 && PROP_TXSTATUS_ENAB(wlc->pub) &&
	    HOST_PROPTXSTATUS_ACTIVATED(wlc) && (pktq_total == 0)) {
		ps_count = wlc_wlfc_scb_ps_count(wlc, scb, bss_info->ps_nodes > 0);
	}
#endif /* PROP_TXSTATUS */

	if (ps_count > 0 && SCB_PS(scb)) {
		if (scb_psinfo->in_pvb)
			return;

		WL_PS(("wl%d.%d: wlc_apps_pvb_update, setting aid %d scb:%p\n", wlc->pub->unit,
		       WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), aid, scb));
		/* set the bit in the pvb */
		setbit(bss_info->pvb, aid);

		/* reset the aid range */
		if ((aid < bss_info->aid_lo) || !bss_info->aid_lo)
			bss_info->aid_lo = aid;
		if (aid > bss_info->aid_hi)
			bss_info->aid_hi = aid;

		scb_psinfo->in_pvb = TRUE;
	} else {
		if (!scb_psinfo->in_pvb)
			return;

		WL_PS(("wl%d.%d: wlc_apps_pvb_entry, clearing aid %d scb:%p\n", wlc->pub->unit,
		       WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), aid, scb));
		/* clear the bit in the pvb */
		clrbit(bss_info->pvb, aid);

		if (bss_info->ps_nodes == 0) {
			bss_info->aid_lo = bss_info->aid_hi = 0;
		} else {
			/* reset the aid range */
			if (aid == bss_info->aid_hi) {
				/* find the next lowest aid value with PS pkts pending */
				for (aid = aid - 1; aid; aid--)
					if (isset(bss_info->pvb, aid)) {
						bss_info->aid_hi = aid;
						break;
					}
				/* no STAs with pending traffic ? */
				if (aid == 0)
					bss_info->aid_hi = bss_info->aid_lo = 0;
			} else if (aid == bss_info->aid_lo) {
				/* find the next highest aid value with PS pkts pending */
				for (aid = aid + 1; aid < wlc->pub->tunables->maxscb; aid++)
					if (isset(bss_info->pvb, aid)) {
						bss_info->aid_lo = aid;
						break;
					}
				ASSERT(aid != wlc->pub->tunables->maxscb);
			}
		}

		scb_psinfo->in_pvb = FALSE;
	}

	/* Update the PVB in the bcn template */
	WL_PS(("wl%d: wlc_apps_pvb_entry -> wlc_bss_update_beacon\n", wlc->pub->unit));
	WL_APSTA_BCN(("wl%d: wlc_apps_pvb_entry -> wlc_bss_update_beacon\n", wlc->pub->unit));
	wlc_bss_update_beacon(wlc, scb->bsscfg);
}

/* Increment the TBTT count for PS SCBs in a particular bsscfg */
static void
wlc_bss_apps_tbtt_update(wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = cfg->wlc;
	struct apps_scb_psinfo *scb_psinfo;
	struct scb *scb;
	struct scb_iter scbiter;

	ASSERT(cfg != NULL);
	ASSERT(BSSCFG_AP(cfg));

	/* If touching all the PS scbs is too inefficient then we
	 * can maintain a single count and only create an ageing event
	 * using the longest listen interval requested by a STA
	 */

	/* increment the tbtt count on all PS scbs */
	/* APSTA: For APSTA, don't bother aging AP SCBs */
	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
		if (!scb->permanent && SCB_PS(scb)) {
			scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
			if (scb_psinfo->tbtt < 0xFFFF) /* do not wrap around */
				scb_psinfo->tbtt++;
		}
	}
}

/* Increment the TBTT count for all PS SCBs */
void
wlc_apps_tbtt_update(wlc_info_t *wlc)
{
	int idx;
	wlc_bsscfg_t *cfg;

	/* If touching all the PS scbs multiple times is too inefficient
	 * then we can restore the old code and have all scbs updated in one pass.
	 */

	FOREACH_UP_AP(wlc, idx, cfg)
	        wlc_bss_apps_tbtt_update(cfg);
}


/* return the count of PS buffered pkts for an SCB */
int
wlc_apps_psq_len(wlc_info_t *wlc, struct scb *scb)
{
	int pktq_total;

#if defined(TXQ_MUX)
	// For TXQ_MUX All PS buffered pkts are on the scbq
	pktq_total = pktq_n_pkts_tot(wlc_scbq_txq(wlc->tx_scbq, scb));
#else
	struct apps_scb_psinfo *scb_psinfo;
	// Pre-TXQ_MUX, PS buffered pkts are on the scb_psinfo->psq
	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	pktq_total = pktq_n_pkts_tot(&scb_psinfo->psq);
#endif /* TXQ_MUX */

	return pktq_total;
}

/* called from bmac when a PS state switch is detected from the transmitter.
 * On PS ON switch, directly call wlc_apps_scb_ps_on(wlc, scb);
 *  On PS OFF, check if there are tx packets pending. If so, make a PS OFF reservation
 *  and wait for the drain. Otherwise, switch to PS OFF.
 *  Sends a message to the bmac pmq manager to signal that we detected this switch.
 *  PMQ manager will delete entries when switch states are in sync and the queue is drained.
 *  return 1 if a switch occured. This allows the caller to invalidate
 *  the header cache.
 */
void BCMFASTPATH
wlc_apps_process_ps_switch(wlc_info_t *wlc, struct ether_addr *ea, int8 ps_on)
{

	struct scb *scb = NULL;
	int32 idx;
	wlc_bsscfg_t *cfg;
	struct apps_scb_psinfo *scb_psinfo;

	/* Look for sta's that are associated with the AP, TDLS peers or IBSS peers. */
	FOREACH_BSS(wlc, idx, cfg) {
		if ((BSSCFG_AP(cfg) && cfg->up) || BSS_TDLS_ENAB(wlc, cfg) ||
			BSSCFG_IBSS(cfg)) {
			scb = wlc_scbfind(wlc, cfg, ea);
			if (scb != NULL)
				break;
		}
	}

	/* only process ps transitions for associated sta's, IBSS bsscfg and WDS peers */
	if (!scb || !(SCB_ASSOCIATED(scb) || BSSCFG_IBSS(cfg) || SCB_WDS(scb))) {
		/* send notification to bmac that this entry doesn't exist
		   up here.
		 */
		uint8 ps = PS_SWITCH_STA_REMOVED;
		uint16 *auxpmq_idx = NULL;

		ps |= (wlc->block_datafifo & DATA_BLOCK_PS) ?
			PS_SWITCH_OFF : PS_SWITCH_FIFO_FLUSHED;
		if (scb && SCB_PSINFO(wlc->psinfo, scb)) {
			auxpmq_idx = &(SCB_PSINFO(wlc->psinfo, scb)->auxpmq_idx);
		}
		wlc_bmac_process_ps_switch(wlc->hw, ea, ps, auxpmq_idx);

		return;
	}

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

	if (scb_psinfo == NULL)
		return;

	if (ps_on) {
		if ((ps_on & PS_SWITCH_PMQ_SUPPR_PKT)) {
			WL_PS(("wl%d.%d: %s: Req by suppr pkt! "MACF"\n",
				wlc->pub->unit, WLC_BSSCFG_IDX(cfg),
				__FUNCTION__, ETHERP_TO_MACF(ea)));
			scb_psinfo->ps_trans_status |= SCB_PS_TRANS_OFF_BLOCKED;
		} else {
			apps_bss_info_t *bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, cfg);
			ASSERT(bss_info);
			if (!SCB_PS(scb)) {
				WL_PS(("wl%d.%d: %s: Actual PMQ entry addition! "MACF"\n",
				       wlc->pub->unit, WLC_BSSCFG_IDX(cfg),
				       __FUNCTION__, ETHERP_TO_MACF(ea)));
			}

			/* This PS ON request is from actual PMQ entry addition. */
			scb_psinfo->ps_trans_status &= ~SCB_PS_TRANS_OFF_BLOCKED;
			bss_info->ps_trans_status &= ~BSS_PS_TRANS_OFF_BLOCKED;
		}
		if (!SCB_PS(scb)) {
#ifdef PSPRETEND
			/* reset pretend status */
			scb->ps_pretend &= ~PS_PRETEND_ON;
			if (PSPRETEND_ENAB(wlc->pub) &&
				(ps_on & PS_SWITCH_PMQ_PSPRETEND) && !SCB_ISMULTI(scb)) {
				wlc_pspretend_on(wlc->pps_info, scb, PS_PRETEND_ACTIVE_PMQ);
			}
			else
#endif /* PSPRETEND */
			{
				wlc_apps_scb_ps_on(wlc, scb, 0);
			}

			WL_PS(("wl%d.%d: "MACF" - PS %s, pretend mode off\n",
			      wlc->pub->unit, WLC_BSSCFG_IDX(cfg), ETHERP_TO_MACF(ea),
			      SCB_PS(scb) ? "on":"off"));
		}
#ifdef PSPRETEND
		else if (SCB_PS_PRETEND(scb) && (ps_on & PS_SWITCH_PMQ_PSPRETEND)) {
			WL_PS(("wl%d.%d: "MACF" PS pretend was already active now with new PMQ "
				   "PPS entry\n", wlc->pub->unit, WLC_BSSCFG_IDX(cfg),
				   ETHERP_TO_MACF(ea)));
			scb->ps_pretend |= PS_PRETEND_ACTIVE_PMQ;
		}
#endif /* PSPRETEND */
		else {
			/* STA is already in PS, clear PS OFF pending bit only */
			scb_psinfo->ps_trans_status &= ~SCB_PS_TRANS_OFF_PEND;
		}
	} else if ((scb_psinfo->ps_trans_status & SCB_PS_TRANS_OFF_BLOCKED)) {
		WL_PS(("wl%d.%d: "MACF" PS off attempt is blocked by WAITPMQ\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(cfg), ETHERP_TO_MACF(ea)));
	} else {
		if ((wlc->block_datafifo & DATA_BLOCK_PS)) {
			/* Prevent ON -> OFF transitions while data fifo is blocked.
			 * We need to finish flushing HW and reque'ing before we
			 * can allow the STA to come out of PS.
			 */
			WL_PS(("wl%d.%d: %s "MACF" DATA_BLOCK_PS %d pkts pending%s\n",
				wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
				ETHERP_TO_MACF(ea), TXPKTPENDTOT(wlc),
				SCB_PS_PRETEND(scb) ? " (ps pretend active)" : ""));
			scb_psinfo->ps_trans_status |= SCB_PS_TRANS_OFF_PEND;
		}
#ifdef PSPRETEND
		else if (SCB_PS_PRETEND_BLOCKED(scb)) {
			/* Prevent ON -> OFF transitions if we were expecting to have
			 * seen a PMQ entry for ps pretend and we have not had it yet.
			 * This is to ensure that when that entry does come later, it
			 * does not cause us to enter ps pretend mode when that condition
			 * should have been cleared
			 */
			WL_PS(("wl%d.%d: "MACF" ps pretend pending off waiting for the PPS PMQ "
				   "entry\n", wlc->pub->unit, WLC_BSSCFG_IDX(cfg),
				   ETHERP_TO_MACF(ea)));
			scb_psinfo->ps_trans_status |= SCB_PS_TRANS_OFF_PEND;
		}
#endif /* PSPRETEND */
		else if (SCB_PS(scb))  {
			wlc_apps_scb_ps_off(wlc, scb, FALSE);
		}
	}

	/* indicate fifo state  */
	if (!(wlc->block_datafifo & DATA_BLOCK_PS))
		ps_on |= PS_SWITCH_FIFO_FLUSHED;

	wlc_bmac_process_ps_switch(wlc->hw, &scb->ea, ps_on, &(scb_psinfo->auxpmq_idx));
	return;

}

/* wlc_apps_pspoll_resp_prepare()
 * Do some final pkt touch up before DMA ring for PS delivered pkts.
 * Also track pspoll response state (pkt callback and more_data signaled)
 */
void
wlc_apps_pspoll_resp_prepare(wlc_info_t *wlc, struct scb *scb,
                             void *pkt, struct dot11_header *h, bool last_frag)
{
	struct apps_scb_psinfo *scb_psinfo;

	ASSERT(scb);
	ASSERT(SCB_PS(scb));

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	/*
	 * FC_MOREDATA is set for every response packet being sent while STA is in PS.
	 * This forces STA to send just one more PS-Poll.  If by that time we actually
	 * have more data, it'll be sent, else a Null data frame without FC_MOREDATA will
	 * be sent.  This technique often improves TCP/IP performance.  The last NULL Data
	 * frame is sent with the WLF_PSDONTQ flag.
	 */

	h->fc |= htol16(FC_MOREDATA);

	/* Register pkt callback for PS-Poll response */
	if (last_frag && !SCB_ISMULTI(scb)) {
		WLF2_PCB2_REG(pkt, WLF2_PCB2_PSP_RSP);
#if !defined(TXQ_MUX)
		scb_psinfo->psp_pending = TRUE;
#endif
	}

	scb_psinfo->psp_flags |= PS_MORE_DATA;
}

/* Fix PDU that is being sent as a PS-Poll response or APSD delivery frame. */
void
wlc_apps_ps_prep_mpdu(wlc_info_t *wlc, void *pkt, wlc_txh_info_t *txh_info)
{
	bool last_frag;
	struct dot11_header *h;
	uint16	macCtlLow, frameid;
	struct scb *scb;
	wlc_bsscfg_t *bsscfg;
	wlc_key_info_t key_info;

	scb = WLPKTTAGSCBGET(pkt);

	h = txh_info->d11HdrPtr;

	WL_PS(("wl%d.%d: wlc_apps_ps_prep_mpdu: pkt %p flags 0x%x flags2 %x fc 0x%x\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), OSL_OBFUSCATE_BUF(pkt),
		WLPKTTAG(pkt)->flags, WLPKTTAG(pkt)->flags2, h->fc));

	/*
	 * Set the IGNOREPMQ bit.
	 *
	 * PS bcast/mcast pkts have following differences from ucast:
	 *    1. use the BCMC fifo
	 *    2. FC_MOREDATA is set by ucode (except for the kludge)
	 *    3. Don't set IGNOREPMQ bit as ucode ignores PMQ when draining
	 *       during DTIM, and looks at PMQ when draining through
	 *       MHF2_TXBCMC_NOW
	 */
	if (ETHER_ISMULTI(txh_info->TxFrameRA)) {
		ASSERT(!SCB_WDS(scb));

		/* Kludge required from wlc_dofrag */
		bsscfg = SCB_BSSCFG(scb);
		frameid = bcmc_fid_generate(wlc, bsscfg, txh_info->TxFrameID);

		/* Update the TxFrameID in both the txh_info struct and the packet header */
		txh_info->TxFrameID = htol16(frameid);

		if (D11REV_GE(wlc->pub->corerev, 40)) {
			d11actxh_t* vhtHdr = &(txh_info->hdrPtr->txd);
			vhtHdr->PktInfo.TxFrameID =  htol16(frameid);
		} else {
			d11txh_t* nonVHTHdr = &(txh_info->hdrPtr->d11txh);
			nonVHTHdr->TxFrameID = htol16(frameid);
		}

		ASSERT(!SCB_A4_DATA(scb));

		/* APSTA: MUST USE BSS AUTH DUE TO SINGLE BCMC SCB; IS THIS OK? */
		wlc_keymgmt_get_bss_tx_key(wlc->keymgmt, bsscfg, FALSE, &key_info);

		if (!bcmwpa_is_wpa_auth(bsscfg->WPA_auth) || key_info.algo != CRYPTO_ALGO_AES_CCM)
			h->fc |= htol16(FC_MOREDATA);
	}
	else if (!SCB_ISMULTI(scb)) {
		/* There is a hack to send uni-cast P2P_PROBE_RESP frames using bsscfg's
		* mcast scb because of no uni-cast scb is available for bsscfg, we need to exclude
		* such hacked packtes from uni-cast processing.
		*/
		last_frag = (ltoh16(h->fc) & FC_MOREFRAG) == 0;
		/* Set IGNOREPMQ bit (otherwise, it may be suppressed again) */
		macCtlLow = ltoh16(txh_info->MacTxControlLow);
		if (D11REV_GE(wlc->pub->corerev, 40)) {
			d11actxh_t* vhtHdr = &(txh_info->hdrPtr->txd);
			macCtlLow |= D11AC_TXC_IPMQ;
			vhtHdr->PktInfo.MacTxControlLow = htol16(macCtlLow);
#ifdef PSPRETEND
			if (PSPRETEND_ENAB(wlc->pub)) {
				uint16 macCtlHigh = ltoh16(txh_info->MacTxControlHigh);
				macCtlHigh &= ~D11AC_TXC_PPS;
				vhtHdr->PktInfo.MacTxControlHigh = htol16(macCtlHigh);
			}
#endif /* PSPRETEND */
		} else {
			d11txh_t* nonVHTHdr = &(txh_info->hdrPtr->d11txh);
			macCtlLow |= TXC_IGNOREPMQ;
			nonVHTHdr->MacTxControlLow = htol16(macCtlLow);
		}

		/*
		 * Set FC_MOREDATA and EOSP bit and register callback.  WLF_APSD is set
		 * for all APSD delivery frames.  WLF_PSDONTQ is set only for the final
		 * Null frame of a series of PS-Poll responses.
		 */
		if (WLPKTTAG(pkt)->flags & WLF_APSD)
			wlc_apps_apsd_prepare(wlc, scb, pkt, h, last_frag);
		else if (!(WLPKTTAG(pkt)->flags & WLF_PSDONTQ))
			wlc_apps_pspoll_resp_prepare(wlc, scb, pkt, h, last_frag);
	}
}

/* Packet callback fn for WLF2_PCB2_PSP_RSP
 *
 */
static void
wlc_apps_psp_resp_complete(wlc_info_t *wlc, void *pkt, uint txs)
{
	struct scb *scb;
	struct apps_scb_psinfo *scb_psinfo;

	BCM_REFERENCE(txs);

	/* Is this scb still around */
	if ((scb = WLPKTTAGSCBGET(pkt)) == NULL)
		return;

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	if (scb_psinfo == NULL)
		return;

	/* clear multiple ps-poll frame protection */
	scb_psinfo->psp_flags &= ~PS_PSP_ONRESP;

	/* Check if the PVB entry needs to be cleared */
	if (scb_psinfo->in_pvb) {
		wlc_apps_pvb_update(wlc, scb);
	}

#if !defined(TXQ_MUX) // Obsolete with PS_PSP_ONRESP logic
	if (scb_psinfo->psp_pending) {
		scb_psinfo->psp_pending = FALSE;
		// Obsolete with PS_PSP_ONRESP logic -- no chance for tx below
		if (scb_psinfo->tx_block != 0) {
			WL_PS(("wl%d.%d: %s: tx blocked\n", wlc->pub->unit,
			       WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
			return;
		}
		// Obsolete with PS_PSP_ONRESP logic -- prevents PS_PSP_REQ_PEND from being set
		if (scb_psinfo->psp_flags & PS_PSP_REQ_PEND) {
			/* send the next ps pkt if requested */
			scb_psinfo->psp_flags &= ~(PS_MORE_DATA | PS_PSP_REQ_PEND);
			WL_ERROR(("wl%d.%d:%s send more frame.\n", wlc->pub->unit,
				WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
			wlc_apps_ps_send(wlc, scb, WLC_PREC_BMP_ALL, 0);
		}
	}
#endif /* TXQ_MUX */
}

#if !defined(TXQ_MUX) // TXQ_MUX uses a different null data pkt prep method
static int
wlc_apps_send_psp_response_cb(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt, void *data)
{
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(cfg);
	BCM_REFERENCE(data);

	/* register packet callback */
	WLF2_PCB2_REG(pkt, WLF2_PCB2_PSP_RSP);
	return BCME_OK;
}
#endif /* TXQ_MUX */

/* wlc_apps_send_psp_response()
 *
 * This function is used in rx path when we get a PSPoll.
 * Also used for proptxstatus when a tx pkt is queued to the driver and
 * SCB_PROPTXTSTATUS_PKTWAITING() was set (pspoll happend, but no pkts local).
 *
 * For TXQ_MUX, this should signal the PS Output fn that we can release a BU. Done with
 * a call to wlc_apps_enable_ps_output().
 */
void
wlc_apps_send_psp_response(wlc_info_t *wlc, struct scb *scb, uint16 fc)
{
	struct apps_scb_psinfo *scb_psinfo;
#if !defined(TXQ_MUX)
	int pktq_total;
#endif

	ASSERT(scb);
	ASSERT(wlc);

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	WL_PS(("wl%d.%d: %s\n", wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
	/* Ignore trigger frames received during tx block period */
	if (scb_psinfo->tx_block != 0) {
		WL_PS(("wl%d.%d: %s tx blocked; ignoring PS poll\n", wlc->pub->unit,
		       WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
		return;
	}

#ifdef PROP_TXSTATUS
	/*
	Immediate:
	caseA - psq is not empty
	caseB - (psq is empty) && (vqdepth != 0) && (ta_bmp == 0)
	caseC - (psq is empty) && (vqdepth == 0) && (ta_bmp == 0)

	Asynchronous:
	caseD: (psq is empty) && (vqdepth == 0) &&  (ta_bmp != 0)
	caseE: (psq is empty) && (vqdepth != 0) &&  (ta_bmp != 0)

	Just deal with async cases here, rest should not have to change.
	*/
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
		struct pktq *pktq;
#if defined(TXQ_MUX)
		/* PS buffered pkts are on the scbq */
		pktq = wlc_scbq_txq(wlc->tx_scbq, scb);
#else
		/* PS buffered pkts are on the scb_psinfo->psq */
		pktq = &scb_psinfo->psq;
#endif /* !TXQ_MUX */
		if (!wlc_wlfc_scb_psq_chk(wlc, scb, fc, pktq)) {
			return;
		}
	}
#endif /* PROP_TXSTATUS */

	/* enable multiple ps-poll frame check */
	if (scb_psinfo->psp_flags & PS_PSP_ONRESP) {
		WL_PS(("wl%d.%d: %s. previous ps-poll frame under handling. "
			"drop new ps-poll frame\n", wlc->pub->unit,
			WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
		return;
	} else {
		scb_psinfo->psp_flags |= PS_PSP_ONRESP;
	}

#if !defined(TXQ_MUX)
	/* PS buffered pkts are on the scb_psinfo->psq */
	pktq_total = pktq_n_pkts_tot(&scb_psinfo->psq);

	/*
	 * Send a null data frame if there are no PS buffered
	 * frames on APSD non-delivery-enabled ACs (WMM/APSD 3.6.1.6).
	 */
	if (pktq_total == 0 || wlc_apps_apsd_ndelv_count(wlc, scb) == 0) {
		/* Ensure pkt is not queued on psq */
		if (wlc_sendnulldata(wlc, scb->bsscfg, &scb->ea, 0, WLF_PSDONTQ,
			PRIO_8021D_BE, wlc_apps_send_psp_response_cb, NULL) == FALSE) {
			WL_ERROR(("wl%d: %s: PS-Poll null data response failed\n",
			          wlc->pub->unit, __FUNCTION__));
			scb_psinfo->psp_pending = FALSE;
		} else
			scb_psinfo->psp_pending = TRUE;

		scb_psinfo->psp_flags &= ~PS_MORE_DATA;
	}
	/* Check if we should ignore the ps poll */
	else if (scb_psinfo->psp_pending && !SCB_ISMULTI(scb)) {
		/* Reply to a non retried PS Poll pkt after the current
		 * psp_pending has completed (if that pending pkt indicated "more
		 * data"). This aids the stalemate introduced if a STA acks a ps
		 * poll response but the AP misses that ack
		 */
		if ((scb_psinfo->psp_flags & PS_MORE_DATA) && !(fc & FC_RETRY))
			scb_psinfo->psp_flags |= PS_PSP_REQ_PEND;
	} else {
		/* Check whether there are any legacy frames before sending
		 * any delivery enabled frames
		 */
		if (wlc_apps_apsd_delv_count(wlc, scb) > 0) {
			ac_bitmap_t ac_non_delv = ~scb->apsd.ac_delv & AC_BITMAP_ALL;
			uint32 precbitmap = WLC_ACBITMAP_TO_PRECBITMAP(ac_non_delv);

			wlc_apps_ps_send(wlc, scb, precbitmap, 0);
		} else {
			wlc_apps_ps_send(wlc, scb, WLC_PREC_BMP_ALL, 0);
		}
	}

#else /* TXQ_MUX */

	/* At this point, we will be responding to the new PSPoll.
	 *
	 * Enable the scb's PS output to deliver a frame to the MUX.  Choice of output AC will be
	 * defered until the output MUX layer needs data from the SCB. A NULL pkt will be delivered
	 * at that time if there is no pending data for the SCB.
	 */
	wlc_apps_enable_ps_output(wlc, scb);

	/* make sure the MUX runs */
	wlc_send_q(wlc, SCB_WLCIFP(scb)->qi);

#endif /* TXQ_MUX */
}

#if defined(TXQ_MUX)
static void
wlc_apps_enable_ps_output(wlc_info_t *wlc, struct scb *scb)
{
	WL_TMP(("TXQ: %s clearing SCBQ_FC_PS for PSPoll resp\n", __FUNCTION__));

	/* Clear the stop flag for the SCBQ to allow delivery */
	wlc_scbq_scb_stop_flag_clear(wlc->tx_scbq, scb, SCBQ_FC_PS);
}
#endif /* TXQ_MUX */

/* get PVB info */
static INLINE void
wlc_apps_tim_pvb(apps_bss_info_t *bss_psinfo, uint8 *offset, int16 *length)
{
	uint8 n1, n2;

	n1 = (uint8)(bss_psinfo->aid_lo/8);
	/* n1 must be highest even number */
	n1 &= ~1;
	n2 = (uint8)(bss_psinfo->aid_hi/8);

	*offset = n1;
	*length = n2 - n1 + 1;

	ASSERT(*offset <= 127);
	ASSERT(*length >= 1 && *length <= 251);
}

/* calculate TIM IE length */
static uint
wlc_apps_tim_len(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	apps_bss_info_t *bss_psinfo;
	uint8 offset;
	int16 length;

	ASSERT(cfg != NULL);
	ASSERT(BSSCFG_AP(cfg));

	bss_psinfo = APPS_BSSCFG_CUBBY(wlc->psinfo, cfg);

	wlc_apps_tim_pvb(bss_psinfo, &offset, &length);

	return TLV_HDR_LEN + DOT11_MNG_TIM_FIXED_LEN + length;
}

/* Fill in the TIM element for the specified bsscfg */
static int
wlc_apps_tim_create(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint8 *buf, uint len)
{
	apps_bss_info_t *bss_psinfo;
	uint8 offset;
	int16 length;
	wlc_bss_info_t *current_bss;

	ASSERT(cfg != NULL);
	ASSERT(BSSCFG_AP(cfg));
	ASSERT(buf != NULL);

	/* perform length check to make sure tim buffer is big enough */
	if (wlc_apps_tim_len(wlc, cfg) > len)
		return BCME_BUFTOOSHORT;

	current_bss = cfg->current_bss;

	bss_psinfo = APPS_BSSCFG_CUBBY(wlc->psinfo, cfg);

	wlc_apps_tim_pvb(bss_psinfo, &offset, &length);

	buf[0] = DOT11_MNG_TIM_ID;
	/* set the length of the TIM */
	buf[1] = (uint8)(DOT11_MNG_TIM_FIXED_LEN + length);
	buf[2] = (uint8)(current_bss->dtim_period - 1);
	buf[3] = (uint8)current_bss->dtim_period;
	/* set the offset field of the TIM */
	buf[4] = offset;
	/* copy the PVB into the TIM */
	bcopy(&bss_psinfo->pvb[offset], &buf[5], length);
	return BCME_OK;
}

/* wlc_apps_scb_supr_enq()
 *
 * Re-enqueue a suppressed frame.
 *
 * This fn is similar to wlc_apps_suppr_frame_enq(), except:
 *   - handles only unicast SCB traffic
 *   - SCB is Associated and in PS
 *   - Handles the suppression of PSPoll response. wlc_apps_suppr_frame does not handle
 *     these because they would not be suppressed due to PMQ suppression since PSP response
 *     are queued while STA is PS, so ignore PMQ is set.
 *
 * Called from:
 *  wlc.c:wlc_pkt_abs_supr_enq()
 *	<- wlc_dotxstatus()
 *	<- wlc_ampdu_dotxstatus_aqm_complete()
 *	<- wlc_ampdu_dotxstatus_complete() (non-AQM)
 *
 *  wlc_ampdu_suppr_pktretry()
 *   Which is called rom wlc_ampdu_dotxstatus_aqm_complete(),
 *   similar to TX_STATUS_SUPR_PPS (Pretend PS) case in wlc_dotxstatus()
 */
bool
wlc_apps_scb_supr_enq(wlc_info_t *wlc, struct scb *scb, void *pkt)
{
	struct apps_scb_psinfo *scb_psinfo;
	apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;

	ASSERT(scb != NULL);
	ASSERT(SCB_PS(scb));
	ASSERT(!SCB_ISMULTI(scb));
	ASSERT((SCB_ASSOCIATED(scb)) || (SCB_WDS(scb) != NULL));
	ASSERT(pkt != NULL);

	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
	ASSERT(scb_psinfo);

	if (WLF2_PCB2(pkt) == WLF2_PCB2_PSP_RSP) {
		/* This packet was the ps-poll response.
		 * Clear multiple ps-poll frame protection.
		 */
		scb_psinfo->psp_flags &= ~PS_PSP_ONRESP;
#if !defined(TXQ_MUX)
		scb_psinfo->psp_pending = FALSE;
#endif
	}

	/* unregister pkt callback */
	WLF2_PCB2_UNREG(pkt);

#if !defined(TXQ_MUX)
	/* This flag is not needed for TXQ_MUX because only suppressed frame are queued
	 * to the scb's psq reorder queue. For non TXQ_MUX, both suppressed and new Tx
	 * pkts are enqueued to the psq so this flag is used to identify.
	 */
	WLPKTTAG(pkt)->flags3 |= WLF3_SUPR;
#endif /* TXQ_MUX */

	/* Mark as retrieved from HW FIFO */
	WLPKTTAG(pkt)->flags |= WLF_FIFOPKT;

	WL_PS(("wl%d:%s: SUPPRESSED packet %p "MACF" PS:%d \n",
	       wlc->pub->unit, __FUNCTION__,
	       pkt, ETHER_TO_MACF(scb->ea), SCB_PS(scb)));

#if !defined(TXQ_MUX)
	/* If enqueue to psq successfully, return FALSE so that PDU is not freed */
	/* Enqueue at higher precedence as these are suppressed packets */
	if (wlc_apps_psq(wlc, pkt, WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt)))) {
		WLPKTTAG(pkt)->flags &= ~WLF_APSD;
		return FALSE;
	}

	WL_ERROR(("wl%d: %s: ps suppr pkt discarded\n", wlc->pub->unit, __FUNCTION__));
	wlc_psinfo->ps_discard++;
	scb_psinfo->ps_discard++;

	return TRUE;

#else /* TXQ_MUX */

	/* enqueue the suppressed packet to the ps transistion queue */
	if (!wlc_prec_enq(wlc, &scb_psinfo->psq, pkt, WLC_PRIO_TO_PREC(PKTPRIO(pkt)))) {
		WL_PS(("wl%d.%d:%s(): wlc_prec_enq() failed\n", wlc->pub->unit,
			WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
		/* This should not happen because the psq should not have a limit */
		ASSERT(0);
		return TRUE;
	}

	/* packet was queued sucessfully, return FALSE so that PDU is not freed */
	return FALSE;

#endif /* TXQ_MUX */
}

/* wlc_apps_suppr_frame_enq()
 *
 * Enqueue a suppressed PDU to psq after fixing up the PDU
 *
 * Called from
 * wlc_dotxstatus():
 *   -> supr_status == TX_STATUS_SUPR_PMQ (PS)
 *   -> supr_status == TX_STATUS_SUPR_PPS (Pretend PS)
 *
 * wlc_ampdu_dotxstatus_aqm_complete()
 *   -> supr_status == TX_STATUS_SUPR_PMQ (PS)
 *
 * wlc_ampdu_dotxstatus_complete() (non-AQM)
 *   -> supr_status == TX_STATUS_SUPR_PMQ (PS)
 *
 * wlc_ampdu_suppr_pktretry()
 *   Which is called from wlc_ampdu_dotxstatus_aqm_complete(),
 *   similar to TX_STATUS_SUPR_PPS (Pretend PS) case in wlc_dotxstatus()
 */
bool
wlc_apps_suppr_frame_enq(wlc_info_t *wlc, void *pkt, tx_status_t *txs, bool last_frag)
{
	uint16 frag = 0;
	uint16 txcnt;
	uint16 seq_num = 0;
	struct scb *scb = WLPKTTAGSCBGET(pkt);
	struct apps_scb_psinfo *scb_psinfo;
#if !defined(TXQ_MUX)
	apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
#endif
	struct dot11_header *h;
	uint16 txc_hwseq;
	wlc_txh_info_t txh_info;
	bool control;

	ASSERT(scb != NULL);

	if (!SCB_PS(scb) && !SCB_ISMULTI(scb)) {
		/* Due to races in what indications are processed first, we either get
		 * a PMQ indication that a SCB has entered PS mode, or we get a PMQ
		 * suppressed packet. This is the patch where a PMQ suppressed packet is
		 * the first indication that a SCB is in PS mode.
		 * Signal the PS switch with the flag that the indication was a suppress packet.
		 */
		WL_PS(("%s: PMQ entry interrupt delayed! "MACF"\n",
		__FUNCTION__, ETHER_TO_MACF(scb->ea)));
		if (ETHER_ISBCAST(&scb->ea)) {
			/* This BCMC SCB PS process is valid only if the unicast SCB that triggered
			 * this PMQ suppression is associated or from IBSS bss.
			 */
			wlc_bsscfg_t *bsscfg = SCB_BSSCFG(scb);
			bool valid = BSSCFG_IBSS(bsscfg);

			if (!valid) {
				struct scb *scbtmp;
				struct scb_iter scbiter;
				FOREACH_BSS_SCB(wlc->scbstate, &scbiter, bsscfg, scbtmp) {
					if (SCB_ASSOCIATED(scbtmp)) {
						valid = TRUE;
					}
				}
			}
			if (valid) {
				wlc_apps_bcmc_scb_ps_on(wlc, bsscfg, TRUE);
			} else {
				WL_ERROR(("%s: Invalid BCMC PMQ suppress!\n", __FUNCTION__));
			}
		} else {
			wlc_apps_process_ps_switch(wlc, &scb->ea, PS_SWITCH_PMQ_SUPPR_PKT);
		}
	}

#if defined(TXQ_MUX)
#if defined(PROP_TXSTATUS)
	/* TXQ_MUX note: This is an early handling of the same non-MUX code (inverted logic in 'if'
	 * test) from below in this routine.
	 */
	/* in proptxstatus, the host will resend these suppressed packets */
	if (PROP_TXSTATUS_ENAB(wlc->pub) && HOST_PROPTXSTATUS_ACTIVATED(wlc) &&
	    (WL_TXSTATUS_GET_FLAGS(WLPKTTAG(pkt)->wl_hdr_information) &
	     WLFC_PKTFLAG_PKTFROMHOST)) {
		return TRUE;
	}
#endif /* PROP_TXSTATUS */
#endif /* TXQ_MUX */

	wlc_get_txh_info(wlc, pkt, &txh_info);

	h = txh_info.d11HdrPtr;
	control = FC_TYPE(ltoh16(h->fc)) == FC_TYPE_CTL;

	if (!control) {
		seq_num = ltoh16(h->seq);
		frag = seq_num & FRAGNUM_MASK;
	}

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	if (scb_psinfo == NULL)
		return FALSE;

	if (D11REV_GE(wlc->pub->corerev, 40)) {
		/* TxStatus in txheader is not needed in chips with MAC agg. */
		/* txcnt = txs->status.frag_tx_cnt << TX_STATUS_FRM_RTX_SHIFT; */
		txcnt = 0;
	}
	else {
		txcnt = txs->status.raw_bits & (TX_STATUS_FRM_RTX_MASK);
	}

	/* Is this the first suppressed frame, and either is partial
	 * MSDU or has been retried at least once, driver needs to
	 * preserve the retry count and sequence number in the PDU so that
	 * next time it is transmitted, the receiver can put it in order
	 * or discard based on txcnt. For partial MSDU, reused sequence
	 * number will allow reassembly
	 */
	if (!scb_psinfo->first_suppr_handled && (frag || txcnt) && !control) {
		/* If the seq num was hw generated then get it from the
		 * status pkt otherwise get it from the original pkt
		 */
		if (D11REV_GE(wlc->pub->corerev, 40)) {
			txc_hwseq = txh_info.MacTxControlLow & htol16(D11AC_TXC_ASEQ);
		} else {
			txc_hwseq = txh_info.MacTxControlLow & htol16(TXC_HWSEQ);
		}

		if (txc_hwseq)
			seq_num = txs->sequence;
		else
			seq_num = seq_num >> SEQNUM_SHIFT;

		h->seq = htol16((seq_num << SEQNUM_SHIFT) | (frag & FRAGNUM_MASK));

		/* Clear hwseq flag in maccontrol low */
		/* set the retry counts */
		if (D11REV_GE(wlc->pub->corerev, 40)) {
			d11actxh_t* vhtHdr = &(txh_info.hdrPtr->txd);
			vhtHdr->PktInfo.MacTxControlLow &=  ~htol16(D11AC_TXC_ASEQ);
			vhtHdr->PktInfo.TxStatus = htol16(txcnt);
		} else {
			d11txh_t* nonVHTHdr = &(txh_info.hdrPtr->d11txh);
			nonVHTHdr->MacTxControlLow &= ~htol16(TXC_HWSEQ);
			nonVHTHdr->TxStatus = htol16(txcnt);
		}

		WL_PS(("Partial MSDU PDU %p - frag:%d seq_num:%d txcnt: %d\n",
			OSL_OBFUSCATE_BUF(pkt), frag, seq_num, txcnt));
	}

	/* This ensures that all the MPDUs of the same SDU get
	 * same seq_num. This is a case when first fragment was retried
	 */
	if (last_frag || !(frag || txcnt))
		scb_psinfo->first_suppr_handled = TRUE;

	WL_PS(("wl%d:%s: SUPPRESSED packet %p - %s "MACF" PS:%d \n",
	       wlc->pub->unit, __FUNCTION__, OSL_OBFUSCATE_BUF(pkt),
	       (FC_TYPE(ltoh16(h->fc)) == FC_TYPE_DATA) ? "data" :
	       (FC_TYPE(ltoh16(h->fc)) == FC_TYPE_MNG) ? "mgmt" :
	       (FC_TYPE(ltoh16(h->fc)) == FC_TYPE_CTL) ? "ctrl" :
	       "unknown",
	       ETHER_TO_MACF(h->a1), SCB_PS(scb)));

#if !defined(TXQ_MUX)
	/* This flag is not needed for TXQ_MUX because only suppressed frame are queued
	 * to the scb's psq reorder queue. For non TXQ_MUX, both suppressed and new Tx
	 * pkts are enqueued to the psq so this flag is used to identify.
	 */
	WLPKTTAG(pkt)->flags3 |= WLF3_SUPR;
#endif /* TXQ_MUX */

	/* Mark as retrieved from HW FIFO */
	WLPKTTAG(pkt)->flags |= WLF_FIFOPKT;

#if !defined(TXQ_MUX)

	/* If in PS mode, enqueue the suppressed PDU to PSQ for ucast SCB otherwise txq */
	if (SCB_PS(scb) && !SCB_ISMULTI(scb) && !(WLPKTTAG(pkt)->flags & WLF_PSDONTQ)) {
#ifdef PROP_TXSTATUS
		/* in proptxstatus, the host will resend these suppressed packets */
		/* -memredux- dropped the packet body already anyway. */
		if (!PROP_TXSTATUS_ENAB(wlc->pub) || !HOST_PROPTXSTATUS_ACTIVATED(wlc) ||
		    !(WL_TXSTATUS_GET_FLAGS(WLPKTTAG(pkt)->wl_hdr_information) &
		      WLFC_PKTFLAG_PKTFROMHOST))
#endif
		{
			/* If enqueue to psq successfully, return FALSE so that PDU is not freed */
			/* Enqueue at higher precedence as these are suppressed packets */
			if (wlc_apps_psq(wlc, pkt, WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt))))
				return FALSE;
		}
		WL_PS(("wl%d.%d: %s: "MACF" ps suppr pkt discarded\n", wlc->pub->unit,
		       WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, ETHER_TO_MACF(scb->ea)));
		wlc_psinfo->ps_discard++;
		scb_psinfo->ps_discard++;
		return TRUE;
	}

#ifdef PROP_TXSTATUS
	if (!PROP_TXSTATUS_ENAB(wlc->pub) || !HOST_PROPTXSTATUS_ACTIVATED(wlc) ||
	    !(WL_TXSTATUS_GET_FLAGS(WLPKTTAG(pkt)->wl_hdr_information) &
	      WLFC_PKTFLAG_PKTFROMHOST) ||
	    (pktqprec_n_pkts(WLC_GET_TXQ(SCB_WLCIFP(scb)->qi),
	               WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt))) < BCMC_MAX))
#endif
	{
		if (wlc_prec_enq(wlc, WLC_GET_TXQ(SCB_WLCIFP(scb)->qi),
		                 pkt, WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt))))
			return FALSE;
	}

	/* error exit, return TRUE to have caller free packet */
	return TRUE;

#else /* TXQ_MUX */

	/* Move the packet to the appropriate queue */
	if (!SCB_ISMULTI(scb)) {
		/* Unicast SCB suppression */

		/* special handling for PSDONTQ */
		if ((WLPKTTAG(pkt)->flags & WLF_PSDONTQ)) {
			WL_PS(("wl%d:%s(): "MACF" seq %04x PSDONTQ frame dropped\n",
			       wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea),
			       seq_num));
			ASSERT(0);
			return TRUE;
		} else {
			/* Move normal buffered traffic the PS reorder queue */
			if (!wlc_prec_enq(wlc, &scb_psinfo->psq, pkt,
			                  WLC_PRIO_TO_PREC(PKTPRIO(pkt)))) {
				WL_PS(("wl%d:%s(): "MACF" seq %04x wlc_prec_enq() failed\n",
				       wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea),
				       seq_num));
				/* This should not happen because the psq should not have a limit */
				ASSERT(0);
				return TRUE;
			}
			WL_PS(("wl%d:%s(): "MACF" seq %04x queued to scb PS reorder q\n",
			       wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea),
			       seq_num));
		}
	} else {
		/* Multicast SCB suppression */
		wlc_bsscfg_t *bsscfg = scb->bsscfg;

		/* Move buffered traffic the BCMC PS reorder queue */
		if (!wlc_bcmc_suppress_enqueue(wlc, bsscfg, pkt, WLC_PRIO_TO_PREC(PKTPRIO(pkt)))) {
			WL_PS(("wl%d:%s(): BCMC seq %04x for BSSID "MACF" wlc_prec_enq() failed\n",
			       wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(bsscfg->BSSID),
			       seq_num));
			/* This should not happen because the psq should not have a limit */
			ASSERT(0);
			return TRUE;
		}
		WL_PS(("wl%d:%s(): "MACF" seq %04x queued to bcmc PS reorder q\n",
		       wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea),
		       seq_num));
	}

	/* success, return FALSE so the caller does not free the packet */
	return FALSE;

#endif /* TXQ_MUX */
}

/*
 * WLF_PSDONTQ notes
 *    wlc_pkt_abs_supr_enq() drops PSDONTQ pkts on suppress with comment
 *        "toss driver generated NULL frame"
 *    wlc_dotxstatus(): PSPRETEND avoids working on PSDONTQ frames with the comment
 *        "the flag for WLF_PSDONTQ is checked because this is used for probe packets."
 *        Also in same fn pspretend avoids psdontq frame for wlc_apps_process_pspretend_status()
 *    wlc_queue_80211_frag(): added PSDONTQ flag to non-bufferable mgmt in r490247
 *        Auth, (Re)Assoc, Probe, Bcn, ATIM
 *    wlc_queue_80211_frag(): USE the PSDONTQ flag to send to TxQ instead of scb->psq
 *    wlc_sendctl(): USE the PSDONTQ flag to send to TxQ instead of scb->psq
 *    _wlc_aibss_sendatim(): marks ATIM pks as PSDONTQ, but directly calls wlc_txfifo(). Seem like
 *        flag would only be used on suppress
 *    wlc_ap_sta_probe(): SETS for PSPretend NULL DATA, clear on AP sta probe.
 *    wlc_ap_do_pspretend_probe(): SETS for PSPretend NULL DATA
 *    _wlc_apps_ps_enq(): TDLS checks and passes to next TxMod "for TDLS PTI resp, don't enq to PSQ,
 *        send right away" odd that this fn is also called for txq_to_psq() for PS on transition,
 *        so don't think TxMod should be happening
 *    wlc_apps_ps_prep_mpdu(): checks PSDONTQ to identify final NULL DATA in pspoll
 *        chain termination
 *    wlc_apps_send_psp_response(): SETTING PSDONTQ in final NULL DATA for pspoll chain
 *    wlc_apps_suppr_frame_enq(): handling PMQ suppressed pkts, sends PSDONTQ pkts to TxQ instead
 *        of psq, just like wlc_queue_80211_frag() would have
 *    wlc_apps_apsd_eosp_send(): SETTING PSDONTQ in final NULL DATA for APSD service period
 *    wlc_apps_apsd_prepare(): ??? TDLS checks PSDONTQ in it's logic to clear eosp
 *    wlc_p2p_send_prbresp(): SETS PSDONTQ for Probe Resp
 *    wlc_probresp_send_probe_resp(): SETS PSDONTQ for Probe Resp
 *    wlc_tdls_send_pti_resp(): SETS PSDONTQ for action frame
 *    wlc_send_q(): BSS_TX_SUPR() processing, for AIBSS, does not queue PSDONTQ pkts
 *    wlc_tx_fifo_sync_complete(): (OLD! Non-NEW_TXQ!) would send any frames recovered during sync
 *        to the psq for a SCB_PS() sta if not PSDONTQ. Looks like it was soft PMQ processing
 *        at the end of sync. Seems like this would have thrown off tx_intransit counts?
 *    wlc_ap_wds_probe():  SETS for PSPretend NULL DATA, clear on AP sta probe.
 */

#ifdef PROP_TXSTATUS
/*
 * APSD Host Packet Timeout (hpkt_tmout)
 * In order to keep a U-APSD Service Period active in a Prop_TxStatus configuration,
 * a timer is used to indicate that a packet may be arriving soon from the host.
 *
 * Normally an apsd service period would end as soon as there were no more pkts queued
 * for a destination. But since there may be a lag from request to delivery of a pkt
 * from the host, the hpkt_tmout timer is set when a host pkt request is made.
 *
 * The pkt completion routine wlc_apps_apsd_complete() will normally send the next packet,
 * or end the service period if no more pkts. Instead of ending the serivice period,
 * if "apsd_hpkt_timer_on" is true, nothing is done in wlc_apps_apsd_complete(), and instead
 * this routine will end the service period if the timer expires.
 *
 * TXQ_MUX: the power save output fn implements the pkt delivery for PS and APSD.
 * The apsd_hpkt_timer_on flag will be used in that fn to hold off announcing EOSP.
 * This routine needs to un-stall the output fn when the timer expires so that
 * the output fn will be call and announce EOSP.
 */
static void
wlc_apps_apsd_hpkt_tmout(void *arg)
{
	struct apps_scb_psinfo *scb_psinfo;
	struct scb *scb;
	wlc_info_t *wlc;

	scb_psinfo = (struct apps_scb_psinfo *)arg;
	ASSERT(scb_psinfo);

	scb = scb_psinfo->scb;
	wlc = scb_psinfo->wlc;

	ASSERT(scb);
	ASSERT(wlc);

#if !defined(TXQ_MUX)
	/* send the eosp if still valid (entry to p2p abs makes apsd_usp false)
	* and no pkt in transit/waiting on pkt complete
	*/

	if (scb_psinfo->apsd_usp == TRUE && !scb_psinfo->apsd_tx_pending &&
	    (scb_psinfo->apsd_cnt > 0 || scb_psinfo->ext_qos_null)) {
		wlc_apps_apsd_send(wlc, scb);
	}
	scb_psinfo->apsd_hpkt_timer_on = FALSE;
#else /* TXQ_MUX */
	/* Clear the timer flag to indicate no more packets are on the way from the host,
	 * or at least we have waited long enough, and un-stall the mux source so that
	 * it will end the service period.
	 */
	scb_psinfo->apsd_hpkt_timer_on = FALSE;

	/* If the service period is still active, and there are no more pkts queued,
	 * end the service period. This is done by un-stalling the highest prio
	 * that is delivery enabled. When the output fn is called to supply pkts,
	 * it will annoucne ESOP if there are no more delivery pkts available.
	 */
	if (scb_psinfo->apsd_usp == TRUE && wlc_apps_apsd_delv_count(wlc, scb) == 0) {
		/* WME/802.1E Access Category Idx to TX FIFO number */
		const uint8 wme_aci2fifo[] = {
			TX_AC_BE_FIFO, TX_AC_BK_FIFO, TX_AC_VI_FIFO, TX_AC_VO_FIFO
		};
		uint max_aci;

		max_aci = wlc_apps_apsd_max_delv_aci(scb->apsd.ac_delv);
		wlc_scbq_scb_stall_clear(wlc->tx_scbq, scb, wme_aci2fifo[max_aci]);

		/* kick the queue associated with this scb */
		wlc_send_q(wlc, SCB_WLCIFP(scb)->qi);
	}
#endif /* TXQ_MUX */

}
#endif /* PROP_TXSTATUS */

#if !defined(TXQ_MUX)
static void
wlc_apps_apsd_send(wlc_info_t *wlc, struct scb *scb)
{
	struct apps_scb_psinfo *scb_psinfo;
	uint prec_bmp;

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);
	ASSERT(scb_psinfo->apsd_cnt > 0 || scb_psinfo->ext_qos_null);

	/*
	 * If there are no buffered frames, send a QoS Null on the highest delivery-enabled AC
	 * (which AC to use is not specified by WMM/APSD).
	 */
	if (scb_psinfo->ext_qos_null ||
	    wlc_apps_apsd_delv_count(wlc, scb) == 0) {
#ifdef WLTDLS
		if (BSS_TDLS_ENAB(wlc, scb->bsscfg) &&
		    wlc_tdls_in_pti_interval(wlc->tdls, scb)) {
			return;
		}
#endif /* WLTDLS */
		wlc_apps_apsd_eosp_send(wlc, scb);
		return;
	}

	prec_bmp = wlc_apps_ac2precbmp_info(scb->apsd.ac_delv);

#ifdef PROP_TXSTATUS
	/* Continuous pkt flow till last packet is is needed for Wi-Fi P2P 6.1.12/6.1.13.
	 * by fetching pkts from host one after another
	 * and wait till either timer expires or new packet is received
	 */
	if (PROP_TXSTATUS_ENAB(wlc->pub) && HOST_PROPTXSTATUS_ACTIVATED(wlc)) {
		if (!scb_psinfo->apsd_hpkt_timer_on &&
		    scb_psinfo->apsd_cnt > 1 &&
		    wlc_apps_apsd_delv_count(wlc, scb) == 1) {
			wlc_wlfc_psmode_request(wlc->wlfc, scb,
				1, prec_bmp, WLFC_CTL_TYPE_MAC_REQUEST_PACKET);
			wl_add_timer(wlc->wl, scb_psinfo->apsd_hpkt_timer,
				WLC_PS_APSD_HPKT_TIME, FALSE);
			scb_psinfo->apsd_hpkt_timer_on = TRUE;
			return;
		}
	}
#endif /* PROP_TXSTATUS */
	/*
	 * Send a delivery frame.  When the frame goes out, the wlc_apps_apsd_complete()
	 * callback will attempt to send the next delivery frame.
	 */
	if (!wlc_apps_ps_send(wlc, scb, prec_bmp, WLF_APSD))
		wlc_apps_apsd_usp_end(wlc, scb);

}

#else  /* TXQ_MUX */

static void
wlc_apps_apsd_send(wlc_info_t *wlc, struct scb *scb)
{
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(scb);
	return;
}

#endif /* !TXQ_MUX */

#ifdef WLTDLS
#if !defined(TXQ_MUX)
void
wlc_apps_apsd_tdls_send(wlc_info_t *wlc, struct scb *scb)
{
	struct apps_scb_psinfo *scb_psinfo;

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	if (BSS_TDLS_ENAB(wlc, scb->bsscfg)) {
		if (!scb_psinfo->apsd_usp)
			return;

		scb_psinfo->apsd_cnt = wlc_apps_apsd_delv_count(wlc, scb);

		if (scb_psinfo->apsd_cnt)
			wlc_apps_apsd_send(wlc, scb);
		else
			wlc_apps_apsd_eosp_send(wlc, scb);
	}
	return;
}
#else
void
wlc_apps_apsd_tdls_send(wlc_info_t *wlc, struct scb *scb)
{
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(scb);
	return;
}
#endif /* !TXQ_MUX */
#endif /* WLTDLS */

#if !defined(TXQ_MUX)
static const uint8 apsd_delv_acbmp2maxprio[] = {
	PRIO_8021D_BE, PRIO_8021D_BE, PRIO_8021D_BK, PRIO_8021D_BK,
	PRIO_8021D_VI, PRIO_8021D_VI, PRIO_8021D_VI, PRIO_8021D_VI,
	PRIO_8021D_NC, PRIO_8021D_NC, PRIO_8021D_NC, PRIO_8021D_NC,
	PRIO_8021D_NC, PRIO_8021D_NC, PRIO_8021D_NC, PRIO_8021D_NC
};
#endif /* !TXQ_MUX */

/* Send frames in a USP, called in response to receiving a trigger frame */
void
wlc_apps_apsd_trigger(wlc_info_t *wlc, struct scb *scb, int ac)
{
	struct apps_scb_psinfo *scb_psinfo;
#if defined(TXQ_MUX)
	/* WME/802.1E Access Category Idx to TX FIFO number */
	const uint8 wme_aci2fifo[] = {
		TX_AC_BE_FIFO, TX_AC_BK_FIFO, TX_AC_VI_FIFO, TX_AC_VO_FIFO
	};
	uint aci;
#endif /* TXQ_MUX */

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	/* Ignore trigger frames received during tx block period */
	if (scb_psinfo->tx_block != 0) {
		WL_PS(("wl%d.%d: wlc_apps_apsd_trigger: tx blocked; ignoring trigger\n",
		       wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb))));
		return;
	}

	/* Ignore trigger frames received during an existing USP */
	if (scb_psinfo->apsd_usp) {
		WL_PS(("wl%d.%d: wlc_apps_apsd_trigger: already in USP; ignoring trigger\n",
		       wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb))));
		return;
	}

	WL_PS(("wl%d.%d: wlc_apps_apsd_trigger: ac %d buffered %d delv %d\n",
	       wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), ac,
	       pktqprec_n_pkts(&scb_psinfo->psq, ac), wlc_apps_apsd_delv_count(wlc, scb)));

	scb_psinfo->apsd_usp = TRUE;

	/* initialize the delivery count for this SP */
	scb_psinfo->apsd_cnt = scb->apsd.maxsplen;

#if !defined(TXQ_MUX)
	/*
	 * Send the first delivery frame.  Subsequent delivery frames will be sent by the
	 * completion callback of each previous frame.  This is not very efficient, but if
	 * we were to queue a bunch of frames to different FIFOs, there would be no
	 * guarantee that the MAC would send the EOSP last.
	 */

	wlc_apps_apsd_send(wlc, scb);
#else /* TXQ_MUX */
	/* Un-Stall the delivery enabled queues to allow the APSD output fn
	 * manage the delivery of queued frames.
	 */
	for (aci = 0; aci < AC_COUNT; aci++) {
		if (isbitset(scb->apsd.ac_delv, aci)) {
			wlc_scbq_scb_stall_clear(wlc->tx_scbq, scb, wme_aci2fifo[aci]);
		}
	}

	/* kick the queue associated with this scb */
	wlc_send_q(wlc, SCB_WLCIFP(scb)->qi);
#endif /* TXQ_MUX */

}

#if !defined(TXQ_MUX)
// TXQ_MUX will use APPS output fn send and queue the eosp frame
static void
wlc_apps_apsd_eosp_send(wlc_info_t *wlc, struct scb *scb)
{
	int prio = (int)apsd_delv_acbmp2maxprio[scb->apsd.ac_delv & 0xf];
	struct apps_scb_psinfo *scb_psinfo;

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	WL_PS(("wl%d.%d: wlc_apps_apsd_send: sending QoS Null prio=%d\n",
	       wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), prio));

	scb_psinfo->ext_qos_null = FALSE;
	scb_psinfo->apsd_cnt = 0;

	if (wlc_sendnulldata(wlc, scb->bsscfg, &scb->ea, 0,
		(WLF_PSDONTQ | WLF_APSD), prio, NULL, NULL) == FALSE) {
		WL_ERROR(("wl%d: %s: could not send QoS Null\n",
		          wlc->pub->unit, __FUNCTION__));
		wlc_apps_apsd_usp_end(wlc, scb);
	}

	/* just reset the apsd_uspflag, don't update the apsd_endtime to allow TDLS PTI */
	/* to send immediately for the first packet */
	if (BSS_TDLS_ENAB(wlc, scb->bsscfg))
		wlc_apps_apsd_usp_end(wlc, scb);
}
#endif /* TXQ_MUX */

/* Make decision if we need to count MMPDU in SP */
static bool
wlc_apps_apsd_count_mmpdu_in_sp(wlc_info_t *wlc, struct scb *scb, void *pkt)
{
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(scb);
	BCM_REFERENCE(pkt);

	return TRUE;
}

/* wlc_apps_apsd_prepare()
 *
 * Keeps track of APSD info as pkts are prepared for TX. Updates the apsd_cnt (number of pkts
 * sent during a Service Period) and marks the EOSP bit on the pkt or calls for
 * a null data frame to do the same (ext_qos_null) if the end of the SP has been reached.
 * TXQ_MUX Note: Keeping the fn as-is for TXQ_MUX, but may need to be adjsted for use in
 * the output fn.
 */
void
wlc_apps_apsd_prepare(wlc_info_t *wlc, struct scb *scb, void *pkt,
                      struct dot11_header *h, bool last_frag)
{
	struct apps_scb_psinfo *scb_psinfo;
	uint16 *pqos;
	bool qos;
	bool more = FALSE;
	bool eosp = FALSE;

	/* The packet must have 802.11 header */
	ASSERT(WLPKTTAG(pkt)->flags & WLF_MPDU);

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	/* Set MoreData if there are still buffered delivery frames */
	if (wlc_apps_apsd_delv_count(wlc, scb) > 0)
		h->fc |= htol16(FC_MOREDATA);
	else
		h->fc &= ~htol16(FC_MOREDATA);

	qos = ((ltoh16(h->fc) & FC_KIND_MASK) == FC_QOS_DATA) ||
	      ((ltoh16(h->fc) & FC_KIND_MASK) == FC_QOS_NULL);

	/* SP countdown */
	if (last_frag &&
	    (qos || wlc_apps_apsd_count_mmpdu_in_sp(wlc, scb, pkt))) {
		/* Indicate EOSP when this is the last MSDU in the psq */
		/* JQL: should we keep going in case there are on-the-fly
		 * MSDUs and let the completion callback to check if there is
		 * any other buffered MSDUs then and indicate the EOSP using
		 * an extra QoS NULL frame?
		 */
		more = (ltoh16(h->fc) & FC_MOREDATA) != 0;
		if (!more)
			scb_psinfo->apsd_cnt = 1;
		/* Decrement count of packets left in service period */
		if (scb_psinfo->apsd_cnt != WLC_APSD_USP_UNB)
			scb_psinfo->apsd_cnt--;
	}

	/* SP termination */
	if (qos) {
		pqos = (uint16 *)((uint8 *)h +
			(SCB_A4_DATA(scb) ? DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN));
		ASSERT(ISALIGNED(pqos, sizeof(*pqos)));

		/* Set EOSP if this is the last frame in the Service Period */
#ifdef WLTDLS
		/* Trigger frames are delivered in PTI interval, because
		 * the the PTI response frame triggers the delivery of buffered
		 * frames before PTI response is processed by TDLS module.
		 * QOS null frames have WLF_PSDONTQ, why should they not terminate
		 * an SP?
		 */
		if (BSS_TDLS_ENAB(wlc, scb->bsscfg) &&
			(wlc_tdls_in_pti_interval(wlc->tdls, scb) ||
			(SCB_PS(scb) && (WLPKTTAG(pkt)->flags & WLF_PSDONTQ) &&
			(scb_psinfo->apsd_cnt != 0)	&&
			((ltoh16(h->fc) & FC_KIND_MASK) != FC_QOS_NULL)))) {
			eosp = FALSE;
		}
		else
#endif
		eosp = scb_psinfo->apsd_cnt == 0 && last_frag;
		if (eosp)
			*pqos |= htol16(QOS_EOSP_MASK);
		else
			*pqos &= ~htol16(QOS_EOSP_MASK);
	}
	/* Send an extra QoS Null to terminate the USP in case
	 * the MSDU doesn't have a EOSP field i.e. MMPDU.
	 */
	else if (scb_psinfo->apsd_cnt == 0)
		scb_psinfo->ext_qos_null = TRUE;

	/* Register callback to end service period after this frame goes out */
	if (last_frag) {
		WLF2_PCB2_REG(pkt, WLF2_PCB2_APSD);
	}

	WL_PS(("wl%d.%d: %s: pkt %p qos %d more %d eosp %d cnt %d lastfrag %d\n",
	       wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
		OSL_OBFUSCATE_BUF(pkt), qos, more, eosp,
	       scb_psinfo->apsd_cnt, last_frag));
}

/* End the USP when the EOSP has gone out
 * Pkt callback fn
 */
static void
wlc_apps_apsd_complete(wlc_info_t *wlc, void *pkt, uint txs)
{
	struct scb *scb;
	struct apps_scb_psinfo *scb_psinfo;

#ifdef BCMDBG
	/* What to do if not ack'd?  Don't want to hang in USP forever... */
	if (txs & TX_STATUS_ACK_RCV)
		WL_PS(("%s():wl%d: delivery frame %p sent\n",
			__FUNCTION__, wlc->pub->unit, OSL_OBFUSCATE_BUF(pkt)));
	else
		WL_PS(("%s():wl%d: delivery frame %p sent (no ACK)\n",
			__FUNCTION__, wlc->pub->unit, OSL_OBFUSCATE_BUF(pkt)));
#else

	BCM_REFERENCE(txs);

#endif

	/* Is this scb still around */
	if ((scb = WLPKTTAGSCBGET(pkt)) == NULL) {
		WL_ERROR(("%s(): scb = %p, WLPKTTAGSCBGET(pkt) = %p\n",
			__FUNCTION__, OSL_OBFUSCATE_BUF(scb),
			OSL_OBFUSCATE_BUF(WLPKTTAGSCBGET(pkt))));
		return;
	}

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

#ifdef PROP_TXSTATUS
	scb_psinfo->apsd_tx_pending = FALSE;
#endif
	/* Send more frames until the End Of Service Period */
	if (scb_psinfo->apsd_cnt > 0 || scb_psinfo->ext_qos_null) {
		if (scb_psinfo->tx_block != 0) {
			WL_PS(("wl%d.%d: %s: tx blocked, cnt %u\n",
			       wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
			       __FUNCTION__, scb_psinfo->apsd_cnt));
			return;
		}
#ifdef PROP_TXSTATUS
		if (!scb_psinfo->apsd_hpkt_timer_on)
#endif
			wlc_apps_apsd_send(wlc, scb);
		return;
	}

	wlc_apps_apsd_usp_end(wlc, scb);
}

void
wlc_apps_scb_tx_block(wlc_info_t *wlc, struct scb *scb, uint reason, bool block)
{
	struct apps_scb_psinfo *scb_psinfo;

	ASSERT(scb != NULL);

	WL_PS(("wl%d.%d: %s: block %d reason %d\n", wlc->pub->unit,
	       WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, block, reason));

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	if (block) {
		mboolset(scb_psinfo->tx_block, reason);
		/* terminate the APSD USP */
		scb_psinfo->apsd_usp = FALSE;
		scb_psinfo->apsd_cnt = 0;
#ifdef PROP_TXSTATUS
		scb_psinfo->apsd_tx_pending = FALSE;
#endif
#if defined(TXQ_MUX)
		/* Stop the SCBQ to prevent any more tx traffic */
		wlc_scbq_scb_stop_flag_set(wlc->tx_scbq, scb, SCBQ_FC_PS);
#endif /* TXQ_MUX */
	} else {
		mboolclr(scb_psinfo->tx_block, reason);
	}
}

int
wlc_apps_scb_apsd_cnt(wlc_info_t *wlc, struct scb *scb)
{
	struct apps_scb_psinfo *scb_psinfo;

	ASSERT(scb != NULL);

	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	WL_PS(("wl%d.%d: %s: apsd_cnt = %d\n", wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
	       __FUNCTION__, scb_psinfo->apsd_cnt));

	return scb_psinfo->apsd_cnt;
}

#if defined(TXQ_MUX) && defined(PROP_TXSTATUS)
/*
 * Return the max ACI in the given bitmap of delivery enabled ACIs
 */
static uint
wlc_apps_apsd_max_delv_aci(uint8 delv_bmp)
{
	uint max_aci;

	if (AC_BITMAP_TST(delv_bmp, AC_VO))
		max_aci = AC_VO;
	else if (AC_BITMAP_TST(delv_bmp, AC_VI))
		max_aci = AC_VI;
	else if (AC_BITMAP_TST(delv_bmp, AC_BE))
		max_aci = AC_BE;
	else
		max_aci = AC_BK;

	return max_aci;
}
#endif /* TXQ_MUX && PROP_TXSTATUS */

/*
 * Return the number of frames pending on delivery-enabled ACs.
 */
static int
wlc_apps_apsd_delv_count(wlc_info_t *wlc, struct scb *scb)
{
#if !defined(TXQ_MUX)
	struct apps_scb_psinfo *scb_psinfo;
#endif /* !TXQ_MUX */
	uint32 precbitmap;
	int delv_count;

	if (scb->apsd.ac_delv == AC_BITMAP_NONE)
		return 0;

	precbitmap = wlc_apps_ac2precbmp_info(scb->apsd.ac_delv);

#if defined(TXQ_MUX)
	/* For TXQ_MUX All PS buffered pkts are on the scbq */
	delv_count = pktq_mlen(wlc_scbq_txq(wlc->tx_scbq, scb), precbitmap);
#else
	/* Non-TXQ_MUX, PS buffered pkts are on the scb_psinfo->psq */
	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	delv_count = pktq_mlen(&scb_psinfo->psq, precbitmap);
#endif /* TXQ_MUX */

	return delv_count;
}

/*
 * Return the number of frames pending on non-delivery-enabled ACs.
 */
static int
wlc_apps_apsd_ndelv_count(wlc_info_t *wlc, struct scb *scb)
{
#if !defined(TXQ_MUX)
	struct apps_scb_psinfo *scb_psinfo;
#endif /* !TXQ_MUX */
	ac_bitmap_t ac_non_delv;
	uint32 precbitmap;
	int count;

	if (scb->apsd.ac_delv == AC_BITMAP_ALL)
		return 0;

	ac_non_delv = ~scb->apsd.ac_delv & AC_BITMAP_ALL;
	precbitmap = wlc_apps_ac2precbmp_info(ac_non_delv);

#if defined(TXQ_MUX)
	/* For TXQ_MUX All PS buffered pkts are on the scbq */
	count = pktq_mlen(wlc_scbq_txq(wlc->tx_scbq, scb), precbitmap);
#else
	/* Non-TXQ_MUX, PS buffered pkts are on the scb_psinfo->psq */
	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	count = pktq_mlen(&scb_psinfo->psq, precbitmap);
#endif /* TXQ_MUX */

	return count;
}

uint8
wlc_apps_apsd_ac_available(wlc_info_t *wlc, struct scb *scb)
{
#if !defined(TXQ_MUX)
	struct apps_scb_psinfo *scb_psinfo;
#endif /* !TXQ_MUX */
	uint32 precbitmap;
	uint8 ac_bitmap = 0;
	struct pktq* q;

#if defined(TXQ_MUX)
	/* For TXQ_MUX All PS buffered pkts are on the scbq */
	q = wlc_scbq_txq(wlc->tx_scbq, scb);
#else
	/* Non-TXQ_MUX, PS buffered pkts are on the scb_psinfo->psq */
	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	q = &scb_psinfo->psq;
#endif /* TXQ_MUX */

	precbitmap = wlc_apps_ac2precbmp_info(WLC_PREC_BMP_AC_BK);
	if (pktq_mlen(q, precbitmap))
		ac_bitmap |= TDLS_PU_BUFFER_STATUS_AC_BK;

	precbitmap = wlc_apps_ac2precbmp_info(WLC_PREC_BMP_AC_BE);
	if (pktq_mlen(q, precbitmap))
		ac_bitmap |= TDLS_PU_BUFFER_STATUS_AC_BE;

	precbitmap = wlc_apps_ac2precbmp_info(WLC_PREC_BMP_AC_VI);
	if (pktq_mlen(q, precbitmap))
		ac_bitmap |= TDLS_PU_BUFFER_STATUS_AC_VO;

	precbitmap = wlc_apps_ac2precbmp_info(WLC_PREC_BMP_AC_VO);
	if (pktq_mlen(q, precbitmap))
		ac_bitmap |= TDLS_PU_BUFFER_STATUS_AC_VI;

	return ac_bitmap;
}

#if defined(MBSS)

#if !defined(TXQ_MUX)
/* TXQ_MUX always has bcmc traffic go to the BCMC queue, and special handling implemented
 * by wlc_apps_bcmc_ps_enqueue() is not needed
 */

/* Enqueue a BC/MC packet onto it's BSS's PS queue */
int
wlc_apps_bcmc_ps_enqueue(wlc_info_t *wlc, struct scb *bcmc_scb, void *pkt)
{
	struct apps_scb_psinfo *scb_psinfo;
	apps_bss_info_t *bss_info;

	scb_psinfo = SCB_PSINFO(wlc->psinfo, bcmc_scb);
	ASSERT(scb_psinfo);
	bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, bcmc_scb->bsscfg);

	/* Check that packet queue length is not exceeded */
	if (pktq_full(&scb_psinfo->psq) || pktqprec_full(&scb_psinfo->psq, MAXPRIO)) {
		WL_NONE(("wlc_apps_bcmc_ps_enqueue: queue full.\n"));
		WLCNTINCR(bss_info->bcmc_discards);
		return BCME_ERROR;
	}
	(void)pktq_penq(&scb_psinfo->psq, MAXPRIO, pkt);
	WLCNTINCR(bss_info->bcmc_pkts_seen);

	return BCME_OK;
}
#endif /* TXQ_MUX */

/* Last STA has gone out of PS.  Check state of its BSS */

static void
wlc_apps_bss_ps_off_start(wlc_info_t *wlc, struct scb *bcmc_scb)
{
	wlc_bsscfg_t *bsscfg;

	bsscfg = bcmc_scb->bsscfg;
	ASSERT(bsscfg != NULL);

	if (!BCMC_PKTS_QUEUED(bsscfg)) {
		/* No pkts in BCMC fifo */
		wlc_apps_bss_ps_off_done(wlc, bsscfg);
	} else { /* Mark in transition */
		ASSERT(bcmc_scb->PS); /* Should only have BCMC pkts if in PS */
		bsscfg->flags |= WLC_BSSCFG_PS_OFF_TRANS;
		WL_PS(("wl%d.%d: START PS-OFF. last fid 0x%x. shm fid 0x%x\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), bsscfg->bcmc_fid,
			bsscfg->bcmc_fid_shm));
#if defined(BCMDBG_MBSS_PROFILE) /* Start transition timing */
		if (bsscfg->ps_start_us == 0) {
			bsscfg->ps_start_us = R_REG(wlc->osh, &wlc->regs->tsf_timerlow);
		}
#endif /* BCMDBG_MBSS_PROFILE */
	}
}

#if defined(MBSS)
/*
 * Due to a STA transitioning to PS on, all packets have been drained from the
 * data fifos.  Update PS state of all BSSs (if not in PS-OFF transition).
 *
 * Note that it's possible that a STA has come out of PS mode during the
 * transition, so we may return to PS-OFF (abort the transition).  Since we
 * don't keep state of which STA and which BSS started the transition, we
 * simply check them all.
 */

void
wlc_apps_bss_ps_on_done(wlc_info_t *wlc)
{
	wlc_bsscfg_t *bsscfg;
	struct scb *bcmc_scb;
	int i;

	FOREACH_UP_AP(wlc, i, bsscfg) {
		if (!(bsscfg->flags & WLC_BSSCFG_PS_OFF_TRANS)) { /* Ignore BSS in PS-OFF trans */
			bcmc_scb = WLC_BCMCSCB_GET(wlc, bsscfg);
			if (BSS_PS_NODES(wlc->psinfo,  bsscfg) != 0) {
#if defined(MBSS)
				if (!SCB_PS(bcmc_scb) && MBSS_ENAB(wlc->pub)) {
					/* PS off, MC pkts to data fifo should be cleared */
					ASSERT(wlc_mbss_get_bcmc_pkts_sent(wlc, bsscfg) == 0);
					wlc_mbss_increment_ps_trans_cnt(wlc, bsscfg);
					WL_NONE(("wl%d.%d: DONE PS-ON\n",
						wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
				}
#endif
				bcmc_scb->PS = TRUE;
			} else { /* Unaffected BSS or transition aborted for this BSS */
				bcmc_scb->PS = FALSE;
#ifdef PSPRETEND
				bcmc_scb->ps_pretend &= ~PS_PRETEND_ON;
#endif /* PSPRETEND */
			}
#if defined(TXQ_MUX)
			/* reconfigure the bcmc MUXs for current PS mode */
			wlc_bcmc_start_mux_sources(bsscfg);
#endif /* TXQ_MUX */
		}
	}
}

/*
 * Last STA for a BSS exitted PS; BSS has no pkts in BC/MC fifo.
 * Check whether other stations have entered PS since and update
 * state accordingly.
 *
 * That is, it is possible that the BSS state will remain PS
 * TRUE (PS delivery mode enabled) if a STA has changed to PS-ON
 * since the start of the PS-OFF transition.
 */

void
wlc_apps_bss_ps_off_done(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	struct scb *bcmc_scb;

	ASSERT(bsscfg->bcmc_fid_shm == INVALIDFID);
	ASSERT(bsscfg->bcmc_fid == INVALIDFID);

	bcmc_scb = WLC_BCMCSCB_GET(wlc, bsscfg);
	ASSERT(SCB_PS(bcmc_scb));

	if (BSS_PS_NODES(wlc->psinfo, bsscfg) != 0) {
		/* Aborted transtion:  Set PS delivery mode */
		bcmc_scb->PS = TRUE;
	} else { /* Completed transition: Clear PS delivery mode */
		bcmc_scb->PS = FALSE;
#ifdef PSPRETEND
		bcmc_scb->ps_pretend &= ~PS_PRETEND_ON;
#endif /* PSPRETEND */

#ifdef MBSS
		wlc_mbss_increment_ps_trans_cnt(wlc, bsscfg);
#endif
		if (bsscfg->flags & WLC_BSSCFG_PS_OFF_TRANS) {
			WL_PS(("wl%d.%d: DONE PS-OFF.\n", wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
		}
	}
	bsscfg->flags &= ~WLC_BSSCFG_PS_OFF_TRANS; /* Clear transition flag */

#if defined(TXQ_MUX)
	/* Update the bcmc scb delivery configuration */
	wlc_bcmc_start_mux_sources(bsscfg);
#else

	/* Forward any packets in MC-PSQ according to new state */
	while (wlc_apps_ps_send(wlc, bcmc_scb, WLC_PREC_BMP_ALL, 0))
		/* Repeat until queue empty */
		;
#endif /* TXQ_MUX */

#if defined(BCMDBG_MBSS_PROFILE)
	if (bsscfg->ps_start_us != 0) {
		uint32 diff_us;

		diff_us = R_REG(wlc->osh, &wlc->regs->tsf_timerlow) - bsscfg->ps_start_us;
		if (diff_us > bsscfg->max_ps_off_us) bsscfg->max_ps_off_us = diff_us;
		bsscfg->tot_ps_off_us += diff_us;
		bsscfg->ps_off_count++;
		bsscfg->ps_start_us = 0;
	}
#endif /* BCMDBG_MBSS_PROFILE */
}

#endif /* MBSS */
#endif /* MBSS */

/*
 * Return the bitmap of ACs with buffered traffic.
 */
uint8
wlc_apps_apsd_ac_buffer_status(wlc_info_t *wlc, struct scb *scb)
{
#if !defined(TXQ_MUX)
	struct apps_scb_psinfo *scb_psinfo;
#endif /* !TXQ_MUX */
	uint32 precbitmap;
	uint8 ac_bitmap = 0;
	struct pktq* q;
	int i;

#if defined(TXQ_MUX)
	/* For TXQ_MUX All PS buffered pkts are on the scbq */
	q = wlc_scbq_txq(wlc->tx_scbq, scb);
#else
	/* Non-TXQ_MUX, PS buffered pkts are on the scb_psinfo->psq */
	scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);

	q = &scb_psinfo->psq;
#endif /* TXQ_MUX */

	for (i = 0; i < AC_COUNT; i++) {
		precbitmap = wlc_apps_ac2precbmp_info((1 << i));

		if (pktq_mlen(q, precbitmap))
			AC_BITMAP_SET(ac_bitmap, i);
	}

	return ac_bitmap;
}

#ifdef PKTQ_LOG
struct pktq* wlc_apps_prec_pktq(wlc_info_t *wlc, struct scb *scb)
{
	struct pktq *q;
	apps_wlc_psinfo_t *wlc_psinfo;
	struct apps_scb_psinfo *scb_psinfo;

	wlc_psinfo = wlc->psinfo;
	scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
	if (scb_psinfo == NULL)
		return NULL;
	q = &scb_psinfo->psq;

	return q;
}
#endif

/* TIM */
static uint
wlc_apps_calc_tim_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_bsscfg_t *cfg = data->cfg;

	if (BSSCFG_AP(cfg))
		return wlc_apps_tim_len(wlc, cfg);

	return 0;
}

static int
wlc_apps_write_tim_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_bsscfg_t *cfg = data->cfg;

	if (BSSCFG_AP(cfg)) {
		wlc_apps_tim_create(wlc, cfg, data->buf, data->buf_len);

		data->cbparm->ft->bcn.tim_ie = data->buf;

#ifdef MBSS
		if (MBSS_ENAB(wlc->pub)) {
			wlc_mbss_set_bcn_tim_ie(wlc, cfg, data->buf);
		}
#endif /* MBSS */
	}

	return BCME_OK;
}

static void
wlc_apps_scb_state_upd_cb(void *ctx, scb_state_upd_data_t *notif_data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	struct scb *scb;
	uint8 oldstate;

	ASSERT(notif_data != NULL);

	scb = notif_data->scb;
	ASSERT(scb != NULL);
	oldstate = notif_data->oldstate;

	if (BSSCFG_AP(scb->bsscfg) && (oldstate & ASSOCIATED) && !SCB_ASSOCIATED(scb)) {
		struct apps_bss_info *bss_info;
		WL_PS(("%s: pvb update needed as SCB is disassociated\n", __FUNCTION__));
		wlc_apps_pvb_update(wlc, scb);
		bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, SCB_BSSCFG(scb));
		if (bss_info && (bss_info->ps_trans_status & BSS_PS_TRANS_OFF_BLOCKED)) {
			WL_ERROR(("wl%d.%d: "MACF" disassociated. bcmc_scb PS off blocked. PS %d\n",
				wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
				ETHER_TO_MACF(scb->ea), SCB_PS(scb)));
		}
	}

}

void
wlc_apps_ps_trans_upd(wlc_info_t *wlc, struct scb *scb)
{
	struct apps_scb_psinfo *scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
	ASSERT(scb_psinfo);
	ASSERT(wlc->block_datafifo & DATA_BLOCK_PS);
	/* We have received an ack in pretend state and are free to exit */
	WL_PS(("wl%d.%d: "MACF" received successful txstatus in threshold "
	       "ps pretend active state\n",
	       wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
	       ETHER_TO_MACF(scb->ea)));
	scb_psinfo->ps_trans_status |= SCB_PS_TRANS_OFF_PEND;
}

#if defined(TXQ_MUX)

/**
 * @brief SCBQ's PS Tx MUX output function
 *
 * SCBQ's Tx MUX output function. This function is registered via a call to wlc_mux_add_member() by
 * wlc_scbq_cubby_init(). This function will supply the packets to the caller from packets held
 * in the SCBQ's per-SCB queues. The packets to output are collected in the provided 'output_q'.
 *
 * This function returns an estimate of the total TX time of the packets added to the output_q.
 * The supplied time may exceed the requested time.
 *
 * @param ctx           a pointer to the scbq_cubby_info_t
 * @param ac            Access Category traffic being requested
 * @param request_time  the total estimated TX time the caller is requesting from the SCB
 * @param output_q      pointer to a packet queue to use to store the output packets
 * @return              The estimated TX time of the packets returned in the output_q. The time
 *                      returned may be greater than the 'request_time'
 */
uint
wlc_scbq_ps_output(void *ctx, uint ac, uint request_time, struct spktq *output_q)
{
	struct scb *scb = (struct scb*)ctx;
	wlc_info_t *wlc = scb->bsscfg->wlc;
	struct pktq* scbq;
	uint supplied_time = 0;
	uint16 prec_map;
	int i;

	WL_PS(("%s: "MACF" output request ac %d\n", __FUNCTION__,
	       ETHER_TO_MACF(scb->ea), ac));

	/* If we are being called, the flowcontrol for PS (SCBQ_FC_PS) must be clear indicating
	 * that there has been a PSPoll request. Supply a packet (one Bufferable Unit per the
	 * 802.11-2012 spec) to satisfy the request, or a NULL Data if we do not have any pkts.
	 */

	/* This fn is called for a particular AC.
	 * If our AC is the highest AC with packets, prepare the response from this AC.
	 * If there are no pks at all, then if we are the highest AC, prepare the
	 * NULL Data response.
	 * Otherwise, do not supply any packets.
	 */
	scbq = wlc_scbq_txq(wlc->tx_scbq, scb);

	for (prec_map = WLC_PREC_BMP_AC_VO, i = TX_AC_VO_FIFO;
	     i >= TX_AC_BK_FIFO;
	     i--, prec_map >>= 4) {
		WL_PS(("%s: prec_map 0x%04x i %d qlen %d\n", __FUNCTION__,
		       prec_map, i, pktq_mlen(scbq, prec_map)));

		if (pktq_mlen(scbq, prec_map) > 0) {
			/* break out with i being the highest ac with pkts */
			break;
		}
	}

	/* 'i' should now be the highest AC with packets.
	 * (AC in precedence order numbering, BK = 0, VO = 3)
	 * 'i' will be -1 if there are no packets at any AC
	 */

	if ((uint)i == ac) {
		void* pkt;
		struct spktq fill_q;

		/* this is the highest AC with packets, prepare a response */

		/* init a queue to receive pkts from the scbq */
		spktq_init(&fill_q, PKTQ_LEN_MAX);

		/* make a request to the configured output fn, only change the requested time
		 * to special 0us to get only one pkt.
		 */
		supplied_time = wlc_scbq_unfiltered_output(wlc->tx_scbq, scb, ac, 0, &fill_q);

		/* there should be one pkt output and a non-zero supplied time */
		ASSERT(spktq_n_pkts(&fill_q) == 1);
		ASSERT(supplied_time > 0);

		WL_PS(("wl%d:%s: sending PSPoll response to "MACF" ac %u supplied "
		       "time %u count %d\n",
		       wlc->pub->unit, __FUNCTION__,
		       ETHER_TO_MACF(scb->ea), ac, supplied_time, spktq_n_pkts(&fill_q)));

		// processing here modled on wlc_apps_ps_send()


		/* Register WLF2_PCB2_PSP_RSP for BU
		 * In case of fragmentation, this is registered for the last frag
		 * at the tail of the queue.
		 */
		pkt = spktq_deq(&fill_q);
		WLF2_PCB2_REG(pkt, WLF2_PCB2_PSP_RSP);

		ASSERT(WLPKTTAG_BSSIDX_GET(WLPKTTAG(pkt)) == WLC_BSSCFG_IDX(scb->bsscfg));

		spktq_enq(output_q, pkt);

		spktq_deinit(&fill_q);
	} else if ((i == -1) && (ac == TX_AC_VO_FIFO)) {
		/* there are no packets in the scbq, and we are the highest AC,
		 * so prepare a NULL Data pacekt respons to the PSPoll
		 */
		supplied_time = wlc_apps_nulldata_output(scb, ac, output_q);

		WL_PS(("wl%d:%s: sending PSPoll response NULL DATA to "MACF" ac %u "
		       "supplied time %u\n",
		       wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea), ac, supplied_time));
	} else {
		if (i == -1) {
			WL_PS(("wl%d:%s: skipping PSPoll response to "MACF" at ac %u since there "
			       "are no pkts and will send NULL Data from highest AC\n",
			       wlc->pub->unit, __FUNCTION__,
			       ETHER_TO_MACF(scb->ea), ac));

			/* Make sure the highest AC will run.
			 * The MUX may have already polled the high AC for output and if at that
			 * time a lower prio in the scbq had data, it would have returned no packets
			 * and stalled. But by the time this AC was polled, if the pkts were flushed
			 * for some reason, code would be at this point in the logic.
			 * Make sure the high AC will be polled again by clearing any potential
			 * stall.
			 */
			wlc_scbq_scb_stall_clear(wlc->tx_scbq, scb, TX_AC_VO_FIFO);

			/* kick the queue associated with this scb */
			wlc_send_q(wlc, SCB_WLCIFP(scb)->qi);
		} else if ((uint)i > ac) {
			WL_PS(("wl%d:%s: skipping PSPoll response to "MACF" at ac %u since there "
			       "are pkts at higher ac %d\n", wlc->pub->unit, __FUNCTION__,
			       ETHER_TO_MACF(scb->ea), ac, i));
		} else {
			WL_PS(("wl%d:%s: skipping PSPoll response to "MACF" at ac %u since there "
			       "are only pkts at lower ac %d\n", wlc->pub->unit, __FUNCTION__,
			       ETHER_TO_MACF(scb->ea), ac, i));
		}
	}

	if (supplied_time > 0) {
		WL_TMP(("TXQ: %s setting SCBQ_FC_PS\n", __FUNCTION__));

		/* Stop the SCBQ until the next PSPoll */
		wlc_scbq_scb_stop_flag_set(wlc->tx_scbq, scb, SCBQ_FC_PS);
	}

	return supplied_time;
}

/**
 * @brief Output function that generates one PS-Poll terminating Null Data response
 *
 * Utility function to output a Null Data frame used to respond to a PS-Poll when there is
 * no more buffered data. Used by wlc_scbq_ps_output().
 *
 * This function returns an estimate of the total TX time of Null Data packet.
 *
 * @param scb           A pointer to the SCB of interest.
 * @param ac            Access Category traffic being requested
 * @param output_q      pointer to a packet queue to use to store the output packets
 * @return              The estimated TX time of the Null Data packet returned in the output_q.
 */
static uint
wlc_apps_nulldata_output(struct scb *scb, uint ac, struct spktq *output_q)
{
	wlc_bsscfg_t *bsscfg = scb->bsscfg;
	wlc_info_t *wlc = bsscfg->wlc;
	osl_t *osh = wlc->osh;
	void *pkt;
	uint supplied_time = 0;
	uint d11_frame_len;
	uint fifo; /* is this param needed anymore ? */
	ratespec_t rspec;

	/*
	 * construct the Null Data frame
	 */
	pkt = wlc_alloc_nulldata(wlc, bsscfg, scb, WLF_PSDONTQ, fifo2prio[ac]);

	if (pkt == NULL) {
		struct apps_scb_psinfo *scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

		WL_ERROR(("wl%d: %s: PS-Poll null data response failed\n",
		          wlc->pub->unit, __FUNCTION__));

		/* clear multiple ps-poll frame protection since we were not able
		 * to respond. The next PSPoll will trigger another attempt.
		 */
		scb_psinfo->psp_flags &= ~PS_PSP_ONRESP;
	}

	/* register packet callback */
	WLF2_PCB2_REG(pkt, WLF2_PCB2_PSP_RSP);

	WLPKTTAGSCBSET(pkt, scb);

	/*
	 * MPDU packet prep
	 */

	d11_frame_len = PKTLEN(osh, pkt);

	/* send the null data at the most robust rate */
	rspec = WLC_LOWEST_SCB_RSPEC(scb);

	/* save the tx options for wlc_prep_pdu */
	wlc_pdu_push_txparams(wlc, pkt, 0, NULL, rspec, ac);

	(void)wlc_prep_pdu(wlc, scb, pkt, &fifo);

	/* calculate and store the estimated pkt tx time */
	supplied_time = wlc_tx_mpdu_time(wlc, scb, rspec, ac, d11_frame_len);
	WLPKTTIME(pkt) = (uint16)supplied_time;

	/* add this pkt to the output queue */
	spktq_enq(output_q, pkt);

	return supplied_time;
}

#endif /* TXQ_MUX */

#ifdef BCMDBG_TXSTUCK
void
wlc_apps_print_scb_psinfo_txstuck(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	struct scb *scb;
	struct apps_scb_psinfo *scb_psinfo;
	struct apps_bss_info *bss_info;
	struct pktq *pktq;
	wlc_bsscfg_t *bsscfg;

	struct scb_iter scbiter;

#if !defined(TXQ_MUX)
	bcm_bprintf(b, "WLC discards:%d, ps_deferred:%d\n",
		wlc->psinfo->ps_discard,
		wlc->psinfo->ps_deferred);
#endif /* !TXQ_MUX */

	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		if (scb == NULL) {
			continue;
		}

		bsscfg = scb->bsscfg;
		if (bsscfg == NULL) {
			continue;
		}

		if (!BSSCFG_AP(bsscfg)) {
			continue;
		}

		scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
		if (scb_psinfo == NULL)
			continue;

		bcm_bprintf(b, "APPS psinfo on SCB %p is %p; scb-PS is %s;"
			"\n", OSL_OBFUSCATE_BUF(scb), OSL_OBFUSCATE_BUF(scb_psinfo),
			SCB_PS(scb) ? "on" : "off");

#if defined(TXQ_MUX)
		bcm_bprintf(b, "tx_block %d ext_qos_null %d\n",
			scb_psinfo->tx_block, scb_psinfo->ext_qos_null);
#else
		bcm_bprintf(b, "tx_block %d ext_qos_null %d psp_pending %d discards %d\n",
			scb_psinfo->tx_block, scb_psinfo->ext_qos_null, scb_psinfo->psp_pending,
			scb_psinfo->ps_discard);
#endif /* TXQ_MUX */

		pktq = &scb_psinfo->psq;
		if (pktq == NULL) {
			bcm_bprintf(b, "Packet queue is NULL\n");
			continue;
		}

		bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, bsscfg);
		bcm_bprintf(b, "Pkt Q %p. Que len %d. Max %d. Avail %d. Seen %d. Disc %d\n",
			OSL_OBFUSCATE_BUF(pktq),
			pktq_n_pkts_tot(pktq), pktq_max(pktq), pktq_avail(pktq),
			WLCNTVAL(bss_info->bcmc_pkts_seen), WLCNTVAL(bss_info->bcmc_discards));
	}
}
#endif /* BCMDBG_TXSTUCK */

struct pktq *
wlc_apps_get_psq(wlc_info_t * wlc, struct scb * scb)
{
	if ((wlc == NULL) ||
		(scb == NULL) ||
		(SCB_PSINFO(wlc->psinfo, scb) == NULL)) {
		return NULL;
	}

	return &SCB_PSINFO(wlc->psinfo, scb)->psq;
}

#ifdef WL_AUXPMQ
/* clear auxpmq index for each SCB. */
void
wlc_apps_clear_auxpmq(wlc_info_t *wlc)
{
	struct scb_iter scbiter;
	struct scb *scb;
	struct apps_scb_psinfo *scb_psinfo;

	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
		if (scb_psinfo) {
			scb_psinfo->auxpmq_idx = AUXPMQ_INVALID_IDX;
		}
	}
}
#endif /* WL_AUXPMQ */
