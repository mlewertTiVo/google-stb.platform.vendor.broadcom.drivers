/*
 * Common (OS-independent) definitions for
 * Broadcom 802.11abg Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc.h 646952 2016-07-01 09:26:00Z $
 */

#ifndef _wlc_h_
#define _wlc_h_

#include <wlc_types.h>
#include "wlc_cfg.h"
#include "wlc_pub.h"

#include <wl_dbg.h>
#include <wlioctl.h>
#ifdef WLC_HIGH
#include <wlc_event.h>
#endif
#include <wlc_pio.h>
#include <wlc_phy_hal.h>
#include <wlc_channel.h>
#ifdef TRAFFIC_MGMT
#include <wlc_traffic_mgmt.h>
#endif
#ifdef WLC_SPLIT
#include <bcm_rpc.h>
#endif
#ifdef NET_DETECT
#include <wlc_net_detect.h>
#endif
#include <wlc_hw.h>
#include <bcm_notif_pub.h>
#include <bcm_mpool_pub.h>

#ifdef BCMSPLITRX
#include <wlc_frmutil.h>
#endif

#ifndef BCM_OL_DEV
#include <wlc_bsscfg.h>
#endif /* !BCM_OL_DEV */


/*
 * defines for join pref processing
 */

#define WLC_JOIN_PREF_LEN_FIXED		2 /* Length for the fixed portion of Join Pref TLV value */

#define JOIN_RSSI_BAND		WLC_BAND_5G	/* WLC_BAND_AUTO disables the feature */
#define JOIN_RSSI_DELTA		10		/* Positive value */
#define JOIN_PREF_IOV_LEN	8		/* 4 bytes each for RSSI Delta and mandatory RSSI */

#define MAX_MCHAN_CHANNELS 4

/* Construct the join pref TLV based on rssi and band */
#define PREP_JOIN_PREF_RSSI_DELTA(_pref, _rssi, _band) \
	do {						\
		(_pref)[0] = WL_JOIN_PREF_RSSI_DELTA;	\
		(_pref)[1] = WLC_JOIN_PREF_LEN_FIXED;	\
		(_pref)[2] = _rssi;			\
		(_pref)[3] = _band;			\
	} while (0)

/* Construct the mandatory TLV for RSSI */
#define PREP_JOIN_PREF_RSSI(_pref) \
	do {						\
		(_pref)[0] = WL_JOIN_PREF_RSSI;		\
		(_pref)[1] = WLC_JOIN_PREF_LEN_FIXED;	\
		(_pref)[2] = 0;				\
		(_pref)[3] = 0;				\
	} while (0)


/* Flags to identify SYNC operation vs FLUSH operation required for Nintendo2 and P2P */
#define SYNCFIFO						(0)
#define FLUSHFIFO						(1)

/* Bitmaps */
/* 0x01=BK, 0x02=BE, 0x04=VI, 0x08=VO, 0x10=B/MCAST, 0x20=ATIM */
#define BITMAP_FLUSH_NO_TX_FIFOS		(0)		/* Bitmap - flush no TX FIFOs */
#define BITMAP_FLUSH_ALL_TX_FIFOS		(0x3F)	/* Bitmap - flush all TX FIFOs */
#define BITMAP_SYNC_ALL_TX_FIFOS		(0x3F)	/* Bitmap - sync all TX FIFOs */

#define MA_WINDOW_SZ		8	/* moving average window size */
#define	WL_HWRXOFF		38	/* chip rx buffer offset */
#define	WL_HWRXOFF_AC		40	/* chip rx buffer offset */
#define	INVCHANNEL		255	/* invalid channel */
#define	WLC_MAXFRAG		16	/* max fragments per SDU */
#define WLC_WEP_IV_SIZE		4	/* size of WEP1 and WEP128 IV */
#define WLC_WEP_ICV_SIZE	4	/* size of WEP ICV */
#define WLC_ASSOC_RETRY_MAX	0	/* associate maximum retry number */
#define WLC_ASSOC_RECREATE_BI_TIMEOUT	10 /* bcn intervals to wait during and assoc_recreate */
#define WLC_ASSOC_VERIFY_TIMEOUT	1000 /* ms to wait to allow an AP to respond to class 3
					      * data if it needs to disassoc/deauth
					      */
#define WLC_ADVERTISED_LISTEN	10	/* listen interval specified in (re)assoc request */
#define WLC_FREQTRACK_THRESHOLD	5	/* seconds w/o beacons before we increase the
					 * frequency tracking bandwidth
					 */
#define WLC_FREQTRACK_DETECT_TIME	2	/* seconds in which we must detect
						 * beacons after setting the wide
						 * bandwidth
						 */
#define WLC_FREQTRACK_MIN_ATTEMPTS	3	/* minimum number of times we should
						 * try to get beacons using the wide freq
						 * tracking b/w
						 */
#define WLC_FREQTRACK_TIMEOUT	30	/* give up on wideband frequency tracking after
					 * 30 seconds
					 */
#ifndef WLC_MAXMODULES
#define WLC_MAXMODULES		64	/* max #  wlc_module_register() calls */
#endif

#define WLC_TXQ_DRAIN_TIMEOUT	3000	/* txq drain timeout in ms */

/* network packet filter bit map matching NDIS_PACKET_TYPE_XXX */
#define WLC_ACCEPT_DIRECTED               0x0001
#define WLC_ACCEPT_MULTICAST              0x0002
#define WLC_ACCEPT_ALL_MULTICAST          0x0004
#define WLC_ACCEPT_BROADCAST              0x0008
#define WLC_ACCEPT_SOURCE_ROUTING         0x0010
#define WLC_ACCEPT_PROMISCUOUS            0x0020
#define WLC_ACCEPT_SMT                    0x0040
#define WLC_ACCEPT_ALL_LOCAL              0x0080

#define WLC_BITSCNT(x)	bcm_bitcount((uint8 *)&(x), sizeof(uint8))


/* freqtrack_override values */
typedef enum _freqtrack_override_t {
	FREQTRACK_AUTO = 0,
	FREQTRACK_ON,
	FREQTRACK_OFF
} freqtrack_override_t;

#define NUM_UCODE_FRAMES        4	/* Num ucode frames for TSSI estimation */

#define WLC_FRAMEBURST_MIN_RATE WLC_RATE_2M	/* frameburst mode is only enabled above 2Mbps */

#define WLC_CHANNEL_QA_NSAMP	2	/* size of channel_qa_sample array */
#define WLC_RM_RPI_INTERVAL	20	/* ms, time between RPI/Noise samples */

#define	WLC_RATEPROBE_RATE	(6 * 2)	/* 6Mbps ofdm rateprobe rate */

/* Maximum wait time for a MAC suspend */
#define	WLC_MAX_MAC_SUSPEND	83000	/* uS: 83mS is max packet time (64KB ampdu @ 6Mbps) */

/* Probe Response timeout - responses for probe requests older that this are tossed, zero to disable
 */
#define WLC_PRB_RESP_TIMEOUT	0 /* Disable probe response timeout */

#define WLC_VLAN_TAG_LEN	4	/* Duplicated to avoid including <proto/vlan.h> */

#define CCX_HEADROOM 0

#define TKIP_TAILROOM MAX(TKIP_MIC_SIZE, TKIP_EOM_SIZE) /* Tailroom needed for TKIP */

/* TKIP and CKIP are mutually exclusive so account only for max */
#define TXTAIL TKIP_TAILROOM

/* Tcp segementation offload */
#if D11CONF_GE(40)
/* only available on D11 core rev >= 40 */
#define TSO_HEADER_LENGTH 16
#define TSO_HEADER_PASSTHROUGH_LENGTH 4
#else
/* no TSO hw, so don't add to TXOFF */
#define TSO_HEADER_LENGTH 0
#define TSO_HEADER_PASSTHROUGH_LENGTH 0
#endif

/* transmit buffer max headroom for protocol headers */
#define D11_COMMON_HDR	(DOT11_A4_HDR_LEN + DOT11_QOS_LEN + DOT11_IV_MAX_LEN + \
				DOT11_LLC_SNAP_HDR_LEN + WLC_VLAN_TAG_LEN + CCX_HEADROOM)
#define D11_TXOFF	(D11_TXH_LEN + D11_PHY_HDR_LEN + D11_COMMON_HDR)
#define D11AC_TXOFF	(D11AC_TXH_LEN + D11_COMMON_HDR + TSO_HEADER_LENGTH)

#define D11_RPC_COMMON_HDR	(2 /* WLC_RPC_TXFIFO_UNALIGN_PAD_2BYTE */ + 4 /* RPC_HDR_LEN */ + \
					4 /* BCM_RPC_TP_ENCAP_LEN */)
#define D11_RPC_TXOFF	(((D11_TXOFF + D11_RPC_COMMON_HDR) + 3) & ~3)
#define D11AC_RPC_TXOFF	(((D11AC_TXOFF + D11_RPC_COMMON_HDR) + 3) & ~3)

#if defined(WLC_HIGH_ONLY) && (defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_TXNOCOPY) || \
	defined(BCM_RPC_TOC))
#if D11CONF_LT(40) && !D11CONF_GE(40)
#define TXOFF	D11_RPC_TXOFF
#else
#define TXOFF	MAX(D11AC_RPC_TXOFF, D11_RPC_TXOFF)
#endif /* D11CONF_LT(40) && !D11CONF_GE(40) */
#else
#if D11CONF_LT(40) && !D11CONF_GE(40)
#define TXOFF	D11_TXOFF
#else
#define TXOFF	MAX(D11AC_TXOFF, D11_TXOFF)
#endif /* D11CONF_LT(40) && !D11CONF_GE(40) */
#endif /* WLC_HIGH_ONLY || (BCM_RPC_NOCOPY) || defined(BCM_RPC_TXNOCOPY) || defined(BCM_RPC_TOC)) */

#if defined(WLC_HIGH) || defined(SCANOL)
/* For managing scan result lists */
typedef struct wlc_bss_list {
	uint		count;
	bool		beacon;		/* set for beacon, cleared for probe response */
	wlc_bss_info_t*	ptrs[MAXBSS];
} wlc_bss_list_t;
#endif /* WLC_HIGH || SCANOL */

#define ISCAN_SUCCESS(wlc)  \
		(wlc->custom_iscan_results_state == WL_SCAN_RESULTS_SUCCESS)
#define ISCAN_PARTIAL(wlc)  \
		(wlc->custom_iscan_results_state == WL_SCAN_RESULTS_PARTIAL)
#define ISCAN_IN_PROGRESS(wlc)  \
		(wlc->custom_iscan_results_state == WL_SCAN_RESULTS_PENDING)
#define ISCAN_ABORTED(wlc)  \
		(wlc->custom_iscan_results_state == WL_SCAN_RESULTS_ABORTED)
#define ISCAN_RESULTS_OK(wlc) (ISCAN_SUCCESS(wlc) || ISCAN_PARTIAL(wlc))

#define	SW_TIMER_MAC_STAT_UPD		30	/* periodic MAC stats update */

#define CRSGLITCH_THRESHOLD_DEFAULT	4000 /* normalized per second count */
#define CCASTATS_THRESHOLD_DEFAULT	40 /* normalized percent stats 0 - 255 */
#define BGNOISE_THRESHOLD_DEFAULT 	-55 /* in dBm */
#define SAMPLE_PERIOD_DEFAULT		1 /* in second */
#define THRESHOLD_TIME_DEFAULT		2 /* number of sample periods */
#define LOCKOUT_PERIOD_DEFAULT		120 /* in second */
#define MAX_ACS_DEFAULT				5 /* number of ACS in a lockout period */
#define CHANIM_SCB_MAX_PROBE 20

#define SAMPLE_PERIOD_MIN		1
#define THRESHOLD_TIME_MIN		1

/* index for accumulative count */
#define CHANIM_HOME_CHAN			0
#define CHANIM_OFF_CHAN 			1
#define CHANIM_ACCUM_CNT			2

/* wlc_chanim_update flags */
#define CHANIM_WD			0x1 /* on watchdog */
#define CHANIM_CHANSPEC 	0x2 /* on chanspec switch */

/* chanim flag defines */
#define CHANIM_DETECTED	0x1 	/* interference detected */
#define CHANIM_ACTON		0x2		/* ACT triggered */

#define WLC_CCASTATS_CAP(wlc)  D11REV_GE(((wlc_info_t*)(wlc))->pub->corerev, 15)

#ifdef WLCHANIM
#define WLC_CHANIM_ENAB(wlc)	1	/* WLCHANIM support, for run time control */
#define WLC_CHANIM_ACT(c_info)		(chanim_mark(c_info).flags & CHANIM_ACTON)
#define WLC_CHANIM_MODE_ACT(c_info) (chanim_config(c_info).mode >= CHANIM_ACT)
#define WLC_CHANIM_MODE_EXT(c_info) (chanim_config(c_info).mode == CHANIM_EXT)
#define WLC_CHANIM_MODE_DETECT(c_info) (chanim_config(c_info).mode >= CHANIM_DETECT)
#define WLC_CHANIM_LOCKOUT(c_info) \
	(chanim_mark(c_info).flags & CHANIM_LOCKOUT)
#else /* WLCHANIM */
#define WLC_CHANIM_ENAB(a) 		0	/* NO WLCHANIM support */
#define WLC_CHANIM_ACT(a)		0	/* NO WLCHANIM support */
#define WLC_CHANIM_MODE_ACT(a) 0
#define WLC_CHANIM_MODE_EXT(a) 0
#define WLC_CHANIM_MODE_DETECT(a) 0
#define WLC_CHANIM_LOCKOUT(a) 0
#endif /* WLCHANIM */

#ifdef WLPKTDLYSTAT_IND
#define TXDELAY_SAMPLE_PERIOD	1	/* Txdelay sample period, in sec */
#define TXDELAY_SAMPLE_CNT		1	/* Txdelay sample count number */
#define TXDELAY_RATIO			30	/* Avg Txdelay ratio */
#endif /* WLPKTDLYSTAT_IND */

#ifndef INT32_MAX
#define INT32_MAX		(2147483647)
#endif /* INT32_MAX */

typedef struct iscan_ignore {
	uint8 bssid[ETHER_ADDR_LEN];
	uint16 ssid_sum;
	uint16 ssid_len;
	chanspec_t chanspec;	/* chan portion is chan on which bcn_prb was rx'd */
} iscan_ignore_t;

/* support 100 ignore list items */
#define WLC_ISCAN_IGNORE_LIST_SZ	1200
#define IGNORE_LIST_MATCH_SZ	OFFSETOF(iscan_ignore_t, chanspec)
#define WLC_ISCAN_IGNORE_MAX	(WLC_ISCAN_IGNORE_LIST_SZ / sizeof(iscan_ignore_t))

/* pkt type used to indicate a roaming STA */
#define ROAM_PKT_TYPE	0x806	/* currently arp */
#define ARP_CMD_OFF		6 /* Unused */
#define ARP_RESP		2 /* Unused */

/* wlc_BSSinit actions */
#define WLC_BSS_JOIN		0 /* join action */
#define WLC_BSS_START		1 /* start action */

#define WLC_BSS_CONNECTED(cfg) \
			((cfg)->associated && (!(cfg)->BSS || !ETHER_ISNULLADDR(&(cfg)->BSSID)))

#define WLC_STA_RETRY_MAX_TIME	3600	/* maximum config value for sta_retry_time (1 hour) */

#define WLC_2050_ROAM_TRIGGER	(-70) /* Roam Trigger for 2050 */
#define WLC_2050_ROAM_DELTA	(20)  /* Roam Delta for 2050 */
#define WLC_2053_ROAM_TRIGGER	(-60) /* Roam Trigger for 2053 */
#define WLC_2053_ROAM_DELTA	(15)  /* Roam Delta for 2053 */
#define WLC_2055_ROAM_TRIGGER	(-75) /* Roam Trigger for 2055 */
#define WLC_2055_ROAM_DELTA	(20)  /* Roam Delta for 2055 */
#define WLC_2060WW_ROAM_TRIGGER	(-75) /* Roam Trigger for 2060ww */
#define WLC_2060WW_ROAM_DELTA	(15)  /* Roam Delta for 2060ww */
#define WLC_2G_ROAM_TRIGGER	(-75)	/* Roam trigger for all other radios */
#define WLC_2G_ROAM_DELTA	(20)	/* Roam delta for all other radios */
#define WLC_5G_ROAM_TRIGGER	(-75)	/* Roam trigger for all other radios */
#define WLC_5G_ROAM_DELTA	(15)	/* Roam delta for all other radios */
#define WLC_AUTO_ROAM_TRIGGER	(-75)	/* This value can dynamically change */
#define WLC_AUTO_ROAM_DELTA	(15)	/* Can also change under motion */
#define WLC_NEVER_ROAM_TRIGGER	(-150) /* Avoid Roaming by setting a large value */
#define WLC_NEVER_ROAM_DELTA	(150)  /* Avoid Roaming by setting a large value */

/* ROAM related defines */
#ifdef BCMQT_CPU
#define WLC_BCN_TIMEOUT		40	/* qt is slow */
#else
#define WLC_BCN_TIMEOUT		8	/* seconds w/o bcns until loss of connection */
#endif
#define WLC_ROAM_SCAN_PERIOD	10	/* roaming scan period in seconds */
#define WLC_FULLROAM_PERIOD 	70	/* full roam scan Period */
#define WLC_CACHE_INVALID_TIMER	60	/* entries are no longer valid after this # of secs */
#define WLC_UATBTT_TBTT_THRESH	10	/* beacon loss before checking unaligned tbtt */
#define WLC_ROAM_TBTT_THRESH	20	/* beacon loss before roaming attempt */

#define ROAM_FULLSCAN_NTIMES	3
#define ROAM_RSSITHRASHDIFF	5
#define ROAM_CACHEINVALIDATE_DELTA	7
#define ROAM_CACHEINVALIDATE_DELTA_MAX	0x80	/* Set to force max ci_delta,
						 * and effectively disable co_delta
						 */

/* join pref rssi delta minimum cutoff
 * The rssi delta will only be applied if the target on the preferred band has an RSSI
 * of at least this minimum value. Below this RSSI value, the preferred band target gets
 * no RSSI preference.
 */
#ifndef WLC_JOIN_PREF_RSSI_BOOST_MIN
#define WLC_JOIN_PREF_RSSI_BOOST_MIN	-65	/* Targets must be at least -65dBm to get pref */
#endif

/* roam->type values */
#define ROAM_FULL		0
#define ROAM_PARTIAL		1
#define ROAM_SPLIT_PHASE	2

/* Defaults? */
#define TXMIN_THRESH_DEFAULT	7 /* Roam scan after 7 packets at min tx rate */
#define TXFAIL_THRESH_DEFAULT	7 /* Start looking if we are stuck at the most basic rate */

/* thresholds for consecutive bcns lost roams allowed */
#define ROAM_CONSEC_BCNS_LOST_THRESH	3

/* Motion detection related defines */
#define ROAM_MOTION_TIMEOUT		120	 /* 2 minutes? */
#define MOTION_RSSI_DELTA_DEFAULT	10	 /* in dBm */

/* AP environment */
#define AP_ENV_DETECT_NOT_USED	0 /* We aren't using AP environment detection */
#define AP_ENV_DENSE		1 /* "Corporate" or other AP dense environment */
#define AP_ENV_SPARSE		2 /* "Home" or other sparse environment */
#define AP_ENV_INDETERMINATE	3 /* AP environment hasn't been identified */

/* Roam delta suggested defaults, in dBm */
#define ROAM_DELTA_AGGRESSIVE	10
#define ROAM_DELTA_MODERATE	20
#define ROAM_DELTA_CONSERVATIVE	30
#define ROAM_DELTA_AUTO		15

/* Defaults? */
#define TXMIN_THRESH_DEFAULT	7 /* Roam scan after 7 packets at min tx rate */
#define TXFAIL_THRESH_DEFAULT	7 /* Start looking if we are stuck at the most basic rate */

/* motion detect */
#define MOTION_DETECT_DELTA_MOD	5 /* Drop the delta by 5 dB if we detect motion */
#define MOTION_DETECT_TRIG_MOD	10 /* Increase the trigger by 10 dB if we detect motion */
#define MOTION_DETECT_RSSI	-50 /* Don't activate the motion detection code above -50 dB */

#define ROAM_REASSOC_TIMEOUT	300 /* 5 minutes? */

#define WLC_MIN_CNTRY_ELT_SZ	6	/* Min size for 802.11d Country Info Element. */

#define VALID_COREREV(corerev)	D11CONF_HAS(corerev)

/* values for shortslot_override */
#define WLC_SHORTSLOT_AUTO	-1 /* Driver will manage Shortslot setting */
#define WLC_SHORTSLOT_OFF	0  /* Turn off short slot */
#define WLC_SHORTSLOT_ON	1  /* Turn on short slot */

/* value for short/long and mixmode/greenfield preamble */

#define WLC_LONG_PREAMBLE	(0)
#define WLC_SHORT_PREAMBLE	(1 << 0)
#define WLC_GF_PREAMBLE		(1 << 1)
#define WLC_MM_PREAMBLE		(1 << 2)
#define WLC_IS_MIMO_PREAMBLE(_pre) (((_pre) == WLC_GF_PREAMBLE) || ((_pre) == WLC_MM_PREAMBLE))

/* values for barker_preamble */
#define WLC_BARKER_SHORT_ALLOWED	0 /* Short pre-amble allowed */
#define WLC_BARKER_LONG_ONLY		1 /* No short pre-amble allowed */

#define	WLC_IBSS_BCN_TIMEOUT	4 /* Timeout to indicate that various types of IBSS beacons have
				   * gone away
				   */

/* assoc->state values */
#define AS_IDLE			0	/* Idle state */
#define AS_JOIN_INIT		1
#define AS_SCAN			2	/* Scan state */
#define AS_JOIN_START		3	/* Join start state */
#define AS_WAIT_IE		4	/* waiting for association ie */
#define AS_WAIT_IE_TIMEOUT	5	/* TimeOut waiting for assoc ie */
#define AS_WAIT_TX_DRAIN	6	/* Waiting for tx packets to drain out */
#define AS_WAIT_TX_DRAIN_TIMEOUT 7	/* TimeOut waiting for tx drain out */
#define AS_SENT_AUTH_1		8	/* Sent Authentication 1 state */
#define AS_SENT_AUTH_2		9	/* Sent Authentication 2 state */
#define AS_SENT_AUTH_3		10	/* Sent Authentication 3 state */
#define AS_SENT_ASSOC		11	/* Sent Association state */
#define AS_REASSOC_RETRY	12	/* Sent Assoc on DOT11_SC_REASSOC_FAIL */
#define AS_JOIN_ADOPT		13	/* Adopting BSS Params */
#define AS_WAIT_RCV_BCN		14	/* Waiting to receive beacon state */
#define AS_SYNC_RCV_BCN		15	/* Waiting Re-sync beacon state */
#define AS_WAIT_DISASSOC	16	/* Waiting to disassociate state */
#define AS_WAIT_PASSHASH	17	/* calculating passhash state */
#define AS_RECREATE_WAIT_RCV_BCN 18	/* wait for former BSS/IBSS bcn in assoc_recreate */
#define AS_ASSOC_VERIFY		19	/* allow former AP time to disassoc/deauth before PS mode */
#define AS_LOSS_ASSOC		20	/* Loss association */
#define AS_JOIN_CACHE_DELAY	21
#define AS_WAIT_FOR_AP_CSA	22	/* Wait for AP to send CSA to clients */
#define AS_WAIT_FOR_AP_CSA_ROAM_FAIL 23	/* Wait for AP to send CSA to clients when roam fails */
#define AS_SENT_FTREQ		24	/* Sent FT Req state */
#define AS_MODE_SWITCH_START 25
#define AS_MODE_SWITCH_COMPLETE 26
#define AS_MODE_SWITCH_FAILED 27
#define AS_DISASSOC_TIMEOUT 28
#define AS_IBSS_CREATE		29	/* No IBSS peers found, STA will create IBSS */
#define AS_DFS_CAC_START	30	/* DFS Slave CAC begins */
#define AS_DFS_CAC_FAIL		31	/* DFS Slave detected radar during CAC state */
#define AS_DFS_ISM_INIT		32	/* DFS state machine notified to begin ISM */
#define AS_LAST_STATE		33

/* assoc->type values */
#define AS_NONE			0 /* No assocication */
#define AS_ASSOCIATION		1 /* Join association */
#define AS_ROAM			2 /* Roaming association */
#define AS_RECREATE		3 /* Re-Creating a prior association */

#define AS_VALUE_NOT_SET	100 /* indicates uninitilized association state or type */

/* assoc->flags */
#define AS_F_CACHED_RESULTS	0x0001	/* Assoc used cached results */
#define AS_F_CACHED_CHANNELS	0x0002	/* Assoc used cached results to create a channel list */
#define AS_F_SPEEDY_RECREATE	0x0004	/* Speedy association recreation */
#define AS_F_CACHED_ROAM	0x0008	/* Assoc used cached results when directed roaming */

/* A fifo is full. Clear precedences related to that FIFO */
#define WLC_TX_FIFO_CLEAR(wlc, fifo) ((wlc)->tx_prec_map &= ~(wlc)->fifo2prec_map[fifo])

/* Fifo is NOT full. Enable precedences for that FIFO */
#define WLC_TX_FIFO_ENAB(wlc, fifo)  ((wlc)->tx_prec_map |= (wlc)->fifo2prec_map[fifo])

/* TxFrameID */
/* seq and frag bits: SEQNUM_SHIFT, FRAGNUM_MASK (802.11.h) */
/* rate epoch bits: TXFID_RATE_SHIFT, TXFID_RATE_MASK ((wlc_rate.c) */
#define TXFID_QUEUE_MASK	0x0007 /* Bits 0-2 */
#define TXFID_SEQ_MASK		0x7FE0 /* Bits 5-15 */
#define TXFID_SEQ_SHIFT		5      /* Number of bit shifts */
#define	TXFID_RATE_PROBE_MASK	0x8000 /* Bit 15 for rate probe */
#define TXFID_RATE_MASK		0x0018 /* Mask for bits 3 and 4 */
#define TXFID_RATE_SHIFT	3      /* Shift 3 bits for rate mask */

/* promote boardrev */
#define BOARDREV_PROMOTABLE	0xFF	/* from */
#define BOARDREV_PROMOTED	1	/* to */

/* power-save mode definitions */
#define PSQ_PKTS_BCMC		50	/* max # of enq'd PS bc/mc pkts */
#ifndef PSQ_PKTS_LO
#define PSQ_PKTS_LO		5	/* max # PS pkts scb can enq */
#endif
#ifndef PSQ_PKTS_HI
#define	PSQ_PKTS_HI		500
#endif

#ifdef STA
/* if wpa is in use then portopen is true when the group key is plumbed otherwise it is always true
 */
#define WLC_PORTOPEN(cfg) \
	(((cfg)->WPA_auth != WPA_AUTH_DISABLED && WSEC_ENABLED((cfg)->wsec)) ? \
	(cfg)->wsec_portopen : TRUE)

#ifdef WLDPT
extern bool wlc_dpt_pm_pending(dpt_info_t *dpt, wlc_bsscfg_t *cfg);
#define WLC_DPT_PM_PENDING(wlc, cfg) (DPT_ENAB((wlc)->pub) ? \
					wlc_dpt_pm_pending((wlc)->dpt, cfg) : FALSE)
#else
#define WLC_DPT_PM_PENDING(wlc, cfg) FALSE
#endif

#ifdef WLTDLS
extern bool wlc_tdls_pm_allowed(tdls_info_t *tdls, wlc_bsscfg_t *cfg);
#define WLC_TDLS_PM_ALLOWED(wlc, cfg) (TDLS_ENAB((wlc)->pub) ? \
					wlc_tdls_pm_allowed((wlc)->tdls, cfg) : TRUE)
#else
#define WLC_TDLS_PM_ALLOWED(wlc, cfg) TRUE
#endif

#define BTA_ACTIVE(wlc)	FALSE
#define BTA_IN_PROGRESS(wlc)	FALSE

#define PS_ALLOWED(cfg)	wlc_ps_allowed(cfg)
#define STAY_AWAKE(wlc) wlc_stay_awake(wlc)
#else
#define PS_ALLOWED(cfg)	FALSE
#define STAY_AWAKE(wlc)	TRUE
#endif /* STA */

#define WLC_STA_AWAKE_STATES_MAX	30
#define WLC_PMD_EVENT_MAX		16

/* 802.1D Priority to TX FIFO number for wme */
extern const uint8 prio2fifo[];

/* rm request types */
#define WLC_RM_TYPE_NONE		0 /* No Radio measurement type */
#define WLC_RM_TYPE_BASIC		1 /* Basic Radio measurement type */
#define WLC_RM_TYPE_CCA			2 /* CCA Radio measurement type */
#define WLC_RM_TYPE_RPI			3 /* RPI Radio measurement type */

/* rm request flags */
#define WLC_RM_FLAG_PARALLEL		(1<<0) /* Flag for setting for Parallel bit */
#define WLC_RM_FLAG_LATE		(1<<1) /* Flag for LATE bit */
#define WLC_RM_FLAG_INCAPABLE		(1<<2) /* Flag for Incapable bit */
#define WLC_RM_FLAG_REFUSED		(1<<3) /* Flag for REFUSED */

/* rm report type */
#define WLC_RM_CLASS_NONE		0 /* No report */
#define WLC_RM_CLASS_IOCTL		1 /* IOCTL type of report */
#define WLC_RM_CLASS_11H		2 /* Report for 11H */
#define WLC_RM_CLASS_11K		3 /* Report for 11K */
#define WLC_RM_CLASS_CCX		0x10 /* Report for CCX */

#define DATA_BLOCK_SCAN		(1 << 0)
#define DATA_BLOCK_QUIET	(1 << 1)
#define DATA_BLOCK_JOIN		(1 << 2)
#define DATA_BLOCK_PS		(1 << 3)
#define DATA_BLOCK_TX_SUPR	(1 << 4)
#define DATA_BLOCK_TXCHAIN	(1 << 5)
#define DATA_BLOCK_SPATIAL	(1 << 6)

/* Ucode MCTL_WAKE override bits */
#define WLC_WAKE_OVERRIDE_CLKCTL	0x01
#define WLC_WAKE_OVERRIDE_PHYREG	0x02
#define WLC_WAKE_OVERRIDE_MACSUSPEND	0x04
#define WLC_WAKE_OVERRIDE_TXFIFO	0x08
#define WLC_WAKE_OVERRIDE_FORCEFAST	0x10
#define WLC_WAKE_OVERRIDE_4335_PMWAR	0x20

/* stuff pulled in from wlc.c */

#define	RETRY_SHORT_DEF			7	/* Default Short retry Limit */
#define	RETRY_SHORT_MAX			255	/* Maximum Short retry Limit */
#define	RETRY_LONG_DEF			4	/* Default Long retry count */
#define	RETRY_LONG_DEF_AQM		6	/* Default Long retry count for mfbr */
#define	RETRY_SHORT_FB			3	/* Short retry count for fallback rate */
#define	RETRY_LONG_FB			2	/* Long retry count for fallback rate */

#define	MAXTXPKTS		6		/* max # pkts pending */
#define	MAXTXPKTS_AMPDUMAC	64
#ifndef	AMPDU_BA_MAX_WSIZE
/* max Tx/Rx ba window size (in pdu) for array allocations in structures. */
#define	AMPDU_BA_MAX_WSIZE	64
#endif

#ifndef MAXTXPKTS_AMPDUAQM
#define MAXTXPKTS_AMPDUAQM	1024		/* max # pkts pending for AQM */
#endif

#ifndef MAXTXPKTS_AMPDUAQM_DFT
/* default value of  max # pkts pending for AQM for BMAC (max buf size 128) */
#define MAXTXPKTS_AMPDUAQM_DFT	256
#endif

/* frameburst */
#define	MAXTXFRAMEBURST		8		/* vanilla xpress mode: max frames/burst */
#define	MAXFRAMEBURST_TXOP	10000		/* Frameburst TXOP in usec */

/* Per-AC retry limit register definitions; uses bcmdefs.h bitfield macros */
#define EDCF_SHORT_S            0
#define EDCF_SFB_S              4
#define EDCF_LONG_S             8
#define EDCF_LFB_S              12
#define EDCF_SHORT_M            BITFIELD_MASK(4)
#define EDCF_SFB_M              BITFIELD_MASK(4)
#define EDCF_LONG_M             BITFIELD_MASK(4)
#define EDCF_LFB_M              BITFIELD_MASK(4)

#define UCODE_MAJOR_BOM 0x0	/* Shm location to read major ucode bom */
#define UCODE_MINOR_BOM 0x2	/* Shm location to read minor ucode bom */

#ifdef STA
/* PM2 tick time in milliseconds and gptimer units */
#define WLC_PM2_TICK_MS	10
#define WLC_PM2_MAX_MS	2000
#define WLC_PM2_TICK_GP(ms)	(ms * 1024)
#endif	/* STA */

#define WME_RETRY_SHORT_GET(val, ac)        GFIELD(val, EDCF_SHORT)
#define WME_RETRY_SFB_GET(val, ac)          GFIELD(val, EDCF_SFB)
#define WME_RETRY_LONG_GET(val, ac)         GFIELD(val, EDCF_LONG)
#define WME_RETRY_LFB_GET(val, ac)          GFIELD(val, EDCF_LFB)

#define WLC_WME_RETRY_SHORT_GET(wlc, ac)    GFIELD(wlc->wme_retries[ac], EDCF_SHORT)
#define WLC_WME_RETRY_SFB_GET(wlc, ac)      GFIELD(wlc->wme_retries[ac], EDCF_SFB)
#define WLC_WME_RETRY_LONG_GET(wlc, ac)     GFIELD(wlc->wme_retries[ac], EDCF_LONG)
#define WLC_WME_RETRY_LFB_GET(wlc, ac)      GFIELD(wlc->wme_retries[ac], EDCF_LFB)

#define WLC_WME_RETRY_SHORT_SET(wlc, ac, val) \
	wlc->wme_retries[ac] = SFIELD(wlc->wme_retries[ac], EDCF_SHORT, val)
#define WLC_WME_RETRY_SFB_SET(wlc, ac, val) \
	wlc->wme_retries[ac] = SFIELD(wlc->wme_retries[ac], EDCF_SFB, val)
#define WLC_WME_RETRY_LONG_SET(wlc, ac, val) \
	wlc->wme_retries[ac] = SFIELD(wlc->wme_retries[ac], EDCF_LONG, val)
#define WLC_WME_RETRY_LFB_SET(wlc, ac, val) \
	wlc->wme_retries[ac] = SFIELD(wlc->wme_retries[ac], EDCF_LFB, val)

/* move WLC_NOISE_REQUEST_xxx  */
#define WLC_NOISE_REQUEST_SCAN	0x1
#define WLC_NOISE_REQUEST_CQ	0x2
#define WLC_NOISE_REQUEST_RM	0x4

#define WLC_RSSI_WINDOW_SZ	16	/* rssi window size */

/* PLL requests */
#define WLC_PLLREQ_SHARED	0x1	/* pll is shared on old chips */
#define WLC_PLLREQ_RADIO_MON	0x2	/* hold pll for radio monitor register checking */
#define WLC_PLLREQ_FLIP		0x4	/* hold/release pll for some short operation */

/* Do we support this rate? */
#define VALID_RATE(wlc, rspec) wlc_valid_rate(wlc, rspec, WLC_BAND_AUTO, FALSE)
#define VALID_RATE_DBG(wlc, rspec) wlc_valid_rate(wlc, rspec, WLC_BAND_AUTO, TRUE)

#define WLC_MIMOPS_RETRY_SEND	1
#define WLC_MIMOPS_RETRY_NOACK	2

/* Check if any join/roam is being processed */
#ifdef STA
#define AS_IN_PROGRESS(wlc)	((wlc)->assoc_req[0] != NULL)

/* Number of beacon-periods */
#define UATBTT_TO_ROAM_BCN 2
#endif /* STA */

#ifdef WLSCANCACHE
/* Time in ms longer than which a cached scan result is considered stale for the purpose
 * of an association attempt. Should be a value short enough to ensure that a target AP
 * would not have reconfigured since we collected the scan result.
 */
#ifndef BCMWL_ASSOC_CACHE_TOLERANCE
#define BCMWL_ASSOC_CACHE_TOLERANCE	8000	/* 8 sec old scan results should be re-scanned */
#endif /* BCMWL_ASSOC_CACHE_TOLERANCE */
#endif /* WLSCANCACHE */

/* set timeout for read/clear ucode uS counter at least every 3000 seconds
 *  to prevent 32bit overflow. max 32bit value is 4.294.967.295 uS
 */
#define TIMEOUT_TO_READ_PM_DUR 3000

/*
 * Macros to check if AP or STA is active.
 * AP Active means more than just configured: driver and BSS are "up";
 * that is, we are beaconing/responding as an AP (aps_associated).
 * STA Active similarly means the driver is up and a configured STA BSS
 * is up: either associated (stas_associated) or trying.
 *
 * Macro definitions vary as per AP/STA ifdefs, allowing references to
 * ifdef'd structure fields and constant values (0) for optimization.
 * Make sure to enclose blocks of code such that any routines they
 * reference can also be unused and optimized out by the linker.
 */
/* NOTE: References structure fields defined in wlc.h */
#if !defined(AP) && !defined(STA)
#define AP_ACTIVE(wlc)	(0)
#define STA_ACTIVE(wlc)	(0)
#elif defined(AP) && !defined(STA)
#define AP_ACTIVE(wlc)	((wlc)->aps_associated)
#define STA_ACTIVE(wlc) (0)
#elif defined(STA) && !defined(AP)
#define AP_ACTIVE(wlc)	(0)
#define STA_ACTIVE(wlc)	((wlc)->stas_associated > 0 || AS_IN_PROGRESS(wlc))
#else /* implies AP && STA defined */
#define AP_ACTIVE(wlc)	((wlc)->aps_associated)
#define STA_ACTIVE(wlc)	((wlc)->stas_associated > 0 || AS_IN_PROGRESS(wlc))
#endif /* defined(AP) && !defined(STA) */

/* pkt header pool for txstatus */
#ifdef DMATXRC
#define PHDR_ENAB(wlc)	((wlc)->pktpool_phdr->inited)
#define PHDR_POOL(wlc)	((wlc)->pktpool_phdr)
#define PHDR_POOL_LEN	SHARED_POOL_LEN
#else
#define PHDR_ENAB(wlc)	0
#define PHDR_POOL_LEN	1
#define PHDR_POOL(wlc)	((pktpool_t *)NULL)
#endif /* DMATXRC */

#define RXOV_MPDU_HIWAT         4               /* mpdu high watermark */
#define RXOV_TIMEOUT_BACKOFF    100             /* ms */
#define RXOV_TIMEOUT_MIN        600             /* ms */
#define RXOV_TIMEOUT_MAX        1200            /* ms */

#define	WLC_ACTION_ASSOC		1 /* Association request for MAC */
#define	WLC_ACTION_ROAM			2 /* Roaming request for MAC */
#define	WLC_ACTION_SCAN			3 /* Scan request for MAC */
#define	WLC_ACTION_QUIET		4 /* Quiet request for MAC */
#define	WLC_ACTION_RM			5 /* Radio Measure request for MAC */
#define	WLC_ACTION_ISCAN		6 /* Incremental Scan request for MAC */
#define	WLC_ACTION_RECREATE_ROAM	7 /* Roaming request for MAC */
#define WLC_ACTION_REASSOC		8 /* Reassociation request */
#define WLC_ACTION_RECREATE		9 /* Association recreate request */
#define WLC_ACTION_ESCAN		10
#define WLC_ACTION_ACTFRAME     	11 /* Action frame off home channel */
#define WLC_ACTION_PNOSCAN		12 /* PNO scan action */
#define WLC_ACTION_LPRIO_EXCRX		13 /* low priority excursion */

/*
 * Simple macro to change a channel number to a chanspec. Useable until the channels we
 * support overlap in the A and B bands.
 */
#define WLC_CHAN2SPEC(chan)	((chan) <= CH_MAX_2G_CHANNEL ? \
	(uint16)((chan) | WL_CHANSPEC_BAND_2G) : (uint16)((chan) | WL_CHANSPEC_BAND_5G))

#define WLC_DOT11_BSSTYPE(infra) ((infra) == 0 ? DOT11_BSSTYPE_INDEPENDENT : \
	DOT11_BSSTYPE_INFRASTRUCTURE)

/* wsec macros */
#define UCAST_NONE(prsn)	(((prsn)->ucount == 1) && \
	((prsn)->unicast[0] == WPA_CIPHER_NONE))
#define UCAST_AES(prsn)		(wlc_rsn_ucast_lookup(prsn, WPA_CIPHER_AES_CCM))
#define UCAST_TKIP(prsn)	(wlc_rsn_ucast_lookup(prsn, WPA_CIPHER_TKIP))
#if defined(WLTDLS)
#define UCAST_WEP(prsn)		(wlc_rsn_ucast_lookup(prsn, WPA_CIPHER_WEP_104) || \
	 wlc_rsn_ucast_lookup(prsn, WPA_CIPHER_WEP_40))
#endif 

#define MCAST_NONE(prsn)	((prsn)->multicast == WPA_CIPHER_NONE)
#define MCAST_AES(prsn)		((prsn)->multicast == WPA_CIPHER_AES_CCM)
#define MCAST_TKIP(prsn)	((prsn)->multicast == WPA_CIPHER_TKIP)
#define MCAST_WEP(prsn)		((prsn)->multicast == WPA_CIPHER_WEP_40 || \
	 (prsn)->multicast == WPA_CIPHER_WEP_104)

#define WLCWLUNIT(wlc)		((wlc)->pub->unit)

/* generic buffer length macro */
#define BUFLEN(start, end)	((end >= start) ? (int)(end - start) : 0)

#define BUFLEN_CHECK_AND_RETURN(len, maxlen, ret) \
{ \
	if ((int)(len) > (int)(maxlen)) {				\
		WL_ERROR(("%s, line %d, BUFLEN_CHECK_AND_RETURN: len = %d, maxlen = %d\n", \
			  __FUNCTION__, __LINE__, (int)(len), (int)(maxlen))); \
		ASSERT(0);						\
		return (ret);				\
	}								\
}

#define BUFLEN_CHECK_AND_RETURN_VOID(len, maxlen) \
{ \
	if ((int)(len) > (int)(maxlen)) {				\
		WL_ERROR(("%s, line %d, BUFLEN_CHECK_AND_RETURN_VOID: len = %d, maxlen = %d\n", \
			  __FUNCTION__, __LINE__, (int)(len), (int)(maxlen))); \
		ASSERT(0);						\
		return;							\
	}								\
}

/* Size of MAX possible TCP ACK SDU */
#define TCPACKSZSDU	(ETHER_HDR_LEN + DOT11_LLC_SNAP_HDR_LEN + 120)

typedef union wlc_txd {
	d11txh_t d11txh;
	d11actxh_t txd;
} wlc_txd_t;

enum txcore_index {
	CCK_IDX = 0,	/* CCK txcore index */
	OFDM_IDX,
	NSTS1_IDX,
	NSTS2_IDX,
	NSTS3_IDX,
	NSTS4_IDX,
	MAX_CORE_IDX
};

enum {
	/* Used as index for chanswitch_history() */
	/* Changes here need to be reflected in wlc_dump_chanswitch() */
	CHANSWITCH_SET_CHANSPEC = 0,
	CHANSWITCH_ADOPT_CONTEXT,
	CHANSWITCH_PREPPM_CONTEXT,
	CHANSWITCH_LAST
};

#define WLC_TXCHAIN_ID_USR		0 /* user setting */
#define WLC_TXCHAIN_ID_TEMPSENSE	1 /* chain mask for temperature sensor */
#define WLC_TXCHAIN_ID_PWRTHROTTLE	2 /* chain mask for pwr throttle feature for 43224 board */
#define WLC_TXCHAIN_ID_PWR_LIMIT	3 /* chain mask for tx power offsets */
#define WLC_TXCHAIN_ID_BTCOEX		4 /* chain mask for BT coex */
#define WLC_TXCHAIN_ID_COUNT		5 /* number of txchain_subval masks */

/* anything affects the single/dual streams/antenna operation */
typedef struct wlc_stf {
	uint8	hw_txchain;		/* HW txchain bitmap cfg */
	uint8	txchain;		/* txchain bitmap being used */
	uint8	txstreams;		/* number of txchains being used */

	uint8	hw_rxchain;		/* HW rxchain bitmap cfg */
	uint8	op_rxstreams;		/* number of operating rxchains being used */
	uint8	rxchain;		/* rxchain bitmap being used */
	uint8	rxstreams;		/* number of rxchains being used */

	uint8 	ant_rx_ovr;		/* rx antenna override */
	int8	txant;			/* userTx antenna setting */
	uint16	phytxant;		/* phyTx antenna setting in txheader */

	uint8	ss_opmode;		/* singlestream Operational mode, 0:siso; 1:cdd */
	bool	ss_algosel_auto;	/* if TRUE, use wlc->stf->ss_algo_channel; */
					/* else use wlc->band->stf->ss_mode_band; */
	uint16	ss_algo_channel;	/* ss based on per-channel algo: 0: SISO, 1: CDD 2: STBC */
	uint8   no_cddstbc;		/* stf override, 1: no CDD (or STBC) allowed */

	uint8   rxchain_restore_delay;	/* delay time to restore default rxchain */

	int8	ldpc;			/* ON/OFF ldpc RX supported */
	int8	ldpc_tx;		/* AUTO/ON/OFF ldpc TX supported */
	uint8	txcore_idx;		/* bitmap of selected core for each Nsts */
	uint8	txcore[MAX_CORE_IDX][2];	/* bitmap of selected core for each Nsts */
	uint8	txcore_override[MAX_CORE_IDX];	/* override of txcore for each Nsts */
	int8	spatialpolicy;
	int8	spatialpolicy_pending;
	uint16	shmem_base;
	ppr_t	*txpwr_ctl;

	bool	tempsense_disable;	/* disable periodic tempsense check */
	uint	tempsense_period;	/* period to poll tempsense */
	uint 	tempsense_lasttime;
	uint8   siso_tx;		/* use SISO and limit rates to MSC 0-7 */

	uint	ipaon;			/* IPA on/off, assume it's the same for 2g and 5g */
	int	pwr_throttle;		/* enable the feature */
	uint8	throttle_state;		/* hwpwr/temp throttle state */
	uint8   tx_duty_cycle_pwr;	/* maximum allowed duty cycle under pwr throttle */
	uint16	tx_duty_cycle_ofdm;	/* maximum allowed duty cycle for OFDM */
	uint16	tx_duty_cycle_cck;	/* maximum allowed duty cycle for CCK */
	uint8	pwr_throttle_mask;	/* core to enable/disable with thromal throttle kicks in */
	uint8	pwr_throttle_test;	/* for testing */
	uint8	txchain_pending;	/* pending value of txchain */
	bool	rssi_pwrdn_disable;	/* disable rssi power down feature */
	uint8	txchain_subval[WLC_TXCHAIN_ID_COUNT];	/* set of txchain enable masks */
	int8	spatial_mode_config[SPATIAL_MODE_MAX_IDX]; /* setting for each band or sub-band */
	wl_txchain_pwr_offsets_t txchain_pwr_offsets;
	int8 	onechain;		/* disable 1 TX/RS */
	uint8	pwrthrottle_config;
	uint8	pwrthrottle_pin;
	uint16	shm_rt_txpwroff_pos;

	uint16	tx_duty_cycle_ofdm_40_5g;	/* max allowed duty cycle for 5g 40Mhz OFDM */
	uint16	tx_duty_cycle_thresh_40_5g;	/* max rate in 500K to apply 5g 40Mhz duty cycle */
	uint16	tx_duty_cycle_ofdm_80_5g;	/* max allowed duty cycle for 5g 40Mhz OFDM */
	uint16	tx_duty_cycle_thresh_80_5g;	/* max rate in 500K to apply 5g 40Mhz duty cycle */
	ppr_t	*txpwr_ctl_qdbm;

	/* used by OLPC to register for callback if stored stf state */
	/* changes during phy calibration */
	wlc_stf_txchain_evt_notify txchain_perrate_state_modify;
	uint8	max_offset;
	/* Country code allows txbf */
	uint8	allow_txbf;
	uint8   tx_duty_cycle_thermal;	/* maximum allowed duty cycle under thermal throttle */
	uint8   txstream_value;
	bool	coremask_override;
} wlc_stf_t;

#define WLC_PWRTHROTTLE_AUTO	-1

#define WLC_THROTTLE_OFF	0
#define WLC_PWRTHROTTLE_ON	1
#define WLC_TEMPTHROTTLE_ON	2

#define MIN_DUTY_CYCLE_ALLOWED 15 /* 15% min dutycycle allowed */
#define NO_DUTY_THROTTLE 100 /* dutycycle throttle disabled */
#define WLC_DUTY_CYCLE_PWR_DEF 25 /* 25% default pwr */
#define WLC_DUTY_CYCLE_PWR_50 50 /* 50% dutycycle pwr */
#define WLC_DUTY_CYCLE_THERMAL_50 50 /* 50% dutycycle pwr */

/* Macro to determine if STBC transmition is forced regardless the receiver capabilities */
#define WLC_IS_STBC_TX_FORCED(wlc) \
	((wlc)->stf->txstreams > 1 && (wlc)->band->band_stf_stbc_tx == ON)

/*
 * Macro to determine if STBC transmition can be applied with regard
 * to the receiver HT STBC capabilities.
*/
#define WLC_STF_SS_STBC_HT_AUTO(wlc, scb) \
	((wlc)->stf->txstreams > 1 && \
	(wlc)->band->band_stf_stbc_tx == AUTO && \
	SCB_STBC_CAP(scb) && \
	isset(&((wlc)->stf->ss_algo_channel), PHY_TXC1_MODE_STBC))

/*
 * Macro to determine if STBC transmition can be applied with regard
 * to the receiver VHT STBC capabilities.
*/
#define WLC_STF_SS_STBC_VHT_AUTO(wlc, scb) \
	((wlc)->stf->txstreams > 1 && \
	(wlc)->band->band_stf_stbc_tx == AUTO && \
	SCB_VHT_RX_STBC_CAP(wlc->vhti, scb) && \
	isset(&((wlc)->stf->ss_algo_channel), PHY_TXC1_MODE_STBC))

#define WLC_STBC_CAP_PHY(wlc)  ((wlc_phy_cap_get((wlc)->band->pi) & PHY_CAP_STBC) != 0)
#define WLC_SGI_CAP_PHY(wlc)   ((wlc_phy_cap_get((wlc)->band->pi) & PHY_CAP_SGI) != 0)
#define WLC_40MHZ_CAP_PHY(wlc) ((wlc_phy_cap_get((wlc)->band->pi) & PHY_CAP_40MHZ) != 0)
#define WLC_80MHZ_CAP_PHY(wlc) ((wlc_phy_cap_get((wlc)->band->pi) & PHY_CAP_80MHZ) != 0)
#define WLC_LDPC_CAP_PHY(wlc)  ((wlc_phy_cap_get((wlc)->band->pi) & PHY_CAP_LDPC) != 0)
#define WLC_VHT_PROP_RATES_CAP_PHY(wlc) \
	((wlc_phy_cap_get((wlc)->band->pi) & PHY_CAP_VHT_PROP_RATES) != 0)

/*
 * Multiband 101:
 *
 * A "band" is a radio frequency/channel range.
 * We support the 802.11b band (2.4Ghz) using the B or G phy and the 2050 radio.
 * We support the 802.11a band (5  Ghz) using the A      phy and the 2060 radio.
 *
 * Some chips and boards support a single band (uniband) and some
 * support both B/G and A bands (multiband).
 *
 * Versions of the dot11 io core <= 4 support a single phytype/band.
 * Versions of the dot11 io core >= 5 support multiple phytypes.
 * The sbtmstatelow.gmode flag is used to switch between phytypes.
 *
 * 4306B0 (corerev4) includes two dot11 cores, one with a Gphy and one with an Aphy.
 * 4306C0 and later chips (>= corerev5) include a single dot11 core having
 * some combination of A, B, and/or G cores .
 * The driver multiband design abstracts the core portion
 * (such as dma engines and pio fifos) from the band portion (phy+radio state).
 *
 * For convenience, band 0 is always the B/G phy, and, if it exists,
 * band 1 is the A phy.
 */

#define EARLY_DETECT_PERIOD 1000 /* ms */

typedef struct wlc_early_bcn_detect {
	uint32 thresh; /* user-thresh for which bcn are considered early, usecs */
	uint32 earliest_offset; /* offset (from next tbtt) of earliest bcn detected, usecs */
	bool detect_done;
	uint32 assoc_start_time;
	uint32 uatbtt_start_time;
} wlc_early_bcn_detect_t;

/*
 * core state (mac)
 */
typedef struct wlccore {
#ifdef WLC_LOW
	uint		coreidx;		/* # sb enumerated core */

	/* fifo */
	uint		*txavail[NFIFO];	/* # tx descriptors available */
	int16		txpktpend[NFIFO];	/* tx admission control */
#endif /* WLC_LOW */

#ifdef WLC_HIGH
	macstat_t	*macstat_snapshot;	/* mac hw prev read values */
#endif
} wlccore_t;

/*
 * band state (phy+ana+radio)
 */
typedef struct wlcband {
	int		bandtype;		/* WLC_BAND_2G, WLC_BAND_5G */
	uint		bandunit;		/* bandstate[] index */

	uint16		phytype;		/* phytype */
	uint16		phyrev;
	uint16		radioid;
	uint16		radiorev;
	wlc_phy_t	*pi;			/* pointer to phy specific information */
	bool		abgphy_encore;

#ifdef WLC_HIGH
	uint8		gmode;			/* currently active gmode (see wlioctl.h) */

	struct scb	*hwrs_scb;		/* permanent scb for hw rateset */

	wlc_rateset_t	defrateset;		/* band-specific copy of default_bss.rateset */

	ratespec_t 	rspec_override;		/* 802.11 rate override */
	ratespec_t	mrspec_override;	/* multicast rate override */
	uint8		band_stf_ss_mode;	/* Configured STF type, 0:siso; 1:cdd */
	int8		band_stf_stbc_tx;	/* STBC TX 0:off; 1:force on; -1:auto */
	wlc_rateset_t	hw_rateset;		/* rates supported by chip (phy-specific) */
	uint8		basic_rate[WLC_MAXRATE + 1]; /* basic rates indexed by rate */
	uint8		bw_cap;			/* Bandwidth bitmask on this band */
	int		roam_trigger;		/* "good enough" threshold for current AP */
	uint		roam_delta;		/* "signif better" thresh for new AP candidates */
	int		roam_trigger_init_def;	/* roam trigger default value in intialization */
	int		roam_trigger_def;	/* roam trigger default value */
	uint		roam_delta_def;		/* roam delta default value */
	int8		antgain;		/* antenna gain from srom */
	int8		sar;			/* SAR programmed in srom */
	int8		sar_cached;		/* cache SROM SAR value */

	uint16		CWmin;	/* The minimum size of contention window, in unit of aSlotTime */
	uint16		CWmax;	/* The maximum size of contention window, in unit of aSlotTime */
	uint16		bcntsfoff;		/* beacon tsf offset */

	/* multiple roaming profile support (up to 4 RSSI brackets) */
	wl_roam_prof_t *roam_prof;
#endif /* WLC_HIGH */
} wlcband_t;

/* generic function callback takes just one arg */
typedef void (*cb_fn_t)(void *);

#if defined(WLC_HIGH) || defined(SCANOL)
/* scan completion callback */
typedef void (*scancb_fn_t)(void *wlc, int status, wlc_bsscfg_t *cfg);
/* scan action callback */
typedef void (*actcb_fn_t)(wlc_info_t *wlc, void *arg, uint *dwell);
#endif /* WLC_HIGH || SCANOL */

#define WLC_MAX_BRCM_ELT	32	/* Max size of BRCM proprietary elt */

/* module control blocks */
typedef	struct modulecb {
	char name[32];			/* module name : NULL indicates empty array member */
	const bcm_iovar_t *iovars;	/* iovar table */
	watchdog_fn_t watchdog_fn;	/* watchdog handler */
	iovar_fn_t iovar_fn;		/* iovar handler */
	up_fn_t up_fn;			/* up handler */
	down_fn_t down_fn;		/* down handler. Note: the int returned
					 * by the down function is a count of the
					 * number of timers that could not be
					 * freed.
					 */
	int ref_cnt;
} modulecb_t;

typedef	struct modulecb_patch {
	const bcm_iovar_t *iovars;	/* patch iovar table */
	iovar_fn_t        iovar_fn;	/* patch iovar handler */
} modulecb_patch_t;

/* For RSDB, modules are attached per WLC.
 * We share the modulecb (function_ptrs) and store modulecb_data (handles) separately
 */
typedef struct modulecb_data {
	void *hdl;			/* Module Instance/Handle passed when 'doiovar' is called */
} modulecb_data_t;

/* ioctl control blocks */
typedef struct wlc_ioctl_cb_s {
	void *ioctl_hdl;		/* module context */
	wlc_ioctl_fn_t ioctl_fn;	/* ioctl handler */
	int num_cmds;			/* Number of elements in ioctl_cmds list */
	const wlc_ioctl_cmd_t *ioctl_cmds;	/* list of ioctl commands and flags that the
					 * handler knows how to process
					 */
	struct wlc_ioctl_cb_s *next;
} wlc_ioctl_cb_t;

typedef int (*wlc_assoc_dc_handle_fn_t)(void *handle,
	wlc_bsscfg_t *cfg, void *arg, struct scb *scb);

typedef struct wlc_dc_handle_s {
	uint16	dc_type;
	wlc_assoc_dc_handle_fn_t dc_handle_fn;
} wlc_dc_handle_t;

typedef struct wlc_assoc_dc_cb_s {
	uint16	assoc_dc_type;		/* dc type */
	wlc_assoc_dc_handle_fn_t assoc_dc_handle_fn;	/* dc handler */
	struct wlc_assoc_dc_cb_s *next;
} wlc_assoc_dc_cb_t;

/* dump control blocks */
typedef	struct dumpcb_s {
	const char *name;		/* dump name */
	dump_fn_t dump_fn;		/* 'wl dump' handler */
	void *dump_fn_arg;
	struct dumpcb_s *next;
} dumpcb_t;

/* Allowed transmit features for scb */
typedef enum scb_txmod {
	TXMOD_START, /* Node to designate start of the tx path */
	TXMOD_DPT,
	TXMOD_TDLS,
	TXMOD_APPS,
	TXMOD_TRF_MGMT,
	TXMOD_NAR,
	TXMOD_AMSDU,
	TXMOD_AMPDU,
	TXMOD_TRANSMIT,	/* Node to designate enqueue to common tx queue */
	TXMOD_LAST
} scb_txmod_t;

/* Function that does transmit packet feature processing. prec is precedence of the packet */
typedef void (*txmod_tx_fn_t)(void *ctx, struct scb *scb, void *pkt, uint prec);

/* Callback for txmod when it gets deactivated by other txmod */
typedef void (*txmod_deactivate_fn_t)(void *ctx, struct scb *scb);

/* Callback for txmod when it gets activated */
typedef void (*txmod_activate_fn_t)(void *ctx, struct scb *scb);

/* Callback for txmod to return packets held by this txmod */
typedef uint (*txmod_pktcnt_fn_t)(void *ctx);

/* go from wlc + txmod id to pkt count */
extern uint wlc_txmod_get_pkts_pending(struct wlc_info *wlc, scb_txmod_t fid);

/* Function vector to make it easy to initialize txmod
 * Note: txmod_info itself is not modified to avoid adding one more level of indirection
 * during transmit of a packet
 */
typedef struct txmod_fns {
	txmod_tx_fn_t		tx_fn;			/* Process the packet */
	txmod_pktcnt_fn_t	pktcnt_fn;		/* Return the packet count */
	txmod_deactivate_fn_t	deactivate_notify_fn;	/* Handle the deactivation of the feature */
	txmod_activate_fn_t	activate_notify_fn;	/* Handle the activation of the feature */
} txmod_fns_t;

/* Structure to store registered functions for a Txmod */
typedef struct txmod_info {
	txmod_tx_fn_t		tx_fn;			/* Process the packet */
	txmod_deactivate_fn_t	deactivate_notify_fn;	/* Handle the deactivation of the feature */
	txmod_activate_fn_t	activate_notify_fn;	/* Handle the activation of the feature */
	txmod_pktcnt_fn_t	pktcnt_fn;		/* Return the packet count */
	void *ctx; 					/* Opaque handle to be passed */
} txmod_info_t;

/* global OBSS Coex info */
#define OBSS_CHANVEC_SIZE	CEIL(CH_MAX_2G_CHANNEL + 1, NBBY)
typedef struct obss_info {
	uint8		num_chan;		/* num of 2G channels */
	uint8		chanvec[OBSS_CHANVEC_SIZE]; /* bitvec of channels in 2G */
	uint8		coex_map[CH_MAX_2G_CHANNEL + 1]; /* channel map of coexist in 2G */
} obss_info_t;

#define WLC_INTOL40_DET(cfg) (((cfg)->obss->coex_det & WL_COEX_40MHZ_INTOLERANT) != 0)
#define WLC_WIDTH20_DET(cfg) (((cfg)->obss->coex_det & WL_COEX_WIDTH20) != 0)
#define WLC_INTOL40_OVRD(cfg) (((cfg)->obss->coex_ovrd & WL_COEX_40MHZ_INTOLERANT) != 0)
#define WLC_WIDTH20_OVRD(cfg) (((cfg)->obss->coex_ovrd & WL_COEX_WIDTH20) != 0)
#define WLC_COEX_STATE_BITS(bit) (bit & (WL_COEX_40MHZ_INTOLERANT | WL_COEX_WIDTH20))

#define COEX_MASK_TEA	0x1
#define COEX_MASK_TEB	0x2
#define COEX_MASK_TEC	0x4
#define COEX_MASK_TED	0x8

/* coex bw downgrade reason code */
#define COEX_UPGRADE_TIMER 	0
#define COEX_DGRADE_TEA		1
#define COEX_DGRADE_40INTOL	2
#define COEX_DGRADE_TEBC	3
#define COEX_DGRADE_TED		4

/* virtual interface */
struct wlc_if {
	wlc_if_t	*next;
	uint8		type;		/* WLC_IFTYPE_BSS or WLC_IFTYPE_WDS */
	uint8		index;		/* assigned in wl_add_if(), index of the wlif if any,
					 * not necessarily corresponding to bsscfg._idx or
					 * AID2PVBMAP(scb).
					 */
	uint8		if_flags;	/* flags for the interface */
	wl_if_t		*wlif;		/* pointer to wlif */
	struct wlc_txq_info *qi;	/* pointer to associated tx queue */
	union {
		struct scb *scb;		/* pointer to scb if WLC_IFTYPE_WDS */
		struct wlc_bsscfg *bsscfg;	/* pointer to bsscfg if WLC_IFTYPE_BSS */
	} u;
	wlc_if_stats_t  _cnt;		/* interface stats counters */
};

/* virtual interface types */
#define WLC_IFTYPE_BSS 1		/* virt interface for a bsscfg */
#define WLC_IFTYPE_WDS 2		/* virt interface for a wds */

/* flags for the interface */
#define WLC_IF_PKT_80211	0x01	/* this interface expects 80211 frames */
#define WLC_IF_LINKED		0x02	/* this interface is linked to a wl_if */
#define WLC_IF_VIRTUAL		0x04

#if defined(DMA_TX_FREE)
#define WLC_TXSTATUS_SZ 128
#define WLC_TXSTATUS_VEC_SZ	(WLC_TXSTATUS_SZ/NBBY)
typedef struct wlc_txstatus_flags {
	uint	head;
	uint	tail;
	uint8	vec[WLC_TXSTATUS_VEC_SZ];
}  wlc_txstatus_flags_t;
#endif /* DMA_TX_FREE */

#define CHANSWITCH_TIMES_HISTORY	8
typedef struct {
	struct  {
		chanspec_t	from;	/* Switch from here */
		chanspec_t	to;	/* to here */
		uint32		start;	/* Start time (in tsf) */
		uint32		end;	/* end time (in tsf) */
	} entry[CHANSWITCH_TIMES_HISTORY];
	int index;
} chanswitch_times_t;

#if defined(DELTASTATS)
#define DELTA_STATS_NUM_INTERVAL	2	/* number of stats intervals stored */

/* structure to store info for delta statistics feature */
typedef struct delta_stats_info {
	uint32 interval;		/* configured delta statistics interval (secs) */
	uint32 seconds;			/* counts seconds to next interval */
	uint32 current_index;		/* index into stats of the current statistics */
	wl_delta_stats_t stats[DELTA_STATS_NUM_INTERVAL];	/* statistics for each interval */
} delta_stats_info_t;
#endif

/* structure for chanim support */
/* accumulative  count structure */
typedef struct {
	uint32 glitch_cnt;
	uint32 badplcp_cnt;
	uint32 ccastats_us[CCASTATS_MAX]; /* in microsecond */
	chanspec_t chanspec;
	uint stats_ms;			/* accumulative time, in millisecond */
	uint32 bphy_glitch_cnt;
	uint32 bphy_badplcp_cnt;
} chanim_accum_t;

/* basic count struct */
typedef struct {
	uint16 glitch_cnt;
	uint16 badplcp_cnt;
	uint32 ccastats_cnt[CCASTATS_MAX];
	uint timestamp;
	uint32 bphy_glitch_cnt;
	uint32 bphy_badplcp_cnt;
} chanim_cnt_t;

/* configurable parameters */
typedef struct {
	uint8 mode;		/* configurable chanim mode */
	uint8 sample_period;	/* in seconds, time to do a sampling measurement */
	uint8 threshold_time;	/* number of sample period to trigger an action */
	uint8 max_acs;		/* the maximum acs scans for one lockout period */
	uint32 lockout_period;	/* in seconds, time for one lockout period */
	uint32 crsglitch_thres;
	uint32 ccastats_thres;
	int8 bgnoise_thres; /* background noise threshold */
	uint scb_max_probe; /* when triggered by intf, how many times to probe */
} chanim_config_t;

/* transient counters/stamps */
typedef struct {
	bool state;
	uint32 detecttime;
	uint32 flags;
	uint8 record_idx;	/* where the next acs record should locate */
	uint scb_max_probe; /* original number of scb probe to conduct */
	uint scb_timeout; /* the storage for the original scb timeout that is swapped */
} chanim_mark_t;

typedef struct wlc_chanim_stats {
	struct wlc_chanim_stats *next;
	chanim_stats_t chanim_stats;
	bool is_valid;
} wlc_chanim_stats_t;

typedef struct {
	wlc_chanim_stats_t *stats; /* normalized stats obtained during scan */
#ifdef WLC_CHANIM_STATS_ROM_COMPAT
	uint8 cur_stats_obsolete[40]; /* Unused. Keep for ROM compatibility. */
#endif
	chanim_accum_t accum_cnt[CHANIM_ACCUM_CNT]; /* accumulative cnt */
	chanim_cnt_t last_cnt;	/* count from last read */
	chanim_mark_t mark;
	chanim_config_t config;
	chanim_acs_record_t record[CHANIM_ACS_RECORD];
	wlc_chanim_stats_t cur_stats; /* normalized stats obtained in home channel */
} chanim_info_t;

#define chanim_mark(c_info)	(c_info)->mark
#define chanim_config(c_info)	(c_info)->config
#define chanim_act_delay(c_info) \
	(chanim_config(c_info).sample_period * chanim_config(c_info).threshold_time)

typedef struct {
	uint32 txdur;	/* num usecs tx'ing */
	uint32 ibss;	/* num usecs rx'ing cur bss */
	uint32 obss;	/* num usecs rx'ing other data */
	uint32 noctg;	/* 802.11 of unknown type */
	uint32 nopkt;	/* non 802.11 */
	uint32 usecs;	/* usecs in this interval */
	uint32 PM;	/* usecs MAC spent in doze mode for PM */
#if defined(ISID_STATS) || defined(ISID_STATS_IN_ROM)
	uint32 crsglitch;	/* num rxcrsglitchs */
	uint32 badplcp;	/* num bad plcp */
	uint32 bphy_crsglitch;  /* num bphy rxcrsglitchs */
	uint32 bphy_badplcp;    /* num bphy bad plcp */
#endif /* ISID_STATS || ISID_STATS_IN_ROM */
} cca_ucode_counts_t;

#if defined(CCA_STATS) || defined(CCA_STATS_IN_ROM)
typedef struct {
	wl_chan_qual_event_t event;
	struct wl_timer *cca_event_timer;	/* timer to limit event notifications */
	bool	is_cca_event_timer_active;	/* flag to indicate timer active */
	uint8 level[WL_CHAN_QUAL_TOTAL];
} cca_chan_qual_t;
#endif /* CCA_STATS || CCA_STATS_IN_ROM */

/* Source Address based duplicate detection (Avoid heavy SCBs) */
#define SA_SEQCTL_SIZE 10

struct sa_seqctl {
	struct ether_addr sa;
	uint16 seqctl_nonqos;
};


/* TX Queue information
 *
 * Each flow of traffic out of the device has a TX Queue with independent
 * flow control. Several interfaces may be associated with a single TX Queue
 * if they belong to the same flow of traffic from the device. For multi-channel
 * operation there are independent TX Queues for each channel.
 */
struct wlc_txq_info {
	struct wlc_txq_info *next;
	struct pktq	q;
	uint		stopped;	/* tx flow control bits */
};

/* Flags used in wlc_txq_info.stopped */
#define TXQ_STOP_FOR_PRIOFC_MASK	0x000000FF /* per prio flow control bits */
#define TXQ_STOP_FOR_PKT_DRAIN		0x00000100 /* stop txq enqueue for packet drain */
#define TXQ_STOP_FOR_AMPDU_FLOW_CNTRL	0x00000200 /* stop txq enqueue for ampdu flow control */
#define TXQ_STOP_FOR_MCHAN_FLOW_CNTRL	0x00000400 /* stop txq enqueue for mchan flow control */

/* defines for gptimer stay awake requestor */
#define WLC_GPTIMER_AWAKE_MCHAN		0x00000001
#define WLC_GPTIMER_AWAKE_MCHAN_SPECIAL	0x00000002
#define WLC_GPTIMER_AWAKE_MCHAN_ABS	0x00000004
#define WLC_GPTIMER_AWAKE_TBTT		0x00000008
#define WLC_GPTIMER_AWAKE_AWDL		0x00000010
#define WLC_GPTIMER_AWAKE_PROXD		0x00000020
#define WLC_GPTIMER_AWAKE_MCHAN_PRECLOSE	0x00000040
#define WLC_GPTIMER_AWAKE_MCHAN_WAIT	(0x00000800)

/* defines for user_wake_req stay awake requestor */
#define WLC_USER_WAKE_REQ_NIC		0x00000001
#define WLC_USER_WAKE_REQ_VHT		0x00000004

/* defined for PMblocked requestor */
#define WLC_PM_BLOCK_SCAN		0x00000001
#define WLC_PM_BLOCK_MCHAN_CHANSW	0x00000002
#define WLC_PM_BLOCK_MCHAN_ABS		0x00000004
#define WLC_PM_BLOCK_AWDL		0x00000008



#if defined(PKTC) || defined(PKTC_DONGLE)
#define PKTC_POLICY_SENDUPC	0	/* Default, send up chained */
#define PKTC_POLICY_SENDUPUC	1	/* Send up unchained */
/* Receive frames chaining info */
struct wlc_rfc {
	wlc_bsscfg_t		*bsscfg;
	struct scb		*scb;
	void			*wlif_dev;
	uint16			prio;
	struct ether_addr 	ds_ea;
	uint8			ac;
};

struct wlc_pktc_info {
	struct	wlc_rfc		rfc[2];		/* receive chaining info */
	struct ether_addr 	*h_da;		/* da of head frame in chain */
	uint8			policy;		/* chaining policy */
};
#endif /* PKTC || PKTC_DONGLE */

#if defined(WL_LTR) || defined(BCM43602A0)
typedef struct wlc_ltr_info {
	uint8 ltr_sw_en;		/* Turn LTR cap on/off in driver */
	uint8 ltr_hw_en;		/* Flag indicating LTR supported in hw */
	uint32 active_lat;		/* LTR active latency */
	uint32 active_idle_lat;	/* LTR active idle latency (not used) */
	uint32 sleep_lat;		/* LTR sleep latency */
	uint32 hi_wm;			/* FIFO watermark for LTR active transition */
	uint32 lo_wm;			/* FIFO watermark for LTR sleep transition */
} wlc_ltr_info_t;
#endif /* WL_LTR || BCM43602A0 */

#ifdef WLC_HIGH_ONLY
typedef struct bmac_rpc_cache {
    bool      cached_tsf;
    bool      cached_mac;
    uint32    tsf_stale;
    uint32    tsf_l;
    uint32    tsf_h;
    uint32    macctrl;
} bmac_rpc_cache_t;
#define WLC_BMAC_RPC_CACHE(wlc, param) (wlc->bmac_rpc_cache.param)
#endif
/** hardware assisted mechanism to alternate fast(er) between two bands */
struct wlc_srvsdb_info {
	uint8  sr_vsdb_enabled;       /* is set when hardware was enabled for srvsdb operation */
	uint8  iovar_force_vsdb;      /* debug mechanism to force vsdb using wl utility */
	uint16 vsdb_chans[2];	     /* the two alternated channels */
	uint8 vsdb_save_valid[2];    /* is set for channel when hw+sw context was saved */
	uint8 ss_algo_save_valid[2]; /* switch faster by skipping stf related calls if possible */
	uint16 ss_algosaved[2];
	ppr_t*	vsdb_txpwr_ctl[2];   /* save/restore power per rate context per channel */
};

#ifdef WLOFFLD
typedef struct {
	uint32  txbytes_prev;   /* Number of bytes transmitted until last second */
	uint32  rxbytes_prev;   /* Number of bytes received until last second */
	uint32  txbytes_rate;   /* Derive this - Number of bytes transmitted in last second */
	uint32  rxbytes_rate;   /* Derive this - Number of bytes received in last second */
	uint32  txframes_prev;   /* Number of frames transmitted until last second */
	uint32  rxframes_prev;   /* Number of frames received until last second */
	uint32  txframes_rate;   /* Derive this - Number of frames transmitted in last second */
	uint32  rxframes_rate;   /* Derive this - Number of frames received in last second */
} wlc_rate_measurement_t;
#endif /* WLOFFLD */

/* Info block related to excess_pm_wake */
typedef struct wlc_excess_pm_wake_info {
	uint32	roam_ms;		/* roam time since driver load */
	uint32	pfn_scan_ms;		/* pfn-scan time since driver load */

	uint32	roam_alert_thresh;	/* User config */
	uint32	pfn_alert_thresh;	/* User config */

	bool	roam_alert_enable;
	bool	pfn_alert_enable;

	uint32	roam_alert_thresh_ts;	/* roam_ms ts on IOVAR set */
	uint32	pfn_alert_thresh_ts;	/* pfn_scan_ms ts on IOVAR set */

	uint32	last_pm_prd_roam_ts;	/* roam ts when last excess_pm_wake prd started */
	uint32	last_pm_prd_pfn_ts;	/* pfn_scan ts when last excess_pm_wake prd started */

	uint32	ca_thresh;	/* user configured constant awake thresh */
	uint32	last_ca_pmdur;	/* last constant awake pmdur */
	uint32	last_ca_osl_time;	/* constant awake osl time */
	uint32	pp_start_event_dur[WLC_PMD_EVENT_MAX]; /* pm period start event-duration */
	uint32	ca_start_event_dur[WLC_PMD_EVENT_MAX]; /* constant awake start event-duration */
	uint32	ca_txrxpkts;	/* transmit receive packets */
	uint32	ca_pkts_allowance;	/* max packets allowed in const awake to generate alert */
	uint32	ca_alert_pmdur;	/* pmdur value at time of const awake alert */
	uint32	ca_alert_pmdur_valid;	/* to differentiate if ca_alert_pmdur is valid */
} wlc_excess_pm_wake_info_t;

/* For recording connection stats */
typedef struct wlc_conn_stats {
	uint32	connect_count;
	uint32	connect_dur;
	uint32	connect_start;
	uint32	connect_end;
	bool	connect_in_progress;
} wlc_conn_stats_t;

/*
 * Principal common (os-independent) software data structure.
 */
struct wlc_info {
	wlc_pub_t	*pub;			/* pointer to wlc public state */
	osl_t		*osh;			/* pointer to os handle */
	struct wl_info	*wl;			/* pointer to os-specific private state */
	d11regs_t	*regs;			/* pointer to device registers */

	wlc_hw_info_t	*hw;			/* HW module (private h/w states) */
	wlc_hw_t	*hw_pub;		/* shadow/direct accessed HW states */

#ifdef WLC_SPLIT
	rpc_info_t	*rpc;			/* Handle to RPC module */
#endif

	/* up and down */
	bool		device_present;		/* (removable) device is present */

	bool		clk;			/* core is out of reset and has clock */
	bool		watchdog_disable;	/* to disable watchdog */

	/* multiband */
	wlccore_t	*core;			/* pointer to active io core */
	wlcband_t	*band;			/* pointer to active per-band state */
	wlccore_t	*corestate;		/* per-core state (one per hw core) */
	wlcband_t	*bandstate[MAXBANDS];		/* per-band state (one per phy/radio) */

	bool		war16165;		/* PCI slow clock 16165 war flag */
	uint16		bt_shm_addr;

	bool		tx_suspended;		/* data fifos need to remain suspended */

	uint		txpend16165war;
	int8		phy_noise_list[MAXCHANNEL];	/* noise right after tx */

	/* packet queue */
	uint		qvalid;			/* DirFrmQValid and BcMcFrmQValid */
	bool            in_send_q;              /* flag to prevent concurent calls to wlc_send_q */



#ifdef WLC_HIGH
#ifdef WLC_HIGH_ONLY
	rpctx_info_t	*rpctx;			/* RPC TX module */
	bool		reset_bmac_pending;	/* bmac reset is in progressing */
	uint32		rpc_agg;		/* host agg: bit 16-31, bmac agg: bit 0-15 */
	uint32		rpc_msglevel;		/* host rpc: bit 16-31, bmac rpc: bit 0-15 */
#ifdef DNGL_WD_KEEP_ALIVE
	bool        	dngl_wd_keep_alive;   	/* enable or disable dngl keep alive timer */
#endif
    bmac_rpc_cache_t bmac_rpc_cache;
#endif /* WLC_HIGH_ONLY */

	/* sub-module handler */
	scb_module_t	*scbstate;		/* scb module handler */
	bsscfg_module_t	*bcmh;			/* bsscfg module handler */
	wlc_seq_cmds_info_t	*seq_cmds_info;	/* pointer to sequence commands info */
#ifdef WLLED
	led_info_t	*ledh;			/* pointer to led specific information */
#endif
#ifdef WLAMSDU
	amsdu_info_t	*ami;			/* amsdu module handler */
#endif
#ifdef WLNAR
	wlc_nar_info_t *nar_handle;		/* nar module private data */
#endif
#ifdef WLAMPDU
	ampdu_tx_info_t *ampdu_tx;		/* ampdu tx module handler */
	ampdu_rx_info_t *ampdu_rx;		/* ampdu rx module handler */
#endif
#ifdef WLTAF
	wlc_taf_info_t *taf_handle;	/* tx packet taf private data */
#endif /* WLTAF */
#ifdef WOWL
	wowl_info_t	*wowl;			/* WOWL module handler */
#endif
#ifdef WLDPT
	dpt_info_t	*dpt;			/* dpt module handler */
#endif
#ifdef WLTDLS
	tdls_info_t	*tdls;			/* tdls module handler */
#endif
#ifdef WLDLS
	dls_info_t		*dls;			/* dls module handler */
#endif
#ifdef L2_FILTER
	l2_filter_info_t *l2_filter;	/* L2 filter module handler */
#endif
#ifdef BCMAUTH_PSK
	wlc_auth_info_t	*authi;			/* authenticator module handler shim */
#endif
#ifdef WLMCNX
	wlc_mcnx_info_t *mcnx;			/* p2p ucode module handler */
	wlc_tbtt_info_t *tbtt;			/* tbtt module handler */
#endif
#ifdef WLP2P
	wlc_p2p_info_t	*p2p;			/* p2p module handler */
#endif
#ifdef WLMCHAN
	mchan_info_t	*mchan;			/* mchan module handler */
#endif
#ifdef PSTA
	wlc_psta_info_t	*psta;			/* proxy sta module info */
#endif /* PSTA */
#ifdef WLEXTLOG
	wlc_extlog_info_t *extlog;		/* extlog handler */
#endif
	antsel_info_t	*asi;			/* antsel module handler */
	wlc_cm_info_t	*cmi;			/* channel manager module handler */
	wlc_ratesel_info_t	*wrsi;		/* ratesel info wrapper */
	wlc_cac_t	*cac;			/* CAC handler */
	wlc_scan_info_t	*scan;			/* ptr to scan state info */
	wlc_plt_pub_t	*plt;			/* plt(Production line Test) module handler */
#ifdef WET
	void		*weth;			/* pointer to wet specific information */
#endif
#ifdef WET_TUNNEL
	void		*wetth;			/* pointer to wet tunnel specific information */
#endif
	wlc_11h_info_t	*m11h;			/* 11h module handler */
	wlc_tpc_info_t	*tpc;			/* TPC module handler */
	wlc_csa_info_t	*csa;			/* CSA module handler */
	wlc_quiet_info_t *quiet;		/* Quiet module handler */
	wlc_dfs_info_t *dfs;			/* DFS module handle */
	wlc_11d_info_t *m11d;			/* 11d module handler */
	wlc_cntry_info_t *cntry;		/* Country module handler */
	wlc_11u_info_t *m11u;			/* 11u module handler */
	wlc_probresp_info_t *mprobresp; 	/* SW probe response module handler */

	wlc_wapi_info_t *wapi;			/* WAPI (WPI/WAI) module handler */
	int		wapi_cfgh;		/* WAI bsscfg cubby handler */
	wlc_txc_info_t *txc;			/* tx header caching module handler */
	wlc_btc_info_t *btch;			/* btc module header */

	void		*btparam;		/* bus type specific cookie */

	uint16		vendorid;		/* PCI vendor id */
	uint16		deviceid;		/* PCI device id */
	uint		ucode_rev;		/* microcode revision */

	uint32		machwcap;		/* MAC capabilities, BMAC shadow */
	uint16		xmtfifo_szh[NFIFO];	/* fifo size in 256B for each xmt fifo */
	uint16		xmtfifo_frmmaxh[AC_COUNT];	/* max # of frames fifo size can hold */

	struct ether_addr perm_etheraddr;	/* original sprom local ethernet address */

	bool		bandlocked;		/* disable auto multi-band switching */
	bool		bandinit_pending;	/* track band init in auto band */

	bool		radio_monitor;		/* radio timer is running */
	bool		down_override;		/* true=down */
	bool		going_down;		/* down path intermediate variable */

#ifdef WLSCANCACHE
	bool		_assoc_cache_assist;	/* enable use of scan cache in assoc attempts */
#endif
	bool		mpc;			/* enable minimum power consumption */
	bool		mpc_out;		/* disable radio_mpc_disable for out */
	bool		mpc_scan;		/* disable radio_mpc_disable for scan */
	bool		mpc_join;		/* disable radio_mpc_disable for join */
	bool		mpc_oidscan;		/* disable radio_mpc_disable for oid scan */
	bool		mpc_oidjoin;		/* disable radio_mpc_disable for oid join */
	bool		mpc_oidp2pscan;		/* disable radio_mpc_disable for oid p2p scan */
	bool		mpc_oidnettype;		/* disable radio_mpc_disable for oid
						 * network_type_in_use
						 */
	uint8		mpc_dlycnt;		/* # of watchdog cnt before turn disable radio */
	uint32		mpc_off_ts;		/* timestamp (ms) when radio was disabled */
	uint8		mpc_delay_off;		/* delay radio disable by # of watchdog cnt */

	/* timer */
	struct wl_timer *wdtimer;		/* timer for watchdog routine */
	uint		fast_timer;		/* Periodic timeout for 'fast' timer */
	uint		slow_timer;		/* Periodic timeout for 'slow' timer */
	uint		glacial_timer;		/* Periodic timeout for 'glacial' timer */
	uint		phycal_mlo;		/* last time measurelow calibration was done */
	uint		phycal_txpower;		/* last time txpower calibration was done */

	struct wl_timer *radio_timer;		/* timer for hw radio button monitor routine */

	/* promiscuous */
	uint32		monitor;		/* monitor (MPDU sniffing) mode */
	bool		bcnmisc_ibss;		/* bcns promisc mode override for IBSS */
	bool		bcnmisc_scan;		/* bcns promisc mode override for scan */
	bool		bcnmisc_monitor;	/* bcns promisc mode override for monitor */

	bool		channel_qa_active;	/* true if chan qual measurement in progress */

	uint8		bcn_wait_prd;		/* max waiting period (for beacon) in 1024TU */

	/* driver feature */
	bool		frameburst;		/* enable per-packet framebursting */
#ifdef WLAMSDU
	bool		_amsdu_rx;		/* true if currently amsdu deagg is enabled */
	bool		_rx_amsdu_in_ampdu;	/* true if currently amsdu deagg is enabled */
#endif
	bool		_amsdu_noack;		/* enable AMSDU no ack mode */
	bool		_rifs;			/* enable per-packet rifs */
	int32		rifs_advert;		/* RIFS mode advertisement */
	int32		rifs_mode;		/* RIFS mode in the HT Info IE */
	int8		sgi_tx;			/* sgi tx */
	bool		wet;			/* true if wireless ethernet bridging mode */
	bool		mac_spoof;		/* Original wireless ethernet, MAC Clone/Spoof */


	/* AP-STA synchronization, power save */
	/* These are only summary flags and used as global WAKE conditions.
	 *  Change them through accessor functions done on each BSS.
	 * - check_for_unaligned_tbtt - wlc_set_uatbtt(cfg, state)
	 * - PMpending - wlc_set_pmpending(cfg, state)
	 * - PSpoll - wlc_set_pspoll(cfg, state)
	 * - PMawakebcn - wlc_set_pmwakebcn(cfg, state)
	 */
	bool		check_for_unaligned_tbtt; /* check unaligned tbtt flag */
	bool		PMpending;		/* waiting for tx status with PM indicated set */
	bool		PSpoll;			/* whether there is an outstanding PS-Poll frame */
	bool		PMawakebcn;		/* bcn recvd during current waking state */

	bool		wake;			/* host-specified PS-mode sleep state */
	mbool		user_wake_req;
#ifdef DEBUG_TBTT
	uint32		prev_TBTT;		/* TSF when last TBTT indicated */
	bool		bad_TBTT;		/* lost the race and iterated */
#endif
	uint8		bcn_li_bcn;		/* beacon listen interval in # beacons */
	uint8		bcn_li_dtim;		/* beacon listen interval in # dtims */

	bool		WDarmed;		/* watchdog timer is armed */
	uint32		WDlast;			/* last time wlc_watchdog() was called */

	/* WME */
	ac_bitmap_t	wme_dp;			/* Discard (oldest first) policy per AC */
	uint16		wme_dp_precmap;		/* Precedence map based on discard policy */
	bool		wme_prec_queuing;	/* enable/disable non-wme STA prec queuing */
	uint16		wme_retries[AC_COUNT];  /* per-AC retry limits */
	uint32		wme_maxbw_ac[AC_COUNT];	/* Max bandwidth per allowed per ac */

	int		vlan_mode;		/* OK to use 802.1Q Tags (ON, OFF, AUTO) */
	uint16		tx_prec_map;		/* Precedence map based on HW FIFO space */
	uint16		fifo2prec_map[NFIFO];	/* pointer to fifo2_prec map based on WME */

	/* BSS Configurations */
	wlc_bsscfg_t	**bsscfg;		/* set of BSS configurations, idx 0 is default and
						 * always valid
						 */
	wlc_bsscfg_t	*cfg;			/* the primary bsscfg (can be AP or STA) */
#ifdef MBSS
	struct ether_addr vether_base;		/* Base virtual MAC addr when user
						 * doesn't provide one
						 */
	uint8		cur_dtim_count;		/* current DTIM count */
	int8		hw2sw_idx[WLC_MAXBSSCFG]; /* Map from uCode index to software index */
	uint32		last_tbtt_us;		/* Timestamp of TBTT time */
	int8		beacon_bssidx;		/* Track start config to rotate order of beacons */
#if defined(WLC_HIGH) && defined(WLC_LOW)
	uint16		prq_base;		/* Base address of PRQ in shm */
	uint16		prq_rd_ptr;		/* Cached read pointer for PRQ */
	int		bcast_next_start;	/* For rotating probe responses to bcast requests */
#endif /* WLC_HIGH && WLC_LOW */
#endif /* MBSS */
	uint8		stas_associated;	/* count of ASSOCIATED STA bsscfgs */
	uint8		stas_connected;		/* # ASSOCIATED STA bsscfgs with valid BSSID */
	uint8		aps_associated;		/* count of UP AP bsscfgs */
	uint8		ibss_bsscfgs;		/* count of IBSS bsscfgs */
	uint8		block_datafifo;		/* prohibit posting frames to data fifos */
	bool		bcmcfifo_drain;		/* TX_BCMC_FIFO is set to drain */

	/* tx queue */
	wlc_txq_info_t	*tx_queues;		/* common TX Queue list */

	/* event */
	wlc_eventq_t 	*eventq;		/* event queue for deferred processing */

	/* security */
	wsec_key_t	**wsec_keys;	/* dynamic key storage */
	wsec_key_t*	wsec_def_keys[WLC_DEFAULT_KEYS];	/* default key storage */
	bool		wsec_swkeys;		/* indicates that all keys should be
						 * treated as sw keys (used for debugging)
						 */

	uint32		lifetime[AC_COUNT];	/* MSDU lifetime per AC in us */
	modulecb_t	*modulecb;		/* Module callback fptrs */
	modulecb_data_t	*modulecb_data;		/* Module handle instance */
	wlc_ioctl_cb_t	*ioctl_cb_head;
	dumpcb_t	*dumpcb_head;
	txmod_info_t	*txmod_fns;

	uint8		mimo_band_bwcap;	/* bw cap per band type */
	bool		mimo_mixedmode;		/* mimo preamble type */
#ifdef WLAMPDU
	bool		ampdu_rts;		/* use RTS for every AMPDU */
#endif
	int8		txburst_limit_override; /* tx burst limit override */
	uint16		txburst_limit;		/* tx burst limit value */
	int8		cck_40txbw;		/* 11N, cck tx b/w override when in 40MHZ mode */
	int8		ofdm_40txbw;		/* 11N, ofdm tx b/w override when in 40MHZ mode */
	int8		mimo_40txbw;		/* 11N, mimo tx b/w override when in 40MHZ mode */
	ht_cap_ie_t	ht_cap;			/* HT CAP IE being advertised by this node */
	ht_add_ie_t	ht_add;			/* HT ADD IE being used by this node */
	obss_info_t	*obss;			/* OBSS Coexistance info */

	uint		seckeys;		/* 54 key table shm address */
	uint		tkmickeys;		/* 12 TKIP MIC key table shm address */

	/* channel quality measure */
	int		channel_quality;	/* quality metric(0-3) of last measured channel, or
						 * -1 if in progress
						 */
	uint8		channel_qa_channel;	/* channel number of channel being evaluated */
	int8		channel_qa_sample[WLC_CHANNEL_QA_NSAMP]; /* rssi samples of background
								  * noise
								  */
	uint		channel_qa_sample_num;	/* count of samples in channel_qa_sample array */
#ifndef SCANOL
	wlc_bss_list_t	*scan_results;
#endif
	int32		scanresults_minrssi;	/* RSSI threshold under which beacon/probe responses
						* are tossed due to weak signal
						*/
	wlc_bss_list_t	*custom_scan_results;	/* results from ioctl scan */
	uint	custom_scan_results_state;	/* see WL_SCAN_RESULTS_* states in wlioctl.h */
	uint	custom_iscan_results_state;	/* see WL_SCAN_RESULTS_* states in wlioctl.h */
#ifdef STA
	iscan_ignore_t	*iscan_ignore_list;	/* networks to ignore on subsequent iscans */
	uint	iscan_ignore_last;		/* iscan_ignore_list count from prev partial scan */
	uint	iscan_ignore_count;		/* cur number of elements in iscan_ignore_list */
	chanspec_t iscan_chanspec_last;		/* resume chan after prev partial scan */
	struct wl_timer *iscan_timer;		/* stops iscan after iscan_duration ms */
#endif /* STA */

	wlc_bss_info_t	*default_bss;		/* configured BSS parameters */

	uint16		AID;			/* association ID */
	uint16		counter;		/* per-sdu monotonically increasing counter */
	uint16		mc_fid_counter;		/* BC/MC FIFO frame ID counter */

	struct ether_addr	desired_BSSID;	/* allow this station */

	bool		ibss_allowed;		/* FALSE, all IBSS will be ignored during a scan
						 * and the driver will not allow the creation of
						 * an IBSS network
						 */
	bool 		ibss_coalesce_allowed;

	/* country, spect management */
	bool		country_list_extended;	/* JP-variants are reported through
						 * WLC_GET_COUNTRY_LIST if TRUE
						 */

	uint16		prb_resp_timeout;	/* do not send prb resp if request older than this,
						 * 0 = disable
						 */

	wlc_rateset_t	sup_rates_override;	/* use only these rates in 11g supported rates if
						 * specifed
						 */

	/* 11a rate table direct adddress map */
	uint16		rt_dirmap_a[D11_RT_DIRMAP_SIZE];

	/* 11b rate table direct adddress map */
	uint16		rt_dirmap_b[D11_RT_DIRMAP_SIZE];

	int16	rssi_win_rfchain[WL_RSSI_ANT_MAX][WLC_RSSI_WINDOW_SZ]; /* rssi per antenna */
	uint8	rssi_win_rfchain_idx;

	chanspec_t	home_chanspec;		/* shared home chanspec */

	/* PHY parameters */
	chanspec_t	chanspec;		/* target operational channel */
	uint16		usr_fragthresh;	/* user configured fragmentation threshold */
	uint16		fragthresh[NFIFO];	/* per-fifo fragmentation thresholds */
	uint16		RTSThresh;		/* 802.11 dot11RTSThreshold */
	uint16		SRL;			/* 802.11 dot11ShortRetryLimit */
	uint16		LRL;			/* 802.11 dot11LongRetryLimit */
	uint16		SFBL;			/* Short Frame Rate Fallback Limit */
	uint16		LFBL;			/* Long Frame Rate Fallback Limit */

	/* network config */
	bool	shortslot;		/* currently using 11g ShortSlot timing */
	int8	shortslot_override;	/* 11g ShortSlot override */
	bool	ignore_bcns;		/* override: ignore non shortslot bcns in a 11g network */
	bool	interference_mode_crs;	/* aphy crs state for interference mitigation mode */
	bool	legacy_probe;		/* restricts probe requests to CCK rates */

	/* 11g/11n protections */
	wlc_prot_info_t *prot;		/* 11g & 11n protection module handle */
	wlc_prot_g_info_t *prot_g;	/* 11g protection module handle */
	wlc_prot_n_info_t *prot_n;	/* 11n protection module handle */

	wlc_stf_t *stf;

	wlc_pcb_info_t *pcb;		/* packet tx complete callback */

	uint32		txretried;		/* tx retried number in one msdu */

	uint8		nickname[32];		/* Set by vx/linux ioctls but currently unused */

	ratespec_t	bcn_rspec;		/* save bcn ratespec purpose */

#ifdef STA
	bool		IBSS_join_only;		/* don't start IBSS if not found */


	wlc_bsscfg_t	*assoc_req[WLC_MAXBSSCFG]; /* join/roam requests */

	/* association */
	uint		sta_retry_time;		/* time to retry on initial assoc failure */

	wlc_bss_list_t	*join_targets;
	uint		join_targets_last;	/* index of last target tried (next: --last) */

#ifdef WLRM
	rm_info_t	*rm_info;
#endif /* WLRM */
	/* Demodulator frequency tracking */
	bool		freqtrack;		/* Have we increased the frequency
						 * tracking bandwidth of the
						 * demodulator?
						 */
	uint		freqtrack_starttime;	/* Start time(in seconds) of the last
						 * frequency tracking attempt
						 */
	int8		freqtrack_attempts;	/* Number of times we tried to acquire
						 * beacons by increasing the freq
						 * tracking b/w
						 */
	int8		freqtrack_override;	/* Override setting from registry */

	/* These are only summary flags and used as global WAKE conditions.
	 *  Change them through accessor functions done on each BSS.
	 * - apsd_sta_usp - wlc_set_apsd_stausp(cfg, state)
	 */
	bool		apsd_sta_usp;		/* Unscheduled Service Period in progress on STA */
#endif	/* STA */

#ifdef AP
	bool		cs_scan_ini;		/* Channel Scan started flag */
	chanspec_t	chanspec_selected;	/* chan# selected by WLC_START_CHANNEL_SEL */
	chanspec_t	*scan_chanspec_list;
	int		scan_chanspec_list_size;
	int		scan_chanspec_count;
	uint		scb_timeout;		/* inactivity timeout for associated STAs */
	uint		scb_activity_time;	/* skip probe if activity during this time */
	bool		reprobe_scb;		/* to let watchdog know there are scbs to probe */
	uint		scb_max_probe;		/* max number of probes to be conducted */
#endif	/* AP */

#ifndef WLC_INFO_RELOC_WLCHANIM
#ifdef WLCHANIM
	chanim_info_t *chanim_info;
#endif	/* WLCHANIM */
#endif	/* WLC_INFO_RELOC_WLCHANIM */

	apps_wlc_psinfo_t *psinfo;              /* Power save nodes related state */
	wlc_ap_info_t *ap;

	uint8	htphy_membership;		/* HT PHY membership */

	/* Global txmaxmsdulifetime, global rxmaxmsdulifetime */
	uint32		txmsdulifetime;
	uint16		rxmsdulifetime;

#ifdef BRCMAPIVTW
	bool		brcm_ap_iv_tw_unused;
	int8		brcm_ap_iv_tw_override;
#endif

#if defined(DELTASTATS)
	delta_stats_info_t *delta_stats;
#endif /* DELTASTATS */

	uint8		noise_req;		/* to manage phy noise sample requests */
	int		phynoise_chan_scan;	/* sampling target channel for scan */

	uint16	rfaware_lifetime;	/* RF awareness lifetime (us/1024, not ms) */
	uint32	exptime_cnt;		/* number of expired pkts since start_exptime */
	uint32	last_exptime;		/* sysuptime for last timer expiration */

	uint8	txpwr_percent;		/* power output percentage */

	uint8	ht_wsec_restriction;	/* the restriction of HT with TKIP or WEP */
#endif /* WLC_HIGH */

#ifdef MBSS
	uint8		mbss_ucidx_mask;	/* used for extracting ucidx from macaddr */
	uint32		max_ap_bss;		/* max ap bss supported by driver */
#endif /* MBSS */


	uint16		tx_duty_cycle_ofdm;	/* maximum allowed duty cycle for OFDM */
	uint16		tx_duty_cycle_cck;	/* maximum allowed duty cycle for CCK */
	bool		nav_reset_war_disable;	/* WAR to reset NAV on 0 duration ACK */

	/* parameters for delaying radio shutdown after sending NULL PM=1 */
	uint16		pm2_radio_shutoff_dly;	/* configurable radio shutoff delay */
	bool		pm2_radio_shutoff_pending;	/* flag indicating radio shutoff pending */
	struct wl_timer *pm2_radio_shutoff_dly_timer;	/* timer to delay radio shutoff */

	struct sa_seqctl ctl_dup_det[SA_SEQCTL_SIZE]; /* Small table for duplicate detections
						       * for cases where SCB does not exist
						       */
	uint8 ctl_dup_det_idx;

#if defined(RWL_WIFI) || defined(WIFI_REFLECTOR)
	void *rwl;
#endif /* RWL_WIFI  || WIFI_REFLECTOR */

	uint16	next_bsscfg_ID;

	/* An unique request ID is assciated to corelate request/response-set */
	uint16	escan_sync_id;

	wlc_rrm_info_t *rrm_info;	/* 11k radio measurement info */
#ifdef WLWNM
	wlc_wnm_info_t *wnm_info;	/* 11v wireless radio management info */
#endif
	wlc_if_t	*wlcif_list;	/* linked list of wlc_if structs */
	wlc_txq_info_t	*active_queue;	/* txq for the currently active transmit context */

#ifdef WL_MULTIQUEUE
	wlc_txq_info_t *primary_queue;	/* current queue for normal (non-excursion) traffic */
	wlc_txq_info_t *excursion_queue; /* txq for excursions (scan, measurement, etc.) */
	bool excursion_active;
	bool txfifo_detach_pending;
	bool txfifo_detach_transition_queue_del; /* del detach tran q after sync is done */
	wlc_txq_info_t *txfifo_detach_transition_queue;
#endif /* WL_MULTIQUEUE */

#ifdef WLPFN
	void		*pfn;
#endif
	uint32		mpc_dur;	/* total time (ms) in mpc mode except for the
					 * portion since radio is turned off last time
					 */
	uint32		mpc_laston_ts;	/* timestamp (ms) when radio is turned off last
					 * time
					*/

	bool		pr80838_war;
	bool		nolinkup; 	/* suppress link up events */
	uint		hwrxoff;
	uint		hwrxoff_pktget;
	wlc_hrt_info_t *hrti;	/* hw gptimer for multiplexed use */
	mbool		gptimer_stay_awake_req; /* bitfield used to keep device awake
						 * if user of gptimer requires it
						 */
	uint16	wake_event_timeout;	/* timeout value in seconds to trigger sending wake_event */
	struct wl_timer *wake_event_timer;	/* timer to send an event up to the host */

#ifdef STA
#if defined(AP_KEEP_ALIVE)
	uint16		keep_alive_time;
	uint16		keep_alive_count;
#endif 
	bool	reset_triggered_pmoff;		/* tells us if reset took place and turned off PM */
#endif /* STA */

	bool rpt_hitxrate;	/* report highest tx rate in history */
#ifdef DMATXRC
	pktpool_t *pktpool_phdr;
	int phdr_len;
	void **phdr_list;
	uint8 txrc_fifo_mask;
#endif
#ifdef WLRXOV
	uint8 rxov_active;
	uint16 rxov_txmaxpkts;
	uint16 rxov_delay;
	struct wl_timer *rxov_timer;
#endif
	bool is_initvalsdloaded;
#if defined(PROP_TXSTATUS)
	/*
	- b[0] = 1 to enable WLFC_FLAGS_RSSI_SIGNALS
	- b[1] = 1 to enable WLFC_FLAGS_XONXOFF_SIGNALS
	- b[2] = 1 to enable WLFC_FLAGS_CREDIT_STATUS_SIGNALS
	*/
	uint8	wlfc_flags;
	uint8	wlfc_vqdepth; /* max elements to store in psq during flow control */
	void*   wlfc_data; /* to store wlfc_mac_desc_handle_map_t data */
#endif


	uint8	roam_rssi_cancel_hysteresis;	/* Cancel ROAM RSSI Hysteresis */

	uint   	iscan_result_last;	/* scanresult index returned in last iscanresults */
	uint32  pkteng_maxlen;      /* maximum packet length */

	int16 excess_pm_period;
	int16 excess_pm_percent;
	uint32 excess_pm_last_pmdur;
	uint32 excess_pm_last_osltime;

	/* Notifier module. */
	bcm_notif_module_t	*notif;

	/* Memory pool manager handle. */
	bcm_mpm_mgr_h	mem_pool_mgr;
#ifndef WLC_HIGH_ONLY
	uint16 fabid;
#endif
	bool  brcm_ie;        /* flag to disable/enable brcm ie in auth req */
	uint32 prb_req_enable_mask;	/* mask of probe request clients */
	uint32 prb_rsp_disable_mask;	/* mask of disable ucode probe response clients */
	ratespec_t	monitor_ampdu_rspec;	/* monitor rspec value for AMPDU sniffing */
	void		*monitor_amsdu_pkts;	/* monitor packet queue for AMSDU sniffing */
	uint32 		monitor_ampdu_counter;
#ifdef STA
	bool    seq_reset;              /* used for determining whether to reset
	                                 * sequence number register in SCR or not
	                                 */
#endif
	struct wlc_btc_param_vars_info* btc_param_vars; /* storage for BTC NVRAM data */

	bool    toe_bypass;
	bool    toe_capable;

	bool dma_avoidance_war;

	uint shm_mbss_base;		/* MBSS ucode shared memory base address */
	uint shm_bcn0_tpl_base;		/* Beacon template base */
	uint txfifo_size_unit;
	uint tplblks_per_bcn;
	uint tplblks_per_prs;
#ifdef TRAFFIC_MGMT
	wlc_trf_mgmt_ctxt_t	    *trf_mgmt_ctxt;    /* pointer to traffic management context */
#endif
	wlc_pktc_info_t	*pktc_info;	/* packet chaining info */

#define WLC_INFO_WL_MFP_INFO_FIELD \
	wlc_mfp_info_t *mfp;
#ifndef WLC_INFO_RELOC_WL_MFP_INFO_FIELD
#ifdef MFP
	WLC_INFO_WL_MFP_INFO_FIELD
#endif /* MFP */
#endif /* !WLC_INFO_RELOC_WL_MFP_INFO_FIELD */

	wlc_macfltr_info_t *macfltr;	/* MAC allow/deny list module handle */
	uint32	nvramrev;
	wlc_bmon_info_t *bmon;		/* BSSID monitor module handle */

	bool blocked_for_slowcal;
	wlc_rmc_info_t *rmc;
	wlc_iem_info_t *iemi;	/* IE mgmt module */
	wlc_vht_info_t *vhti;	/* VHT module */
	wlc_ht_info_t *hti;	/* HT module */
	wlc_akm_info_t *akmi;	/* AKM module */
	wlc_ier_info_t *ieri;	/* IE registry module */
	wlc_ier_reg_t *ier_tdls_srq;	/* TDLS Setup Request registry */
	wlc_ier_reg_t *ier_tdls_srs;	/* TDLS Setup Response registry */
	wlc_ier_reg_t *ier_tdls_scf;	/* TDLS Setup Confirm registry */
	wlc_ier_reg_t *ier_tdls_drs;	/* TDLS Discovery Response registry */
	wlc_ier_reg_t *ier_csw;	/* CSA Wrapper IE registry */
	wlc_ier_reg_t *ier_fbt;	/* FBT over the DS registry */
	wlc_ier_reg_t *ier_ric;	/* FBT RIC Request registry */
	int noise_lvl;

#ifdef ANQPO
	void		*anqpo;			/* anqp offload module handler */
#endif	/* ANQPO */
	uint32 atf;
	wlc_hs20_info_t *hs20;		/* hotspot module handler */
	wlc_sup_info_t	*idsup;			/* supplicant module */
	wlc_fbt_info_t	*fbt;
	wlc_ccxsup_info_t *ccxsup;
	wlc_pmkid_info_t	*pmkid_info;
#ifdef BCM_OL_DEV
	wlc_dngl_ol_info_t  * wlc_dngl_ol;
#endif /* BCM_OL_DEV */
#if defined(WL_LTR) || defined(BCM43602A0)
	wlc_ltr_info_t ltr_info;
#endif /* WL_LTR || BCM43602A0 */
#ifdef WLOLPC
	wlc_olpc_eng_info_t *olpc_info;
#endif /* WLOLPC */

#ifdef WL_BCN_COALESCING
	wlc_bcn_clsg_info_t *bc;		/* beacon coalescing module handler */
#endif /* WL_BCN_COALESCING */
	struct wlc_srvsdb_info *srvsdb_info;

#ifdef WLOFFLD
	wlc_ol_info_t *ol;
	bool ol_tsfupdate;
	wlc_rate_measurement_t	*_wlc_rate_measurement;
#endif

#define WLC_INFO_WL_OKC_INFO_FIELD \
	okc_info_t *okc_info;

#ifndef WLC_INFO_RELOC_WL_OKC_INFO_FIELD
#if defined(WL_OKC) || defined(WLRCC)
	WLC_INFO_WL_OKC_INFO_FIELD
#endif
#endif /* !WLC_INFO_RELOC_WL_OKC_INFO_FIELD */

	wlc_staprio_info_t *staprio;
	wlc_stamon_info_t *stamon_info; /* Opaque pointer to sta monitor module */
	wlc_monitor_info_t *mon_info; /* Opaque pointer to monitor module */
	wlc_wds_info_t	*mwds;			/* wds module handler */
	wlc_lpc_info_t		*wlpci;		/* LPC module handler */

	chanswitch_times_t	*chansw_hist[CHANSWITCH_LAST]; /* chanswitch history */
	chanswitch_times_t	*bandsw_hist[CHANSWITCH_LAST]; /* bandswitch history */

	bool comp_stat_enab;

	uint8 passive_on_restricted;	/* controls scan on restricted channels */
	modulecb_patch_t	*modulecb_patch;
	wlc_obj_registry_t *objr; /* Opaque pointer to Dictionary */

	bool allow_txbf;
	wlc_txbf_info_t *txbf;
	uint32 us_per_1024ticks; /* ref tick to us ratio, calibrated by software first then ucode */
	uint16 scc_per;		/* ref tick to us ratio, same as above, only the integral part */
	uint16 scc_per_frac;  /* the fractional part, 10 bits precision */
	wlc_early_bcn_detect_t *ebd;
	void	*awdl_info; 	/* awdl module handler */

#if defined(CCA_STATS) || defined(CCA_STATS_IN_ROM)
	cca_info_t *cca_info;

#if defined(ISID_STATS) || defined(ISID_STATS_IN_ROM)
	itfr_info_t *itfr_info;
#endif /* ISID_STATS || ISID_STATS_IN_ROM */

	cca_chan_qual_t *cca_chan_qual;
#endif /* CCA_STATS || CCA_STATS_IN_ROM  */
	int noise_lte_lvl;
	wmf_info_t	*wmfi;			/* wmf module handler */
	uint band_order[2];
	uint32 lpas; /* Low power associated sleep mode */
	uint32 wlc_pm_dur_cntr;		/* pm_dur fw counter in ms */
	uint16 pm_dur_clear_timeout;	/* timeout to read/reset ucode counter */
	uint32 wlc_pm_dur_last_sample;
#ifdef KEEP_ALIVE
	void *keepalive;
#endif
	uint16 longpkt_rtsthresh;
	uint16 longpkt_shm;
#ifdef WLPFN
	uint32 excess_pm_last_mpcdur;
#endif
	void *pwrstats;
	wlc_excess_pm_wake_info_t *excess_pmwake;
	wlc_conn_stats_t *conn_stats; /* connection statistics */
	uint16 pm_bcmc_wait; 	/* pm_bcmc_wait */
	bool pm_bcmc_wait_force_zero; 	/* force BCMC timeout to zero */

	bool  psm_watchdog_debug;     /* used for ucode debugging */

	mbool	core_wake; /* indicating each bsscfg core wake state */

	wlc_pps_info_t *pps_info; /* Opaque pointer to ps-pretend */

#ifdef WLDURATION
	duration_info_t *dur_info;	/* Opaque pointer to the duration module */
#endif
#ifdef NET_DETECT
	wlc_net_detect_ctxt_t *net_detect_ctxt; /* NetDetect context */
#endif
#ifdef WLAIBSS
	wlc_aibss_info_t *aibss_info;			/* AIBSS module handler */
#endif	/* WLAIBSS */

#ifdef WLC_INFO_RELOC_WL_OKC_INFO_FIELD
#if defined(WL_OKC) || defined(WLRCC)
	WLC_INFO_WL_OKC_INFO_FIELD
#endif
#endif /* WLC_INFO_RELOC_WL_OKC_INFO_FIELD */
	wlc_prot_obss_info_t *prot_obss;

#ifdef WLC_INFO_RELOC_WL_MFP_INFO_FIELD
	WLC_INFO_WL_MFP_INFO_FIELD
#endif /* WLC_INFO_RELOC_WL_MFP_INFO_FIELD */
	wlc_stats_info_t	*stats_info;	/* Opaque pointer to wlc statistics module */
	wlc_bs_data_info_t  *bs_data_handle;
	wlc_ltecx_info_t	*ltecx;		/* LTECX info (high driver part?) */
	wlc_ipfo_info_t *ipfo_info;

	wlc_pdsvc_info_t *pdsvc_info; /* proximity detection module handler */

	bool tcpack_fast_tx;    /* Faster AMPDU release and transmission in ucode for TCPACK */

#if defined WLOTA_EN
	ota_test_info_t * ota_info;
	uint8 *iov_block;
#endif /* WLOTA_EN */

#ifdef WLC_INFO_RELOC_WLCHANIM
#ifdef WLCHANIM
	chanim_info_t *chanim_info;
#endif	/* WLCHANIM */
#endif	/* WLC_INFO_RELOC_WLCHANIM */

#if defined(WME_PER_AC_TX_PARAMS)
	uint16		wme_max_rate[AC_COUNT]; /* In units of 512 Kbps */
#endif
	wlc_bssload_info_t *mbssload;	/* bss load IE info */
	wlc_mpf_info_t *mpf;
#ifdef WOWLPF
	wowlpf_info_t	*wowlpf;		/* WOWLPF module handler */
#endif
	bool    wd_waitfor_bcn;		/* watchdog timer is armed */
	bool	watchdog_on_bcn;	/* enable/disable delay watchdog on beacon  */
	void *mprobresp_mac_filter;	/* MAC based SW probe resp module */
	bool	hostmem_access_enabled;
	wlc_modesw_info_t *modesw; /* modesw Wlc structure pointer */
	wlc_assoc_dc_cb_t	*assoc_dc_cb_head;	/* assoc decision callback */
	wlc_addrmatch_info_t *addrmatch_info;	/* amt_infio */
};

struct wlc_btc_param_vars_entry {
	uint16 index;
	uint16 value;
};

struct wlc_btc_param_vars_info {
	bool flags_present;
	uint16 flags;
	uint16 num_entries;
	struct wlc_btc_param_vars_entry param_list[0];
};

/* antsel module specific state */
struct antsel_info {
	wlc_info_t *wlc;		/* pointer to main wlc structure */
	wlc_pub_t *pub;			/* pointer to public fn */
	uint8	antsel_type;		/* Type of boardlevel mimo antenna switch-logic
					 * 0 = N/A, 1 = 2x4 board, 2 = 2x3 CB2 board
					 */
	uint8 antsel_antswitch;		/* board level antenna switch type */
	bool antsel_avail;		/* Ant selection availability (SROM based) */
	wlc_antselcfg_t antcfg_11n;	/* antenna configuration */
	wlc_antselcfg_t antcfg_cur;	/* current antenna config (auto) */
};

/* Common tx header info struct -- accomodate AC and pre-AC */
typedef struct wlc_txh_info {
	uint32 Tstamp;
	uint16 TxFrameID;
	uint16 MacTxControlLow;
	uint16 MacTxControlHigh;
	uint16 PhyTxControlWord0;
	uint16 PhyTxControlWord1;
	uint16 PhyTxControlWord2;
	union {
		/* for corerev < 40 */
		uint16 ABI_MimoAntSel;
		/* for corerev >= 40 */
		uint16 FbwInfo;
	} w;
	uint16 d11FrameSize;
	uint hdrSize;
	wlc_txd_t* hdrPtr;
	void* d11HdrPtr;
	void* tsoHdrPtr;
	uint tsoHdrSize;
	uint8* TxFrameRA;
	uint8* plcpPtr;
	uint16 seq;
} wlc_txh_info_t;

/* definitions and structure used to retrieve txpwrs */
#define BW20_IDX        0
#define BW40_IDX        1
#define BW80_IDX        2
#define TXBF_OFF_IDX    0
#define TXBF_ON_IDX     1
typedef struct txpwr204080 {
	uint8 pbw[BW_80MHZ - BW_20MHZ+1][2];
} txpwr204080_t;

#define WLC_HT_WEP_RESTRICT	0x01 	/* restrict HT with WEP */
#define WLC_HT_TKIP_RESTRICT	0x02 	/* restrict HT with TKIP */

#ifdef PROP_TXSTATUS
#define HOST_PROPTXSTATUS_ACTIVATED(wlc) \
		(!!((wlc)->wlfc_flags & WLFC_FLAGS_HOST_PROPTXSTATUS_ACTIVE))
#endif /* PROP_TXSTATUS */

/* DMATXRC Context */
#ifdef DMATXRC

#define TXRCF_RECLAIMED		0x00000001	/* pkt chained was reclaimed */
#define TXRCF_WLHDR_VALID	0x00000002	/* proptxstatus wlhdrs saved */

/* Get TXRC Pkt Ctxt */
#define TXRC_PKTTAIL(osh, p)	((txrc_ctxt_t *)((uint8 *)PKTDATA((osh), (p)) + PKTLEN((osh), (p))))

#define TXRC_ISRECLAIMED(t)	((t)->flags & TXRCF_RECLAIMED)
#define TXRC_CLRRECLAIMED(t)	((t)->flags &= ~TXRCF_RECLAIMED)
#define TXRC_SETRECLAIMED(t)	((t)->flags |= TXRCF_RECLAIMED)

#define TXRC_ISWLHDR(t)		((t)->flags & TXRCF_WLHDR_VALID)
#define TXRC_CLRWLHDR(t)	((t)->flags &= ~TXRCF_WLHDR_VALID)
#define TXRC_SETWLHDR(t)	((t)->flags |= TXRCF_WLHDR_VALID)

#define TXRC_ISMARKER(t)
#define TXRC_SETMARKER(t)

/*
 * This is to support pkt chaining with TX early reclaim. Without pkt chaining,
 * it's one-to-one pairing for phdr and pkt so all information from pkt
 * is already preserved in phdr when pkt is reclaimed. However, with pkt chaining,
 * one phdr can include more than one pkts.  This txrc_ctxt_t is appended to phdr to preserve
 * all wl_hdr_information in pkt chain to be sent back to host at txstatus time.
 * This is particularly important when pkts are suppressed by d11.
 */
typedef struct
{
	uint16 flags;

#if defined(PROP_TXSTATUS) && defined(PKTC_TX_DONGLE)
	uint8  reserve;	/* pad for alignment */
	uint8  n;

	uint32 wlhdr[1];
#endif

#if defined(PROP_TXSTATUS) && defined(PKTC_TX_DONGLE)
	uint16 seq[1];
#endif

} txrc_ctxt_t;
#endif /* DMATXRC */

#define WLC_RFAWARE_LIFETIME_DEFAULT	800	/* default rfaware_lifetime unit: 256us */

#define WLC_EXPTIME_END_TIME	10000	/* exit exptime state if we get frames without expiration
					 * after last expired frame (ms)
					 */

#define	BCM256QAM_DSAB(wlc)	(((wlc_info_t*)(wlc))->pub->boardflags & BFL_DIS_256QAM)

/* BTCX architecture after M93 days  */
#define BT3P_HW_COEX(wlc)  D11REV_GE(((wlc_info_t*)(wlc))->pub->corerev, 15)
#define WLC_BSSCFG_IDX_INVALID (-1)

#define IS_BTCX_FULLTDM(mode) ((mode == WL_BTC_FULLTDM) || (mode == WL_BTC_PREMPT))
/*
 * Conversion between HW and SW BSS indexes.  HW is (currently) based on lower
 * bits of BSSID/MAC address.  SW is based on allocation function.
 * BSS does not need to be up, so caller should check if required.  No error checking.
 * For the reverse map, use WLC_BSSCFG_UCIDX(cfg)
 */
#if defined(MBSS)
#define WLC_BSSCFG_HW2SW_IDX(wlc, uc_idx) ((int)((wlc)->hw2sw_idx[(uc_idx)]))
#else /* !MBSS */
#define WLC_BSSCFG_HW2SW_IDX(wlc, uc_idx) 0
#endif /* MBSS */

#define WLC_ACKSUPR(wlc) (wlc->addrmatch_info)
/*
 * Under MBSS, a pre-TBTT interrupt is generated.  The driver puts beacons in
 * the ATIM fifo at that time and tells uCode about pending BC/MC packets.
 * The delay is settable thru uCode.  MBSS_PRE_TBTT_DEFAULT_us is the default
 * setting for this value.
 * If the driver experiences significant latency, it must avoid setting up
 * beacons or changing the SHM FID registers.  The "max latency" setting
 * indicates the maximum permissible time between the TBTT interrupt and the
 * DPC to respond to the interrupt before the driver must abort the TBTT
 * beacon operations.
 */
#define MBSS_PRE_TBTT_DEFAULT_us 5000		/* 5 milliseconds! */
#define MBSS_PRE_TBTT_MAX_LATENCY_us 4000
#define MBSS_PRE_TBTT_MIN_THRESH_us 1000 /* 1 msec threshold before actual TBTT */

/* Default pre tbtt time for non mbss case */
#define	PRE_TBTT_DEFAULT_us		2

/* Time in usec for PHY enable */
#define	PHY_ENABLE_MAX_LATENCY_us	3000
#define	PHY_DISABLE_MAX_LATENCY_us	3000

/* Maximum time to start roaming */
#define MAX_ROAM_TIME_THRESH	2000

/* Association use of Scan Cache */
#ifdef WLSCANCACHE
#define ASSOC_CACHE_ASSIST_ENAB(wlc)	((wlc)->_assoc_cache_assist)
#else
#define ASSOC_CACHE_ASSIST_ENAB(wlc)	(0)
#endif

/* feature compile time enable/disable and runtime user enable/disable */
#define WLC_TXC_ENAB(wlc)	1

#define	CHANNEL_BANDUNIT(wlc, ch) (((ch) <= CH_MAX_2G_CHANNEL) ? BAND_2G_INDEX : BAND_5G_INDEX)
#define	CHANNEL_BAND(wlc, ch) (((ch) <= CH_MAX_2G_CHANNEL) ? WLC_BAND_2G : WLC_BAND_5G)
#define CHSPEC_BANDUNIT(chspec) (CHSPEC_IS2G(chspec) ? BAND_2G_INDEX : BAND_5G_INDEX)

#define	OTHERBANDUNIT(wlc) ((uint)(((wlc)->band->bandunit == BAND_5G_INDEX)? \
						BAND_2G_INDEX : BAND_5G_INDEX))

#define VALID_BAND(wlc, band)	((band == WLC_BAND_AUTO) || (band == WLC_BAND_2G) || \
				 (band == WLC_BAND_5G))

#define IS_MBAND_UNLOCKED(wlc) \
	((NBANDS(wlc) > 1) && !(wlc)->bandlocked)

#ifdef WLC_LOW
#define WLC_BAND_PI_RADIO_CHANSPEC wlc_phy_chanspec_get(wlc->band->pi)
#else
#define WLC_BAND_PI_RADIO_CHANSPEC (wlc->chanspec)
#endif

/* increment counter */
#define WLCNINC(cntr)
#define WLCNINCN(cntr, n)

#ifdef WLCNT
/* Increment interface stat */
#define WLCIFCNTINCR(_scb, _stat)						\
	do {									\
		if (_scb) {							\
			if (SCB_WDS(_scb)) {					\
				WLCNTINCR(_scb->wds->_cnt._stat);		\
			} else {						\
				wlc_bsscfg_t *_bsscfg = SCB_BSSCFG(_scb);	\
				if (_bsscfg)					\
					WLCNTINCR(_bsscfg->wlcif->_cnt._stat);	\
			}							\
		}								\
	} while (0)
#else
#define WLCIFCNTINCR(_scb, _stat)  do {} while (0)
#endif /* WLCNT */

#define AID2PVBMAP(aid)	(aid & 0x3fff)
#define AID2AIDMAP(aid)	((aid & 0x3fff) - 1)
/* set the 2 MSBs of the AID */
#define AIDMAP2AID(pos)	((pos + 1) | 0xc000)

/* sum the individual fifo tx pending packet counts */
#if defined(WLC_HIGH_ONLY)
#include <wlc_rpctx.h>
#define TXPKTPENDTOT(wlc)		(wlc_rpctx_txpktpend((wlc)->rpctx, 0, TRUE))
#define TXPKTPENDGET(wlc, fifo)		(wlc_rpctx_txpktpend((wlc)->rpctx, (fifo), FALSE))
#define TXPKTPENDINC(wlc, fifo, val)	(wlc_rpctx_txpktpendinc((wlc)->rpctx, (fifo), (val)))
#define TXPKTPENDDEC(wlc, fifo, val)	(wlc_rpctx_txpktpenddec((wlc)->rpctx, (fifo), (val)))
#define TXPKTPENDCLR(wlc, fifo)		(wlc_rpctx_txpktpendclr((wlc)->rpctx, (fifo)))
#define TXAVAIL(wlc, fifo)		(wlc_rpctx_txavail((wlc)->rpctx, (fifo)))
#define GETNEXTTXP(wlc, _queue)		(wlc_rpctx_getnexttxp((wlc)->rpctx, (_queue)))

#else
#define	TXPKTPENDTOT(wlc) ((wlc)->core->txpktpend[0] + (wlc)->core->txpktpend[1] + \
	(wlc)->core->txpktpend[2] + (wlc)->core->txpktpend[3])
#define TXPKTPENDGET(wlc, fifo)		((wlc)->core->txpktpend[(fifo)])
#define TXPKTPENDINC(wlc, fifo, val)	((wlc)->core->txpktpend[(fifo)] += (val))
#define TXPKTPENDDEC(wlc, fifo, val)	((wlc)->core->txpktpend[(fifo)] -= (val))
#define TXPKTPENDCLR(wlc, fifo)		((wlc)->core->txpktpend[(fifo)] = 0)
#define TXAVAIL(wlc, fifo)		(*(wlc)->core->txavail[(fifo)])
#define GETNEXTTXP(wlc, fifo) (!PIO_ENAB((wlc)->pub) ? \
	 dma_getnexttxp(WLC_HW_DI(wlc, fifo), HNDDMA_RANGE_TRANSMITTED) : \
	 wlc_pio_getnexttxp(WLC_HW_PIO(wlc, fifo)))
#endif /* WLC_HIGH_ONLY */

#define WLC_IS_MATCH_SSID(wlc, ssid1, ssid2, len1, len2) \
	((void)(wlc), (len1) == (len2) && !bcmp((ssid1), (ssid2), (len1)))

#if defined(WLPKTDLYSTAT) || defined(WL11K)
/* Macro used to get the current local time (in us) */
#ifdef WLC_HIGH_ONLY
#define WLC_GET_CURR_TIME(wlc)		(OSL_SYSUPTIME())
#else
#define WLC_GET_CURR_TIME(wlc)		(R_REG((wlc)->osh, &(wlc)->regs->tsf_timerlow))
#endif
#endif /* WLPKTDLYSTAT || WL11K */

#define D11_TXH_LEN_EX(wlc) (D11REV_GE((wlc)->pub->corerev, 40) ? D11AC_TXH_LEN : \
	                                             D11_TXH_LEN + D11_PHY_HDR_LEN)
#define D11_TXH_SHORT_LEN(wlc) (WLCISACPHY(wlc->band) ? D11AC_TXH_SHORT_LEN : \
		D11_TXH_LEN + D11_PHY_HDR_LEN)

/*
 * Latency Tolerance Reporting (LTR) time tolerance
 */
#define LTR_ACTIVE_LATENCY			60	/* in us, max 1023 */
#define LTR_ACTIVE_IDLE_LATENCY		100 /* in us, max 1023 */
#define LTR_SLEEP_LATENCY			3	/* in ms, max 1023 */

#define LTR_MAX_ACTIVE_LATENCY		1023	/* in us */
#define LTR_MAX_SLEEP_LATENCY		1023	/* in ms */

/*
 * Number of queued packets on host FIFO that trigger
 * an LTR active (hi) or sleep (lo) transition
 */
#define LTR_HI_WATERMARK			50
#define LTR_LO_WATERMARK			20

#define LTR_MAX_HI_WATERMARK		64
#define LTR_MIN_LO_WATERMARK		10

/*
 * Unit of latency stored in LTR registers
 * 2 => latency in microseconds
 * 4 => latency in milliseconds
 */
#define LTR_SCALE_US				2
#define LTR_SCALE_MS				4

/* Adjustment in TxPwrMin advertised to push it slightly above the minimum supported */
#define WLC_MINTXPWR_OFFSET		4

extern void wlc_srvsdb_reset_engine(wlc_info_t *wlc);
extern void wlc_srvsdb_switch_ppr(wlc_info_t *wlc, chanspec_t new_chanspec, bool last_chan_saved,
	bool switched);

/* API shared by both WLC_HIGH and WLC_LOW driver */
extern void wlc_high_dpc(wlc_info_t *wlc, uint32 macintstatus);
extern void wlc_fatal_error(wlc_info_t *wlc);
extern void wlc_bmac_rpc_watchdog(wlc_info_t *wlc);
extern void wlc_recv(wlc_info_t *wlc, void *p);
extern bool wlc_dotxstatus(wlc_info_t *wlc, tx_status_t *txs, uint32 frm_tx2);
extern void wlc_recover_pkts(wlc_info_t *wlc, uint queue);
extern uint16 wlc_wme_get_frame_medium_time(wlc_info_t *wlc, ratespec_t ratespec,
	uint8 preamble_type, uint mac_len);
extern void wlc_get_txh_info(wlc_info_t* wlc, void* pkt, wlc_txh_info_t* tx_info);
extern void wlc_set_txh_info(wlc_info_t* wlc, void* pkt, wlc_txh_info_t* tx_info);
extern void wlc_txh_set_aging(wlc_info_t *wlc, void *hdr, bool enable);

extern bool wlc_vht_is_40MHz(wlc_info_t* wlc, void* p, struct scb* scb);
extern bool wlc_vht_is_SGI(wlc_info_t* wlc, void* p, struct scb* scb);
extern uint8 wlc_vht_get_mcs(wlc_info_t* wlc, void* p, struct scb* scb);

extern chanspec_t wlc_txh_get_chanspec(wlc_info_t* wlc, wlc_txh_info_t* tx_info);
extern void wlc_txh_set_chanspec(wlc_info_t* wlc, wlc_txh_info_t* tx_info, chanspec_t new_chanspec);

extern bool wlc_txh_get_isAMPDU(wlc_info_t* wlc, wlc_txh_info_t* txh_info);
extern void wlc_txh_set_epoch(wlc_info_t* wlc, wlc_txh_info_t* txh_info, uint8 epoch);
uint8 wlc_txh_get_epoch(wlc_info_t* wlc, wlc_txh_info_t* txh_info);
extern int wlc_pkt_get_vht_hdr(wlc_info_t* wlc, void* p, d11actxh_t **hdrPtrPtr);
extern struct dot11_header* wlc_pkt_get_d11_hdr(wlc_info_t* wlc, void *p);
extern chanspec_t wlc_txh_get_chanspec(wlc_info_t* wlc, wlc_txh_info_t* tx_info);
extern void wlc_pkt_set_ack(wlc_info_t* wlc, void* p, bool want_ack);
extern void wlc_pkt_set_core(wlc_info_t* wlc, void* p, uint8 core);
extern void wlc_pkt_set_txpwr_offset(wlc_info_t* wlc, void* p, uint16 pwr_offset);

extern bool wlc_txh_get_isSGI(wlc_info_t* wlc, wlc_txh_info_t* txh_info);
extern bool wlc_txh_get_isSTBC(wlc_info_t* wlc, wlc_txh_info_t* txh_info);
extern uint8 wlc_txh_info_get_mcs(wlc_info_t* wlc, wlc_txh_info_t* txh_info);

extern void wlc_txfifo(wlc_info_t *wlc, uint fifo, void *p, wlc_txh_info_t* tx_info,
	bool commit, int8 txpktpend);
extern int wlc_txs_create(wlc_info_t *wlc, uint status_bits, tx_status_macinfo_t *status);
extern tx_status_macinfo_t* wlc_txs_create_noack(tx_status_macinfo_t* txstatus);
extern uint16 wlc_txs_alias_to_old_fmt(wlc_info_t *wlc, tx_status_macinfo_t* status);
#ifdef WL_MULTIQUEUE
extern void wlc_tx_fifo_sync_complete(wlc_info_t *wlc, uint fifo_bitmap, uint8 flag);
#endif
extern void wlc_txfifo_complete(wlc_info_t *wlc, uint fifo, uint16 txpktpend);
#ifdef PROP_TXSTATUS
extern void wlc_txfifo_suppress(wlc_info_t *wlc, struct scb* pmq_scb);
#endif
extern bool wlc_should_retry_suppressed_pkt(wlc_info_t *wlc, void *p, uint status);
extern void wlc_info_init(wlc_info_t* wlc, int unit);
extern uint16 wlc_rcvfifo_limit_get(wlc_info_t *wlc);
extern void wlc_print_txstatus(wlc_info_t *wlc, tx_status_t* txs);
extern void wlc_write_template_ram(wlc_info_t *wlc, int offset, int len, void *buf);
extern void wlc_write_hw_bcntemplates(wlc_info_t *wlc, void *bcn, int len, bool both);
extern void wlc_xtal(wlc_info_t* wlc, bool want);
extern void wlc_txflowcontrol_reset_qi(wlc_info_t *wlc, wlc_txq_info_t *qi);
extern wlc_txq_info_t* wlc_txq_alloc(wlc_info_t *wlc, osl_t *osh);
extern void wlc_txq_free(wlc_info_t *wlc, osl_t *osh, wlc_txq_info_t *qi);
extern void wlc_mute(wlc_info_t *wlc, bool on, mbool flags);
extern void wlc_read_tsf(wlc_info_t* wlc, uint32* tsf_l_ptr, uint32* tsf_h_ptr);
extern void wlc_set_cwmin(wlc_info_t *wlc, uint16 newmin);
extern void wlc_set_cwmax(wlc_info_t *wlc, uint16 newmax);
extern void wlc_fifoerrors(wlc_info_t *wlc);
extern void wlc_lq_rssi_init(wlc_info_t *wlc, int rssi);
extern int8 wlc_lq_rssi_pktrxh_cal(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh);
extern int  wlc_lq_recv_snr_compute(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh, int noise_lvl);
extern void wlc_lq_noise_sample_request(wlc_info_t *wlc, uint8 request, uint8 channel);
extern void wlc_lq_noise_cb(wlc_info_t *wlc, uint8 channel, int8 noise_dbm);
extern void wlc_lq_channel_qa_sample_req(wlc_info_t *wlc);
extern void wlc_ampdu_mode_upd(wlc_info_t *wlc, uint8 mode);
extern void wlc_pllreq(wlc_info_t *wlc, bool set, mbool req_bit);
extern void wlc_update_phy_mode(wlc_info_t *wlc, uint32 phy_mode);
extern void wlc_reset_bmac_done(wlc_info_t *wlc);
extern void wlc_gptimer_wake_upd(wlc_info_t *wlc, mbool requestor, bool set);
extern void wlc_user_wake_upd(wlc_info_t *wlc, mbool requestor, bool set);
extern uint16 wlc_acphy_txctl0_calc_ex(wlc_info_t *wlc, ratespec_t rspec, uint8 preamble);
extern uint16 wlc_acphy_txctl1_calc_ex(wlc_info_t *wlc, ratespec_t rspec,
	int8 lpc_offset, uint8 txpwr);
extern uint16 wlc_acphy_txctl2_calc_ex(wlc_info_t *wlc, ratespec_t rspec);

#ifdef WLPKTDLYSTAT
extern void wlc_delay_stats_upd(scb_delay_stats_t *delay_stats, uint32 delay, uint tr,
	bool ack_recd);
#ifdef WLPKTDLYSTAT_IND
extern void wlc_delay_stats_watchdog(wlc_info_t *wlc);
#endif /* WLPKTDLYSTAT_IND */
#endif /* WLPKTDLYSTAT */

#if defined(WLMSG_PRHDRS)
extern void wlc_recv_print_rxh(wlc_d11rxhdr_t *wrxh);
extern void wlc_print_hdrs(wlc_info_t *wlc, const char *prefix, uint8 *frame,
	wlc_txd_t *txd, wlc_d11rxhdr_t *wrxh, uint len);
extern void wlc_print_txdesc(wlc_info_t *wlc, wlc_txd_t *txd);
extern void wlc_print_txdesc_ac(wlc_info_t *wlc, void* hdrsBegin);
#endif
#if defined(WLMSG_PRHDRS) || defined(WLMSG_PRPKT) || defined(WLMSG_ASSOC)
extern void wlc_print_dot11_mac_hdr(uint8* buf, int len);
#endif

#ifdef STA
#ifdef WLRM
extern void wlc_rm_cca_complete(wlc_info_t *wlc, uint32 cca_idle_us);
extern bool wlc_rm_rpi_sample(rm_info_t *wlc, int8 rssi);
#endif
#endif /* STA */

#ifdef MBSS
extern bool wlc_bmac_ucodembss_hwcap(wlc_hw_info_t *wlc_hw);
extern void wlc_shm_ssid_get(wlc_info_t *wlc, int idx, wlc_ssid_t *ssid);
#else
#define wlc_bmac_ucodembss_hwcap(a) 0
#define wlc_shm_ssid_get(wlc, idx, ssid) do {} while (0)
#endif

#ifdef WLC_LOW
extern void wlc_setxband(wlc_hw_info_t *wlc_hw, uint bandunit);
extern void wlc_coredisable(wlc_hw_info_t* wlc_hw);
extern void wlc_rxfifo_setpio(wlc_hw_info_t *wlc_hw);
#endif

#define wlc_log(wlc, str, p1, p2)	do {} while (0)

/* API in HIGH only or monolithic driver */
#ifdef WLC_HIGH
extern wlc_if_t *wlc_wlcif_alloc(wlc_info_t *wlc, osl_t *osh, uint8 type, wlc_txq_info_t *qi);
extern void wlc_wlcif_free(wlc_info_t *wlc, osl_t *osh, wlc_if_t *wlcif);
extern void wlc_lq_rssi_snr_noise_reset_ma(wlc_info_t *wlc, wlc_bsscfg_t *cfg, int nval,
	int snr, int noise);
extern void wlc_lq_rssi_reset_ma(wlc_bsscfg_t *cfg, int nval);
extern int  wlc_lq_rssi_update_ma(wlc_bsscfg_t *cfg, int nval, int qdb, bool unicast);
extern void wlc_lq_rssi_event_update(wlc_bsscfg_t *cfg);
extern int  wlc_lq_snr_update_ma(wlc_bsscfg_t *cfg, int nval, bool unicast);
extern int  wlc_lq_noise_update_ma(wlc_info_t *wlc, int nval);
extern int  wlc_lq_noise_lte_update_ma(wlc_info_t *wlc, int nval);
extern void wlc_lq_update_tbtt(wlc_bsscfg_t *cfg);
#ifdef CCA_STATS
extern bool wlc_lq_cca_chan_qual_event_update(wlc_info_t *wlc, uint8 id, int v);
#endif
extern void wlc_link(wlc_info_t *wlc, bool isup, struct ether_addr *addr, wlc_bsscfg_t *bsscfg,
	uint reason);
extern int wlc_scan_request_ex(wlc_info_t *wlc, int bss_type, const struct ether_addr* bssid,
                               int nssid, wlc_ssid_t *ssids, int scan_type, int nprobes,
                               int active_time, int passive_time, int home_time,
                               const chanspec_t* chanspec_list, int chanspec_num,
                               chanspec_t chanspec_start, bool save_prb,
                               scancb_fn_t fn, void* arg,
                               int macreq, uint scan_flags, wlc_bsscfg_t *cfg,
                               actcb_fn_t act_cb, void *act_cb_arg);
#define wlc_scan_request(wlc, bss_type, bssid, nssid, ssid, scan_type, nprobes, \
		active_time, passive_time, home_time, chanspec_list, chanspec_num, \
		save_prb, fn, arg) \
	wlc_scan_request_ex(wlc, bss_type, bssid, nssid, ssid, scan_type, nprobes, \
		active_time, passive_time, home_time, chanspec_list, chanspec_num, 0, \
		save_prb, fn, arg, WLC_ACTION_SCAN, FALSE, NULL, NULL, NULL)
extern void wlc_set_ssid(wlc_info_t *wlc, uchar SSID[], int len);
extern uint8 wlc_bss_psscb_getcnt(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

#ifdef WLSCANCACHE
extern bool wlc_assoc_cache_validate_timestamps(wlc_info_t *wlc, wlc_bss_list_t *bss_list);
#endif /* WLSCANCACHE */

extern bool wlc_valid_rate(wlc_info_t *wlc, ratespec_t rate, int band, bool verbose);
extern void wlc_do_chanswitch(wlc_bsscfg_t *cfg, chanspec_t newchspec);
extern void *wlc_senddisassoc(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
	const struct ether_addr *da, const struct ether_addr *bssid, const struct ether_addr *sa,
	uint16 reason_code);
extern int wlc_remove_wpa2_pmkid(wlc_info_t *wlc, bcm_tlv_t *wpa2ie);
extern void *wlc_sendassocreq(wlc_info_t *wlc, wlc_bss_info_t *bi, struct scb *scb, bool reassoc);
extern void wlc_BSSinit(wlc_info_t *wlc, wlc_bss_info_t *bi, wlc_bsscfg_t *cfg, int start);
extern void wlc_full_phy_cal(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint8 reason);
extern void wlc_qosinfo_update(struct scb *scb, uint8 qosinfo, bool ac_upd);
extern void wlc_ht_upd_coex_state(wlc_bsscfg_t *cfg, uint8 detected);
extern void wlc_ht_coex_filter_scanresult(wlc_bsscfg_t *cfg);
extern void wlc_ht_coex_exclusion_range(wlc_info_t *wlc, uint8 *ch_min,
                                        uint8 *ch_max, uint8* ch_ctl);
#ifdef WL11N
extern void wlc_ht_obss_scanparams_hostorder(wlc_info_t *wlc, obss_params_t *param,
	bool host_order);
extern void wlc_ht_obss_scanparam_init(obss_params_t *params);
extern void wlc_ht_coex_enab(wlc_bsscfg_t *cfg, int8 setting);
#else
#define wlc_ht_obss_scanparams_hostorder(a, b, c)  do {(void)(a);} while (0)
#define wlc_ht_obss_scanparam_init(a) do {} while (0)
#define wlc_ht_coex_enab(a, b) do {} while (0)
#endif /* WL11N */

extern chanspec_t wlc_ht_chanspec(wlc_info_t *wlc, uint8 chan, uint8 extch);

extern int wlc_duty_cycle_set(wlc_info_t *wlc, int duty_cycle, bool isOFDM, bool writeToShm);

extern obss_params_t *wlc_ht_read_obss_scanparams_ie(wlc_info_t *wlc, uint8 *tlv, int tlv_len);

extern void wlc_ht_build_cap_ie(wlc_bsscfg_t *cfg, ht_cap_ie_t *cap_ie,
	uint8* sup_mcs, bool is2G);
extern uint8* wlc_write_brcm_ht_cap_ie(wlc_info_t *wlc, uint8 *cp, int buflen, ht_cap_ie_t *cap_ie);
extern uint8* wlc_write_brcm_ht_add_ie(wlc_info_t *wlc, uint8 *cp, int buflen, ht_add_ie_t *add_ie);
extern void wlc_ht_update_scbstate(wlc_info_t *wlc, struct scb *scb,
	ht_cap_ie_t *cap_ie, ht_add_ie_t *add_ie, obss_params_t *obss_ie);
extern int wlc_parse_rates(wlc_info_t *wlc, uchar *tlvs, uint tlvs_len, wlc_bss_info_t *bi,
	wlc_rateset_t *rates);
extern int wlc_combine_rateset(wlc_info_t *wlc, wlc_rateset_t *sup, wlc_rateset_t *ext,
	wlc_rateset_t *rates);
extern void *wlc_sendauth(wlc_bsscfg_t *cfg, struct ether_addr *ea, struct ether_addr *bssid,
	struct scb *scb, int auth_alg, int auth_seq, int auth_status, wsec_key_t *scb_key,
	uint8 *challenge_text, bool short_preamble);
extern void wlc_scb_disassoc_cleanup(wlc_info_t *wlc, struct scb *scb);
extern void wlc_cwmin_gphy_update(wlc_info_t *wlc, wlc_rateset_t *rs, bool associated);
extern int wlc_edcf_acp_apply(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool suspend);
extern uchar wlc_assocscb_getcnt(wlc_info_t *wlc);
extern uchar wlc_bss_assocscb_getcnt(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
extern int wlc_abo(wlc_info_t *wlc, int val);
extern void wlc_ap_upd(wlc_info_t* wlc,  wlc_bsscfg_t *bsscfg);
extern int wlc_minimal_up(wlc_info_t* wlc);
extern int wlc_minimal_down(wlc_info_t* wlc);

/* helper functions */
extern bool wlc_rateset_isofdm(uint count, uint8 *rates);
extern void wlc_remove_all_keys(wlc_info_t *wlc);
extern void wlc_tx_suspend(wlc_info_t *wlc);
extern bool wlc_tx_suspended(wlc_info_t *wlc);
extern void wlc_tx_resume(wlc_info_t *wlc);
extern void wlc_rateset_show(wlc_info_t *wlc, wlc_rateset_t *rs, struct ether_addr *ea);
extern void wlc_shm_ssid_upd(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

extern void wlc_rateprobe(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct ether_addr *ea,
	ratespec_t rate_override);
extern void *wlc_senddeauth(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
	struct ether_addr *da, struct ether_addr *bssid, struct ether_addr *sa,
	uint16 reason_code);
extern void wlc_mac_event(wlc_info_t* wlc, uint msg, const struct ether_addr* addr,
	uint result, uint status, uint auth_type, void *data, int datalen);
extern void wlc_bss_mac_event(wlc_info_t* wlc, wlc_bsscfg_t *bsscfg, uint msg,
	const struct ether_addr* addr, uint result, uint status, uint auth_type,
	void *data, int datalen);
extern void wlc_bss_mac_rxframe_event(wlc_info_t* wlc, wlc_bsscfg_t *bsscfg, uint msg,
	const struct ether_addr* addr, uint result, uint status, uint auth_type,
	void *data, int datalen, wl_event_rx_frame_data_t *rxframe_data);
extern void wlc_recv_prep_event_rx_frame_data(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh, uint8 *plcp,
                                         wl_event_rx_frame_data_t *rxframe_data);

#define PROBE_REQ_EVT_MASK	0x01	/* For eventing */
#define PROBE_REQ_DPT_MASK	0x02	/* For Direct Packet Transfer(DPT) */
#define PROBE_REQ_PRMS_MASK	0x04	/* For promiscous mode */
#define PROBE_REQ_PROBRESP_MASK	0x08	/* For SW Probe Response */
#define PROBE_REQ_PROBRESP_P2P_MASK	0x10	/* For P2P SW Probe Response */

extern void wlc_enable_probe_req(wlc_info_t *wlc, uint32 mask, uint32 val);

#define PROBE_RESP_P2P_MASK      1	/* For P2P */
#define PROBE_RESP_SW_MASK       2	/* For SW Probe Response */
#define PROBE_RESP_BSD_MASK      4	/* For FreeBSD/NetBSD */

extern void wlc_disable_probe_resp(wlc_info_t *wlc, uint32 mask, uint32 val);

#if !defined(WLNOEIND)
extern void wlc_bss_eapol_event(wlc_info_t *wlc, wlc_bsscfg_t * bsscfg,
                                const struct ether_addr *ea, uint8 *data, uint32 len);
extern void wlc_bss_eapol_wai_event_indicate(wlc_info_t *wlc, void *p, struct scb *scb,
                                wlc_bsscfg_t *bsscfg, struct ether_header *eh);
#endif /* !defined(WLNOEIND) */

extern void wlc_ibss_disable(wlc_bsscfg_t *cfg);
extern void wlc_ibss_enable(wlc_bsscfg_t *cfg);
extern void wlc_ibss_disable_all(wlc_info_t *wlc);
extern void wlc_ibss_enable_all(wlc_info_t *wlc);

extern bool wlc_sendnulldata(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct ether_addr *da,
	uint rate_override, uint32 pktflags, int prio);
extern bool wlc_sendnulldata_ex(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct ether_addr *da,
	uint rate_override, uint32 pktflags, int prio,
	void (*pre_send_fn)(wlc_info_t*wlc, wlc_bsscfg_t*cfg, void*p, void*data), void *data);
extern void *wlc_nulldata_template(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct ether_addr *da);

extern bool wlc_sendapsdtrigger(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
extern int wlc_wsec(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint32 val);
extern uint8 wlc_wpa_mcast_cipher(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
extern int wlc_set_gmode(wlc_info_t *wlc, uint8 gmode, bool config);

extern void wlc_mac_bcn_promisc(wlc_info_t *wlc);
extern void wlc_mac_promisc(wlc_info_t *wlc);
extern void wlc_sendprobe(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	const uint8 ssid[], int ssid_len,
	const struct ether_addr *da, const struct ether_addr *bssid,
	ratespec_t rate_override, uint8 *extra_ie, int extra_ie_len);
extern void wlc_txflowcontrol(wlc_info_t *wlc, wlc_txq_info_t *qi, bool on, int prio);
extern void wlc_txflowcontrol_override(wlc_info_t *wlc, wlc_txq_info_t *qi, bool on, uint override);
extern bool wlc_txflowcontrol_prio_isset(wlc_info_t *wlc, wlc_txq_info_t *qi, int prio);
extern bool wlc_txflowcontrol_override_isset(wlc_info_t *wlc, wlc_txq_info_t *qi, uint override);
extern void wlc_send_q(wlc_info_t *wlc, wlc_txq_info_t *qi);
extern void wlc_send_active_q(wlc_info_t *wlc);
extern int wlc_prep_sdu(wlc_info_t *wlc, void **sdu, int *nsdu, uint *fifo);
extern int wlc_prep_sdu_fast(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct scb *scb,
	void *sdu, uint *fifop);
extern int wlc_prep_pdu(wlc_info_t *wlc, void *pdu, uint *fifo);
extern bool wlc_send80211_prepped(wlc_info_t *wlc, wlc_if_t *wlcif, void *p);
extern uint16 wlc_prep80211_raw(wlc_info_t *wlc, wlc_if_t *wlcif,
	uint ac, bool data_only, void *p, uint *outfifo);

extern void wlc_send_action_err(wlc_info_t *wlc, struct dot11_management_header *hdr,
	uint8 *body, int body_len);

extern void *wlc_frame_get_mgmt(wlc_info_t *wlc, uint16 fc, const struct ether_addr *da,
	const struct ether_addr *sa, const struct ether_addr *bssid, uint body_len,
	uint8 **pbody);
/* variant of above when iv and tail are needed - e.g. MFP, CCX */
extern void* wlc_frame_get_mgmt_ex(wlc_info_t *wlc, uint16 fc,
	const struct ether_addr *da, const struct ether_addr *sa,
	const struct ether_addr *bssid, uint body_len, uint8 **pbody,
	uint iv_len, uint tail_len);

extern void *wlc_frame_get_action(wlc_info_t *wlc, uint16 fc, const struct ether_addr *da,
	const struct ether_addr *sa, const struct ether_addr *bssid, uint body_len,
	uint8 **pbody, uint8 cat);
extern void *wlc_frame_get_ctl(wlc_info_t *wlc, uint len);
extern bool wlc_sendmgmt(wlc_info_t *wlc, void *p, wlc_txq_info_t *qi, struct scb *scb);
extern bool wlc_queue_80211_frag(wlc_info_t *wlc, void *p, wlc_txq_info_t *qi, struct scb *scb,
	wlc_bsscfg_t *bsscfg, bool preamble, wsec_key_t *key, ratespec_t rate_or);
extern bool wlc_sendctl(wlc_info_t *wlc, void *p, wlc_txq_info_t *qi, struct scb *scb, uint fifo,
	ratespec_t rate_or, bool enq_only);
extern void wlc_recvdata_ordered(wlc_info_t *wlc, struct scb *scb, struct wlc_frminfo *f);
extern uint16 wlc_calc_lsig_len(wlc_info_t *wlc, ratespec_t ratespec, uint mac_len);
extern ratespec_t wlc_rspec_to_rts_rspec(wlc_bsscfg_t *cfg, ratespec_t rspec, bool use_rspec);
extern ratespec_t wlc_rspec_to_rts_rspec_ex(wlc_info_t *wlc, ratespec_t rspec, bool use_rspec,
	bool g_protection);
extern uint16 wlc_compute_rtscts_dur(wlc_info_t *wlc, bool cts_only, ratespec_t rts_rate,
	ratespec_t frame_rate, uint8 rts_preamble_type, uint8 frame_preamble_type,
	uint frame_len, bool ba);
extern uint16 wlc_compute_frame_dur(wlc_info_t *wlc, ratespec_t rate, uint8 preamble_type,
	uint next_frag_len);

extern void wlc_tbtt(wlc_info_t *wlc, d11regs_t *regs);
extern void wlc_bss_tbtt(wlc_bsscfg_t *cfg);

extern void wlc_dump_ucode_fatal(wlc_info_t *wlc);

extern bool wlc_ps_check(wlc_info_t *wlc);
extern void wlc_reprate_init(wlc_info_t *wlc);
extern void wlc_exptime_start(wlc_info_t *wlc);
extern void wlc_exptime_check_end(wlc_info_t *wlc);
extern int wlc_rfaware_lifetime_set(wlc_info_t *wlc, uint16 lifetime);

#ifdef STA
extern void wlc_pspoll_timer(void *arg);
extern void wlc_apsd_trigger_timeout(void *arg);
extern void wlc_set_pmoverride(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_pmpending(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_pspoll(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_pmawakebcn(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_apsd_stausp(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_uatbtt(wlc_bsscfg_t *cfg, bool state);
extern void wlc_update_bcn_info(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_dtim_programmed(wlc_bsscfg_t *cfg, bool state);
extern void wlc_update_pmstate(wlc_bsscfg_t *cfg, uint txstatus);
extern void wlc_module_set_pmstate(wlc_bsscfg_t *cfg, bool state, mbool moduleId);
extern void wlc_set_pmstate(wlc_bsscfg_t *cfg, bool state);
extern void wlc_reset_pmstate(wlc_bsscfg_t *cfg);
extern void wlc_pm_pending_complete(wlc_info_t *wlc);
extern void wlc_pm2_sleep_ret_timer_start(wlc_bsscfg_t *cfg);
extern void wlc_pm2_sleep_ret_timer_stop(wlc_bsscfg_t *cfg);
extern void wlc_pm2_ret_upd_last_wake_time(wlc_bsscfg_t *cfg, uint32 *tsf_l);
extern int wlc_set_pm_mode(wlc_info_t *wlc, int val, wlc_bsscfg_t *bsscfg);
extern void wlc_set_pmenabled(wlc_bsscfg_t *cfg, bool state);
extern int wlc_pm_bcnrx_set(wlc_info_t *wlc, bool enable);
extern void wlc_pm_bcnrx_disable(wlc_info_t *wlc);
extern bool wlc_pm_bcnrx_allowed(wlc_info_t *wlc);

#if defined(WL_PM2_RCV_DUR_LIMIT)
extern void wlc_pm2_rcv_reset(wlc_bsscfg_t *cfg);
#else
#define wlc_pm2_rcv_reset(cfg)
#endif
extern uint8 wlc_stas_connected(wlc_info_t *wlc);
extern bool wlc_ibss_active(wlc_info_t *wlc);
extern bool wlc_stas_active(wlc_info_t *wlc);
extern bool wlc_bssid_is_current(wlc_bsscfg_t *cfg, struct ether_addr *bssid);
extern bool wlc_bss_connected(wlc_bsscfg_t *cfg);
extern bool wlc_portopen(wlc_bsscfg_t *cfg);
extern void wlc_rm_meas_end(wlc_info_t* wlc);
extern void wlc_rm_start(wlc_info_t* wlc);
extern void wlc_rm_stop(wlc_info_t* wlc);
extern int wlc_rm_abort(wlc_info_t* wlc);

#else
#define wlc_stas_active(a) 0
#define wlc_set_pmstate(a, b)
#endif	/* STA */

/* Shared memory access */
#ifdef WLC_HIGH
extern void wlc_write_shm(wlc_info_t *wlc, uint offset, uint16 v);
extern void wlc_update_shm(wlc_info_t *wlc, uint offset, uint16 v, uint16 mask);
extern uint16 wlc_read_shm(wlc_info_t *wlc, uint offset);
extern void wlc_set_shm(wlc_info_t *wlc, uint offset, uint16 v, int len);
extern void wlc_copyto_shm(wlc_info_t *wlc, uint offset, const void* buf, int len);
extern void wlc_copyfrom_shm(wlc_info_t *wlc, uint offset, void* buf, int len);
#endif

extern bool wlc_down_for_mpc(wlc_info_t *wlc);

extern void wlc_update_beacon(wlc_info_t *wlc);
extern void wlc_bss_update_beacon(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

extern void wlc_update_probe_resp(wlc_info_t *wlc, bool suspend);
extern void wlc_bss_update_probe_resp(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool suspend);
extern void wlc_bcn_prb_template(wlc_info_t *wlc, uint type,
	ratespec_t bcn_rate, wlc_bsscfg_t *cfg, uint16 *buf, int *len);
extern void wlc_bcn_prb_body(wlc_info_t *wlc, uint type, wlc_bsscfg_t *cfg,
	uint8 *bcn, int *bcn_len, bool legacy_tpl);
extern int wlc_recv_parse_bcn_prb(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh,
	struct ether_addr *bssid, bool beacon,
	uint8 *body, uint body_len, wlc_bss_info_t *bi);

#ifdef STA
extern bool wlc_ismpc(wlc_info_t *wlc);
extern void wlc_radio_mpc_upd(wlc_info_t* wlc);

/* MPC off requiest because P2P is active */
#define MPC_OFF_REQ_P2P_APP_ACTIVE (1 << 0)

extern int wlc_mpc_off_req_set(wlc_info_t *wlc, mbool mask, mbool val);

#endif /* STA */
extern int
wlc_bss2wl_bss(wlc_info_t *wlc, wlc_bss_info_t *bi, wl_bss_info_t *to_bi, int to_bi_len,
	bool need_ies);

extern void wlc_radio_upd(wlc_info_t* wlc);
extern bool wlc_prec_enq(wlc_info_t *wlc, struct pktq *q, void *pkt, int prec);
extern bool wlc_prec_enq_head(wlc_info_t *wlc, struct pktq *q, void *pkt, int prec, bool head);
extern bool wlc_pkt_abs_supr_enq(wlc_info_t *wlc, struct scb *scb, void *pkt);
extern void wlc_pktq_supr_norm(wlc_info_t *wlc, struct pktq *pktq);
extern void wlc_pktq_scb_free(wlc_info_t *wlc, struct pktq *pktq, struct scb *remove);

extern void wlc_rateset_elts(wlc_info_t *wlc, wlc_bsscfg_t *cfg, const wlc_rateset_t *rates,
	wlc_rateset_t *sup_rates, wlc_rateset_t *ext_rates);
extern uint16 wlc_phytxctl1_calc(wlc_info_t *wlc, ratespec_t rspec, chanspec_t chanspec);

extern void wlc_compute_plcp(wlc_info_t *wlc, ratespec_t rate, uint length, uint16 fc, uint8 *plcp);
extern void wlc_prb_resp_plcp_hdrs(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, wlc_prq_info_t *info,
	int length, uint8 *plcp, d11txh_t *txh, uint8 *d11_hdr);
extern uint wlc_calc_frame_time(wlc_info_t *wlc, ratespec_t ratespec,
	uint8 preamble_type, uint mac_len);
extern void wlc_processpmq(wlc_info_t *wlc);

#define WLC_RX_CHANNEL(rxh)		(CHSPEC_CHANNEL((rxh)->RxChan))

#ifdef WIFI_ACT_FRAME
extern int wlc_send_action_frame(wlc_info_t *wlc, wlc_bsscfg_t *cfg, const struct ether_addr *bssid,
	void *action_frame);
extern int wlc_send_action_frame_off_channel(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	uint32 channel, int32 dwell_time, struct ether_addr *bssid,
	wl_action_frame_t *action_frame);
extern int wlc_tx_action_frame_now(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt);
extern void wlc_af_off_channel_callback(void * arg, int status, wlc_bsscfg_t *cfg);
extern void wlc_af_send(wlc_info_t *wlc, void *arg, uint *dwell);
#endif

extern void wlc_set_chanspec(wlc_info_t *wlc, chanspec_t chanspec);
#ifdef WL_MULTIQUEUE
extern void wlc_excursion_start(wlc_info_t *wlc);
extern void wlc_excursion_end(wlc_info_t *wlc);
extern void wlc_primary_queue_set(wlc_info_t *wlc, wlc_txq_info_t *new_primary);
extern void wlc_active_queue_set(wlc_info_t *wlc, wlc_txq_info_t *new_active);
#else
#define wlc_excursion_start(wlc)	((void)(wlc))
#define wlc_excursion_end(wlc)		((void)(wlc))
#define wlc_primary_queue_set(wlc, qi)  ((void)(wlc), (void)(qi))
#define wlc_active_queue_set(wlc, qi)   ((void)(wlc), (void)(qi))
#endif /* WL_MULTIQUEUE */

extern bool wlc_timers_init(wlc_info_t* wlc, int unit);

extern bool wlc_update_brcm_ie(wlc_info_t *wlc);
extern bool wlc_bss_update_brcm_ie(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

extern void wlc_recvdata(wlc_info_t *wlc, osl_t *osh, wlc_d11rxhdr_t *wrxh, void *p);
extern void wlc_recvdata_sendup(wlc_info_t *wlc, struct scb *scb, bool wds,
	struct ether_addr *da, void *pkt);
extern void wlc_sendup(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb, void *pkt);
extern bool wlc_sendsdu(wlc_info_t *wlc, void *sdu);

extern void wlc_ether_8023hdr(wlc_info_t *wlc, osl_t *osh, struct ether_header *eh, void *p);
extern void wlc_8023_etherhdr(wlc_info_t *wlc, osl_t *osh, void *p);
extern void* wlc_hdr_proc(wlc_info_t *wlc, void *sdu, struct scb *scb);

extern uint16 wlc_sdu_etype(wlc_info_t *wlc, void *sdu);
extern uint8* wlc_sdu_data(wlc_info_t *wlc, void *sdu);

extern int wlc_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int val_size, wlc_if_t *wlcif);
extern int wlc_bssiovar_mkbuf(char *iovar, int bssidx, void *param,
	int paramlen, void *bufptr, int buflen, int *perr);
extern void wlc_if_event(wlc_info_t *wlc, uint8 what, wlc_if_t *wlcif);
#if defined(WLMSG_INFORM)
extern void wlc_print_event(wlc_info_t* wlc, wlc_event_t *e);
#endif


#define wlc_validate_bcn_phytxctl(wlc, cfg) do {} while (0)
extern void * wlc_sdu_to_pdu(wlc_info_t *wlc, void *sdu, struct scb *scb, bool is_8021x);

#if defined(WLMSG_PRPKT)
extern void wlc_print_ies(wlc_info_t *wlc, uint8 *ies, uint ies_len);
#endif

#ifdef WL11N
extern int wlc_set_nmode(wlc_info_t *wlc, int32 nmode);
extern void wlc_ht_mimops_cap_update(wlc_info_t *wlc, uint8 mimops_mode);
extern void wlc_mimops_action_ht_send(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint8 mimops_mode);
extern void *wlc_send_action_ht_mimops(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint8 mimops_mode);
#endif

extern void wlc_txmod_fn_register(wlc_info_t *wlc, scb_txmod_t feature_id, void *ctx,
                                  txmod_fns_t fns);
extern void wlc_txmod_config(wlc_info_t *wlc, struct scb *scb, scb_txmod_t fid);
extern void wlc_txmod_unconfig(wlc_info_t *wlc, struct scb *scb, scb_txmod_t fid);

extern void wlc_custom_scan_complete(void *arg, int status, wlc_bsscfg_t *cfg);
extern void wlc_switch_shortslot(wlc_info_t *wlc, bool shortslot);
#ifndef WLC_NET80211
extern void wlc_set_bssid(wlc_bsscfg_t *cfg);
extern void wlc_clear_bssid(wlc_bsscfg_t *cfg);
#endif
extern void wlc_bcn_tsf_diff(wlc_bsscfg_t *cfg, wlc_d11rxhdr_t *wrxh, void *plcp,
	struct dot11_bcn_prb *bcn, int32 *diff_h, int32 *diff_l);
extern int wlc_bcn_tsf_later(wlc_bsscfg_t *cfg, wlc_d11rxhdr_t *wrxh, void *plcp,
	struct dot11_bcn_prb *bcn);
extern void wlc_tsf_adopt_bcn(wlc_bsscfg_t *cfg, wlc_d11rxhdr_t *wrxh, uint8 *plcp,
	struct dot11_bcn_prb *bcn);
extern void wlc_tsf_adj(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint32 tgt_h, uint32 tgt_l,
	uint32 ref_h, uint32 ref_l, uint32 nxttbtt, uint32 bcnint, bool comp);
extern bool wlc_sendpspoll(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

extern int wlc_sta_info(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
                        const struct ether_addr *ea, void *buf, int len);
extern void wlc_print_plcp(char *str, uchar *plcp);

/* structures and functions to iterate over current hw rateset */
typedef struct wlc_hwrs_iterator_t
{
	const wlc_rateset_t *rs;
	int cur_idx;
} wlc_hwrs_iterator;

extern bool
wlc_hwrs_iterator_finished(wlc_hwrs_iterator *hwrs_walker);

extern uint8
wlc_hwrs_iterator_next(wlc_hwrs_iterator *hwrs_walker);

extern void
wlc_hwrs_iterator_init(wlc_info_t *wlc, wlc_hwrs_iterator *hwrs_walker);

extern void wlc_set_ratetable(wlc_info_t *wlc);
#ifndef WLC_NET80211
extern int wlc_set_mac(wlc_bsscfg_t *cfg);
#else
extern int wlc_set_mac(wlc_info_t *wlc, struct ether_addr *addr);
#endif
extern void wlc_clear_mac(wlc_bsscfg_t *cfg);
extern void wlc_beacon_phytxctl_txant_upd(wlc_info_t *wlc, ratespec_t bcn_rate);
extern void wlc_beacon_phytxctl(wlc_info_t *wlc, ratespec_t bcn_rspec, chanspec_t chanspec);
extern void wlc_mod_prb_rsp_rate_table(wlc_info_t *wlc, uint frame_len);
extern ratespec_t wlc_lowest_basic_rspec(wlc_info_t *wlc, wlc_rateset_t *rs);
extern void wlc_write_hwbcntemplate(wlc_info_t *wlc, void *bcn, int len, bool both);
extern uint16 wlc_compute_bcn_payloadtsfoff(wlc_info_t *wlc, ratespec_t rspec);
extern uint16 wlc_compute_bcntsfoff(wlc_info_t *wlc, ratespec_t rspec,
	bool short_preamble, bool phydelay);
extern bool wlc_radio_disable(wlc_info_t *wlc);
extern bool wlc_nonerp_find(wlc_info_t *wlc, void *body, uint body_len, uint8 **erp, int *len);
extern bool wlc_erp_find(wlc_info_t *wlc, void *body, uint body_len, uint8 **erp, int *len);
extern void wlc_bcn_li_upd(wlc_info_t *wlc);
extern chanspec_t wlc_create_chspec(struct wlc_info *wlc, uint8 channel);

#if defined(WLPKTENG)
extern void* wlc_tx_testframe(wlc_info_t *wlc, struct ether_addr *da,
	struct ether_addr *sa, ratespec_t rate_override, int length);
#endif

extern uint16 wlc_pretbtt_calc(wlc_bsscfg_t *cfg);
extern void wlc_pretbtt_set(wlc_bsscfg_t *cfg);
extern void wlc_dtimnoslp_set(wlc_bsscfg_t *cfg);
#ifndef WLC_NET80211
extern void wlc_macctrl_init(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
#else
extern void wlc_macctrl_init(wlc_info_t *wlc);
#endif
extern uint32 wlc_recover_tsf32(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh);
extern void wlc_recover_tsf64(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh, uint32 *tsf_h, uint32 *tsf_l);
extern int wlc_get_revision_info(wlc_info_t *wlc, void *buf, uint len);
#ifndef BCMNODOWN
extern void wlc_out(wlc_info_t *wlc);
#endif /* BCMNODOWN */
extern int wlc_bandlock(struct wlc_info *, int val);
extern int wlc_set_ratespec_override(wlc_info_t *wlc, int band_id, ratespec_t rspec, bool mcast);
extern void wlc_pcie_war_ovr_update(wlc_info_t *wlc, uint8 aspm);
extern void wlc_pcie_power_save_enable(wlc_info_t *wlc, bool enable);

#if defined(STA) && defined(AP)
extern bool wlc_apup_allowed(wlc_info_t *wlc);
#endif /* STA && AP */

extern void wlc_lifetime_set(wlc_info_t *wlc, void *sdu, uint32 lifetime);

extern void wlc_set_home_chanspec(wlc_info_t *wlc, chanspec_t chanspec);

extern void wlc_update_bandwidth(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	chanspec_t bss_chspec);
extern chanspec_t wlc_get_cur_wider_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
#ifdef STA
extern void wlc_bss_clear_bssid(wlc_bsscfg_t *cfg);
extern void wlc_freqtrack_reset(wlc_info_t *wlc);
extern void wlc_watchdog_upd(wlc_bsscfg_t *cfg, bool tbtt);
extern int wlc_pspoll_timer_upd(wlc_bsscfg_t *cfg, bool allow);
extern int wlc_apsd_trigger_upd(wlc_bsscfg_t *cfg, bool allow);
extern bool wlc_ps_allowed(wlc_bsscfg_t *cfg);
extern bool wlc_stay_awake(wlc_info_t *wlc);
extern void wlc_handle_ap_lost(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
#endif /* STA */

#ifdef PROP_TXSTATUS
#ifdef WLFCTS
extern void wlc_process_wlfc_dbg_update(wlc_info_t *wlc, uint8 dbg_type, void* p);
#endif /* WLFCTS */
extern void wlc_process_wlhdr_complete_txstatus(wlc_info_t *wlc, uint8 status_flag,
	void* p, tx_status_t *txs, bool hold);
extern void wlc_process_wlhdr_txstatus(wlc_info_t *wlc, uint8 status_flag, void* p, bool hold);
extern bool wlc_suppress_sync_fsm(wlc_info_t *wlc, struct scb* sc, void* p, bool suppress);
extern uint8 wlc_wlfc_d11_txstatus_interpret(void *status);
#endif /* PROP_TXSTATUS */

#ifdef WME
extern void wlc_wme_initparams_sta(wlc_info_t *wlc, wme_param_ie_t *pe);
extern int wlc_wme_downgrade_fifo(wlc_info_t *wlc, uint* p_fifo, struct scb *scb);
#endif
extern uint wlc_wme_wmmac_check_fixup(wlc_info_t *wlc, struct scb *scb, void *p,
	uint8 prio, int *bcmerror);

#ifdef WL11N
extern void wlc_ht_obss_scan_reset(wlc_bsscfg_t *cfg);
#else
#define wlc_ht_obss_scan_reset(a)	do {} while (0)
#endif


extern void wlc_bss_list_free(wlc_info_t *wlc, wlc_bss_list_t *bss_list);
extern void wlc_bss_list_xfer(wlc_bss_list_t *from, wlc_bss_list_t *to);

extern bool wlc_ifpkt_chk_cb(void *p, int idx);

extern void wlc_process_event(wlc_info_t *wlc, wlc_event_t *e);
extern int wlc_find_nominal_req_pwr(ratespec_t rspec);

extern void wlc_txh_iv_upd(wlc_info_t *wlc, d11txh_t *txh, uint8 *iv, wsec_key_t *key, bool uc_seq);

#endif /* WLC_HIGH */

#if defined(WLMCHAN) || defined(SRHWVSDB)
void wlc_tsf_adjust(wlc_info_t *wlc, int delta);
#endif 
#ifdef STA
#if defined(AP_KEEP_ALIVE)
extern void wlc_ap_keep_alive_count_update(wlc_info_t *wlc, uint16 keep_alive_time);
extern void wlc_ap_keep_alive_count_default(wlc_info_t *wlc);
#endif 
#endif /* STA */

#ifdef WLTXMONITOR
extern void wlc_tx_monitor(wlc_info_t *wlc, d11txh_t *txh, tx_status_t *txs, void *p,
                    struct wlc_if *wlcif);
#endif
#if defined(WLC_LOW) && defined(DMATXRC)
extern int wlc_phdr_attach(wlc_info_t *wlc);
extern void wlc_phdr_detach(wlc_info_t *wlc);
extern void wlc_phdr_fill(wlc_info_t* wlc);
extern void* wlc_phdr_get(wlc_info_t *wlc);
#endif

#ifdef WLRXOV
extern void wlc_rxov_timer(void *arg);
#endif
#ifdef WLC_HIGH
extern uint16 wlc_assoc_capability(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, wlc_bss_info_t *bi);
extern void wlc_get_sup_ext_rates(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, wlc_rateset_t *sup_rates,
	wlc_rateset_t *ext_rates);
extern void wlc_tdls_reinsert_pkt(wlc_info_t *wlc, wlc_bsscfg_t *parent, struct scb *scb, void *p);
extern void wlc_txstop_intr(wlc_info_t *wlc);
extern void wlc_rfd_intr(wlc_info_t *wlc);
#endif /* WLC_HIGH */

#ifdef PKTQ_LOG
typedef struct pktq * pktq_get_fn_t(wlc_info_t*, struct scb*);
void wlc_pktq_stats_free(wlc_info_t* wlc, struct pktq* q);
#ifdef AP
struct pktq * wlc_apps_prec_pktq(wlc_info_t* wlc, struct scb* scb);
#endif
#ifdef WLAMPDU
struct pktq * scb_ampdu_prec_pktq(wlc_info_t* wlc, struct scb* scb);
#endif
#endif /* PKTQ_LOG */

#if defined(IBSS_PEER_GROUP_KEY)
uint16 wlc_read_seckindxblk_secinfo_by_idx(wlc_info_t *wlc, uint index);
#endif 
void wlc_write_seckindxblk_secinfo_by_idx(wlc_info_t *wlc, uint index, uint16 val);
uint16 wlc_read_amtinfo_by_idx(wlc_info_t *wlc, uint index);
void wlc_write_amtinfo_by_idx(wlc_info_t *wlc, uint index, uint16 val);


#define WLC_AMPDU_TXQ_PROF_ADD_ENTRY(wlc, scb)

extern bool wlc_force_ht(wlc_info_t *wlc, bool force, bool *prev);
extern void wlc_ref_tick_us_update(wlc_info_t *wlc);
extern uint wlc_ref_tick_to_us(wlc_info_t *wlc, uint ticks);
extern uint wlc_us_to_ref_tick(wlc_info_t *wlc, uint us);
extern uint32 wlc_current_pmu_time(void);

#if defined(WLC_LOW) && defined(WLC_HIGH) && defined(MCHAN_MINIDUMP)
extern void chanswitch_history(wlc_info_t *wlc, chanspec_t from, chanspec_t to,
	uint32 start, uint32 end, int context);
#endif
extern void wlc_ampdu_upd_pm(wlc_info_t *wlc, uint8 PM_mode);

extern void wlc_BSSinit_rateset_filter_ex(wlc_bsscfg_t *cfg);
extern uint8 wlc_get_mcsallow(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
#if defined(PKTC) || defined(PKTC_DONGLE)
extern bool wlc_rxframe_chainable(wlc_info_t *wlc, void **pp, uint16 index);
extern void wlc_sendup_chain(wlc_info_t *wlc, void *p);
extern void wlc_pktc_sdu_prep(wlc_info_t *wlc, struct scb *scb, void *p,
	void *n, uint32 lifetime);
#ifdef DMATXRC
extern void* wlc_pktc_sdu_prep_phdr(wlc_info_t *wlc, struct scb *scb, void *p, uint32 lifetime);
#endif
#endif /* PKTC || PKTC_DONGLE */


#if defined(WLOFFLD) || defined(SCANOL)
/* This function tell whether the BK tid
* seq/iv needs to be
* copied from shared memory (frmShm)
*/
extern void wlc_update_seq_iv(wlc_info_t *wlc, bool frmShm, struct scb *scb);
#endif

extern uint16 wlc_mgmt_ctl_d11hdrs(wlc_info_t *wlc, void *pkt, struct scb *scb,
	uint queue, ratespec_t rspec_override);

/* check field of specified length from ptr is valid for pkt */
extern bool wlc_valid_pkt_field(osl_t *osh, void *pkt, void *ptr, int len);

extern uint16 bcmc_fid_generate(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint16 txFrameID);

#ifdef STA
#ifdef WL_EXCESS_PMWAKE
extern void wlc_generate_pm_alert_event(wlc_info_t *wlc, uint32 reason, void *data, uint32 datalen);
extern void wlc_check_roam_alert_thresh(wlc_info_t *wlc);
#endif /* WL_EXCESS_PMWAKE */
#ifdef WL_CONNECTION_STATS
extern void wlc_connect_time_upd(wlc_info_t *wlc);
extern void wlc_connect_start(wlc_info_t *wlc);
extern uint32 wlc_curr_connect_time(wlc_info_t *wlc);
#endif /* WL_CONNECTION_STATS */
#endif /* STA */

/* Enumerated Keys used in WLC OBJECT REGISTRY */

typedef enum wlc_obj_registry_key {
	WLC_OBJR_MODULE_ID,	/* Stores wlc->modulecb */
	WLC_OBJR_BSSCFG_PTR,
	WLC_OBJR_CLM_PTR,
	WLC_OBJR_AMPDUTX_SHARED,
	WLC_OBJR_MAX_KEYS
} wlc_obj_registry_key_t;

/* See if we want to add a wl_bss_info_t to an event */
extern wlc_bss_info_t* wlc_event_needs_bssinfo_tlv(uint msg, wlc_bsscfg_t *bsscfg);
extern uint32 wlc_get_accum_pmdur(wlc_info_t *wlc);
extern uint32 wlc_get_mpc_dur(wlc_info_t *wlc);
extern void wlc_cfg_set_pmstate_upd(wlc_bsscfg_t *cfg, bool pmenabled);
extern void wlc_check_txq_fc(wlc_info_t *wlc, wlc_txq_info_t *qi);

/* find the maximum bandwidth channel that we can use from advertised channel */
extern chanspec_t wlc_max_valid_bw_chanspec(wlc_info_t *wlc, wlcband_t *band,
	wlc_bsscfg_t *cfg, chanspec_t chanspec);

#ifdef BCMSPLITRX
typedef struct wlc_eapol_pktfetch_ctx {
	wlc_info_t *wlc;
	struct scb *scb;
	wlc_frminfo_t f;
	bool ampdu_path;
	struct pktfetch_info *pinfo;
} wlc_eapol_pktfetch_ctx_t;

/* Probably need to move this definition out of here */
static inline struct dot11_llc_snap_header*
wlc_get_llc_snap_header(uchar *pbody, uint32 wsec, bool protected)
{
	if (protected) {
		if (WSEC_WEP_ENABLED(wsec))
			return (struct dot11_llc_snap_header*)(pbody + DOT11_IV_LEN);
		else if (WSEC_TKIP_ENABLED(wsec))
			return (struct dot11_llc_snap_header*)(pbody + DOT11_IV_TKIP_LEN);
		else if (WSEC_AES_ENABLED(wsec))
			return (struct dot11_llc_snap_header*)(pbody + DOT11_IV_AES_CCM_LEN);
		else
			return (struct dot11_llc_snap_header*)pbody;
	} else
		return ((struct dot11_llc_snap_header*)pbody);
}

#define EAPOL_PKTFETCH_REQUIRED(osh, lsh, p) \
	(lsh->dsap == 0xaa && \
			lsh->ssap == 0xaa && \
			lsh->ctl == 0x03 && \
			lsh->oui[0] == 0 && \
			lsh->oui[1] == 0 && \
			lsh->oui[2] == 0 && \
			ntoh16(lsh->type) == ETHER_TYPE_802_1X && \
			PKTFRAGUSEDLEN(osh, p) > 0)

extern void wlc_eapol_schedule_pktfetch(wlc_info_t *wlc, struct scb *scb,
	wlc_frminfo_t *f, bool promisc_frame);
extern void wlc_eapol_pktfetch_cb(void *lbuf, void *orig_lfrag, void *ctx, bool cancelled);
extern void wlc_recreate_frameinfo(wlc_info_t *wlc, void *lbuf, void *lfrag,
	wlc_frminfo_t *fold, wlc_frminfo_t *fnew);
#endif /* BCMSPLITRX */

/* PSM watchdog debug info recording structure */
struct psm_watchdog_debug_st {
	uint32 phydebug;
	uint32 psmdebug;
	uint32 maccontrol;
	uint16 psm_brc;
	uint16 psm_brc_1;
	uint16 wepctl;
	uint16 pcmctl;
	uint16 pcmstat;
	uint16 wep_psdulen;
	uint16 txe_status;
	uint16 phytxerror;
	uint16 ifsstat;
	uint16 txe_ctl;
	uint16 timer_val;
	uint16 psmdebug_loop[64];
	uint16 ucode_rev;
	uint16 rxe_rx_cntr;
	uint16 plcp0;
	uint16 plcp1;
	uint16 fc;
	uint16 rxe_status1;
	uint16 rxe_status2;
	uint16 daggctl;
	uint16 bytesleft;
	uint16 shoffs;
	uint16 daggstat;
	uint16 dagglen;
	uint16 m_cts_duration;
	uint16 m_ucode_dbgst;
};

#if defined(WLAWDL)
extern void wlc_update_phy_chan_cache(wlc_bsscfg_t *cfg, chanspec_t new_infra_chan, bool join);
#endif /* #if defined(WLAWDL) */

#ifdef EVENT_LOG_COMPILE
#define WL_PSM_WD(arg) EVENT_PSM_WD_ arg
#define EVENT_PSM_WD_(args...)  EVENT_LOG(EVENT_LOG_TAG_UCODE_WATCHDOG, args)

#else
#define WL_PSM_WD       WL_PRINT
#define EVENT_DUMP(a, b, c)    WL_PRPKT("PSM DUMP", b, c)
#endif /* EVENT_LOG_COMPILE */

extern int wlc_link_txflow_scb(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 flowid, uint8 op,
	uint8 * sa, uint8 *da, uint8 tid);
extern uint32 wlc_current_pmu_time(void);
extern void wlc_cap_bcmstrbuf(wlc_info_t *wlc, struct bcmstrbuf *b);

extern int wlc_assoc_chanspec_sanitize(wlc_info_t *wlc, chanspec_list_t *list, int len);

extern int wlc_assoc_dc_handle_regisiter(wlc_pub_t *pub,
	const wlc_dc_handle_t *dc_handle, int size);
extern void wlc_assoc_dc_dispatch(wlc_info_t *wlc,
	wlc_bsscfg_t *cfg, assoc_decision_t *dc, struct scb *scb);
extern int wlc_assoc_dc_handle_deregisiter(wlc_pub_t *pub,
	const wlc_dc_handle_t *dc_handle, int size);
#endif	/* _wlc_h_ */
