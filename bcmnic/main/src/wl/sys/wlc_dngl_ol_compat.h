/*
 * Copyright (c) 2013 Broadcom Corporation, All rights reserved.
 * $Id: wlc_dngl_ol_compat.h 419852 2013-08-23 00:45:30Z $
 */

#ifndef _wlc_dngl_ol_compat_h_
#define _wlc_dngl_ol_compat_h_

#include <wlc_types.h>
#include <proto/ethernet.h>
#include <wlc_keymgmt.h>

enum wlc_ol_bsscfg_flags {
	WLC_OL_BSSCFG_F_NONE			= 0x00000000,
	WLC_OL_BSSCFG_F_INITED			= 0x00000001,
	WLC_OL_BSSCFG_F_BSS				= 0x00000002,
	WLC_OL_BSSCFG_F_NATIVEIF		= 0x00000004,
	WLC_OL_BSSCFG_F_PSTA			= 0x00000008,
	WLC_OL_BSSCFG_F_UP				= 0x00000010,
	WLC_OL_BSSCFG_F_NOBCMC			= 0x00000020,
	WLC_OL_BSSCFG_F_TDLS			= 0x00000040,
	WLC_OL_BSSCFG_F_ASSOCIATED		= 0x00000080,
	WLC_OL_BSSCFG_F_AP				= 0x00000100,
	WLC_OL_BSSCFG_F_IBSS			= 0x00000200,
	WLC_OL_BSSCFG_F_WIN7PLUS		= 0x00000400,
	WLC_OL_BSSCFG_F_WAKEON1MICERR	= 0x00000800,
	WLC_OL_BSSCFG_F_ALL				= 0xffffffff
};
typedef enum wlc_ol_bsscfg_flags wlc_ol_bsscfg_flags_t;

struct wlc_pm_st {
	bool		PM;
	mbool		PMblocked;
	bool		WME_PM_blocked;
};

typedef struct wlc_pm_st wlc_pm_st_t;

struct wlc_bsscfg {
	uint8 idx;
	wlc_ol_bsscfg_flags_t flags;
	uint32 wsec;
	uint32 WPA_auth;
	bool wsec_restrict;
	struct ether_addr cur_etheraddr;
	struct ether_addr BSSID;
	wlc_pm_st_t *pm;
};

enum wlc_ol_scb_flags {
	WLC_OL_SCB_F_NONE			= 0x0,
	WLC_OL_SCB_F_INITED			= 0x00000001,
	WLC_OL_SCB_F_MFP			= 0x00000002,
	WLC_OL_SCB_F_CCX_MFP		= 0x00000004,
	WLC_OL_SCB_F_WDS			= 0x00000008,
	WLC_OL_SCB_F_WPA_SUP		= 0x00000010,
	WLC_OL_SCB_F_INTERNAL		= 0x00000020,
	WLC_OL_SCB_F_QOS			= 0x00000040,
	WLC_OL_SCB_F_LEGACY_AES		= 0x00000080,
	WLC_OL_SCB_F_ALL			= 0xffffffff
};
typedef enum wlc_ol_scb_flags wlc_ol_scb_flags_t;

struct scb {
	uint8	idx;
	struct ether_addr ea;
	wlc_ol_scb_flags_t flags;
	wlc_bsscfg_t *bsscfg;
	uint32 WPA_auth;

	/* Fragment state information */
	uint16   seqctl[NUMPRIO];
	void	*fragbuf[NUMPRIO];	/* defragmentation buffer per prio */
	uint	fragresid[NUMPRIO];	/* #bytes unused in frag buffer per prio */
	uint32	fragtimestamp[NUMPRIO];
};

/* convenience macros */

#define WLC_BSSCFG_IDX(_b) (_b)->idx
#define BSSCFG_AP(_b) (((_b)->flags & WLC_OL_BSSCFG_F_AP) != 0)
#define BSSCFG_STA(_b) (!BSSCFG_AP(_b))
#define BSSCFG_PSTA(_b) (((_b)->flags & WLC_OL_BSSCFG_F_PSTA) != 0)
#define BSS_TDLS_ENAB(_wlc, _b) (((_b)->flags & WLC_OL_BSSCFG_F_TDLS) != 0)
#define BSSCFG_IBSS(_b) (((_b)->flags & WLC_OL_BSSCFG_F_BSS) == 0)
#define BSSCFG_BSS(_b) (((_b)->flags & WLC_OL_BSSCFG_F_BSS) != 0)

#define SCB_BSSCFG(_s) (_s)->bsscfg
#define SCB_INTERNAL(_s) (((_s)->flags & WLC_OL_SCB_F_INTERNAL) != 0)
#define SCB_QOS(_s) (((_s)->flags & WLC_OL_SCB_F_QOS) != 0)
#define SCB_MFP(_s) (((_s)->flags & WLC_OL_SCB_F_MFP) != 0)

struct wlc_frminfo {
	struct dot11_header *h;     /* pointer to d11 header */
	uchar *pbody;               /* pointer to body of frame */
	uint body_len;              /* length of body after d11 hdr */
	uint len;                   /* length of first pkt in chain */
	uint totlen;				/* total length of the pkt */
	void *p;                    /* pointer to pkt */
	uint8 prio;                 /* frame priority */
	bool ismulti;               /* TRUE for multicast frame */
	int rx_wep;
	wlc_key_t *key;             /* key */
	wlc_key_info_t key_info;    /* key info */
	uint16 fc;                  /* frame control field */
	struct ether_header *eh;    /* pointer to ether header */
	struct ether_addr *sa;      /* pointer to source address */
	struct ether_addr *da;      /* pointer to dest address */
	struct d11rxhdr *rxh;		/* rxhdr */
	uint16	seq;				/* 802.11 seq */
};

#endif /* _wlc_dngl_ol_compat_h_ */
