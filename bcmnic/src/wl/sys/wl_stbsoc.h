/*
 * wl_stbsoc.c exported functions and definitions
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
 * $Id: wl_stbsoc.h 661232 2016-09-23 21:33:52Z $
 */

#ifndef _wl_stbsoc_h_
#define _wl_stbsoc_h_

#include <wl_linux.h>

extern uint16 wl_stbsoc_get_devid(void);
extern int wl_stbsoc_init(struct wl_platform_info *plat);
extern void wl_stbsoc_deinit(struct wl_platform_info *plat);
extern void wl_stbsoc_enable_intrs(struct wl_platform_info *plat);
extern void wl_stbsoc_disable_intrs(struct wl_platform_info *plat);
extern void wl_stbsoc_d2h_isr(struct wl_platform_info *plat);
extern void wl_stbsoc_d2h_intstatus(struct wl_platform_info *plat);

extern char* wlc_stbsoc_get_macaddr(struct device *dev);

#if !defined(PLATFORM_INTEGRATED_WIFI)
extern void wl_stbsoc_set_drvdata(wl_char_drv_dev_t *cdev, wl_info_t *wl);
extern void* wl_stbsoc_get_drvdata(wl_char_drv_dev_t *cdev);
#endif /* !PLATFORM_INTEGRATED_WIFI */

#define D2H_CPU_STATUS_OFFSET	0x100
#define D2H_CPU_SET_OFFSET	0x104
#define D2H_CPU_CLEAR_OFFSET	0x108
#define D2H_CPU_MASKSTATUS_OFFSET	0x10c
#define D2H_CPU_MASKSET_OFFSET	0x110
#define D2H_CPU_MASKCLEAR_OFFSET	0x114

#define INTF_D2H_BASEMASK		0xFFFFFFFFFFFFF000

#define MAC_ADDR_STR_LEN	6

#define WLAN_D2H_CPU_INTR 133 /* 5 + 3 * 32 = 101 + 32 (ARM GIC pre-processor) = 133 */

#define WLAN_INTF_START		0xf17e0000 /* offset = 0x217e0000 */
#define WLAN_INTF_END		0xf17e0400 /* offset = 0x217e0400 */
#define WLAN_INTF_SIZE		(WLAN_INTF_END - WLAN_INTF_START)

typedef struct _stbsocregs {
	uint64	d2hcpustatus;
	uint64	d2hcpuset;
	uint64	d2hcpuclear;
	uint64	d2hcpumaskstatus;
	uint64	d2hcpumaskset;
	uint64	d2hcpumaskclear;
} stbsocregs_t;

#define GET_REGADDR(base, offset)	((base & INTF_D2H_BASEMASK) | (offset))

#endif /* _wl_stbsoc_h_ */
