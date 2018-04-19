/*
 * BSD port of wl command line utility
 *
 * Copyright (C) 2018, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlu_bsd.c 499182 2014-08-27 22:48:09Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

typedef u_int64_t u64;
typedef u_int32_t u32;
typedef u_int16_t u16;
typedef u_int8_t u8;

#include <typedefs.h>
#include <wlioctl.h>
#include <bcmutils.h>
#include "wlu.h"

#if defined(__FreeBSD__)
#include <dhdioctl.h>
#endif

#define DEV_TYPE_LEN 3 /* Device type length */
#define INTERACTIVE_NUM_ARGS			15
#define INTERACTIVE_MAX_INPUT_LENGTH	512

static cmd_t *wl_find_cmd(char* name);
static int do_interactive(struct ifreq *ifr);
static int wl_do_cmd(struct ifreq *ifr, char **argv);

static void
syserr(const char *s)
{
	perror(s);
	exit(errno);
}

static int
wl_ioctl(void *wl, int cmd, void *buf, int len, bool set)
{
	struct ifreq *ifr = (struct ifreq *) wl;
	wl_ioctl_t ioc;
	int ret = 0;
	int s;

	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		syserr("socket");

	/* do it */
	ioc.cmd = cmd;
	ioc.buf = buf;
	ioc.len = len;
	ioc.set = set;
	ifr->ifr_data = (caddr_t) &ioc;
	if ((ret = ioctl(s, SIOCDEVPRIVATE, ifr)) < 0) {
		if (cmd != WLC_GET_MAGIC)
			perror(ifr->ifr_name);
	}

	/* cleanup */
	close(s);
	return ret;
}

int
wl_get(void *wl, int cmd, void *buf, int len)
{
	return wl_ioctl(wl, cmd, buf, len, FALSE);
}

int
wl_set(void *wl, int cmd, void *buf, int len)
{
	return wl_ioctl(wl, cmd, buf, len, TRUE);
}

void
wl_find(struct ifreq *ifr)
{
	/* Looks for wl0 f no arguments are given */
	strcpy((char *)ifr->ifr_name, "wl0");
}

int
main(int argc, char **argv)
{
	struct ifreq ifr;
	cmd_t *cmd = NULL;
	int help = FALSE;
	int err = 0;
	char *ifname = NULL;
	int status = 0;
	memset(&ifr, 0, sizeof(ifr));

	if (argc < 2) {
		wl_usage(stdout, NULL);
		exit(1);
	}

	wlu_init();
	for (++argv; *argv;) {
		if ((status = wl_option(&argv, &ifname, &help)) == CMD_OPT) {
			if (help)
				break;
			if (ifname)
				strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
			continue;
		} else if (status == CMD_ERR)
			break;
		/* use default interface */
		if (!*ifr.ifr_name)
			wl_find(&ifr);
		if (wl_check((void *)&ifr)) {
			fprintf(stderr, "wl driver adapter not found\n");
			exit(1);
		}

		if (strcmp (*argv, "--interactive") == 0) {
			err = do_interactive(&ifr);
			return err;
		}

		/* search for command */
		cmd = wl_find_cmd(*argv);

		/* defaults to using the set_var and get_var commands */
		if (!cmd)
			cmd = &wl_varcmd;

		/* do command */
		err = (*cmd->func)((void *) &ifr, cmd, argv);

		break;
	}

	if (help && *argv) {
		cmd = wl_find_cmd(*argv);
		if (cmd)
			wl_cmd_usage(stdout, cmd);
		else
			printf("%s: Unrecognized command \"%s\", type -h for help\n",
			       wlu_av0, *argv);
	}
	else if (!cmd)
		wl_usage(stdout, NULL);
	else if (err == BCME_USAGE_ERROR)
		wl_cmd_usage(stderr, cmd);
	else if (err == BCME_IOCTL_ERROR)
		wl_printlasterror((void *) &ifr);
	return err;
}

/* Function called for 'local' interactive session and for 'remote' interactive session */
static int
do_interactive(struct ifreq *ifr)
{
	int err = 0;

	while (1) {
		char *fgsret;
		char line[INTERACTIVE_MAX_INPUT_LENGTH];
		fprintf(stdout, "> ");
		fgsret = fgets(line, sizeof(line), stdin);

		/* end of file */
		if (fgsret == NULL)
			break;
		if (line[0] == '\n')
			continue;

		if (strlen (line) > 0) {
			/* skip past first arg if it's "wl" and parse up arguments */
			char *argv[INTERACTIVE_NUM_ARGS];
			int argc;
			char *token;
			argc = 0;

			while (argc < (INTERACTIVE_NUM_ARGS - 1) &&
			       (token = strtok(argc ? NULL : line, " \t\n")) != NULL) {

				/* Specifically make sure empty arguments (like SSID) are empty */
				if (token[0] == '"' && token[1] == '"') {
				    token[0] = '\0';
				}

				argv[argc++] = token;
			}
			argv[argc] = NULL;
			if (argc == (INTERACTIVE_NUM_ARGS - 1) &&
			    (token = strtok(NULL, " \t")) != NULL) {
				printf("wl:error: too many args; argc must be < %d\n",
				       (INTERACTIVE_NUM_ARGS - 1));
				continue;
			}

			if (strcmp(argv[0], "q") == 0 || strcmp(argv[0], "exit") == 0) {
				break;
			}

			err = wl_do_cmd(ifr, argv);
		} /* end of strlen (line) > 0 */
	} /* while (1) */

	return err;
}

/*
 * find command in argv and execute it
 * Won't handle changing ifname yet, expects that to happen with the --interactive
 * Return an error if unable to find/execute command
 */
static int
wl_do_cmd(struct ifreq *ifr, char **argv)
{
	cmd_t *cmd = NULL;
	int err = 0;
	int help = 0;
	char *ifname = NULL;
	int status = CMD_WL;

	/* skip over 'wl' if it's there */
	if (*argv && strcmp (*argv, "wl") == 0) {
		argv++;
	}

	/* handle help or interface name changes */
	if (*argv && (status = wl_option (&argv, &ifname, &help)) == CMD_OPT) {
		if (ifname) {
			fprintf(stderr,
			        "Interface name change not allowed within --interactive\n");
		}
	}

	/* in case wl_option eats all the args */
	if (!*argv) {
		return err;
	}

	if (status != CMD_ERR) {
		/* search for command */
		cmd = wl_find_cmd(*argv);

		/* defaults to using the set_var and get_var commands */
		if (!cmd) {
			cmd = &wl_varcmd;
		}
		/* do command */
		err = (*cmd->func)((void *)ifr, cmd, argv);
	}
	/* provide for help on a particular command */
	if (help && *argv) {
	  cmd = wl_find_cmd(*argv);
	 if (cmd) {
		wl_cmd_usage(stdout, cmd);
	} else {
			fprintf(stderr, "%s: Unrecognized command \"%s\", type -h for help\n",
			       wlu_av0, *argv);
	       }
	} else if (!cmd)
		wl_usage(stdout, NULL);
	else if (err == BCME_USAGE_ERROR)
		wl_cmd_usage(stderr, cmd);
	else if (err == BCME_IOCTL_ERROR)
		wl_printlasterror((void *)ifr);
	else if (err == BCME_NODEVICE)
		fprintf(stderr, "%s : wl driver adapter not found\n", ifname);

	return err;
}

/* Search the  wl_cmds table for a matching command name.
 * Return the matching command or NULL if no match found.
 */
static cmd_t*
wl_find_cmd(char* name)
{
	return wlu_find_cmd(name);
}
