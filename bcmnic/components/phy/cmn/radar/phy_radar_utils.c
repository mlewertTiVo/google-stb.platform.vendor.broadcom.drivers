/*
 * RadarDetect module implementation (shared by PHY implementations)
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
 * $Id: phy_radar_utils.c 603085 2015-11-30 23:46:58Z chihap $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include "phy_radar_st.h"
#include "phy_radar_utils.h"

/*
 * select_nfrequent - crude for now
 * inlist - input array (tier list) that has been sorted into ascending order
 * length - length of input array
 * n - position of interval value/frequency to return
 * value - interval
 * frequency - number of occurrences of interval value
 * vlist - returned interval list
 * flist - returned frequency list
 */
int
wlc_phy_radar_select_nfrequent(int *inlist, int length, int n, int *value,
	int *position, int *frequency, int *vlist, int *flist)
{
	/*
	 * needs declarations:
		int vlist[RDR_TIER_SIZE];
		int flist[RDR_TIER_SIZE];
	 * from calling routine
	 */
	int i, j, pointer, counter, newvalue, nlength;
	int plist[RDR_LIST_SIZE];
	int f, v, p;

	vlist[0] = inlist[0];
	plist[0] = 0;
	flist[0] = 1;

	pointer = 0;
	counter = 0;

	for (i = 1; i < length; i++) {	/* find the frequencies */
		newvalue = inlist[i];
		if (newvalue != vlist[pointer]) {
			pointer++;
			vlist[pointer] = newvalue;
			plist[pointer] = i;
			flist[pointer] = 1;
			counter = 0;
		} else {
			counter++;
			flist[pointer] = counter;
		}
	}

	nlength = pointer + 1;

	for (i = 1; i < nlength; i++) {	/* insertion sort */
		f = flist[i];
		v = vlist[i];
		p = plist[i];
		j = i - 1;
		while ((j >= 0) && flist[j] > f) {
			flist[j + 1] = flist[j];
			vlist[j + 1] = vlist[j];
			plist[j + 1] = plist[j];
			j--;
		}
		flist[j + 1] = f;
		vlist[j + 1] = v;
		plist[j + 1] = p;
	}

	if (n < nlength) {
		*value = vlist[nlength - n - 1];
		*position = plist[nlength - n - 1];
		*frequency = flist[nlength - n - 1] + 1;
	} else {
		*value = 0;
		*position = 0;
		*frequency = 0;
	}
	return nlength;
}
