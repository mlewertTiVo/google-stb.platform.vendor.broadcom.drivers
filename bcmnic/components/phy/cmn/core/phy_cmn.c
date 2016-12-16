/*
 * PHYComMoN module implementation
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
 * $Id: phy_cmn.c 659833 2016-09-16 05:39:06Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_cmn.h>

struct phy_cmn_info {
	shared_phy_t	*sh;
	phy_info_t	*pi[MAX_RSDB_MAC_NUM];
	uint8		num_d11_cores;
	uint16		phymode;
	/* for multi slice chips a master override is provided */
	/* this will be helpful in setting the master of shared resources such as fems */
	int8 master_override;
	uint8       femctrl_clb_prio_2g;
	uint8       femctrl_clb_prio_5g;
};

phy_cmn_info_t *
BCMATTACHFN(phy_cmn_attach)(phy_info_t *pi)
{
	phy_cmn_info_t *cmn;
	shared_phy_t *sh;
	int ref_count = 0;
	sh = pi->sh;

	/* OBJECT REGISTRY: check if shared key has value already stored */
	cmn = (phy_cmn_info_t *)wlapi_obj_registry_get(sh->physhim, OBJR_PHY_CMN_INFO);
	if (cmn == NULL) {
		if ((cmn = phy_malloc(pi, sizeof(phy_cmn_info_t))) == NULL) {
			PHY_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", sh->unit,
			          __FUNCTION__, MALLOCED(sh->osh)));
			return NULL;
		}
		cmn->sh = sh;

		/* OBJECT REGISTRY: We are the first instance, store value for key */
		wlapi_obj_registry_set(sh->physhim, OBJR_PHY_CMN_INFO, cmn);
	}

	/* OBJECT REGISTRY: Reference the stored value in both instances */
	ref_count = wlapi_obj_registry_ref(sh->physhim, OBJR_PHY_CMN_INFO);
	ASSERT(ref_count <= MAX_RSDB_MAC_NUM);

	cmn->phymode = wlapi_get_phymode(sh->physhim);

	cmn->pi[ref_count - 1] = pi;
	cmn->num_d11_cores++;
	cmn->master_override = -1;

	PHY_INFORM(("\n*** wl%d: %s: cmn->pi[%d] = pi = %p | num_d11_cores = %d phymode %d ***\n\n",
		sh->unit, __FUNCTION__, ref_count - 1, pi, cmn->num_d11_cores, cmn->phymode));

	return cmn;
}

void
BCMATTACHFN(phy_cmn_detach)(phy_cmn_info_t *cmn)
{
	shared_phy_t *sh;

	(void)sh;

	if (cmn == NULL)
		return;

	sh = cmn->sh;

	if (wlapi_obj_registry_unref(sh->physhim, OBJR_PHY_CMN_INFO) == 0) {
		wlapi_obj_registry_set(sh->physhim, OBJR_PHY_CMN_INFO, NULL);
		MFREE(sh->osh, cmn, sizeof(phy_cmn_info_t));
	}
}

int
phy_cmn_register_obj(phy_cmn_info_t *ci, phy_obj_ptr_t *obj, phy_obj_type_t type)
{
	return BCME_OK;
}

phy_obj_ptr_t *
phy_cmn_find_obj(phy_cmn_info_t *ci, phy_obj_type_t type)
{
	return NULL;
}

phy_info_t *
phy_get_other_pi(phy_info_t *pi)
{
	ASSERT(pi != NULL && pi->cmni != NULL);

	/* Check, out of bound array access */
	if (MAX_RSDB_MAC_NUM < 2)
		return pi;

	return (pi == pi->cmni->pi[PHY_RSBD_PI_IDX_CORE0])
		? pi->cmni->pi[PHY_RSBD_PI_IDX_CORE1]
		: pi->cmni->pi[PHY_RSBD_PI_IDX_CORE0];
}

void
phy_set_phymode(phy_info_t *pi, uint16 new_phymode)
{
	phy_info_t *other_pi;
	ASSERT(pi != NULL && pi->cmni != NULL);

	if (new_phymode == pi->cmni->phymode)
		return;

	pi->cmni->phymode = new_phymode;

	other_pi = phy_get_other_pi(pi);
	/* MAC driver has updated to a new phymode,
	 * set a flag to trigger phy init.
	 */
	other_pi->phyinit_pending = pi->phyinit_pending = TRUE;
}

uint8
phy_get_current_core(phy_info_t *pi)
{
	ASSERT(pi != NULL && pi->cmni != NULL);

	return (pi == pi->cmni->pi[PHY_RSBD_PI_IDX_CORE0])
		? PHY_RSBD_PI_IDX_CORE0
		: PHY_RSBD_PI_IDX_CORE1;
}

uint16
phy_get_phymode(const phy_info_t *pi)
{
	ASSERT(pi != NULL && pi->cmni != NULL);

	return pi->cmni->phymode;
}

phy_info_t *
phy_get_pi(const phy_info_t *pi, int idx)
{
	ASSERT(pi != NULL && pi->cmni != NULL);
	ASSERT(idx < MAX_RSDB_MAC_NUM);

	return pi->cmni->pi[idx];
}

int8
phy_get_master(const phy_info_t *pi)
{
	ASSERT(pi != NULL && pi->cmni != NULL);

	return pi->cmni->master_override;
}

int8
phy_set_master(const phy_info_t *pi, int8 master)
{
	ASSERT(pi != NULL && pi->cmni != NULL);
	pi->cmni->master_override = master;
	return pi->cmni->master_override;
}

void
phy_set_femctrl_clb_prio_5g_acphy(phy_info_t *pi, uint32 slice)
{
	ASSERT(pi != NULL && pi->cmni != NULL);
	pi->cmni->femctrl_clb_prio_5g = (uint8) slice;
}

uint32
phy_get_femctrl_clb_prio_5g_acphy(phy_info_t *pi)
{
	ASSERT(pi != NULL && pi->cmni != NULL);
	return pi->cmni->femctrl_clb_prio_5g;
}

void
phy_set_femctrl_clb_prio_2g_acphy(phy_info_t *pi, uint32 slice)
{
	ASSERT(pi != NULL && pi->cmni != NULL);
	pi->cmni->femctrl_clb_prio_2g = (uint8) slice;
}

uint32
phy_get_femctrl_clb_prio_2g_acphy(phy_info_t *pi)
{
	ASSERT(pi != NULL && pi->cmni != NULL);
	return pi->cmni->femctrl_clb_prio_2g;
}

int
phy_numofcores(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t*)pih;
	return pi->pubpi->phy_corenum;
}
