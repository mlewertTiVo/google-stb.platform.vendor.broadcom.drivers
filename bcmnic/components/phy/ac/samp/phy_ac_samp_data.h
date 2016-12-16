/*
 * Sample Collect module internal interface (to PHY specific implementations).
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
 * $Id: _phy_samp_data_h_.h 583048 2015-08-31 16:43:34Z $
 */

#ifndef _phy_samp_data_h_
#define _phy_samp_data_h_

#include <typedefs.h>

#ifdef IQPLAY_DEBUG

/* Macros for sample play */
#define SZ_PING_WAVEFORM_11AX_TEST	5780

extern const uint32 phy_ping_waveform_11ax_test[SZ_PING_WAVEFORM_11AX_TEST];
#endif /* IQPLAY_DEBUG */

#endif /* _phy_samp_data_h_ */
