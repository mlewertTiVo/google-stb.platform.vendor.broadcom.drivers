/*
 * Linux-specific portion of
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wl_linux.c 651483 2016-07-27 06:30:17Z $
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
#include <linux/usb.h>
#if defined(LINUX_HYBRID)
#include <linux/pci_ids.h>
#define WLC_MAXBSSCFG		1	/* single BSS configs */
#else
#include <bcmdevs.h>
#endif

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

#if !defined(BCMDONGLEHOST) && !defined(LINUX_HYBRID)

#include <wlc_cfg.h>
#endif

#include <proto/802.1d.h>

#include <epivers.h>
#include <bcmendian.h>
#include <proto/ethernet.h>
#include <bcmutils.h>
#include <pcicfg.h>
#include <wlioctl.h>
#include <wlc_key.h>

#ifndef LINUX_POSTMOGRIFY_REMOVAL
#include <wlc_channel.h>
#else
typedef const struct si_pub si_t;
#endif
#include <wlc_pub.h>
#ifndef LINUX_POSTMOGRIFY_REMOVAL
#include <wlc_bsscfg.h>
#include <wlc_dump_info.h>
#endif


#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 4, 5)
#error "No support for Kernel Rev <= 2.4.5, As the older kernel revs doesn't support Tasklets"
#endif

#include <wl_dbg.h>
#ifdef WL_MONITOR
#include <wlc_ethereal.h>
#include <proto/ieee80211_radiotap.h>
#endif


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

#if defined(USE_CFG80211)
#include <net/rtnetlink.h>
#endif /* USE_CFG80211 */
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
#endif /* BCMPCMCIA */

#if (defined(WLMCHAN) && defined(WLC_HIGH_ONLY))
#define MCHAN_RPC_RX_SIZE_LIMIT 1000
#endif /* defined(WLMCHAN) && defined(WLC_HIGH_ONLY) */

#ifdef HNDCTF
#include <ctf/hndctf.h>
#endif

#ifdef WLC_HIGH_ONLY
#include "bcm_rpc_tp.h"
#include "bcm_rpc.h"
#include "bcm_xdr.h"
#include "wlc_rpc.h"
#endif


#include <wl_linux.h>

#ifdef PLC
#include <wl_plc_linux.h>
#endif /* PLC */

#ifdef WL_THREAD
#include <linux/kthread.h>
#endif /* WL_THREAD */

#if defined(USE_CFG80211)
#ifdef LINUX_HYBRID
#include <wl_cfg80211_hybrid.h>
#else
#include <wl_cfg80211.h>
#endif /* HYBRID */
#endif /* CFG80211 */

#ifdef DPSTA
#include <dpsta.h>
#endif

#ifdef WLCSO
#include <wlc_tso.h>
#endif /* WL_CSO */

#if defined(BCM_GMAC3)
#include <hndfwd.h>
#endif

static void wl_timer(ulong data);
static void _wl_timer(wl_timer_t *t);
static struct net_device *wl_alloc_linux_if(wl_if_t *wlif);

#ifdef WL_MONITOR
static int wl_monitor_start(struct sk_buff *skb, struct net_device *dev);
#endif

#ifdef WLC_HIGH_ONLY
static void wl_rpc_down(void *wlh);
static void wl_rpcq_free(wl_info_t *wl);
#ifdef WL_THREAD
static int wl_thread_dpc_wlthread(void *data);
static void wl_rpcq_dispatch_wlthread(wl_info_t *wl);
static void wl_start_txqwork_wlthread(wl_info_t *wl);
#else
static void wl_rpcq_dispatch(struct wl_task *task);
#endif /* WL_THREAD */

static void wl_rpc_dispatch_schedule(void *ctx, struct rpc_buf *buf);
#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_TXNOCOPY) || defined(BCM_RPC_TOC)
static void wl_rpc_txdone(void *ctx, struct rpc_buf* buf);
#endif
#define RPCQ_LOCK(_wl, _flags) spin_lock_irqsave(&(_wl)->rpcq_lock, (_flags))
#define RPCQ_UNLOCK(_wl, _flags)  spin_unlock_irqrestore(&(_wl)->rpcq_lock, (_flags))
#endif /* WLC_HIGH_ONLY */

static void wl_start_txqwork(wl_task_t *task);
static void wl_txq_free(wl_info_t *wl);
#define TXQ_LOCK(_wl) spin_lock_bh(&(_wl)->txq_lock)
#define TXQ_UNLOCK(_wl) spin_unlock_bh(&(_wl)->txq_lock)

#ifdef WL_ALL_PASSIVE
static void wl_set_multicast_list_workitem(struct work_struct *work);

static void wl_timer_task(wl_task_t *task);
#ifdef WLC_LOW
static void wl_dpc_rxwork(struct wl_task *task);
#endif
#endif /* WL_ALL_PASSIVE */

#ifdef LINUX_HYBRID
static int wl_reg_proc_entry(wl_info_t *wl);
#endif

static int wl_linux_watchdog(void *ctx);
static int wl_found = 0;

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

#ifdef WL_INFO
#undef WL_INFO
#endif
#define WL_INFO(dev)	((wl_info_t*)(WL_DEV_IF(dev)->wl))	/* dev to wl_info_t */


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
#if defined(AP) || defined(WLC_HIGH_ONLY) || defined(WL_MONITOR)
static int wl_schedule_task(wl_info_t *wl, void (*fn)(struct wl_task *), void *context);
#endif
#ifdef WL_THREAD
static int wl_start_enqueue_wlthread(wl_info_t *wl, struct sk_buff *skb);
#endif
#if !defined(LINUX_HYBRID) && defined(CONFIG_PROC_FS)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static int wl_read_proc(struct seq_file *m, void *v);
#else /* Kernel >= 3.10.0 */
static int wl_read_proc(char *buffer, char **start, off_t offset, int length, int *eof, void *data);
#endif /* Kernel >= 3.10.0 */
#endif /* #if !defined(LINUX_HYBRID) && defined(CONFIG_PROC_FS) */
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



#if !defined(LINUX_HYBRID)
MODULE_LICENSE("Proprietary");
#endif /* LINUX_HYBRID */

#if defined(CONFIG_PCI)
/* recognized PCI IDs */
static struct pci_device_id wl_id_table[] =
{
	{ PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID,
	PCI_CLASS_NETWORK_OTHER << 8, 0xffff00, 0 },
	{ 0 }
};

MODULE_DEVICE_TABLE(pci, wl_id_table);
#endif 


static unsigned int online_cpus = 1;



#if defined(WL_ALL_PASSIVE) && defined(WLC_LOW)
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

#if defined(WLC_HIGH_ONLY)
#define WL_LIMIT_RPCQ

#ifdef WL_LIMIT_RPCQ
#define WL_RPCQ_RXTHRESH 0
static int wl_rpcq_rxthresh = WL_RPCQ_RXTHRESH;
module_param(wl_rpcq_rxthresh, int, 0);
#endif /* WL_LIMIT_RPCQ */
#endif /* WLC_HIGH_ONLY */

#define WL_TXQ_THRESH	0
static int wl_txq_thresh = WL_TXQ_THRESH;
module_param(wl_txq_thresh, int, 0);

static int oneonly = 0;
module_param(oneonly, int, 0);

static int piomode = 0;
module_param(piomode, int, 0);

static int instance_base = 0; /* Starting instance number */
module_param(instance_base, int, 0);

#if defined(BCM_GMAC3)

static int pcie_probe_unit = 0; /* PCIe probe sequence. */
module_param(pcie_probe_unit, int, 0644);

/** WL_FWDER_UNIT(): Fetch the assigned fwder_unit for this radio. */
#define WL_FWDER_UNIT(wl)        ((wl)->fwder_unit)

/** Wl forwarding bypass handler attached to GMAC forwarder. */
static int wl_forward(struct fwder * fwder, struct sk_buff *skb, int skb_cnt,
                      struct net_device *dev);
static int BCMFASTPATH wl_start_try(struct sk_buff *skb, struct net_device *dev,
	bool in_lock);
#endif /* BCM_GMAC3 */


static int nompc = 0;
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
#if defined(USE_CFG80211)
#define BRCM_WLAN_IFNAME wlan%d
#else
#define BRCM_WLAN_IFNAME eth%d
#endif /* USE_CFG80211 */
#endif /* BRCM_WLAN_IFNAME */

static char intf_name[IFNAMSIZ] = quote_str(BRCM_WLAN_IFNAME);

/* allow override of default wlan interface name at insmod time */
module_param_string(intf_name, intf_name, IFNAMSIZ, 0);

/* BCMSLTGT: slow target */

#ifdef WL_MONITOR
#define WL_RADIOTAP_BRCM_SNS		0x01
#define WL_RADIOTAP_BRCM_MCS		0x00000001
#define WL_RADIOTAP_LEGACY_SNS		0x02
#define WL_RADIOTAP_LEGACY_VHT		0x00000001

#define IEEE80211_RADIOTAP_HTMOD_40		0x01
#define IEEE80211_RADIOTAP_HTMOD_SGI		0x02
#define IEEE80211_RADIOTAP_HTMOD_GF		0x04
#define IEEE80211_RADIOTAP_HTMOD_LDPC		0x08
#define IEEE80211_RADIOTAP_HTMOD_STBC_MASK	0x30
#define IEEE80211_RADIOTAP_HTMOD_STBC_SHIFT	4

static const u_int8_t brcm_oui[] =  {0x00, 0x10, 0x18};

/* Dyanmic bandwidth for VHT signaled in NONHT */
#define WL_RADIOTAP_F_NONHT_VHT_DYN_BW			0x01
/* VHT BW is valid in NONHT */
#define WL_RADIOTAP_F_NONHT_VHT_BW			0x02

/* VHT information in non-HT frames; primarily VHT b/w signaling
 * in frames received at legacy rates.
 */
struct wl_radiotap_nonht_vht {
	u_int8_t len;				/* length of the field excluding 'len' field */
	u_int8_t flags;
	u_int8_t bw;
} __attribute__ ((packed));

typedef struct wl_radiotap_nonht_vht wl_radiotap_nonht_vht_t;

/* radiotap standard - non-HT, non-VHT information with Broadcom vendor namespace extension
 * that includes VHT information.
 * Used with monitor type 2 or 3 when received by HT/Legacy PHY and received rate is legacy.
 */
struct wl_radiotap_legacy {
	struct ieee80211_radiotap_header ieee_radiotap;
	u_int32_t it_present_ext;
	u_int32_t pad1;
	uint32 tsft_l;
	uint32 tsft_h;
	uint8 flags;
	uint8 rate;
	uint16 channel_freq;
	uint16 channel_flags;
	uint8 signal;
	uint8 noise;
	int8 antenna;
	uint8 pad2;
	u_int8_t vend_oui[3];
	u_int8_t vend_sns;
	u_int16_t vend_skip_len;
	wl_radiotap_nonht_vht_t nonht_vht;
} __attribute__ ((__packed__));

typedef struct wl_radiotap_legacy wl_radiotap_legacy_t;

#define WL_RADIOTAP_LEGACY_SKIP_LEN htol16(sizeof(struct wl_radiotap_legacy) - \
	offsetof(struct wl_radiotap_legacy, nonht_vht))

#define WL_RADIOTAP_NONHT_VHT_LEN (sizeof(wl_radiotap_nonht_vht_t) - 1)

/* radiotap standard with Broadcom vendor namespace extension that includes
 * HT information. This is for use with monitor type 2 when HT phy was used.
 */
struct wl_radiotap_ht_brcm {
	struct ieee80211_radiotap_header ieee_radiotap;
	u_int32_t it_present_ext;
	u_int32_t pad1;
	uint32 tsft_l;
	uint32 tsft_h;
	u_int8_t flags;
	u_int8_t pad2;
	u_int16_t channel_freq;
	u_int16_t channel_flags;
	u_int8_t signal;
	u_int8_t noise;
	u_int8_t antenna;
	u_int8_t pad3;
	u_int8_t vend_oui[3];
	u_int8_t vend_sns;
	u_int16_t vend_skip_len;
	u_int8_t mcs;
	u_int8_t htflags;
} __attribute__ ((packed));

typedef struct wl_radiotap_ht_brcm wl_radiotap_ht_brcm_t;

#define WL_RADIOTAP_HT_BRCM_SKIP_LEN htol16(sizeof(struct wl_radiotap_ht_brcm) - \
	offsetof(struct wl_radiotap_ht_brcm, mcs))

/* Radiotap standard that includes HT information. This is for use with monitor type 3
 * whenever frame is received by HT-PHY, and received rate is non-VHT.
 */
struct wl_radiotap_ht {
	struct ieee80211_radiotap_header ieee_radiotap;
	uint32 tsft_l;
	uint32 tsft_h;
	u_int8_t flags;
	u_int8_t pad1;
	u_int16_t channel_freq;
	u_int16_t channel_flags;
	u_int8_t signal;
	u_int8_t noise;
	u_int8_t antenna;
	u_int8_t mcs_known;
	u_int8_t mcs_flags;
	u_int8_t mcs_index;
} __attribute__ ((packed));

typedef struct wl_radiotap_ht wl_radiotap_ht_t;

/* Radiotap standard that includes VHT information.
 * This is for use with monitor type 3 whenever frame is
 * received by HT-PHY (VHT-PHY), and received rate is VHT.
 */
struct wl_radiotap_vht {
	struct ieee80211_radiotap_header ieee_radiotap;
	uint32 tsft_l;			/* IEEE80211_RADIOTAP_TSFT */
	uint32 tsft_h;			/* IEEE80211_RADIOTAP_TSFT */
	u_int8_t flags;			/* IEEE80211_RADIOTAP_FLAGS */
	u_int8_t pad1;
	u_int16_t channel_freq;		/* IEEE80211_RADIOTAP_CHANNEL */
	u_int16_t channel_flags;	/* IEEE80211_RADIOTAP_CHANNEL */
	u_int8_t signal;		/* IEEE80211_RADIOTAP_DBM_ANTSIGNAL */
	u_int8_t noise;			/* IEEE80211_RADIOTAP_DBM_ANTNOISE */
	u_int8_t antenna;		/* IEEE80211_RADIOTAP_ANTENNA */
	u_int8_t pad2;
	u_int16_t pad3;
	uint32 ampdu_ref_num;		/* A-MPDU ID */
	u_int16_t ampdu_flags;		/* A-MPDU flags */
	u_int8_t ampdu_delim_crc;	/* Delimiter CRC if present in flags */
	u_int8_t ampdu_reserved;
	u_int16_t vht_known;		/* IEEE80211_RADIOTAP_VHT */
	u_int8_t vht_flags;		/* IEEE80211_RADIOTAP_VHT */
	u_int8_t vht_bw;		/* IEEE80211_RADIOTAP_VHT */
	u_int8_t vht_mcs_nss[4];	/* IEEE80211_RADIOTAP_VHT */
	u_int8_t vht_coding;		/* IEEE80211_RADIOTAP_VHT */
	u_int8_t vht_group_id;		/* IEEE80211_RADIOTAP_VHT */
	u_int16_t vht_partial_aid;	/* IEEE80211_RADIOTAP_VHT */
} __attribute__ ((packed));

typedef struct wl_radiotap_vht wl_radiotap_vht_t;

#define WL_RADIOTAP_PRESENT_LEGACY			\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	 (1 << IEEE80211_RADIOTAP_RATE) |		\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	 (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_ANTENNA) |		\
	 (1 << IEEE80211_RADIOTAP_VENDOR_NAMESPACE) |	\
	 (1 << IEEE80211_RADIOTAP_EXT))


#define WL_RADIOTAP_PRESENT_HT_BRCM			\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	 (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	 (1 << IEEE80211_RADIOTAP_ANTENNA) |		\
	 (1 << IEEE80211_RADIOTAP_VENDOR_NAMESPACE) |	\
	 (1 << IEEE80211_RADIOTAP_EXT))

#define WL_RADIOTAP_PRESENT_HT				\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	 (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	 (1 << IEEE80211_RADIOTAP_ANTENNA) |		\
	 (1 << IEEE80211_RADIOTAP_MCS))

#define WL_RADIOTAP_PRESENT_VHT			\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	 (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	 (1 << IEEE80211_RADIOTAP_ANTENNA) |		\
	 (1 << IEEE80211_RADIOTAP_AMPDU) |		\
	 (1 << IEEE80211_RADIOTAP_VHT))

/* include/linux/if_arp.h
 *	#define ARPHRD_IEEE80211_PRISM 802 IEEE 802.11 + Prism2 header
 *	#define ARPHRD_IEEE80211_RADIOTAP 803 IEEE 802.11 + radiotap header
 * include/net/ieee80211_radiotap.h
 *	radiotap structure
 */

#ifndef ARPHRD_IEEE80211_RADIOTAP
#define ARPHRD_IEEE80211_RADIOTAP 803
#endif
#endif /* WL_MONITOR */

#ifndef SRCBASE
#define SRCBASE "."
#endif

#if defined(USE_CFG80211) && defined(WLC_HIGH)
struct net_device *wl_net_find(void *wl, const char* ifname);
int wl_net_attach(void *netdev, int bssidx);
extern s32 wldev_ioctl(struct net_device *dev, u32 cmd, void *arg, u32 len, u32 set);
#endif /* (USE_CFG80211) && (WLC_HIGH) */

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

#if defined(USE_CFG80211) && defined(WLC_HIGH)
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

#if defined(WLP2P) && (defined(WL_ENABLE_P2P_IF) || defined(WL_CFG80211_P2P_DEV_IF))

	setbit(eventmask, WLC_E_ACTION_FRAME_RX);
	setbit(eventmask, WLC_E_ACTION_FRAME_COMPLETE);
	setbit(eventmask, WLC_E_ACTION_FRAME_OFF_CHAN_COMPLETE);
	setbit(eventmask, WLC_E_P2P_DISC_LISTEN_COMPLETE);
#endif  /* defined(WLP2P) && (defined(WL_ENABLE_P2P_IF) || defined(WL_CFG80211_P2P_DEV_IF)) */

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
#endif /* #if defined(USE_CFG80211) && defined(WLC_HIGH) */
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
		struct wl_info *wl = WL_INFO(dev);

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
	dev->wireless_handlers = (struct iw_handler_def *) &wl_iw_handler_def;
#endif
#endif /* USE_IW */

#if WIRELESS_EXT >= 19 || LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	dev->ethtool_ops = &wl_ethtool_ops;
#endif
}

#if defined(USE_CFG80211) && defined(WLC_HIGH)
struct net_device *wl_net_find(void *wl, const char* ifname)
{
	if (wl && ifname) {
		wl_if_t *wlif = ((wl_info_t *)wl)->if_list;
		do {
			/* WL_ERROR(("%s: wlif->name = %s \n", __FUNCTION__, wlif->dev->name)); */
			/* WL_ERROR(("%s: ifname = %s\n", __FUNCTION__, ifname)); */
			if (strcmp (wlif->dev->name, ifname) == 0) {
					return wlif->dev;
			}
			else
				wlif = wlif->next;
		}
		while (wlif);
	}

	return NULL;
}

int
wl_net_attach(void *netdev, int ifidx)
{
	struct net_device *dev = (struct net_device *)netdev;

	if (dev) {
		wl_if_setup(dev);
	}
	return 0;
}
#endif /* #if defined(USE_CFG80211) && defined(WLC_HIGH) */

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

#if !defined(LINUX_HYBRID) && defined(CONFIG_PROC_FS)
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
#endif /* #if !defined(LINUX_HYBRID) && defined(CONFIG_PROC_FS) */


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
static wl_info_t *
wl_attach(uint16 vendor, uint16 device, ulong regs,
	uint bustype, void *btparam, uint irq, uchar* bar1_addr, uint32 bar1_size)
{
	struct net_device *dev;
	wl_if_t *wlif;
	wl_info_t *wl;
#if !defined(LINUX_HYBRID) && defined(CONFIG_PROC_FS)
	char tmp[128];
#endif
	osl_t *osh;
	int unit, err;
#ifdef HNDCTF
	char ctf_name[IFNAMSIZ];
#endif /* HNDCTF */
#ifdef WLC_HIGH_ONLY
	int dngl_agglimit = 0;
	DEVICE_SPEED device_speed = INVALID_SPEED;
#endif /* WLC_HIGH_ONLY */
#if defined(USE_CFG80211)
	struct device *parentdev;
#endif
#ifdef CTFPOOL
	int32 ctfpoolsz;
	int32 ctfpoolobjsz = 0;
#endif
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
	osh = osl_attach(btparam, bustype, TRUE);
	ASSERT(osh);

	/* Set ACP coherence flag */
	if (OSL_ACP_WAR_ENAB() || OSL_ARCH_IS_COHERENT())
		osl_flag_set(osh, OSL_ACP_COHERENCE);

	/* allocate private info */
	if ((wl = (wl_info_t*) MALLOC(osh, sizeof(wl_info_t))) == NULL) {
		WL_ERROR(("wl%d: malloc wl_info_t, out of memory, malloced %d bytes\n", unit,
			MALLOCED(osh)));
		osl_detach(osh);
		return NULL;
	}
	bzero(wl, sizeof(wl_info_t));

	wl->osh = osh;
	wl->unit = unit;
	atomic_set(&wl->callbacks, 0);

#if defined(BCM_GMAC3)
	/* Assign a fwder_unit based on probe unit and fwder cpumap nvram. */
	WL_FWDER_UNIT(wl) = fwder_assign(FWDER_NIC_MODE, pcie_probe_unit);
	if (WL_FWDER_UNIT(wl) == FWDER_FAILURE) {
		WL_ERROR(("wl%d: %s: fwder_assign<%d> failed!\n",
		          unit, __FUNCTION__, pcie_probe_unit));
		goto fail;
	}
	pcie_probe_unit++;

	/*
	 * Primary interface:
	 * Attach my transmit handler to DNSTREAM fwder instance on core=unit
	 *      et::GMAC# -> et_sendup/chain -> wl_forward -> wl# MAC
	 * and get the UPSTREAM direction transmit handler for use in sendup.
	 *      wl# MAC -> wl_sendup -> wl->fwdh->bypass_func=et_forward-> et_start
	 * NOTE: wl->fwdh is the UPSTREAM direction forwarder!
	 */
	wl->fwdh = fwder_attach(FWDER_DNSTREAM, WL_FWDER_UNIT(wl), FWDER_NIC_MODE,
	                        wl_forward, (struct net_device *)NULL, wl->osh);
#endif /* BCM_GMAC3 */

#ifdef WLC_HIGH_ONLY
#ifdef WL_THREAD
	skb_queue_head_init(&wl->tx_queue);
	skb_queue_head_init(&wl->rpc_queue);
	init_waitqueue_head(&wl->thread_wqh);
#endif /* WL_THREAD */
	wl->rpc_th = bcm_rpc_tp_attach(osh, NULL);
	if (wl->rpc_th == NULL) {
		WL_ERROR(("wl%d: %s: bcm_rpc_tp_attach failed!\n", unit, __FUNCTION__));
		goto fail;
	}

	wl->rpc = bcm_rpc_attach(NULL, osh, wl->rpc_th, &device);
	if (wl->rpc == NULL) {
		WL_ERROR(("wl%d: %s: bcm_rpc_attach failed!\n", unit, __FUNCTION__));
		goto fail;
	}

#ifdef WL_THREAD
	/* to take advantage of Multicore */
	wl->thread = kthread_create(wl_thread_dpc_wlthread, wl, "wl-thread");
	if (IS_ERR(wl->thread)) {
		goto fail;
	}
	wake_up_process(wl->thread);
#endif /* WL_THREAD */
#endif /* WLC_HIGH_ONLY */

#ifdef CONFIG_SMP
	/* initialize number of online cpus */
	online_cpus = num_online_cpus();
#if defined(BCM47XX_CA9)
	if (online_cpus > 1) {
		if (wl_txq_thresh == 0)
			wl_txq_thresh = 512;
	}
#endif /* BCM47XX_CA9 */
#else
	online_cpus = 1;
#endif /* CONFIG_SMP */

	WL_ERROR(("wl%d: online cpus %d\n", unit, online_cpus));

#ifdef WL_ALL_PASSIVE
#ifdef WLC_LOW
	wl->all_dispatch_mode = (passivemode == 0) ? TRUE : FALSE;
#endif
	if (WL_ALL_PASSIVE_ENAB(wl)) {
		/* init tx work queue for wl_start/send pkt; no need to destroy workitem  */
		MY_INIT_WORK(&wl->txq_task.work, (work_func_t)wl_start_txqwork);
		wl->txq_task.context = wl;

		/* init work queue for wl_set_multicast_list(); no need to destroy workitem  */
		MY_INIT_WORK(&wl->multicast_task.work, (work_func_t)wl_set_multicast_list_workitem);

#ifdef WLC_LOW
		MY_INIT_WORK(&wl->wl_dpc_task.work, (work_func_t)wl_dpc_rxwork);
		wl->wl_dpc_task.context = wl;
#endif
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
	else if (bustype == RPC_BUS) {
		/* Do nothing */
	} else {
		bustype = PCI_BUS;
		WL_TRACE(("force to PCI\n"));
	}
	wl->bcm_bustype = bustype;

#ifdef WLC_HIGH_ONLY
	if (wl->bcm_bustype == RPC_BUS) {
		wl->regsva = (void *)0;
		btparam = wl->rpc;
	} else
#endif /* WLC_HIGH_ONLY */
	if ((wl->regsva = ioremap_nocache(dev->base_addr, PCI_BAR0_WINSZ)) == NULL) {
		WL_ERROR(("wl%d: ioremap() failed\n", unit));
		goto fail;
	}

#ifdef WLOFFLD
	wl->bar1_addr = bar1_addr;
	wl->bar1_size = bar1_size;
#endif

#ifdef WLC_HIGH_ONLY
	spin_lock_init(&wl->rpcq_lock);
#else
	spin_lock_init(&wl->lock);
	spin_lock_init(&wl->isr_lock);
#endif /* WLC_HIGH_ONLY */

	if (WL_ALL_PASSIVE_ENAB(wl))
		sema_init(&wl->sem, 1);

	spin_lock_init(&wl->txq_lock);

	/* common load-time initialization */
	if (!(wl->wlc = wlc_attach((void *) wl, vendor, device, unit, wl->piomode,
		osh, wl->regsva, wl->bcm_bustype, btparam, NULL, &err))) {
		printf("wl driver %s failed with code %d\n", EPI_VERSION_STR, err);
		goto fail;
	}
	wl->pub = wlc_pub(wl->wlc);

	/* Populate wlcif of the primary interface in wlif */
	wlif->wlcif = wlc_wlcif_get_by_index(wl->wlc, 0);

#ifdef WLC_HIGH_ONLY
	err = wlc_iovar_getint(wl->wlc, "rpc_dngl_agglimit", &dngl_agglimit);
	if (!err && (dngl_agglimit & BCM_RPC_TP_DNGL_AGG_MASK)) {
		dbus_config_t config;
		config.config_id = DBUS_CONFIG_ID_TXRXQUEUE;
		config.txrxqueue.maxrxq = BCM_RPC_TP_DBUS_NRXQ;
		config.txrxqueue.rxbufsize = dngl_agglimit & BCM_RPC_TP_DNGL_AGG_MASK;
		config.txrxqueue.maxtxq = BCM_RPC_TP_DBUS_NTXQ;
		config.txrxqueue.txbufsize = 0;
		if (DBUS_OK != bcm_rpc_tp_set_config(wl->rpc_th, &config)) {
			WL_ERROR(("set tx/rx queue size and buffersize failed\n"));
		}
#ifdef CTFPOOL
		ctfpoolobjsz = dngl_agglimit & BCM_RPC_TP_DNGL_AGG_MASK;
#endif /* CTFPOOL */
	} else {
		WL_ERROR(("get rpc_dngl_agglimit from client failed\n"));
	}

#ifdef OSLREGOPS
	REGOPSSET(osh, (osl_rreg_fn_t)wlc_reg_read, (osl_wreg_fn_t)wlc_reg_write, wl->wlc);
#endif
	device_speed = bcm_rpc_tp_get_device_speed(wl->rpc_th);
	if ((device_speed != FULL_SPEED) && (device_speed != LOW_SPEED)) {
		WL_TRACE(("%s: HIGH SPEED USB DEVICE\n", __FUNCTION__));
#ifdef BMAC_ENABLE_LINUX_HOST_RPCAGG
		wlc_iovar_setint(wl->wlc, "rpc_agg", BCM_RPC_TP_DNGL_AGG_DPC
						| BCM_RPC_TP_HOST_AGG_AMPDU);
		wlc_iovar_setint(wl->wlc, "rpc_host_agglimit",
			wl->pub->host_rpc_agg_size);
		wlc_iovar_setint(wl->wlc, "ampdu_mpdu",
			wl->pub->ampdu_mpdu);
#endif /* BMAC_ENABLE_LINUX_HOST_RPCAGG */
	} else {
		WL_ERROR(("%s: LOW SPEED USB DEVICE\n", __FUNCTION__));
		/* for slow bus like 12Mbps, it hurts to run ampdu */
		WL_TRACE(("%s: LEGACY USB DEVICE\n", __FUNCTION__));
		wlc_iovar_setint(wl->wlc, "ampdu_manual_mode", 1);
		wlc_iovar_setint(wl->wlc, "ampdu_rx_factor", AMPDU_RX_FACTOR_8K);
		wlc_iovar_setint(wl->wlc, "ampdu_ba_wsize", 8);
	}

	wl->rpc_dispatch_ctx.rpc = wl->rpc;
	wl->rpc_dispatch_ctx.wlc = wl->wlc;
#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_TXNOCOPY) || defined(BCM_RPC_TOC)
	bcm_rpc_rxcb_init(wl->rpc, wl, wl_rpc_dispatch_schedule, wl, wl_rpc_down,
		NULL, wl_rpc_txdone);
#else
	bcm_rpc_rxcb_init(wl->rpc, wl, wl_rpc_dispatch_schedule, wl, wl_rpc_down,
		NULL, NULL);
#endif
#endif /* WLC_HIGH_ONLY */

	if (nompc) {
		if (wlc_iovar_setint(wl->wlc, "mpc", 0)) {
			WL_ERROR(("wl%d: Error setting MPC variable to 0\n", unit));
		}
	}

#ifdef PCOEM_LINUXSTA
	/* Override default in order to work with NetworkManager */
	wlc_iovar_setint(wl->wlc, "scan_passive_time", 170);

	/* Lower the users requested power. Some users don't know this is only
	   users requested power not actual and freak when they see 32 dbm.
	*/
	wlc_iovar_setint(wl->wlc, "qtxpower", 23 * 4);
#endif

#if !defined(LINUX_HYBRID) && defined(CONFIG_PROC_FS)
	/* create /proc/net/wl<unit> */
	sprintf(tmp, "net/wl%d", wl->pub->unit);
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
#endif /* #if !defined(LINUX_HYBRID) && defined(CONFIG_PROC_FS) */

	bcopy(&wl->pub->cur_etheraddr, dev->dev_addr, ETHER_ADDR_LEN);


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

#ifdef HNDCTF
	sprintf(ctf_name, "wl%d", unit);
	wl->cih = ctf_attach(osh, ctf_name, &wl_msg_level, wl_ctf_detach, wl);
#endif /* HNDCTF */

#ifdef CTFPOOL
	/* create packet pool with specified number of buffers */
	ctfpoolsz = (IS_DEV_AC3X3(device) || IS_DEV_AC2X2(device)) ? CTFPOOLSZ_AC3X3 : CTFPOOLSZ;
#ifdef WLC_HIGH_ONLY
	if (ctfpoolobjsz < DBUS_RX_BUFFER_SIZE_RPC)
		ctfpoolobjsz = DBUS_RX_BUFFER_SIZE_RPC;
	if (CTF_ENAB(wl->cih) && (num_physpages >= 8192) &&
	    (osl_ctfpool_init(osh, CTFPOOLSZ, ctfpoolobjsz+BCMEXTRAHDROOM) < 0))
#else
	BCM_REFERENCE(ctfpoolobjsz);
	if (CTF_ENAB(wl->cih) && (num_physpages >= 8192) &&
	    (osl_ctfpool_init(osh, ctfpoolsz, RXBUFSZ+BCMEXTRAHDROOM) < 0))
#endif /* WLC_HIGH_ONLY */
	{
		WL_ERROR(("wl%d: wlc_attach: osl_ctfpool_init failed\n", unit));
		goto fail;
	}
#endif /* CTFPOOL */

#ifdef WLC_LOW
	/* register our interrupt handler */
	{
		if (request_irq(irq, wl_isr, IRQF_SHARED, dev->name, wl)) {
			WL_ERROR(("wl%d: request_irq() failed\n", unit));
			goto fail;
		}
		dev->irq = irq;

#if defined(BCM_GMAC3)
		/* Setup radios irq smp affinity: Interrupt is in UPSTREAM direction. */
		fwder_affinity(FWDER_UPSTREAM, WL_FWDER_UNIT(wl), irq);
#endif /* BCM_GMAC3 */
	}
#endif /* WLC_LOW */

#if defined(USE_IW)
	WL_ERROR(("Using Wireless Extension\n"));
#endif

#if defined(USE_CFG80211)
	wl_preinit_ioctls(dev);
	parentdev = NULL;
	if (wl->bcm_bustype == PCI_BUS) {
		struct pci_dev *pci_dev = (struct pci_dev *)btparam;
		if (pci_dev != NULL)
			SET_NETDEV_DEV(dev, &pci_dev->dev);
		printk("PRDP 4: arriving here\n");
		parentdev = &((struct pci_dev *)btparam)->dev;
	}
#if defined(WLC_HIGH_ONLY)
	else if (wl->bcm_bustype == RPC_BUS) {
		struct usb_device *usb_dev;
		WL_TRACE(("%s: bustype == RPC_BUS \n", __FUNCTION__));
		usb_dev = (struct usb_device *)bcm_rpc_tp_get_devinfo(wl->rpc_th);
		WL_TRACE(("%s: Calling wl_preinit_ioctls() \n", __FUNCTION__));
		wl_preinit_ioctls(dev);

		if (usb_dev != NULL) {
			SET_NETDEV_DEV(dev, &(usb_dev->dev));
			parentdev = &(usb_dev->dev);
		}
	}
#endif /* #if defined(WLC_HIGH_ONLY) */
	else {
		WL_ERROR(("%s: Unsupported bus type %d \n", __FUNCTION__, wl->bcm_bustype));
		goto fail;
	}

	if (parentdev) {
		WL_TRACE(("%s: Calling wl_cfg80211_attach() \n", __FUNCTION__));
#if defined(USE_CFG80211) && defined(WLC_HIGH)
		if (wl_cfg80211_attach(dev, parentdev, wl)) {
#else
		if (wl_cfg80211_attach(dev, parentdev)) {
#endif
			goto fail;
		}
	}
	else {
		WL_ERROR(("%s: parentdev == NULL \n", __FUNCTION__));
		goto fail;
	}
#else

	if (wl->bcm_bustype == PCI_BUS) {
		struct pci_dev *pci_dev = (struct pci_dev *)btparam;
		if (pci_dev != NULL)
			SET_NETDEV_DEV(dev, &pci_dev->dev);
	}
#endif /* defined(USE_CFG80211) */

#if defined(BCMDBUS)
#ifdef WLC_HIGH_ONLY
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	if (wl->bcm_bustype == RPC_BUS) {
		struct usb_device *usb_dev = (struct usb_device *)
			bcm_rpc_tp_get_devinfo(wl->rpc_th);
		if (usb_dev != NULL) {
			SET_NETDEV_DEV(dev, &(usb_dev->dev));
		}
	}
#endif /* WLC_HIGH_ONLY */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0) */
#endif 

#ifdef WLCSO
	if (wlc_tso_support(wl->wlc))
		dev->features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
#endif /* WLCSO */

	if (register_netdev(dev)) {
		WL_ERROR(("wl%d: register_netdev() failed\n", unit));
		goto fail;
	}
	wlif->dev_registered = TRUE;

#if defined(BCM_GMAC3)
	wlif->fwdh = fwder_bind(wl->fwdh, WL_FWDER_UNIT(wl), wlif->subunit,
	                        wlif->dev, TRUE);
#endif /* BCM_GMAC3 */

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

#if defined(PCOEM_LINUXSTA)
	/* set led default duty-cycle */
	if (wlc_iovar_setint(wl->wlc, "leddc", 0xa0000)) {
		WL_ERROR(("wl%d: Error setting led duty-cycle\n", unit));
	}
	if (wlc_set(wl->wlc, WLC_SET_PM, PM_FAST)) {
		WL_ERROR(("wl%d: Error setting PM variable to FAST PS\n", unit));
	}
	/* turn off vlan by default */
	if (wlc_iovar_setint(wl->wlc, "vlan_mode", OFF)) {
		WL_ERROR(("wl%d: Error setting vlan mode OFF\n", unit));
	}
	/* default infra_mode is Infrastructure */
	if (wlc_set(wl->wlc, WLC_SET_INFRA, 1)) {
		WL_ERROR(("wl%d: Error setting infra_mode to infrastructure\n", unit));
	}
#endif /* PCOEM_LINUXSTA */

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


#if defined(LINUX_HYBRID)
	wl_reg_proc_entry(wl);

	/* print hello string */
	if (device > 0x9999)
		printf("%s: Broadcom BCM%d 802.11 Hybrid Wireless Controller " EPI_VERSION_STR,
			dev->name, device);
	else
		printf("%s: Broadcom BCM%04x 802.11 Hybrid Wireless Controller " EPI_VERSION_STR,
			dev->name, device);
#else /* LINUX_HYBRID */
	if (device > 0x9999)
		printf("%s: Broadcom BCM%d 802.11 Wireless Controller " EPI_VERSION_STR,
			dev->name, device);
	else
		printf("%s: Broadcom BCM%04x 802.11 Wireless Controller " EPI_VERSION_STR,
			dev->name, device);
#endif /* LINUX_HYBRID */

	printf("\n");

	wl_found++;
	return wl;

fail:
	wl_free(wl);
	return NULL;
}

#ifdef BCMDBUS
dbus_extdl_t dbus_extdl = { 0 };

static void *
wl_dbus_probe_cb(void *arg, const char *desc, uint32 bustype, uint32 hdrlen)
{
	wl_info_t *wl;
	WL_ERROR(("%s: \n", __FUNCTION__));

	if (!(wl = wl_attach(BCM_DNGL_VID, BCM_DNGL_BDC_PID, (ulong)NULL,
		RPC_BUS, NULL, 0, NULL, 0))) {
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
#ifdef WLC_HIGH_ONLY
		if (bcm_rpc_is_asleep(wl->rpc)) {
			int fw_reload = 0;
			if (!bcm_rpc_resume(wl->rpc, &fw_reload)) {
				WL_ERROR(("wl%d: wl_up fail to resume RPC\n",
					wl->pub->unit));
			}
		}
		bcm_rpc_dngl_suspend_enable_set(wl->rpc, FALSE);
#endif
		WL_LOCK(wl);
		wl_down(wl);
		WL_UNLOCK(wl);
#ifdef WLC_HIGH_ONLY
		wlc_device_removed(wl->wlc);
		wlc_bmac_dngl_reboot(wl->rpc);
		bcm_rpc_down(wl->rpc);
		wl_found--;
#endif
		wl_free(wl);
	}
}
#endif /* BCMDBUS */

#if !defined(LINUX_HYBRID) && defined(CONFIG_PROC_FS)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static int
wl_read_proc(struct seq_file *m, void *v)
{
	wl_info_t *wl;
	char buffer[1016] = {0};

	wl = (wl_info_t *)v;

	WL_LOCK(wl);
	/* pass space delimited variables for dumping */
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
}
#endif /* #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)) */
#endif /* #if !defined(LINUX_HYBRID) && defined(CONFIG_PROC_FS) */

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
	uint32 val;
	uint32 bar1_size = 0;
	void* bar1_addr = NULL;

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

	rc = pci_enable_device(pdev);
	if (rc) {
		WL_ERROR(("%s: Cannot enable device %d-%d_%d\n", __FUNCTION__,
			pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn)));
		return (-ENODEV);
	}
	pci_set_master(pdev);

	/*
	 * Disable the RETRY_TIMEOUT register (0x41) to keep
	 * PCI Tx retries from interfering with C3 CPU state.
	 * Code taken from ipw2100 driver
	 */
	pci_read_config_dword(pdev, 0x40, &val);
	if ((val & 0x0000ff00) != 0)
		pci_write_config_dword(pdev, 0x40, val & 0xffff00ff);
#ifdef WLOFFLD
		bar1_size = pci_resource_len(pdev, 2);
		bar1_addr = (uchar *)ioremap_nocache(pci_resource_start(pdev, 2),
			bar1_size);
#endif
	wl = wl_attach(pdev->vendor, pdev->device, pci_resource_start(pdev, 0), PCI_BUS, pdev,
		pdev->irq, bar1_addr, bar1_size);

	if (!wl)
		return -ENODEV;

	pci_set_drvdata(pdev, wl);

	return 0;
}

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
}

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
	suspend:	wl_suspend,
	resume:		wl_resume,
	remove:		__devexit_p(wl_remove),
	id_table:	wl_id_table,
	};
#endif /* CONFIG_PCI */




#if defined(BCM_DNGL_EMBEDIMAGE)


#define wl_read_nvram_file() do {} while (0)

#define usbap_parse_nvram()	do {} while (0)
#endif /* BCM_DNGL_EMBEDIMAGE */

/**
 * This is the main entry point for the WL driver.
 *
 * This function determines if a device pointed to by pdev is a WL device,
 * and if so, performs a wl_attach() on it.
 *
 */
static int __init
wl_module_init(void)
{
	int error = -ENODEV;


#if defined(WL_ALL_PASSIVE) && defined(WLC_LOW)
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
#endif /* defined(WL_ALL_PASSIVE) && defined(WLC_LOW) */


#if defined(WLC_HIGH_ONLY)
#ifdef WL_LIMIT_RPCQ
	{
		char *var = getvar(NULL, "wl_rpcq_rxthresh");
		if (var)
			wl_rpcq_rxthresh = bcm_strtoul(var, NULL, 0);
	}
#endif /* WL_LIMIT_RPCQ */
#endif /* WLC_HIGH_ONLY */

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

	return (error);
}

/**
 * This function unloads the WL driver from the system.
 *
 * This function unconditionally unloads the WL driver module from the
 * system.
 *
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
}

module_init(wl_module_init);
module_exit(wl_module_exit);

/**
 * This function frees the WL per-device resources.
 *
 * This function frees resources owned by the WL device pointed to
 * by the wl parameter.
 *
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
		if (wl->dev && wl->dev->irq)
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

#ifdef ARPOE
	wl_arp_detach(wl->arpi);
#endif

#if defined(BCM_GMAC3)
	wl->fwdh = fwder_dettach(wl->fwdh, FWDER_DNSTREAM, WL_FWDER_UNIT(wl));
#endif /* BCM_GMAC3 */

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
#if !defined(LINUX_HYBRID) && defined(CONFIG_PROC_FS)
		if ((wl->proc_entry != NULL) && (wl->pub != NULL)) {
			/* remove /proc/net/wl<unit> */
			char tmp[32];
			(void)snprintf(tmp, sizeof(tmp), "net/wl%d", wl->pub->unit);
			tmp[sizeof(tmp) - 1] = '\0';
			WL_INFORM(("wl%d: Removing the proc entry %s \n", wl->pub->unit, tmp));
			remove_proc_entry(tmp, 0);
		}
#endif /* defined(CONFIG_PROC_FS) */
#ifdef LINUX_HYBRID
		{
		char tmp1[128];
		sprintf(tmp1, "%s%d", HYBRID_PROC, wl->pub->unit);
		remove_proc_entry(tmp1, 0);
		}
#endif /* LINUX_HYBRID */
		wlc_detach(wl->wlc);
		wl->wlc = NULL;
		wl->pub = NULL;
	}

	/* virtual interface deletion is deferred so we cannot spinwait */

	/* wait for all pending callbacks to complete */
	while (atomic_read(&wl->callbacks) > 0)
		schedule();

#if defined(USE_CFG80211) && defined(WLC_HIGH)
		WL_TRACE(("%s: Calling wl_cfg80211_detach() \n", __FUNCTION__));
		wl_cfg80211_detach(NULL);
#endif

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
#ifdef WLOFFLD
	if (wl->bar1_addr) {
		iounmap(wl->bar1_addr);
		wl->bar1_addr = NULL;
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

#ifdef WLC_HIGH_ONLY

#ifdef WL_THREAD
	if (wl->thread) kthread_stop(wl->thread);
	wl->thread = NULL;
#endif /* WL_THREAD */
	wl_rpcq_free(wl);
	if (wl->rpc) {
		bcm_rpc_detach(wl->rpc);
		wl->rpc = NULL;
	}

	if (wl->rpc_th) {
		bcm_rpc_tp_detach(wl->rpc_th);
		wl->rpc_th = NULL;
	}
#endif /* WLC_HIGH_ONLY */

	wl_txq_free(wl);


#ifdef CTFPOOL
	/* free the buffers in fast pool */
	osl_ctfpool_cleanup(wl->osh);
#endif /* CTFPOOL */

#ifdef HNDCTF
	/* free ctf resources */
	if (wl->cih)
		ctf_detach(wl->cih);
#endif /* HNDCTF */

	MFREE(osh, wl, sizeof(wl_info_t));

#ifdef BCMDBG_CTRACE
	PKT_CTRACE_DUMP(osh, NULL);
#endif
	if (MALLOCED(osh)) {
		printf("Memory leak of bytes %d\n", MALLOCED(osh));
		ASSERT(0);
	}


	osl_detach(osh);
}

static int
wl_open(struct net_device *dev)
{
	wl_info_t *wl;
	int error = 0;

	if (!dev)
		return -ENETDOWN;

	wl = WL_INFO(dev);

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
}

static int
wl_close(struct net_device *dev)
{
	wl_info_t *wl;

	if (!dev)
		return -ENETDOWN;

#if defined(USE_CFG80211)
	wl_cfg80211_down(dev);
#endif
	wl = WL_INFO(dev);

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
}

#ifdef ARPOE
/* Return the proper arpi pointer for either corr to an IF or
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

/* used by the ARPOE module to get the ARPI context */
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

static int BCMFASTPATH
wl_start_int(wl_info_t *wl, wl_if_t *wlif, struct sk_buff *skb)
{
	void *pkt;

	WL_TRACE(("wl%d: wl_start: len %d data_len %d summed %d csum: 0x%x\n",
		wl->pub->unit, skb->len, skb->data_len, skb->ip_summed, (uint32)skb->csum));

	WL_LOCK(wl);

	/* Convert the packet. Mainly attach a pkttag */
	pkt = PKTFRMNATIVE(wl->osh, skb);
	ASSERT(pkt != NULL);

#ifdef ARPOE
	/* Arp agent */
	if (ARPOE_ENAB(wl->pub)) {
		wl_arp_info_t *arpi = wl_get_arpi(wl, wlif);
		if (arpi) {
			if (wl_arp_send_proc(arpi, pkt) ==
				ARP_REPLY_HOST) {
				PKTFREE(wl->osh, pkt, TRUE);
				WL_UNLOCK(wl);
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

#ifdef WLCSO
	/* TSO related flags */
	if (skb->ip_summed != CHECKSUM_NONE) {
		WLPKTTAG(pkt)->flags3 |= WLF3_HW_CSUM;
	}
#endif /* WLCSO */

	/* Fix the priority if WME is enabled */
	if (WME_ENAB(wl->pub) && (PKTPRIO(pkt) == 0))
		pktsetprio(pkt, FALSE);

#if defined(WLC_HIGH_ONLY) && defined(CTFMAP)
	/* Clear the ctf buf flag to allow full dma map */
	PKTCLRCTF(wl->osh, pkt);
	CTFMAPPTR(wl->osh, pkt) = NULL;
#endif /* WLC_HIGH_ONLY && CTFMAP */

#ifdef PLC
#ifdef PLC_WET
	if (PLC_ENAB(wl->pub) && !WLIF_IS_WDS(wlif)) {
		if (wl_plc_send_proc(wl, skb, wlif) == BCME_OK) {
			WL_UNLOCK(wl);
			return 0;
		}
	}
#else /* PLC_WET */
	/* Try to send the frame over PLC first, if either PLC is down
	 * or if it is determined that WDS is best path to use then the
	 * frame will be looped back to us. Otherwise the frame will be
	 * sent over PLC.
	 */
	if (PLC_ENAB(wl->pub) && WLIF_IS_WDS(wlif)) {
		struct sk_buff *nskb;

		/* Add the VLAN headers before sending to PLC */
		nskb = wl_plc_tx_prep(wlif, skb);
		if (nskb == NULL) {
			WL_UNLOCK(wl);
			return 0;
		}

		WL_PLC("%s: WDS sending to PLC using VID %d\n",
		       __FUNCTION__, wlif->plc->tx_vid);

		/* Send to PLC */
		ASSERT(wlif->plc->plc_dev != NULL);
		nskb = PKTTONATIVE(wl->osh, nskb);
		wl_plc_sendpkt(wlif, nskb, wlif->plc->plc_dev);

		/* Broadcast frames are mirrored to PLC as well as WDS whereas
		 * unicast and multicast frames are sent to PLC only.
		 */
		if (ETHER_ISUCAST(nskb->data) || !ETHER_ISBCAST(nskb->data)) {
			/* Free the original packet */
			if (nskb != skb)
				PKTFREE(wl->osh, skb, TRUE);
			WL_UNLOCK(wl);
			WL_PLC("%s: Sent to PLC only\n", __FUNCTION__);
			return 0;
		}

		if (!wl->pub->up) {
			PKTFREE(wl->osh, skb, TRUE);
			WL_UNLOCK(wl);
			WL_PLC("%s: Dropping frame since WDS is down\n", __FUNCTION__);
			return 0;
		}

		WL_PLC("%s: Also sending untagged to WDS\n", __FUNCTION__);
	}
#endif /* PLC_WET */
#endif /* PLC */

#ifndef LINUX_POSTMOGRIFY_REMOVAL
	/* Mark this pkt as coming from host/bridge. */
	WLPKTTAG(pkt)->flags |= WLF_HOST_PKT;
#endif

#ifdef WLC_HIGH_ONLY
	if (!wl->pub->up)
		PKTFREE(wl->osh, skb, TRUE);
	else
#endif
		wlc_sendpkt(wl->wlc, pkt, wlif->wlcif);

	WL_UNLOCK(wl);

	return (0);
}

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
/* Schedule a completion handler to run at safe time */
static int
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

	if (!schedule_work(&task->work)) {
		WL_ERROR(("wl%d: schedule_work() failed\n", wl->pub->unit));
		MFREE(wl->osh, task, sizeof(wl_task_t));
		return -ENOMEM;
	}

	atomic_inc(&wl->callbacks);

	return 0;
}
#endif /* defined(AP) || defined(WL_ALL_PASSIVE) || defined(WL_MONITOR) */

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
	if (!(wlif = MALLOC(wl->osh, sizeof(wl_if_t)))) {
		WL_ERROR(("wl%d: wl_alloc_if: out of memory, malloced %d bytes\n",
			(wl->pub)?wl->pub->unit:subunit, MALLOCED(wl->osh)));
		return NULL;
	}
	bzero(wlif, sizeof(wl_if_t));
	wlif->wl = wl;
	wlif->wlcif = wlcif;
	wlif->subunit = subunit;
	wlif->if_type = iftype;

#ifdef PLC
	wlif->plc = NULL;
#endif /* PLC */

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
}

static void
wl_free_if(wl_info_t *wl, wl_if_t *wlif)
{
	wl_if_t *p;
#if defined(USE_CFG80211) && defined(WLC_HIGH)
	s32 pre_locked = -1;
#endif

	WL_TRACE(("%s: Started \n", __FUNCTION__));

	ASSERT(wlif);
	ASSERT(wl);

	WL_TRACE(("%s\n", __FUNCTION__));

	/* check if register_netdev was successful */
	if (wlif->dev_registered) {
		ASSERT(wlif->dev);

#if defined(BCM_GMAC3)
		wlif->fwdh = fwder_bind(wlif->fwdh, WL_FWDER_UNIT(wl), wlif->subunit,
		                        wlif->dev, FALSE);
#endif /* BCM_GMAC3 */

#ifdef HNDCTF
		if (wl->cih)
			ctf_dev_unregister(wl->cih, wlif->dev);
#endif /* HNDCTF */
#if defined(USE_CFG80211) && defined(WLC_HIGH)
		pre_locked = wl_cfg80211_ifdel_ops(wlif->dev);
		WL_TRACE(("%s: Start unregister netdev %s\n", __FUNCTION__, wlif->dev->name));
		if (rtnl_is_locked() && pre_locked == 0) {
			WL_TRACE(("%s: locked. call unregister_netdevice\n", __FUNCTION__));
			unregister_netdevice(wlif->dev);
		}
		else
#endif
		{
			unregister_netdev(wlif->dev);
		}
		wlif->dev_registered = FALSE;
		WL_TRACE(("%s: unregister netdev done %s\n", __FUNCTION__, wlif->dev->name));

#if defined(USE_CFG80211)
		WL_TRACE(("%s: Calling  wl_cfg80211_notify_ifdel() \n", __FUNCTION__));
#if defined(WLC_HIGH)
		wl_cfg80211_notify_ifdel(wlif->dev);
#else
		wl_cfg80211_detach(wlif->dev);
#endif
#endif /* #if defined(USE_CFG80211) */
	}
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
#if defined(USE_CFG80211) && defined(WLC_HIGH)
		while (wlif->dev->reg_state != NETREG_UNREGISTERED) {
			msleep(1);
		}
#endif /* #if defined(USE_CFG80211) && defined(WLC_HIGH) */
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

#ifdef PLC
	wl_plc_cleanup(wlif);
#endif /* PLC */

	MFREE(wl->osh, wlif, sizeof(wl_if_t));
}

/* Create a virtual interface. Call only from safe time!
 * can't call register_netdev with WL_LOCK
 */
/* Netdev allocator.  Only newer kernels need this to be async
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
	dev = MALLOC(wl->osh, sizeof(struct net_device));
	if (!dev) {
		WL_ERROR(("wl%d: %s: malloc of net_device failed\n",
			(wl->pub)?wl->pub->unit:wlif->subunit, __FUNCTION__));
		return NULL;
	}
	bzero(dev, sizeof(struct net_device));
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
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0))
	dev = alloc_netdev(sizeof(priv_link_t), intf_name, ether_setup);
#else
	dev = alloc_netdev(sizeof(priv_link_t), intf_name, NET_NAME_UNKNOWN, ether_setup);
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
}

#ifdef LINUX_HYBRID
struct wl_if *
wl_add_if(wl_info_t *wl, struct wlc_if *wlcif, uint unit, struct ether_addr *remote)
{
	WL_ERROR(("%s: not supported\n", __FUNCTION__));
	return NULL;
}

void
wl_del_if(wl_info_t *wl, wl_if_t *wlif)
{
	WL_ERROR(("%s: not supported\n", __FUNCTION__));
	return;
}
#else /* !LINUX_HYBRID */
static wlc_bsscfg_t *
wl_bsscfg_find(wl_if_t *wlif)
{
	wl_info_t *wl = wlif->wl;
	return wlc_bsscfg_find_by_wlcif(wl->wlc, wlif->wlcif);
}


static void
_wl_add_if(wl_task_t *task)
{
	wl_if_t *wlif = (wl_if_t *)task->context;
	wl_info_t *wl = wlif->wl;
	struct net_device *dev;
	wlc_bsscfg_t *cfg;
#if defined(USE_CFG80211) && defined(WLC_HIGH)
	s32 pre_locked = -1;
#endif

	WL_TRACE(("%s\n", __FUNCTION__));

	/* alloc_netdev and populate priv_link */
	if ((dev = wl_alloc_linux_if(wlif)) == NULL) {
		WL_ERROR(("%s: Call to  wl_alloc_linux_if failed\n", __FUNCTION__));
		goto done;
	}

	/* Copy temp to real name */
	ASSERT(strlen(wlif->name) > 0);
	strncpy(wlif->dev->name, wlif->name, strlen(wlif->name));
	wlif->dev->name[strlen(wlif->name)] = '\0';

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

#if defined(PLC) && !defined(PLC_WET)
	WL_LOCK(wl);
	if (WLIF_IS_WDS(wlif))
		wl_plc_init(wlif);
	WL_UNLOCK(wl);
#endif /* PLC */

#if defined(USE_CFG80211) && defined(WLC_HIGH)
	WL_TRACE(("%s: Start register_netdev() %s\n", __FUNCTION__, wlif->name));
	pre_locked = wl_cfg80211_setup_vwdev(dev, 0, P2PAPI_BSSCFG_CONNECTION);
	if (pre_locked == -1)
	{
		WL_ERROR(("%s: Setup cfg80211 netdev failed. name=%s\n", __FUNCTION__, wlif->name));
		goto done;
	}
	if (rtnl_is_locked() && (pre_locked == 0)) {
		WL_TRACE(("%s: it is locked, name=%s\n", __FUNCTION__, wlif->name));
		if (register_netdevice(dev)) {
			WL_ERROR(("wl%d: wl_add_if: register_netdev() failed for \"%s\"\n",
				wl->pub->unit, dev->name));
			goto done;
		}
	}
	else
#endif /* #if defined(USE_CFG80211) && defined(WLC_HIGH) */
	{
		if (register_netdev(dev)) {
			WL_ERROR(("wl%d: wl_add_if: register_netdev() failed for \"%s\"\n",
				wl->pub->unit, dev->name));
			goto done;
		}
	}
	WL_TRACE(("%s: register_netdev succeed\n", __FUNCTION__));
	wlif->dev_registered = TRUE;

#if defined(BCM_GMAC3)
	/* wlif->dev is now in wl%d.%d <unit.subunit> format */
	wlif->fwdh = fwder_bind(wl->fwdh, WL_FWDER_UNIT(wl), wlif->subunit,
	                        wlif->dev, TRUE);
#endif /* BCM_GMAC3 */

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
}

/* Schedule _wl_add_if() to be run at safe time. */
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
#if defined(USE_CFG80211) && defined(WLC_HIGH)
	if (wl_cfg80211_query_if_name(wlif->name) == -1)
	{
		WL_TRACE(("wpa virtual interface name does not exist. Change to %s\n", wlif->name));
		sprintf(wlif->name, "%s%d.%d", devname, wl->pub->unit, wlif->subunit);
	}
	else {
		WL_TRACE(("wpa virtual interface name exist. (%s)\n", wlif->name));
	}
#else
	sprintf(wlif->name, "%s%d.%d", devname, wl->pub->unit, wlif->subunit);
#endif /* #if defined(USE_CFG80211) && defined(WLC_HIGH) */

	if (wl_schedule_task(wl, _wl_add_if, wlif)) {
		MFREE(wl->osh, wlif, sizeof(wl_if_t) + sizeof(struct net_device));
		return NULL;
	}

	return wlif;
}

/* Remove a virtual interface. Call only from safe time! */
static void
_wl_del_if(wl_task_t *task)
{
	wl_if_t *wlif = (wl_if_t *) task->context;
	wl_info_t *wl = wlif->wl;

	wl_free_if(wl, wlif);

	MFREE(wl->osh, task, sizeof(wl_task_t));
	atomic_dec(&wl->callbacks);
}

/* Schedule _wl_del_if() to be run at safe time. */
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
#endif /* !LINUX_HYBRID */

/* Return pointer to interface name */
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
#ifdef WLC_LOW
	uint32 macintmask;
#endif /* WLC_LOW */

	WL_TRACE(("wl%d: wl_reset\n", wl->pub->unit));

#ifdef WLC_LOW
	/* disable interrupts */
	macintmask = wl_intrsoff(wl);
#endif /* WLC_LOW */

	wlc_reset(wl->wlc);

#ifdef WLC_LOW
	/* restore macintmask */
	wl_intrsrestore(wl, macintmask);
#endif /* WLC_LOW */

	/* dpc will not be rescheduled */
	wl->resched = 0;

	return (0);
}

/*
 * These are interrupt on/off entry points. Disable interrupts
 * during interrupt state transition.
 */
void BCMFASTPATH
wl_intrson(wl_info_t *wl)
{
#if defined(WLC_LOW)
	unsigned long flags = 0;

	INT_LOCK(wl, flags);
	wlc_intrson(wl->wlc);
	INT_UNLOCK(wl, flags);
#endif
}

bool
wl_alloc_dma_resources(wl_info_t *wl, uint addrwidth)
{
	return TRUE;
}

uint32 BCMFASTPATH
wl_intrsoff(wl_info_t *wl)
{
#if defined(WLC_LOW)
	unsigned long flags = 0;
	uint32 status;

	INT_LOCK(wl, flags);
	status = wlc_intrsoff(wl->wlc);
	INT_UNLOCK(wl, flags);
	return status;
#else
	return 0;
#endif
}

void
wl_intrsrestore(wl_info_t *wl, uint32 macintmask)
{
#if defined(WLC_LOW)
	unsigned long flags = 0;

	INT_LOCK(wl, flags);
	wlc_intrsrestore(wl->wlc, macintmask);
	INT_UNLOCK(wl, flags);
#endif
}

int
wl_up(wl_info_t *wl)
{
	int error = 0;
	wl_if_t *wlif;

	WL_TRACE(("wl%d: wl_up\n", wl->pub->unit));

	if (wl->pub->up)
		return (0);

#ifdef WLC_HIGH_ONLY
	if (bcm_rpc_is_asleep(wl->rpc)) {
		int fw_reload = 0;
		if (!bcm_rpc_resume(wl->rpc, &fw_reload)) {
			WL_ERROR(("wl%d: wl_up fail to resume RPC\n", wl->pub->unit));
			return -1;
		}
	}
#endif
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

#if defined(PLC_WET) && defined(AP)
	ASSERT(wl->if_list != NULL);
	/* Cleanup before reinit */
	for (wlif = wl->if_list; wlif != NULL; wlif = wlif->next) {
		if (!WLIF_IS_WDS(wl->if_list))
			wl_plc_cleanup(wlif);
	}
	for (wlif = wl->if_list; wlif != NULL; wlif = wlif->next) {
		if (!WLIF_IS_WDS(wl->if_list))
			wl_plc_init(wlif);
	}
#endif /* PLC_WET && AP */

	return (error);
}

void
wl_down(wl_info_t *wl)
{
	wl_if_t *wlif;
	int monitor = 0;
	uint callbacks, ret_val = 0;

	WL_TRACE(("wl%d: wl_down\n", wl->pub->unit));

	for (wlif = wl->if_list; wlif != NULL; wlif = wlif->next) {
#if defined(BCM_GMAC3)
		/* Flush all stations to interface bindings. */
		if (wlif->fwdh != FWDER_NULL) {
			fwder_flush(wlif->fwdh, (uintptr_t)wlif->dev);
		}
#endif /* BCM_GMAC3 */
#ifdef PLC
		/* For PLC enabled interface we want to receive the frames
		 * from the netif layer even when we are down. This will
		 * allow us to send the frames to PLC first. If the frames
		 * are looped back to us to be sent out on wl interface then we
		 * drop them if the wl interface is still down.
		 */
		if (PLC_ENAB(wl->pub)) {
#ifdef PLC_WET
			continue;
#else
			if (WLIF_IS_WDS(wlif))
				continue;
#endif /* PLC_WET */
		}
#endif /* PLC */
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

#ifndef WLC_HIGH_ONLY
#ifdef WL_ALL_PASSIVE
	if (WL_ALL_PASSIVE_ENAB(wl)) {
		int i = 0;
		for (i = 0; (atomic_read(&wl->callbacks) > callbacks) && i < 10000; i++) {
			schedule();
			flush_scheduled_work();
		}
	}
	else
#endif /* WL_ALL_PASIVE */
	{
		/* For HIGH_only driver, it's important to actually schedule other work,
		 * not just spin wait since everything runs at schedule level
		 */
		SPINWAIT((atomic_read(&wl->callbacks) > callbacks), 100 * 1000);
	}
#else
	if (!(WOWL_ENAB(wl->pub) && WOWL_ACTIVE(wl->pub))) {
		bcm_rpc_sleep(wl->rpc);
	} else {
		WL_TRACE(("%s: bcm_rpc is NOT sleeping \n", __FUNCTION__));
	}
#endif /* WLC_HIGH_ONLY */

	WL_LOCK(wl);
}
#ifdef WLC_HIGH_ONLY
#ifdef WL_WOWL_MEDIA
void
wl_down_postwowlenab(wl_info_t *wl)
{
	if ((WOWL_ENAB(wl->pub) && WOWL_ACTIVE(wl->pub))) {
		WL_TRACE(("%s: bcm_rpc is sleeping \n", __FUNCTION__));
		bcm_rpc_sleep(wl->rpc);
	} else {
		WL_ERROR(("%s: Unexpected post wowl enable\n", __FUNCTION__));
		ASSERT(0);
	}

}
#endif /* WL_WOWL_MEDIA */
#endif /* WLC_HIGH_ONLY */

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
	wl_info_t *wl = WL_INFO(dev);

#if WIRELESS_EXT >= 19 || LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	if (!wl || !wl->pub || !wl->wlc || !wl->dev)
		return;
#endif
	bzero(info, sizeof(struct ethtool_drvinfo));
	snprintf(info->driver, sizeof(info->driver), "wl%d", wl->pub->unit);
	strncpy(info->version, EPI_VERSION_STR, sizeof(info->version));
	info->version[(sizeof(info->version))-1] = '\0';
}

#ifdef WLCSO
static int
wl_set_tx_csum(struct net_device *dev, uint32 on_off)
{
	wl_info_t *wl = WL_INFO(dev);

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
}

int
wl_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	wl_info_t *wl;
	wl_if_t *wlif;
	void *buf = NULL;
	wl_ioctl_t ioc;
	int bcmerror;

	if (!dev)
		return -ENETDOWN;

	wl = WL_INFO(dev);
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

#if defined(USE_CFG80211) && defined(BCMDONGLEHOST)
	if (cmd == SIOCDEVPRIVATE+1) {
		WL_TRACE(("%s: wl%d: Call wl_cfgp2p_priv_ioctl() \n", __FUNCTION__, wl->pub->unit));
		bcmerror = wl_cfgp2p_priv_ioctl(dev, ifr, cmd);
		WL_TRACE(("wl%d: wl_ioctl: ret %d \n", wl->pub->unit, bcmerror));
		return bcmerror;
	}
#endif /* #if defined(USE_CFG80211) && defined(WLC_HIGH) */

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
	if (!capable(CAP_NET_ADMIN)) {
		bcmerror = BCME_EPERM;
	} else {
		bcmerror = wlc_ioctl(wl->wlc, ioc.cmd, buf, ioc.len, wlif->wlcif);
	}
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
}

static struct net_device_stats*
wl_get_stats(struct net_device *dev)
{
	struct net_device_stats *stats_watchdog = NULL;
	struct net_device_stats *stats = NULL;
	wl_info_t *wl;
	wl_if_t *wlif;

	if (!dev)
		return NULL;

	if ((wl = WL_INFO(dev)) == NULL)
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

	if ((wl = WL_INFO(dev)) == NULL)
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
}
#endif /* USE_IW */

static int
wl_set_mac_address(struct net_device *dev, void *addr)
{
	int err = 0;
	wl_info_t *wl;
	struct sockaddr *sa = (struct sockaddr *) addr;

	if (!dev)
		return -ENETDOWN;

	wl = WL_INFO(dev);

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
	if (!WL_ALL_PASSIVE_ENAB((wl_info_t *)WL_INFO(dev)))
		_wl_set_multicast_list(dev);
#ifdef WL_ALL_PASSIVE
	else {
		wl_info_t *wl = WL_INFO(dev);
		wl->multicast_task.context = dev;

		if (schedule_work(&wl->multicast_task.work)) {
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
	bool allmulti;

	if (!dev)
		return;
	wl = WL_INFO(dev);

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
		netdev_for_each_mc_addr(ha, dev) {
			if (i >= MAXMULTILIST) {
				allmulti = TRUE;
				i = 0;
				break;
			}
			bcopy(ha->addr, &maclist->ea[i++], ETHER_ADDR_LEN);
		}
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

}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
irqreturn_t BCMFASTPATH
wl_isr(int irq, void *dev_id)
#else
irqreturn_t BCMFASTPATH
wl_isr(int irq, void *dev_id, struct pt_regs *ptregs)
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20) */
{
#if defined(WLC_LOW)
	wl_info_t *wl;
	bool ours, wantdpc;
	unsigned long flags;

	wl = (wl_info_t*) dev_id;

	WL_ISRLOCK(wl, flags);

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
				if (schedule_work(&wl->wl_dpc_task.work))
					atomic_inc(&wl->callbacks);
				else
					ASSERT(0);
			} else
#endif /* WL_ALL_PASSIVE */
			tasklet_schedule(&wl->tasklet);
#endif /* NAPI_POLL */
		}
	}

	WL_ISRUNLOCK(wl, flags);

	return IRQ_RETVAL(ours);
#else
	return IRQ_RETVAL(0);
#endif /* WLC_LOW */
}

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
#ifdef WLC_LOW
	wl_info_t *wl;

#ifdef NAPI_POLL
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
	wl = (wl_info_t *)container_of(napi, wl_info_t, napi);
	wl->pub->tunables->rxbnd = min(RXBND, budget);
#else
	wl = WL_INFO(dev);
	wl->pub->tunables->rxbnd = min(RXBND, *budget);
	ASSERT(wl->pub->tunables->rxbnd <= dev->quota);
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30) */
#else /* NAPI_POLL */

	wl = (wl_info_t *)data;

	WL_LOCK(wl);
#endif /* NAPI_POLL */

	/* call the common second level interrupt handler */
	if (wl->pub->up) {
		wlc_dpc_info_t dpci = {0};

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
			if (!schedule_work(&wl->wl_dpc_task.work)) {
				/* wl_dpc_task alread in queue.
				 * Shall not reach here
				 */
				ASSERT(0);
			}
	}
	else {
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
#endif /* WLC_LOW */
}

#if defined(WL_ALL_PASSIVE) && defined(WLC_LOW)
static void BCMFASTPATH
wl_dpc_rxwork(struct wl_task *task)
{
	wl_info_t *wl = (wl_info_t *)task->context;
	WL_TRACE(("wl%d: %s\n", wl->pub->unit, __FUNCTION__));

	wl_dpc((unsigned long)wl);
	return;
}
#endif /* defined(WL_ALL_PASSIVE) && defined(WLC_LOW) */

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

/*
 * The last parameter was added for the build. Caller of
 * this function should pass 1 for now.
 */
void BCMFASTPATH
wl_sendup(wl_info_t *wl, wl_if_t *wlif, void *p, int numpkt)
{
	struct sk_buff *skb;
#ifdef HNDCTF
	struct sk_buff *nskb;
#endif
	bool brcm_specialpkt;
#if defined(BCM_GMAC3)
	fwder_t * fwdh; /* forwarder handler */
	int fwder_rx_port; /* fwder WOFA cached lookup assist */
#endif /* BCM_GMAC3 */

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
#if defined(BCM_GMAC3)
		fwdh = wlif->fwdh;
		fwder_rx_port = wlif->subunit;
#endif

#ifdef PLC
		if (PLC_ENAB(wl->pub) && WLIF_IS_WDS(wlif)) {
			WL_PLC("%s: From WDS skb %p data %p len %d plc %p\n",
			       __FUNCTION__, skb, skb->data, skb->len, wlif->plc);
			if (wl_plc_recv(skb, skb->dev, wlif->plc, WL_PLC_IF_WDS) == BCME_OK)
				return;
		}
#endif /* PLC */
	} else {
		/* Convert the packet, mainly detach the pkttag */
		skb = PKTTONATIVE(wl->osh, p);
		skb->dev = wl->dev;

#ifdef PLC_WET
		if (PLC_ENAB(wl->pub) && wl->pub->up) {
			struct ether_header *eh = (struct ether_header *)skb->data;

			if (eh->ether_type == HTON16(ETHER_TYPE_8021Q)) {
				if (wl_plc_recv(skb, skb->dev, wl->if_list->plc,
				                WL_PLC_IF_WDS) == BCME_OK)
					return;
			} else {
				/* WIFI forward to PLC. AP only, not URE */
				if (AP_ONLY(wl->pub)) {
					if (wl_plc_forward(skb, skb->dev, wl->if_list->plc,
						WL_PLC_IF_WDS) == BCME_OK)
						return;
				}
			}
		}
#endif /* PLC_WET */

#ifdef DPSTA
		if (PSTA_ENAB(wl->pub)) {
			if (dpsta_recv(skb) != BCME_OK) {
				PKTFRMNATIVE(wl->osh, skb);
				PKTFREE(wl->osh, skb, FALSE);
				return;
			}
		}
#endif /* DPSTA */
#if defined(BCM_GMAC3)
		fwdh = wl->fwdh;
		fwder_rx_port = 0; /* use primary interface */
#endif
	}

#if defined(BCM_GMAC3)
	if (fwdh) { /* upstream forwarder */
		if (fwdh->devs_cnt > 1) { /* local bridging */
			/* May we assume that the Ethernet DstAddr is 2 Byte aligned? */
			uint16 *da = (uint16*)PKTDATA(wl->osh, skb);
			ASSERT(ISALIGNED(da, 2));
			if (ETHER_ISUCAST(da)) {
				uintptr_t wofa_data;
				wofa_data = fwder_lookup(fwdh->mate, da, fwder_rx_port);
				if (wofa_data != WOFA_DATA_INVALID) {
					/* Intra|Inter BSS: forward locally */
					struct net_device * fwd_dev = (struct net_device*)wofa_data;
					wlif = WL_DEV_IF(fwd_dev);
					wl = WL_INFO(fwd_dev); /* could be second radio (Inter) */
					skb->dev = fwd_dev;
					wl_start_int(wl, wlif, skb);
					return;
				}
			} else {
				ASSERT(numpkt == 1);
				fwder_flood(fwdh->mate, skb, wl->osh, TRUE, wl_start_try);
				/* Need to also send to GMAC fwder */
			}
		}

		/* Now send using upstream forwarder to GMAC */
		if (fwder_transmit(fwdh, skb, numpkt, skb->dev) != FWDER_FAILURE) {
			PKTFRMNATIVE(wl->osh, skb);
			PKTCFREE(wl->osh, skb, FALSE);
		}

		return;
	}
#endif /* ! BCM_GMAC3 */

#ifdef HNDCTF
	/* try cut thru' before sending up */
	if (wl_ctf_forward(wl, skb) != BCME_ERROR)
		return;

sendup_next:
	PKTCLRTOBR(wl->osh, skb);
	/* clear skipct flag before sending up */
	PKTCLRSKIPCT(wl->osh, skb);

#ifdef CTFPOOL
	/* allocate and add a new skb to the pkt pool */
	if (PKTISFAST(wl->osh, skb))
		osl_ctfpool_add(wl->osh);

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
		WL_ERROR(("Unaligned assert. skb %p. skb->data %p.\n", skb, skb->data));
		if (wlif) {
			WL_ERROR(("wl_sendup: dev name is %s (wlif) \n", wlif->dev->name));
			WL_ERROR(("wl_sendup: hard header len  %d (wlif) \n",
				wlif->dev->hard_header_len));
		}
		WL_ERROR(("wl_sendup: dev name is %s (wl) \n", wl->dev->name));
		WL_ERROR(("wl_sendup: hard header len %d (wl) \n", wl->dev->hard_header_len));
		ASSERT(ISALIGNED(skb->data, 4));
	}

	/* send it up */
	WL_APSTA_RX(("wl%d: wl_sendup(): pkt %p summed %d on interface %p (%s)\n",
		wl->pub->unit, p, skb->ip_summed, wlif, skb->dev->name));

#ifdef NAPI_POLL
	netif_receive_skb(skb);
#else /* NAPI_POLL */
#if defined(USE_CFG80211) && defined(WLC_HIGH)
	if (in_interrupt()) {
		netif_rx(skb);
	}
	else
		netif_rx_ni(skb);
#elif defined(WLC_HIGH_ONLY)
	/*
	 * On BMAC driver, when 'netif_rx()' is used on an SMP host, out-of-order datagrams are
	 * seen at the receive side while running UDP traffic using iperf.  When 'netif_rx_ni()' is
	 * used, the task preemption is temporarily disabled while sending the pkt to the OS stack.
	 * This resolve the issue of 'out-of-order' packets at the receive side, which will help to
	 * improve the overall throughput performance.
	 */
	netif_rx_ni(skb);
#else
	netif_rx(skb);
#endif /* defined(USE_CFG80211) && defined(WLC_HIGH) */
#endif /* NAPI_POLL */

#ifdef HNDCTF
	if (nskb != NULL) {
		nskb->dev = skb->dev;
		skb = nskb;
		goto sendup_next;
	}
#endif /* HNDCTF */
}

#ifndef LINUX_HYBRID
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
}
#else  /* LINUX_HYBRID */
int
wl_osl_pcie_rc(struct wl_info *wl, uint op, int param)
{
	return 0;
}
#endif /* LINUX_HYBRID */

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
	case WLC_E_NDIS_LINK:
		if (e->event.flags&WLC_EVENT_MSG_LINK)
			wl_link_up(wl, ifname);
		else
			wl_link_down(wl, ifname);
		break;
#if defined(BCMSUP_PSK) && defined(STA)
	case WLC_E_MIC_ERROR: {
		wlc_bsscfg_t *cfg = wlc_bsscfg_find_by_wlcif(wl->wlc, NULL);
		if (cfg == NULL || e->event.bsscfgidx != WLC_BSSCFG_IDX(cfg))
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
	}
}

#if defined(BCM_GMAC3)
void
wl_fwder_event(wl_info_t *wl, char *ifname, wlc_event_t *e)
{
	bool attach;
	wl_if_t * wlif;

	if ((wl->fwdh == NULL) || !(e->addr) || (e->event.status != WLC_E_STATUS_SUCCESS))
		return;


	switch (e->event.event_type) {
		case WLC_E_AUTH_IND:
		case WLC_E_ASSOC_IND:
		case WLC_E_REASSOC_IND:
		case WLC_E_PRE_ASSOC_IND:
			attach = TRUE;
			break;

		case WLC_E_DEAUTH_IND:
		case WLC_E_DISASSOC_IND:
		case WLC_E_PRE_REASSOC_IND:
			attach = FALSE;
			break;

		default:
			return;
	}

	/* Find the wl_if to which this event is directed */
	for (wlif = wl->if_list; wlif != (wl_if_t *)NULL; wlif = wlif->next) {
		if ((wlif->dev != NULL) && (strcmp(ifname, wlif->dev->name) == 0))
			break;
	}

	if ((wlif == NULL) || (wlif->fwdh == FWDER_NULL))
		return;

	/* Add|Del station (Mac Address) to interface forwarding entry in WOFA */
	if (attach) {
		fwder_reassoc(wlif->fwdh, (uint16 *)e->addr, (uintptr_t)wlif->dev);
	} else {
		fwder_deassoc(wlif->fwdh, (uint16 *)e->addr, (uintptr_t)wlif->dev);
	}
}
#endif /* BCM_GMAC3 */

void
wl_event_sync(wl_info_t *wl, char *ifname, wlc_event_t *e)
{
#if defined(BCM_GMAC3)
	wl_fwder_event(wl, ifname, e);
#endif /* BCM_GMAC3 */
}

#ifdef WLC_HIGH_ONLY
static void
wl_rpc_down(void *wlh)
{
	wl_info_t *wl = (wl_info_t*)(wlh);

	wlc_device_removed(wl->wlc);

	wl_rpcq_free(wl);
}

#ifdef WL_THREAD
#define WL_THREAD_TX_Q_MAX 16000
/* enqueue pkt to local queue, schedule a task to run, return this context */
static int
wl_start_enqueue_wlthread(wl_info_t *wl, struct sk_buff *skb)
{
	if (skb_queue_len(&wl->tx_queue) > WL_THREAD_TX_Q_MAX) {
		if (skb->destructor)
			dev_kfree_skb_any(skb);
		else
			dev_kfree_skb(skb);
		return (0);
	}
	skb_queue_tail(&wl->tx_queue, skb);

	wake_up_interruptible(&wl->thread_wqh);

	return (0);
}

#define NUM_TXQ_ITEMS	10
static void
wl_start_txqwork_wlthread(wl_info_t *wl)
{
	wl_if_t *wlif;
	struct sk_buff *skb;
	uint32 count = 0;

	while ((skb = skb_dequeue(&wl->tx_queue)) != NULL) {
		wlif = WL_DEV_IF(skb->dev);

		wl_start_int(WL_INFO(skb->dev), wlif, skb);

		/* bounded our execution, reshedule ourself next */
		if (++count >= NUM_TXQ_ITEMS)
			return;
	}
	return;
}

/* transmit a packet */
static int
wl_start_wlthread(struct sk_buff *skb, struct net_device *dev)
{
	if (!dev)
		return -ENETDOWN;

	return wl_start_enqueue_wlthread(WL_INFO(dev), skb);
}
#endif /* WL_THREAD */
#endif /* WLC_HIGH_ONLY */

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

#if defined(BCM_GMAC3)
/**
 * Bypass transmit handler that will be invoked by GMAC forwarder.
 * A NIC mode forwarder may only forward a packet chain or a packet.
 * fwder is the downstream forwarder.
 */
static int BCMFASTPATH
wl_forward(fwder_t * fwder, struct sk_buff *skb, int skb_cnt, struct net_device *rx_dev)
{
	wl_if_t *wlif;
	wl_info_t *wl;
	struct net_device * dev;

	/* Locate the TX network device using default dev or by looking up WOFA */
	if (fwder->devs_cnt == 1) {
		dev = fwder->dev_def; /* use forwarder's default device */
		if (dev == NULL)
			return FWDER_FAILURE;
	} else { /* Multiple interfaces: find interface using Eth DstAddress */
		uint16 *da = (uint16 *)(skb->data);

		if (ETHER_ISUCAST(da)) { /* fetch device by looking up WOFA */
			uintptr_t wofa_data = fwder_lookup(fwder, da, FWDER_GMAC_PORT);
			if (wofa_data == WOFA_DATA_INVALID)
				return FWDER_FAILURE;
			dev = (struct net_device *)wofa_data;
		} else { /* flood to all interfaces hosted by the downstream fwder. */
			ASSERT(skb_cnt == 1);

			fwder_flood(fwder, skb, fwder->mate->osh, FALSE, wl_start_try);
			PKTFRMNATIVE(fwder->mate->osh, skb);
			PKTFREE(fwder->mate->osh, skb, TRUE);
			return FWDER_SUCCESS;
		}
	}

	wlif = WL_DEV_IF(dev);
	if ((wlif->fwdh == FWDER_NULL) || !(dev->flags & IFF_UP))
		return -ENETDOWN;
	wl = WL_INFO(dev);

	wl_start_int(wl, wlif, skb);

	return FWDER_SUCCESS;
}

static int BCMFASTPATH
wl_start_try(struct sk_buff *skb, struct net_device *dev, bool in_lock)
{
	BCM_REFERENCE(in_lock);
	return wl_start(skb, dev);
}
#endif /* BCM_GMAC3 */

/* Transmit pkt.
 * in PASSIVE mode, enqueue pkt to local queue,schedule task to run, return this context
 * in non passive mode, directly call wl_start_int() to transmit pkt
 */
#ifdef CONFIG_SMP
#define WL_CONFIG_SMP()	TRUE
#else
#define WL_CONFIG_SMP()	FALSE
#endif /* CONFIG_SMP */

static int BCMFASTPATH
wl_start(struct sk_buff *skb, struct net_device *dev)
{
	wl_if_t *wlif;
	wl_info_t *wl;

	if (!dev)
		return -ENETDOWN;

	wlif = WL_DEV_IF(dev);
	wl = WL_INFO(dev);

	/* Call in the same context when we are UP and non-passive is enabled */
	if (WL_ALL_PASSIVE_ENAB(wl) || (WL_RTR() && WL_CONFIG_SMP())) {
		skb->prev = NULL;

		/* Lock the queue as tasklet could be running at this time */
		TXQ_LOCK(wl);

		if ((wl_txq_thresh > 0) && (wl->txq_cnt >= wl_txq_thresh)) {
			PKTFRMNATIVE(wl->osh, skb);
			PKTCFREE(wl->osh, skb, TRUE);
			TXQ_UNLOCK(wl);
			return 0;
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
			else if (txworkq && online_cpus > 1)
				err = (int32)(schedule_work_on(1 - raw_smp_processor_id(),
					&wl->txq_task.work) == 0);
#endif
			else
				err = (int32)(schedule_work(&wl->txq_task.work) == 0);
#endif /* WL_ALL_PASSIVE */

			if (!err) {
				atomic_inc(&wl->callbacks);
				wl->txq_dispatched = TRUE;
			} else
				WL_ERROR(("wl%d: wl_start/schedule_work failed\n",
				          wl->pub->unit));
		}

		TXQ_UNLOCK(wl);
	} else
		return wl_start_int(wl, wlif, skb);

	return (0);
}
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

	return;
}

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

#ifdef WLC_HIGH_ONLY
static void
wl_rpcq_free(wl_info_t *wl)
{
	rpc_buf_t *buf;

	if (wl->rpcq_head == NULL) {
		ASSERT(wl->rpcq_tail == NULL);
		return;
	}

	while (wl->rpcq_head) {
		buf = wl->rpcq_head;
		wl->rpcq_head = bcm_rpc_buf_next_get(wl->rpc_th, buf);
		wl->rpcq_len--;
#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_RXNOCOPY) || defined(BCM_RPC_ROC)
		PKTFRMNATIVE(wl->osh, buf);
		PKTFREE(wl->osh, buf, FALSE);
#else
		bcm_rpc_buf_free(wl->rpc_dispatch_ctx.rpc, buf);
#endif /* BCM_RPC_NOCOPY || BCM_RPC_RXNOCOPY */
	}

	wl->rpcq_tail = NULL;
}
#ifdef WL_THREAD
#define NUM_RXQ_ITEMS	10
void
wl_rpcq_dispatch_wlthread(wl_info_t *wl)
{
	rpc_buf_t *buf;
	struct sk_buff *skb;
	ulong flags;
	uint32 count = 0;

	RPCQ_LOCK(wl, flags);
	if (wl->rpcq_dispatched == TRUE) {
		RPCQ_UNLOCK(wl, flags);
		return;
	}

	wl->rpcq_dispatched = TRUE;

	while ((skb = skb_dequeue(&wl->rpc_queue)) != NULL) {
		buf = (rpc_buf_t *) skb;

		RPCQ_UNLOCK(wl, flags);
		WL_LOCK(wl);
#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_RXNOCOPY) || defined(BCM_RPC_ROC)
		/* In bcm_rpc_buf_recv_high(), we made pkts native before
		 * pushing into rpcq.
		 */
		PKTFRMNATIVE(wl->osh, buf);
#endif /* BCM_RPC_NOCOPY || BCM_RPC_RXNOCOPY */
		wlc_rpc_high_dispatch(&wl->rpc_dispatch_ctx, buf);

		WL_UNLOCK(wl);
		RPCQ_LOCK(wl, flags);

		/* bounded our execution, reshedule ourself next */
		if (++count >= NUM_RXQ_ITEMS)
			break;
	}

	wl->rpcq_dispatched = FALSE;
	RPCQ_UNLOCK(wl, flags);
}

static void
wl_rpcq_add_wlthread(wl_info_t *wl, rpc_buf_t *buf)
{
	struct sk_buff *skb;
	/* skb_queue already look the list */
	/* ulong flags; */

#ifdef RPC_RX_MEMORY_PATCH
#if (defined(WLMCHAN) && defined(WLC_HIGH_ONLY))
	bcm_xdr_buf_t b;
	wlc_rpc_id_t rpc_id;
	int err;
#endif
#endif /* RPC_RX_MEMORY_PATCH */

	/* skb_queue already look the list */
	/* RPCQ_LOCK(wl, flags); */

	skb = (struct sk_buff *) buf;

#ifdef RPC_RX_MEMORY_PATCH
#if (defined(WLMCHAN) && defined(WLC_HIGH_ONLY))
	bcm_xdr_buf_init(&b, bcm_rpc_buf_data(wl->rpc_th, buf),
		bcm_rpc_buf_len_get(wl->rpc_th, buf));

	err = bcm_xdr_unpack_uint32(&b, &rpc_id);
	ASSERT(!err);

	if (wl->rpc_queue.qlen >= MCHAN_RPC_RX_SIZE_LIMIT && rpc_id == WLRPC_WLC_RECV_ID) {
		/* drop it */
#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_RXNOCOPY) || defined(BCM_RPC_ROC)
		PKTFRMNATIVE(wl->osh, buf);
		PKTFREE(wl->osh, buf, FALSE);
#else
		bcm_rpc_buf_free(wl->rpc_dispatch_ctx.rpc, buf);
#endif
		return;
	}
#endif /* defined(WLMCHAN) && defined(WLC_HIGH_ONLY) */
#endif /* RPC_RX_MEMORY_PATCH */

	skb->next = NULL;
	skb_queue_tail(&wl->rpc_queue, skb);

	wake_up_interruptible(&wl->thread_wqh);

	/* skb_queue already look the list */
	/* RPCQ_UNLOCK(wl, flags); */
}

#endif /* WL_THREAD */

#ifndef WL_THREAD
static void
wl_rpcq_dispatch(struct wl_task *task)
{
	wl_info_t *wl = (wl_info_t *)task->context;
	rpc_buf_t *buf;
	ulong flags;

	/* First remove an entry then go for execution */
	RPCQ_LOCK(wl, flags);
	while (wl->rpcq_head) {
		buf = wl->rpcq_head;
		wl->rpcq_head = bcm_rpc_buf_next_get(wl->rpc_th, buf);

		if (wl->rpcq_head == NULL)
			wl->rpcq_tail = NULL;
		wl->rpcq_len--;
		RPCQ_UNLOCK(wl, flags);

		WL_LOCK(wl);
#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_RXNOCOPY) || defined(BCM_RPC_ROC)
		/* In bcm_rpc_buf_recv_high(), we made pkts native before
		 * pushing into rpcq.
		 */
		PKTFRMNATIVE(wl->osh, buf);
#endif /* BCM_RPC_NOCOPY || BCM_RPC_RXNOCOPY */
		wlc_rpc_high_dispatch(&wl->rpc_dispatch_ctx, buf);
		WL_UNLOCK(wl);

		RPCQ_LOCK(wl, flags);
	}

	wl->rpcq_dispatched = FALSE;

	RPCQ_UNLOCK(wl, flags);

	MFREE(wl->osh, task, sizeof(wl_task_t));
	atomic_dec(&wl->callbacks);
}

static void
wl_rpcq_add(wl_info_t *wl, rpc_buf_t *buf)
{
	ulong flags;

	bcm_rpc_buf_next_set(wl->rpc_th, buf, NULL);

	/* Lock the queue as tasklet could be running at this time */
	RPCQ_LOCK(wl, flags);
	if (wl->rpcq_head == NULL)
		wl->rpcq_head = buf;
	else
		bcm_rpc_buf_next_set(wl->rpc_th, wl->rpcq_tail, buf);

	wl->rpcq_tail = buf;
	wl->rpcq_len++;

	if (wl->rpcq_dispatched == FALSE) {
		wl->rpcq_dispatched = TRUE;
		RPCQ_UNLOCK(wl, flags);
		wl_schedule_task(wl, wl_rpcq_dispatch, wl);
	} else {
		RPCQ_UNLOCK(wl, flags);
	}

}
#endif /* WL_THREAD */

/* dongle-side rpc dispatch routine */
static void
wl_rpc_dispatch_schedule(void *ctx, struct rpc_buf *buf)
{
	bcm_xdr_buf_t b;
	wl_info_t *wl = (wl_info_t *)ctx;
	wlc_rpc_id_t rpc_id;
	int err;

	BCM_REFERENCE(err);

	bcm_xdr_buf_init(&b, bcm_rpc_buf_data(wl->rpc_th, buf),
		bcm_rpc_buf_len_get(wl->rpc_th, buf));

	err = bcm_xdr_unpack_uint32(&b, &rpc_id);
	ASSERT(!err);
	/* Handle few emergency ones */
	switch (rpc_id) {
#ifdef WL_LIMIT_RPCQ
	case WLRPC_WLC_RECV_ID:
		if (wl_rpcq_rxthresh && (wl->rpcq_len >= wl_rpcq_rxthresh)) {
#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_RXNOCOPY) || defined(BCM_RPC_ROC)
			PKTFRMNATIVE(wl->osh, buf);
			PKTFREE(wl->osh, buf, FALSE);
#else
			PKTFRMNATIVE(wl->osh, buf);
			bcm_rpc_buf_free(wl->rpc_dispatch_ctx.rpc, buf);
#endif /* BCM_RPC_NOCOPY || BCM_RPC_RXNOCOPY */
			break;
		}
		/* fall through */
#endif /* WL_LIMIT_RPCQ */
	default:
#ifdef WL_THREAD
		wl_rpcq_add_wlthread(wl, buf);
#else
		wl_rpcq_add(wl, buf);
#endif
		break;
	}
}

#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_TXNOCOPY) || defined(BCM_RPC_TOC)
static void
wl_rpc_txdone(void *ctx, struct rpc_buf* buf)
{
	bcm_xdr_buf_t b;
	wlc_rpc_id_t rpc_id;
	uint32 rpc_len, tp_len;
	uint32 totlen, pad, fifo, flags, exptime, commit, frameid, txpktpend;
	int err;
	wl_info_t *wl = (wl_info_t *)ctx;
	struct sk_buff *skb = (struct sk_buff*)buf, *prev = NULL;

	BCM_REFERENCE(err);

	/* buf will be a pointer to a pkt chain if packets were aggregated at rpc layer */
	while (skb) {
		buf = (struct rpc_buf *) skb;
		bcm_xdr_buf_init(&b, bcm_rpc_buf_data(wl->rpc_th, buf),
			bcm_rpc_buf_len_get(wl->rpc_th, buf));
		err = bcm_xdr_unpack_uint32(&b, &tp_len); ASSERT(!err);
		err = bcm_xdr_unpack_uint32(&b, &rpc_len); ASSERT(!err);
		err = bcm_xdr_unpack_uint32(&b, &rpc_id); ASSERT(!err);
		bcm_rpc_buf_pull(wl->rpc_th, buf, BCM_RPC_TP_ENCAP_LEN + 4);
		switch (rpc_id) {
		case WLRPC_WLC_BMAC_TXFIFO_ID :
			err = bcm_xdr_unpack_uint32(&b, &fifo); ASSERT(!err);
			err = bcm_xdr_unpack_uint32(&b, (uint32 *)&commit); ASSERT(!err);
			err = bcm_xdr_unpack_uint32(&b, (uint32 *)&frameid); ASSERT(!err);
			err = bcm_xdr_unpack_uint32(&b, (uint32 *)&txpktpend); ASSERT(!err);
			err = bcm_xdr_unpack_uint32(&b, &flags); ASSERT(!err);
			err = bcm_xdr_unpack_uint32(&b, &exptime); ASSERT(!err);
			err = bcm_xdr_unpack_uint32(&b, &totlen); ASSERT(!err);
			bcm_rpc_buf_pull(wl->rpc_th, buf, WLC_RPCTX_PARAMS);
			tp_len -= (WLC_RPCTX_PARAMS + 4);

			if (flags & WLC_BMAC_F_PAD2) {
				bcm_rpc_buf_pull(wl->rpc_th, buf, WLC_RPC_TXFIFO_UNALIGN_PAD_2BYTE);
				tp_len -= WLC_RPC_TXFIFO_UNALIGN_PAD_2BYTE;
				totlen -= WLC_RPC_TXFIFO_UNALIGN_PAD_2BYTE;

			}
			/* check whether the packet was padded in wlc_rpctx_tx() */
			pad = tp_len - totlen;
			/* TOC, navigate to the last fragment in current packet */
			while (skb && tp_len > 0) {
				tp_len -= PKTLEN(wl->osh, skb);
				prev = skb;
				skb = skb->next;
			}
			prev->next = NULL;
			/* restore the original length if the packet was padded wlc_rpctx_tx() */
			totlen = PKTLEN(wl->osh, prev) - pad;
			PKTSETLEN(wl->osh, prev, totlen);
			break;
		default:
			skb = skb->next;
			((struct sk_buff*)buf)->next = NULL;
			bcm_rpc_tp_buf_free(wl->rpc_th, buf);
			break;
		}
	}
}
#endif /* defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_TXNOCOPY) || defined(BCM_RPC_TOC) */

#endif /* WLC_HIGH_ONLY */

#ifdef WL_ALL_PASSIVE
static void
wl_set_multicast_list_workitem(struct work_struct *work)
{
	wl_task_t *task = (wl_task_t *)work;
	struct net_device *dev = (struct net_device*)task->context;
	wl_info_t *wl;

	wl = WL_INFO(dev);

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
}

wl_timer_t *
wl_init_timer(wl_info_t *wl, void (*fn)(void *arg), void *arg, const char *tname)
{
	wl_timer_t *t;

	t = (wl_timer_t*)MALLOC(wl->osh, sizeof(wl_timer_t));

	if (t == NULL) {
		WL_ERROR(("wl%d: wl_init_timer: out of memory, malloced %d bytes\n",
			wl->unit, MALLOCED(wl->osh)));
		return 0;
	}

	bzero(t, sizeof(wl_timer_t));

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
}

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
wl_monitor(wl_info_t *wl, wl_rxsts_t *rxsts, void *p)
{
#ifdef WL_MONITOR
	struct sk_buff *oskb = (struct sk_buff *)p;
	struct sk_buff *skb;
	uchar *pdata;
	uint len;

	len = 0;
	skb = NULL;
	WL_TRACE(("wl%d: wl_monitor\n", wl->pub->unit));

	if (!wl->monitor_dev)
		return;

	if (wl->monitor_type == 1) {
		p80211msg_t *phdr;

		len = sizeof(p80211msg_t) + oskb->len - D11_PHY_HDR_LEN;
		if ((skb = dev_alloc_skb(len)) == NULL)
			return;

		skb_put(skb, len);
		phdr = (p80211msg_t*)skb->data;
		/* Initialize the message members */
		phdr->msgcode = WL_MON_FRAME;
		phdr->msglen = sizeof(p80211msg_t);
		strcpy(phdr->devname, wl->dev->name);

		phdr->hosttime.did = WL_MON_FRAME_HOSTTIME;
		phdr->hosttime.status = P80211ITEM_OK;
		phdr->hosttime.len = 4;
		phdr->hosttime.data = jiffies;

		phdr->channel.did = WL_MON_FRAME_CHANNEL;
		phdr->channel.status = P80211ITEM_NO_VALUE;
		phdr->channel.len = 4;
		phdr->channel.data = 0;

		phdr->signal.did = WL_MON_FRAME_SIGNAL;
		phdr->signal.status = P80211ITEM_OK;
		phdr->signal.len = 4;
		/* two sets of preamble values are defined in wlc_ethereal and wlc_pub.h
		 * and this assumet their values are matched. Otherwise,
		 * we have to go through conversion, which requires rspec since datarate
		 * is just kbps now
		 */
		phdr->signal.data = rxsts->preamble;

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

		pdata = skb->data + sizeof(p80211msg_t);
		bcopy(oskb->data + D11_PHY_HDR_LEN, pdata, oskb->len - D11_PHY_HDR_LEN);

	}
	else if (wl->monitor_type == 2) {
		int channel_frequency;
		uint16 channel_flags;
		uint8 flags;
		uint16 rtap_len;
		struct dot11_header *mac_header;
		uint16 fc;

		if (rxsts->phytype != WL_RXS_PHY_N)
			rtap_len = sizeof(struct wl_radiotap_legacy);
		else
			rtap_len = sizeof(struct wl_radiotap_ht_brcm);

		len = rtap_len + (oskb->len - D11_PHY_HDR_LEN);
		if ((skb = dev_alloc_skb(len)) == NULL)
			return;

		skb_put(skb, len);

		if (CHSPEC_IS2G(rxsts->chanspec)) {
			channel_flags = IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_DYN;
			channel_frequency = wf_channel2mhz(wf_chspec_ctlchan(rxsts->chanspec),
			                                   WF_CHAN_FACTOR_2_4_G);
		} else {
			channel_flags = IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM;
			channel_frequency = wf_channel2mhz(wf_chspec_ctlchan(rxsts->chanspec),
			                                   WF_CHAN_FACTOR_5_G);
		}

		mac_header = (struct dot11_header *)(oskb->data + D11_PHY_HDR_LEN);
		fc = ntoh16(mac_header->fc);

		flags = IEEE80211_RADIOTAP_F_FCS;

		if (rxsts->preamble == WL_RXS_PREAMBLE_SHORT)
			flags |= IEEE80211_RADIOTAP_F_SHORTPRE;

		if (fc & (FC_WEP >> FC_WEP_SHIFT))
			flags |= IEEE80211_RADIOTAP_F_WEP;

		if (fc & (FC_MOREFRAG >> FC_MOREFRAG_SHIFT))
			flags |= IEEE80211_RADIOTAP_F_FRAG;

		if (rxsts->pkterror & WL_RXS_CRC_ERROR)
			flags |= IEEE80211_RADIOTAP_F_BADFCS;

		if (rxsts->phytype != WL_RXS_PHY_N) {
			struct wl_radiotap_legacy *rtl = (struct wl_radiotap_legacy*)skb->data;

			rtl->ieee_radiotap.it_version = 0;
			rtl->ieee_radiotap.it_pad = 0;
			rtl->ieee_radiotap.it_len = HTOL16(rtap_len);
			rtl->ieee_radiotap.it_present = HTOL32(WL_RADIOTAP_PRESENT_LEGACY);

			rtl->tsft_l = htol32(rxsts->mactime);
			rtl->tsft_h = 0;
			rtl->flags = flags;
			rtl->rate = rxsts->datarate;
			rtl->channel_freq = HTOL16(channel_frequency);
			rtl->channel_flags = HTOL16(channel_flags);
			rtl->signal = (int8)rxsts->signal;
			rtl->noise = (int8)rxsts->noise;
			rtl->antenna = rxsts->antenna;

			/* Broadcom specific */
			memcpy(rtl->vend_oui, brcm_oui, sizeof(brcm_oui));
			rtl->vend_skip_len = WL_RADIOTAP_LEGACY_SKIP_LEN;
			rtl->vend_sns = 0;

			/* VHT b/w signalling */
			memset(&rtl->nonht_vht, 0, sizeof(rtl->nonht_vht));
			rtl->nonht_vht.len = WL_RADIOTAP_NONHT_VHT_LEN;

		} else {
			struct wl_radiotap_ht_brcm *rtht = (struct wl_radiotap_ht_brcm *)skb->data;

			rtht->ieee_radiotap.it_version = 0;
			rtht->ieee_radiotap.it_pad = 0;
			rtht->ieee_radiotap.it_len = HTOL16(rtap_len);
			rtht->ieee_radiotap.it_present = HTOL32(WL_RADIOTAP_PRESENT_HT_BRCM);
			rtht->it_present_ext = HTOL32(WL_RADIOTAP_BRCM_MCS);
			rtht->pad1 = 0;

			rtht->tsft_l = htol32(rxsts->mactime);
			rtht->tsft_h = 0;
			rtht->flags = flags;
			rtht->pad2 = 0;
			rtht->channel_freq = HTOL16(channel_frequency);
			rtht->channel_flags = HTOL16(channel_flags);
			rtht->signal = (int8)rxsts->signal;
			rtht->noise = (int8)rxsts->noise;
			rtht->antenna = rxsts->antenna;
			rtht->pad3 = 0;

			memcpy(rtht->vend_oui, "\x00\x10\x18", 3);
			rtht->vend_sns = WL_RADIOTAP_BRCM_SNS;
			rtht->vend_skip_len = 2;
			rtht->mcs = rxsts->mcs;
			rtht->htflags = 0;
			if (rxsts->htflags & WL_RXS_HTF_40)
				rtht->htflags |= IEEE80211_RADIOTAP_HTMOD_40;
			if (rxsts->htflags & WL_RXS_HTF_SGI)
				rtht->htflags |= IEEE80211_RADIOTAP_HTMOD_SGI;
			if (rxsts->preamble & WL_RXS_PREAMBLE_HT_GF)
				rtht->htflags |= IEEE80211_RADIOTAP_HTMOD_GF;
			if (rxsts->htflags & WL_RXS_HTF_LDPC)
				rtht->htflags |= IEEE80211_RADIOTAP_HTMOD_LDPC;
			rtht->htflags |=
				(rxsts->htflags & WL_RXS_HTF_STBC_MASK) <<
				IEEE80211_RADIOTAP_HTMOD_STBC_SHIFT;
		}

		pdata = skb->data + rtap_len;
		bcopy(oskb->data + D11_PHY_HDR_LEN, pdata, oskb->len - D11_PHY_HDR_LEN);
	}
	else if (wl->monitor_type == 3) {
		int channel_frequency;
		uint16 channel_flags;
		uint8 flags;
		uint16 rtap_len;
		struct dot11_header * mac_header;
		uint16 fc;

		if (rxsts->phytype == WL_RXS_PHY_N) {
			if (rxsts->encoding == WL_RXS_ENCODING_HT)
				rtap_len = sizeof(wl_radiotap_ht_t);
#ifdef WL11AC
			else if (rxsts->encoding == WL_RXS_ENCODING_VHT)
				rtap_len = sizeof(wl_radiotap_vht_t);
#endif /* WL11AC */
			else
				rtap_len = sizeof(wl_radiotap_legacy_t);
		} else {
			rtap_len = sizeof(wl_radiotap_legacy_t);
		}

		len = rtap_len + (oskb->len - D11_PHY_HDR_LEN);

		if (oskb->next) {
			struct sk_buff *amsdu_p = oskb->next;
			uint amsdu_len = 0;
			while (amsdu_p) {
				amsdu_len += amsdu_p->len;
				amsdu_p = amsdu_p->next;
			}
			len += amsdu_len;
		}

		if ((skb = dev_alloc_skb(len)) == NULL)
			return;

		skb_put(skb, len);

		if (CHSPEC_IS2G(rxsts->chanspec)) {
			channel_flags = IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_DYN;
			channel_frequency = wf_channel2mhz(wf_chspec_ctlchan(rxsts->chanspec),
			                                   WF_CHAN_FACTOR_2_4_G);
		} else {
			channel_flags = IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM;
			channel_frequency = wf_channel2mhz(wf_chspec_ctlchan(rxsts->chanspec),
			                                   WF_CHAN_FACTOR_5_G);
		}

		mac_header = (struct dot11_header *)(oskb->data + D11_PHY_HDR_LEN);
		fc = ltoh16(mac_header->fc);

		flags = IEEE80211_RADIOTAP_F_FCS;

		if (rxsts->preamble == WL_RXS_PREAMBLE_SHORT)
			flags |= IEEE80211_RADIOTAP_F_SHORTPRE;

		if (fc & FC_WEP)
			flags |= IEEE80211_RADIOTAP_F_WEP;

		if (fc & FC_MOREFRAG)
			flags |= IEEE80211_RADIOTAP_F_FRAG;

		if (rxsts->pkterror & WL_RXS_CRC_ERROR)
			flags |= IEEE80211_RADIOTAP_F_BADFCS;

#ifdef WL11AC
		if ((rxsts->phytype != WL_RXS_PHY_N) ||
			((rxsts->encoding != WL_RXS_ENCODING_HT) &&
			(rxsts->encoding != WL_RXS_ENCODING_VHT))) {
#else
		if (rxsts->phytype != WL_RXS_PHY_N || rxsts->encoding != WL_RXS_ENCODING_HT) {
#endif /* WL11AC */
			wl_radiotap_legacy_t *rtl = (wl_radiotap_legacy_t *)skb->data;

			rtl->ieee_radiotap.it_version = 0;
			rtl->ieee_radiotap.it_pad = 0;
			rtl->ieee_radiotap.it_len = HTOL16(rtap_len);
			rtl->ieee_radiotap.it_present = HTOL32(WL_RADIOTAP_PRESENT_LEGACY);

			rtl->it_present_ext = HTOL32(WL_RADIOTAP_LEGACY_VHT);
			rtl->tsft_l = htol32(rxsts->mactime);
			rtl->tsft_h = 0;
			rtl->flags = flags;
			rtl->rate = rxsts->datarate;
			rtl->channel_freq = HTOL16(channel_frequency);
			rtl->channel_flags = HTOL16(channel_flags);
			rtl->signal = (int8)rxsts->signal;
			rtl->noise = (int8)rxsts->noise;
			rtl->antenna = rxsts->antenna;

			/* Broadcom specific */
			memcpy(rtl->vend_oui, brcm_oui, sizeof(brcm_oui));
			rtl->vend_skip_len = WL_RADIOTAP_LEGACY_SKIP_LEN;
			rtl->vend_sns = 0;

			/* VHT b/w signalling */
			memset(&rtl->nonht_vht, 0, sizeof(rtl->nonht_vht));
			rtl->nonht_vht.len = WL_RADIOTAP_NONHT_VHT_LEN;
#ifdef WL11AC
			if (((fc & FC_KIND_MASK) == FC_RTS) ||
				((fc & FC_KIND_MASK) == FC_CTS)) {
				rtl->nonht_vht.flags |= WL_RADIOTAP_F_NONHT_VHT_BW;
				rtl->nonht_vht.bw = rxsts->bw_nonht;
				rtl->vend_sns = WL_RADIOTAP_LEGACY_SNS;

			}
			if ((fc & FC_KIND_MASK) == FC_RTS) {
				if (rxsts->vhtflags & WL_RXS_VHTF_DYN_BW_NONHT)
					rtl->nonht_vht.flags
						|= WL_RADIOTAP_F_NONHT_VHT_DYN_BW;
			}
#endif /* WL11AC */
		}
#ifdef WL11AC
		else if (rxsts->encoding == WL_RXS_ENCODING_VHT) {
			wl_radiotap_vht_t *rtvht = (wl_radiotap_vht_t *)skb->data;

			rtvht->ieee_radiotap.it_version = 0;
			rtvht->ieee_radiotap.it_pad = 0;
			rtvht->ieee_radiotap.it_len = HTOL16(rtap_len);
			rtvht->ieee_radiotap.it_present =
				HTOL32(WL_RADIOTAP_PRESENT_VHT);

			rtvht->tsft_l = htol32(rxsts->mactime);
			rtvht->tsft_h = 0;
			rtvht->flags = flags;
			rtvht->pad1 = 0;
			rtvht->channel_freq = HTOL16(channel_frequency);
			rtvht->channel_flags = HTOL16(channel_flags);
			rtvht->signal = (int8)rxsts->signal;
			rtvht->noise = (int8)rxsts->noise;
			rtvht->antenna = rxsts->antenna;

			rtvht->vht_known = (IEEE80211_RADIOTAP_VHT_HAVE_STBC |
				IEEE80211_RADIOTAP_VHT_HAVE_TXOP_PS |
				IEEE80211_RADIOTAP_VHT_HAVE_GI |
				IEEE80211_RADIOTAP_VHT_HAVE_SGI_NSYM_DA |
				IEEE80211_RADIOTAP_VHT_HAVE_LDPC_EXTRA |
				IEEE80211_RADIOTAP_VHT_HAVE_BF |
				IEEE80211_RADIOTAP_VHT_HAVE_BW |
				IEEE80211_RADIOTAP_VHT_HAVE_GID |
				IEEE80211_RADIOTAP_VHT_HAVE_PAID);

			STATIC_ASSERT(WL_RXS_VHTF_STBC ==
				IEEE80211_RADIOTAP_VHT_STBC);
			STATIC_ASSERT(WL_RXS_VHTF_TXOP_PS ==
				IEEE80211_RADIOTAP_VHT_TXOP_PS);
			STATIC_ASSERT(WL_RXS_VHTF_SGI ==
				IEEE80211_RADIOTAP_VHT_SGI);
			STATIC_ASSERT(WL_RXS_VHTF_SGI_NSYM_DA ==
				IEEE80211_RADIOTAP_VHT_SGI_NSYM_DA);
			STATIC_ASSERT(WL_RXS_VHTF_LDPC_EXTRA ==
				IEEE80211_RADIOTAP_VHT_LDPC_EXTRA);
			STATIC_ASSERT(WL_RXS_VHTF_BF ==
				IEEE80211_RADIOTAP_VHT_BF);

			rtvht->vht_flags = HTOL16(rxsts->vhtflags);

			STATIC_ASSERT(WL_RXS_VHT_BW_20 ==
				IEEE80211_RADIOTAP_VHT_BW_20);
			STATIC_ASSERT(WL_RXS_VHT_BW_40 ==
				IEEE80211_RADIOTAP_VHT_BW_40);
			STATIC_ASSERT(WL_RXS_VHT_BW_20L ==
				IEEE80211_RADIOTAP_VHT_BW_20L);
			STATIC_ASSERT(WL_RXS_VHT_BW_20U ==
				IEEE80211_RADIOTAP_VHT_BW_20U);
			STATIC_ASSERT(WL_RXS_VHT_BW_80 ==
				IEEE80211_RADIOTAP_VHT_BW_80);
			STATIC_ASSERT(WL_RXS_VHT_BW_40L ==
				IEEE80211_RADIOTAP_VHT_BW_40L);
			STATIC_ASSERT(WL_RXS_VHT_BW_40U ==
				IEEE80211_RADIOTAP_VHT_BW_40U);
			STATIC_ASSERT(WL_RXS_VHT_BW_20LL ==
				IEEE80211_RADIOTAP_VHT_BW_20LL);
			STATIC_ASSERT(WL_RXS_VHT_BW_20LU ==
				IEEE80211_RADIOTAP_VHT_BW_20LU);
			STATIC_ASSERT(WL_RXS_VHT_BW_20UL ==
				IEEE80211_RADIOTAP_VHT_BW_20UL);
			STATIC_ASSERT(WL_RXS_VHT_BW_20UU ==
				IEEE80211_RADIOTAP_VHT_BW_20UU);

			rtvht->vht_bw = rxsts->bw;

			rtvht->vht_mcs_nss[0] = (rxsts->mcs << 4) |
				(rxsts->nss & IEEE80211_RADIOTAP_VHT_NSS);
			rtvht->vht_mcs_nss[1] = 0;
			rtvht->vht_mcs_nss[2] = 0;
			rtvht->vht_mcs_nss[3] = 0;

			STATIC_ASSERT(WL_RXS_VHTF_CODING_LDCP ==
				IEEE80211_RADIOTAP_VHT_CODING_LDPC);

			rtvht->vht_coding = rxsts->coding;
			rtvht->vht_group_id = rxsts->gid;
			rtvht->vht_partial_aid = HTOL16(rxsts->aid);

			rtvht->ampdu_flags = 0;
			rtvht->ampdu_delim_crc = 0;

			rtvht->ampdu_ref_num = rxsts->ampdu_counter;

			if (!(rxsts->nfrmtype & WL_RXS_NFRM_AMPDU_FIRST) &&
				!(rxsts->nfrmtype & WL_RXS_NFRM_AMPDU_SUB))
				rtvht->ampdu_flags |= IEEE80211_RADIOTAP_AMPDU_IS_LAST;
		}
#endif /* WL11AC */
		else if (rxsts->encoding == WL_RXS_ENCODING_HT) {
			wl_radiotap_ht_t *rtht =
				(wl_radiotap_ht_t *)skb->data;

			rtht->ieee_radiotap.it_version = 0;
			rtht->ieee_radiotap.it_pad = 0;
			rtht->ieee_radiotap.it_len = HTOL16(rtap_len);
			rtht->ieee_radiotap.it_present
				= HTOL32(WL_RADIOTAP_PRESENT_HT);
			rtht->pad1 = 0;

			rtht->tsft_l = htol32(rxsts->mactime);
			rtht->tsft_h = 0;
			rtht->flags = flags;
			rtht->channel_freq = HTOL16(channel_frequency);
			rtht->channel_flags = HTOL16(channel_flags);
			rtht->signal = (int8)rxsts->signal;
			rtht->noise = (int8)rxsts->noise;
			rtht->antenna = rxsts->antenna;

			/* add standard MCS */
			rtht->mcs_known = (IEEE80211_RADIOTAP_MCS_HAVE_BW |
				IEEE80211_RADIOTAP_MCS_HAVE_MCS |
				IEEE80211_RADIOTAP_MCS_HAVE_GI |
				IEEE80211_RADIOTAP_MCS_HAVE_FEC |
				IEEE80211_RADIOTAP_MCS_HAVE_FMT);

			rtht->mcs_flags = 0;
			switch (rxsts->htflags & WL_RXS_HTF_BW_MASK) {
				case WL_RXS_HTF_20L:
					rtht->mcs_flags |= IEEE80211_RADIOTAP_MCS_BW_20L;
					break;
				case WL_RXS_HTF_20U:
					rtht->mcs_flags |= IEEE80211_RADIOTAP_MCS_BW_20U;
					break;
				case WL_RXS_HTF_40:
					rtht->mcs_flags |= IEEE80211_RADIOTAP_MCS_BW_40;
					break;
				default:
					rtht->mcs_flags |= IEEE80211_RADIOTAP_MCS_BW_20;
			}

			if (rxsts->htflags & WL_RXS_HTF_SGI) {
				rtht->mcs_flags |= IEEE80211_RADIOTAP_MCS_SGI;
			}
			if (rxsts->preamble & WL_RXS_PREAMBLE_HT_GF) {
				rtht->mcs_flags |= IEEE80211_RADIOTAP_MCS_FMT_GF;
			}
			if (rxsts->htflags & WL_RXS_HTF_LDPC) {
				rtht->mcs_flags |= IEEE80211_RADIOTAP_MCS_FEC_LDPC;
			}
			rtht->mcs_index = rxsts->mcs;
		}


		pdata = skb->data + rtap_len;
		bcopy(oskb->data + D11_PHY_HDR_LEN, pdata, oskb->len - D11_PHY_HDR_LEN);
	}

	if (skb == NULL) return;

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

/* Create a virtual interface. Call only from safe time!
 * can't call register_netdev with WL_LOCK
 */
/* Equivelent to _wl_add_if */
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
	strncpy(wlif->dev->name, wlif->name, strlen(wlif->name));
	wlif->dev->name[strlen(wlif->name)] = '\0';

	/* Monitor specific tweaks */
	if (wl->monitor_type == 1)
		dev->type = ARPHRD_IEEE80211_PRISM;
	else
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
}

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

/*
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
/* Equivelent to wl_add_if */
void
wl_set_monitor(wl_info_t *wl, int val)
{
#ifdef WL_MONITOR
	const char *devname;
	wl_if_t *wlif;

	WL_TRACE(("wl%d: wl_set_monitor: val %d\n", wl->pub->unit, val));
	if ((val && wl->monitor_dev) || (!val && !wl->monitor_dev)) {
		WL_ERROR(("%s: Mismatched params, return\n", __FUNCTION__));
		return;
	}

	/* Delete monitor */
	if (!val) {
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
	if (wl->monitor_type == 1)
		devname = "prism";
	else
		devname = "radiotap";
	sprintf(wlif->name, "%s%d", devname, wl->pub->unit);

	if (wl_schedule_task(wl, _wl_add_monitor_if, wlif)) {
		MFREE(wl->osh, wlif, sizeof(wl_if_t));
		return;
	}
#endif /* WL_MONITOR */
}

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
	strcpy(dev->node.dev_name, wl->dev->name);
	link->dev = &dev->node; /* link->dev also indicates net_device registered */
	link->state &= ~DEV_CONFIG_PENDING;


	return;

cs_failed:
	WL_NONE(("wl %s: at cs_failed, last_cmd 0x%x, last_ret 0x%x\n",
		__FUNCTION__, last_cmd, last_ret));
	cs_error(link->handle, last_cmd, last_ret);

failed:
	wl_cs_release((u_long) link);
}				/* wl_cs_config */

/*
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
}				/* wl_cs_release */

/*
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




struct net_device *
wl_netdev_get(wl_info_t *wl)
{
	return wl->dev;
}

#ifdef BCM_WL_EMULATOR

/* create an empty wl_info structure to be used by the emulator */
wl_info_t *
wl_wlcreate(osl_t *osh, void *pdev)
{
	wl_info_t *wl;

	/* allocate private info */
	if ((wl = (wl_info_t*) MALLOC(osh, sizeof(wl_info_t))) == NULL) {
		WL_ERROR(("wl%d: malloc wl_info_t, out of memory, malloced %d bytes\n", 0,
			MALLOCED(osh)));
		osl_detach(osh);
		return NULL;
	}
	bzero(wl, sizeof(wl_info_t));
	wl->dev = pdev;
	wl->osh = osh;
	return wl;
}

void * wl_getdev(void *w)
{
	wl_info_t *wl = (wl_info_t *)w;
	return wl->dev;
}
#endif /* BCM_WL_EMULATOR */

/* Linux: no chaining */
int
wl_set_pktlen(osl_t *osh, void *p, int len)
{
	PKTSETLEN(osh, p, len);
	return len;
}
/* Linux: no chaining */
void *
wl_get_pktbuffer(osl_t *osh, int len)
{
	return (PKTGET(osh, len, FALSE));
}

/* Linux version: no chains */
uint
wl_buf_to_pktcopy(osl_t *osh, void *p, uchar *buf, int len, uint offset)
{
	if (PKTLEN(osh, p) < len + offset)
		return 0;
	bcopy(buf, (char *)PKTDATA(osh, p) + offset, len);
	return len;
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
	}
	else
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
	}
	else
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
	}
	else
		WL_ERROR(("%s: No tkip mod ops\n", __FUNCTION__));

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14) */

	/* Error */
	return err;
}


int
wl_tkip_keyset(wl_info_t *wl, wsec_key_t *key)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	bool group_key = FALSE;
	uchar rxseq[IW_ENCODE_SEQ_MAX_SIZE];

	if (key->len != 0) {
		WL_WSEC(("%s: Key Length is Not zero\n", __FUNCTION__));
		if (key->algo != CRYPTO_ALGO_TKIP) {
			WL_WSEC(("%s: Algo is Not TKIP %d\n", __FUNCTION__, key->algo));
			return 0;
		}
		WL_WSEC(("%s: Trying to set a key in TKIP Mod\n", __FUNCTION__));
	}
	else
		WL_WSEC(("%s: Trying to Remove a Key from TKIP Mod\n", __FUNCTION__));

	if (ETHER_ISNULLADDR(&key->ea) || ETHER_ISBCAST(&key->ea)) {
		group_key = TRUE;
		WL_WSEC(("Group Key index %d\n", key->id));
	}
	else
		WL_WSEC(("Unicast Key index %d\n", key->id));

	if (wl->tkipmodops) {
		uint8 keybuf[8];
		if (group_key) {
			if (key->len) {
				if (!wl->tkip_bcast_data[key->id]) {
					WL_WSEC(("Init TKIP Bcast Module\n"));
					WL_UNLOCK(wl);
					wl->tkip_bcast_data[key->id] =
						wl->tkipmodops->init(key->id);
					WL_LOCK(wl);
				}
				if (wl->tkip_bcast_data[key->id]) {
					WL_WSEC(("TKIP SET BROADCAST KEY******************\n"));
					bzero(rxseq, IW_ENCODE_SEQ_MAX_SIZE);
					bcopy(&key->rxiv, rxseq, 6);
					bcopy(&key->data[24], keybuf, sizeof(keybuf));
					bcopy(&key->data[16], &key->data[24], sizeof(keybuf));
					bcopy(keybuf, &key->data[16], sizeof(keybuf));
					wl->tkipmodops->set_key(&key->data, key->len,
						(uint8 *)&key->rxiv, wl->tkip_bcast_data[key->id]);
				}
			}
			else {
				if (wl->tkip_bcast_data[key->id]) {
					WL_WSEC(("Deinit TKIP Bcast Module\n"));
					wl->tkipmodops->deinit(wl->tkip_bcast_data[key->id]);
					wl->tkip_bcast_data[key->id] = NULL;
				}
			}
		}
		else {
			if (key->len) {
				if (!wl->tkip_ucast_data) {
					WL_WSEC(("Init TKIP Ucast Module\n"));
					WL_UNLOCK(wl);
					wl->tkip_ucast_data = wl->tkipmodops->init(key->id);
					WL_LOCK(wl);
				}
				if (wl->tkip_ucast_data) {
					WL_WSEC(("TKIP SET UNICAST KEY******************\n"));
					bzero(rxseq, IW_ENCODE_SEQ_MAX_SIZE);
					bcopy(&key->rxiv, rxseq, 6);
					bcopy(&key->data[24], keybuf, sizeof(keybuf));
					bcopy(&key->data[16], &key->data[24], sizeof(keybuf));
					bcopy(keybuf, &key->data[16], sizeof(keybuf));
					wl->tkipmodops->set_key(&key->data, key->len,
						(uint8 *)&key->rxiv, wl->tkip_ucast_data);
				}
			}
			else {
				if (wl->tkip_ucast_data) {
					WL_WSEC(("Deinit TKIP Ucast Module\n"));
					wl->tkipmodops->deinit(wl->tkip_ucast_data);
					wl->tkip_ucast_data = NULL;
				}
			}
		}
	}
	else
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14) */
		WL_WSEC(("%s: No tkip mod ops\n", __FUNCTION__));
	return 0;
}

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

	snprintf(wl->wl_rfkill.rfkill_name, sizeof(wl->wl_rfkill.rfkill_name),
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

static int
wl_linux_watchdog(void *ctx)
{
	wl_info_t *wl = (wl_info_t *) ctx;
	struct net_device_stats *stats = NULL;
	uint id;
	wl_if_t *wlif;
	wlc_if_stats_t wlcif_stats;
#ifdef USE_IW
	struct iw_statistics *wstats = NULL;
	int phy_noise;
#endif
	if (wl == NULL)
		return -1;

	if (wl->if_list) {
		for (wlif = wl->if_list; wlif != NULL; wlif = wlif->next) {
			memset(&wlcif_stats, 0, sizeof(wlc_if_stats_t));
			wlc_wlcif_stats_get(wl->wlc, wlif->wlcif, &wlcif_stats);

			/* refresh stats */
			if (wl->pub->up) {
				ASSERT(wlif->stats_id < 2);

				id = 1 - wlif->stats_id;
				stats = &wlif->stats_watchdog[id];
				if (stats) {
					stats->rx_packets = WLCNTVAL(wlcif_stats.rxframe);
					stats->tx_packets = WLCNTVAL(wlcif_stats.txframe);
					stats->rx_bytes = WLCNTVAL(wlcif_stats.rxbyte);
					stats->tx_bytes = WLCNTVAL(wlcif_stats.txbyte);
					stats->rx_errors = WLCNTVAL(wlcif_stats.rxerror);
					stats->tx_errors = WLCNTVAL(wlcif_stats.txerror);
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
					wstats->discard.fragment = WLCNTVAL(wlcif_stats.rxfragerr);
					wstats->discard.retries = WLCNTVAL(wlcif_stats.txfail);
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

	return 0;
}

#if defined(LINUX_HYBRID)

/* OS Entry point when app attempts to read */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
static int
wl_proc_read(char *buffer, char **start, off_t offset, int length, int *eof, void *data)
#else
static ssize_t
wl_proc_read(struct file *filp, char __user *buffer, size_t length, loff_t *data)
#endif
{
	wl_info_t * wl = (wl_info_t *)data;
	int bcmerror, to_user;
	int len;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
	if (offset > 0) {
		*eof = 1;
		return 0;
	}
#endif
	/* Give the processed buffer back to userland */
	if (!length) {
		WL_ERROR(("%s: Not enough return buf space\n", __FUNCTION__));
		return 0;
	}
	WL_LOCK(wl);
	bcmerror = wlc_ioctl(wl->wlc, WLC_GET_MONITOR, &to_user, sizeof(int), NULL);
	len = sprintf(buffer, "%d\n", to_user);
	WL_UNLOCK(wl);
	return len;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
static int
wl_proc_write(struct file *filp, const char *buff, unsigned long length, void *data)
#else
static ssize_t
wl_proc_write(struct file *filp, const char __user *buff, size_t length, loff_t *data)
#endif
{
	wl_info_t * wl = (wl_info_t *)data;
	int from_user = 0;
	int bcmerror;

	if (length == 0 || length > 2) {
		/* Expecting one ascii digit, followed by newline */
		WL_ERROR(("%s: Invalid data length\n", __FUNCTION__));
		return -EIO;
	}
	if (copy_from_user(&from_user, buff, 1)) {
		WL_ERROR(("%s: copy from user failed\n", __FUNCTION__));
		return -EIO;
	}
	/* Convert ascii digit to integer */
	if (from_user >= 0x30)
		from_user -= 0x30;

	WL_LOCK(wl);
	bcmerror = wlc_ioctl(wl->wlc, WLC_SET_MONITOR, &from_user, sizeof(int), NULL);
	WL_UNLOCK(wl);

	if (bcmerror < 0) {
		WL_ERROR(("%s: SET_MONITOR failed with %d\n", __FUNCTION__, bcmerror));
		return -EIO;
	}
	return length;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
static const struct file_operations wl_fops = {
	.owner	= THIS_MODULE,
	.read	= wl_proc_read,
	.write	= wl_proc_write,
};
#endif

static int
wl_reg_proc_entry(wl_info_t *wl)
{
	char tmp[32];
	sprintf(tmp, "%s%d", HYBRID_PROC, wl->pub->unit);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
	if ((wl->proc_entry = create_proc_entry(tmp, 0644, NULL)) == NULL) {
		WL_ERROR(("%s: create_proc_entry %s failed\n", __FUNCTION__, tmp));
#else
	if ((wl->proc_entry = proc_create(tmp, 0644, NULL, &wl_fops)) == NULL) {
		WL_ERROR(("%s: proc_create %s failed\n", __FUNCTION__, tmp));
#endif
		ASSERT(0);
		return -1;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
	wl->proc_entry->read_proc = wl_proc_read;
	wl->proc_entry->write_proc = wl_proc_write;
	wl->proc_entry->data = wl;
#endif
	return 0;
}
#endif /* LINUX_HYBRID */
#ifdef WLOFFLD
uint32 wl_pcie_bar1(struct wl_info *wl, uchar** addr)
{
	*addr = wl->bar1_addr;
	return (wl->bar1_size);
}
#endif
