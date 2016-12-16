/*
 * PHY Utils module implementation - miscellaneous
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
 * $Id: phy_utils_misc.c 583048 2015-08-31 16:43:34Z $
 */

#include <phy_utils_misc.h>

/* Sort vector (of length len) into ascending order */
void
phy_utils_shell_sort(int len, int *vector)
{
	int i, j, incr;
	int v;

	incr = 4;

	while (incr < len) {
		incr *= 3;
		incr++;
	}

	while (incr > 1) {
		incr /= 3;
		for (i = incr; i < len; i++) {
			v = vector[i];
			j = i;
			while (vector[j-incr] > v) {
				vector[j] = vector[j-incr];
				j -= incr;
				if (j < incr)
					break;
			}
			vector[j] = v;
		}
	}
}
