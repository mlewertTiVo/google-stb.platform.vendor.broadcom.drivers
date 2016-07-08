/*
 * Support for power-save mode with muted transmit path.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_pm_mute_tx.c 2014-01-31 mgs$
 */

#include <osl.h>
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_pub.h>
#include <wlc_key.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_bmac.h>
#include <wlc_assoc.h>
#include <wlc_alloc.h>
#include <wl_export.h>
#include <wlc_scan.h>
#include <wlc_pm_mute_tx.h>

#define PS_TIMEOUT	10

#ifdef PMMT_ENABLE_MUTE
#define MUTE_TIMEOUT	5
#else
#define MUTE_TIMEOUT	0
#endif

#ifdef	PMMTDBG
#define	WL_PMMT WL_PRINT
#else
#define	WL_PMMT WL_NONE
#endif

#define update_pmmt_fsm(pmmt, event) pmmt_fsm[pmmt->state][event](pmmt)

/* definitions for iovar */

enum {
	IOV_PM_MUTE_TX,
	IOV_LAST
};

static const bcm_iovar_t wlc_pm_mute_tx_iovars[] = {
	{"pm_mute_tx", IOV_PM_MUTE_TX,
	(IOVF_SET_UP | IOVF_BSSCFG_STA_ONLY), IOVT_BUFFER, sizeof(wl_pm_mute_tx_t)
	},
	{NULL, 0, 0, 0, 0}
};

/* pm_mute_tx fsm: definitions of states, events and fwd declaration of function calls */

/* ops to enable pm_mute_tx mode */
static void wlc_pm_mute_tx_fsm_enable(wlc_pm_mute_tx_t *pmmt);

/* ops to enter power-save mode */
static void wlc_pm_mute_tx_fsm_pm(wlc_pm_mute_tx_t *pmmt);

/* ops after pm_mute_tx was completed, and verifies succ or fail */
static void wlc_pm_mute_tx_fsm_complete(wlc_pm_mute_tx_t *pmmt);

/* performs intrnl reset and enters disabled_st */
static void wlc_pm_mute_tx_fsm_reset(wlc_pm_mute_tx_t *pmmt);

/* invalid operation. */
static void wlc_pm_mute_tx_fsm_invop(wlc_pm_mute_tx_t *pmmt);

/* no operation. */
static void wlc_pm_mute_tx_fsm_noop(wlc_pm_mute_tx_t *pmmt);

#ifdef PMMT_ENABLE_MUTE
/* executes ops to suspend tx fifos; */
static void wlc_pm_mute_tx_fsm_mute(wlc_pm_mute_tx_t *pmmt);
#endif

#ifdef PMMTDBG
/* status code for dbg */
static uint8 wlc_pm_mute_tx_fsm_status(wlc_pm_mute_tx_t *pmmt);
#endif

typedef enum pm_mute_tx_ev_enum {
	DIS_EV	= 0,		/* request to disable pm_mute_tx mode; */
	EN_EV	= 1,		/* request to enable pm_mute_tx mode; */
	MAX_EVENTS
} pm_mute_tx_ev;

/* fsm state/event table */

typedef void (*fsm_fn_t)(wlc_pm_mute_tx_t *pmmt);

static fsm_fn_t pmmt_fsm[MAX_STATES][MAX_EVENTS] = {
	/* events:	*	DIS_EV				EN_EV */
	/* states:	*/
	/* DISABLED_ST	*/	{wlc_pm_mute_tx_fsm_noop,	wlc_pm_mute_tx_fsm_enable},
	/* PM_ST	*/	{wlc_pm_mute_tx_fsm_invop,	wlc_pm_mute_tx_fsm_pm},
#ifdef PMMT_ENABLE_MUTE
	/* MUTE_ST	*/	{wlc_pm_mute_tx_fsm_invop,	wlc_pm_mute_tx_fsm_mute},
#endif
	/* COMPLETE_ST	*/	{wlc_pm_mute_tx_fsm_invop,	wlc_pm_mute_tx_fsm_complete},
	/* ENABLED_ST	*/	{wlc_pm_mute_tx_fsm_reset,	wlc_pm_mute_tx_fsm_noop},
	/* FAIL_ST	*/	{wlc_pm_mute_tx_fsm_reset,	wlc_pm_mute_tx_fsm_noop}
};

/* fwd declarations */

static int wlc_pm_mute_tx_doiovar(void *ctx, const bcm_iovar_t *vi, uint32 actionid,
	const char *name, void *params, uint p_len, void *arg, int len, int val_size,
	struct wlc_if *wlcif);
static void wlc_pm_mute_tx_timer(void *pmmt);

/* overall initialization */

#include <wlc_patch.h>

wlc_pm_mute_tx_t *
BCMATTACHFN(wlc_pm_mute_tx_attach)(wlc_info_t *wlc)
{
	wlc_pm_mute_tx_t *pmmt;

	ASSERT(wlc != NULL);

	if ((pmmt = MALLOCZ(wlc->osh, sizeof(wlc_pm_mute_tx_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}
	pmmt->wlc = wlc;

	if (wlc_module_register(wlc->pub, wlc_pm_mute_tx_iovars, "pm_mute_tx", (void *)pmmt,
		wlc_pm_mute_tx_doiovar, NULL, NULL, NULL) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto deallocate_mem;
	}

	if ((pmmt->timer = wl_init_timer(wlc->wl, wlc_pm_mute_tx_timer,
	     pmmt, "wlc_pm_mute_tx_timer")) == NULL) {
		WL_ERROR(("wl%d: wl_init_timer for wlc_pm_mute_tx_timer failed:%s\n",
			wlc->pub->unit, __FUNCTION__));
		goto unregister;
	}

	pmmt->state = DISABLED_ST;

	return pmmt;

unregister:
	wlc_module_unregister(wlc->pub, "wlc_pm_mute_tx", pmmt);

deallocate_mem:
	if (pmmt != NULL)
		MFREE(wlc->osh, pmmt, sizeof(wlc_pm_mute_tx_t));
	return NULL;
}

void
BCMATTACHFN(wlc_pm_mute_tx_detach)(wlc_pm_mute_tx_t *pmmt)
{
	ASSERT(pmmt != NULL);

	if (pmmt->timer != NULL) {
		wl_del_timer(pmmt->wlc->wl, pmmt->timer);
		wl_free_timer(pmmt->wlc->wl, pmmt->timer);
		pmmt->timer = NULL;
	}

	wlc_module_unregister(pmmt->wlc->pub, "pm_mute_tx", pmmt);

	MFREE(pmmt->wlc->osh, pmmt, sizeof(wlc_pm_mute_tx_t));
}

/* iovar function call */

static int
wlc_pm_mute_tx_doiovar(void *ctx, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int val_size, struct wlc_if *wlcif)
{
	wlc_pm_mute_tx_t *pmmt;
	wl_pm_mute_tx_t *wl_params;

	ASSERT(ctx != NULL);

	pmmt = (wlc_pm_mute_tx_t *)ctx;
	wl_params = (wl_pm_mute_tx_t *)arg;
	pmmt->bsscfg = wlc_bsscfg_find_by_wlcif(pmmt->wlc, wlcif);

	if (wl_params->version != WL_PM_MUTE_TX_VER) {
		WL_ERROR(("error: inconsistent pm_mute_tx iovar version used by firmware "
		          "and wl.\n"));
		return BCME_VERSION;
	}

	if (wl_params->len != sizeof(wl_pm_mute_tx_t)) {
		WL_ERROR(("error: inconsistent pm_mute_tx iovar length used by firmware "
		          "and wl.\n"));
		return BCME_BADLEN;
	}

	switch (actionid) {
	case IOV_SVAL(IOV_PM_MUTE_TX):
		if (wl_params->enable < MAX_EVENTS) {
			pmmt->deadline = wl_params->deadline;
			update_pmmt_fsm(pmmt, (pm_mute_tx_ev)wl_params->enable);
			break;
		} else {
			WL_ERROR(("wl_pm_mute_tx parameter input: not in expected range.\n"));
			return BCME_BADARG;
		}
	default:
		/* should take care of IOV_GVAL(IOV_PM_MUTE_TX) until its implementation */
		return BCME_UNSUPPORTED;
		break;
	}
	return BCME_OK;
}

/* timer call */

static void
wlc_pm_mute_tx_timer(void *arg)
{
	wlc_pm_mute_tx_t *pmmt;

	ASSERT(arg != NULL);

	pmmt = (wlc_pm_mute_tx_t *)arg;

	/*
	   Note: the current state is not verified/checked in this timer function on purpose,
	   as it was deliberately created to be general by proceeding to next fsm state.
	   However, from an implementation-specific standpoint we will print error messages in
	   case this function is called in any state other than PM_ST, which shouldn't happen as
	   long as this function is aligned with the rest of current code. Therefore, when/if
	   mute functionality is enabled, the implementation-specific ckeck below will need
	   associated update.
	*/

	/* Implementation-specific check */

	if (pmmt->state != PM_ST) {
		WL_ERROR(("pm_mute_tx_timer function was invoked during state number %d, but "
			  "the curent implementation only does it during PM_ST.\n",
			  pmmt->state));
	}

	/* General check */

	if (pmmt->state < MAX_STATES-1) {
		WL_PMMT(("[pmmt] received timeout or completion notification for state number %d. "
		         "it will now advance to next state.\n", (int)pmmt->state));
		pmmt->state++;
		update_pmmt_fsm(pmmt, EN_EV);
	}
}

/* external event notification function calls */

void
wlc_pm_mute_tx_pm_pending_complete(wlc_pm_mute_tx_t *pmmt)
{
	ASSERT(pmmt != NULL);

	WL_PMMT(("[pmmt] pm status change notification received\n"));

	if (pmmt->state == PM_ST) {
		wl_del_timer(pmmt->wlc->wl, pmmt->timer);
		wl_add_timer(pmmt->wlc->wl, pmmt->timer, 0, 0);
	} else {
		WL_PMMT(("[pmmt] notification was received after pm timeout and "
		         "will be ignored.\n"));
	}
}

#ifdef PMMT_ENABLE_MUTE
void
wlc_pm_mute_tx_mute_notify(wlc_pm_mute_tx_t *pmmt)
{
	ASSERT(pmmt != NULL);

	wl_del_timer(pmmt->wlc->wl, pmmt->timer);
	wl_add_timer(pmmt->wlc->wl, pmmt->timer, 0, 0);
}
#endif /* PMMT_ENABLE_MUTE */

#ifdef PMMTDBG

static uint8
wlc_pm_mute_tx_fsm_status(wlc_pm_mute_tx_t *pmmt)
{
	return ((pmmt->bsscfg->pm->PMenabled == FALSE) << 3) | pmmt->state;
}
#endif

/* fsm processiang function calls */

static void
wlc_pm_mute_tx_fsm_enable(wlc_pm_mute_tx_t *pmmt)
{
	wlc_scan_abort(pmmt->wlc->scan, WLC_E_STATUS_ABORT);
	pmmt->state = PM_ST;
	update_pmmt_fsm(pmmt, EN_EV);
}

static void
wlc_pm_mute_tx_fsm_pm(wlc_pm_mute_tx_t *pmmt)
{
	if (pmmt->deadline <= (PS_TIMEOUT + MUTE_TIMEOUT)) {
		if (pmmt->deadline == 0) {
			WL_PMMT(("[pmmt] deadline was set to 0, will use internal default "
			         "value instead.\n"));
			pmmt->deadline = PS_TIMEOUT + MUTE_TIMEOUT;
		} else {
			WL_PMMT(("[pmmt] error: deadline (%d) <= min allowed time for "
			         "request (%d).\n", pmmt->deadline,
			         PS_TIMEOUT+MUTE_TIMEOUT));
			pmmt->state = FAIL_ST;
			return;
		}
	}
	pmmt->initial_pm = pmmt->bsscfg->pm->PMenabled;
	if (!pmmt->initial_pm) {
		wlc_set_pmstate(pmmt->bsscfg, TRUE);
		WL_PMMT(("[pmmt] setting up timer for pm state change\n"));
		wl_add_timer(pmmt->wlc->wl, pmmt->timer, pmmt->deadline, 0);
	} else {
		pmmt->state = COMPLETE_ST;
		update_pmmt_fsm(pmmt, EN_EV);
	}
}

#ifdef PMMT_ENABLE_MUTE
static void
wlc_pm_mute_tx_fsm_mute(wlc_pm_mute_tx_t *pmmt)
{
	wlc_mute(pmmt->wlc, ON, 0);
	wl_add_timer(pmmt->wlc->wl, pmmt->timer, MUTE_TIMEOUT, 0);
}
#endif

static void
wlc_pm_mute_tx_fsm_complete(wlc_pm_mute_tx_t *pmmt)
{
	if (pmmt->wlc->cfg->pm->PMenabled) {
		pmmt->state = ENABLED_ST;
#ifdef PMMTDBG
		WL_PMMT(("[pmmt] PM is currently ON, mode is enabled "
		         "with status %d.\n", pmmt->state));
#endif
	} else {
		pmmt->state = FAIL_ST;
		WL_PMMT(("[pmmt] PM is currently off, couldn't be enabled: fail state %d.\n",
		         pmmt->state));
	}
#ifdef PMMTDBG
	WL_PMMT(("[pmmt] final status code is %d\n", wlc_pm_mute_tx_fsm_status(pmmt)));
#endif

#ifdef PMMT_ENABLE_MUTE
	if (wlc_bmac_tx_fifo_suspended(pmmt->wlc->hw, TX_CTL_FIFO)) {
		wlc_clear_mac(pmmt->bsscfg);
		pmmt->state = ENABLED_ST;
		WL_PMMT(("[pmmt] pm_mute_tx is enabled, end of pm_mute_tx processing. "
		         "current status code is %ud.\n", pm_mute_tx_fsm_status(pmmt)));
	} else {
		pmmt->state = FAIL_ST;
		WL_PMMT(("[pmmt] unable to suspend fifos: entering fail state.\n"));
	}
#endif
}

static void
wlc_pm_mute_tx_fsm_reset(wlc_pm_mute_tx_t *pmmt)
{
	/* deletes timer if not expired. */
	wl_del_timer(pmmt->wlc->wl, pmmt->timer);

#ifdef PMMT_ENABLE_MUTE
	/* unmuting and resetting mac */
	wlc_mute(pmmt->wlc, OFF, 0);
	wlc_set_mac(pmmt->bsscfg);
#endif

	/* reverting to original power state */
	if (!pmmt->initial_pm)
		wlc_set_pmstate(pmmt->bsscfg, FALSE);

	/* nothing needs to be done to resume scans */

	pmmt->state = DISABLED_ST;

	WL_PMMT(("[pmmt] states for power save, tx queue and timer have been reset; "
	         "pm_mute_tx is now disabled.\n"));
}

static void
wlc_pm_mute_tx_fsm_invop(wlc_pm_mute_tx_t *pmmt)
{
	WL_PMMT(("[pmmt] invalid operation.\n"));
}

static void
wlc_pm_mute_tx_fsm_noop(wlc_pm_mute_tx_t *pmmt)
{
	WL_PMMT(("[pmmt] no operation being executed.\n"));
}
