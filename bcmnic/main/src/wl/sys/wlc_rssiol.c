/*
 * Common (OS-independent) portion of
 * Broadcom 802.11 offload Driver
 *
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_rssiol.c 502164 2014-09-12 01:46:25Z $
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <proto/802.11.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_hw.h>
#include <wlc_hw_priv.h>
#include <wlc_bmac.h>
#include <proto/802.3.h>
#include <proto/ethernet.h>
#include <proto/vlan.h>
#include <proto/bcmarp.h>
#include <bcm_ol_msg.h>
#include <wlc_bcnol.h>
#include <wlc_dngl_ol.h>
#include <wlc_wowlol.h>
#include <wl_export.h>
#include "qmath.h"


#define RSSI_ANT_MERGE_MAX	0	/* pick max rssi of all antennas */
#define RSSI_ANT_MERGE_MIN	1	/* pick min rssi of all antennas */
#define RSSI_ANT_MERGE_AVG	2	/* pick average rssi of all antennas */

#define WLC_DNGL_OL_RSSI_SET(_dngl_ol, v)	(RXOESHARED(_dngl_ol)->rssi_info.rssi = v)
#define WLC_DNGL_OL_RSSI_RXPWR_SET(_dngl_ol, v, i) (RXOESHARED(_dngl_ol)->rssi_info.rxpwr[i] = v)

#define WLC_DNGL_OL_RSSI_ENABLED(o)		(o->enabled)
#define WLC_DNGL_OL_LOW_RSSI_THRESHOLD(o)	(o->low_threshold)
#define WLC_DNGL_OL_LOW_RSSI_ROAM_OFF(o)	(o->roam_off)
#define WLC_DNGL_OL_RSSI_MODE(o)		(o->mode)
#define WLC_DNGL_OL_RSSI_PHYRXCHAIN(o)		(o->phyrxchain)
#define WLC_DNGL_OL_RSSI_TEMPERATURE(o)		(o->current_temp)
#define WLC_DNGL_OL_RSSI_GAIN_ERROR(o, i)	(o->phy_rssi_gain_error[i])
#define WLC_DNGL_OL_RSSI_RAWTEMPSENSE(o)	(o->raw_tempsense)
#define WLC_DNGL_OL_RSSI_RADIO_CHANSPEC(o)	(o->radio_chanspec)
#define WLC_DNGL_OL_RSSI_NUM_MA_SAMPLES(o)	(o->num_samples)

#define WLC_DNGL_OL_LOW_RSSI_THRESHOLD_TIME(o)		(o->notification_sent_time)
#define WLC_DNGL_OL_LOW_RSSI_THRESHOLD_TIME_SET(o, i)	(o->notification_sent_time = i)
#define	WLC_DNGL_OL_LOW_RSSI_DELTA_TIME			(60)

#undef qx

/* copied from wlc_phy_upd_gain_wrt_temp_phy */
#define ACPHY_GAIN_VS_TEMP_SLOPE_2G 7	/* units: db/100C */
#define ACPHY_GAIN_VS_TEMP_SLOPE_5G 4	/* units: db/100C */
#define PHY_TEMPSENSE_MIN 0
#define PHY_TEMPSENSE_MAX 105

struct wlc_dngl_ol_rssi_info {
	wlc_dngl_ol_info_t *wlc_dngl_ol; /* Back pointer to our parent */
	uint8   enabled;                /* Flag to indicate RSSI enabled */
	int8    low_threshold;          /* Low RSSI Threshold value */
	int8    roam_off;               /* Low RSSI roam enabled */
	uint8   mode;                   /* Mode: MAX, MIN, AVG */
	uint8   phyrxchain;             /* Antenna chain */
	int8    phy_rssi_gain_error[WL_RSSI_ANT_MAX]; /* RSSI gain error */
	uint16  current_temp;           /* current temperature */
	uint16  raw_tempsense;          /* temperature from ROM */
	uint16  radio_chanspec;         /* Radio channel spec */
	int16	rxpwr_ma[MA_WINDOW_SZ];	/* Moving average array */
	int16	rxpwr_ma_ant[WL_RSSI_ANT_MAX][MA_WINDOW_SZ]; /* Moving average array */
	uint32	num_samples;		/* Number of samples in moving average */
	uint32	notification_sent_time;	/* when was the last time low RSSI notification sent */
};

wlc_dngl_ol_rssi_info_t *wlc_dngl_ol_rssi_attach(wlc_dngl_ol_info_t *wlc_dngl_ol);

int wlc_dngl_ol_phy_rssi_compute_offload(wlc_dngl_ol_rssi_info_t *rssi_ol, wlc_d11rxhdr_t *wlc_rxh);
void wlc_dngl_ol_rssi_send_proc(wlc_dngl_ol_rssi_info_t *rssi_ol, void *buf, int len);
void wlc_dngl_ol_reset_rssi_ma(wlc_dngl_ol_rssi_info_t *rssi_ol);

static int16	wlc_dngl_ol_calculate_rssi_ma(int16 rxpwr, int16 *win, uint32 win_size);

wlc_dngl_ol_rssi_info_t *
wlc_dngl_ol_rssi_attach(wlc_dngl_ol_info_t *wlc_dngl_ol)
{
	int i;
	wlc_dngl_ol_rssi_info_t *rssi_ol;

	rssi_ol = (wlc_dngl_ol_rssi_info_t *)MALLOC(wlc_dngl_ol->osh,
		sizeof(wlc_dngl_ol_rssi_info_t));
	if (!rssi_ol) {
		WL_ERROR(("rssi_ol malloc failed: %s\n", __FUNCTION__));
		return NULL;
	}

	WLC_DNGL_OL_RSSI_SET(wlc_dngl_ol, WLC_RSSI_EXCELLENT);
	for (i = 0; i < WL_RSSI_ANT_MAX; i++) {
		WLC_DNGL_OL_RSSI_RXPWR_SET(wlc_dngl_ol, WLC_RSSI_EXCELLENT, i);
	}

	bzero(rssi_ol, sizeof(wlc_dngl_ol_rssi_info_t));
	rssi_ol->wlc_dngl_ol = wlc_dngl_ol;

	return rssi_ol;
}

void wlc_dngl_ol_rssi_send_proc(wlc_dngl_ol_rssi_info_t *rssi_ol, void *buf, int len)
{
	int i;
	olmsg_rssi_init *rssi_init;
	rssi_init = (olmsg_rssi_init *)buf;

	if (!rssi_ol) {
		WL_TRACE(("rssi_ol not set: %s\n", __FUNCTION__));
		return;
	}
	rssi_ol->enabled = rssi_init->enabled;
	rssi_ol->low_threshold = rssi_init->low_threshold;
	rssi_ol->roam_off = rssi_init->roam_off;
	rssi_ol->mode = rssi_init->mode;
	rssi_ol->phyrxchain = rssi_init->phyrxchain;
	rssi_ol->current_temp = rssi_init->current_temp;
	rssi_ol->raw_tempsense = rssi_init->raw_tempsense;
	rssi_ol->radio_chanspec = rssi_init->radio_chanspec;
	for (i = 0; i < WL_RSSI_ANT_MAX; i++) {
		rssi_ol->phy_rssi_gain_error[i] = rssi_init->phy_rssi_gain_error[i];
	}

	/* Reset ALL values in averaging array as they may not make sense anymore */
	wlc_dngl_ol_reset_rssi_ma(rssi_ol);

	WL_TRACE(("RSSI init Params seting done: %s\n", __FUNCTION__));
}


void wlc_dngl_ol_reset_rssi_ma(wlc_dngl_ol_rssi_info_t *rssi_ol)
{
	int i, j;

	if (!rssi_ol) {
		WL_TRACE(("rssi_ol not set: %s\n", __FUNCTION__));
		return;
	}
	rssi_ol->num_samples = 0;
	for (i = 0; i < MA_WINDOW_SZ; i++) {
		rssi_ol->rxpwr_ma[i] = WLC_RSSI_EXCELLENT;
	}

	for (j = 0; j < WL_RSSI_ANT_MAX; j++)
		for (i = 0; i < MA_WINDOW_SZ; i++)
			rssi_ol->rxpwr_ma_ant[j][i] = WLC_RSSI_EXCELLENT;
}

static int16 wlc_dngl_ol_calculate_rssi_ma(int16 new_rxpwr, int16 *win, uint32 win_size)
{
	int		i, x, j;
	int16		ma = 0;

	if (new_rxpwr < 0) {
		win[(win_size++ % MA_WINDOW_SZ)] = new_rxpwr;
	} else {
		WL_ERROR(("%s: Invalid RSSI %d\n", __FUNCTION__, new_rxpwr));
	}

	x = (win_size > MA_WINDOW_SZ) ? MA_WINDOW_SZ : win_size;

	for (j = i = 0; i < x; i++) {
		ASSERT(win[i] < 0);
		WL_TRACE(("%s: DEBUG : RSSI WINDOW %d[%d]\n",
			__FUNCTION__, win[i], i));
		ma += win[i];
		++j;
	}
	if (j) {
		ma = ma / j;
	}

	return ma;
}

/*
Currently Assume ALL antennas are active
The antenna information and rssi_mode will be provided by
host driver at run time and when it changes
We have to also determine how to read temperature to calculate
temperature compensation
copied from wlc_phy_rssi_compute_acphy()
*/

int wlc_dngl_ol_phy_rssi_compute_offload(wlc_dngl_ol_rssi_info_t *rssi_ol, wlc_d11rxhdr_t *wlc_rxh)
{
	int16 rxpwr = 0;
	int16 num_activecores;
	int16 rxpwr_core[WL_RSSI_ANT_MAX], rxpwr_activecore[WL_RSSI_ANT_MAX];
	int16 is_status_hacked;
	int i;

	if (!rssi_ol) {
		WL_TRACE(("rssi_ol not set: %s\n", __FUNCTION__));
		return 0;
	}

	if (WLC_DNGL_OL_RSSI_ENABLED(rssi_ol) == 0) {
		WL_TRACE(("rssi_ol not enabled: %s\n", __FUNCTION__));
		return 0;
	}

	WL_TRACE(("%s: Enterings\n", __FUNCTION__));

	/* the rxpwr values in the RXH are already temperature compensated by PHY */
	rxpwr_core[0] = wlc_rxh->rxpwr[0];
	rxpwr_core[1] = wlc_rxh->rxpwr[1];

	/* Currently ARM offloads supports 2x2 config only.  Antenna 2 is
	 * calculated from 0 and 1
	 */
	rxpwr_core[2] = (rxpwr_core[0] + rxpwr_core[1]) / 2;
	rxpwr_core[3] = 0;

	WL_TRACE(("ADJ ANT Power: 0:%d 1:%d 2:%d\n",
		rxpwr_core[0], rxpwr_core[1], rxpwr_core[2]));

	num_activecores = 0;
	/* only 3 antennas are valid for now */
	for (i = 0; i < WL_RSSI_ANT_MAX; i++) {
		if (((WLC_DNGL_OL_RSSI_PHYRXCHAIN(rssi_ol) >> i) & 1) == 1) {
			rxpwr_core[i] = MAX(-128, rxpwr_core[i]);
			rxpwr_activecore[num_activecores] = rxpwr_core[i];
			if (rxpwr_core[i] >= 0) {
				WL_TRACE(("%s: invalid antenna power 0:%d, 1:%d, 2: %d\n",
					__FUNCTION__, rxpwr_core[0], rxpwr_core[1], rxpwr_core[2]));
				return 0;
			}
			/* Average per core rssi over MA_WINDOW_SZ samples */
			rxpwr_core[i] = wlc_dngl_ol_calculate_rssi_ma(rxpwr_core[i],
				rssi_ol->rxpwr_ma_ant[i], rssi_ol->num_samples);
			/* Set value in shared memory */
			WLC_DNGL_OL_RSSI_RXPWR_SET(rssi_ol->wlc_dngl_ol, rxpwr_core[i], i);
			num_activecores++;
		}
	}

	if (num_activecores == 2) {
		rxpwr_core[2] = (rxpwr_core[0] + rxpwr_core[1])/2;
		WLC_DNGL_OL_RSSI_RXPWR_SET(rxpwr_core[2], 2);
	}

	if (num_activecores == 0) {
		WL_ERROR(("%s: No active core?\n", __FUNCTION__));
		ASSERT(0);
	}

	/*
	 * mode = 0: rxpwr = max(rxpwr0, rxpwr1)
	 * mode = 1: rxpwr = min(rxpwr0, rxpwr1)
	 * mode = 2: rxpwr = (rxpwr0+rxpwr1)/2
	 */
	rxpwr = rxpwr_activecore[0];
	for (i = 1; i < num_activecores; i ++) {
		if (WLC_DNGL_OL_RSSI_MODE(rssi_ol) == RSSI_ANT_MERGE_MAX) {
			rxpwr = MAX(rxpwr, rxpwr_activecore[i]);
		} else if (WLC_DNGL_OL_RSSI_MODE(rssi_ol) == RSSI_ANT_MERGE_MIN) {
			rxpwr = MIN(rxpwr, rxpwr_activecore[i]);
		} else if (WLC_DNGL_OL_RSSI_MODE(rssi_ol) == RSSI_ANT_MERGE_AVG) {
			rxpwr += rxpwr_activecore[i];
		} else {
			WL_ERROR(("%s: Invalid mode\n", __FUNCTION__));
			ASSERT(0);
		}
	}

	if (WLC_DNGL_OL_RSSI_MODE(rssi_ol) == RSSI_ANT_MERGE_AVG) {
		int16 qrxpwr = 0;
		rxpwr = (int8)qm_div16(rxpwr, num_activecores, &qrxpwr);
	}

	WL_TRACE(("%s: Instanteneous RSSI %d\n", __FUNCTION__, rxpwr));

	if (rxpwr >= 0)
		return 0;

	rxpwr = wlc_dngl_ol_calculate_rssi_ma(rxpwr, rssi_ol->rxpwr_ma, rssi_ol->num_samples);
	rssi_ol->num_samples =
		(rssi_ol->num_samples == ~((typeof(rssi_ol->num_samples))0)) ?
		MA_WINDOW_SZ : (rssi_ol->num_samples+1);
	WLC_DNGL_OL_RSSI_SET(rssi_ol->wlc_dngl_ol, rxpwr); /* Set value in shared memory */

	if ((rssi_ol->wlc_dngl_ol->wowl_cfg.wowl_enabled == FALSE) &&
	    ((RXOESHARED(rssi_ol->wlc_dngl_ol)->dngl_watchdog_cntr -
		WLC_DNGL_OL_LOW_RSSI_THRESHOLD_TIME(rssi_ol)) > WLC_DNGL_OL_LOW_RSSI_DELTA_TIME) &&
	    (WLC_DNGL_OL_LOW_RSSI_THRESHOLD(rssi_ol) > rxpwr) &&
	    (WLC_DNGL_OL_RSSI_NUM_MA_SAMPLES(rssi_ol) > MA_WINDOW_SZ) &&
	    !WLC_DNGL_OL_LOW_RSSI_ROAM_OFF(rssi_ol)) {

		WL_ERROR(("SEND_NOTIFICATION: %s RSSI %d is lower than threshold %d\n",
			__FUNCTION__, rxpwr, WLC_DNGL_OL_LOW_RSSI_THRESHOLD(rssi_ol)));
		WLC_DNGL_OL_LOW_RSSI_THRESHOLD_TIME_SET(rssi_ol,
			RXOESHARED(rssi_ol->wlc_dngl_ol)->dngl_watchdog_cntr);

	}

	WL_TRACE(("%s: ============= END DUMPING VALUES ===============\n", __FUNCTION__));
	WL_TRACE(("%s: exiting with RSSI %d\n", __FUNCTION__, rxpwr));

	return rxpwr;
}
