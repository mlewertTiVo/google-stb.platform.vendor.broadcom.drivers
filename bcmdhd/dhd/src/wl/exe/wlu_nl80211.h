/*
 * Definitions for DHD nl80211 driver interface.
 *
 * Copyright (C) 1999-2017, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: $
 */

#ifndef DHDU_NL80211_H_
#define DHDU_NL80211_H_

#ifdef NL80211

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

/* libnl 1.x compatibility code */
#if !defined(CONFIG_LIBNL20) && !defined(CONFIG_LIBNL30)
#define nl_sock		nl_handle
#endif

struct wl_netlink_info
{
	struct nl_sock *nl;
	struct nl_cb *cb;
	int nl_id;
	int ifidx;
};

int wlnl_sock_connect(struct wl_netlink_info *wl_nli);
void wlnl_sock_disconnect(struct wl_netlink_info *wl_nli);
int wlnl_do_vndr_cmd(struct wl_netlink_info *wl_nli, wl_ioctl_t *ioc);

#endif /* NL80211 */

#endif /* DHDU_NL80211_H_ */
