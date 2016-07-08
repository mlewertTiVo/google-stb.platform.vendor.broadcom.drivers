/*
 * Common interface to the 802.11 Station Control Block (scb) structure
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
 * $Id: wlc_scb.c 603066 2015-11-30 22:51:38Z $
 */

/**
 * @file
 * @brief
 * SCB is a per-station data structure that is stored in the wl driver. SCB container provides a
 * mechanism through which different wl driver modules can each allocate and maintain private space
 * in the scb used for their own purposes. The scb subsystem (wlc_scb.c) does not need to know
 * anything about the different modules that may have allocated space in scb. It can also be used
 * by per-port code immediately after wlc_attach() has been done (but before wlc_up()).
 *
 * - "container" refers to the entire space within scb that can be allocated opaquely to other
 *   modules.
 * - "cubby" refers to the per-module private space in the container.
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>

#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_scb_ratesel.h>
#include <wlc_macfltr.h>
#include <wlc_cubby.h>
#include <wlc_dump.h>
#include <wlc_stf.h>

#define SCB_MAGIC 0x0505a5a5u

#ifdef SCBFREELIST
#ifdef INT_SCB_OPT
#error "SCBFREELIST incompatible with INT_SCB_OPT"
/* To make it compatible, freelist needs to track internal vs external */
#endif /* INT_SCB_OPT */
#endif /* SCBFREELIST */

/** structure for storing public and private global scb module state */
struct scb_module {
	wlc_info_t	*wlc;			/**< global wlc info handle */
	uint16		nscb;			/**< total number of allocated scbs */
	uint16		aging_threshold;	/**< min # scbs before starting aging */
	uint16		scbtotsize;		/**< total scb size including scb_info */
	uint16		scbpubsize;		/**< struct scb size */
	wlc_cubby_info_t *cubby_info;
#ifdef SCBFREELIST
	struct scb      *free_list;		/**< Free list of SCBs */
#endif
	int		cfgh;			/**< scb bsscfg cubby handle */
	bcm_notif_h 	scb_state_notif_hdl;	/**< scb state notifier handle. */
#ifdef SCB_MEMDBG
	uint32		scballoced;		/**< how many scb calls to 'wlc_scb_allocmem' */
	uint32		scbfreed;		/**< how many scb calls to 'wlc_scb_freemem' */
	uint32		freelistcount;		/**< how many scb's are in free_list */
#endif /* SCB_MEMDBG */
};

/** station control block - one per remote MAC address */
struct scb_info {
	struct scb_info *hashnext;	/**< pointer to next scb under same hash entry */
	struct scb_info	*next;		/**< pointer to next allocated scb */
	uint8 *secbase;			/* secondary cubby allocation base pointer */
#ifdef SCB_MEMDBG
	struct scb_info *hashnext_copy;
	struct scb_info *next_copy;
	uint32 magic;
#endif
};

static void wlc_scb_hash_add(scb_module_t *scbstate, wlc_bsscfg_t *cfg, int bandunit,
	struct scb *scb);
static void wlc_scb_hash_del(scb_module_t *scbstate, wlc_bsscfg_t *cfg,
	struct scb *scbd);
static void wlc_scb_list_add(scb_module_t *scbstate, wlc_bsscfg_t *cfg,
	struct scb *scb);
static void wlc_scb_list_del(scb_module_t *scbstate, wlc_bsscfg_t *cfg,
	struct scb *scbd);

static struct scb *wlc_scbvictim(wlc_info_t *wlc);

static int wlc_scbinit(wlc_info_t *wlc, wlc_bsscfg_t *cfg, int bandunit,
	struct scb *scb);
static void wlc_scbdeinit(wlc_info_t *wlc, struct scb *scbd);

static struct scb_info *wlc_scb_allocmem(scb_module_t *scbstate);
static void wlc_scb_freemem(scb_module_t *scbstate, struct scb_info *scbinfo);

static void wlc_scb_init_rates(wlc_info_t *wlc, wlc_bsscfg_t *cfg, int bandunit,
	struct scb *scb);


#ifdef SCBFREELIST
static void wlc_scbfreelist_free(scb_module_t *scbstate);
#endif /* SCBFREELIST */

/* Each scb has the layout:
 * +-----------------+ <<== offset 0
 * | struct scb      |
 * +-----------------+ <<== scbpubsize
 * | struct scb_info |
 * +-----------------+ <<== scbtotsize
 * | cubbies         |
 * +-----------------+
 */
#define SCBINFO(_scbstate, _scb) \
	(_scb ? (struct scb_info *)((uint8 *)(_scb) + (_scbstate)->scbpubsize) : NULL)
#define SCBPUB(_scbstate, _scbinfo) \
	(_scbinfo ? (struct scb *)((uint8 *)(_scbinfo) - (_scbstate)->scbpubsize) : NULL)

#ifdef SCB_MEMDBG

#define SCBSANITYCHECK(_scbstate, _scb) \
	if (((_scb) != NULL) &&	\
	    ((SCBINFO(_scbstate, _scb)->magic != SCB_MAGIC) || \
	     (SCBINFO(_scbstate, _scb)->hashnext != SCBINFO(_scbstate, _scb)->hashnext_copy) || \
	     (SCBINFO(_scbstate, _scb)->next != SCBINFO(_scbstate, _scb)->next_copy))) { \
		osl_panic("scbinfo corrupted: magic: 0x%x hn: %p hnc: %p n: %p nc: %p\n", \
		          SCBINFO(_scbstate, _scb)->magic, SCBINFO(_scbstate, _scb)->hashnext, \
		          SCBINFO(_scbstate, _scb)->hashnext_copy, \
		          SCBINFO(_scbstate, _scb)->next, SCBINFO(_scbstate, _scb)->next_copy); \
	}

#define SCBFREESANITYCHECK(_scbstate, _scb) \
	if (((_scb) != NULL) &&	\
	    ((SCBINFO(_scbstate, _scb)->magic != ~SCB_MAGIC) || \
	     (SCBINFO(_scbstate, _scb)->next != SCBINFO(_scbstate, _scb)->next_copy))) { \
		osl_panic("scbinfo corrupted: magic: 0x%x hn: %p hnc: %p n: %p nc: %p\n", \
		          SCBINFO(_scbstate, _scb)->magic, SCBINFO(_scbstate, _scb)->hashnext, \
		          SCBINFO(_scbstate, _scb)->hashnext_copy,			\
		          SCBINFO(_scbstate, _scb)->next, SCBINFO(_scbstate, _scb)->next_copy); \
	}

#else

#define SCBSANITYCHECK(_scbstate, _scb)		do {} while (0)
#define SCBFREESANITYCHECK(_scbstate, _scb)	do {} while (0)

#endif /* SCB_MEMDBG */

/** bsscfg cubby */
typedef struct scb_bsscfg_cubby {
	struct scb	**scbhash[MAXBANDS];	/**< scb hash table */
	uint8		nscbhash;		/**< scb hash table size */
	struct scb	*scb;			/**< station control block link list */
} scb_bsscfg_cubby_t;

#define SCB_BSSCFG_CUBBY(ss, cfg) ((scb_bsscfg_cubby_t *)BSSCFG_CUBBY(cfg, (ss)->cfgh))

static int wlc_scb_bsscfg_init(void *context, wlc_bsscfg_t *cfg);
static void wlc_scb_bsscfg_deinit(void *context, wlc_bsscfg_t *cfg);
#define wlc_scb_bsscfg_dump NULL

static void wlc_scb_bsscfg_scbclear(struct wlc_info *wlc, wlc_bsscfg_t *cfg, bool perm);

/* # scb hash buckets */
#define SCB_HASH_N	(uint8)((wlc->pub->tunables->maxscb + 7) >> 3)
#define SCB_HASH_SZ	(sizeof(struct scb *) * MAXBANDS * SCB_HASH_N)

#define	SCBHASHINDEX(hash, ea)	(((ea)[3] ^ (ea)[4] ^ (ea)[5]) % (hash))

static int
wlc_scb_bsscfg_init(void *context, wlc_bsscfg_t *cfg)
{
	scb_module_t *scbstate = (scb_module_t *)context;
	wlc_info_t *wlc = scbstate->wlc;
	scb_bsscfg_cubby_t *scb_cfg = SCB_BSSCFG_CUBBY(scbstate, cfg);
	struct scb **scbhash;
	uint32 i;

	ASSERT(scb_cfg->scbhash[0] == NULL);

	scbhash = MALLOCZ(wlc->osh, SCB_HASH_SZ);
	if (scbhash == NULL) {
		WL_ERROR((WLC_BSS_MALLOC_ERR, WLCWLUNIT(wlc), WLC_BSSCFG_IDX(cfg), __FUNCTION__,
			(int)SCB_HASH_SZ, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}

	scb_cfg->nscbhash = SCB_HASH_N;	/* # scb hash buckets */
	for (i = 0; i < MAXBANDS; i++) {
		scb_cfg->scbhash[i] = scbhash + (i * SCB_HASH_N);
	}

	return BCME_OK;
}

static void
wlc_scb_bsscfg_deinit(void *context, wlc_bsscfg_t *cfg)
{
	scb_module_t *scbstate = (scb_module_t *)context;
	wlc_info_t *wlc = scbstate->wlc;
	scb_bsscfg_cubby_t *scb_cfg = SCB_BSSCFG_CUBBY(scbstate, cfg);

	/* clear all scbs */
	wlc_scb_bsscfg_scbclear(wlc, cfg, TRUE);

	scb_cfg->nscbhash = 0;

	ASSERT(scb_cfg->scbhash[0] != NULL);

	/* N.B.: the hash is contiguously allocated across multiple bands */
	MFREE(wlc->osh, scb_cfg->scbhash[0], SCB_HASH_SZ);
	scb_cfg->scbhash[0] = NULL;
}

static void
wlc_scb_bss_state_upd(void *ctx, bsscfg_state_upd_data_t *evt_data)
{
	scb_module_t *scbstate = (scb_module_t *)ctx;
	wlc_info_t *wlc = scbstate->wlc;
	wlc_bsscfg_t *cfg = evt_data->cfg;

	/* clear all old non-permanent scbs for IBSS only if assoc recreate is not enabled */
	/* and WLC_BSSCFG_PRESERVE cfg flag is not set */
	if (!evt_data->old_up && cfg->up) {
		if (!cfg->BSS &&
		    !(ASSOC_RECREATE_ENAB(wlc->pub) && (cfg->flags & WLC_BSSCFG_PRESERVE))) {
			wlc_scb_bsscfg_scbclear(wlc, cfg, FALSE);
		}
	}
	/* clear all non-permanent scbs when disabling the bsscfg */
	else if (evt_data->old_enable && !cfg->enable) {
		wlc_scb_bsscfg_scbclear(wlc, cfg, FALSE);
	}
}

/* # scb cubby registry entries */
#define SCB_CUBBY_REG_N  (wlc->pub->tunables->maxscbcubbies)
#define SCB_CUBBY_REG_SZ (sizeof(scb_module_t))

/* Min # scbs to have before starting aging.
 * Set to 1 for now as nop.
 */
#define SCB_AGING_THRESHOLD	1

scb_module_t *
BCMATTACHFN(wlc_scb_attach)(wlc_info_t *wlc)
{
	scb_module_t *scbstate;

	if ((scbstate = MALLOCZ(wlc->osh, SCB_CUBBY_REG_SZ)) == NULL) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__, (int)SCB_CUBBY_REG_SZ,
			MALLOCED(wlc->osh)));
		goto fail;
	}
	scbstate->wlc = wlc;

	if ((scbstate->cubby_info =
	     wlc_cubby_attach(wlc, OBJR_SCB_CUBBY, SCB_CUBBY_REG_N)) == NULL) {
		WL_ERROR(("wl%d: %s: wlc_cubby_attach failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* TODO: maybe need a tunable here. */
	/* This is to limit the scb aging in low memory situation to happen
	 * less often in cases like aging out the only scb.
	 * For SCBFREELIST build we can set it to wlc->pub->tunables->maxscb
	 * to bypass the low memory processing.
	 */
	scbstate->aging_threshold = SCB_AGING_THRESHOLD;

	/* reserve cubby in the bsscfg container for per-bsscfg private data */
	if ((scbstate->cfgh = wlc_bsscfg_cubby_reserve(wlc, sizeof(scb_bsscfg_cubby_t),
			wlc_scb_bsscfg_init, wlc_scb_bsscfg_deinit, wlc_scb_bsscfg_dump,
			(void *)scbstate)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	scbstate->scbpubsize = (uint16)sizeof(struct scb);
	scbstate->scbtotsize = scbstate->scbpubsize;
	scbstate->scbtotsize += (uint16)sizeof(struct scb_info);

	if (wlc_bsscfg_state_upd_register(wlc, wlc_scb_bss_state_upd, scbstate) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_state_upd_register failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}


	/* create notification list for scb state change. */
	if (bcm_notif_create_list(wlc->notif, &scbstate->scb_state_notif_hdl) != BCME_OK) {
		WL_ERROR(("wl%d: %s: scb bcm_notif_create_list() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	return scbstate;

fail:
	wlc_scb_detach(scbstate);
	return NULL;
}

void
BCMATTACHFN(wlc_scb_detach)(scb_module_t *scbstate)
{
	wlc_info_t *wlc;

	if (!scbstate)
		return;

	wlc = scbstate->wlc;

	if (scbstate->scb_state_notif_hdl != NULL)
		bcm_notif_delete_list(&scbstate->scb_state_notif_hdl);

	(void)wlc_bsscfg_state_upd_unregister(wlc, wlc_scb_bss_state_upd, scbstate);

#ifdef SCBFREELIST
	wlc_scbfreelist_free(scbstate);
#endif

	wlc_cubby_detach(wlc, scbstate->cubby_info);

	MFREE(wlc->osh, scbstate, SCB_CUBBY_REG_SZ);
}

/* Methods for iterating along a list of scb */

/** Direct access to the next */
static struct scb *
wlc_scb_next(scb_module_t *scbstate, struct scb *scb)
{
	if (scb) {
		struct scb_info *scbinfo = SCBINFO(scbstate, scb);
		SCBSANITYCHECK(scbstate, scb);
		return SCBPUB(scbstate, scbinfo->next);
	}
	return NULL;
}

static struct wlc_bsscfg *
wlc_scb_next_bss(scb_module_t *scbstate, int idx)
{
	wlc_info_t *wlc = scbstate->wlc;
	wlc_bsscfg_t *next_bss = NULL;

	/* get next bss walking over hole */
	while (idx < WLC_MAXBSSCFG) {
		next_bss = WLC_BSSCFG(wlc, idx);
		if (next_bss != NULL)
			break;
		idx++;
	}
	return next_bss;
}

/** Initialize an iterator keeping memory of the next scb as it moves along the list */
void
wlc_scb_iterinit(scb_module_t *scbstate, struct scb_iter *scbiter, wlc_bsscfg_t *bsscfg)
{
	scb_bsscfg_cubby_t *scb_cfg;
	ASSERT(scbiter != NULL);

	if (bsscfg == NULL) {
		/* walk scbs of all bss */
		scbiter->all = TRUE;
		scbiter->next_bss = wlc_scb_next_bss(scbstate, 0);
		if (scbiter->next_bss == NULL) {
			/* init next scb pointer also to null */
			scbiter->next = NULL;
			return;
		}
	} else {
		/* walk scbs of specified bss */
		scbiter->all = FALSE;
		scbiter->next_bss = bsscfg;
	}

	ASSERT(scbiter->next_bss != NULL);
	scb_cfg = SCB_BSSCFG_CUBBY(scbstate, scbiter->next_bss);
	SCBSANITYCHECK(scbstate, scb_cfg->scb);

	/* Prefetch next scb, so caller can free an scb before going on to the next */
	scbiter->next = scb_cfg->scb;
}

/** move the iterator */
struct scb *
wlc_scb_iternext(scb_module_t *scbstate, struct scb_iter *scbiter)
{
	scb_bsscfg_cubby_t *scb_cfg;
	struct scb *scb;

	ASSERT(scbiter != NULL);

	while (scbiter->next_bss) {

		/* get the next scb in the current bsscfg */
		if ((scb = scbiter->next) != NULL) {
			/* get next scb of bss */
			scbiter->next = wlc_scb_next(scbstate, scb);
			return scb;
		}

		/* get the next bsscfg if we have run out of scbs in the current bsscfg */
		if (scbiter->all) {
			scbiter->next_bss =
			        wlc_scb_next_bss(scbstate, WLC_BSSCFG_IDX(scbiter->next_bss) + 1);
			if (scbiter->next_bss != NULL) {
				scb_cfg = SCB_BSSCFG_CUBBY(scbstate, scbiter->next_bss);
				scbiter->next = scb_cfg->scb;
			}
		} else {
			scbiter->next_bss = NULL;
		}
	}

	/* done with all bsscfgs and scbs */
	scbiter->next = NULL;

	return NULL;
}

/**
 * Multiple modules have the need of reserving some private data storage related to a specific
 * communication partner. During ATTACH time, this function is called multiple times, typically one
 * time per module that requires this storage. This function does not allocate memory, but
 * calculates values to be used for a future memory allocation by wlc_scb_allocmem() instead.
 *
 * Return value: negative values are errors.
 */
int
BCMATTACHFN(wlc_scb_cubby_reserve_fn)(wlc_info_t *wlc, uint size, scb_cubby_init_t fn_init,
	scb_cubby_deinit_t fn_deinit, scb_cubby_secsz_t fn_secsz, scb_cubby_dump_t fn_dump,
	void *context)
{
	scb_module_t *scbstate = wlc->scbstate;
	wlc_cubby_fn_t fn;
	int offset;

	ASSERT(scbstate->nscb == 0);

	bzero(&fn, sizeof(fn));
	fn.fn_init = (cubby_init_fn_t)fn_init;
	fn.fn_deinit = (cubby_deinit_fn_t)fn_deinit;
	fn.fn_secsz = (cubby_secsz_fn_t)fn_secsz;
	fn.fn_dump = (cubby_dump_fn_t)fn_dump;

	if ((offset = wlc_cubby_reserve(scbstate->cubby_info, size, &fn, 0, NULL, context)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_cubby_reserve failed with err %d\n",
		          wlc->pub->unit, __FUNCTION__, offset));
		return offset;
	}

	return (int)scbstate->scbtotsize + offset;
}

struct wlcband *
wlc_scbband(wlc_info_t *wlc, struct scb *scb)
{
	return wlc->bandstate[scb->bandunit];
}


#ifdef SCBFREELIST
static struct scb_info *
wlc_scbget_free(scb_module_t *scbstate)
{
	struct scb_info *ret = NULL;

	if (scbstate->free_list == NULL)
		return NULL;

	ret = SCBINFO(scbstate, scbstate->free_list);
	SCBFREESANITYCHECK(scbstate, scbstate->free_list);
	scbstate->free_list = SCBPUB(scbstate, ret->next);
#ifdef SCB_MEMDBG
	ret->next_copy = NULL;
#endif
	ret->next = NULL;

#ifdef SCB_MEMDBG
	scbstate->freelistcount = (scbstate->freelistcount > 0) ? (scbstate->freelistcount - 1) : 0;
#endif /* SCB_MEMDBG */

	return ret;
}

static void
wlc_scbadd_free(scb_module_t *scbstate, struct scb_info *ret)
{
	SCBFREESANITYCHECK(scbstate, scbstate->free_list);
	ret->next = SCBINFO(scbstate, scbstate->free_list);
	scbstate->free_list = SCBPUB(scbstate, ret);
#ifdef SCB_MEMDBG
	ret->magic = ~SCB_MAGIC;
	ret->next_copy = ret->next;
#endif

#ifdef SCB_MEMDBG
	scbstate->freelistcount += 1;
#endif /* SCB_MEMDBG */

	return;
}

static void
wlc_scbfreelist_free(scb_module_t *scbstate)
{
	struct scb_info *ret = NULL;

	ret = SCBINFO(scbstate, scbstate->free_list);
	while (ret) {
		SCBFREESANITYCHECK(scbstate, SCBPUB(scbstate, ret));
		scbstate->free_list = SCBPUB(scbstate, ret->next);
		wlc_scb_freemem(scbstate, ret);
		ret = SCBINFO(scbstate, scbstate->free_list);

#ifdef SCB_MEMDBG
		scbstate->freelistcount = (scbstate->freelistcount > 0) ?
			(scbstate->freelistcount - 1) : 0;
#endif /* SCB_MEMDBG */
	}
}

#endif /* SCBFREELIST */

static void
wlc_scb_reset(scb_module_t *scbstate, struct scb_info *scbinfo)
{
	struct scb *scb = SCBPUB(scbstate, scbinfo);
	uint len;

	len = scbstate->scbtotsize + wlc_cubby_totsize(scbstate->cubby_info);
	bzero(scb, len);

#ifdef SCB_MEMDBG
	scbinfo->magic = SCB_MAGIC;
#endif
}

/**
 * After all the modules indicated how much cubby space they need in the scb, the actual scb can be
 * allocated. This happens one time fairly late within the attach phase, but also when e.g.
 * communication with a new remote party is started.
 */
static struct scb_info *
wlc_scb_allocmem(scb_module_t *scbstate)
{
	wlc_info_t *wlc = scbstate->wlc;
	struct scb_info *scbinfo;
	struct scb *scb;
	uint len;

	/* Make sure free_mem never gets below minimum threshold due to scb_allocs */
	if (OSL_MEM_AVAIL() <= (uint)wlc->pub->tunables->min_scballoc_mem) {
		WL_ERROR(("wl%d: %s: low memory. %d bytes left.\n",
		          wlc->pub->unit, __FUNCTION__, OSL_MEM_AVAIL()));
		return NULL;
	}

	len = scbstate->scbtotsize + wlc_cubby_totsize(scbstate->cubby_info);
	scb = MALLOC(wlc->osh, len);
	if (scb == NULL) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
		          len, MALLOCED(wlc->osh)));
		return NULL;
	}

	scbinfo = SCBINFO(scbstate, scb);

#ifdef SCB_MEMDBG
	scbstate->scballoced++;
#endif /* SCB_MEMDBG */

	return scbinfo;
}

#define _wlc_internalscb_free(wlc, scb) wlc_scbfree(wlc, scb)

static struct scb *
_wlc_internalscb_alloc(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	const struct ether_addr *ea, struct wlcband *band, uint32 flags2)
{
	struct scb_info *scbinfo = NULL;
	scb_module_t *scbstate = wlc->scbstate;
	int bcmerror = 0;
	struct scb *scb;

	if (TRUE &&
#ifdef SCBFREELIST
	    /* If not found on freelist then allocate a new one */
	    (scbinfo = wlc_scbget_free(scbstate)) == NULL &&
#endif
	    (scbinfo = wlc_scb_allocmem(scbstate)) == NULL) {
		WL_ERROR(("wl%d: %s: Couldn't alloc internal scb\n",
		          wlc->pub->unit, __FUNCTION__));
		return NULL;
	}
	wlc_scb_reset(scbstate, scbinfo);

	scb = SCBPUB(scbstate, scbinfo);
	scb->bsscfg = cfg;
	scb->ea = *ea;

	/* used by hwrs and bcmc scbs */
	scb->flags2 = flags2;

	bcmerror = wlc_scbinit(wlc, cfg, band->bandunit, scb);
	if (bcmerror) {
		WL_ERROR(("wl%d: %s failed with err %d\n",
			wlc->pub->unit, __FUNCTION__, bcmerror));
		_wlc_internalscb_free(wlc, scb);
		return NULL;
	}

	wlc_scb_init_rates(wlc, cfg, band->bandunit, scb);

#ifdef TXQ_MUX
	WL_ERROR(("%s: --------------------->allocated internal SCB:%p\n", __FUNCTION__, scb));
#endif

	return scb;
}

struct scb *
wlc_bcmcscb_alloc(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct wlcband *band)
{
	return _wlc_internalscb_alloc(wlc, cfg, &ether_bcast, band, SCB2_BCMC);
}

struct scb *
wlc_hwrsscb_alloc(wlc_info_t *wlc, struct wlcband *band)
{
	const struct ether_addr ether_local = {{2, 0, 0, 0, 0, 0}};
	wlc_bsscfg_t *cfg = wlc_bsscfg_primary(wlc);

	/* TODO: pass NULL as cfg as hwrs scb doesn't belong to
	 * any bsscfg.
	 */
	return _wlc_internalscb_alloc(wlc, cfg, &ether_local, band, SCB2_HWRS);
}

static struct scb *
wlc_userscb_alloc(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	const struct ether_addr *ea, struct wlcband *band)
{
	scb_module_t *scbstate = wlc->scbstate;
	struct scb_info *scbinfo = NULL;
	struct scb *oldscb;
	int bcmerror;
	struct scb *scb;

	/* Make sure we live within our budget, and kick someone out if needed. */
	if (scbstate->nscb >= wlc->pub->tunables->maxscb ||
	    /* age scb in low memory situation as well */
	    (OSL_MEM_AVAIL() <= (uint)wlc->pub->tunables->min_scballoc_mem &&
	     /* apply scb aging in low memory situation in a limited way
	      * to prevent it ages too often.
	      */
	     scbstate->nscb >= scbstate->aging_threshold)) {
		/* free the oldest entry */
		if (!(oldscb = wlc_scbvictim(wlc))) {
			WL_ERROR(("wl%d: %s: no SCBs available to reclaim\n",
			          wlc->pub->unit, __FUNCTION__));
			return NULL;
		}
		SCB_MARK_DEAUTH(oldscb);
		if (!wlc_scbfree(wlc, oldscb)) {
			WL_ERROR(("wl%d: %s: Couldn't free a victimized scb\n",
			          wlc->pub->unit, __FUNCTION__));
			return NULL;
		}
	}
	ASSERT(scbstate->nscb < wlc->pub->tunables->maxscb);

	if (TRUE &&
#ifdef SCBFREELIST
	    /* If not found on freelist then allocate a new one */
	    (scbinfo = wlc_scbget_free(scbstate)) == NULL &&
#endif
	    (scbinfo = wlc_scb_allocmem(scbstate)) == NULL) {
		WL_ERROR(("wl%d: %s: Couldn't alloc user scb\n",
		          wlc->pub->unit, __FUNCTION__));
		return NULL;
	}
	wlc_scb_reset(scbstate, scbinfo);

	scbstate->nscb++;

	scb = SCBPUB(scbstate, scbinfo);
	scb->bsscfg = cfg;
	scb->ea = *ea;

	bcmerror = wlc_scbinit(wlc, cfg, band->bandunit, scb);
	if (bcmerror) {
		WL_ERROR(("wl%d: %s failed with err %d\n", wlc->pub->unit, __FUNCTION__, bcmerror));
		wlc_scbfree(wlc, scb);
		return NULL;
	}

	/* add it to the link list */
	wlc_scb_list_add(scbstate, cfg, scb);

	/* install it in the cache */
	wlc_scb_hash_add(scbstate, cfg, band->bandunit, scb);

	wlc_scb_init_rates(wlc, cfg, band->bandunit, scb);

#ifdef TXQ_MUX
	WL_ERROR(("%s(): --------------------->allocated user SCB:%p\n", __FUNCTION__, scb));
#endif

	return scb;
}

static void
wlc_scb_sec_set(void *ctx, void *obj, void *base)
{
	scb_module_t *scbstate = (scb_module_t *)ctx;
	struct scb *scb = (struct scb *)obj;
	struct scb_info *scbinfo = SCBINFO(scbstate, scb);

	scbinfo->secbase = base;
}

static int
wlc_scbinit(wlc_info_t *wlc, wlc_bsscfg_t *cfg, int bandunit, struct scb *scb)
{
	scb_module_t *scbstate = wlc->scbstate;
	uint i;
	cubby_sec_set_fn_t sec_fn = NULL;

	BCM_REFERENCE(cfg);
	ASSERT(scb != NULL);

	scb->used = wlc->pub->now;
	scb->bandunit = bandunit;

	for (i = 0; i < NUMPRIO; i++)
		scb->seqctl[i] = 0xFFFF;
	scb->seqctl_nonqos = 0xFFFF;

	/* no other inits are needed for internal scb */
	if (SCB_INTERNAL(scb)) {
#ifdef INT_SCB_OPT
		return BCME_OK;
#endif
	}

	if (!SCB_HWRS(scb))
		sec_fn = wlc_scb_sec_set;

	return wlc_cubby_init(scbstate->cubby_info, scb, sec_fn, scbstate);
}

static void
wlc_scb_freemem(scb_module_t *scbstate, struct scb_info *scbinfo)
{
	wlc_info_t *wlc = scbstate->wlc;
	struct scb *scb = SCBPUB(scbstate, scbinfo);
	uint len;

	BCM_REFERENCE(wlc);

	if (scbinfo == NULL)
		return;

	len = scbstate->scbtotsize + wlc_cubby_totsize(scbstate->cubby_info);
	MFREE(wlc->osh, scb, len);

#ifdef SCB_MEMDBG
	scbstate->scbfreed++;
#endif /* SCB_MEMDBG */
}

bool
wlc_scbfree(wlc_info_t *wlc, struct scb *scbd)
{
	scb_module_t *scbstate = wlc->scbstate;
	struct scb_info *remove = SCBINFO(scbstate, scbd);

	if (scbd->permanent)
		return FALSE;

	/* Return if SCB is already being deleted else mark it */
	if (scbd->mark & SCB_DEL_IN_PROG)
		return FALSE;

	if (SCB_INTERNAL(scbd)) {
		goto free;
	}

	wlc_scb_resetstate(wlc, scbd);

	/* delete it from the hash */
	wlc_scb_hash_del(scbstate, SCB_BSSCFG(scbd), scbd);

	/* delete it from the link list */
	wlc_scb_list_del(scbstate, SCB_BSSCFG(scbd), scbd);

	/* update total allocated scb number */
	scbstate->nscb--;

free:
	scbd->mark |= SCB_DEL_IN_PROG;

	wlc_scbdeinit(wlc, scbd);

#ifdef SCBFREELIST
	wlc_scbadd_free(scbstate, remove);
#else
	/* free scb memory */
	wlc_scb_freemem(scbstate, remove);
#endif

	return TRUE;
}

static void *
wlc_scb_sec_get(void *ctx, void *obj)
{
	scb_module_t *scbstate = (scb_module_t *)ctx;
	struct scb *scb = (struct scb *)obj;
	struct scb_info *scbinfo = SCBINFO(scbstate, scb);

	return scbinfo->secbase;
}

static void
wlc_scbdeinit(wlc_info_t *wlc, struct scb *scbd)
{
	scb_module_t *scbstate = wlc->scbstate;
	cubby_sec_get_fn_t sec_fn = NULL;

	/* no other cleanups are needed for internal scb */
	if (SCB_INTERNAL(scbd)) {
#ifdef INT_SCB_OPT
		return;
#endif
	}

	if (!SCB_HWRS(scbd))
		sec_fn = wlc_scb_sec_get;

	wlc_cubby_deinit(scbstate->cubby_info, scbd, sec_fn, scbstate);
}

static void
wlc_scb_list_add(scb_module_t *scbstate, wlc_bsscfg_t *bsscfg, struct scb *scb)
{
	struct scb_info *scbinfo = SCBINFO(scbstate, scb);
	scb_bsscfg_cubby_t *scb_cfg;

	ASSERT(bsscfg != NULL);

	scb_cfg = SCB_BSSCFG_CUBBY(scbstate, bsscfg);

	SCBSANITYCHECK(scbstate, scb_cfg->scb);

	/* update scb link list */
	scbinfo->next = SCBINFO(scbstate, scb_cfg->scb);
#ifdef SCB_MEMDBG
	scbinfo->next_copy = scbinfo->next;
#endif
	scb_cfg->scb = scb;
}

static void
wlc_scb_list_del(scb_module_t *scbstate, wlc_bsscfg_t *bsscfg, struct scb *scbd)
{
	scb_bsscfg_cubby_t *scb_cfg;
	struct scb_info *scbinfo;
	struct scb_info *remove = SCBINFO(scbstate, scbd);

	ASSERT(bsscfg != NULL);

	/* delete it from the link list */

	scb_cfg = SCB_BSSCFG_CUBBY(scbstate, bsscfg);
	scbinfo = SCBINFO(scbstate, scb_cfg->scb);
	if (scbinfo == remove) {
		scb_cfg->scb = wlc_scb_next(scbstate, scbd);
		return;
	}

	while (scbinfo) {
		SCBSANITYCHECK(scbstate, SCBPUB(scbstate, scbinfo));
		if (scbinfo->next == remove) {
			scbinfo->next = remove->next;
#ifdef SCB_MEMDBG
			scbinfo->next_copy = scbinfo->next;
#endif
			break;
		}
		scbinfo = scbinfo->next;
	}
	ASSERT(scbinfo != NULL);
}

/** free all scbs of a bsscfg */
static void
wlc_scb_bsscfg_scbclear(struct wlc_info *wlc, wlc_bsscfg_t *bsscfg, bool perm)
{
	struct scb_iter scbiter;
	struct scb *scb;

	if (wlc->scbstate == NULL)
		return;

	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, bsscfg, scb) {
		if (scb->permanent) {
			if (!perm)
				continue;
			scb->permanent = FALSE;
		}
		wlc_scbfree(wlc, scb);
	}
}

#define SCB_SHORT_TIMEOUT	  60	/**< # seconds of idle time after which we will reclaim an
					 * authenticated SCB if we would otherwise fail
					 * an SCB allocation.
					 */

static struct scb *
wlc_scbvictim(wlc_info_t *wlc)
{
	uint oldest;
	struct scb *scb;
	struct scb *oldscb;
	uint now, age;
	struct scb_iter scbiter;
#if defined(WLMSG_ASSOC)
	char eabuf[ETHER_ADDR_STR_LEN];
#endif 
	wlc_bsscfg_t *bsscfg = NULL;

#ifdef AP
	/* search for an unauthenticated scb */
	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		if (!scb->permanent && (scb->state == 0))
			return scb;
	}
#endif /* AP */

	/* free the oldest scb */
	now = wlc->pub->now;
	oldest = 0;
	oldscb = NULL;
	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		bsscfg = SCB_BSSCFG(scb);
		ASSERT(bsscfg != NULL);
		if (BSSCFG_STA(bsscfg) && bsscfg->BSS && SCB_ASSOCIATED(scb))
			continue;
		if (!scb->permanent && ((age = (now - scb->used)) >= oldest)) {
			oldest = age;
			oldscb = scb;
		}
	}
	/* handle extreme case(s): all are permanent ... or there are no scb's at all */
	if (oldscb == NULL)
		return NULL;

#ifdef AP
	bsscfg = SCB_BSSCFG(oldscb);

	if (BSSCFG_AP(bsscfg)) {
		/* if the oldest authenticated SCB has only been idle a short time then
		 * it is not a candidate to reclaim
		 */
		if (oldest < SCB_SHORT_TIMEOUT)
			return NULL;
	}
#endif /* AP */

	WL_ASSOC(("wl%d: %s: relcaim scb %s, idle %d sec\n",  wlc->pub->unit, __FUNCTION__,
	          bcm_ether_ntoa(&oldscb->ea, eabuf), oldest));

	return oldscb;
}

/* Only notify registered clients of the following states' change. */
static uint8 scb_state_change_notif_mask = AUTHENTICATED | ASSOCIATED | AUTHORIZED;

/** "|" operation. */
void
wlc_scb_setstatebit(wlc_info_t *wlc, struct scb *scb, uint8 state)
{
	scb_module_t *scbstate;
	uint8	oldstate;

	WL_NONE(("set state %x\n", state));
	ASSERT(scb != NULL);

	scbstate = wlc->scbstate;
	oldstate = scb->state;

	if (state & AUTHENTICATED)
	{
		scb->state &= ~PENDING_AUTH;
	}
	if (state & ASSOCIATED)
	{
		ASSERT((scb->state | state) & AUTHENTICATED);
		scb->state &= ~PENDING_ASSOC;
	}

	scb->state |= state;
	WL_NONE(("wlc_scb : state = %x\n", scb->state));

	if ((BSSCFG_AP(SCB_BSSCFG(scb)) && (state & scb_state_change_notif_mask) != 0) ||
	    (oldstate & scb_state_change_notif_mask) != (state & scb_state_change_notif_mask))
	{
		scb_state_upd_data_t data;
		data.scb = scb;
		data.oldstate = oldstate;
		bcm_notif_signal(scbstate->scb_state_notif_hdl, &data);
	}
}

/** "& ~" operation */
void
wlc_scb_clearstatebit(wlc_info_t *wlc, struct scb *scb, uint8 state)
{
	scb_module_t *scbstate;
	uint8	oldstate;

	ASSERT(scb != NULL);
	WL_NONE(("clear state %x\n", state));

	scbstate = wlc->scbstate;
	oldstate = scb->state;

	scb->state &= ~state;
	WL_NONE(("wlc_scb : state = %x\n", scb->state));

	if ((oldstate & scb_state_change_notif_mask) != (scb->state & scb_state_change_notif_mask))
	{
		scb_state_upd_data_t data;
		data.scb = scb;
		data.oldstate = oldstate;
		bcm_notif_signal(scbstate->scb_state_notif_hdl, &data);
	}
}


/** reset all state. */
void
wlc_scb_resetstate(wlc_info_t *wlc, struct scb *scb)
{
	WL_NONE(("reset state\n"));

	wlc_scb_clearstatebit(wlc, scb, ~0);
}

/** set/change rateset and init/reset ratesel */
static void
wlc_scb_init_rates(wlc_info_t *wlc, wlc_bsscfg_t *cfg, int bandunit, struct scb *scb)
{
	wlcband_t *band = wlc->bandstate[bandunit];
	wlc_rateset_t *rs;

	/* use current, target, or per-band default rateset? */
	if (wlc->pub->up &&
	    wlc_valid_chanspec(wlc->cmi, cfg->target_bss->chanspec))
		if (cfg->associated)
			rs = &cfg->current_bss->rateset;
		else
			rs = &cfg->target_bss->rateset;
	else
		rs = &band->defrateset;

	/*
	 * Initialize the per-scb rateset:
	 * - if we are AP, start with only the basic subset of the
	 *	network rates.  It will be updated when receive the next
	 *	probe request or association request.
	 * - if we are IBSS and gmode, special case:
	 *	start with B-only subset of network rates and probe for ofdm rates
	 * - else start with the network rates.
	 *	It will be updated on join attempts.
	 */
	if (BSS_P2P_ENAB(wlc, cfg)) {
		wlc_rateset_filter(rs /* src */, &scb->rateset /* dst */,
		                   FALSE, WLC_RATES_OFDM, RATE_MASK,
		                   wlc_get_mcsallow(wlc, cfg));
	}
	else if (BSSCFG_AP(cfg)) {
		uint8 mcsallow = BSS_N_ENAB(wlc, cfg) ? WLC_MCS_ALLOW : 0;
		wlc_rateset_filter(rs /* src */, &scb->rateset /* dst */,
		                   TRUE, WLC_RATES_CCK_OFDM, RATE_MASK,
		                   mcsallow);
	}
	else if (!cfg->BSS && band->gmode) {
		wlc_rateset_filter(rs /* src */, &scb->rateset /* dst */,
				FALSE, WLC_RATES_CCK, RATE_MASK, 0);
		/* if resulting set is empty, then take all network rates instead */
		if (scb->rateset.count == 0) {
			wlc_rateset_filter(rs /* src */, &scb->rateset /* dst */,
					FALSE, WLC_RATES_CCK_OFDM, RATE_MASK, 0);
		}
	}
	else {
		wlc_rateset_filter(rs /* src */, &scb->rateset /* dst */,
				FALSE, WLC_RATES_CCK_OFDM, RATE_MASK, 0);
	}

	if (!SCB_INTERNAL(scb)) {
		wlc_scb_ratesel_init(wlc, scb);
#ifdef STA
		/* send ofdm rate probe */
		if (BSSCFG_STA(cfg) && !cfg->BSS && band->gmode &&
		    wlc->pub->up)
			wlc_rateprobe(wlc, cfg, &scb->ea, WLC_RATEPROBE_RSPEC);
#endif /* STA */
	}
}

static void
wlc_scb_bsscfg_reinit(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	uint prev_count;
	const wlc_rateset_t *rs;
	wlcband_t *band;
	struct scb *scb;
	struct scb_iter scbiter;
	bool cck_only;
	bool reinit_forced;

	WL_INFORM(("wl%d: %s: bandunit 0x%x phy_type 0x%x gmode 0x%x\n", wlc->pub->unit,
		__FUNCTION__, wlc->band->bandunit, wlc->band->phytype, wlc->band->gmode));

	/* sanitize any existing scb rates against the current hardware rates */
	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, bsscfg, scb) {
		prev_count = scb->rateset.count;
		/* Keep only CCK if gmode == GMODE_LEGACY_B */
		band = wlc_scbband(wlc, scb);
		if (BAND_2G(band->bandtype) && (band->gmode == GMODE_LEGACY_B)) {
			rs = &cck_rates;
			cck_only = TRUE;
		} else {
			rs = &band->hw_rateset;
			cck_only = FALSE;
		}
		if (!wlc_rate_hwrs_filter_sort_validate(&scb->rateset /* [in+out] */, rs /* [in] */,
			FALSE, wlc->stf->txstreams)) {
			/* continue with default rateset.
			 * since scb rateset does not carry basic rate indication,
			 * clear basic rate bit.
			 */
			WL_RATE(("wl%d: %s: invalid rateset in scb 0x%p bandunit 0x%x "
				"phy_type 0x%x gmode 0x%x\n", wlc->pub->unit, __FUNCTION__,
				scb, band->bandunit, band->phytype, band->gmode));

			wlc_rateset_default(&scb->rateset, &band->hw_rateset,
			                    band->phytype, band->bandtype, cck_only, RATE_MASK,
			                    wlc_get_mcsallow(wlc, scb->bsscfg),
			                    CHSPEC_WLC_BW(scb->bsscfg->current_bss->chanspec),
			                    wlc->stf->txstreams);
			reinit_forced = TRUE;
		}
		else
			reinit_forced = FALSE;

		/* if the count of rates is different, then the rate state
		 * needs to be reinitialized
		 */
		if (reinit_forced || (scb->rateset.count != prev_count))
			wlc_scb_ratesel_init(wlc, scb);

		WL_RATE(("wl%d: %s: bandunit 0x%x, phy_type 0x%x gmode 0x%x. final rateset is\n",
			wlc->pub->unit, __FUNCTION__,
			band->bandunit, band->phytype, band->gmode));
	}
}

void
wlc_scb_reinit(wlc_info_t *wlc)
{
	int32 idx;
	wlc_bsscfg_t *bsscfg;

	FOREACH_BSS(wlc, idx, bsscfg) {
		wlc_scb_bsscfg_reinit(wlc, bsscfg);
	}
}

static INLINE struct scb* BCMFASTPATH
_wlc_scbfind(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, const struct ether_addr *ea, int bandunit)
{
	scb_module_t *scbstate = wlc->scbstate;
	int indx;
	struct scb_info *scbinfo;
	scb_bsscfg_cubby_t *scb_cfg;

	ASSERT(bsscfg != NULL);

	/* All callers of wlc_scbfind() should first be checking to see
	 * if the SCB they're looking for is a BC/MC address.  Because we're
	 * using per bsscfg BCMC SCBs, we can't "find" BCMC SCBs without
	 * knowing which bsscfg.
	 */
	ASSERT(ea && !ETHER_ISMULTI(ea));


	/* search for the scb which corresponds to the remote station ea */
	scb_cfg = SCB_BSSCFG_CUBBY(scbstate, bsscfg);
	if (scb_cfg) {
		indx = SCBHASHINDEX(scb_cfg->nscbhash, ea->octet);
		scbinfo =
			SCBINFO(scbstate, scb_cfg->scbhash[bandunit][indx]);
		for (; scbinfo; scbinfo = scbinfo->hashnext) {
			SCBSANITYCHECK(scbstate, SCBPUB(scbstate, scbinfo));
			if (eacmp((const char*)ea,
			          (const char*)&(SCBPUB(scbstate, scbinfo)->ea)) == 0)
				break;
		}

		return SCBPUB(scbstate, scbinfo);
	}
	return (NULL);
}

/** Find station control block corresponding to the remote id */
struct scb * BCMFASTPATH
wlc_scbfind(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, const struct ether_addr *ea)
{
	struct scb *scb = NULL;

	scb = _wlc_scbfind(wlc, bsscfg, ea, wlc->band->bandunit);

#if defined(WLMCHAN)
/* current band could be different, so search again for all scb's */
	if (!scb && MCHAN_ACTIVE(wlc->pub) && NBANDS(wlc) > 1)
		scb = wlc_scbfindband(wlc, bsscfg, ea, OTHERBANDUNIT(wlc));
#endif 
	return scb;
}

/**
 * Lookup station control block corresponding to the remote id.
 * If not found, create a new entry.
 */
static INLINE struct scb *
_wlc_scblookup(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, const struct ether_addr *ea, int bandunit)
{
	struct scb *scb;
	struct wlcband *band;
#if defined(WLMSG_ASSOC)
	char sa[ETHER_ADDR_STR_LEN];
#endif

	/* Don't allocate/find a BC/MC SCB this way. */
	ASSERT(!ETHER_ISMULTI(ea));
	if (ETHER_ISMULTI(ea))
		return NULL;

	/* apply mac filter */
	switch (wlc_macfltr_addr_match(wlc->macfltr, bsscfg, ea)) {
	case WLC_MACFLTR_ADDR_DENY:
		WL_ASSOC(("wl%d.%d mac restrict: Denying %s\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
		          bcm_ether_ntoa(ea, sa)));
		return NULL;
	case WLC_MACFLTR_ADDR_NOT_ALLOW:
		WL_ASSOC(("wl%d.%d mac restrict: Not allowing %s\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
		          bcm_ether_ntoa(ea, sa)));
		return NULL;
	}

	if ((scb = _wlc_scbfind(wlc, bsscfg, ea, bandunit)))
		return (scb);

	/* no scb match, allocate one for the desired bandunit */
	band = wlc->bandstate[bandunit];
	return wlc_userscb_alloc(wlc, bsscfg, ea, band);
}

struct scb *
wlc_scblookup(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, const struct ether_addr *ea)
{
	return (_wlc_scblookup(wlc, bsscfg, ea, wlc->band->bandunit));
}

struct scb *
wlc_scblookupband(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, const struct ether_addr *ea, int bandunit)
{
	/* assert that the band is the current band, or we are dual band and it is the other band */
	ASSERT((bandunit == (int)wlc->band->bandunit) ||
	       (NBANDS(wlc) > 1 && bandunit == (int)OTHERBANDUNIT(wlc)));

	return (_wlc_scblookup(wlc, bsscfg, ea, bandunit));
}

/** Get scb from band */
struct scb * BCMFASTPATH
wlc_scbfindband(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, const struct ether_addr *ea, int bandunit)
{
	/* assert that the band is the current band, or we are dual band and it is the other band */
	ASSERT((bandunit == (int)wlc->band->bandunit) ||
	       (NBANDS(wlc) > 1 && bandunit == (int)OTHERBANDUNIT(wlc)));

	return (_wlc_scbfind(wlc, bsscfg, ea, bandunit));
}

void
wlc_scb_switch_band(wlc_info_t *wlc, struct scb *scb, int new_bandunit,
	wlc_bsscfg_t *bsscfg)
{
	scb_module_t *scbstate = wlc->scbstate;

	/* first, del scb from hash table in old band */
	wlc_scb_hash_del(scbstate, bsscfg, scb);
	/* next add scb to hash table in new band */
	wlc_scb_hash_add(scbstate, bsscfg, new_bandunit, scb);
	return;
}

/**
 * Move the scb's band info.
 * Parameter description:
 *
 * wlc - global wlc_info structure
 * bsscfg - the bsscfg that is about to move to a new chanspec
 * chanspec - the new chanspec the bsscfg is moving to
 *
 */
void
wlc_scb_update_band_for_cfg(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, chanspec_t chanspec)
{
	struct scb_iter scbiter;
	struct scb *scb, *stale_scb;
	int bandunit;
	bool reinit = FALSE;

	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, bsscfg, scb) {
		if (SCB_ASSOCIATED(scb)) {
			bandunit = CHSPEC_WLCBANDUNIT(chanspec);
			if (scb->bandunit != (uint)bandunit) {
				/* We're about to move our scb to the new band.
				 * Check to make sure there isn't an scb entry for us there.
				 * If there is one for us, delete it first.
				 */
				if ((stale_scb = _wlc_scbfind(wlc, bsscfg,
				                      &bsscfg->BSSID, bandunit)) &&
				    (stale_scb->permanent == FALSE)) {
					WL_ASSOC(("wl%d.%d: %s: found stale scb %p on %s band, "
					          "remove it\n",
					          wlc->pub->unit, bsscfg->_idx, __FUNCTION__,
					          stale_scb,
					          (bandunit == BAND_5G_INDEX) ? "5G" : "2G"));
					/* mark the scb for removal */
					stale_scb->mark |= SCB_MARK_TO_REM;
				}
				/* Now perform the move of our scb to the new band */
				wlc_scb_switch_band(wlc, scb, bandunit, bsscfg);
				reinit = TRUE;
			}
		}
	}
	/* remove stale scb's marked for removal */
	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, bsscfg, scb) {
		if (scb->mark & SCB_MARK_TO_REM) {
			WL_ASSOC(("remove stale scb %p\n", scb));
			scb->mark &= ~SCB_MARK_TO_REM;
			wlc_scbfree(wlc, scb);
		}
	}

	if (reinit) {
		wlc_scb_reinit(wlc);
	}
}

struct scb *
wlc_scbibssfindband(wlc_info_t *wlc, const struct ether_addr *ea, int bandunit,
                    wlc_bsscfg_t **bsscfg)
{
	int idx;
	wlc_bsscfg_t *cfg;
	struct scb *scb = NULL;

	/* assert that the band is the current band, or we are dual band
	 * and it is the other band.
	 */
	ASSERT((bandunit == (int)wlc->band->bandunit) ||
	       (NBANDS(wlc) > 1 && bandunit == (int)OTHERBANDUNIT(wlc)));

	FOREACH_IBSS(wlc, idx, cfg) {
		/* Find the bsscfg and scb matching specified peer mac */
		scb = _wlc_scbfind(wlc, cfg, ea, bandunit);
		if (scb != NULL) {
			*bsscfg = cfg;
			break;
		}
	}

	return scb;
}

struct scb *
wlc_scbapfind(wlc_info_t *wlc, const struct ether_addr *ea, wlc_bsscfg_t **bsscfg)
{
	int idx;
	wlc_bsscfg_t *cfg;
	struct scb *scb = NULL;

	*bsscfg = NULL;

	FOREACH_UP_AP(wlc, idx, cfg) {
		/* Find the bsscfg and scb matching specified peer mac */
		scb = wlc_scbfind(wlc, cfg, ea);
		if (scb != NULL) {
			*bsscfg = cfg;
			break;
		}
	}

	return scb;
}

struct scb * BCMFASTPATH
wlc_scbbssfindband(wlc_info_t *wlc, const struct ether_addr *hwaddr,
                   const struct ether_addr *ea, int bandunit, wlc_bsscfg_t **bsscfg)
{
	int idx;
	wlc_bsscfg_t *cfg;
	struct scb *scb = NULL;

	/* assert that the band is the current band, or we are dual band
	 * and it is the other band.
	 */
	ASSERT((bandunit == (int)wlc->band->bandunit) ||
	       (NBANDS(wlc) > 1 && bandunit == (int)OTHERBANDUNIT(wlc)));

	*bsscfg = NULL;

	FOREACH_BSS(wlc, idx, cfg) {
		/* Find the bsscfg and scb matching specified hwaddr and peer mac */
		if (eacmp(cfg->cur_etheraddr.octet, hwaddr->octet) == 0) {
			scb = _wlc_scbfind(wlc, cfg, ea, bandunit);
			if (scb != NULL) {
				*bsscfg = cfg;
				break;
			}
		}
	}

	return scb;
}

static void
wlc_scb_hash_add(scb_module_t *scbstate, wlc_bsscfg_t *bsscfg, int bandunit, struct scb *scb)
{
	scb_bsscfg_cubby_t *scb_cfg;
	int indx;
	struct scb_info *scbinfo;

	ASSERT(bsscfg != NULL);

	scb->bandunit = bandunit;

	scb_cfg = SCB_BSSCFG_CUBBY(scbstate, bsscfg);
	indx = SCBHASHINDEX(scb_cfg->nscbhash, scb->ea.octet);
	scbinfo =
	           SCBINFO(scbstate, scb_cfg->scbhash[bandunit][indx]);

	SCBINFO(scbstate, scb)->hashnext = scbinfo;
#ifdef SCB_MEMDBG
	SCBINFO(scbstate, scb)->hashnext_copy = SCBINFO(scbstate, scb)->hashnext;
#endif

	scb_cfg->scbhash[bandunit][indx] = scb;
}

static void
wlc_scb_hash_del(scb_module_t *scbstate, wlc_bsscfg_t *bsscfg, struct scb *scbd)
{
	scb_bsscfg_cubby_t *scb_cfg;
	int indx;
	struct scb_info *scbinfo;
	struct scb_info *remove = SCBINFO(scbstate, scbd);
	int bandunit = scbd->bandunit;

	ASSERT(bsscfg != NULL);

	scb_cfg = SCB_BSSCFG_CUBBY(scbstate, bsscfg);
	indx = SCBHASHINDEX(scb_cfg->nscbhash, scbd->ea.octet);

	/* delete it from the hash */
	scbinfo =
	           SCBINFO(scbstate, scb_cfg->scbhash[bandunit][indx]);
	ASSERT(scbinfo != NULL);
	SCBSANITYCHECK(scbstate, SCBPUB(scbstate, scbinfo));
	/* special case for the first */
	if (scbinfo == remove) {
		if (scbinfo->hashnext)
			SCBSANITYCHECK(scbstate, SCBPUB(scbstate, scbinfo->hashnext));
		scb_cfg->scbhash[bandunit][indx] =
		        SCBPUB(scbstate, scbinfo->hashnext);
	} else {
		for (; scbinfo; scbinfo = scbinfo->hashnext) {
			SCBSANITYCHECK(scbstate, SCBPUB(scbstate, scbinfo->hashnext));
			if (scbinfo->hashnext == remove) {
				scbinfo->hashnext = remove->hashnext;
#ifdef SCB_MEMDBG
				scbinfo->hashnext_copy = scbinfo->hashnext;
#endif
				break;
			}
		}
		ASSERT(scbinfo != NULL);
	}
}

void
wlc_scb_sortrates(wlc_info_t *wlc, struct scb *scb)
{
	wlcband_t *band = wlc_scbband(wlc, scb);

	wlc_rate_hwrs_filter_sort_validate(&scb->rateset /* [in+out] */,
		&band->hw_rateset /* [in] */, FALSE,
		wlc->stf->txstreams);
}

void
BCMINITFN(wlc_scblist_validaterates)(wlc_info_t *wlc)
{
	struct scb *scb;
	struct scb_iter scbiter;

	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		wlc_scb_sortrates(wlc, scb);
		if (scb->rateset.count == 0)
			wlc_scbfree(wlc, scb);
	}
}



int
wlc_scb_state_upd_register(wlc_info_t *wlc, scb_state_upd_cb_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->scbstate->scb_state_notif_hdl;

	return bcm_notif_add_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

int
wlc_scb_state_upd_unregister(wlc_info_t *wlc, scb_state_upd_cb_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->scbstate->scb_state_notif_hdl;

	return bcm_notif_remove_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

void
wlc_scbfind_delete(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct ether_addr *ea)
{
	int i;
#if defined(WLMSG_ASSOC)
	char eabuf[ETHER_ADDR_STR_LEN];
#endif 
	struct scb *scb;

	for (i = 0; i < (int)NBANDS(wlc); i++) {
		/* Use band 1 for single band 11a */
		if (IS_SINGLEBAND_5G(wlc->deviceid))
			i = BAND_5G_INDEX;

		scb = wlc_scbfindband(wlc, bsscfg, ea, i);
		if (scb) {
			WL_ASSOC(("wl%d: %s: scb for the STA-%s"
				" already exists\n", wlc->pub->unit, __FUNCTION__,
				bcm_ether_ntoa(ea, eabuf)));
			wlc_scbfree(wlc, scb);
		}
	}
}

#if defined(STA) && defined(DBG_BCN_LOSS)
int
wlc_get_bcn_dbg_info(wlc_bsscfg_t *cfg, struct ether_addr *addr,
	struct wlc_scb_dbg_bcn *dbg_bcn)
{
	wlc_info_t *wlc = cfg->wlc;

	if (cfg->BSS) {
		struct scb *scb = wlc_scbfindband(wlc, cfg, addr,
			CHSPEC_WLCBANDUNIT(cfg->current_bss->chanspec));
		if (scb) {
			memcpy(dbg_bcn, &(scb->dbg_bcn), sizeof(struct wlc_scb_dbg_bcn));
			return BCME_OK;
		}
	}
	return BCME_ERROR;
}
#endif /* defined(STA) && defined(DBG_BCN_LOSS) */

/**
 * These function allocates/frees a secondary cubby in the secondary cubby area.
 */
void *
wlc_scb_sec_cubby_alloc(wlc_info_t *wlc, struct scb *scb, uint secsz)
{
	scb_module_t *scbstate = wlc->scbstate;

	return wlc_cubby_sec_alloc(scbstate->cubby_info, scb, secsz);
}

void
wlc_scb_sec_cubby_free(wlc_info_t *wlc, struct scb *scb, void *secptr)
{
	scb_module_t *scbstate = wlc->scbstate;

	wlc_cubby_sec_free(scbstate->cubby_info, scb, secptr);
}
