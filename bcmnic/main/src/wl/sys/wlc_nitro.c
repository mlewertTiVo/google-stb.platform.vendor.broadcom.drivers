/*
 * Nitro protocol implementation
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_nitro.c 544768 2015-03-28 17:06:27Z $
 */


#include <wlc_cfg.h>

#error "WLNINTENDO is not defined"
#ifndef AP
#error "AP is not defined"
#endif	/* AP */
#ifndef STA
#error "STA is not defined"
#endif	/* STA */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <proto/802.11.h>
#include <wlioctl.h>

#include <sbhndpio.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_channel.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_hw.h>
#include <wlc_scb.h>
#include <wlc_frmutil.h>
#include <wlc_ap.h>
#include <wl_export.h>

#include <wlc_nitro.h>
#include <nintendowm.h>
#include <bcmdefs.h>

#include "wlc_vndr_ie_list.h"
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_ie_mgmt_vs.h>

#include <wlc_tx.h>

/* prototypes */
extern uint wlc_get_rxratehistory(wlc_info_t *wlc, struct scb * scb);

#define NITRO_MODE_PARENT       1    /* Parent */
#define NITRO_MODE_CHILD        2    /* Child */

#define NITRO_GAMEINFO_MAX      128	 /* Max length of game info */
#define NITRO_SSIDMASK_SIZE     32   /* Max length of ssid mask (byte) */
#define NITRO_ACTIVEZONE_MIN    0xa  /* Minimum value for active zone */
#define NITRO_ACTIVEZONE_MAX    0xFFFF /* Maximum value for active zone */
#define NITRO_ACTIVEZONE_DEF    0xFFFF /* Default value for active zone */

#define NITRO_MAX_QLEN          2    /* Max no. of frames in TXQ */
#define NITRO_RATE              WLC_RATE_2M
#define MP_FRMTYPE              11   /* Nitro MP Frame type/subtype */
#define KEYRESP_FRMTYPE         7    /* Nitro Key Response Frame type/subtype */

#define NITRO_BYTETIME_2M       (4)  /* time to transmit a byte at 2Mbps */
/* Null Keydata is 28 bytes */
#define NITRO_NULLKEYDATA_DUR   (BPHY_PLCP_SHORT_TIME + (NITRO_BYTETIME_2M * 28))
/* MPACK is 32 bytes */
#define NITRO_MPACK_DUR	        (BPHY_PLCP_SHORT_TIME + (NITRO_BYTETIME_2M * 32))

#define NITRO_MODE(wlc)         (wlc->_nitro)

/* Convert txop from datalength to time(us). */
#define NITRO_TXOP_CONV(len) \
	(NITRO_NULLKEYDATA_DUR + (len * NITRO_BYTETIME_2M))

/* Parent: Information related to the MP sequence */
typedef struct wlc_nitro_mp {
	wl_mpreq_t    mpreq;               /* cache of mpreq structure */
	uint8         mpdata[NITRO_MP_MAXLEN];    /* cache of the mpreq data */
	uint16        rxbitmap;            /* sw bitmap, updated when key response recvd */
	uint16        ucode_rxbitmap;      /* Rx bitmap from ucode, valid if bit 0 is set */
	bool          mpbusy;              /* MP request outstanding */
} wlc_nitro_mp_t;

/* Nitro protocol specific info */
struct wlc_nitro {
	wlc_info_t          *wlc;
	wlc_nitro_cnt_t     stats;           /* Nitro Statistics */
	uint8               ssid_mask[NITRO_SSIDMASK_SIZE];
	uint16              seqnum;        /* Seq num of last MP frame (Parent: tx; Child: rx) */

	/* Fields used in Parent mode */
	void                *mpend;        /* MPEND response frame */
	wlc_nitro_mp_t      mp;            /* Current/Last MP request data */
	uint8               gameInfoLen;   /* Length of gameInfo buffer */
	nitro_gameIE_t      *gameIE;       /* game information element */
	vndr_ie_listel_t    *gameIE_list_el; /* save ptr to gameIE vndr_ie list element */
	struct wl_timer          *timer;

	/* Fields used in Child mode */
	uint16              keytxsts;      /* save info from MP ind; copy to MPACK ind */
};

/* iovar table */
enum {
	IOV_NITRO_MODE,          /* enable/disable nitro mode */
	IOV_NITRO_MPREQ,         /* start Mp sequence */
	IOV_NITRO_KEYDATAREQ,    /* send keydata request */
	IOV_NITRO_SSIDMASK,      /* SSID mask */
	IOV_NITRO_ACTIVEZONE,    /* Active zone */
	IOV_NITRO_VBLANKTSF,     /* vblank tsf */
	IOV_NITRO_GAMEINFO,      /* Game information */
	IOV_NITRO_COUNTERS,      /* Get Nitro protocol related counters */
	IOV_NITRO_CLEAR_COUNTERS, /* Clear Nitro related counters */
	IOV_RXRATE,				/* Get per-scb rx info */
	IOV_TXRATE				/* Get per-scb tx info */
};

static const bcm_iovar_t nitro_iovars[] = {
	{"nitro", IOV_NITRO_MODE, (IOVF_SET_DOWN), IOVT_BOOL, 0},
	{"mpreq", IOV_NITRO_MPREQ, (IOVF_SET_UP), IOVT_BUFFER, WL_MPREQ_FIXEDLEN},
	{"keydatareq", IOV_NITRO_KEYDATAREQ, (IOVF_SET_UP), IOVT_BUFFER, WL_KEYDATAREQ_FIXEDLEN},
	{"ssidmask", IOV_NITRO_SSIDMASK, (0), IOVT_BUFFER, NITRO_SSIDMASK_SIZE},
	{"activezone", IOV_NITRO_ACTIVEZONE, (0), IOVT_UINT16, 0},
	{"vblanktsf", IOV_NITRO_VBLANKTSF, (0), IOVT_UINT16, 0},
	{"gameinfo", IOV_NITRO_GAMEINFO, (0), IOVT_BUFFER, 0},
	{"nitro_counters", IOV_NITRO_COUNTERS, (0), IOVT_BUFFER, sizeof(wlc_nitro_cnt_t)},
	{"nitro_clear_counters", IOV_NITRO_CLEAR_COUNTERS, 0, IOVT_VOID, 0},
	{"rxrate", IOV_RXRATE, 0, IOVT_BUFFER, sizeof(struct ether_addr) },
	{"txrate", IOV_TXRATE, 0, IOVT_UINT32, 0 },
	{NULL, 0, 0, 0, 0}
};

static int wlc_nitro_iovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *p, uint plen, void *a, int alen, int vsize, struct wlc_if *wlcif);
static int wlc_nitro_keydatareq(wlc_nitro_t *nitro, wl_keydatareq_t *keyreq, int len);
static int wlc_nitro_mpreq(wlc_nitro_t *nitro, wl_mpreq_t *mpreq, int len);
static int wlc_nitro_d11hdr(wlc_nitro_t *nitro, void *p, struct dot11_header *h,
	uint16 dur, bool immed_ack, bool sw_seq, d11txh_t **txhp);
static int wlc_nitro_create_mpend(wlc_nitro_t *nitro, wl_mpreq_t *mpreq, int poll_cnt);
static void wlc_nitro_mpend(wlc_nitro_t *nitro, uint16 rxbitmap, uint retries);
static void wlc_nitro_sendup(wlc_info_t *wlc, void *p);
static void wlc_nitro_rx_parent(wlc_nitro_t *nitro, struct scb *scb,
	struct wlc_frminfo *f, int8 rssi, uint rate);
static void wlc_nitro_rx_child(wlc_nitro_t *nitro, struct scb *scb,
	struct wlc_frminfo *f, int8 rssi, uint rate, wlc_d11rxhdr_t *wrxh);
static void wlc_nitro_shm_ssidmask_upd(wlc_info_t *wlc, uint8 *mask);
static nitro_gameIE_t *wlc_nitro_ie_create(wlc_info_t *wlc);
static int wlc_nitro_vndrie_update(wlc_nitro_t *nitro);
static int wlc_nitro_vndrie_delete(wlc_nitro_t *nitro);
static int wlc_nitro_down(void *hdl);
static void wlc_nitro_timer(void *hdl);

/* IE mgmt callbacks */
static int wlc_nitro_bcn_parse_nitro_ie(void *ctx, wlc_iem_parse_data_t *data);

wlc_nitro_t *
BCMATTACHFN(wlc_nitro_attach)(wlc_info_t *wlc)
{
	wlc_nitro_t *nitro;

	if (!(nitro = (wlc_nitro_t *)MALLOCZ(wlc->osh, sizeof(wlc_nitro_t)))) {
		WL_ERROR(("wl%d: wlc_nitro_attach: out of mem, malloced %d bytes\n",
			wlc->pub->unit, MALLOCED(wlc->osh)));
		return NULL;
	}

	nitro->wlc = wlc;

	/* Initialize ssid mask to default */
	memset(nitro->ssid_mask, 0xff, NITRO_SSIDMASK_SIZE);

	/* Initialize gameIE */
	nitro->gameIE = wlc_nitro_ie_create(wlc);

	/* register module */
	if (wlc_module_register(wlc->pub, nitro_iovars, "nitro", nitro, wlc_nitro_iovar,
	                        NULL, NULL, wlc_nitro_down)) {
		WL_ERROR(("wl%d: %s wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		return NULL;
	}

	wlc_iem_vs_add_parse_fn(wlc->iemi, FC_BEACON, WLC_IEM_VS_IE_PRIO_NITRO,
	                        wlc_nitro_bcn_parse_nitro_ie, nitro);


	nitro->timer = wl_init_timer(wlc->wl, wlc_nitro_timer, nitro, "nitromptmr");

	return nitro;
}

void
BCMATTACHFN(wlc_nitro_detach)(wlc_nitro_t *nitro)
{
	wlc_pub_t *pub = nitro->wlc->pub;

	if (!nitro)
		return;

	wlc_module_unregister(pub, "nitro", nitro);
	MFREE(pub->osh, nitro->gameIE, (NITRO_GAMEIE_FIXEDLEN + NITRO_GAMEINFO_MAX));
	MFREE(pub->osh, nitro, sizeof(wlc_nitro_t));
}

static int
wlc_nitro_down(void *hdl)
{
	wlc_nitro_t *nitro = (wlc_nitro_t *)hdl;
	osl_t *osh = nitro->wlc->osh;

	if (nitro->mp.mpbusy)
		nitro->mp.mpbusy = FALSE;
	if (nitro->mpend) {
		PKTFREE(osh, nitro->mpend, FALSE);
		nitro->mpend = NULL;
	}
	return 0;
}

static int
wlc_nitro_iovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid, const char *name,
        void *p, uint plen, void *a, int alen, int vsize, struct wlc_if *wlcif)
{
	wlc_nitro_t *nitro = (wlc_nitro_t *)hdl;
	int32 int_val = 0, i;
	bool bool_val;
	wlc_info_t *wlc = nitro->wlc;
	wlc_bsscfg_t *bsscfg;
	uint16 gameinfo_len;
	int32 *ret_int_ptr = (int32 *)a;

	/* update bsscfg w/provided interface context */
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);

	if (plen >= (int)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));
	bool_val = (int_val != 0) ? TRUE : FALSE;

	switch (actionid) {
	case IOV_GVAL(IOV_NITRO_MODE):
		*ret_int_ptr = wlc->_nitro;
		break;

	case IOV_SVAL(IOV_NITRO_MODE):
	{
		int wasap = 0;
		int isap = (int_val == NITRO_MODE_PARENT) ? 1 : 0;

		wlc_get(wlc, WLC_GET_AP, &wasap);
		wlc_set(wlc, WLC_SET_INFRA, 1);

		wlc->_nitro = (uint8)int_val;	/* set parent/child mode */
		wlc_set(wlc, WLC_SET_AP, isap);

		/* WLC_SET_AP reinits bsscfg info for these transitions:
		 * ap --> sta
		 * ap -->sta
		 */
		if (wasap != isap)
			nitro->gameIE_list_el = NULL;

		/* If we're in a nitro mode */
		/* Update nitro ie in beacons/prb rsp */
		if (int_val)
			wlc_nitro_vndrie_update(nitro);
		else if (nitro->gameIE_list_el)
			wlc_nitro_vndrie_delete(nitro);

		break;
	}

	case IOV_SVAL(IOV_NITRO_MPREQ):
		return (wlc_nitro_mpreq(nitro, (wl_mpreq_t *)a, alen));

	case IOV_SVAL(IOV_NITRO_KEYDATAREQ):
	{
		wl_keydatareq_t aligned_arg;

		bcopy(a, &aligned_arg, alen);
		return (wlc_nitro_keydatareq(nitro, &aligned_arg, alen));
	}

	case IOV_GVAL(IOV_NITRO_SSIDMASK):
	{
		uint8 *mask = a;

		for (i = 0; i < NITRO_SSIDMASK_SIZE; i++)
			mask[i] = ~nitro->ssid_mask[i];
		break;
	}

	case IOV_SVAL(IOV_NITRO_SSIDMASK):
	{
		uint8 *mask = a;

		for (i = 0; i < NITRO_SSIDMASK_SIZE; i++)
			nitro->ssid_mask[i] = ~mask[i];
		if (wlc->clk) {
			wlc_nitro_shm_ssidmask_upd(wlc, nitro->ssid_mask);
			wlc_update_probe_resp(wlc, TRUE);
		}
		break;
	}

	case IOV_GVAL(IOV_NITRO_ACTIVEZONE):
		*ret_int_ptr = ltoh16(nitro->gameIE->activeZone);
		break;

	case IOV_SVAL(IOV_NITRO_ACTIVEZONE):
		if (!WLNITRO_ENAB(wlc))
			return BCME_BADARG;
		if (int_val < NITRO_ACTIVEZONE_MIN || int_val > NITRO_ACTIVEZONE_MAX)
			return BCME_BADARG;

		nitro->gameIE->activeZone = htol16(int_val);
		return wlc_nitro_vndrie_update(nitro);
		break;

	case IOV_GVAL(IOV_NITRO_VBLANKTSF):
		*ret_int_ptr = ltoh16(nitro->gameIE->vblankTsf);
		break;

	case IOV_SVAL(IOV_NITRO_VBLANKTSF):
		if (!WLNITRO_ENAB(wlc))
			return BCME_BADARG;

		nitro->gameIE->vblankTsf = htol16(int_val);
		return wlc_nitro_vndrie_update(nitro);
		break;

	case IOV_GVAL(IOV_NITRO_GAMEINFO):
		if (alen < nitro->gameInfoLen)
			return BCME_BUFTOOSHORT;

		gameinfo_len = nitro->gameInfoLen;
		bcopy(&gameinfo_len, a, sizeof(uint16));
		if (nitro->gameInfoLen) {
			a = (char *)a + sizeof(uint16);
			bcopy(nitro->gameIE->gameInfo, a, nitro->gameInfoLen);
		}
		break;

	case IOV_SVAL(IOV_NITRO_GAMEINFO):
		if (!WLNITRO_ENAB(wlc))
			return BCME_BADARG;
		if (alen > (NITRO_GAMEINFO_MAX + sizeof(uint16)))
			return BCME_BUFTOOLONG;
		if (alen < sizeof(uint16))
			return BCME_BUFTOOSHORT;

		bcopy(a, &gameinfo_len, sizeof(uint16));
		if (gameinfo_len) {
			a = (char *)a + sizeof(uint16);
			bcopy(a, nitro->gameIE->gameInfo, gameinfo_len);
		}

		/* fixup the variable length field in game IE */
		nitro->gameIE->len -= nitro->gameInfoLen;
		nitro->gameInfoLen = gameinfo_len;
		nitro->gameIE->len += nitro->gameInfoLen;

		return wlc_nitro_vndrie_update(nitro);
		break;

	case IOV_GVAL(IOV_NITRO_COUNTERS):
		bcopy(&nitro->stats, a, sizeof(nitro->stats));
		break;

	case IOV_SVAL(IOV_NITRO_CLEAR_COUNTERS):
		bzero(&nitro->stats, sizeof(nitro->stats));
		break;

	case IOV_GVAL(IOV_RXRATE):
	{
		struct ether_addr *ea = (struct ether_addr *)p;
		struct scb *scb;
		uint32 temp;

		scb = wlc_scbfind(wlc, bsscfg, ea);

		if (!scb) {

			WL_NITRO(("wl%d %s: ERROR: NULL scb arg\n", wlc->pub->unit, __FUNCTION__));
			return BCME_BADARG;
		}

		temp = wlc_get_rxratehistory(wlc, scb);
		bcopy(&temp, a, sizeof(uint32));
		break;


	}
	case IOV_GVAL(IOV_TXRATE):
	{
		ratespec_t rspec = wlc_get_rspec_history(bsscfg);

		*ret_int_ptr = RSPEC2KBPS(rspec)/500;
		break;

	}

	default:
		return BCME_BADARG;
	}
	return 0;
}

static int
wlc_nitro_mpreq(wlc_nitro_t *nitro, wl_mpreq_t *mpreq, int len)
{
	void *p;
	uint8 *data = &mpreq->data[0];
	uint16 pollcnt = 0, dur, mpfrm_dur;
	wlc_info_t *wlc = nitro->wlc;
	struct dot11_header *h;
	d11txh_t *txh;
	wlc_txh_info_t txh_info;
	int ret;
	uint32 tmptt, phylen, tsf_l, tsf_h;
	nitro_mpfrm_t *mpfrm;
	bool resume = FALSE;
	wlc_bsscfg_t *cfg = wlc->cfg;

	if (NITRO_MODE(wlc) != NITRO_MODE_PARENT) {
		WL_ERROR(("invalid mode %d\n", NITRO_MODE(wlc)));
		return BCME_NOTAP;
	}
	if (nitro->mp.mpbusy) {
		WL_ERROR(("Mp busy: rxbitmap 0x%x, ucode_rxbitmap 0x%x\n",
			nitro->mp.rxbitmap, nitro->mp.ucode_rxbitmap));
		return BCME_BUSY;
	}
	if (mpreq->length > NITRO_MP_MAXLEN) {
		WL_ERROR(("Invalid data length %d\n", mpreq->length));
		return BCME_BADOPTION;
	}
	if (len < (int)(WL_MPREQ_FIXEDLEN + mpreq->length)) {
		WL_ERROR(("short buffer %d\n", len));
		return BCME_BUFTOOSHORT;
	}
	if (mpreq->retryLimit > WL_MPREQ_RETRYLIMIT_MAX) {
		WL_ERROR(("invalid retrylimit %d\n", mpreq->retryLimit));
		return BCME_BADARG;
	}

	p = wlc_frame_get_ctl(wlc, mpreq->length + NITRO_MPFRM_HDRLEN);
	if (p == NULL)
		return BCME_NOMEM;

	/* If request is resume, then override parameters from previous request */
	if (mpreq->resume & WL_MPREQ_RESUME_EN) {
		if (!(mpreq->resume & WL_MPREQ_RESUME_TXOP))
			mpreq->txop = nitro->mp.mpreq.txop;
		if (!(mpreq->resume & WL_MPREQ_RESUME_TMPTT))
			mpreq->tmptt = nitro->mp.mpreq.tmptt;
		if (!(mpreq->resume & WL_MPREQ_RESUME_BITMAP))
			mpreq->pollbitmap = nitro->mp.rxbitmap;
		if (!(mpreq->resume & WL_MPREQ_RESUME_DATA)) {
			mpreq->length = nitro->mp.mpreq.length;
			data = nitro->mp.mpdata;
		}
		resume = TRUE;
	}

	/* if txop is in byte unit, convert it time (us) units
	 * and add 6us to interoperate with older ds units
	 */
	if (mpreq->txop & WL_MPREQ_TXOP_TIME)
		mpreq->txop &= ~WL_MPREQ_TXOP_TIME;
	else
		mpreq->txop = NITRO_TXOP_CONV(mpreq->txop) + 6;

	/* Count number of children polled */
	pollcnt = bcm_bitcount((uint8 *)&mpreq->pollbitmap, sizeof(mpreq->pollbitmap));

	/* Build Indication frame */
	if ((ret = wlc_nitro_create_mpend(nitro, mpreq, pollcnt)) != 0) {
		PKTFREE(wlc->osh, p, FALSE);
		return ret;
	}

	/* Set busy flag */
	nitro->mp.mpbusy = TRUE;

	/* Build MP frame data */
	mpfrm = (nitro_mpfrm_t *)PKTDATA(wlc->osh, p);
	mpfrm->txop = htol16(mpreq->txop);
	mpfrm->pollbitmap = htol16(mpreq->pollbitmap);
	mpfrm->wmheader = htol16(mpreq->wmheader);
	bcopy(data, &(mpfrm->data[0]), mpreq->length);

	/* Fill in the 802.11 header */
	h = (struct dot11_header *)PKTPUSH(wlc->osh, p, DOT11_A3_HDR_LEN);
	bcopy(NITRO_MP_MACADDR, (char*)&h->a1, ETHER_ADDR_LEN);
	bcopy((char*)&cfg->BSSID, (char*)&h->a2, ETHER_ADDR_LEN);
	bcopy((char*)&cfg->cur_etheraddr, (char *)&h->a3, ETHER_ADDR_LEN);
	h->fc = FC_FROMDS | ((FC_TYPE_DATA << FC_TYPE_SHIFT) |
	                     (FC_SUBTYPE_DATA_CF_POLL << FC_SUBTYPE_SHIFT));
	h->fc = htol16(h->fc);
	h->durid = 0;
	/* For a resume operation use the seqnum from the previous MP sequence.
	 * else the ucode will populate it.
	 */
	h->seq = (resume ? htol16(nitro->seqnum) : 0);

	/* Calculate Duration */
	phylen = PKTLEN(wlc->osh, p) + DOT11_FCS_LEN;
	mpfrm_dur = (uint16)wlc_calc_frame_time(wlc, NITRO_RATE, WLC_SHORT_PREAMBLE, phylen);
	dur = mpfrm_dur + (pollcnt * (BPHY_SIFS_TIME + mpreq->txop))
	      + BPHY_SIFS_TIME + NITRO_MPACK_DUR;

	/* If tmptt is zero, set it to the time for the one sequence */
	/* mpreq->tmptt is 10us unit */
	tmptt = (mpreq->tmptt ? (mpreq->tmptt * 10) :
	                        (dur + BPHY_DIFS_TIME + (BPHY_CWMIN * BPHY_SLOT_TIME)));

	/* convert tmptt to TSF timestamp */
	/* If currtsf is not sent from above, use the current value of tsf timer */
	if (mpreq->currtsf == 0) {
		wlc_read_tsf(wlc, &tsf_l, &tsf_h);
		tmptt += tsf_l;
	}
	else
		tmptt += mpreq->currtsf;

	/* Add d11 header */
	wlc_nitro_d11hdr(nitro, p, h, dur, TRUE, resume, &txh);

	/* Update tx header with nitro parent specific data */
	txh->MacFrameControl = htol16(FC_FROMDS | (MP_FRMTYPE << FC_TYPE_SHIFT));
	txh->PreloadSize = htol16(mpreq->pollbitmap);
	txh->AmpduSeqCtl = htol16(mpreq->txop + BPHY_SIFS_TIME);
	txh->MinMBytes = htol16(mpreq->retryLimit);
	txh->u2.MaxAggLen_FBR = htol16(mpfrm_dur);		/* Duration of the MP frame */
	txh->MaxNMpdus = htol16(tmptt & 0xffff);
	txh->u1.MaxAggLen = htol16((tmptt>>16) & 0xffff);

	/* send the frame */
#ifdef NEW_TXQ
	wlc_low_txq_enq(wlc->txqi, wlc->active_queue->qi->low_txq,
		TX_NITRO_FIFO, p, 1);
#else
	wlc_get_txh_info(wlc, p, &txh_info);
	wlc_txfifo(wlc, TX_NITRO_FIFO, p, &txh_info, TRUE, 1);
#endif /* NEW_TXQ */

	/* Preserve MP req parameters */
	bcopy(mpreq, &nitro->mp.mpreq, WL_MPREQ_FIXEDLEN);
	if (data != nitro->mp.mpdata)
		bcopy(&mpreq->data[0], nitro->mp.mpdata, mpreq->length);
	nitro->mp.rxbitmap = mpreq->pollbitmap;
	nitro->mp.ucode_rxbitmap = 0;

	return 0;
}

static int
wlc_nitro_keydatareq(wlc_nitro_t *nitro, wl_keydatareq_t *keyreq, int len)
{
	void *p;
	int phylen, dur;
	wlc_info_t *wlc = nitro->wlc;
	d11txh_t *txh;
	wlc_txh_info_t txh_info;
	struct dot11_header *h;
	struct scb *scb;
	nitro_keydatafrm_t *keydatafrm;

	if (NITRO_MODE(wlc) != NITRO_MODE_CHILD) {
		WL_ERROR(("invalid mode %d\n", NITRO_MODE(wlc)));
		return BCME_NOTSTA;
	}
	if (keyreq->length > NITRO_KEYDATA_MAXLEN) {
		WL_ERROR(("Invalid data length %d\n", keyreq->length));
		return BCME_BADOPTION;
	}
	if (len < (int)(WL_KEYDATAREQ_FIXEDLEN + keyreq->length)) {
		WL_ERROR(("short buffer %d\n", len));
		return BCME_BUFTOOSHORT;
	}

	if (!(scb = wlc_scbfind(wlc, wlc->cfg, &wlc->cfg->BSSID)) || !(SCB_ASSOCIATED(scb))) {
		WL_ERROR(("sta not associated\n"));
		return BCME_NOTASSOCIATED;
	}

	if (wlc->core->txpktpend[TX_NITRO_FIFO] > NITRO_MAX_QLEN) {
		nitro->stats.txqfull++;
		WL_ERROR(("Child TXQ full\n"));
		return BCME_NORESOURCE;
	}

	p = wlc_frame_get_ctl(wlc, (keyreq->length + NITRO_KEYDATA_FRM_HDRLEN));
	if (p == NULL)
		return BCME_NOMEM;

	/* Fill in packet data */
	keydatafrm = (nitro_keydatafrm_t *)PKTDATA(nitro->wlc->osh, p);
	keydatafrm->wmheader = htol16(keyreq->wmheader);
	bcopy(&keyreq->keyData[0], &(keydatafrm->data[0]), keyreq->length);

	/* Fill in the 802.11 header */
	h = (struct dot11_header *)PKTPUSH(wlc->osh, p, DOT11_A3_HDR_LEN);
	bcopy((char*)&wlc->cfg->BSSID, (char*)&h->a1, ETHER_ADDR_LEN);
	bcopy((char*)&wlc->pub->cur_etheraddr, (char *)&h->a2, ETHER_ADDR_LEN);
	bcopy(NITRO_KEYDATA_MACADDR, (char*)&h->a3, ETHER_ADDR_LEN);
	h->fc = htol16(FC_TODS | ((FC_TYPE_DATA << FC_TYPE_SHIFT) |
	                          (FC_SUBTYPE_DATA_CF_ACK << FC_SUBTYPE_SHIFT)));
	h->durid = 0;	/* ucode will fill in */
	h->seq = 0;		/* ucode will fill in */

	phylen = PKTLEN(wlc->osh, p) + DOT11_FCS_LEN;
	dur = wlc_calc_frame_time(wlc, NITRO_RATE, WLC_SHORT_PREAMBLE, phylen);

	/* Add d11 header */
	wlc_nitro_d11hdr(nitro, p, h, (uint16)dur, FALSE, FALSE, &txh);

	/* Update tx header with Nitro child specific data */
	txh->MacFrameControl = htol16(FC_TODS | (KEYRESP_FRMTYPE << FC_TYPE_SHIFT));

	/* send the frame */
#ifdef NEW_TXQ
	wlc_low_txq_enq(wlc->txqi, wlc->active_queue->qi->low_txq,
		TX_NITRO_FIFO, p, 1);
#else
	wlc_get_txh_info(wlc, p, &txh_info);
	wlc_txfifo(wlc, TX_NITRO_FIFO, p, &txh_info, TRUE, 1);
#endif /* NEW_TXQ */

	return 0;
}

static int
wlc_nitro_d11hdr(wlc_nitro_t *nitro, void *p, struct dot11_header *h, uint16 dur,
	bool immed_ack, bool sw_seq, d11txh_t **txhp)
{
	uint16 mcl = 0, phyctl, frameid, xfts;
	uint32 phylen;
	d11txh_t *txh;
	uint8 *plcp;
	wlc_info_t *wlc = nitro->wlc;

	phylen = PKTLEN(wlc->osh, p) + DOT11_FCS_LEN;

	/* add PLCP */
	plcp = PKTPUSH(wlc->osh, p, D11_PHY_HDR_LEN);
	wlc_compute_plcp(wlc, NITRO_RATE, phylen, ltoh16(h->fc), plcp);

	/* add Broadcom tx descriptor header */
	txh = (d11txh_t*)PKTPUSH(wlc->osh, p, D11_TXH_LEN);
	bzero((char*)txh, D11_TXH_LEN);

	/* set sequence control field, and setup frameid */
	frameid = (((wlc->counter++ << TXFID_SEQ_SHIFT) & TXFID_SEQ_MASK) |
		(WLC_TXFID_SET_QUEUE(TX_NITRO_FIFO)));

	/* MacTxControlLow */
	mcl |= TXC_STARTMSDU | TXC_IGNOREPMQ;
	if (immed_ack)
		mcl |= TXC_IMMEDACK;
	if (!sw_seq)
		mcl |= TXC_HWSEQ;
	txh->MacTxControlLow = htol16(mcl);

	bcopy((char*)&h->a1, (char*)&txh->TxFrameRA, ETHER_ADDR_LEN);
	txh->TxFrameID = htol16(frameid);
	txh->TxFesTimeNormal = htol16(dur);

	/* XtraFrameTypes */
	xfts = FT_CCK;
	xfts |= (FT_CCK << XFTS_RTS_FT_SHIFT);
	xfts |= (FT_CCK << XFTS_FBRRTS_FT_SHIFT);
	xfts |= CHSPEC_CHANNEL(WLC_BAND_PI_RADIO_CHANSPEC) << XFTS_CHANNEL_SHIFT;
	txh->XtraFrameTypes = htol16(xfts);

	phyctl = FT_CCK;
	if (WLCISHTPHY(wlc->band)) {
		phyctl |= (wlc->stf->txant << PHY_TXC_ANT_SHIFT) & PHY_TXC_HTANT_MASK;
	} else
		phyctl |= (wlc->stf->txant << PHY_TXC_ANT_SHIFT) & PHY_TXC_ANT_MASK;
	phyctl |= PHY_TXC_SHORT_HDR; /* Short Preamble */
	txh->PhyTxControlWord = htol16(phyctl);
	txh->PhyTxControlWord_1 = htol16(0x2);

	*txhp = txh;
	return frameid;
}

/* Process Tx status for Nitro frames */
void
wlc_nitro_txstatus(wlc_nitro_t *nitro, tx_status_t *txs, uint16 pollbitmap)
{
	void *p;
	wlc_info_t *wlc = nitro->wlc;
	uint attempts;
	uint16 ucode_bitmap;
	wlc_txh_info_t txh_info;
	struct dot11_header *h;

	if (txs->status.is_intermediate) {
		nitro->stats.istatus++;
		return;
	}
	wlc_print_txstatus(wlc, txs);

	attempts = txs->status.frag_tx_cnt;

	p = (PIO_ENAB(wlc->pub)) ? wlc_pio_getnexttxp(WLC_HW_PIO(wlc, TX_NITRO_FIFO)) :
	        wlc_bmac_dma_getnexttxp(WLC_HW_DI(wlc, TX_NITRO_FIFO), HNDDMA_RANGE_TRANSMITTED);
	ASSERT(p);

	wlc->core->txpktpend[TX_NITRO_FIFO]--;
	ASSERT(wlc->core->txpktpend[TX_NITRO_FIFO] >= 0);

	/* update the bytes in/out counts */
	if (PIO_ENAB(wlc->pub))
		wlc_pio_cntupd(WLC_HW_PIO(wlc, TX_NITRO_FIFO), PKTLEN(wlc->osh, p));

	wlc_get_txh_info(wlc, p, &txh_info);
	ASSERT(txs->frameid == htol16(txh_info.TxFrameID));

	if (attempts) {
		nitro->stats.txnitro++;
		nitro->stats.retrans += (attempts - 1);
	}
	else {
		nitro->stats.txnitro_fail++;
	}

	/* Generate MP.END indication on MP frame tx completion */
	h = (struct dot11_header *)(txh_info.d11HdrPtr);

	if ((ltoh16(h->fc) & FC_KIND_MASK) == (MP_FRMTYPE << FC_TYPE_SHIFT)) {
		/* save seqnum for resume */
		nitro->seqnum = ltoh16(h->seq);

		/* txs->phyerr is overloaded with rx pollbitmap */
		ucode_bitmap = txs->status.was_acked ? pollbitmap : nitro->mp.rxbitmap;
		wlc_nitro_mpend(nitro, ucode_bitmap, attempts);
	}

	PKTFREE(wlc->osh, p, TRUE);
}

/*
 * Allocate and initialize MPEND data buffer.
 */
static int
wlc_nitro_create_mpend(wlc_nitro_t *nitro, wl_mpreq_t *mpreq, int poll_cnt)
{
	int len, pktlen;
	void *p;
	nwm_mpkey_t *mpkey;
	osl_t *osh = nitro->wlc->osh;

	/* Convert txop to max packet length from each child */
	len = ((mpreq->txop - NITRO_NULLKEYDATA_DUR) / NITRO_BYTETIME_2M) +
		NWM_MPKEYDATA_FIXED_SIZE;
	len = ROUNDUP(len, 2);
	pktlen = (len * poll_cnt) + sizeof(struct ether_header) + NWM_MPKEY_FIXED_SIZE + 2;

	/* sdpcmdev needs extra headroom */
	pktlen += BCMEXTRAHDROOM;
	if ((p = PKTGET(osh, pktlen, FALSE)) == NULL)
		return BCME_NOMEM;

	PKTPULL(osh, p, BCMEXTRAHDROOM);
	PKTPULL(osh, p, 2);
	PKTPULL(osh, p, sizeof(struct ether_header));
	mpkey = (nwm_mpkey_t *)PKTDATA(osh, p);
	mpkey->resp_maxlen = len;
	mpkey->bitmap = mpreq->pollbitmap;
	mpkey->count = 0;
	mpkey->txCount = 0;

	ASSERT(nitro->mpend == NULL);
	nitro->mpend = p;
	return 0;
}

/*
 * Close the MPEND data buffer and send it up.
 */
static void
wlc_nitro_mpend(wlc_nitro_t *nitro, uint16 rxbitmap, uint retries)
{
	void *mpend = nitro->mpend;
	nwm_mpkey_t *mpkey;
	int len, i;
	uint32 tsf_l, tsf_h;

	/* If our receive bitmap does not match with ucode,
	 * keydata frames may still be in receive queue;
	 * schedule a timer event to generate the indication.
	 */
	if (rxbitmap ^ nitro->mp.rxbitmap) {
		WL_NITRO(("Mp bitmap mismatch: sw 0x%x, ucode 0x%x\n",
		           nitro->mp.rxbitmap, rxbitmap));
		nitro->mp.ucode_rxbitmap = rxbitmap | 1;
		wl_add_timer(nitro->wlc->wl, nitro->timer, 0, 0);
		return;
	}

	ASSERT(nitro->mp.mpbusy);
	ASSERT(mpend);

	WL_NITRO(("sending up mpend frame\n"));
	mpkey = (nwm_mpkey_t *)PKTDATA(nitro->wlc->osh, nitro->mpend);
	mpkey->txCount += retries;
	mpkey->txCount = hton16(mpkey->txCount);

	memset((char *)&mpkey->nwmindhdr, 0, NWM_INDHDR_FIXED_SIZE);
	mpkey->nwmindhdr.magic = hton32(NWM_NIN_IND_MAGIC);
	mpkey->nwmindhdr.indcode = hton16(NWM_CMDCODE_MA_MPEND_IND);

	/* length of valid keydata in packet */
	len = (mpkey->resp_maxlen * mpkey->count);

	mpkey->count = hton16(mpkey->count);
	mpkey->resp_maxlen = hton16(mpkey->resp_maxlen);

	/* keydata + nwm_mpkey_t subhdr; round _up_ length in 16 bit words */
	mpkey->nwmindhdr.indlength = hton16((len + NWM_MPKEY_SUBHDR_SIZE + 1)/2);
	/* update pktlen = keydata + nwm_mpkey_t header fixed size */
	PKTSETLEN(nitro->wlc->osh, mpend, (len + NWM_MPKEY_FIXED_SIZE));
	if (PKTLEN(nitro->wlc->osh, mpend) !=
		(len + NWM_MPKEY_FIXED_SIZE)) {
		WL_ERROR(("%s: error setting mpend pkt length %d\n",
			__FUNCTION__, (int)(len + NWM_MPKEY_FIXED_SIZE)));
	}

	/* Update the currtsf value in indication */
	wlc_read_tsf(nitro->wlc, &tsf_l, &tsf_h);
	mpkey->currtsf = hton16(((tsf_l >> 6) & 0xffff));
	mpkey->bitmap = hton16(mpkey->bitmap);

	wlc_nitro_sendup(nitro->wlc, mpend);

	/* Update key response error counters */
	if (nitro->mp.rxbitmap) {
		for (i = 1; i < NBITS(nitro->mp.rxbitmap); i++) {
			if (isset(&nitro->mp.rxbitmap, i))
				nitro->stats.rxkeyerr[i-1]++;
		}
		nitro->mp.rxbitmap = 0;
	}

	nitro->mpend = NULL;
	nitro->mp.mpbusy = FALSE;
	nitro->mp.ucode_rxbitmap = 0;
	return;
}

bool
wlc_nitro_frame(wlc_nitro_t *nitro, struct dot11_header *h)
{
	struct ether_addr *da;

	da = (ltoh16(h->fc) & FC_TODS) ? &(h->a3) : &(h->a1);
	/* optimization: compare the common first 3 bytes of the MACs first */
	if ((!bcmp(da, NITRO_MP_MACADDR, 3)) &&
	    ((!bcmp(da, NITRO_KEYDATA_MACADDR, ETHER_ADDR_LEN)) ||
	     (!bcmp(da, NITRO_MP_MACADDR, ETHER_ADDR_LEN)) ||
	     (!bcmp(da, NITRO_MPACK_MACADDR, ETHER_ADDR_LEN))))
		return TRUE;

	return (FALSE);
}

/*
 * Receive nitro protocol frames.
 * Frames are already filtered before ending up here.
 */
void
wlc_nitro_recvdata(wlc_nitro_t *nitro, struct scb *scb, struct wlc_frminfo *f,
	wlc_d11rxhdr_t *wrxh, ratespec_t rspec)
{
	wlc_info_t *wlc = nitro->wlc;
	int prio = 0;
	uint rate;
	int8 rssi;

	/* rate in 100kbps */
	rate = RSPEC2KBPS(rspec)/100;

	rssi = wlc_lq_rssi_pktrxh_cal(wlc, wrxh);

	ASSERT(WLNITRO_ENAB(wlc));

	/* duplicate detection */
	if (!f->ismulti) {
		if ((f->fc & FC_RETRY) && (scb->seqctl[prio] == f->seq)) {
			WL_NITRO(("wl%d: wlc_nitro_recvdata: discarding duplicate MPDU %04x "
				  "received from %s prio: %d\n", wlc->pub->unit, f->seq,
				  bcm_ether_ntoa(&(f->h->a2), (char*)eabuf), prio));
			WLCNTINCR(wlc->pub->_cnt->rxdup);
			PKTFREE(wlc->osh, f->p, FALSE);
			return;
		}
		else
			scb->seqctl[prio] = f->seq;
	}

	switch (NITRO_MODE(wlc)) {
	case NITRO_MODE_PARENT:
		wlc_nitro_rx_parent(nitro, scb, f, rssi, rate);
		break;

	case NITRO_MODE_CHILD:
		wlc_nitro_rx_child(nitro, scb, f, rssi, rate, wrxh);
		break;

	default:
		WL_ERROR(("Received nitro frame when nitro is not enabled\n"));
		PKTFREE(wlc->osh, f->p, FALSE);
	}
}

/*
 * Send nitro frames up with brcm ether type encapsulation.
 */
static void
wlc_nitro_sendup(wlc_info_t *wlc, void *p)
{
	struct ether_header *eth;

	eth = (struct ether_header *)PKTPUSH(wlc->osh, p, sizeof(struct ether_header));
	bcopy(&wlc->pub->cur_etheraddr, (char *)&eth->ether_dhost, ETHER_ADDR_LEN);
	bcopy(&wlc->pub->cur_etheraddr, (char *)&eth->ether_shost, ETHER_ADDR_LEN);
	ETHER_SET_LOCALADDR(&eth->ether_shost);
	eth->ether_type = hton16(ETHER_TYPE_BRCM);

	wl_sendup(wlc->wl, NULL, p, 1);
}

/*
 * Process nitro protocol frames (mp, mpack frames) in child mode.
 * Slap in a MP indication header on top of the frame and send it up.
 */
static void
wlc_nitro_rx_child(wlc_nitro_t *nitro, struct scb *scb, struct wlc_frminfo *f,
	int8 rssi, uint rate, wlc_d11rxhdr_t *wrxh)
{
	d11rxhdr_t *rxh = &wrxh->rxhdr;
	uint16 txop, bitmap;
	nwm_mpind_hdr_t	*mpind_hdr;
	uint16 pollcnt = 0, keysts = 0, acktimestamp = 0, length = 0;
	osl_t *osh = nitro->wlc->osh;
	wlc_info_t *wlc = nitro->wlc;
	nitro_mpfrm_t *mpfrm;
	void *p1;
	uint16 indtype;

	if (!bcmp(&(f->h->a1), NITRO_MP_MACADDR, ETHER_ADDR_LEN)) {
		if (f->body_len < NITRO_MPFRM_HDRLEN) {
			nitro->stats.rxbadnitro++;
			PKTFREE(osh, f->p, FALSE);
			return;
		}

		/* check for duplicate MP frame */
		if ((f->fc & FC_RETRY) && (nitro->seqnum == f->seq)) {
			WL_NITRO(("wl%d: wlc_nitro_rx_child: discarding duplicate MP frame %04x\n",
			          wlc->pub->unit, f->seq));
			nitro->stats.rxdupnitro++;
			PKTFREE(osh, f->p, FALSE);
			return;
		}
		else
			nitro->seqnum = f->seq;

		nitro->stats.rxmp++;
		mpfrm = (nitro_mpfrm_t *)f->pbody;
		txop = ltoh16(mpfrm->txop);
		bitmap = ltoh16(mpfrm->pollbitmap);
		WL_NITRO(("Recvd MP frame txop %d, bitmap 0x%x, seq 0x%x, txfifo status 0x%x\n",
		          txop, bitmap, nitro->seqnum, rxh->RxStatus2));

		/* Is my aid in the pollbitmap */
		nitro->keytxsts = (bitmap & (1 << AID2PVBMAP(scb->aid))) ?
			WL_MPIND_KEYSTS_POLLED : 0;

		/* Was there a response to the MP frame */
		/* If there was no valid response, then a null frame was sent */
		if (rxh->RxStatus2 & RXS_KEYDATA_TX)
			nitro->keytxsts |= WL_MPIND_KEYSTS_RESKEY;
		else if (rxh->RxStatus2 & RXS_KEYDATA_MASK)
				nitro->stats.txnullkeydata++;

		pollcnt = bcm_bitcount((uint8 *)&bitmap, sizeof(bitmap));

		/* Calculate MPACK recv timestamp based on MP frame's timestamp */
		acktimestamp = rxh->RxTSFTime + ((txop+BPHY_SIFS_TIME) * pollcnt);
		length = f->body_len - 4;   /* txop(2), bitmap(2) */

		indtype = NWM_CMDCODE_MA_MP_IND;
	}
	else if (!bcmp(&(f->h->a1), NITRO_MPACK_MACADDR, ETHER_ADDR_LEN)) {
		if (f->body_len < NITRO_MPACKFRM_HDRLEN) {
			nitro->stats.rxbadnitro++;
			PKTFREE(osh, f->p, FALSE);
			return;
		}
		nitro->stats.rxmpack++;
		indtype = NWM_CMDCODE_MA_MPACK_IND;
	}
	else {
		/* Should not come here */
		ASSERT(0);
		WL_ERROR(("wlc_nitro_rx_child: received invalid frame\n"));
		PKTFREE(osh, f->p, FALSE);
		return;
	}

	/* Update keytxsts with num of key response frames in txfifo */
	switch (wlc->core->txpktpend[TX_NITRO_FIFO]) {
	case 1: keysts |= WL_MPIND_KEYSTS_OUTSTS; break;
	case 2: keysts |= (WL_MPIND_KEYSTS_OUTSTS|WL_MPIND_KEYSTS_INSTS); break;
	}

	/* Add MP indication header to the frame before sending it up */
	/* Headroom requirement means we need to allocate a new pkt */
	p1 = PKTGET(osh, sizeof(nwm_mpind_hdr_t) + PKTLEN(osh, f->p) + BCMEXTRAHDROOM, FALSE);
	if (!p1) {
		/* really bad */
		ASSERT(0);
		PKTFREE(osh, f->p, FALSE);
		return;
	}
	PKTPULL(osh, p1, sizeof(nwm_mpind_hdr_t) + BCMEXTRAHDROOM);
	bcopy(PKTDATA(osh, f->p), PKTDATA(osh, p1), PKTLEN(osh, f->p));
	PKTFREE(osh, f->p, FALSE);

	mpind_hdr = (nwm_mpind_hdr_t *)PKTPUSH(osh, p1, sizeof(nwm_mpind_hdr_t));

	/* Force all multibyte quantities to network byte order */
	mpind_hdr->length = hton16(length);
	mpind_hdr->keytxsts = hton16(nitro->keytxsts | keysts);
	mpind_hdr->timestamp = rxh->RxTSFTime;
	mpind_hdr->mpacktimestamp = hton16(acktimestamp);
	mpind_hdr->rate = rate;
	mpind_hdr->rssi = (uint8)rssi;

	memset((char *)&mpind_hdr->nwmindhdr, 0, NWM_MPIND_HDR_SIZE);
	mpind_hdr->nwmindhdr.magic = hton32(NWM_NIN_IND_MAGIC);
	mpind_hdr->nwmindhdr.indcode = hton16(indtype);

	mpind_hdr->nwmindhdr.indlength = hton16(NWM_MPIND_HDR_SIZE/2);

	wlc_nitro_sendup(wlc, p1);
}

/* Process nitro protocol frames in parent mode (keyresponse frames) */
static void
wlc_nitro_rx_parent(wlc_nitro_t *nitro, struct scb *scb, struct wlc_frminfo *f,
	int8 rssi, uint rate)
{
	wlc_info_t *wlc = nitro->wlc;
	uint16 bitmap;
	nwm_mpkey_t *mpkey;
	nwm_mpkey_data_t keydata;
	osl_t *osh = nitro->wlc->osh;
	nwm_mpkey_data_t *pkeydat;
	uint32 offset;
	uint bytes_copied;
#if defined(BCMDBG_ERR)
	char eabuf[ETHER_ADDR_STR_LEN];
#endif

	if (bcmp(&(f->h->a3), NITRO_KEYDATA_MACADDR, ETHER_ADDR_LEN)) {
		WL_ERROR(("Received invalid MP frame; da %s\n",
		           bcm_ether_ntoa(&f->h->a3, eabuf)));
		goto badkey;
	}

	if (nitro->mp.mpbusy == FALSE) {
		WL_ERROR(("Received Keydata out of MP sequence timeframe\n"));
		goto badkey;
	}

	/* Check child is in the polled bitmap */
	bitmap = 1 << AID2PVBMAP(scb->aid);
	if ((bitmap & nitro->mp.mpreq.pollbitmap) == 0) {
		WL_ERROR(("Received Keydata from child %d not in bitmap 0x%x\n", scb->aid,
			nitro->mp.mpreq.pollbitmap));
		goto badkey;
	}

	if ((bitmap & nitro->mp.rxbitmap) == 0) {
		WL_NITRO(("Received duplicate Keydata from child %d\n", scb->aid));
		nitro->stats.rxdupnitro++;
		PKTFREE(wlc->osh, f->p, FALSE);
		return;
	}

	mpkey = (nwm_mpkey_t *)PKTDATA(osh, nitro->mpend);

	if (f->body_len > mpkey->resp_maxlen) {
		WL_ERROR(("Received Keydata length %d more than expected %d. child aid 0x%x\n",
			f->body_len, mpkey->resp_maxlen, scb->aid));
		goto badkey;
	}

	nitro->mp.rxbitmap &= ~bitmap;
	mpkey->bitmap &= ~bitmap;

	pkeydat = (nwm_mpkey_data_t *)((uint8 *)&mpkey->data[0] +
		(mpkey->resp_maxlen * mpkey->count));
	offset = (uint32)((uintptr)pkeydat - (uintptr)PKTDATA(osh, nitro->mpend));

	bzero(&keydata, sizeof(nwm_mpkey_data_t));
	keydata.length = hton16(f->body_len);
	keydata.aid = hton16(AID2PVBMAP(scb->aid));
	keydata.rssi = (uint8)rssi;
	keydata.rate = rate;

	bytes_copied = wl_buf_to_pktcopy(osh, nitro->mpend,
		(uchar *)&keydata, NWM_MPKEYDATA_FIXED_SIZE, offset);

	if (bytes_copied != NWM_MPKEYDATA_FIXED_SIZE) {
		/* What? */
		WL_ERROR(("%s: keydata parms copy failure bytes in %d out %d\n",
		__FUNCTION__, (int)NWM_MPKEYDATA_FIXED_SIZE, bytes_copied));
		goto badkey;
	}

	offset = (uint32)((uintptr)&pkeydat->cdata[0] - (uintptr)PKTDATA(osh, nitro->mpend));

	bytes_copied = wl_buf_to_pktcopy(osh, nitro->mpend, f->pbody, f->body_len, offset);
	if (bytes_copied != f->body_len) {
		WL_ERROR(("%s: keydata copy failure bytes in %d out %d\n",
		__FUNCTION__, f->body_len, bytes_copied));
		goto badkey;
	}
	mpkey->count++;

	if (f->body_len == 0)
		nitro->stats.rxnullkeydata++;
	else
		nitro->stats.rxkeydata++;
	WL_NITRO(("Rx keydata from child aid 0x%x, len %d\n", scb->aid, f->body_len));


	PKTFREE(wlc->osh, f->p, FALSE);
	return;

badkey:
	nitro->stats.rxbadnitro++;
	PKTFREE(wlc->osh, f->p, FALSE);
	return;
}

/* Update ssid mask into shared memory */
static void
wlc_nitro_shm_ssidmask_upd(wlc_info_t *wlc, uint8 *mask)
{
	uint16 i, tmp;

	for (i = 0; i < NITRO_SSIDMASK_SIZE; i += 2) {
		tmp = (uint16)(mask[i] + (mask[i+1] << 8));
		wlc_write_shm(wlc, M_SSID_MASK + i, tmp);
	}
}

/* Update ssid with ssid_mask */
void
wlc_nitro_ssidmask_apply(wlc_info_t *wlc, uint8 *ssid, uint ssid_len)
{
	uint8 i, *mask = wlc->nitro->ssid_mask;

	for (i = 0; i < ssid_len; i++)
		ssid[i] &= mask[i];
}

int
wlc_nitro_core_init(wlc_info_t *wlc)
{
	wlc_nitro_t *nitro = wlc->nitro;

	if (!wlc->clk) {
		ASSERT(0);
		return BCME_NOCLK;
	}

	wlc_mhf(wlc, MHF2, MHF2_NITRO_MODE, (NITRO_MODE(wlc) ? MHF2_NITRO_MODE : 0), WLC_BAND_ALL);

	wlc_nitro_shm_ssidmask_upd(wlc, nitro->ssid_mask);
	return 0;
}

/* Child: Write assigned AID to shared memory */
void
wlc_nitro_aid_upd(wlc_info_t *wlc)
{
	wlc_write_shm(wlc, M_NITRO_AID, (1 << AID2PVBMAP(wlc->AID)));
}

/* Create a vendor IE structure for Nintendo game IE */
static nitro_gameIE_t *
wlc_nitro_ie_create(wlc_info_t *wlc)
{
	nitro_gameIE_t *game_ie;

	game_ie = (nitro_gameIE_t *)MALLOC(wlc->osh,
	                                   (NITRO_GAMEIE_FIXEDLEN + NITRO_GAMEINFO_MAX));

	if (!game_ie) {
		WL_ERROR(("wl%d: wlc_nitro_ie_create: out of memory, malloced %d bytes\n",
			wlc->pub->unit, MALLOCED(wlc->osh)));
		return NULL;
	}

	game_ie->id = DOT11_MNG_PROPR_ID;
	game_ie->len = NITRO_GAMEIE_FIXEDLEN - TLV_HDR_LEN;
	bcopy(NINTENDO_OUI, game_ie->oui, NINTENDO_OUI_LEN);

	game_ie->activeZone = htol16(NITRO_ACTIVEZONE_DEF);
	game_ie->vblankTsf = 0;
	game_ie->reserved = 0;

	return (game_ie);
}

/* Add/modify nintendo ie to wlc vendor ie list */
static int
wlc_nitro_vndrie_update(wlc_nitro_t *nitro)
{
	wlc_info_t *wlc = nitro->wlc;
	wlc_bsscfg_t *bsscfg = wlc->cfg;

	if (!nitro->gameIE_list_el)
		nitro->gameIE_list_el = wlc_vndr_ie_add_elem(bsscfg,
		                        (VNDR_IE_BEACON_FLAG | VNDR_IE_PRBRSP_FLAG),
		                        (vndr_ie_t *)nitro->gameIE);
	else
		nitro->gameIE_list_el = wlc_vndr_ie_mod_elem(bsscfg, nitro->gameIE_list_el,
		                        (VNDR_IE_BEACON_FLAG | VNDR_IE_PRBRSP_FLAG),
		                        (vndr_ie_t *)nitro->gameIE);

	if (bsscfg->up) {
		wlc_bss_update_beacon(wlc, bsscfg);
		wlc_bss_update_probe_resp(wlc, bsscfg, TRUE);
	}

	if (!nitro->gameIE_list_el)
		return BCME_NORESOURCE;

	return 0;
}

/* Delete nintendo ie from wlc vendor ie list */
static int
wlc_nitro_vndrie_delete(wlc_nitro_t *nitro)
{
	wlc_info_t *wlc = nitro->wlc;
	wlc_bsscfg_t *bsscfg = wlc->cfg;
	int status;
	int ielen;
	int buf_reqd_len;
	vndr_ie_buf_t *p;


	/* If ie has not been added just return */
	if (!nitro->gameIE_list_el)
		return 0;

	ielen = nitro->gameIE_list_el->vndr_ie_infoel.vndr_ie_data.len;

	/* should subtract one and not roundup if we want excruciating
	 * accuracy but a little pad will not hurt
	 */
	buf_reqd_len = ROUNDUP(sizeof(vndr_ie_buf_t) + ielen, 4);

	p = (vndr_ie_buf_t *)MALLOC(wlc->osh, buf_reqd_len);
	if (p == NULL) {
		WL_ERROR(("%s: Can't allocate ie buffer len %d\n", __FUNCTION__, buf_reqd_len));
		status = BCME_NOMEM;
		goto err_exit;
	}


	p->iecount = 1;
	bcopy(&nitro->gameIE_list_el->vndr_ie_infoel,
		p->vndr_ie_list, sizeof(vndr_ie_info_t) - 1 + ielen);

	status = wlc_vndr_ie_del(bsscfg, p, buf_reqd_len);
	if (status) {
		printf("%s: wlc_vndr_ie_del returned %d\n", __FUNCTION__, status);
		goto done;
	}

	nitro->gameIE_list_el = NULL;

done:
	MFREE(wlc->osh, p, buf_reqd_len);
err_exit:
	return status;
}

/* Compare ssid with ssidmask */
bool
wlc_nitro_match_ssid(wlc_info_t *wlc, uint8 *ssid1, uint8 *ssid2, uint len1, uint len2)
{
	uint8 *mask = ((wlc_nitro_t *)wlc->nitro)->ssid_mask;
	uint i;

	if (len1 != len2)
		return FALSE;

	for (i = 0; i < len1; i++) {
		if ((ssid1[i] & mask[i]) != (ssid2[i] & mask[i]))
			return FALSE;
	}
	return TRUE;
}

/*
 * Returns true if the da of the frame is Multicast address
 * reserved for nitro protocol and nitro mode is enabled.
 */
bool
wlc_nitro_multicast(wlc_info_t *wlc, struct ether_addr *da)
{
	if ((NITRO_MODE(wlc) == NITRO_MODE_CHILD) &&
	    (!bcmp(da, NITRO_MP_MACADDR, ETHER_ADDR_LEN) ||
	     !bcmp(da, NITRO_MPACK_MACADDR, ETHER_ADDR_LEN)))
		return TRUE;

	return FALSE;
}


/* Update MPACK template and NUll response template with new mac addr */
void
wlc_nitro_macaddr_upd(wlc_info_t *wlc)
{
	char template_buf[(T_RAM_ACCESS_SZ*2)];
	wlc_bsscfg_t *cfg = wlc->cfg;

	/* SA is A3 addr for MPACK frames which is at byte offset 22 */
	/* fill out a buffer with a multiple of 4 length, starting at
	 * a 4 byte boundary offset for wlc_write_template_ram, 6 bytes
	 * of MAC addr and the first 2 bytes of our beaconing BSSID.
	 */
	bcopy((char*)cfg->BSSID.octet+4, template_buf, 2);
	bcopy((char*)cfg->cur_etheraddr.octet, template_buf+2, ETHER_ADDR_LEN);
	wlc_write_template_ram(wlc, (T_MPACK_TPL_BASE + 20), (T_RAM_ACCESS_SZ*2),
	                       template_buf);

	/* SA is A2 addr for Null response frames which is at byte offset 16 */
	/* fill out a buffer with a multiple of 4 length, starting at
	 * a 4 byte boundary offset for wlc_write_template_ram, 2 bytes
	 * of MAC addr and the 6 bytes of DA for NULL response frames.
	 */
	bcopy((char*)wlc->pub->cur_etheraddr.octet, template_buf, ETHER_ADDR_LEN);
	bcopy((char*)NITRO_KEYDATA_MACADDR, template_buf+ETHER_ADDR_LEN, 2);
	wlc_write_template_ram(wlc, (T_NULLRSP_TPL_BASE + 16), (T_RAM_ACCESS_SZ*2),
	                       template_buf);
}

/* Update MPACK template and NUll response template with new ssid */
void
wlc_nitro_bssid_upd(wlc_info_t *wlc)
{
	char template_buf[(T_RAM_ACCESS_SZ*2)];
	wlc_bsscfg_t *cfg = wlc->cfg;

	/* ssid is A2 addr for MPACK frames which is at byte offset 16 */
	/* fill out a buffer with a multiple of 4 length, starting at
	 * a 4 byte boundary offset for wlc_write_template_ram, 6 bytes
	 * of MAC addr and the first 2 bytes of our beaconing BSSID.
	 */
	bcopy((char*)cfg->BSSID.octet, template_buf, ETHER_ADDR_LEN);
	bcopy((char*)cfg->cur_etheraddr.octet, template_buf+ETHER_ADDR_LEN, 2);
	wlc_write_template_ram(wlc, (T_MPACK_TPL_BASE + 16), (T_RAM_ACCESS_SZ*2),
	                       template_buf);

	/* ssid is A1 addr for Null response frames which is at byte offset 10 */
	/* fill out a buffer with a multiple of 4 length, starting at
	 * a 4 byte boundary offset for wlc_write_template_ram, 2 bytes
	 * of dur/id and the 6 bytes of our BSSID.
	 */
	bzero(template_buf, (T_RAM_ACCESS_SZ*2));
	bcopy((char*)wlc->cfg->BSSID.octet, template_buf+2, ETHER_ADDR_LEN);
	wlc_write_template_ram(wlc, (T_NULLRSP_TPL_BASE + 8), (T_RAM_ACCESS_SZ*2),
	                       template_buf);
}


uint
wlc_get_rxratehistory(wlc_info_t *wlc, struct scb * scb)
{
	uint ratesum, i;

	if (!scb) {
		WL_NITRO(("wl%d %s: NULL scb pointer\n", wlc->pub->unit, __FUNCTION__));
		return 0;
	}
	WL_NITRO(("wl%d %s: scb pointer is NOT NULL\n", wlc->pub->unit, __FUNCTION__));
	for (i = ratesum = 0; i < NTXRATE; i++) {
		if (scb->rate_histo)
			ratesum += scb->rate_histo->rxrspec[i] & RATE_MASK;
	}
	ratesum /= NTXRATE;

	return ratesum;
}


static void
wlc_nitro_timer(void *hdl)
{
	wlc_nitro_t *nitro = (wlc_nitro_t *)hdl;

	wlc_nitro_mpend(nitro, nitro->mp.rxbitmap, 0);
}

static int
wlc_nitro_bcn_parse_nitro_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	wlc_nitro_t *nitro = (wlc_nitro_t *)ctx;
	wlc_info_t *wlc = nitro->wlc;
	wlc_bsscfg_t *cfg = data->cfg;
	uint8 buf[NITRO_GAMEIE_FIXEDLEN + 128 + NITRO_BCNPHY_LEN];
	nitro_bcnphy_ie_t *bcnphy;
	nitro_gameIE_t *nie;
	uint nie_len;
	nitro_gameIE_t *gameIE = nitro->gameIE;
	uint32 gminflen;

	if (data->ie == NULL)
		return BCME_OK;

	nie = (nitro_gameIE_t *)data->ie;
	nie_len = data->ie_len;

	gminflen = nie->len - (NITRO_GAMEIE_FIXEDLEN - TLV_HDR_LEN);
	bcopy((char *)&nie->vblankTsf, (char *)&gameIE->vblankTsf, sizeof(nie->vblankTsf));
	nitro->gameInfoLen = gminflen;
	bcopy((char *)&nie->gameInfo, (char *)&gameIE->gameInfo, gminflen);
	bcopy((char *)&nie->activeZone, (char *)&gameIE->activeZone, sizeof(nie->activeZone));

	/* Add game info IE if present */
	ASSERT(nie_len <= NITRO_GAMEIE_FIXEDLEN + 128);
	memcpy(buf, nie, nie_len);

	/* Add fake IE with RSSI + rate */
	bcnphy = (nitro_bcnphy_ie_t *) (buf + nie_len);
	bcnphy->id = 0;
	bcnphy->len = NITRO_BCNPHY_LEN - TLV_HDR_LEN;
	bcnphy->rssi = data->pparm->wrxh->rssi;
	store16_ua(&bcnphy->rate, data->pparm->rspec);

	/* Sendup the received beacon event */
	wlc_mac_event(wlc, WLC_E_BCNRX_MSG, &cfg->BSSID, WLC_E_STATUS_SUCCESS,
	              0, 0, buf, nie_len + NITRO_BCNPHY_LEN);

	return BCME_OK;
}
