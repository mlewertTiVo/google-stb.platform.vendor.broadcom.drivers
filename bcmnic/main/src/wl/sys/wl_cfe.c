/*
 * CFE-specific portion of
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
 * $Id: wl_cfe.c 467328 2014-04-03 01:23:40Z $
 */

#include <wlc_cfg.h>

#include "lib_types.h"
#include "lib_malloc.h"
#include "lib_string.h"
#include "lib_printf.h"

#include "cfe_iocb.h"
#include "cfe_device.h"
#include "cfe_ioctl.h"
#include "cfe_console.h"
#include "cfe_timer.h"
#include "cfe_error.h"
#include "ui_command.h"

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <epivers.h>
#include <bcmendian.h>
#include <proto/ethernet.h>
#include <bcmdevs.h>
#include <wlioctl.h>

#include <proto/802.11.h>
#include <sbhndpio.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_channel.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>

#include <wl_export.h>

typedef struct wl_timer {
	int64_t timer;
	void (*fn)(void *);
	void *arg; /* arg to *fn */
	uint ms;
	bool periodic;
	struct wl_timer *next;
} wl_timer_t;

typedef struct wl_info {
	wlc_pub_t *pub;			/* pointer to public wlc state */
	void *wlc;			/* pointer to private common os-independent data */
	osl_t *osh;
	cfe_devctx_t *ctx;		/* backpoint to device */
	bool link;			/* link state */
	struct pktq rxq;		/* received packet queue */
	struct wl_timer *timers;	/* timer cleanup queue */
	struct wl_info *next;
} wl_info_t;

static wl_info_t *wl_list;

/* prototype called by cfe */
void wl_addcmd(void);

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

	return 0;
}

bool
wl_alloc_dma_resources(wl_info_t *wl, uint addrwidth)
{
	return TRUE;
}

/*
 * These are interrupt on/off enter points.
 * Since wl_isr is serialized with other drive rentries using spinlock,
 * They are SMP safe, just call common routine directly,
 */
void
wl_intrson(wl_info_t *wl)
{
	wlc_intrson(wl->wlc);
}

uint32
wl_intrsoff(wl_info_t *wl)
{
	return wlc_intrsoff(wl->wlc);
}

void
wl_intrsrestore(wl_info_t *wl, uint32 macintmask)
{
	wlc_intrsrestore(wl->wlc, macintmask);
}

int
wl_up(wl_info_t *wl)
{
	WL_TRACE(("wl%d: wl_up\n", wl->pub->unit));

	if (wl->pub->up)
		return 0;

	return wlc_up(wl->wlc);
}

void
wl_down(wl_info_t *wl)
{
	WL_TRACE(("wl%d: wl_down\n", wl->pub->unit));

	wlc_down(wl->wlc);

	/* Force hw to be brought up everytime wlc_up is done */
	wl->pub->hw_up = FALSE;
}

void
wl_dump_ver(wl_info_t *wl, struct bcmstrbuf *b)
{
	bcm_bprintf(b, "wl%d: %s %s version %s\n", wl->pub->unit,
		__DATE__, __TIME__, EPI_VERSION_STR);
}


void
wl_monitor(wl_info_t *wl, wl_rxsts_t *rxsts, void *p)
{
}

void
wl_set_monitor(wl_info_t *wl, int val)
{
}

char *
wl_ifname(wl_info_t *wl, struct wl_if *wlif)
{
	return "";
}

struct wl_if *
wl_add_if(wl_info_t *wl, struct wlc_if* wlcif, uint unit, struct ether_addr *remote)
{
	WL_ERROR(("wl_add_if: virtual interfaces not supported on CFE\n"));
	return NULL;
}

void
wl_del_if(wl_info_t *wl, struct wl_if *wlif)
{
}

wl_timer_t *
wl_init_timer(wl_info_t *wl, void (*fn)(void *arg), void *arg, const char* name)
{
	wl_timer_t *t;

	if (!(t = KMALLOC(sizeof(wl_timer_t), 0)))
		return 0;

	TIMER_CLEAR(t->timer);
	t->fn = fn;
	t->arg = arg;
	t->periodic = FALSE;
	t->ms = 0;
	t->next = wl->timers;
	wl->timers = t;

	return t;
}

void
wl_free_timer(wl_info_t *wl, wl_timer_t *t)
{
	KFREE(t);
}

void
wl_add_timer(wl_info_t *wl, wl_timer_t *t, uint ms, int periodic)
{
	t->ms = ms;
	t->periodic = (bool) periodic;
	TIMER_SET(t->timer, ms * CFE_HZ / 1000 / 2);
}

bool
wl_del_timer(wl_info_t *wl, wl_timer_t *t)
{
	TIMER_CLEAR(t->timer);

	return TRUE;
}

/*  *********************************************************************
 *  WL_PROBE(drv,probe_a,probe_b,probe_ptr)
 *
 *  Probe and install driver.
 *  Create a context structure and attach to the
 *  specified network device.
 *
 *  Input parameters:
 *  drv - driver descriptor
 *  probe_a - device ID
 *  probe_b - unit number
 *  probe_ptr - mapped registers
 *
 *  Return value:
 *  nothing
 * ********************************************************************
 */

static void
wl_probe(cfe_driver_t *drv, unsigned long probe_a, unsigned long probe_b, void *probe_ptr)
{
	wl_info_t *wl;
	uint16 device;
	uint unit;
	char name[128];
	int err;

	device = (uint16) probe_a;
	unit = (uint) probe_b;

	/* allocate private info */
	if (!(wl = (wl_info_t *) KMALLOC(sizeof(wl_info_t), 0))) {
		WL_ERROR(("wl%d: KMALLOC failed\n", unit));
		return;
	}
	bzero(wl, sizeof(wl_info_t));

	pktq_init(&wl->rxq, 1, PKTQ_LEN_DEFAULT);

	wl->osh = osl_attach(wl);
	ASSERT(wl->osh);

	/* common load-time initialization */
	if (!(wl->wlc = wlc_attach(wl,			/* wl */
				   VENDOR_BROADCOM,	/* vendor */
				   device,		/* device */
				   unit,		/* unit */
				   FALSE,		/* piomode */
				   wl->osh,			/* osh */
				   probe_ptr,		/* regsva */
				   PCI_BUS,		/* bustype */
				   wl,			/* sdh */
				   NULL,		/* objr */
				   &err))) {		/* perr */
		WL_ERROR(("wl%d: wlc_attach failed with error %d\n", unit, err));
		KFREE(wl);
		return;
	}
	wl->pub = wlc_pub((void *)wl->wlc);

	/* this is a polling driver - the chip intmask stays zero */
	wlc_intrsoff(wl->wlc);

	/* add us to the global linked list */
	wl->next = wl_list;
	wl_list = wl;

	/* print hello string */
	if (device > 0x9999)
		sprintf(name, "Broadcom BCM%d 802.11 Wireless Controller", device);
	else
		sprintf(name, "Broadcom BCM%04x 802.11 Wireless Controller", device);
	printf("wl%d: %s %s\n", unit, name, EPI_VERSION_STR);

	cfe_attach(drv, wl, NULL, name);
}

/*  *********************************************************************
 *  WL_POLL(ctx,ticks)
 *
 *  Check for changes in the PHY, so we can track speed changes.
 *
 *  Input parameters:
 *     ctx - device context (includes ptr to our softc)
 *     ticks- current time in ticks
 *
 *  Return value:
 *     nothing
 * ********************************************************************
 */

static void
wl_poll(cfe_devctx_t *ctx, int64_t ticks)
{
	wl_info_t *wl = ctx->dev_softc;
	wl_timer_t *t;
	bool dpc;

	WL_TRACE(("wl%d: wl_poll\n", wl->pub->unit));

	/* call common first level interrupt handler */
	if (wlc_isr(wl->wlc, &dpc)) {
		/* if more to do... */
		if (dpc) {
			wlc_dpc_info_t dpci = {0};
			/* call the common second level interrupt handler */
			wlc_dpc(wl->wlc, FALSE, &dpci);
		}
	}

	/* call timer functions */
	for (t = wl->timers; t; t = t->next) {
		if (TIMER_RUNNING(t->timer) &&
		    TIMER_EXPIRED(t->timer)) {
			if (t->periodic)
				wl_add_timer(wl, t, t->ms, t->periodic);
			else
				wl_del_timer(wl, t);
			t->fn(t->arg);
		}
	}
}

/*  *********************************************************************
 *  WL_OPEN(ctx)
 *
 *  Open the wireless device.  The MAC is reset, initialized, and
 *  prepared to receive and send packets.
 *
 *  Input parameters:
 *     ctx - device context (includes ptr to our softc)
 *
 *  Return value:
 *     status, 0 = ok
 * ********************************************************************
 */

static int
wl_open(cfe_devctx_t *ctx)
{
	wl_info_t *wl = ctx->dev_softc;
	int ret;

	WL_TRACE(("wl%d: wl_open\n", wl->pub->unit));

	if ((ret = wl_up(wl)))
		return ret;

	wl_poll(ctx, 0);

	return ret;
}

/*
 * The last parameter was added for the build. Caller of
 * this function should pass 1 for now.
 */
void
wl_sendup(wl_info_t *wl, struct wl_if *wlif, void *p, int numpkt)
{
	WL_TRACE(("wl%d: wl_sendup: %d bytes\n", wl->pub->unit, PKTLEN(NULL, p)));

	ASSERT(!wlif);

	/* Enqueue packet */
	pktenq(&wl->rxq, p);
}

/*  *********************************************************************
 *  WL_INPSTAT(ctx,inpstat)
 *
 *  Check for received packets on the wireless device
 *
 *  Input parameters:
 *     ctx - device context (includes ptr to our softc)
 *      inpstat - pointer to input status structure
 *
 *  Return value:
 *     status, 0 = ok
 * ********************************************************************
 */

static int
wl_inpstat(cfe_devctx_t *ctx, iocb_inpstat_t *inpstat)
{
	wl_info_t *wl = ctx->dev_softc;

	wl_poll(ctx, 0);

	inpstat->inp_status = !pktq_empty(&wl->rxq);

	return 0;
}

/*  *********************************************************************
 *  WL_READ(ctx,buffer)
 *
 *  Read a packet from the wireless device.  If no packets are
 *  available, the read will succeed but return 0 bytes.
 *
 *  Input parameters:
 *     ctx - device context (includes ptr to our softc)
 *      buffer - pointer to buffer descriptor.
 *
 *  Return value:
 *     status, 0 = ok
 * ********************************************************************
 */

static int
wl_read(cfe_devctx_t *ctx, iocb_buffer_t *buffer)
{
	wl_info_t *wl = ctx->dev_softc;
	void *p;

	WL_TRACE(("wl%d: wl_read\n", wl->pub->unit));

	/* assume no packets */
	buffer->buf_retlen = 0;

	/* poll for packet */
	wl_poll(ctx, 0);

	/* get packet */
	if (!(p = pktdeq(&wl->rxq)))
		return 0;

	bcopy(PKTDATA(NULL, p), buffer->buf_ptr, PKTLEN(NULL, p));
	buffer->buf_retlen = PKTLEN(NULL, p);

	/* RFC894: Minimum length of IP over Ethernet packet is 46 octets */
	if (buffer->buf_retlen < 60) {
		bzero(buffer->buf_ptr + buffer->buf_retlen, 60 - buffer->buf_retlen);
		buffer->buf_retlen = 60;
	}

	/* free packet */
	PKTFREE(wl->osh, p, FALSE);

	return 0;
}

/*  *********************************************************************
 *  WL_WRITE(ctx,buffer)
 *
 *  Write a packet to the wireless device.
 *
 *  Input parameters:
 *     ctx - device context (includes ptr to our softc)
 *      buffer - pointer to buffer descriptor.
 *
 *  Return value:
 *     status, 0 = ok
 * ********************************************************************
 */

static int
wl_write(cfe_devctx_t *ctx, iocb_buffer_t *buffer)
{
	wl_info_t *wl = ctx->dev_softc;
	void *p;

	WL_TRACE(("wl%d: wl_start: len %d\n", wl->pub->unit, buffer->buf_length));

	if (!(p = PKTGET(wl->osh, buffer->buf_length, TRUE))) {
		WL_ERROR(("wl%d: PKTGET failed\n", wl->pub->unit));
		return CFE_ERR_NOMEM;
	}

	bcopy(buffer->buf_ptr, PKTDATA(wl->osh, p), buffer->buf_length);

	wlc_sendpkt(wl->wlc, p, NULL);


	return 0;
}

void
wl_txflowcontrol(wl_info_t *wl, struct wl_if *wlif, bool state, int prio) {}

void
wl_event(wl_info_t *wl, char *ifname, wlc_event_t *e)
{
	switch (e->event.event_type) {
	case WLC_E_LINK:
	case WLC_E_NDIS_LINK:
		wl->link = e->event.flags&WLC_EVENT_MSG_LINK;
		if (e->event.flags&WLC_EVENT_MSG_LINK)
			WL_ERROR(("wl%d: link up\n", wl->pub->unit));
		else
			WL_ERROR(("wl%d: link down\n", wl->pub->unit));
		break;
	}
}

void
wl_event_sync(wl_info_t *wl, char *ifname, wlc_event_t *e)
{
}

void
wl_event_sendup(wl_info_t *wl, const wlc_event_t *e, uint8 *data, uint32 len)
{
}


/*  *********************************************************************
 *  WL_IOCTL(ctx,buffer)
 *
 *  Do device-specific I/O control operations for the device
 *
 *  Input parameters:
 *     ctx - device context (includes ptr to our softc)
 *      buffer - pointer to buffer descriptor.
 *
 *  Return value:
 *     status, 0 = ok
 * ********************************************************************
 */

static int
wl_ioctl(cfe_devctx_t *ctx, iocb_buffer_t *buffer)
{
	wl_info_t *wl = ctx->dev_softc;

	WL_TRACE(("wl%d: wl_ioctl: cmd 0x%x\n", wl->pub->unit, buffer->buf_ioctlcmd));

	wl_poll(ctx, 0);

	switch (buffer->buf_ioctlcmd) {

	case IOCTL_ETHER_GETHWADDR:
		bcopy(&wl->pub->cur_etheraddr, buffer->buf_ptr, ETHER_ADDR_LEN);
		break;

	case IOCTL_ETHER_SETHWADDR:
		if (wl->pub->up)
			return CFE_ERR_DEVOPEN;
		bcopy(buffer->buf_ptr, &wl->pub->cur_etheraddr, ETHER_ADDR_LEN);
		break;

	case IOCTL_ETHER_GETLINK:
		*((int *) buffer->buf_ptr) = (int) wl->link;
		break;

	default:
		return CFE_ERR_UNSUPPORTED;

	}

	return 0;
}

/*  *********************************************************************
 *  WL_CLOSE(ctx)
 *
 *  Close the wireless device.
 *
 *  Input parameters:
 *     ctx - device context (includes ptr to our softc)
 *
 *  Return value:
 *     status, 0 = ok
 * ********************************************************************
 */

static int
wl_close(cfe_devctx_t *ctx)
{
	wl_info_t *wl = ctx->dev_softc;
	void *p;

	WL_TRACE(("wl%d: wl_close\n", wl->pub->unit));

	wl_down(wl);

	/* Free queued packets */
	while ((p = pktdeq(&wl->rxq)))
		PKTFREE(wl->osh, p, FALSE);

	return 0;
}

static const cfe_devdisp_t wl_devdisp = {
	wl_open,
	wl_read,
	wl_inpstat,
	wl_write,
	wl_ioctl,
	wl_close,
	wl_poll,
	NULL
};

const cfe_driver_t bcmwl = {
	"Broadcom 802.11",
	"eth",
	CFE_DEV_NETWORK,
	&wl_devdisp,
	wl_probe
};

#if CFG_WLU
#include <wlu.h>

int
wl_get(void *wlc, int cmd, void *buf, int len)
{
	return wlc_ioctl(wlc, cmd, buf, len, NULL);
}

int
wl_set(void *wlc, int cmd, void *buf, int len)
{
	return wlc_ioctl(wlc, cmd, buf, len, NULL);
}

static int
ui_cmd_wl(ui_cmdline_t *cmdline, int argc, char *argv[])
{
	char *name;
	wl_info_t *wl;
	cfe_device_t *dev;
	cmd_t *cmd;
	int ret = 0;

	if (!(cmd_getarg(cmdline, 0))) {
		wl_usage(NULL);
		return CFE_ERR_INV_PARAM;
	}

	if (!cmd_sw_value(cmdline, "-i", &name) || !name)
		name = "eth0";
	if (!(dev = cfe_finddev(name)) || !dev->softc)
		return CFE_ERR_DEVNOTFOUND;
	for (wl = wl_list; wl; wl = wl->next)
		if (wl == dev->softc)
			break;
	if (!wl && !(wl = wl_list))
		return CFE_ERR_DEVNOTFOUND;

	ASSERT(wl_check(wl->wlc) == 0);

	/* search for command */
	cmd = wlu_find_cmd(*argv);

	/* defaults to using the set_var and get_var commands */
	if (cmd == NULL)
		cmd = &wl_varcmd;

	/* do command */
	if (cmd->name)
		ret = (*cmd->func)(wl->wlc, cmd, argv);

	if (!cmd->name)
		wl_usage(NULL);
	else if (ret)
		wl_cmd_usage(cmd);

	return ret;
}

void
wl_addcmd(void)
{
	cmd_addcmd("wl", ui_cmd_wl, NULL, "Broadcom 802.11 utility.", "wl command [args..]\n\n"
	"Configures the specified Broadcom 802.11 interface.", "-i=*;Specifies the interface|");
}
#endif /* CFG_WLU */
