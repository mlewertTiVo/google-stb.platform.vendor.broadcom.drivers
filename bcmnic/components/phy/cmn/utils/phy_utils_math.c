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
 * $Id: phy_utils_math.c 635757 2016-05-05 05:18:39Z vyass $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>

#include <wlc_phy_int.h>
#include <phy_utils_math.h>

void
phy_utils_computedB(uint32 *cmplx_pwr, int8 *p_cmplx_pwr_dB, uint8 core)
{
	uint8 shift_ct, lsb, msb, secondmsb, i;
	uint32 tmp;

	ASSERT(core <= PHY_CORE_MAX);

	PHY_INFORM(("wlc_phy_compute_dB: compute_dB for %d cores\n", core));
	for (i = 0; i < core; i++) {
		tmp = cmplx_pwr[i];
		shift_ct = msb = secondmsb = 0;
		while (tmp != 0) {
			tmp = tmp >> 1;
			shift_ct++;
			lsb = (uint8)(tmp & 1);
			if (lsb == 1)
				msb = shift_ct;
		}

		if (msb != 0)
		secondmsb = (uint8)((cmplx_pwr[i] >> (msb - 1)) & 1);

		p_cmplx_pwr_dB[i] = (int8)(3*msb + 2*secondmsb);
		PHY_INFORM(("wlc_phy_compute_dB: p_cmplx_pwr_dB[%d] %d\n", i, p_cmplx_pwr_dB[i]));
	}
}

/* Atan table for cordic >> num2str(atan(1./(2.^[0:17]))/pi*180,8) */
static const math_fixed AtanTbl[] = {
	2949120,
	1740967,
	919879,
	466945,
	234379,
	117304,
	58666,
	29335,
	14668,
	7334,
	3667,
	1833,
	917,
	458,
	229,
	115,
	57,
	29
};

void
phy_utils_cordic(math_fixed theta, math_cint32 *val)
{
	math_fixed angle, valtmp;
	unsigned iter;
	int signx = 1;
	int signtheta;

	val[0].i = CORDIC_AG;
	val[0].q = 0;
	angle    = 0;

	/* limit angle to -180 .. 180 */
	signtheta = (theta < 0) ? -1 : 1;
	theta = ((theta+FIXED(180)*signtheta)% FIXED(360))-FIXED(180)*signtheta;

	/* rotate if not in quadrant one or four */
	if (FLOAT(theta) > 90) {
		theta -= FIXED(180);
		signx = -1;
	} else if (FLOAT(theta) < -90) {
		theta += FIXED(180);
		signx = -1;
	}

	/* run cordic iterations */
	for (iter = 0; iter < CORDIC_NI; iter++) {
		if (theta > angle) {
			valtmp = val[0].i - (val[0].q >> iter);
			val[0].q = (val[0].i >> iter) + val[0].q;
			val[0].i = valtmp;
			angle += AtanTbl[iter];
		} else {
			valtmp = val[0].i + (val[0].q >> iter);
			val[0].q = -(val[0].i >> iter) + val[0].q;
			val[0].i = valtmp;
			angle -= AtanTbl[iter];
		}
	}

	/* re-rotate quadrant two and three points */
	val[0].i = val[0].i*signx;
	val[0].q = val[0].q*signx;
}

void
phy_utils_invcordic(math_cint32 val, int32 *angle)
{
	int valtmp;
	unsigned iter;

	*angle = 0;
	val.i = val.i << 4;
	val.q = val.q << 4;

	/* run cordic iterations */
	for (iter = 0; iter < CORDIC_NI; iter++) {
		if (val.q < 0) {
			valtmp = val.i - (val.q >> iter);
			val.q = (val.i >> iter) + val.q;
			val.i = valtmp;
			*angle -= AtanTbl[iter];
		} else {
			valtmp = val.i + (val.q >> iter);
			val.q = -(val.i >> iter) + val.q;
			val.i = valtmp;
			*angle += AtanTbl[iter];
		}
	}

}

uint8
phy_utils_nbits(int32 value)
{
	int32 abs_val;
	uint8 nbits = 0;

	abs_val = ABS(value);
	while ((abs_val >> nbits) > 0) nbits++;

	return nbits;
}

uint32
phy_utils_sqrt_int(uint32 value)
{
	uint32 root = 0, shift = 0;

	/* Compute integer nearest to square root of input integer value */
	for (shift = 0; shift < 32; shift += 2) {
		if (((0x40000000 >> shift) + root) <= value) {
			value -= ((0x40000000 >> shift) + root);
			root = (root >> 1) | (0x40000000 >> shift);
		} else {
			root = root >> 1;
		}
	}

	/* round to the nearest integer */
	if (root < value) ++root;

	return root;
}

uint32
phy_utils_qdiv(uint32 dividend, uint32 divisor, uint8 precision, bool round)
{
	uint32 quotient, remainder, roundup, rbit;

	ASSERT(divisor);

	quotient = dividend / divisor;
	remainder = dividend % divisor;
	rbit = divisor & 1;
	roundup = (divisor >> 1) + rbit;

	while (precision--) {
		quotient <<= 1;
		if (remainder >= roundup) {
			quotient++;
			remainder = ((remainder - roundup) << 1) + rbit;
		} else {
			remainder <<= 1;
		}
	}

	/* Final rounding */
	if (round && (remainder >= roundup))
		quotient++;

	return quotient;
}

uint32
phy_utils_qdiv_roundup(uint32 dividend, uint32 divisor, uint8 precision)
{
	return phy_utils_qdiv(dividend, divisor, precision, TRUE);
}

/*
 * Compute matrix rho ( m x 3)
 *
 * column 1 = all 1s
 * column 2 = n[i]
 * column 3 = - n[i] * P[i]
 */
void
phy_utils_mat_rho(int64 *n, int64 *p, int64 *rho, int m)
{
	int i;
	int q1 = 2;

	for (i = 0; i < m; i++) {
		*(rho + (i * 3) + 0) = 1;
		*(rho + (i * 3) + 1) = *(n + (i * 1) + 0);
		*(rho + (i * 3) + 2) =
			- (*(n + (i * 1) + 0) * (*(p + (i * 1) + 0)));
		*(rho + (i * 3) + 2) = (*(rho + (i * 3) + 2) + (int64)(1<<(q1-1))) >> q1;
	}
}

/*
 * Matrix transpose routine
 * matrix a (m x n)
 * matrix b = a_transpose(n x m)
 */
void
phy_utils_mat_transpose(int64 *a, int64 *b, int m, int n)
{
	int i, j;

	for (i = 0; i < m; i++)
		for (j = 0; j < n; j++)
			/* b[j][i] = a[i][j]; */
			*(b + (j * m) + i) = *(a + (i * n) + j);
}

/*
 * Matrix multiply routine.
 * matrix a (m x n)
 * matrix b (n x r)
 * c = result matrix (m x r)
 * m = number of rows of matrix a
 * n = number of cols of matrix a and number of rows of matrix b
 * r = number of cols of matrix b
 * assumes matrixes are allocated in memory contiguously one row after
 * the other
 *
 */
void
phy_utils_mat_mult(int64 *a, int64 *b, int64 *c, int m, int n, int r)
{
	int i, j, k;

	for (i = 0; i < m; i++)
		for (j = 0; j < r; j++) {
			*(c + (i * r) + j) = 0;
			for (k = 0; k < n; k++)
				/* c[i][j] += a[i][k] * b[k][j]; */
				*(c + (i * r) + j)
					+= *(a + (i * n) + k) *
					*(b + (k * r) + j);
		}
}


/*
 * Matrix inverse of a 3x3 matrix * det(matrix)
 * a and b: matrices of 3x3
 */
void
phy_utils_mat_inv_prod_det(int64 *a, int64 *b)
{

	/* C2_calc = [	a22*a33 - a32*a23  a13*a32 - a12*a33  a12*a23 - a13*a22
					a23*a31 - a21*a33  a11*a33 - a13*a31  a13*a21 - a11*a23
					a21*a32 - a31*a22  a12*a31 - a11*a32  a11*a22 - a12*a21];
	*/

	int64 a11 = *(a + (0 * 3) + 0);
	int64 a12 = *(a + (0 * 3) + 1);
	int64 a13 = *(a + (0 * 3) + 2);

	int64 a21 = *(a + (1 * 3) + 0);
	int64 a22 = *(a + (1 * 3) + 1);
	int64 a23 = *(a + (1 * 3) + 2);

	int64 a31 = *(a + (2 * 3) + 0);
	int64 a32 = *(a + (2 * 3) + 1);
	int64 a33 = *(a + (2 * 3) + 2);

	*(b + (0 * 3) + 0) = a22 * a33 - a32 * a23;
	*(b + (0 * 3) + 1) = a13 * a32 - a12 * a33;
	*(b + (0 * 3) + 2) = a12 * a23 - a13 * a22;

	*(b + (1 * 3) + 0) = a23 * a31 - a21 * a33;
	*(b + (1 * 3) + 1) = a11 * a33 - a13 * a31;
	*(b + (1 * 3) + 2) = a13 * a21 - a11 * a23;

	*(b + (2 * 3) + 0) = a21 * a32 - a31 * a22;
	*(b + (2 * 3) + 1) =  a12 * a31 - a11 * a32;
	*(b + (2 * 3) + 2) = a11 * a22 - a12 * a21;
}

void
phy_utils_mat_det(int64 *a, int64 *det)
{

	/* det_C1 = a11*a22*a33 + a12*a23*a31 + a13*a21*a32 -
				 a11*a23*a32 - a12*a21*a33 - a13*a22*a31;
	*/

	int64 a11 = *(a + (0 * 3) + 0);
	int64 a12 = *(a + (0 * 3) + 1);
	int64 a13 = *(a + (0 * 3) + 2);

	int64 a21 = *(a + (1 * 3) + 0);
	int64 a22 = *(a + (1 * 3) + 1);
	int64 a23 = *(a + (1 * 3) + 2);

	int64 a31 = *(a + (2 * 3) + 0);
	int64 a32 = *(a + (2 * 3) + 1);
	int64 a33 = *(a + (2 * 3) + 2);

	*det = a11 * a22 * a33 + a12 * a23 * a31 + a13 * a21 * a32 -
		a11 * a23 * a32 - a12 * a21 * a33 - a13 * a22 * a31;
}

void
phy_utils_angle_to_phasor_lut(uint16 angle, uint16* packed_word)
{
	int16 sin_tbl[] = {
	0x000, 0x00d, 0x019, 0x026, 0x032, 0x03f, 0x04b, 0x058,
	0x064, 0x070, 0x07c, 0x089, 0x095, 0x0a1, 0x0ac, 0x0b8,
	0x0c4, 0x0cf, 0x0db, 0x0e6, 0x0f1, 0x0fc, 0x107, 0x112,
	0x11c, 0x127, 0x131, 0x13b, 0x145, 0x14e, 0x158, 0x161,
	0x16a, 0x173, 0x17b, 0x184, 0x18c, 0x194, 0x19b, 0x1a3,
	0x1aa, 0x1b1, 0x1b7, 0x1bd, 0x1c4, 0x1c9, 0x1cf, 0x1d4,
	0x1d9, 0x1de, 0x1e2, 0x1e6, 0x1ea, 0x1ed, 0x1f1, 0x1f4,
	0x1f6, 0x1f8, 0x1fa, 0x1fc, 0x1fe, 0x1ff, 0x1ff, 0x200};

	uint16 k, num_angles = 2;
	uint16 theta[2], recip_coef_nfrac = 11;
	int16  re = 0, im = 0, exp, quad;
	int16  sin_idx, cos_idx;
	int16  sin_out, cos_out;
	uint32 packed;

	theta[0] = (uint8) (angle & 0xFF);
	theta[1] = (uint8) ((angle >> 8) & 0xFF);

	/* printf("---- theta1 = %d, theta2 = %d\n", theta[0], theta[1]); */

	for (k = 0, packed = 0; k < num_angles; k++) {

		/* 6 LSBs for 1st quadrant */
		sin_idx = (theta[k] & 0x3F);
		cos_idx = 63 - sin_idx;

		sin_out = sin_tbl[sin_idx];
		cos_out = sin_tbl[cos_idx];

		/* 2MSBs for quadrant */
		quad = ((theta[k] >> 6) & 0x3);

		if (quad == 0) {
			re =  cos_out; im = -sin_out;
		} else if (quad == 1) {
			re = -sin_out; im = -cos_out;
		} else if (quad == 2) {
			re = -cos_out; im =  sin_out;
		} else if (quad == 3) {
			re =  sin_out; im =  cos_out;
		}

		re += (re < 0) ? (1 << recip_coef_nfrac) : 0;
		im += (im < 0) ? (1 << recip_coef_nfrac) : 0;
		exp = 1;

		packed = (uint32) ((exp << (2*recip_coef_nfrac)) |
		(im << recip_coef_nfrac) | re);

		if (k == 0) {
			packed_word[0] = (packed & 0xFFFF);
			packed_word[1] = (packed >> 16) & 0xFF;
		} else if (k == 1) {
			packed_word[1] |= ((packed & 0xFF) << 8);
			packed_word[2] = (packed >> 8) & 0xFFFF;
		}
	}
	/* printf("reciprocity packed_word: %x%x%x\n",
	   packed_word[2], packed_word[1], packed_word[0]);
	*/
}
