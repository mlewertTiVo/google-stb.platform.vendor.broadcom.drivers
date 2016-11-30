/**
 * @file
 * @brief
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
 * $Id: $
 */
/* This file generates d11revs.txt file while has the list of supported revs */

#include <stdio.h>
#ifndef D11CONF
#define D11CONF 0
#endif

#ifndef D11CONF2
#define D11CONF2 0
#endif

#ifndef D11CONF3
#define D11CONF3 0
#endif

#ifndef D11CONF4
#define D11CONF4 0
#endif

#ifndef D11CONF5
#define D11CONF5 0
#endif

static void
find_d11rev(FILE *fp, int *d11rev, int d11conf)
{
	int i;

	for (i = 0; i <= 31; i++, (*d11rev)++) {
		if (d11conf & (1 << i)) {
			fprintf(fp, "Rev ID: %d\n", (*d11rev));
			printf("%d ", (*d11rev));
		}
	}
}

int
main(void)
{
	int rev_id = 0;
	FILE *fp;

	fp = fopen("d11shm_d11revs_dbg.txt", "w");

	fprintf(fp, "D11CONF: 0x%08x\n", D11CONF);
	fprintf(fp, "D11CONF2: 0x%08x\n", D11CONF2);
	fprintf(fp, "D11CONF3: 0x%08x\n\n", D11CONF3);
	fprintf(fp, "D11CONF4: 0x%08x\n\n", D11CONF4);
	fprintf(fp, "D11CONF5: 0x%08x\n\n", D11CONF5);

	find_d11rev(fp, &rev_id, D11CONF);
	find_d11rev(fp, &rev_id, D11CONF2);
	find_d11rev(fp, &rev_id, D11CONF3);
	find_d11rev(fp, &rev_id, D11CONF4);
	find_d11rev(fp, &rev_id, D11CONF5);

	fclose(fp);

	return 0;
}
