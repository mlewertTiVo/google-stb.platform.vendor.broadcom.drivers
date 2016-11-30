/*
 * Wrapper to scb rate selection algorithm of Broadcom
 * 802.11 Networking Adapter Device Driver.
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
 * $Id: wlc_scb_ratesel.c 644052 2016-06-17 03:04:30Z $
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <wlioctl.h>

#include <proto/802.11.h>
#include <d11.h>

#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <phy_tpc_api.h>
#include <wlc_scb.h>
#include <wlc_phy_hal.h>
#include <wlc_antsel.h>
#include <wlc_scb_ratesel.h>
#ifdef WL_LPC
#include <wlc_scb_powersel.h>
#endif
#include <wlc_txc.h>

#include <wl_dbg.h>

#include <wlc_vht.h>
#include <wlc_ht.h>
#include <wlc_lq.h>
#include <wlc_ulb.h>
#include <wlc_stf.h>

#ifdef WL_FRAGDUR
#include <wlc_fragdur.h>
#include <wlc_prot_g.h>
#include <wlc_prot_n.h>
#endif

/* iovar table */
enum {
	IOV_SCBRATE_DUMMY, /**< dummy one in order to register the module */
	IOV_SCB_RATESET    /**< dump the per-scb rate set */
};

#define LINK_BW_ENTRY	0
static const bcm_iovar_t scbrate_iovars[] = {
	{"scbrate_dummy", IOV_SCBRATE_DUMMY, (IOVF_SET_DOWN), 0, IOVT_BOOL, 0},
	{NULL, 0, 0, 0, 0, 0}
};

typedef struct ppr_rateset {
	uint16 vht_mcsmap;              /* supported vht mcs nss bit map */
	uint8 mcs[PHY_CORE_MAX];       /* supported mcs index bit map */
	uint16 vht_mcsmap_prop;        /* vht proprietary rates bit map */
} ppr_rateset_t;

/** Supported rates for current chanspec/country */
typedef struct ppr_support_rates {
	chanspec_t chanspec;
	clm_country_t country;
	ppr_rateset_t ppr_20_rates;
#if defined(WL11N) || defined(WL11AC)
	ppr_rateset_t ppr_40_rates;
#endif
#ifdef WL11AC
	ppr_rateset_t ppr_80_rates;
	ppr_rateset_t ppr_160_rates;
#endif
} ppr_support_rates_t;

struct wlc_ratesel_info {
	wlc_info_t	*wlc;		/**< pointer to main wlc structure */
	wlc_pub_t	*pub;		/**< public common code handler */
	ratesel_info_t *rsi;
	int32 scb_handle;
	int32 cubby_sz;
	ppr_support_rates_t *ppr_rates;
};

typedef struct ratesel_cubby ratesel_cubby_t;

/** rcb is per scb per ac rate control block. */
struct ratesel_cubby {
	rcb_t *scb_cubby;
};

#define SCB_RATESEL_INFO(wss, scb) ((SCB_CUBBY((scb), (wrsi)->scb_handle)))

#if defined(WME_PER_AC_TX_PARAMS)
#define SCB_RATESEL_CUBBY(wrsi, scb, ac)	\
	((void *)(((char*)((ratesel_cubby_t *)SCB_RATESEL_INFO(wrsi, scb))->scb_cubby) + \
		(ac * (wrsi)->cubby_sz)))
#else /* WME_PER_AC_TX_PARAMS */
#define SCB_RATESEL_CUBBY(wrsi, scb, ac)	\
	(((ratesel_cubby_t *)SCB_RATESEL_INFO(wrsi, scb))->scb_cubby)
#endif /* WME_PER_AC_TX_PARAMS */

static int wlc_scb_ratesel_doiovar(void *hdl, uint32 actionid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif);

static int wlc_scb_ratesel_scb_init(void *context, struct scb *scb);
static void wlc_scb_ratesel_scb_deinit(void *context, struct scb *scb);
#define wlc_scb_ratesel_dump_scb NULL

static ratespec_t wlc_scb_ratesel_getcurspec(wlc_ratesel_info_t *wrsi,
	struct scb *scb, uint8 ac);

static rcb_t *wlc_scb_ratesel_get_cubby(wlc_ratesel_info_t *wrsi, struct scb *scb,
	uint8 ac);
static int wlc_scb_ratesel_cubby_sz(void);
#ifdef WL11N
void wlc_scb_ratesel_rssi_enable(rssi_ctx_t *ctx);
void wlc_scb_ratesel_rssi_disable(rssi_ctx_t *ctx);
int wlc_scb_ratesel_get_rssi(rssi_ctx_t *ctx);
#endif
/* Get CLM enabled rates bitmap for a bw */
static  ppr_rateset_t *
wlc_scb_ratesel_get_ppr_rates(wlc_info_t *wlc, wl_tx_bw_t bw);

static void wlc_scb_ratesel_ppr_updbmp(wlc_info_t *wlc, ppr_t *target_pwrs);

static void
wlc_scb_ratesel_ppr_filter(wlc_info_t *wlc, ppr_rateset_t *clm_rates,
	wlc_rateset_t *scb_rates, bool scb_VHT);

static wl_tx_bw_t rspecbw_to_bcmbw(uint8 bw);
#ifdef WL_MIMOPS_CFG
static void wlc_scb_ratesel_siso_downgrade(wlc_info_t *wlc, struct scb *scb);
#endif

wlc_ratesel_info_t *
BCMATTACHFN(wlc_scb_ratesel_attach)(wlc_info_t *wlc)
{
	wlc_ratesel_info_t *wrsi;
#ifdef WL11AC
	ppr_support_rates_t *ppr_rates;
#endif

	if (!(wrsi = (wlc_ratesel_info_t *)MALLOC(wlc->osh, sizeof(wlc_ratesel_info_t)))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}

	bzero((char *)wrsi, sizeof(wlc_ratesel_info_t));
#ifdef WL11AC
	if (!(ppr_rates = (ppr_support_rates_t *)MALLOC(wlc->osh, sizeof(ppr_support_rates_t)))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}
	bzero(ppr_rates, sizeof(*ppr_rates));
	wrsi->ppr_rates = ppr_rates;
#endif
	wrsi->wlc = wlc;
	wrsi->pub = wlc->pub;

	if ((wrsi->rsi = wlc_ratesel_attach(wlc)) == NULL) {
		WL_ERROR(("%s: failed\n", __FUNCTION__));
		goto fail;
	}

	/* register module */
	if (wlc_module_register(wlc->pub, scbrate_iovars, "scbrate", wrsi, wlc_scb_ratesel_doiovar,
	                        NULL, NULL, NULL)) {
		WL_ERROR(("wl%d: %s:wlc_module_register failed\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* reserve cubby in the scb container for per-scb-ac private data */
	wrsi->scb_handle = wlc_scb_cubby_reserve(wlc, wlc_scb_ratesel_cubby_sz(),
	                                        wlc_scb_ratesel_scb_init,
	                                        wlc_scb_ratesel_scb_deinit,
	                                        wlc_scb_ratesel_dump_scb,
	                                        (void *)wlc);

	if (wrsi->scb_handle < 0) {
		WL_ERROR(("wl%d: %s:wlc_scb_cubby_reserve failed\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	wrsi->cubby_sz = wlc_ratesel_rcb_sz();

#ifdef WL11N
	wlc_ratesel_rssi_attach(wrsi->rsi, wlc_scb_ratesel_rssi_enable,
		wlc_scb_ratesel_rssi_disable, wlc_scb_ratesel_get_rssi);
#endif

	return wrsi;

fail:
	if (wrsi->rsi)
		wlc_ratesel_detach(wrsi->rsi);

	MFREE(wlc->osh, wrsi, sizeof(wlc_ratesel_info_t));
	return NULL;
}

void
BCMATTACHFN(wlc_scb_ratesel_detach)(wlc_ratesel_info_t *wrsi)
{
	if (!wrsi)
		return;

	wlc_module_unregister(wrsi->pub, "scbrate", wrsi);
	wlc_ratesel_detach(wrsi->rsi);

#ifdef WL11AC
	MFREE(wrsi->pub->osh, wrsi->ppr_rates, sizeof(ppr_support_rates_t));
#endif
	MFREE(wrsi->pub->osh, wrsi, sizeof(wlc_ratesel_info_t));
}

/* alloc per ac cubby space on scb attach. */
static int
wlc_scb_ratesel_scb_init(void *context, struct scb *scb)
{
	wlc_info_t *wlc = (wlc_info_t *)context;
	wlc_ratesel_info_t *wrsi = wlc->wrsi;
	ratesel_cubby_t *cubby_info = SCB_RATESEL_INFO(wrsi, scb);
	rcb_t *scb_rate_cubby;
	int cubby_size;

#if defined(WME_PER_AC_TX_PARAMS)
	cubby_size = AC_COUNT * wrsi->cubby_sz;
#else
	cubby_size = wrsi->cubby_sz;
#endif

	WL_RATE(("%s scb %p allocate cubby space.\n",
		__FUNCTION__, OSL_OBFUSCATE_BUF(scb)));
	if (scb && !SCB_INTERNAL(scb)) {
		scb_rate_cubby = (rcb_t *)MALLOCZ(wlc->osh, cubby_size);
		if (!scb_rate_cubby) {
			WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__, cubby_size,
				MALLOCED(wlc->osh)));
			return BCME_NOMEM;
		}
		bzero(scb_rate_cubby, cubby_size);
		cubby_info->scb_cubby = scb_rate_cubby;
	}
	return BCME_OK;
}

/* free cubby space after scb detach */
static void
wlc_scb_ratesel_scb_deinit(void *context, struct scb *scb)
{
	wlc_info_t *wlc = (wlc_info_t *)context;
	wlc_ratesel_info_t *wrsi = wlc->wrsi;
	ratesel_cubby_t *cubby_info = SCB_RATESEL_INFO(wrsi, scb);
	int cubby_size;

#if defined(WME_PER_AC_TX_PARAMS)
	cubby_size = AC_COUNT * wrsi->cubby_sz;
#else
	cubby_size = wrsi->cubby_sz;
#endif

	WL_RATE(("%s scb %p free cubby space.\n",
		__FUNCTION__, OSL_OBFUSCATE_BUF(scb)));
	if (wlc && cubby_info && !SCB_INTERNAL(scb) && cubby_info->scb_cubby) {
		MFREE(wlc->osh, cubby_info->scb_cubby, cubby_size);
		cubby_info->scb_cubby = NULL;
	}
}

/* handle SCBRATE related iovars */
static int
wlc_scb_ratesel_doiovar(void *hdl, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint val_size, struct wlc_if *wlcif)
{
	wlc_ratesel_info_t *wrsi = hdl;
	wlc_info_t *wlc;
	wlc_bsscfg_t *bsscfg;
	int err = 0;

	BCM_REFERENCE(plen);
	BCM_REFERENCE(val_size);

	wlc = wrsi->wlc;
	/* update bsscfg w/provided interface context */
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);

	if (!bsscfg) {
		return BCME_ERROR;
	}

	switch (actionid) {
	default:
		err = BCME_UNSUPPORTED;
	}
	return err;
}

static rcb_t *
wlc_scb_ratesel_get_cubby(wlc_ratesel_info_t *wrsi, struct scb *scb, uint8 ac)
{

	ASSERT(wrsi);

	ac = (WME_PER_AC_MAXRATE_ENAB(wrsi->pub) ? ac : 0);
	ASSERT(ac < AC_COUNT);

	return (SCB_RATESEL_CUBBY(wrsi, scb, ac));
}



#ifdef WL11N
bool
wlc_scb_ratesel_sync(wlc_ratesel_info_t *wrsi, struct scb *scb, uint8 ac, uint now, int rssi)
{
	rcb_t *state;

	state = wlc_scb_ratesel_get_cubby(wrsi, scb, ac);

	return wlc_ratesel_sync(state, now, rssi);
}
#endif /* WL11N */

#ifdef WLMESH
uint32
wlc_scb_ratesel_getcurpsr(wlc_info_t *wlc, struct scb *scb)
{
	uint16 frameid = TX_AC_BE_FIFO;
	rcb_t *state;

	state = wlc_scb_ratesel_get_cubby(wlc->wrsi, scb, frameid);
	return wlc_ratesel_get_curpsr(state);
}
#endif

static ratespec_t
wlc_scb_ratesel_getcurspec(wlc_ratesel_info_t *wrsi, struct scb *scb, uint8 ac)
{
	rcb_t *state;

	state = wlc_scb_ratesel_get_cubby(wrsi, scb, ac);
	if (state == NULL) {
		WL_ERROR(("%s: null state wrsi = %p scb = %p ac = %d\n",
			__FUNCTION__, OSL_OBFUSCATE_BUF(wrsi), OSL_OBFUSCATE_BUF(scb), ac));
		ASSERT(0);
		return (WLC_RATE_6M | RSPEC_BW_20MHZ);
	}
	return wlc_ratesel_getcurspec(state);
}

/* given only wlc and scb, return best guess at the primary rate */
ratespec_t
wlc_scb_ratesel_get_primary(wlc_info_t *wlc, struct scb *scb, void *pkt)
{
	ratespec_t rspec = 0;
	wlcband_t *scbband = wlc_scbband(wlc, scb);
	uint phyctl1_stf = wlc->stf->ss_opmode;
	uint fifo;
#ifdef WME
	uint tfifo;
#endif
	uint8 prio;

#ifdef WL11N
	uint32 mimo_txbw = 0;
#if WL_HT_TXBW_OVERRIDE_ENAB
	uint32 _txbw2rspecbw[] = {
		RSPEC_BW_20MHZ, /* WL_TXBW20L	*/
		RSPEC_BW_20MHZ, /* WL_TXBW20U	*/
		RSPEC_BW_40MHZ, /* WL_TXBW40	*/
		RSPEC_BW_40MHZ, /* WL_TXBW40DUP */
		RSPEC_BW_20MHZ, /* WL_TXBW20LL */
		RSPEC_BW_20MHZ, /* WL_TXBW20LU */
		RSPEC_BW_20MHZ, /* WL_TXBW20UL */
		RSPEC_BW_20MHZ, /* WL_TXBW20UU */
		RSPEC_BW_40MHZ, /* WL_TXBW40L */
		RSPEC_BW_40MHZ, /* WL_TXBW40U */
		RSPEC_BW_80MHZ /* WL_TXBW80 */
	};
	int8 txbw_override_idx;
#endif /* WL_HT_TXBW_OVERRIDE_ENAB */
#endif /* WL11N */

	prio = 0;
	if ((pkt != NULL) && SCB_QOS(scb)) {
		prio = (uint8)PKTPRIO(pkt);
		ASSERT(prio <= MAXPRIO);
	}

	fifo = TX_AC_BE_FIFO;

	if (BSSCFG_AP(scb->bsscfg) && SCB_ISMULTI(scb) &&
		WLC_BCMC_PSMODE(wlc, scb->bsscfg)) {
		fifo = TX_BCMC_FIFO;
	}
	else if (SCB_WME(scb)) {
		fifo = prio2fifo[prio];
#ifdef WME
		tfifo = fifo;
		if (wlc_wme_downgrade_fifo(wlc, &fifo, scb) == BCME_ERROR) {
			/* packet may be tossed; give a best guess anyway */
			fifo = tfifo;
		}
#endif /* WME */
	}

	if (scbband == NULL) {
		ASSERT(0);
		return 0;
	}
	if (RSPEC_ACTIVE(scbband->rspec_override)) {
		/* get override if active */
		rspec = scbband->rspec_override;
	} else {
		/* let ratesel figure it out if override not present */
		rspec = wlc_scb_ratesel_getcurspec(wlc->wrsi, scb, WME_PRIO2AC(prio));
	}

#ifdef WL11N
	if (N_ENAB(wlc->pub)) {
		/* apply siso/cdd to single stream mcs's or ofdm if rspec is auto selected */
		if (((IS_MCS(rspec) && IS_SINGLE_STREAM(rspec & RSPEC_RATE_MASK)) ||
			RSPEC_ISOFDM(rspec)) &&
			!(rspec & RSPEC_OVERRIDE_MODE)) {

			rspec &= ~(RSPEC_TXEXP_MASK | RSPEC_STBC);

			/* For SISO MCS use STBC if possible */
			if (IS_MCS(rspec) && (WLC_IS_STBC_TX_FORCED(wlc) ||
				((RSPEC_ISVHT(rspec) && WLC_STF_SS_STBC_VHT_AUTO(wlc, scb)) ||
				(RSPEC_ISHT(rspec) && WLC_STF_SS_STBC_HT_AUTO(wlc, scb))))) {
				ASSERT(WLC_STBC_CAP_PHY(wlc));
				rspec |= RSPEC_STBC;
			} else if (phyctl1_stf == PHY_TXC1_MODE_CDD) {
				rspec |= (1 << RSPEC_TXEXP_SHIFT);
			}
		}

		/* bandwidth */
		if (RSPEC_BW(rspec) != RSPEC_BW_UNSPECIFIED) {
			mimo_txbw = RSPEC_BW(rspec);
		} else if ((CHSPEC_IS8080(wlc->chanspec) &&
			scb->flags3 & SCB3_IS_80_80) &&
			RSPEC_ISVHT(rspec)) {
			mimo_txbw = RSPEC_BW_160MHZ;
		} else if ((CHSPEC_IS160(wlc->chanspec) &&
			scb->flags3 & SCB3_IS_160) &&
			RSPEC_ISVHT(rspec)) {
			mimo_txbw = RSPEC_BW_160MHZ;
		} else if (CHSPEC_BW_GE(wlc->chanspec, WL_CHANSPEC_BW_80) &&
			RSPEC_ISVHT(rspec)) {
			mimo_txbw = RSPEC_BW_80MHZ;
		} else if (CHSPEC_BW_GE(wlc->chanspec, WL_CHANSPEC_BW_40)) {
			/* default txbw is 20in40 */
			mimo_txbw = RSPEC_BW_20MHZ;

			if ((RSPEC_ISHT(rspec) || RSPEC_ISVHT(rspec)) &&
			    (scb->flags & SCB_IS40)) {
				mimo_txbw = RSPEC_BW_40MHZ;
#ifdef WLMCHAN
				if (MCHAN_ENAB(wlc->pub) && BSSCFG_AP(scb->bsscfg) &&
					(WLC_ULB_CHSPEC_ISLE20(wlc,
						scb->bsscfg->current_bss->chanspec))) {
					mimo_txbw = RSPEC_BW_20MHZ;
				}
#endif /* WLMCHAN */
			}
		} else	{
			mimo_txbw = RSPEC_BW_20MHZ;
		}

#if WL_HT_TXBW_OVERRIDE_ENAB
			if (CHSPEC_IS40(wlc->chanspec) || CHSPEC_IS80(wlc->chanspec)) {
				WL_HT_TXBW_OVERRIDE_IDX(wlc->hti, rspec, txbw_override_idx);

				if (txbw_override_idx >= 0) {
					mimo_txbw = _txbw2rspecbw[txbw_override_idx];
				}
			}
#endif /* WL_HT_TXBW_OVERRIDE_ENAB */
		rspec &= ~RSPEC_BW_MASK;
		rspec |= mimo_txbw;
	} else
#endif /* WL11N */
	{
		rspec |= RSPEC_BW_20MHZ;
		/* for nphy, stf of ofdm frames must follow policies */
		if ((WLCISNPHY(scbband) || WLCISHTPHY(scbband)) && RSPEC_ISOFDM(rspec)) {
			rspec &= ~RSPEC_TXEXP_MASK;
			if (phyctl1_stf == PHY_TXC1_MODE_CDD) {
				rspec |= (1 << RSPEC_TXEXP_SHIFT);
			}
		}
	}

	if (!RSPEC_ACTIVE(scbband->rspec_override)) {
		if (IS_MCS(rspec) && (WLC_HT_GET_SGI_TX(wlc->hti) == ON))
			rspec |= RSPEC_SHORT_GI;
		else if (WLC_HT_GET_SGI_TX(wlc->hti) == OFF)
			rspec &= ~RSPEC_SHORT_GI;

	}

	if (!RSPEC_ACTIVE(scbband->rspec_override)) {
		ASSERT(!(rspec & RSPEC_LDPC_CODING));
		rspec &= ~RSPEC_LDPC_CODING;
		if (wlc->stf->ldpc_tx == ON ||
			(SCB_LDPC_CAP(scb) && wlc->stf->ldpc_tx == AUTO)) {
			if (IS_MCS(rspec))
				rspec |= RSPEC_LDPC_CODING;
		}
	}
	return rspec;
}

/* wrapper function to select transmit rate given per-scb state */
void BCMFASTPATH
wlc_scb_ratesel_gettxrate(wlc_ratesel_info_t *wrsi, struct scb *scb, uint16 *frameid,
	ratesel_txparams_t *cur_rate, uint16 *flags)
{
	rcb_t *state;

	state = wlc_scb_ratesel_get_cubby(wrsi, scb, cur_rate->ac);
	if (state == NULL) {
		WL_ERROR(("%s: null state wrsi = %p scb = %p ac = %d\n",
		__FUNCTION__, OSL_OBFUSCATE_BUF(wrsi), OSL_OBFUSCATE_BUF(scb), cur_rate->ac));
		ASSERT(0);
		return;
	}

	wlc_ratesel_gettxrate(state, frameid, cur_rate, flags);
}

#ifdef WL11N
void
wlc_scb_ratesel_probe_ready(wlc_ratesel_info_t *wrsi, struct scb *scb, uint16 frameid,
	bool is_ampdu, uint8 ampdu_txretry, uint8 ac)
{
	rcb_t *state;

	state = wlc_scb_ratesel_get_cubby(wrsi, scb, ac);
	if (state == NULL) {
		WL_ERROR(("%s: null state wrsi = %p scb = %p ac = %d\n",
			__FUNCTION__, OSL_OBFUSCATE_BUF(wrsi), OSL_OBFUSCATE_BUF(scb), ac));
		ASSERT(0);
		return;
	}
	wlc_ratesel_probe_ready(state, frameid, is_ampdu, ampdu_txretry);
}

void BCMFASTPATH
wlc_scb_ratesel_upd_rxstats(wlc_ratesel_info_t *wrsi, ratespec_t rx_rspec, uint16 rxstatus2)
{
	wlc_ratesel_upd_rxstats(wrsi->rsi, rx_rspec, rxstatus2);
}
#endif /* WL11N */

/* non-AMPDU txstatus rate update, default to use non-mcs rates only */
void
wlc_scb_ratesel_upd_txstatus_normalack(wlc_ratesel_info_t *wrsi, struct scb *scb, tx_status_t *txs,
	uint16 sfbl, uint16 lfbl, uint8 tx_mcs,
	bool sgi, uint8 antselid, bool fbr, uint8 ac)
{
	rcb_t *state;

	state = wlc_scb_ratesel_get_cubby(wrsi, scb, ac);
	if (state == NULL) {
		ASSERT(0);
		return;
	}
	wlc_ratesel_upd_txstatus_normalack(state, txs, sfbl, lfbl, tx_mcs, sgi, antselid, fbr);
}

#ifdef WL11N
void
wlc_scb_ratesel_aci_change(wlc_ratesel_info_t *wrsi, bool aci_state)
{
	wlc_ratesel_aci_change(wrsi->rsi, aci_state);
}

/*
 * Return the fallback rate of the specified mcs rate.
 * Ensure that is a mcs rate too.
 */
ratespec_t
wlc_scb_ratesel_getmcsfbr(wlc_ratesel_info_t *wrsi, struct scb *scb, uint8 ac, uint8 mcs)
{
	rcb_t *state;

	state = wlc_scb_ratesel_get_cubby(wrsi, scb, ac);
	ASSERT(state);

	return (wlc_ratesel_getmcsfbr(state, ac, mcs));
}

#ifdef WLAMPDU_MAC
/*
 * The case that (mrt+fbr) == 0 is handled as RTS transmission failure.
 */
void
wlc_scb_ratesel_upd_txs_ampdu(wlc_ratesel_info_t *wrsi, struct scb *scb,
	ratesel_txs_t *rs_txs,
	tx_status_t *txs,
	uint16 tx_flags)
{
	rcb_t *state;

	state = wlc_scb_ratesel_get_cubby(wrsi, scb, rs_txs->ac);
	ASSERT(state);

	wlc_ratesel_upd_txs_ampdu(state, rs_txs, txs, tx_flags);
}
#endif /* WLAMPDU_MAC */

/* update state upon received BA */
void BCMFASTPATH
wlc_scb_ratesel_upd_txs_blockack(wlc_ratesel_info_t *wrsi, struct scb *scb, tx_status_t *txs,
	uint8 suc_mpdu, uint8 tot_mpdu, bool ba_lost, uint8 retry, uint8 fb_lim, bool tx_error,
	uint8 mcs, bool sgi, uint8 antselid, uint8 ac)
{
	rcb_t *state;

	state = wlc_scb_ratesel_get_cubby(wrsi, scb, ac);
	ASSERT(state);

	wlc_ratesel_upd_txs_blockack(state, txs, suc_mpdu, tot_mpdu, ba_lost, retry, fb_lim,
		tx_error, mcs, sgi, antselid);
}
#endif /* WL11N */

bool
wlc_scb_ratesel_minrate(wlc_ratesel_info_t *wrsi, struct scb *scb, tx_status_t *txs, uint8 ac)
{
	rcb_t *state;

	state = wlc_scb_ratesel_get_cubby(wrsi, scb, ac);
	ASSERT(state);

	return (wlc_ratesel_minrate(state, txs));
}

static void
wlc_scb_ratesel_ppr_filter(wlc_info_t *wlc, ppr_rateset_t *clm_rates,
	wlc_rateset_t *scb_rates, bool scb_VHT)
{
	uint8 i;

	for (i = 0; i < PHYCORENUM(wlc->stf->txstreams); i++) {
		scb_rates->mcs[i] &= clm_rates->mcs[i];
#ifdef WL11AC
		if (scb_VHT) {
			/* check VHT 8_9 */
			uint16 clm_vht_rate = VHT_MCS_MAP_GET_MCS_PER_SS(i+1,
				clm_rates->vht_mcsmap);
			uint16 scb_vht_rate = VHT_MCS_MAP_GET_MCS_PER_SS(i+1,
				scb_rates->vht_mcsmap);

			if (scb_vht_rate != VHT_CAP_MCS_MAP_NONE) {
				if (clm_vht_rate == VHT_CAP_MCS_MAP_NONE) {
					VHT_MCS_MAP_SET_MCS_PER_SS(i+1, VHT_CAP_MCS_MAP_NONE,
						scb_rates->vht_mcsmap);
				} else if (scb_vht_rate > clm_vht_rate) {
					VHT_MCS_MAP_SET_MCS_PER_SS(i+1, clm_vht_rate,
						scb_rates->vht_mcsmap);
				}
#ifndef NO_PROPRIETARY_VHT_RATES
				/* check VHT 10_11 */
				clm_vht_rate =
					VHT_MCS_MAP_GET_MCS_PER_SS(i+1,
						clm_rates->vht_mcsmap_prop);

				/* Proprietary rates map can be either
				 * VHT_PROP_MCS_MAP_10_11 or VHT_CAP_MCS_MAP_NONE,
				 * if clm_vht_rate is set to VHT_PROP_MCS_MAP_10_11,
				 * no action required.
				 */
				if (clm_vht_rate == VHT_PROP_MCS_MAP_NONE) {
					VHT_MCS_MAP_SET_MCS_PER_SS(i+1,
						VHT_PROP_MCS_MAP_NONE,
						scb_rates->vht_mcsmap_prop);
				}
#endif /* !NO_PROPRIETARY_VHT_RATES */
			}
		}
#endif /* WL11AC */
	}
}

#ifdef WL11AC
static uint8
wlc_scb_get_bw_from_scb_oper_mode(wlc_vht_info_t *vhti, struct scb *scb)
{
	uint8 bw = 0;
	uint8 mode = 0;
	mode = wlc_vht_get_scb_opermode(vhti, scb);
	mode &= DOT11_OPER_MODE_CHANNEL_WIDTH_MASK;
	if (mode == DOT11_OPER_MODE_20MHZ)
		bw = BW_20MHZ;
	else if (mode == DOT11_OPER_MODE_40MHZ)
		bw = BW_40MHZ;
	else if (mode == DOT11_OPER_MODE_80MHZ)
		bw = BW_80MHZ;
	else if (mode == DOT11_OPER_MODE_160MHZ)
		bw = BW_160MHZ;

	return bw;
}
#endif /* WL11AC */

static uint8
wlc_scb_ratesel_link_bw_upd(wlc_info_t *wlc, struct scb *scb)
{
	wlc_ratesel_info_t *wrsi = wlc->wrsi;
	rcb_t *rcb = SCB_RATESEL_CUBBY(wrsi, scb, LINK_BW_ENTRY);
	uint8 bw = BW_20MHZ;
#ifdef WL11AC
	uint8 scb_oper_mode_bw;
#endif
	chanspec_t chanspec = wlc->chanspec;

#ifdef WL11AC
	if (SCB_VHT_CAP(scb) &&
		CHSPEC_IS8080(chanspec) &&
		((scb->flags3 & SCB3_IS_80_80)))
		bw = BW_160MHZ;
	else if (SCB_VHT_CAP(scb) &&
		CHSPEC_IS160(chanspec) &&
		((scb->flags3 & SCB3_IS_160)))
		bw = BW_160MHZ;
	else if (CHSPEC_BW_GE(chanspec, WL_CHANSPEC_BW_80) &&
		SCB_VHT_CAP(scb))
		bw = BW_80MHZ;
	else
#endif /* WL11AC */
	if (((scb->flags & SCB_IS40)) &&
	    CHSPEC_BW_GE(chanspec, WL_CHANSPEC_BW_40))
		bw = BW_40MHZ;

	/* here bw derived from chanspec and capabilities */
#ifdef WL11AC
	/* process operating mode notification for channel bw */

	if ((SCB_HT_CAP(scb) || SCB_VHT_CAP(scb)) &&
		wlc_vht_get_scb_opermode_enab(wlc->vhti, scb) &&
		!DOT11_OPER_MODE_RXNSS_TYPE(wlc_vht_get_scb_opermode(wlc->vhti, scb))) {
		scb_oper_mode_bw = wlc_scb_get_bw_from_scb_oper_mode(wlc->vhti, scb);
		bw = (scb_oper_mode_bw < bw)?scb_oper_mode_bw:bw;
	}

#endif /* WL11AC */

	wlc_ratesel_set_link_bw(rcb, bw);
	return (bw);
}


#ifdef WL_MIMOPS_CFG
/* Filter out any rates above siso rates for the scb if current bsscfg txchain is siso
 * and is different than hw txchain, or if we are in MRC mode
 */
void wlc_scb_ratesel_siso_downgrade(wlc_info_t *wlc, struct scb *scb)
{
	int i = 0;
	bool siso = FALSE;
	wlc_hw_config_t *scb_bsscfg_hw_cfg = wlc_stf_bss_hw_cfg_get(scb->bsscfg);
	wlc_mimo_ps_cfg_t *scb_bsscfg_mimo_ps_cfg = wlc_stf_mimo_ps_cfg_get(scb->bsscfg);

	if (!(scb_bsscfg_hw_cfg && scb_bsscfg_mimo_ps_cfg))
		return;

	/* If MRC mode is enabled, we're really siso */
	if (scb_bsscfg_mimo_ps_cfg && scb_bsscfg_mimo_ps_cfg->mrc_chains_changed)
		siso = TRUE;

	/* If we're single-chain and need to override current HW setting */
	if ((WLC_BITSCNT(scb_bsscfg_hw_cfg->current_txchains) == 1) &&
		(WLC_BITSCNT(wlc->stf->txchain) != 1))
		siso = TRUE;

	/* Filter the rates if needed */
	if (siso) {
		if (SCB_HT_CAP(scb)) {
			for (i = 1; i < MCSSET_LEN; i++)
				scb->rateset.mcs[i] = 0;
		}
#ifdef WL11AC
		if (SCB_VHT_CAP(scb)) {
			scb->rateset.vht_mcsmap = 0xfffe;
		}
#endif
	}
}
#endif /* WL_MIMOPS_CFG */

/* initialize per-scb state utilized by rate selection
 *   ATTEN: this fcn can be called to "reinit", avoid dup MALLOC
 *   this new design makes this function the single entry points for any select_rates changes
 *   this function should be called when any its parameters changed: like bw or stream
 *   this function will build select_rspec[] with all constraint and rateselection will
 *      be operating on this constant array with reference to known_rspec[] for threshold
 */

void
wlc_scb_ratesel_init(wlc_info_t *wlc, struct scb *scb)
{
	wlc_ratesel_info_t *wrsi = wlc->wrsi;
	rcb_t *state;
	uint8 bw = BW_20MHZ;
	int8 sgi_tx = OFF;
	int8 ldpc_tx = OFF;
	int8 vht_ldpc_tx = OFF;
	uint8 active_antcfg_num = 0;
	uint8 antselid_init = 0;
	int32 ac;
	wlc_rateset_t new_rateset;
	ppr_rateset_t *clm_rateset;
	uint8 i;

	if (SCB_INTERNAL(scb))
		return;
#ifdef WL11N
	if (WLANTSEL_ENAB(wlc))
		wlc_antsel_ratesel(wlc->asi, &active_antcfg_num, &antselid_init);

	bw = wlc_scb_ratesel_link_bw_upd(wlc, scb);

	if (wlc->stf->ldpc_tx == AUTO) {
		if (bw != BW_80MHZ && SCB_LDPC_CAP(scb))
			ldpc_tx = AUTO;
#ifdef WL11AC
		if (SCB_VHT_LDPC_CAP(wlc->vhti, scb))
			vht_ldpc_tx = AUTO;
#endif /* WL11AC */
	} else if (wlc->stf->ldpc_tx == ON) {
		if (SCB_LDPC_CAP(scb))
			ldpc_tx = ON;
		if (SCB_VHT_LDPC_CAP(wlc->vhti, scb))
			vht_ldpc_tx = ON;
	}

	if (WLC_HT_GET_SGI_TX(wlc->hti) == AUTO) {
		if (scb->flags2 & SCB2_SGI20_CAP)
			sgi_tx |= SGI_BW20;
		if (bw >= BW_40MHZ && (scb->flags2 & SCB2_SGI40_CAP))
			sgi_tx |= SGI_BW40;
		if (bw >= BW_80MHZ && SCB_VHT_SGI80(wlc->vhti, scb))
			sgi_tx |= SGI_BW80;
		/* Disable SGI Tx in 20MHz on IPA chips */
		if (bw == BW_20MHZ && wlc->stf->ipaon)
			sgi_tx = OFF;
	}
#endif /* WL11N */

#ifdef WL11AC
	/* Set up the mcsmap in scb->rateset.vht_mcsmap */
	if (SCB_VHT_CAP(scb))
	{
		if (wlc->stf->txstream_value == 0) {
			wlc_vht_upd_rate_mcsmap(wlc->vhti, scb);
		} else  {
			/* vht rate override
			* for 3 stream the value 0x 11 11 11 11 1110 10 10
			* for 2 stream the value 0x 11 11 11 11 11 11 1010
			* for 1 stream the value 0x 11 11 11 11 11 11 1110
			*/
			if (wlc->stf->txstream_value == 2) {
				scb->rateset.vht_mcsmap = 0xfffa;
			} else if (wlc->stf->txstream_value == 1) {
				scb->rateset.vht_mcsmap = 0xfffe;
			}
		}
	}
#endif /* WL11AC */

	/* HT rate overide for BTCOEX */
	if ((SCB_HT_CAP(scb) && wlc->stf->txstream_value)) {
		for (i = 1; i < 4; i++) {
			if (i >= wlc->stf->txstream_value) {
				scb->rateset.mcs[i] = 0;
			}
		}
#if defined(WLPROPRIETARY_11N_RATES)
		for (i = WLC_11N_FIRST_PROP_MCS; i <= WLC_MAXMCS; i++) {
			if (GET_PROPRIETARY_11N_MCS_NSS(i) > wlc->stf->txstream_value)
				clrbit(scb->rateset.mcs, i);
		}
#endif /* WLPROPRIETARY_11N_RATES */
	}
#ifdef WL_MIMOPS_CFG
	if (WLC_MIMOPS_ENAB(wlc->pub)) {
		wlc_scb_ratesel_siso_downgrade(wlc, scb);
	}
#endif /* WL_MIMOPS_CFG */

	for (ac = 0; ac < WME_MAX_AC(wlc, scb); ac++) {
		uint8 vht_ratemask = 0;
		uint32 max_rate;
		state = SCB_RATESEL_CUBBY(wrsi, scb, ac);

		if (state == NULL) {
			ASSERT(0);
			return;
		}

		bcopy(&scb->rateset, &new_rateset, sizeof(wlc_rateset_t));

#ifdef WL11N
		if (BSS_N_ENAB(wlc, scb->bsscfg)) {
			if (((WLC_HT_GET_SCB_MIMOPS_ENAB(wlc->hti, scb) &&
				!WLC_HT_GET_SCB_MIMOPS_RTS_ENAB(wlc->hti, scb)) ||
				(wlc->stf->txstreams == 1) || (wlc->stf->siso_tx == 1))) {
				new_rateset.mcs[1] = 0;
				new_rateset.mcs[2] = 0;
			} else if (wlc->stf->txstreams == 2)
				new_rateset.mcs[2] = 0;
		}
#endif

#ifdef WL11AC
		vht_ratemask = wlc_vht_get_scb_ratemask_per_band(wlc->vhti, scb);
#endif
		WL_RATE(("%s: scb 0x%p ac %d state 0x%p bw %s txstreams %d"
			" active_ant %d band %d vht:%u vht_rm:0x%x\n",
			__FUNCTION__, OSL_OBFUSCATE_BUF(scb), ac,
			OSL_OBFUSCATE_BUF(state), (bw == BW_20MHZ) ?
			"20" : ((bw == BW_40MHZ) ? "40" :
			(bw == BW_80MHZ) ? "80" : "80+80/160"),
			wlc->stf->txstreams, active_antcfg_num,
			wlc->band->bandtype, SCB_VHT_CAP(scb), vht_ratemask));

		max_rate = 0;
#if defined(WME_PER_AC_TX_PARAMS)
		if (WME_PER_AC_MAXRATE_ENAB(wrsi->pub) && SCB_WME(scb))
			max_rate = (uint32)wrsi->wlc->wme_max_rate[ac];
#endif
		clm_rateset = wlc_scb_ratesel_get_ppr_rates(wlc, rspecbw_to_bcmbw(bw));
		if (clm_rateset)
			wlc_scb_ratesel_ppr_filter(wlc, clm_rateset, &new_rateset,
					SCB_VHT_CAP(scb));

		wlc_ratesel_init(wrsi->rsi, state, scb, &new_rateset, bw, sgi_tx, ldpc_tx,
			vht_ldpc_tx, vht_ratemask, active_antcfg_num, antselid_init, max_rate, 0);
	}

#ifdef WL_LPC
	wlc_scb_lpc_init(wlc->wlpci, scb);
#endif
}

void
wlc_scb_ratesel_init_all(wlc_info_t *wlc)
{
	struct scb *scb;
	struct scb_iter scbiter;

	FOREACHSCB(wlc->scbstate, &scbiter, scb)
		wlc_scb_ratesel_init(wlc, scb);

#ifdef WL_LPC
	wlc_scb_lpc_init_all(wlc->wlpci);
#endif
}

void
wlc_scb_ratesel_init_bss(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	struct scb *scb;
	struct scb_iter scbiter;

	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
		wlc_scb_ratesel_init(wlc, scb);
	}
#ifdef WL_LPC
	wlc_scb_lpc_init_bss(wlc->wlpci, cfg);
#endif
}

void
wlc_scb_ratesel_rfbr(wlc_ratesel_info_t *wrsi, struct scb *scb, uint8 ac)
{
	rcb_t *state;

	state = wlc_scb_ratesel_get_cubby(wrsi, scb, ac);
	ASSERT(state);

	wlc_ratesel_rfbr(state);
}

void
wlc_scb_ratesel_rfbr_bss(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	struct scb *scb;
	struct scb_iter scbiter;
	rcb_t *state;
	int32 ac;
	wlc_ratesel_info_t *wrsi = wlc->wrsi;

	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
		for (ac = 0; ac < WME_MAX_AC(wlc, scb); ac++) {
			state = SCB_RATESEL_CUBBY(wrsi, scb, ac);
			ASSERT(state);

			wlc_ratesel_rfbr(state);
		}
	}
}

static int wlc_scb_ratesel_cubby_sz(void)
{
	return (sizeof(struct ratesel_cubby));
}

#ifdef WL11N
void wlc_scb_ratesel_rssi_enable(rssi_ctx_t *ctx)
{
	struct scb *scb = (struct scb *)ctx;

	wlc_lq_sample_req_enab(scb, RX_LQ_SAMP_REQ_RATE_SEL, TRUE);
}

void wlc_scb_ratesel_rssi_disable(rssi_ctx_t *ctx)
{
	struct scb *scb = (struct scb *)ctx;

	wlc_lq_sample_req_enab(scb, RX_LQ_SAMP_REQ_RATE_SEL, FALSE);
}

int wlc_scb_ratesel_get_rssi(rssi_ctx_t *ctx)
{
	struct scb *scb = (struct scb *)ctx;

	wlc_bsscfg_t *cfg = SCB_BSSCFG(scb);
	wlc_info_t *wlc = cfg->wlc;
	return wlc_lq_rssi_get(wlc, cfg, scb);
}
#endif /* WL11N */

#ifdef WL_LPC
/* External functions */
void
wlc_scb_ratesel_get_info(wlc_ratesel_info_t *wrsi, struct scb *scb, uint8 ac,
	uint8 rate_stab_thresh, uint32 *new_rate_kbps, bool *rate_stable,
	rate_lcb_info_t *lcb_info)
{
	rcb_t *state = wlc_scb_ratesel_get_cubby(wrsi, scb, ac);
	wlc_ratesel_get_info(state, rate_stab_thresh, new_rate_kbps, rate_stable, lcb_info);
	return;
}

void
wlc_scb_ratesel_reset_vals(wlc_ratesel_info_t *wrsi, struct scb *scb, uint8 ac)
{
	rcb_t *state = NULL;

	if (!scb)
		return;

	state = SCB_RATESEL_CUBBY(wrsi, scb, ac);
	wlc_ratesel_lpc_init(state);
	return;
}

void
wlc_scb_ratesel_clr_cache(wlc_ratesel_info_t *wrsi, struct scb *scb, uint8 ac)
{
	rcb_t *state = wlc_scb_ratesel_get_cubby(wrsi, scb, ac);
	wlc_ratesel_clr_cache(state);
	return;
}
#endif /* WL_LPC */

/* Get current CLM enabled rates bitmap */
static ppr_rateset_t *
wlc_scb_ratesel_get_ppr_rates(wlc_info_t *wlc, wl_tx_bw_t bw)
{
#ifdef WL11AC
	wlc_ratesel_info_t *wrsi = wlc->wrsi;
	if (wrsi->ppr_rates->chanspec != wlc->chanspec ||
		wrsi->ppr_rates->country != wlc_get_country(wlc)) {
		wlc_scb_ratesel_ppr_upd(wlc);
	}

	switch (bw) {
	case WL_TX_BW_20:
		return &wrsi->ppr_rates->ppr_20_rates;
	case WL_TX_BW_40:
		return &wrsi->ppr_rates->ppr_40_rates;
	case WL_TX_BW_80:
		return &wrsi->ppr_rates->ppr_80_rates;
	case WL_TX_BW_160:
		return &wrsi->ppr_rates->ppr_160_rates;
	default:
		ASSERT(0);
		return NULL;
	}
#else
	return NULL;
#endif /* WL11AC */
}

static void
wlc_scb_ratesel_get_ppr_rates_bitmp(wlc_info_t *wlc, ppr_t *target_pwrs, wl_tx_bw_t bw,
	ppr_rateset_t *rates)
{
	uint8 chain;
	ppr_vht_mcs_rateset_t mcs_limits;

	rates->vht_mcsmap = VHT_CAP_MCS_MAP_NONE_ALL;
	rates->vht_mcsmap_prop = VHT_PROP_MCS_MAP_NONE_ALL;
	for (chain = 0; chain < PHYCORENUM(wlc->stf->txstreams); chain++) {
		ppr_get_vht_mcs(target_pwrs, bw, chain+1, WL_TX_MODE_NONE, chain+1, &mcs_limits);
		if (mcs_limits.pwr[0] != WL_RATE_DISABLED) {
			rates->mcs[chain] = 0xff; /* All rates are enabled for this block */
			/* Check VHT rate [8-9] */
#ifdef WL11AC
			if (WLCISACPHY(wlc->band)) {
				if (mcs_limits.pwr[9] != WL_RATE_DISABLED) {
					/* All VHT rates are enabled */
					VHT_MCS_MAP_SET_MCS_PER_SS(chain+1, VHT_CAP_MCS_MAP_0_9,
						rates->vht_mcsmap);
				} else if (mcs_limits.pwr[8] != WL_RATE_DISABLED) {
					/* VHT 0_8 are enabled */
					VHT_MCS_MAP_SET_MCS_PER_SS(chain+1, VHT_CAP_MCS_MAP_0_8,
						rates->vht_mcsmap);
				} else {
					/* VHT 8-9 are disabled in this case */
					VHT_MCS_MAP_SET_MCS_PER_SS(chain+1, VHT_CAP_MCS_MAP_0_7,
						rates->vht_mcsmap);
				}
#ifndef NO_PROPRIETARY_VHT_RATES
				/* Check VHT 10_11 */
				if (mcs_limits.pwr[11] != WL_RATE_DISABLED) {
					/* Both VHT 10_11 are enabled */
					VHT_MCS_MAP_SET_MCS_PER_SS(chain+1,
						VHT_PROP_MCS_MAP_10_11,
						rates->vht_mcsmap_prop);
				} else {
					/* VHT 10_11 are disabled in this case */
					VHT_MCS_MAP_SET_MCS_PER_SS(chain+1,
						VHT_CAP_MCS_MAP_NONE,
						rates->vht_mcsmap_prop);
				}
#endif /* !NO_PROPRIETARY_VHT_RATES */
			}
#endif /* WL11AC */
		}
	}

}

static void
wlc_scb_ratesel_ppr_updbmp(wlc_info_t *wlc, ppr_t *target_pwrs)
{
	wlc_ratesel_info_t *wrsi = wlc->wrsi;
	wlc_scb_ratesel_get_ppr_rates_bitmp(wlc, target_pwrs, WL_TX_BW_20,
		&wrsi->ppr_rates->ppr_20_rates);
#if defined(WL11N) || defined(WL11AC)
	if (CHSPEC_IS40(wlc->chanspec)) {
		wlc_scb_ratesel_get_ppr_rates_bitmp(wlc, target_pwrs, WL_TX_BW_40,
			&wrsi->ppr_rates->ppr_40_rates);
	}
#endif
#if defined(WL11AC)
	if (CHSPEC_IS80(wlc->chanspec)) {
		wlc_scb_ratesel_get_ppr_rates_bitmp(wlc, target_pwrs, WL_TX_BW_80,
			&wrsi->ppr_rates->ppr_80_rates);
	}
	if (CHSPEC_IS160(wlc->chanspec)) {
		wlc_scb_ratesel_get_ppr_rates_bitmp(wlc, target_pwrs, WL_TX_BW_160,
			&wrsi->ppr_rates->ppr_160_rates);
	}
#endif
}

/* Update ppr enabled rates bitmap */
extern void
wlc_scb_ratesel_ppr_upd(wlc_info_t *wlc)
{
	phy_tx_power_t power;
	wl_tx_bw_t ppr_bw;
	ppr_t* reg_limits = NULL;
	int ret;

	wlc_cm_info_t *wlc_cm = wlc->cmi;
	clm_country_t country = wlc_get_country(wlc);


	bzero(&power, sizeof(power));

	ppr_bw = ppr_get_max_bw();
	if ((power.ppr_target_powers = ppr_create(wlc->osh, ppr_bw)) == NULL) {
		goto free_power;
	}
	if ((power.ppr_board_limits = ppr_create(wlc->osh, ppr_bw)) == NULL) {
		goto free_power;
	}
	if ((reg_limits = ppr_create(wlc->osh, PPR_CHSPEC_BW(wlc->chanspec))) == NULL) {
		goto free_power;
	}
	wlc_channel_reg_limits(wlc_cm, wlc->chanspec, reg_limits);

	if ((ret = wlc_phy_txpower_get_current(WLC_PI(wlc),
		reg_limits, &power)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: PHY func fail Return value = %d\n",
			wlc->pub->unit,
			__FUNCTION__, ret));
		goto free_power;
	}

	/* update the rate bitmap with retrieved target power */
	wlc_scb_ratesel_ppr_updbmp(wlc, power.ppr_target_powers);

	wlc->wrsi->ppr_rates->country = country;
	wlc->wrsi->ppr_rates->chanspec = wlc->chanspec;
free_power:
	if (power.ppr_board_limits)
		ppr_delete(wlc->osh, power.ppr_board_limits);
	if (power.ppr_target_powers)
		ppr_delete(wlc->osh, power.ppr_target_powers);
	if (reg_limits)
		ppr_delete(wlc->osh, reg_limits);
}

#ifdef WLATF
/* Get the rate selection control block pointer from ratesel cubby */
rcb_t *
wlc_scb_ratesel_getrcb(wlc_info_t *wlc, struct scb *scb, uint ac)
{
	wlc_ratesel_info_t *wrsi = wlc->wrsi;
	ASSERT(wrsi);

	if (!WME_PER_AC_MAXRATE_ENAB(wlc->pub)) {
		ac = 0;
	}
	ASSERT(ac < (uint)WME_MAX_AC(wlc, scb));

	return (SCB_RATESEL_CUBBY(wrsi, scb, ac));
}

#endif /* WLATF */

static wl_tx_bw_t rspecbw_to_bcmbw(uint8 bw)
{
	wl_tx_bw_t bcmbw = WL_TX_BW_20;
	switch (bw) {
	case BW_40MHZ:
		bcmbw = WL_TX_BW_40;
		break;
	case BW_80MHZ:
		bcmbw = WL_TX_BW_80;
		break;
	case BW_160MHZ:
		bcmbw = WL_TX_BW_160;
		break;
	}

	return bcmbw;
}
#ifdef WLRSDB
/* Clear the wrsi->ppr_rates->chanspec
 * variable to force ppr update after mode switch.
 */
void wlc_scb_ratesel_chanspec_clear(wlc_info_t *wlc)
{
	wlc->wrsi->ppr_rates->chanspec = INVCHANSPEC;
	return;
}
#endif /* WLRSDB */

#ifdef WL_FRAGDUR
/* get correct fbr for tx duration calculation for frag_dur */
ratespec_t
wlc_scb_ratesel_getfbrspec_fragdur(wlc_info_t *wlc, struct scb *scb, uint8 ac)
{
	rcb_t *state;
	ratespec_t fbrrspec;
	ratespec_t rspec;
	wlc_bsscfg_t *bsscfg;
	bool g_prot;
	int8 n_prot;
	wlc_ratesel_info_t *wrsi = wlc->wrsi;

	state = SCB_RATESEL_CUBBY(wrsi, scb, ac);
	/* If this is broadcast/multicast (e.g. AWDL, but never Infra) then return a dummy rate.
	 *  Anyway a later check will prevent these from being fragmented.
	 */
	if (state == NULL)
		return (CCK_RSPEC(WLC_RATE_1M));

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);
	g_prot = WLC_PROT_G_CFG_G(wlc->prot_g, bsscfg);
	n_prot = WLC_PROT_N_CFG_N(wlc->prot_n, bsscfg);

	rspec = wlc_ratesel_getcurspec(state);

	if ((wlc->band->gmode && g_prot && RSPEC_ISOFDM(rspec))) {
		fbrrspec = CCK_RSPEC(WLC_RATE_FRAG_G_PROTECT);
	} else if (N_ENAB(wlc->pub) && IS_MCS(rspec) && n_prot) {
		fbrrspec = OFDM_RSPEC(WLC_RATE_FRAG_N_PROTECT);
	} else {

		if (RSPEC_ACTIVE(wlc->band->rspec_override)) {
			fbrrspec = wlc->band->rspec_override;
		} else {
			fbrrspec = wlc_ratesel_getfbrspec(state);
		}
	}
	return fbrrspec;
}

bool
wlc_scb_ratesel_check_legacy_rspecs(wlc_info_t *wlc, struct scb *scb, uint8 ac)
{
	rcb_t *state;
	ratespec_t fbrrspec;
	ratespec_t currspec;
	bool res;
	wlc_ratesel_info_t *wrsi = wlc->wrsi;

	if (wlc->band->rspec_override) {
		res = (!IS_MCS(wlc->band->rspec_override));
	} else {
		state = SCB_RATESEL_CUBBY(wrsi, scb, ac);
		if (!state)
			return FALSE;
		fbrrspec = wlc_ratesel_getfbrspec(state);
		currspec = wlc_ratesel_getcurspec(state);
		res = (!IS_MCS(currspec)) && (!IS_MCS(fbrrspec));
	}
	return (res);
}

/* return TRUE if the current rate id is higher than or equal to
 * the rate id for tx aggregation threshold
 */
bool BCMFASTPATH
wlc_scb_ratesel_hit_txaggthreshold(wlc_ratesel_info_t *wrsi, struct scb *scb)
{
	wlc_info_t *wlc = wrsi->wlc;
	rcb_t *state;
	uint8 ac;

	(void)wlc;

	for (ac = 0; ac < WME_MAX_AC(wlc, scb); ac++) {
		state = SCB_RATESEL_CUBBY(wrsi, scb, ac);
		if (state && wlc_ratesel_hit_txaggthreshold(state)) {
			return TRUE;
		}
	}

	return FALSE;
}

/* calculate tx aggregation threshold */
int
wlc_scb_ratesel_calc_txaggthreshold(wlc_ratesel_info_t *wrsi, struct scb *scb)
{
	wlc_info_t *wlc = wrsi->wlc;
	rcb_t *state;
	int status = BCME_OK;
	uint8 ac;

	(void)wlc;

	for (ac = 0; ac < WME_MAX_AC(wlc, scb); ac++) {
		state = SCB_RATESEL_CUBBY(wrsi, scb, ac);
		if (state) {
			wlc_ratesel_calc_txaggthreshold(wlc, state);
		} else {
			status = BCME_ERROR;
			break;
		}
	}

	return status;
}
#endif /* WL_FRAGDUR */

uint8
wlc_scb_ratesel_get_link_bw(wlc_info_t *wlc, struct scb *scb)
{
	wlc_ratesel_info_t *wrsi = wlc->wrsi;
	rcb_t *state = SCB_RATESEL_CUBBY(wrsi, scb, LINK_BW_ENTRY);
	return wlc_ratesel_get_link_bw(state);
}
