/*
 * Event mechanism
 *
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
 * $Id: wlc_event.c 644653 2016-06-21 07:23:43Z $
 */

/**
 * @file
 * @brief
 * The WLAN driver currently has tight coupling between different components. In particular,
 * components know about each other, and call each others functions, access data, and invoke
 * callbacks. This means that maintenance and new features require changing these
 * relationships. This is fundamentally a tightly coupled system where everything touches
 * many other things.
 *
 * @brief
 * We can reduce the coupling between our features by reducing their need to directly call
 * each others functions, and access each others data. An mechanism for accomplishing this is
 * a generic event signaling mechanism. The event infrastructure enables modules to communicate
 * indirectly through events, rather than directly by calling each others routines and
 * callbacks.
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <wl_dbg.h>
#include <wlc_pub.h>
#include <wl_export.h>
#include <wlc_event.h>
#include <bcm_mpool_pub.h>
#include <d11.h>
#include <wlc_bsscfg.h>
#include <wlc_rate.h>
#include <wlc.h>
#include <wlc_rx.h>

/* Local prototypes */
static void wlc_eventq_timer_cb(void *arg);
static void wlc_eventq_process(void *arg);
static int wlc_eventq_down(void *ctx);
static int wlc_eventq_doiovar(void *hdl, uint32 actionid,
        void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif);

static void wlc_eventq_bss_deinit(void *ctx, wlc_bsscfg_t *cfg);
static void wlc_eventq_bss_updown(void *ctx, bsscfg_up_down_event_data_t *evt);

static void wlc_eventq_enq(wlc_eventq_t *eq, wlc_event_t *e);
static wlc_event_t *wlc_eventq_deq(wlc_eventq_t *eq);
static bool wlc_eventq_avail(wlc_eventq_t *eq);

#ifndef WLNOEIND
static int wlc_event_sendup(wlc_eventq_t *eq, const wlc_event_t *e,
	struct ether_addr *da, struct ether_addr *sa, uint8 *data, uint32 len);
static int wlc_eventq_query_ind_ext(wlc_eventq_t *eq, eventmsgs_ext_t* in_iovar_msg,
	eventmsgs_ext_t* out_iovar_msg, uint8 *mask);
static int wlc_eventq_handle_ind(wlc_eventq_t *eq, wlc_event_t *e);
static int wlc_eventq_register_ind_ext(wlc_eventq_t *eq, eventmsgs_ext_t* iovar_msg, uint8 *mask);
#else
#define wlc_eventq_query_ind_ext(a, b, c, d) 0
#define wlc_eventq_handle_ind(a, b) do {} while (0)
#define wlc_eventq_register_ind_ext(a, b, c) 0
#endif /* WLNOEIND */

#if defined(BCMPKTPOOL)
static uint8 *wlc_event_get_evpool_events(void);
static int wlc_eventq_test_evpool_mask(wlc_eventq_t *eq, int et);
#endif


enum {
	IOV_EVENT_MSGS = 1,
	IOV_EVENT_MSGS_EXT = 2
};

static const bcm_iovar_t eventq_iovars[] = {
	{"event_msgs", IOV_EVENT_MSGS,
	(IOVF_OPEN_ALLOW|IOVF_RSDB_SET), 0, IOVT_BUFFER, WL_EVENTING_MASK_LEN
	},
	{"event_msgs_ext", IOV_EVENT_MSGS_EXT,
	(IOVF_OPEN_ALLOW|IOVF_RSDB_SET), 0, IOVT_BUFFER, EVENTMSGS_EXT_STRUCT_SIZE
	},
	{NULL, 0, 0, 0, 0, 0}
};

typedef void (*wlc_eventq_cb_t)(void *arg);

#if defined(BCMPKTPOOL)
/* Event bitmap which are considered critical and should not fail due to malloc failure */
static uint8 evpool_events[WL_EVENTING_MASK_EXT_LEN] = {
	0xc9, 0x00, 0x03, 0x04, /* 0-31: 0,3,6,7,16,17,26 */
	0x02, 0x42, 0x00, 0x01, /* 32-63: 33,41,46,56 */
	0x20, 0x04, 0x0A, 0x00, /* 64-95: 69, 74,81,83 */
	0x07, 0x00, 0x00, 0x00  /* 96-127: 96,97,98 */
	/* remaining bytes are currently zeros */
};
#define SUPERCRITICAL(evid) (((evid) == WLC_E_ESCAN_RESULT) || ((evid) == WLC_E_SET_SSID))
#define SUPEREV  0x01
#define SUPEREVD 0x02
#endif /* BCMPKTPOOL */

/* Private data structures */
struct wlc_eventq
{
	wlc_event_t		*head;
	wlc_event_t		*tail;
	wlc_info_t		*wlc;
	void			*wl;
	bool			tpending;
	bool			workpending;
	struct wl_timer		*timer;
	wlc_eventq_cb_t		cb;
	void			*cb_ctx;
	bcm_mp_pool_h		mpool_h;
	uint8			event_inds_mask_len;
	uint8			*event_inds_mask;
	uint8			edata_no_alloc;
	uint8			*event_fail_cnt;
#if defined(BCMPKTPOOL)
	uint16			evpool_pktsize;
	uint8			evpool_mask_len;
	uint8			*evpool_mask;
	pktpool_t		evpool;
	wlc_event_t		**evpre;
	void			**evpred;
	uint16			evpre_msk;
	uint16			evpred_msk;
	/* Last-ditch event reserved for SCAN_COMPLETE and SET_SSID only (super-critical) */
	pktpool_t		superpool;
	wlc_event_t		*superev;
	void			*superevd;
	uint8			supermsk; /* Bit 0/1 superev/superevd available */
#endif
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

static void
wlc_event_eventfail_upd(wlc_eventq_t *eq, uint32 event_id)
{
	uint8 *event_fail_cnt = eq->event_fail_cnt;
	ASSERT(event_fail_cnt);
	ASSERT(event_id < WLC_E_LAST);

	event_fail_cnt[event_id]++;
}

static void
wlc_event_edatanoalloc_upd(wlc_eventq_t *eq)
{
	eq->edata_no_alloc++;
}

#if defined(BCMPKTPOOL)
static uint8 *
BCMRAMFN(wlc_event_get_evpool_events)(void)
{
	return evpool_events;
}
#endif /* BCMPKTPOOL */

/*
 * Export functions
 */
wlc_eventq_t*
BCMATTACHFN(wlc_eventq_attach)(wlc_info_t *wlc)
{
	wlc_eventq_t *eq;
	uint eventqsize;
#if defined(BCMPKTPOOL)
	int n = wlc->pub->tunables->evpool_size;
	ASSERT(n <= (sizeof(eq->evpre_msk) * NBBY));
#endif

	/*
	 * Size of the eventq structure with the space for event mask field (event_inds_mask)
	 * and space for event fail counters (event_fail_cnt)
	 */
	eventqsize = sizeof(wlc_eventq_t) + WL_EVENTING_MASK_EXT_LEN +
		(WLC_E_LAST * sizeof(*eq->event_fail_cnt));

	eq = (wlc_eventq_t*)MALLOCZ(wlc->osh, eventqsize);
	if (eq == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto exit;
	}

	eq->event_inds_mask_len = WL_EVENTING_MASK_EXT_LEN;

	eq->event_inds_mask = (uint8*)((uintptr)eq + sizeof(wlc_eventq_t));

	eq->event_fail_cnt = (uint8*)(eq->event_inds_mask + eq->event_inds_mask_len);

	/* make a reference to get rid of non-debug build error */
	(void)wlc_eventq_avail(eq);

	/* Create memory pool for 'wlc_event_t' data structs. */
	if (bcm_mpm_create_heap_pool(wlc->mem_pool_mgr, sizeof(wlc_event_t),
	                             "event", &eq->mpool_h) != BCME_OK) {
		WL_ERROR(("wl%d: %s: bcm_mpm_create_heap_pool failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto exit;
	}
	eq->cb = wlc_eventq_process;
	eq->cb_ctx = eq;
	eq->wlc = wlc;
	eq->wl = wlc->wl;

	/* utilize the bsscfg cubby machanism to register deinit callback */
	if (wlc_bsscfg_cubby_reserve(wlc, 0, NULL, wlc_eventq_bss_deinit, NULL, eq) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto exit;
	}

	if (wlc_bsscfg_updown_register(wlc, wlc_eventq_bss_updown, eq) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_updown_register failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto exit;
	}

	/* register event module */
	if (wlc_module_register(wlc->pub, eventq_iovars, "eventq", eq, wlc_eventq_doiovar,
	                        NULL, NULL, wlc_eventq_down)) {
		WL_ERROR(("wl%d: %s: event wlc_module_register() failed",
			wlc->pub->unit, __FUNCTION__));
		goto exit;
	}
	if (!(eq->timer = wl_init_timer(eq->wl, wlc_eventq_timer_cb, eq, "eventq"))) {
		WL_ERROR(("wl%d: %s: timer failed\n", wlc->pub->unit, __FUNCTION__));
		goto exit;
	}

#if defined(BCMPKTPOOL)
	if (wlc->pub->tunables->evpool_size) {
	/* Make a separate preallocated pool for both event structures and data, also packets.
	 * (May be able to convert this into some sort of mem_pool later.)
	 */
	eq->evpool_pktsize = (sizeof(bcm_event_t) + wlc->pub->tunables->evpool_maxdata + 2);

	eq->evpool_mask_len = WL_EVENTING_MASK_EXT_LEN;
	eq->evpool_mask = wlc_event_get_evpool_events();

	eq->evpre = (wlc_event_t **)MALLOC(wlc->osh,
		(wlc->pub->tunables->evpool_size * sizeof(*eq->evpre)));
	eq->evpred = (void **)MALLOC(wlc->osh,
		(wlc->pub->tunables->evpool_size * sizeof(*eq->evpred)));

	if ((eq->evpre == NULL) || (eq->evpred == NULL)) {
		WL_ERROR(("Could not prealloc event struct/data pointers\n"));
		goto exit;
	}

	if (pktpool_init(wlc->osh, &eq->evpool, &n, eq->evpool_pktsize, FALSE, lbuf_basic) ||
	    (n < wlc->pub->tunables->evpool_size)) {
		WL_ERROR(("Could not allocate event pool (want %d, got %d)\n",
			wlc->pub->tunables->evpool_size, n));
		goto exit;
	}

	for (n = 0; n < wlc->pub->tunables->evpool_size; n++) {
		eq->evpre[n] = (wlc_event_t *)MALLOCZ(wlc->osh, sizeof(wlc_event_t));
		if (eq->evpre[n])
			eq->evpre_msk |= (1 << n);
		else
			break;

		eq->evpred[n] = (void *)MALLOCZ(wlc->osh,
			wlc->pub->tunables->evpool_maxdata);
		if (eq->evpred[n])
			eq->evpred_msk |= (1 << n);
		else
			break;
	}
	if (n < wlc->pub->tunables->evpool_size) {
		WL_ERROR(("Could not prealloc event struct/data\n"));
		goto exit;
	}

	/* Now the super-critical event reservation */
	n = 1;
	if (pktpool_init(wlc->osh, &eq->superpool,
		&n, eq->evpool_pktsize, FALSE, lbuf_basic) || (n < 1)) {
		WL_ERROR(("Event superpool failed\n"));
		goto exit;
	}

	eq->superev = (wlc_event_t *)MALLOCZ(wlc->osh, sizeof(wlc_event_t));
	eq->superevd = (void *)MALLOCZ(wlc->osh, wlc->pub->tunables->evpool_maxdata);
	if ((eq->superev == NULL) || (eq->superevd == NULL)) {
		WL_ERROR(("Event superev/d failed\n"));
		goto exit;
	}

	eq->supermsk = (SUPEREV | SUPEREVD);
	}
#endif /* BCMPKTPOOL */

	return eq;

exit:
	MODULE_DETACH(eq, wlc_eventq_detach);
	return NULL;
}


void
BCMATTACHFN(wlc_eventq_detach)(wlc_eventq_t *eq)
{
	uint eventqsize;
	wlc_info_t *wlc;

	if (eq == NULL)
		return;

	wlc = eq->wlc;

	if (eq->timer) {
		if (eq->tpending) {
			wl_del_timer(eq->wl, eq->timer);
			eq->tpending = FALSE;
		}
		wl_free_timer(eq->wl, eq->timer);
		eq->timer = NULL;
	}

#if defined(BCMPKTPOOL)
	ASSERT(pktpool_tot_pkts(&eq->evpool) == pktpool_avail(&eq->evpool));
	{
		int n;

		if (eq->superevd) {
			MFREE(eq->wlc->osh, eq->superevd, wlc->pub->tunables->evpool_maxdata);
		}
		if (eq->superev) {
			MFREE(eq->wlc->osh, eq->superev, sizeof(wlc_event_t));
		}

		pktpool_deinit(eq->wlc->osh, &eq->superpool);

		for (n = 0; n < wlc->pub->tunables->evpool_size; n++) {
			if (eq->evpred && eq->evpred[n]) {
				MFREE(eq->wlc->osh, eq->evpred[n],
					wlc->pub->tunables->evpool_maxdata);
			}
			if (eq->evpre && eq->evpre[n]) {
				MFREE(eq->wlc->osh, eq->evpre[n], sizeof(wlc_event_t));
			}
		}
		pktpool_deinit(eq->wlc->osh, &eq->evpool);

		if (eq->evpred) {
			MFREE(eq->wlc->osh, eq->evpred,
				(wlc->pub->tunables->evpool_size * sizeof(*eq->evpred)));
		}
		if (eq->evpre) {
			MFREE(eq->wlc->osh, eq->evpre,
				(wlc->pub->tunables->evpool_size * sizeof(*eq->evpre)));
		}
	}
#endif /*  BCMPKTPOOL */

	wlc_module_unregister(wlc->pub, "eventq", eq);

	(void)wlc_bsscfg_updown_unregister(wlc, wlc_eventq_bss_updown, eq);

	ASSERT(wlc_eventq_avail(eq) == FALSE);

	bcm_mpm_delete_heap_pool(wlc->mem_pool_mgr, &eq->mpool_h);

	eventqsize = sizeof(wlc_eventq_t) + WL_EVENTING_MASK_EXT_LEN +
		(WLC_E_LAST * sizeof(*eq->event_fail_cnt));

	MFREE(wlc->osh, eq, eventqsize);
}

static int
wlc_eventq_doiovar(void *hdl, uint32 actionid,
        void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif)
{
	wlc_eventq_t *eq = (wlc_eventq_t *)hdl;
	int err = BCME_OK;

	BCM_REFERENCE(wlcif);
	BCM_REFERENCE(val_size);
	BCM_REFERENCE(p_len);
	BCM_REFERENCE(eq);

	switch (actionid) {
	case IOV_GVAL(IOV_EVENT_MSGS): {
		eventmsgs_ext_t in_iovar_msg, out_iovar_msg;
		bzero(arg, WL_EVENTING_MASK_LEN);
		in_iovar_msg.len = WL_EVENTING_MASK_LEN;
		out_iovar_msg.len = 0;
		err = wlc_eventq_query_ind_ext(eq, &in_iovar_msg, &out_iovar_msg, arg);
		break;
	}

	case IOV_SVAL(IOV_EVENT_MSGS): {
		eventmsgs_ext_t iovar_msg;
		iovar_msg.len = WL_EVENTING_MASK_LEN;
		iovar_msg.command = EVENTMSGS_SET_MASK;
		err = wlc_eventq_register_ind_ext(eq, &iovar_msg, arg);
		break;
	}

	case IOV_GVAL(IOV_EVENT_MSGS_EXT): {
		if (((eventmsgs_ext_t*)params)->ver != EVENTMSGS_VER) {
			err = BCME_VERSION;
			break;
		}
		if (len < (int)((EVENTMSGS_EXT_STRUCT_SIZE +
		                 ((eventmsgs_ext_t*)params)->len))) {
			err = BCME_BUFTOOSHORT;
			break;
		}
		err = wlc_eventq_query_ind_ext(eq, (eventmsgs_ext_t*)params,
				(eventmsgs_ext_t*)arg, ((eventmsgs_ext_t*)arg)->mask);
		break;
	}

	case IOV_SVAL(IOV_EVENT_MSGS_EXT):
		if (((eventmsgs_ext_t*)arg)->ver != EVENTMSGS_VER) {
			err = BCME_VERSION;
			break;
		}
		if (len < (int)((EVENTMSGS_EXT_STRUCT_SIZE +
		                 ((eventmsgs_ext_t*)arg)->len))) {
			err = BCME_BUFTOOSHORT;
			break;
		}
		err = wlc_eventq_register_ind_ext(eq, (eventmsgs_ext_t*)arg,
				((eventmsgs_ext_t*)arg)->mask);
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}
	return err;
}

static void
_wlc_eventq_process(wlc_eventq_t *eq)
{
	ASSERT(wlc_eventq_avail(eq) == TRUE);
	ASSERT(eq->workpending == FALSE);
	eq->workpending = TRUE;

	if (eq->cb)
		eq->cb(eq->cb_ctx);

	ASSERT(wlc_eventq_avail(eq) == FALSE);
	ASSERT(eq->workpending == TRUE);
	eq->workpending = FALSE;

	eq->tpending = FALSE;
}

static int
BCMUNINITFN(wlc_eventq_down)(void *ctx)
{
	wlc_eventq_t *eq = (wlc_eventq_t *)ctx;
	int callbacks = 0;

	if (eq->tpending && !eq->workpending) {
		if (!wl_del_timer(eq->wl, eq->timer))
			callbacks++;

		_wlc_eventq_process(eq);
	}
	else {
		ASSERT(eq->workpending || wlc_eventq_avail(eq) == FALSE);
	}
	return callbacks;
}

/*
 * The eventid argument specifies the event for which the malloc is requested.
 * This is useful for future development if any backup pools needs to be used for the event when
 * the malloc fails for critical events. Also if any handling specific to a set of events is
 * required, then it can be done without abandoning the calling functions in full dongle.
 */
wlc_event_t*
wlc_event_alloc(wlc_eventq_t *eq, uint eventid)
{
	wlc_event_t *e;

	e = (wlc_event_t *) bcm_mp_alloc(eq->mpool_h);
#if defined(BCMPKTPOOL)
	/* If the heap-pool alloc failed, selected events may use backup event pool(s) */
	if (e == NULL) {
		/* Check for supercritical events first */
		if (SUPERCRITICAL(eventid) && (eq->supermsk & SUPEREV)) {
			eq->supermsk &= ~SUPEREV;
			e = eq->superev;
			goto exit;
		}

		if (eq->evpre_msk && wlc_eventq_test_evpool_mask(eq, eventid)) {
			uint num, msk;

			for (num = 0, msk = eq->evpre_msk; msk; num++, msk >>= 1) {
				if (msk & 1)
					break;
			}

			if (msk) {
				eq->evpre_msk &= ~(1 << num);
				e = eq->evpre[num];
			}
		}
	}
#endif /* BCMPKTPOOL */
	if (e == NULL)
		return NULL;
#if defined(BCMPKTPOOL)
exit:
#endif
	memset(e, 0, sizeof(*e));
	e->event.event_type = eventid;
	return e;
}

/*
 * The eventid argument specifies the event for which the malloc is requested.
 * This is useful for future development if any backup pools needs to be used for the event when
 * the malloc fails for critical events. Also if any handling specific to a set of events is
 * required, then it can be done without abandoning the calling functions in full dongle.
 */
void*
wlc_event_data_alloc(wlc_eventq_t *eq, uint32 datalen, uint32 event_id)
{
	void *dblock = NULL;

	BCM_REFERENCE(event_id);

	dblock = MALLOCZ(eq->wlc->osh, datalen);

#if defined(BCMPKTPOOL)
	/* If the heap alloc failed, selected events may backup data pool(s) */
	if ((dblock == NULL) && (datalen <= eq->wlc->pub->tunables->evpool_maxdata)) {
		/* Check for supercritical first */
		if (SUPERCRITICAL(event_id) && (eq->supermsk & SUPEREVD)) {
			eq->supermsk &= ~SUPEREVD;
			return eq->superevd;
		}

		if (eq->evpred_msk && wlc_eventq_test_evpool_mask(eq, event_id)) {
			uint num, msk;

			for (num = 0, msk = eq->evpred_msk; msk; num++, msk >>= 1) {
				if (msk & 1)
					break;
			}

			if (msk) {
				eq->evpred_msk &= ~(1 << num);
				dblock = eq->evpred[num];
				memset(dblock, 0, datalen);
			}
		}
	}
#endif /* BCMPKTPOOL */

	if (dblock == NULL) {
		wlc_event_edatanoalloc_upd(eq);
	}

	return dblock;
}

int
wlc_event_data_free(wlc_eventq_t *eq, void *data, uint32 datalen)
{
#if defined(BCMPKTPOOL)
	/* Check backup data pool first */
	ASSERT(eq);
	if (data && (datalen <= eq->wlc->pub->tunables->evpool_maxdata)) {
		uint num;

		if (data == eq->superevd) {
			eq->supermsk |= SUPEREVD;
			return 0;
		}

		for (num = 0; num < eq->wlc->pub->tunables->evpool_size; num++) {
			if (data == eq->evpred[num]) {
				eq->evpred_msk |= (1 << num);
				return 0;
			}
		}
	}
#endif /* BCMPKTPOOL */
	MFREE(eq->wlc->osh, data, datalen);
	return 0;
}

/*
 * The eventid argument specifies the event for which the pktget is requested.
 * In future, if any handling specific to a set of events is required, then it can be done without
 * abandoning the calling functions in full dongle.
 */
static void*
wlc_event_pktget(wlc_eventq_t *eq, uint pktlen, uint32 event_id)
{
	void *p = NULL;
	ASSERT(event_id < WLC_E_LAST);

	p = PKTGET(eq->wlc->osh, pktlen, FALSE);

#if defined(BCMPKTPOOL)
	/* If not allocated, try the backup event packet pool if applicable */
	if ((p == NULL) && (pktlen <= eq->evpool_pktsize)) {
		if (wlc_eventq_test_evpool_mask(eq, event_id) || SUPERCRITICAL(event_id)) {
			if (SUPERCRITICAL(event_id))
				p = pktpool_get(&eq->superpool);
			if ((p == NULL) && wlc_eventq_test_evpool_mask(eq, event_id))
				p = pktpool_get(&eq->evpool);
		}
	}
#endif /* BCMPKTPOOL */
	if (p == NULL) {
		wlc_event_eventfail_upd(eq, event_id);
	}

	return p;
}

void
wlc_event_free(wlc_eventq_t *eq, wlc_event_t *e)
{
	if (e->data != NULL) {
		wlc_event_data_free(eq, e->data, e->event.datalen);
	}

	ASSERT(e->next == NULL);

#if defined(BCMPKTPOOL)
	/* Check the preallocated event backup pool first */
	{
		uint num;
		ASSERT(eq);

		if (e == eq->superev) {
			eq->supermsk |= SUPEREV;
			return;
		}

		for (num = 0; num < eq->wlc->pub->tunables->evpool_size; num++) {
			if (e == eq->evpre[num]) {
				eq->evpre_msk |= (1 << num);
				return;
			}
		}
	}
#endif /* BCMPKTPOOL */

	bcm_mp_free(eq->mpool_h, e);
}

static void
wlc_eventq_enq(wlc_eventq_t *eq, wlc_event_t *e)
{
	ASSERT(e->next == NULL);
	e->next = NULL;

	if (eq->tail) {
		eq->tail->next = e;
		eq->tail = e;
	}
	else
		eq->head = eq->tail = e;

	if (!eq->tpending) {
		eq->tpending = TRUE;
		/* Use a zero-delay timer to trigger
		 * delayed processing of the event.
		 */
		wl_add_timer(eq->wl, eq->timer, 0, 0);
	}
}

static wlc_event_t*
wlc_eventq_deq(wlc_eventq_t *eq)
{
	wlc_event_t *e;

	e = eq->head;
	if (e) {
		eq->head = e->next;
		e->next = NULL;

		if (eq->head == NULL)
			eq->tail = eq->head;
	}
	else if (eq->tpending) {
		/* Timer might have been started within event/timeout handlers,
		 * but, since all the events are processed and event queue
		 * is empty delete the pending timer
		 */
		wl_del_timer(eq->wl, eq->timer);
		eq->tpending = FALSE;
	}

	return e;
}

static bool
wlc_eventq_avail(wlc_eventq_t *eq)
{
	return (eq->head != NULL);
}


/* Immediate event processing.
 * Process the event and any events generated during the processing,
 * and queue all these events for deferred processing as well.
 */
void
wlc_event_process(wlc_eventq_t *eq, wlc_event_t *e)
{

	/* deliver event to the port OS right now */
	wl_event_sync(eq->wl, e->event.ifname, e);

	/* queue the event for 0 delay timer delivery */
	wlc_eventq_enq(eq, e);
}

/* Deferred event processing */
static void
wlc_eventq_process(void *arg)
{
	wlc_eventq_t *eq = (wlc_eventq_t *)arg;
	wlc_event_t *etmp;

	while ((etmp = wlc_eventq_deq(eq))) {
		/* Perform OS specific event processing */
		wl_event(eq->wl, etmp->event.ifname, etmp);
		/* Perform common event notification */
		wlc_eventq_handle_ind(eq, etmp);
		wlc_event_free(eq, etmp);
	}
}

#ifndef WLNOEIND
static int
wlc_eventq_register_ind_ext(wlc_eventq_t *eq, eventmsgs_ext_t* iovar_msg, uint8 *mask)
{
	int i;
	int current_event_mask_size;


	/*  re-using the event_msgs_ext iovar struct for convenience, */
	/*	but only using some fields -- */
	/*	if changed remember to check callers */

	current_event_mask_size = MIN(eq->event_inds_mask_len, iovar_msg->len);

	switch (iovar_msg->command) {
		case EVENTMSGS_SET_BIT:
			for (i = 0; i < current_event_mask_size; i++)
				eq->event_inds_mask[i] |= mask[i];
			break;
		case EVENTMSGS_RESET_BIT:
			for (i = 0; i < current_event_mask_size; i++)
				eq->event_inds_mask[i] &= mask[i];
			break;
		case EVENTMSGS_SET_MASK:
			bcopy(mask, eq->event_inds_mask, current_event_mask_size);
			break;
		default:
			return BCME_BADARG;
	};

	wlc_enable_probe_req(
		eq->wlc,
		PROBE_REQ_EVT_MASK,
		wlc_eventq_test_ind(eq, WLC_E_PROBREQ_MSG)? PROBE_REQ_EVT_MASK:0);

	return 0;
}

static int
wlc_eventq_query_ind_ext(wlc_eventq_t *eq, eventmsgs_ext_t* in_iovar_msg,
	eventmsgs_ext_t* out_iovar_msg, uint8 *mask)
{
	out_iovar_msg->len = MIN(eq->event_inds_mask_len, in_iovar_msg->len);
	out_iovar_msg->maxgetsize = eq->event_inds_mask_len;
	bcopy(eq->event_inds_mask, mask, out_iovar_msg->len);
	return 0;
}

int
wlc_eventq_test_ind(wlc_eventq_t *eq, int et)
{
	return isset(eq->event_inds_mask, et);
}

static int
wlc_eventq_handle_ind(wlc_eventq_t *eq, wlc_event_t *e)
{
	wlc_bsscfg_t *cfg;
	struct ether_addr *da;
	struct ether_addr *sa;

	cfg = wlc_bsscfg_find_by_wlcif(eq->wlc, e->wlcif);
	ASSERT(cfg != NULL);

	da = &cfg->cur_etheraddr;
	sa = &cfg->cur_etheraddr;

	if (wlc_eventq_test_ind(eq, e->event.event_type))
		wlc_event_sendup(eq, e, da, sa, e->data, e->event.datalen);
	return 0;
}

void
wlc_eventq_flush(wlc_eventq_t *eq)
{
	if (eq->cb)
		eq->cb(eq->cb_ctx);
	if (eq->tpending) {
		wl_del_timer(eq->wl, eq->timer);
		eq->tpending = FALSE;
	}
}
#endif /* !WLNOEIND */

#if defined(BCMPKTPOOL)
static int
wlc_eventq_test_evpool_mask(wlc_eventq_t *eq, int et)
{
	return isset(eq->evpool_mask, et);
}

int
wlc_eventq_set_evpool_mask(wlc_eventq_t* eq, uint et, bool enab)
{
	if (et >= WLC_E_LAST)
		return -1;
	if (enab)
		setbit(eq->evpool_mask, et);
	else
		clrbit(eq->evpool_mask, et);

	return 0;
}
#endif /* BCMPKTPOOL */

static void
wlc_eventq_bss_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
	struct wlc_eventq *eq = (struct wlc_eventq *)ctx;
	BCM_REFERENCE(cfg);
	BCM_REFERENCE(eq);

	wlc_eventq_flush(eq);
}

static void
wlc_eventq_bss_updown(void *ctx, bsscfg_up_down_event_data_t *evt)
{
	if (!evt->up) {
		struct wlc_eventq *eq = (struct wlc_eventq *)ctx;
		BCM_REFERENCE(eq);

		wlc_eventq_flush(eq);
	}
}

/*
 * Local Functions
 */
static void
wlc_eventq_timer_cb(void *arg)
{
	struct wlc_eventq* eq = (struct wlc_eventq*)arg;

	ASSERT(eq->tpending == TRUE);

	_wlc_eventq_process(eq);

	ASSERT(eq->tpending == FALSE);
}


#ifndef WLNOEIND
/* Abandonable helper function for PROP_TXSTATUS */
static void
wlc_event_mark_packet(wlc_info_t *wlc, void *p)
{
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(p);
}

static void
wlc_assign_event_msg(wlc_info_t *wlc, wl_event_msg_t *msg, const wlc_event_t *e,
	uint8 *data, uint32 len)
{
	void *databuf;
	BCM_REFERENCE(wlc);
	ASSERT(msg && e);

	/* translate the wlc event into bcm event msg */
	msg->version = hton16(BCM_EVENT_MSG_VERSION);
	msg->event_type = hton32(e->event.event_type);
	msg->status = hton32(e->event.status);
	msg->reason = hton32(e->event.reason);
	msg->auth_type = hton32(e->event.auth_type);
	msg->datalen = hton32(len);
	msg->flags = hton16(e->event.flags);
	bzero(msg->ifname, sizeof(msg->ifname));
	strncpy(msg->ifname, e->event.ifname, sizeof(msg->ifname) - 1);
	msg->ifidx = e->event.ifidx;
	msg->bsscfgidx = e->event.bsscfgidx;

	if (e->addr)
		bcopy(e->event.addr.octet, msg->addr.octet, ETHER_ADDR_LEN);

	databuf = (char *)(msg + 1);
	if (len)
		bcopy(data, databuf, len);
}

/*
 * This is placeholder function which can be used to strip off the optional bssinfo at the
 * end of the eventdata in case of low memory situations. If there is a backup pool implemented
 * for critical events, the buffer size of these can be small but still accommodate the eventdata
 * with the help of this stripping.
 */
static uint16
wlc_event_strip_bssinfo(wlc_eventq_t *eq, const wlc_event_t *e, uint8 *data, uint32 len)
{
	/* No stripping allowed */
	return 0;
}

/* Placeholder to allow requeuing */
static int
wlc_event_tracking(wlc_eventq_t *eq, const wlc_event_t *e, bool success)
{
	return BCME_OK;
}

static int
wlc_event_sendup(wlc_eventq_t *eq, const wlc_event_t *e,
	struct ether_addr *da, struct ether_addr *sa, uint8 *data, uint32 len)
{
	wlc_info_t *wlc = eq->wlc;
	void *p;
	char *ptr;
	bcm_event_t *msg;
	uint pktlen;
	wlc_bsscfg_t *cfg;
	struct scb *scb = NULL;
	uint16 adjlen = 0;

	BCM_REFERENCE(wlc);

	ASSERT(e != NULL);
	ASSERT(e->wlcif != NULL);

	BCM_REFERENCE(wlc);
	BCM_REFERENCE(p);

	pktlen = sizeof(bcm_event_t) + len + 2;
	if ((p = wlc_event_pktget(eq, pktlen, e->event.event_type)) == NULL) {
		if (EVDATA_BSSINFO_ENAB(wlc->pub)) {
			/* If we can remove optional info, try again */
			adjlen = wlc_event_strip_bssinfo(eq, e, data, len);
			ASSERT((adjlen <= len) && (adjlen <= e->event.datalen));
			if (adjlen != 0) {
				p = wlc_event_pktget(eq, pktlen - adjlen,
				                     e->event.event_type);
			}
		}
		if (p == NULL) {
			WL_ERROR(("wl%d: wlc_event_sendup: failed to get a pkt for %d dlen %d\n",
			          wlc->pub->unit, e->event.event_type, e->event.datalen));
			return wlc_event_tracking(eq, e, FALSE);
		}
		len = len - adjlen;
	} else {
		wlc_event_tracking(eq, e, TRUE);
	}

	ASSERT(ISALIGNED(PKTDATA(wlc->osh, p), sizeof(uint32)));

	msg = (bcm_event_t *) PKTDATA(wlc->osh, p);

	bcopy(da, &msg->eth.ether_dhost, ETHER_ADDR_LEN);
	bcopy(sa, &msg->eth.ether_shost, ETHER_ADDR_LEN);

	/* Set the locally administered bit on the source mac address if both
	 * SRC and DST mac addresses are the same. This prevents the downstream
	 * bridge from dropping the packet.
	 * Clear it if both addresses are the same and it's already set.
	 */
	if (!bcmp(&msg->eth.ether_shost, &msg->eth.ether_dhost, ETHER_ADDR_LEN))
		ETHER_TOGGLE_LOCALADDR(&msg->eth.ether_shost);

	msg->eth.ether_type = hton16(ETHER_TYPE_BRCM);

	/* BCM Vendor specific header... */
	msg->bcm_hdr.subtype = hton16(BCMILCP_SUBTYPE_VENDOR_LONG);
	msg->bcm_hdr.version = BCMILCP_BCM_SUBTYPEHDR_VERSION;
	bcopy(BRCM_OUI, &msg->bcm_hdr.oui[0], DOT11_OUI_LEN);
	/* vendor spec header length + pvt data length (private indication
	 * hdr + actual message itself)
	 */
	msg->bcm_hdr.length = hton16(BCMILCP_BCM_SUBTYPEHDR_MINLENGTH +
	                             BCM_MSG_LEN +
	                             (uint16)len);
	msg->bcm_hdr.usr_subtype = hton16(BCMILCP_BCM_SUBTYPE_EVENT);

	/* update the event struct */
	wlc_assign_event_msg(wlc, &msg->event, e, data, len);

	PKTSETLEN(wlc->osh, p, (sizeof(bcm_event_t) + len + 2));

	ptr = (char *)(msg + 1);
	/* Last 2 bytes of the message are 0x00 0x00 to signal that there are
	 * no ethertypes which are following this
	 */
	ptr[len + 0] = 0x00;
	ptr[len + 1] = 0x00;

	wlc_event_mark_packet(wlc, p);

	cfg = wlc_bsscfg_find_by_wlcif(wlc, e->wlcif);
	ASSERT(cfg != NULL);

	if (e->wlcif->type == WLC_IFTYPE_WDS)
		scb = e->wlcif->u.scb;

	wlc_sendup_event(wlc, cfg, scb, p);

	return BCME_OK;
}

int
wlc_eventq_set_ind(wlc_eventq_t* eq, uint et, bool enab)
{
	if (et >= WLC_E_LAST)
		return -1;
	if (enab)
		setbit(eq->event_inds_mask, et);
	else
		clrbit(eq->event_inds_mask, et);

	if (et == WLC_E_PROBREQ_MSG)
		wlc_enable_probe_req(eq->wlc, PROBE_REQ_EVT_MASK, enab? PROBE_REQ_EVT_MASK:0);

	return 0;
}
int
wlc_eventq_set_all_ind(wlc_info_t *wlc, wlc_eventq_t* eq, uint et, bool enab)
{
	int i = 0;
	for (i = 0; i < MAX_RSDB_MAC_NUM; i++) {
		wlc_eventq_set_ind(wlc->cmn->wlc[i]->eventq,
				et, enab);
	}
	return 0;
}
#endif /* !WLNOEIND */
