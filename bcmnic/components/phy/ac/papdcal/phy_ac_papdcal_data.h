/*
 * ACPHY PAPD CAL module data interface
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: phy_ac_papdcal_data.h 648244 2016-07-11 14:33:08Z $
 */

#ifndef _phy_ac_papdcal_data_h_
#define _phy_ac_papdcal_data_h_

#include <typedefs.h>

#define SZ_SCAL_TABLE_0		64
#define SZ_SCAL_TABLE_1		128
#define SZ_PGA_GAIN_ARRAY	256
#define SZ_WBPAPD_WAVEFORM	4000

extern const int8 pga_gain_array_2g[SZ_PGA_GAIN_ARRAY];
extern const int8 pga_gain_array_5g_0[SZ_PGA_GAIN_ARRAY];
extern const int8 pga_gain_array_5g_1[SZ_PGA_GAIN_ARRAY];
extern const int8 pga_gain_array_5g_2[SZ_PGA_GAIN_ARRAY];
extern const int8 pga_gain_array_5g_3[SZ_PGA_GAIN_ARRAY];
extern const int8 pga_gain_array_2g_4354[SZ_PGA_GAIN_ARRAY];
extern const int8 pga_gain_array_5g_4354[SZ_PGA_GAIN_ARRAY];
extern const int8 pga_gain_array_2g_epapd[2][SZ_PGA_GAIN_ARRAY];
extern const int8 pga_gain_array_5g_epapd_0[SZ_PGA_GAIN_ARRAY];
extern const int8 pga_gain_array_5g_epapd_1[SZ_PGA_GAIN_ARRAY];
extern const int8 pga_gain_array_5g_epapd_2[SZ_PGA_GAIN_ARRAY];
extern const int8 pga_gain_array_5g_epapd_3[SZ_PGA_GAIN_ARRAY];
extern const int8 lut_log20_bbmult[100];

extern const uint32 acphy_papd_scaltbl_128[SZ_SCAL_TABLE_1];
extern const uint32 acphy_papd_scaltbl[SZ_SCAL_TABLE_0];
extern const uint32 acphy_wbpapd_waveform[SZ_WBPAPD_WAVEFORM];
extern uint32 fcbs_wbpapd_waveform[SZ_WBPAPD_WAVEFORM];

#endif /* _phy_ac_papdcal_data_h_ */
