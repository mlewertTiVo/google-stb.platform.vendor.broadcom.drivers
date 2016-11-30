/*
 * TOF module public interface (to MAC driver).
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
 * $Id: phy_tof_api.h 606042 2015-12-14 06:21:23Z jqliu $
 */
#ifndef _phy_tof_api_h_
#define _phy_tof_api_h_


#include <typedefs.h>
#include <phy_api.h>

#define WL_PROXD_SEQ
#define WL_PROXD_BW_MASK	0x7
#define WL_PROXD_SEQEN		0x80
#define WL_PROXD_80M_40M	1
#define WL_PROXD_80M_20M	2
#define WL_PROXD_40M_20M	3
#define WL_PROXD_RATE_VHT	0
#define WL_PROXD_RATE_6M	1
#define WL_PROXD_RATE_LEGACY	2
#define WL_PROXD_RATE_MCS_0	3
#define WL_PROXD_RATE_MCS	4
/* #define TOF_DBG */
/* #define TOF_SEQ_40_IN_40MHz */
/* #define TOF_SEQ_20_IN_80MHz */
/* #define TOF_SEQ_20MHz_BW */
#define TOF_SEQ_20MHz_BW_512IFFT
/* #define TOF_SEQ_40MHz_BW */


#ifdef WL_PROXD_SEQ
int wlc_phy_tof(wlc_phy_t *ppi, bool enter, bool tx, bool hw_adj, bool seq_en, int core);
int wlc_tof_seq_upd_dly(wlc_phy_t *ppi, bool tx, uint8 core, bool mac_suspend);
int wlc_phy_tof_seq_params(wlc_phy_t *ppi);
int wlc_phy_tof_info(wlc_phy_t *ppi, int *p_frame_type, int *p_frame_bw, int *p_cfo,
	int8 *p_rssi);
int wlc_phy_tof_kvalue(wlc_phy_t *ppi, chanspec_t chanspec, uint32 *kip,
	uint32 *ktp, uint8 seq_en);
int wlc_phy_kvalue(wlc_phy_t *ppi, chanspec_t chanspec, uint32 rspecidx, uint32 *kip,
	uint32 *ktp, uint8 seq_en);
#else
int wlc_phy_tof(wlc_phy_t *ppi, bool enter, bool hw_adj);
int wlc_phy_tof_info(wlc_phy_t *ppi, int* p_frame_type, int* p_frame_bw, int8* p_rssi);
int wlc_phy_tof_kvalue(wlc_phy_t *ppi, chanspec_t chanspec, uint32 *kip, uint32 *ktp);
#endif /* WL_PROXD_SEQ */
int wlc_phy_chan_freq_response(wlc_phy_t *ppi, int len, int nbits, int32* Hr,
	int32* Hi, uint32* Hraw);
int wlc_phy_chan_mag_sqr_impulse_response(wlc_phy_t *ppi, int frame_type,
	int len, int offset, int nbits, int32* h, int* pgd, uint32* hraw, uint16 tof_shm_ptr);
int wlc_phy_seq_ts(wlc_phy_t *ppi, int n, void* p_buffer, int tx, int cfo, int adj,
	void* pparams, int32* p_ts, int32* p_seq_len, uint32* p_raw);
void wlc_phy_tof_cmd(wlc_phy_t *ppi, bool seq);
#ifdef TOF_DBG
int wlc_phy_tof_dbg(wlc_phy_t *ppi, int arg); /* DEBUG */
#endif

#endif /* _phy_tof_api_h_ */
