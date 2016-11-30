/*
 * IOCV module interface - iovar command facility.
 * iovar command is in xTLV format.
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
 * $Id: wlc_iocv_cmd.c 631222 2016-04-13 18:51:05Z $
 */

#include <typedefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <wlc_types.h>
#include <wlc_iocv_cmd.h>

/* Process iovar command.
 * The command in xTLV format is in in_buf/in_len.
 * The out_buf and in_buf may point to the same memory.
 */
int
wlc_iocv_iov_cmd_proc(void *ctx, const wlc_iov_cmd_t *tbl, uint sz, bool set,
	void *in_buf, uint in_len, void *out_buf, uint out_len, struct wlc_if *wlcif)
{
	bcm_xtlv_t *cmd = (bcm_xtlv_t *)in_buf;
	uint16 type = cmd->id;
	uint16 len = cmd->len;
	uint i;
	uint hdrlen = BCM_XTLV_HDR_SIZE;
	int err = BCME_NOTFOUND;

	/* Handle a single command for now */

	for (i = 0; i < sz; i ++) {
		if (tbl[i].cmd != type) {
			continue;
		}

		if (in_len - hdrlen < len) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		/* Do some validation... */

		if (tbl[i].hdlr == NULL) {
			err = BCME_ERROR;
			break;
		}

		len = (uint16)out_len;

		return (tbl[i].hdlr)
		        (ctx, (uint8 *)cmd + hdrlen, cmd->len,
		         out_buf, &len, set, wlcif);
	}

	return err;
}
