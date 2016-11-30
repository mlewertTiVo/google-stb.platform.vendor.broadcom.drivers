/*
 * wl rsdb command module
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
 * $Id: wluc_rsdb.c $
 */
#ifdef WLRSDB
#ifdef WIN32
#include <windows.h>
#endif
#include <wlioctl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include "wlu_common.h"
#include "wlu.h"

static cmd_func_t wl_rsdb_caps;
static cmd_func_t wl_rsdb_bands;
static cmd_func_t wl_rsdb_config;

static int wl_parse_infra_mode_list(char *list_str, uint8* list, int list_num);
static int wl_parse_SIB_param_list(char *list_str, uint32* list, int list_num);

static cmd_t wl_rsdb_cmds[] = {
	{"rsdb_caps", wl_rsdb_caps, WLC_GET_VAR, -1,
	"Get rsdb capabilities for given chip\n"
	"\tUsage : wl rsdb_caps"},
	{"rsdb_bands", wl_rsdb_bands, WLC_GET_VAR, -1,
	"Get band of each wlc/core"
	"\tUsage : wl rsdb_bands"},
	{"rsdb_config", wl_rsdb_config, WLC_GET_VAR, WLC_SET_VAR,
	"Get/Set rsdb config\n"
	"\t wl rsdb_config -n <NonInfraMode> -i <Infra(2G),Infra(5G)> -p <SIB(2G),SIB(5G)> \n"
	"\t-n : <NonInfraMode> Default mode of operation of the device "
	"\t-i : <InfraMode> target mode of operation for Infra 2G/5G connection"
	"\t-p : <allowSIBParallelScan> Allow 2G/5G SIB parallel passive scan"
	"\t SDB op Modes:"
	"\t\tWLC_SDB_MODE_NOSDB_MAIN = 1 :NoSDB mode and Infra is on core 0"
	"\t\tWLC_SDB_MODE_NOSDB_AUX = 2 :NoSDB mode and Infra is on core 1 "
	"\t\tWLC_SDB_MODE_SDB_MAIN = 3 : SDB mode and infra on core-0"
	"\t\tWLC_SDB_MODE_SDB_AUX = 4 : SDB mode and infra on core-1"
	"\t\tWLC_SDB_MODE_SDB_AUTO = 5 : SDB mode and auto infra"
	"Ex : rsdb_config -n 5 -i 5,1 -p 1,1"
	},
	{ NULL, NULL, 0, 0, NULL },
};
static char *buf;

/* module initialization */
void
wluc_rsdb_module_init(void)
{
	(void)g_swap;

	/* get the global buf */
	buf = wl_get_buf();

	/* register scan commands */
	wl_module_cmds_register(wl_rsdb_cmds);
}

static const struct {
	uint8 state;
	char name[28];
} sdb_op_modes[] = {
	{WLC_SDB_MODE_NOSDB_MAIN, "NOSDB_MAIN"},
	{WLC_SDB_MODE_NOSDB_AUX, "NOSDB_AUX"},
	{WLC_SDB_MODE_SDB_MAIN, "SDB_MAIN"},
	{WLC_SDB_MODE_SDB_AUX, "SDB_AUX"},
	{WLC_SDB_MODE_SDB_AUTO, "SDB_AUTO"}
};

static const char *
wlc_sdb_op_mode_name(uint8 state)
{
	uint i;
	for (i = 0; i < ARRAYSIZE(sdb_op_modes); i++) {
		if (sdb_op_modes[i].state == state)
			return sdb_op_modes[i].name;
	}
	return "UNKNOWN";
}
static const struct {
	uint8 state;
	char name[28];
} rsdb_band_names[] = {
	{WLC_BAND_AUTO, "AUTO"},
	{WLC_BAND_2G, "2G"},
	{WLC_BAND_5G, "5G"},
	{WLC_BAND_ALL, "ALL"},
	{WLC_BAND_INVALID, "INVALID"}
};
static const char *
wlc_rsdb_band_name(uint8 state)
{
	uint i;
	for (i = 0; i < ARRAYSIZE(rsdb_band_names); i++) {
		if (rsdb_band_names[i].state == state)
			return rsdb_band_names[i].name;
	}
	return "UNKNOWN";
}
static int
wl_rsdb_caps(void *wl, cmd_t *cmd, char **argv)
{
		int err, i;
		rsdb_caps_response_t *rsdb_caps_ptr = NULL;

		argv++;
		printf("in rsdb caps\n");
		if (*argv == NULL) {
			/* get */
			err = wlu_var_getbuf_sm(wl, cmd->name, NULL, 0, (void *)&rsdb_caps_ptr);
			if (err < 0) {
				printf("%s(): wlu_var_getbuf failed %d \r\n", __FUNCTION__, err);
			} else {
				if (rsdb_caps_ptr->ver != WL_RSDB_CAPS_VER) {
					printf("Bad version : %d\n", rsdb_caps_ptr->ver);
					return BCME_VERSION;
				}
				if (rsdb_caps_ptr->len >= WL_RSDB_CAPS_FIXED_LEN) {
					printf("**RSDB CAPABILITIES**\n");
					printf("RSDB: %d\n", rsdb_caps_ptr->rsdb);
					printf("Num of cores: %d\n", rsdb_caps_ptr->num_of_cores);
					printf("Flags: ");
					if (rsdb_caps_ptr->flags & SYNCHRONOUS_OPERATION_TRUE)
						printf("\tSynchronous Operation supported\n");
					printf("\n");
					/* Add in more capabilites here */
					for (i = 0; i < rsdb_caps_ptr->num_of_cores; i++) {
						printf("Num of chains in core-%d: %d\n", i,
						rsdb_caps_ptr->num_chains[i]);
					}
				}
			}
		} else {
			/* set not allowed */
			return USAGE_ERROR;
		}
		return err;
}

static int
wl_rsdb_bands(void *wl, cmd_t *cmd, char **argv)
{
	int err;
	uint8 i;
	rsdb_bands_t *ptr;

	memset(&ptr, 0, sizeof(*ptr));
	argv++;

	if (*argv == NULL) {
		/* get */
		err = wlu_var_getbuf_sm(wl, cmd->name, NULL, 0, (void *)&ptr);
		if (err < 0) {
			printf("%s(): wlu_var_getbuf_sm failed %d \r\n", __FUNCTION__, err);
		} else {
			/* Check return value of version */
			if (ptr->ver != WL_RSDB_BANDS_VER) {
				printf("Bad version : %d\n", ptr->ver);
				return BCME_VERSION;
			}
			if (ptr->len >= WL_RSDB_BANDS_FIXED_LEN) {
				printf("Num of cores:%d\n", ptr->num_cores);
				for (i = 0; i < ptr->num_cores; i++) {
					printf("WLC[%d]: %s\n", i,
						wlc_rsdb_band_name(ptr->band[i]));
				}
			}
		}
	} else {
		/* set not allowed */
		return USAGE_ERROR;
	}
	return err;
}
static int
wl_rsdb_config(void *wl, cmd_t *cmd, char **argv)
{
	rsdb_config_t rsdb_config;
	int err, i;
	char opt, *p, *endptr = NULL;

	memset(&rsdb_config, 0, sizeof(rsdb_config));

	if (!*++argv) {
		if ((err = wlu_iovar_get(wl, cmd->name, &rsdb_config,
			sizeof(rsdb_config))) < 0) {
			return err;
		}
		if (rsdb_config.ver != WL_RSDB_CONFIG_VER) {
			err = BCME_VERSION;
			goto exit;
		}
		if (rsdb_config.len >= WL_RSDB_CONFIG_LEN) {

			printf("NonInfraMode:%s\n",
				wlc_sdb_op_mode_name(rsdb_config.non_infra_mode));

			for (i = 0; i < MAX_BANDS; i++) {
				printf("InfraMode(%s):%s\n", (i == 0) ? "2G":"5G",
					wlc_sdb_op_mode_name(rsdb_config.infra_mode[i]));
			}
			for (i = 0; i < MAX_BANDS; i++) {
				printf("SIB Scan(%s):", (i == 0) ? "2G":"5G");
				printf("%s\n",
					(rsdb_config.flags[i] & ALLOW_SIB_PARALLEL_SCAN) ?
						"ENABLED":"DISABLED");
			}
			printf("Current_mode: %s\n",
				wlc_sdb_op_mode_name(rsdb_config.current_mode));
		}
	} else {
			/* Get the existing config and change only the fields passed */
			if ((err = wlu_iovar_get(wl, cmd->name, &rsdb_config,
				sizeof(rsdb_config))) < 0) {
				return err;
			}
			if (rsdb_config.ver != WL_RSDB_CONFIG_VER) {
				err = BCME_VERSION;
				goto exit;
			}
			while ((p = *argv) != NULL) {
				argv++;
				opt = '\0';

				if (!strncmp(p, "-", 1)) {
					if ((strlen(p) > 2) || (*argv == NULL)) {
						fprintf(stderr, "Invalid option %s \n", p);
						err = BCME_USAGE_ERROR;
						goto exit;
					}
				} else {
					fprintf(stderr, "Invalid option %s \n", p);
					err = BCME_USAGE_ERROR;
					goto exit;
				}
				opt = p[1];

			switch (opt) {
				case 'n':
					rsdb_config.non_infra_mode = (uint8)strtol(*argv,
						&endptr, 0);
					argv++;
					break;
				case 'i':
					wl_parse_infra_mode_list(*argv, rsdb_config.infra_mode,
						MAX_BANDS);
					argv++;
					break;
				case 'p':
					wl_parse_SIB_param_list(*argv, rsdb_config.flags,
						MAX_BANDS);
					argv++;
				break;
				default:
					fprintf(stderr, "Invalid option %s \n", p);
					err = BCME_USAGE_ERROR;
					goto exit;
			}
		}
		rsdb_config.len = WL_RSDB_CONFIG_LEN;
		if ((err = wlu_var_setbuf(wl, cmd->name, &rsdb_config,
			sizeof(rsdb_config))) < 0) {
			return err;
		}
	}

exit:
	return err;
}

static int
wl_parse_infra_mode_list(char *list_str, uint8* list, int list_num)
{
	int num;
	int val;
	char* str;
	char* endptr = NULL;

	if (list_str == NULL)
		return -1;

	str = list_str;
	num = 0;
	while (*str != '\0') {
		val = (int)strtol(str, &endptr, 0);
		if (endptr == str) {
			fprintf(stderr,
				"could not parse list number starting at"
				" substring \"%s\" in list:\n%s\n",
				str, list_str);
			return -1;
		}
		str = endptr + strspn(endptr, " ,");

		if (num == list_num) {
			fprintf(stderr, "too many members (more than %d) in list:\n%s\n",
				list_num, list_str);
			return -1;
		}

		list[num++] = (uint16)val;
	}

	return num;
}

static int
wl_parse_SIB_param_list(char *list_str, uint32* list, int list_num)
{
	int num;
	int val;
	char* str;
	char* endptr = NULL;

	if (list_str == NULL)
		return -1;

	str = list_str;
	num = 0;
	while (*str != '\0') {
		val = (int)strtol(str, &endptr, 0);
		if (endptr == str) {
			fprintf(stderr,
				"could not parse list number starting at"
				" substring \"%s\" in list:\n%s\n",
				str, list_str);
			return -1;
		}
		str = endptr + strspn(endptr, " ,");

		if (num == list_num) {
			fprintf(stderr, "too many members (more than %d) in list:\n%s\n",
				list_num, list_str);
			return -1;
		}

		list[num++] = (uint16)val;
	}
	return num;
}
#endif /* WLRSDB */
