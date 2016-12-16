/*
 * wl nan command module
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
 * $Id: wluc_nan.c 458728 2014-02-27 18:15:25Z $
 */
#ifdef WL_NAN

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
#include "wlu_avail_utils.h"
#include <bcmiov.h>

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

#include <bcmcrypto/sha256.h>
#include <proto/nan.h>
#include <bcmbloom.h>
#include <bcmtlv.h>

#define MAX_MAC_BUF  20
#define MAX_RSSI_BUF 128
#define MAX_OOB_AF_LIFETIME	512
static void
print_peer_rssi(wl_nan_peer_rssi_data_t *prssi);

static cmd_func_t wl_nan_control;

#ifndef NAN_MAX_PEERS
#define NAN_MAX_PEERS 16
#endif
#define WLU_AVAIL_MAX_SLOTS 32
#define WL_NAN_INVALID_NDPID 0

#define NAN_PARAMS_USAGE	\
"\tUsage: wl nan [command] [cmd options] as follows:\n"	\
"\t\twl nan init [1/0]  1 - initialize nan, 0 - uninitialize nan\n" \
"\t\twl nan enable [1/0] - enable disable nan functionality\n" \
"\t\twl nan state [role] - sets or gets the nan role\n" \
"\t\twl nan hop_count [value] - sets or gets the hop count value\n" \
"\t\twl nan hop_limit [value] - sets or gets the hop count threshold "\
"value\n" \
"\t\twl nan warm_up_time [value] - sets or gets the warm-up time in"\
"units of seconds\n" \
"\t\twl nan rssi_threshold [band][close][mid] - set rssi threshold for a band\n"\
"\t\twl nan status -  gets the status of NAN network \n" \
"\t\twl nan OUI [value] - sets or gets the OUI value\n" \
"\t\twl nan count - get counters\n" \
"\t\twl nan clearcount - clears the counters\n" \
"\t\twl nan chan [chan num] - sets the channel value\n" \
"\t\twl nan band [value] - sets the band value(a/g/auto)\n" \
"\t\twl nan host_enable [value] - enable/disable host election\n" \
"\t\twl nan election_metrics [random_fact][master pref] - set random factor and preference\n" \
"\t\twl nan election_metrics_state - get random fact and preference\n" \
"\t\twl nan join [-start][cid] - joins/starts a nan network\n" \
"\t\twl nan leave [cid]- leaves the network with cid mentioned\n" \
"\t\twl nan merge [cid]- merges the network with cid mentioned\n" \
"\t\twl nan max_peers - Sets / Gets Max NAN Peers\n" \
"\t\twl nan stop [cid]-  stop participating in the network\n" \
"\t\twl nan publish [instance] [service name] [options] - get/set\n" \
"\t\twl nan publish_list [..]\n" \
"\t\twl nan cancel_publish [..]\n" \
"\t\twl nan subscribe [instance] [service name] [options]\n" \
"\t\twl nan subscribe_list [..]\n" \
"\t\twl nan cancel_subscribe [..]\n" \
"\t\twl nan vendor_info [..]\n" \
"\t\twl nan disc_stats [..]\n" \
"\t\twl nan disc_transmit [..]\n" \
"\t\twl nan followup_transmit [..]\n" \
"\t\twl nan show - Shows the NAN status\n" \
"\t\twl nan soc_chans <2G> <5G> - Sets / Gets NAN social channel\n" \
"\t\twl nan awake_dw <2G> <5G> - Sets / Gets DW Awake interval\n" \
"\t\twl nan rssi_notif_thld <2G> <5G> - Sets / Gets Sync Beacon RSSI Notification thresholds\n" \
"\t\twl nan rssi_thld <2G> <2Gclose> <5GMid> <5GClose> - Sets / Gets RSSI thresholds\n" \
"\t\twl nan max_peers <max_peers> - Sets / Gets Max Peers\n" \
"\t\twl nan disc_connection [..]\n" \
"\t\twl nan scan_params -s [scantime] -h [hometime] -i "\
"[merge_scan_interval] -d [merge_scan_duration] -c [6,44,149]\n"\
"\t\twl nan scan\n" \
"\t\twl nan scanresults\n" \
"\t\twl nan event_msgs\n" \
"\t\twl nan event_check\n" \
"\t\twl nan dump\n" \
"\t\twl nan clear\n" \
"\t\twl nan rssi\n" \
"\t\twl nan disc_results\n" \
"\t\twl nan dp_show\n" \
"\t\twl nan dp_stats\n" \
"\t\twl nan dp_status <ndp_id> <reason_code>\n" \
"\t\twl nan dp_schedupd <ndp_id>  \n" \
"\t\twl nan dp_autoconn <val> - 0th bit for auto_dpresp, 1st bit for auto_dpconf \n" \
"\t\twl nan dp_conf <ndp id> <status>\n" \
"\t\twl nan dp_end <ndp id> <status>\n" \
"\t\twl nan dp_req ucast/mcast pub_id <publisher id> peer_mac<peer mac address>" \
" mcast_mac <if dest multicast addr exists> qos <tid, pkt_size, mean_data_rate, " \
" max_service_interval> security <val> svc_spec_info <app specific info in hex>\n" \
"\t\twl nan dp_resp ucast/mcast ndp_id <ndp id/ mc_id for unicast or multicast>" \
" peer_mac<peer mac address> mcast_mac <if dest multicast addr exists>" \
" qos <tid, pkt_size, mean_data_rate, max_service_interval> security <val>" \
" svc_spec_info <app specific info in hex>\n" \
"\tUsage: mutiple commands batching\n\n"\
"\tA set of either SET or GET commands can be batched\n"\
"\t\twl nan enable [1/0] + attr [MAC, ETC]\n" \
"\t\tabove command issues SET enable and SET attr commands at a time\n"\
"\t\twl nan enable + scanresults\n" \
"\t\tabove command issues GET enable and GET scanresults commands at a time\n"

typedef struct wl_nan_dp_sub_cmd wl_nan_dp_sub_cmd_t;
typedef int (nan_dp_cmd_hdlr_t)(void *wl, const wl_nan_dp_sub_cmd_t *cmd, char **argv, int ndp_id);
/* nan cmd list entry  */
struct wl_nan_dp_sub_cmd {
	char *name;                     /* cmd name */
	uint8  version;                 /* cmd  version */
	uint16 id;                      /* id for the dongle f/w switch/case  */
	uint16 type;                    /* base type of argument */
	nan_dp_cmd_hdlr_t *handler;    /* cmd handler */
};

#define NAN_DP_AVAIL_MAX_STR_LEN	32
#define NAN_DP_AVAIL_ENTRIES		8

typedef struct wl_nan_dp_avail_entry {
	uint32 period;
	char entry[NAN_DP_AVAIL_MAX_STR_LEN];
} wl_nan_dp_avail_entry_t;

static char *
bcm_ether_ntoa(const struct ether_addr *ea, char *buf);

static cmd_t wl_nan_cmds[] = {
	{ "nan", wl_nan_control, WLC_GET_VAR, WLC_SET_VAR, ""},
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

uint8
wl_nan_resolution_timeunit(uint8 dur)
{
	switch (dur) {
		case NAN_AVAIL_RES_16_TU:
			return 16;
		break;
		case NAN_AVAIL_RES_32_TU:
			return 32;
		break;
		case NAN_AVAIL_RES_64_TU:
			return 64;
		break;
		default:
			return 255;
	}
}

uint8
wl_nan_resolution_string_to_num(const char *res)
{
	uint8 res_n = NAN_AVAIL_RES_INVALID;
	if (!strcmp(res, "16tu")) {
		res_n = NAN_AVAIL_RES_16_TU;
	}
	else if (!strcmp(res, "32tu")) {
		res_n = NAN_AVAIL_RES_32_TU;
	}
	else if (!strcmp(res, "64tu")) {
		res_n = NAN_AVAIL_RES_64_TU;
	}
	else {
		// Invalid
	}
	return res_n;
}

static wl_nan_status_t
wl_nan_is_instance_valid(int id)
{
	wl_nan_status_t ret = WL_NAN_E_OK;

	if (id <= 0 || id > 255) {
		ret = WL_NAN_E_BAD_INSTANCE;
	}

	return ret;
}

static int
bcm_pack_xtlv_entry_from_hex_string(uint8 **tlv_buf, uint16 *buflen, uint16 type, char *hex);
static int wl_nan_bloom_create(bcm_bloom_filter_t** bp, uint* idx, uint size);
static void* wl_nan_bloom_alloc(void *ctx, uint size);
static void wl_nan_bloom_free(void *ctx, void *buf, uint size);
static uint wl_nan_hash(void* ctx, uint idx, const uint8 *input, uint input_len);
static void wlu_nan_print_wl_avail_entry(uint8 avail_type, wl_avail_entry_t* entry);
static int wl_nan_set_avail_entry_optional(char** argv, wl_avail_t* avail,
	wl_avail_entry_t* entry, uint8 avail_type, uint8* num);
static void wl_nan_print_status(wl_nan_conf_status_t *nstatus);

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
	enum wl_nan_sub_cmd_xtlv_id type;
};

/*  nan ioctl sub cmd handler functions  */
static cmd_handler_t wl_nan_subcmd_cfg_enable;
static cmd_handler_t wl_nan_subcmd_cfg_init;
static cmd_handler_t wl_nan_subcmd_cfg_role;
static cmd_handler_t wl_nan_subcmd_cfg_hop_count;
static cmd_handler_t wl_nan_subcmd_cfg_hop_limit;
static cmd_handler_t wl_nan_subcmd_cfg_warmup_time;
static cmd_handler_t wl_nan_subcmd_cfg_status;
static cmd_handler_t wl_nan_subcmd_cfg_oui;
static cmd_handler_t wl_nan_subcmd_cfg_count;
static cmd_handler_t wl_nan_subcmd_cfg_clearcount;
static cmd_handler_t wl_nan_subcmd_cfg_channel;
static cmd_handler_t wl_nan_subcmd_cfg_band;
static cmd_handler_t wl_nan_subcmd_cfg_cid;
static cmd_handler_t wl_nan_subcmd_cfg_if_addr;
static cmd_handler_t wl_nan_subcmd_cfg_bcn_interval;
static cmd_handler_t wl_nan_subcmd_cfg_sdf_txtime;
static cmd_handler_t wl_nan_subcmd_cfg_sid_beacon;
static cmd_handler_t wl_nan_subcmd_cfg_wfa_testmode;
static cmd_handler_t wl_nan_subcmd_election_host_enable;
static cmd_handler_t wl_nan_subcmd_election_metrics_config;
static cmd_handler_t wl_nan_subcmd_election_metrics_state;
static cmd_handler_t wl_nan_subcmd_leave;
static cmd_handler_t wl_nan_subcmd_merge;
static cmd_handler_t wl_nan_subcmd_publish;
static cmd_handler_t wl_nan_subcmd_publish_list;
static cmd_handler_t wl_nan_subcmd_cancel_publish;
static cmd_handler_t wl_nan_subcmd_subscribe;
static cmd_handler_t wl_nan_subcmd_subscribe_list;
static cmd_handler_t wl_nan_subcmd_cancel_subscribe;
static cmd_handler_t wl_nan_subcmd_sd_vendor_info;
static cmd_handler_t wl_nan_subcmd_sd_statistics;
static cmd_handler_t wl_nan_subcmd_sd_transmit;
static cmd_handler_t wl_nan_subcmd_sd_connection;
static cmd_handler_t wl_nan_subcmd_sd_show;
static cmd_handler_t wl_nan_subcmd_election_advertisers;
static cmd_handler_t wl_nan_subcmd_disc_results;
static cmd_handler_t wl_nan_subcmd_event_msgs;
static cmd_handler_t wl_nan_subcmd_event_check;
static cmd_handler_t wl_nan_subcmd_dump;
static cmd_handler_t wl_nan_subcmd_clear;
static cmd_handler_t wl_nan_subcmd_dbg_rssi;
static cmd_handler_t wl_nan_subcmd_dbg_level;
/* nan 2.0 */
static cmd_handler_t wl_nan_subcmd_dp_cap;
static cmd_handler_t wl_nan_subcmd_dp_autoconn;
static cmd_handler_t wl_nan_subcmd_dp_req;
static cmd_handler_t wl_nan_subcmd_dp_resp;
static cmd_handler_t wl_nan_subcmd_dp_conf;
static cmd_handler_t wl_nan_subcmd_dp_dataend;
static cmd_handler_t wl_nan_subcmd_dp_status;
static cmd_handler_t wl_nan_subcmd_dp_stats;
static cmd_handler_t wl_nan_subcmd_dp_show;
static cmd_handler_t wl_nan_subcmd_dp_schedupd;
static cmd_handler_t wl_nan_subcmd_cfg_avail;
static cmd_handler_t wl_nan_subcmd_range_req;
static cmd_handler_t wl_nan_subcmd_range_resp;
static cmd_handler_t wl_nan_subcmd_range_cancel;
static cmd_handler_t wl_nan_subcmd_range_auto;
static cmd_handler_t wl_nan_subcmd_soc_chans;
static cmd_handler_t wl_nan_subcmd_awake_dws;
static cmd_handler_t wl_nan_subcmd_sbcn_rssi_notif_thld;
static cmd_handler_t wl_nan_subcmd_sbcn_rssi_thld;
static cmd_handler_t wl_nan_subcmd_max_peers;

/* Mandatory parameters count for different commands */
#define WL_NAN_CMD_CFG_OUI_ARGC				2
#define WL_NAN_CMD_CFG_ELECTION_METRICS_ARGC		2
#define WL_NAN_CMD_CFG_SID_BCN_ARGC			2

static const wl_nan_sub_cmd_t nan_cmd_list[] = {
	/* wl nan enable [0/1] or new: "wl nan [0/1]" */
	{"enable", 0x01, WL_NAN_CMD_CFG_NAN_ENAB,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_enable)
	},
	/* Set / get nan device role */
	{"role", 0x01, WL_NAN_CMD_CFG_ROLE,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_role),
	},
	/* wl nan init [0/1] */
	{"init", 0x01, WL_NAN_CMD_CFG_NAN_INIT,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_init),
	},
	/* -- var attributes (treated as cmds now) --  */
	{"hop_count", 0x01, WL_NAN_CMD_CFG_HOP_CNT,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_hop_count),
	},
	{"hop_limit", 0x01, WL_NAN_CMD_CFG_HOP_LIMIT,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_hop_limit),
	},
	{"warm_up_time", 0x01, WL_NAN_CMD_CFG_WARMUP_TIME,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_warmup_time),
	},
	{"status", 0x01, WL_NAN_CMD_CFG_STATUS,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_status),
	},
	{"OUI", 0x01, WL_NAN_CMD_CFG_OUI,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_oui)
	},
	{"count", 0x01, WL_NAN_CMD_CFG_COUNT,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_count)
	},
	{"clearcount", 0x01, WL_NAN_CMD_CFG_CLEARCOUNT,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_clearcount)
	},
	{"chan", 0x01, WL_NAN_CMD_CFG_CHANNEL,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_channel)
	},
	{"band", 0x01, WL_NAN_CMD_CFG_BAND,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_band)
	},
	{"cluster_id", 0x01, WL_NAN_CMD_CFG_CID,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_cid)
	},
	{"if_addr", 0x01, WL_NAN_CMD_CFG_IF_ADDR,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_if_addr)
	},
	{"bcn_interval", 0x01, WL_NAN_CMD_CFG_BCN_INTERVAL,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_bcn_interval)
	},
	{"sdf_txtime", 0x01, WL_NAN_CMD_CFG_SDF_TXTIME,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_sdf_txtime)
	},
	{"sid_beacon", 0x01, WL_NAN_CMD_CFG_SID_BEACON,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_sid_beacon)
	},
	{"avail", 0x01, WL_NAN_CMD_CFG_AVAIL,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_avail)
	},
	{"wfa_testmode", 0x01, WL_NAN_CMD_CFG_WFA_TM,
	IOVT_BUFFER, WL_NAN_FUNC(cfg_wfa_testmode)
	},
	/* ------- nan mac/disc engine commands ---- */
	{"host_enable", 0x01, WL_NAN_CMD_ELECTION_HOST_ENABLE,
	IOVT_BUFFER, WL_NAN_FUNC(election_host_enable)
	},
	{"election_metrics", 0x01, WL_NAN_CMD_ELECTION_METRICS_CONFIG,
	IOVT_BUFFER, WL_NAN_FUNC(election_metrics_config)
	},
	{"election_metrics_state", 0x01, WL_NAN_CMD_ELECTION_METRICS_STATE,
	IOVT_BUFFER, WL_NAN_FUNC(election_metrics_state)
	},
	{"leave", 0x01, WL_NAN_CMD_ELECTION_LEAVE,
	IOVT_BUFFER, WL_NAN_FUNC(leave)
	},
	{"merge", 0x01, WL_NAN_CMD_ELECTION_MERGE,
	IOVT_BUFFER, WL_NAN_FUNC(merge)
	},
	{"advertisers", 0x01, WL_NAN_CMD_ELECTION_ADVERTISERS,
	IOVT_BUFFER, WL_NAN_FUNC(election_advertisers)
	},
	{"publish", 0x01, WL_NAN_CMD_SD_PUBLISH,
	IOVT_BUFFER, WL_NAN_FUNC(publish)
	},
	{"publish_list", 0x01, WL_NAN_CMD_SD_PUBLISH_LIST,
	IOVT_BUFFER, WL_NAN_FUNC(publish_list)
	},
	{"cancel_publish", 0x01, WL_NAN_CMD_SD_CANCEL_PUBLISH,
	IOVT_BUFFER, WL_NAN_FUNC(cancel_publish)
	},
	{"subscribe", 0x01, WL_NAN_CMD_SD_SUBSCRIBE,
	IOVT_BUFFER, WL_NAN_FUNC(subscribe)
	},
	{"subscribe_list", 0x01, WL_NAN_CMD_SD_SUBSCRIBE_LIST,
	IOVT_BUFFER, WL_NAN_FUNC(subscribe_list)
	},
	{"cancel_subscribe", 0x01, WL_NAN_CMD_SD_CANCEL_SUBSCRIBE,
	IOVT_BUFFER, WL_NAN_FUNC(cancel_subscribe)
	},
	{"vendor_info", 0x01, WL_NAN_CMD_SD_VND_INFO,
	IOVT_BUFFER, WL_NAN_FUNC(sd_vendor_info)
	},
	{"disc_stats", 0x01, WL_NAN_CMD_SD_STATS,
	IOVT_BUFFER, WL_NAN_FUNC(sd_statistics)
	},
	{"disc_transmit", 0x01, WL_NAN_CMD_SD_TRANSMIT,
	IOVT_BUFFER, WL_NAN_FUNC(sd_transmit)
	},
	{"followup_transmit", 0x01, WL_NAN_CMD_SD_FUP_TRANSMIT,
	IOVT_BUFFER, WL_NAN_FUNC(sd_transmit)
	},
	{"disc_connection", 0x01, WL_NAN_CMD_SD_CONNECTION,
	IOVT_BUFFER, WL_NAN_FUNC(sd_connection)
	},
	/* uses same cmd handler for as status */
	{"show", 0x01, WL_NAN_CMD_SD_SHOW,
	IOVT_BUFFER, WL_NAN_FUNC(sd_show)
	},
	/* nan debug commands */
	{"event_msgs", 0x01, WL_NAN_CMD_CFG_EVENT_MASK,
	IOVT_BUFFER, WL_NAN_FUNC(event_msgs)
	},
	{"dbg", 0x01, WL_NAN_CMD_DBG_LEVEL,
	IOVT_BUFFER, WL_NAN_FUNC(dbg_level)
	},
	{"event_check", 0x01, WL_NAN_CMD_DBG_EVENT_CHECK,
	IOVT_BUFFER, WL_NAN_FUNC(event_check)
	},
	{"dump", 0x01, WL_NAN_CMD_DBG_DUMP,
	IOVT_BUFFER, WL_NAN_FUNC(dump),
	},
	{"clear", 0x01, WL_NAN_CMD_DBG_CLEAR,
	IOVT_BUFFER, WL_NAN_FUNC(clear),
	},
	/* uses same cmd handler for as status */
	{"rssi", 0x01, WL_NAN_CMD_DBG_RSSI,
	IOVT_BUFFER, WL_NAN_FUNC(dbg_rssi)
	},
	{"disc_results", 0x01, WL_NAN_CMD_DBG_DISC_RESULTS,
	IOVT_BUFFER, WL_NAN_FUNC(disc_results)
	},
	/* nan2.0 data iovars */
	{"dp_cap", 0x01, WL_NAN_CMD_DATA_CAP,
	IOVT_BUFFER, WL_NAN_FUNC(dp_cap)
	},
	{"dp_autoconn", 0x01, WL_NAN_CMD_DATA_AUTOCONN,
	IOVT_BUFFER, WL_NAN_FUNC(dp_autoconn)
	},
	{"dp_req", 0x01, WL_NAN_CMD_DATA_DATAREQ,
	IOVT_BUFFER, WL_NAN_FUNC(dp_req)
	},
	{"dp_resp", 0x01, WL_NAN_CMD_DATA_DATARESP,
	IOVT_BUFFER, WL_NAN_FUNC(dp_resp)
	},
	{"dp_conf", 0x01, WL_NAN_CMD_DATA_DATACONF,
	IOVT_BUFFER, WL_NAN_FUNC(dp_conf)
	},
	{"dp_end", 0x01, WL_NAN_CMD_DATA_DATAEND,
	IOVT_BUFFER, WL_NAN_FUNC(dp_dataend)
	},
	{"dp_status", 0x01, WL_NAN_CMD_DATA_STATUS,
	IOVT_BUFFER, WL_NAN_FUNC(dp_status)
	},
	{"dp_stats", 0x01, WL_NAN_CMD_DATA_STATS,
	IOVT_BUFFER, WL_NAN_FUNC(dp_stats)
	},
	{"dp_schedupd", 0x01, WL_NAN_CMD_DATA_SCHEDUPD,
	IOVT_BUFFER, WL_NAN_FUNC(dp_schedupd)
	},
	{"dp_show", 0x01, WL_NAN_CMD_DATA_NDP_SHOW,
	IOVT_BUFFER, WL_NAN_FUNC(dp_show)
	},
	{"range_req", 0x01, WL_NAN_CMD_RANGE_REQUEST,
	IOVT_BUFFER, WL_NAN_FUNC(range_req)
	},
	{"range_auto", 0x01, WL_NAN_CMD_RANGE_AUTO,
	IOVT_BUFFER, WL_NAN_FUNC(range_auto)
	},
	{"range_resp", 0x01, WL_NAN_CMD_RANGE_RESPONSE,
	IOVT_BUFFER, WL_NAN_FUNC(range_resp)
	},
	{"range_cncl", 0x01, WL_NAN_CMD_RANGE_CANCEL,
	IOVT_BUFFER, WL_NAN_FUNC(range_cancel)
	},
	{"soc_chans", 0x01, WL_NAN_CMD_SYNC_SOCIAL_CHAN,
	IOVT_BUFFER, WL_NAN_FUNC(soc_chans)
	},
	{"awake_dws", 0x01, WL_NAN_CMD_SYNC_AWAKE_DWS,
	IOVT_BUFFER, WL_NAN_FUNC(awake_dws)
	},
	{"rssi_notif_thld", 0x01, WL_NAN_CMD_SYNC_BCN_RSSI_NOTIF_THRESHOLD,
	IOVT_BUFFER, WL_NAN_FUNC(sbcn_rssi_notif_thld)
	},
	{"rssi_thld", 0x01, WL_NAN_CMD_ELECTION_RSSI_THRESHOLD,
	IOVT_BUFFER, WL_NAN_FUNC(sbcn_rssi_thld)
	},
	{"max_peers", 0x01, WL_NAN_CMD_DATA_MAX_PEERS,
	IOVT_BUFFER, WL_NAN_FUNC(max_peers)
	},
	{NULL, 0, 0, 0, NULL}
};

#define NAN_HELP_ALL 0
#define WL_NAN_HELP_FUNC(suffix) wl_nan_help_ ##suffix

/*  nan ioctl sub cmd handler functions  */
static cmd_help_handler_t wl_nan_help_cfg_enable;
static cmd_help_handler_t wl_nan_help_cfg_init;
static cmd_help_handler_t wl_nan_help_sd_transmit;
static cmd_help_handler_t wl_nan_help_sd_publish;
static cmd_help_handler_t wl_nan_help_sd_cancel_publish;
static cmd_help_handler_t wl_nan_help_sd_publish_list;
static cmd_help_handler_t wl_nan_help_sd_subscribe;
static cmd_help_handler_t wl_nan_help_sd_cancel_subscribe;
static cmd_help_handler_t wl_nan_help_sd_subscribe_list;
static cmd_help_handler_t wl_nan_help_cfg_avail;
static cmd_help_handler_t wl_nan_help_dp_req;
static cmd_help_handler_t wl_nan_help_dp_resp;
static cmd_help_handler_t wl_nan_help_dp_conf;
static cmd_help_handler_t wl_nan_help_dp_end;
static cmd_help_handler_t wl_nan_help_cfg_wfa_testmode;

static const wl_nan_cmd_help_t nan_cmd_help_list[] = {
	/* wl nan enable [0/1] or new: "wl nan [0/1]" */
	{WL_NAN_CMD_CFG_NAN_ENAB, WL_NAN_HELP_FUNC(cfg_enable)},
	{WL_NAN_CMD_CFG_NAN_INIT, WL_NAN_HELP_FUNC(cfg_init)},
	{WL_NAN_CMD_SD_FUP_TRANSMIT, WL_NAN_HELP_FUNC(sd_transmit)},
	{WL_NAN_CMD_SD_TRANSMIT, WL_NAN_HELP_FUNC(sd_transmit)},
	{WL_NAN_CMD_SD_PUBLISH, WL_NAN_HELP_FUNC(sd_publish)},
	{WL_NAN_CMD_SD_CANCEL_PUBLISH, WL_NAN_HELP_FUNC(sd_cancel_publish)},
	{WL_NAN_CMD_SD_PUBLISH_LIST, WL_NAN_HELP_FUNC(sd_publish_list)},
	{WL_NAN_CMD_SD_SUBSCRIBE, WL_NAN_HELP_FUNC(sd_subscribe)},
	{WL_NAN_CMD_SD_CANCEL_SUBSCRIBE, WL_NAN_HELP_FUNC(sd_cancel_subscribe)},
	{WL_NAN_CMD_SD_SUBSCRIBE_LIST, WL_NAN_HELP_FUNC(sd_subscribe_list)},
	{WL_NAN_CMD_CFG_AVAIL, WL_NAN_HELP_FUNC(cfg_avail)},
	{WL_NAN_CMD_DATA_DATAREQ, WL_NAN_HELP_FUNC(dp_req)},
	{WL_NAN_CMD_DATA_DATARESP, WL_NAN_HELP_FUNC(dp_resp)},
	{WL_NAN_CMD_DATA_DATACONF, WL_NAN_HELP_FUNC(dp_conf)},
	{WL_NAN_CMD_DATA_DATAEND, WL_NAN_HELP_FUNC(dp_end)},
	{WL_NAN_CMD_CFG_WFA_TM, WL_NAN_HELP_FUNC(cfg_wfa_testmode)},
	{0, NULL}
};

void
wl_nan_help_cfg_enable(void)
{
	printf("wl nan enable [<1|0>]\n");
	printf("\t1: Enable\n\t0: Disable\n");
}

void
wl_nan_help_cfg_init(void)
{
	printf("wl nan init [<0|1>]\n");
	printf("\t0: Uninitialized\n\t1: Initialized\n");
}

void
wl_nan_help_sd_transmit(void)
{
	printf("wl nan followup_transmit lcl-inst <local-instance-id> "
		"remote-inst <remote-instance-id> dest-mac <destination MAC>\n");
}

void
wl_nan_help_sd_publish(void)
{
	printf("\t\twl nan publish [instance] [service name] [options] - get/set\n");
}

void
wl_nan_help_sd_cancel_publish(void)
{
	printf("\t\twl nan cancel_publish [..]\n");
}

void
wl_nan_help_sd_publish_list(void)
{
	printf("\t\twl nan publish_list [..]\n");
}

void
wl_nan_help_sd_subscribe(void)
{
	printf("\t\twl nan subscribe [instance] [service name] [options]\n");
}

void
wl_nan_help_sd_cancel_subscribe(void)
{
	printf("\t\twl nan cancel_subscribe [..]\n");
}

void
wl_nan_help_sd_subscribe_list(void)
{
	printf("\t\twl nan subscribe_list [..]\n");
}

void
wl_nan_help_cfg_avail(void)
{
	printf("\twl nan avail <avail type> entry <entry type> [bitmap | otahexmap <>] ");
	printf("[optional params] [entry...]\n\n");
	printf("\tMandatory params\n");
	printf("\t\t<avail type> 1=local, 2=peer, 3=ndc base sched, 4=immutable, ");
	printf("5=response, 6=counter, 7=ranging\n");
	printf("\t\t<entry type> 1=committed, 2=potential, 4=conditional\n");
	printf("\tOptional params\n");
	printf("\t\t[bitdur <0=16TU(default), 1=32TU, 2=64TU, 3=128TU>]\n");
	printf("\t\t[period <0=non-repeatable, 1=128TU, 2=256TU, 3=512TU(default), ");
	printf("4=1024TU, 5=2048TU, 6=4096TU, 7=8192TU>]\n");
	printf("\t\t[offset <0 - 511>] default = 0\n");
	printf("\t\t[usage <0-3>] default = 3\n");
	printf("\t\t[ndc <ether addr format>] default = AA:BB:CC:00:00:00, ");
	printf("ndc id for ndc base sched avail\n");
	printf("\t\t[chanspec | band <chanspec or band id>] default = 0, ");
	printf("which means all bands/chanspec are supported\n");
	printf("\t\t[peer <ether addr format>] default = 00:90:4c:AA:BB:CC, ");
	printf("peer nmi address for peer avail\n");
}

void
wl_nan_help_dp_req(void)
{
	printf("\twl nan dp_req ucast/mcast pub_id <publisher id> [confirm] peer_mac"
		"<peer mac address> [mcast_mac <if dest multicast addr exists>] qos"
		"<tid, pkt_size, mean_data_rate, max_service_interval> [security <val>]"
		"[svc_spec_info <app specific info in hex>]\n");
}

void
wl_nan_help_dp_resp(void)
{
	printf("\twl nan dp_resp ucast/mcast ndp_id <ndp id/ mc_id for unicast or multicast>"
		"peer_mac<peer mac address> mcast_mac <if dest multicast addr exists>"
		"qos <tid, pkt_size, mean_data_rate, max_service_interval> security <val>"
		"svc_spec_info <app specific info in hex>\n");
}

void
wl_nan_help_dp_conf(void)
{
	printf("\twl nan dp_conf <ndp id> <status>\n");
}

void
wl_nan_help_dp_end(void)
{
	printf("\twl nan dp_end <ndp id> <status>\n");
}

void
wl_nan_help_cfg_wfa_testmode(void)
{
	printf("\twl nan wfa_testmode [<flags>]\n");
	printf("\t\t<flags> for wfa testmode operation\n");
	printf("\t\t0x00000001 ignore NDP terminate AF\n");
	printf("\t\t0x00000002 ignore rx'ed data frame outside NDL CRBs\n");
	printf("\t\t0x00000004 allow tx data frame outside NDL CRBs\n");
	printf("\t\t0x00000008 enforce NDL counter proposal\n");
}

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

static char *
get_nan_cmd_name(uint16 cmd_id)
{
	const wl_nan_sub_cmd_t *nancmd = &nan_cmd_list[0];

	while (nancmd->name != NULL) {
		if (nancmd->id == cmd_id) {
			return nancmd->name;
		}
		nancmd++;
	}
	return NULL;
}

static void
wl_nan_print_status(wl_nan_conf_status_t *nstatus)
{
	printf("enabled	:%d\n", nstatus->enabled);
	print_nan_role(nstatus->role);
	printf("election mode	:%d\n", nstatus->election_mode);
	bcm_ether_ntoa(&nstatus->cid, buf);
	printf("current cluster_id: %s\n", buf);
	bcm_ether_ntoa(&nstatus->nmi, buf);
	printf("current NMI : %s\n", buf);
	printf("social chan (2G) : %d\n", nstatus->social_chans[0]);
	printf("social chan (5G) : %d\n", nstatus->social_chans[1]);
	prhex("master_rank:", nstatus->mr, NAN_MASTER_RANK_LEN);
	prhex("amr", nstatus->amr, NAN_MASTER_RANK_LEN);
	printf("hop_count:%d\n", nstatus->hop_count);
	printf("ambtt:%ul\n", nstatus->ambtt);
	printf("cluster tsf [high %ul  Low : %ul]\n",
		(uint32)(nstatus->cluster_tsf_h), (uint32)(nstatus->cluster_tsf_l));
}

static void
wlu_nan_print_wl_avail_entry(uint8 avail_type, wl_avail_entry_t* entry)
{
	wl_avail_entry_t* e = entry;
	uint16 eflags = (ltoh_ua(&e->flags));
	int j, k;
	uint8 tmp;

	prhex("\n Entry", (uchar*)e, e->length);
	if (avail_type < WL_AVAIL_NDC) {
		printf("   Avail type (1=committed, 2=potential, 4=conditional): ");
		printf("%d\n", eflags & (WL_AVAIL_ENTRY_COM | WL_AVAIL_ENTRY_POT |
			WL_AVAIL_ENTRY_COND));
		if (eflags & WL_AVAIL_ENTRY_BAND_PRESENT) {
			printf("   Band: %d\n", ltoh_ua(&e->u.band));
		} else if (eflags & WL_AVAIL_ENTRY_CHAN_PRESENT) {
			printf("   Chanspec: 0x%x\n", ltoh_ua(&e->u.channel_info));
		} else {
			printf("   Support all bands and channels\n");
		}
		printf("   Flags: 0x%04x, Usage pref: %d \n", eflags,
			WL_AVAIL_ENTRY_USAGE_VAL(eflags));
	}
	if (e->bitmap_len) {
		printf("   Start offset: %d TU | Period: %d TU | Bit dur: %d TU\n",
			16 * (ltoh_ua(&e->start_offset)),
			(e->period ? 128 << (e->period - 1) : 0),
			16 << (WL_AVAIL_ENTRY_BIT_DUR_VAL(ltoh_ua(&e->flags))));
		printf("   Time bitmap (b0b1...): ");
		for (j = 0; j < e->bitmap_len; j++) {
			if ((uint8*)(e->bitmap + j)) {
				tmp = *((uint8*)(e->bitmap + j));
				for (k = 0; k < NBBY; k++) {
					printf("%d", (tmp & 1) ? 1 : 0);
					tmp >>= 1;
				}
			}
		}
		printf("\n");
	} else {
		printf("   No time bitmap: availabile on all time slots\n");
	}
}

static void
wl_nan_print_advertisers(nan_adv_table_t *pnan_adv_tbl)
{
	nan_adv_entry_t	*pnan_adv = NULL;
	uint8 num_adv = pnan_adv_tbl->num_adv;

	printf(" table entries: %u\n", num_adv);
	pnan_adv = pnan_adv_tbl->adv_nodes;
	while (num_adv > 0) {
		printf("----------------------------\n");
		printf("> Address    : %02x:%02x:%02x:%02x:%02x:%02x\n",
			pnan_adv->addr.octet[0],
			pnan_adv->addr.octet[1],
			pnan_adv->addr.octet[2],
			pnan_adv->addr.octet[3],
			pnan_adv->addr.octet[4],
			pnan_adv->addr.octet[5]);
		bcm_ether_ntoa(&pnan_adv->cluster_id, buf);
		printf("> Cluster ID: %s\n", buf);

		prhex("> amr:", pnan_adv->amr, NAN_MASTER_RANK_LEN);
		printf("> hop_count:%d\n", pnan_adv->hop_count);
		printf("> ambtt:%d\n", pnan_adv->ambtt);

		printf("> age: %d\n", pnan_adv->age);

		printf("> Rx channel: 0x%04x\n", pnan_adv->channel);
		printf("> ltsf_h, ltsf_l: 0x%08x 0x%08x\n", pnan_adv->ltsf_h, pnan_adv->ltsf_l);
		printf("> rtsf_h, rtsf_l: 0x%08x 0x%08x\n", pnan_adv->rtsf_h, pnan_adv->rtsf_l);
#ifdef NEW_NAV_WLIOCTL
		printf("> 2G rssi: avg_rssi: %d, last_rssi:%d\n",
			pnan_adv->rssi[0], pnan_adv->last_rssi[0]);
		printf("> 5G rssi: avg_rssi: %d, last_rssi:%d\n",
			pnan_adv->rssi[1], pnan_adv->last_rssi[1]);
#endif
		printf("-----------------------------\n");

		num_adv--;
		pnan_adv++;
	}

	return;
}


/*
 *  a cbfn function, displays bcm_xtlv variables rcvd in get ioctl's xtlv buffer.
 *  it can process GET result for all nan commands, provided that
 *  XTLV types (AKA the explicit xtlv types) packed into the ioctl buff
 *  are unique across all nan ioctl commands
 */
static int
wlu_nan_resp_iovars_cbfn(void *ctx, const uint8 *data, uint16 type, uint16 len)
{
	int res = BCME_OK;
	bcm_iov_batch_buf_t *b_resp = (bcm_iov_batch_buf_t *)ctx;
	uint8	uv8;
	int32 status;
	uint16 cmd_rsp_len;
	char *cmd_name;

	/* if all tlvs are parsed, we should not be here */
	if (b_resp->count == 0) {
		return BCME_BADLEN;
	}

	/*  cbfn params may be used in f/w */
	if (len < sizeof(status)) {
		return BCME_BUFTOOSHORT;
	}

	/* first 4 bytes consists status */
	status = dtoh32(*(uint32 *)data);

	data = data + sizeof(status);
	cmd_rsp_len = len - sizeof(status);

	/* If status is non zero */
	if (status != BCME_OK) {
		if ((cmd_name = get_nan_cmd_name(type)) == NULL) {
			printf("Undefined command type %04x len %04x\n", type, len);
		} else {
			printf("%s failed, status: %04x\n", cmd_name, status);
		}
		return status;
	}

	if (!cmd_rsp_len) {
		if (b_resp->is_set) {
			/* Set cmd resp may have only status, so len might be zero.
			 * just decrement batch resp count
			 */
			goto counter;
		}
		/* Every response for get command expects some data,
		 * return error if there is no data
		 */
		return BCME_ERROR;
	}

	/* TODO: could use more length checks in processing data */
	switch (type) {
	case WL_NAN_CMD_CFG_NAN_ENAB:
	{
		uv8 = *data;
		printf("> nan: %s\n", uv8?"enabled":"disabled");
		break;
	}
	case WL_NAN_CMD_CFG_NAN_INIT:
	{
		uv8 = *data;
		printf("NAN init status : %s\n", uv8?"initialized":"un-initialized");
		break;
	}
	case WL_NAN_CMD_SYNC_SOCIAL_CHAN:
	{
		wl_nan_social_channels_t *sc = (wl_nan_social_channels_t *)data;
		printf("2G Channel : %d\n", sc->soc_chan_2g);
		printf("5G Channel : %d\n", sc->soc_chan_5g);
		break;
	}
	case WL_NAN_CMD_SYNC_AWAKE_DWS:
	{
		wl_nan_awake_dws_t *d = (wl_nan_awake_dws_t *)data;
		printf("2G DW Awake Period : %d\n", d->dw_interval_2g);
		printf("5G DW Awake Period : %d\n", d->dw_interval_5g);
		break;
	}
	case WL_NAN_CMD_SYNC_BCN_RSSI_NOTIF_THRESHOLD:
	{
		wl_nan_rssi_notif_thld_t *th = (wl_nan_rssi_notif_thld_t *)data;
		printf("2G Beacon Notif RSSI Threshold : %d\n", th->bcn_rssi_2g);
		printf("5G Beacon Notif RSSI Threshold : %d\n", th->bcn_rssi_5g);
		break;
	}
	case WL_NAN_CMD_ELECTION_RSSI_THRESHOLD:
	{
		wl_nan_rssi_thld_t *t = (wl_nan_rssi_thld_t *)data;
		printf("2G Sync Beacon RSSI Close	: %d\n", t->rssi_close_2g);
		printf("2G Sync Beacon RSSI Mid		: %d\n", t->rssi_mid_2g);
		printf("5G Sync Beacon RSSI Close	: %d\n", t->rssi_close_5g);
		printf("5G Sync Beacon RSSI Mid		: %d\n", t->rssi_mid_5g);
		break;
	}
	case WL_NAN_CMD_DATA_MAX_PEERS:
	{
		wl_nan_max_peers_t *tp = (wl_nan_max_peers_t *)data;
		printf("Max Peers : %d\n", *(uint8 *)tp);
		break;
	}

	case WL_NAN_CMD_DATA_DATAREQ:
	{
		wl_nan_dp_req_ret_t *dpreq_ret = (wl_nan_dp_req_ret_t *)data;

		printf("> ndp_id: %d\n", (dpreq_ret->ndp_id));
		bcm_ether_ntoa(&dpreq_ret->indi, buf);
		printf("> ndi: %s\n", buf);
		break;
	}
	case WL_NAN_CMD_DATA_DATARESP:
	{
		wl_nan_dp_resp_ret_t *dpresp_ret = (wl_nan_dp_resp_ret_t *)data;

		printf("> nmsgid: %d\n", (dpresp_ret->nmsgid));
		break;
	}
	case WL_NAN_CMD_RANGE_REQUEST:
	{
		wl_nan_range_id *range_id = (wl_nan_range_id *)data;
		printf("> range_id %d\n", *range_id);
		break;
	}
	case WL_NAN_CMD_ELECTION_MERGE:
	{
		uv8 = *data;

		printf("> merge: %s\n", uv8?"enabled":"disabled");
		break;
	}
	case WL_NAN_CMD_CFG_HOP_LIMIT:
	{
		uv8 = *data;

		printf("> hop limit: %d\n", uv8);
		break;
	}
	case WL_NAN_CMD_CFG_OUI:
	{
		wl_nan_oui_type_t *nan_oui_type = (wl_nan_oui_type_t *)data;

		printf("> NAN OUI, type: \n");
		printf("\tOUI %02X-%02X-%02X type %02X\n", nan_oui_type->nan_oui[0],
				nan_oui_type->nan_oui[1], nan_oui_type->nan_oui[2],
				nan_oui_type->type);
		break;
	}
	case WL_NAN_CMD_CFG_CLEARCOUNT:
	{
		wl_nan_count_t *ncount = (wl_nan_count_t *)data;

		printf("> NAN Counter:");
		printf("\tnan_bcn_tx %d", dtoh32(ncount->cnt_bcn_tx));
		printf("\tnan_bcn_rx %d", dtoh32(ncount->cnt_bcn_rx));
		printf("\tnan_svc_disc_tx %d",
				dtoh32(ncount->cnt_svc_disc_tx));
		printf("\tnan_svc_disc_rx %d\n",
				dtoh32(ncount->cnt_svc_disc_rx));
		break;
	}
	case WL_NAN_CMD_ELECTION_METRICS_STATE:
	{
		wl_nan_election_metric_state_t *metric_state;
		metric_state = (wl_nan_election_metric_state_t *)data;

		printf("> Election metrics:\n");
		printf("\tmaster preference:%d\n", metric_state->master_pref);
		printf("\trandom factor:%d\n", metric_state->random_factor);
		break;
	}
	case WL_NAN_CMD_DBG_DUMP:
	{
		wl_nan_dbg_dump_rsp_t *dump_rsp = (wl_nan_dbg_dump_rsp_t *)data;

		if (dump_rsp->dump_type == WL_NAN_DBG_DT_STATS_DATA) {
			wl_nan_stats_t *nan_stats = &dump_rsp->u.nan_stats;

			printf("DWSlots: %u \t",
					dtoh32(nan_stats->cnt_dw));
			printf("DiscBcnSlots: %u\t",
					dtoh32(nan_stats->cnt_disc_bcn_sch));
			printf("AnchorMasterRecExp: %u\t",
					dtoh32(nan_stats->cnt_amr_exp));
			printf("BcnTpltUpd: %u\n",
					dtoh32(nan_stats->cnt_bcn_upd));
			printf("BcnTx: %u\t",
					dtoh32(nan_stats->cnt_bcn_tx));
			printf("SyncBcnTx: %u\t",
					dtoh32(nan_stats->cnt_sync_bcn_tx));
			printf("DiscBcnTx: %u\t",
					dtoh32(nan_stats->cnt_disc_bcn_tx));
			printf("BcnRx: %u\t",
					dtoh32(nan_stats->cnt_bcn_rx));
			printf("SyncBcnRx: %u\t",
					dtoh32(nan_stats->cnt_sync_bcn_rx));
			printf("DiscBcnRx: %u\n",
					dtoh32(nan_stats->cnt_bcn_rx) -
					dtoh32(nan_stats->cnt_sync_bcn_rx));
			printf("SdfTx -> BcMc: %u  Ucast: %u UcFail: %u\t",
					dtoh32(nan_stats->cnt_sdftx_bcmc),
					dtoh32(nan_stats->cnt_sdftx_uc),
					dtoh32(nan_stats->cnt_sdftx_fail));
			printf("SDFRx: %u\n",
					dtoh32(nan_stats->cnt_sdf_rx));
			printf("Roles -> AnchorMaster: %u\t",
					dtoh32(nan_stats->cnt_am));
			printf("Master: %u\t",
					dtoh32(nan_stats->cnt_master));
			printf("NonMasterSync: %u\t",
					dtoh32(nan_stats->cnt_nms));
			printf("NonMasterNonSync: %u\n",
					dtoh32(nan_stats->cnt_nmns));
			printf("UnschTx: %u\t",
					dtoh32(nan_stats->cnt_err_unsch_tx));
			printf("BcnTxErr: %u\t",
					dtoh32(nan_stats->cnt_err_bcn_tx));
			printf("SyncBcnTxMiss: %u\t",
					dtoh32(nan_stats->cnt_sync_bcn_tx_miss));
			printf("SyncBcnTxtimeErr: %u\n",
					dtoh32(nan_stats->cnt_err_txtime));
			printf("MschRegErr: %u\t",
					dtoh32(nan_stats->cnt_err_msch_reg));
			printf("WrongChCB: %u\t",
					dtoh32(nan_stats->cnt_err_wrong_ch_cb));
			printf("DWEarlyCB: %u\t",
					dtoh32(nan_stats->cnt_dw_start_early));
			printf("DWLateCB: %u\t",
					dtoh32(nan_stats->cnt_dw_start_late));
			printf("DWSkips: %u\t", dtoh16(nan_stats->cnt_dw_skip));
			printf("DiscBcnSkips: %u\n",
					dtoh16(nan_stats->cnt_disc_skip));
			printf("MrgScan: %u\t",
					dtoh32(nan_stats->cnt_mrg_scan));
			printf("MrgScanRej: %u\t",
					dtoh32(nan_stats->cnt_err_ms_rej));
			printf("JoinScanRej: %u\t",
					dtoh32(nan_stats->cnt_join_scan_rej));
			printf("NanScanAbort: %u\t",
					dtoh32(nan_stats->cnt_nan_scan_abort));
			printf("ScanRes: %u\t",
					dtoh32(nan_stats->cnt_scan_results));
			printf("NanEnab: %u\t",
					dtoh32(nan_stats->cnt_nan_enab));
			printf("NanDisab: %u\n",
					dtoh32(nan_stats->cnt_nan_disab));
		} else if (dump_rsp->dump_type == WL_NAN_DBG_DT_RSSI_DATA) {
			wl_nan_peer_rssi_data_t *prssi = &dump_rsp->u.peer_rssi;

			print_peer_rssi(prssi);
		}
	}
	break;
	case WL_NAN_CMD_ELECTION_HOST_ENABLE:
	{
		uv8 = *data;

		printf("> nan host: %s\n", uv8?"enabled":"disabled");
		break;
	}
	case WL_NAN_CMD_DBG_LEVEL:
	{
		wl_nan_dbg_level_t *dbg_level = (wl_nan_dbg_level_t *)data;

		printf("> nan_err_level : 0x%x\n> nan_dbg_level : 0x%x\n> nan_info_level : 0x%x\n ",
			dtoh32(dbg_level->nan_err_level), dtoh32(dbg_level->nan_dbg_level),
			dtoh32(dbg_level->nan_info_level));
		break;
	}
	case WL_NAN_CMD_CFG_ROLE:
	{
		wl_nan_role_cfg_t* r = (wl_nan_role_cfg_t*)data;

		printf("> nan config role/state : %d\n", r->cfg_role);
		printf("> nan current role/state : %d\n", r->cur_role);
		bcm_ether_ntoa(&r->target_master.addr, buf);
		printf("> target master: %s\n", buf);
		break;
	}
	case WL_NAN_CMD_CFG_HOP_CNT:
	{
		uv8 = *data;

		printf("> nan hop_cnt : %d\n", uv8);
		break;
	}
	case WL_NAN_CMD_CFG_WARMUP_TIME:
	{
		printf("> nan warm up time : %d\n", dtoh16(*(uint16*)data));
		break;
	}
	case WL_NAN_CMD_CFG_COUNT:
	{
		wl_nan_count_t *ncount = (wl_nan_count_t *)data;

		printf("> NAN Counter:");
		printf("\tnan_bcn_tx %d", dtoh32(ncount->cnt_bcn_tx));
		printf("\tnan_bcn_rx %d", dtoh32(ncount->cnt_bcn_rx));
		printf("\tnan_svc_disc_tx %d",
				dtoh32(ncount->cnt_svc_disc_tx));
		printf("\tnan_svc_disc_rx %d\n",
				dtoh32(ncount->cnt_svc_disc_rx));

		break;
	}
	case WL_NAN_CMD_CFG_STATUS:
	{
		wl_nan_conf_status_t *nstatus = (wl_nan_conf_status_t *)data;
		wl_nan_print_status(nstatus);
		break;
	}
	case WL_NAN_CMD_ELECTION_ADVERTISERS:
	{
		nan_adv_table_t *nadvtbl = (nan_adv_table_t *)data;

		wl_nan_print_advertisers(nadvtbl);
		break;
	}

	case WL_NAN_CMD_CFG_CID:
	{
		wl_nan_cluster_id_t *clid = (wl_nan_cluster_id_t *)data;
		prhex("> Cluster-id:", (uint8 *)clid, ETHER_ADDR_LEN);
	}
	break;
	case WL_NAN_CMD_CFG_IF_ADDR:
	{
		struct ether_addr *ea;

		ea = (struct ether_addr *)data;
		bcm_ether_ntoa(ea, buf);
		printf("> if_addr: %s\n", buf);
	}
	break;
	case WL_NAN_CMD_CFG_BCN_INTERVAL:
	{
		wl_nan_disc_bcn_interval_t bcn_interval;
		bcn_interval = dtoh16(*(uint16 *)data);
		printf("BCN interval = %d\n", bcn_interval);
	}
	break;
	case WL_NAN_CMD_CFG_SDF_TXTIME:
	{
		wl_nan_svc_disc_txtime_t sdf_txtime;
		sdf_txtime = dtoh16(*(uint16 *)data);
		printf("SDF TXTIME = %d\n", sdf_txtime);
	}
	break;
	case WL_NAN_CMD_CFG_SID_BEACON:
	{
		wl_nan_sid_beacon_control_t *sid_bcn;
		sid_bcn = (wl_nan_sid_beacon_control_t *)data;

		printf("> SID beacon ctrl info:\n");
		printf("\tsid: %s", sid_bcn->sid_enable?"enabled":"disabled");
		printf("\tsid limit count: %d\n", sid_bcn->sid_count);
	}
	break;
	case WL_NAN_CMD_CFG_AVAIL:
	{
		wl_avail_t *a = (wl_avail_t*)data;
		wl_avail_entry_t *e;
		uint8* p = (uint8*)a->entry;
		uint16 flags = dtoh16(a->flags);
		int i;
		if (!a->length) {
			break;
		}
		printf("\nType (1=local, 2=peer, 3=ndc, 4=immutable, 5=response, "
			"6=counter, 7=ranging): %d\n", flags);
		if (flags == WL_AVAIL_NDC) {
			prhex("ndc id", (uint8 *)&a->addr, ETHER_ADDR_LEN);
		}
		for (i = 0; i < a->num_entries; i++) {
			e = (wl_avail_entry_t*)p;
			wlu_nan_print_wl_avail_entry(flags & WL_AVAIL_TYPE_MASK, e);
			p += e->length;
		}
		break;
	}
	case WL_NAN_CMD_CFG_WFA_TM:
	{
		wl_nan_wfa_testmode_t flags;

		flags = dtoh32(*(uint32 *)data);
		printf("flag: 0x%08x\n", flags);

		if (flags & WL_NAN_WFA_TM_IGNORE_TERMINATE_NAF) {
			printf("\t0x00000001 ignore NDP terminate AF\n");
		}
		if (flags & WL_NAN_WFA_TM_IGNORE_RX_DATA_OUTSIDE_CRB) {
			printf("\t0x00000002 ignore rx'ed data frame outside NDL CRBs\n");
		}
		if (flags & WL_NAN_WFA_TM_ALLOW_TX_DATA_OUTSIDE_CRB) {
			printf("\t0x00000004 allow tx data frame outside NDL CRBs\n");
		}
		if (flags & WL_NAN_WFA_TM_ENFORCE_NDL_COUNTER) {
			printf("\t0x00000008 enforce NDL counter proposal\n");
		}
	}
	break;
	case WL_NAN_CMD_SD_PARAMS:
	case WL_NAN_CMD_SD_PUBLISH:
	case WL_NAN_CMD_SD_SUBSCRIBE:
	{
		wl_nan_sd_params_t *sd_params = (wl_nan_sd_params_t *)data;
		uint16 flags;

		printf(">Instance ID: %d\n", sd_params->instance_id);
		printf(">Length: %d\n", dtoh16(sd_params->length));
		prhex(">Service Hash:", sd_params->svc_hash,
				WL_NAN_SVC_HASH_LEN);
		flags = dtoh16(sd_params->flags);
		printf(">Flags: 0x%04x\n", flags);
		if (flags & WL_NAN_PUB_UNSOLICIT) {
			printf(" unsolicited/active\n");
		}
		if (flags & WL_NAN_PUB_SOLICIT) {
			printf(" solicited\n");
		}
		if (!(flags & WL_NAN_PUB_UNSOLICIT) &&
				!(flags & WL_NAN_PUB_SOLICIT)) {
			printf(" passive\n");
		}
		if (flags & WL_NAN_FOLLOWUP) {
			printf(" follow-up\n");
		}
		printf("\n");
		printf(">RSSI: %d\n", sd_params->proximity_rssi);
		printf(">TTL: 0x%08x\n", dtoh32(sd_params->ttl));
		printf(">Period: %d\n", sd_params->period);
		break;
	}
	case WL_NAN_CMD_SD_PUBLISH_LIST:
	case WL_NAN_CMD_SD_SUBSCRIBE_LIST:
	{
		wl_nan_service_list_t *svc_list = (wl_nan_service_list_t *)data;
		wl_nan_service_info_t *svc_info = NULL;
		int i = 0;

		printf("%s\n",
				(type == WL_NAN_CMD_SD_PUBLISH_LIST) ? ">Publish List" :
				">Subscribe List");
		printf(">Count: %d\n", svc_list->id_count);
		for (i = 0; i < svc_list->id_count; i++) {
			svc_info = &svc_list->list[i];
			printf(">Instance ID: %d\n", svc_info->instance_id);
			prhex(">Service Hash:", svc_info->service_hash,
					WL_NAN_SVC_HASH_LEN);
			printf("\n");
		}
		break;
	}
	case WL_NAN_CMD_CFG_EVENT_MASK:
	{
		wl_nan_event_mask_t mask = ntoh32(*((wl_nan_event_mask_t *)(data)));
		printf("> event_mask: 0x%08x\n", mask);
		printf("   %s:%d\n  %s:%d\n  %s:%d\n  %s:%d\n  %s:%d\n"
			"  %s:%d\n  %s:%d\n  %s:%d\n  %s:%d\n  %s:%d\n"
			"  %s:%d\n  %s:%d\n  %s:%d\n  %s:%d\n  %s:%d\n"
			"  %s:%d\n  %s:%d\n  %s:%d\n  %s:%d\n  %s:%d\n"
			"  %s:%d\n",
			"WL_NAN_EVENT_START",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_START),
			"WL_NAN_EVENT_JOIN",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_JOIN),
			"WL_NAN_EVENT_ROLE",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_ROLE),
			"WL_NAN_EVENT_SCAN_COMPLETE",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_SCAN_COMPLETE),
			"WL_NAN_EVENT_DISCOVERY_RESULT",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_DISCOVERY_RESULT),

			"WL_NAN_EVENT_REPLIED",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_REPLIED),
			"WL_NAN_EVENT_TERMINATED",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_TERMINATED),
			"WL_NAN_EVENT_RECEIVE",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_RECEIVE),
			"WL_NAN_EVENT_STATUS_CHG",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_STATUS_CHG),
			"WL_NAN_EVENT_MERGE",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_MERGE),

			"WL_NAN_EVENT_STOP",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_STOP),
			"WL_NAN_EVENT_P2P",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_P2P),
			"WL_NAN_EVENT_PEER_DATAPATH_IND",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_PEER_DATAPATH_IND),
			"WL_NAN_EVENT_DATAPATH_ESTB",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_DATAPATH_ESTB),
			"WL_NAN_EVENT_DATAPATH_END",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_DATAPATH_END),

			"WL_NAN_EVENT_BCN_RX",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_BCN_RX),
			"WL_NAN_EVENT_PEER_DATAPATH_RESP",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_PEER_DATAPATH_RESP),
			"WL_NAN_EVENT_PEER_DATAPATH_CONF",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_PEER_DATAPATH_CONF),
			"WL_NAN_EVENT_RNG_REQ_IND",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_RNG_REQ_IND),
			"WL_NAN_EVENT_RNG_RPT_IN",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_RNG_RPT_IND),

			"WL_NAN_EVENT_RNG_TERM_IND",
			IS_NAN_EVT_ON(mask, WL_NAN_EVENT_RNG_TERM_IND));
	}
	break;
	case WL_NAN_CMD_DBG_DISC_RESULTS:
	{
		wl_nan_disc_results_list_t *p;
		int i;
		char hash[WL_NAN_SVC_HASH_LEN+1];

		p = (wl_nan_disc_results_list_t *)data;
		printf(">Disc results: \n\n");
		for (i = 0; i < WL_NAN_MAX_DISC_RESULTS; i++) {

			memcpy(hash,
					p->disc_result[i].svc_hash,
					WL_NAN_SVC_HASH_LEN);

			hash[WL_NAN_SVC_HASH_LEN] = '\0';

			bcm_ether_ntoa(
					(struct ether_addr *)
					&p->disc_result[i].peer_mac,
					buf);

			printf(">Instance id: 0x%x \n"
					">Peer instance id: 0x%x \n",
					p->disc_result[i].instance_id,
					p->disc_result[i].peer_instance_id);
			prhex(">Service hash", (uchar *)hash,
					WL_NAN_SVC_HASH_LEN);
			printf(">Peer MAC: %s \n\n", buf);
		}
	}
	break;
	case WL_NAN_CMD_DBG_STATS:
	{
		wl_nan_stats_t *nan_stats =
			(wl_nan_stats_t *)data;
		printf("DWSlots: %u \t",
				dtoh32(nan_stats->cnt_dw));
		printf("DiscBcnSlots: %u\t",
				dtoh32(nan_stats->cnt_disc_bcn_sch));
		printf("AnchorMasterRecExp: %u\t",
				dtoh32(nan_stats->cnt_amr_exp));
		printf("BcnTpltUpd: %u\n",
				dtoh32(nan_stats->cnt_bcn_upd));
		printf("BcnTx: %u\t",
				dtoh32(nan_stats->cnt_bcn_tx));
		printf("SyncBcnTx: %u\t",
				dtoh32(nan_stats->cnt_sync_bcn_tx));
		printf("DiscBcnTx: %u\t",
				dtoh32(nan_stats->cnt_disc_bcn_tx));
		printf("BcnRx: %u\t",
				dtoh32(nan_stats->cnt_bcn_rx));
		printf("SyncBcnRx: %u\t",
				dtoh32(nan_stats->cnt_sync_bcn_rx));
		printf("DiscBcnRx: %u\n",
				dtoh32(nan_stats->cnt_bcn_rx) -
				dtoh32(nan_stats->cnt_sync_bcn_rx));
		printf("SdfTx -> BcMc: %u  Ucast: %u UcFail: %u\t",
				dtoh32(nan_stats->cnt_sdftx_bcmc),
				dtoh32(nan_stats->cnt_sdftx_uc),
				dtoh32(nan_stats->cnt_sdftx_fail));
		printf("SDFRx: %u\n",
				dtoh32(nan_stats->cnt_sdf_rx));
		printf("Roles -> AnchorMaster: %u\t",
				dtoh32(nan_stats->cnt_am));
		printf("Master: %u\t",
				dtoh32(nan_stats->cnt_master));
		printf("NonMasterSync: %u\t",
				dtoh32(nan_stats->cnt_nms));
		printf("NonMasterNonSync: %u\n",
				dtoh32(nan_stats->cnt_nmns));
		printf("UnschTx: %u\t",
				dtoh32(nan_stats->cnt_err_unsch_tx));
		printf("BcnTxErr: %u\t",
				dtoh32(nan_stats->cnt_err_bcn_tx));
		printf("SyncBcnTxMiss: %u\t",
				dtoh32(nan_stats->cnt_sync_bcn_tx_miss));
		printf("SyncBcnTxtimeErr: %u\n",
				dtoh32(nan_stats->cnt_err_txtime));
		printf("MschRegErr: %u\t",
				dtoh32(nan_stats->cnt_err_msch_reg));
		printf("WrongChCB: %u\t",
				dtoh32(nan_stats->cnt_err_wrong_ch_cb));
		printf("DWEarlyCB: %u\t",
				dtoh32(nan_stats->cnt_dw_start_early));
		printf("DWLateCB: %u\t",
				dtoh32(nan_stats->cnt_dw_start_late));
		printf("DWSkips: %u\t", dtoh16(nan_stats->cnt_dw_skip));
		printf("DiscBcnSkips: %u\n",
				dtoh16(nan_stats->cnt_disc_skip));
		printf("MrgScan: %u\t",
				dtoh32(nan_stats->cnt_mrg_scan));
		printf("MrgScanRej: %u\t",
				dtoh32(nan_stats->cnt_err_ms_rej));
		printf("JoinScanRej: %u\t",
				dtoh32(nan_stats->cnt_join_scan_rej));
		printf("NanScanAbort: %u\t",
				dtoh32(nan_stats->cnt_nan_scan_abort));
		printf("ScanRes: %u\t",
				dtoh32(nan_stats->cnt_scan_results));
		printf("NanEnab: %u\t",
				dtoh32(nan_stats->cnt_nan_enab));
		printf("NanDisab: %u\n",
				dtoh32(nan_stats->cnt_nan_disab));
	}
	break;

	case WL_NAN_CMD_DATA_NDP_SHOW:
	{
		int i = 0;
		wl_nan_ndp_id_list_t *id_list;
		id_list = (wl_nan_ndp_id_list_t *)data;

		if (id_list == NULL) {
			return BCME_ERROR;
		}

		for (; i < id_list->ndp_count; i++) {
			printf("%d \t", id_list->lndp_id[i]);
		}
		printf("\n");
	}
	break;

	case WL_NAN_CMD_DATA_CONFIG:
	{
		wl_nan_ndp_config_t *config = (wl_nan_ndp_config_t *)data;
		int i = 0;
		printf("ndp_id: %d\tpub_id: %d\ttid: %d\tpkt_size: %d\t"
				"mean_rate: %d\tsvc_interval: %d\n",
				config->ndp_id, config->pub_id,
				config->qos.tid, config->qos.pkt_size,
				config->qos.mean_rate,
				config->qos.svc_interval);


		bcm_ether_ntoa(&config->pub_addr, buf);
		printf("pub_addr: %s\n", buf);

		bcm_ether_ntoa(&config->data_addr, buf);
		printf("data_addr: %s\n", buf);

		for (; i < WL_NAN_DATA_SVC_SPEC_INFO_LEN; i++) {
			putchar(config->svc_spec_info[i]);
		}
		printf("\n");

	}
	break;
	case WL_NAN_CMD_DATA_AUTOCONN:
	{
		printf("autoconn: %d\n", *data);
		if (*data & WL_NAN_AUTO_DPRESP) {
			printf("auto_dpresp enabled \n");
		} else {
			printf("auto_dpresp disabled \n");
		}
		if (*data & WL_NAN_AUTO_DPCONF) {
			printf("auto_dpconf enabled \n");
		} else {
			printf("auto_dpconf disabled \n");
		}
	}
	break;
	case WL_NAN_CMD_DATA_SCHEDUPD:
	break;
	case WL_NAN_CMD_DATA_CAP:
	break;
	case WL_NAN_CMD_DATA_STATUS:
	{
		wl_nan_ndp_status_t *status = (wl_nan_ndp_status_t *)data;

		prhex("PEER_NMI", (uint8 *)&status->peer_nmi, ETHER_ADDR_LEN);
		prhex("PEER_NDI", (uint8 *)&status->peer_ndi, ETHER_ADDR_LEN);
		printf("lndp_id = %d\n", status->session.lndp_id);
		printf("state = %d\n", status->session.state);
		printf("pub_id = %d\n", status->session.pub_id);
	}
	break;
	case WL_NAN_CMD_DATA_STATS:
	break;

	default:
	res = BCME_ERROR;
	break;
	}

counter:
	if (b_resp->count > 0) {
		b_resp->count--;
	}

	if (!b_resp->count) {
		res = BCME_IOV_LAST_CMD;
	}

	return res;
}

/*
* listens on bcm events on given interface and prints NAN_EVENT data
* event data is a var size array of bcm_xtlv_t items
*/
static int
wl_nan_print_event_data_tlvs(void *ctx, const uint8 *data, uint16 type, uint16 len)
{
	int err = BCME_OK;

	UNUSED_PARAMETER(ctx);

	switch (type) {
	case WL_NAN_XTLV_SD_SVC_INFO:
	{
		prhex("SVC info", (uint8 *)data, len);
		break;
	}
	case WL_NAN_XTLV_SD_SDF_RX:
	{
		prhex("SDF RX", (uint8 *)data, len);
		break;
	}
	default:
	{
		printf("Unknown xtlv type received: %x\n", type);
		err = BCME_ERROR;
		break;
	}
	}

	return err;
}

static int
wl_nan_print_event_data(uint32 nan_evtnum, uint8 *event_data, uint16 data_len)
{
	uint16 tlvs_offset, opt_tlvs_len = 0;
	uint8 *tlv_buf;
	int err = BCME_OK;

	switch (nan_evtnum) {
	case WL_NAN_EVENT_START:
		/* intentional fall through */
	case WL_NAN_EVENT_JOIN:
		/* intentional fall through */
	case WL_NAN_EVENT_ROLE:
		/* intentional fall through */
	case WL_NAN_EVENT_SCAN_COMPLETE:
		/* intentional fall through */
	case WL_NAN_EVENT_MERGE:
		/* intentional fall through */
	case WL_NAN_EVENT_STOP:
	{
		bcm_xtlv_t *xtlv = (bcm_xtlv_t *)event_data;
		wl_nan_conf_status_t *nstatus = (wl_nan_conf_status_t *)xtlv->data;
		wl_nan_print_status(nstatus);
		break;
	}
	case WL_NAN_EVENT_DISCOVERY_RESULT:
	{
		bcm_xtlv_t *xtlv = (bcm_xtlv_t *)event_data;
		wl_nan_event_disc_result_t *e = (wl_nan_event_disc_result_t *)xtlv->data;

		printf("Publish ID: %d\n", e->pub_id);
		printf("Subscribe ID: %d\n", e->sub_id);
		bcm_ether_ntoa(&e->pub_mac, buf);
		printf("Publish MAC addr: %s\n", buf);
		printf("Publish rssi: %d\n", e->publish_rssi);
		printf("attr_num: %d\n", e->attr_num);
		printf("attr_list_len: %d\n", e->attr_list_len);
		break;
	}
	case WL_NAN_EVENT_REPLIED:
	{
		bcm_xtlv_t *xtlv = (bcm_xtlv_t *)event_data;
		wl_nan_event_replied_t *evr =
			(wl_nan_event_replied_t *)xtlv->data;
		printf("Publish ID: %d\n", evr->pub_id);
		bcm_ether_ntoa(&evr->sub_mac, buf);
		printf("Subscriber MAC addr: %s\n", buf);
		printf("Subscriber RSSI : %d\n", evr->sub_rssi);
		printf("Subscriber ID : %d\n", evr->sub_id);
		printf("attr_num: %d\n", evr->attr_num);
		printf("attr_list_len: %d\n", evr->attr_list_len);
		break;
	}
	case WL_NAN_EVENT_TERMINATED:
	{
		bcm_xtlv_t *xtlv = (bcm_xtlv_t *)event_data;
		wl_nan_ev_terminated_t *pev = (wl_nan_ev_terminated_t *)xtlv->data;
		printf("Instance ID: %d\n", pev->instance_id);
		printf("Reason: %d\n", pev->reason);
		printf("Service Type: %d\n", pev->svctype);
		break;
	}
	case WL_NAN_EVENT_RECEIVE:
	{
		bcm_xtlv_t *xtlv = (bcm_xtlv_t *)event_data;
		wl_nan_ev_receive_t *ev = (wl_nan_ev_receive_t *)xtlv->data;
		printf("Local ID: %d\n", ev->local_id);
		printf("Remote ID: %d\n", ev->remote_id);
		bcm_ether_ntoa(&ev->remote_addr, buf);
		printf("Peer MAC addr: %s\n", buf);
		printf("Peer RSSI : %d\n", ev->fup_rssi);
		printf("attr_num: %d\n", ev->attr_num);
		printf("attr_list_len : %d\n", ev->attr_list_len);
		break;
	}
	case WL_NAN_EVENT_STATUS_CHG:
		break;
	case WL_NAN_EVENT_P2P:
	{
		wl_nan_ev_p2p_avail_t *ev_p2p_avail;
		chanspec_t chanspec;
		ev_p2p_avail = (wl_nan_ev_p2p_avail_t *)event_data;

		printf("Device Role: %d\n", ev_p2p_avail->dev_role);

		bcm_ether_ntoa(&ev_p2p_avail->sender, buf);
		printf("Sender MAC addr: %s\n", buf);

		bcm_ether_ntoa(&ev_p2p_avail->p2p_dev_addr, buf);
		printf("P2P dev addr: %s\n", buf);

		printf("Repeat: %d\n", ev_p2p_avail->repeat);
		printf("Resolution: %d\n", ev_p2p_avail->resolution);
		chanspec = dtoh16(ev_p2p_avail->chanspec);
		if (wf_chspec_valid(chanspec)) {
			wf_chspec_ntoa(chanspec, buf);
			printf("> Chanspec:%s 0x%x\n", buf, chanspec);
		} else {
			printf("> Chanspec: invalid  0x%x\n", chanspec);
		}
		printf("Avail bitmap: 0x%08x\n", dtoh32(ev_p2p_avail->avail_bmap));

		break;
	}
	case WL_NAN_EVENT_PEER_DATAPATH_IND:
	case WL_NAN_EVENT_PEER_DATAPATH_RESP:
	case WL_NAN_EVENT_PEER_DATAPATH_CONF:
	case WL_NAN_EVENT_DATAPATH_ESTB:
	{
		wl_nan_ev_datapath_cmn_t *event;

		event = (wl_nan_ev_datapath_cmn_t *)event_data;
		printf("Event type = %d\n", event->type);
		printf("Status = %d\n", event->status);
		printf("pub_id = %d\n", event->pub_id);
		printf("security = %d\n", event->security);
		if (event->type == NAN_DP_SESSION_UNICAST) {
			printf("NDPID = %d\n", event->ndp_id);
			prhex("INITIATOR_NDI", (uint8 *)&event->initiator_ndi,
					ETHER_ADDR_LEN);
			prhex("RESPONDOR_NDI", (uint8 *)&event->responder_ndi,
					ETHER_ADDR_LEN);
		} else {
			printf("NDPID = %d\n", event->mc_id);
			prhex("INITIATOR_NDI", (uint8 *)&event->initiator_ndi,
					ETHER_ADDR_LEN);
		}
		tlvs_offset = OFFSETOF(wl_nan_ev_datapath_cmn_t, opt_tlvs);
		opt_tlvs_len = data_len - tlvs_offset;

	}
	break;
	case WL_NAN_EVENT_DATAPATH_END:
		break;
	case WL_NAN_EVENT_SDF_RX:
	{
		tlvs_offset = 0;
		opt_tlvs_len = data_len;
		break;
	}
	case WL_NAN_EVENT_BCN_RX:
	{

		bcm_xtlv_t *xtlv = (bcm_xtlv_t *)event_data;
		int ctr = 0;
		printf("Len	: %d\n", xtlv->len);
		printf("Beacon Payload \n");
		for (; ctr < xtlv->len; ctr++) {
			printf("%02X ", xtlv->data[ctr]);
			if ((ctr + 1) % 8  == 0)
				printf("\n");
		}

	}
	break;
	default:
	{
		printf("WARNING: unimplemented NAN APP EVENT code:%d\n", nan_evtnum);
		err = BCME_ERROR;
	}
	break;
	}

	if (opt_tlvs_len) {
		tlv_buf = event_data + tlvs_offset;

		/* Extract event data tlvs and print their resp in cb fn */
		err = bcm_unpack_xtlv_buf((void *)&nan_evtnum,
			(const uint8 *)tlv_buf,
			opt_tlvs_len, BCM_IOV_CMD_OPT_ALIGN32,
			wl_nan_print_event_data_tlvs);
	}

	return err;
}

static int
wl_nan_subcmd_event_check(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int fd, err, octets;
	struct sockaddr_ll sll;
	struct ifreq ifr;
	char ifnames[IFNAMSIZ] = {"eth0"};
	bcm_event_t *event;
	uint32 nan_evtnum, evt_status;
	char* data;
	int event_type;
	void *nandata;

	UNUSED_PARAMETER(nandata);
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(is_set);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);
	UNUSED_PARAMETER(argc);
	if (argv[0] == NULL) {
		printf("<ifname> param is missing\n");
		return BCME_USAGE_ERROR;
	}
	strncpy(ifnames, *argv, (IFNAMSIZ - 1));
	printf("ifname:%s\n", ifnames);
	bzero(&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, ifnames, (IFNAMSIZ - 1));
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
		evt_status = ntoh32(event->event.status);
		nan_evtnum = ntoh32(event->event.reason);
		datalen = ntoh32(event->event.datalen);
		printf("Event Type : %d Event Status : %d nan_evtnum : %d datalen : %d\n",
			event_type, evt_status, nan_evtnum, datalen);
		if ((event_type != WLC_E_NAN_NON_CRITICAL) &&
			(event_type != WLC_E_NAN_CRITICAL)) {
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
			case WL_NAN_EVENT_P2P:
				printf("WL_NAN_EVENT_P2P:\n");
				break;
			case WL_NAN_EVENT_PEER_DATAPATH_IND:
				printf("WL_NAN_EVENT_PEER_DATAPATH_IND:\n");
				break;
			case WL_NAN_EVENT_PEER_DATAPATH_RESP:
				printf("WL_NAN_EVENT_PEER_DATAPATH_RESP:\n");
				break;
			case WL_NAN_EVENT_PEER_DATAPATH_CONF:
				printf("WL_NAN_EVENT_PEER_DATAPATH_CONF:\n");
				break;
			case WL_NAN_EVENT_DATAPATH_ESTB:
				printf("WL_NAN_EVENT_DATAPATH_ESTB:\n");
				break;
			case WL_NAN_EVENT_DATAPATH_END:
				printf("WL_NAN_EVENT_DATAPATH_END:\n");
				break;
			case WL_NAN_EVENT_SDF_RX:
				printf("WL_NAN_EVENT_SDF_RX:\n");
				break;
			case WL_NAN_EVENT_BCN_RX:
				printf("WL_NAN_EVENT_BCN_RX:\n");
				break;
			default:
				printf("WARNING: unimplemented NAN APP EVENT code:%d\n",
					nan_evtnum);
				err = BCME_ERROR;
				break;
		}
		if (evt_status)
			printf("Event status:-%u\n", evt_status);

		if (err == BCME_OK) {
			err = wl_nan_print_event_data(nan_evtnum, (uint8 *)nandata, datalen);
		}
	}
	free(data);
exit1:
	close(fd);
	return (err);
}

static int
wl_nan_process_resp_buf(void *iov_resp, uint16 max_len, uint8 is_set)
{
	int res = BCME_UNSUPPORTED;
	uint16 version;
	uint16 tlvs_len;

	/* Check for version */
	version = dtoh16(*(uint16 *)iov_resp);
	if (version & BCM_IOV_BATCH_MASK) {
		bcm_iov_batch_buf_t *p_resp = (bcm_iov_batch_buf_t *)iov_resp;
		if (!p_resp->count) {
			res = BCME_RANGE;
			goto done;
		}
		p_resp->is_set = is_set;
		/* number of tlvs count */
		tlvs_len = max_len - OFFSETOF(bcm_iov_batch_buf_t, cmds[0]);

		/* Extract the tlvs and print their resp in cb fn */
		res = bcm_unpack_xtlv_buf((void *)p_resp, (const uint8 *)&p_resp->cmds[0],
			tlvs_len, BCM_IOV_CMD_OPT_ALIGN32, wlu_nan_resp_iovars_cbfn);

		if (res == BCME_IOV_LAST_CMD) {
			res = BCME_OK;
		}
	}
	/* else non-batch not supported */
done:
	return res;
}

/*
 *   --- common for all nan get commands ----
 */
int
wl_nan_do_ioctl(void *wl, void *nanioc, uint16 iocsz, uint8 is_set)
{
	/* for gets we only need to pass ioc header */
	uint8 *iocresp = NULL;
	uint8 *resp = NULL;
	char *iov = "nan";
	int max_resp_len = WLC_IOCTL_MAXLEN;
	int res;

	if ((iocresp = malloc(max_resp_len)) == NULL) {
		printf("Failed to malloc %d bytes \n",
				WLC_IOCTL_MAXLEN);
		return BCME_NOMEM;
	}

	if (is_set) {
		int iov_len = strlen(iov) + 1;

		/*  send setbuf nan iovar */
		res = wlu_iovar_setbuf(wl, iov, nanioc, iocsz, iocresp, max_resp_len);
		/* iov string is not received in set command resp buf */
		resp = &iocresp[iov_len];
	} else {
		/*  send getbuf nan iovar */
		res = wlu_iovar_getbuf(wl, iov, nanioc, iocsz, iocresp, max_resp_len);
		resp = iocresp;
	}

	/*  check the response buff  */
	if ((res == BCME_OK) && (iocresp != NULL)) {
		res = wl_nan_process_resp_buf(resp, max_resp_len, is_set);
	}

	free(iocresp);
	return res;
}


/* nan event_msgs:  enable/disable nan events  */
static int
wl_nan_subcmd_event_msgs(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	bool show_help = FALSE;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	/* bit num = event enum -1 */
	const char usage[] = "nan event_msgs [bit] [0/1]\n"
		"	bit 0	- WL_NAN_EVENT_START\n"
		"	bit 1	- WL_NAN_EVENT_JOIN\n"
		"	bit 2	- WL_NAN_EVENT_ROLE\n"
		"	bit 3	- WL_NAN_EVENT_SCAN_COMPLETE\n"
		"	bit 4	- WL_NAN_EVENT_DISCOVERY_RESULT\n"
		"	bit 5	- WL_NAN_EVENT_REPLIED\n"
		"	bit 6	- WL_NAN_EVENT_TERMINATED\n"
		"	bit 7	- WL_NAN_EVENT_RECEIVE\n"
		"	bit 8	- WL_NAN_EVENT_STATUS_CHG\n"
		"	bit 9	- WL_NAN_EVENT_MERGE\n"
		"	bit 10 - WL_NAN_EVENT_STOP\n"
		"	bit 11 - WL_NAN_EVENT_P2P\n"
		"	bit 16 - WL_NAN_EVENT_POST_DISC\n"
		"	bit 17 - WL_NAN_EVENT_DATA_IF_ADD\n"
		"	bit 18 - WL_NAN_EVENT_DATA_PEER_ADD\n"
		"	bit 19 - WL_NAN_EVENT_PEER_DATAPATH_IND\n"
		"	bit 20 - WL_NAN_EVENT_DATAPATH_ESTB\n"
		"	bit 21 - WL_NAN_EVENT_DATA_SDF_RX\n"
		"	bit 22 - WL_NAN_EVENT_DATAPATH_END\n"
		"	bit 23 - WL_NAN_EVENT_BCN_RX\n"
		"	bit 24 - WL_NAN_EVENT_PEER_DATAPATH_RESP\n"
		"	bit 25 - WL_NAN_EVENT_PEER_DATAPATH_CONF\n"
		"	bit 26 - WL_NAN_EVENT_RNG_REQ_IND\n"
		"	bit 27 - WL_NAN_EVENT_RNG_RPT_IND\n"
		"	bit 28 - WL_NAN_EVENT_RNG_TERM_IND\n"
		"	bit 29 ..30 - unused\n"
		"	bit 31 -  set/clr all eventmask bits\n";


	if (*argv == NULL) {
		/* get: handled by cbfn */
		*is_set = FALSE;
		goto exit;
	} else {
		/*  args are present, do set ioctl  */
		uint8 *pxtlv = iov_data;
		uint32 eventmask, val = 0;
		char *ptr = NULL;

		/* User asking for help */
		if (argv[0][0] == '-') {
			show_help = TRUE;
			res = BCME_BADARG;
			goto exit;
		}
		/* in hex format */
		else if (!(ptr = strstr(*argv, "0x"))) {
			show_help = TRUE;
			res = BCME_BADARG;
		} else {
			char *val_p, *endptr;
			val_p = *argv;
			val = strtoul(val_p, &endptr, 0);
			eventmask = ntoh32(val);
			memcpy(pxtlv, &eventmask, sizeof(eventmask));
			*avail_len -= sizeof(eventmask);
		}
	}
exit:
	if (show_help)
		fprintf(stderr, "%s\n", usage);
	return res;
}

/*
*  ********  various debug features, for internal use only ********
*/
/*
*  ********  get nan discovery results ********
*/
static int
wl_nan_subcmd_disc_results(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	if (*argv == NULL || argc == 0) {
		*is_set = FALSE;
	} else if (!*argv) {
		res = BCME_USAGE_ERROR;
	}

	return res;
}

static int
wl_nan_subcmd_dbg_rssi(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_USAGE_ERROR;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);
	UNUSED_PARAMETER(is_set);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);
	UNUSED_PARAMETER(argc);

	return res;
}

static void
wl_nan_help_usage()
{
	printf("%s\n", NAN_PARAMS_USAGE);
}

static void
wl_nan_help(uint16 type)
{
	int i;
	const wl_nan_cmd_help_t *p_help_info = &nan_cmd_help_list[0];

	if (type == NAN_HELP_ALL) {
		wl_nan_help_usage();
		return;
	}

	for (i = 0; i < (int) ARRAYSIZE(nan_cmd_help_list); i++) {
		if (p_help_info->type == type) {
			if (p_help_info->help_handler) {
				(p_help_info->help_handler)();
				break;
			}
		}
		p_help_info++;
	}

	return;
}

/*  ********  enable/disable feature  ************** */
static int
wl_nan_subcmd_cfg_enable(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{

	int res = BCME_OK;
	uint8 enable = 0;
	uint16 len;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(enable);

		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}
		enable = (uint8)atoi(*argv);
		memcpy(iov_data, &enable, sizeof(enable));
	}
	*avail_len -= len;
	return res;
}

static int
wl_nan_subcmd_cfg_role(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint16 len;
	wl_nan_role_t role;
	wl_nan_role_cfg_t *rcfg;
	struct ether_addr master = ether_null;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(*rcfg);
		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			res = BCME_BUFTOOSHORT;
			goto exit;
		}
		role = atoi(*argv++);
		if (role > WL_NAN_ROLE_ANCHOR_MASTER) {
			printf("%s : Invalid role\n", __FUNCTION__);
			res = BCME_BADOPTION;
			goto exit;
		}

		if (!*argv) {
			if (role != WL_NAN_ROLE_AUTO && role != WL_NAN_ROLE_ANCHOR_MASTER) {
				printf("%s : Missing target master address.\n", __FUNCTION__);
				res = BCME_USAGE_ERROR;
				goto exit;
			}
		} else {
			if (!wl_ether_atoe(*argv, &master)) {
				printf("%s: Invalid ether addr provided\n", __FUNCTION__);
				res = BCME_USAGE_ERROR;
				goto exit;
			}
		}

		rcfg = (wl_nan_role_cfg_t *)iov_data;
		rcfg->cfg_role = role;
		memcpy(&rcfg->target_master.addr, &master, ETHER_ADDR_LEN);

	}
	*avail_len -= len;

exit:
	return res;
}

static int
wl_nan_subcmd_cfg_init(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;

	uint16 len = sizeof(uint8);
	wl_nan_init_t *nan_init = NULL;
	uint8 val = 0;


	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		if (len > *avail_len) {
			printf("Buffer short, requested:%d, available:%d\n",
					len, *avail_len);
			res = BCME_BUFTOOSHORT;
			goto exit;
		}
		val = atoi(*argv++);
		if ((val != 0) && (val != 1)) {
			printf("%s : Invalid value\n", __FUNCTION__);
			res = BCME_BADOPTION;
			goto exit;
		}
		nan_init = (wl_nan_init_t *)iov_data;
		*nan_init = val;
	}
	*avail_len -= len;
exit:
	return res;
}

static int
wl_nan_subcmd_cfg_hop_count(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{

	int res = BCME_OK;
	uint16 len;
	wl_nan_hop_count_t hop_cnt;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(is_set);

	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(hop_cnt);
		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}
		hop_cnt = (uint8)atoi(*argv);
		memcpy(iov_data, (uint8 *)&hop_cnt, sizeof(hop_cnt));
	}

	*avail_len -= len;
	return res;
}

static int
wl_nan_subcmd_cfg_hop_limit(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	wl_nan_hop_count_t hop_limit;
	uint16 len;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(hop_limit);

		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		hop_limit = (uint8)atoi(*argv);
		memcpy(iov_data, &hop_limit, sizeof(hop_limit));
	}

	*avail_len -= len;

	return res;
}

static int
wl_nan_subcmd_cfg_warmup_time(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint16 len;
	wl_nan_warmup_time_ticks_t wup_ticks;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(is_set);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(wup_ticks);
		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}
		wup_ticks = (uint32)atoi(*argv);
		memcpy(iov_data, (uint8 *)&wup_ticks, sizeof(wup_ticks));
	}

	*avail_len -= len;
	return res;
}

static int
wl_nan_subcmd_cfg_count(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
	} else {
		printf("config_count doesn't expect any parameters\n");
		return BCME_ERROR;
	}

	return res;
}

static int
wl_nan_subcmd_cfg_clearcount(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	/* Its Set only */
	if ((*argv == NULL) || argc == 0) {
		*is_set = TRUE;
	} else {
		printf("clearcount cmd doesn't accept any parameters\n");
		res = BCME_USAGE_ERROR;
	}

	return res;
}

static int
wl_nan_subcmd_cfg_channel(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	chanspec_t chspec;
	uint16 len;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	*is_set = TRUE;
	len = sizeof(chspec);
	if (len > *avail_len) {
		printf("Buf short, requested:%d, available:%d\n",
				len, *avail_len);
		return BCME_BUFTOOSHORT;
	}

	if (*argv) {
		chspec = wf_chspec_aton(*argv);
	} else {
		return BCME_USAGE_ERROR;
	}

	if (chspec != 0) {
		memcpy(iov_data, (uint8 *)&chspec, sizeof(chspec));
	}
	*avail_len -= len;

	return res;
}

static int
wl_nan_subcmd_cfg_band(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	wl_nan_band_t nan_band;
	uint16 len;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	*is_set = TRUE;
	len = sizeof(nan_band);
	if (len > *avail_len) {
		printf("Buf short, requested:%d, available:%d\n",
				len, *avail_len);
		return BCME_BUFTOOSHORT;
	}

	if (*argv) {
		nan_band = set_nan_band(*argv);
		if (nan_band == NAN_BAND_INVALID) {
			return BCME_USAGE_ERROR;
		}

	} else {
		return BCME_USAGE_ERROR;
	}

	memcpy(iov_data, (uint8*)&nan_band, sizeof(nan_band));
	*avail_len -= len;

	return res;
}

static int
wl_nan_subcmd_cfg_cid(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	wl_nan_cluster_id_t cid;
	uint16 len;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(cid);
		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		if (!wl_ether_atoe(*argv, &cid)) {
			printf("Malformed MAC address parameter\n");
			return BCME_USAGE_ERROR;
		}

		memcpy(iov_data, (uint8*)&cid, sizeof(cid));
	}
	*avail_len -= len;

	return res;
}

static int
wl_nan_subcmd_cfg_if_addr(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	struct ether_addr if_addr;
	uint16 len;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if ((*argv == NULL) || (!strcmp(*argv, WL_IOV_BATCH_DELIMITER))) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(if_addr);
		if (len > *avail_len) {
			printf("%s: Buf short, requested:%d, available:%d\n",
					__FUNCTION__, len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		if (!wl_ether_atoe(*argv, &if_addr)) {
			printf("Malformed MAC address parameter\n");
			return BCME_USAGE_ERROR;
		}

		memcpy(iov_data, (uint8*)&if_addr, sizeof(if_addr));
	}
	*avail_len -= len;

	return res;
}

static int
wl_nan_subcmd_cfg_bcn_interval(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint16 len;
	wl_nan_disc_bcn_interval_t bcn_intvl;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if ((*argv == NULL) || (!strcmp(*argv, WL_IOV_BATCH_DELIMITER))) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(bcn_intvl);
		bcn_intvl = htod16(atoi(*argv));
		if (len > *avail_len) {
			printf("%s: Buf short, requested:%d, available:%d\n",
					__FUNCTION__, len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		memcpy(iov_data, (uint8*)&bcn_intvl, sizeof(bcn_intvl));
	}
	*avail_len -= len;

	return res;
}

static int
wl_nan_subcmd_cfg_sdf_txtime(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint16 len;
	wl_nan_svc_disc_txtime_t sdf_txtime;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if ((*argv == NULL) || (!strcmp(*argv, WL_IOV_BATCH_DELIMITER))) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(sdf_txtime);
		sdf_txtime = htod16(atoi(*argv));
		if (len > *avail_len) {
			printf("%s: Buf short, requested:%d, available:%d\n",
					__FUNCTION__, len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		memcpy(iov_data, (uint8*)&sdf_txtime, sizeof(sdf_txtime));
	}
	*avail_len -= len;

	return res;
}

static int
wl_nan_subcmd_cfg_sid_beacon(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint16 len;
	wl_nan_sid_beacon_control_t *sid_beacon;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if ((*argv == NULL) || (!strcmp(*argv, WL_IOV_BATCH_DELIMITER))) {
		*is_set = FALSE;
		len = 0;
	} else {
		if (ARGCNT(argv) < WL_NAN_CMD_CFG_SID_BCN_ARGC) {
			printf(" Must mention sid_enable and sid_count\n");
			return BCME_USAGE_ERROR;
		}

		*is_set = TRUE;
		len = sizeof(*sid_beacon);

		sid_beacon = (wl_nan_sid_beacon_control_t *)iov_data;
		sid_beacon->sid_enable = atoi(*argv);
		argv++;
		sid_beacon->sid_count = atoi(*argv);

		*avail_len -= len;
	}

	return res;
}

#define AWAKE_DW_BANDOPTION_LEN		4
enum {
	NAN_AWAKE_DW_INTERVAL = 1,
	NAN_AWAKE_DW_BAND = 2
};

typedef struct nan_awake_dw_config_param {
	char *name;	/* <param-name> string to identify the config item */
	uint16 id;
	uint16 len; /* len in bytes */
	char *help_msg; /* Help message */
} nan_awake_dw_config_param_t;

static const nan_awake_dw_config_param_t nan_awake_dw_param[] = {
	/* param-name	param_id				len		help message */
	{ "band", NAN_AWAKE_DW_BAND, AWAKE_DW_BANDOPTION_LEN,
	"Band value can be either 'a', or 'b' " },
	{ "interval", NAN_AWAKE_DW_INTERVAL, sizeof(uint8),
	"DW interval assumes 1,2,4,8,16 values" },
};

const nan_awake_dw_config_param_t *
nan_lookup_awake_dw_config_param(char *param_name)
{
	int i = 0;
	const nan_awake_dw_config_param_t *param_p = &nan_awake_dw_param[0];

	for (i = 0; i < (int)ARRAYSIZE(nan_awake_dw_param); i++) {
		if (stricmp(param_p->name, param_name) == 0) {
			return param_p;
		}
		param_p++;
	}

	return NULL;
}

/*
*  ********  get NAN election commands ********
*/
static int
wl_nan_subcmd_election_host_enable(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	wl_nan_host_enable_t enable;
	uint16 len;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(enable);

		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		enable = (uint8)atoi(*argv);
		memcpy(iov_data, (uint8 *)&enable, sizeof(enable));
		/* advance to the next param */
	}

	*avail_len -= len;

	return res;
}

static int
wl_nan_subcmd_election_metrics_config(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint16 len;
	wl_nan_election_metric_config_t *metrics;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	*is_set = TRUE;
	if (*argv == NULL) {
		printf(" Must mention random factor and master pref\n");
		return BCME_USAGE_ERROR;
	}

	if (ARGCNT(argv) < WL_NAN_CMD_CFG_ELECTION_METRICS_ARGC) {
		return BCME_USAGE_ERROR;
	}

	len = sizeof(wl_nan_election_metric_config_t);
	if (len > *avail_len) {
		printf("Buf short, requested:%d, available:%d\n",
				len, *avail_len);
		return BCME_BUFTOOSHORT;
	}

	metrics = (wl_nan_election_metric_config_t *)iov_data;
	metrics->random_factor = (uint8)atoi(*argv);
	argv++;
	metrics->master_pref = (uint8)atoi(*argv);

	*avail_len -= len;

	return res;
}

static int
wl_nan_subcmd_election_metrics_state(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
	} else {
		printf("metrics state doesn't expect any parameters\n");
		return BCME_USAGE_ERROR;
	}

	return res;
}

/*
*  ********  get NAN status info ********
*/
static int
wl_nan_subcmd_cfg_status(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
	} else {
		printf("config_status doesn't expect any parameters\n");
		return BCME_USAGE_ERROR;
	}

	return res;
}

static int
wl_nan_subcmd_leave(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	wl_nan_cluster_id_t cid;
	uint16 len;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	*is_set = TRUE;
	len = sizeof(cid);
	if (len > *avail_len) {
		printf("Buf short, requested:%d, available:%d\n",
				len, *avail_len);
		return BCME_BUFTOOSHORT;
	}

	/* For stop, only parameter cluster ID is an optional param */
	if ((*argv == NULL) || argc == 0) {
		/* Send null ether address as cid */
		memcpy(&cid, &ether_null, len);
	} else {
		if (!wl_ether_atoe(*argv, &cid)) {
			printf("Malformed MAC address parameter\n");
			return BCME_USAGE_ERROR;
		}
	}

	memcpy(iov_data, &cid, len);

	*avail_len -= len;

	return res;
}

/* Convert a service name to a SHA256 hash.  Not re-entrant. */
static int
wl_nan_service_name_to_hash(char *name, size_t hash_len, uint8 *out_hash)
{
	char *p;
	size_t name_len = 0;
	SHA256_CTX ctx;
	unsigned char sha_key[SHA256_CBLOCK];

	if (!name)
		return BCME_BADARG;

	/* Convert the name to lower case. NOTE: this modifies the name parameter. */
	for (p = name; *p; p++)
	{
		*p = tolower((int)*p);
		name_len++;
	}
	/* Generate a SHA256 hash from the name */
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, name, name_len);
	SHA256_Final(sha_key, &ctx);

	/* The resulting sha_key has length SHA256_DIGEST_LENGTH (32) */
	memcpy(out_hash, sha_key, hash_len);
	return BCME_OK;
}

enum {
	NAN_SD_PARAM_LENGTH = 0x0000,
	NAN_SD_PARAM_FLAGS = 0x0001,
	NAN_SD_PARAM_SVC_HASH = 0x0002,
	NAN_SD_PARAM_INST_ID = 0x0003,
	NAN_SD_PARAM_RSSI = 0x0004,
	NAN_SD_PARAM_PERIOD = 0x0005,
	NAN_SD_PARAM_TTL = 0x0006,
	NAN_SD_PARAM_OPTIONS = 0x0007,
	NAN_SD_PARAM_LCL_SVC_ID = 0x0008,
	NAN_SD_PARAM_REQ_SVC_ID = 0x0009,
	NAN_SD_PARAM_PRIORITY = 0x000A,
	NAN_SD_PARAM_MAC_INCLUDE = 0x000B,
	NAN_SD_PARAM_TOKEN = 0x000C,
	NAN_SD_PARAM_SVC_INFO_LEN = 0x000D,
	NAN_SD_PARAM_SVC_INFO = 0x000E,
	NAN_SD_PARAM_SVC_BLOOM = 0x000F,
	NAN_SD_PARAM_SVC_MATCH_TX = 0x0010,
	NAN_SD_PARAM_SVC_MATCH_RX = 0x0011,
	NAN_SD_PARAM_SVC_SRF_RAW = 0x0012,
	NAN_SD_PARAM_SVC_NAME = 0x0013,
	NAN_SD_PARAM_SVC_MATCH_BOTH = 0x0014,
	NAN_SD_PARAM_MAC_EXCLUDE = 0x0015,
	NAN_SD_PARAM_SVC_BLOOM_IDX = 0x0016,
	NAN_SD_PARAM_SVC_MATCH_RAW = 0x0017,
	NAN_SD_PARAM_SVC_FOLLOWUP = 0x0018,
	NAN_SD_PARAM_SVC_MATCH_RX_RAW = 0x0019,
	NAN_SD_PARAM_DEST_INST_ID = 0x001A,
	NAN_SD_PARAM_DEST_MAC = 0x001B,
	NAN_SD_PARAM_SDE_CONTROL = 0x001C,
	NAN_SD_PARAM_SDE_RANGE_LIMIT = 0x001D
};

typedef struct nan_sd_config_param_info {
	char *name;	/* <param-name> string to identify the config item */
	uint16 id;
	uint16 len; /* len in bytes */
	char *help_msg; /* Help message */
} nan_sd_config_param_info_t;
/*
 * Parameter name and size for service discovery IOVARs
 */
static const nan_sd_config_param_info_t nan_sd_config_param_info[] = {
	/* param-name	param_id				len		help message */
	{ "length",		NAN_SD_PARAM_LENGTH,	sizeof(uint16),
	"Length of SD including options" },
	{ "flags",		NAN_SD_PARAM_FLAGS,		sizeof(uint16),
	"Bitmap representing optional flags" },
	{ "svc-hash",	NAN_SD_PARAM_SVC_HASH,	WL_NAN_SVC_HASH_LEN,
	"6 bytes service hash" },
	{ "id",			NAN_SD_PARAM_INST_ID,	sizeof(uint8),
	"Instance of current service" },
	{ "rssi",		NAN_SD_PARAM_RSSI,		sizeof(int8),
	"RSSI limit to Rx subscribe or pub SDF, 0 no effect" },
	{ "period",		NAN_SD_PARAM_PERIOD,	sizeof(uint8),
	"period of unsolicited SDF xmission in DWs" },
	{ "ttl",		NAN_SD_PARAM_TTL,		sizeof(int32),
	"TTL for this instance or - 1 till cancelled" },
	{ "options",	NAN_SD_PARAM_OPTIONS,	sizeof(uint8),
	"Optional fileds in SDF as appropriate" },
	{ "lcl-svc-id",	NAN_SD_PARAM_LCL_SVC_ID, sizeof(uint8),
	"Sender Service ID" },
	{ "req-svc-id",	NAN_SD_PARAM_REQ_SVC_ID, sizeof(uint8),
	"Destination Service ID" },
	{ "priority",	NAN_SD_PARAM_PRIORITY,	sizeof(uint8),
	"requested relative priority" },
	{ "mac-incl",	NAN_SD_PARAM_MAC_INCLUDE,	ETHER_ADDR_LEN,
	"Adds MAC addr to SRF and sets include bit" },
	{ "token",		NAN_SD_PARAM_TOKEN,		sizeof(uint16),
	"follow_up_token when a follow-up msg is queued successfully" },
	{ "svc-info-len", NAN_SD_PARAM_SVC_INFO_LEN,	sizeof(uint8),
	"size in bytes of the service info payload" },
	{ "svc-info",	NAN_SD_PARAM_SVC_INFO,	sizeof(uint8),	"Service Info payload" },
	{ "bloom", NAN_SD_PARAM_SVC_BLOOM, NAN_BLOOM_LENGTH_DEFAULT,
	"Encode the Service Response Filter using a bloom filter rather than a mac list" },
	{ "match-tx", NAN_SD_PARAM_SVC_MATCH_TX, WL_NAN_MAX_SVC_MATCH_FILTER_LEN,
	"Match filter (hex bytes) to transmit" },
	{ "match-rx", NAN_SD_PARAM_SVC_MATCH_RX, WL_NAN_MAX_SVC_MATCH_FILTER_LEN,
	"Match filter (hex bytes) for receive" },
	{ "match-both", NAN_SD_PARAM_SVC_MATCH_BOTH, WL_NAN_MAX_SVC_MATCH_FILTER_LEN,
	"Match filter (hex bytes) to transmit" },
	{ "srfraw", NAN_SD_PARAM_SVC_SRF_RAW, WL_NAN_MAX_SVC_MATCH_FILTER_LEN,
	"SRF control byte + SRF data" },
	{ "name", NAN_SD_PARAM_SVC_NAME, WL_NAN_MAX_SVC_NAME_LEN, "Service name" },
	{ "mac-excl",	NAN_SD_PARAM_MAC_EXCLUDE,	ETHER_ADDR_LEN,
	"Adds MAC addr to SRF and clears include bit" },
	{ "bloom-idx", NAN_SD_PARAM_SVC_BLOOM_IDX, sizeof(uint8),
	"Bloom hash index used for bloom filter creation" },
	{ "match-raw", NAN_SD_PARAM_SVC_MATCH_RAW, WL_NAN_MAX_SVC_MATCH_FILTER_LEN,
	"Match filter (raw hex bytes) to transmit" },
	{ "followup", NAN_SD_PARAM_SVC_FOLLOWUP, 0, "Follow up" },
	{ "match-rx-raw", NAN_SD_PARAM_SVC_MATCH_RX_RAW, WL_NAN_MAX_SVC_MATCH_FILTER_LEN,
	"Match Rx filter (raw hex bytes) to receive" },
	{ "dest-inst-id", NAN_SD_PARAM_DEST_INST_ID, sizeof(uint8),
	"Destination Instance ID" },
	{ "dest-mac", NAN_SD_PARAM_DEST_MAC, ETHER_ADDR_LEN,
	"Destination MAC address" },
	{ "sde-ctrl", NAN_SD_PARAM_SDE_CONTROL, sizeof(uint16),
	"Service descriptor extension attribute control" },
	{ "sde-range", NAN_SD_PARAM_SDE_RANGE_LIMIT, sizeof(uint32),
	"Service descriptor extension attribute range limit" },
};

const nan_sd_config_param_info_t *
nan_lookup_sd_config_param(char *param_name)
{
	int i = 0;
	const nan_sd_config_param_info_t *param_p = &nan_sd_config_param_info[0];

	for (i = 0; i < (int)ARRAYSIZE(nan_sd_config_param_info); i++) {
		if (stricmp(param_p->name, param_name) == 0) {
			return param_p;
		}
		param_p++;
	}

	return NULL;
}
static int
wl_nan_subcmd_svc(void *wl, const wl_nan_sub_cmd_t *cmd, int instance_id, int argc, char **argv,
	uint8 *iov_data, uint16 *avail_len, uint32 default_flags)
{
	int ret = BCME_OK;
	wl_nan_sd_params_t *params = (wl_nan_sd_params_t *)iov_data;
	char *val_p, *endptr, *svc_info = NULL;
	char *followup = NULL;
	uint32 val = 0;
	uint8 *pxtlv = (uint8 *)&params->optional[0];
	uint16 buflen, buflen_at_start;
	char *hash = NULL, *hash_raw = NULL;

	/* Matching filter variables */
	char *match_raw = NULL, *match_rx_raw = NULL;
	uint8 *match = NULL, *matchtmp = NULL;
	uint8 *match_rx = NULL, *match_rxtmp = NULL;
	uint8 m_len;

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
	uint16 srf_include = 0;
	const nan_sd_config_param_info_t *param_p;
	bool zero_lv_pair = FALSE;

	/* Service discovery extension variables */
	int16 sde_control = -1;
	int32 sde_range_limit = -1;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(argc);

	/* Default options */
	params->period = 1;
	params->ttl = WL_NAN_TTL_UNTIL_CANCEL;
	params->flags = default_flags;

	/* Copy mandatory parameters from command line. */
	if (!*argv) {
		printf("Missing parameters.\n");
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	params->instance_id = instance_id;

	/*
	 * wl nan [publish|subscribe] <instance-id> name <service-name>
	 */
	if (!*argv || (stricmp(*argv, "name") != 0)) {
		printf("Missing service name parameter.\n");
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	argv++;
	hash = *argv;
	argv++;
	buflen = *avail_len;
	buflen_at_start = buflen;
	buflen -= OFFSETOF(wl_nan_sd_params_t, optional[0]);

	/* Parse optional args. Validity is checked by discovery engine */
	while (*argv != NULL) {
		if ((param_p = nan_lookup_sd_config_param(*argv)) == NULL) {
			break;
		}
		/*
		 * Skip param name
		 */
		argv++;
		switch (param_p->id) {
		case NAN_SD_PARAM_SVC_BLOOM:
			if (srfraw_started) {
				printf("Cannot use -bloom-idx with -srfraw\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			srf_started = TRUE;

			if (bloom_len) {
				printf("-bloom can only be set once\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			/* Value is optional */
			if (!*argv) {
				bloom_len = NAN_BLOOM_LENGTH_DEFAULT;
			} else {
				val = strtoul(*argv, &endptr, 0);

				if (*endptr != '\0' || val < 1 || val > 254) {
					printf("Invalid bloom length.\n");
					ret = BCME_USAGE_ERROR;
					goto exit;
				}
			}
			bloom_len = val;
			break;
		case NAN_SD_PARAM_SVC_MATCH_BOTH:
			if (match_raw) {
				printf("-match has already been used, cannot use -m \n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}

			/* Allocate and initalize matching filter buffer */
			if (!match) {
				if ((match = malloc(WLC_IOCTL_MEDLEN)) == NULL) {
					printf("Failed to malloc %d bytes \n",
						WLC_IOCTL_MEDLEN);
					ret = BCME_NOMEM;
					goto exit;
				}
				matchtmp = match;
			}

			/* If no value is given, then it is a zero length LV pair.
			 * So, checking for arg NULL OR
			 * calling nan_lookup_sd_config_param() which return non-NULL
			 * incase the argument is the next config param.
			 */
			if (!*argv || nan_lookup_sd_config_param(*argv)) {
				zero_lv_pair = TRUE;
				*matchtmp++ = 0;
			} else {
				/* Add LV pair to temporary buffer */
				val_p = *argv;
				m_len = strlen(val_p);
				*matchtmp++ = m_len;
				strncpy((char*)matchtmp, val_p, m_len);
				matchtmp += m_len;
			}
			break;
		case NAN_SD_PARAM_SVC_MATCH_RX:
			if (match_rx_raw) {
				printf("match-rx-raw has already been used, cannot use match-rx\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}

			/* Allocate and initalize matching filter buffer */
			if (!match_rx) {
				if ((match_rx = malloc(WLC_IOCTL_MEDLEN)) == NULL) {
					printf("Failed to malloc %d bytes \n",
						WLC_IOCTL_MEDLEN);
					ret = BCME_NOMEM;
					goto exit;
				}
				match_rxtmp = match_rx;
			}
			/* If no value is given, then it is a zero length LV pair.
			 * So, checking for arg NULL OR
			 * calling nan_lookup_sd_config_param() which return non-NULL
			 * incase the argument is the next config param.
			 */
			if (!*argv || nan_lookup_sd_config_param(*argv)) {
				zero_lv_pair = TRUE;
				*match_rxtmp++ = 0;
			} else {
				/* Add LV pair to temporary buffer */
				val_p = *argv;
				m_len = strlen(val_p);
				*match_rxtmp++ = m_len;
				strncpy((char*)match_rxtmp, val_p, m_len);
				match_rxtmp += m_len;
			}
			break;
		case NAN_SD_PARAM_SVC_MATCH_TX:
			if (match_raw) {
				printf("match-raw has already been used, cannot use match-tx \n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}

			/* Allocate and initalize matching filter buffer */
			if (!match) {
				if ((match = malloc(WLC_IOCTL_MEDLEN)) == NULL) {
					printf("Failed to malloc %d bytes \n",
						WLC_IOCTL_MEDLEN);
					ret = BCME_NOMEM;
					goto exit;
				}
				matchtmp = match;
			}
			/* If no value is given, then it is a zero length LV pair.
			 * So, checking for arg NULL OR
			 * calling nan_lookup_sd_config_param() which return non-NULL
			 * incase the argument is the next config param.
			 */
			if (!*argv || nan_lookup_sd_config_param(*argv)) {
				zero_lv_pair = TRUE;
				*matchtmp++ = 0;
			} else {
				/* Add LV pair to temporary buffer */
				val_p = *argv;
				m_len = strlen(val_p);
				*matchtmp++ = m_len;
				strncpy((char*)matchtmp, val_p, m_len);
				matchtmp += m_len;
			}
			break;
		case NAN_SD_PARAM_MAC_INCLUDE:
		case NAN_SD_PARAM_MAC_EXCLUDE:
			if (srfraw_started) {
				printf("Cannot use -i or -x with -srfraw\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			srf_started = TRUE;

			if (!srf_include) {
				/* mac include or exclude param ID */
				srf_include = param_p->id;
			} else if (srf_include != param_p->id) {
				printf("Cannot use mac-incl or mac-excl together\n");
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
			val_p = *argv;
			/* Add MAC address to temporary buffer */
			if (!wl_ether_atoe(val_p, &srf_mac)) {
				printf("Invalid ether addr\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			memcpy(srftmp, &srf_mac, ETHER_ADDR_LEN);
			srftmp += ETHER_ADDR_LEN;
			mac_num++;
			break;
		case NAN_SD_PARAM_SVC_BLOOM_IDX:
			if (srfraw_started) {
				printf("Cannot use -bloom-idx with -srfraw\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			srf_started = TRUE;

			if (bloom_idx != 0xFFFFFFFF) {
				printf("-bloom-idx can only be set once\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			/* Set bloom index */
			bloom_idx = atoi(*argv);
			if (bloom_idx > 3) {
				printf("Invalid -bloom-idx value\n");
			}
			break;
		case NAN_SD_PARAM_SVC_SRF_RAW:
			/* Send raw hex bytes of SRF control and SRF to firmware */
			/* Temporarily store SRF */
			if (srf_started) {
				printf("srfraw is used without other srf options\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			srf_raw = *argv;
			srfraw_started = TRUE;
			break;
		case NAN_SD_PARAM_SVC_INFO:
			/* Temporarily store service info */
			svc_info = *argv;
			break;
		case NAN_SD_PARAM_SVC_MATCH_RAW:
			/* Send raw hex bytes of matching filter to firmware */
			if (match) {
				printf("-m has already been used, cannot use -match \n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			/* Temporarily store matching filter */
			match_raw = *argv;
			break;
		case NAN_SD_PARAM_SVC_MATCH_RX_RAW:
			/* Send raw hex bytes of matching filter rx to firmware */
			if (match_rx) {
				printf("-m-rx has already been used, cannot use"
				 "-match_rx\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			/* Temporarily store matching filter rx */
			match_rx_raw = *argv;
			break;
		case NAN_SD_PARAM_SVC_FOLLOWUP:
			/* Temporarily store service info for Follow-Up */
			followup = *argv;
			break;
		case NAN_SD_PARAM_SVC_HASH:
			/* Temporarily store raw service ID hash */
			hash_raw = *argv;
			break;
		case NAN_SD_PARAM_FLAGS:
			val_p = *argv;
			val = strtoul(val_p, &endptr, 0);
			if (*endptr != '\0') {
				printf("Value is not uint or hex %s\n",
					val_p);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}

			params->flags = val;
			break;
		case NAN_SD_PARAM_PERIOD:
			val_p = *argv;
			val = strtoul(val_p, &endptr, 0);
			if (*endptr != '\0') {
				printf("Value is not uint or hex %s\n",
					val_p);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			params->period = val;
			break;
		case NAN_SD_PARAM_TTL:
			val_p = *argv;
			val = strtoul(val_p, &endptr, 0);
			if (*endptr != '\0') {
				printf("Value is not uint or hex %s\n",
					val_p);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			params->ttl = val;
			break;
		case NAN_SD_PARAM_RSSI:
			params->proximity_rssi = atoi(*argv);
			break;
		case NAN_SD_PARAM_LENGTH:
			params->length = atoi(*argv);
			break;
		case NAN_SD_PARAM_SDE_CONTROL:
			if (cmd->id != WL_NAN_CMD_SD_PUBLISH) {
				printf("SDE options are for publish only\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			sde_control = atoi(*argv);
			break;
		case NAN_SD_PARAM_SDE_RANGE_LIMIT:
			if (cmd->id != WL_NAN_CMD_SD_PUBLISH) {
				printf("SDE options are for publish only\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			sde_range_limit = atoi(*argv);
			break;
		default:
			printf("Unrecognized parameter %s\n",
				param_p->name);
			ret = BCME_USAGE_ERROR;
			goto exit;
			/* TODO add matching filters, service specific info */
		}

		if (zero_lv_pair) {
			zero_lv_pair = FALSE;
		} else {
			argv++;
		}
	}

	/* If service ID raw value was not provided, calculate from name */
	if (!hash_raw) {
		wl_nan_service_name_to_hash(hash, WL_NAN_SVC_HASH_LEN,
			(uint8*)params->svc_hash);
	} else {
		 /* 6-byte Service ID (hash) value */
		if (strlen(hash_raw)/2 != WL_NAN_SVC_HASH_LEN) {
			printf("Need 6 byte service id\n");
			ret = BCME_USAGE_ERROR;
			goto exit;
		} else {
			if (get_ie_data((uchar*)hash_raw, params->svc_hash,
				WL_NAN_SVC_HASH_LEN)) {
				ret = BCME_BADARG;
				goto exit;
			}
		}
	}

	/* Optional parameters */
	/* TODO other optional params */
	if (svc_info) {
		ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv,
			&buflen, WL_NAN_XTLV_SD_SVC_INFO, svc_info);
		if (ret != BCME_OK)
		goto exit;
	}
	if (match_raw) {
		ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv,
			&buflen, WL_NAN_XTLV_CFG_MATCH_TX, match_raw);
		if (ret != BCME_OK)
			goto exit;
	}
	if (match_rx_raw) {
		/* Create XTLV only if matching filter rx is unique from tx */
		if (!match_raw || (match_raw && strcmp(match_raw, match_rx_raw))) {
			ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv,
				&buflen, WL_NAN_XTLV_CFG_MATCH_RX, match_rx_raw);
			if (ret != BCME_OK)
				goto exit;
		}
	}
	if (followup) {
		ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv,
			&buflen, WL_NAN_XTLV_SD_FOLLOWUP, followup);
		if (ret != BCME_OK)
			goto exit;
	}
	if (match) {
		m_len = matchtmp - match;
		ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_CFG_MATCH_TX,
			m_len, match, BCM_XTLV_OPTION_ALIGN32);
		if (ret != BCME_OK) goto exit;
	}
	if (match_rx) {
		m_len = match_rxtmp - match_rx;
		ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_CFG_MATCH_RX,
			m_len, match_rx, BCM_XTLV_OPTION_ALIGN32);
		if (ret != BCME_OK) goto exit;
	}
	/* Check that there is a list of MAC addresses */
	if (mac_list) {
		uint8 srf_control = 0;
		/* Construct SRF control field */
		if (bloom_len) {
			/* This is a bloom filter */
			srf_control = 1;
			if (bloom_idx == 0xFFFFFFFF) {
				bloom_idx = params->instance_id % 4;
			}
			srf_control |= bloom_idx << 2;
		}
		if (srf_include == NAN_SD_PARAM_MAC_INCLUDE) {
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

			ret = bcm_pack_xtlv_entry(&pxtlv, &buflen,
				WL_NAN_XTLV_CFG_SR_FILTER, (mac_num * ETHER_ADDR_LEN + 1),
				(uint8 *)srf, BCM_XTLV_OPTION_ALIGN32);
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

			ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_CFG_SR_FILTER,
				(bloom_len + 1), (uint8 *)srf, BCM_XTLV_OPTION_ALIGN32);
			if (ret != BCME_OK)
				goto exit;
		}
	}
	if (srf_raw) {
		ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv,
			&buflen, WL_NAN_XTLV_CFG_SR_FILTER, srf_raw);
		if (ret != BCME_OK)
			goto exit;
	}
	if (sde_control >= 0) {
		uint16 tmp = (uint16)sde_control;
		ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_SD_SDE_CONTROL,
			sizeof(uint16), (uint8*)&tmp, BCM_XTLV_OPTION_ALIGN32);
		if (ret != BCME_OK)
			goto exit;
	}
	if (sde_range_limit >= 0) {
		uint32 tmp = (uint32)sde_range_limit;
		ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_NAN_XTLV_SD_SDE_RANGE_LIMIT,
			sizeof(uint32), (uint8*)&tmp, BCM_XTLV_OPTION_ALIGN32);
		if (ret != BCME_OK)
			goto exit;
	}
	/* adjust avail_len to the end of last data record */
	*avail_len -= (buflen_at_start - buflen);

exit:
	if (srf)
		free(srf);
	if (mac_list)
		free(mac_list);
	if (match)
		free(match);
	if (match_rx)
		free(match_rx);
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

/*
 * Service Discovery commands
 */
#define NAN_SD_PARAMS_FLAGS_RANGE_CLOSE	((1 << 6))
#define NAN_SD_PARAMS_FLAGS_RANGE_LIMIT (~(1 << 6))
#define NAN_SD_PARAMS_FLAGS_UNSOLICITED ((1 << 12))
#define NAN_SD_PARAMS_FLAGS_SOLICITED	((1 << 13))
#define NAN_SD_PARAMS_FLAGS_UCAST		(~(1 << 14))
#define NAN_SD_PARAMS_FLAGS_BCAST		((1 << 14))
#define NAN_SD_PARAMS_DEFAULT_PERIOD	1
#define NAN_SD_PARAMS_DEFAULT_TTL		0xFFFFFFFF
static int
wl_nan_subcmd_publish(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint16 len = 0;
	int int_val = 0;

	UNUSED_PARAMETER(is_set);

	if (*argv == NULL) {
		res = BCME_USAGE_ERROR;
		goto done;
	}

	int_val = atoi(*argv);
	/* Instance ID must be from 1 to 255 */
	if ((res = wl_nan_is_instance_valid(int_val)) != WL_NAN_E_OK) {
		printf("Invalid instance_id.\n");
		goto done;
	}
	argv++;
	argc--;
	if (*argv == NULL || argc == 0) {
		memcpy(iov_data, &int_val, sizeof(int_val));
		*avail_len -= sizeof(int_val);
		/* GET command support for publish */
		*is_set = FALSE;
		goto done;
	} else {
		len = sizeof(wl_nan_sd_params_t);

		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			res = BCME_BUFTOOSHORT;
			goto done;
		}

		res = wl_nan_subcmd_svc(wl, cmd, int_val, argc, argv, iov_data,
			avail_len, WL_NAN_PUB_BOTH);
	}
done:
	return res;
}

static int
wl_nan_subcmd_publish_list(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	if (*argv == NULL || argc == 0) {
		*is_set = FALSE;
	} else if (!*argv) {
		printf("publish_list: requires no parameters\n");
		res = BCME_USAGE_ERROR;
	}

	return res;
}

static int
wl_nan_subcmd_subscribe(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	int int_val = 0;
	uint16 len = 0;

	UNUSED_PARAMETER(is_set);

	if (*argv == NULL) {
		res = BCME_USAGE_ERROR;
		goto done;
	}

	int_val = atoi(*argv);
	/* Instance ID must be from 1 to 255 */
	if ((res = wl_nan_is_instance_valid(int_val)) != WL_NAN_E_OK) {
		printf("Invalid instance_id.\n");
		*is_set = FALSE;
		goto done;
	}
	argv++;
	argc--;
	if (*argv == NULL || argc == 0) { /* get  */
		memcpy(iov_data, &int_val, sizeof(int_val));
		*avail_len -= sizeof(int_val);
		*is_set = FALSE;
		goto done;
	} else {
		len = sizeof(wl_nan_sd_params_t);

		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			res = BCME_BUFTOOSHORT;
			goto done;
		}

		res = wl_nan_subcmd_svc(wl, cmd, int_val, argc, argv, iov_data,
			avail_len, 0);
	}

done:
	return res;
}

/*
 * Returns active subscribe list instance
 */
static int
wl_nan_subcmd_subscribe_list(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	if (*argv == NULL || argc == 0) {
		*is_set = FALSE;
	} else if (*argv) {
		printf("subscribe: requires no parameters\n");
		res = BCME_USAGE_ERROR;
	}

	return res;
}

static int
wl_nan_subcmd_cancel(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int ret = BCME_OK;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(is_set);
	UNUSED_PARAMETER(argc);
	int int_val = 0;
	wl_nan_instance_id_t instance_id;
	uint16 buflen, buflen_at_start;

	if (!argv[0] || argv[1]) {
		printf("Incorrect number of parameters.\n");
		ret = BCME_USAGE_ERROR;
		goto exit;

	}
	int_val = atoi(argv[0]);
	/* instance id '0' means cancel all publish/subscribe.
	 * So, allowing instance ID '0' also for cancel IOVARs.
	 */
	if (int_val && ((ret = wl_nan_is_instance_valid(int_val)) != WL_NAN_E_OK)) {
		printf("Invalid instance id.\n");
		goto exit;
	}

	instance_id = int_val;

	/* max data we can write, it will be decremented as we pack */
	buflen = *avail_len;
	buflen_at_start = buflen;

	/* Mandatory parameters */
	memcpy(iov_data, &instance_id, sizeof(instance_id));
	buflen -= sizeof(instance_id);

	/* adjust iocsz to the end of last data record */
	*avail_len -= (buflen_at_start - buflen);


exit:
	return ret;
}

/*
 *  wl "nan" iovar iovar handler
 */
static int
wl_nan_subcmd_cancel_publish(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	return wl_nan_subcmd_cancel(wl, cmd, argc, argv,
		is_set, iov_data, avail_len);
}

static int
wl_nan_subcmd_cancel_subscribe(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	return wl_nan_subcmd_cancel(wl, cmd, argc, argv,
		is_set, iov_data, avail_len);
}

static int
wl_nan_subcmd_sd_transmit(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int ret = BCME_OK;
	int id, dest_id = 0;
	struct ether_addr dest = ether_null;
	/* Set highest value as the default value for priority */
	uint8 *pxtlv = NULL;
	uint16 buflen, buflen_at_start, sd_info_len_start;
	wl_nan_sd_transmit_t *sd_xmit = NULL;
	const nan_sd_config_param_info_t *param_p = NULL;
	bool is_lcl_id = FALSE;
	bool is_dest_id = FALSE;
	bool is_dest_mac = FALSE;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(is_set);
	UNUSED_PARAMETER(argc);

	/* Copy mandatory parameters from command line. */
	if (!*argv) {
		printf("Missing instance id parameter.\n");
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	id = atoi(*argv);
	if ((ret = wl_nan_is_instance_valid(id)) != WL_NAN_E_OK) {
		printf("Invalid instance_id.\n");
		goto exit;
	}
	is_lcl_id = TRUE;
	argv++;
	sd_xmit = (wl_nan_sd_transmit_t *)iov_data;
	sd_xmit->local_service_id = id;
	pxtlv = (uint8 *)&sd_xmit->service_info[0];

	/* max data we can write, it will be decremented as we pack */
	buflen = *avail_len;
	buflen_at_start = buflen;

	buflen -= OFFSETOF(wl_nan_sd_transmit_t, service_info[0]);

	while (*argv != NULL) {
		if ((param_p = nan_lookup_sd_config_param(*argv)) == NULL) {
			break;
		}
		/*
		 * Skip param name
		 */
		argv++;
		switch (param_p->id) {
		case NAN_SD_PARAM_DEST_INST_ID:
			if (!*argv) {
				printf("Missing requestor id parameter.\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			dest_id = atoi(*argv);
			if ((ret = wl_nan_is_instance_valid(dest_id)) != WL_NAN_E_OK) {
				printf("Invalid requestor_id.\n");
				goto exit;
			}
			sd_xmit->requestor_service_id = dest_id;
			is_dest_id = TRUE;
			break;
		case NAN_SD_PARAM_DEST_MAC:
			if (!*argv) {
				printf("Missing destination MAC address.\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			if (!wl_ether_atoe(*argv, &dest)) {
				printf("Invalid ether addr provided\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			memcpy(&sd_xmit->destination_addr, &dest, ETHER_ADDR_LEN);
			is_dest_mac = TRUE;
			break;
		case NAN_SD_PARAM_PRIORITY:
			sd_xmit->priority = atoi(*argv);
			break;
		case NAN_SD_PARAM_SVC_INFO:
			sd_info_len_start = buflen;
			ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv, &buflen,
				WL_NAN_XTLV_SD_SVC_INFO, *argv);
			if (ret != BCME_OK) {
				goto exit;
			}
			if ((sd_info_len_start - buflen) > 0xFF) {
				printf("Invalid service info length %d\n",
					(sd_info_len_start - buflen));
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			sd_xmit->service_info_len = (uint8)(sd_info_len_start - buflen);
			break;
		default:
			printf("Unrecognized parameter %s\n", *argv);
			ret = BCME_USAGE_ERROR;
			goto exit;
		}
		argv++;
	}

	/* Check if all mandatory params are provided */
	if (is_lcl_id && is_dest_id && is_dest_mac) {
		/* adjust avail_len to the end of last data record */
		*avail_len -= (buflen_at_start - buflen);
	} else {
		printf("Missing parameters\n");
		ret = BCME_USAGE_ERROR;
	}
exit:
	return ret;
}

static int
wl_nan_subcmd_sd_vendor_info(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(is_set);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	return res;
}

static int
wl_nan_subcmd_sd_statistics(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(is_set);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	return res;
}

static int
wl_nan_subcmd_sd_connection(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(is_set);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	return res;
}

static int
wl_nan_subcmd_sd_show(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(is_set);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	return res;
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

	if (ALIGN_SIZE(BCM_XTLV_HDR_SIZE_EX(BCM_XTLV_OPTION_ALIGN32) + len,
		sizeof(uint32)) > *buflen) {
		printf("bcm_pack_xtlv_entry: no space tlv_buf: requested:%d, available:%d\n",
		 ((int)BCM_XTLV_HDR_SIZE_EX(BCM_XTLV_OPTION_ALIGN32) + len), *buflen);
		return BCME_BADLEN;
	}
	ptlv->id = htol16(type);
	ptlv->len = htol16(len);

	/* copy callers data */
	if (get_ie_data((uchar*)hex, ptlv->data, len)) {
		return BCME_BADARG;
	}

	/* advance callers pointer to tlv buff */
	*tlv_buf += BCM_XTLV_SIZE_EX(ptlv, BCM_XTLV_OPTION_ALIGN32);
	/* decrement the len */
	*buflen -=  BCM_XTLV_SIZE_EX(ptlv, BCM_XTLV_OPTION_ALIGN32);
	return BCME_OK;
}

static int
wl_nan_subcmd_merge(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{

	int res = BCME_OK;
	wl_nan_merge_enable_t enable;
	uint16 len;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(enable);

		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		enable = (uint8)atoi(*argv);
		memcpy(iov_data, &enable, sizeof(enable));
	}

	*avail_len -= len;

	return res;
}

const wl_nan_sub_cmd_t *
nan_get_subcmd_info(char **argv)
{
	char *cmdname = *argv;
	const wl_nan_sub_cmd_t *p_subcmd_info = &nan_cmd_list[0];

	while (p_subcmd_info->name != NULL) {
		if (stricmp(p_subcmd_info->name, cmdname) == 0) {
			return p_subcmd_info;
		}
		p_subcmd_info++;
	}

	return NULL;
}

static int
nan_get_arg_count(char **argv)
{
	int count = 0;
	while (*argv != NULL) {
		if (strcmp(*argv, WL_IOV_BATCH_DELIMITER) == 0) {
			break;
		}
		argv++;
		count++;
	}

	return count;
}

static int
wl_nan_control(void *wl, cmd_t *cmd, char **argv)
{
	int ret = BCME_USAGE_ERROR;
	const wl_nan_sub_cmd_t *nancmd = NULL;
	bcm_iov_batch_buf_t *b_buf = NULL;
	uint8 *p_iov_buf;
	uint16 iov_len, iov_len_start, subcmd_len, nancmd_data_len;
	bool is_set = TRUE;
	bool first_cmd_req = is_set;
	int argc = 0;
	/* Skip the command name */
	UNUSED_PARAMETER(cmd);

	argv++;
	/* skip to cmd name after "nan" */
	if (*argv) {
		if (!strcmp(*argv, "-h") || !strcmp(*argv, "help"))  {
			/* help , or -h* */
			argv++;
			wl_nan_help(NAN_HELP_ALL);
			goto fail;
		}
	}

	/*
	 * malloc iov buf memory
	 */
	b_buf = (bcm_iov_batch_buf_t *)calloc(1, WLC_IOCTL_MEDLEN);
	if (b_buf == NULL) {
		return BCME_NOMEM;
	}
	/*
	 * Fill the header
	 */
	iov_len = iov_len_start = WLC_IOCTL_MEDLEN;
	b_buf->version = htol16(0x8000);
	b_buf->count = 0;
	p_iov_buf = (uint8 *)(&b_buf->cmds[0]);
	iov_len -= OFFSETOF(bcm_iov_batch_buf_t, cmds[0]);

	while (*argv != NULL) {
		bcm_iov_batch_subcmd_t *sub_cmd =
			(bcm_iov_batch_subcmd_t *)p_iov_buf;
		/*
		 * Lookup sub-command info
		 */
		nancmd = nan_get_subcmd_info(argv);
		if (!nancmd) {
			goto fail;
		}
		/* skip over sub-cmd name */
		argv++;

		/*
		 * Get arg count for sub-command
		 */
		argc = nan_get_arg_count(argv);

		sub_cmd->u.options =
			htol32(BCM_XTLV_OPTION_ALIGN32);
		/*
		 * Skip over sub-command header
		 */
		iov_len -= OFFSETOF(bcm_iov_batch_subcmd_t, data);

		/*
		 * take a snapshot of curr avail len,
		 * to calculate iovar data len to be packed.
		 */
		subcmd_len = iov_len;

		/* invoke nan sub-command handler */
		ret = nancmd->handler(wl, nancmd, argc, argv, &is_set,
				(uint8 *)&sub_cmd->data[0], &subcmd_len);

		if (ret != BCME_OK) {
			goto fail;
		}
		nancmd_data_len = (iov_len - subcmd_len);
		/*
		 * In Batched commands, sub-commands TLV length
		 * includes size of options as well. Because,
		 * options are considered as part bcm xtlv data
		 * considered as part bcm xtlv data
		 */
		nancmd_data_len += sizeof(sub_cmd->u.options);

		/*
		 * Data buffer is set NULL, because sub-cmd
		 * tlv data is already filled by command hanlder
		 * and no need of memcpy.
		 */
		ret = bcm_pack_xtlv_entry(&p_iov_buf, &iov_len,
				nancmd->id, nancmd_data_len,
				NULL, BCM_XTLV_OPTION_ALIGN32);

		/*
		 * iov_len is already compensated before sending
		 * the buffer to cmd handler.
		 * xtlv hdr and options size are compensated again
		 * in bcm_pack_xtlv_entry().
		 */
		iov_len += OFFSETOF(bcm_iov_batch_subcmd_t, data);
		if (ret == BCME_OK) {
			/* Note whether first command is set/get */
			if (!b_buf->count) {
				first_cmd_req = is_set;
			} else if (first_cmd_req != is_set) {
				/* Returning error, if sub-sequent commands req is
				 * not same as first_cmd_req type.
				 */
				 ret = BCME_UNSUPPORTED;
				 break;
			}

			/* bump sub-command count */
			b_buf->count++;
			/* No more commands to parse */
			if (*argv == NULL) {
				break;
			}
			/* Still un parsed arguments exist and
			 * immediate argument to parse is not
			 * a BATCH_DELIMITER
			 */
			while (*argv != NULL) {
				if (strcmp(*argv, WL_IOV_BATCH_DELIMITER) == 0) {
					/* skip BATCH_DELIMITER i.e "+" */
					argv++;
					break;
				}
				argv++;
			}
		} else {
			printf("Error handling sub-command %d\n", ret);
			break;
		}
	}

	/* Command usage error handling case */
	if (ret != BCME_OK) {
		goto fail;
	}

	iov_len = iov_len_start - iov_len;

	/*
	 * Dispatch iovar
	 */
	ret = wl_nan_do_ioctl(wl, (void *)b_buf, iov_len, is_set);

fail:
	if (ret != BCME_OK) {
		printf("Error: %d\n", ret);
		if (nancmd) {
			wl_nan_help(nancmd->id);
		} else {
			wl_nan_help(NAN_HELP_ALL);
		}
	}
	free(b_buf);
	return ret;
}

static int
wl_nan_subcmd_election_advertisers(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
	} else {
		printf("config_status doesn't expect any parameters\n");
		return BCME_USAGE_ERROR;
	}

	return res;
}

static int
wl_nan_subcmd_cfg_oui(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	wl_nan_oui_type_t *nan_oui_type;
	uint16 len;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		if (ARGCNT(argv) < WL_NAN_CMD_CFG_OUI_ARGC) {
			return BCME_USAGE_ERROR;
		}

		*is_set = TRUE;
		len = sizeof(*nan_oui_type);

		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		nan_oui_type = (wl_nan_oui_type_t *)iov_data;

		if (get_oui_bytes((uchar *)(*argv), (uchar *)nan_oui_type->nan_oui)) {
			fprintf(stderr, "invalid OUI\n");
			return BCME_BADARG;
		}

		/* advance to the next param */
		argv++;

		nan_oui_type->type =  strtoul(*argv, NULL, 0);
	}

	*avail_len -= len;

	return res;
}

static void
print_peer_rssi(wl_nan_peer_rssi_data_t *prssi)
{
	uint16 peer_cnt, rssi_cnt;
	wl_nan_peer_rssi_entry_t *peer;
	char buf[MAX_MAC_BUF] = {'\0'};
	char raw_rssi[MAX_RSSI_BUF] = {'\0'};
	char avg_rssi[MAX_RSSI_BUF] = {'\0'};
	size_t raw_sz_left, avg_sz_left;
	uint16 raw_sz = 0, avg_sz = 0;

	printf("> RSSI Data: \n");
	printf("no of peers: %d\n", prssi->peer_cnt);
	for (peer_cnt = 0; peer_cnt < prssi->peer_cnt; peer_cnt++) {
		peer = &prssi->peers[peer_cnt];
		bcm_ether_ntoa(&peer->mac, buf);
		raw_sz_left = avg_sz_left = 128;
		raw_sz = avg_sz = 0;
		for (rssi_cnt = 0; rssi_cnt < peer->rssi_cnt; rssi_cnt++) {
			raw_sz += snprintf(raw_rssi+raw_sz, MAX_RSSI_BUF-raw_sz,
				"%d(%u) ", peer->rssi[rssi_cnt].rssi_raw,
				peer->rssi[rssi_cnt].rx_chan);
			raw_sz_left -= raw_sz;
			avg_sz += snprintf(avg_rssi+avg_sz, MAX_RSSI_BUF-avg_sz,
				"%d(%u) ", peer->rssi[rssi_cnt].rssi_avg,
				peer->rssi[rssi_cnt].rx_chan);
			avg_sz_left -= avg_sz;
		}
		if (prssi->flags & WL_NAN_PEER_RSSI) {
			printf("%s %s %s \n", buf, raw_rssi, avg_rssi);
		}
		else if (prssi->flags & WL_NAN_PEER_RSSI_LIST)
		{
			printf("%s\n", buf);
			printf("raw: %s\n", raw_rssi);
			printf("avg: %s\n", avg_rssi);
		} else {
			/* Invalid */
		}
	}
}

static int
wl_nan_subcmd_dump(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int ret = BCME_OK;
	wl_nan_dbg_dump_type_t dump_type;
	uint16 len;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	/* It is always a get */
	*is_set = FALSE;

	if (!argv[0] || argc != 1) {
		printf("Incorrect number of parameters.\n");
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	if (!strcmp(argv[0], "rssi")) {
		dump_type = WL_NAN_DBG_DT_RSSI_DATA;
	} else if (!strcmp(argv[0], "stats")) {
		dump_type = WL_NAN_DBG_DT_STATS_DATA;
	} else {
		printf("Invalid argument %s.\n", argv[0]);
		ret = BCME_USAGE_ERROR;
		goto exit;
	}

	len = sizeof(dump_type);

	if (len > *avail_len) {
		printf("Buf short, requested:%d, available:%d\n",
				len, *avail_len);
		return BCME_BUFTOOSHORT;
	}

	memcpy(iov_data, &dump_type, len);
	*avail_len -= len;

exit:
	return ret;
}

static int
wl_nan_subcmd_dbg_level(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint16 len;
	wl_nan_dbg_level_t dbg_level;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(is_set);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(wl_nan_dbg_level_t);
		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		memset(&dbg_level, 0, len);
		if ((*argv != NULL)) {
			dbg_level.nan_err_level = strtoul(*argv, NULL, 16);
			argv++;
			if ((*argv != NULL)) {
				dbg_level.nan_dbg_level = strtoul(*argv, NULL, 16);
				argv++;
				if ((*argv != NULL)) {
					dbg_level.nan_info_level = strtoul(*argv, NULL, 16);
				}
			}
		}
		memcpy(iov_data, (uint8 *)&dbg_level, sizeof(wl_nan_dbg_level_t));
	}

	*avail_len -= len;
	return res;
}

static int
wl_nan_subcmd_clear(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int ret = BCME_OK;
	wl_nan_dbg_dump_type_t clear_type;
	uint16 len;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	/* It is always set */
	*is_set = TRUE;

	if (!argv[0] || argc != 1) {
		printf("Incorrect number of parameters.\n");
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	if (!strcmp(argv[0], "rssi")) {
		clear_type = WL_NAN_DBG_DT_RSSI_DATA;
	} else if (!strcmp(argv[0], "stats")) {
		clear_type = WL_NAN_DBG_DT_STATS_DATA;
	} else {
		printf("Invalid argument %s.\n", argv[0]);
		ret = BCME_USAGE_ERROR;
		goto exit;
	}

	len = sizeof(clear_type);

	if (len > *avail_len) {
		printf("Buf short, requested:%d, available:%d\n",
				len, *avail_len);
		return BCME_BUFTOOSHORT;
	}

	memcpy(iov_data, &clear_type, len);
	*avail_len -= len;

exit:
	return ret;
}

/* nan 2.0 iovars */

/* device capabilities */
static int
wl_nan_subcmd_dp_cap(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(is_set);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	return res;
}

/* status */
static int
wl_nan_subcmd_dp_status(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint8 ndp_id;
	uint16 len;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if (*argv == NULL) {
		return BCME_IOCTL_ERROR;
	} else {
		/* set is not supported */
		*is_set = FALSE;
		ndp_id = atoi(*argv);

		if (ndp_id == 255) {
			return BCME_BADARG;
		}
		len = sizeof(ndp_id);
		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}
		memcpy(iov_data, &ndp_id, sizeof(ndp_id));
	}
	*avail_len -= len;
	return res;
}

/* statistics */
static int
wl_nan_subcmd_dp_stats(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(is_set);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	return res;
}

/* schedule update */
static int
wl_nan_subcmd_dp_schedupd(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(is_set);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	return res;
}

enum {
	NAN_DP_CFG_TYPE_UCAST		= 0x0001,
	NAN_DP_CFG_TYPE_MCAST		= 0x0002,
	NAN_DP_CFG_SECURITY		= 0x0003,
	NAN_DP_CFG_QOS			= 0x0004,
	NAN_DP_CFG_PUB_INST		= 0x0005,
	NAN_DP_CFG_PEER_IFADDR		= 0x0006,
	NAN_DP_CFG_MCAST_IFADDR		= 0x0007,
	NAN_DP_CFG_DATA_IFADDR		= 0x0008,
	NAN_DP_CFG_SVC_SPEC_INFO	= 0x0009,
	NAN_DP_CFG_NDPID		= 0x000a,
	NAN_DP_CFG_STATUS		= 0x000b,
	NAN_DP_CFG_REASON_CODE		= 0x000c,
	NAN_DP_CFG_AVAIL		= 0x000d,
	NAN_DP_CFG_INIT_DATA_IFADDR	= 0x000e,
	NAN_DP_CFG_CONFIRM		= 0x000f
};

typedef struct nan_dp_config_param_info {
	char *name;	/* <param-name> string to identify the config item */
	uint16 id;
	uint16 len; /* len in bytes */
	char *help_msg; /* Help message */
} nan_dp_config_param_info_t;

/*
 * Parameter name and size for service discovery IOVARs
 */
static const nan_dp_config_param_info_t nan_dp_config_param_info[] = {
	/* param-name	param_id	len	help message */
	{"ucast",	NAN_DP_CFG_TYPE_UCAST,		0,
	"unicast req"},
	{"mcast",	NAN_DP_CFG_TYPE_MCAST,		0,
	"multicast req"},
	{"pub_id",	NAN_DP_CFG_PUB_INST,		sizeof(uint16),
	"public instance id"},
	{"status",	NAN_DP_CFG_STATUS,		sizeof(uint8),
	"response frame status"},
	{"confirm",	NAN_DP_CFG_CONFIRM,		sizeof(uint8),
	"sets confirm/explicit confirm for dp_req/dp_resp"},
	{"reason",	NAN_DP_CFG_REASON_CODE,		sizeof(uint8),
	"response frame reason code"},
	{"ndp_id",	NAN_DP_CFG_NDPID,		sizeof(uint8),
	"Local ndp_id of peer"},
	{"security",	NAN_DP_CFG_SECURITY,		sizeof(uint8),
	"is security enabled"},
	{"peer_mac",	NAN_DP_CFG_PEER_IFADDR,		ETHER_ADDR_LEN,
	"peer mac address"},
	{"data_mac",	NAN_DP_CFG_DATA_IFADDR,		ETHER_ADDR_LEN,
	"local data mac address"},
	{"init_data_mac", NAN_DP_CFG_INIT_DATA_IFADDR,	ETHER_ADDR_LEN,
	"Initiators data address(used in dataresp)"},
	{"mcast_mac",	NAN_DP_CFG_MCAST_IFADDR,	ETHER_ADDR_LEN,
	"multicast mac address"},
	{"qos",		NAN_DP_CFG_QOS,			sizeof(wl_nan_dp_qos_t),
	"qos <tid> <pkt size> <mean rate> <svc_interval>"},
	{"svc_spec_info", NAN_DP_CFG_SVC_SPEC_INFO,	WL_NAN_DP_MAX_SVC_INFO,
	"service specific info"}
};

const nan_dp_config_param_info_t *
nan_lookup_dp_config_param(char *param_name)
{
	int i = 0;
	const nan_dp_config_param_info_t *param_p = &nan_dp_config_param_info[0];

	for (i = 0; i < (int)ARRAYSIZE(nan_dp_config_param_info); i++) {
		if (stricmp(param_p->name, param_name) == 0) {
			return param_p;
		}
		param_p++;
	}

	return NULL;
}

static int
wl_nan_subcmd_dp_autoconn(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint8 autoconn = 0;
	uint16 len;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(autoconn);

		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		autoconn = (uint8)atoi(*argv);
		if (autoconn & WL_NAN_AUTO_DPRESP) {
			printf("auto_dpresp enabled \n");
		} else {
			printf("auto_dpresp disabled \n");
		}
		if (autoconn & WL_NAN_AUTO_DPCONF) {
			printf("auto_dpconf enabled \n");
		} else {
			printf("auto_dpconf disabled \n");
		}
		memcpy(iov_data, &autoconn, sizeof(autoconn));
	}

	*avail_len -= len;

	return res;
}

int
wl_nan_get_qos_params(char **argv, wl_nan_dp_qos_t *qos, uint8 *qos_num_params)
{
	if (*argv != NULL) {
		qos->tid = (uint8)atoi(*argv);
		argv++;
		(*qos_num_params)++;

		if (*argv != NULL) {
			qos->pkt_size = (uint16)atoi(*argv);
			argv++;
			(*qos_num_params)++;

			if (*argv != NULL) {
				qos->mean_rate = (uint16)atoi(*argv);
				argv++;
				(*qos_num_params)++;

				if (*argv != NULL) {
					qos->svc_interval = (uint16)atoi(*argv);
					(*qos_num_params)++;
					return BCME_OK;
				}
			}
		}
	}
	return BCME_ERROR;
}

static int
wl_nan_subcmd_dp_req(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	wl_nan_dp_req_t *datareq = NULL;
	int ret = BCME_OK;
	uint16 len = 0;
	uint8 qos_num_params = 0;
	const nan_dp_config_param_info_t *param_p = NULL;

	uint8 *pxtlv;
	uint16 buflen = *avail_len, buflen_at_start;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if (*argv == NULL) {
		ret = BCME_USAGE_ERROR;
		goto exit;
	}

	datareq = (wl_nan_dp_req_t *)iov_data;
	pxtlv = (uint8 *)&datareq->svc_spec_info;

	if (*avail_len < sizeof(*datareq)) {
		/* buffer is insufficient */
		ret = BCME_NOMEM;
		goto exit;
	}

	/* Setting default data path type to unicast */
	datareq->type = WL_NAN_DP_TYPE_UNICAST;

	*is_set = FALSE;

	/* Parse optional args. Validity is checked by discovery engine */
	while ((*argv != NULL) && argc) {
		if ((param_p = nan_lookup_dp_config_param(*argv)) == NULL) {
			ret = BCME_USAGE_ERROR;
			goto exit;
		}
		/*
		 * Skip param name
		 */
		argv++;
		argc--;
		switch (param_p->id) {

		case NAN_DP_CFG_TYPE_UCAST:
			datareq->type = WL_NAN_DP_TYPE_UNICAST;
			break;

		case NAN_DP_CFG_TYPE_MCAST:
			datareq->type = WL_NAN_DP_TYPE_MULTICAST;
			break;

		case NAN_DP_CFG_CONFIRM:
			datareq->flag |= WL_NAN_DP_FLAG_CONFIRM;
			break;

		case NAN_DP_CFG_SECURITY:
			if (*argv != NULL) {
				datareq->security = (uint8)(atoi(*argv));
				argv++;
				argc--;
			}
			break;

		case NAN_DP_CFG_QOS:
			ret = wl_nan_get_qos_params(argv, &datareq->qos, &qos_num_params);
			if (ret != BCME_OK) {
				return ret;
			}
			argv += qos_num_params;
			argc -= qos_num_params;
			break;

		case NAN_DP_CFG_PUB_INST:
			if (*argv != NULL) {
				datareq->pub_id = (uint8)(atoi(*argv));
				argv++;
				argc--;
			}
			break;

		case NAN_DP_CFG_PEER_IFADDR:
			if (*argv) {
				if (!wl_ether_atoe(argv[0], &datareq->peer_mac)) {
					printf("Malformed MAC address parameter\n");
					break;
				}
				argv++;
			}
			break;

		case NAN_DP_CFG_MCAST_IFADDR:
			if (*argv != NULL) {
				if (datareq->type != WL_NAN_DP_TYPE_MULTICAST) {
					printf("ERROR: multicast mac addr specified"
							"with unicast type\n");
					return BCME_ERROR;
				}
				if (!wl_ether_atoe(argv[0], &datareq->mcast_mac)) {
					printf("Malformed MAC address parameter\n");
					break;
				}
				argv++;
				argc--;
			}
			break;

		case NAN_DP_CFG_SVC_SPEC_INFO:
			if (*argv) {
				buflen_at_start = buflen;
				ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv,
						&buflen, WL_NAN_XTLV_SD_SVC_INFO, *argv);
				if (ret != BCME_OK) {
					printf("unable to process svc_spec_info: %d\n", ret);
					return ret;
				}
				len += (uint8)(buflen_at_start - buflen);
				datareq->flag |= WL_NAN_DP_FLAG_SVC_INFO;
				argv++;
				argc--;
			}
			break;

		default:
			return BCME_ERROR;
			break;

		} /* switch */
	} /* while */

	len += OFFSETOF(wl_nan_dp_req_t, svc_spec_info);
	*avail_len -= len;

exit:
	return ret;
}

static int
wl_nan_subcmd_dp_resp(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	wl_nan_dp_resp_t *dataresp = NULL;
	int ret = BCME_OK;
	uint16 len = 0;
	uint8 qos_num_params = 0;
	const nan_dp_config_param_info_t *param_p = NULL;

	uint8 *pxtlv;
	uint16 buflen = *avail_len, buflen_at_start;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if (*argv == NULL) {
		ret = BCME_USAGE_ERROR;
		goto exit;
	}

	dataresp = (wl_nan_dp_resp_t *)iov_data;
	pxtlv = (uint8 *)&dataresp->svc_spec_info;

	if (*avail_len < sizeof(*dataresp)) {
		/* buffer is insufficient */
		ret = BCME_NOMEM;
		goto exit;
	}

	/* Setting default data path type to unicast */
	dataresp->type = WL_NAN_DP_TYPE_UNICAST;

	*is_set = FALSE;

	/* Parse optional args. Validity is checked by discovery engine */
	while ((*argv != NULL) && argc) {
		if ((param_p = nan_lookup_dp_config_param(*argv)) == NULL) {
			ret = BCME_USAGE_ERROR;
			goto exit;
		}
		/*
		 * Skip param name
		 */
		argv++;
		argc--;
		switch (param_p->id) {

		case NAN_DP_CFG_TYPE_UCAST:
			dataresp->type = WL_NAN_DP_TYPE_UNICAST;
			break;

		case NAN_DP_CFG_TYPE_MCAST:
			dataresp->type = WL_NAN_DP_TYPE_MULTICAST;
			break;

		case NAN_DP_CFG_CONFIRM:
			dataresp->flag |= WL_NAN_DP_FLAG_EXPLICIT_CFM;
			break;

		case NAN_DP_CFG_SECURITY:
			if (*argv != NULL) {
				dataresp->security = (uint8)(atoi(*argv));
				argv++;
				argc--;
			}
			break;

		case NAN_DP_CFG_STATUS:
			if (*argv != NULL) {
				dataresp->status = (uint8)(atoi(*argv));
				argv++;
				argc--;
			}
			break;

		case NAN_DP_CFG_REASON_CODE:
			if (*argv != NULL) {
				dataresp->reason_code = (uint8)(atoi(*argv));
				argv++;
				argc--;
			}
			break;

		case NAN_DP_CFG_QOS:
			ret = wl_nan_get_qos_params(argv, &dataresp->qos, &qos_num_params);
			if (ret != BCME_OK) {
				return ret;
			}
			argv += qos_num_params;
			argc -= qos_num_params;
			break;

		case NAN_DP_CFG_NDPID:
			if (*argv != NULL) {
				dataresp->ndp_id = (uint8)(atoi(*argv));
				argv++;
				argc--;
			}
			break;

		case NAN_DP_CFG_PEER_IFADDR:
		case NAN_DP_CFG_MCAST_IFADDR:
			if (*argv != NULL) {
				if (!wl_ether_atoe(argv[0], &dataresp->mac_addr)) {
					printf("Malformed MAC address parameter\n");
					break;
				}
				argv++;
				argc--;
			}
			break;

		case NAN_DP_CFG_SVC_SPEC_INFO:
			if (*argv) {
				buflen_at_start = buflen;
				ret = bcm_pack_xtlv_entry_from_hex_string(&pxtlv,
						&buflen, WL_NAN_XTLV_SD_SVC_INFO, *argv);
				if (ret != BCME_OK) {
					printf("unable to process svc_spec_info: %d\n", ret);
					return ret;
				}
				len += (uint8)(buflen_at_start - buflen);
				dataresp->flag |= WL_NAN_DP_FLAG_SVC_INFO;
				argv++;
				argc--;
			}
			break;

		default:
			return BCME_ERROR;
			break;

		} /* switch */
	} /* while */

	len += OFFSETOF(wl_nan_dp_resp_t, svc_spec_info);
	*avail_len -= len;

exit:
	return ret;
}

static int
wl_nan_subcmd_dp_conf(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint8 ndp_id;
	uint8 status;
	uint16 len;
	wl_nan_dp_conf_t *dataconf = (wl_nan_dp_conf_t *)iov_data;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	if ((*argv == NULL) || (*argv != NULL && argc < 2)) {
		return BCME_IOCTL_ERROR;
	} else {
		*is_set = TRUE;
		ndp_id = atoi(*argv);

		if (ndp_id == WL_NAN_INVALID_NDPID) {
			return BCME_BADARG;
		}

		len = sizeof(*dataconf);

		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		argv++;
		status = atoi(*argv);

		dataconf->lndp_id = ndp_id;
		dataconf->status = status;
	}

	*avail_len -= len;

	return res;
}

static int
wl_nan_subcmd_dp_dataend(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;
	uint8 ndp_id;
	uint8 status;
	uint16 len;
	wl_nan_dp_end_t dataend;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	if ((*argv == NULL) || (*argv != NULL && argc < 2)) {
		return BCME_IOCTL_ERROR;
	} else {
		/* get is not supported */
		*is_set = TRUE;
		ndp_id = atoi(*argv);

		if (ndp_id == WL_NAN_INVALID_NDPID) {
			return BCME_BADARG;
		}

		len = sizeof(dataend);

		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}

		argv++;
		status = atoi(*argv);
		dataend.lndp_id = ndp_id;
		dataend.status = status;

		memcpy(iov_data, &dataend, sizeof(dataend));
	}

	*avail_len -= len;

	return res;
}

static int
wl_nan_subcmd_dp_show(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int res = BCME_OK;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(iov_data);
	UNUSED_PARAMETER(avail_len);

	if ((*argv != NULL) || argc != 0) {
		return BCME_IOCTL_ERROR;
	} else {
		/* only get is supported */
		*is_set = FALSE;
	}

	return res;
}

static int
wl_nan_subcmd_cfg_avail(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int ret = BCME_OK;
	bool started = FALSE;
	wl_avail_t *avail = (wl_avail_t *)iov_data;
	wl_avail_entry_t *entry;		/* used for filling entry structure */
	uint8 *p = avail->entry;		/* tracking pointer */
	uint8 avail_type;	/* 1=local, 2=peer, 3=ndc, 4=immutable, 5=response, 6=counter */
	uint8 entry_type;	/* 1=committed, 2=potential */
	uint8 optional_num;

	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	if (*argv == NULL) {
		ret = BCME_USAGE_ERROR;
		goto exit;
	}

	avail_type = atoi(*argv++);
	if (avail_type < WL_AVAIL_LOCAL || avail_type > WL_AVAIL_TYPE_MAX) {
		printf("Invalid availability type\n");
		ret = BCME_USAGE_ERROR;
		goto exit;
	}

	/* wl_avail fileds for both set/get */
	memset(avail, 0, sizeof(*avail));
	avail->length = OFFSETOF(wl_avail_t, entry);
	avail->flags = avail_type;
	avail->num_entries = 0;

	/* check for GET first */
	if (*argv == NULL || argc == 1) {
		if ((avail_type >= WL_AVAIL_LOCAL) &&
				(avail_type <= WL_AVAIL_TYPE_MAX)) {
			*avail_len -= avail->length;
			avail->length = htod16(avail->length);
			avail->flags = htod16(avail->flags);
			*is_set = FALSE;
			goto exit;
		}
	}

	*is_set = TRUE;
	/* wl_avail fields for set */

	printf("\nType (1=local, 2=peer, 3=ndc, 4=immutable, 5=response, "
		"6=counter, 7=ranging): %d\n", avail_type);
	/* populate avail entries */
	while (*argv != NULL) {
		entry = (wl_avail_entry_t*)p;
		if ((stricmp(*argv++, "entry") != 0)) {
			prhex("argv", (uchar*)*argv, 7);
			if (!started) {
				printf("Missing avail entry type\n");
				ret = BCME_USAGE_ERROR; goto exit;
			}
			break;
		} else {
			started = TRUE;
			entry_type = atoi(*argv++);
			if ((entry_type == (WL_AVAIL_ENTRY_COM | WL_AVAIL_ENTRY_COND)) ||
					(entry_type >= (WL_AVAIL_ENTRY_COM |
					WL_AVAIL_ENTRY_POT | WL_AVAIL_ENTRY_COND))) {
				printf("Invalid entry type\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			entry->flags = entry_type;
		}
		/* optional parameters */
		optional_num = 0;
		if ((ret = wl_nan_set_avail_entry_optional(argv, avail, entry,
				avail_type, &optional_num)) != BCME_OK) {
			goto exit;
		}

		/* update wl_avail and populate wl_avail_entry */
		entry->length = OFFSETOF(wl_avail_entry_t, bitmap) + entry->bitmap_len;
		avail->num_entries++;
		avail->length += entry->length;

		/* advance pointer for next entry */
		p += entry->length;
		argv += optional_num;

		/* convert to dongle endianness */
		entry->length = htod16(entry->length);
		entry->start_offset = htod16(entry->start_offset);
		entry->u.channel_info = htod32(entry->u.channel_info);
		entry->flags = htod16(entry->flags);

		/* output configuration info (expecting dongle byte order) */
		wlu_nan_print_wl_avail_entry(avail_type & WL_AVAIL_TYPE_MASK, entry);
	}

	/* update avail_len and conver to dongle endianness */
	if (avail->num_entries) {
		*avail_len -= avail->length;
		avail->length = htod16(avail->length);
		avail->flags = htod16(avail->flags);
		prhex("\ntotal wl_avail", (uchar*)avail, dtoh16(avail->length));
	} else {
		printf("No entry in given avail\n");
		ret = BCME_USAGE_ERROR;
	}

exit:
	return ret;
}

#define WLU_AVAIL_NUM_ARGS_PER_PARAM	2
/* process optional parameters and sets them in wl_avail_entry */
static int
wl_nan_set_avail_entry_optional(char** argv, wl_avail_t* avail, wl_avail_entry_t* entry,
	uint8 avail_type, uint8* num)
{
	int ret = BCME_OK;
	uint8 i;
	char *a;
	uint8 bitdur_type;
	uint8 period_type;
	uint16 offset;		/* start offset, 0-511 */
	uint8 usage;		/* usage preference, 0-3 */
	uint32 chanspec;	/* 0=all channels/bands */
	uint32 band;		/* 0=all bands, 1=sub-1G, 2=2.4G, 4=5G */
	struct ether_addr ndc_id;
	struct ether_addr peer_nmi;
	char def_ndc_id[ETHER_ADDR_LEN] = { 0xAA, 0xBB, 0xCC, 0x00, 0x0, 0x0 };
	char def_peer_nmi[ETHER_ADDR_LEN] = { 0x00, 0x90, 0x4c, 0xaa, 0xbb, 0xcc };

	/* set default values for optional parameters */
	entry->start_offset = 0;
	entry->u.band = 0;
	entry->period = WL_AVAIL_PERIOD_512;
	entry->flags |= (3 << WL_AVAIL_ENTRY_USAGE_SHIFT) |
		(WL_AVAIL_BIT_DUR_16 << WL_AVAIL_ENTRY_BIT_DUR_SHIFT);
	if (avail_type == WL_AVAIL_NDC) {
		memcpy(&avail->addr, def_ndc_id, ETHER_ADDR_LEN);
	} else if (avail_type == WL_AVAIL_PEER) {
		memcpy(&avail->addr, def_peer_nmi, ETHER_ADDR_LEN);
	}
	entry->bitmap_len = 0;

	/* parse optional parameters */
	while ((*argv != NULL) && (stricmp(*argv, "entry") != 0)) {
		if ((stricmp(*argv, "bitdur") == 0)) {
			argv++;
			bitdur_type = atoi(*argv++);
			/* by design, local avail slots are 16TU */
			if (bitdur_type > WL_AVAIL_BIT_DUR_128) {
				printf("Invalid bitdur type\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			entry->flags |= bitdur_type << WL_AVAIL_ENTRY_BIT_DUR_SHIFT;
		} else if ((stricmp(*argv, "period") == 0)) {
			argv++;
			period_type = atoi(*argv++);
			if (period_type > WL_AVAIL_PERIOD_8192) {
				printf("Invalid period type\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			entry->period = period_type;
		} else if ((stricmp(*argv, "offset") == 0)) {
			argv++;
			offset = atoi(*argv++);
			if (offset > 511) {
				printf("Invalid offset\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			entry->start_offset = htod16(offset);
		} else if ((stricmp(*argv, "usage") == 0)) {
			argv++;
			usage = atoi(*argv++);
			if (usage > 3) {
				printf("Invalid usage pref\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			entry->flags &= ~(WL_AVAIL_ENTRY_USAGE_MASK);
			entry->flags |= usage << WL_AVAIL_ENTRY_USAGE_SHIFT;
		} else if ((stricmp(*argv, "ndc") == 0)) {
			argv++;
			if (avail_type != WL_AVAIL_NDC) {
				printf("ndc id is for type ndc\n");
				ret = BCME_USAGE_ERROR; goto exit;
			}
			if (!wl_ether_atoe(*argv++, &ndc_id)) {
				printf("Invalid ndc id\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			memcpy(&avail->addr, &ndc_id, ETHER_ADDR_LEN);
		} else if ((stricmp(*argv, "chanspec") == 0)) {
			argv++;
			chanspec = wf_chspec_aton(*argv++);
			if (chanspec == 0) {
				printf("Invalid chanspec\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			entry->flags |= 1 << WL_AVAIL_ENTRY_CHAN_SHIFT;
			entry->u.channel_info = htod32(chanspec);
		} else if ((stricmp(*argv, "band") == 0)) {
			argv++;
			band = atoi(*argv++);
			if (band > 4) {
				printf("Invalid band\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			entry->flags |= 1 << WL_AVAIL_ENTRY_BAND_SHIFT;
			entry->u.band = htod32(band);
		} else if ((stricmp(*argv, "bitmap") == 0)) {
			argv++;
			/* point to bitmap value for processing */
			a = *argv;
			for (i = 0; i < strlen(*argv); i++) {
				if (*a == '1') {
					setbit(entry->bitmap, i);
				}
				a++;
			}
			argv++;
			/* account for partially filled most significant byte */
			entry->bitmap_len = (i + NBBY - 1) / NBBY;
		} else if (stricmp(*argv, "otahexmap") == 0) {
			char *otahexmapstr;
			argv++;

			otahexmapstr = *argv++;
			if (!strnicmp(otahexmapstr, "0x", 2)) {
				/* remove 0x prefix from otahexmap string before
				 * time bitmap len calculation and bitmap conversion.
				 * get_ie_data() always handle data_str as base 16
				 */
				otahexmapstr += 2;
			}

			entry->bitmap_len = (strlen(otahexmapstr) + 1) / 2;
			if (get_ie_data((uchar*)otahexmapstr, entry->bitmap, entry->bitmap_len)) {
				ret = BCME_BADARG;
				goto exit;
			}
		/* peer nmi address for peer NA */
		} else if ((stricmp(*argv, "peer") == 0)) {
			/* peer nmi address for peer NA */
			argv++;
			if (avail_type != WL_AVAIL_PEER) {
				printf("peer nmi is for type peer\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			if (!wl_ether_atoe(*argv++, &peer_nmi)) {
				printf("Invalid peer nmi\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
			memcpy(&avail->addr, &peer_nmi, ETHER_ADDR_LEN);
		} else {
			printf("Recognized parameter %s\n", *argv);
			ret = BCME_BADARG;
			*num = 0;
			goto exit;
		}
		(*num)++;
		/* TODO add option for channel entry in hex data (opclass + bitmap) */

		/* TODO add option for entire avail entry in hex data */
	}
exit:
	*num *= WLU_AVAIL_NUM_ARGS_PER_PARAM;
	return ret;
}

typedef struct nan_rng_config_param_info {
	char *name;	/* <param-name> string to identify the config item */
	uint16 id;
	uint16 len; /* len in bytes */
	char *help_msg; /* Help message */
} nan_rng_config_param_info_t;
enum {
	NAN_RNG_CFG_PEER	= 0x0001,
	NAN_RNG_CFG_RANGE_ID	= 0x0002,
	NAN_RNG_CFG_PUB_ID	= 0x0003,
	NAN_RNG_CFG_INTERVAL	= 0x0004,
	NAN_RNG_CFG_RESOLUTION	= 0x0005,
	NAN_RNG_CFG_INDICATION	= 0x0006,
	NAN_RNG_CFG_INGRESS_LIMIT	= 0x0007,
	NAN_RNG_CFG_EGRESS_LIMIT	= 0x0008,
	NAN_RNG_CFG_RESULT_REQUIRED	= 0x0009,
	NAN_RNG_CFG_AUTO_RESPONSE	= 0x000a,
	NAN_RNG_CFG_RANGE_STATUS	= 0x000b
};


/*
 * Parameter name and size for service discovery IOVARs
 */
static const nan_rng_config_param_info_t nan_rng_config_param_info[] = {
	/* param-name	param_id	len	help message */
	{"range_id",	NAN_RNG_CFG_RANGE_ID,		sizeof(uint8),
	"unique handle"},
	{"peer",	NAN_RNG_CFG_PEER,		ETHER_ADDR_LEN,
	"ranging peer"},
	{"pub_id",	NAN_RNG_CFG_PUB_ID,		sizeof(uint8),
	"range service public instance id"},
	{"resolution",	NAN_RNG_CFG_RESOLUTION,		sizeof(uint32),
	"resolution of distance"},
	{"interval",	NAN_RNG_CFG_INTERVAL,		sizeof(uint32),
	"interval in TU b/w ranging sessions"},
	{"indication",	NAN_RNG_CFG_INDICATION,		sizeof(uint8),
	"eventing condition, cont/ingress/egress/both"},
	{"ingress",	NAN_RNG_CFG_INGRESS_LIMIT,		sizeof(uint32),
	"ingress distance condition"},
	{"egress",	NAN_RNG_CFG_EGRESS_LIMIT,		sizeof(uint32),
	"egress distance condition"},
	{"result",	NAN_RNG_CFG_RESULT_REQUIRED,		0,
	"report required from peer"},
	{"auto",	NAN_RNG_CFG_AUTO_RESPONSE,		0,
	"auto respond to range req"},
	{"status",	NAN_RNG_CFG_RANGE_STATUS,		sizeof(uint8),
	"host status whether accepted or rejected"},
};

const nan_rng_config_param_info_t *
nan_lookup_rng_config_param(char *param_name)
{
	int i = 0;
	const nan_rng_config_param_info_t *param_p = &nan_rng_config_param_info[0];

	for (i = 0; i < (int)ARRAYSIZE(nan_rng_config_param_info); i++) {
		if (stricmp(param_p->name, param_name) == 0) {
			return param_p;
		}
		param_p++;
	}

	return NULL;
}

static int
wl_nan_subcmd_range_req(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	wl_nan_range_req_t *range_req = NULL;
	int ret = BCME_OK;
	uint16 len = 0;
	const nan_rng_config_param_info_t *param_p = NULL;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if (*argv == NULL) {
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	range_req = (wl_nan_range_req_t *)iov_data;


	if (*avail_len < NAN_RNG_REQ_IOV_LEN) {
		/* buffer is insufficient */
		ret = BCME_NOMEM;
		goto exit;
	}

	*is_set = FALSE;

	/* Parse optional args. Validity is checked by discovery engine */
	while (*argv != NULL) {
		if ((param_p = nan_lookup_rng_config_param(*argv)) == NULL) {
			ret = BCME_USAGE_ERROR;
			goto exit;
		}
		/*
		 * Skip param name
		 */
		argv++;
		switch (param_p->id) {

		case NAN_RNG_CFG_PUB_ID:
			if (*argv != NULL) {
				range_req->publisher_id = (uint8)(atoi(*argv));
				argv++;
			}
			break;

		case NAN_RNG_CFG_PEER:
			if (*argv != NULL) {
				if (!wl_ether_atoe(argv[0], &range_req->peer)) {
					printf("Malformed MAC address parameter\n");
					break;
				}
				argv++;
			}
			break;

		case NAN_RNG_CFG_INDICATION:
			if (*argv != NULL) {
				range_req->indication = (uint8)(atoi(*argv));
				argv++;
			}
			break;

		case NAN_RNG_CFG_RESOLUTION:
			if (*argv != NULL) {
				range_req->resolution = (uint8)(atoi(*argv));
				argv++;
			}
			break;

		case NAN_RNG_CFG_INGRESS_LIMIT:
			if (*argv != NULL) {
				range_req->ingress = (uint32)(atoi(*argv));
				argv++;
			}
			break;

		case NAN_RNG_CFG_EGRESS_LIMIT:
			if (*argv != NULL) {
				range_req->egress = (uint32)(atoi(*argv));
				argv++;
			}
			break;

		case NAN_RNG_CFG_INTERVAL:
			if (*argv != NULL) {
				range_req->interval = (uint16)(atoi(*argv));
				argv++;
			}
			break;

		default:
			return BCME_ERROR;
			break;
		}
	}
	len += NAN_RNG_REQ_IOV_LEN;
	*avail_len -= len;
exit:
	return ret;
}

static int
wl_nan_subcmd_range_auto(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	int ret = BCME_OK;
	uint16 len = 0;
	uint8 auto_accept = 0;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if (*argv == NULL) {
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	if (*avail_len < sizeof(auto_accept)) {
		/* buffer is insufficient */
		ret = BCME_NOMEM;
		goto exit;
	}

	*is_set = TRUE;

	*iov_data = auto_accept = (uint8)(atoi(*argv));

	len += sizeof(auto_accept);
	*avail_len -= len;
exit:
	return ret;

}

static int
wl_nan_subcmd_range_resp(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	wl_nan_range_resp_t *range_resp = NULL;
	int ret = BCME_OK;
	uint16 len = 0;
	uint8 flags = 0;
	const nan_rng_config_param_info_t *param_p = NULL;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if (*argv == NULL) {
		ret = BCME_USAGE_ERROR;
		goto exit;
	}
	range_resp = (wl_nan_range_resp_t *)iov_data;

	if (*avail_len < NAN_RNG_RESP_IOV_LEN) {
		/* buffer is insufficient */
		ret = BCME_NOMEM;
		goto exit;
	}

	range_resp->range_id = (uint8)(atoi(*argv));
	argv++;
	argc--;

	*is_set = TRUE;

	/* Parse optional args. Validity is checked by discovery engine */
	while (*argv != NULL) {
		if ((param_p = nan_lookup_rng_config_param(*argv)) == NULL) {
			ret = BCME_USAGE_ERROR;
			goto exit;
		}
		/*
		 * Skip param name
		 */
		argv++;
		switch (param_p->id) {

		case NAN_RNG_CFG_INDICATION:
			if (*argv != NULL) {
				range_resp->indication = (uint8)(atoi(*argv));
				argv++;
			}
			break;

		case NAN_RNG_CFG_RESOLUTION:
			if (*argv != NULL) {
				range_resp->resolution = (uint8)(atoi(*argv));
				argv++;
			}
			break;

		case NAN_RNG_CFG_INGRESS_LIMIT:
			if (*argv != NULL) {
				range_resp->ingress = (uint32)(atoi(*argv));
				argv++;
			}
			break;

		case NAN_RNG_CFG_EGRESS_LIMIT:
			if (*argv != NULL) {
				range_resp->egress = (uint32)(atoi(*argv));
				argv++;
			}
			break;

		case NAN_RNG_CFG_INTERVAL:
			if (*argv != NULL) {
				range_resp->interval = (uint16)(atoi(*argv));
				argv++;
			}
			break;

		case NAN_RNG_CFG_RESULT_REQUIRED:
			flags |= NAN_RANGE_FLAG_RESULT_REQUIRED;
			break;

		case NAN_RNG_CFG_RANGE_STATUS:
			if (*argv != NULL) {
				range_resp->status = (uint8)(atoi(*argv));
				argv++;
			}
			break;

		default:
			return BCME_ERROR;
			break;
		}
	}
	range_resp->flags = flags;
	len += NAN_RNG_RESP_IOV_LEN;
	*avail_len -= len;
exit:
	return ret;
}

static int
wl_nan_subcmd_range_cancel(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	wl_nan_range_id range_id = 0;
	int ret = BCME_OK;
	uint16 len = 0;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if (*argv == NULL) {
		ret = BCME_USAGE_ERROR;
		goto exit;
	}

	if (*avail_len < sizeof(range_id)) {
		/* buffer is insufficient */
		ret = BCME_NOMEM;
		goto exit;
	}

	*is_set = TRUE;

	*iov_data = range_id = (uint8)(atoi(*argv));

	len += sizeof(range_id);
	*avail_len -= len;
exit:
	return ret;
}

static int
wl_nan_subcmd_soc_chans(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	wl_nan_social_channels_t *soc;
	int ret = BCME_OK;
	uint16 len = 0;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = sizeof(wl_nan_social_channels_t);
	} else {
		if (*avail_len < sizeof(wl_nan_social_channels_t)) {
			/* buffer is insufficient */
			ret = BCME_NOMEM;
			goto exit;
		}
		*is_set = TRUE;
		if (!argv)
			goto exit;

		if (argv) {
			soc = (wl_nan_social_channels_t *)iov_data;
			soc->soc_chan_2g = (int8)atoi(*argv);
		}
		argv++;
		if (!argv)
			goto exit;
		if (argv) {
			soc->soc_chan_5g = (int8)atoi(*argv);
		}
		len += sizeof(wl_nan_social_channels_t);
	}
	*avail_len -= len;
exit:
	return ret;
}

static int
wl_nan_subcmd_awake_dws(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	wl_nan_awake_dws_t *d;
	int ret = BCME_OK;
	uint16 len = 0;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = sizeof(wl_nan_awake_dws_t);
	} else {
		if (*avail_len < sizeof(wl_nan_awake_dws_t)) {
			/* buffer is insufficient */
			ret = BCME_NOMEM;
			goto exit;
		}
		*is_set = TRUE;
		d = (wl_nan_awake_dws_t *)iov_data;
		if (!argv)
			goto exit;

		if (argv) {
			d->dw_interval_2g = (int8)atoi(*argv);
		}
		argv++;
		if (!argv)
			goto exit;
		if (argv) {
			d->dw_interval_5g = (int8)atoi(*argv);
		}
		len += sizeof(wl_nan_awake_dws_t);
	}
	*avail_len -= len;
exit:
	return ret;
}

static int
wl_nan_subcmd_sbcn_rssi_notif_thld(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	wl_nan_rssi_notif_thld_t *t;
	int ret = BCME_OK;
	uint16 len = 0;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = sizeof(wl_nan_rssi_notif_thld_t);
	} else {
		if (*avail_len < sizeof(wl_nan_rssi_notif_thld_t)) {
			/* buffer is insufficient */
			ret = BCME_NOMEM;
			goto exit;
		}
		*is_set = TRUE;
		t = (wl_nan_rssi_notif_thld_t *)iov_data;
		if (!argv)
			goto exit;

		if (argv) {
			t->bcn_rssi_2g = (int8)atoi(*argv);
		}
		argv++;
		if (!argv)
			goto exit;
		if (argv) {
			t->bcn_rssi_5g = (int8)atoi(*argv);
		}
		len += sizeof(wl_nan_rssi_notif_thld_t);
	}
	*avail_len -= len;
exit:
	return ret;
}

static int
wl_nan_subcmd_sbcn_rssi_thld(void *wl, const wl_nan_sub_cmd_t *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	wl_nan_rssi_thld_t *d;
	int ret = BCME_OK;
	uint16 len = 0;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argc);

	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = sizeof(wl_nan_rssi_thld_t);
	} else {
		if (*avail_len < sizeof(wl_nan_rssi_thld_t)) {
			/* buffer is insufficient */
			ret = BCME_NOMEM;
			goto exit;
		}
		if (argc != 4) {
			ret = BCME_BADARG;
			goto exit;
		}
		*is_set = TRUE;
		d = (wl_nan_rssi_thld_t *)iov_data;
		d->rssi_close_2g = (int8)atoi(*argv++);
		d->rssi_mid_2g = (int8)atoi(*argv++);
		d->rssi_close_5g = (int8)atoi(*argv++);
		d->rssi_mid_5g = (int8)atoi(*argv++);
		len += sizeof(wl_nan_rssi_thld_t);
	}
	*avail_len -= len;
exit:
	return ret;
}

static int
wl_nan_subcmd_max_peers(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{

	int res = BCME_OK;
	uint8 max_peers = 0;
	uint16 len = 0;
	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
		len = 0;
	} else {
		*is_set = TRUE;
		len = sizeof(max_peers);

		if (len > *avail_len) {
			printf("Buf short, requested:%d, available:%d\n",
					len, *avail_len);
			return BCME_BUFTOOSHORT;
		}
		max_peers = (uint8)atoi(*argv);
		if (max_peers > NAN_MAX_PEERS)
			max_peers = NAN_MAX_PEERS;
		memcpy(iov_data, &max_peers, sizeof(max_peers));
	}
	*avail_len -= len;
	return res;
}

/*  WFA testmode operation */
static int
wl_nan_subcmd_cfg_wfa_testmode(void *wl, const wl_nan_sub_cmd_t  *cmd, int argc, char **argv,
	bool *is_set, uint8 *iov_data, uint16 *avail_len)
{
	wl_nan_wfa_testmode_t flags = 0;

	UNUSED_PARAMETER(wl);
	UNUSED_PARAMETER(cmd);

	/* If no more parameters are passed, it is GET command */
	if ((*argv == NULL) || argc == 0) {
		*is_set = FALSE;
	} else {
		char *endptr = NULL;

		*is_set = TRUE;
		flags = strtoul(*argv, &endptr, 0);

		if (flags & ~WL_NAN_WFA_TM_FLAG_MASK) {
			return BCME_BADARG;
		}
		if (*avail_len < sizeof(flags)) {
			return BCME_BUFTOOSHORT;
		}

		flags = htod32(flags);
		memcpy(iov_data, &flags, sizeof(flags));
		*avail_len -= sizeof(flags);
	}

	return BCME_OK;
}

#endif /* WL_NAN */
