/*
 * OCL phy module internal interface (to other PHY modules).
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
 * $Id: $
 */

#ifndef _phy_ocl_h_
#define _phy_ocl_h_

#include <typedefs.h>
#include <phy_api.h>
#include <wlc_phy_shim.h>

/* forward declaration */
typedef struct phy_ocl_info phy_ocl_info_t;

#ifdef OCL
	#if defined(WL_ENAB_RUNTIME_CHECK)
		#define PHY_OCL_ENAB(physhim)		(wlapi_ocl_enab_check(physhim))
	#elif defined(OCL_DISABLED)
		#define PHY_OCL_ENAB(physhim)		(0)
	#else
		#define PHY_OCL_ENAB(physhim)		(wlapi_ocl_enab_check(physhim))
	#endif
#else
	#define PHY_OCL_ENAB(physhim)			(0)
#endif /* OCL */

#ifdef OCL
/* attach/detach */
phy_ocl_info_t* phy_ocl_attach(phy_info_t *pi);
void phy_ocl_detach(phy_ocl_info_t *ri);
#endif /* OCL */

#endif /* _phy_ocl_h_ */
