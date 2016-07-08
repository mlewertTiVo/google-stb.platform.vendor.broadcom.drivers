/*
 * IOPOS (Nintendo Revolution) -specific portion of
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
 * $Id: wl_iopos.c 467328 2014-04-03 01:23:40Z $
 */


#include <wlc_cfg.h>

#include <typedefs.h>
#include <osl.h>

#include <ios.h>
#include <iostypes.h>
#include <iosresource.h>
#include <priority.h>

#include <epivers.h>
#include <pcicfg.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <wlioctl.h>
#include <sdio_iopos.h>

typedef const struct si_pub	si_t;
#include <wlc_pub.h>
#include <wl_dbg.h>
#include <wl_export.h>

#include <nintendowm.h>
#include <bcmsdh.h>

/* Message Q sizes */
#define WL_RMQSIZE 5             /* depth for the RM queue */
#define WL_EVENTQSIZE (MAX_TIMERS + 1 + 2)    /* Event Q depth: 1 per timer + 1 dpc + 2 spare */
#define WL_LOCKQSIZE 1           /* depth of lock queue */
#define WL_EVENTTHREAD_STACKSIZE	(2*4096)

#define WL_LOCK(wl)	\
	do { \
		IOSMessage lock_msg; \
		IOSError err; \
		err = IOS_ReceiveMessage((wl)->lock_qid, &lock_msg, IOS_MESSAGE_BLOCK); \
		ASSERT(err == IOS_ERROR_OK); \
	} while (0);

#define WL_UNLOCK(wl) \
	do { \
		IOSMessage lock_msg = NULL; \
		IOSError err; \
		err = IOS_SendMessage((wl)->lock_qid, lock_msg, IOS_MESSAGE_NOBLOCK); \
		ASSERT(err == IOS_ERROR_OK); \
	} while (0);

#define WL_INITLOCK(wl) WL_UNLOCK(wl)

#define WL_EVENT_TIMER   0  /* Timer event */
#define WL_EVENT_DPC     1  /* DPC event */
#define WL_EVENT_SD	 2  /* SD Interrupt */

#define SD_HOST_CTRL_1	1 /* Soldered on Host controller */

typedef struct wl_event {
	uint type;              /* Event type */
} wl_event_t;

extern void sd_register_qid(IOSMessageQueueId wl_qid);

typedef struct wl_timer {
	wl_event_t event;       /* event structure (must be the first one) */
	struct wl_info *wl;     /* back pointer to wl */
	void (*fn)(void *);     /* timer callback function */
	void *arg;              /* arg for the callback function */
	uint us;                /* timer interval in microseconds */
	bool periodic;          /* periodic or one shot */
	bool set;               /* timer valid */
	IOSTimerId id;          /* os timer handle */
} wl_timer_t;

typedef struct wl_dpcevent {
	wl_event_t event;       /* event structure (must be the first one) */
} wl_dpcevent_t;

typedef struct wl_sdevent {
	wl_event_t event;       /* event structure (must be the first one) */
} wl_sdevent_t;

struct wl_if {
	struct wl_if *next;     /* pointer to next intf in the list */
	struct wl_info *wl;     /* back pointer to main wl_info_t */
	struct wlc_if *wlcif;   /* wlc interface handle */
	char ifname[32];        /* interface name */
	uint subunit;           /* BSS unit */
};

typedef struct wl_info {
	wlc_pub_t *pub;         /* pointer to common os-independent data */
	wlc_info_t *wlc;        /* pointer to common os-independent data */
	osl_t *osh;             /* osh */
	void            *sdh;
	void *regsva;           /* pointer to core registers */
	uint unit;              /* unit number 0 */
	uint bustype;           /* bus type */
	char ifname[32];        /* Main interface name */
	wl_if_t *if_list;       /* list of all interfaces */
	wl_dpcevent_t dpcevent; /* Event structure for DPC */
	uint ntimers;           /* Number of timers allocated */
	wl_timer_t	timers[MAX_TIMERS];    /* timer list */
	wl_sdevent_t	sd_event;
	IOSThreadId	event_tid;               /* event handling thread id */
	IOSMessageQueueId event_qid;         /* event message q id */
	IOSMessageQueueId lock_qid;          /* Locking q id */
	IOSMessageQueueId rm_qid;            /* Resource manager q id */
	IOSMessage rmq[WL_RMQSIZE];          /* Resource Mgr Q */
	IOSMessage eventq[WL_EVENTQSIZE];    /* event message q */
	IOSMessage lockq[WL_LOCKQSIZE];      /* Locking message q */
	IOSMessageQueueId sendup_qid;        /* 0th sendup q id */
	IOSFd			  sendup_fd;			 /* 0th RM sendup descriptor */
} wl_info_t;


#define STACKSIZE (12*1024)

#define IOS_ThreadExit exit

const uint8 _initStack[STACKSIZE];       /* Stack for the main process */
const uint32 _initStackSize = STACKSIZE; /* Stack size for the main process */
const uint32 _initPriority = IOS_PRIORITY_WL;    /* priority of the main process */

static uint8 event_thread_stack[WL_EVENTTHREAD_STACKSIZE] __attribute__ ((aligned(4)));

static void wl_isr(wl_info_t *wl);
static void wl_timer(wl_timer_t *t);
static struct wl_if *wl_alloc_if(wl_info_t *wl, uint subunit, struct wlc_if* wlcif);
static void wl_detach(wl_info_t *wl);
static void wl_dpc(wl_info_t *wl, bool resched);

static int wlc_iopos_ioctl(wl_info_t *wl, int cmd, void *arg, int len,
	struct wlc_if *wlcif);
extern void sdstd_isr(void *foo);
extern bool wlc_nin_dispatch(wlc_info_t * wlc, void * pkt, struct wlc_if *wlcif);

/* Event thread that handles timer events and dpc event */
static void
wl_eventthread(uint32 data)
{
	wl_info_t *wl = (wl_info_t *)data;
	IOSMessage msg;
	IOSError err;
	wl_event_t *event;

	while (1) {
		/* Receive a command */
		if ((err = IOS_ReceiveMessage(wl->event_qid, &msg, IOS_MESSAGE_BLOCK))
		    != IOS_ERROR_OK) {
			WL_ERROR(("IOS_ReceiveMessage failed, err %d\n", err));
			continue;
		}

		event = (wl_event_t *)msg;
		switch (event->type) {
		case WL_EVENT_TIMER:
			wl_timer((wl_timer_t *)msg);
			break;

		case WL_EVENT_DPC:
			wl_dpc(wl, TRUE);
			break;

		case WL_EVENT_SD:
			sdstd_isr((void *)wl->sdh);

#ifndef USE_XFER_INTS
			if ((err = IOS_ClearAndEnableSDIntr(wl->sd_cntrlr)) != IOS_ERROR_OK) {
				WL_ERROR(("%s: IOS_ClearandEnable SD1 failed, err %d\n",
					__FUNCTION__, err));
			}
#endif
			break;

		default:
			WL_ERROR(("%s: Unknown Message type %d msg = 0x%x\n",
				__FUNCTION__, event->type, msg));
		}

	}
}


/* Create the Event thread and Event Q */
static bool
wl_eventthread_init(wl_info_t *wl)
{
	IOSError err;
	uint32 priority;

	/* Create event queue */
	if ((wl->event_qid = IOS_CreateMessageQueue(wl->eventq, WL_EVENTQSIZE)) < 0)
	{
		WL_ERROR(("Message Q create failed, err %d\n", wl->event_qid));
		return FALSE;
	}

	/* Register wl event q with sd driver */
	sd_register_qid(wl->event_qid);

	/* get current thread priority */
	priority = IOS_GetThreadPriority(0);

	/* Create the event thread */
	if ((wl->event_tid = IOS_CreateThread(wl_eventthread, wl,
	                         (event_thread_stack + WL_EVENTTHREAD_STACKSIZE),
	                         WL_EVENTTHREAD_STACKSIZE, priority,
	                         IOS_THREAD_CREATE_JOINABLE)) < 0)
	{
		WL_ERROR(("Event thread create failed, err %d\n", wl->event_tid));
		goto threrror;
	}

	if ((err = IOS_StartThread(wl->event_tid)) != IOS_ERROR_OK) {
		WL_ERROR(("Event thread start failed, err %d\n", err));
		goto threrror;
	}

#ifndef USE_XFER_INTS
	wl->sd_event.event.type = WL_EVENT_SD;
	if ((err = IOS_HandleEvent(wl->sd_eventnum, wl->event_qid,
	                           (IOSMessage)&wl->sd_event)) != IOS_ERROR_OK) {
		WL_ERROR(("%s: IOS_HandleEvent SD1 failed, err %d\n", __FUNCTION__, err));
		goto threrror;
	}
#endif

	return TRUE;

threrror:
	if (wl->event_tid) {
		IOS_DestroyThread(wl->event_tid, NULL);
		wl->event_tid = 0;
	}

	if (wl->event_qid) {
		IOS_DestroyMessageQueue(wl->event_qid);
		wl->event_qid = 0;
	}

	return FALSE;
}

static void
wl_sdio_isr(void *arg)
{
	wl_isr(arg);
}

#define SDIO_SLOT_1 0x0d080000 /* Slot 1 - soldered on */
#define SDIO_SLOT_0 0x0d070000	/* Slot 0 - plugin slot */

#define WL_DEBUG_TYPE "Release"

static wl_info_t *
wl_attach(uint16 vend_id, uint16 dev_id, uint16 bus, uint16 slot,
	uint16 func, uint bustype, void * regsva, osl_t * osh,
	void * param, char * dongle_file, char * srom_file)
{
	uint err;
	wl_info_t *wl = NULL;
	wl_if_t *wlif = NULL;
	const char __version[] = "$IOSVersion: WL: " WL_BUILD_TIME " "
		WL_TARGET_MEM_SIZE " Ver." EPI_VERSION_STR "/" WL_DEBUG_TYPE " $";

	if (osh == NULL) {
		if ((osh = osl_attach(bus, slot, func, bustype, TRUE)) == NULL) {
			WL_ERROR(("%s: osl_attach failed\n", __FUNCTION__));
			return NULL;
		}
	}

	printf("%s\n", __version);

	/* allocate private info */
	if ((wl = (wl_info_t*) MALLOC(osh, sizeof(wl_info_t))) == NULL) {
		WL_ERROR(("%s: wl malloc failure, malloced %d bytes\n",
		          __FUNCTION__, MALLOCED(osh)));
		osl_detach(osh);
		return NULL;
	}

	/* zero it out */
	bzero(wl, sizeof(wl_info_t));
	wl->osh = (void *)osh;
	wl->unit = 0;
	wl->bustype = bustype;
	wl->regsva = (void *)regsva;
	wl->sdh = (bcmsdh_info_t *)param;

	switch (slot) {
	case 1:
		wl->sd_eventnum = IOS_EVENT_SD1;
		wl->sd_cntrlr = 1;
		break;
	case 0:
		wl->sd_eventnum = IOS_EVENT_SD0;
		wl->sd_cntrlr = 0;
		break;
	default:
		WL_ERROR(("%s: Unknown slot 0x%x\n", __FUNCTION__, slot));
	}

	/* Create wl locking queue and initialize it */
	if ((wl->lock_qid = IOS_CreateMessageQueue(wl->lockq, WL_LOCKQSIZE)) < 0) {
		WL_ERROR(("Message Q create failed, err %d\n", wl->lock_qid));
		goto error;
	}

	/* Initialize the lock Q */
	WL_INITLOCK(wl);

	/* Create event thread/q */
	if (!wl_eventthread_init(wl))
		goto error;


	/* Create Resource Manager Message Queue */
	if ((wl->rm_qid = IOS_CreateMessageQueue(wl->rmq, WL_RMQSIZE)) < 0) {
		WL_ERROR(("Message Q create failed, err %d\n", wl->rm_qid));
		goto error;
	}
	/* Can't leave this at zero: valid descriptor value */
	wl->sendup_fd = -1;

	/* Create the main interface with subunit 0 */
	wlif = wl_alloc_if(wl, 0, NULL);
	if (!wlif) {
		WL_ERROR(("%s: wl_alloc_if failed\n", __FUNCTION__));
		goto error;
	}

	if (!(wl->wlc = (void *)wlc_attach(wl, vend_id, dev_id, wl->unit,
		TRUE, osh, wl->regsva, wl->bustype, (void *)param, NULL, &err))) {
		WL_ERROR(("%s: wlc_attach failed, error %d.\n", __FUNCTION__, err));
		goto error;
	}

	wl->pub = wlc_pub((void *)wl->wlc);

	if ((err = IOS_RegisterResourceManager("/dev/wl", wl->rm_qid)) != IOS_ERROR_OK) {
		WL_ERROR(("Resource Manager register failed, err %d\n", err));
		goto error;
	}

	if ((err = IOS_ClearAndEnableSDIntr(wl->sd_cntrlr)) != IOS_ERROR_OK) {
		WL_ERROR(("%s: IOS_ClearandEnable SD1 failed, err %d\n", __FUNCTION__, err));
	}

#undef TESTMODE
#ifdef TESTMODE
#define TESTNAME  "NDEV_dev"
	if (wlc_iovar_setint(wl->wlc, "mpc", 0)) {
		WL_ERROR(("%s: Error setting MPC variable to 0\n", __FUNCTION__));
	}

	wlc_set(wl->wlc, WLC_SET_INFRA, 1);
	wlc_set(wl->wlc, WLC_SET_AP, 1);

	err = wl_up(wl);
	WL_INFORM(("wl_up ret %d\n", err));

	{
		wlc_ssid_t ssid;

		ssid.SSID_len = strlen(TESTNAME);
		strcpy(ssid.SSID, TESTNAME);
		wlc_ioctl(wl->wlc, WLC_SET_SSID, &ssid, sizeof(ssid), NULL);
	}
#endif	/* TESTMODE */

	WL_ERROR(("%s: Complete\n", __FUNCTION__));

	return (wl);

error:

	wl_detach(wl);
	return NULL;
}

static void
wl_detach(wl_info_t *wl)
{
	osl_t *osh = wl->osh;
	int i;

	/* Free iflist */
	while (wl->if_list)
		wl_del_if(wl, wl->if_list);

	/* detach from wlc */
	if (wl->wlc)
		wlc_detach(wl->wlc);

	/* free timers */
	for (i = 0; i < MAX_TIMERS; i++)
		wl_del_timer(wl, &wl->timers[i]);

	/* kill tasks */
	if (wl->event_tid)
		IOS_DestroyThread(wl->event_tid, NULL);

	/* free msg queues */
	if (wl->event_qid)
		IOS_DestroyMessageQueue(wl->event_qid);

	if (wl->lock_qid)
		IOS_DestroyMessageQueue(wl->lock_qid);

	if (wl->rm_qid)
		IOS_DestroyMessageQueue(wl->lock_qid);

	MFREE(osh, wl, sizeof(wl_info_t));
	if (osh)
		osl_detach(osh);

}

/* Create a subinterface for wl */
static struct wl_if *
wl_alloc_if(wl_info_t *wl, uint subunit, struct wlc_if* wlcif)
{
	wl_if_t *wlif;
	wl_if_t *p;

	if (!(wlif = MALLOC(wl->osh, sizeof(wl_if_t)))) {
		WL_ERROR(("wl%d: wl_alloc_if: out of memory, malloced %d bytes\n",
		          (wl->pub)?wl->pub->unit:subunit, MALLOCED(wl->osh)));
		return NULL;
	}
	bzero(wlif, sizeof(wl_if_t));

	wlif->wl = wl;
	wlif->wlcif = wlcif;
	wlif->subunit = subunit;
	sprintf(wlif->ifname, "/dev/wl%d", subunit);

	/* add the interface to the interface linked list */
	if (wl->if_list == NULL)
		wl->if_list = wlif;
	else {
		p = wl->if_list;
		while (p->next != NULL)
			p = p->next;
		p->next = wlif;
	}

	return wlif;
}

/* Remove a subinterface for wl */
static void
wl_free_if(wl_info_t *wl, wl_if_t *wlif)
{
	wl_if_t *p;

	/* remove the interface from the interface linked list */
	p = wl->if_list;
	if (p == wlif)
		wl->if_list = p->next;
	else {
		while (p != NULL && p->next != wlif)
			p = p->next;
		if (p != NULL)
			p->next = p->next->next;
	}

	MFREE(wl->osh, wlif, sizeof(wl_if_t));
}

/* Add subinterface for wl */
struct wl_if *
wl_add_if(wl_info_t *wl, struct wlc_if* wlcif, uint subunit, struct ether_addr *remote)
{
	return (wl_alloc_if(wl, subunit, wlcif));
}

/* Delete subinterface for wl */
void
wl_del_if(wl_info_t *wl, wl_if_t *wlif)
{
	wl_free_if(wl, wlif);
}

/* Return pointer to interface name */
char *
wl_ifname(wl_info_t *wl, wl_if_t *wlif)
{
	return ((wlif) ? wlif->ifname : wl->ifname);
}

void
wl_txflowcontrol(wl_info_t *wl, struct wl_if *wlif, bool state, int prio) {}

void
wl_init(wl_info_t *wl)
{
	WL_TRACE(("wl%d: wl_init\n", wl->pub->unit));

	wl_reset(wl);

	wlc_init(wl->wlc);
}

uint
wl_reset(wl_info_t *wl)
{
	WL_TRACE(("wl%d: wl_reset\n", wl->pub->unit));

	wlc_reset(wl->wlc);

	return (0);
}

bool
wl_alloc_dma_resources(wl_info_t *wl, uint addrwidth)
{
	return TRUE;
}

/*
 * These are interrupt on/off entry points. Disable interrupts
 * during interrupt state transition.
 */
void
wl_intrson(wl_info_t *wl)
{
	wlc_intrson(wl->wlc);
}

uint32
wl_intrsoff(wl_info_t *wl)
{
	return (wlc_intrsoff(wl->wlc));
}

void
wl_intrsrestore(wl_info_t *wl, uint32 macintmask)
{
	wlc_intrsrestore(wl->wlc, macintmask);
}

int
wl_up(wl_info_t *wl)
{
	int error = 0;
	IOSError ioserr;

	WL_TRACE(("wl%d: wl_up\n", wl->pub->unit));

	if (wl->pub->up)
		return (0);

	if ((ioserr = IOS_SetThreadPriority(0, 27)) != IOS_ERROR_OK)
		WL_ERROR(("wl%d: wl_up: setting thread priority failed. err %d\n",
		          wl->pub->unit, ioserr));

	error = wlc_up(wl->wlc);

	if ((ioserr = IOS_SetThreadPriority(0, IOS_PRIORITY_WL)) != IOS_ERROR_OK)
		WL_ERROR(("wl%d: wl_up: setting thread priority failed. err %d\n",
		          wl->pub->unit, ioserr));


	return (error);
}

void
wl_down(wl_info_t *wl)
{
	IOSMessage msg;

	WL_TRACE(("wl%d: wl_down\n", wl->pub->unit));

	/* call common down function */
	wlc_down(wl->wlc);

	wl->pub->hw_up = FALSE;

	/* flush event queue here , make async calls */
	while (IOS_ReceiveMessage(wl->rm_qid, &msg, IOS_MESSAGE_NOBLOCK) == IOS_ERROR_OK);
}

#define NWM_WD_IFNAME	"/dev/listen"
/* #define USE_IOPOS_QUEUES 1 */
void
wl_sendup(wl_info_t *wl, wl_if_t *wlif, void *p, int numpkt)
{
	IOSError err;
	IOSIobuf *iob;
	struct ether_header *pehdr;

	WL_TRACE(("wl%d: wl_sendup: %d bytes\n", wl->pub->unit, PKTLEN(wl->osh, p)));


	/* route packet to the appropriate interface */
	/* drop if the interface is not up yet */
#ifdef USE_IOPOS_QUEUES
	/* In our case this means if we don't have a valid qid ... */
	if (wl->sendup_qid <= 0) {
		PKTFREE(wl->osh, p, FALSE);
		WL_NIN(("wl%d: wl_sendup: sendup qid not set: dropping pkt\n",
		        wl->pub->unit));
		return;
	}
#else
	/* If the RM sendup descriptor isn't valid, try to open */
	if (wl->sendup_fd < 0) {
		wl->sendup_fd = IOS_Open(NWM_WD_IFNAME, 0);
		/* still? */
		if (wl->sendup_fd < 0) {
			PKTFREE(wl->osh, p, FALSE);
			WL_NIN(("wl%d: wl_sendup: failed to open RM %s dropping pkt\n",
				wl->pub->unit, NWM_WD_IFNAME));
			return;
		}
	}
#endif /* USE_IOPOS_QUEUES */


	/* Convert the packet, mainly detach the pkttag */
	pehdr = (struct ether_header *)PKTDATA(wl->osh, p);
	iob = PKTTONATIVE(wl->osh, p);
	/*  strip ether header */
	PKTPULL(wl->osh, iob, ETHER_HDR_LEN);

	ASSERT(ISALIGNED(iob->data, 4));

	/* send it up */
	WL_TRACE(("wl%d: wl_sendup(): pkt %p on interface (%s)\n",
		wl->pub->unit, iob, NWM_WD_IFNAME));

#ifdef USE_IOPOS_QUEUES
	err = IOS_SendMessage(wl->sendup_qid, (IOSMessage)iob, 0);
	if (err) {
		WL_NIN(("wl%d %s: error %d posting to sendup queue id %d\n",
		        wl->pub->unit, __FUNCTION__, err, wl->sendup_qid));
		PKTFREE(wl->osh, iob, FALSE);
	}
#else
	err = IOS_Write(wl->sendup_fd, iob, sizeof(IOSIobuf));
	if (err) {
		WL_NIN(("wl%d %s: error %d from IOS_WriteAsync on sendup_fd %d\n",
		       wl->pub->unit, __FUNCTION__, err, wl->sendup_fd));
		PKTFREE(wl->osh, iob, FALSE);
	}
#endif /* USE_IOPOS_QUEUES */

}

void
wl_dump_ver(wl_info_t *wl, struct bcmstrbuf *b)
{
	const char __version[] = "$IOSVersion: WL: " WL_BUILD_TIME " "
		WL_TARGET_MEM_SIZE " Ver." EPI_VERSION_STR "/" WL_DEBUG_TYPE " $";

	bcm_bprintf(b, "%s\n", __version);
	WL_ERROR(("%s\n", __version));
}

void
wl_event(wl_info_t *wl, char *ifname, wlc_event_t *e)
{
	return;
}

void
wl_event_sync(wl_info_t *wl, char *ifname, wlc_event_t *e)
{
}

void
wl_monitor(wl_info_t *wl, wl_rxsts_t *rxsts, void *p)
{
	WL_ERROR(("wl_monitor not supported"));
}

void
wl_set_monitor(wl_info_t *wl, int val)
{
	WL_ERROR(("wl_set_monitor not supported"));
	ASSERT(0);
}

static void
wl_timer(wl_timer_t *t)
{
	wl_info_t *wl;

	wl = t->wl;

	WL_LOCK(wl);

	if (t->set) {
		IOS_DestroyTimer(t->id);
		if (t->periodic) {
			t->id = IOS_CreateTimer((t->us), 0, wl->event_qid, (IOSMessage) t);
			ASSERT(t->id >= 0);
			t->set = TRUE;
		} else
			t->set = FALSE;

		t->fn(t->arg);
	}

	WL_UNLOCK(wl);
}

wl_timer_t *
wl_init_timer(wl_info_t *wl, void (*fn)(void *wlc), void *arg, const char *name)
{
	wl_timer_t *t;

	ASSERT(wl->ntimers < MAX_TIMERS);

	t = &wl->timers[wl->ntimers++];
	t->wl = wl;
	t->fn = fn;
	t->arg = arg;
	t->set = FALSE;
	t->periodic = FALSE;
	t->us = 0;
	t->id = 0;
	t->event.type = WL_EVENT_TIMER;

	return t;
}

void
wl_add_timer(wl_info_t *wl, wl_timer_t *t, uint ms, int periodic)
{
	ASSERT(!t->set);

	t->us = (ms ? (ms * 1000) : 1);
	t->periodic = (bool) periodic;
	t->set = TRUE;

	t->id = IOS_CreateTimer(t->us, 0, wl->event_qid, (IOSMessage)t);
	ASSERT(t->id >= 0);
}

/* return TRUE if timer successfully deleted, FALSE if still pending */
bool
wl_del_timer(wl_info_t *wl, wl_timer_t *t)
{
	if (t->set) {
		t->set = FALSE;
		if (IOS_DestroyTimer(t->id) != IOS_ERROR_OK)
			return FALSE;
	}

	return TRUE;
}

void
wl_free_timer(wl_info_t *wl, wl_timer_t *t)
{
	ASSERT(!t->set);
}

/* Enqueue dpc event to the event thread */
static IOSError
wl_dpcevent_sched(wl_info_t *wl)
{
	wl_dpcevent_t *dpcevent = &wl->dpcevent;

	dpcevent->event.type = WL_EVENT_DPC;

	/* Make non blocking call; since the current thread is responsible for dequeing the msg */
	return IOS_SendMessage(wl->event_qid, (IOSMessage)dpcevent, IOS_MESSAGE_NOBLOCK);
}

static void
wl_dpc(wl_info_t *wl, bool resched)
{
	IOSError err;

	WL_LOCK(wl);

	/* call the common second level interrupt handler */
	if (wl->pub->up) {
		wlc_dpc_info_t dpci = {0};
		if (resched)
			wlc_intrsupd(wl->wlc);
		resched = wlc_dpc(wl->wlc, TRUE, &dpci);
	}

	/* wlc_dpc() may bring the driver down */
	if (!wl->pub->up)
		return;

	/* re-schedule dpc */
	if (resched) {
		err = wl_dpcevent_sched(wl);
		/* If unable to schedule event, turn on interrupt and return */
		if (err != IOS_ERROR_OK) {
			wlc_intrson(wl->wlc);
		}
	}
	/* re-enable interrupts */
	else
		wlc_intrson(wl->wlc);

	WL_UNLOCK(wl);
}

static void
wl_isr(wl_info_t *wl)
{
	bool ours, wantdpc;

	WL_LOCK(wl);
	/* call common first level interrupt handler */
	if ((ours = wlc_isr(wl->wlc, &wantdpc))) {
		/* if more to do... */
		if (wantdpc) {
			/* ...and call the second level interrupt handler */
			WL_UNLOCK(wl);
			wl_dpc(wl, FALSE);
			WL_LOCK(wl);
		}
	}
	WL_UNLOCK(wl);

	return;
}


/* RM open call handler */
static IOSError
wl_open(wl_info_t *wl, IOSResourceRequest *req)
{
	IOSResourceOpen *args = &req->args.open;
	wl_if_t *wlif = wl->if_list;

	/* Walk through our interface list and compare the pathname with
	 * the interface name.
	 */
	while (wlif) {
		if (!strcmp(wlif->ifname, args->path))
			return ((IOSError)wlif);
		wlif = wlif->next;
	}

	return IOS_ERROR_NOEXISTS;
}


static u8 wlioctlbuffer[64 * 1024];
/* RM ioctl call handler */
static IOSError
wl_ioctl(wl_info_t *wl, IOSResourceRequest *req)
{
	IOSResourceIoctl *args = &req->args.ioctl;
	wl_if_t *wlif = (wl_if_t *)req->handle;
	int bcmerror;
	wl_ioctl_t *ioc;
	void *pkt;
	void *p;
	uint32 len;
	uint bytes_copied;

	ASSERT(IOS_IsValidIob((IOSIobuf *)args->inPtr) == IOS_ERROR_OK);
	WL_LOCK(wl);

	pkt = PKTFRMNATIVE(wl->osh, (void *)args->inPtr);

	switch (args->cmd) {

		case NWM_SIOCDEVPRIVATE :
		for (p = pkt, len = 0; p; p = PKTNEXT(wl->osh, p))
			len += PKTLEN(wl->osh, p);
		if (len > sizeof(wlioctlbuffer)) {
			bcmerror = BCME_BUFTOOLONG;
			PKTTONATIVE(wl->osh, pkt);
			break;
		}

		bytes_copied = pktcopy(wl->osh, pkt, 0, len, wlioctlbuffer);
		if (bytes_copied != len) {
			WL_ERROR(("%s: copy in error in bytes %d out bytes %d\n",
			          __FUNCTION__, len, bytes_copied));
			bcmerror = BCME_BADLEN;
			PKTTONATIVE(wl->osh, pkt);
			break;
		}
		ioc = (wl_ioctl_t *)wlioctlbuffer;
		bcmerror = wlc_ioctl(wl->wlc, ioc->cmd, &ioc[1], ioc->len, wlif->wlcif);
		if (bcmerror) {
			WL_ERROR(("%s: wlc_ioctl error %d cmd %d\n",
			          __FUNCTION__, bcmerror, ioc->cmd));
		}
		bytes_copied = wl_buf_to_pktcopy(wl->osh, pkt, wlioctlbuffer, len, 0);
		if (bytes_copied != len) {
			WL_ERROR(("%s: copy out error in bytes %d out bytes %d\n",
			          __FUNCTION__, len, bytes_copied));
			bcmerror = BCME_BADLEN;
		}
		PKTTONATIVE(wl->osh, pkt);
		break;

		case NWM_SIOCDEVPRIVATE_NINTENDO :
		/* NB:
		 * Function return values are
		 * TRUE : caller is responsible for pkt
		 * FALSE: callee is responsible: don't touch!
		 *
		 * No chaining of pkts
		 * everything fits in one unchained pkt by decree.
		 *
		 */
		if (wlc_nin_dispatch(wl->wlc, pkt, wlif->wlcif))
			PKTTONATIVE(wl->osh, pkt);
		bcmerror = 0;
		break;

		/* Communicate IOPOS config info here */
		case NWM_SIOCDEVPRIVATE_IOPOS :
		ioc = (wl_ioctl_t *)PKTDATA(wl->osh, pkt);
		bcmerror = wlc_iopos_ioctl(wl, ioc->cmd, &ioc[1], ioc->len, wlif->wlcif);
		PKTTONATIVE(wl->osh, pkt);
		break;

		default:
		bcmerror = BCME_BADARG;
		break;

	}

	WL_UNLOCK(wl);

	if (bcmerror != 0)
		wl->pub->bcmerror = bcmerror;
	return (OSL_ERROR(bcmerror));
}


/* main entry point */
int
main(int argc, char **argv)
{
	wl_info_t *wl;
	IOSError err;
	IOSMessage msg;
	IOSResourceRequest *req;

	if (!bcmsdh_register(&wl_sdio_driver)) {
		WL_ERROR(("%s: bcmsdioh_register returns error\n", __FUNCTION__));
		IOS_ThreadExit(0);
	}

#ifdef SUPPORT_PLUGIN_SLOT
	/* No longer support Plug-in slot, since it is used by IOS */
	WL_INFORM(("PLUG-IN SLOT: Attempting to initialize.\n"));
	if ((wl = (wl_info_t *)bcmsdh_pci_probe(VENDOR_BROADCOM, BCM4318_D11G_ID, 0, 0, 0,
	                                        PCI_BUS, (uint)SDIO_SLOT_0)) == NULL) {
		WL_INFORM(("PLUG-IN SLOT: Empty or cannot initialize\n"));
#endif
		WL_INFORM(("SOLDER SLOT: Attempting to initialize\n"));
		if ((wl = (wl_info_t *)bcmsdh_pci_probe(VENDOR_BROADCOM, BCM4318_D11G_ID, 0, 1, 0,
		                                        PCI_BUS, (uint)SDIO_SLOT_1)) == NULL)
			WL_ERROR(("SDIO SLOT 1: Cannot init!!!\n"));
		else
			WL_INFORM(("Using Soldered SDIO slot\n"));
#ifdef SUPPORT_PLUGIN_SLOT
	} else
		WL_ERROR(("Using Plug-In SDIO slot\n"));
#endif

	/* Main dispatch loop */
	while (1) {
		/* Receive a command */
		if ((err = IOS_ReceiveMessage(wl->rm_qid, &msg, IOS_MESSAGE_BLOCK))
		     != IOS_ERROR_OK) {
			WL_ERROR(("IOS_ReceiveMessage failed, err %d\n", err));
			continue;
		}

		/* Dispatch command */
		req = (IOSResourceRequest *) msg;
		switch (req->cmd) {
		case IOS_OPEN:
			err = wl_open(wl, req);
			break;

		case IOS_IOCTL:
			err = wl_ioctl(wl, req);
			break;

		case IOS_CLOSE:
			err = IOS_ERROR_OK;
			break;

		default:
			err = IOS_ERROR_INVALID;
			break;
		}

		IOS_ResourceReply(req, err);
	}

	wl_detach(wl);
	IOS_ThreadExit(0);
}

#ifdef BCMSDIOH_IOP
void
mem_dump(char *buf, int len)
{
}
#endif
static int
wlc_iopos_ioctl(wl_info_t *wl, int cmd, void *arg, int len, struct wlc_if *wlcif)
{
	int bcmerror = 0;
	nwm_iop_ioctl_t *p = (nwm_iop_ioctl_t *)arg;

	/* All the cmds need buffers */
	if ((arg == NULL) || (len <= 0 || len < sizeof(nwm_iop_ioctl_t))) {
		WL_ERROR(("%s: Command %d needs buffers\n", __FUNCTION__, cmd));
		bcmerror = BCME_ERROR;
		goto done;
	}


	switch (cmd) {

		case NWM_IOPOS_SET_SENDUP_QID:
			if (strncmp(WL_IFNAME_STRING, p->ifname, sizeof(WL_IFNAME_STRING))) {
				bcmerror = BCME_BADARG;
				goto done;
			}
			wl->sendup_qid = p->sendup_qid;
			break;

		default:
			bcmerror = BCME_ERROR;
			break;

	}
done:
	return bcmerror;
}

#define MAX_IOSIOB_BUFSIZE	2048


/* Set the length of the pkt chain */
int
wl_set_pktlen(osl_t *osh, void *p, int len)
{
	int n, req;

	for (req = len; p && len; p = PKTNEXT(osh, p), len -= n) {
		n = MIN(len, MAX_IOSIOB_BUFSIZE);
		PKTSETLEN(osh, p, n);
	}

	return (req - len);
}
/* return a chain of pkt's of total data length len */
void *
wl_get_pktbuffer(osl_t *osh, int len)
{
	void *head, *pkt, *tail;
	int buflen;

	tail = head = NULL;
	while (len > 0) {
		buflen = len > MAX_IOSIOB_BUFSIZE ? MAX_IOSIOB_BUFSIZE : len;
		pkt = PKTGET(osh, buflen, FALSE);
		if (pkt == NULL) {
			WL_ERROR(("%s: failed to allocate pkt buflen %d\n",
			__FUNCTION__, buflen));

			if (head)
				PKTFREE(osh, head, FALSE);
			return NULL;
		}
		if (head) {
			ASSERT(tail);
			PKTSETNEXT(osh, tail, pkt);
		} else {
			head = pkt;
		}
		tail = pkt;
		len -= buflen;
	}

	return head;
}

/* copy a buffer into a pkt chain
 * Caller should check return value against requested value.
 * May not equal bytes requested to copy (len arg):
 * could have run out of pkt space prematurely
 * Note:
 * This function works with PKTDATA, PKTLEN settins as received
 * in the pkt buffer chain but does NOT modify them.
 * It's up to the caller to manipulate those if desired.
 */
uint
wl_buf_to_pktcopy(osl_t *osh, void *p, uchar *buf, int len, uint offset)
{
	uint n, ret = 0;
#if defined(BCMDBG_ERR)
	void *porig = p;
	uint offset_orig = offset;
#endif 

	if (len < 0)
		len = 4096;	/* "infinite" */

	/* skip 'offset' bytes */
	for (; p && offset; p = PKTNEXT(osh, p)) {
		if (offset < (uint)PKTLEN(osh, p)) {
			break;
		}
		offset -= PKTLEN(osh, p);
	}

	if (!p) {
		WL_ERROR(("%s: ERROR: bytes %d offset %d to copy at end of chain orig pktlen %d\n",
		         __FUNCTION__, len, offset_orig, PKTLEN(osh, porig)));
		return 0;
	}

	/* copy the data */
	for (; p && len; p = PKTNEXT(osh, p)) {
		n = MIN((uint)(PKTLEN(osh, p) - offset), (uint)len);
		bcopy(buf, (char *)PKTDATA(osh, p) + offset, n);
		buf += n;
		len -= n;
		ret += n;
		offset = 0;
	}
	return ret;
}
