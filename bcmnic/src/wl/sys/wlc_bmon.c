/**
 * Ucode 'monitored BSSID' feature driver support source
 *
 * @file
 * @brief
 * Ucode compares the incoming packet's a3 (if any) with the configured
 * monitored BSSID(s) (currently only one) and passes the packet up to
 * the driver if the a3 matches one of the BSSID(s).
 *
 * This driver code adds support for users to configure the BSSID(s) and
 * to register the callbacks to be invoked when a matching packet comes.
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
 * $Id: wlc_bmon.c 619400 2016-02-16 13:52:43Z $
 */

/*
 * Future development:
 * As stated above the ucode just compares the a3 of incoming packets
 * as of now even though the feature calls it "monitored BSSID".
 * To make it real BSSID monitor the ucode may need to use FC.FROMDS
 * and FC.TODS flags to determine which address to use to match the
 * configured monitored BSSID(s).
 */


/*
 * TODO: future development
 *
 * - Provide other common filters such as frame type in addition to BSSID.
 */

#include <wlc_cfg.h>
#ifdef WLBMON
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <siutils.h>
#include <d11.h>
#include <wlioctl.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_bmac.h>
#include <bcm_notif_pub.h>
#include <wlc_bmon.h>
#include <wlc_dump.h>

/* ===== iovar table ===== */

enum {
	IOV_BMON_BSSID,		/* plumb BSSID */
};

static const bcm_iovar_t bmon_iovars[] = {
	{NULL, 0, 0, 0, 0, 0}
};

/* ===== data structure and definitions ===== */

/* private module states */
struct wlc_bmon_info {
	wlc_info_t *wlc;
	uint16 mtbl_offset;		/* match table offset in bmon_mem_t */
};

/* match entry and table */
typedef struct {
	mbool user;
	int ref_cnt;	/* TODO: add an bcm_notif API to return the # of registered entries */
	struct ether_addr bssid;
	bcm_notif_h notif_hdl;	/* pkt recv'd notifier handle */
/* ==== please keep these debug stuff at the bottom ==== */
#if defined(BMONDBG)
struct {
	uint match;	/* # matching packets */
} stats;
#endif 
} bmon_match_t;

/* counter/stats access macros */
#if defined(BMONDBG)
#define MATCHCNTINC(me)	((me)->stats.match++)
#else
#define MATCHCNTINC(me)
#endif 

/* # match entries */
#define BMON_MAX_NBSSID 1	/* at the moment ucode supports only one monitored BSSID */

/* memory layout */
typedef struct {
	wlc_bmon_info_t bmi;			/* private states */
	bmon_match_t mtbl[BMON_MAX_NBSSID];	/* match table */
} bmon_mem_t;
#define BMON_MTBL(bmi)	(bmon_match_t *)((uintptr)bmi + (bmi)->mtbl_offset)

/* ===== local symbol delarations ===== */

/* module entries */
static int wlc_bmon_doiovar(void *ctx, uint32 actionid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif);
static int wlc_bmon_up(void *ctx);
static int wlc_bmon_down(void *ctx);

/* ===== implementations ===== */

/* module entries */
wlc_bmon_info_t *
BCMATTACHFN(wlc_bmon_attach)(wlc_info_t *wlc)
{
	wlc_bmon_info_t *bmi;

	/* Allocate private states struct. */
	if ((bmi = MALLOCZ(wlc->osh, sizeof(bmon_mem_t))) == NULL) {
		WL_ERROR(("wl%d: %s: MALLOCZ failed, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->pub->osh)));
		goto fail;
	}
	bmi->wlc = wlc;
	bmi->mtbl_offset = OFFSETOF(bmon_mem_t, mtbl);

	/* Register module entries. */
	if (wlc_module_register(wlc->pub, bmon_iovars, "bmon", bmi, wlc_bmon_doiovar,
	                        NULL, wlc_bmon_up, wlc_bmon_down) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}


	return bmi;

fail:
	MODULE_DETACH(bmi, wlc_bmon_detach);
	return NULL;
}

void
BCMATTACHFN(wlc_bmon_detach)(wlc_bmon_info_t *bmi)
{
	wlc_info_t *wlc;

	if (bmi == NULL)
		return;

	wlc = bmi->wlc;

	wlc_module_unregister(wlc->pub, "bmon", bmi);

	MFREE(wlc->osh, bmi, sizeof(bmon_mem_t));
}


/* handle related iovars */
static int
wlc_bmon_doiovar(void *ctx, uint32 actionid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	wlc_bmon_info_t *bmi = (wlc_bmon_info_t *)ctx;
	int err = BCME_OK;

	(void)bmi;

	switch (actionid) {

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* wlc UP callback */
static int
wlc_bmon_up(void *ctx)
{
	wlc_bmon_info_t *bmi = (wlc_bmon_info_t *)ctx;
	wlc_info_t *wlc = bmi->wlc;

	if (wlc->clk) {
		W_REG(wlc->osh, &wlc->regs->u_rcv.d11regs.rcm_ctl, 0xa);
		W_REG(wlc->osh, &wlc->regs->u_rcv.d11regs.rcm_cond_dly, 0x020e);
	}

	return BCME_OK;
}

static int
wlc_bmon_down(void *ctx)
{

	return BCME_OK;
}

/* debug... */

/* program the BSSID into the h/w, enable the feature if needed */
static int
wlc_bmon_bssid_set(wlc_bmon_info_t *bmi, const struct ether_addr *addr, int idx)
{
	wlc_info_t *wlc = bmi->wlc;

	if (!wlc->clk)
		return BCME_NOCLK;

	ASSERT(idx == 0);
	/* TODO: calc the right h/w location based on the index */
	if (D11REV_LT(wlc->pub->corerev, 40))
		wlc_bmac_set_rxe_addrmatch(wlc->hw, RCM_F_BSSID_0_OFFSET, addr);
	else {
		ASSERT(0);
	}

	/* TODO: check 'wlc->pub->_bmon' and enable the host flag */

	return BCME_OK;
}

/* add/del BSSID */
int
wlc_bmon_bssid_add(wlc_bmon_info_t *bmi, wlc_bmon_reg_info_t *reg)
{
	wlc_info_t *wlc = bmi->wlc;
	bmon_match_t *mtbl = BMON_MTBL(bmi);
	int i;
	struct ether_addr *addr;
	int err;

	ASSERT(reg != NULL);
	ASSERT(reg->bssid != NULL);

	ASSERT(mtbl != NULL);

	addr = reg->bssid;

	/* find any existing entry first */
	for (i = 0; i < BMON_MAX_NBSSID; i ++) {
		if (mtbl[i].ref_cnt != 0 &&
		    bcmp(addr, &mtbl[i].bssid, ETHER_ADDR_LEN) == 0)
			break;
	}
	/* find an empty if no existing entry */
	if (i >= BMON_MAX_NBSSID) {
		for (i = 0; i < BMON_MAX_NBSSID; i ++) {
			if (mtbl[i].ref_cnt == 0)
				break;
		}
	}
	if (i >= BMON_MAX_NBSSID)
		return BCME_NORESOURCE;

	/* bail out if this is our entry (to prevent duplication) */
	if (mtbl[i].user & reg->user)
		return BCME_BUSY;

	/* Create notification list to notify clients of
	 * packet recevied with matching BSSID.
	 */
	if (mtbl[i].ref_cnt == 0 &&
	    (err = bcm_notif_create_list(wlc->notif, &mtbl[i].notif_hdl)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: bcm_notif_create_list failed\n",
		          wlc->pub->unit, __FUNCTION__));
		return err;
	}

	/* Add client callback to the notification list */
	if ((err = bcm_notif_add_interest(mtbl[i].notif_hdl,
	                (bcm_notif_client_callback)reg->fn, reg->arg)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: unable to register callback %p\n",
		          wlc->pub->unit, __FUNCTION__, OSL_OBFUSCATE_BUF(reg->fn)));
		return err;
	}
	mtbl[i].ref_cnt ++;

	/* update wlc->pub->_bmon before calling wlc_bmon_bssid_set() */
	wlc->pub->_bmon = i < BMON_MAX_NBSSID;

	mboolset(mtbl[i].user, reg->user);

	bcopy(addr, &mtbl[i].bssid, ETHER_ADDR_LEN);
	wlc_bmon_bssid_set(bmi, addr, i);
	return BCME_OK;
}

int
wlc_bmon_bssid_del(wlc_bmon_info_t *bmi, wlc_bmon_reg_info_t *reg)
{
	wlc_info_t *wlc = bmi->wlc;
	bmon_match_t *mtbl = BMON_MTBL(bmi);
	int i, ii;
	struct ether_addr *addr;
	int err;

	ASSERT(reg != NULL);
	ASSERT(reg->bssid != NULL);

	ASSERT(mtbl != NULL);

	addr = reg->bssid;

	/* find the entry */
	for (i = 0; i < BMON_MAX_NBSSID; i ++) {
		if (mtbl[i].ref_cnt != 0 &&
		    bcmp(addr, &mtbl[i].bssid, ETHER_ADDR_LEN) == 0)
			break;
	}
	if (i >= BMON_MAX_NBSSID)
		return BCME_NOTFOUND;

	/* bail out if this is not our entry */
	if (!(mtbl[i].user & reg->user))
		return BCME_NOTFOUND;

	/* remove the client from the notification list */
	if ((err = bcm_notif_remove_interest(mtbl[i].notif_hdl,
	                (bcm_notif_client_callback)reg->fn, reg->arg)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: unable to register callback %p\n",
		          wlc->pub->unit, __FUNCTION__, OSL_OBFUSCATE_BUF(reg->fn)));
		return err;
	}
	mtbl[i].ref_cnt --;
	ASSERT(mtbl[i].ref_cnt >= 0);

	/* delete the notification list if not needed any more */
	if (mtbl[i].ref_cnt == 0 &&
	    mtbl[i].notif_hdl != NULL)
		bcm_notif_delete_list(&mtbl[i].notif_hdl);

	/* update wlc->pub->_bmon before calling wlc_bmon_bssid_set() */
	for (ii = 0; ii < BMON_MAX_NBSSID; ii ++) {
		if (mtbl[ii].ref_cnt != 0)
			break;
	}
	wlc->pub->_bmon = ii < BMON_MAX_NBSSID;

	mboolclr(mtbl[i].user, reg->user);

	if (mtbl[i].ref_cnt == 0) {
		bzero(&mtbl[i].bssid, ETHER_ADDR_LEN);
		wlc_bmon_bssid_set(bmi, &ether_null, i);
	}
	return BCME_OK;
}

/* Does the given received packet match with one of our entries?
 * It returns the entry index to the matching table if found;
 * returns -1 otherwise.
 */
int
wlc_bmon_pktrx_match(wlc_bmon_info_t *bmi, struct dot11_header *hdr)
{
	bmon_match_t *mtbl = BMON_MTBL(bmi);
	struct ether_addr *addr;
	uint16 fc;
	int i;

	ASSERT(mtbl != NULL);

	fc = ltoh16(hdr->fc);

	if (FC_TYPE(fc) == FC_TYPE_MNG ||
	    (FC_TYPE(fc) == FC_TYPE_DATA &&
	     (fc & (FC_FROMDS|FC_TODS)) == 0)) {
		addr = &hdr->a3;

		for (i = 0; i < BMON_MAX_NBSSID; i ++) {
			if (mtbl[i].ref_cnt != 0 &&
			    bcmp(addr, &mtbl[i].bssid, ETHER_ADDR_LEN) == 0) {
				MATCHCNTINC(&mtbl[i]);
				return i;
			}
		}
	}

	return -1;
}

/* notify clients of packet received */
void
wlc_bmon_pktrx_notif(wlc_bmon_info_t *bmi, wlc_bmon_pktrx_data_t *notif_data)
{
	bmon_match_t *mtbl = BMON_MTBL(bmi);
	int me;

	ASSERT(notif_data != NULL);

	me = notif_data->me;
	ASSERT(me >= 0 && me < BMON_MAX_NBSSID);

	bcm_notif_signal(mtbl[me].notif_hdl, notif_data);
}

#endif /* WLBMON */
