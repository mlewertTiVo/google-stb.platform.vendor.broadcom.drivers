/*
 * wl msch command module
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
 * $Id: wlu_msch.c 502986 2014-09-16 23:06:58Z $
 */

#ifdef WIN32
#include <windows.h>
#endif

#include <wlioctl.h>


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
#include "wlu_common.h"
#include "wlu.h"

#ifdef WIN32
#define bzero(b, len)	memset((b), 0, (len))
#endif

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_packet.h>

#include <miniopt.h>
#include <errno.h>

static cmd_func_t wl_msch_event_check;
static cmd_func_t wl_msch_request;
static cmd_func_t wl_msch_collect;
static cmd_func_t wl_msch_dump;
static cmd_func_t wl_msch_profiler;

static char *mschbufp = NULL, *mschdata = NULL;
static FILE *mschfp = NULL;

static cmd_t wl_msch_cmds[] = {
	{ "msch_req", wl_msch_request, WLC_GET_VAR, WLC_SET_VAR,
	"register multiple channel scheduler \n"
	"\tUsage: msch_req <id> -w wlc_index -c \"channel_list\" -f flags -t type [params] "
	"-p priority -s start_time -d duration -i interval"
	"\t-w W, --wlc_index=W\tset wlc index\n"
	"\t-c L, --channels=L\tcomma or space separated list of channels to scheduler\n"
	"\t-f F, --flags=F\tset flags, 1 for contiguous cahhhel scheduler\n"
	"\t-t T, --type=T\tset scheduler type: fix, sf, df dur-flex, bf params\n\n"
	"\t-p P, --priority=P\tpriority for the scheduler\n"
	"\t-s S, --start-time=S\tstart time(ms) for the scheduler\n"
	"\t-d D, --duration=D\tduration(ms) for the scheduler\n"
	"\t-i I, --interval=I\tinterval(ms) for the scheduler\n"},

	{ "msch_collect", wl_msch_collect, WLC_GET_VAR, WLC_SET_VAR,
	"control multiple channel scheduler profiler saving data\n"
	"\tUsage: msch_collect {enable|disable} [+/-register] [+/-callback] [+/-profiler]\n"
	"\tenable: alloc memory to save profiler data\n"
	"\tdisable: stop monitor, and free memory\n"
	"\t+/-register: save or skip register data\n"
	"\t+/-callback: save or skip register data\n"
	"\t+/-profiler: save or skip profiler data\n"
	"\t+/-error: save or skip error messages\n"
	"\t+/-debug: save or skip debug messages\n"
	"\t+/-info: save or skip infomation messages\n"
	"\t+/-trace: save or skip trace messages\n"},

	{ "msch_dump", wl_msch_dump, WLC_GET_VAR, WLC_SET_VAR,
	"dump multiple channel scheduler profiler data\n"
	"\tUsage: msch_dump [filename]\n"},

	{ "msch_profiler", wl_msch_profiler, WLC_GET_VAR, WLC_SET_VAR,
	"dump multiple channel scheduler profiler data\n"
	"\tUsage: msch_profiler [uploadfilename] [filename]\n"},

	{ "msch_event", wl_msch_collect, WLC_GET_VAR, WLC_SET_VAR,
	"control multiple channel scheduler profiler event data\n"
	"\tUsage: msch_profiler disable [+/-register] [+/-callback] [+/-profiler]\n"
	"\tdisable: stop rofile event monitor\n"
	"\t+/-register: send or skip register event\n"
	"\t+/-callback: send or skip callback event\n"
	"\t+/-profiler: send or skip profiler data event\n"},

	{ "msch_event_check", wl_msch_event_check, -1, -1,
	"Listen and print Multiple channel scheduler events\n"
	"\tmsch_event_check syntax is: msch_event_check ifname [filename]"
	"[+/-register] [+/-callback] [+/-profiler]\n"},
	{ NULL, NULL, 0, 0, NULL }
};

/* module initialization */
void
wluc_msch_module_init(void)
{
	/* get the global buf */
	if (WLC_IOCTL_MAXLEN >= 2 * WLC_PROFILER_BUFFER_SIZE) {
		mschdata = wl_get_buf();
	} else {
		mschdata = (char *)malloc(2 * WLC_PROFILER_BUFFER_SIZE);
		if (!mschdata)
			return;
	}
	mschbufp = (char *)&mschdata[WLC_PROFILER_BUFFER_SIZE];

	/* register proxd commands */
	wl_module_cmds_register(wl_msch_cmds);
}

static int
wl_parse_msch_chanspec_list(char *list_str, chanspec_t *chanspec_list, int chanspec_num)
{
	int num = 0;
	chanspec_t chanspec;
	char *next, str[8];
	size_t len;

	if ((next = list_str) == NULL)
		return BCME_ERROR;

	while ((len = strcspn(next, " ,")) > 0) {
		if (len >= sizeof(str)) {
			fprintf(stderr, "string \"%s\" before ',' or ' ' is too long\n", next);
			return BCME_ERROR;
		}
		strncpy(str, next, len);
		str[len] = 0;
		chanspec = wf_chspec_aton(str);
		if (chanspec == 0) {
			fprintf(stderr, "could not parse chanspec starting at "
			        "\"%s\" in list:\n%s\n", str, list_str);
			return BCME_ERROR;
		}
		if (num == chanspec_num) {
			fprintf(stderr, "too many chanspecs (more than %d) in chanspec list:\n%s\n",
				chanspec_num, list_str);
			return BCME_ERROR;
		}
		chanspec_list[num++] = htodchanspec(chanspec);
		next += len;
		next += strspn(next, " ,");
	}

	return num;
}

static int
wl_parse_msch_bf_param(char *param_str, uint32 *param)
{
	int num;
	uint32 val;
	char* str;
	char* endptr = NULL;

	if (param_str == NULL)
		return BCME_ERROR;

	str = param_str;
	num = 0;
	while (*str != '\0') {
		val = (uint32)strtol(str, &endptr, 0);
		if (endptr == str) {
			fprintf(stderr,
				"could not parse bf param starting at"
				" substring \"%s\" in list:\n%s\n",
				str, param_str);
			return -1;
		}
		str = endptr + strspn(endptr, " ,");

		if (num >= 4) {
			fprintf(stderr, "too many bf param (more than 6) in param str:\n%s\n",
				param_str);
			return BCME_ERROR;
		}

		param[num++] = htod32(val);
	}

	return num;
}

static int
wl_msch_request(void *wl, cmd_t *cmd, char **argv)
{
	int params_size = sizeof(wl_msch_register_params_t);
	wl_msch_register_params_t *params;
	uint32 val = 0;
	uint16 val16;
	char key[64];
	int keylen;
	char *p, *eq, *valstr, *endptr = NULL;
	char opt;
	bool good_int;
	int err = 0;
	int i;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	params = (wl_msch_register_params_t*)malloc(params_size);
	if (params == NULL) {
		fprintf(stderr, "Error allocating %d bytes for msch register params\n",
			params_size);
		return BCME_NOMEM;
	}
	memset(params, 0, params_size);

	/* skip the command name */
	argv++;

	params->id = 0;

	while ((p = *argv) != NULL) {
		argv++;
		memset(key, 0, sizeof(key));
		opt = '\0';
		valstr = NULL;
		good_int = FALSE;

		if (!strncmp(p, "--", 2)) {
			eq = strchr(p, '=');
			if (eq == NULL) {
				fprintf(stderr,
				"wl_msch_request: missing \" = \" in long param \"%s\"\n", p);
				err = BCME_USAGE_ERROR;
				goto exit;
			}
			keylen = eq - (p + 2);
			if (keylen > 63)
				keylen = 63;
			memcpy(key, p + 2, keylen);

			valstr = eq + 1;
			if (*valstr == '\0') {
				fprintf(stderr, "wl_msch_request: missing value after "
					"\" = \" in long param \"%s\"\n", p);
				err = BCME_USAGE_ERROR;
				goto exit;
			}
		}
		else if (!strncmp(p, "-", 1)) {
			opt = p[1];
			if (strlen(p) > 2) {
				fprintf(stderr, "wl_msch_request: only single char options, "
					"error on param \"%s\"\n", p);
				err = BCME_BADARG;
				goto exit;
			}
			if (*argv == NULL) {
				fprintf(stderr,
				"wl_msch_request: missing value parameter after \"%s\"\n", p);
				err = BCME_USAGE_ERROR;
				goto exit;
			}
			valstr = *argv;
			argv++;
		} else {
			val = (uint32)strtol(p, &endptr, 0);
			if (*endptr == '\0') {
				/* not all the value string was parsed by strtol */
				good_int = TRUE;
			}
			if (!good_int) {
				fprintf(stderr,
				"wl_msch_request: parameter error \"%s\"\n", p);
				err = BCME_BADARG;
				goto exit;
			}
			val16 = (uint16)val;
			params->id = htod16(val16);
			continue;
		}

		/* parse valstr as int just in case */
		val = (uint32)strtol(valstr, &endptr, 0);
		if (*endptr == '\0') {
			/* not all the value string was parsed by strtol */
			good_int = TRUE;
		}

		if (opt == 'w' || !strcmp(key, "wlc_index")) {
			if (!good_int) {
				fprintf(stderr,
				"could not parse \"%s\" as an wlc_index\n",
					valstr);
				err = BCME_BADARG;
				goto exit;
			}
			val16 = (uint16)val;
			params->wlc_index = htod16(val16);
		} else if (opt == 'c' || !strcmp(key, "channels")) {
			i = wl_parse_msch_chanspec_list(valstr, params->chanspec_list,
			                              WL_NUMCHANNELS);
			if (i == BCME_ERROR) {
				fprintf(stderr, "error parsing channel list arg\n");
				err = BCME_BADARG;
				goto exit;
			}
			val = (uint32)i;
			params->chanspec_cnt = htod32(val);
		} else if (opt == 'n' || !strcmp(key, "id")) {
			if (!good_int) {
				fprintf(stderr,
				"could not parse \"%s\" as an register id\n",
					valstr);
				err = BCME_BADARG;
				goto exit;
			}
			val16 = (uint16)val;
			params->id = htod16(val16);
		} else if (opt == 'f' || !strcmp(key, "flags")) {
			if (!good_int) {
				fprintf(stderr,
				"could not parse \"%s\" as an flages\n",
					valstr);
				err = BCME_BADARG;
				goto exit;
			}
			val16 = (uint16)val;
			params->flags = htod16(val16);
		} else if (opt == 't' || !strcmp(key, "type")) {
			if (!strcmp(valstr, "f") || !strncmp(valstr, "fix", 3)) {
				params->req_type = (uint32)MSCH_REG_BOTH_FIXED;
			} else if (!strcmp(valstr, "sf") || !strcmp(valstr, "start-flex")) {
				params->req_type = (uint32)MSCH_REG_START_FLEX;
			} else if (!strcmp(valstr, "df") || !strcmp(valstr, "dur-flex")) {
				if (*argv == NULL) {
					fprintf(stderr,
					"wl_msch_request: missing param of dur-flex\n");
					err = BCME_USAGE_ERROR;
					goto exit;
				}
				valstr = *argv;
				argv++;

				val = (uint32)strtol(valstr, &endptr, 0);
				if (*endptr != '\0') {
					fprintf(stderr,
					"could not parse \"%s\" as dur-flex value\n", valstr);
					err = BCME_USAGE_ERROR;
					goto exit;
				}

				params->req_type = (uint32)MSCH_REG_DUR_FLEX;
				params->dur_flex = htod32(val);
			} else if (!strcmp(valstr, "bf") || !strcmp(valstr, "both-flex")) {
				if (*argv == NULL) {
					fprintf(stderr,
					"wl_msch_request: missing param of both-flex\n");
					err = BCME_USAGE_ERROR;
					goto exit;
				}
				valstr = *argv;
				argv++;

				if (wl_parse_msch_bf_param(valstr, &params->min_dur) != 4) {
					fprintf(stderr, "error parsing both flex params\n");
					err = BCME_BADARG;
					goto exit;
				}
				params->req_type = (uint32)MSCH_REG_BOTH_FLEX;
			} else {
				fprintf(stderr,
				"error type param \"%s\"\n",
					valstr);
				err = BCME_BADARG;
				goto exit;
			}

			params->req_type = htod32(params->req_type);
		} else if (opt == 'p' || !strcmp(key, "priority")) {
			if (!good_int) {
				fprintf(stderr,
				"could not parse \"%s\" as priority\n",
					valstr);
				err = BCME_BADARG;
				goto exit;
			}
			val16 = (uint16)val;
			params->priority = htod16(val16);
		} else if (opt == 's' || !strcmp(key, "start-time")) {
			if (!good_int) {
				fprintf(stderr,
				"could not parse \"%s\" as start time\n",
					valstr);
				err = BCME_BADARG;
				goto exit;
			}
			params->start_time = htod32(val);
		} else if (opt == 'd' || !strcmp(key, "duration")) {
			if (!good_int) {
				fprintf(stderr,
				"could not parse \"%s\" as duration\n",
					valstr);
				err = BCME_BADARG;
				goto exit;
			}
			params->duration = htod32(val);
		} else if (opt == 'i' || !strcmp(key, "interval")) {
			if (!good_int) {
				fprintf(stderr,
				"could not parse \"%s\" as interval\n",
					valstr);
				err = BCME_BADARG;
				goto exit;
			}
			params->interval = htod32(val);
		} else {
			fprintf(stderr,
			"wl_msch_request: error option param \"%s\"\n", p);
			err = BCME_BADARG;
			goto exit;
		}
	}

	err = wlu_var_setbuf(wl, cmd->name, params, params_size);
exit:
	free(params);
	return err;
}

static int
wl_msch_collect(void *wl, cmd_t *cmd, char **argv)
{
	bool eventcmd = (!strcmp(cmd->name, "msch_event"));
	char *p, *endptr;
	int opt, v, val, err = BCME_OK;

	UNUSED_PARAMETER(wl);

	if ((err = wlu_iovar_getint(wl, cmd->name, &val)) < 0)
		return err;

	err = BCME_OK;
	if (!*++argv) {
		printf("MSCH %s: %sable", (eventcmd? "Event" : "Profile"),
			((val & MSCH_CMD_ENABLE_BIT)? "En" : "Dis"));
		if (val & MSCH_CMD_ENABLE_BIT) {
			if (val & MSCH_CMD_REGISTER_BIT)
				printf(" +register");
			if (val & MSCH_CMD_CALLBACK_BIT)
				printf(" +callback");
			if (val & MSCH_CMD_PROFILE_BIT)
				printf(" +profiler");
			if (val & MSCH_CMD_ERROR_BIT)
				printf(" +error");
			if (val & MSCH_CMD_DEBUG_BIT)
				printf(" +debug");
			if (val & MSCH_CMD_INFOM_BIT)
				printf(" +info");
			if (val & MSCH_CMD_TRACE_BIT)
				printf(" +trace");
		}
		printf("\n");
		return err;
	}

	while ((p = *argv) != NULL) {
		opt = 0;
		if (p[0] == '+') {
			opt = 1;
			p++;
		}
		else if (p[0] == '-') {
			opt = 2;
			p++;
		}

		if (opt == 0) {
			v = (uint32)strtol(p, &endptr, 0);
			if (*endptr == '\0') {
				if (v == 1)
					val |= MSCH_CMD_ENABLE_BIT;
				else if (v == 0)
					val &= ~MSCH_CMD_ENABLE_BIT;
				else {
					err = BCME_EPERM;
					break;
				}
			}
			else if (!strcmp(p, "enable") || !strcmp(p, "start")) {
				val |= MSCH_CMD_ENABLE_BIT;
			}
			else if (!strcmp(p, "disable") || !strcmp(p, "stop"))
				val &= ~MSCH_CMD_ENABLE_BIT;
			else if (!eventcmd && !strcmp(p, "dump"))
				return wl_msch_dump(wl, cmd, argv);
			else {
				err = BCME_EPERM;
				break;
			}
		} else {
			if (!strcmp(p, "profiler") || !strcmp(p, "p")) {
				if (opt == 1)
					val |= MSCH_CMD_PROFILE_BIT;
				else
					val &= ~MSCH_CMD_PROFILE_BIT;
			}
			else if (!strcmp(p, "callback") || !strcmp(p, "c")) {
				if (opt == 1)
					val |= MSCH_CMD_CALLBACK_BIT;
				else
					val &= ~MSCH_CMD_CALLBACK_BIT;
			}
			else if (!strcmp(p, "register") || !strcmp(p, "r")) {
				if (opt == 1)
					val |= MSCH_CMD_REGISTER_BIT;
				else
					val &= ~MSCH_CMD_REGISTER_BIT;
			}
			else if (!strcmp(p, "error") || !strcmp(p, "e")) {
				if (opt == 1)
					val |= MSCH_CMD_ERROR_BIT;
				else
					val &= ~MSCH_CMD_ERROR_BIT;
			}
			else if (!strcmp(p, "debug") || !strcmp(p, "d")) {
				if (opt == 1)
					val |= MSCH_CMD_DEBUG_BIT;
				else
					val &= ~MSCH_CMD_DEBUG_BIT;
			}
			else if (!strcmp(p, "info") || !strcmp(p, "i")) {
				if (opt == 1)
					val |= MSCH_CMD_INFOM_BIT;
				else
					val &= ~MSCH_CMD_INFOM_BIT;
			}
			else if (!strcmp(p, "trace") || !strcmp(p, "t")) {
				if (opt == 1)
					val |= MSCH_CMD_TRACE_BIT;
				else
					val &= ~MSCH_CMD_TRACE_BIT;
			}
			else if (opt == 2 && !strcmp(p, "s") && *++argv) {
				p = *argv;
				v = (uint32)strtol(p, &endptr, 0);
				if (*endptr == '\0') {
					val &= ~MSCH_CMD_SIZE_MASK;
					val |= (v << MSCH_CMD_SIZE_SHIFT);
				} else {
					err = BCME_EPERM;
					break;
				}
			}
			else {
				err = BCME_EPERM;
				break;
			}
		}

		argv++;
	}

	val &= ~MSCH_CMD_VER_MASK;
	val |= (MSCH_PROFILER_VER << MSCH_CMD_VER_SHIFT);
	if (err == BCME_OK)
		err = wlu_iovar_setint(wl, cmd->name, val);

	return err;
}

#define MSCH_EVENTS_PRINT(nbytes) \
	do { \
		printf("%s", mschbufp); \
		if (mschfp) \
			fwrite(mschbufp, 1, nbytes, mschfp); \
	} while (0)

#define MSCH_EVENTS_SPPRINT(space) \
	do { \
		if (space > 0) { \
			int ii; \
			for (ii = 0; ii < space; ii++) mschbufp[ii] = ' '; \
			mschbufp[space] = '\0'; \
			MSCH_EVENTS_PRINT(space); \
		} \
	} while (0)

#define MSCH_EVENTS_PRINTF(fmt) \
	do { \
		int nbytes = snprintf(mschbufp, WLC_PROFILER_BUFFER_SIZE, fmt); \
		MSCH_EVENTS_PRINT(nbytes); \
	} while (0)

#define MSCH_EVENTS_PRINTF1(fmt, a) \
	do { \
		int nbytes = snprintf(mschbufp, WLC_PROFILER_BUFFER_SIZE, fmt, \
			(a)); \
		MSCH_EVENTS_PRINT(nbytes); \
	} while (0)

#define MSCH_EVENTS_PRINTF2(fmt, a, b) \
	do { \
		int nbytes = snprintf(mschbufp, WLC_PROFILER_BUFFER_SIZE, fmt, \
			(a), (b)); \
		MSCH_EVENTS_PRINT(nbytes); \
	} while (0)

#define MSCH_EVENTS_PRINTF3(fmt, a, b, c) \
	do { \
		int nbytes = snprintf(mschbufp, WLC_PROFILER_BUFFER_SIZE, fmt, \
			(a), (b), (c)); \
		MSCH_EVENTS_PRINT(nbytes); \
	} while (0)

#define MSCH_EVENTS_PRINTF4(fmt, a, b, c, d) \
	do { \
		int nbytes = snprintf(mschbufp, WLC_PROFILER_BUFFER_SIZE, fmt, \
			(a), (b), (c), (d)); \
		MSCH_EVENTS_PRINT(nbytes); \
	} while (0)

#define MSCH_EVENTS_PRINTF5(fmt, a, b, c, d, e) \
	do { \
		int nbytes = snprintf(mschbufp, WLC_PROFILER_BUFFER_SIZE, fmt, \
			(a), (b), (c), (d), (e)); \
		MSCH_EVENTS_PRINT(nbytes); \
	} while (0)

#define MSCH_EVENTS_SPPRINTF(space, fmt) \
	do { \
		MSCH_EVENTS_SPPRINT(space); \
		MSCH_EVENTS_PRINTF(fmt); \
	} while (0)

#define MSCH_EVENTS_SPPRINTF1(space, fmt, a) \
	do { \
		MSCH_EVENTS_SPPRINT(space); \
		MSCH_EVENTS_PRINTF1(fmt, (a)); \
	} while (0)

#define MSCH_EVENTS_SPPRINTF2(space, fmt, a, b) \
	do { \
		MSCH_EVENTS_SPPRINT(space); \
		MSCH_EVENTS_PRINTF2(fmt, (a), (b)); \
	} while (0)

#define MSCH_EVENTS_SPPRINTF3(space, fmt, a, b, c) \
	do { \
		MSCH_EVENTS_SPPRINT(space); \
		MSCH_EVENTS_PRINTF3(fmt, (a), (b), (c)); \
	} while (0)

#define MSCH_EVENTS_SPPRINTF4(space, fmt, a, b, c, d) \
	do { \
		MSCH_EVENTS_SPPRINT(space); \
		MSCH_EVENTS_PRINTF4(fmt, (a), (b), (c), (d)); \
	} while (0)

#define MSCH_EVENTS_SPPRINTF5(space, fmt, a, b, c, d, e) \
	do { \
		MSCH_EVENTS_SPPRINT(space); \
		MSCH_EVENTS_PRINTF5(fmt, (a), (b), (c), (d), (e)); \
	} while (0)

static char *wl_msch_display_time(uint32 time_h, uint32 time_l)
{
	static char display_time[32];
	uint64 t;
	uint32 s, ss;

	if (time_h == 0xffffffff && time_l == 0xffffffff) {
		snprintf(display_time, 31, "-1");
	} else {
		t = ((uint64)(ntoh32(time_h)) << 32) | ntoh32(time_l);
		s = (uint32)(t / 1000000);
		ss = (uint32)(t % 1000000);
		snprintf(display_time, 31, "%d.%06d", s, ss);
	}
	return display_time;
}

static void
wl_msch_chanspec_list(int sp, char *data, uint16 ptr, uint16 chanspec_cnt)
{
	int i, cnt = (int)ntoh16(chanspec_cnt);
	uint16 *chanspec_list = (uint16 *)(data + ntoh16(ptr));
	char buf[64];
	chanspec_t c;

	MSCH_EVENTS_SPPRINTF(sp, "<chanspec_list>:");
	for (i = 0; i < cnt; i++) {
		c = (chanspec_t)ntoh16(chanspec_list[i]);
		MSCH_EVENTS_PRINTF1(" %s", wf_chspec_ntoa(c, buf));
	}
	MSCH_EVENTS_PRINTF("\n");
}

static void
wl_msch_elem_list(int sp, char *title, char *data, uint16 ptr, uint16 list_cnt)
{
	int i, cnt = (int)ntoh16(list_cnt);
	uint32 *list = (uint32 *)(data + ntoh16(ptr));

	MSCH_EVENTS_SPPRINTF1(sp, "%s_list: ", title);
	for (i = 0; i < cnt; i++) {
		MSCH_EVENTS_PRINTF1("0x%08x->", ntoh32(list[i]));
	}
	MSCH_EVENTS_PRINTF("null\n");
}

static void
wl_msch_req_param_profiler_data(int sp, char *data, uint16 ptr)
{
	int sn = sp + 4;
	msch_req_param_profiler_data_t *p = (msch_req_param_profiler_data_t *)(data + ntoh16(ptr));
	uint32 type;

	MSCH_EVENTS_SPPRINTF(sp, "<request parameters>\n");
	MSCH_EVENTS_SPPRINTF(sn, "req_type: ");
	type = p->req_type;
	if (type < 4) {
		char *req_type[] = {"fixed", "start-flexible", "duration-flexible",
			"both-flexible"};
		MSCH_EVENTS_PRINTF1("%s", req_type[type]);
	}
	else
		MSCH_EVENTS_PRINTF1("unknown(%d)", type);
	if (p->flags)
		MSCH_EVENTS_PRINTF(", channel");
	else
		MSCH_EVENTS_PRINTF(", no");
	MSCH_EVENTS_PRINTF1(" contiguous, priority: %d\n", p->priority);

	MSCH_EVENTS_SPPRINTF3(sn, "start-time: %s, duration: %d(us), interval: %d(us)\n",
		wl_msch_display_time(p->start_time_h, p->start_time_l),
		ntoh32(p->duration), ntoh32(p->interval));

	if (type == 2)
		MSCH_EVENTS_SPPRINTF1(sn, "dur_flex: %d(us)\n", ntoh32(p->flex.dur_flex));
	else if (type == 3) {
		MSCH_EVENTS_SPPRINTF2(sn, "min_dur: %d(us), max_away_dur: %d(us)\n",
			ntoh32(p->flex.bf.min_dur), ntoh32(p->flex.bf.max_away_dur));

		MSCH_EVENTS_SPPRINTF2(sn, "hi_prio_time: %s, hi_prio_interval: %d(us)\n",
			wl_msch_display_time(p->flex.bf.hi_prio_time_h,
			p->flex.bf.hi_prio_time_l),
			ntoh32(p->flex.bf.hi_prio_interval));
	}
}

static void
wl_msch_timeslot_profiler_data(int sp, char *title, char *data, uint16 ptr, bool empty)
{
	int s, sn = sp + 4;
	msch_timeslot_profiler_data_t *p = (msch_timeslot_profiler_data_t *)(data + ntoh16(ptr));
	char *state[] = {"NONE", "CHN_SW", "ONCHAN_FIRE", "OFF_CHN_PREP",
		"OFF_CHN_DONE", "TS_COMPLETE"};

	MSCH_EVENTS_SPPRINTF1(sp, "<%s timeslot>: ", title);
	if (empty) {
		MSCH_EVENTS_PRINTF(" null\n");
		return;
	}
	else
		MSCH_EVENTS_PRINTF1("0x%08x\n", ntoh32(p->p_timeslot));

	s = (int)(ntoh32(p->state));
	if (s > 5) s = 0;

	MSCH_EVENTS_SPPRINTF4(sn, "id: %d, state[%d]: %s, chan_ctxt: [0x%08x]\n",
		ntoh32(p->timeslot_id), ntoh32(p->state), state[s], ntoh32(p->p_chan_ctxt));

	MSCH_EVENTS_SPPRINTF1(sn, "fire_time: %s",
		wl_msch_display_time(p->fire_time_h, p->fire_time_l));

	MSCH_EVENTS_PRINTF1(", pre_start_time: %s",
		wl_msch_display_time(p->pre_start_time_h, p->pre_start_time_l));

	MSCH_EVENTS_PRINTF1(", end_time: %s",
		wl_msch_display_time(p->end_time_h, p->end_time_l));

	MSCH_EVENTS_PRINTF1(", sch_dur: %s\n",
		wl_msch_display_time(p->sch_dur_h, p->sch_dur_l));
}

#define MSCH_RC_FLAGS_ONCHAN_FIRE		(1 << 0)
#define MSCH_RC_FLAGS_START_FIRE_DONE		(1 << 1)
#define MSCH_RC_FLAGS_END_FIRE_DONE		(1 << 2)
#define MSCH_RC_FLAGS_ONFIRE_DONE		(1 << 3)

static void
wl_msch_req_timing_profiler_data(int sp, char *title, char *data, uint16 ptr, bool empty)
{
	int sn = sp + 4;
	msch_req_timing_profiler_data_t *p =
		(msch_req_timing_profiler_data_t *)(data + ntoh16(ptr));
	uint32 type;

	MSCH_EVENTS_SPPRINTF1(sp, "<%s req_timing>: ", title);
	if (empty) {
		MSCH_EVENTS_PRINTF(" null\n");
		return;
	}
	else
		MSCH_EVENTS_PRINTF3("0x%08x (prev 0x%08x, next 0x%08x)\n",
			ntoh32(p->p_req_timing), ntoh32(p->p_prev), ntoh32(p->p_next));

	MSCH_EVENTS_SPPRINTF(sn, "flags:");
	type = ntoh16(p->flags);
	if ((type & 0x3f) == 0)
		MSCH_EVENTS_PRINTF(" NONE");
	else {
		if (type & MSCH_RC_FLAGS_ONCHAN_FIRE)
			MSCH_EVENTS_PRINTF(" ONCHAN_FIRE");
		if (type & MSCH_RC_FLAGS_START_FIRE_DONE)
			MSCH_EVENTS_PRINTF(" START_FIRE");
		if (type & MSCH_RC_FLAGS_END_FIRE_DONE)
			MSCH_EVENTS_PRINTF(" END_FIRE");
		if (type & MSCH_RC_FLAGS_ONFIRE_DONE)
			MSCH_EVENTS_PRINTF(" ONFIRE_DONE");
	}
	MSCH_EVENTS_PRINTF("\n");

	MSCH_EVENTS_SPPRINTF1(sn, "pre_start_time: %s",
		wl_msch_display_time(p->pre_start_time_h, p->pre_start_time_l));

	MSCH_EVENTS_PRINTF1(", start_time: %s",
		wl_msch_display_time(p->start_time_h, p->start_time_l));

	MSCH_EVENTS_PRINTF1(", end_time: %s\n",
		wl_msch_display_time(p->end_time_h, p->end_time_l));

	if (p->p_timeslot && (p->timeslot_ptr == 0))
		MSCH_EVENTS_SPPRINTF2(sn, "<%s timeslot>: 0x%08x\n", title, ntoh32(p->p_timeslot));
	else
		wl_msch_timeslot_profiler_data(sn, title, data, p->timeslot_ptr,
			(p->timeslot_ptr == 0));
}

static void
wl_msch_chan_ctxt_profiler_data(int sp, char *data, uint16 ptr, bool empty)
{
	int sn = sp + 4;
	msch_chan_ctxt_profiler_data_t *p = (msch_chan_ctxt_profiler_data_t *)(data + ntoh16(ptr));
	chanspec_t c;
	char buf[64];

	MSCH_EVENTS_SPPRINTF(sp, "<chan_ctxt>: ");
	if (empty) {
		MSCH_EVENTS_PRINTF(" null\n");
		return;
	}
	else
		MSCH_EVENTS_PRINTF3("0x%08x (prev 0x%08x, next 0x%08x)\n",
			ntoh32(p->p_chan_ctxt), ntoh32(p->p_prev), ntoh32(p->p_next));

	c = (chanspec_t)ntoh16(p->chanspec);
	MSCH_EVENTS_SPPRINTF3(sn, "channel: %s, bf_sch_pending: %s, bf_skipped: %d\n",
		wf_chspec_ntoa(c, buf), p->bf_sch_pending? "TRUE" : "FALSE",
		ntoh32(p->bf_skipped_count));
	MSCH_EVENTS_SPPRINTF2(sn, "bf_link: prev 0x%08x, next 0x%08x\n",
		ntoh32(p->bf_link_prev), ntoh32(p->bf_link_next));

	MSCH_EVENTS_SPPRINTF1(sn, "onchan_time: %s",
		wl_msch_display_time(p->onchan_time_h, p->onchan_time_l));

	MSCH_EVENTS_PRINTF1(", actual_onchan_dur: %s",
		wl_msch_display_time(p->actual_onchan_dur_h, p->actual_onchan_dur_l));

	MSCH_EVENTS_PRINTF1(", pend_onchan_dur: %s\n",
		wl_msch_display_time(p->pend_onchan_dur_h, p->pend_onchan_dur_l));

	wl_msch_elem_list(sn, "req_entity", data, p->req_entity_list_ptr, p->req_entity_list_cnt);
	wl_msch_elem_list(sn, "bf_entity", data, p->bf_entity_list_ptr, p->bf_entity_list_cnt);
}

static void
wl_msch_req_entity_profiler_data(int sp, char *data, uint16 ptr, bool empty)
{
	int sn = sp + 4;
	msch_req_entity_profiler_data_t *p =
		(msch_req_entity_profiler_data_t *)(data + ntoh16(ptr));
	char buf[64];
	chanspec_t c;

	MSCH_EVENTS_SPPRINTF(sp, "<req_entity>: ");
	if (empty) {
		MSCH_EVENTS_PRINTF(" null\n");
		return;
	}
	else
		MSCH_EVENTS_PRINTF3("0x%08x (prev 0x%08x, next 0x%08x)\n",
			ntoh32(p->p_req_entity), ntoh32(p->req_hdl_link_prev),
			ntoh32(p->req_hdl_link_next));

	MSCH_EVENTS_SPPRINTF2(sn, "chan_ctxt_link: prev 0x%08x, next 0x%08x\n",
		ntoh32(p->chan_ctxt_link_prev), ntoh32(p->chan_ctxt_link_next));
	MSCH_EVENTS_SPPRINTF2(sn, "rt_specific_link: prev 0x%08x, next 0x%08x\n",
		ntoh32(p->rt_specific_link_prev), ntoh32(p->rt_specific_link_next));
	MSCH_EVENTS_SPPRINTF2(sn, "start_fixed_link: prev 0x%08x, next 0x%08x\n",
		ntoh32(p->start_fixed_link_prev), ntoh32(p->start_fixed_link_next));
	MSCH_EVENTS_SPPRINTF2(sn, "both_flex_list: prev 0x%08x, next 0x%08x\n",
		ntoh32(p->both_flex_list_prev), ntoh32(p->both_flex_list_next));

	c = (chanspec_t)ntoh16(p->chanspec);
	MSCH_EVENTS_SPPRINTF3(sn, "channel: %s, priority %d, req_hdl: [0x%08x]\n",
		wf_chspec_ntoa(c, buf), ntoh16(p->chanspec), ntoh32(p->p_req_hdl));

	MSCH_EVENTS_SPPRINTF1(sn, "bf_last_serv_time: %s\n",
		wl_msch_display_time(p->bf_last_serv_time_h, p->bf_last_serv_time_l));

	wl_msch_req_timing_profiler_data(sn, "current", data, p->cur_slot_ptr,
		(p->cur_slot_ptr == 0));
	wl_msch_req_timing_profiler_data(sn, "pending", data, p->pend_slot_ptr,
		(p->pend_slot_ptr == 0));

	if (p->p_chan_ctxt && (p->chan_ctxt_ptr == 0))
		MSCH_EVENTS_SPPRINTF1(sn, "<chan_ctxt>: 0x%08x\n", ntoh32(p->p_chan_ctxt));
	else
		wl_msch_chan_ctxt_profiler_data(sn, data, p->chan_ctxt_ptr,
			(p->chan_ctxt_ptr == 0));
}

static void
wl_msch_req_handle_profiler_data(int sp, char *data, uint16 ptr, bool empty)
{
	int sn = sp + 4;
	msch_req_handle_profiler_data_t *p =
		(msch_req_handle_profiler_data_t *)(data + ntoh16(ptr));

	MSCH_EVENTS_SPPRINTF(sp, "<req_handle>: ");
	if (empty) {
		MSCH_EVENTS_PRINTF(" null\n");
		return;
	}
	else
		MSCH_EVENTS_PRINTF3("0x%08x (prev 0x%08x, next 0x%08x)\n",
			ntoh32(p->p_req_handle), ntoh32(p->p_prev), ntoh32(p->p_next));

	wl_msch_elem_list(sn, "req_entity", data, p->req_entity_list_ptr, p->req_entity_list_cnt);
	MSCH_EVENTS_SPPRINTF2(sn, "cb_func: [0x%08x], cb_func: [0x%08x]\n",
		ntoh32(p->cb_func), ntoh32(p->cb_ctxt));

	wl_msch_req_param_profiler_data(sn, data, p->req_param_ptr);

	MSCH_EVENTS_SPPRINTF2(sn, "chan_cnt: %d, flags: 0x%04x\n",
		ntoh16(p->chan_cnt), ntoh32(p->flags));
}

static void
wl_msch_profiler_profiler_data(int sp, char *data, uint16 ptr)
{
	msch_profiler_profiler_data_t *p = (msch_profiler_profiler_data_t *)(data + ntoh16(ptr));

	MSCH_EVENTS_SPPRINTF4(sp, "free list: req_hdl 0x%08x, req_entity 0x%08x,"
		" chan_ctxt 0x%08x, chanspec 0x%08x\n",
		ntoh32(p->free_req_hdl_list), ntoh32(p->free_req_entity_list),
		ntoh32(p->free_chan_ctxt_list), ntoh32(p->free_chanspec_list));

	MSCH_EVENTS_SPPRINTF5(sp, "alloc count: chanspec %d, req_entity %d, req_hdl %d, "
		"chan_ctxt %d, timeslot %d\n",
		ntoh16(p->msch_chanspec_alloc_cnt), ntoh16(p->msch_req_entity_alloc_cnt),
		ntoh16(p->msch_req_hdl_alloc_cnt), ntoh16(p->msch_chan_ctxt_alloc_cnt),
		ntoh16(p->msch_timeslot_alloc_cnt));

	wl_msch_elem_list(sp, "req_hdl", data, p->msch_req_hdl_list_ptr,
		p->msch_req_hdl_list_cnt);
	wl_msch_elem_list(sp, "chan_ctxt", data, p->msch_chan_ctxt_list_ptr,
		p->msch_chan_ctxt_list_cnt);
	wl_msch_elem_list(sp, "req_timing", data, p->msch_req_timing_list_ptr,
		p->msch_req_timing_list_cnt);
	wl_msch_elem_list(sp, "start_fixed", data, p->msch_start_fixed_list_ptr,
		p->msch_start_fixed_list_cnt);
	wl_msch_elem_list(sp, "both_flex_req_entity", data,
		p->msch_both_flex_req_entity_list_ptr,
		p->msch_both_flex_req_entity_list_cnt);
	wl_msch_elem_list(sp, "start_flex", data, p->msch_start_flex_list_ptr,
		p->msch_start_flex_list_cnt);
	wl_msch_elem_list(sp, "both_flex", data, p->msch_both_flex_list_ptr,
		p->msch_both_flex_list_cnt);

	if (p->p_cur_msch_timeslot && (p->cur_msch_timeslot_ptr == 0))
		MSCH_EVENTS_SPPRINTF1(sp, "<cur_msch timeslot>: 0x%08x\n",
			ntoh32(p->p_cur_msch_timeslot));
	else
		wl_msch_timeslot_profiler_data(sp, "cur_msch", data, p->cur_msch_timeslot_ptr,
			(p->cur_msch_timeslot_ptr == 0));

	if (p->p_next_timeslot && (p->next_timeslot_ptr == 0))
		MSCH_EVENTS_SPPRINTF1(sp, "<next timeslot>: 0x%08x\n", ntoh32(p->p_next_timeslot));
	else
		wl_msch_timeslot_profiler_data(sp, "next", data, p->next_timeslot_ptr,
			(p->next_timeslot_ptr == 0));

	MSCH_EVENTS_SPPRINTF3(sp, "ts_id: %d, flags: 0x%08x, cur_armed_timeslot: 0x%08x\n",
		ntoh32(p->ts_id), ntoh32(p->flags), ntoh32(p->cur_armed_timeslot));
	MSCH_EVENTS_SPPRINTF3(sp, "flex_list_cnt: %d, service_interval: %d, "
		"max_lo_prio_interval: %d\n",
		ntoh16(p->flex_list_cnt), ntoh32(p->service_interval),
		ntoh32(p->max_lo_prio_interval));
}

static uint64 solt_start_time, req_start_time, profiler_start_time;
static uint32 solt_chanspec = 0, req_start = 0;
static bool lastMessages = FALSE;

#define	MSCH_CT_REQ_START	0x1
#define	MSCH_CT_ON_CHAN		0x2
#define	MSCH_CT_SLOT_START	0x4
#define	MSCH_CT_SLOT_END	0x8
#define	MSCH_CT_SLOT_SKIP	0x10
#define	MSCH_CT_OFF_CHAN	0x20
#define MSCH_CT_OFF_CHAN_DONE	0x40
#define	MSCH_CT_REQ_END		0x80

static void wl_msch_dump_data(char *data, int type)
{
	uint64 t = 0, tt = 0;
	uint32 s = 0, ss = 0;
	uint16 wlc_index;

	wlc_index = (type & WLC_PROFILER_WLINDEX_MASK) >> WLC_PROFILER_WLINDEX_SHIFT;
	type &= ~WLC_PROFILER_WLINDEX_MASK;

	if (type <= WLC_PROFILER_PROFILE_END) {
		msch_profiler_data_t *pevent = (msch_profiler_data_t *)data;
		tt = ((uint64)(ntoh32(pevent->time_hi)) << 32) | ntoh32(pevent->time_lo);
		s = (uint32)(tt / 1000000);
		ss = (uint32)(tt % 1000000);
	}

	if (lastMessages && (type != WLC_PROFILER_MESSAGE)) {
		MSCH_EVENTS_PRINTF("\n");
		lastMessages = FALSE;
	}

	switch (type) {
	case WLC_PROFILER_START:
		MSCH_EVENTS_PRINTF2("\n%06d.%06d START\n", s, ss);
		break;

	case WLC_PROFILER_EXIT:
		MSCH_EVENTS_PRINTF2("\n%06d.%06d EXIT\n", s, ss);
		break;

	case WLC_PROFILER_REQ:
	{
		msch_req_profiler_data_t *p = (msch_req_profiler_data_t *)data;
		MSCH_EVENTS_PRINTF("\n===============================\n");
		MSCH_EVENTS_PRINTF3("%06d.%06d [wl%d] REGISTER:\n", s, ss, wlc_index);
		wl_msch_req_param_profiler_data(4, data, p->req_param_ptr);
		wl_msch_chanspec_list(4, data, p->chanspec_ptr, p->chanspec_cnt);
		MSCH_EVENTS_PRINTF("===============================\n\n");
	}
		break;

	case WLC_PROFILER_CALLBACK:
	{
		msch_callback_profiler_data_t *p = (msch_callback_profiler_data_t *)data;
		char buf[64];
		uint16 cbtype;

		MSCH_EVENTS_PRINTF3("%06d.%06d [wl%d] CALLBACK: ", s, ss, wlc_index);
		ss = ntoh16(p->chanspec);
		MSCH_EVENTS_PRINTF1("channel %s --", wf_chspec_ntoa(ss, buf));

		cbtype = ntoh16(p->type);
		if (cbtype & MSCH_CT_ON_CHAN)
			MSCH_EVENTS_PRINTF(" ON_CHAN");
		if (cbtype & MSCH_CT_OFF_CHAN)
			MSCH_EVENTS_PRINTF(" OFF_CHAN");
		if (cbtype & MSCH_CT_REQ_START)
			MSCH_EVENTS_PRINTF(" REQ_START");
		if (cbtype & MSCH_CT_REQ_END)
			MSCH_EVENTS_PRINTF(" REQ_END");
		if (cbtype & MSCH_CT_SLOT_START)
			MSCH_EVENTS_PRINTF(" SLOT_START");
		if (cbtype & MSCH_CT_SLOT_SKIP)
			MSCH_EVENTS_PRINTF(" SLOT_SKIP");
		if (cbtype & MSCH_CT_SLOT_END)
			MSCH_EVENTS_PRINTF(" SLOT_END");
		if (cbtype & MSCH_CT_OFF_CHAN_DONE)
			MSCH_EVENTS_PRINTF(" OFF_CHAN_DONE");
		if (cbtype & 0x100)
			MSCH_EVENTS_PRINTF(" ADOPT_CHAN");

		if (cbtype & MSCH_CT_ON_CHAN)
			MSCH_EVENTS_PRINTF1(" ID %d", ntoh32(p->timeslot_id));
		if (cbtype & (MSCH_CT_ON_CHAN | MSCH_CT_SLOT_SKIP)) {
			t = ((uint64)(ntoh32(p->start_time_h)) << 32) |
				ntoh32(p->start_time_l);
			MSCH_EVENTS_PRINTF1(" pre-start: %s",
				wl_msch_display_time(p->start_time_h,
				p->start_time_l));
			tt = ((uint64)(ntoh32(p->end_time_h)) << 32) | ntoh32(p->end_time_l);
			MSCH_EVENTS_PRINTF2(" end: %s duration %d",
				wl_msch_display_time(p->end_time_h, p->end_time_l),
				(p->end_time_h == 0xffffffff && p->end_time_l == 0xffffffff)?
				-1 : (int)(tt - t));
		}

		if (cbtype & MSCH_CT_REQ_START) {
			req_start = 1;
			req_start_time = tt;
		} else if (cbtype & MSCH_CT_REQ_END) {
			if (req_start) {
				MSCH_EVENTS_PRINTF1(": duration %d", (uint32)(tt - req_start_time));
				req_start = 0;
			}
		}

		if (cbtype & MSCH_CT_SLOT_START) {
			solt_chanspec = p->chanspec;
			solt_start_time = tt;
		} else if (cbtype & MSCH_CT_SLOT_END) {
			if (p->chanspec == solt_chanspec) {
				MSCH_EVENTS_PRINTF1(": duration %d",
					(uint32)(tt - solt_start_time));
				solt_chanspec = 0;
			}
		}
		MSCH_EVENTS_PRINTF("\n");
	}
		break;

	case WLC_PROFILER_MESSAGE:
	{
		msch_message_profiler_data_t *p = (msch_message_profiler_data_t *)data;
		MSCH_EVENTS_PRINTF4("%06d.%06d [wl%d]: %s", s, ss, wlc_index, p->message);
		lastMessages = TRUE;
		break;
	}

	case WLC_PROFILER_PROFILE_START:
		profiler_start_time = tt;
		MSCH_EVENTS_PRINTF("-------------------------------\n");
		MSCH_EVENTS_PRINTF3("%06d.%06d [wl%d] PROFILE DATA:\n", s, ss, wlc_index);
		wl_msch_profiler_profiler_data(4, data, 0);
		break;

	case WLC_PROFILER_PROFILE_END:
		MSCH_EVENTS_PRINTF4("%06d.%06d [wl%d] PROFILE END: take time %d\n", s, ss,
			wlc_index, (uint32)(tt - profiler_start_time));
		MSCH_EVENTS_PRINTF("-------------------------------\n\n");
		break;

	case WLC_PROFILER_REQ_HANDLE:
		wl_msch_req_handle_profiler_data(4, data, 0, FALSE);
		break;

	case WLC_PROFILER_REQ_ENTITY:
		wl_msch_req_entity_profiler_data(8, data, 0, FALSE);
		break;

	case WLC_PROFILER_CHAN_CTXT:
		wl_msch_chan_ctxt_profiler_data(4, data, 0, FALSE);
		break;

	case WLC_PROFILER_TIMESLOT:
		wl_msch_timeslot_profiler_data(4, "msch", data, 0, FALSE);
		break;

	case WLC_PROFILER_REQ_TIMING:
		wl_msch_req_timing_profiler_data(4, "msch", data, 0, FALSE);
		break;

	default:
		fprintf(stderr, "[wl%d] ERROR: unsupported EVENT reason code:%d; ",
			wlc_index, type);
		break;
	}
}

int wl_msch_dump(void *wl, cmd_t *cmd, char **argv)
{
	void *ptr;
	int type;
	char *data, *fname = NULL;
	int val, err, start = 1;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	if ((err = wlu_iovar_getint(wl, "msch_collect", &val)) < 0)
		return err;

	if (!(val & MSCH_CMD_ENABLE_BIT))
		return BCME_NOTREADY;

	if (*++argv) {
		fname = *argv;
		if (!(mschfp = fopen(fname, "wb"))) {
			perror(fname);
			fprintf(stderr, "Cannot open file %s\n", fname);
			return BCME_BADARG;
		}
	}

	err = wlu_var_getbuf(wl, "msch_dump", &start, sizeof(int), &ptr);
	while (err >= 0) {
		wl_msch_collect_tlv_t *ptlv = (wl_msch_collect_tlv_t *)ptr;

		type = dtoh16(ptlv->type);
		data = ptlv->value;

		wl_msch_dump_data(data, type);

		err = wlu_var_getbuf(wl, "msch_dump", NULL, 0, &ptr);
	}

	if (mschfp) {
		fflush(mschfp);
		fclose(mschfp);
		mschfp = NULL;
	}

	fflush(stdout);
	return BCME_OK;
}


int wl_format_ssid(char* ssid_buf, uint8* ssid, int ssid_len);

int wl_msch_event_check(void *wl, cmd_t *cmd, char **argv)
{
	int fd, err, octets;
	struct sockaddr_ll sll;
	struct ifreq ifr;
	char ifnames[IFNAMSIZ] = {"eth0"};
	bcm_event_t *event;
	char *data;
	int event_type, reason;
	uint8 event_inds_mask[WL_EVENTING_MASK_LEN];
	char *p, *fname = NULL;
	int opt, val;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	if ((err = wlu_iovar_getint(wl, "msch_event", &val)) < 0)
		return err;

	if (argv[1] == NULL) {
		fprintf(stderr, "<ifname> param is missing\n");
		return -1;
	}

	if (*++argv) {
		strncpy(ifnames, *argv, (IFNAMSIZ - 1));
	}

	bzero(&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, ifnames, (IFNAMSIZ - 1));

	fd = socket(PF_PACKET, SOCK_RAW, hton16(ETHER_TYPE_BRCM));
	if (fd < 0) {
		fprintf(stderr, "Cannot create socket %d\n", fd);
		return -1;
	}

	err = ioctl(fd, SIOCGIFINDEX, &ifr);
	if (err < 0) {
		fprintf(stderr, "Cannot get iface:%s index \n", ifr.ifr_name);
		goto exit;
	}

	bzero(&sll, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_protocol = hton16(ETHER_TYPE_BRCM);
	sll.sll_ifindex = ifr.ifr_ifindex;
	err = bind(fd, (struct sockaddr *)&sll, sizeof(sll));
	if (err < 0) {
		fprintf(stderr, "Cannot bind %d\n", err);
		goto exit;
	}

	while (*++argv) {
		opt = 0;
		p = *argv;
		if (p[0] == '+') {
			opt = 1;
			p++;
		}
		else if (p[0] == '-') {
			opt = 2;
			p++;
		}

		if (opt == 0) {
			fname = p;
			if (mschfp == NULL) {
				if (!(mschfp = fopen(fname, "wb"))) {
					perror(fname);
					fprintf(stderr, "Cannot open file %s\n", fname);
					err = BCME_BADARG;
					goto exit;
				}
			}
			else {
				err = BCME_BADARG;
				fprintf(stderr, "error param: %s\n", p);
				goto exit;
			}
		} else {
			if (!strcmp(p, "profiler") || !strcmp(p, "p")) {
				if (opt == 1)
					val |= MSCH_CMD_PROFILE_BIT;
				else
					val &= ~MSCH_CMD_PROFILE_BIT;
			}
			else if (!strcmp(p, "callback") || !strcmp(p, "c")) {
				if (opt == 1)
					val |= MSCH_CMD_CALLBACK_BIT;
				else
					val &= ~MSCH_CMD_CALLBACK_BIT;
			}
			else if (!strcmp(p, "register") || !strcmp(p, "r")) {
				if (opt == 1)
					val |= MSCH_CMD_REGISTER_BIT;
				else
					val &= ~MSCH_CMD_REGISTER_BIT;
			}
			else if (!strcmp(p, "error") || !strcmp(p, "e")) {
				if (opt == 1)
					val |= MSCH_CMD_ERROR_BIT;
				else
					val &= ~MSCH_CMD_ERROR_BIT;
			}
			else if (!strcmp(p, "debug") || !strcmp(p, "d")) {
				if (opt == 1)
					val |= MSCH_CMD_DEBUG_BIT;
				else
					val &= ~MSCH_CMD_DEBUG_BIT;
			}
			else if (!strcmp(p, "info") || !strcmp(p, "i")) {
				if (opt == 1)
					val |= MSCH_CMD_INFOM_BIT;
				else
					val &= ~MSCH_CMD_INFOM_BIT;
			}
			else if (!strcmp(p, "trace") || !strcmp(p, "t")) {
				if (opt == 1)
					val |= MSCH_CMD_TRACE_BIT;
				else
					val &= ~MSCH_CMD_TRACE_BIT;
			}
			else {
				err = BCME_EPERM;
				goto exit;
			}
		}
	}

	data = &mschdata[sizeof(bcm_event_t)];

	/*  read current mask state  */
	if ((err = wlu_iovar_get(wl, "event_msgs", &event_inds_mask, WL_EVENTING_MASK_LEN))) {
		fprintf(stderr, "couldn't read event_msgs\n");
		goto exit;
	}
	event_inds_mask[WLC_E_MSCH / 8] |= (1 << (WLC_E_MSCH % 8));
	if ((err = wlu_iovar_set(wl, "event_msgs", &event_inds_mask, WL_EVENTING_MASK_LEN))) {
		fprintf(stderr, "couldn't write event_msgs\n");
		goto exit;
	}

	val &= ~MSCH_CMD_VER_MASK;
	val |= (MSCH_CMD_ENABLE_BIT | (MSCH_PROFILER_VER << MSCH_CMD_VER_SHIFT));
	if ((err = wlu_iovar_setint(wl, "msch_event", val))) {
		fprintf(stderr, "couldn't start msch event\n");
		goto exit;
	}

	printf("wating for MSCH events :%s\n", ifr.ifr_name);

	while (1) {
		fflush(stdout);
		octets = recv(fd, mschdata, WLC_PROFILER_BUFFER_SIZE, 0);

		if (octets <= 0)  {
			/* sigterm */
			err = -1;
			break;
		}

		event = (bcm_event_t *)mschdata;
		event_type = ntoh32(event->event.event_type);
		reason = ntoh32(event->event.reason);

		if ((event_type != WLC_E_MSCH)) {
			if (event_type == WLC_E_ESCAN_RESULT) {
				wl_escan_result_t* escan_data = (wl_escan_result_t*)data;
				uint16	i;
				MSCH_EVENTS_PRINTF1("MACEVENT_%d: WLC_E_ESCAN_RESULT:", event_type);
				for (i = 0; i < escan_data->bss_count; i++) {
					wl_bss_info_t *bi = &escan_data->bss_info[i];
					char ssidbuf[SSID_FMT_BUF_LEN];
					char chspec_str[CHANSPEC_STR_LEN];
					wl_format_ssid(ssidbuf, bi->SSID, bi->SSID_len);
					MSCH_EVENTS_PRINTF2(" SSID: \"%s\", Channel: %s;",
						ssidbuf, wf_chspec_ntoa(bi->chanspec, chspec_str));
				}
				MSCH_EVENTS_PRINTF("\n");
			} else {
				char *event_name;
				switch (event_type) {
				case WLC_E_JOIN:
					event_name = "WLC_E_JOIN";
					break;

				case WLC_E_AUTH:
					event_name = "WLC_E_AUTH";
					break;

				case WLC_E_ASSOC:
					event_name = "WLC_E_ASSOC";
					break;

				case WLC_E_LINK:
					event_name = "WLC_E_LINK";
					break;

				case WLC_E_ROAM:
					event_name = "WLC_E_ROAM";
					break;

				case WLC_E_SCAN_COMPLETE:
					event_name = "WLC_E_SCAN_COMPLETE";
					break;

				case WLC_E_SCAN_CONFIRM_IND:
					event_name = "WLC_E_SCAN_CONFIRM_IND";
					break;

				case WLC_E_ASSOC_REQ_IE:
					event_name = "WLC_E_ASSOC_REQ_IE";
					break;

				case WLC_E_ASSOC_RESP_IE:
					event_name = "WLC_E_ASSOC_RESP_IE";
					break;

				case WLC_E_BSSID:
					event_name = "WLC_E_BSSID";
					break;

				default:
					event_name = "Unknown Event";
				}
				MSCH_EVENTS_PRINTF2("MACEVENT_%d: %s\n", event_type, event_name);
			}
			continue;
		}

		wl_msch_dump_data(data, reason);

		if (reason == WLC_PROFILER_EXIT)
			goto exit;
	}
exit:
	/* if we ever reach here */
	close(fd);
	if (mschfp) {
		fflush(mschfp);
		fclose(mschfp);
		mschfp = NULL;
	}

	/* Read the event mask from driver and mask the event WLC_E_MSCH */
	if (!(err = wlu_iovar_get(wl, "event_msgs", &event_inds_mask, WL_EVENTING_MASK_LEN))) {
		event_inds_mask[WLC_E_MSCH / 8] &= (~(1 << (WLC_E_MSCH % 8)));
		err = wlu_iovar_set(wl, "event_msgs", &event_inds_mask, WL_EVENTING_MASK_LEN);
	}

	fflush(stdout);
	return (err);
}

typedef struct wl_msch_profiler_struct {
	uint32	start_ptr;
	uint32	write_ptr;
	uint32	write_size;
	uint32	read_ptr;
	uint32	read_size;
	uint32	total_size;
	uint32  buffer;
} wl_msch_profiler_struct_t;

#define MSCH_MAGIC_1		0x4d534348
#define MSCH_MAGIC_2		0x61676963

#undef ROUNDUP
#define	ROUNDUP(x)		(((x) + 3) & (~0x03))

#define MSCH_PROFILE_HEAD_SIZE	4
#define MAX_PROFILE_DATA_SIZE	4096

int wl_msch_profiler(void *wl, cmd_t *cmd, char **argv)
{
	char *fname = NULL, *buffer = NULL;
	FILE *fp = NULL;
	int err = BCME_OK;
	uint32 magicdata;
	uint32 rptr, rsize, wsize, tsize;
	wl_msch_profiler_struct_t profiler;
	bool found = FALSE;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	if (argv[1] == NULL) {
		fprintf(stderr, "<input filename> param is missing\n");
		return -1;
	}

	fname = *++argv;
	if (!(fp = fopen(fname, "rb"))) {
		perror(fname);
		fprintf(stderr, "Cannot open input file %s\n", fname);
		err = BCME_BADARG;
		goto exit;
	}

	rptr = 0;
	while ((rsize = fread(&magicdata, 1, sizeof(uint32), fp)) > 0) {
		rptr += rsize;
		magicdata = dtoh32(magicdata);
		if (magicdata != MSCH_MAGIC_1)
			continue;

		if ((rsize = fread(&magicdata, 1, sizeof(uint32), fp)) > 0) {
			rptr += rsize;
			magicdata = dtoh32(magicdata);
			if (magicdata != MSCH_MAGIC_2)
				continue;
		}

		rsize = fread(&profiler, 1, sizeof(wl_msch_profiler_struct_t), fp);
		rptr += rsize;
		magicdata = dtoh32(profiler.buffer);

		if (((rptr ^ magicdata) & 0xffff) == 0) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		fprintf(stderr, "Cannot find profiler data from file %s\n", fname);
		err = BCME_NOTFOUND;
		goto exit;
	}

	if (*++argv) {
		fname = *argv;
		if (!(mschfp = fopen(fname, "wb"))) {
			perror(fname);
			fprintf(stderr, "Cannot open file %s\n", fname);
			err = BCME_BADARG;
			goto exit;
		}
	}

	tsize = dtoh32(profiler.total_size);
	buffer = (char*)malloc(tsize + MAX_PROFILE_DATA_SIZE);
	if (buffer == NULL) {
		fprintf(stderr, "Cannot not allocate %d bytes for profiler buffer\n",
			tsize);
		err = BCME_NOMEM;
		goto exit;
	}

	if ((rsize = fread(buffer, 1, tsize, fp)) != tsize) {
		fprintf(stderr, "Cannot read profiler data from file %s, req %d, read %d\n",
			fname, tsize, rsize);
		err = BCME_BADARG;
		goto exit;
	}

	wsize = dtoh32(profiler.write_size);
	rptr = dtoh32(profiler.start_ptr);
	rsize = 0;

	while (rsize < wsize) {
		wl_msch_collect_tlv_t *ptlv = (wl_msch_collect_tlv_t *)(buffer + rptr);
		int type, size, remain;
		char *data;

		size = ROUNDUP(MSCH_PROFILE_HEAD_SIZE + dtoh16(ptlv->size));

		rsize += size;
		if (rsize > wsize)
			break;

		remain = tsize - rptr;
		if (remain >= size) {
			rptr += size;
			if (rptr == tsize)
				rptr = 0;
		} else {
			remain = size - remain;
			memcpy(buffer + tsize, buffer, remain);
			rptr = remain;
		}

		type = dtoh16(ptlv->type);
		data = ptlv->value;

		wl_msch_dump_data(data, type);
	}

	err = BCME_OK;

exit:
	if (fp)
		fclose(fp);

	if (buffer)
		free(buffer);

	if (mschfp) {
		fflush(mschfp);
		fclose(mschfp);
		mschfp = NULL;
	}

	fflush(stdout);
	return err;
}
