/*
 *  Name:       uamp.c
 *
 *  Description: Universal AMP API
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: uamp_osl_ext.c 467328 2014-04-03 01:23:40Z $
 *
 */


/* ---- Include Files ---------------------------------------------------- */

#include "typedefs.h"
#include "bcmutils.h"
#include "bcmendian.h"
#include "uamp_api.h"
#include "wlioctl.h"
#include "wl_drv.h"
#include "osl_ext.h"
#include "proto/bt_amp_hci.h"
#include <string.h>
#include <stdlib.h>


/* ---- Public Variables ------------------------------------------------- */
/* ---- Private Constants and Types -------------------------------------- */

#define UAMP_TRACE(a)	printf a
#define UAMP_ERROR(a)	printf a

#if ((BRCM_BLUETOOTH_HOST == 1) && (UAMP_IS_GKI_AWARE == 1))
#include "gki.h"
#define UAMP_ALLOC(a)	GKI_getbuf(a+sizeof(BT_HDR))
#define UAMP_FREE(a)	GKI_freebuf(a)
#else
#define UAMP_ALLOC(a)	malloc(a)
#define UAMP_FREE(a)	free(a)
#endif   /* BRCM_BLUETOOTH_HOST && UAMP_IS_GKI_AWARE */

#define GET_UAMP_FROM_ID(id)	(&g_uamp_mgr.uamp)

#define MAX_IOVAR_LEN	2096


/* State associated with a single universal AMP. */
typedef struct UAMP_STATE
{
	/* Unique universal AMP identifier. */
	tUAMP_ID		id;

	/* Event/data queues. */
	osl_ext_queue_t		evt_q;
	osl_ext_queue_t		pkt_q;

} UAMP_STATE;


/* State associated with collection of univerisal AMPs. */
typedef struct UAMP_MGR
{
	/* Event/data callback. */
	tUAMP_CBACK		callback;

	/* UAMP state. Only support a single AMP currently. */
	UAMP_STATE		uamp;
} UAMP_MGR;


/* ---- Private Variables ------------------------------------------------ */

static UAMP_MGR		g_uamp_mgr;


/* ---- Private Function Prototypes -------------------------------------- */

static int ioctl_set(int cmd, void *buf, int len);
static int iovar_set(const char *iovar, void *param, int paramlen);
static int iovar_setbuf(const char *iovar, void *param, int paramlen, void *bufptr,
                        int buflen);
static int iovar_mkbuf(const char *name, char *data, uint datalen, char *iovar_buf,
                       uint buflen, int *perr);
static int wl_ioctl(int cmd, void *buf, int len, bool set);
static void wl_event_callback(wl_event_msg_t* event, void* event_data);
static int wl_btamp_rx_pkt_callback(wl_drv_netif_pkt pkt, unsigned int len);

#if (BRCM_BLUETOOTH_HOST == 1)
#if (UAMP_IS_GKI_AWARE == 1)
void wl_event_gki_callback(wl_event_msg_t* event, void* event_data);
int wl_btamp_rx_gki_pkt_callback(wl_drv_netif_pkt pkt, unsigned int len);
#endif   /* UAMP_IS_GKI_AWARE */
static void *uamp_get_acl_buf(unsigned int len);
void *hcisu_amp_get_acl_buf(int len);      /* Get GKI buffer from ACL pool */
void hcisu_handle_amp_data_buf(void *pkt, unsigned int len);   /* Send GKI buffer to BTU task */
void hcisu_handle_amp_evt_buf(void* evt, unsigned int len);
int wl_is_drv_init_done(void);
#endif   /* BRCM_BLUETOOTH_HOST */

/* ---- Functions -------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
BT_API BOOLEAN UAMP_Init(tUAMP_CBACK p_cback)
{
	memset(&g_uamp_mgr, 0, sizeof(g_uamp_mgr));
	g_uamp_mgr.callback = p_cback;

	return (TRUE);
}


/* ------------------------------------------------------------------------- */
BT_API BOOLEAN UAMP_Open(tUAMP_ID amp_id)
{
	wl_drv_btamp_callbacks_t	btamp_cbs;
	wl_drv_event_callback 		event_cb;
	UAMP_STATE			*uamp = GET_UAMP_FROM_ID(amp_id);

#if (BRCM_BLUETOOTH_HOST == 1)
	if (!wl_is_drv_init_done()) {
		UAMP_ERROR(("%s: WLAN driver is not initialized! \n", __FUNCTION__));
		return FALSE;
	}
#endif   /* BRCM_BLUETOOTH_HOST */

	/* Create event queue. */
	if (osl_ext_queue_create("uamp_evt", 64, &uamp->evt_q) != OSL_EXT_SUCCESS) {
		UAMP_ERROR(("%s: Error creating event queue!\n", __FUNCTION__));
		return (FALSE);
	}

	/* Create packet queue. */
	if (osl_ext_queue_create("uamp_pkt", 64, &uamp->pkt_q) != OSL_EXT_SUCCESS) {
		UAMP_ERROR(("%s: Error creating packet queue!\n", __FUNCTION__));
		osl_ext_queue_delete(&g_uamp_mgr.uamp.evt_q);
		return (FALSE);
	}


	memset(&btamp_cbs, 0, sizeof(btamp_cbs));

#if ((BRCM_BLUETOOTH_HOST == 1) && (UAMP_IS_GKI_AWARE == 1))
	btamp_cbs.rx_pkt = wl_btamp_rx_gki_pkt_callback;
	btamp_cbs.pkt_alloc = uamp_get_acl_buf;
	event_cb = wl_event_gki_callback;
#elif (BRCM_BLUETOOTH_HOST == 1)
	btamp_cbs.rx_pkt = wl_btamp_rx_pkt_callback;
	btamp_cbs.pkt_alloc = uamp_get_acl_buf;
	event_cb = wl_event_callback;
#else
	btamp_cbs.rx_pkt = wl_btamp_rx_pkt_callback;
	event_cb = wl_event_callback;
#endif   /* BRCM_BLUETOOTH_HOST && UAMP_IS_GKI_AWARE */

	/* Register WLAN HCI event callback for BTAMP. */
	wl_drv_register_event_callback(NULL, event_cb);

	/* Register WLAN HCI data callback for BTAMP. */
	wl_drv_register_btamp_callbacks(NULL, &btamp_cbs);

	return (TRUE);
}


/* ------------------------------------------------------------------------- */
BT_API void UAMP_Close(tUAMP_ID amp_id)
{
	UAMP_STATE	*uamp = GET_UAMP_FROM_ID(amp_id);
	void 		*data;

#if (BRCM_BLUETOOTH_HOST == 1)
	if (!wl_is_drv_init_done()) {
		UAMP_ERROR(("%s: WLAN driver is not initialized! \n", __FUNCTION__));
		return;
	}
#endif   /* BRCM_BLUETOOTH_HOST */

	/* Free queued packets */
	while (osl_ext_queue_receive(&uamp->evt_q, 0, &data) == OSL_EXT_SUCCESS) {
		UAMP_FREE(data);
	}
	osl_ext_queue_delete(&uamp->evt_q);

	/* Free queued packets */
	while (osl_ext_queue_receive(&uamp->pkt_q, 0, &data) == OSL_EXT_SUCCESS) {
		UAMP_FREE(data);
	}
	osl_ext_queue_delete(&uamp->pkt_q);
}


/* ------------------------------------------------------------------------- */
BT_API UINT16 UAMP_Write(tUAMP_ID amp_id, UINT8 *p_buf, UINT16 num_bytes, tUAMP_CH channel)
{
	int ret = -1;
	UINT16 num_bytes_written = num_bytes;

#if (BRCM_BLUETOOTH_HOST == 1)
	if (!wl_is_drv_init_done()) {
		UAMP_ERROR(("%s: WLAN driver is not initialized! \n", __FUNCTION__));
		return (0);
	}
#endif   /* BRCM_BLUETOOTH_HOST */

	if (channel == UAMP_CH_HCI_CMD) {
		ret = iovar_set("HCI_cmd", p_buf, num_bytes);
	}
	else if (channel == UAMP_CH_HCI_DATA) {
		ret = iovar_set("HCI_ACL_data", p_buf, num_bytes);
	}

	if (ret != 0) {
		num_bytes_written = 0;
	        UAMP_ERROR(("UAMP_Write error: %i  ( 0=success )\n", ret));
	}

	return (num_bytes_written);
}


/* ------------------------------------------------------------------------- */
BT_API UINT16 UAMP_Read(tUAMP_ID amp_id, UINT8 *p_buf, UINT16 buf_size, tUAMP_CH channel)
{
	UAMP_STATE		*uamp = GET_UAMP_FROM_ID(amp_id);
	amp_hci_event_t		*evt;
	amp_hci_ACL_data_t	*pkt;
	uint16			dlen;
	osl_ext_status_t	status;

#if (BRCM_BLUETOOTH_HOST == 1)
	if (!wl_is_drv_init_done()) {
		UAMP_ERROR(("%s: WLAN driver is not initialized! \n", __FUNCTION__));
		return (0);
	}
#endif   /* BRCM_BLUETOOTH_HOST */


	if (channel == UAMP_CH_HCI_EVT) {
		/* Dequeue event. */
		status = osl_ext_queue_receive(&uamp->evt_q, 0, (void **)&evt);
		if (status != OSL_EXT_SUCCESS) {
			UAMP_ERROR(("%s: Event queue receive error!\n", __FUNCTION__));
			return (0);
		}

		dlen = HCI_EVT_PREAMBLE_SIZE + ltoh_ua(&evt->plen);
		if (dlen > buf_size) {
			UAMP_ERROR(("%s: Event buf too small!\n", __FUNCTION__));
			UAMP_FREE(evt);
			return (0);
		}

		/* Copy event into user-supplied buffer. */
		memcpy(p_buf, evt, dlen);
		UAMP_FREE(evt);
		return (dlen);
	}
	else if (channel == UAMP_CH_HCI_DATA) {
		status = osl_ext_queue_receive(&uamp->pkt_q, 0, (void **)&pkt);
		if (status != OSL_EXT_SUCCESS) {
			UAMP_ERROR(("%s: Pkt queue receive error!\n", __FUNCTION__));
			return (0);
		}

		/* Skip ethernet and LLC SNAP protocol headers. */
		dlen = ltoh_ua(&pkt->dlen) + HCI_ACL_DATA_PREAMBLE_SIZE;
		if (dlen > buf_size) {
			UAMP_ERROR(("%s: Pkt buf too small! datasize=%i, bufsize=%i, pktlen=%i\n",
			            __FUNCTION__, dlen, buf_size, pkt->dlen));
			UAMP_FREE(pkt);
			return (0);
		}

		/* Copy event into user-supplied buffer. */
		memcpy(p_buf, pkt, dlen);
		UAMP_FREE(pkt);
		return (dlen);
	}

	return (0);
}


/*
 * Event receive callback that is registered with WLAN driver.
 */
static void wl_event_callback(wl_event_msg_t* event, void* event_data)
{
	if (event == NULL) {
		UAMP_ERROR(("%s: null pointer", __FUNCTION__));
		return;
	}

	if (event->event_type != WLC_E_BTA_HCI_EVENT) {
		return;
	}

	UAMP_TRACE(("%s: received WLC_E_BTA_HCI_EVENT event!\n", __FUNCTION__));

#if (BRCM_BLUETOOTH_HOST == 1)
	hcisu_handle_amp_evt_buf(event_data, event->datalen);
#else
	{
		tUAMP_EVT_DATA	evt_data;
		UAMP_STATE	*uamp = &g_uamp_mgr.uamp;
		void		*wl_evt_data;

		/* Create local copy of event data to queue. */
		wl_evt_data = UAMP_ALLOC(event->datalen);
		if (wl_evt_data == NULL) {
			UAMP_ERROR(("%s: Unable to alloc event data!\n", __FUNCTION__));
		}
		memcpy(wl_evt_data, event_data, event->datalen);


		/* Post event to queue. Stack will de-queue it with call to UAMP_Read(). */
		if (osl_ext_queue_send(&uamp->evt_q, wl_evt_data) != OSL_EXT_SUCCESS) {
			/* Unable to queue packet */
			UAMP_FREE(wl_evt_data);
			UAMP_ERROR(("%s: Unable to queue event packet!\n", __FUNCTION__));
		}


		/* Inform application stack of received event. */
		evt_data.channel = UAMP_CH_HCI_EVT;
		g_uamp_mgr.callback(0, UAMP_EVT_RX_READY, &evt_data);
	}
#endif   /* BRCM_BLUETOOTH_HOST */
}


/*
 * Packet receive callback that is registered with WLAN driver.
 */
static int wl_btamp_rx_pkt_callback(wl_drv_netif_pkt pkt, unsigned int len)
{
	UAMP_TRACE(("%s: received BTAMP packet!\n", __FUNCTION__));

#if (BRCM_BLUETOOTH_HOST == 1)
	hcisu_handle_amp_data_buf(pkt, len);
#else
	{
		tUAMP_EVT_DATA	evt_data;
		UAMP_STATE	*uamp = &g_uamp_mgr.uamp;

		/* Post packet. to queue. Stack will de-queue it with call to UAMP_Read(). */
		if (osl_ext_queue_send(&uamp->pkt_q, pkt) != OSL_EXT_SUCCESS) {
			/* Unable to queue packet. */
			UAMP_ERROR(("%s: Unable to queue event packet!\n", __FUNCTION__));
			return (-1);
		}

		/* Inform application stack of received event. */
		evt_data.channel = UAMP_CH_HCI_DATA;
		g_uamp_mgr.callback(0, UAMP_EVT_RX_READY, &evt_data);
	}
#endif   /* BRCM_BLUETOOTH_HOST */

	return (0);
}


/*
 * Get IOCTL given the parameter buffer.
 */

/*
 * Set IOCTL given the parameter buffer.
 */
static int
ioctl_set(int cmd, void *buf, int len)
{
	return wl_ioctl(cmd, buf, len, TRUE);
}


/*
 * Set named iovar given the parameter buffer.
 */
static int
iovar_set(const char *iovar, void *param, int paramlen)
{
	static char smbuf[MAX_IOVAR_LEN];

	memset(smbuf, 0, sizeof(smbuf));

	return iovar_setbuf(iovar, param, paramlen, smbuf, sizeof(smbuf));
}

/*
 * Set named iovar providing both parameter and i/o buffers.
 */
static int
iovar_setbuf(const char *iovar,
	void *param, int paramlen, void *bufptr, int buflen)
{
	int err;
	int iolen;

	iolen = iovar_mkbuf(iovar, param, paramlen, bufptr, buflen, &err);
	if (err)
		return err;

	return ioctl_set(DHD_SET_VAR, bufptr, iolen);
}


/*
 * Format an iovar buffer.
 */
static int
iovar_mkbuf(const char *name, char *data, uint datalen, char *iovar_buf, uint buflen, int *perr)
{
	int iovar_len;

	iovar_len = strlen(name) + 1;

	/* check for overflow */
	if ((iovar_len + datalen) > buflen) {
		*perr = -1;
		return 0;
	}

	/* copy data to the buffer past the end of the iovar name string */
	if (datalen > 0)
		memmove(&iovar_buf[iovar_len], data, datalen);

	/* copy the name to the beginning of the buffer */
	strcpy(iovar_buf, name);

	*perr = 0;
	return (iovar_len + datalen);
}


/*
 * Send IOCTL to WLAN driver.
 */
static int
wl_ioctl(int cmd, void *buf, int len, bool set)
{
	wl_drv_ioctl_t ioc;

	memset(&ioc, 0, sizeof(ioc));
	ioc.d.cmd = cmd;
	ioc.d.buf = buf;
	ioc.d.len = len;
	ioc.d.set = set;
	ioc.d.driver = DHD_IOCTL_MAGIC;

	return (wl_drv_ioctl(NULL, &ioc) < 0);
}


#if (BRCM_BLUETOOTH_HOST == 1)
static void *uamp_get_acl_buf(unsigned int len)
{
	return (hcisu_amp_get_acl_buf(len));
}
#endif   /* BRCM_BLUETOOTH_HOST */
