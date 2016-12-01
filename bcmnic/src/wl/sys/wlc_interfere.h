/*
 * 802.11 interference stats module header file
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
 * $Id: wlc_interfere.h 523117 2014-12-26 18:32:49Z $
*/

#ifndef _wlc_interfere_h_
#define _wlc_interfere_h_
extern itfr_info_t *wlc_itfr_attach(wlc_info_t *wlc);
extern void wlc_itfr_detach(itfr_info_t *itfr);
#endif /* _wlc_interfere_h_ */
