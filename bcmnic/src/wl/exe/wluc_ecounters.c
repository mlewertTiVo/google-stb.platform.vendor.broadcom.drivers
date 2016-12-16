/*
 * wl ecounters command module
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
 * $Id:$
 */

#ifdef WIN32
#include <windows.h>
#endif
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_packet.h>

#include <wlioctl.h>


#include <sys/stat.h>
#include <errno.h>

/* Because IL_BIGENDIAN was removed there are few warnings that need
 * to be fixed. Windows was not compiled earlier with IL_BIGENDIAN.
 * Hence these warnings were not seen earlier.
 * For now ignore the following warnings
 */
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4761)
#endif

#include <bcmutils.h>
#include <bcmendian.h>
#include <proto/802.3.h>
#include <proto/bcmip.h>
#include <proto/bcmipv6.h>
#include <proto/bcmtcp.h>
#include "wlu_common.h"
#include "wlu.h"

#define EVENT_LOG_DUMPER
#include <event_log.h>

static cmd_func_t wl_ecounters_config;
static cmd_func_t wl_event_ecounters_config;

static cmd_t wl_ecounters_cmds[] = {
	{ "ecounters", wl_ecounters_config, -1, WLC_SET_VAR,
	"\tDescription: Configure ecounters to retrieve statistics at periodic intervals"
	"\tusage: wl ecounters <event_log_set_num> <event_log_buffer_size> "
	"<periodicity in secs> <num_events> <event_log_tag_num1> <event_log_tag_num2>..."
	"\tTo disable ecounters:  set periodicity and num_events = 0 with no tags. "
	"For ex. wl ecounters 0 0 0 0"
	},
	{ "event_ecounters", wl_event_ecounters_config, -1, WLC_SET_VAR,
	"\tDescription: Configure WLC events to trigger ecounters when an event to be \n"
	"\tgenerated matches a hosts supplied events mask\n"
	"\tusage: wl event_ecounters -m mask [-s set -t <tag> [<tag> ...]]\n"
	"\twhere mask is hex bitmask of events\n"
	"\tset is the event log set where ecounters should write data.\n"
	"\ttag(s) is/are event log tags that ecounters should report.\n"
	"\tNote that the host must initialize event log set of\n"
	"\tinterest with event_log_set_init iovar\n"
	"\tTo disable event ecounters configuration:\n"
	"\twl event_ecounters -m 0x0\n"
	},
	{ NULL, NULL, 0, 0, NULL }
};

/* Command format: wl counters <event log set> < event log buffer size>
 * <periodicity in secs> <number of events> <tag1> <tag 2> ...
 */
#define ECOUNTERS_ARG_EVENT_LOG_SET_POS			0
#define ECOUNTERS_ARG_EVENT_LOG_BUFFER_SIZE_POS		1
#define ECOUNTERS_ARG_PERIODICITY_POS			2
#define ECOUNTERS_ARG_NUM_EVENTS			3
#define ECOUNTERS_NUM_BASIC_ARGS			4

static int wl_ecounters_config(void *wl, cmd_t *cmd, char **argv)
{
	ecounters_config_request_t *p_config;
	int	rc = 0;
	uint	i, argc, len, alloc_size_adjustment;
	uint	ecounters_basic_args[ECOUNTERS_NUM_BASIC_ARGS];

	/* Count <type> args and allocate buffer */
	for (argv++, argc = 0; argv[argc]; argc++);

	if (argc < ECOUNTERS_NUM_BASIC_ARGS) {
		fprintf(stderr, "Minimum 4 arguments are required for Ecounters. \n");
		fprintf(stderr, "wl ecounters <event log set > < event log buffer size> "
				"<periodicity> <num_events> <tag1> <tag2>..\n");
		return BCME_ERROR;
	}

	for (i = 0; i < ECOUNTERS_NUM_BASIC_ARGS; i++, argv++) {
		char *endptr;
		ecounters_basic_args[i] = strtoul(*argv, &endptr, 0);
		argc--;
		if (*endptr != '\0') {
			fprintf(stderr, "Type '%s' (arg %d) not a number?\n", *argv, i);
			return BCME_ERROR;
		}
	}

	if (ecounters_basic_args[ECOUNTERS_ARG_EVENT_LOG_SET_POS] > NUM_EVENT_LOG_SETS)
	{
		fprintf(stderr, " Event log set > MAX sets (%d)\n", NUM_EVENT_LOG_SETS);
		return BCME_ERROR;
	}

	if (ecounters_basic_args[ECOUNTERS_ARG_EVENT_LOG_BUFFER_SIZE_POS] >
		EVENT_LOG_MAX_BLOCK_SIZE)
	{
		fprintf(stderr, " Event log buffer size > MAX buffer size (%d)\n",
			EVENT_LOG_MAX_BLOCK_SIZE);
		return BCME_ERROR;
	}

	/* Allocate memory for structure to be sent. If no types were passed, allocated
	 * config structure must have aleast one type present even if not used.
	 */
	alloc_size_adjustment = (argc) ? argc : 1;

	len = OFFSETOF(ecounters_config_request_t, type) +
		(alloc_size_adjustment) * sizeof(uint16);
	p_config = (ecounters_config_request_t *)malloc(len);
	if (p_config == NULL) {
		fprintf(stderr, "malloc failed to allocate %d bytes\n", len);
		return -1;
	}
	memset(p_config, 0, len);

	p_config->version = ECOUNTERS_VERSION_1;
	p_config->set =
		ecounters_basic_args[ECOUNTERS_ARG_EVENT_LOG_SET_POS];
	p_config->size =
		ecounters_basic_args[ECOUNTERS_ARG_EVENT_LOG_BUFFER_SIZE_POS];
	p_config->timeout =
		ecounters_basic_args[ECOUNTERS_ARG_PERIODICITY_POS];
	p_config->num_events =
		ecounters_basic_args[ECOUNTERS_ARG_NUM_EVENTS];

	/* No types were passed. So ntypes = 0. */
	p_config->ntypes = (argc == 0) ? 0 : argc;
	for (i = 0; i < argc; i++, argv++) {
		char *endptr;
		p_config->type[i] = strtoul(*argv, &endptr, 0);
		if (*endptr != '\0') {
			fprintf(stderr, "Type '%s' (arg %d) not a number?\n", *argv, i);
			free(p_config);
			return -1;
		}
	}

	rc = wlu_var_setbuf(wl, cmd->name, p_config, len);
	free(p_config);
	return rc;
}

/* Command format: wl event_ecounters -m <hex string> -s <set> -t <tag list> */
static int wl_event_ecounters_config(void *wl, cmd_t *cmd, char **argv)
{
	int argc = 0, err = 0;
	char *mask_arg = NULL, *set_arg = NULL;
	uint16 masksize, eventmsgs_len, trig_config_len, set, i;
	ecounters_eventmsgs_ext_t *eventmsgs;
	ecounters_trigger_config_t *trigger_config;
	uint8* iovar_data_ptr = NULL;

	/* Count <type> args and allocate buffer */
	for (argv++, argc = 0; argv[argc]; argc++);

	/* Look for -m. Position of these arguments is fixed. */
	if ((*argv) && !strcmp(*argv, "-m")) {
		mask_arg = *++argv;
		if (!mask_arg) {
			fprintf(stderr, "Missing argument to -m option\n");
			return BCME_USAGE_ERROR;
		}
		/* Consumed 2 arguments -m and its argument */
		argc -= 2;
		argv++;
	}
	else {
		fprintf(stderr, "Missing -m <hex string> option\n");
		return BCME_USAGE_ERROR;
	}

	/* Skip any 0x if there are any */
	if (mask_arg[0] == '0' && mask_arg[1] == 'x')
		mask_arg += 2;

	/* adjust the length. If there are  <= 2 characters, minimum 1 byte id required */
	masksize = (strlen(mask_arg)/2);
	if (masksize < 1) {
		masksize = 1;
	}
	masksize = MIN(masksize, WL_EVENTING_MASK_LEN);
	eventmsgs_len =
		ROUNDUP((masksize + ECOUNTERS_EVENTMSGS_EXT_MASK_OFFSET), sizeof(uint32));
	/* send user mask size up to WL_EVENTING_MASK_MAX_LEN */
	eventmsgs = (ecounters_eventmsgs_ext_t*)malloc(eventmsgs_len);

	if (eventmsgs == NULL) {
		fprintf(stderr, "fail to allocate event_msgs"
			"structure of %d bytes\n", eventmsgs_len);
		return BCME_NOMEM;
	}
	memset((void*)eventmsgs, 0, eventmsgs_len);
	err = hexstrtobitvec(mask_arg, eventmsgs->mask, masksize);
	if (err) {
		fprintf(stderr, "hexstrtobitvec() error: %d\n", err);
		free(eventmsgs);
		return BCME_BADARG;
	}
	eventmsgs->len = masksize;
	eventmsgs->version = ECOUNTERS_EVENTMSGS_VERSION_1;

	/* if all entries in mask = 0 => disable command
	 * the tags do not matter in this case
	 * A disable command results in ecounters config release in dongle
	 */
	for (i = 0; i < eventmsgs->len; i++) {
		if (eventmsgs->mask[i])
			break;
	}

	if (i == eventmsgs->len) {
		/* Send only mask no trigger config */
		iovar_data_ptr = (uint8*)eventmsgs;
		trig_config_len = 0;
	}
	else {
		/* look for -s */
		if ((*argv) && !strcmp(*argv, "-s")) {
			set_arg = *++argv;
			if (!set_arg) {
				if (eventmsgs)
					free(eventmsgs);
				fprintf(stderr, "Missing argument to -s option\n");
				return BCME_USAGE_ERROR;
			}
			/* Consumed 2 arguments -s and its argument */
			argc -= 2;
			argv++;
		}
		else {
			if (eventmsgs)
				free(eventmsgs);
			fprintf(stderr, "Missing -s <set> option\n");
			return BCME_USAGE_ERROR;
		}

		set = atoi(set_arg);
		/* look for -t */
		if ((*argv) && !strcmp(*argv, "-t")) {
			/* Consumed only one argument. This will result in argc = 0
			 * but since there is no more argv, the if condition below
			 * will be satisfied and we will get out.
			 */
			argc--;
			if (!(*++argv)) {
				if (eventmsgs)
					free(eventmsgs);
				fprintf(stderr, "argc: %d Missing argument to -t option\n", argc);
				return BCME_USAGE_ERROR;
			}
		}
		else {
			if (eventmsgs)
				free(eventmsgs);
			fprintf(stderr, "Missing -t <tag list> option\n");
			return BCME_USAGE_ERROR;
		}

		trig_config_len = ECOUNTERS_TRIG_CONFIG_TYPE_OFFSET;
		trig_config_len += argc * sizeof(uint16);

		iovar_data_ptr = (uint8*)malloc(trig_config_len + eventmsgs_len);
		if (iovar_data_ptr == NULL) {
			fprintf(stderr, "fail to allocate memory to populate iovar"
				" %d bytes\n", eventmsgs_len + trig_config_len);
			free(eventmsgs);
			return BCME_NOMEM;
		}

		/* copy eventmsgs struct in the allocate dmemory */
		memcpy(iovar_data_ptr, eventmsgs, eventmsgs_len);
		trigger_config = (ecounters_trigger_config_t *)(iovar_data_ptr + eventmsgs_len);

		trigger_config->version = ECOUNTERS_TRIGGER_CONFIG_VERSION_1;
		trigger_config->set = set;
		trigger_config->ntypes = argc;

		/* we don't need this anymore. Release any associated memories */
		free(eventmsgs);

		for (i = 0; i < argc; i++, argv++) {
			char *endptr;
			trigger_config->type[i] = strtoul(*argv, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "Type '%s' (tag %d) not a number?\n", *argv, i);
				free(iovar_data_ptr);
				return BCME_ERROR;
			}
		}
	}

	err = wlu_var_setbuf(wl, cmd->name, iovar_data_ptr, (eventmsgs_len + trig_config_len));
	free(iovar_data_ptr);
	return err;
}

/* module initialization */
void
wluc_ecounters_module_init(void)
{
	/* register ecounters commands */
	wl_module_cmds_register(wl_ecounters_cmds);
}
