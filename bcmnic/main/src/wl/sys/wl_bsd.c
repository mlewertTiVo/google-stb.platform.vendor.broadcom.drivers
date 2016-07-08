/* NetBSD-specific portion of
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
 * wl_bsd can be NetBSD Loadable Kernel Module in 2.0 or
 * compiled into the kernel for all kernel versions
 *
 * $Id: wl_bsd.c 467328 2014-04-03 01:23:40Z $
 *
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* -
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <sys/cdefs.h>
#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/errno.h>
#include <sys/conf.h>
#ifdef __HAVE_NEW_STYLE_BUS_H
#include <sys/bus.h>
#else
#include <machine/bus.h>
#endif
#include <net/if.h>
#include <net/bpf.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/route.h>
#if defined(_KERNEL) && !defined(_LKM)
#include "bpfilter.h" /* Needed for NBPFILTER */
#endif
#include <net/if_llc.h>
#include <net/if_arp.h>

#if __NetBSD_Version__ > 106200000
#include <sys/ksyms.h>
#endif /* __NetBSD_Version__ > 106200000 */


#include <sys/sockio.h>
#include <sys/lock.h>
#include <sys/callout.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif /* INET */

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#ifdef _LKM
#include <pcilkm/pcilkm.h>
#endif


/*
 *  Define this to prevent the inclusion of the
 *  ethernet header defines
 *  Undoes a VxWorks hack in proto/ethernet.h
 */
#undef	ETHER_HDR_LEN
#define  __INCif_etherh

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <epivers.h>
#include <bcmendian.h>
#include <bcmnvram.h>
#include <bcmdevs.h>
#include <bcmutils.h>
#include <siutils.h>
#include <pcicfg.h>
#include <wlioctl.h>
#include <bcmbsd.h>
#include <bcmwpa.h>
#include <proto/eapol.h>
#include <proto/802.1d.h>
#include <sbhndpio.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc_channel.h>
#include <wlc.h>
#include <wlc_pub.h>
#include <wl_dbg.h>
#include <wlc_scb.h>
#ifdef WL_MONITOR
#include <wlc_ethereal.h>
#endif
#include <wlc_ht.h>

#include <wl_export.h>

#undef WME_OUI
#undef WPA_OUI
#undef RSN_CAP_PREAUTH
#undef WME_OUI_TYPE
#undef WPS_OUI_TYPE
#undef WPA_OUI_TYPE


#include <net80211/ieee80211.h>
#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_ioctl.h>
#include <net80211/ieee80211_regdomain.h>

#if APPLE_DEFFERED_ATTACH
#include <sys/static_config.h>
#endif

#ifdef __mips__
#define WLSOFTIRQ 1
#endif

/* Magic cookies for the various structure */
#define SC_MAGIC 	OS_HANDLE_MAGIC + 1
#define WLINFO_MAGIC	SC_MAGIC + 1
#define WLIF_MAGIC	SC_MAGIC + 2
#define WLTIMER_MAGIC	SC_MAGIC + 3
#define PCI_MAGIC	SC_MAGIC + 4
#define WLTASK_MAGIC	SC_MAGIC + 5

#define WL_IFTYPE_BSS	1 /* iftype subunit for BSS */
#define WL_IFTYPE_WDS	2 /* iftype subunit for WDS */

#define WLIF_WDS(wlif) (wlif->type == WL_IFTYPE_WDS) /* Is this i/f a WDS node */

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

/* Interface control information */
struct wl_if {
	uint			magic;		/* Interface Magic cookie */
	struct wl_info		*wl;		/* back pointer to main wl_info_t */
	struct wl_if		*next;		/* Next interface in the list */
	struct ethercom		ec; 		/* NetDevice data structure */
	struct ifmedia		im;		/* Physical media. Not needed ?? */
	struct bpf_if		*rawbpf; 	/* bpf for sending raw 802.11 frames */
	int			type;		/* interface type: WDS, BSS */
	int			vapidx;		/* for VAPs, vapidx <= MAX_BSSCFG => bssindex */
	struct wlc_if		*wlcif;		/* wlc interface handle */
	struct ether_addr	remote;		/* remote WDS partner */
	uint			subunit;	/* WDS/BSS unit */
	bool			attached;	/* Delayed attachment when unset */
	bool			txflowcontrol;	/* Per interface flow control indicator */
	bool			isDWDS;
	/* IEEE80211_IOC_APPIE */
	vndr_ie_setbuf_t	*appie_beacon;
	size_t			appie_beacon_len;
	vndr_ie_setbuf_t	*appie_probereq;
	size_t			appie_probereq_len;
	vndr_ie_setbuf_t	*appie_proberesp;
	size_t			appie_proberesp_len;
	vndr_ie_setbuf_t	*appie_assocreq;
	size_t			appie_assocreq_len;
	vndr_ie_setbuf_t	*appie_assocresp;
	size_t			appie_assocresp_len;
	/* vndr_ie_setbuf_t	*appie_wpa; */

	/* IEEE80211_IOC_APPIEBUF */
	vndr_ie_setbuf_t	*appie_beacon_wps;
	size_t			appie_beacon_wps_len;
	vndr_ie_setbuf_t	*appie_probereq_wps;
	size_t			appie_probereq_wps_len;
	vndr_ie_setbuf_t	*appie_proberesp_wps;
	size_t			appie_proberesp_wps_len;
	vndr_ie_setbuf_t	*appie_beacon_dwds;
	size_t			appie_beacon_dwds_len;
	vndr_ie_setbuf_t	*appie_proberesp_dwds;
	size_t			appie_proberesp_dwds_len;
	vndr_ie_setbuf_t	*appie_assocreq_dwds;
	size_t			appie_assocreq_dwds_len;
	vndr_ie_setbuf_t	*appie_assocresp_dwds;
	size_t			appie_assocresp_dwds_len;
	vndr_ie_setbuf_t	*appie_beacon_awp;
	size_t			appie_beacon_awp_len;
	vndr_ie_setbuf_t	*appie_probereq_awp;
	size_t			appie_probereq_awp_len;
	vndr_ie_setbuf_t	*appie_proberesp_awp;
	size_t	appie_proberesp_awp_len;
};


struct _wl_lock {
	int count;
	int ipl;
};
typedef struct _wl_lock wlock_t;

#define WL_MAXWDSVAP	5 /* Additional 5 WDS peers */
#define WL_MAXVAP	WLC_MAXBSSCFG + WL_MAXWDSVAP
/* Logical structure for VAP */
typedef struct {
	wl_if_t			*wlif;		/* Pointer to lower edge i/f */
	char			*vap_name;	/* Name of the VAP handle */
} wl_vap_t;

/* sta ie list structure */
typedef struct sta_ie_list {
	SLIST_ENTRY(sta_ie_list) entries;
	struct ether_addr addr;
	bcm_tlv_t *tlv;
	uint32 tlvlen;
} sta_ie_list_t;

/* Device control information */
typedef struct wl_info {
	int			magic;		/* magic number */
	wlc_info_t		*wlc;		/* pointer to wlc state */
	wlc_pub_t		*pub;		/* pointer to public wlc state */
	osl_t			*osh;		/* pointer to os handler */
	struct wl_softc		*sc;		/* backpoint to device */
	wlock_t                 mtx;
	uint			bustype;	/* bus type */
	bool			piomode;	/* set from insmod argument */
	volatile bool		txflowcontrol;	/* Flowthrottle indication */
	void			*dpc;		/* DPC handle for main ISR */
	void			*txdequeue;	/* DPC handle for TX dequeue */
	volatile bool		dqwait;		/* Pending TX dequeue flag */
	wl_if_t 		*iflist;	/* list of all interfaces */
	wl_if_t 		*base_if;	/* For convenience, points to Base i/f */
	volatile int		num_if;		/* Number of interfaces */
	wl_vap_t		vap[WL_MAXVAP];	/* VAPs */
	int			wdsvapidx;	/* Index of current wds vap being created */
	int			num_vap;	/* # of VAPs */
	volatile uint 		callbacks;	/* # outstanding callback functions */


	struct wl_timer		*timers;	/* timer cleanup queue */
	struct ifnet		*monitor;	/* monitor pseudo device */
	bool			resched;	/* dpc needs to be and is rescheduled */
	struct ieee80211_txparams_req	*txparams;
	bool			scandone;
	struct ieee80211_channel chans[IEEE80211_CHAN_MAX];
	uint 			nchans;
	uint16_t 		modecaps;
	SLIST_HEAD(sta_ie_list_head, sta_ie_list) assoc_sta_ie_list;
} wl_info_t;

/* Deferred task control information */
typedef struct wl_task {
	uint			magic;			/* task control magic cookie */
	void 			*work;			/* dpc handle */
	void 			*context;		/* Arguments to fn() below */
	void 			(*fn)(void *task);	/* Function to execute */
	wl_info_t		*wl;			/* Back pinter to wl */
} wl_task_t;

/* OS PCI bus control information */
struct wl_pci_softc {
	uint			magic;	/* PCI control info magic cookie */
	struct pci_attach_args	pa;		/* PCI attach args */
	bus_space_tag_t 	iot;	/* OS Tag for pci BAR map */
	bus_space_handle_t 	ioh;	/* OS handle pci BAR map */
	pci_intr_handle_t	ih_t;	/* OS PCI interrupt handle */
	int			ipl;	/* Device interrupt priority level */
	void			*isr;	/* Device interrupt service routine */
	void			*isr_arg;	/* Argument for isr */
	void 			*ih;	/* This handle is used to disestablish interrupts */
	int 			regbase;	/* Start of the Base Address Register */
	pcireg_t		maptype;	/* Type of PCI mapping */
	bus_size_t		mapsize;	/* Size of mapped window */
};

/* OS device control block */
struct wl_softc {
	struct device		dev;		/* Device control info must be first enrty */
	uint 			magic;	/* Magic cookie */
	wl_info_t 		*wl;		/* The wl structure */
	void 			*sdhook;	/* shutdown hook */
	struct wl_pci_softc	pci;		/* OS PCI bus control data */
};

#if APPLE_DEFFERED_ATTACH
#define APPLE_SKU_BUF_SZ 32
struct sku_cspec_map_entry {
	char			sku[APPLE_SKU_BUF_SZ];
	wl_country_t	cspec;
};

struct sku_cspec_map_entry sku_cspec_map[] = {
	{{"FCC"}, {"Q1", 48, "Q1"}}, 			/* fcc dfs chans */
/*	{{"FCC"}, {"Q2", 34, "Q2"}}, */			/* fcc no dfs chans */
	{{"JAPAN"}, {"JP", 46, "JP"}},
	{{"ETSI"}, {"EU", 45, "EU"}},
	{{"APAC"}, {"X1", 14, "X1"}},
	{{"ROW"}, {"X2", 18, "X2"}},
	{{"RUSSIA"}, {"RU", 33, "RU"}},
	{{"SAM"}, {"BR", 9, "BR"}}, 			/* latam dfs chans - placeholder */
/*	{{"SAM"}, {"BR", 10, "BR"}}, */			/* latam no dfs chans - placeholder */
};

#endif /* APPLE_DEFFERED_ATTACH */

#define AUTH_INCLUDES_UNSPEC(_wpa_auth) ((_wpa_auth) & WPA_AUTH_UNSPECIFIED || \
	(_wpa_auth) & WPA2_AUTH_UNSPECIFIED)
#define AUTH_INCLUDES_PSK(_wpa_auth) ((_wpa_auth) & WPA_AUTH_PSK || \
	(_wpa_auth) & WPA2_AUTH_PSK)


/* Locking Primitives */


#define WL_HARDINTR		IPL_NET
#define WL_SOFTINTR		IPL_SOFTCLOCK
#define	_WL_LOCK_INIT_IMPL(_lock, _name) do {	\
	(_lock)->count = 0;			\
} while (0)
#define	_WL_LOCK_IMPL(_lock) do {		\
	int __s = splnet();			\
	if ((_lock)->count++ == 0)		\
		(_lock)->ipl = __s;		\
} while (0)
#define _WL_IS_LOCKED_IMPL(_lock)		\
	((_lock)->count != 0)
#define	_WL_UNLOCK_IMPL(_lock) do {		\
	ASSERT((_lock)->count > 0);		\
	if (--(_lock)->count == 0)		\
		splx((_lock)->ipl);		\
} while (0)
#define	_WL_LOCK_ASSERT_IMPL(_lock)		ASSERT((_lock)->count > 0)


#define WL_LOCK_INIT(wl, name)	_WL_LOCK_INIT_IMPL(&(wl)->mtx, name)
#define WL_LOCK_DESTROY(wl)
#define WL_LOCK(wl)		_WL_LOCK_IMPL(&(wl)->mtx)
#define WL_UNLOCK(wl)		_WL_UNLOCK_IMPL (&(wl)->mtx)
#define WL_LOCK_ASSERT(wl)	_WL_LOCK_ASSERT_IMPL(&(wl)->mtx)
#define WL_SOFTLOCK(wl, ipl)     WL_LOCK(wl)
#define WL_HARDLOCK(wl, ipl)     WL_LOCK(wl)
#define WL_SOFTUNLOCK(wl, ipl)     WL_UNLOCK(wl)
#define WL_HARDUNLOCK(wl, ipl)     WL_UNLOCK(wl)

/* Macros to pick various bits out  */
#define WL_INFO_GET(ifp)		((wl_if_t *)(ifp)->if_softc)->wl
#define WLIF_IFP(ifp)		((wl_if_t *)(ifp)->if_softc)
#define IFP_WLIF(wlif)		&(wlif)->ec.ec_if

/* Starts and stops network packet queue */
#define WL_STOP_IFQUEUE(ifp, s)\
	do {(s) = splnet(); (ifp)->if_flags |= IFF_OACTIVE; splx((s));} while (0)
#define WL_WAKE_IFQUEUE(ifp, s)\
	do {(s) = splnet(); (ifp)->if_flags &= ~IFF_OACTIVE; splx((s));} while (0)

/* Device interface names */
#define WL_DEVICENAME		"bwl"
#define WL_WDS_IFNAME		"wds"
#define WL_BSS_IFNAME		WL_DEVICENAME

/* See if the interface is in AP mode */
#define WLIF_AP_MODE(wl, wlif) (wl_wlif_apmode((wl), (wlif)))

/*
 * Those are needed by the if_media interface.
 */

static int	wl_mediachange(struct ifnet *);
static void	wl_mediastatus(struct ifnet *, struct ifmediareq *);

/* Packet exchange routines */
/*
 * Those are needed by the ifnet interface, and would typically be
 * there for any network interface driver.
 * Some other routines are optional: watchdog and drain.
 */

static void	wl_start(struct ifnet *);				/* Packet TX routine */
#if __NetBSD_Version__ >= 500000003
static int wl_initaddr(struct ifnet *ifp, struct ifaddr *ifaddr, bool src);
#endif
#if WLSOFTIRQ
static void	wl_input(struct ifnet *);				/* OS TX routine */
#endif
static void	wl_close(struct ifnet *, int);				/* Device stop routine */
static int	wl_open(struct ifnet *);				/* Device start routine */
#if __NetBSD_Version__ >= 500000003
static int	wl_ioctl(struct ifnet *, u_long, void *);		/* BSD system Ioctls */
#else
static int	wl_ioctl(struct ifnet *, u_long, caddr_t);
#endif
void		wl_sendup(wl_info_t *wl, wl_if_t *wlif, void *p, int numpkt);	/* RX routine */

#if defined(__mips__)
/* Sets the mac address */
static int 	wl_set_mac_address(struct ifnet *ifp, u_long cmd, struct ifaliasreq *ifra);
#endif /* __mips__ */
/* wl specific routines */

/* ISR and related routines */
static void 	wl_dpc(void *data);		/* Deferred Procedure Call for ISR */
int 		wl_isr(void *isr_arg);		/* Interrupt Service Routine */
static void 	wl_sched_txdequeue(wl_info_t *wl);	/* Initiates TX dequeueu callback */
static void 	wl_txdequeue(void *arg);	/* Call back to dequeue more TX packets */
void 		wl_intrson(wl_info_t *wl);	/* Per port interface to turn interrupts on */
uint32 		wl_intrsoff(wl_info_t *wl);	/* Per port interface to turn interrupts off */
void 		wl_intrsrestore(wl_info_t *wl, uint32 macintmask);

/* WL's private ioctl handlers */
static int 	wl_bcm_ioctl(struct ifnet *ifp, struct ifreq *ifr);
static int 	wl_wlc_ioctl(struct ifnet *ifp, int cmd, void *arg, int len);
static int 	wl_iovar_op(wl_info_t *wl, const char *name, void *params, int p_len, void *arg,
                            int len, bool set, struct wl_if *wlif, struct wlc_if *wlcif);

/* Maps PCI regs and ISR */
static int 	wl_pci_mapdevice(struct wl_pci_softc *pci, char *devname);
/* Plumbing and  enable the ISR */
static int 	wl_pci_plumbintr(struct wl_pci_softc *sc, char *devname);

/* WL state change */
static void 	wl_link_up(wl_info_t *wl);
static void 	wl_link_down(wl_info_t *wl);

/* Clean up/init code */
static void 	wl_shutdown(void *arg);		/* Unplumbs wl on unload or shutdown  */

/* wait for all pending callbacks to complete */
static void 	wl_wait_callbacks(wl_info_t *wl);

/* Utility functions */
static void 	wl_mic_error(wl_info_t *wl, wlc_bsscfg_t *cfg, struct ether_addr *ea,
                             bool group, bool flush_txq);


/* Timer and scheduling functions */
static void 	wl_timer(void *data);

/* WL Interface processing code */
#if __NetBSD_Version__ >= 500000003
static int 	wl_wds_ioctl(struct ifnet *ifp, u_long cmd, void * data);
#else
static int 	wl_wds_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data);
#endif
static void 	_wl_add_if(void *context); /* Create an interface. Call only from safe time! */

/* Schedule _wl_add_wds() to be run at safe time. */
wl_if_t *	wl_add_if(wl_info_t *wl,
                          struct wlc_if* wlcif, uint unit, struct ether_addr *remote);

/* Remove a WDS interface. Call only from safe time! */
static void 	_wl_del_if(void *context);

/* Schedule _wl_add_wds() to be run at safe time. */
void 		wl_del_if(wl_info_t *wl, wl_if_t *wlif);

/* Interface creation routines */
static struct wl_if *wl_alloc_if(wl_info_t *wl, int iftype, uint subunit, struct wlc_if*wlcif,
                                 struct ether_addr *remote, bool attach_now, bool vap);

/* Deferred interface attachment */
static void wl_if_attach(wl_if_t *wlif);

/* Interface detach */
static void 	wl_free_if(wl_info_t *wl, wl_if_t *wlif);

#ifdef DWDS
void
wl_dwds_del_if(wl_info_t *wl, wl_if_t *wlif, bool force);
#endif

/* BPF/Ethereal Monitor interface */

/* WL Debug and diagnostics */

/* Net80211 API */


/* Handle individual if updowns when multiple interfaces exist */
static void wl_vap_close(struct ifnet *, int);	/* VAP stop routine */
static int wl_vap_open(struct ifnet *);		/* VAP start routine */

static void wl_wlif_down(wl_info_t *wl, wl_if_t *wlif);
static int wl_wlif_up(wl_info_t *wl, wl_if_t *wlif);

static bool wl_wlif_apmode(wl_info_t *wl, wl_if_t *wlif);

static int wl_ieee80211_ioctl(wl_info_t *wl, wl_if_t *wlif, struct ifnet *ifp, u_long cmd,
                              char * data);

#if __NetBSD_Version__ >= 500000003
static int wl_ieee80211_ifcreate(struct if_clone *ifc, int unit, const void *params);
#else
static int wl_ieee80211_ifcreate(struct if_clone *ifc, int unit, caddr_t params);
#endif

static int  wl_ieee80211_ifdestroy(struct ifnet*ifp);

static int  wl_ieee80211_ioctl_set(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq);
static int  wl_ieee80211_ioctl_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq);
static void wl_ieee80211_event(struct ifnet *ifp, wlc_event_t *e, int ap_mode);

/* NetBSD bus and device attach glue */
static int	wl_match(struct device *, struct cfdata *, void *);
static void	wl_attach(struct device *, struct device *, void *);
static int	wl_detach(struct device*, int);

static INLINE int wl_iovar_setint(wl_info_t *wl, const char *name, int arg, struct wl_if *wlif,
	struct wlc_if *wlcif);
static INLINE int wl_iovar_getint(wl_info_t *wl, const char *name, int *arg, struct wl_if *wlif,
	struct wlc_if *wlcif);

static int wl_ieee80211_curchan_get(wl_if_t *wlif, struct ieee80211_channel *chan);
static int wl_ieee80211_getchannels(wl_if_t *wlif, u_int *nchans, struct ieee80211_channel *chans);
static void chanspec_to_channel(wlc_info_t *wlc, struct ieee80211_channel *c, chanspec_t chanspec,
	bool isht);
static void wlc_add_sta_ie(wl_if_t *wlif, struct ether_addr *addr, void *data, uint32 datalen);
static void wlc_free_sta_ie(wl_if_t *wlif, struct ether_addr *addr, bool deleteall);
static int wlc_get_sta_ie(wl_if_t *wlif, struct ether_addr *ea, uint8 *ie, uint16 *ie_len);
static uint16 wlc_get_sta_ie_len(wl_if_t *wlif, struct ether_addr *ea);

static size_t convert_appie(wl_info_t *wl, struct ieee80211_appie *app,
	vndr_ie_setbuf_t **vndr_ie_buf, uint8_t ft);
static int setappie(wl_if_t *wlif, vndr_ie_setbuf_t **aie, size_t *len,
	const struct ieee80211req *ireq, uint8_t ft);

static int setwpaauth(wl_if_t *wlif, const struct ieee80211req *ireq);

CFATTACH_DECL(bwl, sizeof(struct wl_softc),
    wl_match, wl_attach, wl_detach, NULL);

static struct if_clone wlan_cloner =
    IF_CLONE_INITIALIZER("wlan", wl_ieee80211_ifcreate, wl_ieee80211_ifdestroy);


#if WLSOFTIRQ
/* Packet exchange routines */
static void
wl_dpc(void *data)
{
	wl_info_t *wl;
	wlc_dpc_info_t dpci = {0};

	wl = (wl_info_t*) data;

	ASSERT(wl);

	ASSERT(wl->magic == WLINFO_MAGIC);

	WL_LOCK(wl);

	wl->callbacks--;

	if (wl->pub->up == FALSE)
		goto done;

	if (wl->resched)
		wlc_intrsupd(wl->wlc);

	/* call the common second level interrupt handler */
	wl->resched = wlc_dpc(wl->wlc, TRUE, &dpci);

	/* wlc_dpc() may bring the driver down */
	if (!wl->pub->up)
		goto done;

	if (wl->resched)
		softintr_schedule(wl->dpc);
	else {
		/* schedule the next tx dequeue */
		wl_sched_txdequeue(wl);

		/* re-enable interrupts */
		wlc_intrson(wl->wlc);
	}

done:
	WL_UNLOCK(wl);
}

#else

static void
wl_dpc(void *data)
{
	wl_info_t *wl;
	wlc_dpc_info_t dpci = {0};

	wl = (wl_info_t*) data;

	ASSERT(wl);

	ASSERT(wl->magic == WLINFO_MAGIC);

	wl->callbacks--;

	if (wl->pub->up == FALSE)
		return;

	wlc_intrsupd(wl->wlc);

	/* call the common second level interrupt handler */
	wl->resched = wlc_dpc(wl->wlc, FALSE, &dpci);

	/* wlc_dpc() may bring the driver down */
	if (!wl->pub->up)
		return;

	/* schedule the next tx dequeue */
	wl_sched_txdequeue(wl);

	/* re-enable interrupts */
	wlc_intrson(wl->wlc);
}
#endif /* WLSOFTIRQ */

/*
 * First-level interrupt processing.
 * Return TRUE if this was our interrupt, FALSE otherwise.
 */
int
wl_isr(void *isr_arg)
{
	wl_info_t *wl;
	bool ours, wantdpc;

	wl = (wl_info_t*)isr_arg;

	/* Call lock with IPL_NET to stop further hard interrupts from
	 * the network hardware. Normally WL_SOFTLOCK runs at IPL_SOFTNET
	 */
#if WLSOFTIRQ
	WL_LOCK(wl);
#endif

	/* call common first level interrupt handler */
	if ((ours = wlc_isr(wl->wlc, &wantdpc))) {
		/* if more to do... */
		if (wantdpc) {

			/* ...and call the second level interrupt handler */
			/* schedule dpc */
#if WLSOFTIRQ
			if (wl->dpc) {
				wl->callbacks++;
				ASSERT(wl->resched == FALSE);
				softintr_schedule(wl->dpc);
			}
#else
			wl->callbacks++;
			wl_dpc(isr_arg);
#endif /* WLSOFTIRQ */

		}
	}

#if WLSOFTIRQ
	WL_UNLOCK(wl);
#endif
	return ours;
}

static int
wl_match(struct device *self, struct cfdata *cfdata, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (wlc_chipmatch(PCI_VENDOR(pa->pa_id), PCI_PRODUCT(pa->pa_id)))
		return (1);

	return 0;
}

/*
 * Sets PCI bus master mode
 * Maps device registers into host VM
 * Sets up host to device intr map
 *
 * Interrupt not enabled.
 */

static int
wl_pci_mapdevice(struct wl_pci_softc *pci, char *devname)
{
	uint res;
	struct pci_attach_args 		*pa = &pci->pa;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcireg_t type;

	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	               pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	               PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_MEM_ENABLE);
	res = pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);

	if ((res & PCI_COMMAND_MEM_ENABLE) == 0) {
		aprint_error("couldn't enable memory mapping\n");
		goto bad;
	}

	if ((res & PCI_COMMAND_MASTER_ENABLE) == 0) {
		aprint_error("couldn't enable bus mastering\n");
		goto bad;
	}

#if __NetBSD_Version__ >= 500000003
	pa->pa_flags |= PCI_FLAGS_MEM_OKAY;
#else
	pa->pa_flags |= PCI_FLAGS_MEM_ENABLED;
#endif

	/*
	 * Setup memory-mapping of PCI registers.
	 */
	type = pci_mapreg_type(pa->pa_pc, pa->pa_tag, PCI_CFG_BAR0);
#ifdef __mips__
	/* Broadcom Hacking */
	type &= ~PCI_MAPREG_MEM_TYPE_64BIT;
#endif /* __mips__ */
	if (pci_mapreg_map(pa, PCI_CFG_BAR0, type, BUS_SPACE_MAP_LINEAR,
	                   &pci->iot, &pci->ioh, NULL, NULL)) {
		aprint_error("cannot map register space\n");
		goto bad;
	}

	/*
	 * Arrange interrupt line.
	 */
	if (pci_intr_map(pa, &pci->ih_t)) {
		aprint_error("couldn't map interrupt\n");
		goto bad1;
	}
	return 0;

bad:
bad1:
	return 1;

}


static int
wl_pci_plumbintr(struct wl_pci_softc *pci, char *devname)
{
	struct pci_attach_args 		*pa = &pci->pa;

	pci->ih = pci_intr_establish(pa->pa_pc, pci->ih_t, pci->ipl, pci->isr, pci->isr_arg);
	if (pci->ih == NULL) {
		WL_ERROR(("%s: couldn't establish interrupt\n", devname));
		return (EIO);
	}

	return 0;
}
/* Allocates net interface structure. Must be called from within perimeter lock
*/
static struct wl_if *
wl_alloc_if(wl_info_t *wl, int iftype, uint subunit,
            struct wlc_if* wlcif, struct ether_addr *remote, bool attach_now, bool vap)
{
	wl_if_t *wlif;
	wl_if_t *p;
	struct ifnet *ifp;
	const char *devname;

	/* make sure that not using VAP if not compiled in */
	/* ASSERT(vap == FALSE); */

	if (!(wlif = MALLOC(wl->osh, sizeof(wl_if_t)))) {
		WL_ERROR(("wl%d: wl_alloc_if: out of memory, malloced %d bytes\n",
		          (wl->pub) ? wl->pub->unit : subunit,
		          MALLOCED(wl->osh)));
		return NULL;
	}

	bzero(wlif, sizeof(wl_if_t));
	/*
	 * One should note that an interface must do multicast in order
	 * to support IPv6.
	 */
	ifp = IFP_WLIF(wlif);

	ifp->if_softc	= wlif;
	ifp->if_flags	= IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST | IFF_NOTRAILERS;

	/* Create regular virtual interface */
	if (!vap) {
		switch (iftype) {
		case	WL_IFTYPE_WDS:
			ifp->if_ioctl = wl_wds_ioctl;
			if (remote)
				bcopy(remote, &wlif->remote, ETHER_ADDR_LEN);
			devname = WL_WDS_IFNAME;
			break;

		case	WL_IFTYPE_BSS:
			ifp->if_ioctl = wl_ioctl;
			devname = WL_BSS_IFNAME;
			break;

		default: /* Error condition
			  * We have been given an invalid
			  * interface type
			  */
			ASSERT(0);
			return NULL;
		}

		if (subunit)
			snprintf(ifp->if_xname, IFNAMSIZ,
			         "%s%d.%d", devname, wl->pub->unit, subunit);
		else
			snprintf(ifp->if_xname, IFNAMSIZ,
			         "%s%d", devname, wl->pub->unit);

		ifp->if_stop	= wl_close;
		ifp->if_init	= wl_open;
	} else {
		/* WDSVAP : Copy the remote address and fix the subunit */
		if (iftype == WL_IFTYPE_WDS) {
			ifp->if_ioctl = wl_wds_ioctl;
			if (remote)
				bcopy(remote, &wlif->remote, ETHER_ADDR_LEN);
			/* ignore the one came from driver */
			subunit = wl->wdsvapidx;
#if NOT_YET
			devname = WL_WDS_IFNAME;
			if (subunit)
				snprintf(ifp->if_xname, IFNAMSIZ, "%s%d.%d", devname,
					wl->pub->unit, subunit);
			else
				snprintf(ifp->if_xname, IFNAMSIZ, "%s%d", devname,
					wl->pub->unit);
#endif
		}

		/* Use the provided VAP name  */
		ASSERT(wl->vap[subunit].vap_name != NULL);
		snprintf(ifp->if_xname, IFNAMSIZ,
		         "%s", wl->vap[subunit].vap_name);
		wl->vap[subunit].wlif = wlif;
		wlif->vapidx = subunit;

		ifp->if_stop	= wl_vap_close;
		ifp->if_init	= wl_vap_open;
		ifp->if_ioctl = wl_ioctl;
	}

	ifp->if_start	=
#if WLSOFTIRQ
		wl_input;
#else
		wl_start;
#endif
#if __NetBSD_Version__ >= 500000003
	ifp->if_initaddr = wl_initaddr;
#endif /* __NetBSD_Version__ > 106200000 */

	/* Indicate the we support the Alt-Q mechanism */
	IFQ_SET_READY(&ifp->if_snd);

	wlif->ec.ec_capabilities = ETHERCAP_VLAN_MTU;
	wlif->type = iftype;
	wlif->wl = wl;
	wlif->wlcif = wlcif;
	wlif->subunit = subunit;
	wlif->magic = WLIF_MAGIC;
	wlif->attached = FALSE;
	wlif->next = NULL;

	/* add the interface to the interface linked list */
	if (wl->iflist == NULL)
		wl->iflist = wlif;
	else {
		int count;
		count = 0;
		p = wl->iflist;
		while (p->next != NULL) {
			/* Check bounds */
			ASSERT((count++) < wl->num_if);
			p = p->next;
		}
		p->next = wlif;
	}

	wl->num_if++;

	if (attach_now)
		wl_if_attach(wlif);

	return wlif;
}

static
void wl_ifmedia_init(wl_if_t *wlif, struct ifnet *ifp)
{
	int	list[3];
	int	i;
	size_t j;
	unsigned int mopts5g[] = {IFM_IEEE80211_11NA};
	unsigned int mopts2g[] = {IFM_IEEE80211_11NG};
	unsigned int *moptsptr;
	size_t moptssize;

	/*
	 * Note that there are 3 steps: init, one or several additions to
	 * list of supported media, and in the end, the selection of one
	 * of them.
	 */

	ifmedia_init(&wlif->im, 0, wl_mediachange, wl_mediastatus);

	ifmedia_add(&wlif->im, IFM_MAKEWORD(IFM_IEEE80211, IFM_AUTO, IFM_AUTO, 0), 0, NULL);


	/* Get the band list to see what media types the chip supports */
	if ((wl_wlc_ioctl(ifp, WLC_GET_BANDLIST, list, sizeof(list))) == 0) {
		if (!list[0])
			return;
		else if (list[0] > 2)
			list[0] = 2;
		for (i = 1; i <= list[0]; i++) {
			moptsptr = NULL;
			switch (list[i]) {
			case WLC_BAND_5G:
				moptsptr = mopts5g;
				moptssize = sizeof(mopts5g);

				break;
			case WLC_BAND_2G: /* For 2G band, set both 11b and 11g capable */
				moptsptr = mopts2g;
				moptssize = sizeof(mopts2g);
				break;
			}
			if (moptsptr) {
				for (j = 0; j < moptssize/sizeof(unsigned int); j++)
				{

					ifmedia_add(&wlif->im,
						IFM_MAKEWORD(IFM_IEEE80211, IFM_AUTO,
						moptsptr[j], 0), 0, NULL);
					ifmedia_add(&wlif->im,
						IFM_MAKEWORD(IFM_IEEE80211, IFM_AUTO,
						moptsptr[j] |
						IFM_IEEE80211_HOSTAP, 0), 0, NULL);
					ifmedia_add(&wlif->im,
						IFM_MAKEWORD(IFM_IEEE80211, IFM_AUTO,
						moptsptr[j] |
						IFM_IEEE80211_WDS, 0), 0, NULL);
				}
			}
		}
	}

}

/* 	Allocates net interface structure.
 *	Must be called from within perimeter lock
 */
static void wl_if_attach(wl_if_t *wlif)
{
	wl_info_t *wl;
	struct	ifnet *ifp;
	int	ipl;
	wlc_bsscfg_t *cfg;

	ASSERT(wlif);
	ASSERT(wlif->magic == WLIF_MAGIC);

	ifp = IFP_WLIF(wlif);
	wl = wlif->wl;

	ipl = splnet();

	cfg = wlc_bsscfg_find_by_wlcif(wl->wlc, wlif->wlcif);

	wl_ifmedia_init(wlif, ifp);
	/* Those steps are mandatory for an Ethernet driver, the first call
	 * being common to all network interface drivers.
	 */

	if_attach(ifp);
	ether_ifattach(ifp, cfg->cur_etheraddr.ether_addr_octet);
#if __NetBSD_Version__ >= 500000003
	bpf_attach2(ifp, DLT_IEEE802_11, ifp->if_hdrlen, &wlif->rawbpf);
#else
	bpfattach2(ifp, DLT_IEEE802_11, ifp->if_hdrlen, &wlif->rawbpf);
#endif

	if (!WLIF_WDS(wlif)) {
		wl_iovar_setint(wl, "wsec", 0, wlif, wlif->wlcif);
		wl_iovar_setint(wl, "wpa_auth", 0, wlif, wlif->wlcif);
		/* dont ever filter multicast */
		wl_iovar_setint(wl, "allmulti", 1, wlif, wlif->wlcif);
	}

	splx(ipl);

	WL_LOCK(wl);
	wlif->attached = TRUE;
	WL_UNLOCK(wl);

	/* match current flow control state to primary interface */
	if ((IFP_WLIF(wl->iflist))->if_flags & IFF_OACTIVE)
		WL_STOP_IFQUEUE(ifp, ipl);
}

static void
wl_free_if(wl_info_t *wl, wl_if_t *wlif)
{
	wl_if_t *p;
	int net_ipl;
	struct ifnet *ifp = IFP_WLIF(wlif);

	ASSERT(wlif);
	ASSERT(wlif->magic == WLIF_MAGIC);

	ifp = IFP_WLIF(wlif);

	if (ifp->if_flags | (IFF_UP|IFF_RUNNING))
			ifp->if_flags &= ~(IFF_UP|IFF_RUNNING);

	WL_LOCK(wl);

	wlif->magic = 0;

	/* remove the interface from the interface linked list */
	p = wl->iflist;
	if (p == wlif)
		wl->iflist = p->next;
	else {
		int count;
		count = 0;
		while (p != NULL && p->next != wlif) {
			/* Check bounds */
			ASSERT((count++) < wl->num_if);
			p = p->next;
		}
		if (p != NULL)
			p->next = p->next->next;
	}
	wl->num_if--;

	if (wlif->attached) {
		net_ipl = splnet();
		ether_ifdetach(ifp);
		if_detach(ifp);
		ifmedia_delete_instance(&wlif->im, IFM_INST_ANY);
		splx(net_ipl);
	}

	WL_UNLOCK(wl);
	MFREE(wl->osh, wlif, sizeof(wl_if_t));

}

/*
 * wl_shutdown: Invoked at reboot
 */
static void
wl_shutdown(void *arg)
{
	struct wl_softc *sc = arg;
	wl_close(IFP_WLIF(sc->wl->iflist), 1);
}

#if APPLE_DEFFERED_ATTACH
static bool
wl_map_sku_to_cpec(wl_country_t *cspec)
{
	size_t i;
	char *sku;
	sku = nvram_get("apple-sku");
	if (sku) {
		for (i = 0; i < sizeof(sku_cspec_map)/sizeof(struct sku_cspec_map_entry); i++) {
			if (!strcmp(sku, sku_cspec_map[i].sku)) {
				memcpy(cspec, &sku_cspec_map[i].cspec, sizeof(wl_country_t));
				return TRUE;
			}
		}
	}
	return FALSE;
}
#endif /* APPLE_DEFFERED_ATTACH */

static int
wl_set_radarthrs(wl_info_t *wl)
{
	wl_radar_thr_t radar_thrs;

	radar_thrs.version = WL_RADAR_THR_VERSION;

	/* Order thresh0_20_lo, thresh1_20_lo, thresh0_40_lo, thresh1_40_lo,
	 * thresh0_80_lo, thresh1_80_lo,
	 * thresh0_20_hi, thresh1_20_hi, thresh0_40_hi, thresh1_40_hi,
	 * thresh0_80_hi, thresh1_80_hi
	 */
	radar_thrs.thresh0_20_lo = 0x694;
	radar_thrs.thresh1_20_lo = 0x30;
	radar_thrs.thresh0_40_lo = 0x694;
	radar_thrs.thresh1_40_lo = 0x30;
	radar_thrs.thresh0_80_lo = 0x69b;
	radar_thrs.thresh1_80_lo = 0x28;
	radar_thrs.thresh0_20_hi = 0x694;
	radar_thrs.thresh1_20_hi = 0x30;
	radar_thrs.thresh0_40_hi = 0x694;
	radar_thrs.thresh1_40_hi = 0x30;
	radar_thrs.thresh0_80_hi = 0x69b;
	radar_thrs.thresh1_80_hi = 0x28;

	return wlc_iovar_op(wl->wlc, "radarthrs", NULL, 0, &radar_thrs, sizeof(wl_radar_thr_t),
		IOV_SET, wl->base_if->wlcif);
}

static void
wl_do_attach(struct device *self)
{
	struct wl_softc 	*sc = (struct wl_softc *)self;
	wl_info_t		*wl;
	uint			err;
	int 			ipl;
	struct	ifnet *ifp;
	struct wl_pci_softc	*pci = &sc->pci;
	uint8 *vaddr; /* opaque chip registers virtual address */
	int int_val;
	pcireg_t address;
	struct pci_attach_args 	*pa = &pci->pa;
	static int cloneattached = 0;
	char path[64];
	char *devidstr;
	uint16 devid;
	/* hard code j28 pcieserdes values for now.
	 * need to fix in the future
	 */
	uint32 arg3[3];
	int i;
#if APPLE_DEFFERED_ATTACH
	wl_country_t cspec = {{0}, 0, {0}};
#endif
	int vhtmode = 0;
#ifdef WL11K
	dot11_rrm_cap_ie_t rrm_cap;
#endif  /* WL11K */

	/* Allocate memory for wl structure */
	sc->wl = malloc(sizeof(wl_info_t), M_DEVBUF, M_WAITOK|M_ZERO);
	if (sc->wl == NULL) {
		WL_ERROR(("%s: %s failed to allocate %d bytes for wl structure\n",
			sc->dev.dv_xname, EPI_VERSION_STR, sizeof(wl_info_t)));
		return;
	};

	/* Back pointer to our device control block */
	sc->wl->sc = sc;

/* Map the interrupt, BAR and registers of the PCI device */

	pci->regbase = PCI_MAPREG_START;	/* Start of mapped region */
	pci->ipl = WL_HARDINTR; 		/* Interrupt priority level = network */
	pci->maptype = PCI_MAPREG_TYPE_MEM; /* Use memory mapped io */

	/* Find out address type of the chip (32-bit/64-bit) */
	ipl = splhigh();
	address = pci_conf_read(pa->pa_pc, pa->pa_tag, pci->regbase);
	splx(ipl);

	pci->maptype |= PCI_MAPREG_MEM_TYPE(address);

	if (wl_pci_mapdevice(&sc->pci, sc->dev.dv_xname))
		goto bad1;

	/* vaddr needed as siutils and others will asssert if this is zero */
	vaddr = bus_space_vaddr(pci->iot, pci->ioh);

	/* need a way for osl to differentiate devices to load correct cal
	 * data pa_device is used to set the os_slot in the OSL. on j28 both devices have the same
	 * bus/device number b/c of the way the radios are hooked up so use dv_unit instead.
	 * nothing else uses OSL_PCI_SLOT other than si_devpath.
	 */
#if APPLE_DEFFERED_ATTACH
	pa->pa_device = device_cfdata(self)->cf_flags;
#else
	pa->pa_device = sc->dev.dv_unit;
#endif
	/* Attach the OSL structure */
	sc->wl->osh = osl_attach(pa, TRUE, sc->dev.dv_xname, OSL_PCI_BUS,
	                         pa->pa_dmat, pci->iot, pci->ioh);
	if (sc->wl->osh == NULL)
		goto bad;

	/* common load-time initialization */
	wl = sc->wl;
	wl->bustype = PCI_BUS;
	wl->piomode = FALSE;
	wl->monitor = NULL;
	wl->magic = WLINFO_MAGIC;
	wl->num_if = 0;
	wl->callbacks = 0;
	wl->wlc = NULL;
	wl->txparams = NULL;
	wl->scandone = FALSE;
	/* initialize assoc sta ie list */
	SLIST_INIT(&wl->assoc_sta_ie_list);

	/* Init the perimeter lock. */
	WL_LOCK_INIT(wl, device_xname(sc->dev));

	snprintf(path, sizeof(path), "pci/%u/%u/devid",
		pa->pa_bus,
		pa->pa_device);
	devidstr = nvram_get(path);
	if (devidstr) {
		devid = (uint16)bcm_strtoul(devidstr, NULL, 0);
	} else {
		devid = PCI_PRODUCT(pa->pa_id);
	}

	/* Attach the OS independent bits */
	if (!(wl->wlc = wlc_attach((void *)wl, PCI_VENDOR(pa->pa_id), devid,
	                           sc->dev.dv_unit, wl->piomode, wl->osh,
	                           vaddr, wl->bustype, NULL, NULL, &err))) {
		WL_ERROR(("%s: %s Driver failed with code %d\n",
		          sc->dev.dv_xname, EPI_VERSION_STR,  err));
		goto bad1;
	}
	wl->pub = wl->wlc->pub;

	if (!cloneattached) {
		if_clone_attach(&wlan_cloner);
		cloneattached = 1;
	}

	/* Create the primary device  */
	WL_LOCK(wl);
	wl->iflist = wl_alloc_if(wl, WL_IFTYPE_BSS, 0, NULL, NULL, TRUE, FALSE);
	WL_UNLOCK(wl);

	if (wl->iflist == NULL) {
		WL_ERROR(("%s: %s Primary interface allocation failed\n",
		          sc->dev.dv_xname, EPI_VERSION_STR));
		goto bad2;
	}

	wl->base_if = wl->iflist;
	/* Default vapidx */
	wl->base_if->vapidx = 0;

	/* Populate wlcif of the primary interface in wlif */
	wl->base_if->wlcif = wlc_wlcif_get_by_index(wl->wlc, 0);

	ifp = IFP_WLIF(wl->base_if);
	ifp->if_type = IFT_IEEE80211;
	/* Set infra mode always to disable IBSS */
	int_val = 1;
	if (wl_wlc_ioctl(ifp, WLC_SET_INFRA, &int_val, sizeof(int_val))) {
		WL_ERROR(("wl: Error setting INFRA mode\n"));
	}

	if (wl_iovar_setint(wl, "mpc", 0, wl->base_if, wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error disabling MPC\n"));
	}

	if (wl_iovar_setint(wl, "mbss", 1, wl->base_if, wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error enabling mbss\n"));
	}

	int_val = 1;
	if (wl_wlc_ioctl(ifp, WLC_SET_REGULATORY, &int_val, sizeof(int_val))) {
		WL_ERROR(("wl: Error enabling regulatory\n"));
	}

	int_val = 1;
	if (wl_wlc_ioctl(ifp, WLC_SET_RADAR, &int_val, sizeof(int_val))) {
		WL_ERROR(("wl: Error enabling radar\n"));
	}

	int_val = SPECT_MNGMT_LOOSE_11H;
	if (wl_wlc_ioctl(ifp, WLC_SET_SPECT_MANAGMENT, &int_val, sizeof(int_val))) {
		WL_ERROR(("wl: Error setting loose spectrum managment\n"));
	}

#ifdef STA
	if (wl_iovar_setint(wl, "roam_off", 1, wl->base_if, wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error disabling roaming\n"));
	}
#endif

	if (wl_iovar_setint(wl, "rx_amsdu_in_ampdu", 0, wl->base_if, wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error disabling amsdu in ampdu\n"));
	}

	/* disable AMSDU tx for now until we get tx chaining implemented */
	if (wl_iovar_setint(wl, "amsdu", 0, wl->base_if, wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error disabling amsdu\n"));
	}

#ifdef WL11K
	/* Disable rrm by default.  let individual interfaces enable/disable
	 */
	bzero(&rrm_cap, DOT11_RRM_CAP_LEN);
	if (wl_iovar_op(wl, "rrm", NULL, 0, &rrm_cap, DOT11_RRM_CAP_LEN, IOV_SET, wl->base_if,
		wl->base_if->wlcif))
		WL_ERROR(("wl: Error disabling rrm\n"));
#endif	/* WL11K */

	/* hard code j28 pcieserdes values for now.
	 * need to fix in the future
	 */
	arg3[0] = 0x808;
	arg3[1] = 0x1c;
	arg3[2] = 0x000b;
	if (wlc_iovar_op(wl->wlc, "pcieserdesreg", NULL, 0, arg3, sizeof(arg3), IOV_SET,
		wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error setting pcieserdesreg\n"));
	}

	arg3[0] = 0x863;
	arg3[1] = 0x13;
	arg3[2] = 0x0190;
	if (wlc_iovar_op(wl->wlc, "pcieserdesreg", NULL, 0, arg3, sizeof(arg3), IOV_SET,
		wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error setting pcieserdesreg\n"));
	}

	arg3[0] = 0x863;
	arg3[1] = 0x19;
	arg3[2] = 0x0191;
	if (wlc_iovar_op(wl->wlc, "pcieserdesreg", NULL, 0, arg3, sizeof(arg3), IOV_SET,
		wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error setting pcieserdesreg\n"));
	}

	arg3[0] = 0x830;
	arg3[1] = 0x10;
	arg3[2] = 0x200;
	if (wlc_iovar_op(wl->wlc, "pcieserdesreg", NULL, 0, arg3, sizeof(arg3), IOV_SET,
		wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error setting pcieserdesreg\n"));
	}

	arg3[0] = 0x820;
	arg3[1] = 0x16;
	arg3[2] = 0xd800;
	if (wlc_iovar_op(wl->wlc, "pcieserdesreg", NULL, 0, arg3, sizeof(arg3), IOV_SET,
		wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error setting pcieserdesreg\n"));
	}

	arg3[0] = 0x800;
	arg3[1] = 0x10;
	arg3[2] = 0x200c;
	if (wlc_iovar_op(wl->wlc, "pcieserdesreg", NULL, 0, arg3, sizeof(arg3), IOV_SET,
		wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error setting pcieserdesreg\n"));
	}

	/* Enable beamforming by default */
	/* Beamformer (txbf_bfr_cap) and beamformee (txbf_bfe_cap) are set
	 * during attach; enable the functionality here.
	 */
	if (wl_iovar_setint(wl, "txbf", 1, wl->base_if, wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error enabling Beamforming\n"));
	}

	/* STBC settings */
	if (wl_iovar_setint(wl, "stbc_tx", -1, wl->base_if, wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error enabling stbc_tx\n"));
	}
	if (wl_iovar_setint(wl, "stbc_rx", 1, wl->base_if, wl->base_if->wlcif)) {
		WL_ERROR(("wl: Error enabling stbc_rx\n"));
	}

	/* init txparams */
	if ((wl->txparams = MALLOC(wl->osh, sizeof(struct ieee80211_txparams_req))) == NULL)
		goto bad2;
	bzero(wl->txparams, sizeof(struct ieee80211_txparams_req));

#if WLSOFTIRQ
	/* wl_isr's DPC */
	wl->dpc = softintr_establish(WL_SOFTINTR, wl_dpc, (void *)wl);

	/* tx dequeue DPC */
	wl->txdequeue = softintr_establish(WL_SOFTINTR, wl_txdequeue, (void *)wl);
#endif

	/* Enable hardware interrupts */
	pci->isr = wl_isr;
	pci->isr_arg = wl;
	if (wl_pci_plumbintr(pci, sc->dev.dv_xname)) {
		WL_ERROR(("%s: %s failed to plumb device interrupt\n",
		          sc->dev.dv_xname, EPI_VERSION_STR));
		goto bad2;
	}
	/* Establish cleanup handler. Runs prior to reboot. */
	sc->sdhook = shutdownhook_establish(wl_shutdown, sc);
	if (sc->sdhook == NULL)
		WL_ERROR(("%s: %s WARNING: unable to establish shutdown hook\n",
		          sc->dev.dv_xname, EPI_VERSION_STR));

	sc->magic = SC_MAGIC;

	/* print hello string */
	printf("\n%s: Broadcom BCM%04x 802.11 Wireless Controller " EPI_VERSION_STR"\n",
	       sc->dev.dv_xname, PCI_PRODUCT(pa->pa_id));

#if APPLE_DEFFERED_ATTACH
	if (wl_map_sku_to_cpec(&cspec)) {
		if ((err = wl_iovar_op(wl, "country", NULL, 0, &cspec, sizeof(cspec),
			IOV_SET, wl->base_if, NULL))) {
			WL_ERROR(("%s: Failed to set country %s/%d\n",
				sc->dev.dv_xname, cspec.country_abbrev, cspec.rev));
			goto bad2;
		}
	} else {
		WL_ERROR(("%s: NO SKU configured!  Using defaults\n", sc->dev.dv_xname));
	}
#endif

	wl_ieee80211_getchannels(wl->base_if, &wl->nchans, wl->chans);

	wl->modecaps = 0;

	for (i = 0; i < wl->nchans; i++)
	{
		wl->modecaps |= 1<< ieee80211_chan2mode(&wl->chans[i]);
	}

	/* we need this hack b/c we re-use
	 * ht20/ht40 channels flags for vht20/vht40.  if there are no vht80 channels
	 * present then we end up not setting IEEE80211_MODE_11AC - not checking error
	 * b/c if this fails then we just wont 11ac mode
	 */
	wl_iovar_getint(wl, "vhtmode", &vhtmode, wl->base_if, wl->base_if->wlcif);
	if (vhtmode && (wl->modecaps & (1 << IEEE80211_MODE_11NA))) {
		wl->modecaps |= 1 << IEEE80211_MODE_11AC;
	}

	/* enable KNOISE by default */
	wlc_iovar_setint(wl->wlc, "noise_metric", NOISE_MEASURE_KNOISE);

	return;

bad2:
	if (wl->txparams) {
		MFREE(wl->osh, wl->txparams, sizeof(struct ieee80211_txparams_req));
		wl->txparams = NULL;
	}
	wlc_detach(wl->wlc);
	wl->pub = NULL;
bad1:
	osl_detach(sc->wl->osh);
	sc->wl->osh = NULL;
bad:
	free(sc->wl, M_DEVBUF);
	sc->wl = NULL;
	return;
}

#if APPLE_DEFFERED_ATTACH
static int
wl_finish_attach(struct device *self)
{
	if (static_config_initialized()) {
		wl_do_attach(self);
		return 0;
	} else {
		return 1;
	}
}
#endif

static void
wl_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args 	*pa = aux;
	struct wl_softc 	*sc = (struct wl_softc *)self;
	struct wl_pci_softc	*pci = &sc->pci;

	sc->magic = 0;

	/* Copy the pci attach args as this is volatile */
	bcopy(pa, &pci->pa, sizeof(struct pci_attach_args));

#if APPLE_DEFFERED_ATTACH
	config_finalize_register(self, wl_finish_attach);
#else
	wl_do_attach(self);
#endif
}

/*
 * When detaching, we do the inverse of what is done in the attach
 * routine, in reversed order.
 */
static int
wl_detach(struct device* self, int flags)
{
	struct wl_softc *sc = (struct wl_softc *)self;
	struct ifnet *ifp;
	struct wl_pci_softc *pci = &sc->pci;
	struct pci_attach_args   *pa = &pci->pa;
	wl_info_t *wl;
	wl_timer_t *t, *next;
	int ipl;

	ASSERT(sc);

	/* This prevents the device from being detached more than once */
	if (sc->magic != SC_MAGIC)
		return 0;

	wl = sc->wl;

	/* Take the primary interface offline. It begins the process
	   of shutting down the secondaries.
	*/
	ifp = IFP_WLIF(wl->iflist);
	wl_close(ifp, 1);

	sc->magic = 0;

	/* Detach shutdown hook */
	shutdownhook_disestablish(sc->sdhook);

	ipl = splnet();

#if WLSOFTIRQ
	/* Detach dpcs */
	softintr_disestablish(wl->dpc);
	softintr_disestablish(wl->txdequeue);
#endif
	wl->dpc = NULL;
	wl->txdequeue = NULL;
	splx(ipl);


	/* Detach wl */
	if (wl->wlc) {
		wl->callbacks = wlc_detach(wl->wlc);
		wl->wlc = NULL;
		wl->pub = NULL;

		/* wait for all pending callbacks to complete */
		if (wl->callbacks)
			wl_wait_callbacks(wl);

		/* free timers */
		WL_LOCK(wl);
		for (t = wl->timers; t; t = next) {
			next = t->next;
			wl_del_timer(wl, t);
			MFREE(wl->osh, t, sizeof(wl_timer_t));
		}
		WL_UNLOCK(wl);
	}

	/* If this is not null not all the secondaries
	   have been deleted. Uh-oh...
	*/
	ASSERT(wl->iflist->next == NULL);

	/* Detach the primary interface */
	wl_free_if(wl, wl->iflist);
	wl->iflist = NULL;

	wl->magic = 0;

	/* Check for memory leaks */
	if (MALLOCED(wl->osh)) {
		printf("Memory leak of bytes %d\n", MALLOCED(wl->osh));
		ASSERT(0);
	}

	if (wl->txparams) {
		MFREE(wl->osh, wl->txparams, sizeof(struct ieee80211_txparams_req));
		wl->txparams = NULL;
	}

	/* Detach OSL */
	if (wl->osh)
		osl_detach(wl->osh);

	/* Deallocate wl structure */
	free(sc->wl, M_DEVBUF);
	sc->wl = NULL;

	/* Detach hardware. This must be done
	  after wlc_detach to ensure the core wl driver
	  has had a chance to do its clean up
	*/
	if (pci->ih)
		pci_intr_disestablish(pa->pa_pc, pci->ih);

	/* Unmap pci device registers */
	if (pci->mapsize)
		bus_space_unmap(pci->iot,
			pci->ioh, pci->mapsize);

	if (!(flags & DETACH_QUIET))
		printf("%s:%s detached\n",
			__FUNCTION__, sc->dev.dv_xname);

	return (0);
}


/*
 * This function is called by the ifmedia layer to notify the driver
 * that the user requested a media mode/option change.
 */
static int
wl_mediachange(struct ifnet *ifp)
{
	wl_if_t *wlif;
	wl_info_t *wl;
	struct ifmedia_entry *ime;
	int ap = 0;
	int band, newband;
	int error = 0;

	wlif = WLIF_IFP(ifp);
	wl = wlif->wl;
	ime = wlif->im.ifm_cur;

	/* Get the current band */
	if (wl_wlc_ioctl(ifp, WLC_GET_BAND, &band, sizeof(uint)))
		band = WLC_BAND_AUTO;

	switch (IFM_MODE(ime->ifm_media)) {
	case IFM_IEEE80211_11NA:
		newband = WLC_BAND_5G;
		break;
	case IFM_IEEE80211_11B: /* 11b mode treated as 2G band only */
	case IFM_IEEE80211_11NG:
		newband = WLC_BAND_2G;
		break;
	default:
		newband = WLC_BAND_AUTO;
	}

	if (band != newband)
		if ((error = wl_wlc_ioctl(ifp, WLC_SET_BAND, &newband, sizeof(uint))))
			return error;

#ifdef AP
	/* Allow change of AP mode only when not in APSTA */
	if (APSTA_ENAB(wl->pub))
		return 0;

	/* Set/Clear AP mode */
	if (ime->ifm_media & IFM_IEEE80211_HOSTAP)
		ap = 1;
	else
		ap = 0;

	/* Don't do anything if already in AP mode */
	if (AP_ENAB(wl->pub) != ap)
		return (wl_wlc_ioctl(ifp, WLC_SET_AP, &ap, sizeof(ap)));
#endif

	return 0;
}

/*
 * Here the user asks for the currently used media.
 */
static void
wl_mediastatus(struct ifnet *ifp, struct ifmediareq *imr)
{
	int val;
	wl_if_t *wlif;
	wl_info_t *wl;
	char bssid[IEEE80211_ADDR_LEN];
	int status;

	imr->ifm_status = IFM_AVALID;
	imr->ifm_active = IFM_IEEE80211;

	wlif = WLIF_IFP(ifp);
	wl = wlif->wl;

	/* Get the current band */
	if (wl_wlc_ioctl(ifp, WLC_GET_BAND, &val, sizeof(uint)))
		val = WLC_BAND_AUTO;

	switch (val) {
	case WLC_BAND_2G:
		imr->ifm_active |= IFM_IEEE80211_11NG;
		break;
	case WLC_BAND_5G:
		imr->ifm_active |= IFM_IEEE80211_11NA;
		break;
	case WLC_BAND_AUTO:
		imr->ifm_active |= IFM_AUTO;
		break;
	}

	if (WLIF_WDS(wlif)) {
		imr->ifm_active |= IFM_IEEE80211_WDS;
		if (wl->pub->up)
			imr->ifm_status |= IFM_ACTIVE;
	} else if (WLIF_AP_MODE(wl, wlif)) {
		imr->ifm_active |= IFM_IEEE80211_HOSTAP;
		status = wlif->vapidx;
		wl_iovar_op(wl, "bss", NULL, 0, &status, sizeof(status), IOV_GET,
		                    wlif, wlif->wlcif);
		if (status) {
			imr->ifm_status |= IFM_ACTIVE;
		}
	} else {
		imr->ifm_active |= IEEE80211_M_STA;
		if (!wl_wlc_ioctl(ifp, WLC_GET_BSSID, &bssid, sizeof(bssid))) {
			imr->ifm_status |= IFM_ACTIVE;
		}
	}


	if (imr->ifm_status & IFM_ACTIVE)
		imr->ifm_current = imr->ifm_active;
}

static void
wl_sched_txdequeue(wl_info_t *wl)
{

	ASSERT(wl);
	ASSERT(wl->iflist);

#if WLSOFTIRQ
	if (wl->txdequeue == NULL)
		return;
#endif

	if (wl->txflowcontrol == ON)
		return;

#if WLSOFTIRQ
	/* Bail if there is a pending request */
	if (wl->dqwait)
		return;

	wl->dqwait = 1;
	wl->callbacks++;
	softintr_schedule(wl->txdequeue);
#else
	wl_txdequeue(wl);
#endif /* WLSOFTIRQ */
}


/* This routine causes the dequeueing of TX packets.
 * The OS only calls the send routine for the packet header
 * and the driver must do the rest.
 */
static void
wl_txdequeue(void *arg)
{
	wl_info_t *wl = (wl_info_t *)arg;
	wl_if_t *wlif;
	struct ifnet *ifp;
	int count;


	ASSERT(wl->magic == WLINFO_MAGIC);

	wlif = wl->iflist;
	ASSERT(wlif->magic == WLIF_MAGIC);

	for (count = 0; count < wl->num_if; count ++) {
		ifp = IFP_WLIF(wlif);
		wl_start(ifp);
		if (wl->txflowcontrol == ON)
			break;
		wlif = wlif->next;
	}
	ASSERT(wlif == NULL);
#if WLSOFTIRQ
	WL_LOCK(wl);
	wl->callbacks--;
	wl->dqwait = 0;
	WL_UNLOCK(wl);
#endif /* WLSOFTIRQ */

}

#if WLSOFTIRQ
/* Function invoked by OS to indicate packets available
 * Schedule a DPC for dequeuing the packet
 */
static void
wl_input(struct ifnet *ifp)
{
	int ipl;
	wl_info_t *wl;
	wl_if_t *wlif;

	wlif = WLIF_IFP(ifp);
	ASSERT(wlif->magic == WLIF_MAGIC);

	wl = wlif->wl;
	ASSERT(wl->magic == WLINFO_MAGIC);

	/* Base i/f no data xfer */
	if (wlif == wl->base_if) {
		WL_ERROR(("No date transfer on base i/f\n"));
		return;
	}

	ipl = splnet();
	wl_sched_txdequeue(wl);
	splx(ipl);
}
#endif /* WLSOFTIRQ */

/* for frames from the stack
 * use classification from net80211 since it handles pkt chains
 * brcm pktsetprio implementation does not
 */
static uint8_t
wl_classify(struct mbuf *m)
{
	const struct ether_header *eh = mtod(m, struct ether_header *);
	uint8_t tos = 0;
#define IPV6_VERSION_MASK			0xF0000000
#define IPV6_PRIORITY_MASK			0x0FF00000
#define IPV6_FLOWLABEL_MASK			0x000FFFFF
#define IPV6_VERSION_SHIFT			28
#define IPV6_PRIORITY_SHIFT			20
#define IPV6_FLOWLABEL_SHIFT		0

	if (eh->ether_type == htons(ETHERTYPE_IP)) {
		/*
		 * IP frame, map the DSCP bits from the TOS field.
		 */
		m_copydata(m, sizeof(struct ether_header) +
			offsetof(struct ip, ip_tos), sizeof(tos), &tos);
		tos >>= IPV4_TOS_PREC_SHIFT;		/* NB: ECN + low 3 bits of DSCP */
	} else if (eh->ether_type == htons(ETHERTYPE_IPV6)) {
		uint32_t ver_pri_flowlabel;
		m_copydata(m, sizeof(struct ether_header), sizeof(ver_pri_flowlabel),
			&ver_pri_flowlabel);
		tos = (ntohl(ver_pri_flowlabel) & IPV6_PRIORITY_MASK) >> IPV6_PRIORITY_SHIFT;
		tos >>= IPV4_TOS_PREC_SHIFT;		/* NB: ECN + low 3 bits of DSCP */
	}
	return tos;
}

/* transmit a packet */
static void
wl_start(struct ifnet *ifp)
{
	wl_info_t *wl;
	wl_if_t *wlif;
	int net_ipl = 0;
	void *pkt;
	int err;
	struct mbuf *m = NULL;
	if ((ifp->if_flags & (IFF_RUNNING|IFF_UP)) == 0)
		return;

	wlif = WLIF_IFP(ifp);
	ASSERT(wlif->magic == WLIF_MAGIC);

	wl = wlif->wl;
	ASSERT(wl->magic == WLINFO_MAGIC);

	WL_LOCK(wl);
	/*
	 * Loop through the send queue, until we drain the queue,
	 * or use up all available transmit descriptors or
	 * when IFF_OACTIVE comes on.
	 */
	while (1) {

		if (IFQ_IS_EMPTY(&ifp->if_snd) || (ifp->if_flags & IFF_OACTIVE))
			break;

		m = NULL;

		IFQ_DEQUEUE(&ifp->if_snd, m);


		/* Under rate limiting IFQ_DEQUEUE() may return null
		 * even if the queue is not empty
		 */

		if (m == NULL)
			continue;

		MCLAIM(m, &wlif->ec.ec_tx_mowner);

		WL_TRACE(("%s: wl_start: len %d\n", ifp->if_xname, m->m_len));

		if (WLIF_WDS(wlif) && (wlif->wlcif == NULL)) {
			m_freem(m);
			continue;
		}

		/* Convert the packet. Mainly attach a pkttag */
		if ((pkt = PKTFRMNATIVE(wl->osh, m)) == NULL) {
			IFQ_INC_DROPS(&ifp->if_snd);
			ifp->if_oerrors++;
			break;
		}

		WL_STOP_IFQUEUE(ifp, net_ipl);

		/* Mark this pkt as coming from host/bridge. */
		WLPKTTAG(pkt)->flags |= WLF_HOST_PKT;

		/* force tx chaining off for now */
		m->m_pkthdr.rcvif = NULL;
		m->m_pkthdr.csum_flags = 0;

		if (WME_ENAB(wl->pub))
			/* for frames from the stack
			 * use classification from net80211 since it handles pkt chains
			 * brcm pktsetprio implementation does not
			 */
			PKTSETPRIO(pkt, wl_classify((struct mbuf *)pkt));

		/* wlc_sendkpt frees the mbuf after it is done. Returns TRUE if it failed */
		err = wlc_sendpkt(wl->wlc, pkt, wlif->wlcif);

		if (wlif->txflowcontrol == OFF)
			WL_WAKE_IFQUEUE(ifp, net_ipl);


		/* Update the OS network counters */

		if (err) {
			WL_ERROR(("%s:txfail\n", ifp->if_xname));
			IFQ_INC_DROPS(&ifp->if_snd);
			ifp->if_oerrors++;
			break;
		} else {
			/* update the counters */
			if (m->m_flags & M_MCAST)
				ifp->if_omcasts++;
			ifp->if_obytes += m->m_pkthdr.len;
			ifp->if_opackets++;
		}
	}

	WL_UNLOCK(wl);
	return;
}

/*
 * The last parameter was added for the build. Caller of
 * this function should pass 1 for now.
 * wl_sendup() must be called from within the perimeter lock.
 */
void
wl_sendup(wl_info_t *wl, wl_if_t *wlif, void *p, int numpkt)
{
	struct mbuf *m;
	struct ifnet *ifp;

	/* wlif can be NULL */
	ASSERT(wl->magic == WLINFO_MAGIC);

	if (wlif == NULL)
		wlif = wl->iflist;

	ASSERT(wlif->magic == WLIF_MAGIC);

	/* drop the pkt if the interface is not defined */
	if (wlif == NULL) {
		WL_ERROR(("wl%d: wl_sendup: i/f not configured\n", wl->pub->unit));
		goto drop_packet;
	}

	/* Switch the i/f to VAP for the default interface */
	if (wlif->vapidx == 0) {
		WL_INFORM(("%s: wl_sendup: vap index zero, defaulting to primary vap\n",
			(IFP_WLIF(wlif))->if_xname));
		if (wl->vap[0].wlif != NULL)
			wlif = wl->vap[0].wlif;
		else {
			WL_ERROR(("%s: wl_sendup: vap i/f not attached\n",
			          (IFP_WLIF(wlif))->if_xname));
			goto drop_packet;
		}
	}

	ifp = IFP_WLIF(wlif);

	/* if ifp is null the interface was not properly configured */
	ASSERT(ifp);

	/* drop pkt if interface is not attached. wlc does not check to
	 * see if interface is fully up before using it,
	 * since it could be plumbed by a dpc, a race condition exists.
	 */
	if (wlif->attached  == FALSE) {
		WL_ERROR(("%s: wl_sendup: i/f not attached\n", ifp->if_xname));
		goto drop_packet;
	}

	/* drop the pkt if the interface is not up */
	if (!(ifp->if_flags & IFF_UP)) {
		WL_ERROR(("%s: wl_sendup: i/f not up\n", ifp->if_xname));
		goto drop_packet;
	}


	WL_TRACE(("%s: wl_sendup: %d bytes\n",
		ifp->if_xname, PKTLEN(wl->osh, p)));

	/* Detach pkttag */
	m = PKTTONATIVE(wl->osh, p);

	WL_PRPKT("rxpkt", (uchar*)m->m_data, m->m_len);

	/* Clear the pointers used for PKTTAG */
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len =  m->m_len;
	m->m_pkthdr.csum_flags = 0;

#if NBPFILTER > 0
	/*
	 * Handle BPF listeners. Let the BPF user see the packet.
	 */
#if __NetBSD_Version__ >= 500000003
	if (ifp->if_bpf)
		bpf_mtap(ifp, m);
#else
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m);
#endif
#endif /* NBPFILTER */

#if WLSOFTIRQ
	WL_LOCK(wl);
#endif
	/* Update multicast counter */
	if (m->m_flags & M_MCAST)
		ifp->if_imcasts++;

	ifp->if_ibytes += m->m_len;
	ifp->if_ipackets++;

	/* Send it up the stack. */
	(*ifp->if_input)(ifp, m);

#if WLSOFTIRQ
	WL_UNLOCK(wl);
#endif

	WL_APSTA_RX(("wl%d: wl_sendup(): pkt %p on interface %p (%s)\n",
		wl->pub->unit, p, wlif, ifp->if_xname));

	return;

drop_packet:
	PKTFREE(wl->osh, p, FALSE);
	WLCNTINCR(wl->pub->_cnt->rxnobuf);
	return;
}

/* wds ioctl handler */
static int
#if __NetBSD_Version__ >= 500000003
wl_wds_ioctl(struct ifnet *ifp, u_long cmd, void * data)
#else
wl_wds_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
#endif
{
	wl_if_t *wds;
	wl_ioctl_t ioc;
	struct ifreq *ifr = (struct ifreq *)data;
	int ipl;
	int err_num = 0;

	wds = WLIF_IFP(ifp);

	WL_TRACE(("%s: wl_wds_ioctl: cmd 0x%x\n", ifp->if_xname, (int)cmd));

	if (cmd != SIOCDEVPRIVATE)
		return EINVAL;

	ipl = splnet();
	if (copyin(ifr->ifr_data, &ioc, sizeof(wl_ioctl_t))) {
		err_num = EFAULT;
		goto done;
	}

	switch (ioc.cmd) {
	case WLC_WDS_GET_REMOTE_HWADDR:
		if (!ioc.buf || ioc.len < ETHER_ADDR_LEN) {
			err_num = EINVAL;
			goto done;
		}
		copyout(wds->remote.ether_addr_octet, ioc.buf, ETHER_ADDR_LEN);
		break;

	default:
		err_num = wl_ioctl(IFP_WLIF(wds), cmd, (char *)ifr);
	}

done:
	splx(ipl);
	return err_num;
}

/* This our private ioctl handler. The forms and conventions
 * differ greatly from how NetBSD lays out its functions.
 * This keeps the 2 separated
 */
static int
wl_bcm_ioctl(struct ifnet *ifp, struct ifreq *ifr)
{
	wl_ioctl_t ioc;
	wl_if_t *wlif;
	wl_info_t *wl;
	void *buf = NULL;
	int status = 0;
	int bcmerror;

	wlif = WLIF_IFP(ifp);
	ASSERT(wlif->magic == WLIF_MAGIC);

	wl = wlif->wl;
	ASSERT(wl->magic == WLINFO_MAGIC);

	if (WLIF_WDS(wlif) && (wlif->wlcif == NULL)) {
		bcmerror = BCME_ERROR;
		goto done1;
	}

	if (copyin(ifr->ifr_data, &ioc, sizeof(wl_ioctl_t))) {
		bcmerror = BCME_BADADDR;
		goto done1;
	}

	if (ioc.buf) {
		if (!(buf = (void *) MALLOC(wl->osh, MAX(ioc.len, WLC_IOCTL_MAXLEN)))) {
			bcmerror = BCME_NORESOURCE;
			goto done1;
		}

		if (copyin(ioc.buf, buf, ioc.len)) {
			bcmerror = BCME_BADADDR;
			goto done;
		}
	}

	WL_INFORM(("%s: wl_bcm_ioctl: ioc.cmd 0x%x\n", ifp->if_xname, (uint)ioc.cmd));
	bcmerror = wlc_ioctl(wl->wlc, ioc.cmd, buf, ioc.len, wlif->wlcif);

done:
	if (ioc.buf && (ioc.buf != buf)) {
		if (copyout(buf, ioc.buf, ioc.len))
			bcmerror = BCME_BADADDR;
		MFREE(wl->osh, buf, MAX(ioc.len, WLC_IOCTL_MAXLEN));
	}
	if (status)
		return status;
done1:
	ASSERT(VALID_BCMERROR(bcmerror));
	if (bcmerror != 0)
		wl->pub->bcmerror = bcmerror;
	return (OSL_ERROR(bcmerror));
}

/* This helper routine is used for common ioctl call with bcmerror conversion
 * For kernel system calls wl_bcm_ioctl cannot be used due to copyin/copyout
 */
static int
wl_wlc_ioctl(struct ifnet *ifp, int cmd, void *arg, int len)
{
	wl_if_t *wlif;
	wl_info_t *wl;
	int bcmerror;

	wlif = WLIF_IFP(ifp);
	wl = wlif->wl;

	WL_INFORM(("%s %d cmd:%d\n", __FUNCTION__, __LINE__, cmd));

	if (WLIF_WDS(wlif) && (wlif->wlcif == NULL))
		return (OSL_ERROR(BCME_ERROR));

	bcmerror = wlc_ioctl(wl->wlc, cmd, arg, len, wlif->wlcif);

	ASSERT(VALID_BCMERROR(bcmerror));
	if (bcmerror != 0) {
		wl->pub->bcmerror = bcmerror;
	}
	return (OSL_ERROR(bcmerror));
}

/* Helper functions to enable locking w/o repeating lot of code */
/* Also convert driver errors to OS errors */
static int
wl_iovar_op(wl_info_t *wl, const char *name,
            void *params, int p_len, void *arg, int len,
            bool set, struct wl_if *wlif, struct wlc_if *wlcif)
{
	int bcmerror;

	/* WDS driver state cleared, os interface delete pending */
	if ((wlif != NULL) && WLIF_WDS(wlif) && (wlif->wlcif == NULL))
		return OSL_ERROR(BCME_ERROR);

	bcmerror = wlc_iovar_op(wl->wlc, name,
	                        params, p_len, arg, len, set, wlcif);
	if (bcmerror) {
		wl->pub->bcmerror = bcmerror;
		bcmerror = OSL_ERROR(bcmerror);
	}
	return bcmerror;
}

static INLINE int
wl_iovar_setint(wl_info_t *wl, const char *name, int arg, struct wl_if *wlif,
                struct wlc_if *wlcif)
{
	int bcmerror;

	/* WDS driver state cleared, os interface delete pending */
	if ((wlif != NULL) && WLIF_WDS(wlif) && (wlif->wlcif == NULL))
		return OSL_ERROR(BCME_ERROR);

	bcmerror = wlc_iovar_op(wl->wlc, name, NULL, 0, &arg, sizeof(int),
	                        IOV_SET, wlcif);
	if (bcmerror) {
		wl->pub->bcmerror = bcmerror;
		bcmerror = OSL_ERROR(bcmerror);
	}
	return bcmerror;
}

static INLINE int
wl_iovar_getint(wl_info_t *wl, const char *name, int *arg, struct wl_if *wlif,
                struct wlc_if *wlcif)
{
	int bcmerror;

	/* WDS driver state cleared, os interface delete pending */
	if ((wlif != NULL) && WLIF_WDS(wlif) && (wlif->wlcif == NULL))
		return OSL_ERROR(BCME_ERROR);

	bcmerror = wlc_iovar_op(wl->wlc, name, NULL, 0, arg, sizeof(int),
	                        IOV_GET, wlcif);
	if (bcmerror) {
		wl->pub->bcmerror = bcmerror;
		bcmerror = OSL_ERROR(bcmerror);
	}

	return bcmerror;
}
/* Net80211 API */

/* IEEE80211 IOCTL handler */
static int
wl_ieee80211_ioctl(wl_info_t *wl, wl_if_t *wlif, struct ifnet *ifp, u_long cmd, char * data)
{
	int error = 0;

	switch (cmd) {
	case SIOCS80211:
		error = wl_ieee80211_ioctl_set(wlif, ifp, (struct ieee80211req *)data);
		break;
	case SIOCG80211:
		error = wl_ieee80211_ioctl_get(wlif, ifp, (struct ieee80211req *)data);
		break;
	case SIOCG80211STATS:
		break;
#if __NetBSD_Version__ >= 600000000
	case SIOCINITIFADDR:
#endif
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
	}
	return error;
}

/*
 * Note that both ifmedia_ioctl() and ether_ioctl() have to be
 * called under splnet().
 */
static int
#if __NetBSD_Version__ >= 500000003
wl_ioctl(struct ifnet *ifp, u_long cmd, void * data)
#else
wl_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
#endif
{
	struct ifreq *ifr = (struct ifreq *)data;
	wl_if_t	*wlif;
	int ipl;
	int error = 0;
	wl_info_t *wl;

	wlif = WLIF_IFP(ifp);

	WL_INFORM(("%s: 1 wl_ioctl: cmd 0x%x\n", ifp->if_xname, (uint)cmd));
	/* Check to see if we got an ioctl not meant for us */
	if (!wlif)
		return EINVAL;

	if (wlif->magic != WLIF_MAGIC)
		return EINVAL;

	wl = wlif->wl;

	WL_INFORM(("%s: wl_ioctl: cmd 0x%x\n", ifp->if_xname, (uint)cmd));

	ipl = splnet();

	switch (cmd) {
#if defined(__mips__)
	case SIOCGIFHWADDR:
		/* Get hardware (MAC) address */
		memcpy(((struct sockaddr *)&ifr->ifr_data)->sa_data,
			LLADDR(ifp->if_sadl), ETHER_ADDR_LEN);
		break;
	case SIOCSIFHWADDR:
		/* Set hardware (MAC) address */
		memcpy(LLADDR(ifp->if_sadl),
		       ((struct sockaddr *)&ifr->ifr_data)->sa_data, ETHER_ADDR_LEN);
	case SIOCSIFPHYADDR:
		/* if (wlif == wl->base_if) */
			error = wl_set_mac_address(ifp, cmd, (struct ifaliasreq *)data);
		break;
#endif /* __mips__ */

	case SIOCDEVPRIVATE: /* Both VAP and base ? */
		error = wl_bcm_ioctl(ifp, ifr);
		break;

	case SIOCSIFMEDIA: /* Both VAP and base ? */
		/* WDSVAP: Don't allow media to be changed */
		if (WLIF_WDS(wlif))
			break;
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &wlif->im, cmd);
		break;
	default:
		error = wl_ieee80211_ioctl(wl, wlif, ifp, cmd, data);
		break;
	} /* end of switch */

	splx(ipl);

	return (error);
}

#if defined(__mips__)
/*
 * Helper function to set Ethernet address.
 */
static int
wl_set_mac_address(struct ifnet *ifp, u_long cmd, struct ifaliasreq *ifra)
{
	struct sockaddr *sa = (struct sockaddr *)&ifra->ifra_addr;
	wl_info_t *wl;
	wl_if_t *wlif;

	wlif = WLIF_IFP(ifp);
	wl = wlif->wl;
	ASSERT(wl->magic == WLINFO_MAGIC);

	if (wl->pub->up)
		return EBUSY;

	if (sa->sa_family != AF_LINK)
		return EINVAL;

	WL_TRACE(("wl%d: wl_set_mac_address\n", wl->pub->unit));

	bcopy(sa->sa_data, LLADDR(ifp->if_sadl), ETHER_ADDR_LEN);

	return wl_iovar_op(wl, "cur_etheraddr", NULL, 0,
	                   sa->sa_data, ETHER_ADDR_LEN, IOV_SET, wlif, wlif->wlcif);
}
#endif /* __mips__ */

/*
 * wl_open() called by ifconfig() when the interface goes up,
 */
static int
wl_open(struct ifnet *ifp)
{
	wl_info_t *wl;
	int error;

	wl = WL_INFO_GET(ifp);
	ASSERT(wl->magic == WLINFO_MAGIC);

	WL_TRACE(("wl%d: wl_open\n", wl->pub->unit));

	WL_LOCK(wl);

	WL_ERROR(("wl%d: (%s): wl_open() -> wl_up()\n",
		wl->pub->unit, ifp->if_xname));

	error = wl_up(wl);

#if NOT_YET
	/* Disable promisc mode as it causes all inter-BSS sta to sta traffic
	 * to get sent up into the bridge and forwarded in the BSS.
	 */
	if (!error) {
		error = wlc_set(wl->wlc,
			WLC_SET_PROMISC, (ifp->if_flags & IFF_PROMISC));
	}
#endif
	WL_UNLOCK(wl);

	return (error ? ENODEV: 0);
}

/* Shutting down 1 interface shut down all of them */
static void
wl_close(struct ifnet *ifp, int disable)
{
	wl_info_t *wl;
	wl = WL_INFO_GET(ifp);

	ASSERT(wl->magic == WLINFO_MAGIC);

	WL_TRACE(("wl%d: wl_close\n", wl->pub->unit));

	/* Do not call wl_down() directly from ifconfig()
	 * as it has to be protected by WL_SOFTLOCK(). The wlc core
	 * calls wl_down from within the perimeter lock so
	 * calling WL_SOFTLOCK() in wl_down will result in deadlock
	 * on a SMP system.
	 */

	WL_LOCK(wl);
	WL_ERROR(("wl%d (%s): wl_close() -> wl_down()\n",
		wl->pub->unit, ifp->if_xname));

	wl_down(wl);
	WL_UNLOCK(wl);

}

void
wl_dump_ver(wl_info_t *wl, struct bcmstrbuf *b)
{
	bcm_bprintf(b, "wl%d: %s %s version %s\n", wl->pub->unit,
	            __DATE__, __TIME__, EPI_VERSION_STR);
}


static void
wl_link_up(wl_info_t *wl)
{
	ASSERT(wl->magic == WLINFO_MAGIC);
	WL_ERROR(("wl%d: link up\n", wl->pub->unit));
}

static void
wl_link_down(wl_info_t *wl)
{
	ASSERT(wl->magic == WLINFO_MAGIC);
	WL_ERROR(("wl%d: link down\n", wl->pub->unit));
}

static void
wl_mic_error(wl_info_t *wl, wlc_bsscfg_t *cfg, struct ether_addr *ea, bool group, bool flush_txq)
{
	ASSERT(wl->magic == WLINFO_MAGIC);

	WL_WSEC(("wl%d: mic error using %s key\n", wl->pub->unit,
		(group) ? "group" : "pairwise"));

#if defined(BCMSUP_PSK) && defined(STA)
	if (wlc_sup_mic_error(cfg, group))
		return;
#endif /* defined(BCMSUP_PSK) && defined(STA) */

}

/* Look for wlif based on ifname */
static wl_if_t *
wl_wlif_lookup(wl_info_t *wl, char *ifname)
{
	wl_if_t *wlif;
	int count;

	if (!ifname)
		return NULL;

	for (wlif = wl->iflist, count = 0; count < wl->num_if; count++, wlif = wlif->next)
		if (strncmp(ifname, (IFP_WLIF(wlif))->if_xname, strlen(ifname)) == 0)
			return wlif;

	return NULL;
}

void
wl_event(wl_info_t *wl, char *ifname, wlc_event_t *e)
{

	struct ifnet *ifp = NULL;
	struct wl_if *wlif = wl_wlif_lookup(wl, ifname);

	if (wlif == NULL)
		wlif = wl->iflist;

	ifp = IFP_WLIF(wlif);

	WL_INFORM(("wl%d: event:%d ifname:%s vapidx:%d ifname:%s \n", wl->pub->unit,
	           e->event.event_type, e->event.ifname, wlif->vapidx, ifname));

	wl_ieee80211_event(ifp, e, WLIF_AP_MODE(wl, wlif));

	switch (e->event.event_type) {
	case WLC_E_LINK:
	case WLC_E_NDIS_LINK:
		if (e->event.flags & WLC_EVENT_MSG_LINK)
			wl_link_up(wl);
		else
			wl_link_down(wl);
		break;
	case WLC_E_MIC_ERROR: {
		wlc_bsscfg_t *cfg = wlc_bsscfg_find_by_wlcif(wl->wlc, NULL);
		if (cfg == NULL || e->event.bsscfgidx != WLC_BSSCFG_IDX(cfg))
			break;
		wl_mic_error(wl, cfg, e->addr, e->event.flags & WLC_EVENT_MSG_GROUP,
		             e->event.flags & WLC_EVENT_MSG_FLUSHTXQ);
		break;
	}
	case WLC_E_SCAN_COMPLETE:
		wl->scandone = TRUE;
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

uint
wl_reset(wl_info_t *wl)
{
	WL_TRACE(("wl%d: wl_reset\n", wl->pub->unit));

	ASSERT(wl->magic == WLINFO_MAGIC);

	wlc_reset(wl->wlc);

	return (0);
}

void
wl_init(wl_info_t *wl)
{
	WL_TRACE(("wl%d: wl_init\n", wl->pub->unit));

	ASSERT(wl->magic == WLINFO_MAGIC);

	wl_reset(wl);
	wlc_init(wl->wlc);
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
	wlc_intrson(wl->wlc);
}

uint32
wl_intrsoff(wl_info_t *wl)
{
	ASSERT(wl->magic == WLINFO_MAGIC);
	return wlc_intrsoff(wl->wlc);
}

void
wl_intrsrestore(wl_info_t *wl, uint32 macintmask)
{
	ASSERT(wl->magic == WLINFO_MAGIC);
	wlc_intrsrestore(wl->wlc, macintmask);
}

int
wl_up(wl_info_t *wl)
{
	int error = 0;
	int ipl;
	int count;
	wl_if_t	*wlif = wl->iflist;
	struct ifnet *ifp;

	WL_TRACE(("wl%d: wl_up\n", wl->pub->unit));

	ASSERT(wl->magic == WLINFO_MAGIC);

	if (wl->pub->up)
		return (0);

	error = wlc_up(wl->wlc);

	if (!error) {
		/* Bringup interfaces */
		for (count = 0; count < wl->num_if; count++) {
			ifp = IFP_WLIF(wlif);
			ipl = splnet();
			ifp->if_flags |= (IFF_RUNNING|IFF_UP);
			splx(ipl);
			/* wake (not just start) all interfaces */
			WL_LOCK(wl);
			wl_txflowcontrol(wl, wlif, OFF, ALLPRIO);
			WL_UNLOCK(wl);

			wlif = wlif->next;
		}
		ASSERT(wlif == NULL);
	}

	return (error);
}

/* Get AP mode of specific interface */
static bool
wl_wlif_apmode(wl_info_t *wl, wl_if_t *wlif)
{
	int ap_mode;
	if ((wl_iovar_getint(wl, "ap", &ap_mode, wlif, wlif->wlcif)))
		return FALSE;

	return ap_mode;
}

/* Bringup just the VAP */
static int
wl_vap_open(struct ifnet *ifp)
{
	wl_info_t *wl;
	int error;
	int ipl = 0;
	wl_if_t *wlif;

	wlif = WLIF_IFP(ifp);
	wl = wlif->wl;
	WL_LOCK(wl);

	WL_ERROR(("wl%d: (%s): wl_vap_open() -> wl_wlif_up()\n",
		wl->pub->unit, ifp->if_xname));

	ASSERT(wlif != wl->base_if);
	/* Bring up the h/w i/f if not up */
	if (!wl->pub->up) {
		error = wlc_up(wl->wlc);
		if (!error) {
			ifp = IFP_WLIF(wl->base_if);
			ipl = splnet();
			ifp->if_flags |= (IFF_RUNNING|IFF_UP);
			splx(ipl);
		}
	}

	error = wl_wlif_up(wl, wlif);

	/* need to be up to set radar thresholds */
	/* dont propagate error since
	 * we set this on both 2.4 and 5ghz and 2.4 radio errors out
	 */
	if (!error)
		wl_set_radarthrs(wl);

#if NOT_YET
	/* Disable promisc mode as it causes all inter-BSS sta to sta traffic
	 * to get sent up into the bridge and forwarded in the BSS.
	 */
	if (!error) {
		error = wlc_set(wl->wlc,
			WLC_SET_PROMISC, (ifp->if_flags & IFF_PROMISC));
	}
#endif

	WL_UNLOCK(wl);

	return (error ? ENODEV: 0);
}

/* Shutting down only the VAP */
static void
wl_vap_close(struct ifnet *ifp, int disable)
{
	wl_info_t *wl;
	wl_if_t *wlif;
	int ipl, count, running = 0;
	struct ifnet *lifp;

	wlif = WLIF_IFP(ifp);
	wl = wlif->wl;
	WL_LOCK(wl);
	WL_ERROR(("wl%d (%s): wl_vap_close() -> wl_wlif_down()\n",
		wl->pub->unit, ifp->if_xname));

	ASSERT(wlif != wl->base_if);
	wl_wlif_down(wl, wlif);
	WL_UNLOCK(wl);

	/* check if this is the last interface to go down */
	ipl = splnet();
	wlif = wl->iflist;
	for (count = 0; count < wl->num_if; count++) {
		lifp = IFP_WLIF(wlif);
		if (lifp->if_flags & (IFF_UP | IFF_RUNNING)) {
			running++;
		}
		wlif = wlif->next;
	}
	/* if only hw interface is left then bring wl down */
	if (running == 1) {
		wl_down(wl);
	}
	splx(ipl);
}

/* Bring specific interface down */
static void
wl_wlif_down(wl_info_t *wl, wl_if_t *wlif)
{
	int ipl;
	struct ifnet *ifp;
	struct {
		int cfg;
		int val;
	} setbuf;

	ASSERT(wlif);
	ifp = IFP_WLIF(wlif);
	ipl = splnet();
	ifp->if_flags &= ~(IFF_UP | IFF_RUNNING);
	splx(ipl);
	WL_STOP_IFQUEUE(ifp, ipl);

	/* WDSVAP: Do nothing for WDS or STA mode */
	if (WLIF_WDS(wlif) || !WLIF_AP_MODE(wl, wlif))
		return;

	setbuf.cfg = wlif->vapidx;
	ASSERT(wlif->vapidx < WLC_MAXBSSCFG);
	setbuf.val = 0;
	wl_iovar_op(wl, "bss", NULL, 0, &setbuf, sizeof(setbuf), IOV_SET, wlif, NULL);
}

/* Bring specific interface up */
static int
wl_wlif_up(wl_info_t *wl, wl_if_t *wlif)
{
	int ipl;
	struct ifnet *ifp;
	struct {
		int cfg;
		int val;
	} setbuf;

	ASSERT(wlif);
	ifp = IFP_WLIF(wlif);
	ipl = splnet();
	ifp->if_flags |= (IFF_RUNNING|IFF_UP);
	splx(ipl);
	WL_WAKE_IFQUEUE(ifp, ipl);

	/* WDSVAP: Do nothing for WDS or STA mode */
	if (WLIF_WDS(wlif) || !WLIF_AP_MODE(wl, wlif))
		return 0;

	setbuf.cfg = wlif->vapidx;
	ASSERT(wlif->vapidx < WLC_MAXBSSCFG);
	setbuf.val = 1;
	return (wl_iovar_op(wl, "bss", NULL, 0, &setbuf, sizeof(setbuf), IOV_SET, wlif, NULL));
}

/* wl_down can be called by both the OS and wlc */
void
wl_down(wl_info_t *wl)
{
	wl_if_t	*wlif = wl->iflist;
	int count;
	struct ifnet *ifp;
	int ipl;

	ASSERT(wl->magic == WLINFO_MAGIC);

	/* free assoc sta ie list */
	wlc_free_sta_ie(wlif, NULL, TRUE);

	/* Detach all interfaces from OS */
	for (count = 0; count < wl->num_if; count++) {
		ifp = IFP_WLIF(wlif);
		ipl = splnet();
		ifp->if_flags &= ~(IFF_UP | IFF_RUNNING);
		splx(ipl);
		wlif = wlif->next;
	};
	ASSERT(wlif == NULL);

	WL_ERROR(("wl%d: wl_down\n", wl->pub->unit));

	/* call common down function */
	wl->callbacks = wlc_down(wl->wlc);

	wl->pub->hw_up = FALSE;

	/* wait for any pending callbacks to complete */
	wl_wait_callbacks(wl);
}

void
wl_txflowcontrol(wl_info_t *wl, struct wl_if *wlif, bool state, int prio)
{
	struct ifnet *ifp;
	int ipl, ipl2 = 0;

	ASSERT(wl->magic == WLINFO_MAGIC);
	ASSERT(prio == ALLPRIO);

	WL_INFORM(("%s: txflowcontrol is %s\n",
		__FUNCTION__, (state == ON) ? "ON" : "OFF"));

	if (wlif == NULL)
		wlif = wl->iflist;

	/* Skip the base i/f in VAP case */
	if (wlif == wl->base_if) {
		WL_ERROR(("Skip flowcontrol for base i/f\n"));
		return;
	}

	/* Setting IPL_NET here prevents us from being perempted
	 * if we were called from a DPC
	 */
	ipl2 = splnet();

	ifp = IFP_WLIF(wlif);
	wlif->txflowcontrol = state;

	if (state == ON)
		WL_STOP_IFQUEUE(ifp, ipl);
	else
		WL_WAKE_IFQUEUE(ifp, ipl);

	splx(ipl2);
}

static void
wl_timer(void *data)
{
	wl_timer_t *t;

	t = (wl_timer_t*) data;

	ASSERT(t);
	ASSERT(t->magic == WLTIMER_MAGIC);

	/*
	 * Set the IPl to WL_SOFTINTR
	*/

	WL_LOCK(t->wl);

	if (t->set) {
		t->set = FALSE;
		if (t->periodic)
			wl_add_timer(t->wl, t, t->ms, t->periodic);
		t->fn(t->arg);
	}

	t->wl->callbacks--;

	callout_ack(&t->timer);

	WL_UNLOCK(t->wl);
}

void
wl_add_timer(wl_info_t *wl, wl_timer_t *t, uint ms, int periodic)
{
	ASSERT(wl->magic == WLINFO_MAGIC);
	ASSERT(t->magic == WLTIMER_MAGIC);

	ASSERT(!t->set);

	t->ms = ms;
	t->periodic = (bool) periodic;
	t->set = TRUE;

	wl->callbacks++;
	if (t->ms)
		callout_schedule(&t->timer, t->ms * TICKS_PER_MSEC);
	else
		callout_schedule(&t->timer, 1*TICKS_PER_MSEC);
}

/* Must be called from within perimeter lock */
bool
wl_del_timer(wl_info_t *wl, wl_timer_t *t)
{
	int ipl;
	bool status = TRUE;

	ASSERT(wl->magic == WLINFO_MAGIC);
	ASSERT(t->magic == WLTIMER_MAGIC);

	ipl = splsoftclock();
	if (t->set) {
		t->set = FALSE;

		if (callout_expired(&t->timer))
			status = FALSE;
		else
			if (callout_invoking(&t->timer))
				status = FALSE;
			else {
				callout_stop(&t->timer);
				wl->callbacks--;
			}
	}
	splx(ipl);


	return status;

}

void
wl_free_timer(wl_info_t *wl, wl_timer_t *t)
{
	ASSERT(wl->magic == WLINFO_MAGIC);
	ASSERT(!t->set);
}

wl_timer_t *
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


	callout_init(&t->timer, CALLOUT_MPSAFE);
	callout_setfunc(&t->timer, wl_timer, (void *)t);

	return t;
}

#ifdef AP

/* Return pointer to interface name */
char *
wl_ifname(wl_info_t *wl, wl_if_t *wlif)
{

	if (wlif)
		return (IFP_WLIF(wlif))->if_xname;
	else
		return (IFP_WLIF(wl->iflist))->if_xname;
}

/* Create an interface. Call only from safe time! */
static void
_wl_add_if(void *context)
{
	wl_if_t *wlif = (wl_if_t *)context;

	ASSERT(wlif->magic == WLIF_MAGIC);
	wl_if_attach(wlif);
}

/* Schedule _wl_add_if() to be run at safe time. */
wl_if_t *
wl_add_if(wl_info_t *wl, struct wlc_if *wlcif, uint unit, struct ether_addr *remote)
{
	wl_if_t *wlif;
	bool vap;

	if (remote) unit = wl->wdsvapidx;

	vap = (wl->vap[unit].vap_name != NULL);

	/* Create interface structures but defer attachment */
	if ((wlif = wl_alloc_if(wl, (remote) ? WL_IFTYPE_WDS : WL_IFTYPE_BSS,
	                        unit, wlcif, remote, FALSE, vap)) == NULL)
		return NULL;
	_wl_add_if(wlif);

	return wlif;
}

/* Remove an interface. Call only from safe time! */
static void
_wl_del_if(void *context)
{
	wl_if_t *wlif = (wl_if_t *)context;

	wl_free_if(wlif->wl, wlif);
}

static bool
wl_wlif_valid(wl_info_t *wl, wl_if_t *wlif)
{
	wl_if_t *p;

	ASSERT(wlif != NULL);

	for (p = wl->iflist; p != NULL; p = p->next) {
		if (p == wlif)
			break;
	}

	return (p != NULL);
}

/* Schedule _wl_del_if() to be run at safe time. */
void
wl_del_if(wl_info_t *wl, wl_if_t *wlif)
{
	/* Make sure wlif is still valid. It is possible that wlif is
	 * removed by ifconfig destroy in response to disassoc indication
	 * where we have dwds scb but no wlif.
	 */
	if (!wl_wlif_valid(wl, wlif))
		return;

	wlif->wlcif = NULL;

	_wl_del_if(wlif);
	return;
}

#ifdef DWDS
/* Postpone interface delete until we get called with dwds config delete iovar */
void
wl_dwds_del_if(wl_info_t *wl, wl_if_t *wlif, bool force)
{
	/* Make sure wlif is still valid. It is possible that wlif is
	 * removed by ifconfig destroy in response to disassoc indication
	 * where we have dwds scb but no wlif.
	 */
	if (!wl_wlif_valid(wl, wlif))
		return;

	/* Set wlcif handle to NULL */
	wlif->wlcif = NULL;
	if (!force)
		return;

	/* Really delete the interface */
	_wl_del_if(wlif);

	return;
}
#endif /* DWDS */

#endif /* #ifdef AP */

/* Turn monitor mode on or off . This function assumes
 * it is called within the perimeterlock
*/
void
wl_set_monitor(wl_info_t *wl, int val)
{
#ifdef WL_MONITOR
	WL_TRACE(("wl%d: wl_set_monitor: val %d\n", wl->pub->unit, val));

	if (val)
		wl->monitor = IFP_WLIF(wl->iflist);
	else
		wl->monitor = NULL;
#endif /* WL_MONITOR */
}

/* Sends encasulated packets up the monitor interface */
void
wl_monitor(wl_info_t *wl, wl_rxsts_t *rxsts, void *p)
{
#ifdef WL_MONITOR
	struct mbuf *m0 = (struct mbuf *)p;
	p80211msg_t *phdr;
	struct mbuf *m = NULL;
	uint len;
	int ipl;
	uchar *pdata;
	struct ifnet *ifp = wl->monitor;

	WL_TRACE(("wl%d: wl_monitor\n", wl->pub->unit));

	ASSERT(wl->magic == WLINFO_MAGIC);

	if (!ifp)
		return;

	if (ifp->if_bpf == NULL)
		return;

	len = sizeof(p80211msg_t) + m0->m_len - D11_PHY_HDR_LEN;

	ASSERT(len < MCLBYTES);

	ipl = splnet();
	MGETHDR(m, M_NOWAIT, MT_HEADER);
	if (m == NULL)
		return;
	MCLGET(m, M_NOWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_free(m);
		return;
	}
	splx(ipl);

	phdr = (p80211msg_t*)m->m_data;

	/* Initialize the message members */
	phdr->msgcode = WL_MON_FRAME;
	phdr->msglen = sizeof(p80211msg_t);
	strcpy(phdr->devname, ifp->if_xname);

	phdr->hosttime.did = WL_MON_FRAME_HOSTTIME;
	phdr->hosttime.status = P80211ITEM_OK;
	phdr->hosttime.len = 4;
	phdr->hosttime.data = mono_time.tv_usec;

	phdr->channel.did = WL_MON_FRAME_CHANNEL;
	phdr->channel.status = P80211ITEM_NO_VALUE;
	phdr->channel.len = 4;
	phdr->channel.data = 0;

	phdr->signal.did = WL_MON_FRAME_SIGNAL;
	phdr->signal.status = P80211ITEM_NO_VALUE;
	phdr->signal.len = 4;
	/* ??? to add MIMO_MM and MIMO_GF */
	phdr->signal.data = (rxsts->preamble == WL_RXS_PREAMBLE_SHORT) ? 1 : 0;

	phdr->noise.did = WL_MON_FRAME_NOISE;
	phdr->noise.status = P80211ITEM_NO_VALUE;
	phdr->noise.len = 4;
	phdr->noise.data = 0;

	phdr->rate.did = WL_MON_FRAME_RATE;
	phdr->rate.status = P80211ITEM_OK;
	phdr->rate.len = 4;
	phdr->rate.data = rxsts->datarate;

	phdr->istx.did = WL_MON_FRAME_ISTX;
	phdr->istx.status = P80211ITEM_NO_VALUE;
	phdr->istx.len = 4;
	phdr->istx.data = 0;

	phdr->mactime.did = WL_MON_FRAME_MACTIME;
	phdr->mactime.status = P80211ITEM_OK;
	phdr->mactime.len = 4;
	phdr->mactime.data = rxsts->mactime;

	phdr->rssi.did = WL_MON_FRAME_RSSI;
	phdr->rssi.status = P80211ITEM_OK;
	phdr->rssi.len = 4;
	phdr->rssi.data = rxsts->signal;		/* to dbm */

	phdr->sq.did = WL_MON_FRAME_SQ;
	phdr->sq.status = P80211ITEM_OK;
	phdr->sq.len = 4;
	phdr->sq.data = rxsts->sq;

	phdr->frmlen.did = WL_MON_FRAME_FRMLEN;
	phdr->frmlen.status = P80211ITEM_OK;
	phdr->frmlen.status = P80211ITEM_OK;
	phdr->frmlen.len = 4;
	phdr->frmlen.data = rxsts->pktlength;

	pdata = m->m_data + sizeof(p80211msg_t);
	bcopy(m0->m_data + D11_PHY_HDR_LEN, pdata, m0->m_len - D11_PHY_HDR_LEN);

	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = m->m_len;
	m->m_pkthdr.csum_flags = 0;

	ipl = splnet();

	BPF_MTAP(ifp->if_bpf, m);

	/* Free the packet. BPF does not do this automatically */
	m_freem(m);
	splx(ipl);

#endif /* WL_MONITOR */
}

/* Callback counter will be decreased by dpcs or timer */
static void
wl_wait_callbacks(wl_info_t *wl)
{

	ASSERT(wl->magic == WLINFO_MAGIC);

	if (wl->callbacks > 0) {
		WL_TRACE(("wl_wait_callbacks, starting SPINWAIT 20ms\n"));
		WL_UNLOCK(wl);
		SPINWAIT((wl->callbacks > 0), 20 * 1000);
		WL_LOCK(wl);
	}
}


/* Find empty spot for BSSCFG for VAP. The algo is:
 * If this is STA, see if idx 0 is empty. If yes, return, else fail
 * If this is AP, start with idx 1 to (maxcfg - 1). If all are taken, then try 0
 * If this is WDS, then start with idx from  maxcfg to (maxvap -1)
 */
static int
wl_get_free_vapidx(wl_info_t *wl, u_int16_t icp_opmode)
{
	int idx;
	switch (icp_opmode) {
	case IEEE80211_M_STA:
		if (wl->vap[0].vap_name != NULL)
			return -1;
		else
			return 0;
	case IEEE80211_M_HOSTAP:
		for (idx = 0; idx < WLC_MAXBSSCFG; idx++)
			if (wl->vap[idx].vap_name == NULL)
				return idx;
		if (idx == WLC_MAXBSSCFG && (wl->vap[0].vap_name == NULL))
			return 0;
	case IEEE80211_M_WDS: /* WDSVAP */
		for (idx = WLC_MAXBSSCFG; idx < WL_MAXVAP; idx++)
			if (wl->vap[idx].vap_name == NULL)
				return idx;
	}
	return -1;
}

/* Create VAP : SIOC80211IFCREATE2 */
static int
#if __NetBSD_Version__ >= 500000003
wl_ieee80211_ifcreate(struct if_clone *ifc, int unit, const void *params)
#else
wl_ieee80211_ifcreate(struct if_clone *ifc, int unit, caddr_t params)
#endif
{
	struct ieee80211_clone_params cp;
	struct ifnet *ifp;
	int error;
	int vapidx;
	wl_if_t *wlif;
	wl_info_t *wl;
	int ap;
	int net_ipl;

	error = copyin(params, &cp, sizeof(cp));
	if (error)
		return error;
	ifp = ifunit(cp.icp_parent);
	if (ifp == NULL)
		return ENXIO;
	WL_ERROR(("%s parent:%s mode: 0x%x flag:0x%x  if_xname:%s ifc name:%s unit:%d if_type:%d\n",
		__FUNCTION__, cp.icp_parent, cp.icp_opmode, cp.icp_flags, ifp->if_xname,
		ifc->ifc_name, unit, ifp->if_type));
	if (ifp->if_type != IFT_IEEE80211) {
		WL_ERROR(("%s: reject, not an 802.11 device %s\n", __func__, ifp->if_xname));
		return EINVAL;
	}
	if (cp.icp_opmode >= IEEE80211_OPMODE_MAX) {
		WL_ERROR(("%s: %s: invalid opmode %d\n",
		    __func__, ifp->if_xname, cp.icp_opmode));
		return EINVAL;
	}

	wlif = WLIF_IFP(ifp);
	wl = wlif->wl;

	/* Make sure that this gets executed on base i/f only */
	ASSERT(wlif == wl->base_if);

	WL_ERROR(("%s parent:%s mode: 0x%x flag:0x%x  if_xname:%s ifc name:%s unit:%d \n",
		__FUNCTION__, cp.icp_parent, cp.icp_opmode, cp.icp_flags, ifp->if_xname,
		ifc->ifc_name, unit));

	/* wlanconfig sets the IEEE80211_CLONE_BSSID flag, just ignore it */
	/* cp.icp_flags &= ~IEEE80211_CLONE_BSSID; */

	/* Sanity check the icp_opmode and flags */
	/* Support only MBSS => !CLONE_BSSID
	 * Support only AP, STA, and WDS modes
	 */
	if (!((cp.icp_opmode == IEEE80211_M_STA) || (cp.icp_opmode == IEEE80211_M_HOSTAP) ||
	       (cp.icp_opmode == IEEE80211_M_WDS)))
		return EINVAL;

	/* Find if i/f already exists!. If yes, then reject */
	net_ipl = splnet();
	vapidx = wl_get_free_vapidx(wl, cp.icp_opmode);

	if (vapidx == -1) {
		splx(net_ipl);
		return EIO; /* ieee80211_wireless.c seems to be returning this */
	}

	if (!(wl->vap[vapidx].vap_name = MALLOC(wl->osh, IFNAMSIZ +1))) {
		splx(net_ipl);
		return ENOMEM;
	}

	snprintf(wl->vap[vapidx].vap_name, IFNAMSIZ, "%s%d", ifc->ifc_name, unit);
	wl->num_vap++;

	switch (cp.icp_opmode) {
	case IEEE80211_M_HOSTAP: {
		struct {
			int cfg;
			int val;
			struct ether_addr ea;
		} setbuf;

		ASSERT(vapidx < WLC_MAXBSSCFG);

		memset(&setbuf, 0, sizeof(setbuf));
		setbuf.cfg = vapidx;
		setbuf.val = 0;
		if (vapidx == 0) {
			wlc_bsscfg_t *cfg;

			/* do this just once for first ap vap */
			wl_down(wl);
			wl_iovar_setint(wl, "apsta", 0, wl->base_if, wl->base_if->wlcif);

			ap = 1;
			wl_wlc_ioctl(ifp, WLC_SET_AP, &ap, sizeof(ap));

			/* set mpc off */
			wl_iovar_setint(wl, "mpc", 0, wl->base_if, wl->base_if->wlcif);

			cfg = wlc_bsscfg_primary(wl->wlc);
			cfg->flags |= WLC_BSSCFG_USR_RADAR_CHAN_SELECT;

			/* alloc a new wlif for primary if and update primary
			 * wlcif with this new wlif
			 */
			if ((wlif->wlcif->wlif =
				wl_add_if(wl, wlif->wlcif, vapidx, NULL)) == NULL) {
				error = EINVAL;
				goto fail;
			}
		} else {
			if ((error = wl_iovar_op(wl, "ap_bss", NULL, 0, &setbuf, sizeof(setbuf),
				IOV_SET, wlif, NULL)))
				goto fail;
		}
		break;
		}
	case IEEE80211_M_STA: {
		ASSERT(vapidx == 0);
		wl_down(wl);

		ap = 0;
		wl_wlc_ioctl(ifp, WLC_SET_AP, &ap, sizeof(ap));

		wl_iovar_setint(wl, "apsta", 1, wl->base_if, wl->base_if->wlcif);

		if ((wlif->wlcif->wlif = wl_add_if(wl, wlif->wlcif, vapidx, NULL)) == NULL) {
			WL_ERROR(("%s: Add if failed!! \n",
				__FUNCTION__));
			error = EINVAL;
			goto fail;
		}
		break;
		}
	case IEEE80211_M_WDS: { /* WDSVAP */
		wlc_dwds_config_t dwds;

		ASSERT(vapidx >= WLC_MAXBSSCFG);

		/* WDSVAP: Assumption.. remote mac addr is in icp_macaddr */
		dwds.mode = 0; /* AP interface */
		dwds.enable = 1; /* enable */
		bcopy(&cp.icp_bssid, &dwds.ea, sizeof(struct ether_addr));
		wl->wdsvapidx = vapidx;
		if ((error = wl_iovar_op(wl, "dwds_config", NULL, 0, &dwds, sizeof(dwds),
			IOV_SET, wlif, wlif->wlcif)))
			/* this can fail b/c no node exists if we fail dont call wl_del_if
			 * since in the wds case none of this stuff gets crated if dwds_config fails
			 */
			goto fail2;
		}
	} /* End of switch */

	splx(net_ipl);
	return 0;
fail:
	if (wl->vap[vapidx].wlif) {
		wl_del_if(wl, wl->vap[vapidx].wlif);
		wl->vap[vapidx].wlif = NULL;
	}
fail2:
	if (wl->vap[vapidx].vap_name) {
		MFREE(wl->osh, wl->vap[vapidx].vap_name, IFNAMSIZ +1);
		wl->vap[vapidx].vap_name = NULL;
	}
	wl->num_vap--;
	splx(net_ipl);
	return error;
}

/* In this just detach from the upper layer. Let the lower layer interface live, except for WDS */
static void
_wl_wlif_detach(void *context)
{
	wl_if_t *wlif = (wl_if_t *)context;
	int net_ipl;
	struct ifnet *ifp = IFP_WLIF(wlif);
	wl_info_t *wl = wlif->wl;

	WL_LOCK(wl);
	if (wlif->attached) {
		net_ipl = splnet();
		setappie(wlif, &wlif->appie_beacon, &wlif->appie_beacon_len, NULL, 0);
		setappie(wlif, &wlif->appie_probereq, &wlif->appie_probereq_len, NULL, 0);
		setappie(wlif, &wlif->appie_proberesp, &wlif->appie_proberesp_len, NULL, 0);
		setappie(wlif, &wlif->appie_assocreq, &wlif->appie_assocreq_len, NULL, 0);
		setappie(wlif, &wlif->appie_assocresp, &wlif->appie_assocresp_len, NULL, 0);
		/* FREEAPPIE(wlif->appie_wpa); */
		setappie(wlif, &wlif->appie_beacon_wps, &wlif->appie_beacon_wps_len, NULL, 0);
		setappie(wlif, &wlif->appie_probereq_wps, &wlif->appie_probereq_wps_len, NULL, 0);
		setappie(wlif, &wlif->appie_proberesp_wps, &wlif->appie_proberesp_wps_len, NULL, 0);
		setappie(wlif, &wlif->appie_beacon_dwds, &wlif->appie_beacon_dwds_len, NULL, 0);
		setappie(wlif, &wlif->appie_proberesp_dwds, &wlif->appie_proberesp_dwds_len,
			NULL, 0);
		setappie(wlif, &wlif->appie_assocreq_dwds, &wlif->appie_assocreq_dwds_len, NULL, 0);
		setappie(wlif, &wlif->appie_assocresp_dwds, &wlif->appie_assocresp_dwds_len,
			NULL, 0);
		setappie(wlif, &wlif->appie_beacon_awp, &wlif->appie_beacon_awp_len, NULL, 0);
		setappie(wlif, &wlif->appie_probereq_awp, &wlif->appie_probereq_awp_len, NULL, 0);
		setappie(wlif, &wlif->appie_proberesp_awp, &wlif->appie_proberesp_awp_len, NULL, 0);
		ether_ifdetach(ifp);
		if_detach(ifp);
		ifmedia_delete_instance(&wlif->im, IFM_INST_ANY);
		wlif->attached = FALSE;
		splx(net_ipl);
	}
	if (wl->vap[wlif->vapidx].vap_name) {
		MFREE(wl->osh, wl->vap[wlif->vapidx].vap_name, IFNAMSIZ +1);
		wl->vap[wlif->vapidx].vap_name = NULL;
	}
	wl->num_vap--;

	WL_UNLOCK(wl);
}

/* Destroy the VAP by shutting down and then destroying the upper edge to the OS */
static int
wl_ieee80211_ifdestroy(struct ifnet*ifp)
{
	wl_if_t *wlif;
	wl_info_t *wl;
	wlc_bsscfg_t *cfg;

	wlif = WLIF_IFP(ifp);
	ASSERT(wlif->magic == WLIF_MAGIC);

	wl = wlif->wl;
	ASSERT(wl->magic == WLINFO_MAGIC);

	WL_LOCK(wl);

	wl_wlif_down(wl, wlif);
	_wl_wlif_detach(wlif);

	/* WDSVAP: Delete WDS i/f completely */
	if (WLIF_WDS(wlif)) {
		wlc_dwds_config_t dwds;
		bzero(&dwds, sizeof(dwds));
		/* just copy peer mac, mode is zero for disable */
		bcopy(&wlif->remote, &dwds.ea, sizeof(struct ether_addr));
		if (wl_iovar_op(wl, "dwds_config", NULL, 0, &dwds, sizeof(dwds),
		                IOV_SET, wlif, wlif->wlcif)) {
			wl_dwds_del_if(wl, wlif, TRUE);
		}
	} else if (wlif->wlcif != NULL) {
		cfg = wlc_bsscfg_find_by_wlcif(wl->wlc, wlif->wlcif);
		ASSERT(cfg != NULL);
		if (wlc_bsscfg_primary(wl->wlc) != cfg)
			wlc_bsscfg_free(wl->wlc, cfg);
		else {
			struct ether_addr ea = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
			/* Clear the neighbor report list for primary interface on
			 * a destroy.
			 */
			wl_iovar_op(wl, "rrm_nbr_del_nbr", NULL, 0, &ea, sizeof(ea),
			            IOV_SET, wlif, wlif->wlcif);
			wl_del_if(wl, wlif);
		}
	}

	WL_UNLOCK(wl);

	return 0;
}

int
wl_osl_pcie_rc(struct wl_info *wl, uint op, int param)
{

	return 0;
}

static int
wl_stainfo2ieee_stainfo(wl_if_t *wlif, struct ifnet *ifp, sta_info_t *sta,
	struct ieee80211req_sta_info *ieeesta)
{
	int i;
	int phynoise = 0;

	if (sta->flags & WL_STA_AUTHO)
		ieeesta->isi_state |= IEEE80211_NODE_AUTH;
	if (sta->flags & WL_STA_WME)
		ieeesta->isi_state |= IEEE80211_NODE_QOS;
	if (!(sta->flags & WL_STA_NONERP))
		ieeesta->isi_state |= IEEE80211_NODE_ERP;
	if (sta->flags & WL_STA_PS)
		ieeesta->isi_state |= IEEE80211_NODE_PWR_MGT;
	if (sta->flags & WL_STA_N_CAP)
		ieeesta->isi_state |= IEEE80211_NODE_HT;
	/* ht compat */
	/* wps */
	/* tsn */
	if (sta->flags & WL_STA_AMPDU_CAP)
		ieeesta->isi_state |= IEEE80211_NODE_AMPDU;
	if (sta->flags & WL_STA_AMSDU_CAP)
		ieeesta->isi_state |= IEEE80211_NODE_AMSDU_TX;
	if (sta->flags & WL_STA_MIMO_PS)
		ieeesta->isi_state |= IEEE80211_NODE_MIMO_PS;
	if (sta->flags & WL_STA_RIFS_CAP)
		ieeesta->isi_state |= IEEE80211_NODE_RIFS;

	ieeesta->isi_associd = sta->aid;

	bcopy(&sta->ea, ieeesta->isi_macaddr, ETHER_ADDR_LEN);

	ieeesta->isi_nrates = (u_int8_t) MIN(sta->rateset.count, IEEE80211_RATE_MAXSIZE);

	for (i = 0; i < ieeesta->isi_nrates; i++)
		ieeesta->isi_rates[i] = (u_int8_t) (sta->rateset.rates[i]);

	ieeesta->isi_txmbps = 2 * sta->tx_rate/1000;
	ieeesta->isi_rxmbps = 2 * sta->rx_rate/1000;

	ieeesta->isi_inact = sta->idle;
	ieeesta->isi_jointime = sta->in;
	ieeesta->isi_capinfo = (u_int8_t)sta->cap;
	ieeesta->isi_htcap = sta->ht_capabilities;

	if (sta->flags & WL_STA_AUTHO)
		ieeesta->isi_authorized |= IEEE80211_NODE_AUTH;


	wl_wlc_ioctl(ifp, WLC_GET_PHY_NOISE, &phynoise, sizeof(phynoise));

	ieeesta->isi_mimo.noise[0] = (int8_t)phynoise;
	ieeesta->isi_mimo.noise[1] = (int8_t)phynoise;
	ieeesta->isi_mimo.noise[2] = (int8_t)phynoise;

	ieeesta->isi_mimo.rssi[0] = sta->rssi[0];
	ieeesta->isi_mimo.rssi[1] = sta->rssi[1];
	ieeesta->isi_mimo.rssi[2] = sta->rssi[2];

	return 0;
}

static int
wl_ieee80211_stainfo_convert(wl_if_t *wlif, struct ifnet *ifp, struct ether_addr *ea, int *space,
	uint8 *p, int *bytes)
{
	sta_info_t sta;
	struct ieee80211req_sta_info *ieeesta;
	struct ieee80211_channel chan;
	int error = 0;
	uint16 ie_len = 0, sta_info_len = 0;
	scb_val_t scb_val;
	wl_info_t *wl = wlif->wl;

	bzero(&sta, sizeof(sta));
	bzero(&chan, sizeof(chan));

	/* get driver sta info */
	if ((error = wl_iovar_op(wl, "sta_info", ea, sizeof(*ea), &sta, sizeof(sta),
		IOV_GET, wlif, wlif->wlcif)))
		return error;

	ASSERT(sta.ver == WL_STA_VER);

	/* get sta ie len */
	ie_len = wlc_get_sta_ie_len(wlif, ea);

	if (sta.len > sizeof(struct ieee80211req_sta_info))
		sta_info_len = sta.len;
	else
		sta_info_len = sizeof(struct ieee80211req_sta_info);
	/* take care of ie len */
	sta_info_len += ie_len;
	sta_info_len = ROUNDUP(sta_info_len, sizeof(uint32));

	ieeesta = (struct ieee80211req_sta_info *)MALLOC(wl->osh, sta_info_len);
	bzero(ieeesta, sta_info_len);
	ieeesta->isi_len = sta_info_len;

	/* convert driver sta info */
	if ((error = wl_stainfo2ieee_stainfo(wlif, ifp, &sta, ieeesta)))
		return error;

	/* copy sta ie */
	ieeesta->isi_ie_off = sizeof(struct ieee80211req_sta_info);
	wlc_get_sta_ie(wlif, ea, ((uint8 *)ieeesta + ieeesta->isi_ie_off), &ieeesta->isi_ie_len);

	/* get chan info */
	if ((error = wl_ieee80211_curchan_get(wlif, &chan)))
		return error;
	/* get rssi */
	bcopy(ea, &scb_val.ea, sizeof(struct ether_addr));
	if ((error = wl_wlc_ioctl(ifp, WLC_GET_RSSI, &scb_val, sizeof(scb_val))))
		return error;

	ieeesta->isi_freq = chan.ic_freq;
	ieeesta->isi_flags = chan.ic_flags;

	ieeesta->isi_rssi = (int8)scb_val.val;

	/* Not enough space */
	if (ieeesta->isi_len > *space) {
		*bytes = 0;
		return 0;
	}

	/* Update bytes written and space left */
	*bytes = ieeesta->isi_len;
	*space -= ieeesta->isi_len;
	error = copyout(ieeesta, p, ieeesta->isi_len);

	MFREE(wl->osh, ieeesta, sta_info_len);
	return error;
}

/* IEEE80211_IOC_STA_INFO */
static int
wl_ieee80211_stainfo_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_sta_req sta_req;
	wl_info_t *wl = wlif->wl;
	struct ether_addr ea;
	uint8_t *p;
	int space, i, bytes = 0, error;

	if (ireq->i_len < sizeof(struct ieee80211req_sta_req))
		return EFAULT;

	if ((error = copyin(ireq->i_data, &sta_req, sizeof(struct ieee80211req_sta_req))))
		return error;

	bcopy(sta_req.is_u.macaddr, &ea, ETHER_ADDR_LEN);

	p =  (uint8_t *)ireq->i_data + OFFSETOF(struct ieee80211req_sta_req, info);
	space = ireq->i_len;

	/* If bcast mac address, then get the sta list and iterate through it */
	if (ETHER_ISMULTI(&ea)) {
		struct maclist *maclist;
		int max_count = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;

		if (!(maclist = MALLOC(wl->osh, WLC_IOCTL_MAXLEN)))
			return ENOMEM;
		bzero(maclist, WLC_IOCTL_MAXLEN);
		maclist->count = max_count;

		/* Get all the authenticated STAs */
		if ((error = wl_iovar_op(wl, "authe_sta_list", NULL, 0, maclist, WLC_IOCTL_MAXLEN,
		                         IOV_GET, wlif, wlif->wlcif))) {
			MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
			return error;
		}

		if (maclist->count == 0) {
			MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
			ireq->i_len = 0;
			return 0;
		}

		for (i = 0; i < maclist->count; i++) {
			if ((error = wl_ieee80211_stainfo_convert(wlif, ifp, &maclist->ea[i],
				&space, p, &bytes))) {
				MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
				return error;
			}
			p += bytes;
		}
		MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
	} else {
		if ((error = wl_ieee80211_stainfo_convert(wlif, ifp, &ea, &space, p, &bytes)))
			return error;
	}

	/* Adjust returned length */
	ireq->i_len -= space;

	return 0;
}

/* IEEE80211_IOC_STA_STATS */
static int
wl_ieee80211_stastats_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	uint8_t macaddr[IEEE80211_ADDR_LEN];
	int error;
	const int off = __offsetof(struct ieee80211req_sta_stats, is_stats);
	struct ieee80211_nodestats is_stats;
	sta_info_t sta;
	wl_info_t *wl = wlif->wl;

	if (ireq->i_len < off)
		return EINVAL;
	error = copyin(ireq->i_data, macaddr, IEEE80211_ADDR_LEN);
	if (error != 0)
		return error;

	if (ireq->i_len > sizeof(struct ieee80211req_sta_stats))
		ireq->i_len = sizeof(struct ieee80211req_sta_stats);

	memset(&is_stats, 0, sizeof(is_stats));

	bzero(&sta, sizeof(sta));

	/* get driver sta info */
	if ((error = wl_iovar_op(wl, "sta_info", macaddr, sizeof(macaddr), &sta, sizeof(sta),
		IOV_GET, wlif, wlif->wlcif)))
		return error;

	ASSERT(sta.ver == WL_STA_VER);

	if (sta.flags & WL_STA_SCBSTATS) {
		is_stats.ns_rx_data = sta.rx_tot_pkts;
		is_stats.ns_rx_ucast = sta.rx_ucast_pkts;
		is_stats.ns_rx_bytes = sta.rx_tot_bytes;
		is_stats.ns_tx_data = sta.tx_tot_pkts;
		is_stats.ns_tx_ucast = sta.tx_pkts;
		is_stats.ns_tx_bytes = sta.tx_tot_bytes;
		is_stats.ns_tx_failures = sta.tx_failures;
	}

	error = copyout(&is_stats, (uint8_t *) ireq->i_data + off, ireq->i_len - off);

	return error;
}

/* IEEE80211_IOC_SCAN_RESULTS: Convert result of driver ioctl to net80211 */
static int
wl_bss2ieee_sr(wl_if_t *wlif, wl_bss_info_t *bi, struct ieee80211req_scan_result *sr)
{
	int i;
	uint8 *cp;
	wl_info_t *wl = wlif->wl;
	struct ieee80211_channel c;

	sr->isr_len = roundup((sizeof(struct ieee80211req_scan_result) + bi->SSID_len +
		bi->ie_length), sizeof(uint32_t));

	/* The magic size is to allow max 512 bytes of IEs */
	if (sr->isr_len >
	    (sizeof(struct ieee80211req_scan_result) + IEEE80211_NWID_LEN + 256 * 2))
		return ENOSPC;

	chanspec_to_channel(wl->wlc, &c, bi->chanspec, FALSE);
	sr->isr_freq = c.ic_freq;
	sr->isr_flags = c.ic_flags;

	bcopy(&bi->BSSID, sr->isr_bssid, IEEE80211_ADDR_LEN);
	sr->isr_capinfo = (u_int8_t)bi->capability;
	sr->isr_intval = (u_int8_t)bi->beacon_period;

	sr->isr_nrates = (u_int8_t)MIN(bi->rateset.count, IEEE80211_RATE_MAXSIZE);
	for (i = 0; i < sr->isr_nrates; i++)
		sr->isr_rates[i] = (u_int8_t) (bi->rateset.rates[i]);

	sr->isr_noise = bi->phy_noise;
	sr->isr_rssi = bi->RSSI;

	sr->isr_ssid_len = bi->SSID_len;
	sr->isr_ie_len = bi->ie_length;

	sr->isr_ie_off = sizeof(struct ieee80211req_scan_result);

	cp = (uint8 *) ((uint8*)sr + sizeof(struct ieee80211req_scan_result));

	/* variable length SSID followed by IE data */
	bcopy(bi->SSID, cp, bi->SSID_len);
	cp += bi->SSID_len;
	bcopy((uint8*)(((uint8 *)bi) + bi->ie_offset), cp, bi->ie_length);
	return 0;
}

/* IEEE80211_IOC_SCAN_RESULTS */
static int
wl_ieee80211_scanresults_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	wl_scan_results_t *list;
	wl_info_t *wl = wlif->wl;
	wl_bss_info_t *bi = NULL;
	u_int8_t *p;
	void *sr_buf = NULL; /* Buffer to allocate and store sr_buf */
	struct ieee80211req_scan_result *sr;
	int error, space = 0, i;
	u_int8_t *buf = NULL;
	size_t listlen;

	if (!wl->scandone) {
		return EBUSY;
	}
	listlen = MAX(ireq->i_len, WLC_IOCTL_MAXLEN);

	if (!(list = (void *) MALLOC(wl->osh, listlen))) {
		error = BCME_NORESOURCE;
		goto sr_exit;
	}

	bzero(list, listlen);
	list->buflen = listlen;

	if ((error = wl_wlc_ioctl(ifp, WLC_SCAN_RESULTS, list, list->buflen))) {
		goto sr_exit;
	}

	ASSERT(list->version == WL_BSS_INFO_VERSION);

	if (!(sr_buf = (void *) MALLOC(wl->osh, WLC_IOCTL_MAXLEN))) {
		error = BCME_NORESOURCE;
		goto sr_exit;
	}

	space = ireq->i_len;

	if (!(buf = (void *) MALLOC(wl->osh, space))) {
		error = BCME_NORESOURCE;
		goto sr_exit;
	}

	p = buf;

	/* Convert wl_bssinfo_t to ieee80211req_scan_result */
	for (i = 0; i < list->count; i++) {
		bi = bi ? (wl_bss_info_t *)((unsigned) bi + bi->length) : list->bss_info;
		ASSERT(((unsigned) bi + bi->length) <= ((unsigned) list + list->buflen));

		/* Infrastructure only */
		if (!(bi->capability & DOT11_CAP_ESS))
			continue;

		bzero(sr_buf, WLC_IOCTL_MAXLEN);

		/* Convert the structure to IEEE80211 struct */
		if (wl_bss2ieee_sr(wlif, bi, (struct ieee80211req_scan_result *)sr_buf) != 0)
			continue;

		sr = (struct ieee80211req_scan_result *) sr_buf;

		/* Ran out of space for the result */
		if (space < sr->isr_len)
			break;

		memcpy(p, sr, sr->isr_len);

		p += sr->isr_len;
		space -= sr->isr_len;
	}

	/* Copy the current entry to results */

	error = copyout(buf, ireq->i_data, p - buf);


sr_exit:
	if (list)
		 MFREE(wl->osh, list, listlen);
	if (sr_buf)
		 MFREE(wl->osh, sr_buf, WLC_IOCTL_MAXLEN);
	if (buf)
		 MFREE(wl->osh, buf, ireq->i_len);
	if (error != BCME_OK) {
		wl->pub->bcmerror = error;
		error = OSL_ERROR(error);
	}

	ireq->i_len -= space;

	return error;
}

static int
wl_ieee80211_cq_scanresults_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int error, space = 0, i;
	u_int8_t *buf = NULL;
	wl_chanim_stats_t *list;
	wl_info_t *wl = wlif->wl;
	chanim_stats_t *ci = NULL;
	struct ieee80211req_cq_scan_result *cqsr;

	if (!wl->scandone) {
		return EBUSY;
	}

	if (!(list = (void *) MALLOC(wl->osh, MAX(ireq->i_len, WLC_IOCTL_MAXLEN)))) {
		error = BCME_NORESOURCE;
		goto cqsr_exit;
	}

	bzero(list, MAX(ireq->i_len, WLC_IOCTL_MAXLEN));
	list->buflen = MAX(ireq->i_len, WLC_IOCTL_MAXLEN);
	list->count = WL_CHANIM_COUNT_ALL;

	if ((error = wl_iovar_op(wl, "chanim_stats", NULL, 0, list,
		list->buflen, IOV_GET, wlif, NULL)))
		goto cqsr_exit;

	space = ireq->i_len;

	if (!(buf = (void *) MALLOC(wl->osh, space))) {
		error = BCME_NORESOURCE;
		goto cqsr_exit;
	}

	cqsr = (struct ieee80211req_cq_scan_result *) buf;

	/* Convert chanim_stats_t to ieee80211req_cq_scan_result */
	for (i = 0; i < list->count; i++) {
		struct ieee80211_channel c;

		ci = ci ? (ci + 1) : list->stats;
		ASSERT((unsigned)(ci + 1) <= ((unsigned) list + list->buflen));


		/* Ran out of space for the result */
		if (space < sizeof(struct ieee80211req_cq_scan_result))
			break;

		chanspec_to_channel(wl->wlc, &c, ci->chanspec, FALSE);
		cqsr->icqsr_freq = c.ic_freq;
		cqsr->icqsr_flags = c.ic_flags;
		cqsr->icqsr_noise = ci->bgnoise;
		cqsr->icqsr_txop = ci->ccastats[CCASTATS_TXOP];

		cqsr++;
		space -= sizeof(struct ieee80211req_cq_scan_result);
	}

	/* Copy the current entry to results */

	error = copyout(buf, ireq->i_data, ((u_int8_t *) cqsr) - buf);

cqsr_exit:
	if (list)
		 MFREE(wl->osh, list, list->buflen);
	if (buf)
		 MFREE(wl->osh, buf, ireq->i_len);

	if (error != BCME_OK) {
		wl->pub->bcmerror = error;
		error = OSL_ERROR(error);
	}

	ireq->i_len -= space;

	return error;
}

/* IEEE80211_IOC_WPAKEY: Since getting the key data should be protected,
 * assumption for this routine is to get key seq
 */
static int
wl_ieee80211_wpakey_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_key ik;
	u_int kid;
	int error, defkey, i;
	union {
		int32 index;
		uint8 tsc[EAPOL_WPA_KEY_RSC_LEN];
	} u;

	if (ireq->i_len != sizeof(ik))
		return EINVAL;

	if ((error = copyin(ireq->i_data, &ik, sizeof(ik))))
		return error;

	bzero(&u, sizeof(u));
	bzero(&ik, sizeof(ik));

	kid = ik.ik_keyix;

	/* We support only key seq retrieval based on index */
	if (kid == IEEE80211_KEYIX_NONE)
		return EINVAL;
	else {
		if (kid >= IEEE80211_WEP_NKID)
			return EINVAL;
		u.index = kid;
	}

	/* find the default tx key */
	i = 0;
	do {
		defkey = i;
		if ((error = wl_wlc_ioctl(ifp, WLC_GET_KEY_PRIMARY, &defkey, sizeof(defkey))))
			return error;

		if (defkey) {
			defkey = i;
			break;
		}
	} while (++i < DOT11_MAX_DEFAULT_KEYS);

	if (kid == defkey) {
		if ((error = wl_wlc_ioctl(ifp, WLC_GET_KEY_SEQ, &u, sizeof(u))))
			return error;
		bcopy(u.tsc, &ik.ik_keytsc, EAPOL_WPA_KEY_RSC_LEN);
	}

	return copyout(&ik, ireq->i_data, sizeof(ik));
}

/* IEEE80211_IOC_WPAKEY */
static int
wl_ieee80211_wpakey_set(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_key ik;
	int error;
	wl_wsec_key_t key;
	wl_info_t *wl = wlif->wl;
	u_int8_t *ivptr;
	u_int16_t kid;

	if (ireq->i_len != sizeof(ik))
		return EINVAL;
	if ((error = copyin(ireq->i_data, &ik, sizeof(ik))))
		return error;

	if (ik.ik_keylen > sizeof(ik.ik_keydata))
		return E2BIG;

	bzero(&key, sizeof(key));

	kid = ik.ik_keyix;

	/* Verify input */
	if (kid == IEEE80211_KEYIX_NONE) {
		if (ik.ik_flags != (IEEE80211_KEY_XMIT | IEEE80211_KEY_RECV))
			return EINVAL;
	} else {
		if (kid >= IEEE80211_WEP_NKID)
			return EINVAL;
		key.index = ik.ik_keyix;
	}

	bcopy(ik.ik_macaddr, &key.ea, ETHER_ADDR_LEN);

	key.len = ik.ik_keylen;
	if (key.len > sizeof(key.data))
		return EINVAL;

	ivptr = (u_int8_t*)&ik.ik_keyrsc;
	key.rxiv.hi = (ivptr[5] << 24) | (ivptr[4] << 16) |
	        (ivptr[3] << 8) | ivptr[2];
	key.rxiv.lo = (ivptr[1] << 8) | ivptr[0];
	key.iv_initialized = TRUE;

	bcopy(ik.ik_keydata, key.data, ik.ik_keylen);

	switch (ik.ik_type) {
	case IEEE80211_CIPHER_NONE:
		key.algo = CRYPTO_ALGO_OFF;
		break;
	case IEEE80211_CIPHER_WEP:
		if (ik.ik_keylen == WEP1_KEY_SIZE)
			key.algo = CRYPTO_ALGO_WEP1;
		else
			key.algo = CRYPTO_ALGO_WEP128;
		break;
	case IEEE80211_CIPHER_TKIP:
		key.algo = CRYPTO_ALGO_TKIP;
		break;
	case IEEE80211_CIPHER_AES_CCM:
		key.algo = CRYPTO_ALGO_AES_CCM;
		break;
	default:
		break;
	}

	if (ik.ik_flags & IEEE80211_KEY_DEFAULT)
		key.flags = WL_PRIMARY_KEY;

	/* Instead of bcast for ea address for default wep keys, driver needs it to be Null */
	if (ETHER_ISMULTI(&key.ea))
		bzero(&key.ea, ETHER_ADDR_LEN);

	return wl_iovar_op(wl, "wsec_key", NULL, 0, &key, sizeof(key), IOV_SET, wlif, wlif->wlcif);
}

/* IEEE80211_IOC_WPAIE */
static int
wl_ieee80211_wpaie_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_wpaie wpaie;
	int error;
	wl_info_t * wl = wlif->wl;
	uint8 *buf;
	int ielen;

	if (ireq->i_len < IEEE80211_ADDR_LEN)
		return EINVAL;

	if ((error = copyin(ireq->i_data, wpaie.wpa_macaddr, IEEE80211_ADDR_LEN)))
		return error;

	if (!(buf = (uint8 *) MALLOC(wl->osh, WLC_IOCTL_MAXLEN))) {
		error = BCME_NORESOURCE;
		wl->pub->bcmerror = error;
		return (OSL_ERROR(error));
	}

	error = wl_iovar_op(wl, "wpaie", wpaie.wpa_macaddr,
		IEEE80211_ADDR_LEN, (void *)buf, WLC_IOCTL_MAXLEN,
		IOV_GET, wlif, wlif->wlcif);
	bzero(wpaie.wpa_ie, sizeof(wpaie.wpa_ie));

	if (!error) {
		ielen = buf[1] + 2;
		/* driver puts wps/wpa/rsn IEs...we dont want wps
		 * since this confuses our hostapd
		 */
		if (!bcm_find_wpsie(buf, ielen)) {
			if (ielen > sizeof(wpaie.wpa_ie))
				ielen = sizeof(wpaie.wpa_ie);
			memcpy(wpaie.wpa_ie, buf, ielen);
		}
	}
	MFREE(wl->osh, buf, WLC_IOCTL_MAXLEN);

	if (ireq->i_len > sizeof(wpaie))
		ireq->i_len = sizeof(wpaie);

	return copyout(&wpaie, ireq->i_data, ireq->i_len);
}


/*
 * Channel support routines.
 */
static int
wl_ieee80211chan_to_chanspec(wl_info_t *wl, struct ieee80211_channel *chan, chanspec_t *chanspec)
{
	*chanspec = 0;
	char buf[32];

	if (chan->ic_freq == 0 || chan->ic_freq == IEEE80211_CHAN_ANY) {
		return EINVAL;
	} else {
		if (IEEE80211_IS_CHAN_HT40(chan))
			*chanspec |= WL_CHANSPEC_BW_40;
		else if (IEEE80211_IS_CHAN_VHT80(chan))
			*chanspec |= WL_CHANSPEC_BW_80;
		else
			*chanspec |= WL_CHANSPEC_BW_20;

		if (IEEE80211_IS_CHAN_2GHZ(chan))
			*chanspec |= WL_CHANSPEC_BAND_2G;
		else if (IEEE80211_IS_CHAN_5GHZ(chan))
			*chanspec |= WL_CHANSPEC_BAND_5G;
		else
			return EINVAL;

		if (CHSPEC_IS40(*chanspec)) {
			if (IEEE80211_IS_CHAN_HT40U(chan)) {
				*chanspec |= WL_CHANSPEC_CTL_SB_L;
				*chanspec |= (chan->ic_ieee + 2);
			} else {
				*chanspec |= WL_CHANSPEC_CTL_SB_U;
				*chanspec |= (chan->ic_ieee - 2);
			}
		} else if (CHSPEC_IS80(*chanspec)) {
			/* hack...couldnt find any APIs to do this */
			sprintf(buf, "%d/80", chan->ic_ieee);
			*chanspec = wf_chspec_aton(buf);
		} else {
			*chanspec |= chan->ic_ieee;
		}
	}

	return 0;
}

static __inline int
chancompar(const void *a, const void *b)
{
	const struct ieee80211_channel *ca = a;
	const struct ieee80211_channel *cb = b;

	return (ca->ic_freq == cb->ic_freq) ?
	(ca->ic_flags & IEEE80211_CHAN_ALL) -
	(cb->ic_flags & IEEE80211_CHAN_ALL) :
	ca->ic_freq - cb->ic_freq;
}

/*
 * Insertion sort.
 */
#define swap(_a, _b, _size) {			\
	uint8_t *s = _b;			\
	int i = _size;				\
	do {					\
		uint8_t tmp = *_a;		\
		*_a++ = *s;			\
		*s++ = tmp;			\
	} while (--i);				\
	_a -= _size;				\
}

static void
sort_channels(void *a, size_t n, size_t size)
{
	uint8_t *aa = a;
	uint8_t *ai, *t;

	for (ai = aa+size; --n >= 1; ai += size)
		for (t = ai; t > aa; t -= size) {
			uint8_t *u = t - size;
			if (chancompar(u, t) <= 0)
				break;
			swap(u, t, size);
		}
}
#undef swap

/*
 * Order channels w/ the same frequency so that
 * b < g < htg and a < hta.  This is used to optimize
 * channel table lookups and some user applications
 * may also depend on it (though they should not).
 */
void
ieee80211_sort_channels(struct ieee80211_channel chans[], int nchans)
{
	if (nchans > 0)
		sort_channels(chans, nchans, sizeof(struct ieee80211_channel));
}

enum ieee80211_phymode
ieee80211_chan2mode(const struct ieee80211_channel *chan)
{
	if (IEEE80211_IS_CHAN_HTA(chan))
		return IEEE80211_MODE_11NA;
	else if (IEEE80211_IS_CHAN_HTG(chan))
		return IEEE80211_MODE_11NG;
	else if (IEEE80211_IS_CHAN_VHT(chan))
		return IEEE80211_MODE_11AC;
	else if (IEEE80211_IS_CHAN_108G(chan))
		return IEEE80211_MODE_TURBO_G;
	else if (IEEE80211_IS_CHAN_ST(chan))
		return IEEE80211_MODE_STURBO_A;
	else if (IEEE80211_IS_CHAN_TURBO(chan))
		return IEEE80211_MODE_TURBO_A;
	else if (IEEE80211_IS_CHAN_A(chan))
		return IEEE80211_MODE_11A;
	else if (IEEE80211_IS_CHAN_ANYG(chan))
		return IEEE80211_MODE_11G;
	else if (IEEE80211_IS_CHAN_B(chan))
		return IEEE80211_MODE_11B;
	else if (IEEE80211_IS_CHAN_FHSS(chan))
		return IEEE80211_MODE_FH;
	else
		return 0;
}

static void
wl_txpower_min_max(wlc_info_t *wlc, chanspec_t chanspec, int *min_pwr, int *max_pwr)
{
	int cur_min = 0xFFFF, cur_max = 0;
	wlc_phy_t *pi = WLC_PI(wlc);
	ppr_t *txpwr;
	ppr_t *srommax;
	int8 min_srom;

	*min_pwr = 5; /* ? */
	*max_pwr = WLC_TXPWR_MAX;

	if ((txpwr = ppr_create(wlc->osh, PPR_CHSPEC_BW(chanspec))) == NULL) {
		return;
	}

	if ((srommax = ppr_create(wlc->osh, PPR_CHSPEC_BW(chanspec))) == NULL) {
		ppr_delete(wlc->osh, txpwr);
		return;
	}

	wlc_channel_reg_limits(wlc->cmi, chanspec, txpwr);

	wlc_phy_txpower_sromlimit(pi, chanspec, (uint8*)&min_srom, srommax, 0);
	/* printf("+++srom min %d srom max %d\n", min_srom, ppr_get_max(srommax)); */

	ppr_apply_vector_ceiling(txpwr, srommax);
	ppr_apply_min(txpwr, min_srom);

	WL_NONE(("(%d)\n", min_srom));

	cur_min = ppr_get_min(txpwr, min_srom);
	cur_max = ppr_get_max(txpwr);

	ppr_delete(wlc->osh, srommax);
	ppr_delete(wlc->osh, txpwr);

	*min_pwr = (int)cur_min;
	*max_pwr = (int)cur_max;
	WL_NONE(("+++*min_pwr %d *max_pwr %d\n", *min_pwr, *max_pwr));
}

static void
chanspec_to_channel(wlc_info_t *wlc, struct ieee80211_channel *c, chanspec_t chanspec, bool isht)
{
	int chan = CHSPEC_CHANNEL(chanspec);
	int chan_min, chan_max;

	/* Get channel min and max power */
	wl_txpower_min_max(wlc, chanspec, &chan_min, &chan_max);

	c->ic_ieee = wf_chspec_ctlchan(chanspec);

	if (CHSPEC_IS2G(chanspec)) {
		c->ic_flags = IEEE80211_CHAN_2GHZ;
		c->ic_freq = wf_channel2mhz(c->ic_ieee, WF_CHAN_FACTOR_2_4_G);
	} else if (CHSPEC_IS5G(chanspec)) {
		c->ic_flags = IEEE80211_CHAN_5GHZ;
		c->ic_freq = wf_channel2mhz(c->ic_ieee, WF_CHAN_FACTOR_5_G);
	} else
		ASSERT(0);

	if (wlc_quiet_chanspec(wlc->cmi, chanspec)) {
		c->ic_flags |= IEEE80211_CHAN_PASSIVE;
	}

	if (wlc_radar_chanspec(wlc->cmi, chanspec)) {
		c->ic_flags |= IEEE80211_CHAN_DFS;
	}

	c->ic_minpower = chan_min / 2;	/* .5 dBm */
	c->ic_maxpower = chan_max / 2;	/* .5 dBm */
	if (!isht) {
		if (CHSPEC_IS2G(chanspec)) {
			c->ic_flags |= IEEE80211_CHAN_CCK;
		} else {
			c->ic_flags |= IEEE80211_CHAN_OFDM;
		}
	} else if (CHSPEC_IS40(chanspec)) {
		if (CHSPEC_SB_LOWER(chanspec)) {
			c->ic_flags |= IEEE80211_CHAN_HT40U;
			c->ic_extieee = UPPER_20_SB(chan);
		} else {
			c->ic_flags |= IEEE80211_CHAN_HT40D;
			c->ic_extieee = LOWER_20_SB(chan);
		}
		/* NB: for legacy operation */
		if (CHSPEC_IS2G(chanspec)) {
			c->ic_flags |= IEEE80211_CHAN_DYN;
		} else {
			c->ic_flags |= IEEE80211_CHAN_OFDM;
		}
	} else if (CHSPEC_IS20(chanspec)) {
		if (CHSPEC_IS2G(chanspec)) {
			c->ic_flags |= IEEE80211_CHAN_DYN | IEEE80211_CHAN_HT20;
		} else {
			c->ic_flags |= IEEE80211_CHAN_OFDM | IEEE80211_CHAN_HT20;
		}
	} else if (CHSPEC_IS80(chanspec)) {
		c->ic_flags |= IEEE80211_CHAN_VHT80;
		c->ic_flags |= IEEE80211_CHAN_OFDM;

	}
	c->ic_maxregpower = c->ic_maxpower;			/* .5 dBm */
	c->ic_state = 0;
}

static int
wlc_getchannels(wlc_info_t *wlc, struct ieee80211_channel channels[], const int maxchan)
{
	struct ieee80211_channel *c;
	chanspec_t cspec;
	int chan, nchans;

	nchans = 0;
	for (chan = 0; chan < MAXCHANNEL; chan++) {
		cspec = CH20MHZ_CHSPEC(chan);
		if (!wlc_valid_chanspec_db(wlc->cmi, cspec))
			continue;
		if (nchans > maxchan) {
			return nchans;
		}
		c = &channels[nchans++];
		chanspec_to_channel(wlc, c, cspec, FALSE);
		if (CHSPEC_IS2G(cspec)) {
			if (nchans > maxchan) {
				return nchans;
			}
			/* add 11g channel to match 11b */
			c = &channels[nchans++];
			c[0] = c[-1];
			c->ic_flags &= ~IEEE80211_CHAN_CCK;
			c->ic_flags |= IEEE80211_CHAN_DYN;
		}
	}
	if (PHYTYPE_11N_CAP(wlc->band->phytype)) {
		/* add HT channels */
		for (chan = 0; chan < MAXCHANNEL; chan++) {
			cspec = CH20MHZ_CHSPEC(chan);
			if (wlc_valid_chanspec_db(wlc->cmi, cspec)) {
				if (nchans > maxchan) {
					return nchans;
				}
				c = &channels[nchans++];
				chanspec_to_channel(wlc, c, cspec, TRUE);
			}
			cspec = CH40MHZ_CHSPEC(chan, WL_CHANSPEC_CTL_SB_LOWER);
			if (wlc_valid_chanspec_db(wlc->cmi, cspec)) {
				if (nchans > maxchan) {
					return nchans;
				}
				c = &channels[nchans++];
				chanspec_to_channel(wlc, c, cspec, TRUE);
			}
			cspec = CH40MHZ_CHSPEC(chan, WL_CHANSPEC_CTL_SB_UPPER);
			if (wlc_valid_chanspec_db(wlc->cmi, cspec)) {
				if (nchans > maxchan) {
					return nchans;
				}
				c = &channels[nchans++];
				chanspec_to_channel(wlc, c, cspec, TRUE);
			}
		}
	}
	if (PHYTYPE_VHT_CAP(wlc->band->phytype)) {
		/* add vht 80 channels */
		for (chan = 0; chan < MAXCHANNEL; chan++) {
			cspec = CH80MHZ_CHSPEC(chan, WL_CHANSPEC_CTL_SB_LL);
			if (wlc_valid_chanspec_db(wlc->cmi, cspec)) {
				if (nchans > maxchan) {
					return nchans;
				}
				c = &channels[nchans++];
				chanspec_to_channel(wlc, c, cspec, TRUE);
			}
			cspec = CH80MHZ_CHSPEC(chan, WL_CHANSPEC_CTL_SB_LU);
			if (wlc_valid_chanspec_db(wlc->cmi, cspec)) {
				if (nchans > maxchan) {
					return nchans;
				}
				c = &channels[nchans++];
				chanspec_to_channel(wlc, c, cspec, TRUE);
			}
			cspec = CH80MHZ_CHSPEC(chan, WL_CHANSPEC_CTL_SB_UL);
			if (wlc_valid_chanspec_db(wlc->cmi, cspec)) {
				if (nchans > maxchan) {
					return nchans;
				}
				c = &channels[nchans++];
				chanspec_to_channel(wlc, c, cspec, TRUE);
			}
			cspec = CH80MHZ_CHSPEC(chan, WL_CHANSPEC_CTL_SB_UU);
			if (wlc_valid_chanspec_db(wlc->cmi, cspec)) {
				if (nchans > maxchan) {
					return nchans;
				}
				c = &channels[nchans++];
				chanspec_to_channel(wlc, c, cspec, TRUE);
			}
		}

	}

	return nchans;
}

static int
wl_ieee80211_getchannels(wl_if_t *wlif, u_int *nchans, struct ieee80211_channel *chans)
{
	wl_info_t *wl = wlif->wl;

	*nchans = wlc_getchannels(wl->wlc, chans, IEEE80211_CHAN_MAX);
	ieee80211_sort_channels(chans, *nchans);

	return 0;
}


/* IEEE80211_IOC_CURCHAN */
static int
wl_ieee80211_curchan_get(wl_if_t *wlif, struct ieee80211_channel *chan)
{
	int error = 0;
	wl_info_t *wl = wlif->wl;
	int nmode, vhtmode, chanspec;
	bool isht = FALSE;

	if ((error = wl_iovar_getint(wl, "chanspec", &chanspec, wlif, wlif->wlcif)))
		return error;

	if ((error = wl_iovar_getint(wl, "nmode", &nmode, wlif, wlif->wlcif)))
		return error;

	if ((error = wl_iovar_getint(wl, "vhtmode", &vhtmode, wlif, wlif->wlcif)))
		return error;

	if (nmode || vhtmode)
		isht = TRUE;

	chanspec_to_channel(wl->wlc, chan, chanspec, isht);

	return error;
}


/* IEEE80211_IOC_CHANINFO */
static int
wl_ieee80211_chaninfo_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_chaninfo chans;
	wl_info_t *wl = wlif->wl;

	memset(&chans, 0, sizeof(chans));

	memcpy(chans.ic_chans, wl->chans, sizeof(struct ieee80211_channel) * wl->nchans);
	chans.ic_nchans = wl->nchans;

	return copyout(&chans, ireq->i_data, sizeof(struct ieee80211req_chaninfo));
}

/* IEEE80211_IOC_CHANLIST */
static int
wl_ieee80211_chanlist_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_chanlist chanlist;
	struct ieee80211_channel chans[IEEE80211_CHAN_MAX];
	u_int nchans = 0;
	u_int i;
	wl_info_t *wl = wlif->wl;

	memset(&chanlist, 0, sizeof(chanlist));
	memset(chans, 0, sizeof(chans));

	memcpy(chans, wl->chans, sizeof(struct ieee80211_channel) * wl->nchans);
	nchans = wl->nchans;

	for (i = 0; i < nchans; i++) {
		setbit(chanlist.ic_channels, chans[i].ic_ieee);
	}

	return (copyout(&chanlist, ireq->i_data, sizeof(chanlist)));
}

/* IEEE80211_IOC_MACCMD */
static int
wl_ieee80211_macmode_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int macmode, space;
	int error, i;
	struct maclist *maclist;
	int max_count = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;
	struct ieee80211req_maclist *ap;
	wl_info_t *wl = wlif->wl;

	switch (ireq->i_val) {
	case IEEE80211_MACCMD_POLICY:
		if ((error = wl_wlc_ioctl(ifp, WLC_GET_MACMODE, &macmode, sizeof(macmode))))
			return error;
		switch (macmode) {
		case WLC_MACMODE_DISABLED:
			ireq->i_val = IEEE80211_MACCMD_POLICY_OPEN;
			return 0;
		case WLC_MACMODE_DENY:
			ireq->i_val = IEEE80211_MACCMD_POLICY_DENY;
			return 0;
		case WLC_MACMODE_ALLOW:
			ireq->i_val = IEEE80211_MACCMD_POLICY_ALLOW;
			return 0;
		}
		return EINVAL;
	case IEEE80211_MACCMD_LIST:
		if (!(maclist = MALLOC(wl->osh, WLC_IOCTL_MAXLEN)))
			return ENOMEM;
		bzero(maclist, WLC_IOCTL_MAXLEN);
		maclist->count = max_count;

		if ((error = wl_wlc_ioctl(ifp, WLC_GET_MACLIST, maclist, WLC_IOCTL_MAXLEN))) {
			MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
			return error;
		}

		if (maclist->count == 0) {
			MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
			return 0;
		}

		space = maclist->count * IEEE80211_ADDR_LEN;
		if (ireq->i_len == 0) {
			ireq->i_len = space;	/* return required space */
			MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
			return 0;		/* NB: must not error */
		}
		ap = ireq->i_data;

		/* Copy enough entries within available space */
		if (ireq->i_len < space)
			max_count = MIN(ireq->i_len / ETHER_ADDR_LEN, maclist->count);

		for (i = 0; i < max_count; i++)
			if ((error = copyout(&maclist->ea[i], &ap[i], ETHER_ADDR_LEN)))
				break;
		MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
		return error;
	}
	return EINVAL;
}

/* IEEE80211_IOC_MACCMD */
static int
wl_ieee80211_macmode_set(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int macmode;
	struct maclist maclist;
	int error;

	switch (ireq->i_val) {
	case IEEE80211_MACCMD_POLICY_ALLOW: /* WLC_MACMODE_ALLOW */
		macmode = WLC_MACMODE_ALLOW;
		return wl_wlc_ioctl(ifp, WLC_SET_MACMODE, &macmode, sizeof(macmode));
	case IEEE80211_MACCMD_POLICY_DENY: /* WLC_MACMODE_DENY */
		macmode = WLC_MACMODE_DENY;
		return wl_wlc_ioctl(ifp, WLC_SET_MACMODE, &macmode, sizeof(macmode));
	case IEEE80211_MACCMD_FLUSH: /* Keep the macmode but delete the maclist */
		maclist.count = 0;
		return wl_wlc_ioctl(ifp, WLC_SET_MACLIST, &maclist, sizeof(int));
	case IEEE80211_MACCMD_POLICY_OPEN: /* Disable the macmode WLC_MACMODE_DISABLED */
		macmode = WLC_MACMODE_DISABLED;
		return wl_wlc_ioctl(ifp, WLC_SET_MACMODE, &macmode, sizeof(macmode));
	case IEEE80211_MACCMD_DETACH: /* Delete both the list and macmode */
		macmode = WLC_MACMODE_DISABLED;
		if ((error = wl_wlc_ioctl(ifp, WLC_SET_MACMODE, &macmode, sizeof(macmode))))
			return error;
		maclist.count = 0;
		return wl_wlc_ioctl(ifp, WLC_SET_MACLIST, &maclist, sizeof(int));
	default:
		return EINVAL;
	}
	return 0;

}

/* IEEE80211_IOC_ADDMAC,IEEE80211_IOC_DELMAC
 * Add /remove Mac to the list/from the list
 */
static int
wl_ieee80211_macmac(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct maclist *maclist;
	int max_count = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;
	u_int8_t mac[IEEE80211_ADDR_LEN];
	int i, error, len;
	wl_info_t *wl = wlif->wl;

	if (ireq->i_len != sizeof(mac))
		return EINVAL;
	error = copyin(ireq->i_data, mac, ireq->i_len);
	if (error)
		return error;

	/* Get the current maclist first */
	if (!(maclist = MALLOC(wl->osh, WLC_IOCTL_MAXLEN)))
		return ENOMEM;
	bzero(maclist, WLC_IOCTL_MAXLEN);
	maclist->count = max_count;

	if ((error = wl_wlc_ioctl(ifp, WLC_GET_MACLIST, maclist, WLC_IOCTL_MAXLEN))) {
		MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
		return error;
	}

	/* Add if it does not exist
	 * Remove if it does
	 */
	for (i = 0; i < maclist->count; i++)
		if (!(bcmp(&maclist->ea[i], mac, ETHER_ADDR_LEN)))
			break;

	if (ireq->i_type == IEEE80211_IOC_ADDMAC) {
		/* Found match. Do nothing */
		if (i != maclist->count) {
			MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
			return 0;
		}

		/* No more space */
		if (i == max_count) {
			MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
			return ENOSPC;
		}

		bcopy(mac, &maclist->ea[i], ETHER_ADDR_LEN);
		maclist->count++;
	} else {
		/* Did not find a match , do nothing */
		if (i == maclist->count) {
			MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);
			return 0;
		}

		/* Shift the remaining list */
		if (i != (maclist->count - 1))
			for (; i < maclist->count - 1; i++)
				bcopy(&maclist->ea[i+1], &maclist->ea[i], ETHER_ADDR_LEN);
		maclist->count--;
	}

	/* Set new list */
	len = sizeof(maclist->count) + maclist->count * sizeof(maclist->ea);
	error = wl_wlc_ioctl(ifp, WLC_SET_MACLIST, maclist, len);
	MFREE(wl->osh, maclist, WLC_IOCTL_MAXLEN);

	return error;
}

/* IEEE80211_IOC_DELKEY: Given macaddr or index, delete a key */
static int
wl_ieee80211_delkey(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_del_key dk;
	int kid, error;
	wl_wsec_key_t key;
	wl_info_t *wl = wlif->wl;

	if (ireq->i_len != sizeof(dk))
		return EINVAL;

	if ((error = copyin(ireq->i_data, &dk, sizeof(dk))))
		return error;

	kid = dk.idk_keyix;

	bzero(&key, sizeof(key));

	if (dk.idk_keyix == (u_int8_t) IEEE80211_KEYIX_NONE)
		bcopy(dk.idk_macaddr, &key.ea, ETHER_ADDR_LEN);
	else {
		if (kid >= IEEE80211_WEP_NKID)
			return EINVAL;
		key.index = dk.idk_keyix;
	}

	return wl_iovar_op(wl, "wsec_key", NULL, 0, &key, sizeof(key), IOV_SET, wlif, wlif->wlcif);
}

/* IEEE80211_IOC_MLME */
static int
wl_ieee80211_mlme_op(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_mlme mlme;
	int error = 0;
	int cmd;
	wl_join_params_t joinparams;
	scb_val_t scb_val;
	wl_info_t *wl = wlif->wl;

	if (ireq->i_len != sizeof(mlme))
		return EINVAL;

	if ((error = copyin(ireq->i_data, &mlme, sizeof(mlme))))
		return error;

	switch (mlme.im_op) {
	case IEEE80211_MLME_ASSOC:
		if (WLIF_AP_MODE(wl, wlif))
			return EINVAL;

		/* mlme.im_ssid specifies SSID while mlme.im_macaddr specifies BSSID to join */
		/* Security needs to be set before doing this by the caller */
		if (mlme.im_ssid_len != 0) {
			/*
			 * Desired ssid specified; must match both bssid and
			 * ssid to distinguish ap advertising multiple ssid's.
			 */
			bzero(&joinparams, sizeof(joinparams));
			joinparams.ssid.SSID_len = mlme.im_ssid_len;
			memcpy(joinparams.ssid.SSID, mlme.im_ssid, mlme.im_ssid_len);
			memcpy(&joinparams.params.bssid, mlme.im_macaddr, ETHER_ADDR_LEN);
			return wl_wlc_ioctl(ifp, WLC_SET_SSID, &joinparams, sizeof(joinparams));
		} else {
			/*
			 * Normal case; just match bssid.
			 */
			bzero(&joinparams, sizeof(joinparams));
			memcpy(&joinparams.params.bssid, mlme.im_macaddr, ETHER_ADDR_LEN);
			return wl_wlc_ioctl(ifp, WLC_SET_SSID, &joinparams, sizeof(joinparams));
		}
	case IEEE80211_MLME_DISASSOC: /* Does not apply to STA for us */
	case IEEE80211_MLME_DEAUTH:
		scb_val.val = mlme.im_reason;
		bcopy(mlme.im_macaddr, &scb_val.ea, ETHER_ADDR_LEN);
		return wl_wlc_ioctl(ifp, WLC_SCB_DEAUTHENTICATE_FOR_REASON, &scb_val,
		                    sizeof(scb_val));
	case IEEE80211_MLME_AUTHORIZE:
	case IEEE80211_MLME_UNAUTHORIZE:
		if (mlme.im_op == IEEE80211_MLME_AUTHORIZE)
			cmd = WLC_SCB_AUTHORIZE;
		else
			cmd = WLC_SCB_DEAUTHORIZE;
		return wl_wlc_ioctl(ifp, cmd, &mlme.im_macaddr, ETHER_ADDR_LEN);

	case IEEE80211_MLME_NULL:
		return wl_iovar_op(wl, "send_nulldata", NULL, 0, &mlme.im_macaddr,
		                   sizeof(mlme.im_macaddr), IOV_SET, wlif, wlif->wlcif);

	default:
		return EINVAL;
	}
	return error;
}


/* IEEE80211_IOC_WME_CWMIN, IEEE80211_IOC_WME_CWMAX, IEEE80211_IOC_WME_AIFS,
 * IEEE80211_IOC_WME_TXOPLIMIT, IEEE80211_IOC_WME_ACM, IEEE80211_IOC_WME_ACKPOLICY
 */
static int
wl_ieee80211_wmeparam_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int ac = (ireq->i_len & IEEE80211_WMEPARAM_VAL);
	edcf_acparam_t acps[AC_COUNT], *acp;
	int error = 0, int_val;
	const char *iov_name; /* IOVAR to use based on request and mode */
	wl_info_t *wl = wlif->wl;

	if (ac >= WME_NUM_AC)
		ac = WME_AC_BE;

	/* Determine whether requesting bss params or local params of the sta or ap */
	if (ireq->i_len & IEEE80211_WMEPARAM_BSS || !WLIF_AP_MODE(wl, wlif))
		iov_name = "wme_ac_sta";
	else
		iov_name = "wme_ac_ap";

	if ((error = wl_iovar_op(wl, iov_name, NULL, 0, &acps, sizeof(acps),
	                         IOV_GET, wlif, wlif->wlcif)))
		return error;

	acp = &acps[ac];

	/* Decide the iovar based on AP or STA mode */
	switch (ireq->i_type) {
	case IEEE80211_IOC_WME_CWMIN:		/* WME: CWmin */
		ireq->i_val = acp->ECW & EDCF_ECWMIN_MASK;
		break;
	case IEEE80211_IOC_WME_CWMAX:		/* WME: CWmax */
		ireq->i_val = (acp->ECW & EDCF_ECWMAX_MASK) >> EDCF_ECWMAX_SHIFT;
		break;
	case IEEE80211_IOC_WME_AIFS:		/* WME: AIFS */
		ireq->i_val = acp->ACI & EDCF_AIFSN_MASK;
		break;
	case IEEE80211_IOC_WME_TXOPLIMIT:	/* WME: txops limit */
		ireq->i_val = acp->TXOP;
		break;
	case IEEE80211_IOC_WME_ACM:		/* WME: ACM (bss only) */
		ireq->i_val = (acp->ACI & EDCF_ACM_MASK)? 1 : 0;
		break;
	case IEEE80211_IOC_WME_ACKPOLICY:	/* WME: ACK policy (!bss only) */
		if ((error = wl_iovar_getint(wl, "wme_noack", &int_val, wlif, wlif->wlcif)))
			break;
		ireq->i_val = int_val;
		break;
	}
	return error;
}

/* IEEE80211_IOC_WME_CWMIN, IEEE80211_IOC_WME_CWMAX, IEEE80211_IOC_WME_AIFS,
 * IEEE80211_IOC_WME_TXOPLIMIT, IEEE80211_IOC_WME_ACM, IEEE80211_IOC_WME_ACKPOLICY
 */
static int
wl_ieee80211_wmeparam_set(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int ac = (ireq->i_len & IEEE80211_WMEPARAM_VAL), aifsn, ecwmin, ecwmax, txop;
	edcf_acparam_t acps[AC_COUNT], *acp;
	int error = 0, int_val;
	const char *iov_name; /* IOVAR to use based on request and mode */
	wl_info_t *wl = wlif->wl;

	if (ac >= WME_NUM_AC)
		ac = WME_AC_BE;

	if (ireq->i_len & IEEE80211_WMEPARAM_BSS || !WLIF_AP_MODE(wl, wlif))
		iov_name = "wme_ac_sta";
	else
		iov_name = "wme_ac_ap";

	/* Get all the ac parameters so that we can just modify the relevant one */
	if ((error = wl_iovar_op(wl, iov_name, NULL, 0, &acps, sizeof(acps),
	                         IOV_GET, wlif, wlif->wlcif)))
		return error;

	acp = &acps[ac];

	/* Decide the iovar based on AP or STA mode */
	switch (ireq->i_type) {
	case IEEE80211_IOC_WME_CWMIN:		/* WME: CWmin */
		ecwmin = ireq->i_val;
		if (ecwmin >= EDCF_ECW_MIN && ecwmin <= EDCF_ECW_MAX)
			acp->ECW =
			        ((acp->ECW & EDCF_ECWMAX_MASK) |
			         (ecwmin & EDCF_ECWMIN_MASK));
		else
			error = EINVAL;
		break;
	case IEEE80211_IOC_WME_CWMAX:		/* WME: CWmax */
		ecwmax = ireq->i_val;
		if (ecwmax >= EDCF_ECW_MIN && ecwmax <= EDCF_ECW_MAX)
			acp->ECW =
			        ((ecwmax << EDCF_ECWMAX_SHIFT) & EDCF_ECWMAX_MASK) |
			        (acp->ECW & EDCF_ECWMIN_MASK);
		else
			error = EINVAL;
		break;
	case IEEE80211_IOC_WME_AIFS:		/* WME: AIFS */
		aifsn = ireq->i_val;
		if (aifsn >= EDCF_AIFSN_MIN && aifsn <= EDCF_AIFSN_MAX)
			acp->ACI =
			        (acp->ACI & ~EDCF_AIFSN_MASK) |
			        (aifsn & EDCF_AIFSN_MASK);
		else
			error = EINVAL;
		break;
	case IEEE80211_IOC_WME_TXOPLIMIT:	/* WME: txops limit */
		txop = ireq->i_val;
		if (txop >= EDCF_TXOP_MIN && txop <= EDCF_TXOP_MAX)
			acp->TXOP = txop;
		else
			error = EINVAL;
		break;
	case IEEE80211_IOC_WME_ACM:		/* WME: ACM (bss only) */
		if (ireq->i_val == 1)
			acp->ACI |= EDCF_ACM_MASK;
		else
			acp->ACI &= ~EDCF_ACM_MASK;
		break;
	case IEEE80211_IOC_WME_ACKPOLICY:	/* WME: ACK policy (!bss only) */
		int_val = ireq->i_val;
		return wl_iovar_setint(wl, "wme_noack", int_val, wlif, wlif->wlcif);
	}

	/* Write back the updated value */
	return wl_iovar_op(wl, iov_name, NULL, 0, acp, sizeof(edcf_acparam_t), IOV_SET,
		wlif, wlif->wlcif);
}


static int
wl_ieee80211_chanswitch(wl_if_t *wlif, struct ieee80211req *ireq)
{
	int error = 0;
	wl_chan_switch_t csa;
	struct ieee80211_chanswitch_req csr;
	wl_info_t *wl = wlif->wl;

	memset(&csa, 0, sizeof(csa));
	if (ireq->i_len != sizeof(csr))
		return EINVAL;

	error = copyin(ireq->i_data, &csr, sizeof(csr));
	if (error != 0)
		return error;

	error = wl_ieee80211chan_to_chanspec(wl, &csr.csa_chan, &csa.chspec);
	if (error != 0)
		return 0;

	csa.count = csr.csa_count;
	csa.mode = csr.csa_mode;

	return wl_iovar_op(wl, "csa", NULL, 0, &csa, sizeof(csa), IOV_SET, wlif, wlif->wlcif);
}

static int
wl_ieee80211_devmode(wl_if_t *wlif, int *mode)
{
	int error = 0, nmode, vhtmode;
	uint32 chanspec;
	wl_info_t *wl = wlif->wl;

	if ((error = wl_iovar_getint(wl, "chanspec", &chanspec, wlif, wlif->wlcif)))
		return error;

	if ((error = wl_iovar_getint(wl, "nmode", &nmode, wlif, wlif->wlcif)))
		return error;

	if ((error = wl_iovar_getint(wl, "vhtmode", &vhtmode, wlif, wlif->wlcif)))
		return error;

	if (vhtmode) {
		if (CHSPEC_IS2G(chanspec))
			*mode = IEEE80211_MODE_11NG; /* HT for 2.4GHz 11AC? */
		else
			*mode = IEEE80211_MODE_11AC;
	} else if (nmode) {
		if (CHSPEC_IS2G(chanspec))
			*mode = IEEE80211_MODE_11NG;
		else
			*mode = IEEE80211_MODE_11NA;
	} else {
		if (CHSPEC_IS2G(chanspec))
			*mode = IEEE80211_MODE_11G;
		else
			*mode = IEEE80211_MODE_11A;
	}

	return error;
}

static int
wl_ieee80211_txparams_set(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int error = 0;
	struct ieee80211_txparams_req parms;
	struct ieee80211_txparam *txparam;
	wl_info_t *wl = wlif->wl;
	int mode, mspec, lrl, chanspec, rspec;
	const char* iov;

	if (ireq->i_len != sizeof(parms))
		return EINVAL;
	if ((error = copyin(ireq->i_data, &parms, sizeof(parms))))
		return error;

	if ((error = wl_iovar_getint(wl, "chanspec", &chanspec, wlif, wlif->wlcif)))
		return error;

	/* get mode */
	if ((error = wl_ieee80211_devmode(wlif, &mode)))
		return error;

	txparam =  &parms.params[mode];

	/* check if ucastrate changed */
	if (txparam->ucastrate != wl->txparams->params[mode].ucastrate) {
		/* start with none */
		rspec = 0;
		if (txparam->ucastrate != IEEE80211_FIXED_RATE_NONE) {
			if ((mode >= IEEE80211_MODE_11A) && (mode < IEEE80211_MODE_11NA)) {
				rspec = (txparam->ucastrate & 0xff);
			} else if ((mode == IEEE80211_MODE_11NA) ||
				(mode == IEEE80211_MODE_11NG)) {
				if ((txparam->ucastrate & IEEE80211_RATE_MCS)) {
					rspec = (txparam->ucastrate & ~IEEE80211_RATE_MCS) |
						WL_RSPEC_ENCODE_HT;
				} else {
					rspec = (txparam->ucastrate & 0xff);
				}
			} else if (mode == IEEE80211_MODE_11AC) {
				if ((txparam->ucastrate & IEEE80211_RATE_VHT_MCS) ==
					IEEE80211_RATE_VHT_MCS) {
					rspec = (txparam->ucastrate & ~IEEE80211_RATE_VHT_MCS) |
						WL_RSPEC_ENCODE_VHT;
					rspec |= (txparam->ucastnss << WL_RSPEC_VHT_NSS_SHIFT);
				} else if ((txparam->ucastrate & IEEE80211_RATE_MCS)) {
					rspec = (txparam->ucastrate & ~IEEE80211_RATE_MCS) |
						WL_RSPEC_ENCODE_HT;
				} else {
					rspec = (txparam->ucastrate & 0xff);
				}
			}
		}
		if (CHSPEC_IS2G(chanspec))
			iov = "2g_rate";
		else
			iov = "5g_rate";
		if ((error = wl_iovar_setint(wl, iov, rspec, wlif, wlif->wlcif)))
			return error;
	}

	if (txparam->mcastrate != wl->txparams->params[mode].mcastrate) {
		/* start with none */
		mspec = 0;
		if (txparam->mcastrate) {
			if ((mode >= IEEE80211_MODE_11A) && (mode < IEEE80211_MODE_11NA)) {
				mspec = (txparam->mcastrate & 0xff);
			} else if ((mode == IEEE80211_MODE_11NA) || (mode == IEEE80211_MODE_11NG)) {
				if (txparam->mcastrate & IEEE80211_RATE_MCS) {
					mspec = (txparam->mcastrate & ~IEEE80211_RATE_MCS) |
						WL_RSPEC_ENCODE_HT;
				} else {
					mspec = (txparam->mcastrate & 0xff);
				}
			} else if (mode == IEEE80211_MODE_11AC) {
				if ((txparam->mcastrate & IEEE80211_RATE_VHT_MCS) ==
					IEEE80211_RATE_VHT_MCS) {
					mspec = (txparam->mcastrate & ~IEEE80211_RATE_VHT_MCS) |
						WL_RSPEC_ENCODE_VHT;
					mspec |= (txparam->mcastnss << WL_RSPEC_VHT_NSS_SHIFT);
				} else if ((txparam->mcastrate & IEEE80211_RATE_MCS)) {
					mspec = (txparam->mcastrate & ~IEEE80211_RATE_MCS) |
						WL_RSPEC_ENCODE_HT;
				} else {
					mspec = (txparam->mcastrate & 0xff);
				}
			}
		}

		if (CHSPEC_IS2G(chanspec))
			iov = "2g_mrate";
		else
			iov = "5g_mrate";

		if ((error = wl_iovar_setint(wl, iov, mspec, wlif, wlif->wlcif)))
			return error;
	}

	if (txparam->maxretry != wl->txparams->params[mode].maxretry) {
		lrl = txparam->maxretry;
		if ((error = wl_wlc_ioctl(ifp, WLC_SET_LRL, &lrl, sizeof(lrl))))
			return error;
	}

	return error;
}

static int
wl_ieee80211_txparams_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int mode, error = 0;
	wl_info_t *wl = wlif->wl;
	const char *iov_rate, *iov_mrate;
	int chanspec, rspec, mspec, lrl;
	uint8 rate, mrate, nss, mnss;

	if (ireq->i_len != sizeof(struct ieee80211_txparams_req))
		return EINVAL;

	if ((error = wl_iovar_getint(wl, "chanspec", &chanspec, wlif, wlif->wlcif)))
		return error;

	/* get mode */
	if ((error = wl_ieee80211_devmode(wlif, &mode)))
		return error;

	if (CHSPEC_IS2G(chanspec)) {
		iov_rate = "2g_rate";
		iov_mrate = "2g_mrate";
	} else {
		iov_rate = "5g_rate";
		iov_mrate = "5g_mrate";
	}

	if ((error =  wl_iovar_getint(wl, iov_mrate, &mspec, wlif, wlif->wlcif)))
		return error;

	if ((error = wl_iovar_getint(wl, iov_rate, &rspec, wlif, wlif->wlcif)))
		return error;

	if ((error = wl_wlc_ioctl(ifp, WLC_GET_LRL, &lrl, sizeof(lrl))))
		return error;

	/* start with none */
	rate = IEEE80211_FIXED_RATE_NONE;
	nss = 0;
	if (RSPEC_ISLEGACY(rspec)) {
		if ((mode >= IEEE80211_MODE_11A) && (mode < IEEE80211_MODE_11NA))
			rate = (rspec & WL_RSPEC_RATE_MASK);
	} else if (RSPEC_ISHT(rspec)) {
		if ((mode == IEEE80211_MODE_11NA) || (mode == IEEE80211_MODE_11NG))
			rate = (rspec & WL_RSPEC_RATE_MASK) | IEEE80211_RATE_MCS;
	} else if (RSPEC_ISVHT(rspec)) {
		if (mode == IEEE80211_MODE_11AC) {
			rate = (rspec & WL_RSPEC_VHT_MCS_MASK) | IEEE80211_RATE_VHT_MCS;
			nss = (rspec & WL_RSPEC_VHT_NSS_MASK) >> WL_RSPEC_VHT_NSS_SHIFT;
		}
	}

	/* start with none */
	mrate = 0;
	mnss = 0;
	if (RSPEC_ISLEGACY(mspec)) {
		if ((mode >= IEEE80211_MODE_11A) && (mode < IEEE80211_MODE_11NA))
			mrate = (mspec & WL_RSPEC_RATE_MASK);
	} else if (RSPEC_ISHT(mspec)) {
		if ((mode == IEEE80211_MODE_11NA) || (mode == IEEE80211_MODE_11NG))
			mrate = (mspec & WL_RSPEC_RATE_MASK) | IEEE80211_RATE_MCS;
	} else if (RSPEC_ISVHT(mspec)) {
		if (mode == IEEE80211_MODE_11AC) {
			mrate = (mspec & WL_RSPEC_VHT_MCS_MASK) | IEEE80211_RATE_VHT_MCS;
			mnss = (mspec & WL_RSPEC_VHT_NSS_MASK) >> WL_RSPEC_VHT_NSS_SHIFT;
		}
	}


	wl->txparams->params[mode].ucastrate = rate ? rate : IEEE80211_FIXED_RATE_NONE;
	wl->txparams->params[mode].ucastnss = nss;
	wl->txparams->params[mode].mgmtrate = 1; /* set to 1 so that ifconfig can display */
	wl->txparams->params[mode].mcastnss = mnss;
	wl->txparams->params[mode].mcastrate = mrate;
	wl->txparams->params[mode].maxretry = lrl;

	error = copyout(wl->txparams, ireq->i_data, sizeof(struct ieee80211_txparams_req));
	ireq->i_len = sizeof(struct ieee80211_txparams_req);

	return error;
}

static int
wl_ieee80211_appie_set(wl_if_t *wlif, struct ieee80211req *ireq)
{
	uint8_t		fc0;
	int			error = 0;
	fc0 = (ireq->i_val & 0xff) & IEEE80211_FC0_SUBTYPE_MASK;

	if ((fc0 & IEEE80211_FC0_TYPE_MASK) != IEEE80211_FC0_TYPE_MGT)
		return EINVAL;

	switch (fc0 & IEEE80211_FC0_SUBTYPE_MASK) {
		case IEEE80211_FC0_SUBTYPE_BEACON:
			error = setappie(wlif, &wlif->appie_beacon, &wlif->appie_beacon_len,
				ireq, fc0);
			break;
		case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
			error = setappie(wlif, &wlif->appie_proberesp, &wlif->appie_proberesp_len,
				ireq, fc0);
			break;
		case IEEE80211_FC0_SUBTYPE_ASSOC_RESP:
			error = setappie(wlif, &wlif->appie_assocresp, &wlif->appie_assocresp_len,
				ireq, fc0);
			break;
		case IEEE80211_FC0_SUBTYPE_PROBE_REQ:
			error = setappie(wlif, &wlif->appie_probereq, &wlif->appie_probereq_len,
				ireq, fc0);
			break;
		case IEEE80211_FC0_SUBTYPE_ASSOC_REQ:
			error = setappie(wlif, &wlif->appie_assocreq, &wlif->appie_assocreq_len,
				ireq, fc0);
			break;
		case (IEEE80211_APPIE_WPA & IEEE80211_FC0_SUBTYPE_MASK):
			error = setwpaauth(wlif, ireq);
			break;
		default:
			error = EINVAL;
			break;
	}
	return error;
}

static int
wl_ieee80211_appiebuf_set(wl_if_t *wlif, struct ieee80211req *ireq)
{
	int error;
	switch (ireq->i_val) {
		case IEEE80211_WPSIE_FRAME_BEACON:
			error = setappie(wlif, &wlif->appie_beacon_wps,
				&wlif->appie_beacon_wps_len, ireq, IEEE80211_FC0_SUBTYPE_BEACON);
			break;
		case IEEE80211_DWDSIE_FRAME_BEACON:
			error = setappie(wlif, &wlif->appie_beacon_dwds,
				&wlif->appie_beacon_dwds_len, ireq, IEEE80211_FC0_SUBTYPE_BEACON);
			break;
		case IEEE80211_WPSIE_FRAME_PROBE_REQ:
			error = setappie(wlif, &wlif->appie_probereq_wps,
				&wlif->appie_probereq_wps_len, ireq,
				IEEE80211_FC0_SUBTYPE_PROBE_REQ);
			break;
		case IEEE80211_WPSIE_FRAME_PROBE_RESP:
			error = setappie(wlif, &wlif->appie_proberesp_wps,
				&wlif->appie_proberesp_wps_len, ireq,
				IEEE80211_FC0_SUBTYPE_PROBE_RESP);
			break;
		case IEEE80211_DWDSIE_FRAME_PROBE_RESP:
			error = setappie(wlif, &wlif->appie_proberesp_dwds,
				&wlif->appie_proberesp_dwds_len, ireq,
				IEEE80211_FC0_SUBTYPE_PROBE_RESP);
			break;
		case IEEE80211_DWDSIE_FRAME_ASSOC_REQ:
			error = setappie(wlif, &wlif->appie_assocreq_dwds,
				&wlif->appie_assocreq_dwds_len, ireq,
				IEEE80211_FC0_SUBTYPE_ASSOC_REQ);
			break;
		case IEEE80211_DWDSIE_FRAME_ASSOC_RESP:
			error = setappie(wlif, &wlif->appie_assocresp_dwds,
				&wlif->appie_assocresp_dwds_len, ireq,
				IEEE80211_FC0_SUBTYPE_ASSOC_RESP);
			break;
#if APPLE_HAVE_AUTO_WIRELESS_CONFIG
		case IEEE80211_AWPIE_FRAME_BEACON:
			error = setappie(wlif, &wlif->appie_beacon_awp,
				&wlif->appie_beacon_awp_len, ireq, IEEE80211_FC0_SUBTYPE_BEACON);
			break;
		case IEEE80211_AWPIE_FRAME_PROBE_REQ:
			error = setappie(wlif, &wlif->appie_probereq_awp,
				&wlif->appie_probereq_awp_len, ireq,
				IEEE80211_FC0_SUBTYPE_PROBE_REQ);
			break;
		case IEEE80211_AWPIE_FRAME_PROBE_RESP:
			error = setappie(wlif, &wlif->appie_proberesp_awp,
				&wlif->appie_proberesp_awp_len, ireq,
				IEEE80211_FC0_SUBTYPE_PROBE_RESP);
			break;
#endif
		default:
			error = EINVAL;
	}
	return error;
}

static int
wl_ieee80211_regdomain_set(wl_if_t *wlif, struct ieee80211req *ireq)
{
	int error = 0;
	struct ieee80211_regdomain ic_regdomain;
	char country_abbrev[WLC_CNTRY_BUF_SZ];
	wl_info_t *wl = wlif->wl;

	if (ireq->i_len != sizeof(struct ieee80211_regdomain_req))
		return EINVAL;

	error = copyin(ireq->i_data, &ic_regdomain, sizeof(struct ieee80211_regdomain));
	if (error != 0)
		return error;

	memset(&country_abbrev[0], 0, sizeof(country_abbrev));
	strncpy(country_abbrev, &ic_regdomain.isocc[0], 2);
	return wl_iovar_op(wl, "country_abbrev_override", NULL, 0, &country_abbrev[0], 2,
	                   IOV_SET, wlif, wlif->wlcif);
}

static int
wl_ieee80211_rrm_enabled(wl_if_t *wlif, struct ifnet *ifp, int *enabled)
{
	char *buf = NULL;
	int buflen;
	dot11_rrm_cap_ie_t *rrm_cap;
	*enabled = 0;
	wl_info_t *wl = wlif->wl;
	int error;

	buflen = strlen("rrm") + 1 + DOT11_RRM_CAP_LEN;

	if (!(buf = (void *) MALLOC(wl->osh, buflen))) {
		return BCME_NORESOURCE;
	}
	memset(buf, 0, buflen);
	strcpy(buf, "rrm");
	error = wl_wlc_ioctl(ifp, WLC_GET_VAR, buf, buflen);
	if (!error) {
		rrm_cap = (dot11_rrm_cap_ie_t *)buf;

		if ((rrm_cap->cap[DOT11_RRM_CAP_NEIGHBOR_REPORT / NBBY] &
			(1 << (DOT11_RRM_CAP_NEIGHBOR_REPORT % NBBY))) &&
			(rrm_cap->cap[DOT11_RRM_CAP_AP_CHANREP / NBBY] &
			(1 << (DOT11_RRM_CAP_AP_CHANREP % NBBY)))) {

			*enabled = 1;
		} else {
			*enabled = 0;
		}
	}
	MFREE(wl->osh, buf, buflen);
	return error;
}

/* SIOCG80211: Basic entry point for Get Net80211 ioctls */
static int
wl_ieee80211_ioctl_get(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int error = 0;
	wl_info_t *wl = wlif->wl;
	int int_val, wep_restrict, eap_restrict, wsec, defkey;
	tx_power_legacy_t power;
	int i, wpa_auth, auth;

	WL_INFORM(("wl%d: cmd:%d\n", wl->pub->unit, ireq->i_type));

	switch (ireq->i_type) {
	case IEEE80211_IOC_SSID: {
		wlc_ssid_t ssid;

		error = wl_iovar_op(wl, "ssid", NULL, 0, &ssid, sizeof(ssid), IOV_GET,
		                    wlif, wlif->wlcif);
		if (error)
			break;

		error = copyout(ssid.SSID, ireq->i_data, ssid.SSID_len);
		ireq->i_len = ssid.SSID_len;
		break;
		}
	case IEEE80211_IOC_NUMSSIDS:
		ireq->i_val = 1;
		break;
	case IEEE80211_IOC_WEP:
		if ((error = wl_iovar_getint(wl, "wsec", &wsec, wlif, wlif->wlcif)))
			break;
		if ((error = wl_wlc_ioctl(ifp, WLC_GET_WEP_RESTRICT, &wep_restrict,
			sizeof(wep_restrict))))
			break;
		if (WSEC_WEP_ENABLED(wsec)) {
			if (wep_restrict)
				ireq->i_val = IEEE80211_WEP_ON;
			else
				ireq->i_val = IEEE80211_WEP_MIXED;
		} else
			ireq->i_val = IEEE80211_WEP_OFF;
		break;
	case IEEE80211_IOC_WEPTXKEY:
		/* ifieee80211.c tries to query deftxkey even after wep off */
		/* find the default tx key */
		i = 0;
		do {
			defkey = i;
			if ((error = wl_wlc_ioctl(ifp, WLC_GET_KEY_PRIMARY, &defkey,
				sizeof(defkey))))
				break;

			if (defkey) {
				defkey = i;
				break;
			}
		} while (++i < DOT11_MAX_DEFAULT_KEYS);

		if (error)
			ireq->i_val = IEEE80211_KEYIX_NONE;
		else
			ireq->i_val = defkey;
		error = 0;
		break;
	case IEEE80211_IOC_NUMWEPKEYS:
		ireq->i_val = IEEE80211_WEP_NKID;
		break;
	case IEEE80211_IOC_BSSID: {
			char bssid[IEEE80211_ADDR_LEN];

			if (ireq->i_len < IEEE80211_ADDR_LEN) {
				error = BCME_NOMEM;
				break;
			}

			if ((error = wl_wlc_ioctl(ifp, WLC_GET_BSSID, &bssid, sizeof(bssid))))
				break;

			error = copyout(bssid, ireq->i_data, IEEE80211_ADDR_LEN);
			break;
		}
	case IEEE80211_IOC_CURCHAN: {
		struct ieee80211_channel chan;

		bzero(&chan, sizeof(chan));

		wl_ieee80211_curchan_get(wlif, &chan);

		copyout(&chan, ireq->i_data, sizeof(chan));
		ireq->i_len = sizeof(chan);
		break;
		}
	case IEEE80211_IOC_DOTH:
		error = wl_wlc_ioctl(ifp, WLC_GET_SPECT_MANAGMENT, &int_val, sizeof(int_val));
		ireq->i_val = (int_val != SPECT_MNGMT_STRICT_11D) ? int_val : 0;
		break;
	case IEEE80211_IOC_DOTD:
		error = wl_wlc_ioctl(ifp, WLC_GET_REGULATORY, &int_val, sizeof(int_val));
		ireq->i_val = int_val;
		break;
	case IEEE80211_IOC_DFS:
		error = wl_wlc_ioctl(ifp, WLC_GET_RADAR, &int_val, sizeof(int_val));
		ireq->i_val = int_val;
		break;
	case IEEE80211_IOC_BURST:
		error = wl_wlc_ioctl(ifp, WLC_GET_FAKEFRAG, &int_val, sizeof(int_val));
		ireq->i_val = int_val;
		break;
	case IEEE80211_IOC_RIFS:
		error = wl_iovar_getint(wl, "rifs", &int_val, wlif, wlif->wlcif);
		ireq->i_val = int_val;
		break;
	case IEEE80211_IOC_INACTIVITY:
		error = wl_wlc_ioctl(ifp, WLC_GET_SCB_TIMEOUT, &int_val, sizeof(int_val));
		ireq->i_val = int_val ? 1 : 0;
		break;
	case IEEE80211_IOC_TXPARAMS: {
		error = wl_ieee80211_txparams_get(wlif, ifp, ireq);
		break;
		}
	case IEEE80211_IOC_HTCONF:
		error = wl_iovar_getint(wl, "nmode", &int_val, wlif, wlif->wlcif);
		if (!error) {
			ireq->i_val = (int_val ? 3:0);
		}

		break;
	case IEEE80211_IOC_AMPDU: {
		int ampdu_tx = 0, ampdu_rx = 0;

		ireq->i_val = 0;

		error = wl_iovar_getint(wl, "ampdu_tx", &ampdu_tx, wlif, wlif->wlcif);
		if (!error)
			error = wl_iovar_getint(wl, "ampdu_rx", &ampdu_rx, wlif, wlif->wlcif);
		if (ampdu_tx)
			ireq->i_val |= 1;
		if (ampdu_rx)
			ireq->i_val |= 2;
		break;
		}
	case IEEE80211_IOC_AMPDU_LIMIT: {
		int rx_factor = 0;
		error = wl_iovar_getint(wl, "ampdu_rx_factor", &rx_factor, wlif, wlif->wlcif);
		ireq->i_val = rx_factor;
		break;
		}
	case IEEE80211_IOC_AMPDU_DENSITY: {
		int ampdu_rx_density = 0;
		error = wl_iovar_getint(wl, "ampdu_rx_density", &ampdu_rx_density,
		                        wlif, wlif->wlcif);
		ireq->i_val = ampdu_rx_density;
		break;
		}
	case IEEE80211_IOC_AMSDU: {
		int amsdu = 0, nmode = 0;

		error = wl_iovar_getint(wl, "amsdu", &amsdu, wlif, wlif->wlcif);
		/* amsdu tx */
		if (!error && amsdu)
			ireq->i_val |= 1;

		error = wl_iovar_getint(wl, "nmode", &nmode, wlif, wlif->wlcif);
		/* amsdu rx */
		if (!error && nmode)
			ireq->i_val |= 2;
		break;
		}
	case IEEE80211_IOC_REGDOMAIN: {
		/* just return these defaults for now */
		struct ieee80211_regdomain ic_regdomain;

		if (ireq->i_len < sizeof(ic_regdomain)) {
			error = BCME_NOMEM;
			break;
		}
		memset(&ic_regdomain, 0, sizeof(ic_regdomain));
		ic_regdomain.country  = CTRY_UNITED_STATES;
		ic_regdomain.dfsdomain = DFS_DOMAIN_NONE;
		ic_regdomain.location = ' ';
		ic_regdomain.isocc[0] = 'U';
		ic_regdomain.isocc[1] = 'S';
		ic_regdomain.regdomain = SKU_FCC;
		error = copyout(&ic_regdomain, ireq->i_data, sizeof(ic_regdomain));
		break;
		}
	case IEEE80211_IOC_DEVCAPS:{
		struct ieee80211_devcaps_req *dc;
		uint16 cap;

		if (ireq->i_len != sizeof(struct ieee80211_devcaps_req))
			return EINVAL;

		dc = MALLOC(wl->osh, sizeof(struct ieee80211_devcaps_req));
		if (dc == NULL)
			return ENOMEM;

		memset(dc, 0, sizeof(struct ieee80211_devcaps_req));

		dc->dc_drivercaps =
			IEEE80211_C_STA		/* station mode */
			| IEEE80211_C_HOSTAP		/* hostap mode */
			| IEEE80211_C_WDS		/* 4-address traffic works */
			| IEEE80211_C_MONITOR		/* monitor mode */
			| IEEE80211_C_SHPREAMBLE	/* short preamble supported */
			| IEEE80211_C_SHSLOT		/* short slot time supported */
			| IEEE80211_C_WPA		/* capable of WPA1+WPA2 */
			| IEEE80211_C_BGSCAN		/* capable of bg scanning */
			| IEEE80211_C_BURST		/* capable of bursting */
			| IEEE80211_C_WME		/* capable of WME/EDCF */
			| IEEE80211_C_TXPMGT
			| IEEE80211_C_AMPDU_NOSEQ
			| IEEE80211_C_NOAMSDUAMPDU
			| IEEE80211_C_CQ_STATS
			;
		dc->dc_cryptocaps = IEEE80211_CRYPTO_WEP | IEEE80211_CRYPTO_AES_CCM;
		if (PHYTYPE_11N_CAP(wl->wlc->band->phytype))
			dc->dc_htcaps =
				IEEE80211_HTCAP_SMPS_OFF	/* SM PS mode off */
				| IEEE80211_HTCAP_CHWIDTH40	/* 40MHz chan width */
				| IEEE80211_HTCAP_MAXAMSDU_3839	/* max A-MSDU length */
				| IEEE80211_HTCAP_DSSSCCK40	/* DSSS/CCK in 40MHz */
				/* s/w capabilities */
				| IEEE80211_HTC_HT		/* HT operation */
				| IEEE80211_HTC_AMPDU;

		cap = wlc_ht_get_cap(wl->wlc->hti);

		if (cap & HT_CAP_SHORT_GI_20)
			dc->dc_htcaps = dc->dc_htcaps | IEEE80211_HTCAP_SHORTGI20;

		if (cap & HT_CAP_SHORT_GI_40)
			dc->dc_htcaps = dc->dc_htcaps | IEEE80211_HTCAP_SHORTGI40;

		if (cap & HT_CAP_LDPC_CODING)
			dc->dc_htcaps = dc->dc_htcaps | IEEE80211_HTCAP_LDPC;

		switch ((cap & HT_CAP_RX_STBC_MASK) >> HT_CAP_RX_STBC_SHIFT) {
			case HT_CAP_RX_STBC_THREE_STREAM:
				dc->dc_htcaps |= IEEE80211_HTCAP_RXSTBC_3STREAM;
				break;
			case HT_CAP_RX_STBC_TWO_STREAM:
				dc->dc_htcaps |= IEEE80211_HTCAP_RXSTBC_2STREAM;
				break;
			case HT_CAP_RX_STBC_ONE_STREAM:
				dc->dc_htcaps |= IEEE80211_HTCAP_RXSTBC_1STREAM;
				break;
		}
		if (cap & HT_CAP_TX_STBC)
			dc->dc_htcaps |= IEEE80211_HTCAP_TXSTBC;

		if (WLC_PHY_11N_CAP(wl->wlc->band))
			dc->dc_htcaps |= IEEE80211_HTC_SMPS;

		memcpy(dc->dc_chaninfo.ic_chans, wl->chans,
			sizeof(struct ieee80211_channel) * wl->nchans);
		dc->dc_chaninfo.ic_nchans = wl->nchans;
		dc->dc_modecaps = wl->modecaps;

		dc->dc_applecaps = IEEE80211_APPLE_C_DWDS_AMPDU_NONE |
			IEEE80211_APPLE_C_DWDS_AMPDU_WAR |
			IEEE80211_APPLE_C_DWDS_AMPDU_FULL;

		error = copyout(dc, ireq->i_data, sizeof(*dc));
		MFREE(wl->osh, dc, sizeof(*dc));
		break;
		}
	case IEEE80211_IOC_AUTHMODE: {
		if ((error = wl_iovar_getint(wl, "wpa_auth", &wpa_auth, wlif, wlif->wlcif)))
			break;
		if ((error = wl_iovar_getint(wl, "auth", &auth, wlif, wlif->wlcif)))
			break;

		if (AUTH_INCLUDES_PSK(wpa_auth))
			ireq->i_val = IEEE80211_AUTH_WPA;
		else if (AUTH_INCLUDES_UNSPEC(wpa_auth))
			ireq->i_val = IEEE80211_AUTH_8021X;
		else if (auth == WL_AUTH_SHARED_KEY)
			ireq->i_val = IEEE80211_AUTH_SHARED;
		else if (auth == WL_AUTH_OPEN_SYSTEM)
			ireq->i_val = IEEE80211_AUTH_OPEN;

		break;
	}
	case IEEE80211_IOC_RTSTHRESHOLD:
		if ((error = wl_iovar_getint(wl, "rtsthresh", &int_val, wlif, wlif->wlcif)))
			break;
		ireq->i_val = int_val;
		break;
	case IEEE80211_IOC_PROTMODE: /* Protection mode - OFF/CTS / RTSCTS */
#if NOT_YET
		if ((error = wl_wlc_ioctl(ifp, WLC_GET_PHYTYPE, &int_val, sizeof(int_val))))
			break;
		if (int_val == PHY_TYPE_N) {
			if ((error = wl_iovar_getint(wl, "nmode_protection", &int_val,
			                             wlif, wlif->wlcif)))
				break;
			if (int_val == WLC_N_PROTECTION_20IN40)
				ireq->i_val = IEEE80211_PROT_CTSONLY;
			else if (int_val == WLC_N_PROTECTION_OFF)
				ireq->i_val = IEEE80211_PROT_NON;
			break;
		}
#endif
		if ((error = wl_wlc_ioctl(ifp, WLC_GET_GMODE_PROTECTION, &int_val,
		                          sizeof(int_val))))
			break;

		ireq->i_val = (int_val == WLC_PROTECTION_ON)? IEEE80211_PROT_CTSONLY:
		        IEEE80211_PROT_NONE;
		break;
	case IEEE80211_IOC_TXPOWER: /* Global tx power limit */
		if ((error = wl_wlc_ioctl(ifp, WLC_CURRENT_PWR, &power, sizeof(power))))
			break;
		/* In .5 dbm */
		ireq->i_val = MIN(power.txpwr_band_max[0], power.txpwr_local_max) / 2;
		break;
	case IEEE80211_IOC_WPA: {
		/* Return the current WPA version */
		if ((error = wl_iovar_getint(wl, "wpa_auth", &wpa_auth, wlif, wlif->wlcif)))
			break;
		if (bcmwpa_includes_wpa_auth(wpa_auth) && bcmwpa_includes_wpa2_auth(wpa_auth))
			ireq->i_val = 3;
		else if (bcmwpa_includes_wpa_auth(wpa_auth))
			ireq->i_val = 1;
		else if (bcmwpa_includes_wpa2_auth(wpa_auth))
			ireq->i_val = 2;
		else
			ireq->i_val = 0;
		break;
	}
	case IEEE80211_IOC_CHANLIST: /* WLC_GET_VALID_CHANNELS */
		return wl_ieee80211_chanlist_get(wlif, ifp, ireq);
	case IEEE80211_IOC_ROAMING:
		break;
	case IEEE80211_IOC_PRIVACY:
		if ((error = wl_iovar_getint(wl, "wsec", &wsec, wlif, wlif->wlcif)))
			break;
		ireq->i_val = WSEC_ENABLED(wsec);
		break;
	case IEEE80211_IOC_DROPUNENCRYPTED:
		if ((error = wl_wlc_ioctl(ifp, WLC_GET_WEP_RESTRICT, &wep_restrict,
		                          sizeof(wep_restrict))))
			break;
		if ((error = wl_iovar_getint(wl, "eap_restrict", &eap_restrict, wlif, wlif->wlcif)))
			break;
		ireq->i_val = wep_restrict || eap_restrict;
		break;
	case IEEE80211_IOC_WME:
		if ((error = wl_iovar_getint(wl, "wme", &int_val, wlif, wlif->wlcif)))
			break;
		ireq->i_val = int_val;
		break;
	case IEEE80211_IOC_HIDESSID:
		if ((error = wl_iovar_getint(wl, "closednet", &int_val, wlif, wlif->wlcif)))
			break;
		ireq->i_val = int_val;
		break;
	case IEEE80211_IOC_APBRIDGE:
		if ((error = wl_iovar_getint(wl, "ap_isolate", &int_val, wlif, wlif->wlcif)))
			break;
		ireq->i_val = !int_val;
		break;
	case IEEE80211_IOC_WPAKEY:
		return wl_ieee80211_wpakey_get(wlif, ifp, ireq);
	case IEEE80211_IOC_CHANINFO: /* WLC_GET_VALID_CHANNELS "per_chan_info" */
		return wl_ieee80211_chaninfo_get(wlif, ifp, ireq);
	case IEEE80211_IOC_WPAIE: /* Per STA WPA IE */
		return wl_ieee80211_wpaie_get(wlif, ifp, ireq);
	case IEEE80211_IOC_SCAN_RESULTS: /* WLC_SCAN_RESULTS */
		return wl_ieee80211_scanresults_get(wlif, ifp, ireq);
	case IEEE80211_IOC_CQSCAN_RESULTS: /* IOV_CHANIM_STATS */
		return wl_ieee80211_cq_scanresults_get(wlif, ifp, ireq);
	case IEEE80211_IOC_STA_INFO: /* "sta_info" */
		return wl_ieee80211_stainfo_get(wlif, ifp, ireq);
	case IEEE80211_IOC_STA_STATS:
		return wl_ieee80211_stastats_get(wlif, ifp, ireq);
	case IEEE80211_IOC_WME_CWMIN:		/* WME: CWmin */
	case IEEE80211_IOC_WME_CWMAX:		/* WME: CWmax */
	case IEEE80211_IOC_WME_AIFS:		/* WME: AIFS */
	case IEEE80211_IOC_WME_TXOPLIMIT:	/* WME: txops limit */
	case IEEE80211_IOC_WME_ACM:		/* WME: ACM (bss only) */
	case IEEE80211_IOC_WME_ACKPOLICY:	/* WME: ACK policy (bss only) */
		return wl_ieee80211_wmeparam_get(wlif, ifp, ireq);
	case IEEE80211_IOC_DTIM_PERIOD: {
		uint8 dtimp;
		error = wl_wlc_ioctl(ifp, WLC_GET_DTIMPRD, &dtimp, sizeof(dtimp));
		ireq->i_val = dtimp;
		break;
		}
	case IEEE80211_IOC_BEACON_INTERVAL: {
		uint16 bi;
		error = wl_wlc_ioctl(ifp, WLC_GET_BCNPRD, &bi, sizeof(bi));
		ireq->i_val = bi;
		break;
		}
	case IEEE80211_IOC_PUREG: { /* WLC_GET_GMODE - GMODE only */
		uint8 gmode;
		error = wl_wlc_ioctl(ifp, WLC_GET_GMODE, &gmode, sizeof(gmode));
		ireq->i_val = (gmode & GMODE_ONLY) != 0;
		break;
		}
	case IEEE80211_IOC_FRAGTHRESHOLD:
		if ((error = wl_iovar_getint(wl, "fragthresh", &int_val, wlif, wlif->wlcif)))
			break;
		ireq->i_val = int_val;
		break;
	case IEEE80211_IOC_MACCMD: /* MAC Mode */
		return wl_ieee80211_macmode_get(wlif, ifp, ireq);
	case IEEE80211_IOC_VHTCONF:
		error = wl_iovar_getint(wl, "vhtmode", &int_val, wlif, wlif->wlcif);
		if (!error)
			ireq->i_val = int_val;
		break;
	case IEEE80211_IOC_LDPC: {
		int ldpc_cap = 0;
		error = wl_iovar_getint(wl, "ldpc_cap", &ldpc_cap, wlif, wlif->wlcif);
		if (!error)
			ireq->i_val = (ldpc_cap != OFF);
		break;
		}

#ifdef WL11K
	case IEEE80211_IOC_RRM: {
		int enabled = 0;
		error = wl_ieee80211_rrm_enabled(wlif, ifp, &enabled);
		ireq->i_val = enabled;
		break;
		}

	case IEEE80211_IOC_RRM_NEIGHBOR_LIST: {
		char *buf = NULL;
		int buflen;
		uint16 list_cnt;

		if (ireq->i_data == NULL && ireq->i_len == 0) {
			if (!(buf = (void *) MALLOC(wl->osh, WLC_IOCTL_MAXLEN))) {
				error = BCME_NORESOURCE;
				return error;
			}
			memset(buf, 0, WLC_IOCTL_MAXLEN);
			strcpy(buf, "rrm_nbr_list");
			buflen = strlen("rrm_nbr_list") + 1;

			error = wl_wlc_ioctl(ifp, WLC_GET_VAR, buf, buflen);
			if (!error)
				ireq->i_len = *(uint16 *)buf;
			MFREE(wl->osh, buf, WLC_IOCTL_MAXLEN);
			return error;
		}

		if (!(buf = (void *) MALLOC(wl->osh, WLC_IOCTL_MAXLEN))) {
			error = BCME_NORESOURCE;
			return error;
		}
		memset(buf, 0, WLC_IOCTL_MAXLEN);
		strcpy(buf, "rrm_nbr_list");
		buflen = strlen("rrm_nbr_list") + 1;
		list_cnt = ireq->i_len / (TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN);
		memcpy(&buf[buflen], &list_cnt, sizeof(uint16));

		error = wl_wlc_ioctl(ifp, WLC_GET_VAR, buf, WLC_IOCTL_MAXLEN);

		if (!error) {
			if (ireq->i_data != NULL)
				error = copyout(buf, ireq->i_data, ireq->i_len);
		}
		MFREE(wl->osh, buf, WLC_IOCTL_MAXLEN);

		break;
		}
#endif /* WL11K */
	case IEEE80211_IOC_GETCAPINFO: {
		char *buf = NULL;
		wl_bss_info_t *bi;
		if (!(buf = (void *) MALLOC(wl->osh, WLC_IOCTL_MAXLEN))) {
			error = BCME_NORESOURCE;
			return error;
		}
		memset(buf, 0, WLC_IOCTL_MAXLEN);
		*(uint32*)buf = WLC_IOCTL_MAXLEN;
		error = wl_wlc_ioctl(ifp, WLC_GET_BSS_INFO, buf, WLC_IOCTL_MAXLEN);
		if (!error) {
			bi = (wl_bss_info_t*)(buf + 4);
			ireq->i_val = bi->capability;
		}
		MFREE(wl->osh, buf, WLC_IOCTL_MAXLEN);
		return error;

		}

	default:
		return EINVAL;
	}

	return error;
}

/* SIOCS80211: Basic entry point for Set Net80211 ioctls */
static int
wl_ieee80211_ioctl_set(wl_if_t *wlif, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int error = 0;
	int wsec, int_val, wpa_auth, auth;
	bool wasup = FALSE;
	wlc_ssid_t ssid;
	wl_info_t *wl = wlif->wl;

	WL_INFORM(("wl%d:%s cmd:%d\n", wl->pub->unit, __func__, ireq->i_type));

	switch (ireq->i_type) {
	case IEEE80211_IOC_SSID:
		if (WLIF_AP_MODE(wl, wlif)) {
			if (ireq->i_val != 0 || ireq->i_len > IEEE80211_NWID_LEN)
				return EINVAL;
			error = copyin(ireq->i_data, ssid.SSID, ireq->i_len);
			if (error)
				break;
			ssid.SSID_len = ireq->i_len;
			error = wl_iovar_op(wl, "ssid", NULL, 0, &ssid, sizeof(ssid),
			                    IOV_SET, wlif, wlif->wlcif);
		}
		break;
	case IEEE80211_IOC_WEP:
		if ((error = wl_iovar_getint(wl, "wsec", &wsec, wlif, wlif->wlcif)))
			break;
		switch (ireq->i_val) {
		case IEEE80211_WEP_OFF:
			wsec &= ~WEP_ENABLED;
			int_val = 0;
			break;
		case IEEE80211_WEP_ON:
			wsec |= WEP_ENABLED;
			int_val = 1;
			break;
		case IEEE80211_WEP_MIXED:
			wsec |= WEP_ENABLED;
			int_val = 0;
			break;
		}
		if ((error = wl_iovar_setint(wl, "wsec", wsec, wlif, wlif->wlcif)))
			break;
		return wl_wlc_ioctl(ifp, WLC_SET_WEP_RESTRICT, &int_val, sizeof(int_val));
	case IEEE80211_IOC_WEPTXKEY: {
		uint32 kid;

		kid = (uint32) ireq->i_val;
		if (kid >= IEEE80211_WEP_NKID &&
		    (uint16_t) kid != IEEE80211_KEYIX_NONE)
			return EINVAL;

		error = wl_wlc_ioctl(ifp, WLC_SET_KEY_PRIMARY, &kid, sizeof(kid));
		break;
		}
	case IEEE80211_IOC_CURCHAN: {
		struct ieee80211_channel chan;
		chanspec_t chanspec = 0;
		uint32 c;

		if (ireq->i_len != sizeof(chan))
			return EINVAL;

		error = copyin(ireq->i_data, &chan, sizeof(chan));
		if (error != 0)
			return error;

		error = wl_ieee80211chan_to_chanspec(wl, &chan, &chanspec);
		if (error != 0)
			return error;

		/* not sure why you have to do this ughh */
		c = (uint32)chanspec;
		error = wl_iovar_op(wl, "chanspec", NULL, 0, &c, sizeof(c),
			IOV_SET, wlif, wlif->wlcif);
		break;
		}
	case IEEE80211_IOC_DOTH:
		/* dont do anything here since we init at attach and this doesnt work well
		 * per virtual interface
		 */
		break;
	case IEEE80211_IOC_DOTD:
		/* dont do anything here since we init at attach and this doesnt work well
		 * per virtual interface
		 */
		break;
	case IEEE80211_IOC_DFS:
		int_val = ireq->i_val ? 1 : 0;

		error = wl_wlc_ioctl(ifp, WLC_SET_RADAR, &int_val, sizeof(int_val));
		break;
	case IEEE80211_IOC_BURST:
		int_val = ireq->i_val ? 1 : 0;

		error = wl_wlc_ioctl(ifp, WLC_SET_FAKEFRAG, &int_val, sizeof(int_val));
		break;
	case IEEE80211_IOC_RIFS:
		int_val = ireq->i_val ? 1 : 0;

		error = wl_iovar_setint(wl, "rifs", int_val, wlif, wlif->wlcif);
		break;
	case IEEE80211_IOC_AMPDU: {
		/* ignore
		 * AMPDU settings from user mode since we cant enable/disable ampdu per
		 * virtual interface.  And since we always do the 4 addr TID WAR this really
		 * doesnt matter
		 */
		error = 0;
		break;
		}
	case IEEE80211_IOC_AMPDU_LIMIT: {
		int rx_factor = 0;

		if (!(IEEE80211_HTCAP_MAXRXAMPDU_8K <= ireq->i_val &&
		      ireq->i_val <= IEEE80211_HTCAP_MAXRXAMPDU_64K))
			return EINVAL;

		rx_factor = ireq->i_val;

		if (wl->pub->up) {
			wl_down(wl);
			wasup = TRUE;
		}

		error = wl_iovar_setint(wl, "ampdu_rx_factor", rx_factor, wlif, wlif->wlcif);

		if (wasup)
			wl_up(wl);
		break;
		}
	case IEEE80211_IOC_AMPDU_DENSITY: {
		int ampdu_rx_density = 0;

		if (!(IEEE80211_HTCAP_MPDUDENSITY_NA <= ireq->i_val &&
		      ireq->i_val <= IEEE80211_HTCAP_MPDUDENSITY_16))
			return EINVAL;

		ampdu_rx_density = ireq->i_val;

		if (wl->pub->up) {
			wl_down(wl);
			wasup = TRUE;
		}

		error = wl_iovar_setint(wl, "ampdu_rx_density", ampdu_rx_density,
		                        wlif, wlif->wlcif);

		if (wasup)
			wl_up(wl);
		break;
		}
	case IEEE80211_IOC_AMSDU: {
		int amsdu_tx;

		/* need to check dev cap? */

		if (ireq->i_val & 1)
			amsdu_tx = 1;
		else
			amsdu_tx = 0;

		if (ireq->i_val & 2) {
			/* Ignore setting amsdu rx */
		}

		if (wl->pub->up) {
			wl_down(wl);
			wasup = TRUE;
		}

		error = wl_iovar_setint(wl, "amsdu", amsdu_tx, wlif, wlif->wlcif);

		if (wasup)
			wl_up(wl);
		break;
		}
	case IEEE80211_IOC_AUTHMODE: {


		switch (ireq->i_val) {
		case IEEE80211_AUTH_WPA:
		case IEEE80211_AUTH_8021X: /* 802.1x */
		case IEEE80211_AUTH_OPEN: /* open */
		case IEEE80211_AUTH_AUTO: /* auto */
			auth = DOT11_OPEN_SYSTEM;
			break;

		case IEEE80211_AUTH_SHARED: /* shared-key */
			auth = DOT11_SHARED_KEY;
		default:
			return EINVAL;
		}
		return wl_iovar_setint(wl, "auth", auth, wlif, wlif->wlcif);

	}
	case IEEE80211_IOC_RTSTHRESHOLD:
		return wl_iovar_setint(wl, "rtsthresh", ireq->i_val, wlif, wlif->wlcif);
	case IEEE80211_IOC_PROTMODE:
		if (ireq->i_val > IEEE80211_PROT_RTSCTS)
			return EINVAL;

		if ((ireq->i_val == IEEE80211_PROT_RTSCTS) ||
			(ireq->i_val == IEEE80211_PROT_CTSONLY))
			int_val = WLC_PROTECTION_AUTO;
		else
			int_val = WLC_PROTECTION_OFF;

		return wl_wlc_ioctl(ifp, WLC_SET_GMODE_PROTECTION_OVERRIDE,
		                    &int_val, sizeof(int_val));
	case IEEE80211_IOC_TXPOWER:
		/* multiply by 2 for half dbm to qdm */
		int_val = ireq->i_val * 2; /* Input is in .5 dbM */
		return wl_iovar_setint(wl, "qtxpower", int_val, wlif, wlif->wlcif);
	case IEEE80211_IOC_ROAMING:
		break;
	case IEEE80211_IOC_PRIVACY:
		int_val = ireq->i_val;

		error = wl_wlc_ioctl(ifp, WLC_SET_WEP_RESTRICT, &int_val, sizeof(int_val));
		break;
	case IEEE80211_IOC_DROPUNENCRYPTED: {
		if (WLIF_AP_MODE(wl, wlif)) {
			int_val = ireq->i_val;
			if ((error = wl_wlc_ioctl(ifp, WLC_SET_WEP_RESTRICT,
			                          &int_val, sizeof(int_val))))
				break;
			if ((error = wl_iovar_getint(wl, "wpa_auth", &wpa_auth, wlif, wlif->wlcif)))
				break;
			if (wpa_auth != WPA_AUTH_DISABLED)
				return (wl_iovar_setint(wl, "eap_restrict", int_val,
				                        wlif, wlif->wlcif));
		} else {
			/* need to figure out what needs to be done here for sta mode */
			return 0;
		}
	}
	case IEEE80211_IOC_WPAKEY:
		return wl_ieee80211_wpakey_set(wlif, ifp, ireq);
	case IEEE80211_IOC_DELKEY:
		return wl_ieee80211_delkey(wlif, ifp, ireq);
	case IEEE80211_IOC_MLME:
		return wl_ieee80211_mlme_op(wlif, ifp, ireq);
	case IEEE80211_IOC_COUNTERMEASURES: /* Do nothing, driver does the right thing */
		return 0;
	case IEEE80211_IOC_WPA: { /* Set WPA Version (0, 1, 2) */
		/* start fresh */
		wpa_auth = WPA_AUTH_DISABLED;
		wsec = 0;

		if (ireq->i_val & 1) {
			wsec |= TKIP_ENABLED;
		}

		if (ireq->i_val & 2) {
			wsec |= AES_ENABLED;
		}

		error = wl_iovar_setint(wl, "wsec", wsec, wlif, wlif->wlcif);
		break;
	}
	case IEEE80211_IOC_WME:
		int_val = ireq->i_val;

		{
			int cur_wme;

			wl_iovar_getint(wl, "wme", &cur_wme, wlif, wlif->wlcif);

			if (cur_wme == int_val)
				return 0;
		}

		if (wl->pub->up) {
			wl_down(wl);
			wasup = TRUE;
		}

		error = wl_iovar_setint(wl, "wme", int_val, wlif, wlif->wlcif);

		if (wasup)
			wl_up(wl);

		break;
	case IEEE80211_IOC_HIDESSID:
		return wl_iovar_setint(wl, "closednet", ireq->i_val, wlif, wlif->wlcif);
	case IEEE80211_IOC_APBRIDGE:
		return wl_iovar_setint(wl, "ap_isolate", !ireq->i_val, wlif, wlif->wlcif);
	case IEEE80211_IOC_SCAN_REQ: {
		struct ieee80211_scan_req sr;
		/* in STA mode always do broadcast and
		 * directed scan so we can fill out the AU scan results
		 */
		wlc_ssid_t *ssidptr;
		int params_size = WL_SCAN_PARAMS_FIXED_SIZE +
			WL_NUMCHANNELS /* WL_NUMCHANNELS */ * sizeof(uint16) +
			WL_SCAN_PARAMS_SSID_MAX * sizeof(wlc_ssid_t);
		wl_scan_params_t *params;

		if (ireq->i_len != sizeof(sr))
			return EINVAL;

		error = copyin(ireq->i_data, &sr, sizeof(sr));
		if (error != 0)
			return error;

		if ((params = (wl_scan_params_t*)MALLOC(wl->osh, params_size)) == NULL)
			return ENOMEM;

		memset(params, 0, params_size);

		memcpy(&params->bssid, &ether_bcast, ETHER_ADDR_LEN);
		params->bss_type = DOT11_BSSTYPE_ANY;
		params->scan_type = (sr.sr_flags & IEEE80211_IOC_SCAN_ACTIVE) ? 0 :
			WL_SCANFLAGS_PASSIVE;
		params->nprobes = -1;
		params->active_time = -1;
		params->passive_time = -1;
		params->home_time = -1;
		params->channel_num = 0;

		/* support only one ssid scan for now */
		if (sr.sr_ssid[0].len > IEEE80211_NWID_LEN) {
			MFREE(wl->osh, params, params_size);
			return EINVAL;
		}
		if (sr.sr_ssid[0].len) {
			/* in STA mode always do broadcast and
			 * directed scan so we can fill out the AU scan results
			 */
			ssidptr = (wlc_ssid_t *)((char *)params +
				ROUNDUP(OFFSETOF(wl_scan_params_t, channel_list), sizeof(uint32)));
			memset(ssidptr, 0, sizeof(wlc_ssid_t));
			ssidptr++;
			ssidptr->SSID_len = sr.sr_ssid[0].len;
			strcpy((char*)ssidptr->SSID, sr.sr_ssid[0].ssid);
			params->channel_num = 2 << WL_SCAN_PARAMS_NSSID_SHIFT;
		} else {
			/* Broadcast scan */
			params->ssid.SSID_len = 0;
		}

		wl->scandone = FALSE;
		if (sr.sr_flags & IEEE80211_IOC_SCAN_CQ) {
			/* Force active, as SCAN_CQ uses scan engine
			 * in active mode (it does not mean we send probe req).
			 */
			params->scan_type = 0;
			params->nprobes = 1;
			params->active_time = 50;
			params->home_time = 0;
			if ((error = wl_iovar_op(wl, "cqm", NULL, 0, params,
				params_size, IOV_SET, wlif, NULL)))
				WL_ERROR(("%s: CQM set failed error (%d)\n",
				__FUNCTION__, error));
		} else
			error = wl_wlc_ioctl(ifp, WLC_SCAN, params, params_size);

		MFREE(wl->osh, params, params_size);

		if (error)
			return error;
		break;
		}
	case IEEE80211_IOC_SCAN_CANCEL:
		/* what should we do for this? */
		break;
	case IEEE80211_IOC_ADDMAC:
	case IEEE80211_IOC_DELMAC:
		return wl_ieee80211_macmac(wlif, ifp, ireq);
	case IEEE80211_IOC_MACCMD:
		return wl_ieee80211_macmode_set(wlif, ifp, ireq);
	case IEEE80211_IOC_WME_CWMIN:		/* WME: CWmin */
	case IEEE80211_IOC_WME_CWMAX:		/* WME: CWmax */
	case IEEE80211_IOC_WME_AIFS:		/* WME: AIFS */
	case IEEE80211_IOC_WME_TXOPLIMIT:	/* WME: txops limit */
	case IEEE80211_IOC_WME_ACM:		/* WME: ACM (bss only) */
	case IEEE80211_IOC_WME_ACKPOLICY:	/* WME: ACK policy (bss only) */
		return wl_ieee80211_wmeparam_set(wlif, ifp, ireq);
	case IEEE80211_IOC_DTIM_PERIOD:
		int_val = ireq->i_val;
		return wl_wlc_ioctl(ifp, WLC_SET_DTIMPRD, &int_val, sizeof(int_val));
	case IEEE80211_IOC_BEACON_INTERVAL:
		int_val = ireq->i_val;
		return wl_wlc_ioctl(ifp, WLC_SET_BCNPRD, &int_val, sizeof(int_val));
	case IEEE80211_IOC_PUREG:
		if (ireq->i_val)
			int_val = GMODE_ONLY;
		else
			int_val = GMODE_AUTO;
		return wl_wlc_ioctl(ifp, WLC_SET_GMODE, &int_val, sizeof(int_val));
		break;
	case IEEE80211_IOC_FRAGTHRESHOLD:
		return wl_iovar_setint(wl, "fragthresh", ireq->i_val, wlif, wlif->wlcif);
	case IEEE80211_IOC_HTCONF: {
		int nmode;

		int_val = (ireq->i_val ? WL_11N_3x3 : 0);

		if ((error = wl_iovar_getint(wl, "nmode", &nmode, wlif, wlif->wlcif)))
			return error;

		if (nmode == int_val)
			break;

		if (wl->pub->up) {
			wl_down(wl);
			wasup = TRUE;
		}

		error = wl_iovar_setint(wl, "nmode", int_val, wlif, wlif->wlcif);

		if (wasup)
			wl_up(wl);

		break;
		}
	case IEEE80211_IOC_SMPS:
		error = 0;
		break;
	case IEEE80211_IOC_STBC:
		error = 0;
		break;
	case IEEE80211_IOC_MAX_AID:
		error = wl_iovar_setint(wl, "bss_maxassoc", ireq->i_val, wlif, wlif->wlcif);
		break;
	case IEEE80211_IOC_APPIEBUF:
		error = wl_ieee80211_appiebuf_set(wlif, ireq);
		break;
	case IEEE80211_IOC_APPIE:
		error = wl_ieee80211_appie_set(wlif, ireq);
		break;
	case IEEE80211_IOC_DWDS: {
		error = 0;
		if (ireq->i_val)
			wlif->isDWDS = true;
		else
			wlif->isDWDS = false;
		break;
		}
	case IEEE80211_IOC_TSN:
		error = 0;
		break;
	case IEEE80211_IOC_WPS: {
		if ((error = wl_iovar_getint(wl, "wsec", &wsec, wlif, wlif->wlcif)))
			break;
		if (ireq->i_val) {
			wsec = wsec | SES_OW_ENABLED;
		} else {
			wsec = wsec & ~SES_OW_ENABLED;
		}
		error = wl_iovar_setint(wl, "wsec", wsec, wlif, wlif->wlcif);
		break;
	}
	case IEEE80211_IOC_INACTIVITY:
		if (WLIF_AP_MODE(wl, wlif)) {
			int_val = ireq->i_val ? SCB_TIMEOUT : 0;
			error = wl_wlc_ioctl(ifp, WLC_SET_SCB_TIMEOUT, &int_val, sizeof(int_val));
		} else {
			error = 0;
		}
		break;
	case IEEE80211_IOC_TXPARAMS:
		error = wl_ieee80211_txparams_set(wlif, ifp, ireq);
		break;
	case IEEE80211_IOC_REGDOMAIN:
		error = wl_ieee80211_regdomain_set(wlif, ireq);
		break;
	case IEEE80211_IOC_CHANSWITCH:
		return wl_ieee80211_chanswitch(wlif, ireq);
	case IEEE80211_IOC_VHTCONF: {
		int vhtmode;
		if ((error = wl_iovar_getint(wl, "vhtmode", &vhtmode, wlif, wlif->wlcif)))
			return error;

		if (vhtmode == ireq->i_val)
			break;

		if (wl->pub->up) {
			wl_down(wl);
			wasup = TRUE;
		}

		error = wl_iovar_setint(wl, "vhtmode", ireq->i_val, wlif, wlif->wlcif);

		if (wasup)
			wl_up(wl);


		break;
	}
	case IEEE80211_IOC_BGSCAN:
		error = 0;
		break;

#ifdef WL11K
	case IEEE80211_IOC_RRM: {
		char *buf = NULL;
		int buflen;
		int i;
		dot11_rrm_cap_ie_t rrm_cap;
		int enabled = 0;
		error = wl_ieee80211_rrm_enabled(wlif, ifp, &enabled);
		if (ireq->i_val != enabled) {

			for (i = 0; i < DOT11_RRM_CAP_LEN; i++)
				rrm_cap.cap[i] = 0;
			if (ireq->i_val != 0) {
				rrm_cap.cap[DOT11_RRM_CAP_NEIGHBOR_REPORT / NBBY] |=
					(1 << (DOT11_RRM_CAP_NEIGHBOR_REPORT % NBBY));
				rrm_cap.cap[DOT11_RRM_CAP_AP_CHANREP / NBBY] |=
					(1 << (DOT11_RRM_CAP_AP_CHANREP % NBBY));
			}

			buflen = strlen("rrm") + 1 + DOT11_RRM_CAP_LEN;
			if (!(buf = (void *) MALLOC(wl->osh, buflen))) {
				error = BCME_NORESOURCE;
				return error;
			}
			memset(buf, 0, buflen);
			strcpy(buf, "rrm");
			memcpy(&buf[strlen("rrm") + 1], &rrm_cap, DOT11_RRM_CAP_LEN);
			error = wl_wlc_ioctl(ifp, WLC_SET_VAR, buf, buflen);
			MFREE(wl->osh, buf, buflen);
		} else {
			error = 0;
		}
		break;
	}

	case IEEE80211_IOC_RRM_ADD_NEIGHBOR: {
		char *buf = NULL;
		int buflen;

		if (ireq->i_len == TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN) {
			buflen = strlen("rrm_nbr_add_nbr") + 1 + ireq->i_len;
			if (!(buf = (void *) MALLOC(wl->osh, buflen))) {
				error = BCME_NORESOURCE;
				return error;
			}
			memset(buf, 0, buflen);
			strcpy(buf, "rrm_nbr_add_nbr");
			error = copyin(ireq->i_data, &buf[buflen - ireq->i_len], ireq->i_len);
			if (error != 0) {
				MFREE(wl->osh, buf, buflen);
				return error;
			}
			error = wl_wlc_ioctl(ifp, WLC_SET_VAR, buf, buflen);
			MFREE(wl->osh, buf, buflen);

		} else
			error = EINVAL;
		break;
	}

	case IEEE80211_IOC_RRM_DEL_NEIGHBOR: {
		char *buf = NULL;
		int buflen;

		if (ireq->i_len != ETHER_ADDR_LEN)
			return EINVAL;

		buflen = strlen("rrm_nbr_del_nbr") + 1 + ireq->i_len;
		if (!(buf = (void *) MALLOC(wl->osh, buflen))) {
			error = BCME_NORESOURCE;
			return error;
		}
		memset(buf, 0, buflen);
		strcpy(buf, "rrm_nbr_del_nbr");
		error = copyin(ireq->i_data, &buf[buflen - ireq->i_len], ireq->i_len);
		if (error != 0) {
			MFREE(wl->osh, buf, buflen);
			return error;
		}
		error = wl_wlc_ioctl(ifp, WLC_SET_VAR, buf, buflen);
		MFREE(wl->osh, buf, buflen);
		break;
	}
#endif /* WL11K */

	default:
		return EINVAL;
	}

	return error;
}

static void
wl_ieee80211_event(struct ifnet *ifp, wlc_event_t *e, int ap_mode)
{
	wl_if_t *wlif;
	int ipl;

	WL_INFORM(("%s: event:%d ifname:%s \n",
	           __FUNCTION__, e->event.event_type, e->event.ifname));

	wlif = WLIF_IFP(ifp);
	ipl = splnet();

	switch (e->event.event_type) {
	case WLC_E_JOIN:
	case WLC_E_REASSOC_IND:
	case WLC_E_ASSOC_IND: {
		struct ieee80211_join_event join_evt;
		int rtm = 0;
		bzero(&join_evt, sizeof(join_evt));
		bcopy(e->addr, join_evt.iev_addr, ETHER_ADDR_LEN);
		/* RTMs are different based on context */
		switch (e->event.event_type) {
		case WLC_E_JOIN:
			ASSERT(!ap_mode);
			rtm = RTM_IEEE80211_ASSOC;
			break;
		case WLC_E_REASSOC_IND:
			rtm = (ap_mode)? RTM_IEEE80211_REJOIN: RTM_IEEE80211_REASSOC;
			break;
		case WLC_E_ASSOC_IND:
			rtm = RTM_IEEE80211_JOIN;
		}
		rt_ieee80211msg(ifp, rtm, &join_evt, sizeof(join_evt));
		if (ap_mode)
			wlc_add_sta_ie(wlif, e->addr, e->data, e->event.datalen);
		if (!ap_mode && wlif->isDWDS) {
			wlc_dwds_config_t dwds;

			bzero(&dwds, sizeof(dwds));
			dwds.mode = 1; /* STA interface */
			dwds.enable = 1;
			wl_iovar_op(wlif->wl, "dwds_config", NULL, 0, &dwds, sizeof(dwds),
			                    IOV_SET, wlif, wlif->wlcif);
		}
		break;
	}
	case WLC_E_DEAUTH:
	case WLC_E_DEAUTH_IND:
	case WLC_E_DISASSOC_IND:{
		struct ieee80211_leave_event leave_evt;

		bzero(&leave_evt, sizeof(leave_evt));
		if (e->addr) {
			bcopy(e->addr, leave_evt.iev_addr, ETHER_ADDR_LEN);
		}
		rt_ieee80211msg(ifp, ap_mode? RTM_IEEE80211_LEAVE:RTM_IEEE80211_DISASSOC,
		                &leave_evt, sizeof(leave_evt));
		if (ap_mode && e->addr)
			wlc_free_sta_ie(wlif, e->addr, FALSE);
		break;
	}
	case WLC_E_SCAN_COMPLETE: {
		rt_ieee80211msg(ifp, RTM_IEEE80211_SCAN, NULL, 0);
		break;
	}
	case WLC_E_CSA_COMPLETE_IND:{
		struct ieee80211_csa_event iev;
		struct ieee80211_channel c;
		uint8 mode = 0;
		int err;

		bzero(&iev, sizeof(iev));
		bzero(&c, sizeof(c));

		if ((err = wl_ieee80211_curchan_get(wlif, &c)))
			WL_ERROR(("%s: wl_ieee80211_curchan_get falied err (%d)\n",
			__FUNCTION__, err));

		iev.iev_flags = c.ic_flags;
		iev.iev_freq = c.ic_freq;
		iev.iev_ieee = c.ic_ieee;
		if (e->data)
			mode = (uint8) *((uint8 *)e->data);
		iev.iev_mode = mode;
		iev.iev_count = 0;
		WL_INFORM(("%s: CSA Notification chan (%d) freq (%d) flags(%x) mode (%d)\n",
		          __FUNCTION__, iev.iev_ieee, iev.iev_freq, iev.iev_flags, iev.iev_mode));
		rt_ieee80211msg(ifp, RTM_IEEE80211_CSA, &iev, sizeof(iev));

		break;
	}
	case WLC_E_MIC_ERROR: {
		struct ieee80211_michael_event mic_evt;
		bzero(&mic_evt, sizeof(mic_evt));
		bcopy(e->addr, mic_evt.iev_src, ETHER_ADDR_LEN);
		mic_evt.iev_cipher = IEEE80211_CIPHER_TKIP;
		rt_ieee80211msg(ifp, RTM_IEEE80211_MICHAEL, &mic_evt, sizeof(mic_evt));
		break;
	}
	case WLC_E_LINK: {
		if (!ap_mode && !(e->event.flags & WLC_EVENT_MSG_LINK)) {
			struct ieee80211_leave_event leave_evt;
			bzero(&leave_evt, sizeof(leave_evt));
			rt_ieee80211msg(ifp, RTM_IEEE80211_DISASSOC,
			                &leave_evt, sizeof(leave_evt));
		}
		break;
	}
	default:
		break;
	}
	splx(ipl);
}

static uint16
wlc_get_sta_ie_len(wl_if_t *wlif, struct ether_addr *ea)
{
	sta_ie_list_t *elt = NULL;
	wl_info_t *wl = wlif->wl;

	SLIST_FOREACH(elt, &wl->assoc_sta_ie_list, entries) {
		if (bcmp(ea, &elt->addr, ETHER_ADDR_LEN) == 0) {
			break;
		}
	}

	if (elt == NULL) {
		/* no entry found in sta ie list, return 0 bytes */
		return 0;
	}

	return elt->tlvlen;
}

static int
wlc_get_sta_ie(wl_if_t *wlif, struct ether_addr *ea, uint8 *ie, uint16 *ie_len)
{
	sta_ie_list_t *elt = NULL;
	wl_info_t *wl = wlif->wl;

	SLIST_FOREACH(elt, &wl->assoc_sta_ie_list, entries) {
		if (bcmp(ea, &elt->addr, ETHER_ADDR_LEN) == 0) {
			break;
		}
	}

	if (elt == NULL) {
		/* no entry found in sta ie list, return 0 bytes */
		*ie_len = 0;
		return 0;
	}

	bcopy((uint8 *)elt->tlv, (uint8 *)ie, elt->tlvlen);
	*ie_len = elt->tlvlen;

	return 0;
}

void
wlc_add_sta_ie(wl_if_t *wlif, struct ether_addr *addr, void *data, uint32 datalen)
{
	sta_ie_list_t *elt;
	char eabuf[ETHER_ADDR_STR_LEN];
	wl_info_t *wl = wlif->wl;

	if (datalen == 0) {
		return;
	}

	if ((addr == NULL) || (data == NULL)) {
		WL_ERROR(("%s: addr(%p) or data(%p) is null\n",
		          __FUNCTION__, addr, data));
		return;
	}

	bcm_ether_ntoa(addr, eabuf);

	/* remove duplicate entries */
	wlc_free_sta_ie(wlif, addr, FALSE);

	/* allocate and add new entry to list */
	if ((elt = (sta_ie_list_t *)MALLOC(wl->osh, sizeof(sta_ie_list_t) + datalen)) == NULL) {
		WL_ERROR(("%s: MALLOC failed for %s\n", __FUNCTION__, eabuf));
		return;
	}

	/* initialize newly allocated entry */
	elt->tlv = (bcm_tlv_t *)((uint8 *)elt + sizeof(sta_ie_list_t));
	elt->tlvlen = datalen;
	bcopy(addr, &elt->addr, ETHER_ADDR_LEN);
	bcopy(data, (void *)elt->tlv, datalen);

	/* insert entry into list */
	SLIST_INSERT_HEAD(&wl->assoc_sta_ie_list, elt, entries);
}

void
wlc_free_sta_ie(wl_if_t *wlif, struct ether_addr *addr, bool deleteall)
{
	sta_ie_list_t *elt;
	bool found;
	char eabuf[ETHER_ADDR_STR_LEN];
	wl_info_t *wl = wlif->wl;

	if ((addr == NULL) && !deleteall) {
		WL_ERROR(("%s: addr is null\n", __FUNCTION__));
		return;
	}

	/* remove duplicate entries */
	do {
		found = FALSE;
		SLIST_FOREACH(elt, &wl->assoc_sta_ie_list, entries) {
			if (deleteall || (bcmp(addr, &elt->addr, ETHER_ADDR_LEN) == 0)) {
				found = TRUE;
				break;
			}
		}
		if (found) {
			SLIST_REMOVE(&wl->assoc_sta_ie_list, elt, sta_ie_list, entries);
			bcm_ether_ntoa(&elt->addr, eabuf);
			WL_ERROR(("%s: entry %p found for %s, removed\n",
			         __FUNCTION__, elt, eabuf));
			MFREE(wl->osh, elt, sizeof(sta_ie_list_t) + elt->tlvlen);
		}
	} while (found);
}

/*
 * Vendor IE support.
 */

static int
count_ies(struct ieee80211_appie *ie)
{
	int			count = 0;
	uint8_t		*data;

	if (ie->ie_len > 2) {
		data = ie->ie_data;

		while (data - ie->ie_data < ie->ie_len) {
			count++;
			data = data + data[1] + 2;
		}
	}
	return count;
}

static int
validate_ies(struct ieee80211_appie *ie)
{
	uint8_t		*data;
	size_t		totLen = 0;
	if (ie->ie_len < 2)
		return 0;

	data = ie->ie_data;
	while (data - ie->ie_data < ie->ie_len) {
		totLen = totLen + data[1] + 2;
		data = data + data[1] + 2;
	}

	return (totLen == ie->ie_len);
}

static size_t
convert_appie(wl_info_t *wl, struct ieee80211_appie *app, vndr_ie_setbuf_t **vndr_ie_buf,
	uint8_t ft)
{
	vndr_ie_setbuf_t			*vbuf;
	size_t						vndr_ie_buf_len;
	int							totie = 0;
	uint8_t						*data;
	vndr_ie_info_t				*vinfo;
	uint32_t					pktflag  = 0;

	switch (ft) {
		case IEEE80211_FC0_SUBTYPE_BEACON:
			pktflag = VNDR_IE_BEACON_FLAG;
			break;
		case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
			pktflag = VNDR_IE_PRBRSP_FLAG;
			break;
		case IEEE80211_FC0_SUBTYPE_ASSOC_RESP:
			pktflag = VNDR_IE_ASSOCRSP_FLAG;
			break;
		case IEEE80211_FC0_SUBTYPE_PROBE_REQ:
			pktflag = VNDR_IE_PRBREQ_FLAG;
			break;
		case IEEE80211_FC0_SUBTYPE_ASSOC_REQ:
			pktflag = VNDR_IE_ASSOCREQ_FLAG;
			break;
	}

	totie = count_ies(app);

	vndr_ie_buf_len = sizeof(vndr_ie_setbuf_t) - sizeof(vndr_ie_info_t) +
		totie * (sizeof(vndr_ie_info_t) - sizeof(vndr_ie_t)) + app->ie_len;

	vbuf = (vndr_ie_setbuf_t *)MALLOC(wl->osh, vndr_ie_buf_len);
	if (!vbuf) {
		*vndr_ie_buf = NULL;
		return 0;
	}
	vbuf->vndr_ie_buffer.iecount = totie;
	data = app->ie_data;
	vinfo = &vbuf->vndr_ie_buffer.vndr_ie_list[0];

	while ((data - app->ie_data) < app->ie_len) {
		memcpy(&vinfo->vndr_ie_data, data, data[1] + 2);
		vinfo->pktflag = pktflag;
		vinfo = (vndr_ie_info_t*)((uint8_t*)vinfo + data[1] + 2 +
			sizeof(vndr_ie_info_t) - sizeof(vndr_ie_t));
		data = data + data[1] + 2;
	}
	*vndr_ie_buf = vbuf;
	return vndr_ie_buf_len;
}

static int
setappie(wl_if_t *wlif, vndr_ie_setbuf_t **aie, size_t *len, const struct ieee80211req *ireq,
	uint8_t ft)
{

	vndr_ie_setbuf_t			*app = *aie;
	vndr_ie_setbuf_t			*napp;
	struct ieee80211_appie		*tmp;
	int							error = 0;
	wl_info_t					*wl = wlif->wl;
	size_t						napp_len;

	if (app != NULL) {
		*aie = NULL;

		snprintf(app->cmd, sizeof(app->cmd), "del");
		wl_iovar_op(wl, "vndr_ie", NULL, 0, app, *len, IOV_SET, wlif, wlif->wlcif);
		MFREE(wl->osh, app, *len);
	}

	if (ireq && ireq->i_len) {
		tmp = (struct ieee80211_appie *)MALLOC(wl->osh,
			sizeof(struct ieee80211_appie) + ireq->i_len);
		if (tmp == NULL)
			return ENOMEM;

		error = copyin(ireq->i_data, tmp->ie_data, ireq->i_len);
		if (error) {
			MFREE(wl->osh, tmp, sizeof(struct ieee80211_appie) + ireq->i_len);
			return error;
		}

		tmp->ie_len = ireq->i_len;

		if (!validate_ies(tmp)) {
			MFREE(wl->osh, tmp, sizeof(struct ieee80211_appie) + ireq->i_len);
			return EINVAL;
		}

		napp_len = convert_appie(wl, tmp, &napp, ft);
		if (napp_len) {
			snprintf(napp->cmd, sizeof(napp->cmd), "add");
			error = wl_iovar_op(wl, "vndr_ie", NULL, 0,
				napp, napp_len, IOV_SET, wlif, wlif->wlcif);
			*aie = napp;
			*len = napp_len;
		}

		MFREE(wl->osh, tmp, sizeof(struct ieee80211_appie) + ireq->i_len);
	}


	return error;
}

static int
setwpaauth(wl_if_t *wlif, const struct ieee80211req *ireq)
{
	struct ieee80211_appie		*tmp;
	int				error = EINVAL;
	wl_info_t			*wl = wlif->wl;
	int				wpa_auth = WPA_AUTH_DISABLED;
	int				tmp_auth;
	bcm_tlv_t			*wpa2ie = NULL;
	wpa_ie_fixed_t			*wpaie = NULL;
	uint8_t				len;
	uint16_t			count, i;
	wpa_suite_auth_key_mgmt_t	*mgmt;
	wpa_suite_mcast_t		*mcast;
	wpa_suite_ucast_t		*ucast;

	if (ireq && ireq->i_len) {
		tmp = (struct ieee80211_appie *)MALLOC(wl->osh,
			sizeof(struct ieee80211_appie) + ireq->i_len);
		if (tmp == NULL)
			return ENOMEM;

		error = copyin(ireq->i_data, tmp->ie_data, ireq->i_len);
		if (error) {
			MFREE(wl->osh, tmp, sizeof(struct ieee80211_appie) + ireq->i_len);
			return error;
		}

		tmp->ie_len = ireq->i_len;

		if (!validate_ies(tmp)) {
			MFREE(wl->osh, tmp, sizeof(struct ieee80211_appie) + ireq->i_len);
			return EINVAL;
		}

		/* map IEs to WPA_AUTH_UNSPECIFIED, WPA_AUTH_PSK, WPA2_AUTH_UNSPECIFIED,
		 * WPA2_AUTH_PSK,
		 * and maybe one day WPA2_AUTH_MFP
		 */

		/* this code is fragile b/c if user mode sends in
		 * bad counts bad things will happen
		 */
		wpaie = bcm_find_wpaie(tmp->ie_data, tmp->ie_len);
		wpa2ie = bcm_parse_tlvs(tmp->ie_data, tmp->ie_len, DOT11_MNG_RSN_ID);
		if (wpaie) {
			len = wpaie->length;
			mcast = (wpa_suite_mcast_t *)&wpaie[1];
			ucast = (wpa_suite_ucast_t *)&mcast[1];
			count = ltoh16_ua(&ucast->count);
			mgmt = (wpa_suite_auth_key_mgmt_t *)&ucast->list[count];
			count = ltoh16_ua(&mgmt->count);
			for (i = 0; i < count; i++) {
				if (bcmwpa_akm2WPAauth((uint8 *)&mgmt->list[0], &tmp_auth, TRUE))
					wpa_auth = wpa_auth | tmp_auth;
			}
		}
		if (wpa2ie) {
			mcast = (wpa_suite_mcast_t *)&wpa2ie->data[WPA2_VERSION_LEN];
			ucast = (wpa_suite_ucast_t *)&mcast[1];
			count = ltoh16_ua(&ucast->count);
			mgmt = (wpa_suite_auth_key_mgmt_t *)&ucast->list[count];
			count = ltoh16_ua(&mgmt->count);
			for (i = 0; i < count; i++) {
				if (bcmwpa_akm2WPAauth((uint8 *)&mgmt->list[0], &tmp_auth, FALSE))
					wpa_auth = wpa_auth | tmp_auth;
			}
		}

		error = wl_iovar_setint(wl, "wpa_auth", wpa_auth, wlif, wlif->wlcif);
		MFREE(wl->osh, tmp, sizeof(struct ieee80211_appie) + ireq->i_len);
	} else
		error = wl_iovar_setint(wl, "wpa_auth", wpa_auth, wlif, wlif->wlcif);
	return error;
}

/* add this so we can override the mac addr */
#if __NetBSD_Version__ >= 500000003
static int
wl_initaddr(struct ifnet *ifp, struct ifaddr *ifaddr, bool src)
{
	struct sockaddr_dl *sdl;

	struct ether_addr ea;
	uint8_t zero[ETHER_ADDR_LEN] = {0};
	wl_if_t *wlif;
	wl_info_t *wl;
	int error;

	wlif = WLIF_IFP(ifp);
	wl = wlif->wl;

	if (wl->pub->up)
		return EINVAL;

	sdl = satosdl(ifaddr->ifa_addr);

	if (sdl->sdl_family != AF_LINK)
		return 0;

	if ((sdl->sdl_type != IFT_ETHER) ||
		(sdl->sdl_alen != ifp->if_addrlen) ||
		ETHER_IS_MULTICAST(CLLADDR(sdl)) ||
		(memcmp(zero, CLLADDR(sdl), sizeof(zero)) == 0)) {
		return EINVAL;
	}

	bcopy(CLLADDR(sdl), &ea, ETHER_ADDR_LEN);

	error = wl_iovar_op(wl, "cur_etheraddr", NULL, 0,
		&ea, ETHER_ADDR_LEN, IOV_SET, wl->base_if, wl->base_if->wlcif);

	if (!error)
		if_set_sadl(ifp, CLLADDR(sdl), sdl->sdl_alen, false);
	return error;
}
#endif /* __NetBSD_Version__ > 106200000 */
