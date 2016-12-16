/*
 * TEMPerature sense module implementation.
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
 * $Id: phy_temp.c 670800 2016-11-17 16:00:05Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_rstr.h>
#include "phy_type_temp.h"
#include "phy_temp_st.h"
#include <phy_temp.h>
#include <phy_hc.h>

#include <phy_utils_var.h>

/* module private states */
struct phy_temp_info {
	phy_info_t *pi;
	phy_type_temp_fns_t *fns;
	/* tempsense */
	phy_txcore_temp_t *temp;
};

/* module private states memory layout */
typedef struct {
	phy_temp_info_t info;
	phy_type_temp_fns_t fns;
	phy_txcore_temp_t temp;
/* add other variable size variables here at the end */
} phy_temp_mem_t;

static void wlc_phy_read_tempdelta_settings(phy_temp_info_t *tempi, int maxtempdelta);

/* local function declaration */

/* attach/detach */
phy_temp_info_t *
BCMATTACHFN(phy_temp_attach)(phy_info_t *pi)
{
	phy_temp_info_t *info;
	phy_txcore_temp_t *temp;
	uint8 init_txrxchain;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((info = phy_malloc(pi, sizeof(phy_temp_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	info->pi = pi;
	info->fns = &((phy_temp_mem_t *)info)->fns;

	/* Parameters for temperature-based fallback to 1-Tx chain */
	temp = &((phy_temp_mem_t *)info)->temp;
	info->temp = temp;

	init_txrxchain = (1 << PHYCORENUM(pi->pubpi->phy_corenum)) - 1;

	temp->disable_temp = (uint8)PHY_GETINTVAR(pi, rstr_tempthresh);
	temp->disable_temp_max_cap = temp->disable_temp;
	temp->hysteresis = (uint8)PHY_GETINTVAR(pi, rstr_temps_hysteresis);
	if ((temp->hysteresis == 0) || (temp->hysteresis == 0xf)) {
		temp->hysteresis = PHY_HYSTERESIS_DELTATEMP;
	}

	temp->enable_temp =
		temp->disable_temp - temp->hysteresis;

	temp->heatedup = FALSE;
	temp->degrade1RXen = FALSE;

	temp->bitmap = (init_txrxchain << 4 | init_txrxchain);

#ifdef DUTY_CYCLE_THROTTLING
	pi->_dct = TRUE;
#else
	pi->_dct = FALSE;
#endif /* DUTY_CYCLE_THROTTLING */
	temp->duty_cycle = 100;
	temp->duty_cycle_throttle_depth = 10;
	temp->duty_cycle_throttle_state = 0;

	pi->phy_tempsense_offset = 0;

	wlc_phy_read_tempdelta_settings(info, PHY_CAL_MAXTEMPDELTA);

	/* Register callbacks */

	return info;

	/* error */
fail:
	phy_temp_detach(info);
	return NULL;
}

void
BCMATTACHFN(phy_temp_detach)(phy_temp_info_t *info)
{
	phy_info_t *pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (info == NULL) {
		PHY_INFORM(("%s: null temp module\n", __FUNCTION__));
		return;
	}

	pi = info->pi;

	phy_mfree(pi, info, sizeof(phy_temp_mem_t));
}

/*
 * Query the states pointer.
 */
phy_txcore_temp_t *
phy_temp_get_st(phy_temp_info_t *ti)
{
	return ti->temp;
}

/* temp. throttle */
uint16
phy_temp_throttle(phy_temp_info_t *ti)
{
	phy_type_temp_fns_t *fns = ti->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));
	if (fns->throt == NULL)
		return 0;

	return (fns->throt)(fns->ctx);
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_temp_register_impl)(phy_temp_info_t *ti, phy_type_temp_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));


	*ti->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_temp_unregister_impl)(phy_temp_info_t *ti)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

#ifdef	WL_DYNAMIC_TEMPSENSE
#if defined(BCMDBG)
int
phy_temp_get_override(phy_temp_info_t *ti)
{
	phy_info_t *pi = ti->pi;

	return  pi->tempsense_override;
}
#endif 

int
phy_temp_get_thresh(phy_temp_info_t *ti)
{
	phy_txcore_temp_t *temp = ti->temp;

	return temp->disable_temp;
}

/*
 * Move this to wlc_phy_cmn.c and do it based on PHY
 * This function DOES NOT calculate temperature and it also DOES NOT
 * read any register from MAC and/or RADIO. This function returns
 * last recorded temperature that is stored in phy_info_t.
 * There are few types of phy that DO NOT record last temperature
 * but derives it based on various other mechnism. This function
 * returns BCME_RANGE for such type of PHY. So that the code would
 * continue as per old fashioned tempsense mechanism at every 10S.
 */
int
phy_temp_get_cur_temp(phy_temp_info_t *ti)
{
	phy_txcore_temp_t *temp = ti->temp;
	phy_type_temp_fns_t *fns = ti->fns;
	int ct = BCME_RANGE;

	if (temp->heatedup) {
		return BCME_RANGE; /* Indicate we are already heated up */
	}

	if (fns->upd_gain != NULL)
		ct = (fns->get)(fns->ctx);

	if (ct >= (temp->disable_temp))
		return BCME_RANGE;
	return ct;
}
#endif /* WL_DYNAMIC_TEMPSENSE */

void
wlc_phy_upd_gain_wrt_temp_phy(phy_info_t *pi, int16 *gain_err_temp_adj)
{
	phy_type_temp_fns_t *fns = pi->tempi->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));
	*gain_err_temp_adj = 0;
	if (fns->upd_gain != NULL) {
		(fns->upd_gain)(fns->ctx, gain_err_temp_adj);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}

void
wlc_phy_upd_gain_wrt_gain_cal_temp_phy(phy_info_t *pi, int16 *gain_err_temp_adj)
{
	phy_type_temp_fns_t *fns = pi->tempi->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));
	*gain_err_temp_adj = 0;
	if (fns->upd_gain_cal != NULL) {
		(fns->upd_gain_cal)(fns->ctx, gain_err_temp_adj);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}

/*
 * Read the phy calibration temperature delta parameters from NVRAM.
 */
static void
BCMATTACHFN(wlc_phy_read_tempdelta_settings)(phy_temp_info_t *tempi, int maxtempdelta)
{
	phy_info_t *pi = tempi->pi;
	phy_txcore_temp_t *temp = phy_temp_get_st(tempi);

	/* Read the temperature delta from NVRAM */
	temp->phycal_tempdelta = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_phycal_tempdelta, 0);

	/* Range check, disable if incorrect configuration parameter */
	/* Preserve default, in case someone wants to use it. */
	if (temp->phycal_tempdelta > maxtempdelta) {
		temp->phycal_tempdelta = temp->phycal_tempdelta_default;
	} else {
		temp->phycal_tempdelta_default = temp->phycal_tempdelta;
	}
}

bool
phy_temp_get_tempsense_degree(phy_temp_info_t *tempi, int16 *pval)
{
	phy_type_temp_fns_t *fns = tempi->fns;
	if (fns->do_tempsense)
		*pval = (fns->do_tempsense)(fns->ctx);
	else
		return FALSE;
	return TRUE;
}

#ifdef RADIO_HEALTH_CHECK

#define PHY_INVALID_TEMPERATURE	-128

int
phy_temp_get_cur_temp_radio_health_check(phy_temp_info_t *ti)
{
	phy_type_temp_fns_t *fns = ti->fns;
	phy_info_t *pi = ti->pi;
	int ct = PHY_INVALID_TEMPERATURE;

	if (fns->get != NULL)
		ct = (fns->get)(fns->ctx);

	if (ct >= phy_hc_get_rhc_tempthresh(pi->hci))
		return BCME_RANGE;

	return ct;
}

int
phy_temp_current_temperature_check(phy_temp_info_t *tempi)
{
	int temperature;
	if ((temperature = phy_temp_get_cur_temp_radio_health_check(tempi)) == BCME_RANGE)
		return BCME_RANGE;
	return BCME_OK;
}
#endif /* RADIO_HEALTH_CHECK */
