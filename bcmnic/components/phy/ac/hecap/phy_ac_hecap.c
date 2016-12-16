/*
 * ACPHY High Efficiency 802.11ax (HE) module implementation
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */

#ifdef WL11AX

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <qmath.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_type_hecap.h>
#include <phy_ac.h>
#include <phy_ac_info.h>
#include <phy_ac_hecap.h>
#include <phy_utils_reg.h>
#include <wlc_phyreg_ac.h>
#include <wlioctl.h>
#include <phy_hecap_api.h>

/* module private states */
struct phy_ac_hecap_info {
	phy_info_t *pi;
	phy_ac_info_t *pi_ac;
	phy_hecap_info_t *hecap_info;
	uint32 phy_he_caps;			    /* Capabilities from HE specific registers */
	uint32 phy_he_caps1;			/* Capabilities from HE specific registers */
};

/* local functions */
void wlc_phy_hecap_enable_acphy(phy_info_t *pi, bool enable);

/* External Definitions */

/* register phy type specific implementation */
phy_ac_hecap_info_t*
BCMATTACHFN(phy_ac_hecap_register_impl)(phy_info_t *pi,
	phy_ac_info_t *pi_ac, phy_hecap_info_t *hecapi)
{
	phy_ac_hecap_info_t *ac_hecap_info;
	phy_type_hecap_fns_t fns;
	uint8 temp_reg = 0;
	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ac_hecap_info = phy_malloc(pi, sizeof(phy_ac_hecap_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	ac_hecap_info->pi = pi;
	ac_hecap_info->pi_ac = pi_ac;
	ac_hecap_info->hecap_info = hecapi;

	/* HE Capabilities */
	/* HE Generic Capabilities */
	ac_hecap_info->phy_he_caps |= READ_PHYREGFLD(pi, PhyInternalCapability5, BWSupport11ax) ?
		PHY_CAP_HE_BW : 0;
	ac_hecap_info->phy_he_caps |= READ_PHYREGFLD(pi, PhyInternalCapability5, PESupport11ax) ?
		PHY_CAP_HE_PE : 0;
	ac_hecap_info->phy_he_caps |= READ_PHYREGFLD(pi, PhyCapability2, Support11axDCM) ?
		PHY_CAP_HE_DCM : 0;
	ac_hecap_info->phy_he_caps |= READ_PHYREGFLD(pi, PhyInternalCapability5,
		BeamChangeSupport11ax) ? PHY_CAP_HE_BEAM_CHANGE : 0;
	temp_reg = READ_PHYREGFLD(pi, PhyInternalCapability5, CPLTFModesSupport11ax);
	ac_hecap_info->phy_he_caps |= (temp_reg & BIT0) ? PHY_CAP_HE_1XLTF_0P8CP : 0;
	ac_hecap_info->phy_he_caps |= (temp_reg & BIT1) ? PHY_CAP_HE_2XLTF_0P8CP : 0;
	ac_hecap_info->phy_he_caps |= (temp_reg & BIT2) ? PHY_CAP_HE_2XLTF_1P6CP : 0;
	ac_hecap_info->phy_he_caps |= (temp_reg & BIT3) ? PHY_CAP_HE_4XLTF_3P2CP : 0;

	/* Single User Capabilities */
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability3,
		SupportSTBCRx11axSU) ? PHY_CAP_HE_SU_STBC_RX : 0;
	temp_reg = READ_PHYREGFLD(pi, PhyInternalCapability3, Support256qamBccSU);
	ac_hecap_info->phy_he_caps1 |= (temp_reg & BIT0) ? PHY_CAP_HE_SU_256Q_BCC_NSS1_0P8CP : 0;
	ac_hecap_info->phy_he_caps1 |= (temp_reg & BIT1) ? PHY_CAP_HE_SU_256Q_BCC_NSS2_0P8CP : 0;
	ac_hecap_info->phy_he_caps1 |= (temp_reg & BIT2) ? PHY_CAP_HE_SU_256Q_BCC_NSS1_1P6CP : 0;
	ac_hecap_info->phy_he_caps1 |= (temp_reg & BIT3) ? PHY_CAP_HE_SU_256Q_BCC_NSS2_1P6CP : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability3,
		Support256qamLdpcSU) ? PHY_CAP_HE_SU_256Q_LDPC : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability3,
		Support1024qamLdpcSU) ? PHY_CAP_HE_SU_1024Q_LDPC : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability3, SupportSUBfmer) ?
		PHY_CAP_HE_SU_BFR : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability3, SupportSUBfee) ?
		PHY_CAP_HE_SU_BFE : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability3,
		SupportMUMIMOBfmerSU) ? PHY_CAP_HE_SU_MU_BFR : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability3,
		SupportMUMIMOBfeeSU) ? PHY_CAP_HE_SU_MU_BFE : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability3, NumSoundDimSU) ?
		PHY_CAP_HE_SU_NUM_SOUND : 0;

	/* Resource Unit Capabilities */
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability4,
		SupportSTBCRx11axRU) ? PHY_CAP_HE_RU_STBC_RX : 0;
	temp_reg = READ_PHYREGFLD(pi, PhyInternalCapability4, Support256qamBccRU);
	ac_hecap_info->phy_he_caps1 |= (temp_reg & BIT0) ? PHY_CAP_HE_RU_256Q_BCC_NSS1_0P8CP : 0;
	ac_hecap_info->phy_he_caps1 |= (temp_reg & BIT1) ? PHY_CAP_HE_RU_256Q_BCC_NSS2_0P8CP : 0;
	ac_hecap_info->phy_he_caps1 |= (temp_reg & BIT2) ? PHY_CAP_HE_RU_256Q_BCC_NSS1_1P6CP : 0;
	ac_hecap_info->phy_he_caps1 |= (temp_reg & BIT3) ? PHY_CAP_HE_RU_256Q_BCC_NSS2_1P6CP : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability4,
		Support256qamLdpcRU) ? PHY_CAP_HE_RU_256Q_LDPC : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability4,
		Support1024qamLdpcRU) ? PHY_CAP_HE_RU_1024Q_LDPC : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability4,
		SupportRUBfmer) ? PHY_CAP_HE_RU_BFR : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability4,
		SupportRUBfee) ? PHY_CAP_HE_RU_BFE : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability4,
		SupportMUMIMOBfmerRU) ? PHY_CAP_HE_RU_MU_BFR : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability4,
		SupportMUMIMOBfeeRU) ? PHY_CAP_HE_RU_MU_BFE : 0;
	ac_hecap_info->phy_he_caps1 |= READ_PHYREGFLD(pi, PhyInternalCapability4,
		NumSoundDimRU) ? PHY_CAP_HE_RU_NUM_SOUND : 0;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));

	fns.hecap = wlc_phy_hecap_enable_acphy;

	fns.ctx = pi;

	phy_hecap_register_impl(hecapi, &fns);

	return ac_hecap_info;
	/* error handling */
fail:
	if (ac_hecap_info != NULL)
		phy_mfree(pi, ac_hecap_info, sizeof(phy_ac_hecap_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_hecap_unregister_impl)(phy_ac_hecap_info_t *ac_hecap_info)
{
	phy_info_t *pi = ac_hecap_info->pi;
	phy_hecap_info_t *hecapi = ac_hecap_info->hecap_info;
	PHY_TRACE(("%s\n", __FUNCTION__));
	/* unregister from common */
	phy_hecap_unregister_impl(hecapi);
	phy_mfree(pi, ac_hecap_info, sizeof(phy_ac_hecap_info_t));
}

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */

void
wlc_phy_hecap_enable_acphy(phy_info_t *pi, bool enable)
{
}


/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */

/* Returns Generic HE capabilities */
uint32
wlc_phy_ac_hecaps(phy_info_t *pi)
{
	return (pi->u.pi_acphy->hecapi->phy_he_caps);
}

/* Returns SU and RU HE capabilities */
uint32
wlc_phy_ac_hecaps1(phy_info_t *pi)
{
	return (pi->u.pi_acphy->hecapi->phy_he_caps1);
}

#endif /* WL11AX */
