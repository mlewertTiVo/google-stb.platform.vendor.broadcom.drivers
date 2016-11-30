/*
 * Code that controls the link quality
 * Broadcom 802.11bang Networking Device Driver
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
 * $Id: wlc_lq.h 645549 2016-06-24 14:53:48Z $
 */

#ifndef _wlc_lq_h_
#define _wlc_lq_h_

/* rssi constants */
/* #define WLC_RSSI_INVALID	0 - defined in wlioctl */
#define	WLC_RSSI_EXCELLENT	-57	/**< Excellent quality cutoffs */

/* snr constants */
#define WLC_SNR_INVALID		0	/**< invalid SNR value */
#define WLC_SNR_EXCELLENT	25
#define WLC_SNR_EXCELLENT_11AC	35

/* noise constants */
#define WLC_NOISE_INVALID	0
#define	WLC_NOISE_EXCELLENT	-91

/* attach/detach */

extern wlc_lq_info_t *wlc_lq_attach(wlc_info_t* wlc);
extern void wlc_lq_detach(wlc_lq_info_t* lqi);

/* ******** chanim ********* */

#ifdef WLCHANIM

bool wlc_lq_chanim_act(chanim_info_t *c_info);
bool wlc_lq_chanim_mode_act(chanim_info_t *c_info);
bool wlc_lq_chanim_mode_ext(chanim_info_t *c_info);
bool wlc_lq_chanim_mode_detect(chanim_info_t *c_info);

#define WLC_CHANIM_ACT(c_info)		wlc_lq_chanim_act(c_info)
#define WLC_CHANIM_MODE_ACT(c_info)	wlc_lq_chanim_mode_act(c_info)
#define WLC_CHANIM_MODE_EXT(c_info)	wlc_lq_chanim_mode_ext(c_info)
#define WLC_CHANIM_MODE_DETECT(c_info)	wlc_lq_chanim_mode_detect(c_info)

#else /* WLCHANIM */
#define WLC_CHANIM_ACT(a)		0	/* NO WLCHANIM support */
#define WLC_CHANIM_MODE_ACT(a)		0
#define WLC_CHANIM_MODE_EXT(a)		0
#define WLC_CHANIM_MODE_DETECT(a)	0
#endif /* WLCHANIM */

/*
 * For multi-channel (VSDB), chanim supports 2 interfaces.
 */
#define CHANIM_NUM_INTERFACES_MCHAN		2
#define CHANIM_NUM_INTERFACES_SINGLECHAN	1

/* wlc_chanim_update flags */
#define CHANIM_WD		0x1 /* on watchdog */
#define CHANIM_CHANSPEC		0x2 /* on chanspec switch */

#ifdef WLCHANIM
extern int wlc_lq_chanim_update(wlc_info_t *wlc, chanspec_t chanspec, uint32 flags);
extern void wlc_lq_chanim_acc_reset(wlc_info_t *wlc);
extern bool wlc_lq_chanim_interfered(wlc_info_t *wlc, chanspec_t chanspec);
extern void wlc_lq_chanim_upd_act(wlc_info_t *wlc);
extern void wlc_lq_chanim_upd_acs_record(chanim_info_t *c_info, chanspec_t home_chspc,
	chanspec_t selected, uint8 trigger);
extern void wlc_lq_chanim_action(wlc_info_t *wlc);
extern void wlc_lq_chanim_create_bss_chan_context(wlc_info_t *wlc, chanspec_t chanspec,
	chanspec_t prev_chanspec);
extern void wlc_lq_chanim_delete_bss_chan_context(wlc_info_t *wlc, chanspec_t chanspec);
extern int wlc_lq_chanim_adopt_bss_chan_context(wlc_info_t *wlc, chanspec_t chanspec,
	chanspec_t prev_chanspec);
extern bool wlc_lq_chanim_stats_get(chanim_info_t *c_info, chanspec_t chanspec,
	chanim_stats_t *stats);
extern chanim_stats_t* wlc_lq_chanspec_to_chanim_stats(chanim_info_t *c_info, chanspec_t);
#else
#define wlc_lq_chanim_acc_reset(a)	do {} while (0)
#define wlc_lq_chanim_interfered(a, b)	0
#define wlc_lq_chanim_upd_act(a)	do {} while (0)
#define wlc_lq_chanim_upd_acs_record(a, b, c, d) do {} while (0)
#define wlc_lq_chanim_action(a)		do {} while (0)
#define wlc_lq_chanim_create_bss_chan_context(a, b, c)	do {} while (0)
#define wlc_lq_chanim_delete_bss_chan_context(a, b)	do {} while (0)
#define wlc_lq_chanim_adopt_bss_chan_context(a, b, c)	BCME_OK
#define wlc_lq_chanim_stats_get(a, b, c) FALSE
#endif /* WLCHANIM */


/* ******** per scb rssi/snr & moving average ********* */

/* Test whether RSSI update is enabled. Made a macro to reduce fn call overhead. */
#define WLC_RX_LQ_SAMP_ENAB(scb) ((scb)->rx_lq_samp_req != 0)

/* compute rssi/snr */
#define wlc_lq_recv_rssi_compute(wlc, wrxh) (int8)(wrxh)->rssi
uint8 wlc_lq_recv_snr_compute(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh, int8 noise);

/* request to reset moving average window */
void wlc_lq_rssi_ma_reset(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
	int8 rssi);
void wlc_lq_rssi_bss_sta_ma_reset(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	int band, int8 rssi);
void wlc_lq_rssi_snr_noise_ma_reset(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
	int8 rssi, uint8 snr, int8 noise);
void wlc_lq_rssi_snr_noise_bss_sta_ma_reset(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	int band, int8 rssi, uint8 snr, int8 noise);

/* request to update moving average */
int8 wlc_lq_rssi_ma_upd(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
	int8 rssi, uint8 qdb, bool ucast);
int8 wlc_lq_rssi_bss_sta_ma_upd(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	int band, int8 rssi, uint8 qdb, bool ucast);
uint8 wlc_lq_snr_ma_upd(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
	uint8 snr, bool ucast);
int8 wlc_lq_noise_ma_upd(wlc_info_t *wlc, int8 noise);
int8 wlc_lq_noise_lte_ma_upd(wlc_info_t *wlc, int8 noise);

/* request to check rssi level and send host indication if necessary */
void wlc_lq_rssi_bss_sta_event_upd(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

/* bit position, max. 8 bits */
typedef enum {
	RX_LQ_SAMP_REQ_WLC = 0,		/* Driver IOVARs */
	RX_LQ_SAMP_REQ_TM = 1,		/* Traffic Management */
	RX_LQ_SAMP_REQ_RATE_SEL = 2,	/* Rate Selection */
	RX_LQ_SAMP_REQ_BSS_STA = 3,	/* Infra STA */
	RX_LQ_SAMP_REQ_IBSS_STA = 4,    /* For all IBSS like(TDLS, MESH, AWDL) modes */
	RX_LQ_SAMP_REQ_RMC = 5
} rx_lq_samp_req_t;

typedef enum wlc_pkt_type {
	PKT_TYPE_DATA = 1,
	PKT_TYPE_BEACON = 2
} wlc_rx_pkt_type_t;


/* enable rx link quality sampling request */
void wlc_lq_sample_req_enab(struct scb *scb, rx_lq_samp_req_t req, bool enab);

/* add sample */
typedef struct {
	int8 rssi;		/* raw rssi reported in wrxh */
	int8 rssi_avg;		/* move average, valid for Infra STA only. */
	uint8 snr;		/* snr based on raw rssi */
	uint8 snr_avg;		/* move average, valid for Infra STA only. */
} rx_lq_samp_t;

void wlc_lq_sample(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
	wlc_d11rxhdr_t *wrxh, bool ucast, rx_lq_samp_t *samp);
void wlc_lq_bss_sta_sample(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	wlc_d11rxhdr_t *wrxh, int band, bool ucast, rx_lq_samp_t *samp);

/* rssi average */
int8 wlc_lq_rssi_get(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb);
/* rssi of last received packet */
int8 wlc_lq_rssi_last_get(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb);
/* rssi average at the given antenna chain */
int8 wlc_lq_ant_rssi_get(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb, int chain);
/* rssi of last received packet at the given antenna chain */
int8 wlc_lq_ant_rssi_last_get(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb, int chain);


/* ******** others ********* */


typedef int (*stats_cb)(wlc_info_t *wlc, void *ctx, uint32 elapsed_time, void *vstats);

/* register a callbk [cb] to return wlc_bmac_obss_counts_t stats after
* req_time millisecs
*/
int wlc_lq_register_dynbw_stats_cb(wlc_info_t *wlc, uint32 req_time_ms, stats_cb cb,
	uint16 connID, void *arg);

/* move WLC_NOISE_REQUEST_xxx  */
#define WLC_NOISE_REQUEST_SCAN	0x1
#define WLC_NOISE_REQUEST_CQ	0x2
#define WLC_NOISE_REQUEST_RM	0x4

void wlc_lq_noise_sample_request(wlc_info_t *wlc, uint8 request, uint8 channel);
void wlc_lq_noise_cb(wlc_info_t *wlc, uint8 channel, int8 noise_dbm);

void wlc_lq_channel_qa_sample_req(wlc_info_t *wlc);
uint32 wlc_rsdb_get_lq_load(wlc_info_t *wlc);


/* ******** WORK-IN-PROGRESS ******** */

/* TODO: make these data structures opaque if possible */

/* per bsscfg link qual info */
struct wlc_link_qual {
	int8	rssi;		/**< RSSI moving average */
	uint8	rssi_qdb;	/**< RSSI qdb portion moving average */
	uint8	snr;		/**< SNR moving average */
};

/* ******** WORK-IN-PROGRESS ******** */

#endif	/* _wlc_lq_h_ */
