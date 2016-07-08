/*
 * ACPHY Calibration Cache module implementation
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_cache.h"
#include <phy_ac.h>
#include <phy_ac_cache.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_radioreg_20691.h>
#include <wlc_radioreg_20696.h>
#include <wlc_phy_radio.h>
#include <wlc_phyreg_ac.h>
#include <wlc_phy_int.h>

#include <phy_utils_reg.h>
#include <phy_ac_info.h>

#define MODULE_DETACH(var, detach_func)\
	if (var) { \
		detach_func(var); \
		(var) = NULL; \
	}

/* module private states */
struct phy_ac_cache_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_cache_info_t *cmn_info;
};

/* local functions */
bool BCMATTACHFN(phy_ac_reg_cache_attach)(phy_info_acphy_t *pi_ac);

/* register phy type specific implementation */
phy_ac_cache_info_t *
BCMATTACHFN(phy_ac_cache_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_cache_info_t *cmn_info)
{
	phy_ac_cache_info_t *ac_info;
	phy_type_cache_fns_t fns;
	phy_info_acphy_t *pi_ac;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ac_info = phy_malloc(pi, sizeof(phy_ac_cache_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	ac_info->pi = pi;
	ac_info->aci = aci;
	ac_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = ac_info;

	if (phy_cache_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_cache_register_impl failed\n", __FUNCTION__));
		goto fail;
	}
	pi_ac = pi->u.pi_acphy;

	if (ACREV_GE(pi->pubpi.phy_rev, 36)) {
		if (ACMAJORREV_40(pi->pubpi.phy_rev)) {
			/* Using same method as 43012 */
			if (!phy_ac_reg_cache_attach(pi_ac)) {
				PHY_ERROR(("%s: phy_ac_reg_cache_attach failed\n", __FUNCTION__));
				goto fail;
			}
		} else if (ACMAJORREV_36(pi->pubpi.phy_rev)) {
			/* setup PHY/RADIO register cache mechanism */
			if (!phy_ac_reg_cache_attach(pi_ac)) {
				PHY_ERROR(("%s: phy_ac_reg_cache_attach failed\n", __FUNCTION__));
				goto fail;
			}
		} else if (ACMAJORREV_33(pi->pubpi.phy_rev) && ACMINORREV_4(pi)) {
			/* Using same method as 43012 */
			if (!phy_ac_reg_cache_attach(pi_ac)) {
				PHY_ERROR(("%s: phy_ac_reg_cache_attach failed\n", __FUNCTION__));
				goto fail;
			}
		}
	}
	return ac_info;

	/* error handling */
fail:
	if (ac_info != NULL)
		phy_mfree(pi, ac_info, sizeof(phy_ac_cache_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_cache_unregister_impl)(phy_ac_cache_info_t *ac_info)
{
	phy_info_t *pi;
	phy_cache_info_t *cmn_info;
	phy_info_acphy_t *pi_ac;

	PHY_TRACE(("%s\n", __FUNCTION__));
	ASSERT(ac_info);
	pi = ac_info->pi;
	cmn_info = ac_info->cmn_info;

	pi_ac = pi->u.pi_acphy;
	BCM_REFERENCE(pi_ac);
	MODULE_DETACH(pi_ac, phy_ac_reg_cache_detach);

	/* unregister from common */
	phy_cache_unregister_impl(cmn_info);

	phy_mfree(pi, ac_info, sizeof(phy_ac_cache_info_t));
}

#define SAVE	1
#define RESTORE	0
static int
phy_ac_reg_cache_process(phy_info_acphy_t *pi_ac, int id, bool save)
{
	int i = 0, core = 0;
	phy_info_t *pi = pi_ac->pi;
	phy_param_info_t *p = pi_ac->paramsi;
	ad_t *cache = NULL;

	/* check if the id to process is reg cache id in use */
	if (id != p->phy_reg_cache_id && id != p->reg_cache_id) {
		PHY_ERROR(("%s: id mismatch (%d != %d %d != %d) - invalid cache!\n", __FUNCTION__,
				id, p->phy_reg_cache_id, id, p->reg_cache_id));
		return BCME_BADARG;
	}

	switch (id) {
		case RADIOREGS_TXIQCAL:
		case RADIOREGS_RXIQCAL:
		case RADIOREGS_PAPDCAL:
		case RADIOREGS_AFECAL:
		case RADIOREGS_TEMPSENSE_VBAT:
		case RADIOREGS_TSSI:
			FOREACH_CORE(pi, core) {
				/* setup base */
				cache = p->reg_cache;
				for (i = 0; i < p->reg_sz; i++) {
					if (save) {
						cache->data[core] =
							phy_utils_read_radioreg(pi,
								cache->addr[core]);
					} else {
						phy_utils_write_radioreg(pi,
								cache->addr[core],
								cache->data[core]);
					}
					cache++;
				}
			}
			break;

		case PHYREGS_TXIQCAL:
		case PHYREGS_RXIQCAL:
		case PHYREGS_PAPDCAL:
		case PHYREGS_TEMPSENSE_VBAT:
		case PHYREGS_TSSI:
			FOREACH_CORE(pi, core) {
				/* setup base */
				cache = p->phy_reg_cache;
				for (i = 0; i < p->phy_reg_sz; i++) {
					if (save) {
						cache->data[core] =
							phy_utils_read_phyreg(pi,
								cache->addr[core]);
					} else {
						phy_utils_write_phyreg(pi,
								cache->addr[core],
								cache->data[core]);
					}
					cache++;
				}
			}
			break;

		default:
			PHY_ERROR(("%s: invalid id\n", __FUNCTION__));
			ASSERT(0);
			break;
	}
	return BCME_OK;
}


static void
phy_ac_reg_cache_parse(phy_info_acphy_t *pi_ac, uint16 id, bool populate)
{
	phy_info_t *pi;
	phy_param_info_t *p;
	ad_t *base;
	ad_t *cache;


	uint16 radioregs_tssi_setup_20696[] = {
		RADIO_REG_20696(pi, AUXPGA_OVR1, 0),
		RADIO_REG_20696(pi, IQCAL_OVR1, 0),
		RADIO_REG_20696(pi, TESTBUF_OVR1, 0),
		RADIO_REG_20696(pi, AUXPGA_CFG1, 0),
		RADIO_REG_20696(pi, IQCAL_CFG1, 0),
		RADIO_REG_20696(pi, IQCAL_CFG2, 0),
		RADIO_REG_20696(pi, IQCAL_CFG4, 0),
		RADIO_REG_20696(pi, IQCAL_CFG5, 0),
		RADIO_REG_20696(pi, IQCAL_CFG6, 0),
		RADIO_REG_20696(pi, IQCAL_GAIN_RFB, 0),
		RADIO_REG_20696(pi, IQCAL_GAIN_RIN, 0),
		RADIO_REG_20696(pi, IQCAL_IDAC, 0),
		RADIO_REG_20696(pi, LPF_REG7, 0),
		RADIO_REG_20696(pi, LPF_REG8, 0),
		RADIO_REG_20696(pi, TESTBUF_CFG1, 0),
	};

	pi = pi_ac->pi;
	p = pi_ac->paramsi;
	base = p->reg_cache;

	cache = base;

	BCM_REFERENCE(pi);
	if (!populate) {
		uint16 radioreg_size = 0;
		uint16 phyreg_size = 0;
		p->reg_cache_id = id;
		p->phy_reg_cache_id = id;

		radioreg_size = ARRAYSIZE(radioregs_tssi_setup_20696);




		phyreg_size = radioreg_size;

		p->phy_reg_cache_depth =  phyreg_size;
		p->reg_cache_depth = radioreg_size;

	} else {
		uint16 *list = NULL;
		uint16 sz = 0;
		switch (id) {
			case RADIOREGS_TSSI:
				list = radioregs_tssi_setup_20696;
				sz   = ARRAYSIZE(radioregs_tssi_setup_20696);
				base = p->reg_cache; /* to baseline for looping over cores */

				p->reg_sz = sz;
				p->reg_cache_id = id;

				break;
			default:
				PHY_ERROR(("%s: Invalid ID %d\n", __FUNCTION__, id));
				ASSERT(0);
				break;
		}
		if (list != NULL) {
			uint8 i = 0, core = 0;
			/* update reg cache id in use */
			/* align to base */
			cache = base;
			PHY_INFORM(("***** core %d ***** : %s\n", core, __FUNCTION__));
			for (i = 0; i < sz; i++) {
				if (ACMAJORREV_33(pi->pubpi.phy_rev) && ACMINORREV_4(pi)) {
					cache->addr[core] = list[i];
					cache->addr[core+1] = (list[i] + JTAG_20696_CR1);
					cache->addr[core+2] = (list[i] + JTAG_20696_CR2);
					cache->addr[core+3] = (list[i] + JTAG_20696_CR3);
					cache->data[core] = 0;
					cache->data[core+1] = 0;
					cache->data[core+2] = 0;
					cache->data[core+3] = 0;
				} else {
					cache->addr[core] = list[i];
					cache->data[core] = 0;
				}
				PHY_INFORM(("0x%8p: 0x%04x\n", cache, cache->addr[core]));
				cache++;
			}

		}
	}	
}

static void
phy_ac_reg_cache_setup(phy_info_acphy_t *pi_ac, uint16 id)
{
	phy_ac_reg_cache_parse(pi_ac, id, TRUE);
}

static void
phy_ac_upd_reg_cache_depth(phy_info_acphy_t *pi_ac)
{
	phy_ac_reg_cache_parse(pi_ac, 0, FALSE);
}


void
BCMATTACHFN(phy_ac_reg_cache_detach)(phy_info_acphy_t *pi_ac)
{
	phy_info_t *pi = pi_ac->pi;
	phy_param_info_t *p = pi_ac->paramsi;

	if (p) {
		if (p->phy_reg_cache)
			phy_mfree(pi, p->phy_reg_cache, p->phy_reg_cache_depth * sizeof(ad_t));
		if (p->reg_cache)
			phy_mfree(pi, p->reg_cache, p->reg_cache_depth * sizeof(ad_t));
	}
}


bool
BCMATTACHFN(phy_ac_reg_cache_attach)(phy_info_acphy_t *pi_ac)
{
	phy_info_t *pi = pi_ac->pi;
	phy_param_info_t *p = pi_ac->paramsi;

	/* update reg cache depth */
	phy_ac_upd_reg_cache_depth(pi_ac);

	if ((p->phy_reg_cache = phy_malloc(pi, p->phy_reg_cache_depth * sizeof(ad_t))) == NULL) {
		PHY_ERROR(("%s: phy phy_malloc ad_t failed\n", __FUNCTION__));
		goto fail;
	}
	if ((p->reg_cache = phy_malloc(pi, p->reg_cache_depth * sizeof(ad_t))) == NULL) {
		PHY_ERROR(("%s: radio phy_malloc ad_t failed\n", __FUNCTION__));
		goto fail;
	}

	return TRUE;
fail:
	MODULE_DETACH(pi_ac, phy_ac_reg_cache_detach);
	return FALSE;
}


int
phy_ac_reg_cache_save(phy_info_acphy_t *pi_ac, uint16 id)
{
	phy_ac_reg_cache_setup(pi_ac, id);
	return phy_ac_reg_cache_process(pi_ac, id, SAVE);
}


int
phy_ac_reg_cache_restore(phy_info_acphy_t *pi_ac, int id)
{
	return phy_ac_reg_cache_process(pi_ac, id, RESTORE);
}
