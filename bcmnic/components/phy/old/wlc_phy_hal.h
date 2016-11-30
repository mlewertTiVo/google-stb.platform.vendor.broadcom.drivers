/*
 * Required functions exported by the PHY module (phy-dependent)
 * to common (os-independent) driver code.
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
 * $Id: wlc_phy_hal.h 644994 2016-06-22 06:23:44Z vyass $
 */

#ifndef _wlc_phy_h_
#define _wlc_phy_h_

#include <typedefs.h>
#include <wlioctl.h>
#include <bcmutils.h>
#include <osl.h>
#include <siutils.h>
#include <d11.h>
#include <wlc_phy_shim.h>
#include <wlc_ppr.h>
#include <wlc_phy_types.h>

#define M_LCNXN_BLK_PTR         (71 * 2)

/* Noise Cal enable */
#define NOISE_CAL_LCNXNPHY
#define ENABLE_LISTEN_GAIN_CHANGE

#define ENABLE_FDS

/* SMTH MACROS */
#define SMTH_DISABLE			0x0
#define SMTH_ENABLE			0x1
#define SMTH_ENABLE_NO_NW		0x2
#define SMTH_ENABLE_NO_NW_GD		0x3
#define SMTH_ENABLE_NO_NW_GD_MTE	0x4
#define DISABLE_SIGB_AND_SMTH		0x5
#define SMTH_FOR_TXBF			0x6

/* SMTH DUMP MODE */
#define SMTH_NODUMP			0
#define SMTH_FREQDUMP			2
#define SMTH_FREQDUMP_AFTER_NW		3
#define SMTH_FREQDUMP_AFTER_GD		4
#define SMTH_FREQDUMP_AFTER_MTE		5
#define SMTH_TIMEDUMP_AFTER_IFFT	6
#define SMTH_TIMEDUMP_AFTER_WIN		7
#define SMTH_FREQDUMP_AFTER_FFT		8

/* Radio macros */

/* Radio ID */
#define	IDCODE_VER_MASK		0x0000000f
#define	IDCODE_VER_SHIFT	0
#define	IDCODE_MFG_MASK		0x00000fff
#define	IDCODE_MFG_SHIFT	0
#define	IDCODE_ID_MASK		0x0ffff000
#define	IDCODE_ID_SHIFT		12
#define	IDCODE_REV_MASK		0xf0000000
#define	IDCODE_REV_SHIFT	28

#define IDCODE_ACPHY_ID_MASK   0xffff
#define IDCODE_ACPHY_ID_SHIFT  0
#define IDCODE_ACPHY_REV_MASK  0xffff0000
#define IDCODE_ACPHY_REV_SHIFT 16
#define IDCODE_ACPHY_MAJORREV_MASK  0xfff00000
#define IDCODE_ACPHY_MAJORREV_SHIFT 20
#define IDCODE_ACPHY_MINORREV_MASK  0x000f0000
#define IDCODE_ACPHY_MINORREV_SHIFT 16

#define IDCODE_LCN20PHY_ID_MASK   0xffff
#define IDCODE_LCN20PHY_ID_SHIFT  0
#define IDCODE_LCN20PHY_REV_MASK  0xffff0000
#define IDCODE_LCN20PHY_REV_SHIFT 16

#define	NORADIO_ID		0xe4f5
#define	NORADIO_IDCODE		0x4e4f5246

#define	BCM2050_ID		0x2050
#define	BCM2050_IDCODE		0x02050000
#define	BCM2050A0_IDCODE	0x1205017f
#define	BCM2050A1_IDCODE	0x2205017f
#define	BCM2050R8_IDCODE	0x8205017f

#define BCM2055_ID		0x2055
#define BCM2055_IDCODE		0x02055000
#define BCM2055A0_IDCODE	0x1205517f

#define BCM2056_ID		0x2056
#define BCM2056_IDCODE		0x02056000
#define BCM2056A0_IDCODE	0x1205617f

#define BCM2057_ID		0x2057
#define BCM2057_IDCODE		0x02057000
#define BCM2057A0_IDCODE	0x1205717f

#define BCM2059_ID		0x2059
#define BCM2059A0_IDCODE	0x0205917f

#define	BCM2060_ID		0x2060
#define	BCM2060_IDCODE		0x02060000
#define	BCM2060WW_IDCODE	0x1206017f

#define BCM2062_ID		0x2062
#define BCM2062_IDCODE		0x02062000
#define BCM2062A0_IDCODE	0x0206217f

#define BCM2063_ID		0x2063
#define BCM2063_IDCODE		0x02063000
#define BCM2063A0_IDCODE	0x0206317f

#define BCM2064_ID		0x2064
#define BCM2064_IDCODE		0x02064000
#define BCM2064A0_IDCODE	0x0206417f

#define BCM2065_ID		0x2065
#define BCM2065_IDCODE		0x02065000
#define BCM2065A0_IDCODE	0x0206517f

#define BCM2067_ID		0x2067
#define BCM2067_IDCODE		0x02067000
#define BCM2067A0_IDCODE	0x0206717f

#define BCM2066_ID		0x2066
#define BCM2066_IDCODE		0x02066000
#define BCM2066A0_IDCODE	0x0206617f

#define BCM20671_ID		0x022e
#define BCM20671_IDCODE		0x0022e000
#define BCM20671A0_IDCODE	0x0022e17f
#define BCM20671A1_IDCODE	0x1022e17f
#define BCM20671B0_IDCODE	0x11022e17f

#define BCM2069_ID		0x2069
#define BCM2069A0_IDCODE	0x02069000

#define BCM20691_ID		0x30b
#define BCM20692_ID		0x3da
#define BCM20693_ID		0x3eb
#define BCM20694_ID		0x45a
#define BCM20695_ID		0x53a
#define BCM20696_ID		0x58e
#define BCM20697_ID		0x5fb


	 /* PHY macros */
#define PHY_MAX_CORES		4	/* max number of cores supported by PHY HAL */

#define PHY_TPC_HW_OFF		FALSE
#define PHY_TPC_HW_ON		TRUE

#define PHY_PERICAL_DRIVERUP	1	/* periodic cal for driver up */
#define PHY_PERICAL_WATCHDOG	2	/* periodic cal for watchdog */
#define PHY_PERICAL_PHYINIT	3	/* periodic cal for phy_init */
#define PHY_PERICAL_JOIN_BSS	4	/* periodic cal for join BSS */
#define PHY_PERICAL_START_IBSS	5	/* periodic cal for join IBSS */
#define PHY_PERICAL_UP_BSS	6	/* periodic cal for up BSS */
#define PHY_PERICAL_CHAN	7 	/* periodic cal for channel change */
#define PHY_FULLCAL		8	/* full calibration routine */
#define PHY_PAPDCAL		10	/* papd calibration routine */
#define PHY_PERICAL_DCS		11 	/* periodic cal for DCS */
#ifdef WLOTA_EN
#define PHY_FULLCAL_SPHASE	PHY_PERICAL_JOIN_BSS /* full cal in single phase */
#endif /* WLOTA_EN */

/* full cal in single phase in the event of phymode switch  */
#define PHY_PERICAL_PHYMODE_SWITCH	12

#define PHY_PERICAL_DISABLE	0	/* periodic cal disabled */
#define PHY_PERICAL_SPHASE	1	/* periodic cal enabled, single phase only */
#define PHY_PERICAL_MPHASE	2	/* periodic cal enabled, can do multiphase */
#define PHY_PERICAL_MANUAL	3	/* disable periodic cal, only run it from iovar */

#define PHY_PERICAL_MAXINTRVL (15*60) /* Maximum time interval in sec between PHY calibrations */

#define PHY_PERICAL_DELAY_DEFAULT	5	/* delay (in ms) between each cal mphase */
#define PHY_PERICAL_DELAY_MIN		1	/* min delay between each cal mphase */
#define PHY_PERICAL_DELAY_MAX		2000	/* max delay between each cal mphase */

#define PHY_MPH_CAL_DELAY		150	/* multi phase phy cal schedule delay */

#define PHY_HOLD_FOR_ASSOC	1	/* hold PHY activities(like cal) during association */
#define PHY_HOLD_FOR_SCAN	2	/* hold PHY activities(like cal) during scan */
#define PHY_HOLD_FOR_RM		4	/* hold PHY activities(like cal) during radio measure */
#define PHY_HOLD_FOR_PLT	8	/* hold PHY activities(like cal) during plt */
#define PHY_HOLD_FOR_MUTE		0x10
#define PHY_HOLD_FOR_NOT_ASSOC	0x20
#define PHY_HOLD_FOR_ACI_SCAN	0x40 /* hold PHY activities(like cal) during ACI scan */
#define PHY_HOLD_FOR_PKT_ENG	0x80 /* hold PHY activities(like cal) during PKTENG mode */
#define PHY_HOLD_FOR_DCS	    0x100 /* hold PHY activities(like cal) during DCS */
#define PHY_HOLD_FOR_MPC_SCAN	0x200 /* hold PHY activities(like cal) during scan in mpc 1 mode */
#define PHY_HOLD_FOR_EXCURSION	0x400 /* hold PHY activities while in excursion */
#define PHY_HOLD_FOR_TOF	0x800 /* hold PHY activities while doing wifi ranging */
#define PHY_HOLD_FOR_VSDB	0x1000 /* hold PHY cals during  VSDB */

#define PHY_MUTE_FOR_PREISM	1
#define PHY_MUTE_ALL		0xffffffff

/* Fixed Noise for APHY/BPHY/GPHY */
#define PHY_NOISE_FIXED_VAL 		(-101)	/* reported fixed noise */
#define PHY_NOISE_FIXED_VAL_NPHY       	(-101)	/* reported fixed noise */
#define PHY_NOISE_FIXED_VAL_LCNPHY     	(-101)	/* reported fixed noise */
#define PHY_NOISE_FIXED_VAL_SSLPNPHY    (-101)   /* reported fixed noise */

/* phy_mode bit defs for high level phy mode state information */
#define PHY_MODE_ACI		0x0001	/* set if phy is in ACI mitigation mode */
#define PHY_MODE_CAL		0x0002	/* set if phy is in Calibration mode */
#define PHY_MODE_NOISEM		0x0004	/* set if phy is in Noise Measurement mode */

#define VARS_ACCESSOR_SZ 10 /* Vars table accessor size for dual PHY chips */
/* Reasons for changing the PHY BG NOISE scheduling */
typedef enum  phy_bgnoise_schd_mode {
	PHY_LPAS_MODE = 0,
	PHY_CAL_MODE = 1,
	PHY_FORCEMEASURE_MODE = 2
} phy_bgnoise_schd_mode_t;

#define WLC_TXPWR_DB_FACTOR	4	/* most txpower parameters are in 1/4 dB unit */

/* Phy capabilities. Not a PHYHW/SW interface definition but an arbitrary software definition. */
#define PHY_CAP_40MHZ			0x00000001	/* 40MHz channels supported */
#define PHY_CAP_STBC			0x00000002
#define PHY_CAP_SGI			0x00000004
#define PHY_CAP_80MHZ			0x00000008	/* 80MHz channels supported */
#define PHY_CAP_LDPC			0x00000010
#define PHY_CAP_VHT_PROP_RATES		0x00000020	/* Supports Proprietary VHT rates */
#define PHY_CAP_8080MHZ			0x00000040	/* 80+80 MHz channels supported */
#define PHY_CAP_160MHZ			0x00000080	/* 160 MHz channels supported */
#define PHY_CAP_HT_PROP_RATES		0x00000100	/* Supports Proprietary 11n rates */
#define PHY_CAP_SU_BFR			0x00000200	/* Su Beamformer cap */
#define PHY_CAP_SU_BFE			0x00000400	/* Su Beamformee cap */
#define PHY_CAP_MU_BFR			0x00000800	/* Mu Beamformer cap */
#define PHY_CAP_MU_BFE			0x00001000	/* Mu Beamformee cap */
#define PHY_CAP_1024QAM			0x00002000	/* 1024QAM (c10, c11) rates supported */
#define PHY_CAP_2P5MHZ			0x00004000	/* 2.5 MHz channels supported */
#define PHY_CAP_5MHZ			0x00008000	/* 5   MHz channels supported */
#define PHY_CAP_10MHZ			0x00010000	/* 10  MHz channels supported */

/* PHY Capabilities added for ACREV_GE 44 */
#define PHY_CAP_FDCS			0x00020000 /* Supports FDCS */
#define PHY_CAP_11N_TURBO_QAM		0x00040000 /* Supports 11n Turbo QAM */
#define PHY_CAP_HE			0x00080000 /* Supports High Efficieny (802.11ax) */
#define PHY_CAP_11AX_DCM		0x00100000 /* Supports HE (11ax) Dual carrier mode */

/* PhyInternalCapability1 */

#define PHY_CAP_256QAM_BCC_RX_NSS1_NGI	0x00000001
#define PHY_CAP_256QAM_BCC_RX_NSS2_NGI	0x00000002
#define PHY_CAP_256QAM_BCC_RX_NSS1_SGI	0x00000004
#define PHY_CAP_256QAM_BCC_RX_NSS2_SGI	0x00000008
#define PHY_CAP_256QAM_LDPC_RX_NSS1_NGI	0x00000010
#define PHY_CAP_256QAM_LDPC_RX_NSS2_NGI	0x00000020
#define PHY_CAP_256QAM_LDPC_RX_NSS1_SGI	0x00000040
#define PHY_CAP_256QAM_LDPC_RX_NSS2_SGI	0x00000080
#define PHY_CAP_1024QAM_LDPC_RX		0x00000100
#define PHY_CAP_STBC_RX			0x00000200

/* PhyInternalCapability2 */
#define PHY_CAP_256QAM_BCC_TX_NSS1_NGI	0x00010000
#define PHY_CAP_256QAM_BCC_TX_NSS2_NGI	0x00020000
#define PHY_CAP_256QAM_BCC_TX_NSS1_SGI	0x00040000
#define PHY_CAP_256QAM_BCC_TX_NSS2_SGI	0x00080000
#define PHY_CAP_256QAM_LDPC_TX_NSS1_NGI	0x00100000
#define PHY_CAP_256QAM_LDPC_TX_NSS2_NGI	0x00200000
#define PHY_CAP_256QAM_LDPC_TX_NSS1_SGI	0x00400000
#define PHY_CAP_256QAM_LDPC_TX_NSS2_SGI	0x00800000
#define PHY_CAP_1024QAM_LDPC_TX		0x01000000
#define PHY_CAP_STBC_TX			0x02000000


/* HE (11ax) capabilities */
#ifdef WL11AX
/* HE Generic capabilies */
#define PHY_CAP_HE_BW				0x00000003 /* Supported BW */
#define PHY_CAP_HE_PE				0x0000000c /* Supported Packet Extension Modes */
#define PHY_CAP_HE_DCM				0x00000010 /* Support Dual Carrier Modulation */
#define PHY_CAP_HE_BEAM_CHANGE			0x00000020 /* Supprot Beam change */
#define PHY_CAP_HE_1XLTF_0P8CP			0x00000100 /* Support 1x LTF and 0.8us CP */
#define PHY_CAP_HE_2XLTF_0P8CP			0x00000200 /* Support 1x LTF and 0.8us CP */
#define PHY_CAP_HE_2XLTF_1P6CP			0x00000400 /* Support 1x LTF and 0.8us CP */
#define PHY_CAP_HE_4XLTF_3P2CP			0x00001000 /* Support 1x LTF and 0.8us CP */


/* HE Single user capabilies */
#define PHY_CAP_HE_SU_STBC_RX			0x00000001 /* Single User STBC RX */
#define PHY_CAP_HE_SU_256Q_BCC_NSS1_0P8CP	0x00000002 /* SU 256QAM with NSS 1 & 0.8us cp */
#define PHY_CAP_HE_SU_256Q_BCC_NSS2_0P8CP	0x00000004 /* SU 256QAM with NSS 2 & 0.8us cp */
#define PHY_CAP_HE_SU_256Q_BCC_NSS1_1P6CP	0x00000008 /* SU 256QAM with NSS 1 & 1.6us cp */
#define PHY_CAP_HE_SU_256Q_BCC_NSS2_1P6CP	0x00000010 /* SU 256QAM with NSS 2 & 1.6us cp */
#define PHY_CAP_HE_SU_256Q_LDPC			0x00000020 /* SU 256QAM LDPC */
#define PHY_CAP_HE_SU_1024Q_LDPC		0x00000040 /* SU 1024QAM LDPC */
#define PHY_CAP_HE_SU_BFR			0x00000080 /* SU Beamformer cap */
#define PHY_CAP_HE_SU_BFE			0x00000100 /* SU Beamformee cap */
#define PHY_CAP_HE_SU_MU_BFR			0x00000200 /* SU MU Beamformer cap */
#define PHY_CAP_HE_SU_MU_BFE			0x00000400 /* SU MU Beamformee cap */
#define PHY_CAP_HE_SU_NUM_SOUND			0x00003800 /* SU Number of Sounding Dimensions */

/* HE Resource Unit capabilies */
#define PHY_CAP_HE_RU_STBC_RX			0x00010000 /* Resource Unit STBC RX */
#define PHY_CAP_HE_RU_256Q_BCC_NSS1_0P8CP	0x00020000 /* RU 256QAM with NSS 1 & 0.8us cp */
#define PHY_CAP_HE_RU_256Q_BCC_NSS2_0P8CP	0x00040000 /* RU 256QAM with NSS 2 & 0.8us cp */
#define PHY_CAP_HE_RU_256Q_BCC_NSS1_1P6CP	0x00080000 /* RU 256QAM with NSS 1 & 1.6us cp */
#define PHY_CAP_HE_RU_256Q_BCC_NSS2_1P6CP	0x00100000 /* RU 256QAM with NSS 2 & 1.6us cp */
#define PHY_CAP_HE_RU_256Q_LDPC			0x00200000 /* RU 256QAM LDPC */
#define PHY_CAP_HE_RU_1024Q_LDPC		0x00400000 /* RU 1024QAM LDPC */
#define PHY_CAP_HE_RU_BFR			0x00800000 /* RU Beamformer cap */
#define PHY_CAP_HE_RU_BFE			0x01000000 /* RU Beamformee cap */
#define PHY_CAP_HE_RU_MU_BFR			0x02000000 /* RU MU Beamformer cap */
#define PHY_CAP_HE_RU_MU_BFE			0x04000000 /* RU MU Beamformee cap */
#define PHY_CAP_HE_RU_NUM_SOUND			0x38000000 /* RU Number of Sounding Dimensions */

#endif /* WL11AX */

/* AFE Override */
#define PHY_AFE_OVERRIDE_USR	1
#define PHY_AFE_OVERRIDE_DRV	2

#ifdef BCMPHYCORENUM
#define PHY_CORE_MAX	(BCMPHYCORENUM)

#ifdef WLRSDB
#if PHY_CORE_MAX > 1
#define PHYCORENUM(cn)	(((cn) > PHY_CORE_MAX) ? (PHY_CORE_MAX) : (cn))
#else
#define PHYCORENUM(cn) PHY_CORE_MAX
#endif /* PHY_CORE_MAX */
#else
#define PHYCORENUM(cn) PHY_CORE_MAX
#endif /* WLRSDB */

#else
#define PHY_CORE_MAX	4
#define PHYCORENUM(cn)	(cn)
#endif /* BCMPHYCORENUM */

#ifdef BCMPHYCOREMASK
#define PHYCOREMASK(cm)	(BCMPHYCOREMASK)
#else
#define PHYCOREMASK(cm)	(cm)
#endif

#define PHY_INVALID_RSSI (-127)
#define DUAL_MAC_SLICES 2

#define PHY_COREMASK_SISO(cm) ((cm == 1 || cm == 2 || cm == 4 || cm == 8) ? 1 : 0)

#define DUAL_PHY_NUM_CORE_MAX 4

/* add the feature to take out the BW RESET duriny the BW change  0: disable 1: enable */
#define BW_RESET 1
/* add the feature to disable DSSF  0: disable 1: enable */
#define DSSF_ENABLE 1
#define DSSFB_ENABLE 1

/* iovar table */
enum {
	/* OLD PHYTYPE specific ones, to phase out, use unified ones at the end
	 * For each unified, mark the original one as "depreciated".
	 * User scripts should stop using them for new testcases
	 */
	IOV_PHY_OCLSCDENABLE,
	IOV_PHY_RXANTSEL,
	IOV_NPHY_5G_PWRGAIN,
	IOV_NPHY_ACI_SCAN,
	IOV_NPHY_BPHY_EVM,
	IOV_NPHY_BPHY_RFCS,
	IOV_NPHY_CALTXGAIN,
	IOV_NPHY_CAL_RESET,
	IOV_NPHY_CAL_SANITY,
	IOV_NPHY_ELNA_GAIN_CONFIG,
	IOV_NPHY_ENABLERXCORE,
	IOV_NPHY_EST_TONEPWR,
	IOV_NPHY_FORCECAL,		/* depreciated */
	IOV_NPHY_GAIN_BOOST,
	IOV_NPHY_GPIOSEL,
	IOV_NPHY_HPVGA1GAIN,
	IOV_NPHY_INITGAIN,
	IOV_NPHY_PAPDCAL,
	IOV_NPHY_PAPDCALINDEX,
	IOV_NPHY_PAPDCALTYPE,
	IOV_NPHY_PERICAL,		/* depreciated */
	IOV_NPHY_PHYREG_SKIPCNT,
	IOV_NPHY_PHYREG_SKIPDUMP,
	IOV_NPHY_RFSEQ,
	IOV_NPHY_RFSEQ_TXGAIN,
	IOV_NPHY_RSSICAL,
	IOV_NPHY_RSSISEL,
	IOV_NPHY_RXCALPARAMS,
	IOV_NPHY_RXIQCAL,
	IOV_NPHY_SCRAMINIT,
	IOV_NPHY_SKIPPAPD,
	IOV_NPHY_TBLDUMP_MAXIDX,
	IOV_NPHY_TBLDUMP_MINIDX,
	IOV_NPHY_TEMPOFFSET,
	IOV_NPHY_TEMPSENSE,		/* depreciated */
	IOV_NPHY_TEST_TSSI,
	IOV_NPHY_TEST_TSSI_OFFS,
	IOV_NPHY_TXIQLOCAL,
	IOV_NPHY_TXPWRCTRL,		/* depreciated */
	IOV_NPHY_TXPWRINDEX,		/* depreciated */
	IOV_NPHY_TX_TEMP_TONE,
	IOV_NPHY_TX_TONE,
	IOV_NPHY_VCOCAL,
	IOV_NPHY_CCK_PWR_OFFSET,

	/* ==========================================
	 * unified phy iovar, independent of PHYTYPE
	 * ==========================================
	 */
	IOV_PHYHAL_MSG = 300,
	IOV_PHY_WATCHDOG,
	IOV_FAST_TIMER,
	IOV_SLOW_TIMER,
	IOV_GLACIAL_TIMER,
	IOV_TXINSTPWR,
	IOV_PHY_FIXED_NOISE,
	IOV_PHYNOISE_POLL,
	IOV_PHY_SPURAVOID,
	IOV_CARRIER_SUPPRESS,
	IOV_UNMOD_RSSI,
	IOV_ACI_EXIT_CHECK_PERIOD,
	IOV_PHY_RXIQ_EST,
	IOV_NUM_STREAM,
	IOV_PHYWREG_LIMIT,
	IOV_MIN_TXPOWER,
	IOV_PHY_MUTED,
	IOV_IQ_IMBALANCE_METRIC_DATA,
	IOV_IQ_IMBALANCE_METRIC,
	IOV_IQ_IMBALANCE_METRIC_PASS,
	IOV_PHY_SAMPLE_COLLECT,
	IOV_PHY_SAMPLE_COLLECT_GAIN_ADJUST,
	IOV_PHY_SAMPLE_COLLECT_GAIN_INDEX,
	IOV_PHY_SAMPLE_DATA,
	IOV_PHY_TXPWRCTRL,
	IOV_PHY_RESETCCA,
	IOV_PHY_GLITCHK,
	IOV_PHY_NOISE_UP,
	IOV_PHY_NOISE_DWN,
	IOV_PHY_TXPWRINDEX,
	IOV_PHY_AVGTSSI_REG,
	IOV_PHY_IDLETSSI_REG,
	IOV_PHY_IDLETSSI,
	IOV_PHY_CAL_DISABLE,
	IOV_PHY_PERICAL,	/* Periodic calibration cofig */
	IOV_PHY_PERICAL_DELAY,
	IOV_PHY_FORCECAL,
	IOV_PHY_TXRX_CHAIN,
	IOV_PHY_BPHY_EVM,
	IOV_PHY_BPHY_RFCS,
	IOV_PHY_ENABLERXCORE,
	IOV_PHY_EST_TONEPWR,
	IOV_PHY_GPIOSEL,
	IOV_PHY_5G_PWRGAIN,
	IOV_PHY_RFSEQ,
	IOV_PHY_SCRAMINIT,
	IOV_PHY_TEMPSENSE,
	IOV_PHY_VBATSENSE,
	IOV_PHY_TEST_TSSI,
	IOV_PHY_TEST_TSSI_OFFS,
	IOV_PHY_TEST_IDLETSSI,
	IOV_PHY_TX_TONE,
	IOV_PHY_TXLO_TONE,
	IOV_PHY_TX_TONE_SYMM,
	IOV_PHY_TX_TONE_HZ,
	IOV_PHY_TX_TONE_STOP,
	IOV_PHY_MAXP,
	IOV_PATRIM,
	IOV_POVARS,
	IOV_RPCALVARS,
	IOV_PHYCAL_STATE,
	IOV_PHY_TXIQCC,
	IOV_PHY_TXLOCC,
	IOV_PHYCAL_TEMPDELTA,
	IOV_PHY_PAPD_DEBUG,
	IOV_PHY_ACTIVECAL,
	IOV_NOISE_MEASURE,
	IOV_PHY_IQLOCALIDX,
	IOV_PHY_LOWPOWER_BEACON_MODE,
	IOV_PHY_SROM_TEMPSENSE,
	IOV_PHY_GAIN_CAL_TEMP,
	IOV_PHY_SETRPTBL,
	IOV_PHY_FORCEIMPBF,
	IOV_PHY_FORCESTEER,
	IOV_PHY_TXFILTER_SM_OVERRIDE,
	IOV_PHY_DEAF,
	IOV_NTD_GDS_LOWTXPWR,
	IOV_PAPDCAL_INDEXDELTA,
	IOV_PHYNOISE_SROM,
	IOV_LCNPHY_SAMP_CAP,
	IOV_PHY_SUBBAND5GVER,
	IOV_PHY_FCBSINIT,
	IOV_PHY_FCBS,
	IOV_PHY_FCBSARM,
	IOV_PHY_FCBSEXIT,
	IOV_PHY_CRS_WAR,
	IOV_LCNPHY_RXIQGAIN,
	IOV_LCNPHY_RXIQGSPOWER,
	IOV_LCNPHY_RXIQPOWER,
	IOV_LCNPHY_RXIQSTATUS,
	IOV_LCNPHY_RXIQSTEPS,
	IOV_LCNPHY_TXPWRCLAMP_DIS,
	IOV_LCNPHY_TXPWRCLAMP_OFDM,
	IOV_LCNPHY_TXPWRCLAMP_CCK,
	IOV_LCNPHY_TSSI_MAXPWR,
	IOV_LCNPHY_TSSI_MINPWR,
	IOV_PHY_CGA_5G,
	IOV_PHY_CGA_2G,
	IOV_PHY_DEBUG_CMD,
	IOV_PHY_FORCE_FDIQI,
	IOV_PHY_FORCE_GAINLEVEL,
	IOV_PHY_BBMULT,
	IOV_LNLDO2,
	IOV_PHY_FORCE_CRSMIN,
	IOV_PHY_FORCE_LESISCALE,
	IOV_PAVARS2,
	IOV_PHY_DSSF,
	IOV_PHY_LESI,
	IOV_PHY_TXPWR_CORE,
	IOV_EDCRS,
	IOV_PHY_FORCECAL_OBT,
	IOV_PHY_DYN_ML,
	IOV_PHY_ACI_NAMS,
	IOV_PHY_RSSI_GAIN_CAL_TEMP,
	IOV_PHY_MAC_TRIGGERED_SAMPLE_COLLECT,
	IOV_PHY_MAC_TRIGGERED_SAMPLE_DATA,
	IOV_PHY_VCOCAL,
	IOV_PKTENG_GAININDEX,
	IOV_PHY_RXGAININDEX,
	IOV_PHY_AUXPGA,
	IOV_HIRSSI_PERIOD,
	IOV_HIRSSI_EN,
	IOV_HIRSSI_BYP_RSSI,
	IOV_HIRSSI_RES_RSSI,
	IOV_HIRSSI_BYP_CNT,
	IOV_HIRSSI_RES_CNT,
	IOV_HIRSSI_STATUS,
	IOV_PHY_TXSWCTRLMAP,
	IOV_PHY_MAC_TRIGGERD_SAMPLE_COLLECT,
	IOV_PHY_MAC_TRIGGERD_SAMPLE_DATA,
	IOV_PHY_AFE_OVERRIDE,
	IOV_CAL_PERIOD,
	IOV_SROM_REV,
	IOV_PHY_TXCAL_GAINSWEEP,
	IOV_PHY_TXCAL_GAINSWEEP_MEAS,
	IOV_PHY_TXCAL_PWR_TSSI_TBL,
	IOV_PHY_TXCAL_APPLY_PWR_TSSI_TBL,
	IOV_PHY_READ_ESTPWR_LUT,
	IOV_OLPC_THRESH,
	IOV_OLPC_ANCHOR_2G,
	IOV_OLPC_ANCHOR_5G,
	IOV_DISABLE_OLPC,
	IOV_OLPC_ANCHOR_IDX,
	IOV_OLPC_IDX_VALID,
	IOV_PHY_TXCAL_STATUS,
	IOV_PHY_ADJUSTED_TSSI,
	IOV_PHY_DUMP_PAGE,
	IOV_PHY_DUMP_ENTRY,
	IOV_OLPC_OFFSET,
	IOV_PHY_MASTER_OVERRIDE,
	IOV_PAPD_EPS_OFFSET,
	IOV_TPC_AV,
	IOV_TPC_VMID,
	IOV_PHY_TXCALVER, /* version for handing different structrues of txcal */
	#if defined(WLTEST) || defined(BCMDBG)
	IOV_PHYMODE,
	IOV_SC_CHAN,
	#endif
	IOV_PHY_LAST	/* insert before this one */
};
typedef enum _wlc_swdiv_swctrl_t {
	SWDIV_SWCTRL_0,   /* Diversity switch controlled via GPIO */
	SWDIV_SWCTRL_1,   /* Diversity switch controlled via LCN40PHY_swctrlOvr_val */
	SWDIV_SWCTRL_2    /* Diversity switch controlled via LCN40PHY_RFOverrideVal0 */
} wlc_swdiv_swctrl_t;

/* forward declaration */
typedef struct shared_phy shared_phy_t;

/* Public phy info structure */
struct phy_pub;
typedef struct phy_pub wlc_phy_t;

typedef struct shared_phy_params {
	void 	*osh;		/* pointer to OS handle */
	si_t	*sih;		/* si handle (cookie for siutils calls) */
	void	*physhim;	/* phy <-> wl shim layer for wlapi */
	uint	unit;		/* device instance number */
	uint	corerev;	/* core revision */
	uint	bustype;	/* SI_BUS, PCI_BUS  */
	uint	buscorerev; 	/* buscore rev */
	char	*vars;		/* phy attach time only copy of vars */
	uint16	vid;		/* vendorid */
	uint16	did;		/* deviceid */
	uint	chip;		/* chip number */
	uint	chiprev;	/* chip revision */
	uint	chippkg;	/* chip package option */
	uint	sromrev;	/* srom revision */
	uint	boardtype;	/* board type */
	uint	boardrev;	/* board revision */
	uint	boardvendor;	/* board vendor */
	uint32	boardflags;	/* board specific flags from srom */
	uint32	boardflags2;	/* more board flags if sromrev >= 4 */
	uint32	boardflags4;	/* more board flags if sromrev >= 12 */
	char	vars_table_accessor[VARS_ACCESSOR_SZ];
} shared_phy_params_t;

/* parameter structure for wlc_phy_txpower_core_offset_get/set functions */
struct phy_txcore_pwr_offsets {
	int8 offset[PHY_MAX_CORES];	/* quarter dBm signed offset for each chain */
};

/* phy_tx_power_t.flags bits */
#define WL_TX_POWER_F_ENABLED	1
#define WL_TX_POWER_F_HW		2
#define WL_TX_POWER_F_MIMO		4
#define WL_TX_POWER_F_SISO		8
#define WL_TX_POWER_F_HT		0x10

typedef struct {
	uint32 flags;
	chanspec_t chanspec;		/* txpwr report for this channel */
	chanspec_t local_chanspec;	/* channel on which we are associated */
	uint8 rf_cores;				/* count of RF Cores being reported */
	uint8 display_core;			/* the displayed core in curpower */
	uint8 est_Pout[4];			/* Latest tx power out estimate per RF chain */
	uint8 est_Pout_act[4]; /* Latest tx power out estimate per RF chain w/o adjustment */
	uint8 est_Pout_cck;			/* Latest CCK tx power out estimate */
	uint8 tx_power_max[4];		/* Maximum target power among all rates */
	uint tx_power_max_rate_ind[4];		/* Index of the rate with the max target power */
	ppr_t *ppr_board_limits;
	ppr_t *ppr_target_powers;
#ifdef WL_SARLIMIT
	int8 SARLIMIT[PHY_MAX_CORES];
#endif
} phy_tx_power_t;

#define NUM_SUBBANDS_FOR_AVVMID 5
typedef struct wlc_phy_avvmid {
	uint8   Av;
	uint8   Vmid;
} wlc_phy_avvmid_t;

typedef struct wlc_phy_avvmid_bands {
	wlc_phy_avvmid_t avvmid[PHY_MAX_CORES][NUM_SUBBANDS_FOR_AVVMID];
} wlc_phy_avvmid_txcal_t;

#ifdef TXPWR_TIMING
extern uint32 hnd_time_us(void);

extern uint wlc_phy_txpower_limit_set_time;
extern uint wlc_channel_reg_limits_time;
extern uint wlc_channel_update_txchain_offsets_time;
extern uint wlc_stf_ss_algo_channel_get_time;
extern uint temp1_time;
extern uint temp2_time;
#endif

#if defined(WLSRVSDB)
#define CHANNEL_UNDEFINED       0xEF
#define SR_MEMORY_BANK	2
#endif /* end WLSRVSDB */

/* NAP disable request ids */
#define WLC_NAP_REQ_HOST		0
#define WLC_NAP_REQ_RSSI		1

#if defined(EVENT_LOG_COMPILE)
#define WLC_PHY_OCL_INFO(args) \
    EVENT_LOG_COMPACT_CAST_PAREN_ARGS(EVENT_LOG_TAG_OCL_INFO, args)
#else
#define WLC_PHY_OCL_INFO(args)    WL_INFORM(args)
#endif /* EVENT_LOG_COMPILE */

/* OCL disable request ids */
#define WLC_OCL_REQ_HOST		0
#define WLC_OCL_REQ_RSSI		1
#define WLC_OCL_REQ_LTEC		2
#define WLC_OCL_REQ_RXCHAIN		3
#define WLC_OCL_REQ_CAL			4
#define WLC_OCL_REQ_CAL_TXBF		5
#define WLC_OCL_REQ_CM			6
#define WLC_OCL_REQ_CHANSWITCH		7


typedef enum {
	PHY_RC_NONE			= 0,
	PHY_RC_TXPOWER_LIMITS		= 1,
	PHY_RC_TEMPSENSE_LIMITS		= 2,
	PHY_RC_VCOCAL_FAILED		= 3,
	PHY_RC_PLL_NOTLOCKED		= 4,
	PHY_RC_DESENSE_LIMITS		= 5,
	PHY_RC_BASEINDEX_LIMITS		= 6,
	PHY_RC_TXCHAIN_INVALID		= 7,
	PHY_RC_PMUCAL_FAILED		= 8,
	PHY_RC_RCAL_INVALID		= 9,
	PHY_RC_FCBS_CHSW_FAILED		= 10,
	PHY_RC_IQEST_FAILED		= 11,
	PHY_RC_RESET2RX_FAILED		= 12,
	PHY_RC_PKTPROC_RESET_FAILED	= 13,
	PHY_RC_RFSEQ_STATUS_INVALID	= 14,
	PHY_RC_TXIQLO_CAL_FAILED	= 15,
	PHY_RC_PAPD_CAL_FAILED		= 16,
	PHY_RC_NOISE_CAL_FAILED		= 17,
	PHY_RC_RFSEQ_STATUS_OCL_INVALID = 18,
	PHY_RC_RX2TX_FAILED		= 19,
	PHY_RC_AFE_CAL_FAILED		= 20,
	PHY_RC_NOMEM			= 21,
	PHY_RC_SAMPLEPLAY_LIMIT		= 22,
	PHY_RC_LAST			/* This must be the last entry */
} phy_crash_reason_t;

/*
 * The PHY APIs below are implemented by the PHY infrastructure module
 */
/* attach/detach */
extern shared_phy_t *wlc_phy_shared_attach(shared_phy_params_t *shp);
extern void  wlc_phy_shared_detach(shared_phy_t *phy_sh);

/* init/deinit */
extern void  wlc_phy_init(wlc_phy_t *ppi, chanspec_t chanspec);
extern void wlc_phy_por_inform(wlc_phy_t *ppi);

/* Watchdog */
extern int32  wlc_phy_watchdog(wlc_phy_t *ppi);
#ifdef WFD_PHY_LL
extern void wlc_phy_wfdll_chan_active(wlc_phy_t * ppi, bool chan_active);
#endif

/* Channel Mgmt */
extern int32 wlc_phy_min_txpwr_limit_get(wlc_phy_t *ppi);
extern void wlc_phy_chanspec_ch14_widefilter_set(wlc_phy_t *ppi, bool wide_filter);
extern void wlc_phy_set_locale(phy_info_t *pi, uint8 region_group);

/* Calibration Mgmt */
extern void wlc_phy_cal_init(wlc_phy_t *ppi);
extern void wlc_phy_cal_perical(wlc_phy_t *ppi, uint8 reason);
extern void wlc_phy_cal_mode(wlc_phy_t *ppi, uint mode);
extern void wlc_phy_initcal_enable(wlc_phy_t *pih, bool initcal);
extern void wlc_phy_mphase_cal_schedule(wlc_phy_t *pih, uint delay_val);


/*
 * The PHY APIs below are implemented by the PHY modules. Each module
 * may have the following functions:
 *	attach
 *	init
 *	reset
 *	detach
 *	and other configuration and action functions	.
 * The function declaration corresponding to a module will be moved to
 * phy_<module>_api.h when it is adapted to the new PHY architecture.
 */
/* PHY Module (Init, Channel Configuration) */

/* flow */
extern void wlc_phy_BSSinit(wlc_phy_t *ppi, bool bonlyap, int noise);


/* AFE Module (Init, Channel Configuration) */

/* Noise Measurement Module */
extern void wlc_sslpnphy_noise_measure(wlc_phy_t *ppi);
extern int8 wlc_phy_noise_avg(wlc_phy_t *wpi);
extern int8 wlc_phy_noise_avg_per_antenna(wlc_phy_t *pih, int coreidx);
extern int8 wlc_phy_noise_lte_avg(wlc_phy_t *wpi);
extern void wlc_phy_noise_sample_request_external(wlc_phy_t *ppi);
extern void wlc_phy_noise_sample_request_crsmincal(wlc_phy_t *ppi);
extern void wlc_phy_noise_sample_intr(wlc_phy_t *ppi);

/* Power Control Module */
extern void wlc_phy_txpower_hw_ctrl_set(wlc_phy_t *ppi, bool hwpwrctrl);
extern uint8 wlc_phy_txpower_get_target_min(wlc_phy_t *ppi);
extern uint8 wlc_phy_txpower_get_target_max(wlc_phy_t *ppi);
extern bool wlc_phy_txpower_ipa_ison(wlc_phy_t *pih);
extern int8 wlc_phy_get_tx_power_offset_by_mcs(wlc_phy_t *ppi, uint8 mcs_offset);
extern int8 wlc_phy_get_tx_power_offset(wlc_phy_t *ppi, uint8 tbl_offset);
int wlc_phy_get_est_pout(wlc_phy_t *ppi, uint8* est_Pout, uint8* est_Pout_adj,
	uint8* est_Pout_cck);
#ifdef WLTXPWR_CACHE
extern void wlc_phy_clear_tx_power_offset(wlc_phy_t *pi);
#endif /* WLTXPWR_CACHE */
/* Submodule - STF */
extern void wlc_phy_stf_chain_init(wlc_phy_t *pih, uint8 txchain, uint8 rxchain);
extern void wlc_phy_stf_chain_set(wlc_phy_t *pih, uint8 txchain, uint8 rxchain);
extern void wlc_phy_stf_chain_get(wlc_phy_t *pih, uint8 *txchain, uint8 *rxchain);
extern uint16 wlc_phy_stf_duty_cycle_chain_active_get(wlc_phy_t *pih);
extern int8 wlc_phy_stf_ssmode_get(wlc_phy_t *pih, chanspec_t chanspec);
/* Submodule - SAR */

#ifdef FCC_PWR_LIMIT_2G
extern bool wlc_phy_isvalid_fcc_pwr_limit(wlc_phy_t *ppi);
extern void wlc_phy_prev_chanspec_set(wlc_phy_t *ppi, chanspec_t prev_chanspec);
#endif /* FCC_PWR_LIMIT_2G */

/* FEM, Antenna Selection/Diversity Module */
extern void wlc_phy_antsel_init(wlc_phy_t *ppi, bool lut_init);
extern void wlc_phy_antsel_type_set(wlc_phy_t *ppi, uint8 antsel_type);

/* AFE (ADC, DAC), PLL Module */
extern void wlc_phy_block_bbpll_change(wlc_phy_t *pih, bool block, bool going_down);

/* Calibration caching Module (regular caching, per channel, smart cal) */
extern int BCMRAMFN(wlc_phy_create_chanctx)(wlc_phy_t *ppi, chanspec_t chanspec);
extern void wlc_phy_avvmid_txcal(wlc_phy_t *ppi, wlc_phy_avvmid_txcal_t *val, bool set);
extern void BCMRAMFN(wlc_phy_destroy_chanctx)(wlc_phy_t *ppi, chanspec_t chanspec);
#if defined(PHYCAL_CACHING)
extern int wlc_phy_cal_cache_init(wlc_phy_t *ppi);
extern void wlc_phy_cal_cache_deinit(wlc_phy_t *ppi);
extern int wlc_phy_invalidate_chanctx(wlc_phy_t *ppi, chanspec_t chanspec);
extern int wlc_phy_reuse_chanctx(wlc_phy_t *ppi, chanspec_t chanspec);
extern void wlc_phy_get_all_cached_ctx(wlc_phy_t *ppi, chanspec_t *chanlist);
extern void wlc_phy_get_cachedchans(wlc_phy_t *ppi, chanspec_t *chanlist);
extern int32 wlc_phy_get_est_chanset_time(wlc_phy_t *ppi, chanspec_t chanspec);
extern bool wlc_phy_chan_iscached(wlc_phy_t *ppi, chanspec_t chanspec);
extern void wlc_phy_set_est_chanset_time(wlc_phy_t *ppi, chanspec_t chanspec,
	bool wascached, bool inband, int32 rectime);
extern int8 wlc_phy_get_max_cachedchans(wlc_phy_t *ppi);
extern void wlc_phy_update_chctx_glacial_time(wlc_phy_t *pi, chanspec_t chanspec);
extern uint32 wlc_phy_get_current_cachedchans(wlc_phy_t *ppi);
#endif /* PHYCAL_CACHING */

#if defined(PHYCAL_CACHING) || defined(PHYCAL_CACHE_SMALL)
extern void wlc_phy_cal_cache_set(wlc_phy_t *ppi, bool state);
extern bool wlc_phy_cal_cache_get(wlc_phy_t *ppi);
#endif

#ifdef BCMDBG
extern void wlc_phy_txpower_limits_dump(ppr_t* txpwr, bool isht);
#endif /* BCM_DBG */


/* Interference and Mitigation (ACI, Noise) Module */
extern void wlc_phy_interference_set(wlc_phy_t *ppi, bool init);
extern void wlc_phy_acimode_noisemode_reset(wlc_phy_t *ppi, chanspec_t chanspec,
	bool clear_aci_state, bool clear_noise_state, bool disassoc);

/* BT Coex Module */
extern void wlc_phy_btclock_war(wlc_phy_t *ppi, bool enable);

/* VSDB, RVSDB (Move it out of PHY) Module */
extern void wlc_phy_reset_srvsdb_engine(wlc_phy_t *pi);
extern void wlc_phy_sr_vsdb_reset(wlc_phy_t *pi);
extern void wlc_phy_force_vsdb_chans(wlc_phy_t *pi, uint16 * chans, uint8 set);
extern void wlc_phy_detach_srvsdb_module(wlc_phy_t *pi);
extern uint8 wlc_phy_attach_srvsdb_module(wlc_phy_t *ppi, chanspec_t chan0, chanspec_t chan1);
extern uint8 wlc_set_chanspec_sr_vsdb(wlc_phy_t *pi, chanspec_t chanspec, uint8 * last_chan_saved);

/* Sample collect Module */
#ifdef WLTEST
extern void wlc_lcnphy_iovar_samp_cap(wlc_phy_t *ppi, int32 gain, int32 *ret);
extern void wlc_phy_pkteng_boostackpwr(wlc_phy_t *ppi);
#endif

extern const uint8 * wlc_phy_get_ofdm_rate_lookup(void);

extern void wlc_phy_tkip_rifs_war(wlc_phy_t *ppi, uint8 rifs);

extern void wlc_phy_btclock_war(wlc_phy_t *ppi, bool enable);
extern uint32 wlc_phy_cap_get(wlc_phy_t *pi);
#if defined(WLC_LOWPOWER_BEACON_MODE)
extern void wlc_phy_lowpower_beacon_mode(wlc_phy_t *pih, int lowpower_beacon_mode);
#endif /* WLC_LOWPOWER_BEACON_MODE */
/* Rx desense Module */
#if defined(RXDESENS_EN)
extern int wlc_nphy_set_rxdesens(wlc_phy_t *ppi, int32 int_val);
#endif

/* Test Module (pkt engine, Sample capture, phy/radio reg dumps) */
extern bool wlc_phy_test_ison(wlc_phy_t *ppi);
extern bool wlc_phy_bist_check_phy(wlc_phy_t *ppi);
extern void wlc_phy_runbist_config(wlc_phy_t *ppi, bool start_end);

/* Low power beacon Module */
#if defined(WLC_LOWPOWER_BEACON_MODE)
extern void wlc_phy_lowpower_beacon_mode(wlc_phy_t *pih, int lowpower_beacon_mode);
#endif /* WLC_LOWPOWER_BEACON_MODE */

/* TXIQLO Module */

/* RXIQ Module */

extern void wlc_phy_reset_srvsdb_engine(wlc_phy_t *pi);
extern void wlc_phy_sr_vsdb_reset(wlc_phy_t *pi);
#ifdef WFD_PHY_LL
extern int wlc_phy_sr_vsdb_invalidate_active(wlc_phy_t * ppi);
#endif
extern void wlc_phy_force_vsdb_chans(wlc_phy_t *pi, uint16 * chans, uint8 set);
extern void wlc_phy_detach_srvsdb_module(wlc_phy_t *pi);
extern uint8 wlc_phy_attach_srvsdb_module(wlc_phy_t *ppi, chanspec_t chan0, chanspec_t chan1);
extern uint8 wlc_set_chanspec_sr_vsdb(wlc_phy_t *pi, chanspec_t chanspec, uint8 * last_chan_saved);

/* Idle TSSI measurement Module */

/* Rx iq est Module */

/* Misc APIs (Move to resp. modules) */
extern void wlc_beacon_loss_war_lcnxn(wlc_phy_t *ppi, uint8 enable);
/* old GPHY stuff */
extern void wlc_phy_freqtrack_start(wlc_phy_t *ppi);
extern void wlc_phy_freqtrack_end(wlc_phy_t *ppi);
extern void wlc_phy_ldpc_override_set(wlc_phy_t *ppi, bool val);
extern void wlc_phy_edcrs_lock(wlc_phy_t *pih, bool lock);
extern void wlc_phy_apply_default_edthresh_acphy(phy_info_t *pi, chanspec_t chanspec);

/* Query */
extern int8 wlc_phy_preamble_override_get(wlc_phy_t *ppi);
extern bool wlc_phy_get_filt_war(wlc_phy_t *ppi);
extern const uint8 * wlc_phy_get_ofdm_rate_lookup(void);
extern uint32 wlc_phy_cap_get(wlc_phy_t *pi);
extern void wlc_phy_ulb_feature_flag_set(wlc_phy_t *pi);

/* Configuration, Utilities */
extern void wlc_phy_preamble_override_set(wlc_phy_t *ppi, int8 override);
extern void wlc_phy_clear_tssi(wlc_phy_t *ppi);
extern void wlc_phy_BSSinit(wlc_phy_t *ppi, bool bonlyap, int noise);
extern void wlc_phy_set_glacial_timer(wlc_phy_t *ppi, uint period);
#ifdef WLTEST
extern void wlc_phy_boardflag_upd(wlc_phy_t *phi);
#endif
#ifdef WLTEST
extern void wlc_phy_pkteng_boostackpwr(wlc_phy_t *ppi);
#endif
extern void wlc_phy_set_filt_war(wlc_phy_t *ppi, bool filt_war);
extern int wlc_phy_lo_gain_nbcal(wlc_phy_t *ppi, bool lo_gain);

extern void  wlc_phy_hw_clk_state_upd(wlc_phy_t *ppi, bool newstate);
extern void  wlc_phy_hw_state_upd(wlc_phy_t *ppi, bool newstate);
extern void wlc_phy_machwcap_set(wlc_phy_t *ppi, uint32 machwcap);
extern void wlc_phy_bf_preempt_enable(wlc_phy_t *pih, bool bf_preempt);
extern void wlc_phy_tkip_rifs_war(wlc_phy_t *ppi, uint8 rifs);
extern void wlc_acphy_set_scramb_dyn_bw_en(wlc_phy_t *pi, bool enable);
extern void wlc_phy_interf_chan_stats_update(wlc_phy_t *pi, chanspec_t chanspec, uint32 crsglitch,
	uint32 bphy_crsglitch, uint32 badplcp, uint32 bphy_badplcp, uint32 mbsstime);

/* WAR */
extern void wlc_phy_ofdm_rateset_war(wlc_phy_t *pih, bool war);
extern void wlc_phy_conditional_suspend(phy_info_t *pi, bool *suspend);
extern void wlc_phy_conditional_resume(phy_info_t *pi, bool *suspend);

extern void wlc_phy_clear_match_tx_offset(wlc_phy_t *ppi, ppr_t *pprptr);
#ifdef RXDESENS_EN
extern void wlc_phy_rxdesens(wlc_phy_t *ppi, bool desens_en);
#ifdef WLSRVSDB
extern bool wlc_phy_rxdesens_defer(wlc_phy_t *ppi);
#endif
#endif /* RXDESENS_EN */
#ifdef WLMCHAN
extern bool wlc_phy_acimode_noisemode_reset_allowed(wlc_phy_t *ppi, uint16 HomeChannel);
extern bool wlc_phy_srvsdb_switch_status(wlc_phy_t *ppi);
#endif /* WLMCHAN */
#ifdef ATE_BUILD
void wlc_ate_gpiosel_disable(wlc_phy_t *ppi);
#endif
#ifdef WLC_TXDIVERSITY
uint8 wlc_phy_get_oclscdenable_status(wlc_phy_t *pih);
#endif /* WLC_TXDIVERSITY */
extern void wlc_phy_update_link_rssi(wlc_phy_t *pih, int8 rssi);
extern void wlc_phy_watchdog_override(phy_info_t *pi, bool val);

#ifdef WLTXPWR_CACHE
tx_pwr_cache_entry_t* wlc_phy_get_txpwr_cache(wlc_phy_t *ppi);
#if !defined(WLTXPWR_CACHE_PHY_ONLY)
void wlc_phy_set_txpwr_cache(wlc_phy_t *ppi, void* cacheptr);
#endif
#endif	/* WLTXPWR_CACHE */

extern uint8 wlc_phy_get_txpwr_backoff(wlc_phy_t *ppi);
extern void wlc_phy_get_txpwr_min(wlc_phy_t *ppi, uint8 *min_pwr);
extern uint8 wlc_phy_get_bfe_ndp_recvstreams(wlc_phy_t *ppi);
extern uint32 wlc_phy_get_cal_dur(wlc_phy_t *pih);

extern void wlc_phy_noise_sched_set(wlc_phy_t *pih, phy_bgnoise_schd_mode_t reason, bool upd);
extern bool wlc_phy_noise_sched_get(wlc_phy_t *pih);
extern void wlc_phy_noise_pmstate_set(wlc_phy_t *pih, bool pmstate);
#if defined(WL_NAP)
	#if defined(WL_ENAB_RUNTIME_CHECK) || !defined(DONGLEBUILD)
		#define PHY_NAP_ENAB(physhim)		(wlapi_nap_enab_check(physhim))
	#elif defined(WL_NAP_DISABLED)
		#define PHY_NAP_ENAB(physhim)		(0)
	#else
		#define PHY_NAP_ENAB(physhim)		(wlapi_nap_enab_check(physhim))
	#endif
#else
	#define PHY_NAP_ENAB(physhim)			(0)
#endif /* WL_NAP */

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
extern void wlc_phy_ocl_disable_req_set(wlc_phy_t *ppi, uint16 req, bool disable, uint8 req_id);
extern void wlc_phy_ocl_status_get(wlc_phy_t *ppi, uint16 *reqs, uint8 *coremask, bool *ocl_en);
extern uint8 wlc_phy_ocl_get_coremask(wlc_phy_t *ppi);
extern void  wlc_phy_ocl_coremask_change(wlc_phy_t *ppi, uint8 coremask);
#endif /* OCL */

extern void wlc_phy_btc_mode_set(wlc_phy_t *ppi, int btc_mode);

#endif	/* _wlc_phy_h_ */
