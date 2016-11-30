/*
 * PHY utils - math library functions.
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
 * $Id: phy_utils_math.h 635757 2016-05-05 05:18:39Z vyass $
 */

#ifndef _phy_utils_math_h_
#define _phy_utils_math_h_

#include <typedefs.h>
#include <phy_api.h>

#define	CORDIC_AG	39797
#define	CORDIC_NI	18
#define	FIXED(X)	((int32)((X) << 16))
#define	FLOAT(X)	(((X) >= 0) ? ((((X) >> 15) + 1) >> 1) : -((((-(X)) >> 15) + 1) >> 1))


void phy_utils_computedB(uint32 *cmplx_pwr, int8 *p_cmplx_pwr_dB, uint8 core);
void phy_utils_cordic(math_fixed theta, math_cint32 *val);
void phy_utils_invcordic(math_cint32 val, int32 *angle);
uint8 phy_utils_nbits(int32 value);
uint32 phy_utils_sqrt_int(uint32 value);
uint32 phy_utils_qdiv(uint32 dividend, uint32 divisor, uint8 precision, bool round);
uint32 phy_utils_qdiv_roundup(uint32 dividend, uint32 divisor, uint8 precision);
void phy_utils_mat_rho(int64 *n, int64 *p, int64 *rho, int m);
void phy_utils_mat_transpose(int64 *a, int64 *b, int m, int n);
void phy_utils_mat_mult(int64 *a, int64 *b, int64 *c, int m, int n, int r);
void phy_utils_mat_inv_prod_det(int64 *a, int64 *b);
void phy_utils_mat_det(int64 *a, int64 *det);
void phy_utils_angle_to_phasor_lut(uint16 angle, uint16* packed_word);
#endif /* _phy_utils_math_h_ */
