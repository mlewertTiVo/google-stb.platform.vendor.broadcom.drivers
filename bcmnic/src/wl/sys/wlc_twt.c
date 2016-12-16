/*
 * 802.11ah/11ax Target Wake Time protocol and d11 h/w manipulation.
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
 * $Id: wlc_twt.c 650798 2016-07-22 13:12:03Z $
 */

#ifdef WLTWT
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <proto/802.11.h>
#include <proto/802.11ah.h>
#include <wl_dbg.h>
#include <wlc_types.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_ie_mgmt_vs.h>
#include <wlc_dump.h>
#include <wlc_iocv_cmd.h>
#include <wlc_twt.h>
#include <wlc_bsscfg.h>
#include <wlc_scb.h>

/* ======== structures and definitions ======== */

/* module info */
struct wlc_twt_info {
	wlc_info_t *wlc;
	int cfgh;
	int scbh;

	/* debug/counters */
	uint rx_mf_ie_errs;	/* parse failures of malformed TWT IEs */
	uint tx_short_bufs;	/* tx buf too short errors */
};

#define WLC_TWT_INV_FLOW_ID	0xFF
#define WLC_TWT_MAX_FLOW_ID	7

/* bsscfg cubby */
typedef struct {
	uint16 num_desc;	/* bcast twt (tmp., single, before having twt database) */
	wl_twt_sdesc_t desc[1];
} bss_twt_t;

#define BSS_TWT_CUBBY(twti, cfg) (bss_twt_t **)BSSCFG_CUBBY((cfg), (twti)->cfgh)
#define BSS_TWT(twti, cfg) *BSS_TWT_CUBBY(twti, cfg)

/* scb cubby */
typedef struct {
	uint8 fid_bmp;		/* indv twt flow id tracking */
} scb_twt_t;

#define SCB_TWT(twti, scb) (scb_twt_t *)SCB_CUBBY((scb), (twti)->scbh)

/* debug */
#define BCAST_TWT_IE_DUMP
#define IND_TWT_IE_DUMP
#define TWT_TEARDOWN_DUMP
#define TWT_INFO_DUMP
/* increment a counter by 1 */
#define TWTCNTINC(twti, cntr)	do {(twti)->cntr ++;} while (0)
#define WLUNIT(twti)	((twti)->wlc->pub->unit)

#ifdef BCMDBG
#define ERR_ONLY_VAR(x)	x
#define WL_TWT_ERR(x)	WL_ERROR(x)
#define INFO_ONLY_VAR(x) x
#define WL_TWT_INFO(x)	WL_ERROR(x)
#else
#define ERR_ONLY_VAR(x)
#define WL_TWT_ERR(x)
#define INFO_ONLY_VAR(x)
#define WL_TWT_INFO(x)
#endif

/* ======== local function declarations ======== */

/* wlc module */
static int wlc_twt_wlc_init(void *ctx);
static int wlc_twt_doiovar(void *ctx, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, struct wlc_if *wlcif);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int wlc_twt_dump(void *ctx, struct bcmstrbuf *b);
#endif

/* bsscfg cubby */
static int wlc_twt_bss_init(void *ctx, wlc_bsscfg_t *cfg);
static void wlc_twt_bss_deinit(void *ctx, wlc_bsscfg_t *cfg);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void wlc_twt_bss_dump(void *ctx, wlc_bsscfg_t *cfg, struct bcmstrbuf *b);
#else
#define wlc_twt_bss_dump NULL
#endif

/* scb cubby */
static int wlc_twt_scb_init(void *ctx, struct scb *scb);
static void wlc_twt_scb_deinit(void *ctx, struct scb *scb);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void wlc_twt_scb_dump(void *ctx, struct scb *scb, struct bcmstrbuf *b);
#else
#define wlc_twt_scb_dump NULL
#endif

/* IE mgmt */
static uint wlc_twt_calc_bcast_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_twt_write_bcast_ie(void *ctx, wlc_iem_build_data_t *data);
static int wlc_twt_parse_bcast_ie(void *ctx, wlc_iem_parse_data_t *data);

/* misc */
static bool wlc_twt_hw_cap(wlc_info_t *wlc);
static void wlc_twt_uint32_to_float(uint32 val, uint8 *exp, uint16 *mant);
static uint32 wlc_twt_float_to_uint32(uint8 exp, uint16 mant);

/* TWT IE */
static int wlc_twt_ie_parse(wlc_twt_info_t *twti, twt_ie_top_t *body, uint body_len,
	wl_twt_sdesc_t *desc, bool *req);

/* ======== iovar table ======== */
enum {
	IOV_TWT = 0,
	IOV_LAST = 1
};

static const bcm_iovar_t twt_iovars[] = {
	{"twt", IOV_TWT, 0, 0, IOVT_UINT32, 0},
	{NULL, 0, 0, 0, 0, 0}
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
*/
#include <wlc_patch.h>

/* ======== attach/detach ======== */

wlc_twt_info_t *
BCMATTACHFN(wlc_twt_attach)(wlc_info_t *wlc)
{
	wlc_twt_info_t *twti;
	uint16 twtfstbmp =
	        FT2BMP(FC_BEACON) |
	        0;

	/* sanity check */
	ASSERT(WLC_TWT_MAX_FLOW_ID < NBITS(sizeof(((scb_twt_t *)0)->fid_bmp)));

	/* allocate private module info */
	if ((twti = MALLOCZ(wlc->osh, sizeof(*twti))) == NULL) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	twti->wlc = wlc;

	/* reserve some space in bsscfg for private data */
	if ((twti->cfgh = wlc_bsscfg_cubby_reserve(wlc, sizeof(bss_twt_t *),
			wlc_twt_bss_init, wlc_twt_bss_deinit, wlc_twt_bss_dump,
			twti)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* reserve some space in scb for private data */
	if ((twti->scbh = wlc_scb_cubby_reserve(wlc, sizeof(scb_twt_t),
			wlc_twt_scb_init, wlc_twt_scb_deinit, wlc_twt_scb_dump,
			twti)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register IE mgmt callbacks - calc/build */

	/* bcn */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, twtfstbmp, DOT11_MNG_TWT_ID,
			wlc_twt_calc_bcast_ie_len, wlc_twt_write_bcast_ie, twti) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, twt ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register IE mgmt callbacks - parse */

	/* bcn */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, twtfstbmp, DOT11_MNG_TWT_ID,
			wlc_twt_parse_bcast_ie, twti) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, twt ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register module up/down, watchdog, and iovar callbacks */
	if (wlc_module_register(wlc->pub, twt_iovars, "twt", twti, wlc_twt_doiovar,
			NULL, wlc_twt_wlc_init, NULL)) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	/* debug dump */
	wlc_dump_register(wlc->pub, "twt", wlc_twt_dump, twti);
#endif

	/* enable TWT by default if possible */
	wlc->pub->_twt = wlc_twt_hw_cap(wlc) ? TRUE : FALSE;

	return twti;

fail:
	wlc_twt_detach(twti);
	return NULL;
}

void
BCMATTACHFN(wlc_twt_detach)(wlc_twt_info_t *twti)
{
	wlc_info_t *wlc;

	if (twti == NULL)
		return;

	wlc = twti->wlc;

	(void)wlc_module_unregister(wlc->pub, "twt", twti);

	MFREE(wlc->osh, twti, sizeof(*twti));
}

/* ======== bsscfg cubby ======== */

static int
wlc_twt_bss_init(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_twt_info_t *twti = ctx;
	wlc_info_t *wlc = twti->wlc;
	bss_twt_t **pbt = BSS_TWT_CUBBY(twti, cfg);
	bss_twt_t *bt = BSS_TWT(twti, cfg);


	/* sanity check */
	ASSERT(bt == NULL);

	if ((bt = MALLOCZ(wlc->osh, sizeof(*bt))) == NULL) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}
	*pbt = bt;

	return BCME_OK;
}

static void
wlc_twt_bss_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_twt_info_t *twti = ctx;
	wlc_info_t *wlc = twti->wlc;
	bss_twt_t **pbt = BSS_TWT_CUBBY(twti, cfg);
	bss_twt_t *bt = BSS_TWT(twti, cfg);


	/* sanity check */
	if (bt == NULL) {
		return;
	}

	MFREE(wlc->osh, bt, sizeof(*bt));
	*pbt = NULL;
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void
wlc_twt_bss_dump(void *ctx, wlc_bsscfg_t *cfg, struct bcmstrbuf *b)
{
	wlc_twt_info_t *twti = ctx;
	bss_twt_t *bt = BSS_TWT(twti, cfg);
	uint16 i;

	if (bt == NULL) {
		return;
	}

	bcm_bprintf(b, "Broadcast TWT IE (%d):\n", bt->num_desc);
	for (i = 0; i < bt->num_desc; i ++) {
		bcm_bprintf(b, "  desc %u>\n", i);
		bcm_bprintf(b, "    flow flags: 0x%x\n", bt->desc[i].flow_flags);
		bcm_bprintf(b, "    flow id: %u\n", bt->desc[i].flow_id);
		bcm_bprintf(b, "    target wake time: 0x%08x%08x\n",
		            bt->desc[i].wake_time_h, bt->desc[i].wake_time_l);
		bcm_bprintf(b, "    nominal min wake duration: 0x%x\n", bt->desc[i].wake_dur);
		bcm_bprintf(b, "    wake interval: 0x%x\n", bt->desc[i].wake_int);
	}
}
#endif /* BCMDBG) || BCMDBG_DUMP */


/* ======== scb cubby ======== */

static int
wlc_twt_scb_init(void *ctx, struct scb *scb)
{
	return BCME_OK;
}

static void
wlc_twt_scb_deinit(void *ctx, struct scb *scb)
{
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void
wlc_twt_scb_dump(void *ctx, struct scb *scb, struct bcmstrbuf *b)
{
	wlc_twt_info_t *twti = ctx;
	scb_twt_t *st = SCB_TWT(twti, scb);

	bcm_bprintf(b, "     flow id alloc: 0x%x\n", st->fid_bmp);
}
#endif /* BCMDBG) || BCMDBG_DUMP */


/* flow id manipulation */
static uint8
wlc_twt_fid_alloc(wlc_twt_info_t *twti, struct scb *scb)
{
	scb_twt_t *st = SCB_TWT(twti, scb);
	uint8 i;

	for (i = 0; i < NBITS(st->fid_bmp); i ++) {
		if (!(st->fid_bmp & (1<<i))) {
			st->fid_bmp |= (1<<i);
			return i;
		}
	}

	{
	ERR_ONLY_VAR(char eabuf[ETHER_ADDR_STR_LEN]);
	WL_TWT_ERR(("wl%d: %s: no flow left for peer %s!\n",
	            WLUNIT(twti), __FUNCTION__, bcm_ether_ntoa(&scb->ea, eabuf)));
	}

	return WLC_TWT_INV_FLOW_ID;
}

static void
wlc_twt_fid_free(wlc_twt_info_t *twti, struct scb *scb, uint8 fid)
{
	scb_twt_t *st = SCB_TWT(twti, scb);

	if (fid > WLC_TWT_MAX_FLOW_ID) {
		ERR_ONLY_VAR(char eabuf[ETHER_ADDR_STR_LEN]);
		WL_TWT_ERR(("wl%d: %s: peer %s flow id %u bad!\n",
		            WLUNIT(twti), __FUNCTION__,
		            bcm_ether_ntoa(&scb->ea, eabuf), fid));
		return;
	}

	st->fid_bmp &= ~(1<<fid);
}

static bool
wlc_twt_fid_taken(wlc_twt_info_t *twti, struct scb *scb, uint8 fid)
{
	scb_twt_t *st = SCB_TWT(twti, scb);

	ASSERT(fid <= WLC_TWT_MAX_FLOW_ID);

	return (st->fid_bmp & (1<<fid)) ? TRUE : FALSE;
}

static int
wlc_twt_fid_take(wlc_twt_info_t *twti, struct scb *scb, uint8 fid)
{
	scb_twt_t *st = SCB_TWT(twti, scb);

	if (fid > WLC_TWT_MAX_FLOW_ID) {
		ERR_ONLY_VAR(char eabuf[ETHER_ADDR_STR_LEN]);
		WL_TWT_ERR(("wl%d: %s: peer %s flow id %u bad!\n",
		            WLUNIT(twti), __FUNCTION__,
		            bcm_ether_ntoa(&scb->ea, eabuf), fid));
		return BCME_BADARG;
	}

	if (wlc_twt_fid_taken(twti, scb, fid)) {
		ERR_ONLY_VAR(char eabuf[ETHER_ADDR_STR_LEN]);
		WL_TWT_ERR(("wl%d: %s: peer %s flow id %u taken!\n",
		            WLUNIT(twti), __FUNCTION__,
		            bcm_ether_ntoa(&scb->ea, eabuf), fid));
		return BCME_BUSY;
	}

	st->fid_bmp |= (1<<fid);

	return BCME_OK;
}

/* validate flow id or assign flow id if auto, and reserve the flow id
 * and other needed resources including HW/ucode
 */
int
wlc_twt_reserve(wlc_twt_info_t *twti, struct scb *scb,
	wl_twt_sdesc_t *desc)
{
	/* assign an flow id */
	if (desc->flow_id == WLC_TWT_SETUP_FLOW_ID_AUTO) {
		uint8 flow_id = wlc_twt_fid_alloc(twti, scb);

		if (flow_id == WLC_TWT_INV_FLOW_ID) {
			return BCME_NORESOURCE;
		}

		desc->flow_id = flow_id;

		WL_TWT_INFO(("wl%d: %s: flow id %u assigned\n",
		             WLUNIT(twti), __FUNCTION__, flow_id));
		return BCME_OK;
	}

	/* validate and use the requested flow id */
	return wlc_twt_fid_take(twti, scb, desc->flow_id);
}

/* release flow id and other reserved resources */
void
wlc_twt_release(wlc_twt_info_t *twti, struct scb *scb, uint8 flow_id)
{
	if (!wlc_twt_fid_taken(twti, scb, flow_id)) {
		return;
	}
	wlc_twt_fid_free(twti, scb, flow_id);
}

/* ======== iovar dispatch ======== */

static int
wlc_twt_cmd_enab(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen,
	bool set, wlc_if_t *wlcif)
{
	wlc_twt_info_t *twti = ctx;
	wlc_info_t *wlc = twti->wlc;
	bcm_xtlv_t *xtlv = (bcm_xtlv_t *)params;

	if (set) {
		if (!wlc_twt_hw_cap(wlc)) {
			return BCME_UNSUPPORTED;
		}
		wlc->pub->_twt = *(uint8 *)(xtlv->data);
	}
	else {
		*result = wlc->pub->_twt;
		*rlen = sizeof(*result);
	}

	return BCME_OK;
}

enum {
	WL_TWT_CMD_ENAB = 0,
	WL_TWT_CMD_LAST
};

/*  TWT cmds  */
static const wlc_iov_cmd_t twt_cmds[] = {
	{WL_TWT_CMD_ENAB, 0, IOVT_UINT8, wlc_twt_cmd_enab},
};

static int
wlc_twt_doiovar(void *ctx, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, struct wlc_if *wlcif)
{
	int err = BCME_OK;

	BCM_REFERENCE(vsize);

	switch (actionid) {
	case IOV_GVAL(IOV_TWT):
		err = wlc_iocv_iov_cmd_proc(ctx, twt_cmds, ARRAYSIZE(twt_cmds),
			FALSE, params, plen, arg, alen, wlcif);
		break;
	case IOV_SVAL(IOV_TWT):
		err = wlc_iocv_iov_cmd_proc(ctx, twt_cmds, ARRAYSIZE(twt_cmds),
			TRUE, params, plen, arg, alen, wlcif);
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* ======== wlc module hooks ======== */

/* wlc init callback */
static int
wlc_twt_wlc_init(void *ctx)
{
	return BCME_OK;
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
/* debug dump */
static int
wlc_twt_dump(void *ctx, struct bcmstrbuf *b)
{
	wlc_twt_info_t *twti = ctx;
	wlc_info_t *wlc = twti->wlc;

	bcm_bprintf(b, "TWT Enab: %d\n", TWT_ENAB(wlc->pub));
	bcm_bprintf(b, " rx_mf_ie_errs: %u\n", twti->rx_mf_ie_errs);
	bcm_bprintf(b, " tx_short_bufs: %u\n", twti->tx_short_bufs);

	return BCME_OK;
}
#endif /* BCMDBG || BCMDBG_DUMP */

/* ======== IE mgmt hooks ======== */

/* Broadcast TWT IE */

/* return the length of the broadcast TWT IE */
static uint
wlc_twt_calc_bcast_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_twt_info_t *twti = ctx;
	wlc_bsscfg_t *cfg = data->cfg;
	bss_twt_t *bt = BSS_TWT(twti, cfg);

	ASSERT(bt != NULL);

	return wlc_twt_ie_len_calc(twti, bt->desc, bt->num_desc);
}

/* write the broadcast TWT IE into the frame */
static int
wlc_twt_write_bcast_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_twt_info_t *twti = ctx;
	wlc_bsscfg_t *cfg = data->cfg;
	bss_twt_t *bt = BSS_TWT(twti, cfg);

	ASSERT(bt != NULL);

	/* TODO: support multiple TWT */

	wlc_twt_ie_build(twti, bt->desc, bt->num_desc, data->buf, data->buf_len);

	return BCME_OK;
}

/* parse the broadcast TWT IE in the frame */
static int
wlc_twt_parse_bcast_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	wlc_twt_info_t *twti = ctx;
	wl_twt_sdesc_t desc;
#ifdef BCMDBG
	static struct {
		twt_ie_top_t top;
		twt_target_wake_time_t twt;
		twt_ie_bottom_t bottom;
	} last_bcast_twt_ie;
#endif
	int err;
	bool req;

	if (data->ie == NULL)
		return BCME_OK;

	if ((err = wlc_twt_ie_parse(twti, (twt_ie_top_t *)data->ie, data->ie_len,
			&desc, &req)) != BCME_OK) {
		WL_TWT_INFO(("wl%d: %s: bad broadcast TWT IE!\n",
		             WLUNIT(twti), __FUNCTION__));
		return err;
	}

	BCM_REFERENCE(req);

	/* TODO: now it's just dump the broadcast TWT, need to program the HW. */

#ifdef BCMDBG
	if (memcmp(&last_bcast_twt_ie, data->ie, sizeof(last_bcast_twt_ie)) != 0) {
#endif
#ifdef BCAST_TWT_IE_DUMP
	WL_PRINT(("Broadcast TWT IE:\n"));
	WL_PRINT(("  request: %d\n", req));
	WL_PRINT(("  setup cmd: %u\n", desc.setup_cmd));
	WL_PRINT(("  flow flags: 0x%x\n", desc.flow_flags));
	WL_PRINT(("  flow id: %u\n", desc.flow_id));
	WL_PRINT(("  target wake time: 0x%08x%08x\n", desc.wake_time_h, desc.wake_time_l));
	WL_PRINT(("  nominal min wake duration: 0x%x\n", desc.wake_dur));
	WL_PRINT(("  wake interval: 0x%x\n", desc.wake_int));
#endif
#ifdef BCMDBG
	memcpy(&last_bcast_twt_ie, data->ie, sizeof(last_bcast_twt_ie));
	}
#endif

	return BCME_OK;
}

/* ======== misc ======== */

/* is the hardware TWT capable */
static bool
wlc_twt_hw_cap(wlc_info_t *wlc)
{
#ifdef HE_MAC_SW_DEV
	return TRUE;
#else
	return FALSE;
#endif
}

/* is this connection TWT Requestor capable */
bool
wlc_twt_req_cap(wlc_twt_info_t *twti, wlc_bsscfg_t *cfg)
{
	return wlc_twt_hw_cap(twti->wlc);
}

/* is this connection TWT Responder capable */
bool
wlc_twt_resp_cap(wlc_twt_info_t *twti, wlc_bsscfg_t *cfg)
{
	return wlc_twt_hw_cap(twti->wlc);
}

/* convert an unsigned 32 bit integer to floating point representation
 * with 5 bit 2 based exponent and 16 bit mantissa.
 */
static void
wlc_twt_uint32_to_float(uint32 val, uint8 *exp, uint16 *mant)
{
	uint8 lzs = (uint8)CLZ(val); /* leading 0's */
	uint8 shift = lzs < 16 ? 16 - lzs : 0;

	*mant = (uint16)(val >> shift);
	*exp = shift;
}

static uint32
wlc_twt_float_to_uint32(uint8 exp, uint16 mant)
{
	return (uint32)mant << exp;
}

/* ======== Action Frame handling ======== */

/* return TWT IE length which includs the 2 TLV header bytes */
uint
wlc_twt_ie_len_calc(wlc_twt_info_t *twti, wl_twt_sdesc_t *desc, uint16 num_desc)
{
	uint len = sizeof(twt_ie_top_t) + sizeof(twt_ie_bottom_t);
	uint twt_len = sizeof(twt_target_wake_time_t);
	uint grping_len = 0;
	uint ndp_len = 0;

	/* TODO: support multiple TWTs */

	if (num_desc == 0) {
		return 0;
	}

	/* TODO: review sec 8.4.2.196 TWT element for optional fields... */

	if (desc->setup_cmd == TWT_SETUP_CMD_GRPING_TWT) {
		twt_len = 0;
		/* TODO: adjust TWT Grouping Assignment field length
		 * based on Zero Offset Present field...
		 */
		grping_len = sizeof(twt_grp_long_t);
	}

	/* TODO: adjust NDP Paging field length
	 * based on NDP Paging Indicator field...
	 */

	return len + grping_len + twt_len + ndp_len;
}

/* Check if the value of setup_cmd in the desc is one of the requests' */
static bool
wlc_twt_ie_is_req(wlc_twt_info_t *twti, wl_twt_sdesc_t *desc)
{
	switch (desc->setup_cmd) {
	case TWT_SETUP_CMD_REQUEST_TWT:
	case TWT_SETUP_CMD_SUGGEST_TWT:
	case TWT_SETUP_CMD_DEMAND_TWT:
		return TRUE;
	default:
		return FALSE;
	}
}

/* initialize TWT IE based on info in 'desc' */
void
wlc_twt_ie_build(wlc_twt_info_t *twti, wl_twt_sdesc_t *desc, uint16 num_desc,
	uint8 *body, uint body_len)
{
	twt_ie_top_t *twt_top;
	twt_target_wake_time_t *wake_time;
	twt_ie_bottom_t *twt_bottom;
	uint16 req_type;
	uint8 wake_exp;
	uint16 wake_mantissa;
	uint ie_len;

	/* IE length has TLV header included */
	ie_len = wlc_twt_ie_len_calc(twti, desc, num_desc);

	ASSERT(body_len >= ie_len);

	/* TODO: support multiple TWTs */

	wlc_twt_uint32_to_float(desc->wake_int, &wake_exp, &wake_mantissa);

	twt_top = (twt_ie_top_t *)body;
	twt_top->id = DOT11_MNG_TWT_ID;
	twt_top->len = ie_len - TLV_HDR_LEN;

	twt_top->ctrl = (desc->flow_flags & WL_TWT_FLOW_FLAG_BROADCAST) ?
	        TWT_CTRL_BCAST : 0;
	req_type = wlc_twt_ie_is_req(twti, desc) ? TWT_REQ_TYPE_REQUEST : 0;
	req_type |= (desc->setup_cmd << TWT_REQ_TYPE_SETUP_CMD_SHIFT) &
	        TWT_REQ_TYPE_SETUP_CMD_MASK;
	req_type |= (desc->flow_flags & WL_TWT_FLOW_FLAG_IMPLICIT) ?
	        TWT_REQ_TYPE_IMPLICIT : 0;
	req_type |= (desc->flow_flags & WL_TWT_FLOW_FLAG_UNANNOUNCED) ?
	        TWT_REQ_TYPE_FLOW_TYPE : 0;
	req_type |= (desc->flow_flags & WL_TWT_FLOW_FLAG_TRIGGER) ?
	        TWT_REQ_TYPE_TRIGGER : 0;
	req_type |= (desc->flow_id << TWT_REQ_TYPE_FLOW_ID_SHIFT) &
	        TWT_REQ_TYPE_FLOW_ID_MASK;
	req_type |= (wake_exp << TWT_REQ_TYPE_WAKE_EXP_SHIFT) &
	        TWT_REQ_TYPE_WAKE_EXP_MASK;
	store16_ua((uint8 *)&twt_top->req_type, req_type);

	if (desc->setup_cmd == TWT_SETUP_CMD_GRPING_TWT) {
		/* TODO: adjust TWT Grouping Assignment field length
		 * based on Zero Offset Present field...
		 */
		twt_grp_long_t *grp_ass = (twt_grp_long_t *)(twt_top + 1);
		twt_bottom = (twt_ie_bottom_t *)(grp_ass + 1);
	}
	else {
		uint32 wake_time_l = htol32(desc->wake_time_l);
		uint32 wake_time_h = htol32(desc->wake_time_h);
		wake_time = (twt_target_wake_time_t *)(twt_top + 1);
		store32_ua((uint8 *)&((uint32 *)wake_time)[0], wake_time_l);
		store32_ua((uint8 *)&((uint32 *)wake_time)[1], wake_time_h);
		twt_bottom = (twt_ie_bottom_t *)(wake_time + 1);
	}

	twt_bottom->nom_wake_dur = desc->wake_dur / TWT_NOM_WAKE_DUR_UNIT;
	wake_mantissa = htol16(wake_mantissa);
	store16_ua((uint8 *)&twt_bottom->wake_int_mant, wake_mantissa);
	twt_bottom->channel = 0;

	/* TODO: parse NDP Paging field if NDP Paging Indicator field is 1... */
}

/* parse TWT IE and fill up 'desc' */
static int
wlc_twt_ie_parse(wlc_twt_info_t *twti, twt_ie_top_t *body, uint body_len,
	wl_twt_sdesc_t *desc, bool *req)
{
	twt_ie_top_t *twt_top;
	twt_target_wake_time_t *wake_time;
	twt_ie_bottom_t *twt_bottom;
	uint16 req_type;
	uint8 wake_exp;
	uint16 wake_mantissa;

	bzero(desc, sizeof(*desc));

	/* TODO: validate IE value length against body_len if variable length IE
	 * with optional fields is being parsed...
	 */

	/* initial buffer length check */
	if (body_len <= OFFSETOF(twt_ie_top_t, ctrl)) {
		TWTCNTINC(twti, rx_mf_ie_errs);
		return BCME_BUFTOOSHORT;
	}

	twt_top = (twt_ie_top_t *)body;
	desc->flow_flags |= (twt_top->ctrl & TWT_CTRL_BCAST) ?
	        WL_TWT_FLOW_FLAG_BROADCAST : 0;
	req_type = ltoh16_ua((uint8 *)&twt_top->req_type);
	*req = (req_type & TWT_REQ_TYPE_REQUEST) ? TRUE : FALSE;
	desc->setup_cmd = (req_type & TWT_REQ_TYPE_SETUP_CMD_MASK) >>
	        TWT_REQ_TYPE_SETUP_CMD_SHIFT;
	desc->flow_flags |= (req_type & TWT_REQ_TYPE_IMPLICIT) ?
	        WL_TWT_FLOW_FLAG_IMPLICIT : 0;
	desc->flow_flags |= (req_type & TWT_REQ_TYPE_FLOW_TYPE) ?
	        WL_TWT_FLOW_FLAG_UNANNOUNCED : 0;
	desc->flow_flags |= (req_type & TWT_REQ_TYPE_TRIGGER) ?
	        WL_TWT_FLOW_FLAG_TRIGGER : 0;
	desc->flow_id = (req_type & TWT_REQ_TYPE_FLOW_ID_MASK) >>
	        TWT_REQ_TYPE_FLOW_ID_SHIFT;
	wake_exp = (req_type & TWT_REQ_TYPE_WAKE_EXP_MASK) >>
	        TWT_REQ_TYPE_WAKE_EXP_SHIFT;

	/* at this point we have all info in order to calculate
	 * the expected IE length so let's validate the buffer legnth
	 * again...
	 */
	if (body_len < wlc_twt_ie_len_calc(twti, desc, 1)) {
		TWTCNTINC(twti, rx_mf_ie_errs);
		return BCME_BUFTOOSHORT;
	}

	if (desc->setup_cmd == TWT_SETUP_CMD_GRPING_TWT) {
		/* TODO: adjust TWT Grouping Assignment field length
		 * based on Zero Offset Present field...
		 */
		twt_grp_long_t *grp_ass = (twt_grp_long_t *)(twt_top + 1);
		twt_bottom = (twt_ie_bottom_t *)(grp_ass + 1);
	}
	else {
		wake_time = (twt_target_wake_time_t *)(twt_top + 1);
		desc->wake_time_l = ltoh32_ua((uint8 *)&((uint32 *)wake_time)[0]);
		desc->wake_time_h = ltoh32_ua((uint8 *)&((uint32 *)wake_time)[1]);
		twt_bottom = (twt_ie_bottom_t *)(wake_time + 1);
	}

	desc->wake_dur = twt_bottom->nom_wake_dur * TWT_NOM_WAKE_DUR_UNIT;
	wake_mantissa = ltoh16_ua((uint8 *)&twt_bottom->wake_int_mant);

	desc->wake_int = wlc_twt_float_to_uint32(wake_exp, wake_mantissa);

	return BCME_OK;
}

/* decide how to respond to the request... */
static int
wlc_twt_setup_resp(wlc_twt_info_t *twti, struct scb *scb, wl_twt_sdesc_t *desc)
{
	wlc_info_t *wlc = twti->wlc;
	int err = BCME_OK;

	switch (desc->setup_cmd) {
	case TWT_SETUP_CMD_REQUEST_TWT:
		/* TODO: fill up setup_cmd in desc... */
		desc->setup_cmd = TWT_SETUP_CMD_ACCEPT_TWT;
		break;
	case TWT_SETUP_CMD_SUGGEST_TWT:
		/* TODO: fill up setup_cmd in desc... */
		desc->setup_cmd = TWT_SETUP_CMD_ACCEPT_TWT;
		break;
	case TWT_SETUP_CMD_DEMAND_TWT:
		/* TODO: fill up setup_cmd in desc... */
		desc->setup_cmd = TWT_SETUP_CMD_ACCEPT_TWT;
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	/* TODO: fill up wake_time in desc... */
	wlc_read_tsf(wlc, &desc->wake_time_l, &desc->wake_time_h);

	return err;
}

/* parse TWT IE */
/* input: expect 'body' points to TWT IE */
/* output: unpacked IE and internal information in 'desc' */
int
wlc_twt_ie_proc(wlc_twt_info_t *twti, struct scb *scb,
	twt_ie_top_t *body, uint body_len, wl_twt_sdesc_t *desc, bool *resp)
{
	int err;
	bool req;

	/* parse TWT IE */
	if ((err = wlc_twt_ie_parse(twti, body, body_len, desc, &req)) != BCME_OK) {
		INFO_ONLY_VAR(char eabuf[ETHER_ADDR_STR_LEN]);
		WL_TWT_INFO(("wl%d: %s: peer %s bad TWT IE!\n",
		             WLUNIT(twti), __FUNCTION__,
		             bcm_ether_ntoa(&scb->ea, eabuf)));
		return err;
	}

#ifdef IND_TWT_IE_DUMP
	WL_PRINT(("Setup TWT IE:\n"));
	WL_PRINT(("  request: %d\n", req));
	WL_PRINT(("  setup cmd: %u\n", desc->setup_cmd));
	WL_PRINT(("  flow flags: 0x%x\n", desc->flow_flags));
	WL_PRINT(("  flow id: %u\n", desc->flow_id));
	WL_PRINT(("  target wake time: 0x%08x%08x\n", desc->wake_time_h, desc->wake_time_l));
	WL_PRINT(("  nominal min wake duration: %u\n", desc->wake_dur));
	WL_PRINT(("  wake interval: %u\n", desc->wake_int));
#endif

	/* Setup Request - the caller needs to respond */
	if (req &&
	    wlc_twt_ie_is_req(twti, desc)) {
		/* validate the request & give feedback if possible */
		if ((err = wlc_twt_setup_resp(twti, scb, desc)) != BCME_OK) {
			WL_TWT_INFO(("wl%d: %s: unable to make decision!\n",
			             WLUNIT(twti), __FUNCTION__));
			return err;
		}
		*resp = TRUE;
	}
	/* Setup Response, or request is invalid */
	else {
		*resp = FALSE;
	}

	return BCME_OK;
}

uint
wlc_twt_teardown_len_calc(wlc_twt_info_t *twti)
{
	return sizeof(uint8);
}

/* twt teardown TWT Flow field init */
void
wlc_twt_teardown_init(wlc_twt_info_t *twti, uint8 flow_id,
	uint8 *body, uint body_len)
{
	ASSERT(body_len >= wlc_twt_teardown_len_calc(twti));

	*body = flow_id & TWT_TEARDOWN_FLOW_ID_MASK;
}

/* twt teardown process */
/* expect 'body' points to TWT Flow field */
int
wlc_twt_teardown_proc(wlc_twt_info_t *twti, struct scb *scb,
	uint8 *body, uint body_len)
{
	uint8 flow_id = *body & TWT_TEARDOWN_FLOW_ID_MASK;

#ifdef TWT_TEARDOWN_DUMP
	WL_PRINT(("TWT Teardown:\n"));
	WL_PRINT((" flow_id: %u\n", flow_id));
#endif

	wlc_twt_fid_free(twti, scb, flow_id);

	return BCME_OK;
}

/* Next TWT field legnt (byte count) */
static const uint8 next_twt_len[] = {0, 4, 6, 8};

/* determine Next TWT Subfield Size field value */
static uint8
twt_info_next_twt_sz(uint32 next_twt_h, uint32 next_twt_l)
{
	uint8 twt_len_idx;

	if (next_twt_h == 0) {
		if (next_twt_l == 0) {
			twt_len_idx = TWT_INFO_NEXT_TWT_SIZE_0;
		}
		else {
			twt_len_idx = TWT_INFO_NEXT_TWT_SIZE_32;
		}
	}
	else if (CLZ(next_twt_h) >= 16) {
		twt_len_idx = TWT_INFO_NEXT_TWT_SIZE_48;
	}
	else {
		twt_len_idx = TWT_INFO_NEXT_TWT_SIZE_64;
	}

	return twt_len_idx;
}

/* calculate TWT Information size */
/* NOTES: next_twt_h/next_twt_l must be populated */
uint
wlc_twt_info_len_calc(wlc_twt_info_t *twti, wl_twt_idesc_t *desc)
{
	uint8 next_twt_sz = twt_info_next_twt_sz(desc->next_twt_h, desc->next_twt_l);

	return sizeof(uint8) + next_twt_len[next_twt_sz];
}

/* build TWT Information */
void
wlc_twt_info_init(wlc_twt_info_t *twti, wl_twt_idesc_t *desc,
	uint8 *body, uint body_len)
{
	bool next_twt;
	uint8 next_twt_sz;
	uint32 next_twt_h, next_twt_l;

	ASSERT(body_len >= wlc_twt_info_len_calc(twti, desc));

	next_twt = desc->next_twt_h != 0 || desc->next_twt_l != 0;
	next_twt_sz = twt_info_next_twt_sz(desc->next_twt_h, desc->next_twt_l);
	ASSERT(next_twt == (next_twt_sz > 0));

	*body = desc->flow_id & TWT_INFO_FLOW_ID_MASK;
	*body |= (desc->flow_flags & WL_TWT_INFO_FLAG_RESP_REQ) ? TWT_INFO_RESP_REQ : 0;
	*body |= next_twt ? TWT_INFO_NEXT_TWT_REQ : 0;
	*body |= (next_twt_sz << TWT_INFO_NEXT_TWT_SIZE_SHIFT) &
	        TWT_INFO_NEXT_TWT_SIZE_MASK;

	if (next_twt_sz > 0) {
		body += sizeof(uint8);

		next_twt_h = htol32(desc->next_twt_h);
		next_twt_l = htol32(desc->next_twt_l);
		switch (next_twt_sz) {
		case TWT_INFO_NEXT_TWT_SIZE_64:
			store32_ua((uint8 *)&((uint32 *)body)[0], next_twt_l);
			store32_ua((uint8 *)&((uint32 *)body)[1], next_twt_h);
			break;
		case TWT_INFO_NEXT_TWT_SIZE_48:
			store16_ua((uint8 *)&((uint32 *)body)[1], (uint16)next_twt_h);
			/* fall through */
		case TWT_INFO_NEXT_TWT_SIZE_32:
			store32_ua((uint8 *)&((uint32 *)body)[0], next_twt_l);
			break;
		}
	}
}

/* parse TWT Information */
/* expect 'body' points to TWT Information field */
int
wlc_twt_info_proc(wlc_twt_info_t *twti, struct scb *scb,
	uint8 *body, uint body_len, wl_twt_idesc_t *desc, bool *resp)
{
	bool next_twt;
	uint8 next_twt_sz;
	uint32 next_twt_h = 0, next_twt_l = 0;

	if (body_len < (uint)sizeof(uint8)) {
		TWTCNTINC(twti, rx_mf_ie_errs);
		return BCME_BUFTOOSHORT;
	}

	bzero(desc, sizeof(*desc));

	desc->flow_id = *body & TWT_INFO_FLOW_ID_MASK;
	desc->flow_flags |= (*body & TWT_INFO_RESP_REQ) ? WL_TWT_INFO_FLAG_RESP_REQ : 0;
	next_twt = (*body & TWT_INFO_NEXT_TWT_REQ) ? TRUE : 0;
	next_twt_sz = (*body & TWT_INFO_NEXT_TWT_SIZE_MASK) >>
	        TWT_INFO_NEXT_TWT_SIZE_SHIFT;

	if (next_twt > 0) {
		body += sizeof(uint8);

		if (body_len < sizeof(uint8) + next_twt_len[next_twt_sz]) {
			TWTCNTINC(twti, rx_mf_ie_errs);
			return BCME_BUFTOOSHORT;
		}

		switch (next_twt_sz) {
		case TWT_INFO_NEXT_TWT_SIZE_64:
			next_twt_l = ltoh32_ua((uint8 *)&((uint32 *)body)[0]);
			next_twt_h = ltoh16_ua((uint8 *)&((uint32 *)body)[1]);
			break;
		case TWT_INFO_NEXT_TWT_SIZE_48:
			next_twt_h = ltoh16_ua((uint8 *)&((uint32 *)body)[1]);
			/* fall through */
		case TWT_INFO_NEXT_TWT_SIZE_32:
			next_twt_l = ltoh32_ua((uint8 *)&((uint32 *)body)[0]);
			break;
		}
		desc->next_twt_h = ltoh32(next_twt_h);
		desc->next_twt_l = ltoh32(next_twt_l);
	}

	*resp = (desc->flow_flags & WL_TWT_INFO_FLAG_RESP_REQ) ? TRUE : FALSE;

#ifdef TWT_INFO_DUMP
	WL_PRINT(("TWT Information:\n"));
	WL_PRINT((" flow_flags: %u\n", desc->flow_flags));
	WL_PRINT((" flow_id: %u\n", desc->flow_id));
	WL_PRINT((" next_twt_sz: %u\n", next_twt_sz));
	WL_PRINT((" next_twt: 0x%08x%08x\n", desc->next_twt_h, desc->next_twt_l));
#endif

	return BCME_OK;
}

/* ======== HEB plumbing ======== */

/* configure bcast twts and program the twt parms */
int
wlc_twt_setup_bcast(wlc_twt_info_t *twti, wlc_bsscfg_t *cfg, wl_twt_sdesc_t *desc)
{
	wlc_info_t *wlc = twti->wlc;
	bss_twt_t *bt = BSS_TWT(twti, cfg);

	ASSERT(bt != NULL);

	/* TODO: validate time line */

	bt->num_desc = 1;
	bt->desc[0] = *desc;

	wlc_suspend_mac_and_wait(wlc);
	wlc_bss_update_beacon(wlc, cfg);
	wlc_enable_mac(wlc);

	return BCME_OK;
}

/* plumb twt parms */
int
wlc_twt_setup(wlc_twt_info_t *twti, struct scb *scb, wl_twt_sdesc_t *desc)
{
	return BCME_OK;
}

/* twt teardown */
int
wlc_twt_teardown_bcast(wlc_twt_info_t *twti, wlc_bsscfg_t *cfg, uint8 flow_id)
{
	wlc_info_t *wlc = twti->wlc;
	bss_twt_t *bt = BSS_TWT(twti, cfg);

	ASSERT(bt != NULL);

	bt->num_desc = 0;
	bzero(bt->desc, sizeof(bt->desc));

	wlc_suspend_mac_and_wait(wlc);
	wlc_bss_update_beacon(wlc, cfg);
	wlc_enable_mac(wlc);

	return BCME_OK;
}

/* twt teardown */
int
wlc_twt_teardown(wlc_twt_info_t *twti, struct scb *scb, uint8 flow_id)
{
	/* TODO: program the HW */

	wlc_twt_release(twti, scb, flow_id);

	return BCME_OK;
}
#endif /* WLTWT */
