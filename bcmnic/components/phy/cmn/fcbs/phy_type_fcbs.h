/*
 * FCBS module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_fcbs.h 616484 2016-02-01 17:32:27Z guangjie $
 */

#ifndef _phy_type_fcbs_h_
#define _phy_type_fcbs_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_fcbs.h>

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_fcbs_ctx_t;

typedef bool (*phy_type_fcbs_init_fn_t)(phy_type_fcbs_ctx_t *ctx, int chanidx, chanspec_t chanspec);
typedef int (*phy_type_fcbs_dump_fn_t)(phy_type_fcbs_ctx_t *ctx, struct bcmstrbuf *b);
typedef bool (*phy_type_fcbs_preinit_fn_t)(phy_type_fcbs_ctx_t *ctx, int chanidx);
typedef bool (*phy_type_fcbs_postinit_fn_t)(phy_type_fcbs_ctx_t *ctx, int chanidx);
typedef bool (*phy_type_fcbs_fcbs_fn_t)(phy_type_fcbs_ctx_t *ctx, int chanidx);
typedef bool (*phy_type_fcbs_prefcbs_fn_t)(phy_type_fcbs_ctx_t *ctx, int chanidx);
typedef bool (*phy_type_fcbs_postfcbs_fn_t)(phy_type_fcbs_ctx_t *ctx, int chanidx);
typedef void (*phy_type_fcbs_readtbl_fn_t) (phy_type_fcbs_ctx_t *ctx, uint32 id,
	uint32 len, uint32 offset, uint32 width, void *data);
typedef uint16 (*phy_type_fcbs_channelindicator_obtain_fn_t)(phy_type_fcbs_ctx_t *ctx);
typedef struct {
	phy_type_fcbs_init_fn_t	fcbsinit;
	phy_type_fcbs_preinit_fn_t	prefcbsinit;
	phy_type_fcbs_postinit_fn_t	postfcbsinit;
	phy_type_fcbs_fcbs_fn_t		fcbs;
	phy_type_fcbs_prefcbs_fn_t	prefcbs;
	phy_type_fcbs_postfcbs_fn_t	postfcbs;
	phy_type_fcbs_readtbl_fn_t	fcbsreadtbl;
	phy_type_fcbs_channelindicator_obtain_fn_t channelindicator_obtain;
	phy_type_fcbs_ctx_t *ctx;
} phy_type_fcbs_fns_t;

/*
 * Register/unregister PHY type implementation to the MultiPhaseCal module.
 * It returns BCME_XXXX.
 */
int phy_fcbs_register_impl(phy_fcbs_info_t *cmn_info, phy_type_fcbs_fns_t *fns);
void phy_fcbs_unregister_impl(phy_fcbs_info_t *cmn_info);

#endif /* _phy_type_fcbs_h_ */
