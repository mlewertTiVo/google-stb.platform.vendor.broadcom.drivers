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
 * $Id: wlc_vasip.c 624453 2016-03-11 18:23:04Z $
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

#ifdef BCM7271
#include <wl_bcm7xxx_dbg.h>
#undef WL_ERROR
#define WL_ERROR(args) do {BCM7XXX_TRACE(); Bcm7xxxPrintSpace(NULL); printf args;} while (0)
#endif /* BCM7271 */


/* defines */
#define VASIP_COUNTERS_ADDR_OFFSET	0x20000
#define VASIP_STATUS_ADDR_OFFSET	0x20100
#define VASIP_MCS_ADDR_OFFSET		0x21e10
#define VASIP_SPECTRUM_TBL_ADDR_OFFSET	0x26800

#define VASIP_COUNTERS_LMT		256
#define VASIP_DEFINED_COUNTER_NUM	26
#define SVMP_MEM_OFFSET_MAX		0x28000
#define SVMP_MEM_DUMP_LEN_MAX		4096

#define SVMP_ACCESS_VIA_PHYTBL
#define SVMP_ACCESS_BLOCK_SIZE 16

/* local prototypes */

/* read/write svmp memory */
int wlc_svmp_read_blk(wlc_hw_info_t *wlc_hw, uint16 *val, uint32 offset, uint16 len);
int wlc_svmp_write_blk(wlc_hw_info_t *wlc_hw, uint16 *val, uint32 offset, uint16 len);

/* iovar table */
enum {
	IOV_VASIP_COUNTERS_CLEAR,
	IOV_SVMP_MEM,
	IOV_MU_RATE
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
#ifndef SVMP_ACCESS_VIA_PHYTBL
static void
wlc_vasip_write(wlc_hw_info_t *wlc_hw, const uint32 vasip_code[], const uint nbytes)
{
	osl_t *osh = wlc_hw->osh;
	uchar *bar_va;
	uint32 bar_size;
	uint32 *vasip_mem;
	int idx;
	uint32 vasipaddr;
#ifdef WL_MU_TX
	int i, count;
	count = (nbytes/sizeof(uint32));
#endif

	WL_TRACE(("wl%d: %s\n", wlc_hw->unit, __FUNCTION__));

	ASSERT(ISALIGNED(nbytes, sizeof(uint32)));

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
	bar_size = wl_pcie_bar2(wlc_hw->wlc->wl, &bar_va);
	if (bar_size) {
		OSL_PCI_WRITE_CONFIG(osh, PCI_BAR1_CONTROL, sizeof(uint32), vasipaddr);
	} else {
		bar_size = wl_pcie_bar1(wlc_hw->wlc->wl, &bar_va);
		OSL_PCI_WRITE_CONFIG(osh, PCI_BAR1_WIN, sizeof(uint32), vasipaddr);
	}

	BCM_REFERENCE(bar_size);
	ASSERT(bar_va != NULL && bar_size != 0);

	vasip_mem = (uint32 *)bar_va;

	/* write vasip code to program memory */
	BCM_REFERENCE(vasip_code);
	BCM_REFERENCE(nbytes);
#ifdef WL_MU_TX
	for (i = 0; i < count; i++) {
		vasip_mem[i] = vasip_code[i];
	}
#endif

	/* save base address in global */
	wlc_hw->vasip_addr = (uint16 *)vasip_mem;
}

#ifdef VASIP_SPECTRUM_ANALYSIS
static void
wlc_vasip_spectrum_tbl_write(wlc_hw_info_t *wlc_hw,
        const uint32 vasip_spectrum_tbl[], const uint nbytes_tbl)
{
	osl_t *osh = wlc_hw->osh;
	uchar *bar_va;
	uint32 bar_size;
	uint32 *vasip_mem;
	int idx;
	uint32 vasipaddr;

	int n, tbl_count;
	tbl_count = (nbytes_tbl/sizeof(uint32));

	WL_TRACE(("wl%d: %s\n", wlc_hw->unit, __FUNCTION__));

	ASSERT(ISALIGNED(nbytes, sizeof(uint32)));

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
	bar_size = wl_pcie_bar2(wlc_hw->wlc->wl, &bar_va);
	if (bar_size) {
		OSL_PCI_WRITE_CONFIG(osh, PCI_BAR1_CONTROL, sizeof(uint32), vasipaddr);
	} else {
		bar_size = wl_pcie_bar1(wlc_hw->wlc->wl, &bar_va);
		OSL_PCI_WRITE_CONFIG(osh, PCI_BAR1_WIN, sizeof(uint32), vasipaddr);
	}

	BCM_REFERENCE(bar_size);
	ASSERT(bar_va != NULL && bar_size != 0);

	vasip_mem = (uint32 *)bar_va;

	/* write spactrum analysis tables to VASIP_SPECTRUM_TBL_ADDR_OFFSET */
	BCM_REFERENCE(vasip_spectrum_tbl);
	BCM_REFERENCE(nbytes_tbl);
	for (n = 0; n < tbl_count; n++) {
		vasip_mem[VASIP_SPECTRUM_TBL_ADDR_OFFSET + n] = vasip_spectrum_tbl[i];
	}

	/* save base address in global */
	wlc_hw->vasip_addr = (uint16 *)vasip_mem;
}
#endif /* VASIP_SPECTRUM_ANALYSIS */
#endif /* SVMP_ACCESS_VIA_PHYTBL */

/* initialize vasip */
void
BCMINITFN(wlc_vasip_init)(wlc_hw_info_t *wlc_hw)
{
	const uint32 *vasip_code = NULL;
	uint nbytes = 0;
	uint8 vasipver;

#ifdef VASIP_SPECTRUM_ANALYSIS
	const uint32 *vasip_spectrum_tbl = NULL;
	uint nbytes_tbl = 0;
#endif

#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */


	WL_TRACE(("wl%d: %s\n", wlc_hw->unit, __FUNCTION__));

	vasipver = phy_misc_get_vasip_ver((phy_info_t *)wlc_hw->band->pi);

#ifdef BCM7271
	BCM7XXX_DBG(("%s(%d) vasip code ver %d.%d vasipver=0x%08x\n",
		__FUNCTION__, __LINE__, d11vasipcode_major, d11vasipcode_minor, vasipver));
#endif /* BCM7271 */


	if (vasipver == VASIP_NOVERSION) {

#ifdef BCM7271
		BCM7XXX_EXIT_ERR();
#endif /* BCM7271 */

		return;
	}

	phy_misc_vasip_clk_set((phy_info_t *)wlc_hw->band->pi, 1);

	if (vasipver == 0 || vasipver == 2 || vasipver == 3) {
		vasip_code = d11vasip0;
		nbytes = d11vasip0sz;
#ifdef VASIP_SPECTRUM_ANALYSIS
		vasip_spectrum_tbl = d11vasip_tbl;
		nbytes_tbl = d11vasip_tbl_sz;
#endif
	} else {
		WL_ERROR(("%s: wl%d: unsupported vasipver %d \n",
			__FUNCTION__, wlc_hw->unit, vasipver));
		ASSERT(0);
#ifdef BCM7271
		BCM7XXX_EXIT_ERR();
#endif /* BCM7271 */
		return;
	}

	if (vasip_code != NULL) {
		/* stop the vasip processor */
		phy_misc_vasip_proc_reset((phy_info_t *)wlc_hw->band->pi, 1);

		/* write binary to the vasip program memory */
#ifdef SVMP_ACCESS_VIA_PHYTBL
		phy_misc_vasip_bin_write((phy_info_t *)wlc_hw->band->pi, vasip_code, nbytes);
#else
		wlc_vasip_write(wlc_hw, vasip_code, nbytes);
#endif /* SVMP_ACCESS_VIA_PHYTBL */

		/* write spectrum tables to the vasip SVMP M4 0x26800 */
#ifdef VASIP_SPECTRUM_ANALYSIS
#ifdef SVMP_ACCESS_VIA_PHYTBL
		phy_misc_vasip_spectrum_tbl_write((phy_info_t *)wlc_hw->band->pi,
		        vasip_spectrum_tbl, nbytes_tbl);
#else
		wlc_vasip_spectrum_tbl_write(wlc_hw, vasip_spectrum_tbl, nbytes_spectrum_tbl);
#endif /* SVMP_ACCESS_VIA_PHYTBL */
#endif /* VASIP_SPECTRUM_ANALYSIS */

		/* reset and start the vasip processor */
#ifdef WL_MU_TX
		phy_misc_vasip_proc_reset((phy_info_t *)wlc_hw->band->pi, 0);
#endif
	}

#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */

}


int
wlc_svmp_read_blk(wlc_hw_info_t *wlc_hw, uint16 *val, uint32 offset, uint16 len)
{
#ifndef SVMP_ACCESS_VIA_PHYTBL
	uint16 *svmp_addr;
#endif
	uint16 i;

	if (!VASIP_PRESENT(wlc_hw->corerev)) {
		return BCME_UNSUPPORTED;
	}

	if (!wlc_hw->clk) {
		return BCME_NOCLK;
	}

	if ((offset + len) > SVMP_MEM_OFFSET_MAX) {
		return BCME_RANGE;
	}

#ifndef SVMP_ACCESS_VIA_PHYTBL
	svmp_addr = (uint16 *)(wlc_hw->vasip_addr + offset);
	for (i = 0; i < len; i++) {
		val[i] = svmp_addr[i];
	}
#else
	//phy_misc_vasip_svmp_read_blk((phy_info_t *)wlc_hw->band->pi, offset, len, val);
	for (i = 0; i < (len/SVMP_ACCESS_BLOCK_SIZE); i++) {
		phy_misc_vasip_svmp_read_blk((phy_info_t *)wlc_hw->band->pi,
		        offset, SVMP_ACCESS_BLOCK_SIZE, val);
		offset += SVMP_ACCESS_BLOCK_SIZE;
		val += SVMP_ACCESS_BLOCK_SIZE;
		len -= SVMP_ACCESS_BLOCK_SIZE;
	}
	if (len > 0) {
		phy_misc_vasip_svmp_read_blk((phy_info_t *)wlc_hw->band->pi, offset, len, val);
	}
#endif /* SVMP_ACCESS_VIA_PHYTBL */

	return BCME_OK;
}

int
wlc_svmp_write_blk(wlc_hw_info_t *wlc_hw, uint16 *val, uint32 offset, uint16 len)
{
#ifndef SVMP_ACCESS_VIA_PHYTBL
	volatile uint16 * svmp_addr;
#endif
	uint16 i;

	if (!VASIP_PRESENT(wlc_hw->corerev)) {
		return BCME_UNSUPPORTED;
	}

	if (!wlc_hw->clk) {
		return BCME_NOCLK;
	}

	if ((offset + len) > SVMP_MEM_OFFSET_MAX) {
		return BCME_RANGE;
	}

#ifndef SVMP_ACCESS_VIA_PHYTBL
	svmp_addr = (volatile uint16 *)(wlc_hw->vasip_addr + offset);
	for (i = 0; i < len; i++) {
		if (val[i] != 0xffff) {
			svmp_addr[i] = val[i];
		}
	}
#else
	//phy_misc_vasip_svmp_write_blk((phy_info_t *)wlc_hw->band->pi, offset, len, val);
	for (i = 0; i < (len/SVMP_ACCESS_BLOCK_SIZE); i++) {
		phy_misc_vasip_svmp_write_blk((phy_info_t *)wlc_hw->band->pi,
		        offset, SVMP_ACCESS_BLOCK_SIZE, val);
		offset += SVMP_ACCESS_BLOCK_SIZE;
		val += SVMP_ACCESS_BLOCK_SIZE;
		len -= SVMP_ACCESS_BLOCK_SIZE;
	}
	if (len > 0) {
		phy_misc_vasip_svmp_write_blk((phy_info_t *)wlc_hw->band->pi, offset, len, val);
	}
#endif /* SVMP_ACCESS_VIA_PHYTBL */

	return BCME_OK;
}
#endif /* VASIP_HW_SUPPORT */
