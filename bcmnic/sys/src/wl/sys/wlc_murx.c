/*
 * MU-MIMO receive module for Broadcom 802.11 Networking Adapter Device Drivers
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_murx.c 600056 2015-11-17 18:09:05Z $
 */

/*
 * An MU-MIMO receiver listens for Group ID management frames, which it uses
 * to determine which MU-MIMO groups it is a member of, and for each group, the
 * user position in the VHT-SIG-A that indicates the number of spatial streams
 * in received MU-PPDUs to demodulate. This module extracts this information
 * from Group ID management frames, stores this state on a per-BSS basis, and
 * updates the hardware with new membership data.
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <osl.h>
#include <wlc_types.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wl_dbg.h>
#include <wlc_bsscfg.h>
#include <wlc_scb.h>
#include <wlc_txbf.h>
#include <wlc_murx.h>
#include <phy_mu_api.h>
#include <wlc_dump.h>

#ifdef WL_MU_RX

/* IOVAR table */
enum {
	IOV_MURX_MEMB_CLEAR,
	IOV_MURX_LAST
};

static const bcm_iovar_t murx_iovars[] = {
	{NULL, 0, 0, 0, 0 }
};

/* State structure for the MU-MIMO receive module created by
 * wlc_murx_module_attach().
 */
struct wlc_murx_info {
	osl_t *osh;              /* OSL handle */
	wlc_info_t *wlc;         /* wlc_info_t handle */
	wlc_pub_t *pub;          /* wlc_pub_t handle */

	/* BSS config cubby offset */
	int bsscfg_handle;

	/* MU group membership can only be written to hardware for a single BSS.
	 * A receiver can associate to multiple BSSs and receive GID update messages
	 * from each. To avoid overwriting the group membership in hardware, record
	 * the bsscfg whose group membership is currently in hardware. A GID message
	 * received in the same BSS can update the hw. GID messages received in other
	 * BSSs are ignored.
	 */
	wlc_bsscfg_t *mu_bss;

	/* Total number of Group ID mgmt frames received in all BSSs. */
	uint16 gid_msg_rx;

	/* Number of GID messages ignored because of memory allocation failure. */
	uint16 gid_msg_no_mem;

	/* Number of GID messages received from a sender not in an infrastructure BSS,
	 * or in a BSS where this device is not a STA.
	 */
	uint16 gid_msg_not_infrabss;

	/* Number of GID messages received from a sender not in the BSS this station
	 * is accepting group membership from.
	 */
	uint16 gid_msg_other_bss;
};

/* STA may participate in multiple BSSs with other APs or STAs that send MU-MIMO
 * frames. So membership and user position is stored per BSS.
 */
typedef struct murx_bsscfg {

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

	/* Number of Group ID Management frames received in this BSS */
	uint16 gid_msg_cnt;

	/* Number of times the hardware update of the group membership failed */
	uint16 hw_update_err;

} murx_bsscfg_t;

/* Actual BSS config cubby is just a pointer to dynamically allocated structure. */
typedef struct murx_bsscfg_cubby {
	murx_bsscfg_t *murx_bsscfg;
} murx_bsscfg_cubby_t;

/* Basic module infrastructure */
static int wlc_murx_up(void *mi);
static int murx_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid,
                        const char *name, void *p, uint plen, void *a,
                        int alen, int vsize, struct wlc_if *wlcif);

/* BSS configuration cubby routines */
static void murx_bsscfg_deinit(void *hdl, wlc_bsscfg_t *cfg);
#if defined(WLDUMP)
void murx_bsscfg_dump(void *ctx, wlc_bsscfg_t *cfg, struct bcmstrbuf *b);
static int wlc_murx_dump(wlc_murx_info_t *mu_info, struct bcmstrbuf *b);
#else
#define murx_bsscfg_dump NULL
#endif

static int
murx_grp_memb_hw_update(wlc_info_t *wlc, murx_bsscfg_t *mu_bsscfg, struct scb *scb,
	uint8 *new_membership, uint8 *user_position);

#define MURX_BSSCFG_CUBBY(murx, cfg) ((murx_bsscfg_cubby_t *)BSSCFG_CUBBY((cfg), \
	                                    (murx)->bsscfg_handle))
#define MURX_BSSCFG(murx, cfg) ((murx_bsscfg_t*)(MURX_BSSCFG_CUBBY(murx, cfg)->murx_bsscfg))


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

/* Return TRUE if the membership array indicates membership in at least one group. */
static bool
mu_is_group_member(uint8 *membership)
{
	uint32 *mw = (uint32*) membership;  /* start of one 4-byte word of membership array */
	int i;

	if (!membership)
		return FALSE;

	for (i = 0; i < (MU_MEMBERSHIP_SIZE / sizeof(uint32)); i++) {
		if (*mw != 0) {
			return TRUE;
		}
		mw++;
	}
	return FALSE;
}

/**
 * Create the MU-MIMO receive module infrastructure for the wl
 * driver. wlc_module_register() is called to register the
 * module's handlers.
 *
 * Returns
 *    A wlc_murx_info_t structure, or NULL in case of failure.
 */
wlc_murx_info_t*
BCMATTACHFN(wlc_murx_attach)(wlc_info_t *wlc)
{
	wlc_murx_info_t *mu_info;
	int err;

	/* allocate the main state structure */
	mu_info = MALLOCZ(wlc->osh, sizeof(wlc_murx_info_t));
	if (mu_info == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));

		return NULL;
	}

	mu_info->wlc = wlc;
	mu_info->pub = wlc->pub;
	mu_info->osh = wlc->osh;
	wlc->pub->cmn->_mu_rx = FALSE;

#ifdef WL_MU_RX_DISABLED
	return mu_info;
#else

	/* Avoid registering callbacks if phy is not MU capable */
	if (!WLC_MU_BFE_CAP_PHY(wlc)) {
		return mu_info;
	}

	err = wlc_module_register(mu_info->pub, murx_iovars, "murx", mu_info,
	                          murx_doiovar, NULL, wlc_murx_up, NULL);

	if (err != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed with error %d (%s).\n",
		          wlc->pub->unit, __FUNCTION__, err, bcmerrorstr(err)));

		/* use detach as a common failure deallocation */
		wlc_murx_detach(mu_info);
		return NULL;
	}

	/* reserve cubby in the bsscfg container for private data. Allocation of cubby
	 * structure is deferred to receipt of first GID message. So no need to
	 * register init function.
	 */
	if ((mu_info->bsscfg_handle = wlc_bsscfg_cubby_reserve(wlc,
	                                                       sizeof(struct murx_bsscfg_t*),
	                                                       NULL,
	                                                       murx_bsscfg_deinit,
	                                                       murx_bsscfg_dump,
	                                                       (void *)mu_info)) < 0) {
		WL_ERROR(("wl%d: %s: MURX failed to reserve BSS config cubby\n",
		          wlc->pub->unit, __FUNCTION__));
		wlc_murx_detach(mu_info);
		return NULL;
	}

#if defined(WLDUMP)
	wlc_dump_register(mu_info->pub, "murx", (dump_fn_t)wlc_murx_dump, mu_info);
#endif

	return mu_info;
#endif   /* !WL_MU_RX_DISABLED */
}

/* Free all resources associated with the MU-MIMO receive module.
 * This is done at the cleanup stage when freeing the driver.
 *
 * mu_info    MU-MIMO receive module state structure
 */
void
BCMATTACHFN(wlc_murx_detach)(wlc_murx_info_t *mu_info)
{
	if (mu_info == NULL) {
		return;
	}
#ifndef WL_MU_RX_DISABLED
	wlc_module_unregister(mu_info->pub, "murx", mu_info);
#endif
	MFREE(mu_info->osh, mu_info, sizeof(wlc_murx_info_t));
}

#if defined(WLDUMP)
/* Dump MU-MIMO state information. */
static int
wlc_murx_dump(wlc_murx_info_t *mu_info, struct bcmstrbuf *b)
{
	char ssidbuf[SSID_FMT_BUF_LEN];

	if (mu_info->mu_bss) {
		wlc_format_ssid(ssidbuf, mu_info->mu_bss->SSID, mu_info->mu_bss->SSID_len);
	} else {
		wlc_format_ssid(ssidbuf, "<none>", 6);
	}

	bcm_bprintf(b, "MU-MIMO receive state:\n");
	bcm_bprintf(b, "MU RX active in BSS %s\n", ssidbuf);
#ifdef WLCNT
	bcm_bprintf(b, "GID msgs rxd                  %u\n", mu_info->gid_msg_rx);
	bcm_bprintf(b, "GID msgs dropped. No mem.     %u\n", mu_info->gid_msg_no_mem);
	bcm_bprintf(b, "GID msgs rxd not in infra BSS %u\n", mu_info->gid_msg_not_infrabss);
	bcm_bprintf(b, "GID msgs rxd in other BSS     %u\n", mu_info->gid_msg_other_bss);
#endif
	return BCME_OK;
}
#endif 

/* IOVar handler for the MU-MIMO infrastructure module */
static int
murx_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid,
             const char *name,
             void *p, uint plen,
             void *a, int alen,
             int vsize,
             struct wlc_if *wlcif)
{
	int32 int_val = 0;
	int err = 0;

	if (plen >= (int)sizeof(int_val))
		memcpy(&int_val, p, sizeof(int_val));

	switch (actionid) {
		default:
			err = BCME_UNSUPPORTED;
			break;
	}

	return err;
}

/* Registered callback when radio comes up. */
static int
wlc_murx_up(void *mi)
{
	wlc_murx_info_t *mu_info = (wlc_murx_info_t*) mi;
	wlc_info_t *wlc = mu_info->wlc;

	/* Check if system is MU BFE capable, both physically and by current
	 * configuration, and if so, set _mu_rx.
	 */
	if (wlc_txbf_murx_capable(wlc->txbf)) {
		wlc->pub->cmn->_mu_rx = TRUE;
	} else {
		wlc->pub->cmn->_mu_rx = FALSE;
	}

	/* Have not yet decided which BSS we will do MU in */
	mu_info->mu_bss = NULL;

	return BCME_OK;
}

/* Deinitialize BSS config cubby. Free memory for cubby. */
static void
murx_bsscfg_deinit(void *hdl, wlc_bsscfg_t *cfg)
{
	wlc_murx_info_t *mu_info = (wlc_murx_info_t *)hdl;
	wlc_info_t *wlc = mu_info->wlc;
	murx_bsscfg_cubby_t *cubby = MURX_BSSCFG_CUBBY(mu_info, cfg);
	murx_bsscfg_t *mu_bsscfg = cubby->murx_bsscfg;

	/* If BSS going away is the BSS whose MU group membership was written
	 * to hardware, clear the membership info in hw.
	 */
	if (cfg == mu_info->mu_bss) {
		if (mu_bsscfg != NULL) {
			(void) murx_grp_memb_hw_update(wlc, mu_bsscfg, NULL,
				NULL, NULL);
		}
		mu_info->mu_bss = NULL;
	}
	/* If cubby was allocated for this bss, free it, regardless of whether
	 * MU was active in this BSS.
	 */
	if (mu_bsscfg != NULL) {
		MFREE(mu_info->osh, mu_bsscfg, sizeof(murx_bsscfg_t));
		cubby->murx_bsscfg = NULL;
	}
}

#if defined(WLDUMP)
void
murx_bsscfg_dump(void *ctx, wlc_bsscfg_t *cfg, struct bcmstrbuf *b)
{
	wlc_murx_info_t *mu_info = (wlc_murx_info_t *)ctx;
	murx_bsscfg_t *mu_bsscfg;
	uint16 g;
	uint8 pos;

	bcm_bprintf(b, "MU receive %sactive in this BSS\n",
		(mu_info->mu_bss == cfg) ? "" : "not ");

	mu_bsscfg = MURX_BSSCFG(mu_info, cfg);
	if (mu_bsscfg) {
		/* Dump group membership */
		bcm_bprintf(b, "MU-MIMO group membership\n");
		for (g = MU_GROUP_ID_MIN; g < MU_GROUP_ID_MAX; g++) {
			if (isset(mu_bsscfg->membership, g)) {
				pos = mu_user_pos_get(mu_bsscfg->position, g);
				bcm_bprintf(b, "    Group %u Pos %u\n", g, pos);
			}
		}
#ifdef WLCNT
		bcm_bprintf(b, "Number of Group ID msgs received: %u\n", mu_bsscfg->gid_msg_cnt);
		bcm_bprintf(b, "Number of hw update errors: %u\n", mu_bsscfg->hw_update_err);
#endif
	}
}
#endif   

/* Filter advertisement of MU BFE capability for a given BSS. A STA can only do
 * MU receive in one BSS. If doing MU rx in a BSS already, then do not advertise
 * MU BFE capability in other BSSs.
 * Inputs:
 *   mu_info - MU receive state
 *   bsscfg  - BSS configuration for BSS where capabilities are to be advertised
 *   cap     - (in/out) vht capabilities
 */
void
wlc_murx_filter_bfe_cap(wlc_murx_info_t *mu_info, wlc_bsscfg_t *bsscfg, uint32 *cap)
{
	if (mu_info->mu_bss && (mu_info->mu_bss != bsscfg)) {
		*cap &= ~VHT_CAP_INFO_MU_BEAMFMEE;
	}
}

/* Push the group membership and user positions for a given BSS to hardware.
 * Called after receiving a GID mgmt frame, but before updating the membership and
 * user position on the bsscfg, so that new membership can be compared with old.
 *
 * Inputs:
 *   wlc - radio
 *   mu_bsscfg - The current group membership and user positions for one BSS.
 *   scb - Source of the group membership information. May be NULL if clearing
 *         all group membership for the BSS.
 *   new_membership - membership array just received in GID mgmt frame. If NULL,
 *                    clear all group membership for this BSS.
 *   user_position - user position for new memberships. May be NULL if
 *                   new_membership is NULL.
 *
 * Returns:
 *   BCME_OK if membership successfully set
 *   BCME_ERROR if any other error
 */
static int
murx_grp_memb_hw_update(wlc_info_t *wlc, murx_bsscfg_t *mu_bsscfg, struct scb *scb,
	uint8 *new_membership, uint8 *user_position)
{
	uint16 g;
	uint8 pos = 0;
	uint8 old_pos = 0;
	wlc_phy_t *pih = WLC_PI(wlc);
	int err;
	int rv = BCME_OK;
	bool was_member;         /* TRUE if STA was previously a member of a given group */
	bool is_member;          /* TRUE if STA is now a member of a given group */
	DBGERRONLY(
	char ssidbuf[SSID_FMT_BUF_LEN];
	char eabuf[ETHER_ADDR_STR_LEN];
	char *change_type; )

	BCM_REFERENCE(scb);

	DBGERRONLY(
	if (scb) {
		wlc_format_ssid(ssidbuf, scb->bsscfg->SSID, scb->bsscfg->SSID_len);
	} else {
		wlc_format_ssid(ssidbuf, (uchar *)"", 0);
	})

	for (g = MU_GROUP_ID_MIN; g < MU_GROUP_ID_MAX; g++) {
		err = BCME_OK;
		was_member = isset(mu_bsscfg->membership, g);
		is_member = (new_membership && isset(new_membership, g));

		if (was_member && !is_member) {
			/* Lost membership in this group. pos value irrelevant. */
			DBGERRONLY(change_type = "remove"; )
			err = phy_mu_group_set((phy_info_t *)pih, g, 0, 0);
		}
		else if (!was_member && is_member) {
			/* Gained membership in this group. */
			DBGERRONLY(change_type = "add"; )
			pos = mu_user_pos_get(user_position, g);
			err = phy_mu_group_set((phy_info_t *)pih, g, pos, 1);
		} else if (was_member && is_member) {
			/* Still a member of group g. Check in user pos changed */
			DBGERRONLY(change_type = "modify"; )
			old_pos = mu_user_pos_get(mu_bsscfg->position, g);
			pos = mu_user_pos_get(user_position, g);
			if (pos != old_pos) {
				err = phy_mu_group_set((phy_info_t *)pih, g, pos, 1);
			}
		}
		/* no-op if was not and is not a member */
		if (err != BCME_OK) {
			WL_ERROR(("wl%d: %s: Failed to %s MU group %u "
				  "user position %u received from %s in BSS %s. "
				  "Error %d (%s).\n",
				  wlc->pub->unit, __FUNCTION__,
				  change_type, g, pos,
				  scb ? bcm_ether_ntoa(&scb->ea, eabuf) : "", ssidbuf,
				  err, bcmerrorstr(err)));
			rv = err;
		}
	}
	return rv;
}

/* Update MU-MIMO group membership and this STA's user position for all MU groups.
 * Called in response to receipt of a Group ID Management action frame.
 * Inputs:
 *   wlc - radio
 *   scb - Neighbor that sent the frame. Implies a BSS.
 *   membership_status - The membership status field in the GID message
 *   user_position - The user position array in the GID message
 * Returns:  BCME_OK
 *           BCME_NOMEM if memory allocation for bsscfg cubby fails
 */
int
wlc_murx_gid_update(wlc_info_t *wlc, struct scb *scb,
                    uint8 *membership_status, uint8 *user_position)
{
	wlc_murx_info_t *mu_info = wlc->murx;
	murx_bsscfg_cubby_t *cubby;
	murx_bsscfg_t *mu_bsscfg;

	if (!MU_RX_ENAB(wlc))
		return BCME_OK;

	if (!scb || SCB_INTERNAL(scb))
		return BCME_OK;

	WLCNTINCR(mu_info->gid_msg_rx);

	/* If this device is not a STA in an infrastructure BSS where the message
	 * was received, ignore the GID message.
	 */
	if (!BSSCFG_INFRA_STA(scb->bsscfg)) {
		WLCNTINCR(mu_info->gid_msg_not_infrabss);
		return BCME_OK;
	}

	/* STA can only accept the MU group membership from a single BSS. If we have
	 * already written group membership to hardware, then can only update the
	 * group membership if this message was received in the same BSS.
	 */
	if (mu_info->mu_bss && (mu_info->mu_bss != scb->bsscfg)) {
		WLCNTINCR(mu_info->gid_msg_other_bss);
		return BCME_OK;
	}

	mu_bsscfg = MURX_BSSCFG(mu_info, scb->bsscfg);
	if (mu_bsscfg == NULL) {
		/* allocate memory and point bsscfg cubby to it */
		if ((mu_bsscfg = MALLOCZ(mu_info->osh, sizeof(murx_bsscfg_t))) == NULL) {
			WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
				mu_info->pub->unit, __FUNCTION__, MALLOCED(mu_info->osh)));

			/* Since the ucode has already ACK'd the GID message but group state
			 * will not be updated in hardware, it is likely this station will
			 * receive MU-PPDUs that it is unable to demodulate.
			 */
			WLCNTINCR(mu_info->gid_msg_no_mem);
			return BCME_NOMEM;
		}
		cubby = MURX_BSSCFG_CUBBY(mu_info, scb->bsscfg);
		cubby->murx_bsscfg = mu_bsscfg;
	}

	WLCNTINCR(mu_bsscfg->gid_msg_cnt);
	if (mu_is_group_member(membership_status))
		mu_info->mu_bss = scb->bsscfg;   /* Remember the BSS where MU RX is active */
	else
		mu_info->mu_bss = NULL;          /* Not a member of any group in this BSS now */

	/* Push group membership and user pos to hardware. */
	if (murx_grp_memb_hw_update(wlc, mu_bsscfg, scb,
		membership_status, user_position) != BCME_OK) {
		WLCNTINCR(mu_bsscfg->hw_update_err);
	}

	memcpy(mu_bsscfg->membership, membership_status, MU_MEMBERSHIP_SIZE);
	memcpy(mu_bsscfg->position, user_position, MU_POSITION_SIZE);


	return BCME_OK;
}
#endif   /* WL_MU_RX */
