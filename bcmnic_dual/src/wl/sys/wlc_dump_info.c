/** \file wlc_dump_info.c
 *
 * Common (OS-independent) info dump portion of Broadcom 802.11bang Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_dump_info.c 451514 2014-01-27 00:24:19Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmdevs.h>
#include <pcicfg.h>
#include <siutils.h>
#include <bcmendian.h>
#include <nicpci.h>
#include <wlioctl.h>
#include <pcie_core.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_key.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wl_export.h>
#include <wl_dbg.h>
#include <wlc_channel.h>
#include <wlc_stf.h>
#include <wlc_bmac.h>
#include <wlc_scan.h>
#include <wlc_rm.h>
#include <wlc_addrmatch.h>
#include <wlc_ampdu.h>
#include <bcmnvram.h>
#ifdef WLC_HIGH_ONLY
#include <bcm_rpc_tp.h>
#include <bcm_rpc.h>
#include <bcm_xdr.h>
#include <wlc_rpc.h>
#include <wlc_rpctx.h>
#endif /* WLC_HIGH_ONLY */
#ifdef WL11K
#include <wlc_rrm.h>
#endif /* WL11K */

#include <wlc_dump_info.h>

/* Shared memory location index for various AC params */
#define wme_shmemacindex(ac)	wme_ac2fifo[ac]

#if defined(BCMDBG_PHYDUMP) || defined(TDLS_TESTBED) || defined(BCMDBG_AMPDU) || \
	defined(BCMDBG_DUMP_RSSI) || defined(MCHAN_MINIDUMP)
static char* wlc_dump_next_name(char **buf);
static int wlc_dump_registered_name(wlc_info_t *wlc, char *name, struct bcmstrbuf *b);
#endif 
#ifdef WLTINYDUMP
static int wlc_tinydump(wlc_info_t *wlc, struct bcmstrbuf *b);
#endif /* WLTINYDUMP */


extern uint8 wme_ac2fifo[];
#if defined(WLMSG_INFORM)
extern const char *const aci_names[];
#endif

#if defined(BCMDBG_PHYDUMP) || defined(BCMDBG_AMPDU) || defined(BCMDBG_DUMP_RSSI) || \
	defined(MCHAN_MINIDUMP) || defined(WLTINYDUMP)
#define IOV_DUMP_DEF_IOVAR_ENAB 1
#else
#define IOV_DUMP_DEF_IOVAR_ENAB 0
#endif 


#define IOV_DUMP_NVM_IOVAR_ENAB 0

#if (IOV_DUMP_NVM_IOVAR_ENAB || IOV_DUMP_DEF_IOVAR_ENAB)
#define IOV_DUMP_ANY_IOVAR_ENAB 1
#else
#define IOV_DUMP_ANY_IOVAR_ENAB 0
#endif /* IOV_DUMP_NVM_IOVAR_ENAB || IOV_DUMP_DEF_IOVAR_ENAB */


#if defined(BCMDBG_PHYDUMP)
#if defined(BCMDBG_PHYDUMP)
static int
wlc_dump_phy_radioreg(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	int err = 0;
	if ((err = wlc_iocregchk(wlc, WLC_BAND_AUTO)))
		return err;

	if ((err = wlc_iocpichk(wlc, wlc->band->phytype)))
		return err;

	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_RADIOREG_ID);
	return 0;
}

static int
wlc_dump_phy_phyreg(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	int err = 0;
	if ((err = wlc_iocregchk(wlc, WLC_BAND_AUTO)))
		return err;

	if ((err = wlc_iocpichk(wlc, wlc->band->phytype)))
		return err;

	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHYREG_ID);
	return 0;
}
#endif 


static int
wlc_dump_phy_cal(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_PHYCAL_ID);
	return 0;
}

static int
wlc_dump_phy_aci(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_ACI_ID);
	return 0;
}

static int
wlc_dump_phy_papd(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_PAPD_ID);

	if (b->size == b->origsize)
		return BCME_UNSUPPORTED;

	return 0;
}

static int
wlc_dump_phy_noise(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_NOISE_ID);
	return 0;
}

static int
wlc_dump_phy_state(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_STATE_ID);
	return 0;
}

static int
wlc_dump_phy_measlo(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	if (!WLCISGPHY(wlc->band))
		return BCME_UNSUPPORTED;

	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_MEASLO_ID);
	return 0;
}

static int
wlc_dump_phy_lnagain(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	if (!wlc->clk)
		return BCME_NOCLK;

	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_LNAGAIN_ID);
	return 0;
}

static int
wlc_dump_phy_initgain(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	if (!wlc->clk)
		return BCME_NOCLK;

	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_INITGAIN_ID);
	return 0;
}

static int
wlc_dump_phy_hpf1tbl(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	if (!(WLCISNPHY(wlc->band) || WLCISHTPHY(wlc->band) || WLCISACPHY(wlc->band)))
		return BCME_UNSUPPORTED;

	if (!wlc->clk)
		return BCME_NOCLK;

	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_HPF1TBL_ID);
	return 0;
}

static int
wlc_dump_phy_lpphytbl0(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	if (!WLCISLPPHY(wlc->band))
		return BCME_UNSUPPORTED;

	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_LPPHYTBL0_ID);
	return 0;
}

static int
wlc_dump_phy_chanest(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	if (!wlc->clk)
		return BCME_NOCLK;

	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_CHANEST_ID);
	return 0;
}

static int
wlc_dump_suspend(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_SUSPEND_ID);
	return 0;
}

#ifdef ENABLE_FCBS
static int
wlc_dump_phy_fcbs(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_FCBS_ID);
	return 0;
}
#endif /* ENABLE_FCBS */

static int
wlc_dump_phy_txv0(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	wlc_bmac_dump(wlc->hw, b, BMAC_DUMP_PHY_TXV0_ID);
	return 0;
}
#endif 



#if IOV_DUMP_ANY_IOVAR_ENAB
static int
wlc_dump_info_doiovar(void *context, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int vsize, struct wlc_if *wlcif);
#endif /* IOV_DUMP_ANY_IOVAR_ENAB */

void
BCMATTACHFN(wlc_dump_info_detach)(wlc_info_t *wlc)
{
#if IOV_DUMP_ANY_IOVAR_ENAB
	wlc_module_unregister(wlc->pub, "dump_info", wlc);
#endif /* IOV_DUMP_ANY_IOVAR_ENAB */
}


#if defined(BCMDBG_PHYDUMP) || defined(TDLS_TESTBED) || defined(BCMDBG_AMPDU) || \
	defined(BCMDBG_DUMP_RSSI) || defined(MCHAN_MINIDUMP)
int
wlc_iovar_dump(wlc_info_t *wlc, const char *params, int p_len, char *out_buf, int out_len)
{
	struct bcmstrbuf b;
	char *name_list;
	int name_list_len;
	char *name1;
	char *name;
	const char *p;
	const char *endp;
	int err = 0;
	char *name_list_ptr;

	bcm_binit(&b, out_buf, out_len);
	p = params;
	endp = p + p_len;

	/* find the dump name list length to make a copy */
	while (p != endp && *p != '\0')
		p++;

	/* return an err if the name list was not null terminated */
	if (p == endp)
		return BCME_BADARG;

	/* copy the dump name list to a new buffer since the output buffer
	 * may be the same memory as the dump name list
	 */
	name_list_len = (int) ((const uint8*)p - (const uint8*)params + 1);
	name_list = (char*)MALLOC(wlc->osh, name_list_len);
	if (!name_list)
	      return BCME_NOMEM;
	bcopy(params, name_list, name_list_len);

	name_list_ptr = name_list;

	/* get the first two dump names */
	name1 = wlc_dump_next_name(&name_list_ptr);
	name = wlc_dump_next_name(&name_list_ptr);

	/* if the dump list was empty, return the default dump */
	if (name1 == NULL) {
		WL_ERROR(("doing default dump\n"));
		goto exit;
	}

	/* dump the first section
	 * only print the separator if there are more than one dump
	 */
	if (name != NULL)
		bcm_bprintf(&b, "\n%s:------\n", name1);
	err = wlc_dump_registered_name(wlc, name1, &b);
	if (err)
		goto exit;

	/* dump the rest */
	while (name != NULL) {
		bcm_bprintf(&b, "\n%s:------\n", name);
		err = wlc_dump_registered_name(wlc, name, &b);
		if (err)
			break;

		name = wlc_dump_next_name(&name_list_ptr);
	}

exit:
	MFREE(wlc->osh, name_list, name_list_len);

	/* make sure the output is at least a null terminated empty string */
	if (b.origbuf == b.buf && b.size > 0)
		b.buf[0] = '\0';

	return err;
}

static char*
wlc_dump_next_name(char **buf)
{
	char *p;
	char *name;

	p = *buf;

	if (p == NULL)
		return NULL;

	/* skip leading space */
	while (bcm_isspace(*p) && *p != '\0')
		p++;

	/* remember the name start position */
	if (*p != '\0')
		name = p;
	else
		name = NULL;

	/* find the end of the name
	 * name is terminated by space or null
	 */
	while (!bcm_isspace(*p) && *p != '\0')
		p++;

	/* replace the delimiter (or '\0' character) with '\0'
	 * and set the buffer pointer to the character past the delimiter
	 * (or to NULL if the end of the string was reached)
	 */
	if (*p != '\0') {
		*p++ = '\0';
		*buf = p;
	} else {
		*buf = NULL;
	}

	/* return the pointer to the name */
	return name;
}

/* Dump all matching the given name */
static int
wlc_dump_registered_name(wlc_info_t *wlc, char *name, struct bcmstrbuf *b)
{
	dumpcb_t *dumpcb;
	int err = 0;
	int rv = BCME_UNSUPPORTED; /* If nothing found, return this. */

	/* find the given dump name */
	for (dumpcb = wlc->dumpcb_head; dumpcb != NULL; dumpcb = dumpcb->next) {
		if (!strcmp(name, dumpcb->name)) {
			if (rv == BCME_UNSUPPORTED) { /* Found one */
				rv = BCME_OK;
			}
			err = dumpcb->dump_fn(dumpcb->dump_fn_arg, b);
			if (b->size == 0) { /* check for output buffer overflow */
				rv = BCME_BUFTOOSHORT;
				break;
			}
			if (err != 0) { /* Record last non successful error code */
				rv = err;
			}
		}
	}

	return rv;
}
#endif 

#if defined(WLTINYDUMP)
static int
wlc_tinydump(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	char perm[32], cur[32];
	char ssidbuf[SSID_FMT_BUF_LEN];
	int i;
	wlc_bsscfg_t *bsscfg;

	wl_dump_ver(wlc->wl, b);

	bcm_bprintf(b, "\n");

	bcm_bprintf(b, "resets %d\n", WLCNTVAL(wlc->pub->_cnt->reset));

	bcm_bprintf(b, "perm_etheraddr %s cur_etheraddr %s\n",
		bcm_ether_ntoa(&wlc->perm_etheraddr, perm),
		bcm_ether_ntoa(&wlc->pub->cur_etheraddr, cur));

	bcm_bprintf(b, "board 0x%x, board rev %s", wlc->pub->sih->boardtype,
	            bcm_brev_str(wlc->pub->boardrev, cur));
	if (wlc->pub->boardrev == BOARDREV_PROMOTED)
		bcm_bprintf(b, " (actually 0x%02x)", BOARDREV_PROMOTABLE);
	bcm_bprintf(b, "\n");

	bcm_bprintf(b, "rate_override: A %d, B %d\n",
		wlc->bandstate[BAND_5G_INDEX]->rspec_override,
		wlc->bandstate[BAND_2G_INDEX]->rspec_override);

	bcm_bprintf(b, "ant_rx_ovr %d txant %d\n", wlc->stf->ant_rx_ovr, wlc->stf->txant);

	FOREACH_BSS(wlc, i, bsscfg) {
		char ifname[32];

		bcm_bprintf(b, "\n");

		wlc_format_ssid(ssidbuf, bsscfg->SSID, bsscfg->SSID_len);
		strncpy(ifname, wl_ifname(wlc->wl, bsscfg->wlcif->wlif), sizeof(ifname));
		ifname[sizeof(ifname) - 1] = '\0';
		bcm_bprintf(b, "BSS Config %d: \"%s\"\n", i, ssidbuf);

		bcm_bprintf(b, "enable %d up %d wlif 0x%p \"%s\"\n",
		            bsscfg->enable,
		            bsscfg->up, bsscfg->wlcif->wlif, ifname);
		bcm_bprintf(b, "wsec 0x%x auth %d wsec_index %d wep_algo %d\n",
		            bsscfg->wsec,
		            bsscfg->auth, bsscfg->wsec_index,
		            WSEC_BSS_DEFAULT_KEY(bsscfg) ?
		            WSEC_BSS_DEFAULT_KEY(bsscfg)->algo : 0);

		bcm_bprintf(b, "current_bss->BSSID %s\n",
		            bcm_ether_ntoa(&bsscfg->current_bss->BSSID, (char*)perm));

		wlc_format_ssid(ssidbuf, bsscfg->current_bss->SSID,
		                bsscfg->current_bss->SSID_len);
		bcm_bprintf(b, "current_bss->SSID \"%s\"\n", ssidbuf);

#ifdef STA
		/* STA ONLY */
		if (!BSSCFG_STA(bsscfg))
			continue;

		bcm_bprintf(b, "bsscfg %d assoc_state %d\n", WLC_BSSCFG_IDX(bsscfg),
		            bsscfg->assoc->state);
#endif /* STA */
	}
	bcm_bprintf(b, "\n");

#ifdef STA
	bcm_bprintf(b, "AS_IN_PROGRESS() %d stas_associated %d\n", AS_IN_PROGRESS(wlc),
	            wlc->stas_associated);
#endif /* STA */

	bcm_bprintf(b, "aps_associated %d\n", wlc->aps_associated);
	FOREACH_UP_AP(wlc, i, bsscfg)
	        bcm_bprintf(b, "BSSID %s\n", bcm_ether_ntoa(&bsscfg->BSSID, (char*)perm));

	return 0;
}
#endif /* WLTINYDUMP */




#if defined(WLC_LOW) && defined(WLC_HIGH) && defined(MCHAN_MINIDUMP)
static int
wlc_dump_chanswitch(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	int i, j, nsamps, contexts;
	uint band;
	chanswitch_times_t *history = NULL;
	uint32 diff, total;
	uint us, ms, ts;
	char chanbuf[CHANSPEC_STR_LEN];
	char chanbuf1[CHANSPEC_STR_LEN];
	const char *chanswitch_history_names[] = {
		"wlc_set_chanspec",
		"wlc_adopt_context",
		"wlc_preppm_context"
	};

	if (ARRAYSIZE(chanswitch_history_names) != CHANSWITCH_LAST) {
		WL_ERROR(("%s: num_labels needs to match num of events!\n", __FUNCTION__));
		return -1;
	}

	for (contexts = 0; contexts < CHANSWITCH_LAST; contexts++) {
		bcm_bprintf(b, "**** %s  **** \n", chanswitch_history_names[contexts]);

		for (band = 0; band < NBANDS(wlc); band++) {
			if (band == 0) {
				history = wlc->chansw_hist[contexts];
				bcm_bprintf(b, "Channelswitch:      Duration"
					"          timestamp\n");
			} else {
				history = wlc->bandsw_hist[contexts];
				bcm_bprintf(b, "Bandswitch:         Duration"
					"          timestamp\n");
			}
			ASSERT(history);
			j = history->index % CHANSWITCH_TIMES_HISTORY;
			total = 0;
			nsamps = 0;
			for  (i = 0; i < CHANSWITCH_TIMES_HISTORY; i++) {
				diff = history->entry[j].end - history->entry[j].start;
				if (diff) {
					us = (diff % TSF_TICKS_PER_MS) * 1000 / TSF_TICKS_PER_MS;
					ms = diff / TSF_TICKS_PER_MS;
					total += diff;
					nsamps++;

					ts = history->entry[j].start / TSF_TICKS_PER_MS;

					bcm_bprintf(b, "%-6s => %-6s"
					"      %2.2d.%03u             %03u\n",
					wf_chspec_ntoa(history->entry[j].from, chanbuf),
					wf_chspec_ntoa(history->entry[j].to, chanbuf1),
					ms, us, ts);
				}
				j = (j + 1) % CHANSWITCH_TIMES_HISTORY;
			}
			if (nsamps) {
				total /= nsamps;
				us = (total % TSF_TICKS_PER_MS) * 1000 / TSF_TICKS_PER_MS;
				ms = total / TSF_TICKS_PER_MS;
				bcm_bprintf(b, "%s: %s: Avg %d.%03u Millisecs, %d Samples\n\n",
					chanswitch_history_names[contexts],
					band ? "Bandswitch" : "Channelswitch",
					ms, us, nsamps);
			} else {
				bcm_bprintf(b, "  -                   -                   -\n");
			}
		}
	}
	return 0;
}
#endif 








static void
wlc_dump_register_phy(wlc_info_t *wlc)
{
#if defined(BCMDBG_PHYDUMP)
#if defined(BCMDBG_PHYDUMP)
	wlc_dump_register(wlc->pub, "phyreg", (dump_fn_t)wlc_dump_phy_phyreg, (void *)wlc);
	/* "wl dump phytbl" and "wl dump phytbl2" requires more than 1024 bytes, */
	/* which is the current limitation for BMAC dump through RPC call. */
	/* Don't support them for now. */
	wlc_dump_register(wlc->pub, "radioreg",	(dump_fn_t)wlc_dump_phy_radioreg, (void *)wlc);
#endif 


	wlc_dump_register(wlc->pub, "phycal", (dump_fn_t)wlc_dump_phy_cal, (void *)wlc);
	wlc_dump_register(wlc->pub, "phyaci", (dump_fn_t)wlc_dump_phy_aci, (void *)wlc);
	wlc_dump_register(wlc->pub, "phypapd",	(dump_fn_t)wlc_dump_phy_papd, (void *)wlc);
	wlc_dump_register(wlc->pub, "phynoise", (dump_fn_t)wlc_dump_phy_noise, (void *)wlc);
	wlc_dump_register(wlc->pub, "phystate",	(dump_fn_t)wlc_dump_phy_state, (void *)wlc);
	wlc_dump_register(wlc->pub, "phylo", (dump_fn_t)wlc_dump_phy_measlo, (void *)wlc);
	wlc_dump_register(wlc->pub, "phylnagain", (dump_fn_t)wlc_dump_phy_lnagain, (void *)wlc);
	wlc_dump_register(wlc->pub, "phyinitgain", (dump_fn_t)wlc_dump_phy_initgain, (void *)wlc);
	wlc_dump_register(wlc->pub, "phyhpf1tbl", (dump_fn_t)wlc_dump_phy_hpf1tbl, (void *)wlc);
	wlc_dump_register(wlc->pub, "phylpphytbl0", (dump_fn_t)wlc_dump_phy_lpphytbl0, (void *)wlc);
	wlc_dump_register(wlc->pub, "phychanest", (dump_fn_t)wlc_dump_phy_chanest, (void *)wlc);
	wlc_dump_register(wlc->pub, "macsuspend", (dump_fn_t)wlc_dump_suspend, (void *)wlc);
#ifdef ENABLE_FCBS
	wlc_dump_register(wlc->pub, "phyfcbs", (dump_fn_t)wlc_dump_phy_fcbs, (void *)wlc);
#endif /* ENABLE_FCBS */
	wlc_dump_register(wlc->pub, "phytxv0", (dump_fn_t)wlc_dump_phy_txv0, (void *)wlc);
#endif 
}

#if IOV_DUMP_ANY_IOVAR_ENAB
enum {
	IOV_DUMP = 0,
	IOV_NVRAM_DUMP = 1,
	IOV_SMF_STATS = 2,
	IOV_SMF_STATS_ENABLE = 3,
	IOV_DUMP_LAST
};

static const bcm_iovar_t dump_info_iovars[] = {
#if IOV_DUMP_DEF_IOVAR_ENAB
	{"dump", IOV_DUMP,
	(IOVF_OPEN_ALLOW), IOVT_BUFFER, 0
	},
#endif /* IOV_DUMP_DEF_IOVAR_ENAB */
#if IOV_DUMP_NVM_IOVAR_ENAB
	{"nvram_dump", IOV_NVRAM_DUMP,
	(IOVF_MFG), IOVT_BUFFER, 0
	},
#endif /* IOV_DUMP_NVM_IOVAR_ENAB */
	{NULL, 0, 0, 0, 0}
};

static int
wlc_dump_info_doiovar(void *context, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int vsize, struct wlc_if *wlcif)
{
	int err = BCME_OK;
	struct bcmstrbuf b;
	wlc_info_t *wlc = (wlc_info_t *)context;
	wlc_bsscfg_t *bsscfg;
	int32 int_val = 0;
	int32 *ret_int_ptr;

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	/* convenience int and bool vals for first 8 bytes of buffer */
	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	/* update bsscfg w/provided interface context */
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);

	BCM_REFERENCE(b);
	BCM_REFERENCE(bsscfg);
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(int_val);
	BCM_REFERENCE(ret_int_ptr);

	switch (actionid) {
		case IOV_GVAL(IOV_DUMP):
#if defined(BCMDBG_PHYDUMP) || defined(BCMDBG_AMPDU) || defined(BCMDBG_DUMP_RSSI) || \
	defined(MCHAN_MINIDUMP)
			if (wl_msg_level & WL_LOG_VAL || WL_CHANLOG_ON())
				bcmdumplog((char*)arg, len);
			else
				err = wlc_iovar_dump(wlc, (const char *)params, p_len,
					(char*)arg, len);
#elif defined(WLTINYDUMP)
		#if defined(TDLS_TESTBED)
		if (bcmp(params, "tdls", sizeof("tdls") - 1)) {
		#endif /* TDLS_TESTBED */
			bcm_binit(&b, arg, len);
			err = wlc_tinydump(wlc, &b);
		#if defined(TDLS_TESTBED)
			}
		else
			err = wlc_iovar_dump(wlc, (const char *)params, p_len, (char*)arg, len);
		#endif /* TDLS_TESTBED */
#endif 
			break;
#if defined(WLPKTENG)
		case IOV_GVAL(IOV_NVRAM_DUMP):
			bcm_binit(&b, (char*)arg, len);
			err = wlc_nvram_dump(wlc, &b);
		break;
#endif 
		default:
			err = BCME_UNSUPPORTED;
		break;
	}
	return err;
}
#endif /* IOV_DUMP_ANY_IOVAR_ENAB */

int
BCMATTACHFN(wlc_dump_info_attach)(wlc_info_t *wlc)
{
	wlc_pub_t *pub = wlc->pub;
	int err = BCME_OK;
	BCM_REFERENCE(pub);
#if IOV_DUMP_ANY_IOVAR_ENAB
	if (wlc_module_register(pub, dump_info_iovars, "dump_info",
		wlc, wlc_dump_info_doiovar, NULL, NULL, NULL)) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
			wlc->pub->unit, __FUNCTION__));
		err = BCME_ERROR;
	}
#endif /* IOV_DUMP_ANY_IOVAR_ENAB */
#ifdef WLTINYDUMP
	wlc_dump_register(pub, "tiny", (dump_fn_t)wlc_tinydump, (void *)wlc);
#endif /* WLTINYDUMP */
	wlc_dump_register_phy(wlc);

/* Register dump handler for memory pool manager. */




#if defined(WLC_LOW) && defined(WLC_HIGH) && defined(MCHAN_MINIDUMP)
	wlc_dump_register(pub, "chanswitch", (dump_fn_t)wlc_dump_chanswitch, (void *)wlc);
#endif



	return err;
}
