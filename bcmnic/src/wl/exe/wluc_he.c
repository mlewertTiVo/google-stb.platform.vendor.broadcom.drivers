/*
 * wl he command module
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
 * $Id: wluc_he.c 627609 2016-03-25 21:00:45Z $
 */

#ifdef WIN32
/* Because IL_BIGENDIAN was removed there are few warnings that need
 * to be fixed. Windows was not compiled earlier with IL_BIGENDIAN.
 * Hence these warnings were not seen earlier.
 * For now ignore the following warnings
 */
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4761)
#include <windows.h>
#endif

#include <wlioctl.h>

#include <bcmutils.h>
#include <bcmendian.h>

#include <proto/802.11ah.h>

#include <miniopt.h>

#include "wlu_common.h"
#include "wlu.h"

#ifndef bzero
#define bzero(mem, len)	memset(mem, 0, len)
#endif


#define HE_CMD_HELP_STR \
	"HE (802.11ax) protocol control commands\n\n" \
	"\tUsage: wl he [command] [cmd options]\n\n" \
	"Available commands and command options:\n" \
	"\twl he enab [0|1] - query or enable/disable HE feature\n" \
	"\twl he features [<features mask>] - query or enable/disable HE sub-features\n" \
	"\twl he twt setup [<flow flags>] [<twt type] [<options>] <setup cmd> <flow id> - "\
	"setup target wake time (TWT)\n" \
	"\t\t<flow flags>:\n" \
	"\t\t\t-b - Broadcast TWT\n" \
	"\t\t\t-i - Implicit TWT\n" \
	"\t\t\t-u - Unannounced\n" \
	"\t\t\t-b - Trigger\n" \
	"\t\t<twt type>:\n" \
	"\t\t\t-y tsf - Absolute TSF\n" \
	"\t\t\t-y offset - TSF offset\n" \
	"\t\t<options>:\n" \
	"\t\t\t-a <peer MAC address>\n" \
	"\t\t\t-k <dialog token>\n" \
	"\t\t\t-w <target wake time>\n" \
	"\t\t\t-d <wake duration>\n" \
	"\t\t\t-p <wake interval>\n" \
	"\t\t<setup cmd>:\n" \
	"\t\t\trequest\n" \
	"\t\t\tsuggest\n" \
	"\t\t\tdemand\n" \
	"\twl he twt teardown [<flow flags>] [<options>] <flow id> - teardown flow\n" \
	"\t\t<flow flags>:\n" \
	"\t\t\t-b - Broadcast TWT\n" \
	"\t\t<options>:\n" \
	"\t\t\t-a <peer MAC address>\n" \
	"\twl he twt info [<flow flags>] [<options>] <flow id> - request information\n" \
	"\t\t<flow flags>:\n" \
	"\t\t\t-r - response request\n" \
	"\t\t<options>:\n" \
	"\t\t\t-a <peer MAC address>\n" \
	"\t\t\t-w <target wake time>\n"

static cmd_func_t wl_he_cmd;

/* wl he top level command list */
static cmd_t wl_he_cmds[] = {
	{ "he", wl_he_cmd, WLC_GET_VAR, WLC_SET_VAR, HE_CMD_HELP_STR },
	{ NULL, NULL, 0, 0, NULL }
};

static char *buf;

/* module initialization */
void
wluc_he_module_init(void)
{
	/* get the global buf */
	buf = wl_get_buf();

	/* register HE commands */
	wl_module_cmds_register(wl_he_cmds);
}

/* HE sub cmd */
typedef struct sub_cmd sub_cmd_t;
typedef int (sub_cmd_func_t)(void *wl, const cmd_t *cmd, const sub_cmd_t *sub, char **argv);
struct sub_cmd {
	char *name;
	uint16 id;		/* id for the dongle f/w switch/case  */
	uint16 type;		/* base type of argument IOVT_XXXX */
	sub_cmd_func_t *hdlr;	/* cmd handler  */
};

/*  HE ioctl sub cmd handler functions  */
static sub_cmd_func_t wl_he_cmd_uint;
static sub_cmd_func_t wl_he_cmd_twt;
static sub_cmd_func_t wl_he_cmd_twt_setup;
static sub_cmd_func_t wl_he_cmd_twt_teardown;
static sub_cmd_func_t wl_he_cmd_twt_info;

/* wl he sub cmd list */
static const sub_cmd_t he_cmd_list[] = {
	/* wl he enab [0|1] */
	{"enab", WL_HE_CMD_ENAB, IOVT_UINT8, wl_he_cmd_uint},
	/* wl he features [<features>] */
	{"features", WL_HE_CMD_FEATURES, IOVT_UINT32, wl_he_cmd_uint},
	/* wl he twt ... */
	{"twt", 0, 0, wl_he_cmd_twt},
	{NULL, 0, 0, NULL}
};

/* wl he twt sub cmd list */
static const sub_cmd_t twt_cmd_list[] = {
	/* wl he twt setup ... */
	{"setup", WL_HE_CMD_TWT_SETUP, IOVT_BUFFER, wl_he_cmd_twt_setup},
	/* wl he twt teardown ... */
	{"teardown", WL_HE_CMD_TWT_TEARDOWN, IOVT_BUFFER, wl_he_cmd_twt_teardown},
	/* wl he twt info ... */
	{"info", WL_HE_CMD_TWT_INFO, IOVT_BUFFER, wl_he_cmd_twt_info},
	{NULL, 0, 0, NULL}
};

#ifdef NEED_STRTOULL
static unsigned long long int
strtoull(const char *nptr, char **endptr, int base)
{
	unsigned long long int result;
	unsigned char value;
	bool minus;

	minus = FALSE;

	while (bcm_isspace(*nptr)) {
		nptr++;
	}

	if (nptr[0] == '+') {
		nptr++;
	}
	else if (nptr[0] == '-') {
		minus = TRUE;
		nptr++;
	}

	if (base == 0) {
		if (nptr[0] == '0') {
			if ((nptr[1] == 'x') || (nptr[1] == 'X')) {
				base = 16;
				nptr = &nptr[2];
			} else {
				base = 8;
				nptr = &nptr[1];
			}
		} else {
			base = 10;
		}
	} else if (base == 16 &&
	           (nptr[0] == '0') && ((nptr[1] == 'x') || (nptr[1] == 'X'))) {
		nptr = &nptr[2];
	}

	result = 0;

	while (bcm_isxdigit(*nptr) &&
	       (value = bcm_isdigit(*nptr) ? *nptr - '0' : bcm_toupper(*nptr) - 'A' + 10) <
	       (unsigned char)base) {
		/* TODO: The strtoul() function should only convert the initial part
		 * of the string in nptr to an unsigned long int value according to
		 * the given base...so strtoull() should follow the same rule...
		 */
		result = result * base + value;
		nptr++;
	}

	if (minus) {
		result = result * -1;
	}

	if (endptr) {
		*endptr = DISCARD_QUAL(nptr, char);
	}

	return result;
}
#endif /* NEED_STRTOULL */

/* wl he command */
static int
wl_he_cmd(void *wl, cmd_t *cmd, char **argv)
{
	const sub_cmd_t *sub = he_cmd_list;
	char *he_query[2] = {"enab", NULL};
	char *he_en[3] = {"enab", "1", NULL};
	char *he_dis[3] = {"enab", "0", NULL};
	int ret = BCME_USAGE_ERROR;

	/* skip to cmd name after "he" */
	argv++;

	if (!*argv) {
		/* query he "enab" state */
		argv = he_query;
	}
	else if (*argv[0] == '1') {
		argv = he_en;
	}
	else if (*argv[0] == '0') {
		argv = he_dis;
	}
	else if (!strcmp(*argv, "-h") || !strcmp(*argv, "help"))  {
		/* help , or -h* */
		return BCME_USAGE_ERROR;
	}

	while (sub->name != NULL) {
		if (strcmp(sub->name, *argv) == 0)  {
			/* dispacth subcmd to the handler */
			if (sub->hdlr != NULL) {
				ret = sub->hdlr(wl, cmd, sub, ++argv);
			}
			return ret;
		}
		sub ++;
	}

	return BCME_IOCTL_ERROR;
}

typedef struct {
	uint16 id;
	uint16 len;
	uint32 val;
} he_xtlv_v32;


static uint
wl_he_iovt2len(uint iovt)
{
	switch (iovt) {
	case IOVT_BOOL:
	case IOVT_INT8:
	case IOVT_UINT8:
		return sizeof(uint8);
	case IOVT_INT16:
	case IOVT_UINT16:
		return sizeof(uint16);
	case IOVT_INT32:
	case IOVT_UINT32:
		return sizeof(uint32);
	default:
		/* ASSERT(0); */
		return 0;
	}
}

static bool
wl_he_get_uint_cb(void *ctx, uint16 *id, uint16 *len)
{
	he_xtlv_v32 *v32 = ctx;

	*id = v32->id;
	*len = v32->len;

	return FALSE;
}

static void
wl_he_pack_uint_cb(void *ctx, uint16 id, uint16 len, uint8 *buf)
{
	he_xtlv_v32 *v32 = ctx;

	BCM_REFERENCE(id);
	BCM_REFERENCE(len);

	v32->val = htod32(v32->val);

	switch (v32->len) {
	case sizeof(uint8):
		*buf = (uint8)v32->val;
		break;
	case sizeof(uint16):
		store16_ua(buf, (uint16)v32->val);
		break;
	case sizeof(uint32):
		store16_ua(buf, v32->val);
		break;
	default:
		/* ASSERT(0); */
		break;
	}
}

/*  ******** generic uint8/uint16/uint32   ******** */
static int
wl_he_cmd_uint(void *wl, const cmd_t *cmd, const sub_cmd_t *sub, char **argv)
{
	int res = BCME_OK;

	/* get */
	if (*argv == NULL) {
		bcm_xtlv_t mybuf;
		uint8 *resp;

		mybuf.id = sub->id;
		mybuf.len = 0;

		/*  send getbuf iovar */
		res = wlu_var_getbuf_sm(wl, cmd->name, &mybuf, sizeof(mybuf),
			(void **)&resp);

		/*  check the response buff  */
		if (res == BCME_OK && resp != NULL) {
			uint len = wl_he_iovt2len(sub->type);
			uint32 v32;

			switch (len) {
			case sizeof(uint8):
				v32 = *resp;
				break;
			case sizeof(uint16):
				v32 = load16_ua(resp);
				break;
			case sizeof(uint32):
				v32 = load32_ua(resp);
				break;
			default:
				v32 = ~0;
				break;
			}
			v32 = dtoh32(v32);
			printf("%u\n", v32);
		}
	}
	/* set */
	else {
		uint8 mybuf[32];
		int mybuf_len = sizeof(mybuf);
		he_xtlv_v32 v32;

		v32.id = sub->id;
		v32.len = wl_he_iovt2len(sub->type);
		v32.val = atoi(*argv);

		res = bcm_pack_xtlv_buf((void *)&v32, mybuf, sizeof(mybuf),
			BCM_XTLV_OPTION_ALIGN32, wl_he_get_uint_cb, wl_he_pack_uint_cb,
			&mybuf_len);
		if (res != BCME_OK) {
			goto fail;
		}

		res = wlu_var_setbuf(wl, cmd->name, mybuf, mybuf_len);
	}

	return res;

fail:
	fprintf(stderr, "error:%d\n", res);
	return res;
}

/* wl he twt top level commend */
static int
wl_he_cmd_twt(void *wl, const cmd_t *cmd, const sub_cmd_t *sub, char **argv)
{
	const sub_cmd_t *sub2 = twt_cmd_list;
	int res = BCME_OK;

	BCM_REFERENCE(sub);

	if (*argv == NULL) {
		return BCME_USAGE_ERROR;
	}

	while (sub2->name != NULL) {
		if (strcmp(sub2->name, *argv) == 0)  {
			/* dispacth subcmd to the handler */
			if (sub2->hdlr != NULL) {
				res = sub2->hdlr(wl, cmd, sub2, ++argv);
			}
			return res;
		}
		sub2 ++;
	}

	return BCME_USAGE_ERROR;
}

/* setup command name to value conversion */
static struct {
	const char *name;
	uint8 val;
} setup_cmd_val[] = {
	{"request", TWT_SETUP_CMD_REQUEST_TWT},
	{"suggest", TWT_SETUP_CMD_SUGGEST_TWT},
	{"demand", TWT_SETUP_CMD_DEMAND_TWT}
};

#define WL_HE_TWT_CMD_INVAL	255

static uint8
wl_he_twt_cmd2val(const char *name)
{
	uint i;

	for (i = 0; i < ARRAYSIZE(setup_cmd_val); i ++) {
		if (strcmp(name, setup_cmd_val[i].name) == 0) {
			return setup_cmd_val[i].val;
		}
	}

	return WL_HE_TWT_CMD_INVAL;
}

/* target wake time (twt) type to value conversion */
static struct {
	const char *name;
	uint8 val;
} twt_type_val[] = {
	{"tsf", WL_TWT_TIME_TYPE_BSS},
	{"offset", WL_TWT_TIME_TYPE_OFFSET}
};

#define WL_HE_TWT_TYPE_INVAL	255

static uint8
wl_he_twt_type2val(const char *name)
{
	uint i;

	for (i = 0; i < ARRAYSIZE(twt_type_val); i ++) {
		if (strcmp(name, twt_type_val[i].name) == 0) {
			return twt_type_val[i].val;
		}
	}

	return WL_HE_TWT_TYPE_INVAL;
}

/* wl he twt setup command */
static int
wl_he_cmd_twt_setup(void *wl, const cmd_t *cmd, const sub_cmd_t *sub, char **argv)
{
	int res = BCME_OK;
	uint8 mybuf[64];
	uint8 *rem = mybuf;
	uint16 rem_len = sizeof(mybuf);
	wl_twt_setup_t val;
	miniopt_t opt;
	int opt_err;
	int argc;
	bool got_mandatory = FALSE;

	BCM_REFERENCE(wl);

	if (*argv == NULL) {
		return BCME_USAGE_ERROR;
	}

	/* arg count */
	for (argc = 0; argv[argc]; argc++)
		;

	bzero(&val, sizeof(val));

	val.version = WL_TWT_SETUP_VER;
	val.length = sizeof(val.version) + sizeof(val.length);

	miniopt_init(&opt, __FUNCTION__, "biut", FALSE);

	while ((opt_err = miniopt(&opt, argv)) != -1) {

		if (opt_err == 1) {
			res = BCME_USAGE_ERROR;
			goto fail;
		}

		/* flags and options */
		if (!opt.positional) {

			/* flags */

			/* -b (broadcast) */
			if (opt.opt == 'b') {
				val.desc.flow_flags |= WL_TWT_FLOW_FLAG_BROADCAST;
			}
			/* -i (implicit) */
			else if (opt.opt == 'i') {
				val.desc.flow_flags |= WL_TWT_FLOW_FLAG_IMPLICIT;
			}
			/* -u (unannounced) */
			else if (opt.opt == 'u') {
				val.desc.flow_flags |= WL_TWT_FLOW_FLAG_UNANNOUNCED;
			}
			/* -t (trigger) */
			else if (opt.opt == 't') {
				val.desc.flow_flags |= WL_TWT_FLOW_FLAG_TRIGGER;
			}

			/* options */

			/* -a peer_address */
			else if (opt.opt == 'a') {
				if (!wl_ether_atoe(opt.valstr, &val.peer)) {
					fprintf(stderr, "Malformed TWT peer address '%s'\n",
					        opt.valstr);
					res = BCME_BADARG;
					goto fail;
				}
			}
			/* -k dialog_token */
			else if (opt.opt == 'k') {
				val.dialog = (uint8)strtoul(opt.valstr, NULL, 0);
			}
			/* -y twt type */
			else if (opt.opt == 'y') {
				if ((val.desc.wake_type =
				     wl_he_twt_type2val(opt.valstr)) == WL_HE_TWT_TYPE_INVAL) {
					fprintf(stderr, "Unrecognized TWT type '%s'\n",
					        opt.valstr);
					res = BCME_BADARG;
					goto fail;
				}
			}
			/* -w target_wake_time (twt) */
			else if (opt.opt == 'w') {
				uint64 twt = strtoull(opt.valstr, NULL, 0);
				val.desc.wake_time_h = htod32((uint32)(twt >> 32));
				val.desc.wake_time_l = htod32((uint32)twt);
			}
			/* -d duration */
			else if (opt.opt == 'd') {
				val.desc.wake_dur = htod32(opt.uval);
			}
			/* -p interval */
			else if (opt.opt == 'p') {
				val.desc.wake_int = htod32(opt.uval);
			}
			else {
				fprintf(stderr, "Unrecognized option '%s'\n", *argv);
				res = BCME_BADARG;
				goto fail;
			}
		}

		/* positional/mandatory */
		else {
			if (argc < 2) {
				res = BCME_USAGE_ERROR;
				goto fail;
			}

			/* setup_cmd */
			if ((val.desc.setup_cmd =
			     wl_he_twt_cmd2val(*argv)) == WL_HE_TWT_CMD_INVAL) {
				fprintf(stderr, "Unrecognized TWT Setup command '%s'\n", *argv);
				res = BCME_BADARG;
				goto fail;
			}
			argv++;

			/* flow_id */
			val.desc.flow_id = (uint8)strtoul(*argv, NULL, 0);
			argv++;

			got_mandatory = TRUE;

			break;
		}

		argv += opt.consumed;
		argc -= opt.consumed;
	}

	if (!got_mandatory) {
		res = BCME_USAGE_ERROR;
		goto fail;
	}

	res = bcm_pack_xtlv_entry(&rem, &rem_len, sub->id,
		sizeof(val), (uint8 *)&val, BCM_XTLV_OPTION_ALIGN32);
	if (res != BCME_OK) {
		goto fail;
	}

	return wlu_var_setbuf(wl, cmd->name, mybuf, sizeof(mybuf) - rem_len);

fail:
	fprintf(stderr, "error:%d\n", res);
	return res;
}

/* wl he twt teardown command */
static int
wl_he_cmd_twt_teardown(void *wl, const cmd_t *cmd, const sub_cmd_t *sub, char **argv)
{
	int res = BCME_OK;
	uint8 mybuf[64];
	uint8 *rem = mybuf;
	uint16 rem_len = sizeof(mybuf);
	wl_twt_teardown_t val;
	miniopt_t opt;
	int opt_err;
	int argc;
	bool got_mandatory = FALSE;

	BCM_REFERENCE(wl);

	if (*argv == NULL) {
		return BCME_USAGE_ERROR;
	}

	/* arg count */
	for (argc = 0; argv[argc]; argc++)
		;

	bzero(&val, sizeof(val));

	val.version = WL_TWT_TEARDOWN_VER;
	val.length = sizeof(val.version) + sizeof(val.length);

	miniopt_init(&opt, __FUNCTION__, "b", FALSE);

	while ((opt_err = miniopt(&opt, argv)) != -1) {

		if (opt_err == 1) {
			res = BCME_USAGE_ERROR;
			goto fail;
		}

		/* flags and options */
		if (!opt.positional) {

			/* flags */

			/* -b (broadcast) */
			if (opt.opt == 'b') {
				val.flow_flags |= WL_TWT_FLOW_FLAG_BROADCAST;
			}

			/* options */

			/* -a peer_address */
			else if (opt.opt == 'a') {
				if (!wl_ether_atoe(opt.valstr, &val.peer)) {
					res = BCME_BADARG;
					goto fail;
				}
			}
		}

		/* positionals */
		else {
			if (argc < 1) {
				res = BCME_USAGE_ERROR;
				goto fail;
			}

			/* flow_id */
			val.flow_id = (uint8)strtoul(*argv, NULL, 0);
			argv++;

			got_mandatory = TRUE;

			break;
		}

		argv += opt.consumed;
	}

	if (!got_mandatory) {
		res = BCME_USAGE_ERROR;
		goto fail;
	}

	res = bcm_pack_xtlv_entry(&rem, &rem_len, sub->id,
		sizeof(val), (uint8 *)&val, BCM_XTLV_OPTION_ALIGN32);
	if (res != BCME_OK) {
		goto fail;
	}

	return wlu_var_setbuf(wl, cmd->name, mybuf, sizeof(mybuf) - rem_len);

fail:
	fprintf(stderr, "error:%d\n", res);
	return res;
}

/* wl he twt info command */
static int
wl_he_cmd_twt_info(void *wl, const cmd_t *cmd, const sub_cmd_t *sub, char **argv)
{
	int res = BCME_OK;
	uint8 mybuf[64];
	uint8 *rem = mybuf;
	uint16 rem_len = sizeof(mybuf);
	wl_twt_info_t val;
	miniopt_t opt;
	int opt_err;
	int argc;
	bool got_mandatory = FALSE;

	BCM_REFERENCE(wl);

	if (*argv == NULL) {
		return BCME_USAGE_ERROR;
	}

	/* arg count */
	for (argc = 0; argv[argc]; argc++)
		;

	bzero(&val, sizeof(val));

	val.version = WL_TWT_INFO_VER;
	val.length = sizeof(val.version) + sizeof(val.length);

	miniopt_init(&opt, __FUNCTION__, "r", FALSE);

	while ((opt_err = miniopt(&opt, argv)) != -1) {

		if (opt_err == 1) {
			res = BCME_USAGE_ERROR;
			goto fail;
		}

		/* flags and options */
		if (!opt.positional) {

			/* flags */

			/* -r (response request) */
			if (opt.opt == 'r') {
				val.desc.flow_flags |= WL_TWT_INFO_FLAG_RESP_REQ;
			}

			/* options */

			/* -a peer_address */
			else if (opt.opt == 'a') {
				if (!wl_ether_atoe(opt.valstr, &val.peer)) {
					res = BCME_BADARG;
					goto fail;
				}
			}
			/* -w target_wake_time (twt) */
			else if (opt.opt == 'w') {
				uint64 twt = strtoull(opt.valstr, NULL, 0);
				val.desc.next_twt_h = htod32((uint32)(twt >> 32));
				val.desc.next_twt_l = htod32((uint32)twt);
			}
		}

		/* positionals */
		else {
			if (argc < 1) {
				res = BCME_USAGE_ERROR;
				goto fail;
			}

			/* flow_id */
			val.desc.flow_id = (uint8)strtoul(*argv, NULL, 0);
			argv++;

			got_mandatory = TRUE;

			break;
		}

		argv += opt.consumed;
	}

	if (!got_mandatory) {
		res = BCME_USAGE_ERROR;
		goto fail;
	}

	res = bcm_pack_xtlv_entry(&rem, &rem_len, sub->id,
		sizeof(val), (uint8 *)&val, BCM_XTLV_OPTION_ALIGN32);
	if (res != BCME_OK) {
		goto fail;
	}

	return wlu_var_setbuf(wl, cmd->name, mybuf, sizeof(mybuf) - rem_len);

fail:
	fprintf(stderr, "error:%d\n", res);
	return res;
}
