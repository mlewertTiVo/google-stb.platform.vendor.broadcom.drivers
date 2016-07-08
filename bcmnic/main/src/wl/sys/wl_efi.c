/*
 * EFI-specific portion of
 * Broadcom 802.11abgn Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wl_efi.c 544707 2015-03-28 02:10:39Z $
 */

#include <wlc_cfg.h>
#include <osl.h>
#include <epivers.h>
#include <bcmdevs.h>
#include <pcicfg.h>
#include <wlioctl.h>
#include <wlioctl_utils.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <bcmstdlib.h>
#include <bcmwpa.h>
#include <siutils.h>

#include <wlc_pub.h>
#include <wl_dbg.h>
#include <wl_export.h>
#include <wl_efi.h>

#include <sbhndpio.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_bsscfg.h>
#include <wlc_channel.h>
#include <wlc_pio.h>
#include <wlc.h>
#include <wlc_scan.h>
#include <wlc_keymgmt.h>

extern EFI_GUID gBcmwlIoctlGuid;
extern EFI_GUID gApple80211ProtocolGuid;
extern EFI_GUID gApple80211IoctlScanGuid;
extern EFI_GUID gApple80211IoctlScanResultGuid;
extern EFI_GUID gApple80211IoctlJoinGuid;
extern EFI_GUID gApple80211IoctlStateGuid;
extern EFI_GUID gApple80211IoctlInfoGuid;
extern EFI_GUID gApple80211IoctlIBSSGuid;

/* private tunables */
#define	NTXBUF	(256)	/* # local transmit buffers */
#define	NRXBUF	(256)	/* # local receive buffers */

#define PCI_VENDOR(vid) ((vid) & 0xFFFF)
#define PCI_DEVID(vid) ((vid) >> 16)

/*  Local function prototypes. */
/* -- Driver Binding Protocol Prototypes -- */
STATIC EFI_STATUS
EFIAPI
BCMWL_DriverSupported(IN EFI_DRIVER_BINDING_PROTOCOL *This,
                      IN EFI_HANDLE Controller,
                      IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath);

STATIC EFI_STATUS
EFIAPI
BCMWL_DriverStart(IN EFI_DRIVER_BINDING_PROTOCOL *This,
                  IN EFI_HANDLE Controller,
                  IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath);

STATIC EFI_STATUS
EFIAPI
BCMWL_DriverStop(IN  EFI_DRIVER_BINDING_PROTOCOL *This,
                 IN  EFI_HANDLE Controller,
                 IN  UINTN NumberOfChildren,
                 IN  EFI_HANDLE *ChildHandleBuffer);

/* -- Component Name Protocol -- */
STATIC EFI_STATUS
EFIAPI
BCMWL_ComponentNameGetDriverName(IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
                                 IN  CHAR8 *Language,
                                 OUT CHAR16 **DriverName);

STATIC EFI_STATUS
EFIAPI
BCMWL_ComponentNameGetControllerName(IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
                                     IN  EFI_HANDLE ControllerHandle,
                                     IN  EFI_HANDLE ChildHandle OPTIONAL,
                                     IN  CHAR8 *Language,
                                     OUT CHAR16 **ControllerName);

/* APPLE_80211_PROTOCOL */
STATIC EFI_STATUS
EFIAPI
BCMWL_80211Ioctl(IN VOID *This,
                 IN EFI_GUID *SelectorGuid,
                 IN VOID *InParams,
                 IN UINTN InSize,
                 OUT VOID *OutParams,
                 IN OUT UINT32 *OutSize);

/* APPLE_LINK_STATE_PROTOCOL */
STATIC EFI_STATUS
EFIAPI
BCMWL_GetLinkState(IN VOID *This);

#ifdef APPLE_BUILD
STATIC EFI_STATUS
EFIAPI
BCMWL_GetAdaptorInformation(IN VOID *This,
                            IN EFI_GUID *InformationType,
                            OUT VOID **InformationBlock,
                            OUT UINTN *InformationBlockSize);
#endif /* APPLE_BUILD */

STATIC VOID
EFIAPI
BCMWL_ExitBootService(EFI_EVENT event, VOID *context);

/* -- Loaded Image Protocol -- */
STATIC EFI_STATUS
EFIAPI
BCMWL_ImageUnloadHandler(EFI_HANDLE ImageHandle);

EFI_COMPONENT_NAME_PROTOCOL BCMWL_ComponentName =
{
	BCMWL_ComponentNameGetDriverName,
	BCMWL_ComponentNameGetControllerName,
	"eng"
};

STATIC EFI_UNICODE_STRING_TABLE BCMWL_DriverNameTable[] =
{
	{ "eng", L"Broadcom 802.11 Wireless Controller Driver"},
	{ NULL, NULL }
};

STATIC EFI_UNICODE_STRING_TABLE BCMWL_ControllerNameTable[] =
{
	{ "eng", L"BCM XXXX Wireless Controller"},
	{ NULL, NULL }
};

STATIC EFI_DRIVER_BINDING_PROTOCOL BCMWL_DriverBinding =
{
	BCMWL_DriverSupported,
	BCMWL_DriverStart,
	BCMWL_DriverStop,
	EPI_VERSION_NUM,
	NULL,
	NULL
};

struct wl_info;
struct wl_if;

#define WLC_CONNECTED(wlc) (wlc_bss_connected((wlc)->cfg) && wlc_portopen((wlc)->cfg))

#ifdef BCMWLUNDI
#define BCMWL_UNDI_NOT_REQUIRED        0xffff
#define BCMWL_UNDI_SIZE_VARY           0xfffe
#define BCMWL_UNDI_AT_LEAST_STARTED     1
#define BCMWL_UNDI_AT_LEAST_INITIALIZED 2
#define NII_GUID gEfiNetworkInterfaceIdentifierProtocolGuid_31
typedef struct UNDI_API_ENTRY
{
	UINT16 cpbsize;
	UINT16 dbsize;
	UINT16 opflags;
	UINT16 state;
	void (*handler_fn)();
	const char *name;
} UNDI_API_ENTRY, *pUNDI_API_ENTRY;

STATIC VOID BCMWL_UNDIAPIEntry_31(UINT64 cdb);
STATIC VOID BCMWL_UNDIAPIEntry(UINT64 cdb, struct wl_info *wl);
STATIC VOID BCMWL_UNDIGetState(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIStart(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIStop(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIGetInitInfo(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIGetConfigInfo(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIInitialize(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIReset(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIShutdown(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIInterruptCtrl(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIRxFilter(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIStationAddr(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIStatistics(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIIpToMac(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDINVData(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIGetStatus(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIFillHeader(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDITxPacket(PXE_CDB *cdb_ptr, struct wl_info *wl);
STATIC VOID BCMWL_UNDIRxPacket(PXE_CDB *cdb_ptr, struct wl_info *wl);

/* Dispatch table */
UNDI_API_ENTRY undi_api_table[PXE_OPCODE_LAST_VALID+1] =
{
	/* PXE_OPCODE_GET_STATE */
	{PXE_CPBSIZE_NOT_USED, PXE_DBSIZE_NOT_USED, PXE_OPFLAGS_NOT_USED,
	 BCMWL_UNDI_NOT_REQUIRED, BCMWL_UNDIGetState, "BCMWL_UNDIGetState"},

	/* PXE_OPCODE_START */
	{BCMWL_UNDI_SIZE_VARY, PXE_DBSIZE_NOT_USED, PXE_OPFLAGS_NOT_USED,
	 BCMWL_UNDI_NOT_REQUIRED, BCMWL_UNDIStart, "BCMWL_UNDIStart"},

	/* PXE_OPCODE_STOP */
	{PXE_CPBSIZE_NOT_USED, PXE_DBSIZE_NOT_USED, PXE_OPFLAGS_NOT_USED,
	 BCMWL_UNDI_AT_LEAST_STARTED, BCMWL_UNDIStop, "BCMWL_UNDIStop"},

	/* PXE_OPCODE_GET_INIT_INFO */
	{PXE_CPBSIZE_NOT_USED, sizeof(PXE_DB_GET_INIT_INFO), PXE_OPFLAGS_NOT_USED,
	 BCMWL_UNDI_AT_LEAST_STARTED, BCMWL_UNDIGetInitInfo, "BCMWL_UNDIGetInitInfo"},

	/* PXE_OPCODE_GET_CONFIG_INFO */
	{PXE_CPBSIZE_NOT_USED, sizeof(PXE_DB_GET_CONFIG_INFO), PXE_OPFLAGS_NOT_USED,
	 BCMWL_UNDI_AT_LEAST_STARTED, BCMWL_UNDIGetConfigInfo, "BCMWL_UNDIGetConfigInfo"},

	/* PXE_OPCODE_INITIALIZE */
	{sizeof(PXE_CPB_INITIALIZE), sizeof(PXE_DB_INITIALIZE), BCMWL_UNDI_NOT_REQUIRED,
	 BCMWL_UNDI_AT_LEAST_STARTED, BCMWL_UNDIInitialize, "BCMWL_UNDIInitialize"},

	/* PXE_OPCODE_RESET */
	{PXE_CPBSIZE_NOT_USED, PXE_DBSIZE_NOT_USED, BCMWL_UNDI_NOT_REQUIRED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDIReset, "BCMWL_UNDIReset"},

	/* PXE_OPCODE_SHUTDOWN */
	{PXE_CPBSIZE_NOT_USED, PXE_DBSIZE_NOT_USED, PXE_DBSIZE_NOT_USED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDIShutdown, "BCMWL_UNDIShutdown"},

	/* PXE_OPCODE_INTERRUPT_ENABLES */
	{PXE_CPBSIZE_NOT_USED, PXE_DBSIZE_NOT_USED, BCMWL_UNDI_NOT_REQUIRED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDIInterruptCtrl, "BCMWL_UNDIInterruptCtrl"},

	/* PXE_OPCODE_RECEIVE_FILTERS */
	{BCMWL_UNDI_NOT_REQUIRED, BCMWL_UNDI_NOT_REQUIRED, BCMWL_UNDI_NOT_REQUIRED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDIRxFilter, "BCMWL_UNDIRxFilter"},

	/* PXE_OPCODE_STATION_ADDRESS */
	{BCMWL_UNDI_NOT_REQUIRED, BCMWL_UNDI_NOT_REQUIRED, BCMWL_UNDI_NOT_REQUIRED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDIStationAddr, "BCMWL_UNDIStationAddr"},

	/* PXE_OPCODE_STATISTICS  */
	{PXE_CPBSIZE_NOT_USED, BCMWL_UNDI_NOT_REQUIRED, BCMWL_UNDI_NOT_REQUIRED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDIStatistics, "BCMWL_UNDIStatistics"},

	/* PXE_OPCODE_MCAST_IP_TO_MAC */
	{sizeof(PXE_CPB_MCAST_IP_TO_MAC), sizeof(PXE_DB_MCAST_IP_TO_MAC), BCMWL_UNDI_NOT_REQUIRED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDIIpToMac, "BCMWL_UNDIIpToMac"},

	/* PXE_OPCODE_NVDATA */
	{BCMWL_UNDI_NOT_REQUIRED, BCMWL_UNDI_NOT_REQUIRED, BCMWL_UNDI_NOT_REQUIRED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDINVData, "BCMWL_UNDINVData"},

	/* PXE_OPCODE_GET_STATUS */
	{PXE_CPBSIZE_NOT_USED, BCMWL_UNDI_NOT_REQUIRED, BCMWL_UNDI_NOT_REQUIRED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDIGetStatus, "BCMWL_UNDIGetStatus"},

	/* PXE_OPCODE_FILL_HEADER */
	{BCMWL_UNDI_NOT_REQUIRED, PXE_DBSIZE_NOT_USED, BCMWL_UNDI_NOT_REQUIRED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDIFillHeader, "BCMWL_UNDIFillHeader"},

	/* PXE_OPCODE_TRANSMIT */
	{BCMWL_UNDI_NOT_REQUIRED, PXE_DBSIZE_NOT_USED, BCMWL_UNDI_NOT_REQUIRED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDITxPacket, "BCMWL_UNDITxPacket"},

	/* PXE_OPCODE_RECEIVE */
	{sizeof(PXE_CPB_RECEIVE), sizeof(PXE_DB_RECEIVE), PXE_OPFLAGS_NOT_USED,
	 BCMWL_UNDI_AT_LEAST_INITIALIZED, BCMWL_UNDIRxPacket, "BCMWL_UNDIRxPacket"}
};

STATIC EFI_STATUS wl_install_undi(struct wl_info *wl,
                                  EFI_DEVICE_PATH_PROTOCOL *UndiDevicePath,
                                  uint8 *addr);

STATIC EFI_STATUS BCMWL_CreateMacDevPath(struct wl_info *wl,
                                         EFI_DEVICE_PATH_PROTOCOL **dev_p,
                                         EFI_DEVICE_PATH_PROTOCOL *basedev_p,
                                         uint8 *addr);
#endif /* BCMWLUNDI */

/* Magic cookies for the various structure */
#define SC_MAGIC	OS_HANDLE_MAGIC + 1
#define WLINFO_MAGIC	SC_MAGIC + 1
#define WLIF_MAGIC	SC_MAGIC + 2
#define WLTIMER_MAGIC	SC_MAGIC + 3

/* Timer control information */
typedef struct wl_timer {
	uint magic;		/* Timer magic cookie */
	EFI_EVENT Evt;		/* Associated EFI Event */
	struct wl_info		*wl;		/* back pointer to main wl_info_t */
	void			(*fn)(void *);	/* dpc executes this function */
	void*			arg;		/* fn is called with this arg */
	uint			ms;		/* Time in milliseconds */
	UINT64			timer_unit;	/* Timer in 100ns unit are required by EFI API */
	bool			periodic;	/* Recurring timer if set */
	bool			set;		/* Timer is armed if set */
	struct wl_timer		*next;		/* Link to next timer object */
} wl_timer_t;

typedef struct {
	APPLE_80211_SCANRESULT	sr;
	uint8		*ie_data;	/* IE from probe in scan results */
	uint		ie_len;
} wl_80211_scanresults_t;

/* Device control information */
typedef struct wl_info {
	UINTN Signature;	/* Use CR method recommended for Device drivers */
	EFI_HANDLE WlHandle;	/* Handle to the controller */
	EFI_HANDLE ChildHandle;

	/* Protocols that are consumed */
	EFI_PCI_IO_PROTOCOL	*PciIoIntf;
	EFI_PCI_IO_PROTOCOL	*ChildPciIoIntf;

#ifdef BCMWLUNDI
	/* Only UNDI 3.1 interfaces implemented */
	EFI_DEVICE_PATH_PROTOCOL *UndiBaseDevPath;
	EFI_DEVICE_PATH_PROTOCOL *UndiDevPath;
	uint DevPathLen;

	EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL nii_31;
	void		*pxe_ptr; /* Pointer to allocated/unaligned memory */
	PXE_SW_UNDI	*pxe_31;  /* Pointer to aligned structure as required */
	UINT16		UndiState; /*  stopped, started or initialized */
	uint	nmcastcnt;

	/* Store the callback for now but the driver uses EFI API directly
	 * as wl attach and wl up need to happen before UDNIStart will happen
	 * wl up needs Map/Unmap for DMA
	 */
	struct {
		DELAY_API Delay;
		VIRT2PHY_API Virt2Phys;
		BLOCK_API Block;
		MEMIO_API Mem_IO;
		MAP_API Map;
		UNMAP_API UnMap;
		SYNC_API Sync;
		UINT64 SAP;
	} undi_callback;

#endif /* BCMWLUNDI */
	EFI_EVENT ExitBootServiceEvent;

	APPLE_80211_PROTOCOL	apple80211proto;
	enum apple80211_state		apple80211state;

	APPLE_LINK_STATE_PROTOCOL AppleLinkStateProto;
#ifdef APPLE_BUILD
	APPLE_ADAPTER_INFORMATION_PROTOCOL AdaptorInformationProto;
#endif

	wlc_info_t	*wlc;		/* pointer to wlc state */
	wlc_pub_t	*pub;		/* pointer to public wlc state */

	/* OSL Support */
	osl_t		*osh;		/* pointer to os handler */
	VOID*	DeviceMemBase; /* Virt addr for the regs */
	uint32		ntxbuf;		/* number of tx buffers */
	uint32		nrxbuf;		/* number of rx buffers */
	uint16	devid;

	/* General per-port stuff */
	volatile BOOLEAN	TxFlowControl;	/* Flowthrottle indication */
	volatile UINT16		callbacks;	/* # outstanding callback functions */
	uint bustype;
	bool piomode;

	/* SNP related */
	struct spktq txdone; /* holds packets that have been transmitted */
	struct spktq rxsendup; /* packets to be sent up to the stack */
	uint txpendcnt;

	/* Timers and Locks */
	EFI_LOCK	lock;		/* perimeter lock at NOTIFY level */
	/* Code to debug lock issues */

	wl_timer_t	*isr_timer;  /* Timer for ISR */
	wl_timer_t	*assoc_timer;
	wl_timer_t	*timers;		/* timer cleanup queue */

	int		scan_done;
	uint		scan_result_count;
	wl_80211_scanresults_t	*scan_result_list;

	bool		ssid_changed;
	bool		wifi_bounced;
} wl_info_t;

/* Convenient (and recommended) Macro to retrieve pvt. context data from produced protocols */
#define EFI_WLINFO_SNP_FROM_THIS(a) CR (a, wl_info_t, snp, BCMWL_DRIVER_SIGNATURE)
#define EFI_WLINFO_80211_FROM_THIS(a) CR (a, wl_info_t, apple80211proto, BCMWL_DRIVER_SIGNATURE)
#define EFI_WLINFO_APPLE_LINK_STATE_FROM_THIS(a) \
	CR(a, wl_info_t, AppleLinkStateProto, BCMWL_DRIVER_SIGNATURE)
#ifdef APPLE_BUILD
#define EFI_WLINFO_ADAPTOR_INFO_FROM_THIS(a) \
	CR(a, wl_info_t, AdaptorInformationProto, BCMWL_DRIVER_SIGNATURE)
#endif

#define WL_ISR_FREQ	1 /* in ms */
#define WL_TIMER_TPL	EFI_TPL_CALLBACK
#define WL_ISR_TPL	(EFI_TPL_CALLBACK + 2) /* Run at HIGHER TPL to prevent fifo overflows */

#define WL_LOCK_TPL MAX(WL_TIMER_TPL, WL_ISR_TPL)

#define WL_PKTTX_TIMEOUT 1000000	/* 1 seconds timeout for making sure that
					 * all pkts are xmitted
					 */

#define WL_LOCK(_wl, _tpl) (_tpl) = 0; EfiAcquireLock(&(_wl)->lock)
#define WL_UNLOCK(_wl, _tpl) EfiReleaseLock(&(_wl)->lock)


STATIC void wl_free(wl_info_t *wl, EFI_DRIVER_BINDING_PROTOCOL *This);
STATIC void wl_timer(IN EFI_EVENT timer_event, IN VOID *Context);

#ifdef EFI_SSID
STATIC void wl_check_assoc(void* arg);
STATIC void wl_assoc(wl_info_t *wl);
#endif

STATIC wl_info_t *gWlDevice = NULL; /* Current SINGLE wl device handle */
extern uint32 wl_msg_level;

/* Ioctl helper functions */
STATIC EFI_STATUS wl_set(wl_info_t *wl, int cmd, int arg);
STATIC EFI_STATUS wl_iovar_setint(wl_info_t *wl, const char *name, int arg);
STATIC EFI_STATUS wl_iovar_getint(wl_info_t *wl, const char *name, int *arg);
STATIC EFI_STATUS wl_iovar_op(wl_info_t *wl, const char *name, void *params, int p_len, void *arg,
                              int len, bool set, struct wlc_if *wlcif);

STATIC EFI_STATUS wl_setInfraMode(wl_info_t *wl, int mode);

STATIC void BCMWL_ScanFreeResults(wl_info_t *wl);

EFI_DRIVER_ENTRY_POINT(BCMWL_InitializeDriver)

/* Release version of the driver cannot have any console prints */
#define PRINT_BANNER(_s)

EFI_STATUS
EFIAPI
BCMWL_InitializeDriver(IN EFI_HANDLE           ImageHandle,
                       IN EFI_SYSTEM_TABLE     *SystemTable)
{
	EFI_STATUS Status;
	EFI_LOADED_IMAGE_PROTOCOL *LoadedImageInterface;

	EfiInitializeDriverLib(ImageHandle, SystemTable);

	PRINT_BANNER(("\n\rCopyright (c) 2007 Broadcom Corporation\n\r"));
	PRINT_BANNER(("Broadcom 802.11 Wireless EFI driver v%a\n\r",
	              EPI_VERSION_STR));

#ifdef BCMWLUNDI
	PRINT_BANNER(("Supports UNDI 3.1 Protocol\n\r\n\r"));
#endif


	/*
	 * Install Driver Binding Protocol on the the driver's binding
	 * handle.  And also initialize driver's library stuffs.
	 */
	if ((Status = EfiLibInstallAllDriverProtocols(ImageHandle,
	                                              SystemTable,
	                                              &BCMWL_DriverBinding,
	                                              ImageHandle,
	                                              &BCMWL_ComponentName,
	                                              NULL,
	                                              NULL)) != EFI_SUCCESS) {
		WL_ERROR(("%s: Install Driver Binding failed!\n",
		          __FUNCTION__));
		EFI_ERRMSG(Status, "Install Driver Binding Failed");
		return Status;
	}

	/* Finally, install unload handler... */
	gBS->OpenProtocol(ImageHandle,
	                  &gEfiLoadedImageProtocolGuid,
	                  (VOID **)&LoadedImageInterface,
	                  NULL,
	                  NULL,
	                  EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	LoadedImageInterface->Unload = BCMWL_ImageUnloadHandler;

	return EFI_SUCCESS;
}


STATIC EFI_STATUS
EFIAPI
BCMWL_DriverSupported(IN EFI_DRIVER_BINDING_PROTOCOL *This,
                      IN EFI_HANDLE Controller,
                      IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath)
{
	EFI_STATUS Status;
	EFI_PCI_IO_PROTOCOL *PciIo;

	UINT32 VidDid = 0;

	/*
	 * Determine if Controller handle has Device Path Protocol
	 * installed on our driver binding handle.
	 */
	Status = gBS->OpenProtocol(Controller,
	                           &gEfiDevicePathProtocolGuid,
	                           NULL,
	                           This->DriverBindingHandle,
	                           Controller,
	                           EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
	if (EFI_ERROR (Status))
		return Status;

	Status = EFI_UNSUPPORTED;

	/*
	 * Determine if Controller handle has PCI IO Protocol
	 * installed on our driver binding handle. If present,
	 * it will return PCI IO Interface handler.
	 */
	Status = gBS->OpenProtocol(Controller,
	                           &gEfiPciIoProtocolGuid,
	                           (VOID **)&PciIo,
	                           This->DriverBindingHandle,
	                           Controller,
	                           EFI_OPEN_PROTOCOL_BY_DRIVER);
	if (EFI_ERROR (Status))
		return Status;

	/*  Read vendor id/device id */
	Status = PciIo->Pci.Read(PciIo,
	                         EfiPciIoWidthUint32,
	                         0,
	                         1,
	                         &VidDid);

	if (EFI_ERROR (Status))
		goto Done;

	Status = EFI_SUCCESS;

	wl_msg_level &=	~WL_ERROR_VAL; /*  errors */

	if (!wlc_chipmatch(PCI_VENDOR(VidDid), PCI_DEVID(VidDid)))
		Status = EFI_UNSUPPORTED;
	else
		WL_TRACE(("%s: viddid:0x%x status: 0x%x\n\r",
		          __FUNCTION__, VidDid, Status));


Done:
	gBS->CloseProtocol(Controller,
	                   &gEfiPciIoProtocolGuid,
	                   This->DriverBindingHandle,
	                   Controller);

	gBS->CloseProtocol(Controller,
	                   &gEfiDevicePathProtocolGuid,
	                   This->DriverBindingHandle,
	                   Controller);

	return Status;
}

STATIC
void
wl_free(wl_info_t *wl, EFI_DRIVER_BINDING_PROTOCOL *This)
{
	EFI_TPL tpl;
	wl_timer_t *timer, *next;
	struct EFI_PKT *efi_pkt;

	if (wl == NULL)
		return;

	if (wl->wlc) {
		if (wl->pub->up) {
			WL_LOCK(wl, tpl);
			wl_down(wl);
			WL_UNLOCK(wl, tpl);
		}

		wl->callbacks = wlc_detach((void*)wl->wlc);
		wl->pub = NULL;
		wl->wlc = NULL;

		if (wl->callbacks)
			ASSERT(0);
	}

	for (timer = wl->timers; timer; timer = next) {
		next = timer->next;
		wl_del_timer(wl, timer);
		gBS->CloseEvent(timer->Evt);
		MFREE(wl->osh, timer, sizeof(wl_timer_t));
	}

	BCMWL_ScanFreeResults(wl);
	/* Free the packets that were not polled by upper-layers */
	while ((efi_pkt = pktdeq(&wl->rxsendup))) {
		MFREE(wl->osh, efi_pkt->Buffer, (uint)efi_pkt->BufferSize);
		MFREE(wl->osh, efi_pkt, sizeof(struct EFI_PKT));
	}

	/* Free the packets that were not reclaimed by upper-layers */
	while ((efi_pkt = pktdeq(&wl->txdone)))
		MFREE(wl->osh, efi_pkt, sizeof(struct EFI_PKT));

	if (wl->osh != NULL) {
		if (MALLOCED(wl->osh))
			PRINT_BANNER(("Memory leak of bytes %d\n", MALLOCED(wl->osh)));
		osl_detach(wl->osh);
		wl->osh = NULL;
	}

	gBS->FreePool(wl);
}


STATIC void
wl_dpc(wl_info_t *wl)
{
	wlc_dpc_info_t dpci = {0};

	if (wl->pub->up == FALSE)
		return;

	/* call the common second level interrupt handler */
	wlc_dpc((void *)wl->wlc, FALSE, &dpci);

	/* wlc_dpc() may bring the driver down */
	if (!wl->pub->up)
		return;
}

/* Packet exchange routines */
STATIC void
wl_isr(void *arg)
{
	wl_info_t *wl;
	bool wantdpc;

	/* No LOCKs are needed as wl_isr itself is in the context of timer for EFI
	 * Timers in EFI raise the TPL before calling the function
	 */
	wl = (wl_info_t*) arg;

	ASSERT(wl);
	ASSERT(wl->Signature == BCMWL_DRIVER_SIGNATURE);

	/* Combined wl_isr and wl_dpc */
	if (wlc_isr((void*)wl->wlc, &wantdpc) == TRUE) {
		if (wantdpc) {
			wl_dpc(wl);
			/* re-enable interrupts */
			wlc_intrson((void *)wl->wlc);
		}
	}
}

STATIC
EFI_STATUS
wl_isr_timer_create(wl_info_t *wl)
{
	wl->isr_timer = wl_init_timer(wl, wl_isr, wl, "isr");

	if (wl->isr_timer == NULL)
		return EFI_OUT_OF_RESOURCES;

	return EFI_SUCCESS;
}

static EFI_STATUS
wl_cur_etheraddr_set(wl_info_t *wl, uint8 *addr)
{
	EFI_STATUS Status;

	Status = wl_iovar_op(wl, "cur_etheraddr", NULL, 0,
	                     addr, ETHER_ADDR_LEN, IOV_SET, NULL);

	/* Update 80211proto address */
	if (Status != EFI_SUCCESS)
		bcopy(addr, &wl->apple80211proto.mode->HwAddress, ETHER_ADDR_LEN);

	return Status;
}


EFI_STATUS
EFIAPI
BCMWL_DriverStart(IN EFI_DRIVER_BINDING_PROTOCOL *This,
                  IN EFI_HANDLE Controller,
                  IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath)
{
	EFI_STATUS Status;
	EFI_DEVICE_PATH_PROTOCOL *DevicePath;
	EFI_PCI_IO_PROTOCOL *PciIoIntf;
	UINT32 Vaddr;
	osl_t *osh;
	UINT32 VidDid = 0;
	uint err;
	wl_info_t *wl;
	EFI_TPL tpl;
	uint8 addr[ETHER_ADDR_LEN];

	WL_NONE(("%s:\n\r", __FUNCTION__));

	wl = NULL;

	/*
	 * Determine if Controller handle has PCI IO Protocol
	 * installed on our driver binding handle. If present,
	 * it will return PCI IO Interface handler.
	 */
	Status = gBS->OpenProtocol(Controller,
	                           &gEfiPciIoProtocolGuid,
	                           (VOID **)&PciIoIntf,
	                           This->DriverBindingHandle,
	                           Controller,
	                           EFI_OPEN_PROTOCOL_BY_DRIVER);

	if (EFI_ERROR (Status)) {
		WL_ERROR(("%s: No PciIo Protocol on handle!\n\r",
		          __FUNCTION__));
		EFI_ERRMSG(Status, "No PciIOProtocol on handle");
		return Status;
	}

	Status = gBS->OpenProtocol(Controller,
	                           &gEfiDevicePathProtocolGuid,
	                           (VOID **)&DevicePath,
	                           This->DriverBindingHandle,
	                           Controller,
	                           EFI_OPEN_PROTOCOL_BY_DRIVER);

	if (EFI_ERROR (Status)) {
		EFI_ERRMSG(Status, "DevicePath open failed\r\n");
		goto Error;
	}

	/* First device */
	ASSERT(gWlDevice == NULL);
	wl = EfiLibAllocateZeroPool(sizeof(wl_info_t));

	if (wl == NULL) {
		WL_ERROR(("%s: Allocate POOL failed for gWlDevice\n\r", __FUNCTION__));
		goto Error;
	}

	/*  Find the memory Base address */
	wl->PciIoIntf = PciIoIntf;

	/* EFI is non-virtual memory environment. The base address of the device
	 * is the same as virtual address for device reg map.
	 * Register reads don't require the virtual address as it's offset relative to Bar index
	 * but some sb API don't like Vaddr to be 0
	 */
	PciIoIntf->Pci.Read(PciIoIntf,
	                    EfiPciIoWidthUint32,
	                    PCI_CFG_BAR0,
	                    1,
	                    &Vaddr);

	Vaddr = Vaddr & PCIBAR_MEM32_MASK;

	wl->DeviceMemBase = (VOID *)(uintptr)Vaddr;
	wl->WlHandle = Controller;

	Status = PciIoIntf->Pci.Read(PciIoIntf,
	                             EfiPciIoWidthUint32,
	                             0,
	                             1,
	                             &VidDid);

	/* Enable the PCI Master */
	/*
	 * Our device support 64-bit DMA.  Let's enable DAC support to take
	 * advantage of it if chipset supports it.  If it's supported, then
	 * buffer copy is eliminated during mapping.
	 */
	Status = PciIoIntf->Attributes(PciIoIntf,
	                               EfiPciIoAttributeOperationEnable,
	                               EFI_PCI_IO_ATTRIBUTE_MEMORY |
	                               EFI_PCI_IO_ATTRIBUTE_BUS_MASTER,
	                               NULL);

	if (EFI_ERROR(Status)) {
		UINT16 Cmd = 0x6;
		EFI_ERRMSG(Status, "Setting Attribute failed");
		PciIoIntf->Pci.Write(PciIoIntf,
		                     EfiPciIoWidthUint32,
		                     PCI_COMMAND,
		                     1,
		                     &Cmd);
	}

	/* OSL ATTACH */
	wl->ntxbuf = NTXBUF;
	wl->nrxbuf = NRXBUF;

	/* Initialize the packet queues
	 *    txdone: Holds the buffers handed to the driver by stack
	 *    rxdone: Holds the buffers to be given to the stack
	 * Make these queues virtually unbounded!
	 */
	pktqinit(&wl->txdone, -1);
	wl->txpendcnt = 0;
	pktqinit(&wl->rxsendup, -1);

	/* piomode turned off for now */
	osh = osl_attach(Controller, wl->DeviceMemBase, PciIoIntf,
	                 wl->ntxbuf,
	                 wl->nrxbuf,
	                 0,
	                 &wl->txdone);

	if (!osh) {
		WL_ERROR(("%s: osl_attach failed\n\r", __FUNCTION__));
		goto Error;
	}

	wl->bustype = PCI_BUS;
	wl->Signature = BCMWL_DRIVER_SIGNATURE;
	wl->wlc = NULL;
	wl->osh = osh;
	wl->devid = PCI_DEVID(VidDid);

	/* Lock to prevent re-entrancy */
	EfiInitializeLock(&wl->lock, WL_LOCK_TPL);

	if (!(wl->wlc = wlc_attach((void *)wl, PCI_VENDOR(VidDid), PCI_DEVID(VidDid),
	                           0, wl->piomode, wl->osh,
	                           (void *)(uintptr)Vaddr, wl->bustype, NULL, NULL, &err))) {
		WL_ERROR(("%s: %s Driver failed with code %d\n",
		          __FUNCTION__, EPI_VERSION_STR, err));
		goto Error;
	}
	wl->pub = wl->wlc->pub;

	/* Now initialize the packet buffer memory */
	Status = osl_pktinit(osh);

	if (EFI_ERROR(Status)) {
		EFI_ERRMSG(Status, "osl_pktinit failed");
		goto Error;
	}

	gWlDevice = wl; /* Save it in global pointer */

	/* Get the perm_etheraddr and set cur_etheraddr to it */
	wl_iovar_op(wl, "perm_etheraddr", NULL, 0,
	            addr, ETHER_ADDR_LEN, IOV_GET, NULL);

#ifdef BCMWLUNDI
	wl->UndiBaseDevPath = DevicePath;

	Status = wl_install_undi(wl, wl->UndiBaseDevPath, addr);

	if (EFI_ERROR (Status)) {
		EFI_ERRMSG(Status, "NII Installation failed\r\n");
		goto Error;
	}

	/* Open For Child Device */
	Status = gBS->OpenProtocol(Controller,
	                           &gEfiPciIoProtocolGuid,
	                           (VOID **)&wl->ChildPciIoIntf,
	                           This->DriverBindingHandle,
	                           wl->ChildHandle,
	                           EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER);

#endif /* BCMWLUNDI */

	/* Install Apple80211 and AppleLinkState protocol */
	wl->apple80211proto.Ioctl = BCMWL_80211Ioctl;
	wl->apple80211proto.mode = MALLOC(wl->osh, sizeof(APPLE_80211_MODE));

	bcopy(addr, &wl->apple80211proto.mode->HwAddress, ETHER_ADDR_LEN);

	if (wl->apple80211proto.mode == NULL) {
		WL_ERROR(("wl%d: apple80211proto allocation failed\n",
		          wl->pub->unit));
		goto Error;
	}

	/* Add AppleLinkState and AdaptorInformationProto functions. */
	wl->AppleLinkStateProto.GetLinkState = BCMWL_GetLinkState;
#ifdef APPLE_BUILD
	wl->AdaptorInformationProto.GetAdapterInformation = BCMWL_GetAdaptorInformation;
#endif

	/* add 80211, AdaptorInformationProto and AppleLinkState to the handle */
	Status = gBS->InstallMultipleProtocolInterfaces(&wl->ChildHandle,
	                                                &gApple80211ProtocolGuid,
	                                                &(wl->apple80211proto),
	                                                &gAppleLinkStateProtocolGuid,
	                                                &(wl->AppleLinkStateProto),
#ifdef APPLE_BUILD
	                                                &gAppleAipProtocolGuid,
	                                                &(wl->AdaptorInformationProto),
#endif
	                                                NULL);

	if (EFI_ERROR (Status)) {
		EFI_ERRMSG(Status, "Apple80211 and AppleLinkState Installation failed\r\n");
		goto Error;
	}

	/* Some default settings */
	if (wl_iovar_setint(wl, "mpc", 0) != EFI_SUCCESS)
		WL_ERROR(("wl%d: Error setting MPC variable to 0\n", wl->pub->unit));

	if (wl_iovar_setint(wl, "mimo_preamble", WLC_N_PREAMBLE_MIXEDMODE) != EFI_SUCCESS)
		WL_ERROR(("wl%d: Error setting mimo_preamble variable to 0\n", wl->pub->unit));

	if (wl_iovar_setint(wl, "brcm_ie", 0) != EFI_SUCCESS)
		WL_ERROR(("wl%d: Error Disable Broadcom Vendor IEs\n", wl->pub->unit));

	if (wl_iovar_setint(wl, "btc_mode", 0) != EFI_SUCCESS)
		WL_ERROR(("wl%d: Error Disable BT Coex Mode\n", wl->pub->unit));

	if (wl_iovar_setint(wl, "vlan_mode", 0) != EFI_SUCCESS)
		WL_ERROR(("wl%d: Error Disable VLAN Mode\n", wl->pub->unit));

	/* set default 11n Bandwidth to 20MHz only in 2.4GHz, 20/40 in 5GHz */
	if (wl_iovar_setint(wl, "mimo_bw_cap", WLC_N_BW_20IN2G_40IN5G) != EFI_SUCCESS)
		WL_ERROR(("wl%d: Error setting 11n Bandwidth to 2G-20MHz and 5G-20/40MHz\n",
		          wl->pub->unit));

	/*  Initialize a timer for checking periodic interrupts (Should this be at NOTIFY level? */
	Status = wl_isr_timer_create(wl);

	if (EFI_ERROR(Status)) {
		EFI_ERRMSG(Status, "isr_timer init failed\rn");
		Status = EFI_OUT_OF_RESOURCES;
		goto Error;
	}

	WL_LOCK(wl, tpl);

	if (wl_up(wl)) {
		WL_UNLOCK(wl, tpl);
		WL_ERROR(("wl%d:wl_up failed\n", wl->pub->unit));
		Status = EFI_OUT_OF_RESOURCES;
		goto Error;
	}

	WL_ERROR(("wl%d: wl_up status :%d\n", wl->pub->unit, wl->pub->up));

	WL_UNLOCK(wl, tpl);

#ifdef EFI_SSID
	{
		EFI_TPL tpl;

		WL_LOCK(wl, tpl);
		/*  All wlc calls need to be within perimeter! */
		wl_assoc(wl);
		if ((wl->assoc_timer = wl_init_timer(wl, wl_check_assoc, wl,
		                                     "assoc_retry_timer")) == NULL)
			WL_ERROR(("wl%d: assoc retry timer creation failed\n",
			          wl->pub->unit));
		else
			wl_add_timer(wl, wl->assoc_timer, 5000, 0); /*  5 seconds */
		WL_UNLOCK(wl, tpl);
	}
#endif /* EFI_SSID */

	Status = gBS->CreateEvent(EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES,
	                          EFI_TPL_NOTIFY,
	                          BCMWL_ExitBootService,
	                          (VOID *) wl,
	                          &wl->ExitBootServiceEvent);

	if (EFI_ERROR (Status)) {
		EFI_ERRMSG(Status, "Error Create Event Handler\r\n");
		goto Error;
	}

	return EFI_SUCCESS;

Error:
	EFI_ERRMSG(Status, "DriverStart Failed");

	if ((wl != NULL) && wl->ExitBootServiceEvent != NULL)
		gBS->CloseEvent(wl->ExitBootServiceEvent);

	wl_free(wl, This);
	gWlDevice = NULL;

	gBS->CloseProtocol(Controller,
	                   &gEfiDevicePathProtocolGuid,
	                   This->DriverBindingHandle,
	                   Controller);

	gBS->CloseProtocol(Controller,
	                   &gEfiPciIoProtocolGuid,
	                   This->DriverBindingHandle,
	                   Controller);

	return Status;
}

STATIC EFI_STATUS
EFIAPI
BCMWL_DriverStop(IN EFI_DRIVER_BINDING_PROTOCOL *This,
                 IN EFI_HANDLE Controller,
                 IN UINTN NumberOfChildren,
                 IN EFI_HANDLE *ChildHandleBuffer)
{
	wl_info_t *wl;
	EFI_STATUS		Status;

	WL_TRACE(("%s: # of Children=%d\n\r", __FUNCTION__, NumberOfChildren));

	if (gWlDevice == NULL)
		return EFI_SUCCESS;

	if (gWlDevice->WlHandle != Controller)
		return EFI_DEVICE_ERROR;

	/*
	 * Complete all outstanding transactions to Controller.
	 * Don't allow any new transaction to Controller to be started.
	 */
	if (NumberOfChildren == 0)
	{
		DEBUG((EFI_D_ERROR, "Unloading Broadcom Wireless Driver.\n"));
		/*
		 * Complete all outstanding transactions to Controller.
		 * Don't allow any new transaction to Controller to be started.
		 */
		gWlDevice->PciIoIntf->Attributes(gWlDevice->PciIoIntf,
			EfiPciIoAttributeOperationDisable,
			EFI_PCI_IO_ATTRIBUTE_MEMORY |
			EFI_PCI_IO_ATTRIBUTE_BUS_MASTER |
			EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE,
			NULL);

		/*  Free the device before closing PCI get */
		wl_free(gWlDevice, This);

		/* Close the bus driver */
		Status = gBS->CloseProtocol(Controller,
		                            &gEfiDevicePathProtocolGuid,
		                            This->DriverBindingHandle,
		                            Controller);

		Status = gBS->CloseProtocol(Controller,
		                            &gEfiPciIoProtocolGuid,
		                            This->DriverBindingHandle,
		                            Controller);

		gWlDevice = NULL;

		return EFI_SUCCESS;
	}

	if (gWlDevice->ExitBootServiceEvent != NULL)
		gBS->CloseEvent(gWlDevice->ExitBootServiceEvent);

	wl = gWlDevice;
#ifdef BCMWLUNDI
	/* Check if we have been called already */
	if (wl->UndiDevPath == NULL || wl->pxe_ptr == NULL)
	{
		DEBUG((EFI_D_ERROR, "%s: NumberOfChildren = %d\n", __FUNCTION__, NumberOfChildren));
		return EFI_SUCCESS;
	}

	/* Uninstall the protocol if already installed on the handle */
	Status = gBS->UninstallMultipleProtocolInterfaces(wl->ChildHandle,
	                                                  &NII_GUID,
	                                                  &wl->nii_31,
	                                                  &gEfiDevicePathProtocolGuid,
	                                                  wl->UndiDevPath,
	                                                  NULL);

	Status =  gBS->CloseProtocol(wl->WlHandle,
	                             &gEfiPciIoProtocolGuid,
	                             This->DriverBindingHandle,
	                             wl->ChildHandle);

	if (wl->UndiDevPath)
		MFREE(wl->osh, wl->UndiDevPath, wl->DevPathLen);
	wl->UndiDevPath = NULL;

	if (EFI_ERROR(Status))
		EFI_ERRMSG(Status, "Uninstall NII failed\r\n");

	ASSERT(wl->UndiState == PXE_STATFLAGS_GET_STATE_STOPPED);

	if (wl->pxe_ptr)
		MFREE(wl->osh, wl->pxe_ptr, sizeof(PXE_SW_UNDI) + 0x10);

	wl->pxe_ptr = NULL;
	wl->pxe_31 = NULL;

#endif /* BCMWLUNDI */

	/* Uninstall Apple80211 and AppleLinkState protocol */
	Status = gBS->UninstallMultipleProtocolInterfaces(wl->ChildHandle,
	                                                  &gApple80211ProtocolGuid,
	                                                  &(wl->apple80211proto),
	                                                  &gAppleLinkStateProtocolGuid,
	                                                  &(wl->AppleLinkStateProto),
#ifdef APPLE_BUILD
	                                                  &gAppleAipProtocolGuid,
	                                                  &(wl->AdaptorInformationProto),
#endif
	                                                  NULL);

	if (EFI_ERROR(Status))
		EFI_ERRMSG(Status, "Uninstall Apple80211 and AppleLinkState failed\r\n");

	if (wl->apple80211proto.mode != NULL) {
		MFREE(wl->osh, wl->apple80211proto.mode, sizeof(APPLE_80211_MODE));
		wl->apple80211proto.mode = NULL;
	}

	return EFI_SUCCESS;
}

EFI_STATUS BCMWL_ComponentNameGetDriverName(IN  EFI_COMPONENT_NAME_PROTOCOL *This,
                                            IN  CHAR8 *Language,
                                            OUT CHAR16 **DriverName)
{
	return EfiLibLookupUnicodeString(Language,
	                                 BCMWL_ComponentName.SupportedLanguages,
	                                 BCMWL_DriverNameTable,
	                                 DriverName);
}

EFI_STATUS BCMWL_ComponentNameGetControllerName(IN EFI_COMPONENT_NAME_PROTOCOL *This,
                                                IN EFI_HANDLE ControllerHandle,
                                                IN EFI_HANDLE ChildHandle OPTIONAL,
                                                IN  CHAR8 *Language,
                                                OUT CHAR16 **ControllerName)
{
	ASSERT(gWlDevice->WlHandle == ControllerHandle);

	SPrint(BCMWL_ControllerNameTable[0].UnicodeString,
	       (EfiStrLen(BCMWL_ControllerNameTable[0].UnicodeString)+1) * 2,
	       L"BCM%x Wireless Controller", gWlDevice->devid);

	return EfiLibLookupUnicodeString(Language,
	                                 BCMWL_ComponentName.SupportedLanguages,
	                                 BCMWL_ControllerNameTable,
	                                 ControllerName);
}

/*
 *
 * Routine Description:
 * Unload function that is registered in the LoadImage protocol.  It un-installs
 * protocols produced and deallocates pool used by the driver.  Called by the core
 * when unloading the driver.
 *   Arguments:
 *      EFI_HANDLE ImageHandle
 * Returns:
 *      Nothing
 */

STATIC EFI_STATUS BCMWL_ImageUnloadHandler(EFI_HANDLE ImageHandle)
{
	EFI_STATUS  Status;
	EFI_HANDLE *DeviceHandleBuffer;
	UINTN DeviceHandleCount;
	UINTN Index;

	WL_TRACE(("%s: %p\n\r", __FUNCTION__, ImageHandle));

	/* Get a list of all handles in the handle database */
	Status = gBS->LocateHandleBuffer(AllHandles,
	                                 NULL,
	                                 NULL,
	                                 &DeviceHandleCount,
	                                 &DeviceHandleBuffer);

	if (EFI_ERROR(Status))
		return Status;

	for (Index = 0; Index < DeviceHandleCount; Index++) {
		Status = gBS->DisconnectController(DeviceHandleBuffer[Index],
		                                   ImageHandle,
		                                   NULL);
	}

	if (DeviceHandleBuffer != NULL)
		gBS->FreePool(DeviceHandleBuffer);

	Status = gBS->UninstallMultipleProtocolInterfaces(ImageHandle,
	                                                  &gEfiDriverBindingProtocolGuid,
	                                                  &BCMWL_DriverBinding,
	                                                  &gEfiComponentNameProtocolGuid,
	                                                  &BCMWL_ComponentName,
	                                                  NULL);

	if (EFI_ERROR (Status)) {
		WL_ERROR(("%s: "
		          "Failed to uninstall protocol interfaces.\n", __FUNCTION__));
		EFI_ERRMSG(Status, "Uninstall protocol failed\n");
		return (Status);
	}

	return EFI_SUCCESS;
}

/* Per-port API */
void wl_init(struct wl_info *wl)
{
	WL_TRACE(("%s\n", __FUNCTION__));
	/* reset the interface */
	wl_reset(wl);

	/* initialize the driver and chip */
	wlc_init(wl->wlc);
}

uint wl_reset(struct wl_info *wl)
{
	wlc_reset(wl->wlc);

	WL_TRACE(("%s\n", __FUNCTION__));
	return 0;
}

bool
wl_alloc_dma_resources(wl_info_t *wl, uint addrwidth)
{
	return TRUE;
}

void wl_intrson(struct wl_info *wl)
{
	WL_TRACE(("%s\n", __FUNCTION__));
	wlc_intrson((void *)wl->wlc);
}

uint32 wl_intrsoff(struct wl_info *wl)
{
	WL_TRACE(("%s\n", __FUNCTION__));
	return wlc_intrsoff((void *)wl->wlc);
}

void wl_intrsrestore(struct wl_info *wl, uint32 macintmask)
{
	WL_TRACE(("%s\n", __FUNCTION__));
	wlc_intrsrestore((void *)wl->wlc, macintmask);
}

/* Mapping for reason code to Apple status APPLE80211_S_BAD_KEY */
static uint apple80211_bad_key_rc[] = {
	DOT11_RC_UNSPECIFIED,
	DOT11_RC_AUTH_INVAL,
	DOT11_RC_4WH_TIMEOUT,
	DOT11_RC_8021X_AUTH_FAIL,
	0
};

/* Mapping for reason code to Apple status APPLE80211_S_BAD_KEY */
static uint apple80211_bad_key_sc[] = {
	DOT11_SC_FAILURE,
	DOT11_SC_ASSOC_FAIL,
	DOT11_SC_AUTH_CHALLENGE_FAIL,
	0
};

static bool wl_code2applebadkey_map(uint reason, uint* map)
{
	int i = 0;

	while (map[i]) {
		if (reason == map[i])
			return TRUE;
		i++;
	}
	return FALSE;
}

void wl_event(struct wl_info *wl, char *ifname, wlc_event_t *evt)
{
	uint event_type = evt->event.event_type;
	uint reason = evt->event.reason;
	uint status = evt->event.status;
	wlc_info_t *wlc = wl->wlc;
	int wsec;

	switch (event_type) {
	case WLC_E_LINK:
		if ((evt->event.flags & WLC_EVENT_MSG_LINK) != WLC_EVENT_MSG_LINK) {
		        wl->apple80211state = APPLE80211_S_IDLE;
			WL_ASSOC(("%s: Link Down Event\n", __FUNCTION__));
		} else {
			WL_ASSOC(("%s: Link Up Event\n", __FUNCTION__));
		}
		break;
	case WLC_E_SET_SSID:
		if (status != WLC_E_STATUS_SUCCESS)
			wl->apple80211state = APPLE80211_S_NONETWORK;

		/*  Handle IBSS create/join */
		if (wlc->default_bss->infra == 0)
			if (status != WLC_E_STATUS_SUCCESS)
				wl_setInfraMode(wl, DOT11_BSSTYPE_ANY);
		break;
	case WLC_E_AUTH:
		if (status != WLC_E_STATUS_SUCCESS) {
			if (wl_iovar_getint(wl, "wsec", &wsec) != EFI_SUCCESS)
				wsec = 0;

			/* See if security is ON, if yes, then map the Status code to
			 * bad key
			 */
			if (wsec && wl_code2applebadkey_map(reason, apple80211_bad_key_sc))
				wl->apple80211state = APPLE80211_S_BADKEY;
			else
				wl->apple80211state = APPLE80211_S_AUTHFAIL;
		}
		break;
	case WLC_E_ROAM:
		WL_ASSOC(("%s: Roamed Event\n", __FUNCTION__));
		break;
	case WLC_E_REASSOC:
	case WLC_E_ASSOC:
		if (status != WLC_E_STATUS_SUCCESS)
			wl->apple80211state = APPLE80211_S_ASSOCFAIL;
		break;
	case WLC_E_DEAUTH_IND: {
		if (wl_iovar_getint(wl, "wsec", &wsec) != EFI_SUCCESS)
			wsec = 0;

		/* See if security is ON, if yes, then map the Status code to
		 * bad key
		 */
		if (wsec && wl_code2applebadkey_map(reason, apple80211_bad_key_rc))
			wl->apple80211state = APPLE80211_S_BADKEY;
		else if (reason != 0)
			wl->apple80211state = APPLE80211_S_OTHER;
		else
			wl->apple80211state = APPLE80211_S_IDLE;
		break;
	}
	case WLC_E_DISASSOC_IND:
		wl->apple80211state = APPLE80211_S_IDLE;
		break;
	}
}

void
wl_event_sync(wl_info_t *wl, char *ifname, wlc_event_t *e)
{
}

void
wl_event_sendup(wl_info_t *wl, const wlc_event_t *e, uint8 *data, uint32 len)
{
}

int wl_up(struct wl_info *wl)
{
	int error;

	error = 0;

	if (wl->pub->up)
		return (0);

	error = wlc_up(wl->wlc);

	if (wl->pub->up)
		wl_add_timer(wl, wl->isr_timer, WL_ISR_FREQ, 1);
	else
		wl_del_timer(wl, wl->isr_timer);

	return error;
}

void wl_down(struct wl_info *wl)
{

	/* Stop the ISR Timer */
	wl_del_timer(wl, wl->isr_timer);

	/* call the common down function */
	wl->callbacks = wlc_down(wl->wlc);

	/* Force hw to be brought up everytime wlc_up is done */
	wl->pub->hw_up = FALSE;

	WL_TRACE(("wl%d:wl_down %d \n", wl->pub->unit, wl->callbacks));
}

void wl_dump_ver(struct wl_info *wl, struct bcmstrbuf *b)
{
	bcm_bprintf(b, "wl%d: %s %s version %s\n", wl->pub->unit,
	            __DATE__, __TIME__, EPI_VERSION_STR);
}

void wl_txflowcontrol(struct wl_info *wl, struct wl_if *wlif, bool state, int prio)
{
	wl->TxFlowControl = state;
}

/* Let the driver print errorstr for ioctl requests as it can help debugging issues
 * with other tools
 */
STATIC void
wl_print_lasterror(wl_info_t *wl)
{
	char error_str[128];

	if (wlc_iovar_op((void *)wl->wlc, "bcmerrorstr", NULL, 0,
	                 error_str, sizeof(error_str), IOV_GET, NULL) == 0)
		PRINT_BANNER(("wl%d: %a\n\r", wl->pub->unit, error_str));
}

/* Timer API */
STATIC void
wl_timer(IN EFI_EVENT timer_event,
         IN VOID *Context)
{
	wl_timer_t *timer = (wl_timer_t*)Context;
	EFI_TPL tpl;

	ASSERT(timer->magic == WLTIMER_MAGIC);

	WL_LOCK(timer->wl, tpl);
	if (timer->set) {
		timer->set = FALSE;
		timer->fn(timer->arg);

		/*  Periodic is set when arming the timer */
		if (timer->periodic) {
			timer->set = TRUE;
			timer->wl->callbacks++;
		}
	}
	timer->wl->callbacks--;
	WL_UNLOCK(timer->wl, tpl);
}

STATIC wlc_bsscfg_t *wl_bsscfg_find(wl_info_t *wl)
{
	return wlc_bsscfg_find_by_wlcif(wl->wlc, NULL);
}

#ifdef EFI_SSID
STATIC void wl_assoc(wl_info_t *wl)
{
	wlc_ssid_t ssid;

	ssid.SSID_len = (uint32)strlen(EFI_SSID);
	strncpy(ssid.SSID, EFI_SSID, ssid.SSID_len);

	PRINT_BANNER(("Setting the ssid %a...\r\n", EFI_SSID));
	/*  All wlc calls need to be within perimeter! */
	wlc_ioctl(wl->wlc, WLC_SET_SSID, &ssid, sizeof(ssid), NULL);
}

STATIC void wl_check_assoc(void* arg)
{
	wl_info_t *wl = (wl_info_t *)arg;
	wlc_bsscfg_t *bsscfg;

	bsscfg = wl_bsscfg_find(wl);
	ASSERT(bsscfg != NULL);

	if (bsscfg->associated == TRUE) {
		uchar ssid[DOT11_MAX_SSID_LEN];
		wlc_bss_info_t *current_bss = wlc_get_current_bss(bsscfg);

		wlc_format_ssid(ssid, current_bss->SSID, current_bss->SSID_len);

		PRINT_BANNER(("Associated.. %a\n\r", ssid));
		return;
	}

	wl_assoc(wl);
	wl_add_timer(wl, wl->assoc_timer, 5000, 0); /*  5 seconds */
}
#endif /* EFI_SSID */

struct wl_timer *
wl_init_timer(struct wl_info *wl,
              void (*fn)(void* arg),
              void *arg,
              const char *name)
{
	wl_timer_t *timer;
	EFI_STATUS Status;

	ASSERT(wl->Signature == BCMWL_DRIVER_SIGNATURE);
	ASSERT(name);

	timer = MALLOC(wl->osh, sizeof(wl_timer_t));

	if (!timer) {
		WL_ERROR(("%s: Timer allcoation failed for %s\n\r", __FUNCTION__, name));
		return NULL;
	}

	/* Create a timer event
	 * ISR timer is created at NOTIFY level (may be even high level
	 */
	Status = gBS->CreateEvent(EFI_EVENT_TIMER | EFI_EVENT_NOTIFY_SIGNAL,
	                          (fn == wl_isr)? WL_ISR_TPL: WL_TIMER_TPL,
	                          (EFI_EVENT_NOTIFY)wl_timer,
	                          (VOID *) timer,
	                          &timer->Evt);
	if (EFI_ERROR(Status)) {
		WL_ERROR(("%s: Event creation failed for %s\n\r", __FUNCTION__, name));
		EFI_ERRMSG(Status, "Event creation failed\n");
		MFREE(wl->osh, timer, sizeof(wl_timer_t));
		return NULL;
	}

	timer->magic = WLTIMER_MAGIC;
	timer->fn = fn;
	timer->arg = arg;
	timer->wl = wl;
	timer->set = FALSE;
	timer->periodic = FALSE;


	/*  Add to the list of timers */
	timer->next = wl->timers;
	wl->timers = timer;

	WL_NONE(("%s: Success: %p, %s\n\r", __FUNCTION__, timer, name));

	return timer;
}

void wl_free_timer(struct wl_info *wl, struct wl_timer *timer)
{
	/*  Do nothing.. free the timers in detach */
}

void wl_add_timer(struct wl_info *wl, struct wl_timer *timer, uint ms, int periodic)
{
	EFI_STATUS Status;

	ASSERT(wl->Signature == BCMWL_DRIVER_SIGNATURE);
	ASSERT(timer->magic == WLTIMER_MAGIC);

	ASSERT(!timer->set);

	WL_TRACE(("%s: 0x%x %s ms:%d periodic:%d\n\r", __FUNCTION__,
	          timer, timer->name, ms, periodic));

	timer->ms = ms;
	timer->periodic = FALSE;
	timer->set = FALSE;

	timer->timer_unit = ms * 10000;

	if (timer->fn == wl_isr)
		timer->timer_unit /= 2;

	/*  Time has to be in 100ns units */
	if ((Status = gBS->SetTimer(timer->Evt, (periodic != 0)? TimerPeriodic: TimerRelative,
	                            timer->timer_unit)) != EFI_SUCCESS) {
		WL_ERROR(("wl%d: wl_add_timer failed for %s\n",
		          wl->pub->unit, timer->name));
		EFI_ERRMSG(Status, "wl_add_timer SetTimer failed\n");
		return;
	}

	timer->periodic = (periodic != 0);
	timer->set = TRUE;
	wl->callbacks++;
}

bool wl_del_timer(struct wl_info *wl, struct wl_timer *timer)
{
	EFI_STATUS Status = EFI_SUCCESS;

	ASSERT(wl->Signature == BCMWL_DRIVER_SIGNATURE);
	ASSERT(timer->magic == WLTIMER_MAGIC);

	WL_TRACE(("%s: %s set:%d \n\r", __FUNCTION__, timer->name,  timer->set));

	/* If set right now or periodic, then go ahead and cancel the timer */
	if (timer->set || timer->periodic) {
		timer->set = FALSE;
		timer->periodic = FALSE;

		/*  Use SetTimer to Cancel the event. It's not clear that
		 * cancelling stops the event from happening
		 */

		if ((Status = gBS->SetTimer(timer->Evt, TimerCancel, 0)) == EFI_SUCCESS)
			wl->callbacks--;
	}


	return (Status == EFI_SUCCESS);
}

/* data receive and interface management functions */
/*
 * The last parameter was added for the build. Caller of
 * this function should pass 1 for now.
 */
void wl_sendup(struct wl_info *wl, struct wl_if *wlif, void *p, int numpkt)
{
	struct EFI_PKT *efi_pkt = NULL;

	WL_PORT(("%s %d\n", __FUNCTION__, __LINE__));

	if (pktq_full(&wl->rxsendup)) {
		ASSERT(0);
		PKTFREE(wl->osh, p, FALSE);
		return;
	}
	efi_pkt = PKTTONATIVE(wl->osh, p);

	PKTFREE(wl->osh, p, FALSE);

	if (efi_pkt == NULL) {
		WL_ERROR(("%s %d Pkttonative failed\n", __FUNCTION__, __LINE__));
		return;
	} else
		pktenq(&wl->rxsendup, (void*)efi_pkt);
}

char *wl_ifname(struct wl_info *wl, struct wl_if *wlif)
{
	return "wl0";
}

struct wl_if *
wl_add_if(wl_info_t *wl, struct wlc_if* wlcif, uint unit, struct ether_addr *remote)
{
	WL_ERROR(("wl_add_if: virtual interfaces not supported\n"));
	return NULL;
}

void
wl_del_if(wl_info_t *wl, struct wl_if *wlif)
{
}

void wl_monitor(struct wl_info *wl, wl_rxsts_t *rxsts, void *p)
{
	WL_ERROR(("%s %d\n", __FUNCTION__, __LINE__));
}

void wl_set_monitor(struct wl_info *wl, int val)
{
	WL_ERROR(("%s %d\n", __FUNCTION__, __LINE__));
}

#ifdef BCMWLUNDI
STATIC
EFI_STATUS
BCMWL_CreateMacDevPath(wl_info_t *wl,
                       EFI_DEVICE_PATH_PROTOCOL **dev_p,
                       EFI_DEVICE_PATH_PROTOCOL *basedev_p,
                       uint8 *addr)
{
	EFI_MAC_ADDRESS MACAddress;
	UINT16 j;
	UINT8 addr_len;
	MAC_ADDR_DEVICE_PATH mac_addr_node;
	EFI_DEVICE_PATH_PROTOCOL     *end_node;
	UINT8 *d_ptr;
	UINT16 tot_path_len, base_path_len;

	addr_len = 6;
	for (j = 0; j < addr_len; j++) {
		MACAddress.Addr[j] = addr[j];
	}

	for (j = addr_len; j < sizeof(EFI_MAC_ADDRESS); j++) {
		MACAddress.Addr[j] = 0;
	}

	/*  fill the mac address node first */
	bzero((char *)&mac_addr_node, sizeof(mac_addr_node));
	memcpy((char *)&mac_addr_node.MacAddress, (char *)&MACAddress,
	       sizeof(EFI_MAC_ADDRESS));

	mac_addr_node.Header.Type = MESSAGING_DEVICE_PATH;
	mac_addr_node.Header.SubType = MSG_MAC_ADDR_DP;
	mac_addr_node.Header.Length[0] = sizeof(mac_addr_node);
	mac_addr_node.Header.Length[1] = 0;

	/*  Find the size of the base dev path. */
	end_node = basedev_p;
	while (!IsDevicePathEnd(end_node)) {
		end_node = NextDevicePathNode(end_node);
	}

	base_path_len = ((UINT16) end_node - (UINT16) basedev_p);
	/*  create space for full dev path */
	tot_path_len = base_path_len + sizeof(mac_addr_node) +
	        sizeof(EFI_DEVICE_PATH_PROTOCOL);

	d_ptr = MALLOC(wl->osh, tot_path_len);
	wl->DevPathLen = tot_path_len;

	if (d_ptr == NULL)
		return EFI_OUT_OF_RESOURCES;

	/*  copy the base path, mac addr and end_dev_path nodes */
	*dev_p = (EFI_DEVICE_PATH_PROTOCOL *)d_ptr;
	memcpy(d_ptr, (char *)basedev_p, base_path_len);
	d_ptr += base_path_len;
	memcpy(d_ptr, (char *)&mac_addr_node, sizeof(mac_addr_node));
	d_ptr += sizeof(mac_addr_node);
	memcpy(d_ptr, (char *)end_node, sizeof(EFI_DEVICE_PATH_PROTOCOL));

	return EFI_SUCCESS;
}

STATIC UINT8
wl_cksum(void *buf, unsigned len)
{
	unsigned char cksum = 0;
	char *bp;

	if ((bp = buf) != NULL)
		while (len--)
			cksum = cksum + *bp++;

	return cksum;
}

STATIC void
wl_init_pxestruct(PXE_SW_UNDI *pxe)
{
	/*  initialize the !PXE structure */
	pxe->Signature = PXE_ROMID_SIGNATURE;
	pxe->Len = sizeof(PXE_SW_UNDI);
	pxe->Fudge = 0;  /* cksum */
	pxe->IFcnt = 0;  /*  number of NICs this undi supports */
	pxe->Rev = PXE_ROMID_REV;
	pxe->MajorVer = PXE_ROMID_MAJORVER;

	pxe->MinorVer = PXE_ROMID_MINORVER_31;
	pxe->EntryPoint = (UINT64)BCMWL_UNDIAPIEntry_31;
	pxe->reserved1 = 0;

	pxe->Implementation = PXE_ROMID_IMP_SW_VIRT_ADDR |
	        PXE_ROMID_IMP_STATION_ADDR_SETTABLE |
		PXE_ROMID_IMP_PROMISCUOUS_MULTICAST_RX_SUPPORTED |
		PXE_ROMID_IMP_PROMISCUOUS_RX_SUPPORTED |
		PXE_ROMID_IMP_BROADCAST_RX_SUPPORTED |
		PXE_ROMID_IMP_FILTERED_MULTICAST_RX_SUPPORTED |
		PXE_ROMID_IMP_TX_COMPLETE_INT_SUPPORTED |
		PXE_ROMID_IMP_PACKET_RX_INT_SUPPORTED |
		PXE_ROMID_IMP_64BIT_DEVICE;

	pxe->reserved2[0] = pxe->reserved2[1] = pxe->reserved2[2]  = 0;
	pxe->BusCnt = 1;
	pxe->BusType[0] = PXE_BUSTYPE_PCI;

	pxe->Fudge -= (wl_cksum((void *)pxe, pxe->Len));
}

STATIC EFI_STATUS
wl_install_undi(wl_info_t *wl, EFI_DEVICE_PATH_PROTOCOL *UndiBaseDevPath, uint8 *addr)
{
	EFI_STATUS Status;

	wl->pxe_ptr = MALLOC(wl->osh, sizeof(PXE_SW_UNDI) + 0x10);

	if (wl->pxe_ptr == NULL)
		return EFI_OUT_OF_RESOURCES;

	/* Must be aligned to 16-byte boundary */
	wl->pxe_31 = (PXE_SW_UNDI *)(((UINTN)wl->pxe_ptr + 0x10) & (~(UINTN)0xf));
	bzero(wl->pxe_31, sizeof(PXE_SW_UNDI));

	/* Initiailze !PXE structure */
	wl_init_pxestruct(wl->pxe_31);
	wl->UndiState = PXE_STATFLAGS_GET_STATE_STOPPED;

	/* Initialize NII and install NII */
#ifdef APPLE_BUILD
	wl->nii_31.Id = (UINT64) wl->pxe_31;
#else
	wl->nii_31.ID = (UINT64) wl->pxe_31;
#endif
	wl->nii_31.IfNum = 0;

	wl->nii_31.Revision = EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION_31;
	wl->nii_31.Type = EfiNetworkInterfaceUndi;
	wl->nii_31.MajorVer = PXE_ROMID_MAJORVER;
	wl->nii_31.MinorVer = PXE_ROMID_MINORVER_31;
	wl->nii_31.ImageSize = 0;
	wl->nii_31.ImageAddr = 0;
	wl->nii_31.Ipv6Supported = FALSE;
	wl->nii_31.StringId[0] = 'U';
	wl->nii_31.StringId[1] = 'N';
	wl->nii_31.StringId[2] = 'D';
	wl->nii_31.StringId[3] = 'I';

	Status = BCMWL_CreateMacDevPath(wl,
	                                &wl->UndiDevPath,
	                                wl->UndiBaseDevPath,
	                                addr);

	if (EFI_ERROR (Status)) {
		EFI_ERRMSG(Status, "Dev path creation failed\r\n");
		return Status;
	}

	wl->ChildHandle = NULL;
	return  gBS->InstallMultipleProtocolInterfaces(&wl->ChildHandle,
	                                               &NII_GUID,
	                                               &(wl->nii_31),
	                                               &gEfiDevicePathProtocolGuid,
	                                               wl->UndiDevPath,
	                                               NULL);
}

STATIC VOID
BCMWL_UNDIAPIEntry_31(UINT64 cdb)
{
	PXE_CDB *cdb_ptr;

	if (cdb == (UINT64) 0)
		return;

	cdb_ptr = (PXE_CDB *)(UINTN)cdb;

	if (gWlDevice == NULL) {
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
		return;
	}

	BCMWL_UNDIAPIEntry(cdb, gWlDevice);
}

STATIC VOID
BCMWL_UNDIAPIEntry(UINT64 cdb, wl_info_t *wl)
{
	PXE_CDB *cdb_ptr;
	PXE_CDB tmp_cdb;
	EFI_TPL tpl;
	pUNDI_API_ENTRY api_entry;

	WL_LOCK(wl, tpl);

	cdb_ptr = (PXE_CDB *)cdb;
	tmp_cdb = *cdb_ptr;

	WL_UNLOCK(wl, tpl);

	/*  Check the OPCODE range */
	if ((tmp_cdb.OpCode > PXE_OPCODE_LAST_VALID) ||
	    (tmp_cdb.StatCode != PXE_STATCODE_INITIALIZE) ||
	    (tmp_cdb.StatFlags != PXE_STATFLAGS_INITIALIZE)) {
		WL_ERROR(("BCMWL_UNDIAPIEntry(): Bad parms!\n\r"));
		goto badcdb;
	}

	if (tmp_cdb.CPBsize == PXE_CPBSIZE_NOT_USED) {
		if (tmp_cdb.CPBaddr != PXE_CPBADDR_NOT_USED) {
			WL_ERROR(("BCMWL_UNDIAPIEntry()): Bad CPBaddr! Expected=%d\n\r",
			          PXE_CPBADDR_NOT_USED));
			goto badcdb;
		}
	} else if (tmp_cdb.CPBaddr == PXE_CPBADDR_NOT_USED) {
		WL_ERROR(("BCMWL_UNDIAPIEntry()): Bad CPBaddr! Expected !=%d\n\r",
		          PXE_CPBADDR_NOT_USED));
		goto badcdb;
	}

	if (tmp_cdb.DBsize == PXE_DBSIZE_NOT_USED) {
		if (tmp_cdb.DBaddr != PXE_DBADDR_NOT_USED) {
			WL_ERROR(("BCMWL_UNDIAPIEntry()): Bad DBaddr! Expected %d\n\r",
			          PXE_CPBADDR_NOT_USED));
			goto badcdb;
		}
	} else if (tmp_cdb.DBaddr == PXE_DBADDR_NOT_USED) {
		WL_ERROR(("BCMWL_UNDIAPIEntry()): Bad DBaddr! Expected !=%d\n\r",
		          PXE_CPBADDR_NOT_USED));
		goto badcdb;
	}

	api_entry = &undi_api_table[tmp_cdb.OpCode];

	if (api_entry->cpbsize != BCMWL_UNDI_SIZE_VARY) {
		if ((api_entry->cpbsize != BCMWL_UNDI_NOT_REQUIRED) &&
		    (api_entry->cpbsize != tmp_cdb.CPBsize)) {
			WL_ERROR(("BCMWL_UNDIAPIEntry()): %s Bad cpbsize! Expected %d got %d\n\r",
			          api_entry->name, api_entry->cpbsize, tmp_cdb.CPBsize));
			goto badcdb;
		}
	}

	if (api_entry->dbsize != BCMWL_UNDI_SIZE_VARY)  {
		if ((api_entry->dbsize != BCMWL_UNDI_NOT_REQUIRED) &&
		    (api_entry->dbsize != tmp_cdb.DBsize)) {
			WL_ERROR(("BCMWL_UNDIAPIEntry()): %s Bad cpbsize! Expected %d got %d\n\r",
			          api_entry->name, api_entry->dbsize, tmp_cdb.DBsize));
			goto badcdb;
		}
	}

	if ((api_entry->opflags != BCMWL_UNDI_NOT_REQUIRED) &&
	    (api_entry->opflags != tmp_cdb.OpFlags)) {
		WL_ERROR(("BCMWL_UNDIAPIEntry()): %s Bad opflags! Expected 0x%x got 0x%x\n\r",
		          api_entry->name, api_entry->opflags, tmp_cdb.OpFlags));
		goto badcdb;
	}

	if (api_entry->state != BCMWL_UNDI_NOT_REQUIRED) {
		if (wl->UndiState == PXE_STATFLAGS_GET_STATE_STOPPED &&
			tmp_cdb.OpCode != PXE_OPCODE_STOP &&
			tmp_cdb.OpCode != PXE_OPCODE_SHUTDOWN) {
			WL_ERROR(("BCMWL_UNDIAPIEntry(): %s(OpCode %d) Bad state!"
			          " current state is STOPPED\n\r",
			          api_entry->name, tmp_cdb.OpCode));
			tmp_cdb.StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
			tmp_cdb.StatCode = PXE_STATCODE_NOT_STARTED;
			goto badcdb;
		}

		/*  check if it should be initialized */
		if (api_entry->state == BCMWL_UNDI_AT_LEAST_INITIALIZED) {
			if (wl->UndiState != PXE_STATFLAGS_GET_STATE_INITIALIZED &&
				wl->UndiState != PXE_STATFLAGS_GET_STATE_STOPPED) {
				WL_ERROR(("BCMWL_UNDIAPIEntry()): %s Bad state!"
				          " Expected INTIALIZED got %d\n\r",
				          api_entry->name, wl->UndiState));
				tmp_cdb.StatCode = PXE_STATCODE_NOT_INITIALIZED;
				tmp_cdb.StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
				goto badcdb;
			}
		}
	}

	tmp_cdb.StatFlags = PXE_STATFLAGS_COMMAND_COMPLETE;
	tmp_cdb.StatCode = PXE_STATCODE_SUCCESS;

	api_entry->handler_fn(&tmp_cdb, wl);

	WL_LOCK(wl, tpl);
	*cdb_ptr = tmp_cdb;
	WL_UNLOCK(wl, tpl);

	return;

badcdb:
	if (!(tmp_cdb.StatFlags & PXE_STATFLAGS_STATUS_MASK)) {
		tmp_cdb.StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		tmp_cdb.StatCode = PXE_STATCODE_INVALID_CDB;
	}

	/* Copy the information back */
	WL_LOCK(wl, tpl);
	*cdb_ptr = tmp_cdb;
	WL_UNLOCK(wl, tpl);
}

STATIC VOID
BCMWL_UNDIGetState(PXE_CDB *cdb_ptr,
                   wl_info_t *wl)
{
	EFI_TPL tpl;

	WL_LOCK(wl, tpl);
	cdb_ptr->StatFlags |= wl->UndiState;
	WL_UNLOCK(wl, tpl);
}

STATIC VOID
BCMWL_UNDIStart(PXE_CDB *cdb_ptr,
                struct wl_info *wl)
{
	PXE_CPB_START_31 *cpb_ptr_31;
	EFI_TPL tpl;

	WL_LOCK(wl, tpl);
	if (wl->UndiState != PXE_STATFLAGS_GET_STATE_STOPPED) {
		WL_UNLOCK(wl, tpl);
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_ALREADY_STARTED;
		return;
	}

	/* Store callbacks but don't use them */
	cpb_ptr_31 = (PXE_CPB_START_31 *)(cdb_ptr->CPBaddr);
	wl->undi_callback.Delay = (DELAY_API)cpb_ptr_31->Delay;
	wl->undi_callback.Virt2Phys = (VIRT2PHY_API)cpb_ptr_31->Virt2Phys;
	wl->undi_callback.Block = (BLOCK_API)cpb_ptr_31->Block;
	wl->undi_callback.Mem_IO = (MEMIO_API)cpb_ptr_31->Mem_IO;
	wl->undi_callback.Map = (MAP_API)cpb_ptr_31->Map_Mem;
	wl->undi_callback.UnMap = (UNMAP_API)cpb_ptr_31->UnMap_Mem;
	wl->undi_callback.Sync = (SYNC_API)cpb_ptr_31->Sync_Mem;
	wl->undi_callback.SAP =  cpb_ptr_31->Unique_ID;

	wl->UndiState = PXE_STATFLAGS_GET_STATE_STARTED;
	WL_UNLOCK(wl, tpl);
}

STATIC VOID
BCMWL_UNDIStop(PXE_CDB *cdb_ptr,
               struct wl_info *wl)
{
	EFI_TPL tpl;

	WL_LOCK(wl, tpl);
	if (wl->UndiState == PXE_STATFLAGS_GET_STATE_INITIALIZED) {
		WL_UNLOCK(wl, tpl);
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_NOT_SHUTDOWN;
		return;
	}

	/*  Delete the call back function pointers  */
	wl->UndiState = PXE_STATFLAGS_GET_STATE_STOPPED;
	WL_UNLOCK(wl, tpl);
}

STATIC VOID
BCMWL_UNDIGetInitInfo(PXE_CDB *cdb_ptr,	struct wl_info *wl)
{
	PXE_DB_GET_INIT_INFO  *db_ptr;
	int i;

	db_ptr = (PXE_DB_GET_INIT_INFO *)(cdb_ptr->DBaddr);
	/* The driver allocates it's own memory */

	db_ptr->MemoryRequired = 0;
	db_ptr->FrameDataLen = PXE_MAX_TXRX_UNIT_ETHER;

	for (i = 0; i < 4; i++)
		db_ptr->LinkSpeeds[i] = 0;

	db_ptr->NvCount = 0;
	db_ptr->NvWidth = 0;    /*  dword */
	db_ptr->MediaHeaderLen = PXE_MAC_HEADER_LEN_ETHER;
	db_ptr->HWaddrLen = PXE_HWADDR_LEN_ETHER;

	wl->nmcastcnt = MIN(MAX_MCAST_ADDRESS_CNT, MAXMULTILIST);
	db_ptr->MCastFilterCnt = wl->nmcastcnt;

	db_ptr->TxBufCnt = (UINT16)NTXBUF;
	db_ptr->TxBufSize = (UINT16)PKTBUFSZ;
	db_ptr->RxBufCnt = (UINT16)NRXBUF;
	db_ptr->RxBufSize = (UINT16)PKTBUFSZ;
	db_ptr->IFtype = PXE_IFTYPE_ETHERNET;
#ifdef EDK2_NAMES
	db_ptr->SupportedDuplexModes = 0;
	db_ptr->SupportedLoopBackModes = 0;
#else
	db_ptr->Duplex = 0;
	db_ptr->LoopBack = 0;
#endif
	cdb_ptr->StatFlags = PXE_STATFLAGS_CABLE_DETECT_SUPPORTED;

	return;
}

STATIC VOID
BCMWL_UNDIGetConfigInfo(PXE_CDB *cdb_ptr,
                        struct wl_info *wl)
{
	UINT16 i;
	PXE_DB_GET_CONFIG_INFO  *db_ptr;
	UINT32 value;
	UINTN Seg;

	db_ptr = (PXE_DB_GET_CONFIG_INFO *)(cdb_ptr->DBaddr);

	db_ptr->pci.BusType = PXE_BUSTYPE_PCI;
	wl->PciIoIntf->GetLocation(wl->PciIoIntf,
	                           &Seg,
	                           (UINTN *)&db_ptr->pci.Bus,
	                           (UINTN *)&db_ptr->pci.Device,
	                           (UINTN *)&db_ptr->pci.Function);

	for (i = 0; i < MAX_PCI_CONFIG_LEN; i++)  {
		wl->PciIoIntf->Pci.Read(wl->PciIoIntf,
		                        EfiPciIoWidthUint32,
		                        i*4,
		                        1,
		                        &value);
		db_ptr->pci.Config.Dword[i] = value;
	}
}

STATIC VOID
BCMWL_UNDIInitialize(PXE_CDB *cdb_ptr, struct wl_info *wl)
{
	EFI_TPL tpl;

	/* Ignore everything coming down from the protocol driver.
	 * This driver manages it's own resources
	 */
	WL_LOCK(wl, tpl);

	/* If already up, then do nothing */
	if (wl->pub->up)
		goto Done;

	if (wl_up(wl)) {
		WL_UNLOCK(wl, tpl);
		return;
	}

Done:
	if ((cdb_ptr->OpFlags != PXE_OPFLAGS_INITIALIZE_DO_NOT_DETECT_CABLE)) {
	  if (!WLC_CONNECTED(wl->wlc)) {
			cdb_ptr->StatCode = PXE_STATCODE_NOT_STARTED;
			cdb_ptr->StatFlags |= PXE_STATFLAGS_INITIALIZED_NO_MEDIA;
		}
	}

	wl->UndiState = PXE_STATFLAGS_GET_STATE_INITIALIZED;
	WL_UNLOCK(wl, tpl);

	return;
}

STATIC VOID
BCMWL_UNDIReset(PXE_CDB *cdb_ptr,
                struct wl_info *wl)
{
	EFI_TPL tpl;
	WL_ERROR(("%s %d\n", __FUNCTION__, __LINE__));

	WL_LOCK(wl, tpl);
	wl_reset(wl);
	WL_UNLOCK(wl, tpl);
}

STATIC VOID
BCMWL_UNDIShutdown(PXE_CDB *cdb_ptr,
                   struct wl_info *wl)
{
	EFI_TPL tpl;
	WL_TRACE(("%s %d\n", __FUNCTION__, __LINE__));

	WL_LOCK(wl, tpl);
	wl->UndiState = PXE_STATFLAGS_GET_STATE_STARTED;
	WL_UNLOCK(wl, tpl);
}

STATIC VOID
BCMWL_UNDIInterruptCtrl(PXE_CDB *cdb_ptr,
	struct wl_info *wl)
{
	WL_ERROR(("%s %d OpFlags 0x%x\n", __FUNCTION__, __LINE__, cdb_ptr->OpFlags));
}

STATIC VOID
BCMWL_UNDIRxFilter(PXE_CDB *cdb_ptr,
                   struct wl_info *wl)
{
	UINT16 new_filter, opflags;
	PXE_CPB_RECEIVE_FILTERS *pRxFilter;
	PXE_DB_RECEIVE_FILTERS *pRxReturnFilter;
	UINT32 i, mc_count;
	EFI_TPL tpl;
	wlc_bsscfg_t *bsscfg;

	bsscfg = wl_bsscfg_find(wl);
	ASSERT(bsscfg != NULL);

	opflags = cdb_ptr->OpFlags;
	new_filter = opflags &
	        (PXE_OPFLAGS_RECEIVE_FILTER_UNICAST |
	         PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST |
	         PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST |
	         PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS |
	         PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST);

	switch (opflags & PXE_OPFLAGS_RECEIVE_FILTER_OPMASK)   {
	case PXE_OPFLAGS_RECEIVE_FILTER_READ:
		WL_LOCK(wl, tpl);
		if (cdb_ptr->DBsize && bsscfg->nmulticast) {
			pRxReturnFilter = (PXE_DB_RECEIVE_FILTERS *)cdb_ptr->DBaddr;

			bzero((char *)cdb_ptr->DBaddr, (UINT16)cdb_ptr->DBsize);

			for (i = 0; i < bsscfg->nmulticast; i++)
				bcopy((char *) &bsscfg->multicast[i],
				      (char *)&pRxReturnFilter->MCastList[i],
				      ETHER_ADDR_LEN);
		}

		cdb_ptr->StatFlags |= PXE_STATFLAGS_RECEIVE_FILTER_BROADCAST;
		if (bsscfg->nmulticast)
			cdb_ptr->StatFlags |= PXE_STATFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST;
		else if (bsscfg->allmulti)
			cdb_ptr->StatFlags |= PXE_STATFLAGS_RECEIVE_FILTER_ALL_MULTICAST;
		cdb_ptr->StatFlags |= PXE_STATFLAGS_RECEIVE_FILTER_UNICAST;

		if (wl->pub->promisc)
			cdb_ptr->StatFlags |= PXE_STATFLAGS_RECEIVE_FILTER_PROMISCUOUS;
		WL_UNLOCK(wl, tpl);
		break;

	case PXE_OPFLAGS_RECEIVE_FILTER_ENABLE: {
		/*  There should be at least one other filter bit set. */
		if (!new_filter)
			goto Error;

		WL_LOCK(wl, tpl);
		if (new_filter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) {
			if (cdb_ptr->CPBsize != PXE_CPBSIZE_NOT_USED) {
				pRxFilter = (PXE_CPB_RECEIVE_FILTERS *)cdb_ptr->CPBaddr;
				mc_count = (cdb_ptr->CPBsize/PXE_MAC_LENGTH);

				for (i = 0; i < mc_count; i++) {
					if (i >= wl->nmcastcnt) {
						bsscfg->allmulti = TRUE;
						i = 0;
						break;
					}

					bcopy((char *)&pRxFilter->MCastList[i],
					      &bsscfg->multicast[i],
					      ETHER_ADDR_LEN);
				}

				bsscfg->nmulticast = i;
			}
		}

		if (new_filter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST)
			bsscfg->allmulti = TRUE;

		if (new_filter & PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS)
			wl_set(wl, WLC_SET_PROMISC, 1);
		WL_UNLOCK(wl, tpl);
	}
		break;

	case PXE_OPFLAGS_RECEIVE_FILTER_DISABLE:
		WL_LOCK(wl, tpl);
		if (new_filter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST)
			bsscfg->nmulticast = 0;

		if (new_filter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST)
			bsscfg->allmulti = FALSE;

		if (new_filter & PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS)
			wl_set(wl, WLC_SET_PROMISC, 0);
		WL_UNLOCK(wl, tpl);
		break;

	default:
		goto Error;
	}

	return;

Error:
	cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
	cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
}

STATIC VOID
BCMWL_UNDIStationAddr(PXE_CDB *cdb_ptr,
                      struct wl_info *wl)
{
	PXE_CPB_STATION_ADDRESS *cpb_ptr;
	PXE_DB_STATION_ADDRESS *db_ptr;
	uint8 addr[ETHER_ADDR_LEN];
	uint8 perm_addr[ETHER_ADDR_LEN];
	EFI_TPL tpl;
	EFI_STATUS Status;

	/* We want to configure MAC address */
	cpb_ptr = (PXE_CPB_STATION_ADDRESS *)(cdb_ptr->CPBaddr);

	if ((cpb_ptr != NULL) &&
	    (cdb_ptr->CPBsize != sizeof(PXE_CPB_STATION_ADDRESS)))
		goto Error;

	switch (cdb_ptr->OpFlags) {
	case PXE_OPFLAGS_STATION_ADDRESS_WRITE:
		if (cpb_ptr != (UINT64) 0) {
			bcopy(cpb_ptr->StationAddr, addr, ETHER_ADDR_LEN);
			WL_LOCK(wl, tpl);
			Status = wl_cur_etheraddr_set(wl, addr);
			WL_UNLOCK(wl, tpl);
			if (Status != EFI_SUCCESS) {
				EFI_ERRMSG(Status, "Error setting cur_etheraddr\r\n");
				goto Error;
			}
		}
		break;

	case PXE_OPFLAGS_STATION_ADDRESS_RESET: {
		/* Get the perm_etheraddr and set cur_etheraddr to it */
		WL_LOCK(wl, tpl);
		Status = wl_iovar_op((void *)wl->wlc, "perm_etheraddr", NULL, 0,
		                        addr, ETHER_ADDR_LEN, IOV_GET, NULL);
		WL_UNLOCK(wl, tpl);
		if (Status != EFI_SUCCESS) {
			EFI_ERRMSG(Status, "Error getting perm_etheraddr\r\n");
			goto Error;
		}
		WL_LOCK(wl, tpl);
		Status = wl_cur_etheraddr_set(wl, addr);
		WL_UNLOCK(wl, tpl);
		if (Status != EFI_SUCCESS) {
			EFI_ERRMSG(Status, "Error resetting cur_etheraddr\r\n");
			goto Error;
		}
	}
		break;

	default:
		break;
	}

	if (cdb_ptr->DBaddr != (UINT64)0) {
		/* Upper layer wants to query MAC address */
		db_ptr = (PXE_DB_STATION_ADDRESS *)(cdb_ptr->DBaddr);
		/*  fill it with the new values */
		WL_LOCK(wl, tpl);
		Status = wl_iovar_op(wl, "cur_etheraddr", NULL, 0,
		                     addr, ETHER_ADDR_LEN, IOV_GET, NULL);
		Status = wl_iovar_op(wl, "perm_etheraddr", NULL, 0,
		                        perm_addr, ETHER_ADDR_LEN, IOV_GET, NULL);

		WL_UNLOCK(wl, tpl);

		bzero(&db_ptr->StationAddr, PXE_MAC_LENGTH);
		bzero(&db_ptr->BroadcastAddr, PXE_MAC_LENGTH);
		bzero(&db_ptr->PermanentAddr, PXE_MAC_LENGTH);

		bcopy(addr, &db_ptr->StationAddr, ETHER_ADDR_LEN);
		bcopy(&ether_bcast, &db_ptr->BroadcastAddr, ETHER_ADDR_LEN);

		bcopy(perm_addr, &db_ptr->PermanentAddr, ETHER_ADDR_LEN);
	}

	return;
Error:
	cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
	cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
}

STATIC int
wl_db_ptr_data_update(void *ctx, uint8 *data, uint16 type, uint16 len)
{
	PXE_DB_STATISTICS *db_ptr = ctx;
	int res = BCME_OK;

	switch (type) {
		case WL_CNT_XTLV_WLC: {
			wl_cnt_wlc_t *cnt = (wl_cnt_wlc_t *)data;
			if (len > sizeof(wl_cnt_wlc_t)) {
				printf("counter structure length invalid! %d > %d\n",
					len, (int)sizeof(wl_cnt_wlc_t));
			}
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_TOTAL_FRAMES])[0] =
				cnt->rxframe + cnt->rxerror;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_TOTAL_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_GOOD_FRAMES])[0] = cnt->rxframe;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_GOOD_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_UNDERSIZE_FRAMES])[0] =
				cnt->rxrunt;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_UNDERSIZE_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_OVERSIZE_FRAMES])[0] =
				cnt->rxgiant;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_OVERSIZE_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_DROPPED_FRAMES])[0] =
				cnt->rxerror;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_DROPPED_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_TOTAL_BYTES])[0] =
				cnt->rxbyte;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_TOTAL_BYTES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_TOTAL_FRAMES])[0] =
				cnt->txframe + cnt->txerror;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_TOTAL_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_GOOD_FRAMES])[0] =
				cnt->txframe;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_GOOD_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_DROPPED_FRAMES])[0] =
				cnt->txerror;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_DROPPED_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_UNICAST_FRAMES])[0] =
				cnt->txfrag;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_UNICAST_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_MULTICAST_FRAMES])[0] =
				cnt->txmulti;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_MULTICAST_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_BROADCAST_FRAMES])[0] =
				cnt->txmulti;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_BROADCAST_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_TOTAL_BYTES])[0] =
				cnt->txbyte;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_TX_TOTAL_BYTES])[1] = 0;

			break;
		}
		case WL_CNT_XTLV_CNTV_LE10_UCODE: {
			wl_cnt_v_le10_mcst_t *cnt = (wl_cnt_v_le10_mcst_t *)data;
			if (len != sizeof(wl_cnt_v_le10_mcst_t)) {
				printf("counter structure length mismatch! %d != %d\n",
					len, (int)sizeof(wl_cnt_v_le10_mcst_t));
			}
			/* Recvd uncast data in my bss */
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_UNICAST_FRAMES])[0] =
				cnt->rxdfrmucastmbss;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_UNICAST_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_BROADCAST_FRAMES])[0] =
				cnt->rxdfrmmcast;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_BROADCAST_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_MULTICAST_FRAMES])[0] =
				cnt->rxdfrmmcast;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_MULTICAST_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_CRC_ERROR_FRAMES])[0] =
				cnt->rxbadfcs;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_CRC_ERROR_FRAMES])[1] = 0;
			break;
		}
		case WL_CNT_XTLV_LT40_UCODE_V1:
		case WL_CNT_XTLV_GE40_UCODE_V1:
		{
			/* Offsets of the macstat counters variables here are the same in
			 * wl_cnt_lt40mcst_v1_t and wl_cnt_ge40mcst_v1_t.
			 * So we can just cast to wl_cnt_lt40mcst_v1_t here.
			 */
			wl_cnt_lt40mcst_v1_t *cnt = (wl_cnt_lt40mcst_v1_t *)data;
			if (len != WL_CNT_MCST_STRUCT_SZ) {
				printf("counter structure length mismatch! %d != %d\n",
					len, WL_CNT_MCST_STRUCT_SZ);
			}
			/* Recvd uncast data in my bss */
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_UNICAST_FRAMES])[0] =
				cnt->rxdtucastmbss;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_UNICAST_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_BROADCAST_FRAMES])[0] =
				cnt->rxdtmcast;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_BROADCAST_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_MULTICAST_FRAMES])[0] =
				cnt->rxdtmcast;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_MULTICAST_FRAMES])[1] = 0;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_CRC_ERROR_FRAMES])[0] =
				cnt->rxbadfcs;
			((UINT32 *)&db_ptr->Data[PXE_STATISTICS_RX_CRC_ERROR_FRAMES])[1] = 0;
			break;
		}
		default:
			WL_ERROR(("%s %d: Unsupported type %d\n", __FUNCTION__, __LINE__, type));
			break;
	}
	return res;
}


STATIC VOID
BCMWL_UNDIStatistics(PXE_CDB *cdb_ptr,
                     struct wl_info *wl)
{
	PXE_DB_STATISTICS *db_ptr;
	EFI_TPL tpl;
	UINT8 cntbuf[WL_CNTBUF_MAX_SIZE];
	wl_cnt_info_t *cntinfo;
	EFI_STATUS Status;

	/* Don't support resetting stats */
	if (cdb_ptr->OpFlags & PXE_OPFLAGS_STATISTICS_RESET) {
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_UNSUPPORTED;
		return;
	}
	if (cdb_ptr->DBaddr == (UINT64)0) {
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
		return;
	}
	db_ptr = (PXE_DB_STATISTICS *)cdb_ptr->DBaddr;
	memset(cntbuf, 0, WL_CNTBUF_MAX_SIZE);
	WL_LOCK(wl, tpl);
	/* Get it through iovar */
	Status = wl_iovar_op(wl, "counters", NULL, 0,
	                     cntbuf, WL_CNTBUF_MAX_SIZE, FALSE, NULL);
	WL_UNLOCK(wl, tpl);

	if (Status != EFI_SUCCESS) {
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
		return;
	}
	cntinfo = (wl_cnt_info_t *)cntbuf;
	if (cntinfo->version > WL_CNT_T_VERSION) {
		WL_ERROR(("\tIncorrect version of counters struct: expected %d; got %d\n",
			WL_CNT_T_VERSION, cntinfo->version));
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
		return;
	}

	/* No need to give corerev here as we don't refer to
	 * variables that are different per corerev.
	 */
	Status = wl_cntbuf_to_xtlv_format(wl->osh, cntinfo, WL_CNTBUF_MAX_SIZE, 0);
	if (Status != EFI_SUCCESS) {
		WL_ERROR(("\twl_cntbuf_to_xtlv_format failed! %d\n", Status));
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
		return;
	}

	db_ptr->Supported = ((1 << PXE_STATISTICS_RX_TOTAL_FRAMES) |
	                     (1 << PXE_STATISTICS_RX_GOOD_FRAMES) |
	                     (1 << PXE_STATISTICS_RX_UNDERSIZE_FRAMES) |
	                     (1 << PXE_STATISTICS_RX_OVERSIZE_FRAMES) |
	                     (1 << PXE_STATISTICS_RX_DROPPED_FRAMES) |
	                     (1 << PXE_STATISTICS_RX_UNICAST_FRAMES) |
	                     (1 << PXE_STATISTICS_RX_BROADCAST_FRAMES) |
	                     (1 << PXE_STATISTICS_RX_MULTICAST_FRAMES) |
	                     (1 << PXE_STATISTICS_RX_CRC_ERROR_FRAMES) |
	                     (1 << PXE_STATISTICS_RX_TOTAL_BYTES) |
	                     (1 << PXE_STATISTICS_TX_TOTAL_FRAMES) |
	                     (1 << PXE_STATISTICS_TX_GOOD_FRAMES) |
	                     (1 << PXE_STATISTICS_TX_DROPPED_FRAMES) |
	                     (1 << PXE_STATISTICS_TX_UNICAST_FRAMES) |
	                     (1 << PXE_STATISTICS_TX_BROADCAST_FRAMES) |
	                     (1 << PXE_STATISTICS_TX_MULTICAST_FRAMES) |
	                     (1 << PXE_STATISTICS_TX_TOTAL_BYTES));


	if (bcm_unpack_xtlv_buf(db_ptr, cntinfo->data, cntinfo->datalen,
		BCM_XTLV_OPTION_ALIGN32, wl_db_ptr_data_update) != BCME_OK) {
		WL_ERROR(("\tbcm_unpack_xtlv_buf failed!\n"));
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
	}
}

STATIC VOID
BCMWL_UNDIIpToMac(PXE_CDB *cdb_ptr,
	struct wl_info *wl)
{
	PXE_CPB_MCAST_IP_TO_MAC *cpb;
	PXE_DB_MCAST_IP_TO_MAC *db_ptr;

	cpb = (PXE_CPB_MCAST_IP_TO_MAC *)(UINTN)cdb_ptr->CPBaddr;
	db_ptr = (PXE_DB_MCAST_IP_TO_MAC *)(UINTN)cdb_ptr->DBaddr;

	if (cdb_ptr->OpFlags & PXE_OPFLAGS_MCAST_IPV6_TO_MAC) {
		/* We don't support IPv6 yet. */
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_UNSUPPORTED;
		return;
	}

	WL_ERROR(("IP = %02d.%02d.%02d.%02d\n",
	          ((UINT8 *)&cpb->IP.IPv4)[0],
	          ((UINT8 *)&cpb->IP.IPv4)[1],
	          ((UINT8 *)&cpb->IP.IPv4)[2],
	          ((UINT8 *)&cpb->IP.IPv4)[3]));

	/* Convert Multicast IP address to Multicast MAC adress per RFC 1112 */
	db_ptr->MAC[0] = 0x01;
	db_ptr->MAC[1] = 0x00;
	db_ptr->MAC[2] = 0x5e;
	db_ptr->MAC[3] = ((UINT8 *)&cpb->IP.IPv4)[1] & 0x7f;
	db_ptr->MAC[4] = ((UINT8 *)&cpb->IP.IPv4)[2];
	db_ptr->MAC[5] = ((UINT8 *)&cpb->IP.IPv4)[3];

	return;
}

STATIC VOID
BCMWL_UNDINVData(PXE_CDB *cdb_ptr,
	struct wl_info *wl)
{
	cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
	cdb_ptr->StatCode = PXE_STATCODE_UNSUPPORTED;
}

STATIC VOID
BCMWL_UNDIGetStatus(PXE_CDB *cdb_ptr,
	struct wl_info *wl)
{
	PXE_DB_GET_STATUS *db_ptr;
	UINT16 tx_buffer_size;
	UINT32 i;
	EFI_TPL tpl;

	WL_PORT(("%s %d\n", __FUNCTION__, __LINE__));

	if (cdb_ptr->DBsize == 0) {
		WL_ERROR(("UM_UNDIGetStatus(): DBsize is zero\n\r"));
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
		return;
	}

	db_ptr = (PXE_DB_GET_STATUS *)cdb_ptr->DBaddr;

	WL_LOCK(wl, tpl);
	/* Update once */
	wl_isr(wl);

	/* If the scan is in progress then the packets won't be sent out. So check only if
	 * scan is not in progress
	 */
	if (!wlc_scan_inprog(wl->wlc)) {
		i = 0;

		/* Now wait till all pending packets are done */
		while (wl->txpendcnt != (uint)pktq_len(&wl->txdone) && i < (WL_PKTTX_TIMEOUT/100)) {
			OSL_DELAY(100);
			wl_isr(wl);
			i++;
		}

		if (wl->txpendcnt != (uint)pktq_len(&wl->txdone))
			WL_ERROR(("wl%d: TX pkt timed out. %d pkts pending\n",
			          wl->pub->unit, wl->txpendcnt - pktq_len(&wl->txdone)));
	}

	if (pktq_len(&wl->rxsendup) != 0) {
		struct EFI_PKT *pkt = pktq_peek((struct pktq *)&wl->rxsendup, NULL);
		db_ptr->RxFrameLen = (UINT32)pkt->BufferSize;
	} else
		db_ptr->RxFrameLen = 0;

	db_ptr->reserved = 0;

	if (pktq_len(&wl->txdone) == 0)
		cdb_ptr->StatFlags |= (PXE_STATFLAGS_GET_STATUS_NO_TXBUFS_WRITTEN |
		                       PXE_STATFLAGS_GET_STATUS_TXBUF_QUEUE_EMPTY);

	/* Update the interrupt status first before dequeueing */
	if (cdb_ptr->OpFlags & PXE_OPFLAGS_GET_INTERRUPT_STATUS) {
		/* Set the interrupt status based on whether packets are there */
		if (pktq_len(&wl->rxsendup) != 0)
			cdb_ptr->StatFlags |= PXE_STATFLAGS_GET_STATUS_RECEIVE;

		if (pktq_len(&wl->txdone) != 0)
			cdb_ptr->StatFlags |= PXE_STATFLAGS_GET_STATUS_TRANSMIT;
	}

	/* Enqueue as many tx buffers as possible based on remaining DBsize */
	if (cdb_ptr->OpFlags & PXE_OPFLAGS_GET_TRANSMITTED_BUFFERS) {
		/* Decrement to account for first 2 UINT32 fields */
		tx_buffer_size = cdb_ptr->DBsize - sizeof(UINT64);
		cdb_ptr->DBsize = sizeof(UINT64);
		i = 0;
		while (!pktq_empty(&wl->txdone) &&
		      tx_buffer_size >= sizeof(UINT64)) {
			struct EFI_PKT *pkt = pktdeq(&wl->txdone);
			wl->txpendcnt--;
			db_ptr->TxBuffer[i] = (UINT64)pkt->Buffer;

			MFREE(wl->osh, pkt, sizeof(struct EFI_PKT));
			i++;
			tx_buffer_size -= sizeof(UINT64);
		}
	}

	WL_UNLOCK(wl, tpl);
}

STATIC VOID
BCMWL_UNDIFillHeader(PXE_CDB *cdb_ptr,
	struct wl_info *wl)
{
	PXE_CPB_FILL_HEADER *cpb;
	PXE_CPB_FILL_HEADER_FRAGMENTED *cpbf;
	struct ether_header  *mac_hdr;
	UINTN i;

	WL_PORT(("%s %d\n", __FUNCTION__, __LINE__));

	if (cdb_ptr->CPBsize == PXE_CPBSIZE_NOT_USED) {
		cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
		return;
	}

	if (cdb_ptr->OpFlags & PXE_OPFLAGS_FILL_HEADER_FRAGMENTED) {
		cpbf = (PXE_CPB_FILL_HEADER_FRAGMENTED *)cdb_ptr->CPBaddr;

		/*  Assume 1st fragment is big enough for the mac header */
		if (!cpbf->FragCnt || cpbf->FragDesc[0].FragLen < PXE_MAC_HEADER_LEN_ETHER) {
			/*  no buffers given */
			cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
			cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
			return;
		}

		mac_hdr = (struct ether_header *)cpbf->FragDesc[0].FragAddr;
		mac_hdr->ether_type = cpbf->Protocol;

		for (i = 0; i < PXE_HWADDR_LEN_ETHER; i++) {
			mac_hdr->ether_dhost[i] =  cpbf->DestAddr[i];
			mac_hdr->ether_shost[i]  = cpbf->SrcAddr[i];
		}
	} else {
		cpb = (PXE_CPB_FILL_HEADER *)cdb_ptr->CPBaddr;

		mac_hdr = (struct ether_header *)cpb->MediaHeader;
		mac_hdr->ether_type = cpb->Protocol;

		for (i = 0; i < PXE_HWADDR_LEN_ETHER; i++) {
			mac_hdr->ether_dhost[i] =  cpb->DestAddr[i];
			mac_hdr->ether_shost[i]  = cpb->SrcAddr[i];
		}
	}
}

STATIC VOID
BCMWL_UNDITxPacket(PXE_CDB *cdb_ptr,
	struct wl_info *wl)
{
	PXE_CPB_TRANSMIT *tx_ptr_1;
	struct EFI_PKT *pxe_pkt;
	void *pkt;
	EFI_TPL tpl;

	WL_PORT(("%s %d\n", __FUNCTION__, __LINE__));

	WL_LOCK(wl, tpl);
	if (cdb_ptr->CPBsize == PXE_CPBSIZE_NOT_USED) {
		cdb_ptr->StatCode = PXE_STATCODE_INVALID_CDB;
		goto Error;
	}

	if (wl->TxFlowControl == TRUE) {
		cdb_ptr->StatCode = PXE_STATCODE_QUEUE_FULL;
		goto Error;
	}

	if (cdb_ptr->OpFlags & PXE_OPFLAGS_TRANSMIT_FRAGMENTED) {
		cdb_ptr->StatCode = PXE_STATCODE_UNSUPPORTED;
		goto Error;
	}

	if (!WLC_CONNECTED(wl->wlc) || (wl->ssid_changed)) {
		cdb_ptr->StatCode = PXE_STATCODE_NO_MEDIA;
		wl->ssid_changed = FALSE;
		goto Error;
	}

	tx_ptr_1 = (PXE_CPB_TRANSMIT *)cdb_ptr->CPBaddr;

	pxe_pkt = MALLOC(wl->osh, sizeof(struct EFI_PKT));

	if (pxe_pkt == NULL) {
		cdb_ptr->StatCode = PXE_STATCODE_NOT_ENOUGH_MEMORY;
		goto Error;
	}

	/* Fill in pxe_pkt information */
	pxe_pkt->Link = NULL;
	pxe_pkt->Buffer = (void *)(uintptr)tx_ptr_1->FrameAddr;
	pxe_pkt->BufferSize = tx_ptr_1->DataLen;

	/* Following are being ignored for now */
	pxe_pkt->HeaderSize = tx_ptr_1->MediaheaderLen;
	pxe_pkt->SrcAddr = NULL;
	pxe_pkt->DestAddr = NULL;
	pxe_pkt->Protocol = NULL;

	if ((pkt = PKTFRMNATIVE(wl->osh, pxe_pkt)) == NULL) {
		MFREE(wl->osh, pxe_pkt, sizeof(struct EFI_PKT));
		pxe_pkt = NULL;
		cdb_ptr->StatCode = PXE_STATCODE_NOT_ENOUGH_MEMORY;
		goto Error;
	}

	wlc_sendpkt((void *)wl->wlc, pkt, NULL);

	wl->txpendcnt++;
	WL_UNLOCK(wl, tpl);

	return;

Error:
	WL_UNLOCK(wl, tpl);
	cdb_ptr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;

}

STATIC VOID
BCMWL_UNDIRxPacket(PXE_CDB *cdb_ptr,
                   struct wl_info *wl)
{
	PXE_CPB_RECEIVE *pRxCpd;
	PXE_DB_RECEIVE *pRxDb;
	struct EFI_PKT *pxe_pkt;
	UINT32 CopyLen;
	EFI_TPL tpl;

	WL_TRACE(("%s %d\n", __FUNCTION__, __LINE__));

	pRxCpd = (PXE_CPB_RECEIVE *)cdb_ptr->CPBaddr;
	pRxDb = (PXE_DB_RECEIVE *)cdb_ptr->DBaddr;

	WL_LOCK(wl, tpl);
	if (!WLC_CONNECTED(wl->wlc) || (wl->ssid_changed)) {
		cdb_ptr->StatCode = PXE_STATCODE_NO_MEDIA;
		wl->ssid_changed = FALSE;
		WL_UNLOCK(wl, tpl);
		return;
	}

	if (pktq_empty(&wl->rxsendup)) {
		cdb_ptr->StatCode = PXE_STATCODE_NO_DATA;
		WL_UNLOCK(wl, tpl);
		return;
	}

	pxe_pkt = pktdeq(&wl->rxsendup);
	WL_UNLOCK(wl, tpl);

	CopyLen  = MIN((UINT32)pxe_pkt->BufferSize, pRxCpd->BufferLen);

	bcopy(pxe_pkt->Buffer, pRxCpd->BufferAddr, CopyLen);

	pRxDb->FrameLen = (UINT32)pxe_pkt->BufferSize;
	pRxDb->MediaHeaderLen = PXE_MAC_HEADER_LEN_ETHER;
	if (ETHER_ISBCAST(pxe_pkt->DestAddr))
		pRxDb->Type = PXE_FRAME_TYPE_BROADCAST;
	else if (ETHER_ISMULTI(pxe_pkt->DestAddr))
		pRxDb->Type = PXE_FRAME_TYPE_MULTICAST;
	else
		pRxDb->Type = PXE_FRAME_TYPE_UNICAST;

	bcopy(pxe_pkt->SrcAddr, pRxDb->SrcAddr, ETHER_ADDR_LEN);
	bcopy(pxe_pkt->DestAddr, pRxDb->DestAddr, ETHER_ADDR_LEN);
	pRxDb->Protocol = *pxe_pkt->Protocol;

	MFREE(wl->osh, pxe_pkt->Buffer, (uint)pxe_pkt->BufferSize);
	MFREE(wl->osh, pxe_pkt, sizeof(struct EFI_PKT));
}

#endif /* BCMWLUNDI */

/* WL ioctl related code */
/* Helper functions to enable locking w/o repeating lot of code */
/* Also convert driver errors to OS errors */
STATIC
EFI_STATUS
wl_iovar_op(wl_info_t *wl, const char *name,
            void *params, int p_len, void *arg, int len,
            bool set, struct wlc_if *wlcif)
{
	int bcmerror;

	bcmerror = wlc_iovar_op(wl->wlc, name,
	                        params, p_len, arg, len, set, wlcif);
	if (bcmerror) {
		wl_print_lasterror(wl);
		wl->pub->bcmerror = bcmerror;
		return OSL_ERROR(bcmerror);
	}
	return EFI_SUCCESS;
}

STATIC INLINE
EFI_STATUS
wl_iovar_setint(wl_info_t *wl, const char *name, int arg)
{
	int bcmerror;

	bcmerror = wlc_iovar_op(wl->wlc, name, NULL, 0, &arg, sizeof(int),
	                        IOV_SET, NULL);
	if (bcmerror) {
		wl_print_lasterror(wl);
		wl->pub->bcmerror = bcmerror;
		return OSL_ERROR(bcmerror);
	}
	return EFI_SUCCESS;
}

/* Helper function */
STATIC INLINE
EFI_STATUS
wl_iovar_getint(wl_info_t *wl, const char *name, int *arg)
{
	int bcmerror;

	bcmerror = wlc_iovar_op(wl->wlc, name, NULL, 0, arg, sizeof(int),
	                        IOV_GET, NULL);
	if (bcmerror) {
		wl_print_lasterror(wl);
		wl->pub->bcmerror = bcmerror;
		return OSL_ERROR(bcmerror);
	}

	return EFI_SUCCESS;
}

STATIC INLINE
EFI_STATUS
wl_set(wl_info_t *wl, int cmd, int arg)
{
	int bcmerror;

	bcmerror = wlc_set(wl->wlc, cmd, arg);
	if (bcmerror) {
		wl_print_lasterror(wl);
		wl->pub->bcmerror = bcmerror;
		return OSL_ERROR(bcmerror);
	}

	return EFI_SUCCESS;

}

STATIC EFI_STATUS
wl_setInfraMode(wl_info_t *wl, int mode)
{
	EFI_STATUS Status;
	bool isup;
	int infra;

	wl_iovar_getint(wl, "infra_configuration", &infra);

	/* No change, just return */
	if ((mode == DOT11_BSSTYPE_INFRASTRUCTURE && infra == WL_BSSTYPE_INFRA) ||
	    (mode == DOT11_BSSTYPE_INDEPENDENT && infra == WL_BSSTYPE_INDEP) ||
	    (mode == DOT11_BSSTYPE_ANY && infra == WL_BSSTYPE_ANY))
	    return EFI_SUCCESS;

	isup = wl->pub->up;
	if (isup)
		wl_down(wl);

	/* infra_configuration DOES NOT match bss_type
	 * BSS_TYPE == 0 => infra == 1
	 * BSS_TYPE == 1 => infra == 0
	 * BSS_TYPE == 2 => infra == non-zero/one
	 */
	switch (mode) {
	case DOT11_BSSTYPE_INFRASTRUCTURE:
		infra = WL_BSSTYPE_INFRA;
		break;
	case DOT11_BSSTYPE_INDEPENDENT:
		infra = WL_BSSTYPE_INDEP;
		break;
	case DOT11_BSSTYPE_ANY:
		infra = WL_BSSTYPE_ANY;
		break;
	default:
		ASSERT(0);
	};

	Status = wl_iovar_setint(wl, "infra_configuration", infra);

	if (isup)
		wl_up(wl);

	wl_iovar_getint(wl, "infra_configuration", &infra);

	return Status;
}

STATIC INLINE
EFI_STATUS
wl_ioctl(wl_info_t *wl, int cmd, void *arg, int len, wlc_if_t *wlcif)
{
	int bcmerror;

	bcmerror = wlc_ioctl(wl->wlc, cmd, arg, len, wlcif);
	if (bcmerror) {
		wl_print_lasterror(wl);
		wl->pub->bcmerror = bcmerror;
		return OSL_ERROR(bcmerror);
	}

	return EFI_SUCCESS;
}

STATIC EFI_STATUS
EFIAPI
BCMWL_wlioctl(wl_info_t *wl, int cmd, void *arg, uint len)
{
	EFI_STATUS Status;
	EFI_TPL tpl;

	WL_LOCK(wl, tpl);
	Status = wl_ioctl(wl, cmd, arg, len, NULL);
	WL_UNLOCK(wl, tpl);

	return Status;
}

int
wl_osl_pcie_rc(struct wl_info *wl, uint op, int param)
{
	return 0;
}


static void
BCMWL_ScanFreeResults(wl_info_t *wl)
{
	wl->scan_done = 0;
	if (wl->scan_result_list) {
		uint i;
		wl_80211_scanresults_t *wl_sr;
		for (i = 0;  i < wl->scan_result_count; i++) {
			wl_sr = &wl->scan_result_list[i];
			ASSERT(wl_sr);
			if (wl_sr->ie_len != 0) {
				MFREE(wl->osh, wl_sr->ie_data, wl_sr->ie_len);
				wl_sr->ie_data = NULL;
			}
			ASSERT(wl_sr->ie_data == NULL);
		}
		gBS->FreePool(wl->scan_result_list);
		wl->scan_result_list = NULL;
	}
	wl->scan_result_count = 0;
}

static uint32
wl_cipher2appleadvcap(uint32 cipher)
{
	switch (cipher) {
	case WPA_CIPHER_NONE:
		return 0;
	case WPA_CIPHER_WEP_40:
	case WPA_CIPHER_WEP_104:
		return APPLE80211_AC_WEP;
	case WPA_CIPHER_TKIP:
		return APPLE80211_AC_TKIP;
	case WPA_CIPHER_AES_OCB:
	case WPA_CIPHER_AES_CCM:
		return APPLE80211_AC_AES;
	}
	return 0;
}

static uint32
wl_akm2appleadvcap(uint32 akm)
{
	if (akm == RSN_AKM_UNSPECIFIED)
		return APPLE80211_AC_Enterprise;
	else if (akm == RSN_AKM_PSK)
		return APPLE80211_AC_PSK;
	return 0;
}

static void
BCMWL_ScanResultAdvcapUpd(struct rsn_parms *rsn, UINT32 *advcap)
{
	int i;

	*advcap |= wl_cipher2appleadvcap(rsn->multicast);

	for (i = 0; i < rsn->ucount; i++)
		*advcap |= wl_cipher2appleadvcap(rsn->unicast[i]);

	for (i = 0; i < rsn->acount; i++)
		*advcap |= wl_akm2appleadvcap(rsn->auth[i]);
}

static void
BCMWL_ScanConvertResult(wl_info_t *wl, wlc_bss_info_t * bi, wl_80211_scanresults_t *wl_sr)
{
	APPLE_80211_SCANRESULT *asr = &wl_sr->sr;


	asr->channel = wf_chspec_ctlchan(bi->chanspec);
	asr->noise = bi->phy_noise;
	asr->rssi = bi->RSSI;
	asr->cap = bi->capability;
	bcopy(&bi->BSSID, (char *)&asr->bssid, ETHER_ADDR_LEN);

	asr->ssid_len = (uint8)MIN((int)bi->SSID_len, APPLE80211_MAX_SSID_LEN);
	bcopy(bi->SSID, asr->ssid, asr->ssid_len);

	/* Is WPA supported ? */
	if (bi->wpa.flags & RSN_FLAGS_SUPPORTED) {
		asr->advCap |= APPLE80211_AC_WPA;
		BCMWL_ScanResultAdvcapUpd(&bi->wpa, &asr->advCap);
	}

	if (bi->wpa2.flags & RSN_FLAGS_SUPPORTED) {
		asr->advCap |= APPLE80211_AC_WPA2;
		BCMWL_ScanResultAdvcapUpd(&bi->wpa2, &asr->advCap);
	}

	if (bi->capability & DOT11_CAP_PRIVACY) {
		/* If none of the WPA/encr are set and Privacy is set
		 * then it must be WEP
		 */
		if (asr->advCap == 0)
			asr->advCap |= APPLE80211_AC_WEP;
	}

	/* Save the IE as driver may need to filter this based on an IE passed when collecting
	 * the scan results
	 */
	if (bi->bcn_prb) {
		wl_sr->ie_len = bi->bcn_prb_len - DOT11_BCN_PRB_LEN;
		wl_sr->ie_data = MALLOC(wl->osh, wl_sr->ie_len);

		if (wl_sr->ie_data == NULL)
			wl_sr->ie_len = 0;
		else
			bcopy((uint8 *)bi->bcn_prb + DOT11_BCN_PRB_LEN, wl_sr->ie_data,
			      wl_sr->ie_len);
	} else {
		wl_sr->ie_len = 0;
		wl_sr->ie_data = NULL;
	}


}

static void
BCMWL_ScanSaveResults(void *context, int status, wlc_bsscfg_t *cfg)
{
	wl_info_t *wl = (wl_info_t *)context;

	wl_80211_scanresults_t *sr = NULL;
	wlc_bss_info_t *bi = NULL;
	wlc_info_t *wlc = wl->wlc;
	uint i;

	if (status == WLC_E_STATUS_ERROR)
		return;

	ASSERT(wl->scan_result_list == NULL);

	/* Scan done */
	if (wl->apple80211state == APPLE80211_S_SCAN)
		wl->apple80211state = APPLE80211_S_IDLE;


	if (wlc->scan_results->count == 0)
		return;

	/* Allocate the whole list and treat it like an array to make it easy to retrieve */
	sr = (wl_80211_scanresults_t *)EfiLibAllocateZeroPool(sizeof(wl_80211_scanresults_t) *
	                                                     wlc->scan_results->count);

	if (!sr) {
		WL_ERROR(("wl%d: Saving of scan results failed\n", wl->pub->unit));
		return;
	}

	wl->scan_result_count = wlc->scan_results->count;
	wl->scan_result_list = sr;

	for (i = 0; i < wlc->scan_results->count; i++) {
		bi = wlc->scan_results->ptrs[i];
		sr = &wl->scan_result_list[i];
		BCMWL_ScanConvertResult(wl, bi, sr);
	}

	wl->scan_done = 1;
}

STATIC EFI_STATUS
EFIAPI
BCMWL_ScanResultIoctl(wl_info_t *wl, APPLE_80211_SR_REQ *SrReq,
                      APPLE_80211_SCANRESULT *scanResult,
                      UINT32 *OutSize)
{
	EFI_TPL tpl;

	WL_LOCK(wl, tpl);
	if (wl->scan_done == 0) {
		WL_UNLOCK(wl, tpl);
		return EFI_NOT_READY;
	}

	bzero(scanResult, sizeof(APPLE_80211_SCANRESULT));

	if (SrReq->index >= (uint32)wl->scan_result_count)
		scanResult->channel = 0;
	else {
		uint8 *ie = SrReq->ie;
		wl_80211_scanresults_t *result = &wl->scan_result_list[SrReq->index];
		/* If length is non-zero, then locate the IE in the scan result */
		if (ie[1] != 0) {
			uint8 id = ie[0];
			if (bcm_parse_tlvs(result->ie_data, result->ie_len, id) == NULL) {
				*OutSize = 0;
				WL_UNLOCK(wl, tpl);
				return EFI_SUCCESS;
			}
		}
		bcopy(&result->sr, scanResult, sizeof(APPLE_80211_SCANRESULT));
	}
	WL_UNLOCK(wl, tpl);

	*OutSize = sizeof(APPLE_80211_SCANRESULT);

	return EFI_SUCCESS;
}

STATIC EFI_STATUS
EFIAPI
BCMWL_ScanIoctl(wl_info_t *wl,
                APPLE_80211_SCANPARAMS *scanParams)
{
	int bcmerr = 0;
	wlc_ssid_t req_ssid;
	chanspec_t chanspec_list[MAXCHANNEL];
	int chanspec_count;
	int passive_time;
	int i;
	EFI_TPL tpl;
	BOOLEAN abort;

	abort = (BOOLEAN)(scanParams->NumChannels == (UINT32)-1);
	if (abort) {
		WL_LOCK(wl, tpl);

		wlc_scan_abort((wl->wlc)->scan, WLC_E_STATUS_ABORT);
		wl->apple80211state = APPLE80211_S_IDLE;
		WL_UNLOCK(wl, tpl);
		return 0;
	}
	/* Convert the channel list */
	for (i = 0; i < (int)scanParams->NumChannels && i < MAXCHANNEL; i++)
		chanspec_list[i] = CH20MHZ_CHSPEC(scanParams->Channels[i]);

	chanspec_count = i;

	/* convert the passive time.
	 * 0 means default to EFI, so pass -1 which means default to wlc_scan_request
	 */
	passive_time = (scanParams->PassiveDwellTime == 0) ? -1 : scanParams->PassiveDwellTime;

	/* Prepare the ssid_t */
	bzero(&req_ssid, sizeof(req_ssid));
	req_ssid.SSID_len = MIN(DOT11_MAX_SSID_LEN, scanParams->SsidLen);
	bcopy(scanParams->Ssid, req_ssid.SSID, req_ssid.SSID_len);

	WL_LOCK(wl, tpl);

	BCMWL_ScanFreeResults(wl);

	bcmerr = wlc_scan_request(wl->wlc,
	                          DOT11_BSSTYPE_ANY,            /* bss_type, */
	                          &ether_bcast,                 /* bssid */
	                          1, &req_ssid,                 /* single ssid */
	                          DOT11_SCANTYPE_ACTIVE,        /* scan_type, */
	                          -1,                           /* nprobes, */
	                          scanParams->ActiveDwellTime,  /* active_time, */
	                          passive_time,                 /* passive_time, */
	                          -1,                           /* home_time, */
	                          chanspec_list,                /* chanspec_list */
	                          chanspec_count,               /* chanspec_num, */
	                          TRUE,                         /* save_prb, */
	                          BCMWL_ScanSaveResults,        /* fn, */
	                          wl);                          /* arg */

	if (bcmerr && VALID_BCMERROR(bcmerr)) {
		wl->pub->bcmerror = bcmerr;
		wl_print_lasterror(wl);
	} else
		wl->apple80211state = APPLE80211_S_SCAN;
	WL_UNLOCK(wl, tpl);

	return osl_error(bcmerr);
}

STATIC
EFI_STATUS
BCMWL_SetCipherKey(wl_info_t *wl, uint8 *keyData, int keylen, int keyIdx, bool txKey,
                   wl_wsec_key_t *key)
{
	if (keylen != 0)
		WL_ERROR(("setCipherKey %c: len %d, idx %d\n", txKey ? 'T' : 'R', keylen, keyIdx));

	bzero(key, sizeof(wl_wsec_key_t));

	key->len = keylen;
	key->index = keyIdx;

	if (keylen == 5)
		key->algo = CRYPTO_ALGO_WEP1;
	else if (keylen == 13)
		key->algo = CRYPTO_ALGO_WEP128;
	else if (keylen != 0) {
		WL_ERROR(("setCipherKey: bad keylen = %d\n", keylen));
		return EFI_INVALID_PARAMETER;
	}

	if (keylen && txKey)
		key->flags = WL_PRIMARY_KEY; /* this is also the tx key */

	if (keylen && keyData)
		bcopy(keyData, key->data, keylen);

	return EFI_SUCCESS;
}

STATIC EFI_STATUS
BCMWL_JoinIoctl(wl_info_t *wl, APPLE_80211_JOINPARAMS *joinParams)
{
	APPLE_80211_KEY	*Key = &joinParams->Key;
	wl_wsec_key_t wlkey;
	wsec_pmk_t pmk;
	uint mode;
	wlc_ssid_t ssid;
	EFI_STATUS Status;
	EFI_TPL tpl;
	int macAuth = DOT11_OPEN_SYSTEM;
	int wpaAuth = WPA_AUTH_DISABLED;
	int wsec = 0;

	/* first, handle special case:  disassociate */
	if (joinParams->bssType == APPLE80211_BSSTYPE_NONE) {
		WL_LOCK(wl, tpl);
		wl_set(wl, WLC_DISASSOC, 0);
		wl->ssid_changed = FALSE;
		WL_UNLOCK(wl, tpl);
		return EFI_SUCCESS;
	}

	/* Set Auth type */
	switch (joinParams->LowerAuth) {
	case APPLE80211_AUTHTYPE_OPEN:
		macAuth = DOT11_OPEN_SYSTEM;
		break;

	case APPLE80211_AUTHTYPE_SHARED:
		macAuth = DOT11_SHARED_KEY;
		break;

	case APPLE80211_AUTHTYPE_CISCO:
		macAuth = 0x80;
		break;

	default:
		return EFI_INVALID_PARAMETER;
	}

	WL_PORT(("LowerAuth %d UpperAuth %d CipherType 0x%x", joinParams->LowerAuth,
	          joinParams->UpperAuth, Key->CipherType));
	if (Key->CipherType != APPLE80211_CIPHER_NONE) {
		if (Key->KeyLen == 0)
			WL_PORT(("password: %s", Key->Password));
		else {
			uint i;
			WL_PORT(("Len: %d data: ", Key->KeyLen));
			for (i = 0; i < Key->KeyLen; i++)
				WL_PORT(("%x", Key->Key[i]));
		}
	}

	switch (joinParams->UpperAuth) {
	case APPLE80211_AUTHTYPE_NONE:
		break;
	case APPLE80211_AUTHTYPE_WPA_PSK:
		wpaAuth = WPA_AUTH_PSK;
		break;
	case APPLE80211_AUTHTYPE_WPA: /* Only WPA-PSK/WPA2-PSK */
		ASSERT(0);
		wpaAuth = WPA_AUTH_UNSPECIFIED;
		break;
	case APPLE80211_AUTHTYPE_WPA2_PSK:
		wpaAuth = WPA2_AUTH_PSK;
		break;
	case APPLE80211_AUTHTYPE_WPA2:
		ASSERT(0);
		wpaAuth = WPA2_AUTH_UNSPECIFIED;
		break;
	case APPLE80211_AUTHTYPE_LEAP:
		ASSERT(0);
		macAuth = 0x80;
		break;
	case APPLE80211_AUTHTYPE_8021X:
	case APPLE80211_AUTHTYPE_WPS:
		break;
	default:
		return EFI_INVALID_PARAMETER;
	}

	/* Set Cipher key */
	Status = EFI_SUCCESS;
	switch (Key->CipherType) {
	case APPLE80211_CIPHER_NONE:
		break;

	case APPLE80211_CIPHER_WEP_40:
	case APPLE80211_CIPHER_WEP_104:
		if (Key->KeyLen != 5 && Key->KeyLen != 13) {
			WL_ERROR(("wl%d: setCIPHER_KEY: cipher type WEP (%u) "
				  "but len was not 5 or 13 (%u)\n",
				  wl->pub->unit, Key->CipherType, Key->KeyLen));
			return EFI_INVALID_PARAMETER;
		}

		wsec = WEP_ENABLED;

		Status = BCMWL_SetCipherKey(wl, Key->Key,
		                            (int)Key->KeyLen,
		                            (int)Key->KeyIndex,
		                            TRUE,
		                            &wlkey);
		break;

	case APPLE80211_CIPHER_TKIP: /* WPA EAP is not supported */
	case APPLE80211_CIPHER_AES_CCM:
		if (strlen(Key->Password) == 0 && Key->Key != NULL && Key->KeyLen == PMK_LEN) {
			/* using raw key material */
			pmk.key_len = Key->KeyLen;
			memcpy(pmk.key, Key->Key, Key->KeyLen);
			pmk.flags &= ~WSEC_PASSPHRASE;
		} else {
			/* using preshared key which must be hashed */
			if ((strlen(Key->Password) < WSEC_MIN_PSK_LEN) ||
			    (strlen(Key->Password) > WSEC_MAX_PSK_LEN))
			{
				return EFI_INVALID_PARAMETER;
			}
			pmk.key_len = strlen(Key->Password);
			pmk.flags |= WSEC_PASSPHRASE;
			strncpy(pmk.key, Key->Password, pmk.key_len);
		}
		wsec = (Key->CipherType == APPLE80211_CIPHER_TKIP) ? TKIP_ENABLED : AES_ENABLED;
		break;
	case APPLE80211_CIPHER_PMK:
		break;

	default:
		WL_ERROR(("wl%d: unknown cipher type %d",
		          wl->pub->unit, Key->CipherType));
		return EFI_INVALID_PARAMETER;
	}

	if (EFI_ERROR(Status))
		return Status;

	switch (joinParams->bssType) {
	case APPLE80211_BSSTYPE_IBSS:
		mode = DOT11_BSSTYPE_INDEPENDENT;
		break;
	case APPLE80211_BSSTYPE_INFRA:
		mode = DOT11_BSSTYPE_INFRASTRUCTURE;
		break;
	case APPLE80211_BSSTYPE_ANY:
		mode = DOT11_BSSTYPE_ANY;
		break;
	default:
		WL_ERROR(("wl%d: unknown bss type %d",
		          wl->pub->unit, joinParams->bssType));
		return EFI_INVALID_PARAMETER;
	}

	/* Set SSID */
	ssid.SSID_len = joinParams->SsidLen;
	bcopy(joinParams->Ssid, ssid.SSID, MIN(ssid.SSID_len, sizeof(ssid.SSID)));

	/* Combine all the setting to facilitate locking */
	WL_LOCK(wl, tpl);

	/* Reset the state to idle */
	wl->apple80211state = APPLE80211_S_IDLE;
	wl->ssid_changed = TRUE;
	wl->wifi_bounced = TRUE;

	/* Reset the infrastructure mode to Any, as we can join any infra */
	wl_setInfraMode(wl, DOT11_BSSTYPE_ANY);

	/* Don't create IBSS if it does not exist */
	{
		int i = 1;
		wl_iovar_op(wl, "IBSS_join_only", NULL, 0, &i, sizeof(i), TRUE, NULL);
	}

	/* set mac-layer auth */
	wl_set(wl, WLC_SET_AUTH, macAuth);

	/* set upper-layer auth */
	wl_set(wl, WLC_SET_WPA_AUTH, wpaAuth);

	/* set in-driver supplicant */
	wl_iovar_setint(wl, "sup_wpa", ((wpaAuth & (WPA_AUTH_PSK | WPA2_AUTH_PSK)) == 0)? 0: 1);

	/* Clear the current keys */
	wlc_keymgmt_reset(wl->wlc->keymgmt, wl->wlc->cfg, NULL);
	wl_iovar_setint(wl, "wsec", 0);

	/* set wsec */
	wl_iovar_setint(wl, "wsec", wsec);

	/* set the key if wsec */
	if (wsec == WEP_ENABLED)
		wl_ioctl(wl, WLC_SET_KEY, &wlkey, sizeof(wlkey), NULL);
	else if (wsec != 0)
		wl_ioctl(wl, WLC_SET_WSEC_PMK,
		         &pmk, sizeof(pmk), NULL);

	Status = wl_ioctl(wl, WLC_SET_SSID, &ssid, sizeof(ssid), NULL);

	WL_UNLOCK(wl, tpl);

	return Status;
}

STATIC
EFI_STATUS
BCMWL_InfoIoctl(wl_info_t *wl, APPLE_80211_INFO *info, UINT32 *OutSize)
{
	wlc_bss_info_t bi;
	EFI_TPL tpl;
	wlc_bsscfg_t *bsscfg;
	wlc_bss_info_t *current_bss;

	bsscfg = wl_bsscfg_find(wl);
	ASSERT(bsscfg != NULL);

	current_bss = wlc_get_current_bss(bsscfg);

	*OutSize = sizeof(APPLE_80211_INFO);
	bzero(info, sizeof(APPLE_80211_INFO));
	WL_LOCK(wl, tpl);
	if (!bsscfg->associated) {
		WL_UNLOCK(wl, tpl);
		info->Channel = 0;
		return EFI_SUCCESS;
	}

	/* Lock while info is being copied */
	bcopy(current_bss, &bi, sizeof(wlc_bss_info_t));
	wl_ioctl(wl, WLC_GET_PHY_NOISE, &bi.phy_noise, sizeof(bi.phy_noise), NULL);
	WL_UNLOCK(wl, tpl);

	info->Channel = wf_chspec_ctlchan(bi.chanspec);
	info->SsidLen = bi.SSID_len;
	bcopy(bi.SSID, info->Ssid, bi.SSID_len);
	bcopy(&bi.BSSID, &info->bssid, ETHER_ADDR_LEN);
	info->Noise = bi.phy_noise;
	info->RSSI = bi.RSSI;

	return EFI_SUCCESS;
}

STATIC EFI_STATUS
BCMWL_IBSSIoctl(wl_info_t *wl, APPLE_80211_IBSSPARAMS *ibssParams)
{
	wlc_ssid_t ssid;
	EFI_STATUS Status;
	EFI_TPL tpl;
	int macAuth = DOT11_OPEN_SYSTEM;
	int wpaAuth = WPA_AUTH_DISABLED;
	int wsec = 0;
	wl_wsec_key_t wlkey;
	int i;

	WL_PORT(("%s %d\n", __FUNCTION__, __LINE__));

	/* A WEP Key is provided */
	if (ibssParams->keyLen != 0) {
		if (ibssParams->keyLen != 5 && ibssParams->keyLen != 13) {
			WL_ERROR(("wl%d: WEP Key"
				  "but len was not 5 or 13 (%u)\n",
				  wl->pub->unit, ibssParams->keyLen));
			return EFI_INVALID_PARAMETER;
		}

		wsec = WEP_ENABLED;

		Status = BCMWL_SetCipherKey(wl, ibssParams->key,
		                            (int)ibssParams->keyLen,
		                            (int)0,
		                            TRUE,
		                            &wlkey);
		if (EFI_ERROR(Status))
			return Status;
	}

	/* Set SSID */
	ssid.SSID_len = ibssParams->ssidLen;
	bcopy(ibssParams->ssid, ssid.SSID, MIN(ssid.SSID_len, sizeof(ssid.SSID)));

	/* Combine all the setting to facilitate locking */
	WL_LOCK(wl, tpl);

	/* Reset the state */
	wl->apple80211state = APPLE80211_S_IDLE;

	/* Clear the mode and joining restrictions */
	wl_setInfraMode(wl, DOT11_BSSTYPE_INDEPENDENT);

	i = 0;
	wl_iovar_op(wl, "IBSS_join_only", NULL, 0, &i, sizeof(i), TRUE, NULL);

	/* set mac-layer auth */
	wl_set(wl, WLC_SET_AUTH, macAuth);

	/* set upper-layer auth */
	wl_set(wl, WLC_SET_WPA_AUTH, wpaAuth);

	/* set in-driver supplicant */
	wl_iovar_setint(wl, "sup_wpa", 0);

	/* Clear the current keys */
	wlc_keymgmt_reset(wl->wlc->keymgmt, wl->wlc->cfg, NULL);
	wl_iovar_setint(wl, "wsec", 0);

	/* set wsec */
	wl_iovar_setint(wl, "wsec", wsec);

	/* set the key if wsec */
	if (wsec == WEP_ENABLED)
		wl_ioctl(wl, WLC_SET_KEY, &wlkey, sizeof(wlkey), NULL);

	wl_set(wl, WLC_SET_CHANNEL, ibssParams->channel);

	Status = wl_ioctl(wl, WLC_SET_SSID, &ssid, sizeof(ssid), NULL);

	WL_UNLOCK(wl, tpl);

	return Status;
}

STATIC EFI_STATUS
EFIAPI
BCMWL_80211Ioctl(IN void *this,
                 IN EFI_GUID *SelectorGuid,
                 IN VOID *InParams,
                 IN UINTN InSize,
                 OUT VOID *OutParams,
                 IN OUT UINT32 *OutSize)
{
	wl_info_t  *wl;
	EFI_STATUS Status;

	if (this == NULL) {
		Status = EFI_INVALID_PARAMETER;
		goto Done;
	}

	wl = EFI_WLINFO_80211_FROM_THIS((APPLE_80211_PROTOCOL *)this);

	if (wl == NULL) {
		Status = EFI_DEVICE_ERROR;
		goto Done;
	}

	if (bcmp((char *)SelectorGuid, (char *)&gApple80211IoctlScanGuid, sizeof(EFI_GUID)) == 0) {
		if (InSize != sizeof(APPLE_80211_SCANPARAMS) ||
		    InParams == NULL) {
			Status = EFI_INVALID_PARAMETER;
			goto Done;
		}
		Status = BCMWL_ScanIoctl(wl, (APPLE_80211_SCANPARAMS *)InParams);
		if (EFI_ERROR(Status)) {
			EFI_ERRMSG(Status, "BCMWL_80211Ioctl SCAN failed\r\n");
			goto Done;
		}
	}

	if (bcmp((char *)SelectorGuid, (char *)&gApple80211IoctlScanResultGuid,
	         sizeof(EFI_GUID)) == 0) {
		APPLE_80211_SR_REQ *SrReq = ((APPLE_80211_SR_REQ *)InParams);
		if (InParams == NULL ||
		    InSize < sizeof(APPLE_80211_SR_REQ) ||
		    OutParams == NULL) {
			Status = EFI_INVALID_PARAMETER;
			goto Done;
		}
		Status = BCMWL_ScanResultIoctl(wl, SrReq, (APPLE_80211_SCANRESULT *)OutParams,
		                               OutSize);
		if (EFI_ERROR(Status)) {
			*OutSize = 0;
			if (Status != EFI_NOT_READY)
				EFI_ERRMSG(Status, "BCMWL_80211Ioctl SCAN RESULTS failed\r\n");
			goto Done;
		}
	}

	if (bcmp((char *)SelectorGuid, (char *)&gApple80211IoctlJoinGuid, sizeof(EFI_GUID)) == 0) {
		if (InSize < sizeof(APPLE_80211_JOINPARAMS) ||
		    InParams == NULL ||
		    OutParams != NULL) {
			Status = EFI_INVALID_PARAMETER;
			goto Done;
		}
		Status = BCMWL_JoinIoctl(wl, (APPLE_80211_JOINPARAMS *)InParams);
		if (EFI_ERROR(Status)) {
			EFI_ERRMSG(Status, "BCMWL_80211Ioctl JOIN failed\r\n");
			goto Done;
		}
	}

	if (bcmp((char *)SelectorGuid, (char *)&gApple80211IoctlStateGuid, sizeof(EFI_GUID)) == 0) {
		UINT32 *state = ((UINT32 *)OutParams);
		if (OutParams == NULL ||
		    InSize != 0 ||
		    InParams != NULL) {
			Status = EFI_INVALID_PARAMETER;
			goto Done;
		}
		if (WLC_CONNECTED(wl->wlc))
			wl->apple80211state = APPLE80211_S_RUN;
		*state = wl->apple80211state;
		return EFI_SUCCESS;
	}

	if (bcmp((char *)SelectorGuid, (char *)&gApple80211IoctlInfoGuid, sizeof(EFI_GUID)) == 0) {
		if (OutParams == NULL ||
		    InSize != 0 ||
		    InParams != NULL) {
			Status = EFI_INVALID_PARAMETER;
			goto Done;
		}

		Status = BCMWL_InfoIoctl(wl, (APPLE_80211_INFO *)OutParams, OutSize);
		if (EFI_ERROR(Status)) {
			*OutSize = 0;
			EFI_ERRMSG(Status, "BCMWL_80211Ioctl INFO failed\r\n");
			goto Done;
		}
	}

	if (bcmp((char *)SelectorGuid, (char *)&gApple80211IoctlIBSSGuid, sizeof(EFI_GUID)) == 0) {
		if (InSize < sizeof(APPLE_80211_IBSSPARAMS) ||
		    InParams == NULL ||
		    OutParams != NULL) {
			Status = EFI_INVALID_PARAMETER;
			goto Done;
		}
		Status = BCMWL_IBSSIoctl(wl, (APPLE_80211_IBSSPARAMS *)InParams);
		if (EFI_ERROR(Status)) {
			EFI_ERRMSG(Status, "BCMWL_80211Ioctl IBSS failed\r\n");
			goto Done;
		}
	}

	/* Last ulong is wlioctl specific cmd. Match the rest */
	if (bcmp((char *)SelectorGuid, (char *)&gBcmwlIoctlGuid,
	         sizeof(EFI_GUID) - sizeof(UINT32)) == 0) {
		Status = BCMWL_wlioctl(wl, *((uint32 *)&SelectorGuid->Data4[4]),
		                       InParams, (uint)InSize);
		if (EFI_ERROR(Status)) {
			EFI_ERRMSG(Status, "BCMWL_80211Ioctl failed\r\n");
			goto Done;
		}
	}

Done:
	return Status;
}

EFI_STATUS
GetLinkState(IN wl_info_t  *wl)
{
	bool connected = FALSE;
	EFI_STATUS Status = EFI_NO_MEDIA;

	if (wl->pub->up)
		connected = WLC_CONNECTED(wl->wlc);

	if (!connected) {
		Status = EFI_NO_MEDIA;
	} else {
		if (wl->wifi_bounced == TRUE) {
			Status = EFI_NOT_READY;
			wl->wifi_bounced = FALSE;
		} else {
			Status = EFI_SUCCESS;
		}
	}

	return Status;
}

STATIC EFI_STATUS
EFIAPI
BCMWL_GetLinkState(IN VOID *This)
{
	wl_info_t  *wl;
	EFI_STATUS Status = EFI_NO_MEDIA;

	if (This == NULL) {
		Status = EFI_INVALID_PARAMETER;
	}

	if (!EFI_ERROR(Status)) {
		wl = EFI_WLINFO_APPLE_LINK_STATE_FROM_THIS((APPLE_LINK_STATE_PROTOCOL *)This);

		if (wl == NULL) {
			Status = EFI_DEVICE_ERROR;

		}
	}

	if (!EFI_ERROR(Status)) {
		Status = GetLinkState(wl);
	}

	return (Status);
}

#ifdef APPLE_BUILD
STATIC EFI_STATUS
EFIAPI
BCMWL_GetAdaptorInformation(
			    IN VOID *This,
			    IN EFI_GUID *InformationType,
			    OUT VOID **InformationBlock,
			    OUT UINTN *InformationBlockSize)
{
	wl_info_t *wl;
	EFI_STATUS Status = EFI_SUCCESS;

	if (This == NULL) {
		Status = EFI_INVALID_PARAMETER;
	}

	if (!EFI_ERROR(Status)) {
		wl = EFI_WLINFO_ADAPTOR_INFO_FROM_THIS(This);

		if (wl == NULL) {
			Status = EFI_DEVICE_ERROR;
		}
	}

	if (!EFI_ERROR(Status)) {
		Status = GetLinkState(wl);
	}

	return Status;
}
#endif /* APPLE_BUILD */

STATIC VOID
EFIAPI
BCMWL_ExitBootService(EFI_EVENT event, VOID *context)
{
	wl_info_t  *wl = (wl_info_t *)context;

	if (wl == NULL) {
		return;
	}

	/* Stop the driver */
	if (wl->wlc && wl->pub->up) {
		wl_down(wl);
	}

	/*
	* Complete all outstanding transactions to Controller.
	* Don't allow any new transaction to Controller to be started.
	*/
	gWlDevice->PciIoIntf->Attributes(gWlDevice->PciIoIntf,
		EfiPciIoAttributeOperationDisable,
		EFI_PCI_IO_ATTRIBUTE_MEMORY |
		EFI_PCI_IO_ATTRIBUTE_BUS_MASTER |
		EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE,
		NULL);
}
