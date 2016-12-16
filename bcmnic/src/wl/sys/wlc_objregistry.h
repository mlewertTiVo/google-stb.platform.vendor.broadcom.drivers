/*
 * WLC Object Registry API definition
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wlc_objregistry.h 665171 2016-10-15 15:40:29Z $
 */

/**
 * @file
 * @brief
 * Chip/Feature specific enable/disable need to be done for Object registry.
 * Moved object registry related functions from wl/sys to utils folder.
 * A wrapper is provided in this layer/file to enab/disab obj registry for each key
 */


#ifndef _wlc_objregistry_h_
#define _wlc_objregistry_h_

typedef enum obj_registry_key {
	OBJR_SELF,		/* Must be @ 0 */
	OBJR_MODULE_ID,		/* Stores wlc->modulecb */
	OBJR_BSSCFG_PTR,
	OBJR_CLM_PTR,
	OBJR_AMPDUTX_CONFIG,
	OBJR_AMPDURX_CONFIG,
	OBJR_WLC_CMN_INFO,	/* wlc shared structure */
	OBJR_WLC_PUB_CMN_INFO,	/* wlc pub_cmn_t structure */
	OBJR_PHY_CMN_SROM_INFO, /* sharing phycmn srom structure */
	OBJR_PHY_CMN_INFO,	/* phy shared structure	*/
	OBJR_PHY_CMN_RADAR_INFO, /* phy shared radar structure */
	OBJR_ACPHY_SROM_INFO,	/* sharing acphy srom structure	*/
	OBJR_WLC_BANDSTATE,	/* wlc->bandstate[] */
	OBJR_SCANPUBLIC_CMN, /* Common info for scan public */
	OBJR_SCANPRIV_CMN, /* Common info for scan private */
	OBJR_SCANUTILS_CMN, /* Common info for scan utils */
	OBJR_DEFAULT_BSS, /* Shared default BSS  */
	OBJR_P2P_DATA_PTR,	/* shared p2p data */
	OBJR_ASSOCIATION_INFO, /* Shared association info. */
	OBJR_BSSCFG_CUBBY,
	OBJR_SCB_CUBBY,
	OBJR_NAN_CMN_INFO, /* nan module cmn info */
	OBJR_NAN_MAC_INFO,   /* nan mac info */
	OBJR_NAN_DISC_INFO,  /* nan disc info */
	OBJR_RSDB_CMN_INFO, /* RSDB common info */
	OBJR_BWTE_INFO,     /* BT tunnel engine info */
	OBJR_TBOW_INFO,     /* tbow info */
	OBJR_NAN_SCHED_INFO, /* nan sched info */
	OBJR_ASDB_INFO, /* ASDB info */
	OBJR_ANQPO_CMN, /* ANQPO shared info */
	OBJR_WL_CMN_INFO,	/* wl common info */
	OBJR_LTECX_CMN_INFO,	/* ltecx common info */
	OBJR_RSDB_POLICY_CMN_INFO, /* policymgr cmn info */
	OBJR_PFN_CMN_INFO,  /* shared PFN info */
	OBJR_11D_CMN_INFO,	/* 11d commn info */
	OBJR_11H_CMN_INFO,	/* 11h commn info */
	OBJR_TDLS_CMN_INFO, /* TDLS common info */
	OBJR_SCB_HANDLE_MAP, /* scb handle info */
	OBJR_NAN_DP_INFO, /* nan data path info */
	OBJR_NAN_NDP_INFO, /* nan ndp info */
	OBJR_NAN_NDL_INFO, /* nan ndl info */
	OBJR_NATOE_CMN_INFO, /* NATOE common info */
	OBJR_MSCH_PROFILER_INFO, /* msch profiler info */
	OBJR_AWDL_CMN_INFO,  /* awdl cmn info */
	OBJR_AWDL_TMR_INFO,  /* awdl timer info */
	OBJR_MBO_CMN_DATA,   /* shared mbo data */
	OBJR_NAN_FSM_REG, /* nan fsm registry */
	OBJR_SCANDB_CMN, /* common info for scandb */
	OBJR_NAN_AVAIL_INFO,  /* nan avail info */
	OBJR_OCE_CMN_INFO, /* oce common info */
	OBJR_CAC_CMN_INFO, /* CAC common info */
	OBJR_FTM_CMN_INFO,   /* ftm cmmon info */
	OBJR_PKT_FILTER,	/* packet filter */
	OBJR_KEEPALIVE_CMN_INFO, /* keep_alive_cmn info */
	OBJR_NAN_RANGE_INFO,
	OBJR_NAN_DAM_INFO,	/* nan dam info */
	OBJR_SLOTTED_BSS_CMN_INFO, /* slotted bss cmn info */
	OBJR_TEST_CMN_INFO,	/* test common info */
	OBJR_BTC_PROFILE_INFO,	/* BTC profile info */
	OBJR_EVENT_CMN_INFO,
	OBJR_MBO_OCE_INFO, /* mbo-oce info */
	OBJR_TKO_CMN,           /* TKO common data */
	OBJR_IFACE_CREATE_INFO,	/* Interface create info */
	OBJR_MAX_KEYS
} obj_registry_key_t;


typedef struct wlc_obj_registry wlc_obj_registry_t;
#ifdef WL_OBJ_REGISTRY

/* === Object Registry API === */

/* Create the registry only once in wl_xxx.c per port layer and pass it to wlc_attach()
 * Each wlc_attach() creates a new WLC instance that shares the same objr instance
 */
wlc_obj_registry_t* obj_registry_alloc(osl_t *osh, int count);

/* Destroy the registry at the end, after all instances of WLC are freed-up */
void obj_registry_free(wlc_obj_registry_t *wlc_objr, osl_t *osh);

/* obj_registry_set() is used to setup the value for key.
 * It simply overwrites the existing value if any
 * returns, BCME_OK on success
 * returns, BCME_RANGE if key exceeds, max limit
 */
int obj_registry_set(wlc_obj_registry_t *wlc_objr, obj_registry_key_t key, void *value);

/* obj_registry_get() is used to get the value for key.
 * return of NULL, indicates key is unused / invalid
 *
 */
void * obj_registry_get(wlc_obj_registry_t *wlc_objr, obj_registry_key_t key);

/* Ref counting on registry objects is provided for users to keep track of ref counts */

/*
 * Typical call sequence will be as follows:
 * 	Step (1). check if the registry has a value for key 'KEY_X'
 * 	Step (2). if it has value, go to Step (4)
 * 	Step (3). registry has no value, so allocate and store value for 'KEY_X'
 *	Step (4). reference the stored value for 'KEY_X'
 */

/* obj_registy_ref() is used to increment ref_cnt associated with 'key'
 * If there is no value stored, reference is not incremented.
 */
int obj_registry_ref(wlc_obj_registry_t *objr, obj_registry_key_t key);

/* obj_registry_unref() is used to decrement ref_cnt associated with 'key'
 * Decrements the reference count for each call.
 * If there is no value stored, reference is not decremented.
 */
int obj_registry_unref(wlc_obj_registry_t *objr, obj_registry_key_t key);

int obj_registry_islast(wlc_obj_registry_t *objr);

void obj_registry_disable(wlc_obj_registry_t *wlc_objr, obj_registry_key_t key);

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
/* Debug function to dump out all contents of the registry */
int wlc_dump_objr(wlc_obj_registry_t *objr, struct bcmstrbuf *b);
#endif

#else /* WL_OBJ_REGISTRY */

#define obj_registry_alloc(o, c) (NULL)
#define obj_registry_free(d, o)
#define obj_registry_set(d, k, v)
#define obj_registry_get(d, k) (NULL)
#define obj_registry_ref(d, k) (1)
#define obj_registry_unref(d, k) (0)
#define obj_registry_islast(d) (1)
#define obj_registry_disable(d, k)
#endif /* WL_OBJ_REGISTRY */

#endif /* _wlc_objregistry_h_ */
