/*
 * ASDB related header file.
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_asdb.h 549348 2015-04-15 14:54:30Z $
 */

/**
 * @file
 * @brief
 * Twiki: http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/AdaptiveSimultaneousDualBand
 */
#ifndef _wlc_asdb_h_
#define _wlc_asdb_h_
#ifdef WL_ASDB
extern void wlc_asdb_detach(wlc_asdb_t *asdb_info);
extern wlc_asdb_t * wlc_asdb_attach(wlc_info_t *wlc);
extern bool wlc_asdb_active(wlc_asdb_t *asdb);
extern bool wlc_asdb_modesw_required(wlc_asdb_t* asdb);
#endif /* WL_ASDB */
#endif /* _wlc_asdb_h_ */
