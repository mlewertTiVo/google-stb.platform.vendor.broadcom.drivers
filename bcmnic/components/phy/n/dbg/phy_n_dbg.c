/*
 * NPHY Debug modules implementation
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
 * $Id: phy_n_dbg.c $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_mem.h>
#include "phy_type_dbg.h"
#include <phy_n.h>
#include <phy_n_dbg.h>
#if defined(BCMDBG)
#include <bcmdevs.h>
#include <wlc_phyreg_n.h>
#include <phy_utils_reg.h>
#endif

/* module private states */
struct phy_n_dbg_info {
	phy_info_t *pi;
	phy_n_info_t *ni;
	phy_dbg_info_t *info;
};

/* local functions */
#if defined(BCMDBG)
static int phy_n_dbg_test_evm(phy_type_dbg_ctx_t *ctx, int channel, uint rate,
	int txpwr);
static int phy_n_dbg_test_carrier_suppress(phy_type_dbg_ctx_t *ctx, int channel);
#else
#define phy_n_dbg_test_evm NULL
#define phy_n_dbg_test_carrier_suppress NULL
#endif 

/* register phy type specific implementation */
phy_n_dbg_info_t *
BCMATTACHFN(phy_n_dbg_register_impl)(phy_info_t *pi, phy_n_info_t *ni, phy_dbg_info_t *info)
{
	phy_n_dbg_info_t *di;
	phy_type_dbg_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((di = phy_malloc(pi, sizeof(phy_n_dbg_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	di->pi = pi;
	di->ni = ni;
	di->info = info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = di;
	fns.test_evm = phy_n_dbg_test_evm;
	fns.test_carrier_suppress = phy_n_dbg_test_carrier_suppress;

	if (phy_dbg_register_impl(info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_dbg_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return di;

	/* error handling */
fail:
	if (di != NULL)
		phy_mfree(pi, di, sizeof(phy_n_dbg_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_n_dbg_unregister_impl)(phy_n_dbg_info_t *di)
{
	phy_info_t *pi;
	phy_dbg_info_t *info;

	ASSERT(di);

	pi = di->pi;
	info = di->info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_dbg_unregister_impl(info);

	phy_mfree(pi, di, sizeof(phy_n_dbg_info_t));
}

#if defined(BCMDBG)
static int
phy_n_dbg_test_evm(phy_type_dbg_ctx_t *ctx, int channel, uint rate, int txpwr)
{
	phy_n_dbg_info_t *di = (phy_n_dbg_info_t *)ctx;
	phy_info_t *pi = di->pi;
	uint16 reg = 0;
	int bcmerror = 0;

	/* stop any test in progress */
	wlc_phy_test_stop(pi);

	/* channel 0 means restore original contents and end the test */
	if (channel == 0) {
		phy_utils_write_phyreg(pi, (NPHY_TO_BPHY_OFF + BPHY_TEST), pi->evm_phytest);

		pi->evm_phytest = 0;

		if (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardflags) & BFL_PACTRL) {
			W_REG(pi->sh->osh, &pi->regs->psm_gpio_out, pi->evm_o);
			W_REG(pi->sh->osh, &pi->regs->psm_gpio_oe, pi->evm_oe);
			OSL_DELAY(1000);
		}
		return 0;
	}

	phy_dbg_test_evm_init(pi);

	if ((bcmerror = wlc_phy_test_init(pi, channel, TRUE)))
		return bcmerror;

	reg = phy_dbg_test_evm_reg(rate);

	PHY_INFORM(("wlc_evm: rate = %d, reg = 0x%x\n", rate, reg));

	/* Save original contents */
	if (pi->evm_phytest == 0) {
		pi->evm_phytest = phy_utils_read_phyreg(pi, (NPHY_TO_BPHY_OFF + BPHY_TEST));
	}

	/* Set EVM test mode */
	phy_utils_and_phyreg(pi, (NPHY_TO_BPHY_OFF + BPHY_TEST),
		~(TST_TXTEST_ENABLE|TST_TXTEST_RATE|TST_TXTEST_PHASE));
	phy_utils_or_phyreg(pi, (NPHY_TO_BPHY_OFF + BPHY_TEST), TST_TXTEST_ENABLE | reg);

	return BCME_OK;
}

static int
phy_n_dbg_test_carrier_suppress(phy_type_dbg_ctx_t *ctx, int channel)
{
	phy_n_dbg_info_t *di = (phy_n_dbg_info_t *)ctx;
	phy_info_t *pi = di->pi;
	int bcmerror = 0;

	/* stop any test in progress */
	wlc_phy_test_stop(pi);

	/* channel 0 means restore original contents and end the test */
	if (channel == 0) {
		phy_utils_write_phyreg(pi, (NPHY_TO_BPHY_OFF + BPHY_TEST), pi->car_sup_phytest);

		pi->car_sup_phytest = 0;
		return 0;
	}

	if ((bcmerror = wlc_phy_test_init(pi, channel, TRUE)))
		return bcmerror;

	/* Save original contents */
	if (pi->car_sup_phytest == 0) {
		pi->car_sup_phytest = phy_utils_read_phyreg(pi, (NPHY_TO_BPHY_OFF + BPHY_TEST));
	}

	/* set carrier suppression test mode */
	PHY_REG_LIST_START
		PHY_REG_AND_RAW_ENTRY(NPHY_TO_BPHY_OFF + BPHY_TEST, 0xfc00)
		PHY_REG_OR_RAW_ENTRY(NPHY_TO_BPHY_OFF + BPHY_TEST, 0x0228)
	PHY_REG_LIST_EXECUTE(pi);

	return BCME_OK;
}
#endif 
