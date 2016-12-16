/**
 * @file
 * Miscellaneous IE handlers
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
 * $Id: wlc_ie_misc_hndlrs.c 656767 2016-08-30 01:02:39Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmwifi_channels.h>
#include <siutils.h>
#include <bcmendian.h>
#include <proto/802.1d.h>
#include <proto/802.11.h>
#include <proto/802.11e.h>
#include <proto/bcmip.h>
#include <proto/wpa.h>
#include <proto/vlan.h>
#include <hndsoc.h>
#include <sbchipc.h>
#include <pcicfg.h>
#include <bcmsrom.h>
#include <wlioctl.h>
#include <epivers.h>
#if defined(BCMSUP_PSK) || defined(STA) || defined(LINUX_CRYPTO)
#include <proto/eapol.h>
#endif
#include <bcmwpa.h>
#include <sbhndpio.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <hndpmu.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_cca.h>
#include <wlc_interfere.h>
#include <wlc_bsscfg.h>
#include <wlc_vndr_ie_list.h>
#include <wlc_channel.h>
#include <wlc.h>
#include <wlc_hw.h>
#include <wlc_hw_priv.h>
#include <wlc_bmac.h>
#include <wlc_apps.h>
#include <wlc_scb.h>
#include <wlc_phy_hal.h>
#include <wlc_antsel.h>
#include <wlc_led.h>
#include <wlc_frmutil.h>
#include <wlc_stf.h>
#ifdef WLMCNX
#include <wlc_mcnx.h>
#include <wlc_tbtt.h>
#endif
#ifdef WLP2P
#include <wlc_p2p.h>
#endif
#ifdef WLMCHAN
#include <wlc_mchan.h>
#endif
#include <wlc_scb_ratesel.h>
#ifdef WL_LPC
#include <wlc_scb_powersel.h>
#endif /* WL_LPC */
#include <wlc_event.h>
#include <wlc_seq_cmds.h>
#include <wl_export.h>
#include "d11ucode.h"
#if defined(BCMSUP_PSK)
#include <wlc_sup.h>
#endif
#include <wlc_pmkid.h>
#if defined(BCMAUTH_PSK)
#include <wlc_auth.h>
#endif
#ifdef WET
#include <wlc_wet.h>
#endif
#ifdef PSTA
#include <wlc_psta.h>
#endif /* PSTA */
#ifdef BCMCCMP
#include <bcmcrypto/aes.h>
#endif
#include <wlc_rm.h>
#include "wlc_cac.h"
#include <wlc_ap.h>
#ifdef AP
#include <wlc_apcs.h>
#endif
#include <wlc_scan.h>
#include <wlc_extlog.h>
#include <wlc_assoc.h>
#ifdef STA
#include <wlc_wpa.h>
#endif /* STA */
#include <wlc_lq.h>
#include <wlc_11h.h>
#include <wlc_tpc.h>
#include <wlc_csa.h>
#include <wlc_quiet.h>
#include <wlc_dfs.h>
#include <wlc_11d.h>
#include <wlc_cntry.h>
#include <bcm_mpool_pub.h>
#include <wlc_utils.h>
#include <wlc_hrt.h>
#include <wlc_prot.h>
#include <wlc_prot_g.h>
#define _inc_wlc_prot_n_preamble_	/* include static INLINE uint8 wlc_prot_n_preamble() */
#include <wlc_prot_n.h>
#include <wlc_probresp.h>
#ifdef WL11AC
#include <wlc_vht.h>
#endif
#include <wlc_pcb.h>
#include <wlc_txc.h>
#ifdef MFP
#include <wlc_mfp.h>
#endif
#include <wlc_macfltr.h>
#include <wlc_addrmatch.h>
#ifdef WL_RELMCAST
#include "wlc_relmcast.h"
#endif
#include <wlc_btcx.h>
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_ie_mgmt_vs.h>
#include <wlc_ie_reg.h>
#include <wlc_ie_helper.h>
#include <wlc_akm_ie.h>
#include <wlc_ht.h>
#ifdef ANQPO
#include <wl_anqpo.h>
#endif
#include <wlc_hs20.h>
#ifdef STA
#include <wlc_pm.h>
#ifdef WLWNM
#include <wlc_wnm.h>
#endif /* WLWNM */
#endif /* STA */
#ifdef WLFBT
#include <wlc_fbt.h>
#endif
#ifdef WLOLPC
#include <wlc_olpc_engine.h>
#endif /* OPEN LOOP POWER CAL */
#ifdef WL_STAPRIO
#include <wlc_staprio.h>
#endif /* WL_STAPRIO */
#include <wlc_monitor.h>
#include <wlc_stamon.h>
#include <wlc_ie_misc_hndlrs.h>
#include <wlc_ht.h>
#ifdef WLTDLS
#include <wlc_tdls.h>
#endif
#include <wlc_bsscfg_viel.h>
#ifdef WL_PWRSTATS
#include <wlc_pwrstats.h>
#endif /* WL_PWRSTATS */

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 * TODO: Move these functions to their own modules when possible.
 * Leave them here for now.
 * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 */
/* Extended Capabilities IE */

/* encodes the ext cap ie based on intersection of target AP and self ext cap
 * output buffer size must be DOT11_EXTCAP_LEN_MAX, encoded length is returned
 */
static int
encode_ext_cap_ie(wlc_bsscfg_t *cfg, uint8 *buffer)
{
	uint32 result_len = 0;
	wlc_bss_info_t *bi = cfg->target_bss;
	uint bcn_parse_len = bi->bcn_prb_len - sizeof(struct dot11_bcn_prb);
	uint8 *bcn_parse = (uint8*)bi->bcn_prb + sizeof(struct dot11_bcn_prb);
	bcm_tlv_t *bcn_ext_cap;

	bcn_ext_cap = bcm_parse_tlvs(bcn_parse, bcn_parse_len, DOT11_MNG_EXT_CAP_ID);

	if (bcn_ext_cap && cfg->ext_cap_len) {
		uint32 i;
		uint32 ext_cap_len = MIN(bcn_ext_cap->len, cfg->ext_cap_len);

		for (i = 0; i < ext_cap_len; i++) {
			/* Take the intersection of AP and self ext cap */
			buffer[i] = bcn_ext_cap->data[i] & cfg->ext_cap[i];
			/* is intersection non-zero */
			if (buffer[i])
				result_len = i + 1;
		}
	}

	return result_len;
}

static uint
wlc_bss_calc_conditional_ext_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	uint8 buffer[DOT11_EXTCAP_LEN_MAX];
	int len;

	BCM_REFERENCE(ctx);

	if ((len = encode_ext_cap_ie(data->cfg,	buffer)) > 0) {
		len += TLV_HDR_LEN;
	}

	return len;
}

static int
wlc_bss_write_conditional_ext_cap_ie(void *ctx, wlc_iem_build_data_t *data)
{
	uint8 buffer[DOT11_EXTCAP_LEN_MAX];
	int len;

	BCM_REFERENCE(ctx);

	len = encode_ext_cap_ie(data->cfg, buffer);

	/* add extended capabilities */
	if (len) {
		if (bcm_write_tlv_safe(DOT11_MNG_EXT_CAP_ID, buffer,
			len, data->buf, data->buf_len) == data->buf) {
			return BCME_BUFTOOSHORT;
		}
	}

	return BCME_OK;
}

static uint
wlc_bss_calc_ext_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_bsscfg_t *cfg = data->cfg;

	if (cfg->ext_cap_len == 0) {
		return 0;
	}

	if (data->ft == FC_ASSOC_REQ ||
		data->ft == FC_REASSOC_REQ ||
		data->ft == FC_AUTH) {
		/* ext cap IE added only if target AP supports it */
		return wlc_bss_calc_conditional_ext_cap_ie_len(ctx, data);
	}

	return TLV_HDR_LEN + cfg->ext_cap_len;
}

static int
wlc_bss_write_ext_cap_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_bsscfg_t *cfg = data->cfg;

	if (cfg->ext_cap_len == 0) {
		return BCME_OK;
	}

	if (data->ft == FC_ASSOC_REQ ||
		data->ft == FC_REASSOC_REQ ||
		data->ft == FC_AUTH) {
		/* ext cap IE added only if target AP supports it */
		return wlc_bss_write_conditional_ext_cap_ie(ctx, data);
	}

	if (bcm_write_tlv_safe(DOT11_MNG_EXT_CAP_ID, cfg->ext_cap,
		cfg->ext_cap_len, data->buf, data->buf_len) == data->buf) {
		return BCME_BUFTOOSHORT;
	}

	return BCME_OK;
}

static int
wlc_bss_parse_interworking_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	wlc_bsscfg_t *cfg = data->cfg;
	wlc_bss_info_t *bi = NULL;
	bcm_tlv_t *iw = (bcm_tlv_t *)data->ie;

	if (!BSSCFG_INFRA_STA(cfg) || !cfg->associated)
		return BCME_OK;

	if ((cfg->assoc->state != AS_WAIT_RCV_BCN) && (cfg->assoc->state != AS_IDLE))
		return BCME_OK;

	bi = cfg->current_bss;

	if (iw && iw->len > 0) {
		bi->bcnflags |= WLC_BSS_INTERWORK_PRESENT;
		bi->accessnet = iw->data[0];
	} else {
		bi->bcnflags &= ~WLC_BSS_INTERWORK_PRESENT;
	}

	return BCME_OK;
}

#ifdef WLP2P
static bool
wlc_vndr_non_p2p_ie_filter(void *arg, const vndr_ie_t *ie)
{
	uint8 *parse;
	uint parse_len;
	BCM_REFERENCE(arg);

	ASSERT(ie != NULL);

	if (ie->id != DOT11_MNG_VS_ID)
		return FALSE;
/* unfortunately no way of disposing of const cast  - the
 * function is a callback so must have particular signature
 * and making bcm_is_p2p_ie causes cascade bringing need for
 * const cast in other places
 */
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wcast-qual\"")
#endif
	parse = (uint8 *)ie;
	parse_len = TLV_HDR_LEN + ie->len;

	return !bcm_is_p2p_ie((uint8 *)ie, &parse, &parse_len);
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic pop")
#endif

}

static int
wlc_vndr_non_p2p_ie_getlen(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	uint32 pktflag, int *totie)
{
	return wlc_vndr_ie_getlen_ext(wlc->vieli, cfg, wlc_vndr_non_p2p_ie_filter, pktflag, totie);
}

static bool
wlc_vndr_non_p2p_ie_write_filter(void *arg, uint type, const vndr_ie_t *ie)
{
	BCM_REFERENCE(type);
	return wlc_vndr_non_p2p_ie_filter(arg, ie);
}

static uint8 *
wlc_vndr_non_p2p_ie_write(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	uint type, uint32 pktflag, uint8 *cp, int buflen)
{
	return wlc_vndr_ie_write_ext(wlc->vieli, cfg, wlc_vndr_non_p2p_ie_write_filter, type,
	                             cp, buflen, pktflag);
}
#endif /* WLP2P */

/* Common: Vendor added IEs */
static uint
wlc_bss_calc_vndr_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_bsscfg_t *cfg = data->cfg;
	uint16 type = data->ft;
	uint32 flag;

	if (type == FC_AUTH) {
		wlc_iem_ft_cbparm_t *ftcbparm;

		ASSERT(data->cbparm != NULL);
		ftcbparm = data->cbparm->ft;
		ASSERT(ftcbparm != NULL);

		flag = wlc_auth2vieflag(ftcbparm->auth.seq);
	}
	else
		flag = wlc_ft2vieflag(type);

#ifdef WLP2P
	/* leave all p2p IEs up to p2p IE handling */
	if (type == FC_ASSOC_RESP || type == FC_REASSOC_RESP) {
		wlc_iem_ft_cbparm_t *ftcbparm;

		ASSERT(data->cbparm != NULL);
		ftcbparm = data->cbparm->ft;
		ASSERT(ftcbparm != NULL);

		if (SCB_P2P(ftcbparm->assocresp.scb))
			return wlc_vndr_non_p2p_ie_getlen(wlc, cfg, flag, NULL);
	}
	else if (BSS_P2P_ENAB(wlc, cfg)) {
		return wlc_vndr_non_p2p_ie_getlen(wlc, cfg, flag, NULL);
	}
#endif /* WLP2P */

	if (flag != 0)
		return wlc_vndr_ie_getlen(wlc->vieli, cfg, flag, NULL);

	return 0;
}

static int
wlc_bss_write_vndr_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_bsscfg_t *cfg = data->cfg;
	uint16 type = data->ft;
	uint32 flag;

	if (type == FC_AUTH) {
		wlc_iem_ft_cbparm_t *ftcbparm;

		ASSERT(data->cbparm != NULL);
		ftcbparm = data->cbparm->ft;
		ASSERT(ftcbparm != NULL);

		flag = wlc_auth2vieflag(ftcbparm->auth.seq);
	}
	else
		flag = wlc_ft2vieflag(type);

#ifdef WLP2P
	/* leave all p2p IEs up to p2p IE handling */
	if (type == FC_ASSOC_RESP || type == FC_REASSOC_RESP) {
		wlc_iem_ft_cbparm_t *ftcbparm;

		ASSERT(data->cbparm != NULL);
		ftcbparm = data->cbparm->ft;
		ASSERT(ftcbparm != NULL);

		if (SCB_P2P(ftcbparm->assocresp.scb)) {
			wlc_vndr_non_p2p_ie_write(wlc, cfg, type, flag, data->buf, data->buf_len);
			return BCME_OK;
		}
	}
	else if (BSS_P2P_ENAB(wlc, cfg)) {
		wlc_vndr_non_p2p_ie_write(wlc, cfg, type, flag, data->buf, data->buf_len);
		return BCME_OK;
	}
#endif /* WLP2P */

	if (flag != 0) {
		wlc_vndr_ie_write(wlc->vieli, cfg, data->buf, data->buf_len, flag);
	}

	return BCME_OK;
}

/* Common: BRCM IE */
static uint
wlc_bss_calc_brcm_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_bsscfg_t *cfg = data->cfg;

	if ((wlc->brcm_ie) && (cfg->brcm_ie[TLV_LEN_OFF] > 0))
		return TLV_HDR_LEN + cfg->brcm_ie[TLV_LEN_OFF];

	return 0;
}

static int
wlc_bss_write_brcm_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_bsscfg_t *cfg = data->cfg;

	if ((wlc->brcm_ie) && (cfg->brcm_ie[TLV_LEN_OFF] > 0)) {
		bcm_copy_tlv(cfg->brcm_ie, data->buf);
	}

	return BCME_OK;
}

static int
wlc_bss_parse_brcm_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	struct scb *scb;

	scb = wlc_iem_parse_get_assoc_bcn_scb(data);
	if (scb != NULL) {
		if (data->ie != NULL) {
			wlc_process_brcm_ie(wlc, scb, (brcm_ie_t *)data->ie);
		}
	}

	return BCME_OK;
}

/* register common IE mgmt handlers */
int
BCMATTACHFN(wlc_register_iem_fns)(wlc_info_t *wlc)
{
	wlc_iem_info_t *iemi = wlc->iemi;
	/* Extended Capabilities IE */
	uint16 ext_cap_build_fstbmp =
	        FT2BMP(FC_ASSOC_REQ) |
	        FT2BMP(FC_ASSOC_RESP) |
	        FT2BMP(FC_REASSOC_REQ) |
	        FT2BMP(FC_REASSOC_RESP) |
	        FT2BMP(FC_PROBE_REQ) |
	        FT2BMP(FC_PROBE_RESP) |
	        FT2BMP(FC_BEACON) |
	        0;
	/* Vendor added Vendor Specific IEs */
	uint16 vndr_build_fstbmp =
	        FT2BMP(FC_ASSOC_REQ) |
	        FT2BMP(FC_ASSOC_RESP) |
	        FT2BMP(FC_REASSOC_REQ) |
	        FT2BMP(FC_REASSOC_RESP) |
	        FT2BMP(FC_PROBE_REQ) |
	        FT2BMP(FC_PROBE_RESP) |
	        FT2BMP(FC_BEACON) |
	        FT2BMP(FC_DISASSOC) |
	        FT2BMP(FC_AUTH) |
	        FT2BMP(FC_DEAUTH) |
	        0;
	/* Brcm Vendor Specific IE */
	uint16 brcm_build_fstbmp =
	        FT2BMP(FC_ASSOC_REQ) |
	        FT2BMP(FC_ASSOC_RESP) |
	        FT2BMP(FC_REASSOC_REQ) |
	        FT2BMP(FC_REASSOC_RESP) |
	        FT2BMP(FC_PROBE_REQ) |
	        FT2BMP(FC_PROBE_RESP) |
	        FT2BMP(FC_BEACON) |
	        FT2BMP(FC_AUTH) |
	        0;
	uint16 brcm_parse_fstbmp =
	        FT2BMP(FC_ASSOC_REQ) |
	        FT2BMP(FC_ASSOC_RESP) |
	        FT2BMP(FC_REASSOC_REQ) |
	        FT2BMP(FC_REASSOC_RESP) |
	        FT2BMP(FC_BEACON) |
	        0;

	uint16 interworking_parse_fstbmp =
	        FT2BMP(FC_BEACON) |
	        0;

	int err = BCME_OK;

	/* calc/build */
	if ((err = wlc_iem_add_build_fn_mft(iemi, ext_cap_build_fstbmp, DOT11_MNG_EXT_CAP_ID,
	      wlc_bss_calc_ext_cap_ie_len, wlc_bss_write_ext_cap_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_add_build_fn failed, err %d, ext cap ie\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
	if ((err = wlc_iem_vs_add_build_fn_mft(iemi, vndr_build_fstbmp, WLC_IEM_VS_IE_PRIO_VNDR,
	      wlc_bss_calc_vndr_ie_len, wlc_bss_write_vndr_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_vs_add_build_fn failed, err %d, vndr ie\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
	if ((err = wlc_iem_vs_add_build_fn_mft(iemi, brcm_build_fstbmp, WLC_IEM_VS_IE_PRIO_BRCM,
	      wlc_bss_calc_brcm_ie_len, wlc_bss_write_brcm_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_vs_add_build_fn failed, err %d, brcm ie\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
	/* parse */
	if ((err = wlc_iem_vs_add_parse_fn_mft(iemi, brcm_parse_fstbmp, WLC_IEM_VS_IE_PRIO_BRCM,
	                                       wlc_bss_parse_brcm_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_vs_add_parse_fn failed, err %d, brcm ie in assocreq\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
	if ((err = wlc_iem_add_parse_fn_mft(iemi, interworking_parse_fstbmp,
	                                    DOT11_MNG_INTERWORKING_ID,
	                                    wlc_bss_parse_interworking_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_add_parse_fn failed, err %d, interworking in beacon\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}

	return BCME_OK;
fail:

	return err;
}

/* SSID IE */
static uint
wlc_bcn_calc_ssid_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_bsscfg_t *cfg = data->cfg;

	BCM_REFERENCE(ctx);

	if (data->ft == FC_BEACON && cfg->closednet_nobcnssid) {
	}

	return TLV_HDR_LEN + cfg->SSID_len;
}

static int
wlc_bcn_write_ssid_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_bsscfg_t *cfg = data->cfg;
	uint8 ssid[DOT11_MAX_SSID_LEN];

	BCM_REFERENCE(ctx);

	if ((data->ft == FC_BEACON) && cfg->closednet_nobcnssid) {
	}

	/* in the closed net case the ssid is faked to be all zero */
	if ((data->ft == FC_BEACON) && cfg->closednet_nobcnssid) {
		bzero(ssid, cfg->SSID_len);
	} else {
		bcopy(cfg->SSID, ssid, cfg->SSID_len);
	}

	bcm_write_tlv(DOT11_MNG_SSID_ID, ssid, cfg->SSID_len, data->buf);

	return BCME_OK;
}

/* Supported Rates IE */
static uint
wlc_bcn_calc_sup_rates_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;

	BCM_REFERENCE(ctx);

	return TLV_HDR_LEN + ftcbparm->bcn.sup->count;
}

static int
wlc_bcn_write_sup_rates_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;

	BCM_REFERENCE(ctx);

	bcm_write_tlv(DOT11_MNG_RATES_ID, ftcbparm->bcn.sup->rates,
		ftcbparm->bcn.sup->count, data->buf);

	return BCME_OK;
}

/* Extended Supported Rates IE */
static uint
wlc_bcn_calc_ext_rates_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;

	if (wlc->band->gmode || wlc_ht_get_phy_membership(wlc->hti)) {
		if (ftcbparm->bcn.ext->count == 0)
			return 0;
		return TLV_HDR_LEN + ftcbparm->bcn.ext->count;
	}

	return 0;
}

static int
wlc_bcn_write_ext_rates_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;

	if (wlc->band->gmode || wlc_ht_get_phy_membership(wlc->hti)) {
		if (ftcbparm->bcn.ext->count == 0) {
			return BCME_OK;

		}

		bcm_write_tlv(DOT11_MNG_EXT_RATES_ID, ftcbparm->bcn.ext->rates,
			ftcbparm->bcn.ext->count, data->buf);
	}

	return BCME_OK;
}

/* DS Parameters */
static uint
wlc_bcn_calc_ds_parms_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_bsscfg_t *cfg = data->cfg;
	wlc_bss_info_t *current_bss = cfg->current_bss;

	BCM_REFERENCE(ctx);

	if (!CHSPEC_IS2G(current_bss->chanspec))
		return 0;

	return TLV_HDR_LEN + 1;
}

static int
wlc_bcn_write_ds_parms_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_bsscfg_t *cfg = data->cfg;
	wlc_bss_info_t *current_bss = cfg->current_bss;
	uint8 chan;

	BCM_REFERENCE(ctx);

	if (!CHSPEC_IS2G(current_bss->chanspec))
		return BCME_OK;

	chan = wf_chspec_ctlchan(current_bss->chanspec);

	bcm_write_tlv(DOT11_MNG_DS_PARMS_ID, &chan, 1, data->buf);

	return BCME_OK;
}

#ifdef STA
/* IBSS Params */
static uint
wlc_bcn_calc_ibss_parms_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
#ifdef WLP2P
	wlc_info_t *wlc = (wlc_info_t *)ctx;
#endif
	wlc_bsscfg_t *cfg = data->cfg;

	if (!cfg->BSS &&
#ifdef WLP2P
	    !BSS_P2P_DISC_ENAB(wlc, cfg) &&
#endif
#ifdef WLMESH
		!BSSCFG_MESH(cfg) &&
#endif
	    TRUE) {
		return TLV_HDR_LEN + sizeof(uint16);
	}

	return 0;
}

static int
wlc_bcn_write_ibss_parms_ie(void *ctx, wlc_iem_build_data_t *data)
{
#ifdef WLP2P
	wlc_info_t *wlc = (wlc_info_t *)ctx;
#endif
	wlc_bsscfg_t *cfg = data->cfg;
	wlc_bss_info_t *current_bss = cfg->current_bss;

	if (!cfg->BSS &&
#ifdef WLP2P
	    !BSS_P2P_DISC_ENAB(wlc, cfg) &&
#endif
#ifdef WLMESH
		!BSSCFG_MESH(cfg) &&
#endif
	    TRUE) {
		uint16 atim = htol16(current_bss->atim_window);

		bcm_write_tlv(DOT11_MNG_IBSS_PARMS_ID, &atim, sizeof(uint16), data->buf);
	}

	return BCME_OK;
}
#endif /* STA */

/* register Bcn/PrbRsp IE mgmt handlers */
int
BCMATTACHFN(wlc_bcn_register_iem_fns)(wlc_info_t *wlc)
{
	wlc_iem_info_t *iemi = wlc->iemi;
	int err = BCME_OK;
	uint16 fstbmp = FT2BMP(FC_BEACON) | FT2BMP(FC_PROBE_RESP);

	/* calc/build */
	/* bcn/prbrsp */
	if ((err = wlc_iem_add_build_fn_mft(iemi, fstbmp, DOT11_MNG_SSID_ID,
	      wlc_bcn_calc_ssid_ie_len, wlc_bcn_write_ssid_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_add_build_fn failed, err %d, ssid ie\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
	if ((err = wlc_iem_add_build_fn_mft(iemi, fstbmp, DOT11_MNG_RATES_ID,
	      wlc_bcn_calc_sup_rates_ie_len, wlc_bcn_write_sup_rates_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_add_build_fn failed, err %d, sup rates ie\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
	if ((err = wlc_iem_add_build_fn_mft(iemi, fstbmp, DOT11_MNG_EXT_RATES_ID,
	      wlc_bcn_calc_ext_rates_ie_len, wlc_bcn_write_ext_rates_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_add_build_fn failed, err %d, ext rates ie\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
	if ((err = wlc_iem_add_build_fn_mft(iemi, fstbmp, DOT11_MNG_DS_PARMS_ID,
	      wlc_bcn_calc_ds_parms_ie_len, wlc_bcn_write_ds_parms_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, err %d, ds parms ie\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
#ifdef STA
	if ((err = wlc_iem_add_build_fn_mft(iemi, fstbmp, DOT11_MNG_IBSS_PARMS_ID,
	      wlc_bcn_calc_ibss_parms_ie_len, wlc_bcn_write_ibss_parms_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, err %d, ibss parms ie\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
#endif

	return BCME_OK;

fail:
	return err;
}

/* SSID IE */
static uint
wlc_prq_calc_ssid_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;

	BCM_REFERENCE(ctx);

	return TLV_HDR_LEN + ftcbparm->prbreq.ssid_len;
}

static int
wlc_prq_write_ssid_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;

	BCM_REFERENCE(ctx);

	bcm_write_tlv(DOT11_MNG_SSID_ID, ftcbparm->prbreq.ssid,
		ftcbparm->prbreq.ssid_len, data->buf);

	return BCME_OK;
}

/* Supported Rates IE */
static uint
wlc_prq_calc_sup_rates_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;

	BCM_REFERENCE(ctx);

	return TLV_HDR_LEN + ftcbparm->prbreq.sup->count;
}

static int
wlc_prq_write_sup_rates_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;

	BCM_REFERENCE(ctx);

	bcm_write_tlv(DOT11_MNG_RATES_ID, ftcbparm->prbreq.sup->rates,
		ftcbparm->prbreq.sup->count, data->buf);

	return BCME_OK;
}

/* Extended Supported Rates IE */
static uint
wlc_prq_calc_ext_rates_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;

	BCM_REFERENCE(ctx);

	if (ftcbparm->prbreq.ext->count == 0) {
		return 0;
	}

	return TLV_HDR_LEN + ftcbparm->prbreq.ext->count;
}


static int
wlc_prq_write_ext_rates_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;

	BCM_REFERENCE(ctx);

	bcm_write_tlv(DOT11_MNG_EXT_RATES_ID, ftcbparm->prbreq.ext->rates,
		ftcbparm->prbreq.ext->count, data->buf);

	return BCME_OK;
}

/* DS Parameters */
static uint
wlc_prq_calc_ds_parms_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;

	BCM_REFERENCE(data);

	if (!CHSPEC_IS2G(wlc->chanspec))
		return 0;

	return TLV_HDR_LEN + 1;
}

static int
wlc_prq_write_ds_parms_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	uint8 chan;

	if (!CHSPEC_IS2G(wlc->chanspec))
		return BCME_OK;

	chan = wf_chspec_ctlchan(wlc->chanspec);

	bcm_write_tlv(DOT11_MNG_DS_PARMS_ID, &chan, 1, data->buf);

	return BCME_OK;
}

/* register PrbReq IE mgmt handlers */
int
BCMATTACHFN(wlc_prq_register_iem_fns)(wlc_info_t *wlc)
{
	wlc_iem_info_t *iemi = wlc->iemi;
	int err = BCME_OK;

	/* calc/build */
	/* prbreq */
	if ((err = wlc_iem_add_build_fn(iemi, FC_PROBE_REQ, DOT11_MNG_SSID_ID,
	      wlc_prq_calc_ssid_ie_len, wlc_prq_write_ssid_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_add_build_fn failed, err %d, ssid in prbreq\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
	if ((err = wlc_iem_add_build_fn(iemi, FC_PROBE_REQ, DOT11_MNG_RATES_ID,
	      wlc_prq_calc_sup_rates_ie_len, wlc_prq_write_sup_rates_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_add_build_fn failed, err %d, sup rates in prbreq\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
	if ((err = wlc_iem_add_build_fn(iemi, FC_PROBE_REQ, DOT11_MNG_EXT_RATES_ID,
	      wlc_prq_calc_ext_rates_ie_len, wlc_prq_write_ext_rates_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_add_build_fn failed, err %d, ext rates in prbreq\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}
	if ((err = wlc_iem_add_build_fn(iemi, FC_PROBE_REQ, DOT11_MNG_DS_PARMS_ID,
	      wlc_prq_calc_ds_parms_ie_len, wlc_prq_write_ds_parms_ie, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, err %d, ds parms in prbreq\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}

	return BCME_OK;

fail:
	return err;
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 * TODO: Move above functions to their own modules when possible.
 * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 */
