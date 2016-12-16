/*
 * Sample Collect module interface (to other PHY modules).
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
 * $Id: phy_samp.h 662012 2016-09-28 03:05:05Z $
 */

#ifndef _phy_samp_h_
#define _phy_samp_h_

#include <phy_api.h>

/* forward declaration */
typedef struct phy_samp_info phy_samp_info_t;

/* attach/detach */
phy_samp_info_t *phy_samp_attach(phy_info_t *pi);
void phy_samp_detach(phy_samp_info_t *cmn_info);

/* up/down */
int phy_samp_init(phy_samp_info_t *cmn_info);
int phy_samp_down(phy_samp_info_t *cmn_info);

/* MAC based sample play regs */
#define PHYREF_SampleCollectCurPtr	u.d11acregs.SampleCollectCurPtr
#define PHYREF_SaveRestoreStartPtr	u.d11acregs.SaveRestoreStartPtr
#define PHYREF_SampleCollectStopPtr	u.d11acregs.SampleCollectStopPtr
#define PHYREF_SampleCollectStartPtr	u.d11acregs.SampleCollectStartPtr
#define PHYREF_SampleCollectPlayCtrl	u.d11acregs.SampleCollectPlayCtrl
#define PHYREF_SampleCollectCurPtrHigh	u.d11acregs.SampleCollectCurPtrHigh
#define PHYREF_SampleCollectPlayPtrHigh	u.d11acregs.SampleCollectPlayPtrHigh
#define PHYREF_ClkGateReqCtrl0 u.d11regs.ClkGateReqCtrl0
#define PHYREF_ClkGateReqCtrl1 u.d11regs.ClkGateReqCtrl1
#define PHYREF_ClkGateReqCtrl2 u.d11regs.ClkGateReqCtrl2
#define PHYREF_ClkGateUcodeReq0 u.d11regs.ClkGateUcodeReq0
#define PHYREF_ClkGateUcodeReq1 u.d11regs.ClkGateUcodeReq1
#define PHYREF_ClkGateUcodeReq2 u.d11regs.ClkGateUcodeReq2
#define PHYREF_ClkGateStretch0 u.d11regs.ClkGateStretch0
#define PHYREF_ClkGateStretch1 u.d11regs.ClkGateStretch1
#define PHYREF_ClkGateMisc u.d11regs.ClkGateMisc
#define PHYREF_ClkGateDivCtrl u.d11regs.ClkGateDivCtrl
#define PHYREF_ClkGatePhyClkCtrl u.d11regs.ClkGatePhyClkCtrl
#define PHYREF_ClkGateSts u.d11regs.ClkGateSts
#define PHYREF_ClkGateExtReq0 u.d11regs.ClkGateExtReq0
#define PHYREF_ClkGateExtReq1 u.d11regs.ClkGateExtReq1
#define PHYREF_ClkGateExtReq2 u.d11regs.ClkGateExtReq2
#define PHYREF_ClkGateUcodePhyClkCtrl u.d11regs.ClkGateUcodePhyClkCtrl

/* bitfields in PhyCtrl (IHR Address 0x049) */
#define PHYCTRL_SAMPLEPLAYSTART_SHIFT 11
#define PHYCTRL_MACPHYFORCEGATEDCLKSON_SHIFT 1

/* bitfields in SampleCollectPlayCtrl | Applicable to (d11rev >= 53) and (d11rev == 50) */
#define SAMPLE_COLLECT_PLAY_CTRL_PLAY_START_SHIFT 9


/* ****************************************** */
/* CMN Layer sample collect Public API's      */
/* ****************************************** */
#ifdef SAMPLE_COLLECT
extern int phy_iovars_sample_collect(phy_info_t *pi, uint32 actionid, uint16 type, void *p,
	uint plen, void *a, int alen, int vsize);

/* ************************* */
/* phytype export prototypes */
/* ************************* */

/* HTPHY */
extern int phy_ht_sample_data(phy_info_t *pi, wl_sampledata_t *p, void *b);
extern int phy_ht_sample_collect(phy_info_t *pi, wl_samplecollect_args_t *p, uint32 *b);

/* NPHY */
extern int8 phy_n_sample_collect_gainadj(phy_info_t *pi, int8 gainadj, bool set);
extern int phy_n_sample_data(phy_info_t *pi, wl_sampledata_t *p, void *b);
extern int phy_n_sample_collect(phy_info_t *pi,	wl_samplecollect_args_t *p, uint32 *b);
extern int phy_n_mac_triggered_sample_data(phy_info_t *pi, wl_sampledata_t *p, void *b);
extern int phy_n_mac_triggered_sample_collect(phy_info_t *pi, wl_samplecollect_args_t *p,
	uint32 *b);

extern int wlc_phy_sample_collect_lcn20phy(phy_info_t *pi, wl_samplecollect_args_t *collect,
	uint32 *buf);
#ifdef IQPLAY_DEBUG
extern int phy_samp_prep_IQplay(phy_info_t *pi);
#endif /* IQPLAY_DEBUG */

#endif /* SAMPLE_COLLECT */

#endif /* _phy_samp_h_ */
