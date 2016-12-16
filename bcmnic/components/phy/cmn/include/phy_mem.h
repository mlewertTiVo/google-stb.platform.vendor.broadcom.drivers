/*
 * Memory manipulation interface
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
 * $Id: phy_mem.h 643020 2016-06-12 01:13:23Z $
 */

#ifndef _phy_mem_h_
#define _phy_mem_h_

#include <typedefs.h>
#include <osl.h>

/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
/* TODO: all these are going away... > */

#define phy_malloc(pi, sz) MALLOCZ((pi)->sh->osh, sz)
#define phy_mfree(pi, mem, sz) MFREE((pi)->sh->osh, mem, sz)
void * phy_malloc_fatal(phy_info_t *pi, uint sz);

#endif /* _phy_mem_h_ */
