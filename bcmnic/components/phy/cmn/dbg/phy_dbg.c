/*
 * PHY module debug utilities
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
 * $Id: phy_dbg.c 620395 2016-02-23 01:15:14Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <wlc_dump_reg.h>
#include <phy_mem.h>
#include <phy_dbg_api.h>
#include "phy_type_dbg.h"
#include <phy_dbg.h>
#ifdef PHY_DUMP_BINARY
#include <phy_misc.h>
#include <phy_radio.h>
#include <phy_utils_reg.h>
#endif
typedef struct phy_dbg_mem phy_dbg_mem_t;

/* module private states */
struct phy_dbg_info {
	phy_info_t *pi;
	wlc_dump_reg_info_t *dump;
	phy_type_dbg_fns_t *fns;
	phy_dbg_mem_t *mem;
#ifdef PHY_DUMP_BINARY
	/* PHY and radio dump list and sz */
	phyradregs_list_t *phyreglist;
	uint16 phyreglist_sz;
	phyradregs_list_t *radreglist;
	uint16 radreglist_sz;
#endif
};

/* module private states memory layout */
struct phy_dbg_mem {
	phy_dbg_info_t info;
	phy_type_dbg_fns_t fns;
/* add other variable size variables here at the end */
};

/* default registries sizes */
#ifndef PHY_DUMP_REG_SZ
#define PHY_DUMP_REG_SZ 32
#endif

#if defined(BCMDBG_ERR) || defined(BCMDBG)
uint32 phyhal_msg_level = PHYHAL_ERROR;
#else
uint32 phyhal_msg_level = 0;
#endif

#ifdef PHY_DUMP_BINARY
#define PHYRAD_DUMP_BMPCNT_MASK	0x80000000
#define SIZEOF_PHYRADREG_VALUE	2

/* Number of header bytes for rad/phy binary dump */
#define PHYRAD_BINDUMP_HDSZ	12
#endif

/* module attach/detach */
phy_dbg_info_t *
BCMATTACHFN(phy_dbg_attach)(phy_info_t *pi)
{
	phy_dbg_info_t *di = NULL;
	phy_dbg_mem_t *mem;

	if ((mem = phy_malloc(pi, sizeof(phy_dbg_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	di = &(mem->info);
	di->mem = mem;
	di->pi = pi;
	di->dump = wlc_dump_reg_create(pi->sh->osh, PHY_DUMP_REG_SZ);
	if (di->dump == NULL) {
		PHY_ERROR(("%s: wlc_dump_reg_create failed\n", __FUNCTION__));
		goto fail;
	}
	di->fns = &(mem->fns);

	return di;

fail:
	phy_dbg_detach(di);
	return NULL;
}

void
BCMATTACHFN(phy_dbg_detach)(phy_dbg_info_t *di)
{
	phy_info_t *pi;
	phy_dbg_mem_t *mem;

	if (di == NULL)
		return;

	pi = di->pi;

	if (di->dump) {
		wlc_dump_reg_destroy(di->dump);
	}

	mem = di->mem;

	if (mem != NULL)
		phy_mfree(pi, mem, sizeof(phy_dbg_mem_t));
}

/* add a dump fn */
int
phy_dbg_add_dump_fns(phy_info_t *pi, const char *name,
	phy_dump_dump_fn_t fn, phy_dump_clr_fn_t clr, void *ctx)
{
	phy_dbg_info_t *di = pi->dbgi;

	return wlc_dump_reg_add_fns(di->dump, name, fn, clr, ctx);
}

/*
 * invoke dump function for name.
 */
int
phy_dbg_dump(phy_info_t *pi, const char *name, struct bcmstrbuf *b)
{
	phy_dbg_info_t *di = pi->dbgi;

	return wlc_dump_reg_invoke_dump_fn(di->dump, name, b);
}

/*
 * invoke dump function for name.
 */
int
phy_dbg_dump_clr(phy_info_t *pi, const char *name)
{
	phy_dbg_info_t *di = pi->dbgi;

	return wlc_dump_reg_invoke_clr_fn(di->dump, name);
}

/*
 * List dump names
 */
int
phy_dbg_dump_list(phy_info_t *pi, struct bcmstrbuf *b)
{
	phy_dbg_info_t *di = pi->dbgi;

	return wlc_dump_reg_list(di->dump, b);
}

/*
 * Dump dump registry internals
 */
int
phy_dbg_dump_dump(phy_info_t *pi, struct bcmstrbuf *b)
{
	phy_dbg_info_t *di = pi->dbgi;

	return wlc_dump_reg_dump(di->dump, b);
}

/*
 * Register/unregister PHY type implementation to the Debug module.
 * It returns BCME_XXXX.
 */
int
phy_dbg_register_impl(phy_dbg_info_t *di, phy_type_dbg_fns_t *fns)
{
	*(di->fns) = *fns;

	return BCME_OK;
}

void
phy_dbg_unregister_impl(phy_dbg_info_t *di)
{
	BCM_REFERENCE(di);
	/* nothing to do at this moment */
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
/* dump txerr */
void
phy_dbg_txerr_dump(phy_info_t *pi, uint16 err)
{
	phy_dbg_info_t *di = pi->dbgi;
	phy_type_dbg_fns_t *fns = di->fns;

	if (fns->txerr_dump != NULL) {
		(fns->txerr_dump)(fns->ctx, err);
	}
}
#endif /* BCMDBG || BCMDBG_DUMP */

#if defined(DNG_DBGDUMP)
void wlc_phy_print_phydbg_regs(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_dbg_fns_t *fns = pi->dbgi->fns;
	if (fns->print_phydbg_regs != NULL) {
		(fns->print_phydbg_regs)(fns->ctx);
	}
}
#endif /* DNG_DBGDUMP */

#ifdef WL_MACDBG
/* dump txerr */
void
phy_dbg_gpio_out_enab(phy_info_t *pi, bool enab)
{
	phy_dbg_info_t *di = pi->dbgi;
	phy_type_dbg_fns_t *fns = di->fns;

	if (fns->gpio_out_enab != NULL) {
		(fns->gpio_out_enab)(fns->ctx, enab);
	}
}
#endif

void phy_fatal_error(phy_info_t *pi, phy_crash_reason_t reason_code)
{
	pi->phy_crash_rc = reason_code;
	wlapi_wlc_fatal_error(pi->sh->physhim, (uint32)reason_code);
}

#ifdef PHY_DUMP_BINARY
static uint16
phy_dbg_dump_tblsz(phy_info_t *pi, phyradregs_list_t *list, uint16 totalsize)
{
	uint16 binsz, i;

	binsz = totalsize;
	for (i = 0; i < (totalsize/sizeof(*list)); i++) {
		uint32 bmp_cnt = (list[i].bmp_cnt[0] << 24) |
			(list[i].bmp_cnt[1] << 16) |
			(list[i].bmp_cnt[2] << 8) |
			(list[i].bmp_cnt[3]);

		if (bmp_cnt & PHYRAD_DUMP_BMPCNT_MASK) {
			binsz += (bmp_cnt & (~PHYRAD_DUMP_BMPCNT_MASK)) *
					SIZEOF_PHYRADREG_VALUE;
		} else {
			binsz += bcm_bitcount((uint8 *)&(bmp_cnt), sizeof(bmp_cnt)) *
					SIZEOF_PHYRADREG_VALUE;
		}
	}
	return binsz;
}
#endif /* PHY_DUMP_BINARY */

int
phy_dbg_dump_getlistandsize(wlc_phy_t *ppi, uint8 type)
{
#ifdef PHY_DUMP_BINARY
	phy_info_t *pi = (phy_info_t*)ppi;
	uint16 binsz = PHYRAD_BINDUMP_HDSZ;
	int ret = BCME_OK;

	if (!pi->sh->clk)
		return BCME_NOCLK;

	if ((type != TAG_TYPE_PHY) && (type != TAG_TYPE_RAD)) {
		PHY_INFORM(("%s: wl%d: unsupported tag for phy and radio dump %d\n",
			__FUNCTION__,  pi->sh->unit, type));
		return BCME_BADARG;
	}

	if (type == TAG_TYPE_PHY) {
		pi->dbgi->phyreglist = NULL;
		pi->dbgi->phyreglist_sz = 0;
		if ((ret = phy_misc_getlistandsize(pi, &(pi->dbgi->phyreglist),
				&(pi->dbgi->phyreglist_sz))) != BCME_OK) {
			return ret;
		}
		/* Calculate the total binary size */
		binsz += phy_dbg_dump_tblsz(pi, pi->dbgi->phyreglist, pi->dbgi->phyreglist_sz);
	} else if (type == TAG_TYPE_RAD) {
		pi->dbgi->radreglist = NULL;
		pi->dbgi->radreglist_sz = 0;
		if ((ret = phy_radio_getlistandsize(pi, &(pi->dbgi->radreglist),
				&(pi->dbgi->radreglist_sz))) != BCME_OK) {
			return ret;
		}
		/* Calculate the total binary size */
		binsz += phy_dbg_dump_tblsz(pi, pi->dbgi->radreglist, pi->dbgi->radreglist_sz);
	}

	/* Last check for bad length calculation and header addition */
	if (binsz == PHYRAD_BINDUMP_HDSZ)
		return BCME_BADLEN;

	return (int)binsz;
#else /* PHY_DUMP_BINARY */
	return BCME_UNSUPPORTED;
#endif /* PHY_DUMP_BINARY */
}

int
phy_dbg_dump_binary(wlc_phy_t *ppi, uint8 type, uchar *p, int * buf_len)
{
#ifdef PHY_DUMP_BINARY
	phy_info_t *pi = (phy_info_t*)ppi;
	phy_dbg_info_t *di = pi->dbgi;
	phy_type_dbg_fns_t *fns = di->fns;
	phyradregs_list_t *pregs, *preglist;
	uint i;
	uint16 addr;
	uint16	val_16 = 0;
	uint array_size = 0;
	int rem_len = *buf_len;
	uint8 corenum;

	corenum = (uint8) PHYCORENUM(pi->pubpi->phy_corenum);

	if (phy_dbg_dump_getlistandsize(ppi, type) < 0)
		return BCME_NOTREADY;

	if (rem_len < PHYRAD_BINDUMP_HDSZ) {
		PHY_INFORM(("%s: wl%d: \n buffer too short \n", __FUNCTION__, pi->sh->unit));
		return BCME_BUFTOOSHORT;
	}

	if (type == TAG_TYPE_PHY) {
		*(uint16 *)p = TAG_TYPE_PHY;
		*(uint16 *)(p + 2) = (uint16) pi->pubpi->phy_type;
		*(uint16 *)(p + 4) = (uint16) pi->pubpi->phy_rev;
		*(uint16 *)(p + 6) = PHYRADDBG1_TYPE;
		preglist = pi->dbgi->phyreglist;
		array_size = pi->dbgi->phyreglist_sz / sizeof(*pi->dbgi->phyreglist);
		PHY_INFORM(("\n %s: phytype: %d phyrev: %d structtype: %d\n", __FUNCTION__,
				pi->pubpi->phy_type, pi->pubpi->phy_rev, PHYRADDBG1_TYPE));
	} else if (type == TAG_TYPE_RAD) {
		*(uint16 *)p = TAG_TYPE_RAD;
		*(uint16 *)(p + 2) = pi->pubpi->radioid;
		*(uint16 *)(p + 4) = (pi->pubpi->radiorev);
		*(uint16 *)(p + 6) = PHYRADDBG1_TYPE;
		preglist = pi->dbgi->radreglist;
		array_size = pi->dbgi->radreglist_sz / sizeof(*pi->dbgi->radreglist);
		PHY_INFORM(("\n %s: radioid: %d wlc->band->radiorev: %d structtype: %d\n",
			__FUNCTION__, pi->pubpi->radioid, pi->pubpi->radiorev, PHYRADDBG1_TYPE));
	} else {
		PHY_INFORM(("%s: unsupported type in function\n", __FUNCTION__));
			return BCME_UNSUPPORTED;
	}

	*(uint16 *)(p + 8) = (uint16) array_size;
	*(uint16 *)(p + 10) = (uint16) corenum;
	PHY_INFORM(("\n %s: arraysize: %d corenum: %d \n", __FUNCTION__, array_size, corenum));

	p += PHYRAD_BINDUMP_HDSZ;
	rem_len -= PHYRAD_BINDUMP_HDSZ;
	PHY_INFORM(("\n %s: sizeof(phyradregs_list_t)=%d \n",
		__FUNCTION__, (int) sizeof(phyradregs_list_t)));

	for (i = 0; i < array_size; i++) {
		uint32 bitmap = 0, cnt = 0, bmp_cnt;

		if (rem_len < (int)sizeof(phyradregs_list_t)) {
			return BCME_BUFTOOSHORT;
		}
		pregs = &preglist[i];
		bcopy(pregs, p, sizeof(phyradregs_list_t));
		p += sizeof(phyradregs_list_t);
		rem_len -= sizeof(phyradregs_list_t);
		addr = pregs->addr;

		/* bmp_cnt has either bitmap or count, if the MSB is set, then
		*   bmp_cnt[30:0] has count, i.e, number of registers who values are
		*   contigous from the start address. If MSB is zero, then the value
		*   should be considered as a bitmap of 31 discreet addresses
		*/
		bmp_cnt = (pregs->bmp_cnt[0] << 24) | (pregs->bmp_cnt[1] << 16) |
				(pregs->bmp_cnt[2] << 8) | (pregs->bmp_cnt[3]);

		if (bmp_cnt & PHYRAD_DUMP_BMPCNT_MASK)
			cnt = (bmp_cnt) & (~PHYRAD_DUMP_BMPCNT_MASK);
		else
			bitmap = (bmp_cnt) & (~PHYRAD_DUMP_BMPCNT_MASK);

		while (bitmap || cnt) {
			if ((cnt) || (bitmap & 0x1)) {
				if (rem_len < (int)sizeof(val_16)) {
					return BCME_BUFTOOSHORT;
				}

				if (type == TAG_TYPE_PHY) {
					/*
					 * The TableDataWide register is only valid to read once the
					 * TableID and TableOffset registers are set.
					 */
					if (fns->phyregaddr != NULL) {
						val_16 = (fns->phyregaddr)(fns->ctx, addr);
					} else {
						val_16 = phy_utils_read_phyreg(pi, addr);
					}
					PHY_INFORM(("\n %s : phyreg addr=0x%x, val=0x%x, "
						"rem_len=%d\n",
						__FUNCTION__, addr, val_16, rem_len));
				} else if (type == TAG_TYPE_RAD) {
					val_16 = phy_utils_read_radioreg(pi, addr);
					PHY_INFORM(("\n %s : radioreg addr=0x%x, val=0x%x, "
						"rem_len=%d\n", __FUNCTION__,
						addr, val_16, rem_len));
				}

				*(uint16 *)(p) = val_16;
				p += sizeof(val_16);
				rem_len -= sizeof(val_16);
			}

			/* Decrement the count or delete bitmap bits */
			if (cnt)
				cnt--;
			else
				bitmap = bitmap >> 1;

			++addr;
		}
	}
	*buf_len = *buf_len - rem_len;
	return BCME_OK;
#else /* PHY_DUMP_BINARY */
	return BCME_UNSUPPORTED;
#endif /* PHY_DUMP_BINARY */
}
