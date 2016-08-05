/*
 * mac filter module source file
 * Broadcom 802.11 Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_key.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_macfltr.h>
#include <wlc_addrmatch.h>
#include <wlc_stamon.h>
#ifdef PSTA
#include <wlc_psta.h>
#endif
/* ioctl table */
static const wlc_ioctl_cmd_t wlc_macfltr_ioctls[] = {
	{WLC_GET_MACLIST, 0, 0},
	{WLC_SET_MACLIST, 0, sizeof(uint)},
	{WLC_GET_MACMODE, 0, sizeof(uint)},
	{WLC_SET_MACMODE, 0, sizeof(uint)}
};

/* module struct */
struct wlc_macfltr_info {
	wlc_info_t *wlc;
	int cfgh;
};

/* cubby struct and access macros */
typedef struct {
	/* MAC filter */
	int macmode;			/* allow/deny stations on maclist array */
	uint nmac;			/* # of entries on maclist array */
	struct ether_addr *maclist;	/* list of source MAC addrs to match */
	int max_maclist;        /* max entry of maclist */
} bss_macfltr_info_t;
#define BSS_MACFLTR_INFO(mfi, cfg) ((bss_macfltr_info_t *)BSSCFG_CUBBY(cfg, (mfi)->cfgh))

/* forward declaration */
/* ioctl */
static int wlc_macfltr_doioctl(void *ctx, int cmd, void *arg, int len, struct wlc_if *wlcif);
/* dump */

/* cubby */
static void wlc_macfltr_bss_deinit(void *ctx, wlc_bsscfg_t *cfg);
static int wlc_macfltr_bss_init(void *ctx, wlc_bsscfg_t *cfg);
static int wlc_macfltr_acksupr_enable(wlc_bsscfg_t *cfg, bool enable);

#define wlc_macfltr_bss_dump NULL
static int wlc_macfltr_acksupr_doiovar(
		    void                *hdl,
		    const bcm_iovar_t   *vi,
		    uint32              actionid,
		    const char          *name,
		    void                *p,
		    uint                plen,
		    void                *a,
		    int                 alen,
		    int                 vsize,
		    struct wlc_if       *wlcif);


/* IOVar table */
enum {
	IOV_ACKSUPR_MACFLTR = 0,	/* enable / disable acksupr. */
	IOV_LAST
};

static const bcm_iovar_t acksupr_iovars[] = {
	{"acksupr_mac_filter", IOV_ACKSUPR_MACFLTR,
	0, IOVT_BOOL, 0
	},
	{NULL, 0, 0, 0, 0 }
};


/* module entries */
wlc_macfltr_info_t *
BCMATTACHFN(wlc_macfltr_attach)(wlc_info_t *wlc)
{
	int err = 0;
	wlc_macfltr_info_t *mfi;

	ASSERT(OFFSETOF(wlc_macfltr_info_t, wlc) == 0);

	if ((mfi = MALLOCZ(wlc->osh, sizeof(wlc_macfltr_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	mfi->wlc = wlc;

	/* reserve cubby in the bsscfg container for per-bsscfg private data */
	if ((mfi->cfgh = wlc_bsscfg_cubby_reserve(wlc, sizeof(bss_macfltr_info_t),
	                wlc_macfltr_bss_init, wlc_macfltr_bss_deinit, wlc_macfltr_bss_dump,
	                (void *)mfi)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	err = wlc_module_register(wlc->pub, acksupr_iovars, "acksupr_mac_filter", wlc,
		wlc_macfltr_acksupr_doiovar, NULL, NULL, NULL);
	if (err) {
		WL_ERROR(("%s: wlc_module_register failed\n", __FUNCTION__));
		goto fail;
	}

	if (wlc_module_add_ioctl_fn(wlc->pub, mfi, wlc_macfltr_doioctl,
	                ARRAYSIZE(wlc_macfltr_ioctls), wlc_macfltr_ioctls) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_add_ioctl_fn() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}


	return mfi;

	/* error handling */
fail:
	wlc_macfltr_detach(mfi);
	return NULL;
}

void
BCMATTACHFN(wlc_macfltr_detach)(wlc_macfltr_info_t *mfi)
{
	wlc_info_t *wlc;

	if (mfi == NULL)
		return;

	wlc = mfi->wlc;

	wlc_module_unregister(wlc->pub, "acksupr_mac_filter", wlc);

	wlc_module_remove_ioctl_fn(wlc->pub, mfi);

	MFREE(wlc->osh, mfi, sizeof(wlc_macfltr_info_t));
}

static int
wlc_macfltr_acksupr_doiovar(
	void				*hdl,
		const bcm_iovar_t	*vi,
		uint32				actionid,
		const char			*name,
		void				*p,
		uint				plen,
		void				*a,
		int 				alen,
		int 				vsize,
		struct wlc_if		*wlcif)
{
	wlc_info_t* wlc = (wlc_info_t*) hdl;
	wlc_bsscfg_t *cfg;
	bss_macfltr_info_t *bfi;
	int32 *val = (int32 *)a;
	int err = BCME_OK;

	/* update bsscfg w/provided interface context */
	cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(cfg != NULL);

	switch (actionid) {
		case IOV_GVAL(IOV_ACKSUPR_MACFLTR):
			if (cfg->acksupr_mac_filter)
				*val = 1;
			else
				*val = 0;
			break;
		case IOV_SVAL(IOV_ACKSUPR_MACFLTR):
			bfi = BSS_MACFLTR_INFO(wlc->macfltr, cfg);
			ASSERT(bfi != NULL);

			if (bfi->macmode != WLC_MACMODE_ALLOW)
				err = BCME_UNSUPPORTED;
			else {
				if (*val != (cfg->acksupr_mac_filter?1:0)) {
					err = wlc_macfltr_acksupr_enable(cfg, *val?TRUE:FALSE);
				}
			}
			break;
		default:
			err = BCME_UNSUPPORTED;
			break;
	}

	return err;
}

static bool
wlc_macfltr_is_stamon_resrv_slot(wlc_info_t *wlc, int amt_idx)
{
	if (wlc->wsec_keys[amt_idx + WSEC_MAX_DEFAULT_KEYS] == (wsec_key_t*)STAMON_WSEC_KEYS_STUB)
		return TRUE;

	return FALSE;
}

bool
wlc_macfltr_acksupr_duplicate(wlc_info_t *wlc, struct ether_addr *ea)
{
	int idx, i;
	wlc_bsscfg_t *cfg;
	bss_macfltr_info_t *bfi;
	if (!WLC_ACKSUPR(wlc))
		return FALSE;

	FOREACH_AP(wlc, idx, cfg) {
		if (BSSCFG_ACKSUPR(cfg)) {
			bfi = BSS_MACFLTR_INFO(wlc->macfltr, cfg);
			ASSERT(bfi != NULL);

			for (i = 0; i < bfi->max_maclist; i++) {
				if (bcmp(ea, &bfi->maclist[i], ETHER_ADDR_LEN) == 0) {
					WL_ERROR(("wl%d: duplicated entry configured in "
						"stamon and macfilter for %s\n",
						wlc->pub->unit, bcm_ether_ntoa(ea, eabuf)));
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

int
wlc_macfltr_find_and_add_addrmatch(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	struct ether_addr *addr, uint16 attr)
{
	int i, emp_idx = BCME_NOTFOUND, amt_idx = BCME_NOTFOUND;
	int max_maclist, max;
	struct ether_addr tmp_ea;
	uint16 tmp_attr;
	bss_macfltr_info_t *bfi = BSS_MACFLTR_INFO(wlc->macfltr, cfg);

	ASSERT(bfi != NULL);

	max_maclist = bfi->max_maclist;

	if (D11REV_GE(wlc->pub->corerev, 40))
		max = MIN(max_maclist, AMT_MAX_MACLIST_NUM);
	else
		max = MIN(max_maclist, RCMTA_SIZE);

	emp_idx = BCME_NOTFOUND;
	for (i = 0; i < max; i++) {
		/* skip reserved slot by sta_monitor */
		if (wlc->wsec_keys[i + WSEC_MAX_DEFAULT_KEYS] ==
			(wsec_key_t*)STAMON_WSEC_KEYS_STUB)
			continue;

		wlc_get_addrmatch(wlc, i, &tmp_ea, &tmp_attr);

		/* skip empty slot, and record an empty one */
		if (tmp_attr == 0) {
			if (emp_idx == BCME_NOTFOUND)
				emp_idx = i;
			continue;
		}

		if (!ether_cmp(&tmp_ea, addr) && (attr == tmp_attr)) {
			amt_idx = i;
			break;
		}
	}

	if (amt_idx == BCME_NOTFOUND) {
		amt_idx = emp_idx;
		if (emp_idx != BCME_NOTFOUND) {
			wlc_set_addrmatch(wlc, amt_idx, addr, attr);
		}
		else {
			WL_ERROR(("WARNING: no available"
				"enrty in AMT table ...\n"));
		}
	}

	return amt_idx;
}

int
wlc_macfltr_find_and_clear_addrmatch(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	struct ether_addr *addr, uint16 attr)
{
	int i, amt_idx = BCME_NOTFOUND;
	int max_maclist, max;
	struct ether_addr tmp_ea;
	uint16 tmp_attr;
	bss_macfltr_info_t *bfi = BSS_MACFLTR_INFO(wlc->macfltr, cfg);

	ASSERT(bfi != NULL);

	max_maclist = bfi->max_maclist;

	if (D11REV_GE(wlc->pub->corerev, 40))
		max = MIN(max_maclist, AMT_MAX_MACLIST_NUM);
	else
		max = MIN(max_maclist, RCMTA_SIZE);

	for (i = 0; i < max; i++) {
		/* skip reserved or allocated slot */
		if (wlc->wsec_keys[i + WSEC_MAX_DEFAULT_KEYS] != NULL)
			continue;

		wlc_get_addrmatch(wlc, i, &tmp_ea, &tmp_attr);

		/* skip empty slot */
		if (tmp_attr == 0)
			continue;

		if (!ether_cmp(&tmp_ea, addr) && (attr == tmp_attr)) {
			amt_idx = i;
			wlc_clear_addrmatch(wlc, amt_idx);
			break;
		}
	}

	if (amt_idx == BCME_NOTFOUND) {
		WL_ERROR(("WARNING: no available"
			"enrty in AMT table ...\n"));
	}

	return amt_idx;
}

static void
wlc_macfltr_bss_acksupr_enable(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool enable)
{
	if (D11REV_GE(wlc->pub->corerev, 40)) { /* 11ac */
		uint16 rw_val;
		rw_val = wlc_read_amtinfo_by_idx(wlc, AMT_IDX_MAC);
		if (enable) {
			rw_val |= ADDR_BMP_ACKSUPR;
		}
		else {
			rw_val &= ~ADDR_BMP_ACKSUPR;
		}
		wlc_write_amtinfo_by_idx(wlc, AMT_IDX_MAC, rw_val);
	}
	else {
		int idx;
		wlc_bsscfg_t *bsscfg;
		bool acksupr = FALSE;
		uint16 val = enable?MHF4_EN_ACKSUPR_BITMAP:0;

		FOREACH_AP(wlc, idx, bsscfg) {
			if (bsscfg == cfg)
				continue;
			if (bsscfg->acksupr_mac_filter) {
				acksupr = TRUE;
				break;
			}
		}
		if (!acksupr) {
			wlc_mhf(wlc, MHF4, MHF4_EN_ACKSUPR_BITMAP, val, WLC_BAND_ALL);
		}
	}
}

static int
wlc_macfltr_acksupr_set(wlc_info_t *wlc, wlc_bsscfg_t *cfg, int *max_maclist, bool config)
{
	int i, amt_idx;
	uint16 attr;
	bss_macfltr_info_t *bfi;

	bfi = BSS_MACFLTR_INFO(wlc->macfltr, cfg);
	ASSERT(bfi != NULL);

	if (cfg->acksupr_mac_filter && config) {

		bfi->max_maclist = *max_maclist;

		wlc_macfltr_bss_acksupr_enable(wlc, cfg, TRUE);

		/* add current maclist */
		for (i = 0; i < bfi->nmac; i++) {

			/* set amt table for each mac entry */
			if (D11REV_GE(wlc->pub->corerev, 40)) /* 11ac */
				attr = (AMT_ATTR_VALID | AMT_ATTR_A2);
			else /* pre 11ac */
				attr = AMT_ATTR_VALID;

			/* find existed amt entry or empty slot to set addrmatch */
			amt_idx = wlc_macfltr_find_and_add_addrmatch(wlc, cfg,
				&bfi->maclist[i], attr);
			if (amt_idx == BCME_NOTFOUND) {
				WL_ERROR(("WARNING: no available AMT entry for add %s.... \n",
					bcm_ether_ntoa(&bfi->maclist[i], eabuf)));
			}
		}

		WL_INFORM(("ACKSUPR is enabled because of current"
			"condition acksupr %d macmode %d\n",
			cfg->acksupr_mac_filter, bfi->macmode));
	}
	else {
		if (WLC_ACKSUPR(wlc)) {

			wlc_macfltr_bss_acksupr_enable(wlc, cfg, FALSE);

			/* unconfigure all amt enty in maclist */
			for (i = 0; i < bfi->nmac; i++) {
				if (D11REV_GE(wlc->pub->corerev, 40)) /* 11ac */
					attr = (AMT_ATTR_VALID | AMT_ATTR_A2);
				else /* pre 11ac */
					attr = AMT_ATTR_VALID;

				amt_idx = wlc_macfltr_find_and_clear_addrmatch(wlc, cfg,
					&bfi->maclist[i], attr);
				if (amt_idx == BCME_NOTFOUND) {
					WL_ERROR(("WARNING: no available AMT entry"
						"for remove %s.... \n",
						bcm_ether_ntoa(&bfi->maclist[i], eabuf)));
				}
			}
		}

		/* reset max maxlist to default value when acksupr is enabled */
		*max_maclist = bfi->max_maclist;
		bfi->max_maclist = MAXMACLIST;

		WL_INFORM(("ACKSUPR is disabled because of current"
			"condition acksupr %d macmode %d\n",
			cfg->acksupr_mac_filter, bfi->macmode));
	}

	return BCME_OK;
}

static int
wlc_macfltr_acksupr_enable(wlc_bsscfg_t *cfg, bool enable)
{
	int max_maclist = 0, i, ret = BCME_OK, idx;
	wlc_info_t *wlc = cfg->wlc;
	bss_macfltr_info_t *bfi;
	bool acksupr = FALSE;
	wlc_bsscfg_t *bsscfg;

	/*
	* TODO: current support chip list:
	*           11ac chip with corerev >= 40,
	*           43217 with corerev 30,
	*           4331 with corerev 29
	*/
	if (D11REV_LT(wlc->pub->corerev, 40) && (!D11REV_IS(wlc->pub->corerev, 30)) &&
		(!D11REV_IS(wlc->pub->corerev, 29)))
		return BCME_UNSUPPORTED;

	cfg->acksupr_mac_filter = enable;

	/* allocate amt_info if acksupr is set to enabled */
	if (BSSCFG_ACKSUPR(cfg)) {
		if (D11REV_GE(wlc->pub->corerev, 40)) {
			max_maclist = AMT_MAX_MACLIST_NUM;
#ifdef PSTA
			max_maclist = PSTA_IS_REPEATER(wlc)?PSTA_RA_PRIM_INDX:AMT_MAX_MACLIST_NUM;
#endif
		}
		else {
			max_maclist = RCMTA_SIZE;
#ifdef PSTA
			max_maclist = PSTA_IS_REPEATER(wlc)?PSTA_RA_PRIM_INDX:RCMTA_SIZE;
#endif
		}

		/* check if there is suplicated entry in sta_monitor list */
		bfi = BSS_MACFLTR_INFO(wlc->macfltr, cfg);
		ASSERT(bfi != NULL);
		for (i = 0; i < bfi->nmac; i++) {
			if (wlc_stamon_acksupr_duplicate(wlc, &bfi->maclist[i])) {
				cfg->acksupr_mac_filter = FALSE;
				return BCME_BADARG;
			}
		}
		/* alloc addrmatch_info and sync to amt table */
		if ((ret = wlc_addrmatch_info_alloc(wlc, max_maclist)) != BCME_OK) {
			return ret;
		}
	}

	/* set  bss maclist to amt table (add /remove), and override max_maclist limit */
	wlc_macfltr_acksupr_set(wlc, cfg, &max_maclist, TRUE);

	/* free amt_info if acksupr is set to disabled */
	FOREACH_AP(wlc, idx, bsscfg) {
		if (bsscfg->acksupr_mac_filter) {
			acksupr = TRUE;
			break;
		}
	}
	if (!acksupr) {
		wlc_addrmatch_info_free(wlc, max_maclist);
	}
	return BCME_OK;
}
/* ioctl entry */
static int
wlc_macfltr_doioctl(void *ctx, int cmd, void *arg, int len, struct wlc_if *wlcif)
{
	wlc_macfltr_info_t *mfi = (wlc_macfltr_info_t *)ctx;
	wlc_info_t *wlc = mfi->wlc;
	wlc_bsscfg_t *cfg;
	bss_macfltr_info_t *bfi;
	int val, *pval;
	int err = BCME_OK;

	/* default argument is generic integer */
	pval = (int *)arg;

	/* This will prevent the misaligned access */
	if (pval != NULL && (uint)len >= sizeof(val))
		bcopy(pval, &val, sizeof(val));
	else
		val = 0;

	cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(cfg != NULL);

	bfi = BSS_MACFLTR_INFO(mfi, cfg);
	ASSERT(bfi != NULL);

	switch (cmd) {
	case WLC_GET_MACLIST:
		err = wlc_macfltr_list_get(mfi, cfg, (struct maclist *)arg, (uint)len);
		break;

	case WLC_SET_MACLIST:
		err = wlc_macfltr_list_set(mfi, cfg, (struct maclist *)arg, (uint)len);
		break;

	case WLC_GET_MACMODE:
		*pval = bfi->macmode;
		break;

	case WLC_SET_MACMODE:
		/* macmode can only be configurable when acksupr is disabled */
		if (BSSCFG_ACKSUPR(cfg)) {
			WL_ERROR(("WARNING: macmode on ap bss can only be"
					"set when acksupr is disabled\n"));
			return BCME_UNSUPPORTED;
		}
		WL_INFORM(("Setting mac mode to %d %s\n", val,
			val == 0  ? "disabled" : val == 1 ? "deny" : "allow"));
		bfi->macmode = val;
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* debug... */

static int
wlc_macfltr_bss_init(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_macfltr_info_t *mfi = (wlc_macfltr_info_t *)ctx;
	wlc_info_t *wlc = mfi->wlc;
	bss_macfltr_info_t *bfi;

	ASSERT(cfg != NULL);

	bfi = BSS_MACFLTR_INFO(mfi, cfg);
	ASSERT(bfi != NULL);

	/* initialized max maclist entry with default value MAXMACLIST */
	if (BSSCFG_ACKSUPR(cfg)) {
		if (D11REV_GE(wlc->pub->corerev, 40))
			bfi->max_maclist = AMT_MAX_MACLIST_NUM;
		else
			bfi->max_maclist = RCMTA_SIZE;
	}
	else {
		bfi->max_maclist = MAXMACLIST;
	}
	return BCME_OK;
}

/* bsscfg cubby entries */
static void
wlc_macfltr_bss_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_macfltr_info_t *mfi = (wlc_macfltr_info_t *)ctx;
	wlc_info_t *wlc = mfi->wlc;
	bss_macfltr_info_t *bfi;
	int max_maclist = 0;

	(void)wlc;

	ASSERT(cfg != NULL);

	bfi = BSS_MACFLTR_INFO(mfi, cfg);
	ASSERT(bfi != NULL);

	/* deinit acksupr first */
	if (BSSCFG_ACKSUPR(cfg)) {
		wlc_macfltr_acksupr_set(wlc, cfg, &max_maclist, FALSE);
	}

	if (bfi->maclist != NULL) {
		MFREE(wlc->osh, bfi->maclist, OFFSETOF(struct maclist, ea) +
		                              bfi->nmac * ETHER_ADDR_LEN);
		bfi->maclist = NULL;
	}
}


/* APIs */
int
wlc_macfltr_addr_match(wlc_macfltr_info_t *mfi, wlc_bsscfg_t *cfg,
	const struct ether_addr *addr)
{
	bss_macfltr_info_t *bfi;
	uint i;

	ASSERT(cfg != NULL);

	bfi = BSS_MACFLTR_INFO(mfi, cfg);
	ASSERT(bfi != NULL);

	if (bfi->macmode != WLC_MACMODE_DISABLED) {
		for (i = 0; i < bfi->nmac; i++) {
			if (bcmp(addr, &bfi->maclist[i], ETHER_ADDR_LEN) == 0)
				break;
		}
		if (i < bfi->nmac) {
			switch (bfi->macmode) {
			case WLC_MACMODE_DENY:
				return WLC_MACFLTR_ADDR_DENY;
			case WLC_MACMODE_ALLOW:
				return WLC_MACFLTR_ADDR_ALLOW;
			default:
				ASSERT(0);
				break;
			}
		}
		else {
			switch (bfi->macmode) {
			case WLC_MACMODE_DENY:
				return WLC_MACFLTR_ADDR_NOT_DENY;
			case WLC_MACMODE_ALLOW:
				return WLC_MACFLTR_ADDR_NOT_ALLOW;
			default:
				ASSERT(0);
				break;
			}
		}
	}
	return WLC_MACFLTR_DISABLED;
}

/*
*	used when maclist add /delete
*/
static void
wlc_macfltr_addrmatch_set(wlc_info_t *wlc, bss_macfltr_info_t *bfi, wlc_bsscfg_t *cfg,
	struct maclist *maclist)
{
	int i, j, emp_idx;
	struct ether_addr ea;
	uint16 attr;
	bool found = FALSE;

	/* mean delete entry */
	if (maclist->count < bfi->nmac) {
		for (i = 0; i < bfi->max_maclist; i++) {
			/* skip reserved or allocated slot */
			if (wlc->wsec_keys[i + WSEC_MAX_DEFAULT_KEYS] != NULL)
				continue;

			wlc_get_addrmatch(wlc, i, &ea, &attr);

			/* skip empty slot */
			if (attr == 0)
				continue;

			found = FALSE;
			for (j = 0; j < maclist->count; j++) {
				if (!ether_cmp(&ea, &maclist->ea[j])) {
					found = TRUE;
					break;
				}
			}

			/* found delete entry */
			if (!found) {
				/* delete this entry from AMT table */
				wlc_clear_addrmatch(wlc, i);
			}
		}
	}
	else if (maclist->count > bfi->nmac) {
		for (i = 0; i < maclist->count; i++) {
			found = FALSE;
			emp_idx = BCME_NOTFOUND;
			for (j = 0; j < bfi->max_maclist; j++) {

				if (wlc_macfltr_is_stamon_resrv_slot(wlc, j))
					continue;

				wlc_get_addrmatch(wlc, j, &ea, &attr);

				/* skip empty slot, and record an empty one */
				if (attr == 0) {
					if (emp_idx == BCME_NOTFOUND)
						emp_idx = j;
					continue;
				}

				if (!ether_cmp(&ea, &maclist->ea[i])) {
					found = TRUE;
					break;
				}
			}

			/* found add entry */
			if (!found) {
				if (emp_idx == BCME_NOTFOUND) {
					WL_ERROR(("WARNING: no available"
						"enrty in AMT table ...\n"));
					continue;
				}

				/* add this entry to AMT table */
				wlc_set_addrmatch(wlc, emp_idx, &maclist->ea[i],
					(AMT_ATTR_VALID | AMT_ATTR_A2));
			}
		}
	}

}

/*
 * update amt entry if amt slot is reserved by stamon whenever it is configured
 * from disabled to enabled
 */
void
wlc_macfltr_addrmatch_resrv(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool enable)
{
	int i;
	int max_keys = WLC_MAX_WSEC_KEYS(wlc) - WSEC_MAX_DEFAULT_KEYS;
	int start = WSEC_MAX_DEFAULT_KEYS;
	int end = start + max_keys -1;
	struct ether_addr ea;
	uint16 attr;
	int amt_idx, tmp_amt_idx;

	ASSERT(WLC_ACKSUPR(wlc));

	for (i = start; i <= end; i++) {

		if (wlc->wsec_keys[i] == (wsec_key_t*)STAMON_WSEC_KEYS_STUB) {
			tmp_amt_idx = i - WSEC_MAX_DEFAULT_KEYS;

			if (enable) {
				/* get occuppied amt entry of reserved slot */
				wlc_get_addrmatch(wlc, tmp_amt_idx, &ea, &attr);

				/* if empty, reset it as AMT_ENTRY_USED directly, and try next  */
				if (attr == 0)
					continue;

				/* find existed amt entry or empty slot to set addrmatch */
				amt_idx = wlc_macfltr_find_and_add_addrmatch(wlc, cfg, &ea, attr);
				if (amt_idx == BCME_NOTFOUND) {
					WL_ERROR(("WARNING: no available AMT entry"
						"for add %s.... \n",
						bcm_ether_ntoa(&ea, eabuf)));
				}
			}
			wlc_clear_addrmatch(wlc, tmp_amt_idx);
		}
	}
}

/* set/get list */
int
wlc_macfltr_list_set(wlc_macfltr_info_t *mfi, wlc_bsscfg_t *cfg, struct maclist *maclist, uint len)
{
	wlc_info_t *wlc = mfi->wlc;
	bss_macfltr_info_t *bfi;
	int i;

	if ((uint)len < OFFSETOF(struct maclist, ea) + maclist->count * ETHER_ADDR_LEN)
		return BCME_BUFTOOSHORT;

	ASSERT(cfg != NULL);

	bfi = BSS_MACFLTR_INFO(mfi, cfg);
	ASSERT(bfi != NULL);

	if (maclist->count > bfi->max_maclist)
		return BCME_RANGE;

	/* update ucode AMT table first */
	if (BSSCFG_ACKSUPR(cfg)) {
		for (i = 0; i < maclist->count; i++) {
			if (wlc_stamon_acksupr_duplicate(wlc, &maclist->ea[i]))
				return BCME_BADARG;
		}
		wlc_macfltr_addrmatch_set(wlc, bfi, cfg, maclist);
	}

	/* free the old one */
	if (bfi->maclist != NULL) {
		MFREE(wlc->osh, bfi->maclist, OFFSETOF(struct maclist, ea) +
		                              bfi->nmac * ETHER_ADDR_LEN);
		bfi->nmac = 0;
	}
	bfi->maclist = MALLOC(wlc->osh, OFFSETOF(struct maclist, ea) +
	                                maclist->count * ETHER_ADDR_LEN);
	if (bfi->maclist == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}

	bfi->nmac = maclist->count;
	bcopy(maclist->ea, bfi->maclist, bfi->nmac * ETHER_ADDR_LEN);
	return BCME_OK;
}

int
wlc_macfltr_list_get(wlc_macfltr_info_t *mfi, wlc_bsscfg_t *cfg, struct maclist *maclist, uint len)
{
	bss_macfltr_info_t *bfi;

	ASSERT(cfg != NULL);

	bfi = BSS_MACFLTR_INFO(mfi, cfg);
	ASSERT(bfi != NULL);

	if (len < (bfi->nmac - 1) * sizeof(struct ether_addr) + sizeof(struct maclist))
		return BCME_BUFTOOSHORT;

	maclist->count = bfi->nmac;
	bcopy(bfi->maclist, maclist->ea, bfi->nmac * ETHER_ADDR_LEN);
	return BCME_OK;
}
