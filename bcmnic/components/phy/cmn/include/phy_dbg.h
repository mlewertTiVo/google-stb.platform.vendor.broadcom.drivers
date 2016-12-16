/*
 * PHY modules debug utilities
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
 * $Id: phy_dbg.h 659514 2016-09-14 20:19:00Z $
 */

#ifndef _phy_dbg_h_
#define _phy_dbg_h_

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmutils.h>
#include <wlc_dump_reg.h>
#include <phy_api.h>
#include <phy_dbg_api.h>

#include <devctrl_if/phyioctl_defs.h>

extern uint32 phyhal_msg_level;

#if defined(BCMDBG) && !defined(BCMDONGLEHOST) && !defined(BCMDBG_EXCLUDE_HW_TIMESTAMP)
char *wlc_dbg_get_hw_timestamp(void);
#define PHY_TIMESTAMP()	do {						\
		if (phyhal_msg_level & PHYHAL_TIMESTAMP) {		\
			printf("%s", wlc_dbg_get_hw_timestamp());	\
		}							\
	} while (0)
#else
#define PHY_TIMESTAMP()
#endif

#define phy_log(wlc, str, p1, p2)       do {} while (0)


#define PHY_PRINT(args) do { PHY_TIMESTAMP(); printf args; } while (0)

#if defined(BCMDBG_ERR) && defined(ERR_USE_EVENT_LOG)
#define	PHY_ERROR(args)		do { \
				if (phyhal_msg_level & PHYHAL_ERROR) { \
				EVENT_LOG_COMPACT_CAST_PAREN_ARGS(EVENT_LOG_TAG_PHY_ERROR, args); }\
				} while (0)
#elif defined(BCMDBG_ERR) || defined(BCMDBG) || defined(PHYDBG)
#define	PHY_ERROR(args)	do {if (phyhal_msg_level & PHYHAL_ERROR) PHY_PRINT(args);} while (0)
#else
#define	PHY_ERROR(args)
#endif /* defined(BCMDBG_ERR) && defined(ERR_USE_EVENT_LOG) */

#if defined(BCMDBG) || defined(PHYDBG)
#define	PHY_TRACE(args)	do {if (phyhal_msg_level & PHYHAL_TRACE) PHY_PRINT(args);} while (0)
#define	PHY_INFORM(args) do {if (phyhal_msg_level & PHYHAL_INFORM) PHY_PRINT(args);} while (0)
#define	PHY_TMP(args)	do {if (phyhal_msg_level & PHYHAL_TMP) PHY_PRINT(args);} while (0)
#define	PHY_TXPWR(args)	do {if (phyhal_msg_level & PHYHAL_TXPWR) PHY_PRINT(args);} while (0)
#define	PHY_CAL(args)	do {if (phyhal_msg_level & PHYHAL_CAL) PHY_PRINT(args);} while (0)
#define	PHY_ACI(args)	do {if (phyhal_msg_level & PHYHAL_ACI) PHY_PRINT(args);} while (0)
#define	PHY_RADAR(args)	do {if (phyhal_msg_level & PHYHAL_RADAR) PHY_PRINT(args);} while (0)
#define PHY_THERMAL(args) do {if (phyhal_msg_level & PHYHAL_THERMAL) PHY_PRINT(args);} while (0)
#define PHY_PAPD(args)	do {if (phyhal_msg_level & PHYHAL_PAPD) PHY_PRINT(args);} while (0)
#define PHY_FCBS(args)	do {if (phyhal_msg_level & PHYHAL_FCBS) PHY_PRINT(args);} while (0)
#define PHY_RXIQ(args)	do {if (phyhal_msg_level & PHYHAL_RXIQ) PHY_PRINT(args);} while (0)
#define PHY_WD(args)	do {if (phyhal_msg_level & PHYHAL_WD) PHY_PRINT(args);} while (0)
#define PHY_CHANLOG(w, s, i, j)  \
	do {if (phyhal_msg_level & PHYHAL_CHANLOG) phy_log(w, s, i, j);} while (0)

#define	PHY_NONE(args)	do {} while (0)
#else
#define	PHY_TRACE(args)
#define	PHY_INFORM(args)
#define	PHY_TMP(args)
#define	PHY_TXPWR(args)
#define	PHY_CAL(args)
#define	PHY_ACI(args)
#define	PHY_RADAR(args)
#define PHY_THERMAL(args)
#define PHY_PAPD(args)
#define PHY_FCBS(args)
#define PHY_RXIQ(args)
#define PHY_WD(args)
#define PHY_CHANLOG(w, s, i, j)
#define	PHY_NONE(args)
#endif /* BCMDBG || PHYDBG */

#define PHY_INFORM_ON()		(phyhal_msg_level & PHYHAL_INFORM)
#define PHY_THERMAL_ON()	(phyhal_msg_level & PHYHAL_THERMAL)
#define PHY_CAL_ON()		(phyhal_msg_level & PHYHAL_CAL)

#define PHY_FATAL_ERROR_MESG(args)	do {PHY_PRINT(args);} while (0)
#define PHY_FATAL_ERROR(pi, reason_code) phy_fatal_error(pi, reason_code)
void phy_fatal_error(phy_info_t *pi, phy_crash_reason_t reason_code);

/* PHY and RADIO register binary dump data types */
#define PHYRADDBG1_TYPE       1

/* PHYRADDBG1_TYPE structure definition */
/* Note: bmp_cnt has either bitmap or count, if the MSB (bit 31) is set, then
*   bmp_cnt[30:0] has count, i.e, number of valid registers whose values are
*   contigous from the start address. If MSB is zero, then the value
*   should be considered as a bitmap of 31 discreet addresses from the base addr.
*   The data type for bmp_cnt is chosen as an array of uint8 to avoid
*   padding. A uint32 type will lead to 2bytes of padding which will consume
*   ~20% more space per table
*/
typedef struct _phyradregs_bmp_list {
	uint16 addr;		/* start address */
	uint8 bmp_cnt[4];	/* bit[31]=1, bit[30:0] is count else it is a bitmap */
} phyradregs_list_t;

typedef struct phy_dbg_info phy_dbg_info_t;

/* attach/detach */
phy_dbg_info_t *phy_dbg_attach(phy_info_t *pi);
void phy_dbg_detach(phy_dbg_info_t *di);

/* add a dump and/or dump clear callback(s) */
typedef wlc_dump_reg_dump_fn_t phy_dump_dump_fn_t;
typedef wlc_dump_reg_clr_fn_t phy_dump_clr_fn_t;
int phy_dbg_add_dump_fns(phy_info_t *pi, const char *name,
	phy_dump_dump_fn_t fn, phy_dump_clr_fn_t clr, void *ctx);
/* keep the old style API for all existing callers who don't have dump clear fn */
#define phy_dbg_add_dump_fn(pi, name, fn, ctx) \
	phy_dbg_add_dump_fns(pi, name, fn, NULL, ctx)

#if defined(BCMDBG)
void phy_dbg_test_evm_init(phy_info_t *pi);
uint16 phy_dbg_test_evm_reg(uint rate);
int phy_dbg_test_evm(phy_info_t *pi, int channel, uint rate, int txpwr);
int phy_dbg_test_carrier_suppress(phy_info_t *pi, int channel);
#endif

#endif /* _phy_dbg_h_ */
