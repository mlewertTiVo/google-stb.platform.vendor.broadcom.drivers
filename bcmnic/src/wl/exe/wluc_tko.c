/*
 * wl tko command module - TCP Keepalive Offload
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

static cmd_func_t wl_tko;
static cmd_func_t wl_tko_auto_config;
static cmd_func_t wl_tko_event_check;

static cmd_t wl_tko_cmds[] = {
	{ "tko", wl_tko, WLC_GET_VAR, WLC_SET_VAR,
	"TCP keepalive offload subcommands:\n"
	"\ttko max_tcp\n"
	"\ttko param <interval> <retry interval> <retry count>\n"
	"\ttko connect <index> [<ip_addr_type> <src MAC> <dst MAC>\n"
		"<src IP> <dst IP> <src port> <dst port>\n"
		"<seq num> <ack num> <TCP window>]\n"
	"\ttko enable <0|1>\n"
	"\ttko status\n"},
	{ "tko_auto_config", wl_tko_auto_config, -1, WLC_SET_VAR,
	"Listens for TCP and automatically configures TKO.\n"},
	{ "tko_event_check", wl_tko_event_check, -1, WLC_SET_VAR,
	"Listens for TKO event.\n"},
	{ NULL, NULL, 0, 0, NULL }
};

static char *buf;

/* module initialization */
void
wluc_tko_module_init(void)
{
	/* get the global buf */
	buf = wl_get_buf();

	/* register tko commands */
	wl_module_cmds_register(wl_tko_cmds);
}

/* encode IPv6 TCP keepalive packet
 * input 'buf' size = (ETHER_HDR_LEN + sizeof(struct ipv6_hdr) +
 * 	sizeof(struct bcmtcp_hdr))
 * returns the size of the encoded packet
 */
static int
wl_ipv6_tcp_keepalive_pkt(uint8 *buf,
	struct ether_addr *daddr, struct ether_addr *saddr,
	struct ipv6_addr *dipv6addr, struct ipv6_addr *sipv6addr,
	uint16 sport, uint16 dport, uint32 seq, uint32 ack, uint16 tcpwin)
{
	struct ether_header *eth;
	struct ipv6_hdr *ipv6;
	struct bcmtcp_hdr *tcp;

	if (buf == NULL) {
		return 0;
	}

	/* encode ethernet header */
	eth = (struct ether_header *)buf;
	memcpy(eth->ether_dhost, daddr, ETHER_ADDR_LEN);
	memcpy(eth->ether_shost, saddr, ETHER_ADDR_LEN);
	eth->ether_type = hton16(ETHER_TYPE_IPV6);

	/* encode IPv6 header */
	ipv6 = (struct ipv6_hdr *)(buf + sizeof(struct ether_header));
	memset(ipv6, 0, sizeof(*ipv6));
	ipv6->version = IPV6_VERSION;
	ipv6->payload_len = hton16(sizeof(struct bcmtcp_hdr));
	ipv6->nexthdr = IP_PROT_TCP;
	ipv6->hop_limit = IPV6_HOP_LIMIT;
	memcpy(&ipv6->daddr, dipv6addr, sizeof(ipv6->daddr));
	memcpy(&ipv6->saddr, sipv6addr, sizeof(ipv6->saddr));

	/* encode TCP header */
	tcp = (struct bcmtcp_hdr *)(buf + sizeof(struct ether_header) + sizeof(struct ipv6_hdr));
	memset(tcp, 0, sizeof(*tcp));
	tcp->src_port = hton16(sport);
	tcp->dst_port = hton16(dport);
	tcp->seq_num = hton32(seq);
	tcp->ack_num = hton32(ack);
	tcp->tcpwin = hton16(tcpwin);
	tcp->chksum = 0;
	tcp->urg_ptr = 0;
	tcp->hdrlen_rsvd_flags = (TCP_FLAG_ACK << 8) |
	    ((sizeof(struct bcmtcp_hdr) >> 2) << TCP_HDRLEN_SHIFT);

	/* calculate checksum */
	tcp->chksum = ipv6_tcp_hdr_cksum((uint8 *)ipv6, (uint8 *)tcp, sizeof(*tcp));

	return (sizeof(struct ether_header) + sizeof(struct ipv6_hdr) +
		sizeof(struct bcmtcp_hdr));
}

/* encode TCP keepalive packet
 * input 'buf' size = (ETHER_HDR_LEN + sizeof(struct ipv4_hdr) +
 * 	sizeof(struct bcmtcp_hdr))
 * returns the size of the encoded packet
 */
static int
wl_tcp_keepalive_pkt(uint8 *buf,
	struct ether_addr *daddr, struct ether_addr *saddr,
	struct ipv4_addr *dipaddr, struct ipv4_addr *sipaddr,
	uint16 sport, uint16 dport, uint32 seq, uint32 ack, uint16 tcpwin)
{
	struct ether_header *eth;
	struct ipv4_hdr *ip;
	struct bcmtcp_hdr *tcp;

	if (buf == NULL) {
		return 0;
	}
	eth = (struct ether_header *)buf;
	memcpy(eth->ether_dhost, daddr, ETHER_ADDR_LEN);
	memcpy(eth->ether_shost, saddr, ETHER_ADDR_LEN);
	eth->ether_type = hton16(ETHER_TYPE_IP);
	ip = (struct ipv4_hdr *)(buf + sizeof(struct ether_header));
	memcpy(ip->dst_ip, dipaddr, IPV4_ADDR_LEN);
	memcpy(ip->src_ip, sipaddr, IPV4_ADDR_LEN);
	ip->version_ihl = (IP_VER_4 << IPV4_VER_SHIFT) | (IPV4_MIN_HEADER_LEN / 4);
	ip->tos = 0;
	ip->tot_len = hton16(sizeof(struct ipv4_hdr) + sizeof(struct bcmtcp_hdr));
	ip->id = hton16(0);
	ip->frag = 0;
	ip->ttl = 32;
	ip->prot = IP_PROT_TCP;
	ip->hdr_chksum = 0;
	ip->hdr_chksum = ipv4_hdr_cksum((uint8 *)ip, IPV4_HLEN(ip));
	tcp = (struct bcmtcp_hdr *)(buf + sizeof(struct ether_header) +
	    sizeof(struct ipv4_hdr));
	tcp->src_port = hton16(sport);
	tcp->dst_port = hton16(dport);
	tcp->seq_num = hton32(seq);
	tcp->ack_num = hton32(ack);
	tcp->tcpwin = hton16(tcpwin);
	tcp->chksum = 0;
	tcp->urg_ptr = 0;
	tcp->hdrlen_rsvd_flags = (TCP_FLAG_ACK << 8) |
	    ((sizeof(struct bcmtcp_hdr) >> 2) << TCP_HDRLEN_SHIFT);

	/* calculate TCP header checksum */
	tcp->chksum = ipv4_tcp_hdr_cksum((uint8 *)ip, (uint8 *)tcp, sizeof(*tcp));

	return (sizeof(struct ether_header) + sizeof(struct ipv4_hdr) +
		sizeof(struct bcmtcp_hdr));
}

/* intialize TKO connect struct and returns the length of connect struct */
static int
wl_tko_connect_init(wl_tko_connect_t *connect, uint8 index, uint8 ip_addr_type,
	struct ether_addr *saddr, struct ether_addr *daddr, void *sipaddr, void *dipaddr,
	uint16 local_port, uint16 remote_port, uint32 local_seq, uint32 remote_seq, uint16 tcpwin)
{
	int data_len = 0;

	connect->index = index;
	connect->ip_addr_type = ip_addr_type;
	connect->local_port = htod16(local_port);
	connect->remote_port = htod16(remote_port);
	connect->local_seq = htod32(local_seq);
	connect->remote_seq = htod32(remote_seq);

	if (ip_addr_type) {
		/* IPv6 */
		struct ipv6_addr *sipv6addr = sipaddr;
		struct ipv6_addr *dipv6addr = dipaddr;

		memcpy(&connect->data[data_len], sipv6addr, sizeof(*sipv6addr));
		data_len += sizeof(*sipv6addr);
		memcpy(&connect->data[data_len], dipv6addr, sizeof(*dipv6addr));
		data_len += sizeof(*dipv6addr);
		/* IPv6 TCP keepalive request */
		connect->request_len = wl_ipv6_tcp_keepalive_pkt(
			&connect->data[data_len], daddr, saddr,
			dipv6addr, sipv6addr, local_port, remote_port,
			local_seq - 1, remote_seq, tcpwin);
		data_len += connect->request_len;
		/* IPv6 TCP keepalive response */
		connect->response_len = wl_ipv6_tcp_keepalive_pkt(
			&connect->data[data_len], daddr, saddr,
			dipv6addr, sipv6addr, local_port, remote_port,
			local_seq, remote_seq, tcpwin);
			data_len += connect->response_len;
	} else {
		/* IPv4 */
		struct ipv4_addr *sipv4addr = sipaddr;
		struct ipv4_addr *dipv4addr = dipaddr;

		memcpy(&connect->data[data_len], sipv4addr, sizeof(*sipv4addr));
		data_len += sizeof(*sipv4addr);
		memcpy(&connect->data[data_len], dipv4addr, sizeof(*dipv4addr));
		data_len += sizeof(*dipv4addr);
		/* TCP keepalive request */
		connect->request_len = wl_tcp_keepalive_pkt(
			&connect->data[data_len], daddr, saddr,
			dipv4addr, sipv4addr, local_port, remote_port,
			local_seq - 1, remote_seq, tcpwin);
		data_len += connect->request_len;
		/* TCP keepalive response */
		connect->response_len = wl_tcp_keepalive_pkt(
			&connect->data[data_len], daddr, saddr,
			dipv4addr, sipv4addr, local_port, remote_port,
			local_seq, remote_seq, tcpwin);
		data_len += connect->response_len;
	}

	return (OFFSETOF(wl_tko_connect_t, data) + data_len);
}

static int
wl_tko(void *wl, cmd_t *cmd, char **argv)
{
	int err = -1;
	char *subcmd;
	int subcmd_len;

	/* skip iovar */
	argv++;

	/* must have subcommand */
	subcmd = *argv++;
	if (!subcmd) {
		return BCME_USAGE_ERROR;
	}
	subcmd_len = strlen(subcmd);

	/* GET for "connect" has 1 parameter */
	if ((!*argv && strncmp(subcmd, "connect", subcmd_len)) ||
		(!strncmp(subcmd, "connect", subcmd_len) && argv[0] && !argv[1])) {
		/* get */
		uint8 buffer[OFFSETOF(wl_tko_t, data) +	sizeof(wl_tko_get_connect_t)];
		wl_tko_t *tko = (wl_tko_t *)buffer;
		int len = OFFSETOF(wl_tko_t, data);

		memset(tko, 0, len);
		if (!strncmp(subcmd, "max_tcp", subcmd_len)) {
			tko->subcmd_id = WL_TKO_SUBCMD_MAX_TCP;
		} else if (!strncmp(subcmd, "param", subcmd_len)) {
			tko->subcmd_id = WL_TKO_SUBCMD_PARAM;
		} else if (!strncmp(subcmd, "connect", subcmd_len)) {
			wl_tko_get_connect_t *get_connect = (wl_tko_get_connect_t *)tko->data;
			tko->subcmd_id = WL_TKO_SUBCMD_CONNECT;
			tko->len = sizeof(*get_connect);
			get_connect->index = atoi(argv[0]);
		} else if (!strncmp(subcmd, "enable", subcmd_len)) {
			tko->subcmd_id = WL_TKO_SUBCMD_ENABLE;
		} else if (!strncmp(subcmd, "status", subcmd_len)) {
			tko->subcmd_id = WL_TKO_SUBCMD_STATUS;
		} else {
			return BCME_USAGE_ERROR;
		}
		len += tko->len;

		/* invoke GET iovar */
		tko->subcmd_id = htod16(tko->subcmd_id);
		tko->len = htod16(tko->len);
		if ((err = wlu_iovar_getbuf(wl, cmd->name, tko, len, buf, WLC_IOCTL_MEDLEN)) < 0) {
			return err;
		}

		/* process and print GET results */
		tko = (wl_tko_t *)buf;
		tko->subcmd_id = dtoh16(tko->subcmd_id);
		tko->len = dtoh16(tko->len);

		switch (tko->subcmd_id) {
		case WL_TKO_SUBCMD_MAX_TCP:
		{
			wl_tko_max_tcp_t *max_tcp =
				(wl_tko_max_tcp_t *)tko->data;
			if (tko->len >= sizeof(*max_tcp)) {
				printf("max TCP: %d\n", max_tcp->max);
			} else {
				err = BCME_BADLEN;
			}
			break;

		}
		case WL_TKO_SUBCMD_PARAM:
		{
			wl_tko_param_t *param =	(wl_tko_param_t *)tko->data;
			if (tko->len >= sizeof(*param)) {
				param->interval = dtoh16(param->interval);
				param->retry_interval = dtoh16(param->retry_interval);
				param->retry_count = dtoh16(param->retry_count);
				printf("interval: %d\n", param->interval);
				printf("retry interval: %d\n", param->retry_interval);
				printf("retry count: %d\n", param->retry_count);
			} else {
				err = BCME_BADLEN;
			}
			break;
		}
		case WL_TKO_SUBCMD_CONNECT:
		{
			wl_tko_connect_t *connect = (wl_tko_connect_t *)tko->data;
			if (tko->len >= sizeof(*connect)) {
				connect->local_port = dtoh16(connect->local_port);
				connect->remote_port = dtoh16(connect->remote_port);
				connect->local_seq = dtoh32(connect->local_seq);
				connect->remote_seq = dtoh32(connect->remote_seq);
				printf("index:        %d\n", connect->index);
				printf("IP addr type: %d\n", connect->ip_addr_type);
				printf("local port:   %u\n", connect->local_port);
				printf("remote port:  %u\n", connect->remote_port);
				printf("local seq:    %u\n", connect->local_seq);
				printf("remote seq:   %u\n", connect->remote_seq);
				if (connect->ip_addr_type) {
					/* IPv6 */
					printf("src IPv6:     %s\n",
						wl_ipv6toa((struct ipv6_addr *)&connect->data[0]));
					printf("dst IPv6:     %s\n",
						wl_ipv6toa((struct ipv6_addr *)
						&connect->data[IPV6_ADDR_LEN]));
					printf("IPv6 TCP keepalive request\n");
					wl_hexdump(&connect->data[2 * IPV6_ADDR_LEN],
						connect->request_len);
					printf("IPv6 TCP keepalive response\n");
					wl_hexdump(&connect->data[2 * IPV6_ADDR_LEN +
						connect->request_len], connect->response_len);
				} else {
					/* IPv4 */
					printf("src IP:       %s\n",
						wl_iptoa((struct ipv4_addr *)&connect->data[0]));
					printf("dst IP:       %s\n",
						wl_iptoa((struct ipv4_addr *)
						&connect->data[IPV4_ADDR_LEN]));
					printf("TCP keepalive request\n");
					wl_hexdump(&connect->data[2 * IPV4_ADDR_LEN],
						connect->request_len);
					printf("TCP keepalive response\n");
					wl_hexdump(&connect->data[2 * IPV4_ADDR_LEN +
						connect->request_len], connect->response_len);
				}
			} else {
				err = BCME_BADLEN;
			}
			break;
		}
		case WL_TKO_SUBCMD_ENABLE:
		{
			wl_tko_enable_t *tko_enable = (wl_tko_enable_t *)tko->data;
			if (tko->len >= sizeof(*tko_enable)) {
				printf("%d\n", tko_enable->enable);
			} else {
				err = BCME_BADLEN;
			}
			break;
		}
		case WL_TKO_SUBCMD_STATUS:
		{
			wl_tko_status_t *tko_status = (wl_tko_status_t *)tko->data;
			if (tko->len >= sizeof(*tko_status)) {
				int i;
				for (i = 0; i < tko_status->count; i++) {
					uint8 status = tko_status->status[i];
					printf("%d: %s\n", i,
					status == TKO_STATUS_NORMAL ? "normal" :
					status == TKO_STATUS_NO_RESPONSE ? "no response" :
					status == TKO_STATUS_NO_TCP_ACK_FLAG ? "no TCP ACK flag" :
					status == TKO_STATUS_UNEXPECT_TCP_FLAG ?
						"unexpected TCP flag" :
					status == TKO_STATUS_SEQ_NUM_INVALID ? "seq num invalid" :
					status == TKO_STATUS_REMOTE_SEQ_NUM_INVALID ?
						"remote seq num invalid" :
					status == TKO_STATUS_TCP_DATA ? "TCP data" :
					status == TKO_STATUS_UNAVAILABLE ? "unavailable" :
					"unknown");
				}
			} else {
				err = BCME_BADLEN;
			}
			break;
		}
		default:
			break;
		}
	}
	else {
		/* set */
		wl_tko_t *tko = (wl_tko_t *)buf;
		int len;

		if (!strncmp(subcmd, "param", subcmd_len) &&
			argv[0] && argv[1] && argv[2]) {
			wl_tko_param_t *param =
				(wl_tko_param_t *)tko->data;
			tko->subcmd_id = WL_TKO_SUBCMD_PARAM;
			tko->len = sizeof(*param);
			param->interval = htod16(atoi(argv[0]));
			param->retry_interval = htod16(atoi(argv[1]));
			param->retry_count = htod16(atoi(argv[2]));
		} else if (!strncmp(subcmd, "connect", subcmd_len) &&
			argv[0] && argv[1] && argv[2] && argv[3] && argv[4] &&
			argv[5] && argv[6] && argv[7] && argv[8] && argv[9] && argv[10]) {
			wl_tko_connect_t *connect = (wl_tko_connect_t *)tko->data;
			uint8 index;
			uint8 ip_addr_type;
			struct ether_addr saddr;
			struct ether_addr daddr;
			struct ipv4_addr sipaddr;
			struct ipv4_addr dipaddr;
			struct ipv6_addr sipv6addr;
			struct ipv6_addr dipv6addr;
			uint16 local_port;
			uint16 remote_port;
			uint32 local_seq;
			uint32 remote_seq;
			uint16 tcpwin;

			index = atoi(argv[0]);
			ip_addr_type = atoi(argv[1]);
			if (!wl_ether_atoe(argv[2], &saddr)) {
				return BCME_USAGE_ERROR;
			}
			if (!wl_ether_atoe(argv[3], &daddr)) {
				return BCME_USAGE_ERROR;
			}
			if (ip_addr_type) {
				/* IPv6 */
				if (!wl_atoipv6(argv[4], &sipv6addr)) {
					return BCME_USAGE_ERROR;
				}
				if (!wl_atoipv6(argv[5], &dipv6addr)) {
					return BCME_USAGE_ERROR;
				}
			} else {
				/* IPv4 */
				if (!wl_atoip(argv[4], &sipaddr)) {
					return BCME_USAGE_ERROR;
				}
				if (!wl_atoip(argv[5], &dipaddr)) {
					return BCME_USAGE_ERROR;
				}
			}
			local_port = atoi(argv[6]);
			remote_port = atoi(argv[7]);
			local_seq = bcm_strtoul(argv[8], NULL, 0);
			remote_seq = bcm_strtoul(argv[9], NULL, 0);
			tcpwin = atoi(argv[10]);

			tko->subcmd_id = WL_TKO_SUBCMD_CONNECT;
			tko->len = wl_tko_connect_init(connect, index, ip_addr_type,
				&saddr, &daddr,
				ip_addr_type ? (void *)&sipv6addr : (void *)&sipaddr,
				ip_addr_type ? (void *)&dipv6addr : (void *)&dipaddr,
				local_port, remote_port, local_seq, remote_seq, tcpwin);

		} else if (!strncmp(subcmd, "enable", subcmd_len) &&
			(!strcmp(argv[0], "0") || !strcmp(argv[0], "1"))) {
			wl_tko_enable_t *tko_enable = (wl_tko_enable_t *)tko->data;
			tko->subcmd_id = WL_TKO_SUBCMD_ENABLE;
			tko->len = sizeof(*tko_enable);
			tko_enable->enable = atoi(argv[0]);

		} else {
			return BCME_USAGE_ERROR;
		}

		/* invoke SET iovar */
		len = OFFSETOF(wl_tko_t, data) + tko->len;
		tko->subcmd_id = htod16(tko->subcmd_id);
		tko->len = htod16(tko->len);
		err = wlu_iovar_set(wl, cmd->name, tko, len);
	}

	return err;
}


#define ETH_P_ALL		0x0003
#define TKO_BUFFER_SIZE		2048
#define TKO_TCP_WINDOW_SIZE	400

/* automatically configures TKO by monitoring TCP connections */
static int
wl_tko_auto_config(void *wl, cmd_t *cmd, char **argv)
{
	/* 802.3 llc/snap header */
	static const uint8 llc_snap_hdr[SNAP_HDR_LEN] =
		{0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};
	bool is_checksum = TRUE;
	int err;
	int fd = -1;
	int i;
	wl_tko_t *tko;
	wl_tko_max_tcp_t *tko_max;
	uint8 cur_tcp = 0;
	uint8 max_tcp = 0;
	uint8 **tko_buf = NULL;
	uint8 *frame = NULL;
	struct ether_addr ea;
	struct sockaddr_ll sll;
	struct ifreq ifr;
	char ifnames[IFNAMSIZ] = {"eth1"};

	UNUSED_PARAMETER(cmd);

	argv++;
	if (*argv) {
		if (!stricmp(*argv, "-nochecksum")) {
			/* note: ssh on FC15 has TCP checksum failures on packets read
			 * from socket but checksum correct over-the-air
			 */
			printf("no checksum check\n");
			is_checksum = FALSE;
		}
	}

	memset(&ifr, 0, sizeof(ifr));
	if (wl) {
		strncpy(ifr.ifr_name, ((struct ifreq *)wl)->ifr_name, (IFNAMSIZ - 1));
	} else {
		strncpy(ifr.ifr_name, ifnames, (IFNAMSIZ - 1));
	}

	if ((err = wlu_iovar_get(wl, "cur_etheraddr", &ea, ETHER_ADDR_LEN)) < 0) {
		printf("Failed to get current ether address\n");
		goto exit;
	}

	fd = socket(PF_PACKET, SOCK_RAW, hton16(ETH_P_ALL));
	if (fd < 0) {
		printf("Cannot create socket %d\n", fd);
		err = -1;
		goto exit;
	}

	err = ioctl(fd, SIOCGIFINDEX, &ifr);
	if (err < 0) {
		printf("Cannot get index %d\n", err);
		goto exit;
	}

	/* bind the socket first before starting so we won't miss any event */
	memset(&sll, 0, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_protocol = hton16(ETH_P_ALL);
	sll.sll_ifindex = ifr.ifr_ifindex;
	err = bind(fd, (struct sockaddr *)&sll, sizeof(sll));
	if (err < 0) {
		printf("Cannot bind %d\n", err);
		goto exit;
	}

	/* get the max TCP connections supported */
	tko = (wl_tko_t *)buf;
	memset(tko, 0, sizeof(*tko));
	tko->subcmd_id = htod16(WL_TKO_SUBCMD_MAX_TCP);
	tko->len = htod16(tko->len);
	if ((err = wlu_iovar_getbuf(wl, "tko", tko, sizeof(*tko), buf, WLC_IOCTL_SMLEN)) < 0) {
		goto exit;
	}
	tko = (wl_tko_t *)buf;
	tko_max = (wl_tko_max_tcp_t *)tko->data;
	max_tcp = tko_max->max;

	/* allocate TKO buffers */
	tko_buf = (uint8 **)malloc(sizeof(uint8 *) * max_tcp);
	if (tko_buf == NULL) {
		printf("Cannot not allocate %d bytes for TKO buffer\n",
			(int)(sizeof(uint8 *) * max_tcp));
		err = BCME_NOMEM;
		goto exit;
	}
	for (i = 0; i < max_tcp; i++) {
		int size = OFFSETOF(wl_tko_t, data) + OFFSETOF(wl_tko_connect_t, data) + 1024;
		tko_buf[i] = malloc(size);
		if (tko_buf[i] == NULL) {
			printf("Cannot not allocate %d bytes for TKO buffer\n",	size);
			err = BCME_NOMEM;
			goto exit;
		}
		memset(tko_buf[i], 0, size);
	}

	/* frame buffer */
	frame = (uint8 *)malloc(TKO_BUFFER_SIZE);
	if (frame == NULL) {
		printf("Cannot not allocate %d bytes for receive buffer\n",
			TKO_BUFFER_SIZE);
		err = BCME_NOMEM;
		goto exit;
	}

	/* receive result */
	while (1) {
		int length;
		uint8 *end, *pt;
		uint16 ethertype;
		struct bcmtcp_hdr *tcp_hdr = NULL;
		uint16 tcp_len = 0;
		uint8 ip_addr_type = 0;
		uint8 *src_ip = NULL;
		uint8 *dst_ip = NULL;

		length = recv(fd, frame, TKO_BUFFER_SIZE, 0);
		end = frame + length;

		if (length < ETHER_HDR_LEN) {
			continue;
		}

		/* process only tx'ed packets */
		if (memcmp(ea.octet, &frame[ETHER_SRC_OFFSET], sizeof(ea)) != 0) {
			continue;
		}

		/* process Ethernet II or SNAP-encapsulated 802.3 frames */
		if (ntoh16_ua((const void *)(frame + ETHER_TYPE_OFFSET)) >= ETHER_TYPE_MIN) {
			/* Frame is Ethernet II */
			pt = frame + ETHER_TYPE_OFFSET;
		} else if (length >= ETHER_HDR_LEN + SNAP_HDR_LEN + ETHER_TYPE_LEN &&
			!memcmp(llc_snap_hdr, frame + ETHER_HDR_LEN, SNAP_HDR_LEN)) {
			pt = frame + ETHER_HDR_LEN + SNAP_HDR_LEN;
		} else {
			continue;
		}

		ethertype = ntoh16_ua((const void *)pt);
		pt += ETHER_TYPE_LEN;

		/* check ethertype */
		if ((ethertype != ETHER_TYPE_IP) && (ethertype != ETHER_TYPE_IPV6)) {
			continue;
		}

		if (ethertype == ETHER_TYPE_IP) {
			struct ipv4_hdr *ipv4_hdr;
			uint16 iphdr_len;
			uint16 cksum;

			ipv4_hdr = (struct ipv4_hdr *)pt;
			iphdr_len = IPV4_HLEN(ipv4_hdr);
			if (pt + iphdr_len > end) {
				continue;
			}
			pt += iphdr_len;

			if (ipv4_hdr->prot != IP_PROT_TCP) {
				continue;
			}

			if (ntoh16(ipv4_hdr->frag) & IPV4_FRAG_OFFSET_MASK) {
				continue;
			}

			if (ntoh16(ipv4_hdr->frag) & IPV4_FRAG_MORE) {
				continue;
			}

			tcp_len = ntoh16(ipv4_hdr->tot_len) - iphdr_len;
			if (pt + tcp_len > end) {
				continue;
			}
			tcp_hdr = (struct bcmtcp_hdr *)pt;

			/* verify IP header checksum */
			cksum = ipv4_hdr_cksum((uint8 *)ipv4_hdr, iphdr_len);
			if (cksum != ipv4_hdr->hdr_chksum) {
				continue;
			}

			/* verify TCP header checksum */
			if (is_checksum) {
				cksum = ipv4_tcp_hdr_cksum((uint8 *)ipv4_hdr,
					(uint8 *)tcp_hdr, tcp_len);
				if (cksum != tcp_hdr->chksum) {
					printf("TCP checksum failed: 0x%04x 0x%04x\n",
						ntoh16(cksum), ntoh16(tcp_hdr->chksum));
					continue;
				}
			}

			ip_addr_type = 0;
			src_ip = ipv4_hdr->src_ip;
			dst_ip = ipv4_hdr->dst_ip;

		} else if (ethertype == ETHER_TYPE_IPV6) {
			struct ipv6_hdr *ipv6_hdr;
			uint16 cksum;

			if (pt + sizeof(struct ipv6_hdr) > end) {
				continue;
			}
			ipv6_hdr = (struct ipv6_hdr *)pt;
			pt += sizeof(struct ipv6_hdr);

			if (ipv6_hdr->nexthdr != IP_PROT_TCP) {
				continue;
			}

			tcp_len = ntoh16(ipv6_hdr->payload_len);
			if (pt + tcp_len > end) {
				continue;
			}
			tcp_hdr = (struct bcmtcp_hdr *)pt;

			/* verify TCP checksum */
			if (is_checksum) {
				cksum = ipv6_tcp_hdr_cksum((uint8 *)ipv6_hdr,
					(uint8 *)tcp_hdr, tcp_len);
				if (cksum != tcp_hdr->chksum) {
					printf("IPv6 TCP checksum failed: 0x%04x 0x%04x\n",
						ntoh16(cksum), ntoh16(tcp_hdr->chksum));
					continue;
				}
			}

			ip_addr_type = 1;
			src_ip = ipv6_hdr->saddr.addr;
			dst_ip = ipv6_hdr->daddr.addr;
		}

		if (src_ip != NULL && dst_ip != NULL && tcp_hdr != NULL) {
			uint16 src_port = ntoh16(tcp_hdr->src_port);
			uint16 dst_port = ntoh16(tcp_hdr->dst_port);
			uint32 seq_num = ntoh32(tcp_hdr->seq_num);
			uint32 ack_num = ntoh32(tcp_hdr->ack_num);
			uint8 tcp_flags[2];
			uint16 tcp_data_len;
			wl_tko_connect_t *connect;
			uint8 index;
			int len;

			/* find existing TCP connection */
			for (i = 0; i < cur_tcp; i++) {
				int ip_addr_size = ip_addr_type ? sizeof(struct ipv6_addr) :
					sizeof(struct ipv4_addr);

				tko = (wl_tko_t *)tko_buf[i];
				connect = (wl_tko_connect_t *)tko->data;

				/* existing TCP connection if src/dst IP and ports match */
				if (connect->ip_addr_type == ip_addr_type &&
					memcmp(src_ip, &connect->data[0], ip_addr_size) == 0 &&
					memcmp(dst_ip, &connect->data[ip_addr_size],
						ip_addr_size) == 0 &&
					connect->local_port == src_port &&
					connect->remote_port == dst_port) {
					/* found existing TCP connection */
					index = connect->index;
					break;
				}
			}
			/* TCP connection not found */
			if (i == cur_tcp) {
				if (cur_tcp < max_tcp) {
					/* new TCP connection */
					index = cur_tcp;
					cur_tcp++;
				} else {
					/* no more */
					printf("max TCP connections supported %d\n", max_tcp);
					continue;
				}
			}

			/* TCP data length */
			memcpy(tcp_flags, &tcp_hdr->hdrlen_rsvd_flags, sizeof(tcp_flags));
			tcp_data_len = tcp_len - (TCP_HDRLEN(tcp_flags[0]) << 2);
			if (tcp_data_len != 0) {
				printf("TCP data len=%d\n", tcp_data_len);
			}

			/* adjust the sequence num with the TCP data length */
			seq_num += tcp_data_len;

			printf("index:        %d\n", index);
			if (ip_addr_type) {
				/* IPv6 */
				printf("src IPv6:     %s\n",
					wl_ipv6toa((struct ipv6_addr *)src_ip));
				printf("dst IPv6:     %s\n",
					wl_ipv6toa((struct ipv6_addr *)dst_ip));
			} else {
				/* IPv4 */
				printf("src IP:       %s\n",
					wl_iptoa((struct ipv4_addr *)src_ip));
				printf("dst IP:       %s\n",
					wl_iptoa((struct ipv4_addr *)dst_ip));
			}
			printf("local port:   %u\n", src_port);
			printf("remote port:  %u\n", dst_port);
			printf("local seq:    %u\n", seq_num);
			printf("remote seq:   %u\n", ack_num);

			tko = (wl_tko_t *)tko_buf[index];
			connect = (wl_tko_connect_t *)tko->data;

			tko->len = wl_tko_connect_init(connect, index, ip_addr_type,
				(struct ether_addr *)&frame[ETHER_SRC_OFFSET],
				(struct ether_addr *)&frame[ETHER_DEST_OFFSET],
				src_ip, dst_ip, src_port, dst_port, seq_num,
				ack_num, TKO_TCP_WINDOW_SIZE);

			/* invoke SET iovar */
			tko->subcmd_id = WL_TKO_SUBCMD_CONNECT;
			len = OFFSETOF(wl_tko_t, data) + tko->len;
			tko->subcmd_id = htod16(tko->subcmd_id);
			tko->len = htod16(tko->len);
			err = wlu_iovar_set(wl, "tko", tko, len);
			if (err < 0) {
				printf("TKO connect failed\n");
				break;
			}
		}
	}

exit:
	if (tko_buf != NULL) {
		for (i = 0; i < max_tcp; i++) {
			if (tko_buf[i] != NULL) {
				free(tko_buf[i]);
			}
		}
		free(tko_buf);
	}
	if (frame != NULL) {
		free(frame);
	}
	if (fd >= 0) {
		close(fd);
	}
	return err;
}

static void
wl_tko_event_cb(int event_type, bcm_event_t *bcm_event)
{
	wl_event_tko_t *data = (wl_event_tko_t *) (bcm_event + 1);

	if (event_type == WLC_E_TKO) {
		printf("WLC_E_TKO(%d): status=%d index=%d\n",
			WLC_E_TKO, ntoh32(bcm_event->event.status), data->index);
	}
}

static int
wl_tko_event_check(void *wl, cmd_t *cmd, char **argv)
{
	if (argv[1] && argv[1][0] == '-') {
		wl_cmd_usage(stderr, cmd);
		return -1;
	}
	return wl_wait_for_event(wl, argv, WLC_E_TKO, 256, wl_tko_event_cb);
}
