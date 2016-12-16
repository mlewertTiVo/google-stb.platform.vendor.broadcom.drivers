/*
 * wl btcx command module
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
 * $Id: wluc_btcx.c 458728 2014-02-27 18:15:25Z $
 */

#ifdef WIN32
#include <windows.h>
#endif

#include <wlioctl.h>


/* Because IL_BIGENDIAN was removed there are few warnings that need
 * to be fixed. Windows was not compiled earlier with IL_BIGENDIAN.
 * Hence these warnings were not seen earlier.
 * For now ignore the following warnings
 */
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4761)
#endif

#include <bcmutils.h>
#include <bcmendian.h>
#include <miniopt.h>
#include "wlu_common.h"
#include "wlu.h"

static cmd_func_t wl_btc_profile;

#define WL_BTCPROFILE_USAGE \
"Usage:\n" \
"\t -p profile_index [-e mode_strong_wl_bt_rssi] [-f mode_weak_wl_rssi]" \
" [-g  mode_weak_bt_rssi] [-h mode_weak_wl_bt_rssi]\n" \
"\t [-W mode_wl_hi_lo_rssi_thresh] [-w mode_wl_lo_hi_rssi_thresh]" \
" [-B mode_bt_hi_lo_rssi_thresh] [-b mode_bt_lo_hi_rssi_thresh]\n" \
"\t [-D desense_wl_hi_lo_rssi_thresh] [-d desense_wl_lo_hi_rssi_thresh]\n" \
"\t [-A ack_pwr_wl_hi_lo_rssi_thresh] [-a ack_pwr_wl_lo_hi_rssi_thresh]\n" \
"\t [-T tx_pwr_wl_hi_lo_rssi_thresh] [-t tx_pwr_wl_lo_hi_rssi_thresh]\n" \
"\t [-l desense_level[0] desense_level[1] desense_level[2]] : desense level per chain\n" \
"\t [-X ack_pwr_strong_wl[0] ack_pwr_strong_wl[1] ack_pwr_strong_wl[2]] :" \
" ACK power per chain at strong RSSI\n" \
"\t [-x ack_pwr_weak_wl[0] ack_pwr_weak_wl[1] ack_pwr_weak_wl[2]] :" \
" ACK power per chain at weak RSSI\n" \
"\t [-Y tx_pwr_strong_wl[0] tx_pwr_strong_wl[1] tx_pwr_strong_wl[2]] :" \
" Tx power per chain at strong RSSI\n" \
"\t [-y tx_pwr_weak_wl[0] tx_pwr_weak_wl[1] tx_pwr_weak_wl[2]] :" \
" Tx power per chain at weak RSSI\n\n"

static cmd_t wl_btcx_cmds[] = {
	{ "btc_params", wlu_reg2args, WLC_GET_VAR, WLC_SET_VAR, "g/set BT Coex parameters"},
	{ "btc_flags", wlu_reg2args, WLC_GET_VAR, WLC_SET_VAR, "g/set BT Coex flags"},
	{ "btc_profile", wl_btc_profile, WLC_GET_VAR, WLC_SET_VAR, WL_BTCPROFILE_USAGE},
	{ NULL, NULL, 0, 0, NULL }
};

static char *buf;

/* module initialization */
void
wluc_btcx_module_init(void)
{
	(void)g_swap;

	/* get the global buf */
	buf = wl_get_buf();

	/* register btcx commands */
	wl_module_cmds_register(wl_btcx_cmds);
}

static void
wl_btc_print_profile(wlc_btcx_profile_v1_t *pbtcx_profile)
{
	int i;
	uint8 chain_attr_count = pbtcx_profile->chain_attr_count;
	/* Basic profile information */
	printf("Profile Version: %u\n"
			"Profile Size: %u\n"
			"Profile Initialized: %s\n"
			"Number of Tx chains: %u\n"
			"Profile Index: %u\n"
			"BTC mode under strong WLAN and BT RSSI: %u\n"
			"BTC mode under weak WLAN RSSI: %u\n"
			"BTC mode under weak BT RSSI: %u\n"
			"BTC mode under weak WLAN and BT RSSI: %u\n",
			pbtcx_profile->version,
			pbtcx_profile->length,
			pbtcx_profile->init ? "Yes" : "No",
			pbtcx_profile->chain_attr_count,
			pbtcx_profile->profile_index,
			pbtcx_profile->mode_strong_wl_bt,
			pbtcx_profile->mode_weak_wl,
			pbtcx_profile->mode_weak_bt,
			pbtcx_profile->mode_weak_wl_bt);
	/* Threshold information */
	printf("WLAN strong to weak RSSI threshold for mode change: %d\n"
			"WLAN weak to strong RSSI threshold for mode change: %d\n"
			"BT strong to weak RSSI threshold for mode change: %d\n"
			"BT weak to strong RSSI threshold for mode change: %d\n"
			"WLAN strong to weak RSSI threshold for desense: %d\n"
			"WLAN weak to strong RSSI threshold for desense: %d\n"
			"WLAN strong to weak RSSI threshold for ACK power: %d\n"
			"WLAN weak to strong RSSI threshold for ACK power: %d\n"
			"WLAN strong to weak RSSI threshold for Tx power: %d\n"
			"WLAN weak to strong RSSI threshold for Tx power: %d\n",
			pbtcx_profile->mode_wl_hi_lo_rssi_thresh,
			pbtcx_profile->mode_wl_lo_hi_rssi_thresh,
			pbtcx_profile->mode_bt_hi_lo_rssi_thresh,
			pbtcx_profile->mode_bt_lo_hi_rssi_thresh,
			pbtcx_profile->desense_wl_hi_lo_rssi_thresh,
			pbtcx_profile->desense_wl_lo_hi_rssi_thresh,
			pbtcx_profile->ack_pwr_wl_hi_lo_rssi_thresh,
			pbtcx_profile->ack_pwr_wl_lo_hi_rssi_thresh,
			pbtcx_profile->tx_pwr_wl_hi_lo_rssi_thresh,
			pbtcx_profile->tx_pwr_wl_lo_hi_rssi_thresh);
	/* Desense and Power values per core */
	printf("--- Desense and power limits per core ---\n");
	for (i = 0; i < chain_attr_count; i++) {
		printf("Core %u:\t desense level:%d\t"
				"ACK power strong RSSI:%d\t"
				"ACK power weak RSSI:%d\t"
				"Tx power strong RSSI:%d\t"
				"Tx power weak RSSI:%d\n", i,
				pbtcx_profile->chain_attr[i].desense_level,
				pbtcx_profile->chain_attr[i].ack_pwr_strong_rssi,
				pbtcx_profile->chain_attr[i].ack_pwr_weak_rssi,
				pbtcx_profile->chain_attr[i].tx_pwr_strong_rssi,
				pbtcx_profile->chain_attr[i].tx_pwr_weak_rssi);
	}
}

/* This IOVAR does a read-modify-write of the user specified profile */
/* Since the utility does not know about the number of Tx chains on the device */
/* We use the MAX_UCM_CHAINS to size the variable length array to pull the IO buffer */
/* Once the buffer is modified by the user, the wlu layer passes the buffer back to driver */
/* This buffer is sized by the number of TX chains (chain_attr_count) on the device */
static int
wl_btc_profile(void *wl, cmd_t *cmd, char **argv)
{
	void *ptr = NULL;
	wlc_btcx_profile_v1_t *pbtcx_profile = NULL;
	wlc_btcx_chain_attr_t *chain_attr;
	uint8 chain_attr_count;
	int8 val;
	int err, idx, opt_err, ncon;
	char *endptr = NULL;
	miniopt_t to;
	size_t ucm_prof_sz = sizeof(*pbtcx_profile)
		+ MAX_UCM_CHAINS*sizeof(pbtcx_profile->chain_attr[1]);

	/* discard the command name */
	++argv;
	miniopt_init(&to, __FUNCTION__, NULL, FALSE);

	/* First extract the profile index */
	opt_err = miniopt(&to, argv);
	if (opt_err == 0) {
		if (to.opt == 'p') {
			if (!to.good_int) {
				fprintf(stderr,
					"%s: could not parse \"%s\" as an int for"
					" the profile index\n", __FUNCTION__, to.valstr);
				err = BCME_BADARG;
				goto exit;
			}
			if (to.uval >= MAX_UCM_PROFILES) {
				fprintf(stderr, "Profile selected is out of range.\n");
				err = BCME_RANGE;
				goto exit;
			} else {
				idx = to.uval;
			}
		} else {
			fprintf(stderr, "Invalid first option. "
				"Please select a profile first.\n");
			err = BCME_BADOPTION;
			goto exit;
		}
	} else {
		err = BCME_USAGE_ERROR;
		goto exit;
	}

	/* Get the relevant btc profile */
	if (ucm_prof_sz > WLC_IOCTL_SMLEN) {
		err = BCME_BUFTOOSHORT;
		goto exit;
	}

	if ((err = wlu_var_getbuf_sm(wl, cmd->name, &idx, sizeof(idx), &ptr)) < 0) {
		fprintf(stderr, "%s: getbuf ioctl failed\n", __FUNCTION__);
		err = BCME_ERROR;
		goto exit;
	}

	/* WL utility will pull MAX_UCM_CHAINS size variable array */
	/* however the FW mallocs only chain_attr_count size variable array */
	pbtcx_profile = calloc(1, ucm_prof_sz);
	if (pbtcx_profile == NULL) {
		err = BCME_NOMEM;
		goto exit;
	}
	memcpy(pbtcx_profile, ptr, ucm_prof_sz);

	if (pbtcx_profile->version != UCM_PROFILE_VERSION_1) {
		err = BCME_VERSION;
		goto exit;
	}

	chain_attr_count = pbtcx_profile->chain_attr_count;
	if (chain_attr_count > MAX_UCM_CHAINS) {
		fprintf(stderr, "number of Tx chains (%u) on the device exceeds"
			" the maximum supported chains (%u)\n", chain_attr_count,
			MAX_UCM_CHAINS);
		err = BCME_ERROR;
		goto exit;
	}

	argv += to.consumed;
	chain_attr = pbtcx_profile->chain_attr;

	/* Get: no more arguments */
	if (*argv == NULL) {
		wl_btc_print_profile(pbtcx_profile);
	} else {
	/* Set: more arguments to be parsed */
		while ((opt_err = miniopt(&to, argv)) != -1) {
			if (opt_err == 1) {
				err = BCME_USAGE_ERROR;
				goto exit;
			}

			if (!to.good_int) {
				fprintf(stderr, "could not parse %s as an int for"
					" option -%c\n", to.valstr, to.opt);
				err = BCME_BADARG;
				goto exit;
			}

			switch (to.opt) {
				case 'e' :
					pbtcx_profile->mode_strong_wl_bt = to.val;
					ncon  = to.consumed;
					break;

				case 'f' :
					pbtcx_profile->mode_weak_wl = to.val;
					ncon  = to.consumed;
					break;

				case 'g' :
					pbtcx_profile->mode_weak_bt = to.val;
					ncon  = to.consumed;
					break;

				case 'h' :
					pbtcx_profile->mode_weak_wl_bt = to.val;
					ncon  = to.consumed;
					break;

				case 'W' :
					pbtcx_profile->mode_wl_hi_lo_rssi_thresh = to.val;
					ncon  = to.consumed;
					break;

				case 'w' :
					pbtcx_profile->mode_wl_lo_hi_rssi_thresh = to.val;
					ncon  = to.consumed;
					break;

				case 'B' :
					pbtcx_profile->mode_bt_hi_lo_rssi_thresh = to.val;
					ncon  = to.consumed;
					break;

				case 'b' :
					pbtcx_profile->mode_bt_lo_hi_rssi_thresh = to.val;
					ncon  = to.consumed;
					break;

				case 'D' :
					pbtcx_profile->desense_wl_hi_lo_rssi_thresh = to.val;
					ncon  = to.consumed;
					break;

				case 'd' :
					pbtcx_profile->desense_wl_lo_hi_rssi_thresh = to.val;
					ncon  = to.consumed;
					break;

				case 'A' :
					pbtcx_profile->ack_pwr_wl_hi_lo_rssi_thresh = to.val;
					ncon  = to.consumed;
					break;

				case 'a' :
					pbtcx_profile->ack_pwr_wl_lo_hi_rssi_thresh = to.val;
					ncon  = to.consumed;
					break;

				case 'T' :
					pbtcx_profile->tx_pwr_wl_hi_lo_rssi_thresh = to.val;
					ncon  = to.consumed;
					break;

				case 't' :
					pbtcx_profile->tx_pwr_wl_lo_hi_rssi_thresh = to.val;
					ncon  = to.consumed;
					break;

				case 'l' :
					argv++;
					for (idx = 0; idx < chain_attr_count; idx++) {
						if (*(argv+idx) != NULL) {
							val = (int8)strtoul(*(argv+idx),
								&endptr, 0);
							if (*endptr != '\0') {
								err = BCME_BADOPTION;
								goto exit;
							}
							chain_attr[idx].desense_level = val;
						} else {
							err = BCME_BADLEN;
							goto exit;
						}
					}
					ncon = idx;
					break;

				case 'X' :
					argv++;
					for (idx = 0; idx < chain_attr_count; idx++) {
						if (*(argv+idx) != NULL) {
							val = (int8)strtoul(*(argv+idx),
								&endptr, 0);
							if (*endptr != '\0') {
								err = BCME_BADOPTION;
								goto exit;
							}
							chain_attr[idx].ack_pwr_strong_rssi = val;
						} else {
							err = BCME_BADLEN;
							goto exit;
						}
					}
					ncon = idx;
					break;

				case 'x' :
					argv++;
					for (idx = 0; idx < chain_attr_count; idx++) {
						if (*(argv+idx) != NULL) {
							val = (int8)strtoul(*(argv+idx),
								&endptr, 0);
							if (*endptr != '\0') {
								err = BCME_BADOPTION;
								goto exit;
							}
							chain_attr[idx].ack_pwr_weak_rssi = val;
						} else {
							err = BCME_BADLEN;
							goto exit;
						}
					}
					ncon = idx;
					break;

				case 'Y' :
					argv++;
					for (idx = 0; idx < chain_attr_count; idx++) {
						if (*(argv+idx) != NULL) {
							val = (int8)strtoul(*(argv+idx),
								&endptr, 0);
							if (*endptr != '\0') {
								err = BCME_BADOPTION;
								goto exit;
							}
							chain_attr[idx].tx_pwr_strong_rssi = val;
						} else {
							err = BCME_BADLEN;
							goto exit;
						}
					}
					ncon = idx;
					break;

				case 'y' :
					argv++;
					for (idx = 0; idx < chain_attr_count; idx++) {
						if (*(argv+idx) != NULL) {
							val = (int8)strtoul(*(argv+idx),
								&endptr, 0);
							if (*endptr != '\0') {
								err = BCME_BADOPTION;
								goto exit;
							}
							chain_attr[idx].tx_pwr_weak_rssi = val;
						} else {
							err = BCME_BADLEN;
							goto exit;
						}
					}
					ncon = idx;
					break;

				default :
					fprintf(stderr, "Invalid option %c\n", to.opt);
			}
			argv += ncon;
		}

		/* write back using var length array size of pbtcx_profile->chain_attr_count */
		if ((err = wlu_var_setbuf(wl, cmd->name, pbtcx_profile, sizeof(*pbtcx_profile)
			+ chain_attr_count*sizeof(pbtcx_profile->chain_attr[1])) < 0)) {
			fprintf(stderr, "%s: fail to set %d\n", __FUNCTION__, err);
			err = BCME_ERROR;
			goto exit;
		}
	}
	err = BCME_OK;

exit:
	if (pbtcx_profile != NULL)
		free(pbtcx_profile);
	return err;
}
