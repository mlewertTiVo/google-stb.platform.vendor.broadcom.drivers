/*
 * Linux-specific portion of
 * Broadcom 802.11abg Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom. All Rights Reserved.
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
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: wl_linux.c 657844 2016-09-02 20:52:35Z $
 */


#define LINUX_PORT

#define __UNDEF_NO_VERSION__

#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#include <linux/module.h>
#endif

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/ethtool.h>
#include <linux/completion.h>
#include <linux/random.h>
#include <linux/usb.h>
#include <bcmdevs.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
#include <asm/switch_to.h>
#else
#include <asm/system.h>
#endif
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>
#include <asm/unaligned.h>

#if !defined(BCMDONGLEHOST)
#include <wlc_cfg.h>
#endif

#include <proto/802.1d.h>

#include <epivers.h>
#include <bcmendian.h>
#include <proto/ethernet.h>
#include <bcmutils.h>
#include <pcicfg.h>
#include <wlioctl.h>
#include <wlc_keymgmt.h>

#if defined(OEM_ANDROID)
#include <wl_android.h>
#endif /* OEM_ANDROID */

#ifndef LINUX_POSTMOGRIFY_REMOVAL
#include <wlc_channel.h>
#else
typedef const struct si_pub si_t;
#endif
#include <wlc_pub.h>
#include <wlc.h>
#ifndef LINUX_POSTMOGRIFY_REMOVAL
#include <wlc_bsscfg.h>
#endif


#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 4, 5)
#error "No support for Kernel Rev <= 2.4.5, As the older kernel revs doesn't support Tasklets"
#endif

#include <wl_dbg.h>
#include <proto/monitor.h>
#ifdef WL_MONITOR
#include <bcmwifi_monitor.h>
#include <bcmwifi_radiotap.h>
#include <wlc_ethereal.h>
#endif
#ifdef WL_STA_MONITOR
#include <wlc_stamon.h>
#endif /* WL_STA_MONITOR */


#include <wl_iw.h>
#ifdef USE_IW
struct iw_statistics *wl_get_wireless_stats(struct net_device *dev);
#endif

#include <wl_export.h>
#ifdef TOE
#include <wl_toe.h>
#endif
#ifdef ARPOE
#include <wl_arpoe.h>
#endif

#ifndef LINUX_POSTMOGRIFY_REMOVAL
#ifdef WL_EVENTQ
#include <wl_eventq.h>
#endif
#if defined(P2PO) || defined(ANQPO)
#include <wl_gas.h>
#endif
#ifdef P2PO
#include <wl_p2po_disc.h>
#include <wl_p2po.h>
#endif
#ifdef ANQPO
#include <wl_anqpo.h>
#endif
#ifdef BDO
#include <wl_bdo.h>
#endif
#ifdef TKO
#include <wl_tko.h>
#endif
#endif /* LINUX_POSTMOGRIFY_REMOVAL */

#ifdef BCMDBUS
#include "dbus.h"
/* BMAC_NOTES: Remove, but just in case your Linux system has this defined */
#undef CONFIG_PCI
#endif

#ifdef BCMPCMCIA
/*
 * If you compile kernel PCMCIA support as a module, and you don't
 * have a controller, PCMCIA support will refuse to load, and any
 * driver that depends on it will be unable to load. The BCMPCMCIA
 * flag allows us to force off PCMCIA support.
 */
#if defined(CONFIG_PCMCIA)
#include <sbpcmcia.h>
#define WL_PCMCIA	1
#endif
#endif /* BCMPCMCIA */

#ifdef HNDCTF
#include <ctf/hndctf.h>
#endif


#ifdef BCM7271
#ifdef PLATFORM_INTEGRATED_WIFI
#include <linux/platform_device.h>
#include <linux/of.h>
#include <bcmnvram.h>
#else /* PLATFORM_INTEGRATED_WIFI */
#include <wl_linux_bcm7xxx.h>
#endif /* PLATFORM_INTEGRATED_WIFI */
#undef CONFIG_PCI
#endif /* BCM7271 */

#include <wl_linux.h>

#ifdef WL_THREAD
#include <linux/kthread.h>
#endif /* WL_THREAD */

#if defined(USE_CFG80211)
#include <wl_cfg80211.h>
#if !defined(BCMDONGLEHOST)
#include <net/rtnetlink.h>
#include <wldev_common.h>
#endif /* !BCMDONGLEHOST */
#endif /* CFG80211 */

#ifdef DPSTA
#include <dpsta.h>
#if defined(STA) && defined(DWDS)
#include <wlc_wds.h>
#endif /* STA && DWDS */
#ifdef PSTA
#include <wlc_psta.h>
#endif /* PSTA */
#endif /* DPSTA */

#ifdef WLCSO
#include <wlc_tso.h>
#endif /* WL_CSO */


#include <wlc_objregistry.h>
#include <wlc_perf_utils.h>
#include <wlc_dump.h>
#include <wlc_iocv.h>

#ifdef WLCXO_CTRL
#include <wlc_cxo_types.h>
#include <wlc_cxo_ipc.h>
#include <linux/kthread.h>
#endif

#ifdef WLCXO_FULL
#include "wl_cxo.h"
#endif

#ifdef WLRSDB
#include <wlc_rsdb.h>
#endif /* WLRSDB */

static void wl_timer(ulong data);
static void _wl_timer(wl_timer_t *t);
static struct net_device *wl_alloc_linux_if(wl_if_t *wlif);

#ifdef WL_MONITOR
static int wl_monitor_start(struct sk_buff *skb, struct net_device *dev);
#endif

static void wl_start_txqwork(wl_task_t *task);
static void wl_txq_free(wl_info_t *wl);
#define TXQ_LOCK(_wl) spin_lock_bh(&(_wl)->txq_lock)
#define TXQ_UNLOCK(_wl) spin_unlock_bh(&(_wl)->txq_lock)

#ifdef WL_ALL_PASSIVE
static void wl_set_multicast_list_workitem(struct work_struct *work);

static void wl_timer_task(wl_task_t *task);
static void wl_dpc_rxwork(struct wl_task *task);
#endif /* WL_ALL_PASSIVE */

static void wl_linux_watchdog(void *ctx);
static int wl_found = 0;
#if defined(CONFIG_WL_MODULE) && defined(CONFIG_BCM47XX) && defined(WLRSDB)
static int rsdb_found = 0;
#endif /* CONFIG_WL_MODULE && CONFIG_BCM47XX && WLRSDB */

#ifdef WLCXO_IPC
static void wl_cxo_ctrl_ipc_task(wl_task_t *task);
#endif

#ifdef LINUX_CRYPTO
struct ieee80211_tkip_data {
#define TKIP_KEY_LEN 32
	u8 key[TKIP_KEY_LEN];
	int key_set;

	u32 tx_iv32;
	u16 tx_iv16;
	u16 tx_ttak[5];
	int tx_phase1_done;

	u32 rx_iv32;
	u16 rx_iv16;
	u16 rx_ttak[5];
	int rx_phase1_done;
	u32 rx_iv32_new;
	u16 rx_iv16_new;

	u32 dot11RSNAStatsTKIPReplays;
	u32 dot11RSNAStatsTKIPICVErrors;
	u32 dot11RSNAStatsTKIPLocalMICFailures;

	int key_idx;

	struct crypto_tfm *tfm_arc4;
	struct crypto_tfm *tfm_michael;

	/* scratch buffers for virt_to_page() (crypto API) */
	u8 rx_hdr[16], tx_hdr[16];
};
#endif /* LINUX_CRYPTO */

/* This becomes netdev->priv and is the link between netdev and wlif struct */
typedef struct priv_link {
	wl_if_t *wlif;
} priv_link_t;

#define WL_DEV_IF(dev)          ((wl_if_t*)((priv_link_t*)DEV_PRIV(dev))->wlif)

#define WL_INFO_GET(dev)	((wl_info_t*)(WL_DEV_IF(dev)->wl))	/* dev to wl_info_t */


/* local prototypes */
static int wl_open(struct net_device *dev);
static int wl_close(struct net_device *dev);
#ifdef WL_THREAD
static int wl_start_wlthread(struct sk_buff *skb, struct net_device *dev);
#else
static int BCMFASTPATH wl_start(struct sk_buff *skb, struct net_device *dev);
#endif
static int wl_start_int(wl_info_t *wl, wl_if_t *wlif, struct sk_buff *skb);

static struct net_device_stats *wl_get_stats(struct net_device *dev);
static int wl_set_mac_address(struct net_device *dev, void *addr);
static void wl_set_multicast_list(struct net_device *dev);
static void _wl_set_multicast_list(struct net_device *dev);
static int wl_ethtool(wl_info_t *wl, void *uaddr, wl_if_t *wlif);
#ifdef NAPI_POLL
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
static int wl_poll(struct napi_struct *napi, int budget);
#else
static int wl_poll(struct net_device *dev, int *budget);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30) */
#else /* NAPI_POLL */
static void wl_dpc(ulong data);
#endif /* NAPI_POLL */
static void wl_tx_tasklet(ulong data);
static void wl_link_up(wl_info_t *wl, char * ifname);
static void wl_link_down(wl_info_t *wl, char *ifname);
#if defined(BCMSUP_PSK) && defined(STA)
static void wl_mic_error(wl_info_t *wl, wlc_bsscfg_t *cfg,
	struct ether_addr *ea, bool group, bool flush_txq);
#endif
#if defined(AP) || defined(WL_MONITOR)
static int wl_schedule_task(wl_info_t *wl, void (*fn)(struct wl_task *), void *context);
#endif
#ifdef WL_THREAD
static int wl_start_enqueue_wlthread(wl_info_t *wl, struct sk_buff *skb);
#endif
#if defined(CONFIG_PROC_FS)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static int wl_read_proc(struct seq_file *m, void *v);
#else /* Kernel >= 3.10.0 */
static int wl_read_proc(char *buffer, char **start, off_t offset, int length, int *eof, void *data);
#endif /* Kernel >= 3.10.0 */
#endif /* #if defined(CONFIG_PROC_FS) */
static struct wl_if *wl_alloc_if(wl_info_t *wl, int iftype, uint unit, struct wlc_if* wlc_if);
static void wl_free_if(wl_info_t *wl, wl_if_t *wlif);
static void wl_get_driver_info(struct net_device *dev, struct ethtool_drvinfo *info);
#ifdef WLCSO
static int wl_set_tx_csum(struct net_device *dev, uint32 on_off);
#endif

#if defined(WL_CONFIG_RFKILL)
#include <linux/rfkill.h>
static int wl_init_rfkill(wl_info_t *wl);
static void wl_uninit_rfkill(wl_info_t *wl);
static int wl_set_radio_block(void *data, bool blocked);
static void wl_report_radio_state(wl_info_t *wl);
#endif

#ifdef WL_PCMCIA

static void wl_cs_config(dev_link_t *link);
static void wl_cs_release(u_long arg);
static int wl_cs_event(event_t event, int priority, event_callback_args_t *args);
static dev_link_t *wl_cs_attach(void);
static void wl_cs_detach(dev_link_t *link);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)

static struct pcmcia_driver wl_driver = {
	.drv = { .name = "wl" },
	.attach = wl_cs_attach,
	.detach = wl_cs_detach,
	.owner = THIS_MODULE
};

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0) */

/*
 * The dev_info variable is the "key" that is used to match up this
 * device driver with appropriate cards, through the card
 * configuration database.
 */
static dev_info_t dev_info = "wl";

/* global linked list of allocated devices */
static dev_link_t *dev_list = NULL;

#endif /* WL_PCMCIA */



#ifdef PLATFORM_INTEGRATED_WIFI
/*
 * Platform driver type used here for integrated WIFI connected with an internal bus.
 */
static int wl_plat_drv_probe(struct platform_device *pdev);
static int wl_plat_drv_remove(struct platform_device *pdev);
static void wl_plat_drv_shutdown(struct platform_device *pdev);
static int wl_plat_drv_suspend(struct platform_device *pdev, pm_message_t state);
static int wl_plat_drv_resume(struct platform_device *pdev);
#endif /* PLATFORM_INTEGRATED_WIFI */

#ifdef STB_SOC_WIFI
static void wl_soc_wifi_enable_intrs(wl_info_t *wl);
static void wl_soc_wifi_disable_intrs(wl_info_t *wl);
static void wl_soc_wifi_d2h_isr(wl_info_t *wl);
static void wl_soc_wifi_d2h_intstatus(wl_info_t *wl);
#endif /* STB_SOC_WIFI */

#if defined(PLATFORM_INTEGRATED_WIFI) /* && defined(SPLITNIC) */
MODULE_LICENSE("GPL");
#else /* PLATFORM_INTEGRATED_WIFI */
MODULE_LICENSE("Proprietary");
#endif /* PLATFORM_INTEGRATED_WIFI */

#if defined(BCM7271) && !defined(PLATFORM_INTEGRATED_WIFI)
wl_info_t *
wl_attach(uint16 vendor, uint16 device, ulong regs,
		uint bustype, void *btparam, uint irq, uchar* bar1_addr, uint32 bar1_size,
		uchar* bar2_addr, uint32 bar2_size, void *cmndata);
#endif /* BCM7271 && !PLATFORM_INTEGRATED_WIFI */

struct wl_cmn_data {
	void *objrptr;
	void *oshcmnptr;
	si_t *sih;
	uint16 device;
};

#if defined(CONFIG_PCI)
static struct wl_cmn_data wlcmndata;
/* recognized PCI IDs */
static struct pci_device_id wl_id_table[] =
{
	{ PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID,
	PCI_CLASS_NETWORK_OTHER << 8, 0xffff00, (unsigned long)&wlcmndata},
	{ 0, 0, 0, 0, 0, 0, 0}
};

MODULE_DEVICE_TABLE(pci, wl_id_table);
#endif 




#if defined(BCM7271)

/* force to use WL_ALL_PASSIVE and txworkq */
static int passivemode = 1;
module_param(passivemode, int, 0);
static int txworkq = 1;
module_param(txworkq, int, 0);

#else /* !defined(BCM7271) */

#if defined(WL_ALL_PASSIVE)
/* WL Passive Mode: Enable(1)/Disable(0) */
#ifdef WLP2P
static int passivemode = 1;
module_param(passivemode, int, 1);
#else /* WLP2P */
static int passivemode = 0;
module_param(passivemode, int, 0);
#endif /* WLP2P */
#else
static int passivemode = 0;
module_param(passivemode, int, 0);
#endif /* WLP2P */

static int txworkq = 0;
module_param(txworkq, int, 0);

#endif /* defined(BCM7271) */

#ifdef WLCXO_SIM
#define WL_TXHPKTQ_THRESH 512
static int wl_txhpktq_thresh = WL_TXHPKTQ_THRESH;
#endif

#ifdef WLCXO_CTRL
#define WL_TXQ_THRESH	512
#else
#define WL_TXQ_THRESH	0
#endif
static int wl_txq_thresh = WL_TXQ_THRESH;
module_param(wl_txq_thresh, int, 0);

static int oneonly = 0;
module_param(oneonly, int, 0);

static int piomode = 0;
module_param(piomode, int, 0);

static int instance_base = 0; /* Starting instance number */
module_param(instance_base, int, 0);

#ifdef DPSTA
#if defined(STA) && defined(DWDS)
static void wl_dpsta_dwds_register(wl_info_t *wl, wlc_bsscfg_t *cfg);
#endif /* STA && DWDS */
#ifdef PSTA
static void wl_dpsta_psta_register(wl_info_t *wl, wlc_bsscfg_t *cfg);
#endif
#endif /* DPSTA */

#if defined(BCM7271)
static int nompc = 1;
#else
static int nompc = 0;
#endif
module_param(nompc, int, 0);

#ifdef quote_str
#undef quote_str
#endif /* quote_str */
#ifdef to_str
#undef to_str
#endif /* quote_str */
#define to_str(s) #s
#define quote_str(s) to_str(s)

#ifndef BRCM_WLAN_IFNAME
#define BRCM_WLAN_IFNAME eth%d
#endif

#ifdef WLCXO_IPC
static int wl_swq_thread(void *data);
static struct swq_struct *wl_swq_create(wl_info_t *wl, uint8 *name, uint32 cpu);
static void wl_swq_destroy(wl_info_t *wl);
static int32 wl_swq_add_work(struct swq_struct *swq, struct work_struct *work);
#define SCHEDULE_WORK(wl, work) \
({ \
	int32 error; \
	if (WLCXO_ENAB(wl->pub)) \
		error = wl_swq_add_work((wl)->cxo_ctrl_wq, (work)); \
	else \
		error = schedule_work(work); \
	error; \
})
#define SCHEDULE_WORK_ON(wl, cpu, work) \
({ \
	int32 error; \
	if (WLCXO_ENAB(wl->pub)) \
		error = wl_swq_add_work((wl)->cxo_ctrl_wq, (work)); \
	else \
		error = schedule_work_on((cpu), (work)); \
	error; \
})
#else
#define SCHEDULE_WORK(wl, work)		schedule_work(work)
#define SCHEDULE_WORK_ON(wl, cpu, work)	schedule_work_on((cpu), (work))
#endif /* WLCXO_IPC */
static char intf_name[IFNAMSIZ] = quote_str(BRCM_WLAN_IFNAME);

/* allow override of default wlan interface name at insmod time */
module_param_string(intf_name, intf_name, IFNAMSIZ, 0);

/* BCMSLTGT: slow target */
#ifdef BCM7271
#else /* !BCM7271 */

#endif /* BCM7271 */

/*
 * Before CT DMA is mature enough, here ctdma is off by default, but you can pass
 * module parameter "ctdma=1" during  driver installation to turn on it.
 */
#if defined(BCM_DMA_CT) && !defined(BCM_DMA_CT_DISABLED)
static int ctdma = 0;
module_param(ctdma, int, 0);
#endif

#if defined(USE_CFG80211) && !defined(BCMDONGLEHOST)
int passive_channel_skip = 0;
module_param(passive_channel_skip, int, (S_IRUSR|S_IWUSR));
#endif /* defined(USE_CFG80211) && !defined(BCMDONGLEHOST) */

#ifndef SRCBASE
#define SRCBASE "."
#endif

#define FOREACH_WL(wl_cmn, idx, current_wl) \
	for ((idx) = 0; (int) (idx) < MAX_RSDB_MAC_NUM; (idx)++) \
		if ((((current_wl) = wl_cmn->wl[(idx)]) != NULL))

#if WIRELESS_EXT >= 19 || LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
static struct ethtool_ops wl_ethtool_ops =
#else
static const struct ethtool_ops wl_ethtool_ops =
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19) */
{
	.get_drvinfo = wl_get_driver_info,
#ifdef WLCSO
	.set_tx_csum = wl_set_tx_csum
#endif
};
#endif /* WIRELESS_EXT >= 19 || LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29) */

#ifdef WL_THREAD
static int
wl_thread_dpc_wlthread(void *data)
{
	wl_info_t *wl = (wl_info_t *) data;

	current->flags |= PF_NOFREEZE;

#ifdef WL_THREADNICE
	set_user_nice(current, WL_THREADNICE);
#endif

	while (1) {
		wait_event_interruptible_timeout(wl->thread_wqh,
			skb_queue_len(&wl->rpc_queue) || skb_queue_len(&wl->tx_queue),
			1);

		if (kthread_should_stop())
			break;

		wl_rpcq_dispatch_wlthread(wl);
		wl_start_txqwork_wlthread(wl);
	}

	skb_queue_purge(&wl->tx_queue);
	skb_queue_purge(&wl->rpc_queue);

	return 0;
}
#endif /* WL_THREAD */

#if defined(WL_USE_NETDEV_OPS)
/* Physical interface netdev ops */
static const struct net_device_ops wl_netdev_ops =
{
	.ndo_open = wl_open,
	.ndo_stop = wl_close,
#ifdef WL_THREAD
	.ndo_start_xmit = wl_start_wlthread,
#else
	.ndo_start_xmit = wl_start,
#endif
	.ndo_get_stats = wl_get_stats,
	.ndo_set_mac_address = wl_set_mac_address,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
	.ndo_set_rx_mode = wl_set_multicast_list,
#else
	.ndo_set_multicast_list = wl_set_multicast_list,
#endif
	.ndo_do_ioctl = wl_ioctl
};

#if defined(USE_CFG80211) && !defined(BCMDONGLEHOST)
static int wl_preinit_ioctls(struct net_device *ndev)
{

	int ret = 0;
	char eventmask[WL_EVENTING_MASK_LEN];
	char iovbuf[WL_EVENTING_MASK_LEN + 12]; /*  Room for "event_msgs" + '\0' + bitvec  */


	/* Read event_msgs mask */
	bcm_mkiovar("event_msgs", NULL, 0, iovbuf, sizeof(iovbuf));
	ret = wldev_ioctl(ndev, WLC_GET_VAR, iovbuf, sizeof(iovbuf), false);

	if (unlikely(ret)) {
		WL_ERROR(("Get event_msgs error (%d)\n", ret));
		goto done;
	}

	memcpy(eventmask, iovbuf, WL_EVENTING_MASK_LEN);

	/* Setup event_msgs */
	setbit(eventmask, WLC_E_SET_SSID);
	setbit(eventmask, WLC_E_PRUNE);
	setbit(eventmask, WLC_E_AUTH);
	setbit(eventmask, WLC_E_ASSOC);
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
	setbit(eventmask, WLC_E_NDIS_LINK);
	setbit(eventmask, WLC_E_MIC_ERROR);
	setbit(eventmask, WLC_E_ASSOC_REQ_IE);
	setbit(eventmask, WLC_E_ASSOC_RESP_IE);

#ifndef WL_CFG80211
	setbit(eventmask, WLC_E_PMKID_CACHE);
	setbit(eventmask, WLC_E_TXFAIL);
#endif

	setbit(eventmask, WLC_E_JOIN_START);
	setbit(eventmask, WLC_E_SCAN_COMPLETE);

#ifdef WL_CFG80211
	setbit(eventmask, WLC_E_ESCAN_RESULT);

#if defined(WLP2P) && defined(WL_ENABLE_P2P_IF)

	setbit(eventmask, WLC_E_ACTION_FRAME_RX);
	setbit(eventmask, WLC_E_ACTION_FRAME_COMPLETE);
	setbit(eventmask, WLC_E_ACTION_FRAME_OFF_CHAN_COMPLETE);
	setbit(eventmask, WLC_E_P2P_DISC_LISTEN_COMPLETE);
#endif  /* defined(WLP2P) && defined(WL_ENABLE_P2P_IF) */

#ifdef WL_SDO
	setbit(eventmask, WLC_E_SERVICE_FOUND);
	setbit(eventmask, WLC_E_GAS_FRAGMENT_RX);
	setbit(eventmask, WLC_E_GAS_COMPLETE);
#endif
#endif /* WL_CFG80211 */


	/* Write updated Event mask */
	bcm_mkiovar("event_msgs", eventmask, WL_EVENTING_MASK_LEN, iovbuf, sizeof(iovbuf));

	ret = wldev_ioctl(ndev, WLC_SET_VAR, iovbuf, sizeof(iovbuf), true);
	if (unlikely(ret)) {
		WL_ERROR(("Set event_msgs error (%d)\n", ret));
		goto done;
	}

done:
	return ret;
}
#endif /* USE_CFG80211 */

#ifdef WL_MONITOR
static const struct net_device_ops wl_netdev_monitor_ops =
{
	.ndo_start_xmit = wl_monitor_start,
	.ndo_get_stats = wl_get_stats,
	.ndo_do_ioctl = wl_ioctl
};
#endif /* WL_MONITOR */
#endif /* WL_USE_NETDEV_OPS */

static void
wl_if_setup(struct net_device *dev)
{
#if defined(WL_USE_NETDEV_OPS)
	dev->netdev_ops = &wl_netdev_ops;
#else
	dev->open = wl_open;
	dev->stop = wl_close;
#ifdef WL_THREAD
	dev->hard_start_xmit = wl_start_wlthread;
#else
	dev->hard_start_xmit = wl_start;
#endif
	dev->get_stats = wl_get_stats;
	dev->set_mac_address = wl_set_mac_address;
	dev->set_multicast_list = wl_set_multicast_list;
	dev->do_ioctl = wl_ioctl;
#endif /* WL_USE_NETDEV_OPS */
#ifdef NAPI_POLL
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
	{
		struct wl_info *wl = WL_INFO_GET(dev);

		netif_napi_add(dev, &wl->napi, wl_poll, 64);
		napi_enable(&wl->napi);
	}
#else
	dev->poll = wl_poll;
	dev->weight = 64;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
	netif_poll_enable(dev);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21) */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30) */
#endif /* NAPI_POLL */

#ifdef USE_IW
#if WIRELESS_EXT < 19
	dev->get_wireless_stats = wl_get_wireless_stats;
#endif
#if WIRELESS_EXT > 12
	dev->wireless_handlers = (const struct iw_handler_def *) &wl_iw_handler_def;
#endif
#endif /* USE_IW */

#if WIRELESS_EXT >= 19 || LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	dev->ethtool_ops = &wl_ethtool_ops;
#endif
} /* wl_if_setup */

#ifdef HNDCTF

static void
wl_ctf_detach(ctf_t *ci, void *arg)
{
	wl_info_t *wl = (wl_info_t *)arg;

	wl->cih = NULL;

#ifdef CTFPOOL
	/* free the buffers in fast pool */
	osl_ctfpool_cleanup(wl->osh);
#endif /* CTFPOOL */

	return;
}

#endif /* HNDCTF */

#if defined(CONFIG_PROC_FS)
/* create_proc_read_entry() removed in linux 3.10.0, use proc_create_data() instead. */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
#include <linux/seq_file.h>

static int wl_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, wl_read_proc, PDE_DATA(inode));
}

static const struct file_operations wl_proc_fops = {
	.open           = wl_proc_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = seq_release,
};
#endif /* #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)) */
#endif /* #if defined(CONFIG_PROC_FS) */

#ifdef WLCXO_IPC
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
#define BCM_SET_LIST_FIRST_ENTRY(entry, ptr, type, member) \
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic ignored \"-Wcast-qual\"") \
(entry) = list_first_entry((ptr), type, member); \
_Pragma("GCC diagnostic pop") \

#else
#define BCM_SET_LIST_FIRST_ENTRY(entry, ptr, type, member) \
(entry) = list_first_entry((ptr), type, member); \

#endif /* STRICT_GCC_WARNINGS */

static int
wl_swq_thread(void *data)
{
	unsigned long flags;
	struct swq_struct *swq = (struct swq_struct *)data;
	struct work_struct *work;

	current->flags |= PF_NOFREEZE;

	while (!kthread_should_stop()) {
		/* Wait for work items to be added */
		wait_event_interruptible(swq->thread_waitq, !list_empty(&swq->work_list));

		if (kthread_should_stop())
			break;

		/* Process work items */
		while (1) {
			spin_lock_irqsave(&swq->lock, flags);
			if (list_empty(&swq->work_list)) {
				spin_unlock_irqrestore(&swq->lock, flags);
				break;
			}
			BCM_SET_LIST_FIRST_ENTRY(work, &swq->work_list, struct work_struct, entry);
			clear_bit(0, work_data_bits(work));
			list_del_init(&work->entry);
			spin_unlock_irqrestore(&swq->lock, flags);

			/* Call the work handler */
			work->func(work);
		}
	}

	/* Cleanup */
	return 0;
}

/*
 * Add work to work queue.
 */
static int32
wl_swq_add_work(struct swq_struct *swq, struct work_struct *work)
{
	int32 ret = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&swq->lock, flags);
	if (!test_and_set_bit(0, work_data_bits(work))) {
		list_add_tail(&work->entry, &swq->work_list);
		ret = 1;
		spin_unlock_irqrestore(&swq->lock, flags);
		wake_up_interruptible(&swq->thread_waitq);
	} else
		spin_unlock_irqrestore(&swq->lock, flags);
	return ret;
}

/*
 * Create simple light wt workqueue.
 */
static struct swq_struct *
wl_swq_create(wl_info_t *wl, uint8 *name, uint32 cpu)
{
	struct swq_struct *swq;

	swq = MALLOCZ(wl->osh, sizeof(*swq));
	if (swq == NULL)
		return NULL;

	/* Create kernel thread for given workqueue */
	swq->thread = kthread_create(wl_swq_thread, swq, name);
	if (IS_ERR(swq->thread))
		return NULL;

	spin_lock_init(&swq->lock);
	init_waitqueue_head(&swq->thread_waitq);
	INIT_LIST_HEAD(&swq->work_list);

	kthread_bind(swq->thread, cpu);
	wake_up_process(swq->thread);

	return swq;
}
/*
 * destroy wt workqueue
 */
static void
wl_swq_destroy(wl_info_t *wl)
{
	struct swq_struct *swq = wl->cxo_ctrl_wq;

	if (!swq)
		return;

	if (swq->thread) {
		force_sig(SIGKILL, swq->thread);
		kthread_stop(swq->thread);
		swq->thread = NULL;
	}

	MFREE(wl->osh, swq, sizeof(*swq));

}
#endif /* WLCXO_IPC */

/* Below define is only for router platform */
#if defined(CONFIG_WL_MODULE) && defined(CONFIG_BCM47XX)
#ifdef WLRSDB
/**
 * Check if a register_netdev should be blocked or not.
 *
 * This function is currently enabled only for router platform.
 * In case of RSDB chips, if the current operating mode is not RSDB, then the second interface
 * should not be registered. If the second interface is created in MIMO, other router applications,
 * which are dependent on the interfaces created will behave erratically.
 *
 * This function is kept generic, so that it can be extended if required.
 *
 * Returns 0 if the interface should NOT be blocked
 * Returns 1 if the interface should be blocked
 */
static int
wl_rsdb_block_netdev(struct wl_cmn_data *cmndata, uint unit)
{
	const char *var;

	// Always allow first unit to register
	if (unit == 0) {
		return 0;
	}

	// Allow Non-RSDB Chips to register always
	if ((!cmndata->sih) || (si_numd11coreunits(cmndata->sih) <= 1)) {
		return 0;
	}

	var = getvar(NULL, "rsdb_mode");

	/* If "rsdb_mode" not defined, default is mimo mode. mimo mode combines
	 * both d11 cores so do not create a network interface for the second d11.
	 */
	if (!var) {
		return 1;
	}

	/* If "rsdb_mode" is mimo or 80p80, it combines both d11 cores so do not create
	   a network interface for the second d11.
	 */
	if ((bcm_atoi(var) == WLC_RSDB_MODE_2X2) ||
			(bcm_atoi(var) == WLC_RSDB_MODE_80P80)) {
		return 1;
	}

	return 0;
}
#endif /* WLRSDB */
#endif /* CONFIG_WL_MODULE && CONFIG_BCM47XX */

/**
 * attach to the WL device.
 *
 * Attach to the WL device identified by vendor and device parameters.
 * regs is a host accessible memory address pointing to WL device registers.
 *
 * wl_attach is not defined as static because in the case where no bus
 * is defined, wl_attach will never be called, and thus, gcc will issue
 * a warning that this function is defined but not used if we declare
 * it as static.
 */

#if defined(BCM7271) && !defined(PLATFORM_INTEGRATED_WIFI)
wl_info_t *
#else
static wl_info_t *
#endif /* BCM7271 && !PLATFORM_INTEGRATED_WIFI */
wl_attach(uint16 vendor, uint16 device, ulong regs,
	uint bustype, void *btparam, uint irq, uchar* bar1_addr, uint32 bar1_size,
	uchar* bar2_addr, uint32 bar2_size, void *cmndata)
{
	struct net_device *dev;
	wl_if_t *wlif;
	wl_info_t *wl;
#if defined(CONFIG_PROC_FS)
	char tmp[128];
#endif
	osl_t *osh;
	int unit, err;

#ifdef WLRSDB
	unsigned int pci_barwin_sz = PCI_BAR0_WINSZ * 2;
#elif defined(BCM7271)
	unsigned int pci_barwin_sz = SI_REG_BASE_SIZE;
#else
	unsigned int pci_barwin_sz = PCIE2_BAR0_WINSZ;
#endif
#ifdef HNDCTF
	char ctf_name[IFNAMSIZ];
#endif /* HNDCTF */
#ifdef WL_OBJ_REGISTRY
	uint numcores = 0;
	int wl_unit = 0;
#endif /* WL_OBJ_REGISTRY */
#if defined(USE_CFG80211)
	struct device *parentdev;
#endif
#ifdef CTFPOOL
	int32 ctfpoolsz;
#endif
	struct wl_cmn_data	*commmondata = (struct wl_cmn_data	*)cmndata;
	int primary_idx = 0;
	uint online_cpus, iomode = 0;
	unit = wl_found + instance_base;
	err = 0;

	if (unit < 0) {
		WL_ERROR(("wl%d: unit number overflow, exiting\n", unit));
		return NULL;
	}

	if (oneonly && (unit != instance_base)) {
		WL_ERROR(("wl%d: wl_attach: oneonly is set, exiting\n", unit));
		return NULL;
	}

	/* Requires pkttag feature */
#ifdef SHARED_OSL_CMN
	/* Use single osh->cmn to keep track of memory usage and allocations. */
	osh = osl_attach(btparam, bustype, TRUE, &commmondata->oshcmnptr);
#else
	BCM_REFERENCE(commmondata);
	osh = osl_attach(btparam, bustype, TRUE);
#endif /* SHARED_OSL_CMN */

	ASSERT(osh);

	/* Set ACP coherence flag */
	if (OSL_ACP_WAR_ENAB() || OSL_ARCH_IS_COHERENT())
		osl_flag_set(osh, OSL_ACP_COHERENCE);

	/* allocate private info */
	if ((wl = (wl_info_t*) MALLOCZ(osh, sizeof(wl_info_t))) == NULL) {
		WL_ERROR(("wl%d: malloc wl_info_t, out of memory, malloced %d bytes\n", unit,
			MALLOCED(osh)));
		osl_detach(osh);
		return NULL;
	}

	wl->osh = osh;
	wl->unit = unit;
	atomic_set(&wl->callbacks, 0);

#if defined(BCM7271) && !defined(PLATFORM_INTEGRATED_WIFI)
	wl->bcm7xxx = wl_bcm7xxx_attach(wl);
#endif /* BCM7271 */


#ifdef CONFIG_SMP
	/* initialize number of online cpus */
	online_cpus = num_online_cpus();
#if defined(BCM47XX_CA9)
	if (online_cpus > 1) {
		if (wl_txq_thresh == 0)
			wl_txq_thresh = TXQ_CNT_THRESH;
	}
#endif /* BCM47XX_CA9 */
#else
	online_cpus = 1;
#endif /* CONFIG_SMP */
	wl->max_cpu_id = online_cpus - 1;

	WL_ERROR(("wl%d: online cpus %d\n", unit, online_cpus));

#ifdef WL_ALL_PASSIVE
	wl->all_dispatch_mode = (passivemode == 0) ? TRUE : FALSE;
	if (WL_ALL_PASSIVE_ENAB(wl)) {
#ifdef WLCXO_IPC
		/* create workqueue on host processor to process events from offload
		 * processor as well as others. bind the ctrl task to cpu 1.
		 */
		wl->cxo_ctrl_wq = wl_swq_create(wl, "wl-cx-ctrl-ipc", 1);
		if (IS_ERR(wl->cxo_ctrl_wq))
			goto fail;
#endif
		/* init tx work queue for wl_start/send pkt; no need to destroy workitem  */
		MY_INIT_WORK(&wl->txq_task.work, (work_func_t)wl_start_txqwork);
		wl->txq_task.context = wl;

		/* init work queue for wl_set_multicast_list(); no need to destroy workitem  */
		MY_INIT_WORK(&wl->multicast_task.work, (work_func_t)wl_set_multicast_list_workitem);

		MY_INIT_WORK(&wl->wl_dpc_task.work, (work_func_t)wl_dpc_rxwork);
		wl->wl_dpc_task.context = wl;
	} else if (txworkq) {
		/* init tx work queue for wl_start/send pkt; no need to destroy workitem  */
		MY_INIT_WORK(&wl->txq_task.work, (work_func_t)wl_start_txqwork);
		wl->txq_task.context = wl;
	}
#endif /* WL_ALL_PASSIVE */

	wl->txq_dispatched = FALSE;
	wl->txq_head = wl->txq_tail = NULL;
	wl->txq_cnt = 0;

	/* Allocate a wlif */
	wlif = wl_alloc_if(wl, WL_IFTYPE_BSS, 0, NULL);
	if (!wlif) {
		WL_ERROR(("wl%d: %s: wl_alloc_if failed\n", unit, __FUNCTION__));
		MFREE(osh, wl, sizeof(wl_info_t));
		osl_detach(osh);
		return NULL;
	}

	/* Allocate netdev and sets wlif->dev & dev->local->wlif */
	if (wl_alloc_linux_if(wlif) == 0) {
		WL_ERROR(("wl%d: %s: wl_alloc_linux_if failed\n", unit, __FUNCTION__));
		MFREE(osh, wl, sizeof(wl_info_t));
		osl_detach(osh);
		return NULL;
	}

	dev = wlif->dev;
	wl->dev = dev;
	wl_if_setup(dev);

	/* map chip registers (47xx: and sprom) */
	dev->base_addr = regs;

	WL_TRACE(("wl%d: Bus: ", unit));
	if (bustype == PCMCIA_BUS) {
		/* Disregard command overwrite */
		wl->piomode = TRUE;
		WL_TRACE(("PCMCIA\n"));
	} else if (bustype == PCI_BUS) {
		/* piomode can be overwritten by command argument */
		wl->piomode = piomode;
		WL_TRACE(("PCI/%s\n", wl->piomode ? "PIO" : "DMA"));
	}
#ifdef BCM7271
	else if (bustype == SI_BUS) {
	WL_TRACE(("USING SI_BUS\n"));
	}
#endif 
	else if (bustype == RPC_BUS) {
		/* Do nothing */
	} else {
		bustype = PCI_BUS;
		WL_TRACE(("force to PCI\n"));
	}
	wl->bcm_bustype = bustype;

	if ((wl->regsva = ioremap_nocache(dev->base_addr, pci_barwin_sz)) == NULL) {
		WL_ERROR(("wl%d: ioremap() failed\n", unit));
		goto fail;
	}

#ifdef BCM7271
	WL_ERROR(("%s(%d) REG_MAP PHY=0x%08x VIR=0x%p REG_MAP_SIZE=0x%08x\n",
		__FUNCTION__, __LINE__, (uint32)dev->base_addr, wl->regsva, pci_barwin_sz));
#endif /* BCM7271 */
#if defined(VASIP_HW_SUPPORT)
	wl->bar1_addr = bar1_addr;
	wl->bar1_size = bar1_size;
	wl->bar2_addr = bar2_addr;
	wl->bar2_size = bar2_size;
#endif

	spin_lock_init(&wl->lock);
	spin_lock_init(&wl->isr_lock);

	if (WL_ALL_PASSIVE_ENAB(wl))
		sema_init(&wl->sem, 1);

	spin_lock_init(&wl->txq_lock);
#ifdef WLCXO_SIM
	g_txhpktq_lock = &wl->txq_lock;
#endif


#ifdef WL_OBJ_REGISTRY
	if (commmondata->sih) {
		numcores = si_numd11coreunits(commmondata->sih);
	}

	/* Note : For RSDB chips like 53573, the wl_attach is called two times for each core.
	   During the first call, commondata->sih will be null, and numcores will be 0.
	   For non RSDB chips also numcores will be zero at this point.
	   So, the following condition is satisfied for
	    - Non RSDB chips
	    - RSDB chips first instance
	   The else case will be satisfied for RSDB chip 2nd instance, where it has to share the
	   objregistry from the first core. This check also ensures that obj registry is not shared
	   between 2 different chips. Ex: 4360+4335 in router platform.
	*/
	if ((numcores <= 1) || (commmondata->device != device)) {
		wl->objr = obj_registry_alloc(osh, OBJR_MAX_KEYS);
		commmondata->objrptr = wl->objr;
		obj_registry_set(wl->objr, OBJR_SELF, wl->objr);
	} else {
		wl->objr = (wlc_obj_registry_t *)commmondata->objrptr;
	}

	obj_registry_ref(wl->objr, OBJR_SELF);

	wl->cmn = (wl_cmn_info_t *)obj_registry_get(wl->objr, OBJR_WL_CMN_INFO);
	if (wl->cmn == NULL) {
		if ((wl->cmn = (wl_cmn_info_t *)MALLOCZ(osh, sizeof(wl_cmn_info_t))) == NULL) {
			goto fail;
		}
		obj_registry_set(wl->objr, OBJR_WL_CMN_INFO, wl->cmn);
	}
	obj_registry_ref(wl->objr, OBJR_WL_CMN_INFO);
	wl_unit = 0;

	/* Find the wl index by identifying the first non-null location of the cmn->wl array. */
	while (wl->cmn->wl[wl_unit]) {
		wl_unit++;
	}

	/* Keep a copy of all wl pointers in the wl->cmn structure */
	wl->cmn->wl[wl_unit] = wl;

#endif /* WL_OBJ_REGISTRY */

#ifdef HNDCTF
	(void)snprintf(ctf_name, sizeof(ctf_name), "wl%d", unit);
	wl->cih = ctf_attach(osh, ctf_name, &wl_msg_level, wl_ctf_detach, wl);
#endif /* HNDCTF */

#ifdef WLCXO_FULL
	wl->wl_cxo = wl_cxo_attach(osh);
#endif /* WLCXO_FULL */

	if (wl->piomode)
		iomode = IOMODE_TYPE_PIO;
#if defined(BCM_DMA_CT) && !defined(BCM_DMA_CT_DISABLED)
	else if (ctdma)
		iomode = IOMODE_TYPE_CTDMA;
#endif

	/* common load-time initialization */
	if (!(wl->wlc = wlc_attach((void *) wl, vendor, device, unit, iomode,
		osh, wl->regsva, wl->bcm_bustype, btparam, wl->objr, &err))) {
		printf("wl driver %s failed with code %d\n", EPI_VERSION_STR, err);
		goto fail;
	}

	wl->pub = wlc_pub(wl->wlc);

	if (commmondata != NULL) {
		/* Make sure object has been allocated */
		commmondata->device = device;
		commmondata->sih =  wl->pub->sih;
	}

#if defined(WLCXO_IPC)
	if (WLCXO_ENAB(wl->pub)) {
		wl->ipc = wlc_cxo_ctrl_ipc(wl->wlc);
		ASSERT(wl->ipc != NULL);
	}
#endif

	/* Populate wlcif of the primary interface in wlif */
	primary_idx = WLC_BSSCFG_IDX(wlc_bsscfg_primary(wl->wlc));
	wlif->wlcif = wlc_wlcif_get_by_index(wl->wlc, primary_idx);

	if (nompc) {
		if (wlc_iovar_setint(wl->wlc, "mpc", 0)) {
			WL_ERROR(("wl%d: Error setting MPC variable to 0\n", unit));
		}
	}

#if defined(CONFIG_PROC_FS)
	/* create /proc/net/wl<unit> */
	(void)snprintf(tmp, sizeof(tmp), "net/wl%d", wl->pub->unit);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	/* create_proc_read_entry removed in linux 3.10.0, use proc_create_data() instead. */
	wl->proc_entry = proc_create_data(tmp, S_IRUGO, NULL, &wl_proc_fops, (void*)wl);
#else
	wl->proc_entry = create_proc_read_entry(tmp, 0, 0, wl_read_proc, (void*)wl);
#endif /* #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)) */
	if (wl->proc_entry == NULL) {
		WL_ERROR(("wl%d: Create proc entry '%s' failed\n", unit, tmp));
		goto fail;
	}
	WL_INFORM(("wl%d: Created the proc entry %s \n", unit, tmp));
#endif /* #if defined(CONFIG_PROC_FS) */

	bcopy(&wl->pub->cur_etheraddr, dev->dev_addr, ETHER_ADDR_LEN);

#ifdef WLCXO_CTRL
	if (online_cpus > 1 && txworkq == 0) {
#ifdef WLCXO_SIM
		ASSERT(WL_ALL_PASSIVE_ENAB(wl));
		txworkq = 1;
#else
		txworkq = 0;
#endif
		printf("%s: txworkq set to %d\n", __FUNCTION__, txworkq);
	}
#endif /* WLCXO_CTRL */

#ifndef NAPI_POLL
	/* setup the bottom half handler */
	tasklet_init(&wl->tasklet, wl_dpc, (ulong)wl);
#endif

	tasklet_init(&wl->tx_tasklet, wl_tx_tasklet, (ulong)wl);

#ifdef TOE
	/* allocate the toe info struct */
	if ((wl->toei = wl_toe_attach(wl->wlc)) == NULL) {
		WL_ERROR(("wl%d: wl_toe_attach failed\n", unit));
		goto fail;
	}
#endif

#ifdef ARPOE
	/* allocate the arp info struct */
	if ((wl->arpi = wl_arp_attach(wl->wlc)) == NULL) {
		WL_ERROR(("wl%d: wl_arp_attach failed\n", unit));
		goto fail;
	}
#endif

#ifndef LINUX_POSTMOGRIFY_REMOVAL
#ifdef WL_EVENTQ
	/* allocate wl_eventq info struct */
	if ((wl->wlevtq = wl_eventq_attach(wl->wlc)) == NULL) {
		WL_ERROR(("wl%d: wl_eventq_attach failed\n", unit));
		goto fail;
	}
#endif /* WL_EVENTQ */

#if defined(P2PO) || defined(ANQPO)
	/* allocate gas info struct */
	if ((wl->gas = wl_gas_attach(wl->wlc, wl->wlevtq)) == NULL) {
		WL_ERROR(("wl%d: wl_gas_attach failed\n", unit));
		goto fail;
	}
#endif /* P2PO || ANQPO */

#if defined(P2PO)
	/* allocate the disc info struct */
	if ((wl->disc = wl_disc_attach(wl->wlc)) == NULL) {
		WL_ERROR(("wl%d: wl_disc_attach failed\n", unit));
		goto fail;
	}

	/* allocate the p2po info struct */
	if ((wl->p2po = wl_p2po_attach(wl->wlc, wl->wlevtq, wl->gas, wl->disc)) == NULL) {
		WL_ERROR(("wl%d: wl_p2po_attach failed\n", unit));
		goto fail;
	}
#endif /* P2PO */

#if defined(ANQPO)
	/* allocate the anqpo info struct */
	if ((wl->anqpo = wl_anqpo_attach(wl->wlc, wl->gas)) == NULL) {
		WL_ERROR(("wl%d: wl_anqpo_attach failed\n", unit));
		goto fail;
	}
#endif /* ANQPO */

#if defined(BDO)
	/* allocate the bdo info struct */
	if ((wl->bdo = wl_bdo_attach(wl->wlc)) == NULL) {
		WL_ERROR(("wl%d: wl_bdo_attach failed\n", unit));
		goto fail;
	}
#endif /* defined(BDO) */
#if defined(TKO)
	/* allocate the tko info struct */
	if ((wl->tko = wl_tko_attach(wl->wlc)) == NULL) {
		WL_ERROR(("wl%d: wl_tko_attach failed\n", unit));
		goto fail;
	}
#endif /* TKO */
#endif /* LINUX_POSTMOGRIFY_REMOVAL */

#ifdef HNDCTF
#endif /* HNDCTF */

#ifdef CTFPOOL
	/* create packet pool with specified number of buffers */
	ctfpoolsz = (IS_AC2_DEV(device) ||
		IS_DEV_AC3X3(device) || IS_DEV_AC2X2(device)) ? CTFPOOLSZ_AC3X3 : CTFPOOLSZ;
	if (CTF_ENAB(wl->cih) && (num_physpages >= 8192) &&
	    (osl_ctfpool_init(osh, ctfpoolsz, PKTBUFSZ+BCMEXTRAHDROOM) < 0))
	{
		WL_ERROR(("wl%d: wlc_attach: osl_ctfpool_init failed\n", unit));
		goto fail;
	}
#endif /* CTFPOOL */

	/* register our interrupt handler */
	{
		if (!WLCXO_ENAB(wl->pub)) {
			/* Request irq for cxo full and non cxo driver cases */
			if (request_irq(irq, wl_isr, IRQF_SHARED, dev->name, wl)) {
				WL_ERROR(("wl%d: request_irq() failed\n", unit));
				goto fail;
			}
		}
		dev->irq = irq;

	}

#if defined(USE_IW)
	WL_ERROR(("Using Wireless Extension\n"));
#endif

#if defined(USE_CFG80211)

#if !defined(BCMDONGLEHOST)
	wl_preinit_ioctls(dev);
#endif /* !BCMDONGLEHOST */

	parentdev = NULL;
	if (wl->bcm_bustype == PCI_BUS) {
		parentdev = &((struct pci_dev *)btparam)->dev;
	}
#ifdef BCM7271
	wl_cfg80211_attach(dev, parentdev);
#else
	if (parentdev) {
		if (wl_cfg80211_attach(dev, parentdev)) {
			goto fail;
		}
	}
	else {
		WL_ERROR(("unsupported bus type\n"));
		goto fail;
	}
#endif /* BCM7271 */
#else

	if (wl->bcm_bustype == PCI_BUS) {
		struct pci_dev *pci_dev = (struct pci_dev *)btparam;
		ASSERT(pci_dev);
		SET_NETDEV_DEV(dev, &pci_dev->dev);
	}
#endif /* defined(USE_CFG80211) */

#ifdef WLCSO
	if (wlc_tso_support(wl->wlc))
		dev->features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
#endif /* WLCSO */

#if defined(CONFIG_WL_MODULE) && defined(CONFIG_BCM47XX)
#if defined(__ARM_ARCH_7A__) && defined(CONFIG_INET_GRO)
	dev->features |= NETIF_F_GRO;
#endif	/* __ARM_ARCH_7A__ && CONFIG_INET_GRO */
#endif /* CONFIG_WL_MODULE && CONFIG_BCM47XX */

#if defined(CONFIG_WL_MODULE) && defined(CONFIG_BCM47XX)
	/* Only for router platform */
	/* Check if the netdev should be allowed to register or not */
#ifdef WLRSDB
	if (wl_rsdb_block_netdev(commmondata, rsdb_found)) {
		goto success;
	}
	if (si_numd11coreunits(commmondata->sih) > 1) {
		/* This is a RSDB chip */
		rsdb_found++;
	}
#endif /* WLRSDB */
#endif /* CONFIG_WL_MODULE && CONFIG_BCM47XX */

	if (register_netdev(dev)) {
		WL_ERROR(("wl%d: register_netdev() failed\n", unit));
		goto fail;
	}
	wlif->dev_registered = TRUE;


#if defined(HNDCTF)
	if ((ctf_dev_register(wl->cih, dev, FALSE) != BCME_OK) ||
	    (ctf_enable(wl->cih, dev, TRUE, &wl->pub->brc_hot) != BCME_OK)) {
		WL_ERROR(("wl%d: ctf_dev_register() failed\n", unit));
		goto fail;
	}
#endif /* HNDCTF */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#ifdef LINUX_CRYPTO
	/* load the tkip module */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	wl->tkipmodops = lib80211_get_crypto_ops("TKIP");
	if (wl->tkipmodops == NULL) {
		request_module("lib80211_crypt_tkip");
		wl->tkipmodops = lib80211_get_crypto_ops("TKIP");
	}
#else
	wl->tkipmodops = ieee80211_get_crypto_ops("TKIP");
	if (wl->tkipmodops == NULL) {
		request_module("ieee80211_crypt_tkip");
		wl->tkipmodops = ieee80211_get_crypto_ops("TKIP");
	}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29) */
#endif /* LINUX_CRYPTO */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14) */
#ifdef USE_IW
	wlif->iw.wlinfo = (void *)wl;
#endif

#if defined(WL_CONFIG_RFKILL)
	if (wl_init_rfkill(wl) < 0)
		WL_ERROR(("%s: init_rfkill_failure\n", __FUNCTION__));
#endif

#ifdef DISABLE_HT_RATE_FOR_WEP_TKIP
	/* disallow HT rate for WEP/TKIP */
	if (wlc_iovar_setint(wl->wlc, "ht_wsec_restrict", 0x3)) {
		WL_ERROR(("wl%d: Error setting ht_wsec_restrict \n", unit));
	}
#endif /* DISABLE_HT_RATE_FOR_WEP_TKIP */

#ifdef DEFAULT_EAPVER_AP
	/* use EAPOL version from AP */
	if (wlc_iovar_setint(wl->wlc, "sup_wpa2_eapver", -1)) {
		WL_ERROR(("wl%d: Error setting sup_wpa2_eapver \n", unit));
	}
	if (wlc_iovar_setint(wl->wlc, "sup_m3sec_ok", 1)) {
		WL_ERROR(("wl%d: Error setting sup_m3sec_ok \n", unit));
	}
#endif /* DEFAULT_EAPVER_AP */

	/* register module */
	if (wlc_module_register(wl->pub, NULL, "linux", wl, NULL, wl_linux_watchdog, NULL, NULL)) {
		WL_ERROR(("wl%d: %s wlc_module_register() failed\n",
		          wl->pub->unit, __FUNCTION__));
		goto fail;
	}


	if (device > 0x9999)
		printf("%s: Broadcom BCM%d 802.11 Wireless Controller " EPI_VERSION_STR,
			dev->name, device);
	else
		printf("%s: Broadcom BCM%04x 802.11 Wireless Controller " EPI_VERSION_STR,
			dev->name, device);

	printf("\n");

#if defined(CONFIG_WL_MODULE) && defined(CONFIG_BCM47XX)
#ifdef WLRSDB
success:
#endif
#endif /* CONFIG_WL_MODULE && CONFIG_BCM47XX */
	wl_found++;
	return wl;

fail:
	wl_free(wl);
	return NULL;
} /* wl_attach */

#ifdef BCMDBUS
dbus_extdl_t dbus_extdl = {NULL, 0, NULL, 0};

static void *
wl_dbus_probe_cb(void *arg, const char *desc, uint32 bustype, uint32 hdrlen)
{
	wl_info_t *wl;
	WL_ERROR(("%s: \n", __FUNCTION__));

	if (!(wl = wl_attach(BCM_DNGL_VID, BCM_DNGL_BDC_PID, (ulong)NULL,
		RPC_BUS, NULL, 0, NULL, 0, NULL, 0, 0))) {
		WL_ERROR(("%s: wl_attach failed\n", __FUNCTION__));
	}

	/* This is later passed to wl_dbus_disconnect_cb */
	return wl;
}

static void
wl_dbus_disconnect_cb(void *arg)
{
	wl_info_t *wl = arg;

	WL_ERROR(("%s: \n", __FUNCTION__));

	if (wl) {
		struct wl_if *wlif = NULL;
		for (wlif = wl->if_list; wlif != NULL; wlif = wlif->next) {
			if (wlif->dev) {
				netif_stop_queue(wlif->dev);
			}
		}

		WL_LOCK(wl);
		wl_down(wl);
		WL_UNLOCK(wl);
		wl_free(wl);
	}
} /* wl_dbus_disconnect_cb */

#endif /* BCMDBUS */

#if defined(CONFIG_PROC_FS)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static int
wl_read_proc(struct seq_file *m, void *v)
{
	wl_info_t *wl;
	char buffer[1016] = {0};

	wl = (wl_info_t *)v;

	WL_LOCK(wl);
	WL_UNLOCK(wl);

	seq_puts(m, buffer);

	return 0;
}
#else /* #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)) */
static int
wl_read_proc(char *buffer, char **start, off_t offset, int length, int *eof, void *data)
{
	int len;
	off_t pos;
	off_t begin;

	len = pos = begin = 0;

	pos = begin + len;

	if (pos < offset) {
		len = 0;
		begin = pos;
	}

	*eof = 1;

	*start = buffer + (offset - begin);
	len -= (offset - begin);

	if (len > length)
		len = length;

	return (len);
} /* wl_read_proc */
#endif /* #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)) */
#endif /* #if defined(CONFIG_PROC_FS) */

#if defined(BCM7271) && !defined(PLATFORM_INTEGRATED_WIFI)
#else /* !BCM7271 && !PLATFORM_INTEGRATED_WIFI */

/* For now, JTAG, SDIO, and PCI are mutually exclusive.  When this changes, remove
 * #if !defined(BCMJTAG) && !defined(BCMSDIO) ... #endif conditionals.
 */
#ifdef CONFIG_PCI
static void __devexit wl_remove(struct pci_dev *pdev);

/**
 * determines if a device is a WL device, and if so, attaches it.
 *
 * This function determines if a device pointed to by pdev is a WL device,
 * and if so, performs a wl_attach() on it.
 *
 */
int __devinit
wl_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int rc;
	wl_info_t *wl;
#ifdef LINUXSTA_PS
	uint32 val;
#endif
	uint32 bar1_size = 0, bar2_size = 0;
	void *bar1_addr = NULL;
	void *bar2_addr = NULL;

	if (!pdev->irq) {
		WL_TRACE(("%s: Rejecting device 0x%x with irq 0 on bus %d slot %d\n",
			__FUNCTION__, pdev->device, pdev->bus->number, PCI_SLOT(pdev->devfn)));
		return (-ENODEV);
	}

	WL_TRACE(("%s: bus %d slot %d func %d irq %d\n", __FUNCTION__,
		pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn), pdev->irq));

	if ((pdev->vendor != PCI_VENDOR_ID_BROADCOM) ||
	    (((pdev->device & 0xff00) != 0x4300) &&
	     (pdev->device != 0x576) &&
	     ((pdev->device & 0xff00) != 0x4700) &&
	     ((pdev->device < 43000) || (pdev->device > 43999)))) {
		WL_TRACE(("%s: unsupported vendor %x device %x\n", __FUNCTION__,
			pdev->vendor, pdev->device));
		return (-ENODEV);
	}
#if defined(WLRSDB) && defined(WLRSDB_DISABLED)
	if (BCM53573_DEVICE(pdev->device) && wl_found) {
		WL_TRACE(("%s: Blocking second wl for RSDB_DISABLED device %x\n",
				__FUNCTION__, pdev->device));
		return (-ENODEV);
	}
#endif /* WLRSDB && WLRSDB_DISABLED */

	rc = pci_enable_device(pdev);
	if (rc) {
		WL_ERROR(("%s: Cannot enable device %d-%d_%d\n", __FUNCTION__,
			pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn)));
		return (-ENODEV);
	}
	pci_set_master(pdev);

#ifdef LINUXSTA_PS
	/*
	 * Disable the RETRY_TIMEOUT register (0x41) to keep
	 * PCI Tx retries from interfering with C3 CPU state.
	 * Code taken from ipw2100 driver
	 */
	pci_read_config_dword(pdev, 0x40, &val);
	if ((val & 0x0000ff00) != 0)
		pci_write_config_dword(pdev, 0x40, val & 0xffff00ff);
#endif /* LINUXSTA_PS */

#if defined(VASIP_HW_SUPPORT)
	bar1_size = pci_resource_len(pdev, 2);
	bar1_addr = (uchar *)ioremap_nocache(pci_resource_start(pdev, 2),
		bar1_size);
	bar2_size = pci_resource_len(pdev, 4);
	if (bar2_size) {
		bar2_addr = (uchar *)ioremap_nocache(pci_resource_start(pdev, 4), bar2_size);
	}
#endif

	wl = wl_attach(pdev->vendor, pdev->device, pci_resource_start(pdev, 0), PCI_BUS, pdev,
		pdev->irq, bar1_addr, bar1_size, bar2_addr, bar2_size, (void *)ent->driver_data);

	if (!wl)
		return -ENODEV;

	pci_set_drvdata(pdev, wl);

	return 0;
} /* wl_pci_probe */


#ifdef LINUXSTA_PS

/** LINUXSTA_PS specific function */
static int
wl_suspend(struct pci_dev *pdev, DRV_SUSPEND_STATE_TYPE state)
{
	wl_info_t *wl = (wl_info_t *) pci_get_drvdata(pdev);

	WL_TRACE(("wl: wl_suspend\n"));

	wl = (wl_info_t *) pci_get_drvdata(pdev);
	if (!wl) {
		WL_ERROR(("wl: wl_suspend: pci_get_drvdata failed\n"));
		return -ENODEV;
	}

	WL_LOCK(wl);
	WL_APSTA_UPDN(("wl%d (%s): wl_suspend() -> wl_down()\n", wl->pub->unit, wl->dev->name));
	si_pcie_prep_D3(wl->pub->sih, TRUE);
	wl_down(wl);
	wl->pub->hw_up = FALSE;
	si_pci_sleep(wl->pub->sih);
	wl->pub->hw_off = TRUE;
	WL_UNLOCK(wl);
	PCI_SAVE_STATE(pdev, wl->pci_psstate);
	pci_disable_device(pdev);
	return pci_set_power_state(pdev, PCI_D3hot);
}

/** LINUXSTA_PS specific function */
static int
wl_resume(struct pci_dev *pdev)
{
	wl_info_t *wl = (wl_info_t *) pci_get_drvdata(pdev);
	int err = 0;
	uint32 val;

	WL_TRACE(("wl: wl_resume\n"));

	if (!wl) {
		WL_ERROR(("wl: wl_resume: pci_get_drvdata failed\n"));
		return -ENODEV;
	}

	err = pci_set_power_state(pdev, PCI_D0);
	if (err)
		return err;

	PCI_RESTORE_STATE(pdev, wl->pci_psstate);

	err = pci_enable_device(pdev);
	if (err)
		return err;

	pci_set_master(pdev);
	/*
	 * Suspend/Resume resets the PCI configuration space, so we have to
	 * re-disable the RETRY_TIMEOUT register (0x41) to keep
	 * PCI Tx retries from interfering with C3 CPU state
	 * Code taken from ipw2100 driver
	 */
	pci_read_config_dword(pdev, 0x40, &val);
	if ((val & 0x0000ff00) != 0)
		pci_write_config_dword(pdev, 0x40, val & 0xffff00ff);

	WL_LOCK(wl);
	WL_APSTA_UPDN(("wl%d: (%s): wl_resume() -> wl_up()\n", wl->pub->unit, wl->dev->name));

	wl->pub->hw_off = FALSE;
	err = wl_up(wl); /* re-inits e.g. PMU registers before 'up' */
	WL_UNLOCK(wl);

	return (err);
} /* wl_resume */

#endif /* LINUXSTA_PS */


static void __devexit
wl_remove(struct pci_dev *pdev)
{
	wl_info_t *wl = (wl_info_t *) pci_get_drvdata(pdev);
	uint16 vendorid, deviceid;

	if (!wl) {
		WL_ERROR(("wl: wl_remove: pci_get_drvdata failed\n"));
		return;
	}

	/* Get the the actual vendor/device id used in the driver */
	wlc_get_override_vendor_dev_id(wl->wlc, &vendorid, &deviceid);

	if (!wlc_chipmatch(vendorid, deviceid)) {
		WL_ERROR(("wl: wl_remove: wlc_chipmatch failed\n"));
		return;
	}

	/* wl_set_monitor(wl, 0); */

	WL_LOCK(wl);
	WL_APSTA_UPDN(("wl%d (%s): wl_remove() -> wl_down()\n", wl->pub->unit, wl->dev->name));
	wl_down(wl);
	WL_UNLOCK(wl);
#ifdef DSLCPE_DGASP
	kerSysDeregisterDyingGaspHandler(wl_netdev_get(wl)->name);
#endif /* DSLCPE_DGASP */

	wl_free(wl);
	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
}

static struct pci_driver wl_pci_driver = {
	name:		"wl",
	probe:		wl_pci_probe,
#ifdef LINUXSTA_PS
	suspend:	wl_suspend,
	resume:		wl_resume,
#endif /* LINUXSTA_PS */
	remove:		__devexit_p(wl_remove),
	id_table:	wl_id_table,
	};
#endif /* CONFIG_PCI */



#ifdef PLATFORM_INTEGRATED_WIFI
static struct platform_driver wl_plat_drv = {
	.probe	=		wl_plat_drv_probe,
	.remove =		wl_plat_drv_remove,
	.shutdown =		wl_plat_drv_shutdown,
	.suspend =		wl_plat_drv_suspend,
	.resume =		wl_plat_drv_resume,
	.driver =		{
		.name = "wl",
		.owner = THIS_MODULE,
		.of_match_table = plat_devices_of_match,
		},
};
#endif /* PLATFORM_INTEGRATED_WIFI */

#if defined(BCM_DNGL_EMBEDIMAGE)


#define wl_read_nvram_file() do {} while (0)

#define usbap_parse_nvram()	do {} while (0)
#endif /* BCM_DNGL_EMBEDIMAGE */

/**
 * This is the main entry point for the WL driver.
 *
 * This function determines if a device pointed to by pdev is a WL device,
 * and if so, performs a wl_attach() on it.
 */
static int __init
wl_module_init(void)
{
	int error = -ENODEV;


#if defined(WL_ALL_PASSIVE)
	{
		const char *var = getvar(NULL, "wl_dispatch_mode");
		if (var)
			passivemode = bcm_strtoul(var, NULL, 0);
		printf("%s: passivemode set to 0x%x\n", __FUNCTION__, passivemode);
		var = getvar(NULL, "txworkq");
		if (var)
			txworkq = bcm_strtoul(var, NULL, 0);
		printf("%s: txworkq set to 0x%x\n", __FUNCTION__, txworkq);
	}
#endif /* defined(WL_ALL_PASSIVE) */


#if defined(CONFIG_WL_ALL_PASSIVE_RUNTIME) || defined(WL_ALL_PASSIVE)
	{
		char *var = getvar(NULL, "wl_txq_thresh");
		if (var)
			wl_txq_thresh = bcm_strtoul(var, NULL, 0);
	}
#endif /* CONFIG_WL_ALL_PASSIVE_RUNTIME || WL_ALL_PASSIVE */



#ifdef CONFIG_PCI
	if (!(error = pci_module_init(&wl_pci_driver)))
		return (0);

#ifdef WL_PCMCIA
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	if (!(error = pcmcia_register_driver(&wl_driver)))
		return (0);
#else
	if (!(error = register_pccard_driver(&dev_info, &wl_cs_attach, &wl_cs_detach)))
		return (0);
#endif
#endif /* WL_PCMCIA */
#endif /* CONFIG_PCI */

#ifdef BCMDBUS
#if defined(BCM_DNGL_EMBEDIMAGE)
	usbap_parse_nvram();
	wl_read_nvram_file();
#endif

	/* BMAC_NOTE: define hardcode number, why NODEVICE is ok ? */
	error = dbus_register(BCM_DNGL_VID, 0, wl_dbus_probe_cb, wl_dbus_disconnect_cb,
		NULL, NULL, NULL);
	if (error == DBUS_ERR_NODEVICE) {
		error = DBUS_OK;
	}
#endif /* BCMDBUS */

#ifdef PLATFORM_INTEGRATED_WIFI
	if (!(error = platform_driver_register(&wl_plat_drv))) {
		return 0;
	}
#endif /* PLATFORM_INTEGRATED_WIFI */

	return (error);
} /* wl_module_init */

/**
 * This function unloads the WL driver from the system.
 *
 * This function unconditionally unloads the WL driver module from the
 * system.
 */
static void __exit
wl_module_exit(void)
{


#ifdef CONFIG_PCI
	pci_unregister_driver(&wl_pci_driver);

#ifdef WL_PCMCIA
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)

	pcmcia_unregister_driver(&wl_driver);

#else /* !LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0) */

	unregister_pccard_driver(&dev_info);

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0) */

	while (dev_list != NULL) {
		if (dev_list->state & DEV_CONFIG)
			wl_cs_release((u_long) dev_list);
		wl_cs_detach(dev_list);
	}
#endif /* WL_PCMCIA */
#endif /* CONFIG_PCI */

#ifdef BCMDBUS
	dbus_deregister();
#endif /* BCMDBUS */

#ifdef PLATFORM_INTEGRATED_WIFI
	platform_driver_unregister(&wl_plat_drv);
#endif /* PLATFORM_INTEGRATED_WIFI */
} /* wl_module_exit */

module_init(wl_module_init);
module_exit(wl_module_exit);
#endif /* BCM7271 and PLATFORM_INTEGRATED_WIFI! */

/**
 * This function frees the WL per-device resources.
 *
 * This function frees resources owned by the WL device pointed to
 * by the wl parameter.
 */
void
wl_free(wl_info_t *wl)
{
	wl_timer_t *t, *next;
	osl_t *osh;

	WL_TRACE(("wl: wl_free\n"));
#ifdef SAVERESTORE
	/* need to disable SR before unload the driver
	 * the HW/CLK may be down
	 */
	wlc_iovar_setint(wl->wlc, "sr_enable", 0);
#endif /* SAVERESTORE */
	{
		if (!WLCXO_ENAB(wl->pub) && wl->dev && wl->dev->irq)
			free_irq(wl->dev->irq, wl);
	}

#if defined(WL_CONFIG_RFKILL)
	wl_uninit_rfkill(wl);
#endif

#ifdef NAPI_POLL
	clear_bit(__LINK_STATE_START, &wl->dev->state);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
	napi_disable(&wl->napi);
#elif  LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
	netif_poll_disable(wl->dev);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30) */
#endif /* NAPI_POLL */

	if (wl->dev) {
		wl_free_if(wl, WL_DEV_IF(wl->dev));
		wl->dev = NULL;
	}
	/* free monitor */
	if (wl->monitor_dev) {
		wl_free_if(wl, WL_DEV_IF(wl->monitor_dev));
		wl->monitor_dev = NULL;
	}

#ifdef TOE
	wl_toe_detach(wl->toei);
#endif

#ifndef LINUX_POSTMOGRIFY_REMOVAL
#if defined(P2PO)
	if (wl->p2po)
		wl_p2po_detach(wl->p2po);
	if (wl->disc)
		wl_disc_detach(wl->disc);
#endif /* P2PO */
#if defined(ANQPO)
	if (wl->anqpo)
		wl_anqpo_detach(wl->anqpo);
#endif /* ANQPO */
#if defined(P2PO) || defined(ANQPO)
	if (wl->gas)
		wl_gas_detach(wl->gas);
#endif	/* P2PO || ANQPO */
#ifdef WL_EVENTQ
	if (wl->wlevtq)
		wl_eventq_detach(wl->wlevtq);
#endif /* WL_EVENTQ */
#if defined(BDO) && !defined(BDO_DISABLED)
	if (wl->bdo)
		wl_bdo_detach(wl->bdo);
#endif /* defined(BDO) && !defined(BDO_DISABLED) */
#if defined(TKO)
	if (wl->tko)
		wl_tko_detach(wl->tko);
#endif /* TKO */
#endif /* LINUX_POSTMOGRIFY_REMOVAL */

#ifdef ARPOE
	wl_arp_detach(wl->arpi);
#endif


#ifndef NAPI_POLL
	/* kill dpc */
	tasklet_kill(&wl->tasklet);
#endif

	/* kill tx tasklet */
	tasklet_kill(&wl->tx_tasklet);

	if (wl->pub) {
		wlc_module_unregister(wl->pub, "linux", wl);
	}

	/* free common resources */
	if (wl->wlc) {
#if defined(CONFIG_PROC_FS)
		if ((wl->proc_entry != NULL) && (wl->pub != NULL)) {
			/* remove /proc/net/wl<unit> */
			char tmp[32];

			(void)snprintf(tmp, sizeof(tmp), "net/wl%d", wl->pub->unit);
			tmp[sizeof(tmp) - 1] = '\0';
			WL_INFORM(("wl%d: Removing the proc entry %s \n", wl->pub->unit, tmp));
			remove_proc_entry(tmp, 0);
		}
#endif /* defined(CONFIG_PROC_FS) */
		wlc_detach(wl->wlc);
		wl->wlc = NULL;
		wl->pub = NULL;
	}

	/* virtual interface deletion is deferred so we cannot spinwait */

	/* wait for all pending callbacks to complete */
	while (atomic_read(&wl->callbacks) > 0)
		schedule();

	/* free timers */
	for (t = wl->timers; t; t = next) {
		next = t->next;
		MFREE(wl->osh, t, sizeof(wl_timer_t));
	}

	osh = wl->osh;
	/*
	 * unregister_netdev() calls get_stats() which may read chip registers
	 * so we cannot unmap the chip registers until after calling unregister_netdev() .
	 */
	if (wl->regsva && BUSTYPE(wl->bcm_bustype) != SDIO_BUS &&
	    BUSTYPE(wl->bcm_bustype) != JTAG_BUS) {
		iounmap((void*)wl->regsva);
	}
	wl->regsva = NULL;
	/* move following code under bustype */
#if defined(VASIP_HW_SUPPORT)
	if (wl->bar1_addr) {
		iounmap(wl->bar1_addr);
		wl->bar1_addr = NULL;
	}

	if (wl->bar2_addr) {
		iounmap(wl->bar2_addr);
		wl->bar2_addr = NULL;
	}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#ifdef LINUX_CRYPTO
	/* un register the TKIP module...if any */
	if (wl->tkipmodops != NULL) {
		int idx;
		if (wl->tkip_ucast_data) {
			wl->tkipmodops->deinit(wl->tkip_ucast_data);
			wl->tkip_ucast_data = NULL;
		}
		for (idx = 0; idx < NUM_GROUP_KEYS; idx++) {
			if (wl->tkip_bcast_data[idx]) {
				wl->tkipmodops->deinit(wl->tkip_bcast_data[idx]);
				wl->tkip_bcast_data[idx] = NULL;
			}
		}
	}
#endif /* LINUX_CRYPTO */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14) */

#ifdef WL_ALL_PASSIVE
	if (WL_ALL_PASSIVE_ENAB(wl)) {

#if defined(WLCXO_SIM) && defined(WLCXO_IPC)
		g_d2c_cxo_ctrl_wq = NULL;
#endif

#ifdef WLCXO_IPC
		if (wl->cxo_ctrl_wq) {
			wl_swq_destroy(wl);
			wl->cxo_ctrl_wq = NULL;
		}
#endif
	}
#endif /* WL_ALL_PASSIVE */

	wl_txq_free(wl);

#ifdef WLCXO_SIM
	wl_txhpktq_free(wl);
#endif


#ifdef CTFPOOL
	/* free the buffers in fast pool */
	osl_ctfpool_cleanup(wl->osh);
#endif /* CTFPOOL */

#ifdef HNDCTF
	/* free ctf resources */
	if (wl->cih)
		ctf_detach(wl->cih);
#endif /* HNDCTF */

#ifdef WL_OBJ_REGISTRY
	if (wl->cmn && (obj_registry_unref(wl->objr, OBJR_WL_CMN_INFO) == 0)) {
		obj_registry_set(wl->objr, OBJR_WL_CMN_INFO, NULL);
		MFREE(osh, wl->cmn, sizeof(*(wl->cmn)));
		wl->cmn = NULL;
	}
	if (wl->objr && (obj_registry_unref(wl->objr, OBJR_SELF) == 0)) {
		obj_registry_set(wl->objr, OBJR_SELF, NULL);
		obj_registry_free(wl->objr, osh);
	}
	wl->objr = NULL;
#endif
#ifdef WLCXO_FULL
	if (wl->wl_cxo != NULL) {
		wl_cxo_detach(wl->wl_cxo);
	}
#endif /* WLCXO_FULL */

	MFREE(osh, wl, sizeof(wl_info_t));

#ifdef BCMDBG_CTRACE
	PKT_CTRACE_DUMP(osh, NULL);
#endif
	if (MEMORY_LEFTOVER(osh)) {
		printf("Memory leak of bytes %d\n", MEMORY_LEFTOVER(osh));
		ASSERT(0);
	}


	osl_detach(osh);
} /* wl_free */

static int
wl_open(struct net_device *dev)
{
	wl_info_t *wl;
	int error = 0;

	if (!dev)
		return -ENETDOWN;

	wl = WL_INFO_GET(dev);

	WL_TRACE(("wl%d: wl_open\n", wl->pub->unit));

	WL_LOCK(wl);
	WL_APSTA_UPDN(("wl%d: (%s): wl_open() -> wl_up()\n",
		wl->pub->unit, wl->dev->name));
	/* Since this is resume, reset hw to known state */
	error = wl_up(wl);
	if (!error) {
		error = wlc_set(wl->wlc, WLC_SET_PROMISC, (dev->flags & IFF_PROMISC));
	}
	WL_UNLOCK(wl);

	if (!error)
		OLD_MOD_INC_USE_COUNT;

#if defined(USE_CFG80211)
	if (wl_cfg80211_up(dev)) {
		WL_ERROR(("%s: failed to bring up cfg80211\n", __func__));
		return -1;
	}
#endif
	return (error? -ENODEV : 0);
} /* wl_open */

static int
wl_close(struct net_device *dev)
{
	wl_info_t *wl;

	if (!dev)
		return -ENETDOWN;

#if defined(USE_CFG80211)
	wl_cfg80211_down(dev);
#endif
	wl = WL_INFO_GET(dev);

	WL_TRACE(("wl%s: wl_close\n", dev->name));

	WL_LOCK(wl);
	WL_APSTA_UPDN(("wl(%s): wl_close() -> wl_down()\n", dev->name));
	if (dev == wl->dev) {
		wl_down(wl);
	} else {
		netif_down(dev);
		netif_stop_queue(dev);
	}

	WL_UNLOCK(wl);

	OLD_MOD_DEC_USE_COUNT;

	return (0);
} /* wl_close */

#ifdef ARPOE
/**
 * Return the proper arpi pointer for either corr to an IF or
 *	default. For IF case, Check if arpi is present. It is possible that, upon a
 *	down->arpoe_en->up scenario, interfaces are not reallocated, and
 *	so, wl->arpi could be NULL. If so, allocate it and use.
 */
static wl_arp_info_t *
wl_get_arpi(wl_info_t *wl, struct wl_if *wlif)
{
	if (wlif != NULL) {
		if (wlif->arpi == NULL)
			wlif->arpi = wl_arp_alloc_ifarpi(wl->arpi, wlif->wlcif);
		/* note: this could be null if the above wl_arp_alloc_ifarpi fails */
		return wlif->arpi;
	} else
		return wl->arpi;
}
#endif /* ARPOE */

/** used by the ARPOE module to get the ARPI context */
void * BCMFASTPATH
wl_get_ifctx(struct wl_info *wl, int ctx_id, wl_if_t *wlif)
{
	void *ifctx = NULL;

	switch (ctx_id) {
#ifdef ARPOE
	case IFCTX_ARPI:
		ifctx = (void *)wlif->arpi;
		break;
#endif
	case IFCTX_NETDEV:
		ifctx = (void *)((wlif == NULL) ? wl->dev : wlif->dev);
		break;

	default:
		break;
	}

	return ifctx;
}

/** forwards one or more packets to transmit to the WLC layer */
static int BCMFASTPATH
wl_start_int(wl_info_t *wl, wl_if_t *wlif, struct sk_buff *skb)
{
	void *pkt;

	WL_TRACE(("wl%d: wl_start: len %d data_len %d summed %d csum: 0x%x\n",
		wl->pub->unit, skb->len, skb->data_len, skb->ip_summed, (uint32)skb->csum));

	_WL_LOCK(wl);

	/* Convert the packet. Mainly attach a pkttag */
	pkt = PKTFRMNATIVE(wl->osh, skb);
	ASSERT(pkt != NULL);

#ifdef ARPOE
	/* Arp agent */
	if (ARPOE_ENAB(wl->pub)) {
		wl_arp_info_t *arpi = wl_get_arpi(wl, wlif);
		if (arpi && wl_arp_components_enab()) {
			if (wl_arp_send_proc(arpi, pkt) ==
				ARP_REPLY_HOST) {
				PKTFREE(wl->osh, pkt, TRUE);
				_WL_UNLOCK(wl);
				return 0;
			}
		}
	}
#endif /* ARPOE */

#ifdef TOE
	/* Apply TOE */
	if (TOE_ENAB(wl->pub))
		wl_toe_send_proc(wl->toei, pkt);
#endif

	/* Fix the priority if WME is enabled */
	if (WME_ENAB(wl->pub) && (PKTPRIO(pkt) == 0))
		pktsetprio(pkt, FALSE);

#ifndef LINUX_POSTMOGRIFY_REMOVAL
	/* Mark this pkt as coming from host/bridge. */
	WLPKTTAG(pkt)->flags |= WLF_HOST_PKT;
#endif


#ifdef WLCXO_CTRL
	if (WLCXO_ENAB(wl->pub))
		wlc_cxo_ctrl_sendpkt(wl->wlc, pkt, wlif->wlcif);
	else
#endif
		wlc_sendpkt(wl->wlc, pkt, wlif->wlcif);

	_WL_UNLOCK(wl);

	return (0);
} /* wl_start_int */

void
wl_txflowcontrol(wl_info_t *wl, struct wl_if *wlif, bool state, int prio)
{
	struct net_device *dev;

	ASSERT(prio == ALLPRIO);

	if (wlif == NULL)
		dev = wl->dev;
	else if (!wlif->dev_registered)
		return;
	else
		dev = wlif->dev;

	if (state == ON)
		netif_stop_queue(dev);
	else
		netif_wake_queue(dev);
}

#if defined(AP) || defined(WL_ALL_PASSIVE) || defined(WL_MONITOR)
/** Schedule a completion handler to run at safe time */
int
wl_schedule_task(wl_info_t *wl, void (*fn)(struct wl_task *task), void *context)
{
	wl_task_t *task;

	WL_TRACE(("wl%d: wl_schedule_task\n", wl->pub->unit));

	if (!(task = MALLOC(wl->osh, sizeof(wl_task_t)))) {
		WL_ERROR(("wl%d: wl_schedule_task: out of memory, malloced %d bytes\n",
			wl->pub->unit, MALLOCED(wl->osh)));
		return -ENOMEM;
	}

	MY_INIT_WORK(&task->work, (work_func_t)fn);
	task->context = context;

	if (!SCHEDULE_WORK(wl, &task->work)) {
		WL_ERROR(("wl%d: schedule_work() failed\n", wl->pub->unit));
		MFREE(wl->osh, task, sizeof(wl_task_t));
		return -ENOMEM;
	}

	atomic_inc(&wl->callbacks);

	return 0;
}
#endif /* defined(AP) || defined(WL_ALL_PASSIVE) || defined(WL_MONITOR) */

#ifdef WLCXO_CTRL
typedef struct wl_cx_task {
	struct work_struct	work;
	void			*context;
	wl_info_t		*wl;
	void			(*fn)(void *ctx);
} wl_cx_task_t;

static void
wl_cxo_scheduled_fn(struct work_struct *work)
{
	wl_cx_task_t *task = (wl_cx_task_t *)work;
	void *context = task->context;
	wl_info_t *wl = task->wl;

	WL_WMF(("wl%d: wl_cxo_scheduled_fn\n", wl->pub->unit));
	WL_LOCK(wl);
	task->fn(context);
	WL_UNLOCK(wl);

	MFREE(wl->osh, task, sizeof(*task));
}

int32
wl_cxo_schedule_fn(wl_info_t *wl, void (*fn)(void *ctx), void *context)
{
	wl_cx_task_t *task;

	WL_WMF(("wl%d: wl_schedule_task\n", wl->pub->unit));

	if (!(task = MALLOC(wl->osh, sizeof(*task)))) {
		WL_ERROR(("wl%d: wl_schedule_task: out of memory, malloced %d bytes\n",
			wl->pub->unit, MALLOCED(wl->osh)));
		return -ENOMEM;
	}

	MY_INIT_WORK(&task->work, (work_func_t)wl_cxo_scheduled_fn);
	task->fn = fn;
	task->context = context;
	task->wl = wl;

	if (!SCHEDULE_WORK(wl, &task->work)) {
		WL_ERROR(("wl%d: schedule_work() failed\n", wl->pub->unit));
		MFREE(wl->osh, task, sizeof(*task));
		return -ENOMEM;
	}

	atomic_inc(&wl->callbacks);

	return 0;
}
#endif /* WLCXO_CTRL */

/****************
priv_link is the private struct that we tell netdev about.  This in turn point to a wlif.

Newer kernels require that we call alloc_netdev to alloc the netdev and associated private space
from outside of our lock, which means we need to run it asynchronously in a thread but at
the same time common code wants us to return a pointer synchronously.

Answer is to add a layer of indirection so we MALLOC and return a wlif immediatly (with
wlif->dev = NULL and dev_registered = FALSE) and also spawn a thread to alloc a netdev
and priv_link for private space.  When the netdev_alloc() eventually completes and we hook it
all up.  netdev.priv contains (or points to) priv_link.  priv_link points to wlif.
wlif.dev points back to netdev.

The old way of having netdev.priv contain (or point to) wlif cannot work on newer kernels
since that was called from within our WL_LOCK perimeter lock and we would get a
'could sleep from atomic context' warning from the kernel.
*/

static struct wl_if *
wl_alloc_if(wl_info_t *wl, int iftype, uint subunit, struct wlc_if *wlcif)
{
	wl_if_t *wlif;
	wl_if_t *p;

	/* All kernels get a syncronous wl_if_t.  Older kernels get it populated
	   now, newer kernels get it populated async later
	 */
	if (!(wlif = MALLOCZ(wl->osh, sizeof(wl_if_t)))) {
		WL_ERROR(("wl%d: wl_alloc_if: out of memory, malloced %d bytes\n",
			(wl->pub)?wl->pub->unit:subunit, MALLOCED(wl->osh)));
		return NULL;
	}
	wlif->wl = wl;
	wlif->wlcif = wlcif;
	wlif->subunit = subunit;
	wlif->if_type = iftype;

	/* add the interface to the interface linked list */
	if (wl->if_list == NULL)
		wl->if_list = wlif;
	else {
		p = wl->if_list;
		while (p->next != NULL)
			p = p->next;
		p->next = wlif;
	}

#ifdef ARPOE
	/* create and populate arpi for this IF */
	if (ARPOE_ENAB(wl->pub))
		wlif->arpi = wl_arp_alloc_ifarpi(wl->arpi, wlcif);
#endif /* ARPOE */

	return wlif;
} /* wl_alloc_if */

static void
wl_free_if(wl_info_t *wl, wl_if_t *wlif)
{
	wl_if_t *p;
	ASSERT(wlif);
	ASSERT(wl);

	WL_TRACE(("%s\n", __FUNCTION__));

	/* check if register_netdev was successful */
	if (wlif->dev_registered) {
		ASSERT(wlif->dev);


#ifdef HNDCTF
		if (wl->cih)
			ctf_dev_unregister(wl->cih, wlif->dev);
#endif /* HNDCTF */
		unregister_netdev(wlif->dev);
		wlif->dev_registered = FALSE;
	}

#if defined(USE_CFG80211)
	wl_cfg80211_detach(wl_get_cfg(wlif->dev));
#endif
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

	if (wlif->dev) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24))
		MFREE(wl->osh, wlif->dev->priv, sizeof(priv_link_t));
		MFREE(wl->osh, wlif->dev, sizeof(struct net_device));
#else
		free_netdev(wlif->dev);
#endif /* 2.6.24 */
	}

#ifdef ARPOE
	/* free arpi for this IF */
	if (ARPOE_ENAB(wl->pub))
		wl_arp_free_ifarpi(wlif->arpi);
#endif /* ARPOE */

	MFREE(wl->osh, wlif, sizeof(wl_if_t));
} /* wl_free_if */

/**
 * Create a virtual interface. Call only from safe time!
 * can't call register_netdev with WL_LOCK
 *
 * Netdev allocator.  Only newer kernels need this to be async
 * but we'll run it async for all kernels for ease of maintenance.
 *
 * Sets:  wlif->dev & dev->priv_link->wlif
 */
static struct net_device *
wl_alloc_linux_if(wl_if_t *wlif)
{
	wl_info_t *wl = wlif->wl;
	struct net_device *dev;
	priv_link_t *priv_link;

	WL_TRACE(("%s\n", __FUNCTION__));
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24))
	dev = MALLOCZ(wl->osh, sizeof(struct net_device));
	if (!dev) {
		WL_ERROR(("wl%d: %s: malloc of net_device failed\n",
			(wl->pub)?wl->pub->unit:wlif->subunit, __FUNCTION__));
		return NULL;
	}
	ether_setup(dev);

	strncpy(dev->name, intf_name, IFNAMSIZ-1);
	dev->name[IFNAMSIZ-1] = '\0';

	priv_link = MALLOC(wl->osh, sizeof(priv_link_t));
	if (!priv_link) {
		WL_ERROR(("wl%d: %s: malloc of priv_link failed\n",
			(wl->pub)?wl->pub->unit:wlif->subunit, __FUNCTION__));
		MFREE(wl->osh, dev, sizeof(struct net_device));
		return NULL;
	}
	dev->priv = priv_link;
#else
	/* KERNEL >= 2.6.24 */
	/*
	 * Use osl_malloc for our own wlif priv area wl_if and use the netdev->priv area only
	 * as a pointer to our wl_if *.
	 */

	/* Allocate net device, including space for private structure */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0))
	dev = alloc_netdev(sizeof(priv_link_t), intf_name, NET_NAME_UNKNOWN, ether_setup);
#else
	dev = alloc_netdev(sizeof(priv_link_t), intf_name, ether_setup);
#endif
	if (!dev) {
		WL_ERROR(("wl%d: %s: alloc_netdev failed\n",
			(wl->pub)?wl->pub->unit:wlif->subunit, __FUNCTION__));
		return NULL;
	}
	priv_link = netdev_priv(dev);
	if (!priv_link) {
		WL_ERROR(("wl%d: %s: cannot get netdev_priv\n",
			(wl->pub)?wl->pub->unit:wlif->subunit, __FUNCTION__));
		return NULL;
	}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24) */

	/* Connect wlif and netdev */
	priv_link->wlif = wlif;
	wlif->dev = dev;


	/* match current flow control state */
	if (wlif->if_type != WL_IFTYPE_MON && wl->dev && netif_queue_stopped(wl->dev))
		netif_stop_queue(dev);

	return dev;
} /* wl_alloc_linux_if */

static wlc_bsscfg_t *
wl_bsscfg_find(wl_if_t *wlif)
{
	wl_info_t *wl = wlif->wl;
	return wlc_bsscfg_find_by_wlcif(wl->wlc, wlif->wlcif);
}



/** !LINUXSIM specific */
static void
_wl_add_if(wl_task_t *task)
{
	wl_if_t *wlif = (wl_if_t *)task->context;
	wl_info_t *wl = wlif->wl;
	struct net_device *dev;
	wlc_bsscfg_t *cfg;

	WL_TRACE(("%s\n", __FUNCTION__));

	/* alloc_netdev and populate priv_link */
	if ((dev = wl_alloc_linux_if(wlif)) == NULL) {
		WL_ERROR(("%s: Call to  wl_alloc_linux_if failed\n", __FUNCTION__));
		goto done;
	}

	/* Copy temp to real name */
	ASSERT(strlen(wlif->name) > 0);
	strncpy(wlif->dev->name, wlif->name, strlen(wlif->name) + 1);

#if defined(WL_USE_NETDEV_OPS)
	dev->netdev_ops = &wl_netdev_ops;
#else /* WL_USE_NETDEV_OPS */
#ifdef WL_THREAD
	dev->hard_start_xmit = wl_start_wlthread;
#else
	dev->hard_start_xmit = wl_start;
#endif
	dev->do_ioctl = wl_ioctl;
	dev->set_mac_address = wl_set_mac_address;
	dev->get_stats = wl_get_stats;
#endif /* WL_USE_NETDEV_OPS */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
	dev->ethtool_ops = &wl_ethtool_ops;
#endif

	cfg = wl_bsscfg_find(wlif);
	ASSERT(cfg != NULL);

	bcopy(&cfg->cur_etheraddr, dev->dev_addr, ETHER_ADDR_LEN);

	if (register_netdev(dev)) {
		WL_ERROR(("wl%d: wl_add_if: register_netdev() failed for \"%s\"\n",
			wl->pub->unit, dev->name));
		goto done;
	}
	wlif->dev_registered = TRUE;


#ifdef HNDCTF
	if ((ctf_dev_register(wl->cih, dev, FALSE) != BCME_OK) ||
	    (ctf_enable(wl->cih, dev, TRUE, NULL) != BCME_OK)) {
		ctf_dev_unregister(wl->cih, dev);
		WL_ERROR(("wl%d: ctf_dev_register() failed\n", wl->pub->unit));
		goto done;
	}
#endif /* HNDCTF */

done:
	MFREE(wl->osh, task, sizeof(wl_task_t));
	atomic_dec(&wl->callbacks);
} /* _wl_add_if */

/** Schedule _wl_add_if() to be run at safe time. */
struct wl_if *
wl_add_if(wl_info_t *wl, struct wlc_if *wlcif, uint unit, struct ether_addr *remote)
{
	wl_if_t *wlif;
	int iftype;
	const char *devname;

	WL_TRACE(("%s\n", __FUNCTION__));
	if (remote) {
		iftype = WL_IFTYPE_WDS;
		devname = "wds";
	} else {
		iftype = WL_IFTYPE_BSS;
		devname = "wl";
	}

	wlif = wl_alloc_if(wl, iftype, unit, wlcif);

	if (!wlif) {
		WL_ERROR(("wl%d: wl_add_if: failed to create %s interface %d\n",
			wl->pub->unit, (remote)?"WDS":"BSS", unit));
		return NULL;
	}

	/* netdev isn't ready yet so stash name here for now and
	   copy into netdev when it becomes ready
	 */
	(void)snprintf(wlif->name, sizeof(wlif->name), "%s%d.%d", devname, wl->pub->unit,
	               wlif->subunit);

	if (wl_schedule_task(wl, _wl_add_if, wlif)) {
		MFREE(wl->osh, wlif, sizeof(wl_if_t) + sizeof(struct net_device));
		return NULL;
	}

	return wlif;
} /* wl_add_if */

/** Remove a virtual interface. Call only from safe time! */
static void
_wl_del_if(wl_task_t *task)
{
	wl_if_t *wlif = (wl_if_t *) task->context;
	wl_info_t *wl = wlif->wl;

	wl_free_if(wl, wlif);

	MFREE(wl->osh, task, sizeof(wl_task_t));
	atomic_dec(&wl->callbacks);
}

#ifdef WLRSDB
/** RSDB specific function. Update the wl pointer for RSDB bsscfg Move */
void
wl_update_if(struct wl_info *from_wl, struct wl_info *to_wl, wl_if_t *from_wlif,
	struct wlc_if *to_wlcif)
{
	ASSERT(to_wl != NULL);
	ASSERT(to_wlcif != NULL);
	if (from_wlif) {
		from_wlif->wl = to_wl;
		from_wlif->wlcif = to_wlcif;
	}
#ifdef WL_DUALNIC_RSDB
	else {
		struct wlc_if *from_wlcif;
		struct net_device *dev;
		from_wlif = ((priv_link_t*)netdev_priv(from_wl->dev))->wlif;
		from_wlif->wl = to_wl;
		from_wlcif = from_wlif->wlcif;
		from_wlif->wlcif = to_wlcif;

		from_wlif = ((priv_link_t*)netdev_priv(to_wl->dev))->wlif;
		from_wlif->wl = from_wl;
		from_wlif->wlcif = from_wlcif;

		dev = from_wl->dev;
		from_wl->dev = to_wl->dev;
		to_wl->dev = dev;
	}
#endif /* WL_DUALNIC_RSDB */
} /* wl_update_if */
#endif /* WLRSDB */

/** Schedule _wl_del_if() to be run at safe time. */
void
wl_del_if(wl_info_t *wl, wl_if_t *wlif)
{
	ASSERT(wlif != NULL);
	ASSERT(wlif->wl == wl);

	wlif->wlcif = NULL;

	if (wl_schedule_task(wl, _wl_del_if, wlif)) {
		WL_ERROR(("wl%d: wl_del_if: schedule_task() failed\n", wl->pub->unit));
		return;
	}
}

#ifdef WLCXO
#ifndef WLCXO_IPC
/* Return pointer to lock var */
void *
wl_lockvar(wl_info_t *wl)
{
	return (void *)&wl->sem;
}
#endif /* WLCXO_IPC */

#ifdef HNDCTF
/* Return pointer to lock var */
ctf_t *
wl_cihvar(wl_info_t *wl)
{
	return wl->cih;
}
#endif /* HNDCTF */
#endif /* WLCXO */

/** Return pointer to interface name */
char *
wl_ifname(wl_info_t *wl, wl_if_t *wlif)
{
	if (wlif) {
		return wlif->name;
	} else {
		return wl->dev->name;
	}
}

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
	uint32 macintmask;

	WL_TRACE(("wl%d: wl_reset\n", wl->pub->unit));

	/* disable interrupts */
	macintmask = wl_intrsoff(wl);

	wlc_reset(wl->wlc);

	/* restore macintmask */
	wl_intrsrestore(wl, macintmask);

	/* dpc will not be rescheduled */
	wl->resched = 0;

	return (0);
}

/**
 * These are interrupt on/off entry points. Disable interrupts
 * during interrupt state transition.
 */
void BCMFASTPATH
wl_intrson(wl_info_t *wl)
{
	if (WLCXO_ENAB(wl->pub))
		wlc_intrson(wl->wlc);
	else {
		unsigned long flags = 0;

		INT_LOCK(wl, flags);
		wlc_intrson(wl->wlc);
		INT_UNLOCK(wl, flags);
	}

#ifdef BCM7271
#ifdef STB_SOC_WIFI
		wl_soc_wifi_enable_intrs(wl);
#else /* STB_SOC_WIFI */
		wl_bcm7xxx_interrupt_enable();
#endif /* STB_SOC_WIFI  */
#endif /* BCM7271 */

}

bool
wl_alloc_dma_resources(wl_info_t *wl, uint addrwidth)
{
	return TRUE;
}

uint32 BCMFASTPATH
wl_intrsoff(wl_info_t *wl)
{
	unsigned long flags = 0;
	uint32 status;

	if (WLCXO_ENAB(wl->pub))
		status = wlc_intrsoff(wl->wlc);
	else {
		INT_LOCK(wl, flags);
		status = wlc_intrsoff(wl->wlc);
		INT_UNLOCK(wl, flags);
	}

#ifdef BCM7271
#ifdef STB_SOC_WIFI
		wl_soc_wifi_disable_intrs(wl);
#else /* STB_SOC_WIFI */
		wl_bcm7xxx_interrupt_disable();
#endif /* STB_SOC_WIFI  */
#endif /* BCM7271 */

	return status;
}

void
wl_intrsrestore(wl_info_t *wl, uint32 macintmask)
{
	if (WLCXO_ENAB(wl->pub))
		wlc_intrsrestore(wl->wlc, macintmask);
	else {
		unsigned long flags = 0;

		INT_LOCK(wl, flags);
		wlc_intrsrestore(wl->wlc, macintmask);
		INT_UNLOCK(wl, flags);
	}
}

int
wl_up(wl_info_t *wl)
{
	int error = 0;
	wl_if_t *wlif;

	WL_TRACE(("wl%d: wl_up\n", wl->pub->unit));

	if (wl->pub->up)
		return (0);

	error = wlc_up(wl->wlc);

	/* wake (not just start) all interfaces */
	if (!error) {
		for (wlif = wl->if_list; wlif != NULL; wlif = wlif->next) {
			wl_txflowcontrol(wl, wlif, OFF, ALLPRIO);
		}
	}

#ifdef NAPI_POLL
	set_bit(__LINK_STATE_START, &wl->dev->state);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
	/* use d11 h/w rng to add some entropy to linux. */
#define WL_LINUX_RBUF_SZ 16
	if (!error) {
		uint8 rbuf[WL_LINUX_RBUF_SZ];
		if (wlc_getrand(wl->wlc, rbuf, sizeof(rbuf)) == BCME_OK) {
			WL_WSEC(("wl%d: updating linux rng w/ wlc random data\n", wl->pub->unit));
			add_device_randomness(rbuf, sizeof(rbuf));
		}
	}
#endif /* linuxver >= 3.10.0 */

	return (error);
} /* wl_up */

void
wl_down(wl_info_t *wl)
{
	wl_if_t *wlif;
	int monitor = 0;
	uint callbacks, ret_val = 0;

	WL_TRACE(("wl%d: wl_down\n", wl->pub->unit));

	for (wlif = wl->if_list; wlif != NULL; wlif = wlif->next) {
		if (wlif->dev) {
			netif_down(wlif->dev);
			netif_stop_queue(wlif->dev);
		}
	}

	if (wl->monitor_dev) {
		ret_val = wlc_ioctl(wl->wlc, WLC_SET_MONITOR, &monitor, sizeof(int), NULL);
		if (ret_val != BCME_OK) {
			WL_ERROR(("%s: Disabling MONITOR failed %d\n", __FUNCTION__, ret_val));
		}
	}


	/* call common down function */
	if (wl->wlc)
		ret_val = wlc_down(wl->wlc);

	callbacks = atomic_read(&wl->callbacks) - ret_val;
	BCM_REFERENCE(callbacks);

	/* wait for down callbacks to complete */
	WL_UNLOCK(wl);

#ifdef WL_ALL_PASSIVE
	if (WL_ALL_PASSIVE_ENAB(wl)) {
		int i = 0;
		for (i = 0; (atomic_read(&wl->callbacks) > callbacks) && i < 10000; i++) {
			schedule();
			flush_scheduled_work();
		}
	} else
#endif /* WL_ALL_PASIVE */
	{
		/* For HIGH_only driver, it's important to actually schedule other work,
		 * not just spin wait since everything runs at schedule level
		 */
		SPINWAIT((atomic_read(&wl->callbacks) > callbacks), 100 * 1000);
	}

	WL_LOCK(wl);
} /* wl_down */

/* Retrieve current toe component enables, which are kept as a bitmap in toe_ol iovar */
static int
wl_toe_get(wl_info_t *wl, uint32 *toe_ol)
{
	if (wlc_iovar_getint(wl->wlc, "toe_ol", toe_ol) != 0)
		return -EOPNOTSUPP;

	return 0;
}

/* Set current toe component enables in toe_ol iovar, and set toe global enable iovar */
static int
wl_toe_set(wl_info_t *wl, uint32 toe_ol)
{
	if (wlc_iovar_setint(wl->wlc, "toe_ol", toe_ol) != 0)
		return -EOPNOTSUPP;

	/* Enable toe globally only if any components are enabled. */

	if (wlc_iovar_setint(wl->wlc, "toe", (toe_ol != 0)) != 0)
		return -EOPNOTSUPP;

	return 0;
}

static void
wl_get_driver_info(struct net_device *dev, struct ethtool_drvinfo *info)
{
	wl_info_t *wl = WL_INFO_GET(dev);

#if WIRELESS_EXT >= 19 || LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	if (!wl || !wl->pub || !wl->wlc || !wl->dev)
		return;
#endif
	bzero(info, sizeof(struct ethtool_drvinfo));
	(void)snprintf(info->driver, sizeof(info->driver), "wl%d", wl->pub->unit);
	strncpy(info->version, EPI_VERSION_STR, sizeof(info->version));
	info->version[(sizeof(info->version))-1] = '\0';
}

#ifdef WLCSO
static int
wl_set_tx_csum(struct net_device *dev, uint32 on_off)
{
	wl_info_t *wl = WL_INFO_GET(dev);

	wlc_set_tx_csum(wl->wlc, on_off);
	if (on_off)
		dev->features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
	else
		dev->features &= ~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);

	return 0;
}
#endif

static int
wl_ethtool(wl_info_t *wl, void *uaddr, wl_if_t *wlif)
{
	struct ethtool_drvinfo info;
	struct ethtool_value edata;
	uint32 cmd;
	uint32 toe_cmpnt = 0, csum_dir;
	int ret;

	if (!wl || !wl->pub || !wl->wlc)
		return -ENODEV;

	/* skip this trace in emulator builds since it happens every second */
	WL_TRACE(("wl%d: %s\n", wl->pub->unit, __FUNCTION__));

	if (copy_from_user(&cmd, uaddr, sizeof(uint32)))
		return (-EFAULT);

	switch (cmd) {
	case ETHTOOL_GDRVINFO:
		if (!wl->dev)
			return -ENETDOWN;

		wl_get_driver_info(wl->dev, &info);
		info.cmd = cmd;
		if (copy_to_user(uaddr, &info, sizeof(info)))
			return (-EFAULT);
		break;

	/* Get toe offload components */
	case ETHTOOL_GRXCSUM:
	case ETHTOOL_GTXCSUM:
		if ((ret = wl_toe_get(wl, &toe_cmpnt)) < 0)
			return ret;

		csum_dir = (cmd == ETHTOOL_GTXCSUM) ? TOE_TX_CSUM_OL : TOE_RX_CSUM_OL;

		edata.cmd = cmd;
		edata.data = (toe_cmpnt & csum_dir) ? 1 : 0;

		if (copy_to_user(uaddr, &edata, sizeof(edata)))
			return (-EFAULT);
		break;

	/* Set toe offload components */
	case ETHTOOL_SRXCSUM:
	case ETHTOOL_STXCSUM:
		if (copy_from_user(&edata, uaddr, sizeof(edata)))
			return (-EFAULT);

		/* Read the current settings, update and write back */
		if ((ret = wl_toe_get(wl, &toe_cmpnt)) < 0)
			return ret;

		csum_dir = (cmd == ETHTOOL_STXCSUM) ? TOE_TX_CSUM_OL : TOE_RX_CSUM_OL;

		if (edata.data != 0)
			toe_cmpnt |= csum_dir;
		else
			toe_cmpnt &= ~csum_dir;

		if ((ret = wl_toe_set(wl, toe_cmpnt)) < 0)
			return ret;

		/* If setting TX checksum mode, tell Linux the new mode */
		if (cmd == ETHTOOL_STXCSUM) {
			if (!wl->dev)
				return -ENETDOWN;
			if (edata.data)
				wl->dev->features |= NETIF_F_IP_CSUM;
			else
				wl->dev->features &= ~NETIF_F_IP_CSUM;
		}

		break;

	default:
		return (-EOPNOTSUPP);

	}

	return (0);
} /* wl_ethtool */

int
wl_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	wl_info_t *wl;
	wl_if_t *wlif;
	void *buf = NULL;
	wl_ioctl_t ioc;
	int bcmerror;
#ifdef WLRSDB
	int lock_all_wl = 0;
	int idx;
	wl_info_t *wl_iter;
#endif /* WLRSDB */

	if (!dev)
		return -ENETDOWN;

	wl = WL_INFO_GET(dev);
	wlif = WL_DEV_IF(dev);
	if (wlif == NULL || wl == NULL || wl->dev == NULL)
		return -ENETDOWN;

	bcmerror = 0;


	WL_TRACE(("wl%d: wl_ioctl: cmd 0x%x\n", wl->pub->unit, cmd));

#ifdef CONFIG_PREEMPT
	if (preempt_count())
		WL_ERROR(("wl%d: wl_ioctl: cmd = 0x%x, preempt_count=%d\n",
			wl->pub->unit, cmd, preempt_count()));
#endif

#ifdef USE_IW
	/* linux wireless extensions */
	if ((cmd >= SIOCIWFIRST) && (cmd <= SIOCIWLAST)) {
		/* may recurse, do NOT lock */
		return wl_iw_ioctl(dev, ifr, cmd);
	}
#endif /* USE_IW */

	if (cmd == SIOCETHTOOL)
		return (wl_ethtool(wl, (void*)ifr->ifr_data, wlif));

#if defined(OEM_ANDROID)
	if (cmd == SIOCDEVPRIVATE+1)
		return wl_android_priv_cmd(dev, ifr, cmd);
#endif /* OEM_ANDROID */

	switch (cmd) {
		case SIOCDEVPRIVATE :
			break;
		default:
			bcmerror = BCME_UNSUPPORTED;
			goto done2;
	}

	if (copy_from_user(&ioc, ifr->ifr_data, sizeof(wl_ioctl_t))) {
		bcmerror = BCME_BADADDR;
		goto done2;
	}

	/* optimization for direct ioctl calls from kernel */
	if (segment_eq(get_fs(), KERNEL_DS))
		buf = ioc.buf;

	else if (ioc.buf) {
		if (!(buf = (void *) MALLOC(wl->osh, MAX(ioc.len, WLC_IOCTL_MAXLEN)))) {
			bcmerror = BCME_NORESOURCE;
			goto done2;
		}

		if (copy_from_user(buf, ioc.buf, ioc.len)) {
			bcmerror = BCME_BADADDR;
			goto done1;
		}
	}

	WL_LOCK(wl);
#ifdef WLRSDB
	/*
	 * oper_mode iovar require both the perimeter locks to be taken. This iovar checks the
	 * possibility of upgrading to MIMO and if that happens will bring down the second wlc.
	 * So, locking the second wl is also required.
	 */

	if (wl->cmn && wl->cmn->wl[1] && (wl == wl->cmn->wl[0]) && buf) {
		if (!strcmp((char *)buf, "oper_mode")) {
			lock_all_wl = 1;
		}
	}
	if (lock_all_wl) {
		FOREACH_WL(wl->cmn, idx, wl_iter) {
			if (wl_iter != wl) {
				WL_LOCK(wl_iter);
			}
		}
	}
#endif /* WLRSDB */
	if (!capable(CAP_NET_ADMIN)) {
		bcmerror = BCME_EPERM;
	} else {
		bcmerror = wlc_ioctl(wl->wlc, ioc.cmd, buf, ioc.len, wlif->wlcif);
	}
#ifdef WLRSDB
	if (lock_all_wl) {
		FOREACH_WL(wl->cmn, idx, wl_iter) {
			if (wl_iter != wl) {
				WL_UNLOCK(wl_iter);
			}
		}
	}
#endif /* WLRSDB */
	WL_UNLOCK(wl);

done1:
	if (ioc.buf && (ioc.buf != buf)) {
		if (copy_to_user(ioc.buf, buf, ioc.len))
			bcmerror = BCME_BADADDR;
		MFREE(wl->osh, buf, MAX(ioc.len, WLC_IOCTL_MAXLEN));
	}

done2:
	ASSERT(VALID_BCMERROR(bcmerror));
	if (bcmerror != 0)
		wl->pub->bcmerror = bcmerror;

	return (OSL_ERROR(bcmerror));
} /* wl_ioctl */

static struct net_device_stats*
wl_get_stats(struct net_device *dev)
{
	struct net_device_stats *stats_watchdog = NULL;
	struct net_device_stats *stats = NULL;
	wl_info_t *wl;
	wl_if_t *wlif;

	if (!dev)
		return NULL;

	if ((wl = WL_INFO_GET(dev)) == NULL)
		return NULL;

	if ((wlif = WL_DEV_IF(dev)) == NULL)
		return NULL;

	if ((stats = &wlif->stats) == NULL)
		return NULL;

	/* skip this trace in emulator builds since it happens every second */
	WL_TRACE(("wl%d: wl_get_stats\n", wl->pub->unit));

	ASSERT(wlif->stats_id < 2);
	stats_watchdog = &wlif->stats_watchdog[wlif->stats_id];
	memcpy(stats, stats_watchdog, sizeof(struct net_device_stats));
	return (stats);
}

#ifdef USE_IW
struct iw_statistics *
wl_get_wireless_stats(struct net_device *dev)
{
	int res = 0;
	wl_info_t *wl;
	wl_if_t *wlif;
	struct iw_statistics *wstats = NULL;
	struct iw_statistics *wstats_watchdog = NULL;
	int phy_noise, rssi;

	if (!dev)
		return NULL;

	if ((wl = WL_INFO_GET(dev)) == NULL)
		return NULL;

	if ((wlif = WL_DEV_IF(dev)) == NULL)
		return NULL;

	if ((wstats = &wlif->wstats) == NULL)
		return NULL;

	WL_TRACE(("wl%d: wl_get_wireless_stats\n", wl->pub->unit));

	ASSERT(wlif->stats_id < 2);
	wstats_watchdog = &wlif->wstats_watchdog[wlif->stats_id];

	phy_noise = wlif->phy_noise;
#if WIRELESS_EXT > 11
	wstats->discard.nwid = 0;
	wstats->discard.code = wstats_watchdog->discard.code;
	wstats->discard.fragment = wstats_watchdog->discard.fragment;
	wstats->discard.retries = wstats_watchdog->discard.retries;
	wstats->discard.misc = wstats_watchdog->discard.misc;

	wstats->miss.beacon = 0;
#endif /* WIRELESS_EXT > 11 */

	/* RSSI measurement is somewhat meaningless for AP in this context */
	if (AP_ENAB(wl->pub))
		rssi = 0;
	else {
		scb_val_t scb;
		res = wlc_ioctl(wl->wlc, WLC_GET_RSSI, &scb, sizeof(int), wlif->wlcif);
		if (res) {
			WL_ERROR(("wl%d: %s: WLC_GET_RSSI failed (%d)\n",
				wl->pub->unit, __FUNCTION__, res));
			return NULL;
		}
		rssi = scb.val;
	}

	if (rssi <= WLC_RSSI_NO_SIGNAL)
		wstats->qual.qual = 0;
	else if (rssi <= WLC_RSSI_VERY_LOW)
		wstats->qual.qual = 1;
	else if (rssi <= WLC_RSSI_LOW)
		wstats->qual.qual = 2;
	else if (rssi <= WLC_RSSI_GOOD)
		wstats->qual.qual = 3;
	else if (rssi <= WLC_RSSI_VERY_GOOD)
		wstats->qual.qual = 4;
	else
		wstats->qual.qual = 5;

	/* Wraps to 0 if RSSI is 0 */
	wstats->qual.level = 0x100 + rssi;
	wstats->qual.noise = 0x100 + phy_noise;
#if WIRELESS_EXT > 18
	wstats->qual.updated |= (IW_QUAL_ALL_UPDATED | IW_QUAL_DBM);
#else
	wstats->qual.updated |= 7;
#endif /* WIRELESS_EXT > 18 */

	return wstats;
} /* wl_get_wireless_stats */
#endif /* USE_IW */

static int
wl_set_mac_address(struct net_device *dev, void *addr)
{
	int err = 0;
	wl_info_t *wl;
	struct sockaddr *sa = (struct sockaddr *) addr;

	if (!dev)
		return -ENETDOWN;

	wl = WL_INFO_GET(dev);

	WL_TRACE(("wl%d: wl_set_mac_address\n", wl->pub->unit));

	WL_LOCK(wl);

	bcopy(sa->sa_data, dev->dev_addr, ETHER_ADDR_LEN);
	err = wlc_iovar_op(wl->wlc, "cur_etheraddr", NULL, 0, sa->sa_data, ETHER_ADDR_LEN,
		IOV_SET, (WL_DEV_IF(dev))->wlcif);
	WL_UNLOCK(wl);
	if (err)
		WL_ERROR(("wl%d: wl_set_mac_address: error setting MAC addr override\n",
			wl->pub->unit));
	return err;
}

static void
wl_set_multicast_list(struct net_device *dev)
{
	if (!WL_ALL_PASSIVE_ENAB((wl_info_t *)WL_INFO_GET(dev)))
		_wl_set_multicast_list(dev);
#ifdef WL_ALL_PASSIVE
	else {
		wl_info_t *wl = WL_INFO_GET(dev);
		wl->multicast_task.context = dev;

		if (SCHEDULE_WORK(wl, &wl->multicast_task.work)) {
			/* work item may already be on the work queue, so only inc callbacks if
			 * we actually schedule a new item
			 */
			atomic_inc(&wl->callbacks);
		}
	}
#endif /* WL_ALL_PASSIVE */
}

static void
_wl_set_multicast_list(struct net_device *dev)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 34)
	struct dev_mc_list *mclist;
#else
	struct netdev_hw_addr *ha;
#endif
	wl_info_t *wl;
	int i, buflen;
	struct maclist *maclist;
	int allmulti;

	if (!dev)
		return;
	wl = WL_INFO_GET(dev);

	WL_TRACE(("wl%d: wl_set_multicast_list\n", wl->pub->unit));


	if (wl->pub->up) {
		allmulti = (dev->flags & IFF_ALLMULTI)? TRUE: FALSE;


		buflen = sizeof(struct maclist) + (MAXMULTILIST * ETHER_ADDR_LEN);

		if ((maclist = MALLOC(wl->pub->osh, buflen)) == NULL) {
			return;
		}

		/* copy the list of multicasts into our private table */
		i = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 34)
		for (mclist = dev->mc_list; mclist && (i < dev->mc_count); mclist = mclist->next) {
			if (i >= MAXMULTILIST) {
				allmulti = TRUE;
				i = 0;
				break;
			}
			bcopy(mclist->dmi_addr, &maclist->ea[i++], ETHER_ADDR_LEN);
		}
#else
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wcast-qual\"")
#endif
		netdev_for_each_mc_addr(ha, dev)
		{
			if (i >= MAXMULTILIST) {
				allmulti = TRUE;
				i = 0;
				break;
			}
			bcopy(ha->addr, &maclist->ea[i++], ETHER_ADDR_LEN);
		}
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic pop")
#endif
#endif /* LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 34) */
		maclist->count = i;

		WL_LOCK(wl);

		/* update ALL_MULTICAST common code flag */
		wlc_iovar_op(wl->wlc, "allmulti", NULL, 0, &allmulti, sizeof(allmulti), IOV_SET,
			(WL_DEV_IF(dev))->wlcif);
		wlc_set(wl->wlc, WLC_SET_PROMISC, (dev->flags & IFF_PROMISC));

		/* set up address filter for multicasting */
		wlc_iovar_op(wl->wlc, "mcast_list", NULL, 0, maclist, buflen, IOV_SET,
			(WL_DEV_IF(dev))->wlcif);

		WL_UNLOCK(wl);
		MFREE(wl->pub->osh, maclist, buflen);
	}
} /* _wl_set_multicast_list */


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
irqreturn_t BCMFASTPATH
wl_isr(int irq, void *dev_id)
#else
irqreturn_t BCMFASTPATH
wl_isr(int irq, void *dev_id, struct pt_regs *ptregs)
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20) */
{
	wl_info_t *wl;
	bool ours, wantdpc;
	unsigned long flags;

	wl = (wl_info_t*) dev_id;

	WL_ISRLOCK(wl, flags);

#ifdef BCM7271
#ifdef STB_SOC_WIFI
	wl_soc_wifi_d2h_isr(wl);
#else /* STB_SOC_WIFI */
	wl_bcm7xxx_d2h_int2_isr();
#endif /* STB_SOC_WIFI  */
#endif /* BCM7271 */

	/* call common first level interrupt handler */
	if ((ours = wlc_isr(wl->wlc, &wantdpc))) {
		/* if more to do... */
		if (wantdpc) {

			/* ...and call the second level interrupt handler */
			/* schedule dpc */
			ASSERT(wl->resched == FALSE);
#ifdef NAPI_POLL
			/* allow the device to be added to the cpu polling
			 * list if we are up
			 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
			napi_schedule(&wl->napi);
#else
			netif_rx_schedule(wl->dev);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30) */
#else /* NAPI_POLL */
#ifdef WL_ALL_PASSIVE
			if (WL_ALL_PASSIVE_ENAB(wl)) {
				if (SCHEDULE_WORK(wl, &wl->wl_dpc_task.work))
					atomic_inc(&wl->callbacks);
				else
					ASSERT(0);
			} else
#endif /* WL_ALL_PASSIVE */
			tasklet_schedule(&wl->tasklet);
#endif /* NAPI_POLL */
		}
	}

#ifdef STB_SOC_WIFI
	/* re-enable device to host interrupt */
	wl_soc_wifi_d2h_intstatus(wl);
#endif /* STB_SOC_WIFI */

	WL_ISRUNLOCK(wl, flags);

	return IRQ_RETVAL(ours);
} /* wl_isr */

#ifdef NAPI_POLL
static int BCMFASTPATH
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
wl_poll(struct napi_struct *napi, int budget)
#else
wl_poll(struct net_device *dev, int *budget)
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30) */
#else /* NAPI_POLL */
static void BCMFASTPATH
wl_dpc(ulong data)
#endif /* NAPI_POLL */
{
	wl_info_t *wl;

#ifdef NAPI_POLL
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
	wl = (wl_info_t *)container_of(napi, wl_info_t, napi);
	wl->pub->tunables->rxbnd = min(RXBND, budget);
#else
	wl = WL_INFO_GET(dev);
	wl->pub->tunables->rxbnd = min(RXBND, *budget);
	ASSERT(wl->pub->tunables->rxbnd <= dev->quota);
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30) */
#else /* NAPI_POLL */

	wl = (wl_info_t *)data;

	WL_LOCK(wl);
#endif /* NAPI_POLL */

#ifdef WLCXO_CTRL
	ASSERT(!WLCXO_ENAB(wl->pub));
#endif

	/* call the common second level interrupt handler */
	if (wl->pub->up) {
		wlc_dpc_info_t dpci = {0, 0};

		if (wl->resched) {
			unsigned long flags = 0;
			INT_LOCK(wl, flags);
			wlc_intrsupd(wl->wlc);
			INT_UNLOCK(wl, flags);
		}
		wl->resched = wlc_dpc(wl->wlc, TRUE, &dpci);
		wl->processed = dpci.processed;
	}

	/* wlc_dpc() may bring the driver down */
	if (!wl->pub->up) {
#ifdef WL_ALL_PASSIVE
		/* Reenable wl_dpc_task to be dispatch */
		if ((WL_ALL_PASSIVE_ENAB(wl))) {
			atomic_dec(&wl->callbacks);
		}
#endif /* WL_ALL_PASSIVE */
		goto done;
	}

#ifndef NAPI_POLL
#ifdef WL_ALL_PASSIVE
	if (wl->resched) {
		if (!(WL_ALL_PASSIVE_ENAB(wl)))
			tasklet_schedule(&wl->tasklet);
		else
			if (!SCHEDULE_WORK(wl, &wl->wl_dpc_task.work)) {
				/* wl_dpc_task alread in queue.
				 * Shall not reach here
				 */
				ASSERT(0);
			}
	} else {
		/* re-enable interrupts */
		if (WL_ALL_PASSIVE_ENAB(wl))
			atomic_dec(&wl->callbacks);
		wl_intrson(wl);
	}
#else /* WL_ALL_PASSIVE */
	if (wl->resched)
		tasklet_schedule(&wl->tasklet);
	else {
		/* re-enable interrupts */
		wl_intrson(wl);
	}
#endif /* WL_ALL_PASSIVE */

done:
	WL_UNLOCK(wl);
	return;
#else /* NAPI_POLL */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
	WL_TRACE(("wl%d: wl_poll: rxbnd %d budget %d processed %d\n",
		wl->pub->unit, wl->pub->rxbnd, budget, wl->processed));

	ASSERT(wl->processed <= wl->pub->tunables->rxbnd);

	/* update number of frames processed */
	/* we got packets but no budget */
	if (!wl->resched) {
		napi_complete(&wl->napi);
		/* enable interrupts now */
		wl_intrson(wl);
	}
	return wl->processed;
done:
	return 0;

#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30) */
	WL_TRACE(("wl%d: wl_poll: rxbnd %d quota %d budget %d processed %d\n",
	          wl->pub->unit, wl->pub->rxbnd, dev->quota,
	          *budget, wl->processed));

	ASSERT(wl->processed <= wl->pub->tunables->rxbnd);

	/* update number of frames processed */
	*budget -= wl->processed;
	dev->quota -= wl->processed;

	/* we got packets but no budget */
	if (wl->resched)
		/* indicate that we are not done, don't enable
		 * interrupts yet. linux network core will call
		 * us again.
		 */
		return 1;

	netif_rx_complete(dev);

	/* enable interrupts now */
	wl_intrson(wl);
done:
	return 0;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30) */
#endif /* NAPI_POLL */
} /* wl_poll / wl_dpc */

#if defined(WL_ALL_PASSIVE)
static void BCMFASTPATH
wl_dpc_rxwork(struct wl_task *task)
{
	wl_info_t *wl = (wl_info_t *)task->context;
	WL_TRACE(("wl%d: %s\n", wl->pub->unit, __FUNCTION__));

	wl_dpc((unsigned long)wl);
	return;
}
#endif /* defined(WL_ALL_PASSIVE) */

#ifdef HNDCTF
static inline int32
wl_ctf_forward(wl_info_t *wl, struct sk_buff *skb)
{
	int32 ret;
	/* use slow path if ctf is disabled */
	if (!CTF_ENAB(wl->cih))
		return (BCME_ERROR);

	/* try cut thru first */
	if ((ret = ctf_forward(wl->cih, skb, skb->dev)) != BCME_ERROR) {
		if (ret == BCME_EPERM)
			PKTCFREE(wl->osh, skb, FALSE);
		return (BCME_OK);
	}

	return (BCME_ERROR);
}
#endif /* HNDCTF */

void
wl_sendup_event(wl_info_t *wl, wl_if_t *wlif, void *p)
{
	wl_sendup(wl, wlif, p, 1);
}

#if defined(WLCXO_SIM) && defined(WLCXO_IPC)
void
wl_cxo_ctrl_ipc_init(struct wl_info *wl)
{
	/* Initialize task struct for host ipc receive */
	MY_INIT_WORK(&wl->ipc_task.work, (work_func_t)wl_cxo_ctrl_ipc_task);
	wl->ipc_task.context = wl;
	/* initialize cxo ipc globals */
	g_d2c_ipc_hdl = &wl->ipc_task.work;
	g_d2c_cxo_ctrl_wq = wl->cxo_ctrl_wq;
	sema_init(&wl->ctrl_ret_sem, 0);
	g_d2c_ipc_ret_hdl = &wl->ctrl_ret_sem;
	smp_wmb();
}
#endif /* WLCXO_SIM && WLCXO_IPC */

#ifdef WLCXO_SIM
/**
 * Called by host driver to indicate a new message to offload driver.
 */
void
wl_cxo_new_msg_intr(wl_info_t *wl)
{
	ASSERT(wl != NULL);

	/* Indication to offload driver that a new ipc message is pending */
	schedule_work_on(1, (struct work_struct *)(uintptr)g_c2d_ipc_hdl);
}
#endif /* WLCXO_SIM */

#if defined(WLCXO_IPC)
void
wl_cxo_ret_msg_intr(wl_info_t *wl)
{
	ASSERT(wl != NULL);

	/* Indication to offload driver that a new ipc message is pending */
	up((struct semaphore *)(uintptr)g_c2d_ipc_ret_hdl);
}

void
wl_cxo_wait_for_ipc_event(wl_info_t *wl)
{
	down((struct semaphore *)(uintptr)g_d2c_ipc_ret_hdl);
}

static void
wl_cxo_ctrl_ipc_task(wl_task_t *task)
{
	wl_info_t *wl = (wl_info_t *)task->context;

	_WL_LOCK(wl);
	while (wlc_cxo_ipc_msg_handle(wl->ipc) == BCME_OK)
		;
	_WL_UNLOCK(wl);
}
#endif /* WLCXO_IPC */

/**
 * The last parameter was added for the build. Caller of
 * this function should pass 1 for now.
 */
void BCMFASTPATH
wl_sendup(wl_info_t *wl, wl_if_t *wlif, void *p, int numpkt)
{
	struct sk_buff *skb;
#ifdef HNDCTF
	struct sk_buff *nskb;
#endif /* HNDCTF */
	bool brcm_specialpkt;
#ifdef DPSTA
	wlc_bsscfg_t *bsscfg = wlc_bsscfg_find_by_wlcif(wl->wlc, NULL);
#endif
	WL_TRACE(("wl%d: wl_sendup: %d bytes\n", wl->pub->unit, PKTLEN(wl->osh, p)));

	/* Internally generated events have the special ether-type of
	 * ETHER_TYPE_BRCM
	*/
	brcm_specialpkt =
		(ntoh16_ua(PKTDATA(wl->pub->osh, p) + ETHER_TYPE_OFFSET) == ETHER_TYPE_BRCM);

	if (!brcm_specialpkt) {
#ifdef ARPOE
		/* Arp agent */
		if (ARPOE_ENAB(wl->pub)) {
			wl_arp_info_t *arpi = wl_get_arpi(wl, wlif);
			if (arpi) {
				int err = wl_arp_recv_proc(arpi, p);
				if ((err == ARP_REQ_SINK) || (err ==  ARP_REPLY_PEER)) {
					PKTFREE(wl->pub->osh, p, FALSE);
					return;
				}
			}
		}
#endif

#ifdef TOE
		/* Apply TOE */
		if (TOE_ENAB(wl->pub))
			(void)wl_toe_recv_proc(wl->toei, p);
#endif

#ifdef BDO
		if (BDO_SUPPORT(wl->pub) && BDO_ENAB(wl->pub)) {
			if (wl_bdo_rx(wl->bdo, PKTDATA(wl->pub->osh, p), PKTLEN(wl->pub->osh, p))) {
				PKTFREE(wl->pub->osh, p, FALSE);
				return;
			}
		}
#endif	/* BDO */
#ifdef TKO
		if (TKO_ENAB(wl->pub)) {
			if (wl_tko_rx(wl->tko, p)) {
				PKTFREE(wl->pub->osh, p, FALSE);
				return;
			}
		}
#endif	/* TKO */
	}

	/* route packet to the appropriate interface */
	if (wlif) {
		/* drop if the interface is not up yet */
		if (!wlif->dev || !netif_device_present(wlif->dev)) {
			WL_ERROR(("wl%d: wl_sendup: interface not ready\n", wl->pub->unit));
			PKTFREE(wl->osh, p, FALSE);
			return;
		}
		/* Convert the packet, mainly detach the pkttag */
		skb = PKTTONATIVE(wl->osh, p);
		skb->dev = wlif->dev;
	} else {
		/* Convert the packet, mainly detach the pkttag */
		skb = PKTTONATIVE(wl->osh, p);
		skb->dev = wl->dev;

#ifdef DPSTA
		BCM_REFERENCE(bsscfg);
		if (PSTA_ENAB(wl->pub) || DWDS_ENAB(bsscfg)) {
			if (dpsta_recv(skb) != BCME_OK) {
				PKTFRMNATIVE(wl->osh, skb);
				PKTFREE(wl->osh, skb, FALSE);
				return;
			}
		}
#endif /* DPSTA */
	}


#ifdef HNDCTF
	/* try cut thru' before sending up */
	if (wl_ctf_forward(wl, skb) != BCME_ERROR)
		return;

sendup_next:
	/* clear skipct flag before sending up */
	PKTCLRSKIPCT(wl->osh, skb);

#ifdef CTFPOOL
	/* allocate and add a new skb to the pkt pool */
	if (PKTISFAST(wl->osh, skb))
		osl_ctfpool_add_by_poolptr(wl->osh, CTFPOOLPTR(wl->osh, skb));

	/* clear fast buf flag before sending up */
	PKTCLRFAST(wl->osh, skb);

	/* re-init the hijacked field */
	CTFPOOLPTR(wl->osh, skb) = NULL;
#endif /* CTFPOOL */

	nskb = (PKTISCHAINED(skb) ? PKTCLINK(skb) : NULL);
	PKTSETCLINK(skb, NULL);
	PKTCLRCHAINED(wl->osh, skb);
	PKTCCLRFLAGS(skb);

	/* map the unmapped buffer memory before sending up */
	PKTCTFMAP(wl->osh, skb);
#endif /* HNDCTF */


	skb->protocol = eth_type_trans(skb, skb->dev);
	/* Internally generated special ether-type ETHER_TYPE_BRCM packets for event data
	 * have no requirement for alignment, so skip the alignment check for brcm_specialpkt
	*/
	if (!brcm_specialpkt && !ISALIGNED(skb->data, 4)) {
		WL_APSTA_RX(("Unaligned assert. skb %p. skb->data %p.\n", skb, skb->data));
		if (wlif) {
			WL_APSTA_RX(("wl_sendup: dev name is %s (wlif) \n", wlif->dev->name));
			WL_APSTA_RX(("wl_sendup: hard header len  %d (wlif) \n",
				wlif->dev->hard_header_len));
		}
		WL_APSTA_RX(("wl_sendup: dev name is %s (wl) \n", wl->dev->name));
		WL_APSTA_RX(("wl_sendup: hard header len %d (wl) \n", wl->dev->hard_header_len));
#ifdef WLCXO
		PKTFREE(wl->osh, skb, FALSE);
		return;
#endif
	}

	/* send it up */
	WL_APSTA_RX(("wl%d: wl_sendup(): pkt %p summed %d on interface %p (%s)\n",
		wl->pub->unit, p, skb->ip_summed, wlif, skb->dev->name));

#ifdef NAPI_POLL
	netif_receive_skb(skb);
#else /* NAPI_POLL */
	netif_rx(skb);
#endif /* NAPI_POLL */

#ifdef HNDCTF
	if (nskb != NULL) {
		nskb->dev = skb->dev;
		skb = nskb;
		goto sendup_next;
	}
#endif /* HNDCTF */
} /* wl_sendup */

/* I/O ports for configuration space */
#define	PCI_CFG_ADDR	0xcf8	/* Configuration address to read/write */
#define	PCI_CFG_DATA	0xcfc	/* Configuration data for conf 1 mode */
#define	PCI_CFG_DATA2	0xcfa	/* Configuration data for conf 2 mode */
#define PCI_EN 0x80000000

static uint32
read_pci_cfg(uint32 bus, uint32 slot, uint32 fun, uint32 addr)
{
	uint32 config_cmd = PCI_EN | (bus << PCICFG_BUS_SHIFT) |
		(slot << PCICFG_SLOT_SHIFT) | (fun << PCICFG_FUN_SHIFT) | (addr & 0xfffffffc);

	outl(config_cmd, PCI_CFG_ADDR);
	return inl(PCI_CFG_DATA);
}

static void
write_pci_cfg(uint32 bus, uint32 slot, uint32 fun, uint32 addr, uint32 data)
{
	uint32 config_cmd = PCI_EN | (bus << PCICFG_BUS_SHIFT) |
		(slot << PCICFG_SLOT_SHIFT) | (fun << PCICFG_FUN_SHIFT) | (addr & 0xfffffffc);

	outl(config_cmd, PCI_CFG_ADDR);
	outl(data, PCI_CFG_DATA);
}

static uint32 rc_bus = 0xffffffff, rc_dev, rc_fun;

int
wl_osl_pcie_rc(struct wl_info *wl, uint op, int param)
{
	uint32 data;

	if (op == 0) {	/* return link capability in configuration space */
		struct pci_dev *pdev, *pdev_rc;
		uint32 header_type, cap_ptr, link_cap_speed = 0;

		pdev = osl_pci_device(wl->osh);

		if (pdev == NULL)
			return -1;

		pdev_rc = pdev->bus->self;

		if (pdev_rc == NULL)
			return -2;

		rc_bus = pdev_rc->bus->number;
		rc_dev = PCI_SLOT(pdev_rc->devfn);
		rc_fun = PCI_FUNC(pdev_rc->devfn);

		/* Header Type */
		data = read_pci_cfg(rc_bus, rc_dev, rc_fun, 0xc);
		header_type = (data >> 16) & 0x7f;

		if (header_type != 1)
			return -3;

		/* Status */
		data = read_pci_cfg(rc_bus, rc_dev, rc_fun, 0x4);
		data = (data >> 16) & 0xffff;

		if (((data >> 4) & 1) == 0)
			return -4;

		/* Capabilities Pointer */
		data = read_pci_cfg(rc_bus, rc_dev, rc_fun, 0x34);
		cap_ptr = data & 0xff;

		while (cap_ptr) {
			/* PCI Express Capabilities */
			data = read_pci_cfg(rc_bus, rc_dev, rc_fun, cap_ptr + 0x0);

			/* PCI Express Cap ID */
			if ((data & 0xff) != 0x10) {
				/* next cap pointer */
				cap_ptr = (data >> 8) & 0xff;
				continue;
			}

			/* Link Capabilities */
			data = read_pci_cfg(rc_bus, rc_dev, rc_fun, cap_ptr + 0xc);
			link_cap_speed = data & 0xf;
			break;
		}

		return link_cap_speed;
	} else if (op == 1) {		/* hot reset */
		if (rc_bus == 0xffffffff)
			return -1;

		data = read_pci_cfg(rc_bus, rc_dev, rc_fun, 0x3c);
		write_pci_cfg(rc_bus, rc_dev, rc_fun, 0x3c, data | 0x400000);
		OSL_DELAY(50 * 1000);
		write_pci_cfg(rc_bus, rc_dev, rc_fun, 0x3c, data);
		return 0;
	}

	return 0;
} /* wl_osl_pcie_rc */

void
wl_dump_ver(wl_info_t *wl, struct bcmstrbuf *b)
{
	bcm_bprintf(b, "wl%d: %s %s version %s\n", wl->pub->unit,
		__DATE__, __TIME__, EPI_VERSION_STR);
}


static void
wl_link_up(wl_info_t *wl, char *ifname)
{
	WL_ERROR(("wl%d: link up (%s)\n", wl->pub->unit, ifname));
}

static void
wl_link_down(wl_info_t *wl, char *ifname)
{
	WL_ERROR(("wl%d: link down (%s)\n", wl->pub->unit, ifname));
}

void
wl_event(wl_info_t *wl, char *ifname, wlc_event_t *e)
{
#ifdef USE_IW
	wl_iw_event(wl->dev, &(e->event), e->data);
#endif /* USE_IW */

#if defined(USE_CFG80211)
	wl_cfg80211_event(wl->dev, &(e->event), e->data);
#endif
	switch (e->event.event_type) {
	case WLC_E_LINK:
		if (e->event.flags&WLC_EVENT_MSG_LINK)
			wl_link_up(wl, ifname);
		else
			wl_link_down(wl, ifname);
		break;
#if defined(BCMSUP_PSK) && defined(STA)
	case WLC_E_MIC_ERROR: {
		wlc_bsscfg_t *cfg = wlc_bsscfg_find(wl->wlc, e->event.bsscfgidx, NULL);
		if (cfg == NULL)
			break;
		wl_mic_error(wl, cfg, e->addr,
			e->event.flags&WLC_EVENT_MSG_GROUP,
			e->event.flags&WLC_EVENT_MSG_FLUSHTXQ);
		break;
	}
#endif
#if defined(WL_CONFIG_RFKILL)
	case WLC_E_RADIO: {
		mbool i;
		if (wlc_get(wl->wlc, WLC_GET_RADIO, &i) < 0)
			WL_ERROR(("%s: WLC_GET_RADIO failed\n", __FUNCTION__));
		if (wl->last_phyind == (mbool)(i & WL_RADIO_HW_DISABLE))
			break;

		wl->last_phyind = (mbool)(i & WL_RADIO_HW_DISABLE);

		WL_ERROR(("wl%d: Radio hardware state changed to %d\n", wl->pub->unit, i));
		wl_report_radio_state(wl);
		break;
	}
#else
	case WLC_E_RADIO:
		break;
#endif /* WL_CONFIG_RFKILL */
#if defined(DPSTA) && ((defined(STA) && defined(DWDS)) || defined(PSTA))
	case WLC_E_DPSTA_INTF_IND: {
		wl_dpsta_intf_event_t *dpsta_if = (wl_dpsta_intf_event_t *)(e->data);
		wlc_bsscfg_t *cfg = wlc_bsscfg_find(wl->wlc, e->event.bsscfgidx, NULL);

#if defined(STA) && defined(DWDS)
		if (dpsta_if->intf_type == WL_INTF_DWDS) {
			wl_dpsta_dwds_register(wl, cfg);
			break;
		}
#endif /* STA && DWDS */
#ifdef PSTA
		if (dpsta_if->intf_type == WL_INTF_PSTA) {
			wl_dpsta_psta_register(wl, cfg);
			break;
		}
#endif /* PSTA */
	}
#endif /* DPSTA && ((STA &&DWDS) || PSTA) */
	}
} /* wl_event */


void
wl_event_sync(wl_info_t *wl, char *ifname, wlc_event_t *e)
{
#ifndef LINUX_POSTMOGRIFY_REMOVAL
#ifdef WL_EVENTQ
	/* duplicate event for local event q */
	wl_eventq_dup_event(wl->wlevtq, e);
#endif /* WL_EVENTQ */
#endif /* LINUX_POSTMOGRIFY_REMOVAL */

}

#ifndef WL_THREAD
/*
 * Called in non-passive mode when we need to send frames received on other CPU.
 */
static void BCMFASTPATH
wl_sched_tx_tasklet(void *t)
{
	wl_info_t *wl = (wl_info_t *)t;
	tasklet_schedule(&wl->tx_tasklet);
}


#ifdef CONFIG_SMP
#define WL_CONFIG_SMP()	TRUE
#else
#define WL_CONFIG_SMP()	FALSE
#endif /* CONFIG_SMP */

static int32 BCMFASTPATH
wl_txq_xmit(wl_info_t *wl, struct sk_buff *skb)
{
	skb->prev = NULL;

	/* Lock the queue as tasklet could be running at this time */
	TXQ_LOCK(wl);

	if ((wl_txq_thresh > 0) && (wl->txq_cnt >= wl_txq_thresh)) {
		PKTFRMNATIVE(wl->osh, skb);
		PKTCFREE(wl->osh, skb, TRUE);
		TXQ_UNLOCK(wl);
		return BCME_OK;
	}

	if (wl->txq_head == NULL)
		wl->txq_head = skb;
	else
		wl->txq_tail->prev = skb;
	wl->txq_tail = skb;
	wl->txq_cnt++;

	if (!wl->txq_dispatched) {
		int32 err = 0;

		/* In smp non passive mode, schedule tasklet for tx */
		if (!WL_ALL_PASSIVE_ENAB(wl) && txworkq == 0)
			wl_sched_tx_tasklet(wl);
#ifdef WL_ALL_PASSIVE
#ifdef CONFIG_SMP
		else if (txworkq && wl->max_cpu_id > 0)
			err = (int32)(SCHEDULE_WORK_ON(wl,
				wl->max_cpu_id - raw_smp_processor_id(),
				&wl->txq_task.work) == 0);
#endif
		else
			err = (int32)(SCHEDULE_WORK(wl, &wl->txq_task.work) == 0);
#endif /* WL_ALL_PASSIVE */

		if (!err) {
			atomic_inc(&wl->callbacks);
			wl->txq_dispatched = TRUE;
		} else
			WL_ERROR(("wl%d: wl_start/schedule_work failed\n",
			          wl->pub->unit));
	}

	TXQ_UNLOCK(wl);

	return BCME_OK;
}

#ifdef WLCXO_SIM
static int32 BCMFASTPATH
wl_cxo_txhpktq_xmit(wl_info_t *wl, struct sk_buff *skb)
{
	if ((wl_txhpktq_thresh > 0) && (g_txhpktq_cnt >= wl_txhpktq_thresh)) {
		PKTFRMNATIVE(wl->osh, skb);
		PKTCFREE(wl->osh, skb, TRUE);
		return BCME_OK;
	}

	/* Assume CTF forwarded packets are directly recevied from the
	 * h/w ethernet switch interface, equivalent to transmitting frames
	 * via real offload path. Use headroom to store pktinfo.
	 */
	TXQ_LOCK(wl);
	PKTFRMNATIVE(wl->osh, skb);

	/* Add it to the txhpkt queue and schedule work item */
	skb->prev = NULL;
	if (g_txhpktq_head == NULL)
		g_txhpktq_head = skb;
	else
		g_txhpktq_tail->prev = skb;
	g_txhpktq_tail = skb;
	g_txhpktq_cnt += (PKTISCHAINED(skb) ? PKTCCNT(skb) : 1);

	TXQ_UNLOCK(wl);

	wl_cxo_new_msg_intr(wl);

	return BCME_OK;
}
#endif /* WLCXO_SIM */

/**
 * Transmit pkt. In PASSIVE mode, enqueue pkt to local queue,schedule task to
 * run, return this context. In non passive mode, directly call wl_start_int()
 * to transmit pkt. !WL_THREAD specific.
 */
static int BCMFASTPATH
wl_start(struct sk_buff *skb, struct net_device *dev)
{
	wl_if_t *wlif;
	wl_info_t *wl;

	if (!dev)
		return -ENETDOWN;

	wlif = WL_DEV_IF(dev);
	wl = WL_INFO_GET(dev);

#ifdef WLCXO_SIM
	/* If the frame is offloadable then add it to a different queue */
	if (WLCXO_ENAB(wl->pub) && (PKTISCXODATA(skb) || (PKTISCTF(wl->osh, skb) &&
	    wlc_cxo_ctrl_txframe_offloadable(wl->wlc, skb, wlif->wlcif)))) {
		return wl_cxo_txhpktq_xmit(wl, skb);
	}
#endif

	/* Call in the same context when we are UP and non-passive is enabled */
	if (WL_ALL_PASSIVE_ENAB(wl) || (WL_RTR() && WL_CONFIG_SMP())) {
		return wl_txq_xmit(wl, skb);
	} else {
		return wl_start_int(wl, wlif, skb);
	}

	return BCME_OK;
} /* wl_start */

#endif /* WL_THREAD */

static void BCMFASTPATH
wl_start_txqwork(wl_task_t *task)
{
	wl_info_t *wl = (wl_info_t *)task->context;
	struct sk_buff *skb;

	WL_TRACE(("wl%d: %s txq_cnt %d\n", wl->pub->unit, __FUNCTION__, wl->txq_cnt));


	TXQ_LOCK(wl);
	while (wl->txq_head) {
		skb = wl->txq_head;
		wl->txq_head = skb->prev;
		skb->prev = NULL;
		if (wl->txq_head == NULL)
			wl->txq_tail = NULL;
		wl->txq_cnt--;
		TXQ_UNLOCK(wl);

		/* it has WL_LOCK/WL_UNLOCK inside */
		wl_start_int(wl, WL_DEV_IF(skb->dev), skb);

		TXQ_LOCK(wl);
	}

	wl->txq_dispatched = FALSE;
	atomic_dec(&wl->callbacks);
	TXQ_UNLOCK(wl);
} /* wl_start_txqwork */

static void BCMFASTPATH
wl_tx_tasklet(ulong data)
{
	wl_task_t task;
	task.context = (void *)data;
	wl_start_txqwork(&task);
}

static void
wl_txq_free(wl_info_t *wl)
{
	struct sk_buff *skb;

	if (wl->txq_head == NULL) {
		ASSERT(wl->txq_tail == NULL);
		return;
	}

	while (wl->txq_head) {
		skb = wl->txq_head;
		wl->txq_head = skb->prev;
		wl->txq_cnt--;
		PKTFRMNATIVE(wl->osh, skb);
		PKTCFREE(wl->osh, skb, TRUE);
	}

	wl->txq_tail = NULL;
}

#ifdef WLCXO_SIM
void
wl_txhpktq_free(wl_info_t *wl)
{
	struct sk_buff *skb, *txhpkt;

	if (g_txhpktq_head == NULL) {
		ASSERT(g_txhpktq_tail == NULL);
		return;
	}

	txhpkt = g_txhpktq_head;
	while (txhpkt != NULL) {
		skb = txhpkt;
		txhpkt = txhpkt->prev;
		skb->prev = NULL;
		g_txhpktq_cnt--;
		PKTCFREE(wl->osh, skb, TRUE);
	}

	g_txhpktq_head = g_txhpktq_tail = NULL;
}
#endif /* WLCXO_SIM */

#ifdef WL_ALL_PASSIVE
static void
wl_set_multicast_list_workitem(struct work_struct *work)
{
	wl_task_t *task = (wl_task_t *)work;
	struct net_device *dev = (struct net_device*)task->context;
	wl_info_t *wl;

	wl = WL_INFO_GET(dev);

	atomic_dec(&wl->callbacks);

	_wl_set_multicast_list(dev);
}

static void
wl_timer_task(wl_task_t *task)
{
	wl_timer_t *t = (wl_timer_t *)task->context;

	_wl_timer(t);
	MFREE(t->wl->osh, task, sizeof(wl_task_t));

	/* This dec is for the task_schedule. The timer related
	 * callback is decremented in _wl_timer
	 */
	atomic_dec(&t->wl->callbacks);
}
#endif /* WL_ALL_PASSIVE */

static void
wl_timer(ulong data)
{
	wl_timer_t *t = (wl_timer_t *)data;

	if (!WL_ALL_PASSIVE_ENAB(t->wl))
		_wl_timer(t);
#ifdef WL_ALL_PASSIVE
	else
		wl_schedule_task(t->wl, wl_timer_task, t);
#endif /* WL_ALL_PASSIVE */
}

static void
_wl_timer(wl_timer_t *t)
{
	wl_info_t *wl = t->wl;

	WL_LOCK(wl);

	if (t->set && (!timer_pending(&t->timer))) {
		if (t->periodic) {
			/* Periodic timer can't be a zero delay */
			ASSERT(t->ms != 0);

			/* See the comment in the similar logic in wl_add_timer in this file but
			 * note in this case of re-programming a periodic timer, there has
			 * been a conscious decision to still add the +1 adjustment.  We want
			 * to guarantee that two consecutive callbacks are always AT LEAST the
			 * requested ms delay apart, even if this means the callbacks might "drift"
			 * from even the rounded ms to jiffy HZ period.
			 */
			t->timer.expires = jiffies + (t->ms*HZ+999)/1000 + 1;
			atomic_inc(&wl->callbacks);
			add_timer(&t->timer);
			t->set = TRUE;
		} else
			t->set = FALSE;

		t->fn(t->arg);

	}

	atomic_dec(&wl->callbacks);

	WL_UNLOCK(wl);
} /* _wl_timer */

wl_timer_t *
wl_init_timer(wl_info_t *wl, void (*fn)(void *arg), void *arg, const char *tname)
{
	wl_timer_t *t;

	t = (wl_timer_t*)MALLOCZ(wl->osh, sizeof(wl_timer_t));

	if (t == NULL) {
		WL_ERROR(("wl%d: wl_init_timer: out of memory, malloced %d bytes\n",
			wl->unit, MALLOCED(wl->osh)));
		return 0;
	}

	init_timer(&t->timer);
	t->timer.data = (ulong) t;
	t->timer.function = wl_timer;
	t->wl = wl;
	t->fn = fn;
	t->arg = arg;
	t->next = wl->timers;
	wl->timers = t;


	return t;
}

/* BMAC_NOTE: Add timer adds only the kernel timer since it's going to be more accurate
 * as well as it's easier to make it periodic
 */
void
wl_add_timer(wl_info_t *wl, wl_timer_t *t, uint ms, int periodic)
{
	/* ASSERT(!t->set); */

	/* Delay can't be zero for a periodic timer */
	ASSERT(periodic == 0 || ms != 0);

	t->ms = ms;
	t->periodic = (bool) periodic;

	/* if timer has been added, Just return w/ updated behavior */
	if (t->set)
		return;

	t->set = TRUE;
	/* Make sure that you meet the guarantee of ms delay before
	 * calling the function.  You must consider both rounding to
	 * HZ and the fact that the next jiffy might be imminent, e.g.
	 * the timer interrupt is only a us away.
	 */
	if (ms == 0) {
		/* Zero is special - no HZ rounding up necessary nor
		 * accounting for an imminent timer tick.  Just use
		 * the current jiffies value.
		 */
		t->timer.expires = jiffies;
	} else {
		/* In converting ms to HZ, round up. Example: with HZ=250
		 * and thus a 4 ms jiffy/tick, round a 3 ms request to
		 * 1 jiffy, i.e. 4 ms.  In addition because the timer
		 * tick might occur imminently, you must add an extra
		 * jiffy/tick to guarantee the 3 ms request.
		 */
		t->timer.expires = jiffies + (ms*HZ+999)/1000 + 1;
	}

	atomic_inc(&wl->callbacks);
	add_timer(&t->timer);
} /* wl_add_timer */

/* return TRUE if timer successfully deleted, FALSE if we deleted an inactive timer. */
bool
wl_del_timer(wl_info_t *wl, wl_timer_t *t)
{
	ASSERT(t);
	if (t->set) {
		t->set = FALSE;
		if (!del_timer(&t->timer)) {
#ifdef WL_ALL_PASSIVE
			/*
			 * The timer was inactive - this is normal in passive mode when we
			 * try to delete a timer after it fired, but before the associated
			 * task got scheduled.
			 */
			return TRUE;
#else
			return FALSE;
#endif
		}
		atomic_dec(&wl->callbacks);
	}

	return TRUE;
}

void
wl_free_timer(wl_info_t *wl, wl_timer_t *t)
{
	wl_timer_t *tmp;

	/* delete the timer in case it is active */
	wl_del_timer(wl, t);

	if (wl->timers == t) {
		wl->timers = wl->timers->next;
		MFREE(wl->osh, t, sizeof(wl_timer_t));
		return;

	}

	tmp = wl->timers;
	while (tmp) {
		if (tmp->next == t) {
			tmp->next = t->next;
			MFREE(wl->osh, t, sizeof(wl_timer_t));
			return;
		}
		tmp = tmp->next;
	}

}

#if defined(BCMSUP_PSK) && defined(STA)
static void
wl_mic_error(wl_info_t *wl, wlc_bsscfg_t *cfg, struct ether_addr *ea, bool group, bool flush_txq)
{
	WL_WSEC(("wl%d: mic error using %s key\n", wl->pub->unit,
		(group) ? "group" : "pairwise"));

	if (wlc_sup_mic_error(cfg, group))
		return;
}
#endif /* defined(BCMSUP_PSK) && defined(STA) */

void
wl_monitor(wl_info_t *wl, struct wl_rxsts *rxsts, void *p)
{
#ifdef WL_MONITOR
	struct sk_buff *oskb = (struct sk_buff *)p;
	struct sk_buff *skb = NULL;
	uint16 len = 0, olen = 0;

	WL_TRACE(("wl%d: wl_monitor\n", wl->pub->unit));

	if (!wl->monitor_dev) {
		WL_ERROR(("wl%d: %s: monitor_dev is NULL\n", wl->pub->unit, __FUNCTION__));
		return;
	}

	PKTPULL(wl->wlc->osh, p, D11_PHY_HDR_LEN);

	len = PKTLEN(wl->osh, oskb);

	if (oskb->next) {
		struct sk_buff *amsdu_p = oskb->next;
		uint amsdu_len = 0;
		while (amsdu_p) {
			amsdu_len += amsdu_p->len;
			amsdu_p = amsdu_p->next;
		}
		len += amsdu_len;
	} else
		len = PKTLEN(wl->osh, oskb);

	olen = len;

	if ((skb = dev_alloc_skb(len + MAX_RADIOTAP_SIZE)) == NULL)
		return;

	len = wl_rxsts_to_rtap(rxsts, oskb->data, len, skb->data);

	if ((rxsts->nfrmtype & WL_RXS_NFRM_AMSDU_FIRST) ||
		(rxsts->nfrmtype & WL_RXS_NFRM_AMSDU_SUB)) {
		uint8* pdata = skb->data + (len - olen);
		memcpy(pdata, oskb->data, oskb->len);

		/* copy MSDU chain to payload portion of radiotap header */
		if (oskb->next) {
			struct sk_buff *amsdu_p = oskb->next;
			pdata += oskb->len;
			while (amsdu_p) {
				memcpy(pdata, amsdu_p->data, amsdu_p->len);
				pdata += amsdu_p->len;
				amsdu_p = amsdu_p->next;
			}
		}
	}

	skb_put(skb, len);

	skb->dev = wl->monitor_dev;
	skb->dev->last_rx = jiffies;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 22)
	skb_reset_mac_header(skb);
#else
	skb->mac.raw = skb->data;
#endif
	skb->ip_summed = CHECKSUM_NONE;
	skb->pkt_type = PACKET_OTHERHOST;
	skb->protocol = htons(ETH_P_80211_RAW);
#ifdef NAPI_POLL
	netif_receive_skb(skb);
#else
	netif_rx(skb);
#endif /* NAPI_POLL */
#endif /* WL_MONITOR */
}

#ifdef WL_MONITOR

static int
wl_monitor_start(struct sk_buff *skb, struct net_device *dev)
{
	wl_info_t *wl;

	wl = WL_DEV_IF(dev)->wl;
	PKTFREE(wl->osh, skb, FALSE);
	return 0;
}

/**
 * Create a virtual interface. Call only from safe time!
 * can't call register_netdev with WL_LOCK
 *
 * Equivalent to _wl_add_if
 */
static void
_wl_add_monitor_if(wl_task_t *task)
{
	struct net_device *dev;
	wl_if_t *wlif = (wl_if_t *) task->context;
	wl_info_t *wl = wlif->wl;

	WL_TRACE(("wl%d: %s\n", wl->pub->unit, __FUNCTION__));
	ASSERT(wl);
	ASSERT(!wl->monitor_dev);

	if ((dev = wl_alloc_linux_if(wlif)) == NULL) {
		WL_ERROR(("wl%d: %s: wl_alloc_linux_if failed\n", wl->pub->unit, __FUNCTION__));
		goto done;
	}

	/* Copy temp to real name */
	ASSERT(strlen(wlif->name) > 0);
	strncpy(wlif->dev->name, wlif->name, strlen(wlif->name) + 1);

	dev->type = ARPHRD_IEEE80211_RADIOTAP;

	/* override some fields */
	bcopy(wl->dev->dev_addr, dev->dev_addr, ETHER_ADDR_LEN);

	/* initialize dev fn pointers */
#if defined(WL_USE_NETDEV_OPS)
	dev->netdev_ops = &wl_netdev_monitor_ops;
#else
	dev->hard_start_xmit = wl_monitor_start;
	dev->do_ioctl = wl_ioctl;
	dev->get_stats = wl_get_stats;
#endif /* WL_USE_NETDEV_OPS */

	if (register_netdev(dev)) {
		WL_ERROR(("wl%d: %s, register_netdev failed for %s\n",
			wl->pub->unit, __FUNCTION__, dev->name));
		wl->monitor_dev = NULL;
		goto done;
	}

	/* Move monitor_dev assignment to here avoid panic */
	wl->monitor_dev = dev;
	wlif->dev_registered = TRUE;

done:
	MFREE(wl->osh, task, sizeof(wl_task_t));
	atomic_dec(&wl->callbacks);
} /* _wl_add_monitor_if */

static void
_wl_del_monitor(wl_task_t *task)
{
	wl_info_t *wl = (wl_info_t *) task->context;

	ASSERT(wl);
	ASSERT(wl->monitor_dev);

	WL_TRACE(("wl%d: _wl_del_monitor\n", wl->pub->unit));

	wl_free_if(wl, WL_DEV_IF(wl->monitor_dev));
	wl->monitor_dev = NULL;

	MFREE(wl->osh, task, sizeof(wl_task_t));
	atomic_dec(&wl->callbacks);
}

#endif /* WL_MONITOR */

/**
 * Create a dedicated monitor interface since libpcap caches the
 * packet type when it opens the device. The protocol type in the skb
 * is dropped somewhere in libpcap, and every received frame is tagged
 * with the DLT/ARPHRD type that's read by libpcap when the device is
 * opened.
 *
 * If libpcap was fixed to handle per-packet link types, we might not
 * need to create a pseudo device at all, wl_set_monitor() would be
 * unnecessary, and wlc->monitor could just get set in wlc_ioctl().
 */
/* Equivalent to wl_add_if */
void
wl_set_monitor(wl_info_t *wl, int val)
{
#ifdef WL_MONITOR
	const char *devname;
	wl_if_t *wlif;

	WL_TRACE(("wl%d: wl_set_monitor: val %d\n", wl->pub->unit, val));
	if ((val && wl->monitor_dev) || (!val && !wl->monitor_dev) ||
		(!val && !wl->monitor_type)) {
		WL_ERROR(("%s: Mismatched params, return\n", __FUNCTION__));
		return;
	}

	/* Delete monitor */
	if (!val) {
		wl->monitor_type = val;
		(void) wl_schedule_task(wl, _wl_del_monitor, wl);
		return;
	}

	/* Add monitor */
	if (val >= 1 && val <= 3) {
		wl->monitor_type = val;
	} else {
		WL_ERROR(("monitor type %d not supported\n", val));
		ASSERT(0);
	}

	wlif = wl_alloc_if(wl, WL_IFTYPE_MON, wl->pub->unit, NULL);
	if (!wlif) {
		WL_ERROR(("wl%d: %s: alloc wlif failed\n", wl->pub->unit, __FUNCTION__));
		return;
	}

	/* netdev isn't ready yet so stash name here for now and
	   copy into netdev when it becomes ready
	 */
	devname = "radiotap";
	(void)snprintf(wlif->name, sizeof(wlif->name), "%s%d", devname, wl->pub->unit);

	if (wl_schedule_task(wl, _wl_add_monitor_if, wlif)) {
		MFREE(wl->osh, wlif, sizeof(wl_if_t));
		return;
	}
#endif /* WL_MONITOR */
} /* wl_set_monitor */

#if LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 15)
const char *
print_tainted()
{
	return "";
}
#endif /* LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 15) */

#ifdef WL_PCMCIA
/* Remove zombie instances (card removed, detach pending) */
static void
flush_stale_links(void)
{
	dev_link_t *link, *next;

	for (link = dev_list; link; link = next) {
		next = link->next;
		if (link->state & DEV_STALE_LINK) {
			wl_cs_detach(link);
		}
	}
}

/**
 * This creates an "instance" of the driver, allocating local data
 * structures for one device.  The device is registered with Card
 * Services.
 *
 * The dev_link structure is initialized, but we don't actually
 * configure the card at this point -- we wait until we receive a card
 * insertion event.
 */
static dev_link_t *
wl_cs_attach(void)
{
	struct pcmcia_dev *dev;
	dev_link_t *link;
	client_reg_t client_reg;
	int ret;

	WL_TRACE(("wl: %s\n", __FUNCTION__));

	/* A bit of cleanup */
	flush_stale_links();

	dev = kmalloc(sizeof(struct pcmcia_dev), GFP_ATOMIC);
	if (!dev)
		return NULL;
	memset(dev, 0, sizeof(struct pcmcia_dev));

	/* Link both structures together */
	link = &dev->link;
	link->priv = dev;

	/* Initialize the dev_link_t structure */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
	init_timer(&link->release);
	link->release.function = &wl_cs_release;
	link->release.data = (u_long) link;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0) */

	/* General socket configuration defaults can go here.  In this
	 * client, we assume very little, and rely on the CIS for
	 * almost everything.  In most clients, many details (i.e.,
	 * number, sizes, and attributes of IO windows) are fixed by
	 * the nature of the device, and can be hard-wired here.
	 */
	link->conf.Attributes = 0;
	link->conf.IntType = INT_MEMORY_AND_IO;

	/* Register with Card Services */
	link->next = dev_list;
	dev_list = link;

	client_reg.dev_info = &dev_info;
	client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
	client_reg.EventMask =
		CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
		CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
		CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
	client_reg.event_handler = &wl_cs_event;
	client_reg.Version = 0x0210;
	client_reg.event_callback_args.client_data = link;

	ret = pcmcia_register_client(&link->handle, &client_reg);
	if (ret != CS_SUCCESS) {
		WL_NONE(("wl %s: CardServices - RegisterClient returned 0x%x\n",
			__FUNCTION__, ret));
		cs_error(link->handle, RegisterClient, ret);
		wl_cs_detach(link);
		return NULL;
	}

	return link;
} /* wl_cs_attach */

/**
 * This deletes a driver "instance".  The device is de-registered with
 * Card Services.  If it has been released, all local data structures
 * are freed.  Otherwise, the structures will be freed when the device
 * is released.
 */
static void
wl_cs_detach(dev_link_t * link)
{
	dev_link_t **linkp;
	struct pcmcia_dev *dev = (struct pcmcia_dev *) link->priv;

	WL_TRACE(("wl: %s\n", __FUNCTION__));

	/* Locate device structure */
	for (linkp = &dev_list; *linkp; linkp = &(*linkp)->next)
		if (*linkp == link)
			break;
	if (*linkp == NULL) {
		BUG();
		return;
	}

	if (link->state & DEV_CONFIG) {
		wl_cs_release((u_long)link);
		if (link->state & DEV_CONFIG) {
			link->state |= DEV_STALE_LINK;
			return;
		}
	}

	printk("wl: %s: deregistering\n", __FUNCTION__);

	/* Break the link with Card Services */
	if (link->handle)
		pcmcia_deregister_client(link->handle);

	/* Unlink device structure, and free it */
	*linkp = link->next;
	kfree(dev);
} /* wl_cs_detach */

/*
 * wl_cs_config() is scheduled to run after a CARD_INSERTION
 * event is received, to configure the PCMCIA socket, and to make the
 * device available to the system.
 */

#define CS_CHECK(fn, cmd, args...) \
	while ((last_cmd = cmd, last_ret = fn(args)) != 0) goto cs_failed

#define CFG_CHECK(fn, args...) \
	if (fn(args) != 0) goto next_entry

typedef struct cistpl_brcm_hnbu_t {
	uint8 SubType;
	uint16 VendorId;
	uint16 DeviceId;
	uint16 ChipRev;
} __attribute__ ((packed)) cistpl_brcm_hnbu_t;

static void
wl_cs_config(dev_link_t *link)
{
	struct pcmcia_dev *dev = link->priv;
	client_handle_t handle = link->handle;
	wl_info_t *wl = (wl_info_t *) dev->drv;
	int last_cmd, last_ret;
	u_char buf[64];
	config_info_t conf;
	cisinfo_t info;
	tuple_t tuple;
	cisparse_t parse;
	win_req_t req;
	cistpl_brcm_hnbu_t *brcm = (cistpl_brcm_hnbu_t *) buf;
	uint16 vendor, device;

	WL_TRACE(("wl: %s\n", __FUNCTION__));

	CS_CHECK(pcmcia_validate_cis, ValidateCIS, handle, &info);

	/*
	 * This reads the card's CONFIG tuple to find its
	 * configuration registers.
	 */
	tuple.DesiredTuple = CISTPL_CONFIG;
	tuple.Attributes = 0;
	tuple.TupleData = buf;
	tuple.TupleDataMax = sizeof(buf);
	tuple.TupleOffset = 0;
	CS_CHECK(pcmcia_get_first_tuple, GetFirstTuple, handle, &tuple);
	CS_CHECK(pcmcia_get_tuple_data, GetTupleData, handle, &tuple);
	CS_CHECK(pcmcia_parse_tuple, ParseTuple, handle, &tuple, &parse);
	link->conf.ConfigBase = parse.config.base;
	link->conf.Present = parse.config.rmask[0];

	/* Check our tuple */
	tuple.DesiredTuple = CISTPL_BRCM_HNBU;
	CS_CHECK(pcmcia_get_first_tuple, GetFirstTuple, handle, &tuple);
	CS_CHECK(pcmcia_get_tuple_data, GetTupleData, handle, &tuple);
	while (brcm->SubType != HNBU_CHIPID) {
		CS_CHECK(pcmcia_get_next_tuple, GetNextTuple, handle, &tuple);
		CS_CHECK(pcmcia_get_tuple_data, GetTupleData, handle, &tuple);
	}
	if (brcm->SubType == HNBU_CHIPID) {
		/* These fields are unaligned */
		vendor = le16_to_cpu(get_unaligned(&brcm->VendorId));
		device = le16_to_cpu(get_unaligned(&brcm->DeviceId));
		if (!wlc_chipmatch(vendor, device)) {
			WL_ERROR(("wl %s: No match for 0x%x- 0x%x\n",
				__FUNCTION__, vendor, device));
			goto failed;
		}
	} else
		goto failed;

	/* Configure card */
	link->state |= DEV_CONFIG;

	/* Look up the current Vcc */
	CS_CHECK(pcmcia_get_configuration_info, GetConfigurationInfo, handle, &conf);
	link->conf.Vcc = conf.Vcc;

	/*
	 * In this loop, we scan the CIS for configuration table
	 * entries, each of which describes a valid card
	 * configuration, including voltage, IO window, memory window,
	 * and interrupt settings.
	 *
	 * We make no assumptions about the card to be configured: we
	 * use just the information available in the CIS.  In an ideal
	 * world, this would work for any PCMCIA card, but it requires
	 * a complete and accurate CIS.  In practice, a driver usually
	 * "knows" most of these things without consulting the CIS,
	 * and most client drivers will only use the CIS to fill in
	 * implementation-defined details.
	 */
	tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
	CS_CHECK(pcmcia_get_first_tuple, GetFirstTuple, handle, &tuple);
	while (1) {
		cistpl_cftable_entry_t *cfg = &(parse.cftable_entry);
		cistpl_cftable_entry_t dflt = { .index = 0 };

		CFG_CHECK(pcmcia_get_tuple_data, handle, &tuple);
		CFG_CHECK(pcmcia_parse_tuple, handle, &tuple, &parse);

		if (cfg->flags & CISTPL_CFTABLE_DEFAULT)
			dflt = *cfg;
		if (cfg->index == 0)
			goto next_entry;
		link->conf.ConfigIndex = cfg->index;

		if (cfg->vpp1.present & (1 << CISTPL_POWER_VNOM))
			link->conf.Vpp1 = link->conf.Vpp2 =
				cfg->vpp1.param[CISTPL_POWER_VNOM] / 10000;
		else if (dflt.vpp1.present & (1 << CISTPL_POWER_VNOM))
			link->conf.Vpp1 = link->conf.Vpp2 =
				dflt.vpp1.param[CISTPL_POWER_VNOM] / 10000;

		/* Do we need to allocate an interrupt? */
		if (cfg->irq.IRQInfo1 || dflt.irq.IRQInfo1) {
			cistpl_irq_t *irq =
				(cfg->irq.IRQInfo1) ? &cfg->irq : &dflt.irq;
			link->conf.Attributes |= CONF_ENABLE_IRQ;
			link->irq.IRQInfo1 = irq->IRQInfo1;
			link->irq.IRQInfo2 = irq->IRQInfo2;
		}

		/*
		 * Now set up a common memory window, if needed.  There is room
		 * in the dev_link_t structure for one memory window handle,
		 * but if the base addresses need to be saved, or if multiple
		 * windows are needed, the info should go in the private data
		 * structure for this device.
		 *
		 * Note that the memory window base is a physical address, and
		 * needs to be mapped to virtual space with ioremap() before it
		 * is used.
		 */
		if ((cfg->mem.nwin > 0) || (dflt.mem.nwin > 0)) {
			cistpl_mem_t *mem =
				(cfg->mem.nwin) ? &cfg->mem : &dflt.mem;
			req.Attributes = WIN_DATA_WIDTH_16|WIN_MEMORY_TYPE_CM;
			req.Base = mem->win[0].host_addr;
			/* Request minimum size */
			req.Size = 0;
			req.AccessSpeed = 0;
			CS_CHECK(pcmcia_request_window, RequestWindow, &link->handle,
				&req, &link->win);
		}


		/* If we got this far, we're cool! */

		break;

	next_entry:
		if (link->win)
			pcmcia_release_window(link->win);
		last_ret = pcmcia_get_next_tuple(handle, &tuple);
		if (last_ret  == CS_NO_MORE_ITEMS) {
			printk(KERN_ERR "GetNextTuple().  No matching CIS configuration, "
				"maybe you need the ignore_cis_vcc=1 parameter.\n");
			goto cs_failed;
		}
	}

	/*
	 * Allocate an interrupt line.  Note that this does not assign
	 * a handler to the interrupt, unless the 'Handler' member of
	 * the irq structure is initialized.
	 */
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
		CS_CHECK(pcmcia_request_irq, RequestIRQ, link->handle, &link->irq);

	/*
	 * This actually configures the PCMCIA socket -- setting up
	 * the I/O windows and the interrupt mapping, and putting the
	 * card and host interface into "Memory and IO" mode.
	 */
	CS_CHECK(pcmcia_request_configuration, RequestConfiguration, link->handle, &link->conf);

	/* Ok, we have the configuration, prepare to register the netdev */
	if (!(dev->base = ioremap_nocache(req.Base, req.Size)))
		goto failed;
	dev->size = req.Size;
	wl = wl_attach(vendor, device, req.Base, PCMCIA_BUS, dev, link->irq.AssignedIRQ);
	if (!wl) {
		WL_TRACE(("wl: %s wl_attach failed\n", __FUNCTION__));
		goto failed;
	}
	dev->drv = wl;

	/* At this point, the dev_node_t structure(s) needs to be
	 * initialized and arranged in a linked list at link->dev.
	 */
	dev->node.major = dev->node.minor = 0;
	strncpy(dev->node.dev_name, wl->dev->name, sizeof(dev->node.dev_name));
	dev->node.dev_name[sizeof(dev->node.dev_name) - 1] = '\0';
	link->dev = &dev->node; /* link->dev also indicates net_device registered */
	link->state &= ~DEV_CONFIG_PENDING;


	return;

cs_failed:
	WL_NONE(("wl %s: at cs_failed, last_cmd 0x%x, last_ret 0x%x\n",
		__FUNCTION__, last_cmd, last_ret));
	cs_error(link->handle, last_cmd, last_ret);

failed:
	wl_cs_release((u_long) link);
} /* wl_cs_config */

/**
 * After a card is removed, wl_cs_release() will unregister the
 * device, and release the PCMCIA configuration.  If the device is
 * still open, this will be postponed until it is closed.
 */
static void
wl_cs_release(u_long arg)
{
	dev_link_t *link = (dev_link_t *) arg;
	struct pcmcia_dev *dev = (struct pcmcia_dev *) link->priv;
	wl_info_t *wl = (wl_info_t *) dev->drv;

	WL_TRACE(("wl: %s\n", __FUNCTION__));

	/* We're committed to taking the device away now, so mark the
	 * hardware as unavailable
	 */
	if (wl) {
		netif_device_detach(wl->dev);
		WL_LOCK(wl);
		wl_down(wl);
		WL_UNLOCK(wl);
		wl_free(wl);
		dev->drv = NULL;
	}

	/* Don't bother checking to see if these succeed or not */
	pcmcia_release_configuration(link->handle);
	if (dev->base)
		iounmap(dev->base);
	if (link->win)
		pcmcia_release_window(link->win);
	if (link->irq.AssignedIRQ)
		pcmcia_release_irq(link->handle, &link->irq);
	link->state &= ~DEV_CONFIG;
} /* wl_cs_release */

/**
 * The card status event handler.  Mostly, this schedules other stuff
 * to run after an event is received.
 */
static int
wl_cs_event(event_t event, int priority, event_callback_args_t *args)
{
	dev_link_t *link = args->client_data;

	WL_TRACE(("wl: %s: event %d\n", __FUNCTION__, event));

	switch (event) {
	case CS_EVENT_PM_SUSPEND:
		link->state |= DEV_SUSPEND;
		/* Fall through... */
	case CS_EVENT_RESET_PHYSICAL:
		/* Fall through... */
	case CS_EVENT_CARD_REMOVAL:
		link->state &= ~DEV_PRESENT;
		wl_cs_release((u_long)link);
		break;

	case CS_EVENT_PM_RESUME:
		link->state &= ~DEV_SUSPEND;
		/* Fall through... */
	case CS_EVENT_CARD_RESET:
		/* Fall through... */
	case CS_EVENT_CARD_INSERTION:
		link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
		wl_cs_config(link);
		break;
	}

	return 0;
} /* wl_cs_event */

#endif /* WL_PCMCIA */



#ifdef PLATFORM_INTEGRATED_WIFI
static int
wl_plat_drv_probe(struct platform_device *pdev)
{
	struct wl_platform_info *plat;
	struct resource *r;
	int error = 0;
	wl_info_t	*wl;
#ifdef BCMNVRAMR
	char *base = NULL, *nvp = NULL;
	int nvlen = 0;
#endif /* BCMNVRAMR */

	plat = devm_kzalloc(&pdev->dev, sizeof(*plat), GFP_KERNEL);
	if (!plat) {
		return -ENOMEM;
	}
	bzero(plat, sizeof(struct wl_platform_info));

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	plat->regs = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(plat->regs)) {
		return PTR_ERR(plat->regs);
	}

	plat->irq = platform_get_irq(pdev, 0);
	if (plat->irq < 0) {
		return plat->irq;
	}

#ifdef BCMNVRAMR
	base = nvp = MALLOC(NULL, MAXSZ_NVRAM_VARS);
	if (base == NULL)
		return BCME_NOMEM;

	/* Init nvram from nvram file if they exist */
	if (initvars_file(NULL, NULL, &nvp, (int*)&nvlen) == 0)
		plat->deviceid = (uint16)getintvar(base, "devid");

	if (plat->deviceid == 0)
		WL_ERROR(("Cannot get devid from nvram file.\n"));

	MFREE(NULL, base, MAXSZ_NVRAM_VARS);
#endif /* BCMNVRAMR */

	/*	devid = BCM7271_D11AC_ID, BCM7271_D11AC2G_ID, BCM7271_D11AC5G_ID, */
	wl = wl_attach(VENDOR_BROADCOM, plat->deviceid, SI_ENUM_BASE /* regs */,
		SI_BUS /* bus type */, &pdev->dev /* btparam or dev */, plat->irq,
		NULL /* BAR1_ADDR */, 0 /* BAR1_SIZE */, NULL /* BAR2_ADDR */,
		0 /* BAR2_SIZE */,NULL /* private data */);

	if (!wl) {
		error = -ENODEV;
	} else {
		wl->plat_info = plat;
		platform_set_drvdata(pdev, wl);
		printf("BCM7271 devid:0x%x Platform Driver\n", plat->deviceid);
	}

	return error;
}

static int
wl_plat_drv_remove(struct platform_device *pdev)
{
	wl_info_t	*wl;

	wl = (wl_info_t *)platform_get_drvdata(pdev);

	WL_LOCK((wl_info_t *)wl);
	wl_down((wl_info_t *)wl);
	WL_UNLOCK((wl_info_t *)wl);
	wl_free((wl_info_t *)wl);

	return 0;
}

static void
wl_plat_drv_shutdown(struct platform_device *pdev)
{
	BCM_REFERENCE(pdev);
	return;
}

static int
wl_plat_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	BCM_REFERENCE(pdev);
	BCM_REFERENCE(state);
	return 0;
}

static int
wl_plat_drv_resume(struct platform_device *pdev)
{
	BCM_REFERENCE(pdev);
	return 0;
}
#endif /* PLATFORM_INTEGRATED_WIFI */

#ifdef STB_SOC_WIFI
static void
wl_soc_wifi_enable_intrs(wl_info_t *wl)
{
#ifdef PLATFORM_INTEGRATED_WIFI
	struct wl_platform_info *plat = wl->plat_info;
	volatile uint32 *pReg = plat->regs;
	uint32	offset;

	/* clear wlan interface mask to enable interrupt */
	offset = ((BCHP_WLAN_INTF_D2H_INTR2_CPU_MASK_CLEAR & 0x1ff)>>2);
	pReg = pReg + offset;
	*pReg = 0xFFFFF;
#else /* PLATFORM_INTEGRATED_WIFI */
	BCM_REFERENCE(wl);
#endif /* PLATFORM_INTEGRATED_WIFI */
	return;
}

static void
wl_soc_wifi_disable_intrs(wl_info_t *wl)
{
#ifdef PLATFORM_INTEGRATED_WIFI
	struct wl_platform_info *plat = wl->plat_info;
	volatile uint32 *pReg = plat->regs;
	uint32	offset;

	/* clear wlan interface mask to disable interrupt */
	offset = ((BCHP_WLAN_INTF_D2H_INTR2_CPU_MASK_SET & 0x1ff)>>2);
	pReg = pReg + offset;
	*pReg = 0xFFFFF;
#else /* PLATFORM_INTEGRATED_WIFI */
	BCM_REFERENCE(wl);
#endif /* PLATFORM_INTEGRATED_WIFI */
	return;
}

/* Device to host interrupt handler */
static void
wl_soc_wifi_d2h_isr(wl_info_t *wl)
{
#ifdef PLATFORM_INTEGRATED_WIFI
	struct wl_platform_info *plat = wl->plat_info;
	volatile uint32 *pReg = plat->regs;
	volatile uint32 *reg;
	uint32 offset;
	uint32 d2hintstatus;

	offset = ((BCHP_WLAN_INTF_D2H_INTR2_CPU_STATUS & 0x1ff)>>2);
	reg = pReg + offset;

	d2hintstatus = *((volatile uint32*)(reg));
#else /* PLATFORM_INTEGRATED_WIFI */
	BCM_REFERENCE(wl);
#endif /* PLATFORM_INTEGRATED_WIFI */

	/* TODO. nothing for now. */
	return;
}

static void
wl_soc_wifi_d2h_intstatus(wl_info_t *wl)
{
#ifdef PLATFORM_INTEGRATED_WIFI
	struct wl_platform_info *plat = wl->plat_info;
	volatile uint32 *pReg = plat->regs;
	volatile uint32 *reg;
	uint32 offset;

	offset = ((BCHP_WLAN_INTF_D2H_INTR2_CPU_CLEAR & 0x1ff)>>2);
	reg = pReg + offset;
	*((volatile uint32*)(reg)) = 0xFFFFF;
#else /* PLATFORM_INTEGRATED_WIFI */
	BCM_REFERENCE(wl);
#endif /* PLATFORM_INTEGRATED_WIFI */
	return;
}
#endif /* STB_SOC_WIFI */

struct net_device *
wl_netdev_get(wl_info_t *wl)
{
	return wl->dev;
}

#ifdef LINUX_CRYPTO

int
wl_tkip_miccheck(wl_info_t *wl, void *p, int hdr_len, bool group_key, int key_index)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	struct sk_buff *skb = (struct sk_buff *)p;
	skb->dev = wl->dev;

	if (wl->tkipmodops) {
		if (group_key && wl->tkip_bcast_data[key_index])
			return (wl->tkipmodops->decrypt_msdu(skb, key_index, hdr_len,
				wl->tkip_bcast_data[key_index]));
		else if (!group_key && wl->tkip_ucast_data)
			return (wl->tkipmodops->decrypt_msdu(skb, key_index, hdr_len,
				wl->tkip_ucast_data));
	}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14) */
	WL_ERROR(("%s: No tkip mod ops\n", __FUNCTION__));

	return -1;
}

int
wl_tkip_micadd(wl_info_t *wl, void *p, int hdr_len)
{
	int error = -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	struct sk_buff *skb = (struct sk_buff *)p;
	skb->dev = wl->dev;

	if (wl->tkipmodops) {
		if (wl->tkip_ucast_data)
			error = wl->tkipmodops->encrypt_msdu(skb, hdr_len, wl->tkip_ucast_data);
		if (error)
			WL_ERROR(("Error encrypting MSDU %d\n", error));
	} else
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14) */
		WL_ERROR(("%s: No tkip mod ops\n", __FUNCTION__));
	return error;
}

int
wl_tkip_encrypt(wl_info_t *wl, void *p, int hdr_len)
{
	int error = -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	struct sk_buff *skb = (struct sk_buff *)p;
	skb->dev = wl->dev;

	if (wl->tkipmodops) {
		if (wl->tkip_ucast_data)
			error = wl->tkipmodops->encrypt_mpdu(skb, hdr_len, wl->tkip_ucast_data);
		if (error) {
			WL_ERROR(("Error encrypting MPDU %d\n", error));
		}
	} else
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14) */
		WL_ERROR(("%s: No tkip mod ops\n", __FUNCTION__));
	return error;

}

int
wl_tkip_decrypt(wl_info_t *wl, void *p, int hdr_len, bool group_key)
{
	int err = -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	struct sk_buff *skb = (struct sk_buff *)p;
	uint8 *pos;
	uint8 key_idx = 0;

	if (group_key) {
		skb->dev = wl->dev;
		pos = skb->data + hdr_len;
		key_idx = pos[3];
		key_idx >>= 6;
		WL_ERROR(("%s: Invalid key_idx %d\n", __FUNCTION__, key_idx));
		ASSERT(key_idx < NUM_GROUP_KEYS);
	}

	if (wl->tkipmodops) {
		if (group_key && key_idx < NUM_GROUP_KEYS && wl->tkip_bcast_data[key_idx])
			err = wl->tkipmodops->decrypt_mpdu(skb, hdr_len,
				wl->tkip_bcast_data[key_idx]);
		else if (!group_key && wl->tkip_ucast_data)
			err = wl->tkipmodops->decrypt_mpdu(skb, hdr_len, wl->tkip_ucast_data);
	} else {
		WL_ERROR(("%s: No tkip mod ops\n", __FUNCTION__));
	}

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14) */

	/* Error */
	return err;
} /* wl_tkip_decrypt */



int
wl_tkip_keyset(wl_info_t *wl, const wlc_key_info_t *key_info,
	const uint8 *key_data, size_t key_len,
	const uint8 *rx_seq, size_t rx_seq_len)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	bool group_key = FALSE;
	uint8 key_buf[32];

	ASSERT(key_info != NULL);
	if (key_len != 0) {
		ASSERT(key_len == 32);
		ASSERT(rx_seq_len == 6);
		WL_WSEC(("%s: Key Length is Not zero\n", __FUNCTION__));
		if (key_info->algo != CRYPTO_ALGO_TKIP) {
			WL_WSEC(("%s: Algo is Not TKIP %d\n", __FUNCTION__, key_info->algo));
			return 0;
		}
		WL_WSEC(("%s: Trying to set a key in TKIP Mod\n", __FUNCTION__));
	} else {
		WL_WSEC(("%s: Trying to Remove a Key from TKIP Mod\n", __FUNCTION__));
	}

	if (key_info->flags & WLC_KEY_FLAG_GROUP) {
		group_key = TRUE;
		WL_WSEC(("Group Key index %d\n", key_info->key_id));
	} else {
		WL_WSEC(("Unicast Key index %d\n", key_info->key_id));
	}

	if (wl->tkipmodops) {
		if (group_key) {
			if (key_len) {
				if (!wl->tkip_bcast_data[key_info->key_id]) {
					WL_WSEC(("Init TKIP Bcast Module\n"));
					WL_UNLOCK(wl);
					wl->tkip_bcast_data[key_info->key_id] =
						wl->tkipmodops->init(key_info->key_id);
					WL_LOCK(wl);
				}
				if (wl->tkip_bcast_data[key_info->key_id]) {
					WL_WSEC(("TKIP SET BROADCAST KEY******************\n"));
					bcopy(key_data, key_buf, 16);
					bcopy(key_data+24, key_buf + 16, 8);
					bcopy(key_data+16, key_buf + 24, 8);

/* set_key defined by linux kernel so that 3 parameter is not const, so
 * must disable -Wcast-qual warning
 */
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wcast-qual\"")
#endif
					wl->tkipmodops->set_key(key_buf, key_len, (uint8 *)rx_seq,
						wl->tkip_bcast_data[key_info->key_id]);
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic pop")
#endif
				}
			} else {
				if (wl->tkip_bcast_data[key_info->key_id]) {
					WL_WSEC(("Deinit TKIP Bcast Module\n"));
					wl->tkipmodops->deinit(
						wl->tkip_bcast_data[key_info->key_id]);
					wl->tkip_bcast_data[key_info->key_id] = NULL;
				}
			}
		} else {
			if (key_len) {
				if (!wl->tkip_ucast_data) {
					WL_WSEC(("Init TKIP Ucast Module\n"));
					WL_UNLOCK(wl);
					wl->tkip_ucast_data =
						wl->tkipmodops->init(key_info->key_id);
					WL_LOCK(wl);
				}
				if (wl->tkip_ucast_data) {
					WL_WSEC(("TKIP SET UNICAST KEY******************\n"));
					bcopy(key_data, key_buf, 16);
					bcopy(key_data+24, key_buf + 16, 8);
					bcopy(key_data+16, key_buf + 24, 8);
/* set_key defined by linux kernel so that 3 parameter is not const, so
 * must disable -Wcast-qual warning
 */
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wcast-qual\"")
#endif
					wl->tkipmodops->set_key(key_buf, key_len,
						(uint8 *)rx_seq, wl->tkip_ucast_data);
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic pop")
#endif

				}
			} else {
				if (wl->tkip_ucast_data) {
					WL_WSEC(("Deinit TKIP Ucast Module\n"));
					wl->tkipmodops->deinit(wl->tkip_ucast_data);
					wl->tkip_ucast_data = NULL;
				}
			}
		}
	} else
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14) */
		WL_WSEC(("%s: No tkip mod ops\n", __FUNCTION__));

	return 0;
} /* wl_tkip_keyset */

void
wl_tkip_printstats(wl_info_t *wl, bool group_key)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
	struct seq_file sfile;
	struct seq_file *debug_buf = &sfile;
#else
	char debug_buf[512];
#endif
	int idx;
	if (wl->tkipmodops) {
		if (group_key) {
			for (idx = 0; idx < NUM_GROUP_KEYS; idx++) {
				if (wl->tkip_bcast_data[idx])
					wl->tkipmodops->print_stats(debug_buf,
						wl->tkip_bcast_data[idx]);
			}
		} else if (!group_key && wl->tkip_ucast_data)
			wl->tkipmodops->print_stats(debug_buf, wl->tkip_ucast_data);
		else
			return;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
		printk("%s: TKIP stats from module: %s\n",
			debug_buf->buf, group_key?"Bcast":"Ucast");
#else
		printk("%s: TKIP stats from module: %s\n",
			debug_buf, group_key?"Bcast":"Ucast");
#endif
	}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14) */
}

#endif /* LINUX_CRYPTO */

#ifdef BCM7271
#else /* !BCM7271 */

#endif /* BCM7271 */

#if defined(WL_CONFIG_RFKILL)   /* Rfkill support */

static int
wl_set_radio_block(void *data, bool blocked)
{
	wl_info_t *wl = data;
	uint32 radioval;

	WL_TRACE(("%s: kernel set blocked = %d\n", __FUNCTION__, blocked));

	radioval = WL_RADIO_SW_DISABLE << 16 | blocked;

	WL_LOCK(wl);

	if (wlc_set(wl->wlc, WLC_SET_RADIO, radioval) < 0) {
		WL_ERROR(("%s: SET_RADIO failed\n", __FUNCTION__));
		return 1;
	}

	WL_UNLOCK(wl);

	return 0;
}

static const struct rfkill_ops bcmwl_rfkill_ops = {
	.set_block = wl_set_radio_block
};

static int
wl_init_rfkill(wl_info_t *wl)
{
	int status;

	(void)snprintf(wl->wl_rfkill.rfkill_name, sizeof(wl->wl_rfkill.rfkill_name),
	"brcmwl-%d", wl->pub->unit);

	wl->wl_rfkill.rfkill = rfkill_alloc(wl->wl_rfkill.rfkill_name, &wl->dev->dev,
	RFKILL_TYPE_WLAN, &bcmwl_rfkill_ops, wl);

	if (!wl->wl_rfkill.rfkill) {
		WL_ERROR(("%s: RFKILL: Failed to allocate rfkill\n", __FUNCTION__));
		return -ENOMEM;
	}


	if (wlc_get(wl->wlc, WLC_GET_RADIO, &status) < 0) {
		WL_ERROR(("%s: WLC_GET_RADIO failed\n", __FUNCTION__));
		return 1;
	}

	rfkill_init_sw_state(wl->wl_rfkill.rfkill, status);

	if (rfkill_register(wl->wl_rfkill.rfkill)) {
		WL_ERROR(("%s: rfkill_register failed! \n", __FUNCTION__));
		rfkill_destroy(wl->wl_rfkill.rfkill);
		return 2;
	}

	WL_ERROR(("%s: rfkill registered\n", __FUNCTION__));
	wl->wl_rfkill.registered = TRUE;
	return 0;
}

static void
wl_uninit_rfkill(wl_info_t *wl)
{
	if (wl->wl_rfkill.registered) {
		rfkill_unregister(wl->wl_rfkill.rfkill);
		rfkill_destroy(wl->wl_rfkill.rfkill);
		wl->wl_rfkill.registered = FALSE;
		wl->wl_rfkill.rfkill = NULL;
	}
}

static void
wl_report_radio_state(wl_info_t *wl)
{
	WL_TRACE(("%s: report radio state %d\n", __FUNCTION__, wl->last_phyind));

	rfkill_set_hw_state(wl->wl_rfkill.rfkill, wl->last_phyind != 0);
}

#endif /* WL_CONFIG_RFKILL */

static void
wl_linux_watchdog(void *ctx)
{
	wl_info_t *wl = (wl_info_t *) ctx;
	struct net_device_stats *stats = NULL;
	uint id;
	wl_if_t *wlif;
	wl_if_stats_t wlif_stats;
#ifdef USE_IW
	struct iw_statistics *wstats = NULL;
	int phy_noise;
#endif
	if (wl == NULL)
		return;

	if (wl->if_list) {
		for (wlif = wl->if_list; wlif != NULL; wlif = wlif->next) {
			memset(&wlif_stats, 0, sizeof(wl_if_stats_t));
			wlc_wlcif_stats_get(wl->wlc, wlif->wlcif, &wlif_stats);

			/* refresh stats */
			if (wl->pub->up) {
				ASSERT(wlif->stats_id < 2);

				id = 1 - wlif->stats_id;
				stats = &wlif->stats_watchdog[id];
				if (stats) {
					stats->rx_packets = WLCNTVAL(wlif_stats.rxframe);
					stats->tx_packets = WLCNTVAL(wlif_stats.txframe);
					stats->rx_bytes = WLCNTVAL(wlif_stats.rxbyte);
					stats->tx_bytes = WLCNTVAL(wlif_stats.txbyte);
					stats->rx_errors = WLCNTVAL(wlif_stats.rxerror);
					stats->tx_errors = WLCNTVAL(wlif_stats.txerror);
					stats->collisions = 0;
					stats->rx_length_errors = 0;
					/*
					 * Stats which are not kept per interface
					 * come from per radio stats
					 */
					stats->rx_over_errors = WLCNTVAL(wl->pub->_cnt->rxoflo);
					stats->rx_crc_errors = WLCNTVAL(wl->pub->_cnt->rxcrc);
					stats->rx_frame_errors = 0;
					stats->rx_fifo_errors = WLCNTVAL(wl->pub->_cnt->rxoflo);
					stats->rx_missed_errors = 0;
					stats->tx_fifo_errors = 0;
				}

#ifdef USE_IW
				wstats = &wlif->wstats_watchdog[id];
				if (wstats) {
#if WIRELESS_EXT > 11
					wstats->discard.nwid = 0;
					wstats->discard.code = WLCNTVAL(wl->pub->_cnt->rxundec);
					wstats->discard.fragment = WLCNTVAL(wlif_stats.rxfragerr);
					wstats->discard.retries = WLCNTVAL(wlif_stats.txfail);
					wstats->discard.misc = WLCNTVAL(wl->pub->_cnt->rxrunt) +
						WLCNTVAL(wl->pub->_cnt->rxgiant);
					wstats->miss.beacon = 0;
#endif /* WIRELESS_EXT > 11 */
				}
#endif /* USE_IW */

				wlif->stats_id = id;
			}
#ifdef USE_IW
			if (!wlc_get(wl->wlc, WLC_GET_PHY_NOISE, &phy_noise))
				wlif->phy_noise = phy_noise;
#endif /* USE_IW */

		}
	}

#ifdef CTFPOOL
	/* allocate and add a new skbs to the pkt pool */
	if (CTF_ENAB(wl->cih))
		osl_ctfpool_replenish(wl->osh, CTFPOOL_REFILL_THRESH);
#endif /* CTFPOOL */
} /* wl_linux_watchdog */

#if defined(VASIP_HW_SUPPORT)
uint32 wl_pcie_bar1(struct wl_info *wl, uchar** addr)
{
	*addr = wl->bar1_addr;
	return (wl->bar1_size);
}

uint32 wl_pcie_bar2(struct wl_info *wl, uchar** addr)
{
	*addr = wl->bar2_addr;
	return (wl->bar2_size);
}
#endif

#ifdef DPSTA
#if defined(STA) && defined(DWDS)

static bool
wl_dwds_is_ds_sta(wl_info_t *wl, wlc_bsscfg_t *bsscfg, struct ether_addr *mac)
{
	bool ret;
	wlc_bsscfg_t *cfg = wlc_bsscfg_find_by_wlcif(wl->wlc, NULL);

	WL_LOCK(wl);
	ret = wlc_dwds_is_ds_sta(cfg->wlc, mac);
	WL_UNLOCK(wl);

	return ret;
}

static bool
wl_dwds_authorized(wl_info_t *wl, wlc_bsscfg_t *cfg)
{
	bool ret;

	WL_LOCK(wl);
	ret = wlc_dwds_authorized(cfg);
	WL_UNLOCK(wl);

	return ret;
}

static void
wl_dpsta_dwds_register(wl_info_t *wl, wlc_bsscfg_t *pcfg)
{
	psta_if_api_t api;

	/* Register dwds sta APIs with DPSTA module */
	api.is_ds_sta = (bool (*)(void *, void *, struct ether_addr *))wl_dwds_is_ds_sta;
	api.psta_find = (void *(*)(void *, void *, uint8 *)) NULL;
	api.bss_auth = (bool (*)(void *, void *))wl_dwds_authorized;
	api.wl = wl;
	api.psta = pcfg->wlc;
	api.bsscfg = pcfg;
	api.mode = DPSTA_MODE_DWDS;
	(void) dpsta_register(wl->pub->unit, &api);

	return;
}

#endif /* STA && DWDS */

#ifdef PSTA

static bool
wl_psta_is_ds_sta(wl_info_t *wl, wlc_bsscfg_t *cfg, struct ether_addr *mac)
{
	bool ret;

	WL_LOCK(wl);
	ret = wlc_psta_is_ds_sta(cfg->wlc, mac);
	WL_UNLOCK(wl);

	return ret;
}

static bool
wl_psta_find_dpsta(wl_info_t *wl, wlc_bsscfg_t *cfg, uint8 *mac)
{
	bool ret;

	WL_LOCK(wl);
	ret = wlc_psta_find_dpsta(cfg->wlc, mac);
	WL_UNLOCK(wl);

	return ret;
}

static bool
wl_psta_authorized(wl_info_t *wl, wlc_bsscfg_t *cfg)
{
	bool ret;

	WL_LOCK(wl);
	ret = wlc_psta_authorized(cfg);
	WL_UNLOCK(wl);

	return ret;
}

static void
wl_dpsta_psta_register(wl_info_t *wl, wlc_bsscfg_t *pcfg)
{
	psta_if_api_t api;

	/* Register proxy sta APIs with DPSTA module */
	api.is_ds_sta = (bool (*)(void *, void *, struct ether_addr *))wl_psta_is_ds_sta;
	api.psta_find = (void *(*)(void *, void *, uint8 *))wl_psta_find_dpsta;
	api.bss_auth = (bool (*)(void *, void *))wl_psta_authorized;
	api.wl = wl;
	api.psta = wlc_psta_get_psta(pcfg->wlc);
	api.bsscfg = pcfg;
	api.mode = DPSTA_MODE_PSTA;
	(void) dpsta_register(wl->pub->unit, &api);

	return;
}

#endif /* PSTA */
#endif /* DPSTA */

int wl_fatal_error(void * wl, int rc)
{
	return FALSE;
}

#ifdef WLCXO_FULL
void*
wl_get_wl_cxo(wl_info_t *wl)
{
	return wl->wl_cxo;
}
#endif /* WLCXO_FULL */
