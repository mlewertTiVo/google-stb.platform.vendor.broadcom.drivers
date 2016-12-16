/*
 * ACPHY RSSI Compute module interface
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
 * $Id: phy_ac_rssi.h 653304 2016-08-06 03:05:00Z $
 */

#ifndef _phy_ac_rssi_h_
#define _phy_ac_rssi_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_rssi.h>
#include <phy_type_rssi.h>

/* forward declaration */
typedef struct phy_ac_rssi_info phy_ac_rssi_info_t;

typedef struct phy_ac_rssi_data {
	/* this data is shared between rssi and misc */
	bool	rxgaincal_rssical;	/* 0 = rxgain error cal and 1 = RSSI error cal */
	/* this data is shared between rssi, misc and rxgcrs */
	bool	rssi_cal_rev;		/* 0 = OLD and 1 = NEW */
	/* this data is only shared to phy_ac_rssi_iov */
	bool	rssi_qdB_en; /* 0 = dqB reporting of RSSI is disabled */
} phy_ac_rssi_data_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_rssi_info_t *phy_ac_rssi_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_rssi_info_t *ri);
void phy_ac_rssi_unregister_impl(phy_ac_rssi_info_t *info);

/* inter-module data API */
phy_ac_rssi_data_t *phy_ac_rssi_get_data(phy_ac_rssi_info_t *rssii);

/* intra-module data API */
/* this is accessed by rxgcrs module */
int phy_ac_rssi_set_cal_rev(phy_ac_rssi_info_t *ri, bool set_val);
/* this is accessed only by the iov file */
int phy_ac_rssi_set_qdb_en(phy_ac_rssi_info_t *ri, bool set_val);
int phy_ac_rssi_set_cal_rxgain(phy_ac_rssi_info_t *ri, bool set_val);

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
void phy_ac_rssi_init_gain_err(phy_ac_rssi_info_t *info);

extern int16 phy_ac_rssi_compute_compensation(phy_type_rssi_ctx_t *ctx,
	int16 *rxpwr_core, bool db_qdb);
uint8 wlc_phy_rssi_get_chan_freq_range_acphy(phy_info_t *pi, uint8 core_segment_mapping,
	uint8 core);
int phy_ac_rssi_set_qdb_en(phy_ac_rssi_info_t *ri, bool set_val);
int phy_ac_rssi_get_qdb_en(phy_ac_rssi_info_t *ri, int32 *ret_val);

#endif /* _phy_ac_rssi_h_ */
