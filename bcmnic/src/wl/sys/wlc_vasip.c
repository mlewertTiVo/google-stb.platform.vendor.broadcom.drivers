/*
 * VASIP related functions
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
 * $Id: wlc_vasip.c 672288 2016-11-25 11:46:09Z $
 */

#include <wlc_cfg.h>

#if defined(WLVASIP) || defined(SAVERESTORE)
#include <typedefs.h>
#include <wlc_types.h>
#include <siutils.h>
#include <wlioctl.h>
#include <wlc_pub.h>
#include <wlioctl.h>
#include <wlc.h>
#include <wlc_dbg.h>
#include <phy_vasip_api.h>
#include <phy_prephy_api.h>
#include <wlc_hw_priv.h>
#include <d11vasip_code.h>
#include <pcicfg.h>
#include <wl_export.h>
#include <wlc_vasip.h>
#include <wlc_dump.h>

#define VASIP_COUNTERS_LMT		256
#define VASIP_DEFINED_COUNTER_NUM	26
#define SVMP_MEM_OFFSET_MAX		0x28000
#define SVMP_MEM_DUMP_LEN_MAX		4096

#define SVMP_ACCESS_VIA_PHYTBL
#define SVMP_ACCESS_BLOCK_SIZE 16

/* Module private states */
struct wlc_vasip_info {
	wlc_info_t *wlc;
};

static void wlc_vasip_clock(wlc_hw_info_t *wlc_hw, int enb);

/* local prototypes */
#if (defined(BCMDBG) || defined(BCMDBG_DUMP))
/* dump vasip counters from vasip program memory */
static int wlc_dump_vasip_counters(wlc_info_t *wlc, struct bcmstrbuf *b);

/* dump vasip status data from vasip program memory */
static int wlc_dump_vasip_status(wlc_info_t *wlc, struct bcmstrbuf *b);

/* clear vasip counters */
static int wlc_vasip_counters_clear(wlc_hw_info_t *wlc);

/* copy svmp memory to a buffer starting from offset of length 'len', len is count of uint16's */
static int
wlc_svmp_mem_read(wlc_hw_info_t *wlc_hw, uint16 *ret_svmp_addr, uint32 offset, uint16 len);

/* set svmp memory with a value from offset of length 'len', len is count of uint16's */
static int wlc_svmp_mem_set(wlc_hw_info_t *wlc_hw, uint32 offset, uint16 len, uint16 val);
#endif /* (BCMDBG || BCMDBG_DUMP) */

static uint32 *wlc_vasip_addr(wlc_hw_info_t *wlc_hw, uint32 offset);

static void wlc_vasip_download(wlc_hw_info_t *wlc_hw, const uint32 *vasip_code,
	const uint nbytes, uint32 offset, uint32 vasipver, bool nopi);

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void wlc_vasip_verify(wlc_hw_info_t *wlc_hw, const uint32 vasip_code[],
	const uint nbytes, uint32 offset);
#endif

/* read/write svmp memory */
int wlc_svmp_read_blk(wlc_hw_info_t *wlc_hw, uint16 *val, uint32 offset, uint16 len);
int wlc_svmp_write_blk(wlc_hw_info_t *wlc_hw, uint16 *val, uint32 offset, uint16 len);

/* iovar table */
enum {
	IOV_VASIP_COUNTERS_CLEAR,
	IOV_SVMP_MEM,
	IOV_MU_RATE,
	IOV_MU_GROUP,
	IOV_MU_MCS_RECMD,
	IOV_MU_MCS_CAPPING,
	IOV_MU_SGI_RECMD,
	IOV_MU_SGI_RECMD_TH,
	IOV_MU_GROUP_DELAY,
	IOV_MU_PRECODER_DELAY,
	IOV_SVMP_DOWNLOAD,
	IOV_VASIP_LAST
};

static int
wlc_vasip_doiovar(void *context, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint vsize, struct wlc_if *wlcif);

static const bcm_iovar_t vasip_iovars[] = {
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	{"vasip_counters_clear", IOV_VASIP_COUNTERS_CLEAR,
	(0), 0, IOVT_VOID, 0
	},
	{"svmp_mem", IOV_SVMP_MEM,
	(0), 0, IOVT_BUFFER, sizeof(svmp_mem_t),
	},
	{"mu_rate", IOV_MU_RATE,
	(0), 0, IOVT_BUFFER, 0,
	},
	{"mu_group", IOV_MU_GROUP,
	(0), 0, IOVT_BUFFER, 0,
	},
	{"mu_mcs_recmd", IOV_MU_MCS_RECMD,
	(0), 0, IOVT_UINT16, 0
	},
	{"mu_mcs_capping", IOV_MU_MCS_CAPPING,
	(0), 0, IOVT_UINT16, 0
	},
	{"mu_sgi_recmd", IOV_MU_SGI_RECMD,
	(0), 0, IOVT_INT16, 0
	},
	{"mu_sgi_recmd_th", IOV_MU_SGI_RECMD_TH,
	(0), 0, IOVT_UINT16, 0
	},
	{"mu_group_delay", IOV_MU_GROUP_DELAY,
	(0), 0, IOVT_UINT16, 0
	},
	{"mu_precoder_delay", IOV_MU_PRECODER_DELAY,
	(0), 0, IOVT_UINT16, 0
	},
	{"svmp_download", IOV_SVMP_DOWNLOAD,
	(0), 0, IOVT_VOID, 0
	},
#endif /* BCMDBG || BCMDBG_DUMP */
	{NULL, 0, 0, 0, 0, 0}
};

void
BCMATTACHFN(wlc_vasip_detach)(wlc_vasip_info_t *vasip)
{
	wlc_info_t *wlc;

	if (!vasip)
		return;

	wlc = vasip->wlc;
	wlc_module_unregister(wlc->pub, "vasip", vasip);
	MFREE(wlc->osh, vasip, sizeof(*vasip));
}

wlc_vasip_info_t *
BCMATTACHFN(wlc_vasip_attach)(wlc_info_t *wlc)
{
	wlc_pub_t *pub = wlc->pub;
	wlc_vasip_info_t *vasip;

	if ((vasip = MALLOCZ(wlc->osh, sizeof(wlc_vasip_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: vasip memory alloc. failed\n",
			pub->unit, __FUNCTION__));
		return NULL;
	}

	vasip->wlc = wlc;

	if ((wlc_module_register(pub, vasip_iovars, "vasip",
		vasip, wlc_vasip_doiovar, NULL, NULL, NULL)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
			pub->unit, __FUNCTION__));
		pub->_vasip = FALSE;
		MFREE(wlc->osh, vasip, sizeof(*vasip));
		return NULL;
	}

#if (defined(BCMDBG) || defined(BCMDBG_DUMP))
	wlc_dump_register(pub, "vasip_counters", (dump_fn_t)wlc_dump_vasip_counters, (void *)wlc);
	wlc_dump_register(pub, "vasip_status", (dump_fn_t)wlc_dump_vasip_status, (void *)wlc);
#endif /* (BCMDBG || BCMDBG_DUMP) */

	return vasip;
}

static int
wlc_vasip_doiovar(void *context, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint vsize, struct wlc_if *wlcif)
{
	wlc_vasip_info_t *vasip = (wlc_vasip_info_t*)context;
	wlc_info_t *wlc = vasip->wlc;
	int err = BCME_OK;

	BCM_REFERENCE(wlc);

	switch (actionid) {
#if (defined(BCMDBG) || defined(BCMDBG_DUMP))
		case IOV_SVAL(IOV_VASIP_COUNTERS_CLEAR):
			err = wlc_vasip_counters_clear(wlc->hw);
			break;

		case IOV_GVAL(IOV_SVMP_MEM): {
			svmp_mem_t *mem = (svmp_mem_t *)params;
			uint32 mem_addr;
			uint16 mem_len;

			mem_addr = mem->addr;
			mem_len = mem->len;

			if (len < mem_len) {
				err = BCME_BUFTOOSHORT;
				break;
			}

			if (mem_addr & 1) {
				err = BCME_BADADDR;
				break;
			}

			err = wlc_svmp_mem_read(wlc->hw, (uint16 *)arg, mem_addr, mem_len);
			break;
		}

		case IOV_SVAL(IOV_SVMP_MEM): {
			svmp_mem_t *mem = (svmp_mem_t *)params;
			uint32 mem_addr;
			uint16 mem_len;

			mem_addr = mem->addr;
			mem_len = mem->len;

			if (mem_addr & 1) {
				err = BCME_BADADDR;
				break;
			}

			err = wlc_svmp_mem_set(wlc->hw, mem_addr, mem_len, mem->val);
			break;
		}
		case IOV_SVAL(IOV_SVMP_DOWNLOAD):
		{
			/* download vasip codes - wlc_vasip_download() */
			break;
		}

	case IOV_GVAL(IOV_MU_RATE): {
		mu_rate_t *mu = (mu_rate_t *)arg;
		uint32 mem_addr;
		uint16 mem_len;
		bool fix_rate;
		uint16 grouping_forced, grouping_forced_mcs;

		if (len < sizeof(mu_rate_t)) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		mem_addr = VASIP_OVERWRITE_MCS_FLAG_ADDR_OFFSET;
		mem_len = 1;
		err = wlc_svmp_mem_read(wlc->hw, (uint16 *) &fix_rate, mem_addr, mem_len);

		mem_addr = VASIP_GROUP_FORCED_ADDR_OFFSET;
		mem_len = 1;
		err = wlc_svmp_mem_read(wlc->hw,
			(uint16 *) &grouping_forced, mem_addr, mem_len);

		mem_addr = VASIP_GROUP_FORCED_MCS_ADDR_OFFSET;
		mem_len = 1;
		err = wlc_svmp_mem_read(wlc->hw,
			(uint16 *) &grouping_forced_mcs, mem_addr, mem_len);

		fix_rate |= ((grouping_forced > 0) && (grouping_forced_mcs == 1));

		if (fix_rate == 0) {
			mu->auto_rate = 1;
		} else {
			mu->auto_rate = 0;
		}

		mem_len = 4;
		mem_addr = VASIP_STEERING_MCS_BUF_ADDR_OFFSET;
		err = wlc_svmp_mem_read(wlc->hw, mu->rate_user, mem_addr, mem_len);

		break;
	}

	case IOV_SVAL(IOV_MU_RATE): {
		mu_rate_t *mu = (mu_rate_t *)params;
		uint16 forced_group, group_method;
		uint32 mem_addr;
		uint16 mem_len;
		bool   fix_rate;

		/* get forced_group */
		mem_addr = VASIP_GROUP_FORCED_ADDR_OFFSET;
		mem_len = 1;
		err = wlc_svmp_mem_read(wlc->hw, &forced_group, mem_addr, mem_len);
		/* get group_method */
		mem_addr = VASIP_GROUP_METHOD_ADDR_OFFSET;
		mem_len = 1;
		err = wlc_svmp_mem_read(wlc->hw, &group_method, mem_addr, mem_len);
		/* only allow fix mu_rate when */
		/* either "forced only 1 group" or "auto-groupping with old method (M=0)" */
		fix_rate = (!mu->auto_rate) &&
			((forced_group == WL_MU_GROUP_FORCED_1GROUP) ||
				((forced_group == WL_MU_GROUP_MODE_AUTO) &&
				(group_method == WL_MU_GROUP_METHOD_OLD)));
		mem_addr = VASIP_OVERWRITE_MCS_FLAG_ADDR_OFFSET;
		mem_len = 1;
		if ((err = wlc_svmp_mem_set(wlc->hw,
				mem_addr, mem_len, fix_rate)) != BCME_OK) {
			break;
		}
		if (fix_rate) {
			mem_addr = VASIP_OVERWRITE_MCS_BUF_ADDR_OFFSET;
			mem_len = 4;
			err = wlc_svmp_mem_blk_set(wlc->hw, mem_addr, mem_len, mu->rate_user);
		}
		/* barf if set mu_rate but blocked by mu_group */
		if (mu->auto_rate == fix_rate) {
			err = BCME_USAGE_ERROR;
		}
		break;

	}

	case IOV_GVAL(IOV_MU_GROUP): {
		mu_group_t *mugrp = (mu_group_t *)arg;
		uint16 forced_group;
		uint32 mem_addr;
		uint16 mem_len;
		uint16 v2m_grp[14];
		uint16 m, n;

		/* set WL_MU_GROUP_PARAMS_VERSION */
		mugrp->version = WL_MU_GROUP_PARAMS_VERSION;

		/* check if forced */
		mem_addr = VASIP_GROUP_FORCED_ADDR_OFFSET;
		mem_len = 1;
		err = wlc_svmp_mem_read(wlc->hw, &forced_group, mem_addr, mem_len);

		if (forced_group > 0) {
			mugrp->forced = 1;
			mugrp->forced_group_num = forced_group;
			/* get forced_group_mcs */
			mem_addr = VASIP_GROUP_FORCED_MCS_ADDR_OFFSET;
			mem_len = 1;
			err = wlc_svmp_mem_read(wlc->hw,
				(uint16*)(&(mugrp->forced_group_mcs)), mem_addr, mem_len);
			/* get forced_group_option */
			mem_addr = VASIP_GROUP_FORCED_OPTION_ADDR_OFFSET;
			mem_len = forced_group*WL_MU_GROUP_NUSER_MAX;
			err = wlc_svmp_mem_read(wlc->hw,
				(uint16*)(&(mugrp->group_option[0][0])), mem_addr, mem_len);
		} else {
			mugrp->forced = 0;
			mugrp->forced_group_num = 0;
			/* get group_method */
			mem_addr = VASIP_GROUP_METHOD_ADDR_OFFSET;
			mem_len = 1;
			err = wlc_svmp_mem_read(wlc->hw,
				(uint16*)&(mugrp->group_method), mem_addr, mem_len);
			/* get group_number */
			mem_addr = VASIP_GROUP_NUMBER_ADDR_OFFSET;
			mem_len = 1;
			err = wlc_svmp_mem_read(wlc->hw,
				(uint16*)(&(mugrp->group_number)), mem_addr, mem_len);
			/* method name */
			if (mugrp->group_method == 0) {
				snprintf(mugrp->group_method_name,
					sizeof(mugrp->group_method_name),
					"1 group for all admitted users");
			} else if (mugrp->group_method == 1) {
				snprintf(mugrp->group_method_name,
					sizeof(mugrp->group_method_name),
					"N best THPT groups and equally distributed across all BW");
			} else if (mugrp->group_method == 2) {
				snprintf(mugrp->group_method_name,
					sizeof(mugrp->group_method_name),
					"greedy non-disjoint grouping");
			} else if (mugrp->group_method == 3) {
				snprintf(mugrp->group_method_name,
					sizeof(mugrp->group_method_name),
					"disjoint grouping");
			} else {
				snprintf(mugrp->group_method_name,
					sizeof(mugrp->group_method_name),
					"not support yet");
			}
			/* read out v2m_buf_grp[] to get latest recommend grouping */
			mem_addr = VASIP_V2M_GROUP_OFFSET;
			mem_len = 2;
			err = wlc_svmp_mem_read(wlc->hw, v2m_grp, mem_addr, mem_len);
			n = (v2m_grp[0] - 2) / 28;
			if ((n > mugrp->group_number) ||
					((v2m_grp[0] != 0) && (v2m_grp[0] != (n * 28 + 2)))) {
				mugrp->auto_group_num = 0;
				WL_ERROR(("%s: unexpected auto_group_num=%d "
					"or v2m_len=%d (should be 2+28*N)\n",
					__FUNCTION__, n, v2m_grp[0]));
			} else {
				mugrp->auto_group_num = n;
				mem_addr = VASIP_V2M_GROUP_OFFSET + 2;
				mem_len = 14;
				for (m = 0; m < mugrp->auto_group_num; m++) {
					err = wlc_svmp_mem_read(wlc->hw, v2m_grp,
							mem_addr, mem_len);
					mugrp->group_GID[m] = v2m_grp[1] & 0x003f;
					for (n = 0; n < 4; n++) {
						mugrp->group_option[m][n] =
							((v2m_grp[2+3*n] & 0x00ff) << 8) +
								(v2m_grp[3+3*n] & 0x00ff);
					}
					mem_addr += mem_len;
				}
			}
		}
		break;
	}

	case IOV_SVAL(IOV_MU_GROUP): {
		mu_group_t *mugrp = (mu_group_t *)arg;
		uint16 forced_group;
		uint32 mem_addr;
		uint16 mem_len;

		/* check WL_MU_GROUP_PARAMS_VERSION */
		if (mugrp->version != WL_MU_GROUP_PARAMS_VERSION) {
			err = BCME_BADARG;
			break;
		}

		/* forced grouping */
		if (mugrp->forced == WL_MU_GROUP_MODE_FORCED) {
			/* set forced_group with forced_group_num */
			mem_addr = VASIP_GROUP_FORCED_ADDR_OFFSET;
			mem_len = 1;
			if ((err = wlc_svmp_mem_set(wlc->hw, mem_addr, mem_len,
					mugrp->forced_group_num)) != BCME_OK) {
				break;
			}
			/* set forced_group mcs */
			mem_addr = VASIP_GROUP_FORCED_MCS_ADDR_OFFSET;
			mem_len = 1;
			if ((err = wlc_svmp_mem_set(wlc->hw, mem_addr, mem_len,
					mugrp->forced_group_mcs)) != BCME_OK) {
				break;
			}
			/* store forced grouping options into SVMP */
			mem_addr = VASIP_GROUP_FORCED_OPTION_ADDR_OFFSET;
			mem_len = mugrp->forced_group_num*WL_MU_GROUP_NUSER_MAX;
			if ((err = wlc_svmp_write_blk(wlc->hw,
					(uint16*)(&(mugrp->group_option[0][0])),
					mem_addr, mem_len)) != BCME_OK) {
				break;
			}
			/* set fix_rate=0 for forced_group==1 && forced_group_num!=1 */
			if (mugrp->forced_group_num != 1) {
				mem_addr = VASIP_OVERWRITE_MCS_FLAG_ADDR_OFFSET;
				mem_len = 1;
				if ((err = wlc_svmp_mem_set(wlc->hw,
						mem_addr, mem_len, 0)) != BCME_OK) {
					break;
				}
			}
		} else if (mugrp->forced == WL_MU_GROUP_MODE_AUTO) {
			/* disable forced_group */
			mem_addr = VASIP_GROUP_FORCED_ADDR_OFFSET;
			mem_len = 1;
			if ((err = wlc_svmp_mem_set(wlc->hw,
					mem_addr, mem_len, 0)) != BCME_OK) {
				break;
			}
			/* clean first forced grouping option in SVMP */
			mem_addr = VASIP_GROUP_FORCED_OPTION_ADDR_OFFSET;
			mem_len = WL_MU_GROUP_NGROUP_MAX*WL_MU_GROUP_NUSER_MAX;
			if ((err = wlc_svmp_mem_set(wlc->hw,
					mem_addr, mem_len, 0xffff)) != BCME_OK) {
				break;
			}
		} /* mugrp->forced can be WL_MU_GROUP_ENTRY_EMPTY for no '-g' option */

		/* auto grouping parameters only when not-forced-group */
		mem_addr = VASIP_GROUP_FORCED_ADDR_OFFSET;
		mem_len = 1;
		err = wlc_svmp_mem_read(wlc->hw, &forced_group, mem_addr, mem_len);
		if (forced_group == WL_MU_GROUP_MODE_AUTO) {
			if (mugrp->group_number != WL_MU_GROUP_ENTRY_EMPTY) {
				/* update group_number */
				mem_addr = VASIP_GROUP_NUMBER_ADDR_OFFSET;
				mem_len = 1;
				if ((err = wlc_svmp_mem_set(wlc->hw, mem_addr, mem_len,
						mugrp->group_number)) != BCME_OK) {
					break;
				}
			}
			if (mugrp->group_method != WL_MU_GROUP_ENTRY_EMPTY) {
				/* update group method */
				mem_addr = VASIP_GROUP_METHOD_ADDR_OFFSET;
				mem_len = 1;
				if ((err = wlc_svmp_mem_set(wlc->hw, mem_addr, mem_len,
						mugrp->group_method)) != BCME_OK) {
					break;
				}
				/* set group number */
				if (mugrp->group_method == WL_MU_GROUP_METHOD_OLD) {
					/*  old method: it should be 1 */
					mem_addr = VASIP_GROUP_NUMBER_ADDR_OFFSET;
					mem_len = 1;
					if ((err = wlc_svmp_mem_set(wlc->hw, mem_addr, mem_len,
							1)) != BCME_OK) {
						break;
					}
				} else if (mugrp->group_number == WL_MU_GROUP_ENTRY_EMPTY) {
					/* set if not specified group_number
					 *   method 1: set 1 if not specified group_number
					 *   method 2&3: don't-care (set to 4)
					 */
					mem_addr = VASIP_GROUP_NUMBER_ADDR_OFFSET;
					mem_len = 1;
					if (mugrp->group_method == 1) {
						mugrp->group_number = 1;
					} else if ((mugrp->group_method == 2) ||
							(mugrp->group_method == 3)) {
						mugrp->group_number = 4;
					}
					if ((err = wlc_svmp_mem_set(wlc->hw,
							mem_addr, mem_len,
							mugrp->group_number)) != BCME_OK) {
						break;
					}
				}
				/* set fix_rate=0 for forced_group==0 && old mathod */
				if (mugrp->group_method != WL_MU_GROUP_METHOD_OLD) {
					mem_addr = VASIP_OVERWRITE_MCS_FLAG_ADDR_OFFSET;
					mem_len = 1;
					if ((err = wlc_svmp_mem_set(wlc->hw,
							mem_addr, mem_len, 0)) != BCME_OK) {
						break;
					}
				}
			}
		}
		break;
	}

	/* VASIP FW knobs: unsigned int16 */
	case IOV_GVAL(IOV_MU_MCS_RECMD):
	case IOV_GVAL(IOV_MU_MCS_CAPPING):
	case IOV_GVAL(IOV_MU_SGI_RECMD_TH):
	case IOV_GVAL(IOV_MU_GROUP_DELAY):
	case IOV_GVAL(IOV_MU_PRECODER_DELAY): {
		uint16 value;
		uint32 tmp_val;
		uint16 mem_len = 1;
		uint32 mem_addr = VASIP_MCS_RECMND_MI_ENA_OFFSET;

		if (len < mem_len) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		if (IOV_ID(actionid) == IOV_MU_MCS_RECMD) {
			mem_addr = VASIP_MCS_RECMND_MI_ENA_OFFSET;
		} else if (IOV_ID(actionid) == IOV_MU_MCS_CAPPING) {
			mem_addr = VASIP_MCS_CAPPING_ENA_OFFSET;
		} else if (IOV_ID(actionid) == IOV_MU_SGI_RECMD_TH) {
			mem_addr = VASIP_SGI_RECMND_THRES_OFFSET;
		} else if (IOV_ID(actionid) == IOV_MU_GROUP_DELAY) {
			mem_addr = VASIP_DELAY_GROUP_TIME_OFFSET;
		} else if (IOV_ID(actionid) == IOV_MU_PRECODER_DELAY) {
			mem_addr = VASIP_DELAY_PRECODER_TIME_OFFSET;
		}

		err = wlc_svmp_mem_read(wlc->hw, &value, mem_addr, mem_len);
		tmp_val = (uint32)value;
		memcpy(arg, &tmp_val, sizeof(uint32));
		break;
	}

	case IOV_SVAL(IOV_MU_MCS_RECMD):
	case IOV_SVAL(IOV_MU_MCS_CAPPING):
	case IOV_SVAL(IOV_MU_SGI_RECMD_TH):
	case IOV_SVAL(IOV_MU_GROUP_DELAY):
	case IOV_SVAL(IOV_MU_PRECODER_DELAY): {
		uint16 value;
		uint16 mem_len = 1;
		uint32 mem_addr = VASIP_MCS_RECMND_MI_ENA_OFFSET;

		if (len < mem_len) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		memcpy(&value, params, sizeof(uint16));

		if (IOV_ID(actionid) == IOV_MU_MCS_RECMD) {
			mem_addr = VASIP_MCS_RECMND_MI_ENA_OFFSET;
			if ((value != 0) && (value != 1)) {
				return BCME_USAGE_ERROR;
			}
		} else if (IOV_ID(actionid) == IOV_MU_MCS_CAPPING) {
			mem_addr = VASIP_MCS_CAPPING_ENA_OFFSET;
			if ((value != 0) && (value != 1)) {
				return BCME_USAGE_ERROR;
			}
		} else if (IOV_ID(actionid) == IOV_MU_SGI_RECMD_TH) {
			mem_addr = VASIP_SGI_RECMND_THRES_OFFSET;
		} else if (IOV_ID(actionid) == IOV_MU_GROUP_DELAY) {
			mem_addr = VASIP_DELAY_GROUP_TIME_OFFSET;
		} else if (IOV_ID(actionid) == IOV_MU_PRECODER_DELAY) {
			mem_addr = VASIP_DELAY_PRECODER_TIME_OFFSET;
		}

		err = wlc_svmp_mem_set(wlc->hw, mem_addr, mem_len, value);
		break;
	}

	/* VASIP FW knobs: signed int16 */
	case IOV_GVAL(IOV_MU_SGI_RECMD): {
		int16 value;
		uint32 tmp_val;
		uint16 mem_len = 1;
		uint32 mem_addr = VASIP_SGI_RECMND_METHOD_OFFSET;

		if (len < mem_len) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		if (IOV_ID(actionid) == IOV_MU_SGI_RECMD) {
			mem_addr = VASIP_SGI_RECMND_METHOD_OFFSET;
		}

		err = wlc_svmp_mem_read(wlc->hw, (uint16*)&value, mem_addr, mem_len);
		tmp_val = (uint32)value;
		memcpy(arg, &tmp_val, sizeof(uint32));
		break;
	}
	case IOV_SVAL(IOV_MU_SGI_RECMD): {
		int16 value;
		uint16 mem_len = 1;
		uint32 mem_addr = VASIP_SGI_RECMND_METHOD_OFFSET;

		if (len < mem_len) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		memcpy(&value, params, sizeof(uint16));

		if (IOV_ID(actionid) == IOV_MU_SGI_RECMD) {
			mem_addr = VASIP_SGI_RECMND_METHOD_OFFSET;
			if ((value != 0) && (value != 1) && (value != -1) && (value != 2)) {
				return BCME_USAGE_ERROR;
			}
		}

		err = wlc_svmp_mem_set(wlc->hw, mem_addr, mem_len, (uint16)value);
		break;
	}
#endif /* (BCMDBG || BCMDBG_DUMP) */

		default :
			err = BCME_ERROR;
			break;
	}
	return err;
}

static uint32 *
wlc_vasip_addr(wlc_hw_info_t *wlc_hw, uint32 offset)
{
#if !defined(STB_SOC_WIFI)
	osl_t *osh = wlc_hw->osh;
	uchar *bar_va;
	uint32 bar_size;
#endif 
	uint32 *vasip_mem;
	int idx;
	uint32 vasipaddr;

	/* save current core */
	idx = si_coreidx(wlc_hw->sih);
	if (si_setcore(wlc_hw->sih, ACPHY_CORE_ID, 0) != NULL) {
		/* get the VASIP memory base */
#ifdef STB_SOC_WIFI
		vasipaddr = (uint32)(unsigned long)si_coreregs(wlc_hw->sih);
#else
		vasipaddr = si_addrspace(wlc_hw->sih, CORE_SLAVE_PORT_0, CORE_BASE_ADDR_0);
#endif /* STB_SOC_WIFI */
		/* restore core */
		(void)si_setcoreidx(wlc_hw->sih, idx);
	} else {
		WL_ERROR(("%s: wl%d: Failed to find ACPHY core \n",
			__FUNCTION__, wlc_hw->unit));
		ASSERT(0);
		return 0;
	}

#if !defined(STB_SOC_WIFI)
	bar_size = wl_pcie_bar2(wlc_hw->wlc->wl, &bar_va);

	if (bar_size) {
		OSL_PCI_WRITE_CONFIG(osh, PCI_BAR1_CONTROL, sizeof(uint32), vasipaddr);
	} else {
		bar_size = wl_pcie_bar1(wlc_hw->wlc->wl, &bar_va);
		OSL_PCI_WRITE_CONFIG(osh, PCI_BAR1_WIN, sizeof(uint32), vasipaddr);
	}

	BCM_REFERENCE(bar_size);
	ASSERT(bar_va != NULL && bar_size != 0);

	vasip_mem = (uint32 *)bar_va;
#else
	vasip_mem = (uint32 *)(unsigned long)vasipaddr;
#endif 

	ASSERT(vasip_mem);

	vasip_mem += (offset/sizeof(uint32));

	return vasip_mem;
}

/* write vasip code to vasip program memory */
void wlc_vasip_write(wlc_hw_info_t *wlc_hw, const uint32 vasip_code[],
	const uint nbytes, uint32 offset)
{
	uint32 *vasip_mem;
	int i, count;
	count = (nbytes/sizeof(uint32));

	ASSERT(ISALIGNED(nbytes, sizeof(uint32)));

	vasip_mem = wlc_vasip_addr(wlc_hw, offset);

	/* write vasip code to program memory */
	BCM_REFERENCE(vasip_code);
	BCM_REFERENCE(nbytes);
	for (i = 0; i < count; i++) {
		vasip_mem[i] = vasip_code[i];
	}
}

#ifdef VASIP_SPECTRUM_ANALYSIS
static void
wlc_vasip_spectrum_tbl_write(wlc_hw_info_t *wlc_hw,
        const uint32 vasip_spectrum_tbl[], const uint nbytes_tbl, uint32 offset)
{
	uint32 *vasip_mem;
	int n, tbl_count;

	tbl_count = (nbytes_tbl/sizeof(uint32));

	WL_TRACE(("wl%d: %s\n", wlc_hw->unit, __FUNCTION__));

	ASSERT(ISALIGNED(nbytes_tbl, sizeof(uint32)));

	vasip_mem = wlc_vasip_addr(wlc_hw, offset);

	/* write spactrum analysis tables to VASIP_SPECTRUM_TBL_ADDR_OFFSET */
	BCM_REFERENCE(vasip_spectrum_tbl);
	BCM_REFERENCE(nbytes_tbl);
	for (n = 0; n < tbl_count; n++) {
		vasip_mem[VASIP_SPECTRUM_TBL_ADDR_OFFSET + n] = vasip_spectrum_tbl[n];
	}
}
#endif /* VASIP_SPECTRUM_ANALYSIS */

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
/* read from vasip program memory and compare it with vasip code */
static void
wlc_vasip_verify(wlc_hw_info_t *wlc_hw, const uint32 vasip_code[], const uint nbytes, uint32 offset)
{
	uint32 *vasip_mem;
	uint32 rd_data, err_cnt = 0;
	int i, count;

	WL_TRACE(("wl%d: %s\n", wlc_hw->unit, __FUNCTION__));

	ASSERT(ISALIGNED(nbytes, sizeof(uint32)));

	count = (nbytes/sizeof(uint32));
	vasip_mem = wlc_vasip_addr(wlc_hw, offset);

	/* write vasip code to program memory */
	for (i = 0; i < count; i++) {
		rd_data = vasip_mem[i];
		if (rd_data != vasip_code[i]) {
			err_cnt++;
		}
	}
	if (err_cnt == 0) {
		WL_ERROR(("wl%d: %s, download success\n", wlc_hw->unit, __FUNCTION__));
	} else {
		WL_ERROR(("wl%d: %s, download error\n", wlc_hw->unit, __FUNCTION__));
	}
}
#endif /* BCMDBG || BCMDBG_DUMP */

bool
BCMRAMFN(wlc_vasip_support)(wlc_hw_info_t *wlc_hw, uint32 *vasipver, bool nopi)
{
	d11regs_t *regs = wlc_hw->regs;

	if (nopi) {
		phy_prephy_vasip_ver_get(wlc_hw->prepi, regs, vasipver);
	} else {
		*vasipver = phy_vasip_get_ver((phy_info_t *)wlc_hw->band->pi);
	}

	if (*vasipver == VASIP_NOVERSION) {
		WL_ERROR(("%s: wl%d: vasipver not available\n", __FUNCTION__, wlc_hw->unit));
		return FALSE;
	}

	if (*vasipver == 0) {
		WL_TRACE(("%s: wl%d: vasipver %d\n",
			__FUNCTION__, wlc_hw->unit, *vasipver));
		return TRUE;
	} else if (*vasipver == 3) {
		/* TBD : need to be updated once vasip image is ready */
		WL_ERROR(("%s: wl%d: unsupported vasipver %d\n",
			__FUNCTION__, wlc_hw->unit, *vasipver));
		return FALSE;
	} else {
		WL_ERROR(("%s: wl%d: unsupported vasipver %d\n",
			__FUNCTION__, wlc_hw->unit, *vasipver));
		return FALSE;
	}
}

/* vasip code download */
static void
wlc_vasip_download(wlc_hw_info_t *wlc_hw, const uint32 *vasip_code,
	const uint nbytes, uint32 offset, uint32 vasipver, bool nopi)
{
	d11regs_t *regs = wlc_hw->regs;

	ASSERT(vasip_code);

	if (vasipver == 3) {
		wlc_vasip_clock(wlc_hw, 1);
	}

	/* stop the vasip processor */
	if (nopi) {
		phy_prephy_vasip_proc_reset(wlc_hw->prepi, regs, TRUE);
	} else {
		phy_vasip_reset_proc((phy_info_t *)wlc_hw->band->pi, 1);
	}

	/* write binary to the vasip program memory */
	if (!wlc_hw->vasip_loaded) {
		/* write binary to the vasip program memory */
		if (nopi) {
			wlc_vasip_write(wlc_hw, vasip_code, nbytes, offset);
		} else {
#ifdef SVMP_ACCESS_VIA_PHYTBL
			phy_vasip_write_bin((phy_info_t *)wlc_hw->band->pi,
				vasip_code, nbytes);
#else
			wlc_vasip_write(wlc_hw, vasip_code, nbytes, offset);
#endif /* SVMP_ACCESS_VIA_PHYTBL */
		}

		/* write spectrum tables to the vasip SVMP M4 0x26800 */
#ifdef VASIP_SPECTRUM_ANALYSIS
		if (nopi) {
			wlc_vasip_spectrum_tbl_write(wlc_hw, vasip_code,
				nbytes, offset);
		} else {
#ifdef SVMP_ACCESS_VIA_PHYTBL
			phy_vasip_write_spectrum_tbl((phy_info_t *)wlc_hw->band->pi,
				vasip_code, nbytes);
#else
			wlc_vasip_spectrum_tbl_write(wlc_hw, vasip_code,
				nbytes, offset);
#endif /* SVMP_ACCESS_VIA_PHYTBL */
		}
#endif /* VASIP_SPECTRUM_ANALYSIS */
		wlc_hw->vasip_loaded = TRUE;
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
		wlc_vasip_verify(wlc_hw, vasip_code, nbytes, offset);
#endif /* BCMDBG || BCMDBG_DUMP */
	}

	if (vasipver == 3) {
		wlc_vasip_clock(wlc_hw, 0);
	}

	/* reset and start the vasip processor */
	if (nopi) {
		phy_prephy_vasip_proc_reset(wlc_hw->prepi, regs, FALSE);
	} else {
		phy_vasip_reset_proc((phy_info_t *)wlc_hw->band->pi, 0);
	}
}

/* initialize vasip clock */
static void
wlc_vasip_clock(wlc_hw_info_t *wlc_hw, int enb)
{
	uint32 acphyidx = si_findcoreidx(wlc_hw->sih, ACPHY_CORE_ID, 0);

	if (enb) {
		/* enter reset */
		si_core_wrapperreg(wlc_hw->sih, acphyidx,
			OFFSETOF(aidmp_t, resetctrl), ~0, 0x1);
		/* enable vasip clock */
		si_core_wrapperreg(wlc_hw->sih, acphyidx,
			OFFSETOF(aidmp_t, ioctrl), ~0, 0x1);
		/* exit reset */
		si_core_wrapperreg(wlc_hw->sih, acphyidx,
			OFFSETOF(aidmp_t, resetctrl), ~0, 0x0);
	} else {
		/* disable vasip clock */
		si_core_wrapperreg(wlc_hw->sih, acphyidx,
			OFFSETOF(aidmp_t, ioctrl), ~0, 0x0);
	}
}

/* initialize vasip */
void
BCMATTACHFN(wlc_vasip_init)(wlc_hw_info_t *wlc_hw, uint32 vasipver, bool nopi)
{
	const uint32 *vasip_code = NULL;
	uint nbytes = 0;

#ifdef VASIP_SPECTRUM_ANALYSIS
	if (vasipver == 0 || vasipver == 2) {
		vasip_code = d11vasip_tbl;
		nbytes = d11vasip_tbl_sz;
	} else if (vasipver == 3) {
		/* TBD : need to be updated once vasip image is ready */
		vasip_code = d11vasip_tbl;
		nbytes = d11vasip_tbl_sz;
	} else {
		WL_ERROR(("%s: wl%d: unsupported vasipver %d \n",
			__FUNCTION__, wlc_hw->unit, vasipver));
		ASSERT(0);
		return;
	}
#else
	if (vasipver == 0 || vasipver == 2) {
		vasip_code = d11vasip0;
		nbytes = d11vasip0sz;
	} else if (vasipver == 3) {
		/* TBD : need to be updated once vasip image is ready */
		vasip_code = d11vasip0;
		nbytes = d11vasip0sz;
	} else {
		WL_ERROR(("%s: wl%d: unsupported vasipver %d \n",
			__FUNCTION__, wlc_hw->unit, vasipver));
		ASSERT(0);
		return;
	}
#endif /* VASIP_SPECTRUM_ANALYSIS */

	wlc_vasip_download(wlc_hw, vasip_code, nbytes, VASIP_CODE_OFFSET, vasipver, nopi);
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
/* dump vasip code info */
void
wlc_vasip_code_info(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	bcm_bprintf(b, "vasip code ver %d.%d\n", d11vasipcode_major, d11vasipcode_minor);
	bcm_bprintf(b, "vasip hw rev %d\n",
		phy_vasip_get_ver((phy_info_t *)wlc->hw->band->pi));
	bcm_bprintf(b, "vasip mem addr %p\n", wlc_vasip_addr(wlc->hw, VASIP_CODE_OFFSET));
}

/* dump vasip status data from vasip program memory */
int
wlc_dump_vasip_status(wlc_info_t *wlc, struct bcmstrbuf *b)
{
#ifdef SVMP_ACCESS_VIA_PHYTBL
	uint16 status[VASIP_COUNTERS_LMT];
#else
	uint16 * status;
#endif
	uint32 offset = VASIP_COUNTERS_ADDR_OFFSET;
	wlc_hw_info_t *wlc_hw = wlc->hw;
	int ret = BCME_OK, i;

	if (!VASIP_PRESENT(wlc_hw->corerev)) {
		bcm_bprintf(b, "VASIP is not present!\n");
		return ret;
	}

	if (!wlc->clk) {
		return BCME_NOCLK;
	}

#ifndef SVMP_ACCESS_VIA_PHYTBL
	status = (uint16 *)wlc_vasip_addr(wlc_hw, offset);
#endif

	for (i = 0; i < VASIP_COUNTERS_LMT; i++) {
#ifdef SVMP_ACCESS_VIA_PHYTBL
		phy_vasip_read_svmp((phy_info_t *)wlc_hw->band->pi, offset+i, &status[i]);
#endif
		bcm_bprintf(b, "status[%d] %u\n", i, status[i]);
	}

	return ret;
}

/* dump vasip counters from vasip program memory */
int
wlc_dump_vasip_counters(wlc_info_t *wlc, struct bcmstrbuf *b)
{
#ifdef SVMP_ACCESS_VIA_PHYTBL
	uint16 counter[VASIP_COUNTERS_LMT];
	uint16 mcs[16], mcs1[16], c[4], s[4], c1[4], s1[4], N_user;
#else
	uint16 * counter;
#endif
	uint32 offset = VASIP_COUNTERS_ADDR_OFFSET;
	uint32 offset_steered_mcs = VASIP_STEER_MCS_ADDR_OFFSET;
	uint32 offset_recommended_mcs = VASIP_RECOMMEND_MCS_ADDR_OFFSET;
	wlc_hw_info_t *wlc_hw = wlc->hw;
	int ret = BCME_OK, i;

	if (!VASIP_PRESENT(wlc_hw->corerev)) {
		bcm_bprintf(b, "VASIP is not present!\n");
		return ret;
	}

	if (!wlc->clk) {
		return BCME_NOCLK;
	}

#ifndef SVMP_ACCESS_VIA_PHYTBL
	counter = (uint16 *)wlc_vasip_addr(wlc_hw, offset);
#endif

#ifdef SVMP_ACCESS_VIA_PHYTBL
	/* print for any non-zero values */
	for (i = 0; i < VASIP_COUNTERS_LMT; i++) {
		phy_vasip_read_svmp((phy_info_t *)wlc_hw->band->pi, offset+i, &counter[i]);
	}
	for (i = 0; i < 16; i++) {
		phy_vasip_read_svmp((phy_info_t *)wlc_hw->band->pi, offset_steered_mcs+i, &mcs[i]);
	}


	for (i = 0; i < 4; i++) {
		s[i] = ((mcs[i] & 0xf0) >> 4) + 1;
		c[i] = mcs[i] & 0xf;
	}

	for (i = 0; i < 4; i++) {
		phy_vasip_read_svmp((phy_info_t *)wlc_hw->band->pi,
			offset_recommended_mcs+i, &mcs1[i]);
	}

	for (i = 0; i < 4; i++) {
		s1[i] = ((mcs1[i] & 0xf0) >> 4) + 1;
		c1[i] = mcs1[i] & 0xf;
	}
#endif /* SVMP_ACCESS_VIA_PHYTBL */

	bcm_bprintf(b, "Received Interrupts:\n"
		"      bfr_module_done:0x%x     bfe_module_done:0x%x     bfe_imp_done:0x%x\n"
			"      m2v_transfer_done:0x%x   v2m_transfder_done:0x%x\n\n",
			counter[2], counter[3], counter[4], counter[8], counter[9]);

	N_user = (mcs[9] > 4) ? 0 : mcs[9];
	for (i = 0; i < N_user; i++) {
		bcm_bprintf(b, "user%d: bfidx %d, steered rate c%ds%d, "
				"recommended rate c%ds%d, precoderSNR %ddB\n",
				i, mcs[10+i], c[i], s[i], c1[i], s1[i], mcs[5+i]/4);
	}

	bcm_bprintf(b, "\nImportant SVMP address:\n"
			"      PPR table address:           0x%x\n"
			"      MCS threshold table address: 0x%x\n"
			"      M2V address0:                0x%x\n"
			"      M2V address1:                0x%x\n"
			"      grouping V2M address:        0x%x\n"
			"      precoder V2M address:        0x%x\n"
			"      driver interface address:    0x%x\n",
			VASIP_PPR_TABLE_OFFSET, VASIP_MCS_THRESHOLD_OFFSET, VASIP_M2V0_OFFSET,
			VASIP_M2V1_OFFSET, VASIP_V2M_GROUP_OFFSET, VASIP_V2M_PRECODER_OFFSET,
			VASIP_RATECAP_BLK);

	/*
	for (i = VASIP_DEFINED_COUNTER_NUM; i < VASIP_COUNTERS_LMT; i++) {
		if (counter[i] != 0) {
			bcm_bprintf(b, "counter[%d] 0x%x ", i, counter[i]);
		}
	}
	*/

	if (counter[128] & 0x1)
		bcm_bprintf(b, "ERROR: more than 4 users in group selecton.\n");

	if (counter[128] & 0x2)
		bcm_bprintf(b, "ERROR: user BW mismatch in group selecton.\n");

	if (counter[128] & 0x4)
		bcm_bprintf(b, "ERROR: user BW is beyond 80MHz in group selecton.\n");

	if (counter[128] & 0x8)
		bcm_bprintf(b, "ERROR: BFM index is out of range in group selecton.\n");

	if (counter[128] & 0x10)
		bcm_bprintf(b, "ERROR: BFM index is repeated in group selecton.\n");

	if (counter[128] & 0x20)
		bcm_bprintf(b, "ERROR: output address is out of range in precoder.\n");

	if (counter[128] & 0x40)
		bcm_bprintf(b, "ERROR: BFM index is out of range in precoder.\n");

	if (counter[128] & 0x80)
		bcm_bprintf(b, "ERROR: one user has more than 2 streams in precoder.\n");

	if (counter[128] & 0x100)
		bcm_bprintf(b, "ERROR: more than 4 streams in precoder.\n");

	if (counter[128] & 0x200)
		bcm_bprintf(b, "ERROR: user BW mismatch in precoder.\n");

	if (counter[128] & 0x400)
		bcm_bprintf(b, "ERROR: user BW is beyond 80MHz in precoder.\n");

	if (counter[128] & 0x800)
		bcm_bprintf(b, "ERROR: number of TX antenna is not 4.\n");

	if (counter[128] & 0x1000)
		bcm_bprintf(b, "ERROR: sounding report type is not correct.\n");

	bcm_bprintf(b, "\n");
	return ret;
}

/* clear vasip counters */
int
wlc_vasip_counters_clear(wlc_hw_info_t *wlc_hw)
{
	int i;
#ifdef SVMP_ACCESS_VIA_PHYTBL
	uint32 offset = VASIP_COUNTERS_ADDR_OFFSET;
#else
	uint16 * counter;
#endif

	if (!VASIP_PRESENT(wlc_hw->corerev)) {
		return BCME_UNSUPPORTED;
	}
	if (!wlc_hw->clk) {
		return BCME_NOCLK;
	}

#ifndef SVMP_ACCESS_VIA_PHYTBL
	counter = (uint16 *)wlc_vasip_addr(wlc_hw, VASIP_COUNTERS_ADDR_OFFSET);
#endif
	for (i = 0; i < VASIP_COUNTERS_LMT; i++) {
#ifdef SVMP_ACCESS_VIA_PHYTBL
		phy_vasip_write_svmp((phy_info_t *)wlc_hw->band->pi, offset+i, 0);
#else
		counter[i] = 0;
#endif
	}
	return BCME_OK;
}

/* copy svmp memory to a buffer starting from offset of length 'len', len is count of uint16's */
int
wlc_svmp_mem_read(wlc_hw_info_t *wlc_hw, uint16 *ret_svmp_addr, uint32 offset, uint16 len)
{
#ifndef SVMP_ACCESS_VIA_PHYTBL
	uint16 * svmp_addr;
#endif
	uint16 i;

	if (!VASIP_PRESENT(wlc_hw->corerev)) {
		return BCME_UNSUPPORTED;
	}

	if (!wlc_hw->clk) {
		return BCME_NOCLK;
	}

	if ((offset + len) > SVMP_MEM_OFFSET_MAX) {
		return BCME_RANGE;
	}

#ifndef SVMP_ACCESS_VIA_PHYTBL
	svmp_addr = (uint16 *)wlc_vasip_addr(wlc_hw, offset);
#endif

	for (i = 0; i < len; i++) {
#ifdef SVMP_ACCESS_VIA_PHYTBL
		phy_vasip_read_svmp((phy_info_t *)wlc_hw->band->pi, offset+i, &ret_svmp_addr[i]);
#else
		ret_svmp_addr[i] = svmp_addr[i];
#endif
	}
	return BCME_OK;
}

/* set svmp memory with a value from offset of length 'len', len is count of uint16's */
int
wlc_svmp_mem_set(wlc_hw_info_t *wlc_hw, uint32 offset, uint16 len, uint16 val)
{
#ifndef SVMP_ACCESS_VIA_PHYTBL
	volatile uint16 * svmp_addr;
#endif
	uint16 i;

	if (!VASIP_PRESENT(wlc_hw->corerev)) {
		return BCME_UNSUPPORTED;
	}

	if (!wlc_hw->clk) {
		return BCME_NOCLK;
	}

	if ((offset + len) > SVMP_MEM_OFFSET_MAX) {
		return BCME_RANGE;
	}

#ifndef SVMP_ACCESS_VIA_PHYTBL
	svmp_addr = (volatile uint16 *)wlc_vasip_addr(wlc_hw, offset);
#endif

	for (i = 0; i < len; i++) {
#ifdef SVMP_ACCESS_VIA_PHYTBL
		phy_vasip_write_svmp((phy_info_t *)wlc_hw->band->pi, offset+i, val);
#else
		svmp_addr[i] = val;
#endif
	}
	return BCME_OK;
}
#endif /* BCMDBG || BCMDBG_DUMP */

/* set svmp memory with a value from offset of length 'len', len is count of uint16's */
int
wlc_svmp_mem_blk_set(wlc_hw_info_t *wlc_hw, uint32 offset, uint16 len, uint16 *val)
{
#ifndef SVMP_ACCESS_VIA_PHYTBL
	volatile uint16 * svmp_addr;
#endif

	uint16 i;

	if (!VASIP_PRESENT(wlc_hw->corerev)) {
		return BCME_UNSUPPORTED;
	}

	if (!wlc_hw->clk) {
		return BCME_NOCLK;
	}

	if ((offset + len) > SVMP_MEM_OFFSET_MAX) {
		return BCME_RANGE;
	}


#ifndef SVMP_ACCESS_VIA_PHYTBL
	svmp_addr = (volatile uint16 *)wlc_vasip_addr(wlc_hw, offset);
#endif

	for (i = 0; i < len; i++) {
		if (val[i] != 0xffff) {
#ifdef SVMP_ACCESS_VIA_PHYTBL
			phy_vasip_write_svmp((phy_info_t *)wlc_hw->band->pi, offset+i, val[i]);
#else
			svmp_addr[i] = val[i];
#endif
		}
	}
	return BCME_OK;
}

int
wlc_svmp_read_blk(wlc_hw_info_t *wlc_hw, uint16 *val, uint32 offset, uint16 len)
{
#ifndef SVMP_ACCESS_VIA_PHYTBL
	uint16 *svmp_addr;
#endif
	uint16 i;

	if (!VASIP_PRESENT(wlc_hw->corerev)) {
		return BCME_UNSUPPORTED;
	}

	if (!wlc_hw->clk) {
		return BCME_NOCLK;
	}

	if ((offset + len) > SVMP_MEM_OFFSET_MAX) {
		return BCME_RANGE;
	}

#ifndef SVMP_ACCESS_VIA_PHYTBL
	svmp_addr = (uint16 *)wlc_vasip_addr(wlc_hw, offset);
	for (i = 0; i < len; i++) {
		val[i] = svmp_addr[i];
	}
#else
	for (i = 0; i < (len/SVMP_ACCESS_BLOCK_SIZE); i++) {
		phy_vasip_read_svmp_blk((phy_info_t *)wlc_hw->band->pi,
		        offset, SVMP_ACCESS_BLOCK_SIZE, val);
		offset += SVMP_ACCESS_BLOCK_SIZE;
		val += SVMP_ACCESS_BLOCK_SIZE;
		len -= SVMP_ACCESS_BLOCK_SIZE;
	}
	if (len > 0) {
		phy_vasip_read_svmp_blk((phy_info_t *)wlc_hw->band->pi, offset, len, val);
	}
#endif /* SVMP_ACCESS_VIA_PHYTBL */

	return BCME_OK;
}

int
wlc_svmp_write_blk(wlc_hw_info_t *wlc_hw, uint16 *val, uint32 offset, uint16 len)
{
#ifndef SVMP_ACCESS_VIA_PHYTBL
	volatile uint16 * svmp_addr;
#endif
	uint16 i;

	if (!VASIP_PRESENT(wlc_hw->corerev)) {
		return BCME_UNSUPPORTED;
	}

	if (!wlc_hw->clk) {
		return BCME_NOCLK;
	}

	if ((offset + len) > SVMP_MEM_OFFSET_MAX) {
		return BCME_RANGE;
	}

#ifndef SVMP_ACCESS_VIA_PHYTBL
	svmp_addr = (uint16 *)wlc_vasip_addr(wlc_hw, offset);
	for (i = 0; i < len; i++) {
		if (val[i] != 0xffff) {
			svmp_addr[i] = val[i];
		}
	}
#else
	for (i = 0; i < (len/SVMP_ACCESS_BLOCK_SIZE); i++) {
		phy_vasip_write_svmp_blk((phy_info_t *)wlc_hw->band->pi,
		        offset, SVMP_ACCESS_BLOCK_SIZE, val);
		offset += SVMP_ACCESS_BLOCK_SIZE;
		val += SVMP_ACCESS_BLOCK_SIZE;
		len -= SVMP_ACCESS_BLOCK_SIZE;
	}
	if (len > 0) {
		phy_vasip_write_svmp_blk((phy_info_t *)wlc_hw->band->pi, offset, len, val);
	}
#endif /* SVMP_ACCESS_VIA_PHYTBL */

	return BCME_OK;
}
#endif /* WLVASIP */
