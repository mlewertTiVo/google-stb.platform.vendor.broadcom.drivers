/*
 * FreeBSD-specific portion of
 * Broadcom 802.11abgn Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wl_fbsd.c 467328 2014-04-03 01:23:40Z $
 */

/* OS headers */
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/conf.h>
#include <machine/bus.h>
#include <sys/bus.h>
#include <sys/module.h>
#include <sys/rman.h>
#include <machine/resource.h>

#include <sys/priv.h>

#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <net/if_llc.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_regdomain.h>


/* Broadcom headers */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <epivers.h>
#include <bcmendian.h>
#include <bcmdevs.h>
#include <bcmutils.h>
#include <pcicfg.h>
#include <wlioctl.h>
#include <bcmbsd.h>

typedef void wlc_info_t;
typedef const struct si_pub si_t;
extern int wlc_bmac_up_prep(wlc_info_t *wlc);
extern int wlc_bmac_up_finish(wlc_info_t *wlc);
extern void wlc_bmac_hw_etheraddr(wlc_info_t *wlc, struct ether_addr *ea);



#include "wlc_pub.h"





#include "wl_fbsd.h"
#include "wl_dbg.h"

#include <pcicfg.h>
#include <wl_export.h>


#if defined(__FreeBSD__)
#define NAME_UNIT		device_get_nameunit(sc->dev)
#endif /* defined OS */


#if defined(__FreeBSD__)
/* JFH Maybe we should be using mtx_trylock ??? */
/* JFH do these need to be recursive??? */
#define WL_SOFTLOCK_INIT	mtx_init(&(wl)->soft_mtx, device_get_nameunit(wl->sc->dev), \
									      NULL, MTX_DEF);
#define WL_HARDLOCK_INIT	mtx_init(&(wl)->hard_mtx, device_get_nameunit(wl->sc->dev), \
									      NULL, MTX_DEF);
#define WL_SOFTLOCK(wl, ipl)	mtx_lock(&(wl)->soft_mtx); (void) ipl;
#define WL_SOFTUNLOCK(wl, ipl)	mtx_unlock (&(wl)->soft_mtx); (void) ipl;
#define WL_HARDLOCK(wl, ipl)	mtx_lock(&(wl)->hard_mtx); (void) ipl;
#define WL_HARDUNLOCK(wl, ipl)	mtx_unlock (&(wl)->hard_mtx); (void) ipl;
#endif /* defined (__FreeBSD__) */

/* Macros to pick various bits out  */
#define WL_INFO_GET(ifp)		((wl_if_t *)(ifp)->if_softc)->wl
#define WLIF_IFP(ifp)		((wl_if_t *)(ifp)->if_softc)
#define IFP_WLIF(wlif)		&(wlif)->ec.ec_if

/* Device interface names */
#define WL_DEVICENAME		"wl"
#define WL_WDS_IFNAME		"wds"
#define WL_BSS_IFNAME		WL_DEVICENAME

/* See if the interface is in AP mode */
#define WLIF_AP_MODE(wl, wlif) (wl_wlif_apmode((wl), (wlif)))

/* Timer Stuff - Start */

/* hz specifies the number of times the hardclock(9)
 * timer ticks per second. Defined in <sys/kernel.h>
 * This macro sets the number of timer ticks
 * per millisecond.
 */
#define TICKS_PER_MSEC	hz/1000

/* Timer control information */
typedef struct wl_timer {
	uint			magic;		/* Timer magic cookie */
	struct callout 		timer;		/* Timer dpc info */
	struct wl_info 		*wl;		/* back pointer to main wl_info_t */
	void 			(*fn)(void *);	/* dpc executes this function */
	void*			arg;		/* fn is called with this arg */
	uint 			ms;		/* Time in milliseconds */
	bool 			periodic;	/* Recurring timer if set */
	bool 			set;		/* Timer is armed if set */
	struct wl_timer		*next;		/* Link to next timer object */
} wl_timer_t;

/* Timer Stuff - End */


/* Interface control information */
struct wl_if {
	uint			magic;		/* Interface Magic cookie */
	struct wl_info 		*wl;		/* back pointer to main wl_info_t */
	struct wl_if 		*next;		/* Next interface in the list */

	int 			type;		/* interface type: WDS, BSS */
	int 			vapidx;		/* for VAPs, vapidx <= MAX_BSSCFG => bssindex */
	struct wlc_if 		*wlcif;		/* wlc interface handle */
	struct ether_addr 	remote;		/* remote WDS partner */
	uint 			subunit;		/* WDS/BSS unit */
	bool			attached;		/* Delayed attachment when unset */
	bool			txflowcontrol;	/* Per interface flow control indicator */
};




/* Packet exchange routines */
/*
 * Those are needed by the ifnet interface, and would typically be
 * there for any network interface driver.
 * Some other routines are optional: watchdog and drain.
 */

static void	wl_start(struct ifnet *);				/* Packet TX routine */
static void	wl_open(void *);				/* Device start routine */
static int	wl_ioctl(struct ifnet *, u_long, caddr_t);		/* BSD system Ioctls */


static int wl_getchannels(struct wl_softc *sc);
static void getchannels(struct wl_softc *sc, int *nchans, struct ieee80211_channel chans[]);


#define WL_IFTYPE_BSS	1 /* iftype subunit for BSS */
#define WL_IFTYPE_WDS	2 /* iftype subunit for WDS */

#define WLIF_WDS(wlif) (wlif->type == WL_IFTYPE_WDS) /* Is this i/f a WDS node */


int
wl_attach(struct wl_softc* sc)
{
	wl_info_t *wl;
	uint8 *vaddr;
	uint err;
	struct ifnet *ifp;
	struct ieee80211com *ic;
	int error = 0;

	WL_ERROR(("%s: start\n", __FUNCTION__));

	ifp = sc->ifp = if_alloc(IFT_IEEE80211);
	if (ifp == NULL) {
		device_printf(sc->dev, "can not if_alloc()\n");
		error = ENOSPC;
		goto bad;
	}
	ic = ifp->if_l2com;

	/* set these up early for if_printf use */
	if_initname(ifp, device_get_name(sc->dev),
		device_get_unit(sc->dev));

	sc->magic = 0;

	/* Allocate memory for wl structure */
	wl = malloc(sizeof(wl_info_t), M_DEVBUF, M_WAITOK|M_ZERO);
	if (wl == NULL) {
		WL_ERROR(("%s: %s failed to allocate %d bytes for wl structure\n",
			NAME_UNIT, EPI_VERSION_STR, sizeof(wl_info_t)));
		return 1;
	};

	vaddr = sc->va;
	wl->osh = osl_attach(sc->dev, NAME_UNIT, sc->st, sc->sh, vaddr);

	/* common load-time initialization */
	sc->wl = wl;
	sc->wl->sc = sc; /* Back pointer to our device control block */
	wl->bustype = PCI_BUS;
	wl->piomode = FALSE;
	wl->magic = WLINFO_MAGIC;
	wl->pub = NULL;

	/* Init the perimeter locks. One for hardware interrupt context, the other for software */
	WL_HARDLOCK_INIT;
	WL_SOFTLOCK_INIT;

	/* Attach the OS independent bits */
	if (!(wl->pub = wlc_attach((void *)wl, pci_get_vendor(sc->dev), pci_get_device(sc->dev),
	                           device_get_unit(sc->dev), wl->piomode, wl->osh,
	                           vaddr, wl->bustype, NULL, NULL, &err))) {
		WL_ERROR(("%s: %s Driver failed with code %d\n",
		          NAME_UNIT, EPI_VERSION_STR,  err));
		goto bad1;
	}


	wl->base_if = wl->iflist;

	ifp->if_softc = wl;
	ifp->if_flags = IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST;
	ifp->if_start = wl_start;
	//ifp->if_watchdog = wl_watchdog; /* what's this for? */
	ifp->if_ioctl = wl_ioctl;
	ifp->if_init = wl_open;
	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);
	ifp->if_snd.ifq_drv_maxlen = IFQ_MAXLEN;
	IFQ_SET_READY(&ifp->if_snd);

	ic->ic_ifp = ifp;
	ic->ic_phytype = IEEE80211_T_OFDM;
	ic->ic_opmode = IEEE80211_M_STA;
	ic->ic_caps =
		  IEEE80211_C_IBSS		/* ibss, nee adhoc, mode */
		| IEEE80211_C_HOSTAP		/* hostap mode */
		| IEEE80211_C_MONITOR		/* monitor mode */
		| IEEE80211_C_AHDEMO		/* adhoc demo mode */
		| IEEE80211_C_WDS		/* 4-address traffic works */
		| IEEE80211_C_SHPREAMBLE	/* short preamble supported */
		| IEEE80211_C_SHSLOT		/* short slot time supported */
		| IEEE80211_C_WPA		/* capable of WPA1+WPA2 */
		| IEEE80211_C_BGSCAN		/* capable of bg scanning */
		| IEEE80211_C_TXFRAG		/* handle tx frags */
		;

	/* Collect the default channel list. */
	error = wl_getchannels(sc);
	if (error != 0)
		goto bad;

	/*
	 * Indicate we need the 802.11 header padded to a
	 * 32-bit boundary for 4-address and QoS frames.
	 */
	ic->ic_flags |= IEEE80211_F_DATAPAD;

	/* get mac address from hardware */
	wlc_bmac_hw_etheraddr(wl->pub, (struct ether_addr *)ic->ic_myaddr);


	ieee80211_ifattach(ic);
	ieee80211_announce(ic);

	sc->magic = SC_MAGIC;

	WL_ERROR(("%s: end\n", __FUNCTION__));

	return 0;

bad1:
	osl_detach(sc->wl->osh);
	sc->wl->osh = NULL;
bad:
	/* probably should revisit this later to make sure we shouldn't clean up more... */
	if (ifp != NULL)
		if_free(ifp);

	return err;
}


int
wl_detach(struct wl_softc *sc)
{
	wl_info_t *wl;
	wl_timer_t *t, *next;
	int ipl;
	struct ifnet *ifp = sc->ifp;

	WL_ERROR(("%s: start\n", __FUNCTION__));

	/* This prevents the device from being detached more than once */
	if (sc->magic != SC_MAGIC) {
		WL_ERROR(("%s: wrong sc magic of %d when %d was expected\n", __FUNCTION__,
		          sc->magic, SC_MAGIC));
		return 0;
	}


	ieee80211_ifdetach(ifp->if_l2com);

	wl = sc->wl;

	ipl = splnet();

	splx(ipl);

	/* Detach wl */
	if (wl->pub) {
		wl->callbacks = wlc_detach((void *)wl->pub);
		wl->pub = NULL;


		/* free timers */
		WL_SOFTLOCK(wl, ipl);
		for (t = wl->timers; t; t = next) {
			next = t->next;
			wl_del_timer(wl, t);
			MFREE(wl->osh, t, sizeof(wl_timer_t));
		}
		WL_SOFTUNLOCK(wl, ipl);
	}

	wl->magic = 0;


	/* Deallocate wl structure */
	free(sc->wl, M_DEVBUF);
	sc->wl = NULL;

	sc->magic = 0;

	WL_ERROR(("%s: end\n", __FUNCTION__));
	return TRUE;
}



void
wl_isr(void *arg)
{
	WL_ERROR(("%s: start\n", __FUNCTION__));
}

void
wl_add_timer(wl_info_t *wl, struct wl_timer *t, uint ms, int periodic)
{
	WL_ERROR(("%s: start\n", __FUNCTION__));
}

/* Must be called from within perimeter lock */
bool
wl_del_timer(wl_info_t *wl, struct wl_timer *t)
{
	WL_ERROR(("%s: start\n", __FUNCTION__));
	return FALSE;
}


void
wl_free_timer(wl_info_t *wl, struct wl_timer *t)
{
	WL_ERROR(("%s: start\n", __FUNCTION__));
}


struct wl_timer *
wl_init_timer(wl_info_t *wl, void (*fn)(void *arg), void* arg, const char *name)
{
	wl_timer_t *t;

	ASSERT(wl->magic == WLINFO_MAGIC);

	if (!(t = MALLOC(wl->osh, sizeof(wl_timer_t)))) {
		WL_ERROR(("wl%d: wl_init_timer: out of memory, malloced %d bytes\n",
			wl->pub->unit, MALLOCED(wl->osh)));
		return 0;
	}

	bzero(t, sizeof(wl_timer_t));

	t->wl = wl;
	t->fn = fn;
	t->arg = arg;
	t->magic = WLTIMER_MAGIC;
	t->next = wl->timers;
	wl->timers = t;


#if defined(__FreeBSD__)
	callout_init(&t->timer, CALLOUT_MPSAFE);
#endif /* defined (OS) */

	return t;
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
	ASSERT(wl->magic == WLINFO_MAGIC);
	wlc_intrson((void *)wl->pub);
}

uint32
wl_intrsoff(wl_info_t *wl)
{
	ASSERT(wl->magic == WLINFO_MAGIC);
	return wlc_intrsoff((void *)wl->pub);
}

void
wl_intrsrestore(wl_info_t *wl, uint32 macintmask)
{
	ASSERT(wl->magic == WLINFO_MAGIC);
	wlc_intrsrestore((void *)wl->pub, macintmask);
}


uint
wl_reset(wl_info_t *wl)
{
	WL_TRACE(("wl%d: wl_reset\n", wl->pub->unit));

	ASSERT(wl->magic == WLINFO_MAGIC);

	wlc_reset((void *)wl->pub);

	return (0);
}

void
wl_init(wl_info_t *wl)
{
	WL_TRACE(("wl%d: wl_init\n", wl->pub->unit));

	ASSERT(wl->magic == WLINFO_MAGIC);

	wl_reset(wl);

	printf("%s,  wlc_init is not implemented yet, otherwise we'd be calling it here...\n",
	       __FUNCTION__);
}


/* transmit a packet */
static void
wl_start(struct ifnet *ifp)
{
	wl_info_t *wl;
	wl_if_t *wlif;

	if ((ifp->if_flags & (IFF_DRV_RUNNING|IFF_UP)) == 0)
		return;

	wlif = WLIF_IFP(ifp);
	ASSERT(wlif->magic == WLIF_MAGIC);

	wl = wlif->wl;
	ASSERT(wl->magic == WLINFO_MAGIC);

	WL_ERROR(("%s:  To be finished...\n", __FUNCTION__));

	return;
}

/*
 * Note that both ifmedia_ioctl() and ether_ioctl() have to be
 * called under splnet().
 */
static int
wl_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	wl_if_t *wlif;
	int ipl;
	int error = 0;
	wl_info_t *wl;

	wlif = WLIF_IFP(ifp);

	/* Check to see if we got an ioctl not meant for us */
	if (!wlif)
		return EINVAL;

	if (wlif->magic != WLIF_MAGIC)
		return EINVAL;

	wl = wlif->wl;

	WL_TRACE(("%s: wl_ioctl: cmd 0x%x\n", ifp->if_xname, (uint)cmd));

	ipl = splnet();

	switch (cmd) {
	default:
		error = ether_ioctl(ifp, cmd, data);
	} /* end of switch */

	splx(ipl);

	return (error);
}


/*
 * wl_open() called by ifconfig() when the interface goes up,
 */
static void
wl_open(void *arg)
{
	wl_info_t *wl = (wl_info_t *)arg;
	struct ifnet *ifp = wl->sc->ifp;
	int error;
	int ipl = 0;

	ASSERT(wl->magic == WLINFO_MAGIC);

	WL_TRACE(("wl%d: wl_open\n", wl->pub->unit));

	WL_HARDLOCK(wl, ipl);

	error = wl_up(wl);

	if (!error) {
		int val = ifp->if_flags & IFF_PROMISC;
		error = wlc_ioctl(wl->wlc, WLC_SET_PROMISC,
		                  &val, sizeof(int), NULL);
	}
	WL_HARDUNLOCK(wl, ipl);

	return;
}

int
wl_up(wl_info_t *wl)
{
	int error = 0;

	WL_TRACE(("wl%d: wl_up\n", wl->pub->unit));

	ASSERT(wl->magic == WLINFO_MAGIC);

	if (wl->pub->up)
		return (0);

	WL_ERROR(("%s: calling wlc_bmac_up_prep", __FUNCTION__));
	if (!(error = wlc_bmac_up_prep((void *)wl->pub))) {
		WL_ERROR(("%s: calling wlc_bmac_up_finish", __FUNCTION__));
		error = wlc_bmac_up_prep((void *)wl->pub);
	}


	return (error);
}


static int
wl_getchannels(struct wl_softc *sc)
{
	struct ifnet *ifp = sc->ifp;
	struct ieee80211com *ic = ifp->if_l2com;

	/*
	 * Use the channel info from the hal to craft the
	 * channel list for net80211.  Note that we pass up
	 * an unsorted list; net80211 will sort it for us.
	 */
	memset(ic->ic_channels, 0, sizeof(ic->ic_channels));
	ic->ic_nchans = 0;
	/* Convert WL channels to ieee80211 ones. */
	getchannels(sc, &ic->ic_nchans, ic->ic_channels);

	ic->ic_regdomain.regdomain = SKU_DEBUG;
	ic->ic_regdomain.country = CTRY_DEFAULT;
	ic->ic_regdomain.location = 'I';
	ic->ic_regdomain.isocc[0] = ' ';
	ic->ic_regdomain.isocc[1] = ' ';
	return (ic->ic_nchans == 0 ? EIO : 0);


}


static void
getchannels(struct wl_softc *sc, int *nchans, struct ieee80211_channel chans[])
{

	*nchans = 1;

	chans[0].ic_ieee = 1; /* ieee channel */
	chans[0].ic_freq = 2412;
	chans[0].ic_flags = IEEE80211_CHAN_B;
	chans[0].ic_minpower = 0;
	chans[0].ic_maxpower = 1;
	chans[0].ic_maxregpower = 1;

}
