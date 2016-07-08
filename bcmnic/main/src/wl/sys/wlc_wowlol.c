/*
 * Broadcom 802.11 Wowl offload driver
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
 * $Id: wlc_wowlol.c 496689 2014-08-13 23:58:22Z $
 */


#include <osl.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_bmac.h>
#include <bcm_ol_msg.h>
#include <wlc_dngl_ol.h>
#include <wl_export.h>
#include <wlc_l2keepaliveol.h>
#include <wlc_wowlol.h>
#include <wl_mdns.h>
#include <rte_trap.h>

#define WOWL_PME_DEFERRAL_TIME	    25

typedef enum {
	wlc_wowl_defer_pme_assertion = 0
} wlc_wowl_deferral_type_t;

typedef enum {
	wlc_wowl_ol_state_disabled = 0,
	wlc_wowl_ol_state_starting,
	wlc_wowl_ol_state_enabled
} wlc_wowl_ol_state_t;

/* Wowl offload private info structure */
struct wlc_dngl_ol_wowl_info {
	wlc_dngl_ol_info_t	*wlc_dngl_ol;

	/* current WoWL state */
	wlc_wowl_ol_state_t	wowl_ol_state;

	/* one-shot timer for test and timer wake events */
	struct wl_timer		*wowl_test_timer;

	/* one-shot timer for PME assertion deferral */
	struct wl_timer		*wowl_deferral_timer;
	wlc_wowl_deferral_type_t    wowl_deferral_reason;
	uint32			deferred_wake_reason;

	/* WoWL state */
	bool			wowl_pme_asserted;
 };

/* Private functions */
static void wlc_wowl_ol_test_timer_cb(void *arg);
static void wlc_wowl_ol_deferral_timer_cb(void *arg);
static void wlc_wowl_ol_enable_completed(wlc_dngl_ol_wowl_info_t *wowl_ol);
static void wlc_wowl_ol_fw_halt(void *ctx);
static void wlc_wowl_ol_assert_pme(wlc_dngl_ol_wowl_info_t *wowl_ol, uint32 wake_reason);

/*
 * Initialize wowl private context.
 * Returns a pointer to the wowl private context, NULL on failure.
 */
wlc_dngl_ol_wowl_info_t *
BCMATTACHFN(wlc_wowl_ol_attach)(wlc_dngl_ol_info_t *wlc_dngl_ol)
{
	wlc_dngl_ol_wowl_info_t *wowl_ol;
	wlc_info_t				*wlc = wlc_dngl_ol->wlc;

	ENTER();

	/* allocate wowl private info struct */
	wowl_ol = (wlc_dngl_ol_wowl_info_t *)MALLOC(
						wlc_dngl_ol->osh,
						sizeof(wlc_dngl_ol_wowl_info_t));

	if (!wowl_ol) {
		WL_ERROR(("%s: wowl_ol malloc failed\n", __FUNCTION__));
		return NULL;
	}

	/* init wowl private info struct */
	bzero(wowl_ol, sizeof(wlc_dngl_ol_wowl_info_t));

	wowl_ol->wlc_dngl_ol = wlc_dngl_ol;

	/* Allocate the wowl timers */
	wowl_ol->wowl_test_timer =
		wl_init_timer(
		    wlc->wl,
		    wlc_wowl_ol_test_timer_cb,
		    wowl_ol,
		    "wowl_ol test timer");

	if (wowl_ol->wowl_test_timer == NULL) {
		WL_ERROR(("%s: wl_init_timer failed\n", __FUNCTION__));

		MFREE(wlc_dngl_ol->osh,	wowl_ol, sizeof(wlc_dngl_ol_wowl_info_t));

		return NULL;
	}

	wowl_ol->wowl_deferral_timer =
		wl_init_timer(
		    wlc->wl,
		    wlc_wowl_ol_deferral_timer_cb,
		    wowl_ol,
		    "wowl_ol deferral timer");

	if (wowl_ol->wowl_deferral_timer == NULL) {
		WL_ERROR(("%s: wl_init_timer\n", __FUNCTION__));

		MFREE(wlc_dngl_ol->osh,	wowl_ol, sizeof(wlc_dngl_ol_wowl_info_t));

		return NULL;
	}

	bzero((void *)RXOEWOWLINFO(wlc_dngl_ol), sizeof(wowl_host_info_t));

	WL_ERROR(("WoWL attached\n"));

	EXIT();

	return wowl_ol;
}

wlc_dngl_ol_wowl_info_t *gwowl_ol = NULL;

/* called from startarm.S hnd_die() */
static void
wlc_wowl_ol_fw_halt(void *ctx)
{
	WL_ERROR(("%s: ***FAULT*** trying to wake up host.\n", __FUNCTION__));

	/* wake the host up */
	wlc_wowl_ol_wake_host(gwowl_ol, NULL, 0, NULL, 0, WL_WOWL_FW_HALT);
}

void wlc_wowl_ol_send_proc(wlc_dngl_ol_wowl_info_t *wowl_ol, void *buf, int len)
{
	wlc_dngl_ol_info_t	*wlc_dngl_ol = wowl_ol->wlc_dngl_ol;
	wlc_info_t		*wlc = wlc_dngl_ol->wlc;
	olmsg_wowl_enable_start *wowl_enable_start_msg;
	olmsg_header		*msg_hdr;
	wowl_host_info_t *wowl_host_info;

	msg_hdr = (olmsg_header *)buf;

	WL_ERROR(("%s: message type:%d\n", __FUNCTION__, msg_hdr->type));

	switch (msg_hdr->type) {
		case BCM_OL_WOWL_ENABLE_START:
			wowl_enable_start_msg = (olmsg_wowl_enable_start *)buf;

			ASSERT(len >= sizeof(olmsg_wowl_enable_start));

			wowl_host_info = (wowl_host_info_t *)RXOEWOWLINFO(wlc_dngl_ol);
			bzero(wowl_host_info, sizeof(wowl_host_info_t));

			/*
			 * This host message only tells us that the host has started
			 * to enable WoWL. So, let any regietered modules know about
			 * the new WoWL state.
			 */
			if (wowl_ol->wowl_ol_state == wlc_wowl_ol_state_disabled) {
				wowl_ol->wowl_ol_state = wlc_wowl_ol_state_starting;

				WL_ERROR(("%s: WoWL Flags = 0x%08x\n",
					__FUNCTION__, wowl_enable_start_msg->wowl_cfg.wowl_flags));
			} else {
				WL_ERROR(("%s: Invalid BCM_OL_WOWL_ENABLE_START state = %08x\n",
					__FUNCTION__, wowl_ol->wowl_ol_state));
			}


			ASSERT(wowl_enable_start_msg->wowl_cfg.wowl_enabled);
			ASSERT(wowl_enable_start_msg->wowl_cfg.wowl_flags);

			/*
			 * Announce the BCM_OL_E_WOWL_START event in case someone
			 * is intertested
			 */
			wlc_dngl_ol_event(
				wlc_dngl_ol,
				BCM_OL_E_WOWL_START,
				&wowl_enable_start_msg->wowl_cfg);

			break;

		case BCM_OL_WOWL_ENABLE_COMPLETE:
			/*
			 * This host message only tells us that the host has completed
			 * enabling WoWL.
			 */
			WL_ERROR(("%s: BCM_OL_WOWL_ENABLE_COMPLETE\n", __FUNCTION__));

			wlc_bmac_sync_sw_state(wlc_dngl_ol->wlc_hw);

			if (wowl_ol->wowl_ol_state == wlc_wowl_ol_state_starting) {
				wowl_ol->wowl_ol_state = wlc_wowl_ol_state_enabled;

				/*
				 * Send ACK of the wowl enable msg to host. This allows the host
				 * to safely complete the remainder of the preparation for D3
				 * transition.
				 */
				wlc_wowl_ol_enable_completed(wowl_ol);

				/*
				 * Are we configured for wowl test?
				 */
				if (wlc_dngl_ol->wowl_cfg.wowl_flags & WL_WOWL_TST) {
					/*
					 * Start the timer for wowl test and wakeup the
					 * system after the timer expires.
					 */
					ASSERT(wlc_dngl_ol->wowl_cfg.wowl_test > 0);

					/* wake ucode and set MHF1_ULP and HPS bit */
					wlc_bmac_set_wake_ctrl(wlc_dngl_ol->wlc_hw, TRUE);
					wlc_bmac_mctrl(wlc_dngl_ol->wlc_hw, MCTL_HPS, MCTL_HPS);
					wlc_bmac_mhf(wlc_dngl_ol->wlc_hw, MHF1, MHF1_ULP, MHF1_ULP,
						WLC_BAND_ALL);
					/* put the ucode into sleep */
					wlc_bmac_set_wake_ctrl(wlc_dngl_ol->wlc_hw, FALSE);
					wl_add_timer(wlc->wl, wowl_ol->wowl_test_timer,
						wlc_dngl_ol->wowl_cfg.wowl_test * 1000,	FALSE);
				}
			} else {
				WL_ERROR(("%s: Invalid BCM_OL_WOWL_ENABLE_COMPLETE state = %08x\n",
					__FUNCTION__, wowl_ol->wowl_ol_state));
			}

			gwowl_ol = wowl_ol;

			/* Set halt handler */
			hnd_set_fwhalt(wlc_wowl_ol_fw_halt, wlc_dngl_ol);

			/*
			 * Announce the BCM_OL_E_WOWL_COMPLETE event in case someone
			 * is intertested
			 */
			wlc_dngl_ol_event(wlc_dngl_ol, BCM_OL_E_WOWL_COMPLETE, NULL);

			break;
		default:
			WL_ERROR(("%s: INVALID message type:%d\n",
				__FUNCTION__, msg_hdr->type));
	}
}

/* Common Routine to copy the structure and packet data */
void wlc_wowl_ol_wake_host(
				wlc_dngl_ol_wowl_info_t *wowl_ol,
				void *wowlol_struct, uint32 len,
				uchar *pkt, uint32 pkt_len,
				uint32 wake_reason)
{
	wowl_host_info_t volatile *wowl_host_info;

	wlc_dngl_ol_mic_fail_info_to_host(wowl_ol->wlc_dngl_ol);

	/* Shared info for all the wake packets */
	wowl_host_info = RXOEWOWLINFO(wowl_ol->wlc_dngl_ol);
	if (pkt && pkt_len && (pkt_len <= ETHER_MAX_DATA)) {
		wowl_host_info->pkt_len = pkt_len;
		bcopy(pkt, (void *)wowl_host_info->pkt, pkt_len);
	}

	if (wowlol_struct && len)
		bcopy(wowlol_struct, (void *)&wowl_host_info->u, len);

	wlc_wowl_ol_assert_pme(wowl_ol, wake_reason);
}

/* Asserts PME to wake up host */
static void
wlc_wowl_ol_assert_pme(wlc_dngl_ol_wowl_info_t *wowl_ol, uint32 wake_reason)
{
	wlc_dngl_ol_info_t	*wlc_dngl_ol;
	wlc_info_t		*wlc;
	d11regs_t		*d11_regs;
	osl_t			*osh;
	wowl_host_info_t volatile *wowl_host_info;
	wowl_cfg_t		*wowl_cfg;

	ENTER();

	wlc_dngl_ol	= wowl_ol->wlc_dngl_ol;
	wlc		= wlc_dngl_ol->wlc;
	wowl_cfg	= &wlc_dngl_ol->wowl_cfg;
	d11_regs	= (d11regs_t *)wlc_dngl_ol->regs;
	osh		= wlc_dngl_ol->osh;

	if (wowl_ol->wowl_ol_state == wlc_wowl_ol_state_disabled) {
		/*
		 * The host has not enabled WoWL so don't do anything
		 */
		WL_ERROR(("%s: wowl not enabled\n", __FUNCTION__));
		return;
	} else
	if (wowl_ol->wowl_ol_state == wlc_wowl_ol_state_starting) {
		/*
		 * The host has not completed the WoWL enable phase,
		 * so we'll defer PME asseertion for a while to allow
		 * the host put the device into D3.
		 */
		WL_ERROR(("%s: wowl not completed enable phase\n", __FUNCTION__));

		wowl_ol->wowl_deferral_reason = wlc_wowl_defer_pme_assertion;
		wowl_ol->deferred_wake_reason = wake_reason;

		wl_add_timer(
		    wlc->wl,
		    wowl_ol->wowl_deferral_timer,
		    WOWL_PME_DEFERRAL_TIME,
		    FALSE);

		return;
	}

	if (!(wowl_cfg->wowl_flags & wake_reason)) {
		WL_ERROR(("%s: no matching host wowl flags enabled, wake_reason == 0x%x\n",
			__FUNCTION__, wake_reason));
		return;
	}

	if (wowl_ol->wowl_pme_asserted == TRUE) {
		WL_ERROR(("%s: PME already asserted\n", __FUNCTION__));

		return;
	}

	wowl_ol->wowl_pme_asserted = TRUE;

	/* Prepare the wowl host info */
	wowl_host_info = RXOEWOWLINFO(wlc_dngl_ol);
	wowl_host_info->wake_reason |= wake_reason;

	/* Announce the BCM_OL_E_PME_ASSERTED asserted event in case someone is interested */
	wlc_dngl_ol_event(wlc_dngl_ol, BCM_OL_E_PME_ASSERTED, NULL);

	/* Assert PME through the PSM MAC int status register */
	OR_REG(osh, &d11_regs->psm_macintmask_l, MI_PME);
	OR_REG(osh, &d11_regs->psm_macintstatus_l, MI_PME);
	OSL_DELAY(5);

	/* disable all interrupts to the ARM and suspend mac */
	wlc_intrsoff(wlc);
	wlc_bmac_suspend_mac_and_wait(wlc_dngl_ol->wlc_hw);

	WL_ERROR(("%s: PME asserted with reason 0x%08x\n", __FUNCTION__, wake_reason));

	EXIT();
}


/* ************** PRIVATE FUNCTIONS ************************************* */
/* Sends up an ACK to the host about the status of enabling wowl mode */
static void wlc_wowl_ol_enable_completed(wlc_dngl_ol_wowl_info_t *wowl_ol)
{
	wlc_dngl_ol_info_t		*wlc_dngl_ol;
	olmsg_wowl_enable_completed	*wowl_enable_completed_msg;
	void				*p;

	ENTER();

	wlc_dngl_ol = wowl_ol->wlc_dngl_ol;

	/* Get a packet to send to the host */
	p = pktpool_get(wlc_dngl_ol->shared_msgpool);

	if (p == NULL) {
		WL_ERROR(("Failed to get packet\n"));
		ASSERT(0);
	}

	/* Prepare the payload of the done msg to be sent to the host */
	PKTSETLEN(wlc_dngl_ol->osh, p, sizeof(olmsg_wowl_enable_completed));

	wowl_enable_completed_msg = (olmsg_wowl_enable_completed *)PKTDATA(wlc_dngl_ol->osh, p);
	bzero(wowl_enable_completed_msg, sizeof(olmsg_wowl_enable_completed));

	/* Send up the wowl info to the host */
	wowl_enable_completed_msg->hdr.type = BCM_OL_WOWL_ENABLE_COMPLETED;
	wowl_enable_completed_msg->hdr.seq = 0;
	wowl_enable_completed_msg->hdr.len =
	    sizeof(olmsg_wowl_enable_completed) - sizeof(olmsg_header);

	wl_msgup(wlc_dngl_ol->wlc->wl, wlc_dngl_ol->osh, p);

	EXIT();
}

/* wowl timer callback used for test and timer wake events */
static void wlc_wowl_ol_test_timer_cb(void *arg)
{
	wlc_dngl_ol_wowl_info_t *wowl_ol = arg;
	wowl_cfg_t		*wowl_cfg;

	ENTER();

	wowl_cfg = &wowl_ol->wlc_dngl_ol->wowl_cfg;

	if (wowl_ol->wowl_ol_state == wlc_wowl_ol_state_disabled) {
		WL_ERROR(("%s: wowl not enabled\n", __FUNCTION__));
		return;
	}

	if ((wowl_cfg->wowl_flags == 0) ||
	    (!(wowl_cfg->wowl_flags & WL_WOWL_TST))) {
		WL_ERROR(("%s: WL_WOWL_TST not enabled\n", __FUNCTION__));
		return;
	}
	/* wake ucode and clear MHF1_ULP and HPS bit */
	wlc_bmac_set_wake_ctrl(wowl_ol->wlc_dngl_ol->wlc_hw, TRUE);
	wlc_bmac_mhf(wowl_ol->wlc_dngl_ol->wlc_hw, MHF1, MHF1_ULP, 0, WLC_BAND_ALL);
	wlc_bmac_mctrl(wowl_ol->wlc_dngl_ol->wlc_hw, MCTL_HPS, 0);

	/* wake the host up */
	wlc_wowl_ol_wake_host(wowl_ol, NULL, 0, NULL, 0, WL_WOWL_TST);

	EXIT();
}

/* wowl timer callback used for deferred WoWL events */
static void wlc_wowl_ol_deferral_timer_cb(void *arg)
{
	wlc_dngl_ol_wowl_info_t *wowl_ol = arg;

	ENTER();

	if (wowl_ol->wowl_ol_state != wlc_wowl_ol_state_enabled) {
		WL_ERROR(("%s: wowl not enabled\n", __FUNCTION__));

		return;
	}

	WL_ERROR(("%s: deferral reason 0x%08x\n",
	    __FUNCTION__, wowl_ol->wowl_deferral_reason));

	switch (wowl_ol->wowl_deferral_reason) {
	    case wlc_wowl_defer_pme_assertion:
		wlc_wowl_ol_wake_host(wowl_ol, NULL, 0, NULL, 0, wowl_ol->deferred_wake_reason);
		break;

	    default:
		WL_ERROR(("%s: unknown deferral reason 0x%08x\n",
		    __FUNCTION__, wowl_ol->wowl_deferral_reason));

		ASSERT(0);
	}

	EXIT();
}
