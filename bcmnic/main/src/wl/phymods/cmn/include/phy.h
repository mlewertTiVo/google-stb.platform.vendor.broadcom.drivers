/*
 * PHY Core module internal interface.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#ifndef _phy_h_
#define _phy_h_

/* PHY Dual band support */
#if defined(WL_ENAB_RUNTIME_CHECK)
	#define PHY_BAND5G_ENAB(pi)   ((pi)->ff->_dband)
#elif defined(DBAND)
	#define PHY_BAND5G_ENAB(pi)   (1)
#else
	#define PHY_BAND5G_ENAB(pi)   (0)
#endif

#endif /* _phy_h_ */
