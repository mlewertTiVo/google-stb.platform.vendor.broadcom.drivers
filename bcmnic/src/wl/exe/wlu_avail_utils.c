/*
 * Availability support functions
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
 * $Id: wlu_avail_utils.c abhik$
 */
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmendian.h>
#include <wlioctl.h>
#if defined(__FreeBSD__)
#include <machine/stdarg.h>
#include <stdbool.h>
#else
#include <stdarg.h>
#endif
#include <stdio.h>
#include <string.h>
#include <bcmutils.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define stricmp strcmp
#define strnicmp strncmp

static bool
wl_get_tmu_from_str(char *str, wl_tmu_t *p_tmu)
{
	if (stricmp(str, "tu") == 0)
		*p_tmu = WL_TMU_TU;
	else if (stricmp(str, "s") == 0)
		*p_tmu = WL_TMU_SEC;
	else if (stricmp(str, "ms") == 0)
		*p_tmu = WL_TMU_MILLI_SEC;
	else if (stricmp(str, "us") == 0)
		*p_tmu = WL_TMU_MICRO_SEC;
	else if (stricmp(str, "ns") == 0)
		*p_tmu = WL_TMU_NANO_SEC;
	else if (stricmp(str, "ps") == 0)
		*p_tmu = WL_TMU_PICO_SEC;
	else
		return FALSE;

	return TRUE;
}

/*
* parse and pack 'chanspec' from a command-line argument
* Input:
*  arg_channel: a channel-string
*  out_channel: buffer to store the result
*/
static int
wl_parse_channel(char *arg_channel, uint32 *out_channel)
{
	int   status = BCME_OK;
	uint32 src_data_channel = 0;  /* default: invalid */

	/* note, chanespec_t is currently defined as 16-bit, however */
	/* wl-interface use 'uint32' to allow future change for 32-bit */
	if (arg_channel != (char *) NULL)
		src_data_channel = (uint32) wf_chspec_aton(arg_channel);

	if (src_data_channel == 0) {
		printf("error: invalid chanspec\n");
		status = BCME_BADARG;
	}

	*out_channel = htol32(src_data_channel);
	return status;
}

/*
* parse and pack 'intvl' param-value from a command-line argument
* Input:
*  arg_intvl: a time-intvl string
*  out_intvl: buffer to store the result
*/
static int
wl_avail_parse_intvl(char *arg_intvl, wl_time_interval_t *out_intvl)
{
	wl_time_interval_t  src_data_intvl;
	char *p_end;

	/* initialize */
	memset(out_intvl, 0, sizeof(*out_intvl));

	if (arg_intvl == (char *) NULL) {
		printf("error: time-interval value is not specified\n");
		return BCME_BADARG;
	}

	errno = 0;
	memset(&src_data_intvl, 0, sizeof(src_data_intvl));
	/* time interval e.g. 10ns */
	/* get the number */
	p_end = NULL;
	src_data_intvl.intvl = htol32(strtoul(arg_intvl, &p_end, 10));
	if (errno) {
		printf("error: invalid time interval (errno=%d)\n", errno);
		return BCME_BADARG;
	}

	/* get time-unit */
	src_data_intvl.tmu = WL_TMU_TU; /* default */
	if (*p_end != '\0') {
		if (!wl_get_tmu_from_str(p_end, &src_data_intvl.tmu)) {
			printf("error: invalid time-unit %s\n", p_end);
			return BCME_BADARG;
		}
	}
	src_data_intvl.tmu = htol16(src_data_intvl.tmu);

	/* return to caller */
	memcpy(out_intvl, &src_data_intvl, sizeof(*out_intvl));

	return BCME_OK;
}

/*
* parse and pack one 'slot' param-value from a command-line
* Input:
*  arg_slot: 'slot' param-value argument string
*             in "channel:start-tmu:duration-tmu" format
*  out_avail_slot: buffer to store the result
*/
int
wl_avail_parse_slot(char *arg_slot, wl_time_slot_t *out_avail_slot)
{
	int arg_idx;
	const char *tmp_start, *tmp_end;
	char tmpbuf[128];
	int len;
	int   status = BCME_OK;

	if (arg_slot == (char *) NULL) {
		printf("error: slot value is not specified\n");
		return BCME_BADARG;
	}

	/* parse channel:start-tmu:duration-tmu */
	tmp_start = arg_slot;
	for (arg_idx = 0; arg_idx < 3; arg_idx++) {
		tmp_end = strchr(tmp_start, ':');
		if (tmp_end == NULL) {
			if (arg_idx != 2 || *tmp_start == '\0') {
				status = BCME_BADARG;
				goto done;
			}
			/* for last 'duration intvl' */
			tmp_end = tmp_start + strlen(tmp_start);
		}

		/* create a temp null-terminated substring */
		if ((len = tmp_end - tmp_start) >= (int) sizeof(tmpbuf)) {
			status = BCME_BADARG;
			goto done;
		}

		memcpy(tmpbuf, tmp_start, len);
		tmpbuf[len] = '\0';  /* null-terminate */

		if (arg_idx == 0)
			status = wl_parse_channel(tmpbuf, &out_avail_slot->chanspec);
		else if (arg_idx == 1)
			status = wl_avail_parse_intvl(tmpbuf, &out_avail_slot->start);
		else /* arg_idx == 2 */
			status = wl_avail_parse_intvl(tmpbuf, &out_avail_slot->duration);

		if (status != BCME_OK)
			goto done;
		/* continue on next element */
		tmp_start = tmp_end + 1;

	}
done:
	if (status == BCME_BADARG)
		printf("error: invalid value for slot\n");

	return status;
}

/*
* parse and pack one 'period' param-value from a command-line
* Input:
*  arg_slot: 'slot' param-value argument string
*             in "channel:start-tmu:duration-tmu:period" format
*  out_avail_slot: buffer to store the result
*/
int
wl_avail_parse_period(char *arg_slot, wl_time_interval_t *out_intvl)
{
	int cnt = 0;
	char *p = arg_slot;
	int status = BCME_OK;
	if (p == NULL) {
		return BCME_BADARG;
	}

	/* channel:start-tmu:duration-tmu:period */
	for (; cnt < 3; cnt++) {
		p = strchr(p, ':');
		if (!p) {
			break;
		}
		p++;
	}

	if (p == NULL) {
		return BCME_BADARG;
	}
	status = wl_avail_parse_intvl(p, out_intvl);

	return status;
}

int
wl_avail_validate_format(char *arg_slot)
{
	/* channel:start-tmu:duration-tmu:period OR
	 * channel:start-tmu:duration-tmu
	 */
	char *p = arg_slot;
	int cnt = 0;

	while (p != NULL) {
		p = strchr(p, ':');
		if (p == NULL)
			break;
		p++;
		cnt++;
	}

	if (cnt == 2 || cnt == 3) {
		return BCME_OK;
	}

	return BCME_BADARG;
}
