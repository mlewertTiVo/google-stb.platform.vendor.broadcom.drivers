/*
 * IE management module Vendor Specific IE utilities
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
 * $Id: wlc_ie_mgmt_vs.h 665073 2016-10-14 20:33:29Z $
 */

#ifndef _wlc_ie_mgmt_vs_h_
#define _wlc_ie_mgmt_vs_h_

#include <typedefs.h>
#include <wlc_types.h>
#include <wlc_ie_mgmt_lib.h>

/*
 * Special id (WLC_IEM_VS_ID_MAX to WLC_IEM_ID_MAX).
 */
#define WLC_IEM_VS_ID_MAX	(WLC_IEM_ID_MAX - 5)

/*
 * Priority/id (0 to WLC_IEM_VS_ID_MAX - 1).
 *
 * - It is used as a Priority when registering a Vendor Specific IE's
 *   calc_len/build callback pair. The IE management's IE calc_len/build
 *   function invokes the callbacks in their priorities' ascending order.
 *
 * - It is used as an ID when registering a Vendor Specific IE's parse
 *   callback. The IE management's IE parse function queries the user
 *   supplied classifier callback (which may use the OUI plus some other
 *   information in the IE being parsed to decide the ID), and invokes the
 *   callback.
 */
/* !Please leave some holes in between priorities when possible! */
#define WLC_IEM_VS_IE_PRIO_VNDR		64
#define WLC_IEM_VS_IE_PRIO_BRCM_HT	80
#define WLC_IEM_VS_IE_PRIO_BRCM_EXT_CH	88
#define WLC_IEM_VS_IE_PRIO_BRCM_VHT	104
#define WLC_IEM_VS_IE_PRIO_BRCM_TPC	136
#define WLC_IEM_VS_IE_PRIO_BRCM_RMC	140
#define WLC_IEM_VS_IE_PRIO_BRCM		152
#define WLC_IEM_VS_IE_PRIO_BRCM_PSTA	156
#define WLC_IEM_VS_IE_PRIO_WPA		160
#define WLC_IEM_VS_IE_PRIO_WPS		164
#define WLC_IEM_VS_IE_PRIO_WME		168
#define WLC_IEM_VS_IE_PRIO_WME_TS	176
#ifdef WLFBT
#define WLC_IEM_VS_IE_PRIO_CCX_EXT_CAP	179
#endif
#define WLC_IEM_VS_IE_PRIO_HS20		190
#define WLC_IEM_VS_IE_PRIO_P2P		192
#define WLC_IEM_VS_IE_PRIO_OSEN		196
#define WLC_IEM_VS_IE_PRIO_NAN		197
#define WLC_IEM_VS_IE_PRIO_BRCM_BTCX	200
#define WLC_IEM_VS_IE_PRIO_ULB		204
#define WLC_IEM_VS_IE_PRIO_MBO      210
#define WLC_IEM_VS_IE_PRIO_MBO_OCE  210

/*
 * Map Vendor Specific IE to an id
 */
extern wlc_iem_tag_t wlc_iem_vs_get_id(uint8 *ie);

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
extern int wlc_iem_vs_dump(void *ctx, struct bcmstrbuf *b);
#endif

#endif /* _wlc_ie_mgmt_vs_h_ */
