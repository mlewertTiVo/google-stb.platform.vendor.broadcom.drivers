/*
 * Required functions exported by the wlc_diag.c
 * to common (os-independent) driver code.
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_diag.h 523117 2014-12-26 18:32:49Z $
 */

#ifndef _wlc_diag_h_
#define _wlc_diag_h_


extern int wlc_diag(wlc_info_t *wlc, uint32 diagtype, uint32 *result);

#endif	/* _wlc_diag_h_ */
