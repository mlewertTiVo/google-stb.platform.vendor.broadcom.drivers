/*
 * TEMPerature sense module internal interface.
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
 * $Id: phy_temp_st.h 653133 2016-08-05 04:40:58Z $
 */

#ifndef _phy_temp_st_
#define _phy_temp_st_

#include <typedefs.h>
#include <phy_temp.h>

typedef struct {
	uint8	disable_temp; /* temp at which to drop to 1-Tx chain */
	uint8	disable_temp_max_cap;
	uint8	hysteresis;   /* temp hysteresis to enable multi-Tx chains */
	uint8	enable_temp;  /* temp at which to enable multi-Tx chains */
	bool	heatedup;     /* indicates if chip crossed tempthresh */
	uint8	bitmap;       /* upper/lower nibble is for rxchain/txchain */
	bool    degrade1RXen; /* 1-RX chain is enabled */
	uint8	duty_cycle; /* Current DutyCycle RSDB mode only */
	uint8	duty_cycle_throttle_depth;
	uint8	duty_cycle_throttle_state;
	uint8	phycal_tempdelta; /* temperature delta below which
							* phy calibrations will not run
							*/
	uint8	phycal_tempdelta_default;
} phy_txcore_temp_t;

/*
 * Query the states pointer.
 */
phy_txcore_temp_t *phy_temp_get_st(phy_temp_info_t *ti);

#endif /* _phy_temp_st_ */
