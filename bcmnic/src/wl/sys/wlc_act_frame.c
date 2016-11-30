/**
 * @file
 * @brief
 * Action frame functions
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
 * $Id: wlc_act_frame.c 645620 2016-06-24 22:29:00Z $
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <osl.h>
#include <wlioctl.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_tx.h>
#include <wlc_msch.h>
#include <wlc_pcb.h>
#include <wlc_act_frame.h>
#include <wlc_quiet.h>
#include <wlc_scb.h>
#include <wlc_ulb.h>
#include <wlc_event_utils.h>

#ifdef WLRSDB
#include <wlc_rsdb.h>
#ifdef WLMCNX
#include <wlc_mcnx.h>
#endif /* WLMCNX */
#endif /* WLRSDB */

#define ACTFRAME_MESSAGE(x)

/* iovar table */
enum {
	IOV_ACTION_FRAME = 0,	/* Wifi protocol action frame */
	IOV_AF = 1
};

static const bcm_iovar_t actframe_iovars[] = {
	{"wifiaction", IOV_ACTION_FRAME,
	(0), 0, IOVT_BUFFER, WL_WIFI_ACTION_FRAME_SIZE
	},
	{"actframe", IOV_AF,
	(0), 0, IOVT_BUFFER, OFFSETOF(wl_af_params_t, action_frame) +
	OFFSETOF(wl_action_frame_t, data)
	},
	{NULL, 0, 0, 0, 0, 0}
};

struct wlc_act_frame_info {
	wlc_info_t *wlc;		/* pointer to main wlc structure */
	int32 cfg_cubby_hdl;		/* BSSCFG cubby offset */
};

typedef struct act_frame_cubby {
	int32			af_dwell_time;
	void			*action_frame;		/* action frame for off channel */
	wlc_msch_req_handle_t	*req_msch_actframe_hdl;	/* hdl to msch request */
	wlc_actframe_callback	cb_func;		/* callback function */
	void			*cb_ctxt;		/* callback context to be passed back */
	wlc_act_frame_tx_cb_fn_t af_tx_cb;		/* optional cb func for specfic needs */
	void			*arg;			/* cb context for the optional cb func */
	void			*wlc_handler;		/* wlc's MSCH used to send act frame. */
} act_frame_cubby_t;

#define ACTION_FRAME_IN_PROGRESS(act_frame_cubby)	((act_frame_cubby)->action_frame != NULL)

/* bsscfg states access macros */
#define BSSCFG_ACT_FRAME_CUBBY_LOC(act_frame_info, cfg) \
		((act_frame_cubby_t **)BSSCFG_CUBBY((cfg), (act_frame_info)->cfg_cubby_hdl))

#define BSSCFG_ACT_FRAME_CUBBY(act_frame_info, cfg) \
		(*BSSCFG_ACT_FRAME_CUBBY_LOC(act_frame_info, cfg))

static int wlc_act_frame_init(void *ctx, wlc_bsscfg_t *cfg);
static void wlc_act_frame_deinit(void *ctx, wlc_bsscfg_t *cfg);

#ifdef STA
static int wlc_actframe_send_action_frame_cb(void* handler_ctxt, wlc_msch_cb_info_t *cb_info);
#endif
static void *wlc_prepare_action_frame(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	const struct ether_addr *bssid, void *action_frame);
static void wlc_actionframetx_complete(wlc_info_t *wlc, void *pkt, uint txstatus);

static int wlc_act_frame_doiovar(void *ctx, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint val_size,
	struct wlc_if *wlcif);
static int wlc_msch_af_send(void* handler_ctxt, wlc_msch_cb_info_t *cb_info);

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

wlc_act_frame_info_t *
BCMATTACHFN(wlc_act_frame_attach)(wlc_info_t *wlc)
{
	wlc_act_frame_info_t *act_frame_info;

	act_frame_info = MALLOCZ(wlc->osh, sizeof(wlc_act_frame_info_t));

	if (!act_frame_info) {
		WL_ERROR(("%s: Alloc failure for act_frame_info\n", __FUNCTION__));
		return NULL;
	}

	/* reserve cubby in the bsscfg container for private data */
	if ((act_frame_info->cfg_cubby_hdl =
			wlc_bsscfg_cubby_reserve(wlc, sizeof(act_frame_cubby_t *),
			wlc_act_frame_init, wlc_act_frame_deinit, NULL,
			(void *)act_frame_info)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register packet class callback */
	if (wlc_pcb_fn_set(wlc->pcb, 0, WLF2_PCB1_AF, wlc_actionframetx_complete) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_pcb_fn_set(af) failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* action frame ioctls & iovars */
	if (wlc_module_register(wlc->pub, actframe_iovars, "actframe", act_frame_info,
		wlc_act_frame_doiovar, NULL, NULL, NULL) != BCME_OK) {

		WL_ERROR(("wl%d: actframe wlc_module_register() failed\n",
		          wlc->pub->unit));
		goto fail;
	}

	act_frame_info->wlc = wlc;
	return act_frame_info;

fail:
	if (act_frame_info) {
		MFREE(act_frame_info->wlc->osh, act_frame_info, sizeof(wlc_act_frame_info_t));
	}
	return NULL;
}

void
BCMATTACHFN(wlc_act_frame_detach)(wlc_act_frame_info_t *act_frame_info)
{
	ASSERT(act_frame_info);
	wlc_module_unregister(act_frame_info->wlc->pub, "actframe", act_frame_info);
	MFREE(act_frame_info->wlc->osh, act_frame_info, sizeof(wlc_act_frame_info_t));
}

/* bsscfg cubby */
static int
wlc_act_frame_init(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_act_frame_info_t *act_frame_info = (wlc_act_frame_info_t *)ctx;
	wlc_info_t *wlc;
	act_frame_cubby_t **act_frame_loc;
	act_frame_cubby_t *act_frame_cubby_info;

	wlc = cfg->wlc;

	/* allocate cubby info */
	if ((act_frame_cubby_info = MALLOCZ(wlc->osh, sizeof(act_frame_cubby_t))) == NULL) {
		WL_ERROR(("wl%d: %s: MALLOC failed, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}

	act_frame_loc = BSSCFG_ACT_FRAME_CUBBY_LOC(act_frame_info, cfg);
	*act_frame_loc = act_frame_cubby_info;

	return BCME_OK;
}

static void
wlc_act_frame_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_act_frame_info_t *act_frame_info = (wlc_act_frame_info_t *)ctx;
	wlc_info_t *wlc;
	act_frame_cubby_t *act_frame_cubby;

	wlc = act_frame_info->wlc;
	act_frame_cubby = BSSCFG_ACT_FRAME_CUBBY(act_frame_info, cfg);

	if (act_frame_cubby == NULL) {
		return;
	}

	if (act_frame_cubby->req_msch_actframe_hdl) {
		wlc_msch_timeslot_unregister(wlc->msch_info,
			&act_frame_cubby->req_msch_actframe_hdl);
	}

	MFREE(wlc->osh, act_frame_cubby, sizeof(act_frame_cubby_t));
}


/* iovar dispatch */
static int
wlc_act_frame_doiovar(void *ctx, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif)
{
	wlc_act_frame_info_t *act_frame_info = (wlc_act_frame_info_t *)ctx;
	wlc_info_t *wlc;
	act_frame_cubby_t *actframe_cubby;

	int err = BCME_OK;
	wlc_bsscfg_t *bsscfg;

	ASSERT(act_frame_info != NULL);
	wlc = act_frame_info->wlc;
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);

	actframe_cubby = BSSCFG_ACT_FRAME_CUBBY(act_frame_info, bsscfg);
	ASSERT(actframe_cubby != NULL);

	/* Do the actual parameter implementation */
	switch (actionid) {

	case IOV_SVAL(IOV_ACTION_FRAME):
		if (wlc->pub->up) {
			/* Send frame to remote client/server */
			err = wlc_send_action_frame(wlc, bsscfg, NULL, arg);
		} else {
			err = BCME_NOTUP;
		}
		break;
	case IOV_SVAL(IOV_AF): {
#ifdef STA
		bool af_continue;
#endif /* STA */
		bool do_event = TRUE;

		wl_af_params_t *af = NULL;

		if (len > WL_WIFI_AF_PARAMS_SIZE) {
			err = BCME_BUFTOOLONG;
			break;
		}

		af = MALLOC(wlc->osh, WL_WIFI_AF_PARAMS_SIZE);
		if (!af) {
			WL_ERROR(("wl%d: %s: af malloc failed\n", wlc->pub->unit, __FUNCTION__));
			err = BCME_NOMEM;
			break;
		}
		memcpy(af, arg, len);
		do {
			if (BSSCFG_AP(bsscfg)) {
				if (!wlc->pub->up) {
					WL_ERROR(("wl%d: %s: not up\n",
						wlc->pub->unit, __FUNCTION__));
					err = BCME_NOTUP;
					break;
				}
				/* Send frame to remote client/server */
				err = wlc_send_action_frame(wlc, bsscfg, &bsscfg->BSSID,
					&af->action_frame);
				do_event = FALSE;
				break;
			}

#ifdef STA
			af_continue = FALSE;

			/* Piggyback on current scan channel if the user
			 * does not care about the channel
			 */
			if (af->channel == 0 ||
				af->channel == wf_chspec_ctlchan(WLC_BAND_PI_RADIO_CHANSPEC)) {
				af_continue = TRUE;
			}

			if (WLFMC_ENAB(wlc->pub) && af->channel == 0) {
				af_continue = TRUE;
			}

			if (!af_continue) {
				if (ACTION_FRAME_IN_PROGRESS(actframe_cubby)) {
					WL_ERROR(("Action frame is in progress\n"));
					err = BCME_BUSY;
					break;
				}
				err = wlc_send_action_frame_off_channel(wlc, bsscfg,
					BSSCFG_MINBW_CHSPEC(wlc, bsscfg, af->channel),
					af->dwell_time, &af->BSSID, &af->action_frame, NULL, NULL);
				do_event = FALSE;
				break;
			}

			if (!wlc->pub->up) {
				err = BCME_NOTUP;
				break;
			}

			/* If current channel is a 'quiet' channel then reject */
			if (BSS_QUIET_STATE(wlc->quiet, bsscfg) & SILENCE) {
				err = BCME_NOTREADY;
				break;
			}

			/* Send frame to remote client/server */
			err = wlc_send_action_frame(wlc, bsscfg, &af->BSSID, &af->action_frame);

			do_event = FALSE;
#endif /* STA */
		} while (FALSE);

		if (do_event) {
			/* Send back the Tx status to the host */
			wlc_bss_mac_event(wlc, bsscfg, WLC_E_ACTION_FRAME_COMPLETE,
				NULL, WLC_E_STATUS_NO_ACK, 0, 0,
				&af->action_frame.packetId, sizeof(af->action_frame.packetId));
		}
		MFREE(wlc->osh, af, WL_WIFI_AF_PARAMS_SIZE);
		break;
	}
	default:
		WL_ERROR(("wl%d: %s: Command unsupported", wlc->pub->unit, __FUNCTION__));
		err = BCME_UNSUPPORTED;
		break;
	}
	return err;
}

#ifdef STA
static int
wlc_actframe_send_action_frame_cb(void* handler_ctxt, wlc_msch_cb_info_t *cb_info)
{
	wlc_bsscfg_t *cfg = (wlc_bsscfg_t *)handler_ctxt;
	uint32 packetId;
	act_frame_cubby_t *actframe_cubby;
	wlc_msch_onchan_info_t *onchan;
	wlc_info_t *wlc;
	ASSERT(cfg);
	actframe_cubby = BSSCFG_ACT_FRAME_CUBBY(cfg->wlc->wlc_act_frame_info, cfg);

	wlc = (wlc_info_t*)actframe_cubby->wlc_handler;
	ASSERT(wlc);
	if (cb_info->type & MSCH_CT_SLOT_START) {

		if (!ACTION_FRAME_IN_PROGRESS(actframe_cubby)) {
			WL_ERROR(("wl%d: Action frame in not progress\n", wlc->pub->unit));
			goto fail;
		}

		onchan = (wlc_msch_onchan_info_t *)cb_info->type_specific;

#ifdef WLP2P
		if (BSS_P2P_ENAB(wlc, cfg)) {
			/* Set time of expiry over dwell time(ms) */
			wlc_lifetime_set(wlc, actframe_cubby->action_frame,
				(actframe_cubby->af_dwell_time*2)*1000);
		}
#endif
		/* if only one request happens at the corresponding timeslot
		 * and excursion queue is not active, use excursion queue
		*/
		if (!wlc_msch_query_ts_shared(wlc->msch_info, onchan->timeslot_id) &&
			!wlc->excursion_active) {
			/* suspend normal tx queue operation for channel excursion */
			wlc_suspend_mac_and_wait(wlc);
			wlc_excursion_start(wlc);
			wlc_enable_mac(wlc);
		}
#if defined(WLRSDB) && defined(WLMCNX)
		/* If we are using wlc which is not hosting the bsscfg, then program the AMT
		 * to receive response action frame with da as bsscfg->cur_etheraddr
		 */
		if (RSDB_ENAB(wlc->pub) && MCNX_ENAB(wlc->pub) && wlc != cfg->wlc) {
			wlc_mcnx_alt_ra_set(wlc->mcnx, cfg);
		}
#endif /* defined(WLRSDB) && defined(WLMCNX) */

		if (actframe_cubby->af_tx_cb) {
			actframe_cubby->af_tx_cb(wlc, actframe_cubby->arg,
				actframe_cubby->action_frame);
		} else {
			if (wlc_tx_action_frame_now(wlc, cfg, actframe_cubby->action_frame,
				NULL) != BCME_OK) {
				WL_ERROR(("wl%d: Could not send out action frame\n",
					wlc->pub->unit));
				goto fail;
			}
		}
	}
	if (cb_info->type &  MSCH_CT_SLOT_END) {
		ASSERT(cb_info->type & MSCH_CT_REQ_END);
		/* if AF is aborted */
		if (actframe_cubby->action_frame) {
			packetId = WLPKTTAG(actframe_cubby->action_frame)->shared.packetid;
			wlc_bss_mac_event(wlc, cfg, WLC_E_ACTION_FRAME_COMPLETE, NULL,
				WLC_E_STATUS_NO_ACK, 0, 0, &packetId, sizeof(packetId));
		}

		/* Send back the channel switch indication to the host */
		wlc_bss_mac_event(wlc, cfg, WLC_E_ACTION_FRAME_OFF_CHAN_COMPLETE,
			NULL, 0, 0, 0, 0, 0);

		actframe_cubby->req_msch_actframe_hdl = NULL;

#if defined(WLRSDB) && defined(WLMCNX)
		/* If we are using wlc which is not hosting the bsscfg, then program the AMT
		 * to receive response action frame with da as bsscfg->cur_etheraddr
		 */
		if (RSDB_ENAB(wlc->pub) && MCNX_ENAB(wlc->pub) && wlc != cfg->wlc) {
			wlc_mcnx_alt_ra_unset(wlc->mcnx, cfg);
		}
		actframe_cubby->wlc_handler = NULL;
#endif /* defined(WLRSDB) && defined(WLMCNX) */

		if (wlc->excursion_active) {
			wlc_suspend_mac_and_wait(wlc);
			/* resume normal tx queue operation */
			wlc_excursion_end(wlc);
			wlc_enable_mac(wlc);
		}

		/* Bringup down the core once done */
		wlc_mpc_off_req_set(wlc, MPC_OFF_REQ_TX_ACTION_FRAME, 0);

	}
	return BCME_OK;

fail:
	if (actframe_cubby->action_frame) {
		PKTFREE(wlc->osh, actframe_cubby->action_frame, TRUE);
		actframe_cubby->action_frame = NULL;
	}
	if (wlc->excursion_active) {
		wlc_suspend_mac_and_wait(wlc);
		wlc_excursion_end(wlc);
		wlc_enable_mac(wlc);
	}
	if (actframe_cubby->req_msch_actframe_hdl) {
		if (wlc_msch_timeslot_unregister(wlc->msch_info,
			&actframe_cubby->req_msch_actframe_hdl) != BCME_OK) {
			WL_ERROR(("wl%d: %s: MSCH timeslot unregister failed\n",
				wlc->pub->unit, __FUNCTION__));
		}
		actframe_cubby->req_msch_actframe_hdl = NULL;
	}
	actframe_cubby->wlc_handler = NULL;

	/* Bringup down the core once done */
	wlc_mpc_off_req_set(wlc, MPC_OFF_REQ_TX_ACTION_FRAME, 0);

	return BCME_ERROR;
}

#endif /* STA */

static void *
wlc_prepare_action_frame(wlc_info_t *wlc, wlc_bsscfg_t *cfg, const struct ether_addr *bssid,
	void *action_frame)
{
	void *pkt;
	uint8* pbody;
	uint16 body_len;
	struct ether_addr da;
	uint32 packetId;
	wlc_pkttag_t *pkttag;
	uint8 category;

	memcpy(&packetId, (uint8*)action_frame +
		OFFSETOF(wl_action_frame_t, packetId), sizeof(uint32));

	memcpy(&body_len, (uint8*)action_frame + OFFSETOF(wl_action_frame_t, len),
		sizeof(body_len));

	memcpy(&da, (uint8*)action_frame + OFFSETOF(wl_action_frame_t, da), ETHER_ADDR_LEN);

	if (bssid == NULL) {
		bssid = (WLC_BSS_CONNECTED(cfg)) ? &cfg->BSSID : &ether_bcast;
	}
	pkt = NULL;
	if (body_len) {
		/* get category */
		category = ((wl_action_frame_t *)action_frame)->data[0];
		/* get action frame */
		if ((pkt = wlc_frame_get_action(wlc, &da, &cfg->cur_etheraddr,
			bssid, body_len, &pbody, category)) == NULL) {
			return NULL;
		}

		pkttag = WLPKTTAG(pkt);
		pkttag->shared.packetid = packetId;
		WLPKTTAGBSSCFGSET(pkt, cfg->_idx);

		memcpy(pbody, (uint8*)action_frame + OFFSETOF(wl_action_frame_t, data), body_len);
	}

	return pkt;
}

int
wlc_tx_action_frame_now(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt, struct scb *scb)
{
	ratespec_t rate_override = 0;
	struct dot11_management_header *hdr;
	hdr = (struct dot11_management_header *)PKTDATA(wlc->osh, pkt);

	if (!ETHER_ISMULTI(&hdr->da) && !scb) {
		scb = wlc_scbfind(wlc, cfg, &hdr->da);
		if (scb && !SCB_ASSOCIATED(scb))
			scb = NULL;
	}
	if (BSS_P2P_ENAB(wlc, cfg)) {
		rate_override = WLC_RATE_6M;
	} else {
		rate_override = WLC_LOWEST_BAND_RSPEC(wlc->band);
	}

	/* register packet callback */
	WLF2_PCB1_REG(pkt, WLF2_PCB1_AF);

	if (!wlc_queue_80211_frag(wlc, pkt, wlc->active_queue, scb, cfg,
		FALSE, NULL, rate_override)) {
		WL_ERROR(("%s, wlc_queue_80211_frag failed", __FUNCTION__));
		return BCME_ERROR;
	}

	return BCME_OK;
} /* wlc_tx_action_frame_now */

#ifdef STA
int
wlc_send_action_frame_off_channel(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, chanspec_t chanspec,
	int32 dwell_time, struct ether_addr *bssid, wl_action_frame_t *action_frame,
	wlc_act_frame_tx_cb_fn_t cb, void *arg)
{
	chanspec_t chanspec_list[1];
	int err = BCME_OK;
	wlc_msch_req_param_t req_param;
	act_frame_cubby_t *act_frame_cubby;
#ifdef WLRSDB
	if (RSDB_ENAB(wlc->pub) && WLC_RSDB_DUAL_MAC_MODE(WLC_RSDB_CURR_MODE(wlc))) {
		wlc_info_t *wlc_2g, *wlc_5g;

		wlc_rsdb_get_wlcs(wlc, &wlc_2g, &wlc_5g);

		/* Note - The below code overwrite the wlc. Any wlc related code should be added
		 * after #endif WLRSDB.
		 */
		if (CHSPEC_IS2G(chanspec)) {
			wlc = wlc_2g;
		} else {
			wlc = wlc_5g;
		}
	}
#endif /* WLRSDB */

	act_frame_cubby = BSSCFG_ACT_FRAME_CUBBY(wlc->wlc_act_frame_info, bsscfg);

	act_frame_cubby->action_frame = wlc_prepare_action_frame(wlc, bsscfg, bssid, action_frame);

	if (!act_frame_cubby->action_frame) {
		ACTFRAME_MESSAGE(("%s, action_frame is NULL\n", __FUNCTION__));
		wlc_bss_mac_event(wlc, bsscfg, WLC_E_ACTION_FRAME_COMPLETE,
			NULL, WLC_E_STATUS_NO_ACK, 0, 0,
			&action_frame->packetId, sizeof(action_frame->packetId));
		return BCME_NOMEM;
	}

	/* Bringup the core for the duration of off channel Action Frame Event */
	wlc_mpc_off_req_set(wlc, MPC_OFF_REQ_TX_ACTION_FRAME, MPC_OFF_REQ_TX_ACTION_FRAME);

	act_frame_cubby->af_tx_cb = cb;
	act_frame_cubby->arg = arg;
	act_frame_cubby->wlc_handler = wlc;

	chanspec_list[0] = chanspec;

	bzero(&req_param, sizeof(req_param));
	req_param.req_type = MSCH_RT_START_FLEX;
	req_param.duration = MS_TO_USEC(dwell_time);
	req_param.priority = MSCH_DEFAULT_PRIO;

	act_frame_cubby->af_dwell_time = dwell_time;
	if ((err = wlc_msch_timeslot_register(wlc->msch_info,
		&chanspec_list[0], 1, wlc_actframe_send_action_frame_cb, bsscfg, &req_param,
		&act_frame_cubby->req_msch_actframe_hdl)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: MSCH timeslot register failed, err %d\n",
			wlc->pub->unit, __FUNCTION__, err));

		if (act_frame_cubby->action_frame) {
			PKTFREE(wlc->osh, act_frame_cubby->action_frame, TRUE);
			act_frame_cubby->action_frame = NULL;
		}
	}
	return err;
}
#endif /* STA */

int
wlc_send_action_frame(wlc_info_t *wlc, wlc_bsscfg_t *cfg, const struct ether_addr *bssid,
	void *action_frame)
{
	void *pkt = wlc_prepare_action_frame(wlc, cfg, bssid, action_frame);

	if (!pkt) {
		return BCME_NOMEM;
	}

	return wlc_tx_action_frame_now(wlc, cfg, pkt, NULL);
}

static void
wlc_actionframetx_complete(wlc_info_t *wlc, void *pkt, uint txstatus)
{
	int err;
	int idx;
	wlc_bsscfg_t *cfg;
	uint status = WLC_E_STATUS_SUCCESS;
	uint32 packetId;
	act_frame_cubby_t *actframe_cubby;

	if (!(txstatus & TX_STATUS_ACK_RCV))
		status = WLC_E_STATUS_NO_ACK;

	packetId = WLPKTTAG(pkt)->shared.packetid;

	idx = WLPKTTAGBSSCFGGET(pkt);
	if ((cfg = wlc_bsscfg_find(wlc, idx, &err)) == NULL) {
		WL_ERROR(("wl%d: %s, wlc_bsscfg_find failed\n",
			wlc->pub->unit, __FUNCTION__));
		return;
	}
	actframe_cubby = BSSCFG_ACT_FRAME_CUBBY(wlc->wlc_act_frame_info, cfg);


	/* Slot End comes before tx_complete callback function */
	if (actframe_cubby->action_frame && !actframe_cubby->req_msch_actframe_hdl) {
		ACTFRAME_MESSAGE(("%s, slot end comes before tx_complete\n", __FUNCTION__));
		actframe_cubby->action_frame = NULL;
		return;
	}

	/* Send back the Tx status to the host */
	wlc_bss_mac_event(wlc, cfg, WLC_E_ACTION_FRAME_COMPLETE, NULL, status,
		0, 0, &packetId, sizeof(packetId));
	actframe_cubby->action_frame = NULL;

	return;
}

static int
wlc_msch_af_send(void* handler_ctxt, wlc_msch_cb_info_t *cb_info)
{
	wlc_bsscfg_t *bsscfg = (wlc_bsscfg_t *)handler_ctxt;
	act_frame_cubby_t *actframe_cubby;
	ASSERT(bsscfg);

	actframe_cubby = BSSCFG_ACT_FRAME_CUBBY(bsscfg->wlc->wlc_act_frame_info, bsscfg);

	if (cb_info->type & MSCH_CT_SLOT_START) {
		if (actframe_cubby->cb_func) {
			actframe_cubby->cb_func(bsscfg->wlc,
				actframe_cubby->cb_ctxt, (uint *)(&actframe_cubby->af_dwell_time));
		}
	}
	if (cb_info->type &  MSCH_CT_SLOT_END) {
		ASSERT(cb_info->type & MSCH_CT_REQ_END);
	}
	return BCME_OK;
}

int
wlc_msch_actionframe(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint32 channel,
	uint32 dwell_time, struct ether_addr *bssid, wlc_actframe_callback cb_func, void *arg)
{
	chanspec_t chanspec_list[1];
	int err = BCME_OK;
	wlc_msch_req_param_t req_param;
	act_frame_cubby_t *act_frame_cubby;
	act_frame_cubby = BSSCFG_ACT_FRAME_CUBBY(wlc->wlc_act_frame_info, bsscfg);

	chanspec_list[0] = (channel == 0)? WLC_BAND_PI_RADIO_CHANSPEC :
		CH20MHZ_CHSPEC(channel);

	bzero(&req_param, sizeof(req_param));
	req_param.req_type = MSCH_RT_START_FLEX;
	req_param.duration = dwell_time;
	req_param.priority = MSCH_DEFAULT_PRIO;

	act_frame_cubby->af_dwell_time = dwell_time;
	act_frame_cubby->cb_func = cb_func;
	act_frame_cubby->cb_ctxt = arg;

	if ((err = wlc_msch_timeslot_register(wlc->msch_info,
		&chanspec_list[0], 1, wlc_msch_af_send, bsscfg, &req_param,
		&act_frame_cubby->req_msch_actframe_hdl)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: MSCH timeslot register failed, err %d\n",
			wlc->pub->unit, __FUNCTION__, err));
	}
	return err;
}

/* TODO: implement this function */
bool
wlc_act_frame_tx_inprog(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	return FALSE;
}
