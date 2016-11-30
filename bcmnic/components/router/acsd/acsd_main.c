/*
 * ACS deamon code
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * $Id: acsd_main.c 619998 2016-02-19 09:26:15Z $
 */

#include "acsd_svr.h"

acsd_wksp_t *d_info;

static void
acsd_close_listen_socket(acsd_wksp_t *d_info)
{
	/* close event dispatcher socket */
	if (d_info->listen_fd != ACSD_DFLT_FD) {
		ACSD_INFO("listenfd: close  socket %d\n", d_info->listen_fd);
		acsd_close_socket(d_info->listen_fd);
		d_info->listen_fd = ACSD_DFLT_FD;
	}
	return;
}

static void
acsd_close_event_socket(acsd_wksp_t *d_info)
{
	/* close event dispatcher socket */
	if (d_info->event_fd != ACSD_DFLT_FD) {
		ACSD_INFO("eventfd: close event socket %d\n", d_info->event_fd);
		acsd_close_socket(d_info->event_fd);
		d_info->event_fd = ACSD_DFLT_FD;
	}
	return;
}

static int
acsd_validate_wlpvt_message(int bytes, uint8 *dpkt)
{
	bcm_event_t *pvt_data;

	/* the message should be at least the header to even look at it */
	if (bytes < sizeof(bcm_event_t) + 2) {
		ACSD_ERROR("Invalid length of message\n");
		goto error_exit;
	}
	pvt_data  = (bcm_event_t *)dpkt;
	if (ntoh16(pvt_data->bcm_hdr.subtype) != BCMILCP_SUBTYPE_VENDOR_LONG) {
		ACSD_ERROR("%s: not vendor specifictype\n",
		       pvt_data->event.ifname);
		goto error_exit;
	}
	if (pvt_data->bcm_hdr.version != BCMILCP_BCM_SUBTYPEHDR_VERSION) {
		ACSD_ERROR("%s: subtype header version mismatch\n",
			pvt_data->event.ifname);
		goto error_exit;
	}
	if (ntoh16(pvt_data->bcm_hdr.length) < BCMILCP_BCM_SUBTYPEHDR_MINLENGTH) {
		ACSD_ERROR("%s: subtype hdr length not even minimum\n",
			pvt_data->event.ifname);
		goto error_exit;
	}
	if (bcmp(&pvt_data->bcm_hdr.oui[0], BRCM_OUI, DOT11_OUI_LEN) != 0) {
		ACSD_ERROR("%s: acsd_validate_wlpvt_message: not BRCM OUI\n",
			pvt_data->event.ifname);
		goto error_exit;
	}
	/* check for wl dcs message types */
	switch (ntoh16(pvt_data->bcm_hdr.usr_subtype)) {
		case BCMILCP_BCM_SUBTYPE_EVENT:

			ACSD_INFO("subtype: event\n");
			break;
		default:
			goto error_exit;
			break;
	}
	return ACSD_OK; /* good packet may be this is destined to us */
error_exit:
	return ACSD_FAIL;
}
/*
 * Receives and processes the commands from client
 * o Wait for connection from client
 * o Process the command and respond back to client
 * o close connection with client
 */
static int
acsd_proc_client_req(void)
{
	uint resp_size = 0;
	int rcount = 0;
	char* buf;
	int ret = 0;

	if (!d_info->cmd_buf)
		d_info->cmd_buf = acsd_malloc(ACSD_BUFSIZE_4K);

	buf = d_info->cmd_buf;

	/* get command from client */
	if ((rcount = sread(d_info->conn_fd, buf, ACSD_BUFSIZE_4K)) < 0) {
		ACSD_ERROR("Failed reading message from client \n");
		ret = -1;
		goto done;
	}

	/* reqeust is small string. */
	if (rcount == ACSD_BUFSIZE_4K) {
		ACSD_ERROR("Client Req too large\n");
		ret = -1;
		goto done;
	}
	buf[rcount] = '\0';

	acsd_proc_cmd(d_info, buf, rcount, &resp_size);

	if (swrite(d_info->conn_fd, buf, resp_size) < 0) {
		ACSD_ERROR("Failed sending message \n");
		ret = -1;
		goto done;
	}

done:
	acsd_close_socket(d_info->conn_fd);
	d_info->conn_fd = -1;
	return ret;
}

/* Check if stay in current channel long enough */
static bool
chanim_record_chan_dwell(acs_chaninfo_t *c_info, chanim_info_t *ch_info)
{
	acs_fcs_t *dcs_info = &c_info->acs_fcs;
	uint8 cur_idx = chanim_mark(ch_info).record_idx;
	uint8 start_idx;
	chanim_acs_record_t *start_record;
	time_t now = time(NULL);

	start_idx = MODSUB(cur_idx, 1, CHANIM_ACS_RECORD);
	start_record = &ch_info->record[start_idx];
	if (now - start_record->timestamp > dcs_info->acs_chan_dwell_time)
		return TRUE;
	return FALSE;
}

void
acsd_event_handler(uint8 *pkt, int bytes)
{
	uint16 ether_type = 0;
	uint32 evt_type;
	char *ifname = (char *)pkt;
	struct ether_header *eth_hdr;
	bcm_event_t *pvt_data;
	int idx = 0;
	bool chan_least_dwell = FALSE;
	wl_bcmdcs_data_t dcs_data;
	int err;
	acs_chaninfo_t *c_info;
#ifdef WL_MEDIA_ACSD
	int pkt_offset = 0; /* Media uses raw socket, no ifname in the pkt */
#else
	int pkt_offset = IFNAMSIZ;
#endif

	eth_hdr = (struct ether_header *)(pkt + pkt_offset);
	bytes -= pkt_offset;

	if ((ether_type = ntoh16(eth_hdr->ether_type) != ETHER_TYPE_BRCM)) {
		ACSD_INFO("recved ether type %x\n", ether_type);
		return;
	}

	if ((err = acsd_validate_wlpvt_message(bytes, (uint8 *)eth_hdr)))
		return;

	pvt_data = (bcm_event_t *)(pkt + pkt_offset);
	evt_type = ntoh32(pvt_data->event.event_type);
	ACSD_INFO("recved brcm event, event_type: %d\n", evt_type);
#ifdef WL_MEDIA_ACSD
	if ((idx = acs_idx_from_sa(pvt_data->eth.ether_dhost)) < 0) {
		return;
	}
#else
	if ((idx = acs_idx_from_map(ifname)) < 0) {
		ACSD_INFO("cannot find the mapped entry for ifname: %s\n", ifname);
		return;
	}
#endif
	c_info = d_info->acs_info->chan_info[idx];

#ifdef WL_MEDIA_ACSD
	ifname = c_info->name;
#endif

	ACSD_INFO("recved %d bytes from eventfd, ifname: %s\n",
		bytes, ifname);

	if (c_info->mode == ACS_MODE_DISABLE && c_info->acs_boot_only) {
		ACSD_INFO("No event handling enabled. Only boot selection \n");
		return;
	}

	if (!AUTOCHANNEL(c_info)) {
		ACSD_INFO("Event fail ACSD not in autochannel mode \n");
		return;
	}

	if (c_info->acs_bgdfs != NULL && c_info->acs_bgdfs->state != BGDFS_STATE_IDLE) {
		return;
	}

	d_info->stats.valid_events++;

	switch (evt_type) {

	case WLC_E_DCS_REQUEST: {
		dot11_action_wifi_vendor_specific_t * actfrm;
		actfrm = (dot11_action_wifi_vendor_specific_t *)(pvt_data + 1);

		if ((err = dcs_parse_actframe(actfrm, &dcs_data))) {
			ACSD_ERROR("err from dcs_parse_request: %d\n", err);
			break;
		}

		if ((err = dcs_handle_request(ifname, &dcs_data, DOT11_CSA_MODE_ADVISORY,
			DCS_CSA_COUNT, CSA_BROADCAST_ACTION_FRAME)))
			ACSD_ERROR("err from dcs_handle_request: %d\n", err);

		break;
	}
	case WLC_E_SCAN_COMPLETE: {
		ACSD_INFO("recved brcm event: scan complete\n");
		break;
	}
	case WLC_E_PKTDELAY_IND: {
		txdelay_event_t pktdelay;

		memcpy(&pktdelay, (txdelay_event_t *)(pvt_data + 1),
			sizeof(txdelay_event_t));

		/* stay in current channel more than acs_chan_dwell_time */
		chan_least_dwell = chanim_record_chan_dwell(c_info, c_info->chanim_info);

		if (chan_least_dwell &&
			(pktdelay.chanim_stats.chan_idle <
				c_info->acs_fcs.acs_fcs_chanim_stats)) {

			c_info->switch_reason = APCS_TXDLY;
#ifndef WL_MEDIA_ACSD
			acs_select_chspec(c_info);
#else
			dcs_select_rand_chspec(c_info);
#endif /* WL_MEDIA_ACSD */
			dcs_data.reason = 0;
			dcs_data.chspec = c_info->selected_chspec;

			if ((err = dcs_handle_request(ifname, &dcs_data,
				DOT11_CSA_MODE_ADVISORY, FCS_CSA_COUNT,
				c_info->acs_fcs.acs_dcs_csa)))
				ACSD_ERROR("err from dcs_handle_request: %d\n", err);
			else
				chanim_upd_acs_record(c_info->chanim_info,
					c_info->selected_chspec, APCS_TXDLY);
		}
		break;
	}
	case WLC_E_TXFAIL_THRESH: {
		wl_intfer_event_t *event;
		unsigned char *addr;

		if (!ACS_FCS_MODE(c_info)) {
			ACSD_FCS("Cannot handle event ACSD not in FCS mode \n");
			break;
		}

		/* ensure we have the latest channel information and dwell time etc */
		acs_update_status(c_info);

		event = (wl_intfer_event_t *)(pvt_data + 1);
		chan_least_dwell = chanim_record_chan_dwell(c_info, c_info->chanim_info);
		addr = (unsigned char *)(&(pvt_data->event.addr));

		ACSD_FCS("Intfer:%s Mac:%02x:%02x:%02x:%02x:%02x:%02x status=0x%x\n",
			ifname, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
			event->status);

		for (idx = 0; idx < WLINTFER_STATS_NSMPLS; idx++) {
			ACSD_FCS("0x%x\t", event->txfail_histo[idx]);
		}

		ACSD_FCS("\n time:%u", (uint32)time(NULL));

		if (!chan_least_dwell) {
			ACSD_FCS("chan_least_dwell is FALSE\n");
			break;
		}

		if (acsd_trigger_dfsr_check(c_info)) {
			ACSD_DFSR("trigger DFS reentry...\n");
			acs_dfsr_set(ACS_DFSR_CTX(c_info),
				c_info->cur_chspec, __FUNCTION__);
			break;
		}

		if (!acsd_need_chan_switch(c_info)) {
			ACSD_FCS("No channel switch...\n");
			break;
		}

		if (acsd_hi_chan_check(c_info)) {
			c_info->selected_chspec = c_info->dfs_forced_chspec;
			ACSD_FCS("Select 0x%x\n", c_info->selected_chspec);
		}
		else {
			c_info->switch_reason = APCS_TXFAIL;
#ifndef WL_MEDIA_ACSD
			acs_select_chspec(c_info);
#else
			dcs_select_rand_chspec(c_info);
#endif /* WL_MEDIA_ACSD */
		}

		if (!acs_bgdfs_attempt_on_txfail(c_info)) {
			dcs_data.reason = 0;
			dcs_data.chspec = c_info->selected_chspec;
			ACSD_INFO("%s Performing CSA on chspec 0x%x\n", c_info->name,
					dcs_data.chspec);
			if ((err = dcs_handle_request(ifname, &dcs_data,
					DOT11_CSA_MODE_ADVISORY, FCS_CSA_COUNT,
					c_info->acs_fcs.acs_dcs_csa))) {
				ACSD_ERROR("err from dcs_handle_request: %d\n", err);
			} else {
				c_info->recent_prev_chspec = c_info->cur_chspec;
				c_info->acs_prev_chan_at = time(NULL);
				acs_intfer_config(c_info);
				chanim_upd_acs_record(c_info->chanim_info,
					c_info->selected_chspec, APCS_TXFAIL);
			}
		}
		break;
	}
	default:
		ACSD_INFO("recved event type %x\n", evt_type);
		break;
	}
}

int
acsd_init(void)
{
	int err = ACSD_OK;
	uint  port = ACSD_DFLT_CLI_PORT;

	d_info = (acsd_wksp_t*)acsd_malloc(sizeof(acsd_wksp_t));

	d_info->version = ACSD_VERSION;
	d_info->fdmax = ACSD_DFLT_FD;
	d_info->event_fd = ACSD_DFLT_FD;
	d_info->listen_fd = ACSD_DFLT_FD;
	d_info->conn_fd = ACSD_DFLT_FD;
	d_info->poll_interval = ACSD_DFLT_POLL_INTERVAL;
	d_info->ticks = 0;
	d_info->cmd_buf = NULL;

#ifndef WL_MEDIA_ACSD
	err = acsd_open_event_socket(d_info);
#endif
	err = acsd_open_listen_socket(port, d_info);

	return err;
}

void
acsd_cleanup(void)
{
	if (d_info) {
		acs_cleanup(&d_info->acs_info);
		acsd_free(d_info->cmd_buf);
		acsd_free(d_info);
	}
}

void
acsd_watchdog(uint ticks)
{
	int i, ret;
	acs_chaninfo_t* c_info;

	for (i = 0; i < ACS_MAX_IF_NUM; i++) {
		c_info = d_info->acs_info->chan_info[i];

		if ((!c_info) || (c_info->mode == ACS_MODE_DISABLE))
			continue;
		ACSD_INFO("tick:%d\n", ticks);

		if (ticks % ACS_ASSOCLIST_POLL == 0)  {
			acs_update_assoc_info(c_info);
		}

		if (ACS_11H_AND_BGDFS(c_info) &&
				c_info->acs_bgdfs->state != BGDFS_STATE_IDLE) {
			acs_bgdfs_info_t * bgdfs = c_info->acs_bgdfs;
			time_t now = time(NULL);
			bool bgdfs_scan_done = FALSE;
			if ((ticks % ACS_BGDFS_SCAN_STATUS_CHECK_INTERVAL) == 0) {
				if ((ret = acs_bgdfs_get(c_info)) != BGDFS_CAP_TYPE0) {
					ACSD_ERROR("acs bgdfs get failed with %d\n", ret);
				}
				if (bgdfs->status.move_status != DFS_SCAN_S_INPROGESS) {
					bgdfs_scan_done = TRUE;
				}
			}
			if (bgdfs_scan_done || bgdfs->timeout < now) {
				bgdfs->state = BGDFS_STATE_IDLE;
				if (!bgdfs_scan_done &&
						(ret = acs_bgdfs_get(c_info)) != BGDFS_CAP_TYPE0) {
					ACSD_ERROR("acs bgdfs get failed with %d\n", ret);
				}
				if (bgdfs->fallback_blocking_cac &&
						bgdfs->acs_bgdfs_on_txfail &&
						((ret = acs_bgdfs_check_status(c_info, TRUE))
						 != BCME_OK)) {
					wl_bcmdcs_data_t dcs_data;
					int err;
					ACSD_INFO("%s####BGDFS Failed. Do Full MIMO CAC#####\n",
							c_info->name);

					dcs_data.reason = 0;
					dcs_data.chspec = c_info->selected_chspec;
					if ((err = dcs_handle_request(c_info->name,
							&dcs_data,
							DOT11_CSA_MODE_ADVISORY,
							FCS_CSA_COUNT,
							c_info->acs_fcs.acs_dcs_csa))) {
						ACSD_ERROR("%s Error dcs_handle_request: %d\n",
								c_info->name, err);
					}
					bgdfs->acs_bgdfs_on_txfail = FALSE;

				} else if (bgdfs->next_scan_chan != 0 || bgdfs->best_cleared != 0) {
					if ((ret = acs_bgdfs_check_status(c_info, FALSE))
							== BCME_OK) {
						ACSD_INFO("acs bgdfs ch 0x%x is radar free\n",
								bgdfs->next_scan_chan);
						if (bgdfs->next_scan_chan != 0) {
							c_info->dfs_forced_chspec =
								bgdfs->next_scan_chan;
							acs_set_dfs_forced_chspec(c_info);
						}
					} else if (bgdfs->next_scan_chan != 0) {
						ACSD_INFO("acs bgdfs chan 0x%x is not radar free "
								"(err: %d)\n",
								bgdfs->next_scan_chan, ret);
					}
					/* reset for next try; let it recompute channel */
					bgdfs->next_scan_chan = 0;
				}
			} else {
				continue;
			}
		}

		if (ACS_FCS_MODE(c_info)) {	/* Update channel idle times */
			if ((ticks % ACS_TRAFFIC_INFO_UPDATE_INTERVAL) == 0) {
				if ((ret = acs_activity_update(c_info)) != BCME_OK) {
					ACSD_ERROR("activity update failed");
				}

				(void) acs_bgdfs_idle_check(c_info);

				if (ACS_11H_AND_BGDFS(c_info) && c_info->acs_bgdfs->idle &&
						c_info->acs_bgdfs->state == BGDFS_STATE_IDLE) {
					if (c_info->acs_bgdfs->ahead &&
							acs_bgdfs_ahead_trigger_scan(c_info) !=
							BCME_OK) {
						ACSD_ERROR("BGDFS ahead trigger scan "
								"failed\n");
					}
				}
			}
			acs_dfsr_activity_update(ACS_DFSR_CTX(c_info), c_info->name);
		}

		if (ticks % ACS_STATUS_POLL == 0)
			acs_update_status(c_info);

		acsd_chanim_check(ticks, c_info);

		acs_scan_timer_or_dfsr_check(c_info); /* AUTOCHANNEL/DFSR is checked in called fn */

		/* Do CI scan only if not disabled */
		if (!CI_SCAN_DISABLED(c_info)) {
			if (AUTOCHANNEL(c_info) &&
				(acs_fcs_ci_scan_check(c_info) || CI_SCAN(c_info))) {
				acs_do_ci_update(ticks, c_info);
			}
		}

	}
}
#ifdef DEBUG
static void acsd_test_cswitch()
{
	char test_cswitch_ifname[32];
	wl_bcmdcs_data_t dcs_data;
	int err;
	int idx = 0;
	acs_chaninfo_t *c_info;

	strncpy(test_cswitch_ifname, nvram_safe_get("acs_test_cs"), sizeof(test_cswitch_ifname));
	test_cswitch_ifname[sizeof(test_cswitch_ifname) - 1] = '\0';
	nvram_unset("acs_test_cs");

	if (test_cswitch_ifname[0]) {
		if ((idx = acs_idx_from_map(test_cswitch_ifname)) < 0) {
			ACSD_ERROR("cannot find the mapped entry for ifname: %s\n",
				test_cswitch_ifname);
			return;
		}
		c_info = d_info->acs_info->chan_info[idx];

		ACSD_FCS("trigger Fake cswitch from:0x%x:...\n", c_info->cur_chspec);

		if (acsd_trigger_dfsr_check(c_info)) {
			ACSD_DFSR("trigger DFS reentry...\n");
			acs_dfsr_set(ACS_DFSR_CTX(c_info), c_info->cur_chspec, __FUNCTION__);
		}
		else {
			if (acsd_hi_chan_check(c_info)) {
				c_info->selected_chspec = c_info->dfs_forced_chspec;
				ACSD_FCS("Select 0x%x:...\n", c_info->selected_chspec);
			}
			else {
			    c_info->switch_reason = APCS_TXFAIL;
				acs_select_chspec(c_info);
			}
			dcs_data.reason = 1;
			dcs_data.chspec = c_info->selected_chspec;

			if ((err = dcs_handle_request(test_cswitch_ifname, &dcs_data,
				DOT11_CSA_MODE_ADVISORY, FCS_CSA_COUNT,
				c_info->acs_fcs.acs_dcs_csa))) {
				ACSD_ERROR("err from dcs_handle_request: %d\n", err);
			}
			else {
				acs_intfer_config(c_info);
				chanim_upd_acs_record(c_info->chanim_info,
					c_info->selected_chspec, APCS_TXFAIL);
			}
		}
	}
}
#endif /* DEBUG */

void acsd_set_event_fd(int fd)
{
	d_info->event_fd = fd;
}

/* service main entry */
int
main(int argc, char *argv[])
{
	int err = ACSD_OK;
	struct timeval tv;
	char *val;
	int bytes;
	int data_type;

	val = nvram_safe_get("acsd_debug_level");
	if (strcmp(val, ""))
		acsd_debug_level = strtoul(val, NULL, 0);

	ACSD_INFO("acsd start...\n");

	val = nvram_safe_get("acs_ifnames");
	if (!strcmp(val, "")) {
		ACSD_ERROR("No interface specified, exiting...");
		return err;
	}

	if ((err = acsd_init()))
		goto cleanup;

	acs_init_run(&d_info->acs_info);

#ifndef WL_MEDIA_ACSD
/* Media doesn't use daemon, instead a simple app is used */
#if !defined(DEBUG)
	if (acsd_daemonize())
		goto cleanup;
#endif /* !defined(DEBUG) */
#endif /* WL_MEDIA_ACSD */

	tv.tv_sec = d_info->poll_interval;
	tv.tv_usec = 0;

	while (1) {
		/* Don't change channel when WPS is in the processing,
		 * to avoid WPS fails
		 */
		if (ACS_WPS_RUNNING) {
			sleep_ms(1000);
			continue;
		}

		if (tv.tv_sec == 0 && tv.tv_usec == 0) {
			d_info->ticks ++;
			tv.tv_sec = d_info->poll_interval;
			tv.tv_usec = 0;
			ACSD_DEBUG("ticks: %d\n", d_info->ticks);
			acsd_watchdog(d_info->ticks);
		}

#ifdef DEBUG
		acsd_test_cswitch();
#endif
		data_type = acsd_wait_for_events(&tv, d_info);
		switch (data_type)
		{
		case ACSD_DATA_FROM_LISTEN_FD: {
			acsd_proc_client_req();
		}
		/* Fall through - check for data from event fd */
		case ACSD_DATA_FROM_EVENT_FD: {
			if (acsd_check_for_data_in_event_socket(d_info, &bytes) == TRUE) {
				acsd_event_handler(d_info->packet, bytes);
			}
			break;
		}
		default:
			break;
		}
#ifdef WL_MEDIA_ACSD
		/* Get logs to the file frequently */
		fflush(stdout);
#endif /* WL_MEDIA_ACSD */
	}

cleanup:
	acsd_close_event_socket(d_info);
	acsd_close_listen_socket(d_info);
	acsd_cleanup();
#ifdef WL_MEDIA_ACSD
	acsd_free_mem();
#endif /* WL_MEDIA_ACSD */
	return err;
}
