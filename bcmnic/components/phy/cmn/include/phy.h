/*
 * PHY Core module internal interface.
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
 * $Id: phy.h 650795 2016-07-22 13:03:38Z mvermeid $
 */

#ifndef _phy_h_
#define _phy_h_

#if defined(WL_DSI)
	#if defined(WL_ENAB_RUNTIME_CHECK) || !defined(DONGLEBUILD)
		#define DSI_ENAB(pi)		((pi)->ff->_dsi)
	#elif defined(WL_DSI_DISABLED)
		#define DSI_ENAB(pi)		(0)
	#else
		#define DSI_ENAB(pi)		(1)
	#endif
#else
	#define DSI_ENAB(pi)			(0)
#endif /* WL_DSI */

#if defined(WL_WBPAPD)
	#if defined(WL_ENAB_RUNTIME_CHECK) || !defined(DONGLEBUILD)
		#define WBPAPD_ENAB(pi)		((pi)->ff->_wbpapd)
	#elif defined(WL_WBPAPD_DISABLED)
		#define WBPAPD_ENAB(pi)		(0)
	#else
		#define WBPAPD_ENAB(pi)		(1)
	#endif
#else
	#define WBPAPD_ENAB(pi)			(0)
#endif /* WL_WBPAPD */

/* ET Check */
#if defined(WL_ETMODE)
	#if defined(WL_ENAB_RUNTIME_CHECK) || !defined(DONGLEBUILD)
		#define ET_ENAB(pi)		((pi)->ff->_et)
	#elif defined(WL_ET_DISABLED)
		#define ET_ENAB(pi)		(0)
	#else
		#define ET_ENAB(pi)		((pi)->ff->_et)
	#endif
#else
	#define ET_ENAB(pi)			(0)
#endif /* WL_WBPAPD */

/* Ultra-Low Bandwidth (ULB) Mode support */
#ifdef WL11ULB
	#if defined(WL_ENAB_RUNTIME_CHECK)
		#define PHY_ULB_ENAB(pi)		((pi)->ff->_ulb)
	#elif defined(WL11ULB_DISABLED)
		#define PHY_ULB_ENAB(pi)		(0)
	#else
		#define PHY_ULB_ENAB(pi)		(1)
	#endif
#else
	#define PHY_ULB_ENAB(pi)			(0)
#endif /* WL11ULB */

/* PHY Dual band support */
#if defined(WL_ENAB_RUNTIME_CHECK)
	#define PHY_BAND5G_ENAB(pi)   ((pi)->ff->_dband)
#elif defined(DBAND)
	#define PHY_BAND5G_ENAB(pi)   (1)
#else
	#define PHY_BAND5G_ENAB(pi)   (0)
#endif

#endif /* _phy_h_ */
