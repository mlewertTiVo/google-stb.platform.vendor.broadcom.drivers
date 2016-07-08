/*
 * FIPS handler for
 * Broadcom 802.11abg Networking Device Driver
 *
 * @file
 * @brief
 * This file contains the functions for FIPS(Federal
 * Information Processing Standard).  The functions
 * work only in Windows environment with FUNK's software.
 * See sample in FIPS "Odyssey Client FIPS Adaptation
 * Kit For NIC Drivers"
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wl_ndfips.c 476923 2014-05-10 07:11:25Z $
 */


#include <wlc_cfg.h>

#error "NDIS is not defined"

#error "WLFIPS is not defined"

#include <epivers.h>

#include <typedefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <wlioctl.h>
#include <siutils.h>
#include <proto/802.11.h>
#include <sbhndpio.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <oidencap.h>
#include <wlc_keymgmt.h>
#include <wlc_rate.h>
#include <wlc_channel.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <bcmcrypto/aes.h>
#if (0>= 0x0600)
#include <wl_ndis.h>
#else
#include <wl_ndis5.h>
#endif 
#include <wl_ndfips.h>

/* Funk odFIPS public headers */
#include <odFIPSAPI.h>
/* Funk odFIPS driver interface header */
#include <ntddodfips.h>

#define	TARGET_OD_FIPS_API_VERSION	1	/* fips driver API version expected */

#if	(TARGET_OD_FIPS_API_VERSION < OD_FIPS_MIN_API_VERSION) || \
	(TARGET_OD_FIPS_API_VERSION > OD_FIPS_API_VERSION)
#error "Incompatible odFIPSAPI.h"
#endif

struct wl_fips_info {

	wlc_pub_t	*pub;		/* pointer to public wlc state */
	wlc_info_t	*wlc;		/* pointer to private common os-independent data */
	uint8		flags;		/* fips mode configuration pending */

	NDIS_HANDLE adapterhandle;	/* miniport driver handle */
	NDIS_WORK_ITEM	work_item;	/* work item for setting mode at PASSIVE level */

	odFIPS_VTable	*funk_func_tbl;	/* pointer to funk FIPS driver function dispatch table */

	HANDLE		api_handle;	/* brcm API handle */
	FuncTbl		*brcm_func_tbl;	/* pointer to brcm FIPS function dispatch table */

	PFILE_OBJECT	FileObject;	/* FileObject reference to keep FIPS driver in memory */

	/* key contexts */
	bool			key_set[WLC_KEYMGMT_NUM_GROUP_KEYS];	/* key avail? */
	OD_FIPS_AES_CCM_CTX	key_ctx[WLC_KEYMGMT_NUM_GROUP_KEYS];	/* funk fips driver */
};

/* flag bits */
#define PENDING_REQ		0x01	/* request pending for mode change */
#define FIPS_PROHIBITED	0x02	/* FIPS mode is prohibited */
#define FUNK_FIPS_API	0x04	/* set if use FUNK's FIPS API */
#define BRCM_FIPS_API	0x08	/* set if use Broadcom FIPS API */

/* set/reset wsec FIPS bit */
#define SET_FIPS_WSEC(wsec, enab) {if (enab) (wsec) |= FIPS_ENABLED; else (wsec) &= ~FIPS_ENABLED;}

/* macros handle API types */
#define IS_FUNK_API(flags) ((flags) & FUNK_FIPS_API)
#define IS_BRCM_API(flags) ((flags) & BRCM_FIPS_API)
#define CHECK_API_TYPE(oid, flags) \
	(((oid) != OID_FSW_FIPS_MODE && (oid) != OID_BCM_FIPS_MODE) || \
	(((oid) == OID_FSW_FIPS_MODE && IS_BRCM_API(flags)) ||	\
	((oid) == OID_BCM_FIPS_MODE && IS_FUNK_API(flags))))
#define SET_API_TYPE(oid, flags) \
	WL_TRACE(("set %s API\n", ((oid) == OID_FSW_FIPS_MODE ? "Funk" : "Broadcom"))); \
	flags |= ((oid) == OID_FSW_FIPS_MODE ? FUNK_FIPS_API : BRCM_FIPS_API)
#define ENAB_FIPS_MODE_VAL(flags) \
	(IS_FUNK_API(flags) ? FswFIPSMode_Enabled : FIPS_MODE_ENABLE)
#define DISAB_FIPS_MODE_VAL(flags) \
	(IS_FUNK_API(flags) ? FswFIPSMode_Disabled : FIPS_MODE_DISABLE)
#define FIPS_API_VERSION(flags) \
	(uint32)(IS_FUNK_API(flags) ? TARGET_OD_FIPS_API_VERSION : TARGET_FIPS_API_VERSION)
#define FIPS_FUNC_GET_API_VER(fi, min_ver, cur_ver, tbl_sz) \
	(IS_FUNK_API(fi->flags) ? \
	fi->funk_func_tbl->fodFIPS_GetAPIVersion(&min_ver, &cur_ver, &tbl_sz) : \
	fi->brcm_func_tbl->FipsGetDriverVersion(&min_ver, &cur_ver))
#define FIPS_FUNC_DRIVER_ATTACH(fi, err) \
	err = BCME_OK; \
	if (IS_BRCM_API(fi->flags) && \
		(fi->api_handle = fi->brcm_func_tbl->FipsAttach()) == NULL) \
		err = BCME_ERROR;
#define FIPS_FUNC_DRIVER_DETACH(fi) \
	if (IS_BRCM_API(fi->flags)) \
		fi->brcm_func_tbl->FipsDetach(fi->api_handle);
#define FIPS_FUNC_ADD_KEY(fi, key, key_idx) \
	(IS_FUNK_API(fi->flags) ? \
	fi->funk_func_tbl->fodFIPS_AES_CCM_Init(&fi->key_ctx[key_idx], \
		sizeof(fi->key_ctx[key_idx]), key->KeyMaterial, key->KeyLength) : \
	fi->brcm_func_tbl->FipsAddKey(fi->api_handle, key->KeyMaterial, \
		key->KeyLength, key_index))
#define FIPS_FUNC_REMOVE_KEY(fi, key_idx) \
	(IS_FUNK_API(fi->flags) ? \
	fi->funk_func_tbl->fodFIPS_AES_CCM_Term(&fi->key_ctx[key_idx]) : \
	fi->brcm_func_tbl->FipsRemoveKey(fi->api_handle, key_idx))
#define FIPS_FUNC_ENAB_FIPS(fi, err) \
	if (IS_FUNK_API(fi->flags)) { \
		fi->funk_func_tbl->fodFIPS_EnableFIPSMode(NULL); \
		err = (fi->funk_func_tbl->fodFIPS_GetState() != OD_FIPS_STATE__ENABLED); \
	} else \
		err = (bool)fi->brcm_func_tbl->FipsSetMode(fi->api_handle, FIPS_MODE_ENABLE);
#define FIPS_FUNC_DISAB_FIPS(fi) \
	if (IS_BRCM_API(fi->flags)) \
		fi->brcm_func_tbl->FipsSetMode(fi->api_handle, FIPS_MODE_DISABLE);
#define FIPS_FUNC_ENCRYPT_DATA(fi, key_idx, nonce, aad, la, buf_in, buf_out, len, mic) \
	(IS_FUNK_API(fi->flags) ? \
	fi->funk_func_tbl->fodFIPS_AES_CCM_EncryptMessage(\
		&fi->key_ctx[key_idx], nonce, AES_CCMP_NONCE_LEN, aad, la, \
		buf_in, buf_out, len, mic, AES_CCM_AUTH_LEN) != OD_FIPS_SUCCESS : \
	fi->brcm_func_tbl->FipsEncryptData(\
		fi->api_handle, key_idx, nonce, AES_CCMP_NONCE_LEN, aad, la, \
		buf_in, buf_out, len, mic, AES_CCM_AUTH_LEN))
#define FIPS_FUNC_DECRYPT_DATA(fi, key_idx, nonce, aad, la, buf_in, buf_out, len, mic) \
	(IS_FUNK_API(fi->flags) ? \
	fi->funk_func_tbl->fodFIPS_AES_CCM_DecryptMessage(\
		&fi->key_ctx[key_idx], nonce, AES_CCMP_NONCE_LEN, aad, la, \
		buf_in, buf_out, len, mic, AES_CCM_AUTH_LEN) != OD_FIPS_SUCCESS : \
	fi->brcm_func_tbl->FipsDecryptData(\
		fi->api_handle, key_idx, nonce, AES_CCMP_NONCE_LEN, aad, la, \
		buf_in, buf_out, len, mic, AES_CCM_AUTH_LEN))

/* iovar table */
enum {
	IOV_FIPS_OID,	/* request from NDIS OID handle */
	IOV_FIPS_ADD_KEY,
	IOV_FIPS_REMOVE_KEY,
	IOV_FIPS
};

static const bcm_iovar_t fips_iovars[] = {
	{"fips_oid", IOV_FIPS_OID, (0), IOVT_BUFFER, 0},
	{"fips_add_key", IOV_FIPS_ADD_KEY, (0), IOVT_BUFFER, 0},
	{"fips_remove_key", IOV_FIPS_REMOVE_KEY, (0), IOVT_BUFFER, 0},
	{"fips", IOV_FIPS, (0), IOVT_UINT32, 0},	/* internal use by wl fips command */
	{NULL, 0, 0, 0, 0}
};

/* definition for fips command. must sync with wl command */
#define DISABLE_FIPS	0
#define ENABLE_FIPS		1
#define PROHIBIT_FIPS	-1

static int wl_fips_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid,
	const char *name, void *params, uint p_len, void *arg, int len, int val_size,
	struct wlc_if *wlcif);
static int wl_fips_load_driver(wl_fips_info_t *fi);
static void wl_fips_unload_driver(wl_fips_info_t *fi);
static int wl_fips_set_mode(wl_fips_info_t *fi, int fips_mode);
static void wl_fips_mode_enab(PNDIS_WORK_ITEM work_item, wl_fips_info_t *fi);
static int wl_fips_mode_do_enab(wl_fips_info_t *fi);
static int wl_fips_add_key(wl_fips_info_t *fi, PNDIS_802_11_KEY key);
static int wl_fips_remove_key(wl_fips_info_t *fi, PNDIS_802_11_REMOVE_KEY key);

wl_fips_info_t *
wl_fips_attach(wlc_info_t *wlc, NDIS_HANDLE adapterhandle)
{
	wl_fips_info_t *fi;

	WL_TRACE(("wl%d: %s\n", wlc->pub->unit, __FUNCTION__));

	if ((fi = (wl_fips_info_t *)MALLOCZ(wlc->osh, sizeof(wl_fips_info_t))) == NULL) {
		WL_ERROR(("wl%d: wl_fips_attach: wl_fips_info_t out of memory, malloced %d bytes",
			wlc->pub->unit, MALLOCED(wlc->osh)));
		return NULL;
	}

	fi->wlc = wlc;
	fi->pub = wlc->pub;
	fi->adapterhandle = adapterhandle;

	/* register module */
	if (wlc_module_register(fi->pub, fips_iovars, "fips", fi,
	                        wl_fips_doiovar, NULL, NULL, NULL)) {
		WL_ERROR(("wl%d: %s wlc_module_register() failed\n",
		          fi->pub->unit, __FUNCTION__));
		return NULL;
	}

	return fi;
}

void
wl_fips_detach(wl_fips_info_t *fi)
{
	if (!fi)
		return;

	WL_TRACE(("wl%d: %s: fi = %p\n", fi->pub->unit, __FUNCTION__, fi));
	/* unregister module */
	wlc_module_unregister(fi->pub, "fips", fi);
	/* disable FIPS and unload driver */
	wl_fips_set_mode(fi, DISABLE_FIPS);
	MFREE(fi->pub->osh, fi, sizeof(wl_fips_info_t));
}

static int
wl_detect_api(wl_fips_info_t *fi)
{
	NDIS_STATUS status;
	PDEVICE_OBJECT deviceObject;
	PFILE_OBJECT fileObject = NULL;
	NDIS_STRING funk_deviceName = NDIS_STRING_CONST("" DD_ODFIPS_DEVICE_NAME_W);
	NDIS_STRING brcm_deviceName = NDIS_STRING_CONST("" FIPS_DEVICE_NAME_W);
	int err;

	WL_TRACE(("wl%d: %s\n", fi->pub->unit, __FUNCTION__));

	ASSERT(fi->FileObject == NULL);

	do {
		/* open funk device object first */
		status = IoGetDeviceObjectPointer(
			&funk_deviceName,
			FILE_READ_DATA,
			&fileObject, &deviceObject);

		if (NT_SUCCESS(status)) {
			fi->flags |= FUNK_FIPS_API;
			err = BCME_OK;
			break;
		}

		/* open brcm device object */
		status = IoGetDeviceObjectPointer(
			&brcm_deviceName,
			FILE_READ_DATA,
			&fileObject, &deviceObject);

		if (NT_SUCCESS(status)) {
			fi->flags |= BRCM_FIPS_API;
			err = BCME_OK;
			break;
		}

		err = BCME_ERROR;
	} while (FALSE);

	if (fileObject)
		ObDereferenceObject(fileObject);

	return err;
}

static int
wl_fips_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int val_size, struct wlc_if *wlcif)
{
	wl_fips_info_t *fi = hdl;
	int32 val, int_val = 0;
	bool bool_val;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)arg;
	fips_oid_req_t *oid_req;

	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	bool_val = (int_val != 0) ? TRUE : FALSE;

	switch (actionid) {
	case IOV_SVAL(IOV_FIPS_OID):
		/* FIPS mode can only be changed when STA not associated */
		if (fi->pub->associated) {
			err = BCME_ERROR;
			break;
		}

		oid_req = (fips_oid_req_t*)arg;

		/* check current FIPS API. only one set API could be activated */
		if (CHECK_API_TYPE(oid_req->oid, fi->flags)) {
			err = BCME_UNSUPPORTED;
			break;
		}

		/* set API type(FUNK or Broadcom) */
		SET_API_TYPE(oid_req->oid, fi->flags);

		/* FIPS can not be enabled if FIPS is disallowed */
		if (fi->flags & FIPS_PROHIBITED && oid_req->val == ENAB_FIPS_MODE_VAL(fi->flags)) {
			err = BCME_UNSUPPORTED;
			break;
		}

		/* set FIPS mode */
		val = oid_req->val == ENAB_FIPS_MODE_VAL(fi->flags) ? ENABLE_FIPS : DISABLE_FIPS;
		err = wl_fips_set_mode(fi, val);
		break;

	case IOV_GVAL(IOV_FIPS_OID):
		oid_req = (fips_oid_req_t*)arg;

		/* check current FIPS API.  only one set API could be activated */
		if (CHECK_API_TYPE(oid_req->oid, fi->flags)) {
			err = BCME_UNSUPPORTED;
			break;
		}
		/* set API type(FUNK or Broadcom) */
		SET_API_TYPE(oid_req->oid, fi->flags);

		oid_req->val = FIPS_ENAB(fi->wlc) ?
			ENAB_FIPS_MODE_VAL(fi->flags) : DISAB_FIPS_MODE_VAL(fi->flags);
		break;

	case IOV_SVAL(IOV_FIPS_ADD_KEY):
		err = wl_fips_add_key(fi, (PNDIS_802_11_KEY)arg);
		break;

	case IOV_SVAL(IOV_FIPS_REMOVE_KEY):
		err = wl_fips_remove_key(fi, (PNDIS_802_11_REMOVE_KEY)arg);
		break;

	case IOV_SVAL(IOV_FIPS):	/* internal interface for FIPS mode */
		/* FIPS mode can only be changed when STA not associated */
		if (fi->pub->associated) {
			err = BCME_ERROR;
			break;
		}
		/* FIPS can not be enabled if FIPS is disallowed */
		if (fi->flags & FIPS_PROHIBITED && int_val == ENABLE_FIPS) {
			err = BCME_BADOPTION;
			break;
		}

		if (int_val == PROHIBIT_FIPS) {
			/* once prohibited flag set, have to set registry value and
			 * reload driver to re-active FIPS
			 */
			int_val = DISABLE_FIPS;
			fi->flags |= FIPS_PROHIBITED;
			if (!FIPS_ENAB(fi->wlc)) {
				break;
			}
		}

		/* must be from wl command */
		if ((fi->flags & (FUNK_FIPS_API | BRCM_FIPS_API)) == 0) {
			/* detect fips driver */
			err = wl_detect_api(fi);
			if (err != BCME_OK)
				break;
		}

		/* set FIPS mode */
		err = wl_fips_set_mode(fi, int_val);
		break;

	case IOV_GVAL(IOV_FIPS):
		if (fi->flags & FIPS_PROHIBITED)
			*ret_int_ptr = PROHIBIT_FIPS;
		else
			*ret_int_ptr = FIPS_ENAB(fi->wlc) ? ENABLE_FIPS : DISABLE_FIPS;
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* load FIPS kernel driver */
static int
wl_fips_load_driver(wl_fips_info_t *fi)
{
	NDIS_STATUS status;
	PDEVICE_OBJECT deviceObject;
	PFILE_OBJECT fileObject = NULL;
	NDIS_STRING funk_deviceName = NDIS_STRING_CONST("" DD_ODFIPS_DEVICE_NAME_W);
	NDIS_STRING brcm_deviceName = NDIS_STRING_CONST("" FIPS_DEVICE_NAME_W);
	PIRP irp;
	KEVENT event;
	IO_STATUS_BLOCK ioStatus;
	F_odFIPS_GetVTable funk_get_func_tbl = NULL;
	pFipsGetFuncTbl brcm_get_func_tbl = NULL;
	uint32 cur_ver, min_ver, func_tbl_size;
	int err;

	WL_TRACE(("wl%d: %s\n", fi->pub->unit, __FUNCTION__));

	/* if already loaded, return */
	if (fi->FileObject)
		return BCME_OK;

	/* open the device object */
	status = IoGetDeviceObjectPointer(
		(fi->flags & FUNK_FIPS_API ? &funk_deviceName : &brcm_deviceName),
		FILE_READ_DATA,
		&fileObject, &deviceObject);

	if (!(NT_SUCCESS(status))) {
		WL_ERROR(("wl%d: wl_fips_load_driver: open device error. status 0x%x\n",
			fi->pub->unit, status));
		goto err_exit;
	}

	/* initialize a notification event to wait on for I/O completion */
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	/* build an IRP_MJ_INTERNAL_DEVICE_CONTROL request to retrieve the
	 * function table pointer from FIPS driver
	 */
	if (fi->flags & FUNK_FIPS_API) {
		irp = IoBuildDeviceIoControlRequest(
			IOCTL_ODFIPS_GET_PROC_ADDRESS_V1_1,
			deviceObject,
			ODFIPS_GET_VTABLE_PROC_NAME, sizeof(ODFIPS_GET_VTABLE_PROC_NAME),
			&funk_get_func_tbl, sizeof(funk_get_func_tbl),
			TRUE,	/* IRP_MJ_INTERNAL_DEVICE_CONTROL */
			&event,
			&ioStatus);
	} else {	/* brcm api */
		irp = IoBuildDeviceIoControlRequest(
			IOCTL_FIPS_GET_PROC_TBL_V1,
			deviceObject,
			FIPS_GET_FUNC_TBL_REQ, sizeof(FIPS_GET_FUNC_TBL_REQ),
			&brcm_get_func_tbl, sizeof(brcm_get_func_tbl),
			TRUE,	/* IRP_MJ_INTERNAL_DEVICE_CONTROL */
			&event,
			&ioStatus);
	}

	if (!irp) {
		WL_ERROR(("wl%d: wl_fips_load_driver: error: IoBuildDeviceIoControlRequest\n",
			fi->pub->unit));
		goto err_exit;
	}

	/* call the driver to get the function table */
	status = IoCallDriver(deviceObject, irp);
	if (!(NT_SUCCESS(status))) {
		WL_ERROR(("wl%d: wl_fips_load_driver: error: IoCallDriver\n", fi->pub->unit));
		goto err_exit;
	}

	/* wait for the event to be signaled before continuing */
	status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
	if (!(NT_SUCCESS(status))) {
		WL_ERROR(("wl%d: wl_fips_load_driver: error: KeWaitForSingleObject\n",
			fi->pub->unit));
		goto err_exit;
	}

	/* get the status of the request */
	status = ioStatus.Status;
	if (!(NT_SUCCESS(status))) {
		WL_ERROR(("wl%d: wl_fips_load_driver: error: io status\n", fi->pub->unit));
		goto err_exit;
	}

	/* check if the function pointer came back */
	if (!funk_get_func_tbl && !brcm_get_func_tbl) {
		WL_ERROR(("wl%d: wl_fips_load_driver: error: no function pointer\n",
			fi->pub->unit));
		goto err_exit;
	}

	/* get the VTable */
	if (fi->flags & FUNK_FIPS_API) {
		fi->funk_func_tbl = funk_get_func_tbl();
		if (!fi->funk_func_tbl) {
			WL_ERROR(("wl%d: wl_fips_load_driver: error: no function table\n",
				fi->pub->unit));
			goto err_exit;
		}
	} else {
		fi->brcm_func_tbl = brcm_get_func_tbl();
		if (!fi->brcm_func_tbl) {
			WL_ERROR(("wl%d: wl_fips_load_driver: error: no function table\n",
				fi->pub->unit));
			goto err_exit;
		}
	}

	/* get FIPS driver version information and verify that it is compatible */
	if (FIPS_FUNC_GET_API_VER(fi, min_ver, cur_ver, func_tbl_size)) {
		WL_ERROR(("wl%d: wl_fips_load_driver: error: getting version\n", fi->pub->unit));
		goto err_exit;
	} else if (min_ver > FIPS_API_VERSION(fi->flags)) {
		WL_ERROR(("wl%d: wl_fips_load_driver: error: FIPS driver version too recent\n",
			fi->pub->unit));
		goto err_exit;
	} else if (cur_ver < FIPS_API_VERSION(fi->flags)) {
		WL_ERROR(("wl%d: wl_fips_load_driver: error: FIPS driver version too old\n",
			fi->pub->unit));
		goto err_exit;
	}

	FIPS_FUNC_DRIVER_ATTACH(fi, err);
	if (err) {
		WL_ERROR(("wl%d: wl_fips_load_driver: error: attach failed\n", fi->pub->unit));
		goto err_exit;
	}

	/* fips driver successfully loaded */
	fi->FileObject = fileObject;
	return BCME_OK;

err_exit:
	/* cleanup the fileObject referenc */
	if (fileObject)
		ObDereferenceObject(fileObject);
	return BCME_ERROR;
}

/* unload FIPS kernel driver */
static void
wl_fips_unload_driver(wl_fips_info_t *fi)
{
	WL_TRACE(("wl%d: %s\n", fi->pub->unit, __FUNCTION__));

	if (!fi->FileObject)
		return;

	/* detach fips driver */
	FIPS_FUNC_DRIVER_DETACH(fi);

	/* clean up api flags */
	fi->flags &= ~(FUNK_FIPS_API | BRCM_FIPS_API);

	/* remove FIPS driver function table and release the FileObject reference */
	fi->funk_func_tbl = NULL;
	fi->brcm_func_tbl = NULL;

	/* dereference fips driver */
	ObDereferenceObject(fi->FileObject);
	fi->FileObject = NULL;
}

/* set FIPS mode.  If enabled, load FIPS driver and use
 * its sw functions to handle AES_CCMP encryption/decryption.
 * If disabled, remove the key from FIPS driver and
 * unload it
 */
static int
wl_fips_set_mode(wl_fips_info_t *fi, int fips_mode)
{
	wlc_info_t *wlc = fi->wlc;
	int status;
	uint32 key_index;

	WL_TRACE(("wl%d: %s\n", fi->pub->unit, __FUNCTION__));

	/* if mode already set, done */
	if ((fips_mode == ENABLE_FIPS && FIPS_ENAB(wlc)) ||
		(fips_mode == DISABLE_FIPS && !FIPS_ENAB(wlc)))
		return BCME_OK;

	switch (fips_mode) {
	case DISABLE_FIPS:
		/* set fips mode flag */
		SET_FIPS_WSEC(wlc->cfg->wsec, FALSE);
		/* cleanup any key contexts that have been set */
		for (key_index = 0; key_index < WLC_KEYMGMT_NUM_GROUP_KEYS; key_index++) {
			if (fi->key_set[key_index]) {
				FIPS_FUNC_REMOVE_KEY(fi, key_index);
				fi->key_set[key_index] = FALSE;
			}
		}

		/*
		 * NOTE from FUNK for the issue in their fips driver:
		 * Do not disable FIPS mode in the odFIPS driver. The odFIPS driver
		 * does not 'count' the number enables / disables and thus disabling
		 * FIPS mode will cause other clients of odFIPS to fail. Leaving
		 * FIPS mode enabled does not consume any resources and thus it is
		 * safe to do.
		 */

		/* disable fips mode */
		FIPS_FUNC_DISAB_FIPS(fi);

		/* unload fips driver */
		wl_fips_unload_driver(fi);
		/* remove all keys */
		wlc_keymgmt_reset(wlc->keymgmt, wlc->cfg, NULL);
		status = BCME_OK;
		break;
	case ENABLE_FIPS:
		status = wl_fips_mode_do_enab(fi);
		status = (status == NDIS_STATUS_SUCCESS) ? BCME_OK : BCME_ERROR;
		break;
	default:
		/* the specified value of FipsMode is invalid */
		status = BCME_BADOPTION;
		break;
	}

	return status;
}

/* enable FIPS mode at PASSIVE level required by the FIPS enabling
 * function inside the FUNK's FIPS driver
 * NOTE: this function is not protected by spin lock, but use a
 * flag PENDING_REQ to enforce only one request can go through
 */
static void
wl_fips_mode_enab(PNDIS_WORK_ITEM work_item, wl_fips_info_t *fi)
{
	int status;

	ASSERT((&fi->work_item) == work_item);

	status = wl_fips_mode_do_enab(fi);

	/* complete the pending NDIS Request.  do not call ndis completion function
	 * if request is not from NDIS
	 */
	if (fi->flags & PENDING_REQ) {
		NdisMSetInformationComplete(fi->adapterhandle, status);
		fi->flags &= ~PENDING_REQ;
	}
}

static int
wl_fips_mode_do_enab(wl_fips_info_t *fi)
{
	int status;

	WL_TRACE(("wl%d: %s\n", fi->pub->unit, __FUNCTION__));

	ASSERT(!FIPS_ENAB(fi->wlc));

	/* if associated, error */
	if (fi->pub->associated)
		status = IS_FUNK_API(fi->flags) ?
			FSW_ERROR_FIPS_GENERAL_FAILURE : NDIS_STATUS_FAILURE;
	/* load fips driver */
	else if (wl_fips_load_driver(fi)) {
		WL_ERROR(("wl%d: wl_fips_mode_enab: wl_fips_load_driver fails\n", fi->pub->unit));
		status = IS_FUNK_API(fi->flags) ? FSW_ERROR_FIPS_LINK : NDIS_STATUS_FAILURE;
	} else {
		bool err;
		/* enable FIPS mode in fips driver */
		FIPS_FUNC_ENAB_FIPS(fi, err);
		if (err) {
			/* the driver could not be placed into FIPS mode.  unload the driver */
			wl_fips_unload_driver(fi);
			status = IS_FUNK_API(fi->flags) ?
				FSW_ERROR_FIPS_INITIALIZE : NDIS_STATUS_FAILURE;
		} else {
			/* remove all keys */
			wlc_keymgmt_reset(fi->wlc->keymgmt, fi->wlc->cfg, NULL);
			/* set fips mode flag */
			SET_FIPS_WSEC(fi->wlc->cfg->wsec, TRUE);
			status = NDIS_STATUS_SUCCESS;
		}
	}

	return status;
}

/* handle OID_802_11_ADD_KEY */
static int
wl_fips_add_key(wl_fips_info_t *fi, PNDIS_802_11_KEY key)
{
	uint32 key_index;

	WL_TRACE(("wl%d: %s: key index %d\n", fi->pub->unit, __FUNCTION__, (key->KeyIndex & 0xff)));

	/* if FIPS disabled, ignore */
	if (!FIPS_ENAB(fi->wlc))
		return BCME_OK;

	key_index = key->KeyIndex & 0xff;

	/* release any previous key context */
	if (fi->key_set[key_index]) {
		FIPS_FUNC_REMOVE_KEY(fi, key_index);
		fi->key_set[key_index] = FALSE;
	}

	/* use the specified key material to initialize the key context */
	if (FIPS_FUNC_ADD_KEY(fi, key, key_index) != OD_FIPS_SUCCESS) {
		WL_ERROR(("wl%d: wl_fips_add_key: add key error\n", fi->pub->unit));
		return BCME_ERROR;
	}

	WL_TRACE(("wl%d: wl_fips_add_key: key index %d\n", fi->pub->unit, key_index));
	fi->key_set[key_index] = TRUE;
	return BCME_OK;
}

/* handle OID_802_11_REMOVE_KEY */
static int
wl_fips_remove_key(wl_fips_info_t *fi, PNDIS_802_11_REMOVE_KEY key)
{
	uint32 key_index;

	WL_TRACE(("wl%d: %s: key index %d\n", fi->pub->unit, __FUNCTION__, (key->KeyIndex & 0xff)));

	/* if FIPS disabled, ignore */
	if (FIPS_ENAB(fi->wlc)) {
		key_index = key->KeyIndex & 0xff;
		/* release any previous key context */
		if (fi->key_set[key_index]) {
			FIPS_FUNC_REMOVE_KEY(fi, key_index);
			fi->key_set[key_index] = FALSE;
		}
	}

	return BCME_OK;
}

/* encrypt packet */
int
wl_fips_encrypt_pkt(wl_fips_info_t *fi, uint8 key_index, const struct dot11_header *h, uint len,
	uint8 nonce_1st_byte)
{
	uint8 *mic = (uint8*)h + len;
	uint8 nonce[AES_CCMP_NONCE_LEN], aad[AES_CCMP_AAD_MAX_LEN];
	uint la, lh;

	WL_TRACE(("wl%d: %s: key index %d, pkt len %d\n", fi->pub->unit, __FUNCTION__,
		key_index, len));

	if (!FIPS_ENAB(fi->wlc)) {
		WL_ERROR(("wl%d: wl_fips_encrypt_pkt: FIPS mode error\n", fi->pub->unit));
		return BCME_ERROR;
	}

	if (key_index >= WLC_KEYMGMT_NUM_GROUP_KEYS) {
		WL_ERROR(("wl%d: wl_fips_encrypt_pkt: key index(%d) error\n", fi->pub->unit,
			key_index));
		return BCME_ERROR;
	}

	if (!fi->key_set[key_index]) {
		WL_ERROR(("wl%d: wl_fips_encrypt_pkt: no key at index %d\n", fi->pub->unit,
			key_index));
		return BCME_ERROR;
	}

	/* calculate nonce and aad */
	aes_ccmp_cal_params(h, FALSE, nonce_1st_byte, nonce, aad, &la, &lh);

	/* encrypt packet */
	if (FIPS_FUNC_ENCRYPT_DATA(fi, key_index, nonce, aad, la,
		(uint8*)h + lh, (uint8*)h + lh, len - lh, mic)) {
		WL_ERROR(("wl%d: wl_fips_encrypt_pkt: pkt encryption error\n", fi->pub->unit));
		return BCME_ERROR;
	}

	return BCME_OK;
}

/* decrypt packet */
int
wl_fips_decrypt_pkt(wl_fips_info_t *fi, uint8 key_index, const struct dot11_header *h, uint len,
	uint8 nonce_1st_byte)
{
	uint8 *mic = (uint8*)h + len - AES_CCM_AUTH_LEN;
	uint8 nonce[AES_CCMP_NONCE_LEN], aad[AES_CCMP_AAD_MAX_LEN];
	uint la, lh;

	WL_TRACE(("wl%d: %s: key index %d, pkt len %d\n", fi->pub->unit, __FUNCTION__,
		key_index, len));

	if (!FIPS_ENAB(fi->wlc)) {
		WL_ERROR(("wl%d: wl_fips_decrypt_pkt: FIPS mode error\n", fi->pub->unit));
		return BCME_ERROR;
	}

	if (key_index >= WLC_KEYMGMT_NUM_GROUP_KEYS) {
		WL_ERROR(("wl%d: wl_fips_decrypt_pkt: key index(%d) error\n", fi->pub->unit,
			key_index));
		return BCME_ERROR;
	}

	if (!fi->key_set[key_index]) {
		WL_ERROR(("wl%d: wl_fips_decrypt_pkt: no key at index %d\n",
			fi->pub->unit, key_index));
		return BCME_ERROR;
	}

	/* calculate nonce and aad */
	aes_ccmp_cal_params(h, FALSE, nonce_1st_byte, nonce, aad, &la, &lh);

	/* decrypt packet */
	if (FIPS_FUNC_DECRYPT_DATA(fi, key_index, nonce, aad, la,
		(uint8*)h + lh, (uint8*)h + lh, len - lh - AES_CCM_AUTH_LEN, mic)) {
		WL_ERROR(("wl%d: wl_fips_decrypt_pkt: pkt decryption error.\n", fi->pub->unit));
		return BCME_ERROR;
	}

	return BCME_OK;
}
