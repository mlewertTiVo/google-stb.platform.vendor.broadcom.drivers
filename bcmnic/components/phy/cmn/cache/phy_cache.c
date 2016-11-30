/*
 * CACHE module implementation
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
 * $Id: phy_cache.c 643020 2016-06-12 01:13:23Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmutils.h>
#include <bcmwifi_channels.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_api.h>
#include <phy_chanmgr_notif.h>
#include "phy_cache_cfg.h"
#include <phy_type_cache.h>
#include <phy_cache_api.h>

/* cache registry entry */
typedef struct {
	phy_cache_init_fn_t init;
	phy_cache_save_fn_t save;
	phy_cache_restore_fn_t restore;
	phy_cache_dump_fn_t dump;
	phy_cache_ctx_t *ctx;
	uint32 offset;
	uint32 flags;	/* See PHY_CACHE_FLAG_XXXX */
} phy_cache_reg_t;

/* cache control entry */
typedef struct {
	uint8 chan;	/* control channel # */
	uint8 state;
	uint16 ttdel;	/* time to delete */
} phy_cache_ctl_t;

/* state */
#define CTL_ST_USED	(1<<0)

/* module private states */
struct phy_cache_info {
	phy_info_t *pi;
	phy_type_cache_fns_t	*fns;	/* PHY specific function ptrs */

	/* control index */
	uint8 ctl_cur;

	/* cache registry */
	uint8 reg_sz;
	uint8 reg_cnt;
	phy_cache_reg_t *reg;
	/* entry size */
	uint32 bufsz;
	/* control flags */
	uint16 *flags;

	/* cache control */
	uint8 ctl_sz;
	phy_cache_ctl_t *ctl;

	/* cache entries */
	uint8 **bufp;
};

/* control index */
#define CTL_CUR_INV	255		/* invalid index */

/* control flags */
#define CTL_FLAG_VAL	(1<<0)	/* valid flag */

/* module private states memory layout */
typedef struct {
	phy_cache_info_t info;
	phy_cache_reg_t reg[PHY_CACHE_REG_SZ];
	uint16 flags[PHY_CACHE_REG_SZ * PHY_CACHE_SZ];
	phy_cache_ctl_t ctl[PHY_CACHE_SZ];
	uint8 *bufp[PHY_CACHE_SZ];
	phy_type_cache_fns_t fns;
} phy_cache_mem_t;


/* local function declaration */
#ifdef NEW_PHY_CAL_ARCH
static int phy_cache_init(phy_init_ctx_t *ctx);
static int phy_cache_chspec_notif(phy_chanmgr_notif_ctx_t *ctx, phy_chanmgr_notif_data_t *data);
#endif /* NEW_PHY_CAL_ARCH */
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int phy_cache_dump(void *ctx, struct bcmstrbuf *b);
#endif
#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(WLTEST)
static int wlc_phydump_phycal(void *ctx, struct bcmstrbuf *b);
#endif

/* attach/detach */
phy_cache_info_t *
BCMATTACHFN(phy_cache_attach)(phy_info_t *pi)
{
	phy_cache_info_t *info;
#ifdef NEW_PHY_CAL_ARCH
	uint16 events = (PHY_CHANMGR_NOTIF_OPCHCTX_OPEN | PHY_CHANMGR_NOTIF_OPCHCTX_CLOSE |
	                 PHY_CHANMGR_NOTIF_OPCH_CHG | PHY_CHANMGR_NOTIF_CH_CHG);
#endif /* NEW_PHY_CAL_ARCH */
	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((info = phy_malloc(pi, sizeof(phy_cache_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	info->pi = pi;

	info->reg_sz = PHY_CACHE_REG_SZ;

	info->reg = ((phy_cache_mem_t *)info)->reg;

	info->flags = ((phy_cache_mem_t *)info)->flags;

	info->bufp = ((phy_cache_mem_t *)info)->bufp;

	info->ctl = ((phy_cache_mem_t *)info)->ctl;

	info->ctl_sz = PHY_CACHE_SZ;
	info->ctl_cur = CTL_CUR_INV;
	info->fns = &((phy_cache_mem_t *)info)->fns;

#ifdef NEW_PHY_CAL_ARCH
	/* register init fn */
	if (phy_init_add_init_fn(pi->initi, phy_cache_init, info, PHY_INIT_CACHE) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn failed\n", __FUNCTION__));
		goto fail;
	}

	/* register chspec notification callback */
	if (phy_chanmgr_notif_add_interest(pi->chanmgr_notifi,
	                phy_cache_chspec_notif, info, PHY_CHANMGR_NOTIF_ORDER_CACHE,
	                events) != BCME_OK) {
		PHY_ERROR(("%s: phy_chanmgr_notif_add_interest failed\n", __FUNCTION__));
		goto fail;
	}
#endif /* NEW_PHY_CAL_ARCH */

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	/* register dump callback */
	phy_dbg_add_dump_fn(pi, "phycache", phy_cache_dump, info);
#endif
#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(WLTEST)
	phy_dbg_add_dump_fn(pi, "phycal", wlc_phydump_phycal, pi);
#endif

	return info;

	/* error */
fail:
	phy_cache_detach(info);
	return NULL;
}

void
BCMATTACHFN(phy_cache_detach)(phy_cache_info_t *info)
{
	phy_info_t *pi;
	uint i;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (info == NULL) {
		PHY_INFORM(("%s: null cache module\n", __FUNCTION__));
		return;
	}

	pi = info->pi;

	/* free cache entries */
	for (i = 0; i < info->ctl_sz; i ++) {
		if (info->bufp[i] == NULL)
			continue;
		phy_mfree(pi, info->bufp[i], info->bufsz);
	}

	phy_mfree(pi, info, sizeof(phy_cache_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_cache_register_impl)(phy_cache_info_t *ci, phy_type_cache_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));


	*ci->fns = *fns;
	return BCME_OK;
}

void
BCMATTACHFN(phy_cache_unregister_impl)(phy_cache_info_t *ci)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}


/*
 * Reserve cubby in cache entry and register operation callbacks for the cubby.
 */
int
BCMATTACHFN(phy_cache_reserve_cubby)(phy_cache_info_t *ci, phy_cache_init_fn_t init,
	phy_cache_save_fn_t save, phy_cache_restore_fn_t restore, phy_cache_dump_fn_t dump,
	phy_cache_ctx_t *ctx, uint32 size, uint32 flags, phy_cache_cubby_id_t *ccid)
{
	uint reg;
	uint32 offset;

	PHY_TRACE(("%s: size %u\n", __FUNCTION__, size));

	/* check registry occupancy */
	if (ci->reg_cnt == ci->reg_sz) {
		PHY_ERROR(("%s: too many cache control entries\n", __FUNCTION__));
		return BCME_NORESOURCE;
	}

	/* sanity check */
	ASSERT(save != NULL);
	ASSERT(restore != NULL);
	ASSERT(size > 0);

	reg = ci->reg_cnt;
	offset = ci->bufsz;

	/* use one registry entry */
	ci->reg[reg].init = init;
	ci->reg[reg].save = save;
	ci->reg[reg].restore = restore;
	ci->reg[reg].dump = dump;
	ci->reg[reg].ctx = ctx;
	ci->reg[reg].offset = offset;
	ci->reg[reg].flags = flags;

	/* account for the size and round it up to the next pointer */
	ci->bufsz += ROUNDUP(size, sizeof(void *));
	ci->reg_cnt ++;

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
#endif /* BCMDBG || BCMDBG_DUMP */

	/* use the registry index as the cubby ID */
	*ccid = (phy_cache_cubby_id_t)reg;

	return BCME_OK;
}

#ifdef NEW_PHY_CAL_ARCH
/* Find the control struct index (same as cache entry index) for 'chanspec' */
static int
phy_cache_find_ctl(phy_cache_info_t *ci, chanspec_t chanspec)
{
	int ctl;

	/* TODO: change to some faster search when necessary */

	for (ctl = 0; ctl < (int)ci->ctl_sz; ctl ++) {
		if ((ci->ctl[ctl].state & CTL_ST_USED) &&
		    ci->ctl[ctl].chan == wf_chspec_ctlchan(chanspec))
			return ctl;
	}

	return BCME_NOTFOUND;
}

/*
 * Create a cache entry for the 'chanspec' if one doesn't exist.
 */
static int
phy_cache_add_entry(phy_cache_info_t *ci, chanspec_t chanspec)
{
#if !PHY_CACHE_PREALLOC
	phy_info_t *pi = ci->pi;
#endif
	int ctl;

	PHY_TRACE(("%s: chanspec 0x%x\n", __FUNCTION__, chanspec));

	if ((ctl = phy_cache_find_ctl(ci, chanspec)) >= 0) {
		ASSERT(ci->bufp[ctl] != NULL);
		goto init;
	}

	/* find an empty entry */
	for (ctl = 0; ctl < (int)ci->ctl_sz; ctl ++) {
		if (ci->ctl[ctl].state & CTL_ST_USED)
			continue;
#if !PHY_CACHE_PREALLOC
		ci->bufp[ctl] = phy_malloc_fatal(pi, ci->bufsz);
#else
		ASSERT(ci->bufp[ctl] != NULL);
#endif
		goto init;
	}

	return BCME_NORESOURCE;

init:
	ASSERT(ctl >= 0 && ctl < (int)ci->ctl_sz);
	ci->ctl[ctl].chan = wf_chspec_ctlchan(chanspec);
	ci->ctl[ctl].state |= CTL_ST_USED;

	return BCME_OK;
}

/*
 * Delete the cache entry for 'chanspec' if one exists.
 */
static int
phy_cache_del_entry(phy_cache_info_t *ci, chanspec_t chanspec)
{
#if !PHY_CACHE_PREALLOC
	phy_info_t *pi = ci->pi;
#endif
	int ctl;

	PHY_TRACE(("%s: chanspec 0x%x\n", __FUNCTION__, chanspec));

	if ((ctl = phy_cache_find_ctl(ci, chanspec)) < 0)
		return ctl;

	ASSERT(ci->bufp[ctl] != NULL);
#if !PHY_CACHE_PREALLOC
	phy_mfree(pi, ci->bufp[ctl], ci->bufsz);
	ci->bufp[ctl] = NULL;
#endif
	bzero(&ci->ctl[ctl], sizeof(ci->ctl[ctl]));
	bzero(&ci->flags[ctl * ci->reg_sz], sizeof(ci->flags[0]) * ci->reg_sz);

	return BCME_OK;
}
#endif /* NEW_PHY_CAL_ARCH */

/* Invoke 'save' callback to save client states to cubby 'reg' of entry 'ctl' */
static int
_phy_cache_save(phy_cache_info_t *ci, uint ctl, uint reg)
{
	uint8 *p;
	int err;

	PHY_TRACE(("%s: ctl %u reg %u\n", __FUNCTION__, ctl, reg));

	ASSERT(ctl < ci->ctl_sz);
	ASSERT(reg < ci->reg_cnt);

	/* save one */
	ASSERT(ci->bufp[ctl] != NULL);
	p = ci->bufp[ctl] + ci->reg[reg].offset;
	ASSERT(ci->reg[reg].save != NULL);
	if ((err = (ci->reg[reg].save)(ci->reg[reg].ctx, p)) != BCME_OK) {
		PHY_ERROR(("%s: save %p err %d\n",
		           __FUNCTION__, ci->reg[reg].save, err));
		return err;
	}
#ifdef NEW_PHY_CAL_ARCH
	ci->flags[ctl * ci->reg_sz + reg] |= CTL_FLAG_VAL;
#endif /* NEW_PHY_CAL_ARCH */

	return BCME_OK;
}

#ifdef NEW_PHY_CAL_ARCH
/* Automatically invoke all applicable 'save' callbacks when the cache entry 'ctl'
 * is made non-current.
 */
static int
phy_cache_auto_save(phy_cache_info_t *ci, uint ctl)
{
	uint reg;
	int err;

	PHY_TRACE(("%s: ctl %u\n", __FUNCTION__, ctl));

	/* Is this necessary? */
	ASSERT(ctl == ci->ctl_cur);

	/* save to 'auto save' cubbies */
	for (reg = 0; reg < ci->reg_cnt; reg ++) {
		if (!(ci->reg[reg].flags & PHY_CACHE_FLAG_AUTO_SAVE))
			continue;
		if ((err = _phy_cache_save(ci, ctl, reg)) != BCME_OK) {
			PHY_ERROR(("%s: auto save %u err %d\n", __FUNCTION__, reg, err));
			(void)err;
		}
	}

	return BCME_OK;
}

/*
 * Set the cache entry associated with 'chanspec' as the current and
 * restore client states through the registered 'restore' callbacks.
 */
static int
phy_cache_restore(phy_cache_info_t *ci, chanspec_t chanspec)
{
	int ctl;
	uint reg;
	uint8 *p;
	int err;

	PHY_TRACE(("%s: chanspec 0x%x\n", __FUNCTION__, chanspec));

	if ((ctl = phy_cache_find_ctl(ci, chanspec)) < 0)
		return ctl;

	/* perform auto save */
	if (ci->ctl_cur != CTL_CUR_INV)
		phy_cache_auto_save(ci, ci->ctl_cur);

	ci->ctl_cur = (uint8)ctl;

	/* restore all */
	for (reg = 0; reg < ci->reg_cnt; reg ++) {
		/* initialize client states before the cubby content is 'valid' */
		if (!(ci->flags[ctl * ci->reg_sz + reg] & CTL_FLAG_VAL)) {
			if (ci->reg[reg].init != NULL)
				(ci->reg[reg].init)(ci->reg[reg].ctx);
			continue;
		}
		/* restore client states */
		ASSERT(ci->bufp[ctl] != NULL);
		ASSERT(ci->reg[reg].offset < ci->bufsz);
		p = ci->bufp[ctl] + ci->reg[reg].offset;
		ASSERT(ci->reg[reg].restore != NULL);
		if ((err = (ci->reg[reg].restore)(ci->reg[reg].ctx, p)) != BCME_OK) {
			PHY_ERROR(("%s: restore %p err %d\n",
			           __FUNCTION__, ci->reg[reg].restore, err));
			(void)err;
		}
	}

	return BCME_OK;
}
#endif /* NEW_PHY_CAL_ARCH */

/*
 * Save the client states to cache through the registered 'save' callback.
 */
int
phy_cache_save(phy_cache_info_t *ci, phy_cache_cubby_id_t ccid)
{
	uint ctl;
	uint reg;

	PHY_TRACE(("%s: ccid %u\n", __FUNCTION__, ccid));

	ctl = ci->ctl_cur;

	if (ctl == CTL_CUR_INV)
		return BCME_ERROR;

	ASSERT(ctl < ci->ctl_sz);

	/* 'ccid' is the registry index */
	reg = (uint)(uintptr)ccid;

	if (reg >= ci->reg_cnt)
		return BCME_BADARG;

	/* save one */
	return _phy_cache_save(ci, ctl, reg);
}

/*
 * Invalidate the current cache entry index (due to someone is
 * changing the radio chanspec without going through the cache).
 */
#ifdef NEW_PHY_CAL_ARCH
static void
phy_cache_inv(phy_cache_info_t *ci)
{
	PHY_TRACE(("%s\n", __FUNCTION__));

	/* perform auto save */
	if (ci->ctl_cur != CTL_CUR_INV)
		phy_cache_auto_save(ci, ci->ctl_cur);

	ci->ctl_cur = CTL_CUR_INV;
}

/* chspec notification callback */
static int
phy_cache_chspec_notif(phy_chanmgr_notif_ctx_t *ctx, phy_chanmgr_notif_data_t *data)
{
	phy_cache_info_t *ci = (phy_cache_info_t *)ctx;
	int status;

	PHY_TRACE(("%s\n", __FUNCTION__));

	switch (data->event) {
	case PHY_CHANMGR_NOTIF_OPCHCTX_OPEN:
		status = phy_cache_add_entry(ci, data->new);
		break;
	case PHY_CHANMGR_NOTIF_OPCHCTX_CLOSE:
		status = phy_cache_del_entry(ci, data->new);
		break;
	case PHY_CHANMGR_NOTIF_OPCH_CHG:
		status = phy_cache_restore(ci, data->new);
		break;
	case PHY_CHANMGR_NOTIF_CH_CHG:
		phy_cache_inv(ci);
		status = BCME_OK;
		break;
	default:
		status = BCME_ERROR;
		ASSERT(0);
		break;
	}

	return status;
}

/* allocate cache entries */
static int
WLBANDINITFN(phy_cache_init)(phy_init_ctx_t *ctx)
{
#if PHY_CACHE_PREALLOC
	phy_cache_info_t *ci = (phy_cache_info_t *)ctx;
	phy_info_t *pi = ci->pi;
	uint i;
#endif

	PHY_TRACE(("%s\n", __FUNCTION__));

#if PHY_CACHE_PREALLOC
	/* allocate cache entries */
	for (i = 0; i < ci->ctl_sz; i ++) {
		if (ci->bufp[i] != NULL)
			continue;
		if ((ci->bufp[i] = phy_malloc(pi, ci->bufsz)) == NULL) {
			PHY_ERROR(("%s: unable to allocate memory\n", __FUNCTION__));
			return BCME_NOMEM;
		}
	}
#endif

	return BCME_OK;
}
#endif /* NEW_PHY_CAL_ARCH */

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int
phy_cache_dump(void *ctx, struct bcmstrbuf *b)
{
	phy_cache_info_t *ci = (phy_cache_info_t *)ctx;
	uint i, j;
	uint8 *p;
	int err;

	bcm_bprintf(b, "cur: %u\n", ci->ctl_cur);
	bcm_bprintf(b, "reg: max %u cnt %u bufsz %u\n",
	            ci->reg_sz, ci->reg_cnt, ci->bufsz);
	for (i = 0; i < ci->reg_cnt; i ++) {
		bcm_bprintf(b, "  idx %u: init %p save %p restore %p dump %p ctx %p offset %u\n",
		            i, ci->reg[i].init, ci->reg[i].save, ci->reg[i].restore,
		            ci->reg[i].dump, ci->reg[i].ctx, ci->reg[i].offset);
	}

	bcm_bprintf(b, "ctl: sz %u\n", ci->ctl_sz);
	for (i = 0; i < ci->ctl_sz; i ++) {
		bcm_bprintf(b, "  idx %u: state 0x%x chan %u\n",
		            i, ci->ctl[i].state, ci->ctl[i].chan);
		for (j = 0; j < ci->reg_cnt; j ++) {
			bcm_bprintf(b, "    idx %u: flags 0x%x\n",
			            j, ci->flags[i * ci->reg_sz + j]);
		}
	}

	bcm_bprintf(b, "bufp: sz %u\n", ci->ctl_sz);
	for (i = 0; i < ci->ctl_sz; i ++) {
		bcm_bprintf(b, "  idx %u: bufp %p\n", i, ci->bufp[i]);
	}

	bcm_bprintf(b, "buf: sz %u\n", ci->ctl_sz);
	for (i = 0; i < ci->ctl_sz; i ++) {
		bcm_bprintf(b, "entry %u >\n", i);
		for (j = 0; j < ci->reg_cnt; j ++) {
			bcm_bprintf(b, "  idx %u: offset %u >\n", j, ci->reg[j].offset);
			if (ci->bufp[i] == NULL)
				continue;
			if (ci->reg[j].dump == NULL)
				continue;
			p = ci->bufp[i] + ci->reg[j].offset;
			err = (ci->reg[j].dump)(ci->reg[j].ctx, p, b);
			if (err != BCME_OK) {
				PHY_ERROR(("%s: dump %p err %d\n",
				           __FUNCTION__, ci->reg[j].dump, err));
			}
		}
	}

	return BCME_OK;
}
#endif /* BCMDBG || BCMDBG_DUMP */



#if defined(PHYCAL_CACHING)
void
wlc_phy_cal_cache(wlc_phy_t *ppi)
{
	phy_info_t * pi = (phy_info_t *)ppi;
	phy_cache_info_t * cachei = pi->cachei;
	phy_type_cache_fns_t * fns = cachei->fns;
	phy_type_cache_ctx_t * cache_ctx = (phy_type_cache_ctx_t *)pi;

	ASSERT(pi != NULL);

	if (fns->cal_cache != NULL) {
		fns->cal_cache(cache_ctx);
	}
}

int
wlc_phy_cal_cache_restore(phy_info_t *pi)
{
	phy_cache_info_t * cachei = pi->cachei;
	phy_type_cache_fns_t *fns = cachei->fns;
	phy_type_cache_ctx_t * cache_ctx = (phy_type_cache_ctx_t *)pi;

	ASSERT(pi != NULL);

	/* TODO: Implement other phy types after ac_phy */
	if (fns->restore != NULL) {
		return fns->restore(cache_ctx);
	}
	else
		return BCME_UNSUPPORTED;
}

int
wlc_phy_cal_cache_return(wlc_phy_t *ppi)
{
	return wlc_phy_cal_cache_restore((phy_info_t *)ppi);
}
#endif /* PHYCAL_CACHING */

#if defined(PHYCAL_CACHING) && (defined(BCMDBG) || defined(WLTEST))
static void
wlc_phydump_chanctx(phy_info_t *phi, struct bcmstrbuf *b)
{
	ch_calcache_t *ctx = phi->phy_calcache;
	phy_cache_info_t * cachei = phi->cachei;
	phy_type_cache_fns_t *fns = cachei->fns;
	phy_type_cache_ctx_t * cache_ctx = (phy_type_cache_ctx_t *)phi;

	ASSERT(phi != NULL);


	if (phi->HW_FCBS) {
		return;
	}

	bcm_bprintf(b, "Current chanspec: 0x%x\n", phi->radio_chanspec);
	while (ctx) {
		bcm_bprintf(b, "%sContext found for chanspec: 0x%x\n",
			(ctx->valid)? "Valid ":"",
			ctx->chanspec);
		if (fns->dump_chanctx != NULL) {
			fns->dump_chanctx(cache_ctx, ctx, b);
		}

		ctx = ctx->next;
	}
}
#endif /* PHYCAL_CACHING && (BCMDBG || WLTEST)  */

#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(WLTEST)
static int
wlc_phydump_phycal(void *ctx, struct bcmstrbuf *b)
{
	phy_info_t *pi = ctx;
	phy_cache_info_t * cachei = pi->cachei;
	phy_type_cache_fns_t *fns = cachei->fns;
	phy_type_cache_ctx_t * cache_ctx = (phy_type_cache_ctx_t *)pi;

	ASSERT(pi != NULL);

	if (!pi->sh->up)
		return BCME_NOTUP;

	if (fns->dump_cal != NULL) {
		fns->dump_cal(cache_ctx, b);
	}
	else
		return BCME_UNSUPPORTED;

#if defined(PHYCAL_CACHING) && (defined(BCMDBG) || defined(WLTEST))
	wlc_phydump_chanctx(pi, b);
#endif /* defined(PHYCAL_CACHING) && (defined(BCMDBG) || defined(WLTEST)) */

	return BCME_OK;
}
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(WLTEST) */

/* Moved from wlc_phy_cmn.c */
#if defined(PHYCAL_CACHING)
int
wlc_phy_reinit_chanctx(phy_info_t *pi, ch_calcache_t *ctx, chanspec_t chanspec)
{
	ASSERT(ctx);
	ctx->valid = FALSE;
	ctx->chanspec = chanspec;
	ctx->creation_time = pi->sh->now;
	ctx->cal_info.last_cal_time = 0;
	ctx->cal_info.last_papd_cal_time = 0;
	ctx->cal_info.last_cal_temp = -50;
	ctx->in_use = TRUE;
	return 0;
}

int
wlc_phy_invalidate_chanctx(wlc_phy_t *ppi, chanspec_t chanspec)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	ch_calcache_t *ctx = pi->phy_calcache;

	while (ctx) {
		if (ctx->chanspec == chanspec) {
			ctx->valid = FALSE;
			ctx->cal_info.last_cal_time = 0;
			ctx->cal_info.last_papd_cal_time = 0;
			ctx->cal_info.last_cal_temp = -50;
			return 0;
		}
		ctx = ctx->next;
	}

	return 0;
}
/*   This function will try and reuse the existing ctx:
	return 0 --> couldn't find any ctx
	return 1 --> channel ctx exist
	return 2 --> grabbed an invalid ctx
*/
int
wlc_phy_reuse_chanctx(wlc_phy_t *ppi, chanspec_t chanspec)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	ch_calcache_t *ctx = pi->phy_calcache;

	/* Check for existing */
	if (wlc_phy_get_chanctx(pi, chanspec)) {
		PHY_INFORM(("wl%d: %s | using existing chanctx for Ch %d\n",
			PI_INSTANCE(pi), __FUNCTION__,
			CHSPEC_CHANNEL(chanspec)));
		return 1;
	}

	/* Check if there are any invalid entries and use them */
	while (ctx) {
		if (!ctx->in_use)
		{
			wlc_phy_reinit_chanctx(pi, ctx, chanspec);
			PHY_INFORM(("wl%d: %s | grabbed an unused chanctx\n",
				PI_INSTANCE(pi), __FUNCTION__));
			return 2;
		}
		ctx = ctx->next;
	}

	PHY_INFORM(("wl%d: %s | couldn't find any ctx for Ch %d\n",
		PI_INSTANCE(pi), __FUNCTION__,
		CHSPEC_CHANNEL(chanspec)));

	return BCME_OK;
}

void
wlc_phy_update_chctx_glacial_time(wlc_phy_t *ppi, chanspec_t chanspec)
{
	ch_calcache_t *ctx;
	phy_info_t *pi = (phy_info_t *)ppi;
	if ((ctx = wlc_phy_get_chanctx((phy_info_t *)ppi, chanspec)))
		ctx->cal_info.last_cal_time = pi->sh->now - pi->sh->glacial_timer;
}

ch_calcache_t *
wlc_phy_get_chanctx_oldest(phy_info_t *phi)
{
	ch_calcache_t *ctx = phi->phy_calcache;
	ch_calcache_t *ctx_lo_ctime = ctx;
	while (ctx->next) {
		ctx = ctx->next;
		if (ctx_lo_ctime->creation_time > ctx->creation_time)
			ctx_lo_ctime = ctx;
	}
	return ctx_lo_ctime;
}

ch_calcache_t *
wlc_phy_get_chanctx(phy_info_t *phi, chanspec_t chanspec)
{
	ch_calcache_t *ctx = phi->phy_calcache;
	while (ctx) {
		if (ctx->chanspec == chanspec)
			return ctx;
		ctx = ctx->next;
	}
	return NULL;
}

bool
wlc_phy_chan_iscached(wlc_phy_t *ppi, chanspec_t chanspec)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	ch_calcache_t *ctx;
	bool ret = 0;

	ctx = wlc_phy_get_chanctx(pi, chanspec);

	if (ctx != NULL)
		if (ctx->valid)
			ret = 1;

	return ret;
}

void wlc_phy_get_all_cached_ctx(wlc_phy_t *ppi, chanspec_t *chanlist)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	ch_calcache_t *ctx = pi->phy_calcache;
	int i = 0;

	while (ctx) {
		*(chanlist+i) = ctx->chanspec;
		i++;
		ctx = ctx->next;
	}
}

void
wlc_phy_get_cachedchans(wlc_phy_t *ppi, chanspec_t *chanlist)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	ch_calcache_t *ctx = pi->phy_calcache;
	chanlist[0] = 0;

	while (ctx) {
		if (ctx->valid) {
			chanlist[0] += 1;
			chanlist[chanlist[0]] = ctx->chanspec;
		}
		ctx = ctx->next;
	}
}

/*
 * TODO: Following function is not called from anywhere and it has phy specific code and therefore
 * disabled
 */

uint32
wlc_phy_get_current_cachedchans(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	if (pi) {
#if defined(PHYCAL_CACHING)|| defined(WL_MODESW)
		return pi->phy_calcache_num;
#endif
	}
	return 0;
}

#endif /* PHYCAL_CACHING */
