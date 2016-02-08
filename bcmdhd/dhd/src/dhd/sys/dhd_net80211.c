/*
 * Broadcom Dongle Host Driver (DHD)
 * Broadcom 802.11bang Networking Device Driver
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: dhd_net80211.c 335620 2012-05-29 19:01:21Z $
 */

#include <typedefs.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_media.h>
#include <net/if_var.h>
#include <net/route.h>
#include <sys/kernel.h>
#include <sys/ioccom.h>
#include <sys/callout.h>
#include <net80211/ieee80211_ioctl.h>
#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_regdomain.h>

#undef WME_OUI
#undef WME_OUI_TYPE
#undef WPA_OUI
#undef WPA_OUI_TYPE
#undef WPS_OUI
#undef WPS_OUI_TYPE
#undef RSN_CAP_PREAUTH

#include <bcmutils.h>
#include <bcmendian.h>
#include <osl.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <dhd_dbg.h>
#include <proto/eapol.h>
#include <bcmwpa.h>
#include <bcmwifi_channels.h>
#include <dhd_net80211.h>

typedef struct dhd_net80211 {
	struct ifnet *ifp;
	struct callout  scan_timer;		/* scan timer */
} dhd_net80211_t;


static inline void
wl_bss_info_convert(wl_bss_info_t *bi)
{
#if defined(IL_BIGENDIAN)
	bi->version		= ltoh32(bi->version);
	bi->length		= ltoh32(bi->length);
	bi->beacon_period	= ltoh16(bi->beacon_period);
	bi->capability		= ltoh16(bi->capability);
	bi->rateset.count	= ltoh32(bi->rateset.count);
	bi->chanspec		= ltoh16(bi->chanspec);
	bi->atim_window		= ltoh16(bi->atim_window);
	bi->RSSI		= ltoh16(bi->RSSI);
	bi->nbss_cap		= ltoh32(bi->nbss_cap);
	bi->vht_rxmcsmap	= ltoh16(bi->vht_rxmcsmap);
	bi->vht_txmcsmap	= ltoh16(bi->vht_txmcsmap);
	bi->ie_offset		= ltoh16(bi->ie_offset);
	bi->ie_length		= ltoh32(bi->ie_length);
	bi->SNR			= ltoh16(bi->SNR);
#endif
}


static int
_net80211_doiovar(dhd_pub_t *dhdp, char *iov_name, int cmd,
	char *data, int data_len, char *iov_buf, int iov_buflen, int set)
{
	int len;
	int error = BCME_OK;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	DHD_TRACE(("%s(): Calling bcm_mkiovar() \n", __FUNCTION__));
	len = bcm_mkiovar(iov_name, data, data_len, iov_buf, iov_buflen);
	if (len == 0) {
		error = BCME_ERROR;
		DHD_ERROR(("%s(): ERROR.. bcm_mkiovar() returns %d for cmd '%s' \n",
			__FUNCTION__, error, iov_name));
		goto done;
	}
	if (set == IOV_SET) {
		error = dhd_wl_ioctl_cmd(dhdp, cmd, iov_buf, len, WL_IOCTL_ACTION_SET, 0);
		if (error != BCME_OK) {
			DHD_ERROR(("%s(): ERROR.. dhd_wl_ioctl_cmd() returns %d for cmd '%s' \n",
				__FUNCTION__, error, iov_name));
			goto done;
		}

	}
	else if (set == IOV_GET) {
		DHD_TRACE(("%s(): Calling dhd_wl_ioctl_cmd() \n", __FUNCTION__));
		error = dhd_wl_ioctl_cmd(dhdp, cmd, iov_buf, iov_buflen, WL_IOCTL_ACTION_GET, 0);
		if (error != BCME_OK) {
			DHD_ERROR(("%s(): ERROR.. dhd_wl_ioctl_cmd() returns %d for cmd '%s' \n",
				__FUNCTION__, error, iov_name));
			goto done;
		}
	}
	else {
		DHD_ERROR(("%s(): ERROR.. Unknown operation %d for cmd %s \n",
			__FUNCTION__, set, iov_name));
	}

done:
	DHD_TRACE(("%s(): Returns %d \n", __FUNCTION__, error));
	return error;
}


static int
_net80211_bss2ieee_scanresults(dhd_pub_t *dhdp, wl_bss_info_t *bi,
	struct ieee80211req_scan_result *sr, unsigned int sr_buf_size)
{
	int channel, len, chan_bitmap, index;
	int error = BCME_OK;
	uint8 *temp_ptr;
	char chan_buf[32];

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	len = sizeof(struct ieee80211req_scan_result) + bi->SSID_len + bi->ie_length;
	if (sr_buf_size < len) {
		error = BCME_NORESOURCE;
		goto done;
	}

	sr->isr_len  = roundup2(len, 4);
	sr->isr_ie_off = sizeof(struct ieee80211req_scan_result);

	channel      = bi->chanspec & WL_CHANSPEC_CHAN_MASK;
	sr->isr_freq = dhd_channel2freq(channel);

	/* Form isr_flags for the channel */
	if (channel < 14)
		channel |= WL_CHANSPEC_BAND_2G;
	else
		channel |= WL_CHANSPEC_BAND_5G;

	channel = htol32(channel);

	/* Get per_chan_info */
	DHD_TRACE(("%s(): Getting per_chan_info \n", __FUNCTION__));
	error = _net80211_doiovar(dhdp, "per_chan_info", WLC_GET_VAR,
		(char *)&channel, sizeof(channel), (char *)chan_buf, sizeof(chan_buf), IOV_GET);
	if (error != BCME_OK) {
		goto done;
	}

	chan_bitmap = ltoh32(*((uint *)chan_buf));

	DHD_TRACE(("%s(): channel = 0x%x,  chan_bitmap = 0x%x \n",
		__FUNCTION__, ltoh32(channel), chan_bitmap));

	if ((chan_bitmap & WL_CHAN_VALID_HW) && (chan_bitmap & WL_CHAN_VALID_SW)) {
		sr->isr_flags = ((chan_bitmap & WL_CHAN_BAND_5G) ?
			IEEE80211_CHAN_5GHZ: IEEE80211_CHAN_2GHZ);
		if (chan_bitmap & WL_CHAN_PASSIVE)
			sr->isr_flags |= IEEE80211_CHAN_PASSIVE;
	}

	bcopy(&bi->BSSID, sr->isr_bssid, IEEE80211_ADDR_LEN);

	sr->isr_capinfo = (u_int8_t)bi->capability;
	sr->isr_intval  = (u_int8_t)bi->beacon_period;
	sr->isr_nrates  = (u_int8_t)MIN(bi->rateset.count, IEEE80211_RATE_MAXSIZE);

	for (index = 0; index < sr->isr_nrates; index++)
		sr->isr_rates[index] = (u_int8_t)(bi->rateset.rates[index]);

	sr->isr_noise    = bi->phy_noise;
	sr->isr_rssi     = bi->RSSI;
	sr->isr_ssid_len = bi->SSID_len;
	sr->isr_ie_len   = bi->ie_length;

	/* variable length SSID followed by IE data */
	temp_ptr = (uint8 *)((uint8*)sr + sizeof(struct ieee80211req_scan_result));
	bcopy(bi->SSID, temp_ptr, bi->SSID_len);

	temp_ptr += bi->SSID_len;
	bcopy((uint8*)(((uint8 *)bi) + bi->ie_offset), temp_ptr, bi->ie_length);

done:
	DHD_TRACE(("%s(): Returns %d \n", __FUNCTION__, error));
	return (error);
}


static void
_net80211_scan_timeout_cb(void *arg)
{
	dhd_pub_t *dhdp = (dhd_pub_t *)arg;
	dhd_net80211_t *net80211_ctxt;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	net80211_ctxt = (dhd_net80211_t *)dhdp->net80211_ctxt;

	/* callout_stop(&net80211_ctxt->scan_timer); */
	rt_ieee80211msg(net80211_ctxt->ifp, RTM_IEEE80211_SCAN, NULL, 0);

	return;
}

#ifdef NOT_YET
static int
dhd_stainfo2ieee_stainfo(dhd_pub_t *dhdp, struct ether_addr *ea, int *space, uint8 *p, int *bytes)
{
	sta_info_t sta;
	struct ieee80211req_sta_info ieeesta;
	int error, index;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	bzero(&sta, sizeof(sta));
	bzero(&ieeesta, sizeof(ieeesta));

	error = _net80211_doiovar(dhdp, "sta_info", WLC_GET_VAR, NULL, 0,
		(char *)&sta, sizeof(sta), IOV_GET);
	if (error != BCME_OK) {
		goto done;
	}

	ASSERT(sta.ver == WL_STA_VER);

	/* Not enough space */
	if (sta.len > *space) {
		*bytes = 0;
		error = BCME_NORESOURCE;
		goto done;
	}

	/* Update ieeesta fields */
	if (sta.flags & WL_STA_AUTHO)
		ieeesta.isi_flags |= IEEE80211_NODE_AUTH;
	if (sta.flags & WL_STA_WME)
		ieeesta.isi_flags |= IEEE80211_NODE_QOS;
	if (sta.flags & WL_STA_PS)
		ieeesta.isi_flags |= IEEE80211_NODE_PWR_MGT;

	bcopy(&sta.ea, ieeesta.isi_macaddr, ETHER_ADDR_LEN);
	ieeesta.isi_nrates = (u_int8_t) MIN(sta.rateset.count, IEEE80211_RATE_MAXSIZE);

	for (index = 0; index < ieeesta.isi_nrates; index++)
		ieeesta.isi_rates[index] = (u_int8_t) (sta.rateset.rates[index]);
	ieeesta.isi_inact = sta.idle;
	ieeesta.isi_capinfo = (u_int8_t)sta.cap;
	ieeesta.isi_len = sta.len;

	/* Update bytes written and space left */
	*bytes = ieeesta.isi_len;
	*space -= ieeesta.isi_len;
	error = copyout(&ieeesta, p, ieeesta.isi_len);

done:
	return (error);
}

/* IEEE80211_IOC_STA_INFO */
static int
dhd_net80211_stainfo_get(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_sta_req sta_req;
	struct ether_addr ea;
	u_int8_t *p;
	int space, index, bytes = 0, error;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if (ireq->i_len < sizeof(struct ieee80211req_sta_req)) {
		error = BCME_NORESOURCE;
		goto done;
	}
	if ((error = copyin(ireq->i_data, &sta_req, sizeof(struct ieee80211req_sta_req))))
		goto done;

	bcopy(sta_req.is_u.macaddr, &ea, ETHER_ADDR_LEN);

	p = ireq->i_data;
	space = ireq->i_len;

	/* If bcast mac address, then get the sta list and iterate through it */
	if (ETHER_ISMULTI(&ea)) {
		struct maclist *maclist;
		int max_count = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;

		if (!(maclist = MALLOC(dhdp->osh, WLC_IOCTL_MAXLEN))) {
			error = BCME_NOMEM;
			goto done;
		}

		bzero(maclist, WLC_IOCTL_MAXLEN);
		maclist->count = max_count;

		/* Get all the authenticated STAs */
		error = _net80211_doiovar(dhdp, "authe_sta_list", WLC_GET_VAR, NULL, 0,
			(char *)maclist, WLC_IOCTL_MAXLEN, IOV_GET);
		if (error != BCME_OK) {
			MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
			goto done;
		}

		if (maclist->count == 0) {
			MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
			ireq->i_len = 0;
			error = BCME_OK;
			goto done;
		}

		for (index = 0; index < maclist->count; index++) {
			error = dhd_stainfo2ieee_stainfo(dhdp, &maclist->ea[index],
				&space, p, &bytes);
			if (error != BCME_OK) {
				MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
				goto done;
			}
			p += bytes;
		}
		MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);

	}
	else {
		error = dhd_stainfo2ieee_stainfo(dhdp, &ea, &space, p, &bytes);
		if (error != BCME_OK) {
			/* MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN); */
			goto done;
		}
	}

	/* Adjust returned length */
	ireq->i_len -= space;

done:
	return error;
}

/* IEEE80211_IOC_CHANLIST */
static int
dhd_net80211_chanlist_get(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int error, index;
	uint32 buf[WL_NUMCHANNELS + 1];
	uint8 channels[WL_NUMCHANNELS + 1];
	wl_uint32_list_t *list;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	list = (wl_uint32_list_t *)buf;
	list->count = WL_NUMCHANNELS;

	error = _net80211_doiovar(dhdp, "channels", WLC_GET_VALID_CHANNELS, NULL, 0,
		(char *)buf, sizeof(buf), IOV_GET);
	if (error != BCME_OK) {
		goto done;
	}

	/* Convert from uint32 array to uint 8 before copyout */
	for (index = 0; index < list->count; index++)
		channels[index] = (uint8)list->element[index];

	error = copyout(channels, ireq->i_data, MIN(list->count * sizeof(uint8), ireq->i_len));

done:
	return (error);
}

/* IEEE80211_IOC_WPAIE */
static int
dhd_net80211_wpaie_get(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_wpaie wpaie;
	uint8 *buf;
	int error, ielen;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if (ireq->i_len < IEEE80211_ADDR_LEN) {
		error = BCME_BADLEN;
		goto done;
	}

	if ((error = copyin(ireq->i_data, wpaie.wpa_macaddr, IEEE80211_ADDR_LEN)))
		goto done;

	if (!(buf = (uint8 *) MALLOC(dhdp->osh, IEEE80211_MAX_OPT_IE))) {
		error = BCME_NOMEM;
		goto done;
	}

	/* need to pass mac addr as input ??? */
	error = _net80211_doiovar(dhdp, "wpaie", 0, NULL, 0, buf, IEEE80211_MAX_OPT_IE, IOV_GET);
	if (error != BCME_OK) {
		MFREE(dhdp->osh, buf, IEEE80211_MAX_OPT_IE);
		goto done;
	}

	bzero(wpaie.wpa_ie, sizeof(wpaie.wpa_ie));

	ielen = buf[1] + 2;
	if (ielen > sizeof(wpaie.wpa_ie))
		ielen = sizeof(wpaie.wpa_ie);

	memcpy(wpaie.wpa_ie, buf, ielen);

	MFREE(dhdp->osh, buf, IEEE80211_MAX_OPT_IE);

	if (ireq->i_len > sizeof(wpaie))
		ireq->i_len = sizeof(wpaie);

	error = copyout(&wpaie, ireq->i_data, ireq->i_len);

done:
	return (error);
}


/* IEEE80211_IOC_ADDMAC,IEEE80211_IOC_DELMAC
 * Add /remove Mac to the list/from the list
 */
static int
dhd_net80211_mac_update(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct maclist *maclist;
	int max_count = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;
	u_int8_t mac[IEEE80211_ADDR_LEN];
	int index, error, len;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if (ireq->i_len != sizeof(mac)) {
		error = BCME_BADLEN;
		goto done;
	}

	error = copyin(ireq->i_data, mac, ireq->i_len);
	if (error) {
		error = BCME_ERROR;
		goto done;
	}

	/* Get the current maclist first */
	if (!(maclist = MALLOC(dhdp->osh, WLC_IOCTL_MAXLEN))) {
		error = BCME_NOMEM;
		goto done;
	}

	bzero(maclist, WLC_IOCTL_MAXLEN);
	maclist->count = max_count;

	error = _net80211_doiovar(dhdp, "mac", WLC_GET_MACLIST, NULL, 0,
		(char *)&maclist, sizeof(maclist), IOV_GET);
	if (error != BCME_OK) {
		MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
		goto done;
	}

	/* Add if it does not exist
	 * Remove if it does
	 */
	for (index = 0; index < maclist->count; index++)
		if (!(bcmp(&maclist->ea[index], mac, ETHER_ADDR_LEN)))
			break;

	switch (ireq->i_type) {
		case IEEE80211_IOC_ADDMAC:
			/* Found match. Do nothing */
			if (index != maclist->count) {
				MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
				error = BCME_OK;
				goto done;
			}

			/* No more space */
			if (index == max_count) {
				MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
				error = BCME_ERROR;
				goto done;
			}

			bcopy(mac, &maclist->ea[index], ETHER_ADDR_LEN);
			maclist->count++;
			break;

		case IEEE80211_IOC_DELMAC:
			/* Did not find a match , do nothing */
			if (index == maclist->count) {
				MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
				error = BCME_OK;
				goto done;
			}

			/* Shift the remaining list */
			if (index != (maclist->count - 1)) {
				for (; index < maclist->count - 1; index++) {
					bcopy(&maclist->ea[index+1],
						&maclist->ea[index], ETHER_ADDR_LEN);
				}
			}
			maclist->count--;
			break;

		default:
			error = BCME_UNSUPPORTED;
			break;
	}

	if (error == BCME_OK) {
		/* Set new list */
		len = sizeof(maclist->count) + maclist->count * sizeof(maclist->ea);
		error = _net80211_doiovar(dhdp, "mac", WLC_GET_MACLIST, NULL, 0,
			(char *)&maclist, sizeof(maclist), IOV_SET);
		MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
	}

done:
	return error;
}

/* IEEE80211_IOC_WME_CWMIN, IEEE80211_IOC_WME_CWMAX, IEEE80211_IOC_WME_AIFS,
 * IEEE80211_IOC_WME_TXOPLIMIT, IEEE80211_IOC_WME_ACM, IEEE80211_IOC_WME_ACKPOLICY
 */
static int
dhd_net80211_wmeparam_get(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int ac = (ireq->i_len & IEEE80211_WMEPARAM_VAL);
	edcf_acparam_t acps[AC_COUNT], *acp;
	int error = 0, int_val;
	char *iov_name; /* IOVAR to use based on request and mode */

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if (ac >= WME_NUM_AC)
		ac = WME_AC_BE;

	/* Determine whether requesting bss params or local params of the sta or ap */
	if (ireq->i_len & IEEE80211_WMEPARAM_BSS || !WLIF_AP_MODE(dhdp))
		iov_name = "wme_ac_sta";
	else
		iov_name = "wme_ac_ap";

	/* Get all the ac parameters so that we can just modify the relevant one */
	error = _net80211_doiovar(dhdp, iov_name, 0, NULL, 0, (char *)acps, sizeof(acps), IOV_GET);
	if (error != BCME_OK)
		goto done;

	acp = &acps[ac];

	/* Decide the iovar based on AP or STA mode */
	switch (ireq->i_type) {
		case IEEE80211_IOC_WME_CWMIN:		/* WME: CWmin */
			ireq->i_val = acp->ECW & EDCF_ECWMIN_MASK;
			break;
		case IEEE80211_IOC_WME_CWMAX:		/* WME: CWmax */
			ireq->i_val = (acp->ECW & EDCF_ECWMAX_MASK) >> EDCF_ECWMAX_SHIFT;
			break;
		case IEEE80211_IOC_WME_AIFS:		/* WME: AIFS */
			ireq->i_val = acp->ACI & EDCF_AIFSN_MASK;
			break;
		case IEEE80211_IOC_WME_TXOPLIMIT:	/* WME: txops limit */
			ireq->i_val = acp->TXOP;
			break;
		case IEEE80211_IOC_WME_ACM:		/* WME: ACM (bss only) */
			ireq->i_val = (acp->ACI & EDCF_ACM_MASK)? 1 : 0;
			break;
		case IEEE80211_IOC_WME_ACKPOLICY:	/* WME: ACK policy (!bss only) */
			error = _net80211_doiovar(dhdp, "wme_noack", 0, NULL, 0,
				(char *)&int_val, sizeof(int_val), IOV_GET);
			if (error != BCME_OK)
				goto done;
			ireq->i_val = int_val;
			break;
	}

done:
	return error;
}


/* IEEE80211_IOC_WME_CWMIN, IEEE80211_IOC_WME_CWMAX, IEEE80211_IOC_WME_AIFS,
 * IEEE80211_IOC_WME_TXOPLIMIT, IEEE80211_IOC_WME_ACM, IEEE80211_IOC_WME_ACKPOLICY
 */
static int
dhd_net80211_wmeparam_set(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int ac = (ireq->i_len & IEEE80211_WMEPARAM_VAL), aifsn, ecwmin, ecwmax, txop;
	edcf_acparam_t acps[AC_COUNT], *acp;
	int error = 0, int_val;
	char *iov_name; /* IOVAR to use based on request and mode */

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if (ac >= WME_NUM_AC)
		ac = WME_AC_BE;

	if (ireq->i_len & IEEE80211_WMEPARAM_BSS)
		iov_name = "wme_ac_sta";
	else
		iov_name = "wme_ac_ap";

	/* Get all the ac parameters so that we can just modify the relevant one */
	error = _net80211_doiovar(dhdp, iov_name, 0, NULL, 0, (char *)acps, sizeof(acps), IOV_GET);
	if (error != BCME_OK)
		goto done;

	acp = &acps[ac];

	/* Decide the iovar based on AP or STA mode */
	switch (ireq->i_type) {
		case IEEE80211_IOC_WME_CWMIN:		/* WME: CWmin */
			ecwmin = ireq->i_val;
			if (ecwmin >= EDCF_ECW_MIN && ecwmin <= EDCF_ECW_MAX) {
				acp->ECW = ((acp->ECW & EDCF_ECWMAX_MASK) |
				         (ecwmin & EDCF_ECWMIN_MASK));
			}
			else {
				error = BCME_BADLEN;
			}
			break;

		case IEEE80211_IOC_WME_CWMAX:		/* WME: CWmax */
			ecwmax = ireq->i_val;
			if (ecwmax >= EDCF_ECW_MIN && ecwmax <= EDCF_ECW_MAX) {
				acp->ECW = ((ecwmax << EDCF_ECWMAX_SHIFT) & EDCF_ECWMAX_MASK) |
				        (acp->ECW & EDCF_ECWMIN_MASK);
			}
			else {
				error = BCME_BADLEN;
			}
			break;

		case IEEE80211_IOC_WME_AIFS:		/* WME: AIFS */
			aifsn = ireq->i_val;
			if (aifsn >= EDCF_AIFSN_MIN && aifsn <= EDCF_AIFSN_MAX) {
				acp->ACI = (acp->ACI & ~EDCF_AIFSN_MASK) |
				        (aifsn & EDCF_AIFSN_MASK);
			}
			else {
				error = BCME_BADLEN;
			}
			break;

		case IEEE80211_IOC_WME_TXOPLIMIT:	/* WME: txops limit */
			txop = ireq->i_val;
			if (txop >= EDCF_TXOP_MIN && txop <= EDCF_TXOP_MAX)
				acp->TXOP = txop;
			else
				error = BCME_BADLEN;
			break;

		case IEEE80211_IOC_WME_ACM:		/* WME: ACM (bss only) */
			if (ireq->i_val == 1)
				acp->ACI |= EDCF_ACM_MASK;
			else
				acp->ACI &= ~EDCF_ACM_MASK;
			break;

		case IEEE80211_IOC_WME_ACKPOLICY:	/* WME: ACK policy (!bss only) */
			int_val = ireq->i_val;
			error = _net80211_doiovar(dhdp, "wme_noack", 0, NULL, 0,
				(char *)&int_val, sizeof(int_val), IOV_SET);
			goto done;

		default:
			error = BCME_UNSUPPORTED;
			break;
	}

	if (error == BCME_OK) {
		/* Write back the updated value */
		error = _net80211_doiovar(dhdp, iov_name, 0, NULL, 0,
			(char *)acp, sizeof(acp), IOV_SET);
	}

done:
	return (error);
}

/* IEEE80211_IOC_MACCMD */
static int
dhd_net80211_macmode_get(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int error;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	switch (ireq->i_val) {
		case IEEE80211_MACCMD_POLICY: {
			int macmode;
			error = _net80211_doiovar(dhdp, "macmode", 0, NULL, 0,
				(char *)&macmode, sizeof(macmode), IOV_GET);
			if (error != BCME_OK)
				goto done;
			switch (macmode) {
				case WLC_MACMODE_DISABLED:
					ireq->i_val = IEEE80211_MACCMD_POLICY_OPEN;
					break;
				case WLC_MACMODE_DENY:
					ireq->i_val = IEEE80211_MACCMD_POLICY_DENY;
					break;
				case WLC_MACMODE_ALLOW:
					ireq->i_val = IEEE80211_MACCMD_POLICY_ALLOW;
					break;
				default:
					error = BCME_UNSUPPORTED;
					break;
			}
			break;
		}

		case IEEE80211_MACCMD_LIST: {
			struct maclist *maclist;
			int max_count = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;
			struct ieee80211req_maclist *ap;
			int space, index;

			if (!(maclist = MALLOC(dhdp->osh, WLC_IOCTL_MAXLEN))) {
				error = BCME_NOMEM;
				break;
			}

			bzero(maclist, WLC_IOCTL_MAXLEN);
			maclist->count = max_count;

			error = _net80211_doiovar(dhdp, "mac", 0, NULL, 0,
				(char *)maclist, sizeof(maclist), IOV_GET);
			if (error != BCME_OK) {
				MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
				break;
			}

			if (maclist->count == 0) {
				MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
				error = BCME_OK;
				break;
			}

			space = maclist->count * IEEE80211_ADDR_LEN;
			if (ireq->i_len == 0) {
				ireq->i_len = space;	/* return required space */
				MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
				error = BCME_OK;        /* NB: must not error */
				break;
			}

			ap = ireq->i_data;

			/* Copy enough entries within available space */
			if (ireq->i_len < space)
				max_count = MIN(ireq->i_len / ETHER_ADDR_LEN, maclist->count);

			for (index = 0; index < max_count; index++) {
				error = copyout(&maclist->ea[index], &ap[index], ETHER_ADDR_LEN);
				if (error)
					break;
			}
			MFREE(dhdp->osh, maclist, WLC_IOCTL_MAXLEN);
			break;
		}

		default:
			error = BCME_UNSUPPORTED;
			break;

	}

done:
	return error;
}

/* IEEE80211_IOC_MACCMD */
static int
dhd_net80211_macmode_set(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int macmode;
	struct maclist maclist;
	int error;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	switch (ireq->i_val) {
		case IEEE80211_MACCMD_POLICY_ALLOW:    /* WLC_MACMODE_ALLOW */
			macmode = WLC_MACMODE_ALLOW;
			error = _net80211_doiovar(dhdp, "macmode", 0, NULL, 0,
				(char *)&macmode, sizeof(macmode), IOV_SET);
			break;

		case IEEE80211_MACCMD_POLICY_DENY:    /* WLC_MACMODE_DENY */
			macmode = WLC_MACMODE_DENY;
			error = _net80211_doiovar(dhdp, "macmode", 0, NULL, 0,
				(char *)&macmode, sizeof(macmode), IOV_SET);
			break;

		case IEEE80211_MACCMD_FLUSH:    /* Keep macmode but delete maclist */
			maclist.count = 0;
			error = _net80211_doiovar(dhdp, "mac", 0, NULL, 0,
				(char *)&maclist, sizeof(maclist), IOV_SET);
			break;

		case IEEE80211_MACCMD_POLICY_OPEN:    /* Disable macmode */
			macmode = WLC_MACMODE_DISABLED;
			error = _net80211_doiovar(dhdp, "macmode", 0, NULL, 0,
				(char *)&macmode, sizeof(macmode), IOV_SET);
			break;

		case IEEE80211_MACCMD_DETACH: /* Delete both the list and macmode */
			macmode = WLC_MACMODE_DISABLED;
			error = _net80211_doiovar(dhdp, "macmode", 0, NULL, 0,
				(char *)&macmode, sizeof(macmode), IOV_SET);
			if (error == BCME_OK) {
				maclist.count = 0;
				error = _net80211_doiovar(dhdp, "mac", 0, NULL, 0,
					(char *)&maclist, sizeof(maclist), IOV_SET);
			}
			break;

		default:
			error = BCME_UNSUPPORTED;
			break;
	}

	return error;
}
#endif /* #ifdef NOT_YET */

/* IEEE80211_IOC_WPAKEY: Since getting the key data should be protected,
 * assumption for this routine is to get key seq
 */
static int
dhd_net80211_wpakey_get(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_key ik;
	u_int kid;
	int error = BCME_OK;
	union {
		int32 index;
		uint8 tsc[EAPOL_WPA_KEY_RSC_LEN];
	} u;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if (ireq->i_len != sizeof(ik)) {
		error = BCME_BADLEN;
		goto done;
	}

	if ((error = copyin(ireq->i_data, &ik, sizeof(ik))))
		goto done;

	bzero(&u, sizeof(u));
	kid = ik.ik_keyix;

	/* We support only key seq retrieval based on index */
	if (kid == IEEE80211_KEYIX_NONE) {
		error = BCME_BADKEYIDX;
		goto done;
	}
	else {
		u.index = kid;
	}

	error = _net80211_doiovar(dhdp, "tsc", 0, NULL, 0, (char *)&u, sizeof(u), IOV_GET);
	if (error != BCME_OK)
		goto done;

	bcopy(u.tsc, &ik.ik_keytsc, EAPOL_WPA_KEY_RSC_LEN);
	error = copyout(&ik, ireq->i_data, sizeof(ik));

done:
	return (error);
}

/* IEEE80211_IOC_WPAKEY */
static int
dhd_net80211_wpakey_set(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	char iov_buf[WLC_IOCTL_SMLEN];
	struct ieee80211req_key ik;
	int error;
	wl_wsec_key_t key;
	u_int8_t *ivptr;
	u_int16_t kid;
	u_int8_t keybuf[8];

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if (ireq->i_len != sizeof(ik)) {
		error = BCME_BADLEN;
		goto done;
	}

	error = copyin(ireq->i_data, &ik, sizeof(ik));
	if (error != BCME_OK) {
		error = BCME_ERROR;
		goto done;
	}

	if (ik.ik_keylen > sizeof(ik.ik_keydata)) {
		error = BCME_BADLEN;
		goto done;
	}

	bzero(&key, sizeof(key));

	kid = ik.ik_keyix;

	/* Verify input */
	if (kid == IEEE80211_KEYIX_NONE) {
		if (ik.ik_flags != (IEEE80211_KEY_XMIT | IEEE80211_KEY_RECV)) {
			error = BCME_BADKEYIDX;
			goto done;
		}
		key.index = 0;
	} else {
		if (kid >= IEEE80211_WEP_NKID) {
			error = BCME_BADKEYIDX;
			goto done;
		}
		key.index = ik.ik_keyix;
	}

	bcopy(ik.ik_macaddr, &key.ea, ETHER_ADDR_LEN);

	key.len = ik.ik_keylen;
	if (key.len > sizeof(key.data)) {
		error = BCME_BADLEN;
		goto done;
	}

	ivptr = (u_int8_t*)&ik.ik_keyrsc;
	key.rxiv.hi = (ivptr[5] << 24) | (ivptr[4] << 16) |
	        (ivptr[3] << 8) | ivptr[2];
	key.rxiv.lo = (ivptr[1] << 8) | ivptr[0];
	key.iv_initialized = TRUE;

	bcopy(ik.ik_keydata, key.data, ik.ik_keylen);

	switch (ik.ik_type) {
		case IEEE80211_CIPHER_NONE:
			key.algo = CRYPTO_ALGO_OFF;
			break;
		case IEEE80211_CIPHER_WEP:
			if (ik.ik_keylen == WEP1_KEY_SIZE)
				key.algo = CRYPTO_ALGO_WEP1;
			else
				key.algo = CRYPTO_ALGO_WEP128;
			break;
		case IEEE80211_CIPHER_TKIP:
			key.algo = CRYPTO_ALGO_TKIP;
			/* wpa_supplicant switches the third and fourth quarters of the TKIP key */
			bcopy(&key.data[24], keybuf, sizeof(keybuf));
			bcopy(&key.data[16], &key.data[24], sizeof(keybuf));
			bcopy(keybuf, &key.data[16], sizeof(keybuf));
			break;
		case IEEE80211_CIPHER_AES_CCM:
			key.algo = CRYPTO_ALGO_AES_CCM;
			break;
		default:
			break;
	}

	if (ik.ik_flags & IEEE80211_KEY_DEFAULT)
		key.flags = WL_PRIMARY_KEY;

	/* Instead of bcast for ea address for default wep keys, driver needs it to be Null */
	if (ETHER_ISMULTI(&key.ea))
		bzero(&key.ea, ETHER_ADDR_LEN);

	key.index	= htol32(key.index);
	key.len		= htol32(key.len);
	key.algo	= htol32(key.algo);
	key.flags	= htol32(key.flags);
	key.iv_initialized = htol32(key.iv_initialized);
	key.rxiv.hi	= htol32(key.rxiv.hi);
	key.rxiv.lo	= htol16(key.rxiv.lo);

	error = _net80211_doiovar(dhdp, "wsec_key", WLC_SET_VAR, (char *)&key, sizeof(key),
	 iov_buf, sizeof(iov_buf), IOV_SET);

done:
	return (error);
}

#ifdef NOT_YET
/* IEEE80211_IOC_MCASTCIPHER, IEEE80211_IOC_MCASTKEYLEN, IEEE80211_IOC_UCASTCIPHERS,
 * IEEE80211_IOC_UCASTCIPHER, IEEE80211_IOC_UCASTKEYLEN, IEEE80211_IOC_KEYMGTALGS,
 * IEEE80211_IOC_RSNCAPS
 */
static int
dhd_net80211_rsnparam_get(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int wsec, wpa_auth, wpa_cap;
	int error;

	/* Get the common properties anyway instead of trying to optimize */
	error = _net80211_doiovar(dhdp, "wsec", &wsec, sizeof(wsec), IOV_GET);
	if (error != BCME_OK)
		goto done;

	error = _net80211_doiovar(dhdp, "wpa_auth", &wpa_auth, sizeof(wpa_auth), IOV_GET);
	if (error != BCME_OK)
		goto done;

	/* return the particular parameters */
	switch (ireq->i_type) {

		case IEEE80211_IOC_MCASTCIPHER:
			if (WSEC_WEP_ENABLED(wsec))
				ireq->i_val = IEEE80211_CIPHER_WEP;
			else if (WSEC_TKIP_ENABLED(wsec))
				ireq->i_val = IEEE80211_CIPHER_TKIP;
			else if (WSEC_AES_ENABLED(wsec))
				ireq->i_val = IEEE80211_CIPHER_AES_CCM;
			break;

		case IEEE80211_IOC_MCASTKEYLEN:
			error = BCME_UNSUPPORTED;
			break;

		case IEEE80211_IOC_UCASTCIPHERS:
			ireq->i_val = 0;
			if (WSEC_TKIP_ENABLED(wsec))
				ireq->i_val |= (1<<IEEE80211_CIPHER_TKIP);
			if (WSEC_AES_ENABLED(wsec))
				ireq->i_val |= (1<<IEEE80211_CIPHER_AES_CCM);
			break;

		case IEEE80211_IOC_KEYMGTALGS:
			ireq->i_val = WPA_ASE_NONE;
			if (AUTH_INCLUDES_UNSPEC(wpa_auth))
				ireq->i_val |= WPA_ASE_8021X_UNSPEC;
			if (AUTH_INCLUDES_PSK(wpa_auth))
				ireq->i_val |= WPA_ASE_8021X_PSK;
			break;

		case IEEE80211_IOC_RSNCAPS:
			error = _net80211_doiovar(dhdp, "wpa_cap",
				&wpa_cap, sizeof(wpa_cap), IOV_GET);
			if (error != BCME_OK)
				break;
			if ((wpa_auth & WPA2_AUTH_UNSPECIFIED) &&
			    (wpa_cap & WPA_CAP_WPA2_PREAUTH))
				ireq->i_val = RSN_CAP_PREAUTH;
			break;

		case IEEE80211_IOC_UCASTCIPHER:
		case IEEE80211_IOC_UCASTKEYLEN:
			error = BCME_UNSUPPORTED;
			break;

		default:
			error = BCME_UNSUPPORTED;
			break;
	}

done:
	return error;
}

/* IEEE80211_IOC_MCASTCIPHER, IEEE80211_IOC_MCASTKEYLEN, IEEE80211_IOC_UCASTCIPHERS,
 * IEEE80211_IOC_UCASTCIPHER, IEEE80211_IOC_UCASTKEYLEN, IEEE80211_IOC_KEYMGTALGS,
 * IEEE80211_IOC_RSNCAPS
 */
static int
dhd_net80211_rsnparam_set(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	int wsec = 0, wpa_auth = 0;
	int error;

	/* Get the properties so that read-modify-write can be done */
	error = _net80211_doiovar(dhdp, "wsec", &wsec, sizeof(wsec), IOV_GET);
	if (error != BCME_OK)
		goto done;

	error = _net80211_doiovar(dhdp, "wpa_auth", &wpa_auth, sizeof(wpa_auth), IOV_GET);
	if (error != BCME_OK)
		goto done;


	/* return the particular parameters */
	switch (ireq->i_type) {

		case IEEE80211_IOC_MCASTCIPHER:
			switch (ireq->i_val) {
				case IEEE80211_CIPHER_WEP:
					wsec |= WEP_ENABLED;
					break;
				case IEEE80211_CIPHER_TKIP:
					wsec |= TKIP_ENABLED;
					break;
				case IEEE80211_CIPHER_AES_CCM:
					wsec |= AES_ENABLED;
					break;
				default:
					error = BCME_BADOPTION;
					break;
			}
			break;

		case IEEE80211_IOC_MCASTKEYLEN:
			error = BCME_UNSUPPORTED;
			break;

		case IEEE80211_IOC_UCASTCIPHERS:
			if (ireq->i_val & (1<<IEEE80211_CIPHER_TKIP))
				wsec |= TKIP_ENABLED;
			if (ireq->i_val & (1<<IEEE80211_CIPHER_AES_CCM))
				wsec |= AES_ENABLED;
			break;

		case IEEE80211_IOC_KEYMGTALGS:
			if (ireq->i_val == WPA_ASE_NONE) {
				wpa_auth = 0;
			}
			else {
				if (ireq->i_val & WPA_ASE_8021X_UNSPEC) {
					wpa_auth |= WPA_AUTH_UNSPECIFIED;
					wpa_auth |= WPA2_AUTH_UNSPECIFIED;
				}
				if (ireq->i_val & WPA_ASE_8021X_PSK) {
					wpa_auth |= WPA_AUTH_PSK;
					wpa_auth |= WPA2_AUTH_PSK;
				}
			}
			break;

		case IEEE80211_IOC_RSNCAPS: {
			int wpa_cap;
			if (ireq->i_val & RSN_CAP_PREAUTH)
				wpa_cap = WPA_CAP_WPA2_PREAUTH;
			else
				wpa_cap = 0;
			error = _net80211_doiovar(dhdp, "wpa_cap",
				&wpa_cap, sizeof(wpa_cap), IOV_SET);
			break;
		}

		case IEEE80211_IOC_UCASTCIPHER:
		case IEEE80211_IOC_UCASTKEYLEN:
			error = BCME_UNSUPPORTED;
			break;

		default:
			error = BCME_UNSUPPORTED;
			break;
	}

	if (error != BCME_OK)
		goto done;

	/* Write back the results */
	error = _net80211_doiovar(dhdp, "wsec", &wsec, sizeof(wsec), IOV_SET);
	if (error != BCME_OK)
		goto done;

	error = _net80211_doiovar(dhdp, "wpa_auth", &wpa_auth, sizeof(wpa_auth), IOV_SET);

done:
	return error;
}
#endif /* #ifdef NOT_YET */


static int
dhd_net80211_txparams_get(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211_txparam txparam;
	int error = BCME_OK;

	memset(&txparam, 0, sizeof(txparam));

	/* Set some default values now.  Modify later on */
	txparam.ucastrate  = IEEE80211_FIXED_RATE_NONE;
	txparam.mgmtrate   = 0 | IEEE80211_RATE_MCS;
	txparam.mcastrate  = 0 | IEEE80211_RATE_MCS;
	txparam.maxretry   = IEEE80211_TXMAX_DEFAULT;

	ireq->i_len = sizeof(struct ieee80211_txparam);

	error = copyout(&txparam, ireq->i_data, ireq->i_len);
	return (error);
}


/* IEEE80211_IOC_MLME */
static int
dhd_net80211_mlme_set(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_mlme mlme;
	int error = 0;
	wl_ioctl_t ioc;
	const struct ether_addr ether_bcast = {{255, 255, 255, 255, 255, 255}};

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if (ireq->i_len != sizeof(mlme)) {
		error = BCME_BADLEN;
		goto done;
	}

	error = copyin(ireq->i_data, &mlme, sizeof(mlme));
	if (error != BCME_OK) {
		goto done;
	}

	memset(&ioc, 0, sizeof(ioc));

	switch (mlme.im_op) {
		case IEEE80211_MLME_ASSOC: {
			wl_join_params_t join_params;
			int infra;

			DHD_TRACE(("%s(): IEEE80211_MLME_ASSOC, im_ssid = %s, im_ssid_len = %d \n",
				__FUNCTION__, mlme.im_ssid, mlme.im_ssid_len));

			/* Security needs to be set before doing this by the caller */
			if (mlme.im_ssid_len != 0) {

				DHD_TRACE(("%s(): Set WLC_SET_INFRA \n", __FUNCTION__));
				ioc.cmd = WLC_SET_INFRA;
				ioc.set = WL_IOCTL_ACTION_SET;
				infra = htol32(WL_BSSTYPE_INFRA);
				error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&infra, sizeof(infra));
				if (error != BCME_OK)
					goto done;

				/* Desired ssid specified; ToDo: must match both bssid and ssid
				 * to distinguish ap advertising multiple ssid's.
				 */
				memset(&join_params, 0, sizeof(join_params));

				join_params.ssid.SSID_len = htol32(mlme.im_ssid_len);
				memcpy(join_params.ssid.SSID, mlme.im_ssid, mlme.im_ssid_len);
				memcpy(&join_params.params.bssid, &ether_bcast, IEEE80211_ADDR_LEN);

				DHD_TRACE(("%s(): Set WLC_SET_SSID, bssid = %x:%x:%x:%x:%x:%x \n",
					__FUNCTION__,
					join_params.params.bssid.octet[0],
					join_params.params.bssid.octet[1],
					join_params.params.bssid.octet[2],
					join_params.params.bssid.octet[3],
					join_params.params.bssid.octet[4],
					join_params.params.bssid.octet[5]));
				memset(&ioc, 0, sizeof(ioc));
				ioc.cmd = WLC_SET_SSID;
				ioc.set = WL_IOCTL_ACTION_SET;
				error = dhd_wl_ioctl(dhdp, 0, &ioc,
					(char *)&join_params, sizeof(join_params));
				if (error != BCME_OK)
					goto done;
			} else {
				/* Normal case; just match bssid.  ToDo: Need to get the scan list,
				 * match the BSSID, retrieve the SSID, and then do SET_SSID
				 */
				error = BCME_UNSUPPORTED;
			}
			break;
		}

		case IEEE80211_MLME_DISASSOC:
			DHD_TRACE(("%s(): IEEE80211_MLME_DISASSOC \n", __FUNCTION__));
			ioc.cmd = WLC_DISASSOC;
			ioc.set = WL_IOCTL_ACTION_SET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, NULL, 0);
			break;

		case IEEE80211_MLME_DEAUTH: {
			scb_val_t scb_val;
			DHD_TRACE(("%s(): IEEE80211_MLME_DEAUTH, im_reason = %d \n",
				__FUNCTION__, mlme.im_reason));

			scb_val.val = htol32(mlme.im_reason);
			bcopy(mlme.im_macaddr, &scb_val.ea, ETHER_ADDR_LEN);

			ioc.cmd = WLC_SCB_DEAUTHENTICATE_FOR_REASON;
			ioc.set = WL_IOCTL_ACTION_SET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&scb_val, sizeof(scb_val));
			break;
		}

		case IEEE80211_MLME_AUTHORIZE:
			DHD_TRACE(("%s(): IEEE80211_MLME_AUTHORIZE \n", __FUNCTION__));
			ioc.cmd = WLC_SCB_AUTHORIZE;
			ioc.set = WL_IOCTL_ACTION_SET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, mlme.im_macaddr, IEEE80211_ADDR_LEN);
			break;

		case IEEE80211_MLME_UNAUTHORIZE:
			DHD_TRACE(("%s(): IEEE80211_MLME_UNAUTHORIZE \n", __FUNCTION__));
			ioc.cmd = WLC_SCB_DEAUTHORIZE;
			ioc.set = WL_IOCTL_ACTION_SET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, mlme.im_macaddr, IEEE80211_ADDR_LEN);
			break;

		default:
			DHD_TRACE(("%s(): default: Unsupported \n", __FUNCTION__));
			error = BCME_UNSUPPORTED;
			break;
	}

done:
	return error;
}


/* IEEE80211_IOC_SCAN_RESULTS */
static int
dhd_net80211_scanresults_get(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	wl_scan_results_t *wl_sr = NULL;
	struct ieee80211req_scan_result *ieee_sr = NULL;
	wl_bss_info_t *bss_info = NULL;
	wl_ioctl_t ioc;
	u_int8_t *temp_ptr;
	int error, space, index;
	int wl_sr_len = 0, ieee_sr_len = 0;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	wl_sr_len = WL_SCAN_RESULTS_FIXED_SIZE +
		(sizeof(wl_bss_info_t) * IEEE80211_MAX_NUM_OF_BSSIDS);
	wl_sr = (wl_scan_results_t *)MALLOC(dhdp->osh, wl_sr_len);
	if (!wl_sr) {
		error = BCME_NOMEM;
		goto done;
	}

	memset(wl_sr, 0, wl_sr_len);
	wl_sr->buflen = htol32(wl_sr_len);

	DHD_TRACE(("%s(): Getting WLC_SCAN_RESULTS. wl_sr_len = %d \n", __FUNCTION__, wl_sr_len));
	memset(&ioc, 0, sizeof(ioc));
	ioc.cmd = WLC_SCAN_RESULTS;
	ioc.set = WL_IOCTL_ACTION_GET;
	error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)wl_sr, wl_sr_len);
	if (error != BCME_OK) {
		goto done;
	}
	DHD_TRACE(("%s(): wl_sr->version = %d, wl_sr->count = %d \n",
		__FUNCTION__, ltoh32(wl_sr->version), ltoh32(wl_sr->count)));

	/* ASSERT(wl_sr->version == WL_BSS_INFO_VERSION); */
	ieee_sr_len = sizeof(struct ieee80211req_scan_result) + IEEE80211_NWID_LEN + (256 * 2);
	ieee_sr = (struct ieee80211req_scan_result *)MALLOC(dhdp->osh, ieee_sr_len);
	if (!ieee_sr) {
		error = BCME_NOMEM;
		goto done;
	}

	temp_ptr  = ireq->i_data;
	space     = ireq->i_len;
	bss_info  = wl_sr->bss_info;

	/* Convert wl_bssinfo_t to ieee80211req_scan_result */
	for (index = 0; index < ltoh32(wl_sr->count); index++) {

		wl_bss_info_convert(bss_info);

		DHD_TRACE(("%s(): Format conversion for SSID = %s \n",
			__FUNCTION__, bss_info->SSID));

		/* Infrastructure only */
		if (!(bss_info->capability & DOT11_CAP_ESS))
			continue;

		/* Convert the structure to IEEE80211 struct */
		memset(ieee_sr, 0, ieee_sr_len);
		error = _net80211_bss2ieee_scanresults(dhdp, bss_info, ieee_sr, ieee_sr_len);
		if (error != BCME_OK)
			continue;

		/* Ran out of space for the result */
		if (space < ieee_sr->isr_len)
			break;

		/* Copy the current entry to results */
		error = copyout(ieee_sr, temp_ptr, ieee_sr->isr_len);
		if (error != BCME_OK)
		    break;

		temp_ptr  += ieee_sr->isr_len;
		space     -= ieee_sr->isr_len;
		bss_info  = (wl_bss_info_t *)((unsigned)bss_info + bss_info->length);
	}

	ireq->i_len -= space;

done:
	if (wl_sr)
		MFREE(dhdp->osh, wl_sr, wl_sr_len);
	if (ieee_sr)
		MFREE(dhdp->osh, ieee_sr, ieee_sr_len);

	DHD_TRACE(("%s(): Returns %d \n", __FUNCTION__, error));
	return error;
}


/* IEEE80211_IOC_SCAN_REQ */
static int
dhd_net80211_scanreq_set(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211_scan_req scan_req;
	int error, index;
	wl_scan_params_t scan_params;
	wl_ioctl_t ioc;
	dhd_net80211_t *net80211_ctxt;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if (ireq->i_len < sizeof(scan_req)) {
		error = BCME_NOMEM;
		goto done;
	}

	error = copyin(ireq->i_data, &scan_req, sizeof(scan_req));
	if (error != BCME_OK) {
		error = BCME_NOMEM;
		goto done;
	}

	DHD_TRACE(("%s(): sr_flags = 0x%x, sr_duration = %d, sr_nssid = %d \n", __FUNCTION__,
		scan_req.sr_flags, scan_req.sr_duration, scan_req.sr_nssid));

	memset(&scan_params, 0, sizeof(scan_params));

	for (index = 0; index < scan_req.sr_nssid; index++) {
		DHD_TRACE(("%s():     ssid[%d] = %s, len = %d \n", __FUNCTION__,
			index, scan_req.sr_ssid[index].ssid, scan_req.sr_ssid[index].len));
	}

	strncpy(scan_params.ssid.SSID, scan_req.sr_ssid[0].ssid, scan_req.sr_ssid[0].len);
	scan_params.ssid.SSID_len = scan_req.sr_ssid[0].len;

	scan_params.bssid.octet[0] = 255;
	scan_params.bssid.octet[1] = 255;
	scan_params.bssid.octet[2] = 255;
	scan_params.bssid.octet[3] = 255;
	scan_params.bssid.octet[4] = 255;
	scan_params.bssid.octet[5] = 255;

	scan_params.bss_type     = DOT11_BSSTYPE_ANY;
	scan_params.scan_type    = DOT11_SCANTYPE_ACTIVE;
	scan_params.nprobes      = htod32(1);
	scan_params.active_time  = htod32(-1);
	scan_params.passive_time = htod32(-1);
	scan_params.home_time    = htod32(10);
	scan_params.channel_num  = 0;

	memset(&ioc, 0, sizeof(ioc));
	ioc.cmd = WLC_SCAN;
	ioc.set = WL_IOCTL_ACTION_SET;
	error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&scan_params, sizeof(scan_params));

	/* Start the scan timer */
	net80211_ctxt = (dhd_net80211_t *)dhdp->net80211_ctxt;
	callout_reset(&net80211_ctxt->scan_timer,
		((IEEE80211_SCAN_TIMEOUT * hz) / 1000), _net80211_scan_timeout_cb, dhdp);
done:
	return (error);
}


/* IEEE80211_IOC_DELKEY: Given macaddr or index, delete a key */
static int
dhd_net80211_key_delete(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211req_del_key dk;
	int kid, error;
	wl_wsec_key_t key;
	wl_ioctl_t ioc;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if (ireq->i_len != sizeof(dk)) {
		error = BCME_BADLEN;
		goto done;
	}

	error = copyin(ireq->i_data, &dk, sizeof(dk));
	if (error != BCME_OK) {
		error = BCME_ERROR;
		goto done;
	}

	kid = dk.idk_keyix;

	bzero(&key, sizeof(key));

	if (dk.idk_keyix == (u_int8_t) IEEE80211_KEYIX_NONE) {
		/* Return error for now */
		error = BCME_BADKEYIDX;
		goto done;
		/* bcopy(dk.idk_macaddr, &key.ea, ETHER_ADDR_LEN); */
	}
	else {
		if (kid >= IEEE80211_WEP_NKID) {
			error = BCME_BADKEYIDX;
			goto done;
		}
		key.index = htol32(dk.idk_keyix);
	}

	memset(&ioc, 0, sizeof(ioc));
	ioc.cmd = WLC_SET_KEY;
	ioc.set = WL_IOCTL_ACTION_SET;
	error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&key, sizeof(key));
	if (error != BCME_OK) {
		DHD_ERROR(("%s(): ERROR.. WLC_SET_KEY returns %d \n", __FUNCTION__, error));
		error = BCME_OK; /* Returns OK for now */
	}

done:
	return (error);
}


/* IEEE80211_IOC_CHANINFO */
static int
dhd_net80211_chaninfo_get(dhd_pub_t *dhdp, struct ifnet *ifp,
	struct ieee80211req_chaninfo *chans, int max_channels)
{
	char channels_buf[(WL_NUMCHANNELS * sizeof(uint32)) + sizeof(wl_uint32_list_t)];
	uint32 chan_buf[32];
	wl_uint32_list_t *list;
	int channel, _channel, chan_bitmap, index;
	int error = BCME_OK;
	wl_ioctl_t ioc;

	DHD_TRACE(("%s(): Started.  no_of_channels = %d \n", __FUNCTION__, max_channels));

	memset(channels_buf, 0, sizeof(channels_buf));
	list = (wl_uint32_list_t *)channels_buf;
	list->count = htol32(WL_NUMCHANNELS);

	memset(&ioc, 0, sizeof(ioc));
	ioc.cmd = WLC_GET_VALID_CHANNELS;
	ioc.set = WL_IOCTL_ACTION_GET;
	error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)channels_buf, sizeof(channels_buf));
	if (error != BCME_OK) {
		goto done;
	}

	DHD_TRACE(("%s(): No of channels = %d \n", __FUNCTION__, ltoh32(list->count)));

	chans->ic_nchans = 0;

	for (index = 0; index < ltoh32(list->count); index++) {

		_channel = list->element[index];
		channel = ltoh32(_channel);

		/* Get per_chan_info */
		error = _net80211_doiovar(dhdp, "per_chan_info", WLC_GET_VAR, (char *)&_channel,
			sizeof(_channel), (char *)chan_buf, sizeof(chan_buf), IOV_GET);
		if (error != BCME_OK) {
			break;
		}

		chan_bitmap = ltoh32(*((uint *)chan_buf));
		/* minutes = (bitmap >> 24) & 0xff;  */

		DHD_TRACE(("%s(): channel = 0x%x, chan_bitmap = 0x%x \n",
		__FUNCTION__, channel, chan_bitmap));

		if ((chan_bitmap & WL_CHAN_VALID_HW) || (chan_bitmap & WL_CHAN_VALID_SW)) {

			chans->ic_chans[chans->ic_nchans].ic_freq = dhd_channel2freq(channel);
			if (chan_bitmap & WL_CHAN_BAND_5G)
				chans->ic_chans[chans->ic_nchans].ic_flags = IEEE80211_CHAN_5GHZ;
			else
				chans->ic_chans[chans->ic_nchans].ic_flags = IEEE80211_CHAN_2GHZ;
			if (chan_bitmap & WL_CHAN_PASSIVE) {
				chans->ic_chans[chans->ic_nchans].ic_flags |=
					IEEE80211_CHAN_PASSIVE;
			}

			chans->ic_nchans++;

			/* constrain to max_channels */
			if (chans->ic_nchans >= max_channels) {
				goto done;
			}
		}

	}

done:
	return (error);
}


/* IEEE80211_IOC_DEVCAPS:  Get driver+device capabilities */
static int
dhd_net80211_driver_cap_get(dhd_pub_t *dhdp, struct ifnet *ifp, struct ieee80211req *ireq)
{
	struct ieee80211_devcaps_req *dc;
	struct ieee80211req_chaninfo *ci;
	int maxchans;
	int error = BCME_OK;

	maxchans = 1 + ((ireq->i_len - sizeof(struct ieee80211_devcaps_req)) /
		sizeof(struct ieee80211_channel));

	DHD_TRACE(("%s(): maxchans = %d \n", __FUNCTION__, maxchans));

	/* NB: require 1 so we know ic_nchans is accessible */
	if (maxchans < 1) {
		error = BCME_NORESOURCE;
		goto done;
	}

	/* constrain max request size to WL_NUMCHANNELS */
	if (maxchans > WL_NUMCHANNELS)
		maxchans = WL_NUMCHANNELS;

	dc = (struct ieee80211_devcaps_req *)MALLOC(dhdp->osh, IEEE80211_DEVCAPS_SIZE(maxchans));
	if (dc == NULL) {
		error = BCME_NOMEM;
		goto done;
	}
	memset(dc, 0, IEEE80211_DEVCAPS_SIZE(maxchans));

	dc->dc_drivercaps |= IEEE80211_C_STA;		/* station mode */
	dc->dc_drivercaps |= IEEE80211_C_WPA;		/* capable of WPA1+WPA2 */
	dc->dc_cryptocaps |= IEEE80211_CRYPTO_WEP;
	dc->dc_cryptocaps |= IEEE80211_CRYPTO_AES_OCB;
	dc->dc_cryptocaps |= IEEE80211_CRYPTO_AES_CCM;
	/* dc->dc_cryptocaps |= IEEE80211_CRYPTO_CKIP; */
	dc->dc_cryptocaps |= IEEE80211_CRYPTO_TKIP;

#ifdef NOT_YET
	dc->dc_htcaps |= IEEE80211_HTC_HT;
#endif

	/* Retrieve the channel info */
	ci = &dc->dc_chaninfo;
	error = dhd_net80211_chaninfo_get(dhdp, ifp, ci, maxchans);
	if (error != BCME_OK) {
		MFREE(dhdp->osh, dc, IEEE80211_DEVCAPS_SIZE(maxchans));
		goto done;
	}

	ieee80211_sort_channels(ci->ic_chans, ci->ic_nchans);
	error = copyout(dc, ireq->i_data, IEEE80211_DEVCAPS_SPACE(dc));
	MFREE(dhdp->osh, dc, IEEE80211_DEVCAPS_SIZE(maxchans));

done:
	return (error);
}


/* SIOCG80211: Basic entry point for Get Net80211 ioctls */
int
dhd_net80211_ioctl_get(dhd_pub_t *dhdp, struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ieee80211req *ireq;
	char iov_buf[WLC_IOCTL_SMLEN];
	int error = BCME_OK;
	wl_ioctl_t ioc;

	ireq = (struct ieee80211req *)data;
	DHD_TRACE(("%s(): if_name = %s, cmd = 0x%lx, request type = %d, ireq->i_len = %d \n",
		__FUNCTION__, ireq->i_name, cmd, ireq->i_type, ireq->i_len));

	memset(&ioc, 0, sizeof(ioc));

	switch (ireq->i_type) {

		case IEEE80211_IOC_SSID: {
			wlc_ssid_t ssid = { 0, {0} };
			DHD_TRACE(("%s(): case IEEE80211_IOC_SSID: \n", __FUNCTION__));
			if (ireq->i_len < IEEE80211_NWID_LEN) {
				error = BCME_NOMEM;
				break;
			}
			ioc.cmd = WLC_GET_SSID;
			ioc.set = WL_IOCTL_ACTION_GET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, &ssid, sizeof(wlc_ssid_t));
			if (error != BCME_OK) {
				break;
			}

			DHD_TRACE(("%s(): ssid = %s \n", __FUNCTION__, ssid.SSID));
			ireq->i_len = strlen(ssid.SSID);
			error = copyout(ssid.SSID, ireq->i_data, strlen(ssid.SSID));
			break;
		}

		case IEEE80211_IOC_BSSID: {
			uchar bssid[IEEE80211_ADDR_LEN] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

			DHD_TRACE(("%s(): case IEEE80211_IOC_BSSID: \n", __FUNCTION__));
			if (ireq->i_len < IEEE80211_ADDR_LEN) {
				error = BCME_NOMEM;
				break;
			}
			ioc.cmd = WLC_GET_BSSID;
			ioc.set = WL_IOCTL_ACTION_GET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, bssid, sizeof(bssid));
			if (error != BCME_OK) {
				break;
			}
			DHD_TRACE(("%s(): bssid = %02x:%02x:%02x:%02x:%02x:%02x \n", __FUNCTION__,
				bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]));
			error = copyout(bssid, ireq->i_data, IEEE80211_ADDR_LEN);
			break;
		}

		case IEEE80211_IOC_NUMSSIDS:
			DHD_TRACE(("%s(): case IEEE80211_IOC_NUMSSIDS: \n", __FUNCTION__));
			ireq->i_val = 0;
			error = BCME_OK;
			break;

#ifdef NOT_YET
		case IEEE80211_IOC_STATIONNAME: {
			DHD_TRACE(("%s(): case IEEE80211_IOC_STATIONNAME: \n", __FUNCTION__));
			if (ireq->i_len < IEEE80211_NWID_LEN) {
				error = BCME_NOMEM;
				break;
			}
			error = _net80211_doiovar(dhdp, "staname", WLC_GET_VAR, NULL, 0,
				iov_buf, sizeof(iov_buf), IOV_GET);
			if (error != BCME_OK) {
				break;
			}
			DHD_TRACE(("%s(): staname = %s \n", __FUNCTION__, iov_buf));
			error = copyout(iov_buf, ireq->i_data, IEEE80211_NWID_LEN);
			break;
		}
#endif /* NOT_YET */

		case IEEE80211_IOC_REGDOMAIN: {    /* regulatory data */
			struct ieee80211_regdomain ic_regdomain;
			DHD_TRACE(("%s(): case IEEE80211_IOC_REGDOMAIN: \n", __FUNCTION__));
			if (ireq->i_len < sizeof(ic_regdomain)) {
				error = BCME_NOMEM;
				break;
			}
			memset(&ic_regdomain, 0, sizeof(ic_regdomain));
			ic_regdomain.country  = CTRY_UNITED_STATES;
			ic_regdomain.location = ' ';
			ic_regdomain.isocc[0] = 'U';
			ic_regdomain.isocc[1] = 'S';
			error = copyout(&ic_regdomain, ireq->i_data, sizeof(ic_regdomain));
			break;
		}

		case IEEE80211_IOC_CURCHAN: {
			struct channel_info chan_info;
			struct ieee80211_channel ch;
			int channel, _channel, chan_bitmap;

			DHD_TRACE(("%s(): case IEEE80211_IOC_CURCHAN: \n", __FUNCTION__));
			if (ireq->i_len < sizeof(ch)) {
				error = BCME_NOMEM;
				break;
			}
			ioc.cmd = WLC_GET_CHANNEL;
			ioc.set = WL_IOCTL_ACTION_GET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&chan_info, sizeof(chan_info));
			if (error != BCME_OK) {
				break;
			}
			DHD_TRACE(("%s(): hw_ch = %d, target_ch = %d, scan_ch = %d \n",
				__FUNCTION__, chan_info.hw_channel, chan_info.target_channel,
				chan_info.scan_channel));

			_channel = chan_info.hw_channel;
			channel = ltoh32(_channel);
			error = _net80211_doiovar(dhdp, "per_chan_info", WLC_GET_VAR,
				(char *)&_channel, sizeof(_channel),
				(char *)iov_buf, sizeof(iov_buf), IOV_GET);
			if (error != BCME_OK) {
				break;
			}

			chan_bitmap = ltoh32(*((uint *)iov_buf));
			DHD_TRACE(("%s(): channel = %d, chan_bitmap = 0x%x \n",
				__FUNCTION__, channel, chan_bitmap));

			memset(&ch, 0, sizeof(ch));
			ch.ic_freq  = dhd_channel2freq(channel);
			ch.ic_ieee  = channel;
			if (chan_bitmap & WL_CHAN_BAND_5G)
				ch.ic_flags = IEEE80211_CHAN_5GHZ;
			else
				ch.ic_flags = IEEE80211_CHAN_2GHZ;
			if (chan_bitmap & WL_CHAN_PASSIVE)
				ch.ic_flags |= IEEE80211_CHAN_PASSIVE;
			error = copyout(&ch, ireq->i_data, sizeof(ch));
			break;
		}

		case IEEE80211_IOC_DEVCAPS:
			DHD_TRACE(("%s(): case IEEE80211_IOC_DEVCAPS: \n", __FUNCTION__));
			error = dhd_net80211_driver_cap_get(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_ROAMING:  /* Roaming Mode */
			DHD_TRACE(("%s(): case IEEE80211_IOC_ROAMING: \n", __FUNCTION__));
			ireq->i_val = IEEE80211_ROAMING_MANUAL;   /* application control */
			error = BCME_OK;
			break;

		case IEEE80211_IOC_PRIVACY: {
			int wsec;
			DHD_TRACE(("%s(): case IEEE80211_IOC_PRIVACY: \n", __FUNCTION__));
			ioc.cmd = WLC_GET_WSEC;
			ioc.set = WL_IOCTL_ACTION_GET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&wsec, sizeof(wsec));
			if (error != BCME_OK) {
				break;
			}
			wsec = ltoh32(wsec);
			DHD_TRACE(("%s(): wsec = 0x%x \n", __FUNCTION__, wsec));
			ireq->i_val = WSEC_ENABLED(wsec);
			break;
		}

		case IEEE80211_IOC_WPA: {    /* Return the WPA mode (0,1,2) */
			int wpa_auth;
			DHD_TRACE(("%s(): case IEEE80211_IOC_WPA: \n", __FUNCTION__));
			ioc.cmd = WLC_GET_WPA_AUTH;
			ioc.set = WL_IOCTL_ACTION_GET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&wpa_auth, sizeof(wpa_auth));
			if (error != BCME_OK)
				break;
			wpa_auth = ltoh32(wpa_auth);
			DHD_TRACE(("%s(): wpa_auth = 0x%x \n", __FUNCTION__, wpa_auth));

			if (bcmwpa_includes_wpa_auth(wpa_auth) &&
				bcmwpa_includes_wpa2_auth(wpa_auth))
				ireq->i_val = 3;
			else if (bcmwpa_includes_wpa_auth(wpa_auth))
				ireq->i_val = 1;
			else if (bcmwpa_includes_wpa2_auth(wpa_auth))
				ireq->i_val = 2;
			else
				ireq->i_val = 0;
			DHD_TRACE(("%s(): New ireq->i_val = 0x%x \n", __FUNCTION__, ireq->i_val));
			break;
		}

		case IEEE80211_IOC_AUTHMODE: {
			int auth, wpa_auth;
			DHD_TRACE(("%s(): case IEEE80211_IOC_AUTHMODE: \n", __FUNCTION__));
			ioc.cmd = WLC_GET_AUTH;
			ioc.set = WL_IOCTL_ACTION_GET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&auth, sizeof(auth));
			if (error != BCME_OK)
				break;
			auth = ltoh32(auth);
			DHD_TRACE(("%s(): auth = 0x%x \n", __FUNCTION__, auth));

			if (auth == DOT11_OPEN_SYSTEM) {
				ireq->i_val = IEEE80211_AUTH_OPEN;
			}
			else {
				memset(&ioc, 0, sizeof(ioc));
				ioc.cmd = WLC_GET_WPA_AUTH;
				ioc.set = WL_IOCTL_ACTION_GET;
				error = dhd_wl_ioctl(dhdp, 0, &ioc,
					(char *)&wpa_auth, sizeof(wpa_auth));
				if (error != BCME_OK)
					break;
				wpa_auth = ltoh32(wpa_auth);
				DHD_TRACE(("%s(): wpa_auth = 0x%x \n", __FUNCTION__, wpa_auth));
				if (AUTH_INCLUDES_PSK(wpa_auth))
					ireq->i_val = IEEE80211_AUTH_WPA;
				else if (AUTH_INCLUDES_UNSPEC(wpa_auth))
					ireq->i_val = IEEE80211_AUTH_8021X;
			}
			break;
		}

		case IEEE80211_IOC_WEP: {
			int wsec;
			DHD_TRACE(("%s(): case IEEE80211_IOC_WEP: \n", __FUNCTION__));
			ioc.cmd = WLC_GET_WSEC;
			ioc.set = WL_IOCTL_ACTION_GET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&wsec, sizeof(wsec));
			if (error != BCME_OK)
				break;

			wsec = ltoh32(wsec);
			DHD_TRACE(("%s(): wsec = 0x%x \n", __FUNCTION__, wsec));

			if (wsec & WEP_ENABLED)
				ireq->i_val = IEEE80211_WEP_ON;
			else
				ireq->i_val = IEEE80211_WEP_OFF;
			break;
		}

		case IEEE80211_IOC_WEPTXKEY:   /* default/group tx key index */
			DHD_TRACE(("%s(): case IEEE80211_IOC_WEPTXKEY: \n", __FUNCTION__));
			ireq->i_val = 0;  /* Set 0 for now */
			break;

		case IEEE80211_IOC_NUMWEPKEYS:
			ireq->i_val = 1;  /* Set 1 for now */
			break;

		case IEEE80211_IOC_POWERSAVE:
			DHD_TRACE(("%s(): case IEEE80211_IOC_POWERSAVE: \n", __FUNCTION__));
			ireq->i_val = IEEE80211_POWERSAVE_OFF;  /* OFF for now */
			break;

		case IEEE80211_IOC_DOTD:
			DHD_TRACE(("%s(): case IEEE80211_IOC_DOTD: \n", __FUNCTION__));
			ireq->i_val = 0;  /* OFF for now */
			break;

		case IEEE80211_IOC_TXPOWER: {    /* Global tx power limit */
			tx_power_legacy_t power;
			DHD_TRACE(("%s(): case IEEE80211_IOC_TXPOWER: \n", __FUNCTION__));
			ioc.cmd = WLC_CURRENT_PWR;
			ioc.set = WL_IOCTL_ACTION_GET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&power, sizeof(power));
			if (error != BCME_OK)
				break;

			ireq->i_val = 2 * MIN(power.txpwr_band_max[0], power.txpwr_local_max);
			DHD_TRACE(("%s(): ireq->i_val = %d \n", __FUNCTION__, ireq->i_val));
			break;
		}

		case IEEE80211_IOC_SCAN_RESULTS:
			DHD_TRACE(("%s(): case IEEE80211_IOC_SCAN_RESULTS: \n", __FUNCTION__));
			error = dhd_net80211_scanresults_get(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_RTSTHRESHOLD:
			DHD_TRACE(("%s(): case IEEE80211_IOC_RTSTHRESHOLD: \n", __FUNCTION__));
#ifdef NOT_YET
			error = _net80211_doiovar(dhdp, "rtsthresh", 0, NULL, 0,
				iov_buf, sizeof(iov_buf), IOV_GET);
			if (error != BCME_OK)
				break;
			ireq->i_val = *((int *)iov_buf);
#else
			ireq->i_val = 0;
#endif
			DHD_TRACE(("%s(): rtsthresh (ireq->i_val) = 0x%x \n",
				__FUNCTION__, ireq->i_val));
			break;

		case IEEE80211_IOC_FRAGTHRESHOLD:
			DHD_TRACE(("%s(): case IEEE80211_IOC_FRAGTHRESHOLD: \n", __FUNCTION__));
#ifdef NOT_YET
			error = _net80211_doiovar(dhdp, "fragthresh", 0, NULL, 0,
				iov_buf, sizeof(iov_buf), IOV_GET);
			if (error != BCME_OK)
				break;
			ireq->i_val = *((int *)iov_buf);
#else
			ireq->i_val = 0;
#endif
			DHD_TRACE(("%s(): fragthresh = 0x%x \n", __FUNCTION__, ireq->i_val));
			break;

		case IEEE80211_IOC_BMISSTHRESHOLD:
			DHD_TRACE(("%s(): case IEEE80211_IOC_BMISSTHRESHOLD: \n", __FUNCTION__));
			ireq->i_val = 0;
			break;

		case IEEE80211_IOC_TXPARAMS:
			DHD_TRACE(("%s(): case IEEE80211_IOC_TXPARAMS: \n", __FUNCTION__));
			error = dhd_net80211_txparams_get(dhdp, ifp, ireq);
			break;

#ifdef NOT_YET
		case IEEE80211_IOC_PROTMODE:    /* Protection mode - OFF/CTS / RTSCTS */
			DHD_TRACE(("%s(): case IEEE80211_IOC_PROTMODE: \n", __FUNCTION__));
#ifdef NOT_YET
			error = _net80211_doiovar(dhdp, "phytype", &int_val,
				sizeof(int_val), IOV_GET);
			if (error != BCME_OK)
				break;

			if (int_val == PHY_TYPE_N) {
				error = _net80211_doiovar(dhdp, "nmode_protection", &int_val,
					sizeof(int_val), IOV_GET);
				if (error != BCME_OK)
					break;

				if (int_val == WLC_N_PROTECTION_20IN40)
					ireq->i_val = IEEE80211_PROT_CTSONLY;
				else if (int_val == WLC_N_PROTECTION_OFF)
					ireq->i_val = IEEE80211_PROT_NON;
				break;
			}
#endif  /* #ifdef NOT_YET */
			error = _net80211_doiovar(dhdp, "gmode_protection", 0, NULL, 0,
				(char *)&int_val, sizeof(int_val), IOV_GET);
			if (error != BCME_OK)
				break;
			/* We have only CTS-to-self or NONE */
			ireq->i_val = (int_val == WLC_PROTECTION_ON)? IEEE80211_PROT_CTSONLY:
			        IEEE80211_PROT_NONE;
			break;

		case IEEE80211_IOC_MCASTCIPHER:
		case IEEE80211_IOC_MCASTKEYLEN:
		case IEEE80211_IOC_UCASTCIPHERS:
		case IEEE80211_IOC_UCASTCIPHER:
		case IEEE80211_IOC_UCASTKEYLEN:
		case IEEE80211_IOC_KEYMGTALGS:
		case IEEE80211_IOC_RSNCAPS:
			error = dhd_net80211_rsnparam_get(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_CHANLIST:    /* WLC_GET_VALID_CHANNELS */
			DHD_TRACE(("%s(): case IEEE80211_IOC_CHANLIST: \n", __FUNCTION__));
			error = dhd_net80211_chanlist_get(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_DROPUNENCRYPTED: {
			int wep_restrict, eap_restrict;
			DHD_TRACE(("%s(): case IEEE80211_IOC_DROPUNENCRYPTED: \n", __FUNCTION__));
			error = _net80211_doiovar(dhdp, "wsec_restrict", 0, NULL, 0,
				(char *)&wep_restrict, sizeof(wep_restrict), IOV_GET);
			if (error != BCME_OK)
				break;
			error = _net80211_doiovar(dhdp, "eap_restrict", 0, NULL, 0,
				(char *)&eap_restrict, sizeof(eap_restrict), IOV_GET);
			if (error != BCME_OK)
				break;
			ireq->i_val = wep_restrict || eap_restrict;
			break;
		}

		case IEEE80211_IOC_WME:
			DHD_TRACE(("%s(): case IEEE80211_IOC_WME: \n", __FUNCTION__));
			error = _net80211_doiovar(dhdp, "wme", 0, NULL, 0, (char *)&int_val,
				sizeof(int_val), IOV_GET);
			if (error != BCME_OK)
				break;
			ireq->i_val = int_val;
			break;

		case IEEE80211_IOC_HIDESSID:
			DHD_TRACE(("%s(): case IEEE80211_IOC_HIDESSID: \n", __FUNCTION__));
			error = _net80211_doiovar(dhdp, "closednet", 0, NULL, 0, (char *)&int_val,
				sizeof(int_val), IOV_GET);
			if (error != BCME_OK)
				break;
			ireq->i_val = int_val;
			break;

		case IEEE80211_IOC_APBRIDGE:
			DHD_TRACE(("%s(): case IEEE80211_IOC_APBRIDGE: \n", __FUNCTION__));
			error = _net80211_doiovar(dhdp, "ap_isolate", 0, NULL, 0, (char *)&int_val,
				sizeof(int_val), IOV_GET);
			if (error != BCME_OK)
				break;
			ireq->i_val = int_val;
			break;
#endif  /* #ifdef NOT_YET */

		case IEEE80211_IOC_WPAKEY:
			DHD_TRACE(("%s(): case IEEE80211_IOC_WPAKEY: \n", __FUNCTION__));
			error = dhd_net80211_wpakey_get(dhdp, ifp, ireq);
			break;

#ifdef NOT_YET
		case IEEE80211_IOC_CHANINFO: {    /* WLC_GET_VALID_CHANNELS "per_chan_info" */
			struct ieee80211req_chaninfo chaninfo;
			int len;

			DHD_TRACE(("%s(): case IEEE80211_IOC_CHANINFO: \n", __FUNCTION__));
			error = dhd_net80211_chaninfo_get(dhdp, ifp, &chaninfo, IEEE80211_CHAN_MAX);
			if (error == BCME_OK) {
				len = OFFSETOF(struct ieee80211req_chaninfo,
					ic_chans[chaninfo.ic_nchans]);
				if (len > ireq->i_len)
					len = ireq->i_len;
				error = copyout(&chaninfo, ireq->i_data, len);
			}
			break;
		}

		case IEEE80211_IOC_WPAIE:    /* Per STA WPA IE */
			DHD_TRACE(("%s(): case IEEE80211_IOC_WPAIE: \n", __FUNCTION__));
			error = dhd_net80211_wpaie_get(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_STA_INFO:    /* "sta_info" */
			DHD_TRACE(("%s(): case IEEE80211_IOC_STA_INFO: \n", __FUNCTION__));
			error = dhd_net80211_stainfo_get(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_WME_CWMIN:      /* WME: CWmin */
		case IEEE80211_IOC_WME_CWMAX:      /* WME: CWmax */
		case IEEE80211_IOC_WME_AIFS:       /* WME: AIFS */
		case IEEE80211_IOC_WME_TXOPLIMIT:  /* WME: txops limit */
		case IEEE80211_IOC_WME_ACM:        /* WME: ACM (bss only) */
		case IEEE80211_IOC_WME_ACKPOLICY:  /* WME: ACK policy (bss only) */
			DHD_TRACE(("%s(): case IEEE80211_IOC_WME_CWMIN.. etc: \n", __FUNCTION__));
			error = dhd_net80211_wmeparam_get(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_DTIM_PERIOD:
			DHD_TRACE(("%s(): case IEEE80211_IOC_DTIM_PERIOD: \n", __FUNCTION__));
			error = _net80211_doiovar(dhdp, "dtim", 0, NULL, 0, (char *)&ireq->i_val,
				sizeof(ireq->i_val), IOV_GET);
			break;

		case IEEE80211_IOC_BEACON_INTERVAL:
			DHD_TRACE(("%s(): case IEEE80211_IOC_BEACON_INTERVAL: \n", __FUNCTION__));
			error = _net80211_doiovar(dhdp, "bi", 0, NULL, 0, (char *)&ireq->i_val,
				sizeof(ireq->i_val), IOV_GET);
			break;

		case IEEE80211_IOC_PUREG: /* WLC_GET_GMODE - GMODE only */
			DHD_TRACE(("%s(): case IEEE80211_IOC_PUREG: \n", __FUNCTION__));
			error = _net80211_doiovar(dhdp, "gmode", 0, NULL, 0, (char *)&ireq->i_val,
				sizeof(ireq->i_val), IOV_GET);
			if (error != BCME_OK)
				break;
			ireq->i_val = (ireq->i_val & GMODE_ONLY) != 0;
			break;

		case IEEE80211_IOC_MCAST_RATE: {
			int band;
			const char *aname = "a_mrate";
			const char *bgname = "nbg_mrate";
			const char *name = NULL;
			int list[3];

			/* Get the band WLC_GET_BAND */
			error = _net80211_doiovar(dhdp, "band", &band, sizeof(uint), IOV_GET);
			if (error != BCME_OK)
				break;

			error = _net80211_doiovar(dhdp, "bands", &list, sizeof(list), IOV_GET);
			if (error != BCME_OK)
				break;

			if (!list[0]) {
				error = BCME_BADLEN;
				break;
			}
			else if (list[0] > 2) {
				list[0] = 2;
			}

			switch (band) {
				case WLC_BAND_AUTO :
					if (list[0] > 1) {
						error = BCME_BADLEN;
						break;
					}
					else if (list[1] == WLC_BAND_5G)
						name = aname;
					else if (list[1] == WLC_BAND_2G)
						name = bgname;
					else {
						error = BCME_BADBAND;
						break;
					}
					break;

				case WLC_BAND_5G:
					name = aname;
					break;

				case WLC_BAND_2G:
					name = bgname;
					break;

				default:
					error = BCME_BADBAND;
					break;
			}

			if (error != BCME_OK)
				break;

			error = _net80211_doiovar(dhdp, name, &int_val, sizeof(int_val), IOV_GET);
			if (error != BCME_OK)
				break;

			ireq->i_val = int_val;
			break;
		}

		case IEEE80211_IOC_MACCMD: /* MAC Mode */
			DHD_TRACE(("%s(): case IEEE80211_IOC_MACCMD: \n", __FUNCTION__));
			error = dhd_net80211_macmode_get(dhdp, ifp, ireq);
			break;
#endif /* #ifdef NOT_YET */

		default:
			DHD_TRACE(("%s(): case default: Not Supported \n", __FUNCTION__));
			error = BCME_UNSUPPORTED;
			break;
	}

	DHD_TRACE(("%s(): Returns = %d \n", __FUNCTION__, error));
	return error;
}

/* Is any of the tlvs the expected entry? If
 * not update the tlvs buffer pointer/length.
 */
static bool
wl_has_ie(u_int8_t *ie, u_int8_t **tlvs, u_int32_t *tlvs_len, const u_int8_t *oui,
	u_int32_t oui_len, u_int8_t type)
{
	/* If the contents match the OUI and the type */
	if (ie[TLV_LEN_OFF] >= oui_len + 1 &&
		!bcmp(&ie[TLV_BODY_OFF], oui, oui_len) &&
		type == ie[TLV_BODY_OFF + oui_len]) {
		return TRUE;
	}

	if (tlvs == NULL)
		return FALSE;
	/* point to the next ie */
	ie += ie[TLV_LEN_OFF] + TLV_HDR_LEN;
	/* calculate the length of the rest of the buffer */
	*tlvs_len -= (int)(ie - *tlvs);
	/* update the pointer to the start of the buffer */
	*tlvs = ie;

	return FALSE;
}

/* Check whether pointed-to IE looks like WPA. */
#define wl_is_wpa_ie(ie, tlvs, len)	wl_has_ie(ie, tlvs, len, \
		(const uint8 *)WPS_OUI, WPS_OUI_LEN, WPA_OUI_TYPE)
/* Check whether pointed-to IE looks like WPS. */
#define wl_is_wps_ie(ie, tlvs, len)	wl_has_ie(ie, tlvs, len, \
		(const uint8 *)WPS_OUI, WPS_OUI_LEN, WPS_OUI_TYPE)

bcm_tlv_t *
wl_find_wpaie(u_int8_t *parse, u_int32_t len)
{
	bcm_tlv_t *ie;

	while ((ie = bcm_parse_tlvs(parse, (u_int32_t)len, DOT11_MNG_VS_ID))) {
		if (wl_is_wpa_ie((u_int8_t*)ie, &parse, &len)) {
			return (bcm_tlv_t *)ie;
		}
	}
	return NULL;
}

u_int8_t gwps_ie[WLC_IOCTL_SMLEN];

static int
dhd_net80211_management_ie_set(dhd_pub_t *dhdp, struct ifnet *ifp, uint8 *ie, uint16_t ie_len,
	const char *command)
{
	int error = BCME_UNSUPPORTED;
	char iov_buf[WLC_IOCTL_SMLEN];
	vndr_ie_setbuf_t *ie_setbuf;
	uint32 pktflag;
	int ielen, datalen, buflen, iecount;

	ielen = ie[1];
	datalen = ielen - VNDR_IE_MIN_LEN;
	buflen = sizeof(vndr_ie_setbuf_t) + datalen - 1;

	if (!(ie_setbuf = (vndr_ie_setbuf_t *) MALLOC(dhdp->osh, buflen))) {
		error = BCME_NOMEM;
		goto done;
	}
	/* Copy the vndr_ie SET command ("add"/"del") to the buffer */
	strncpy(ie_setbuf->cmd, command, VNDR_IE_CMD_LEN - 1);
	ie_setbuf->cmd[VNDR_IE_CMD_LEN - 1] = '\0';

	/* Buffer contains only 1 IE */
	iecount = htod32(1);
	memcpy((void *)&ie_setbuf->vndr_ie_buffer.iecount, &iecount, sizeof(int));

	/*
	* The packet flag bit field indicates the packets that will
	* contain this IE
	*/
	pktflag = htod32(VNDR_IE_ASSOCREQ_FLAG);
	memcpy((void *)&ie_setbuf->vndr_ie_buffer.vndr_ie_list[0].pktflag, &pktflag,
		sizeof(uint32));

	/* Now, add the IE to the buffer */
	ie_setbuf->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.id = (uchar) DOT11_MNG_PROPR_ID;
	ie_setbuf->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.len = (uchar) ielen;

	memcpy(&ie_setbuf->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data, ie, ie_len);

	error = _net80211_doiovar(dhdp, "vndr_ie", WLC_SET_VAR, (u_int8_t *)ie_setbuf, buflen,
	 iov_buf, sizeof(iov_buf), IOV_SET);

	memcpy(gwps_ie, ie, ie_len);
	if (ie_setbuf)
		MFREE(dhdp->osh, ie_setbuf, buflen);

done:
	return (error);
}

/* SIOCS80211: Basic entry point for Set Net80211 ioctls */
int
dhd_net80211_ioctl_set(dhd_pub_t *dhdp, struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ieee80211req *ireq;
	int error = BCME_UNSUPPORTED;
	wl_ioctl_t ioc;

	ireq = (struct ieee80211req *)data;
	DHD_TRACE(("%s(): if_name = %s, cmd = 0x%lx \n", __FUNCTION__, ireq->i_name, cmd));
	DHD_TRACE(("%s(): request type = %d, ireq->i_len = %d, ireq->i_val = %d \n",
		__FUNCTION__, ireq->i_type, ireq->i_len, ireq->i_val));

	memset(&ioc, 0, sizeof(ioc));

	switch (ireq->i_type) {

		case IEEE80211_IOC_ROAMING:    /* Setting Roaming mode */
			DHD_TRACE(("%s(): case IEEE80211_IOC_ROAMING: ireq->i_val = %d \n",
				__FUNCTION__, ireq->i_val));
			error = BCME_OK;
			break;

		case IEEE80211_IOC_WPA: {    /* Set WPA Version (0, 1, 2) */
			int wpa_auth = 0;
			DHD_TRACE(("%s(): case IEEE80211_IOC_WPA: ireq->i_val = %d \n",
				__FUNCTION__, ireq->i_val));

			if ((ireq->i_val & 1))
				wpa_auth |= WPA_AUTH_PSK;
			else
				wpa_auth &= ~(WPA_AUTH_PSK | WPA_AUTH_UNSPECIFIED);

			if ((ireq->i_val & 2))
				wpa_auth |= WPA2_AUTH_PSK;
			else
				wpa_auth &= ~(WPA2_AUTH_PSK | WPA2_AUTH_UNSPECIFIED);

			DHD_TRACE(("%s(): Modified wpa_auth = 0x%x \n", __FUNCTION__, wpa_auth));
			wpa_auth = htol32(wpa_auth);
			memset(&ioc, 0, sizeof(ioc));
			ioc.cmd = WLC_SET_WPA_AUTH;
			ioc.set = WL_IOCTL_ACTION_SET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&wpa_auth, sizeof(wpa_auth));
			break;
		}

		case IEEE80211_IOC_DELKEY:
			DHD_TRACE(("%s(): case IEEE80211_IOC_DELKEY: \n", __FUNCTION__));
			error = dhd_net80211_key_delete(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_COUNTERMEASURES: /* Do nothing now.  Check if driver does this */
			DHD_TRACE(("%s(): case IEEE80211_IOC_COUNTERMEASURES: \n", __FUNCTION__));
			error = BCME_OK;
			break;

		case IEEE80211_IOC_PRIVACY: {
			int wsec = 0;
			DHD_TRACE(("%s(): case IEEE80211_IOC_PRIVACY: ireq->i_val = %d \n",
				__FUNCTION__, ireq->i_val));

			switch (ireq->i_val) {
				case IEEE80211_CIPHER_WEP:
					wsec = WEP_ENABLED;
					break;
				case IEEE80211_CIPHER_TKIP:
					wsec = TKIP_ENABLED;
					break;
				case IEEE80211_CIPHER_AES_CCM:
					wsec = AES_ENABLED;
					break;
				default:
					break;
			}

			DHD_TRACE(("%s(): New wsec = 0x%x \n", __FUNCTION__, wsec));
			wsec = htol32(wsec);
			memset(&ioc, 0, sizeof(ioc));
			ioc.cmd = WLC_SET_WSEC;
			ioc.set = WL_IOCTL_ACTION_SET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&wsec, sizeof(wsec));
			break;
		}

		case IEEE80211_IOC_SCAN_REQ: {
			DHD_TRACE(("%s(): case IEEE80211_IOC_SCAN_REQ: \n", __FUNCTION__));
			error = dhd_net80211_scanreq_set(dhdp, ifp, ireq);
			break;
		}

		case IEEE80211_IOC_DROPUNENCRYPTED: {
			DHD_TRACE(("%s(): case IEEE80211_IOC_DROPUNENCRYPTED: \n", __FUNCTION__));
			int wep_rest;
			wep_rest = htol32(ireq->i_val);
			ioc.cmd = WLC_SET_WEP_RESTRICT;
			ioc.set = WL_IOCTL_ACTION_SET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&wep_rest, sizeof(wep_rest));
			if (error != BCME_OK)
				break;
			error = BCME_OK;
			break;
		}

		case IEEE80211_IOC_AUTHMODE: {
			int cur_auth, new_auth;
			int cur_wpa_auth, new_wpa_auth;
			DHD_TRACE(("%s(): case IEEE80211_IOC_AUTHMODE: \n", __FUNCTION__));
			ioc.cmd = WLC_GET_AUTH;
			ioc.set = WL_IOCTL_ACTION_GET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc, (char *)&cur_auth, sizeof(cur_auth));
			if (error != BCME_OK)
				break;
			cur_auth = ltoh32(cur_auth);
			new_auth = cur_auth;

			memset(&ioc, 0, sizeof(ioc));
			ioc.cmd = WLC_GET_WPA_AUTH;
			ioc.set = WL_IOCTL_ACTION_GET;
			error = dhd_wl_ioctl(dhdp, 0, &ioc,
				(char *)&cur_wpa_auth, sizeof(cur_wpa_auth));
			if (error != BCME_OK)
				break;
			cur_wpa_auth = ltoh32(cur_wpa_auth);
			new_wpa_auth = cur_wpa_auth;

			DHD_TRACE(("%s(): cur_auth = %d, cur_wpa_auth = %d \n",
				__FUNCTION__, cur_auth, cur_wpa_auth));

			switch (ireq->i_val) {
				case IEEE80211_AUTH_NONE:
					new_wpa_auth = WPA_AUTH_NONE;
					break;
				case IEEE80211_AUTH_OPEN:
					new_auth = DOT11_OPEN_SYSTEM;
					break;
				case IEEE80211_AUTH_SHARED:
					new_auth = DOT11_SHARED_KEY;
					break;
				case IEEE80211_AUTH_8021X:
					new_wpa_auth = 0;
					new_wpa_auth |= (WPA_AUTH_UNSPECIFIED |
						WPA2_AUTH_UNSPECIFIED);
					break;
				case IEEE80211_AUTH_AUTO:
					break;
				case IEEE80211_AUTH_WPA:
					new_wpa_auth = 0;
					new_wpa_auth |= (WPA_AUTH_PSK | WPA2_AUTH_PSK);
					break;
				default:
					error = BCME_BADARG;
					break;
			}
			if (error != BCME_OK)
				break;

			DHD_TRACE(("%s(): new_auth = %d, new_wpa_auth = %d \n",
				__FUNCTION__, new_auth, new_wpa_auth));

			if (cur_auth != new_auth) {
				DHD_TRACE(("%s(): Setting new_auth = %d \n",
					__FUNCTION__, new_auth));
				new_auth = htol32(new_auth);
				memset(&ioc, 0, sizeof(ioc));
				ioc.cmd = WLC_SET_AUTH;
				ioc.set = WL_IOCTL_ACTION_SET;
				error = dhd_wl_ioctl(dhdp, 0, &ioc,
					(char *)&new_auth, sizeof(new_auth));
				if (error != BCME_OK)
					break;
			}
			if (cur_wpa_auth != new_wpa_auth) {
				DHD_TRACE(("%s(): Setting new_wpa_auth = %d \n",
					__FUNCTION__, new_wpa_auth));
				new_wpa_auth = htol32(new_wpa_auth);
				memset(&ioc, 0, sizeof(ioc));
				ioc.cmd = WLC_SET_WPA_AUTH;
				ioc.set = WL_IOCTL_ACTION_SET;
				error = dhd_wl_ioctl(dhdp, 0, &ioc,
					(char *)&new_wpa_auth, sizeof(new_wpa_auth));
				if (error != BCME_OK)
					break;
			}
			break;
		}

#ifdef IEEE80211_IOC_APPIE
		case IEEE80211_IOC_APPIE: {
			DHD_TRACE(("%s(): case IEEE80211_IOC_APPIE: \n", __FUNCTION__));
#else
		case IEEE80211_IOC_OPTIE: {
			DHD_TRACE(("%s(): case IEEE80211_IOC_OPTIE: \n", __FUNCTION__));
#endif /* IEEE80211_IOC_APPIE */
			uint8 *ie = NULL;
			char iov_buf[WLC_IOCTL_SMLEN];
			bcm_tlv_t *wpa_ie = NULL, *wpa2_ie = NULL;

			if (ireq->i_len > IEEE80211_MAX_OPT_IE) {
				error = BCME_BADARG;
				break;
			}
			if (ireq->i_len) {
				if (!(ie = MALLOC(dhdp->osh, ireq->i_len))) {
					error = BCME_NOMEM;
					break;
				}
				error = copyin(ireq->i_data, ie, ireq->i_len);
				if (error) {
					MFREE(dhdp->osh, ie, ireq->i_len);
					error = BCME_ERROR;
					break;
				}
			}
			else {
				DHD_TRACE(("%s(): Just want to cleanup the wpaie.\n",
				 __FUNCTION__));
				error = _net80211_doiovar(dhdp, "wpaie", WLC_SET_VAR, ie,
					ireq->i_len, iov_buf, sizeof(iov_buf), IOV_SET);
				if (ie)
					MFREE(dhdp->osh, ie, ireq->i_len);
				break;
			}
			/* find the RSN_IE */
			if ((wpa2_ie = bcm_parse_tlvs((u_int8_t *)ie, ireq->i_len,
				DOT11_MNG_RSN_ID)) != NULL) {
				DHD_TRACE(("%s(): WPA2 IE is found\n", __FUNCTION__));
			}
			/* find the WPA_IE */
			if ((wpa_ie = wl_find_wpaie((u_int8_t *)ie, ireq->i_len)) != NULL) {
				DHD_TRACE(("%s(): WPA IE is found\n", __FUNCTION__));
			}
			if (wpa_ie != NULL || wpa2_ie != NULL) {
				error = _net80211_doiovar(dhdp, "wpaie", WLC_SET_VAR, ie,
				 ireq->i_len, iov_buf, sizeof(iov_buf), IOV_SET);

				if (gwps_ie[0] != 0x0) {
					DHD_TRACE(("%s(): Just want to cleanup the wpsie.\n",
					 __FUNCTION__));
					error = dhd_net80211_management_ie_set(dhdp, ifp, gwps_ie,
					 26, "del");
					memset(&gwps_ie, 0, sizeof(gwps_ie));
				}
			} else {
				DHD_TRACE(("%s(): vndr_ie\n", __FUNCTION__));
				error = dhd_net80211_management_ie_set(dhdp, ifp, ie,
				 ireq->i_len, "add");
			}
			if (ie)
				MFREE(dhdp->osh, ie, ireq->i_len);
			break;
		}

		case IEEE80211_IOC_MLME:
			DHD_TRACE(("%s(): case IEEE80211_IOC_MLME: \n", __FUNCTION__));
			error = dhd_net80211_mlme_set(dhdp, ifp, ireq);
			break;

#ifdef NOT_YET
		case IEEE80211_IOC_WEP: {
			int wsec;

			error = _net80211_doiovar(dhdp, "wsec", 0, NULL, 0,
				(char *)&wsec, sizeof(wsec), IOV_GET);
			if (error != BCME_OK)
				break;

			switch (ireq->i_val) {
				case IEEE80211_WEP_OFF:
					wsec &= ~WEP_ENABLED;
					int_val = 0;
					break;
				case IEEE80211_WEP_ON:
					wsec |= WEP_ENABLED;
					int_val = 1;
					break;
				case IEEE80211_WEP_MIXED:
					wsec |= WEP_ENABLED;
					int_val = 0;
					break;
			}
			error = _net80211_doiovar(dhdp, "wsec", 0, NULL, 0,
				(char *)&wsec, sizeof(wsec), IOV_SET);
			if (error != BCME_OK)
				break;
			error = _net80211_doiovar(dhdp, "wsec_restrict", 0, NULL, 0,
				(char *)&int_val, sizeof(int_val), IOV_SET);
			break;
		}

		case IEEE80211_IOC_RTSTHRESHOLD:
			error = _net80211_doiovar(dhdp, "rtsthresh", 0, NULL, 0,
				(char *)&ireq->i_val, sizeof(ireq->i_val), IOV_SET);
			if (error != BCME_OK)
				break;

		case IEEE80211_IOC_PROTMODE:
			/* Driver does not support RTSCTS option */
			if (ireq->i_val > IEEE80211_PROT_CTSONLY) {
				error = BCME_BADARG;
				break;
			}
			int_val =  (ireq->i_val == IEEE80211_PROT_CTSONLY)? WLC_PROTECTION_AUTO:
			        WLC_PROTECTION_OFF;
			error = _net80211_doiovar(dhdp, "gmode_protection_override", 0, NULL, 0,
				(char *)&int_val, sizeof(int_val), IOV_SET);
			break;

		case IEEE80211_IOC_TXPOWER:
			int_val = ireq->i_val / 2; /* Input is in .5 dbM */
			error = _net80211_doiovar(dhdp, "qtxpower", 0, NULL, 0,
				(char *)&int_val, sizeof(int_val), IOV_SET);
			break;
#endif  /* #ifdef NOT_YET */

		case IEEE80211_IOC_WPAKEY:
			error = dhd_net80211_wpakey_set(dhdp, ifp, ireq);
			break;

#ifdef NOT_YET
		case IEEE80211_IOC_WME:
			error = _net80211_doiovar(dhdp, "wme", 0, NULL, 0,
				(char *)&ireq->i_val, sizeof(ireq->i_val), IOV_SET);
			break;

		case IEEE80211_IOC_HIDESSID:
			error = _net80211_doiovar(dhdp, "closednet", 0, NULL, 0,
				(char *)&ireq->i_val, sizeof(ireq->i_val), IOV_SET);
			break;

		case IEEE80211_IOC_APBRIDGE:
			error = _net80211_doiovar(dhdp, "ap_isolate", 0, NULL, 0,
				(char *)&ireq->i_val, sizeof(ireq->i_val), IOV_SET);
			break;

		case IEEE80211_IOC_MCASTCIPHER:
		case IEEE80211_IOC_MCASTKEYLEN:
		case IEEE80211_IOC_UCASTCIPHERS:
		case IEEE80211_IOC_UCASTCIPHER:
		case IEEE80211_IOC_UCASTKEYLEN:
		case IEEE80211_IOC_KEYMGTALGS:
		case IEEE80211_IOC_RSNCAPS:
			/* The method of setting the parameters depends on mode as
			 * AP stores this information in wpa_auth/wsec settings
			 * For STA, this information is to be decoded from the BSS IE
			 */
			error = dhd_net80211_rsnparam_set(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_ADDMAC:
		case IEEE80211_IOC_DELMAC:
			error = dhd_net80211_mac_update(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_MACCMD:
			error = dhd_net80211_macmode_set(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_WME_CWMIN:		/* WME: CWmin */
		case IEEE80211_IOC_WME_CWMAX:		/* WME: CWmax */
		case IEEE80211_IOC_WME_AIFS:		/* WME: AIFS */
		case IEEE80211_IOC_WME_TXOPLIMIT:	/* WME: txops limit */
		case IEEE80211_IOC_WME_ACM:			/* WME: ACM (bss only) */
		case IEEE80211_IOC_WME_ACKPOLICY:	/* WME: ACK policy (bss only) */
			error = dhd_net80211_wmeparam_set(dhdp, ifp, ireq);
			break;

		case IEEE80211_IOC_DTIM_PERIOD:
			error = _net80211_doiovar(dhdp, "dtim", 0, NULL, 0,
				(char *)&ireq->i_val, sizeof(ireq->i_val), IOV_SET);
			break;

		case IEEE80211_IOC_BEACON_INTERVAL:
			error = _net80211_doiovar(dhdp, "bi", 0, NULL, 0,
				(char *)&ireq->i_val, sizeof(ireq->i_val), IOV_SET);
			break;

		case IEEE80211_IOC_PUREG:
			if (ireq->i_val)
				ireq->i_val = GMODE_ONLY;
			else
				ireq->i_val = GMODE_AUTO;
			error = _net80211_doiovar(dhdp, "gmode", 0, NULL, 0,
				(char *)&ireq->i_val, sizeof(ireq->i_val), IOV_SET);
			break;

		case IEEE80211_IOC_MCAST_RATE: {
			const char *aname = "a_mrate";
			const char *bgname = "bg_mrate";
			const char *name = NULL;
			int list[3];
			int band;

			/* Get the band WLC_GET_BAND */
			error = _net80211_doiovar(dhdp, "band", &band, sizeof(uint), IOV_GET);
			if (error != BCME_OK)
				break;

			error = _net80211_doiovar(dhdp, "bands", &list, sizeof(list), IOV_GET);
			if (error != BCME_OK)
				break;

			if (!list[0]) {
				error = BCME_BADLEN;
				break;
			}
			else if (list[0] > 2)
				list[0] = 2;

			switch (band) {
				case WLC_BAND_AUTO :
					if (list[0] > 1) {
						error = BCME_BADLEN;
						break;
					}
					else if (list[1] == WLC_BAND_5G)
						name = aname;
					else if (list[1] == WLC_BAND_2G)
						name = bgname;
					else {
						error = BCME_BADBAND;
						break;
					}
					break;

				case WLC_BAND_5G:
					name = aname;
					break;

				case WLC_BAND_2G:
					name = bgname;
					break;

				default:
					error = BCME_BADBAND;
					break;
			}

			if (error != BCME_OK)
				break;

			error = _net80211_doiovar(dhdp, name, &ireq->i_val,
				sizeof(ireq->i_val), IOV_SET);
			break;
		}

		case IEEE80211_IOC_FRAGTHRESHOLD:
			error = _net80211_doiovar(dhdp, "fragthresh", 0, NULL, 0,
				(char *)&ireq->i_val, sizeof(ireq->i_val), IOV_SET);
			break;
#endif  /* #ifdef NOT_YET */

		default:
			DHD_TRACE(("%s(): case default: Not Supported \n", __FUNCTION__));
			error = BCME_UNSUPPORTED;
			break;
	}

	DHD_TRACE(("%s(): Returns %d \n", __FUNCTION__, error));
	return error;
}


/* SIOCG80211: Basic entry point for Get Net80211 ioctls */
int
dhd_net80211_ioctl_stats(dhd_pub_t *dhdp, struct ifnet *ifp, u_long cmd, caddr_t data)
{
#if defined(DHD_DEBUG)
	struct ieee80211req *ireq = (struct ieee80211req *)data;
#endif
	int error = BCME_UNSUPPORTED;

#if defined(DHD_DEBUG)
	DHD_TRACE(("%s(): if_name = %s, cmd = 0x%lx, request type = %d\n",
		__FUNCTION__, ireq->i_name, cmd, ireq->i_type));
#endif

	DHD_TRACE(("%s(): Not Supported yet \n", __FUNCTION__));
	return error;
}

/* SIOCG80211: Basic entry point for Get Net80211 ioctls */
int
dhd_net80211_event_proc(dhd_pub_t *dhdp, struct ifnet *ifp, wl_event_msg_t *event, int ap_mode)
{
	int error = BCME_OK;
	uint32 type, status, reason;
	uint16 flags;

	type   = ntoh32_ua((void *)&event->event_type);
	flags  = ntoh16_ua((void *)&event->flags);
	status = ntoh32_ua((void *)&event->status);
	reason = ntoh32_ua((void *)&event->reason);

	DHD_TRACE(("%s(): Started... type: %d, flags = %d, status = %d, reason = %d \n",
		__FUNCTION__, type, flags, status, reason));

	switch (type) {
		case WLC_E_SCAN_COMPLETE: {
			dhd_net80211_t *net80211_ctxt = (dhd_net80211_t *)dhdp->net80211_ctxt;
			DHD_TRACE(("%s(): evnet --> WLC_E_SCAN_COMPLETE \n", __FUNCTION__));
			if (callout_pending(&net80211_ctxt->scan_timer) ||
				callout_active(&net80211_ctxt->scan_timer)) {
				DHD_TRACE(("%s(): Stop Callback timer \n", __FUNCTION__));
				callout_stop(&net80211_ctxt->scan_timer);
			}
			rt_ieee80211msg(ifp, RTM_IEEE80211_SCAN, NULL, 0);
			break;
		}

		case WLC_E_JOIN:
		case WLC_E_REASSOC_IND:
		case WLC_E_ASSOC_IND: {
			struct ieee80211_join_event join_evt;
			int rtm = 0;
			bzero(&join_evt, sizeof(join_evt));
			bcopy(&event->addr, join_evt.iev_addr, ETHER_ADDR_LEN);

			/* RTMs are different based on context */
			if (type == WLC_E_JOIN) {
				DHD_TRACE(("%s(): evnet --> WLC_E_JOIN \n", __FUNCTION__));
				ASSERT(!ap_mode);
				rtm = RTM_IEEE80211_ASSOC;
			}
			else if (type == WLC_E_REASSOC_IND) {
				DHD_TRACE(("%s(): evnet --> WLC_E_REASSOC_IND \n", __FUNCTION__));
				rtm = (ap_mode)? RTM_IEEE80211_REJOIN: RTM_IEEE80211_REASSOC;
			}
			else if (type == WLC_E_ASSOC_IND) {
				DHD_TRACE(("%s(): evnet --> WLC_E_ASSOC_IND \n", __FUNCTION__));
				rtm = RTM_IEEE80211_JOIN;
			}
			DHD_TRACE(("%s(): iev_addr = %x:%x:%x:%x:%x:%x \n", __FUNCTION__,
				join_evt.iev_addr[0], join_evt.iev_addr[1], join_evt.iev_addr[2],
				join_evt.iev_addr[3], join_evt.iev_addr[4], join_evt.iev_addr[5]));
			rt_ieee80211msg(ifp, rtm, &join_evt, sizeof(join_evt));
			break;
		}

		case WLC_E_DISASSOC:
		case WLC_E_DISASSOC_IND:{
			struct ieee80211_leave_event leave_evt;

			bzero(&leave_evt, sizeof(leave_evt));
			bcopy(&event->addr, leave_evt.iev_addr, ETHER_ADDR_LEN);
			if (ap_mode)
				DHD_TRACE(("%s(): evnet --> RTM_IEEE80211_LEAVE \n", __FUNCTION__));
			else
				DHD_TRACE(("%s(): evnet --> RTM_IEEE80211_DISASSOC \n",
				 __FUNCTION__));
			rt_ieee80211msg(ifp, ap_mode? RTM_IEEE80211_LEAVE:RTM_IEEE80211_DISASSOC,
				&leave_evt, sizeof(leave_evt));
			break;
		}

		case WLC_E_MIC_ERROR: {
			struct ieee80211_michael_event mic_evt;
			bzero(&mic_evt, sizeof(mic_evt));
			bcopy(&event->addr, mic_evt.iev_src, ETHER_ADDR_LEN);
			mic_evt.iev_cipher = IEEE80211_CIPHER_TKIP;
			rt_ieee80211msg(ifp, RTM_IEEE80211_MICHAEL, &mic_evt, sizeof(mic_evt));
			break;
		}

		default:
			DHD_TRACE(("%s(): default: Not processed \n", __FUNCTION__));
			error = BCME_UNSUPPORTED;
			break;
	}

	DHD_TRACE(("%s(): Returns %d \n", __FUNCTION__, error));
	return error;
}


int
dhd_net80211_detach(dhd_pub_t *dhdp)
{
	dhd_net80211_t *net80211_ctxt;
	int ret = BCME_OK;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if (dhdp->net80211_ctxt == NULL) {
		DHD_ERROR(("%s(): ERROR.. Bad Argument \n", __FUNCTION__));
		ret = BCME_BADARG;
		goto done;
	}

	net80211_ctxt = (dhd_net80211_t *)dhdp->net80211_ctxt;

	callout_stop(&net80211_ctxt->scan_timer);

	MFREE(dhdp->osh, net80211_ctxt, sizeof(dhd_net80211_t));
	dhdp->net80211_ctxt = NULL;

done:
	DHD_TRACE(("%s(): Returns %d \n", __FUNCTION__, ret));
	return (ret);
}


int
dhd_net80211_attach(dhd_pub_t *dhdp, struct ifnet *ifp)
{
	dhd_net80211_t *net80211_ctxt;
	int ret = BCME_OK;

	DHD_TRACE(("%s(): Started \n", __FUNCTION__));

	if ((dhdp->net80211_ctxt != NULL) || (ifp == NULL)) {
		DHD_ERROR(("%s(): ERROR.. Bad Argument \n", __FUNCTION__));
		ret = BCME_BADARG;
		goto done;
	}

	net80211_ctxt = (dhd_net80211_t *)MALLOC(dhdp->osh, sizeof(dhd_net80211_t));
	if (!net80211_ctxt) {
		DHD_ERROR(("%s(): ERROR.. MALLOC() failed \n", __FUNCTION__));
		ret = BCME_NOMEM;
		goto done;
	}
	dhdp->net80211_ctxt = net80211_ctxt;

	memset(net80211_ctxt, 0, sizeof(dhd_net80211_t));

	net80211_ctxt->ifp  = ifp;
	callout_init(&net80211_ctxt->scan_timer, CALLOUT_MPSAFE);

done:
	DHD_TRACE(("%s(): Returns %d \n", __FUNCTION__, ret));
	return (ret);
}
