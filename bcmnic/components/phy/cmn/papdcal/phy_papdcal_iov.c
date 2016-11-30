/*
 * PAPD CAL module implementation - iovar handlers & registration
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
 * $Id: phy_papdcal_iov.c 639713 2016-05-24 18:02:57Z vyass $
 */

#include <phy_papdcal_iov.h>
#include <phy_papdcal.h>
#include <phy_type_papdcal.h>
#include <wlc_phyreg_ac.h>
#include <phy_utils_reg.h>
#include <wlc_iocv_reg.h>
#include <wlc_phy_int.h>

static const bcm_iovar_t phy_papdcal_iovars[] = {
#if defined(WLTEST) || defined(BCMDBG)
	{"phy_enable_epa_dpd_2g", IOV_PHY_ENABLE_EPA_DPD_2G, IOVF_SET_UP, 0, IOVT_INT8, 0},
	{"phy_enable_epa_dpd_5g", IOV_PHY_ENABLE_EPA_DPD_5G, IOVF_SET_UP, 0, IOVT_INT8, 0},
	{"phy_epacal2gmask", IOV_PHY_EPACAL2GMASK, 0, 0, IOVT_INT16, 0},
#endif /* defined(WLTEST) || defined(BCMDBG) */

#if defined(BCMINTERNAL) || defined(WLTEST)
	{"phy_pacalidx0", IOV_PHY_PACALIDX0, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_pacalidx1", IOV_PHY_PACALIDX1, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_pacalidx", IOV_PHY_PACALIDX, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG) || defined(ATE_BUILD)
	{"phy_papd_en_war", IOV_PAPD_EN_WAR, (IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0},
#ifndef ATE_BUILD
	{"phy_skippapd", IOV_PHY_SKIPPAPD, (IOVF_SET_DOWN | IOVF_MFG), 0, IOVT_UINT8, 0},
#endif /* !ATE_BUILD */
#endif /* BCMINTERNAL || WLTEST || DBG_PHY_IOV || WFD_PHY_LL_DEBUG || ATE_BUILD */
#if defined(WFD_PHY_LL)
	{"phy_wfd_ll_enable", IOV_PHY_WFD_LL_ENABLE, 0, 0, IOVT_UINT8, 0},
#endif /* WFD_PHY_LL */
	{NULL, 0, 0, 0, 0, 0}
};

static int
phy_papdcal_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int int_val = 0;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;

	BCM_REFERENCE(*pi);
	BCM_REFERENCE(*ret_int_ptr);

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
#if defined(WLTEST) || defined(BCMDBG)
		case IOV_SVAL(IOV_PHY_ENABLE_EPA_DPD_2G):
		case IOV_SVAL(IOV_PHY_ENABLE_EPA_DPD_5G):
		{
			if ((int_val < 0) || (int_val > 1)) {
				err = BCME_RANGE;
				PHY_ERROR(("Value out of range\n"));
				break;
			}
			phy_papdcal_epa_dpd_set(pi, (uint8)int_val,
				(aid == IOV_SVAL(IOV_PHY_ENABLE_EPA_DPD_2G)));
			break;
		}
		case IOV_GVAL(IOV_PHY_EPACAL2GMASK): {
			*ret_int_ptr = (uint32)pi->papdcali->data->epacal2g_mask;
			break;
		}

		case IOV_SVAL(IOV_PHY_EPACAL2GMASK): {
			pi->papdcali->data->epacal2g_mask = (uint16)int_val;
			break;
		}
#endif /* defined(WLTEST) || defined(BCMDBG) */

#if defined(BCMINTERNAL) || defined(WLTEST)
		case IOV_GVAL(IOV_PHY_PACALIDX0):
			err = phy_papdcal_get_lut_idx0(pi, ret_int_ptr);
			break;

		case IOV_GVAL(IOV_PHY_PACALIDX1):
			err = phy_papdcal_get_lut_idx1(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_PACALIDX):
			err = phy_papdcal_set_idx(pi, (int8)int_val);
			break;
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG) || defined(ATE_BUILD)
		case IOV_SVAL(IOV_PAPD_EN_WAR):
			wlapi_bmac_write_shm(pi->sh->physhim, M_PAPDOFF_MCS(pi), (uint16)int_val);
			break;

		case IOV_GVAL(IOV_PAPD_EN_WAR):
			*ret_int_ptr = wlapi_bmac_read_shm(pi->sh->physhim, M_PAPDOFF_MCS(pi));
			break;
#ifndef ATE_BUILD
		case IOV_SVAL(IOV_PHY_SKIPPAPD):
			if ((int_val != 0) && (int_val != 1)) {
				err = BCME_RANGE;
				break;
			}
			err = phy_papdcal_set_skip(pi, (uint8)int_val);
			break;

		case IOV_GVAL(IOV_PHY_SKIPPAPD):
			err = phy_papdcal_get_skip(pi, ret_int_ptr);
			break;
#endif /* !ATE_BUILD */
#endif /* BCMINTERNAL || WLTEST || DBG_PHY_IOV || WFD_PHY_LL_DEBUG || ATE_BUILD */
#if defined(WFD_PHY_LL)
		case IOV_SVAL(IOV_PHY_WFD_LL_ENABLE):
			if ((int_val < 0) || (int_val > 2)) {
				err = BCME_RANGE;
			} else {
				err = phy_papdcal_set_wfd_ll_enable(pi->papdcali, (uint8) int_val);
			}
			break;

		case IOV_GVAL(IOV_PHY_WFD_LL_ENABLE):
			err = phy_papdcal_get_wfd_ll_enable(pi->papdcali, ret_int_ptr);
			break;
#endif /* WFD_PHY_LL */
		default:
			err = BCME_UNSUPPORTED;
			break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_papdcal_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_papdcal_iovars,
	                   NULL, NULL,
	                   phy_papdcal_doiovar, NULL, NULL, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
