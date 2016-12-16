/*
 * STB SOC specific functions
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
 * $Id: wl_stbsoc.c 661232 2016-09-23 21:33:52Z $
 */

#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#include <linux/module.h>
#endif
#if defined(PLATFORM_INTEGRATED_WIFI) && defined(CONFIG_OF)
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_net.h>
#endif /* PLATFORM_INTEGRATED_WIFI && CONFIG_OF */

#include <wlc_cfg.h>
#include <wlioctl.h>
#include <wlc_channel.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wl_linux.h>
#include <bcmnvram.h>
#include <wl_stbsoc.h>

#if !defined(BCMNVRAMR)
#error "BCMNVRAMR needed to read nvram file for devid."
#endif /* !BCMNVRAMR */

#if !defined(PLATFORM_INTEGRATED_WIFI)
static char* wl_stb_htoa(void *hex, int len, char* buf);
#endif

static char stb_eabuf[ETHER_ADDR_STR_LEN];

uint16
wl_stbsoc_get_devid(void)
{
	char *base = NULL, *nvp = NULL;
	int nvlen = 0;
	uint16 devid = 0;

	/* Get devid from nvram */
	base = nvp = MALLOC(NULL, MAXSZ_NVRAM_VARS);
	if (base == NULL) {
		WL_ERROR(("%s failure. Error code: %d\n", __FUNCTION__, BCME_NOMEM));
	}

	/* Init nvram from nvram file if they exist */
	if (nvram_file_read(&nvp, (int*)&nvlen) == 0)
		devid = (uint16)getintvar(base, "devid");

	MFREE(NULL, base, MAXSZ_NVRAM_VARS);

	return devid;
}

/* Get MAC address from STB device tree */
char*
wlc_stbsoc_get_macaddr(struct device *dev)
{
#if defined(PLATFORM_INTEGRATED_WIFI) && defined(CONFIG_OF)
	struct device_node *dn = dev->of_node;
	return bcm_ether_ntoa((const struct ether_addr *)of_get_mac_address(dn), stb_eabuf);
#else
	void *node_fp = NULL;
	int len = 0;
	char stb_macbuf[MAC_ADDR_STR_LEN];

	node_fp = (void*)osl_os_open_image("/proc/device-tree/rdb/wlan@f17e0000/local-mac-address");
	if (node_fp == NULL) {
		printf("Could not read local mac addr property.\n");
		return NULL;
	}

	len = osl_os_get_image_block(stb_macbuf, MAC_ADDR_STR_LEN, node_fp);
	if (len != MAC_ADDR_STR_LEN) {
		printf("Could not open local mac addr property\n");
		osl_os_close_image(node_fp);
		return NULL;
	}

	osl_os_close_image(node_fp);
	return wl_stb_htoa(stb_macbuf, len, stb_eabuf);
#endif /* PLATFORM_INTEGRATED_WIFI && CONFIG_OF */
}

#if !defined(PLATFORM_INTEGRATED_WIFI)
/* Convert hex to string in brcm format aa:bb:cc:dd:ee:ff */
static char*
wl_stb_htoa(void *hex, int len, char* buf)
{
	int i;
	char *ph = (char*)hex;
	char *pb = buf;

	if ((len <= 0) || (hex == NULL) || (buf == NULL)) {
		return NULL;
	}

	for (i = 0; i < len; i++) {
		snprintf((pb++), 16, "%x", (ph[i] & 0xf0));
		snprintf((pb++), 16, "%x", (ph[i] & 0x0f));
		*(pb++) = ':';
	}

	/* Terminate end of string */
	*(pb-1) = '\0';

	return buf;
}

void
wl_stbsoc_set_drvdata(wl_char_drv_dev_t *cdev, wl_info_t *wl)
{
	cdev->drvdata = (void*)wl;
}

void*
wl_stbsoc_get_drvdata(wl_char_drv_dev_t *cdev)
{
	return cdev->drvdata;
}
#endif /* !PLATFORM_INTEGRATED_WIFI */

int
wl_stbsoc_init(struct wl_platform_info *plat)
{
	int err = BCME_OK;
	stbsocregs_t *d2hregs;
#if defined(PLATFORM_INTEGRATED_WIFI) && defined(CONFIG_OF)
	struct platform_device *platdev = plat->pdev;
	d2hregs = devm_kzalloc(&platdev->dev, sizeof(stbsocregs_t), GFP_KERNEL);
#else
	d2hregs = kmalloc(sizeof(stbsocregs_t), GFP_KERNEL);
#endif /* PLATFORM_INTEGRATED_WIFI && CONFIG_OF */
	if (!d2hregs) {
		return BCME_NOMEM;
	}

	/* Init d2h int regs */
	d2hregs->d2hcpustatus = GET_REGADDR((uintptr)plat->regs, D2H_CPU_STATUS_OFFSET);
	d2hregs->d2hcpuset = GET_REGADDR((uintptr)plat->regs, D2H_CPU_SET_OFFSET);
	d2hregs->d2hcpuclear = GET_REGADDR((uintptr)plat->regs, D2H_CPU_CLEAR_OFFSET);
	d2hregs->d2hcpumaskstatus = GET_REGADDR((uintptr)plat->regs, D2H_CPU_MASKSTATUS_OFFSET);
	d2hregs->d2hcpumaskset = GET_REGADDR((uintptr)plat->regs, D2H_CPU_MASKSET_OFFSET);
	d2hregs->d2hcpumaskclear = GET_REGADDR((uintptr)plat->regs, D2H_CPU_MASKCLEAR_OFFSET);

	plat->plat_priv = (void *)d2hregs;

	return err;
}

void
wl_stbsoc_deinit(struct wl_platform_info *plat)
{
#if defined(PLATFORM_INTEGRATED_WIFI) && defined(CONFIG_OF)
	struct platform_device *platdev = plat->pdev;

	if (plat->plat_priv)
		devm_kfree(&platdev->dev, plat->plat_priv);
#else
	if (plat->plat_priv)
		kfree(plat->plat_priv);
#endif /* PLATFORM_INTEGRATED_WIFI && CONFIG_OF */
}

void
wl_stbsoc_enable_intrs(struct wl_platform_info *plat)
{
	stbsocregs_t *d2hregs = (stbsocregs_t *)plat->plat_priv;

	*(volatile uint32 *)(uintptr)d2hregs->d2hcpumaskclear = 0xFFFFF;
	return;
}

void
wl_stbsoc_disable_intrs(struct wl_platform_info *plat)
{
	stbsocregs_t *d2hregs = (stbsocregs_t *)plat->plat_priv;

	*(volatile uint32 *)(uintptr)d2hregs->d2hcpumaskset = 0xFFFFF;
	return;
}

/* Device to host interrupt handler */
void
wl_stbsoc_d2h_isr(struct wl_platform_info *plat)
{
	stbsocregs_t *d2hregs = (stbsocregs_t *)plat->plat_priv;
	uint32 d2hintstatus;

	d2hintstatus = *(volatile uint32 *)(uintptr)d2hregs->d2hcpustatus;

	/* TODO. nothing for now. */
	return;
}

void
wl_stbsoc_d2h_intstatus(struct wl_platform_info *plat)
{
	stbsocregs_t *d2hregs = (stbsocregs_t *)plat->plat_priv;

	*(volatile uint32 *)(uintptr)d2hregs->d2hcpuclear = 0xFFFFF;
	return;
}
