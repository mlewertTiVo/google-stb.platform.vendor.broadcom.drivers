/*
 * Wireless Ethernet (WET) interface
 *
 *   Copyright (C) 2016, Broadcom Corporation
 *   All Rights Reserved.
 *   
 *   This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 *   the contents of this file may not be disclosed to third parties, copied
 *   or duplicated in any form, in whole or in part, without the prior
 *   written permission of Broadcom Corporation.
 *
 *
 *   <<Broadcom-WL-IPTag/Proprietary:>>
 *
 *   $Id: wlc_wet.h 523117 2014-12-26 18:32:49Z $
 */


#ifndef _wlc_wet_h_
#define _wlc_wet_h_

/* forward declaration */
typedef struct wlc_wet_info wlc_wet_info_t;

/*
 * Initialize wet private context.It returns a pointer to the
 * wet private context if succeeded. Otherwise it returns NULL.
 */
extern wlc_wet_info_t *wlc_wet_attach(wlc_info_t *wlc);

/* Cleanup wet private context */
extern void wlc_wet_detach(wlc_wet_info_t *weth);

/* Process frames in transmit direction */
extern int wlc_wet_send_proc(wlc_wet_info_t *weth, void *sdu, void **new);

/* Process frames in receive direction */
extern int wlc_wet_recv_proc(wlc_wet_info_t *weth, void *sdu);


#endif	/* _wlc_wet_h_ */
