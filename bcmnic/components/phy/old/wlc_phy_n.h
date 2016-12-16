/*
 * ABGPHY module header file
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
 * $Id: wlc_phy_n.h 659514 2016-09-14 20:19:00Z $
 */

#ifndef _wlc_phy_n_h_
#define _wlc_phy_n_h_


#include <typedefs.h>
#include <phy_n_info.h>

/* iovar table */
enum {
	/* OLD PHYTYPE specific ones, to phase out, use unified ones at the end
	 * For each unified, mark the original one as "depreciated".
	 * User scripts should stop using them for new testcases
	 */
	IOV_PHY_OCLSCDENABLE = 100,
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
	IOV_NPHY_CCK_PWR_OFFSET
};

extern void
wlc_phy_rfctrl_override_nphy_rev7(phy_info_t *pi, uint16 field, uint16 value, uint8 core_mask,
                                  uint8 off, uint8 override_id);

extern void
wlc_phy_rfctrl_override_nphy(phy_info_t *pi, uint16 field, uint16 value, uint8 core_mask,
                             uint8 off);

/* ********************* REMOVE *********************** */
int16 wlc_phy_rxgaincode_to_dB_nphy(phy_info_t *pi, uint16 gain_code);
void wlc_phy_set_spurmode_nphy(phy_info_t *pi, chanspec_t chanspec);
void wlc_phy_nphy_tkip_rifs_war(phy_info_t *pi, uint8 rifs);
#endif /* _wlc_phy_n_h_ */
