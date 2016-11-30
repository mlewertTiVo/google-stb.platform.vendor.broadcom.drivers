/*
 * Monitor Mode routines.
 * This header file housing the define and function use by DHD
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: bcmwifi_monitor.h 512698 2016-02-11 13:12:15Z $
 */
#ifndef _BCMWIFI_MONITOR_H_
#define _BCMWIFI_MONITOR_H_

#include <proto/monitor.h>
#include <bcmmsgbuf.h>
#include <bcmwifi_radiotap.h>

#define MAX_RADIOTAP_SIZE	sizeof(wl_radiotap_vht_t)
#define MAX_MON_PKT_SIZE	(4096 + MAX_RADIOTAP_SIZE)

typedef struct monitor_info monitor_info_t;

extern uint16 bcmwifi_monitor_create(monitor_info_t**);
extern void bcmwifi_monitor_delete(monitor_info_t* info);
extern uint16 bcmwifi_monitor(monitor_info_t* info,
	host_rxbuf_cmpl_t* msg, void *pdata, uint16 len, void* pout, uint16* offset);
extern uint16 wl_rxsts_to_rtap(struct wl_rxsts* rxsts, void *pdata, uint16 len, void* pout);

#endif /* _BCMWIFI_MONITOR_H_ */
