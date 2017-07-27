/*
 * passhash.h
 * Perform password to key hash algorithm as defined in WPA and 802.11i
 * specifications.
 *
 * Copyright (C) 1999-2017, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: passhash.h 523133 2014-12-27 05:50:30Z $
 */

#ifndef _PASSHASH_H_
#define _PASSHASH_H_

#include <typedefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* passhash: perform passwork to key hash algorithm as defined in WPA and 802.11i
 * specifications.
 *
 *	password is an ascii string of 8 to 63 characters in length
 *	ssid is up to 32 bytes
 *	ssidlen is the length of ssid in bytes
 *	output must be at lest 40 bytes long, and returns a 256 bit key
 *	returns 0 on success, non-zero on failure
 */
extern int passhash(char *password, int passlen, unsigned char *ssid, int ssidlen,
                              unsigned char *output);

/* init_passhash/do_passhash/get_passhash: perform passwork to key hash algorithm
 * as defined in WPA and 802.11i specifications, and break lengthy calculation into
 * smaller pieces.
 *
 *	password is an ascii string of 8 to 63 characters in length
 *	ssid is up to 32 bytes
 *	ssidlen is the length of ssid in bytes
 *	output must be at lest 40 bytes long, and returns a 256 bit key
 *	returns 0 on success, negative on failure.
 *
 *	Allocate passhash_t and call init_passhash() to initialize it before
 *	calling do_passhash(), and don't release password and ssid until passhash
 *	is done.
 *	Call do_passhash() to request and perform # iterations. do_passhash()
 *	returns positive value to indicate it is in progress, so continue to
 *	call it until it returns 0 which indicates a success.
 *	Call get_passhash() to get the hash value when do_passhash() is done.
 */
#include <bcmcrypto/sha1.h>

typedef struct {
	unsigned char digest[SHA1HashSize];	/* Un-1 */
	int count;				/* Count */
	unsigned char output[2*SHA1HashSize];	/* output */
	char *password;
	int passlen;
	unsigned char *ssid;
	int ssidlen;
	int iters;
} passhash_t;

extern int init_passhash(passhash_t *ph,
                         char *password, int passlen, unsigned char *ssid, int ssidlen);
extern int do_passhash(passhash_t *ph, int iterations);
extern int get_passhash(passhash_t *ph, unsigned char *output, int outlen);

#ifdef __cplusplus
}
#endif
#endif /* _PASSHASH_H_ */
