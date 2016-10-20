/*
 * Linux cfg80211 driver - Android related functions
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
 * $Id: wl_android.h 655991 2016-08-24 18:34:07Z $
 */

#include <linux/module.h>
#include <linux/netdevice.h>
#include <wldev_common.h>

typedef struct _android_wifi_priv_cmd {
    char *buf;
    int used_len;
    int total_len;
} android_wifi_priv_cmd;

#ifdef CONFIG_COMPAT
typedef struct _compat_android_wifi_priv_cmd {
    compat_caddr_t buf;
    int used_len;
    int total_len;
} compat_android_wifi_priv_cmd;
#endif /* CONFIG_COMPAT */

/**
 * Android platform dependent functions, feel free to add Android specific functions here
 * (save the macros in wl). Please do NOT declare functions that are NOT exposed to wl
 * or cfg, define them as static in wl_android.c
 */

int wl_android_priv_cmd(struct net_device *net, struct ifreq *ifr, int cmd);
int wl_handle_private_cmd(struct net_device *net, char *command, u32 cmd_len);
int wl_android_get_80211_mode(struct net_device *dev, char *command, int total_len);
int wl_android_get_chanspec(struct net_device *dev, char *command, int total_len);
int wl_android_get_datarate(struct net_device *dev, char *command, int total_len);
int wl_android_get_assoclist(struct net_device *dev, char *command, int total_len);
int wl_android_set_roam_mode(struct net_device *dev, char *command, int total_len);
int wl_android_set_ibss_beacon_ouidata(struct net_device *dev, char *command, int total_len);
int wl_keep_alive_set(struct net_device *dev, char* extra, int total_len);
int wl_android_set_roam_offload_bssid_list(struct net_device *dev, const char *cmd);

s32 wl_netlink_send_msg(int pid, int type, int seq, void *data, size_t size);

/* hostap mac mode */
#define MACLIST_MODE_DISABLED   0
#define MACLIST_MODE_DENY       1
#define MACLIST_MODE_ALLOW      2

/* max number of assoc list */
#define MAX_NUM_OF_ASSOCLIST    64

#define WL_SCAN_ASSOC_ACTIVE_TIME  40 /* ms: Embedded default Active setting from WL driver */
#define WL_SCAN_UNASSOC_ACTIVE_TIME 80 /* ms: Embedded def. Unassoc Active setting from WL driver */
#define WL_SCAN_PASSIVE_TIME       130 /* ms: Embedded default Passive setting from WL driver */

/* Bandwidth */
#define WL_CH_BANDWIDTH_20MHZ 20
#define WL_CH_BANDWIDTH_40MHZ 40
#define WL_CH_BANDWIDTH_80MHZ 80
/* max number of mac filter list
 * restrict max number to 10 as maximum cmd string size is 255
 */
#define MAX_NUM_MAC_FILT        10

int wl_android_set_ap_mac_list(struct net_device *dev, int macmode, struct maclist *maclist);
