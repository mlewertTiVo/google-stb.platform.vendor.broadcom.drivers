/*
 * Common interface to the 802.11 Station Control Block (scb) structure
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
 * $Id: wlc_scb.h 640732 2016-05-30 13:13:25Z $
 */


#ifndef _wlc_scb_h_
#define _wlc_scb_h_

#include <typedefs.h>
#include <proto/ethernet.h>
#include <wlc_types.h>
#include <wlc_rate.h>
#include <wlioctl.h>
#include <wlc_bsscfg.h>

#ifndef WLCXO_CTRL
#define SCBPKTPENDGET(wlc, scb)	0
#define SCBPKTPENDSET(wlc, scb, val)
#define SCBPKTPENDINC(wlc, scb, val)
#define SCBPKTPENDDEC(wlc, scb, val)
#define SCBPKTPENDCLR(wlc, scb)
#endif /* WLCXO_CTRL */

#ifdef WLCNTSCB
typedef struct wlc_scb_stats {
	uint32 tx_pkts;			/**< # of packets transmitted (ucast) */
	uint32 tx_failures;		/**< # of packets failed */
	uint32 rx_ucast_pkts;		/**< # of unicast packets received */
	uint32 rx_mcast_pkts;		/**< # of multicast packets received */
	ratespec_t tx_rate;		/**< Rate of last successful tx frame */
	ratespec_t rx_rate;		/**< Rate of last successful rx frame */
	uint32 rx_decrypt_succeeds;	/**< # of packet decrypted successfully */
	uint32 rx_decrypt_failures;	/**< # of packet decrypted unsuccessfully */
	uint32 tx_mcast_pkts;		/**< # of mcast pkts txed */
	uint64 tx_ucast_bytes;		/**< data bytes txed (ucast) */
	uint64 tx_mcast_bytes;		/**< data bytes txed (mcast) */
	uint64 rx_ucast_bytes;		/**< data bytes recvd ucast */
	uint64 rx_mcast_bytes;		/**< data bytes recvd mcast */
	uint32 tx_pkts_retried;		/**< # of packets where a retry was necessary */
	uint32 tx_pkts_retry_exhausted;	/**< # of packets where a retry was exhausted */
	ratespec_t tx_rate_mgmt;	/**< Rate of last transmitted management frame */
	uint32 tx_rate_fallback;	/**< last used lowest fallback TX rate */
	uint32 rx_pkts_retried;		/**< # rx with retry bit set */
	uint32 tx_pkts_total;
	uint32 tx_pkts_retries;
	uint32 tx_pkts_fw_total;
	uint32 tx_pkts_fw_retries;
	uint32 tx_pkts_fw_retry_exhausted;
} wlc_scb_stats_t;
#endif /* WLCNTSCB */

/**
 * Information about a specific remote entity, and the relation between the local and that remote
 * entity. Station Control Block.
 */
struct scb {
	uint32	flags;		/**< various bit flags as defined below */
	uint32	flags2;		/**< various bit flags2 as defined below */
	uint32  flags3;		/**< various bit flags3 as defined below */
	wlc_bsscfg_t *bsscfg;	/**< bsscfg to which this scb belongs */
	struct ether_addr ea;	/**< station address, must be aligned */
	uint32	pktc_pps;	/**< pps counter for activating pktc */
	uint8	state;		/**< current state bitfield of auth/assoc process */
	bool	permanent;	/**< scb should not be reclaimed */
	uint8	mark;		/**< various marking bitfield */
	uint8	rx_lq_samp_req;	/**< recv link quality sampling request - fast path */
	uint	used;		/**< time of last use */
	uint32	assoctime;	/**< time of association */
	uint	bandunit;	/**< tha band it belongs to */
	uint32	WPA_auth;	/**< WPA: authenticated key management */
	uint32	wsec; /**< ucast security algo. should match key->algo. Needed before key is set */

	wlc_rateset_t	rateset;	/**< operational rates for this remote station */

	uint16	seqctl[NUMPRIO];	/**< seqctl of last received frame (for dups) */
	uint16	seqctl_nonqos;		/**< seqctl of last received frame (for dups) for
					 * non-QoS data and management
					 */
	uint16	seqnum[NUMPRIO];	/**< WME: driver maintained sw seqnum per priority */

	/* APSD configuration */
	struct {
		uint16		maxsplen;   /**< Maximum Service Period Length from assoc req */
		ac_bitmap_t	ac_defl;    /**< Bitmap of ACs enabled for APSD from assoc req */
		ac_bitmap_t	ac_trig;    /**< Bitmap of ACs currently trigger-enabled */
		ac_bitmap_t	ac_delv;    /**< Bitmap of ACs currently delivery-enabled */
	} apsd;

	uint16		aid;		/**< association ID */
	uint8		auth_alg;	/**< 802.11 authentication mode */
	bool		PS;		/**< remote STA in PS mode */
	uint8           ps_pretend;     /**< AP pretending STA is in PS mode */
	uint16		cap;		/**< sta's advertized capability field */
	wlc_if_t	*wds;		/**< per-port WDS cookie */
	tx_path_node_t	*tx_path;	/**< pkt tx path (allocated as scb cubby) */
#ifdef WL_CS_RESTRICT_RELEASE
	uint16	restrict_txwin;
	uint8	restrict_deadline;
#endif /* WL_CS_RESTRICT_RELEASE */
#ifdef WLPKTDLYSTAT
	/* it's allocated as an array, use 'delay_stats + ac' to access per ac element! */
	scb_delay_stats_t *delay_stats;	/**< per-AC delay stats (allocated as scb cubby) */
#ifdef WLPKTDLYSTAT_IND
	txdelay_params_t *txdelay_params;
#endif /* WLPKTDLYSTAT_IND */
#endif /* WLPKTDLYSTAT */
#ifdef WLCNTSCB
	wlc_scb_stats_t scb_stats;
#endif /* WLCNTSCB */
#if defined(STA) && defined(DBG_BCN_LOSS)
	struct wlc_scb_dbg_bcn dbg_bcn;
#endif
};

#define CXO_PAUSE_REASON_PS	(1 << 0)
#define CXO_PAUSE_REASON_PPS	(1 << 1)

/** Iterator for scb list */
struct scb_iter {
	struct scb	*next;			/**< next scb in bss */
	wlc_bsscfg_t	*next_bss;		/**< next bss pointer */
	bool		all;			/**< walk all bss or not */
};

#define SCB_BSSCFG(a)           ((a)->bsscfg)
#define SCB_WLC(a)              ((a)->bsscfg->wlc)

/** Initialize an scb iterator pre-fetching the next scb as it moves along the list */
void wlc_scb_iterinit(scb_module_t *scbstate, struct scb_iter *scbiter,
	wlc_bsscfg_t *bsscfg);
/** move the iterator */
struct scb *wlc_scb_iternext(scb_module_t *scbstate, struct scb_iter *scbiter);

/* Iterate thru' scbs of specified bss */
#define FOREACH_BSS_SCB(scbstate, scbiter, bss, scb) \
	for (wlc_scb_iterinit((scbstate), (scbiter), (bss)); \
	     ((scb) = wlc_scb_iternext((scbstate), (scbiter))) != NULL; )

/* Iterate thru' scbs of all bss. Use this only when needed. For most of
 * the cases above one should suffice.
 */
#define FOREACHSCB(scbstate, scbiter, scb) \
	for (wlc_scb_iterinit((scbstate), (scbiter), NULL); \
	     ((scb) = wlc_scb_iternext((scbstate), (scbiter))) != NULL; )

/* module attach/detach interface */
scb_module_t *wlc_scb_attach(wlc_info_t *wlc);
void wlc_scb_detach(scb_module_t *scbstate);

/* scb cubby cb functions */
typedef int (*scb_cubby_init_t)(void *, struct scb *);
typedef void (*scb_cubby_deinit_t)(void *, struct scb *);
/* return the secondary cubby size */
typedef uint (*scb_cubby_secsz_t)(void *, struct scb *);
typedef void (*scb_cubby_dump_t)(void *, struct scb *, struct bcmstrbuf *b);
typedef void (*scb_cubby_datapath_log_dump_t)(void *, struct scb *, int);

typedef struct scb_cubby_params {
	void *context;
	scb_cubby_init_t fn_init;
	scb_cubby_deinit_t fn_deinit;
	scb_cubby_dump_t fn_dump;
	scb_cubby_datapath_log_dump_t fn_data_log_dump;
	scb_cubby_secsz_t fn_secsz;
} scb_cubby_params_t;

/**
 * This function allocates an opaque cubby of the requested size in the scb container.
 * The cb functions fn_init/fn_deinit are called when a scb is allocated/freed.
 * The cb fn_secsz is called if registered to tell the secondary cubby size.
 * The functions are called with the context passed in and a scb pointer.
 * It returns a handle that can be used in macro SCB_CUBBY to retrieve the cubby.
 * Function returns a negative number on failure
 */
int wlc_scb_cubby_reserve_ext(wlc_info_t *wlc, uint size, scb_cubby_params_t *params);
int wlc_scb_cubby_reserve(wlc_info_t *wlc, uint size,
                          scb_cubby_init_t fn_init, scb_cubby_deinit_t fn_deinit,
                          scb_cubby_dump_t fn_dump, void *context);

/* macro to retrieve pointer to module specific opaque data in scb container */
#define SCB_CUBBY(scb, handle)	(void *)(((uint8 *)(scb)) + handle)

/**
 * This function allocates a secondary cubby in the secondary cubby area.
 */
void *wlc_scb_sec_cubby_alloc(wlc_info_t *wlc, struct scb *scb, uint secsz);
void wlc_scb_sec_cubby_free(wlc_info_t *wlc, struct scb *scb, void *secptr);

/* Cubby Name ID Registration for Datapath logging */

#define WLC_SCB_NAME_ID_INVALID 0xFF /**< error return for wlc_scb_cubby_name_register() */

/**
 * Cubby Name ID Registration: This fn associates a string with a unique ID (uint8).
 */
uint8 wlc_scb_cubby_name_register(scb_module_t *scbstate, const char *name);

/**
 * This fn dumps the Cubby Name registry built with wlc_scb_cubby_name_register().
 */
void wlc_scb_cubby_name_dump(scb_module_t *scbstate, int tag);

/*
 * Accessors
 */

struct wlcband *wlc_scbband(wlc_info_t *wlc, struct scb *scb);

/** Find station control block corresponding to the remote id */
struct scb *wlc_scbfind(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, const struct ether_addr *ea);

/** Lookup station control for ID. If not found, create a new entry. */
struct scb *wlc_scblookup(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, const struct ether_addr *ea);

/** Lookup station control for ID. If not found, create a new entry. */
struct scb *wlc_scblookupband(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
                              const struct ether_addr *ea, int bandunit);

/** Get scb from band */
struct scb *wlc_scbfindband(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
                            const struct ether_addr *ea, int bandunit);

/** Move the scb's band info */
void wlc_scb_update_band_for_cfg(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, chanspec_t chanspec);

extern struct scb *wlc_scbibssfindband(wlc_info_t *wlc, const struct ether_addr *ea,
	int bandunit, wlc_bsscfg_t **bsscfg);

/** Find the STA acorss all APs */
extern struct scb *wlc_scbapfind(wlc_info_t *wlc, const struct ether_addr *ea,
	wlc_bsscfg_t **bsscfg);

extern struct scb *wlc_scbbssfindband(wlc_info_t *wlc, const struct ether_addr *hwaddr,
	const struct ether_addr *ea, int bandunit, wlc_bsscfg_t **bsscfg);

struct scb *wlc_bcmcscb_alloc(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct wlcband *band);
#define wlc_bcmcscb_free(wlc, scb) wlc_scbfree(wlc, scb)
struct scb *wlc_hwrsscb_alloc(wlc_info_t *wlc, struct wlcband *band);
#define wlc_hwrsscb_free(wlc, scb) wlc_scbfree(wlc, scb)

bool wlc_scbfree(wlc_info_t *wlc, struct scb *remove);

/** * "|" operation */
void wlc_scb_setstatebit(wlc_info_t *wlc, struct scb *scb, uint8 state);

bool wlc_scbfree_pktpend(wlc_info_t *wlc, struct scb *remove, int32 *pktpend);

/** * "& ~" operation . */
void wlc_scb_clearstatebit(wlc_info_t *wlc, struct scb *scb, uint8 state);

/** * reset all state. the multi ssid array is cleared as well. */
void wlc_scb_resetstate(wlc_info_t *wlc, struct scb *scb);

void wlc_scb_reinit(wlc_info_t *wlc);

/** sort rates for a single scb */
void wlc_scb_sortrates(wlc_info_t *wlc, struct scb *scb);
/** sort rates for all scb in wlc */
void wlc_scblist_validaterates(wlc_info_t *wlc);

#if defined(WL_NAN)
extern void wlc_scb_awdl_free(struct wlc_info *wlc);
#endif

/* SCB flags */
#define SCB_NONERP		0x0001		/**< No ERP */
#define SCB_LONGSLOT		0x0002		/**< Long Slot */
#define SCB_SHORTPREAMBLE	0x0004		/**< Short Preamble ok */
#define SCB_8021XHDR		0x0008		/**< 802.1x Header */
#define SCB_WPA_SUP		0x0010		/**< 0 - authenticator, 1 - supplicant */
#define SCB_DEAUTH		0x0020		/**< 0 - ok to deauth, 1 - no (just did) */
#define SCB_WMECAP		0x0040		/**< WME Cap; may ONLY be set if WME_ENAB(wlc) */
#define SCB_WPSCAP		0x0080		/**< Peer is WPS capable */
#define SCB_BRCM		0x0100		/**< BRCM AP or STA */
#define SCB_WDS_LINKUP		0x0200		/**< WDS link up */
#define SCB_LEGACY_AES		0x0400		/**< legacy AES device */
#define SCB_MYAP		0x1000		/**< We are associated to this AP */
#define SCB_PENDING_PROBE	0x2000		/**< Probe is pending to this SCB */
#define SCB_AMSDUCAP		0x4000		/**< A-MSDU capable */
#define SCB_HTCAP		0x10000		/**< HT (MIMO) capable device */
#define SCB_RECV_PM		0x20000		/**< state of PM bit in last data frame recv'd */
#define SCB_AMPDUCAP		0x40000		/**< A-MPDU capable */
#define SCB_IS40		0x80000		/**< 40MHz capable */
#define SCB_NONGF		0x100000	/**< Not Green Field capable */
#define SCB_APSDCAP		0x200000	/**< APSD capable */
#define SCB_DEFRAG_INPROG	0x400000	/**< Defrag in progress, summary for all prio */
#define SCB_PENDING_PSPOLL	0x800000	/**< PS-Poll is pending to this SCB */
#define SCB_RIFSCAP		0x1000000	/**< RIFS capable */
#define SCB_HT40INTOLERANT	0x2000000	/**< 40 Intolerant */
#define SCB_WMEPS		0x4000000	/**< PS + WME w/o APSD capable */
#define SCB_SENT_APSD_TRIG	0x8000000	/**< APSD Trigger Null Frame was recently sent */
#define SCB_COEX_MGMT		0x10000000	/**< Coexistence Management supported */
#define SCB_IBSS_PEER		0x20000000	/**< Station is an IBSS peer */
#define SCB_STBCCAP		0x40000000	/**< STBC Capable */

/* scb flags2 */
#define SCB2_SGI20_CAP          0x00000001      /**< 20MHz SGI Capable */
#define SCB2_SGI40_CAP          0x00000002      /**< 40MHz SGI Capable */
#define SCB2_RX_LARGE_AGG       0x00000004      /**< device can rx large aggs */
#define SCB2_BCMC               0x00000008      /**< scb is used to support bc/mc traffic */
#define SCB2_HWRS               0x00000010	/**< scb is used to hold h/w rateset */
#define SCB2_WAIHDR             0x00000020      /**< WAI Header */
#define SCB2_P2P                0x00000040      /**< WiFi P2P */
#define SCB2_LDPCCAP            0x00000080      /**< LDPC Cap */
#define SCB2_BCMDCS             0x00000100      /**< BCM_DCS */
#define SCB2_MFP                0x00000200      /**< 802.11w MFP_ENABLE */
#define SCB2_SHA256             0x00000400      /**< sha256 for AKM */
#define SCB2_VHTCAP             0x00000800      /**< VHT (11ac) capable device */
#define SCB2_HECAP		0x00001000      /**< HE (11ax) capable device */
#define SCB2_TWT_REQCAP		0x00002000      /**< TWT Requestor capable device */
#define SCB2_TWT_RESPCAP	0x00004000      /**< TWT Responder capable device */
#define SCB2_IGN_SMPS		0x08000000 	/**< ignore SM PS update */
#define SCB2_IS80               0x10000000      /**< 80MHz capable */
#define SCB2_AMSDU_IN_AMPDU_CAP	0x20000000      /**< AMSDU over AMPDU */
#define SCB2_CCX_MFP		0x40000000	/**< CCX MFP enable */
#define SCB2_DWDS_ACTIVE	0x80000000      /**< DWDS is active */

/* scb flags3 */
#define SCB3_A4_DATA		0x00000001      /**< scb does 4 addr data frames */
#define SCB3_A4_NULLDATA	0x00000002	/**< scb does 4-addr null data frames */
#define SCB3_A4_8021X		0x00000004	/**< scb does 4-addr 8021x frames */
#define SCB3_FTM_INITIATOR	0x00000010	/**< fine timing meas initiator */
#define SCB3_FTM_RESPONDER	0x00000020	/**< fine timing meas responder */
#ifdef WL_RELMCAST
#define SCB3_RELMCAST		0x00000800	/**< Reliable Multicast */
#define SCB3_RELMCAST_NOACK	0x00001000	/**< Reliable Multicast No ACK rxed */
#endif
#define SCB3_PKTC		0x00002000      /**< Enable packet chaining */
#define SCB3_OPER_MODE_NOTIF    0x00004000      /**< 11ac Oper Mode Notif'n */
#define SCB3_RRM		0x00008000      /**< Radio Measurement */
#define SCB3_DWDS_CAP		0x00010000      /**< DWDS capable */
#define SCB3_IS_160		0x00020000      /**< VHT 160 cap */
#define SCB3_IS_80_80		0x00040000      /**< VHT 80+80 cap */
#define SCB3_1024QAM_CAP	0x00080000      /**< VHT 1024QAM rates cap */
#define SCB3_HT_PROP_RATES_CAP  0x00100000      /**< Broadcom proprietary 11n rates */
#define SCB3_MU_CAP		0x00200000      /**< VHT MU cap */
#define SCB3_IS_10		0x00400000	/* ULB 10 MHz Capable */
#define SCB3_IS_5		0x00800000	/* ULB 5 MHz Capable */
#define SCB3_IS_2P5		0x01000000	/* ULB 2.5MHz Capable */
#define SCB3_ECSA_SUPPORT       0x02000000      /* ECSA supported STA */
#define SCB3_AWDL_AGGR_CHANGE	0x04000000	/* Reduce tx agg for AWDL (Jira 49554)
						 * Reserve for AWDL compatibility
						 */
#define SCB3_CXO		0x08000000      /* Enable CXO */
#define SCB3_HT_BEAMFORMEE      0x10800000      /* Receive NDP Capable */
#define SCB3_HT_BEAMFORMER      0x20000000      /* Transmit NDP Capable */
#define SCB3_HE_TRIGGERED_TX	0x40000000		/* Use trigger queues for tx */

/* scb association state bitfield */
#define AUTHENTICATED		1	/**< 802.11 authenticated (open or shared key) */
#define ASSOCIATED		2	/**< 802.11 associated */
#define PENDING_AUTH		4	/**< Waiting for 802.11 authentication response */
#define PENDING_ASSOC		8	/**< Waiting for 802.11 association response */
#define AUTHORIZED		0x10	/**< 802.1X authorized */

/* scb association state helpers */
#define SCB_ASSOCIATED(a)	((a)->state & ASSOCIATED)
#define SCB_ASSOCIATING(a)	((a)->state & PENDING_ASSOC)
#define SCB_AUTHENTICATING(a)	((a)->state & PENDING_AUTH)
#define SCB_AUTHENTICATED(a)	((a)->state & AUTHENTICATED)
#define SCB_AUTHORIZED(a)	((a)->state & AUTHORIZED)

/* scb marking bitfield - used by scb module itself */
#define SCB_MARK_TO_DEL		(1 << 0)	/**< mark to be deleted in watchdog */
#define SCB_DEL_IN_PROG		(1 << 1)	/**< delete in progress - clip recursion */
#define SCB_MARK_TO_REM		(1 << 2)
#define SCB_MARK_TO_DEAUTH	(1 << 3)	/**< send deauth when AP freeing the scb */

/* scb marking bitfield helpers */
#define SCB_MARKED_DEL(a)	((a)->mark & SCB_MARK_TO_DEL)
#define SCB_MARK_DEL(a)		((a)->mark |= SCB_MARK_TO_DEL)
#define SCB_MARKED_DEAUTH(a)	((a)->mark & SCB_MARK_TO_DEAUTH)
#define SCB_MARK_DEAUTH(a)	((a)->mark |= SCB_MARK_TO_DEAUTH)

/* flag access */
#define SCB_ISMYAP(a)           ((a)->flags & SCB_MYAP)
#define SCB_ISPERMANENT(a)      ((a)->permanent)
#define	SCB_INTERNAL(a)		((a)->flags2 & (SCB2_BCMC | SCB2_HWRS))
#define	SCB_BCMC(a)		((a)->flags2 & SCB2_BCMC)
#define	SCB_HWRS(a)		((a)->flags2 & SCB2_HWRS)
#define SCB_IS_BRCM(a)		((a)->flags & SCB_BRCM)
#define SCB_ISMULTI(a)		SCB_BCMC(a)

/* scb_info macros */
#ifdef AP
#define SCB_PS(a)		((a) && (a)->PS)
#ifdef WDS
#define SCB_WDS(a)		((a)->wds)
#else
#define SCB_WDS(a)		NULL
#endif
#define SCB_INTERFACE(a)        ((a)->wds ? (a)->wds->wlif : (a)->bsscfg->wlcif->wlif)
#define SCB_WLCIFP(a)           ((a)->wds ? (a)->wds : ((a)->bsscfg->wlcif))
#define WLC_BCMC_PSMODE(wlc, bsscfg) (SCB_PS(WLC_BCMCSCB_GET(wlc, bsscfg)))
#else
#define SCB_PS(a)		FALSE
#define SCB_WDS(a)		NULL
#define SCB_INTERFACE(a)        ((a)->bsscfg->wlcif->wlif)
#define SCB_WLCIFP(a)           ((a)->bsscfg->wlcif)
#define WLC_BCMC_PSMODE(wlc, bsscfg) (FALSE)
#endif /* AP */

#ifdef WME
#define SCB_WME(a)		((a)->flags & SCB_WMECAP)	/* Also implies WME_ENAB(wlc) */
#else
#define SCB_WME(a)		((void)(a), FALSE)
#endif

#ifdef WLAMPDU
#define SCB_AMPDU(a)		((a)->flags & SCB_AMPDUCAP)
#else
#define SCB_AMPDU(a)		FALSE
#endif

#ifdef WLAMSDU
#define SCB_AMSDU(a)		((a)->flags & SCB_AMSDUCAP)
#define SCB_AMSDU_IN_AMPDU(a)	((a)->flags2 & SCB2_AMSDU_IN_AMPDU_CAP)
#else
#define SCB_AMSDU(a)		FALSE
#define SCB_AMSDU_IN_AMPDU(a) FALSE
#endif

#ifdef WL11N
#define SCB_HT_CAP(a)		(((a)->flags & SCB_HTCAP) != 0)
#define SCB_VHT_CAP(a)		(((a)->flags2 & SCB2_VHTCAP) != 0)
#define SCB_HE_CAP(a)		(((a)->flags2 & SCB2_HECAP) != 0)
#define SCB_ISGF_CAP(a)		(((a)->flags & (SCB_HTCAP | SCB_NONGF)) == SCB_HTCAP)
#define SCB_NONGF_CAP(a)	(((a)->flags & (SCB_HTCAP | SCB_NONGF)) == \
					(SCB_HTCAP | SCB_NONGF))
#define SCB_COEX_CAP(a)		((a)->flags & SCB_COEX_MGMT)
#define SCB_STBC_CAP(a)		((a)->flags & SCB_STBCCAP)
#define SCB_LDPC_CAP(a)		(SCB_HT_CAP(a) && ((a)->flags2 & SCB2_LDPCCAP))
#define SCB_HT_PROP_RATES_CAP(a) (((a)->flags3 & SCB3_HT_PROP_RATES_CAP) != 0)
#define SCB_HT_PROP_RATES_CAP_SET(a) ((a)->flags3 |= SCB3_HT_PROP_RATES_CAP)
#define SCB_HE_TRIG_TX(a) (((a)->flags3 & SCB3_HE_TRIGGERED_TX) != 0)
#else /* WL11N */
#define SCB_HT_CAP(a)		FALSE
#define SCB_VHT_CAP(a)		FALSE
#define SCB_HE_CAP(a)		FALSE
#define SCB_ISGF_CAP(a)		FALSE
#define SCB_NONGF_CAP(a)	FALSE
#define SCB_COEX_CAP(a)		FALSE
#define SCB_STBC_CAP(a)		FALSE
#define SCB_LDPC_CAP(a)		FALSE
#define SCB_HT_PROP_RATES_CAP(a) FALSE
#define SCB_HT_PROP_RATES_CAP_SET(a) do {} while (0)
#define SCB_HE_TRIG_TX(a)	FALSE
#endif /* WL11N */

#define SCB_TWT_REQ_CAP(a)	((a)->flags2 & SCB2_TWT_REQCAP)
#define SCB_TWT_RESP_CAP(a)	((a)->flags2 & SCB2_TWT_RESPCAP)

#ifdef WL11AC
#define SCB_OPER_MODE_NOTIF_CAP(a) ((a)->flags3 & SCB3_OPER_MODE_NOTIF)
#else
#define SCB_OPER_MODE_NOTIF_CAP(a) (0)
#endif /* WL11AC */

#define SCB_IS_IBSS_PEER(a)	((a)->flags & SCB_IBSS_PEER)
#define SCB_SET_IBSS_PEER(a)	((a)->flags |= SCB_IBSS_PEER)
#define SCB_UNSET_IBSS_PEER(a)	((a)->flags &= ~SCB_IBSS_PEER)

#if defined(PKTC) || defined(PKTC_DONGLE)
#define SCB_PKTC_ENABLE(a)	((a)->flags3 |= SCB3_PKTC)
#define SCB_PKTC_DISABLE(a)	((a)->flags3 &= ~SCB3_PKTC)
#define SCB_PKTC_ENABLED(a)	((a)->flags3 & SCB3_PKTC)
#else
#define SCB_PKTC_ENABLE(a)
#define SCB_PKTC_DISABLE(a)
#define SCB_PKTC_ENABLED(a)	FALSE
#endif

#ifdef WLCXO_CTRL
#define SCB_CXO_ENABLE(a)	((a)->flags3 |= SCB3_CXO)
#define SCB_CXO_DISABLE(a)	((a)->flags3 &= ~SCB3_CXO)
#define SCB_CXO_ENABLED(a)	((a)->flags3 & SCB3_CXO)
#else
#define SCB_CXO_ENABLE(a)
#define SCB_CXO_DISABLE(a)
#define SCB_CXO_ENABLED(a)	FALSE
#endif

#define SCB_11E(a)		FALSE

#define SCB_QOS(a)		((a)->flags & (SCB_WMECAP | SCB_HTCAP))

#ifdef WLP2P
#define SCB_P2P(a)		((a)->flags2 & SCB2_P2P)
#else
#define SCB_P2P(a)		FALSE
#endif

#ifdef DWDS
#define SCB_DWDS_CAP(a)		((a)->flags3 & SCB3_DWDS_CAP)
#define SCB_DWDS(a)		((a)->flags2 & SCB2_DWDS_ACTIVE)
#else
#define SCB_DWDS(a)		FALSE
#define SCB_DWDS_CAP(a)		FALSE
#endif

#define SCB_ECSA_CAP(a)		(((a)->flags3 & SCB3_ECSA_SUPPORT) != 0)
#define SCB_DWDS_ACTIVATE(a)	((a)->flags2 |= SCB2_DWDS_ACTIVE)
#define SCB_DWDS_DEACTIVATE(a)	((a)->flags2 &= ~SCB2_DWDS_ACTIVE)

#define SCB_LEGACY_WDS(a)	(SCB_WDS(a) && !SCB_DWDS(a))

#define SCB_A4_DATA(a)		((a)->flags3 & SCB3_A4_DATA)
#define SCB_A4_DATA_ENABLE(a)	((a)->flags3 |= SCB3_A4_DATA)
#define SCB_A4_DATA_DISABLE(a)	((a)->flags3 &= ~SCB3_A4_DATA)

#define SCB_A4_NULLDATA(a)	((a)->flags3 & SCB3_A4_NULLDATA)
#define SCB_A4_8021X(a)		((a)->flags3 & SCB3_A4_8021X)

#define SCB_MFP(a)		((a) && ((a)->flags2 & SCB2_MFP))
#define SCB_SHA256(a)		((a) && ((a)->flags2 & SCB2_SHA256))
#define SCB_CCX_MFP(a)	((a) && ((a)->flags2 & SCB2_CCX_MFP))

#define SCB_1024QAM_CAP(a)	((a)->flags3 & SCB3_1024QAM_CAP)

#define SCB_MU(a)		((a)->flags3 & SCB3_MU_CAP)
#define SCB_MU_ENABLE(a)	((a)->flags3 |= SCB3_MU_CAP)
#define SCB_MU_DISABLE(a)	((a)->flags3 &= ~SCB3_MU_CAP)

#define SCB_SEQNUM(scb, prio)	(scb)->seqnum[(prio)]

#ifdef WL11K
#define SCB_RRM(a)		((a)->flags3 & SCB3_RRM)
#else
#define SCB_RRM(a)		FALSE
#endif /* WL11K */

#define SCB_FTM(_a)		(((_a)->flags3 & (SCB3_FTM_INITIATOR | SCB3_FTM_RESPONDER)) != 0)
#define SCB_FTM_INITIATOR(_a)	(((_a)->flags3 & SCB3_FTM_INITIATOR) != 0)
#define SCB_FTM_RESPONDER(_a)	(((_a)->flags3 & SCB3_FTM_RESPONDER) != 0)

#ifdef WLCNTSCB
#define WLCNTSCBINCR(a)			((a)++)	/**< Increment by 1 */
#define WLCNTSCBDECR(a)			((a)--)	/**< Decrement by 1 */
#define WLCNTSCBADD(a,delta)		((a) += (delta)) /**< Increment by specified value */
#define WLCNTSCBSET(a,value)		((a) = (value)) /**< Set to specific value */
#define WLCNTSCBVAL(a)			(a)	/**< Return value */
#define WLCNTSCB_COND_SET(c, a, v)	do { if (c) (a) = (v); } while (0)
#define WLCNTSCB_COND_ADD(c, a, d)	do { if (c) (a) += (d); } while (0)
#define WLCNTSCB_COND_INCR(c, a)	do { if (c) (a) += (1); } while (0)
#else /* WLCNTSCB */
#define WLCNTSCBINCR(a)			/* No stats support */
#define WLCNTSCBDECR(a)			/* No stats support */
#define WLCNTSCBADD(a,delta)		/* No stats support */
#define WLCNTSCBSET(a,value)		/* No stats support */
#define WLCNTSCBVAL(a)		0	/* No stats support */
#define WLCNTSCB_COND_SET(c, a, v)	/* No stats support */
#define WLCNTSCB_COND_ADD(c, a, d) 	/* No stats support */
#define WLCNTSCB_COND_INCR(c, a)	/* No stats support */
#endif /* WLCNTSCB */

extern void wlc_scb_switch_band(wlc_info_t *wlc, struct scb *scb, int new_bandunit,
	wlc_bsscfg_t *bsscfg);

extern struct scb * wlc_scbfind_dualband(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	struct ether_addr *addr);

typedef struct {
	struct scb *scb;
	uint8	oldstate;
} scb_state_upd_data_t;
typedef void (*scb_state_upd_cb_t)(void *arg, scb_state_upd_data_t *data);
extern int wlc_scb_state_upd_register(wlc_info_t *wlc, scb_state_upd_cb_t fn, void *arg);
extern int wlc_scb_state_upd_unregister(wlc_info_t *wlc, scb_state_upd_cb_t fn, void *arg);


void wlc_scbfind_delete(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	struct ether_addr *ea);

void wlc_scb_datapath_log_dump(wlc_info_t *wlc, struct scb *scb, int tag);

#if defined(STA) && defined(DBG_BCN_LOSS)
int wlc_get_bcn_dbg_info(struct wlc_bsscfg *cfg, struct ether_addr *addr,
	struct wlc_scb_dbg_bcn *dbg_bcn);
#endif

#endif /* _wlc_scb_h_ */
