/*
 * ACPHY Core module internal interface (to other PHY modules).
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
 * $Id: phy_ac_info.h 650803 2016-07-22 13:50:37Z luka $
 */

#ifndef _phy_ac_info_h_
#define _phy_ac_info_h_

#include <bcmdevs.h>

#include "phy_ac_ana.h"
#include "phy_ac_antdiv.h"
#include "phy_ac_btcx.h"
#include "phy_ac_cache.h"
#include "phy_ac_calmgr.h"
#include "phy_ac_chanmgr.h"
#include "phy_ac_et.h"
#include "phy_ac_fcbs.h"
#include "phy_ac_lpc.h"
#include "phy_ac_misc.h"
#include "phy_ac_noise.h"
#include "phy_ac_papdcal.h"
#include "phy_ac_radar.h"
#include "phy_ac_radio.h"
#include "phy_ac_rxgcrs.h"
#include "phy_ac_rssi.h"
#include "phy_ac_rxiqcal.h"
#include "phy_ac_rxspur.h"
#include "phy_ac_samp.h"
#include "phy_ac_tbl.h"
#include "phy_ac_temp.h"
#include "phy_ac_tpc.h"
#include <phy_tpc_api.h>
#include "phy_ac_txpwrcap.h"
#include "phy_ac_tssical.h"
#include "phy_ac_txiqlocal.h"
#include "phy_ac_vcocal.h"
#include "phy_ac_dsi.h"
#include "phy_ac_mu.h"
#include "phy_ac_tblid.h"
#include "phy_ac_dbg.h"
#include "phy_ac_dccal.h"
#include "phy_ac_tof.h"
#include "phy_ac_hirssi.h"
#include <phy_ac_chanmgr.h>
#include "phy_ac_nap.h"
#include "phy_ac_ocl.h"
#include "phy_ac_txpwrcap.h"
#include "phy_ac_hecap.h"


/*
 * ACPHY Core REV info and mapping to (Major/Minor)
 * http://confluence.broadcom.com/display/WLAN/ACPHY+Major+and+Minor+PHY+Revision+Mapping
 *
 * revid  | chip used | (major, minor) revision
 *  0     : 4360A0/A2 (3x3) | (0, 0)
 *  1     : 4360B0    (3x3) | (0, 1)
 *  2     : 4335A0    (1x1 + low-power improvment, no STBC/antdiv) | (1, 0)
 *  3     : 4350A0/B0 (2x2 + low-power 2x2) | (2, 0)
 *  4     : 4345TC    (1x1 + tiny radio)  | (3, 0)
 *  5     : 4335B0    (1x1, almost the same as rev2, txbf NDP stuck fix) | (1, 1)
 *  6     : 4335C0    (1x1, rev5 + bug fixes + stbc) | (1, 2)
 *  7     : 4345A0    (1x1 + tiny radio, rev4 improvment) | (3, 1)
 *  8     : 4350C0    (2x2 + low-power 2x2) | (2, 1)
 *  9     : 43602A0   (3x3 + MDP for lower power, proprietary 256 QAM for 11n) | (5, 0)
 *  10    : 4349TC2A0
 *  11    : 43457A0   (Based on 4345A0 plus support A4WP (Wireless Charging)
 *  12    : 4349A0    (1x1, Reduced Area Tiny-2 Radio plus channel bonding 80+80 supported)
 *  13    : 4345B0/43457B0	(1x1 + tiny radio, phyrev 7 improvements) | (3, 3)
 *  14    : 4350C2    (2x2 + low power) | (2, 4)
 *  15    : 4354A1/43569A0    (2x2 + low-power 2x2) | (2, 3)
 *  16    : 43909A0   (1x1 + tiny radio) | (3, 4)
 *  18    : 43602A1   (3x3 + MDP for lower power, proprietary 256 QAM for 11n) | (5, 1)
 *  20    : 4345C0    (1x1 + tiny radio) | (3, 4)
 *  24    : 4349B0    (1x1, Reduced Area Tiny-2 Radio plus channel bonding 80+80 supported)
 *  25    : 4347TC2
 *  26    : 4364A0    (3x3 of 4364, derived from 43602)
 *  27    : 4364A0    (1x1 of 4364, derived from 4345C0)
 *  32    : 4365A0    (4x4) | (32, 0)
 *  37    : 7271A0    (4x4 28nm) | (37,0)
 *  40    : 4347A0    (2x2 + 20MHz only 2x2, 28nm) | (40, 0)
 *
 */

/* Major Revs */
#define USE_HW_MINORVERSION(phy_rev) (ACREV_IS(phy_rev, 24) || ACREV_GE(phy_rev, 32))
#define ACMAJORREV_25_40(phy_rev) \
	(ACREV_IS(phy_rev, 25) || ACREV_IS(phy_rev, 40))

#define ACMAJORREV_40(phy_rev) \
	(ACREV_IS(phy_rev, 40))

#define ACMAJORREV_37(phy_rev) \
	(ACREV_IS(phy_rev, 37))

#define ACMAJORREV_36(phy_rev) \
	(ACREV_IS(phy_rev, 36))

#define ACMAJORREV_33(phy_rev) \
	(ACREV_IS(phy_rev, 33))

#define ACMAJORREV_32(phy_rev) \
	(ACREV_IS(phy_rev, 32))

#define ACMAJORREV_GE32(phy_rev) \
	(ACREV_GE(phy_rev, 32))

#define ACMAJORREV_GE33(phy_rev) \
	(ACREV_GE(phy_rev, 33))

#define ACMAJORREV_25(phy_rev) \
	(ACREV_IS(phy_rev, 25))

#define ACMAJORREV_5(phy_rev) \
	(ACREV_IS(phy_rev, 9) || ACREV_IS(phy_rev, 18) || ACREV_IS(phy_rev, 26))

#define ACMAJORREV_4(phy_rev) \
	(ACREV_IS(phy_rev, 12) || ACREV_IS(phy_rev, 24))

#define ACMAJORREV_3(phy_rev) \
	(ACREV_IS(phy_rev, 4) || ACREV_IS(phy_rev, 7) || ACREV_IS(phy_rev, 10) || \
	 ACREV_IS(phy_rev, 11) || ACREV_IS(phy_rev, 13) || ACREV_IS(phy_rev, 16) || \
	 ACREV_IS(phy_rev, 20) || ACREV_IS(phy_rev, 27))

#define ACMAJORREV_2(phy_rev) \
	(ACREV_IS(phy_rev, 3) || ACREV_IS(phy_rev, 8) || ACREV_IS(phy_rev, 14) || \
		ACREV_IS(phy_rev, 15) || ACREV_IS(phy_rev, 17))

#define ACMAJORREV_1(phy_rev) \
	(ACREV_IS(phy_rev, 2) || ACREV_IS(phy_rev, 5) || ACREV_IS(phy_rev, 6))

#define ACMAJORREV_0(phy_rev) \
	(ACREV_IS(phy_rev, 0) || ACREV_IS(phy_rev, 1))

/* Minor Revs */
#ifdef BCMPHYACMINORREV
#define HW_ACMINORREV(pi) (BCMPHYACMINORREV)
#else /* BCMPHYACMINORREV */
/* Read MinorVersion HW reg for chipc with phyrev >= 12 */
#define HW_ACMINORREV(pi) ((pi)->u.pi_acphy->phy_minor_rev)
#endif /* BCMPHYACMINORREV */

#define HW_MINREV_IS(pi, val) ((HW_ACMINORREV(pi)) == (val))

#define SW_ACMINORREV_0(phy_rev) \
	(ACREV_IS(phy_rev, 0) || ACREV_IS(phy_rev, 2) || ACREV_IS(phy_rev, 3) || \
	 ACREV_IS(phy_rev, 4) || ACREV_IS(phy_rev, 9) || ACREV_IS(phy_rev, 10) || \
	 ACREV_IS(phy_rev, 12) || ACREV_IS(phy_rev, 36) || ACREV_IS(phy_rev, 37))

#define SW_ACMINORREV_1(phy_rev) \
	(ACREV_IS(phy_rev, 1) || ACREV_IS(phy_rev, 5) || ACREV_IS(phy_rev, 7) || \
	 ACREV_IS(phy_rev, 8) || ACREV_IS(phy_rev, 18))

#define SW_ACMINORREV_2(phy_rev) \
	(ACREV_IS(phy_rev, 6) || ACREV_IS(phy_rev, 11) || ACREV_IS(phy_rev, 24) || \
	 ACREV_IS(phy_rev, 26))

#define SW_ACMINORREV_3(phy_rev) \
	(ACREV_IS(phy_rev, 13) || ACREV_IS(phy_rev, 15))

#define SW_ACMINORREV_4(phy_rev) \
	(ACREV_IS(phy_rev, 14) || ACREV_IS(phy_rev, 16))

#define SW_ACMINORREV_5(phy_rev) \
	(ACREV_IS(phy_rev, 17) || ACREV_IS(phy_rev, 20))

#define SW_ACMINORREV_6(phy_rev) \
	(ACREV_IS(phy_rev, 27))

/* To get the MinorVersion for chips that do not have this as hw register */
#define GET_SW_ACMINORREV(phy_rev) \
	(SW_ACMINORREV_0(phy_rev) ? 0 : (SW_ACMINORREV_1(phy_rev) ? 1 : \
	(SW_ACMINORREV_2(phy_rev) ? 2 : (SW_ACMINORREV_3(phy_rev) ? 3 : \
	(SW_ACMINORREV_4(phy_rev) ? 4 : 5)))))

#define ACMINORREV_0(pi) \
	(USE_HW_MINORVERSION((pi)->pubpi->phy_rev) ? \
	HW_MINREV_IS(pi, 0) : SW_ACMINORREV_0((pi)->pubpi->phy_rev))

#define ACMINORREV_1(pi) \
	(USE_HW_MINORVERSION((pi)->pubpi->phy_rev) ? \
	HW_MINREV_IS(pi, 1) : SW_ACMINORREV_1((pi)->pubpi->phy_rev))

#define ACMINORREV_2(pi) \
	(USE_HW_MINORVERSION((pi)->pubpi->phy_rev) ? \
	HW_MINREV_IS(pi, 2) : SW_ACMINORREV_2((pi)->pubpi->phy_rev))

#define ACMINORREV_3(pi) \
	(USE_HW_MINORVERSION((pi)->pubpi->phy_rev) ? \
	HW_MINREV_IS(pi, 3) : SW_ACMINORREV_3((pi)->pubpi->phy_rev))

#define ACMINORREV_4(pi) \
	(USE_HW_MINORVERSION((pi)->pubpi->phy_rev) ? \
	HW_MINREV_IS(pi, 4) : SW_ACMINORREV_4((pi)->pubpi->phy_rev))

#define ACMINORREV_5(pi) \
	(USE_HW_MINORVERSION((pi)->pubpi->phy_rev) ? \
	HW_MINREV_IS(pi, 5) : SW_ACMINORREV_5((pi)->pubpi->phy_rev))

#define ACMINORREV_6(pi) \
	(USE_HW_MINORVERSION((pi)->pubpi->phy_rev) ? \
	HW_MINREV_IS(pi, 6) : SW_ACMINORREV_6((pi)->pubpi->phy_rev))
#if defined(ACCONF) && ACCONF
#endif	/*	ACCONF */

#define ACMINORREV_GE(pi, val) \
	(USE_HW_MINORVERSION((pi)->pubpi->phy_rev) ? \
	HW_ACMINREV_GE(pi, val) : SW_ACMINREV_GE(((pi)->pubpi->phy_rev), val))

#define ACMINORREV_GT(pi, val) \
	(USE_HW_MINORVERSION((pi)->pubpi->phy_rev) ? \
	HW_ACMINREV_GT(pi, val) : SW_ACMINREV_GT(((pi)->pubpi->phy_rev), val))

#define ACMINORREV_LE(pi, val) \
	(USE_HW_MINORVERSION((pi)->pubpi->phy_rev) ? \
	HW_ACMINREV_LE(pi, val) : SW_ACMINREV_LE(((pi)->pubpi->phy_rev), val))

#define ACMINORREV_LT(pi, val) \
	(USE_HW_MINORVERSION((pi)->pubpi->phy_rev) ? \
	HW_ACMINREV_LT(pi, val) : SW_ACMINREV_LT(((pi)->pubpi->phy_rev), val))

#define HW_ACMINREV_GE(pi, val) ((HW_ACMINORREV(pi)) >= (val))
#define HW_ACMINREV_GT(pi, val) ((HW_ACMINORREV(pi))  > (val))
#define HW_ACMINREV_LE(pi, val) ((HW_ACMINORREV(pi)) <= (val))
#define HW_ACMINREV_LT(pi, val) ((HW_ACMINORREV(pi))  < (val))

#define SW_ACMINREV_GE(phyrev, val) ((GET_SW_ACMINORREV(phyrev)) >= (val))
#define SW_ACMINREV_GT(phyrev, val) ((GET_SW_ACMINORREV(phyrev))  > (val))
#define SW_ACMINREV_LE(phyrev, val) ((GET_SW_ACMINORREV(phyrev)) <= (val))
#define SW_ACMINREV_LT(phyrev, val) ((GET_SW_ACMINORREV(phyrev))  < (val))

#define RSDB_FAMILY(pi)	ACMAJORREV_4((pi)->pubpi->phy_rev)

#ifndef DONGLEBUILD
	#define ROUTER_4349(pi) \
		(ACMAJORREV_4((pi)->pubpi->phy_rev) && (ACMINORREV_1(pi) || \
		ACMINORREV_3(pi)))
	#define IS_ACR(pi) ((((pi)->sh->boardrev >> 12) & 0xF) == 2)
#else
	#define ROUTER_4349(pi) (0)
	#define IS_ACR(pi) (0)
#endif /* !DONGLEBUILD */

#define IS_4364_1x1(pi) (ACMAJORREV_3((pi)->pubpi->phy_rev) && ACMINORREV_6(pi))
#define IS_4364_3x3(pi) (ACMAJORREV_5((pi)->pubpi->phy_rev) && ACMINORREV_2(pi))
#define IS_4364(pi) ((ACMAJORREV_5((pi)->pubpi->phy_rev) && ACMINORREV_2(pi)) || \
		(ACMAJORREV_3((pi)->pubpi->phy_rev) && ACMINORREV_6(pi)))
#define IS_4350(pi) \
	ACMAJORREV_2((pi)->pubpi->phy_rev) && ((ACMINORREV_0(pi)) || \
		(ACMINORREV_1(pi)) || (ACMINORREV_4(pi)))

#define PHY_AS_80P80(pi, chanspec) \
	(ACMAJORREV_33(pi->pubpi->phy_rev) && \
	(CHSPEC_IS160(chanspec) || CHSPEC_IS8080(chanspec)))

#define PHY_SUPPORT_SCANCORE(pi) \
	(ACMAJORREV_32(pi->pubpi->phy_rev) || \
	ACMAJORREV_33(pi->pubpi->phy_rev))

#define PHY_SUPPORT_BW80P80(pi) \
	(ACMAJORREV_32(pi->pubpi->phy_rev) || \
	ACMAJORREV_33(pi->pubpi->phy_rev))

/* ********************************************************************* */
/* The following definitions shared between calibration, cache modules				*/
/* ********************************************************************* */

#define PHY_REG_BANK_CORE1_OFFSET	0x200

/* ACPHY PHY REV mapping to MAJOR/MINOR Revs */
/* Major Revs */

/* Macro's ACREV0 and ACREV3 indicate an old(er) vs a new(er) 2069 radio */
#if ACCONF || ACCONF2
#ifdef BCMCHIPID
#define  ACREV0 (BCMCHIPID == BCM4360_CHIP_ID || BCMCHIPID == BCM4352_CHIP_ID || \
		 BCMCHIPID == BCM43460_CHIP_ID || BCMCHIPID == BCM43526_CHIP_ID || \
		 BCM43602_CHIP(BCMCHIPID) || BCMCHIPID == BCM4364_CHIP_ID)
#define  ACREV3 BCM4350_CHIP(BCMCHIPID)
#define  ACREV32 BCM4365_CHIP(BCMCHIPID)
#define	 ACREV0_SUB (BCMCHIPID == BCM4364_CHIP_ID)
#else
#define ACREV0 (CHIPID(pi->sh->chip) == BCM4360_CHIP_ID || \
		CHIPID(pi->sh->chip) == BCM4352_CHIP_ID || \
		CHIPID(pi->sh->chip) == BCM43460_CHIP_ID || \
		CHIPID(pi->sh->chip) == BCM43526_CHIP_ID || \
		BCM43602_CHIP(pi->sh->chip) || CHIPID(pi->sh->chip) == BCM4364_CHIP_ID)
#define ACREV3 BCM4350_CHIP(pi->sh->chip)
#define ACREV32 BCM4365_CHIP(pi->sh->chip)
#define ACREV0_SUB (CHIPID(pi->sh->chip) == BCM4364_CHIP_ID)
#endif /* BCMCHIPID */
#else
#define ACREV0 0
#define ACREV3 0
#define ACREV32 0
#define ACREV0_SUB 0
#endif /* ACCONF || ACCONF2 */

#define ACPHY_GAIN_VS_TEMP_SLOPE_2G 7   /* units: db/100C */
#define ACPHY_GAIN_VS_TEMP_SLOPE_5G 7   /* units: db/100C */
#define ACPHY_SWCTRL_NVRAM_PARAMS 5
#define ACPHY_RSSIOFFSET_NVRAM_PARAMS 4
#define ACPHY_GAIN_DELTA_2G_PARAMS 2
/* The above variable had only 2 params (gain settings)
 * ELNA ON and ELNA OFF
 * With Olympic, we added 2 more gain settings for 2 Routs
 * Order of the variables is - ELNA_On, ELNA_Off, Rout_1, Rout_2
 * Both the routs should be with ELNA on, as
 * Elna_on - Elna_off offset is added to tr_loss and
 * ELNA_Off value is never used again.
 */
#define ACPHY_GAIN_DELTA_2G_PARAMS_EXT 4
#define ACPHY_GAIN_DELTA_ELNA_ON 0
#define ACPHY_GAIN_DELTA_ELNA_OFF 1
#define ACPHY_GAIN_DELTA_ROUT_1 2
#define ACPHY_GAIN_DELTA_ROUT_2 3

#define ACPHY_GAIN_DELTA_5G_PARAMS_EXT 4
#define ACPHY_GAIN_DELTA_5G_PARAMS 2
#ifdef UNRELEASEDCHIP
#define ACPHY_RCAL_OFFSET (((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) || \
	(CHIPID(pi->sh->chip) == BCM4355_CHIP_ID)) ? 0x14 : 0x10)
#else
#define ACPHY_RCAL_OFFSET  0x10  /* otp offset for Wlan RCAL code */
#endif
#define ACPHY_RCAL_VAL_1X1 0xa  /* hard coded rcal_trim val for 1X1 chips */
#define ACPHY_RCAL_VAL_2X2 0x9  /* hard coded rcal_trim val for 2X2 chips */

/* ACPHY tables */
#define ACPHY_TBL_ID_MCS                          1
#define ACPHY_TBL_ID_TXEVMTBL                     2
#define ACPHY_TBL_ID_NVNOISESHAPINGTBL	          3
#define ACPHY_TBL_ID_NVRXEVMSHAPINGTBL	          4
#define ACPHY_TBL_ID_PHASETRACKTBL                5
#define ACPHY_TBL_ID_SQTHRESHOLD                  6
#define ACPHY_TBL_ID_RFSEQ                        7
#define ACPHY_TBL_ID_RFSEQEXT                     8
#define ACPHY_TBL_ID_ANTSWCTRLLUT                 9
#define ACPHY_TBL_ID_FEMCTRLLUT                  10
#define ACPHY_TBL_ID_GAINLIMIT                   11
#define ACPHY_TBL_ID_PHASETRACKTBL_B             11
#define ACPHY_TBL_ID_IQLOCAL                     12
#define ACPHY_TBL_ID_IQLOCAL0                    12
#define ACPHY_TBL_ID_PAPR                        13
#define ACPHY_TBL_ID_SAMPLEPLAY                  14
#define ACPHY_TBL_ID_DUPSTRNTBL                  15
#define ACPHY_TBL_ID_BFMUSERINDEX                16
#define ACPHY_TBL_ID_BFECONFIG                   17
#define ACPHY_TBL_ID_BFEMATRIX                   18
#define ACPHY_TBL_ID_FASTCHSWITCH                19
#define ACPHY_TBL_ID_RFSEQBUNDLE                 20
#define ACPHY_TBL_ID_LNAROUT                     21
#define ACPHY_TBL_ID_MCDSNRVAL                   22
#define ACPHY_TBL_ID_BFRRPT                      23
#define ACPHY_TBL_ID_BFERPT                      24
#define ACPHY_TBL_ID_NVADJTBL                    25
#define ACPHY_TBL_ID_PHASETRACKTBL_1X1           26
#define ACPHY_TBL_ID_SCD_DBMTBL                  27
#define ACPHY_TBL_ID_DCD_DBMTBL                  28
#define ACPHY_TBL_ID_SLNAGAIN                    29
#define ACPHY_TBL_ID_BFECONFIG2X2TBL             30
#define ACPHY_TBL_ID_SLNAGAINBTEXTLNA            31
#define ACPHY_TBL_ID_GAINCTRLBBMULTLUTS          32
#define ACPHY_TBL_ID_ESTPWRSHFTLUTS              33
#define ACPHY_TBL_ID_CHANNELSMOOTHING_1x1        34
#define ACPHY_TBL_ID_SGIADJUST                   35
#define ACPHY_TBL_ID_FDCSMTETBL                  32
#define ACPHY_TBL_ID_FDCSMTE2TBL                 33
#define ACPHY_TBL_ID_FDCSPWINTBL                 34
#define ACPHY_TBL_ID_FDCSFILTCOEFTBL             35
#define ACPHY_TBL_ID_SLNAGAINTR                  36
#define ACPHY_TBL_ID_SLNAGAINEXTLNATR            37
#define ACPHY_TBL_ID_SLNACLIPSTAGETOTGAIN        38
#define ACPHY_TBL_ID_VASIPREGISTERS              41
#define ACPHY_TBL_ID_IQLOCAL1                    44
#define ACPHY_TBL_ID_SVMPMEMS                    48
#define ACPHY_TBL_ID_SVMPSAMPCOL                 49
#define ACPHY_TBL_ID_ESTPWRLUTS0                 64
#define ACPHY_TBL_ID_IQCOEFFLUTS0                65
#define ACPHY_TBL_ID_LOFTCOEFFLUTS0              66
#define ACPHY_TBL_ID_RFPWRLUTS0                  67
#define ACPHY_TBL_ID_GAIN0                       68
#define ACPHY_TBL_ID_GAINBITS0                   69
#define ACPHY_TBL_ID_RSSICLIPGAIN0               70
#define ACPHY_TBL_ID_EPSILON0                    71
#define ACPHY_TBL_ID_SCALAR0                     72
#define ACPHY_TBL_ID_CORE0CHANESTTBL             73
#define ACPHY_TBL_ID_CORE0CHANSMTH_CHAN          78
#define ACPHY_TBL_ID_CORE0CHANSMTH_FLTR          79
#define ACPHY_TBL_ID_SNOOPAGC                    80
#define ACPHY_TBL_ID_SNOOPPEAK                   81
#define ACPHY_TBL_ID_SNOOPCCKLMS                 82
#define ACPHY_TBL_ID_SNOOPLMS                    83
#define ACPHY_TBL_ID_SNOOPDCCMP                  84
#define ACPHY_TBL_ID_BBPDRFPWRLUTS0              85
#define ACPHY_TBL_ID_BBPDEPSILON_I0              86
#define ACPHY_TBL_ID_BBPDEPSILON_Q0              87
#define ACPHY_TBL_ID_FDSS_MCSINFOTBL0            88
#define ACPHY_TBL_ID_FDSS_MCSINFOTBL1            120
#define ACPHY_TBL_ID_FDSS_SCALEADJUSTFACTORSTBL0 89
#define ACPHY_TBL_ID_FDSS_SCALEADJUSTFACTORSTBL1 121
#define ACPHY_TBL_ID_FDSS_BREAKPOINTSTBL0        90
#define ACPHY_TBL_ID_FDSS_BREAKPOINTSTBL1        122
#define ACPHY_TBL_ID_FDSS_SCALEFACTORSTBL0       91
#define ACPHY_TBL_ID_FDSS_SCALEFACTORSTBL1       123
#define ACPHY_TBL_ID_FDSS_SCALEFACTORSDELTATBL0  92
#define ACPHY_TBL_ID_FDSS_SCALEFACTORSDELTATBL1  124
#define ACPHY_TBL_ID_PAPDLUTSELECT0              93
#define ACPHY_TBL_ID_GAINLIMIT0                  94
#define ACPHY_TBL_ID_TXGAINCTRLBBMULTLUTS0       95
#define ACPHY_TBL_ID_RESAMPLERTBL                95
#define ACPHY_TBL_ID_ESTPWRLUTS1                 96
#define ACPHY_TBL_ID_IQCOEFFLUTS1                97
#define ACPHY_TBL_ID_LOFTCOEFFLUTS1              98
#define ACPHY_TBL_ID_RFPWRLUTS1                  99
#define ACPHY_TBL_ID_GAIN1                      100
#define ACPHY_TBL_ID_GAINBITS1                  101
#define ACPHY_TBL_ID_RSSICLIPGAIN1              102
#define ACPHY_TBL_ID_EPSILON1                   103
#define ACPHY_TBL_ID_SCALAR1                    104
#define ACPHY_TBL_ID_CORE1CHANESTTBL            105
#define ACPHY_TBL_ID_CORE1CHANSMTH_CHAN         110
#define ACPHY_TBL_ID_CORE1CHANSMTH_FLTR         111
#define ACPHY_TBL_ID_RSSITOGAINCODETBL0         114
#define ACPHY_TBL_ID_DYNRADIOREGTBL0            115
#define ACPHY_TBL_ID_ADCSAMPCAP_PATH0           116
#define ACPHY_TBL_ID_BBPDRFPWRLUTS1             117
#define ACPHY_TBL_ID_BBPDEPSILON_I1             118
#define ACPHY_TBL_ID_BBPDEPSILON_Q1             119
#define ACPHY_TBL_ID_MCSINFOTBL1                120
#define ACPHY_TBL_ID_SCALEADJUSTFACTORSTBL1     121
#define ACPHY_TBL_ID_BREAKPOINTSTBL1            122
#define ACPHY_TBL_ID_SCALEFACTORSTBL1           123
#define ACPHY_TBL_ID_SCALEFACTORSDELTATBL1      124
#define ACPHY_TBL_ID_PAPDLUTSELECT1             125
#define ACPHY_TBL_ID_GAINLIMIT1                 126
#define ACPHY_TBL_ID_TXGAINCTRLBBMULTLUTS1      127
#define ACPHY_TBL_ID_MCLPAGCCLIP2TBL0           127

/* Table IDs 128 and 129 have conflicting table names
	for 4349:
		ID 128 -- ACPHY_TBL_ID_GAINCTRLBBMULTLUTS0
		ID 129 -- ACPHY_TBL_ID_ESTPWRSHFTLUTS0
	for other chips:
		ID 128 -- ACPHY_TBL_ID_ESTPWRLUTS2
		ID 129 -- ACPHY_TBL_ID_IQCOEFFLUTS2

	This is handled by appropriately conditioning the code
	with major and minor revs
*/
/*
#define ACPHY_TBL_ID_IQLOCALTBL0			12
#define ACPHY_TBL_ID_LNAROUTLUT				21
#define ACPHY_TBL_ID_SGIADJUSTTBL                       35
#define ACPHY_TBL_ID_EPSILONTABLE0			71
#define ACPHY_TBL_ID_SCALARTABLE0                       72
#define ACPHY_TBL_ID_RSSICLIP2GAIN0                     76
#define ACPHY_TBL_ID_DCOE_TABLE0                        130
#define ACPHY_TBL_ID_IDAC_TABLE0                        131
#define ACPHY_TBL_ID_IDAC_GMAP_TABLE0                   132
#define ACPHY_TBL_ID_ET_SHAPING_LUT                     137
#define ACPHY_TBL_ID_ETMFTABLE0                         137
#define ACPHY_TBL_ID_WBCAL_PTBL0                        140
#define ACPHY_TBL_ID_WBCAL_TXDELAY_BUF0                 144
*/
#define ACPHY_TBL_ID_ESTPWRLUTS2                128
#define ACPHY_TBL_ID_IQCOEFFLUTS2               129

#define ACPHY_TBL_ID_GAINCTRLBBMULTLUTS0 		128
#define ACPHY_TBL_ID_ESTPWRSHFTLUTS0    		129

#define ACPHY_TBL_ID_LOFTCOEFFLUTS2             130
#define ACPHY_TBL_ID_RFPWRLUTS2                 131
#define ACPHY_TBL_ID_GAIN2                      132
#define ACPHY_TBL_ID_GAINBITS2                  133
#define ACPHY_TBL_ID_RSSICLIPGAIN2              134
#define ACPHY_TBL_ID_EPSILON2                   135
#define ACPHY_TBL_ID_SCALAR2                    136
#define ACPHY_TBL_ID_CORE2CHANESTTBL            137
#define ACPHY_TBL_ID_CORE2CHNSMCHANNELTBL       142
#define ACPHY_TBL_ID_RSSITOGAINCODETBL1         146
#define ACPHY_TBL_ID_DYNRADIOREGTBL1    		147
#define ACPHY_TBL_ID_ADCSAMPCAP_PATH1   		148
#define ACPHY_TBL_ID_GAINLIMIT2                 158
#define ACPHY_TBL_ID_TXGAINCTRLBBMULTLUTS2      159
#define ACPHY_TBL_ID_MCLPAGCCLIP2TBL1           159
#define ACPHY_TBL_ID_GAINCTRLBBMULTLUTS1 		160
#define ACPHY_TBL_ID_ESTPWRLUTS3                160
#define ACPHY_TBL_ID_ESTPWRSHFTLUTS1    		161
#define ACPHY_TBL_ID_IDAC_GMAP0			132
#define ACPHY_TBL_ID_IQCOEFFLUTS3               161
#define ACPHY_TBL_ID_LOFTCOEFFLUTS3             162
#define ACPHY_TBL_ID_RFPWRLUTS3                 163
#define ACPHY_TBL_ID_GAIN3                      164
#define ACPHY_TBL_ID_GAINBITS3                  165
#define ACPHY_TBL_ID_RSSICLIPGAIN3              166
#define ACPHY_TBL_ID_EPSILON3                   167
#define ACPHY_TBL_ID_SCALAR3                    168
#define ACPHY_TBL_ID_CORE3CHANESTTBL            169
#define ACPHY_TBL_ID_CORE3CHNSMCHANNELTBL       174
#define ACPHY_TBL_ID_GAINCTRLBBMULTLUTS2        176
#define ACPHY_TBL_ID_ESTPWRSHFTLUTS2            177
#define ACPHY_TBL_ID_RSSITOGAINCODETBL2         178
#define ACPHY_TBL_ID_DYNRADIOREGTBL2            179
#define ACPHY_TBL_ID_GAINLIMIT3                 190
#define ACPHY_TBL_ID_MCLPAGCCLIP2TBL2           191
#define ACPHY_TBL_ID_GAINCTRLBBMULTLUTS3        208
#define ACPHY_TBL_ID_ESTPWRSHFTLUTS3            209
#define ACPHY_TBL_ID_RSSITOGAINCODETBL3         210
#define ACPHY_TBL_ID_DYNRADIOREGTBL3            211
#define ACPHY_TBL_ID_MCLPAGCCLIP2TBL3           223
#define ACPHY_TBL_ID_ADCSAMPCAP                 224
#define ACPHY_TBL_ID_DACRAWSAMPPLAY             225
#define ACPHY_TBL_ID_TSSIMEMTBL                 226

/* Name conflict, use new AC2PHY tables name */
#define AC2PHY_TBL_ID_CHANNELSMOOTHING_1x1      36
#define AC2PHY_TBL_ID_SGIADJUST                 37
#define AC2PHY_TBL_ID_SLNAGAINTR                38
#define AC2PHY_TBL_ID_SLNAGAINEXTLNATR          39
#define AC2PHY_TBL_ID_SLNACLIPSTAGETOTGAIN      40
#define AC2PHY_TBL_ID_IQLOCAL                   50
#define AC2PHY_TBL_ID_GAINCTRLBBMULTLUTS0       112
#define AC2PHY_TBL_ID_ESTPWRSHFTLUTS0           113
#define AC2PHY_TBL_ID_GAINCTRLBBMULTLUTS1       144
#define AC2PHY_TBL_ID_ESTPWRSHFTLUTS1           145
#define ACPHY_TBL_ID_GAINLIMIT3                 190
#define AC2PHY_TBL_ID_DCOE_TABLE0               321
#define AC2PHY_TBL_ID_IDAC_TABLE0               322
#define AC2PHY_TBL_ID_IDAC_GMAP_TABLE0          323
#define AC2PHY_TBL_ID_DCOE_TABLE1               353
#define AC2PHY_TBL_ID_IDAC_TABLE1               354
#define AC2PHY_TBL_ID_IDAC_GMAP_TABLE1          355

/* 4365C0 specific PHY tables */
#define ACPHY_TBL_ID_PHASETRACKTBL_B          11

#define ACPHY_NUM_DIG_FILT_COEFFS 				15
#define ACPHY_TBL_LEN_NVNOISESHAPINGTBL         256
#define ACPHY_SPURWAR_NTONES_OFFSET             24 /* Starting offset for spurwar */
#define ACPHY_NV_NTONES_OFFSET                  0 /* Starting offset for nvshp */
#define ACPHY_SPURWAR_NTONES                    8 /* Numver of tones for spurwar */
#define ACPHY_NV_NTONES                         24 /* Numver of tones for nvshp */
/* Number of tones(spurwar+nvshp) to be written */
#define ACPHY_SPURWAR_NV_NTONES                 32

/* channel smoothing 1x1 and core0 chanest data formate */
#define UNPACK_FLOAT_AUTO_SCALE			1

#define CORE0CHANESTTBL_TABLE_WIDTH		32
#define CORE0CHANESTTBL_INTEGER_DATA_SIZE	14
#define CORE0CHANESTTBL_INTEGER_DATA_MASK	((1 << CORE0CHANESTTBL_INTEGER_DATA_SIZE) - 1)
#define CORE0CHANESTTBL_INTEGER_MAXVALUE	((CORE0CHANESTTBL_INTEGER_DATA_MASK+1)>>1)

#define CORE0CHANESTTBL_FLOAT_FORMAT		1
#define CORE0CHANESTTBL_REV0_DATA_SIZE		12
#define CORE0CHANESTTBL_REV0_EXP_SIZE		6
#define CORE0CHANESTTBL_REV2_DATA_SIZE		9
#define CORE0CHANESTTBL_REV2_EXP_SIZE		5

#define CHANNELSMOOTHING_FLOAT_FORMAT		0
#define CHANNELSMOOTHING_FLOAT_DATA_SIZE	11
#define CHANNELSMOOTHING_FLOAT_EXP_SIZE		8
#define CHANNELSMOOTHING_DATA_OFFSET		256

#define RXEVMTBL_DEPTH      256
#define RXNOISESHPTBL_DEPTH 256
#define IQLUT_DEPTH         128
#define LOFTLUT_DEPTH       128
#define ESTPWRSHFTLUT_DEPTH 64
#define RXEVMTBL_DEPTH_MAXBW20      64
#define RXNOISESHPTBL_DEPTH_MAXBW20 64
#define SQTHRESHOLDTBL_DEPTH_MAXBW20 64

/* hirssi elnabypass */
#define PHY_SW_HIRSSI_UCODE_CAP(pi)	ACMAJORREV_0((pi)->pubpi->phy_rev)
#define PHY_SW_HIRSSI_PERIOD      5    /* 5 second timeout */
#define PHY_SW_HIRSSI_OFF         (-1)
#define PHY_SW_HIRSSI_BYP_THR    (-13)
#define PHY_SW_HIRSSI_RES_THR    (-15)
#define PHY_SW_HIRSSI_W1_BYP_REG  ACPHY_W2W1ClipCnt3(rev)
#define PHY_SW_HIRSSI_W1_BYP_CNT  31
#define PHY_SW_HIRSSI_W1_RES_REG  ACPHY_W2W1ClipCnt1(rev)
#define PHY_SW_HIRSSI_W1_RES_CNT  31

#define ACPHY_TBL_ID_ESTPWRLUTS(core)	\
	(((core == 0) ? ACPHY_TBL_ID_ESTPWRLUTS0 : \
	((core == 1) ? ACPHY_TBL_ID_ESTPWRLUTS1 : \
	((core == 2) ? ACPHY_TBL_ID_ESTPWRLUTS2 : ACPHY_TBL_ID_ESTPWRLUTS3))))

#define ACPHY_TBL_ID_CHANEST(core)	\
	(((core == 0) ? ACPHY_TBL_ID_CORE0CHANESTTBL : \
	((core == 1) ? ACPHY_TBL_ID_CORE1CHANESTTBL : \
	((core == 2) ? ACPHY_TBL_ID_CORE2CHANESTTBL : ACPHY_TBL_ID_CORE3CHANESTTBL))))

#define ACPHY_TBL_ID_FDSS_MCSINFOTBL(core) \
		(((core == 0) ? ACPHY_TBL_ID_FDSS_MCSINFOTBL0 : \
		  ACPHY_TBL_ID_FDSS_MCSINFOTBL1))
#define ACPHY_TBL_ID_FDSS_SCALEADJUSTFACTORSTBL(core) \
		(((core == 0) ? ACPHY_TBL_ID_FDSS_SCALEADJUSTFACTORSTBL0 : \
		  ACPHY_TBL_ID_FDSS_SCALEADJUSTFACTORSTBL1))
#define ACPHY_TBL_ID_FDSS_BREAKPOINTSTBL(core) \
		(((core == 0) ? ACPHY_TBL_ID_FDSS_BREAKPOINTSTBL0 : \
		  ACPHY_TBL_ID_FDSS_BREAKPOINTSTBL1))
#define ACPHY_TBL_ID_FDSS_SCALEFACTORSTBL(core) \
		(((core == 0) ? ACPHY_TBL_ID_FDSS_SCALEFACTORSTBL0 : \
		  ACPHY_TBL_ID_FDSS_SCALEFACTORSTBL1))
#define ACPHY_TBL_ID_FDSS_SCALEFACTORSDELTATBL(core) \
		(((core == 0) ? ACPHY_TBL_ID_FDSS_SCALEFACTORSDELTATBL0 : \
		  ACPHY_TBL_ID_FDSS_SCALEFACTORSDELTATBL1))
#define ACPHY_TBL_ID_FDSS_MCSINFOTBL(core) \
	(((core == 0) ? ACPHY_TBL_ID_FDSS_MCSINFOTBL0 : \
	  ACPHY_TBL_ID_FDSS_MCSINFOTBL1))
#define ACPHY_TBL_ID_FDSS_SCALEADJUSTFACTORSTBL(core) \
	(((core == 0) ? ACPHY_TBL_ID_FDSS_SCALEADJUSTFACTORSTBL0 : \
	  ACPHY_TBL_ID_FDSS_SCALEADJUSTFACTORSTBL1))
#define ACPHY_TBL_ID_FDSS_BREAKPOINTSTBL(core) \
	(((core == 0) ? ACPHY_TBL_ID_FDSS_BREAKPOINTSTBL0 : \
	  ACPHY_TBL_ID_FDSS_BREAKPOINTSTBL1))
#define ACPHY_TBL_ID_FDSS_SCALEFACTORSTBL(core) \
	(((core == 0) ? ACPHY_TBL_ID_FDSS_SCALEFACTORSTBL0 : \
	  ACPHY_TBL_ID_FDSS_SCALEFACTORSTBL1))
#define ACPHY_TBL_ID_FDSS_SCALEFACTORSDELTATBL(core) \
	(((core == 0) ? ACPHY_TBL_ID_FDSS_SCALEFACTORSDELTATBL0 : \
	  ACPHY_TBL_ID_FDSS_SCALEFACTORSDELTATBL1))

/* This implements a WAR for bug in 4349A0 RTL where 5G lnaRoutLUT locations
 * and lna2 locations are not accessible. For more details refer the
 * 4349 Phy cheatsheet and JIRA:SW4349-243
 */
#define ACPHY_LNAROUT_BAND_OFFSET(pi, chanspec) \
	(CHSPEC_IS5G(chanspec) ? 8 : 0)

#define ACPHY_LNAROUT_CORE_WRT_OFST(phy_rev, core) (24*core)

/* lnaRoutLUT WAR for 4349A2: Read offset is always kept to 0 */
#define ACPHY_LNAROUT_CORE_RD_OFST(pi, core) 	(24*core)

/* ACPHY RFSeq Commands */
#define ACPHY_RFSEQ_RX2TX		0x0
#define ACPHY_RFSEQ_TX2RX		0x1
#define ACPHY_RFSEQ_RESET2RX		0x2
#define ACPHY_RFSEQ_UPDATEGAINH		0x3
#define ACPHY_RFSEQ_UPDATEGAINL		0x4
#define ACPHY_RFSEQ_UPDATEGAINU		0x5
#define ACPHY_RFSEQ_OCL_RESET2RX	0x6
#define ACPHY_RFSEQ_OCL_TX2RX 		0x7


#define ACPHY_SPINWAIT_VCO_CAL_STATUS		100*100
#define ACPHY_SPINWAIT_AFE_CAL_STATUS		100*100
#define ACPHY_SPINWAIT_RFSEQ_STOP		1000
#define ACPHY_SPINWAIT_RFSEQ_FORCE		200000
#define ACPHY_SPINWAIT_RUNSAMPLE		1000
#define ACPHY_SPINWAIT_TXIQLO			20000
#define ACPHY_SPINWAIT_TXIQLO_L			5000000
#define ACPHY_SPINWAIT_IQEST			10000
#define ACPHY_SPINWAIT_PMU_CAL_STATUS		100*100
#define ACPHY_SPINWAIT_RCAL_STATUS		1000*100
#define ACPHY_SPINWAIT_MINIPMU_CAL_STATUS	100*100
#define ACPHY_SPINWAIT_FCBS_CHSW_STATUS		1000
#define ACPHY_SPINWAIT_NOISE_CAL_STATUS		83*1000
#define ACPHY_SPINWAIT_RESET2RX		1000
#define ACPHY_SPINWAIT_PKTPROC_STATE		1000

#define ACPHY_NUM_BW                    3
#define ACPHY_NUM_CHANS                 123
#define ACPHY_NUM_BW_2G                 2

#define ACPHY_ClassifierCtrl_classifierSel_MASK 0x7

#define ACPHY_RFSEQEXT_TBL_WIDTH	60
#define ACPHY_GAINLMT_TBL_WIDTH		8
#define ACPHY_GAINDB_TBL_WIDTH		8
#define ACPHY_GAINBITS_TBL_WIDTH	8



/* JIRA(CRDOT11ACPHY-142) - Don't use idx = 0 of lna1/lna2 */
#define ACPHY_MIN_LNA1_LNA2_IDX 1
/* We're ok to use index 0 on tiny phys */
#define ACPHY_MIN_LNA1_LNA2_IDX_TINY 0


/* AvVmid from NVRAM */
#define ACPHY_NUM_BANDS 5
#define ACPHY_AVVMID_NVRAM_PARAMS 2

/* wrapper macros to enable invalid register accesses error messages */
#if defined(BCMDBG_PHYREGS_TRACE)
#define _PHY_REG_READ(pi, reg)			phy_utils_read_phyreg_debug(pi, reg, #reg)
#define _PHY_REG_MOD(pi, reg, mask, val)	phy_utils_mod_phyreg_debug(pi, reg, mask, val, #reg)
#define _READ_RADIO_REG(pi, reg)		phy_utils_read_radioreg_debug(pi, reg, #reg)
#define _MOD_RADIO_REG(pi, reg, mask, val) \
	phy_utils_mod_radioreg_debug(pi, reg, mask, val, #reg)
#define _PHY_REG_WRITE(pi, reg, val)		phy_utils_write_phyreg_debug(pi, reg, val, #reg)
#else
#define _PHY_REG_READ(pi, reg)			phy_utils_read_phyreg(pi, reg)
#define _PHY_REG_MOD(pi, reg, mask, val)	phy_utils_mod_phyreg(pi, reg, mask, val)
#define _READ_RADIO_REG(pi, reg)		phy_utils_read_radioreg(pi, reg)
#define _WRITE_RADIO_REG(pi, reg, val)		phy_utils_write_radioreg(pi, reg, val)
#define _MOD_RADIO_REG(pi, reg, mask, val)	phy_utils_mod_radioreg(pi, reg, mask, val)
#define _PHY_REG_WRITE(pi, reg, val)		phy_utils_write_phyreg(pi, reg, val)
#endif /* BCMDBG_PHYREGS_TRACE */


#if PHY_CORE_MAX == 1	/* Single PHY core chips */

#define ACPHY_REG_FIELD_MASK(pi, reg, core, field)	\
	ACPHY_Core0##reg##_##field##_MASK(pi->pubpi->phy_rev)
#define ACPHY_REG_FIELD_SHIFT(pi, reg, core, field)	\
	ACPHY_Core0##reg##_##field##_SHIFT(pi->pubpi->phy_rev)
#define ACPHY_REG_FIELD_MASKE(pi, reg, core, field)	\
	ACPHY_##reg##0_##field##_MASK(pi->pubpi->phy_rev)
#define ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field)	\
	ACPHY_##reg##0_##field##_SHIFT(pi->pubpi->phy_rev)
#define ACPHY_REG_FIELD_MASKM(pi, reg0, reg1, core, field)	\
	ACPHY_##reg0##0_##reg1##_##field##_MASK(pi->pubpi->phy_rev)
#define ACPHY_REG_FIELD_SHIFTM(pi, reg0, reg1, core, field)	\
	ACPHY_##reg0##0_##reg1##_##field##_SHIFT(pi->pubpi->phy_rev)
#define ACPHY_REG_FIELD_MASKEE(pi, reg, core, field)	\
	ACPHY_##reg##0_##field##0_MASK(pi->pubpi->phy_rev)
#define ACPHY_REG_FIELD_SHIFTEE(pi, reg, core, field)	\
	ACPHY_##reg##0_##field##0_SHIFT(pi->pubpi->phy_rev)
#define ACPHY_REG_FIELD_MASKXE(pi, reg, field, core)	\
	ACPHY_##reg##_##field##0_MASK(pi->pubpi->phy_rev)
#define ACPHY_REG_FIELD_SHIFTXE(pi, reg, field, core)	\
	ACPHY_##reg##_##field##0_SHIFT(pi->pubpi->phy_rev)

#elif PHY_CORE_MAX == 2	/* Dual PHY core chips */

#define ACPHY_REG_FIELD_MASK(pi, reg, core, field) \
	((core == 0) ? ACPHY_Core0##reg##_##field##_MASK(pi->pubpi->phy_rev) : \
	 ACPHY_Core1##reg##_##field##_MASK(pi->pubpi->phy_rev))
#define ACPHY_REG_FIELD_SHIFT(pi, reg, core, field) \
	((core == 0) ? ACPHY_Core0##reg##_##field##_SHIFT(pi->pubpi->phy_rev) : \
	 ACPHY_Core1##reg##_##field##_SHIFT(pi->pubpi->phy_rev))
#define ACPHY_REG_FIELD_MASKE(pi, reg, core, field) \
	((core == 0) ? ACPHY_##reg##0_##field##_MASK(pi->pubpi->phy_rev) : \
	 ACPHY_##reg##1_##field##_MASK(pi->pubpi->phy_rev))
#define ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field) \
	((core == 0) ? ACPHY_##reg##0_##field##_SHIFT(pi->pubpi->phy_rev) : \
	 ACPHY_##reg##1_##field##_SHIFT(pi->pubpi->phy_rev))
#define ACPHY_REG_FIELD_MASKM(pi, reg0, reg1, core, field) \
	((core == 0) ? ACPHY_##reg0##0_##reg1##_##field##_MASK(pi->pubpi->phy_rev) : \
	 ACPHY_##reg0##1_##reg1##_##field##_MASK(pi->pubpi->phy_rev))
#define ACPHY_REG_FIELD_SHIFTM(pi, reg0, reg1, core, field) \
	((core == 0) ? ACPHY_##reg0##0_##reg1##_##field##_SHIFT(pi->pubpi->phy_rev) : \
	 ACPHY_##reg0##1_##reg1##_##field##_SHIFT(pi->pubpi->phy_rev))
#define ACPHY_REG_FIELD_MASKEE(pi, reg, core, field) \
	((core == 0) ? ACPHY_##reg##0_##field##0_MASK(pi->pubpi->phy_rev) : \
	 ACPHY_##reg##1_##field##1_MASK(pi->pubpi->phy_rev))
#define ACPHY_REG_FIELD_SHIFTEE(pi, reg, core, field) \
	((core == 0) ? ACPHY_##reg##0_##field##0_SHIFT(pi->pubpi->phy_rev) : \
	 ACPHY_##reg##1_##field##1_SHIFT(pi->pubpi->phy_rev))
#define ACPHY_REG_FIELD_MASKXE(pi, reg, field, core) \
	((core == 0) ? ACPHY_##reg##_##field##0_MASK(pi->pubpi->phy_rev) : \
	ACPHY_##reg##_##field##1_MASK(pi->pubpi->phy_rev))
#define ACPHY_REG_FIELD_SHIFTXE(pi, reg, field, core) \
	((core == 0) ? ACPHY_##reg##_##field##0_SHIFT(pi->pubpi->phy_rev) : \
	ACPHY_##reg##_##field##1_SHIFT(pi->pubpi->phy_rev))

#else

#define ACPHY_REG_FIELD_MASK(pi, reg, core, field) \
	((core == 0) ? ACPHY_Core0##reg##_##field##_MASK(pi->pubpi->phy_rev) : \
	((core == 1) ? ACPHY_Core1##reg##_##field##_MASK(pi->pubpi->phy_rev) : \
	ACPHY_Core2##reg##_##field##_MASK(pi->pubpi->phy_rev)))
#define ACPHY_REG_FIELD_SHIFT(pi, reg, core, field) \
	((core == 0) ? ACPHY_Core0##reg##_##field##_SHIFT(pi->pubpi->phy_rev) : \
	((core == 1) ? ACPHY_Core1##reg##_##field##_SHIFT(pi->pubpi->phy_rev) : \
	ACPHY_Core2##reg##_##field##_SHIFT(pi->pubpi->phy_rev)))
#define ACPHY_REG_FIELD_MASKE(pi, reg, core, field) \
	((core == 0) ? ACPHY_##reg##0_##field##_MASK(pi->pubpi->phy_rev) : \
	((core == 1) ? ACPHY_##reg##1_##field##_MASK(pi->pubpi->phy_rev) : \
	ACPHY_##reg##2_##field##_MASK(pi->pubpi->phy_rev)))
#define ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field) \
	((core == 0) ? ACPHY_##reg##0_##field##_SHIFT(pi->pubpi->phy_rev) : \
	((core == 1) ? ACPHY_##reg##1_##field##_SHIFT(pi->pubpi->phy_rev) : \
	ACPHY_##reg##2_##field##_SHIFT(pi->pubpi->phy_rev)))
#define ACPHY_REG_FIELD_MASKM(pi, reg0, reg1, core, field) \
	((core == 0) ? ACPHY_##reg0##0_##reg1##_##field##_MASK(pi->pubpi->phy_rev) : \
	((core == 1) ? ACPHY_##reg0##1_##reg1##_##field##_MASK(pi->pubpi->phy_rev) : \
	ACPHY_##reg0##2_##reg1##_##field##_MASK(pi->pubpi->phy_rev)))
#define ACPHY_REG_FIELD_SHIFTM(pi, reg0, reg1, core, field) \
	((core == 0) ? ACPHY_##reg0##0_##reg1##_##field##_SHIFT(pi->pubpi->phy_rev) : \
	((core == 1) ? ACPHY_##reg0##1_##reg1##_##field##_SHIFT(pi->pubpi->phy_rev) : \
	ACPHY_##reg0##2_##reg1##_##field##_SHIFT(pi->pubpi->phy_rev)))
#define ACPHY_REG_FIELD_MASKEE(pi, reg, core, field) \
	((core == 0) ? ACPHY_##reg##0_##field##0_MASK(pi->pubpi->phy_rev) : \
	((core == 1) ? ACPHY_##reg##1_##field##1_MASK(pi->pubpi->phy_rev) : \
	ACPHY_##reg##2_##field##2_MASK(pi->pubpi->phy_rev)))
#define ACPHY_REG_FIELD_SHIFTEE(pi, reg, core, field) \
	((core == 0) ? ACPHY_##reg##0_##field##0_SHIFT(pi->pubpi->phy_rev) : \
	((core == 1) ? ACPHY_##reg##1_##field##1_SHIFT(pi->pubpi->phy_rev) : \
	ACPHY_##reg##2_##field##2_SHIFT(pi->pubpi->phy_rev)))
#define ACPHY_REG_FIELD_MASKXE(pi, reg, field, core) \
	((core == 0) ? ACPHY_##reg##_##field##0_MASK(pi->pubpi->phy_rev) : \
	((core == 1) ? ACPHY_##reg##_##field##1_MASK(pi->pubpi->phy_rev) : \
	ACPHY_##reg##_##field##2_MASK(pi->pubpi->phy_rev)))
#define ACPHY_REG_FIELD_SHIFTXE(pi, reg, field, core) \
	((core == 0) ? ACPHY_##reg##_##field##0_SHIFT(pi->pubpi->phy_rev) : \
	((core == 1) ? ACPHY_##reg##_##field##1_SHIFT(pi->pubpi->phy_rev) : \
	ACPHY_##reg##_##field##2_SHIFT(pi->pubpi->phy_rev)))

#endif	/* PHY_CORE_MAX */

/**
 * Register and register bitfield access macro's. Macro postfixes and associated (example)
 * register bitfields:
 *
 * None: e.g. ACPHY_RfctrlCmd_chip_pu (does not take core# into account)
 * C   : e.g. ACPHY_Core2FastAgcClipCntTh_fastAgcNbClipCntTh
 * CE  : e.g. ACPHY_RfctrlOverrideAfeCfg0_afe_iqdac_pwrup
 * CEE : e.g. ACPHY_EpsilonTableAdjust0_epsilonOffset0
 */

#define ACPHYREGCE(pi, reg, core) ((ACPHY_##reg##0(pi->pubpi->phy_rev)) \
	+ ((core) * PHY_REG_BANK_CORE1_OFFSET))
#define ACPHYREGCM(pi, reg0, reg1, core) ((ACPHY_##reg0##0_##reg1(pi->pubpi->phy_rev)) \
	+ ((core) * PHY_REG_BANK_CORE1_OFFSET))
#define ACPHYREGC(pi, reg, core) ((ACPHY_Core0##reg(pi->pubpi->phy_rev)) \
	+ ((core) * PHY_REG_BANK_CORE1_OFFSET))

/* When the broadcast bit is set in the PHY reg address
 * it writes to the corresponding registers in all the cores
 * See the defintion of the PhyRegAddr (Offset 0x3FC) MAC register
 */
#define ACPHY_REG_BROADCAST(pi) \
	(D11REV_GE(pi->sh->corerev, 64) ? 0x4000: ((ACREV0 || ACREV3) ? 0x1000 : 0))

#define WRITE_PHYREG(pi, reg, value)					\
	_PHY_REG_WRITE(pi, ACPHY_##reg(pi->pubpi->phy_rev), (value))

#define WRITE_PHYREGC(pi, reg, core, value)			\
	_PHY_REG_WRITE(pi, ACPHYREGC(pi, reg, core), (value))

#define WRITE_PHYREGCE(pi, reg, core, value)			\
	_PHY_REG_WRITE(pi, ACPHYREGCE(pi, reg, core), (value))

#define MOD_PHYREG(pi, reg, field, value)				\
	_PHY_REG_MOD(pi, ACPHY_##reg(pi->pubpi->phy_rev),		\
		ACPHY_##reg##_##field##_MASK(pi->pubpi->phy_rev),	\
		((value) << ACPHY_##reg##_##field##_##SHIFT(pi->pubpi->phy_rev)))

#define MOD_PHYREG_2(pi, reg, field1, value1, field2, value2) \
	_PHY_REG_MOD(pi, ACPHY_##reg(pi->pubpi->phy_rev), \
		(ACPHY_##reg##_##field1##_MASK(pi->pubpi->phy_rev) |\
			ACPHY_##reg##_##field2##_MASK(pi->pubpi->phy_rev)), \
			(((value1) << ACPHY_##reg##_##field1##_##SHIFT(pi->pubpi->phy_rev)) |\
	   ((value2) << ACPHY_##reg##_##field2##_##SHIFT(pi->pubpi->phy_rev))))
#define MOD_PHYREGCE_2(pi, reg, core, field1, value1, field2, value2) \
	_PHY_REG_MOD(pi, ACPHYREGCE(pi, reg, core), \
		(ACPHY_REG_FIELD_MASKE(pi, reg, core, field1) |\
		ACPHY_REG_FIELD_MASKE(pi, reg, core, field2)), \
			(((value1) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field1))|\
			((value2) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field2))))

#define MOD_PHYREGCE_3(pi, reg, core, field1, value1, field2, value2, field3, value3) \
	_PHY_REG_MOD(pi, ACPHYREGCE(pi, reg, core), \
		(ACPHY_REG_FIELD_MASKE(pi, reg, core, field1) |\
		ACPHY_REG_FIELD_MASKE(pi, reg, core, field2) |\
		ACPHY_REG_FIELD_MASKE(pi, reg, core, field3)), \
			(((value1) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field1))|\
			((value2) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field2))|\
			((value3) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field3))))

#define MOD_PHYREGCE_4(pi, reg, core, field1, value1, field2, value2, \
field3, value3, field4, value4) \
	_PHY_REG_MOD(pi, ACPHYREGCE(pi, reg, core), \
		(ACPHY_REG_FIELD_MASKE(pi, reg, core, field1) |\
		ACPHY_REG_FIELD_MASKE(pi, reg, core, field2) |\
		ACPHY_REG_FIELD_MASKE(pi, reg, core, field3) |\
		ACPHY_REG_FIELD_MASKE(pi, reg, core, field4)), \
			(((value1) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field1))|\
			((value2) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field2))|\
			((value3) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field3))|\
			((value4) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field4))))

#define MOD_PHYREGCE_5(pi, reg, core, field1, value1, field2, value2, \
field3, value3, field4, value4, field5, value5) \
	_PHY_REG_MOD(pi, ACPHYREGCE(pi, reg, core), \
		(ACPHY_REG_FIELD_MASKE(pi, reg, core, field1) |\
		ACPHY_REG_FIELD_MASKE(pi, reg, core, field2) |\
		ACPHY_REG_FIELD_MASKE(pi, reg, core, field3) |\
		ACPHY_REG_FIELD_MASKE(pi, reg, core, field4) |\
		ACPHY_REG_FIELD_MASKE(pi, reg, core, field5)), \
			(((value1) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field1))|\
			((value2) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field2))|\
			((value3) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field3))|\
			((value4) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field4))|\
			((value5) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field5))))

#define MOD_PHYREGC(pi, reg, core, field, value)			\
	_PHY_REG_MOD(pi,						\
	            ACPHYREGC(pi, reg, core),				\
	            ACPHY_REG_FIELD_MASK(pi, reg, core, field),		\
	            ((value) << ACPHY_REG_FIELD_SHIFT(pi, reg, core, field)))

#define MOD_PHYREGCE(pi, reg, core, field, value)			\
	_PHY_REG_MOD(pi,						\
	            ACPHYREGCE(pi, reg, core),				\
	            ACPHY_REG_FIELD_MASKE(pi, reg, core, field),	\
	            ((value) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field)))

#define MOD_PHYREGCM(pi, reg0, reg1, core, field, value)			\
	_PHY_REG_MOD(pi,						\
	            ACPHYREGCM(pi, reg0, reg1, core),				\
	            ACPHY_REG_FIELD_MASKM(pi, reg0, reg1, core, field),	\
	            ((value) << ACPHY_REG_FIELD_SHIFTM(pi, reg0, reg1, core, field)))

#define MOD_PHYREGCEE(pi, reg, core, field, value)			\
	_PHY_REG_MOD(pi,						\
	            ACPHYREGCE(pi, reg, core),				\
	            ACPHY_REG_FIELD_MASKEE(pi, reg, core, field),		\
	            ((value) << ACPHY_REG_FIELD_SHIFTEE(pi, reg, core, field)))

#define MOD_PHYREGCXE(pi, reg, field, core, value)			\
	_PHY_REG_MOD(pi,						\
	            ACPHY_##reg(pi->pubpi->phy_rev),				\
	            ACPHY_REG_FIELD_MASKXE(pi, reg, field, core),		\
	            ((value) << ACPHY_REG_FIELD_SHIFTXE(pi, reg, field, core)))

#define READ_PHYREG(pi, reg) \
	_PHY_REG_READ(pi, ACPHY_##reg(pi->pubpi->phy_rev))

#define READ_PHYREGC(pi, reg, core) \
	_PHY_REG_READ(pi, ACPHYREGC(pi, reg, core))

#define READ_PHYREGCE(pi, reg, core) \
	_PHY_REG_READ(pi, ACPHYREGCE(pi, reg, core))

#define READ_PHYREGFLD(pi, reg, field)				\
	((READ_PHYREG(pi, reg)					\
	 & ACPHY_##reg##_##field##_##MASK(pi->pubpi->phy_rev)) >>	\
	 ACPHY_##reg##_##field##_##SHIFT(pi->pubpi->phy_rev))

#define READ_PHYREGFLDC(pi, reg, core, field) \
	((READ_PHYREGC(pi, reg, core) \
		& ACPHY_REG_FIELD_MASK(pi, reg, core, field)) \
		>> ACPHY_REG_FIELD_SHIFT(pi, reg, core, field))

#define READ_PHYREGFLDCE(pi, reg, core, field) \
	((READ_PHYREGCE(pi, reg, core) \
		& ACPHY_REG_FIELD_MASKE(pi, reg, core, field)) \
		>> ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field))

/* Used in PAPD cal */
#define READ_PHYREGFLDCEE(pi, reg, core, field) \
	((READ_PHYREGCE(pi, reg, core) \
		& ACPHY_REG_FIELD_MASKEE(pi, reg, core, field)) \
		>> ACPHY_REG_FIELD_SHIFTEE(pi, reg, core, field))

/* Required for register fields like RxFeCtrl1.iqswap{0,1,2} */
#define READ_PHYREGFLDCXE(pi, reg, field, core) \
	((READ_PHYREG(pi, reg) \
		& ACPHY_REG_FIELD_MASKXE(pi, reg, field, core)) \
		>> ACPHY_REG_FIELD_SHIFTXE(pi, reg, field, core))


#ifdef WLRSDB
#define ACPHYREG_BCAST(pi, reg, val) \
{\
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {\
		ASSERT(phy_get_phymode(pi) != PHYMODE_80P80); \
		WRITE_PHYREG_BCAST(pi, reg, val); \
	} else if (ACMAJORREV_3(pi->pubpi->phy_rev)) {\
		_PHY_REG_WRITE(pi, ACPHY_##reg(pi->pubpi->phy_rev), val); \
	} else {\
		_PHY_REG_WRITE(pi, ACPHY_##reg(pi->pubpi->phy_rev) | \
			ACPHY_REG_BROADCAST(pi), val); \
	}\
}
#else
#define ACPHYREG_BCAST(pi, reg, val) \
	_PHY_REG_WRITE(pi, ACPHY_##reg(pi->pubpi->phy_rev) | \
		ACPHY_REG_BROADCAST(pi), val)
#endif /* WLRSDB */

/* BCAST for rsdb family of chips */
#define WRITE_PHYREG_BCAST(pi, reg, val) \
{ \
		int core_idx = 0; \
		FOREACH_CORE(pi, core_idx) { \
			_PHY_REG_WRITE(pi, ACPHY_##reg(pi->pubpi->phy_rev) + \
				((core_idx) * PHY_REG_BANK_CORE1_OFFSET), (val)); \
		} \
}

/* helper macro for table driven writes */
#define ACPHY_REG(pi, regname)	ACPHY_##regname(pi->pubpi->phy_rev)

/* radio-specific macros */
#define RADIO_REG_2069X(pi, id, regnm, core)	RF##core##_##id##_##regnm(pi->pubpi->radiorev)
#define RADIO_PLLREG_2069X(pi, id, regnm, pll)	RFP##pll##_##id##_##regnm(pi->pubpi->radiorev)

#define RADIO_REG_20691(pi, regnm, core)	RADIO_REG_2069X(pi, 20691, regnm, 0)

#define RADIO_REG_20694(pi, regpfx, regnm, core) \
	((core == 0) ? regpfx##0##_##20694##_##regnm((pi)->pubpi->radiorev) : \
	  ((core == 1) ? regpfx##1##_##20694##_##regnm((pi)->pubpi->radiorev) : INVALID_ADDRESS))


#define RADIO_REG_20695(pi, regpfx, regnm, core) \
			((core == 0) ? (regpfx##0##_##20695##_##regnm((pi)->pubpi->radiorev)) \
					: INVALID_ADDRESS)
#define RADIO_REG_20695_FLD_MASK(pi, regnm, fldname) \
		RF_##20695##_##regnm##_##fldname##_MASK(pi->pubpi.radiorev)
#define RADIO_REG_20695_FLD_SHIFT(pi, regnm, fldname) \
		RF_##20695##_##regnm##_##fldname##_SHIFT(pi->pubpi.radiorev)
#if PHY_CORE_MAX == 1	/* Single PHY core chips */
#define RADIO_REG_20693(pi, regnm, core)	RADIO_REG_2069X(pi, 20693, regnm, 0)
#elif PHY_CORE_MAX == 2	/* Dual PHY core chips */
#define RADIO_REG_20693(pi, regnm, core)	\
	((core == 0) ? RADIO_REG_2069X(pi, 20693, regnm, 0) : \
	 RADIO_REG_2069X(pi, 20693, regnm, 1))
#elif PHY_CORE_MAX == 3 /* 3 PHY core chips */
#define RADIO_REG_20693(pi, regnm, core)	\
	((core == 0) ? RADIO_REG_2069X(pi, 20693, regnm, 0) : \
	((core == 1) ? RADIO_REG_2069X(pi, 20693, regnm, 1) : \
	 RADIO_REG_2069X(pi, 20693, regnm, 2)))
#else
#define RADIO_REG_20693(pi, regnm, core)	\
		((core == 0) ? RADIO_REG_2069X(pi, 20693, regnm, 0) : \
		((core == 1) ? RADIO_REG_2069X(pi, 20693, regnm, 1) : \
		((core == 2) ? RADIO_REG_2069X(pi, 20693, regnm, 2) : \
		 RADIO_REG_2069X(pi, 20693, regnm, 3))))
#endif	/* PHY_CORE_MAX */


#define RADIO_PLLREG_20693(pi, regnm, pll)	\
	((pll == 0) ? RADIO_PLLREG_2069X(pi, 20693, regnm, 0) : \
	 RADIO_PLLREG_2069X(pi, 20693, regnm, 1))

#define RADIO_ALLREG_20693(pi, regnm) RFX_20693_##regnm(pi->pubpi->radiorev)
#define RADIO_ALLPLLREG_20693(pi, regnm) RFPX_20693_##regnm(pi->pubpi->radiorev)

/* Should be probably renamed to RADIO_REG_TINY */
#define RADIO_REG(pi, regnm, core)	\
	((RADIOID_IS((pi)->pubpi->radioid, BCM20691_ID)) \
		? RADIO_REG_20691(pi, regnm, core) : \
	 (RADIOID_IS((pi)->pubpi->radioid, BCM20693_ID)) \
		? RADIO_REG_20693(pi, regnm, core) : INVALID_ADDRESS)

#define RADIO_REG_20696(pi, regnm, core) \
	((core == 0) ? RF##0##_##20696##_##regnm((pi)->pubpi->radiorev) : \
	((core == 1) ? RF##1##_##20696##_##regnm((pi)->pubpi->radiorev) : \
	((core == 2) ? RF##2##_##20696##_##regnm((pi)->pubpi->radiorev) : \
	((core == 3) ? RF##3##_##20696##_##regnm((pi)->pubpi->radiorev) : \
	  INVALID_ADDRESS))))

#define RADIO_PLLREG_20696(pi, regnm)  RFP0_20696_##regnm(pi->pubpi->radiorev)
#define RADIO_ALLREG_20696(pi, regnm)  RFX_20696_##regnm(pi->pubpi->radiorev)

#define RADIO_PLLREGC_FLD_20693(pi, regnm, pll, fldname, value) \
	{RADIO_PLLREG_20693(pi, regnm, pll), \
	 RF_20693_##regnm##_##fldname##_MASK(pi->pubpi->radiorev), \
	 ((value) << RF_20693_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev))}

#define MOD_RADIO_REG(pi, regpfx, regnm, fldname, value) \
	_MOD_RADIO_REG(pi, \
	              regpfx##_2069_##regnm, \
	              RF_2069_##regnm##_##fldname##_MASK, \
	              ((value) << RF_2069_##regnm##_##fldname##_SHIFT))

#define MOD_RADIO_REGC(pi, regnm, core, fldname, value) \
	_MOD_RADIO_REG(pi, \
	               RF_2069_##regnm(core), \
	               RF_2069_##regnm##_##fldname##_MASK, \
	               ((value) << RF_2069_##regnm##_##fldname##_SHIFT))

#define READ_RADIO_REG(pi, regpfx, regnm) \
	_READ_RADIO_REG(pi, regpfx##_2069_##regnm)

#define READ_RADIO_REG_20691(pi, regnm, core) \
	_READ_RADIO_REG(pi, RADIO_REG_20691(pi, regnm, core))

/* Tiny radio's */
#define READ_RADIO_REG_20693(pi, regnm, core) \
	_READ_RADIO_REG(pi, RADIO_REG_20693(pi, regnm, core))

#define READ_RADIO_PLLREG_20693(pi, regnm, pll) \
	_READ_RADIO_REG(pi, RADIO_PLLREG_20693(pi, regnm, pll))

#define READ_RADIO_REG_TINY(pi, regnm, core) \
	((RADIOID_IS((pi)->pubpi->radioid, BCM20691_ID)) ? READ_RADIO_REG_20691(pi, regnm, core) : \
	 (RADIOID_IS((pi)->pubpi->radioid, BCM20693_ID)) ? \
	 READ_RADIO_REG_20693(pi, regnm, core) : 0)

/* 28nm radio's */
#define READ_RADIO_REG_20694(pi, regpfx, regnm, core) \
	_READ_RADIO_REG(pi, RADIO_REG_20694(pi, regpfx, regnm, core))
#define READ_RADIO_REG_20695(pi, regpfx, regnm, core) \
	_READ_RADIO_REG(pi, RADIO_REG_20695(pi, regpfx, regnm, core))
#define READ_RADIO_REG_20696(pi, regnm, core) \
	_READ_RADIO_REG(pi, RADIO_REG_20696(pi, regnm, core))

#define READ_RADIO_PLLREG_20696(pi, regnm) \
	_READ_RADIO_REG(pi, RADIO_PLLREG_20696(pi, regnm))

#define WRITE_RADIO_PLLREG_20696(pi, regnm, val) \
	_WRITE_RADIO_REG(pi, RADIO_PLLREG_20696(pi, regnm), val)

#define WRITE_RADIO_ALLREG_20696(pi, regnm, val) \
	_WRITE_RADIO_REG(pi, RADIO_ALLREG_20696(pi, regnm), val)

#define READ_RADIO_REG_28NM(pi, regpfx, regnm, core) \
	((RADIOID_IS((pi)->pubpi->radioid, BCM20694_ID) ? \
	  READ_RADIO_REG_20694(pi, regpfx, regnm, core) : \
	 (RADIOID_IS((pi)->pubpi->radioid, BCM20695_ID) ? \
	  READ_RADIO_REG_20695(pi, regpfx, regnm, core) : \
	 (RADIOID_IS((pi)->pubpi->radioid, BCM20696_ID) ? \
	  READ_RADIO_REG_20696(pi, regnm, core) : \
	  0))))

#define READ_RADIO_REGC(pi, regpfx, regnm, core) \
	_READ_RADIO_REG(pi, regpfx##_2069_##regnm(core))

#define READ_RADIO_REGFLD(pi, regpfx, regnm, fldname) \
	((_READ_RADIO_REG(pi, regpfx##_2069_##regnm) & \
	              RF_2069_##regnm##_##fldname##_MASK) \
	              >> RF_2069_##regnm##_##fldname##_SHIFT)

#define READ_RADIO_REGFLDC(pi, regnmcr, regnm, fldname) \
	((_READ_RADIO_REG(pi, regnmcr) & \
	              RF_2069_##regnm##_##fldname##_MASK) \
	              >> RF_2069_##regnm##_##fldname##_SHIFT)

#define READ_RADIO_REGFLD_20691(pi, regnm, core, fldname) \
	((_READ_RADIO_REG(pi, RADIO_REG_20691(pi, regnm, core)) & \
		RF_20691_##regnm##_##fldname##_MASK(pi->pubpi->radiorev)) \
		>> RF_20691_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev))

#define READ_RADIO_REGFLD_20693(pi, regnm, core, fldname) \
	((_READ_RADIO_REG(pi, RADIO_REG_20693(pi, regnm, core)) & \
		RF_20693_##regnm##_##fldname##_MASK(pi->pubpi->radiorev)) \
		>> RF_20693_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev))

#define READ_RADIO_PLLREGFLD_20693(pi, regnm, pll, fldname) \
	((_READ_RADIO_REG(pi, RADIO_PLLREG_20693(pi, regnm, pll)) & \
		RF_20693_##regnm##_##fldname##_MASK(pi->pubpi->radiorev)) \
		>> RF_20693_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev))

#define READ_RADIO_REGFLD_20694(pi, regpfx, regnm, core, fldname) \
	((_READ_RADIO_REG(pi, RADIO_REG_20694(pi, regpfx, regnm, core)) & \
		RF_20694_##regnm##_##fldname##_MASK(pi->pubpi->radiorev)) \
		>> RF_20694_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev))

#define READ_RADIO_REGFLD_20695(pi, regpfx, regnm, core, fldname) \
	((_READ_RADIO_REG(pi, RADIO_REG_20695(pi, regpfx, regnm, core)) & \
		RF_20695_##regnm##_##fldname##_MASK(pi->pubpi->radiorev)) \
		>> RF_20695_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev))

#define READ_RADIO_REGFLD_20696(pi, regpfx, regnm, core, fldname) \
	((_READ_RADIO_REG(pi, RADIO_REG_20696(pi, regnm, core)) & \
		RF_20696_##regnm##_##fldname##_MASK(pi->pubpi->radiorev)) \
		>> RF_20696_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev))

#define READ_RADIO_REGFLD_TINY(pi, regnm, core, fldname) \
	((RADIOID_IS((pi)->pubpi->radioid, BCM20691_ID)) \
		? READ_RADIO_REGFLD_20691(pi, regnm, core, fldname) : \
	 (RADIOID_IS((pi)->pubpi->radioid, BCM20693_ID)) \
		? READ_RADIO_REGFLD_20693(pi, regnm, core, fldname) : 0)
#define READ_RADIO_REGFLD_28NM(pi, regpfx, regnm, core, fldname) \
	((RADIOID_IS((pi)->pubpi->radioid, BCM20695_ID)) \
		? READ_RADIO_REGFLD_20695(pi, regpfx, regnm, core, fldname) : 0) \

#define READ_RADIO_PLLREGFLD_20696(pi, regnm, fldname) \
	((_READ_RADIO_REG(pi, RADIO_PLLREG_20696(pi, regnm)) & \
		RF_20696_##regnm##_##fldname##_MASK(pi->pubpi->radiorev)) \
		>> RF_20696_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev))

#define MOD_RADIO_REG_2069X(pi, id, regnm, core, fldname, value) \
	_MOD_RADIO_REG(pi, \
		RADIO_REG_##id(pi, regnm, core), \
		RF_##id##_##regnm##_##fldname##_MASK(pi->pubpi->radiorev), \
		((value) << RF_##id##_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev)))

#define MOD_RADIO_PLLREG_2069X(pi, id, regnm, pll, fldname, value) \
	_MOD_RADIO_REG(pi, \
		RADIO_PLLREG_##id(pi, regnm, pll), \
		RF_##id##_##regnm##_##fldname##_MASK(pi->pubpi.radiorev), \
		((value) << RF_##id##_##regnm##_##fldname##_SHIFT(pi->pubpi.radiorev)))

#define MOD_RADIO_REG_20691(pi, regnm, core, fldname, value) \
	MOD_RADIO_REG_2069X(pi, 20691, regnm, core, fldname, value)

#define MOD_RADIO_REG_20693(pi, regnm, core, fldname, value) \
	MOD_RADIO_REG_2069X(pi, 20693, regnm, core, fldname, value)

#define MOD_RADIO_PLLREG_20693(pi, regnm, pll, fldname, value) \
	MOD_RADIO_PLLREG_2069X(pi, 20693, regnm, pll, fldname, value)

#define MOD_RADIO_PLLREG_20696(pi, regnm, fldname, value) \
	_MOD_RADIO_REG(pi, \
		RADIO_PLLREG_20696(pi, regnm), \
		RF_20696_##regnm##_##fldname##_MASK(pi->pubpi->radiorev), \
		((value) << RF_20696_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev)))

#define MOD_RADIO_ALLREG_20693(pi, regnm, fldname, value) \
	_MOD_RADIO_REG(pi, \
		RADIO_ALLREG_20693(pi, regnm), \
		RF_20693_##regnm##_##fldname##_MASK(pi->pubpi->radiorev), \
		((value) << RF_20693_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev)))

#define MOD_RADIO_ALLPLLREG_20693(pi, regnm, fldname, value) \
	_MOD_RADIO_REG(pi, \
		RADIO_ALLPLLREG_20693(pi, regnm), \
		RF_20693_##regnm##_##fldname##_MASK(pi->pubpi.radiorev), \
		((value) << RF_20693_##regnm##_##fldname##_SHIFT(pi->pubpi.radiorev)))

#define MOD_RADIO_REG_20694(pi, regpfx, regnm, core, fldname, value) \
	_MOD_RADIO_REG(pi, \
		RADIO_REG_20694(pi, regpfx, regnm, core), \
		RF_##20694##_##regnm##_##fldname##_MASK(pi->pubpi->radiorev), \
		((value) << RF_##20694##_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev)))

#define MOD_RADIO_REG_20695(pi, regpfx, regnm, core, fldname, value) \
	_MOD_RADIO_REG(pi, \
		RADIO_REG_20695(pi, regpfx, regnm, core), \
		RF_##20695##_##regnm##_##fldname##_MASK(pi->pubpi->radiorev), \
		((value) << RF_##20695##_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev)))
#define MOD_RADIO_REG_TINY(pi, regnm, core, fldname, value) \
	(RADIOID_IS((pi)->pubpi->radioid, BCM20691_ID)) \
		? MOD_RADIO_REG_20691(pi, regnm, core, fldname, value) : \
	(RADIOID_IS((pi)->pubpi->radioid, BCM20693_ID)) \
		? MOD_RADIO_REG_20693(pi, regnm, core, fldname, value) : BCM_REFERENCE(pi)
#define MOD_RADIO_REG_20695(pi, regpfx, regnm, core, fldname, value) \
	_MOD_RADIO_REG(pi, \
		RADIO_REG_20695(pi, regpfx, regnm, core), \
		RF_##20695##_##regnm##_##fldname##_MASK(pi->pubpi->radiorev), \
		((value) << RF_##20695##_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev)))
#define MOD_RADIO_REG_20696(pi, regnm, core, fldname, value) \
	_MOD_RADIO_REG(pi, \
		RADIO_REG_20696(pi, regnm, core), \
		RF_##20696##_##regnm##_##fldname##_MASK(pi->pubpi->radiorev), \
		((value) << RF_##20696##_##regnm##_##fldname##_SHIFT(pi->pubpi->radiorev)))

#define MOD_RADIO_REG_28NM(pi, regpfx, regnm, core, fldname, value) \
	((RADIOID_IS((pi)->pubpi->radioid, BCM20695_ID)) \
	? MOD_RADIO_REG_20695(pi, regpfx, regnm, core, fldname, value) : BCM_REFERENCE(pi))

#define WRITE_RADIO_REG_20695(pi, regpfx, regnm, core, value) \
	_WRITE_RADIO_REG(pi, RADIO_REG_20695(pi, regpfx, regnm, core), value)

#define WRITE_RADIO_REG_20694(pi, regpfx, regnm, core, value) \
	_WRITE_RADIO_REG(pi, RADIO_REG_20694(pi, regpfx, regnm, core), value)

#define WRITE_RADIO_REG_20696(pi, regnm, core, value) \
	_WRITE_RADIO_REG(pi, RADIO_REG_20696(pi, regnm, core), value)

#define WRITE_RADIO_REG_28NM(pi, regpfx, regnm, core, value) \
		((RADIOID_IS((pi)->pubpi->radioid, BCM20695_ID)) \
		? WRITE_RADIO_REG_20695(pi, regpfx, regnm, core, value) : BCM_REFERENCE(pi))

#define MOD_RADIO_REG_20695_MULTI_7(pi, regpfx, regnm, core, field1, value1, field2, value2, \
	field3, value3, field4, value4, \
	field5, value5, field6, value6, field7, value7) \
	_MOD_RADIO_REG(pi, \
		RADIO_REG_20695(pi, regpfx, regnm, core), \
		(RF_##20695##_##regnm##_##field1##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field2##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field3##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field4##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field5##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field6##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field7##_##MASK(pi->pubpi->radiorev)), \
		(((value1) << \
			RF_##20695##_##regnm##_##field1##_##SHIFT(pi->pubpi->radiorev)) |\
			((value2) << \
				RF_##20695##_##regnm##_##field2##_##SHIFT(pi->pubpi->radiorev)) |\
			((value3) << \
				RF_##20695##_##regnm##_##field3##_##SHIFT(pi->pubpi->radiorev)) |\
			((value4) << \
				RF_##20695##_##regnm##_##field4##_##SHIFT(pi->pubpi->radiorev)) |\
			((value5) << \
				RF_##20695##_##regnm##_##field5##_##SHIFT(pi->pubpi->radiorev)) |\
			((value6) << \
				RF_##20695##_##regnm##_##field6##_##SHIFT(pi->pubpi->radiorev)) |\
			((value7) << \
				RF_##20695##_##regnm##_##field7##_##SHIFT(pi->pubpi->radiorev))))

#define MOD_RADIO_REG_20695_MULTI_6(pi, regpfx, regnm, core, field1, value1, field2, value2, \
	field3, value3, field4, value4, \
	field5, value5, field6, value6) \
\
	_MOD_RADIO_REG(pi, \
		RADIO_REG_20695(pi, regpfx, regnm, core), \
		(RF_##20695##_##regnm##_##field1##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field2##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field3##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field4##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field5##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field6##_##MASK(pi->pubpi->radiorev)), \
		(((value1) << \
			RF_##20695##_##regnm##_##field1##_##SHIFT(pi->pubpi->radiorev)) |\
			((value2) << \
				RF_##20695##_##regnm##_##field2##_##SHIFT(pi->pubpi->radiorev)) |\
			((value3) << \
				RF_##20695##_##regnm##_##field3##_##SHIFT(pi->pubpi->radiorev)) |\
			((value4) << \
				RF_##20695##_##regnm##_##field4##_##SHIFT(pi->pubpi->radiorev)) |\
			((value5) << \
				RF_##20695##_##regnm##_##field5##_##SHIFT(pi->pubpi->radiorev)) |\
			((value6) << \
				RF_##20695##_##regnm##_##field6##_##SHIFT(pi->pubpi->radiorev))))

#define MOD_RADIO_REG_20695_MULTI_5(pi, regpfx, regnm, core, field1, value1, field2, value2, \
	field3, value3, field4, value4, \
	field5, value5) \
\
	_MOD_RADIO_REG(pi, \
		RADIO_REG_20695(pi, regpfx, regnm, core), \
		(RF_##20695##_##regnm##_##field1##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field2##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field3##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field4##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field5##_##MASK(pi->pubpi->radiorev)), \
		(((value1) << \
			RF_##20695##_##regnm##_##field1##_##SHIFT(pi->pubpi->radiorev)) |\
			((value2) << \
				RF_##20695##_##regnm##_##field2##_##SHIFT(pi->pubpi->radiorev)) |\
			((value3) << \
				RF_##20695##_##regnm##_##field3##_##SHIFT(pi->pubpi->radiorev)) |\
			((value4) <<  \
				RF_##20695##_##regnm##_##field4##_##SHIFT(pi->pubpi->radiorev)) |\
			((value5) << \
				RF_##20695##_##regnm##_##field5##_##SHIFT(pi->pubpi->radiorev))))

#define MOD_RADIO_REG_20695_MULTI_3(pi, regpfx, regnm, core, field1, value1, field2, value2, \
	field3, value3) \
\
	_MOD_RADIO_REG(pi, \
		RADIO_REG_20695(pi, regpfx, regnm, core), \
		(RF_##20695##_##regnm##_##field1##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field2##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field3##_##MASK(pi->pubpi->radiorev)), \
		(((value1) << \
			RF_##20695##_##regnm##_##field1##_##SHIFT(pi->pubpi->radiorev)) |\
			((value2) << \
				RF_##20695##_##regnm##_##field2##_##SHIFT(pi->pubpi->radiorev)) |\
			((value3) << \
				RF_##20695##_##regnm##_##field3##_##SHIFT(pi->pubpi->radiorev))))

#define MOD_RADIO_REG_20695_MULTI_2(pi, regpfx, regnm, core, field1, value1, field2, value2) \
	_MOD_RADIO_REG(pi, \
		RADIO_REG_20695(pi, regpfx, regnm, core), \
		(RF_##20695##_##regnm##_##field1##_##MASK(pi->pubpi->radiorev) |\
			RF_##20695##_##regnm##_##field2##_##MASK(pi->pubpi->radiorev)), \
		(((value1) << \
			RF_##20695##_##regnm##_##field1##_##SHIFT(pi->pubpi->radiorev)) |\
			((value2) << \
				RF_##20695##_##regnm##_##field2##_##SHIFT(pi->pubpi->radiorev))))
#define IS43602WLCSP ((CHIPID(pi->sh->chip) == BCM43602_CHIP_ID ||	\
		   CHIPID(pi->sh->chip) == BCM43602_CHIP_ID) &&	\
		  RADIOREV(pi->pubpi->radiorev) == 13)

/* 2069 iPA or ePA radio */
/* Check Minor Radio Revid */
#define ACRADIO_2069_EPA_IS(radio_rev_id) \
	((RADIO2069REV(radio_rev_id) == 2) || (RADIO2069REV(radio_rev_id) == 3) || \
	 (RADIO2069REV(radio_rev_id) == 4) || (RADIO2069REV(radio_rev_id) == 7) || \
	 (RADIO2069REV(radio_rev_id) ==  8) || (RADIO2069REV(radio_rev_id) == 18) || \
	 (RADIO2069REV(radio_rev_id) == 24) || (RADIO2069REV(radio_rev_id) == 26) || \
	 (RADIO2069REV(radio_rev_id) == 34) || (RADIO2069REV(radio_rev_id) == 36) || \
	 (RADIO2069REV(radio_rev_id) == 64) || (RADIO2069REV(radio_rev_id) == 13) || \
	 (RADIO2069REV(radio_rev_id) == 66))

/* 20691 iPA or ePA radio */
/* Check Minor Radio Revid */
#define ACRADIO_20691_EPA_IS(radio_rev_id) \
	((RADIO20691REV(radio_rev_id) == 13) || (RADIO20691REV(radio_rev_id) == 30))

#define ACPHY_DISABLE_STALL(pi)	MOD_PHYREG(pi, RxFeCtrl1, disable_stalls, 1)
#define ACPHY_ENABLE_STALL(pi, stall_val) MOD_PHYREG(pi, RxFeCtrl1, disable_stalls, stall_val)

/* Table driven register access for dongle memory optimizations */
#if (defined(BCMRADIOREV) || defined(BCMRADIO20691REV) || defined(BCMRADIOREV20693REV) \
	|| defined(BCMRADIOREV2096REV)) && defined(DONGLEBUILD) && !IS_MULTI_REV(ACCONF) && \
	defined(BCMCHIPID)
#define ACPHY_REG_LIST_START						\
	{ static const uint16 write_phy_reg_table[] = {
#define ACPHY_REG_LIST_EXECUTE(pi)					\
	};								\
	phy_utils_write_phyreg_array(pi, write_phy_reg_table,		\
	sizeof(write_phy_reg_table)/sizeof(write_phy_reg_table[0])); }

#define ACPHYREG_BCAST_ENTRY(pi, reg, val)				\
	PHY_REG_WRITE_RAW_ENTRY(ACPHY_REG(pi, reg) | ACPHY_REG_BROADCAST(pi), val)
#define ACPHY_DISABLE_STALL_ENTRY(pi)					\
	MOD_PHYREG_ENTRY(pi, RxFeCtrl1, disable_stalls, 1)

#define WRITE_PHYREG_ENTRY(pi, reg, value)				\
	PHY_REG_WRITE_RAW_ENTRY(ACPHY_REG(pi, reg), (value))
#define MOD_PHYREG_RAW_ENTRY(pi, reg, mask, value)			\
	PHY_REG_MOD_RAW_ENTRY(reg, mask, value)
#define MOD_PHYREG_ENTRY(pi, reg, field, value)				\
	PHY_REG_MOD_RAW_ENTRY(ACPHY_REG(pi, reg),			\
		ACPHY_##reg##_##field##_MASK(pi->pubpi->phy_rev),	\
		((value) << ACPHY_##reg##_##field##_##SHIFT(pi->pubpi->phy_rev)))
#define MOD_PHYREGCE_ENTRY(pi, reg, core, field, value)			\
	PHY_REG_MOD_RAW_ENTRY(ACPHYREGCE(pi, reg, core),		\
		ACPHY_REG_FIELD_MASKE(pi, reg, core, field),		\
		((value) << ACPHY_REG_FIELD_SHIFTE(pi, reg, core, field)))

#define MOD_RADIO_REGC_ENTRY(pi, regnm, core, fldname, value)		\
	RADIO_REG_MOD_ENTRY(RF_2069_##regnm(core),			\
		RF_2069_##regnm##_##fldname##_MASK,			\
		((value) << RF_2069_##regnm##_##fldname##_SHIFT))

#define MOD_RADIO_REG_ENTRY(pi, regpfx, regnm, fldname, value)		\
	RADIO_REG_MOD_ENTRY(regpfx##_2069_##regnm,			\
		RF_2069_##regnm##_##fldname##_MASK,			\
		((value) << RF_2069_##regnm##_##fldname##_SHIFT))
#define WRITE_RADIO_REG_ENTRY(pi, reg, val)				\
	RADIO_REG_WRITE_ENTRY(reg, val)

#define MOD_RADIO_REG_2069X_ENTRY(pi, id, regnm, core, fldname, value)	\
	RADIO_REG_MOD_ENTRY(RADIO_REG_##id(pi, regnm, core),		\
		RF_##id##_##regnm##_##fldname##_MASK(pi->pubpi.radiorev), \
		((value) << RF_##id##_##regnm##_##fldname##_SHIFT(pi->pubpi.radiorev)))
#define MOD_RADIO_REG_20691_ENTRY(pi, regnm, core, fldname, value)	\
	MOD_RADIO_REG_2069X_ENTRY(pi, 20691, regnm, core, fldname, value)
#define MOD_RADIO_REG_20693_ENTRY(pi, regnm, core, fldname, value)	\
	MOD_RADIO_REG_2069X_ENTRY(pi, 20693, regnm, core, fldname, value)

#define MOD_RADIO_REG_20694_ENTRY(pi, regpfx, regnm, core, fldname, value) \
	RADIO_REG_MOD_ENTRY(RADIO_REG_20694(pi, regpfx, regnm, core),		\
		RF_##20694##_##regnm##_##fldname##_MASK(pi->pubpi.radiorev), \
		((value) << RF_##20694##_##regnm##_##fldname##_SHIFT(pi->pubpi.radiorev)))

#define MOD_RADIO_REG_20695_ENTRY(pi, regpfx, regnm, core, fldname, value) \
	RADIO_REG_MOD_ENTRY(RADIO_REG_20695(pi, regpfx, regnm, core),		\
		RF_##20695##_##regnm##_##fldname##_MASK(pi->pubpi.radiorev), \
		((value) << RF_##20695##_##regnm##_##fldname##_SHIFT(pi->pubpi.radiorev)))

#define WRITE_RADIO_REG_20695_ENTRY(pi, regpfx, regnm, core, value) \
		 RADIO_REG_WRITE_ENTRY(RADIO_REG_20695(pi, regpfx, regnm, core), value)
#if BCMRADIOID == BCM20695_ID
	#define MOD_RADIO_REG_28NM_ENTRY(pi, regpfx, regnm, core, fldname, value)	\
		MOD_RADIO_REG_20695_ENTRY(pi, regpfx, regnm, core, fldname, value)
	#define WRITE_RADIO_REG_28NM_ENTRY(pi, regpfx, regnm, core, value)	\
		WRITE_RADIO_REG_20695_ENTRY(pi, regpfx, regnm, core, value)
#else /* BCMRADIOID Just adding if else logic to add other radio ids following 28NM later */
	#define MOD_RADIO_REG_28NM_ENTRY(pi, regpfx, regnm, core, fldname, value)	\
		MOD_RADIO_REG_20695_ENTRY(pi, regpfx, regnm, core, fldname, value)
	#define WRITE_RADIO_REG_28NM_ENTRY(pi, regpfx, regnm, core, value)	\
		WRITE_RADIO_REG_20695_ENTRY(pi, regpfx, regnm, core, value)
#endif /* BCMRADIOID */
#if BCMRADIOID == BCM20691_ID
	#define MOD_RADIO_REG_TINY_ENTRY(pi, regnm, core, fldname, value)	\
		MOD_RADIO_REG_20691_ENTRY(pi, regnm, core, fldname, value)
#elif BCMRADIOID == BCM20693_ID
	#define MOD_RADIO_REG_TINY_ENTRY(pi, regnm, core, fldname, value)	\
		MOD_RADIO_REG_20693_ENTRY(pi, regnm, core, fldname, value)
#else /* BCMRADIOID */
	#define MOD_RADIO_REG_TINY_ENTRY(pi, regnm, core, fldname, value)	\
		MOD_RADIO_REG_20691_ENTRY(pi, regnm, core, fldname, value)
#endif /* BCMRADIOID */

#else /* RADIOREV && DONGLEBUILD && !IS_MULTI_REV && BCMCHIPID */
#define ACPHY_REG_LIST_START
#define ACPHY_REG_LIST_EXECUTE(pi)
#define ACPHYREG_BCAST_ENTRY(pi, reg, val)				\
	ACPHYREG_BCAST(pi, reg, val);
#define ACPHY_DISABLE_STALL_ENTRY(pi)					\
	ACPHY_DISABLE_STALL(pi);
#define WRITE_PHYREG_ENTRY(pi, reg, value)				\
	WRITE_PHYREG(pi, reg, value);
#define MOD_PHYREG_RAW_ENTRY(pi, reg, mask, value)			\
	_PHY_REG_MOD(pi, reg, mask, value);
#define MOD_PHYREG_ENTRY(pi, reg, field, value)				\
	MOD_PHYREG(pi, reg, field, value);
#define MOD_PHYREGCE_ENTRY(pi, reg, core, field, value)			\
	MOD_PHYREGCE(pi, reg, core, field, value);
#define MOD_RADIO_REGC_ENTRY(pi, regnm, core, fldname, value)		\
	MOD_RADIO_REGC(pi, regnm, core, fldname, value);
#define MOD_RADIO_REG_ENTRY(pi, regpfx, regnm, fldname, value)		\
	MOD_RADIO_REG(pi, regpfx, regnm, fldname, value);
#define WRITE_RADIO_REG_ENTRY(pi, reg, val)				\
	phy_utils_write_radioreg(pi, reg, val);
#define MOD_RADIO_REG_20691_ENTRY(pi, regnm, core, fldname, value)	\
	MOD_RADIO_REG_20691(pi, regnm, core, fldname, value);
#define MOD_RADIO_REG_20693_ENTRY(pi, regnm, core, fldname, value)	\
	MOD_RADIO_REG_20693(pi, regnm, core, fldname, value);
#define MOD_RADIO_REG_TINY_ENTRY(pi, regnm, core, fldname, value)	\
	MOD_RADIO_REG_TINY(pi, regnm, core, fldname, value);
#define MOD_RADIO_REG_20694_ENTRY(pi, regpfx, regnm, core, fldname, value) \
	MOD_RADIO_REG_20694(pi, regpfx, regnm, core, fldname, value);
#define MOD_RADIO_REG_20695_ENTRY(pi, regpfx, regnm, core, fldname, value) \
	MOD_RADIO_REG_20695(pi, regpfx, regnm, core, fldname, value);
#define MOD_RADIO_REG_28NM_ENTRY(pi, regpfx, regnm, core, fldname, value)	\
		MOD_RADIO_REG_28NM(pi, regpfx, regnm, core, fldname, value);

#endif /* RADIOREV && DONGLEBUILD && !IS_MULTI_REV && BCMCHIPID */
/* Force use of ACPHY specific REG_LIST_xxx macros instead of generic ones */
#undef PHY_REG_LIST_START
#undef PHY_REG_LIST_EXECUTE


/* *********************************************** */
/* The following definitions shared between chanmgr, tpc, rxgcrs ... */
/* *********************************************** */
#ifndef D11AC_IOTYPES
/* 80 MHz support is included if D11AC_IOTYPES is defined */
#define CHSPEC_IS80(chspec) (0)
#define WL_CHANSPEC_CTL_SB_LL (0)
#define WL_CHANSPEC_CTL_SB_LU (0)
#define WL_CHANSPEC_CTL_SB_UL (0)
#define WL_CHANSPEC_CTL_SB_UU (0)
#endif /* D11AC_IOTYPES */

#define HWACI_OFDM_DESENSE	(ACPHY_LO_NF_MODE_ELNA_TINY(pi) ? 18 : 9)
#define HWACI_BPHY_DESENSE	(ACPHY_LO_NF_MODE_ELNA_TINY(pi) ? 21 : 12)
#define HWACI_LNA1_DESENSE		1
#define HWACI_LNA2_DESENSE      2
#define HWACI_CLIP_INIT_DESENSE	(ACPHY_LO_NF_MODE_ELNA_TINY(pi) ? 9 : 12)
#define HWACI_CLIP_HIGH_DESENSE (ACPHY_LO_NF_MODE_ELNA_TINY(pi) ? 12 : 0)
#define HWACI_CLIP_MED_DESENSE	(ACPHY_LO_NF_MODE_ELNA_TINY(pi) ? 12 : 0)
#define HWACI_CLIP_LO_DESENSE	(ACPHY_LO_NF_MODE_ELNA_TINY(pi) ? 9 : 0)

/* ********************************************************************* */
/* The following definitions shared between attach, radio, rxiqcal and phytbl ... */
/* ********************************************************************* */

/* On a monolithic driver, receive status is always converted to host byte order early
 * in the receive path (wlc_bmac_recv()). On a split driver, receive status is stays
 * in little endian on the on-chip processor, but is converted to host endian early
 * in the host driver receive path (wlc_recv()). We assume all on-chip processors
 * are little endian, and therefore, these macros do not require ltoh conversion.
 * ltoh conversion would be harmful on a monolithic driver running on a big endian
 * host, where the conversion has already been done. Complain if there is ever
 * a big-endian on-chip processor.
 */

#ifndef ACPHY_HACK_PWR_STATUS
#define ACPHY_HACK_PWR_STATUS(rxs)	(((rxs)->PhyRxStatus_1 & PRXS1_ACPHY_BIT_HACK) >> 3)
#endif
/* Get Rx power on core 0 */
#ifndef ACPHY_RXPWR_ANT0
#define ACPHY_RXPWR_ANT0(rxs)	(((rxs)->PhyRxStatus_2 & PRXS2_ACPHY_RXPWR_ANT0) >> 8)
#endif
/* Get Rx power on core 1 */
#ifndef ACPHY_RXPWR_ANT1
#define ACPHY_RXPWR_ANT1(rxs)	((rxs)->PhyRxStatus_3 & PRXS3_ACPHY_RXPWR_ANT1)
#endif
/* Get Rx power on core 2 */
#ifndef ACPHY_RXPWR_ANT2
#define ACPHY_RXPWR_ANT2(rxs)	(((rxs)->PhyRxStatus_3 & PRXS3_ACPHY_RXPWR_ANT2) >> 8)
#endif
/* Get Rx power on core 3 */
#ifndef ACPHY_RXPWR_ANT3
#define ACPHY_RXPWR_ANT3(rxs)	((rxs)->PhyRxStatus_4 & PRXS4_ACPHY_RXPWR_ANT3)
#endif

#define ACPHY_VCO_2P5V	1
#define ACPHY_VCO_1P35V	0

/* Macro to enable clock gating changes in different cores */
#define SAMPLE_SYNC_CLK_BIT 	17

#define ACPHY_FEMCTRL_ACTIVE(pi)			\
	((ACMAJORREV_0((pi)->pubpi->phy_rev) || \
	  ACMAJORREV_1((pi)->pubpi->phy_rev) || \
	  ACMAJORREV_2((pi)->pubpi->phy_rev) || \
	  ACMAJORREV_5((pi)->pubpi->phy_rev) || \
	  ACMAJORREV_32((pi)->pubpi->phy_rev) || \
	  ACMAJORREV_33((pi)->pubpi->phy_rev) || \
	  ACMAJORREV_37((pi)->pubpi->phy_rev) || \
	  ACMAJORREV_40((pi)->pubpi->phy_rev) || \
	  ACMAJORREV_36((pi)->pubpi->phy_rev))	\
		? ((BF3_FEMTBL_FROM_NVRAM((pi)->u.pi_acphy)) == 0) : 0)
#define ACPHY_SWCTRLMAP4_EN(pi)                 \
		((ACMAJORREV_32((pi)->pubpi->phy_rev) || \
		 ACMAJORREV_33((pi)->pubpi->phy_rev) || \
		 ACMAJORREV_37((pi)->pubpi->phy_rev) || \
		 IS_4364_1x1(pi) || IS_4364_3x3(pi))			\
		 ? (pi)->u.pi_acphy->sromi->swctrlmap4->enable : 0)
/* ************************************************ */
/* Makefile driven Board flags and FemCtrl settings */
/* ************************************************ */

#ifdef FEMCTRL
#define BFCTL(x)			FEMCTRL
#else
#define BFCTL(x)			((x)->sromi->femctrl)
#endif /* FEMCTRL */

#ifdef BOARD_FLAGS
#define BF_ELNA_2G(x)			((BOARD_FLAGS & BFL_SROM11_EXTLNA) != 0)
#define BF_ELNA_5G(x)			((BOARD_FLAGS & BFL_SROM11_EXTLNA_5GHz) != 0)
#define BF_SROM11_BTCOEX(x)		((BOARD_FLAGS & BFL_SROM11_BTCOEX) != 0)
#define BF_SROM11_GAINBOOSTA01(x)	((BOARD_FLAGS & BFL_SROM11_GAINBOOSTA01) != 0)
#else
#define BF_ELNA_2G(x)			((x)->sromi->elna2g_present)
#define BF_ELNA_5G(x)			((x)->sromi->elna5g_present)
#define BF_SROM11_BTCOEX(x)		((x)->sromi->bt_coex)
#define BF_SROM11_GAINBOOSTA01(x)	((x)->sromi->gainboosta01)
#endif /* BOARD_FLAGS */

#ifdef BOARD_FLAGS2
#define BF2_SROM11_APLL_WAR(x)		((BOARD_FLAGS2 & BFL2_SROM11_APLL_WAR) != 0)
#define BF2_2G_SPUR_WAR(x)		((BOARD_FLAGS2 & BFL2_2G_SPUR_WAR) != 0)
#define BF2_DAC_SPUR_IMPROVEMENT(x)	((BOARD_FLAGS2 & BFL2_DAC_SPUR_IMPROVEMENT) != 0)
#else
#define BF2_SROM11_APLL_WAR(x)		((x)->sromi->rfpll_5g)
#define BF2_2G_SPUR_WAR(x)		((x)->sromi->spur_war_enb_2g)
#define BF2_DAC_SPUR_IMPROVEMENT(x)	((x)->sromi->dac_spur_improve)
#endif /* BOARD_FLAGS2 */

#ifdef BOARD_FLAGS3
#define BF3_FEMCTRL_SUB(x)		(BOARD_FLAGS3 & BFL3_FEMCTRL_SUB)
#define BF3_AGC_CFG_2G(x)		((BOARD_FLAGS3 & BFL3_AGC_CFG_2G) != 0)
#define BF3_AGC_CFG_5G(x)		((BOARD_FLAGS3 & BFL3_AGC_CFG_5G) != 0)
#define BF3_5G_SPUR_WAR(x)		((BOARD_FLAGS3 & BFL3_5G_SPUR_WAR) != 0)
#define BF3_RCAL_WAR(x)			((BOARD_FLAGS3 & BFL3_RCAL_WAR) != 0)
#define BF3_RCAL_OTP_VAL_EN(x)		((BOARD_FLAGS3 & BFL3_RCAL_OTP_VAL_EN) != 0)
#define BF3_BBPLL_SPR_MODE_DIS(x)	((BOARD_FLAGS3 & BFL3_BBPLL_SPR_MODE_DIS) != 0)
#define BF3_VLIN_EN_FROM_NVRAM(x)	(0)
#define BF3_PPR_BIT_EXT(x) \
	((BOARD_FLAGS3 & BFL3_PPR_BIT_EXT) >> BFL3_PPR_BIT_EXT_SHIFT)
#define BF3_TXGAINTBLID(x) \
	((BOARD_FLAGS3 & BFL3_TXGAINTBLID) >> BFL3_TXGAINTBLID_SHIFT)
#define BF3_TSSI_DIV_WAR(x) \
	((BOARD_FLAGS3 & BFL3_TSSI_DIV_WAR) >> BFL3_TSSI_DIV_WAR_SHIFT)
#define BF3_2GTXGAINTBL_BLANK(x) \
	((BOARD_FLAGS3 & BFL3_2GTXGAINTBL_BLANK) >> BFL3_2GTXGAINTBL_BLANK_SHIFT)
#define BF3_5GTXGAINTBL_BLANK(x) \
	((BOARD_FLAGS3 & BFL3_5GTXGAINTBL_BLANK) >>	BFL3_5GTXGAINTBL_BLANK_SHIFT)
#define BF3_PHASETRACK_MAX_ALPHABETA(x) \
	((BOARD_FLAGS3 & BFL3_PHASETRACK_MAX_ALPHABETA) >> BFL3_PHASETRACK_MAX_ALPHABETA_SHIFT)
#define BF3_LTECOEX_GAINTBL_EN(x) \
	((BOARD_FLAGS3 & BFL3_LTECOEX_GAINTBL_EN) >> BFL3_LTECOEX_GAINTBL_EN_SHIFT)
#define BF3_ACPHY_LPMODE_2G(x) \
	((BOARD_FLAGS3 & BFL3_ACPHY_LPMODE_2G) >> BFL3_ACPHY_LPMODE_2G_SHIFT)
#define BF3_ACPHY_LPMODE_5G(x) \
	((BOARD_FLAGS3 & BFL3_ACPHY_LPMODE_5G) >> BFL3_ACPHY_LPMODE_5G_SHIFT)
#define BF3_FEMTBL_FROM_NVRAM(x) \
	((BOARD_FLAGS3 & BFL3_FEMTBL_FROM_NVRAM) >> BFL3_FEMTBL_FROM_NVRAM_SHIFT)
#define BF3_AVVMID_FROM_NVRAM(x) \
	((BOARD_FLAGS3 & BFL3_AVVMID_FROM_NVRAM) >> BFL3_AVVMID_FROM_NVRAM_SHIFT)
#define BF3_RSDB_1x1_BOARD(x) \
	((BOARD_FLAGS3 & BFL3_1X1_RSDB_ANT) >> BFL3_1X1_RSDB_ANT_SHIFT)
#else
#define BF3_FEMCTRL_SUB(x)			((x)->sromi->femctrl_sub)
#define BF3_AGC_CFG_2G(x)			((x)->sromi->agc_cfg_2g)
#define BF3_AGC_CFG_5G(x)			((x)->sromi->agc_cfg_5g)
#define BF3_5G_SPUR_WAR(x)			((x)->sromi->spur_war_enb_5g)
#define BF3_RCAL_WAR(x)				((x)->sromi->rcal_war)
#define BF3_RCAL_OTP_VAL_EN(x)			((x)->sromi->rcal_otp_val_en)
#define BF3_PPR_BIT_EXT(x)			((x)->sromi->ppr_bit_ext)
#define BF3_TXGAINTBLID(x)			((x)->sromi->txgaintbl_id)
#define BF3_BBPLL_SPR_MODE_DIS(x)		((x)->sromi->bbpll_spr_modes_dis)
#define BF3_VLIN_EN_FROM_NVRAM(x)		((x)->sromi->vlin_en_from_nvram)
#define BF3_TSSI_DIV_WAR(x)			((x)->sromi->tssi_div_war)
#define BF3_2GTXGAINTBL_BLANK(x)		((x)->sromi->txgaintbl2g_blank)
#define BF3_5GTXGAINTBL_BLANK(x)		((x)->sromi->txgaintbl5g_blank)
#define BF3_PHASETRACK_MAX_ALPHABETA(x)		((x)->sromi->phasetrack_max_alphabeta)
#define BF3_LTECOEX_GAINTBL_EN(x)		((x)->sromi->ltecoex_gaintbl_en)
#define BF3_ACPHY_LPMODE_2G(x)			((x)->sromi->lpmode_2g)
#define BF3_ACPHY_LPMODE_5G(x)			((x)->sromi->lpmode_5g)
#define BF3_FEMTBL_FROM_NVRAM(x)		((x)->sromi->femctrl_from_nvram)
#define BF3_AVVMID_FROM_NVRAM(x)		((x)->sromi->avvmid_from_nvram)
#define BF3_RSDB_1x1_BOARD(x)			((x)->sromi->rsdb_1x1_board)
#endif /* BOARD_FLAGS3 */


/* ************************************************ */
/* The following are utility structs                */
/* ************************************************ */

/* AddressDataPair_PerCore */
typedef struct {
	uint16 addr[PHY_CORE_MAX];
	uint16 data[PHY_CORE_MAX];
} ad_t;

/* TxPwrCtrl_Multi_Mode<core>.txPwrIndexLimit setting: */
/* Format: <2gcck> <2gofdm> <5gLL> <5gLH> <5gHL> <5gHH> */
#define USE_OOB_GAINT(x)			((x)->u.pi_acphy->sromi->oob_gaint)
#define NUM_TXPWRINDEX_LIM 6
typedef enum {
	ACPHY_TXPWRINDEX_LIM_2G_CCK = 0,
	ACPHY_TXPWRINDEX_LIM_2G_OFDM,
	ACPHY_TXPWRINDEX_LIM_5G_LL,
	ACPHY_TXPWRINDEX_LIM_5G_LH,
	ACPHY_TXPWRINDEX_LIM_5G_HL,
	ACPHY_TXPWRINDEX_LIM_5G_HH
} acphy_txpwrindex_lim_opt_t;

/* ************************************************ */
/* The following definitions used by phy_info_acphy */
/* ************************************************ */

typedef struct {
	int8 rssi_corr_normal[PHY_CORE_MAX][ACPHY_NUM_BW_2G];
	int8 rssi_corr_normal_5g[PHY_CORE_MAX][ACPHY_RSSIOFFSET_NVRAM_PARAMS][ACPHY_NUM_BW];
} acphy_nvram_rssioffset_t;

typedef struct {
	uint32 swctrlmap_2g[ACPHY_SWCTRL_NVRAM_PARAMS];
	uint32 swctrlmapext_2g[ACPHY_SWCTRL_NVRAM_PARAMS];
	uint32 swctrlmap_5g[ACPHY_SWCTRL_NVRAM_PARAMS];
	uint32 swctrlmapext_5g[ACPHY_SWCTRL_NVRAM_PARAMS];
	int8 txswctrlmap_2g;
	uint16 txswctrlmap_2g_mask;
	int8 txswctrlmap_5g;
} acphy_nvram_femctrl_t;

typedef struct {
	uint32 map_2g[DUAL_MAC_SLICES][PHY_CORE_MAX];
	uint32 map_5g[DUAL_MAC_SLICES][PHY_CORE_MAX];
} acphy_nvram_femctrl_clb_t;

typedef struct {
	int8 rssi_corr_normal[PHY_CORE_MAX][ACPHY_NUM_BW_2G];
	int8 rssi_corr_normal_5g[PHY_CORE_MAX][ACPHY_RSSIOFFSET_NVRAM_PARAMS][ACPHY_NUM_BW];
	int8 rssi_corr_gain_delta_2g[PHY_CORE_MAX][ACPHY_GAIN_DELTA_2G_PARAMS][ACPHY_NUM_BW_2G];
	int8 rssi_corr_gain_delta_2g_sub[PHY_CORE_MAX][ACPHY_GAIN_DELTA_2G_PARAMS_EXT]
	[ACPHY_NUM_BW_2G][CH_2G_GROUP_NEW];
	int8 rssi_corr_gain_delta_5g[PHY_CORE_MAX][ACPHY_GAIN_DELTA_5G_PARAMS][ACPHY_NUM_BW]
	[CH_5G_4BAND];
	int8 rssi_corr_gain_delta_5g_sub[PHY_CORE_MAX][ACPHY_GAIN_DELTA_5G_PARAMS_EXT][ACPHY_NUM_BW]
	[CH_5G_4BAND];
	int8 rssi_tr_offset;
} acphy_rssioffset_t;

typedef struct {
	uint8 enable;
	uint8 bitwidth8;
	uint8 bitwidth10_ext;
	uint8 misc_usage;
	uint16 tx2g[PHY_CORE_MAX];
	uint16 rx2g[PHY_CORE_MAX];
	uint16 rxbyp2g[PHY_CORE_MAX];
	uint16 misc2g[PHY_CORE_MAX];
	uint16 tx5g[PHY_CORE_MAX];
	uint16 rx5g[PHY_CORE_MAX];
	uint16 rxbyp5g[PHY_CORE_MAX];
	uint16 misc5g[PHY_CORE_MAX];
} acphy_swctrlmap4_t;

typedef struct {
	uint16 femctrlmask_2g, femctrlmask_5g;
	acphy_nvram_femctrl_t nvram_femctrl;
	acphy_rssioffset_t  rssioffset;
	uint8 rssi_cal_freq_grp[14];
	acphy_fem_rxgains_t femrx_2g[PHY_CORE_MAX];
	acphy_fem_rxgains_t femrx_5g[PHY_CORE_MAX];
	acphy_fem_rxgains_t femrx_5gm[PHY_CORE_MAX];
	acphy_fem_rxgains_t femrx_5gh[PHY_CORE_MAX];
	int16 rxgain_tempadj_2g;
	int16 rxgain_tempadj_5gl;
	int16 rxgain_tempadj_5gml;
	int16 rxgain_tempadj_5gmu;
	int16 rxgain_tempadj_5gh;
	int32 ed_thresh2g;
	int32 ed_thresh5g;
	int32 ed_thresh_default;
	uint8 femctrl;
	bool elna2g_present, elna5g_present;
	uint8 gainboosta01;
	uint8 bt_coex;
	uint8 rfpll_5g;
	uint8 spur_war_enb_2g;
	uint8 dac_spur_improve;
	uint8 femctrl_sub;
	uint8 agc_cfg_2g;
	uint8 agc_cfg_5g;
	uint8 spur_war_enb_5g;
	uint8 rcal_war;
	uint8 txgaintbl_id;
	uint8 ppr_bit_ext;
	uint8 rcal_otp_val_en;
	uint8 bbpll_spr_modes_dis;
	uint8 vlin_en_from_nvram;
	uint8 tssi_div_war;
	uint8 txgaintbl2g_blank;
	uint8 txgaintbl5g_blank;
	uint8 phasetrack_max_alphabeta;
	uint8 ltecoex_gaintbl_en;
	uint8 lpmode_2g;
	uint8 lpmode_5g;
	uint8 femctrl_from_nvram;
	uint8 avvmid_from_nvram;
	uint8 rsdb_1x1_board;
	acphy_swctrlmap4_t *swctrlmap4;
	uint32 femctrl_init_val_2g;
	uint32 femctrl_init_val_5g;
	uint32 dot11b_opts;
	uint8  tiny_maxrxgain[3];
	bool gainctrlsph;
	uint8 avvmid_set_from_nvram[PHY_MAX_CORES][NUM_SUBBANDS_FOR_AVVMID][2];
	uint8 oob_gaint;
	acphy_nvram_femctrl_clb_t nvram_femctrl_clb;
} acphy_srom_info_t;

typedef struct acphy_lpfCT_phyregs {
	bool   is_orig;
	uint16 RfctrlOverrideLpfCT[PHY_CORE_MAX];
	uint16 RfctrlCoreLpfCT[PHY_CORE_MAX];
} acphy_lpfCT_phyregs_t;

struct phy_param_info {
	uint8	mfcrs_th_bw20;
	uint8 	mfcrs_th_bw40;
	uint8 	mfcrs_th_bw80;
	uint8 	accrs_th_bw20;
	uint8 	accrs_th_bw40;
	uint8 	accrs_th_bw80;

	/* reusable register cache infra */
	uint16	reg_cache_id;
	uint16	reg_cache_depth;
	ad_t	*reg_cache;
	uint16	reg_sz;

	uint16	phy_reg_cache_id;
	uint16	phy_reg_cache_depth;
	ad_t	*phy_reg_cache;
	uint16	phy_reg_sz;
};

/* The PHY Information for AC PHY structure definition */
struct phy_info_acphy
{
	/* ************************************************************************************ */

	phy_info_t		*pi;
	phy_ac_ana_info_t	*anai;
	phy_ac_btcx_info_t	*btcxi;
	phy_ac_cache_info_t	*cachei;
	phy_ac_calmgr_info_t	*calmgri;
	phy_ac_chanmgr_info_t	*chanmgri;
	phy_ac_et_info_t	*eti;
	phy_ac_fcbs_info_t	*fcbsi;
	phy_ac_lpc_info_t	*lpci;
	phy_ac_misc_info_t	*misci;
	phy_ac_noise_info_t	*noisei;
	phy_ac_papdcal_info_t	*papdcali;
	phy_ac_radar_info_t	*radari;
	phy_ac_radio_info_t	*radioi;
	phy_ac_rxgcrs_info_t	*rxgcrsi;
	phy_ac_rssi_info_t	*rssii;
	phy_ac_rxiqcal_info_t	*rxiqcali;
	phy_ac_rxspur_info_t	*rxspuri;
	phy_ac_samp_info_t	*sampi;
	phy_ac_tbl_info_t	*tbli;
	phy_ac_tpc_info_t	*tpci;
	phy_ac_txpwrcap_info_t	*txpwrcapi;
	phy_ac_antdiv_info_t	*antdivi;
	phy_ac_temp_info_t	*tempi;
	phy_ac_txiqlocal_info_t	*txiqlocali;
	phy_ac_vcocal_info_t	*vcocali;
	phy_ac_tssical_info_t	*tssicali;
	phy_ac_dsi_info_t	*dsii;
	phy_ac_mu_info_t	*mui;
	phy_ac_dbg_info_t	*dbgi;
	phy_ac_dccal_info_t *dccali;
	phy_ac_tof_info_t	*tofi;
	phy_ac_hirssi_info_t	*hirssii;
	phy_ac_ocl_info_t *ocli;
	phy_ac_hecap_info_t *hecapi;

	/* ************************************************************************************ */
	/* [PHY_REARCH] Do not add any variables here. Add them to the individual modules */
	/* ************************************************************************************ */

	acphy_rxcal_phyregs_t	*ac_rxcal_phyregs_orig;
	acphy_srom_info_t	*sromi;
	acphy_lpfCT_phyregs_t *ac_lpfCT_phyregs_orig;

	/* ************************************************************************************ */

	chan_info_tx_farrow(*tx_farrow)[ACPHY_NUM_CHANS];
	chan_info_rx_farrow(*rx_farrow)[ACPHY_NUM_CHANS];

	/* ************************************************************************************ */

	int8	txpwrindex[PHY_CORE_MAX];		/* index if hwpwrctrl if OFF */
	int8	phy_noise_all_core[PHY_CORE_MAX];	/* noise power in dB for all cores */
	int8	phy_noise_in_crs_min[PHY_CORE_MAX];	/* noise power in dB for all cores */
	int8	phy_noise_pwr_array[PHY_SIZE_NOISE_ARRAY][PHY_CORE_MAX];
	uint8	acphy_txpwr_idx_2G[PHY_CORE_MAX]; 	/* txpwr index for 2G band */
	uint8	acphy_txpwr_idx_5G[PHY_CORE_MAX]; 	/* txpwr index for 2G band */
	uint8   core_freq_mapping[PHY_CORE_MAX];

	/* ************************************************************************************ */

	uint8	dac_mode;
	uint16	deaf_count;
	int8 	phy_noise_counter;		/* Dummy variable for noise averaging */
	uint8 	phy_debug_crscal_counter;
	uint8 	phy_debug_crscal_channel;
	uint32	phy_caps;		/* Capabilities queried from the registers */
	uint32	phy_caps1;		/* Additional Capabilities queried from the registers */
	bool	init;
	uint32	curr_bw;
	uint8	radar_cal_active;		/* to mask radar detect during cal's tone-play */
	bool	trigger_crsmin_cal;

	/* ************************************************************************************ */
	/* ************************************************************************************ */
	uint8	band2g_init_done;
	uint8	band5g_init_done;

	/* ************************************************************************************ */
	/* ************************************************************************************ */
	uint8	curr_subband;
	bool	acphy_papd_kill_switch_en;	/* indicate if lna kill switch is enabled */
	bool	acphy_force_papd_cal;
	uint	acphy_papd_last_cal;		/* time of last papd cal */
	uint32	acphy_papd_recal_counter;
	bool	acphy_papdcomp;
	uint8	txpwrindex_hw_save_cck[PHY_CORE_MAX]; /* txpwr start index for cck hwpwrctrl */
	bool	logenmode43;
	int8	cckfilttype;
	int8	ofdm_filt;
	int8	ofdm_filt_2g;
	bool	limit_desense_on_rssi;

	/* ************************************************************************************ */

	uint8	papdmode;
	bool	poll_adc_WAR;
	uint16	rfldo;
	uint8	acphy_force_lpvco_2G;
	uint8 	acphy_lp_status;
	uint8 	acphy_4335_radio_pd_status;
	void	*chan_tuning;
	uint32	chan_tuning_tbl_len;
	int8	pa_mode;			/* Modes: High Efficiency, High Linearity */
	bool	mdgain_trtx_allowed;
	bool	rxgaincal_rssical;		/* 0 = rxgain error cal and 1 = RSSI error cal */
	bool	rssi_cal_rev;			/* 0 = OLD ad 1 = NEW */
	uint16	*gaintbl_2g;
	uint16 	*gaintbl_5g;
	bool	hw_aci_status;
	uint16	phy_minor_rev;
	uint8	CCTrace;			/* Chanspec Call Trace */
	int	fc; 				/* Center Freq */

	/* ------------------------------------------------------------------------------------ */
	/*                        Variables in acphy info (with cflags)                         */
	/* ------------------------------------------------------------------------------------ */

	/* #ifdef WL_PROXDETECT */
	bool	tof_active;
	bool	tof_smth_forced;
	/* #endif */

	/* ************************************************************************************ */
	/* [PHY_REARCH] Do not add any variables here. Add them to the individual modules */
	/* ************************************************************************************ */


	/* Parameter which determines the DAC frequency on 43012A0 */
	uint8	ulp_tx_mode;
	/* Parameter which determines the ADC frequency on 43012A0 */
	uint8	ulp_adc_mode;

	phy_param_info_t	*paramsi; /* phytype specific */

	/* Parameter to enable Napping feature in DS1 */
	int8	ds1_napping_enable;

	/* Parameter to indicate PLL used */
	radio_pll_sel_t pll_sel;

	uint16 gain_idx_forced;
	bool both_txchain_rxchain_eq_1;
	bool current_preemption_status;

	/* Flag to indicate X14 */
	uint8 rx5ggainwar;

	uint8 cbuck_out;

	/* Nap info structure */
	phy_ac_nap_info_t *napi;
	bool enIndxCap;

	uint16 ocl_disable_reqs;
	uint8 ocl_en;
	uint8 ocl_coremask;
	bool is_p25TxGainTbl;
	uint16 pktabortctl;

	/* lesi */
	bool lesi;
	bool tia_idx_max_eq_init;

	/* Flag for enabling auto lesiscale cal */
	bool lesiscalecal_enable;

	/* ************************************************************************************ */
	/* [PHY_REARCH] Do not add any variables here. Add them to the individual modules */
	/* ************************************************************************************ */
};

/* Chanspec Call Trace - Various call Paths */
#define CALLED_ON_INIT			1
#define CALLED_ON_BW_CHG		2
#define CALLED_ON_BW_CHG_80P80	3
#define CALLED_ON_BAND_CHG		4

/* Chanspec Call Trace - Which Path ? */
#define CCT_INIT(x)	(mboolisset((x)->CCTrace, CALLED_ON_INIT))
#define CCT_BW_CHG(x)	(mboolisset((x)->CCTrace, CALLED_ON_BW_CHG))
#define CCT_BW_CHG_80P80(x)	(mboolisset((x)->CCTrace, CALLED_ON_BW_CHG_80P80))
#define CCT_BAND_CHG(x)	(mboolisset((x)->CCTrace, CALLED_ON_BAND_CHG))

/* Chanspec Call Trace - Clear All */
#define CCT_CLR(x)	(mboolclr((x)->CCTrace, CALLED_ON_INIT | CALLED_ON_BW_CHG |\
			CALLED_ON_BAND_CHG))

/*
 * Masks for PA mode selection of linear
 * vs. high efficiency modes.
 */

#define PAMODE_HI_LIN_MASK		0x0000FFFF
#define PAMODE_HI_EFF_MASK		0xFFFF0000

typedef enum {
	PAMODE_HI_LIN = 0,
	PAMODE_HI_EFF
} acphy_swctrl_pa_modes_t;

typedef enum {
	ACPHY_TEMPSENSE_VBG = 0,
	ACPHY_TEMPSENSE_VBE = 1
} acphy_tempsense_cfg_opt_t;

#define CAL_COEFF_READ    0
#define CAL_COEFF_WRITE   1
#define CAL_COEFF_WRITE_BIQ2BYP   2
#define MPHASE_TXCAL_CMDS_PER_PHASE  2 /* number of tx iqlo cal commands per phase in mphase cal */
#define ACPHY_RXCAL_TONEAMP 181

extern void phy_ac_rfseq_mode_set(phy_info_t *pi, bool cal_mode);
extern void wlc_phy_tx_farrow_mu_setup(phy_info_t *pi, uint16 MuDelta_l, uint16 MuDelta_u,
	uint16 MuDeltaInit_l, uint16 MuDeltaInit_u);
extern void wlc_phy_radio_tiny_vcocal(phy_info_t *pi);
extern uint8 phy_get_rsdbbrd_corenum(phy_info_t *pi, uint8 core);

/* *********************** Remove ************************** */
void wlc_phy_get_initgain_dB_acphy(phy_info_t *pi, int16 *initgain_dB);
#endif /* _phy_ac_info_h_ */
