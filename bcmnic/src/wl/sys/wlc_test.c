/*
 * WLTEST related source code.
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_test.c 661665 2016-09-27 00:31:02Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <bcmsrom.h>
#include <bcmnvram.h>
#include <wlc_bmac.h>
#include <wlc_txbf.h>
#include <wlc_tx.h>
#include <wlc_test.h>
#include <wlc_dump.h>
#include <wlc_iocv.h>
#include <wlc_hw_priv.h>
#include <wlc_objregistry.h>
#include <wl_export.h>

#ifdef WL_MUPKTENG
#include <wlc_mutx.h>
#endif

#define TU2uS(tu) ((uint32)(tu) * 1024)
#define uS2TU(us) ((uint32)(us) / 1024)
#define SIM_PM_DEF_CYCLE 100
#define SIM_PM_DEF_UP 5
typedef enum {
	SIM_PM_STATE_DISABLE = 0,
	SIM_PM_STATE_AWAKE = 1,
	SIM_PM_STATE_ASLEEP = 2
} sim_pm_state_t;

/* IOVar table - please number the enumerators explicity */
enum {
	IOV_PKTENG = 0,
	IOV_PKTENG_MAXLEN = 1,	/* max packet size limit for packet engine */
	IOV_NVRAM_GET = 2,
	IOV_NVRAM_DUMP = 3,
	IOV_CISWRITE = 4,
	IOV_CISDUMP = 5,
	IOV_MANF_INFO = 6,
	IOV_LONGPKT = 7,	/* Enable long pkts for pktengine */
	IOV_PKTENG_STATUS = 8,
	IOV_SIM_PM = 9,
	IOV_LAST
};

static const bcm_iovar_t test_iovars[] = {
#if defined(WLPKTENG)
	{"pkteng", IOV_PKTENG, IOVF_SET_UP | IOVF_MFG, 0, IOVT_BUFFER, sizeof(wl_pkteng_t)},
	{"pkteng_maxlen", IOV_PKTENG_MAXLEN, IOVF_SET_UP | IOVF_MFG, 0, IOVT_UINT32, 0},
	{"pkteng_status", IOV_PKTENG_STATUS, IOVF_GET_UP | IOVF_MFG, 0, IOVT_BOOL, 0},
	{"longpkt", IOV_LONGPKT, (IOVF_SET_UP | IOVF_GET_UP), 0, IOVT_INT16, 0},
#endif
	{NULL, 0, 0, 0, 0, 0},
};


typedef struct wlc_test_cmn_info {
	int sim_pm_saved_PM;		/* PM state to return to */
	sim_pm_state_t sim_pm_state;	/* sim_pm state */
	uint32 sim_pm_cycle;		/* sim_pm cycle time in us */
	uint32 sim_pm_up;		/* sim_pm up time in us */
	struct wl_timer *sim_pm_timer;	/* timer to create simulated PM wake / sleep pattern */
} wlc_test_cmn_info_t;

/* private info */
struct wlc_test_info {
	wlc_info_t *wlc;
	wlc_test_cmn_info_t *cmn;
};

/* local functions */
static int wlc_test_doiovar(void *ctx, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, struct wlc_if *wlcif);
static int wlc_test_doioctl(void *ctx, uint cmd, void *arg, uint len, struct wlc_if *wlcif);

#if defined(WLPKTENG)
static void *wlc_tx_testframe_get(wlc_info_t *wlc, const struct ether_addr *da,
	const struct ether_addr *sa, const struct ether_addr *bssid, uint body_len);
#endif


#if defined(BCMDBG_DUMP)
static int wlc_nvram_dump(wlc_info_t *wlc, struct bcmstrbuf *b);
#endif 

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

#if defined(BCMDBG_DUMP)
static int
wlc_nvram_dump(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	char *nvram_vars;
	const char *q = NULL;
	int err;

	/* per-device vars first, if any */
	if (wlc->pub->vars) {
		q = wlc->pub->vars;
		/* loop to copy vars which contain null separated strings */
		while (*q != '\0') {
			bcm_bprintf(b, "%s\n", q);
			q += strlen(q) + 1;
		}
	}

	/* followed by global nvram vars second, if any */
	if ((nvram_vars = MALLOC(wlc->osh, MAXSZ_NVRAM_VARS)) == NULL) {
		err = BCME_NOMEM;
		goto exit;
	}
	if ((err = nvram_getall(nvram_vars, MAXSZ_NVRAM_VARS)) != BCME_OK)
		goto exit;
	if (nvram_vars[0]) {
		q = nvram_vars;
		/* loop to copy vars which contain null separated strings */
		while (((q - nvram_vars) < MAXSZ_NVRAM_VARS) && *q != '\0') {
			bcm_bprintf(b, "%s\n", q);
			q += strlen(q) + 1;
		}
	}

	/* check empty nvram */
	if (q == NULL)
		err = BCME_NOTFOUND;
exit:
	if (nvram_vars)
		MFREE(wlc->osh, nvram_vars, MAXSZ_NVRAM_VARS);

	return err;
}
#endif	



/* attach/detach */
wlc_test_info_t *
BCMATTACHFN(wlc_test_attach)(wlc_info_t *wlc)
{
	wlc_test_info_t *test;

	/* allocate module info */
	if ((test = MALLOCZ(wlc->osh, sizeof(*test))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	test->wlc = wlc;

		/* Check from obj registry if common info is allocated */
	test->cmn = (wlc_test_cmn_info_t *) obj_registry_get(wlc->objr, OBJR_TEST_CMN_INFO);

	if (test->cmn == NULL) {
		/* Object not found ! so alloc new object here and set the object */
		if (!(test->cmn = (wlc_test_cmn_info_t *)MALLOCZ(wlc->osh,
			sizeof(wlc_test_cmn_info_t)))) {
			WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			goto fail;
		}


		/* Update registry after all allocations */
		obj_registry_set(wlc->objr, OBJR_TEST_CMN_INFO, test->cmn);
	}

	(void) obj_registry_ref(wlc->objr, OBJR_TEST_CMN_INFO);

	/* register ioctl/iovar dispatchers and other module entries */
	if (wlc_module_add_ioctl_fn(wlc->pub, test, wlc_test_doioctl, 0, NULL) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_add_ioctl_fn() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register module up/down, watchdog, and iovar callbacks */
	if (wlc_module_register(wlc->pub, test_iovars, "wltest", test, wlc_test_doiovar,
			NULL, NULL, NULL) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#if defined(BCMDBG_DUMP)
	wlc_dump_register(wlc->pub, "nvram", (dump_fn_t)wlc_nvram_dump, (void *)wlc);
#endif

	/* set maximum packet length */
	wlc->pkteng_maxlen = PKTBUFSZ - TXOFF;

	return test;

fail:
	wlc_test_detach(test);
	return NULL;
}

void
BCMATTACHFN(wlc_test_detach)(wlc_test_info_t *test)
{
	wlc_info_t *wlc;

	if (test == NULL)
		return;

	wlc = test->wlc;

	(void)wlc_module_remove_ioctl_fn(wlc->pub, test);
	(void)wlc_module_unregister(wlc->pub, "wltest", test);

	if (obj_registry_unref(test->wlc->objr, OBJR_TEST_CMN_INFO) == 0) {
		obj_registry_set(test->wlc->objr, OBJR_TEST_CMN_INFO, NULL);
		if (test->cmn != NULL) {
			MFREE(test->wlc->osh, test->cmn,
				sizeof(wlc_test_cmn_info_t));
		}
	}
	MFREE(wlc->osh, test, sizeof(*test));
}

/* ioctl dispatcher */
static int
wlc_test_doioctl(void *ctx, uint cmd, void *arg, uint len, struct wlc_if *wlcif)
{
	wlc_test_info_t *test = ctx;
	wlc_info_t *wlc = test->wlc;
	int bcmerror = BCME_OK;

	BCM_REFERENCE(wlc);

	switch (cmd) {

	case WLC_GET_SROM: {
		srom_rw_t *s = (srom_rw_t *)arg;
		if (s->byteoff == 0x55ab) {	/* Hack for srcrc */
			s->byteoff = 0;
			bcmerror = wlc_iovar_op(wlc, "srcrc", arg, len, arg, len, IOV_GET, wlcif);
		} else
			bcmerror = wlc_iovar_op(wlc, "srom", arg, len, arg, len, IOV_GET, wlcif);
		break;
	}

#if defined(BCMDBG)
	case WLC_SET_SROM: {
		bcmerror = wlc_iovar_op(wlc, "srom", NULL, 0, arg, len, IOV_SET, wlcif);
		break;

	}
#endif	



	default:
		bcmerror = BCME_UNSUPPORTED;
		break;
	}

	return bcmerror;
} /* wlc_test_doioctl */


/* iovar dispatcher */
static int
wlc_test_doiovar(void *ctx, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, struct wlc_if *wlcif)
{
	wlc_test_info_t *test = ctx;
	wlc_info_t *wlc = test->wlc;
	int32 *ret_int_ptr;
	int32 int_val = 0;
	int err = BCME_OK;

	BCM_REFERENCE(wlc);
	BCM_REFERENCE(ret_int_ptr);

	if (plen >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	/* all iovars require mcnx being enabled */
	switch (actionid) {

#if defined(WLPKTENG)
	case IOV_SVAL(IOV_PKTENG): {
		wl_pkteng_t pkteng;
		void *pkt = NULL;
		uint32 pkteng_flags;

		bcopy(params, &pkteng, sizeof(wl_pkteng_t));
		pkteng_flags = pkteng.flags  & WL_PKTENG_PER_MASK;

		if (pkteng.length + TXOFF > WL_PKTENG_MAXPKTSZ)
			return BCME_BADLEN;

		/* Prepare the packet for Tx */
		if ((pkteng_flags == WL_PKTENG_PER_TX_START) ||
			(pkteng_flags == WL_PKTENG_PER_TX_WITH_ACK_START)) {
			struct ether_addr *sa;

			/* Don't allow pkteng to start if associated */
			if (wlc->pub->associated)
				return BCME_ASSOCIATED;
			if (!wlc->band->rspec_override)
				return BCME_ERROR;
#ifdef WL_BEAMFORMING
			if (TXBF_ENAB(wlc->pub)) {
				wlc_txbf_pkteng_tx_start(wlc->txbf, wlc->band->hwrs_scb);
			}
#endif /*  WL_BEAMFORMING */

			sa = (ETHER_ISNULLADDR(&pkteng.src)) ?
				&wlc->pub->cur_etheraddr : &pkteng.src;
			/* pkt will be freed in wlc_bmac_pkteng() */
			pkt = wlc_tx_testframe(wlc, &pkteng.dest, sa,
				0, pkteng.length);
			if (pkt == NULL)
				return BCME_ERROR;
			/* Unmute the channel for pkteng if quiet */
			if (wlc_quiet_chanspec(wlc->cmi, wlc->chanspec))
				wlc_mute(wlc, OFF, 0);
		}

		err = wlc_bmac_pkteng(wlc->hw, &pkteng, pkt);

		if ((err != BCME_OK) || (pkteng_flags == WL_PKTENG_PER_TX_STOP)) {
			/* restore Mute State after pkteng is done */
			if (wlc_quiet_chanspec(wlc->cmi, wlc->chanspec))
				wlc_mute(wlc, ON, 0);
#ifdef WL_BEAMFORMING
			if (TXBF_ENAB(wlc->pub)) {
				wlc_txbf_pkteng_tx_stop(wlc->txbf, wlc->band->hwrs_scb);
			}
#endif /*  WL_BEAMFORMING */
		}
		break;
	}

	case IOV_GVAL(IOV_PKTENG_MAXLEN): {
		*ret_int_ptr = wlc->pkteng_maxlen;
		break;
	}
	case IOV_GVAL(IOV_PKTENG_STATUS): {
		*ret_int_ptr = wlc->hw->pkteng_status;
		break;
	}
#endif 




#if defined(WLPKTENG)
	case IOV_GVAL(IOV_LONGPKT):
		*ret_int_ptr = ((wlc->pkteng_maxlen == WL_PKTENG_MAXPKTSZ) ? 1 : 0);
		break;

	case IOV_SVAL(IOV_LONGPKT):
		if ((int_val == 0) && (wlc->pkteng_maxlen == WL_PKTENG_MAXPKTSZ)) {
			/* restore 'wl rtsthresh' */
			wlc->RTSThresh = wlc->longpkt_rtsthresh;

			/* restore 'wl shmem 0x20' */
			wlc_write_shm(wlc, (uint16)0x20, wlc->longpkt_shm);

			/* restore pkteng_maxlen */
			wlc->pkteng_maxlen = PKTBUFSZ - TXOFF;
		} else if ((int_val == 1) && (wlc->pkteng_maxlen == (PKTBUFSZ - TXOFF))) {
			/* save current values */
			wlc->longpkt_rtsthresh = wlc->RTSThresh;
			wlc->longpkt_shm = wlc_read_shm(wlc, (uint16)0x20);

			/* 'wl rtsthresh 20000' */
			wlc->RTSThresh = (uint16) 20000;

			/* 'wl shmem 0x20 20000' */
			wlc_write_shm(wlc, (uint16)0x20, (uint16)20000);

			/* increase pkteng_maxlen */
			wlc->pkteng_maxlen = WL_PKTENG_MAXPKTSZ;
		}
		break;
#endif 

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

#if defined(WLPKTENG)
static void*
wlc_tx_testframe_get(wlc_info_t *wlc, const struct ether_addr *da,
	const struct ether_addr *sa, const struct ether_addr *bssid, uint body_len)
{
	int i, start = 0, len, buflen, filllen;
	void *p = NULL, *prevp = NULL, *headp = NULL;
	osl_t *osh;
	struct dot11_management_header *hdr;
	uint8 *pbody;
	bool first = TRUE;
	uint16 fc = FC_DATA;

	osh = wlc->osh;
	len = DOT11_MGMT_HDR_LEN + TXOFF + body_len;
#ifdef WL_MUPKTENG
	/* mupkteng: AMPDU will use QOS (Best Effort) */
	fc = wlc_mutx_pkteng_on(wlc->mutx)? FC_QOS_DATA : FC_DATA;
#endif

	while (len > 0) {
		buflen = (len > PKTBUFSZ) ? PKTBUFSZ : len;

		if ((p = PKTGET(osh, buflen, TRUE)) == NULL) {
			WL_ERROR(("wl%d: wlc_frame_get_mgmt_test: pktget error for len %d \n",
				wlc->pub->unit, buflen));
			WLCNTINCR(wlc->pub->_cnt->txnobuf);
			PKTFREE(osh, headp, TRUE);
			return (NULL);
		}
		ASSERT(ISALIGNED((uintptr)PKTDATA(osh, p), sizeof(uint32)));

		if (first) {
			/* reserve TXOFF bytes of headroom */
			PKTPULL(osh, p, TXOFF);
			PKTSETLEN(osh, p, buflen - TXOFF);

			/* Set MAX Prio for MGMT packets */
			PKTSETPRIO(p, MAXPRIO);

			/* Remember the head pointer */
			headp = p;

			/* construct a management frame */
			hdr = (struct dot11_management_header *)PKTDATA(osh, p);

			wlc_fill_mgmt_hdr(hdr, fc, da, sa, bssid);
			/* Find start of data and length */
			pbody = (uint8 *)&hdr[1];
			filllen = buflen - TXOFF - sizeof(struct dot11_management_header);

			first = FALSE;
		} else {
			pbody = (uint8 *)PKTDATA(osh, p);
			filllen = buflen;
		}

		for (i = start; i < start + filllen; i++) {
			pbody[i - start] = (uint8)i;
		}
		start = i;

		/* chain the pkt buffer */
		if (prevp)
			PKTSETNEXT(osh, prevp, p);

		prevp = p;
		len -= PKTBUFSZ;
	};

	return (headp);
} /* wlc_tx_testframe_get */

/** Create a test frame and enqueue into tx fifo */
void *
wlc_tx_testframe(wlc_info_t *wlc, struct ether_addr *da, struct ether_addr *sa,
                 ratespec_t rate_override, int length)
{
	void *p;
	bool shortpreamble;

	if ((p = wlc_tx_testframe_get(wlc, da, sa, sa, length)) == NULL)
		return NULL;

	/* check if the rate overrides are set */
	if (!RSPEC_ACTIVE(rate_override)) {
#if defined(TXQ_MUX)
		if (ETHER_ISMULTI(da)) {
			rate_override = wlc->band->mrspec_override;
		} else {
			rate_override = wlc->band->rspec_override;
		}

		if (!RSPEC_ACTIVE(rate_override)) {
			rate_override = LEGACY_RSPEC(wlc->band->hwrs_scb->rateset.rates[0]);
		}

		/* default to channel BW if the rate overrides had BW unspecified */
		if (RSPEC_BW(rate_override) == RSPEC_BW_UNSPECIFIED) {
			if (CHSPEC_IS20(wlc->chanspec)) {
				rate_override |= RSPEC_BW_20MHZ;
			} else if (CHSPEC_IS40(wlc->chanspec)) {
				rate_override |= RSPEC_BW_40MHZ;
			} else if (CHSPEC_IS80(wlc->chanspec)) {
				rate_override |= RSPEC_BW_80MHZ;
			} else if (CHSPEC_IS8080(wlc->chanspec) || CHSPEC_IS160(wlc->chanspec)) {
				rate_override |= RSPEC_BW_160MHZ;
			} else {
				/* un-handled bandwidth */
				ASSERT(0);
			}
		}
#else
		if (ETHER_ISMULTI(da))
			rate_override = wlc->band->mrspec_override;
		else
			rate_override = wlc->band->rspec_override;

		if (!RSPEC_ACTIVE(rate_override))
			rate_override = wlc->band->hwrs_scb->rateset.rates[0] & RATE_MASK;
#endif /* TXQ_MUX */
	}
	if (wlc->cfg->PLCPHdr_override == WLC_PLCP_LONG)
		shortpreamble = FALSE;
	else
		shortpreamble = TRUE;

	/* add headers */
	wlc_d11hdrs(wlc, p, wlc->band->hwrs_scb, shortpreamble, 0, 1,
	            TX_DATA_FIFO, 0, NULL, rate_override, NULL);

	/* Originally, we would call wlc_txfifo() here */
	/* wlc_txfifo(wlc, TX_DATA_FIFO, p, TRUE, 1); */
	/* However, that job is now the job of wlc_pkteng(). We return the packet so we can pass */
	/* it in as a parameter to wlc_pkteng() */
	return p;
} /* wlc_tx_testframe_get */

#ifdef WL_MUPKTENG
/* Create a test frame */
void *
wlc_mutx_testframe(wlc_info_t *wlc, struct scb *scb, struct ether_addr *sa,
                 ratespec_t rate_override, int fifo, int length, uint16 seq)
{
	void *p;
	bool shortpreamble = FALSE;

	if ((p = wlc_tx_testframe_get(wlc, &scb->ea, sa, sa, length)) == NULL) {
		printf("%s pkt is NULL\n", __FUNCTION__);
		return NULL;
	}

	/* Set BE Prio for packets */
	PKTSETPRIO(p, PRIO_8021D_BE);
	WLPKTTAG(p)->flags |= WLF_AMPDU_MPDU;
	WLPKTTAG(p)->flags |= WLF_BYPASS_TXC;
	WLPKTTAG(p)->flags &= ~WLF_EXPTIME;
	WLPKTTAG(p)->seq = seq;
	/* add headers */
	wlc_d11hdrs(wlc, p, scb, shortpreamble, 0, 1,
	            fifo, 0, NULL, rate_override, NULL);
	return p;
}
#endif /*  WL_MUPKTENG */
#endif 
