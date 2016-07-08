/*
 * Header file used by Visual Studio Windows8.x projects
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlconfig_win_nic_win8x.h 511191 2014-10-29 01:09:26Z $
 */

/*
 * For all drivers
 */
#define NDIS
#define NDIS_MINIPORT_DRIVER
#define NDIS_WDM 1
#define WDM
#define DELTASTATS
#define BCMDRIVER
#define NDIS_DMAWAR
#define BCMDBG_TRAP
#define UNRELEASEDCHIP
#define PKTQ_LOG
#define WLWSEC
#define WL11H
#define WLCSA
#define WLQUIET
#define WLTPC
#define WL11D
#define WLCNTRY
#define WLEXTLOG
#define WLSCANCACHE
#define EXT_STA
#define WL_MONITOR
#define IBSS_PEER_GROUP_KEY
#define IBSS_PEER_DISCOVERY_EVENT
#define IBSS_PEER_MGMT
#define AP
#define WLVIF
#define WL_OIDS
#define WDS
#define APCS
#define WME_PER_AC_TX_PARAMS
#define WME_PER_AC_TUNING
#define STA
#define WLOVERTHRUSTER
#define WLLED
#define WME
#define WL11N
#define DBAND
#define WLRM
#define WLCNT
#define WLCNTSCB
#define WLCOEX
#define BCMSUP_PSK
#define BCMINTSUP
#define WLCAC
#define BCMAUTH_PSK
#define BCMCCMP
#define WIFI_ACT_FRAME
#define BCMDMA32
#define WLAMSDU
#define WLAMSDU_SWDEAGG
#define WLAMPDU
#define WLAMPDU_HW
#define WLAMPDU_MAC
#define WLAMPDU_PRECEDENCE
#define BTC2WIRE
#define WL_ASSOC_RECREATE
#define WLP2P
#define WL_BSSCFG_TX_SUPR
#define WLP2P_UCODE
#define WLMCNX
#define WLMCHAN
#define WL_MULTIQUEUE
#define WLBTAMP
#define WLBTWUSB
#define BCMNVRAMR
#define BCMNVRAMW
#define WLDIAG
#define WLTINYDUMP
#define BCM_DCS
#define WLPFN
#define NLO
#define MFP
#define IO80211P2P
#define WL11AC
#define PPR_API
#define BCMECICOEX
#define WLATF
#define WLC_HOSTOID
#define WPP_TRACING
#define WL_WLC_SHIM
#define WL_WLC_SHIM_EVENTS
/* #define BISON_SHIM_PATCH */
/* #define CARIBOU_SHIM_PATCH */

/*
 * For NIC drivers only
 */
#if defined(WLC_HIGH) && defined(WLC_LOW) && !defined(BCMDONGLEHOST)
#define MEMORY_TAG 'NWMB'
#define WLPIO
#define BCMDMA64OSL
#define WLOLPC
#define WOWL
#define TDLS_TESTBED
#define WLTDLS
#define CCA_STATS
#define ISID_STATS
#define TRAFFIC_MGMT
#define TRAFFIC_SHAPING
#define WL_BCN_COALESCING
#define WL_LPC
#define WL_BEAMFORMING
#define MULTICHAN_4313
#define GTK_RESET
#define SURVIVE_PERST_ENAB
#define WL_LTR
#define WL_PRE_AC_DELAY_NOISE_INT
#define PHYCAL_CACHING
#define WLWNM
#define WL_EXPORT_AMPDU_RETRY
#define BCMWAPI_WPI
#define BCMWAPI_WAI
#define SAVERESTORE
#define SR_ESSENTIALS
#endif /* #if defined(WLC_HIGH) && defined(WLC_LOW) && !defined(BCMDONGLEHOST) */

/*
 * For BMAC-HIGH drivers only
 */
#if defined(WLC_HIGH) && !defined(WLC_LOW) && !defined(BCMDONGLEHOST)
#define BCMBUSTYPE RPC_BUS
#define OSLREGOPS
#define BCMUSB
#define PREATTACH_NORECLAIM
#define BCMDBUS
#define BCMWDF
#define BCMTRXV2
#define BCM_DNGL_EMBEDIMAGE
#define MEMORY_TAG 'DWMB'
#define WLCQ
#define SEQ_CMDS
#define WL_FW_DECOMP
#define EMBED_IMAGE_43143b0
#define EMBED_IMAGE_43236b
#define EMBED_IMAGE_43526a
#define EMBED_IMAGE_43526b
#define EMBED_IMAGE_43569a0
#define NDIS_SS
#endif /* #if defined(WLC_HIGH) && !defined(WLC_LOW) && !defined(BCMDONGLEHOST) */

/*
 * For DHD drivers only
 * (placeholder for now, DHD drivers currently do not use this file)
 */
#if defined(BCMDONGLEHOST)
#endif /* #if defined(BCMDONGLEHOST) */
