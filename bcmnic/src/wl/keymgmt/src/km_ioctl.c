/*
 * Key Management Module Implementation - ioctl support
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
 * $Id: km_ioctl.c 619609 2016-02-17 13:46:00Z $
 */

#include "km_pvt.h"
#include <wlc_iocv.h>

/* internal interface */

#define KM_IOCTL_IOCF_FLAGS (WLC_IOCF_OPEN_ALLOW)

static const wlc_ioctl_cmd_t km_ioctls[] = {
	{WLC_GET_KEY, KM_IOCTL_IOCF_FLAGS, sizeof(int)},
	{WLC_SET_KEY, KM_IOCTL_IOCF_FLAGS, sizeof(int)},
	{WLC_GET_KEY_SEQ, KM_IOCTL_IOCF_FLAGS, sizeof(int)},
	{WLC_GET_KEY_PRIMARY, KM_IOCTL_IOCF_FLAGS, sizeof(int)},
	{WLC_SET_KEY_PRIMARY, KM_IOCTL_IOCF_FLAGS, sizeof(int)},
	{WLC_SET_WSEC_TEST, KM_IOCTL_IOCF_FLAGS, sizeof(int)}
};

static int
km_ioctl(void *ctx, uint32 cmd, void *arg, uint len, struct wlc_if *wlcif)
{
	keymgmt_t *km = (wlc_keymgmt_t*)ctx;
	wlc_info_t *wlc;
	int err = BCME_OK;
	int val;
	int *pval;
	wlc_bsscfg_t *bsscfg;
	wlc_key_id_t key_id;

	KM_DBG_ASSERT(KM_VALID(km)); /* wlcif may be NULL (primary) */
	wlc = km->wlc;
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	KM_DBG_ASSERT(bsscfg != NULL);

	pval = (int *)arg;
	if (pval != NULL && (uint32)len >= sizeof(val))
		memcpy(&val, pval, sizeof(val));
	else
		val = 0;

	KM_TRACE(("wl%d.%d: %s: cmd %d, val 0x%x (%d) len %d\n",
		WLCWLUNIT(wlc), WLC_BSSCFG_IDX(bsscfg), __FUNCTION__,
		cmd, val, val, len));

	switch (cmd) {
	case WLC_SET_KEY:
		err = km_doiovar_wrapper(km, IOV_SVAL(IOV_WSEC_KEY),
			NULL, 0, arg, len, 0, wlcif);
		break;

#if defined(WLMOTOROLALJ)
	case WLC_GET_KEY: {
		wl_wsec_key_t wl_key;

		if ((size_t)len < sizeof(wl_key)) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		memset(&wl_key, 0, sizeof(wl_key));
		wl_key.index = val;
		err = km_doiovar_wrapper(km, IOV_GVAL(IOV_WSEC_KEY), NULL, 0,
			&wl_key, sizeof(wl_key), 0, wlcif);
		if (err != BCME_OK)
			break;

		memcpy(arg, &wl_key, sizeof(wl_key));
		break;
	}
#endif 

	case WLC_GET_KEY_SEQ:
		err = km_doiovar_wrapper(km, IOV_GVAL(IOV_WSEC_KEY_SEQ), &val, sizeof(val),
			arg, len, 0, wlcif);
		break;
	case WLC_GET_KEY_PRIMARY:
		if ((uint)len < sizeof(val)) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		key_id = wlc_keymgmt_get_bss_tx_key_id(wlc->keymgmt, bsscfg, FALSE);
		if (pval != NULL)
			*pval = key_id == val ? TRUE : FALSE;
		else
			err = BCME_BADARG;

		break;
	case WLC_SET_KEY_PRIMARY:
		if ((uint)len < sizeof(val)) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		err = wlc_keymgmt_set_bss_tx_key_id(wlc->keymgmt, bsscfg, (wlc_key_id_t)val, FALSE);
		break;
	}

	if (err != BCME_OK) {
		if (VALID_BCMERROR(err))
			wlc->pub->bcmerror = err;
	}
	return err;
}

/* public interface */
int
BCMATTACHFN(km_register_ioctl)(keymgmt_t *km)
{
	KM_DBG_ASSERT(KM_VALID(km));
	return wlc_module_add_ioctl_fn(KM_PUB(km), km, km_ioctl,
		ARRAYSIZE(km_ioctls), km_ioctls);
}

int
BCMATTACHFN(km_unregister_ioctl)(keymgmt_t *km)
{
	KM_DBG_ASSERT(KM_VALID(km));
	return wlc_module_remove_ioctl_fn(KM_PUB(km), km);
}
