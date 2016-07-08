/*
 * wl_linux.c exported functions and definitions
 *
 * Copyright (C) 2016, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: wl_linux.h 626450 2016-03-21 22:39:20Z $
 */

#ifndef _wl_linux_h_
#define _wl_linux_h_

#include <wlc_types.h>

/* WL_ALL_PASSIVE should be defined for high-only driver */
#if !defined(WLC_LOW) && !defined(WL_ALL_PASSIVE)
#define WL_ALL_PASSIVE 1
#endif

#ifdef BCM7271
/* Opaque handle */
typedef struct BCM7XXX_Data *BCM7XXX_Handle;
#undef WL_ALL_PASSIVE
/* for EMU force everything to all passive */
#define WL_ALL_PASSIVE 1
#endif /* BCM7271 */

/* BMAC Note: High-only driver is no longer working in softirq context as it needs to block and
 * sleep so perimeter lock has to be a semaphore instead of spinlock. This requires timers to be
 * submitted to workqueue instead of being on kernel timer
 */
typedef struct wl_timer {
	struct timer_list 	timer;
	struct wl_info 		*wl;
	void 				(*fn)(void *);
	void				*arg; /* argument to fn */
	uint 				ms;
	bool 				periodic;
	bool 				set;
	struct wl_timer 	*next;
#if defined(BCM7271)
	char* 				name; /* Description of the timer */
	uint32				ticks;	/* how many timer timer fired */
#endif
} wl_timer_t;

/* contortion to call functions at safe time */
/* In 2.6.20 kernels work functions get passed a pointer to the struct work, so things
 * will continue to work as long as the work structure is the first component of the task structure.
 */
typedef struct wl_task {
	struct work_struct work;
	void *context;
} wl_task_t;

#define WL_IFTYPE_BSS	1 /* iftype subunit for BSS */
#define WL_IFTYPE_WDS	2 /* iftype subunit for WDS */
#define WL_IFTYPE_MON	3 /* iftype subunit for MONITOR */

struct wl_if {
#ifdef USE_IW
	wl_iw_t		iw;		/* wireless extensions state (must be first) */
#endif /* USE_IW */
	struct wl_if *next;
	struct wl_info *wl;		/* back pointer to main wl_info_t */
	struct net_device *dev;		/* virtual netdevice */
	struct wlc_if *wlcif;		/* wlc interface handle */
	uint subunit;			/* WDS/BSS unit */
	bool dev_registered;	/* netdev registed done */
	int  if_type;			/* WL_IFTYPE */
	char name[IFNAMSIZ];		/* netdev may not be alloced yet, so store the name
					   here temporarily until the netdev comes online
					 */
#ifdef ARPOE
	wl_arp_info_t	*arpi;		/* pointer to arp agent offload info */
#endif /* ARPOE */
	struct net_device_stats stats;  /* stat counter reporting structure */
	uint    stats_id;               /* the current set of stats */
	struct net_device_stats stats_watchdog[2]; /* ping-pong stats counters updated */
	                                           /* by Linux watchdog */
#ifdef USE_IW
	struct iw_statistics wstats_watchdog[2];
	struct iw_statistics wstats;
	int             phy_noise;
#endif /* USE_IW */
};

struct rfkill_stuff {
	struct rfkill *rfkill;
	char rfkill_name[32];
	char registered;
};

struct wl_info {
	uint		unit;		/* device instance number */
	wlc_pub_t	*pub;		/* pointer to public wlc state */
	void		*wlc;		/* pointer to private common os-independent data */
	osl_t		*osh;		/* pointer to os handler */
#ifdef HNDCTF
	ctf_t		*cih;		/* ctf instance handle */
#endif /* HNDCTF */
	struct net_device *dev;		/* backpoint to device */

	struct semaphore sem;		/* use semaphore to allow sleep */
	spinlock_t	lock;		/* per-device perimeter lock */
	spinlock_t	isr_lock;	/* per-device ISR synchronization lock */

	uint		bcm_bustype;	/* bus type */
	bool		piomode;	/* set from insmod argument */
	void *regsva;			/* opaque chip registers virtual address */
	wl_if_t *if_list;		/* list of all interfaces */
	atomic_t callbacks;		/* # outstanding callback functions */
	struct wl_timer *timers;	/* timer cleanup queue */
#ifndef NAPI_POLL
	struct tasklet_struct tasklet;	/* dpc tasklet */
	struct tasklet_struct tx_tasklet; /* tx tasklet */
#endif /* NAPI_POLL */

#if defined(NAPI_POLL) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30))
	struct napi_struct napi;
#endif /* defined(NAPI_POLL) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)) */

	struct net_device *monitor_dev;	/* monitor pseudo device */
	uint		monitor_type;	/* monitor pseudo device */
	bool		resched;	/* dpc needs to be and is rescheduled */
#ifdef TOE
	wl_toe_info_t	*toei;		/* pointer to toe specific information */
#endif
#ifdef ARPOE
	wl_arp_info_t	*arpi;		/* pointer to arp agent offload info */
#endif
#ifdef LINUXSTA_PS
	uint32		pci_psstate[16];	/* pci ps-state save/restore */
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#define NUM_GROUP_KEYS 4
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	struct lib80211_crypto_ops *tkipmodops;
#else
	struct ieee80211_crypto_ops *tkipmodops;	/* external tkip module ops */
#endif
	struct ieee80211_tkip_data  *tkip_ucast_data;
	struct ieee80211_tkip_data  *tkip_bcast_data[NUM_GROUP_KEYS];
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14) */
	/* RPC, handle, lock, txq, workitem */
#ifdef WLC_HIGH_ONLY
	rpc_info_t 	*rpc;		/* RPC handle */
	rpc_tp_info_t	*rpc_th;	/* RPC transport handle */
	wlc_rpc_ctx_t	rpc_dispatch_ctx;

	bool	   rpcq_dispatched;	/* Avoid scheduling multiple tasks */
	spinlock_t rpcq_lock;		/* Lock for the queue */
	rpc_buf_t *rpcq_head;		/* RPC Q */
	rpc_buf_t *rpcq_tail;		/* Points to the last buf */
	int        rpcq_len;		/* length of RPC queue */
#endif /* WLC_HIGH_ONLY */

	bool		txq_dispatched;	/* Avoid scheduling multiple tasks */
	spinlock_t	txq_lock;	/* Lock for the queue */
	struct sk_buff	*txq_head;	/* TX Q */
	struct sk_buff	*txq_tail;	/* Points to the last buf */
	int		txq_cnt;	/* the txq length */

#ifdef WL_ALL_PASSIVE
	wl_task_t	txq_task;	/* work queue for wl_start() */
	wl_task_t	multicast_task;	/* work queue for wl_set_multicast_list() */

#ifdef WLC_LOW
	wl_task_t	wl_dpc_task;	/* work queue for wl_dpc() */
	bool		all_dispatch_mode;
#endif /* WLC_LOW */
#endif /* WL_ALL_PASSIVE */

#ifdef WL_THREAD
    struct task_struct      *thread;
    wait_queue_head_t       thread_wqh;
    struct sk_buff_head     tx_queue;
    struct sk_buff_head     rpc_queue;
#endif /* WL_THREAD */
#if defined(WL_CONFIG_RFKILL)
	struct rfkill_stuff wl_rfkill;
	mbool last_phyind;
#endif /* defined(WL_CONFIG_RFKILL) */

	uint processed;		/* Number of rx frames processed */
	struct proc_dir_entry *proc_entry;
#if defined(WLOFFLD) || defined(VASIP_HW_SUPPORT)
	uchar* bar1_addr;
	uint32 bar1_size;
#endif
#if defined(VASIP_HW_SUPPORT)
	uchar* bar2_addr;
	uint32 bar2_size;
#endif
	obj_registry_t	*objr;		/* Common shared object registry for RSDB */
#ifndef LINUX_POSTMOGRIFY_REMOVAL
#ifdef P2PO
	wl_p2po_info_t	*p2po;		/* pointer to p2p offload info */
	wl_disc_info_t	*disc;		/* pointer to disc info */
#endif /* P2PO */
#ifdef ANQPO
	wl_anqpo_info_t *anqpo;	/* pointer to anqp offload info */
#endif /* ANQPO */
#if defined(P2PO) || defined(ANQPO)
	wl_gas_info_t	*gas;		/* pointer to gas info */
#endif /* P2PO || ANQPO */
#ifdef WL_EVENTQ
	wl_eventq_info_t	*wlevtq;	/* pointer to wl_eventq info */
#endif /* WL_EVENTQ */
#endif /* LINUX_POSTMOGRIFY_REMOVAL */

#ifdef BCM7271
	BCM7XXX_Handle	bcm7xxx; /* this is the global handle for BCM7271 WIFI */
#ifdef BCM7271_TAS
	spinlock_t		wl_spin_lock;	/* use for wl_lock_tas */
	uint32			wl_lock_tas; 	/* use for wl_lock */

	spinlock_t		isr_spin_lock;	/* use for isr_lock_tas */
	uint32			isr_lock_tas; 	/* use for isr_lock_tas */
#endif /* BCM7271_TAS */
#endif /* BCM7271 */

};

#ifdef LINUX_HYBRID
#define HYBRID_PROC   "brcm_monitor"
#endif

#if (defined(NAPI_POLL) && defined(WL_ALL_PASSIVE))
#error "WL_ALL_PASSIVE cannot co-exists w/ NAPI_POLL"
#endif /* defined(NAPI_POLL) && defined(WL_ALL_PASSIVE) */

#define WLIF_FWDER(wlif)        (FALSE)

#if defined(WL_ALL_PASSIVE_ON) || defined(WLC_HIGH_ONLY)
#define WL_ALL_PASSIVE_ENAB(wl)	1
#else
#ifdef WL_ALL_PASSIVE
#define WL_ALL_PASSIVE_ENAB(wl)	(!(wl)->all_dispatch_mode)
#else
#define WL_ALL_PASSIVE_ENAB(wl)	0
#endif /* WL_ALL_PASSIVE */
#endif 

/* perimeter lock */
#define WL_LOCK(wl)	do { \
				if (WL_ALL_PASSIVE_ENAB(wl)) \
					down(&(wl)->sem); \
				else \
					spin_lock_bh(&(wl)->lock); \
			} while (0)

#define WL_UNLOCK(wl)	do { \
				if (WL_ALL_PASSIVE_ENAB(wl)) \
					up(&(wl)->sem); \
				else \
					spin_unlock_bh(&(wl)->lock); \
			} while (0)

#ifdef WLC_HIGH_ONLY
/* locking from inside wl_isr */
#define WL_ISRLOCK(wl, flags)
#define WL_ISRUNLOCK(wl, flags)

#elif defined(BCM7271) && defined(BCM7271_TAS)
#undef WL_LOCK
#undef WL_UNLOCK
#define WL_LOCK(wl) do {wl_lock_tas((wl));} while (0)
#define WL_UNLOCK(wl) do {wl_unlock_tas((wl));} while (0)

#define WL_ISRLOCK(wl, flags) do {isr_lock_tas((wl));(void)(flags);} while (0)
#define WL_ISRUNLOCK(wl, flags) do {isr_unlock_tas((wl));(void)(flags);} while (0)

#define INT_LOCK(wl, flags)	do {isr_lock_tas((wl));(void)(flags);} while (0)
#define INT_UNLOCK(wl, flags)	do {isr_unlock_tas((wl));(void)(flags);} while (0)

#else 

/* locking from inside wl_isr */
#define WL_ISRLOCK(wl, flags) do {spin_lock(&(wl)->isr_lock); (void)(flags);} while (0)
#define WL_ISRUNLOCK(wl, flags) do {spin_unlock(&(wl)->isr_lock); (void)(flags);} while (0)

/* locking under WL_LOCK() to synchronize with wl_isr */
#define INT_LOCK(wl, flags)	spin_lock_irqsave(&(wl)->isr_lock, flags)
#define INT_UNLOCK(wl, flags)	spin_unlock_irqrestore(&(wl)->isr_lock, flags)
#endif	/* WLC_HIGH_ONLY */


/* handle forward declaration */
typedef struct wl_info wl_info_t;


#ifdef BCM7271
#ifdef BCM7271_TAS
void wl_lock_tas(wl_info_t *wl);
void wl_unlock_tas(wl_info_t *wl);
void isr_lock_tas(wl_info_t *wl);
void isr_unlock_tas(wl_info_t *wl);

#endif /* BCM7271_TAS */
#endif /* BCM7271 */

#ifndef PCI_D0
#define PCI_D0		0
#endif

#ifndef PCI_D3hot
#define PCI_D3hot	3
#endif

/* exported functions */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
extern irqreturn_t wl_isr(int irq, void *dev_id);
#else
extern irqreturn_t wl_isr(int irq, void *dev_id, struct pt_regs *ptregs);
#endif

extern int __devinit wl_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
extern void wl_free(wl_info_t *wl);
extern int  wl_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
extern struct net_device * wl_netdev_get(wl_info_t *wl);

#ifdef BCM_WL_EMULATOR
extern wl_info_t *  wl_wlcreate(osl_t *osh, void *pdev);
#endif


#endif /* _wl_linux_h_ */
