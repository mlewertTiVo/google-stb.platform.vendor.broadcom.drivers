/**
 * ACSD include file for config as done by NVRAM (used only for media build)
 *
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
 * $Id: acsd_config.h 597371 2015-11-05 02:05:29Z $
 *
 */

#ifndef _acsconfig_h_
#define _acsconfig_h_

#define TRUE 1
#define FALSE 0
#define CONFIG_FILE_LINE_LENGTH 80
#define HASH_MAX_ENTRY_IN_POWER_OF_2 6
#define HASH_MAX_ENTRY (1 << HASH_MAX_ENTRY_IN_POWER_OF_2)
#define HASH_MAX_ENTRY_MASK (HASH_MAX_ENTRY - 1)

char* acsd_config_safe_get(const char *name);
char* acsd_config_get(const char *name);
int acsd_config_match(const char *name, const char *match);
void acsd_free_mem();
int acsd_config_unset(const char *name);

struct acsd_hashtable_s {
	 int size;
	 struct entry_s **table;
};
typedef struct acsd_hashtable_s acsd_hashtable_t;

#ifdef ACSD_CONFIG_INIT_DEBUG
#define ACSD_CONF_INIT_PRINT ACSD_PRINT
#else
#define ACSD_CONF_INIT_PRINT(fmt, arg...)
#endif

#endif /* _acsconfig_h_ */
