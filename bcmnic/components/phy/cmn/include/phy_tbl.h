/*
 * PHYTableInit module internal interface (to other PHY modules).
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
 * $Id: phy_tbl.h 657351 2016-08-31 23:00:22Z $
 */

#ifndef _phy_tbl_h_
#define _phy_tbl_h_

#include <phy_api.h>

/* forward declaration */
typedef struct phy_tbl_info phy_tbl_info_t;

/* attach/detach */
phy_tbl_info_t *phy_tbl_attach(phy_info_t *pi);
void phy_tbl_detach(phy_tbl_info_t *ii);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
int phy_tbl_dump_txv0(void *ctx, struct bcmstrbuf *b);
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */

/* phy table structure used for MIMOPHY */
struct phytbl_info {
	const void   *tbl_ptr;
	uint32  tbl_len;
	uint32  tbl_id;
	uint32  tbl_offset;
	uint32  tbl_width;
};

#endif /* _phy_tbl_h_ */
