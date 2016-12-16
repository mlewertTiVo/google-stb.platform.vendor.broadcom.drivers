/*
 * VASIP related declarations
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wlc_vasip.h 672288 2016-11-25 11:46:09Z $
 */

#ifndef _WLC_VASIP_H_
#define _WLC_VASIP_H_

#if defined(WLVASIP) || defined(SAVERESTORE)
void wlc_vasip_write(wlc_hw_info_t *wlc_hw, const uint32 vasip_code[],
	const uint nbytes, uint32 offset);
void wlc_vasip_read(wlc_hw_info_t *wlc_hw, uint32 vasip_code[],
	const uint nbytes, uint32 offset);
bool wlc_vasip_present(wlc_hw_info_t *wlc_hw);
#endif /* defined(WLVASIP) || defined(SAVERESTORE) */

/* defines svmp address */
#define VASIP_COUNTERS_ADDR_OFFSET		0x20000
#define VASIP_STATUS_ADDR_OFFSET		0x20100
#define VASIP_STEER_MCS_ADDR_OFFSET		0x21e10
#define VASIP_RECOMMEND_MCS_ADDR_OFFSET		0x22ea0
#define VASIP_SPECTRUM_TBL_ADDR_OFFSET		0x6800	/* 0x26800 - 0x8000 * 4 */
#define VASIP_OVERWRITE_MCS_FLAG_ADDR_OFFSET	0x20060
#define VASIP_OVERWRITE_MCS_BUF_ADDR_OFFSET	0x21d00
#define VASIP_STEERING_MCS_BUF_ADDR_OFFSET	0x21e10
#define VASIP_GROUP_METHOD_ADDR_OFFSET		0x20045
#define VASIP_GROUP_NUMBER_ADDR_OFFSET		0x20046
#define VASIP_GROUP_FORCED_ADDR_OFFSET		0x20047
#define VASIP_GROUP_FORCED_MCS_ADDR_OFFSET	0x20048
#define VASIP_MCS_CAPPING_ENA_OFFSET		0x20044
#define VASIP_MCS_RECMND_MI_ENA_OFFSET		0x20049
#define VASIP_SGI_RECMND_METHOD_OFFSET		0x2004a
#define VASIP_SGI_RECMND_THRES_OFFSET		0x2004b
#define VASIP_DELAY_GROUP_TIME_OFFSET		0x20054
#define VASIP_DELAY_PRECODER_TIME_OFFSET	0x20055
#define VASIP_GROUP_FORCED_OPTION_ADDR_OFFSET	0x21be0
#define VASIP_PPR_TABLE_OFFSET			0x21d20
#define VASIP_MCS_THRESHOLD_OFFSET		0x22EE0
#define VASIP_M2V0_OFFSET			0x20300
#define VASIP_M2V1_OFFSET			0x20700
#define VASIP_V2M_GROUP_OFFSET			0x20b00
#define VASIP_V2M_PRECODER_OFFSET		0x20be0
#define VASIP_RATECAP_BLK			0x22f20


#define VASIP_RTCAP_SGI_NBIT		0x2
#define VASIP_RTCAP_LDPC_NBIT		0x4
#define VASIP_RTCAP_BCMSTA_NBIT		0x5

#if defined(WLVASIP)
/* initialize vasip */
void wlc_vasip_init(wlc_hw_info_t *wlc_hw, uint32 vasipver, bool nopi);
/* vasip version provision */
bool wlc_vasip_support(wlc_hw_info_t *wlc_hw, uint32 *vasipver, bool nopi);

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
/* dump vasip code info */
void wlc_vasip_code_info(wlc_info_t *wlc, struct bcmstrbuf *b);
#endif

/* attach/detach */
extern wlc_vasip_info_t *wlc_vasip_attach(wlc_info_t *wlc);
extern void wlc_vasip_detach(wlc_vasip_info_t *vasip);
int wlc_svmp_mem_blk_set(wlc_hw_info_t *wlc_hw, uint32 offset, uint16 len, uint16 *val);

#define VASIP_CODE_OFFSET	0
#endif /* WLVASIP */

#endif /* _WLC_VASIP_H_ */
