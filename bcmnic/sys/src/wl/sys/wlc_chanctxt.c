/*
 * Common (OS-independent) portion of
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_chanctxt.c 613017 2016-01-15 23:02:23Z $
 */

/* Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <proto/802.11.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_bsscfg.h>
#include <wlc_bsscfg_psq.h>
#include <wlc.h>
#include <wlc_bmac.h>
#include <wlc_assoc.h>
#include <wlc_tx.h>
#ifdef PROP_TXSTATUS
#include <wlfc_proto.h>
#include <wlc_ampdu.h>
#include <wlc_apps.h>
#include <wlc_wlfc.h>
#include <wlc_scb.h>
#endif /* PROP_TXSTATUS */
#ifdef WLMCHAN
#include <wlc_mchan.h>
#include <wlc_p2p.h>
#endif /* WLMCHAN */
#include <wlc_lq.h>
#ifdef WLTDLS
#include <wlc_tdls.h>
#endif
#include <wlc_ulb.h>
#include <wlc_msch.h>
#include <wlc_chanctxt.h>
#include <wlc_ap.h>
#include <wlc_dfs.h>
#include <wlc_pm.h>
#include <wlc_srvsdb.h>
#include <wlc_sta.h>

#define SCHED_MAX_TIME_GAP	200
#define SCHED_TIMER_DELAY	100

/* bss chan context */
struct wlc_chanctxt {
	struct wlc_chanctxt *next;
	wlc_txq_info_t *qi;		/* tx queue */
	chanspec_t	chanspec;	/* channel specific */
	uint16		count;		/* count for txq attached */
};

struct wlc_chanctxt_info {
	wlc_info_t *wlc;		/* pointer to main wlc structure */
	wlc_chanctxt_t *chanctxt_list;	/* chan context link list */
	wlc_chanctxt_t *curr_chanctxt;	/* current chan context */
	int cfgh;			/* bsscfg cubby handle */
};

/* per BSS data */
typedef struct {
	wlc_chanctxt_t *chanctxt;	/* chan context for this bss */
	bool		on_channel;	/* whether the bss is on channel */
} chanctxt_bss_info_t;

/* locate per BSS data */
#define CHANCTXT_BSS_CUBBY_LOC(chanctxt_info, cfg) \
	((chanctxt_bss_info_t **)BSSCFG_CUBBY((cfg), (chanctxt_info)->cfgh))
#define CHANCTXT_BSS_INFO(chanctxt_info, cfg) \
	(*CHANCTXT_BSS_CUBBY_LOC(chanctxt_info, cfg))

/* macros */
#define WLC_SCHED_SAME_CTLCHAN(c1, c2) (wf_chspec_ctlchan(c1) == wf_chspec_ctlchan(c2))

/* local prototypes */
static void wlc_chanctxt_bsscfg_state_upd(void *ctx, bsscfg_state_upd_data_t *evt);
static int wlc_chanctxt_bss_init(void *ctx, wlc_bsscfg_t *cfg);
static void wlc_chanctxt_bss_deinit(void *ctx, wlc_bsscfg_t *cfg);
static bool wlc_chanctxt_in_use(wlc_info_t *wlc, wlc_chanctxt_t *chanctxt,
	wlc_bsscfg_t *filter_cfg);
static bool wlc_tx_qi_exist(wlc_info_t *wlc, wlc_txq_info_t *qi);
static wlc_txq_info_t *wlc_get_qi_with_no_context(wlc_info_t *wlc);
static void wlc_chanctxt_set_channel(wlc_chanctxt_info_t *chanctxt_info,
	wlc_chanctxt_t *chanctxt, chanspec_t chanspec, bool destroy_old);
static wlc_chanctxt_t *wlc_chanctxt_alloc(wlc_chanctxt_info_t *chanctxt_info,
	chanspec_t chanspec);
static void wlc_chanctxt_free(wlc_chanctxt_info_t *chanctxt_info, wlc_chanctxt_t *chanctxt);
static wlc_chanctxt_t *wlc_get_chanctxt(wlc_chanctxt_info_t *chanctxt_info,
	chanspec_t chanspec);
static bool wlc_all_cfg_chanspec_is_same(wlc_info_t *wlc,
	wlc_chanctxt_t *chanctxt, chanspec_t chanspec);
static wlc_chanctxt_t *wlc_chanctxt_create_txqueue(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	chanspec_t chanspec);
static int wlc_chanctxt_delete_txqueue(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint *oldqi_info);

#ifdef SRHWVSDB
static void wlc_chanctxt_srvsdb_upd(wlc_chanctxt_info_t *chanctxt_info);
#endif

wlc_chanctxt_info_t *
BCMATTACHFN(wlc_chanctxt_attach)(wlc_info_t *wlc)
{
	wlc_chanctxt_info_t *chanctxt_info;

	/* module states */
	chanctxt_info = (wlc_chanctxt_info_t *)MALLOCZ(wlc->osh, sizeof(wlc_chanctxt_info_t));
	if (!chanctxt_info) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}

	chanctxt_info->wlc = wlc;

	/* reserve cubby in the bsscfg container for per-bsscfg private data */
	if ((chanctxt_info->cfgh = wlc_bsscfg_cubby_reserve(wlc, sizeof(chanctxt_bss_info_t *),
		wlc_chanctxt_bss_init, wlc_chanctxt_bss_deinit, NULL, chanctxt_info)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	if (wlc_bsscfg_state_upd_register(wlc, wlc_chanctxt_bsscfg_state_upd, chanctxt_info)
		!= BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_state_upd_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	return chanctxt_info;

fail:
	/* error handling */
	wlc_chanctxt_detach(chanctxt_info);

	return NULL;
}

void
BCMATTACHFN(wlc_chanctxt_detach)(wlc_chanctxt_info_t *chanctxt_info)
{
	wlc_info_t *wlc;
	wlc_chanctxt_t *chanctxt_entity;

	ASSERT(chanctxt_info);
	wlc = chanctxt_info->wlc;

	/* delete all chan contexts */
	while ((chanctxt_entity = chanctxt_info->chanctxt_list)) {
		wlc_chanctxt_free(chanctxt_info, chanctxt_entity);
	}

	(void)wlc_bsscfg_state_upd_unregister(wlc, wlc_chanctxt_bsscfg_state_upd, chanctxt_info);

	MFREE(wlc->osh, chanctxt_info, sizeof(wlc_chanctxt_info_t));
}

static int
wlc_chanctxt_bss_init(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_chanctxt_info_t *chanctxt_info = (wlc_chanctxt_info_t *)ctx;
	wlc_info_t *wlc = chanctxt_info->wlc;
	chanctxt_bss_info_t **cbi_loc;
	chanctxt_bss_info_t *cbi;

	/* allocate cubby info */
	if ((cbi = MALLOCZ(wlc->osh, sizeof(chanctxt_bss_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: MALLOC failed, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}

	cbi_loc = CHANCTXT_BSS_CUBBY_LOC(chanctxt_info, cfg);
	*cbi_loc = cbi;

	return BCME_OK;
}

static void
wlc_chanctxt_bss_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_chanctxt_info_t *chanctxt_info = (wlc_chanctxt_info_t *)ctx;
	wlc_info_t *wlc = chanctxt_info->wlc;
	chanctxt_bss_info_t *cbi = CHANCTXT_BSS_INFO(chanctxt_info, cfg);

	if (cbi != NULL) {
		if (cbi->chanctxt) {
			wlc_chanctxt_delete_txqueue(wlc, cfg, NULL);
		}

		MFREE(wlc->osh, cbi, sizeof(chanctxt_bss_info_t));
	}
}

static void
wlc_chanctxt_bsscfg_state_upd(void *ctx, bsscfg_state_upd_data_t *evt)
{
	wlc_bsscfg_t *cfg = evt->cfg;
	ASSERT(cfg != NULL);

	if (evt->old_enable && !cfg->enable) {
		chanctxt_bss_info_t *cbi = CHANCTXT_BSS_INFO((wlc_chanctxt_info_t *)ctx, cfg);

		ASSERT(cbi);

		if (cbi->chanctxt) {
			wlc_chanctxt_delete_txqueue(cfg->wlc, cfg, NULL);
			cbi->chanctxt = NULL;
		}
	}
}

void
wlc_txqueue_start(wlc_bsscfg_t *cfg, chanspec_t chanspec, wlc_chanctxt_restore_fn_t restore_fn)
{
	wlc_info_t *wlc = cfg->wlc;
	wlc_chanctxt_info_t *chanctxt_info = wlc->chanctxt_info;
	chanctxt_bss_info_t *cbi = CHANCTXT_BSS_INFO(chanctxt_info, cfg);
	wlc_chanctxt_t *chanctxt;
	wlc_txq_info_t *qi;
	DBGONLY(char chanbuf[CHANSPEC_STR_LEN]; )

	WL_MQ(("wl%d.%d: %s: channel %s\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
		wf_chspec_ntoa(chanspec, chanbuf)));

	ASSERT(cbi);
	chanctxt = cbi->chanctxt;

	if (chanctxt == NULL || ((chanctxt->chanspec != chanspec) &&
		(!WLC_SCHED_SAME_CTLCHAN(chanctxt->chanspec, chanspec) ||
		CHSPEC_BW_GT(chanspec, CHSPEC_BW(chanctxt->chanspec))))) {
		chanctxt = wlc_chanctxt_create_txqueue(wlc, cfg, chanspec);
	}

	ASSERT(chanctxt);

	qi = chanctxt->qi;
	WL_MQ(("wl%d.%d: %s: qi %p, primary %p, active %p\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
		qi, wlc->primary_queue, wlc->active_queue));

	ASSERT(!wlc->excursion_active);

	/* Restore the bsscfg saving packets */
	if (restore_fn) {
		void *pkt;
		int prec;
		while ((pkt = restore_fn(cfg, &prec))) {
			WL_MQ(("Enq: pkt %p, prec %d\n", pkt, prec));
			pktq_penq(WLC_GET_TXQ(qi), prec, pkt);
		}
	}

	/* Attaches tx qi */
	chanctxt->count++;
	if (chanctxt->count == 1) {
		wlc_suspend_mac_and_wait(wlc);
		wlc_primary_queue_set(wlc, qi);
		wlc_enable_mac(wlc);

		WL_MQ(("wl%d.%d: %s: attach qi %p, primary %p, active %p\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
			qi, wlc->primary_queue, wlc->active_queue));
	}

	ASSERT(wlc->primary_queue == qi);

	if (chanctxt_info->curr_chanctxt != chanctxt && chanctxt_info->curr_chanctxt != NULL) {
		/* Tell CHANIM that we're about to switch channels */
		if (wlc_lq_chanim_adopt_bss_chan_context(wlc, chanctxt->chanspec,
			chanctxt_info->curr_chanctxt->chanspec) != BCME_OK) {
			WL_INFORM(("wl%d.%d %s: chanim adopt blocked scan/assoc/rm: \n",
			  wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__));
		}
	}

	chanctxt_info->curr_chanctxt = chanctxt;
	cbi->on_channel = TRUE;
}

/**
 * Context structure used by wlc_txqueue_end_filter() while filtering a pktq
 */
struct wlc_txqueue_end_filter_info {
	int cfgidx;         /**< index of bsscfg who's packets are being deleted */
	wlc_bsscfg_t *cfg;  /**< bsscfg who's packets are being deleted */
	wlc_chanctxt_backup_fn_t backup_fn;  /**< function that takes possesion of filtered pkts */
	int prec;           /**< prec currently being filtered */
};

/**
 * pktq filter function to filter out pkts associated with a BSSCFG index,
 * passing them to a "backup" function.
 * Used by wlc_txqueue_end().
 */
static pktq_filter_result_t
wlc_txqueue_end_filter(void* ctx, void* pkt)
{
	struct wlc_txqueue_end_filter_info *info = (struct wlc_txqueue_end_filter_info *)ctx;
	int idx;
	pktq_filter_result_t ret;

	/* if the bsscfg index matches, filter out the packet and pass to the backup_fn */
	idx = WLPKTTAGBSSCFGGET(pkt);
	if (idx == info->cfgidx) {
		WL_MQ(("Deq: pkt %p, prec %d\n", pkt, info->prec));
		(info->backup_fn)(info->cfg, pkt, info->prec);
		ret = PKT_FILTER_REMOVE;
	} else {
		ret = PKT_FILTER_NOACTION;
	}

	return ret;
}

void
wlc_txqueue_end(wlc_bsscfg_t *cfg, wlc_chanctxt_backup_fn_t backup_fn)
{
	wlc_info_t *wlc = cfg->wlc;
	wlc_chanctxt_info_t *chanctxt_info = wlc->chanctxt_info;
	chanctxt_bss_info_t *cbi = CHANCTXT_BSS_INFO(chanctxt_info, cfg);
	wlc_chanctxt_t *chanctxt;
	wlc_txq_info_t *qi;

	ASSERT(cbi);
	chanctxt = cbi->chanctxt;

	ASSERT(chanctxt != NULL && chanctxt == chanctxt_info->curr_chanctxt);
	if (!cbi->on_channel)
		return;

#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
		wlc_wlfc_mchan_interface_state_update(wlc, cfg,
			WLFC_CTL_TYPE_INTERFACE_CLOSE, FALSE);
	}
#endif /* PROP_TXSTATUS */

	qi = chanctxt->qi;
	WL_MQ(("wl%d.%d: %s: qi %p, primary %p, active %p\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
		qi, wlc->primary_queue, wlc->active_queue));

	/* Detaches tx qi */
	chanctxt->count--;
	if (chanctxt->count == 0) {
		wlc_suspend_mac_and_wait(wlc);
		wlc_primary_queue_set(wlc, NULL);
		wlc_enable_mac(wlc);

		WL_MQ(("wl%d.%d: %s: detach qi %p, primary %p, active %p\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
			qi, wlc->primary_queue, wlc->active_queue));
	}

	/* Backup the bsscfg packets and saving them */
	if (backup_fn && !pktq_empty(WLC_GET_TXQ(qi))) {
		struct wlc_txqueue_end_filter_info info;
		struct pktq *pq;
		int prec;

		WL_MQ(("wl%d.%d: %s: Backup the bsscfg packets and saving them\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__));

		info.cfgidx = WLC_BSSCFG_IDX(cfg);
		info.cfg = cfg;
		info.backup_fn = backup_fn;

		/* Dequeue the packets from tx qi that belong to the bsscfg */
		pq = WLC_GET_TXQ(qi);
		PKTQ_PREC_ITER(pq, prec) {
			info.prec = prec;
			wlc_txq_pktq_filter(wlc, pq, wlc_txqueue_end_filter, &info);
		}
	}

	cbi->on_channel = FALSE;
}

bool
wlc_has_chanctxt(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	chanctxt_bss_info_t *cbi = CHANCTXT_BSS_INFO(wlc->chanctxt_info, cfg);

	ASSERT(cbi);

	return cbi->chanctxt != NULL;
}

chanspec_t
wlc_get_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	chanctxt_bss_info_t *cbi = CHANCTXT_BSS_INFO(wlc->chanctxt_info, cfg);
	wlc_chanctxt_t *chanctxt;

	ASSERT(cbi);
	chanctxt = cbi->chanctxt;

	return (chanctxt ? chanctxt->chanspec : INVCHANSPEC);
}

bool
_wlc_shared_chanctxt(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wlc_chanctxt_t *shared_chanctxt)
{
	chanctxt_bss_info_t *cbi = CHANCTXT_BSS_INFO(wlc->chanctxt_info, cfg);
	wlc_chanctxt_t *chanctxt;

	ASSERT(cbi);
	chanctxt = cbi->chanctxt;

	return ((chanctxt != NULL) && (chanctxt == shared_chanctxt));
}

bool
wlc_shared_chanctxt(wlc_info_t *wlc, wlc_bsscfg_t *cfg1, wlc_bsscfg_t *cfg2)
{
	chanctxt_bss_info_t *cbi1 = CHANCTXT_BSS_INFO(wlc->chanctxt_info, cfg1);
	chanctxt_bss_info_t *cbi2 = CHANCTXT_BSS_INFO(wlc->chanctxt_info, cfg2);

	ASSERT(cbi1 && cbi2);

	return cbi1->chanctxt != NULL && cbi2->chanctxt != NULL &&
	        cbi1->chanctxt == cbi2->chanctxt;
}

bool
wlc_shared_current_chanctxt(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	wlc_chanctxt_info_t *chanctxt_info = wlc->chanctxt_info;
	chanctxt_bss_info_t *cbi = CHANCTXT_BSS_INFO(chanctxt_info, cfg);
	wlc_chanctxt_t *chanctxt;

	ASSERT(cbi);
	chanctxt = cbi->chanctxt;

	return ((chanctxt != NULL) && (chanctxt == chanctxt_info->curr_chanctxt));
}

bool
_wlc_ovlp_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *cfg, chanspec_t chspec, uint chbw)
{
	chanctxt_bss_info_t *cbi = CHANCTXT_BSS_INFO(wlc->chanctxt_info, cfg);
	wlc_chanctxt_t *chanctxt;

	ASSERT(cbi);
	chanctxt = cbi->chanctxt;

	return chanctxt == NULL ||
	        CHSPEC_CTLOVLP(chanctxt->chanspec, chspec, (int)chbw);
}

bool
wlc_ovlp_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *cfg1, wlc_bsscfg_t *cfg2, uint chbw)
{
	chanctxt_bss_info_t *cbi1 = CHANCTXT_BSS_INFO(wlc->chanctxt_info, cfg1);
	chanctxt_bss_info_t *cbi2 = CHANCTXT_BSS_INFO(wlc->chanctxt_info, cfg2);
	wlc_chanctxt_t *chantxt1, *chantxt2;

	ASSERT(cbi1 && cbi2);
	chantxt1 = cbi1->chanctxt;
	chantxt2 = cbi2->chanctxt;

	return chantxt1 == NULL || chantxt2 == NULL || chantxt1 == chantxt2 ||
	        CHSPEC_CTLOVLP(chantxt1->chanspec, chantxt2->chanspec, (int)chbw);
}

static bool
wlc_chanctxt_in_use(wlc_info_t *wlc, wlc_chanctxt_t *chanctxt, wlc_bsscfg_t *filter_cfg)
{
	int idx;
	wlc_bsscfg_t *cfg;

	ASSERT(chanctxt != NULL);
	FOREACH_BSS(wlc, idx, cfg) {
		if ((cfg != filter_cfg) &&
			_wlc_shared_chanctxt(wlc, cfg, chanctxt)) {
			return (TRUE);
		}
	}
	return (FALSE);
}

static bool
wlc_tx_qi_exist(wlc_info_t *wlc, wlc_txq_info_t *qi)
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
wlc_get_qi_with_no_context(wlc_info_t *wlc)
{
	wlc_chanctxt_info_t *chanctxt_info = wlc->chanctxt_info;
	wlc_chanctxt_t *chanctxt_entity;
	wlc_txq_info_t *qi_list;
	bool found;

	/* walk through all queues */
	for (qi_list = wlc->tx_queues; qi_list; qi_list = qi_list->next) {
		found = FALSE;
		/* walk through all context */
		chanctxt_entity = chanctxt_info->chanctxt_list;
		for (; chanctxt_entity; chanctxt_entity = chanctxt_entity->next) {
			if (chanctxt_entity->qi == qi_list) {
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
			WL_MQ(("found qi %p with no associated ctxt\n", qi_list));
			return (qi_list);
		}
	}

	return (NULL);
}

/** Take care of all the stuff needed when a ctx channel is set */
static void
wlc_chanctxt_set_channel(wlc_chanctxt_info_t *chanctxt_info,
	wlc_chanctxt_t *chanctxt, chanspec_t chanspec, bool destroy_old)
{
	if (destroy_old) {
		/* delete phy context for calibration */
		wlc_phy_destroy_chanctx(WLC_PI(chanctxt_info->wlc), chanctxt->chanspec);
	}
	chanctxt->chanspec = chanspec;

	/* create phy context for calibration */
	wlc_phy_create_chanctx(WLC_PI(chanctxt_info->wlc), chanspec);
}

#ifdef SRHWVSDB
static void
wlc_chanctxt_srvsdb_upd(wlc_chanctxt_info_t *chanctxt_info)
{
	wlc_chanctxt_t *chanctxt = chanctxt_info->chanctxt_list;
	wlc_info_t *wlc = chanctxt_info->wlc;

	if (SRHWVSDB_ENAB(wlc->pub)) {
		/* reset the engine so that previously saved stuff will be gone */
		wlc_srvsdb_reset_engine(wlc);

		/* Attach SRVSDB module */
		if (wlc_multi_chanctxt(wlc)) {
			wlc_srvsdb_set_chanspec(wlc, chanctxt->chanspec, chanctxt->next->chanspec);
			if (wlc_bmac_activate_srvsdb(wlc->hw, chanctxt->chanspec,
				chanctxt->next->chanspec) != BCME_OK) {
				wlc_bmac_deactivate_srvsdb(wlc->hw);
			}
		} else {
			wlc_bmac_deactivate_srvsdb(wlc->hw);
		}
	}
}
#endif /* SRHWVSDB */

static wlc_chanctxt_t *
wlc_chanctxt_alloc(wlc_chanctxt_info_t *chanctxt_info, chanspec_t chanspec)
{
	wlc_info_t *wlc = chanctxt_info->wlc;
	osl_t *osh = wlc->osh;
	wlc_chanctxt_t *chanctxt;
	wlc_txq_info_t *qi;
	DBGONLY(char chanbuf[CHANSPEC_STR_LEN]; )

	WL_MQ(("wl%d: %s: channel %s\n", wlc->pub->unit, __FUNCTION__,
		wf_chspec_ntoa(chanspec, chanbuf)));

	chanctxt = (wlc_chanctxt_t *)MALLOCZ(osh, sizeof(wlc_chanctxt_t));
	if (chanctxt == NULL) {
		WL_ERROR(("wl%d: %s: failed to allocate mem for chan context\n",
			wlc->pub->unit, __FUNCTION__));
		return (NULL);
	}

	qi = wlc_get_qi_with_no_context(wlc);
	/* if qi with no context found, then use it */
	if (qi) {
		chanctxt->qi = qi;
		WL_MQ(("use existing qi = %p\n", qi));
	}
	/* else allocate a new queue */
	else {
		chanctxt->qi = wlc_txq_alloc(wlc, osh);
		WL_MQ(("allocate new qi = %p\n", chanctxt->qi));
	}

	ASSERT(chanctxt->qi != NULL);
	wlc_chanctxt_set_channel(chanctxt_info, chanctxt, chanspec, FALSE);

	/* add chanctxt to head of context list */
	chanctxt->next = chanctxt_info->chanctxt_list;
	chanctxt_info->chanctxt_list = chanctxt;

#ifdef SRHWVSDB
	wlc_chanctxt_srvsdb_upd(chanctxt_info);
#endif /* SRHWVSDB */

	return (chanctxt);
}

static void
wlc_chanctxt_free(wlc_chanctxt_info_t *chanctxt_info, wlc_chanctxt_t *chanctxt)
{
	wlc_info_t *wlc = chanctxt_info->wlc;
	osl_t *osh = wlc->osh;
	wlc_txq_info_t *oldqi;
	chanspec_t chanspec = chanctxt->chanspec;
	wlc_chanctxt_t *prev, *next;
	int idx,  prec;
	wlc_bsscfg_t *cfg;
	void * pkt;
	DBGONLY(char chanbuf[CHANSPEC_STR_LEN]; )

	WL_MQ(("wl%d: %s: channel %s\n", wlc->pub->unit, __FUNCTION__,
		wf_chspec_ntoa(chanspec, chanbuf)));

	ASSERT(chanctxt != NULL);

	/* save the qi pointer, delete from context list and free context */
	oldqi = chanctxt->qi;

	prev = (wlc_chanctxt_t *)&chanctxt_info->chanctxt_list;
	while ((next = prev->next)) {
		if (next == chanctxt) {
			prev->next = chanctxt->next;
			break;
		}
		prev = next;
	}
	MFREE(osh, chanctxt, sizeof(wlc_chanctxt_t));

	/* delete phy context for calibration */
	wlc_phy_destroy_chanctx(WLC_PI(wlc), chanspec);

#ifdef SRHWVSDB
	wlc_chanctxt_srvsdb_upd(chanctxt_info);
#endif /* SRHWVSDB */

	chanctxt = chanctxt_info->chanctxt_list;

	/* continue with deleting this queue only if there are more contexts */
	if (chanctxt == NULL) {
		/* flush the queue */
		WL_MQ(("wl%d: %s: our queue is only context queue, don't delete, "
			"simply flush the queue, qi 0x%p len %d\n", wlc->pub->unit,
			__FUNCTION__, oldqi, (WLC_GET_TXQ(oldqi))->len));
		wlc_txq_pktq_flush(wlc, WLC_GET_TXQ(oldqi));
		ASSERT(pktq_empty(WLC_GET_TXQ(oldqi)));
		return;
	}

	/* if this queue is the primary_queue, detach it first */
	if (oldqi == wlc->primary_queue) {
		ASSERT(oldqi != chanctxt->qi);
		WL_MQ(("wl%d: %s: detach primary queue %p\n",
			wlc->pub->unit, __FUNCTION__, wlc->primary_queue));
		wlc_suspend_mac_and_wait(wlc);
		wlc_primary_queue_set(wlc, NULL);
		wlc_enable_mac(wlc);
	}

	/* Before freeing this queue, any bsscfg that is using this queue
	 * should now use the primary queue.
	 * Active queue can be the excursion queue if we're scanning so using
	 * the primary queue is more appropriate.  When scan is done, active
	 * queue will be switched back to primary queue.
	 * This can be true for bsscfg's that never had contexts.
	 */
	FOREACH_BSS(wlc, idx, cfg) {
		if (cfg->wlcif->qi == oldqi) {
			WL_MQ(("wl%d.%d: %s: cfg's qi %p is about to get deleted, "
				"move to queue %p\n",
				wlc->pub->unit, WLC_BSSCFG_IDX(cfg),
				__FUNCTION__, oldqi, chanctxt->qi));
			cfg->wlcif->qi = chanctxt->qi;
		}
	}

	/* flush the queue */
	/* wlc_txq_pktq_flush(wlc, &oldqi->q); */
	while ((pkt = pktq_deq(WLC_GET_TXQ(oldqi), &prec))) {
#ifdef PROP_TXSTATUS
		if (PROP_TXSTATUS_ENAB(wlc->pub)) {
			wlc_process_wlhdr_txstatus(wlc,
				WLFC_CTL_PKTFLAG_DISCARD, pkt, FALSE);
		}
#endif /* PROP_TXSTATUS */
		PKTFREE(osh, pkt, TRUE);
	}

	ASSERT(pktq_empty(WLC_GET_TXQ(oldqi)));

	/* delete the queue */
	wlc_txq_free(wlc, osh, oldqi);
	WL_MQ(("wl%d %s: flush and free q\n", wlc->pub->unit, __FUNCTION__));
}

/**
 * This function returns channel context pointer based on chanspec passed in
 * If no context associated to chanspec, will return NULL.
 */
static wlc_chanctxt_t *
wlc_get_chanctxt(wlc_chanctxt_info_t *chanctxt_info, chanspec_t chanspec)
{
	wlc_chanctxt_t *chanctxt = chanctxt_info->chanctxt_list;

	while (chanctxt) {
		if (WLC_SCHED_SAME_CTLCHAN(chanctxt->chanspec, chanspec)) {
			return chanctxt;
		}
		chanctxt = chanctxt->next;
	}

	return NULL;
}

/**
 * Given a chanctxt and chanspec as input, looks at all bsscfg using this
 * chanctxt and see if bsscfg->chanspec matches exactly with chanspec.
 * Returns TRUE if all bsscfg->chanspec matches chanspec.  Else returns FALSE.
 */
static bool
wlc_all_cfg_chanspec_is_same(wlc_info_t *wlc,
	wlc_chanctxt_t *chanctxt, chanspec_t chanspec)
{
	int idx;
	wlc_bsscfg_t *cfg;

	FOREACH_AS_BSS(wlc, idx, cfg) {
		if (_wlc_shared_chanctxt(wlc, cfg, chanctxt) &&
		    !WLC_SCHED_SAME_CTLCHAN(cfg->current_bss->chanspec, chanspec)) {
			return (FALSE);
		}
	}
	return (TRUE);
}

/** before bsscfg register to MSCH, call this function */
static wlc_chanctxt_t *
wlc_chanctxt_create_txqueue(wlc_info_t *wlc, wlc_bsscfg_t *cfg, chanspec_t chanspec)
{
	wlc_chanctxt_info_t *chanctxt_info = wlc->chanctxt_info;
	chanctxt_bss_info_t *cbi = CHANCTXT_BSS_INFO(chanctxt_info, cfg);
	wlc_chanctxt_t *chanctxt, *new_chanctxt;
	bool flowcontrol;
	wlc_txq_info_t *oldqi = cfg->wlcif->qi;
	uint oldqi_stopped_flag = 0;
#ifdef ENABLE_FCBS
	wlc_phy_t *pi = WLC_PI(wlc);
#endif
	DBGONLY(char chanbuf[CHANSPEC_STR_LEN]; )

	ASSERT(cbi);
	chanctxt = cbi->chanctxt;

	WL_MQ(("wl%d.%d: %s: called on %s chanspec %s\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
		BSSCFG_AP(cfg) ? "AP" : "STA",
		wf_chspec_ntoa_ex(chanspec, chanbuf)));

	/* fetch context for this chanspec */
	new_chanctxt = wlc_get_chanctxt(chanctxt_info, chanspec);

	/* check old qi to see if we need to perform flow control on new qi.
	 * Only need to check TXQ_STOP_FOR_PKT_DRAIN because it is cfg specific.
	 * All other TXQ_STOP types are queue specific.
	 * Existing flow control setting only pertinent if we already have chanctxt.
	 */
	flowcontrol = ((chanctxt != NULL) &&
	               wlc_txflowcontrol_override_isset(wlc, cfg->wlcif->qi,
	                                                TXQ_STOP_FOR_PKT_DRAIN));

	/* This cfg has an existing context, do some checks to determine what to do */
	if (chanctxt) {
		WL_MQ(("wl%d.%d: %s: cfg alredy has context!\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__));

		wlc_lq_chanim_create_bss_chan_context(wlc, chanspec, chanctxt->chanspec);

		if (WLC_SCHED_SAME_CTLCHAN(chanctxt->chanspec, chanspec)) {
			/* we know control channel is same but if chanspecs are not
			 * identical, will want chan context to adopt to new chanspec,
			 * if no other cfg's using it or all cfg's using that context
			 * are using new chanspec.
			 */
			if ((chanctxt->chanspec != chanspec) &&
				(!wlc_chanctxt_in_use(wlc, chanctxt, cfg) ||
				wlc_all_cfg_chanspec_is_same(wlc, chanctxt,
					chanspec)) &&
				((chanspec == wlc_get_cur_wider_chanspec(wlc, cfg)) ||
			       /* In case either of the chanspec is ULB, we will re-use
			        * the same chan-ctxt. This is to ensure VSDB operations
			        * won`t start with ULB Mode Switch
			        */
			       (CHSPEC_IS_ULB(wlc, chanspec) ||
			        CHSPEC_IS_ULB(wlc, chanctxt->chanspec)))) {

				WL_MQ(("context is the same but need to adopt "
				          "from chanspec 0x%x to 0x%x\n",
				          chanctxt->chanspec, chanspec));
#ifdef ENABLE_FCBS
				wlc_phy_fcbs_uninit(pi, chanctxt->chanspec);
				wlc_phy_fcbs_arm(pi, chanspec, 0);
#endif
				wlc_chanctxt_set_channel(chanctxt_info, chanctxt, chanspec,
					!BSSCFG_IS_MODESW_BWSW(cfg));
			}
			else {
				WL_MQ(("context is the same as new one, do nothing\n"));
			}

			return chanctxt;
		}
		/* Requested nonoverlapping channel. */
		else {
			if (!wlc_chanctxt_in_use(wlc, chanctxt, cfg)) {
				wlc_chanctxt_delete_txqueue(wlc, cfg, &oldqi_stopped_flag);
			}
		}
	}

	/* check to see if a context for this chanspec already exist */
	if (new_chanctxt == NULL) {
		wlc_lq_chanim_create_bss_chan_context(wlc, chanspec, 0);

		/* context for this chanspec doesn't exist, create a new one */
		WL_MQ(("wl%d.%d: %s: allocate new context\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__));
		new_chanctxt = wlc_chanctxt_alloc(chanctxt_info, chanspec);
#ifdef ENABLE_FCBS
		if (!wlc_phy_fcbs_arm(pi, chanspec, 0))
			WL_MQ(("%s: wlc_phy_fcbs_arm FAILed\n", __FUNCTION__));
#endif
	}

	ASSERT(new_chanctxt != NULL);
	if (new_chanctxt == NULL)
		return NULL;

	/* assign context to cfg */
	cfg->wlcif->qi = new_chanctxt->qi;
	cbi->chanctxt = new_chanctxt;

	/* if we switched queue/context, need to take care of flow control */
	if (oldqi != new_chanctxt->qi) {
		/* evaluate to see if we need to enable flow control for PKT_DRAIN
		 * If flowcontrol is true, this means our old chanctxt has flow control
		 * of type TXQ_STOP_FOR_PKT_DRAIN enabled.  Transfer this setting to new qi.
		 */
		if (flowcontrol) {
			WL_MQ(("wl%d.%d: %s: turn ON flow control for "
			          "TXQ_STOP_FOR_PKT_DRAIN on new qi %p\n",
			          wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
			          new_chanctxt->qi));
			wlc_txflowcontrol_override(wlc, new_chanctxt->qi,
			                           ON, TXQ_STOP_FOR_PKT_DRAIN);
			/* Check our oldqi, if it is different and exists,
			 * then we need to disable flow control on it
			 * since we just moved queue.
			 */
			if (wlc_tx_qi_exist(wlc, oldqi)) {
				WL_MQ(("wl%d.%d: %s: turn OFF flow control for "
				          "TXQ_STOP_FOR_PKT_DRAIN on old qi %p\n",
				          wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
				          oldqi));
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
				WL_MQ(("wl%d.%d: %s: turn OFF flow control for "
				          "TXQ_STOP_FOR_PKT_DRAIN on new qi %p\n",
				          wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
				          oldqi));
				wlc_txflowcontrol_override(wlc, new_chanctxt->qi, OFF,
				                           TXQ_STOP_FOR_PKT_DRAIN);
			}
		}
	}

	if (CHSPEC_BW_GT(chanspec, CHSPEC_BW(new_chanctxt->chanspec))) {
		WL_MQ(("wl%d.%d: %s: Upgrade chanctxt chanspec "
			"from 0x%x to 0x%x\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
			new_chanctxt->chanspec, chanspec));
#ifdef ENABLE_FCBS
		wlc_phy_fcbs_uninit(pi, new_chanctxt->chanspec);
		wlc_phy_fcbs_arm(pi, chanspec, 0);
#endif
		wlc_chanctxt_set_channel(chanctxt_info, new_chanctxt, chanspec,
			!BSSCFG_IS_MODESW_BWSW(cfg));
	}

	WL_MQ(("wl%d.%d: %s: cfg chanctxt chanspec set to %s\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
		wf_chspec_ntoa_ex(new_chanctxt->chanspec, chanbuf)));

	return new_chanctxt;
}

/** Everytime bsscfg becomes disassociated, call this function */
static int
wlc_chanctxt_delete_txqueue(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint *oldqi_info)
{
	wlc_chanctxt_info_t *chanctxt_info = wlc->chanctxt_info;
	chanctxt_bss_info_t *cbi = CHANCTXT_BSS_INFO(chanctxt_info, cfg);
	wlc_chanctxt_t *chanctxt;
	bool saved_state = cfg->associated;
	DBGONLY(char chanbuf[CHANSPEC_STR_LEN]; )

	ASSERT(cbi);
	chanctxt = cbi->chanctxt;

	/* if context was not created due to AP up being deferred and we called down,
	 * can have a chan_ctxt that is NULL.
	 */
	if (chanctxt == NULL) {
		return (BCME_OK);
	}

	WL_MQ(("wl%d.%d: %s: called on %s chanspec %s\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
		BSSCFG_AP(cfg) ? "AP" : "STA",
		wf_chspec_ntoa_ex(chanctxt->chanspec, chanbuf)));

	if (cbi->on_channel) {
		chanctxt->count--;
		cbi->on_channel = FALSE;
	}

	wlc_lq_chanim_delete_bss_chan_context(wlc, chanctxt->chanspec);

	/* temporarily clear the cfg associated state.
	 * during roam, we are associated so when we do roam,
	 * we can potentially be deleting our old context first
	 * before creating our new context.  Just want to make sure
	 * that when we delete context, we're not associated.
	 */
	cfg->associated = FALSE;
	/* clear the cfg's context pointer */
	cbi->chanctxt = NULL;

#ifdef STA
	if (BSSCFG_STA(cfg)) {
#if defined(WLMCHAN) && defined(WLP2P)
		if (MCHAN_ENAB(wlc->pub) && P2P_CLIENT(wlc, cfg)) {
			if (wlc_p2p_noa_valid(wlc->p2p, cfg)) {
				mboolclr(cfg->pm->PMblocked, WLC_PM_BLOCK_MCHAN_ABS);
			}
		} else
#endif /* WLP2P */
		{
			mboolclr(cfg->pm->PMblocked, WLC_PM_BLOCK_CHANSW);
		}
		wlc_update_pmstate(cfg, TX_STATUS_NO_ACK);
	}
#endif /* STA */
#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
		/* Change the state of interface to OPEN so that pkts will be drained */
		wlc_wlfc_mchan_interface_state_update(wlc, cfg,
			WLFC_CTL_TYPE_INTERFACE_OPEN, TRUE);
		wlc_wlfc_flush_pkts_to_host(wlc, cfg);
		wlfc_sendup_ctl_info_now(wlc->wlfc);
	}
#endif /* PROP_TXSTATUS */
	wlc_txflowcontrol_override(wlc, chanctxt->qi, OFF,
		TXQ_STOP_FOR_MSCH_FLOW_CNTRL);

	/* if txq currently in use by cfg->wlcif->qi is deleted in
	 * wlc_mchanctxt_free(), cfg->wlcif->qi will be set to the primary queue
	 */

	/* Take care of flow control for this cfg.
	 * If it was turned on for the queue, then this cfg has flow control on.
	 * Turn it off for this cfg only.
	 * Allocate a stale queue and temporarily attach cfg to this queue.
	 * Copy over the qi->stopped flag and turn off flow control.
	 */
	if (chanctxt->qi->stopped) {
		if (oldqi_info != NULL) {
			*oldqi_info = chanctxt->qi->stopped;
		}
		else
		{
			wlc_txq_info_t *qi = wlc_txq_alloc(wlc, wlc->osh);
			wlc_txq_info_t *orig_qi = cfg->wlcif->qi;

			WL_MQ(("wl%d.%d: %s: flow control on, turn it off!\n", wlc->pub->unit,
			          WLC_BSSCFG_IDX(cfg), __FUNCTION__));
			/* Use this new q to drain the wl layer packets to make sure flow control
			 * is turned off for this interface.
			 */
			qi->stopped = chanctxt->qi->stopped;
			cfg->wlcif->qi = qi;
			/* reset_qi() below will not allow tx flow control to turn back on */
			wlc_txflowcontrol_reset_qi(wlc, qi);
			/* if txq currently in use by cfg->wlcif->qi is deleted in
			 * wlc_infra_sched_free(), cfg->wlcif->qi will be set to the primary queue
			 */
			wlc_txq_pktq_flush(wlc, WLC_GET_TXQ(qi));
			ASSERT(pktq_empty(WLC_GET_TXQ(qi)));
			wlc_txq_free(wlc, wlc->osh, qi);
			cfg->wlcif->qi = orig_qi;
		}
	}

	/* no one else using this context, safe to delete it */
	if (!wlc_chanctxt_in_use(wlc, chanctxt, cfg)) {
#ifdef ENABLE_FCBS
		if (!wlc_phy_fcbs_uninit(WLC_PI(wlc), chanctxt->chanspec))
			WL_INFORM(("%s: wlc_phy_fcbs_uninit() FAILed\n", __FUNCTION__));
#endif
		if (chanctxt == chanctxt_info->curr_chanctxt) {
#ifdef STA
			/* If we were in the middle of transitioning to PM state because of this
			 * BSS, then cancel it
			 */
			if (wlc->PMpending) {
				wlc_bsscfg_t *icfg;
				int idx;
				FOREACH_AS_STA(wlc, idx, icfg) {
					if (icfg->pm->PMpending &&
						_wlc_shared_chanctxt(wlc, icfg, chanctxt)) {
						wlc_update_pmstate(icfg, TX_STATUS_BE);
					}
				}
			}
#endif /* STA */
			chanctxt_info->curr_chanctxt = NULL;
		}

		wlc_chanctxt_free(chanctxt_info, chanctxt);
		chanctxt = NULL;
	}

	/* restore cfg associated state */
	cfg->associated = saved_state;

	return BCME_OK;
}
