/*
 * ACPHY PHYTableInit module interface
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
 * $Id: phy_ac_tbl.h 655464 2016-08-19 23:20:17Z $
 */

#ifndef _phy_ac_tbl_h_
#define _phy_ac_tbl_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_tbl.h>

/* forward declaration */
typedef struct phy_ac_tbl_info phy_ac_tbl_info_t;
typedef struct phytbl_info acphytbl_info_t;

typedef struct phy_ac_tbl_data {
	/* this data is shared between tbl and radio */
	void	*chan_tuning;
	uint32	chan_tuning_tbl_len;
	/* this data is shared between tbl and tpc */
	bool	is_p25TxGainTbl;
} phy_ac_tbl_data_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_tbl_info_t *phy_ac_tbl_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_tbl_info_t *ti);
void phy_ac_tbl_unregister_impl(phy_ac_tbl_info_t *info);

/* inter-module data API */
phy_ac_tbl_data_t *phy_ac_tbl_get_data(phy_ac_tbl_info_t *tbli);

uint8 wlc_phy_get_tbl_id_gainctrlbbmultluts(phy_info_t *pi, uint8 core);
uint8 wlc_phy_get_tbl_id_estpwrshftluts(phy_info_t *pi, uint8 core);
extern uint32 wlc_phy_ac_caps(phy_info_t *pi);
extern uint32 wlc_phy_ac_caps1(phy_info_t *pi);
extern uint8 wlc_phy_ac_phycap_maxbw(phy_info_t *pi);
extern void wlc_phy_table_write_ext_acphy(phy_info_t *pi, const acphytbl_info_t *ptbl_info);
extern void wlc_phy_table_write_acphy(phy_info_t *pi, uint32 id, uint32 len, uint32 offset,
	uint32 width, const void *data);
extern void wlc_phy_table_read_ext_acphy(phy_info_t *pi, const acphytbl_info_t *ptbl_info);
extern void wlc_phy_table_read_acphy(phy_info_t *pi, uint32 i, uint32 l, uint32 o, uint32 w,
	void *d);
extern void wlc_phy_table_write_acphy_dac_war(phy_info_t *pi, uint32 id, uint32 len,
	uint32 offset, uint32 width, void *data, uint8 core);
extern void wlc_phy_table_read_acphy_dac_war(phy_info_t *pi, uint32 id, uint32 len,
	uint32 offset, uint32 width, void *data, uint8 core);
extern void wlc_phy_table_write_tiny_chnsmth(phy_info_t *pi, uint32 id, uint32 len, uint32 offset,
	uint32 width, const void *data);
extern void wlc_phy_force_mac_clk(phy_info_t *pi, uint16 *orig_phy_ctl);
extern void wlc_phy_clear_static_table_acphy(phy_info_t *pi, const phytbl_info_t *ptbl_info,
	const uint32 tbl_info_cnt);
const uint16 *wlc_phy_get_txgain_tbl_20695(phy_info_t *pi);
void wlc_phy_ac_gains_load(phy_ac_tbl_info_t *tbli);
void wlc_phy_tx_gain_table_write_acphy(phy_ac_tbl_info_t *tbli, uint32 l,
	uint32 o, uint32 w, const void *d);
#endif /* _phy_ac_tbl_h_ */
