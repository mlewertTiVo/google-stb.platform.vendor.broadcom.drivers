/*
 * Separate alloc/free module for wlc_xxx.c files. Decouples
 * the code that does alloc/free from other code so data
 * structure changes don't affect ROMMED code as much.
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
 * $Id: wlc_alloc.c 613374 2016-01-18 21:22:35Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <proto/802.11.h>
#include <proto/802.11e.h>
#include <proto/wpa.h>
#include <proto/vlan.h>
#include <wlioctl.h>
#if defined(BCMSUP_PSK)
#include <proto/eapol.h>
#endif 
#include <bcmwpa.h>
#include <bcmdevs.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_alloc.h>
#include <wlc_keymgmt.h>
#include <wlc_objregistry.h>
#if defined(WLRSDB)
#include <wlc_rsdb.h>
#endif
#include <wlc_stf.h>

static wlc_pub_t *wlc_pub_malloc(wlc_info_t * wlc, osl_t *osh, uint unit, uint devid);
static void wlc_pub_mfree(wlc_info_t * wlc, osl_t *osh, wlc_pub_t *pub);

static void wlc_tunables_init(wlc_tunables_t *tunables, uint devid);

static bool wlc_attach_malloc_high(wlc_info_t *wlc, osl_t *osh, uint unit, uint *err, uint devid);
static bool wlc_attach_malloc_misc(wlc_info_t *wlc, osl_t *osh, uint unit, uint *err, uint devid);
static void wlc_detach_mfree_high(wlc_info_t *wlc, osl_t *osh);
static void wlc_detach_mfree_misc(wlc_info_t *wlc, osl_t *osh);

#ifndef NTXD_USB_4319
#define NTXD_USB_4319 0
#define NRPCTXBUFPOST_USB_4319 0
#endif
#ifndef DNGL_MEM_RESTRICT_RXDMA /* used only for BMAC low driver */
#define DNGL_MEM_RESTRICT_RXDMA 0
#endif

int
wlc_get_wlcindex(wlc_info_t *wlc)
{
	wlc_cmn_info_t* wlc_cmn;
	int idx;
	wlc_cmn = wlc->cmn;
	for ((idx) = 0; (int) (idx) < MAX_RSDB_MAC_NUM; (idx)++) {
		if (wlc == wlc_cmn->wlc[(idx)])
			return idx;
	}
	return -1;
}

void
BCMATTACHFN(wlc_tunables_init)(wlc_tunables_t *tunables, uint devid)
{
	/* tx/rx ring size for DMAs with 512 descriptor ring size max */
	tunables->ntxd = NTXD;
	tunables->nrxd = NRXD;

	/* tx/rx ring size for DMAs with 4096 descriptor ring size max */
	tunables->ntxd_large = NTXD_LARGE;
	tunables->nrxd_large = NRXD_LARGE;

#if defined(TXQ_MUX)
	/* low TxQ high/low watermarks */
	tunables->txq_highwater = WLC_TXQ_HIGHWATER;
	tunables->txq_lowwater = WLC_TXQ_LOWWATER;
#endif /* TXQ_MUX */

	tunables->rxbufsz = PKTBUFSZ;
	tunables->nrxbufpost = NRXBUFPOST;
	tunables->maxscb = MAXSCB;
	tunables->ampdunummpdu2streams = AMPDU_NUM_MPDU;
	tunables->ampdunummpdu3streams = AMPDU_NUM_MPDU_3STREAMS;
	tunables->maxpktcb = MAXPKTCB;
	tunables->maxucodebss = WLC_MAX_UCODE_BSS;
	tunables->maxucodebss4 = WLC_MAX_UCODE_BSS4;
	tunables->maxbss = MAXBSS;
	tunables->maxubss = MAXUSCANBSS;
	tunables->datahiwat = WLC_DATAHIWAT;
	tunables->ampdudatahiwat = WLC_AMPDUDATAHIWAT;
	tunables->rxbnd = RXBND;
	tunables->txsbnd = TXSBND;
	tunables->pktcbnd = PKTCBND;
	tunables->maxtdls = WLC_MAXTDLS;
	tunables->pkt_maxsegs = MAX_DMA_SEGS;
	tunables->maxscbcubbies = MAXSCBCUBBIES;
	tunables->maxbsscfgcubbies = MAXBSSCFGCUBBIES;

	tunables->max_notif_clients = MAX_NOTIF_CLIENTS;
	tunables->max_notif_servers = MAX_NOTIF_SERVERS;
	tunables->max_mempools = MAX_MEMPOOLS;
	tunables->amsdu_resize_buflen = PKTBUFSZ/4;
	tunables->ampdu_pktq_size = AMPDU_PKTQ_LEN;
	tunables->ampdu_pktq_fav_size = AMPDU_PKTQ_FAVORED_LEN;
	tunables->maxpcbcds = MAXPCBCDS;

#ifdef PROP_TXSTATUS
	tunables->wlfcfifocreditac0 = WLFCFIFOCREDITAC0;
	tunables->wlfcfifocreditac1 = WLFCFIFOCREDITAC1;
	tunables->wlfcfifocreditac2 = WLFCFIFOCREDITAC2;
	tunables->wlfcfifocreditac3 = WLFCFIFOCREDITAC3;
	tunables->wlfcfifocreditbcmc = WLFCFIFOCREDITBCMC;
	tunables->wlfcfifocreditother = WLFCFIFOCREDITOTHER;
	tunables->wlfc_fifo_cr_pending_thresh_ac_bk = WLFC_FIFO_CR_PENDING_THRESH_AC_BK;
	tunables->wlfc_fifo_cr_pending_thresh_ac_be = WLFC_FIFO_CR_PENDING_THRESH_AC_BE;
	tunables->wlfc_fifo_cr_pending_thresh_ac_vi = WLFC_FIFO_CR_PENDING_THRESH_AC_VI;
	tunables->wlfc_fifo_cr_pending_thresh_ac_vo = WLFC_FIFO_CR_PENDING_THRESH_AC_VO;
	tunables->wlfc_fifo_cr_pending_thresh_bcmc = WLFC_FIFO_CR_PENDING_THRESH_BCMC;
	tunables->wlfc_trigger = WLFC_INDICATION_TRIGGER;
	tunables->wlfc_fifo_bo_cr_ratio = WLFC_FIFO_BO_CR_RATIO;
	tunables->wlfc_comp_txstatus_thresh = WLFC_COMP_TXSTATUS_THRESH;
#endif /* PROP_TXSTATUS */

	/* set 4360 specific tunables */
	if (IS_DEV_AC4X4(devid) ||
		IS_DEV_AC3X3(devid) || IS_DEV_AC2X2(devid)) {
		tunables->ntxd = NTXD_AC3X3;
		tunables->nrxd = NRXD_AC3X3;
		tunables->rxbnd = RXBND_AC3X3;
		tunables->nrxbufpost = NRXBUFPOST_AC3X3;
		tunables->pktcbnd = PKTCBND_AC3X3;

		/* tx/rx ring size for DMAs with 4096 descriptor ring size max */
		tunables->ntxd_large = NTXD_LARGE_AC3X3;
		tunables->nrxd_large = NRXD_LARGE_AC3X3;
	}

	/* Key management */
	tunables->max_keys = WLC_KEYMGMT_MAX_KEYS;

#ifdef MFP
	tunables->max_keys += WLC_KEYMGMT_NUM_BSS_IGTK;
#endif /* MFP */

	/* IE mgmt */
	tunables->max_ie_build_cbs = MAXIEBUILDCBS;
	tunables->max_vs_ie_build_cbs = MAXVSIEBUILDCBS;
	tunables->max_ie_parse_cbs = MAXIEPARSECBS;
	tunables->max_vs_ie_parse_cbs = MAXVSIEPARSECBS;
	tunables->max_ie_regs = MAXIEREGS;

	tunables->num_rxivs = WLC_KEY_NUM_RX_SEQ;

	tunables->maxbestn = BESTN_MAX;
	tunables->maxmscan = MSCAN_MAX;
	tunables->nrxbufpost_fifo1 = NRXBUFPOST_FIFO1;
	tunables->ntxd_lfrag =	NTXD_LFRAG;
	tunables->nrxd_fifo1 =	NRXD_FIFO1;
	tunables->maxroamthresh = MAX_ROAM_TIME_THRESH;
	tunables->copycount = COPY_CNT_BYTES;
	tunables->nrxd_classified_fifo =	NRXD_CLASSIFIED_FIFO;
	tunables->bufpost_classified_fifo = NRXBUFPOST_CLASSIFIED_FIFO;

	tunables->scan_settle_time = WLC_DEFAULT_SETTLE_TIME;
	tunables->min_scballoc_mem = MIN_SCBALLOC_MEM;
}

static wlc_pub_t *
BCMATTACHFN(wlc_pub_malloc)(wlc_info_t * wlc, osl_t *osh, uint unit, uint devid)
{
	wlc_pub_t *pub;
	BCM_REFERENCE(unit);

	if ((pub = (wlc_pub_t*) MALLOCZ(osh, sizeof(wlc_pub_t))) == NULL) {
		goto fail;
	}

	pub->cmn = (wlc_pub_cmn_t*) obj_registry_get(wlc->objr, OBJR_WLC_PUB_CMN_INFO);
	if (pub->cmn == NULL) {
		if ((pub->cmn = (wlc_pub_cmn_t*) MALLOCZ(osh, sizeof(wlc_pub_cmn_t))) == NULL) {
			goto fail;
		}
		 obj_registry_set(wlc->objr, OBJR_WLC_PUB_CMN_INFO, pub->cmn);
	}
	(void)obj_registry_ref(wlc->objr, OBJR_WLC_PUB_CMN_INFO);

	if ((pub->tunables = (wlc_tunables_t *)
	     MALLOCZ(osh, sizeof(wlc_tunables_t))) == NULL) {
		goto fail;
	}

	/* need to init the tunables now */
	wlc_tunables_init(pub->tunables, devid);

#ifdef WLCNT
	if ((pub->_cnt = (wl_cnt_wlc_t *)MALLOCZ(osh, sizeof(wl_cnt_wlc_t))) == NULL) {
		goto fail;
	}

	if ((pub->_mcst_cnt = MALLOCZ(osh, WL_CNT_MCST_STRUCT_SZ)) == NULL) {
		goto fail;
	}
#endif /* WLCNT */
	if ((pub->_rxfifo_cnt = (wl_rxfifo_cnt_t*)
		 MALLOCZ(osh, sizeof(wl_rxfifo_cnt_t))) == NULL) {
		goto fail;
	}

#ifdef BCM_MEDIA_CLIENT
	pub->_media_client = TRUE;
#endif /* BCM_MEDIA_CLIENT */

	return pub;

fail:
	wlc_pub_mfree(wlc, osh, pub);
	return NULL;
}

static void
BCMATTACHFN(wlc_pub_mfree)(wlc_info_t * wlc, osl_t *osh, wlc_pub_t *pub)
{
	BCM_REFERENCE(wlc);
	if (pub == NULL)
		return;

	if (pub->tunables) {
		MFREE(osh, pub->tunables, sizeof(wlc_tunables_t));
		pub->tunables = NULL;
	}

#ifdef WLCNT
	if (pub->_cnt) {
		MFREE(osh, pub->_cnt, sizeof(wl_cnt_wlc_t));
		pub->_cnt = NULL;
	}

	if (pub->_mcst_cnt) {
		MFREE(osh, pub->_mcst_cnt, WL_CNT_MCST_STRUCT_SZ);
		pub->_mcst_cnt = NULL;
	}
#endif /* WLCNT */
	if (pub->_rxfifo_cnt) {
		MFREE(osh, pub->_rxfifo_cnt, sizeof(wl_rxfifo_cnt_t));
		pub->_rxfifo_cnt = NULL;
	}

	if (pub->cmn && obj_registry_unref(wlc->objr, OBJR_WLC_PUB_CMN_INFO) == 0) {
		obj_registry_set(wlc->objr, OBJR_WLC_PUB_CMN_INFO, NULL);
		MFREE(osh, pub->cmn, sizeof(wlc_pub_cmn_t));
	}

	MFREE(osh, pub, sizeof(wlc_pub_t));
}

static bool
BCMATTACHFN(wlc_attach_malloc_high)(wlc_info_t *wlc, osl_t *osh, uint unit, uint *err, uint devid)
{
	int i;

	BCM_REFERENCE(unit);
	BCM_REFERENCE(devid);


	/* OBJECT REGISTRY: check if shared key has value already stored */
	wlc->cmn = (wlc_cmn_info_t*) obj_registry_get(wlc->objr, OBJR_WLC_CMN_INFO);

	if (wlc->cmn == NULL) {
		if ((wlc->cmn =  (wlc_cmn_info_t*) MALLOCZ(osh,
			sizeof(wlc_cmn_info_t))) == NULL) {
			*err = 1035;
			goto fail;
		}
		/* OBJECT REGISTRY: We are the first instance, store value for key */
		obj_registry_set(wlc->objr, OBJR_WLC_CMN_INFO, wlc->cmn);
	}
	{
		int ref_count;

		/* OBJECT REGISTRY: Reference the stored value in both instances */
		ref_count = obj_registry_ref(wlc->objr, OBJR_WLC_CMN_INFO);
		ASSERT(ref_count <= MAX_RSDB_MAC_NUM);
		wlc->cmn->wlc[ref_count - 1] = wlc;
#ifdef WLRSDB
		/* Perform RSDB specific CMN initializations */
		if (wlc_rsdb_wlc_cmn_attach(wlc)) {
			goto fail;
		}
#endif
	}

	wlc->bandstate[0] = (wlcband_t*) obj_registry_get(wlc->objr, OBJR_WLC_BANDSTATE);
	if (wlc->bandstate[0] == NULL) {
		if ((wlc->bandstate[0] = (wlcband_t*)
			MALLOCZ(osh, (sizeof(wlcband_t) * MAXBANDS))) == NULL) {
			*err = 1010;
			goto fail;
		}
		obj_registry_set(wlc->objr, OBJR_WLC_BANDSTATE, wlc->bandstate[0]);
	}
	(void)(obj_registry_ref(wlc->objr, OBJR_WLC_BANDSTATE));

	for (i = 1; i < MAXBANDS; i++) {
		wlc->bandstate[i] =
		        (wlcband_t *)((uintptr)wlc->bandstate[0] + (sizeof(wlcband_t) * i));
	}

	if ((wlc->bandinst = (wlcband_inst_t **)
		MALLOCZ(osh, sizeof(wlcband_inst_t *) * MAXBANDS)) == NULL) {
		*err = 1016;
		goto fail;
	}

	for (i = 0; i < MAXBANDS; i++) {
		if ((wlc->bandinst[i] = (wlcband_inst_t *)
			MALLOCZ(osh, sizeof(wlcband_inst_t))) == NULL) {
			*err = 1018;
			goto fail;
		}
	}

	/* OBJECT REGISTRY: check if shared key has value already stored */
	wlc->modulecb = (modulecb_t*) obj_registry_get(wlc->objr, OBJR_MODULE_ID);
	if (wlc->modulecb == NULL) {
		if ((wlc->modulecb = (modulecb_t*) MALLOCZ(osh,
			sizeof(modulecb_t) * wlc->pub->max_modules)) == NULL) {
			*err = 1012;
			goto fail;
		}
		/* OBJECT REGISTRY: We are the first instance, store value for key */
		obj_registry_set(wlc->objr, OBJR_MODULE_ID, wlc->modulecb);
	}
	/* OBJECT REGISTRY: Reference the stored value in both instances */
	(void)obj_registry_ref(wlc->objr, OBJR_MODULE_ID);


	if ((wlc->modulecb_data = (modulecb_data_t*)
	     MALLOCZ(osh, sizeof(modulecb_data_t) * wlc->pub->max_modules)) == NULL) {
		*err = 1013;
		goto fail;
	}


	wlc->bsscfg = (wlc_bsscfg_t**) obj_registry_get(wlc->objr, OBJR_BSSCFG_PTR);
	if (wlc->bsscfg == NULL) {
		if ((wlc->bsscfg = (wlc_bsscfg_t**) MALLOCZ(osh,
			sizeof(wlc_bsscfg_t*) * WLC_MAXBSSCFG)) == NULL) {
			*err = 1016;
			goto fail;
		}
		obj_registry_set(wlc->objr, OBJR_BSSCFG_PTR, wlc->bsscfg);
	}
	(void)obj_registry_ref(wlc->objr, OBJR_BSSCFG_PTR);
#ifndef WLRSDB_DVT
	wlc->default_bss = obj_registry_get(wlc->objr, OBJR_DEFAULT_BSS);
#endif

	if (wlc->default_bss == NULL) {
		if ((wlc->default_bss = (wlc_bss_info_t*)
		     MALLOCZ(osh, sizeof(wlc_bss_info_t))) == NULL) {
			*err = 1010;
			goto fail;
		}
		obj_registry_set(wlc->objr, OBJR_DEFAULT_BSS, wlc->default_bss);
	}
#ifndef WLRSDB_DVT
	(void)obj_registry_ref(wlc->objr, OBJR_DEFAULT_BSS);
#endif
	if ((wlc->stf = (wlc_stf_t*)
	     MALLOCZ(osh, sizeof(wlc_stf_t))) == NULL) {
		*err = 1017;
		goto fail;
	}

	if ((wlc->corestate->macstat_snapshot = (uint16*)
	     MALLOCZ(osh, sizeof(uint16) * MACSTAT_OFFSET_SZ)) == NULL) {
		*err = 1027;
		goto fail;
	}

#if defined(DELTASTATS)
	if ((wlc->delta_stats = (delta_stats_info_t*)
	     MALLOCZ(osh, sizeof(delta_stats_info_t))) == NULL) {
		*err = 1023;
		goto fail;
	}
#endif /* DELTASTATS */

#ifdef WLROAMPROF
	if (MAXBANDS > 0) {
		int i;

		if ((wlc->bandstate[0]->roam_prof = (wl_roam_prof_t *)
		     MALLOCZ(osh, sizeof(wl_roam_prof_t) *
		     WL_MAX_ROAM_PROF_BRACKETS * MAXBANDS)) == NULL) {
			*err = 1032;
			goto fail;
		}

		for (i = 1; i < MAXBANDS; i++) {
			wlc->bandstate[i]->roam_prof =
				&wlc->bandstate[0]->roam_prof[i * WL_MAX_ROAM_PROF_BRACKETS];
		}
	}
#endif /* WLROAMPROF */
	return TRUE;

fail:
	return FALSE;
}

static bool
BCMATTACHFN(wlc_attach_malloc_misc)(wlc_info_t *wlc, osl_t *osh, uint unit, uint *err, uint devid)
{
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(osh);
	BCM_REFERENCE(unit);
	BCM_REFERENCE(err);
	BCM_REFERENCE(devid);

	return TRUE;
}

/*
 * The common driver entry routine. Error codes should be unique
 */
wlc_info_t *
BCMATTACHFN(wlc_attach_malloc)(osl_t *osh, uint unit, uint *err, uint devid, void *objr)
{
	wlc_info_t *wlc;

	if ((wlc = (wlc_info_t*) MALLOCZ(osh, sizeof(wlc_info_t))) == NULL) {
		*err = 1002;
		goto fail;
	}
	wlc->hwrxoff = WL_HWRXOFF;
	wlc->hwrxoff_pktget = (wlc->hwrxoff % 4) ?  wlc->hwrxoff : (wlc->hwrxoff + 2);

	/* Store the object registry */
	wlc->objr = objr;

	/* allocate wlc_pub_t state structure */
	if ((wlc->pub = wlc_pub_malloc(wlc, osh, unit, devid)) == NULL) {
		*err = 1003;
		goto fail;
	}
	wlc->pub->wlc = wlc;

	wlc->pub->max_modules = WLC_MAXMODULES;

#ifdef BCMPKTPOOL
	wlc->pub->pktpool = SHARED_POOL;
#endif /* BCMPKTPOOL */

#ifdef BCMFRAGPOOL
	wlc->pub->pktpool_lfrag = SHARED_FRAG_POOL;
#endif /* BCMFRAGPOOL */

#ifdef BCMRXFRAGPOOL
	wlc->pub->pktpool_rxlfrag = SHARED_RXFRAG_POOL;
#endif /* BCMRXFRAGPOOL */


	if ((wlc->corestate = (wlccore_t*)
	     MALLOCZ(osh, sizeof(wlccore_t))) == NULL) {
		*err = 1011;
		goto fail;
	}

	if (!wlc_attach_malloc_high(wlc, osh, unit, err, devid))
		goto fail;

	if (!wlc_attach_malloc_misc(wlc, osh, unit, err, devid))
		goto fail;

	return wlc;

fail:
	wlc_detach_mfree(wlc, osh);
	return NULL;
}
extern void *get_osh_cmn(osl_t *osh);

static void
BCMATTACHFN(wlc_detach_mfree_high)(wlc_info_t *wlc, osl_t *osh)
{
	int idx;

	if (wlc->modulecb && (obj_registry_unref(wlc->objr, OBJR_MODULE_ID) == 0)) {
		obj_registry_set(wlc->objr, OBJR_MODULE_ID, NULL);
		MFREE(osh, wlc->modulecb, sizeof(modulecb_t) * wlc->pub->max_modules);
	}
	wlc->modulecb = NULL;

	if (wlc->modulecb_data) {
		MFREE(osh, wlc->modulecb_data, sizeof(modulecb_data_t) * wlc->pub->max_modules);
		wlc->modulecb_data = NULL;
	}

	if (wlc->bsscfg && (obj_registry_unref(wlc->objr, OBJR_BSSCFG_PTR) == 0)) {
		obj_registry_set(wlc->objr, OBJR_BSSCFG_PTR, NULL);
		MFREE(osh, wlc->bsscfg, sizeof(wlc_bsscfg_t*) * WLC_MAXBSSCFG);
	}
	wlc->bsscfg = NULL;

	if (obj_registry_unref(wlc->objr, OBJR_DEFAULT_BSS) == 0) {
		if (wlc->default_bss) {
			MFREE(osh, wlc->default_bss, sizeof(wlc_bss_info_t));
			wlc->default_bss = NULL;
		}
		obj_registry_set(wlc->objr, OBJR_DEFAULT_BSS, NULL);
	}

	if (wlc->stf) {
		MFREE(osh, wlc->stf, sizeof(wlc_stf_t));
		wlc->stf = NULL;
	}

#if defined(DELTASTATS)
	if (wlc->delta_stats) {
		MFREE(osh, wlc->delta_stats, sizeof(delta_stats_info_t));
		wlc->delta_stats = NULL;
	}
#endif /* DELTASTATS */

	if (wlc->cmn != NULL) {
		if (obj_registry_unref(wlc->objr, OBJR_WLC_CMN_INFO) == 0) {
			obj_registry_set(wlc->objr, OBJR_WLC_CMN_INFO, NULL);
			MFREE(osh, wlc->cmn, sizeof(wlc_cmn_info_t));
		}
		else {
			idx = wlc_get_wlcindex(wlc);
			ASSERT(idx >= 0);
			wlc->cmn->wlc[idx] = NULL;
		}
	}

	if (wlc->bandstate[0] && obj_registry_unref(wlc->objr, OBJR_WLC_BANDSTATE) == 0) {
		obj_registry_set(wlc->objr, OBJR_WLC_BANDSTATE, NULL);
#ifdef WLROAMPROF
		if (wlc->bandstate[0]->roam_prof) {
			MFREE(osh, wlc->bandstate[0]->roam_prof,
				sizeof(wl_roam_prof_t) * MAXBANDS * WL_MAX_ROAM_PROF_BRACKETS);
		}
#endif /* WLROAMPROF */
		MFREE(osh, wlc->bandstate[0], (sizeof(wlcband_t) * MAXBANDS));
	}

	if (wlc->bandinst) {
		int i = 0;
		for (i = 0; i < MAXBANDS; i++) {
			if (wlc->bandinst[i]) {
				MFREE(osh, wlc->bandinst[i], sizeof(wlcband_inst_t));
				wlc->bandinst[i] = NULL;
			}
		}
		MFREE(osh, wlc->bandinst, sizeof(wlcband_inst_t *) * MAXBANDS);
		wlc->bandinst = NULL;
	}
}

static void
BCMATTACHFN(wlc_detach_mfree_misc)(wlc_info_t *wlc, osl_t *osh)
{
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(osh);

}

void
BCMATTACHFN(wlc_detach_mfree)(wlc_info_t *wlc, osl_t *osh)
{
	if (wlc == NULL)
		return;

	if (wlc->pub == NULL)
		goto free_wlc;

	wlc_detach_mfree_misc(wlc, osh);

	wlc_detach_mfree_high(wlc, osh);

	if (wlc->corestate) {
		if (wlc->corestate->macstat_snapshot) {
			MFREE(osh, wlc->corestate->macstat_snapshot,
				sizeof(uint16) * MACSTAT_OFFSET_SZ);
			wlc->corestate->macstat_snapshot = NULL;
		}
		MFREE(osh, wlc->corestate, sizeof(wlccore_t));
		wlc->corestate = NULL;
	}

	/* free pub struct */
	wlc_pub_mfree(wlc, osh, wlc->pub);
	wlc->pub = NULL;

free_wlc:
	/* free the wlc */
	MFREE(osh, wlc, sizeof(wlc_info_t));
	wlc = NULL;
}
