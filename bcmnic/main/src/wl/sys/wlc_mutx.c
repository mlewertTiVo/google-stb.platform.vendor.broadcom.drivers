/*
 * MU-MIMO transmit module for Broadcom 802.11 Networking Adapter Device Drivers
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_mutx.c 613452 2016-01-19 03:04:16Z $
 */

/* This module manages multi-user MIMO transmit configuration and state.
 *
 * Terminology:
 *  MU group - As defined by 802.11ac, one of 62 MU-MIMO groups, numbered from 1 to 62,
 *  that identifies the set of STAs included in an MU-PPDU and the position of each in
 *  the VHT-SIG-A NSTS field. A STA can be a member of multiple groups. A group ID can
 *  be overloaded, such that the same group ID identifies different combinations of STAs;
 *  however, a sta that appears in multiple overloaded groups must be in the same user
 *  position in each group with the same group ID.
 *
 *  User position - An integer from 0 to 3 that specifies which of the 4 NSTS fields in
 *  VHT-SIG-A applies to a given user for a given group ID.
 *
 *  MU Capable STA - A currently associated VHT STA that has advertised MU beamformee capability.
 *
 *  MU Client Set - A set of N MU-capable STAs currently selected to receive MU transmission
 *  from this device. N is a fixed number determined by hardware/memory limitations.
 *
 *  Client index - An integer that uniquely identifies a STA that is a member of the current
 *  MU client set. The client index is used as an index into the set of MU tx FIFOs and
 *  as an index into the blocks of shared memory that communicate MU user configuration and
 *  state between the driver and microcode.
 *
 * This module selects the set of MU client STAs from among all MU-capable STAs. The
 * maximum number of MU clients is limited by hardware. The initial implementation
 * simply selects the first N MU-capable STAs that associate to the AP. These STAs
 * remain MUC clients as long as they remain associated and MU-capable. If a MU client
 * disassociates or loses its MU capability, then another MU capable STA is chosen
 * to replace it. The module is written to support a more intelligent selection of
 * MU clients, but development of an algorithm to do MU client selection is deferred
 * to a future release.
 *
 * This module provides APIs to get and set a STA's MU group membership status and
 * user position for an MU-MIMO group ID. Currently, the group membership and
 * user positions are static. The static values are coded into this module. The
 * APIs support a potential future capability for the hardware to dynamically
 * make group membership assignments and communicate those assignments back to the
 * driver.
 *
 * This module includes functions that construct and send Group ID Management frames
 * to MU clients to inform the client of its group membership and user position
 * within each group.
 *
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <osl.h>
#include <wlc_types.h>
#include <siutils.h>
#include <bcmwifi_channels.h>
#include <wlioctl.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wl_dbg.h>
#include <wlc_pcb.h>
#include <wlc_scb.h>
#include <wlc_txbf.h>
#include <wlc_vht.h>
#include <wlc_txc.h>
#include <wlc_mutx.h>
#include <wlc_bmac.h>
#include <wlc_tx.h>
#include <wlc_ampdu.h>
#include <wl_export.h>
#include <wlc_scb_ratesel.h>

#ifdef WL_MU_TX

/* Maximum VHT MCS index. MCS 0 - 11, up to 4 streams */
#define MUTX_MCS_NUM  12
#define MUTX_NSS_NUM   4
#define MUTX_MCS_INDEX_NUM  (MUTX_MCS_NUM*MUTX_NSS_NUM)

/* iovar table */
enum {
	IOV_MUTX_UT,
#ifdef WL_MUPKTENG
	IOV_MUTX_PKTENG,
	IOV_MUTX_PKTENG_ADDSTA,
	IOV_MUTX_PKTENG_CLRSTA,
	IOV_MUTX_PKTENG_TX,
#endif
#if defined(BCMDBG_MU)
	IOV_MUTX,
#endif
	IOV_MUTX_MAX_MUCLIENTS,
#if defined(BCMDBG_MU)
	IOV_MUTX_SENDGID,
#endif
	IOV_MUTX_LAST
};

/* Max number of times the driver will send a Group ID Mgmt frame to
 * a STA when the STA does not ACK.
 */
#define MU_GID_SEND_LIMIT  3

/* Minimum number of transmit chains for MU TX to be active */
#define MUTX_TXCHAIN_MIN   4

static const bcm_iovar_t mutx_iovars[] = {
	{"mutx_ut", IOV_MUTX_UT,
	0, IOVT_UINT32, 0
	},
#ifdef WL_MUPKTENG
	{"mutx_pkteng", IOV_MUTX_PKTENG,
	0, IOVT_BOOL, 0
	},
	{"mupkteng_addsta", IOV_MUTX_PKTENG_ADDSTA,
	0, IOVT_BUFFER, 0
	},
	{"mupkteng_clrsta", IOV_MUTX_PKTENG_CLRSTA,
	0, IOVT_BUFFER, 0
	},
	{"mupkteng_tx", IOV_MUTX_PKTENG_TX,
	0, IOVT_BUFFER, 0
	},
#endif
#if defined(BCMDBG_MU)
	{"mutx", IOV_MUTX,
	0, IOVT_BOOL, 0
	},
	{"mutx_sendgid", IOV_MUTX_SENDGID,
	0, IOVT_BOOL, 0
	},
#endif
	{"max_muclients", IOV_MUTX_MAX_MUCLIENTS,
	IOVF_SET_DOWN, IOVT_UINT8, 0
	},
	{NULL, 0, 0, 0, 0 }
};

/* By default, each MU client is a member of groups 1 - 9. */
static uint8 dflt_membership[MUCLIENT_NUM][MU_MEMBERSHIP_SIZE] = {
	{ 0xFE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xFE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xFE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xFE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xFE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xFE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xFE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xFE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
	};

/* The default user position for each of the 8 MU clients. */
static uint8 dflt_position[MUCLIENT_NUM][MU_POSITION_SIZE] = {
	{ 0x00, 0x18, 0x00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0x00, 0x6C, 0x05, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0x94, 0x01, 0x08, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0xD4, 0x42, 0x0D, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0x68, 0x95, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0x78, 0xA6, 0x07, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0xAC, 0xFB, 0x0A, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0xFC, 0xFF, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }
};

/* State structure for the MU-MIMO module created by
 * wlc_mutx_module_attach().
 */
struct wlc_mutx_info {
	osl_t *osh;              /* OSL handle */
	wlc_info_t *wlc;         /* wlc_info_t handle */
	wlc_pub_t *pub;          /* wlc_pub_t handle */

	int scb_handle;			     /* scb cubby handle to retrieve data from scb */

	/* TRUE if MU transmit is operational. To be operational, the phy must support MU BFR,
	 * MU BFR must be enabled by configuration, and there must be nothing in the system
	 * state that precludes MU TX, such as having too few transmit chains enabled.
	 */
	bool mutx_active;

	/* TRUE if the current set of MU clients is considered stale and a new
	 * set should be selected.
	 */
	bool muclient_elect_pending;

	/* Current set of MU clients. The index to this array is the client_index
	 * member of mutx_scb_t. If fewer than MUCLIENT_NUM associated clients
	 * are MU capable, then the array is not full. Non-empty elements may follow
	 * empty elements. This happens when a MU client is removed (disassociates
	 * or loses MU capability).
	 */
	struct scb *mu_clients[MUCLIENT_NUM];

#ifdef WLCNT
	/* Count of memory allocation failures for scb cubbies */
	uint16 mutx_scb_alloc_fail;
#endif
	bool mutx_on; /* if mutx is on or not -- debug feature */
	bool mutx_pkteng; /* mupkteng -- debug feature */
	/* below are reported from txstatus */
	uint32 queued_cnt; /* queued as MU */
	uint32 mutx_cnt;   /* tx'd as MU */
	uint32 sutx_lost_cnt; /* tx'd as SU but not acked */
	uint32 mutx_lost_cnt; /* tx'd as MU but not acked */

	uint32 bw_policy;
};

#ifdef WLCNT
typedef struct mutx_stats {

	/* Number of data frames sent for each group ID. Counts both SU and MU GIDs. */
	uint32 gid_cnt[MIMO_GROUP_NUM];

	/* Last sent rate for each group ID. The rate value is represented as
	 * 2 bits of NSS (actual NSS - 1) followed by 4 bits of MCS.
	 */
	uint8 gid_last_rate[MIMO_GROUP_NUM];

	/* Number of MU-MIMO frames received for each VHT MCS. Indexed by
	 * mcs + ((nss - 1) * mcs_num)
	 */
	uint32 mu_mcs_cnt[MUTX_MCS_INDEX_NUM];

	/* Same for SU frames */
	uint32 su_mcs_cnt[MUTX_MCS_INDEX_NUM];

	uint32 queued_cnt; /* queued as MU */
	uint32 mutx_cnt;   /* tx'd as MU */
	uint32 sutx_lost_cnt; /* tx'd as SU but not acked */
	uint32 mutx_lost_cnt; /* tx'd as MU but not acked */
} mutx_stats_t;
#endif   /* WLCNT */

/* MU-MIMO tx scb cubby structure. */
typedef struct mutx_scb {

	/* If this STA is in the MU clients set, then client_index is the index
	 * for this STA in the mu_clients[] array above. If this STA is not a
	 * MU client, then client_index is set to MU_CLIENT_INDEX_NONE.
	 */
	uint16 client_index;

	/* Bit mask indicating STA's membership in each MU-MIMO group. Indexed by
	 * group ID. A 1 at index N indicates the STA is a member of group N. Lowest
	 * order bit indicates membership status in group ID 0. Groups 0 and 63 are
	 * reserved for SU.
	 */
	uint8 membership[MU_MEMBERSHIP_SIZE];

	/* Bit string indicating STA's user position within each MU-MIMO group.
	 * A user position is represented by two bits (positions 0 - 3), under the
	 * assumption that no more than 4 MPDUs are included in an MU-PPDU. A
	 * position value is only valid if the corresponding membership flag is set.
	 */
	uint8 position[MU_POSITION_SIZE];

	/* TRUE if the STA's MU group membership or user position within a
	 * group has changed since the last time we successfully sent
	 * a Group ID message to this user.
	 */
	bool membership_change;

	/* Number of times the current Group ID mgmt frame has been sent
	 * to this STA. Used to limit driver-initiated retries.
	 */
	uint8 gid_send_cnt;

	/* Some STAs ack the GID frame but do not behave like having received the GID frames.
	 * Resend the GID management frame as a WAR.
	 */
	uint8 gid_retry_war;

	/* Receive data frame statistics */
#ifdef WLCNT
	mutx_stats_t *mutx_stats;	/* counters/stats */
#endif /* WLCNT */
	uint8 retry_gid;
} mutx_scb_t;

#define GID_RETRY_DELAY 3

/* MU TX scb cubby is just a pointer to a dynamically allocated structure. Of course,
 * this memory allocation can fail. So generally speaking, the return value of MUTX_SCB
 * must be checked for NULL. One exception: An scb in the MU client set is guaranteed
 * to have anon-NULL mutx_scb.
 */
typedef struct mutx_scb_cubby {
	mutx_scb_t *mutx_scb;
} mutx_scb_cubby_t;

/* A structure to pass to the group ID frame callback to provide information
 * about the intended receiver. Allows callback function to lookup the SCB
 * the GID frame was addressed to.
 */
typedef struct mutx_pkt_info
{
	/* Ethernet address of STA GID was addressed to */
	struct ether_addr	ea;

	/* BSS configuration index */
	int8	bssID;

} mutx_pkt_info_t;

/* The following table lists the aqmfifo allocated to each MU client */
static uint8  mutx_aqmfifo[][AC_COUNT] = {
	/* bk, be, vi, vo */
	/* original designed fifo allocation */
	{ 8,  8,  9, 10},
	{11, 11, 12, 13},
	{14, 14, 15, 16},
	{17, 17, 18, 19},
	{20, 20, 21, 22},
	{23, 23, 24, 25},
	{26, 26, 27, 28},
	{29, 29, 30, 31}
};

/* The following table lists the aqmfifo allocated to each MU client */
static uint8  mutx_aqmfifo_4365b1[][AC_COUNT] = {
	/* bk, be, vi, vo */
	{17, 16, 17, 17},
	{19, 18, 19, 19},
	{21, 20, 21, 21},
	{23, 22, 23, 23},
	{25, 24, 25, 25},
	{27, 26, 27, 27},
	{29, 28, 29, 29},
	{31, 30, 31, 31}
};

#define MUTX_SCB_CUBBY(mu_info, scb)  ((mutx_scb_cubby_t*)(SCB_CUBBY((scb), (mu_info)->scb_handle)))
#define MUTX_SCB(mu_info, scb)        (mutx_scb_t*)(MUTX_SCB_CUBBY(mu_info, scb)->mutx_scb)

/* Basic module infrastructure */
static int mu_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid,
                      const char *name, void *p, uint plen, void *a,
                      int alen, int vsize, struct wlc_if *wlcif);
#if defined(BCMDBG_AMPDU) || defined(BCMDBG_MU)
static int wlc_mutx_dump(wlc_mutx_info_t *mu_info, struct bcmstrbuf *b);
#endif
static void wlc_mutx_watchdog(void *mi);
static int wlc_mutx_up(void *mi);
#if defined(BCMDBG_MU)
static int wlc_mutx_sendgid(wlc_mutx_info_t *mu_info);
#endif

/* SCB cubby management */
static void wlc_mutx_scb_state_upd(void *ctx, scb_state_upd_data_t *notif_data);
static mutx_scb_t *mu_scb_cubby_alloc(wlc_mutx_info_t *mu_info, struct scb *scb);
static void mu_scb_cubby_deinit(void *context, struct scb *scb);
#if defined(BCMDBG_AMPDU) || defined(BCMDBG_MU)
static void mu_scb_cubby_dump(void *ctx, struct scb *scb, struct bcmstrbuf *b);
#else
#define mu_scb_cubby_dump NULL
#endif

/* MU client set management */
static struct scb *mu_candidate_select(wlc_mutx_info_t *mu_info);
static int mu_client_set_add(wlc_mutx_info_t *mu_info, struct scb *scb);
static int mu_client_set_remove(wlc_mutx_info_t *mu_info, struct scb *scb);
static void mu_client_set_clear(wlc_mutx_info_t *mu_info);
static bool mu_client_set_full(wlc_mutx_info_t *mu_info);
static void mu_client_set_new_sta(wlc_mutx_info_t *mu_info, struct scb *scb);
static void mu_client_set_departed_sta(wlc_mutx_info_t *mu_info, struct scb *scb);
static bool mu_in_client_set(wlc_mutx_info_t *mu_info, struct scb *scb);
static int mu_clients_select(wlc_mutx_info_t *mu_info);

static bool mu_sta_mu_capable(wlc_mutx_info_t *mu_info, struct scb *scb);
static bool mu_sta_mu_ready(wlc_mutx_info_t *mu_info, struct scb *scb);
static int mu_sta_mu_enable(wlc_mutx_info_t *mu_info, struct scb *scb);
static int mu_sta_mu_disable(wlc_mutx_info_t *mu_info, struct scb *scb);
static bool mu_scb_membership_change_get(wlc_info_t *wlc, struct scb *scb);
static void mu_scb_membership_change_set(wlc_info_t *wlc, struct scb *scb, bool pending);

/* Manage user position, group membership bit arrays */
#if defined(BCMDBG_AMPDU) || defined(BCMDBG_MU)
static uint8 mu_user_pos_get(uint8 *pos_array, uint16 group_id);
#endif
static bool mu_any_membership(uint8 *membership);

/* Sending group ID management frames */
static int mu_group_id_msg_send(wlc_info_t *wlc, struct scb *scb,
                                uint8 *membership, uint8 *user_pos);
static void wlc_mutx_gid_complete(wlc_info_t *wlc, uint txstatus, void *arg);

#ifdef WL_MUPKTENG
static int wlc_mupkteng_addsta(wlc_mutx_info_t *mu_info, struct ether_addr *ea, int client_idx,
		int nrxchain);
static int wlc_mupkteng_clrsta(wlc_mutx_info_t *mu_info, int client_idx);
static int wlc_mupkteng_tx(wlc_mutx_info_t *mu_info, mupkteng_tx_t *mupkteng_tx);
#endif


#if defined(BCMDBG_AMPDU) || defined(BCMDBG_MU)
static uint8
mu_user_pos_get(uint8 *pos_array, uint16 group_id)
{
	uint8 user_pos;
	uint8 pos_mask;

	/* Index of first (low) bit in user position field */
	uint16 bit_index = group_id * MU_POSITION_BIT_LEN;
	pos_mask = 0x3 << (bit_index % NBBY);

	user_pos = (pos_array[(bit_index) / NBBY] & pos_mask) >> (bit_index % NBBY);
	return user_pos;
}
#endif   

/* Return TRUE if the membership array indicates membership in at least one group. */
static bool
mu_any_membership(uint8 *membership)
{
	uint32 *mw = (uint32*) membership;  /* start of one 4-byte word of membership array */
	int i;
	int num_ints = MU_MEMBERSHIP_SIZE / sizeof(uint32);

	if (!membership)
		return FALSE;

	for (i = 0; i < num_ints; i++) {
		if (*mw != 0) {
			return TRUE;
		}
		mw++;
	}
	return FALSE;
}

/* Module Attach/Detach */

/*
 * Create the MU-MIMO module infrastructure for the wl driver.
 * wlc_module_register() is called to register the module's
 * handlers. The dump function is also registered. Handlers are only
 * registered if the phy is MU BFR capable and if MU TX is not disabled
 * in the build.
 *
 * _mu_tx is always FALSE upon return. Because _mu_tx reflects not only
 * phy capability, but also configuration, have to wait until the up callback, if
 * registered, to set _mu_tx to TRUE.
 *
 * Returns
 *     A wlc_mutx_info_t structure, or NULL in case of failure.
 */
wlc_mutx_info_t*
BCMATTACHFN(wlc_mutx_attach)(wlc_info_t *wlc)
{
	wlc_mutx_info_t *mu_info;
	int err;

	/* allocate the main state structure */
	mu_info = MALLOCZ(wlc->osh, sizeof(wlc_mutx_info_t));
	if (mu_info == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));

		return NULL;
	}

	mu_info->wlc = wlc;
	mu_info->pub = wlc->pub;
	mu_info->osh = wlc->osh;
	wlc->_mu_tx = FALSE;

#ifdef WL_MU_TX_DISABLED
	return mu_info;
#endif

	/* Avoid registering callbacks if phy is not MU capable */
	if (!WLC_MU_BFR_CAP_PHY(wlc)) {
		return mu_info;
	}

	/* Default setting mutx_on to TRUE so that driver behavior is more consistent across
	 * builds w/ or w/o the mutx command.
	 */
	mu_info->mutx_on = TRUE;

	/* Default let the drivers switch SU/MU automatically */
	if (D11REV_GE(mu_info->pub->corerev, 64))
		mu_info->pub->mu_features = MU_FEATURES_AUTO;

	/* External */
	if (D11REV_GT(mu_info->pub->corerev, 64))
		mu_info->pub->max_muclients = MUCLIENT_NUM_4;
	else
		mu_info->pub->max_muclients = MUCLIENT_NUM_MIN;

	/* Phy is MU TX capable. And build has not prohibited MU TX. So
	 * register callbacks and allow MU TX to be dynamically enabled.
	 */
	err = wlc_module_register(mu_info->pub, mutx_iovars, "mutx", mu_info,
	                          mu_doiovar, wlc_mutx_watchdog, wlc_mutx_up, NULL);

	if (err != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed with error %d (%s).\n",
		          wlc->pub->unit, __FUNCTION__, err, bcmerrorstr(err)));

		/* use detach as a common failure deallocation */
		wlc_mutx_detach(mu_info);
		return NULL;
	}

	/* reserve scb cubby space for STA-specific data. No init function is
	 * registered because cubby memory is allocated on demand.
	 */
	mu_info->scb_handle = wlc_scb_cubby_reserve(wlc, sizeof(mutx_scb_cubby_t),
	                                            NULL,
	                                            mu_scb_cubby_deinit,
	                                            mu_scb_cubby_dump,
	                                            mu_info);
	if (mu_info->scb_handle < 0) {
		WL_ERROR(("wl%d: %s: Failed to reserve scb cubby space.\n",
		         wlc->pub->unit, __FUNCTION__));

		wlc_mutx_detach(mu_info);
		return NULL;
	}

	/* Add client callback to the scb state notification list */
	if ((err = wlc_scb_state_upd_register(wlc,
	                (bcm_notif_client_callback)wlc_mutx_scb_state_upd, mu_info)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: unable to register callback %p\n",
		          wlc->pub->unit, __FUNCTION__, wlc_mutx_scb_state_upd));

		wlc_mutx_detach(mu_info);
		return NULL;
	}

	/* Use specific AQM fifo index table for 4365B1 chip */
	if (D11REV_IS(mu_info->pub->corerev, 64))
		memcpy(mutx_aqmfifo, mutx_aqmfifo_4365b1, sizeof(mutx_aqmfifo));

#if defined(BCMDBG_AMPDU) || defined(BCMDBG_MU)
	wlc_dump_register(mu_info->pub, "mutx", (dump_fn_t)wlc_mutx_dump, mu_info);
#endif

	return mu_info;
}

/* Free all resources associated with the MU-MIMO module
 * infrastructure. This is done at the cleanup stage when
 * freeing the driver.
 *
 * mu_info    MU-MIMO module state structure
 */
void
BCMATTACHFN(wlc_mutx_detach)(wlc_mutx_info_t *mu_info)
{
	if (mu_info == NULL) {
		return;
	}
#ifndef WL_MU_TX_DISABLED
	wlc_scb_state_upd_unregister(mu_info->wlc,
	                             (bcm_notif_client_callback)wlc_mutx_scb_state_upd,
	                             mu_info);

	wlc_module_unregister(mu_info->pub, "mutx", mu_info);
#endif  /* WL_MU_TX_DISABLED */
	MFREE(mu_info->osh, mu_info, sizeof(wlc_mutx_info_t));
}

#if defined(BCMDBG_AMPDU) || defined(BCMDBG_MU)
/* Dump MU-MIMO state information. */
static int
wlc_mutx_dump(wlc_mutx_info_t *mu_info, struct bcmstrbuf *b)
{
	struct scb *scb;
	mutx_scb_t *mu_scb;
	int i, g;
	uint8 pos;
	char eabuf[ETHER_ADDR_STR_LEN];
	bool empty_group;
#ifdef WLCNT
	mutx_stats_t *stats;
#endif

	if (!mu_info->wlc) {
		bcm_bprintf(b, "wlc pointer is NULL on mu_info\n");
		return BCME_OK;
	}

	if (MU_TX_ENAB(mu_info->wlc))
		bcm_bprintf(b, "MU BFR capable and configured; ");

	bcm_bprintf(b, "MU TX is %sactive, %s\n",
		mu_info->mutx_active ? "" : "not ",
		mu_info->mutx_on ? "and ON" : "but OFF");

	bcm_bprintf(b, "Maximum MU clients: %d\n", mu_info->pub->max_muclients);

	if (mu_info->muclient_elect_pending)
		bcm_bprintf(b, "MU client election pending\n");

	/* print group membership */
	bcm_bprintf(b, "MU Groups:\n");
	for (g = MU_GROUP_ID_MIN; g < MU_GROUP_ID_MAX; g++) {
		/* 1st iteration to tell whether this is an empty group */
		empty_group = TRUE;
		for (i = 0; i < MUCLIENT_NUM; i++) {
			scb = mu_info->mu_clients[i];
			if (scb) {
				mu_scb = MUTX_SCB(mu_info, scb);
				if (mu_scb && isset(mu_scb->membership, g)) {
					if (empty_group) {
						bcm_bprintf(b, "    GID %u:", g);
						empty_group = FALSE;
						break;
					}
				}
			}
		}
		/* 2nd iteration to print the client indices in each position */
		for (pos = 0; pos < 4; pos++) {
			if (!empty_group)
				bcm_bprintf(b, " [");
			for (i = 0; i < MUCLIENT_NUM; i++) {
				scb = mu_info->mu_clients[i];
				if (scb) {
					mu_scb = MUTX_SCB(mu_info, scb);
					if (mu_scb && isset(mu_scb->membership, g)) {
						if (mu_user_pos_get(mu_scb->position, g) == pos) {
							bcm_bprintf(b, " %u", i);
						}
					}
				}
			}
			if (!empty_group)
				bcm_bprintf(b, " ]");
		}
		if (!empty_group)
			bcm_bprintf(b, "\n");
	}

	/* Dump current MU client set */
	bcm_bprintf(b, "MU clients:\n");
	for (i = 0; i < MUCLIENT_NUM; i++) {
		scb = mu_info->mu_clients[i];
		if (scb) {
			mu_scb = MUTX_SCB(mu_info, scb);
			if (mu_scb) {
				bcm_bprintf(b, "[%u] %s\n",
					i, bcm_ether_ntoa(&scb->ea, eabuf));
#ifdef WLCNT
				if (mu_scb->mutx_stats) {
					stats = mu_scb->mutx_stats;
					bcm_bprintf(b, "mutx: queued %u tx_as_mu %u"
						" nlost_mu %u nlost_su %u\n",
						stats->queued_cnt, stats->mutx_cnt,
						stats->mutx_lost_cnt, stats->sutx_lost_cnt);

					stats->queued_cnt = 0;
					stats->mutx_cnt = 0;
					stats->mutx_lost_cnt = 0;
					stats->sutx_lost_cnt = 0;
				}
#endif /* WLCNT */
			}
		}
	}

	bcm_bprintf(b, "\nTotal mutx: queued %u tx_as_mu %u nlost_mu %u nlost_su %u\n",
		mu_info->queued_cnt, mu_info->mutx_cnt,
		mu_info->mutx_lost_cnt, mu_info->sutx_lost_cnt);

	mu_info->queued_cnt = 0;
	mu_info->mutx_cnt = 0;
	mu_info->mutx_lost_cnt = 0;
	mu_info->sutx_lost_cnt = 0;

	return BCME_OK;
}
#endif 

/* IOVar handler for the MU-MIMO infrastructure module */
static int
mu_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid,
           const char *name,
           void *p, uint plen,
           void *a, int alen,
           int vsize,
           struct wlc_if *wlcif)
{
	wlc_mutx_info_t *mu_info = (wlc_mutx_info_t*) hdl;
	int32 int_val = 0;
	int err = 0;
	int32 *ret_int_ptr = (int32 *)a;
	uint32 *ret_uint_ptr = (uint32 *)a;
	bool set_bool;

	BCM_REFERENCE(mu_info);
	BCM_REFERENCE(ret_int_ptr);
	BCM_REFERENCE(ret_uint_ptr);
	BCM_REFERENCE(set_bool);

	if (plen >= (int)sizeof(int_val))
		memcpy(&int_val, p, sizeof(int_val));

	switch (actionid) {
#ifdef WL_MUPKTENG
	case IOV_SVAL(IOV_MUTX_PKTENG):
		set_bool = int_val? TRUE : FALSE;
		if (mu_info->mutx_pkteng != set_bool) {
			mu_info->mutx_pkteng = set_bool;
		}
		break;

	case IOV_GVAL(IOV_MUTX_PKTENG):
		*ret_uint_ptr = (uint8)mu_info->mutx_pkteng;
		break;

	case IOV_SVAL(IOV_MUTX_PKTENG_ADDSTA): {
		mupkteng_sta_t sta;
		bcopy(p, &sta, sizeof(mupkteng_sta_t));
		err = wlc_mupkteng_addsta(mu_info, &sta.ea, sta.idx, sta.nrxchain);
		break;
	}

	case IOV_SVAL(IOV_MUTX_PKTENG_CLRSTA): {
		mupkteng_sta_t sta;
		bcopy(p, &sta, sizeof(mupkteng_sta_t));
		err = wlc_mupkteng_clrsta(mu_info, sta.idx);
		break;
	}
	case IOV_SVAL(IOV_MUTX_PKTENG_TX): {
		mupkteng_tx_t mupkteng_tx;
		bcopy(p, &mupkteng_tx, sizeof(mupkteng_tx_t));
		err = wlc_mupkteng_tx(mu_info, &mupkteng_tx);
		break;
	}
#endif /* WL_MUPKTENG */
#if defined(BCMDBG_MU)
	case IOV_SVAL(IOV_MUTX):
		set_bool = int_val? TRUE : FALSE;
		if (mu_info->mutx_on != set_bool) {
			mu_info->mutx_on = set_bool;
			if (WLC_TXC_ENAB(mu_info->wlc)) {
				wlc_txc_inv_all(mu_info->wlc->txc);
			}
		}

		break;
	case IOV_GVAL(IOV_MUTX):
		*ret_uint_ptr = (uint8)mu_info->mutx_on;
		break;

	case IOV_GVAL(IOV_MUTX_SENDGID):
		*ret_uint_ptr = wlc_mutx_sendgid(mu_info);
		break;

#endif 
	case IOV_GVAL(IOV_MUTX_MAX_MUCLIENTS):
		*ret_int_ptr = (int32)mu_info->pub->max_muclients;
		break;

	case IOV_SVAL(IOV_MUTX_MAX_MUCLIENTS): {
		int32 max = MUCLIENT_NUM;

		if (BUSTYPE(mu_info->pub->sih->bustype) == SI_BUS) {
			if (D11REV_LE(mu_info->pub->corerev, 64))
				max = MUCLIENT_NUM_4;
			if (D11REV_IS(mu_info->pub->corerev, 65))
				max = MUCLIENT_NUM_6;
		}

		if (int_val < MUCLIENT_NUM_MIN || int_val > max) {
			err = BCME_RANGE;
			break;
		}

		err = wlc_txbf_mu_max_links_upd(mu_info->wlc->txbf, (uint8)int_val);
		if (err)
			break;

		mu_info->pub->max_muclients = (uint8)int_val;
		break;
	}

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* Update and return the BW policy */
uint32
wlc_mutx_bw_policy_update(wlc_mutx_info_t *mu_info)
{
	wlc_info_t *wlc = mu_info->wlc;
	struct scb *scb;
	struct scb_iter scbiter;
	uint16 vht_flags;
	uint32 num_stas_bw[BW_80MHZ+1];
	uint32 bw;
	uint32 metric = 0, tmp;
	uint32 bw_policy_old = mu_info->bw_policy;

	/* Init the counters */
	for (bw = 0; bw <= BW_80MHZ; bw++)
		num_stas_bw[bw] = 0;

	/* Iterate and count */
	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		if (!SCB_ASSOCIATED(scb)) {
			continue;
		}
		if (!SCB_VHT_CAP(scb)) {
			/* STA not VHT capable. Can't be MU capable. */
			continue;
		}
		vht_flags = wlc_vht_get_scb_flags(wlc->vhti, scb);
		if ((vht_flags & SCB_MU_BEAMFORMEE) == 0) {
			continue;
		}
		bw = wlc_scb_ratesel_getbwcap(wlc->wrsi, scb, 0);
		if (bw >= BW_20MHZ && bw <= BW_80MHZ)
			num_stas_bw[bw]++;
	}

	/* Decide the BW policy based on the counters */
	bw = 0;
	if (num_stas_bw[BW_80MHZ] > 1) {
		bw = BW_80MHZ;
		metric = num_stas_bw[BW_80MHZ] * 80;
	}
	if (num_stas_bw[BW_40MHZ] > 1) {
		tmp = num_stas_bw[BW_40MHZ] * 40;
		if (tmp > metric) {
			bw = BW_40MHZ;
			metric = tmp;
		}
	}
	if (num_stas_bw[BW_20MHZ] > 1) {
		tmp = num_stas_bw[BW_20MHZ] * 20;
		if (tmp > metric) {
			bw = BW_20MHZ;
			metric = tmp;
		}
	}
	mu_info->bw_policy = bw;

	if (mu_info->mutx_active && bw != bw_policy_old) {
		struct scb *scb;
		struct scb_iter scbiter;
		uint16 vht_flags;

		/* Clear current MU clients */
		mu_client_set_clear(mu_info);
		/* Update txbf links for MU capable STAs */
		/* Delete all current TXBF links */
		FOREACHSCB(wlc->scbstate, &scbiter, scb) {
			if (!SCB_VHT_CAP(scb)) {
				continue;
			}
			vht_flags = wlc_vht_get_scb_flags(wlc->vhti, scb);
			if ((vht_flags & SCB_MU_BEAMFORMEE) == 0) {
				continue;
			}
			wlc_txbf_delete_link(mu_info->wlc->txbf, scb);
		}
		/* Readd TXBF links for all MU capable STAs */
		FOREACHSCB(wlc->scbstate, &scbiter, scb) {
			if (!SCB_VHT_CAP(scb)) {
				continue;
			}
			vht_flags = wlc_vht_get_scb_flags(wlc->vhti, scb);
			if ((vht_flags & SCB_MU_BEAMFORMEE) == 0) {
				continue;
			}
			wlc_txbf_init_link(mu_info->wlc->txbf, scb);
		}
		if (bw) {
			/* More than two MU capable STAs have the same BW */
			mu_info->muclient_elect_pending = TRUE;
		}
	}

	return (mu_info->bw_policy);
}

uint32
wlc_mutx_bw_policy(wlc_mutx_info_t *mu_info)
{
	/* current BW policy */
	return (mu_info->bw_policy);
}


/* retry GID in watchdog reason codes */
#define MUTX_R_GID_DISABLED		0	/* retry GID disabled in watchdog */
#define MUTX_R_GID_NO_BUFFER		1	/* retry GID due to out of buffer */
#define MUTX_R_GID_CALLBACK_FAIL	2	/* retry GID due to callback register fail */
#define MUTX_R_GID_RETRY_CNT	        3	/* retry GID due to retry cnt */
#define MUTX_R_GID_BAD_USR_POS		4	/* retry GID due to bad usr pos */

/* Function registered as this module's watchdog callback. Called once
 * per second. Checks if one of the current MU clients has not yet
 * ack'd a Group ID Mgmt frame, and in that case, sends one to the
 * MU client, if the number of retries has not reached the limit.
 * Also, if MU client selection is pending, select a new set of MU
 * clients.
 */
static void
wlc_mutx_watchdog(void *mi)
{
	wlc_mutx_info_t *mu_info = (wlc_mutx_info_t*) mi;
	mutx_scb_t *mu_scb;
	int i;

	wlc_info_t *wlc = mu_info->wlc;

	/* If MU-MIMO tx is disabled, no-op */
	if (!mu_info->mutx_active) {
		/* Check if we need to switch from SU to MU */
		if (wlc_txbf_mu_cap_stas(wlc->txbf) >= 2) {
			wlc_mutx_switch(wlc, TRUE, FALSE);
		}
		return;
	}

	/* Check if we need to switch from MU to SU */
	if (wlc_txbf_mu_cap_stas(wlc->txbf) < 2) {
		wlc_mutx_switch(wlc, FALSE, FALSE);
		return;
	}

	/* Check if we need to do MU client selection */
	if (mu_info->muclient_elect_pending) {
		mu_clients_select(mu_info);
	}

	/* Check if we need to send a Group ID management message to a client */
	for (i = 0; i < MUCLIENT_NUM; i++) {
		if (mu_info->mu_clients[i] == NULL) {
			continue;
		}

		mu_scb = MUTX_SCB(mu_info, mu_info->mu_clients[i]);
		if (!mu_scb || (!mu_scb->membership_change && !mu_scb->gid_retry_war)) {
			continue;
		}
		if (mu_scb->gid_retry_war) {
			mu_scb->gid_retry_war--;
			if (!mu_scb->membership_change && mu_scb->gid_retry_war)
				continue;
		}

		if (mu_scb->gid_send_cnt < MU_GID_SEND_LIMIT) {
			if (mu_group_id_msg_send(mu_info->wlc, mu_info->mu_clients[i],
			                         mu_scb->membership,
			                         mu_scb->position) == BCME_OK) {
				mu_scb->gid_send_cnt++;
			}
		}
		else if (mu_scb->retry_gid != MUTX_R_GID_DISABLED) {
			/* Exhausted retries for GID management. Remove STA from
			 * MU client set and select a replacement. This STA could
			 * be chosen again if another spot opens (i.e., we are not
			 * permanently locking it out).
			 *
			 * Could implement a hold down time (e.g., 60 sec)
			 * during which STA cannot be selected as a MU client, to avoid
			 * continually thrashing the MU client set and continually
			 * sending GID messages that go unacked? Might happen if multiple
			 * MU-capable STAs go away w/o disassociating. Not sure if that's
			 * likely enough to add the extra complexity. I'd guess not since
			 * AP only sends 1 GID msg per second to each such MU client, and
			 * scb will eventually time out.
			 */
			mu_client_set_departed_sta(mu_info, mu_info->mu_clients[i]);
		}
	}

	return;
}

/* Registered callback when radio comes up. */
static int
wlc_mutx_up(void *mi)
{
	wlc_mutx_info_t *mu_info = (wlc_mutx_info_t*) mi;
	wlc_info_t *wlc = mu_info->wlc;

	/* Check if system is MU BFR capable, both physically and by current
	 * configuration, and if so, set _mu_tx. Configuration can only change
	 * while radio is down. So checking at up time is sufficient.
	 */
	wlc_mutx_update(wlc, wlc_txbf_mutx_enabled(wlc->txbf));

#ifdef WL_PSMX
	wlc_mutx_hostflags_update(wlc);
#endif /* WL_PSMX */

	/* Make sure we start with empty MU client set */
	// mu_client_set_clear(mu_info);

	/* Determine whether MU TX can be active */
	wlc_mutx_active_update(mu_info);

	return BCME_OK;
}

/* Allocate memory for an scb cubby. Point real cubby to allocated memory.
 * Initialize client index.
 * Inputs
 *   mu_info - MU TX state
 *   scb     - scb whose cubby is allocated
 *
 * Returns
 *   MU SCB if allocation successful
 *   NULL otherwise
 */
static mutx_scb_t *
mu_scb_cubby_alloc(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	mutx_scb_t *mu_scb;
	mutx_scb_cubby_t *cubby = MUTX_SCB_CUBBY(mu_info, scb);

	mu_scb = MALLOCZ(mu_info->osh, sizeof(mutx_scb_t));
	if (!mu_scb) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			mu_info->pub->unit, __FUNCTION__, MALLOCED(mu_info->osh)));
		WLCNTINCR(mu_info->mutx_scb_alloc_fail);
		return NULL;
	}
#ifdef WLCNT
	if ((mu_scb->mutx_stats = MALLOCZ(mu_info->osh, sizeof(mutx_stats_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			mu_info->pub->unit, __FUNCTION__, MALLOCED(mu_info->osh)));

		MFREE(mu_info->osh, mu_scb, sizeof(mutx_scb_t));
		return NULL;
	}
#endif
	cubby->mutx_scb = mu_scb;

	/* New STA is not in the MU client set by default */
	mu_scb->client_index = MU_CLIENT_INDEX_NONE;
	mu_scb->retry_gid = MUTX_R_GID_RETRY_CNT;

	return mu_scb;
}

/* Deinitialize this module's scb state. Remove the STA from the MU client set.
 * Free the module's state structure.
 */
static void
mu_scb_cubby_deinit(void *ctx, struct scb *scb)
{
	wlc_mutx_info_t *mu_info = (wlc_mutx_info_t*) ctx;
	mu_sta_mu_disable(mu_info, scb);
}
#if defined(BCMDBG_MU)
static int
wlc_mutx_sendgid(wlc_mutx_info_t *mu_info)
{
	mutx_scb_t *mu_scb;
	int i, success = BCME_OK;

	/* Send a Group ID management message to each MU client */
	for (i = 0; i < MUCLIENT_NUM; i++) {
		if (mu_info->mu_clients[i] == NULL) {
			continue;
		}

		mu_scb = MUTX_SCB(mu_info, mu_info->mu_clients[i]);

		if (mu_group_id_msg_send(mu_info->wlc, mu_info->mu_clients[i],
		                         mu_scb->membership,
		                         mu_scb->position) == BCME_OK) {
			WL_MUMIMO(("Mu-MIMO client index: %u GID frame Successful\n", i));
		} else {
			WL_MUMIMO(("Mu-MIMO client index: %u GID frame Failed\n", i));
			success = BCME_ERROR;
		}
	}
	return success;

}
#endif 

#if defined(BCMDBG_AMPDU) || defined(BCMDBG_MU)
/* Dump MU-MIMO cubby data for a given STA. */
static void
mu_scb_cubby_dump(void *ctx, struct scb *scb, struct bcmstrbuf *b)
{
	int i;
	wlc_mutx_info_t *mu_info = (wlc_mutx_info_t*) ctx;
	mutx_scb_t *mu_scb = MUTX_SCB(mu_info, scb);

	if (mu_scb == NULL) {
		return;
	}

	bcm_bprintf(b, "     MU-MIMO client index: %u\n", mu_scb->client_index);
	if (mu_any_membership(mu_scb->membership)) {
		bcm_bprintf(b, "%15s %15s\n", "Group ID", "User Position");
		for (i = MU_GROUP_ID_MIN; i <= MU_GROUP_ID_MAX; i++) {
			if (isset(mu_scb->membership, i)) {
				bcm_bprintf(b, "%15u %15u\n", i,
				            mu_user_pos_get(mu_scb->position, i));
			}
		}
	}
}
#endif 

/* Add a STA to the current MU client set.
 * Returns
 *   BCME_OK if STA is added to MU client set
 *   BCME_ERROR if STA already a member of MU client set
 *   BCME_NORESOURCE if MU client set is already full
 *   BCME_BADARG if scb cubby is NULL. All MU clients must have a non-NULL cubby
 */
static int
mu_client_set_add(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	wlc_info_t *wlc = mu_info->wlc;
	mutx_scb_t *mu_scb = MUTX_SCB(mu_info, scb);
	char eabuf[ETHER_ADDR_STR_LEN];
	int i;
	int err;

	BCM_REFERENCE(eabuf);

	if (!mu_scb) {
		return BCME_BADARG;
	}

	if (mu_in_client_set(mu_info, scb))
		return BCME_ERROR;

	i = wlc_txbf_get_mubfi_idx(wlc->txbf, scb);
	if (i == BF_SHM_IDX_INV)
		return BCME_NORESOURCE;

	ASSERT(i < MUCLIENT_NUM);

	mu_info->mu_clients[i] = scb;
	mu_scb->client_index = i;

	/* Use default group membership. This sends Group ID message to client. */
	err = wlc_mutx_membership_set(mu_info, i, dflt_membership[i], dflt_position[i]);

	if (err != BCME_OK) {
		/* unable to set membership; clear the list */
		mu_info->mu_clients[i] = NULL;
		mu_scb->client_index = MU_CLIENT_INDEX_NONE;
	} else {
		WL_MUMIMO(("wl%d: STA %s selected as MU client at index %u.\n",
			mu_info->pub->unit, bcm_ether_ntoa(&scb->ea, eabuf), i));
	}
	return err;
}

/* Remove STA from current MU client set.
 * Returns
 *   BCME_OK if STA is removed from MU client set
 *   BCME_NOTFOUND if STA is not found
 */
static int
mu_client_set_remove(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	mutx_scb_t *mu_scb = MUTX_SCB(mu_info, scb);
	struct scb *scb1;
	int i;

	if (!mu_scb) {
		return BCME_NOTFOUND;
	}

	ASSERT(mu_info->mu_clients[mu_scb->client_index] == scb);

	wlc_mutx_membership_clear(mu_info, mu_scb->client_index);
	mu_info->mu_clients[mu_scb->client_index] = NULL;
	mu_scb->client_index = MU_CLIENT_INDEX_NONE;
	mu_scb->retry_gid = MUTX_R_GID_RETRY_CNT;

	WL_MUMIMO(("wl%d: STA %s removed from MU client set.\n",
		mu_info->pub->unit, bcm_ether_ntoa(&scb->ea, eabuf)));

	/* Inform the ucode/hw of client set change */
	SCB_MU_DISABLE(scb);

	/* Update the ampdu_mpdu number */
	scb_ampdu_update_config(mu_info->wlc->ampdu_tx, scb);

	if (D11REV_IS(mu_info->wlc->pub->corerev, 64) && !SCB_LDPC_CAP(scb)) {
		for (i = 0; i < MUCLIENT_NUM; i++) {
			scb1 = mu_info->mu_clients[i];
			if (scb1 && SCB_MU(scb1) && !SCB_LDPC_CAP(scb1)) {
				return BCME_OK;
			}
		}
		/* No BCC STA, disable the WAR */
		wlc_mhf(mu_info->wlc, MXHF0, MXHF0_MUBCC, 0, WLC_BAND_ALL);
	}

	return BCME_OK;
}

/* Clear the entire MU client set. */
static void
mu_client_set_clear(wlc_mutx_info_t *mu_info)
{
	int i;
	struct scb *scb;

	for (i = 0; i < MUCLIENT_NUM; i++) {
		scb = mu_info->mu_clients[i];
		if (scb) {
			mu_client_set_remove(mu_info, scb);
		}
	}
}

/* Report whether a given STA is one of the current MU clients. */
static bool
mu_in_client_set(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	return (wlc_mutx_sta_client_index(mu_info, scb) != MU_CLIENT_INDEX_NONE);
}

/* Returns TRUE if the current MU client set is full, or reach to max_muclients limitation. */
static bool
mu_client_set_full(wlc_mutx_info_t *mu_info)
{
	int i, clients = 0;

	for (i = 0; i < MUCLIENT_NUM; i++) {
		if (mu_info->mu_clients[i])
			clients ++;
	}

	if (clients >= mu_info->pub->max_muclients)
		return TRUE;

	return FALSE;
}

/* Check if the addition of a new MU-capable STA should cause a change
 * to the MU client set. For now, if the MU client set is not full,
 * and MU TX is active, the STA is added to the MU client set. In the future,
 * this could be one trigger for the MU client selection algorithm to run.
 */
static void
mu_client_set_new_sta(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	mutx_scb_t *mu_scb = MUTX_SCB(mu_info, scb);

	if (!mu_info->mutx_active)
		return;

	if (mu_client_set_full(mu_info)) {
		return;
	}

	if (mu_scb == NULL) {
		return;
	}

	/* Candidate set has room to add new STA. */
	(void)mu_client_set_add(mu_info, scb);
}

/* Check if the departure of an MU-capable STA affects the MU client set.
 * If the STA is a MU client, remove the STA from the MU client set.
 * If another MU-capable STA is ready, add it to the MU client set.
 */
static void
mu_client_set_departed_sta(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	struct scb *new_cand;

	/* Remove scb from candidate set if currently a member */
	if (!mu_in_client_set(mu_info, scb)) {
		return;
	}
	/* Before removing the old candidate, find potential replacement. */
	new_cand = mu_candidate_select(mu_info);
	if (mu_client_set_remove(mu_info, scb) == BCME_OK) {
		/* Space now available in candidate set. */
		if (new_cand) {
			mu_client_set_new_sta(mu_info, new_cand);
		}
	}
}

/* Select an MU-capable STA to be an MU client. No rocket science here.
 * Just select the first MU-capable STA that is not currently in the
 * MU client set.
 */
static struct scb*
mu_candidate_select(wlc_mutx_info_t *mu_info)
{
	wlc_info_t *wlc = mu_info->wlc;
	struct scb *scb;
	struct scb_iter scbiter;
	mutx_scb_t *mu_scb;

	if (!mu_info->mutx_active)
		return NULL;

	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		if (mu_sta_mu_ready(mu_info, scb)) {
			mu_scb = MUTX_SCB(mu_info, scb);
			if (!mu_scb) {
				/* Another chance to allocate cubby if failed earlier */
				mu_scb = mu_scb_cubby_alloc(mu_info, scb);
				if (!mu_scb) {
					continue;
				}
			}
			if (mu_scb->client_index == MU_CLIENT_INDEX_NONE) {
				return scb;
			}
		}
	}
	return NULL;
}

/* For now, the first 8 MU-capable STAs are designated candidates. They are
 * added to the MU client set when they associate. A more sophisticated
 * MU client selection algorithm could be inserted here.
 */
static int
mu_clients_select(wlc_mutx_info_t *mu_info)
{
	struct scb *scb;

	scb = mu_candidate_select(mu_info);
	while (scb && !mu_client_set_full(mu_info)) {
		mu_client_set_new_sta(mu_info, scb);
		scb = mu_candidate_select(mu_info);
	}

	mu_info->muclient_elect_pending = FALSE;
	return BCME_OK;
}

/* Take all actions required when a new MU-capable STA is available.
 * Inputs:
 *   mu_info - MU TX state
 *   scb     - the new MU-capable STA
 * Returns:
 *   BCME_OK if successful
 *   BCME_NOMEM if memory allocation for the scb cubby failed
 */
static int
mu_sta_mu_enable(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	mutx_scb_t *mu_scb;
	mutx_scb_cubby_t *cubby = MUTX_SCB_CUBBY(mu_info, scb);

	if (!cubby->mutx_scb && !SCB_INTERNAL(scb)) {
		mu_scb = mu_scb_cubby_alloc(mu_info, scb);
		if (!mu_scb) {
			return BCME_NOMEM;
		}
	}

	if (!mu_in_client_set(mu_info, scb)) {
		mu_client_set_new_sta(mu_info, scb);
	}
	return BCME_OK;
}

/* Take actions required when a STA disassociates or is no longer is MU-capable. */
static int
mu_sta_mu_disable(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	mutx_scb_cubby_t *cubby = MUTX_SCB_CUBBY(mu_info, scb);

	/* Remove scb from MU client set if a member */
	mu_client_set_departed_sta(mu_info, scb);

	if (cubby->mutx_scb) {
#ifdef WLCNT
		if (cubby->mutx_scb->mutx_stats != NULL) {
			MFREE(mu_info->osh, cubby->mutx_scb->mutx_stats, sizeof(mutx_stats_t));
		}
#endif
		MFREE(mu_info->osh, cubby->mutx_scb, sizeof(mutx_scb_t));
		cubby->mutx_scb = NULL;
	}

	return BCME_OK;
}

/* Report whether a given STA is MU capable.
 * STA must be VHT capable and advertise MU BEAMFORMEE capability.
 * Internal scbs are not considered MU capable.
 * Returns TRUE if STA is MU capable.
 */
static bool
mu_sta_mu_capable(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	wlc_info_t *wlc = mu_info->wlc;
	uint16 vht_flags;

	if (SCB_INTERNAL(scb)) {
		return FALSE;
	}

	if (!SCB_VHT_CAP(scb)) {
		/* STA not VHT capable. Can't be MU capable. */
		return FALSE;
	}

	vht_flags = wlc_vht_get_scb_flags(wlc->vhti, scb);
	if ((vht_flags & SCB_MU_BEAMFORMEE) == 0) {
		return FALSE;
	}

	return TRUE;
}

/* Report whether a given STA is ready to be an MU receiver.
 * STA must MU capable.
 * STA must also be associated to a BSS where this device is acting as an AP.
 * Returns TRUE if STA is MU capable.
 */
static bool
mu_sta_mu_ready(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	wlc_info_t *wlc = mu_info->wlc;
	uint32 bw_policy;

	if (!mu_sta_mu_capable(mu_info, scb)) {
		return FALSE;
	}

	/* Check BW policy */
	bw_policy = wlc_mutx_bw_policy(mu_info);
	if (wlc_scb_ratesel_getbwcap(wlc->wrsi, scb, 0) != bw_policy)
		return FALSE;

	if (!SCB_ASSOCIATED(scb) || !BSSCFG_AP(scb->bsscfg)) {
		return FALSE;
	}

	/* Can come here looking for a STA to replace a STA just removed
	 * from the MU client set. If client was removed because its BSS
	 * is going down, avoid selecting another STA in the same BSS.
	 */
	if (!scb->bsscfg->up) {
		return FALSE;
	}

	return TRUE;
}

/* Return TRUE if a given STA's group membership has changed, but the STA
 * has not yet acknowledged the Group ID Mgmt frame that informed it of
 * the change.
 * Return FALSE if the STA has acknowledged receipt of its current
 * membership status.
 */
static bool
mu_scb_membership_change_get(wlc_info_t *wlc, struct scb *scb)
{
	wlc_mutx_info_t *mu_info = (wlc_mutx_info_t*) wlc->mutx;
	mutx_scb_t *mu_scb = MUTX_SCB(mu_info, scb);

	if (mu_scb)
		return mu_scb->membership_change;
	else
		return FALSE;
}

/* Update the scb state that indicates whether this AP is waiting for the scb
 * to acknowledge receipt of a Group ID Mgmt frame.
 */
static void
mu_scb_membership_change_set(wlc_info_t *wlc, struct scb *scb, bool pending)
{
	wlc_mutx_info_t *mu_info = (wlc_mutx_info_t*) wlc->mutx;
	mutx_scb_t *mu_scb = MUTX_SCB(mu_info, scb);

	if (!mu_scb)
		return;

	mu_scb->membership_change = pending;

	/* is NONE then we came here from clear/remove */
	if (mu_any_membership(mu_scb->membership)) {
		SCB_MU_ENABLE(scb);
		/* tell HW now */

		/* Update beamforming link */
		wlc_txbf_link_upd(mu_info->wlc->txbf, scb);

		/* Update the ampdu_mpdu number */
		scb_ampdu_update_config(mu_info->wlc->ampdu_tx, scb);

		if (D11REV_IS(wlc->pub->corerev, 64) && !SCB_LDPC_CAP(scb)) {
			wlc_mhf(wlc, MXHF0, MXHF0_MUBCC, MXHF0_MUBCC, WLC_BAND_ALL);
		}
	}
}

/* Get the MU client index for a given STA.
 * Returns the client index (0 to 7) if STA is one of the current MU clients.
 * If STA is not in the current MU client set, returns MU_CLIENT_INDEX_NONE.
 * Caller may wish to call wlc_mutx_is_muclient() to verify that STA is a
 * fully-qualified MU client.
 */
uint16
wlc_mutx_sta_client_index(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	mutx_scb_t *mu_scb;

	if (SCB_INTERNAL(scb)) {
		return MU_CLIENT_INDEX_NONE;
	}

	mu_scb = MUTX_SCB(mu_info, scb);
	if (mu_scb)
		return mu_scb->client_index;
	else
		return MU_CLIENT_INDEX_NONE;
}

#ifdef WL_MUPKTENG
uint8
wlc_mutx_pkteng_on(wlc_mutx_info_t *mu_info)
{
	return mu_info->mutx_pkteng;
}
#endif

#if defined(BCMDBG_MU)
uint8
wlc_mutx_on(wlc_mutx_info_t *mu_info)
{
	return mu_info->mutx_on;
}
#endif

int
wlc_mutx_switch(wlc_info_t *wlc, bool mutx_feature, bool is_iov)
{
	int err = 0;

	if (D11REV_LT(wlc->pub->corerev, 64))
		return BCME_UNSUPPORTED;

	/* If we have a forced setting and the switch request does not come from an IOVAR,
	 * then we just ignore the request.
	 */
	if (!(wlc->pub->mu_features & MU_FEATURES_AUTO) && !is_iov)
		return BCME_EPERM;

	WL_MUMIMO(("wl%d: %s: switching to %s-TX\n",
		wlc->pub->unit, __FUNCTION__, mutx_feature? "MU":"SU"));

	if (mutx_feature) {
		wlc->pub->mu_features |= WLC_MU_BFR_CAP_PHY(wlc) ? MU_FEATURES_MUTX : 0;
	} else {
		wlc->pub->mu_features &= ~MU_FEATURES_MUTX;
	}
#if defined(BCM_DMA_CT) && !defined(BCM_DMA_CT_DISABLED)
	wlc_bmac_ctdma_update(wlc->hw, mutx_feature);
#endif
	/* Control mutx feature based on Mu txbf capability. */
	wlc_mutx_update(wlc, wlc_txbf_mutx_enabled(wlc->txbf));

#if defined(BCM_DMA_CT) && defined(WLC_LOW)
	if (wlc->txdma_timer) {
		wl_del_timer(wlc->wl, wlc->txdma_timer);
	}
#endif
	if (wlc->pub->up) {
#ifdef WL_PSMX
		wlc_mutx_hostflags_update(wlc);
#endif /* WL_PSMX */
		wlc_mutx_active_update(wlc->mutx);
#ifdef WL_PSMX
		wlc_txbf_mutimer_update(wlc->txbf);
#endif /* WL_PSMX */
#ifdef WL_BEAMFORMING
		if (wlc->pub->associated && TXBF_ENAB(wlc->pub)) {
			wlc_bsscfg_t *bsscfg;
			struct scb *scb;
			struct scb_iter scbiter;
			int i;
			FOREACH_BSS(wlc, i, bsscfg) {
				FOREACH_BSS_SCB(wlc->scbstate, &scbiter, bsscfg, scb) {
					wlc_txbf_delete_link(wlc->txbf, scb);
				}
			}
		}
#endif
		wlc_mu_reinit(wlc);
#if defined(BCM_DMA_CT) && defined(WLC_LOW)
		if (BCM_DMA_CT_ENAB(wlc) && wlc->cpbusy_war) {
			wl_add_timer(wlc->wl, wlc->txdma_timer, 10, TRUE);
		}
#endif
	}

	return (err);
}

void BCMFASTPATH
wlc_mutx_sta_txfifo(wlc_mutx_info_t *mu_info, struct scb *scb, uint *pfifo)
{
	mutx_scb_t *mu_scb;

	if (!mu_info->mutx_active)
		return;

	mu_scb = MUTX_SCB(mu_info, scb);
	if (mu_scb && (mu_scb->client_index != MU_CLIENT_INDEX_NONE)) {
		*pfifo = mutx_aqmfifo[mu_scb->client_index][*pfifo];
	}
}

/* Report whether a given STA is one of the current MU clients. For this API,
 * a STA is only considered a fully qualified MU client if this AP has received
 * acknowledgement of the most recent Group ID Mgmt frame sent to the STA.
 */
bool
wlc_mutx_is_muclient(wlc_mutx_info_t *mu_info, struct scb *scb)
{
	if (SCB_INTERNAL(scb)) {
		return FALSE;
	}

	if (mu_scb_membership_change_get(mu_info->wlc, scb))
		return FALSE;

	return (wlc_mutx_sta_client_index(mu_info, scb) != MU_CLIENT_INDEX_NONE);
}

/* Report whether a given STA is in an MU group
 * Inputs:
 *   mu_info  - MU TX state
 *   scb      - STA in question
 *   group_id - Either a specific MU group ID, 1 - 62, or can use 0 as
 *              a wildcard, in which case report whether STA is a member
 *              of any MU group
 * Returns:
 *   TRUE if STA is a group member
 *   FALSE otherwise
 */
bool
wlc_mutx_is_group_member(wlc_mutx_info_t *mu_info, struct scb *scb, uint16 group_id)
{
	mutx_scb_t *mu_scb = MUTX_SCB(mu_info, scb);

	if (mu_scb) {
		if (group_id == MU_GROUP_ID_ANY) {
			return mu_any_membership(mu_scb->membership);
		} else {
			return isset(mu_scb->membership, group_id);
		}
	}
	return FALSE;
}

/* Callback function invoked when a STA's association state changes.
 * Inputs:
 *   ctx -        mutx state structure
 *   notif_data - information describing the state change
 */
static void
wlc_mutx_scb_state_upd(void *ctx, scb_state_upd_data_t *notif_data)
{
	wlc_mutx_info_t *mu_info = (wlc_mutx_info_t*) ctx;
	wlc_info_t *wlc = mu_info->wlc;
	struct scb *scb;
	uint8 oldstate;

	ASSERT(notif_data != NULL);

	scb = notif_data->scb;
	ASSERT(scb != NULL);
	oldstate = notif_data->oldstate;

	BCM_REFERENCE(wlc);

	wlc_mutx_bw_policy_update(mu_info);

	/* If this device is not MU tx capable, no need to maintain MU client set.
	 * Radio has to go down to enable MUTX; so clients will reassociate
	 * and AP can relearn them then.
	 */
	if (!MU_TX_ENAB(wlc))
		return;

	if ((oldstate & ASSOCIATED) && !SCB_ASSOCIATED(scb)) {
		/* STA has disassociated. If STA is in current MU client set,
		 * remove it.
		 */
		WL_MUMIMO(("wl%d: STA %s has disassociated.\n",
		          wlc->pub->unit, bcm_ether_ntoa(&scb->ea, eabuf)));
		mu_sta_mu_disable(mu_info, scb);
	}
	else if (mu_sta_mu_ready(mu_info, scb)) {
		/* MU capable STA now fully ready */
		WL_MUMIMO(("wl%d: MU capable STA %s has associated.\n",
		          wlc->pub->unit, bcm_ether_ntoa(&scb->ea, eabuf)));
		(void) mu_sta_mu_enable(mu_info, scb);
	}
}

/* Respond to a change in VHT capabilities for a given client. The client's
 * new capabilities must already be stored in the driver's internal data
 * structures when this function is called. If STA is capable of
 * acting as an MU-MIMO receiver, consider adding the STA to the set of
 * MU clients.
 *
 * This function is only called when MU-MIMO transmit is enabled on this device.
 * A call to this function is triggered by the following events:
 *
 *   - VHT is globally disabled on this device
 *   - This device is an AP that just received a frame which
 *     includes an updated set of VHT capabilities for the client
 *
 * Inputs:
 *      mu_info - MU-MIMO state
 *      scb     - STA whose VHT capabilities have just been received
 */
void
wlc_mutx_update_vht_cap(wlc_mutx_info_t *mu_info, struct scb *scb)
{

	/* If MU-MIMO transmit disabled locally, no need to process STA. */
	if (!MU_TX_ENAB(mu_info->wlc)) {
		return;
	}

	WL_MUMIMO(("wl%d: MU-MIMO module notified of VHT capabilities update for STA %s.\n",
	         mu_info->pub->unit, bcm_ether_ntoa(&scb->ea, eabuf)));

	if (mu_sta_mu_ready(mu_info, scb)) {
		(void) mu_sta_mu_enable(mu_info, scb);
	}
	else {
		mu_sta_mu_disable(mu_info, scb);
	}
}

/* Enable or disable Mu Tx feature */
void
wlc_mutx_update(wlc_info_t *wlc, bool enable)
{
	/* Mu tx feature depends on CutThru DMA support, but mu_features would override
	 * the CTDMA setting.
	 */
	if ((wlc->pub->mu_features & MU_FEATURES_MUTX) != 0) {
		wlc->_mu_tx = enable;
	} else {
		/* Force disable mutx feature. */
		wlc->_mu_tx = FALSE;
	}
}

#ifdef WL_PSMX
void
wlc_mutx_hostflags_update(wlc_info_t *wlc)
{
	if (MU_TX_ENAB(wlc)) {
		wlc_mhf(wlc, MXHF0, MXHF0_CHKFID, MXHF0_CHKFID, WLC_BAND_ALL);

		/* When MUTX is enabled, which implies CTDMA needs to be on,
		 * we will enable preloading for 4365c0.
		 */
		wlc_mhf(wlc, MHF2, MHF2_PRELD_GE64, 0, WLC_BAND_ALL);
		if (D11REV_IS(wlc->pub->corerev, 65)) {
			wlc_mhf(wlc, MHF2, MHF2_PRELD_GE64, MHF2_PRELD_GE64, WLC_BAND_ALL);
		}
	} else {
		wlc_mhf(wlc, MXHF0, MXHF0_CHKFID, 0, WLC_BAND_ALL);

		/* When MUTX is disabled, preloading is enabled if CTDMA is on.
		 */
		wlc_mhf(wlc, MHF2, MHF2_PRELD_GE64, 0, WLC_BAND_ALL);
		if (wlc->_dma_ct_flags & DMA_CT_PRECONFIGURED) {
			if (D11REV_IS(wlc->pub->corerev, 64) ||
			    D11REV_IS(wlc->pub->corerev, 65)) {
				wlc_mhf(wlc, MHF2, MHF2_PRELD_GE64, MHF2_PRELD_GE64, WLC_BAND_ALL);
			}
		}
	}
}
#endif /* WL_PSMX */

void
wlc_mutx_active_update(wlc_mutx_info_t *mu_info)
{
	wlc_info_t *wlc = mu_info->wlc;
	bool mutx_active = TRUE;
	bool mutx_was_active = mu_info->mutx_active;
	int txchains;

	/* This checks both phy capability and current configuration */
	if (!MU_TX_ENAB(wlc)) {
		WL_MUMIMO(("wl%d: MU TX not active. MU BFR not enabled.\n",
			wlc->pub->unit));
		mutx_active = FALSE;
	}

	/* Check number of transmit streams */
	txchains = WLC_BITSCNT(wlc->stf->txchain);
	if (txchains < MUTX_TXCHAIN_MIN) {
		WL_MUMIMO(("wl%d: MU TX not active. "
			   "Number of tx chains, %d, less than required, %u.\n",
			   wlc->pub->unit, txchains, MUTX_TXCHAIN_MIN));
		mutx_active = FALSE;
	}

	mu_info->mutx_active = mutx_active;

	if (!mutx_was_active && mutx_active) {
		/* Bring MU TX up. Schedule MU client election. Don't do election
		 * immediately in case there are other configuration or state changes
		 * that follow immediately after this one. Give system time to settle.
		 */
		mu_info->muclient_elect_pending = TRUE;
	} else if (mutx_was_active && !mutx_active) {
		/* Bring MU TX down */
		/* Clear MU client set */
		mu_client_set_clear(mu_info);
	}
}

/* Clear MU group ID membership for a client with a given client index. */
void
wlc_mutx_membership_clear(wlc_mutx_info_t *mu_info, uint16 client_index)
{
	mutx_scb_t *mu_scb;

	if (client_index >= MUCLIENT_NUM) {
		WL_ERROR(("wl%d: %s: MU client index %u out of range.\n",
		         mu_info->pub->unit, __FUNCTION__, client_index));
		return;
	}

	if (mu_info->mu_clients[client_index] == NULL) {
		WL_ERROR(("wl%d: %s: No MU client with client index %u.\n",
		         mu_info->pub->unit, __FUNCTION__, client_index));
		return;
	}

	mu_scb = MUTX_SCB(mu_info, mu_info->mu_clients[client_index]);
	if (mu_scb) {
		memset(mu_scb->membership, 0, MU_MEMBERSHIP_SIZE);
		memset(mu_scb->position, 0, MU_POSITION_SIZE);
	}

	/* Tell STA it no longer has any MU group membership */
	mu_group_id_msg_send(mu_info->wlc, mu_info->mu_clients[client_index], NULL, NULL);
}

/* Get the current membership and user positions for the STA at a given client index.
 * Could be used to get current state, modify membership, and call wlc_mutx_membership_set()
 * with modified state.
 *
 * Inputs:
 *   mu_info -    MU TX state
 *   client_index - Index of a STA in the MU client set.
 *   membership - (out) Group membership bit mask. 1 indicates client is a member of the
 *                corresponding group. Lowest order bit indicates membership
 *                status in group ID 0.
 *   position -   (out) User position array for this STA. Two bits per group. Same order
 *                as membership array (user pos for group 0 first).
 * Returns:
 *   BCME_OK if membership and position arrays are set
 *   BCME_RANGE if client_index is out of range
 *   BCME_ERROR if scb does not have a mutx cubby
 *   BCME_BADARG if membership or position array is NULL, or if there is no MU
 *               client at the given client index
 */
int
wlc_mutx_membership_get(wlc_mutx_info_t *mu_info, uint16 client_index,
	uint8 *membership, uint8 *position)
{
	mutx_scb_t *mu_scb;

	if (client_index >= MUCLIENT_NUM) {
		WL_ERROR(("wl%d: %s: MU client index %u out of range.\n",
			mu_info->pub->unit, __FUNCTION__, client_index));
		return BCME_RANGE;
	}

	if (mu_info->mu_clients[client_index] == NULL) {
		return BCME_BADARG;
	}

	mu_scb = MUTX_SCB(mu_info, mu_info->mu_clients[client_index]);
	if (mu_scb == NULL) {
		return BCME_ERROR;
	}

	if (!membership || !position) {
		return BCME_BADARG;
	}

	memcpy(membership, mu_scb->membership, MU_MEMBERSHIP_SIZE);
	memcpy(position, mu_scb->position, MU_POSITION_SIZE);

	return BCME_OK;
}

/* Set the MU group membership and user position for all MU groups for a STA with
 * a given client index.
 *
 * Inputs:
 *   mu_info - MU TX state
 *   client_index - Index of a STA in the MU client set.
 *   membership - Group membership bit mask. 1 indicates client is a member of the
 *                corresponding group. Lowest order bit indicates membership
 *                status in group ID 0.
 *   position - User position array for this STA. Two bits per group. Same order
 *              as membership array (user pos for group 0 first).
 *
 * Returns:
 *   BCME_OK if membership set
 *   BCME_RANGE if client_index is out of range
 *   BCME_BADARG if there is no MU client at the given client index
 *   BCME_ERROR if another error occurs
 */
int
wlc_mutx_membership_set(wlc_mutx_info_t *mu_info, uint16 client_index,
	uint8 *membership, uint8 *position)
{
	mutx_scb_t *mu_scb;

	if (client_index >= MUCLIENT_NUM) {
		WL_ERROR(("wl%d: %s: MU client index %u out of range.\n",
			mu_info->pub->unit, __FUNCTION__, client_index));
		return BCME_RANGE;
	}

	if (mu_info->mu_clients[client_index] == NULL) {
		return BCME_BADARG;
	}

	if (!membership || !position) {
		return BCME_BADARG;
	}

	mu_scb = MUTX_SCB(mu_info, mu_info->mu_clients[client_index]);
	if (mu_scb == NULL) {
		mu_scb = mu_scb_cubby_alloc(mu_info, mu_info->mu_clients[client_index]);
	}
	if (mu_scb == NULL) {
		return BCME_ERROR;
	}

	/* Test if this client's group membership has changed */
	if (memcmp(mu_scb->membership, membership, MU_MEMBERSHIP_SIZE) ||
	    memcmp(mu_scb->position, position, MU_POSITION_SIZE)) {
		mu_scb->membership_change = TRUE;
	}

	memcpy(mu_scb->membership, membership, MU_MEMBERSHIP_SIZE);
	memcpy(mu_scb->position, position, MU_POSITION_SIZE);

	if (mu_scb->membership_change) {
		/* Tell STA new group membership and user position data */
		mu_scb->gid_send_cnt = 0;
		mu_scb->gid_retry_war = GID_RETRY_DELAY;
		if (mu_group_id_msg_send(mu_info->wlc, mu_info->mu_clients[client_index],
		                         mu_scb->membership, mu_scb->position) == BCME_OK) {
			mu_scb->gid_send_cnt++;
		}
	}


	return BCME_OK;
}

/* Build and queue a Group ID Management frame to a given STA.
 * Inputs:
 *      wlc - radio
 *      scb - STA to receive the GID frame
 *      membership - membership array to put in the message. If NULL, send a GID message
 *                   indicating that the STA is not a member of any MU group
 *      user_pos   - user position array to put in the message. May be NULL if membership
 *                   is NULL.
 * Returns BCME_OK     if message sent and send status callback registered
 *         BCME_BADARG if called with an invalid argument
 *         BCME_NOMEM  if memory allocation failure
 *         BCME_ERROR  if send status callback registration failed
 */

static int
mu_group_id_msg_send(wlc_info_t *wlc, struct scb *scb, uint8 *membership, uint8 *user_pos)
{
	void *pkt;
	uint8 *pbody;
	uint body_len;             /* length in bytes of the body of the group ID frame */
	struct dot11_action_group_id *gid_msg;
	wlc_bsscfg_t *bsscfg;      /* BSS config for BSS to which STA is associated */
	struct ether_addr *da;     /* RA for group ID frame */
	mutx_pkt_info_t *pkt_info;  /* packet info for callback */
	int ret;
#if defined(BCMDBG_ERR)
	char eabuf[ETHER_ADDR_STR_LEN];
#endif
	mutx_scb_t *mu_scb;
	wlc_mutx_info_t *mu_info = (wlc_mutx_info_t*) wlc->mutx;

	mu_scb = MUTX_SCB(mu_info, scb);
	ASSERT(mu_scb != NULL);
	if (!mu_scb) {
		return BCME_ERROR;
	}

	if (mu_scb->retry_gid == MUTX_R_GID_DISABLED) {
		return BCME_ERROR;
	}

	if (!user_pos && membership) {
		WL_ERROR(("wl%d: User position must be specified if membership is specified.\n",
		          wlc->pub->unit));
		/* Will retx at next tick */
		mu_scb->retry_gid = MUTX_R_GID_RETRY_CNT;
		return BCME_BADARG;
	}

	/* Allocate frame */
	WL_MUMIMO(("wl%d: Sending Group ID Mgmt frame to STA %s.\n",
	          wlc->pub->unit, bcm_ether_ntoa(&scb->ea, eabuf)));

	bsscfg = scb->bsscfg;
	da = &scb->ea;
	body_len = sizeof(struct dot11_action_group_id);
	pkt = wlc_frame_get_mgmt(wlc, FC_ACTION, da, &bsscfg->cur_etheraddr, &bsscfg->BSSID,
	                         body_len, &pbody);
	if (pkt == NULL) {
		WL_ERROR(("wl%d: No memory to allocate Group ID frame to %s.\n",
		wlc->pub->unit, bcm_ether_ntoa(da, eabuf)));
		mu_scb->retry_gid = MUTX_R_GID_NO_BUFFER;
		return BCME_NOMEM;
	}

	PKTSETPRIO(pkt, PRIO_8021D_VO);
	/* Write body of frame */
	gid_msg = (struct dot11_action_group_id*) pbody;
	gid_msg->category = DOT11_ACTION_CAT_VHT;
	gid_msg->action = DOT11_VHT_ACTION_GID_MGMT;

	if (membership) {
		memcpy(gid_msg->membership_status, membership, DOT11_ACTION_GID_MEMBERSHIP_LEN);
		memcpy(gid_msg->user_position, user_pos, DOT11_ACTION_GID_USER_POS_LEN);
	} else {
		memset(gid_msg->membership_status, 0, DOT11_ACTION_GID_MEMBERSHIP_LEN);
		memset(gid_msg->user_position, 0, DOT11_ACTION_GID_USER_POS_LEN);
	}


	/* Register callback to be invoked when tx status is processed for this frame */
	pkt_info = (mutx_pkt_info_t*) MALLOCZ(wlc->osh, sizeof(mutx_pkt_info_t));
	if (pkt_info == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		         wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		mu_scb->retry_gid = MUTX_R_GID_NO_BUFFER;
		PKTFREE(wlc->osh, pkt, TRUE);
		return BCME_NOMEM;
	}
	memcpy(&pkt_info->ea, &scb->ea, sizeof(struct ether_addr));
	pkt_info->bssID = bsscfg->ID;
	ret = wlc_pcb_fn_register(wlc->pcb, wlc_mutx_gid_complete, (void*) pkt_info, pkt);
	if (ret != 0) {
		/* We won't get status for tx of this frame. So we will retransmit,
		 * even if it gets ACK'd. Should be a rare case, and retx isn't harmful.
		 */
		MFREE(wlc->osh, pkt_info, sizeof(mutx_pkt_info_t));
		WL_MUMIMO(("wl%d: Failed to register for status of Group ID Mgmt "
		           "frame to STA %s.\n",
		           wlc->pub->unit, bcm_ether_ntoa(&scb->ea, eabuf)));
		mu_scb->retry_gid = MUTX_R_GID_CALLBACK_FAIL;
		PKTFREE(wlc->osh, pkt, TRUE);
		return BCME_ERROR;
	}

	ret = wlc_sendmgmt(wlc, pkt, bsscfg->wlcif->qi, scb);
	if (!ret) {
		/* Cancel any pending wlc_mutx_gid_complete packet callbacks. */
		wlc_pcb_fn_find(wlc->pcb, wlc_mutx_gid_complete, (void*) pkt_info, TRUE);
		mu_scb->retry_gid = MUTX_R_GID_NO_BUFFER;
		MFREE(wlc->osh, pkt_info, sizeof(mutx_pkt_info_t));
		return BCME_ERROR;

	}

	mu_scb->retry_gid = MUTX_R_GID_DISABLED;
	return BCME_OK;
}

/* Callback invoked when tx status is processed for a group ID mgmt frame.
 * If the STA ack'd the frame, then we know it is prepared to receive
 * MU-PPDUs from this AP. If not ACK'd, we will retransmit.
 */
static void
wlc_mutx_gid_complete(wlc_info_t *wlc, uint txstatus, void *arg)
{
	mutx_pkt_info_t *pkt_info = (mutx_pkt_info_t*) arg;
	wlc_bsscfg_t *bsscfg = wlc_bsscfg_find_by_ID(wlc, pkt_info->bssID);
	struct scb *scb;
#if defined(BCMDBG_ERR)
	char eabuf[ETHER_ADDR_STR_LEN];
#endif
	mutx_scb_t *mu_scb;
	wlc_mutx_info_t *mu_info = (wlc_mutx_info_t*) wlc->mutx;


	/* in case bsscfg is freed before this callback is invoked */
	if (bsscfg == NULL) {
		MFREE(wlc->osh, pkt_info, sizeof(mutx_pkt_info_t));
		return;
	}
	if (ETHER_ISMULTI(&pkt_info->ea)) {
		WL_ERROR(("wl%d: Group ID message complete callback with mcast ea %s.\n",
		          wlc->pub->unit, bcm_ether_ntoa(&pkt_info->ea, eabuf)));
		return;
	}

	if ((scb = wlc_scbfind(wlc, bsscfg, &pkt_info->ea)) == NULL) {
		MFREE(wlc->osh, pkt_info, sizeof(mutx_pkt_info_t));
		return;
	}

	mu_scb = MUTX_SCB(mu_info, scb);
	if (!mu_scb) {
		MFREE(wlc->osh, pkt_info, sizeof(mutx_pkt_info_t));
		return;
	}

	if (txstatus & TX_STATUS_ACK_RCV) {
		/* STA is now prepared to receive MU-PPDUs */
		mu_scb_membership_change_set(wlc, scb, FALSE);
		WL_MUMIMO(("wl%d: STA %s acknowledged MU-MIMO Group ID message.\n",
		        wlc->pub->unit, bcm_ether_ntoa(&scb->ea, eabuf)));
	} else {
		WL_MUMIMO(("wl%d: STA %s failed to acknowledge MU-MIMO Group ID message.\n",
		        wlc->pub->unit, bcm_ether_ntoa(&scb->ea, eabuf)));
		/* Will retx at next tick */
		mu_scb->retry_gid = MUTX_R_GID_RETRY_CNT;
	}
	MFREE(wlc->osh, pkt_info, sizeof(mutx_pkt_info_t));
}


#ifdef WL_MUPKTENG
struct mupkteng_clinfo
{
	int idx;
	int aid;
	int fifo;
	struct scb *scb;

} mupkteng_clinfo[] = {
	{0, 1, 8, NULL},
	{1, 2, 11, NULL},
	{2, 3, 14, NULL},
	{3, 4, 17, NULL},
	{4, 5, 20, NULL},
	{5, 6, 23, NULL},
	{6, 7, 26, NULL},
	{7, 8, 29, NULL}
	};

static int
wlc_mupkteng_addsta(wlc_mutx_info_t *mu_info, struct ether_addr *ea, int client_idx, int nrxchain)
{
	struct scb *scb;
	char eabuf[ETHER_ADDR_STR_LEN];
	vht_cap_ie_t vht_cap;
	wlc_info_t *wlc =  mu_info->wlc;
	int err = BCME_OK;
	BCM_REFERENCE(eabuf);
	if (!wlc_mutx_pkteng_on(mu_info)) {
		WL_ERROR(("%s: mutx_pkteng is not set\n", __FUNCTION__));
		return BCME_ERROR;
	}

	WL_MUMIMO(("%s ea %s idx %d nrxchain %d\n", __FUNCTION__, bcm_ether_ntoa(ea, eabuf),
		client_idx, nrxchain));

	wlc = mu_info->wlc;

	if ((client_idx < 0) || (client_idx > 7))
		return BCME_ERROR;

	if (ETHER_ISNULLADDR(ea) || ETHER_ISMULTI(ea))
		 return BCME_ERROR;

	if (nrxchain == 0)
		 return BCME_ERROR;


	if (mupkteng_clinfo[client_idx].scb != NULL) {
		WL_ERROR(("%s client ea %s already exists @idx %d\n", __FUNCTION__,
		bcm_ether_ntoa(&mupkteng_clinfo[client_idx].scb->ea, eabuf), client_idx));
		return BCME_ERROR;
	}

	scb = wlc_scblookup(wlc, wlc->cfg, ea);
	if (scb == NULL) {
		WL_ERROR(("Cannot create scb for ea %s\n", bcm_ether_ntoa(ea, eabuf)));
		return BCME_ERROR;
	}
	/* Make STA VHT capable */
	scb->flags2 |= SCB2_VHTCAP;
	wlc_vht_update_scb_state(wlc->vhti, wlc->band->bandtype,
	                         scb, 0, &vht_cap, 0, 0);
	wlc_scb_setstatebit(scb, AUTHENTICATED);
	wlc_scb_setstatebit(scb, ASSOCIATED);
	SCB_MU_ENABLE(scb);
	scb->aid = mupkteng_clinfo[client_idx].aid;
	err = wlc_txbf_mupkteng_addsta(wlc->txbf, scb, client_idx, nrxchain);

	if (err != BCME_OK) {
		wlc_scbfree(wlc, scb);
		return BCME_ERROR;
	}

	mupkteng_clinfo[client_idx].scb = scb;
	return BCME_OK;
}

static int
wlc_mupkteng_clrsta(wlc_mutx_info_t *mu_info, int client_idx)
{
	wlc_info_t *wlc =  mu_info->wlc;
	struct scb *scb;

	scb = mupkteng_clinfo[client_idx].scb;
	if (scb == NULL) {
		return BCME_OK;
	}
	wlc_txbf_mupkteng_clrsta(wlc->txbf, scb);
	wlc_scbfree(wlc, scb);
	mupkteng_clinfo[client_idx].scb = NULL;
	return BCME_OK;
}

static int
wlc_mupkteng_tx(wlc_mutx_info_t *mu_info, mupkteng_tx_t *mupkteng_tx)
{
	struct ether_addr *sa;
	char eabuf[ETHER_ADDR_STR_LEN];
	struct scb *scb;
	void *pkt = NULL;
	int fifo_idx = -1;
	wlc_info_t *wlc = mu_info->wlc;
	ratespec_t rate_override;
	int i, j, nframes, nclients, flen, ntx;
	wl_pkteng_t pkteng;
	uint16 seq;
	int err = BCME_OK;
	BCM_REFERENCE(eabuf);

	sa = &wlc->pub->cur_etheraddr;

	nclients = mupkteng_tx->nclients;
	pkteng.flags = WL_MUPKTENG_PER_TX_START;
	pkteng.delay = 60;
	pkteng.nframes = ntx = mupkteng_tx->ntx;

	WL_MUMIMO(("%s sa %s total clients %d ntx %d\n", __FUNCTION__, bcm_ether_ntoa(sa, eabuf),
		nclients, ntx));

	if ((nclients == 0) || (nclients > 8)) {
		return BCME_ERROR;
	}

	for (i = 0; i < nclients; i++) {
		scb = mupkteng_clinfo[mupkteng_tx->client[i].idx].scb;
		if (scb == NULL) {
			WL_ERROR(("%s scb is NULL @client_idx %d\n", __FUNCTION__, i));
			return BCME_ERROR;
		}
		WL_MUMIMO(("client %s: idx %d rspec %d  nframes %d flen %d \n",
			bcm_ether_ntoa(&scb->ea, eabuf), mupkteng_tx->client[i].idx,
			mupkteng_tx->client[i].rspec, mupkteng_tx->client[i].nframes,
			mupkteng_tx->client[i].flen));
	}
	rate_override = wlc->band->rspec_override;

	err = wlc_bmac_pkteng(wlc->hw, &pkteng, NULL);
	if ((err != BCME_OK)) {
		/* restore Mute State after pkteng is done */
		if (wlc_quiet_chanspec(wlc->cmi, wlc->chanspec))
			wlc_mute(wlc, ON, 0);
		return BCME_ERROR;
	}

	/* Unmute the channel for pkteng if quiet */
	if (wlc_quiet_chanspec(wlc->cmi, wlc->chanspec))
		wlc_mute(wlc, OFF, 0);

	wlc_bmac_suspend_macx_and_wait(wlc->hw);

	nclients = mupkteng_tx->nclients;
	for (i = 0; i < nclients; i++) {
		fifo_idx =  mupkteng_clinfo[mupkteng_tx->client[i].idx].fifo;
		nframes = mupkteng_tx->client[i].nframes;
		flen =  mupkteng_tx->client[i].flen;
		scb = mupkteng_clinfo[mupkteng_tx->client[i].idx].scb;

		if (scb == NULL) {
			WL_ERROR(("%s: scb @client idx %d is NULL\n", __FUNCTION__, i));
			pkteng.flags = WL_MUPKTENG_PER_TX_STOP;
			wlc_bmac_pkteng(wlc->hw, &pkteng, NULL);
			/* restore Mute State after pkteng is done */
			if (wlc_quiet_chanspec(wlc->cmi, wlc->chanspec))
				wlc_mute(wlc, ON, 0);
			return BCME_ERROR;
		}
		for (j = 0; j < nframes; j++) {
			seq = j + 1;
			pkt = wlc_mutx_testframe(wlc, scb, sa, rate_override, fifo_idx,
				flen, seq);
			if (pkt == NULL) {
				WL_ERROR(("%s: failed to allocate mutx testframe\n", __FUNCTION__));
				pkteng.flags = WL_MUPKTENG_PER_TX_STOP;
				wlc_bmac_pkteng(wlc->hw, &pkteng, NULL);
				/* restore Mute State after pkteng is done */
				if (wlc_quiet_chanspec(wlc->cmi, wlc->chanspec))
					wlc_mute(wlc, ON, 0);
				return BCME_ERROR;
			}
			wlc_bmac_txfifo(wlc->hw, fifo_idx, pkt, TRUE, INVALIDFID, 1);
		}
		WL_INFORM(("%s: client %s idx %d queued nframes %d flen %d fifo_idx %d\n",
			__FUNCTION__, bcm_ether_ntoa(&scb->ea, eabuf), i, nframes, flen, fifo_idx));
	}

	OSL_DELAY(5000);
	wlc_bmac_enable_macx(wlc->hw);
	return BCME_OK;
}
#endif /* WL_MUPKTENG */

#ifdef WLCNT
void
wlc_mutx_update_txcounters(wlc_mutx_info_t *mu_info, struct scb *scb, ratespec_t rspec,
		bool is_mu, bool txs_mu, uint16 ncons, uint16 nlost, uint8 gid)
{
	mutx_scb_t *mu_scb;
	uint8 nss, mcs;
	uint8 rate_index;

	if (!mu_info || !scb) {
		return;
	}

	ASSERT(gid <= VHT_SIGA1_GID_MAX_GID);

	mu_scb = MUTX_SCB(mu_info, scb);
	if (mu_scb == NULL) {
		return;
	}

	if (is_mu) {
		mu_info->queued_cnt += ncons;
		mu_scb->mutx_stats->queued_cnt += ncons;
		if (txs_mu) {
			mu_info->mutx_cnt += ncons;
			mu_scb->mutx_stats->mutx_cnt += ncons;
			mu_info->mutx_lost_cnt += nlost;
			mu_scb->mutx_stats->mutx_lost_cnt += nlost;
		} else {
			mu_info->sutx_lost_cnt += nlost;
			mu_scb->mutx_stats->sutx_lost_cnt += nlost;
		}
	}

	/* Update group ID count */
	mcs = rspec & RSPEC_VHT_MCS_MASK;
	nss = (rspec & RSPEC_VHT_NSS_MASK) >> RSPEC_VHT_NSS_SHIFT;
	ASSERT(mcs <= WLC_MAX_VHT_MCS);
	ASSERT((nss > 0) && (nss <= VHT_CAP_MCS_MAP_NSS_MAX));

	mu_scb->mutx_stats->gid_cnt[gid]++;

	/* Update MCS count */
	rate_index = mcs + ((nss - 1) * MUTX_MCS_NUM);
	ASSERT(rate_index < MUTX_MCS_INDEX_NUM);

	if (is_mu) {
		mu_scb->mutx_stats->mu_mcs_cnt[rate_index]++;
	} else {
		mu_scb->mutx_stats->su_mcs_cnt[rate_index]++;
	}

	mu_scb->mutx_stats->gid_last_rate[gid] = rate_index;

}
#endif   /* WLCNT */
#endif /* WL_MU_TX */
