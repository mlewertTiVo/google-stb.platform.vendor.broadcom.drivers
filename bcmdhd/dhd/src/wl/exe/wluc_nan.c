/*
 * wl nan command module
 *
 * Copyright (C) 2018, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wluc_nan.c 458728 2014-02-27 18:15:25Z $
 */
#ifdef WL_NAN


#include <wlioctl.h>


/* Because IL_BIGENDIAN was removed there are few warnings that need
 * to be fixed. Windows was not compiled earlier with IL_BIGENDIAN.
 * Hence these warnings were not seen earlier.
 * For now ignore the following warnings
 */

#include <bcmutils.h>
#include <bcmendian.h>
#include "wlu_common.h"
#include "wlu.h"

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <strings.h>

#include <miniopt.h>

#include <proto/nan.h>
#include <bcmbloom.h>

static cmd_func_t wl_nan_control;

#define NAN_PARAMS_USAGE	\
"\tUsage: wl nan [command] [cmd options] as follows:\n"	\
"\t\twl nan enable [1/0] - enable disable nan functionality\n" \
"\t\twl nan attr [MAC, ETC] \n" \
"\t\twl nan publish <instance> <service name> [options]\n" \
"\t\twl nan subscribe <instance> <service name> [options]\n" \
"\t\twl nan cancel_publish <instance>\n" \
"\t\twl nan cancel_subscribe <instance>\n" \
"\t\twl nan scan\n" \
"\t\twl nan scanresults\n" \
"\t\twl nan scan_params -s <scantime> -h <hometime> -c <6,44,149>"

static cmd_t wl_nan_cmds[] = {
	{ "nan", wl_nan_control, WLC_GET_VAR, WLC_SET_VAR,
	"superfunction for nan protocol commands \n\n"
	NAN_PARAMS_USAGE},
	{ NULL, NULL, 0, 0, NULL }
};

static char *buf;

/* module initialization */
void
wluc_nan_module_init(void)
{
	/* get the global buf */
	buf = wl_get_buf();

	/* register nan commands */
	wl_module_cmds_register(wl_nan_cmds);
}

static int
bcm_pack_xtlv_entry_from_hex_string(uint8 **tlv_buf, uint16 *buflen, uint16 type, char *hex);
static int wl_nan_bloom_create(bcm_bloom_filter_t** bp, uint* idx, uint size);
static void* wl_nan_bloom_alloc(void *ctx, uint size);
static void wl_nan_bloom_free(void *ctx, void *buf, uint size);
static uint wl_nan_hash(void* ctx, uint idx, const uint8 *input, uint input_len);
/* ************************** wl NAN command & event handlers ********************** */
#define NAN_ERROR(x) printf x
#define WL_NAN_FUNC(suffix) wl_nan_subcmd_ ##suffix
#define NAN_IOC_BUFSZ  256 /* some sufficient ioc buff size for our module */
#define NAN_BLOOM_LENGTH_DEFAULT	240
#define NAN_SRF_MAX_MAC	(NAN_BLOOM_LENGTH_DEFAULT / ETHER_ADDR_LEN)
/* Mask for CRC32 output, used in hash function for NAN bloom filter */
#define NAN_BLOOM_CRC32_MASK	0xFFFF

struct tlv_info_list {
	char *name;
	enum wl_nan_cmd_xtlv_id type;
};

/*  nan ioctl sub cmd handler functions  */
static cmd_handler_t wl_nan_subcmd_enable;
static cmd_handler_t wl_nan_subcmd_join;
static cmd_handler_t wl_nan_subcmd_leave;
static cmd_handler_t wl_nan_subcmd_merge;
static cmd_handler_t wl_nan_subcmd_publish;
static cmd_handler_t wl_nan_subcmd_subscribe;
static cmd_handler_t wl_nan_subcmd_cancel_publish;
static cmd_handler_t wl_nan_subcmd_cancel_subscribe;
static cmd_handler_t wl_nan_subcmd_transmit;
static cmd_handler_t wl_nan_subcmd_status;
static cmd_handler_t wl_nan_subcmd_stop;
static cmd_handler_t wl_nan_subcmd_set_attr;
static cmd_handler_t wl_nan_subcmd_event_check;
static cmd_handler_t wl_nan_subcmd_scan;
static cmd_handler_t wl_nan_subcmd_scan_params;
static cmd_handler_t wl_nan_subcmd_debug;
static cmd_handler_t wl_nan_subcmd_event_msgs;
static cmd_handler_t wl_nan_subcmd_tsreserve;
static cmd_handler_t wl_nan_subcmd_tsrelease;
static cmd_handler_t wl_nan_subcmd_OUI;

static const wl_nan_sub_cmd_t nan_cmd_list[] = {
	/* wl nan enable [0/1] or new: "wl nan [0/1]" */
	{"enable", 0x01, WL_NAN_CMD_ENABLE,
	IOVT_INT32, WL_NAN_FUNC(enable)
	},
	/* read write multiple nan attributes (obsolete) */
	{"attr", 0x01, WL_NAN_CMD_ATTR,
	IOVT_BUFFER, NULL, /* WL_NAN_FUNC(attr) OBSOLETE cmd */
	},
	/* -- var attributes (treated as cmds now) --  */
	{"master", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_MASTER_PREF, WL_NAN_FUNC(set_attr),
	},
	{"cluster_id", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_CLUSTER_ID, WL_NAN_FUNC(set_attr),
	},
	{"if_addr", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_MAC_ADDR, WL_NAN_FUNC(set_attr)
	},
	{"role", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_ROLE, WL_NAN_FUNC(set_attr)
	},
	{"bcn_interval", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_BCN_INTERVAL, WL_NAN_FUNC(set_attr)
	},
	{"chan", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_MAC_CHANSPEC, WL_NAN_FUNC(set_attr)
	},
	{"tx_rate", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_MAC_TXRATE, WL_NAN_FUNC(set_attr)
	},
	{"dw_len", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_DW_LEN, WL_NAN_FUNC(set_attr)
	},
	{"hop_count", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_MAC_HOPCNT, WL_NAN_FUNC(set_attr),
	},
	{"warm_up_time", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_WARM_UP_TIME, WL_NAN_FUNC(set_attr),
	},
	{"pre_tbtt_override", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_PTBTT_OVERRIDE, WL_NAN_FUNC(set_attr),
	},
	{"pm_option", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_PM_OPTION, WL_NAN_FUNC(set_attr),
	},
	{"OUI", 0x01, WL_NAN_CMD_OUI,
	WL_NAN_XTLV_OUI, WL_NAN_FUNC(OUI)
	},
	{"idle_dw_timeout", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_IDLE_DW_TIMEOUT, WL_NAN_FUNC(set_attr),
	},
	{"idle_dw_len", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_IDLE_DW_LEN, WL_NAN_FUNC(set_attr),
	},
	{"sdf_txtime", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_SVC_DISC_TXTIME, WL_NAN_FUNC(set_attr)
	},
	{"band", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_OPERATING_BAND, WL_NAN_FUNC(set_attr)
	},
	{"stop_bcn_tx", 0x01, WL_NAN_CMD_ATTR,
	WL_NAN_XTLV_STOP_BCN_TX, WL_NAN_FUNC(set_attr)
	},
	/* ------- nan mac/disc engine  commands ---- */
	{"join", 0x01, WL_NAN_CMD_NAN_JOIN,
	IOVT_BUFFER, WL_NAN_FUNC(join)
	},
	{"leave", 0x01, WL_NAN_CMD_LEAVE,
	IOVT_BUFFER, WL_NAN_FUNC(leave)
	},
	{"merge", 0x01, WL_NAN_CMD_MERGE,
	IOVT_INT32, WL_NAN_FUNC(merge)
	},
	{"status", 0x01, WL_NAN_CMD_STATUS,
	IOVT_BUFFER, WL_NAN_FUNC(status)
	},
	{"counters", 0x01, WL_NAN_CMD_COUNT,
	IOVT_BUFFER, WL_NAN_FUNC(status)
	},
	{"reset_cnts", 0x01, WL_NAN_CMD_CLEARCOUNT,
	IOVT_BUFFER, WL_NAN_FUNC(status)
	},
	{"stop", 0x01, WL_NAN_CMD_STOP,
	IOVT_BUFFER, WL_NAN_FUNC(stop)
	},
	{"publish", 0x01, WL_NAN_CMD_PUBLISH,
	IOVT_BUFFER, WL_NAN_FUNC(publish)
	},
	{"subscribe", 0x01, WL_NAN_CMD_SUBSCRIBE,
	IOVT_BUFFER, WL_NAN_FUNC(subscribe)
	},
	{"cancel_publish", 0x01, WL_NAN_CMD_CANCEL_PUBLISH,
	IOVT_BUFFER, WL_NAN_FUNC(cancel_publish)
	},
	{"cancel_subscribe", 0x01, WL_NAN_CMD_CANCEL_SUBSCRIBE,
	IOVT_BUFFER, WL_NAN_FUNC(cancel_subscribe)
	},
	{"transmit", 0x01, WL_NAN_CMD_TRANSMIT,
	IOVT_BUFFER, WL_NAN_FUNC(transmit)
	},
	/* uses same cmd handler for as status */
	{"show", 0x01, WL_NAN_CMD_SHOW,
	IOVT_BUFFER, WL_NAN_FUNC(status)
	},
	/* nan debug commands */
	{"scan_params", 0x01, WL_NAN_CMD_SCAN_PARAMS,
	IOVT_BUFFER,  WL_NAN_FUNC(scan_params)
	},
	{"scan", 0x01, WL_NAN_CMD_SCAN,
	IOVT_BUFFER, WL_NAN_FUNC(scan)
	},
	{"scanresults", 0x01, WL_NAN_CMD_SCAN_RESULTS,
	IOVT_BUFFER, WL_NAN_FUNC(status)  /* shares the same get cbfn handler */
	},
	{"event_msgs", 0x01, WL_NAN_CMD_EVENT_MASK,
	IOVT_BUFFER, WL_NAN_FUNC(event_msgs)
	},
	{"event_check", 0x01, WL_NAN_CMD_EVENT_CHECK,
	IOVT_BUFFER, WL_NAN_FUNC(event_check)
	},
	{"debug", 0x01, WL_NAN_CMD_DEBUG,
	IOVT_BUFFER, WL_NAN_FUNC(debug)
	},
	/* timeslot management */
	{"tsreserve", 0x01, WL_NAN_CMD_TSRESERVE,
	IOVT_BUFFER, WL_NAN_FUNC(tsreserve)
	},
	{"tsrelease", 0x01, WL_NAN_CMD_TSRELEASE,
	IOVT_BUFFER, WL_NAN_FUNC(tsrelease)
	},
	{NULL, 0, 0, 0, NULL}
};


static char *
bcm_ether_ntoa(const struct ether_addr *ea, char *buf)
{
	static const char hex[] =
	  {
		  '0', '1', '2', '3', '4', '5', '6', '7',
		  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
	  };
	const uint8 *octet = ea->octet;
	char *p = buf;
	int i;
	for (i = 0; i < 6; i++, octet++) {
		*p++ = hex[(*octet >> 4) & 0xf];
		*p++ = hex[*octet & 0xf];
		*p++ = ':';
	}
	*(p-1) = '\0';
	return (buf);
}
void print_nan_role(uint32 role)
{
	const char *msg = "";
	if (role == WL_NAN_ROLE_AUTO) {
					msg = "auto";
	} else
	if (role == WL_NAN_ROLE_NON_MASTER_NON_SYNC) {
		msg = "non-master-non-sync";
	} else
	if (role == WL_NAN_ROLE_NON_MASTER_SYNC) {
		msg = "non-master-sync";
	} else
	if (role == WL_NAN_ROLE_MASTER) {
		msg = "master";
	} else
	if (role == WL_NAN_ROLE_ANCHOR_MASTER) {
		msg = "anchor-master";
	} else {
		msg = "undefined";
	}
	printf("> role %d: %s\n", role, msg);
}

static int print_vendor_info(uint8 *p, uint16 len)
{
	char buf[20];
	uint8 mapcontrol;
	uint8 proto;
	chanspec_t chanspec;
	uint32 bitmap;

	if (*p != NAN_ATTR_VENDOR_SPECIFIC || len < NAN_VENDOR_HDR_SIZE)
		return BCME_ERROR;

	if (*(p + 6) == NAN_VENDOR_TYPE_RTT) {
	p += NAN_VENDOR_HDR_SIZE;

	bcm_ether_ntoa((struct ether_addr *)p, buf);
	p += ETHER_ADDR_LEN;
	mapcontrol = *p++;
	proto = *p++;
	bitmap = *(uint32 *)p;
	p += 4;
	chanspec = *(chanspec_t *)p;

		printf("----------------------------------------------------------------------\n");
	printf("RTT MAC %s Chanspec 0x%x Bitmap 0x%08x Mapcontrol 0x%x Proto %d\n", buf,
		chanspec, bitmap, mapcontrol, proto);
	} else {
		prhex("unKnown Vendor Specific Info", p, len);
	}

	return BCME_OK;
}

static int print_nan_tsinfo(uint8 *p, uint16 len)
{
	nan_timeslot_t *tsp = (nan_timeslot_t *)p;
	int i;

	if (len < sizeof(*tsp))
		return BCME_ERROR;

	printf("Bitmap 0x%08x\n", tsp->abitmap);
	for (i = 0; i < NAN_MAX_TIMESLOT; i++)
		if (tsp->abitmap & (1 << i))
			printf("Slot %d Chanspec %x\n", i, tsp->chanlist[i]);

	return BCME_OK;
}

static void
get_nan_band(char *str, uint8 band)
{
	if (band == NAN_BAND_B)
		memcpy(str, "b", WL_NAN_BAND_STR_SIZE);
	else if (band == NAN_BAND_A)
		memcpy(str, "a", WL_NAN_BAND_STR_SIZE);
	else if (band == NAN_BAND_AUTO)
		memcpy(str, "auto", WL_NAN_BAND_STR_SIZE);
}

static uint8
set_nan_band(char *str)
{
	uint8 nan_band = NAN_BAND_INVALID;
	if (!strcmp("a", str))
		nan_band = NAN_BAND_A;
	else if (!strcmp("b", str))
		nan_band = NAN_BAND_B;
	else if (!strcmp("auto", str))
		nan_band = NAN_BAND_AUTO;

	return nan_band;
}

/*
 *  a cbfn function, displays bcm_xtlv variables rcvd in get ioctl's xtlv buffer.
 *  it can process GET result for all nan commands, provided that
 *  XTLV types (AKA the explicit xtlv types) packed into the ioctl buff
 *  are unique across all nan ioctl commands
 */
int wlu_nan_set_vars_cbfn(void *ctx, uint8 *data, uint16 type, uint16 len)
{
	int res = BCME_OK;
	uint8	uv8;
	uint16	uv16;
	uint32	uv32;
	chanspec_t chanspec;
	uint16	cmd;
	const char *msg = "";
	cmd = ((wl_nan_ioc_t *)(ctx))->id;
	char buf[64];
	char str[WL_NAN_BAND_STR_SIZE];

	/*  cbfn params may be used in f/w */
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(len);
	UNUSED_PARAMETER(msg);

	/* printf("cmd:%d, type: %x, len:%d, data: %x %x\n", cmd, type, len, *data, *(data+1)); */

		switch (type) {
			case WL_NAN_XTLV_ENABLED:
				uv8 = *data;
				if (cmd == WL_NAN_CMD_MERGE)
					printf("> merge: %d\n", uv8);
				else
				printf("> nan: %s\n", uv8?"enabled":"disabled");
				break;
			case WL_NAN_XTLV_MASTER_PREF:
			/* master role and preference  mac has them as two u8's  */
				uv16 = ltoh16_ua(data);
				/* attrib is present unpacked */
					printf("> master preference:%d \n", uv16 & 0x0ff);
					printf("> rnd_factor:%d\n", uv16  >> 8);
				break;
			case WL_NAN_XTLV_MASTER_RANK:
				prhex("> master_rank", (uint8 *)data, NAN_MASTER_RANK_LEN);
				break;
			case WL_NAN_XTLV_CLUSTER_ID:
			/* interface address */
				if (cmd == WL_NAN_CMD_SCAN_RESULTS)
					printf("-------------------------------\n");
				bcm_ether_ntoa((struct ether_addr *)data, buf);
				printf("> cluster_id: %s\n", buf);
				break;

			case WL_NAN_XTLV_ROLE:
				/*  nan device role , master, master-sync nosync etc  */
				uv32 = ltoh32_ua(data);
				print_nan_role(uv32);
				break;
			case WL_NAN_XTLV_BCN_INTERVAL:
				/* discovery beacon interval */
				uv16 = ltoh16_ua(data);
				printf("> bcn interval:%d\n", uv16);
				break;
			case WL_NAN_XTLV_DW_LEN:
				uv16 = ltoh16_ua(data);
				printf("> discovery window length:%d\n", uv16);
				break;
			case WL_NAN_XTLV_MAC_INITED:
				printf("> inited:%d\n", *data);
				break;
			case WL_NAN_XTLV_MAC_ENABLED:
				printf("> joined:%d\n", *data);
				break;
			case WL_NAN_XTLV_MAC_CHANSPEC:
				chanspec = ltoh16_ua(data);
				if (wf_chspec_valid(chanspec)) {
					wf_chspec_ntoa(chanspec, buf);
					printf("> chanspec:%s 0x%x\n", buf, chanspec);
				} else {
					printf("> chanspec: invalid  0x%x\n", chanspec);
				}
				break;
			case WL_NAN_XTLV_MAC_AMR:
				prhex("> amr", data, NAN_MASTER_RANK_LEN);
				break;
			case WL_NAN_XTLV_MAC_HOPCNT:
				printf("> hop_count:%d\n", *data);
				break;
			case WL_NAN_XTLV_WARM_UP_TIME:
				uv16 = ltoh16_ua(data);
				printf("> warm_up_time:%d\n", uv16);
				break;
			case WL_NAN_XTLV_MAC_AMBTT:
				uv32 = ltoh32_ua(data);
				printf("> ambtt:0x%x\n", uv32);
				break;
			case WL_NAN_XTLV_MAC_TXRATE:
			uv32 = ltoh32_ua(data);
			printf("> ratespec:0x%x\n", uv32);
			break;
			case WL_NAN_XTLV_INSTANCE_ID:
				printf("------------------\n");
				printf("> Instance ID:0x%02x\n", *data);
				break;
			case WL_NAN_XTLV_SVC_NAME:
				printf("> Service Name:%.*s\n", WL_NAN_SVC_HASH_LEN, data);
				break;
			case WL_NAN_XTLV_SVC_PARAMS:
				{
					wl_nan_disc_params_t *params = (wl_nan_disc_params_t *)data;

					printf("> flags:0x%04x |", dtoh32(params->flags));
					printf(" ttl:0x%08x |", dtoh32(params->ttl));
					printf(" period:%d\n", dtoh32(params->period));
				}
				break;
			case WL_NAN_XTLV_MAC_STATUS:
				{
					wl_nan_status_t *nstatus = (wl_nan_status_t *)data;

					printf("> inited:%d\n", nstatus->inited);
					printf("> joined:%d\n", nstatus->joined);
					print_nan_role(nstatus->role);

					nstatus->chspec = dtoh16(nstatus->chspec);
					if (wf_chspec_valid(nstatus->chspec)) {
					wf_chspec_ntoa(nstatus->chspec, buf);
						printf("> chanspec:%s 0x%x\n", buf,
							nstatus->chspec);
					} else {
						printf("> chanspec: invalid  0x%x\n",
							nstatus->chspec);
					}
					printf("> hop_count:%d\n", nstatus->hop_count);
					bcm_ether_ntoa(&nstatus->cid, buf);
					printf("> cluster_id: %s\n", buf);
					prhex("> amr", nstatus->amr, NAN_MASTER_RANK_LEN);
					printf("> cnt_pend_txfrm:%d\n",
					       dtoh32(nstatus->cnt_pend_txfrm));
					printf("> cnt_bcn_tx:%d\n",
					       dtoh32(nstatus->cnt_bcn_tx));
					printf("> cnt_bcn_rx:%d\n",
					       dtoh32(nstatus->cnt_bcn_rx));
					printf("> cnt_svc_disc_tx:%d\n",
					       dtoh32(nstatus->cnt_svc_disc_tx));
					printf("> cnt_svc_disc_rx:%d\n",
					       dtoh32(nstatus->cnt_svc_disc_rx));
				}
				break;
			case WL_NAN_XTLV_MAC_COUNT:
				{
					wl_nan_count_t *ncount = (wl_nan_count_t *)data;
					printf("NAN Counter:");
					printf(" nan_bcn_tx %d", dtoh32(ncount->cnt_bcn_tx));
					printf(" nan_bcn_rx %d", dtoh32(ncount->cnt_bcn_rx));
					printf(" nan_svc_disc_tx %d",
						dtoh32(ncount->cnt_svc_disc_tx));
					printf(" nan_svc_disc_rx %d\n",
						dtoh32(ncount->cnt_svc_disc_rx));
				}
				break;
			case WL_NAN_XTLV_NAN_SCANPARAMS: {
					nan_scan_params_t *scan_params = (nan_scan_params_t *)data;
					int i, chnum;
					char *chlist;

					chnum = dtoh16(scan_params->chspec_num);
					if (chnum > NAN_SCAN_MAX_CHCNT)
						break;

					chlist = buf;
					for (i = 0; i < chnum; i++) {
						int strsz;
						wf_chspec_ntoa(scan_params->chspec_list[i], chlist);
						strsz = strlen(chlist);
						chlist += strsz;
						*chlist++ = ',';
					}
					*--chlist = 0;
					printf("scan params:\nscan_time:%d \nhome_time:%d"
						" \nmerge_scan_interval:%d \nmerge_scan_duration:%d"
						"\nchcnt:%d \nchlist:%s\n",
						dtoh16(scan_params->scan_time),
						dtoh16(scan_params->home_time),
						dtoh16(scan_params->ms_intvl),
						dtoh16(scan_params->ms_dur),
						chnum, buf);
				}
				break;

			case WL_NAN_XTLV_DEBUGPARAMS:
			{
				nan_debug_params_t *params = (nan_debug_params_t *)data;

				printf("> debug params:\n  enabled:%d\n  collect:%d\n"
					"  msglevel:0x%x\n  last cmd:%x\n  status:%d\n",
					params->enabled, params->collect, params->msglevel,
					dtoh16(params->cmd), dtoh16(params->status));

			}
				break;
			case WL_NAN_XTLV_EVENT_MASK:
				uv32 = ltoh32_ua(data);
				uv32 = dtoh32(uv32);
				printf("> event_mask: %x\n  %s:%d\n  %s:%d\n  %s:%d\n"
					"  %s:%d\n  %s:%d\n  %s:%d\n  %s:%d\n  %s:%d\n  %s:%d\n"
					"  %s:%d\n  %s:%d\n",
					uv32,
					"WL_NAN_EVENT_START",
					IS_NAN_EVT_ON(uv32, WL_NAN_EVENT_START),
					"WL_NAN_EVENT_JOIN",
					IS_NAN_EVT_ON(uv32, WL_NAN_EVENT_JOIN),
					"WL_NAN_EVENT_ROLE",
					IS_NAN_EVT_ON(uv32, WL_NAN_EVENT_ROLE),
					"WL_NAN_EVENT_SCAN_COMPLETE",
					IS_NAN_EVT_ON(uv32, WL_NAN_EVENT_SCAN_COMPLETE),
					"WL_NAN_EVENT_DISCOVERY_RESULT",
					IS_NAN_EVT_ON(uv32, WL_NAN_EVENT_DISCOVERY_RESULT),
					"WL_NAN_EVENT_REPLIED",
					IS_NAN_EVT_ON(uv32, WL_NAN_EVENT_REPLIED),
					"WL_NAN_EVENT_TERMINATED",
					IS_NAN_EVT_ON(uv32, WL_NAN_EVENT_TERMINATED),
					"WL_NAN_EVENT_RECEIVE",
					IS_NAN_EVT_ON(uv32, WL_NAN_EVENT_RECEIVE),
					"WL_NAN_EVENT_STATUS_CHG",
					IS_NAN_EVT_ON(uv32, WL_NAN_EVENT_STATUS_CHG),
					"WL_NAN_EVENT_MERGE",
					IS_NAN_EVT_ON(uv32, WL_NAN_EVENT_MERGE),
					"WL_NAN_EVENT_STOP",
					IS_NAN_EVT_ON(uv32, WL_NAN_EVENT_STOP));
			break;
			case WL_NAN_XTLV_DW_INTERVAL:
				/*  TODO: add dw_interval in wlu.c & nan_mac */
				/* res |= bcm_unpack_xtlv_entry(&pxtlv,
					NAN_TLV_DW_INTERVAL, sizeof(uint16), &nmi->dw_interval);
					if (res != BCME_OK) goto fail;
				*/
				break;
			case WL_NAN_XTLV_PTBTT_OVERRIDE:
				uv16 = ltoh16_ua(data);
				printf("> pre_tbtt_override:%d\n", uv16);
				break;
			case WL_NAN_XTLV_PM_OPTION:
				uv32 = ltoh32_ua(data);
				uv32 = dtoh32(uv32);
				printf("> pm_option:0x%x\n", uv32);
				break;
			case WL_NAN_XTLV_IDLE_DW_TIMEOUT:
				uv16 = ltoh16_ua(data);
				printf("> idle_dw_timeout:%d\n", uv16);
				break;
			case WL_NAN_XTLV_IDLE_DW_LEN:
				uv16 = ltoh16_ua(data);
				printf("> idle_dw_len:%d\n", uv16);
				break;
			case WL_NAN_XTLV_PUBLR_ID:
				printf("------------------\n");
				printf("> Publish ID: %d\n", *data);
				break;
			case WL_NAN_XTLV_SUBSCR_ID:
				printf("------------------\n");
				printf("> Subscriber ID: %d\n", *data);
				break;
			case WL_NAN_XTLV_MAC_ADDR:
				bcm_ether_ntoa((struct ether_addr *)data, buf);
				if (cmd == WL_NAN_CMD_ATTR)
					printf("> if_addr:    %s\n", buf);
				else
				printf("> macaddr:    %s\n", buf);
				break;
			case WL_NAN_XTLV_SVC_INFO:
				if (len > 0)
					prhex("> Service Info", data, len);
				break;
			case WL_NAN_XTLV_VNDR:
				if (len > 0)
					print_vendor_info(data, len);
				break;
			case WL_NAN_XTLV_REASON:
				uv32 = ltoh32_ua(data);
				uv32 = dtoh32(uv32);
				printf("Reason %d\n", uv32);
				break;
			case WL_NAN_XTLV_TSRESERVE:
				if (len > 0)
					print_nan_tsinfo(data, len);
				break;
			case WL_NAN_XTLV_OUI:
				uv32 = ltoh32_ua(data);
				uv32 = dtoh32(uv32);
				printf("OUI %02X-%02X-%02X type %02X\n", uv32 & 0xff,
					(uv32 >> 8) & 0xff, (uv32 >> 16) & 0xff,
					(uv32 >> 24) & 0xff);
				break;
			case WL_NAN_XTLV_ZERO:
				/* don't parse empty space in the buffer  */
				res = BCME_ERROR;
				break;
			case WL_NAN_XTLV_SVC_DISC_TXTIME:
				uv16 = ltoh16_ua(data);
				printf("> sdf_txtime:%d us\n", uv16);
				break;
			case WL_NAN_XTLV_OPERATING_BAND:
				get_nan_band(str, data[0]);
				printf("> band:%s \n", str);
				break;
			case WL_NAN_XTLV_STOP_BCN_TX:
				if (data[0] == NAN_BAND_AUTO)
					strcpy(str, "none");
				else
					get_nan_band(str, data[0]);
				printf("> stop_bcn_tx:%s \n", str);
				break;
			default:
				res = BCME_ERROR;
				break;
		}

	return res;
}

/*
* listens on bcm events on given interface and prints NAN_EVENT data
* event data is a var size array of bcm_xtlv_t items
*/
static int
wl_nan_subcmd_event_check(void *wl, const wl_nan_sub_cmd_t  *cmd, char **argv)
{
	int fd, err, octets;
	struct sockaddr_ll sll;
	struct ifreq ifr;
	char ifnames[IFNAMSIZ] = {"eth0"};
	bcm_event_t *event;
	uint32 nan_evtnum;
	char* data;
	int event_type;
	void *nandata;

	uint8 event_inds_mask[WL_EVENTING_MASK_LEN];	/* 128-bit mask */
	UNUSED_PARAMETER(nandata);
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	if (argv[0] == NULL) {
		printf("<ifname> param is missing\n");
		return -1;
	}
	strncpy(ifnames, *argv, (IFNAMSIZ - 1));
	printf("ifname:%s\n", ifnames);
	bzero(&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, ifnames, (IFNAMSIZ - 1));
	if ((err = wlu_iovar_get(wl, "event_msgs", &event_inds_mask, WL_EVENTING_MASK_LEN))) {
		printf("couldn't read event_msgs\n");
		return (err);
	}
	event_inds_mask[WLC_E_NAN / 8] |= 1 << (WLC_E_NAN % 8);
	if ((err = wlu_iovar_set(wl, "event_msgs", &event_inds_mask, WL_EVENTING_MASK_LEN)))
		return (err);
	fd = socket(PF_PACKET, SOCK_RAW, hton16(ETHER_TYPE_BRCM));
	if (fd < 0) {
		printf("Cannot create socket %d\n", fd);
		return -1;
	}
	err = ioctl(fd, SIOCGIFINDEX, &ifr);
	if (err < 0) {
		printf("Cannot get iface:%s index \n", ifr.ifr_name);
		goto exit1;
	}
	bzero(&sll, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_protocol = hton16(ETHER_TYPE_BRCM);
	sll.sll_ifindex = ifr.ifr_ifindex;
	err = bind(fd, (struct sockaddr *)&sll, sizeof(sll));
	if (err < 0) {
		printf("Cannot bind %d\n", err);
		goto exit1;
	}
	data = (char*)malloc(NAN_EVENT_BUFFER_SIZE);
	if (data == NULL) {
		printf("Cannot not allocate %d bytes for events receive buffer\n",
			NAN_EVENT_BUFFER_SIZE);
		goto exit1;
	}
	printf("wating for NAN events on iface:%s\n", ifr.ifr_name);
	while (1) {
		uint16 datalen;
		fflush(stdout);
		octets = recv(fd, data, NAN_EVENT_BUFFER_SIZE, 0);
		if (octets <= 0)  {
			err = -1;
			printf(".");
			continue;
		}
		event = (bcm_event_t *)data;
		event_type = ntoh32(event->event.event_type);
		nan_evtnum = ntoh32(event->event.reason);
		datalen = ntoh32(event->event.datalen);
		if ((event_type != WLC_E_NAN)) {
				continue;
		}

		err = BCME_OK;
		/*  set ptr to event data body */
		nandata = &data[sizeof(bcm_event_t)];

		/* printf("nan event_num:%d len :%d\n", nan_evtnum, datalen); */
		switch (nan_evtnum) {
		case WL_NAN_EVENT_START:
			printf("WL_NAN_EVENT_START:\n");
			break;
		case WL_NAN_EVENT_JOIN:
			printf("WL_NAN_EVENT_JOIN:\n");
			break;
		case WL_NAN_EVENT_ROLE:
			printf("WL_NAN_EVENT_ROLE:\n");
			break;
		case WL_NAN_EVENT_SCAN_COMPLETE:
			printf("WL_NAN_EVENT_SCAN_COMPLETE:\n");
		break;
		case WL_NAN_EVENT_DISCOVERY_RESULT:
			printf("WL_NAN_EVENT_DISCOVERY_RESULT:\n");
			break;
		case WL_NAN_EVENT_REPLIED:
			printf("WL_NAN_EVENT_REPLIED:\n");
			break;
		case WL_NAN_EVENT_TERMINATED:
			printf("WL_NAN_EVENT_TERMINATED:\n");
			break;
		case WL_NAN_EVENT_RECEIVE:
			printf("WL_NAN_EVENT_RECEIVE:\n");
			break;
		case WL_NAN_EVENT_STATUS_CHG:
			printf("WL_NAN_EVENT_STATUS_CHG:\n");
		break;
		case WL_NAN_EVENT_MERGE:
			printf("WL_NAN_EVENT_MERGE:\n");
			break;
		case WL_NAN_EVENT_STOP:
			printf("WL_NAN_EVENT_STOP:\n");
			break;
		default:
			printf("WARNING: unimplemented NAN APP EVENT code:%d\n",
				nan_evtnum);
			err = -1;
			break;
		}
		/* the event content is printed by the same cbfn that used for get ioctls */
		if (err == BCME_OK)
			err = bcm_unpack_xtlv_buf(&nan_evtnum, nandata, datalen,
				BCM_XTLV_OPTION_ALIGN32, wlu_nan_set_vars_cbfn);
	}
	free(data);
exit1:
	close(fd);
	if (!(err = wlu_iovar_get(wl, "event_msgs", &event_inds_mask, WL_EVENTING_MASK_LEN))) {
		event_inds_mask[WLC_E_NAN / 8] &= (~(1 << (WLC_E_NAN % 8)));
		err = wlu_iovar_set(wl, "event_msgs", &event_inds_mask, WL_EVENTING_MASK_LEN);
	}
	fflush(stdout);
	return (err);
}

/*
 *   --- common for all nan get commands ----
 */
int wl_nan_do_get_ioctl(void *wl, wl_nan_ioc_t *nanioc, uint16 iocsz)
{
	/* for gets we only need to pass ioc header */
	wl_nan_ioc_t *iocresp = NULL;
	int res;
	/*  send getbuf nan iovar */
	res = wlu_var_getbuf(wl, "nan", nanioc, iocsz, (void *)&iocresp);

/*  check the response buff  */
	if ((res == BCME_OK) && (iocresp != NULL)) {
		/* scans ioctl tlvbuf f& invokes the cbfn for processing  */
		/* prhex("iocresp buf:", iocresp->data, iocresp->len); */
		res = bcm_unpack_xtlv_buf(nanioc, iocresp->data, iocresp->len,
			BCM_XTLV_OPTION_ALIGN32, wlu_nan_set_vars_cbfn);
	}

	return res;
}


/* nan event_msgs:  enable/disable nan events  */
static int
wl_nan_subcmd_event_msgs(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	int res = BCME_BADARG;
	wl_nan_ioc_t *nanioc;
	uint16 iocsz;

	/* bit num = event enum -1 */
	const char usage[] = "nan event_msgs [bit] [0/1]\n"
		"	bit 0 - WL_NAN_EVENT_START\n"
		"	bit 1 - WL_NAN_EVENT_JOIN\n"
		"	bit 2 - WL_NAN_EVENT_ROLE\n"
		"	bit 3 - WL_NAN_EVENT_SCAN_COMPLETE\n"
		"	bit 4 - WL_NAN_EVENT_DISCOVERY_RESULT\n"
		"	bit 5 - WL_NAN_EVENT_REPLIED\n"
		"	bit 6 - WL_NAN_EVENT_TERMINATED\n"
		"	bit 7 - WL_NAN_EVENT_RECEIVE\n"
		"	bit 8 - WL_NAN_EVENT_STATUS_CHG\n"
		"	bit 10 ..30 - unused\n"
		"   bit 31 -  set/clr all eventmask bits\n";

/*  * events defined in bcmevent.h
	WL_NAN_EVENT_START = 1,
	WL_NAN_EVENT_JOIN = 2,
	WL_NAN_EVENT_ROLE = 3,
	WL_NAN_EVENT_SCAN_COMPLETE = 4,
	WL_NAN_EVENT_DISCOVERY_RESULT = 5,
	WL_NAN_EVENT_REPLIED = 6,
	WL_NAN_EVENT_TERMINATED = 7,
	WL_NAN_EVENT_RECEIVE = 8,
	WL_NAN_EVENT_STATUS_CHG = 9
*/

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, NAN_IOC_BUFSZ);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = cmd->id;
	nanioc->len = BCM_XTLV_HDR_SIZE + sizeof(uint32);
	iocsz = sizeof(wl_nan_ioc_t) + nanioc->len;

	if (*argv == NULL) {
	/* get: handled by cbfn */
		res = wl_nan_do_get_ioctl(wl, nanioc, iocsz);
	} else {
		/*  args are present, do set ioctl  */
		uint8 bit = 32, val = 3;
		uint8 *pxtlv = nanioc->data;
		uint16 buflen = nanioc->len;
		uint32 eventmask;

		bit = atoi(argv[0]);

		if ((argv[0][0] == '-')) {
			fprintf(stderr,
				"%s\n", usage);
			res = BCME_BADARG;
			goto exit;
		}

		if (bit > 31) {
			fprintf(stderr, "1st param should be [0..31]\n");
			fprintf(stderr, "%s\n", usage);
			res = BCME_BADARG;
			goto exit;
		}

		if (argv[1])
			val = atoi(argv[1]);

		if (val > 1) {
			fprintf(stderr, "2nd param should be [0|1]\n");
			fprintf(stderr, "%s\n", usage);
			res = BCME_BADARG;
			goto exit;
		}

		if (bit == 31)	{
				/*  set all bits */
				eventmask = 0x7fffffff;
		} else {
			/* set/reset one bit */
			eventmask = 1 << bit;
		}
		if (val == 0) {
			/* and mask to rst bit */
			eventmask = ~eventmask;
		}

		eventmask = htod32(eventmask);

		res = bcm_pack_xtlv_entry(&pxtlv,
			&buflen, WL_NAN_XTLV_EVENT_MASK,
			sizeof(uint32), &eventmask, BCM_XTLV_OPTION_ALIGN32);

		res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);
	}

exit:
	if (res != BCME_OK)
		printf("error:%d \n", res);
	if (nanioc)
		free(nanioc);
	return res;
}

/*
*  ********  various debug features, for internal use only ********
*/
static int
wl_nan_subcmd_debug(void *wl, const wl_nan_sub_cmd_t  *cmd, char **argv)
{
	int res = BCME_BADARG;
	wl_nan_ioc_t *nanioc;
	uint16 iocsz, i;
	nan_debug_params_t params;
	const char usage[] = "> nan debug -ml [bitnum] [0/1]\n"
		"ml [bitnums]: \n"
		"	bit 0 - on/off all trace messages, if==0 other levels won't print\n"
		"	bit 1 - nan_mac trace\n"
		"	bit 2 - nan_disc\n"
		"	bit 3 - nan_cmn\n"
		"	bit 4 - nan_iov\n"
		"	bit 5 - nan_app_svc, ibss, rtt etc\n"
		"	bit 6 - nan warnings\n"
		"	bit 7 - nan state transitions\n"
		"	bit 8 - nan_mac timing (tsf, etc)\n"
		"	bit 9 - nan events\n"
		"	bit 10..30 - unused\n"
		"	bit 31 - set/clr all [0..30] bits\n"
		"> nan debug collect [0/1]\n";

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, NAN_IOC_BUFSZ);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = cmd->id;
	nanioc->len = BCM_XTLV_HDR_SIZE + sizeof(nan_debug_params_t);
	iocsz = sizeof(wl_nan_ioc_t) + nanioc->len;

	/*	for now only two integer params: bit <0..31>
		and value < val - 0/1>  to set/reset in nan_msg_level
	*/

	if (*argv == NULL) {
	/* get: handled by cbfn */
		res = wl_nan_do_get_ioctl(wl, nanioc, iocsz);
	} else {
		/*  args are present, do set ioctl  */
		uint8 *pxtlv = nanioc->data;
		uint16 buflen = nanioc->len;

		uint8 bit = 0;  /* bit number in msg_level_mask to toggle */
		uint8 val = 0; /* 0 - reset bit , 1 set */


		for (i = 0; (argv[i] != NULL); ++i) {

			/*  ******  debug msg level *******  */
			if (!strcmp(argv[i], "-ml")) {

				/* set/rst one specified bit or all */
				if (!argv[++i]) {
					fprintf(stderr, "%s\n", usage);
					goto exit;
				}

				bit = atoi(argv[i]);
				if (bit > 31)  {
					fprintf(stderr,
						"%s\n", "1st param should be [0..31]\n");
					res = BCME_BADARG;
					goto exit;
				}
				if (!argv[++i]) {
					fprintf(stderr,
						"%s\n", usage);
					res = BCME_BADARG;
					goto exit;
				}
				val = atoi(argv[i]);

				if (val > 1)  {
					fprintf(stderr,
						"%s\n", "2nd param should be [0|1]\n");
					res = BCME_BADARG;
					goto exit;
				}

				if (bit == 31)	{
					/* 32 set or reset all bits in the mask */
					params.msglevel = 0x7fffffff;
				} else {
					/* set/reset one bit */
					params.msglevel = 1 << bit;
				}

				if (val == 0) {
					/* and mask to rst bit */
					params.msglevel = ~params.msglevel;
				}
				params.msglevel = htod32(params.msglevel);
				continue;
			} else
			/*   **** collect ******  */
			if (!strcmp(argv[i], "-cl")) {

				if (atoi(argv[++i]))
					params.collect = atoi(argv[i]);
					else goto exit;

				continue;
			} else	{
				fprintf(stderr, "%s\n", usage);
				goto exit;
			}
		}


		res = bcm_pack_xtlv_entry(&pxtlv,
			&buflen, WL_NAN_XTLV_DEBUGPARAMS,
			sizeof(nan_debug_params_t), &params, BCM_XTLV_OPTION_ALIGN32);

		res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);

	} /* do set ioctl */

exit:
	if (nanioc)
		free(nanioc);
	return res;
}

/*
*  ********  get NAN status info ********
*/
static int
wl_nan_subcmd_scan_params(void *wl, const wl_nan_sub_cmd_t  *cmd, char **argv)
{
	int res = BCME_OK;
	int opt_err;
	miniopt_t to;
	const char* fn_name = "scan_params";
	int nchan = 0;

	/* nan scan params inited to dflt */
	nan_scan_params_t scan_params = {
		.scan_time = 0,
		.home_time = 0,
		.ms_intvl = 0,
		.ms_dur = 0,
		.chspec_num = 0,
		.chspec_list = {0x1006, 0xd02c, 0xd095} /* ch 6/20, ch 44/20 , ch 149/20 */
	};

	wl_nan_ioc_t *nanioc;
	uint16 iocsz;

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, NAN_IOC_BUFSZ);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = cmd->id;
	nanioc->len = ALIGN_SIZE(BCM_XTLV_HDR_SIZE + sizeof(nan_scan_params_t), 4);
	iocsz = sizeof(wl_nan_ioc_t) + nanioc->len;

	if (*argv == NULL) {
	/* get: handled by cbfn */
		res = wl_nan_do_get_ioctl(wl, nanioc, iocsz);
	} else {
		/*  args are present, do set ioctl  */
		int ok_params_cnt = 0;
		int i;
		uint8 *pxtlv = nanioc->data;
		uint16 buflen = nanioc->len;

		miniopt_init(&to, fn_name, NULL, FALSE);

		while ((opt_err = miniopt(&to, argv)) != -1) {

			if (opt_err == 1) {
				res = BCME_USAGE_ERROR;
				goto exit;
			}
			argv += to.consumed;

			if (to.opt == 's') {
				if (!to.good_int) {
					fprintf(stderr,
					        "%s: option value %s\" is not an integer\n",
					        fn_name, to.valstr);
					res = BCME_BADARG;
					goto exit;
				}
				scan_params.scan_time = to.uval;
				ok_params_cnt++;
			}

			if (to.opt == 'h') {
				if (!to.good_int) {
					fprintf(stderr,
						"%s: option value %s\" is not an integer\n",
						fn_name, to.valstr);
					res = BCME_BADARG;
					goto exit;
				}

				scan_params.home_time = to.uval;
				ok_params_cnt++;
			}

			if (to.opt == 'i') {
				if (!to.good_int) {
					fprintf(stderr,
						"%s: option value %s\" is not an integer\n",
						fn_name, to.valstr);
					res = BCME_BADARG;
					goto exit;
				}
				scan_params.ms_intvl = to.uval;
				ok_params_cnt++;
			}

			if (to.opt == 'd') {
				if (!to.good_int) {
					fprintf(stderr,
						"%s: option value %s\" is not an integer\n",
						fn_name, to.valstr);
					res = BCME_BADARG;
					goto exit;
				}
				scan_params.ms_dur = to.uval;
				ok_params_cnt++;
			}

			if (to.opt == 'c') {
				nchan =  wl_parse_chanspec_list(to.valstr,
					scan_params.chspec_list, 8);

				if ((nchan == -1) | (nchan >= 8)) {
					fprintf(stderr, "error parsing channel list arg\n");
					res = BCME_BADARG;
					goto exit;
				}

				/* convert to dongle endianness */
				for (i = 0; i < nchan; i++) {
					scan_params.chspec_list[i] =
						htodchanspec(scan_params.chspec_list[i]);
				}
				scan_params.chspec_num = nchan;
				ok_params_cnt++;
			} /* if chan list */


		} /*  while options proc loop  */

		if (ok_params_cnt == 0) {
			res = BCME_USAGE_ERROR;
			goto exit;
		}

		/* convert to dongle indianness */
		scan_params.scan_time = htod16(scan_params.scan_time);
		scan_params.home_time = htod16(scan_params.home_time);
		scan_params.ms_intvl = htod16(scan_params.ms_intvl);
		scan_params.ms_dur = htod16(scan_params.ms_dur);

		res = bcm_pack_xtlv_entry(&pxtlv,
			&buflen, WL_NAN_XTLV_NAN_SCANPARAMS,
			sizeof(nan_scan_params_t), &scan_params, BCM_XTLV_OPTION_ALIGN32);

		res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);

	} /* do set ioctl */

exit:
	if (nanioc)
		free(nanioc);
	return res;
}

/*
* ********  nan scan for clusters  ********
*/
static int
wl_nan_subcmd_scan(void *wl, const wl_nan_sub_cmd_t  *cmd, char **argv)
{
	int res = BCME_USAGE_ERROR;
	wl_nan_ioc_t *nanioc;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;
	uint8 *pxtlv;
	uint16 buflen, buflen_at_start;
	struct ether_addr cid;


	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	nanioc->len = NAN_IOC_BUFSZ;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = cmd->id;
	nanioc->len = htod16(BCM_XTLV_HDR_SIZE + 1);
	pxtlv = nanioc->data;

	/*  nan scan is a SET only ioctl no params passes NULL ether addr */
	if (*argv) {
		if (!wl_ether_atoe(*argv, &cid))
			goto exit;
	} else {
		/* bcast mac: scan for all nan clusters */
		memcpy(&cid, &ether_null, ETHER_ADDR_LEN);
	}

	buflen = NAN_IOC_BUFSZ;
	buflen_at_start = buflen;
	/* we'll adjust final ioc size at the end */
	res = bcm_pack_xtlv_entry(&pxtlv,
		&buflen, WL_NAN_XTLV_CLUSTER_ID, ETHER_ADDR_LEN, &cid, BCM_XTLV_OPTION_ALIGN32);
	if (res != BCME_OK)
		goto exit;

	nanioc->len = (buflen_at_start - buflen);
	iocsz = sizeof(wl_nan_ioc_t) + nanioc->len;
	res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);

exit:
	if (nanioc)
		free(nanioc);
	return res;
}

/*  ********  enable/disable feature  ************** */
static int
wl_nan_subcmd_enable(void *wl, const wl_nan_sub_cmd_t  *cmd, char **argv)
{

	int res = BCME_OK;
	wl_nan_ioc_t *nanioc;
	uint16 buflen;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;
	uint8 *pxtlv;


	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	nanioc->len = NAN_IOC_BUFSZ;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = cmd->id;
	nanioc->len = htod16(iocsz);
	pxtlv = nanioc->data;


	if (*argv == NULL) { /* get  */

		iocsz = sizeof(wl_nan_ioc_t) + sizeof(bcm_xtlv_t);
		wl_nan_do_get_ioctl(wl, nanioc, iocsz);

	} else {	/* set */
		/* max tlv data we can write, it will be decremented as we pack */
		uint8 val =  atoi(*argv);
		printf("nan %s :%d\n", cmd->name, atoi(*argv));

		buflen = NAN_IOC_BUFSZ;
		/* save buflen at start  */
		uint16 buflen_at_start = buflen;
		/* we'll adjust final ioc size at the end */

		res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_ENABLED,
			sizeof(uint8), &val, BCM_XTLV_OPTION_ALIGN32);
		if (res != BCME_OK) goto fail;

		/* adjust iocsz to the end of last data record */
		iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);
		nanioc->len = (buflen_at_start - buflen);

		res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);
	}

	free(nanioc);
	return res;

fail:
	printf("error:%d \n", res);
	free(nanioc);
	return res;
}

/*
*  ********  get NAN status info ********
*/
static int
wl_nan_subcmd_status(void *wl, const wl_nan_sub_cmd_t  *cmd, char **argv)
{

	int res = BCME_OK;
	wl_nan_ioc_t*nanioc;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = cmd->id;
	nanioc->len = NAN_IOC_BUFSZ;

	if (*argv == NULL) { /* get  */
		wl_nan_do_get_ioctl(wl, nanioc, iocsz);
	} else {
		printf("status cmd doesn't accept any parameters\n");
		res = BCME_ERROR;
	}

	if (res != BCME_OK)
		printf("error:%d \n", res);
	if (nanioc)
		free(nanioc);
	return res;
}

/*
*  ********  get NAN status info ********
*/
static int
wl_nan_subcmd_stop(void *wl, const wl_nan_sub_cmd_t  *cmd, char **argv)
{
	int res = BCME_USAGE_ERROR;
	uint16 buflen, buflen_at_start;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;
	uint8 *pxtlv;
	wl_nan_ioc_t *nanioc = NULL;
	struct ether_addr cid = ether_null;

	/* cmd takes only one optional argument: cid */

	if ((*argv) && !wl_ether_atoe(argv[0], &cid)) {
		printf("%s : Malformed MAC address parameter\n", __FUNCTION__);
		goto exit;
	}

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = htod16(cmd->id);   /* override with cmd attr */
	nanioc->len = htod16(NAN_IOC_BUFSZ); /*  */
	pxtlv = nanioc->data;

	/* max data we can write, it will be decremented as we pack */
	buflen = NAN_IOC_BUFSZ;
	buflen_at_start = buflen;

	res = bcm_pack_xtlv_entry(&pxtlv,
		&buflen, WL_NAN_XTLV_CLUSTER_ID, ETHER_ADDR_LEN, &cid, BCM_XTLV_OPTION_ALIGN32);
	if (res != BCME_OK) goto exit;

	/* adjust iocsz to the end of last data record */
	iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);
	nanioc->len = (buflen_at_start - buflen);

	res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);

exit:
	if (nanioc)
		free(nanioc);
	return res;
}


/*
* ***  nan set single attribute  ****
* syntax: "wl nan <attr_name> [value]
* e.g: "wl nan master 0x2 0x23 (pref & randomfactor)
*      "wl nan role 2
*/
static int
wl_nan_subcmd_set_attr(void *wl, const wl_nan_sub_cmd_t  *cmd, char **argv)
{
	int res = BCME_OK;
	wl_nan_ioc_t *nanioc;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;
	uint8 *pxtlv;

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	nanioc->len = NAN_IOC_BUFSZ;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = htod16(WL_NAN_CMD_ATTR);   /* override with cmd attr */
	nanioc->len = htod16(NAN_IOC_BUFSZ); /*  */
	pxtlv = nanioc->data;

	if (*argv == NULL) {
	/* get: handled by cbfn */
		res = wl_nan_do_get_ioctl(wl, nanioc, iocsz);
	} else {
	/* got args, do set  */
		uint16 buflen, buflen_at_start;
		uint16 rnd_pref;
		uint8 master_preference = 1;
		uint8 random_factor = 0;
		struct ether_addr mac = ether_null;
		uint32 role = 0;
		uint16 disc_bcn_interval = 100;
		uint16 dw_len = 16;
		uint8 hop_count = 0;
		uint32 txrate = 6;
		chanspec_t chspec_b = 6;
		chanspec_t chspec_a = 44;
		uint16 warm_up_time = 230;
		uint16 pre_tbtt_override = 10000;
		uint32 pm_option = 0;
		uint16 idle_dw_timeout = 6;
		uint16 idle_dw_len = 4;
		uint16 sdf_txtime = 0;
		uint8 stop_bcn_tx = 0;
		uint8 nan_band = 0;

		/* save buflen at start  */
		buflen = NAN_IOC_BUFSZ;
		buflen_at_start = buflen;

		switch (cmd->type) {

		case WL_NAN_XTLV_MASTER_PREF:
			/*  two args required here */
			if (argv[0])
				master_preference = (uint8)(strtoul(argv[0], NULL, 0));
			if (argv[1])
				random_factor =  (uint8)(strtoul(argv[1], NULL, 0));
			rnd_pref = htod16(master_preference | (random_factor << 8));

			res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_MASTER_PREF,
				sizeof(uint16), &rnd_pref, BCM_XTLV_OPTION_ALIGN32);
			break;
		case WL_NAN_XTLV_CLUSTER_ID:
			if (!wl_ether_atoe(argv[0], &mac)) {
				printf("Malformed MAC address parameter\n");
				res = BCME_USAGE_ERROR;
				goto exit;
			}
			res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_CLUSTER_ID,
				ETHER_ADDR_LEN, &mac, BCM_XTLV_OPTION_ALIGN32);
			break;
		case WL_NAN_XTLV_MAC_ADDR:
			if (!wl_ether_atoe(argv[0], &mac)) {
				printf("Malformed MAC address parameter\n");
				res = BCME_USAGE_ERROR;
				goto exit;
			}
			res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_MAC_ADDR,
				ETHER_ADDR_LEN, &mac, BCM_XTLV_OPTION_ALIGN32);
			break;
		case WL_NAN_XTLV_ROLE:
			role = htod32(atoi(argv[0]));
			res = bcm_pack_xtlv_entry(&pxtlv,
				&buflen, WL_NAN_XTLV_ROLE, 4, &role, BCM_XTLV_OPTION_ALIGN32);
			break;
		case WL_NAN_XTLV_BCN_INTERVAL:
			disc_bcn_interval = htod16(atoi(argv[0]));
			res = bcm_pack_xtlv_entry(&pxtlv,
				&buflen, WL_NAN_XTLV_BCN_INTERVAL,
				sizeof(uint16), &disc_bcn_interval, BCM_XTLV_OPTION_ALIGN32);
			break;
		case WL_NAN_XTLV_MAC_CHANSPEC:
			/* reqires at least 1 arg  */
			if (argv[0]) {
				chspec_b = wf_chspec_aton(argv[0]);
				if (chspec_b != 0) {
					res = bcm_pack_xtlv_entry(&pxtlv, &buflen,
						WL_NAN_XTLV_MAC_CHANSPEC, sizeof(chanspec_t),
						&chspec_b, BCM_XTLV_OPTION_ALIGN32);
				}
			}
			if (argv[1]) { /* rfu  */
				chspec_a = wf_chspec_aton(argv[1]);
				if (chspec_a != 0) {
					res = bcm_pack_xtlv_entry(&pxtlv, &buflen,
						WL_NAN_XTLV_MAC_CHANSPEC, sizeof(chanspec_t),
						&chspec_a, BCM_XTLV_OPTION_ALIGN32);
				}
			}
			break;

		case WL_NAN_XTLV_MAC_TXRATE:
			txrate = htod32(atoi(argv[0]));
			res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_MAC_TXRATE,
				sizeof(uint32), &txrate, BCM_XTLV_OPTION_ALIGN32);
			break;
		case WL_NAN_XTLV_DW_LEN:
			dw_len = htod16(atoi(argv[0]));
			res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_DW_LEN,
				sizeof(uint16), &dw_len, BCM_XTLV_OPTION_ALIGN32);
				break;
		case WL_NAN_XTLV_MAC_HOPCNT:
			hop_count = (uint8)(strtoul(argv[0], NULL, 0));
			res = bcm_pack_xtlv_entry(&pxtlv,
				&buflen, WL_NAN_XTLV_MAC_HOPCNT, sizeof(uint8),
				&hop_count, BCM_XTLV_OPTION_ALIGN32);
				break;
		case WL_NAN_XTLV_WARM_UP_TIME:
			warm_up_time = (uint16)atoi(argv[0]);
			res = bcm_pack_xtlv_entry(&pxtlv,
				&buflen, WL_NAN_XTLV_WARM_UP_TIME, sizeof(uint16),
				&warm_up_time, BCM_XTLV_OPTION_ALIGN32);
				break;
		case WL_NAN_XTLV_PTBTT_OVERRIDE:
			pre_tbtt_override = (uint16)atoi(argv[0]);
			res = bcm_pack_xtlv_entry(&pxtlv,
				&buflen, WL_NAN_XTLV_PTBTT_OVERRIDE, sizeof(uint16),
				&pre_tbtt_override, BCM_XTLV_OPTION_ALIGN32);
				break;
		case WL_NAN_XTLV_PM_OPTION:
			pm_option = htod32(strtoul(argv[0], NULL, 0));
			res = bcm_pack_xtlv_entry(&pxtlv,
				&buflen, WL_NAN_XTLV_PM_OPTION, sizeof(uint32),
				&pm_option, BCM_XTLV_OPTION_ALIGN32);
				break;
		case WL_NAN_XTLV_IDLE_DW_TIMEOUT:
			idle_dw_timeout = (uint16)atoi(argv[0]);
			res = bcm_pack_xtlv_entry(&pxtlv,
				&buflen, WL_NAN_XTLV_IDLE_DW_TIMEOUT, sizeof(uint16),
				&idle_dw_timeout, BCM_XTLV_OPTION_ALIGN32);
				break;
		case WL_NAN_XTLV_IDLE_DW_LEN:
			idle_dw_len = (uint16)atoi(argv[0]);
			res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_IDLE_DW_LEN,
				sizeof(uint16), &idle_dw_len, BCM_XTLV_OPTION_ALIGN32);
				break;
		case WL_NAN_XTLV_SVC_DISC_TXTIME:
			sdf_txtime = (uint16)atoi(argv[0]);
			/* valid range is 3ms to 4.5ms */
			if (sdf_txtime < 3000 || sdf_txtime > 4500) {
				res = BCME_USAGE_ERROR;
				goto exit;
			}
			res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_SVC_DISC_TXTIME,
				sizeof(uint16), &sdf_txtime, BCM_XTLV_OPTION_ALIGN32);
				break;
		case WL_NAN_XTLV_OPERATING_BAND:
			nan_band = set_nan_band(argv[0]);
			if (nan_band == NAN_BAND_INVALID) {
				res = BCME_USAGE_ERROR;
				goto exit;
			}
			res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_OPERATING_BAND,
				sizeof(uint8), &nan_band, BCM_XTLV_OPTION_ALIGN32);
			break;
		case WL_NAN_XTLV_STOP_BCN_TX:
			stop_bcn_tx = set_nan_band(argv[0]);
			if (stop_bcn_tx == NAN_BAND_INVALID) {
				res = BCME_USAGE_ERROR;
				goto exit;
			}
			res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_STOP_BCN_TX,
				sizeof(uint8), &stop_bcn_tx, BCM_XTLV_OPTION_ALIGN32);
			break;
		default:
			res = BCME_BADARG;
			break;
		}

		if (res == BCME_OK) {
			/* adjust iocsz to the end of last data record */
			iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);

			nanioc->len = (buflen_at_start - buflen);

			res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);
		} else {
			printf("xtlv pack error\n");
		}

	}
exit:
	if (nanioc)
		free(nanioc);
	return res;
}

/*  nan join cluter or start a new one  */
static int
wl_nan_subcmd_join(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	int res = BCME_USAGE_ERROR;
	uint16 buflen, buflen_at_start;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;
	uint8 *pxtlv;
	uint tot_len = 0;
	wl_nan_ioc_t*nanioc = NULL;
	struct ether_addr cid = ether_null;
	bool start = FALSE;

	printf("ioc:= handler:%s, cmdname:%s cmd:%d, type:%d\n",
		__FUNCTION__, cmd->name, cmd->id, cmd->type);

	while (*argv) {
		char *s = *argv;
		if (!stricmp(s, "-start")) {
			start = TRUE;
			printf("start=true\n");
			tot_len += (BCM_XTLV_HDR_SIZE + 1);
		} else {
			if (!wl_ether_atoe(argv[0], &cid)) {
				printf("%s : Malformed MAC address parameter\n", __FUNCTION__);
				goto exit;
			}
			tot_len += (BCM_XTLV_HDR_SIZE + ETHER_ADDR_LEN);
		}
		argv++;
	}
	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = htod16(cmd->id);   /* override with cmd attr */
	nanioc->len = htod16(NAN_IOC_BUFSZ); /*  */
	pxtlv = nanioc->data;


	/* max data we can write, it will be decremented as we pack */
	buflen = NAN_IOC_BUFSZ;
	buflen_at_start = buflen;

	res = bcm_pack_xtlv_entry(&pxtlv,
		&buflen, WL_NAN_XTLV_CLUSTER_ID, ETHER_ADDR_LEN, &cid, BCM_XTLV_OPTION_ALIGN32);
	if (res != BCME_OK) goto exit;

	res = bcm_pack_xtlv_entry(&pxtlv,
		&buflen, WL_NAN_XTLV_START, 1, &start, BCM_XTLV_OPTION_ALIGN32);
	if (res != BCME_OK) goto exit;

	/* adjust iocsz to the end of last data record */
	iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);
	nanioc->len = (buflen_at_start - buflen);

	res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);

exit:
	if (nanioc)
		free(nanioc);
	return res;
}


static int
wl_nan_subcmd_leave(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	int res = BCME_USAGE_ERROR;
	uint16 buflen, buflen_at_start;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;
	uint8 *pxtlv;
	wl_nan_ioc_t*nanioc = NULL;
	struct ether_addr cid = ether_null;


	printf("ioc:= handler:%s, cmdname:%s cmd:%d, type:%d\n",
		__FUNCTION__, cmd->name, cmd->id, cmd->type);

	if (*argv) {
		if (!wl_ether_atoe(*argv, &cid)) {
			printf("%s : Malformed MAC address parameter\n", __FUNCTION__);
			return BCME_USAGE_ERROR;
		}
	}


	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = htod16(cmd->id);
	nanioc->len = htod16(NAN_IOC_BUFSZ);
	pxtlv = nanioc->data;

	/* max data we can write, it will be decremented as we pack */
	buflen = NAN_IOC_BUFSZ;
	buflen_at_start = buflen;

	if (memcmp(&cid, &ether_null, sizeof(struct ether_addr)))
	{
		res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_CLUSTER_ID,
			ETHER_ADDR_LEN, &cid, BCM_XTLV_OPTION_ALIGN32);
		if (res != BCME_OK) goto exit;
	}
	/* adjust iocsz to the end of last data record */
	iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);
	nanioc->len = (buflen_at_start - buflen);

	res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);

exit:
	if (nanioc)
		free(nanioc);
	return res;
}

static int
wl_nan_subcmd_svc(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv, uint32 default_flags)
{
	int ret = BCME_OK;
	int int_val = 0;
	wl_nan_disc_params_t params;
	uint iocsz = WLC_IOCTL_MAXLEN;
	wl_nan_ioc_t *nanioc = NULL;
	char *param, *val_p, *endptr, *svc_info = NULL;
	char *match = NULL, *match_rx = NULL;
	uint32 val = 0;
	uint8 *pxtlv;
	uint16 buflen, buflen_at_start;

	/* SRF variables */
	uint8 *srf = NULL, *srftmp = NULL, *mac_list = NULL;
	bool srfraw_started = FALSE;
	bool srf_started = FALSE;
	/* Bloom length default, indicates use mac filter not bloom */
	uint bloom_len = 0;
	struct ether_addr srf_mac;
	bcm_bloom_filter_t *bp = NULL;
	/* Bloom filter index default, indicates it has not been set */
	uint bloom_idx = 0xFFFFFFFF;
	uint mac_num = 0;
	char *srf_raw = NULL;
	char srf_include = '\0';

	/* Default options */
	params.period = 1;
	params.ttl = WL_NAN_TTL_UNTIL_CANCEL;
	params.flags = default_flags;

	/* Copy mandatory parameters from command line. */
	if (!*argv) {
		printf("%s : Missing instance id parameter.\n", __FUNCTION__);
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	int_val = atoi(*argv++);
	if (int_val < 0 || int_val > 255) {
		printf("%s : Invalid instance_id.\n", __FUNCTION__);
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	params.instance_id = int_val;

	if (!*argv) {
		printf("%s : Missing service name parameter.\n", __FUNCTION__);
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	/* TODO: Use real hash function instead of copying 6 characters */
	strncpy((char*)params.svc_hash, *argv++, WL_NAN_SVC_HASH_LEN);

	/* Parse optional args. Validity is checked by discovery engine */
	while ((param = *argv++) != NULL) {
		if (stricmp(param, "-bloom") == 0) {
			if (srfraw_started) {
				printf("%s: Cannot use -bloom-idx with -srfraw\n",
					__FUNCTION__);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			srf_started = TRUE;

			if (bloom_len) {
				printf("%s: -bloom can only be set once\n", __FUNCTION__);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			/* Value is optional */
			if (!*argv || **argv == '-') {
				bloom_len = NAN_BLOOM_LENGTH_DEFAULT;
				continue;
			}
			val = strtoul(*argv++, &endptr, 0);

			if (*endptr != '\0' || val < 1 || val > 254) {
				printf("%s: Invalid bloom length.\n", __FUNCTION__);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			bloom_len = val;
			param = *argv++;
		}

		val_p = *argv++;
		if (!*val_p || *val_p == '-') {
			printf("%s: Need value following %s\n", __FUNCTION__, param);
		}

		if ((stricmp(param, "-i") == 0) || (stricmp(param, "-x") == 0)) {
			if (srfraw_started) {
				printf("%s: Cannot use -i or -x with -srfraw\n", __FUNCTION__);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			srf_started = TRUE;

			if (!srf_include) {
				/* i or x */
				srf_include = param[1];
			} else if (srf_include != param[1]) {
				printf("%s: Cannot use -i or -x together\n", __FUNCTION__);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			/* Allocate and initalize SRF buffer */
			if (!srftmp) {
				if ((mac_list = malloc(WLC_IOCTL_MEDLEN)) == NULL) {
					printf("Failed to malloc %d bytes \n",
						WLC_IOCTL_MEDLEN);
					ret = BCME_NOMEM;
					goto exit;
				}
				srftmp = mac_list;
			}
			/* Add MAC address to temporary buffer */
			if (!wl_ether_atoe(val_p, &srf_mac)) {
				printf("%s: Invalid ether addr\n",
					__FUNCTION__);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			memcpy(srftmp, &srf_mac, ETHER_ADDR_LEN);
			srftmp += ETHER_ADDR_LEN;
			mac_num++;
		}
		else if (stricmp(param, "-bloom-idx") == 0) {
			if (srfraw_started) {
				printf("%s: Cannot use -bloom-idx with -srfraw\n",
					__FUNCTION__);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			srf_started = TRUE;

			if (bloom_idx != 0xFFFFFFFF) {
				printf("%s: -bloom-idx can only be set once\n", __FUNCTION__);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			/* Set bloom index */
			bloom_idx = atoi(val_p);
			if (bloom_idx > 3) {
				printf("%s: Invalid -bloom-idx value\n", __FUNCTION__);
			}
		}
		else if (stricmp(param, "-srfraw") == 0) {
			/* Send raw hex bytes of SRF control and SRF to firmware */
			/* Temporarily store SRF */
			if (srf_started) {
				printf("%s: srfraw is used without other srf options\n",
					__FUNCTION__);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}

			srf_raw = val_p;
			srfraw_started = TRUE;
		}
		else if (stricmp(param, "-info") == 0) {
			/* Temporarily store service info */
			svc_info = val_p;
		}
		else if (stricmp(param, "-match") == 0) {
			/* Temporarily store matching filter */
			match = val_p;
		}
		else if (stricmp(param, "-match-rx") == 0) {
			/* Temporarily store matching filter rx */
			match_rx = val_p;
		}
		else {
			/* For mandatory params, check that value is provided and is numeric */
		val = strtoul(val_p, &endptr, 0);
		if (*endptr != '\0') {
			printf("%s: Value is not uint or hex %s\n", __FUNCTION__, val_p);
				ret = BCME_USAGE_ERROR;
				goto exit;
		}

		if (stricmp(param, "-flags") == 0) {
			params.flags = val;
		}
		else if (stricmp(param, "-period") == 0) {
			params.period = val;
		}
		else if (stricmp(param, "-ttl") == 0) {
			params.ttl = val;
		}
		else {
			printf("%s: Unrecognized parameter %s\n", __FUNCTION__, param);
				ret = BCME_USAGE_ERROR;
				goto exit;
		}
		/* TODO add matching filters, service specific info */
	}
	}

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL) {
		ret = BCME_NOMEM;
		goto exit;
	}

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = htod16(cmd->id);   /* override with cmd attr */
	nanioc->len = htod16(WLC_IOCTL_MAXLEN);
	pxtlv = nanioc->data;

	/* max data we can write, it will be decremented as we pack */
	buflen = WLC_IOCTL_MAXLEN;
	buflen_at_start = buflen;

	/* Mandatory parameters */
	ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_SVC_PARAMS,
		sizeof(wl_nan_disc_params_t), &params, BCM_XTLV_OPTION_ALIGN32);
	if (ret != BCME_OK)
		goto exit;

	/* Optional parameters */
	/* TODO other optional params */
	if (svc_info) {
		ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv,
			&buflen, WL_NAN_XTLV_SVC_INFO, svc_info);
		if (ret != BCME_OK)
		goto exit;
	}
	if (match) {
		ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv,
			&buflen, WL_NAN_XTLV_MATCH_TX, match);
		if (ret != BCME_OK)
			goto exit;
	}
	if (match_rx) {
		/* Create XTLV only if matching filter rx is unique from tx */
		if (!match || (match && strcmp(match, match_rx))) {
			ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv,
				&buflen, WL_NAN_XTLV_MATCH_RX, match_rx);
			if (ret != BCME_OK)
				goto exit;
		}
	}
	/* Check that there is a list of MAC addresses */
	if (mac_list) {
		uint8 srf_control = 0;
		/* Construct SRF control field */
		if (bloom_len) {
			/* This is a bloom filter */
			srf_control = 1;
			if (bloom_idx == 0xFFFFFFFF) {
				bloom_idx = params.instance_id % 4;
			}
			srf_control |= bloom_idx << 2;
		}
		if (srf_include == 'i') {
			srf_control |= 1 << 1;
		}

		if (bloom_len == 0) {
			/* MAC list */
			if (mac_num < NAN_SRF_MAX_MAC) {
				if ((srf = malloc(mac_num * ETHER_ADDR_LEN + 1))	== NULL) {
					printf("Failed to malloc %d bytes \n",
						mac_num * ETHER_ADDR_LEN + 1);
					ret = BCME_NOMEM;
					goto exit;
				}

				srftmp = srf;
				memcpy(srftmp++, &srf_control, 1);
				memcpy(srftmp, mac_list, mac_num * ETHER_ADDR_LEN);
			} else {
				printf("%s: Too many MAC addresses\n", __FUNCTION__);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}

			ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_SR_FILTER,
				(mac_num * ETHER_ADDR_LEN + 1), srf, BCM_XTLV_OPTION_ALIGN32);
			if (ret != BCME_OK)
				goto exit;

		} else {
			/* Bloom filter */
			if (wl_nan_bloom_create(&bp, &bloom_idx, bloom_len) != BCME_OK) {
				printf("%s: Bloom create failed\n", __FUNCTION__);
				ret = BCME_ERROR;
				goto exit;
			}
			uint i;
			srftmp = mac_list;
			for (i = 0; i < mac_num; i++) {
				if (bcm_bloom_add_member(bp, srftmp,
					ETHER_ADDR_LEN) != BCME_OK) {
					printf("%s: Cannot add to bloom filter\n",
						__FUNCTION__);
				}
				srftmp += ETHER_ADDR_LEN;
			}

			/* Create bloom filter */
			if ((srf = malloc(bloom_len + 1)) == NULL) {
				printf("Failed to malloc %d bytes \n", bloom_len + 1);
				ret = BCME_NOMEM;
				goto exit;
			}
			srftmp = srf;
			memcpy(srftmp++, &srf_control, 1);

			uint bloom_size;
			if ((bcm_bloom_get_filter_data(bp, bloom_len, srftmp,
				&bloom_size)) != BCME_OK) {
				printf("%s: Cannot get filter data\n", __FUNCTION__);
				ret = BCME_ERROR;
				goto exit;
			}

			ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_SR_FILTER,
				(bloom_len + 1), srf, BCM_XTLV_OPTION_ALIGN32);
			if (ret != BCME_OK)
				goto exit;
		}
	}
	if (srf_raw) {
		ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv,
			&buflen, WL_NAN_XTLV_SR_FILTER, srf_raw);
		if (ret != BCME_OK)
			goto exit;
	}
	/* adjust iocsz to the end of last data record */
	buflen = htod16(buflen);
	iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);
	nanioc->len = (buflen_at_start - buflen);

	ret = wlu_var_setbuf(wl, "nan", nanioc, iocsz);

exit:
	if (nanioc)
		free(nanioc);
	if (srf)
		free(srf);
	if (mac_list)
		free(mac_list);
	if (bp)
		bcm_bloom_destroy(&bp, wl_nan_bloom_free);
	return ret;
}

static int wl_nan_bloom_create(bcm_bloom_filter_t** bp, uint* idx, uint size)
{
	uint i;
	int err;

	err = bcm_bloom_create(wl_nan_bloom_alloc, wl_nan_bloom_free,
		idx, WL_NAN_HASHES_PER_BLOOM, size, bp);
	if (err != BCME_OK) {
		goto exit;
	}

	/* Populate bloom filter with hash functions */
	for (i = 0; i < WL_NAN_HASHES_PER_BLOOM; i++) {
		err = bcm_bloom_add_hash(*bp, wl_nan_hash, &i);
		if (err) {
			goto exit;
		}
	}
exit:
	return err;
}

static void* wl_nan_bloom_alloc(void *ctx, uint size)
{
	BCM_REFERENCE(ctx);
	uint8 *buf;

	if ((buf = malloc(size)) == NULL) {
		printf("Failed to malloc %d bytes \n", size);
		buf = NULL;
	}

	return buf;
}

static void wl_nan_bloom_free(void *ctx, void *buf, uint size)
{
	BCM_REFERENCE(ctx);
	BCM_REFERENCE(size);

	free(buf);
}

static uint wl_nan_hash(void* ctx, uint index, const uint8 *input, uint input_len)
{
	uint8* filter_idx = (uint8*)ctx;
	uint8 i = (*filter_idx * WL_NAN_HASHES_PER_BLOOM) + (uint8)index;
	uint b = 0;

	/* Steps 1 and 2 as explained in Section 6.2 */
	/* Concatenate index to input and run CRC32 by calling hndcrc32 twice */
	b = hndcrc32(&i, sizeof(uint8), CRC32_INIT_VALUE);
	b = hndcrc32((uint8*) input, input_len, b);
	/* Obtain the last 2 bytes of the CRC32 output */
	b &= NAN_BLOOM_CRC32_MASK;

	/* Step 3 is completed by bcmbloom functions */
	return b;

}

static int
wl_nan_subcmd_publish(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	int res = BCME_OK;
	wl_nan_ioc_t*nanioc;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = htod16(cmd->id);
	nanioc->len = NAN_IOC_BUFSZ;

	if (*argv == NULL) { /* get  */
		/* TODO Ignoring error due to fixed length buffer, fix */
		wl_nan_do_get_ioctl(wl, nanioc, iocsz);
	} else {
		res = wl_nan_subcmd_svc(wl, cmd, argv, WL_NAN_PUB_BOTH);
	}

	if (res != BCME_OK)
		printf("error:%d \n", res);
	if (nanioc)
		free(nanioc);
	return res;
}

static int
wl_nan_subcmd_subscribe(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	int res = BCME_OK;
	wl_nan_ioc_t*nanioc;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = htod16(cmd->id);
	nanioc->len = NAN_IOC_BUFSZ;

	if (*argv == NULL) { /* get  */
		/* TODO Ignoring error due to fixed length buffer, fix */
		wl_nan_do_get_ioctl(wl, nanioc, iocsz);
	} else {
		res = wl_nan_subcmd_svc(wl, cmd, argv, 0);
	}

	if (res != BCME_OK)
		printf("error:%d \n", res);
	if (nanioc)
		free(nanioc);
	return res;
}

static int
wl_nan_subcmd_cancel(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	int ret = BCME_OK;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;
	wl_nan_ioc_t *nanioc = NULL;
	int int_val = 0;
	wl_nan_instance_id_t instance_id;
	uint8 *pxtlv;
	uint16 buflen, buflen_at_start;

	if (!argv[0] || argv[1]) {
		printf("%s : Incorrect number of parameters.\n", __FUNCTION__);
		ret = BCME_USAGE_ERROR;
		goto exit;

	}
	int_val = atoi(argv[0]);
	if (int_val < 0 || int_val > 255) {
		printf("%s : Invalid instance id.\n", __FUNCTION__);
		ret = BCME_USAGE_ERROR;
		goto exit;
	}

	instance_id = int_val;
	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = htod16(cmd->id);
	nanioc->len = htod16(iocsz);
	pxtlv = nanioc->data;

	/* max data we can write, it will be decremented as we pack */
	buflen = NAN_IOC_BUFSZ;
	buflen_at_start = buflen;

	/* Mandatory parameters */
	ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_INSTANCE_ID,
		sizeof(instance_id), &instance_id, BCM_XTLV_OPTION_ALIGN32);
	if (ret != BCME_OK)
		goto exit;

	/* adjust iocsz to the end of last data record */
	iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);
	nanioc->len = (buflen_at_start - buflen);

	ret = wlu_var_setbuf(wl, "nan", nanioc, iocsz);

exit:
	if (nanioc)
		free(nanioc);
	return ret;
}

/*
 *  wl "nan" iovar iovar handler
 */
static int
wl_nan_subcmd_cancel_publish(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	return wl_nan_subcmd_cancel(wl, cmd, argv);
}

static int
wl_nan_subcmd_cancel_subscribe(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	return wl_nan_subcmd_cancel(wl, cmd, argv);
}

static int
wl_nan_subcmd_transmit(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	int ret = BCME_OK;
	int id, dest_id = 0;
	struct ether_addr dest = ether_null;
	/* Set highest value as the default value for priority */
	uint8 priority = 1;
	uint16 iocsz = WLC_IOCTL_MAXLEN;
	wl_nan_ioc_t *nanioc = NULL;
	uint8 *pxtlv = NULL;
	uint16 buflen, buflen_at_start;
	char *param = NULL;

	/* Copy mandatory parameters from command line. */
	if (!*argv) {
		printf("%s : Missing instance id parameter.\n", __FUNCTION__);
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	id = atoi(*argv++);
	if (id < 0 || id > 255) {
		printf("%s : Invalid instance_id.\n", __FUNCTION__);
		ret = BCME_USAGE_ERROR;
		goto exit;
	}

	if (!*argv) {
		printf("%s : Missing requestor id parameter.\n", __FUNCTION__);
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	dest_id = atoi(*argv++);
	if (dest_id < 0 || dest_id > 255) {
		printf("%s : Invalid requestor_id.\n", __FUNCTION__);
		ret = BCME_USAGE_ERROR;
		goto exit;
	}

	if (!*argv) {
		printf("%s : Missing destination MAC address.\n", __FUNCTION__);
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	if (!wl_ether_atoe(*argv++, &dest)) {
			printf("%s: Invalid ether addr provided\n", __FUNCTION__);
			ret = BCME_USAGE_ERROR;
			goto exit;
	}

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = htod16(cmd->id);   /* override with cmd attr */
	nanioc->len = htod16(WLC_IOCTL_MAXLEN);
	pxtlv = nanioc->data;

	/* max data we can write, it will be decremented as we pack */
	buflen = WLC_IOCTL_MAXLEN;
	buflen_at_start = buflen;

	ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_INSTANCE_ID,
		sizeof(wl_nan_instance_id_t), &id, BCM_XTLV_OPTION_ALIGN32);
	if (ret != BCME_OK)
		goto exit;

	ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_REQUESTOR_ID,
		sizeof(wl_nan_instance_id_t), &dest_id, BCM_XTLV_OPTION_ALIGN32);
	if (ret != BCME_OK)
		goto exit;

	ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_MAC_ADDR,
		ETHER_ADDR_LEN, &dest, BCM_XTLV_OPTION_ALIGN32);
	if (ret != BCME_OK)
		goto exit;

	/* Parse optional args. Validity is checked by discovery engine */
	while ((param = *argv++) != NULL) {
		if (stricmp(param, "-priority") == 0) {
			priority = atoi(*argv++);
			ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_PRIORITY,
				sizeof(uint8), &priority, BCM_XTLV_OPTION_ALIGN32);
			if (ret != BCME_OK)
				goto exit;
		}
		else if (stricmp(param, "-info") == 0) {
			ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv, &buflen,
				WL_NAN_XTLV_SVC_INFO, *argv++);
			if (ret != BCME_OK)
				goto exit;
		}
		else {
			printf("%s: Unrecognized parameter %s\n", __FUNCTION__, param);
			ret = BCME_USAGE_ERROR;
			goto exit;
		}
	}

	/* adjust iocsz to the end of last data record */
	buflen = htod16(buflen);
	iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);
	nanioc->len = (buflen_at_start - buflen);

	ret = wlu_var_setbuf(wl, "nan", nanioc, iocsz);
exit:
	if (nanioc)
		free(nanioc);
	return ret;
}

/*
 *  packs user data (in hex string) into tlv record
 *  advances tlv pointer to next xtlv slot
 *  buflen is used for tlv_buf space check
 */
static int
bcm_pack_xtlv_entry_from_hex_string(uint8 **tlv_buf, uint16 *buflen, uint16 type, char *hex)
{
	bcm_xtlv_t *ptlv = (bcm_xtlv_t *)*tlv_buf;
	uint16 len = strlen(hex)/2;

	/* copy data from tlv buffer to dst provided by user */

	if (ALIGN_SIZE(BCM_XTLV_HDR_SIZE + len, BCM_XTLV_OPTION_ALIGN32) > *buflen) {
		printf("bcm_pack_xtlv_entry: no space tlv_buf: requested:%d, available:%d\n",
		 ((int)BCM_XTLV_HDR_SIZE + len), *buflen);
		return BCME_BADLEN;
	}
	ptlv->id = htol16(type);
	ptlv->len = htol16(len);

	/* copy callers data */
	if (get_ie_data((uchar*)hex, ptlv->data, len)) {
		return BCME_BADARG;
	}

	/* advance callers pointer to tlv buff */
	*tlv_buf += BCM_XTLV_SIZE(ptlv, BCM_XTLV_OPTION_ALIGN32);
	/* decrement the len */
	*buflen -=  BCM_XTLV_SIZE(ptlv, BCM_XTLV_OPTION_ALIGN32);
	return BCME_OK;
}

static int
wl_nan_subcmd_merge(void *wl, const wl_nan_sub_cmd_t  *cmd, char **argv)
{

	int res = BCME_OK;
	wl_nan_ioc_t *nanioc;
	uint16 buflen;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;
	uint8 *pxtlv;

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = cmd->id;
	nanioc->len = htod16(iocsz);
	pxtlv = nanioc->data;


	if (*argv == NULL) { /* get  */

		iocsz = sizeof(wl_nan_ioc_t) + sizeof(bcm_xtlv_t);
		wl_nan_do_get_ioctl(wl, nanioc, iocsz);

	} else {	/* set */
		/* max tlv data we can write, it will be decremented as we pack */
		uint8 val =  atoi(*argv);
		printf("nan %s :%d\n", cmd->name, atoi(*argv));

		buflen = NAN_IOC_BUFSZ;
		/* save buflen at start  */
		uint16 buflen_at_start = buflen;
		/* we'll adjust final ioc size at the end */

		res = bcm_pack_xtlv_entry(&pxtlv,
			&buflen, WL_NAN_XTLV_ENABLED, sizeof(uint8), &val, BCM_XTLV_OPTION_ALIGN32);
		if (res != BCME_OK) goto fail;

		/* adjust iocsz to the end of last data record */
		iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);
		nanioc->len = (buflen_at_start - buflen);

		res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);
	}

	free(nanioc);
	return res;

fail:
	printf("error:%d \n", res);
	free(nanioc);
	return res;
}

static int
wl_nan_control(void *wl, cmd_t *cmd, char **argv)
{
	int ret = BCME_USAGE_ERROR;
	char *nan_query[2] = {"enable", NULL};
	char *nan_en[3] = {"enable", "1", NULL};
	char *nan_dis[3] = {"enable", "0", NULL};
	const wl_nan_sub_cmd_t *nancmd = &nan_cmd_list[0];
	/* Skip the command name */
	UNUSED_PARAMETER(cmd);

	argv++;
	/* skip to cmd name after "nan" */
	if (!*argv) {
		/* query nan "enable" state */
		argv = nan_query;
	} else
	if (*argv[0] == 0x31) {
		argv = nan_en;
	} else
	if (*argv[0] == 0x30) {
		argv = nan_dis;
	} else
	if (!strcmp(*argv, "-h") || !strcmp(*argv, "help"))  {
		/* help , or -h* */
		return BCME_USAGE_ERROR;
	}

	while (nancmd->name != NULL) {
		if (strcmp(nancmd->name, *argv) == 0)  {
			/* dispacth cmd to appropriate handler */
			if (nancmd->handler)
				ret = nancmd->handler(wl, nancmd, ++argv);

			return ret; /* nan_cmd_disaptcher() */
		}
		nancmd++;
	}

	return BCME_IOCTL_ERROR;
}

static int
wl_nan_subcmd_tsreserve(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	int res = BCME_OK;
	wl_nan_ioc_t*nanioc;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;
	uint8 *pxtlv;
	uint16 buflen, buflen_at_start;
	chanspec_t chanspec;

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = htod16(cmd->id);
	nanioc->len = NAN_IOC_BUFSZ;
	pxtlv = nanioc->data;

	if (*argv == NULL) { /* get  */
		/* TODO Ignoring error due to fixed length buffer, fix */
		wl_nan_do_get_ioctl(wl, nanioc, iocsz);
	} else {
		nan_timeslot_t tsconfig;

		bzero(&tsconfig, sizeof(nan_timeslot_t));
		tsconfig.abitmap = strtoul(argv[0], NULL, 0);
		argv++;
		if (tsconfig.abitmap) {
			/* bitmap chanlist */
			int i;
			uint32 bitmap = tsconfig.abitmap;
			for (i = 0; (i < NAN_MAX_TIMESLOT) && bitmap; i++) {
				if (bitmap & (1 << i)) {
					if (!argv[0]) {
						fprintf(stderr, "missing chanspec\n");
						goto exit;
					}
					if ((chanspec = wf_chspec_aton(argv[0])) == 0) {
						fprintf(stderr, "invalid chanspec %s\n", argv[0]);
						res = BCME_BADARG;
						goto exit;
					}
					tsconfig.chanlist[i] = chanspec;
					bitmap &= ~(1 << i);
					argv++;
				}
			}
		} else {
			/* bitmap chanspec duration */
			if (argv[0] && argv[1]) {
				if ((chanspec = wf_chspec_aton(argv[0])) == 0) {
					fprintf(stderr, "%s: could not parse \"%s\" as channel\n",
						cmd->name, argv[0]);
					res = BCME_BADARG;
					goto exit;
				}
				tsconfig.chanlist[0] = wl_chspec_to_driver(chanspec);
				tsconfig.chanlist[1] = atoi(argv[1]);
			} else if (!argv[0]) {
				fprintf(stderr, "missing chanspec\n");
				goto exit;
			} else {
				fprintf(stderr, "missing duration\n");
				goto exit;
			}
		}

		buflen = NAN_IOC_BUFSZ;
		/* save buflen at start  */
		buflen_at_start = buflen;
		/* we'll adjust final ioc size at the end */
		res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_TSRESERVE,
			sizeof(nan_timeslot_t), &tsconfig, BCM_XTLV_OPTION_ALIGN32);
		if (res != BCME_OK) {
			fprintf(stderr, "xtlv error %d\n", res);
			goto exit;
		}
		/* adjust iocsz to the end of last data record */
		iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);
		nanioc->len = (buflen_at_start - buflen);

		res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);
	}

exit:
	if (nanioc)
		free(nanioc);
	return res;
}

static int
wl_nan_subcmd_tsrelease(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	int res = BCME_OK;
	wl_nan_ioc_t*nanioc;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;
	uint16 buflen, buflen_at_start;
	uint8 *pxtlv;
	uint32 bitmap;

	if (!*argv) {
		fprintf(stderr, "missing available bitmap\n");
		return BCME_BADARG;
	}

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = cmd->id;
	nanioc->len = NAN_IOC_BUFSZ;
	pxtlv = nanioc->data;

	/* set  */
	bitmap =  strtoul(*argv, NULL, 0);

	buflen = NAN_IOC_BUFSZ;
	/* save buflen at start  */
	buflen_at_start = buflen;
	/* we'll adjust final ioc size at the end */

	res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_TSRELEASE,
		sizeof(uint32), &bitmap, BCM_XTLV_OPTION_ALIGN32);
	if (res != BCME_OK) goto fail;

	/* adjust iocsz to the end of last data record */
	iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);
	nanioc->len = (buflen_at_start - buflen);

	res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);

fail:
	if (res != BCME_OK)
		printf("error:%d \n", res);
	if (nanioc)
		free(nanioc);
	return res;
}
static int
wl_nan_subcmd_OUI(void *wl, const wl_nan_sub_cmd_t *cmd, char **argv)
{
	int res = BCME_OK;
	wl_nan_ioc_t*nanioc;
	uint16 iocsz = sizeof(wl_nan_ioc_t) + NAN_IOC_BUFSZ;
	uint16 buflen, buflen_at_start;
	uint8 *pxtlv;

	/*  alloc mem for ioctl headr +  tlv data  */
	nanioc = calloc(1, iocsz);
	if (nanioc == NULL)
		return BCME_NOMEM;

	/* make up nan cmd ioctl header */
	nanioc->version = htod16(WL_NAN_IOCTL_VERSION);
	nanioc->id = cmd->id;
	nanioc->len = NAN_IOC_BUFSZ;
	pxtlv = nanioc->data;

	if (*argv) { /* set  */
		uint32 value = 0;
		uint8 type;

		if (!argv[1]) {
			fprintf(stderr, "missing OUI type\n");
			res = BCME_BADARG;
			goto fail;
		}
		if (get_oui_bytes((uchar *)argv[0], (uchar *)&value)) {
			fprintf(stderr, "invalid OUI\n");
			res = BCME_BADARG;
			goto fail;
		}

		type =  strtoul(argv[1], NULL, 0);
		value = (value & 0xffffff) + (type << 24);

		buflen = NAN_IOC_BUFSZ;
		/* save buflen at start  */
		buflen_at_start = buflen;
		/* we'll adjust final ioc size at the end */

		res = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_OUI,
			sizeof(uint32), &value, BCM_XTLV_OPTION_ALIGN32);
		if (res != BCME_OK) {
			printf("error %d\n", res);
			goto fail;
		}
		/* adjust iocsz to the end of last data record */
		iocsz = sizeof(wl_nan_ioc_t) + (buflen_at_start - buflen);
		nanioc->len = (buflen_at_start - buflen);

		res = wlu_var_setbuf(wl, "nan", nanioc, iocsz);
	} else {
		wl_nan_do_get_ioctl(wl, nanioc, iocsz);
	}
fail:
	if (nanioc)
		free(nanioc);
	return res;
}

#endif /* WL_NAN */
