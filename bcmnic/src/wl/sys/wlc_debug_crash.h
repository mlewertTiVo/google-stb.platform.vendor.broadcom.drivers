
/**
 * @file
 * @brief
 * Common (OS-independent) portion of Broadcom debug crash validation
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
 * $Id: wlc_debug_crash.h 614820 2016-01-23 17:16:17Z $
 */

#ifndef __WLC_DEBUG_CRASH_H
#define __WLC_DEBUG_CRASH_H

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <wlc_pub.h>

#define DBG_CRSH_TYPE_RD_RANDOM		0x00
#define DBG_CRSH_TYPE_RD_INV_CORE	0x01
#define DBG_CRSH_TYPE_WR_INV_CORE	0x02
#define DBG_CRSH_TYPE_RD_INV_WRAP	0x03
#define DBG_CRSH_TYPE_WR_INV_WRAP	0x04
#define DBG_CRSH_TYPE_RD_RES_CORE	0x05
#define DBG_CRSH_TYPE_WR_RES_CORE	0x06
#define DBG_CRSH_TYPE_RD_RES_WRAP	0x07
#define DBG_CRSH_TYPE_WR_RES_WRAP	0x08
#define DBG_CRSH_TYPE_RD_CORE_NO_CLK	0x09
#define DBG_CRSH_TYPE_WR_CORE_NO_CLK	0x0A
#define DBG_CRSH_TYPE_RD_CORE_NO_PWR	0x0B
#define DBG_CRSH_TYPE_WR_CORE_NO_PWR	0x0C
#define DBG_CRSH_TYPE_PCIe_AER		0x0D
#define DBG_CRSH_TYPE_POWERCYCLE	0x0E
#define DBG_CRSH_TYPE_TRAP		0x0E
#define DBG_CRSH_TYPE_HANG		0x0F
#define DBG_CRSH_TYPE_PHYREAD		0x10
#define DBG_CRSH_TYPE_PHYWRITE		0x11
#define DBG_CRSH_TYPE_INV_PHYREAD	0x12
#define DBG_CRSH_TYPE_INV_PHYWRITE	0x13
#define DBG_CRSH_TYPE_DUMP_STATE	0x14


/* Radio/PHY health check crash scenarios - reserved 0x16 to 0x30 */
#define DBG_CRSH_TYPE_RADIO_HEALTHCHECK_START	0x16
#define DBG_CRSH_TYPE_DESENSE_LIMITS		0x17
#define DBG_CRSH_TYPE_BASEINDEX_LIMITS		0x18
#define DBG_CRSH_TYPE_TXCHAIN_INVALID		0x19
#define DBG_CRSH_TYPE_RADIO_HEALTHCHECK_LAST	0x30
#define DBG_CRSH_TYPE_LAST			0x31

#define DBG_CRASH_SRC_DRV		0x00
#define DBG_CRASH_SRC_FW		0x01
#define DBG_CRASH_SRC_UCODE		0x02

enum {
	IOV_DEBUG_CRASH = 0,
	IOV_DEBUG_LAST
	};

wlc_debug_crash_info_t * wlc_debug_crash_attach(wlc_info_t *wlc);
void wlc_debug_crash_detach(wlc_debug_crash_info_t *ctxt);
extern uint32 wlc_debug_crash_execute_crash(wlc_info_t* wlc, uint32 type, uint32 delay, int * err);

#endif /* __WLC_DEBUG_CRASH_H */
