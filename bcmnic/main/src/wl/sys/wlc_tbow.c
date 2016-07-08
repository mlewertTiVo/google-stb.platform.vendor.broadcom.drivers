/*
* Tunnel BT Traffic Over Wlan
*
* Copyright (C) 2016, Broadcom Corporation
* All Rights Reserved.
* 
* This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
* the contents of this file may not be disclosed to third parties, copied
* or duplicated in any form, in whole or in part, without the prior
* written permission of Broadcom Corporation.
*
* $Id: wlc_tbow.c 487741 2014-06-26 22:33:48Z $
*
*/
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <proto/802.1d.h>
#include <bcmwpa.h>
#include <d11.h>
#include <wlc_pub.h>
#include <wlc_key.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wl_export.h>
#include <wlc_mcnx.h>
#include <wlc_pcb.h>

#include <wlc_bwte.h>

#include <wlc_tbow.h>
#include <wlc_tbow_priv.h>

#ifdef GC_LOOP_SCO
static void tbow_send_bt_wlandata(tbow_info_t *tbow_info, tbow_pkt_type_t pkt_type, bool force);
#endif
static int tbow_process_iovar(tbow_info_t *tbow_info, uint32 actionid,
void* arg, int len, wlc_bsscfg_t *bsscfg);

enum {
	IOV_TBOW_DOHO,
	IOV_TBOW_LAST
};

/* IOVar table */
static const bcm_iovar_t wl_tbow_iovars[] = {
	{"tbow_doho", IOV_TBOW_DOHO,
	(0), IOVT_BOOL, 0
	},
	{NULL, 0, 0, 0, 0 }
};

static int
tbow_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int val_size, struct wlc_if *wlcif)
{
	int err = 0;
	tbow_info_t *tbow_info;
	wlc_bsscfg_t *bsscfg;

	tbow_info = (tbow_info_t *)hdl;
	ASSERT(tbow_info);

	/* update bsscfg w/provided interface context */
	bsscfg = wlc_bsscfg_find_by_wlcif(tbow_info->wlc, wlcif);
	ASSERT(bsscfg);

	if ((err = wlc_iovar_check(tbow_info->wlc, vi, arg, len, IOV_ISSET(actionid), wlcif)) != 0)
		return err;

	err = tbow_process_iovar(tbow_info, actionid, arg, len, bsscfg);
	return err;
}

static int
tbow_process_iovar(tbow_info_t *tbow_info, uint32 actionid,
void* arg, int len, wlc_bsscfg_t *bsscfg)
{
	int err = 0;
	wlc_info_t* wlc = tbow_info->wlc;

	switch (actionid) {
		case IOV_SVAL(IOV_TBOW_DOHO):
		{
			tbow_setup_netinfo_t *netinfo = (tbow_setup_netinfo_t *)arg;
			if (netinfo->opmode == TBOW_HO_MODE_TEST_GO) {
				tbow_send_goinfo(tbow_info);
				tbow_start_ho(tbow_info);
				break;
			} else if (netinfo->opmode == TBOW_HO_MODE_STOP_GO) {
				tbow_ho_stop(tbow_info);
				break;
			}

			if (netinfo->opmode == TBOW_HO_MODE_START_GC) {
				/* print for debug */
				int i;
				for (i = 0; i < WLC_MAXBSSCFG; i++)
					if (wlc->bsscfg[i])
						WL_TRACE(("bsscfg[%d]: %d, %d, %d, "
						    "%02x:%02x:%02x:%02x:%02x:%02x\n",
						    i, wlc->bsscfg[i]->up, wlc->bsscfg[i]->enable,
						    wlc->bsscfg[i]->_ap,
						    wlc->bsscfg[i]->cur_etheraddr.octet[0],
						    wlc->bsscfg[i]->cur_etheraddr.octet[1],
						    wlc->bsscfg[i]->cur_etheraddr.octet[2],
						    wlc->bsscfg[i]->cur_etheraddr.octet[3],
						    wlc->bsscfg[i]->cur_etheraddr.octet[4],
						    wlc->bsscfg[i]->cur_etheraddr.octet[5]));
			}
			WL_TRACE(("HO opmode: %d\n", netinfo->opmode));
			WL_TRACE(("HO channel: %d\n", netinfo->channel));
			netinfo->ssid[netinfo->ssid_len] = 0;
			WL_TRACE(("HO SSID: %s\n", netinfo->ssid));
			netinfo->passphrase[netinfo->passphrase_len] = 0;
			WL_TRACE(("HO PASS: %s\n", netinfo->passphrase));
			if (netinfo->opmode == TBOW_HO_MODE_START_GO) {
				WL_TRACE(("MAC for GO: %02x:%02x:%02x:%02x:%02x:%02x\n",
				    netinfo->macaddr[0], netinfo->macaddr[1],
				    netinfo->macaddr[2], netinfo->macaddr[3],
				    netinfo->macaddr[4], netinfo->macaddr[5]));
			} else if (netinfo->opmode == TBOW_HO_MODE_START_GC) {
				WL_TRACE(("BSSID to connect: %02x:%02x:%02x:%02x:%02x:%02x\n",
				    netinfo->macaddr[0], netinfo->macaddr[1],
				    netinfo->macaddr[2], netinfo->macaddr[3],
				    netinfo->macaddr[4], netinfo->macaddr[5]));
				tbow_info->ho_state = TBOW_HO_START;
			} else {
				tbow_info->ho_state = TBOW_HO_START;
			}

			wlc_bss_mac_event(wlc, bsscfg,
				WLC_E_BT_WIFI_HANDOVER_REQ, NULL,
				WLC_E_STATUS_SUCCESS, 0, 0, arg, len);

			break;
		}

		default:
			err = BCME_UNSUPPORTED;
			break;
	}

	return err;
}


static char* tbow_get_msgstr(uchar msg_type)
{
	char* s;
	switch (msg_type) {
		case TBOW_HO_MSG_BT_READY:
			s = "bt_ready";
			break;
		case TBOW_HO_MSG_GO_STA_SETUP:
			s = "go_setup";
			break;
		case TBOW_HO_MSG_SETUP_ACK:
			s = "gc_ack";
			break;
		case TBOW_HO_MSG_START:
			s = "ho_start";
			break;
		case TBOW_HO_MSG_WLAN_READY:
			s = "wlan_ready";
			break;
		case TBOW_HO_MSG_S1:
			s = "ho_s1";
			break;
		case TBOW_HO_MSG_S2:
			s = "ho_s2";
			break;
		case TBOW_HO_MSG_S3:
			s = "ho_s3";
			break;
		case TBOW_HO_MSG_STOP:
			s = "ho_stop";
			break;
		case TBOW_HO_MSG_BT_DROP:
			s = "bt_drop";
			break;
		case TBOW_HO_MSG_BT_DROP_ACK:
			s = "bt_drop_ack";
			break;
		case TBOW_HO_MSG_WLAN_DROP:
			s = "wlan_drop";
			break;
		case TBOW_HO_MSG_WLAN_DROP_ACK:
			s = "wlan_drop_ack";
			break;
		case TBOW_HO_MSG_INIT_START:
			s = "initiate_ho_start";
			break;
		case TBOW_HO_MSG_INIT_STOP:
			s = "initiateho_stop";
			break;
		case TBOW_HO_MSG_BT_SUSPEND_FAILED:
			s = "bt_suspend_failed";
			break;
		case TBOW_HO_MSG_BT_RESUME_FAILED:
			s = "bt_resume_failed";
			break;
		case TBOW_HO_MSG_WLAN_RSSI_QUERY:
			s = "wlan_rssi_query";
			break;
		case TBOW_HO_MSG_WLAN_RSSI_RESP:
			s = "wlan_rssi_resp";
			break;
		case TBOW_HO_MSG_BT_RESET:
			s = "bt_reset";
			break;
		case TBOW_HO_MSG_BT_RESET_ACK:
			s = "bt_reset_ack";
			break;
		default:
			s = "no_name";
			break;
	}

	return s;
}

static void tbow_cleanup_data(tbow_info_t *tbow_info, tbow_pkt_type_t pkt_type)
{
	tbow_datachan_state_t *chan_state;
	wlc_bwte_payload_t payload = WLC_BWTE_PAYLOAD_CNT;

	if (!tbow_info) {
		WL_ERROR(("%s: NULL tbow_info pointer\n", __FUNCTION__));
		return;
	}

	if (pkt_type >= TBOW_TYPE_DATA_CNT)
	{
		WL_ERROR(("%s: invalid pkt_type(%d)\n", __FUNCTION__, pkt_type));
		return;
	}

	/* bt2waln chan */
	if (pkt_type ==  TBOW_TYPE_DATA_ACL) {
		payload = WLC_BWTE_LO_DATA;
	} else if (pkt_type ==  TBOW_TYPE_DATA_SCO) {
		payload = WLC_BWTE_HI_DATA;
	}

	wlc_bwte_reclaim_wlan_buf(tbow_info->wlc->bwte_info, WLC_BWTE_CLIENT_TBOW, payload, TRUE);

	/* per link chan state */
	chan_state = &tbow_info->datachan_state[pkt_type];
	chan_state->tx_seq_num = 0;
	chan_state->exp_rx_seq_num = 0;
	chan_state->pend_eos_seq = 0;
	chan_state->wait_for_pkt = FALSE;
}

static void tbow_cleanup(tbow_info_t *tbow_info)
{
	void *p;

	if (!tbow_info) {
		WL_ERROR(("%s: NULL tbow_info pointer\n", __FUNCTION__));
		return;
	}

	tbow_info->bsscfg_idx = -1;

	while ((p = pktq_mdeq(&tbow_info->send_pq,
		(1<< TBOW_TYPE_DATA_ACL | 1 << TBOW_TYPE_DATA_SCO | 1 << TBOW_TYPE_CTL), NULL))) {
		PKTFREE(tbow_info->wlc->osh, p, TRUE);
	}

	while ((p = pktq_mdeq(&tbow_info->recv_pq,
		(1<< TBOW_TYPE_DATA_ACL | 1 << TBOW_TYPE_DATA_SCO), NULL))) {
		PKTFREE(tbow_info->wlc->osh, p, FALSE);
	}

	tbow_info->ist_timer_set = FALSE;
	tbow_info->intransit_pkt_cnt = 0;
	tbow_info->tx_wlan_ho_stop = 0;
	tbow_info->rx_wlan_ho_stop = 0;
	tbow_info->pend_eos_chan = 0;
	tbow_info->pull_bt_stop = FALSE;
	tbow_info->tunnel_stop = FALSE;
	tbow_info->push_bt_stop = FALSE;
	tbow_info->linkdown_timer_set = FALSE;

	tbow_cleanup_data(tbow_info, TBOW_TYPE_DATA_ACL);
	tbow_cleanup_data(tbow_info, TBOW_TYPE_DATA_SCO);
}

static wlc_bsscfg_t *tbow_is_link_ready(tbow_info_t *tbow_info)
{
	wlc_bsscfg_t *bsscfg;

	if (!tbow_info || !tbow_info->wlc) {
		WL_ERROR(("%s: NULL pointer, tbow_info(%p), wlc(%p)\n", __FUNCTION__, tbow_info,
			tbow_info ? tbow_info->wlc : NULL));
		return NULL;
	}

	if (tbow_info->bsscfg_idx == -1) {
		int i;
		wlc_bsscfg_t *bsscfg;

		FOREACH_BSS(tbow_info->wlc, i, bsscfg) {
			if (bsscfg->associated &&
				!bcmp(&bsscfg->cur_etheraddr, &tbow_info->local_addr,
				ETHER_ADDR_LEN)) {
				tbow_info->bsscfg_idx = i;
				WL_TRACE(("Found bsscfg(%p), wlcif(%p)\n", bsscfg, bsscfg->wlcif));
				break;
			}
		}

		if (tbow_info->bsscfg_idx == -1) {
			WL_ERROR(("no bsscfg found\n"));
			return NULL;
		}
	}

	bsscfg = tbow_info->wlc->bsscfg[tbow_info->bsscfg_idx];

	if (bsscfg && bsscfg->associated) {
		if (tbow_info->linkdown_timer && tbow_info->linkdown_timer_set) {
			wl_del_timer(tbow_info->wlc->wl, tbow_info->linkdown_timer);
			tbow_info->linkdown_timer_set = FALSE;
		}
		return bsscfg;
	} else {
		if (tbow_info->linkdown_timer && !tbow_info->linkdown_timer_set) {
			wl_add_timer(tbow_info->wlc->wl, tbow_info->linkdown_timer,
				TBOW_LINKDOWN_TIMEOUT, FALSE);
			tbow_info->linkdown_timer_set = TRUE;
		}

		return NULL;
	}
}

static void tbow_linkdown(void *arg)
{
	tbow_info_t *tbow_info = (tbow_info_t *)arg;
	uchar *msg_buf;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return;
	}

	msg_buf = MALLOC(tbow_info->wlc->osh, 1);
	if (!msg_buf) {
		WL_ERROR(("%s: alloc msg memory failed\n", __FUNCTION__));
		tbow_info->linkdown_timer_set = FALSE;
		return;
	}

	tbow_info->pull_bt_stop = TRUE;
	tbow_info->push_bt_stop = TRUE;
	tbow_info->tunnel_stop = TRUE;
	tbow_ho_stop(tbow_info);

	*msg_buf = TBOW_HO_MSG_WLAN_DROP;
	tbow_send_bt_msg(tbow_info, msg_buf, 1);
}


static int tbow_queue_btdata(tbow_info_t *tbow_info, bool new_pkt, uchar *s, uint16 len,
	void* ori_pkt, tbow_pkt_type_t pkt_type, uint8 flag, uint32 timeout_val,
	struct wl_timer *timer)
{
	void  *pkt;
	uint8 *frame;
	uint16 pktlen;
	struct ether_header *eth_hdr = NULL;
	tbow_header_t *tbow_header = NULL;
	tbow_tail_t *tbow_tail = NULL;
	bool new_ts = TRUE;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return -1;
	}

	if (pkt_type >= TBOW_TYPE_CNT) {
		WL_ERROR(("%s: invalid pkt_type(%d)\n", __FUNCTION__, pkt_type));
		return -1;
	}

	if (!tbow_is_link_ready(tbow_info)) {
		WL_ERROR(("%s: wlan link not ready\n", __FUNCTION__));
		return -1;
	}

#ifndef GO_LOOP_SCO
	if (new_pkt && (tbow_info->intransit_pkt_cnt >= MAX_INTRASNIT_COUNT)) {
		return -1;
	}
#endif

	if (s) {
#ifdef GC_LOOP_USE_RX_LBUF
		pktlen = TBOW_ETH_HDR_LEN + sizeof(tbow_header_t) + len;
#else
		pktlen = TBOW_ETH_HDR_LEN + sizeof(tbow_header_t) + len + sizeof(tbow_tail_t);
#endif
		/* prepare lbuf */
		if (!(pkt = PKTGET(tbow_info->wlc->osh, pktlen, TRUE))) {
			WL_ERROR(("%s: alloc pkt failed!!\n", __FUNCTION__));
			return -1;
		}

#ifndef GC_LOOP_USE_RX_LBUF
		PKTSETLEN(tbow_info->wlc->osh, pkt, pktlen - sizeof(tbow_tail_t));
#endif
	} else if (ori_pkt) {
		pkt = ori_pkt;
		pktlen = PKTLEN(tbow_info->wlc->osh, pkt);
	} else {
		WL_ERROR(("both s and ori_pkt is NULL\n"));
		return -1;
	}

	/* fill ethernet header */
	frame = PKTDATA(tbow_info->wlc->osh, pkt);
	eth_hdr = (struct ether_header *)frame;

	bcopy(tbow_info->remote_addr.octet, eth_hdr->ether_dhost, sizeof(struct ether_addr));
	bcopy(tbow_info->local_addr.octet, eth_hdr->ether_shost, sizeof(struct ether_addr));
	eth_hdr->ether_type = hton16(ETHER_TYPE_TBOW);
	frame += TBOW_ETH_HDR_LEN;

	tbow_header = (tbow_header_t *)frame;
#ifdef GC_LOOP_USE_RX_LBUF
	tbow_tail = &tbow_header->tail;
#else
	tbow_tail = (tbow_tail_t *)(PKTDATA(tbow_info->wlc->osh, pkt) +
		PKTLEN(tbow_info->wlc->osh, pkt));
#endif
	/* fill data payload */
	if (new_pkt) {
		/* fill seq num */
		tbow_header->pkt_type = pkt_type;
		if (pkt_type == TBOW_TYPE_DATA_ACL) {
			tbow_header->seq_num =
				tbow_info->datachan_state[TBOW_TYPE_DATA_ACL].tx_seq_num++;
		} else if (pkt_type == TBOW_TYPE_DATA_SCO) {
			tbow_header->seq_num =
				tbow_info->datachan_state[TBOW_TYPE_DATA_SCO].tx_seq_num++;
		} else {
			tbow_header->seq_num = 0;
		}

#if defined(GC_LOOP_SCO) && defined(GC_LOOP_USE_RX_LBUF)
		if (!tbow_info->isGO) {
			new_ts = FALSE;
		}
#endif
		if (new_ts) {
			tbow_header->tx_ts = R_REG(tbow_info->wlc->osh,
				&tbow_info->wlc->regs->tsf_timerlow);
		}

		frame += sizeof(tbow_header_t);
		if (s && len) {
			/* copy bt data */
			bcopy(s, frame, len);
		}

		tbow_tail->flag = flag;
		tbow_tail->tx_cnt = 1;
		tbow_tail->timeout_val = timeout_val;
		tbow_tail->timer = timer;
	} else {
		tbow_tail->tx_cnt++;
		frame += sizeof(tbow_header_t);
	}

	/* queue pkt */
	if (new_pkt) {
		/* tail of Q */
		pktq_penq(&tbow_info->send_pq, pkt_type, pkt);

		if (flag & TBOW_TAIL_FLAG_MAC_ACK) {
			tbow_info->intransit_pkt_cnt++;
		}
	} else {
		/* head of Q */
		pktq_penq_head(&tbow_info->send_pq, pkt_type, pkt);
	}

	return 0;
}

static void
tbow_tunnel_btdata_cb(wlc_info_t *wlc, uint txstatus, void *arg)
{
	tbow_info_t *tbow_info;
	tbow_header_t *tbow_header;
	tbow_tail_t *tbow_tail;

	if (!wlc || !wlc->tbow_info || !arg) {
		WL_ERROR(("%s: NULL input, wlc(%p), tbow_info(%p), arg(%p)\n",
			__FUNCTION__, wlc, wlc ? wlc->tbow_info : NULL, arg));
		return;
	}

	tbow_info = (tbow_info_t *)wlc->tbow_info;
	tbow_header = (tbow_header_t *)(PKTDATA(tbow_info->wlc->osh, arg) +  TBOW_ETH_HDR_LEN);
#ifdef GC_LOOP_USE_RX_LBUF
	tbow_tail = &tbow_header->tail;
#else
	tbow_tail = (tbow_tail_t *)(PKTDATA(tbow_info->wlc->osh, arg) +
		PKTLEN(tbow_info->wlc->osh, arg));
#endif

	if (txstatus & TX_STATUS_ACK_RCV) {
		if (tbow_header->pkt_type == TBOW_TYPE_CTL) {
			uint8 msg_type = *(uint8 *)(tbow_header + 1);

			if ((msg_type == TBOW_HO_MSG_S3) ||
			(msg_type == TBOW_HO_MSG_BT_DROP) ||
			(msg_type == TBOW_HO_MSG_BT_RESET)) {
				/* call stop function to tear down link */
				WL_TRACE(("%s: ready to tear down link\n", __FUNCTION__));
				/* flush possible bt data in queue */
				tbow_info->tunnel_stop = TRUE;
				tbow_ho_stop(tbow_info);
			}

			if ((msg_type == TBOW_HO_MSG_BT_DROP) ||
			(msg_type == TBOW_HO_MSG_BT_RESET)) {
				uchar *msg_buf;
				/* ack bt drop msg */
				msg_buf = MALLOC(tbow_info->wlc->osh, 1);
				if (!msg_buf) {
					WL_ERROR(("%s: alloc msg memory failed\n", __FUNCTION__));
					goto free;
				}

				if (msg_type == TBOW_HO_MSG_BT_DROP) {
					*msg_buf = TBOW_HO_MSG_BT_DROP_ACK;
				} else if (msg_type == TBOW_HO_MSG_BT_RESET) {
					*msg_buf = TBOW_HO_MSG_BT_RESET_ACK;
				} else {
					WL_ERROR(("%s: unknown msg %d\n", __FUNCTION__, msg_type));
					goto free;
				}

				tbow_send_bt_msg(tbow_info, msg_buf, 1);
			}
		}
free:
		PKTFREE(tbow_info->wlc->osh, arg, TRUE);
		tbow_info->intransit_pkt_cnt--;
	} else {
		int ret;

		ret = tbow_queue_btdata(tbow_info, FALSE, NULL, 0, arg, tbow_header->pkt_type,
			TBOW_TAIL_FLAG_MAC_ACK, 0, NULL);
		if (ret) {
			WL_ERROR(("%s: requeue pkt failed\n", __FUNCTION__));
		}
	}

	/* Enqueue to sendQ may be blocked by MAX_INTRASNIT_COUNT, trigger ISR */
	wlc_bwte_process_bt_intr(tbow_info->wlc->bwte_info, WLC_BWTE_CLIENT_TBOW);

#ifdef GC_LOOP_SCO
	if (!tbow_info->isGO && pktq_plen(&tbow_info->recv_pq, TBOW_TYPE_DATA_SCO)) {
		tbow_send_bt_wlandata(tbow_info, TBOW_TYPE_DATA_SCO, FALSE);
	}
#endif

	if (pktq_len(&tbow_info->send_pq) && !tbow_info->ist_timer_set) {
		tbow_info->ist_timer_set = TRUE;
		wl_add_timer(tbow_info->wlc->wl, tbow_info->ist_timer,
			(txstatus & TX_STATUS_ACK_RCV) ? 2 : (2 * tbow_tail->tx_cnt), FALSE);
	}
}

static void tbow_tunnel_btdata(tbow_info_t *tbow_info)
{
	void *pkt, *last_pkt, *clone_pkt;
	int ret, prec_out;
	tbow_header_t *tbow_header;
	tbow_tail_t *tbow_tail;
	wlc_bsscfg_t *bsscfg;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return;
	}

	if (!(bsscfg = tbow_is_link_ready(tbow_info))) {
		WL_ERROR(("%s: wlan link not ready\n", __FUNCTION__));
		return;
	}

	BCM_REFERENCE(tbow_header);
	last_pkt = NULL;

	while ((pkt = pktq_mdeq(&tbow_info->send_pq,
		(1 << TBOW_TYPE_DATA_ACL | 1 << TBOW_TYPE_DATA_SCO | 1 << TBOW_TYPE_CTL),
		&prec_out))) {
		if (tbow_info->tunnel_stop) {
			PKTFREE(tbow_info->wlc->osh, pkt, TRUE);
			tbow_info->intransit_pkt_cnt--;
			continue;
		}
		tbow_header = (tbow_header_t *)(PKTDATA(tbow_info->wlc->osh, pkt) +
			TBOW_ETH_HDR_LEN);
#ifdef GC_LOOP_USE_RX_LBUF
		tbow_tail = &tbow_header->tail;
#else
		tbow_tail = (tbow_tail_t *)(PKTDATA(tbow_info->wlc->osh, pkt) +
			PKTLEN(tbow_info->wlc->osh, pkt));
#endif
		if (last_pkt == pkt) {
			WL_TRACE(("sa-"));
			pktq_penq_head(&tbow_info->send_pq, prec_out, pkt);
			break;
		}

		last_pkt = pkt;

#ifdef GO_LOOP_SCO
		if (tbow_info->isGO && (prec_out == TBOW_TYPE_DATA_SCO)) {
			tbow_recv_wlandata(tbow_info, pkt);
			continue;
		}
#endif

		if (tbow_tail->flag & TBOW_TAIL_FLAG_MAC_ACK) {
			clone_pkt = osl_pktclone(tbow_info->wlc->osh, pkt, 0,
				PKTLEN(tbow_info->wlc->osh, pkt));
			if (!clone_pkt) {
				WL_ERROR(("%s: clone failed\n", __FUNCTION__));
				pktq_penq_head(&tbow_info->send_pq, prec_out, pkt);
				break;
			}
			ret = wlc_pcb_fn_register(tbow_info->wlc->pcb, tbow_tunnel_btdata_cb,
				pkt, clone_pkt);
			if (ret) {
				WL_ERROR(("%s: register cb failed\n", __FUNCTION__));
				pktq_penq_head(&tbow_info->send_pq, prec_out, pkt);
				break;
			}
		} else {
			clone_pkt = pkt;
		}

		if (tbow_tail->flag & TBOW_TAIL_FLAG_STACK_ACK) {
			wl_add_timer(tbow_info->wlc->wl, tbow_tail->timer, tbow_tail->timeout_val,
				FALSE);
		}

		if (prec_out == TBOW_TYPE_DATA_SCO) {
			PKTSETPRIO(clone_pkt, PRIO_8021D_VO);
		} else {
			PKTSETPRIO(clone_pkt, PRIO_8021D_BE);
		}

		wlc_sendpkt(tbow_info->wlc, clone_pkt, bsscfg->wlcif);
	}

	return;
}

static void tbow_send_bt_ho_stop(tbow_info_t *tbow_info, uint8 stop_msg)
{
	uchar *msg_buf;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return;
	}

	if ((stop_msg != TBOW_HO_MSG_STOP) && (stop_msg != TBOW_HO_MSG_S1) &&
		(stop_msg != TBOW_HO_MSG_S2) && (stop_msg != TBOW_HO_MSG_S3)) {
		WL_ERROR(("%s: stop_msg(%d) is invalid\n", __FUNCTION__, stop_msg));
		return;
	}

	msg_buf = MALLOC(tbow_info->wlc->osh, 1);
	if (!msg_buf) {
		WL_ERROR(("%s: alloc msg memory failed\n", __FUNCTION__));
		return;
	}
	*msg_buf = stop_msg;
	tbow_send_bt_msg(tbow_info, msg_buf, 1);
}

static int tbow_data_free(void* arg, uchar* p, int len)
{
	tbow_info_t *tbow_info = (tbow_info_t *)arg;
	uint32 sdu;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return -1;
	}

	if (p) {
		/* get lbuf addr and free it */
		sdu = *((uint32 *)((uint32)p - sizeof(tbow_header_t) - sizeof(void *)));
		PKTFREE(tbow_info->wlc->osh, (void *)sdu, FALSE);

		if (!tbow_info->ist_timer_set) {
			tbow_info->ist_timer_set = TRUE;
			wl_add_timer(tbow_info->wlc->wl, tbow_info->ist_timer, 0, FALSE);
		}
	}

	return 0;
}

static void tbow_send_bt_wlandata(tbow_info_t *tbow_info, tbow_pkt_type_t pkt_type, bool force)
{
	tbow_datachan_state_t *chan_state;
	struct wl_timer *chan_timer;
	void *p;
	wlc_bwte_payload_t payload = WLC_BWTE_PAYLOAD_CNT;
	bool data_sent = FALSE;
	int ret;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return;
	}

	if (pkt_type >= TBOW_TYPE_DATA_CNT)
	{
		WL_ERROR(("%s: invalid pkt_type(%d)\n", __FUNCTION__, pkt_type));
		return;
	}

	if (pkt_type == TBOW_TYPE_DATA_ACL) {
		chan_timer = tbow_info->data_acl_timer;
		payload = WLC_BWTE_LO_DATA;
	} else if (pkt_type ==  TBOW_TYPE_DATA_SCO) {
		chan_timer = tbow_info->data_sco_timer;
		payload = WLC_BWTE_HI_DATA;
		force  = TRUE;
	}

	wlc_bwte_reclaim_wlan_buf(tbow_info->wlc->bwte_info, WLC_BWTE_CLIENT_TBOW, payload, FALSE);

	chan_state = &tbow_info->datachan_state[pkt_type];

	while ((p = pktq_pdeq(&tbow_info->recv_pq, pkt_type))) {
		tbow_header_t *tbow_header = (tbow_header_t *)PKTDATA(tbow_info->wlc->osh, p);
		bool qBack = FALSE, qWait = FALSE;

		if (tbow_info->push_bt_stop) {
			chan_state->exp_rx_seq_num = (uint16)(tbow_header->seq_num + 1);
			PKTFREE(tbow_info->wlc->osh, p, TRUE);
			continue;
		}

		if ((tbow_header->seq_num == chan_state->exp_rx_seq_num) || force) {
			uint32 tsf_l, rtsf_l;
			tsf_l = R_REG(tbow_info->wlc->osh, &tbow_info->wlc->regs->tsf_timerlow);
#if defined(GO_LOOP_SCO) || defined(GC_LOOP_SCO)
			rtsf_l = tsf_l;
#else
			rtsf_l = wlc_mcnx_l2r_tsf32(tbow_info->wlc->mcnx,
				tbow_info->wlc->bsscfg[tbow_info->bsscfg_idx], tsf_l);
#endif

			BCM_REFERENCE(rtsf_l);
			if (chan_state->wait_for_pkt) {
				if (tbow_header->seq_num == chan_state->exp_rx_seq_num) {
					WL_TRACE(("pkt_type(%d) got exp_seq(%d)\n", pkt_type,
						chan_state->exp_rx_seq_num));
				} else {
					WL_TRACE(("pkt_type(%d) wait seq(%d) timeout\n", pkt_type,
						chan_state->exp_rx_seq_num));
				}
				chan_state->wait_for_pkt = FALSE;

#ifndef GC_LOOP_SCO
				wl_del_timer(tbow_info->wlc->wl, chan_timer);
#endif
			}
			PKTPULL(tbow_info->wlc->osh, p, sizeof(tbow_header_t));

#ifdef GC_LOOP_SCO
			if (!tbow_info->isGO && (pkt_type == TBOW_TYPE_DATA_SCO)) {
#ifdef GC_LOOP_USE_RX_LBUF
				PKTPUSH(tbow_info->wlc->osh, p, sizeof(tbow_header_t) +
					TBOW_ETH_HDR_LEN);
				ret = tbow_queue_btdata(tbow_info, TRUE, NULL, 0, p, pkt_type,
					TBOW_TAIL_FLAG_MAC_ACK, 0, NULL);
#else
				ret = tbow_queue_btdata(tbow_info, TRUE,
					PKTDATA(tbow_info->wlc->osh, p),
					PKTLEN(tbow_info->wlc->osh, p), NULL, pkt_type,
					TBOW_TAIL_FLAG_MAC_ACK, 0, NULL);
#endif

				if (ret) {
#ifdef GC_LOOP_USE_RX_LBUF
					PKTPULL(tbow_info->wlc->osh, p, TBOW_ETH_HDR_LEN);
#else
					PKTPUSH(tbow_info->wlc->osh, p, sizeof(tbow_header_t));
#endif
					pktq_penq_head(&tbow_info->recv_pq, pkt_type, p);
					break;
				} else {
					chan_state->exp_rx_seq_num =
						(uint16)(tbow_header->seq_num + 1);
#ifndef GC_LOOP_USE_RX_LBUF
					PKTFREE(tbow_info->wlc->osh, p, FALSE);
#endif
					continue;
				}
			}
#endif /* GC_LOOP_SCO */

			ret = wlc_bwte_send(tbow_info->wlc->bwte_info, WLC_BWTE_CLIENT_TBOW,
				payload,
				PKTDATA(tbow_info->wlc->osh, p), PKTLEN(tbow_info->wlc->osh, p),
				tbow_data_free, tbow_info);
			if (!ret) {
				data_sent = TRUE;
				chan_state->exp_rx_seq_num = (uint16)(tbow_header->seq_num + 1);
				if (tbow_info->rx_wlan_ho_stop &&
					((uint16)(chan_state->pend_eos_seq + 1) ==
					chan_state->exp_rx_seq_num)) {
					tbow_info->pend_eos_chan--;
					WL_TRACE(("%s: pkt_type(%d), pend_eos_chan(%d)\n",
						__FUNCTION__, pkt_type, tbow_info->pend_eos_chan));
				}
			} else {
				PKTPUSH(tbow_info->wlc->osh, p, sizeof(tbow_header_t));
				qBack = TRUE;
			}
		} else {
			qWait = TRUE;
		}

		if (qWait || qBack)
		{
			pktq_penq_head(&tbow_info->recv_pq, pkt_type, p);

			if (qWait && !chan_state->wait_for_pkt) {
				WL_TRACE(("early packet, pkt_type(%d), cur_seq(%d), exp_seq(%d)\n",
					pkt_type, tbow_header->seq_num,
					chan_state->exp_rx_seq_num));

				chan_state->wait_for_pkt = TRUE;
#ifndef GC_LOOP_SCO
				wl_add_timer(tbow_info->wlc->wl, chan_timer, TBOW_REORDER_TIMEOUT,
					FALSE);
#endif
			}
			break;
		}
	}

	if (data_sent && tbow_info->rx_wlan_ho_stop && !tbow_info->pend_eos_chan) {
		tbow_send_bt_ho_stop(tbow_info, tbow_info->rx_wlan_ho_stop);
		tbow_info->rx_wlan_ho_stop = 0;
	}

#ifdef GC_LOOP_SCO
	if (!tbow_info->isGO && pktq_len(&tbow_info->send_pq)) {
		tbow_tunnel_btdata(tbow_info);
	}
#endif
}

static bool
tbow_prec_enque(struct pktq *pq, int prec, void* p)
{
	struct pktq_prec *q;
	uint16 seq, seq2;
	void *p2, *p2_prev;

	if (!pq || !p)
		return FALSE;


	ASSERT(prec >= 0 && prec < pq->num_prec);
	ASSERT(PKTLINK(p) == NULL);         /* queueing chains not allowed */

	ASSERT(!pktq_full(pq));
	ASSERT(!pktq_pfull(pq, prec));

	q = &pq->q[prec];

	PKTSETLINK(p, NULL);
	if (q->head == NULL) {
		/* empty queue */
		q->head = p;
		q->tail = p;
	} else {
		seq = ((tbow_header_t *)PKTDATA(NULL, p))->seq_num;
		seq2 = ((tbow_header_t *)PKTDATA(NULL, q->tail))->seq_num;

		if (seq == seq2) {
			WL_TRACE(("duplicate pkt, seq(%d)\n", seq));
			return FALSE;
		}

		if (TBOW_IS_SEQ_ADVANCED(seq, seq2)) {
			/* correct sequence */
			PKTSETLINK(q->tail, p);
			q->tail = p;
		} else {
			/* need reorder */
			p2 = q->head;
			p2_prev = NULL;
			seq2 = ((tbow_header_t *)PKTDATA(NULL, p2))->seq_num;

			while (TBOW_IS_SEQ_ADVANCED(seq, seq2)) {
				if (seq == seq2) {
					WL_TRACE(("duplicate pkt, seq(%d)\n", seq));
					return FALSE;
				}

				p2_prev = p2;
				p2 = PKTLINK(p2);
				if (!p2) {
					break;
				}
				seq2 = ((tbow_header_t *)PKTDATA(NULL, p2))->seq_num;
			}

			if (p2_prev == NULL) {
				/* insert head */
				PKTSETLINK(p, q->head);
				q->head = p;
			} else if (p2 == NULL) {
				/* insert tail */
				PKTSETLINK(p2_prev, p);
				q->tail = p;
			} else {
				/* insert after p2_prev */
				PKTSETLINK(p, PKTLINK(p2_prev));
				PKTSETLINK(p2_prev, p);
			}
		}
	}

	q->len++;
	pq->len++;

	if (pq->hi_prec < prec)
		pq->hi_prec = (uint8)prec;

	return TRUE;
}

static void tbow_process_wlanmsg(tbow_info_t *tbow_info, uchar *msg_buf, int msg_len)
{
	tbow_ho_stop_msg_t *ho_stop_msg;
	tbow_datachan_state_t *chan_state;
	char *msg_str;
	int i;

	if (!msg_buf || (msg_len < 1)) {
		WL_ERROR(("%s: invalid paramter, tbow_info(%p), msg_buf(%p), msg_len(%d)\n",
			__FUNCTION__, tbow_info, msg_buf, msg_len));
		return;
	}

	msg_str = tbow_get_msgstr(*msg_buf);
	BCM_REFERENCE(msg_str);
	WL_TRACE(("[peer->self] ctl msg(0x%x, %s) from peer\n", *msg_buf, msg_str));

	switch (*msg_buf) {
		case TBOW_HO_MSG_STOP:
			tbow_send_bt_ho_stop(tbow_info, TBOW_HO_MSG_STOP);
			break;
		case TBOW_HO_MSG_S1:
		case TBOW_HO_MSG_S2:
		case TBOW_HO_MSG_S3:
			if (msg_len < sizeof(tbow_ho_stop_msg_t)) {
				WL_ERROR(("%s: invalid msg_len(%d)\n", __FUNCTION__, msg_len));
				break;
			}
			ho_stop_msg = (tbow_ho_stop_msg_t *)msg_buf;
			tbow_info->rx_wlan_ho_stop = ho_stop_msg->msg_type;
			if (ho_stop_msg->msg_type != TBOW_HO_MSG_S3) {
				int eos_chan_cnt = 0;
				for (i = 0; i < TBOW_TYPE_DATA_CNT; i++) {
					chan_state = &tbow_info->datachan_state[i];
					chan_state->pend_eos_seq = ho_stop_msg->last_seq[i];
#ifdef GO_LOOP_SCO
					if (i == TBOW_TYPE_DATA_SCO) {
						chan_state->pend_eos_seq = tbow_info->isGO ?
							(chan_state->tx_seq_num -1) : -1;
					}
#endif
					if ((uint16)(chan_state->pend_eos_seq + 1) !=
						chan_state->exp_rx_seq_num) {
						WL_TRACE(("%s: pkt_type(%d), eos_seq(%d)\n",
							__FUNCTION__, i, chan_state->pend_eos_seq));
						eos_chan_cnt++;
					}
				}
				tbow_info->pend_eos_chan = eos_chan_cnt;
				if (ho_stop_msg->msg_type == TBOW_HO_MSG_S2) {
					/* recv stop2, clear stop1 send timer and flag */
					WL_TRACE(("clear stop1 send timer and flag\n"));
					wl_del_timer(tbow_info->wlc->wl,
						tbow_info->stopmsg_timer);
					tbow_info->tx_wlan_ho_stop = 0;
				}
			} else {
				tbow_info->pend_eos_chan = 0;
				/* recv stop3, clear stop2 send flag */
				WL_TRACE(("clear TBOW_HO_MSG_S2 send timer and flag\n"));
				tbow_info->tx_wlan_ho_stop = 0;
				/* call stop function to tear down link */
				WL_TRACE(("%s: ready to tear down link\n", __FUNCTION__));
				tbow_ho_stop(tbow_info);
			}

			if (tbow_info->rx_wlan_ho_stop && !tbow_info->pend_eos_chan) {
				tbow_send_bt_ho_stop(tbow_info, tbow_info->rx_wlan_ho_stop);
				tbow_info->rx_wlan_ho_stop = 0;
			}
			break;
		case TBOW_HO_MSG_BT_DROP:
		case TBOW_HO_MSG_BT_RESET: {
			uchar *msg;

			tbow_info->pull_bt_stop = TRUE;
			tbow_info->push_bt_stop = TRUE;
			tbow_info->tunnel_stop = TRUE;
			tbow_ho_stop(tbow_info);

			msg = MALLOC(tbow_info->wlc->osh, 1);
			if (!msg) {
				WL_ERROR(("%s: alloc msg memory failed\n", __FUNCTION__));
				break;
			}

			if (*msg_buf == TBOW_HO_MSG_BT_DROP) {
				*msg = TBOW_HO_MSG_BT_DROP;
			} else if (*msg_buf == TBOW_HO_MSG_BT_RESET) {
				tbow_info->linkdown_timer_set = TRUE;
				*msg = TBOW_HO_MSG_WLAN_DROP;
			} else {
				WL_ERROR(("%s: unknown msg %d\n", __FUNCTION__, *msg_buf));
				break;
			}
			tbow_send_bt_msg(tbow_info, msg, 1);
			break;
		}

		case TBOW_HO_MSG_BT_SUSPEND_FAILED:
		case TBOW_HO_MSG_BT_RESUME_FAILED:
		{
			uchar *msg;
			msg = MALLOC(tbow_info->wlc->osh, 1);
			if (!msg) {
				WL_ERROR(("%s: alloc msg memory failed\n", __FUNCTION__));
				break;
			}
			*msg = *msg_buf;
			tbow_send_bt_msg(tbow_info, msg, 1);
			break;
		}

		default:
			break;
	}

}

static void tbow_force_release_acl(void *arg)
{
	tbow_info_t *tbow_info = (tbow_info_t *)arg;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return;
	}

	/* free or queue wlan data */
	tbow_send_bt_wlandata(tbow_info, TBOW_TYPE_DATA_ACL, TRUE);
}

static void tbow_force_release_sco(void *arg)
{
	tbow_info_t *tbow_info = (tbow_info_t *)arg;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return;
	}

	/* free or queue wlan data */
	tbow_send_bt_wlandata(tbow_info, TBOW_TYPE_DATA_SCO, TRUE);
}

static void tbow_bt_send_ho_stop(tbow_info_t *tbow_info)
{
	tbow_ho_stop_msg_t ho_stop_msg;
	int i, ret;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return;
	}

	if (tbow_info->tx_wlan_ho_stop == 0) {
		WL_ERROR(("%s: tx_wlan_ho_stop is 0, no stop msg to send\n", __FUNCTION__));
		return;
	}

	ho_stop_msg.msg_type = tbow_info->tx_wlan_ho_stop;

	WL_TRACE(("[self->peer] ctl msg(0x%x, %s)\n", ho_stop_msg.msg_type,
		tbow_get_msgstr(ho_stop_msg.msg_type)));

	for (i = 0; i < TBOW_TYPE_DATA_CNT; i++) {
		ho_stop_msg.last_seq[i] = tbow_info->datachan_state[i].tx_seq_num - 1;
		WL_TRACE(("pkt_type(%d), last_seq(%d)\n", i, ho_stop_msg.last_seq[i]));
	}

	if (tbow_info->tx_wlan_ho_stop != TBOW_HO_MSG_S3) {
		ret = tbow_queue_btdata(tbow_info, TRUE, (uchar *)&ho_stop_msg, sizeof(ho_stop_msg),
			NULL, TBOW_TYPE_CTL, TBOW_TAIL_FLAG_STACK_ACK, TBOW_STOPMSG_TIMEOUT,
			tbow_info->stopmsg_timer);
	} else {
		ret = tbow_queue_btdata(tbow_info, TRUE, (uchar *)&ho_stop_msg, sizeof(ho_stop_msg),
			NULL, TBOW_TYPE_CTL, TBOW_TAIL_FLAG_MAC_ACK, 0, NULL);
	}

	if (ret) {
		WL_ERROR(("%s: queue bt data failed\n", __FUNCTION__));
	}

	tbow_tunnel_btdata(tbow_info);
}

static void tbow_send_ho_stop_timeout(void *arg)
{
	tbow_info_t *tbow_info = (tbow_info_t *)arg;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return;
	}

	if (tbow_info->tx_wlan_ho_stop == TBOW_HO_MSG_S1) {
		/* retry HO_MSG_S1 msg */
		tbow_bt_send_ho_stop(tbow_info);
	} else if (tbow_info->tx_wlan_ho_stop == TBOW_HO_MSG_S2) {
		/* HO_MSG_S2 send timeout */
		if (!tbow_is_link_ready(tbow_info)) {
			/* connection lost means HO_S3 lost, send BT fake HO_MSG_S3 */
			/* call stop function to tear down link */
			tbow_info->tx_wlan_ho_stop = 0;

			tbow_send_bt_ho_stop(tbow_info, TBOW_HO_MSG_S3);
		}
	}
}

static void tbow_process_btmsg(tbow_info_t *tbow_info)
{
	uchar* msg = NULL;
	int msg_len = 0;
	bool passdown_msg = TRUE;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return;
	}

	msg = tbow_info->msg;
	msg_len = tbow_info->msg_len;
	if (!msg || !msg_len) {
		WL_ERROR(("%s: no msg(%p, %d)\n", __FUNCTION__, msg, msg_len));
		return;
	}

	WL_TRACE(("[bt->wlan] ctl msg(%p, %d), pkt_type(0x%x, %s)\n",
		msg, msg_len, *msg, tbow_get_msgstr(*msg)));

	/* call handle function for this pkt type */
	switch (*msg) {
		case TBOW_HO_MSG_START:
			/* clean up */
			tbow_cleanup(tbow_info);
			break;

		case TBOW_HO_MSG_STOP:
		case TBOW_HO_MSG_BT_SUSPEND_FAILED:
		case TBOW_HO_MSG_BT_RESUME_FAILED:
		{
			uchar msg_buf[1];

			msg_buf[0] = *msg;
			tbow_queue_btdata(tbow_info, TRUE, msg_buf, 1, NULL,
				TBOW_TYPE_CTL, TBOW_TAIL_FLAG_MAC_ACK, 0, NULL);
			tbow_tunnel_btdata(tbow_info);
			passdown_msg = FALSE;
			break;
		}

		case TBOW_HO_MSG_S1:
		case TBOW_HO_MSG_S2:
		case TBOW_HO_MSG_S3:
			/* start hand over stop process */
			tbow_info->pull_bt_stop = TRUE;
			tbow_info->tx_wlan_ho_stop = *msg;
			tbow_bt_send_ho_stop(tbow_info);
			passdown_msg = FALSE;
			break;

		case TBOW_HO_MSG_GO_STA_SETUP:
			tbow_info->isGO = FALSE;
			break;

		case TBOW_HO_MSG_SETUP_ACK:
			tbow_info->isGO = TRUE;
			break;

		case TBOW_HO_MSG_BT_DROP:
		case TBOW_HO_MSG_BT_RESET: {
			uchar msg_buf[1];

			/* stop consuming bt data */
			tbow_info->pull_bt_stop = TRUE;

			/* stop pushing bt data */
			tbow_info->push_bt_stop = TRUE;

			/* reclaim wlan buf */
			wlc_bwte_reclaim_wlan_buf(tbow_info->wlc->bwte_info, WLC_BWTE_CLIENT_TBOW,
				WLC_BWTE_HI_DATA, TRUE);
			wlc_bwte_reclaim_wlan_buf(tbow_info->wlc->bwte_info, WLC_BWTE_CLIENT_TBOW,
				WLC_BWTE_LO_DATA, TRUE);

			passdown_msg = FALSE;

			/* tunnel bt drop msg */
			msg_buf[0] = *msg;
			tbow_queue_btdata(tbow_info, TRUE, msg_buf, 1, NULL, TBOW_TYPE_CTL,
				TBOW_TAIL_FLAG_MAC_ACK, 0, NULL);
			tbow_tunnel_btdata(tbow_info);
			break;
		}

		case TBOW_HO_MSG_BT_DROP_ACK:
		case TBOW_HO_MSG_WLAN_DROP_ACK:
			/* reclaim wlan buf */
			wlc_bwte_reclaim_wlan_buf(tbow_info->wlc->bwte_info, WLC_BWTE_CLIENT_TBOW,
				WLC_BWTE_HI_DATA, TRUE);
			wlc_bwte_reclaim_wlan_buf(tbow_info->wlc->bwte_info, WLC_BWTE_CLIENT_TBOW,
				WLC_BWTE_LO_DATA, TRUE);
			passdown_msg = FALSE;
			break;

		case TBOW_HO_MSG_WLAN_RSSI_QUERY: {
			uchar *msg_buf;
			int msg_len = sizeof(uchar) + sizeof(int);
			int *p_rssi;
			wlc_bsscfg_t *bsscfg = NULL;

			passdown_msg = FALSE;

			msg_buf = MALLOC(tbow_info->wlc->osh, msg_len);
			if (!msg_buf) {
				WL_ERROR(("%s: alloc msg memory failed\n", __FUNCTION__));
				break;
			}
			*msg_buf = TBOW_HO_MSG_WLAN_RSSI_RESP;
			p_rssi = (int *)(msg_buf + 1);

			bsscfg = tbow_is_link_ready(tbow_info);
			if (!bsscfg) {
				WL_ERROR(("%s: wlan link not ready\n", __FUNCTION__));
				*p_rssi = WLC_RSSI_INVALID;
			} else {
				int err;
				scb_val_t scb_val_arg;
				if (tbow_info->isGO) {
					bcopy(&tbow_info->remote_addr, &scb_val_arg.ea,
						ETHER_ADDR_LEN);
				} else {
					bcopy(&tbow_info->local_addr, &scb_val_arg.ea,
						ETHER_ADDR_LEN);
				}
				scb_val_arg.val = 0;

				err = wlc_ioctl(tbow_info->wlc, WLC_GET_RSSI, &scb_val_arg,
					sizeof(scb_val_arg), bsscfg->wlcif);
				if (err) {
					WL_ERROR(("%s: err(%d) from WLC_GET_RSSI\n", __FUNCTION__,
						err));
					*p_rssi = WLC_RSSI_INVALID;
				} else {
					*p_rssi = (int)(scb_val_arg.val);
				}
			}

			tbow_send_bt_msg(tbow_info, msg_buf, 1);

			break;
		}

		case TBOW_HO_MSG_TEAR_WLAN:
			passdown_msg = FALSE;
			tbow_info->tunnel_stop = TRUE;
			tbow_ho_stop(tbow_info);
			break;

		case TBOW_HO_MSG_WLAN_DROP:
		case TBOW_HO_MSG_WLAN_RSSI_RESP:
		case TBOW_HO_MSG_BT_RESET_ACK:
			passdown_msg = FALSE;
			break;

		default:
			break;

	}

	if (passdown_msg) {
		tbow_ho_parse_ctrlmsg(tbow_info, msg, msg_len);
	}

	MFREE(tbow_info->wlc->osh, msg, msg_len);
	tbow_info->msg = NULL;
	tbow_info->msg_len = 0;
}

static void tbow_ist(void *arg)
{
	tbow_info_t *tbow_info = (tbow_info_t *)arg;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return;
	}

	tbow_info->ist_timer_set = FALSE;

	if (tbow_info->msg && tbow_info->msg_len) {
		tbow_process_btmsg(tbow_info);
	}

	/* check to send possible packets */
	if (pktq_len(&tbow_info->send_pq) && tbow_is_link_ready(tbow_info)) {
		tbow_tunnel_btdata(tbow_info);
	}

	if (pktq_plen(&tbow_info->recv_pq, TBOW_TYPE_DATA_ACL)) {
		tbow_send_bt_wlandata(tbow_info, TBOW_TYPE_DATA_ACL, FALSE);
	}

	if (pktq_plen(&tbow_info->recv_pq, TBOW_TYPE_DATA_SCO)) {
		tbow_send_bt_wlandata(tbow_info, TBOW_TYPE_DATA_SCO, FALSE);
	}
}

static int tbow_process_btdata(tbow_info_t *tbow_info, tbow_pkt_type_t pkt_type, uchar* p, int len)
{
	int ret = 0;

	if (!tbow_info) {
		WL_ERROR(("%s: NULL tbow_info pointer\n", __FUNCTION__));
		return -1;
	}

	if (pkt_type >= TBOW_TYPE_DATA_CNT)
	{
		WL_ERROR(("%s: invalid pkt_type(%d)\n", __FUNCTION__, pkt_type));
		return -1;
	}

	if (tbow_info->pull_bt_stop) {
		return -1;
	}

#if defined(GO_LOOP_SCO) || defined(GC_LOOP_SCO)
	if (!tbow_info->isGO && (pkt_type == TBOW_TYPE_DATA_SCO)) {
		/* bypass gc side sco data */
		return 0;
	}
#endif
	/* have data to queue */
	ret = tbow_queue_btdata(tbow_info, TRUE, p, len, NULL, pkt_type, TBOW_TAIL_FLAG_MAC_ACK,
		0, NULL);
	if (!ret && !tbow_info->ist_timer_set) {
		tbow_info->ist_timer_set = TRUE;
		wl_add_timer(tbow_info->wlc->wl, tbow_info->ist_timer, 0, FALSE);
	}

	return ret;
}

static int tbow_lo_data_cb(void* arg, uchar* p, int len)
{
	tbow_info_t *tbow_info = (tbow_info_t *)arg;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return -1;
	}

	return tbow_process_btdata(tbow_info, TBOW_TYPE_DATA_ACL, p, len);
}

static int tbow_hi_data_cb(void* arg, uchar* p, int len)
{
	tbow_info_t *tbow_info = (tbow_info_t *)arg;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return -1;
	}

	return tbow_process_btdata(tbow_info, TBOW_TYPE_DATA_SCO, p, len);
}

static int tbow_msg_cb(void* arg, uchar* msg, int len)
{
	tbow_info_t *tbow_info = (tbow_info_t *)arg;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return -1;
	}

	if (tbow_info->msg) {
		WL_ERROR(("%s: existing un-handled msg\n", __FUNCTION__));
		return -1;
	}

	tbow_info->msg = MALLOC(tbow_info->wlc->osh, len);
	if (!tbow_info->msg) {
		WL_ERROR(("%s: malloc msg buf(len:%d)failed\n", __FUNCTION__, len));
		return -1;
	}

	bcopy(msg, tbow_info->msg, len);
	tbow_info->msg_len = len;

	if (!tbow_info->ist_timer_set) {
		tbow_info->ist_timer_set = TRUE;
		wl_add_timer(tbow_info->wlc->wl, tbow_info->ist_timer, 0, FALSE);
	}

	return 0;
}

static int tbow_msg_free(void* arg, uchar* p, int len)
{
	tbow_info_t *tbow_info = (tbow_info_t *)arg;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return -1;
	}

	WL_TRACE(("free wlan ctl msg(%p, %d)\n", p, len));
	if (p) {
		MFREE(tbow_info->wlc->osh, p, len);
	}

	return 0;
}

tbow_info_t *
BCMATTACHFN(wlc_tbow_attach)(wlc_info_t* wlc)
{
	tbow_info_t *tbow_info;
	char *rmacaddr = NULL;
	struct ether_addr local_addr;
	struct ether_addr remote_addr;
	int ret;

	if (!wlc) {
		WL_ERROR(("%s: wlc is NULL\n", __FUNCTION__));
		return NULL;
	}

	tbow_info = MALLOC(wlc->osh, sizeof(tbow_info_t));
	if (tbow_info == NULL) {
		WL_ERROR(("wl%d: %s: MALLOC failed, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}
	bzero(tbow_info, sizeof(tbow_info_t));

	tbow_info->wlc = wlc;

	/* need remove later as HOM will pass the infromation */
	if ((rmacaddr = getvar(NULL, "lmacaddr"))) {
		bcm_ether_atoe(rmacaddr, &local_addr);
	}

	if ((rmacaddr = getvar(NULL, "rmacaddr"))) {
		bcm_ether_atoe(rmacaddr, &remote_addr);
	}

	tbow_set_mac(tbow_info, &local_addr, &remote_addr);
	/* need remove end */

{
	int i;

	for (i = 0; i < TBOW_TYPE_DATA_CNT; i++) {
		WL_TRACE(("pkt_type(%d), tx_seq(%p), exp_rx_seq(%p)\n", i,
			&tbow_info->datachan_state[i].tx_seq_num,
			&tbow_info->datachan_state[i].exp_rx_seq_num));
	}

}
	tbow_info->ist_timer =
		wl_init_timer(wlc->wl, tbow_ist, tbow_info, "tbow_ist_timer");
	tbow_info->data_acl_timer =
		wl_init_timer(wlc->wl, tbow_force_release_acl, tbow_info, "tbow_data_acl_timer");
	tbow_info->data_sco_timer =
		wl_init_timer(wlc->wl, tbow_force_release_sco, tbow_info, "tbow_data_sco_timer");
	tbow_info->stopmsg_timer =
		wl_init_timer(wlc->wl, tbow_send_ho_stop_timeout, tbow_info, "tbow_stopmsg_timer");
	tbow_info->linkdown_timer =
		wl_init_timer(wlc->wl, tbow_linkdown, tbow_info, "tbow_linkdown_timer");

	pktq_init(&tbow_info->send_pq, TBOW_TYPE_CNT, TBOW_PKTQ_CNT);
	pktq_init(&tbow_info->recv_pq, TBOW_TYPE_CNT, TBOW_PKTQ_CNT);

	ret = wlc_bwte_register_client(wlc->bwte_info, WLC_BWTE_CLIENT_TBOW, tbow_msg_cb,
		tbow_lo_data_cb, tbow_hi_data_cb, tbow_info);
	if (ret) {
		WL_ERROR(("%s: wlc_bwte_register_client failed, ret=%d\n", __FUNCTION__, ret));
		MFREE(wlc->osh, tbow_info, sizeof(tbow_info_t));
		return NULL;
	}

	/* register a module (to handle iovars) */
	if (wlc_module_register(wlc->pub, wl_tbow_iovars, "wl_tbow_iovars", tbow_info,
		tbow_doiovar, NULL, NULL, NULL)) {
		WL_ERROR(("wl%d: Error registering tbow iovar\n", wlc->pub->unit));
		MFREE(wlc->osh, tbow_info, sizeof(tbow_info_t));
		return NULL;
	}

	return tbow_info;
}

void
BCMATTACHFN(wlc_tbow_detach)(tbow_info_t *tbow_info)
{
	if (tbow_info && tbow_info->wlc) {
		tbow_cleanup(tbow_info);

		wlc_bwte_unregister_client(tbow_info->wlc->bwte_info, WLC_BWTE_CLIENT_TBOW);

		if (tbow_info->msg && tbow_info->msg_len) {
			MFREE(tbow_info->wlc->osh, tbow_info->msg, tbow_info->msg_len);
			tbow_info->msg = NULL;
			tbow_info->msg_len = 0;
		}

		MFREE(tbow_info->wlc->osh, tbow_info, sizeof(tbow_info_t));
	}
}

void tbow_set_mac(tbow_info_t *tbow_info, struct ether_addr *p_local, struct ether_addr *p_remote)
{
	if (!tbow_info || !p_local || !p_remote) {
		WL_ERROR(("%s: NULL pointer, tbow_info(%p), p_local(%p), p_remote(%p)\n",
			__FUNCTION__, tbow_info, p_local, p_remote));
	}

	bcopy(p_local, &tbow_info->local_addr, sizeof(struct ether_addr));
	bcopy(p_remote, &tbow_info->remote_addr, sizeof(struct ether_addr));

	WL_TRACE(("local_addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
		tbow_info->local_addr.octet[0], tbow_info->local_addr.octet[1],
		tbow_info->local_addr.octet[2], tbow_info->local_addr.octet[3],
		tbow_info->local_addr.octet[4], tbow_info->local_addr.octet[5]));

	WL_TRACE(("remote_addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
		tbow_info->remote_addr.octet[0], tbow_info->remote_addr.octet[1],
		tbow_info->remote_addr.octet[2], tbow_info->remote_addr.octet[3],
		tbow_info->remote_addr.octet[4], tbow_info->remote_addr.octet[5]));

}

/* msg_buf should come from malloc and caller give up ownership after success invocation */
int tbow_send_bt_msg(tbow_info_t *tbow_info, uchar* msg_buf, int msg_len)
{
	int ret;

	if (!tbow_info) {
		WL_ERROR(("%s: tbow_info is NULL\n", __FUNCTION__));
		return -1;
	}

	if (!msg_buf || !msg_len) {
		WL_ERROR(("%s: msg buffer error, msg(%p, %d)\n", __FUNCTION__, msg_buf, msg_len));
		return -1;
	}

	ret = wlc_bwte_send(tbow_info->wlc->bwte_info, WLC_BWTE_CLIENT_TBOW, WLC_BWTE_CTL_MSG,
		msg_buf, msg_len, tbow_msg_free, tbow_info);

	WL_TRACE(("[wlan->bt] ctl msg(%p, %d), pkt_type(0x%x, %s), ret(%d)\n", msg_buf, msg_len,
		*msg_buf, tbow_get_msgstr(*msg_buf), ret));

	if (*msg_buf == TBOW_HO_MSG_WLAN_READY) {
		tbow_is_link_ready(tbow_info);
	}

	return ret;
}

/* TRUE for send up, FALSE for no send up */
bool tbow_recv_wlandata(tbow_info_t *tbow_info, void *sdu)
{
	uint8 *frame;
	int length;
	uint8 *pt = NULL;
	struct ether_header *eth = NULL;
	tbow_header_t *tbow_header;
	void **sdu_loc;

	if (!tbow_info || !sdu) {
		WL_ERROR(("%s: NULL pointer, tbow_info(%p), sdu(%p)\n", __FUNCTION__, tbow_info,
			sdu));
		return TRUE;
	}

	frame = PKTDATA(tbow_info->wlc->osh, sdu);
	length = PKTLEN(tbow_info->wlc->osh, sdu);

	if (length <= ETHER_HDR_LEN) {
		return TRUE;
	}

	if (ntoh16_ua((const void *)(frame + ETHER_TYPE_OFFSET)) >= ETHER_TYPE_MIN) {
		/* Frame is Ethernet II */
		eth = (struct ether_header *)frame;
	} else {
		WL_ERROR(("%s: not ethernet II packet\n", __FUNCTION__));
		return TRUE;
	}

	if ((ntoh16(eth->ether_type) != ETHER_TYPE_TBOW)) {
		/* not BT pkt */
		return TRUE;
	}

	pt = frame + TBOW_ETH_HDR_LEN;
	length -= TBOW_ETH_HDR_LEN;

	if (length < sizeof(tbow_header_t)) {
		WL_ERROR(("incorrect tbow header length(%d)\n", length));
		PKTFREE(tbow_info->wlc->osh, (void *)sdu, FALSE);
		return FALSE;
	}

	/* tbow header */
	tbow_header = (tbow_header_t *)pt;
	switch (tbow_header->pkt_type) {
		case TBOW_TYPE_DATA_ACL:
		case TBOW_TYPE_DATA_SCO:
			{
				tbow_datachan_state_t *chan_state =
					&tbow_info->datachan_state[tbow_header->pkt_type];

				if (!TBOW_IS_SEQ_ADVANCED(tbow_header->seq_num,
					chan_state->exp_rx_seq_num)) {
					WL_TRACE(("late packet, pkt_type(%d),curr_seq(%u), "
						"exp_seq(%u)\n", tbow_header->pkt_type,
						tbow_header->seq_num, chan_state->exp_rx_seq_num));
					PKTFREE(tbow_info->wlc->osh, (void *)sdu, FALSE);
					break;
				}

				/* store lbuf pointer at 4 bytes right before tbow header */
				sdu_loc = (void**)((uint32)pt - sizeof(void *));
				*sdu_loc = sdu;

				PKTPULL(tbow_info->wlc->osh, sdu,
					PKTLEN(tbow_info->wlc->osh, sdu) - length);

				if (!tbow_prec_enque(&tbow_info->recv_pq, tbow_header->pkt_type,
					sdu)) {
					PKTFREE(tbow_info->wlc->osh, (void *)sdu, FALSE);
				}

				tbow_send_bt_wlandata(tbow_info, tbow_header->pkt_type, FALSE);
			}
			break;

		case TBOW_TYPE_CTL:
			tbow_process_wlanmsg(tbow_info, pt + sizeof(tbow_header_t),
				length - sizeof(tbow_header_t));
			PKTFREE(tbow_info->wlc->osh, (void *)sdu, FALSE);
			break;

		default:
			WL_ERROR(("unsupported pkt_type(%d)\n", tbow_header->pkt_type));
			PKTFREE(tbow_info->wlc->osh, (void *)sdu, FALSE);
			break;
	}

	return FALSE;
}
