/*
 * TxPowerCapl module implementation.
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
 * $Id: phy_txpwrcap.c 644299 2016-06-18 03:37:28Z gpasrija $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_init.h>
#include <phy_rstr.h>
#include <phy_type_txpwrcap.h>
#include <phy_txpwrcap_api.h>
#include <phy_txpwrcap.h>
#include <phy_utils_channel.h>
#include <phy_utils_var.h>

#if defined(WLC_TXPWRCAP)

/* ******* Local Functions ************ */
static int phy_txpwrcap_init(phy_init_ctx_t *ctx);

/* ********************************** */

/* module private states */
struct phy_txpwrcap_info {
	phy_info_t *pi;
	phy_type_txpwrcap_fns_t *fns;
};

/* module private states memory layout */
typedef struct {
	phy_txpwrcap_info_t info;
	phy_type_txpwrcap_fns_t fns;
/* add other variable size variables here at the end */
} phy_txpwrcap_mem_t;

/* local function declaration */

/* attach/detach */
phy_txpwrcap_info_t *
BCMATTACHFN(phy_txpwrcap_attach)(phy_info_t *pi)
{
	phy_txpwrcap_info_t *info;
	phy_type_txpwrcap_fns_t *fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((info = phy_malloc(pi, sizeof(phy_txpwrcap_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	info->pi = pi;
	fns = &((phy_txpwrcap_mem_t *)info)->fns;
	info->fns = fns;

	/* register init fn */
	if (phy_init_add_init_fn(pi->initi, phy_txpwrcap_init, info, PHY_INIT_TXPWRCAP) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register callbacks */

	return info;

	/* error */
fail:
	phy_txpwrcap_detach(info);
	return NULL;
}

void
BCMATTACHFN(phy_txpwrcap_detach)(phy_txpwrcap_info_t *info)
{
	phy_info_t *pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (info == NULL) {
		PHY_INFORM(("%s: null txpwrcap module\n", __FUNCTION__));
		return;
	}

	pi = info->pi;

	phy_mfree(pi, info, sizeof(phy_txpwrcap_mem_t));
}


/* register phy type specific implementations */
int
BCMATTACHFN(phy_txpwrcap_register_impl)(phy_txpwrcap_info_t *ti, phy_type_txpwrcap_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));


	*ti->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_txpwrcap_unregister_impl)(phy_txpwrcap_info_t *ti)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

/* init Txpwrcap */
static int
WLBANDINITFN(phy_txpwrcap_init)(phy_init_ctx_t *ctx)
{
	phy_txpwrcap_info_t *ii = (phy_txpwrcap_info_t *)ctx;
	phy_type_txpwrcap_fns_t *fns = ii->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	ASSERT(fns->init != NULL);
	return (fns->init)(fns->ctx);
}


int
wlc_phy_txpwrcap_tbl_set(wlc_phy_t *pih, wl_txpwrcap_tbl_t *txpwrcap_tbl)
{
	phy_info_t *pi = (phy_info_t *)pih;
	phy_txpwrcap_info_t *ti = pi->txpwrcapi;
	phy_type_txpwrcap_fns_t *fns = ti->fns;

	if (fns->txpwrcap_tbl_set)
		return (fns->txpwrcap_tbl_set)(fns->ctx, txpwrcap_tbl);
	else
		return BCME_UNSUPPORTED;
}

int
wlc_phy_txpwrcap_tbl_get(wlc_phy_t *pih, wl_txpwrcap_tbl_t *txpwrcap_tbl)
{
	phy_info_t *pi = (phy_info_t *)pih;
	phy_txpwrcap_info_t *ti = pi->txpwrcapi;
	phy_type_txpwrcap_fns_t *fns = ti->fns;

	if (fns->txpwrcap_tbl_get)
		return (fns->txpwrcap_tbl_get)(fns->ctx, txpwrcap_tbl);
	else
		return BCME_UNSUPPORTED;

}

int
wlc_phy_cellstatus_override_set(phy_info_t *pi, int value)
{
	phy_txpwrcap_info_t *ti = pi->txpwrcapi;
	phy_type_txpwrcap_fns_t *fns = ti->fns;

	if (fns->txpwrcap_set_cellstatus) {
		if (value == -1)
			/* Clear the Force bit allowing WCI2 updates to take effect */
			(fns->txpwrcap_set_cellstatus)(fns->ctx,
				(TXPWRCAP_CELLSTATUS_FORCE_MASK |
				TXPWRCAP_CELLSTATUS_FORCE_UPD_MASK),
				0);
		else
			(fns->txpwrcap_set_cellstatus)(fns->ctx,
				(TXPWRCAP_CELLSTATUS_FORCE_MASK |
				TXPWRCAP_CELLSTATUS_FORCE_UPD_MASK |
				TXPWRCAP_CELLSTATUS_MASK),
				(TXPWRCAP_CELLSTATUS_FORCE_MASK |
				TXPWRCAP_CELLSTATUS_FORCE_UPD_MASK |
				(value & TXPWRCAP_CELLSTATUS_MASK)));
		return BCME_OK;
	} else
		return BCME_UNSUPPORTED;
}

int wlc_phyhal_txpwrcap_get_cellstatus(wlc_phy_t *pih, int32* cellstatus)
{
	phy_info_t *pi = (phy_info_t *)pih;
	phy_txpwrcap_info_t *ti = pi->txpwrcapi;
	phy_type_txpwrcap_fns_t *fns = ti->fns;

	if (fns->txpwrcap_get_cellstatus) {
		*cellstatus = (fns->txpwrcap_get_cellstatus)(fns->ctx);
		return BCME_OK;
	}
	else
		return BCME_UNSUPPORTED;
}

void
wlc_phyhal_txpwrcap_set_cellstatus(wlc_phy_t *pih, int status)
{
	phy_info_t *pi = (phy_info_t*)pih;
	phy_txpwrcap_info_t *ti = pi->txpwrcapi;
	phy_type_txpwrcap_fns_t *fns = ti->fns;

	if (fns->txpwrcap_set_cellstatus) {
		(fns->txpwrcap_set_cellstatus)(fns->ctx, TXPWRCAP_CELLSTATUS_WCI2_MASK,
			(status & 1) << TXPWRCAP_CELLSTATUS_WCI2_NBIT);
	}
}

uint32
wlc_phy_get_txpwrcap_inuse(phy_info_t *pi)
{
	phy_txpwrcap_info_t *ti = pi->txpwrcapi;
	phy_type_txpwrcap_fns_t *fns = ti->fns;

	if (fns->txpwrcap_in_use) {
		return (fns->txpwrcap_in_use)(fns->ctx);
	} else
		return 0;
}

bool
wlc_phy_txpwrcap_get_enabflg(wlc_phy_t *pih)
{
	return (((phy_info_t *)pih)->_txpwrcap);
}

#ifdef WLC_SW_DIVERSITY
uint8
wlc_phy_get_current_core(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t*)pih;
	return phy_get_current_core(pi);
}
void
wlc_phy_txpwrcap_to_shm(wlc_phy_t *pih, uint16 tx_ant, uint16 rx_ant)
{
	phy_info_t *pi = (phy_info_t*)pih;
	phy_txpwrcap_info_t *ti = pi->txpwrcapi;
	phy_type_txpwrcap_fns_t *fns = ti->fns;

	if (fns->txpwrcap_to_shm)
		(fns->txpwrcap_to_shm)(fns->ctx, tx_ant, rx_ant);
}
#endif /* WLC_SW_DIVERSITY */
#endif /* WLC_TXPWRCAP */
