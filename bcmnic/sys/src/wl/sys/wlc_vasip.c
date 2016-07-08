/*
 * VASIP related functions
 * Broadcom 802.11abg Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_vasip.c 612761 2016-01-14 23:06:00Z $
 */

#include <wlc_cfg.h>

#ifdef VASIP_HW_SUPPORT
#include <typedefs.h>
#include <wlc_types.h>
#include <siutils.h>
#include <wlioctl.h>
#include <wlc_pub.h>
#include <wlioctl.h>
#include <wlc.h>
#include <wlc_dbg.h>
#include <phy_misc_api.h>
#include <wlc_hw_priv.h>
#include <d11vasip_code.h>
#include <pcicfg.h>
#include <wl_export.h>
#include <wlc_vasip.h>
#include <wlc_dump.h>

/* defines */
#define VASIP_COUNTERS_ADDR_OFFSET	0x20000
#define VASIP_STATUS_ADDR_OFFSET	0x20100

#define VASIP_COUNTERS_LMT		256
#define VASIP_DEFINED_COUNTER_NUM	26
#define SVMP_MEM_OFFSET_MAX		0xfffff
#define SVMP_MEM_DUMP_LEN_MAX		4096

/* local prototypes */

/* iovar table */
enum {
	IOV_VASIP_COUNTERS_CLEAR,
	IOV_SVMP_MEM
};

static int
wlc_vasip_doiovar(void *context, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int vsize, struct wlc_if *wlcif);

static const bcm_iovar_t vasip_iovars[] = {
	{NULL, 0, 0, 0, 0}
};

void
BCMATTACHFN(wlc_vasip_detach)(wlc_info_t *wlc)
{
	wlc_module_unregister(wlc->pub, "vasip", wlc);
}

int
BCMATTACHFN(wlc_vasip_attach)(wlc_info_t *wlc)
{
	wlc_pub_t *pub = wlc->pub;
	int err = BCME_OK;

	if ((err = wlc_module_register(pub, vasip_iovars, "vasip",
		wlc, wlc_vasip_doiovar, NULL, NULL, NULL))) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
			wlc->pub->unit, __FUNCTION__));
		return err;
	}


	return err;
}

static int
wlc_vasip_doiovar(void *context, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int vsize, struct wlc_if *wlcif)
{

	int err = BCME_OK;
	switch (actionid) {

	}
	return err;
}

/* write vasip code to vasip program memory */
static void
wlc_vasip_write(wlc_hw_info_t *wlc_hw, const uint32 vasip_code[], const uint nbytes)
{
	osl_t *osh = wlc_hw->osh;
	uchar * bar1va;
	uint32 bar1_size;
	uint32 *vasip_mem;
	int i, count, idx;
	uint32 vasipaddr;

	WL_TRACE(("wl%d: %s\n", wlc_hw->unit, __FUNCTION__));

	ASSERT(ISALIGNED(nbytes, sizeof(uint32)));

	count = (nbytes/sizeof(uint32));

	/* save current core */
	idx = si_coreidx(wlc_hw->sih);
	if (si_setcore(wlc_hw->sih, ACPHY_CORE_ID, 0) != NULL) {
		/* get the VASIP memory base */
		vasipaddr = si_addrspace(wlc_hw->sih, 0);
		/* restore core */
		(void)si_setcoreidx(wlc_hw->sih, idx);
	} else {
		WL_ERROR(("%s: wl%d: Failed to find ACPHY core \n",
			__FUNCTION__, wlc_hw->unit));
		ASSERT(0);
		return;
	}
	OSL_PCI_WRITE_CONFIG(osh, PCI_BAR1_WIN, sizeof(uint32), vasipaddr);
	bar1_size = wl_pcie_bar1(wlc_hw->wlc->wl, &bar1va);

	BCM_REFERENCE(bar1_size);
	ASSERT(bar1va != NULL && bar1_size != 0);

	vasip_mem = (uint32 *)bar1va;

	/* write vasip code to program memory */
	for (i = 0; i < count; i++) {
		vasip_mem[i] = vasip_code[i];
	}

	/* save base address in global */
	wlc_hw->vasip_addr = vasip_mem;
}

/* initialize vasip */
void
wlc_vasip_init(wlc_hw_info_t *wlc_hw)
{
	const uint32 *vasip_code = NULL;
	uint nbytes = 0;
	uint8 vasipver;

	WL_TRACE(("wl%d: %s\n", wlc_hw->unit, __FUNCTION__));

	vasipver = phy_misc_get_vasip_ver((phy_info_t *)wlc_hw->band->pi);
	if (vasipver == VASIP_NOVERSION) {
		return;
	}

	if (vasipver == 0) {
		vasip_code = d11vasip0;
		nbytes = d11vasip0sz;
	} else {
		WL_ERROR(("%s: wl%d: unsupported vasipver %d \n",
			__FUNCTION__, wlc_hw->unit, vasipver));
		ASSERT(0);
		return;
	}

	if (vasip_code != NULL) {
		/* stop the vasip processor */
		phy_misc_vasip_proc_reset((phy_info_t *)wlc_hw->band->pi, 1);

		/* write binary to the vasip program memory */
		wlc_vasip_write(wlc_hw, vasip_code, nbytes);

		/* reset and start the vasip processor */
		phy_misc_vasip_proc_reset((phy_info_t *)wlc_hw->band->pi, 0);
	}
}

#endif /* VASIP_HW_SUPPORT */
