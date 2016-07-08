/*
 * FIPS function header
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wl_ndfips.h 467328 2014-04-03 01:23:40Z $
 */


#ifndef	_wl_ndfips_h_
#define	_wl_ndfips_h_

/* Funk Software Private OID/GUID definitions */
#include <fswPrivate.h>

/* Broadcom FIPS definitions */
#ifndef GUID_DEFINED
#include <guiddef.h>
#endif	/* !GUID_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(CGUID_BCM_FIPS_MODE, 0xdfede317, 0x00e9, 0x40c5, 0xa1, 0x27, 0x02,
	0x51, 0x26, 0x70, 0xd7, 0x0a);

#define CGUID_BCM_FIPS_MODE \
	{0xdfede317, 0x00e9, 0x40c5, {0xa1, 0x27, 0x02, 0x51, 0x26, 0x70, 0xd7, 0x0a}}

/* fips device name */
#define FIPS_DEVICE_NAME_W    L"\\Device\\fips"

/* ioctl function */
#define OEM_FUNC_BASE	0x2048
#define FIPS_GET_FUNC_TBL	(OEM_FUNC_BASE + 0)

/* IRP_MJ_INTERNAL_CONTROL values to get fips driver function table */
#define IOCTL_FIPS_GET_PROC_TBL_V1 \
	CTL_CODE(FILE_DEVICE_UNKNOWN, FIPS_GET_FUNC_TBL, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* request to get fips driver function table */
#define	FIPS_GET_FUNC_TBL_REQ	"FIPS_GetFuncTable"

/* fips API version this wl driver supported(one in range of minimum ver. and current ver.) */
#define	TARGET_FIPS_API_VERSION	1

typedef enum _FIPS_MODE {
	FIPS_MODE_DISABLE = 0,
	FIPS_MODE_ENABLE
} FIPS_MODE;

/* function table the fips driver supported.  return status: 0 - success, !0 fail */
typedef struct {
	int (*FipsGetDriverVersion)(int *minimum_version, int *current_version);
	void* (*FipsAttach)(void);
	void (*FipsDetach)(HANDLE api_handle);
	int (*FipsSetMode)(HANDLE api_handle, FIPS_MODE fips_mode);
	int (*FipsAddKey)(HANDLE api_handle, void *key, int key_len, int key_index);
	int (*FipsRemoveKey)(HANDLE api_handle, int key_index);
	int (*FipsEncryptData)(HANDLE api_handle, int key_index, void *NONCE, int NONCE_len,
		void *AAD, int AAD_len, void *in_buf, void *out_buf, int buf_len,
		void *out_mic, int out_mic_len);
	int (*FipsDecryptData)(HANDLE api_handle, int key_index, void *NONCE, int NONCE_len,
		void *AAD, int AAD_len, void *in_buf, void *out_buf, int buf_len,
		void *in_mic, int in_mic_len);
} FuncTbl;

/* get fips driver function table function pointer */
typedef FuncTbl* (*pFipsGetFuncTbl)(void);

#ifdef __cplusplus
}
#endif

/* structure for fips_oid iovar */
typedef struct {
	int	oid;	/* request oid */
	ulong val;	/* request value */
} fips_oid_req_t;

/* forward declaration */
typedef struct wl_fips_info wl_fips_info_t;

extern wl_fips_info_t *wl_fips_attach(wlc_info_t *wlc, NDIS_HANDLE adapterhandle);
extern void wl_fips_detach(void *ctx);
extern int wl_fips_encrypt_pkt(wl_fips_info_t *fi, uint8 key_index,
	const struct dot11_header *h, uint len, uint8 nonce_1st_byte);
extern int wl_fips_decrypt_pkt(wl_fips_info_t *fi, uint8 key_index,
	const struct dot11_header *h, uint len, uint8 nonce_1st_byte);

#endif	/* _wl_ndfips_h_ */
