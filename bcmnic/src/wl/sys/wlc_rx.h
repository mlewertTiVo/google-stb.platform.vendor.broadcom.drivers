/*
 * wlc_rx.h
 *
 * Common headers for receive datapath components
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
 * $Id: wlc_rx.h 643373 2016-06-14 12:34:01Z $
 *
 */
#ifndef _wlc_rx_c
#define _wlc_rx_c

typedef enum wlc_rx_ststus
{
	WLC_RX_STS_OK                       = 0,
	WLC_RX_STS_TOSS_NON_AWDL            = 1,
	WLC_RX_STS_TOSS_NOT_IN_BSS          = 2,
	WLC_RX_STS_TOSS_AP_BSSCFG           = 3,
	WLC_RX_STS_TOSS_MAC_ADDR_RACE       = 4,
	WLC_RX_STS_TOSS_NON_WDS_NON_BSS     = 5,
	WLC_RX_STS_TOSS_AMSDU_DISABLED      = 6,
	WLC_RX_STS_TOSS_DROP_FRAME          = 7,
	WLC_RX_STS_TOSS_NO_SCB              = 8,
	WLC_RX_STS_TOSS_BAD_DS              = 9,
	WLC_RX_STS_TOSS_MCAST_AP            = 10,
	WLC_RX_STS_TOSS_INV_MCS             = 11,
	WLC_RX_STS_TOSS_BUF_FAIL            = 12,
	WLC_RX_STS_TOSS_RXS_FCSERR          = 13,
	WLC_RX_STS_TOSS_RUNT_FRAME          = 14,
	WLC_RX_STS_TOSS_BAD_PROTO           = 15,
	WLC_RX_STS_TOSS_RXFRAG_ERR          = 16,
	WLC_RX_STS_TOSS_SAFE_MODE_AMSDU     = 17,
	WLC_RX_STS_TOSS_PKT_LEN_SHORT       = 18,
	WLC_RX_STS_TOSS_8021X_FRAME         = 19,
	WLC_RX_STS_TOSS_EAPOL_SUP           = 20,
	WLC_RX_STS_TOSS_EAPOL_AUTH          = 21,
	WLC_RX_STS_TOSS_INV_SRC_MAC         = 22,
	WLC_RX_STS_TOSS_FRAG_NOT_ALLOWED    = 23,
	WLC_RX_STS_TOSS_BAD_DEAGG_LEN       = 24,
	WLC_RX_STS_TOSS_DEAGG_UNALIGNED     = 25,
	WLC_RX_STS_TOSS_BAD_DEAGG_SF_LEN    = 26,
	WLC_RX_STS_TOSS_PROMISC_OTHER       = 27,
	WLC_RX_STS_TOSS_STA_DWDS            = 28,
	WLC_RX_STS_TOSS_DECRYPT_FAIL        = 29,
	WLC_RX_STS_TOSS_MIC_CHECK_FAIL      = 30,
	WLC_RX_STS_TOSS_NO_ROOM_RX_CTX      = 31,
	WLC_RX_STS_TOSS_3_ADDR_FRAME        = 32,
	WLC_RX_STS_TOSS_AMPDU_DUP			= 33,
	WLC_RX_STS_TOSS_ICV_ERR				= 34,
	WLC_RX_STS_TOSS_UNKNOWN         	= 35,
	WLC_RX_STS_LAST                     = 36
} wlc_rx_status_t;

#define RX_STS_REASON_STRINGS \
	"OK",                        /* WLC_RX_STS_OK                   = 0 */ \
	"non_AWDL_pkt_on_AWDL_link",	/* WLC_RX_STS_TOSS_RX_NON_AWDL     = 1 */ \
	"promisc_mode_not_in_bss",	/* WLC_RX_STS_TOSS_RX_NOT_IN_BSS   = 2 */ \
	"promisc_mode_AP_config",	/* WLC_RX_STS_TOSS_RX_AP_BSSCFG;   = 3 */ \
	"mac_addr_not_updated",      /* WLC_RX_STS_TOSS_RX_MAC_ADDR_RACE= 4 */ \
	"non_wds_frame_not_in_bss",	/* WLC_RX_STS_TOSS_RX_NON_WDS_NON_BSS   = 5 */ \
	"AMSDU_disabled",            /* WLC_RX_STS_TOSS_RX_AMSDU_DISABLED    = 6 */ \
	"drop_frame",                /* WLC_RX_STS_TOSS_RX_DROP_FRAME   = 7 */ \
	"no_scb",                    /* WLC_RX_STS_TOSS_RX_NO_SCB       = 8 */ \
	"bad_ds_field",              /* WLC_RX_STS_TOSS_RX_BAD_DS      = 9, */ \
	"mcast_in_AP_mode",          /* WLC_RX_STS_TOSS_RX_MCAST_AP   = 10, */ \
	"invalid_mcs",               /* WLC_RX_STS_TOSS_RX_INV_MCS    = 11, */ \
	"buf_alloc_fail",            /* WLC_RX_STS_TOSS_RX_BUF_FAIL   = 12, */ \
	"pkt_with_bad_fcs",          /* WLC_RX_STS_TOSS_RX_RXS_FCSERR = 13, */ \
	"runt_frame",                /* WLC_RX_STS_TOSS_RX_RUNT_FRAME = 14, */ \
	"pkt_type_invalid",          /* WLC_RX_STS_TOSS_RX_BAD_PROTO  = 15, */ \
	"missed_frags_error",        /* WLC_RX_STS_TOSS_RX_RXFRAG_ERR = 16, */ \
	"in_safe_mode_an_amsdu",     /* WLC_RX_STS_TOSS_RX_SAFE_MODE_AMSDU, */ \
	"packet_length_too_short",	/* WLC_RX_STS_TOSS_RX_PKT_LEN_SHORT  , */ \
	"non_8021x_frame",           /* WLC_RX_STS_TOSS_RX_8021X_FRAME    , */ \
	"eapol_supplicant",          /* WLC_RX_STS_TOSS_RX_EAPOL_SUP      , */ \
	"eapol_authunticator",       /* WLC_RX_STS_TOSS_RX_EAPOL_AUTH     , */ \
	"invalid_src_mac",           /* WLC_RX_STS_TOSS_RX_INV_SRC_MAC    , */ \
	"fragments_not_allowed",     /* WLC_RX_STS_TOSS_RX_FRAG_NOT_ALLOWED */ \
	"bad_deagg_len",             /* WLC_RX_STS_TOSS_RX_BAD_DEAGG_LEN  , */ \
	"deagg_sf_body_unaligned",	/* WLC_RX_STS_TOSS_RX_DEAGG_UNALIGNED  */ \
	"deagg_sflen_mismatch",      /* WLC_RX_STS_TOSS_RX_BAD_DEAGG_SF_LEN */ \
	"promisc_pkt_other",         /* WLC_RX_STS_TOSS_RX_PROMISC_OTHER    */ \
	"sta_bsscfg_and_dwds",       /* WLC_RX_STS_TOSS_RX_STA_DWDS         */ \
	"decrypt_fail",              /* WLC_RX_STS_TOSS_RX_DECRYPT_FAIL     */ \
	"mic_check_fail",            /* WLC_RX_STS_TOSS_RX_MIC_CHECK_FAIL   */ \
	"no_room_for_Rx_ctx",        /* WLC_RX_STS_TOSS_RX_NO_ROOM_RX_CTX   */ \
	"toss_3_addr_frame",         /* WLC_RX_STS_TOSS_RX_3_ADDR_FRAME     */ \
	"ampdu_seq_invalid",         /* WLC_RX_STS_TOSS_AMPDU_DUP           */ \
	"icv_error",                 /* WLC_RX_STS_TOSS_ICV_ERR             */ \
	"Unknown_toss_reason"           /* WLC_RX_STS_TOSS_UNKNOWN        = 35 */
#define RX_STS_RSN_MAX	(WLC_RX_STS_LAST)


#ifdef WL_RX_STALL

#ifndef RX_HC_FRAMECNT
#define RX_HC_FRAMECNT 100
#endif

#ifndef RX_HC_TIMES_HISTORY
#define RX_HC_TIMES_HISTORY 0
#endif /* RX_HC_TIMES_HISTORY */

#ifndef RX_HC_ALERT_THRESHOLD
#define RX_HC_ALERT_THRESHOLD 90
#endif

#ifndef RX_HC_STALL_CNT
#define RX_HC_STALL_CNT 9
#endif

typedef struct wlc_rx_hc_err_info {
	uint32 sum;
	uint32 dropped;
	uint32 fail_rate;
	uint32 ac;
	uint32 stall_bitmap0;
	uint32 stall_bitmap1;
	uint32 threshold;
	uint32 sample_size;
	char  prefix[ETHER_ADDR_STR_LEN+1];
} wlc_rx_hc_err_info_t;

typedef struct wlc_rx_hc_counters {
	uint32  rx_pkts[AC_COUNT][WLC_RX_STS_LAST];
	uint32  dropped_all[AC_COUNT];
	uint32  ts[AC_COUNT];
	uint32  stall_cnt[AC_COUNT];
} wlc_rx_hc_counters_t;

struct wlc_rx_hc {
	wlc_info_t * wlc;
	int scb_handle;
	int cfg_handle;
	wlc_rx_hc_counters_t counters;
	uint32  rx_hc_alert_th;
	uint32  rx_hc_cnt;
#if RX_HC_TIMES_HISTORY > 0
	struct	{
		uint32  sum;
		uint32	start_ts;
		uint32	end_ts;
		uint32	dropped;
		uint32  ac;
		char  prefix[ETHER_ADDR_STR_LEN];
		int	reason;
	} entry[RX_HC_TIMES_HISTORY];
	int curr_index;
#endif
	wlc_rx_hc_err_info_t error;
};
#endif /* WL_RX_STALL */


extern uint16* wlc_get_mrxs(wlc_info_t *wlc, d11rxhdr_t *rxh);

void BCMFASTPATH
wlc_recv(wlc_info_t *wlc, void *p);

void BCMFASTPATH
wlc_recvdata(wlc_info_t *wlc, osl_t *osh, wlc_d11rxhdr_t *wrxh, void *p);

void BCMFASTPATH
wlc_recvdata_ordered(wlc_info_t *wlc, struct scb *scb, struct wlc_frminfo *f);

void BCMFASTPATH
wlc_recvdata_sendup(wlc_info_t *wlc, struct scb *scb, bool wds, struct ether_addr *da, void *p);

void BCMFASTPATH
wlc_sendup(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb, void *pkt);

void wlc_sendup_event(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb, void *pkt);

extern void wlc_tsf_adopt_bcn(wlc_bsscfg_t *cfg, wlc_d11rxhdr_t *wrxh, uint8 *plcp,
	struct dot11_bcn_prb *bcn);

extern void
wlc_bcn_ts_tsf_calc(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh,
	void *plcp, uint32 *tsf_h, uint32 *tsf_l);

extern void wlc_bcn_tsf_diff(wlc_bsscfg_t *cfg, wlc_d11rxhdr_t *wrxh, void *plcp,
	struct dot11_bcn_prb *bcn, int32 *diff_h, int32 *diff_l);
extern int wlc_bcn_tsf_later(wlc_bsscfg_t *cfg, wlc_d11rxhdr_t *wrxh, void *plcp,
	struct dot11_bcn_prb *bcn);
int wlc_arq_pre_parse_frame(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint16 ft,
	uint8 *ies, uint ies_len, wlc_pre_parse_frame_t *ppf);
int wlc_scan_pre_parse_frame(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint16 ft,
	uint8 *ies, uint ies_len, wlc_pre_parse_frame_t *ppf);

#ifdef BCMSPLITRX
extern void wlc_pktfetch_get_scb(wlc_info_t *wlc, wlc_frminfo_t *f,
        wlc_bsscfg_t **bsscfg, struct scb **scb, bool promisc_frame, uint32 ctx_assoctime);
#endif

#if defined(PKTC) || defined(PKTC_DONGLE) || defined(PKTC_TX_DONGLE)
extern bool wlc_rxframe_chainable(wlc_info_t *wlc, void **pp, uint16 index);
extern void wlc_sendup_chain(wlc_info_t *wlc, void *p);
#endif /* PKTC || PKTC_DONGLE */

uint16 wlc_recv_mgmt_rx_channel_get(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh);
extern int wlc_process_eapol_frame(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
		struct scb *scb, struct wlc_frminfo *f, void *pkt);

#ifdef WL_RX_STALL
wlc_rx_hc_t *wlc_rx_hc_attach(wlc_info_t * wlc);
void wlc_rx_hc_detach(wlc_rx_hc_t * rx_hc);
int wlc_rx_healthcheck_update_counters(wlc_info_t * wlc, int ac, scb_t * scb,
	wlc_bsscfg_t * bsscfg, int rx_status, int count);
int wlc_rx_healthcheck_verify(wlc_info_t *wlc,
	wlc_rx_hc_counters_t *counters, int ac, const char * prefix);
void wlc_rx_healthcheck_report(wlc_info_t *wlc);
int wlc_rx_healthcheck(uint8* buffer_ptr, uint16 remaining_len,
	void* context, uint16* bytes_written);
const char *wlc_rx_dropped_get_name(int reason);
void wlc_rx_healthcheck_force_fail(wlc_info_t *wlc);
#endif /* WL_RX_STALL */


#endif /* _wlc_rx_c */
