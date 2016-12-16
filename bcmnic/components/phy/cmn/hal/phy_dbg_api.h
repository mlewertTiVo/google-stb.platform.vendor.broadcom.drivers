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
 * $Id: phy_dbg_api.h 659514 2016-09-14 20:19:00Z $
 */

#ifndef _phy_dbg_api_h_
#define _phy_dbg_api_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_api.h>

#define TAG_TYPE_PHY               0xf0
#define TAG_TYPE_RAD               0xf1

/* Error Types */
typedef enum {
	PHY_RC_NONE						= 0,
	PHY_RC_TXPOWER_LIMITS			= 1,
	PHY_RC_TEMPSENSE_LIMITS			= 2,
	PHY_RC_VCOCAL_FAILED			= 3,
	PHY_RC_PLL_NOTLOCKED			= 4,
	PHY_RC_DESENSE_LIMITS			= 5,
	PHY_RC_BASEINDEX_LIMITS			= 6,
	PHY_RC_TXCHAIN_INVALID			= 7,
	PHY_RC_PMUCAL_FAILED			= 8,
	PHY_RC_RCAL_INVALID				= 9,
	PHY_RC_FCBS_CHSW_FAILED			= 10,
	PHY_RC_IQEST_FAILED				= 11,
	PHY_RC_RESET2RX_FAILED			= 12,
	PHY_RC_PKTPROC_RESET_FAILED		= 13,
	PHY_RC_RFSEQ_STATUS_INVALID		= 14,
	PHY_RC_TXIQLO_CAL_FAILED		= 15,
	PHY_RC_PAPD_CAL_FAILED			= 16,
	PHY_RC_NOISE_CAL_FAILED			= 17,
	PHY_RC_RFSEQ_STATUS_OCL_INVALID	= 18,
	PHY_RC_RX2TX_FAILED				= 19,
	PHY_RC_AFE_CAL_FAILED			= 20,
	PHY_RC_NOMEM					= 21,
	PHY_RC_SAMPLEPLAY_LIMIT			= 22,
	PHY_RC_RCCAL_INVALID			= 23,
	PHY_RC_IDLETSSI_INVALID			= 24,
	PHY_RC_LAST			/* This must be the last entry */
} phy_crash_reason_t;

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
