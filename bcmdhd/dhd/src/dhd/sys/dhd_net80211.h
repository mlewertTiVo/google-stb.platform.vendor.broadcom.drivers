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
 * $Id: dhd_net80211.h 335620 2012-05-29 19:01:21Z $
 */

#ifndef _dhd_net80211_h_
#define _dhd_net80211_h_

#if defined(IL_BIGENDIAN)
#include <bcmendian.h>
#define htod32(i) (bcmswap32(i))
#define htod16(i) (bcmswap16(i))
#define dtoh32(i) (bcmswap32(i))
#define dtoh16(i) (bcmswap16(i))
#define htodchanspec(i) htod16(i)
#define dtohchanspec(i) dtoh16(i)
#else
#define htod32(i) (i)
#define htod16(i) (i)
#define dtoh32(i) (i)
#define dtoh16(i) (i)
#define htodchanspec(i) (i)
#define dtohchanspec(i) (i)
#endif

#define  IEEE80211_MAX_NUM_OF_BSSIDS    63
#define  IEEE80211_SCAN_TIMEOUT         3000  /* 3 Sec */

#define AUTH_INCLUDES_UNSPEC(_wpa_auth) ((_wpa_auth) & WPA_AUTH_UNSPECIFIED || \
	(_wpa_auth) & WPA2_AUTH_UNSPECIFIED)
#define AUTH_INCLUDES_PSK(_wpa_auth) ((_wpa_auth) & WPA_AUTH_PSK || \
	(_wpa_auth) & WPA2_AUTH_PSK)

/* See if the interface is in AP mode */
#define WLIF_AP_MODE(dhdp)    FALSE

#define dhd_channel2freq(channel) (wf_channel2mhz((channel), \
	((channel) <= CH_MAX_2G_CHANNEL) ? WF_CHAN_FACTOR_2_4_G : WF_CHAN_FACTOR_5_G))

int dhd_net80211_ioctl_set(dhd_pub_t *dhdp, struct ifnet *ifp, u_long cmd, caddr_t data);
int dhd_net80211_ioctl_get(dhd_pub_t *dhdp, struct ifnet *ifp, u_long cmd, caddr_t data);
int dhd_net80211_ioctl_stats(dhd_pub_t *dhdp, struct ifnet *ifp, u_long cmd, caddr_t data);
int dhd_net80211_event_proc(dhd_pub_t *dhdp, struct ifnet *ifp, wl_event_msg_t *event, int ap_mode);
int dhd_net80211_detach(dhd_pub_t *dhdp);
int dhd_net80211_attach(dhd_pub_t *dhdp, struct ifnet *ifp);

#endif /* #ifndef _dhd_net80211_h_ */
