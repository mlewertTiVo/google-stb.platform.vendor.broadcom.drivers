/*
 * Common (OS-independent) portion of
 * Broadcom 802.11 Networking Device Driver
 *
 * VHT support
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
 * $Id: wlc_vht.h 638403 2016-05-17 19:01:20Z $
 */


#ifndef _wlc_vht_h_
#define _wlc_vht_h_

#ifdef WL11AC

#include "osl.h"
#include "typedefs.h"
#include "bcmwifi_channels.h"
#include "proto/802.11.h"
#include "wlc_types.h"
#include "proto/monitor.h"

/* Macro Definitions */
#define VHT_AP_DOWNGRADE_BACKOFF	50
#define WLC_VHT_GET_MU_BFE_CAP(wlc)	(VHT_ENAB((wlc)->pub) && \
		(wlc_vht_get_cap_info((wlc)->vhti) & VHT_CAP_INFO_MU_BEAMFMEE))

/* module entries */
extern wlc_vht_info_t *wlc_vht_attach(wlc_info_t *wlc);
extern void wlc_vht_detach(wlc_vht_info_t *vhti);
extern void wlc_vht_init_defaults(wlc_vht_info_t *vhti);

/* Update tx and rx mcs maps */
extern void wlc_vht_update_mcs_cap_ext(wlc_vht_info_t *vhti);
extern void wlc_vht_update_mcs_cap(wlc_vht_info_t *vhti);

/* IE mgmt */
extern vht_cap_ie_t * wlc_read_vht_cap_ie(wlc_vht_info_t *vhti, uint8 *tlvs, int tlvs_len,
	vht_cap_ie_t* cap_ie);
extern vht_op_ie_t * wlc_read_vht_op_ie(wlc_vht_info_t *vhti, uint8 *tlvs, int tlvs_len,
	vht_op_ie_t* op_ie);
extern uint8 *wlc_read_vht_features_ie(wlc_vht_info_t *vhti,  uint8 *tlvs,
	int tlvs_len, uint8 *rate_mask, int *prop_tlv_len, wlc_bss_info_t *bi);

/* return valid vht chanspec given the op_ie and ht_chanspec */
extern chanspec_t wlc_vht_chanspec(wlc_vht_info_t *vhti, vht_op_ie_t *op_ie,
	chanspec_t ht_chanspec, bool oper_mode_enab, uint8 oper_mode, wlc_bsscfg_t *cfg);
extern void wlc_vht_upd_rate_mcsmap(wlc_vht_info_t *vhti, struct scb *scb);
extern void wlc_vht_update_sgi_rx(wlc_vht_info_t *vhti, uint int_val);
extern void wlc_vht_update_ampdu_cap(wlc_vht_info_t *vhti, uint8 vht_rx_factor);

/* get/set current cap info this node uses */
extern void wlc_vht_set_ldpc_cap(wlc_vht_info_t *vhti, bool enab);

extern void wlc_vht_set_tx_stbc_cap(wlc_vht_info_t *vhti, bool enab);

extern void wlc_vht_set_rx_stbc_cap(wlc_vht_info_t *vhti, int val);

extern uint32 wlc_vht_get_cap_info(wlc_vht_info_t *vhti);

/* per SCB functions */
extern void wlc_vht_bcn_scb_upd(wlc_vht_info_t *vhti, int band, struct scb *scb,
	ht_cap_ie_t *ht_cap, vht_cap_ie_t *vht_cap, vht_op_ie_t *vht_op,
	uint8 vht_ratemask);
extern void wlc_vht_update_scb_state(wlc_vht_info_t *vhti, int band, struct scb *scb,
	ht_cap_ie_t *cap_ie, vht_cap_ie_t *vht_cap_ie, vht_op_ie_t *vht_op_ie,
	uint8 vht_ratemask);

/* perform per scb oper mode changes */
extern void wlc_vht_update_scb_oper_mode(wlc_vht_info_t *vhti, struct scb *scb,
	uint8 mode);

/* get the ratemask corresponding to the scb */
extern uint8 wlc_vht_get_scb_ratemask(wlc_vht_info_t *vhti, struct scb *scb);

/* get the vhtflags corresponding to the scb */
extern uint16 wlc_vht_get_scb_flags(wlc_vht_info_t *vhti, struct scb *scb);

/* get oper mode */
extern bool wlc_vht_get_scb_opermode_enab(wlc_vht_info_t *vhti, struct scb *scb);
extern uint8 wlc_vht_get_scb_opermode(wlc_vht_info_t *vhti, struct scb *scb);
extern uint8 wlc_vht_get_scb_ratemask_per_band(wlc_vht_info_t *vhti, struct scb *scb);

/* agg parameters */
extern uint16 wlc_vht_get_scb_amsdu_mtu_pref(wlc_vht_info_t *vhti, struct scb *scb);
extern uint8 wlc_vht_get_scb_ampdu_max_exp(wlc_vht_info_t *vhti, struct scb *scb);
extern uint8 wlc_vht_get_gid(uint8 *plcp);
extern void wlc_vht_upd_txbf_cap(wlc_vht_info_t *vhti, uint8 bfr, uint8 bfe, uint32 *cap);
extern void wlc_vht_upd_txbf_virtif_cap(wlc_vht_info_t *vhti, uint8 bfr, uint8 bfe, uint32 *cap);
extern bool wlc_vht_prep_rate_info(wlc_vht_info_t *vhti, wlc_d11rxhdr_t *wrxh,
	uint8 *plcp, ratespec_t rspec, struct wl_rxsts *sts);
/* ht/vht operating mode (11ac) */
extern void wlc_frameaction_vht(wlc_vht_info_t *vhti, uint action_id, struct scb *scb,
struct dot11_management_header *hdr, uint8 *body, int body_len);

#ifdef WLTXMONITOR
extern void wlc_vht_txmon_htflags(uint16 phytxctl, uint16 phytxctl1, uint8 *plcp,
	uint16 chan_bw, uint16 chan_band, uint16 chan_num, ratespec_t *rspec,
	struct wl_txsts *sts);

extern void
wlc_vht_txmon_chspec(uint16 phytxctl, uint16 phytxctl1,
	uint16 chan_band, uint16 chan_num,
	struct wl_txsts *sts, uint16 *chan_bw);
#endif /* WLTXMONITOR */

extern void wlc_send_action_vht_oper_mode(wlc_vht_info_t *vhti, wlc_bsscfg_t *bsscfg,
       const struct ether_addr *ea,  bool fromSMPSLearning);
#ifdef WL_STF_ARBITRATOR
extern void wlc_vht_set_ac_mode(wlc_vht_info_t *vhti);
#endif /* WL_STF_ARBITRATOR */

extern bool wlc_vht_is_40MHz(wlc_info_t* wlc, void* p, struct scb* scb);
extern bool wlc_vht_is_SGI(wlc_info_t* wlc, void* p, struct scb* scb);
extern uint8 wlc_vht_get_mcs(wlc_info_t* wlc, void* p, struct scb* scb);


#else
/* empty macros to avoid having to use WL11AC flags everywhere */
#define WLC_VHT_GET_MU_BFE_CAP(wlc)	0
#define wlc_vht_get_scb_amsdu_mtu_pref(x, y) 0
#define wlc_vht_update_ampdu_cap(x, y)
#define wlc_vht_update_sgi_rx(x, y)
#define wlc_frameaction_vht(a, b, c, d, e, f)
#define wlc_vht_bcn_scb_upd(a, b, c, d, e, f, g)
#define wlc_vht_chanspec(a, b, c, d, e, f)
#define wlc_vht_set_ldpc_cap(x, y)
#define wlc_vht_init_defaults(x)
#define wlc_vht_set_rx_stbc_cap(x, y)
#define wlc_vht_update_mcs_cap_ext(x)
#define wlc_vht_update_mcs_cap(x)
#define wlc_vht_get_cap_info(x) 0
#define wlc_vht_update_mcs_cap(x)
#define wlc_read_vht_cap_ie(a, b, c, d) (NULL)
#define wlc_read_vht_cap_ie(a, b, c, d) (NULL)
#define wlc_read_vht_features_ie(a, b, c, d, e, f) (NULL)
#define wlc_vht_prep_rate_info(a, b, c, d, e) (FALSE)
#endif /* WL11AC */

/* TODO: mvoe these flags back to scb to speed up access */
#ifdef WL11AC
/* scb vht flags */
#define SCB_VHT_LDPCCAP		0x0001
#define SCB_SGI80		0x0002
#define SCB_SGI160		0x0004
#define SCB_VHT_TX_STBCCAP	0x0008
#define SCB_VHT_RX_STBCCAP	0x0010
#define SCB_SU_BEAMFORMER	0x0020
#define SCB_SU_BEAMFORMEE	0x0040
#define SCB_MU_BEAMFORMER	0x0080
#define SCB_MU_BEAMFORMEE	0x0100
#define SCB_VHT_TXOP_PS		0x0200
#define SCB_HTC_VHT_CAP		0x0400
#define SCB_VHT_LDPC_CAP(v, a)	(SCB_VHT_CAP(a) && \
	(wlc_vht_get_scb_flags(v, a) & SCB_VHT_LDPCCAP))
#define SCB_VHT_TX_STBC_CAP(v, a)	(SCB_VHT_CAP(a) && \
	(wlc_vht_get_scb_flags(v, a) & SCB_VHT_TX_STBCCAP))
#define SCB_VHT_RX_STBC_CAP(v, a)	(SCB_VHT_CAP(a) && \
	(wlc_vht_get_scb_flags(v, a) & SCB_VHT_RX_STBCCAP))
#define SCB_VHT_SGI80(v, a)	(SCB_VHT_CAP(a) && \
	(wlc_vht_get_scb_flags(v, a) & SCB_SGI80))
#define SCB_VHT_SGI160(v, a)	(SCB_VHT_CAP(a) && \
		(wlc_vht_get_scb_flags(v, a) & SCB_SGI160))
#else /* WL11AC */
#define SCB_VHT_LDPC_CAP(v, a)		FALSE
#define SCB_VHT_TX_STBC_CAP(v, a)	FALSE
#define SCB_VHT_RX_STBC_CAP(v, a)	FALSE
#define SCB_VHT_SGI80(v, a)		FALSE
#define SCB_VHT_SGI160(v, a)		FALSE
#endif /* WL11AC */

#endif /* _wlc_vht_h_ */
