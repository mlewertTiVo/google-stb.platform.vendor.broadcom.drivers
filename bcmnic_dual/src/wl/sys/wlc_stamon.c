/*
 * STA monitor implementation
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_stamon.c $
 */

/**
 * @file
 * @brief
 * This is an AP/router specific feature.
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_key.h>
#include <wlc.h>
#include <wlc_hw.h>
#include <wlc_hw_priv.h>
#include <wlc_bmac.h>
#include <wlc_alloc.h>
#include <wlc_stamon.h>
#ifdef PSTA
#include <wlc_psta.h>
#endif /* PSTA */
#include <wlc_addrmatch.h>
#include <wlc_macfltr.h>
#include <bcmnvram.h>


/* Maximum STAs to monitor */
#define DFLT_STA_MONITOR_ENRTY 4
#define MAX_STA_MONITOR_ENRTY 8

/* wlc access macros */
#define WLCUNIT(x) ((x)->wlc->pub->unit)
#define WLCPUB(x) ((x)->wlc->pub)
#define WLCOSH(x) ((x)->wlc->osh)
#define WLCHW(x) ((x)->wlc->hw)
#define WLC(x) ((x)->wlc)

struct wlc_stamon_sta_cfg_entry {
	struct ether_addr ea;
	int8  rcmta_idx; /* index of RCMTA/AMT tables */
};

struct wlc_stamon_info {
	wlc_info_t *wlc;
	struct wlc_stamon_sta_cfg_entry *stacfg;
	uint32 stacfg_memsize; /* memory size of the stacfg array */
	uint16 stacfg_num; /* Total number of currently configured STAs */
};
#define STACFG_MAX_ENTRY(_stamon_info_) \
	((_stamon_info_)->stacfg_memsize/sizeof(struct wlc_stamon_sta_cfg_entry))

/* IOVar table */
enum {
	IOV_STA_MONITOR = 0,	/* Add/delete the monitored STA's MAC addresses. */
	IOV_LAST
};

static const bcm_iovar_t stamon_iovars[] = {
	{"sta_monitor", IOV_STA_MONITOR,
	(IOVF_SET_UP), IOVT_BUFFER, sizeof(wlc_stamon_sta_config_t)
	},

	{NULL, 0, 0, 0, 0 }
};

/* **** Private Functions Prototypes *** */

/* Forward declarations for functions registered for this module */
static int wlc_stamon_doiovar(
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
static int wlc_stamon_up(void *handle);
static int wlc_stamon_down(void *handle);
static void wlc_stamon_delete_all_stations(wlc_stamon_info_t *ctxt);
static void wlc_stamon_sta_ucode_sniff_enab(wlc_stamon_info_t *ctxt,
	int8 idx, bool enab);
static int wlc_stamon_sta_get(wlc_stamon_info_t *stamon_ctxt,
	struct maclist *ml);
static int wlc_stamon_rcmta_slots_reserve(wlc_stamon_info_t *ctxt);
static int wlc_stamon_rcmta_slots_free(wlc_stamon_info_t *ctxt);

/* **** Public Functions *** */
/*
 * Initialize the sta monitor private context and resources.
 * Returns a pointer to the sta monitor private context, NULL on failure.
 */
wlc_stamon_info_t *
BCMATTACHFN(wlc_stamon_attach)(wlc_info_t *wlc)
{
	wlc_pub_t *pub = wlc->pub;
	wlc_stamon_info_t *stamon_ctxt;
	uint32 i = 0;
	uint32 malloc_size, stacfg_memsize;
	char *val;
	int8 stamon_entry = DFLT_STA_MONITOR_ENRTY;

	/*
	 * override max_stamon_entry if nvram is configured, for
	 * dynamically sta_monitor entry configuration
	 */
	val = nvram_get("max_stamon_entry");
	if (val && (*val != '\0')) {
		stamon_entry = bcm_atoi(val);
		if (stamon_entry > MAX_STA_MONITOR_ENRTY) {
			WL_ERROR(("wl%d: max_stamon_entry %d exceed max available "
			"entry number %d, override to %d\n", wlc->pub->unit,
			stamon_entry, MAX_STA_MONITOR_ENRTY, MAX_STA_MONITOR_ENRTY));
			stamon_entry = MAX_STA_MONITOR_ENRTY;
		}
	}

	stacfg_memsize = (sizeof(struct wlc_stamon_sta_cfg_entry) *
		stamon_entry);

	malloc_size = sizeof(wlc_stamon_info_t) + stacfg_memsize;

	stamon_ctxt = (wlc_stamon_info_t*)MALLOC(pub->osh, malloc_size);
	if (stamon_ctxt == NULL) {
		WL_ERROR(("wl%d: %s: stamon_ctxt MALLOC failed; total "
			"mallocs %d bytes\n",
			wlc->pub->unit,
			__FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}

	/* Initializing stamon_ctxt structure */
	stamon_ctxt->wlc = wlc;
	stamon_ctxt->stacfg_memsize = stacfg_memsize;
	stamon_ctxt->stacfg =
		(struct wlc_stamon_sta_cfg_entry*)(((uint8*)stamon_ctxt) +
		sizeof(wlc_stamon_info_t));

	stamon_ctxt->stacfg_num = 0;
	for (i = 0; i < STACFG_MAX_ENTRY(stamon_ctxt); i++) {
		bcopy(&ether_null, &stamon_ctxt->stacfg[i].ea, ETHER_ADDR_LEN);
		stamon_ctxt->stacfg[i].rcmta_idx = -1;
	}

	/* register module */
	if (wlc_module_register(
			    wlc->pub,
			    stamon_iovars,
			    "sta_monitor",
			    stamon_ctxt,
			    wlc_stamon_doiovar,
			    NULL,
			    wlc_stamon_up,
			    wlc_stamon_down)) {
				WL_ERROR(("wl%d: %s wlc_module_register() failed\n",
				    WLCUNIT(stamon_ctxt),
				    __FUNCTION__));

				goto fail;
			    }

	return stamon_ctxt;

fail:
	MFREE(pub->osh, stamon_ctxt, malloc_size);

	return NULL;
}

/*
 * Release net detect private context and resources.
 */
void
BCMATTACHFN(wlc_stamon_detach)(wlc_stamon_info_t *ctxt)
{
	if (ctxt != NULL) {
		if (STAMON_ENAB(WLCPUB(ctxt))) {
			/* Clearing all existing monitoring STA */
			wlc_stamon_delete_all_stations(ctxt);
			/* Free RCMTA resource */
			wlc_stamon_rcmta_slots_free(ctxt);
		/* Disabling the STA monitor feature */
			WLCPUB(ctxt)->_stamon = FALSE;
		}
		/* Unregister the module */
		wlc_module_unregister(WLCPUB(ctxt), "sta_monitor", ctxt);

		/* Free the all context memory */
		MFREE(WLCOSH(ctxt), ctxt,
			(sizeof(wlc_stamon_info_t) + ctxt->stacfg_memsize));
	}
}


/*
 * Enable or disable sniffing of specified STA.
 *
 * Parameters:
 * ea - MAC address of STA to enable/disable sniffing.
 * enab - FALSE/TRUE - disable/enable sniffing.
 *
 * Return values:
 * BCME_UNSUPPORTED - feature is not supported.
 * BCME_BADARG - invalid argument;
 * BCME_ERROR - feature is supported but not enabled
 * BCME_NOTFOUND - entry not found.
 * BCME_OK - succeed.
 */
int
wlc_stamon_sta_sniff_enab(wlc_stamon_info_t *ctxt,
	struct ether_addr *ea, bool enab)
{
	int8 cfg_idx = 0;

#if defined(WLMSG_ERROR)
	char eabuf [ETHER_ADDR_STR_LEN];
#endif 

	if (ctxt == NULL)
		return BCME_UNSUPPORTED;

	if (!STAMON_ENAB(WLCPUB(ctxt)))
		return BCME_ERROR;

	if (ea == NULL) {
		WL_ERROR(("wl%d: %s: Invalid mac address pointer.\n",
			WLCUNIT(ctxt), __FUNCTION__));
		return BCME_BADARG;
					}

	/* Check MAC address validity.
	 * The MAC address must be valid unicast.
					 */
	if (ETHER_ISNULLADDR(ea) || !ETHER_ISUCAST(ea)) {
#if defined(WLMSG_ERROR)
		WL_ERROR(("wl%d: %s: Invalid MAC address %s.\n",
			WLCUNIT(ctxt), __FUNCTION__,
			bcm_ether_ntoa(ea, eabuf)));
#endif 
		return BCME_BADARG;
					}

	/* Searching for corresponding entry in the STAs list. */
	cfg_idx = wlc_stamon_sta_find(ctxt, ea);
	if (cfg_idx < 0) {
#if defined(WLMSG_ERROR)
		WL_ERROR(("wl%d: %s: STA %s not found.\n",
			WLCUNIT(ctxt), __FUNCTION__,
			bcm_ether_ntoa(ea, eabuf)));
#endif 
		return BCME_NOTFOUND;
	}

	/* programming the ucode */
	wlc_stamon_sta_ucode_sniff_enab(ctxt, cfg_idx, enab);

	return BCME_OK;
}

/*
 * The function processing the corresponding commands passed via
 * cfg->cmd parameter.
 *
 * STAMON_CFG_CMD_ENB command enable STA monitor feature
 * in runtime.
 *
 * STAMON_CFG_CMD_DSB command disable STA monitor feature
 * in runtime.
 *
 * STAMON_CFG_CMD_ADD command adding given MAC address of the STA
 * into the STA list and automatically start sniffing. The address
 * must be valid unicast.
 *
 * STAMON_CFG_CMD_DEL command removes given MAC address of the STA
 * from the STA list and automatically stop sniffing. The address
 * must be valid unicast.
 * The BROADCAST mac address has special meaning. It indicates to
 * clear all existing stations in the list.
 *
 * Return values:
 * BCME_UNSUPPORTED - feature is not supported.
 * BCME_BADARG - invalid argument;
 * BCME_NORESOURCE - no more entry in the STA list.
 * BCME_ERROR - feature is supported but not enabled
 * BCME_NOTFOUND - entry not found.
 * BCME_OK - succeed.
 */
int
wlc_stamon_sta_config(wlc_stamon_info_t *ctxt, wlc_stamon_sta_config_t* cfg)
{
	int8 cfg_idx;

	if (ctxt == NULL)
		return BCME_UNSUPPORTED;

	ASSERT(cfg != NULL);

	if (cfg->cmd == STAMON_CFG_CMD_DSB) {

		if (STAMON_ENAB(WLCPUB(ctxt))) {
			/* Clearing all existing monitoring STA */
			wlc_stamon_delete_all_stations(ctxt);
			/* Free RCMTA resource */
			wlc_stamon_rcmta_slots_free(ctxt);
			/* Disabling the STA monitor feature */
			WLCPUB(ctxt)->_stamon = FALSE;
		}
	} else if (cfg->cmd == STAMON_CFG_CMD_ENB) {

		if (!STAMON_ENAB(WLCPUB(ctxt))) {
			/* Reserve RCMTA resourse */
			wlc_stamon_rcmta_slots_reserve(ctxt);
			/* Enabling the STA monitor feature */
			WLCPUB(ctxt)->_stamon = TRUE;
	}
	} else if (cfg->cmd == STAMON_CFG_CMD_ADD) {

		if (!STAMON_ENAB(WLCPUB(ctxt))) {
			WL_ERROR(("wl%d: %s: Feature is not enabled.\n",
			WLCUNIT(ctxt), __FUNCTION__));
			return BCME_ERROR;
		}
	/* Check MAC address validity. The MAC address must be valid unicast. */
	if (ETHER_ISNULLADDR(&(cfg->ea)) ||
		!ETHER_ISUCAST(&(cfg->ea))) {
			WL_ERROR(("wl%d: %s: Invalid MAC address.\n",
				WLCUNIT(ctxt), __FUNCTION__));
		return BCME_BADARG;
	}

		/* check if there is duplicated entry in macfltr */
		if (WLC_ACKSUPR(ctxt->wlc) &&
			wlc_macfltr_acksupr_duplicate(ctxt->wlc, &cfg->ea))
			return BCME_BADARG;

		/* Searching existing entry in the list. */
		cfg_idx = wlc_stamon_sta_find(ctxt, &cfg->ea);
		if (cfg_idx < 0) {
			/* Searching free entry in the list. */
			cfg_idx = wlc_stamon_sta_find(ctxt, &ether_null);
			if (cfg_idx < 0) {
				WL_ERROR(("wl%d: %s: No more entry in the list.\n",
					WLCUNIT(ctxt), __FUNCTION__));
				return BCME_NORESOURCE;
			}
		}

		/* Adding MAC to the list */
		bcopy(&cfg->ea, &ctxt->stacfg[cfg_idx].ea, ETHER_ADDR_LEN);
		ctxt->stacfg_num++;

		/* Start sniffing the frames */
		wlc_stamon_sta_ucode_sniff_enab(ctxt, cfg_idx, TRUE);

	} else if (cfg->cmd == STAMON_CFG_CMD_DEL) {

		if (!STAMON_ENAB(WLCPUB(ctxt))) {
			WL_ERROR(("wl%d: %s: Feature is not enabled.\n",
			WLCUNIT(ctxt), __FUNCTION__));
			return BCME_ERROR;
			}
		/* The broadcast mac address indicates to
		 * clear all existing stations in the list.
		 */
		if (ETHER_ISBCAST(&(cfg->ea)))
			wlc_stamon_delete_all_stations(ctxt);
		else {
			/* Searching specified MAC address in the list. */
			cfg_idx = wlc_stamon_sta_find(ctxt, &cfg->ea);
			if (cfg_idx < 0) {
				WL_ERROR(("wl%d: %s: Entry not found.\n",
					WLCUNIT(ctxt), __FUNCTION__));
				return BCME_NOTFOUND;
			}

			/* Stop sniffing frames from this STA */
			wlc_stamon_sta_ucode_sniff_enab(ctxt, cfg_idx, FALSE);

			/* Delete the specified MAC address from the list */
			bcopy(&ether_null, &ctxt->stacfg[cfg_idx].ea, ETHER_ADDR_LEN);
			ctxt->stacfg_num--;
		}
	} else
		return BCME_BADARG;

	return BCME_OK;
}

/* Copy configured MAC to maclist
 *
 * Input Parameters:
 * ml - mac list to copy the configured sta's MACs
 *		ml->count contains maximum number of MAC
 *		the list can carry.
 */
int
wlc_stamon_sta_get(wlc_stamon_info_t *ctxt, struct maclist *ml)
{
	uint i, j;

	if (ctxt == NULL)
		return BCME_UNSUPPORTED;

	ASSERT(ml != NULL);

	/* ml->count contains maximum number of MAC maclist can carry. */
	for (i = 0, j = 0; i < STACFG_MAX_ENTRY(ctxt) && j < ml->count; i++) {
		if (!ETHER_ISNULLADDR(&ctxt->stacfg[i].ea)) {
			bcopy(&ctxt->stacfg[i].ea, &ml->ea[j], ETHER_ADDR_LEN);
			j++;
	}
	}
	/* return the number of copied MACs to the maclist */
	ml->count = j;

	return BCME_OK;
}

/* Searching the STA entry with given MAC address.
 * Return:
 * If entry is found return the index of the cfg entry.
 * If entry not found return -1.
 */
int8
wlc_stamon_sta_find(wlc_stamon_info_t *stamon_ctxt, const struct ether_addr *ea)
{
	int8 i;

	if (stamon_ctxt == NULL || ea == NULL)
		return -1;

	for (i = 0; i < (int8)(STACFG_MAX_ENTRY(stamon_ctxt)); i++)
		if (bcmp(ea, &stamon_ctxt->stacfg[i].ea, ETHER_ADDR_LEN) == 0)
			return i;

	return -1;
}

bool
wlc_stamon_acksupr_duplicate(wlc_info_t *wlc, struct ether_addr *ea)
{
	int i;
	wlc_stamon_info_t *stamon_ctxt;
	struct wlc_stamon_sta_cfg_entry *stacfg;
	if (!WLC_ACKSUPR(wlc))
		return FALSE;

	stamon_ctxt = wlc->stamon_info;
	ASSERT(stamon_ctxt != NULL);

	stacfg = stamon_ctxt->stacfg;
	ASSERT(stacfg != NULL);

	for (i = 0; i < STACFG_MAX_ENTRY(stamon_ctxt); i++) {
		if (bcmp(ea, &stacfg[i].ea, ETHER_ADDR_LEN) == 0) {
			WL_ERROR(("wl%d: duplicated entry configured in"
				"stamon and macfilter for %s\n",
				wlc->pub->unit, bcm_ether_ntoa(ea, eabuf)));
			return TRUE;
		}
	}

	return FALSE;
}

uint16
wlc_stamon_sta_num(wlc_stamon_info_t *stamon_ctxt)
{
	if (stamon_ctxt == NULL)
		return 0;

	return stamon_ctxt->stacfg_num;
}

/* **** Private Functions *** */

static int
wlc_stamon_doiovar(
	void                *hdl,
	const bcm_iovar_t   *vi,
	uint32              actionid,
	const char          *name,
	void                *p,
	uint                plen,
	void                *a,
	int                 alen,
	int                 vsize,
	struct wlc_if       *wlcif)
{
	wlc_stamon_info_t	*stamon_ctxt = hdl;
	int			err = BCME_OK;
	struct maclist *maclist;

	BCM_REFERENCE(vi);
	BCM_REFERENCE(name);
	BCM_REFERENCE(p);
	BCM_REFERENCE(plen);
	BCM_REFERENCE(vsize);

	switch (actionid) {
	case IOV_GVAL(IOV_STA_MONITOR):
		{
				maclist = (struct maclist *) a;
				if (alen < (int)(stamon_ctxt->stacfg_num*
					sizeof(struct ether_addr)))
				return BCME_BUFTOOSHORT;
				err = wlc_stamon_sta_get(stamon_ctxt, maclist);
			break;
		}
	case IOV_SVAL(IOV_STA_MONITOR):
		{
			wlc_stamon_sta_config_t *stamon_cfg = (wlc_stamon_sta_config_t *)a;
			err = wlc_stamon_sta_config(stamon_ctxt, stamon_cfg);
			break;
		}
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/*
 * Interface going up.
 */
static int
wlc_stamon_up(void *handle)
{
	wlc_stamon_info_t *ctxt = (wlc_stamon_info_t*)handle;

	if (ctxt == NULL)
		return BCME_UNSUPPORTED;

	if (STAMON_ENAB(WLCPUB(ctxt))) {
		/* Reserve RCMTA resourse */
		wlc_stamon_rcmta_slots_reserve(ctxt);
}

	return BCME_OK;
}


/*
 * Interface going down.
 */
static int
wlc_stamon_down(void *handle)
{
	wlc_stamon_info_t *ctxt = (wlc_stamon_info_t*)handle;

	if (ctxt == NULL)
		return BCME_UNSUPPORTED;

	if (STAMON_ENAB(WLCPUB(ctxt))) {
		/* Clearing all existing monitoring STA */
		wlc_stamon_delete_all_stations(ctxt);
		/* Free RCMTA resource */
		wlc_stamon_rcmta_slots_free(ctxt);
	}
	return BCME_OK;
}

/* Helper procedure function to program to
 * start/stop sniffing of the specified STA
 * in ucode.
 *
 * Parameters:
 * idx - Valid entry in the STAs list.
 * Caller must make validation.
 */
static void
wlc_stamon_sta_ucode_sniff_enab(wlc_stamon_info_t *ctxt,
	int8 idx, bool enab)
{
	uint16 amt_attr, bitmap;
	const struct ether_addr *shm_ea = NULL;
	struct wlc_stamon_sta_cfg_entry *cfg;

	cfg = &ctxt->stacfg[idx];
	/* Preparing target MAC address for the RCMTA or AMT table entry */
	shm_ea = enab ? &cfg->ea : &ether_null;

	if (D11REV_GE(WLCPUB(ctxt)->corerev, 40)) {
		amt_attr = enab ? (AMT_ATTR_VALID | AMT_ATTR_A2) : 0;
		bitmap = enab ? ADDR_STAMON_NBIT : 0;
		wlc_set_addrmatch(WLC(ctxt), cfg->rcmta_idx,
			shm_ea, amt_attr);
		wlc_write_amtinfo_by_idx(WLC(ctxt),
			cfg->rcmta_idx, bitmap);
	} else {
		amt_attr = enab ? (AMT_ATTR_VALID | AMT_ATTR_A2) : 0;
		bitmap = enab ? SKL_STAMON_NBIT : 0;
		wlc_set_addrmatch(WLC(ctxt), cfg->rcmta_idx, shm_ea, amt_attr);
		wlc_write_seckindxblk_secinfo_by_idx(WLC(ctxt),
			cfg->rcmta_idx + WSEC_MAX_DEFAULT_KEYS, bitmap);
	}
}


/* Clearing all monitored stations from the list.
 * Disabling STA sniffing if it is active for any
 * of the STAs from the list.
 */
static void
wlc_stamon_delete_all_stations(wlc_stamon_info_t *ctxt)
{
	int8 i;

	for (i = 0; i < ((int8)STACFG_MAX_ENTRY(ctxt)); i++) {
		/* Stop STA sniffing */
		wlc_stamon_sta_ucode_sniff_enab(ctxt, i, FALSE);
		bcopy(&ether_null, &ctxt->stacfg[i].ea, ETHER_ADDR_LEN);
	}
	ctxt->stacfg_num = 0;
}

/* Reserve dummy slots in wlc->wsec_keys key array per
 * STA config entry.
 * By this way the key management engine would consider
 * these slot as used and wouldn't use the corresponding
 * RCMTA entries.
 * NOTE: The order of rcmta slots is not required to be
 * consecutive.
 * RCMTA indexes are stored in sta config entries.
 *
 * Return:
 * BCME_NORESOURCE - unable to allocate dummy key entry.
 * BCME_OK - succeeded.
 */
static int
wlc_stamon_rcmta_slots_reserve(wlc_stamon_info_t *ctxt)
{
	uint wsec_start = WSEC_MAX_DEFAULT_KEYS;
	uint wsec_end;
	uint total = WLCPUB(ctxt)->wsec_max_rcmta_keys;
	uint wsec_idx, cfg_idx;
	uint cfg_max_idx = STACFG_MAX_ENTRY(ctxt);

#ifdef PSTA
	/* RCMTA slots above 24 are reserved by PSTA */
	if (PSTA_ENAB(WLCPUB(ctxt)))
		total = PSTA_RA_PRIM_INDX - 1;
#endif /* PSTA */

	wsec_end = wsec_start + total - 1;

	/* Searching for free key slots in the range */
	for (wsec_idx = wsec_start, cfg_idx = 0;
		(wsec_idx <= wsec_end) && (cfg_idx < cfg_max_idx);
		wsec_idx++) {
		if (WLC(ctxt)->wsec_keys[wsec_idx] != NULL)
			continue;
		/* assigning non NULL stub value,
		 * thus key management engine will
		 * skip this not null entry.
		 */
		WLC(ctxt)->wsec_keys[wsec_idx] = (wsec_key_t*)STAMON_WSEC_KEYS_STUB;
		/* Setup rcmta index into the config entry */
		ctxt->stacfg[cfg_idx].rcmta_idx = wsec_idx - WSEC_MAX_DEFAULT_KEYS;
		cfg_idx++;
	}

	/* update amt_info and re-locate amt table */
	if (WLC_ACKSUPR(WLC(ctxt)))
		wlc_macfltr_addrmatch_resrv(WLC(ctxt), WLC(ctxt)->cfg, TRUE);

	if (wsec_idx > wsec_end) {
		WL_ERROR(("wl%d %s: out of wsec_keys entries\n",
			WLCUNIT(ctxt), __FUNCTION__));
		return BCME_NORESOURCE;
	}

	return BCME_OK;
}

/* Free dummy key slots in wlc->wsec_keys
 * allocated per STA config.
 */
static int
wlc_stamon_rcmta_slots_free(wlc_stamon_info_t *ctxt)
{
	uint8 cfg_max_idx = STACFG_MAX_ENTRY(ctxt);
	uint8 wsec_idx, cfg_idx;

	/* update amt_info and re-locate amt table */
	if (WLC_ACKSUPR(WLC(ctxt)))
		wlc_macfltr_addrmatch_resrv(WLC(ctxt), WLC(ctxt)->cfg, FALSE);

	for (cfg_idx = 0; cfg_idx < cfg_max_idx; cfg_idx++) {
		/* Check for valid RCMTA index */
		if (ctxt->stacfg[cfg_idx].rcmta_idx < 0)
			continue;

		/* Free corresponding dummy key slots in wlc->wsec_keys */
		wsec_idx = (uint8)(ctxt->stacfg[cfg_idx].rcmta_idx
			+ WSEC_MAX_DEFAULT_KEYS);

		if (WLC(ctxt)->wsec_keys[wsec_idx] == (wsec_key_t*)STAMON_WSEC_KEYS_STUB)
			WLC(ctxt)->wsec_keys[wsec_idx] = NULL;

		/* Invalidate the rcmta index for this STA */
		ctxt->stacfg[cfg_idx].rcmta_idx = -1;
	}

	return BCME_OK;
}
