/*
* Tunnel BT Traffic Over Wlan public header file
*
* Copyright (C) 2016, Broadcom Corporation
* All Rights Reserved.
* 
* This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
* the contents of this file may not be disclosed to third parties, copied
* or duplicated in any form, in whole or in part, without the prior
* written permission of Broadcom Corporation.
*
* $Id: wlc_tbow.h 467328 2014-04-03 01:23:40Z $
*
*/
#ifndef _wlc_tbow_h_
#define _wlc_tbow_h_

extern tbow_info_t *wlc_tbow_attach(wlc_info_t* wlc);
extern void wlc_tbow_detach(tbow_info_t *tbow_info);

bool tbow_recv_wlandata(tbow_info_t *tbow_info, void *sdu);

uint32 tbow_ho_connection_done(tbow_info_t *tbow_info, wlc_bsscfg_t *bsscfg, wl_wsec_key_t *key);
#endif /* _wlc_tbow_h_ */
