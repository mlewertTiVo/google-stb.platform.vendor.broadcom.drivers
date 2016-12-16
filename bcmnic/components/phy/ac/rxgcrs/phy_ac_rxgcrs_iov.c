/*
 * ACPHY Rx Gain Control and Carrier Sense module implementation - iovar handlers & registration
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
 * $Id: phy_ac_rxgcrs_iov.c 663404 2016-10-05 07:00:28Z $
 */

#include <phy_ac_rxgcrs_iov.h>
#include <phy_ac_rxgcrs.h>
#include <wlc_iocv_reg.h>
#include <phy_ac_info.h>

/* iovar ids */
enum {
	IOV_PHY_FORCE_GAINLEVEL = 1,
	IOV_PHY_FORCE_CRSMIN = 2,
	IOV_PHY_FORCE_LESISCALE = 3,
	IOV_PHY_LESI = 4
};

static const bcm_iovar_t phy_ac_rxgcrs_iovars[] = {
#if (defined(BCMDBG) || defined(BCMDBG_DUMP))
	{"phy_force_gainlevel", IOV_PHY_FORCE_GAINLEVEL,
	IOVF_SET_UP, 0, IOVT_UINT32, 0},
#endif /* BCMDBG */
#if ACCONF || ACCONF2
	{"phy_force_crsmin", IOV_PHY_FORCE_CRSMIN,
	IOVF_SET_UP, 0, IOVT_BUFFER, 4*sizeof(int8)},
	{"phy_force_lesiscale", IOV_PHY_FORCE_LESISCALE,
	IOVF_SET_UP, 0, IOVT_BUFFER, 4*sizeof(int8)},
#endif /* ACCONF  */
	{"phy_lesi", IOV_PHY_LESI,
	IOVF_SET_UP, 0, IOVT_UINT32, 0},
	{NULL, 0, 0, 0, 0, 0}
};

static int
phy_ac_rxgcrs_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif);

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

static int
phy_ac_rxgcrs_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int32 int_val = 0;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;
	phy_ac_rxgcrs_info_t *rxgcrsi = pi->u.pi_acphy->rxgcrsi;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
#if (defined(BCMDBG) || defined(BCMDBG_DUMP))
		case IOV_SVAL(IOV_PHY_FORCE_GAINLEVEL):
		{
			wlc_phy_force_gainlevel_acphy(pi, (int16) int_val);
			break;
		}
#endif /* BCMDBG */
#if ACCONF || ACCONF2
		case IOV_SVAL(IOV_PHY_FORCE_CRSMIN):
		{
			wlc_phy_force_crsmin_acphy(pi, p);
			break;
		}
		case IOV_SVAL(IOV_PHY_FORCE_LESISCALE):
		{
			phy_ac_rxgcrs_force_lesiscale(rxgcrsi, p);
			break;
		}
#endif /* ACCONF || ACCONF2 */
		case IOV_GVAL(IOV_PHY_LESI):
			err = phy_ac_rxgcrs_iovar_get_lesi(rxgcrsi, ret_int_ptr);
			break;
		case IOV_SVAL(IOV_PHY_LESI):
			err = phy_ac_rxgcrs_iovar_set_lesi(rxgcrsi, int_val);
			break;

		default:
			err = BCME_UNSUPPORTED;
			break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_ac_rxgcrs_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;
#if defined(WLC_PATCH_IOCTL)
	wlc_iov_disp_fn_t disp_fn = IOV_PATCH_FN;
	const bcm_iovar_t *patch_table = IOV_PATCH_TBL;
#else
	wlc_iov_disp_fn_t disp_fn = NULL;
	const bcm_iovar_t* patch_table = NULL;
#endif /* WLC_PATCH_IOCTL */

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_ac_rxgcrs_iovars,
	                   NULL, NULL,
	                   phy_ac_rxgcrs_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
