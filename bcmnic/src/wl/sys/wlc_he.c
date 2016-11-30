/*
 * 802.11ax HE (High Efficiency) STA signaling and d11 h/w manipulation.
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_he.c 642695 2016-06-09 15:09:17Z $
 */

#ifdef WL11AX

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <proto/802.11.h>
#include <proto/802.11ax.h>
#include <wl_dbg.h>
#include <wlc_types.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_ie_mgmt_vs.h>
#include <wlc_dump.h>
#include <wlc_iocv_cmd.h>
#include <wlc_bsscfg.h>
#include <wlc_scb.h>
#include <wlc_he.h>
#include <wlc_twt.h>
#include <wlc_pcb.h>
#include <wlc_event_utils.h>
#include <wlc_mcnx.h>

/* forward declaration */
struct he_ppe_threshold {
	uint8 ppet_len;
	uint8* ppet_info;
};
typedef struct he_ppe_threshold he_ppet_t;

/* module info */
struct wlc_he_info {
	wlc_info_t *wlc;
	int scbh;
	/* HE capabilities of this node being advertised in outgoing mgmt frames.
	* Do not modify this while processing incoming mgmt frames.
	*/
	he_cap_ie_t he_cap; /* HE capabilities of this node */
	he_ppet_t ppet;

	/* next action frame dialog token (for AUTO only) */
	uint8 dialog_token;

	/* debug counters */
	uint af_rx_errs;	/* # AF parsing with errors */
	uint af_rx_usols;	/* # unsolicated AFs recv'd */
	uint af_tx_errs;	/* # send/queue AF errors */
	uint twt_req_errs;	/* # request errors */
	uint twt_resp_errs;	/* # response errors */
	uint fsm_busy_errs;	/* # FSMs busy errors */
};

/* debug */
#define HECNTRINC(hei, cntr)	do {(hei)->cntr++;} while (0)
#define WLUNIT(hei)	((hei)->wlc->pub->unit)
#define WL_HE_ERR(x)	WL_ERROR(x)
#define ERR_ONLY_VAR(x)
#define WL_HE_INFO(x)	WL_ERROR(x)
#define INFO_ONLY_VAR(x)
#define WL_HE_LOG(x)	WL_PRINT(x)

/* handy macros */
#define _SIZEOF_(type, field) sizeof(((type *)0)->field)

/* scb cubby */
/* TODO: compress the structure as much as possible */
typedef struct {
	uint8 frag_level;	/* fragmentation level supported by the peer */
	/* FSMs */
	uint8 af_pend;	/* tells what state we're in. See HE_AF_PEND_XXX. af='action frame'. */
	uint8 af_to;	/* # ticks till timeout (also indicates the process in progress).
			 * 0 disabled.
			 */
	union {
		/* for TWT Requesting STA FSM */
		struct {
			/* necessites for caller notification... */
			wlc_he_twt_setup_cplt_fn_t cplt_fn;	/* completion callback fn */
			void *cplt_ctx;		/* completion callback context */
			uint16 dialog_token;
			uint8 flow_id;
		} twt_req;
		/* for other FSMs */
		uint8 flow_id;
	};
} scb_he_t;

/* TWT FSMs & states */
#define WLC_HE_TWT_SETUP_ST_NOTHING	0
/* Requesting STA FSM states */
#define WLC_HE_TWT_SETUP_ST_REQ_ACK	1	/* Waiting for Request AF Ack */
#define WLC_HE_TWT_SETUP_ST_RESP	2	/* Waiting for Response AF */
#define WLC_HE_TWT_SETUP_ST_NOTIF_TO	3	/* Notified with timeout */
#define WLC_HE_TWT_SETUP_ST_NOTIF	4	/* Notified */
/* Responding STA FSM states */
#define WLC_HE_TWT_SETUP_ST_RESP_ACK	11	/* Waiting for Response AF Ack */
#define WLC_HE_TWT_SETUP_ST_RESP_TO	12	/* Response AF Ack timeout */
#define WLC_HE_TWT_SETUP_ST_RESP_DONE	13	/* Response AF Tx completion done */
/* Teardown FSM states */
#define WLC_HE_TWT_TEARDOWN_ST_ACK	21	/* Waiting for Teardown AF Ack */
#define WLC_HE_TWT_TEARDOWN_ST_TO	22	/* Teardown AF Ack timeout */
#define WLC_HE_TWT_TEARDOWN_ST_DONE	23	/* Teardown AF Tx completion done */

/* cubby access macros */
#define SCB_HE_CUBBY(hei, scb)	(scb_he_t **)SCB_CUBBY(scb, (hei)->scbh)
#define SCB_HE(hei, scb)	*SCB_HE_CUBBY(hei, scb);

/* default features */
#ifndef WL_HE_FEATURES_DEFAULT
#define WL_HE_FEATURES_DEFAULT	(WL_HE_FEATURES_2G|WL_HE_FEATURES_5G)
#endif

/* local declarations */

/* wlc module */
static int wlc_he_wlc_init(void *ctx);
static void wlc_he_watchdog(void *ctx);
static int wlc_he_doiovar(void *context, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, struct wlc_if *wlcif);

/* bsscfg module */
static void wlc_he_bsscfg_state_upd(void *ctx, bsscfg_state_upd_data_t *evt);

/* scb cubby */
static int wlc_he_scb_init(void *ctx, struct scb *scb);
static void wlc_he_scb_deinit(void *ctx, struct scb *scb);
static uint wlc_he_scb_secsz(void *, struct scb *);

/* IE mgmt */
static uint wlc_he_calc_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_he_write_cap_ie(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_he_calc_op_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_he_write_op_ie(void *ctx, wlc_iem_build_data_t *data);
static void _wlc_he_parse_cap_ie(struct scb *scb, he_cap_ie_t *cap);
static int wlc_he_parse_cap_ie(void *ctx, wlc_iem_parse_data_t *data);
static void _wlc_he_parse_op_ie(wlc_bsscfg_t *cfg, he_op_ie_t *op);
static int wlc_he_parse_op_ie(void *ctx, wlc_iem_parse_data_t *data);
static int wlc_he_scan_parse_cap_ie(void *ctx, wlc_iem_parse_data_t *data);
static int wlc_he_scan_parse_op_ie(void *ctx, wlc_iem_parse_data_t *data);

/* misc */
static bool wlc_he_hw_cap(wlc_info_t *wlc);
static uint8 wlc_he_get_dialog_token(wlc_he_info_t *hei);
static void wlc_he_af_txcomplete(wlc_info_t *wlc, void *pkt, uint txstatus);
static uint8 wlc_he_get_frag_cap(wlc_info_t *wlc);
static uint8 wlc_he_get_ppet_info(wlc_info_t *wlc, uint8 *ppet);


/* iovar table */
enum {
	IOV_HE = 0,
	IOV_LAST
};

static const bcm_iovar_t he_iovars[] = {
	{"he", IOV_HE, 0, 0, IOVT_BUFFER, 0},
	{NULL, 0, 0, 0, 0, 0}
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
*/
#include <wlc_patch.h>

/* ======== attach/detach ======== */

wlc_he_info_t *
BCMATTACHFN(wlc_he_attach)(wlc_info_t *wlc)
{
	uint16 build_capfstbmp =
	        FT2BMP(FC_BEACON) |
	        FT2BMP(FC_PROBE_RESP) |
#ifdef STA
	        FT2BMP(FC_ASSOC_REQ) |
	        FT2BMP(FC_REASSOC_REQ) |
#endif
#ifdef AP
	        FT2BMP(FC_ASSOC_RESP) |
	        FT2BMP(FC_REASSOC_RESP) |
#endif
	        FT2BMP(FC_PROBE_REQ) |
	        0;
	uint16 build_opfstbmp =
	        FT2BMP(FC_BEACON) |
	        FT2BMP(FC_PROBE_RESP) |
#ifdef AP
	        FT2BMP(FC_ASSOC_RESP) |
	        FT2BMP(FC_REASSOC_RESP) |
#endif
	        0;
	uint16 parse_capfstbmp =
#ifdef AP
	        FT2BMP(FC_ASSOC_REQ) |
	        FT2BMP(FC_REASSOC_REQ) |
#endif
#ifdef STA
	        FT2BMP(FC_ASSOC_RESP) |
	        FT2BMP(FC_REASSOC_RESP) |
#endif
	        FT2BMP(FC_PROBE_REQ) |
	        0;
	uint16 parse_opfstbmp =
#ifdef STA
	        FT2BMP(FC_ASSOC_RESP) |
	        FT2BMP(FC_REASSOC_RESP) |
#endif
	        0;
	uint16 scanfstbmp =
	        FT2BMP(WLC_IEM_FC_SCAN_BCN) |
	        FT2BMP(WLC_IEM_FC_SCAN_PRBRSP |
		0);
	wlc_he_info_t *hei;

	/* allocate private module info */
	if ((hei = MALLOCZ(wlc->osh, sizeof(*hei))) == NULL) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	hei->wlc = wlc;

	/* reserve some space in scb for private data */
	{
	scb_cubby_params_t cubby_params;

	bzero(&cubby_params, sizeof(cubby_params));

	cubby_params.context = hei;
	cubby_params.fn_init = wlc_he_scb_init;
	cubby_params.fn_deinit = wlc_he_scb_deinit;
	cubby_params.fn_secsz = wlc_he_scb_secsz;

	if ((hei->scbh = wlc_scb_cubby_reserve_ext(wlc,
			sizeof(scb_he_t *), &cubby_params)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve_ext() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	}

	/* register IE mgmt callbacks - calc/build */

	/* bcn/prbreq/prbrsp/assocreq/reassocreq/assocresp/reassocresp */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, build_capfstbmp, DOT11_MNG_HE_CAP_ID,
			wlc_he_calc_cap_ie_len, wlc_he_write_cap_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, he cap ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	/* bcn/prbrsp/assocresp/reassocresp */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, build_opfstbmp, DOT11_MNG_HE_OP_ID,
			wlc_he_calc_op_ie_len, wlc_he_write_op_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, he op ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register IE mgmt callbacks - parse */

	/* assocreq/reassocreq/assocresp/reassocresp */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, parse_capfstbmp, DOT11_MNG_HE_CAP_ID,
			wlc_he_parse_cap_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, he cap ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	/* assocresp/reassocresp */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, parse_opfstbmp, DOT11_MNG_HE_OP_ID,
			wlc_he_parse_op_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, he op ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	/* bcn/prbrsp in scan */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, scanfstbmp, DOT11_MNG_HE_CAP_ID,
	                             wlc_he_scan_parse_cap_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, cap ie in scan\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	/* bcn/prbrsp in scan */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, scanfstbmp, DOT11_MNG_HE_OP_ID,
	                             wlc_he_scan_parse_op_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, op ie in scan\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* bsscfg state change callback */
	if (wlc_bsscfg_state_upd_register(wlc, wlc_he_bsscfg_state_upd, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_state_upd_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register packet class callback */
	if (wlc_pcb_fn_set(wlc->pcb, 0, WLF2_PCB1_HE_AF, wlc_he_af_txcomplete) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_pcb_fn_set failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register module up/down, watchdog, and iovar callbacks */
	if (wlc_module_register(wlc->pub, he_iovars, "he", hei, wlc_he_doiovar,
			wlc_he_watchdog, wlc_he_wlc_init, NULL)) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}


	/* enable HE by default if possible */
	if (wlc_he_hw_cap(wlc)) {
		WLC_HE_FEATURES_SET(wlc->pub, WL_HE_FEATURES_DEFAULT);
		wlc->pub->_he_enab = TRUE;
	}

	return hei;

fail:
	wlc_he_detach(hei);
	return NULL;
}

void
BCMATTACHFN(wlc_he_detach)(wlc_he_info_t *hei)
{
	wlc_info_t *wlc;

	if (hei == NULL) {
		return;
	}

	wlc = hei->wlc;

	(void)wlc_module_unregister(wlc->pub, "he", hei);

	MFREE(wlc->osh, hei, sizeof(*hei));
}

/*  Initialize supported HE capabilities  */
void
BCMATTACHFN(wlc_he_init_defaults)(wlc_he_info_t *hei)
{
	wlc_info_t *wlc = hei->wlc;
	he_cap_info_t *info;
	he_ppet_t *ppet;

	ASSERT(wlc);

	if (!HE_ENAB(wlc->pub))
		return;

	info = &hei->he_cap.info;
	ppet = &hei->ppet;

	*info = 0;

	/* Initialize fragmentation level supported by HE PHY. */
	*info |= (wlc_he_get_frag_cap(wlc) & HE_INFO_FRAG_SUPPORT_MASK) <<
			HE_INFO_FRAG_SUPPORT_SHIFT;

	/* Initialize PPE Thresholds info for HE Phy  */
	if ((ppet->ppet_len = wlc_he_get_ppet_info(wlc, ppet->ppet_info)) != 0) {
		*info |= HE_INFO_PPE_THRESH_PRESENT;
	}

	/* copy HE capabilities into default BSS */
	wlc->default_bss->he_capabilities = hei->he_cap.info;

}

/* ======== iovar dispatch ======== */


static int
wlc_he_cmd_enab(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen,
	bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	bcm_xtlv_t *xtlv = (bcm_xtlv_t *)params;

	if (set) {
		if (!wlc_he_hw_cap(wlc)) {
			return BCME_UNSUPPORTED;
		}
		wlc->pub->_he_enab = *(uint8 *)(xtlv->data);
	}
	else {
		*result = wlc->pub->_he_enab;
		*rlen = sizeof(*result);
	}

	return BCME_OK;
}

static int
wlc_he_cmd_features(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen,
	bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	bcm_xtlv_t *xtlv = (bcm_xtlv_t *)params;

	if (set) {
		wlc->pub->he_features = *(uint32 *)(xtlv->data);
	}
	else {
		*(uint32 *)result = wlc->pub->he_features;
		*rlen = sizeof(*(uint32 *)result);
	}

	return BCME_OK;
}

#define WL_TWT_SETUP_CPLT_LEN sizeof(wl_twt_setup_cplt_t) + sizeof(wl_twt_sdesc_t)
#define WL_TWT_SETUP_CPLT_HDR_LEN (_SIZEOF_(wl_twt_setup_cplt_t, version) + \
				   _SIZEOF_(wl_twt_setup_cplt_t, length))

static void
wlc_he_cmd_twt_setup_cplt(void *ctx, struct scb *scb, wlc_he_twt_setup_cplt_data_t *rslt)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	uint8 data[WL_TWT_SETUP_CPLT_LEN];
	wl_twt_setup_cplt_t *event = (wl_twt_setup_cplt_t *)data;

	/* log */
	WL_HE_LOG(("%s: dialog_token %u setup_cmd %u flow_id %u\n", __FUNCTION__,
	           rslt->dialog_token, rslt->desc.setup_cmd, rslt->desc.flow_id));

	event->version = WL_TWT_SETUP_CPLT_VER;
	event->length = WL_TWT_SETUP_CPLT_LEN - WL_TWT_SETUP_CPLT_HDR_LEN;
	event->dialog = (uint8)rslt->dialog_token;
	event->status = (int32)rslt->status;
	*(wl_twt_sdesc_t *)&event[1] = rslt->desc;

	/* event */
	wlc_bss_mac_event(wlc, SCB_BSSCFG(scb), WLC_E_HE_TWT_SETUP, &scb->ea,
		0, WLC_E_STATUS_SUCCESS, 0, event, WL_TWT_SETUP_CPLT_LEN);
}

static int
wlc_he_cmd_twt_setup(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen,
	bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	struct scb *scb = NULL;
	wlc_he_twt_setup_t req;
	wl_twt_setup_t *setup = (wl_twt_setup_t *)params;
	uint16 hdrlen;
	struct ether_addr *addr;

	if (!set) {
		return BCME_UNSUPPORTED;
	}

	if (!TWT_ENAB(wlc->pub)) {
		return BCME_UNSUPPORTED;
	}

	/* sanity check */
	hdrlen = sizeof(setup->version) + sizeof(setup->length);
	if (plen < hdrlen) {
		WL_ERROR(("wl%d: %s: buffer is shorter than the header\n",
		          wlc->pub->unit, __FUNCTION__));
		return BCME_BUFTOOSHORT;
	}
	if (plen < setup->length + hdrlen) {
		WL_ERROR(("wl%d: %s: buffer is too short. expected %u got %u\n",
		          wlc->pub->unit, __FUNCTION__, setup->length + hdrlen, plen));
		return BCME_BUFTOOSHORT;
	}
	if (setup->version != WL_TWT_SETUP_VER) {
		WL_ERROR(("wl%d: %s: version is not supported. expected %u got %u\n",
		          wlc->pub->unit, __FUNCTION__, WL_TWT_SETUP_VER, setup->version));
		return BCME_UNSUPPORTED;
	}

	/* lookup the bsscfg */
	cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(cfg != NULL);

	/* the command is supported only when the bsscfg is "associated" */
	if (!cfg->associated) {
		return BCME_NOTASSOCIATED;
	}

	/* ignore the user supplied peer address for infra STA and use BSSID instead */
	addr = BSSCFG_STA(cfg) && cfg->BSS ? &cfg->BSSID : &setup->peer;

	/* move the desc to local and fixup a few things */
	bzero(&req, sizeof(req));
	req.cplt_fn = wlc_he_cmd_twt_setup_cplt;
	req.cplt_ctx = hei;
	req.dialog_token = setup->dialog;
	req.desc = setup->desc;

	/* broadcast TWT is valid to AP or IBSS only */
	if ((req.desc.flow_flags & WL_TWT_FLOW_FLAG_BROADCAST) &&
	    BSSCFG_STA(cfg) && cfg->BSS) {
		WL_ERROR(("wl%d: %s: broadcast TWT is not valid for Infra STA\n",
		          wlc->pub->unit, __FUNCTION__));
		return BCME_NOTSTA;
	}

	/* target wake time */
	switch (req.desc.wake_type) {
	case WL_TWT_TIME_TYPE_BSS:
		break;
	case WL_TWT_TIME_TYPE_OFFSET:
#ifdef WLMCNX
		if (MCNX_ENAB(wlc->pub)) {
			wlc_mcnx_read_tsf64(wlc->mcnx, cfg,
				&req.desc.wake_time_h, &req.desc.wake_time_l);
		} else
#endif
		{
			wlc_read_tsf(wlc, &req.desc.wake_time_l, &req.desc.wake_time_h);
		}
		wlc_uint64_add(&req.desc.wake_time_h, &req.desc.wake_time_l,
			setup->desc.wake_time_h, setup->desc.wake_time_l);
		break;
	default:
		return BCME_UNSUPPORTED;
	}

	/* just plumb the broadcast TWT to the HW */
	if (req.desc.flow_flags & WL_TWT_FLOW_FLAG_BROADCAST) {
		/* TODO: check if we are TWT scheduling STA capable */
		/* if (!wlc_twt_sch_cap(wlc->twti, cfg)) {
		 * }
		 */
		return wlc_twt_setup_bcast(wlc->twti, cfg, &req.desc);
	}

	/* lookup the scb */
	if ((scb = wlc_scbfind(wlc, cfg, addr)) == NULL) {
		DBGONLY(char eabuf[ETHER_ADDR_STR_LEN]);
		WL_ERROR(("wl%d: %s: scb %s not found\n", wlc->pub->unit,
		          __FUNCTION__, bcm_ether_ntoa(addr, eabuf)));
		return BCME_NOTFOUND;
	}

	/* are we TWT requesting STA capable? */
	if (!wlc_twt_req_cap(wlc->twti, cfg)) {
		return BCME_UNSUPPORTED;
	}

	/* are they TWT responding STA capable? */
	if (!SCB_TWT_RESP_CAP(scb)) {
		return BCME_UNSUPPORTED;
	}

	/* start the individual TWT Setup process */

	return wlc_he_twt_setup(hei, scb, &req);
}

static int
wlc_he_cmd_twt_teardown(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen,
	bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	struct scb *scb = NULL;
	wl_twt_teardown_t *td = (wl_twt_teardown_t *)params;
	uint16 hdrlen;
	struct ether_addr *addr;

	if (!set) {
		return BCME_UNSUPPORTED;
	}

	if (!TWT_ENAB(wlc->pub)) {
		return BCME_UNSUPPORTED;
	}

	/* sanity check */
	hdrlen = sizeof(td->version) + sizeof(td->length);
	if (plen < hdrlen) {
		WL_ERROR(("wl%d: %s: buffer is shorter than the header\n",
		          wlc->pub->unit, __FUNCTION__));
		return BCME_BUFTOOSHORT;
	}
	if (plen < td->length + hdrlen) {
		WL_ERROR(("wl%d: %s: buffer is too short. expected %u got %u\n",
		          wlc->pub->unit, __FUNCTION__, td->length + hdrlen, plen));
		return BCME_BUFTOOSHORT;
	}
	if (td->version != WL_TWT_TEARDOWN_VER) {
		WL_ERROR(("wl%d: %s: version is not supported. expected %u got %u\n",
		          wlc->pub->unit, __FUNCTION__, WL_TWT_TEARDOWN_VER, td->version));
		return BCME_UNSUPPORTED;
	}

	/* lookup the bsscfg */
	cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(cfg != NULL);

	/* the command is supported only when the bsscfg is "associated" */
	if (!cfg->associated) {
		return BCME_NOTASSOCIATED;
	}

	/* ignore the user supplied peer address for infra STA and use BSSID instead */
	addr = BSSCFG_STA(cfg) && cfg->BSS ? &cfg->BSSID : &td->peer;

	/* broadcast TWT is valid to AP or IBSS only */
	if ((td->flow_flags & WL_TWT_FLOW_FLAG_BROADCAST) &&
	    BSSCFG_STA(cfg) && cfg->BSS) {
		WL_ERROR(("wl%d: %s: broadcast TWT is not valid for Infra STA\n",
		          wlc->pub->unit, __FUNCTION__));
		return BCME_NOTSTA;
	}

	/* just remove the broadcast TWT from the HW */
	if (td->flow_flags & WL_TWT_FLOW_FLAG_BROADCAST) {
		/* TODO: check if we are TWT scheduling STA capable */
		/* if (!wlc_twt_sch_cap(wlc->twti, cfg)) {
		 * }
		 */
		return wlc_twt_teardown_bcast(wlc->twti, cfg, td->flow_id);
	}

	/* lookup the scb */
	if ((scb = wlc_scbfind(wlc, cfg, addr)) == NULL) {
		DBGONLY(char eabuf[ETHER_ADDR_STR_LEN]);
		WL_ERROR(("wl%d: %s: scb %s not found\n", wlc->pub->unit,
		          __FUNCTION__, bcm_ether_ntoa(addr, eabuf)));
		return BCME_NOTFOUND;
	}

	/* start the individual TWT Teardown process */

	return wlc_he_twt_teardown(hei, scb, td->flow_id);
}

static int
wlc_he_cmd_twt_info(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen,
	bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	struct scb *scb = NULL;
	wl_twt_info_t *info = (wl_twt_info_t *)params;
	uint16 hdrlen;
	struct ether_addr *addr;

	if (!set) {
		return BCME_UNSUPPORTED;
	}

	if (!TWT_ENAB(wlc->pub)) {
		return BCME_UNSUPPORTED;
	}

	/* sanity check */
	hdrlen = sizeof(info->version) + sizeof(info->length);
	if (plen < hdrlen) {
		WL_ERROR(("wl%d: %s: buffer is shorter than the header\n",
		          wlc->pub->unit, __FUNCTION__));
		return BCME_BUFTOOSHORT;
	}
	if (plen < info->length + hdrlen) {
		WL_ERROR(("wl%d: %s: buffer is too short. expected %u got %u\n",
		          wlc->pub->unit, __FUNCTION__, info->length + hdrlen, plen));
		return BCME_BUFTOOSHORT;
	}
	if (info->version != WL_TWT_INFO_VER) {
		WL_ERROR(("wl%d: %s: version is not supported. expected %u got %u\n",
		          wlc->pub->unit, __FUNCTION__, WL_TWT_INFO_VER, info->version));
		return BCME_UNSUPPORTED;
	}

	/* lookup the bsscfg */
	cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(cfg != NULL);

	/* the command is supported only when the bsscfg is "associated" */
	if (!cfg->associated) {
		return BCME_NOTASSOCIATED;
	}

	/* ignore the user supplied peer address for infra STA and use BSSID instead */
	addr = BSSCFG_STA(cfg) && cfg->BSS ? &cfg->BSSID : &info->peer;

	/* lookup the scb */
	if ((scb = wlc_scbfind(wlc, cfg, addr)) == NULL) {
		DBGONLY(char eabuf[ETHER_ADDR_STR_LEN]);
		WL_ERROR(("wl%d: %s: scb %s not found\n", wlc->pub->unit,
		          __FUNCTION__, bcm_ether_ntoa(addr, eabuf)));
		return BCME_NOTFOUND;
	}

	/* start the individual TWT Information process */

	return wlc_he_twt_info(hei, scb, &info->desc);
}

/*  HE cmds  */
static const wlc_iov_cmd_t he_cmds[] = {
	{WL_HE_CMD_ENAB, 0, IOVT_UINT8, wlc_he_cmd_enab},
	{WL_HE_CMD_FEATURES, 0, IOVT_UINT32, wlc_he_cmd_features},
	{WL_HE_CMD_TWT_SETUP, 0, IOVT_BUFFER, wlc_he_cmd_twt_setup},
	{WL_HE_CMD_TWT_TEARDOWN, 0, IOVT_BUFFER, wlc_he_cmd_twt_teardown},
	{WL_HE_CMD_TWT_INFO, 0, IOVT_BUFFER, wlc_he_cmd_twt_info},
};

static int
wlc_he_doiovar(void *ctx, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, struct wlc_if *wlcif)
{
	int err = BCME_OK;

	BCM_REFERENCE(vsize);

	switch (actionid) {
	case IOV_GVAL(IOV_HE):
		err = wlc_iocv_iov_cmd_proc(ctx, he_cmds, ARRAYSIZE(he_cmds),
			FALSE, params, plen, arg, alen, wlcif);
		break;
	case IOV_SVAL(IOV_HE):
		err = wlc_iocv_iov_cmd_proc(ctx, he_cmds, ARRAYSIZE(he_cmds),
			TRUE, params, plen, arg, alen, wlcif);
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* ======== wlc module hooks ========= */

/* wlc init callback */
static int
wlc_he_wlc_init(void *ctx)
{
	return BCME_OK;
}

/* timer/timeout processing */
/* someone needs to tick and this function just decrements it's timeout
 * value by 1 at each and every tick...and execute timeout processing
 * code when the timeout value changes frmo 1 to 0.
 */
static void
wlc_he_twt_to_proc(wlc_he_info_t *hei)
{
	wlc_info_t *wlc = hei->wlc;
	struct scb_iter scbiter;
	struct scb *scb;

	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		scb_he_t *sh = SCB_HE(hei, scb);

		if (sh == NULL || sh->af_to == 0 || --sh->af_to > 0) {
			continue;
		}

		{
		DBGONLY(char eabuf[ETHER_ADDR_STR_LEN]);
		WL_HE_INFO(("wl%d: %s: peer %s state %u timeout\n",
		            WLUNIT(hei), __FUNCTION__, bcm_ether_ntoa(&scb->ea, eabuf),
		            sh->af_pend));
		}

		switch (sh->af_pend) {

		/* -------- TWT Requesting STA FSM -------- */

		case WLC_HE_TWT_SETUP_ST_REQ_ACK:
		case WLC_HE_TWT_SETUP_ST_RESP: {
			wlc_he_twt_setup_cplt_data_t cplt_data;

			/* invoke the callback */
			bzero(&cplt_data, sizeof(cplt_data));

			cplt_data.status = BCME_ERROR;
			cplt_data.dialog_token = sh->twt_req.dialog_token;

			(sh->twt_req.cplt_fn)(sh->twt_req.cplt_ctx, scb, &cplt_data);

			/* move the state */
			sh->af_pend = WLC_HE_TWT_SETUP_ST_NOTIF_TO;

			/* rlease flow id and other reserved resources */
			wlc_twt_release(wlc->twti, scb, sh->twt_req.flow_id);
			break;
		}

		/* -------- TWT Responding STA FSM -------- */

		case WLC_HE_TWT_SETUP_ST_RESP_ACK:
			/* move the state */
			sh->af_pend = WLC_HE_TWT_SETUP_ST_RESP_TO;

			/* rlease flow id and other reserved resources */
			wlc_twt_release(wlc->twti, scb, sh->flow_id);
			break;

		/* -------- TWT Teardown FSM -------- */

		case WLC_HE_TWT_TEARDOWN_ST_ACK:
			/* move the state */
			sh->af_pend = WLC_HE_TWT_TEARDOWN_ST_TO;

			/* free the flow id? */
			break;

		default:
			break;
		}
	}
}

/* wlc watchdog */
static void
wlc_he_watchdog(void *ctx)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;

	BCM_REFERENCE(wlc);

	if (TWT_ENAB(wlc->pub)) {
		wlc_he_twt_to_proc(hei);
	}
}


/* ======== bsscfg module hooks ======== */

/* bsscfg state change callback */
static void
wlc_he_bsscfg_state_upd(void *ctx, bsscfg_state_upd_data_t *evt)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	wlc_info_t *wlc =  hei->wlc;
	wlc_bsscfg_t *cfg = evt->cfg;

	if (!evt->old_up && cfg->up &&
	    (BSSCFG_AP(cfg) || BSSCFG_IBSS(cfg))) {
		wlc_bss_info_t *bi = cfg->current_bss;

		bi->flags3 &= ~(WLC_BSS3_HE);

		if (BSS_HE_ENAB_BAND(wlc, wlc->band->bandtype, cfg)) {
			bi->flags3 |= WLC_BSS3_HE;
		}
	}
}

/* ======== scb cubby ======== */

static int
wlc_he_scb_init(void *ctx, struct scb *scb)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	scb_he_t **psh = SCB_HE_CUBBY(hei, scb);
	scb_he_t *sh = SCB_HE(hei, scb);

	ASSERT(sh == NULL);

	*psh = wlc_scb_sec_cubby_alloc(wlc, scb, sizeof(*sh));

	return BCME_OK;
}

static void
wlc_he_scb_deinit(void *ctx, struct scb *scb)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	scb_he_t **psh = SCB_HE_CUBBY(hei, scb);
	scb_he_t *sh = SCB_HE(hei, scb);

	ASSERT(sh != NULL);

	wlc_scb_sec_cubby_free(wlc, scb, sh);
	*psh = NULL;
}

static uint
wlc_he_scb_secsz(void *ctx, struct scb *scb)
{
	scb_he_t *sh;
	return sizeof(*sh);
}


/* ======== IE mgmt ======== */


/* HE Cap IE */
static uint
wlc_he_calc_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;

	if (!data->cbparm->he)
		return 0;

	return sizeof(he_cap_ie_t) + hei->ppet.ppet_len;
}

static int
wlc_he_write_cap_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	wlc_info_t *wlc = hei->wlc;
	he_cap_info_t info = 0;
	uint8 *buf = data->buf;
	uint8 ppet_len = 0;

	buf[TLV_TAG_OFF] = DOT11_MNG_HE_CAP_ID;
	/* set the length of the HE cap IE */
	ppet_len = hei->ppet.ppet_len;
	buf[TLV_LEN_OFF] = (uint8)(sizeof(he_cap_info_t) + ppet_len);

	/* Initializing default HE capabilities
	* Set other bits to add new capabilities
	*/
	info = hei->he_cap.info;

	info &= ~(HE_INFO_TWT_REQ_SUPPORT | HE_INFO_TWT_RESP_SUPPORT);

	info |= TWT_ENAB(wlc->pub) && wlc_twt_req_cap(wlc->twti, data->cfg) ?
	        HE_INFO_TWT_REQ_SUPPORT : 0;
	info |= TWT_ENAB(wlc->pub) && wlc_twt_resp_cap(wlc->twti, data->cfg) ?
	        HE_INFO_TWT_RESP_SUPPORT : 0;

	/* Copy 2 bytes of he cap info */
	info = htol16(info);
	memcpy(&buf[TLV_BODY_OFF], &info, sizeof(he_cap_info_t));
	/* copy the ppet info from bss cubby */
	if (hei->he_cap.info & HE_INFO_PPE_THRESH_PRESENT) {
		memcpy(&buf[TLV_BODY_OFF + sizeof(he_cap_info_t)], hei->ppet.ppet_info, ppet_len);
	}

	return BCME_OK;
}

static void
_wlc_he_parse_cap_ie(struct scb *scb, he_cap_ie_t *cap)
{
	uint16 info = 0;

	scb->flags2 &= ~(SCB2_HECAP|SCB2_TWT_REQCAP|SCB2_TWT_RESPCAP);

	if (cap == NULL) {
		return;
	}

	scb->flags2 |= SCB2_HECAP;

	info = ltoh16_ua((uint8 *)&cap->info);
	scb->flags2 |= ((info & HE_INFO_TWT_REQ_SUPPORT) != 0) ? SCB2_TWT_REQCAP : 0;
	scb->flags2 |= ((info & HE_INFO_TWT_RESP_SUPPORT) != 0) ? SCB2_TWT_RESPCAP : 0;
}

static int
wlc_he_parse_cap_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	switch (data->ft) {
#ifdef AP
	case FC_ASSOC_REQ:
	case FC_REASSOC_REQ: {
		wlc_iem_ft_pparm_t *ftpparm = data->pparm->ft;
		struct scb *scb = ftpparm->assocreq.scb;

		ASSERT(scb != NULL);

		_wlc_he_parse_cap_ie(scb, (he_cap_ie_t *)data->ie);
		break;
	}
#endif
#ifdef STA
	case FC_ASSOC_RESP:
	case FC_REASSOC_RESP: {
		wlc_iem_ft_pparm_t *ftpparm = data->pparm->ft;
		struct scb *scb = ftpparm->assocresp.scb;

		ASSERT(scb != NULL);

		_wlc_he_parse_cap_ie(scb, (he_cap_ie_t *)data->ie);
		break;
	}
#endif
	default:
		break;
	}

	return BCME_OK;
}

static int
wlc_he_scan_parse_cap_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	he_cap_ie_t *cap;
	wlc_iem_ft_pparm_t *ftpparm;
	wlc_bss_info_t *bi;

	ftpparm = data->pparm->ft;
	bi = ftpparm->scan.result;

	bi->flags3 &= ~(WLC_BSS3_HE);
	bi->he_capabilities = 0;

	if ((cap = (he_cap_ie_t *)data->ie) == NULL)
		return BCME_OK;

	/* Mark the BSS as VHT capable */
	bi->flags3 |= WLC_BSS3_HE;
	bi->he_capabilities = ltoh16_ua((uint8 *)&cap->info);

	return BCME_OK;
}

/* HE OP IE */
static uint
wlc_he_calc_op_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	if (!data->cbparm->he)
		return 0;

	return sizeof(he_op_ie_t);
}

static int
wlc_he_write_op_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_bsscfg_t *cfg = data->cfg;
	he_op_ie_t op;

	op.parms = htol16((uint16)(cfg->bss_color & HE_OP_BSS_COLOR_MASK));
	bcm_write_tlv(DOT11_MNG_HE_OP_ID, &op.parms, sizeof(op.parms), data->buf);

	return BCME_OK;
}

static void
_wlc_he_parse_op_ie(wlc_bsscfg_t *cfg, he_op_ie_t *op)
{
	cfg->bss_color = 0;

	if (op != NULL) {
		uint16 parms = ltoh16_ua((uint8 *)&op->parms);

		cfg->bss_color = (uint8)(parms & HE_OP_BSS_COLOR_MASK);
	}
}

static int
wlc_he_parse_op_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	switch (data->ft) {
#ifdef STA
	case FC_ASSOC_RESP:
	case FC_REASSOC_RESP: {
		wlc_bsscfg_t *cfg = data->cfg;

		_wlc_he_parse_op_ie(cfg, (he_op_ie_t *)data->ie);
		break;
	}
#endif
	default:
		break;
	}

	return BCME_OK;
}

static int
wlc_he_scan_parse_op_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	he_op_ie_t *op;

	if ((op = (he_op_ie_t *)data->ie) == NULL)
		return BCME_OK;

	return BCME_OK;
}

/* ======== misc ========= */

/* is the hardware HE capable */
static bool
wlc_he_hw_cap(wlc_info_t *wlc)
{
#ifdef HE_MAC_SW_DEV
	return TRUE;
#else
	return WLC_PHY_HE_CAP(wlc->band);
#endif
}

/* update scb using the cap and op contents */
/* Note - capie and opie are in raw format i.e. LSB. */
void
wlc_he_update_scb_state(wlc_he_info_t *hei, int bandtype, struct scb *scb,
	he_cap_ie_t *capie, he_op_ie_t *opie)
{
	_wlc_he_parse_cap_ie(scb, capie);
	_wlc_he_parse_op_ie(SCB_BSSCFG(scb), opie);
}

void
wlc_he_bcn_scb_upd(wlc_he_info_t *hei, int bandtype, struct scb *scb,
	he_cap_ie_t *capie, he_op_ie_t *opie)
{
	wlc_he_update_scb_state(hei, bandtype, scb, capie, opie);
}

/* return a dialog token */
static uint8
wlc_he_get_dialog_token(wlc_he_info_t *hei)
{
	uint8 dialog_token = hei->dialog_token ++;

	return dialog_token;
}

/* ======== process action frame ======== */

/* maximum # wlc watchdog tick till timeout */
#define HE_AF_PEND_TO		2	/* timeout in 2 ticks */
#define HE_AF_PEND_TO_DIS	0	/* disable the timeout */

/* action frame tx complete callback */
static void
wlc_he_af_txcomplete(wlc_info_t *wlc, void *pkt, uint txstatus)
{
	struct scb *scb = WLPKTTAGSCBGET(pkt);
	scb_he_t *sh;
	bool no_ack;

	/* bail out in case the scb has been deleted */
	if (scb == NULL) {
		return;
	}

	sh = SCB_HE(wlc->hei, scb);
	ASSERT(sh != NULL);

	no_ack = ((txstatus & TX_STATUS_MASK) == TX_STATUS_NO_ACK);

	switch (sh->af_pend) {

	/* -------- TWT Requesting STA FSM -------- */

	case WLC_HE_TWT_SETUP_ST_REQ_ACK:	/* we're waiting for ack */
		/* move to next state - waiting for Response */
		sh->af_pend = WLC_HE_TWT_SETUP_ST_RESP;
		if (no_ack) {
			/* do nothing as we may have lost the ack and can still
			 * hope the "response" to come...
			 */
		}
		break;

	/* -------- TWT Responding STA FSM -------- */

	case WLC_HE_TWT_SETUP_ST_RESP_ACK:
		/* disarm the timer */
		sh->af_to = HE_AF_PEND_TO_DIS;
		/* move the state */
		sh->af_pend = WLC_HE_TWT_SETUP_ST_RESP_DONE;
		if (no_ack) {
			/* we may need to notify the host that we're not getting
			 * the ack...which may be lost but we don't know that and
			 * can't assume the requesting STA has received it...
			 * so notifying the host of such condition and expecting
			 * the host do something about it...
			 */
			/* release any reserved resources */
			wlc_twt_release(wlc->twti, scb, sh->flow_id);
		}
		break;

	/* -------- TWT Teardown FSM -------- */

	case WLC_HE_TWT_TEARDOWN_ST_ACK:
		/* disarm the timer */
		sh->af_to = HE_AF_PEND_TO_DIS;
		/* move the state */
		sh->af_pend = WLC_HE_TWT_TEARDOWN_ST_DONE;
		if (no_ack) {
			/* we may need to notify the host that we're not getting
			 * the ack...which may be lost but we don't know that and
			 * can't assume the requesting STA has received it...
			 * so notifying the host of such condition and expecting
			 * the host do something about it...
			 */
			break;
		}
		/* free the flow id and release any resources including the HW entry */
		wlc_twt_teardown(wlc->twti, scb, sh->flow_id);
		break;

	default:
		/* nothing to do */
		break;
	}
}

/* send TWT Setup request/suggest/demand */
int
wlc_he_twt_setup(wlc_he_info_t *hei, struct scb *scb, wlc_he_twt_setup_t *req)
{
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	void *pkt;
	uint8 *body;
	uint body_len;
	uint16 dialog_token;
	int err;
	scb_he_t *sh;

	if (!TWT_ENAB(wlc->pub)) {
		return BCME_UNSUPPORTED;
	}

	/* bail out if FSM is in progress */
	sh = SCB_HE(hei, scb);
	ASSERT(sh != NULL);

	if (sh->af_to > 0) {
		WL_HE_ERR(("wl%d: %s: FSM is busy\n", WLUNIT(hei), __FUNCTION__));
		HECNTRINC(hei, fsm_busy_errs);
		return BCME_BUSY;
	}

	/* log */
	WL_HE_LOG(("%s: dialog %u flow %u\n", __FUNCTION__,
	           req->dialog_token, req->desc.flow_id));

	ASSERT(req->cplt_fn != NULL);

	/* validate the dialog token or assign one if auto */
	if ((dialog_token = req->dialog_token) == WLC_HE_TWT_SETUP_DIALOG_TOKEN_AUTO) {
		dialog_token = wlc_he_get_dialog_token(hei);
		req->dialog_token = (uint8)dialog_token;
		WL_HE_INFO(("wl%d: %s: dialog_token %u assigned\n",
		            WLUNIT(hei), __FUNCTION__, dialog_token));
	}

	/* validate the flow id or reserve one if auto */
	if ((err = wlc_twt_reserve(wlc->twti, scb, &req->desc)) != BCME_OK) {
		HECNTRINC(hei, twt_req_errs);
		return err;
	}

	body_len = HE_AF_TWT_SETUP_TWT_IE_OFF + wlc_twt_ie_len_calc(wlc->twti, &req->desc, 1);

	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg != NULL);

	/* allocate the packet */
	if ((pkt = wlc_frame_get_action(wlc, &scb->ea, &cfg->cur_etheraddr,
			&cfg->BSSID, body_len, &body, DOT11_ACTION_CAT_HE)) == NULL) {
		WL_ERROR(("wl%d: %s: unable to create TWT Setup frame\n",
		          wlc->pub->unit, __FUNCTION__));
		err = BCME_NOMEM;
		goto fail;
	}

	/* fill up the packet */
	body[HE_AF_CAT_OFF] = DOT11_ACTION_CAT_HE;
	body[HE_AF_ACT_OFF] = HE_ACTION_TWT_SETUP;
	body[HE_AF_TWT_SETUP_TOKEN_OFF] = (uint8)dialog_token;

	wlc_twt_ie_build(wlc->twti, &req->desc, 1,
		&body[HE_AF_TWT_SETUP_TWT_IE_OFF], body_len - HE_AF_TWT_SETUP_TWT_IE_OFF);

	/* TWT Requesting STA FSM */
	sh->af_pend = WLC_HE_TWT_SETUP_ST_REQ_ACK;
	/* start timer now */
	sh->af_to = HE_AF_PEND_TO;
	sh->twt_req.cplt_fn = req->cplt_fn;
	sh->twt_req.cplt_ctx = req->cplt_ctx;
	sh->twt_req.dialog_token = req->dialog_token;
	sh->twt_req.flow_id = req->desc.flow_id;

	/* enable packet callback */
	WLF2_PCB1_REG(pkt, WLF2_PCB1_HE_AF);

	/* commit for transmission */
	if (!wlc_sendmgmt(wlc, pkt, wlc->active_queue, scb)) {
		HECNTRINC(hei, af_tx_errs);
		err = BCME_BUSY;
		pkt = NULL;
		goto fail;
	}

	return BCME_OK;

fail:
	if (pkt != NULL) {
		PKTFREE(wlc->osh, pkt, TRUE);
	}
	wlc_twt_release(wlc->twti, scb, req->desc.flow_id);
	return err;
}

/* send TWT Setup grouping/accept/alternate/dictate/reject */
static int
wlc_he_twt_setup_resp(wlc_he_info_t *hei, struct scb *scb,
	wl_twt_sdesc_t *desc, uint8 dialog_token)
{
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	void *pkt;
	uint8 *body;
	uint body_len;
	scb_he_t *sh;
	int err;

	/* bail out if FSM is in progress */
	sh = SCB_HE(hei, scb);
	ASSERT(sh != NULL);

	if (sh->af_to > 0) {
		WL_HE_ERR(("wl%d: %s: FSM is busy\n", WLUNIT(hei), __FUNCTION__));
		HECNTRINC(hei, fsm_busy_errs);
		return BCME_BUSY;
	}

	/* validate the flow id */
	if ((err = wlc_twt_reserve(wlc->twti, scb, desc)) != BCME_OK) {
		HECNTRINC(hei, twt_resp_errs);
		return err;
	}

	body_len = HE_AF_TWT_SETUP_TWT_IE_OFF + wlc_twt_ie_len_calc(wlc->twti, desc, 1);

	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg != NULL);

	/* allocate the packet */
	if ((pkt = wlc_frame_get_action(wlc, &scb->ea, &cfg->cur_etheraddr,
			&cfg->BSSID, body_len, &body, DOT11_ACTION_CAT_HE)) == NULL) {
		WL_ERROR(("wl%d: %s: unable to create TWT Setup frame\n",
		          wlc->pub->unit, __FUNCTION__));
		err = BCME_NOMEM;
		goto fail;
	}

	/* fill up the packet */
	body[HE_AF_CAT_OFF] = DOT11_ACTION_CAT_HE;
	body[HE_AF_ACT_OFF] = HE_ACTION_TWT_SETUP;
	body[HE_AF_TWT_SETUP_TOKEN_OFF] = dialog_token;

	wlc_twt_ie_build(wlc->twti, desc, 1,
		&body[HE_AF_TWT_SETUP_TWT_IE_OFF], body_len - HE_AF_TWT_SETUP_TWT_IE_OFF);

	/* -------- TWT Responding STA FSM -------- */
	sh->af_pend = WLC_HE_TWT_SETUP_ST_RESP_ACK;
	/* start timer now */
	sh->af_to = HE_AF_PEND_TO;
	sh->flow_id = desc->flow_id;

	/* enable packet callback */
	WLF2_PCB1_REG(pkt, WLF2_PCB1_HE_AF);

	/* commit for transmission */
	if (!wlc_sendmgmt(wlc, pkt, wlc->active_queue, scb)) {
		HECNTRINC(hei, af_tx_errs);
		err = BCME_BUSY;
		pkt = NULL;
		goto fail;
	}

	return BCME_OK;

fail:
	if (pkt != NULL) {
		PKTFREE(wlc->osh, pkt, TRUE);
	}
	wlc_twt_release(wlc->twti, scb, desc->flow_id);
	return err;
}

/* TWT Setup frame processing */
static int
wlc_he_twt_setup_proc(wlc_he_info_t *hei, struct scb *scb, uint8 *body, uint body_len)
{
	wlc_info_t *wlc = hei->wlc;
	wl_twt_sdesc_t desc;
	uint8 dialog_token;
	int err;
	scb_he_t *sh;
	wlc_he_twt_setup_cplt_data_t cplt_data;
	bool resp;

	if (!TWT_ENAB(wlc->pub)) {
		return BCME_UNSUPPORTED;
	}

	if (body_len <= HE_AF_TWT_SETUP_TWT_IE_OFF) {
		return BCME_BUFTOOSHORT;
	}

	dialog_token = body[HE_AF_TWT_SETUP_TOKEN_OFF];

	/* process the IE and extract the Setup parameters */
	if ((err = wlc_twt_ie_proc(wlc->twti, scb,
			(twt_ie_top_t *)&body[HE_AF_TWT_SETUP_TWT_IE_OFF],
			body_len - HE_AF_TWT_SETUP_TWT_IE_OFF,
			&desc, &resp)) != BCME_OK) {
		return err;
	}

	/* N.B.: desc.may_resp indicates if the received frame is Request or Response */
	/* we're receiving a Request frame... */
	if (resp) {
		return wlc_he_twt_setup_resp(hei, scb, &desc, dialog_token);
	}

	/* we're receiving a "Response"... */
	sh = SCB_HE(hei, scb);
	ASSERT(sh != NULL);

	/* toss unsolicited responses */
	if (dialog_token != sh->twt_req.dialog_token) {
		HECNTRINC(hei, af_rx_usols);
		return BCME_OK;
	}

	/* TWT Requesting STA FSM */
	switch (sh->af_pend) {
	case WLC_HE_TWT_SETUP_ST_REQ_ACK:
	case WLC_HE_TWT_SETUP_ST_RESP:
		break;
	default:
		ASSERT(0);
		break;
	}

	/* TODO: check the Setup Command in the response and handle reject... */

	/* notifying the user of TWT Setup completion */

	/* reset the timeout timer */
	sh->af_to = HE_AF_PEND_TO_DIS;

	/* invoke the callback */
	cplt_data.status = BCME_OK;
	cplt_data.dialog_token = dialog_token;
	cplt_data.desc = desc;

	(sh->twt_req.cplt_fn)(sh->twt_req.cplt_ctx, scb, &cplt_data);

	/* move the state */
	sh->af_pend = WLC_HE_TWT_SETUP_ST_NOTIF;

	/* program the HW */
	return wlc_twt_setup(wlc->twti, scb, &desc);
}

/* send TWT Teardown frame */
int
wlc_he_twt_teardown(wlc_he_info_t *hei, struct scb *scb, uint8 flow_id)
{
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	void *pkt;
	uint8 *body;
	uint body_len;
	scb_he_t *sh;
	int err;

	if (!TWT_ENAB(wlc->pub)) {
		return BCME_UNSUPPORTED;
	}

	/* bail out if FSM is in progress */
	sh = SCB_HE(hei, scb);
	ASSERT(sh != NULL);

	if (sh->af_to > 0) {
		WL_HE_ERR(("wl%d: %s: FSM is busy\n", WLUNIT(hei), __FUNCTION__));
		HECNTRINC(hei, fsm_busy_errs);
		return BCME_BUSY;
	}

	body_len = HE_AF_TWT_TEARDOWN_FLOW_OFF + wlc_twt_teardown_len_calc(wlc->twti);

	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg != NULL);

	/* allocate the packet */
	if ((pkt = wlc_frame_get_action(wlc, &scb->ea, &cfg->cur_etheraddr,
			&cfg->BSSID, body_len, &body, DOT11_ACTION_CAT_HE)) == NULL) {
		WL_ERROR(("wl%d: %s: unable to create TWT Setup frame\n",
		          wlc->pub->unit, __FUNCTION__));
		err = BCME_NOMEM;
		goto fail;
	}

	/* fill up the packet */
	body[HE_AF_CAT_OFF] = DOT11_ACTION_CAT_HE;
	body[HE_AF_ACT_OFF] = HE_ACTION_TWT_TEARDOWN;

	wlc_twt_teardown_init(wlc->twti, flow_id,
		&body[HE_AF_TWT_TEARDOWN_FLOW_OFF], body_len - HE_AF_TWT_TEARDOWN_FLOW_OFF);

	/* TWT Teardown FSM */
	sh->af_pend = WLC_HE_TWT_TEARDOWN_ST_ACK;
	/* start timer now */
	sh->af_to = HE_AF_PEND_TO;
	sh->flow_id = flow_id;

	/* enable packet callback */
	WLF2_PCB1_REG(pkt, WLF2_PCB1_HE_AF);

	/* commit for transmission */
	if (!wlc_sendmgmt(wlc, pkt, wlc->active_queue, scb)) {
		HECNTRINC(hei, af_tx_errs);
		err = BCME_BUSY;
		pkt = NULL;
		goto fail;
	}

	return BCME_OK;

fail:
	if (pkt != NULL) {
		PKTFREE(wlc->osh, pkt, TRUE);
	}
	return err;
}

/* TWT Teardown frame processing */
static int
wlc_he_twt_teardown_proc(wlc_he_info_t *hei, struct scb *scb, uint8 *body, uint body_len)
{
	wlc_info_t *wlc = hei->wlc;

	if (!TWT_ENAB(wlc->pub)) {
		return BCME_UNSUPPORTED;
	}

	if (body_len <= HE_AF_TWT_TEARDOWN_FLOW_OFF) {
		return BCME_BUFTOOSHORT;
	}

	return wlc_twt_teardown_proc(wlc->twti, scb,
			&body[HE_AF_TWT_TEARDOWN_FLOW_OFF],
			body_len - HE_AF_TWT_TEARDOWN_FLOW_OFF);
}

/* send TWT Information */
int
wlc_he_twt_info(wlc_he_info_t *hei, struct scb *scb, wl_twt_idesc_t *desc)
{
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	void *pkt;
	uint8 *body;
	uint body_len;
	int err;

	if (!TWT_ENAB(wlc->pub)) {
		return BCME_UNSUPPORTED;
	}

	body_len = HE_AF_TWT_INFO_OFF + wlc_twt_info_len_calc(wlc->twti, desc);

	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg != NULL);

	/* allocate the packet */
	if ((pkt = wlc_frame_get_action(wlc, &scb->ea, &cfg->cur_etheraddr,
			&cfg->BSSID, body_len, &body, DOT11_ACTION_CAT_HE)) == NULL) {
		WL_ERROR(("wl%d: %s: unable to create TWT Setup frame\n",
		          wlc->pub->unit, __FUNCTION__));
		err = BCME_NOMEM;
		goto fail;
	}

	/* fill up the packet */
	body[HE_AF_CAT_OFF] = DOT11_ACTION_CAT_HE;
	body[HE_AF_ACT_OFF] = HE_ACTION_TWT_INFO;

	wlc_twt_info_init(wlc->twti, desc,
		&body[HE_AF_TWT_INFO_OFF], body_len - HE_AF_TWT_INFO_OFF);

	/* commit for transmission */
	if (!wlc_sendmgmt(wlc, pkt, wlc->active_queue, scb)) {
		HECNTRINC(hei, af_tx_errs);
		err = BCME_BUSY;
		pkt = NULL;
		goto fail;
	}

	return BCME_OK;

fail:
	if (pkt != NULL) {
		PKTFREE(wlc->osh, pkt, TRUE);
	}
	return err;
}

/* send TWT Information response */
static int
wlc_he_twt_info_resp(wlc_he_info_t *hei, struct scb *scb, wl_twt_idesc_t *desc)
{
	return wlc_he_twt_info(hei, scb, desc);
}

/* TWT Information frame processing */
static int
wlc_he_twt_info_proc(wlc_he_info_t *hei, struct scb *scb, uint8 *body, uint body_len)
{
	wlc_info_t *wlc = hei->wlc;
	wl_twt_idesc_t desc;
	int err;
	bool resp;

	if (!TWT_ENAB(wlc->pub)) {
		return BCME_UNSUPPORTED;
	}

	if (body_len <= HE_AF_TWT_INFO_OFF) {
		return BCME_BUFTOOSHORT;
	}

	/* parse TWT Information field in the incoming AF */
	if ((err = wlc_twt_info_proc(wlc->twti, scb,
			&body[HE_AF_TWT_INFO_OFF], body_len - HE_AF_TWT_INFO_OFF,
			&desc, &resp)) != BCME_OK) {
		return err;
	}

	/* respond to the request if needed */
	if (resp) {
		desc.flow_flags &= ~WL_TWT_INFO_FLAG_RESP_REQ;
		return wlc_he_twt_info_resp(hei, scb, &desc);
	}

	return err;
}

/* process action frame */
int
wlc_he_actframe_proc(wlc_he_info_t *hei, uint action_id, struct scb *scb,
	uint8 *body, int body_len)
{
	int err = BCME_OK;

	if (!HE_ENAB(hei->wlc->pub)) {
		return BCME_UNSUPPORTED;
	}

	switch (action_id) {
	case HE_ACTION_TWT_SETUP:
		err = wlc_he_twt_setup_proc(hei, scb, body, body_len);
		break;

	case HE_ACTION_TWT_TEARDOWN:
		err = wlc_he_twt_teardown_proc(hei, scb, body, body_len);
		break;

	case HE_ACTION_TWT_INFO:
		err = wlc_he_twt_info_proc(hei, scb, body, body_len);
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	if (err != BCME_OK) {
		HECNTRINC(hei, af_rx_errs);
	}
	return err;
}

static uint8
wlc_he_get_frag_cap(wlc_info_t *wlc)
{
	/* currently no fragmentation supported.
	* TODO: update this capabililty once fragmentation supported by Phy
	*/
	return HE_FRAG_SUPPORT_NO_FRAG;
}

static uint8
wlc_he_get_ppet_info(wlc_info_t *wlc, uint8 *ppet)
{

	/* TODO: No sufficient info yet on how to fill this field. */
	return 0;
}

#endif /* WL11AX */
