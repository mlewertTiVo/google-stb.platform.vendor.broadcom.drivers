/*
 * Common (OS-independent) definitions for
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wlc.h 645630 2016-06-24 23:27:55Z $
 */

#ifndef _wlc_h_
#define _wlc_h_

#include <wlc_types.h>
#include "wlc_cfg.h"
#include "wlc_pub.h"
#include <bcmlimits.h>
#include <wl_dbg.h>
#include <wlioctl.h>
#include <wlc_rate.h>
#include <proto/vlan.h>
#include <wlc_pio.h>
#include <wlc_phy_hal.h>
#include <wlc_iocv_types.h>
#include <wlc_msfblob.h>
#include <wlc_channel.h>
#include <bcm_notif_pub.h>
#include <bcm_mpool_pub.h>
#if defined(TXQ_MUX)
#include <wlc_mux.h>
#include <wlc_scbq.h>
#endif /* TXQ_MUX */
#include <d11.h>
#ifdef WLCXO_CTRL
#include <wlc_cxo_ctrl.h>
#endif


/*
 * defines for join pref processing
 */

#define WLC_JOIN_PREF_LEN_FIXED		2 /* Length for the fixed portion of Join Pref TLV value */

#define JOIN_RSSI_BAND		WLC_BAND_5G	/* WLC_BAND_AUTO disables the feature */
#define JOIN_RSSI_DELTA		10		/* Positive value */
#define JOIN_PREF_IOV_LEN	8		/* 4 bytes each for RSSI Delta and mandatory RSSI */

/* Maximum value of the duration that can be sent when using DCF */
#define MAX_DCF_DURATION 32767
#define MAX_MAC_ENABLE_RETRIES 100             /* Max retries to enable MAC before sending CTS */

#define MAX_MCHAN_CHANNELS 4

#define PKTPRIO_NON_QOS_DEFAULT		-1
#define PKTPRIO_NON_QOS_HIGH		-2

/* consider an IBSS as HW enabled if it uses hardware beacon (even if hardware
 * probe response is disabled), hence leaving the option of software probe
 * response open for later IBSS applications.
 */
#define IBSS_HW_ENAB(cfg)	(BSSCFG_IBSS(cfg) && HWBCN_ENAB(cfg))

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

#define MODULE_DETACH(var, detach_func)\
	if (var) { \
		detach_func(var); \
		(var) = NULL; \
	}
#define MODULE_DETACH_TYPECASTED(var, detach_func) detach_func(var)

/* Flags to identify SYNC operation vs FLUSH operation required for Nintendo2 and P2P */
#define SYNCFIFO	(0)
#define FLUSHFIFO	(1)
#define FLUSHFIFO_FLUSHID		(2) /* Flush on flowid match */
#define SUPPRESS_FLUSH_FIFO		(3) /* Suppress host packets
	                                 * and flush internal packets
	                                 */

/* Bitmaps */
/* 0x01=BK, 0x02=BE, 0x04=VI, 0x08=VO, 0x10=B/MCAST, 0x20=ATIM */
#define BITMAP_FLUSH_NO_TX_FIFOS	(0)	/* Bitmap - flush no TX FIFOs */
#define BITMAP_FLUSH_ALL_TX_FIFOS	(0x3F)	/* Bitmap - flush all TX FIFOs */
#define BITMAP_SYNC_ALL_TX_FIFOS	(0x3F)	/* Bitmap - sync all TX FIFOs */

#define WLC_WEP_IV_SIZE		4	/* size of WEP1 and WEP128 IV */
#define WLC_WEP_ICV_SIZE	4	/* size of WEP ICV */

#define WLC_ASSOC_RECREATE_BI_TIMEOUT	10 /* bcn intervals to wait during and assoc_recreate */
#define WLC_ASSOC_VERIFY_TIMEOUT	1000 /* ms to wait to allow an AP to respond to class 3
					      * data if it needs to disassoc/deauth
					      */
#define WLC_ADVERTISED_LISTEN	10	/* listen interval specified in (re)assoc request */

#define WLC_TXQ_DRAIN_TIMEOUT	3000	/* txq drain timeout in ms */

#define WLC_BITSCNT(x)	(uint8)bcm_bitcount((uint8 *)&(x), sizeof(uint8))

#define WLC_FRAMEBURST_MIN_RATE WLC_RATE_2M	/* frameburst mode is only enabled above 2Mbps */
#define WLC_BASIC_RATE(wlc, rspec)	((wlc)->band->basic_rate[RSPEC_REFERENCE_RATE(rspec)])

#define	WLC_RATEPROBE_RSPEC	(OFDM_RSPEC(WLC_RATE_6M))	/* 6Mbps ofdm rateprobe rate */

/* Maximum wait time for a MAC suspend */
#define	WLC_MAX_MAC_SUSPEND	83000	/* uS: 83mS is max packet time (64KB ampdu @ 6Mbps) */

/* Probe Response timeout - responses for probe requests older that this are tossed, zero to disable
 */
#define WLC_PRB_RESP_TIMEOUT	0 /* Disable probe response timeout */

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

#define TXHPKTSZ	(BCMEXTRAHDROOM + 2 * ETHER_HDR_LEN + DOT11_LLC_SNAP_HDR_LEN)

/* transmit buffer max headroom for protocol headers */
#define D11_COMMON_HDR	(DOT11_A4_HDR_LEN + DOT11_QOS_LEN + DOT11_IV_MAX_LEN + \
				DOT11_LLC_SNAP_HDR_LEN + VLAN_TAG_LEN + CCX_HEADROOM)
#define D11_TXOFF	(D11_TXH_LEN + D11_PHY_HDR_LEN + D11_COMMON_HDR)
#define D11AC_TXOFF	(D11AC_TXH_LEN + D11_COMMON_HDR + TSO_HEADER_LENGTH)

#define D11_RPC_COMMON_HDR	(2 /* WLC_RPC_TXFIFO_UNALIGN_PAD_2BYTE */ + 4 /* RPC_HDR_LEN */ + \
					4 /* BCM_RPC_TP_ENCAP_LEN */)
#define D11_RPC_TXOFF	(((D11_TXOFF + D11_RPC_COMMON_HDR) + 3) & ~3)
#define D11AC_RPC_TXOFF	(((D11AC_TXOFF + D11_RPC_COMMON_HDR) + 3) & ~3)

#if D11CONF_LT(40) && !D11CONF_GE(40)
#define TXOFF	D11_TXOFF
#else
#define TXOFF	MAX(D11AC_TXOFF, D11_TXOFF)
#endif /* D11CONF_LT(40) && !D11CONF_GE(40) */

#define	SW_TIMER_MAC_STAT_UPD		30	/* periodic MAC stats update */

/* wlc_BSSinit actions */
#define WLC_BSS_JOIN		0 /* join action */
#define WLC_BSS_START		1 /* start action */

#define WLC_BSS_CONNECTED(cfg) \
			((cfg)->associated && (!(cfg)->BSS || !ETHER_ISNULLADDR(&(cfg)->BSSID)))

#define WLC_BSS_ASSOC_NOT_ROAM(cfg) \
			(((cfg)->associated) && (!(cfg)->BSS || \
			!ETHER_ISNULLADDR(&(cfg)->BSSID)) && ((cfg)->assoc->type != AS_ROAM))

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
#define WLC_FULLROAM_PERIOD	70	/* full roam scan Period */

#define WLC_CACHE_INVALID_TIMER	60	/* entries are no longer valid after this # of secs */
#define WLC_UATBTT_TBTT_THRESH	10	/* beacon loss before checking unaligned tbtt */
#define WLC_ROAM_TBTT_THRESH	20	/* beacon loss before roaming attempt */

#define ROAM_FULLSCAN_NTIMES	3
#define ROAM_RSSITHRASHDIFF	5
#define ROAM_CU_THRASH_DIFF	5
#define ROAM_CACHEINVALIDATE_DELTA	7
#define ROAM_CACHEINVALIDATE_DELTA_MAX	0x80	/* Set to force max ci_delta,
						 * and effectively disable ci_delta
						 */

/* join pref rssi delta minimum cutoff
 * The rssi delta will only be applied if the target on the preferred band has an RSSI
 * of at least this minimum value. Below this RSSI value, the preferred band target gets
 * no RSSI preference.
 */
#ifndef WLC_JOIN_PREF_RSSI_BOOST_MIN
#ifdef WLFMC
#define WLC_JOIN_PREF_RSSI_BOOST_MIN	-70	/* Targets must be at least -70dBm to get pref */
#else
#define WLC_JOIN_PREF_RSSI_BOOST_MIN	-65	/* Targets must be at least -65dBm to get pref */
#endif /* WLFMC */
#endif /* WLC_JOIN_PREF_RSSI_BOOST_MIN */

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
#define AS_WAIT_IE		3	/* waiting for association ie */
#define AS_WAIT_IE_TIMEOUT	4	/* TimeOut waiting for assoc ie */
#define AS_JOIN_START		5	/* Join start state */
#define AS_SENT_FTREQ		6	/* Sent FT Req state */
#define AS_RECV_FTRES		7	/* Received FT Resp state */
#define AS_WAIT_TX_DRAIN	8	/* Waiting for tx packets to drain out */
#define AS_SENT_AUTH_1		9	/* Sent Authentication 1 state */
#define AS_SENT_AUTH_2		10	/* Sent Authentication 2 state */
#define AS_SENT_AUTH_3		11	/* Sent Authentication 3 state */
#define AS_SENT_ASSOC		12	/* Sent Association state */
#define AS_REASSOC_RETRY	13	/* Sent Assoc on DOT11_SC_REASSOC_FAIL */
#define AS_JOIN_ADOPT		14	/* Adopting BSS Params */
#define AS_WAIT_RCV_BCN		15	/* Waiting to receive beacon state */
#define AS_SYNC_RCV_BCN		16	/* Waiting Re-sync beacon state */
#define AS_WAIT_DISASSOC	17	/* Waiting to disassociate state */
#define AS_WAIT_PASSHASH	18	/* calculating passhash state */
#define AS_RECREATE_WAIT_RCV_BCN 19	/* wait for former BSS/IBSS bcn in assoc_recreate */
#define AS_ASSOC_VERIFY		20	/* allow former AP time to disassoc/deauth before PS mode */
#define AS_LOSS_ASSOC		21	/* Loss association */
#define AS_JOIN_CACHE_DELAY	22
#define AS_WAIT_FOR_AP_CSA	23	/* Wait for AP to send CSA to clients */
#define AS_WAIT_FOR_AP_CSA_ROAM_FAIL 24	/* Wait for AP to send CSA to clients when roam fails */
#define AS_MODE_SWITCH_START	25
#define AS_MODE_SWITCH_COMPLETE	26
#define AS_MODE_SWITCH_FAILED	27
#define AS_DISASSOC_TIMEOUT	28
#define AS_IBSS_CREATE		29	/* No IBSS peers found, STA will create IBSS */
#define AS_DFS_CAC_START	30	/* DFS Slave CAC begins */
#define AS_DFS_CAC_FAIL		31	/* DFS Slave detected radar during CAC state */
#define AS_DFS_ISM_INIT		32	/* DFS state machine notified to begin ISM */
#define AS_LAST_STATE		33

/* assoc->type values */
#define AS_NONE			0 /* No association */
#define AS_ASSOCIATION		1 /* Join association */
#define AS_ROAM			2 /* Roaming association */
#define AS_RECREATE		3 /* Re-Creating a prior association */
#define AS_VERIFY_TIMEOUT		4 /* Re-Creating Timedout */

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
#define TXFID_QUEUE_MASK        0x001F /* Bits 0-4 */
#define TXFID_SEQ_MASK		0x7F80 /* Bits 7-14 */
#define TXFID_SEQ_SHIFT		7      /* Number of bit shifts */
#define	TXFID_RATE_PROBE_MASK	0x8000 /* Bit 15 for rate probe */
#define TXFID_RATE_MASK		0x0060 /* Mask for bits 5 and 6 */
#define TXFID_RATE_SHIFT	5      /* Shift 5 bits for rate mask */

#define WLC_TXFID_GET_QUEUE(frameID)  ((frameID) & TXFID_QUEUE_MASK)
#define WLC_TXFID_SET_QUEUE(fifo)	((fifo) & TXFID_QUEUE_MASK)

/* promote boardrev */
#define BOARDREV_PROMOTABLE	0xFF	/* from */
#define BOARDREV_PROMOTED	1	/* to */

/* Iterator for all wlcs :  (wlc_cmn_info_t* cmn, int idx) */
#define FOREACH_WLC(wlc_cmn, idx, current_wlc) \
	for ((idx) = 0; (int) (idx) < MAX_RSDB_MAC_NUM; (idx)++) \
		if ((((current_wlc) = wlc_cmn->wlc[(idx)]) != NULL))

/* Iterator for wlc_up. wlc_up needs to be called for each wlc in case of dual-nic-rsdb
 * or NIC rsdb
 */
#define FOREACH_WLC_UP(wlc, idx, current_wlc) \
	for ((idx) = 0; (int) (idx) < MAX_RSDB_MAC_NUM; (idx)++) \
		if ((((current_wlc) = wlc->cmn->wlc[(idx)]) != NULL))

#ifdef STA
/* if wpa is in use then portopen is true when the group key is plumbed otherwise it is always true
 */
#define WLC_PORTOPEN(cfg) \
	(((cfg)->WPA_auth != WPA_AUTH_DISABLED && WSEC_ENABLED((cfg)->wsec)) ? \
	(cfg)->wsec_portopen : TRUE)

#ifdef WLTDLS
extern bool wlc_tdls_pm_allowed(tdls_info_t *tdls, wlc_bsscfg_t *cfg);
#define WLC_TDLS_PM_ALLOWED(wlc, cfg) (TDLS_ENAB((wlc)->pub) ? \
					wlc_tdls_pm_allowed((wlc)->tdls, cfg) : TRUE)
#else
#define WLC_TDLS_PM_ALLOWED(wlc, cfg) TRUE
#endif

#ifdef WLAIBSS
extern bool wlc_aibss_pm_allowed(wlc_aibss_info_t *aibss_info, wlc_bsscfg_t *cfg);
#define WLC_AIBSS_PM_ALLOWED(wlc, cfg) (AIBSS_ENAB((wlc)->pub) ? \
	wlc_aibss_pm_allowed((wlc)->aibss_info, (cfg)) : TRUE)
#else
#define WLC_AIBSS_PM_ALLOWED(wlc, cfg)	TRUE
#endif /* WLAIBSS */

#define BSSCFG_PM_ALLOWED(cfg) (BSSCFG_STA(cfg) && (!BSSCFG_IBSS(cfg) || \
				(BSSCFG_IBSS(cfg) && BSSCFG_IS_AIBSS_PS_ENAB(cfg))))

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

/**
 * The blocks on block_datafifo prevent pkts from flowing onto the DMA ring, but still allow the DMA
 * ring to flow. For instance, DATA_BLOCK_PS is set as an AP when a station goes to PS to prevent
 * any more pkts to go to the ring while still allowing the ring and DMA txfifo drain out, and to
 * allow the ucode to suppress any pkts that were queued for the STA that entered PS mode.
 */
#define DATA_BLOCK_CHANSW	(1 << 0)
#define DATA_BLOCK_QUIET	(1 << 1) /* only stop tx'ing data pkts, not management or ACKs */
#define DATA_BLOCK_JOIN		(1 << 2)
#define DATA_BLOCK_PS		(1 << 3)
#define DATA_BLOCK_TX_SUPR	(1 << 4) /* wlc_sendq() will block if DATA_BLOCK_TX_SUPR is set */
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

#ifndef	AMPDU_BA_MAX_WSIZE
/* max Tx/Rx ba window size (in pdu) for array allocations in structures. */
#define	AMPDU_BA_MAX_WSIZE	64
#endif

#define	MAXTXPKTS		6		/* max # pkts pending */
#define	MAXTXPKTS_AMPDUMAC	64
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

#ifdef STA
/* PM2 tick time in milliseconds and gptimer units */
#define WLC_PM2_TICK_MS	10
#define WLC_PM2_MAX_MS	2000
#define WLC_PM2_TICK_GP(ms)	((ms) * 1024)
#endif	/* STA */

#define WLC_MPC_MODE_OFF	0x0	/* turn off mpc */
#define WLC_MPC_MODE_1		0x1	/* mpc with radio off */
#define WLC_MPC_MODE_2		0x2	/* mpc with ucode sleep */

/* PLL requests */
#define WLC_PLLREQ_SHARED	0x1	/* pll is shared on old chips */
#define WLC_PLLREQ_RADIO_MON	0x2	/* hold pll for radio monitor register checking */
#define WLC_PLLREQ_FLIP		0x4	/* hold/release pll for some short operation */

/* Do we support this rate? */
#define VALID_RATE(wlc, rspec) wlc_valid_rate(wlc, rspec, WLC_BAND_AUTO, FALSE)
#define VALID_RATE_DBG(wlc, rspec) wlc_valid_rate(wlc, rspec, WLC_BAND_AUTO, TRUE)

#define WLC_MIMOPS_RETRY_SEND	1
#define WLC_MIMOPS_RETRY_NOACK	2
#define WLC_MIMOPS_RETRY_MULTISEND	4 /* retry logic for multiple action frame */
#define WLC_MIMOPS_RETRY_MAX		3 /* MAX retry count to send mimo ps action */
#define WLC_CH_WIDTH_RETRY_SEND		1
#define WLC_CH_WIDTH_RETRY_NOACK	2
#define WLC_MGMT_RETRY_SEND			1
#define WLC_MGMT_RETRY_NOACK		2
#define WLC_MIMO_UPDATE_MCS			1
#define WLC_MIMO_DONOT_UPDATE_MCS	0

/* Check if any join/roam is being processed */
#ifdef STA
/* Number of beacon-periods */
#ifdef OPPORTUNISTIC_ROAM
#define UATBTT_TO_ROAM_BCN 10
#else
#define UATBTT_TO_ROAM_BCN 2
#endif
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
#define PHDR_POOL(wlc)	((pktpool_t *)NULL)
#define PHDR_POOL_LEN	1
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
#define WLC_ACTION_ACTFRAME		11 /* Action frame off home channel */
#define WLC_ACTION_PNOSCAN		12 /* PNO scan action */
#define WLC_ACTION_LPRIO_EXCRX		13 /* low priority excursion */
#define WLC_ACTION_RTT			14 /* 11mc */

/*
 * Simple macro to change a channel number to a chanspec. Useable until the channels we
 * support overlap in the A and B bands.
 */
#define WLC_CHAN2SPEC(chan)	((chan) <= CH_MAX_2G_CHANNEL ? \
	(uint16)((chan) | WL_CHANSPEC_BAND_2G) : (uint16)((chan) | WL_CHANSPEC_BAND_5G))

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

union wlc_txd {
	d11txh_t d11txh;
	d11actxh_t txd;
	d11axtxh_t d11ax_txd;
};

/* Macros to extract the CacheInfo and RateInfo pointers from a .11ac tx descriptor.
 * CacheInfo is a single structure. RateInfo is an array.
 */
#define WLC_TXD_CACHE_INFO_GET(txd, corerev) \
	(D11REV_GE(corerev, 64) ? &txd->u.rev64.CacheInfo : &txd->u.rev40.CacheInfo)

#define WLC_TXD_RATE_INFO_GET(txd, corerev) \
	(D11REV_GE(corerev, 64) ? txd->u.rev64.RateInfo : txd->u.rev40.RateInfo)

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

#define WLC_STBC_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_STBC) != 0)
#define WLC_SGI_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_SGI) != 0)
#define WLC_40MHZ_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_40MHZ) != 0)
#define WLC_80MHZ_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_80MHZ) != 0)
#define WLC_8080MHZ_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_8080MHZ) != 0)
#define WLC_160MHZ_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_160MHZ) != 0)
#define WLC_LDPC_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_LDPC) != 0)
#define WLC_2P5MHZ_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_2P5MHZ) != 0)
#define WLC_5MHZ_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_5MHZ) != 0)
#define WLC_10MHZ_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_10MHZ) != 0)

#define WLC_VHT_PROP_RATES_CAP_PHY(wlc) \
	((wlc_phy_cap_get(WLC_PI(wlc)) & PHY_CAP_VHT_PROP_RATES) != 0)
#define WLC_HT_PROP_RATES_CAP_PHY(wlc) \
	((wlc_phy_cap_get(WLC_PI(wlc)) & PHY_CAP_HT_PROP_RATES) != 0)

#define WLC_SU_BFR_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_SU_BFR) != 0)
#define WLC_SU_BFE_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_SU_BFE) != 0)
#define WLC_MU_BFR_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_MU_BFR) != 0)
#define WLC_MU_BFE_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_MU_BFE) != 0)
#define WLC_1024QAM_CAP_PHY(w)	((wlc_phy_cap_get(WLC_PI((w))) & PHY_CAP_1024QAM) != 0)

#define WLC_CORENUM_PHY(w)	(phy_utils_get_corenum((phy_info_t *)WLC_PI((w))))
#define WLC_COREMASK_PHY(w)	((1 << WLC_CORENUM_PHY((w))) - 1)

#define WLC_PHY_AS_80P80(wlc, chanspec) \
	(ACREV_IS(wlc->band->phyrev, 33) && \
	(CHSPEC_IS160(chanspec) || CHSPEC_IS8080(chanspec)))

#define WD_BLOCK_REQ_LTECX	0x1

#define WLC_BIT_SET		0
#define WLC_BIT_CLEAR		1
#define MWS_ANTMAP_DEFAULT	0

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
	uint32 thresh; /**< user-thresh for which bcn are considered early, usecs */
	uint32 earliest_offset; /**< offset (from next tbtt) of earliest bcn detected, usecs */
	bool detect_done;
	uint32 assoc_start_time;
	uint32 uatbtt_start_time;
} wlc_early_bcn_detect_t;

/*
 * core state (mac)
 */
typedef struct wlccore {
	uint		coreidx;		/**< # sb enumerated core */

	/* fifo */
	uint		**txavail;		/* # tx descriptors available */
	int16		txpktpend[NFIFO_EXT];	/**< tx admission control */
	uint16		*macstat_snapshot;	/**< mac hw prev read values */
#if defined(WL_PSMX)
	uint16		*macxstat_snapshot;	/* mac hw prev read values */
#endif /* WL_PSMX */
	int16		txpktpendtot;
} wlccore_t;

/** band state (phy+ana+radio) */
struct wlcband {
	int		bandtype;		/**< WLC_BAND_2G, WLC_BAND_5G */
	uint		bandunit;		/**< bandstate[] index */

	uint16		phytype;		/**< phytype */
	uint16		phyrev;
	uint16		radioid;
	uint16		radiorev;

	uint8		gmode;			/**< currently active gmode (see wlioctl.h) */

	struct scb	*hwrs_scb;		/**< permanent scb for hw rateset */

	wlc_rateset_t	defrateset;		/**< band-specific copy of default_bss.rateset */

	ratespec_t 	rspec_override;		/**< 802.11 rate override */
	ratespec_t	mrspec_override;	/**< multicast rate override */
	uint8		band_stf_ss_mode;	/**< Configured STF type, 0:siso; 1:cdd */
	int8		band_stf_stbc_tx;	/**< STBC TX 0:off; 1:force on; -1:auto */
	wlc_rateset_t	hw_rateset;		/**< rates supported by chip (phy-specific) */
	uint8		basic_rate[WLC_MAXRATE + 1]; /**< basic rates indexed by rate */

	/* EXT_STA */
	int		cached_roam_trigger;	/**< value to be restored after media stream mode */

	uint8		bw_cap;			/**< Bandwidth bitmask on this band */
	int		roam_trigger;		/**< "good enough" threshold for current AP */
	uint		roam_delta;		/**< "signif better" thresh for new AP candidates */
	int		roam_trigger_init_def;	/**< roam trigger default value in intialization */
	int		roam_trigger_def;	/**< roam trigger default value */
	uint		roam_delta_def;		/**< roam delta default value */
	int8		antgain;		/**< antenna gain from srom */
	int8		sar;			/**< SAR programmed in srom */
	int8		sar_cached;		/**< cache SROM SAR value */

	uint16		CWmin;	/**< The minimum size of contention window, in unit of aSlotTime */
	uint16		CWmax;	/**< The maximum size of contention window, in unit of aSlotTime */
	uint16		bcntsfoff;		/**< beacon tsf offset */

	/** multiple roaming profile support (up to 4 RSSI brackets) */
	wl_roam_prof_t *roam_prof;
	int roam_prof_max_idx;

	uint16		phy_minor_rev;		/* phy minor rev */
};

/* In RSDB, we share the wlc->bandstate (wlcband_t) between WLCs
* wlc[0]->bandstate[2G/5G] == wlc[1]->bandstate[2G/5G]
* But band instance, one per WLC, stores instance specific band data
* wlc[0]->bandinst[2G/5G] = pi0
* wlc[1]->bandinst[2G/5G] = pi1
*/
typedef struct _wlcband_inst {
	wlc_phy_t	*pi;
} wlcband_inst_t;

#define WLC_PI_BANDUNIT(w, bandunit)	((w)->bandinst[(bandunit)]->pi)
#define WLC_PI(w)			((w)->pi)

void wlc_pi_band_update(wlc_info_t *wlc, uint band_index);

/* generic function callback takes just one arg */
typedef void (*cb_fn_t)(void *);

#define SI_CHIPID(sih)	si_chipid(sih)

#define WLC_MAX_BRCM_ELT	32	/* Max size of BRCM proprietary elt */

/* module control blocks */
typedef	struct modulecb {
	char name[32];			/**< module name : NULL indicates empty array member */
	watchdog_fn_t watchdog_fn;	/**< watchdog handler */
	up_fn_t up_fn;			/**< up handler */
	down_fn_t down_fn;		/**< down handler. Note: the int returned
					 * by the down function is a count of the
					 * number of timers that could not be
					 * freed.
					 */
	int ref_cnt;
} modulecb_t;

/* For RSDB, modules are attached per WLC.
 * We share the modulecb (function_ptrs) and store modulecb_data (handles) separately
 */
typedef struct modulecb_data {
	void *hdl;		/**< Module Instance/Handle passed when 'doiovar' is called */
} modulecb_data_t;

/** virtual interface */
struct wlc_if {
	wlc_if_t	*next;
	uint8		type;		/**< WLC_IFTYPE_BSS or WLC_IFTYPE_WDS */
	uint8		index;		/**< assigned in wl_add_if(), index of the wlif if any,
					 * not necessarily corresponding to bsscfg._idx or
					 * AID2PVBMAP(scb).
					 */
	uint8		if_flags;	/**< flags for the interface */
	wl_if_t		*wlif;		/**< pointer to wlif */
	struct wlc_txq_info *qi;	/**< pointer to associated tx queue */
	union {
		struct scb *scb;		/**< pointer to scb if WLC_IFTYPE_WDS */
		struct wlc_bsscfg *bsscfg;	/**< pointer to bsscfg if WLC_IFTYPE_BSS */
	} u;
	wl_if_stats_t  *_cnt;		/**< interface stats counters */
	struct wlc_cx_if *wlc_cx_if;	/**< pointer to CXO offload if structure */
};

/* virtual interface types */
#define WLC_IFTYPE_BSS 1		/* virt interface for a bsscfg */
#define WLC_IFTYPE_WDS 2		/* virt interface for a wds */

/* flags for the interface */
#define WLC_IF_PKT_80211	0x01	/* this interface expects 80211 frames */
#define WLC_IF_LINKED		0x02	/* this interface is linked to a wl_if */
#define WLC_IF_VIRTUAL		0x04

#define WLC_IF_IS_VIRTUAL(x)  ((x)->if_flags & WLC_IF_VIRTUAL)

#if defined(DMA_TX_FREE)
#define WLC_TXSTATUS_SZ 128
#define WLC_TXSTATUS_VEC_SZ	(WLC_TXSTATUS_SZ/NBBY)
typedef struct wlc_txstatus_flags {
	uint	head;
	uint	tail;
	uint8	vec[WLC_TXSTATUS_VEC_SZ];
}  wlc_txstatus_flags_t;
#endif /* DMA_TX_FREE */

/* DELTASTATS */
#define DELTA_STATS_NUM_INTERVAL	2	/* number of stats intervals stored */

/* structure to store info for delta statistics feature */
typedef struct delta_stats_info {
	uint32 interval;		/**< configured delta statistics interval (secs) */
	uint32 seconds;			/**< counts seconds to next interval */
	uint32 current_index;		/**< index into stats of the current statistics */
	wl_delta_stats_t stats[DELTA_STATS_NUM_INTERVAL];	/**< statistics for each interval */
} delta_stats_info_t;

/* Source Address based duplicate detection (Avoid heavy SCBs) */
#define SA_SEQCTL_SIZE 10

struct sa_seqctl {
	struct ether_addr sa;
	uint16 seqctl_nonqos;
};


/**
 * TX Queue information
 *
 * Each flow of traffic out of the device has a TX Queue with independent
 * flow control. Several interfaces may be associated with a single TX Queue
 * if they belong to the same flow of traffic from the device. For multi-channel
 * operation there are independent TX Queues for each channel.
 */
struct wlc_txq_info {
	struct wlc_txq_info*    next;
	struct pktq             txq;		/**< See comment below */
	uint                    stopped;	/**< tx flow control bits */
	txq_t*                  low_txq;
#ifdef TXQ_MUX
	wlc_info_t*             wlc;
	wlc_mux_t*              ac_mux[NFIFO];
	mux_source_handle_t     mux_hdl[NFIFO];
#endif /* TXQ_MUX */
	uint8			epoch[AC_COUNT];	/* in case of WL_MULTIQUEUE this is used to
							 * save/restore the per ac epoch state
							 */
};

/*
 * Macro returns tx queue structure given the txq info structure
 *
 * This should be used in preference to direct access when
 * backporting code form other branches.
 *
 * The internal structure of the TXQ Info is subject
 * to change as the new TXQ features are introduced.
 */
#define WLC_GET_TXQ(qi) (&(qi)->txq)

/* Flags used in wlc_txq_info.stopped */
#define TXQ_STOP_FOR_PRIOFC_MASK	0x000000FF /* per prio flow control bits */
#define TXQ_STOP_FOR_PKT_DRAIN		0x00000100 /* stop txq enqueue for packet drain */
#define TXQ_STOP_FOR_AMPDU_FLOW_CNTRL	0x00000200 /* stop txq enqueue for ampdu flow control */
#define TXQ_STOP_FOR_MSCH_FLOW_CNTRL	0x00000400 /* stop txq enqueue for msch flow control */

/* defines for gptimer stay awake requestor */
#define WLC_GPTIMER_AWAKE_MSCH		0x00000001
#define WLC_GPTIMER_AWAKE_TBTT		0x00000002
#define WLC_GPTIMER_AWAKE_AWDL		0x00000004
#define WLC_GPTIMER_AWAKE_PROXD		0x00000008
#define WLC_GPTIMER_AWAKE_NAN		0x00001000

/* defines for user_wake_req stay awake requestor */
#define WLC_USER_WAKE_REQ_NIC		0x00000001
/* wake up request for watchdog to run */
#define WLC_USER_WAKE_REQ_WD		0x00000002
#define WLC_USER_WAKE_REQ_VHT		0x00000004
/* wake request for NAN */
#define WLC_USER_WAKE_REQ_NAN		0x00000008
#define WLC_USER_WAKE_REQ_TX		0x00000010
/* wake request for TSYNC */
#define WLC_USER_WAKE_REQ_TSYNC		0x00000020


/* defined for PMblocked requestor */
#define WLC_PM_BLOCK_CHANSW		0x00000001
#define WLC_PM_BLOCK_MCHAN_ABS		0x00000002
#define WLC_PM_BLOCK_AWDL		0x00000004
#define WLC_PM_BLOCK_MODESW		0x00000010


typedef struct {
	uint32  txbytes_prev;   /**< Number of bytes transmitted until last second */
	uint32  rxbytes_prev;   /**< Number of bytes received until last second */
	uint32  txbytes_rate;   /**< Derive this - Number of bytes transmitted in last second */
	uint32  rxbytes_rate;   /**< Derive this - Number of bytes received in last second */
	uint32  txframes_prev;   /**< Number of frames transmitted until last second */
	uint32  rxframes_prev;   /**< Number of frames received until last second */
	uint32  txframes_rate;   /**< Derive this - Number of frames transmitted in last second */
	uint32  rxframes_rate;   /**< Derive this - Number of frames received in last second */
} wlc_rate_measurement_t;

/** Info block related to excess_pm_wake */
typedef struct wlc_excess_pm_wake_info {
	uint32	roam_ms;		/**< roam time since driver load */
	uint32	pfn_scan_ms;		/**< pfn-scan time since driver load */

	uint32	roam_alert_thresh;	/**< User config */
	uint32	pfn_alert_thresh;	/**< User config */

	bool	roam_alert_enable;
	bool	pfn_alert_enable;

	uint32	roam_alert_thresh_ts;	/**< roam_ms ts on IOVAR set */
	uint32	pfn_alert_thresh_ts;	/**< pfn_scan_ms ts on IOVAR set */

	uint32	last_pm_prd_roam_ts;	/**< roam ts when last excess_pm_wake prd started */
	uint32	last_pm_prd_pfn_ts;	/**< pfn_scan ts when last excess_pm_wake prd started */

	uint32	ca_thresh;	/**< user configured constant awake thresh */
	uint32	last_ca_pmdur;	/**< last constant awake pmdur */
	uint32	last_ca_osl_time;	/**< constant awake osl time */
	uint32	pp_start_event_dur[WLC_PMD_EVENT_MAX]; /**< pm period start event-duration */
	uint32	ca_start_event_dur[WLC_PMD_EVENT_MAX]; /**< constant awake start event-duration */
	uint32	ca_txrxpkts;	/**< transmit receive packets */
	uint32	ca_pkts_allowance;	/**< max packets allowed in const awake to generate alert */
	uint32	ca_alert_pmdur;	/**< pmdur value at time of const awake alert */
	uint32	ca_alert_pmdur_valid;	/**< to differentiate if ca_alert_pmdur is valid */
	uint32	last_frts_dur;		/**< frts duration when period started */
	uint32	last_cal_dur;		/**< calibration time when period started */
	wl_pmalert_t *pm_alert_data;	/**< PM Alert data if excess_pm_percent threshhold hit */
	uint32	pm_alert_datalen;	/**< Lenth of the collect PM Alert data */
} wlc_excess_pm_wake_info_t;

#ifdef WLCXO_CTRL
/** helper struct for ctrl-side packet reordering */
struct wlc_rxr_pkt_info {
	void	*ofld_p;
	uint16	headroom;
	uint16	pktlen;
	uint16	fraglen;		/**< used only in DONGLEBUILD */
};
typedef struct wlc_rxr_pkt_info wlc_rxr_pkt_info_t;
#endif

/** Principal common (os-independent) software data structure */
struct wlc_info {
	wlc_pub_t	*pub;			/**< pointer to wlc public state */
	osl_t		*osh;			/**< pointer to os handle */
	struct wl_info	*wl;			/**< pointer to os-specific private state */
	wlc_cmn_info_t	*cmn;	/**< pointer to common part of two wlc structure variables */
	d11regs_t	*regs;			/**< pointer to device registers */

	wlc_hw_info_t	*hw;			/**< HW module (private h/w states) */
	wlc_hw_t	*hw_pub;		/**< shadow/direct accessed HW states */

	/* up and down */
	bool		device_present;		/**< (removable) device is present */

	bool		clk;			/**< core is out of reset and has clock */
	bool		watchdog_disable;	/**< to disable watchdog */

	/* multiband */
	wlccore_t	*core;                  /**< pointer to active io core */
	wlccore_t	*corestate;             /**< per-core state (one per hw core) */
	wlc_phy_t	*pi;			/**< pointer to active per-phy info */
	wlcband_t	*band;                  /**< pointer to active per-band state */
	wlcband_t	*bandstate[MAXBANDS];   /**< per-band state (one per phy/radio) */
	wlcband_inst_t	**bandinst;		/**< per-band instance */

	uint16		bt_shm_addr;

	bool		tx_suspended;		/**< data fifos need to remain suspended */

	/* packet queue */
	txq_info_t	*txqi;		/**< low serialized TxQ */
#ifdef TXQ_MUX
	wlc_mux_info_t	*tx_mux;		/**< tx queuing MUX */
	wlc_scbq_info_t	*tx_scbq;		/**< tx path per-scb queuing */
#endif /* TXQ_MUX */
	uint		qvalid;			/**< DirFrmQValid and BcMcFrmQValid */
	bool            in_send_q;	/**< flag to prevent concurent calls to wlc_send_q */



	/* sub-module handler */
	scb_module_t	*scbstate;		/**< scb module handler */
	bsscfg_module_t	*bcmh;			/**< bsscfg module handler */
	wlc_seq_cmds_info_t	*seq_cmds_info;	/**< pointer to sequence commands info */

	/* WLLED */
	led_info_t	*ledh;			/**< pointer to led specific information */
	/* WLAMSDU || WLAMSDU_TX */
	amsdu_info_t	*ami;			/**< amsdu module handler */
	/* WLNAR */
	wlc_nar_info_t *nar_handle;		/**< nar module private data */
	/* SCB_BS_DATA */
	wlc_bs_data_info_t  *bs_data_handle;
	/* WLAMPDU */
	ampdu_tx_info_t *ampdu_tx;		/**< ampdu tx module handler */
	ampdu_rx_info_t *ampdu_rx;		/**< ampdu rx module handler */
	/* WOWL */
	wowl_info_t	*wowl;			/**< WOWL module handler */
	/* WLTDLS */
	tdls_info_t	*tdls;			/**< tdls module handler */
	/* WLDLS */
	dls_info_t		*dls;		/**< dls module handler */
	/* L2_FILTER */
	l2_filter_info_t *l2_filter;		/**< L2 filter module handler */
	/* BCMAUTH_PSK */
	wlc_auth_info_t	*authi;			/**< authenticator module handler shim */
	/* WLMCNX */
	wlc_mcnx_info_t *mcnx;			/**< p2p ucode module handler */
	wlc_tbtt_info_t *tbtt;			/**< tbtt module handler */
	/* WLP2P */
	wlc_p2p_info_t	*p2p;			/**< p2p module handler */
	/* WLMCHAN */
	mchan_info_t	*mchan;			/**< mchan module handler */
	/* WLBTAMP */
	bta_info_t	*bta;			/**< bta module handler */
	/* PSTA */
	wlc_psta_info_t	*psta;			/**< proxy sta module info */
	/* WLEXTLOG */
	wlc_extlog_info_t *extlog;		/**< extlog handler */
	antsel_info_t	*asi;			/**< antsel module handler */
	wlc_cm_info_t	*cmi;			/**< channel manager module handler */
	wlc_ratesel_info_t	*wrsi;		/**< ratesel info wrapper */
	wlc_cac_t	*cac;			/**< CAC handler */
	wlc_scan_info_t	*scan;			/**< ptr to scan state info */
	wlc_plt_pub_t	*plt;			/**< plt(Production line Test) module handler */
	void		*weth;			/**< pointer to wet specific information */
	void		*wetth;			/**< pointer to wet tunnel specific information */
	void		*PAD;			/**< increased ROM compatibility */
	void		*PAD;			/**< increased ROM compatibility */
	wlc_11h_info_t	*m11h;			/**< 11h module handler */
	wlc_tpc_info_t	*tpc;			/**< TPC module handler */
	wlc_csa_info_t	*csa;			/**< CSA module handler */
	wlc_quiet_info_t *quiet;		/**< Quiet module handler */
	wlc_dfs_info_t *dfs;			/**< DFS module handle */
	wlc_11d_info_t *m11d;			/**< 11d module handler */
	wlc_cntry_info_t *cntry;		/**< Country module handler */
	wlc_11u_info_t *m11u;			/**< 11u module handler */
	wlc_probresp_info_t *mprobresp; 	/**< SW probe response module handler */

	wlc_wapi_info_t *wapi;			/**< WAPI (WPI/WAI) module handler */
	int		wapi_cfgh;		/**< WAI bsscfg cubby handler */
	wlc_txc_info_t *txc;			/**< tx header caching module handler */
	wlc_btc_info_t *btch;			/**< btc module header */
	wlc_mbss_info_t	*mbss;			/**< mbss fields */

	void		*btparam;		/**< bus type specific cookie */

	uint16		vendorid;		/**< PCI vendor id */
	uint16		deviceid;		/**< PCI device id */
	uint		ucode_rev;		/**< microcode revision */

#ifdef VASIP_HW_SUPPORT
	uint32		vasip_major;		/**< VASIP core major version */
	uint32		vasip_minor;		/**< VASIP core minjor version */
#endif /* VASIP_HW_SUPPORT */

	uint32		machwcap;		/**< MAC capabilities, BMAC shadow */
	uint16		xmtfifo_szh[NFIFO];	/**< fifo size in 256B for each xmt fifo */
	uint16		xmtfifo_frmmaxh[AC_COUNT];	/**< max # of frames fifo size can hold */

	struct ether_addr perm_etheraddr;	/**< original sprom local ethernet address */

	bool		bandlocked;		/**< disable auto multi-band switching */
	bool		bandinit_pending;	/**< track band init in auto band */

	bool		radio_monitor;		/**< radio timer is running */
	bool		down_override;		/**< true=down */
	bool		going_down;		/**< down path intermediate variable */
	/* WLSCANCACHE */
	bool		_assoc_cache_assist;	/**< enable use of scan cache in assoc attempts */

	bool		mpc;			/**< enable minimum power consumption */
	bool		mpc_out;		/**< disable radio_mpc_disable for out */
	bool		mpc_scan;		/**< disable radio_mpc_disable for scan */
	bool		mpc_join;		/**< disable radio_mpc_disable for join */
	bool		mpc_oidscan;		/**< disable radio_mpc_disable for oid scan */
	bool		mpc_oidjoin;		/**< disable radio_mpc_disable for oid join */
	bool		mpc_oidp2pscan;		/**< disable radio_mpc_disable for oid p2p scan */
	bool		mpc_oidnettype;		/**< disable radio_mpc_disable for oid
						 * network_type_in_use
						 */
	uint8		mpc_dlycnt;		/**< # of watchdog cnt before turn disable radio */
	uint32		mpc_off_ts;		/**< timestamp (ms) when radio was disabled */
	uint8		mpc_delay_off;		/**< delay radio disable by # of watchdog cnt */

	mbool		mpc_off_req;		/**< request to set MPC off. See MPC_OFF_REQ_xxx
						 * bit definitinos
						 */
	uint32		mpc_dur;	/**< total time (ms) in mpc mode except for the
					 * portion since radio is turned off last time
					 */
	uint32		mpc_laston_ts;	/**< timestamp (ms) when radio is turned off last
					 * time
					*/
	uint32 		mpc_lastoff_ts; /**< timestamp (ms) when radio is turned on last
					 * time
					*/
	uint32 		total_on_time;  /**< aggregate radio on time: pm or mpc */
	uint8		mpc_mode; /* mpc mode */

	/* timer */
	struct wl_timer *wdtimer;		/**< timer for watchdog routine */
	uint		fast_timer;		/**< Periodic timeout for 'fast' timer */
	uint		slow_timer;		/**< Periodic timeout for 'slow' timer */
	uint		glacial_timer;		/**< Periodic timeout for 'glacial' timer */
	uint		phycal_mlo;		/**< last time measurelow calibration was done */
	uint		phycal_txpower;		/**< last time txpower calibration was done */

	struct wl_timer *radio_timer;		/**< timer for hw radio button monitor routine */

	/* promiscuous */
	uint32		monitor;		/**< monitor (MPDU sniffing) mode */
	bool		bcnmisc_ibss;		/**< bcns promisc mode override for IBSS */
	bool		bcnmisc_scan;		/**< bcns promisc mode override for scan */
	bool		bcnmisc_monitor;	/**< bcns promisc mode override for monitor */

	uint8		bcn_wait_prd;		/**< max waiting period (for beacon) in 1024TU */

	/* WLAMSDU */
	bool		_amsdu_rx;		/**< true if currently amsdu deagg is enabled */
	bool		_rx_amsdu_in_ampdu;	/**< true if currently amsdu deagg is enabled */
	bool		_amsdu_noack;		/**< enable AMSDU no ack mode */
	bool		wet;			/**< true if wireless ethernet bridging mode */
	bool		mac_spoof;		/**< Original wireless ethernet, MAC Clone/Spoof */
	bool            _mu_tx;                 /**< true if MU-MIMO Tx feature is enabled */
	bool            _mu_rx;                 /**< true if MU-MIMO Tx feature is enabled */

	/* AP-STA synchronization, power save */
	/* These are only summary flags and used as global WAKE conditions.
	 *  Change them through accessor functions done on each BSS.
	 * - check_for_unaligned_tbtt - wlc_set_uatbtt(cfg, state)
	 * - PMpending - wlc_set_pmpending(cfg, state)
	 * - PSpoll - wlc_set_pspoll(cfg, state)
	 * - PMawakebcn - wlc_set_pmwakebcn(cfg, state)
	 */
	bool		check_for_unaligned_tbtt; /**< check unaligned tbtt flag */
	bool		PMpending;		/**< waiting for tx status with PM indicated set */
	bool		PSpoll;		/**< whether there is an outstanding PS-Poll frame */
	bool		PMawakebcn;		/**< bcn recvd during current waking state */

	bool		wake;			/**< host-specified PS-mode sleep state */
	mbool		user_wake_req;
	uint8		bcn_li_bcn;		/**< beacon listen interval in # beacons */
	uint8		bcn_li_dtim;		/**< beacon listen interval in # dtims */

	bool		WDarmed;		/**< watchdog timer is armed */
	uint32		WDlast;			/**< last time wlc_watchdog() was called */

	/* WME */
	ac_bitmap_t	wme_dp;			/**< Discard (oldest first) policy per AC */
	uint16		wme_dp_precmap;		/**< Precedence map based on discard policy */
	bool		wme_prec_queuing;	/**< enable/disable non-wme STA prec queuing */
	uint16		wme_max_rate[AC_COUNT]; /**< In units of 512 Kbps */
	uint16		wme_retries[AC_COUNT];  /**< per-AC retry limits */
	uint32		wme_maxbw_ac[AC_COUNT];	/**< Max bandwidth per allowed per ac */

	int		vlan_mode;		/**< OK to use 802.1Q Tags (ON, OFF, AUTO) */
	uint16		tx_prec_map;		/**< Precedence map based on HW FIFO space */
	uint16		fifo2prec_map[NFIFO];	/**< pointer to fifo2_prec map based on WME */

	/* BSS Configurations */
	wlc_bsscfg_t	**bsscfg;		/**< set of BSS configurations, idx 0 is default and
						 * always valid
						 */
	wlc_bsscfg_t	*cfg;			/**< the primary bsscfg (can be AP or STA) */

	uint8		stas_associated;	/**< count of ASSOCIATED STA bsscfgs */
	uint8		stas_connected;		/**< # ASSOCIATED STA bsscfgs with valid BSSID */
	uint8		aps_associated;		/**< count of UP AP bsscfgs */
	uint8		ibss_bsscfgs;		/**< count of IBSS bsscfgs */
	uint8		block_datafifo;		/**< prohibit posting frames to data fifos */
	bool		bcmcfifo_drain;		/**< TX_BCMC_FIFO is set to drain */

	/* tx queue */
	wlc_txq_info_t	*tx_queues;		/**< common TX Queue list */

	/* event */
	wlc_eventq_t	*eventq;		/**< event queue for deferred processing */

	uint32		lifetime[AC_COUNT];	/**< MSDU lifetime per AC in us */
	modulecb_t	*modulecb;		/**< Module callback fptrs */
	modulecb_data_t	*modulecb_data;		/**< Module handle instance */
	wlc_dump_info_t	*dumpi;			/**< dump registry handle */
	wlc_txmod_info_t *txmodi;		/**< txmod module handle */

	wlc_bss_list_t	*scan_results;

	wlc_bss_info_t	*default_bss;		/**< configured BSS parameters */
	uint16		counter;		/**< per-sdu monotonically increasing counter */
	uint16		mc_fid_counter;		/**< BC/MC FIFO frame ID counter */
	wlc_assoc_info_t *as;			/**< Association information */
	wlc_pm_info_t *pm;			/**< Power Management module handle */

	/* EXT_STA */
	bool		scan_suppress_ssid;	/**< Flag: suppress ssids after hibernation */
	int		allow_mode;		/**< 0: any, 1: desired_BSSID, 2: none */
	uint32		link_report;	/**< period link speed and link quality report timer */

	struct ether_addr	desired_BSSID;	/**< allow this station */
	bool		ibss_allowed;		/**< FALSE, all IBSS will be ignored during a scan
						 * and the driver will not allow the creation of
						 * an IBSS network
						 */
	bool 		ibss_coalesce_allowed;

	/* country, spect management */
	bool		country_list_extended;	/**< JP-variants are reported through
						 * WLC_GET_COUNTRY_LIST if TRUE
						 */

	uint16		prb_resp_timeout;	/* do not send prb resp if request older than this,
						 * 0 = disable
						 */

	wlc_rateset_t	sup_rates_override;	/* use only these rates in 11g supported rates if
						 * specifed
						 */

	/** 11a rate table direct adddress map */
	uint16		rt_dirmap_a[D11_RT_DIRMAP_SIZE];

	/** 11b rate table direct adddress map */
	uint16		rt_dirmap_b[D11_RT_DIRMAP_SIZE];

	chanspec_t	home_chanspec;		/**< shared home chanspec */

	/* PHY parameters */
	chanspec_t	chanspec;		/**< target operational channel */
	uint16		usr_fragthresh;	/* user configured fragmentation threshold */
	uint16		fragthresh[NFIFO];	/**< per-fifo fragmentation thresholds */
	uint16		RTSThresh;		/**< 802.11 dot11RTSThreshold */
	uint16		SRL;			/**< 802.11 dot11ShortRetryLimit */
	uint16		LRL;			/**< 802.11 dot11LongRetryLimit */
	uint16		SFBL;			/**< Short Frame Rate Fallback Limit */
	uint16		LFBL;			/**< Long Frame Rate Fallback Limit */

	/* network config */
	bool	shortslot;		/**< currently using 11g ShortSlot timing */
	int8	shortslot_override;	/**< 11g ShortSlot override */
	bool	ignore_bcns;		/**< override: ignore non shortslot bcns in a 11g network */
	bool	interference_mode_crs;	/**< aphy crs state for interference mitigation mode */
	bool	legacy_probe;		/**< restricts probe requests to CCK rates */

	/* 11g/11n protections */
	wlc_prot_info_t *prot;		/**< 11g & 11n protection module handle */
	wlc_prot_g_info_t *prot_g;	/**< 11g protection module handle */
	wlc_prot_n_info_t *prot_n;	/**< 11n protection module handle */

	wlc_prot_obss_info_t *prot_obss;
	wlc_obss_dynbw_t *obss_dynbw;

	wlc_stf_t *stf;

	wlc_pcb_info_t *pcb;		/**< packet tx complete callback */

	uint32		txretried;		/**< tx retried number in one msdu */

	ratespec_t	bcn_rspec;		/**< save bcn ratespec purpose */

	/* STA */
	bool		IBSS_join_only;		/**< don't start IBSS if not found */

	/* EXT_STA */
	bool	media_stream_mode;	/**< media streaming mode enabled? */
	bool	use_group_enabled;	/**< ignore BSS which doesn't support
					* pairwise cipher
					*/

	/** WLRM */
	rm_info_t	*rm_info;

	/* These are only summary flags and used as global WAKE conditions.
	 *  Change them through accessor functions done on each BSS.
	 * - apsd_sta_usp - wlc_set_apsd_stausp(cfg, state)
	 */
	bool		apsd_sta_usp;	/**< Unscheduled Service Period in progress on STA */

	/* WLCHANIM */
	chanim_info_t *chanim_info;

	apps_wlc_psinfo_t *psinfo;              /**< Power save nodes related state */
	wlc_ap_info_t *ap;
	cs_info_t *cs;

	/* Global txmaxmsdulifetime, global rxmaxmsdulifetime */
	uint32		txmsdulifetime;
	uint16		rxmsdulifetime;

	/* DELTASTATS */
	delta_stats_info_t *delta_stats;

	uint16	rfaware_lifetime;	/**< RF awareness lifetime (us/1024, not ms) */
	uint32	exptime_cnt;		/**< number of expired pkts since start_exptime */
	uint32	last_exptime;		/**< sysuptime for last timer expiration */

	uint8	txpwr_percent;		/**< power output percentage */

	uint16		tx_duty_cycle_ofdm;	/**< maximum allowed duty cycle for OFDM */
	uint16		tx_duty_cycle_cck;	/**< maximum allowed duty cycle for CCK */
	bool		nav_reset_war_disable;	/**< WAR to reset NAV on 0 duration ACK */

	/* parameters for delaying radio shutdown after sending NULL PM=1 */
	uint16		pm2_radio_shutoff_dly;	/**< configurable radio shutoff delay */
	bool		pm2_radio_shutoff_pending; /**< flag indicating radio shutoff pending */
	struct wl_timer *pm2_radio_shutoff_dly_timer;	/**< timer to delay radio shutoff */

	struct sa_seqctl ctl_dup_det[SA_SEQCTL_SIZE]; /**< Small table for duplicate detections
						       * for cases where SCB does not exist
						       */
	uint8 ctl_dup_det_idx;

	void *rwl;

	wlc_rrm_info_t *rrm_info;	/**< 11k radio measurement info */
	/* WLWNM */
	wlc_wnm_info_t *wnm_info;	/**< 11v wireless radio management info */

	wlc_if_t	*wlcif_list;	/**< linked list of wlc_if structs */
	wlc_txq_info_t	*active_queue;	/**< txq for the currently active transmit context */

	/* WL_MULTIQUEUE */
	wlc_txq_info_t *primary_queue;	/**< current queue for normal (non-excursion) traffic */
	wlc_txq_info_t *excursion_queue; /**< txq for excursions (scan, measurement, etc.) */
	bool excursion_active;
	bool txfifo_attach_pending;
	bool txfifo_detach_pending;
	bool txfifo_detach_transition_queue_del; /* del detach tran q after sync is done */
	wlc_txq_info_t *txfifo_detach_transition_queue;

	/* WLPFN */
	void		*pfn;
	/* EXT_STA */
	bool		wakeup_scan;		/**< during wakeup sccan */

	bool		pr80838_war;
	bool		nolinkup; 	/**< suppress link up events */
	uint16		d11rxoff;	/* it includes all headers except SW RXHDR */
	uint16		hwrxoff;	/* it includes SW RXHDR as well */
	uint16		hwrxoff_pktget;
	wlc_hrt_info_t *hrti;	/**< hw gptimer for multiplexed use */
	mbool		gptimer_stay_awake_req; /**< bitfield used to keep device awake
						 * if user of gptimer requires it
						 */
	uint16	wake_event_timeout; /**< timeout value in seconds to trigger sending wake_event */
	struct wl_timer *wake_event_timer;	/**< timer to send an event up to the host */

	/* STA && (AP_KEEP_ALIVE || BCMCCX) */
	uint16		keep_alive_time;
	uint16		keep_alive_count;

	/* STA */
	bool	reset_triggered_pmoff;	/**< tells us if reset took place and turned off PM */

	bool rpt_hitxrate;	/**< report highest tx rate in history */

	/* DMATXRC */
	pktpool_t *pktpool_phdr;
	int phdr_len;
	void **phdr_list;
	uint8 txrc_fifo_mask;

	/* WLRXOV */
	uint8 rxov_active;
	uint16 rxov_txmaxpkts;
	uint16 rxov_delay;
	struct wl_timer *rxov_timer;

	bool is_initvalsdloaded;

	/* PROP_TXSTATUS */
	/*
	- b[0] = 1 to enable WLFC_FLAGS_RSSI_SIGNALS
	- b[1] = 1 to enable WLFC_FLAGS_XONXOFF_SIGNALS
	- b[2] = 1 to enable WLFC_FLAGS_CREDIT_STATUS_SIGNALS
	*/
	uint8	wlfc_flags;
	wlc_wlfc_info_t *wlfc;		/**< dongle host flow control module handle */

	/** BCMSDIO */
	void * sdh;

	uint8	roam_rssi_cancel_hysteresis;	/**< Cancel ROAM RSSI Hysteresis */

	uint32  pkteng_maxlen;      /**< maximum packet length */

	int16 excess_pm_period;
	int16 excess_pm_percent;
	uint32 excess_pm_last_pmdur;
	uint32 excess_pm_last_osltime;

	/** Notifier module. */
	bcm_notif_module_t	*notif;

	/** Memory pool manager handle. */
	bcm_mpm_mgr_h	mem_pool_mgr;
	uint16 fabid;
	bool  brcm_ie;        /**< flag to disable/enable brcm ie in auth req */
	uint32 prb_req_enable_mask;	/**< mask of probe request clients */
	uint32 prb_rsp_disable_mask;	/**< mask of disable ucode probe response clients */
	ratespec_t	monitor_ampdu_rspec;	/**< monitor rspec value for AMPDU sniffing */
	void		*monitor_amsdu_pkts;	/**< monitor packet queue for AMSDU sniffing */
	uint32 		monitor_ampdu_counter;

	/* STA */
	bool    seq_reset;              /**< used for determining whether to reset
	                                 * sequence number register in SCR or not
	                                 */
	/* WOWLPF */
	wowlpf_info_t   *wowlpf;        /**< WOWLPF module handler */

	/* WLOTA_EN */
	ota_test_info_t * ota_info;
	uint8 *iov_block;

	struct wlc_btc_param_vars_info* btc_param_vars; /**< storage for BTC NVRAM data */

	bool    toe_bypass;
	bool    toe_capable;

	bool dma_avoidance_war;

	/* TRAFFIC_MGMT */
	wlc_trf_mgmt_ctxt_t	    *trf_mgmt_ctxt;    /**< pointer to traffic management context */

	wlc_pktc_info_t	*pktc_info;	/**< packet chaining info */

	/* MFP */
	wlc_mfp_info_t *mfp;

	wlc_macfltr_info_t *macfltr;	/**< MAC allow/deny list module handle */
	uint32	nvramrev;
	wlc_bmon_info_t *bmon;		/**< BSSID monitor module handle */

	bool blocked_for_slowcal;
	wlc_rmc_info_t *rmc;

	/** WLWSEC */
	wlc_keymgmt_t *keymgmt;

	wlc_iem_info_t *iemi;	/**< IE mgmt module */
	wlc_vht_info_t *vhti;	/**< VHT module */
	wlc_ht_info_t *hti;	/**< HT module */
	wlc_obss_info_t *obss; /**< OBSS coex module */

	wlc_akm_info_t *akmi;	/**< AKM module */
	wlc_ier_info_t *ieri;	/**< IE registry module */
	wlc_ier_reg_t *ier_tdls_srq;	/**< TDLS Setup Request registry */
	wlc_ier_reg_t *ier_tdls_srs;	/**< TDLS Setup Response registry */
	wlc_ier_reg_t *ier_tdls_scf;	/**< TDLS Setup Confirm registry */
	wlc_ier_reg_t *ier_tdls_drs;	/**< TDLS Discovery Response registry */
	wlc_ier_reg_t *ier_csw;	/**< CSA Wrapper IE registry */
	wlc_ier_reg_t *ier_fbt;	/**< FBT over the DS registry */
	wlc_ier_reg_t *ier_ric;	/**< FBT RIC Request registry */

	/* ANQPO */
	void		*anqpo;			/**< anqp offload module handler */

	uint32 atf;
	wlc_hs20_info_t *hs20;			/**< hotspot module handler */
	wlc_sup_info_t	*idsup;			/**< supplicant module */
	wlc_fbt_info_t	*fbt;
	wlc_assoc_mgr_info_t	*assoc_mgr;	/* fine grained handling of association */
	wlc_ccxsup_info_t *ccxsup;
	wlc_pmkid_info_t	*pmkid_info;

	wlc_ltr_info_t *ltr_info;

	/* WLOLPC */
	wlc_olpc_eng_info_t *olpc_info;

	wlc_srvsdb_info_t *srvsdb_info;

	/* WL_OKC || WLRCC */
	okc_info_t *okc_info;

	wlc_staprio_info_t *staprio;
	wlc_stamon_info_t *stamon_info; /**< Opaque pointer to sta monitor module */
	wlc_monitor_info_t *mon_info;   /**< Opaque pointer to monitor module */
	wlc_wds_info_t	*mwds;			/**< wds module handler */
	wlc_lpc_info_t		*wlpci;		/**< LPC module handler */

	chanswitch_times_t	*chansw_hist; /**< chanswitch history */
	chanswitch_times_t	*bandsw_hist; /**< bandswitch history */

	uint8 passive_on_restricted;	/**< controls scan on restricted channels */
	wlc_obj_registry_t *objr; /**< Opaque pointer to Object Registry */

	/* WLAIBSS */
	wlc_aibss_info_t *aibss_info;			/**< AIBSS module handler */

	bool allow_txbf;
	wlc_txbf_info_t *txbf;
	wlc_early_bcn_detect_t *ebd;
	wlc_stats_info_t	*stats_info;	/**< Opaque pointer to wlc statistics module */
	wmf_info_t	*wmfi;			/**< wmf module handler */

	uint16 longpkt_rtsthresh;
	uint16 longpkt_shm;

	bool	psm_watchdog_debug;		/**< used for ucode debugging */

	wlc_iocv_info_t *iocvi;		/**< iocv module handle */

	wlc_pps_info_t *pps_info; /**< Opaque pointer to ps-pretend */

	/* WLDURATION */
	duration_info_t *dur_info;	/**< Opaque pointer to the duration module */

	/* KEEP_ALIVE */
	void *keepalive;

	/* WL_PM_MUTE_TX */
#ifdef WL_PM_MUTE_TX
	wlc_pm_mute_tx_t *pm_mute_tx;   /**< Handler for power-save mode with muted transmit path */
#endif

	uint32 wlc_pm_dur_cntr;		/**< pm_dur fw counter in ms */
	uint16 pm_dur_clear_timeout;	/**< timeout to read/reset ucode counter */
	uint32 wlc_pm_dur_last_sample;
	uint32 lpas; /**< Low power associated sleep mode */

	/** WLPFN */
	uint32 excess_pm_last_mpcdur;

	void *pwrstats;
	wlc_excess_pm_wake_info_t *excess_pmwake;
	wlc_ipfo_info_t *ipfo_info;
	wlc_pdsvc_info_t *pdsvc_info; /**< proximity detection module handler */
	bool tcpack_fast_tx;    /**< Faster AMPDU release and transmission in ucode for TCPACK */
	wlc_bssload_info_t *mbssload;	/**< bss load IE info */
	wlc_ltecx_info_t	*ltecx;		/**< LTECX info */
	void *mprobresp_mac_filter;	/**< MAC based SW probe resp module */
	bwte_info_t *bwte_info;		/**< BWTE module handler */
	tbow_info_t *tbow_info;		/**< TBOW module handler */
	wlc_modesw_info_t *modesw; /**< modesw Wlc structure pointer */

	/* WL_NAN */
	wlc_nan_info_t	*nan;		/**<  bcm NAN instance */

	/* CCA_STATS */
	cca_info_t *cca_info;
	cca_chan_qual_t *cca_chan_qual;

	/* ISID_STATS */
	itfr_info_t *itfr_info;

	void *PAD;

	bool    wd_waitfor_bcn;		/**< watchdog timer is armed */
	bool	watchdog_on_bcn;	/**< enable/disable delay watchdog on beacon  */
	bool	wd_run_flag;		/**< flag to decide whether watchdog can run or not */
	bool wd_deferred;		/**< track whether wd has deferred */

	struct wl_timer *bcmc_wdtimer;	/**< timer used to schedule WD on bc/mc fc flag  */
	bool bcmc_wdtimer_armed;	/**< flag to determine whether timer is running or not */

	wlc_bcntrim_info_t *bcntrim;	/**< beacon Trim Wlc structure pointer */

	wlc_misc_info_t *misc;	/**< handle to 'misc' module */
	wlc_smfs_info_t	*smfs;	/**< handle to "Selected Mgmt Frame Stats" module */
	wlc_bsscfg_psq_info_t *psqi;	/**< per-bss psq management */
	wlc_bsscfg_viel_info_t *vieli;	/**< per-bss vendor plumbed IEs */
	wlc_linkstats_info_t *linkstats_info;
	wlc_lq_info_t *lqi;		/* link quality module handle */
	wlc_ulp_info_t *ulp; /* ULP feature Wlc structure pointer */
	wlc_mesh_info_t *mesh_info;
	wlc_mutx_info_t *mutx;		/* MU-MIMO transmit module */
	wlc_murx_info_t *murx;          /* MU-MIMO receive module */
	wlc_ulb_info_t *ulb_info;
	wlc_tsmap_info_t *wlctsmap;   /* timeslot map utility instance */
	wlc_frag_info_t *frag;		/* fragmentation module handle */
	struct wlc_cxo_ctrl *cxo_ctrl;	/* handle to WLCXO ctrl info */
	struct wlc_cx_info *wlc_cx;	/* Handle to wlc info of offload drv */
	struct wlc_ipc	*ipc;		/* Handle to ipc module */
	wlc_sta_info_t *sta_info;	/* MSCH context module handler */
	wlc_msch_info_t *msch_info;		/* MSCH module handler */
	wlc_cxnoa_info_t *cxnoa;	/* NoA based BTCX module handler */
	wlc_act_frame_info_t *wlc_act_frame_info;	/* Action frame module handler */
	wlc_qos_info_t *qosi;		/* QoS/EDCF/WME/APSD config module handle */
	wlc_rsdb_info_t *rsdbinfo;      /* RSDB Module Structure */
	wlc_macdbg_info_t *macdbg;	/* MAC Debug */
	wlc_rspec_info_t *rspeci;	/* rspec module info */
	health_check_info_t *hc; /* healthcheck info instance */
	wlc_ndis_info_t *ndis;
	wlc_scan_utils_t *sui;		/* scan utils handle */
	wlc_test_info_t *testi;		/* WLTEST module */
	wlc_flow_ctx_info_t *flow_tbli;		/* WLTEST module */
	wlc_randmac_info_t *randmac_info; /* MAC randomization module handle */

	bool		mac_suspended_for_halt; /* MAC Is suspended based on the halt req */
	uint16		ucode_dump_bufsize; /* buf size needed for ucode dump on fatal err */
	bool		fatal_error_dump_done; /* fatal error log gathered already */
	uint32	lifetime_tsftimer_cache; /* tsf_timerlow cache (us) */
	uint32	lifetime_osltime_cache;  /* OSL_SYSUPTIME() cache (ms) */
	uint8		join_timeout;
	uint8		scan_timeout;

	wlc_perf_utils_t *pui;		/* perf utils handle */
	wlc_chanctxt_info_t *chanctxt_info;	/* channel context module */
	wlc_asdb_t *asdb;		/* ASDB Module Information Structure */

#ifdef ACKSUPR_MAC_FILTER
	wlc_addrmatch_info_t *addrmatch_info;	/* amt_infio */
#endif /* ACKSUPR_MAC_FILTER */
	wlc_rsdb_policymgr_info_t *rsdb_policy_info; /* info ptr for rsdb policymgr module */
	wlc_debug_crash_info_t * debug_crash_info; /* Debug module handler */
	uint32	fifoflush_id;
	uint32	lifetime_ctrl;	/* pkt lifetime for control frames */
	mbool	wd_block_req;	/* used to bailout watchdog function */

	uint32	last_chansw_time;	/* last channel switch time in ms */

	const shmdefs_t *shmdefs;		/* Pointer to Auto-SHM structure */
	wlc_swdiv_info_t *swdiv;		/* software diversity handle */

	wlc_he_info_t *hei;		/* HE module handle */
	wlc_twt_info_t *twti;		/**< TWT module handle */

	wlc_tsync_t	*tsync;		/* Timestamp sync structure */

	wlc_calload_info_t	*cldi;		/**< calibration download module handler */
	wlc_fragdur_info_t	*fragdur;	/* frag_dur module information structure */
	char		*dump_args;	/* arguments pointer for wl dump */
	bool		_dma_ct;	/* true if BCM_DMA_CT (Cut Through DMA) feature is enabled.
					 * In future, plan is to have this set/cleared
					 * depending on MAC capability for allowing CT.
					 */
	uint32	_dma_ct_flags;	/* CTDMA flags controlled by ctdma/mu_features */
	/* MBO */
	wlc_mbo_info_t *mbo;         /* MBO instance */
	bool		wet_enab;		/* true if wet mode, for security usage */
	wlc_rx_hc_t * rx_hc;

	/* indicates the ac/tid mapping type used by pcie bus layer */
	uint8 pciedev_prio_map;

	wl_mimo_meas_metrics_t  *mimo_siso_metrics;		/* mimo_siso counters */
	wl_mimo_meas_metrics_t  *last_mimo_siso_cnt;	/* last mimo_siso counter */
	bool	core_boot_init;

	wlc_tx_stall_info_t * tx_stall;
	/* ====== !!! ADD NEW FIELDS ABOVE HERE !!! ====== */

};

#if defined(HC_RX_HANDLER) || defined(HC_TX_HANDLER)
/** context for xtlv unpack callback fn */
struct wlc_hc_ctx {
	wlc_info_t *wlc;
	wlc_if_t *wlcif;
};
int wlc_hc_unpack_idlist(bcm_xtlv_t *id_list, uint16 *ids, uint *count);

#endif

#define DMA_CT_PRECONFIGURED		(1 << 0)
#define DMA_CT_IOCTL_OVERRIDE		(1 << 1)
#define DMA_CT_IOCTL_CONFIG		(1 << 2)

/** wlc shared structure for common wlc informations for different wlc instances. */
struct wlc_cmn_info {
	wlc_info_t *wlc[MAX_RSDB_MAC_NUM]; /**< Array of all available WLCs */
	uint	num_d11_cores;
	int8	rsdb_mode;	/**< bit0-6 => rsdb_mode. bit7 => auto or override */
	int8	ap_rsdb_mode;	/* Indicate whether AP's rsdb_mode is auto or not */
	bool    hostmem_access_enabled;	   /**< Is host memory access enabled? */
	bool	dualmac_rsdb;
	wl_lifetime_mg_t *lifetime_mg;
	struct wl_timer *rsdb_clone_timer;
	struct wl_timer *mchan_clone_timer;
	int8	rsdb_mode_target; /* Target mode for Mode switch */
	mbool rsdb_auto_override;
	wlc_rateset_t *max_rateset; /* max supported rateset */
	bool	hwreinit;
	uint	ps_multista; /* power save in multi sta */
	mbool	core_wake; /* indicating each bsscfg core wake state */
	bool reinit_active;	/* during reinit allows pending backplane accesses to finish */
	mbool rsdb_move_ovr;		/* RSDB Move override */
	bool rxfrags_disabled;
	bool poolreorg_during_D3;
	uint16 frwd_resrv_bufcnt;
};

/** antsel module specific state */
/* RSDB move overrides */
#define WLC_RSDB_MOVE_OVR_IOVAR		0x00000001
#define WLC_RSDB_MOVE_FORCE_IOVAR	0x00000002
#define WLC_RSDB_MOVE_OVR_PM		0x00000004
#define WLC_RSDB_MOVE_OVR_RSSI		0x00000008
struct antsel_info {
	wlc_info_t *wlc;		/**< pointer to main wlc structure */
	wlc_pub_t *pub;			/**< pointer to public fn */
	uint8	antsel_type;		/**< Type of boardlevel mimo antenna switch-logic
					 * 0 = N/A, 1 = 2x4 board, 2 = 2x3 CB2 board
					 */
	uint8 antsel_antswitch;		/**< board level antenna switch type */
	bool antsel_avail;		/**< Ant selection availability (SROM based) */
	wlc_antselcfg_t antcfg_11n;	/**< antenna configuration */
	wlc_antselcfg_t antcfg_cur;	/**< current antenna config (auto) */
};

/**
 * Common tx header info struct
 * This struct is an accommodation for the different ucode headers for
 * pre-rev40 and rev40+ d11 cores (d11txh_t and d11actxh_t).
 * This struct can be populated from a packet with wlc_get_txh_info(), and the common fields
 * can be written back out the the ucode headers with wlc_set_txh_info().
 * There are also a handful of routines to update a single field and write out the change
 * to the ucode header without calling the full wlc_set_txh_info().
 */
struct wlc_txh_info {
	uint32 Tstamp;
	uint16 TxFrameID;
	uint16 MacTxControlLow;
	uint16 MacTxControlHigh;
	uint16 PhyTxControlWord0;
	uint16 PhyTxControlWord1;
	uint16 PhyTxControlWord2;
	/* Additional ctl words for 11ax STA */
	uint16 PhyTxControlWord3;
	uint16 PhyTxControlWord4;
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
};

/* definitions and structure used to retrieve txpwrs */
#define BW20_IDX        0
#define BW40_IDX        1
#define BW80_IDX        2
#define TXBF_OFF_IDX    0
#define TXBF_ON_IDX     1
typedef struct txpwr204080 {
	uint8 pbw[BW_80MHZ - BW_20MHZ+1][2];
} txpwr204080_t;

#ifdef PROP_TXSTATUS
#ifdef BCMPCIEDEV_ENABLED
/* For pcie protxstatus signals terminate at bus layer */
/* So setting the macro to one for pcie */
#define HOST_PROPTXSTATUS_ACTIVATED(wlc) 1
#else
#define HOST_PROPTXSTATUS_ACTIVATED(wlc) \
		(!!((wlc)->wlfc_flags & WLFC_FLAGS_HOST_PROPTXSTATUS_ACTIVE))
#endif /* BCMPCIEDEV */
#endif /* PROP_TXSTATUS */

#if defined(WLATF_DONGLE)
#define PCIEDEV_AC_PRIO_MAP	 0
#define PCIEDEV_TID_PRIO_MAP 1
#endif /* WLATF_DONGLE */

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

/**
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

/* Default sync ID with which first scan abort event is sent to host */
#define WLC_DEFAULT_SYNCID				0xFFFF

#define WLC_EXPTIME_END_TIME	10000	/* exit exptime state if we get frames without expiration
					 * after last expired frame (ms)
					 */

#define	BCM256QAM_DSAB(wlc)	(((wlc_info_t*)(wlc))->pub->boardflags & BFL_DIS_256QAM)

/* BTCX architecture after M93 days  */
#define BT3P_HW_COEX(wlc)  D11REV_GE(((wlc_info_t*)(wlc))->pub->corerev, 15)

#define IS_BTCX_FULLTDM(mode) ((mode == WL_BTC_FULLTDM) || (mode == WL_BTC_PREMPT))

#ifdef ACKSUPR_MAC_FILTER
#define WLC_ACKSUPR(wlc) (wlc->addrmatch_info)
#endif /* ACKSUPR_MAC_FILTER */

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

/* MU-MIMO Tx Support macros */
#ifdef WL_MU_TX

/* If the MU Tx feature is defined, have dongle builds always force on or off,
 * but have NIC and ROM builds use the runtime check.
 * Dongle RAM code do not need the runtime config, they should be compiled
 * on way or the other.
 */

	#define MU_TX_ENAB(wlc)         ((wlc)->_mu_tx)

#else /* WL_MU_TX */

#define MU_TX_ENAB(wlc)                 (0)

#endif /* WL_MU_TX */

/* MU-MIMO Rx Support macros */
#ifdef WL_MU_RX

/* If the MU Rx feature is defined, have dongle builds always force on or off,
 * but have NIC and ROM builds use the runtime check.
 * Dongle RAM code do not need the runtime config, they should be compiled
 * on way or the other.
 */

	#define MU_RX_ENAB(wlc)         ((wlc)->pub->cmn->_mu_rx)

#else /* WL_MU_RX */

#define MU_RX_ENAB(wlc)                 (0)

#endif /* WL_MU_RX */

/* BCM DMA Cut Through mode Support macros */
#ifdef BCM_DMA_CT

/* Check if hardware supports Cut Through Mode.
 * In the future check for Cut Through mode Capability bit when HW supports it.
 */
#define WLC_CT_HW_SUPPORTED(wlc) ((D11REV_GE((wlc)->pub->corerev, 64)) &&\
	(D11REV_LT((wlc)->pub->corerev, 80)))

/* If the BCM_DMA_CT feature is defined, have dongle builds always force on or off,
 * but have NIC and ROM builds use the runtime check.
 * Dongle RAM code do not need the runtime config, they should be compiled
 * on way or the other.
 */

	#define BCM_DMA_CT_ENAB(wlc)	((wlc)->_dma_ct)

#else /* BCM_DMA_CT */

#define BCM_DMA_CT_ENAB(wlc)		(0)

#endif /* BCM_DMA_CT */

#define	CHANNEL_BANDUNIT(wlc, ch) (((ch) <= CH_MAX_2G_CHANNEL) ? BAND_2G_INDEX : BAND_5G_INDEX)
#define	CHANNEL_BAND(wlc, ch) (((ch) <= CH_MAX_2G_CHANNEL) ? WLC_BAND_2G : WLC_BAND_5G)
#define CHSPEC_BANDUNIT(chspec) (CHSPEC_IS2G(chspec) ? BAND_2G_INDEX : BAND_5G_INDEX)

#define	OTHERBANDUNIT(wlc) ((uint)(((wlc)->band->bandunit == BAND_5G_INDEX)? \
						BAND_2G_INDEX : BAND_5G_INDEX))

#define VALID_BAND(wlc, band)	((band == WLC_BAND_AUTO) || (band == WLC_BAND_2G) || \
				 (band == WLC_BAND_5G))

#define IS_MBAND_UNLOCKED(wlc) \
	((NBANDS(wlc) > 1) && !(wlc)->bandlocked)

#include <phy_utils_api.h>
#define WLC_BAND_PI_RADIO_CHANSPEC phy_utils_get_chanspec((phy_info_t *)WLC_PI(wlc))

/* increment counter */
#define WLCNINC(cntr)
#define WLCNINCN(cntr, n)

#ifdef WLCNT
/* Increment interface stat */
#define WLCIFCNTINCR(_scb, _stat)						\
	do {									\
		if (_scb) {							\
			if (SCB_WDS(_scb)) {					\
				WLCNTINCR(_scb->wds->_cnt->_stat);		\
			} else {						\
				wlc_bsscfg_t *_bsscfg = SCB_BSSCFG(_scb);	\
				if (_bsscfg)					\
					WLCNTINCR(_bsscfg->wlcif->_cnt->_stat);	\
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
#ifdef WLCXO_DATA
#define	TXPKTPENDTOT(wlc)		((wlc)->core->txpktpend[0] + \
	                                 (wlc)->core->txpktpend[1] + \
	                                 (wlc)->core->txpktpend[2] + \
	                                 (wlc)->core->txpktpend[3] + \
	                                 (wlc)->core->htxpktpend[0] + \
	                                 (wlc)->core->htxpktpend[1] + \
	                                 (wlc)->core->htxpktpend[2] + \
	                                 (wlc)->core->htxpktpend[3])
#define TXPKTPENDGET(wlc, fifo)		((wlc)->core->txpktpend[(fifo)] + \
	                                 (wlc)->core->htxpktpend[(fifo)])
#elif defined(WLCXO_CTRL)
#define TXPKTPENDTOT(wlc) \
	((WLCXO_ENAB(wlc->pub) ? __wlc_cxo_c2d_txpktpend((wlc)->ipc, (wlc), 0, TRUE) : 0) + \
	                        ((wlc)->core->txpktpendtot))
#define TXPKTPENDGET(wlc, fifo)	\
	((WLCXO_ENAB(wlc->pub) ? __wlc_cxo_c2d_txpktpend((wlc)->ipc, (wlc), (fifo), FALSE) : 0) + \
	                        (wlc)->core->txpktpend[(fifo)])
#else /* !WLCXO_CTRL && !WLCXO_DATA */
#define	TXPKTPENDTOT(wlc)		((wlc)->core->txpktpendtot)
#define TXPKTPENDGET(wlc, fifo)		((wlc)->core->txpktpend[(fifo)])
#endif /* !WLCXO_CTRL && !WLCXO_DATA */
#define TXPKTPENDINC(wlc, fifo, val)					\
do {									\
	if (fifo < TX_BCMC_FIFO ||					\
		(BCM_DMA_CT_ENAB(wlc) && fifo >= TX_FIFO_EXT_START))	\
		wlc->core->txpktpendtot += val;				\
	wlc->core->txpktpend[fifo] += val;				\
} while (0)
#define TXPKTPENDDEC(wlc, fifo, val)					\
do {									\
	if (fifo < TX_BCMC_FIFO ||					\
		(BCM_DMA_CT_ENAB(wlc) && fifo >= TX_FIFO_EXT_START))	\
		wlc->core->txpktpendtot -= val;				\
	wlc->core->txpktpend[fifo] -= val;				\
} while (0)
#define TXPKTPENDCLR(wlc, fifo)	TXPKTPENDDEC(wlc, fifo, wlc->core->txpktpend[fifo])

/* For a monolithic driver, txavail and getnexttxp functionality is handled
 * by the dma/pio interface
 */
#ifdef WLCXO_DATA
/**
 * The reason behind checking for at least 32 or more entries is to allow host based data traffic
 * when there is continuous high thru'put offload data traffic. Doing this way host traffic has some
 * descriptors reserved for its transmissions. This scenario specifically happens for bit-torrent
 * traffic tests. Normal iperf tests don't create this problem scenario.
 */
#define TXAVAIL(wlc, fifo)		((*(wlc)->core->txavail[(fifo)] >= 32) ? \
	                                 (*(wlc)->core->txavail[(fifo)]): 0)
#elif defined(WLCXO_CTRL)
#define TXAVAIL(wlc, fifo)		(WLCXO_ENAB(wlc->pub) ? \
	                                 __wlc_cxo_c2d_txavail((wlc)->ipc, (wlc), (fifo)) : \
	                                 *(wlc)->core->txavail[(fifo)])
#elif !defined(TXAVAIL)
#define TXAVAIL(wlc, fifo)		(*(wlc)->core->txavail[(fifo)])
#endif

#ifdef WLCXO_DATA
#define HTXPKTPENDINC(wlc, fifo, val)	((wlc)->core->htxpktpend[(fifo)] += (val))
#define HTXPKTPENDDEC(wlc, fifo, val)	((wlc)->core->htxpktpend[(fifo)] -= (val))
#endif

#ifdef WLCXO_CTRL
#define GETNEXTTXP(wlc, fifo) (!PIO_ENAB((wlc)->pub) ? \
	wlc_cxo_ctrl_desc_txrc_getnexttxp(wlc, fifo, HNDDMA_RANGE_TRANSMITTED) : \
	wlc_pio_getnexttxp(WLC_HW_PIO(wlc, fifo)))
#elif !defined(GETNEXTTXP) /* WLCXO_CTRL */
#ifndef WL_DMA_DESC_TXRC
#define GETNEXTTXP(wlc, fifo) (!PIO_ENAB((wlc)->pub) ? \
	 dma_getnexttxp(WLC_HW_DI(wlc, fifo), HNDDMA_RANGE_TRANSMITTED) : \
	 wlc_pio_getnexttxp(WLC_HW_PIO(wlc, fifo)))
#else
#define GETNEXTTXP(wlc, fifo) (!PIO_ENAB((wlc)->pub) ? \
	wlc_cxo_ctrl_desc_txrc_getnexttxp(wlc, fifo, HNDDMA_RANGE_TRANSMITTED) : \
	wlc_pio_getnexttxp(WLC_HW_PIO(wlc, fifo)))
#endif /* WL_DMA_DESC_TXRC */
#endif /* WLCXO_CTRL */

/* Macro used to match ssid. */
#define WLC_IS_MATCH_SSID(wlc, ssid1, ssid2, len1, len2) \
	((void)(wlc), (len1) == (len2) && !bcmp((ssid1), (ssid2), (len1)))

/* Calculate Length for long format 11ax TXD */
#define D11_AX_TXH_LEN_EX(wlc, pofs)		((D11REV_GE((wlc)->pub->corerev, 80)) ? \
						(D11AX_TXH_LEN(0, 0, pofs)) : \
						(D11_PRE_AX_TXH_LEN_EX(wlc)))

/* Calculate Length for long format pre 11ax TXD */
#define D11_PRE_AX_TXH_LEN_EX(wlc)	(D11REV_GE((wlc)->pub->corerev, 40) ? D11AC_TXH_LEN : \
					(D11_TXH_LEN + D11_PHY_HDR_LEN))

/**
 * Calculate Length for long format pre and post 11ax TXD
 *
 * Todo : for 11ax cases calculate power offset from wlc (for now use static value of 2 for 4369)
 */
#ifdef WL11AX
#define D11_TXH_LEN_EX(wlc)		(D11REV_GE((wlc)->pub->corerev, 80) ? \
					(D11_AX_TXH_LEN_EX(wlc, 2)) : \
					(D11_PRE_AX_TXH_LEN_EX(wlc)))

#define D11_TXH_SHORT_LEN(wlc)		(D11REV_GE((wlc)->pub->corerev, 80) ? \
					(D11AX_TXH_SHORT_LEN) : \
					(D11REV_GE((wlc)->pub->corerev, 40) ? \
					(D11AC_TXH_SHORT_LEN) : \
					(D11_TXH_LEN + D11_PHY_HDR_LEN)))
#else /* !WL11AX */
#define D11_TXH_LEN_EX(wlc)		(D11_PRE_AX_TXH_LEN_EX(wlc))

#define D11_TXH_SHORT_LEN(wlc)		(D11REV_GE((wlc)->pub->corerev, 40) ? \
					(D11AC_TXH_SHORT_LEN) : \
					(D11_TXH_LEN + D11_PHY_HDR_LEN))
#endif /* WL11AX */

#define TXFIFO_SIZE_UNIT(r)		(D11REV_GE(r, 40) ? \
					TXFIFO_AC_SIZE_PER_UNIT : TXFIFO_SIZE_PER_UNIT)

#define D11AC2_PHY_SUPPORT(wlc)		(D11REV_GE((wlc)->pub->corerev, 64) ||\
					D11REV_IS((wlc)->pub->corerev, 61))

/* Adjustment in TxPwrMin advertised to push it slightly above the minimum supported */
#define WLC_MINTXPWR_OFFSET		4

#define SPURWAR_OVERRIDE_OFF	0
#define SPURWAR_OVERRIDE_X51A	1
#define SPURWAR_OVERRIDE_DBPAD	2

#define WLC_FATAL_ERROR(wlc) wlc_fatal_error(wlc)
extern void wlc_fatal_error(wlc_info_t *wlc);
extern void wlc_recv(wlc_info_t *wlc, void *p);
extern void wlc_recover_pkts(wlc_info_t *wlc, uint queue);

extern int wlc_pkt_get_vht_hdr(wlc_info_t* wlc, void* p, d11actxh_t **hdrPtrPtr);
#ifdef WL11AX
extern int wlc_pkt_get_he_hdr(wlc_info_t* wlc, void* p, d11axtxh_t **hdrPtrPtr);
#endif /* WL11AX */
extern struct dot11_header* wlc_pkt_get_d11_hdr(wlc_info_t* wlc, void *p);
extern void wlc_pkt_set_ack(wlc_info_t* wlc, void* p, bool want_ack);
extern void wlc_pkt_set_core(wlc_info_t* wlc, void* p, uint8 core);
extern void wlc_pkt_set_txpwr_offset(wlc_info_t* wlc, void* p, uint16 pwr_offset);

extern void wlc_write_template_ram(wlc_info_t *wlc, int offset, int len, void *buf);
extern void wlc_write_hw_bcntemplates(wlc_info_t *wlc, void *bcn, int len, bool both);
extern void wlc_mute(wlc_info_t *wlc, bool on, mbool flags);
extern void wlc_read_tsf(wlc_info_t* wlc, uint32* tsf_l_ptr, uint32* tsf_h_ptr);
extern uint32 wlc_read_usec_timer(wlc_info_t* wlc);
extern void wlc_set_cwmin(wlc_info_t *wlc, uint16 newmin);
extern void wlc_set_cwmax(wlc_info_t *wlc, uint16 newmax);
extern void wlc_fifoerrors(wlc_info_t *wlc);
extern void wlc_ampdu_mode_upd(wlc_info_t *wlc, uint8 mode);
extern void wlc_pllreq(wlc_info_t *wlc, bool set, mbool req_bit);
extern void wlc_update_phy_mode(wlc_info_t *wlc, uint32 phy_mode);
extern void wlc_reset_bmac_done(wlc_info_t *wlc);
extern void wlc_gptimer_wake_upd(wlc_info_t *wlc, mbool requestor, bool set);
extern void wlc_user_wake_upd(wlc_info_t *wlc, mbool requestor, bool set);

#ifdef ENABLE_CORECAPTURE
extern int wlc_dump_mem(wlc_info_t *wlc, int type);
#endif /* ENABLE_CORECAPTURE */

#define wlc_log(wlc, str, p1, p2)	do {} while (0)

/* API in HIGH only or monolithic driver */
extern wlc_if_t *wlc_wlcif_alloc(wlc_info_t *wlc, osl_t *osh, uint8 type, wlc_txq_info_t *qi);
extern void wlc_wlcif_free(wlc_info_t *wlc, osl_t *osh, wlc_if_t *wlcif);
extern void wlc_link(wlc_info_t *wlc, bool isup, struct ether_addr *addr, wlc_bsscfg_t *bsscfg,
	uint reason);

extern uint8 wlc_bss_psscb_getcnt(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

extern bool wlc_valid_rate(wlc_info_t *wlc, ratespec_t rate, int band, bool verbose);
extern void wlc_do_chanswitch(wlc_bsscfg_t *cfg, chanspec_t newchspec);
extern int wlc_senddisassoc(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
	const struct ether_addr *da, const struct ether_addr *bssid,
	const struct ether_addr *sa, uint16 reason_code);
extern int wlc_senddisassoc_ex(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
	const struct ether_addr *da, const struct ether_addr *bssid,
	const struct ether_addr *sa, uint16 reason_code,
	int (*pre_send_fn)(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt,
	void* arg, void *extra_arg),
	void *arg, void *extra_arg);

extern void *wlc_sendassocreq(wlc_info_t *wlc, wlc_bss_info_t *bi, struct scb *scb, bool reassoc);
extern void wlc_BSSinit(wlc_info_t *wlc, wlc_bss_info_t *bi, wlc_bsscfg_t *cfg, int start);
extern void wlc_full_phy_cal(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint8 reason);

extern uint8* wlc_write_brcm_ht_cap_ie(wlc_info_t *wlc, uint8 *cp, int buflen, ht_cap_ie_t *cap_ie);
extern uint8* wlc_write_brcm_ht_add_ie(wlc_info_t *wlc, uint8 *cp, int buflen, ht_add_ie_t *add_ie);
extern int wlc_parse_rates(wlc_info_t *wlc, uchar *tlvs, uint tlvs_len, wlc_bss_info_t *bi,
	wlc_rateset_t *rates);
extern int wlc_combine_rateset(wlc_info_t *wlc, wlc_rateset_t *sup, wlc_rateset_t *ext,
	wlc_rateset_t *rates);
extern void *wlc_sendauth(wlc_bsscfg_t *cfg, struct ether_addr *ea, struct ether_addr *bssid,
	struct scb *scb, int auth_alg, int auth_seq, int auth_status,
	uint8 *challenge_text, bool short_preamble);

extern void wlc_scb_disassoc_cleanup(wlc_info_t *wlc, struct scb *scb);
extern void wlc_cwmin_gphy_update(wlc_info_t *wlc, wlc_rateset_t *rs, bool associated);
extern uchar wlc_assocscb_getcnt(wlc_info_t *wlc);
extern uchar wlc_bss_assocscb_getcnt(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
extern void wlc_ap_upd(wlc_info_t* wlc,  wlc_bsscfg_t *bsscfg);
extern int wlc_minimal_up(wlc_info_t* wlc);
extern int wlc_minimal_down(wlc_info_t* wlc);

/* helper functions */
extern bool wlc_rateset_isofdm(uint count, uint8 *rates);
extern void wlc_req_wd_block(wlc_info_t *wlc, uint set_clear, uint req);

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

#define PROBE_REQ_EVT_MASK	0x01	/* For eventing */
#define PROBE_REQ_PRMS_MASK	0x04	/* For promiscous mode */
#define PROBE_REQ_PROBRESP_MASK	0x08	/* For SW Probe Response */
#define PROBE_REQ_PROBRESP_P2P_MASK	0x10	/* For P2P SW Probe Response */

extern void wlc_enable_probe_req(wlc_info_t *wlc, uint32 mask, uint32 val);

#define PROBE_RESP_P2P_MASK      1	/* For P2P */
#define PROBE_RESP_SW_MASK       2	/* For SW Probe Response */
#define PROBE_RESP_BSD_MASK      4	/* For FreeBSD/NetBSD */

extern void wlc_disable_probe_resp(wlc_info_t *wlc, uint32 mask, uint32 val);

extern void wlc_ibss_disable(wlc_bsscfg_t *cfg);
extern void wlc_ibss_enable(wlc_bsscfg_t *cfg);
extern void wlc_ibss_disable_all(wlc_info_t *wlc);
extern void wlc_ibss_enable_all(wlc_info_t *wlc);

extern bool wlc_sendnulldata(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct ether_addr *da,
	uint rate_override, uint32 pktflags, int prio,
	int (*pre_send_fn)(wlc_info_t*wlc, wlc_bsscfg_t*cfg, void*p, void*data), void *data);
extern void *wlc_nulldata_template(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct ether_addr *da);

extern int wlc_set_gmode(wlc_info_t *wlc, uint8 gmode, bool config);

extern void wlc_mac_bcn_promisc(wlc_info_t *wlc);
extern void wlc_mac_promisc(wlc_info_t *wlc);
extern void wlc_sendprobe(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	const uint8 ssid[], int ssid_len,
	const struct ether_addr *sa, const struct ether_addr *da, const struct ether_addr *bssid,
	ratespec_t rate_override, uint8 *extra_ie, int extra_ie_len);
extern uint16 wlc_prep80211_raw(wlc_info_t *wlc, wlc_if_t *wlcif,
	uint ac, void *p, ratespec_t rspec, uint *outfifo);

extern int wlc_send_action_err(wlc_info_t *wlc, struct dot11_management_header *hdr,
	uint8 *body, int body_len);

extern void *wlc_frame_get_mgmt(wlc_info_t *wlc, uint16 fc, const struct ether_addr *da,
	const struct ether_addr *sa, const struct ether_addr *bssid, uint body_len,
	uint8 **pbody);
/* variant of above when iv and tail are needed - e.g. MFP, CCX */
extern void* wlc_frame_get_mgmt_ex(wlc_info_t *wlc, uint16 fc,
	const struct ether_addr *da, const struct ether_addr *sa,
	const struct ether_addr *bssid, uint body_len, uint8 **pbody,
	uint iv_len, uint tail_len);

extern void *wlc_frame_get_action(wlc_info_t *wlc, const struct ether_addr *da,
	const struct ether_addr *sa, const struct ether_addr *bssid, uint body_len,
	uint8 **pbody, uint8 cat);
extern void *wlc_frame_get_ctl(wlc_info_t *wlc, uint len);
extern bool wlc_sendmgmt(wlc_info_t *wlc, void *p, wlc_txq_info_t *qi, struct scb *scb);
extern bool wlc_queue_80211_frag(wlc_info_t *wlc, void *p, wlc_txq_info_t *qi, struct scb *scb,
	wlc_bsscfg_t *bsscfg, bool preamble, wlc_key_t *key, ratespec_t rate_or);
extern bool wlc_sendctl(wlc_info_t *wlc, void *p, wlc_txq_info_t *qi, struct scb *scb, uint fifo,
	ratespec_t rate_or, bool enq_only);
extern uint16 wlc_calc_lsig_len(wlc_info_t *wlc, ratespec_t ratespec, uint mac_len);
extern uint16 wlc_compute_rtscts_dur(wlc_info_t *wlc, bool cts_only, ratespec_t rts_rate,
	ratespec_t frame_rate, uint8 rts_preamble_type, uint8 frame_preamble_type,
	uint frame_len, bool ba);

extern void wlc_tbtt(wlc_info_t *wlc, d11regs_t *regs);
extern void wlc_bss_tbtt(wlc_bsscfg_t *cfg);


extern bool wlc_ps_check(wlc_info_t *wlc);
extern void wlc_reprate_init(wlc_info_t *wlc);
extern void wlc_exptime_start(wlc_info_t *wlc);
extern void wlc_exptime_check_end(wlc_info_t *wlc);
extern int wlc_rfaware_lifetime_set(wlc_info_t *wlc, uint16 lifetime);

#ifdef STA
extern void wlc_set_pmoverride(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_pmpending(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_pspoll(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_pmawakebcn(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_apsd_stausp(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_uatbtt(wlc_bsscfg_t *cfg, bool state);
extern void wlc_update_bcn_info(wlc_bsscfg_t *cfg, bool state);
extern void wlc_set_dtim_programmed(wlc_bsscfg_t *cfg, bool state);
extern int wlc_set_dtim_period(wlc_bsscfg_t *cfg, uint16 dtim);
extern int wlc_get_dtim_period(wlc_bsscfg_t *cfg, uint16* dtim);
extern void wlc_update_pmstate(wlc_bsscfg_t *cfg, uint txstatus);
extern void wlc_module_set_pmstate(wlc_bsscfg_t *cfg, bool state, mbool moduleId);
extern void wlc_set_pmstate(wlc_bsscfg_t *cfg, bool state);
extern void wlc_reset_pmstate(wlc_bsscfg_t *cfg);
extern void wlc_pm_pending_complete(wlc_info_t *wlc);
extern void wlc_pm2_sleep_ret_timer_start(wlc_bsscfg_t *cfg, uint period);
extern void wlc_pm2_sleep_ret_timer_stop(wlc_bsscfg_t *cfg);
extern void wlc_pm2_ret_upd_last_wake_time(wlc_bsscfg_t *cfg, uint32 *tsf_l);
extern int wlc_set_pm_mode(wlc_info_t *wlc, int val, wlc_bsscfg_t *bsscfg);
extern void wlc_set_pmenabled(wlc_bsscfg_t *cfg, bool state);
extern int wlc_pm_bcnrx_set(wlc_info_t *wlc, bool enable);
extern void wlc_pm_bcnrx_disable(wlc_info_t *wlc);
extern bool wlc_pm_bcnrx_allowed(wlc_info_t *wlc);
#ifdef WLPM_BCNRX
void wlc_pm_bcnrx_init(wlc_info_t *wlc);
#endif

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


#else
#define wlc_set_pmstate(a, b)
#define wlc_stas_active(a) 0
#endif	/* STA */

/* Shared memory access */
extern void wlc_write_shm(wlc_info_t *wlc, uint offset, uint16 v);
extern void wlc_update_shm(wlc_info_t *wlc, uint offset, uint16 v, uint16 mask);
extern uint16 wlc_read_shm(wlc_info_t *wlc, uint offset);
extern void wlc_set_shm(wlc_info_t *wlc, uint offset, uint16 v, int len);
extern void wlc_copyto_shm(wlc_info_t *wlc, uint offset, const void* buf, int len);
extern void wlc_copyfrom_shm(wlc_info_t *wlc, uint offset, void* buf, int len);
#if defined(WL_PSMX)
extern uint16 wlc_read_shmx(wlc_info_t *wlc, uint offset);
extern void wlc_write_shmx(wlc_info_t *wlc, uint offset, uint16 v);
extern uint16 wlc_read_macregx(wlc_info_t *wlc, uint offset);
extern void wlc_write_macregx(wlc_info_t *wlc, uint offset, uint16 v);
#endif /* WL_PSMX */
#ifdef WL_HWKTAB
extern void wlc_copyto_keytbl(wlc_info_t *wlc, uint offset, const uint8* buf, int len);
extern void wlc_copyfrom_keytbl(wlc_info_t *wlc, uint offset, uint8* buf, int len);
extern void wlc_set_keytbl(wlc_info_t *wlc_hw, uint offset, uint32 v, int len);
#else
#define wlc_copyto_keytbl(wlc, offset, buf, len) do {} while (0)
#define wlc_copyfrom_keytbl(wlc, offset, buf, len) do {} while (0)
#define wlc_set_keytbl(wlc, offset, v, len) do {} while (0)
#endif /* WL_HWKTAB */

extern bool wlc_down_for_mpc(wlc_info_t *wlc);

extern void wlc_update_beacon(wlc_info_t *wlc);
extern void wlc_bss_update_beacon(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

extern void wlc_update_probe_resp(wlc_info_t *wlc, bool suspend);
extern void wlc_bss_update_probe_resp(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool suspend);
extern void wlc_bcn_prb_template(wlc_info_t *wlc, uint type,
	ratespec_t bcn_rate, wlc_bsscfg_t *cfg, uint16 *buf, int *len);
extern void wlc_bcn_prb_body(wlc_info_t *wlc, uint type, wlc_bsscfg_t *cfg,
	uint8 *bcn, int *bcn_len, bool legacy_tpl);
extern int wlc_write_hw_bcnparams(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	uint16 *bcn, int len, ratespec_t bcn_rspec, bool suspend);

#ifdef STA
extern bool wlc_ismpc(wlc_info_t *wlc);
extern void wlc_radio_mpc_upd(wlc_info_t* wlc);

/* MPC off request because P2P is active. */
#define MPC_OFF_REQ_P2P_APP_ACTIVE (1 << 0)
/* MPC off request because TSYNC is active */
#define MPC_OFF_REQ_TSYNC_ACTIVE (1 << 1)
/* MPC off request because Tx Action Frame is active */
#define MPC_OFF_REQ_TX_ACTION_FRAME (1 << 2)

extern int wlc_mpc_off_req_set(wlc_info_t *wlc, mbool mask, mbool val);
#endif /* STA */

extern int
wlc_bss2wl_bss(wlc_info_t *wlc, wlc_bss_info_t *bi, wl_bss_info_t *to_bi, int to_bi_len,
	bool need_ies);

extern void wlc_radio_upd(wlc_info_t* wlc);
extern bool wlc_prec_enq(wlc_info_t *wlc, struct pktq *q, void *pkt, int prec);
extern bool wlc_prec_enq_head(wlc_info_t *wlc, struct pktq *q, void *pkt, int prec, bool head);
extern bool wlc_pkt_abs_supr_enq(wlc_info_t *wlc, struct scb *scb, void *pkt);

extern void wlc_rateset_elts(wlc_info_t *wlc, wlc_bsscfg_t *cfg, const wlc_rateset_t *rates,
	wlc_rateset_t *sup_rates, wlc_rateset_t *ext_rates);

extern void wlc_compute_plcp(wlc_info_t *wlc, ratespec_t rate, uint length, uint16 fc, uint8 *plcp);
extern uint wlc_calc_frame_time(wlc_info_t *wlc, ratespec_t ratespec,
	uint8 preamble_type, uint mac_len);
extern void wlc_processpmq(wlc_info_t *wlc);

#define WLC_RX_CHANNEL(rev, rxh)	D11REV_GE(rev, 80) ? \
	CHSPEC_CHANNEL((rxh)->ge80.uc_rxhdr.RxChan) : CHSPEC_CHANNEL((rxh)->lt80.RxChan)

extern void wlc_set_chanspec(wlc_info_t *wlc, chanspec_t chanspec, int reason);

extern bool wlc_update_brcm_ie(wlc_info_t *wlc);
extern bool wlc_bss_update_brcm_ie(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

extern void wlc_process_brcm_ie(wlc_info_t *wlc, struct scb *scb, brcm_ie_t *brcm_ie);
extern bcm_tlv_t *wlc_find_wme_ie(uint8 *tlvs, uint tlvs_len);

extern void wlc_ether_8023hdr(wlc_info_t *wlc, osl_t *osh, struct ether_header *eh, void *p);

extern uint16 wlc_sdu_etype(wlc_info_t *wlc, void *sdu);
extern uint8* wlc_sdu_data(wlc_info_t *wlc, void *sdu);

void wlc_datapath_log_dump(wlc_info_t *wlc, int tag);

#define wlc_validate_bcn_phytxctl(wlc, cfg) do {} while (0)

extern void wlc_switch_shortslot(wlc_info_t *wlc, bool shortslot);
extern void wlc_set_bssid(wlc_bsscfg_t *cfg);
extern void wlc_clear_bssid(wlc_bsscfg_t *cfg);
extern void wlc_tsf_adj(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint32 tgt_h, uint32 tgt_l,
	uint32 ref_h, uint32 ref_l, uint32 nxttbtt, uint32 bcnint, bool comp);
extern bool wlc_sendpspoll(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

extern int wlc_sta_info(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
                        const struct ether_addr *ea, void *buf, int len);

typedef struct {
	wlc_rateset_t *sup;
	wlc_rateset_t *ext;
	uint16 *chan;
} wlc_pre_parse_frame_t;

extern void wlc_set_ratetable(wlc_info_t *wlc);

extern int wlc_set_mac(wlc_bsscfg_t *cfg);
extern void wlc_clear_mac(wlc_bsscfg_t *cfg);

extern uint16 wlc_compute_bcn_payloadtsfoff(wlc_info_t *wlc, ratespec_t rspec);
extern uint16 wlc_compute_bcntsfoff(wlc_info_t *wlc, ratespec_t rspec,
	bool short_preamble, bool phydelay);
extern bool wlc_radio_disable(wlc_info_t *wlc);
extern bool wlc_nonerp_find(wlc_info_t *wlc, void *body, uint body_len, uint8 **erp, int *len);
extern bool wlc_erp_find(wlc_info_t *wlc, void *body, uint body_len, uint8 **erp, int *len);
extern void wlc_bcn_li_upd(wlc_info_t *wlc);
extern chanspec_t wlc_create_chspec(struct wlc_info *wlc, uint8 channel);
extern uint16 wlc_proc_time_us(wlc_info_t *wlc);
extern uint16 wlc_pretbtt_calc(wlc_bsscfg_t *cfg);
extern void wlc_pretbtt_set(wlc_bsscfg_t *cfg);
extern void wlc_dtimnoslp_set(wlc_bsscfg_t *cfg);
extern void wlc_macctrl_init(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
extern uint32 wlc_recover_tsf32(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh);
extern void wlc_recover_tsf64(wlc_info_t *wlc, wlc_d11rxhdr_t *wrxh, uint32 *tsf_h, uint32 *tsf_l);
#ifndef BCMNODOWN
extern void wlc_out(wlc_info_t *wlc);
#endif /* BCMNODOWN */
extern int wlc_bandlock(struct wlc_info *, int val);
extern int wlc_change_band(wlc_info_t *wlc, int val);
extern void wlc_pcie_war_ovr_update(wlc_info_t *wlc, uint8 aspm);
extern void wlc_pcie_power_save_enable(wlc_info_t *wlc, bool enable);

extern void wlc_lifetime_set(wlc_info_t *wlc, void *sdu, uint32 lifetime);

extern void wlc_set_home_chanspec(wlc_info_t *wlc, chanspec_t chanspec);

extern void wlc_update_bandwidth(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	chanspec_t bss_chspec);
extern chanspec_t wlc_get_cur_wider_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
#ifdef STA
extern void wlc_bss_clear_bssid(wlc_bsscfg_t *cfg);
extern void wlc_watchdog_upd(wlc_bsscfg_t *cfg, bool tbtt);
extern int wlc_pspoll_timer_upd(wlc_bsscfg_t *cfg, bool allow);
extern int wlc_apsd_trigger_upd(wlc_bsscfg_t *cfg, bool allow);
extern bool wlc_ps_allowed(wlc_bsscfg_t *cfg);
extern bool wlc_stay_awake(wlc_info_t *wlc);
#endif /* STA */

#ifdef WME
extern int wlc_wme_downgrade_fifo(wlc_info_t *wlc, uint* p_fifo, struct scb *scb);
#endif
extern uint wlc_wme_wmmac_check_fixup(wlc_info_t *wlc, struct scb *scb, void *p,
	uint8 prio, int *bcmerror);

extern void wlc_bss_list_free(wlc_info_t *wlc, wlc_bss_list_t *bss_list);
extern void wlc_bss_list_xfer(wlc_bss_list_t *from, wlc_bss_list_t *to);

extern void wlc_txh_iv_upd(wlc_info_t *wlc, d11txh_t *txh, uint8 *iv,
	wlc_key_t *key, const wlc_key_info_t *key_info);

void wlc_tsf_adjust(wlc_info_t *wlc, int delta);

#ifdef STA
#if defined(AP_KEEP_ALIVE)
extern void wlc_ap_keep_alive_count_update(wlc_info_t *wlc, uint16 keep_alive_time);
extern void wlc_ap_keep_alive_count_default(wlc_info_t *wlc);
#endif 
#endif /* STA */

#if defined(DMATXRC)
extern int wlc_phdr_attach(wlc_info_t *wlc);
extern void wlc_phdr_detach(wlc_info_t *wlc);
extern void wlc_phdr_fill(wlc_info_t* wlc);
extern void* wlc_phdr_get(wlc_info_t *wlc);
#endif

#ifdef WLRXOV
extern void wlc_rxov_timer(void *arg);
#endif
extern uint16 wlc_assoc_capability(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, wlc_bss_info_t *bi);
extern void wlc_txstop_intr(wlc_info_t *wlc);
extern void wlc_rfd_intr(wlc_info_t *wlc);

#if defined(IBSS_PEER_GROUP_KEY)
uint16 wlc_read_seckindxblk_secinfo_by_idx(wlc_info_t *wlc, uint index);
#endif 
uint16 wlc_read_amtinfo_by_idx(wlc_info_t *wlc, uint index);
void wlc_write_amtinfo_by_idx(wlc_info_t *wlc, uint index, uint16 val);

#ifdef WLCXO_CTRL
typedef enum txd_range txd_range_t;
extern void *wlc_cxo_ctrl_desc_txrc_getnexttxp(wlc_info_t *wlc, uint32 fifo, txd_range_t range);
#endif /* WLCXO_CTRL */

extern bool wlc_force_ht(wlc_info_t *wlc, bool force, bool *prev);
extern uint32 wlc_current_pmu_time(wlc_info_t *wlc);
#define CURRENT_PMU_TIME(wlc) wlc_current_pmu_time(wlc)

extern void wlc_ampdu_upd_pm(wlc_info_t *wlc, uint8 PM_mode);
extern void wlc_BSSinit_rateset_filter(wlc_bsscfg_t *cfg);

extern uint8 wlc_get_mcsallow(wlc_info_t *wlc, wlc_bsscfg_t *cfg);


extern uint16 wlc_mgmt_ctl_d11hdrs(wlc_info_t *wlc, void *pkt, struct scb *scb,
	uint queue, ratespec_t rspec_override);

/* check field of specified length from ptr is valid for pkt */
extern bool wlc_valid_pkt_field(osl_t *osh, void *pkt, void *ptr, int len);

extern void wlc_cfg_set_pmstate_upd(wlc_bsscfg_t *cfg, bool pmenabled);
extern void wlc_check_txq_fc(wlc_info_t *wlc, wlc_txq_info_t *qi);
extern int wlc_prio2prec(wlc_info_t *wlc, int prio);
extern uint wlc_calc_cts_time(wlc_info_t *wlc, ratespec_t rate, uint8 preamble_type);
extern uint16 wlc_compute_frame_dur(wlc_info_t *wlc, ratespec_t rate,
	uint8 preamble_type, uint next_frag_len);
extern void wlc_fragthresh_set(wlc_info_t *wlc, uint8 ac, uint16 thresh);

extern void wlc_cck_plcp_set(int rate_500, uint length, uint8 *plcp);

/* currently the best mechanism for determining SIFS is the band in use */
#define SIFS(band) ((band)->bandtype == WLC_BAND_5G ? APHY_SIFS_TIME : BPHY_SIFS_TIME);

/* wlc_pdu_tx_params.flags */
#define	WLC_TX_PARAMS_SHORTPRE	0x01
#define WLC_TX_PARAMS_AWDL_AF_TS	0x02


typedef struct wlc_pdu_tx_params {
	uint flags;
	uint fifo;
	wlc_key_t *key;
	ratespec_t rspec_override;
} wlc_pdu_tx_params_t;

/* To inform the ucode of the last mcast frame posted so that it can clear moredata bit */
#define BCMCFID(wlc, fid) wlc_bmac_write_shm((wlc)->hw, M_BCMC_FID, (fid))

#ifdef STA
#ifdef WL_PWRSTATS
extern void wlc_connect_time_upd(wlc_info_t *wlc);
#endif /* WL_PWRSTATS */
#endif /* STA */

extern uint32 wlc_get_accum_pmdur(wlc_info_t *wlc);
extern uint32 wlc_get_mpc_dur(wlc_info_t *wlc);

/* Check if a bandwidth is valid for use with current cfg */
extern bool wlc_is_valid_bw(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint8 bandunit, uint16 bw);

/* find the maximum bandwidth channel that we can use from advertised channel */
extern chanspec_t wlc_max_valid_bw_chanspec(wlc_info_t *wlc, wlcband_t *band,
	wlc_bsscfg_t *cfg, chanspec_t chanspec);

extern void wlc_watchdog(void *arg);
extern uint32 wlc_watchdog_backup_bi(wlc_info_t * wlc);

extern void wlc_8023_etherhdr(wlc_info_t *wlc, osl_t *osh, void *p);

int wlc_isup(wlc_info_t *wlc);

#define SSTLOOKUP(proto) (((proto) == 0x80f3) || ((proto) == 0x8137))

/*
 * _EVENT_LOG macro expects the string and does not handle a pointer to the string.
 *  Hence MACRO is used to have identical string thereby compiler can optimize.
 */
#define WLC_MALLOC_ERR          "wl%d: %s: MALLOC(%d) failed, malloced %d bytes\n"
#define WLC_BSS_MALLOC_ERR	"wl%d.%d: %s: MALLOC(%d) failed, malloced %d bytes\n"

extern bool rfc894chk(wlc_info_t *wlc, struct dot11_llc_snap_header *lsh);
bool wlc_maccore_wake_state(wlc_info_t *wlc);

/** (de)authorize/(de)authenticate single station */
int wlc_scb_set_auth(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
	bool enable, uint32 flag, int rc);

extern void wlc_cts_to_nowhere(wlc_info_t *wlc, uint16 duration);

int wlc_validate_mac(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct ether_addr *addr);
extern uint32 wlc_need_reinit(wlc_info_t * wlc);

void wlc_bsscfg_set_current_bss_chan(wlc_bsscfg_t *bsscfg, chanspec_t cspec);
extern void wlc_bandinit_ordered(wlc_info_t *wlc, chanspec_t chanspec);
extern int BCMFASTPATH wlc_send_fwdpkt(wlc_info_t *wlc, void *p, wlc_if_t *wlcif);
extern bool wlc_mpccap(wlc_info_t* wlc);
extern void wlc_update_txpktsuccess_stats(wlc_info_t *wlc, struct scb *scb, uint32 pkt_len,
	uint8 prio, uint16 npkts);

void wlc_fill_mgmt_hdr(struct dot11_management_header *hdr, uint16 fc,
	const struct ether_addr *da, const struct ether_addr *sa,
	const struct ether_addr *bssid);

extern void wlc_srpwr_init(wlc_info_t *wlc);
extern void wlc_srpwr_request_on(wlc_info_t *wlc);
extern void wlc_srpwr_request_off(wlc_info_t *wlc);
extern void wlc_srpwr_spinwait(wlc_info_t *wlc);
extern bool wlc_srpwr_stat(wlc_info_t *wlc);
extern bool wlc_is_shared_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

extern int wlc_halt_hw(wlc_info_t * wlc);
#ifdef OCL
extern void wlc_stf_ocl_rssi_thresh_handling(wlc_bsscfg_t *bsscfg);
extern int wlc_get_ocl_meas_metrics(wlc_info_t *wlc, uint8 *destptr, int destlen);
#endif /* OCL */

#ifdef WL_MIMO_SISO_STATS
extern void wlc_mimo_siso_metrics_snapshot(wlc_info_t *wlc, bool reset_last, uint16 reason);
extern int wlc_get_mimo_siso_meas_metrics(wlc_info_t *wlc, uint8 *destptr, int destlen);
extern int wlc_mimo_siso_metrics_report(wlc_info_t *wlc, bool el);
#else /* stubs */
#define wlc_mimo_siso_metrics_snapshot(a, b, c)  do {} while (0)
#define wlc_get_mimo_siso_meas_metrics(a, b, c) (0)
#define wlc_mimo_siso_metrics_report(a, b) (0)
#endif /* WL_MIMO_SISO_STATS */

#ifdef WL_NAP
extern void wlc_stf_nap_rssi_thresh_handling(wlc_bsscfg_t *bsscfg);
#endif /* WL_NAP */

int wlc_hwrsscbs_alloc(wlc_info_t *wlc);
void wlc_hwrsscbs_free(wlc_info_t *wlc);

#ifdef NEW_PHY_CAL_ARCH
/* Set current operating channel */
int wlc_set_operchan(wlc_info_t *wlc, chanspec_t chanspec);
#endif /* NEW_PHY_CAL_ARCH */

#endif	/* _wlc_h_ */
