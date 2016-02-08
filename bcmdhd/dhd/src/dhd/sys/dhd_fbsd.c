/*
 * Broadcom Dongle Host Driver (DHD)
 * Broadcom 802.11bang Networking Device Driver
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: dhd_fbsd.c 335620 2012-05-29 19:01:21Z $
 */

#include <typedefs.h>
#include <osl.h>

#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/module.h>
#include <sys/kthread.h>
#include <sys/sema.h>
#include <net/bpf.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <dev/usb/usb_freebsd.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/tdfx/tdfx_io.h>

#ifdef DHD_NET80211
#include <net80211/ieee80211_ioctl.h>
#define	WFD_MASK	0x0004

#undef WME_OUI
#undef WME_OUI_TYPE
#undef WPA_OUI
#undef WPA_OUI_TYPE
#undef WPS_OUI
#undef WPS_OUI_TYPE
#undef RSN_CAP_PREAUTH
#elif defined(FBSD_MUL_VIF) && defined(FBSD_ENABLE_P2P_DEV_IF)
#define WFD_MASK	0x0004
#endif

#include <epivers.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <dhd_dbg.h>
#include <dhd_proto.h>
#include <dhd_bus.h>
#include <dbus.h>
#include <fbsd_osl.h>
#include <proto/ethernet.h>
#include <proto/bcmevent.h>

#ifdef DHD_NET80211
#include <dhd_net80211.h>
#endif
#include <sys/callout.h>
#ifdef BCM_FD_AGGR
#include <bcm_rpc.h>
#include <bcm_rpc_tp.h>
#endif
#ifdef PROP_TXSTATUS
#include <wlfc_proto.h>
#include <dhd_wlfc.h>
/*
 * FreeBsd uses the system header which does not have this definition
 */
#define ETHER_ISMULTI(ea) (((const uint8 *)(ea))[0] & 1)
/*
 * Is this OS agnostic or needs to be tuned per OS
 */
const uint8 wme_fifo2ac[] = { 0, 1, 2, 3, 1, 1 };
const uint8 prio2fifo[8] = { 1, 0, 0, 1, 2, 2, 3, 3 };
#define WME_PRIO2AC(prio)  wme_fifo2ac[prio2fifo[(prio)]]

extern bool dhd_wlfc_skip_fc(void);
extern void dhd_wlfc_plat_init(void *dhd);
extern void dhd_wlfc_plat_deinit(void *dhd);
#endif /* endif PROP_TXSTATUS */

#ifndef ETHER_TYPE_BRCM
#define ETHER_TYPE_BRCM   0x886c
#endif
#ifndef ETHER_TYPE_802_1X
#define ETHER_TYPE_802_1X 0x888e          /* 802.1x */
#endif

#define MAX_WAIT_FOR_8021X_TX	100
#define DELAY_FOR_8021X_TX	10000   /* micro seconds */

/* API's for external use */
void dhd_module_cleanup(void);
void dhd_module_init(void);
int dhd_change_mtu(dhd_pub_t *dhdp, int new_mtu, int ifidx);
/* int dhd_do_driver_init(struct net_device *net); */
/* void dhd_event(struct dhd_info *dhd, char *evpkt, int evlen, int ifidx); */
/* void dhd_sendup_event(dhd_pub_t *dhdp, wl_event_msg_t *event, void *data); */
/* void dhd_bus_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf); */
/* void dhd_bus_clearcounts(dhd_pub_t *dhdp); */
/* bool dhd_bus_dpc(struct dhd_bus *bus); */

#define GET_DHDINFO(xx)               (dhd_info_t *)((xx)->if_softc)
#ifdef BCM_FD_AGGR
#define DBUS_RX_BUFFER_SIZE_DHD(net)    (BCM_RPC_TP_DNGL_AGG_MAX_BYTE)
#else
#ifndef PROP_TXSTATUS
#define DBUS_RX_BUFFER_SIZE_DHD(net)    (net->mtu + net->hard_header_len + dhd->pub.hdrlen)
#else
#define DBUS_RX_BUFFER_SIZE_DHD(net)    (net->mtu + net->hard_header_len + dhd->pub.hdrlen + 128)
#endif
#endif /* BCM_FD_AGGR */
#define copy_from_user(dst, src, len) copyin(src, dst, len)
#define copy_to_user(dst, src, len)   copyout(src, dst, len)

#define DHD_FBSD_NET_INTF_NAME  "wl"

#if defined(FBSD_MUL_VIF) && defined(FBSD_ENABLE_P2P_DEV_IF)
#define DHD_FBSD_P2P_DEV_IFIDX		1
#define DHD_FBSD_P2P_DEV_IF_NAME	"p2p0"
#endif /* FBSD_MUL_VIF && FBSD_ENABLE_P2P_DEV_IF */

/* Interface control information */
typedef struct dhd_if {
	struct dhd_info  *info;   /* back pointer to dhd_info */
	struct ifnet     *net;    /* OS/stack specifics */

#ifdef FBSD_MUL_VIF
	struct ifmedia	sc_ifmedia;
#endif /* FBSD_MUL_VIF */

	int    idx;        /* iface idx in dongle */
	int    state;      /* interface state */
	uint   subunit;    /* subunit */
	uint8  mac_addr[ETHER_ADDR_LEN]; /* assigned MAC address */
	bool   attached;            /* Delayed attachment when unset */
	bool   txflowcontrol;       /* Per interface flow control indicator */
	char   name[IFNAMSIZ+1];    /* Interface name */
	uint8  bssidx;              /* bsscfg index for the interface */
} dhd_if_t;

/* Local private structure (extension of pub) */
typedef struct dhd_info {
	dhd_pub_t   pub;       /* pub MUST be the first one */
	void        *softc;

#ifndef FBSD_MUL_VIF
	struct ifmedia  sc_ifmedia;
#endif /* !FBSD_MUL_VIF */

	dhd_if_t    *iflist[DHD_MAX_IFS];  /* For supporting multiple interfaces */

	struct cv   ioctl_resp_wait;       /* FreeBSD using cv for waiting */
	struct mtx  ioctl_resp_wait_lock;

	struct mtx    txqlock;
	struct mtx    ioctl_lock;
	struct sx     proto_sem;
	struct mtx    tx_lock;

#ifdef PROP_TXSTATUS
	/* Mutex for wl flow control */
	struct mtx    wlfc_mutex;
#endif /* PROP_TXSTATUS */

#ifndef FBSD_MUL_VIF
	struct ether_addr macvalue;
#endif /* !FBSD_MUL_VIF */
	dhd_attach_states_t    dhd_state;
	int    pend_8021x_cnt;
#ifdef BCM_FD_AGGR
	void *rpc_th;
	void *rpc_osh;
	int  rpcth_hz;
	struct callout rpcth_timer;
	bool rpcth_timer_active;
	uint8 fdaggr;
#endif

	struct callout txfc_timer;
	struct ifnet *txfc_net;
	int txfc_ifidx;
} dhd_info_t;


/* Static Functions */
static int dhd_netif_media_change(struct ifnet *ifp);
static void dhd_netif_media_status(struct ifnet *ifp, struct ifmediareq *imr);
static int dhd_wait_pend8021x(struct ifnet *ifp);
/* static int dhd_wl_host_event(dhd_info_t *dhd, int *ifidx, void *pktdata,
				wl_event_msg_t *event_ptr, void **data_ptr);
*/
int dhd_net2idx(dhd_info_t *dhd, struct ifnet *net);

#ifdef TOE
#ifndef BDC
#error TOE requires BDC
#endif /* !BDC */
static int dhd_toe_get(dhd_pub_t *dhdp, int idx, uint32 *toe_ol);
static int dhd_toe_set(dhd_pub_t *dhdp, int idx, uint32 toe_ol);
#endif /* TOE */
static void dhd_bus_detach(dhd_pub_t *dhdp);
static void dhd_open(void *priv);
static int dhd_stop(struct ifnet *net);
static int dhd_ioctl_entry(struct ifnet *net, u_long cmd, caddr_t data);
static void dhd_start_xmit(struct ifnet *net);
static void dhd_dbus_disconnect_cb(void *arg);
static void *dhd_dbus_probe_cb(void *arg, const char *desc, uint32 bustype, uint32 hdrlen);


/* Needed for dhd.h */
uint dhd_roam_disable = 1;
uint dhd_radio_up = 1;

int dhd_ioctl_timeout_msec = IOCTL_RESP_TIMEOUT;

/* User-specified vid/pid, may be set from environment by using "kenv" command */
static int dhd_vid = 0xa5c;
static int dhd_pid = 0x48f;

static char dhd_version[] = "Dongle Host Driver, version " EPI_VERSION_STR;

/* The USB device structure */
extern device_t bwl_device;

static void dhd_dbus_send_complete(void *handle, void *info, int status);
static void dhd_dbus_recv_buf(void *handle, uint8 *buf, int len);
static void dhd_dbus_recv_pkt(void *handle, void *pkt);
static void dhd_dbus_txflowcontrol(void *handle, bool onoff);
static void dhd_dbus_errhandler(void *handle, int err);
static void dhd_dbus_ctl_complete(void *handle, int type, int status);
static void dhd_dbus_state_change(void *handle, int state);
static void *dhd_dbus_pktget(void *handle, uint len, bool send);
static void dhd_dbus_pktfree(void *handle, void *p, bool send);
#ifdef BCM_FD_AGGR
static void dhd_rpcth_watchdog(void *data);
static void dbus_rpcth_tx_complete(void *ctx, void *pktbuf, int status);
static void dbus_rpcth_rx_aggrbuf(void *context, uint8 *buf, int len);
static void dbus_rpcth_rx_aggrpkt(void *context, void *rpc_buf);
#endif

static dbus_callbacks_t dhd_dbus_cbs = {
#ifdef BCM_FD_AGGR
	.send_complete  = dbus_rpcth_tx_complete,
	.recv_buf       = dbus_rpcth_rx_aggrbuf,
	.recv_pkt       = dbus_rpcth_rx_aggrpkt,
#else

	.send_complete  = dhd_dbus_send_complete,
	.recv_buf       = dhd_dbus_recv_buf,
	.recv_pkt       = dhd_dbus_recv_pkt,
#endif
	.txflowcontrol  = dhd_dbus_txflowcontrol,
	.errhandler     = dhd_dbus_errhandler,
	.ctl_complete   = dhd_dbus_ctl_complete,
	.state_change   = dhd_dbus_state_change,
	.pktget         = dhd_dbus_pktget,
	.pktfree        = dhd_dbus_pktfree
};


/*
 * Pubic functions for now. This needs to go in the header file
 */
struct net_device*
dhd_handle_ifadd(dhd_info_t *dhd, int ifidx, char *name,
        uint8 *mac, uint8 bssidx, bool need_rtnl_lock);
int
dhd_handle_ifdel(dhd_info_t *dhd, int ifidx, bool need_rtnl_lock);
/*
*
* Some OS specific functions required to implement DHD driver in OS independent way
*
*/
int
dhd_os_proto_block(dhd_pub_t *pub)
{
	dhd_info_t * dhd = (dhd_info_t *)(pub->info);

	if (dhd) {
		if (!sx_xlock_sig(&dhd->proto_sem))
			return 1;
	}
	return 0;
}

int
dhd_os_proto_unblock(dhd_pub_t *pub)
{
	dhd_info_t * dhd = (dhd_info_t *)(pub->info);

	if (dhd) {
		sx_xunlock(&dhd->proto_sem);
		return 1;
	}
	return 0;
}

unsigned int
dhd_os_get_ioctl_resp_timeout(void)
{
	return ((unsigned int)dhd_ioctl_timeout_msec);
}

void
dhd_os_set_ioctl_resp_timeout(unsigned int timeout_msec)
{
	dhd_ioctl_timeout_msec = (int)timeout_msec;
}

void
dhd_os_ioctl_resp_lock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = pub->info;
	MTX_LOCK(&dhd->ioctl_resp_wait_lock);
}

void
dhd_os_ioctl_resp_unlock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = pub->info;
	MTX_UNLOCK(&dhd->ioctl_resp_wait_lock);
}

int
dhd_os_ioctl_resp_wait(dhd_pub_t *pub, uint *condition)
{
	dhd_info_t *dhd = pub->info;
	int err = 0;
	struct timeval tv;

	if (!(*condition)) {
		tv.tv_sec  = dhd_ioctl_timeout_msec / 1000;
		tv.tv_usec = (dhd_ioctl_timeout_msec % 1000) * 1000;

		err = cv_timedwait(&dhd->ioctl_resp_wait,
			&dhd->ioctl_resp_wait_lock, tvtohz(&tv));
	}

	if (err) {
		DHD_ERROR(("Wait ioctl_resp_wait timeout = %d\n", dhd_ioctl_timeout_msec));
	}
	return (err == 0);
}

int
dhd_os_ioctl_resp_wake(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *) (pub->info);

	cv_broadcast(&dhd->ioctl_resp_wait);
	return 0;
}

void
dhd_os_wd_timer(void *bus, uint wdtick)
{
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return;
}

void *
dhd_os_open_image(char *filename)
{
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return NULL;
}

int
dhd_os_get_image_block(char *buf, int len, void *image)
{
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return -1;
}

void
dhd_os_close_image(void *image)
{
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return;
}

void
dhd_os_sdlock_txq(dhd_pub_t *pub)
{
	dhd_info_t *dhd;

	dhd = (dhd_info_t *)(pub->info);
	MTX_LOCK(&dhd->txqlock);
}

void
dhd_os_sdunlock_txq(dhd_pub_t *pub)
{
	dhd_info_t *dhd;

	dhd = (dhd_info_t *)(pub->info);

	MTX_UNLOCK(&dhd->txqlock);
}
#ifdef PROP_TXSTATUS
int
dhd_os_wlfc_block(dhd_pub_t *pub)
{
	dhd_info_t *di;
	di = (dhd_info_t *)(pub->info);

	ASSERT(di != NULL);
	MTX_LOCK(&di->wlfc_mutex);
	return 1;
}

int
dhd_os_wlfc_unblock(dhd_pub_t *pub)
{
	dhd_info_t *di;
	di = (dhd_info_t *)(pub->info);

	ASSERT(di != NULL);
	MTX_UNLOCK(&di->wlfc_mutex);
	return 1;
}

bool dhd_wlfc_skip_fc(void)
{
	return FALSE;
}
void dhd_wlfc_plat_init(void *dhd)
{
	return;
}
void dhd_wlfc_plat_deinit(void *dhd)
{
	return;
}
#endif /* endif PROP_TXSTATUS */

void
dhd_os_sdlock_rxq(dhd_pub_t *pub)
{
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return;
}

void
dhd_os_sdunlock_rxq(dhd_pub_t *pub)
{
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return;
}

void
dhd_timeout_start(dhd_timeout_t *tmo, uint usec)
{
	DHD_TRACE(("%s(): Enter\n", __FUNCTION__));
	tmo->limit = usec;
	tmo->increment = 0;
	tmo->elapsed = 0;
	/* tmo->tick = 1000000/HZ;  TODO: HOW TO GET THE HZ */
	DHD_ERROR(("%s(): ERROR.. NEED TO know the tick\n", __FUNCTION__));
}

int
dhd_timeout_expired(dhd_timeout_t *tmo)
{
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return -1;
}

int
dhd_net2idx(dhd_info_t *dhd, struct ifnet *net)
{
	int i = 0;

	ASSERT(dhd);
	while (i < DHD_MAX_IFS) {
		/* DHD_TRACE(("%s(): dhd=0x%x, dhd->iflist[%d]=0x%x \n",
			__FUNCTION__, (uint32) dhd, i, (uint32) dhd->iflist[i]));
		*/
		if (dhd->iflist[i] && (dhd->iflist[i]->net == net))
			return i;
		i++;
	}

	DHD_ERROR(("%s(): ERROR: network interface not found! \n", __FUNCTION__));

	return DHD_BAD_IF;
}

struct ifnet *
dhd_idx2net(struct dhd_pub *dhd_pub, int ifidx)
{
	struct dhd_info *dhd_info;

	if (!dhd_pub || ifidx < 0 || ifidx >= DHD_MAX_IFS) {
		if (!dhd_pub)
			DHD_ERROR(("%s(): ERROR: dhd_pub is NULL \n", __FUNCTION__));
		if (ifidx < 0 || ifidx >= DHD_MAX_IFS)
			DHD_ERROR(("%s(): ERROR: ifidx is %d out of range\n", __FUNCTION__, ifidx));
		return NULL;
	}

	dhd_info = dhd_pub->info;
	if (!dhd_info) {
		DHD_ERROR(("%s(): ERROR: dhd_info is NULL \n", __FUNCTION__));
		return NULL;
	}
	if (!dhd_info->iflist[ifidx]) {
		/* DHD_ERROR(("%s(): ERROR: iflist[%d] is NULL \n", __FUNCTION__, ifidx)); */
		return NULL;
	}

	return dhd_info->iflist[ifidx]->net;
}

int
dhd_ifname2idx(dhd_info_t *dhd, char *name)
{
	int i = DHD_MAX_IFS;

	ASSERT(dhd);

	if (name == NULL || *name == '\0') {
		return 0;
	}

	for (i = 0; i < DHD_MAX_IFS; i++) {
		if (dhd->iflist[i] && !strncmp(dhd->iflist[i]->name, name, IFNAMSIZ))
			break;
	}

	if (i == DHD_MAX_IFS) {
		i = 0;  /* default - the primary interface */
		DHD_ERROR(("%s(): ERROR: interface %s not in the list, return default idx 0 \n",
			__FUNCTION__, name));
	}

	DHD_TRACE(("%s(): Return idx %d for interface '%s' \n", __FUNCTION__, i, name));
	return i;
}

char *
dhd_ifname(dhd_pub_t *dhdp, int ifidx)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;

	ASSERT(dhd);

	if (ifidx < 0 || ifidx >= DHD_MAX_IFS) {
		DHD_ERROR(("%s(): ifidx %d out of range\n", __FUNCTION__, ifidx));
		return "<if_bad>";
	}

	if (dhd->iflist[ifidx] == NULL) {
		DHD_ERROR(("%s(): Null i/f %d\n", __FUNCTION__, ifidx));
		return "<if_null>";
	}

	if (dhd->iflist[ifidx]->net == NULL) {
		DHD_ERROR(("%s(): dhd->iflist[%d]->net not allocated\n", __FUNCTION__, ifidx));
		return "<if_none>";
	}

	return (dhd->iflist[ifidx]->name);
}

int dhd_netif_media_change(struct ifnet *ifp)
{
	DHD_TRACE(("%s(): Not implemented \n", __FUNCTION__));
	return BCME_OK;
}

void dhd_netif_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
	dhd_pub_t *pub = (dhd_pub_t *) ifp->if_softc;
	imr->ifm_active = IFM_ETHER;
	imr->ifm_status = IFM_AVALID;
	if (ifp->if_flags & IFF_UP) {
		imr->ifm_status = IFM_ACTIVE;
		pub->busstate = DHD_BUS_DATA;
		DHD_TRACE(("%s(): set ifm_status active busstate up\n", __FUNCTION__));
	} else {
		pub->busstate = DHD_BUS_SUSPEND;
		DHD_TRACE(("%s(): set busstate suspend\n", __FUNCTION__));
	}
	return;
}

static int
dhd_get_pend_8021x_cnt(dhd_info_t *dhd)
{
	return (atomic_load_acq_int(&dhd->pend_8021x_cnt));
}

int
dhd_wait_pend8021x(struct ifnet *ifp)
{
	dhd_info_t *dhd = (dhd_info_t *)ifp->if_softc;
	int ntimes      = MAX_WAIT_FOR_8021X_TX;
	int pend        = dhd_get_pend_8021x_cnt(dhd);
	struct timeval tv;
	int timeout;

	tv.tv_sec  = 0;
	tv.tv_usec = DELAY_FOR_8021X_TX;
	timeout = tvtohz(&tv);

	while (ntimes && pend) {
		if (pend) {
			DHD_PERIM_UNLOCK(&dhd->pub);
			pause("wait8021x", timeout);
			DHD_PERIM_LOCK(&dhd->pub);
			ntimes--;
		}
		pend = dhd_get_pend_8021x_cnt(dhd);
	}
	if (ntimes == 0)
	{
		atomic_readandclear_int(&dhd->pend_8021x_cnt);
		DHD_ERROR(("%s: TIMEOUT\n", __FUNCTION__));
	}
	return pend;
}

static void
_dhd_txfc_timer(void *data)
{
	dhd_info_t *dhd = (dhd_info_t *)data;
	struct ifnet *net;
	int i;

	if (dhd->txfc_net && (dhd->txfc_ifidx != ALL_INTERFACES)) {
		if (!(dhd->txfc_net->if_drv_flags & IFF_DRV_OACTIVE)) {
			DHD_INFO(("callout txfc timer if_start\n"));
			if_start(dhd->txfc_net);
		}
	} else if (!dhd->txfc_net && (dhd->txfc_ifidx == ALL_INTERFACES)) {
		for (i = 0; i < DHD_MAX_IFS; i++) {
			net = dhd_idx2net(&dhd->pub, i);
			if (net && (net->if_flags & IFF_UP) &&
				(!(net->if_drv_flags & IFF_DRV_OACTIVE)))
			{
				DHD_INFO(("callout txfc timer if_start\n"));
				if_start(net);
			}
		}
	}
}

#ifdef TOE
/* Retrieve current toe component enables, which are kept as a bitmap in toe_ol iovar */
static int
dhd_toe_get(dhd_pub_t *dhdp, int ifidx, uint32 *toe_ol)
{
	int ret = dhd_wl_ioctl_get_intiovar(dhdp, "toe_ol",
		toe_ol, WLC_GET_VAR, FALSE, ifidx);
	if (ret) {
		/* Check for older dongle image that doesn't support toe_ol */
		if (ret == -EIO) {
			DHD_ERROR(("%s: toe not supported by device\n",
				dhd_ifname(dhdp, ifidx)));
			return BCME_UNSUPPORTED;
		}

		DHD_INFO(("%s: could not get toe_ol: ret=%d\n", dhd_ifname(dhdp, ifidx), ret));
	}

	return ret;
}

/* Set current toe component enables in toe_ol iovar, and set toe global enable iovar */
static int
dhd_toe_set(dhd_pub_t *dhdp, int ifidx, uint32 toe_ol)
{
	int toe, ret;

	/* Set toe_ol as requested */

	ret = dhd_wl_ioctl_set_intiovar(dhdp, "toe_ol",
		toe_ol, WLC_SET_VAR, TRUE, ifidx);
	if (ret) {
		DHD_ERROR(("%s: could not set toe_ol: ret=%d\n",
			dhd_ifname(dhdp, ifidx), ret));
		return ret;
	}

	/* Enable toe globally only if any components are enabled. */

	toe = (toe_ol != 0);
	ret = dhd_wl_ioctl_set_intiovar(dhdp, "toe",
		toe, WLC_SET_VAR, TRUE, ifidx);
	if (ret) {
		DHD_ERROR(("%s: could not set toe: ret=%d\n", dhd_ifname(dhdp, ifidx), ret));
		return ret;
	}

	return 0;
}

#endif /* TOE */

/*
*
* Some API's needed for external use (for dhd.h)
*
*/
int dhd_change_mtu(dhd_pub_t *dhdp, int new_mtu, int ifidx)
{
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return -1;
}

int dhd_do_driver_init(struct net_device *net)
{
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return -1;
}
void
dhd_event(struct dhd_info *dhd, char *evpkt, int evlen, int ifidx)
{

#ifdef DHD_NET80211
	dhd_if_t *ifp;
	wl_event_msg_t event;

	DHD_TRACE(("%s(): Started.  ifidx = %d, evlen = %d \n", __FUNCTION__, ifidx, evlen));

	memcpy(&event, &(((bcm_event_t *)evpkt)->event), sizeof(wl_event_msg_t));

	ifp = dhd->iflist[ifidx];
	if (ifp == NULL)
		ifp = dhd->iflist[0];

	dhd_net80211_event_proc(&dhd->pub, ifp->net, &event, 0);
#endif
	return;
}

/* send up locally generated event */
void
dhd_sendup_event(dhd_pub_t *dhdp, wl_event_msg_t *event, void *data)
{
#ifdef NOT_YET
	switch (ntoh32(event->event_type)) {
	default:
		break;
	}
#else
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return;
#endif
}

void
dhd_bus_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf)
{
	bcm_bprintf(strbuf, "Bus USB\n");
}

void
dhd_bus_clearcounts(dhd_pub_t *dhdp)
{
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return;
}

bool
dhd_bus_dpc(struct dhd_bus *bus)
{
	return FALSE;
}

int
dhd_dbus_txdata(dhd_pub_t *dhdp, void *pktbuf)
{

	if (dhdp->txoff)
		return BCME_EPERM;

#ifdef BCM_FD_AGGR
	if (((dhd_info_t *)(dhdp->info))->fdaggr & BCM_FDAGGR_H2D_ENABLED) {

		dhd_info_t *dhd;
		int ret;
		dhd = (dhd_info_t *)(dhdp->info);
		ret = bcm_rpc_tp_buf_send(dhd->rpc_th, pktbuf);
		if (dhd->rpcth_timer_active == FALSE) {
			dhd->rpcth_timer_active = TRUE;
			callout_reset(&dhd->rpcth_timer, dhd->rpcth_hz,
				dhd_rpcth_watchdog, (void *)dhdp->info);
		}
		return ret;
	} else
#endif /* BCM_FD_AGGR */
	return dbus_send_pkt(dhdp->dbus, pktbuf, pktbuf);
}

/*
*
* API's for dhd callback
*
*/
void
dhd_txcomplete(dhd_pub_t *dhdp, void *txp, bool success)
{
	dhd_info_t *dhd = (dhd_info_t *)(dhdp->info);
	struct ether_header *eh;
	struct mbuf *p;

	dhd_prot_hdrpull(dhdp, NULL, txp, NULL, NULL);

	/*
	 * osl_pktpull leaves some mbufs with zero length
	 * skip over to find the mbuf with data to get
	 * protocol header
	 */
	p = (struct mbuf *) txp;
	while (p && (p->m_len == 0))
		p = p->m_next;

	eh = (struct ether_header *)p->m_data;

	if (ETHER_TYPE_802_1X == ntoh16(eh->ether_type))
		atomic_subtract_int(&dhd->pend_8021x_cnt, 1);

#ifdef PROP_TXSTATUS
	if (dhdp->wlfc_state && (dhdp->proptxstatus_mode != WLFC_FCMODE_NONE)) {
		uint datalen  = PKTLEN(dhd->pub.osh, txp);

		if (success) {
			dhdp->dstats.tx_packets++;
			dhdp->tx_packets++;
			dhdp->dstats.tx_bytes += datalen;
		} else {
			dhdp->dstats.tx_dropped++;
			dhdp->tx_dropped++;
		}
	}
#endif /* PROP_TXSTATUS */
}

int
dhd_sendpkt(dhd_pub_t *dhdp, int ifidx, void *pktbuf)
{
	struct ether_header *eh = NULL;
	dhd_info_t *dhd = (dhd_info_t *)(dhdp->info);
	int ret = BCME_OK;

	/* Reject if down */
	if (!dhdp->up || (dhdp->busstate == DHD_BUS_DOWN)) {
		if (!dhdp->up) {
			DHD_ERROR(("%s(): !dhdp->up \n", __FUNCTION__));
		}
		if (dhdp->busstate == DHD_BUS_DOWN) {
			DHD_ERROR(("%s(): (dhdp->busstate == DHD_BUS_DOWN) \n", __FUNCTION__));
		}
		/* free the packet here since the caller won't */
		PKTFREE(dhdp->osh, pktbuf, TRUE);
		return -ENODEV;
	}

	/* Update multicast statistic */
	if (PKTLEN(dhdp->osh, pktbuf) >= ETHER_HDR_LEN) {
		uint8 *pktdata = (uint8 *)PKTDATA(dhdp->osh, pktbuf);
		eh = (struct ether_header *)pktdata;

		 if (ntoh16(eh->ether_type) == ETHER_TYPE_802_1X)
			atomic_add_int(&dhd->pend_8021x_cnt, 1);

	} else {
		PKTCFREE(dhdp->osh, pktbuf, TRUE);
		return BCME_ERROR;
	}

	/* Look into the packet and update the packet priority */
	pktsetprio(pktbuf, FALSE);

#if defined(FBSD_MUL_VIF) && defined(FBSD_ENABLE_P2P_DEV_IF)
	if (ifidx >= DHD_FBSD_P2P_DEV_IFIDX) {
		ifidx --;
	}
#endif /* FBSD_MUL_VIF && FBSD_ENABLE_P2P_DEV_IF */

#ifdef PROP_TXSTATUS
	if (dhd_wlfc_is_supported(dhdp)) {
		uint8 *pktdata = (uint8 *)PKTDATA(dhdp->osh, pktbuf);
		eh = (struct ether_header *)pktdata;

		/* store the interface ID */
		DHD_PKTTAG_SETIF(PKTTAG(pktbuf), ifidx);

		/* store destination MAC in the tag as well */
		DHD_PKTTAG_SETDSTN(PKTTAG(pktbuf), eh->ether_dhost);

		/* decide which FIFO this packet belongs to */
		if (ETHER_ISMULTI(eh->ether_dhost)) {
			/* Update multicast statistic */
			dhdp->tx_multicast++;
			/* one additional queue index (highest AC + 1) is used for bc/mc queue */
			DHD_PKTTAG_SETFIFO(PKTTAG(pktbuf), AC_COUNT);
		 } else
			DHD_PKTTAG_SETFIFO(PKTTAG(pktbuf), WME_PRIO2AC(PKTPRIO(pktbuf)));
	} else
#endif /* PROP_TXSTATUS */
	/* If the protocol uses a data header, apply it */

	dhd_prot_hdrpush(dhdp, ifidx, &pktbuf);

	/* Use bus module to send data frame */
#ifdef WLMEDIA_HTSF
	dhd_htsf_addtxts(dhdp, pktbuf);
#endif

#ifdef PROP_TXSTATUS
	if (dhd_wlfc_commit_packets(dhdp, (f_commitpkt_t)dhd_dbus_txdata,
		dhdp, pktbuf, TRUE) == WLFC_UNSUPPORTED) {
		/* non-proptxstatus way */
		ret = dhd_dbus_txdata(dhdp, pktbuf);
	}
#else
	ret = dhd_dbus_txdata(dhdp, pktbuf);
#endif
	if (ret)
		PKTFREE(dhdp->osh, pktbuf, TRUE);
	return ret;
}


void
dhd_rx_frame(dhd_pub_t *dhdp, int ifidx, void *pktbuf, int numpkt, uint8 chan)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;
	struct mbuf *m = NULL;
	void *pnext;
	int i;
	dhd_if_t *ifp;
	struct ether_header *eh;

	DHD_TRACE(("%s(): Enter\n", __FUNCTION__));

#if defined(FBSD_MUL_VIF) && defined(FBSD_ENABLE_P2P_DEV_IF)
	if (ifidx >= DHD_FBSD_P2P_DEV_IFIDX) {
		ifidx ++;
	}

	if (ifidx >= DHD_MAX_IFS) {
		ifidx = 0;
	}
#endif /* FBSD_MUL_VIF && FBSD_ENABLE_P2P_DEV_IF */

	for (i = 0; pktbuf && i < numpkt; i++, pktbuf = pnext) {

		pnext = PKTNEXT(dhdp->osh, pktbuf);
		PKTSETNEXT(dhdp->osh, pktbuf, NULL);

		m = PKTTONATIVE(dhdp->osh, pktbuf);

		ifp = dhd->iflist[ifidx];
		if (ifp == NULL) {
			ifp = dhd->iflist[0];
			DHD_ERROR(("%s(): dhd->iflist[%d] is NULL, use primary one (0) instead\n",
				__FUNCTION__, ifidx));
		}

		ASSERT(ifp);

		m->m_pkthdr.rcvif = ifp->net;

		eh = mtod(m, struct ether_header *);

#ifdef PROP_TXSTATUS
		if (dhd_wlfc_is_header_only_pkt(dhdp, pktbuf)) {
			/* WLFC may send header only packet when
			 * there is an urgent message but no packet to
			 * piggy-back on
			 */
			PKTCFREE(dhdp->osh, pktbuf, FALSE);
			continue;
		}
#endif
		/* Process special event packets and then discard them */
		if (ntoh16(eh->ether_type) == ETHER_TYPE_BRCM) {
			wl_event_msg_t event;
			void *data;
			memset(&event, 0, sizeof(event));
			DHD_TRACE(("%s(): ETHER_TYPE_BRCM received. \n", __FUNCTION__));
			wl_host_event(dhdp, &ifidx, m->m_data, &event, &data, NULL);
			wl_event_to_host_order(&event);
#ifdef DHD_DONOT_FORWARD_BCMEVENT_AS_NETWORK_PKT
			PKTFREE(dhdp->osh, pktbuf, FALSE);
			continue;
#endif /* DHD_DONOT_FORWARD_BCMEVENT_AS_NETWORK_PKT */
		} else {
#ifdef PROP_TXSTATUS
			dhd_wlfc_save_rxpath_ac_time(dhdp, (uint8)PKTPRIO(pktbuf));
#endif /* PROP_TXSTATUS */
		}

		ASSERT(ifidx < DHD_MAX_IFS && dhd->iflist[ifidx]);
		if (dhd->iflist[ifidx] && !dhd->iflist[ifidx]->state)
			ifp = dhd->iflist[ifidx];

		if (ntoh16(eh->ether_type) != ETHER_TYPE_BRCM) {
			dhdp->dstats.rx_bytes += PKTLEN(dhdp->osh, pktbuf);
			dhdp->dstats.rx_packets++;
			dhdp->rx_packets++; /* Local count */
		}

		bcm_object_trace_opr(m, BCM_OBJDBG_REMOVE, __FUNCTION__, __LINE__);
		(*ifp->net->if_input)(ifp->net, m);

	}
	/*
	// DHD_OS_WAKE_LOCK_TIMEOUT_ENABLE was replaced by DHD_OS_WAKE_LOCK_RX_TIMEOUT_ENABLE
	// and DHD_OS_WAKE_LOCK_CTRL_TIMEOUT_ENABLE, but all these 3 macros in FreeBSD are
	// empty (defined as NULL), so it is safe to just comment it out
	DHD_OS_WAKE_LOCK_TIMEOUT_ENABLE(dhdp, DHD_PACKET_TIMEOUT_MS);
	*/
}


static int
bwl_tx_data(dhd_pub_t *pub, int ifidx, struct mbuf *m)
{
	struct m_tag    *mtag;

	if (m_tag_find(m, FREEBSD_PKTTAG, (struct m_tag *)NULL) == NULL) {
		mtag = m_tag_get(FREEBSD_PKTTAG, OSL_PKTTAG_SZ, M_DONTWAIT);
		if (!mtag) {
			DHD_ERROR(("%s(): FREEBSD_PKTTAG not found \n", __FUNCTION__));
			m_freem(m);
			return -1;
		}
		bzero((void *)(mtag + 1), OSL_PKTTAG_SZ);
	        m_tag_prepend(m, mtag);
	}

	if (m_tag_find(m, FREEBSD_PKTPRIO, (struct m_tag *)NULL) == NULL) {
		mtag = m_tag_get(FREEBSD_PKTPRIO, OSL_PKTPRIO_SZ, M_DONTWAIT);
		if (!mtag) {
			DHD_ERROR(("%s(): FREEBSD_PKTPRIO not found \n", __FUNCTION__));
			m_freem(m);
			return -1;
		}
		bzero((void *)(mtag + 1), OSL_PKTPRIO_SZ);
		m_tag_prepend(m, mtag);
	}

	bcm_object_trace_opr(m, BCM_OBJDBG_ADD_PKT, __FUNCTION__, __LINE__);
	dhd_sendpkt(pub, ifidx, m);

	return 0;
}


void
dhd_txflowcontrol(dhd_pub_t *dhdp, int ifidx, bool state)
{
	struct ifnet *net;
	int i;

	if (ifidx == ALL_INTERFACES) {
		dhdp->info->txfc_net = NULL;
		for (i = 0; i < DHD_MAX_IFS; i++) {
			net = dhd_idx2net(dhdp, i);
			if (net != NULL) {
				if (state == ON) {
					net->if_drv_flags |= IFF_DRV_OACTIVE;
				} else {
					net->if_drv_flags &= ~IFF_DRV_OACTIVE;
				}
			}
		}
	}
	else {
		net = dhd_idx2net(dhdp, ifidx);
		dhdp->info->txfc_net = net;
		if (net != NULL) {
			if (state == ON) {
				net->if_drv_flags |= IFF_DRV_OACTIVE;
			} else {
				net->if_drv_flags &= ~IFF_DRV_OACTIVE;
			}
		}
	}

	if (state == OFF) {
		dhdp->info->txfc_ifidx = ifidx;
		callout_reset(&dhdp->info->txfc_timer, 0, _dhd_txfc_timer, (void *)dhdp->info);
	}
}


static void
dhd_op_if(dhd_if_t *ifp)
{
	dhd_info_t *dhd;
	int ret = 0, err = 0;

	ASSERT(ifp && ifp->info && ifp->idx);	/* Virtual interfaces only */

	dhd = ifp->info;

	DHD_TRACE(("%s(): idx %d, state %d\n", __FUNCTION__, ifp->idx, ifp->state));

	switch (ifp->state) {
		case WLC_E_IF_ADD:
			/* Delete the existing interface before overwriting it,
			* in case if we missed the WLC_E_IF_DEL event.
			*/
			DHD_TRACE(("%s(): ifp->state = WLC_E_IF_ADD \n", __FUNCTION__));
			if (ifp->net != NULL) {
				DHD_ERROR(("%s(): ERROR, net device is NULL\n", __FUNCTION__));
#ifdef FBSD_MUL_VIF
				ifmedia_removeall(&ifp->sc_ifmedia);
#else
				ifmedia_removeall(&dhd->sc_ifmedia);
#endif /* FBSD_MUL_VIF */
				ether_ifdetach(ifp->net);
				if_free(ifp->net);
			}
			/* Allocate etherdev, including space for private structure */
			if (!(ifp->net = if_alloc(IFT_ETHER))) {
				DHD_ERROR(("%s(): ERROR.. if_alloc() failed \n", __FUNCTION__));
				ret = -ENOMEM;
			}
			if (ret == 0) {
				if_initname(ifp->net, ifp->name, device_get_unit(bwl_device));
				ifp->net->if_softc = dhd;
				if ((err = dhd_register_if(&dhd->pub, ifp->idx, TRUE)) != 0) {
					DHD_ERROR(("%s(): ERROR.. if_alloc() failed. err = %d \n",
						__FUNCTION__, err));
					ret = -EOPNOTSUPP;
				} else {
					ifp->state = 0;
				}
			}
			break;
		case WLC_E_IF_DEL:
			DHD_TRACE(("%s(): ifp->state = WLC_E_IF_DEL \n", __FUNCTION__));
			if (ifp->net != NULL) {
				DHD_TRACE(("\n%s: got 'WLC_E_IF_DEL' state\n", __FUNCTION__));
#ifdef FBSD_MUL_VIF
				ifmedia_removeall(&ifp->sc_ifmedia);
				ether_ifdetach(ifp->net);
#endif /* FBSD_MUL_VIF */
				ret = DHD_DEL_IF;	/* Make sure the free_netdev() is called */
			}
			break;
		default:
			DHD_TRACE(("%s(): ifp->state = %d.  Not handled \n",
				__FUNCTION__, ifp->state));
			ASSERT(!ifp->state);
			break;
	}

	if (ret < 0) {
		if (ifp->net) {
			if_free(ifp->net);
		}
		dhd->iflist[ifp->idx] = NULL;
		MFREE(dhd->pub.osh, ifp, sizeof(*ifp));
	}
}


static void
dhd_dbus_send_complete(void *handle, void *info, int status)
{
	dhd_info_t *dhd = (dhd_info_t *)handle;
	void *pkt = info;

	DHD_TRACE(("%s(): Started. status = %d \n", __FUNCTION__, status));

	if (pkt == NULL)
		return;
	if (dhd == NULL) {
		DHD_ERROR(("%s(): ERROR.. dhd = NULL \n", __FUNCTION__));
		return;
	}

	if (status == DBUS_OK) {
		dhd->pub.dstats.tx_packets++;
		dhd->pub.tx_packets++;
	} else {
		DHD_ERROR(("%s(): ERROR.. status (%d) != DBUS_OK \n",
			__FUNCTION__, status));
		dhd->pub.dstats.tx_errors++;
		dhd->pub.tx_errors++;
	}
#ifdef PROP_TXSTATUS
	if (DHD_PKTTAG_WLFCPKT(PKTTAG(pkt)) &&
		(dhd_wlfc_txcomplete(&dhd->pub, pkt, status == DBUS_OK) != WLFC_UNSUPPORTED)) {
		return;
	}
#endif /* PROP_TXSTATUS */


	PKTFREE(dhd->pub.osh, pkt, TRUE);

}

static void
dhd_dbus_recv_buf(void *handle, uint8 *buf, int len)
{
	dhd_info_t *dhd = (dhd_info_t *)handle;
	void *pkt;

	DHD_TRACE(("%s(): Started. len = %d \n", __FUNCTION__, len));

	if (dhd == NULL) {
		DHD_ERROR(("%s(): ERROR.. dhd = NULL \n", __FUNCTION__));
		return;
	}

	/* It looks fine for FreeBSD to use this */
	if ((pkt = PKTGET(dhd->pub.osh, len, FALSE)) == NULL) {
		DHD_ERROR(("%s(): ERROR.. PKTGET failed \n", __FUNCTION__));
		return;
	}

	bcopy(buf, PKTDATA(dhd->pub.osh, pkt), len);
	dhd_dbus_recv_pkt(dhd, pkt);
}

static void
dhd_dbus_recv_pkt(void *handle, void *pkt)
{
	uchar reorder_info_buf[WLHOST_REORDERDATA_TOTLEN];
	uint reorder_info_len;
	uint pkt_count;
	dhd_info_t *dhd = (dhd_info_t *)handle;
	int ifidx = 0;

	if (dhd == NULL) {
		DHD_ERROR(("%s(): ERROR.. dhd = NULL \n", __FUNCTION__));
		return;
	}

	/* If the protocol uses a data header, check and remove it */
	if (dhd_prot_hdrpull(&dhd->pub, &ifidx, pkt, reorder_info_buf,
		&reorder_info_len) != 0) {
		DHD_ERROR(("%s() rx protocol error\n", __FUNCTION__));
		PKTFREE(dhd->pub.osh, pkt, FALSE);
		dhd->pub.rx_errors++;
		return;
	}

	if (reorder_info_len) {
		/* Reordering info from the firmware */
		dhd_process_pkt_reorder_info(&dhd->pub, reorder_info_buf, reorder_info_len,
			&pkt, &pkt_count);
		if (pkt_count == 0)
			return;
	}
	else
		pkt_count = 1;

	dhd_rx_frame(&dhd->pub, ifidx, pkt, pkt_count, 0);
}

static void
dhd_dbus_txflowcontrol(void *handle, bool onoff)
{
	dhd_info_t *dhd = (dhd_info_t *)handle;
	bool wlfc_enabled = FALSE;

	DHD_TRACE(("%s(): Started. \n", __FUNCTION__));

	if (dhd == NULL) {
		DHD_ERROR(("%s(): ERROR.. dhd = NULL \n", __FUNCTION__));
		return;
	}

#ifdef PROP_TXSTATUS
	wlfc_enabled = (dhd_wlfc_flowcontrol(&dhd->pub, onoff, !onoff) != WLFC_UNSUPPORTED);
#endif

	if (!wlfc_enabled) {
		dhd_txflowcontrol(&dhd->pub, ALL_INTERFACES, onoff);
	}
}
static void
dhd_dbus_errhandler(void *handle, int err)
{
	DHD_ERROR(("%s(): Not implemented \n", __FUNCTION__));
	return;
}

static void
dhd_dbus_ctl_complete(void *handle, int type, int status)
{
	dhd_info_t *dhd = (dhd_info_t *)handle;

	DHD_TRACE(("%s(): Started. type = %d, status = %d \n",
		__FUNCTION__, type, status));

	if (dhd == NULL) {
		DHD_ERROR(("%s(): ERROR.. dhd = NULL \n", __FUNCTION__));
		return;
	}

	if (type == DBUS_CBCTL_READ) {
		if (status == DBUS_OK)
			dhd->pub.rx_ctlpkts++;
		else
			dhd->pub.rx_ctlerrs++;
	} else if (type == DBUS_CBCTL_WRITE) {
		if (status == DBUS_OK)
			dhd->pub.tx_ctlpkts++;
		else
			dhd->pub.tx_ctlerrs++;
	}

	dhd_prot_ctl_complete(&dhd->pub);
}

static void
dhd_dbus_state_change(void *handle, int state)
{
	dhd_info_t *dhd = (dhd_info_t *)handle;

	if (dhd == NULL)
		return;

	if (state == DBUS_STATE_DOWN) {
		DHD_TRACE(("%s: DBUS is down\n", __FUNCTION__));
		dhd->pub.busstate = DHD_BUS_DOWN;
	} else if (state == DBUS_STATE_UP) {
		DHD_TRACE(("%s: DBUS is up\n", __FUNCTION__));
		dhd->pub.busstate = DHD_BUS_DATA;
	}

	DHD_TRACE(("%s: DBUS current state=%d\n", __FUNCTION__, state));
}

static void *
dhd_dbus_pktget(void *handle, uint len, bool send)
{
	dhd_info_t *dhd = (dhd_info_t *)handle;
	void *p = NULL;

	DHD_TRACE(("%s(): Started. \n", __FUNCTION__));

	if (dhd == NULL) {
		DHD_ERROR(("%s(): ERROR.. dhd = NULL \n", __FUNCTION__));
		return NULL;
	}

	if (send == TRUE) {
		dhd_os_sdlock_txq(&dhd->pub);
		p = PKTGET(dhd->pub.osh, len, TRUE);
		dhd_os_sdunlock_txq(&dhd->pub);
	} else {
		dhd_os_sdlock_rxq(&dhd->pub);
		p = PKTGET(dhd->pub.osh, len, FALSE);
		dhd_os_sdunlock_rxq(&dhd->pub);
	}

	return p;
}

static void
dhd_dbus_pktfree(void *handle, void *p, bool send)
{
	dhd_info_t *dhd = (dhd_info_t *)handle;

	DHD_TRACE(("%s(): Started. \n", __FUNCTION__));

	if (dhd == NULL) {
		DHD_ERROR(("%s(): ERROR.. dhd = NULL \n", __FUNCTION__));
		return;
	}

	if (send == TRUE) {
#ifdef PROP_TXSTATUS
		if (DHD_PKTTAG_WLFCPKT(PKTTAG(p)) &&
			(dhd_wlfc_txcomplete(&dhd->pub, p, FALSE) != WLFC_UNSUPPORTED)) {
			return;
		}
#endif /* PROP_TXSTATUS */

		dhd_os_sdlock_txq(&dhd->pub);
		PKTFREE(dhd->pub.osh, p, TRUE);
		dhd_os_sdunlock_txq(&dhd->pub);
	} else {
		dhd_os_sdlock_rxq(&dhd->pub);
		PKTFREE(dhd->pub.osh, p, FALSE);
		dhd_os_sdunlock_rxq(&dhd->pub);
	}
}

#ifdef BCM_FD_AGGR

static void
dbus_rpcth_tx_complete(void *ctx, void *pktbuf, int status)
{
	dhd_info_t *dhd = (dhd_info_t *)ctx;

	/*
	 * mbuf chain get deleted all at once
	 */

	if (dhd->fdaggr & BCM_FDAGGR_H2D_ENABLED) {
		uint32 rpc_len, padhead_len;

		rpc_len = ltoh32_ua(bcm_rpc_buf_data(dhd->rpc_th, pktbuf));
		padhead_len = (rpc_len >> BCM_RPC_TP_PADHEAD_SHIFT) & 0x3;
		rpc_len &= BCM_RPC_TP_LEN_MASK;

		bcm_rpc_buf_pull(dhd->rpc_th, pktbuf, BCM_RPC_TP_ENCAP_LEN + padhead_len);

		bcm_rpc_buf_len_set(dhd->rpc_th, pktbuf, rpc_len);
	}

	dhd_dbus_send_complete(ctx, pktbuf, status);

}
static void
dbus_rpcth_rx_pkt(void *context, rpc_buf_t *rpc_buf)
{
	dhd_dbus_recv_pkt(context, rpc_buf);
}

static void
dbus_rpcth_rx_aggrpkt(void *context, void *rpc_buf)
{
	dhd_info_t *dhd = (dhd_info_t *)context;

	if (dhd == NULL)
		return;

	/* all the de-aggregated packets are delivered back to function dbus_rpcth_rx_pkt()
	* as cloned packets
	*/
	bcm_rpc_dbus_recv_aggrpkt(dhd->rpc_th, rpc_buf,
		bcm_rpc_buf_len_get(dhd->rpc_th, rpc_buf));

	/* free the original packet */
	dhd_dbus_pktfree(context, rpc_buf, FALSE);
}

static void
dbus_rpcth_rx_aggrbuf(void *context, uint8 *buf, int len)
{
	dhd_info_t *dhd = (dhd_info_t *)context;

	if (dhd == NULL)
		return;

	if (dhd->fdaggr & BCM_FDAGGR_D2H_ENABLED) {
		bcm_rpc_dbus_recv_aggrbuf(dhd->rpc_th, buf, len);
	}
	else {
		dhd_dbus_recv_buf(context, buf, len);
	}

}

static void
dhd_rpcth_watchdog(void *data)
{
	dhd_info_t *dhd = (dhd_info_t *)data;

	if (dhd->pub.dongle_reset) {
		return;
	}

	dhd->rpcth_timer_active = FALSE;
	/* release packets in the aggregation queue */
	bcm_rpc_tp_watchdog(dhd->rpc_th);
}

static int
dhd_fdaggr_ioctl(dhd_pub_t *dhd_pub, int ifindex, wl_ioctl_t *ioc, void *buf, int len)
{
	int bcmerror = 0;
	void *rpc_th;

	rpc_th = dhd_pub->info->rpc_th;

	if (!strcmp("rpc_agg", ioc->buf)) {
		uint32 rpc_agg;
		uint32 rpc_agg_host = 0;
		uint32 rpc_agg_dngl = 0;
		if (ioc->set) {
			memcpy(&rpc_agg, ioc->buf + strlen("rpc_agg") + 1, sizeof(uint32));
			rpc_agg = ltoh32(rpc_agg);
			rpc_agg_host = rpc_agg & BCM_RPC_TP_HOST_AGG_MASK;
			if (rpc_agg_host)
				bcm_rpc_tp_agg_set(rpc_th, rpc_agg_host, TRUE);
			else
				bcm_rpc_tp_agg_set(rpc_th, BCM_RPC_TP_HOST_AGG_MASK, FALSE);
			if (!dhd_wl_ioctl_set_intiovar(dhd_pub, ioc->buf,
				rpc_agg, ioc->cmd, ioc->set, ifindex)) {
				dhd_pub->info->fdaggr = 0;
				if (rpc_agg & BCM_RPC_TP_HOST_AGG_MASK)
					dhd_pub->info->fdaggr |= BCM_FDAGGR_H2D_ENABLED;
				if (rpc_agg & BCM_RPC_TP_DNGL_AGG_MASK)
					dhd_pub->info->fdaggr |= BCM_FDAGGR_D2H_ENABLED;
			}
		} else {
			rpc_agg_host = bcm_rpc_tp_agg_get(rpc_th);
			(void) dhd_wl_ioctl_get_intiovar(dhd_pub, ioc->buf,
				&rpc_agg_dngl, ioc->cmd, ioc->set, ifindex);
			rpc_agg = (rpc_agg_host & BCM_RPC_TP_HOST_AGG_MASK) |
				(rpc_agg_dngl & BCM_RPC_TP_DNGL_AGG_MASK);
			rpc_agg = htol32(rpc_agg);
			memcpy(buf, &rpc_agg, sizeof(uint32));
		}
	} else if (!strcmp("rpc_host_agglimit", ioc->buf)) {
		uint8 sf;
		uint16 bytes;
		uint32 agglimit;

		if (ioc->set) {
			memcpy(&agglimit, ioc->buf + strlen("rpc_host_agglimit") + 1,
				sizeof(uint32));
			agglimit = ltoh32(agglimit);
			sf = agglimit >> 16;
			bytes = agglimit & 0xFFFF;
			bcm_rpc_tp_agg_limit_set(rpc_th, sf, bytes);
		} else {
			bcm_rpc_tp_agg_limit_get(rpc_th, &sf, &bytes);
			agglimit = (uint32)((sf << 16) + bytes);
			agglimit = htol32(agglimit);
			memcpy(buf, &agglimit, sizeof(uint32));
		}

	} else {
		dhd_info_t *dhd = dhd_pub->info;
		MTX_UNLOCK(&dhd->ioctl_lock);
		bcmerror = dhd_wl_ioctl(dhd_pub, ifindex, ioc, buf, len);
		MTX_LOCK(&dhd->ioctl_lock);
	}
	return bcmerror;
}
#endif /* BCM_FD_AGGR */
/*
*
* Main API's used in dhd driver
*
*/
dhd_pub_t *
dhd_attach(osl_t *osh, struct dhd_bus *bus, uint bus_hdrlen)
{
	dhd_info_t *dhd = NULL;
	struct ifnet *ifp = NULL;
	dhd_attach_states_t dhd_state = DHD_ATTACH_STATE_INIT;

	DHD_TRACE(("%s(): Enter\n", __FUNCTION__));

	/* Allocate primary dhd_info */
	DHD_TRACE(("%s(): Allocate primary dhd_info \n", __FUNCTION__));
	if (!(dhd = MALLOC(osh, sizeof(dhd_info_t)))) {
		DHD_ERROR(("%s(): ERROR.. MALLOC() failed \n", __FUNCTION__));
		goto fail;
	}
	memset(dhd, 0, sizeof(dhd_info_t));
	dhd_state |= DHD_ATTACH_STATE_DHD_ALLOC;

	dhd->pub.osh = osh;
	dhd->pub.info = dhd;
	dhd->pub.bus = bus;
	dhd->pub.hdrlen = bus_hdrlen;

	/* facility initialization */
	cv_init(&dhd->ioctl_resp_wait, "ioctl_resp_wait");
	MTX_INIT(&dhd->ioctl_resp_wait_lock, "ioctl_resp_wait_lock", NULL, MTX_DEF);
	MTX_INIT(&dhd->txqlock, "txqlock", NULL, MTX_DEF);

	DHD_TRACE(("%s(): Calling dhd_handle_ifadd()\n", __FUNCTION__));
#ifdef FBSD_MUL_VIF
	if (!(ifp = dhd_handle_ifadd(dhd, 0, (char *) DHD_FBSD_NET_INTF_NAME, NULL, 0, TRUE))) {
#else
	if (!(ifp = dhd_handle_ifadd(dhd, 0, NULL, NULL, 0, TRUE))) {
#endif /* FBSD_MUL_VIF */
		DHD_ERROR(("%s(): ERROR.. dhd_handle_ifadd() failed \n", __FUNCTION__));
		goto fail;
	}

#if defined(FBSD_MUL_VIF) && defined(FBSD_ENABLE_P2P_DEV_IF)
	if (!(ifp = dhd_handle_ifadd(dhd, DHD_FBSD_P2P_DEV_IFIDX,
		(char *) DHD_FBSD_P2P_DEV_IF_NAME, NULL, DHD_FBSD_P2P_DEV_IFIDX, TRUE))) {
		DHD_ERROR(("%s(): ERROR.. dhd_handle_ifadd() %s failed \n", __FUNCTION__,
			DHD_FBSD_P2P_DEV_IF_NAME));
		goto fail;
	}
#endif /* FBSD_MUL_VIF && FBSD_ENABLE_P2P_DEV_IF */

	dhd_state |= DHD_ATTACH_STATE_ADD_IF;

	sx_init(&dhd->proto_sem, "proto_sem");
	MTX_INIT(&dhd->ioctl_lock, "ioctl_lock", NULL, MTX_DEF);
	MTX_INIT(&dhd->tx_lock, "tx_lock", NULL, MTX_DEF);

#ifdef PROP_TXSTATUS
	MTX_INIT(&dhd->wlfc_mutex, "dhdwlfc", NULL, MTX_DEF);

	dhd->pub.skip_fc = dhd_wlfc_skip_fc;
	dhd->pub.plat_init = dhd_wlfc_plat_init;
	dhd->pub.plat_deinit = dhd_wlfc_plat_deinit;

#ifdef DHD_WLFC_THREAD
	dhd->pub.wlfc_wqhead = &(dhd->pub.wlfc_thread);
	dhd->pub.wlfc_thread_go = TRUE;
	if (kthread_add(dhd_wlfc_transfer_packets, &dhd->pub, NULL, NULL, 0, 0, "wlfc-thread")) {
		DHD_ERROR(("%s(): ERROR.. kthread_add() wlfc-thread failed \n", __FUNCTION__));
		dhd->pub.wlfc_wqhead = NULL;
		dhd->pub.wlfc_thread_go = FALSE;
		goto fail;
	}
#endif /* DHD_WLFC_THREAD */
#endif /* PROP_TXSTATUS */

	dhd_state |= DHD_ATTACH_STATE_WAKELOCKS_INIT;

	/* Attach and link in the protocol */
	DHD_TRACE(("%s(): Calling dhd_prot_attach() \n", __FUNCTION__));
	if (dhd_prot_attach(&dhd->pub) != 0) {
		DHD_ERROR(("%s(): ERROR.. dhd_prot_attach() failed \n", __FUNCTION__));
		goto fail;
	}
	dhd_state |= DHD_ATTACH_STATE_PROT_ATTACH;


	/* Not creating any threads now */
	dhd_state |= DHD_ATTACH_STATE_THREADS_CREATED;

	dhd_state |= DHD_ATTACH_STATE_DONE;

	/* Save the dhd_info into the priv */
	dhd->dhd_state = dhd_state;

	DHD_TRACE(("%s(): Returns SUCCESS \n", __FUNCTION__));

	return &dhd->pub;

fail:
	DHD_ERROR(("%s(): ERROR.. Doing cleanup \n", __FUNCTION__));
	DHD_TRACE(("%s(): Calling dhd_detach() dhd_state 0x%x &dhd->pub %p \n",
		__FUNCTION__, dhd_state, &dhd->pub));
	dhd->dhd_state = dhd_state;
	dhd_detach(&dhd->pub);
	dhd_free(&dhd->pub);
	return NULL;
}

int
dhd_register_if(dhd_pub_t *dhdp, int ifidx, bool need_rtnl_lock)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;
	struct ifnet *net = NULL;
#ifdef FBSD_MUL_VIF
	dhd_if_t *ifp = NULL;
	uint8 zero_mac[ETHER_ADDR_LEN] = {0};
#endif /* FBSD_MUL_VIF */

	DHD_TRACE(("%s(): ifidx %d \n", __FUNCTION__, ifidx));

#ifdef FBSD_MUL_VIF
	ASSERT(dhd);

	ifp = dhd->iflist[ifidx];

	ASSERT(ifp);

	net = ifp->net;
#else
	ASSERT(dhd && dhd->iflist[ifidx]);

	net = dhd->iflist[ifidx]->net;
#endif /* FBSD_MUL_VIF */

	ASSERT(net);

#ifdef FBSD_MUL_VIF
	ifmedia_init(&ifp->sc_ifmedia, 0, dhd_netif_media_change, dhd_netif_media_status);
	ifmedia_add(&ifp->sc_ifmedia, IFM_ETHER|IFM_AUTO, 0, NULL);
	ifmedia_set(&ifp->sc_ifmedia, IFM_ETHER|IFM_AUTO);
#else
	ifmedia_init(&dhd->sc_ifmedia, 0, dhd_netif_media_change, dhd_netif_media_status);
	ifmedia_add(&dhd->sc_ifmedia, IFM_ETHER|IFM_AUTO, 0, NULL);
	ifmedia_set(&dhd->sc_ifmedia, IFM_ETHER|IFM_AUTO);
#endif /* FBSD_MUL_VIF */

#ifdef FBSD_MUL_VIF
#ifdef FBSD_ENABLE_P2P_DEV_IF
	if (ifidx == 0)
#endif /* FBSD_ENABLE_P2P_DEV_IF */
	{
		if_initname(net, DHD_FBSD_NET_INTF_NAME, ifidx);
	}
#ifdef FBSD_ENABLE_P2P_DEV_IF
	else if (ifidx == DHD_FBSD_P2P_DEV_IFIDX) {
		if_initname(net, DHD_FBSD_P2P_DEV_IF_NAME, IF_DUNIT_NONE);
	} else {
		if_initname(net, DHD_FBSD_NET_INTF_NAME, ifidx - 1);
	}
#endif /* FBSD_ENABLE_P2P_DEV_IF */

	strncpy(ifp->name, ifp->net->if_xname, IFNAMSIZ);
#else
	if_initname(net, DHD_FBSD_NET_INTF_NAME, device_get_unit(bwl_device));
#endif /* FBSD_MUL_VIF */

	net->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST | IFF_ALLMULTI;
	net->if_drv_flags &= ~(IFF_DRV_RUNNING | IFF_DRV_OACTIVE);
#ifdef TOE
	net->if_capabilities = IFCAP_HWCSUM;
#endif /* TOE */
	net->if_init = dhd_open;
	net->if_ioctl = dhd_ioctl_entry;
	net->if_start = dhd_start_xmit;
	net->if_mtu = 1500;

	IFQ_SET_MAXLEN(&net->if_snd, INF_QUEUE_LIST_COUNT);
	net->if_snd.ifq_drv_maxlen = INF_QUEUE_LIST_COUNT;
	IFQ_SET_READY(&net->if_snd);

	DHD_TRACE(("%s(): net=0x%x, net->if_softc=0x%x\n", __FUNCTION__,
		(uint32)net, (uint32)(net->if_softc)));

#ifdef FBSD_MUL_VIF
	if (memcmp(ifp->mac_addr, zero_mac, ETHER_ADDR_LEN) == 0) {
		memcpy(ifp->mac_addr, dhdp->mac.octet, ETHER_ADDR_LEN);
#ifdef FBSD_ENABLE_P2P_DEV_IF
		if (ifidx == DHD_FBSD_P2P_DEV_IFIDX) {
			ifp->mac_addr[0] |= 0x02;
		}
#endif /* FBSD_ENABLE_P2P_DEV_IF */
	}
#else
	memcpy(&dhd->macvalue, dhdp->mac.octet, ETHER_ADDR_LEN);
#endif /* FBSD_MUL_VIF */

	DHD_TRACE(("%s: Calling ether_ifattach() \n", __FUNCTION__));
#ifdef FBSD_MUL_VIF
	ether_ifattach(net, ifp->mac_addr);
#else
	ether_ifattach(net, dhd->macvalue.octet);
#endif /* FBSD_MUL_VIF */

#ifdef DHD_NET80211
#ifdef FBSD_MUL_VIF
	if (ifidx == 0)
#endif /* FBSD_MUL_VIF */
	{
		int ret;
		DHD_TRACE(("%s(): Calling dhd_net80211_attach() \n", __FUNCTION__));
		ret = dhd_net80211_attach(dhdp, net);
		if (ret) {
			DHD_ERROR(("%s(): ERROR.. dhd_net80211_attach() failed\n", __FUNCTION__));
		}
	}
#endif /* #ifdef DHD_NET80211 */

	return 0;

}


void
dhd_detach(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd;

	DHD_TRACE(("%s(): Enter\n", __FUNCTION__));

	if (!dhdp) {
		DHD_ERROR(("%s(): ERROR.. dhdp is NULL \n", __FUNCTION__));
		return;
	}

	dhd = (dhd_info_t *)dhdp->info;

	if (!dhd) {
		DHD_ERROR(("%s(): ERROR.. dhd is NULL \n", __FUNCTION__));
		return;
	}

	DHD_TRACE(("%s(): Enter state 0x%x \n", __FUNCTION__, dhd->dhd_state));

	if (!(dhd->dhd_state & DHD_ATTACH_STATE_DONE)) {
		/* Give some delay for threads to start running
		* in case dhd_attach() has failed
		*/
		osl_delay(1000*100);
	}

#ifdef PROP_TXSTATUS
#ifdef DHD_WLFC_THREAD
	if (dhdp->wlfc_wqhead) {
		dhdp->wlfc_thread_go = FALSE;
		wakeup(dhdp->wlfc_wqhead);
	}
#endif /* DHD_WLFC_THREAD */
#endif /* PROP_TXSTATUS */

	/* delete all interfaces, start with virtual  */
	DHD_TRACE(("%s(): Deleting all the interfaces \n", __FUNCTION__));
	if (dhd->dhd_state & DHD_ATTACH_STATE_ADD_IF) {
		int i = 1;
		dhd_if_t *ifp;

		for (i = 1; i < DHD_MAX_IFS; i++) {
			if (dhd->iflist[i]) {
				dhd->iflist[i]->state = WLC_E_IF_DEL;
				dhd->iflist[i]->idx = i;
				dhd_op_if(dhd->iflist[i]);
			}
		}

		/*  delete primary interface 0 */
		ifp = dhd->iflist[0];
		ASSERT(ifp);
		if (ifp->net) {
			dhd_stop(ifp->net);
#ifdef FBSD_MUL_VIF
			ifmedia_removeall(&ifp->sc_ifmedia);
#else
			ifmedia_removeall(&dhd->sc_ifmedia);
#endif /* FBSD_MUL_VIF */
			ether_ifdetach(ifp->net);
			if_free(ifp->net);
			MFREE(dhd->pub.osh, ifp, sizeof(*ifp));

#ifdef FBSD_MUL_VIF
			dhd->iflist[0] = NULL;
#endif /* FBSD_MUL_VIF */
		}
	}

	if (dhd->dhd_state & DHD_ATTACH_STATE_PROT_ATTACH) {
		DHD_TRACE(("%s(): Calling dhd_bus_detach() \n", __FUNCTION__));
		dhd_bus_detach(dhdp);
		if (dhdp->prot) {
			DHD_TRACE(("%s(): Calling dhd_prot_detach() \n", __FUNCTION__));
			dhd_prot_detach(dhdp);
		}
	}

	if (dhdp->dbus) {
		DHD_TRACE(("%s(): Calling dbus_detach() \n", __FUNCTION__));
		dbus_detach(dhdp->dbus);
		dhdp->dbus = NULL;
	}

#ifdef NOT_YET
	if (dhd->dhd_state & DHD_ATTACH_STATE_WAKELOCKS_INIT) {
	/* && defined(CONFIG_PM_SLEEP) */
	}
#endif

	MTX_DESTROY(&dhd->ioctl_resp_wait_lock);
	cv_destroy(&dhd->ioctl_resp_wait);

	MTX_DESTROY(&dhd->tx_lock);
	MTX_DESTROY(&dhd->ioctl_lock);
	sx_destroy(&dhd->proto_sem);
	MTX_DESTROY(&dhd->txqlock);

#ifdef PROP_TXSTATUS
	MTX_DESTROY(&dhd->wlfc_mutex);
#endif

	DHD_TRACE(("%s(): Returns SUCCESS \n", __FUNCTION__));
	return;

}


static void
dhd_bus_detach(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd;

	DHD_TRACE(("%s(): Enter\n", __FUNCTION__));

	if (dhdp) {
		dhd = (dhd_info_t *)dhdp->info;
		if (dhd) {
			if (dhd->pub.busstate != DHD_BUS_DOWN) {

				/* Stop the protocol module */
				dhd_prot_stop(&dhd->pub);

				/* Force Dongle terminated */
				DHD_ERROR(("%s send WLC_TERMINATED\n", __FUNCTION__));
				if (dhd_wl_ioctl_cmd(dhdp, WLC_TERMINATED, NULL, 0, TRUE, 0) < 0)
					DHD_ERROR(("%s Setting WLC_TERMINATED failed\n",
						__FUNCTION__));

				/* Stop the bus module */
				DHD_TRACE(("%s(): Calling dbus_stop() \n", __FUNCTION__));
				dbus_stop(dhd->pub.dbus);
				dhd->pub.busstate = DHD_BUS_DOWN;
#if defined(OOB_INTR_ONLY)
				bcmsdh_unregister_oob_intr();
#endif /* defined(OOB_INTR_ONLY) */
			}
		}
	}
}


static void
dhd_open(void *priv)
{
	dhd_info_t *dhd = (dhd_info_t *) priv;
	dhd_if_t *ifp;
	int ifidx;

	DHD_TRACE(("%s(): Started dhd = 0x%x \n", __FUNCTION__, (unsigned int)dhd));

	if (dhd == NULL) {
		DHD_ERROR(("%s(): ERROR..  dhd == NULL. Return -1 \n", __FUNCTION__));
		return;
	}

	atomic_set_int(&dhd->pend_8021x_cnt, 0);

	/* ifidx = dhd_net2idx2(dhd, net); */
	ifidx = 0;
	ifp = dhd->iflist[ifidx];

	DHD_TRACE(("%s: ifidx %d \n", __FUNCTION__, ifidx));

	if (ifp == NULL) {
		DHD_ERROR(("%s(): ifp == NULL. Return -1\n", __FUNCTION__));
		return;
	}

	if (ifp->state == WLC_E_IF_DEL) {
		DHD_ERROR(("%s(): ERROR..  IF already deleted. Return -1 \n", __FUNCTION__));
		return;
	}

	if (ifp->name[0] == '\0') {
		strncpy(ifp->name, ifp->net->if_xname, IFNAMSIZ);
	}

	/* Allow transmit calls */
	IFQ_SET_READY(&dhd->iflist[0]->net->if_snd);
	DHD_TRACE(("%s(): Seting pub.up to 1 \n", __FUNCTION__));
	dhd->pub.up = 1;

	return;
}


static int
dhd_up_down(struct ifnet *net, bool up)
{
	dhd_info_t *dhd = (dhd_info_t *)(net->if_softc);
	dhd_if_t *ifp;
	int ifidx;
	int ret = BCME_OK;

	DHD_TRACE(("%s(): Started dhd = 0x%x \n", __FUNCTION__, (unsigned int)dhd));

	if (dhd == NULL) {
		DHD_ERROR(("%s(): ERROR..  dhd == NULL. Return -1 \n", __FUNCTION__));
		ret = BCME_BADARG;
		goto done;
	}

	ifidx = dhd_net2idx(dhd, net);

	if (ifidx < 0) {
		DHD_ERROR(("%s(): ERROR..  ifidx < 0. Return -1 \n", __FUNCTION__));
		ret = BCME_BADARG;
		goto done;
	}

	DHD_TRACE(("%s: ifidx %d \n", __FUNCTION__, ifidx));

	ifp   = dhd->iflist[ifidx];

	if (ifp == NULL) {
		DHD_ERROR(("%s(): ERROR..  ifp == NULL. Return -1 \n", __FUNCTION__));
		ret = BCME_BADARG;
		goto done;
	}

	if (ifp->state == WLC_E_IF_DEL) {
		DHD_ERROR(("%s(): ERROR..  IF already deleted. Return -1 \n", __FUNCTION__));
		ret = BCME_BADARG;
		goto done;
	}

	if (ifp->name[0] == '\0') {
		strncpy(ifp->name, ifp->net->if_xname, IFNAMSIZ);
	}

	if (ifidx == 0) { /* do it only for primary eth0 */

#ifdef TOE
		if (up == TRUE) {
			uint32 toe_ol;
			if (!(dhd_toe_get(&dhd->pub, ifidx, &toe_ol))) {
				/* Get enabled TOE mode from dongle */
				if (toe_ol & TOE_TX_CSUM_OL)
					dhd->iflist[ifidx]->net->if_capenable |= IFCAP_TXCSUM;
				else
					dhd->iflist[ifidx]->net->if_capenable &= ~IFCAP_TXCSUM;
				if (toe_ol & TOE_RX_CSUM_OL)
					dhd->iflist[ifidx]->net->if_capenable |= IFCAP_RXCSUM;
				else
					dhd->iflist[ifidx]->net->if_capenable &= ~IFCAP_RXCSUM;
			}
			/* dhd->iflist[ifidx]->net->if_capenable |= IFCAP_TOE4; */
		}
#endif /* TOE */

#if defined(DHD_NET80211) || (defined(FBSD_MUL_VIF) && defined(FBSD_ENABLE_P2P_DEV_IF))
		{
			wl_ioctl_t ioc;
			memset(&ioc, 0, sizeof(ioc));
			if (up == TRUE)
				ioc.cmd = WLC_UP;
			else
				ioc.cmd = WLC_DOWN;
			ioc.set = WL_IOCTL_ACTION_SET;

			DHD_TRACE(("%s(): Calling dhd_wl_ioctl() with cmd = %d \n",
				__FUNCTION__, ioc.cmd));
			MTX_UNLOCK(&dhd->ioctl_lock);
			ret = dhd_wl_ioctl(&dhd->pub, ifidx, &ioc, NULL, 0);
			osl_delay(3000);
			MTX_LOCK(&dhd->ioctl_lock);
			if (ret != BCME_OK) {
				DHD_ERROR(("%s(): ERROR... while doing WLC_UP/WLC_DOWN \n",
					__FUNCTION__));
				goto done;
			}
		}
#endif /* #if defined(DHD_NET80211) || ((FBSD_MUL_VIF) && (FBSD_ENABLE_P2P_DEV_IF)) */

#ifdef FBSD_MUL_VIF
		if (up == TRUE)
			dhd->pub.up = 1;
#endif /* FBSD_MUL_VIF */
	}

done:
	return ret;
}

static int
dhd_stop(struct ifnet *net)
{
	dhd_info_t *dhd;

	if (net == NULL) {
		return 0;
	}

	DHD_TRACE(("%s(): Srtarted. net = 0x%x \n", __FUNCTION__, (unsigned int)net));
	dhd = (dhd_info_t *) net->if_softc;
	if (dhd->pub.up == 0) {
		return 0;
	}

	/* Set state and stop OS transmissions */
	dhd->pub.up = 0;
	net->if_drv_flags &= ~IFF_DRV_RUNNING;

#ifdef PROP_TXSTATUS
	dhd_wlfc_cleanup(&dhd->pub, NULL, 0);
#endif
	/* Stop the protocol module */
	dhd_prot_stop(&dhd->pub);
	return 0;
}

void
dhd_free(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd;

	DHD_TRACE(("%s(): Enter\n", __FUNCTION__));

	if (dhdp) {
#ifdef CACHE_FW_IMAGES
		if (dhdp->cached_fw) {
			MFREE(dhdp->osh, dhdp->cached_fw, dhdp->bus->ramsize);
			dhdp->cached_fw = NULL;
		}

		if (dhdp->cached_nvram) {
			MFREE(dhdp->osh, dhdp->cached_nvram, MAX_NVRAMBUF_SIZE);
			dhdp->cached_nvram = NULL;
		}
#endif
		dhd = (dhd_info_t *)dhdp->info;
		if (dhd) {
			MFREE(dhd->pub.osh, dhd, sizeof(*dhd));
		}
	}
	DHD_TRACE(("%s(): Returns \n", __FUNCTION__));
}

int
dhd_event_ifadd(dhd_info_t *dhd, wl_event_data_if_t *ifevent, char *name, uint8 *mac)
{
	int ifidx = ifevent->ifidx;
	int bssidx = ifevent->bssidx;
	struct net_device* ndev;

	BCM_REFERENCE(ndev);
#ifdef WL_CFG80211
	if (wl_cfg80211_notify_ifadd(ifidx, name, mac, bssidx) == BCME_OK)
		return BCME_OK;
#endif

	/* handle IF event caused by wl commands, SoftAP, WEXT and
	 * anything else
	 */
#if defined(FBSD_MUL_VIF) && defined(FBSD_ENABLE_P2P_DEV_IF)
	/* Neither primary interface nor p2p0 */
	/* Increase ifidx by 1 as p2p0 has been registered in index 1 */
	ifidx ++;

	if (ifidx >= DHD_MAX_IFS) {
		return BCME_ERROR;
	}
#endif /* FBSD_MUL_VIF && FBSD_ENABLE_P2P_DEV_IF */

	ndev = dhd_handle_ifadd(dhd, ifidx, name, mac, bssidx, TRUE);

#ifdef FBSD_MUL_VIF
	if (ifidx > 0) {
		/* primary interface will be registered else where */
		dhd_register_if(&dhd->pub, ifidx, TRUE);
	}
#endif /* FBSD_MUL_VIF */

	return BCME_OK;
}

int
dhd_event_ifdel(dhd_info_t *dhd, wl_event_data_if_t *ifevent, char *name, uint8 *mac)
{
	int ifidx = ifevent->ifidx;

#ifdef WL_CFG80211
	if (wl_cfg80211_notify_ifdel(ifidx, name, mac, ifevent->bssidx) == BCME_OK)
		return BCME_OK;
#endif

	/* handle IF event caused by wl commands, SoftAP, WEXT and
	 * anything else
	 */
#if defined(FBSD_MUL_VIF) && defined(FBSD_ENABLE_P2P_DEV_IF)
	/* Neither primary interface nor p2p0 */
	/* Increase ifidx by 1 as p2p0 has been registered in index 1 */
	ifidx ++;

	if (ifidx >= DHD_MAX_IFS) {
		return BCME_ERROR;
	}
#endif /* FBSD_MUL_VIF && FBSD_ENABLE_P2P_DEV_IF */

	dhd_handle_ifdel(dhd, ifidx, TRUE);

	return BCME_OK;
}

struct net_device*
dhd_handle_ifadd(dhd_info_t *dhd, int ifidx, char *name,
	uint8 *mac, uint8 bssidx, bool need_rtnl_lock)
{
	dhd_if_t *ifp;
	struct ifnet *ifnetp = NULL;

	DHD_TRACE(("%s(): Calling if_alloc() \n", __FUNCTION__));
	if (!(ifnetp = if_alloc(IFT_ETHER))) {
		DHD_ERROR(("%s(): ERROR.. if_alloc() failed \n", __FUNCTION__));
		return NULL;
	}
	/* Save the dhd_info into the priv */
	ifnetp->if_softc = dhd;

	DHD_TRACE(("%s(): idx %d, ifnetp->%p\n", __FUNCTION__, ifidx, ifnetp));
	ASSERT(dhd && (ifidx < DHD_MAX_IFS));

	ifp = dhd->iflist[ifidx];
	if (ifp != NULL) {
		DHD_TRACE(("%s(): ERROR.. ifp not null, stop it\n", __FUNCTION__));
		if (ifp->net != NULL) {
			ifp->net->if_drv_flags &= ~IFF_DRV_RUNNING;
			ifp->net->if_drv_flags |= IFF_DRV_OACTIVE;
			if_detach(ifp->net);
			if_free(ifp->net);
		}
	} else {
		DHD_TRACE(("%s(): ifp null, allocate it\n", __FUNCTION__));
		if ((ifp = MALLOC(dhd->pub.osh, sizeof(dhd_if_t))) == NULL) {
			DHD_ERROR(("%s(): ERROR.. MALLOC() failed \n", __FUNCTION__));
			goto FAIL;
		}
		dhd->iflist[ifidx] = ifp;
	}
	memset(ifp, 0, sizeof(dhd_if_t));
	ifp->info = dhd;
	if (name)
		strncpy(ifp->name, name, IFNAMSIZ);
	ifp->name[IFNAMSIZ] = '\0';
	if (mac != NULL) {
		DHD_TRACE(("%s()(): set the mac_addr to ifp: %x:%x:%x:%x:%x:%x\n", __FUNCTION__,
			mac[0], mac[1], mac[2],
			mac[3], mac[4], mac[5]));
		memcpy(&ifp->mac_addr, mac, ETHER_ADDR_LEN);
	}

#ifdef FBSD_MUL_VIF
	ifp->idx = ifidx;
	ifp->bssidx = bssidx;
#endif /* FBSD_MUL_VIF */


	ifp->net = ifnetp;

	return ifp->net;

FAIL:
	if (ifnetp)
		if_free(ifnetp);

	return NULL;
}

int
dhd_handle_ifdel(dhd_info_t *dhd, int ifidx, bool need_rtnl_lock)
{
	dhd_if_t *ifp;

	DHD_TRACE(("%s(): idx %d\n", __FUNCTION__, ifidx));

	ASSERT(dhd && ifidx && (ifidx < DHD_MAX_IFS));
	ifp = dhd->iflist[ifidx];
	if (!ifp) {
		DHD_ERROR(("%s(): ERROR.. Null interface\n", __FUNCTION__));
		return BCME_OK;
	}

#ifdef FBSD_MUL_VIF
	dhd->iflist[ifidx] = NULL;
#endif /* FBSD_MUL_VIF */

	ifp->state = WLC_E_IF_DEL;
	ifp->idx = ifidx;

#ifdef FBSD_MUL_VIF
	if (ifidx > 0) {
		ifmedia_removeall(&ifp->sc_ifmedia);
		ether_ifdetach(ifp->net);
		if_free(ifp->net);
		MFREE(dhd->pub.osh, ifp, sizeof(*ifp));
		dhd->iflist[ifidx] = NULL;
	}
#endif /* FBSD_MUL_VIF */

	return BCME_OK;
}

static int
dhd_ioctl_entry(struct ifnet *net, u_long cmd, caddr_t data)
{
	dhd_info_t *dhd = (dhd_info_t *)net->if_softc;
	/* dhd_ioctl_t dhd_ioc; */
	int bcmerror = BCME_OK;
	int buflen = 0;
	uint driver = 0;
	int ifidx;
	int error = 0;
	struct ifreq *ifr = (struct ifreq *) data;

	MTX_LOCK(&dhd->ioctl_lock);

	ifidx = dhd_net2idx(dhd, net);
	DHD_TRACE(("%s(): Started... ifidx %d, cmd 0x%lx \n", __FUNCTION__, ifidx, cmd));

	if (ifidx == DHD_BAD_IF) {
		MTX_UNLOCK(&dhd->ioctl_lock);
		DHD_TRACE(("%s: ERROR: ifidx %d, get DHD_BAD_IF\n", __FUNCTION__, ifidx));
		return (OSL_ERROR(BCME_NODEVICE));
	}

	/* memset(&dhd_ioc, 0, sizeof(dhd_ioctl_t)); */

	switch (cmd) {
		case SIOCSIFFLAGS:
			DHD_TRACE(("%s(): SIOCSIFFLAGS \n", __FUNCTION__));
			/* MUTEX_LOCK(sc); */
			if (net->if_flags & IFF_UP) {
				DHD_TRACE(("%s(): (if_flags & IFF_UP) = TRUE \n", __FUNCTION__));
				if ((net->if_drv_flags & IFF_DRV_RUNNING) == 0) {
					error = dhd_up_down(net, TRUE);
					net->if_drv_flags |= IFF_DRV_RUNNING;
					dhd->pub.busstate = DHD_BUS_DATA;
#ifdef PROP_TXSTATUS
					dhd_wlfc_init(&dhd->pub);
#endif
				}
			} else {
				DHD_TRACE(("%s(): (if_flags & IFF_UP) = FALSE \n", __FUNCTION__));
				if (net->if_drv_flags & IFF_DRV_RUNNING) {
					error = dhd_up_down(net, FALSE);
					net->if_drv_flags &= ~IFF_DRV_RUNNING;
				}
			}
			/* MUTEX_UNLOCK(sc); */
			break;

		case SIOCGIFMEDIA:
#ifdef FBSD_MUL_VIF
			{
				dhd_if_t *ifp = dhd->iflist[ifidx];

				if (ifp) {
					DHD_TRACE(("%s(): SIOCGIFMEDIA: ifr->ifm_count = %d \n",
						__FUNCTION__,
						((struct ifmediareq *)ifr)->ifm_count));
					error = ifmedia_ioctl(net, ifr, &ifp->sc_ifmedia, cmd);
					DHD_TRACE(("%s(): New ifr->ifm_count = %d \n",
						__FUNCTION__,
						((struct ifmediareq *)ifr)->ifm_count));
				}

				break;
			}
#else
			DHD_TRACE(("%s(): SIOCGIFMEDIA: ifr->ifm_count = %d \n",
				__FUNCTION__, ((struct ifmediareq *)ifr)->ifm_count));
			error = ifmedia_ioctl(net, ifr, &dhd->sc_ifmedia, cmd);
			DHD_TRACE(("%s(): New ifr->ifm_count = %d \n", 	__FUNCTION__,
				((struct ifmediareq *)ifr)->ifm_count));
			break;
#endif /* FBSD_MUL_VIF */

		case SIOCSIFMEDIA:
#ifdef FBSD_MUL_VIF
			{
				dhd_if_t *ifp = dhd->iflist[ifidx];

				if (ifp) {
					DHD_TRACE(("%s(): SIOCSIFMEDIA: ifr->ifm_count = %d \n",
						__FUNCTION__,
						((struct ifmediareq *)ifr)->ifm_count));
					error = ifmedia_ioctl(net, ifr, &ifp->sc_ifmedia, cmd);
					DHD_TRACE(("%s(): New ifr->ifm_count = %d \n",
						__FUNCTION__,
						((struct ifmediareq *)ifr)->ifm_count));
					break;
				}
			}
#else
			DHD_TRACE(("%s(): SIOCSIFMEDIA: ifr->ifm_count = %d \n",
				__FUNCTION__, ((struct ifmediareq *)ifr)->ifm_count));
			error = ifmedia_ioctl(net, ifr, &dhd->sc_ifmedia, cmd);
			DHD_TRACE(("%s(): New ifr->ifm_count = %d \n", __FUNCTION__,
				((struct ifmediareq *)ifr)->ifm_count));
			break;
#endif /* FBSD_MUL_VIF */

#ifdef NOT_YET
		case SIOCGIFADDR:
			DHD_TRACE(("%s(): SIOCGIFADDR \n", __FUNCTION__));
			error = ifmedia_ioctl(net, ifr, &dhd->sc_ifmedia, cmd);
			break;
#endif
		case SIOCDEVPRIVATE:
		{
			wl_ioctl_t wl_ioc;
			void *buf = NULL;

			DHD_TRACE(("%s(): SIOCDEVPRIVATE \n", __FUNCTION__));
			if (copy_from_user (&wl_ioc, ifr->ifr_data, sizeof(wl_ioctl_t))) {
				DHD_ERROR(("%s(): ERROR.. copy_from_user() error \n",
					__FUNCTION__));
				bcmerror = BCME_BADADDR;
				goto done;
			}
			if (wl_ioc.buf) {
				buflen = MIN(wl_ioc.len, DHD_IOCTL_MAXLEN);
				if (!(buf = (void *) MALLOC(dhd->pub.osh, buflen))) {
					DHD_ERROR(("%s(): ERROR.. MALLOC() failed \n",
						__FUNCTION__));
					bcmerror = BCME_NOMEM;
					goto done;
				}
				if (copy_from_user (buf, wl_ioc.buf, buflen)) {
					DHD_ERROR(("%s(): ERROR.. copy_from_user() error \n",
						__FUNCTION__));
					bcmerror = BCME_BADADDR;
					goto done;
				}
			}

			/* To differentiate between wl and dhd read 4 more byes */
			if ((copy_from_user(&driver, (char *)ifr->ifr_data + sizeof(wl_ioctl_t),
				sizeof(uint)) != 0)) {
				DHD_ERROR(("%s(): ERROR.. copy_from_user() error \n",
					__FUNCTION__));
				bcmerror = BCME_BADADDR;
				goto done;
			}


			/* check for local dhd ioctl and handle it */
			if (driver == DHD_IOCTL_MAGIC) {
				DHD_TRACE(("%s(): Calling dhd_ioctl() \n",
					__FUNCTION__));
				bcmerror = dhd_ioctl(&dhd->pub,
					(dhd_ioctl_t *)(&wl_ioc), buf, buflen);
				if (bcmerror)
					dhd->pub.bcmerror = bcmerror;
				goto done;
			}

			/*
			 * Flush the TX queue if required for proper message serialization:
			 * Intercept WLC_SET_KEY IOCTL - serialize M4 send and set key IOCTL to
			 * prevent M4 encryption and intercept WLC_DISASSOC IOCTL - serialize
			 * WPS-DONE and WLC_DISASSOC IOCTL to prevent disassoc frame being
			 * sent before WPS-DONE frame.
			 */
			if (wl_ioc.cmd == WLC_SET_KEY ||
				(wl_ioc.cmd == WLC_SET_VAR && wl_ioc.buf != NULL &&
				strncmp("wsec_key", wl_ioc.buf, 9) == 0) ||
				(wl_ioc.cmd == WLC_SET_VAR && wl_ioc.buf != NULL &&
				strncmp("bsscfg:wsec_key", wl_ioc.buf, 15) == 0) ||
				(wl_ioc.cmd == WLC_DISASSOC)) {
				MTX_UNLOCK(&dhd->ioctl_lock);
				dhd_wait_pend8021x(net);
				MTX_LOCK(&dhd->ioctl_lock);
			}

			DHD_TRACE(("%s(): Calling dhd_wl_ioctl() with cmd = %d, buflen = %d \n",
				__FUNCTION__, wl_ioc.cmd, buflen));

#if defined(FBSD_MUL_VIF) && defined(FBSD_ENABLE_P2P_DEV_IF)
			/* There is no ifidx corresponding to p2p0 in our firmware. */
			if (ifidx >= DHD_FBSD_P2P_DEV_IFIDX) {
				/* Non primary interface */
				/*
				 * Set to primary interface for p2p0
				 * or decrease ifidx by 1 for other interfaces
				 * for the index mapping of dongle dev bind.
				 */
				ifidx --;
			}
#endif /* FBSD_MUL_VIF && FBSD_ENABLE_P2P_DEV_IF */

			if ((wl_ioc.cmd == WLC_SET_VAR || wl_ioc.cmd == WLC_GET_VAR) &&
				buf != NULL && strncmp("rpc_", buf, 4) == 0) {
#ifdef BCM_FD_AGGR
				bcmerror = dhd_fdaggr_ioctl(&dhd->pub, ifidx,
					(wl_ioctl_t *)&wl_ioc, buf, buflen);
#else
				bcmerror = BCME_UNSUPPORTED;
#endif
				goto done;
			}

			MTX_UNLOCK(&dhd->ioctl_lock);
			bcmerror = dhd_wl_ioctl(&dhd->pub, ifidx,
				(wl_ioctl_t *)&wl_ioc, buf, buflen);
			MTX_LOCK(&dhd->ioctl_lock);

done:
			if (!bcmerror && buf && wl_ioc.buf) {
				if (copy_to_user(wl_ioc.buf, buf, buflen))
					bcmerror = -EFAULT;
			}

			error = OSL_ERROR(bcmerror);

			if (buf) {
				MFREE(dhd->pub.osh, buf, buflen);
			}
			break;
		}

#ifdef DHD_NET80211
		case SIOCG80211:
			DHD_TRACE(("%s(): Calling dhd_net80211_ioctl_get() \n", __FUNCTION__));
			bcmerror = dhd_net80211_ioctl_get(&dhd->pub, net, cmd, data);
			error = OSL_ERROR(bcmerror);
			break;

		case SIOCS80211:
			DHD_TRACE(("%s(): Calling dhd_net80211_ioctl_set() \n", __FUNCTION__));
			bcmerror = dhd_net80211_ioctl_set(&dhd->pub, net, cmd, data);
			error = OSL_ERROR(bcmerror);
			break;

		case SIOCG80211STATS:
			DHD_TRACE(("%s(): Calling dhd_net80211_ioctl_stats() \n", __FUNCTION__));
			bcmerror = dhd_net80211_ioctl_stats(&dhd->pub, net, cmd, data);
			error = OSL_ERROR(bcmerror);
			break;
#endif  /* #if defined(DHD_NET80211) */

		case SIOCSIFADDR:
		case SIOCGIFADDR:
		case SIOCSIFMTU:
			DHD_TRACE(("%s(): Calling ether_ioctl() \n", __FUNCTION__));
			error = ether_ioctl(net, cmd, data);
			break;

		default:
			DHD_TRACE(("%s(): Unknown Command \n", __FUNCTION__));
			error = OSL_ERROR(BCME_UNSUPPORTED);
			break;

	}

	MTX_UNLOCK(&dhd->ioctl_lock);
	return (error);
}


int
dhd_preinit_ioctls(dhd_pub_t *dhd)
{
	int ret = 0;
	char eventmask[WL_EVENTING_MASK_LEN];
	char iovbuf[WL_EVENTING_MASK_LEN + 12]; /*  Room for "event_msgs" + '\0' + bitvec  */
	char buf[WLC_IOCTL_SMLEN];

	DHD_TRACE(("%s(): Srtarted. \n", __FUNCTION__));

	/* Get the default device MAC address directly from firmware */
	memset(buf, 0, sizeof(buf));
	bcm_mkiovar("cur_etheraddr", 0, 0, buf, sizeof(buf));
	ret = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, buf, sizeof(buf), FALSE, 0);
	if (ret < 0) {
		DHD_ERROR(("%s: can't get MAC address , error=%d\n", __FUNCTION__, ret));
		return BCME_NOTUP;
	}
	/* Update public MAC address after reading from Firmware */
	memcpy(dhd->mac.octet, buf, ETHER_ADDR_LEN);

	DHD_INFO(("%s(): mac=%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", __FUNCTION__,
		dhd->mac.octet[0], dhd->mac.octet[1], dhd->mac.octet[2],
		dhd->mac.octet[3], dhd->mac.octet[4], dhd->mac.octet[5]));

	/* Read event_msgs mask */
	bcm_mkiovar("event_msgs", eventmask, WL_EVENTING_MASK_LEN, iovbuf, sizeof(iovbuf));
	ret  = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, iovbuf, sizeof(iovbuf), WL_IOCTL_ACTION_GET, 0);
	if (ret < 0) {
		DHD_ERROR(("%s(): read Event mask failed %d\n", __FUNCTION__, ret));
		goto done;
	}
	bcopy(iovbuf, eventmask, WL_EVENTING_MASK_LEN);

	/* Setup event_msgs */
	setbit(eventmask, WLC_E_SET_SSID);
	setbit(eventmask, WLC_E_PRUNE);
	setbit(eventmask, WLC_E_AUTH);
	setbit(eventmask, WLC_E_REASSOC);
	setbit(eventmask, WLC_E_REASSOC_IND);
	setbit(eventmask, WLC_E_DEAUTH);
	setbit(eventmask, WLC_E_DEAUTH_IND);
	setbit(eventmask, WLC_E_DISASSOC_IND);
	setbit(eventmask, WLC_E_DISASSOC);
	setbit(eventmask, WLC_E_JOIN);
	setbit(eventmask, WLC_E_ASSOC_IND);
	setbit(eventmask, WLC_E_PSK_SUP);
	setbit(eventmask, WLC_E_LINK);
	setbit(eventmask, WLC_E_MIC_ERROR);
	setbit(eventmask, WLC_E_ASSOC_REQ_IE);
	setbit(eventmask, WLC_E_ASSOC_RESP_IE);
	setbit(eventmask, WLC_E_PMKID_CACHE);
	setbit(eventmask, WLC_E_TXFAIL);
	setbit(eventmask, WLC_E_JOIN_START);
	setbit(eventmask, WLC_E_SCAN_COMPLETE);
#ifdef WLMEDIA_HTSF
	setbit(eventmask, WLC_E_HTSFSYNC);
#endif /* WLMEDIA_HTSF */
#ifdef PNO_SUPPORT
	setbit(eventmask, WLC_E_PFN_NET_FOUND);
#endif /* PNO_SUPPORT */
	/* enable dongle roaming event */

#if defined(DHD_NET80211) || (defined(FBSD_MUL_VIF) && defined(FBSD_ENABLE_P2P_DEV_IF))
	setbit(eventmask, WLC_E_ESCAN_RESULT);
	if ((dhd->op_mode & WFD_MASK) == WFD_MASK) {
		setbit(eventmask, WLC_E_ACTION_FRAME_RX);
		setbit(eventmask, WLC_E_ACTION_FRAME_COMPLETE);
		setbit(eventmask, WLC_E_ACTION_FRAME_OFF_CHAN_COMPLETE);
		setbit(eventmask, WLC_E_P2P_PROBREQ_MSG);
		setbit(eventmask, WLC_E_P2P_DISC_LISTEN_COMPLETE);
	}
#endif /* (DHD_NET80211) || ((FBSD_MUL_VIF) && (FBSD_ENABLE_P2P_DEV_IF)) */

	/* Write updated Event mask */
	bcm_mkiovar("event_msgs", eventmask, WL_EVENTING_MASK_LEN, iovbuf, sizeof(iovbuf));
	ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), WL_IOCTL_ACTION_SET, 0);
	if (ret < 0) {
		DHD_ERROR(("%s(): Set Event mask failed %d\n", __FUNCTION__, ret));
		goto done;
	}

done:
	return ret;
}


static void
dhd_start_xmit(struct ifnet *net)
{
	dhd_info_t *dhd = net->if_softc;
	struct mbuf *m;
	int ifidx;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	MTX_LOCK(&dhd->tx_lock);

	if (!dhd->pub.up || (dhd->pub.busstate == DHD_BUS_DOWN)) {
		DHD_ERROR(("%s(): xmit rejected pub.up=%d busstate=%d \n",
			__FUNCTION__, dhd->pub.up, dhd->pub.busstate));
		goto xmit_done;
	}

	/* Net if status check  */
	if ((net->if_flags & IFF_UP) == 0) {
		DHD_ERROR(("%s(): Interface not up !!\n", __FUNCTION__));
		goto xmit_done;
	}

	ifidx = dhd_net2idx(dhd, net);
	if (ifidx == DHD_BAD_IF) {
		DHD_ERROR(("%s(): Bad Interface. ifidx = 0x%x !! \n", __FUNCTION__, ifidx));
		goto xmit_done;
	}


	if (!(net->if_drv_flags &IFF_DRV_RUNNING)) {
		goto xmit_done;
	}

	while (1) {
		if (net->if_drv_flags & IFF_DRV_OACTIVE) {
			/* DHD_ERROR(("%s(): flow control is on !! \n", __FUNCTION__)); */
			break;
		}
		IFQ_DRV_DEQUEUE(&net->if_snd, m);
		if (m == NULL) {
			/* DHD_ERROR(("%s(): ERROR.. m is NULL !! \n", __FUNCTION__)); */
			break;
		}

		bwl_tx_data(&dhd->pub, ifidx, m);
	}


xmit_done:
	dhd->pub.dstats.tx_packets++;
	dhd->pub.tx_packets++;

	MTX_UNLOCK(&dhd->tx_lock);

	DHD_TRACE(("%s(): finished \n", __FUNCTION__));

	return;
}


static void
dhd_dbus_disconnect_cb(void *arg)
{
	dhd_info_t *dhd = (dhd_info_t *)arg;
	dhd_pub_t *pub;
	osl_t *osh;

	DHD_TRACE(("%s(): Enter\n", __FUNCTION__));

	if (dhd == NULL) {
		DHD_ERROR(("%s(): ERROR.. arg is NULL. Returning \n", __FUNCTION__));
		return;
	}

	pub = &dhd->pub;
	osh = pub->osh;

	callout_stop(&dhd->txfc_timer);

#ifdef BCM_FD_AGGR
	callout_stop(&dhd->rpcth_timer);
	bcm_rpc_tp_deregister_cb(dhd->rpc_th);
	rpc_osl_detach(dhd->rpc_osh);
	bcm_rpc_tp_detach(dhd->rpc_th);
#endif

#ifdef DHD_NET80211
	DHD_TRACE(("%s(): Calling dhd_net80211_detach() \n", __FUNCTION__));
	dhd_net80211_detach(pub);
#endif

	DHD_TRACE(("%s(): Calling dhd_detach() \n", __FUNCTION__));
	dhd_detach(pub);
	DHD_TRACE(("%s(): Calling dhd_free() \n", __FUNCTION__));
	dhd_free(pub);

#ifdef NOT_YET
	if (MALLOCED(osh)) {
		DHD_ERROR(("%s(): MEMORY LEAK %d bytes\n", __FUNCTION__, MALLOCED(osh)));
	}
#endif
	DHD_TRACE(("%s(): Calling osl_detach() \n", __FUNCTION__));
	osl_detach(osh);

	DHD_TRACE(("%s(): Returns \n", __FUNCTION__));
	return;

}


static void *
dhd_dbus_probe_cb(void *arg, const char *desc, uint32 bustype, uint32 hdrlen)
{
	osl_t *osh = NULL;
	dbus_attrib_t attrib;
	dhd_pub_t *pub = NULL;
#ifdef BCM_FD_AGGR
	struct timeval tv;
	dbus_config_t config;
	uint32 agglimit = 0;
	uint32 rpc_agg = BCM_RPC_TP_DNGL_AGG_DPC; /* host aggr not enabled yet */
#endif
	int ret;

	DHD_TRACE(("%s(): Enter\n", __FUNCTION__));

	/* Ask the OS interface part for an OSL handle */
	DHD_TRACE(("%s(): Calling dhd_osl_attach() \n", __FUNCTION__));
	osh = osl_attach(NULL, "dhd", 0, 0, NULL);
	if (!osh) {
		DHD_ERROR(("%s(): ERROR.. osl_attach() failed\n", __FUNCTION__));
		goto fail;
	}

	/* Attach to the dhd/OS interface */
	DHD_TRACE(("%s(): Calling dhd_attach() \n", __FUNCTION__));
	pub = dhd_attach(osh, NULL /* bus */, hdrlen);
	if (!pub) {
		DHD_ERROR(("%s(): ERROR.. dhd_attach() failed\n", __FUNCTION__));
		goto fail;
	}

	/* TODO: Set the NRXQ to 1, increase it later */
	if (pub->rxsz == 0)
		pub->rxsz = BWL_RX_BUFFER_SIZE;

	DHD_TRACE(("%s(): Calling dbus_attach() \n", __FUNCTION__));
	pub->dbus = dbus_attach(osh, pub->rxsz, BWL_RX_LIST_COUNT, BWL_TX_LIST_COUNT,
		pub->info, &dhd_dbus_cbs, NULL, NULL);
	if (pub->dbus) {
		DHD_TRACE(("%s(): Calling dbus_get_attrib() \n", __FUNCTION__));
		dbus_get_attrib(pub->dbus, &attrib);
		DHD_INFO(("%s(): DBUS: vid=0x%x pid=0x%x devid=0x%x bustype=0x%x mtu=%d \n",
			__FUNCTION__, attrib.vid, attrib.pid, attrib.devid,
			attrib.bustype, attrib.mtu));
	} else {
		DHD_ERROR(("%s(): dbus_attach failed \n", __FUNCTION__));
		goto fail;
	}

	if (pub->busstate != DHD_BUS_DATA) {
		DHD_TRACE(("%s(): Calling dbus_up() \n", __FUNCTION__));
		ret = dbus_up(pub->dbus);
		if (ret != BCME_OK) {
			DHD_ERROR(("%s(): ERROR..  dbus_up() return %d \n", __FUNCTION__, ret));
			goto fail;
		}
		pub->busstate = DHD_BUS_DATA;
	}

	/* Bus is ready, query any dongle information */
	DHD_TRACE(("%s(): Calling dhd_sync_with_dongle() \n", __FUNCTION__));
	ret = dhd_sync_with_dongle(pub);
	if (ret != BCME_OK) {
		DHD_ERROR(("%s(): ERROR. dhd_sync_with_dongle() return %d \n", __FUNCTION__, ret));
		goto fail;
	}

	callout_init(&pub->info->txfc_timer, CALLOUT_MPSAFE);

#ifdef BCM_FD_AGGR
	pub->info->rpc_th = bcm_rpc_tp_attach(osh, (void *)pub->dbus);
	if (!pub->info->rpc_th) {
		DHD_ERROR(("%s: bcm_rpc_tp_attach failed\n", __FUNCTION__));
		ret = -ENXIO;
		goto fail;
	}

	pub->info->rpc_osh = rpc_osl_attach(osh);
	if (!pub->info->rpc_osh) {
		DHD_ERROR(("%s: rpc_osl_attach failed\n", __FUNCTION__));
		bcm_rpc_tp_detach(pub->info->rpc_th);
		pub->info->rpc_th = NULL;
		ret = -ENXIO;
		goto fail;
	}
	/* Set up the aggregation release timer */
	tv.tv_sec = 0;
	tv.tv_usec = 1000 * BCM_RPC_TP_HOST_TMOUT;
	pub->info->rpcth_hz = tvtohz(&tv);
	callout_init(&pub->info->rpcth_timer, CALLOUT_MPSAFE);
	callout_reset(&pub->info->rpcth_timer, pub->info->rpcth_hz,
		dhd_rpcth_watchdog, (void *)pub->info);
	pub->info->rpcth_timer_active = FALSE;

	bcm_rpc_tp_register_cb(pub->info->rpc_th, NULL, pub->info,
		dbus_rpcth_rx_pkt, pub->info, pub->info->rpc_osh);

	config.config_id = DBUS_CONFIG_ID_AGGR_LIMIT;


	if (rpc_agg & BCM_RPC_TP_DNGL_AGG_DPC) {
		ret = dhd_wl_ioctl_get_intiovar(pub, "rpc_dngl_agglimit",
			&agglimit, WLC_GET_VAR, FALSE, 0);
		if (!ret) {
			config.aggr_param.maxrxsf = agglimit >> BCM_RPC_TP_AGG_SF_SHIFT;
			config.aggr_param.maxrxsize = agglimit & BCM_RPC_TP_AGG_BYTES_MASK;
			if (config.aggr_param.maxrxsize > BWL_RX_BUFFER_SIZE) {
				ASSERT(0);
				DHD_ERROR(("%s(): ERROR.. rx buffer size limit too small\n",
					__FUNCTION__));
				goto fail;
			}
			DHD_ERROR(("rpc_dngl_agglimit %x : sf_limit %d bytes_limit %d\n",
				agglimit, config.aggr_param.maxrxsf, config.aggr_param.maxrxsize));
			if ((ret = bcm_rpc_tp_set_config(pub->info->rpc_th, &config))) {
				DHD_ERROR(("set tx/rx queue size and buffersize failed\n"));
			}
		}
		if (ret) {
			rpc_agg &= ~BCM_RPC_TP_DNGL_AGG_MASK;
		}
	}

	/* Set aggregation for TX */
	bcm_rpc_tp_agg_set(pub->info->rpc_th, rpc_agg & BCM_RPC_TP_HOST_AGG_MASK,
		rpc_agg & BCM_RPC_TP_HOST_AGG_MASK);

	/* Set aggregation for RX */
	if (!dhd_wl_ioctl_set_intiovar(pub, "rpc_agg", rpc_agg, WLC_SET_VAR, TRUE, 0)) {
		pub->info->fdaggr = 0;
		if (rpc_agg & BCM_RPC_TP_HOST_AGG_MASK)
			pub->info->fdaggr |= BCM_FDAGGR_H2D_ENABLED;
		if (rpc_agg & BCM_RPC_TP_DNGL_AGG_MASK)
			pub->info->fdaggr |= BCM_FDAGGR_D2H_ENABLED;
	}
#endif /* BCM_FD_AGGR */

#ifdef TOE
	if (dhd_toe_set(pub, 0, TOE_TX_CSUM_OL | TOE_RX_CSUM_OL) < 0) {
		DHD_ERROR(("%s(): ERROR.. enable rx checksum offload failed\n", __FUNCTION__));
		goto fail;
	}
#endif /* TOE */

	/* Attach to the OS network interface */
	DHD_TRACE(("%s(): Calling dhd_register_if() \n", __FUNCTION__));
	ret = dhd_register_if(pub, 0, TRUE);
	if (ret) {
		DHD_ERROR(("%s(): ERROR.. dhd_register_if() failed\n", __FUNCTION__));
		goto fail;
	}

#if defined(FBSD_MUL_VIF) && defined(FBSD_ENABLE_P2P_DEV_IF)
	ret = dhd_register_if(pub, DHD_FBSD_P2P_DEV_IFIDX, TRUE);
	if (ret) {
		DHD_ERROR(("%s(): ERROR.. dhd_register_if() %s failed\n", __FUNCTION__,
			DHD_FBSD_P2P_DEV_IF_NAME));
		goto fail;
	}
#endif /* FBSD_MUL_VIF && FBSD_ENABLE_P2P_DEV_IF */

	DHD_TRACE(("%s(): Returns SUCCESS \n", __FUNCTION__));
	/* This is passed to dhd_dbus_disconnect_cb */
	return (pub->info);

fail:
	DHD_ERROR(("%s(): ERROR.. Doing cleanup \n", __FUNCTION__));
	/* Release resources in reverse order */
	if (osh) {
		if (pub) {
			dhd_detach(pub);
			dhd_free(pub);
		}
		/* dhd_osl_detach(osh); */
	}
	DHD_ERROR(("%s(): ERROR.. cleanup done\n", __FUNCTION__));
	return NULL;
}

void
dhd_module_cleanup(void)
{
	DHD_TRACE(("%s(): Enter\n", __FUNCTION__));
	dbus_deregister();
	DHD_TRACE(("%s(): Exit \n", __FUNCTION__));
	return;
}

void
dhd_module_init(void)
{
	int error;

	DHD_TRACE(("%s(): Enter\n", __FUNCTION__));

	TUNABLE_INT("dhd_vid", &dhd_vid);
	TUNABLE_INT("dhd_pid", &dhd_pid);

	error = dbus_register(dhd_vid, dhd_pid, dhd_dbus_probe_cb,
		dhd_dbus_disconnect_cb, NULL /* arg */, NULL, NULL);
	if (error == DBUS_OK) {
		printf("%s \n", dhd_version);
	}
	else {
		printf("%s(): ERROR..  dbus_register() returns 0x%x \n", __FUNCTION__, error);
	}
	DHD_TRACE(("%s(): Exit \n", __FUNCTION__));
	return;
}
