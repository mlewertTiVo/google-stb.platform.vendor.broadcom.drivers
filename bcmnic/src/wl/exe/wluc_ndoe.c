/*
 * wl ndoe command module
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
 * $Id: wluc_ndoe.c 458728 2014-02-27 18:15:25Z $
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

static cmd_func_t wl_ndstatus, wl_solicitipv6, wl_remoteipv6;
static cmd_func_t wl_nd_hostip_extended;

static cmd_t wl_ndoe_cmds[] = {
	{ "nd_hostip", wl_nd_hostip_extended, WLC_GET_VAR, WLC_SET_VAR,
	"Add/delete a local host Ipv6 address or get list of address\n"
	"Usage: wl nd_hostip <ver|add|del|list> [addr] [-t <uc|ac>]\n"
	"       wl nd_hostip ver              : get iovar version supported\n"
	"       wl nd_hostip add <addr>       : add unicast host ip addr\n"
	"       wl nd_hostip add <addr> -t ac : add anycast host ip addr\n"
	"       wl nd_hostip del <addr>       : delete specified addr\n"
	"       wl nd_hostip del -t uc        : delete all unicast addr\n"
	"       wl nd_hostip del -t ac        : delete all anycast addr\n"
	"       wl nd_hostip del              : delete all addr\n"
	"       wl nd_hostip list             : get list of all addr\n"
	"     * wl nd_hostip <addr>           : add unicast host ip address\n"
	"     * wl nd_hostip                  : display list of address\n"},
	{ "nd_solicitip", wl_solicitipv6, WLC_GET_VAR, WLC_SET_VAR,
	"Add a local host solicit ipv6 address or display them"},
	{ "nd_remoteip", wl_remoteipv6, WLC_GET_VAR, WLC_SET_VAR,
	"Add a local remote ipv6 address or display them"},
	{ "nd_status", wl_ndstatus, WLC_GET_VAR, -1,
	"Displays Neighbor Discovery Status"},
	{ "nd_hostip_clear", wl_var_void, -1, WLC_SET_VAR,
	"Clear all host-ip addresses"},
	{ "nd_macaddr", wl_iov_mac, WLC_GET_VAR, WLC_SET_VAR,
	"Get/set the MAC address for offload" },
	{ "nd_status_clear", wl_var_void, -1, WLC_SET_VAR,
	"Clear neighbor discovery status"},
	{ "nd_unsolicited_na_filter", wl_varint, WLC_GET_VAR, WLC_SET_VAR,
	"Enable/disable Unsolicited Neighbor Advertisement Filtering"},
	{ NULL, NULL, 0, 0, NULL }
};

static char *buf;

/* module initialization */
void
wluc_ndoe_module_init(void)
{
	(void)g_swap;

	/* get the global buf */
	buf = wl_get_buf();

	/* register ndoe commands */
	wl_module_cmds_register(wl_ndoe_cmds);
}

static int
wl_print_ipv6_addr(struct ipv6_addr *addr)
{
	if (addr) {
		uint i;
		uint16 *ip_addr = (uint16*)addr;

		/* IPv6 is represented as 8 groups of 4 hex digit separated by : */
		for (i = 0; i < (IPV6_ADDR_LEN/sizeof(uint16)); i++) {
			printf("%x", ntoh16(ip_addr[i]));
			if (i < 7)
				printf(":");
		}
		return 0;
	} else {
		printf("null");
		return -1;
	}
}

static int
wl_nd_hostip_extended(void *wl, cmd_t *cmd, char **argv)
{
	int ret;
	wl_nd_hostip_t hostip_op;
	char **saved_argv = argv;

	if (!*++argv) {
		/* Get for nd_hostip iovar old syntax */
		return wl_hostipv6(wl, cmd, saved_argv);
	} else if (!stricmp(*argv, "ver")) {
		wl_nd_hostip_t *nd_hostip_ver = NULL;

		/* Get iovar version */
		hostip_op.version = htod16(WL_ND_HOSTIP_IOV_VER);
		hostip_op.op_type = htod16(WL_ND_HOSTIP_OP_VER);
		hostip_op.length = htod32(WL_ND_HOSTIP_FIXED_LEN + sizeof(uint16));
		hostip_op.u.version = 0;

		ret = wlu_var_getbuf(wl, cmd->name, &hostip_op,
				WL_ND_HOSTIP_FIXED_LEN + sizeof(uint16), (void **)&nd_hostip_ver);

		if (ret < 0) {
			return BCME_UNSUPPORTED;
		}

		nd_hostip_ver->version = dtoh16(nd_hostip_ver->version);
		nd_hostip_ver->op_type = dtoh16(nd_hostip_ver->op_type);
		nd_hostip_ver->length = dtoh32(nd_hostip_ver->length);
		nd_hostip_ver->u.version = dtoh16(nd_hostip_ver->u.version);

		if ((nd_hostip_ver->version == WL_ND_HOSTIP_IOV_VER) &&
		   (nd_hostip_ver->op_type == WL_ND_HOSTIP_OP_VER) &&
		   (nd_hostip_ver->length == WL_ND_HOSTIP_FIXED_LEN +
		    sizeof(uint16))) {
			printf("nd_hostip ver %d\n", nd_hostip_ver->u.version);
		} else {
			printf("nd_hostip ver 0\n");
			return BCME_UNSUPPORTED;
		}
	} else if (!stricmp(*argv, "add")) {
		/* Add host ip */
		hostip_op.version = htod16(WL_ND_HOSTIP_IOV_VER);
		hostip_op.op_type = htod16(WL_ND_HOSTIP_OP_ADD);
		hostip_op.length = htod32(WL_ND_HOSTIP_WITH_ADDR_LEN);

		if (*++argv) {
			/* address */
			if (!wl_atoipv6(*argv, &hostip_op.u.host_ip.ip_addr)) {
				/* Invalid address */
				return BCME_USAGE_ERROR;
			}

			/* type of address (unicast default) */
			hostip_op.u.host_ip.type = WL_ND_IPV6_ADDR_TYPE_UNICAST;

			if (*++argv && !stricmp(*argv, "-t")) {
				if (*++argv && !stricmp(*argv, "ac")) {
					/* anycast address */
					hostip_op.u.host_ip.type = WL_ND_IPV6_ADDR_TYPE_ANYCAST;
				}
			}

			ret = wlu_var_setbuf(wl, cmd->name, &hostip_op,
					WL_ND_HOSTIP_WITH_ADDR_LEN);
		} else {
			/* no address given */
			return BCME_USAGE_ERROR;
		}
	} else if (!stricmp(*argv, "del")) {
		/* Delete host ip */
		hostip_op.version = htod16(WL_ND_HOSTIP_IOV_VER);

		if (!*++argv) {
			/* delete all address */
			hostip_op.op_type = htod16(WL_ND_HOSTIP_OP_DEL_ALL);
			hostip_op.length = htod32(WL_ND_HOSTIP_FIXED_LEN);
			ret = wlu_var_setbuf(wl, cmd->name, &hostip_op, WL_ND_HOSTIP_FIXED_LEN);
		} else if (!stricmp(*argv, "-t")) {
			if (*++argv) {
				if (!stricmp(*argv, "uc")) {
					/* delete unicast address */
					hostip_op.op_type = htod16(WL_ND_HOSTIP_OP_DEL_UC);
					hostip_op.length = htod32(WL_ND_HOSTIP_FIXED_LEN);
				} else if (!stricmp(*argv, "ac")) {
					/* delete anycast address */
					hostip_op.op_type = htod16(WL_ND_HOSTIP_OP_DEL_AC);
					hostip_op.length = htod32(WL_ND_HOSTIP_FIXED_LEN);
				} else {
					return BCME_USAGE_ERROR;
				}
				ret = wlu_var_setbuf(wl, cmd->name, &hostip_op,
						WL_ND_HOSTIP_FIXED_LEN);
			} else {
				return BCME_USAGE_ERROR;
			}
		} else if (wl_atoipv6(*argv, &hostip_op.u.host_ip.ip_addr)) {
			/* delete specified address */
			hostip_op.op_type = htod16(WL_ND_HOSTIP_OP_DEL);
			hostip_op.length = htod32(WL_ND_HOSTIP_WITH_ADDR_LEN);
			hostip_op.u.host_ip.type = 0;   /* don't care */
			ret = wlu_var_setbuf(wl, cmd->name, &hostip_op,
					WL_ND_HOSTIP_WITH_ADDR_LEN);
		} else {
			return BCME_USAGE_ERROR;
		}

	} else if (!stricmp(*argv, "list")) {
		/* get list of address */
		wl_nd_host_ip_list_t *list = NULL;

		hostip_op.version = htod16(WL_ND_HOSTIP_IOV_VER);
		hostip_op.op_type = htod16(WL_ND_HOSTIP_OP_LIST);
		hostip_op.length = htod32(WL_ND_HOSTIP_FIXED_LEN);

		ret = wlu_var_getbuf(wl, cmd->name, &hostip_op,
				WL_ND_HOSTIP_FIXED_LEN, (void **)&list);
		if (!ret) {
			uint i;
			list->count = dtoh32(list->count);
			for (i = 0; i < list->count; i++) {
				uint32 addr_type = list->host_ip[i].type;
				wl_print_ipv6_addr(&list->host_ip[i].ip_addr);
				if (addr_type == WL_ND_IPV6_ADDR_TYPE_UNICAST) {
					printf(" unicast\r\n");
				} else if (addr_type == WL_ND_IPV6_ADDR_TYPE_ANYCAST) {
					printf(" anycast\r\n");
				} else {
					printf(" unknown type\n\n");
				}
			}
		}
	} else {
		/* Set for nd_hostip iovar old syntax */
		return wl_hostipv6(wl, cmd, saved_argv);
	}

	return ret;
}

static int
wl_ndstatus(void *wl, cmd_t *cmd, char **argv)
{
	int ret = 0;
	struct nd_ol_stats_t *nd_stats;

	if (!*++argv) {
	/* Get */
	void *ptr = NULL;

		if ((ret = wlu_var_getbuf(wl, cmd->name, NULL, 0, &ptr)) < 0)
			return ret;

		nd_stats = (struct nd_ol_stats_t *)ptr;

		printf("host_ip_entries %d\r\n", nd_stats->host_ip_entries);
		printf("host_ip_overflow %d\r\n", nd_stats->host_ip_overflow);
		printf("peer_request %d\r\n", nd_stats->peer_request);
		printf("peer_request_drop %d\r\n", nd_stats->peer_request_drop);
		printf("peer_reply_drop %d\r\n", nd_stats->peer_reply_drop);
		printf("peer_service %d\r\n", nd_stats->peer_service);
	} else {
		printf("Cannot set nd stats\n");
	}

	return 0;
}

/*
 * If a solicit IP address is given, add it
 * e.g. "wl nd_solicitip fe00:0:0:0:0:290:1fc0:18c0 ".
 * If no address is given, dump all the addresses.
 */
static int
wl_solicitipv6(void *wl, cmd_t *cmd, char **argv)
{
	int ret, i;
	struct ipv6_addr ipa_set, *ipa_get, null_ipa;
	uint16 *ip_addr;
	if (!*++argv) {
		/* Get */
		void *ptr = NULL;

		if ((ret = wlu_var_getbuf(wl, cmd->name, NULL, 0, &ptr)) < 0)
			return ret;

		ip_addr = (uint16*)ptr;
		memset(null_ipa.addr, 0, IPV6_ADDR_LEN);
		for (ipa_get = (struct ipv6_addr *)ptr;
			 memcmp(null_ipa.addr, ipa_get->addr, IPV6_ADDR_LEN) != 0;
			 ipa_get++) {
			/* Print ipv6 Addr */
			for (i = 0; i < 8; i++) {
				printf("%x", ntoh16(ip_addr[i]));
				if (i < 7)
					printf(":");
			}
			printf("\r\n");

			ip_addr += 8;
		}
	} else {
		/* Add */
		if (!wl_atoipv6(*argv, &ipa_set))
			return BCME_USAGE_ERROR;

		/* we add one ip-addr at a time */
		return wlu_var_setbuf(wl, cmd->name, &ipa_set, IPV6_ADDR_LEN);
	}
	return ret;
}

/*
 * If a remote IP address is given, add it
 * e.g. "wl nd_remoteip fe00:0:0:0:0:290:1fc0:18c0 ".
 * If no address is given, dump the addresses.
 */
static int
wl_remoteipv6(void *wl, cmd_t *cmd, char **argv)
{
	int ret, i;
	struct ipv6_addr ipa_set, *ipa_get, null_ipa;
	uint16 *ip_addr;
	if (!*++argv) {
		/* Get */
		void *ptr = NULL;

		if ((ret = wlu_var_getbuf(wl, cmd->name, NULL, 0, &ptr)) < 0)
			return ret;

		ip_addr = (uint16*)ptr;
		memset(null_ipa.addr, 0, IPV6_ADDR_LEN);
		for (ipa_get = (struct ipv6_addr *)ptr;
			 memcmp(null_ipa.addr, ipa_get->addr, IPV6_ADDR_LEN) != 0;
			 ipa_get++) {
			/* Print ipv6 Addr */
			for (i = 0; i < 8; i++) {
				printf("%x", ntoh16(ip_addr[i]));
				if (i < 7)
					printf(":");
			}
			printf("\r\n");

			ip_addr += 8;
		}
	} else {
		/* Add */
		if (!wl_atoipv6(*argv, &ipa_set))
			return BCME_USAGE_ERROR;

		/* we add one ip-addr at a time */
		return wlu_var_setbuf(wl, cmd->name, &ipa_set, IPV6_ADDR_LEN);
	}
	return ret;
}
