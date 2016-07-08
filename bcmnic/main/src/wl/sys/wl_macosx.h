/*
 * Mac OS X specific driver header file.
 * Broadcom 802.11 Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wl_macosx.h 514073 2014-11-08 20:46:19Z $
 */
/* FILE-CSTYLED */

#ifndef _wl_macosx_h
#define _wl_macosx_h
#include <IOKit/apple80211/IO80211Controller.h>
#ifdef IO80211P2P
#include <IOKit/apple80211/IO80211P2PInterface.h>
#endif
#ifdef BONJOUR
#include <mDNSOffloadUserClient/mDNSOffloadUserClient.h>
#endif //BONJOUR

extern "C" {
	#include <wl_radiotap.h>
}

typedef struct txflc {
	mbool txFlowControl;		//wlc/Per-port flowcontrol status, per prio
} txflc_t;

// Scan Result linked list structure
typedef struct scan_result {
	SLIST_ENTRY(scan_result) entries;
	struct apple80211_scan_result asr;
} scan_result_t;

typedef struct intf_info_blk {
	uint32		association_status;	// cached value for getASSOCIATION_STATUS
	uint32		cached_assocstatus;	// cached status while setSSID pending
	uint16		deauth_reason;		// cached value for getDEAUTH
	uint16		link_deauth_reason;	// cached value for setLinkState
	struct ether_addr deauth_ea;		// cached value for getDEAUTH

	u_int32_t	authtype_lower;		// cached value for getAUTH_TYPE
	u_int32_t	authtype_upper;		// cached value for getAUTH_TYPE
	struct apple80211_last_rx_pkt_data rxpd;	// Stats about the last recvd. pkt on i/f

} intf_info_blk_t;

typedef struct chip_power_levels {
	uint	chipnum;
	uint	chiprev;
	int	down;		/* mW when down */
	int	idle_pm2;	/* mW when associated, idle, PM 2 */
	int	idle_pm0;	/* mW when associated, idle, PM 0 */
} chip_power_levels_t;

#if VERSION_MAJOR > 9
typedef struct link_quality_sample {
        int rssi_bucket;
        int tx_rate_bucket;
        uint32 now; // timestamp
        int rssi;
        int tx_rate;
} link_quality_sample_t;
#endif

#if VERSION_MAJOR > 8
#define AAPLPWROFF
#endif

#ifdef IO80211P2P
#define IFMTHD( ifobj, mthd, ... ) ( OSDynamicCast( IO80211Interface, ifobj ) ?					\
				     ((IO80211Interface *)ifobj)->mthd( __VA_ARGS__ ) :	\
				     ((IO80211VirtualInterface *)ifobj)->mthd( __VA_ARGS__ ) )
#else
#define IFMTHD( ifobj, mthd, ... ) ((IO80211Interface *)ifobj)->mthd( __VA_ARGS__ )
#endif

#ifdef IO80211P2P
#define IFINFOBLKP( ifobj ) ( OSDynamicCast( AirPort_Brcm43xxInterface, ifobj ) ? \
			     &((AirPort_Brcm43xxInterface *)ifobj)->intf_info :	\
			     &((AirPort_Brcm43xxP2PInterface *)ifobj)->intf_info )
#else
#define IFINFOBLKP( ifobj ) (&((AirPort_Brcm43xxInterface *)ifobj)->intf_info)
#endif

#ifdef WLP2P
#define WLC_P2P_CAP(p)	wlc_p2p_cap(p->p2p)
#else
#define WLC_P2P_CAP(p)	FALSE
#endif /* WLP2P */

#ifdef WL_TIMERSTATS
#define WLINC_TIMERCNT(a)	a++
#else
#define WLINC_TIMERCNT(a)
#endif /* WL_TIMERSTATS */

#ifdef IO80211P2P
#ifdef MACOS_VIRTIF
#error "MACOS_VIRTIF cannot be defined for IO80211P2P"
#endif
struct wl_if {
	struct wl_info *wl;
	struct wlc_if *wlcif;		/* wlc interface handle */
	IO80211P2PInterface *virtIf;

	char ifname[IFNAMSIZ];
	uint unit;
	txflc_t intf_txflowcontrol;		// contains all flow control flags and queues
};
#else
#ifdef MACOS_VIRTIF
struct wl_if {
	struct wl_info *wl;
	struct wlc_if *wlcif;		/* wlc interface handle */
	IOEthernetInterface *virtIf;

	char ifname[IFNAMSIZ];
	uint unit;
	txflc_t intf_txflowcontrol;		// contains all flow control flags and queues
};
#else
struct wl_if;
#endif /* MACOS_VIRTIF */
#endif /* IO80211P2P */

#ifdef WLTCPKEEPA
enum {
    Keepalive_addr_type_ipv4 = 0,
    Keepalive_addr_type_ipv6 = 1
};

struct keepalive_ipv4 {			// IPv4 keepalive structure passed to firmware
    UInt8   addr_type;			// 0 for ipv4, 1 for ipv6
    UInt32  timeout;			// timeout in seconds
    in_addr_t local_ip;			// local, remote ip addresses
    in_addr_t remote_ip;
    IOEthernetAddress remoteMacAddress; // MAC address of destination
    in_port_t local_port;
    in_port_t remote_port;
    UInt32  sequence_number;
    UInt32  ack_number;
    UInt16  window_size;
} __attribute__((__packed__));

typedef struct  {
    union {
	struct keepalive_ipv4 ka4;
    } u;
} Keepalive_IP4_IP6;
#endif /* WLTCPKEEPA */

// Forward declarations of structs / classes needed
class AirPort_Brcm43xxP2PInterface;
class AirPort_Brcm43xxInterface;
class AirPort_Brcm43xxTimer;
class Brcm43xx_InputQueue;

struct wlc_bss_list;
typedef struct wlc_bss_list wlc_bss_list_t;

//===========================================================================
// AirPort_Brcm43xx class
//===========================================================================
#ifdef BONJOUR
typedef struct dnsSymList {
	struct dnsSymList *next;        // next in the list or null
	unsigned int offset;            // start of the string
} dnsSymList;
#endif //BONJOUR
class AirPort_Brcm43xx : public IO80211Controller
{
	OSDeclareDefaultStructors(AirPort_Brcm43xx)

public:
	// Methods inherited from IOService (or its super)
	virtual bool		init( OSDictionary *properties );
	virtual	IOService *	probe(IOService * provider, SInt32 *score);
	bool		start( IOService *provider );
	void		stop( IOService *provider );
	IOReturn	enable( IONetworkInterface * aNetif );
	IOReturn	disable( IONetworkInterface * aNetif );
	static IOReturn terminate_gated(OSObject * owner, void * arg0, void * arg1, void * arg2, void * arg3);
	bool	        terminate(IOOptionBits options);
	IOReturn	registerWithPolicyMaker(IOService *policyMaker);
	IOReturn	powerStateWillChangeTo(IOPMPowerFlags, unsigned long, IOService*);
#ifdef AAPLPWROFF
	IOReturn	powerStateDidChangeTo( IOPMPowerFlags capabilities, unsigned long stateNumber, IOService * whatDevice );
	IOReturn	setPowerState( unsigned long powerStateOrdinal, IOService * whatDevice );
#endif /* AAPLPWROFF */
#ifdef IO80211P2P
	AirPort_Brcm43xxP2PInterface *getVirtIf( OSObject *interface) const;
	wl_if_t *createVirtIf(struct wlc_if *wlcif, uint unit);
	void destroyVirtIf(AirPort_Brcm43xxP2PInterface *removeIf);
	void validateIfRole(struct wlc_if *wlcif);
#endif /* IO80211P2P */
#ifdef BONJOUR
	IOReturn		mDNS_Callback(mDNSOffloadUserClient *userclient, mDNSHandoff *handoff, void *ref);
	virtual IOReturn	newUserClient(task_t owningTask, void * securityID, UInt32 type, IOUserClient ** handler );
	/* Flatten related */
	bool                        flatten(bool suppress);
	bool                        flat_append(void *src, UInt32 length);
	UInt8                       flat_count_solicited();
	UInt8                       flat_mask_to_prefixlen(struct in6_addr *ipv6mask);
	bool                        flat_ipv6_to_solicited(struct in6_addr *src, struct in6_addr *solicited);
	UInt16                      flat_count_rr_of_type(int dnsServiceType);
	UInt16                      flat_count_rr_non_empty_txt();
	bool                        flat_append_rr(void *rr, dnsSymList **symList, UInt8 *newbase);
	void                        flat_get_rr_type_len(void *rr, UInt16 *rr_type, UInt16 *rr_len);
	UInt16                      flat_rr_get_flat_length(void *rr, dnsSymList **symList, UInt8 *newbase);
	UInt8 *                     flat_rr_copy(void *rr, UInt8 *dest, dnsSymList **symList, UInt8 *newbase);
	bool                        flat_append_subtypes(void *rr, dnsSymList **symList, UInt8 *newbase);
#endif //BONJOUR
#ifdef WLTCPKEEPA
	int 			    locate_keepalive_data_new_uc();
    	bool			    flat_extract_keepalive_data(int recordnum, Keepalive_IP4_IP6 *kr);
	bool 			    flat_extract_keepalive_data_vers2( int recordnum,
					wl_mtcpkeep_alive_timers_pkt_t *keep_timers,
					wl_mtcpkeep_alive_conn_pkt_t *tcpkeepconn);
    	bool			    flat_parse_keepalive_data_v4(char *buf, struct keepalive_ipv4 *kr);
#endif /* WLTCPKEEPA */
#ifdef MACOS_VIRTIF
	wl_if_t *createVirtIf(struct wlc_if *wlcif, uint unit);
	void destroyVirtIf(wl_if_t *wlif);
	UInt32		outputPacketVirtIf( mbuf_t m, void *param);
	struct wlc_if *apif_wlcif();
	wl_if_t *getapif();
#endif
	// Methods inherited from IONetworkController
	UInt32		outputPacket( mbuf_t m, void *param);

	bool		configureInterface( IONetworkInterface *netif );
	IONetworkInterface *createInterface();
#ifdef WLCSO
	void		outputChecksumOffload(mbuf_t m, uint32 checksumDemand);
	void		inputChecksumOffload(mbuf_t m);
	virtual IOReturn	getChecksumSupport(UInt32 *checksumMask, UInt32 checksumFamily, bool isOutput);
#endif /* WLCSO */
	const OSString	*newVendorString() const;
	const OSString	*newModelString() const;

	// Methods inherited from IOEthernetController
	IOReturn	getHardwareAddress( IOEthernetAddress *addr );
	IOReturn	setHardwareAddress( const IOEthernetAddress *addr );
	IOReturn	setPromiscuousMode( bool active );
	void		populateMulticastMode( );
	IOReturn	setMulticastMode( IOEnetMulticastMode mode );
	void		populateMulticastList( );
	IOReturn	setMulticastList( IOEthernetAddress *addrs, UInt32 count );

	// my own public methods
	mbuf_t          coalesce_output(mbuf_t m, bool coalesce = false, bool free_orig = true);
	void		inputPacketQueued(mbuf_t m, wl_if_t *wlif = NULL, int16 rssi = 0);
	bool		checkBoardId( const char *cmpboardid );
	bool		canLoad( IOService *provider );

	const char*	getIFName();
	const char*	getIFName( OSObject *netIf, char *name);


	// IOVAR and IOCTL API functions
	virtual int wlIovarOp(const char *name, void * params, size_t p_len, void *buf, size_t len, bool set,
				OSObject *iface);
	virtual int wlIovarGetInt(const char *name, int *arg) const;
	int wlIovarSetInt(const char *name, int arg);
	virtual int wlIoctl(uint cmd, void *arg, size_t len, bool set, OSObject *iface);
	virtual int wlIoctlGet(uint cmd, void *arg, size_t len, OSObject *iface) const;
	int wlIoctlGetInt(uint cmd, int *arg) const;
	int wlIoctlSetInt(uint cmd,  int arg);

#ifdef IO80211P2P
	IO80211VirtualInterface * createVirtualInterface( struct ether_addr * addr, UInt32 role );
	SInt32 enableVirtualInterface( IO80211VirtualInterface * vif );
	SInt32 disableVirtualInterface( IO80211VirtualInterface * vif );
	UInt32 outputActionFrame( OSObject * interface, mbuf_t m );
	UInt32 bpfOutputPacket( OSObject * interface, UInt32 dlt, mbuf_t m );
	UInt32 bpfOutput80211Radio( OSObject * interface, mbuf_t m );
	ratespec_t calcRspecFromRTap(uint8 *rtap_header);
	bool rtapFlags(uint8 *rtap_header, uint8* flags);
	void rtapParseInit(radiotap_parse_t *rtap, uint8 *rtap_header);
	void rtapParseReset(radiotap_parse_t *rtap);
	void* rtapParseFindField(radiotap_parse_t *rtap, uint search_idx);
	int ConfigP2PFeatures(AirPort_Brcm43xxP2PInterface *intf, int features, bool enab);
#endif /* IO80211P2P */
	UInt32		monitorPacketHeaderLength( IO80211Interface * interface );
	UInt32		monitorDLT( IO80211Interface * interface );
	SInt32		monitorModeSetEnabled( IO80211Interface * interface, bool enabled, UInt32 dlt );

	void		wlEvent( char *ifname, wlc_event_t *evt );
	void		wlEventSync( char *ifname, wlc_event_t *evt );
	void		MacAuthEvent( char *ifname, intf_info_blk_t *blk, UInt32 status, UInt32 err, UInt8 *addr );
	void		txFlowControlClearAll();

	static int	oscallback_dotxstatus( struct wlc_info *wlc, void *scb, void *p, void *txs,  uint status);

	// public data members (bad programmer!)
	// P2P virtual interface considerations
	txflc_t 	intf_txflowcontrol;	// contains all flow control flags and queues

	bool		coalesce_packet;	// Coalesce output packet for few configurations
						// to avoid taking performance hit.

	bool		resetCountryCode;	// Country code needs to be reset on next down
#ifdef FACTORY_MERGE
	bool		disableFactory11d;	// Disable 11d for factory testing
	bool		disableFactoryPowersave; //Disable Power save mode for factory testing
	bool		disableFactoryRoam;	//Disable Roam for factory testing
#endif

	struct ether_addr configured_bssid;	// setBSSID() value for association restrictions
	struct ether_addr mscan_bssid_list[APPLE80211_MAX_SCAN_BSSID_CNT];	// used to filter for multiple scan request
	uint8 num_mscan_bssid;
	bool inMultiScan;
	struct wlc_info	*pWLC;
	wlc_pub_t	*pub;
	// One case may be for MBSS with SWAP
	const static int unit = 0;
	osl_t		*osh;
	AirPort_Brcm43xxTimer	*timers;
	bool wowl_cap;
	bool wowl_cap_platform;
	IOPCIDevice *parent_dev;

	int cached_PM_state;
	int cached_Apple_PM_state;
	int cached_nmode_state;
	int cached_vhtmode_state;
	wl_rssi_ant_t cached_rssi_ant;

	uint32 sgi_cap;
	bool		avsEnabled;			// Which DLTs are being tapped
	bool		ieee80211Enabled;
	bool		bsdEnabled;
	bool		ppiEnabled;
	bool		skip_flowcontrol;  //by pass the flow control check so in wlc_tx_enq the priority can be changed

#if VERSION_MAJOR > 9
	// P2P virtual interface considerations
	link_quality_sample_t last_sample;	// Sample that was 'sent' last time.
	uint32 max_txrate;
	uint32 rssi_hysteresis;
	uint32 tx_rate_hysteresis;
	struct apple80211_link_qual_event_data link_qual_config;

	u_int8_t dbg_flags_map[APPLE80211_MAP_SIZE(APPLE80211_V_DBG_MAX + 1)];
	bool pwr_throttle_state;	// For output
	uint32 timeNow;
#if defined(WLBTCPROF) && defined(APPLE80211_IOC_BTCOEX_PROFILES)
	struct apple80211_btc_profiles_data btc_in_profile[BTC_SUPPORT_BANDS];  //0-2.4G band, 1-5G band
	struct apple80211_btc_config_data btc_in_config;
#endif /* WLBTCPROF && APPLE80211_IOC_BTCOEX_PROFILES */
#endif

	// Methods inherited from IO80211Controller

	// IOCTL Handlers
	struct wlc_if *wlLookupWLCIf( OSObject *interface) const;

#if VERSION_MAJOR < 10
	SInt32		apple80211_ioctl( IO80211Interface * interface, ifnet_t ifn, u_int32_t cmd, void * data );
#else
#if defined(IO80211P2P) || (VERSION_MAJOR > 10)
	SInt32		apple80211_ioctl( IO80211Interface * interface, IO80211VirtualInterface * vif, ifnet_t ifn, unsigned long cmd, void * data );
#else
	SInt32		apple80211_ioctl( IO80211Interface * interface, ifnet_t ifn, unsigned long cmd, void * data );
#endif /* IO80211P2P */

#endif /* VERSION_MAJOR < 10 */

	UInt32  	hardwareOutputQueueDepth (IO80211Interface *interface );
	SInt32  	getSSID( OSObject * interface, struct apple80211_ssid_data * sd );
	OSData *	getCIPHER_KEY( OSObject * interface, int index );
	SInt32		getCHANNEL( OSObject * interface, struct apple80211_channel_data * cd );
	SInt32		getTXPOWER( OSObject * interface, struct apple80211_txpower_data * td );
	SInt32		getBSSID( OSObject * interface, struct apple80211_bssid_data * bd );
	SInt32		getCARD_CAPABILITIES( OSObject * interface, struct apple80211_capability_data * cd );
	SInt32		getSTATE( OSObject * interface, struct apple80211_state_data * sd );
	SInt32		getPHY_MODE( OSObject * interface, struct apple80211_phymode_data * pd );
	SInt32		getOP_MODE( OSObject * interface, struct apple80211_opmode_data * od );
	SInt32		getMCS_INDEX_SET( OSObject * interface, struct apple80211_mcs_index_set_data * misd );
#ifdef APPLE80211_IOC_VHT_MCS_INDEX_SET
	SInt32		getVHT_MCS_INDEX_SET( OSObject * interface, struct apple80211_vht_mcs_index_set_data * vmisd );
#endif
	SInt32		getRSSI( OSObject * interface, struct apple80211_rssi_data * rd );
	SInt32		getNOISE( OSObject * interface, struct apple80211_noise_data * nd );
	SInt32		getINT_MIT( OSObject * interface, struct apple80211_intmit_data * imd );
	SInt32		getPOWER( OSObject * interface, struct apple80211_power_data * pd );
	SInt32		getSCAN_RESULT( OSObject * interface, struct apple80211_scan_result ** scan_result );
	SInt32		getASSOCIATION_STATUS( OSObject * interface, struct apple80211_assoc_status_data * asd );
	SInt32		getAUTH_TYPE( OSObject * interface, struct apple80211_authtype_data * ad );
	SInt32		getRATE( OSObject * interface, struct apple80211_rate_data * rd );
	SInt32		getSTATUS_DEV( OSObject * interface, struct apple80211_status_dev_data * dd );
	SInt32		getSUPPORTED_CHANNELS( OSObject * interface, struct apple80211_sup_channel_data  * sd );
	SInt32		getLOCALE( OSObject * interface, struct apple80211_locale_data * ld );
	SInt32		getAP_MODE( OSObject * interface, struct apple80211_apmode_data * ad );
	SInt32		getPROTMODE( OSObject * interface, struct apple80211_protmode_data * pd );
	SInt32		getFRAG_THRESHOLD( OSObject * interface, struct apple80211_frag_threshold_data * td );
	SInt32		getRATE_SET( OSObject * interface, struct apple80211_rate_set_data * rd );
	SInt32		getSHORT_SLOT( OSObject * interface, struct apple80211_short_slot_data * sd );
	SInt32		getMULTICAST_RATE( OSObject * interface, struct apple80211_rate_data * rd );
	SInt32		getSHORT_RETRY_LIMIT( OSObject * interface, struct apple80211_retry_limit_data * rld );
	SInt32		getLONG_RETRY_LIMIT( OSObject * interface, struct apple80211_retry_limit_data * rld );
	SInt32		getTX_ANTENNA( OSObject * interface, struct apple80211_antenna_data * ad );
	SInt32		getRX_ANTENNA( OSObject * interface, struct apple80211_antenna_data * ad );
	SInt32		getANTENNA_DIVERSITY( OSObject * interface, struct apple80211_antenna_data * ad );
	SInt32		getDTIM_INT( OSObject * interface, apple80211_dtim_int_data * dd );
	SInt32		getSTATION_LIST( OSObject * interface, struct apple80211_sta_data * sd );
	SInt32		getDRIVER_VERSION( OSObject * interface, struct apple80211_version_data * vd );
	SInt32		getHARDWARE_VERSION( OSObject * interface, struct apple80211_version_data * vd );
	SInt32		getPOWERSAVE(OSObject * interface, struct apple80211_powersave_data *pd);
	SInt32		getROM( OSObject * interface, struct apple80211_rom_data * rd );
	SInt32		getRAND( OSObject * interface, struct apple80211_rand_data * rd );
	// SInt32	getBACKGROUND_SCAN( OSObject * interface, struct apple80211_scan_data *sd );
	SInt32		getRSN_IE( OSObject * interface, struct apple80211_rsn_ie_data * rid );
	SInt32		getAP_IE_LIST( OSObject * interface, struct apple80211_ap_ie_data * ied );
	SInt32		getSTATS( OSObject * interface, struct apple80211_stats_data * sd );
	SInt32		getDEAUTH( OSObject * interface, struct apple80211_deauth_data *dd );
	SInt32		getCOUNTRY_CODE( OSObject * interface, struct apple80211_country_code_data * ccd );
	SInt32		getLAST_RX_PKT_DATA( OSObject * interface, struct apple80211_last_rx_pkt_data * pd );
	SInt32		getRADIO_INFO( OSObject * interface, struct apple80211_radio_info_data * rid );
	SInt32		getGUARD_INTERVAL(OSObject * interface, struct apple80211_guard_interval_data * gid);

	SInt32		getMIMO_POWERSAVE( OSObject * interface, struct apple80211_powersave_data * pd )
			{ return EOPNOTSUPP; }

	SInt32		getMCS(	OSObject * interface, struct apple80211_mcs_data * md );
#ifdef APPLE80211_IOC_MCS_VHT
	SInt32		getMCS_VHT( OSObject * interface, struct apple80211_mcs_vht_data * mvd );
#endif

	SInt32		getRIFS( OSObject * interface, struct apple80211_rifs_data * rd )
			{ return EOPNOTSUPP; }

	SInt32		getLDPC( OSObject * interface, struct apple80211_ldpc_data * ld );

	SInt32		getMSDU( OSObject * interface, struct apple80211_msdu_data * md );
	SInt32		getMPDU( OSObject * interface, struct apple80211_mpdu_data * md );
	SInt32		getBLOCK_ACK( OSObject * interface, struct apple80211_block_ack_data * bad );

	SInt32		getPLS( OSObject * interface, struct apple80211_pls_data * pd )
			{ return EOPNOTSUPP; }

	SInt32		getPSMP( OSObject * interface, struct apple80211_psmp_data * pd )
			{ return EOPNOTSUPP; }

	SInt32		getPHY_SUB_MODE( OSObject * interface, struct apple80211_physubmode_data * pd );
#ifdef SUPPORT_FEATURE_WOW
	SInt32		getWOW_PARAMETERS( OSObject * interface, struct apple80211_wow_parameter_data * wpd );
#endif

#ifdef APPLE80211_IOC_CDD_MODE
	SInt32  	getCDD_MODE( IO80211Interface * interface, struct apple80211_cdd_mode_data * cmd );
#endif
#ifdef APPLE80211_IOC_THERMAL_THROTTLING
	SInt32  	getTHERMAL_THROTTLING( IO80211Interface * interface, struct apple80211_thermal_throttling * tt );
#endif
#if VERSION_MAJOR > 9
	SInt32	getROAM_THRESH( OSObject * interface, struct apple80211_roam_threshold_data * rtd );

	// Apple change: hostapd support
	SInt32		getSTA_IE_LIST( OSObject * interface, struct apple80211_sta_ie_data * sid );
	SInt32		getKEY_RSC( OSObject * interface, struct apple80211_key * key );
	SInt32		getSTA_STATS( OSObject * interface, struct apple80211_sta_stats_data * ssd );

	SInt32		getLINK_QUAL_EVENT_PARAMS( OSObject * interface, struct apple80211_link_qual_event_data * lqd );
	SInt32		getVENDOR_DBG_FLAGS( OSObject * interface,
									 struct apple80211_vendor_dbg_data * vdd );

	// Apple change: Add coex get handler
	SInt32		getBTCOEX_MODE( OSObject * interface, struct apple80211_btc_mode_data * btcdata );
#if defined(WLBTCPROF) && defined(APPLE80211_IOC_BTCOEX_PROFILES)
	SInt32		getBTCOEX_PROFILES( OSObject * interface, struct apple80211_btc_profiles_data * btc_data );
	SInt32		getBTCOEX_CONFIG( OSObject * interface, struct apple80211_btc_config_data * btc_data );
#endif /* WLBTCPROF && APPLE80211_IOC_BTCOEX_PROFILES */

#ifdef APPLEIOCIE
	SInt32		getIE( OSObject * interface, struct apple80211_ie_data * ied );
#endif
#endif

#ifdef APPLE80211_IOC_TX_CHAIN_POWER
	SInt32		getTX_CHAIN_POWER( OSObject * interface, struct apple80211_chain_power_data_get * txcpd );
#endif
#ifdef FACTORY_MERGE
	SInt32		getFACTORY_MODE( IO80211Interface * interface, struct apple80211_factory_mode_data * fmd );
#endif
#ifdef APPLE80211_IOC_INTERRUPT_STATS
	SInt32		getINTERRUPT_STATS( OSObject * interface, struct apple80211_interrupt_stats_data *isd );
#endif
#ifdef APPLE80211_IOC_TIMER_STATS
	SInt32		getTIMER_STATS( OSObject * interface, struct apple80211_timer_stats_data *tsd );
#endif
#ifdef APPLE80211_IOC_OFFLOAD_STATS
	SInt32		getOFFLOAD_STATS( OSObject * interface, struct apple80211_offload_stats_data *osd );
	SInt32		setOFFLOAD_STATS_RESET( OSObject * interface );
#endif
#ifdef APPLE80211_IOC_OFFLOAD_ARP
	SInt32		getOFFLOAD_ARP( OSObject * interface, struct apple80211_offload_arp_data * oad );
	SInt32		setOFFLOAD_ARP( OSObject * interface, struct apple80211_offload_arp_data * oad );
#endif
#ifdef APPLE80211_IOC_OFFLOAD_NDP
	SInt32		getOFFLOAD_NDP( OSObject * interface, struct apple80211_offload_ndp_data * ond );
	SInt32		setOFFLOAD_NDP( OSObject * interface, struct apple80211_offload_ndp_data * ond );
#endif
#ifdef APPLE80211_IOC_OFFLOAD_SCAN
	SInt32		getOFFLOAD_SCAN( OSObject * interface, struct apple80211_offload_scan_data * osd );
	SInt32		setOFFLOAD_SCAN( OSObject * interface, struct apple80211_offload_scan_data * osd );
#endif
#ifdef APPLE80211_IOC_TX_NSS
	SInt32		getTX_NSS( OSObject * interface, struct apple80211_tx_nss_data * tnd );
	SInt32		setTX_NSS( OSObject * interface, struct apple80211_tx_nss_data * tnd );
#endif
#ifdef APPLE80211_IOC_ROAM_PROFILE
	SInt32		getROAM_PROFILE( OSObject * interface, struct apple80211_roam_profile_band_data * rpbd );
	SInt32		setROAM_PROFILE( OSObject * interface, struct apple80211_roam_profile_band_data * rpbd );
#endif
	SInt32		setCHANNEL( OSObject * interface, struct apple80211_channel_data * cd );
	SInt32		setPROTMODE( OSObject * interface, struct apple80211_protmode_data * pd );
	SInt32		setTXPOWER( OSObject * interface, struct apple80211_txpower_data * td );
	SInt32		setSCAN_REQ( OSObject * interface, struct apple80211_scan_data * sd );
	SInt32		setSCAN_REQ_MULTIPLE( OSObject * interface, struct apple80211_scan_multiple_data * sd );
	SInt32		setASSOCIATE( OSObject * interface, struct apple80211_assoc_data * ad );
	SInt32		setPOWER( OSObject * interface, struct apple80211_power_data * pd );
	SInt32		setIBSS_MODE( OSObject * interface, struct apple80211_network_data * nd );
	SInt32		setHOST_AP_MODE( OSObject * interface, struct apple80211_network_data * nd );
	SInt32		setCIPHER_KEY( OSObject * interface, struct apple80211_key * key );
	SInt32		setAUTH_TYPE( OSObject * interface, struct apple80211_authtype_data * ad );
	SInt32		setDISASSOCIATE( OSObject * interface );
	SInt32		setSSID( OSObject * interface, struct apple80211_ssid_data * sd );
	SInt32		setAP_MODE( OSObject * interface, struct apple80211_apmode_data * ad );
	SInt32		setLOCALE( OSObject * interface, struct apple80211_locale_data * ld );
	SInt32		setINT_MIT( OSObject * interface, struct apple80211_intmit_data * imd );
	SInt32		setBSSID(OSObject * interface, struct apple80211_bssid_data *bd);
	SInt32		setDEAUTH( OSObject * interface, struct apple80211_deauth_data * dd );
	SInt32		setCOUNTERMEASURES( OSObject * interface, struct apple80211_countermeasures_data * cd );
	SInt32		setFRAG_THRESHOLD( OSObject * interface, struct apple80211_frag_threshold_data * td );
	SInt32		setRATE_SET(OSObject * interface, struct apple80211_rate_set_data *rd);
	SInt32		setSHORT_SLOT( OSObject * interface, struct apple80211_short_slot_data * sd );
	SInt32		setRATE( OSObject * interface, struct apple80211_rate_data * rd );
	SInt32		setMULTICAST_RATE( OSObject * interface, struct apple80211_rate_data * rd );
	SInt32		setSHORT_RETRY_LIMIT( OSObject * interface, struct apple80211_retry_limit_data * rld );
	SInt32		setLONG_RETRY_LIMIT( OSObject * interface, struct apple80211_retry_limit_data * rld );
	SInt32		setTX_ANTENNA( OSObject * interface, struct apple80211_antenna_data * ad );
	SInt32		setANTENNA_DIVERSITY( OSObject * interface, struct apple80211_antenna_data * ad );
	SInt32		setDTIM_INT( OSObject * interface, apple80211_dtim_int_data * dd );
	SInt32		setPOWERSAVE(OSObject * interface, struct apple80211_powersave_data *pd);
	SInt32		setRSN_IE( OSObject * interface, struct apple80211_rsn_ie_data * rid );
	SInt32		setPHY_MODE(OSObject *interface, struct apple80211_phymode_data *pd);
	SInt32		setGUARD_INTERVAL( OSObject * interface, struct apple80211_guard_interval_data * gid );
	SInt32		setMCS(	OSObject * interface, struct apple80211_mcs_data * md );
#ifdef APPLE80211_IOC_MCS_VHT
	SInt32		setMCS_VHT( OSObject * interface, struct apple80211_mcs_vht_data * mvd );
#endif

	SInt32		setRIFS( OSObject * interface, struct apple80211_rifs_data * rd )
			{ return EOPNOTSUPP; }

	SInt32		setLDPC( OSObject * interface, struct apple80211_ldpc_data * ld );
	SInt32		setMSDU( OSObject * interface, struct apple80211_msdu_data * md );
	SInt32		setMPDU( OSObject * interface, struct apple80211_mpdu_data * md );
	SInt32		setBLOCK_ACK( OSObject * interface, struct apple80211_block_ack_data * bad );

	SInt32		setPLS( OSObject * interface, struct apple80211_pls_data * pd )
			{ return EOPNOTSUPP; }

	SInt32		setPSMP( OSObject * interface, struct apple80211_psmp_data * pd )
			{ return EOPNOTSUPP; }

	SInt32		setPHY_SUB_MODE( OSObject * interface, struct apple80211_physubmode_data * pd );
#ifdef SUPPORT_FEATURE_WOW
	SInt32		setWOW_PARAMETERS( OSObject * interface, struct apple80211_wow_parameter_data * wpd );
#endif
#ifdef APPLE80211_IOC_CDD_MODE
	SInt32  	setCDD_MODE( IO80211Interface * interface, struct apple80211_cdd_mode_data * cmd );
#endif
#ifdef APPLE80211_IOC_THERMAL_THROTTLING
	SInt32  	setTHERMAL_THROTTLING( IO80211Interface * interface, struct apple80211_thermal_throttling * tt );
#endif
#ifdef APPLE80211_IOC_REASSOCIATE
	SInt32  	setREASSOCIATE( IO80211Interface * interface );
#endif
#if VERSION_MAJOR > 9
	SInt32		setROAM_THRESH( OSObject * interface, struct apple80211_roam_threshold_data * rtd );

	// Apple change: coex set handler's name should be consistent with other ioctl handlers
	SInt32		setBTCOEX_MODE( OSObject * interface, struct apple80211_btc_mode_data * btcdata );
#if defined(WLBTCPROF) && defined(APPLE80211_IOC_BTCOEX_PROFILES)
	SInt32		setBTCOEX_PROFILES( OSObject * interface, struct apple80211_btc_profiles_data * btc_data );
	SInt32		setBTCOEX_CONFIG( OSObject * interface, struct apple80211_btc_config_data * btc_data );
#endif /* WLBTCPROF && APPLE80211_IOC_BTCOEX_PROFILES */

	// Apple change: hostapd support
	SInt32		setSTA_DEAUTH( OSObject * interface, struct apple80211_sta_deauth_data * sdd );
	SInt32		setSTA_AUTHORIZE( OSObject * interface, struct apple80211_sta_authorize_data * sad );
	SInt32		setRSN_CONF( OSObject * interface, struct apple80211_rsn_conf_data * rcd );

	SInt32		setLINK_QUAL_EVENT_PARAMS( OSObject * interface, struct apple80211_link_qual_event_data * lqd );
	SInt32		setVENDOR_DBG_FLAGS( OSObject * interface,
	                                     struct apple80211_vendor_dbg_data * vdd );
#ifdef APPLEIOCIE
	SInt32		setIE( OSObject * interface, struct apple80211_ie_data * ied );
#endif

#if VERSION_MAJOR > 10
#ifdef APPLE80211_IOC_DESENSE
	SInt32		setDESENSE( OSObject * interface, struct apple80211_desense_data * dd );
	SInt32		getDESENSE( OSObject * interface, struct apple80211_desense_data * dd );
#endif /* APPLE80211_IOC_DESENSE */

#ifdef APPLE80211_IOC_DESENSE_LEVEL
	SInt32		getDESENSE_LEVEL( OSObject * interface, struct apple80211_desense_level_data * btcdata );
	SInt32		setDESENSE_LEVEL( OSObject * interface, struct apple80211_desense_level_data * btcdata );
#endif

#ifdef APPLE80211_IOC_CHAIN_ACK
	SInt32		setCHAIN_ACK( OSObject * interface, struct apple80211_chain_ack * ca );
	SInt32		getCHAIN_ACK( OSObject * interface, struct apple80211_chain_ack * ca );
#endif /* APPLE80211_IOC_CHAIN_ACK */

#ifdef APPLE80211_IOC_OFFLOAD_SCANNING
	SInt32		setOFFLOAD_SCANNING( OSObject * interface, struct apple80211_offload_scanning_data * osd );
	SInt32		getOFFLOAD_SCANNING( OSObject * interface, struct apple80211_offload_scanning_data * osd );
#endif /* APPLE80211_IOC_OFFLOAD_SCANNING */

#ifdef APPLE80211_IOC_OFFLOAD_KEEPALIVE_L2
	SInt32		setOFFLOAD_KEEPALIVE_L2( OSObject * interface, struct apple80211_offload_l2_keepalive_data * ccd );
	SInt32		getOFFLOAD_KEEPALIVE_L2( OSObject * interface, struct apple80211_offload_l2_keepalive_data * ccd );
#endif /* APPLE80211_IOC_OFFLOAD_KEEPALIVE_L2 */

#ifdef APPLE80211_IOC_OFFLOAD_ARP_NDP
	SInt32		setOFFLOAD_ARP_NDP( OSObject * interface, struct apple80211_offload_arp_ndp * oand );
	SInt32		getOFFLOAD_ARP_NDP( OSObject * interface, struct apple80211_offload_arp_ndp * oand );
#endif /* APPLE80211_IOC_OFFLOAD_ARP_NDP */

#ifdef APPLE80211_IOC_OFFLOAD_BEACONS
	SInt32		setOFFLOAD_BEACONS( OSObject * interface, struct apple80211_offload_beacons * bp );
	SInt32		getOFFLOAD_BEACONS( OSObject * interface, struct apple80211_offload_beacons * bp );
#endif /* APPLE80211_IOC_OFFLOAD_BEACONS */

#ifdef OPPORTUNISTIC_ROAM
	SInt32		setROAM( OSObject * interface, struct apple80211_sta_roam_data * rd );
#endif /* OPPORTUNISTIC_ROAM */
	SInt32		setSCANCACHE_CLEAR( IO80211Interface * interface );
#endif /* VERSION_MAJOR > 10 */

#if defined(IO80211P2P) || defined(IO80211MBSS)
	SInt32		setVIRTUAL_IF_CREATE( OSObject * interface, struct apple80211_virt_if_create_data * icd );
	SInt32		setVIRTUAL_IF_DELETE( OSObject * interface, struct apple80211_virt_if_delete_data * idd );
#endif

#ifdef IO80211P2P
	SInt32		setP2P_LISTEN( OSObject * interface, struct apple80211_p2p_listen_data * pld );
	SInt32		setP2P_SCAN( OSObject * interface, struct apple80211_scan_data * sd );
	SInt32		setP2P_GO_CONF( OSObject * interface, struct apple80211_p2p_go_conf_data * gcd );
#if VERSION_MAJOR > 10
	SInt32 		setP2P_OPP_PS( OSObject * interface, struct apple80211_opp_ps_data * opd );
	SInt32		getP2P_OPP_PS( OSObject * interface, struct apple80211_opp_ps_data * opd );
	SInt32 		setP2P_CT_WINDOW( OSObject * interface,	struct apple80211_ct_window_data * cwd );
	SInt32 		getP2P_CT_WINDOW( OSObject * interface, struct apple80211_ct_window_data * cwd );
#endif // VERSION_MAJOR > 10
#endif // IO80211P2P
#ifdef APPLE80211_IOC_TX_CHAIN_POWER
	SInt32		setTX_CHAIN_POWER( OSObject * interface, struct apple80211_chain_power_data_set * txcpd );
#endif
#ifdef FACTORY_MERGE
	SInt32		setFACTORY_MODE( IO80211Interface * interface, struct apple80211_factory_mode_data * fmd );
#endif

#ifdef APPLE80211_IOC_ACL_POLICY
	SInt32		setACL_POLICY( OSObject * interface, struct apple80211_acl_policy_data * pd );
	SInt32		getACL_POLICY( OSObject * interface, struct apple80211_acl_policy_data * pd );
#endif
#ifdef APPLE80211_IOC_ACL_ADD
	SInt32		setACL_ADD( OSObject * interface, struct apple80211_acl_entry_data * ad );
#endif
#ifdef APPLE80211_IOC_ACL_REMOVE
	SInt32		setACL_REMOVE( OSObject * interface, struct apple80211_acl_entry_data * ad );
#endif
#ifdef APPLE80211_IOC_ACL_FLUSH
	SInt32		setACL_FLUSH( OSObject * interface );
#endif
#ifdef APPLE80211_IOC_ACL_LIST
	SInt32		getACL_LIST( OSObject * interface, struct apple80211_acl_entry_data * ad );
#endif

	void 		macosWatchdog();

#endif // VERSION_MAJOR > 9

#ifdef WL_BSSLISTSORT
	bool wlSortBsslist(wlc_bss_info_t **bip);
#endif

#if VERSION_MAJOR > 8
	SInt32 requestCheck( UInt32 req, int type );
	SInt32 apple80211Request( UInt32 req, int type, IO80211Interface * intf, void * data );
#endif /* VERSION_MAJOR > 8 */

	SInt32		performCountryCodeOperation( IO80211Interface * interface, IO80211CountryCodeOp op );

#if VERSION_MAJOR > 8
	void		postMessage( int msg, void * data = NULL, size_t dataLen  = 0);
#else
	void		postMessage( int msg );
#endif

	SInt32		stopDMA();

	SInt32		enableFeature( IO80211FeatureCode feature, void * refcon ) ;

	// Apple Change: Support virtually, but not physically contiguous packets
	virtual UInt32 getFeatures() const;

#ifdef IO80211P2P
	SInt32 apple80211VirtualRequest( UInt32 req, int type, IO80211VirtualInterface * vif, void * data );
#endif

#ifdef MACOS_VIRTIF
	static IOReturn handleVirtIOC(void *target, void * param0, void *param1, void *param2, void *param3);
	static IOReturn outputPacketGated( OSObject *owner, void * arg0, void * arg1, void * arg2, void * arg3 );
#endif /* MACOS_VIRTIF */

#ifdef AAPLPWROFF

	#define SYNC_STATE_WILL_CHANGE 1
	#define SYNC_STATE_SET_POWER   2
	#define SYNC_STATE_DID_CHANGE  3

	bool leaveModulePoweredForOffloads();
	IOReturn syncPowerState( unsigned long powerStateOrdinal, int callback );
	static IOReturn syncPowerStateGated( OSObject * owner, void * arg0, void * arg1, void * arg2, void * arg3 );
	void powerOffThread();
	static IOReturn powerOffThreadGated( OSObject * owner, void * arg0, void * arg1, void * arg2, void * arg3 );
	void checkInterfacePowerState();
	static IOReturn powerChangeCallbackGated( OSObject * owner, void * arg0, void * arg1, void * arg2, void * arg3 );
	void powerChange( UInt32 messageType, void *messageArgument );
	static IOReturn powerChangeCallback( void * target, void * refCon, UInt32 messageType,
										 IOService * service, void * messageArgument, vm_size_t argSize );
	bool platformSupportsTruePowerOff();
	void setACPIProperty( const char * propName, bool enable );
	// Apple change: Support for WoW on systems with WoW hardware, but no EFI support
	bool acpiPropertySupported( const char * propName );
	void callPlatformFunction( const char * funcName, bool val );
#endif /* AAPLPWROFF */

	void checkOverrideASPM();
	void overrideASPMRoot(bool L0s, bool L1s);
	void overrideASPMDevice(bool L0s, bool L1s);
	void checkWakeIndication();
	bool checkCardPresent( bool do_print, uint32 *status );
	void wl_nocard_war(void);
	void freeAssocStaIEList(struct ether_addr *addr, bool deleteAll);
	int  get_regrev( int idx );

#if VERSION_MAJOR > 10
	bool checkMaskIsDeviceValid(IOPCIDevice *device, IOPCIBridge *parent, uint32 checkmask);
	IOReturn doDeviceRecovery(IOPCIDevice *device, IOPCIBridge *parent);
	IOReturn tryDeviceRecovery(IOPCIDevice *device, IOPCIBridge *parent, int checkrecovery,
		int retries, uint32 checkmask);
	IOPCIDevice *getPCIDevice() { return pciNub; };
	void SecondaryBusReset(IOPCIDevice * device);
#endif /* VERSION_MAJOR > 10 */

#if (defined(WL_EVENTQ) && defined(P2PO) && !defined(P2PO_DISABLED))
	wl_eventq_info_t	*wlevtq;	/* pointer to wl_eventq info */
#endif
#if (defined(P2PO) && !defined(P2PO_DISABLED)) || (defined(ANQPO) && \
	!defined(ANQPO_DISABLED))
	wl_gas_info_t	*gas;		/* pointer to gas info */
#if defined(P2PO) && !defined(P2PO_DISABLED)
	wl_p2po_info_t	*p2po;		/* pointer to p2p offload info */
	wl_disc_info_t	*disc;		/* pointer to disc info */
#endif /* P2PO && !P2PO_DISABLED */
#if defined(ANQPO) && !defined(ANQPO_DISABLED)
	wl_anqpo_info_t *anqpo;	/* pointer to anqp offload info */
#endif /* ANQPO && !ANQPO_DISABLED */
#endif /* (P2PO && !P2PO_DISABLED) || (ANQPO && !ANQPO_DISABLED) */

protected:
	SInt32 wlIocCommon(struct apple80211req *req, OSObject *iface, bool set, bool userioc = true );

	OSString *	getVersionString();

	IOReturn	setCipherKey( UInt8 *keyData, int keylen, int keyIndex, bool txKey );
	void		SetCryptoKey( UInt8 *keyData, int keylen, int keyIdx, UInt8 *sc, bool txKey, struct ether_addr * ea );

		// my own protected methods
	void		stop_local();
	bool		wlcStart();
	void		wlcStop();
	void		deviceWowlClr();

	bool		createInterruptEvent();
	static void	interruptEntryPoint( OSObject*, IOInterruptEventSource *src, int count );

	static void	inputAction(IONetworkController *target, mbuf_t m);

	bool		PubWirelessMedium();
	void		SelectAntennas();
	void		BTCoexistence();

private:

	void	allocAssocStaIEList(struct ether_addr *addr, void *data, UInt32 datalen);

	static const chip_power_levels_t* chip_power_lookup(uint chipnum, uint chiprev);
	void	wl_maclist_update();
	void	mscan_maclist_fill(struct ether_addr *bssid_list, uint8 num_bssid);
	void	mscan_maclist_clear();
	bool	mscan_maclist_filter(struct ether_addr *bssid);
	uint	wl_calc_wsec(OSObject * interface);
	uint	wl_calc_wsec_from_rsn(bcm_tlv_t *rsn_ie);
	int	wl_parse_wpa_ciphers(bcm_tlv_t *rsn_ie,
			wpa_suite_t **mc_suite, uint *uc_count, wpa_suite_t **uc_suite);
	int	bandSupport(int *bandtypes) const;
	int	bandsOperational(uint32 *bands);
	int	rateOverride(uint32 nrate, bool multicast);
	void	rateOverrideReset();
	void	phySubmodeDisassocReset();
	bool	setLinkState( IO80211LinkState linkState , UInt32 reason = 1);
	SInt32	scanreq_common(uint32 bss_type, uint32 scan_type, uint32 dwell_time, uint32 rest_time,
	                       uint32 phy_mode, uint32 num_channels, struct apple80211_channel *channels,
	                       int *bssType, int *scanType, int *activeTime, int *passiveTime, int *homeTime,
	                       chanspec_t *chanspec_list, int *chanspec_count);
#if defined(WLBTCPROF) && defined(APPLE80211_IOC_BTCOEX_PROFILES)
	SInt32 setup_btc_mode( wlc_btc_profile_t *btc_select_profile , uint32 *btc_mode);
	void   setup_btc_chain_ack( wlc_btc_profile_t *btc_select_profile );
	SInt32 setup_btc_tx_power( wlc_btc_profile_t *btc_select_profile );
	SInt32 setup_btc_select_profile( wlc_btc_profile_t *btc_select_profile );
	SInt32 get_iovar_select_profile( wlc_btc_select_profile_t *btc_select_profile, int len );
	SInt32 set_iovar_select_profile( wlc_btc_select_profile_t *btc_select_profile, int len );
#endif /* WLBTCPROF && APPLE80211_IOC_BTCOEX_PROFILES */

#if VERSION_MAJOR > 9
	void linkQualHistoryReset();
	void linkQualHistoryUpd(UInt32 maxRate);

	// Apple change: hostapd support
	SInt32 setSTA_WPA_Key( struct apple80211_key * key );
#ifdef APPLEIOCIE
	bool	matchIESignature(uint8 *signature, u_int32_t signature_len, vndr_ie_t *ie);
	void 		printApple80211IE(struct apple80211_ie_data * ied, bool get, bool req);
#endif
#endif

#ifdef SUPPORT_FEATURE_WOW
	// Apple change: <rdar://problem/6501147> Support dynamic interface wake flags
	virtual IOReturn getPacketFilters( const OSSymbol * group,
	                                   UInt32 *filters ) const;

	bool wowCapableAP() const;
#endif

	static void apple80211_scan_done( void *drv, int status, wlc_bsscfg_t *cfg );
	void	scanFreeResults();
	void	scanComplete(wlc_bss_list_t *scan_results);
	bool	scanPhyModeMatch(uint32 phy_mode, wlc_bss_info_t * bi);
	void	ratesetModulations(wlc_rateset_t *rs, bool onlyBasic,
	                           bool *cck, bool *ofdm, bool *other);
	void	chanspec2applechannel(chanspec_t chanspec, struct apple80211_channel *channel);
	void	scanConvertResult(wlc_bss_info_t * bi, struct apple80211_scan_result *asr);
	int	setAP(bool ap);
	int	setIBSS(bool ibss);

	wlc_bsscfg_t *wl_find_bsscfg(struct wlc_if * wlcif = NULL) const;

	// Apple change
	bool wowCapablePlatform();

#define WOWL_ARP_OFFLOADID		1
#define WOWL_NS_OFFLOADID		2
#define WOWL_GTK_OFFLOADID		3

#define NDP_ADDR_COUNT          4

#define SC17_MAX_RETRY 3
	// P2P Virtual Interface considerations
	void	checkStatusCode17();
	struct apple80211_ssid_data sc17_data;
	uint8	sc17_retry_cnt;

	AirPort_Brcm43xxInterface	*myNetworkIF;

#ifdef IO80211P2P
	SLIST_HEAD(virt_intf_head, virt_intf_entry) virt_intf_list;
	UInt32 _attachingIfRole;
#endif /* IO80211P2P */
#ifdef MACOS_VIRTIF
	IOEthernetInterface 	*mysecondIF;
	wl_if_t 		apif;
#endif /* MACOS_VIRTIF */

	IOPCIDevice		*pciNub;		// my class's provider
	IOMemoryMap		*ioMapBAR0;
	IOMemoryMap		*ioMapBAR1;
	IONetworkStats		*netStats;
	IOEthernetStats		*etherStats;
	IOInterruptEventSource	*interruptSrc;
	Brcm43xx_InputQueue	*inputQueue;
	wlc_rev_info_t revinfo;

	bool		multicastEnabled;
	uint		nMcast;
	struct		ether_addr  mcastList[MAXMULTILIST];
	bool		forceInterferenceMitigation;	// true when device should always have intmit on

	bool		bpfRegistered;

	char		ifname[IFNAMSIZ];
	uint8		*iobuf_small;
	int		scan_done;
	int		scan_event_external;
	uint32		scan_phy_mode;
	SLIST_HEAD(scan_list_head, scan_result) scan_result_list;
	scan_result_t	*current_scan_result;
#if VERSION_MAJOR > 8
	volatile SInt32 		entering_sleep;
#else
	SInt32 		entering_sleep;
#endif
	int		phytype;
	uint		num_radios;
	int		rate_override;
	int		mrate_override;
	uint32		phy_submode;
	uint32		phy_submode_flags;
	bool		enable11n;
#ifdef APPLE80211_IOC_TX_CHAIN_POWER
	int32_t		*tx_chain_offset;
#endif
	bool	_down;
	bool	_up;

	SLIST_HEAD(sta_ie_list_head, sta_ie_list) assocStaIEList;

#ifdef AAPLPWROFF
	unsigned long _powerState;
	// <rdar://problem/10765373> Track last user requested power state incase
	// a power change request comes in before the previous user requested power
	// change request has completed
	unsigned long _lastUserRequestedPowerState;
	bool	_powerSleep;
	volatile SInt32 _powerOffInProgress;
	thread_call_t _pwrOffThreadCall;
	bool _powerOffThreadRequest;
	IONotifier * _powerChangeNotifier;
	bool _systemSleeping;
	void enableTruePowerOffIfSupported();
	void platformWoWEnable( bool enable );
#if VERSION_MAJOR > 10
	static IOReturn configHandler( void * ref, IOMessage message, IOPCIDevice * device, uint32_t state );

	// Cached PCI vendor/device ID
	UInt16 _pciVendorId;
	UInt16 _pciDeviceId;

	volatile UInt32 _pciSavedConfig[256];
	int _enabledDeviceRecovery;

	IOReturn restoreDeviceState( IOPCIDevice *device );
	IOReturn saveDeviceState( IOPCIDevice *device, int override = 0 );
	IOReturn configValidChk( IOPCIDevice *device );
#endif
#ifdef BONJOUR
//	IOCommandGate               *fGate;                 // to run usercommands on normal workloop
	mDNSHandoff                *fHandoff;              // handoff record for mDNS proxy
	UInt8                       *fOffloadData;          // buffer ready to hand to firmware
	UInt32                      fOffloadLen;            // number of bytes to offload
	UInt32                      fOffloadSize;           // size of the buffer
#endif  //BONJOUR

public:
	volatile SInt32 _powerOffThreadRunning;
#endif /* AAPLPWROFF */

	// Apple change: WPA2-PSK IBSS support
	bool ibssRunning;
	char ibssSSID[APPLE80211_MAX_SSID_LEN];
	UInt32 ibssSSIDLen;

	// Apple change: post wake indication
	// if awoken by PCI device
	bool systemWoke;

#ifdef IO80211P2P
	UInt32 actFrmID;
#ifdef FAST_ACTFRM_TX
	bool actFrmPending;
	void restartActFrmQueue();
#endif // FAST_ACTFRM_TX
#endif // IO80211P2P
#ifdef WLOFFLD
	uchar* bar1_addr;
	uint32 bar1_size;
#endif

	// Apple change: Cache BT Coex setting
	// so it can be applied when power is
	// turned on.
	int _cachedBTCoexMode;

#if VERSION_MAJOR > 10
	// Apple change: Support for APPLE80211_M_ACT_FRM_TX_COMPLETE
	struct apple80211_act_frm_tx_complete_event_data _actFrmCompleteData;
#endif // VERSION_MAJOR > 10

#ifdef MACOS_AQM
	static IOReturn processAllServiceClassQueues( OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3 );

	IOReturn processServiceClassQueue( IONetworkInterface *interface,
											uint32_t *pktcount, int reapcount, uint32_t *pktsent,
											int ac, int hmark, int sc );

	IOReturn outputStart( IONetworkInterface *interface, IOOptionBits options );


	// For configurability
	int getOutputPacketSchedulingModel() { return _outputPacketSchedulingModel; }
	void setOutputPacketSchedulingModel( int type ) { _outputPacketSchedulingModel = type; }

	// Tunables
	struct
	{
		int    pullmode;
		UInt32 txringsize;
		UInt32 txsendqsize;
		UInt32 reapcount;
		UInt32 reapmin;
	} _aqmTunables;

private:
	int _outputPacketSchedulingModel;
#endif /* MACOS_AQM */
private:
	uint32 pci_int_mask;
};
#endif /* _wl_macosx_h */
