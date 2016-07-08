
/*   _____________________________________________________________________
 *  |                                                                     |
 *  |  Copyright (C) 2003 Mitsumi electric co,. ltd. All Rights Reserved. |
 *  |  This software contains the valuable trade secrets of Mitsumi.      |
 *  |  The software is protected under copyright laws as an unpublished   |
 *  |  work of Mitsumi. Notice is for informational purposes only and     |
 *  |  does not imply publication. The user of this software may make     |
 *  |  copies of the software for use with parts manufactured by Mitsumi  |
 *  |  or under license from Mitsumi and for no other use.                |
 *  |_____________________________________________________________________|
 */

/*
 * Nintendo library functions
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
 * $Id: wlc_nin.c 467328 2014-04-03 01:23:40Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>

#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <proto/802.11.h>
#include <wlioctl.h>
#include <epivers.h>

#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_channel.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_rate_sel.h>
#include <wlc_scb.h>
#include <wlc_phy_hal.h>
#include <wlc_frmutil.h>
#include <wl_export.h>
#include <sbpcmcia.h>


#include <wlc_pub.h>
#include <wl_dbg.h>

#include <nintendowm.h>
#include <wlc_nitro.h>
#include <proto/nitro.h>
#include <wlc_nin.h>
#include <wlc_ppr.h>

/* local prototypes */
static void wlc_nin_get_wepmodecmd(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *wepmode, uint16 *result);
static int wlc_nin_get_wlstate(wlc_info_t *wlc, struct wlc_if *wlcif);
static void wlc_nin_get_modecmd(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *mode, uint16 *result);
static void
wlc_nin_retrycmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *sharg,
	uint16 *lgarg, uint16 *result, bool get);
static void
wlc_nin_set_wepmodecmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 wepmode,
	uint16 *result);
static void
wlc_nin_wepkeyid(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *wepkeyid, uint16 *result, bool get);
static void
wlc_nin_beaconfrmtype(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *frmtype, uint16 *result, bool get);
static void
wlc_nin_prbresp(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *proberesp, uint16 *result, bool get);
static void
wlc_nin_bcntimeout(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *bcntmout, uint16 *result, bool get);
static void
wlc_nin_activezone(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *actzone,
	uint16 *result, bool get);
static void
wlc_nin_ssidmask(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *ssidmask,
	uint16 *result, bool get);
static void
wlc_nin_preambleCmd(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *preamble, uint16 *result, bool get);
static void
wlc_nin_authalgocmd(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *authtype, uint16 *result, bool get);
static void
wlc_nin_set_diversity(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 diversity,
	uint16 useantenna, uint16 *result);
static void
wlc_nin_bcnind_enable(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *rcven, uint16 *snden, uint16 *result, bool get);
static void
wlc_nin_bcnprd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *bcnint,
	uint16 *result, bool get);
static void
wlc_nin_dtimprd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *dtim,
	uint16 *result, bool get);
static void
wlc_nin_set_wepkey(wlc_info_t *wlc, struct wlc_if *wlcif, uint8 *keydata,
	int index, uint16 *result);
static void
wlc_nin_macaddr(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *macaddr,
	uint16 *result, bool get);
static void
wlc_nin_getcountryinfo(wlc_info_t * wlc, struct wlc_if * wlcif,
	char * country, uint16 *chvector, uint16 *channelinuse, uint16 *result);
static void
wlc_nin_set_modecmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint32 mode, uint16 *result);
static void
wlc_nin_set_pmk(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *keydat,
	uint16 keylen, uint16 *result);
static void
wlc_nin_create_errind(wlc_info_t * wlc, char *pdata, int len,
	struct wlc_if *wlcif, uint16 type);
static void
wlc_nin_get_wlcounters(wlc_info_t *wlc, struct wlc_if *wlcif, nwm_wl_cnt_t *pwlcnt,
	wlc_nitro_cnt_t *pnitro, uint16 *result);
static void
wlc_nin_clr_wlcounters(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *result);
static bool wlc_mlme_reset_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
cmd_reserved_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_mlme_scan_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_mlme_join_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
nwm_mlme_deauth_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_mlme_start_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_mlme_measchan_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_ma_data_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_ma_keydata_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_ma_mp_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_ma_testdata_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_ma_clrdata_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_all_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_macadrs_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_retry_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_enablechannel_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_mode_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_rate_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_wepmode_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_wepkeyid_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_wepkey_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_beacontype_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_resbcssid_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_beaconlostth_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_activezone_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_ssidmask_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_preambletype_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_authalgo_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_lifetime_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_maxconn_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_txant_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_diversity_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_bcnsendrecvind_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_interference_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_bssid_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_beaconperiod_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_dtimperiod_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_gameinfo_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_vblanktsf_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_maclist_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_rtsthresh_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_fragthresh_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_pmk_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramset_eerom_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_all_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_macadrs_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_retry_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_mode_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_rate_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_wepmode_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_wepkeyid_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_beacontype_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_resbcssid_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_beaconlostth_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_activezone_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_ssidmask_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_preambletype_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_authalgo_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_maxconn_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_txant_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_diversity_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_bcnsendrecvind_cmd(void * pkt,
	nwm_cmd_req_t * pReq, nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc,
	struct wlc_if * wlcif);
static bool
wlc_paramget_interference_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_bssid_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_ssid_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_beaconperiod_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_dtimperiod_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_gameinfo_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_vblanktsf_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_maclist_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_rtsthresh_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_fragthresh_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_eerom_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_rssi_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_dev_idle_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_dev_class1_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_dev_reboot_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_dev_clearwlinfo_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_dev_getverinfo_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_dev_getwlinfo_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_dev_getstate_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_dev_testsignal_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_enablechannel_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);

static bool
wlc_paramset_txpwr_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_txpwr_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);

static bool wlc_paramset_mrate_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);
static bool
wlc_paramget_mrate_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif);

/* data structures for this file only */
typedef	struct wlc_nin_cmdtbl
{
	uint16		RequestMinLength;
	uint16		ConfirmMinLength;
	bool		(*pCmdFunc)(void * pkt, nwm_cmd_req_t* pReq,
		nwm_cmd_cfm_t* pCfm, wlc_info_t * wlc, struct wlc_if *wlcif);
} wlc_nin_cmdtbl_t;

struct wlc_nin {
	wlc_info_t		*wlc;			/* back pointer */
	uint8 			buffer[WLC_IOCTL_MAXLEN];	/* scratch buffer */
	nwm_wl_cnt_t	wlcntr_offset;		/* software 'zero' */
	wlc_nitro_cnt_t	nitrocntr_offset;	/* software 'zero' */
	int				interference_mode;	/* Cache interference setting */
};

typedef
struct nwm_err_conv_tbl {
	int bcm_err;
	int nwm_err;
} nwm_err_conv_tbl_t;

/* Tables using local data structures */
static
nwm_err_conv_tbl_t nwm_bcm2nwm_errtbl[] = {
	{0, NWM_CMDRES_SUCCESS},
	{BCME_ERROR, NWM_CMDRES_FAILURE},
	{BCME_BADARG, NWM_CMDRES_INVALID_PARAM},
	{BCME_BADOPTION, NWM_CMDRES_INVALID_PARAM},
	{BCME_NOTUP, NWM_CMDRES_STATE_WRONG},
	{BCME_NOTDOWN, NWM_CMDRES_STATE_WRONG},
	{BCME_NOTAP, NWM_CMDRES_ILLEGAL_MODE},
	{BCME_NOTSTA, NWM_CMDRES_ILLEGAL_MODE},
	{BCME_BADKEYIDX, NWM_CMDRES_INVALID_PARAM},
	{BCME_RADIOOFF, NWM_CMDRES_STATE_WRONG},
	{BCME_NOTBANDLOCKED, NWM_CMDRES_STATE_WRONG},
	{BCME_NOCLK, NWM_CMDRES_FAILURE},
	{BCME_BADRATESET, NWM_CMDRES_INVALID_PARAM},
	{BCME_BADBAND, NWM_CMDRES_INVALID_PARAM},
	{BCME_BUFTOOSHORT, NWM_CMDRES_FAILURE},
	{BCME_BUFTOOLONG, NWM_CMDRES_FAILURE},
	{BCME_BUSY, NWM_CMDRES_REQUEST_BUSY},
	{BCME_NOTASSOCIATED, NWM_CMDRES_STATE_WRONG},
	{BCME_BADSSIDLEN, NWM_CMDRES_INVALID_PARAM},
	{BCME_OUTOFRANGECHAN, NWM_CMDRES_INVALID_PARAM},
	{BCME_BADCHAN, NWM_CMDRES_INVALID_PARAM},
	{BCME_BADADDR, NWM_CMDRES_INVALID_PARAM},
	{BCME_NORESOURCE, NWM_CMDRES_NOT_ENOUGH_MEM},
	{BCME_UNSUPPORTED, NWM_CMDRES_NOT_SUPPORT},
	{BCME_BADLEN, NWM_CMDRES_INVALID_PARAM},
	{BCME_NOTREADY, NWM_CMDRES_STATE_WRONG},
	{BCME_EPERM, NWM_CMDRES_REFUSE},
	{BCME_NOMEM, NWM_CMDRES_NOT_ENOUGH_MEM},
	{BCME_ASSOCIATED, NWM_CMDRES_STATE_WRONG},
	{BCME_RANGE, NWM_CMDRES_INVALID_PARAM},
	{BCME_NOTFOUND, NWM_CMDRES_NOT_SUPPORT},
	{BCME_WME_NOT_ENABLED, NWM_CMDRES_STATE_WRONG},
	{BCME_TSPEC_NOTFOUND, NWM_CMDRES_FAILURE},
	{BCME_ACM_NOTSUPPORTED, NWM_CMDRES_NOT_SUPPORT},
	{BCME_NOT_WME_ASSOCIATION, NWM_CMDRES_STATE_WRONG},
	};

/* Turn a BCME_x into a NWM_CMDRES_x */
static uint16
nwm_bcmerr_to_nwmerr(int bcm_err)
{
	int i;

	for (i = 0; i < ARRAYSIZE(nwm_bcm2nwm_errtbl); i++) {
		if (nwm_bcm2nwm_errtbl[i].bcm_err == bcm_err)
			return nwm_bcm2nwm_errtbl[i].nwm_err;
	}
	/* ASSERT(0); */
	/* Not in table: just return general error */
	WL_NIN(("%s: BCME_x error %d not in table\n", __FUNCTION__, bcm_err));
	return NWM_CMDRES_FAILURE;
}


static const wlc_nin_cmdtbl_t wlc_nin_cmdtbl_mlme[] = {
{NWM_MLME_RESET_REQ_MINSIZE, NWM_MLME_RESET_CFM_MINSIZE, wlc_mlme_reset_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE,  NWM_CMD_RESVD_REQ_MINSIZE,  cmd_reserved_cmd  },
{NWM_MLME_SCAN_REQ_MINSIZE,  NWM_MLME_SCAN_CFM_MINSIZE,  wlc_mlme_scan_cmd },
{NWM_MLME_JOIN_REQ_MINSIZE,  NWM_MLME_JOIN_CFM_MINSIZE,  wlc_mlme_join_cmd },
{NWM_CMD_RESVD_REQ_MINSIZE,  NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE,  NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE,  NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE,  NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_MLME_DISASSOC_REQ_MINSIZE, NWM_MLME_DISASSOC_CFM_MINSIZE, nwm_mlme_deauth_cmd},
{NWM_MLME_START_REQ_MINSIZE, NWM_MLME_START_CFM_MINSIZE, wlc_mlme_start_cmd},
{NWM_MLME_MEASCHAN_REQ_MINSIZE, NWM_MLME_MEASCHAN_CFM_MINSIZE, wlc_mlme_measchan_cmd},
};

static const wlc_nin_cmdtbl_t wlc_nin_cmdtbl_ma[] = {
{MA_DATA_REQ_MINSIZE,        NWM_CMD_RESERVED_CFM_MINSIZE, wlc_ma_data_cmd},
{NWM_MA_KEYDATA_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, wlc_ma_keydata_cmd},
{NWM_MA_MP_REQ_MINSIZE,      NWM_CMD_RESERVED_CFM_MINSIZE, wlc_ma_mp_cmd},
{NWM_MA_TESTDATA_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, wlc_ma_testdata_cmd},
{NWM_MA_CLRDATA_REQ_MINSIZE, NWM_MA_CLRDATA_CFM_MINSIZE, wlc_ma_clrdata_cmd},
};

static const wlc_nin_cmdtbl_t wlc_nin_cmdtbl_paramset[] = {
{NWM_SET_ALL_REQ_MINSIZE,     NWM_SET_ALL_CFM_MINSIZE,     wlc_paramset_all_cmd},
{NWM_SET_MACADRS_REQ_MINSIZE, NWM_SET_MACADRS_CFM_MINSIZE, wlc_paramset_macadrs_cmd},
{NWM_SET_RETRY_REQ_MINSIZE,   NWM_SET_RETRY_CFM_MINSIZE,   wlc_paramset_retry_cmd},
{NWM_SET_ENCH_REQ_MINSIZE,    NWM_SET_ENCH_CFM_MINSIZE,   wlc_paramset_enablechannel_cmd},
{NWM_SET_MODE_REQ_MINSIZE,    NWM_SET_MODE_CFM_MINSIZE,   wlc_paramset_mode_cmd},
{NWM_SET_RATESET_REQ_MINSIZE, NWM_SET_RATESET_CFM_MINSIZE, wlc_paramset_rate_cmd},
{NWM_SET_WEPMODE_REQ_MINSIZE, NWM_SET_WEPMODE_CFM_MINSIZE, wlc_paramset_wepmode_cmd},
{NWM_SET_WEPKEYID_REQ_MINSIZE, NWM_SET_WEPKEYID_CFM_MINSIZE, wlc_paramset_wepkeyid_cmd},
{NWM_SET_WEPKEY_REQ_MINSIZE,  NWM_SET_WEPKEY_CFM_MINSIZE, wlc_paramset_wepkey_cmd},
{NWM_SET_BCN_TYPE_REQ_MINSIZE, NWM_SET_BCN_TYPE_CFM_MINSIZE, wlc_paramset_beacontype_cmd},
{NWM_SET_RES_BC_SSID_REQ_MINSIZE, NWM_SET_RES_BC_SSID_CFM_MINSIZE, wlc_paramset_resbcssid_cmd},
{NWM_SET_BCN_LOST_REQ_MINSIZE, NWM_SET_BCN_LOST_CFM_MINSIZE, wlc_paramset_beaconlostth_cmd},
{NWM_SET_ACT_ZONE_REQ_MINSIZE, NWM_SET_ACT_ZONE_CFM_MINSIZE, wlc_paramset_activezone_cmd},
{NWM_SET_SSID_MASK_REQ_MINSIZE, NWM_SET_SSID_MASK_CFM_MINSIZE, wlc_paramset_ssidmask_cmd},
{NWM_SET_PREAMBLE_TYPE_REQ_MINSIZE, NWM_SET_PREAMBLE_TYPE_CFM_MINSIZE,
wlc_paramset_preambletype_cmd},
{NWM_SET_AUTHALGO_REQ_MINSIZE, NWM_SET_AUTHALGO_CFM_MINSIZE, wlc_paramset_authalgo_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE,   NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_SET_LIFE_TIME_REQ_MINSIZE, NWM_SET_LIFE_TIME_CFM_MINSIZE, wlc_paramset_lifetime_cmd},
{NWM_SET_MAX_CONN_REQ_MINSIZE, NWM_SET_MAX_CONN_CFM_MINSIZE, wlc_paramset_maxconn_cmd},
{NWM_SET_TXANT_REQ_MINSIZE,   NWM_PARAMSET_TXANT_CFM_MINSIZE, wlc_paramset_txant_cmd},
{NWM_SET_DIVERSITY_REQ_MINSIZE, NWM_PARAMSET_DIVERSITY_CFM_MINSIZE, wlc_paramset_diversity_cmd},
{NWM_SET_BCNTXRXIND_REQ_MINSIZE, NWM_PARAMSET_BCNTXRXIND_CFM_MINSIZE,
wlc_paramset_bcnsendrecvind_cmd},
{NWM_SET_INTERFERENCE_REQ_MINSIZE,   NWM_PARAMSET_INTERFERENCE_CFM_MINSIZE,
wlc_paramset_interference_cmd},
};

static const wlc_nin_cmdtbl_t wlc_nin_cmdtbl_paramset2[] = {
{NWM_SET_BSSID_REQ_MINSIZE, NWM_SET_BSSID_CFM_MINSIZE, wlc_paramset_bssid_cmd          },
{NWM_CMD_RESVD_REQ_MINSIZE,  NWM_CMD_RESERVED_CFM_MINSIZE,  cmd_reserved_cmd          },
{NWM_SET_BCN_PERIOD_REQ_MINSIZE, NWM_SET_BCN_PERIOD_CFM_MINSIZE,
wlc_paramset_beaconperiod_cmd},
{NWM_SET_DTIM_PERIOD_REQ_MINSIZE, NWM_SET_DTIM_PERIOD_CFM_MINSIZE,
wlc_paramset_dtimperiod_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_SET_GAME_INFO_REQ_MINSIZE, NWM_SET_GAME_INFO_CFM_MINSIZE, wlc_paramset_gameinfo_cmd},
{NWM_SET_VBLANK_TSF_REQ_MINSIZE, NWM_SET_VBLANK_TSF_CFM_MINSIZE, wlc_paramset_vblanktsf_cmd },
{NWM_SET_MACLIST_REQ_MINSIZE, NWM_SET_MACLIST_CFM_MINSIZE, wlc_paramset_maclist_cmd},
{NWM_SET_RTSTHRESH_REQ_MINSIZE, NWM_SET_RTSTHRESH_CFM_MINSIZE, wlc_paramset_rtsthresh_cmd},
{NWM_SET_FRAGTHRESH_REQ_MINSIZE, NWM_SET_FRAGTHRESH_CFM_MINSIZE, wlc_paramset_fragthresh_cmd},
{NWM_SET_PMK_KEY_REQ_MINSIZE, NWM_SET_PMK_CFM_MINSIZE, wlc_paramset_pmk_cmd},
{NWM_SET_EEROM_REQ_MINSIZE, NWM_SET_EEROM_CFM_MINSIZE, wlc_paramset_eerom_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, wlc_paramset_txpwr_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, wlc_paramset_mrate_cmd},
};

static const wlc_nin_cmdtbl_t wlc_nin_cmdtbl_paramget[] = {
{NWM_GET_ALL_REQ_MINSIZE, NWM_GET_ALL_CFM_MINSIZE, wlc_paramget_all_cmd},
{NWM_GET_MACADRS_REQ_MINSIZE, NWM_GET_MACADRS_CFM_MINSIZE, wlc_paramget_macadrs_cmd},
{NWM_GET_RETRY_REQ_MINSIZE, NWM_GET_RETRY_CFM_MINSIZE, wlc_paramget_retry_cmd},
{NWM_GET_ENCH_REQ_MINSIZE, NWM_GET_ENCH_CFM_MINSIZE, wlc_paramget_enablechannel_cmd},
{NWM_GET_MODE_REQ_MINSIZE, NWM_GET_MODE_CFM_MINSIZE, wlc_paramget_mode_cmd},
{NWM_GET_RATESET_REQ_MINSIZE, NWM_GET_RATESET_CFM_MINSIZE, wlc_paramget_rate_cmd},
{NWM_GET_WEPMODE_REQ_MINSIZE, NWM_GET_WEPMODE_CFM_MINSIZE, wlc_paramget_wepmode_cmd},
{NWM_GET_WEPKEYID_REQ_MINSIZE, NWM_GET_WEPKEYID_CFM_MINSIZE, wlc_paramget_wepkeyid_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_GET_BCN_TYPE_REQ_MINSIZE, NWM_GET_BCN_TYPE_CFM_MINSIZE, wlc_paramget_beacontype_cmd},
{NWM_GET_RES_BC_SSID_REQ_MINSIZE, NWM_GET_RES_BC_SSID_CFM_MINSIZE, wlc_paramget_resbcssid_cmd},
{NWM_GET_BCN_LOST_REQ_MINSIZE, NWM_GET_BCN_LOST_CFM_MINSIZE, wlc_paramget_beaconlostth_cmd},
{NWM_GET_ACT_ZONE_REQ_MINSIZE, NWM_GET_ACT_ZONE_CFM_MINSIZE, wlc_paramget_activezone_cmd},
{NWM_GET_SSID_MASK_REQ_MINSIZE, NWM_GET_SSID_MASK_CFM_MINSIZE, wlc_paramget_ssidmask_cmd},
{NWM_GET_PREAMBLE_TYPE_REQ_MINSIZE, NWM_GET_PREAMBLE_TYPE_CFM_MINSIZE,
wlc_paramget_preambletype_cmd},
{NWM_GET_AUTHALGO_REQ_MINSIZE, NWM_GET_AUTHALGO_CFM_MINSIZE, wlc_paramget_authalgo_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_GET_MAX_CONN_REQ_MINSIZE, NWM_GET_MAX_CONN_CFM_MINSIZE, wlc_paramget_maxconn_cmd},
{PARAMGET_TXANT_REQ_MINSIZE, NWM_GET_TXANT_CFM_MINSIZE, wlc_paramget_txant_cmd},
{PARAMGET_DIVERSITY_REQ_MINSIZE, NWM_GET_DIVERSITY_CFM_MINSIZE, wlc_paramget_diversity_cmd},
{NWM_GET_BCNTXRXIND_REQ_MINSIZE, NWM_GET_BCNTXRXIND_CFM_MINSIZE, wlc_paramget_bcnsendrecvind_cmd},
{PARAMGET_INTERFERENCE_REQ_MINSIZE, NWM_GET_INTERFERENCE_CFM_MINSIZE,
wlc_paramget_interference_cmd},
};

static const wlc_nin_cmdtbl_t wlc_nin_cmdtbl_paramget2[] = {
{NWM_GET_BSSID_REQ_MINSIZE, NWM_GET_BSSID_CFM_MINSIZE, wlc_paramget_bssid_cmd          },
{NWM_GET_SSID_REQ_MINSIZE, NWM_GET_SSID_CFM_MINSIZE, wlc_paramget_ssid_cmd           },
{NWM_GET_BCN_PERIOD_REQ_MINSIZE, NWM_GET_BCN_PERIOD_CFM_MINSIZE,
wlc_paramget_beaconperiod_cmd},
{NWM_GET_DTIM_PERIOD_REQ_MINSIZE, NWM_GET_DTIM_PERIOD_CFM_MINSIZE,
wlc_paramget_dtimperiod_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_GET_GAME_INFO_REQ_MINSIZE, NWM_GET_GAME_INFO_CFM_MINSIZE, wlc_paramget_gameinfo_cmd},
{NWM_GET_VBLANK_TSF_REQ_MINSIZE, NWM_GET_VBLANK_TSF_CFM_MINSIZE, wlc_paramget_vblanktsf_cmd},
{NWM_GET_MACLIST_REQ_MINSIZE, NWM_GET_MACLIST_CFM_MINSIZE, wlc_paramget_maclist_cmd},
{NWM_GET_RTSTHRESH_REQ_MINSIZE, NWM_GET_RTSTHRESH_CFM_MINSIZE, wlc_paramget_rtsthresh_cmd},
{NWM_GET_FRAGTHRESH_REQ_MINSIZE, NWM_GET_FRAGTHRESH_CFM_MINSIZE, wlc_paramget_fragthresh_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_GET_EEROM_REQ_MINSIZE, NWM_GET_EEROM_CFM_MINSIZE, wlc_paramget_eerom_cmd},
{NWM_GET_RSSI_REQ_MINSIZE, NWM_GET_RSSI_CFM_MINSIZE, wlc_paramget_rssi_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, wlc_paramget_txpwr_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, wlc_paramget_mrate_cmd},
};

static const wlc_nin_cmdtbl_t wlc_nin_cmdtbl_dev[] = {
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_CMD_RESVD_REQ_MINSIZE, NWM_CMD_RESERVED_CFM_MINSIZE, cmd_reserved_cmd},
{NWM_DEV_IDLE_REQ_MINSIZE, NWM_DEV_IDLE_CFM_MINSIZE, wlc_dev_idle_cmd},
{NWM_DEV_CLASS1_REQ_MINSIZE, NWM_DEV_CLASS1_CFM_MINSIZE, wlc_dev_class1_cmd},
{NWM_DEV_REBOOT_REQ_MINSIZE, NWM_DEV_REBOOT_CFM_MINSIZE, wlc_dev_reboot_cmd},
{NWM_DEV_CLR_WLINFO_REQ_MINSIZE, NWM_DEV_CLR_WLINFO_CFM_MINSIZE, wlc_dev_clearwlinfo_cmd},
{NWM_DEV_GET_VERINFO_REQ_MINSIZE, NWM_DEV_GET_VERINFO_CFM_MINSIZE, wlc_dev_getverinfo_cmd},
{NWM_DEV_GET_WLINFO_REQ_MINSIZE, NWM_DEV_GET_WLINFO_CFM_MINSIZE, wlc_dev_getwlinfo_cmd},
{NWM_DEV_GET_STATE_REQ_MINSIZE, NWM_DEV_GET_STATE_CFM_MINSIZE, wlc_dev_getstate_cmd},
{NWM_DEV_TEST_SIGNAL_REQ_MINSIZE, NWM_DEV_TEST_SIGNAL_CFM_MINSIZE, wlc_dev_testsignal_cmd},
};

/*
 * The main dispatch function
 */

#define MLME_REQ_NUM			ARRAYSIZE(wlc_nin_cmdtbl_mlme)
#define MA_REQ_NUM				ARRAYSIZE(wlc_nin_cmdtbl_ma)
#define PARAMSET_REQ_NUM		ARRAYSIZE(wlc_nin_cmdtbl_paramset)
#define PARAMSET2_REQ_NUM		ARRAYSIZE(wlc_nin_cmdtbl_paramset2)
#define PARAMGET_REQ_NUM		ARRAYSIZE(wlc_nin_cmdtbl_paramget)
#define PARAMGET2_REQ_NUM		ARRAYSIZE(wlc_nin_cmdtbl_paramget2)
#define DEV_REQ_NUM				ARRAYSIZE(wlc_nin_cmdtbl_dev)

extern bool wlc_nin_dispatch(wlc_info_t * wlc, void * pkt, struct wlc_if *wlcif);
/* TRUE means return a response */
bool
wlc_nin_dispatch(wlc_info_t * wlc, void * pkt, struct wlc_if *wlcif)
{
	nwm_cmd_req_t* pReq;
	nwm_cmd_cfm_t* pCfm;
	const wlc_nin_cmdtbl_t * pCmdTbl;
	uint16	vCode, vCodeMax, err = 0;
	bool cmdresult = TRUE;
	bool pktsend;
	uint32 computed_len;
	uint32 len = PKTLEN(wlc->osh, pkt);


	/* Long enough to look at request length field ? */
	if (len < NWM_MIN_ENCAP_SIZE) {
		WL_NIN((
			"wl%d: ERROR: packet too short: %d bytes, "
			"expected min %d bytes, exiting\n",
			wlc->pub->unit, (int)len, (int)NWM_MIN_PKT_SIZE));
		return TRUE;
	}

	WL_NIN((
	"wl%d: request length is 0x%x Headroom is 0x%x Tailroom is 0x%x\n",
	wlc->pub->unit, len, (int)PKTHEADROOM(wlc->osh, pkt),
	(int)PKTTAILROOM(wlc->osh, pkt)));

	/* Get the command request & confirm portions */
	pReq = (nwm_cmd_req_t *)PKTDATA(wlc->osh, pkt);
	pCfm = (nwm_cmd_cfm_t *)(WL_CalcConfirmPointer(pReq));
	WL_NIN(("wl%d Alignment of data is 0x%x\n",
	        wlc->pub->unit, ((uint32)(uintptr)pReq & 0x3)));

	WL_NIN(("wl%d: %s cmd 0x%x\n",
	wlc->pub->unit, __FUNCTION__, pReq->header.code));

	/* pkt send commands do not have a confirm field */
	pktsend = NWM_IS_PKT_SEND_CMD(pReq->header.code);
	computed_len = (pReq->header.length * 2) + NWM_MIN_GET_REQ_SIZE;
	computed_len += pktsend ? 0 : NWM_MIN_SET_CFM_SIZE;

	if (computed_len > len) {
		WL_NIN(("wl%d: ERROR: packet too short:\n", wlc->pub->unit));
		WL_NIN(("wl %d: Computed %d bytes, given %d bytes, exiting\n",
			wlc->pub->unit, computed_len, len));
		return TRUE;
	}

	if (!pktsend && (pReq->header.code != pCfm->header.code))
	{
		WL_NIN(("wl%d: %s: ERROR: req 0x%x cfm 0x%x reqlen 0x%x \n",
		wlc->pub->unit, __FUNCTION__, pReq->header.code,
		pCfm->header.code, pReq->header.length));
		err = NWM_CMDRES_CONFIRM_CODE_ERR;
		goto error_exit;
	}

	pCmdTbl  = wlc_nin_cmdtbl_dev;
	switch (pReq->header.code & NWM_CMDGCODE_MASK) {
	case NWM_CMDGCODE_MLME:
		pCmdTbl  = wlc_nin_cmdtbl_mlme;
		vCode    = pReq->header.code & NWM_CMDSCODE_MASK;
		vCodeMax = MLME_REQ_NUM;

		break;

	case NWM_CMDGCODE_MA:
		pCmdTbl  = wlc_nin_cmdtbl_ma;
		vCode    = pReq->header.code & NWM_CMDSCODE_MASK;
		vCodeMax = MA_REQ_NUM;

		break;

	case NWM_CMDGCODE_PARAM:
		vCode    = pReq->header.code & NWM_CMDSCODE_MASK;
		if (vCode < (NWM_CMDCODE_PARAM_SET_BSSID & NWM_CMDSCODE_MASK)) {

			pCmdTbl  = wlc_nin_cmdtbl_paramset;
			vCodeMax = PARAMSET_REQ_NUM;
		} else
		if (vCode < (NWM_CMDCODE_PARAM_GET_ALL & NWM_CMDSCODE_MASK)) {
			pCmdTbl  = wlc_nin_cmdtbl_paramset2;
			vCode   -= NWM_SET2_STR_OFST;
			vCodeMax = PARAMSET2_REQ_NUM;
		} else
		if (vCode < (NWM_CMDCODE_PARAM_GET_BSSID & NWM_CMDSCODE_MASK)) {
			pCmdTbl  = wlc_nin_cmdtbl_paramget;
			vCode   -= NWM_GET_STR_OFST;
			vCodeMax = PARAMGET_REQ_NUM;
		} else {
			pCmdTbl  = wlc_nin_cmdtbl_paramget2;
			vCode   -= NWM_GET2_STR_OFST;
			vCodeMax = PARAMGET2_REQ_NUM;
		}
		break;

	case	NWM_CMDGCODE_DEV:
		pCmdTbl  = wlc_nin_cmdtbl_dev;
		vCode    = pReq->header.code & NWM_CMDSCODE_MASK;
		vCodeMax = DEV_REQ_NUM;
		break;

	default:
		vCode    = 1;
		vCodeMax = 0;
	} /* end switch */

	WL_NIN(("%s: vCode 0x%x vCodeMax 0x%x\n", __FUNCTION__, vCode, vCodeMax));
	if (vCode >= vCodeMax)
	{
		WL_NIN(("wl%d: %s ERROR: cmd 0x%x vcode 0x%x vCodeMax 0x%x invalid\n",
		wlc->pub->unit, __FUNCTION__, pReq->header.code, vCode, vCodeMax));
		err = NWM_CMDRES_NOT_SUPPORT;
		goto error_exit;
	}

	else
	if ((pReq->header.length < pCmdTbl[vCode].RequestMinLength) ||
		(!pktsend && (pCfm->header.length < pCmdTbl[vCode].ConfirmMinLength)))
	{
		WL_NIN((
		"wl%d: %s ERROR: cmd 0x%x reqlen 0x%x reqtbllen 0x%x "
		"cfmlen 0x%x cfmtbllen 0x%x invalid\n",
		wlc->pub->unit, __FUNCTION__, pReq->header.code,
		pReq->header.length, pCmdTbl[vCode].RequestMinLength,
		pCfm->header.length, pCmdTbl[vCode].ConfirmMinLength));
		err = NWM_CMDRES_LENGTH_ERR;
		goto error_exit;
	}


	/* Do it: return code tells us to send/not send a response */

	/* store this now: pkt might get freed underneath us */

	cmdresult   = (*pCmdTbl[vCode].pCmdFunc)(pkt, pReq, pCfm, wlc, wlcif);
	WL_NIN(("wl%d: %s cmd 0x%x cmdresult 0x%x\n",
	wlc->pub->unit, __FUNCTION__, code, cmdresult));

	return cmdresult;

	/*
	 * Respond with error status
	 */
error_exit:
	WL_NIN(("wl%d: %s ERROR: cmd 0x%x errcode 0x%x\n",
	wlc->pub->unit, __FUNCTION__, pReq->header.code, err));
	pCfm->header.length = 1;
	pCfm->resultCode    = err;
	return TRUE;
}
static int
wlc_nin_restore_defaults(wlc_info_t *wlc, struct wlc_if *wlcif)
{
	uint32 err = 0;
	uint16 arg1, arg2;
	uint16 result, opmode;
	uint16 ssid_mask[16];


	/* short and long retry limits, respectively */
	arg1 = 7;
	arg2 = 4;
	wlc_nin_retrycmd(wlc, wlcif, &arg1, &arg2, &result, FALSE);
	err |= result;

	if (err)
		WL_ERROR(("wl%d: %s: error setting retry limits\n",
			wlc->pub->unit, __FUNCTION__));

	/* set beacon frame type */
	arg1 = 0;
	wlc_nin_beaconfrmtype(wlc, wlcif, &arg1, &result, FALSE);
	err |= result;
	if (result)
		WL_ERROR(("wl%d: %s: error setting beacon frame type\n",
			wlc->pub->unit, __FUNCTION__));

	/* set probe response */
	arg1 = 0;
	wlc_nin_prbresp(wlc, wlcif, &arg1, &result, FALSE);
	err |= result;
	if (result)
		WL_ERROR(("wl%d: %s: error setting probe response\n",
			wlc->pub->unit, __FUNCTION__));

	arg1 = 16;
	wlc_nin_bcntimeout(wlc, wlcif, &arg1, &result, FALSE);
	err |= result;
	if (result)
		WL_ERROR(("wl%d: %s: error setting beacon lost thresh\n",
			wlc->pub->unit, __FUNCTION__));

	/* active zone: only for nitro parent */
	wlc_nin_get_modecmd(wlc, wlcif, &opmode, &result);
	err |= result;
	if (result)
		WL_ERROR(("wl%d: %s: error getting opmode\n",
			wlc->pub->unit, __FUNCTION__));
	else if (opmode == NWM_NIN_NITRO_PARENT_MODE) {
		arg1 = 0xffff;
		wlc_nin_activezone(wlc, wlcif, &arg1, &result, FALSE);
		err |= result;
		if (result)
			WL_ERROR(("wl%d: %s: error setting active zone\n",
				wlc->pub->unit, __FUNCTION__));
	}

	bzero(ssid_mask, sizeof(ssid_mask));
	wlc_nin_ssidmask(wlc, wlcif, ssid_mask, &result, FALSE);
	err |= result;
	if (result)
		WL_ERROR(("wl%d: %s: error clearing ssid mask \n",
			wlc->pub->unit, __FUNCTION__));

	/* preamble aka PLCP header */
	arg1 = 1;
	wlc_nin_preambleCmd(wlc, wlcif, &arg1, &result, FALSE);
	err |= result;
	if (result)
		WL_ERROR(("wl%d: %s: error setting preamble\n",
			wlc->pub->unit, __FUNCTION__));

	/* auth algorithm: open */
	arg1 = 0;
	wlc_nin_authalgocmd(wlc, wlcif, &arg1, &result, FALSE);
	err |= result;
	if (result)
		WL_ERROR(("wl%d: %s: error setting auth algorithm\n",
			wlc->pub->unit, __FUNCTION__));

	/* antenna diversity mode: ON */
	wlc_nin_set_diversity(wlc, wlcif, 1, 0, &result);
	err |= result;
	if (result)
		WL_ERROR(("wl%d: %s: error setting ant div\n",
			wlc->pub->unit, __FUNCTION__));

	/* beacon indication enables */
	arg1 = arg2 = 0;
	wlc_nin_bcnind_enable(wlc, wlcif, &arg1, &arg2, &result, FALSE);
	err |= result;
	if (result)
		WL_ERROR(("wl%d: %s: error setting beacon indication enables\n",
			wlc->pub->unit, __FUNCTION__));

	/* beacon period */
	arg1 = 16;
	wlc_nin_bcnprd(wlc, wlcif, &arg1, &result, FALSE);
	err |= result;
	if (result)
		WL_ERROR(("wl%d: %s: error setting beacon period\n",
			wlc->pub->unit, __FUNCTION__));

	/* dtim_period */
	arg1 = 1;
	wlc_nin_dtimprd(wlc, wlcif, &arg1, &result, FALSE);
	err |= result;
	if (result)
		WL_ERROR(("wl%d: %s: error setting dtim period\n",
			wlc->pub->unit, __FUNCTION__));
	return err;
}

/*
 * rateset
 *
 */
static struct {
	uint8 rate;
	bool basic;
} nwm_rateconvtbl[] = {
	{2, TRUE},		/* 1 Mbps */
	{4, TRUE},		/* 2 Mbps */
	{11, TRUE},		/* 5.5 Mbps */
	{12, FALSE},		/* 6 Mbps */
	{18, FALSE},		/* 9 Mbps */
	{22, TRUE},		/* 11 Mbps */
	{24, FALSE},		/* 12 Mbps */
	{36, FALSE},		/* 18 Mbps */
	{48, FALSE},		/* 24 Mbps */
	{72, FALSE},		/* 36 Mbps */
	{96, FALSE},		/* 48 Mbps */
	{108, FALSE},			/* 54 Mbps */
};
wlc_nin_t *
BCMATTACHFN(wlc_nin_attach)(wlc_info_t *wlc)
{
	int err;
#ifdef ROAMING_DISABLE
	uint32 arg;
#endif
	wlc_nin_t *nin;
	uint8 bitvec[WL_EVENTING_MASK_LEN];

	/* Here to make sure it always executes */
	STATIC_ASSERT(NWM_PKT_HDROOM >= TXOFF);

	if (!(nin = (wlc_nin_t *)MALLOC(wlc->osh, sizeof(wlc_nin_t)))) {
		WL_ERROR(("wl%d: wlc_nin_attach: out of mem, malloced %d bytes\n",
			wlc->pub->unit, MALLOCED(wlc->osh)));
		return NULL;
	}

	bzero((char *)nin, sizeof(wlc_nin_t));
	nin->wlc = wlc;
	nin->interference_mode = WLAN_AUTO;	/* should match the default in wlc_phy.c */

#ifdef ROAMING_DISABLE
	arg = 1;
	err = wlc_iovar_op(wlc, "roam_off", NULL, 0, &arg, sizeof(arg), IOV_SET, NULL);
	if (err)
		WL_ERROR(("wl%d: wlc_nin_attach: Failed to disable roaming\n",
			wlc->pub->unit));
#endif /* ROAMING_DISABLE */

	err = wlc_iovar_op(wlc, "event_msgs", NULL, 0,
		bitvec, sizeof(bitvec), IOV_GET, NULL);
	if (err)
		WL_ERROR(("wl%d: wlc_nin_attach: Failed to get event_msgs bitvec\n",
			wlc->pub->unit));

	/* enable interesting event flags */
	setbit(bitvec, WLC_E_SET_SSID);
	setbit(bitvec, WLC_E_ASSOC_IND);
	setbit(bitvec, WLC_E_REASSOC_IND);
	setbit(bitvec, WLC_E_DISASSOC_IND);
	setbit(bitvec, WLC_E_DISASSOC);
	setbit(bitvec, WLC_E_DEAUTH_IND);
	setbit(bitvec, WLC_E_DEAUTH);
	setbit(bitvec, WLC_E_LINK);
	setbit(bitvec, WLC_E_BCNLOST_MSG);
	err = wlc_iovar_op(wlc, "event_msgs", NULL, 0,
		bitvec, sizeof(bitvec), IOV_SET, NULL);
	if (err)
		WL_ERROR(("wl%d: wlc_nin_attach: Failed to set event_msgs bitvec\n",
			wlc->pub->unit));

	err = wlc_nin_restore_defaults(wlc, NULL);
	if (err)
		WL_ERROR(("wl%d: wlc_nin_attach: Failed to restore defaults\n",
			wlc->pub->unit));
	return nin;
}

void
BCMATTACHFN(wlc_nin_detach)(wlc_nin_t *nin)
{
	if (!nin)
		return;

	MFREE(nin->wlc->osh, nin, sizeof(wlc_nin_t));
}

/* following are the functions that are plugged into the tables */

/* MLME */
/*
 * Conditionally init wireless counters per supplied parameter
 * Do we have any to clear?
 * See wlc->pub->_cnt->*
 * Overall effect roughly similar to WLC_DOWN followed by WLC_UP
 * but we also zero out the ssid so as to wind up in Class1 state
 *
 */
static bool wlc_mlme_reset_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	int err, arg, state;
	uint16 result;
	wlc_ssid_t ssid_arg;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	char *pbuf = (char *)(pnin->buffer);
	int iolen;
	int arglen = strlen("ssid") + 1 + strlen("bsscfg:");

	/* wipe it */
	bzero(&ssid_arg, sizeof(ssid_arg));

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  preq->header.code,
		preq->header.length));

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state == NWM_NIN_IDLE) {
		err = BCME_NOTUP;
		WL_ERROR(("wl%d %s: illegal state: not up\n",
			wlc->pub->unit, __FUNCTION__));
		goto done;
	}

	arg = 0;
	err = wlc_ioctl(wlc, WLC_DOWN, &arg, sizeof(arg), wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: failed to set WLC_DOWN err %d\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	iolen = wlc_bssiovar_mkbuf("ssid", 0, &ssid_arg, sizeof(ssid_arg),
		pbuf, sizeof(pnin->buffer), &err);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d creating ssid iobuf\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	err = wlc_iovar_op(wlc, pbuf, NULL, 0, (void*)((int8*)pbuf + arglen),
		iolen - arglen, IOV_SET, wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d setting ssid\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}


	err = wlc_ioctl(wlc, WLC_UP, &arg, sizeof(arg), wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: failed to set WLC_UP err %d\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	/* conditionally "clear" the wireless counters */
	if (((nwm_mlme_reset_req_t *)preq)->mib) {
		wlc_nin_clr_wlcounters(wlc, wlcif, &result);
		err = result ? BCME_ERROR : 0;
		if (err) {
			WL_ERROR(("wl%d %s: error clearing counters err %d\n",
				wlc->pub->unit, __FUNCTION__, err));
		}
	}

done:
	pcfm->resultCode = nwm_bcmerr_to_nwmerr(err);
	return TRUE;
}

static char rateconvarr[] = {
	2,		/* 1Mbps	*/
	4,		/* 2Mbps	*/
	11,		/* 5.5Mbps	*/
	12,		/* 6 Mbps	*/
	18,		/* 9 Mbps	*/
	22,		/* 11 Mbps	*/
	24,		/* 12 Mbps	*/
	36,		/* 18 Mbps	*/
	48,		/* 24 Mbps	*/
	72,		/* 36 Mbps	*/
	96,		/* 48 Mbps	*/
	108,	/* 54 Mbps	*/
};
	/*
	* Scan callback function:
	* packages scan results as indication and sends it up
	*/

static void
wl_scan_complete(void *arg, int status, wlc_bsscfg_t *cfg)
{
	void *pkt;
	wlc_info_t *wlc = (wlc_info_t *)arg;
	int buflen, i, j, k;
	nwm_scanind_t *pind;
	nwm_bss_desc_t *pi;
	wlc_bss_info_t *bi;
	int accum_bsslen = 0;
	int next_bss_offset = 0;
	uint8 *pbuf;
	uint bytes_copied;
	int copylen;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);

	if (status != WLC_E_STATUS_SUCCESS)
		return;

	pbuf = (uint8 *)(pnin->buffer);

	WL_NIN(("wl%d %s called: wlnin requested Scan complete\n",
	wlc->pub->unit, __FUNCTION__));
	/* how much did we get? */
	for (buflen = i = 0; i < wlc->scan_results->count; i++) {
		bi = wlc->scan_results->ptrs[i];
		ASSERT(bi != NULL);

		buflen += NWM_BSSDESC_FIXED_SIZE;
		if (bi->bcn_prb) {
			ASSERT(bi->bcn_prb_len > DOT11_BCN_PRB_LEN);
			buflen += ROUNDUP((bi->bcn_prb_len - DOT11_BCN_PRB_LEN), 2);
		}
	}

	buflen += NWM_SCANIND_FIXED_SIZE;
	WL_NIN(("wl%d %s: buflen is %d\n", wlc->pub->unit, __FUNCTION__, buflen));

	/* header */
	buflen += ROUNDUP(ETHER_HDR_LEN, 4);
	WL_NIN(("wl %d %s: buflen is %d\n", wlc->pub->unit, __FUNCTION__, buflen));

	if (buflen > sizeof(pnin->buffer)) {

		WL_ERROR(("wl%d %s: scan results too large: %d, bailing\n",
			wlc->pub->unit, __FUNCTION__, buflen));
		return;
	}
	bzero(pbuf, buflen);
	pind = (nwm_scanind_t *)pbuf;
	pind->header.magic     = NWM_NIN_IND_MAGIC;
	pind->header.indcode   = NWM_CMDCODE_MLME_SCANCMPLT_IND;
	pind->status = NWM_CMDRES_SUCCESS;
	pind->header.indlength = 0;

	/* traverse the list & copy over data */
	pi = pind->bss_desc_list;
	pind->bss_desc_count = wlc->scan_results->count;
	WL_NIN(("wl%d %s: number of bss's %d\n",
	wlc->pub->unit, __FUNCTION__, wlc->scan_results->count));
	for (i = 0; i < wlc->scan_results->count; i++,
		pi = (nwm_bss_desc_t*)((int8*)pi + next_bss_offset)) {
		bi = wlc->scan_results->ptrs[i];

		pi->length = NWM_BSSDESC_FIXED_SIZE;
		if (bi->bcn_prb) {
			uint8 *ie;
			int ie_len;

			ie = (uint8 *)bi->bcn_prb + DOT11_BCN_PRB_LEN;
			ie_len = bi->bcn_prb_len - DOT11_BCN_PRB_LEN;

			pi->length += ie_len;
			pi->info_elt_len = ie_len;
			bcopy(ie, (char *)&pi->info_elt, ie_len);
		}
		pi->rssi = bi->RSSI;
		bcopy(bi->BSSID.octet, (char *)&pi->bssid, ETHER_ADDR_LEN);
		pi->ssidlength = bi->SSID_len;
		bcopy(bi->SSID, pi->ssid, bi->SSID_len);
		pi->capainfo = bi->capability;
		pi->beaconPeriod = bi->beacon_period;
		pi->dtimPeriod = bi->dtim_period;
		pi->channel = CHSPEC_CHANNEL(bi->chanspec);
		pi->cfpPeriod = 0;
		pi->cfpMaxDuration = 0;
		/* Nintendo wants length in uint16 words, bssdesc's
		 * aligned on uint16 boundaries
		 */
		pi->length = ROUNDUP(pi->length, 2);
		accum_bsslen +=  pi->length;
		next_bss_offset = pi->length;
		pi->length /= 2;

		for (j = 0; j < bi->rateset.count; j++) {
			int basic = bi->rateset.rates[j] & 0x80;
			int rate  = bi->rateset.rates[j] & 0x7f;

			for (k = 0; k < ARRAYSIZE(rateconvarr); k++) {
				if (rate == rateconvarr[k]) {
					pi->rateset.support |= NBITVAL(k);
					if (basic)
						pi->rateset.basic |= NBITVAL(k);
				}
			}
		}
	}

	pind->header.indlength = (accum_bsslen + NWM_SCANIND_FIXED_LEN)/2;
	copylen = accum_bsslen + NWM_SCANIND_FIXED_SIZE;
	WL_NIN(("wl%d Scan Complete: pktlen 0x%x indlength 0x%x\n",
		wlc->pub->unit, copylen, pind->header.indlength));

	pkt = wl_get_pktbuffer(wlc->osh, buflen);
	if (pkt == NULL) {
		WL_ERROR(("wl%d %s: pkt allocation failed for len %d\n",
			wlc->pub->unit, __FUNCTION__, buflen));
		return;
	}

	bytes_copied = wl_buf_to_pktcopy(wlc->osh, pkt, pbuf, copylen,
		ROUNDUP(ETHER_HDR_LEN, sizeof(uint32)));
	if (bytes_copied != copylen) {
		WL_ERROR((
			"wl%d %s: error copying scan buffer: expected %d copied %d\n",
				wlc->pub->unit, __FUNCTION__, copylen, bytes_copied));
		PKTFREE(wlc->osh, pkt, FALSE);
		return;
	}

	PKTPULL(wlc->osh, pkt, ROUNDUP(ETHER_HDR_LEN, sizeof(uint32)));
	wlc_nin_sendup(wlc, pkt, NULL);
	return;
}

/*
 * Do a scan
 */
static bool wlc_mlme_scan_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	int err;
	uint8 i, count;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);

	wl_scan_params_t *params = (wl_scan_params_t *)(pnin->buffer);
	nwm_mlme_scan_req_t *pscan = (nwm_mlme_scan_req_t *)preq;
	int params_size = WL_SCAN_PARAMS_FIXED_SIZE + WL_NUMCHANNELS * sizeof(uint16);
	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));
	/* get the args from pscan into params */
	bzero(params, params_size);
	if (pscan->ssid_length) {
		bcopy((char *)pscan->ssid, params->ssid.SSID, pscan->ssid_length);
		params->ssid.SSID_len = pscan->ssid_length;
	}

	bcopy((char *)pscan->bssid, params->bssid.octet, ETHER_ADDR_LEN);
	params->bss_type = DOT11_BSSTYPE_ANY;
	params->nprobes = -1;
	params->scan_type = pscan->scan_type;
	params->active_time = params->passive_time = pscan->max_channel_time;
	params->home_time = -1;
	params->channel_num = 0;

	/* bits 0,15 correspond to invalid, unused channels respectively */
	for (i = 1, count = 0; i < NWM_MAX_CHANNELS - 1; i++) {
		if (pscan->chvector & NBITVAL(i)) {
			params->channel_list[count] = wlc_create_chspec(wlc, i);
			WL_NIN(("wl%d %s channel %d chanspec 0x%x\n",
			wlc->pub->unit, __FUNCTION__, i, params->channel_list[count]));
			count++;

		}
	}
	params->channel_num = count;


	/* fix this up before passing args */
	params_size = WL_SCAN_PARAMS_FIXED_SIZE + params->channel_num * sizeof(uint16);

	err = wlc_scan_request(wlc, params->bss_type, &params->bssid, 1, &params->ssid,
	                       params->scan_type, params->nprobes, params->active_time,
	                       params->passive_time, params->home_time, params->channel_list,
	                       params->channel_num, TRUE, wl_scan_complete, wlc);

	if (err)
		pcfm->resultCode = NWM_CMDRES_FAILURE;
	else
		pcfm->resultCode = NWM_CMDRES_SUCCESS;

	return TRUE;

}

/*
 * The opmode, i.e. wlc_nin_set_modecmd(), should be set before this.
 */

static void
wlc_nin_joincmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 ssidlen,
	uint8 *ssid, uint16 capainfo, uint16 *result)
{
	int err;
	wlc_ssid_t ssid_arg;
	int state;

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_CLASS1) {
		WL_ERROR(("wl%d %s: state %d not class1\n",
			wlc->pub->unit, __FUNCTION__, state));
		err = BCME_NOTREADY;
		goto done;
	}
	ssid_arg.SSID_len = ssidlen;
	bcopy(ssid, ssid_arg.SSID, ssid_arg.SSID_len);

	/* Join the ssid. It's all automatic from here on. */
	err = wlc_ioctl(wlc, WLC_SET_SSID, &ssid_arg, sizeof(ssid_arg), wlcif);
	if (err)
		WL_NIN(("wl%d: %s: failed to set ssid\n",
			wlc->pub->unit, __FUNCTION__));

done:
	*result = nwm_bcmerr_to_nwmerr(err);
	return;
}

static bool wlc_mlme_join_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_mlme_join_req_t *prq = (nwm_mlme_join_req_t *)pReq;
	nwm_mlme_join_cfm_t *pcfm = (nwm_mlme_join_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_joincmd(wlc, wlcif, prq->bssdesc.ssidlength,
		prq->bssdesc.ssid, prq->bssdesc.capainfo, &pcfm->resultcode);

	return TRUE;
}
static bool nwm_mlme_deauth_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	uint16 mode, result, err;
	scb_val_t scbarg;
	int state;
	int arg = 0;
	nwm_mlme_disassoc_req_t *p = (nwm_mlme_disassoc_req_t *)preq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_CLASS3) {
		WL_ERROR(("wl%d %s: state %d not class3\n",
			wlc->pub->unit, __FUNCTION__, state));
		err = BCME_NOTREADY;
		goto done;
	}

	wlc_nin_get_modecmd(wlc, wlcif, &mode, &result);
	if (result != NWM_CMDRES_SUCCESS) {
		WL_ERROR(("wl%d %s: error %d retrieving mode\n",
			wlc->pub->unit, __FUNCTION__, result));
		err = BCME_ERROR;
		goto done;
	}

	if (NWM_IS_AP_PARENT(mode)) {
		scbarg.val = p->reason;
		bcopy((char *)p->mac_addr, scbarg.ea.octet, ETHER_ADDR_LEN);
		err = wlc_ioctl(wlc, WLC_SCB_DEAUTHENTICATE_FOR_REASON, &scbarg,
			sizeof(scbarg), wlcif);
	} else {
		err = wlc_ioctl(wlc, WLC_DISASSOC, &arg, sizeof(arg), wlcif);
	}

	WL_NIN(("wl%d %s mode=%d err=%d\n", wlc->pub->unit, __FUNCTION__, mode, err));

done:
	pcfm->resultCode = nwm_bcmerr_to_nwmerr(err);
	return TRUE;
}

/*
 * Start the BSS (parent/AP)
 * Set a bunch of parameters and go to CLASS3 (AP)
 * See state diagram
 *
 */
static bool wlc_mlme_start_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_mlme_startreq_t *p = (nwm_mlme_startreq_t *)preq;
	int arg = 0;
	wlc_ssid_t ssidarg;
	int err = 0;
	wl_rateset_t rateset;
	int i, j;
	int state;
	uint16 rvector, brvector;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_CLASS1) {
		WL_ERROR(("wl%d %s: state %d not class1\n",
			wlc->pub->unit, __FUNCTION__, state));
		err = BCME_NOTREADY;
		goto done;
	}

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  preq->header.code,
		preq->header.length));

	if ((arg = p->beaconperiod)) {
		err = wlc_ioctl(wlc, WLC_SET_BCNPRD, &arg, sizeof(uint32), wlcif);
		if (err) {
			WL_NIN(("wl%d: %s :set bcn prd failed\n", wlc->pub->unit, __FUNCTION__));
			goto done;
		}
	}

	if ((arg = p->dtimperiod)) {
		err = wlc_ioctl(wlc, WLC_SET_DTIMPRD, &arg, sizeof(uint32), wlcif);
		if (err) {
			WL_NIN(("wl%d: %s :set dtim prd failed\n", wlc->pub->unit, __FUNCTION__));
			goto done;
		}
	}


	if (p->channel != 0 && (NBITVAL(p->channel) & NWM_UNALLOWED_CHANNEL_VECTOR)) {
		WL_NIN(("wl%d: %s : channel %d not allowed\n",
			wlc->pub->unit, __FUNCTION__, p->channel));
		err = BCME_ERROR;
		goto done;
	}
	/* zero means use default */
	if (p->channel) {
		int i;
		int maxentries = NWM_MAX_CHANNELS;
		uint32 *channels = (uint32 *)(pnin->buffer);
		wl_uint32_list_t * argch = (wl_uint32_list_t *)channels;
		bool found = FALSE;


		argch->count = WL_NUMCHANNELS;
		err = wlc_ioctl(wlc, WLC_GET_VALID_CHANNELS, argch, WL_NUMCHANNELS, wlcif);
		if (err) {
			WL_NIN(("wl%d: %s : get channels failed\n",
				wlc->pub->unit, __FUNCTION__));
			goto done;
		}

		for (i = 0;
			i < MIN(maxentries, argch->count) && argch->element[i] < maxentries;
			i++) {
			if (p->channel == argch->element[i]) {
				found = TRUE;
				break;
			}
		}
		if (!found) {
			WL_NIN(("wl%d: %s : channel %d not found\n",
				wlc->pub->unit, __FUNCTION__, p->channel));
			goto done;
		}

		arg = p->channel;
		err = wlc_ioctl(wlc, WLC_SET_CHANNEL, &arg, sizeof(arg), wlcif);
		if (err) {
			WL_NIN(("wl%d: %s : set channel %d failed\n",
				wlc->pub->unit, __FUNCTION__, p->channel));
			goto done;
		}

	}

	bzero(&rateset, sizeof(wl_rateset_t));
	rvector  = p->supp_rateset;
	brvector = p->basic_rateset;
	/* skip over if nothing specified */
	if (rvector != 0 && brvector != 0) {
		for (i = j = 0; i < ARRAYSIZE(nwm_rateconvtbl) && i < WL_MAXRATES_IN_SET; i++) {
			if (rvector & NBITVAL(i)) {
				rateset.rates[j] = nwm_rateconvtbl[i].rate;
				if (brvector & NBITVAL(i))
					rateset.rates[j] |= 0x80;
				j++;
			}
		}
		rateset.count = j;
		err = wlc_ioctl(wlc, WLC_SET_RATESET, &rateset, sizeof(rateset), wlcif);
		if (err) {
			WL_NIN(("wl%d: %s : set rateset supp 0x%x basic 0x%x failed\n",
				wlc->pub->unit, __FUNCTION__, p->supp_rateset, p->basic_rateset));
			goto done;
		}
	}

	/* push down the gameinfo only if we're a nitro parent
	 */
	{
		uint16 opmode;
		uint16 result;

		wlc_nin_get_modecmd(wlc, wlcif, &opmode, &result);
		if (result)  {
			WL_NIN(("wl%d: %s : get opmode failed\n",
				wlc->pub->unit, __FUNCTION__));
			goto done;

		}

		if (opmode == NWM_NIN_NITRO_PARENT_MODE && p->gameinfolength) {
			uint16 * pbuf = (uint16*)(pnin->buffer);

			int len = p->gameinfolength + sizeof(uint16);
			if (WL_NIN_ON()) {
				WL_NIN(("wl%d: gameinfo is:\n", wlc->pub->unit));
			}

			bcopy((char *)p->gameinfo, (char *)&pbuf[1], p->gameinfolength);
			pbuf[0] = p->gameinfolength;
			err = wlc_iovar_op(wlc, "gameinfo", NULL, 0, pbuf, len, IOV_SET, wlcif);
			if (err) {
				WL_NIN(("wl%d: %s : set gameinfo failed\n",
					wlc->pub->unit, __FUNCTION__));
				goto done;
			}
		}
	}

	ssidarg.SSID_len = p->ssidlength;
	bcopy(p->ssid, ssidarg.SSID, ssidarg.SSID_len);

	err = wlc_ioctl(wlc, WLC_SET_SSID, &ssidarg, sizeof(ssidarg), wlcif);
	if (err) {
		WL_NIN(("wl%d: %s: set ssid failed\n", wlc->pub->unit, __FUNCTION__));
		goto done;
	}

done:
	pcfm->resultCode = nwm_bcmerr_to_nwmerr(err);
	return TRUE;
}


static void
wl_rmreq_complete(void *arg)
{
	wlc_info_t *wlc = (wlc_info_t *)arg;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	char *pbuf = (char *)(pnin->buffer);
	void * newpkt = NULL;
	int buflen;
	nwm_mlme_measchan_ind_t *pind;
	uint8 stored_type = 0;
	int offset, first_loop;

	wl_rm_rep_t *rep_set;
	wl_rm_rep_elt_t rep;
	uint8* data;
	int err, bin;
	uint32 val;
	uint16 channel;
	bool aband;
	int len, interference_mode;

	WL_NIN(("wl%d : %s: Radio measurement complete\n",
		wlc->pub->unit, __FUNCTION__));

	err = wlc_iovar_op(wlc, "rm_rep", NULL, 0, pbuf,
		WLC_IOCTL_MAXLEN, IOV_GET, NULL);
	if (err) goto fatal_errdone;

	interference_mode = pnin->interference_mode;
	err = wlc_ioctl(wlc, WLC_SET_INTERFERENCE_MODE, &interference_mode,
	                sizeof(interference_mode), NULL);
	if (err) goto fatal_errdone;

	/* get a pkt big enough for whatever we might put into it */
	buflen = sizeof(nwm_mlme_measchan_ind_t);

	/* align */
	buflen += sizeof(uint32);
	newpkt = wlc_frame_get_ctl(wlc, buflen);
	if (!newpkt) {
		WL_ERROR(("%s: Out of memory for radio measurement results\n",
			__FUNCTION__));
		goto fatal_errdone;
	}
	bzero(PKTDATA(wlc->osh, newpkt), buflen);
	/* newpkt has the headroom but not necessarily the alignment we need */
	offset = sizeof(uint32) - (((uintptr)(PKTDATA(wlc->osh, newpkt))) & 0x3);
	offset &= 0x03;
	WL_NIN(("wl%d %s Alignment of newpkt is %d TXOFF is %d\n",
	        wlc->pub->unit, __FUNCTION__, offset, (int)TXOFF));
	PKTPULL(wlc->osh, newpkt, offset);

	/* Fill in the top level indication stuff */
	pind = (nwm_mlme_measchan_ind_t *)PKTDATA(wlc->osh, newpkt);
	pind->header.magic     = NWM_NIN_IND_MAGIC;
	pind->header.indcode   = NWM_CMDCODE_MLME_MEASCH_IND;
	pind->result_code = NWM_CMDRES_SUCCESS;


	/* Figure out what we've got and stuff it into the indication structure */
	rep_set = (wl_rm_rep_t *)pbuf;

	WL_NIN(("wl%d Measurement Report: token %d, length %d\n",
	wlc->pub->unit, rep_set->token, rep_set->len));

	len = rep_set->len;
	data = (uint8*)rep_set->rep;
	for (first_loop = 0; len > 0; (len -= rep.len), (data += rep.len)) {
		if (len >= WL_RM_REP_ELT_FIXED_LEN)
			bcopy(data, &rep, WL_RM_REP_ELT_FIXED_LEN);
		else
			break;

		data += WL_RM_REP_ELT_FIXED_LEN;
		len -= WL_RM_REP_ELT_FIXED_LEN;

		if (first_loop == 0) {
			stored_type = rep.type;
			switch (rep.type) {
			case WL_RM_TYPE_CCA:
				pind->type = NWM_MEASTYPE_CCA;
				pind->header.indlength = sizeof(pind->u.cca_results);
				break;

			case WL_RM_TYPE_RPI:
				pind->type = NWM_MEASTYPE_RPI;
				pind->header.indlength = sizeof(pind->u.rpi_results);
				break;

			default:
				WL_NIN(("wl%d %s: unrecognized measure type %d\n",
					wlc->pub->unit, __FUNCTION__, rep.type));
				goto fatal_errdone;
			} /* end switch */
			pind->header.indlength += NWM_MEASCHAN_IND_FIXED_LEN;
			pind->header.indlength /= 2;
		}

		if (stored_type != rep.type) {
		WL_NIN(("wl%d %s: inconsistent retrieved type old %d new %d\n",
			wlc->pub->unit, __FUNCTION__, stored_type, rep.type));
		err = BCME_ERROR;
		goto done;
		}

		if (rep.flags & (WL_RM_FLAG_LATE |
			WL_RM_FLAG_INCAPABLE |
			WL_RM_FLAG_REFUSED)) {
			WL_NIN(("wl%d %s: measurement not performed reason %d\n",
				wlc->pub->unit, __FUNCTION__, rep.flags));
			err = BCME_ERROR;
			goto done;
		}

		channel = CHSPEC_CHANNEL(rep.chanspec);
		aband = CHSPEC_IS5G(rep.chanspec);

		pind->chvector |= NBITVAL(channel);

		if (len < (int)rep.len) {
			WL_NIN(("wl%d Error: partial report element, %d report bytes "
			       "remain, element claims %d\n",
			       wlc->pub->unit, len, rep.len));
			err = BCME_ERROR;
			goto done;
		}

		if (rep.type == WL_RM_TYPE_BASIC) {
			if (rep.len >= 4) {
				bcopy(data, &val, sizeof(uint32));
			}
		} else if (rep.type == WL_RM_TYPE_CCA) {
			if (rep.len >= 4) {
				bcopy(data, &val, sizeof(uint32));
				/* copy to our cca array here */
				pind->u.cca_results[channel] = val;
			}
		} else if (rep.type == WL_RM_TYPE_RPI) {
			if (rep.len >= sizeof(wl_rm_rpi_rep_t)) {
				wl_rm_rpi_rep_t rpi_rep;
				bcopy(data, &rpi_rep, sizeof(wl_rm_rpi_rep_t));

				for (bin = 0; bin < WL_NUM_RPI_BINS; bin++) {
					/* copy to our rpi array here */
					pind->u.rpi_results[channel][bin] = rpi_rep.rpi[bin];
				}
			}
		} /* end type WL_RM_TYPE_RPI */
	} /* end for loop */
	/* normal termination */

done:
	if (err) {
		pind->result_code = NWM_CMDRES_FAILURE;
		WL_NIN(("wl%d %s: Non fatal ERROR with radio measurement results\n",
			wlc->pub->unit, __FUNCTION__));
	}
	wlc_nin_sendup(wlc, newpkt, NULL);
	return;

	/* fatal, no indication sent */
fatal_errdone:
	WL_NIN(("wl%d %s: Fatal ERROR with radio measurement results\n",
		wlc->pub->unit, __FUNCTION__));
	if (newpkt)
		PKTFREE(wlc->osh, newpkt, FALSE);
	return;
}
/*
	* CCA or RPI
	* Measure channel usage
*/
static bool wlc_mlme_measchan_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	int err, i, j, state, arg;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	wl_rm_req_t *pmrq = (wl_rm_req_t *)(pnin->buffer);
	nwm_mlme_measchan_req_t *p = (nwm_mlme_measchan_req_t *)pReq;
	uint8 type;
	int len = sizeof(wl_rm_req_t) +
		(NWM_MAX_CHANNELS * sizeof(wl_rm_req_elt_t));

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_CLASS1) {
		err = BCME_ASSOCIATED;
		goto done;
	}


	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	arg = INTERFERE_NONE;
	err = wlc_ioctl(wlc, WLC_SET_INTERFERENCE_MODE, &arg, sizeof(arg), wlcif);
	if (err < 0) {
		WL_ERROR(("Interference mode set error:%d\n", err));
		goto done;
	}

	bzero(pmrq, len);
	pmrq->cb = wl_rmreq_complete;
	pmrq->cb_arg = wlc;
	switch (p->testmode) {
		case 0:
			type = WL_RM_TYPE_CCA;
			break;
		case 1:
			type = WL_RM_TYPE_RPI;
			break;
		default:
			err = BCME_BADARG;
			goto done;
	}

	/* traverse channel list, set up one wl_rm_req_elt_t per channel */
	len = sizeof(wl_rm_req_t);
	/* bits 0,15 correspond to invalid, unused channels respectively */
	for (i = 1, j = 0; i < NWM_MAX_CHANNELS - 1; i++) {
		if (!(p->chvector & NBITVAL(i))) continue;

		len += sizeof(wl_rm_req_elt_t);
		pmrq->count = j + 1;
		pmrq->req[j].type = type;
		pmrq->req[j].dur = p->measure_time;
		pmrq->req[j].flags = 0;
		pmrq->req[j].chanspec = i;
		pmrq->req[j].chanspec |= WL_CHANSPEC_BW_20;
		pmrq->req[j].chanspec |= WL_CHANSPEC_BAND_2G;
		pmrq->req[j].token = j;
		j++;
	}

	if (j == 0) {
		err = BCME_BADARG;
		goto done;
	}

	err = wlc_iovar_op(wlc, "rm_req", NULL, 0, pmrq, len, IOV_SET, wlcif);

done:
	pCfm->resultCode = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
	return TRUE;
}


/* MA */
/*
 * Send a packet
 * Src, Dst macs provided
 */
static bool wlc_ma_data_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_ma_data_req_t *p = (nwm_ma_data_req_t *)pReq;
	nwm_ma_fatalerr_tx_ind_t errdat;

	/* get this now: if frame can't be enqueued it will be dropped
	 * and data will be unavailable
	 */
	errdat.frameid = p->frameid;
	errdat.errcode = NWM_CMDRES_FAILURE;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code,
		pReq->header.length));

	/* strip the Nintendo cmd header & frame id, what follows is an ethernet
	 * frame, hopefully properly addressed ...
	 */
	PKTPULL(wlc->osh, pkt, NWM_SEND_REQ_HDR);
	if (wlc_sendpkt(wlc, pkt, wlcif) == TRUE) {
		WL_NIN(("wl%d %s: failed to enqueue pkt, dropped\n",
			wlc->pub->unit, __FUNCTION__));
		wlc_nin_create_errind(wlc, (char *)&errdat, sizeof(errdat),
			wlcif, NWM_TX_ENQUEUE_ERR);
	}
	return FALSE;
}

/*
 * Already done in nitro
 * Phew
 */
static bool wlc_ma_keydata_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_ma_keydata_req_t *pkeyreq = (nwm_ma_keydata_req_t *)pReq;
	int len;
	void *pkeydat;
	int bcmerror;


	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	pkeydat = (void *)&pkeyreq->length;
	len = pkeyreq->length + WL_KEYDATAREQ_FIXEDLEN;

	bcmerror = wlc_iovar_op(wlc, "keydatareq", NULL, 0, pkeydat, len, IOV_SET, wlcif);
	if (bcmerror) {
		nwm_ma_fatalerr_tx_ind_t errdat;

		WL_NIN(("wl%d %s: keydatareq operation failed: ret value %d\n",
		wlc->pub->unit, __FUNCTION__, bcmerror));
		errdat.frameid = pkeyreq->keycmdhdr.frameid;
		errdat.errcode = nwm_bcmerr_to_nwmerr(bcmerror);
		wlc_nin_create_errind(wlc, (char *)&errdat, sizeof(errdat),
			wlcif, NWM_KEYDATAREQ_FAILURE);
	}
	PKTFREE(wlc->osh, pkt, TRUE);
	return FALSE;
}

/*
 * Already done in nitro
 * Phew
 */
static bool wlc_ma_mp_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_ma_mp_req_t *pmpreq = (nwm_ma_mp_req_t *)pReq;
	void *pmpdat;
	int len;
	int bcmerror;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	pmpdat = (void *)&pmpreq->resume;
	len = sizeof(nwm_ma_mp_req_t) - NWM_MP_REQ_HDR_SIZE + pmpreq->datalength - sizeof(uint16);

	bcmerror = wlc_iovar_op(wlc, "mpreq", NULL, 0, pmpdat, len, IOV_SET, wlcif);
	if (bcmerror) {
		nwm_ma_fatalerr_tx_ind_t errdat;

		WL_NIN(("wl%d %s: mpreq operation failed: ret value %d\n",
		wlc->pub->unit, __FUNCTION__, bcmerror));
		errdat.frameid = pmpreq->mpcmdhdr.frameid;
		errdat.errcode = nwm_bcmerr_to_nwmerr(bcmerror);
		wlc_nin_create_errind(wlc, (char *)&errdat, sizeof(errdat),
			wlcif, NWM_MPREQ_FAILURE);
	}

	PKTFREE(wlc->osh, pkt, TRUE);
	return FALSE;
}

/*
 * Unspecified testing purposes
 */
static bool wlc_ma_testdata_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	pCfm->resultCode = NWM_CMDRES_NOT_SUPPORT;
	return TRUE;
}

/*
 * Clear all queued transmit data
 * Other consequences?
 * Not sure how to do this
 */
static bool wlc_ma_clrdata_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	pCfm->resultCode = NWM_CMDRES_NOT_SUPPORT;
	return TRUE;
}


/* Set */
/*
 * Not really set "all", but set "many"
 * See subsequent individual set commands for details
 */
static bool wlc_paramset_all_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	int i, state;
	uint16 err;
	nwm_set_all_req_t *p = (nwm_set_all_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE && state != NWM_NIN_CLASS1) {
		err = BCME_ASSOCIATED;
		goto done;
	}
	wlc_nin_retrycmd(wlc, wlcif, &p->shortRetryLimit, &p->longRetryLimit,
	&err, FALSE);
	if (err) {
		WL_ERROR(("wl%d %s: error setting retry short %d long %d\n",
			wlc->pub->unit, __FUNCTION__, p->shortRetryLimit, p->longRetryLimit));
		goto done;
	}

	wlc_nin_set_modecmd(wlc, wlcif, p->opmode, &err);
	if (err) {
		WL_ERROR(("wl%d %s: error setting mode %d \n",
			wlc->pub->unit, __FUNCTION__, p->opmode));
		goto done;
	}

	wlc_nin_set_wepmodecmd(wlc, wlcif, p->wepmode, &err);
	if (err) {
		WL_ERROR(("wl%d %s: error setting wepmode %d \n",
			wlc->pub->unit, __FUNCTION__, p->wepmode));
		goto done;
	}


	/* wepmode MSUT be set before this ... */
	if (p->wepmode == NWM_NIN_WEP_40_BIT || p->wepmode == NWM_NIN_WEP_104_BIT) {
		for (i = 0; i < WSEC_MAX_DEFAULT_KEYS; i++) {
			wlc_nin_set_wepkey(wlc, wlcif, (uint8 *)&(p->wepKey[i][0]), i, &err);
			if (err) {
				WL_ERROR(("wl%d %s: error setting wepkey %d \n",
					wlc->pub->unit, __FUNCTION__, i));
				goto done;
			}
		}

		wlc_nin_wepkeyid(wlc, wlcif, &p->wepKeyId, &err, FALSE);
		if (err) {
			WL_ERROR(("wl%d %s: error setting wepkeyid %d \n",
				wlc->pub->unit, __FUNCTION__, p->wepKeyId));
			goto done;
		}
	}

	if (p->wepmode == NWM_NIN_WPA_PSK_TKIP ||
		p->wepmode == NWM_NIN_WPA2_PSK_AES) {
		wlc_nin_set_pmk(wlc, wlcif, p->wpa_passphrase, p->wpa_passphrase_len,
			&err);
		if (err) {
			WL_ERROR(("wl%d %s: error setting pmk \n",
				wlc->pub->unit, __FUNCTION__));
			goto done;
		}
	}

	wlc_nin_beaconfrmtype(wlc, wlcif, &p->beacontype, &err, FALSE);
	if (err) {
		WL_ERROR(("wl%d %s: error setting beacontype %d \n",
			wlc->pub->unit, __FUNCTION__, p->beacontype));
		goto done;
	}

	wlc_nin_prbresp(wlc, wlcif, &p->proberes, &err, FALSE);
	if (err) {
		WL_ERROR(("wl%d %s: error setting proberesponse %d \n",
			wlc->pub->unit, __FUNCTION__, p->proberes));
		goto done;
	}

	wlc_nin_bcntimeout(wlc, wlcif, &p->beaconlost_thr, &err, FALSE);
	if (err) {
		WL_ERROR(("wl%d %s: error setting bcn lost thr %d \n",
			wlc->pub->unit, __FUNCTION__, p->beaconlost_thr));
		goto done;
	}

	wlc_nin_ssidmask(wlc, wlcif, p->ssidmask, &err, FALSE);
	if (err) {
		WL_ERROR(("wl%d %s: error setting ssidmask \n",
			wlc->pub->unit, __FUNCTION__));
		goto done;
	}

	wlc_nin_preambleCmd(wlc, wlcif, &p->preambletype, &err, FALSE);
	if (err) {
		WL_ERROR(("wl%d %s: error setting preamble type %d \n",
			wlc->pub->unit, __FUNCTION__, p->preambletype));
		goto done;
	}

	wlc_nin_authalgocmd(wlc, wlcif, &p->authalgo, &err, FALSE);
	if (err) {
		WL_ERROR(("wl%d %s: error setting auth algo %d \n",
			wlc->pub->unit, __FUNCTION__, p->authalgo));
		goto done;
	}

done:
	pCfm->resultCode = nwm_bcmerr_to_nwmerr(err);
	return TRUE;
}

static void
wlc_nin_macaddr(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *macaddr,
	uint16 *result, bool get)
{
	int err, state;
	struct ether_addr *pmac = (struct ether_addr *)macaddr;

	if (get) {
		err = wlc_iovar_op(wlc, "cur_etheraddr", NULL, 0, pmac,
			ETHER_ADDR_LEN, IOV_GET, wlcif);
		goto done;
	}


	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE) {
		err = BCME_NOTDOWN;
		goto done;
	}
	err = wlc_iovar_op(wlc, "cur_etheraddr", NULL, 0, pmac,
		ETHER_ADDR_LEN, IOV_SET, wlcif);
done:
	*result = nwm_bcmerr_to_nwmerr(err);
}

/*
 * Testing only
 */
static bool wlc_paramset_macadrs_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_macadrs_req_t *p = (nwm_set_macadrs_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_macaddr(wlc, wlcif, p->macaddr, &pCfm->resultCode, FALSE);
	return TRUE;
}

/*
 * Set long & short retry
 *
 */

static void
wlc_nin_retrycmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *sharg,
	uint16 *lgarg, uint16 *result, bool get)
{
	int err;
	uint32 shlim, lglim;

	if (get) {
		err = wlc_ioctl(wlc, WLC_GET_SRL, &shlim, sizeof(int), wlcif);
		err |= wlc_ioctl(wlc, WLC_GET_LRL, &lglim, sizeof(int), wlcif);
		if (!err) {
			*sharg = shlim;
			*lgarg = lglim;
		}
		goto done;
	}

	shlim = *sharg;
	lglim = *lgarg;
	err = wlc_ioctl(wlc, WLC_SET_SRL, &shlim, sizeof(uint32), wlcif);
	err |= wlc_ioctl(wlc, WLC_SET_LRL, &lglim, sizeof(uint32), wlcif);

done:
	*result = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;


}

static bool wlc_paramset_retry_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_retrylimit_req_t * p = (nwm_set_retrylimit_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_retrycmd(wlc, wlcif, &p->shortRetryLimit, &p->longRetryLimit,
		&pCfm->resultCode, FALSE);
	return TRUE;
}


/*
 * Takes country input in usual form, e.g. "US"
 * Returns list of allowed channels in 16 bit vector
 * See table 5-1 of spec for details
 * WLC_SET_COUNTRY
 * WLC_GET_VALID_CHANNELS
 */
static void
wlc_nin_set_countrycmd(wlc_info_t *wlc, struct wlc_if *wlcif, char *country,
	uint16 *chanvector, uint16 *result)
{
	int err, i, state, arg;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	uint32 *channels = (uint32 *)(pnin->buffer);
	uint32 countrybuf;

	wl_uint32_list_t * argch = (wl_uint32_list_t *)channels;
	int maxentries = NWM_MAX_CHANNELS;

	*chanvector = 0;
	argch->count = WL_NUMCHANNELS;

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state >  NWM_NIN_CLASS1) {
		*result = NWM_CMDRES_ILLEGAL_MODE;
		return;
	}


	/* insurance */
	arg = 0;
	err = wlc_ioctl(wlc, WLC_DOWN, &arg, sizeof(arg), wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: WLC_DOWN error %d\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	country[3] = 0;
	bcopy(country, &countrybuf, sizeof(countrybuf));
	err = wlc_ioctl(wlc, WLC_SET_COUNTRY, &countrybuf, WLC_CNTRY_BUF_SZ, wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: WLC_SET_COUNTRY error %d\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	err = wlc_ioctl(wlc, WLC_GET_VALID_CHANNELS, argch, WL_NUMCHANNELS, wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: WLC_GET_VALID_CHANNELS error %d\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	for (i = 0; i < MIN(maxentries, argch->count) && argch->element[i] < maxentries; i++) {
		*chanvector |= NBITVAL(argch->element[i]);
	}

done:
	*result = nwm_bcmerr_to_nwmerr(err);
}
static bool wlc_paramset_enablechannel_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	char * argstr = (char *)((nwm_set_enablechannel_req_t *)pReq)->countrystring;
	nwm_set_enablechannel_cfm_t * p = (nwm_set_enablechannel_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_set_countrycmd(wlc, wlcif, argstr, &p->enableChannel, &p->resultCode);
	return TRUE;
}


/*
 * Test, (nitro) Parent, (nitro) child, infrastructure,
 * ad-hoc, AP, TravelRouter
 * Some far-reaching consequences for TravelRouter setting ...
 */
static
struct {
	int ap, infra, nitro, apsta;
} modetbl[] = {
	{0,0,0,0},	/* test */
	{1,1,1,0},	/* nitro parent */
	{0,1,2,0},	/* nitro child  */
	{0,1,0,0},	/* infra sta */
	{0,0,0,0},	/* ad hoc sta */
	{1,1,0,0},	/* ap    */
	{1,1,0,1},	/* ure    */
};
static void
wlc_nin_set_modecmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint32 mode, uint16 *result)
{
	int err = 0;
	int nitroarg, aparg, infrarg;
	int state;
	int arg;
	int i = mode;
	/*
	 * 0000h: invalid, not used
	 * 0001h: Parent Mode
	 * 0002h: Child Mode
	 * 0003h:  STA Infrastructure Mode  (default)
	 * 0004h:  STA Ad hoc mode
	 * 0005h: AP mode
	 * 0006h: Travel Router
	 *
	 *
	 */

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_CLASS1 && state != NWM_NIN_IDLE) {
		err = BCME_ASSOCIATED;
		goto done;
	}
	if (i >= ARRAYSIZE(modetbl) || i < 1) {
		err = BCME_BADARG;
		goto done;
	}

	nitroarg = modetbl[i].nitro;
	aparg    = modetbl[i].ap;
	infrarg  = modetbl[i].infra;
	WL_NIN(("wl%d %s: nitroarg %d aparg %d infrarg %d mode %d\n",
		wlc->pub->unit, __FUNCTION__, nitroarg, aparg, infrarg, i));


	arg = 0;
	err = wlc_ioctl(wlc, WLC_DOWN, &arg, sizeof(arg), wlcif);
	if (err) {
		WL_ERROR(("wl%d %s : error setting DOWN err %dn",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	err = wlc_iovar_setint(wlc, "nitro", nitroarg);
	if (err) {
		WL_ERROR(("wl%d %s : error setting nitro mode err %dn",
		wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	err = wlc_ioctl(wlc, WLC_SET_AP, &aparg, sizeof(aparg), wlcif);
	if (err) {
		WL_ERROR(("wl%d %s : error setting ap mode err %dn",
		wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	err = wlc_ioctl(wlc, WLC_SET_INFRA, &infrarg, sizeof(infrarg), wlcif);
	if (err) {
		WL_ERROR(("wl%d %s : error setting infra mode err %dn",
		wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	if (state == NWM_NIN_CLASS1) {
		err = wlc_ioctl(wlc, WLC_UP, &arg, sizeof(arg), wlcif);
		if (err) {
			WL_ERROR(("wl%d %s : error setting WLC_UP err %dn",
				wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}
	}

done:
	*result = nwm_bcmerr_to_nwmerr(err);
	return;
}

static bool wlc_paramset_mode_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_mode_req_t * p = (nwm_set_mode_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_set_modecmd(wlc, wlcif, p->mode, &pCfm->resultCode);

	return TRUE;
}


static void
wlc_nin_set_ratesetcmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *srate,
	uint16 *brate, uint16 *result)
{
	int err, i, j, nitroarg;
	wl_rateset_t rateset;
	uint16 svector = *srate;
	uint16 bvector = *brate;
	int state;

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE) {
		err = BCME_NOTDOWN;
		goto done;
	}

	bzero((char *)&rateset, sizeof(wl_rateset_t));
	err = wlc_iovar_getint(wlc, "nitro", &nitroarg);
	if (err)
		goto done;


	for (i = j = 0; i < ARRAYSIZE(nwm_rateconvtbl) && i < WL_MAXRATES_IN_SET; i++) {
		if (svector & NBITVAL(i)) {
			rateset.rates[j] = nwm_rateconvtbl[i].rate;
			if (bvector & NBITVAL(i))
				rateset.rates[j] |= 0x80;
			j++;
		}
	}
	rateset.count = j;
	err = wlc_ioctl(wlc, WLC_SET_RATESET, &rateset, sizeof(rateset), wlcif);

done:
	*result = nwm_bcmerr_to_nwmerr(err);
}
static bool wlc_paramset_rate_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_rateset_req_t *p = (nwm_set_rateset_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_set_ratesetcmd(wlc, wlcif, &p->supprate, &p->basicrate, &pCfm->resultCode);
	return TRUE;
}

/*
 * encryption modes
 */
static
struct {
	int wparg, wsecarg;
} wlc_nin_secmodetbl[] = {
	{0,0},								/* OFF */
	{WPA_AUTH_DISABLED, WEP_ENABLED},	/* 40 bit wep */
	{WPA_AUTH_DISABLED, WEP_ENABLED},	/* 104 bit wep */
	{0, 0},								/* Reserved, set OFF */
	{WPA_AUTH_PSK, TKIP_ENABLED },		/* WPA-PSK-TKIP, sta only */
	{WPA2_AUTH_PSK, AES_ENABLED },		/* WPA2-PSK-AES, sta only */
	{WPA_AUTH_PSK, AES_ENABLED },		/* WPA-PSK-AES, sta only */
};

static void
wlc_nin_set_wepmodecmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 wepmode, uint16 *result)
{
	int err;
	uint32 wparg, wsecarg, index;
	wl_wsec_key_t keyinfo;
	int state;
	int sup_val = 0;

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE && state != NWM_NIN_CLASS1) {
		err = BCME_ASSOCIATED;
		goto done;
	}

	if (wepmode >= ARRAYSIZE(wlc_nin_secmodetbl) || wepmode == NWM_NIN_WSEC_RESERVED) {
		err = BCME_BADARG;
		goto done;
	}

	wparg = wlc_nin_secmodetbl[wepmode].wparg;
	wsecarg = wlc_nin_secmodetbl[wepmode].wsecarg;

	err = wlc_ioctl(wlc, WLC_SET_WPA_AUTH, &wparg, sizeof(wparg), wlcif);
	if (err) {
		WL_ERROR((
			"wl%d: %s: failed to set WLC_SET_WPA_AUTH err %d wparg %d\n",
			wlc->pub->unit, __FUNCTION__, err, wparg));
		goto done;
	}

	err = wlc_ioctl(wlc, WLC_SET_WSEC, &wsecarg, sizeof(wsecarg), wlcif);
	if (err) {
		WL_ERROR((
			"wl%d: %s: failed to set WLC_SET_WSEC err %d wsecarg %d\n",
			wlc->pub->unit, __FUNCTION__, err, wsecarg));
		goto done;
	}

	/* wpa(2)-psk-{tkip,aes} is sta mode only, set sup_wpa */
	if (wepmode == NWM_NIN_WPA_PSK_TKIP || wepmode == NWM_NIN_WPA2_PSK_AES ||
		wepmode == NWM_NIN_WPA_PSK_AES)
		sup_val = 1;
	err = wlc_iovar_setint(wlc, "sup_wpa", sup_val);
	if (err) {
		WL_ERROR(("wl%d: %s: failed to set sup_wpa to %d err %d\n",
			wlc->pub->unit, __FUNCTION__, sup_val, err));
		goto done;
	}


	/* default keys to all zeroes, length according to wep mode */
	bzero(&keyinfo, sizeof(keyinfo));
	switch (wepmode) {
		case NWM_NIN_NO_ENCRYPTION :
		case NWM_NIN_WPA_PSK_TKIP :
		case NWM_NIN_WPA2_PSK_AES :
		case NWM_NIN_WPA_PSK_AES :
			err = 0;
			goto done;

		case NWM_NIN_WEP_40_BIT :
			keyinfo.algo = CRYPTO_ALGO_WEP1;
			keyinfo.len = 5;
			break;

		case NWM_NIN_WEP_104_BIT :
			keyinfo.algo = CRYPTO_ALGO_WEP128;
			keyinfo.len = 13;
			break;

		default :
			err = BCME_BADARG;
			goto done;
	}

	for (index = 0; index < WSEC_MAX_DEFAULT_KEYS; index++) {
		keyinfo.index = index;
		err = wlc_ioctl(wlc, WLC_SET_KEY, &keyinfo, sizeof(keyinfo), wlcif);
		if (err) {
			WL_ERROR(("wl%d: %s: failed to set key index %d err %d\n",
				wlc->pub->unit, __FUNCTION__, index, err));
			goto done;
		}
	}
	/* set primary key to zero */
	index = 0;
	err = wlc_ioctl(wlc, WLC_SET_KEY_PRIMARY, &index, sizeof(index), wlcif);
	if (err) {
		WL_ERROR(("wl%d: %s: failed to set primary key index err %d\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

done:
	*result = nwm_bcmerr_to_nwmerr(err);
}

static bool wlc_paramset_wepmode_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_wepmode_req_t *p = (nwm_set_wepmode_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_set_wepmodecmd(wlc, wlcif, p->wepmode, &pCfm->resultCode);
	return TRUE;
}

/*
 * Index for default key
 * 0-3 corresponding to wep keys 0-3
 */
static void
wlc_nin_wepkeyid(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *wepkeyid, uint16 *result, bool get)
{
	int err;
	uint32 arg, i;
	int state;

	/* This is one for Ripley:
	 * We have to submit a "guess".
	 * If the returned result is 1, we were correct,
	 * otherwise try again.
	 * AAARRRGGGHHH!!!
	 *
	 */
	if (get) {
		for (i = 0; i < WSEC_MAX_DEFAULT_KEYS; i++) {
			arg = i;
			err = wlc_ioctl(wlc, WLC_GET_KEY_PRIMARY, &arg, sizeof(arg), wlcif);
			WL_NIN(("wl%d %s: err %d arg %d\n",
			wlc->pub->unit, __FUNCTION__, err, arg));
			if (err)
				goto done;

			if (arg) {
				*wepkeyid = i;
				goto done;
			}
		}
		/* None set: error */
		err = BCME_NOTFOUND;
		goto done;
	}

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE && state != NWM_NIN_CLASS1) {
		err = BCME_ASSOCIATED;
		goto done;
	}

	arg = *wepkeyid;
	err = wlc_ioctl(wlc, WLC_SET_KEY_PRIMARY, &arg, sizeof(arg), wlcif);

done:
	*result = nwm_bcmerr_to_nwmerr(err);
	WL_NIN(("wl%d %s: exiting, err %d wepkeyid %d result %d\n",
		wlc->pub->unit, __FUNCTION__, err, *wepkeyid, *result));
}
static bool wlc_paramset_wepkeyid_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_wepkeyid_req_t *p = (nwm_set_wepkeyid_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x \n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_wepkeyid(wlc, wlcif, &p->wepKeyId, &pCfm->resultCode, FALSE);

	return TRUE;
}

/*
 * Set all 4 wep keys
 * Key length determined by wep mode
 */
static void
wlc_nin_set_wepkey(wlc_info_t *wlc, struct wlc_if *wlcif, uint8 *keydata, int index, uint16 *result)
{
	wl_wsec_key_t keyinfo;
	int err;
	uint16 wepmode, wepresult;
	int state;


	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE && state != NWM_NIN_CLASS1) {
		err = BCME_ASSOCIATED;
		goto done;
	}

	bzero(&keyinfo, sizeof(keyinfo));
	wlc_nin_get_wepmodecmd(wlc, wlcif, &wepmode, &wepresult);
	if (wepresult != NWM_CMDRES_SUCCESS) {
		err = BCME_ERROR;
		goto done;
	}

	/* make sure you set wepmode first */
	switch (wepmode) {
		case NWM_NIN_WEP_40_BIT :
			keyinfo.algo = CRYPTO_ALGO_WEP1;
			keyinfo.len = 5;
			break;

		case NWM_NIN_WEP_104_BIT :
			keyinfo.algo = CRYPTO_ALGO_WEP128;
			keyinfo.len = 13;
			break;

		default :
			err = BCME_BADARG;
			goto done;
	}

	bcopy(keydata, keyinfo.data, keyinfo.len);
	keyinfo.index = index;

	err = wlc_ioctl(wlc, WLC_SET_KEY, &keyinfo, sizeof(keyinfo), wlcif);
	if (err) {
		WL_ERROR(("wl%d: %s: failed to set wep keys\n",
			wlc->pub->unit, __FUNCTION__));
		goto done;
	}

done:
	*result = nwm_bcmerr_to_nwmerr(err);
}
static bool wlc_paramset_wepkey_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	int i;
	uint16 index;
	nwm_set_wepkey_req_t *p = (nwm_set_wepkey_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	for (i = 0; i < WSEC_MAX_DEFAULT_KEYS; i++) {
		wlc_nin_set_wepkey(wlc, wlcif, (uint8 *)&(p->wepKey[i][0]), i, &pCfm->resultCode);
		if (pCfm->resultCode != NWM_CMDRES_SUCCESS) {
			WL_ERROR(("wl%d: %s: failed to set key at index %d\n",
				wlc->pub->unit, __FUNCTION__, i));
			goto done;
		}
	}

	/* default: set primary key to zero */
	index = 0;
	wlc_nin_wepkeyid(wlc, wlcif, &index, &pCfm->resultCode, FALSE);

done:
	return TRUE;
}

/*
 * sets/gets broadcast or no broadcast SSID
 *
 */
static void
wlc_nin_beaconfrmtype(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *frmtype, uint16 *result, bool get)
{
	int err;
	int arg;

	if (get) {
		err = wlc_iovar_getint(wlc, "nobcnssid", &arg);
		if (!err)
			*frmtype = (uint16)arg;
		goto done;
	}

	err = wlc_iovar_setint(wlc, "nobcnssid", *frmtype);

done:
	*result = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
}
static bool wlc_paramset_beacontype_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_beacontype_req_t *p = (nwm_set_beacontype_req_t *)preq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));

	wlc_nin_beaconfrmtype(wlc, wlcif, &p->beacontype, &pcfm->resultCode, FALSE);

	return TRUE;
}

/*
 * sets respond or no respond to broadcast probe req
 *
 */
static void
wlc_nin_prbresp(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *proberesp, uint16 *result, bool get)
{
	int err;
	int arg;

	if (get) {
		err = wlc_iovar_getint(wlc, "nobcprbresp", &arg);
		if (!err)
			*proberesp = (uint16)arg;
		goto done;
	}

	err = wlc_iovar_setint(wlc, "nobcprbresp", *proberesp);

done:
	*result = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
}

static bool wlc_paramset_resbcssid_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_proberes_req_t *p = (nwm_set_proberes_req_t *)preq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));

	wlc_nin_prbresp(wlc, wlcif, &p->proberes, &pcfm->resultCode, FALSE);

	return TRUE;
}

/*
 * Threshold to send beacon lost indication
 * 0-255
 * zero -- never
 */
static void
wlc_nin_bcntimeout(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *bcntmout, uint16 *result, bool get)
{
	int err;
	int arg;

	if (get) {
		err = wlc_iovar_getint(wlc, "bcn_thresh", &arg);
		if (!err)
			*bcntmout = (uint16)arg;
		goto done;
	}

	if (*bcntmout > NWM_BCN_THRESH_MAX) {
		err = BCME_BADARG;
		goto done;
	}
	err = wlc_iovar_setint(wlc, "bcn_thresh", *bcntmout);

done:
	*result = nwm_bcmerr_to_nwmerr(err);
}

static bool wlc_paramset_beaconlostth_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_beaconlostth_req_t *p = (nwm_set_beaconlostth_req_t *)preq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));

	wlc_nin_bcntimeout(wlc, wlcif, &p->beaconlost_thr, &pcfm->resultCode, FALSE);

	return TRUE;
}

/*
 * Set active zone value, used in GameInfo element
 *
 */
static void
wlc_nin_activezone(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *actzone, uint16 *result, bool get)
{
	int err;

	if (get) {
		int arg;
		err = wlc_iovar_getint(wlc, "activezone", &arg);
		*actzone = (uint16)arg;
	} else {
		err = wlc_iovar_setint(wlc, "activezone", *actzone);
	}

	*result = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
}
static bool wlc_paramset_activezone_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_activezone_req_t *p = (nwm_set_activezone_req_t *)preq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));
	wlc_nin_activezone(wlc, wlcif, &p->active_zone_time, &pcfm->resultCode, FALSE);
	return TRUE;
}

static bool wlc_paramget_activezone_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_activezone_cfm_t *p = (nwm_get_activezone_cfm_t *)pcfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));
	wlc_nin_activezone(wlc, wlcif, &p->active_zone_time, &pcfm->resultCode, TRUE);
	return TRUE;
}
/*
 * Just like it says, set the SSID mask, 32 bytes long
 */
static void
wlc_nin_ssidmask(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *ssidmask,
	uint16 *result, bool get)
{
	int err, state;

	if (get) {
		err = wlc_iovar_op(wlc, "ssidmask", NULL, 0, ssidmask,
			NWM_SSIDMASK_LEN, IOV_GET, wlcif);
		goto done;
	}

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE && state != NWM_NIN_CLASS1) {
		err = BCME_ASSOCIATED;
		goto done;
	}
	err = wlc_iovar_op(wlc, "ssidmask", NULL, 0, ssidmask,
		NWM_SSIDMASK_LEN, IOV_SET, wlcif);
done:
	*result = nwm_bcmerr_to_nwmerr(err);
}
static bool wlc_paramset_ssidmask_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_ssidmask_req_t *p = (nwm_set_ssidmask_req_t *)preq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));
	wlc_nin_ssidmask(wlc, wlcif, p->ssidmask, &pcfm->resultCode, FALSE);

	return TRUE;
}

/*
 * Set PLCP header (preamble): short or long
 * 0 - long, 1 - short
 * WLC_SET_PLCPHDR maps nicely (except for auto case)
 * There's no problem...

 * - All 1Mbps frames use long preamble.  That's the law.
 * - All Nitro frames use 2Mbps, so we can use whatever preamble they set.

 * Ray
 *
 */
static void
wlc_nin_preambleCmd(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *preamble, uint16 *result, bool get)
{
	int err;
	uint32 arg;

	if (get) {
		err = wlc_ioctl(wlc, WLC_GET_PLCPHDR, &arg, sizeof(arg), wlcif);
		if (!err) {
			switch (arg) {
				case WLC_PLCP_AUTO :
					*preamble = 0;
					break;
				case WLC_PLCP_SHORT :
					*preamble = 1;
					break;
				default:
					err = BCME_BADARG;
					goto done;
			}
		}
		goto done;
	}

	switch (*preamble) {
		case 0 :
			arg = WLC_PLCP_AUTO;
			break;
		case 1 :
			arg = WLC_PLCP_SHORT;
			break;
		default:
			err = BCME_BADARG;
			goto done;
	}
	err = wlc_ioctl(wlc, WLC_SET_PLCPHDR, &arg, sizeof(arg), wlcif);

done:
	*result = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
}
static bool wlc_paramset_preambletype_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_preambletype_req_t *p = (nwm_set_preambletype_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_preambleCmd(wlc, wlcif, &p->type, &pCfm->resultCode, FALSE);
	return TRUE;
}

/*
 * For pre-association authorization
 * 0: open system
 * 1: shared key
 *
 */
static void
wlc_nin_authalgocmd(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *authtype, uint16 *result, bool get)
{
	int err;
	uint32 arg;
	int state;

	if (get) {
		err = wlc_ioctl(wlc, WLC_GET_AUTH, &arg, sizeof(arg), wlcif);
		if (!err)
			*authtype = arg;
		goto done;
	}

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE && state != NWM_NIN_CLASS1) {
		err = BCME_ASSOCIATED;
		goto done;
	}

	switch (*authtype) {
		case 0 :
		case 1 :
			arg = *authtype;
			break;

		default :
			err = BCME_BADARG;
			goto done;
	}
	err = wlc_ioctl(wlc, WLC_SET_AUTH, &arg, sizeof(arg), wlcif);

done:
	*result = nwm_bcmerr_to_nwmerr(err);
}
static bool wlc_paramset_authalgo_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_authalgo_req_t *p = (nwm_set_authalgo_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_authalgocmd(wlc, wlcif, &p->type, &pCfm->resultCode, FALSE);
	return TRUE;
}

static void
wlc_set_lifetime_cmd(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 mtime, uint16 ntime, uint16 frame_lifetime, uint16 *result)
{
	int i, err;
	uint32 marg;
	wl_lifetime_t lifetime;
	int state;

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE && state != NWM_NIN_CLASS1) {
		err = BCME_ASSOCIATED;
		goto done;
	}
	/* AC_COUNT # of classes, make them all equal */
	lifetime.lifetime = frame_lifetime;
	for (i = 0; i < AC_COUNT; i++) {
		lifetime.ac = i;
		err = wlc_iovar_op(wlc, "lifetime", NULL, 0,
			&lifetime, sizeof(lifetime), IOV_SET, wlcif);
		if (err) {
			WL_NIN(("wl%d %s: lifetime iovar set failed for ac %d val %d\n",
				wlc->pub->unit, __FUNCTION__, i, frame_lifetime));
			goto done;
		}
	}

	/* these values have no use in sta */
	if (AP_ENAB(wlc->pub)) {
		err = wlc_iovar_setint(wlc, "scb_activity_time", ntime);
		if (err) {
			WL_NIN(("wl%d %s: scb_activity_time iovar set failed val %d\n",
				wlc->pub->unit, __FUNCTION__, ntime));
			goto done;
		}

		/* pesky uint16 --> uint32, we're passing a pointer */
		marg = mtime;
		err = wlc_ioctl(wlc, WLC_SET_SCB_TIMEOUT, &marg, sizeof(marg), wlcif);
		if (err) {
			WL_NIN(("wl%d %s: WLC_SET_SCB_TIMEOUT ioctl set failed val %d\n",
				wlc->pub->unit, __FUNCTION__, marg));
			goto done;
		}
	}

done:
	*result = nwm_bcmerr_to_nwmerr(err);
}

/*
 * Frame lifetime AND association timeout
 *
 */
static bool wlc_paramset_lifetime_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_lifetime_req_t *p = (nwm_set_lifetime_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_set_lifetime_cmd(wlc, wlcif, p->Mtime, p->Ntime, p->frameLifeTime,
		&pCfm->resultCode);

	return TRUE;
}

/*
 * Set max number of connected child stations (nitro)
 * Also applies to number of associated STA's when we're
 * a regular 802.11 AP
 */
static void
wlc_nin_maxassoc(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *maxassoc,
	uint16 *result, bool get)
{
	int err, state, max;

	if (get) {
		err = wlc_iovar_getint(wlc, "maxassoc", &max);
		if (err)
			goto done;

		*maxassoc = max;
		goto done;
	}

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE && state != NWM_NIN_CLASS1) {
		WL_ERROR(("wl%d %s: illegal state %d\n",
			wlc->pub->unit, __FUNCTION__, state));
		err = BCME_ASSOCIATED;
		goto done;
	}
	err = wlc_iovar_setint(wlc, "maxassoc", *maxassoc);
	if (err)
		WL_ERROR(("wl%d %s: failed to set maxassoc to %d err %d\n",
			wlc->pub->unit, __FUNCTION__, *maxassoc, err));

done:
	*result = nwm_bcmerr_to_nwmerr(err);

}
static bool wlc_paramset_maxconn_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_maxconn_req_t *p = (nwm_set_maxconn_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_maxassoc(wlc, wlcif, &p->count, &pCfm->resultCode, FALSE);
	return TRUE;
}

/* Set/Get Tx antenna */
static void
wlc_nin_txant(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *useantenna,
	uint16 *result, bool get)
{
	int arg, err;

	if (get) {
		err = wlc_ioctl(wlc, WLC_GET_TXANT, &arg, sizeof(arg), wlcif);
		if (err) {
			WL_ERROR(("wl%d: %s: WLC_GET_TXANT failed err %d\n",
				wlc->pub->unit, __FUNCTION__, err));
		}
		else
			*useantenna = arg;
		goto done;
	}

	arg = *useantenna;
	err = wlc_ioctl(wlc, WLC_SET_TXANT, &arg, sizeof(arg), wlcif);
	if (err) {
		WL_ERROR(("wl%d: %s: WLC_SET_TXANT failed err %d arg %d\n",
			wlc->pub->unit, __FUNCTION__, err, arg));
	}

done:
	*result = nwm_bcmerr_to_nwmerr(err);
	return;
}

static bool wlc_paramset_txant_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_txant_req_t *p = (nwm_set_txant_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_txant(wlc, wlcif, &p->useantenna, &pCfm->resultCode, FALSE);
	return TRUE;
}

/*
 * A subset of WLC_SET_ANTDIV settings?
 * BRCM action: adapt ours to theirs (binary: main only/sub only)
 */
static void
wlc_nin_set_diversity(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 diversity,
	uint16 useantenna, uint16 *result)
{
	int err;
	uint32 arg;

	if (diversity == 0)
		arg = useantenna;
	else
		arg = 3;

	err = wlc_ioctl(wlc, WLC_SET_ANTDIV, &arg, sizeof(arg), wlcif);

	*result = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;

}
static bool wlc_paramset_diversity_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_diversity_req_t *p = (nwm_set_diversity_req_t *)pReq;
	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_set_diversity(wlc, wlcif, p->diversity, p->useantenna, &pCfm->resultCode);
	return TRUE;
}


static void
wlc_nin_bcnind_enable(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *rcven, uint16 *snden, uint16 *result, bool get)
{
	int err;
	uint8 bitvec[WL_EVENTING_MASK_LEN];

	err = wlc_iovar_op(wlc, "event_msgs", NULL, 0,
		bitvec, sizeof(bitvec), IOV_GET, wlcif);
	if (err) {
		*result = NWM_CMDRES_FAILURE;
		return;
	}

	if (get) {
		*rcven = isset(bitvec, WLC_E_BCNRX_MSG) ? 1 : 0;
		*snden = isset(bitvec, WLC_E_BCNSENT_IND) ? 1 : 0;
		*result = NWM_CMDRES_SUCCESS;
		return;
	}

	if (*snden)
		setbit(bitvec, WLC_E_BCNSENT_IND);
	else
		clrbit(bitvec, WLC_E_BCNSENT_IND);

	if (*rcven)
		setbit(bitvec, WLC_E_BCNRX_MSG);
	else
		clrbit(bitvec, WLC_E_BCNRX_MSG);

	err = wlc_iovar_op(wlc, "event_msgs", NULL, 0,
		bitvec, sizeof(bitvec), IOV_SET, wlcif);

	*result = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
}
/*
 * Two flags:
 * enable send of indication upon beacon receipt
 * enable send of indication upon beacon transmission
 * 0 - don't, 1 - do
 */
static bool wlc_paramset_bcnsendrecvind_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_beaconsendrecvind_req_t *p = (nwm_set_beaconsendrecvind_req_t *)pReq;
	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_bcnind_enable(wlc, wlcif, &p->enable_recv, &p->enable_send,
		&pCfm->resultCode, FALSE);
	return TRUE;
}

/* Set/Get Interference mode */
static void
wlc_nin_interference(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *interference,
	uint16 *result, bool get)
{
	int arg, err;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);

	if (get) {
		err = wlc_ioctl(wlc, WLC_GET_INTERFERENCE_MODE, &arg, sizeof(arg), wlcif);
		if (err) {
			WL_ERROR(("wl%d: %s: WLC_GET_INTERFERENCE_MODE, failed err %d\n",
				wlc->pub->unit, __FUNCTION__, err));
		}
		else
			*interference = arg & ~AUTO_ACTIVE;
		goto done;
	}

	arg = *interference;
	err = wlc_ioctl(wlc, WLC_SET_INTERFERENCE_MODE, &arg, sizeof(arg), wlcif);
	if (err) {
		WL_ERROR(("wl%d: %s: WLC_SET_INTERFERENCE_MODE failed err %d arg %d\n",
			wlc->pub->unit, __FUNCTION__, err, arg));
	}
	else
		pnin->interference_mode = arg;	/* save the config */

done:
	*result = nwm_bcmerr_to_nwmerr(err);
	return;
}

static bool wlc_paramset_interference_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_interference_req_t *p = (nwm_set_interference_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_interference(wlc, wlcif, &p->interference, &pCfm->resultCode, FALSE);
	return TRUE;
}

/*
 * Set BSSID, for testing only
 */
static bool wlc_paramset_bssid_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	pCfm->resultCode = NWM_CMDRES_NOT_SUPPORT;
	return TRUE;
}


/*
 * Set beacon period 10-1000 milliseconds
 */
static void
wlc_nin_bcnprd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *bcnint,
	uint16 *result, bool get)
{
	int err;
	uint32 arg;

	if (get) {
		err = wlc_ioctl(wlc, WLC_GET_BCNPRD, &arg, sizeof(arg), wlcif);
		if (!err)
			*bcnint = arg;
		goto done;
	}
	arg = *bcnint;
	err = wlc_ioctl(wlc, WLC_SET_BCNPRD, &arg, sizeof(arg), wlcif);

done:
	*result = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
}
static bool wlc_paramset_beaconperiod_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_beaconperiod_req_t *p = (nwm_set_beaconperiod_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_bcnprd(wlc, wlcif, &p->beaconPeriod, &pCfm->resultCode, FALSE);
	return TRUE;
}

/*
 * set DTIM 1-255 beacon intervals
 */
static void
wlc_nin_dtimprd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *dtim,
	uint16 *result, bool get)
{
	int err;
	uint32 arg;

	if (get) {
		err = wlc_ioctl(wlc, WLC_GET_DTIMPRD, &arg, sizeof(arg), wlcif);
		if (!err)
			*dtim = arg;
		goto done;
	}

	arg = *dtim;
	err = wlc_ioctl(wlc, WLC_SET_DTIMPRD, &arg, sizeof(arg), wlcif);

done:
	*result = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
}
static bool wlc_paramset_dtimperiod_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_dtimperiod_req_t *p = (nwm_set_dtimperiod_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_dtimprd(wlc, wlcif, &p->dtimPeriod, &pCfm->resultCode, FALSE);
	return TRUE;
}


/*
 * Game info is written to Nintendo IE in beacon
 */
static void
wlc_nin_gameinfo(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *gameinfo,
	uint16 *length, uint16 *result, bool get)
{
	int err;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	uint16 * p = (uint16*)(pnin->buffer);
	int len = *length + sizeof(uint16);

	if (!get) {
		bcopy((char *)gameinfo, (char *)&p[1], *length);
		p[0] = *length;
		err = wlc_iovar_op(wlc, "gameinfo", NULL, 0, p, len, IOV_SET, wlcif);
	} else {
		err = wlc_iovar_op(wlc, "gameinfo", NULL, 0, p, len, IOV_GET, wlcif);
		if (!err) {
			if (*length < p[0]) {
				*result = NWM_CMDRES_NOT_ENOUGH_MEM;
				*length = p[0];
				return;
			}
			*length = p[0];
			bcopy((char *)&p[1], (char *)&gameinfo[0], *length);
		}
	}

	if (err) {
		WL_NIN(("wl%d %s err %d\n", wlc->pub->unit, __FUNCTION__, err));
		*result = NWM_CMDRES_FAILURE;
	} else {
		*result = NWM_CMDRES_SUCCESS;
	}

}
static bool wlc_paramset_gameinfo_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_gameinfo_req_t *p = (nwm_set_gameinfo_req_t *)preq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));
	wlc_nin_gameinfo(wlc, wlcif, p->gameinfo, &p->game_info_length, &pcfm->resultCode, FALSE);

	return TRUE;
}

static void
wlc_nin_vblanktsf(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *vblank,
	uint16 *result, bool get)
{
	int err;
	int arg;


	if (get) {
		err = wlc_iovar_getint(wlc, "vblanktsf", &arg);
	} else {
		err = wlc_iovar_setint(wlc, "vblanktsf", *vblank);
	}

	if (err) {
		*result = NWM_CMDRES_FAILURE;
		WL_NIN(("wl%d %s: err %d\n", wlc->pub->unit, __FUNCTION__, err));
	}
	else {
		*result = NWM_CMDRES_SUCCESS;
		if (get)
			*vblank = arg;
	}

}
static bool wlc_paramset_vblanktsf_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_vblanktsf_req_t *p = (nwm_set_vblanktsf_req_t *)preq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));
	wlc_nin_vblanktsf(wlc, wlcif, &p->vblank, &pcfm->resultCode, FALSE);
	return TRUE;
}

static void
wlc_nin_set_maclist(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 count,
	char * input, uint16 enableflag, uint16 *result)
{
	int macmode;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	struct maclist *arg = (struct maclist *)(pnin->buffer);
	int len = count * ETHER_ADDR_LEN;
	int err = 0;

	bcopy(input, (char *)arg->ea, len);
	arg->count = count;

	err = wlc_ioctl(wlc, WLC_SET_MACLIST, arg, sizeof(pnin->buffer), wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: error %d setting maclist\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	if (enableflag > 2) {
		WL_ERROR(("wl%d %s: illegal value for macmode %d\n",
			wlc->pub->unit, __FUNCTION__, enableflag));
		err = BCME_BADARG;
		goto done;
	}
	macmode = (enableflag == 0) ? 0 : (enableflag ^ 0x3);
	err = wlc_ioctl(wlc, WLC_SET_MACMODE, &macmode, sizeof(macmode), wlcif);
	if (err)
		WL_ERROR(("wl%d %s: error %d setting macmode \n",
			wlc->pub->unit, __FUNCTION__, err));

done:
	*result = nwm_bcmerr_to_nwmerr(err);

}
static bool wlc_paramset_maclist_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_maclist_req_t *p = (nwm_set_maclist_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_set_maclist(wlc, wlcif, p->count, (char *)&p->macentry,
		p->enableflag, &pCfm->resultCode);
	return TRUE;
}

static void
wlc_nin_set_rtsthresh(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *rtsthresh, uint16 *result)
{
	int err;

	err = wlc_iovar_setint(wlc, "rtsthresh", *rtsthresh);

	if (err)
		*result = NWM_CMDRES_FAILURE;
	else {
		*result = NWM_CMDRES_SUCCESS;
	}

}
static bool wlc_paramset_rtsthresh_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_rtsthresh_req_t * p = (nwm_set_rtsthresh_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_set_rtsthresh(wlc, wlcif, &p->rtsthresh, &pCfm->resultCode);
	return TRUE;
}

static void
wlc_nin_fragthresh(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *fragthresh, uint16 *result, bool get)
{
	int err;

	if (get) {
		int arg;

		err = wlc_iovar_getint(wlc, "fragthresh", &arg);
		if (err) {
			*result = NWM_CMDRES_FAILURE;
			return;
		}
		*fragthresh = arg;
		goto done;
	}

	err = wlc_iovar_setint(wlc, "fragthresh", *fragthresh);

done:
	*result = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;

}
static bool wlc_paramset_fragthresh_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_fragthresh_req_t *p = (nwm_set_fragthresh_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_fragthresh(wlc, wlcif, &p->fragthresh, &pCfm->resultCode, FALSE);
	return TRUE;
}

static void
wlc_nin_set_pmk(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *keydat,
	uint16 keylen, uint16 *result)
{
	int err;
	wsec_pmk_t arg;
	int state;


	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE && state != NWM_NIN_CLASS1) {
		err = BCME_ASSOCIATED;
		goto done;
	}
	if (keylen < WSEC_MIN_PSK_LEN || keylen > WSEC_MAX_PSK_LEN) {
		WL_NIN(("wl%d %s: keylen %d not allowed\n",
			wlc->pub->unit, __FUNCTION__, keylen));
		err = BCME_BADARG;
		goto done;
	}

	bzero(&arg, sizeof(arg));
	arg.key_len = keylen;
	bcopy(keydat, arg.key, keylen);
	arg.flags = WSEC_PASSPHRASE;

	err = wlc_ioctl(wlc, WLC_SET_WSEC_PMK, &arg, sizeof(arg), wlcif);

done:
	*result = nwm_bcmerr_to_nwmerr(err);

}
static bool wlc_paramset_pmk_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_pmk_req_t *p = (nwm_set_pmk_req_t *)pReq;
	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_set_pmk(wlc, wlcif, p->key, p->keylen, &pCfm->resultCode);
	return TRUE;
}

#define SROM_BYTES_CT 256
static bool
wlc_nin_parse_srombuf(char *buf, int len, bool get, char *valstr, int val_len,
	uint32 *offset, uint32 cistpl, uint32 hnbu_code)
{
	uint8 tup, tlen;
	int i;
	int cis_count = 0;

	/* step over the first 8 bytes: no tuples there */
	i = 8;

	do {
		tup = buf[i++];
		tlen = buf[i++];
		WL_NIN(("tup 0x%x tlen 0x%x\n", tup, tlen));

		if (i + tlen >= len) break;

		if (tup == cistpl && buf[i] == hnbu_code) {
			WL_NIN(("%s: Found cistpl 0x%x hnbu_code 0x%x :\n",
				__FUNCTION__, cistpl, hnbu_code));
			WL_NIN(("CC0 0x%x CC1 0x%x CC2 0x%x \n",
			buf[i + 1], buf[i + 2], buf[i + 3]));
			*offset = i + 1;
			if (get)
				bcopy(&buf[i + 1], valstr, MIN(tlen - 1, val_len));
			else
				bcopy(valstr, &buf[i + 1], MIN(tlen - 1, val_len));

			return TRUE;
		}

		i += tlen;
		if (tup == 0xff) cis_count++;
	} while (cis_count < 2);

	return FALSE;
}
static void
wlc_eerom_cmd(wlc_info_t *wlc, struct wlc_if *wlcif, char *ccstr, uint16 val_len,
	uint16 *result, bool get, int cistpl, int hnbu_code)
{
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	srom_rw_t   *srt;
	int err;
	uint32 offset = 0;
	bool tupl_found;
	int buflen = SROM_BYTES_CT + sizeof(srom_rw_t);

	srt = (srom_rw_t *)pnin->buffer;
	bzero(srt, buflen);
	srt->byteoff = 0;
	srt->nbytes = SROM_BYTES_CT;

	err = wlc_ioctl(wlc, WLC_GET_SROM, srt, sizeof(pnin->buffer), wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: error %d calling WLC_GET_SROM\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	/* sift through and find what we want */
	tupl_found = wlc_nin_parse_srombuf((char *)srt->buf, SROM_BYTES_CT, get,
		(char *)ccstr, val_len, &offset, cistpl, hnbu_code);

	if (!tupl_found) {
		WL_ERROR(("wl%d %s: cistupl 0x%x hnbu_code 0x%x not found\n",
			wlc->pub->unit, __FUNCTION__, cistpl, hnbu_code));

		err = BCME_ERROR;
		goto done;
	}
	WL_NIN(("\n%s: AFTER buffer scan:\n\n", __FUNCTION__));
	WL_NIN(("offset 0x%x ccstr[0] 0x%x ccstr[1] 0x%x ccstr[2] 0x%x\n",
		offset, ccstr[0], ccstr[1], ccstr[2]));
	WL_NIN(("Buffer:\n"));
	/* Write it back out if requested */
	if (!get) {
		char tempstr[32];
		int wrlen;
		char *p;

		/* Now write out our piece */
		wrlen = (val_len & 1) ? val_len + 1 : val_len;

		p = (char *)srt->buf;
		bcopy(p + offset, tempstr, wrlen);

		bzero(srt, buflen);
		srt->byteoff = offset;
		srt->nbytes = wrlen;
		bcopy(tempstr, srt->buf, wrlen);
		WL_NIN(("About to write at offset 0x%x...\n", offset));

		err = wlc_ioctl(wlc, WLC_SET_SROM, srt, sizeof(pnin->buffer), wlcif);
		if (err)
			WL_ERROR(("wl%d %s: WLC_SET_SROM failed err %d\n",
				wlc->pub->unit, __FUNCTION__, err));

	}

done:
	*result = nwm_bcmerr_to_nwmerr(err);
}
static bool wlc_paramset_eerom_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_eerom_req_t *p = (nwm_set_eerom_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_eerom_cmd(wlc, wlcif, (char *)p->country_code, sizeof(p->country_code) - 1,
		&pCfm->resultCode, FALSE, CISTPL_BRCM_HNBU, HNBU_CCODE);
	return TRUE;
}

static void
wlc_txpwr_cmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *result,
	uint16 *val, bool get)
{
	int pwrarg, i, err;
	int state;
	size_t pprsize = ppr_ser_size_by_bw(ppr_get_max_bw());
	/* Allocate memory for structure and 2 serialisation buffer */
	tx_pwr_rpt_t *ppr_wl = (tx_pwr_rpt_t *)MALLOC(wlc->osh, sizeof(tx_pwr_rpt_t) + pprsize*2);
	uint8 *ppr_ser;
	if (ppr_wl == NULL)
		return;

	ppr_ser = ppr_wl->pprdata;
	memset(ppr_wl, 0, sizeof(tx_pwr_rpt_t) + pprsize*2);
	ppr_wl->board_limit_len  = pprsize;
	ppr_wl->target_len       = pprsize;
	ppr_wl->version          = TX_POWER_T_VERSION;
	/* init allocated mem for serialisation */
	ppr_init_ser_mem_by_bw(ppr_ser, ppr_get_max_bw(), ppr_wl->board_limit_len);
	ppr_ser += ppr_wl->board_limit_len;
	ppr_init_ser_mem_by_bw(ppr_ser, ppr_get_max_bw(), ppr_wl->target_len);

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state == NWM_NIN_IDLE) {
		err = BCME_NOTUP;
		goto done;
	}

	if (get) {
		ppr_t* ppr_target = NULL;
		ppr_dsss_rateset_t dsss_limit;
		ppr_ofdm_rateset_t ofdm_limit;
		err = wlc_ioctl(wlc, WLC_CURRENT_PWR, ppr_wl, sizeof(tx_pwr_rpt_t) + pprsize*2,
			wlcif);
		if (err)
			goto done;

		if ((err = ppr_deserialize_create(NULL, ppr_ser, ppr_wl->target_len, &ppr_target))
			!= BCME_OK) {
			goto free_ppr;
		}

		ppr_get_dsss(ppr_target, WL_TX_BW_20, WL_TX_CHAINS_1, &dsss_limit);
		pwrarg = dsss_limit.pwr[0];
		for (i = 1; i < WL_RATESET_SZ_DSSS; i++)
			pwrarg = MIN(pwrarg, dsss_limit.pwr[i]);

		ppr_get_ofdm(ppr_target, WL_TX_BW_20, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limit);
		for (i = 0; i < WL_RATESET_SZ_OFDM; i++)
			pwrarg = MIN(pwrarg, ofdm_limit.pwr[i]);

		*val = pwrarg;

free_ppr:
		if (ppr_target != NULL) {
			ppr_delete(NULL, ppr_target);
		}
		goto done;
	}

	/* restore defaults */
	pwrarg = (*val > 127) ? 127 : *val;

	err = wlc_iovar_op(wlc, "qtxpower", NULL, 0, &pwrarg,
		sizeof(pwrarg), IOV_SET, wlcif);

done:
	*result = nwm_bcmerr_to_nwmerr(err);
	MFREE(wlc->osh, ppr_wl, sizeof(tx_pwr_rpt_t) + pprsize*2);
}

static bool wlc_paramset_txpwr_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_txpwr_req_t *p = (nwm_set_txpwr_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_txpwr_cmd(wlc, wlcif, &pCfm->resultCode, &p->txpwr, FALSE);
	return TRUE;
}

static void
wlc_mrate_cmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *result,
	uint16 *val, bool get)
{
	uint32 arg;
	int err;

	if (get) {
		err = wlc_iovar_op(wlc, "bg_mrate", NULL, 0, &arg,
			sizeof(arg), IOV_GET, wlcif);
		if (!err)
			*val = arg;
		goto done;
	}

	arg = *val;
	err = wlc_iovar_op(wlc, "bg_mrate", NULL, 0, &arg,
		sizeof(arg), IOV_SET, wlcif);

done:
	*result = nwm_bcmerr_to_nwmerr(err);
}

static bool wlc_paramset_mrate_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_set_mcast_rate_req_t *p = (nwm_set_mcast_rate_req_t *)pReq;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_mrate_cmd(wlc, wlcif, &pCfm->resultCode, &p->mcast_rate, FALSE);
	return TRUE;
}

/* Get */
/* A rough symmetry with the set commands
 *
 */

static bool wlc_paramget_all_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	uint16 err;
	nwm_get_all_cfm_t *p = (nwm_get_all_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_retrycmd(wlc, wlcif, &p->shortRetryLimit, &p->longRetryLimit,
	&err, TRUE);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d retrieving retry limits\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	wlc_nin_macaddr(wlc, wlcif, p->macaddr, &pCfm->resultCode, TRUE);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d retrieving macaddr\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	/* NULL is country buffer: not used here */
	wlc_nin_getcountryinfo(wlc, wlcif, NULL, &p->enableChannel,
		&p->channel, &err);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d retrieving country info\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	wlc_nin_get_modecmd(wlc, wlcif, &p->opmode, &err);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d retrieving mode\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	wlc_nin_get_wepmodecmd(wlc, wlcif, &p->wepmode, &err);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d retrieving wepmode\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	if (p->wepmode == NWM_NIN_WEP_40_BIT || p->wepmode == NWM_NIN_WEP_104_BIT) {
		wlc_nin_wepkeyid(wlc, wlcif, &p->wepKeyId, &err, TRUE);
		if (err) {
			WL_ERROR(("wl%d %s: ERROR %d retrieving wepkeyid\n",
			wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}
	} else
		p->wepKeyId = 0;

	wlc_nin_beaconfrmtype(wlc, wlcif, &p->beacontype, &err, TRUE);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d retrieving beacontype\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	wlc_nin_prbresp(wlc, wlcif, &p->proberes, &err, TRUE);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d retrieving prbresp\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	wlc_nin_bcntimeout(wlc, wlcif, &p->beaconlost_thr, &err, TRUE);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d retrieving bcntimeout\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	wlc_nin_ssidmask(wlc, wlcif, p->ssidmask, &err, TRUE);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d retrieving ssidmask\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	wlc_nin_preambleCmd(wlc, wlcif, &p->preambletype, &err, TRUE);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d retrieving preambletype\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	wlc_nin_authalgocmd(wlc, wlcif, &p->authalgo, &err, TRUE);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d retrieving preambletype\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

done:
	p->resultCode = nwm_bcmerr_to_nwmerr(err);

	return TRUE;
}

static bool wlc_paramget_macadrs_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_macadrs_cfm_t *p = (nwm_get_macadrs_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_macaddr(wlc, wlcif, p->macaddr, &pCfm->resultCode, TRUE);
	return TRUE;
}

static bool wlc_paramget_retry_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_retrylimit_cfm_t * p = (nwm_get_retrylimit_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_retrycmd(wlc, wlcif, &p->shortRetryLimit, &p->longRetryLimit,
	&p->resultCode, TRUE);
	return TRUE;
}

static void wlc_nin_getcountryinfo(wlc_info_t * wlc, struct wlc_if * wlcif,
	char * country, uint16 *chvector, uint16 *channelinuse, uint16 *result)
{
	int err, i;
	channel_info_t chinfoarg;
	int maxentries = NWM_MAX_CHANNELS;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	wl_uint32_list_t * argch = (wl_uint32_list_t *)(pnin->buffer);
	*chvector = *channelinuse = 0;
	argch->count = WL_NUMCHANNELS;

	err = 0;
	if (country) {
		uint32 countrybuf;

		err = wlc_ioctl(wlc, WLC_GET_COUNTRY, &countrybuf, WLC_CNTRY_BUF_SZ, wlcif);
		if (err) {
			WL_ERROR(("wl%d %s: WLC_GET_COUNTRY error %d\n",
				wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}
		bcopy(&countrybuf, country, sizeof(countrybuf));
	}
	err = wlc_ioctl(wlc, WLC_GET_CHANNEL, &chinfoarg, sizeof(channel_info_t), wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: WLC_GET_CHANNEL error %d\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	err = wlc_ioctl(wlc, WLC_GET_VALID_CHANNELS, argch, WL_NUMCHANNELS, wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: WLC_GET_VALID_CHANNELS error %d\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	*channelinuse = chinfoarg.target_channel;
	for (i = 0; i < MIN(maxentries, argch->count) && argch->element[i] < maxentries; i++) {
		*chvector |= NBITVAL(argch->element[i]);
	}

done:
	*result = nwm_bcmerr_to_nwmerr(err);
}

static bool wlc_paramget_enablechannel_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_enablechannel_cfm_t * p = (nwm_get_enablechannel_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_getcountryinfo(wlc, wlcif, (char *)p->country, &p->enableChannel,
	&p->channel, &p->resultCode);
	return TRUE;
}

static void
wlc_nin_get_modecmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *mode,
	uint16 *result)
{
	int err;
	int aparg, infrarg, nitroarg, apstarg;
	int i;

	err = wlc_ioctl(wlc, WLC_GET_AP, &aparg, sizeof(aparg), wlcif);
	err |= wlc_ioctl(wlc, WLC_GET_INFRA, &infrarg, sizeof(infrarg), wlcif);
	err |= wlc_iovar_getint(wlc, "nitro", &nitroarg);
	err |= wlc_iovar_getint(wlc, "apsta", &apstarg);
	if (err) {
		*result = NWM_CMDRES_FAILURE;
		return;
	}

	for (i = 1; i < ARRAYSIZE(modetbl); i++) {
		if (modetbl[i].ap == aparg && modetbl[i].infra == infrarg &&
			modetbl[i].nitro == nitroarg && modetbl[i].apsta == apstarg) {
			*mode = i;
			*result = NWM_CMDRES_SUCCESS;
			return;
		}
	}
	*result = NWM_CMDRES_FAILURE;
}
static bool wlc_paramget_mode_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_mode_cfm_t *p = (nwm_get_mode_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_get_modecmd(wlc, wlcif, &p->mode, &p->resultCode);

	return TRUE;
}

static void
wlc_nin_get_ratesetcmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *srate,
	uint16 *brate, uint16 *result)
{
	int err, i, j;
	wl_rateset_t rateset;
	uint32 svector = 0;
	uint32 bvector = 0;

	err = wlc_ioctl(wlc, WLC_GET_CURR_RATESET, &rateset, sizeof(rateset), wlcif);
	if (err) {
		*result = NWM_CMDRES_FAILURE;
		return;
	}

	for (i = 0; i < rateset.count; i++) {
		for (j = 0; j < ARRAYSIZE(nwm_rateconvtbl); j++) {
			if (nwm_rateconvtbl[j].rate == (rateset.rates[i] & 0x7f)) {
				svector |= NBITVAL(j);
				if (rateset.rates[i] & 0x80)
					bvector |= NBITVAL(j);
				break;
			}
		}
	}

	*srate = svector;
	*brate = bvector;
	*result = NWM_CMDRES_SUCCESS;
}
static bool wlc_paramget_rate_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_rateset_cfm_t *p = (nwm_get_rateset_cfm_t *)pCfm;
	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_get_ratesetcmd(wlc, wlcif, &p->supprate, &p->basicrate, &p->resultCode);
	return TRUE;
}
static void
wlc_nin_get_wepmodecmd(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *wepmode,
	uint16 *result)
{
	int err, wparg, wsecarg;
	int i;
	wl_wsec_key_t keyarg;

	bzero(&keyarg, sizeof(wl_wsec_key_t));
	keyarg.index = 0;

	err = wlc_ioctl(wlc, WLC_GET_WPA_AUTH, &wparg, sizeof(wparg), wlcif);
	err |= wlc_ioctl(wlc, WLC_GET_WSEC, &wsecarg, sizeof(wsecarg), wlcif);
	if (err)
		goto errdone;

	for (i = 0; i < ARRAYSIZE(wlc_nin_secmodetbl); i++) {

		if (wlc_nin_secmodetbl[i].wparg == wparg &&
			wlc_nin_secmodetbl[i].wsecarg == wsecarg)
			break;
	}
	if (i >= ARRAYSIZE(wlc_nin_secmodetbl))
		goto errdone;

	/* 40 bit or 104 bit wep? Retrieve the key length to resolve. */
	if (i == NWM_NIN_WEP_40_BIT || i == NWM_NIN_WEP_104_BIT) {
		err = wlc_ioctl(wlc, WLC_GET_KEY, &keyarg, sizeof(keyarg), wlcif);
		if (err) goto errdone;
		*wepmode = (keyarg.len == 5) ? NWM_NIN_WEP_40_BIT : NWM_NIN_WEP_104_BIT;
	} else {
		*wepmode = i;
	}

	*result = NWM_CMDRES_SUCCESS;
	return;

errdone:
	*result = NWM_CMDRES_FAILURE;
}

static bool wlc_paramget_wepmode_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_wepmode_cfm_t *p = (nwm_get_wepmode_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
		wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_get_wepmodecmd(wlc, wlcif, &p->wepmode, &p->resultCode);
	WL_NIN(("wl%d %s result code is 0x%x wepmode is %d\n",
	wlc->pub->unit, __FUNCTION__,  p->resultCode, p->wepmode));

	return TRUE;
}

static bool wlc_paramget_wepkeyid_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_wepkeyid_cfm_t *p = (nwm_get_wepkeyid_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_wepkeyid(wlc, wlcif, &p->wepKeyId, &p->resultCode, TRUE);
	return TRUE;
}

static bool wlc_paramget_beacontype_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_beacontype_cfm_t *p = (nwm_get_beacontype_cfm_t *)pcfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));

	wlc_nin_beaconfrmtype(wlc, wlcif, &p->beacontype, &p->resultcode, TRUE);


	return TRUE;
}

static bool wlc_paramget_resbcssid_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_proberes_cfm_t *p = (nwm_get_proberes_cfm_t *)pcfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));

	wlc_nin_prbresp(wlc, wlcif, &p->proberes, &p->resultcode, TRUE);
	return TRUE;
}

static bool wlc_paramget_beaconlostth_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	 nwm_get_beaconlostth_cfm_t *p = (nwm_get_beaconlostth_cfm_t *)pcfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));
	wlc_nin_bcntimeout(wlc, wlcif, &p->beaconlost_thr, &p->resultCode, TRUE);
	return TRUE;
}


static bool wlc_paramget_ssidmask_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_ssidmask_cfm_t *p = (nwm_get_ssidmask_cfm_t *)pcfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));

	wlc_nin_ssidmask(wlc, wlcif, p->ssidmask, &p->resultCode, TRUE);
	return TRUE;
}

static bool wlc_paramget_preambletype_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_preambletype_cfm_t *p = (nwm_get_preambletype_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_preambleCmd(wlc, wlcif, &p->type, &p->resultCode, TRUE);
	return TRUE;
}

static bool wlc_paramget_authalgo_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_authalgo_cfm_t *p = (nwm_get_authalgo_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_authalgocmd(wlc, wlcif, &p->type, &p->resultCode, TRUE);
	return TRUE;
}


static bool wlc_paramget_maxconn_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_maxconn_cfm_t *p = (nwm_get_maxconn_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_maxassoc(wlc, wlcif, &p->count, &p->resultCode, TRUE);
	return TRUE;
}

/* Get tx antenna */
static bool wlc_paramget_txant_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_txant_cfm_t *p = (nwm_get_txant_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_txant(wlc, wlcif, &p->useantenna, &p->resultCode, TRUE);
	return TRUE;
}

static void
wlc_nin_get_diversity(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *diversity, uint16 *useantenna, uint16 *result)
{
	int err, arg;

	err = wlc_ioctl(wlc, WLC_GET_ANTDIV, &arg, sizeof(arg), wlcif);
	if (err) {
		*result = NWM_CMDRES_FAILURE;
		WL_NIN(("wl%d: %s: get antdiv err %d\n", wlc->pub->unit, __FUNCTION__, err));
		return;
	}
	if (arg == 3) {
		*diversity = 1;
		*useantenna = 0;
	} else {
		*useantenna = arg;
		*diversity = 0;
	}

	*result = NWM_CMDRES_SUCCESS;
}
static bool wlc_paramget_diversity_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_diversity_cfm_t * p = (nwm_get_diversity_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_get_diversity(wlc, wlcif, &p->diversity, &p->useantenna, &p->resultCode);
	return TRUE;
}

static bool wlc_paramget_bcnsendrecvind_cmd(void * pkt,
	nwm_cmd_req_t * pReq, nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc,
	struct wlc_if * wlcif)
{
	nwm_get_beaconsendrecvind_cfm_t *p = (nwm_get_beaconsendrecvind_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_bcnind_enable(wlc, wlcif, &p->enable_recv, &p->enable_send,
		&p->resultCode, TRUE);
	return TRUE;
}

/* Get Interference Mode */
static bool wlc_paramget_interference_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_interference_cfm_t *p = (nwm_get_interference_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_interference(wlc, wlcif, &p->interference, &p->resultCode, TRUE);
	return TRUE;
}

static bool wlc_paramget_bssid_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_bssid_cfm_t *p = (nwm_get_bssid_cfm_t *)pCfm;
	uint8 bssid[ETHER_ADDR_LEN];
	int err;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	bzero(bssid, ETHER_ADDR_LEN);
	err = wlc_ioctl(wlc, WLC_GET_BSSID, bssid, ETHER_ADDR_LEN, wlcif);
	if (!err) {
		bcopy(bssid, p->bssid, ETHER_ADDR_LEN);
	}
	pCfm->resultCode = nwm_bcmerr_to_nwmerr(err);
	return TRUE;
}

static bool wlc_paramget_ssid_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	return TRUE;
}

static bool wlc_paramget_beaconperiod_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_beaconperiod_cfm_t *p = (nwm_get_beaconperiod_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_bcnprd(wlc, wlcif, &p->beaconPeriod, &p->resultCode, TRUE);
	return TRUE;
}

static bool wlc_paramget_dtimperiod_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_dtimperiod_cfm_t *p = (nwm_get_dtimperiod_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_dtimprd(wlc, wlcif, &p->dtimPeriod, &p->resultCode, TRUE);
	return TRUE;
}


static bool wlc_paramget_gameinfo_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_gameinfo_cfm_t *p = (nwm_get_gameinfo_cfm_t *)pcfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));
	wlc_nin_gameinfo(wlc, wlcif, p->gameinfo, &p->game_info_length, &pcfm->resultCode, TRUE);

	return TRUE;
}

static bool wlc_paramget_vblanktsf_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_vblanktsf_cfm_t *p = (nwm_get_vblanktsf_cfm_t *)pcfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));
	wlc_nin_vblanktsf(wlc, wlcif, &p->vblank, &pcfm->resultCode, TRUE);
	return TRUE;
}

static void
wlc_nin_get_maclist(wlc_info_t *wlc, struct wlc_if *wlcif,
	struct ether_addr *peabuf, uint16 *count, uint16 *enableflag,
	uint16 *result, int *eabuf_len)
{
	int err, macmode, copylen;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	struct maclist *pmaclist = (struct maclist *)(pnin->buffer);

	WL_NIN(("%s: sizeof pnin->buffer is %d\n", __FUNCTION__, (int)sizeof(pnin->buffer)));
	bzero(pmaclist, sizeof(pnin->buffer));
	pmaclist->count = MAXMACLIST;

	err = wlc_ioctl(wlc, WLC_GET_MACLIST, pmaclist, sizeof(pnin->buffer), wlcif);
	if (err)
		goto done;

	copylen = pmaclist->count * ETHER_ADDR_LEN;
	if (*eabuf_len < copylen) {
		err = BCME_NOMEM;
		goto done;
	}
	*eabuf_len = copylen;
	bcopy(pmaclist->ea, peabuf, copylen);

	err = wlc_ioctl(wlc, WLC_GET_MACMODE, &macmode, sizeof(macmode), wlcif);

	if (!err) {
		*enableflag = (macmode == 0) ? 0 : (macmode ^ 0x3);
		*count = pmaclist->count;
	}

done:
	*result = nwm_bcmerr_to_nwmerr(err);
}
static bool wlc_paramget_maclist_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_maclist_req_t *p = (nwm_get_maclist_req_t *)pCfm;
	int listlen;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	/* Room available for mac entries: one already in cfm structure */
	listlen = PKTLEN(wlc->osh, pkt) -
		sizeof(nwm_get_maclist_req_t) + ETHER_ADDR_LEN;
	WL_NIN(("wl%d %s: available listlen %d\n", wlc->pub->unit, __FUNCTION__, listlen));

	wlc_nin_get_maclist(wlc, wlcif, (struct ether_addr *)p->macentry,
		&p->count, &p->enableflag, &p->resultCode, &listlen);

	return TRUE;
}

static void
wlc_nin_get_rtsthresh(wlc_info_t *wlc, struct wlc_if *wlcif,
	uint16 *rtsthresh, uint16 *result)
{
	int err, arg;

	err = wlc_iovar_getint(wlc, "rtsthresh", &arg);
	if (err) {
		*result = NWM_CMDRES_FAILURE;
		return;
	}
	*rtsthresh =  arg;
	*result = NWM_CMDRES_SUCCESS;

}
static bool wlc_paramget_rtsthresh_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_rtsthresh_cfm_t *p = (nwm_get_rtsthresh_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_get_rtsthresh(wlc, wlcif, &p->rtsthresh, &p->resultCode);
	return TRUE;
}

static bool wlc_paramget_fragthresh_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_fragthresh_cfm_t *p = (nwm_get_fragthresh_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_fragthresh(wlc, wlcif, &p->fragthresh, &p->resultCode, TRUE);
	return TRUE;
}

static bool
wlc_paramget_eerom_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_eerom_cfm_t *p = (nwm_get_eerom_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_eerom_cmd(wlc, wlcif, (char *)p->country_code,
		sizeof(p->country_code) - 1, &pCfm->resultCode,
		TRUE, CISTPL_BRCM_HNBU, HNBU_CCODE);
	return TRUE;
}

static void
wlc_nin_get_rxtx_rssi_info(wlc_info_t *wlc, struct wlc_if *wlcif,
	nwm_rssi_info_t *plist, uint16 *count, uint16 *tssi, uint16 *result)
{
	int err, i;
	struct scb *scb;
	wl_rxtxdata_req_t rtxarg;
	scb_val_t scb_val_arg;
	nwm_rssi_info_t *p = plist;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	int scbcount = 0;
	struct maclist *pmaclist = (struct maclist *)(pnin->buffer);
	size_t pprsize = ppr_ser_size_by_bw(ppr_get_max_bw());
	/* Allocate memory for structure and 2 serialisation buffer */
	tx_pwr_rpt_t *ppr_wl = (tx_pwr_rpt_t *)MALLOC(wlc->osh, sizeof(tx_pwr_rpt_t) + pprsize*2);
	uint8 *ppr_ser;
	if (ppr_wl == NULL)
		return;

	ppr_ser = ppr_wl->pprdata;
	memset(ppr_wl, 0, sizeof(tx_pwr_rpt_t) + pprsize*2);
	ppr_wl->board_limit_len  = pprsize;
	ppr_wl->target_len       = pprsize;
	ppr_wl->version          = TX_POWER_T_VERSION;
	/* init allocated mem for serialisation */
	ppr_init_ser_mem_by_bw(ppr_ser, ppr_get_max_bw(), ppr_wl->board_limit_len);
	ppr_ser += ppr_wl->board_limit_len;
	ppr_init_ser_mem_by_bw(ppr_ser, ppr_get_max_bw(), ppr_wl->target_len);

	pmaclist->count = wlc->pub->tunables->maxscb;


	/* get a list of mac addresses that are associated */
	err = wlc_ioctl(wlc, WLC_GET_ASSOCLIST, pmaclist, sizeof(pnin->buffer), wlcif);

	if (err) {
		WL_NIN(("wl%d %s: failed to get assoclist error %d\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	for (i = 0; i < pmaclist->count; i++) {

		bcopy(&pmaclist->ea[i], p[i].macaddr, sizeof(struct ether_addr));
		scb = wlc_scbfind(wlc, wlc_bsscfg_find_by_wlcif(wlc, wlcif), &pmaclist->ea[i]);
		if (!scb) {
			err = BCME_BADARG;
			goto done;
		}

		rtxarg.scb = scb;
		rtxarg.data = 0;
		err = wlc_iovar_op(wlc, "rxrate", NULL, 0,
			&rtxarg, sizeof(wl_rxtxdata_req_t), IOV_GET, NULL);
		if (err) {
			WL_NIN(("wl%d %s: err  %d retrieving rxrate iovar\n",
				wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}
		p[i].RxRate = rtxarg.data & RATE_MASK;


		rtxarg.data = 0;
		err = wlc_iovar_op(wlc, "txrate", NULL, 0,
			&rtxarg, sizeof(wl_rxtxdata_req_t), IOV_GET, NULL);
		if (err) {
			WL_NIN(("wl%d %s: err  %d retrieving txrate iovar\n",
				wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}
		p[i].TxRate = rtxarg.data & RATE_MASK;


		bcopy(&pmaclist->ea[i], &scb_val_arg.ea, ETHER_ADDR_LEN);
		scb_val_arg.val = 0;
		err = wlc_ioctl(wlc, WLC_GET_RSSI, &scb_val_arg, sizeof(scb_val_arg), wlcif);
		if (err) {
			WL_NIN(("wl%d %s: err  %d from WLC_GET_RSSI \n",
				wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}
		p->RSSI = scb_val_arg.val & 0xff;
		p->RSSI &= 0xff;
		scbcount++;

	}


	*count = scbcount;
	/* NOT per scb */
	err = wlc_ioctl(wlc, WLC_CURRENT_PWR, ppr_wl, sizeof(tx_pwr_rpt_t) + pprsize*2, wlcif);
	if (err) {
		WL_NIN(("wl%d %s: err  %d retrieving WLC_CURRENT_PWR\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	*tssi = MAX(ppr_wl->est_Pout[0], ppr_wl->est_Pout_cck);


done:
	*result = (err) ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
	MFREE(wlc->osh, ppr_wl, sizeof(tx_pwr_rpt_t) + pprsize*2);
}

static bool
wlc_paramget_rssi_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_rssi_cfm_t *p = (nwm_get_rssi_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_get_rxtx_rssi_info(wlc, wlcif, p->info, &p->count, &p->TSSI,
		&pCfm->resultCode);
	return TRUE;
}

static bool
wlc_paramget_txpwr_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_txpwr_cfm_t *p = (nwm_get_txpwr_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_txpwr_cmd(wlc, wlcif, &pCfm->resultCode, &p->txpwr, TRUE);
	return TRUE;
}

static bool
wlc_paramget_mrate_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_get_mcast_rate_cfm_t *p = (nwm_get_mcast_rate_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_mrate_cmd(wlc, wlcif, &pCfm->resultCode, &p->mcast_rate, TRUE);
	return TRUE;
}

/*
 * Go to IDLE state
 * Other consequences
 * Including but not limited to:
 * drop associations, authentications
 * turn off radio, disable mac
 * zero out ssid
 * But don't clear any other config
 */
static bool wlc_dev_idle_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	int err, arg;
	wlc_ssid_t ssid_arg;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	char *pbuf = (char *)(pnin->buffer);
	int iolen;
	int arglen = strlen("ssid") + 1 + strlen("bsscfg:");

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));

	arg = 0;
	err = wlc_ioctl(wlc, WLC_DOWN, &arg, sizeof(arg), wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: error %d setting WLC_DOWN\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	/* wipe it */
	bzero(&ssid_arg, sizeof(ssid_arg));
	iolen = wlc_bssiovar_mkbuf("ssid", 0, &ssid_arg, sizeof(ssid_arg),
		pbuf, sizeof(pnin->buffer), &err);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d creating ssid iobuf\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	err = wlc_iovar_op(wlc, pbuf, NULL, 0, (void*)((int8*)pbuf + arglen),
		iolen - arglen, IOV_SET, wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d setting ssid\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}


done:
	pcfm->resultCode = nwm_bcmerr_to_nwmerr(err);
	return TRUE;
}


/*
 * Go to CLASS1 state
 * Maps to WLC_UP
 *
 */
static bool wlc_dev_class1_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	int err, arg;
	int state;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));
	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE) {
		pcfm->resultCode = NWM_CMDRES_FAILURE;
		return TRUE;
	}

#ifdef NONWLAN_INTERFERENCE
	arg = NON_WLAN;
	err = wlc_ioctl(wlc, WLC_SET_INTERFERENCE_MODE, &arg, sizeof(arg), wlcif);
	if (err < 0) {
		WL_ERROR(("Error:%d\n", err));
		pcfm->resultCode = NWM_CMDRES_FAILURE;
		return TRUE;
	}
#endif /* NONWLAN_INTERFERENCE */

	arg = 0;
	err = wlc_ioctl(wlc, WLC_UP, &arg, sizeof(arg), wlcif);

	pcfm->resultCode = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
	return TRUE;
}

/*
 * This was translated as restart, the name says reboot,
 * the action is SHUTDOWN which we don't support.
 * Instead, we'll go to the IDLE state with all configuration
 * defaulted according to values in the Nintendo spec.
 * Note that we also zero out the ssid.
 */
static bool wlc_dev_reboot_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	int arg, err;
	wlc_ssid_t ssid_arg;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	char *pbuf = (char *)(pnin->buffer);
	int iolen;
	int arglen = strlen("ssid") + 1 + strlen("bsscfg:");

	/* wipe it */
	bzero(&ssid_arg, sizeof(ssid_arg));

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));

	arg = 0;
	err = wlc_ioctl(wlc, WLC_DOWN, &arg, sizeof(arg), wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: failed to set WLC_DOWN err %d\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	iolen = wlc_bssiovar_mkbuf("ssid", 0, &ssid_arg, sizeof(ssid_arg),
		pbuf, sizeof(pnin->buffer), &err);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d creating ssid iobuf\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}
	err = wlc_iovar_op(wlc, pbuf, NULL, 0, (void*)((int8*)pbuf + arglen),
		iolen - arglen, IOV_SET, wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: ERROR %d setting ssid\n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

	err = wlc_nin_restore_defaults(wlc, wlcif);
	if (err) {
		WL_ERROR(("wl%d %s: error %d restoring defaults \n",
			wlc->pub->unit, __FUNCTION__, err));
		goto done;
	}

done:
	pcfm->resultCode = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;

	return TRUE;
}

static void
wlc_nin_clr_wlcounters(wlc_info_t *wlc, struct wlc_if *wlcif, uint16 *result)
{
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	void *pbuf = (void *)(pnin->buffer);
	nwm_wl_cnt_t *pwlcnt = (nwm_wl_cnt_t *)pbuf;
	wlc_nitro_cnt_t *pnitro = (wlc_nitro_cnt_t *)(pwlcnt + 1);

	wlc_nin_get_wlcounters(wlc, wlcif, pwlcnt, pnitro, result);

	if (*result != NWM_CMDRES_SUCCESS)
		goto done;

	/* update our private 'offset' counters */
	bcopy((char *)pwlcnt, (char *)&pnin->wlcntr_offset, sizeof(pnin->wlcntr_offset));
	bcopy((char *)pnitro, (char *)&pnin->nitrocntr_offset, sizeof(pnin->nitrocntr_offset));

done:
	return;
}

/*
 * Clear wireless info counters
 */
static bool wlc_dev_clearwlinfo_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	wlc_nin_clr_wlcounters(wlc, wlcif, &pCfm->resultCode);
	return TRUE;
}

/*
 * retrieve wl's version
 */
static bool wlc_dev_getverinfo_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	int err;
	int revlen, verlen;
	wlc_rev_info_t *psrc;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	char * pstr = (char *)(pnin->buffer);
	nwm_dev_getverinfo_cfm_t *ptarg = (nwm_dev_getverinfo_cfm_t *)pCfm;

#define WL_DEBUG_TYPE "Release"
#ifndef WL_TARGET_MEM_SIZE
#define WL_TARGET_MEM_SIZE "Linux"
#endif

	const char __version[] = "$IOSVersion: WL: " WL_BUILD_TIME " "
	   WL_TARGET_MEM_SIZE " Ver." EPI_VERSION_STR "/" WL_DEBUG_TYPE " $";

	revlen = sizeof(wlc_rev_info_t);

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	sprintf(pstr, __version);

	/* defensive: must be less than our buffer size */
	verlen = MIN(strlen(pstr), NWM_VER_STRLEN - 1);
	bcopy(pstr, ptarg->version_str, verlen);

	psrc = (wlc_rev_info_t *)(pnin->buffer);
	bzero(psrc, revlen);
	err = wlc_ioctl(wlc, WLC_GET_REVINFO, psrc, revlen, wlcif);
	if (err) goto done;

	ptarg->vendorid = psrc->vendorid;
	ptarg->deviceid = psrc->deviceid;
	ptarg->radiorev = psrc->radiorev;
	ptarg->chiprev = psrc->chiprev;
	ptarg->corerev = psrc->corerev;
	ptarg->boardid = psrc->boardid;
	ptarg->boardvendor = psrc->boardvendor;
	ptarg->boardrev = psrc->boardrev;
	ptarg->driverrev = psrc->driverrev;
	ptarg->ucoderev = psrc->ucoderev;
	ptarg->bus = psrc->bus;
	ptarg->chipnum = psrc->chipnum;

done:
	pCfm->resultCode = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
	return TRUE;
}
#define WLCNTOFFSET(name, p, offset)	((p->name) -= (offset->name))
/* helper function for our software zeroes */
static void
wlc_nin_apply_counter_offsets(nwm_wl_cnt_t *pwl, wlc_nitro_cnt_t *pnitro,
	wlc_info_t *wlc)
{
	uint8 i;
	wlc_nin_t *pnin = (wlc_nin_t *)wlc->nin;
	nwm_wl_cnt_t *pwloffs = &pnin->wlcntr_offset;
	wlc_nitro_cnt_t *pnoffs = &pnin->nitrocntr_offset;

	WLCNTOFFSET(txframe, pwl, pwloffs);
	WLCNTOFFSET(txbyte, pwl, pwloffs);
	WLCNTOFFSET(txretrans, pwl, pwloffs);
	WLCNTOFFSET(txerror, pwl, pwloffs);
	WLCNTOFFSET(txctl, pwl, pwloffs);
	WLCNTOFFSET(txprshort, pwl, pwloffs);
	WLCNTOFFSET(txserr, pwl, pwloffs);
	WLCNTOFFSET(txnobuf, pwl, pwloffs);
	WLCNTOFFSET(txnoassoc, pwl, pwloffs);
	WLCNTOFFSET(txrunt, pwl, pwloffs);
	WLCNTOFFSET(txchit, pwl, pwloffs);
	WLCNTOFFSET(txcmiss, pwl, pwloffs);
	WLCNTOFFSET(txuflo, pwl, pwloffs);
	WLCNTOFFSET(txphyerr, pwl, pwloffs);
	WLCNTOFFSET(txphycrs, pwl, pwloffs);
	WLCNTOFFSET(rxframe, pwl, pwloffs);
	WLCNTOFFSET(rxbyte, pwl, pwloffs);
	WLCNTOFFSET(rxerror, pwl, pwloffs);
	WLCNTOFFSET(rxctl, pwl, pwloffs);
	WLCNTOFFSET(rxnobuf, pwl, pwloffs);
	WLCNTOFFSET(rxnondata, pwl, pwloffs);
	WLCNTOFFSET(rxbadds, pwl, pwloffs);
	WLCNTOFFSET(rxbadcm, pwl, pwloffs);
	WLCNTOFFSET(rxfragerr, pwl, pwloffs);
	WLCNTOFFSET(rxrunt, pwl, pwloffs);
	WLCNTOFFSET(rxgiant, pwl, pwloffs);
	WLCNTOFFSET(rxnoscb, pwl, pwloffs);
	WLCNTOFFSET(rxbadproto, pwl, pwloffs);
	WLCNTOFFSET(rxbadsrcmac, pwl, pwloffs);
	WLCNTOFFSET(rxbadda, pwl, pwloffs);
	WLCNTOFFSET(rxfilter, pwl, pwloffs);
	WLCNTOFFSET(rxoflo, pwl, pwloffs);
	for (i = 0; i < NFIFO; i++)
		WLCNTOFFSET(rxuflo[i], pwl, pwloffs);
	WLCNTOFFSET(d11cnt_txrts_off, pwl, pwloffs);
	WLCNTOFFSET(d11cnt_rxcrc_off, pwl, pwloffs);
	WLCNTOFFSET(d11cnt_txnocts_off, pwl, pwloffs);
	WLCNTOFFSET(dmade, pwl, pwloffs);
	WLCNTOFFSET(dmada, pwl, pwloffs);
	WLCNTOFFSET(dmape, pwl, pwloffs);
	WLCNTOFFSET(reset, pwl, pwloffs);
	WLCNTOFFSET(tbtt, pwl, pwloffs);
	WLCNTOFFSET(txdmawar, pwl, pwloffs);
	WLCNTOFFSET(pkt_callback_reg_fail, pwl, pwloffs);
	WLCNTOFFSET(txallfrm, pwl, pwloffs);
	WLCNTOFFSET(txrtsfrm, pwl, pwloffs);
	WLCNTOFFSET(txctsfrm, pwl, pwloffs);
	WLCNTOFFSET(txackfrm, pwl, pwloffs);
	WLCNTOFFSET(txdnlfrm, pwl, pwloffs);
	WLCNTOFFSET(txbcnfrm, pwl, pwloffs);
	for (i = 0; i < 8; i++)
		WLCNTOFFSET(txfunfl[i], pwl, pwloffs);
	WLCNTOFFSET(txtplunfl, pwl, pwloffs);
	WLCNTOFFSET(txphyerror, pwl, pwloffs);
	WLCNTOFFSET(rxfrmtoolong, pwl, pwloffs);
	WLCNTOFFSET(rxfrmtooshrt, pwl, pwloffs);
	WLCNTOFFSET(rxinvmachdr, pwl, pwloffs);
	WLCNTOFFSET(rxbadfcs, pwl, pwloffs);
	WLCNTOFFSET(rxbadplcp, pwl, pwloffs);
	WLCNTOFFSET(rxcrsglitch, pwl, pwloffs);
	WLCNTOFFSET(rxstrt, pwl, pwloffs);
	WLCNTOFFSET(rxdfrmucastmbss, pwl, pwloffs);
	WLCNTOFFSET(rxmfrmucastmbss, pwl, pwloffs);
	WLCNTOFFSET(rxcfrmucast, pwl, pwloffs);
	WLCNTOFFSET(rxrtsucast, pwl, pwloffs);
	WLCNTOFFSET(rxctsucast, pwl, pwloffs);
	WLCNTOFFSET(rxackucast, pwl, pwloffs);
	WLCNTOFFSET(rxdfrmocast, pwl, pwloffs);
	WLCNTOFFSET(rxmfrmocast, pwl, pwloffs);
	WLCNTOFFSET(rxcfrmocast, pwl, pwloffs);
	WLCNTOFFSET(rxrtsocast, pwl, pwloffs);
	WLCNTOFFSET(rxctsocast, pwl, pwloffs);
	WLCNTOFFSET(rxdfrmmcast, pwl, pwloffs);
	WLCNTOFFSET(rxmfrmmcast, pwl, pwloffs);
	WLCNTOFFSET(rxcfrmmcast, pwl, pwloffs);
	WLCNTOFFSET(rxbeaconmbss, pwl, pwloffs);
	WLCNTOFFSET(rxdfrmucastobss, pwl, pwloffs);
	WLCNTOFFSET(rxbeaconobss, pwl, pwloffs);
	WLCNTOFFSET(rxrsptmout, pwl, pwloffs);
	WLCNTOFFSET(bcntxcancl, pwl, pwloffs);
	WLCNTOFFSET(rxf0ovfl, pwl, pwloffs);
	WLCNTOFFSET(rxf1ovfl, pwl, pwloffs);
	WLCNTOFFSET(rxf2ovfl, pwl, pwloffs);
	WLCNTOFFSET(txsfovfl, pwl, pwloffs);
	WLCNTOFFSET(pmqovfl, pwl, pwloffs);
	WLCNTOFFSET(rxcgprqfrm, pwl, pwloffs);
	WLCNTOFFSET(rxcgprsqovfl, pwl, pwloffs);
	WLCNTOFFSET(txcgprsfail, pwl, pwloffs);
	WLCNTOFFSET(txcgprssuc, pwl, pwloffs);
	WLCNTOFFSET(prs_timeout, pwl, pwloffs);
	WLCNTOFFSET(rxnack, pwl, pwloffs);
	WLCNTOFFSET(frmscons, pwl, pwloffs);
	WLCNTOFFSET(txnack, pwl, pwloffs);
	WLCNTOFFSET(txfrag, pwl, pwloffs);
	WLCNTOFFSET(txmulti, pwl, pwloffs);
	WLCNTOFFSET(txfail, pwl, pwloffs);
	WLCNTOFFSET(txretry, pwl, pwloffs);
	WLCNTOFFSET(txretrie, pwl, pwloffs);
	WLCNTOFFSET(rxdup, pwl, pwloffs);
	WLCNTOFFSET(txrts, pwl, pwloffs);
	WLCNTOFFSET(txnocts, pwl, pwloffs);
	WLCNTOFFSET(txnoack, pwl, pwloffs);
	WLCNTOFFSET(rxfrag, pwl, pwloffs);
	WLCNTOFFSET(rxmulti, pwl, pwloffs);
	WLCNTOFFSET(rxcrc, pwl, pwloffs);
	WLCNTOFFSET(txfrmsnt, pwl, pwloffs);
	WLCNTOFFSET(rxundec, pwl, pwloffs);
	WLCNTOFFSET(tkipmicfaill, pwl, pwloffs);
	WLCNTOFFSET(tkipcntrmsr, pwl, pwloffs);
	WLCNTOFFSET(tkipreplay, pwl, pwloffs);
	WLCNTOFFSET(ccmpfmterr, pwl, pwloffs);
	WLCNTOFFSET(ccmpreplay, pwl, pwloffs);
	WLCNTOFFSET(ccmpundec, pwl, pwloffs);
	WLCNTOFFSET(fourwayfail, pwl, pwloffs);
	WLCNTOFFSET(wepundec, pwl, pwloffs);
	WLCNTOFFSET(wepicverr, pwl, pwloffs);
	WLCNTOFFSET(decsuccess, pwl, pwloffs);
	WLCNTOFFSET(tkipicverr, pwl, pwloffs);
	WLCNTOFFSET(wepexcluded, pwl, pwloffs);
	WLCNTOFFSET(txchanrej, pwl, pwloffs);
	/* nitro stats */
	WLCNTOFFSET(txnitro, pnitro, pnoffs);
	WLCNTOFFSET(txnitro_fail, pnitro, pnoffs);
	WLCNTOFFSET(txqfull, pnitro, pnoffs);
	WLCNTOFFSET(txnullkeydata, pnitro, pnoffs);
	WLCNTOFFSET(rxmp, pnitro, pnoffs);
	WLCNTOFFSET(rxkeydata, pnitro, pnoffs);
	WLCNTOFFSET(rxnullkeydata, pnitro, pnoffs);
	WLCNTOFFSET(rxbadnitro, pnitro, pnoffs);
	WLCNTOFFSET(rxdupnitro, pnitro, pnoffs);
	WLCNTOFFSET(rxmpack, pnitro, pnoffs);
	WLCNTOFFSET(istatus, pnitro, pnoffs);
	WLCNTOFFSET(retrans, pnitro, pnoffs);
	for (i = 0; i < MAX_NITRO_STA_ALLOWED; i++)
		WLCNTOFFSET(rxkeyerr[i], pnitro, pnoffs);

}
static void
wlc_nin_get_wlcounters(wlc_info_t *wlc, struct wlc_if *wlcif,
	nwm_wl_cnt_t *pwlcnt, wlc_nitro_cnt_t *pnitro, uint16 *result)
{
	int err, len;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);
	void *pbuf = (void *)(pnin->buffer);

	/* fetch the counters first */
	/* Sneaky: nwm_wl_cnt_t is subset of wl_cnt_t */
	len = sizeof(nwm_wl_cnt_t);
	bzero(pbuf, len);
	err = wlc_iovar_op(wlc, "counters", NULL, 0, pbuf, len, IOV_GET, wlcif);
	if (err)
		goto done;
	bcopy(pbuf, pwlcnt, len);

	len = sizeof(wlc_nitro_cnt_t);
	bzero(pbuf, len);
	err = wlc_iovar_op(wlc, "nitro_counters", NULL, 0, pbuf, len, IOV_GET, wlcif);
	if (err)
		goto done;
	bcopy(pbuf, pnitro, len);


done:
	*result = err ? NWM_CMDRES_FAILURE : NWM_CMDRES_SUCCESS;
}
/*
 * retrieve the values of wireless counters
 */
static bool
wlc_dev_getwlinfo_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_dev_getinfo_cfm_t *p = (nwm_dev_getinfo_cfm_t *)pCfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));
	wlc_nin_get_wlcounters(wlc, wlcif, &p->counter, &p->nitro, &p->resultCode);

	/* apply the offsets: our software zero */
	if (p->resultCode == NWM_CMDRES_SUCCESS) {
		nwm_get_pktcnt_t *pktcnt = &p->pktcounters;
		nwm_wl_cnt_t *pcnt = &p->counter;

		wlc_nin_apply_counter_offsets(&p->counter, &p->nitro, wlc);
		/* create the get_pktcnt_t info */
		pktcnt->rx_good_pkt = pcnt->rxframe;
		pktcnt->rx_bad_pkt  = pcnt->rxerror;
		pktcnt->tx_good_pkt = pcnt->txfrmsnt;
		pktcnt->tx_bad_pkt  = pcnt->txerror + pcnt->txfail;
		pktcnt->rx_ocast_good_pkt  = pcnt->rxmfrmocast;
	}
	return TRUE;
}

/* Retrieve the wireless driver state:
 * For an ap
 * CLASS1 == up (WLC_GET_UP)
 * CLASS3 == up && (valid ssid)
 *
 * For a sta
 * CLASS1 == up (WLC_GET_UP)
 * CLASS3 == up && associated
 *
 */
static int
wlc_nin_get_wlstate(wlc_info_t *wlc, struct wlc_if *wlcif)
{
	int err;
	bool up;

	up = (wlc->pub->up || wlc_down_for_mpc(wlc));

	if (!up)
		return NWM_NIN_IDLE;

	/* We're up. What are we: ap/parent or sta/child ? */
	if (AP_ENAB(wlc->pub)) {
		wlc_ssid_t ssidarg;
		bzero(&ssidarg, sizeof(wlc_ssid_t));
		/* up, ap. Got SSID ? */
		err = wlc_ioctl(wlc, WLC_GET_SSID, &ssidarg, sizeof(wlc_ssid_t), wlcif);
		return (ssidarg.SSID_len && (err == 0)) ? NWM_NIN_CLASS3 : NWM_NIN_CLASS1;
	}
	/* up, sta/child. associated ? */
	return wlc->pub->associated ? NWM_NIN_CLASS3 : NWM_NIN_CLASS1;
}

static bool wlc_dev_getstate_cmd(void * pkt, nwm_cmd_req_t * preq,
	nwm_cmd_cfm_t * pcfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	nwm_dev_getstate_cfm_t *p = (nwm_dev_getstate_cfm_t *)pcfm;

	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  preq->header.code, preq->header.length));
	p->state = wlc_nin_get_wlstate(wlc, wlcif);
	p->resultCode = NWM_CMDRES_SUCCESS;
	return TRUE;
}

#include <packed_section_start.h>
typedef BWL_PRE_PACKED_STRUCT struct wlc_testsig {
	int		channel;
	ratespec_t	rate;
	int		txpwr;
} BWL_POST_PACKED_STRUCT wlc_testsig_t;
#include <packed_section_end.h>

static struct wlc_testsig_stat {
	bool busy;
	int	 sigtype;
} testsig_stat;
/* Send specified test signal on specified channel at specified rate */
static bool wlc_dev_testsignal_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	int len, sigtype;
	int arg1, state;
	wlc_testsig_t testargs;
	int err = 0;
	nwm_dev_testsignal_req_t *p = (nwm_dev_testsignal_req_t *)pReq;

	state = wlc_nin_get_wlstate(wlc, wlcif);
	if (state != NWM_NIN_IDLE) {
		err = BCME_ASSOCIATED;
		goto done;
	}

	bzero(&testargs, sizeof(testargs));
	WL_NIN(("wl%d %s called with Command 0x%x Length 0x%x\n",
	wlc->pub->unit, __FUNCTION__,  pReq->header.code, pReq->header.length));

	/*
	 * Test signal type:
	 * WLC_EVM
	 * arg layout:
	 * int		channel
	 * int		rspec
	 * int		txpwr		// not used
	 *
	 * WLC_FREQ_ACCURACY
	 * int		channel
	 *
	 * WLC_CARRIER_SUPPRESS
	 * int		channel
	 *
	 * Channels:
	 * 1-14
	 * Set to zero to turn running test off
	 *
	 * Rate: (EVM only)
	 * Only CCK rates (1,2,5.5,11 MBPS) supported
	 */
	switch (p->signal) {
		case NWM_TEST_EVM:
			len = 3 * sizeof(int);
			sigtype = WLC_EVM;
			/* rate only relevant for evm */
			switch (p->rate) {
				case NWM_RATE_1MBPS:
					testargs.rate = 2;
					break;
				case NWM_RATE_2MBPS:
					testargs.rate = 4;
					break;
				case NWM_RATE_5_5MBPS:
					testargs.rate = 11;
					break;
				case NWM_RATE_11MBPS:
					testargs.rate = 22;
					break;
				default:
					err = BCME_BADARG;
					WL_NIN(("wl%d %s: Bad rate arg %d for evm test\n",
						wlc->pub->unit, __FUNCTION__, p->rate));
					goto done;
			}
			break;

		case NWM_TEST_FREQ_ACC:
			len = sizeof(int);
			sigtype = WLC_FREQ_ACCURACY;
			break;

		case NWM_TEST_SUPP_CARRIER:
			len = sizeof(int);
			sigtype = WLC_CARRIER_SUPPRESS;
			break;
		default:
			err = BCME_BADARG;
			WL_NIN(("wl%d %s: Unrecognized test type %d\n",
				wlc->pub->unit, __FUNCTION__, p->signal));
			goto done;
			break;
	}

	/* Request to run test? */
	if (p->control) {
		if (testsig_stat.busy != FALSE) {
		WL_NIN(("wl%d %s: BUSY running %d\n", wlc->pub->unit, __FUNCTION__, sigtype));
			err = BCME_BUSY;
			goto done;
		}

		testargs.channel = p->channel;
		if (testargs.channel == 0) {
			err = BCME_BADARG;
			WL_NIN(("wl%d %s: ERROR: request to run test with channel = 0\n",
				wlc->pub->unit, __FUNCTION__));
			goto done;
		}

		arg1 = 0;
		err = wlc_ioctl(wlc, WLC_UP, &arg1, sizeof(arg1), wlcif);
		if (err) {
		WL_NIN(("wl%d %s: ioctl err %d on WLC_UP\n",
			wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}
		/* Like WLC_DOWN except clocks keep running */
		arg1 = 0;
		err = wlc_ioctl(wlc, WLC_OUT, &arg1, sizeof(arg1), wlcif);
		if (err) {
		WL_NIN(("wl%d %s: ioctl err %d on WLC_OUT\n",
			wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}
		err = wlc_ioctl(wlc, sigtype, &testargs, len, wlcif);
		if (err) {
		WL_NIN(("wl%d %s: ioctl err %d on sigtype %d channel %d rate %d txpwr %d\n",
			wlc->pub->unit, __FUNCTION__, err, sigtype,
			testargs.channel, testargs.rate, testargs.txpwr));
			goto done;
		}

		testsig_stat.busy = TRUE;
		testsig_stat.sigtype = sigtype;
	} else {
		/* refuse if not busy */
		if (testsig_stat.busy == FALSE) {
			WL_NIN(("wl%d %s: Req to cancel %d with %d busy %d\n",
				wlc->pub->unit, __FUNCTION__,
				testsig_stat.sigtype, sigtype, testsig_stat.busy));
			err = BCME_BADARG;
			goto done;
		}

		/* zero channel: turn off test signal */
		testargs.channel = 0;
		err = wlc_ioctl(wlc, testsig_stat.sigtype, &testargs, len, wlcif);
		if (err) {
			WL_NIN(("wl%d %s: ERROR %d cancelling test %d\n",
				wlc->pub->unit, __FUNCTION__, err, sigtype));
			goto done;
		}
		testsig_stat.busy = FALSE;
		arg1 = 0;
		err = wlc_ioctl(wlc, WLC_DOWN, &arg1, sizeof(arg1), wlcif);
		if (err) {
		WL_NIN(("wl%d %s: ioctl err %d on WLC_DOWN\n",
			wlc->pub->unit, __FUNCTION__, err));
			goto done;
		}
	}

done:
	pCfm->resultCode = nwm_bcmerr_to_nwmerr(err);
	return TRUE;
}

static
bool cmd_reserved_cmd(void * pkt, nwm_cmd_req_t * pReq,
	nwm_cmd_cfm_t * pCfm, wlc_info_t * wlc, struct wlc_if * wlcif)
{
	WL_ERROR(("wl%d %s: unsupported command pReq->header.code 0x%x\n",
		wlc->pub->unit, __FUNCTION__, pReq->header.code));
	pCfm->resultCode = NWM_CMDRES_NOT_SUPPORT;
	return TRUE;
}
int
wlc_nin_ioctl(wlc_info_t *wlc, int cmd, void *buf, int buflen, wlc_if_t *wlcif)
{
	void * newpkt;
	char * pnew;
	uint32 offset;
	bool result;

	WL_NIN(("wl%d %s: buflen %d\n", wlc->pub->unit, __FUNCTION__, buflen));
	/* get the arg buffer into a properly aligned PKT, add 4 for "waste" */
	newpkt = wlc_frame_get_ctl(wlc, buflen + sizeof(uint32));
	if (!newpkt) {
		WL_NIN(("wl%d %s: out of mem, bailing\n", wlc->pub->unit, __FUNCTION__));
		return BCME_NOMEM;
	}
	/* newpkt has the headroom but not necessarily the alignment we need */
	offset = sizeof(uint32) - (((uintptr)(PKTDATA(wlc->osh, newpkt))) & 0x3);
	offset &= 0x03;
	PKTPULL(wlc->osh, newpkt, offset);
	pnew = (char *)PKTDATA(wlc->osh, newpkt);
	WL_NIN(("wl%d %s: alignment is pnew 0x%x \n",
	        wlc->pub->unit, __FUNCTION__, ((uint32)(uintptr)pnew & 0x3)));

	bcopy(buf, pnew, buflen);
	PKTSETLEN(wlc->osh, newpkt, buflen);

	result = wlc_nin_dispatch(wlc, newpkt, wlcif);

	/* get the PKT contents back into the arg buf */
	if (result) {
		pnew = (char *)PKTDATA(wlc->osh, newpkt);
		bcopy(pnew, buf, buflen);
		PKTFREE(wlc->osh, newpkt, TRUE);
	}

	return 0;
}

/* TRUE - packet dropped, FALSE - not dropped */
bool
wlc_nin_process_sendup(wlc_info_t *wlc, void *p)
{
	struct ether_addr ea;
	uint32 status;
	uint32 reason;
	uint16 flags;
	bcm_event_t *msg;
	struct ether_header *pehdr, ehdrbuf;
	nwm_top_indhdr_t *pmgc;
	bool isbrcm_frame;
	bool discard = FALSE;
	int err, nitroarg;
	bool nitromode;
	struct wlc_nin *pnin = (struct wlc_nin *)(wlc->nin);


	/* These are the indications which get a bcm_event_t header replaced with
	 * their own indication header
	 */
	STATIC_ASSERT(sizeof(bcm_event_t) >= NWM_JOIN_IND_SIZE);
	STATIC_ASSERT(sizeof(bcm_event_t) >= NWM_ASSOC_IND_SIZE);
	STATIC_ASSERT(sizeof(bcm_event_t) >= NWM_DISASSOC_IND_SIZE);
	STATIC_ASSERT(sizeof(bcm_event_t) >= NWM_BCNLOST_IND_SIZE);
	STATIC_ASSERT(sizeof(bcm_event_t) >= NWM_BCNSEND_IND_SIZE);
	STATIC_ASSERT(sizeof(bcm_event_t) >= NWM_BCNRCV_IND_SIZE);
	STATIC_ASSERT(sizeof(bcm_event_t) >= NWM_MA_CHANNEL_USE_SIZE);

	pehdr = (struct ether_header *)PKTDATA(wlc->osh, p);
	isbrcm_frame = (pehdr->ether_type == hton16(ETHER_TYPE_BRCM)) ? TRUE : FALSE;

	/* Figure out what we have:
	 * - a properly formatted indication
	 * - an event
	 * - anything else is a received data frame
	 */

	err = wlc_iovar_getint(wlc, "nitro", &nitroarg);
	if (err) {
		nitroarg = 0;
		WL_ERROR(("wl%d %s: error reading nitro settings\n",
			wlc->pub->unit, __FUNCTION__));
	}
	nitromode = (nitroarg != 0) ? TRUE : FALSE;

	pmgc = (nwm_top_indhdr_t *)((char *)PKTDATA(wlc->osh, p) +  ETHER_HDR_LEN);
	/* some of these are nitro frames: drop them if we're not in a nitro mode */
	if (isbrcm_frame && (pmgc->magic == NWM_NIN_IND_MAGIC)) {

		pmgc->magic = 0;
		if (pmgc->indcode == NWM_CMDCODE_MA_MPEND_IND ||
			pmgc->indcode == NWM_CMDCODE_MA_MP_IND ||
			pmgc->indcode == NWM_CMDCODE_MA_MPACK_IND) {
			if (!nitromode) {
				WL_NIN(("wl%d %s: dropping nitro frame: non-nitro mode\n",
					wlc->pub->unit, __FUNCTION__));
				discard = TRUE;
				PKTFREE(wlc->osh, p, FALSE);
			}
		}
		goto done;
	}

	/* grab the header, will need it later */
	bcopy(pehdr, &ehdrbuf, ETHER_HDR_LEN);

	/*
	 * Look for the bcmeth_hdr_t : event
	 * events need the appropriate indication header attached
	 *
	 */
	msg = (bcm_event_t *)(PKTDATA(wlc->osh, p));

	/* All of these indications are for both nitro and wifi modes */
	if (isbrcm_frame && msg->bcm_hdr.subtype == ntoh16(BCMILCP_SUBTYPE_VENDOR_LONG)) {
		bcopy(msg->event.addr.octet, ea.octet, ETHER_ADDR_LEN);
		status = ntoh32(msg->event.status);
		reason = ntoh32(msg->event.reason);
		flags  = ntoh16(msg->event.flags);
	WL_NIN(("wl%d %s Event 0x%x Status 0x%x Reason 0x%x\n",
	wlc->pub->unit, __FUNCTION__, ntoh32(msg->event.event_type),
	status, reason));
		/* manufacture the corresponding indication */
		switch (ntoh32(msg->event.event_type)) {

			case WLC_E_ASSOC_IND :
			case WLC_E_REASSOC_IND :
				/* The fun starts.
				 * IF the status === success, We need to:
				 * -1- scan our scb list to find the aid (match mac addr)
				 * -2- fetch the SSID
				 */
			{
				wlc_ssid_t ssidarg;
				nwm_mlme_assoc_ind_t *pind =
					(nwm_mlme_assoc_ind_t *)&msg->bcm_hdr;

				/* retrieve the data from the event msg */
				ssidarg.SSID_len = ntoh32(msg->event.datalen);
				bcopy((char *)(msg + 1), ssidarg.SSID, ssidarg.SSID_len);

				bzero(pind, NWM_ASSOC_IND_SIZE);
				pind->header.code = NWM_CMDCODE_MLME_ASSOC_IND;
				pind->header.length = NWM_ASSOC_IND_LENGTH;
				if (status == WLC_E_STATUS_SUCCESS) {
					struct scb *scb;

					ASSERT(wlc->bsscfg[WLPKTTAGBSSCFGGET(p)] != NULL);
					scb = wlc_scbfind(wlc,
					          wlc->bsscfg[WLPKTTAGBSSCFGGET(p)], &ea);
					/* Got an assoc indication but no scb? */
					ASSERT(scb);
					/* fill in fields */
					{

						pind->aid = AID2PVBMAP(scb->aid);
						bcopy(ea.octet, (char *)pind->mac_addr,
							ETHER_ADDR_LEN);
						pind->ssid_len = ssidarg.SSID_len;
						bcopy(ssidarg.SSID, (char *)pind->ssid,
							ssidarg.SSID_len);
					}

				} else {
					PKTFREE(wlc->osh, p, FALSE);
					discard = TRUE;
					break;

				}
				PKTSETLEN(wlc->osh, p, NWM_ASSOC_IND_SIZE + ETHER_HDR_LEN);
				break;
			}
			case WLC_E_SET_SSID :
			{
				nwm_mlme_join_ind_t *pind = (nwm_mlme_join_ind_t *)&msg->bcm_hdr;

				bzero(pind, NWM_JOIN_IND_SIZE);
				pind->header.code = NWM_CMDCODE_MLME_JOINCMPLT_IND;
				pind->header.length = NWM_JOIN_IND_LENGTH;
				pind->resultcode = (status == WLC_E_STATUS_SUCCESS)
					?  NWM_CMDRES_SUCCESS : NWM_CMDRES_FAILURE;

				bcopy(ea.octet, (char *)pind->bssid, ETHER_ADDR_LEN);
				pind->aid = AID2PVBMAP(wlc->AID);	/* THIS station's aid */
				PKTSETLEN(wlc->osh, p, NWM_JOIN_IND_SIZE + ETHER_HDR_LEN);
				break;
			}
			case WLC_E_DISASSOC_IND :
			case WLC_E_DISASSOC :
/* Nintendo request: remap deauth to disassoc */
			case WLC_E_DEAUTH_IND :
			case WLC_E_DEAUTH :
			{
				nwm_mlme_disassoc_ind_t *pind =
					(nwm_mlme_disassoc_ind_t *)&msg->bcm_hdr;
				bzero(pind, NWM_DISASSOC_IND_SIZE);
				pind->header.code = NWM_CMDCODE_MLME_DISASSOC_IND;
				pind->header.length = NWM_DISASSOC_IND_LENGTH;
				pind->reason_code = reason;
				bcopy(ea.octet, (char *)pind->mac_addr, ETHER_ADDR_LEN);
				PKTSETLEN(wlc->osh, p, NWM_DISASSOC_IND_SIZE + ETHER_HDR_LEN);

				break;
			}

			/* This one triggers a channel use indication for BT coexistence */
			case WLC_E_LINK :
			{
				int err;
				channel_info_t chinfoarg;
				nwm_ma_channel_use_ind_t *pind =
					(nwm_ma_channel_use_ind_t *)&msg->bcm_hdr;

				bzero(pind, NWM_MA_CHANNEL_USE_SIZE);
				pind->header.indcode = NWM_CMDCODE_MA_CHANNEL_USE_IND;
				pind->header.indlength = NWM_CHANNEL_USE_IND_LENGTH;
				pind->on_off = (flags & WLC_EVENT_MSG_LINK) ? 1 : 0;
				bzero(&chinfoarg, sizeof(chinfoarg));
				err = wlc_ioctl(wlc, WLC_GET_CHANNEL, &chinfoarg,
					sizeof(channel_info_t), NULL);
				if (err) {
					WL_ERROR((
						"wl%d %s: WLC_GET_CHANNEL failed"
						"for WLC_E_LINK evt\n",
						wlc->pub->unit, __FUNCTION__));
					pind->channel = 0;
				} else {
					pind->channel = chinfoarg.target_channel;
				}
				break;
			}

			case WLC_E_BCNLOST_MSG :
			{
				nwm_mlme_beaconlost_ind_t *pind =
					(nwm_mlme_beaconlost_ind_t *)&msg->bcm_hdr;
				bzero(pind, NWM_BCNLOST_IND_SIZE);
				pind->header.code = NWM_CMDCODE_MLME_BCLOST_IND;
				pind->header.length = NWM_BCNLOST_IND_LENGTH;

				bcopy(ea.octet, (char *)pind->peer_mac, ETHER_ADDR_LEN);
				PKTSETLEN(wlc->osh, p, NWM_BCNLOST_IND_SIZE + ETHER_HDR_LEN);
				break;
			}

			case WLC_E_BCNSENT_IND :
			{
				nwm_mlme_beaconsend_ind_t *pind =
					(nwm_mlme_beaconsend_ind_t *)&msg->bcm_hdr;

				bzero(pind, NWM_BCNSEND_IND_SIZE);
				pind->header.code = NWM_CMDCODE_MLME_BCSEND_IND;
				pind->header.length = NWM_BCNSEND_IND_LENGTH;
				PKTSETLEN(wlc->osh, p, NWM_BCNSEND_IND_SIZE + ETHER_HDR_LEN);
				break;
			}

			case WLC_E_BCNRX_MSG :
			{
				/* This is for STA/child ONLY */
				/* use wlc_scbfind(wlc, ea) */
				struct scb *scb;

				nwm_mlme_beaconrecv_ind_t *pind =
					(nwm_mlme_beaconrecv_ind_t *)&msg->bcm_hdr;

				nitro_gameIE_t *nin_ie;
				nitro_gameIE_t *ie_buf = (nitro_gameIE_t *)(pnin->buffer);
				uint32 ielen = ntoh32(msg->event.datalen);
				uint32 indlen;


				/* our data is at the end of the msg structure */
				nin_ie = (nitro_gameIE_t *)(msg + 1);
				/* ielen can be zero */
				bcopy(nin_ie, (char *)ie_buf, ielen);
				indlen =
					NWM_BCNRECV_IND_FIXED_SIZE;

				if (ielen)
					indlen += ielen - NITRO_GAMEIE_FIXEDLEN;


				bzero(pind, indlen);
				pind->header.code = NWM_CMDCODE_MLME_BCRECV_IND;
				pind->header.length = NWM_BCNRECV_IND_FIXED_LEN;

				bcopy(ea.octet, (char *)pind->src_mac_adrs, ETHER_ADDR_LEN);

				if (ielen) {
					uint16 tmp;
					bcopy((char *)&ie_buf->vblankTsf, (char *)&tmp,
						sizeof(ie_buf->vblankTsf));
					pind->vblank_tsf = ltoh16(tmp);

					pind->game_info_length =
						ie_buf->len - (NITRO_GAMEIE_FIXEDLEN - TLV_HDR_LEN);
					bcopy(ie_buf->gameInfo, (char *)pind->game_info,
						pind->game_info_length);
					pind->header.length += (pind->game_info_length/2);
				}
				pind->rssi = wlc->cfg->link->rssi;
				/* Should be the latest, just grab it */

				ASSERT(wlc->bsscfg[WLPKTTAGBSSCFGGET(p)] != NULL);
				scb = wlc_scbfind(wlc, wlc->bsscfg[WLPKTTAGBSSCFGGET(p)], &ea);
				/* Getting a beacon but no scb ? */
				ASSERT(scb);
				if (scb->rate_histo)
					pind->rate =
					scb->rate_histo->rxrspec[scb->rate_histo->rxrspecidx]
							 & RSPEC_RATE_MASK;

				PKTSETLEN(wlc->osh, p, indlen + ETHER_HDR_LEN);
				break;
			}

			default :
				/* discard */
				WL_NIN((
					"wl%d:%s: dropping indication %d\n",
					wlc->pub->unit, __FUNCTION__,
					ntoh32(msg->event.event_type)));
				PKTFREE(wlc->osh, p, FALSE);
				discard = TRUE;
				break;
		} /* end switch */

	} else {

		/*
		 * Anything else: a data frame
		 * data frames need a Nintendo receive data indication header
		 *
		 */
		nwm__madataind_hdr_t *phdr;
		int payloadlen;
		struct ether_header *eth;
		int hdrm = PKTHEADROOM(wlc->osh, p);
		int hdrm_reqd = sizeof(nwm__madataind_hdr_t) + ETHER_HDR_LEN;


		/* There should be just enough, make sure */

		if (hdrm < hdrm_reqd) {
			WL_ERROR(("wl%d %s: insufficient pkt headroom: %d, Need %d\n",
				wlc->pub->unit, __FUNCTION__, hdrm, hdrm_reqd));
			discard = TRUE;
			PKTFREE(wlc->osh, p, FALSE);
			goto done;
		}

		if (nitromode) {
			WL_NIN(("wl%d %s: dropping non-nitro frame: nitro mode\n",
				wlc->pub->unit, __FUNCTION__));
			discard = TRUE;
			PKTFREE(wlc->osh, p, FALSE);
			goto done;
		}

		/* keep the whole frame, just stack an indication hdr on top of it */
		payloadlen = PKTLEN(wlc->osh, p);
		PKTPUSH(wlc->osh, p, NWM_MADATA_RCV_HDR_SIZE);
		phdr = (nwm__madataind_hdr_t *)PKTDATA(wlc->osh, p);

		bzero(phdr, NWM_MADATA_RCV_HDR_SIZE);
		phdr->header.indcode = NWM_CMDCODE_MA_DATA_IND;
		phdr->header.indlength = payloadlen;

		/* Add a BRCM ether header to it */
		eth = (struct ether_header *)PKTPUSH(wlc->osh, p, ETHER_HDR_LEN);
		bcopy(&wlc->pub->cur_etheraddr, (char *)&eth->ether_dhost, ETHER_ADDR_LEN);
		bcopy(&wlc->pub->cur_etheraddr, (char *)&eth->ether_shost, ETHER_ADDR_LEN);
		ETHER_SET_LOCALADDR(&eth->ether_shost);
		eth->ether_type = hton16(ETHER_TYPE_BRCM);
	}

done:
	return discard;

}

/* Create an error indication and send it up:
 * type is indication type
 * pdata points to type specific data to be copied in
 */
static void
wlc_nin_create_errind(wlc_info_t * wlc, char *pdata, int datalen,
	struct wlc_if *wlcif, uint16 type)
{
	void *p;
	int pktlen;
	nwm_ma_fatalerr_ind_t *pind;


	/* pad to make sure we can put pkt data at 4 byte boundary */
	pktlen = NWM_FATALERR_IND_FIXED_LENGTH + datalen + ETHER_HDR_LEN + sizeof(uint32);


	p = PKTGET(wlc->osh, pktlen, TRUE);
	if (p == NULL) {
		WL_NIN(("wl%d %s: failed to allocate pkt for error indication\n",
			wlc->pub->unit, __FUNCTION__));
		return;
	}
	pind = (nwm_ma_fatalerr_ind_t *)PKTPULL(wlc->osh, p, ROUNDUP(ETHER_HDR_LEN, 4));

	bzero(pind, NWM_FATALERR_IND_FIXED_LENGTH + datalen);
	pind->header.magic     = NWM_NIN_IND_MAGIC;
	pind->header.indcode   = NWM_CMDCODE_MA_FATAL_ERR_IND;
	pind->header.indlength = (NWM_FATALERR_INDLENGTH + datalen + 1)/2;
	pind->errcode          = type;
	bcopy(pdata, (char *)pind->data, datalen);

	/* send it up */
	wlc_nin_sendup(wlc, p, wlcif);

	return;
}
