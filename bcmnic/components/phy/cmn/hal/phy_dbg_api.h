/*
 * Debug module public interface (to MAC driver).
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
 * $Id: phy_dbg_api.h 620395 2016-02-23 01:15:14Z vyass $
 */

#ifndef _phy_dbg_api_h_
#define _phy_dbg_api_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_api.h>

#define TAG_TYPE_PHY               0xf0
#define TAG_TYPE_RAD               0xf1

/*
 * Invoke dump function for module 'name'. Return BCME_XXXX.
 */
int phy_dbg_dump(phy_info_t *pi, const char *name, struct bcmstrbuf *b);

/*
 * Invoke dump clear function for module 'name'. Return BCME_XXXX.
 */
int phy_dbg_dump_clr(phy_info_t *pi, const char *name);

/*
 * List dump names
 */
int phy_dbg_dump_list(phy_info_t *pi, struct bcmstrbuf *b);

/*
 * Dump dump registry
 */
int phy_dbg_dump_dump(phy_info_t *pi, struct bcmstrbuf *b);

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
/*
 * Dump phy txerr
 */
void phy_dbg_txerr_dump(phy_info_t *pi, uint16 err);
#endif

#if defined(DNG_DBGDUMP)
/*
 * Print phy debug registers
 */
extern void wlc_phy_print_phydbg_regs(wlc_phy_t *ppi);
#endif /* DNG_DBGDUMP */

#ifdef WL_MACDBG
/*
 * Enable GPIO Out
 */
void phy_dbg_gpio_out_enab(phy_info_t *pi, bool enab);
#endif

/*
 * phy and radio register binary dump size
 */
int phy_dbg_dump_getlistandsize(wlc_phy_t *ppi, uint8 type);

/*
 * phy and radio register binary dump
 */
int phy_dbg_dump_binary(wlc_phy_t *ppi, uint8 type, uchar *p, int * buf_len);

#endif /* _phy_dbg_api_h_ */
