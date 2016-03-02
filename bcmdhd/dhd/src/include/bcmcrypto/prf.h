/*
 * prf.h
 * PRF function used in WPA and TGi
 *
 * Copyright (C) 1999-2016, Broadcom Corporation
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
 * $Id: prf.h 523133 2014-12-27 05:50:30Z $
 */

#ifndef _PRF_H_
#define _PRF_H_

#include <typedefs.h>

/* lengths in Bytes */
#define PRF_MAX_I_D_LEN	128
#define PRF_MAX_KEY_LEN	64
#define PRF_OUTBUF_LEN	80

extern int PRF(unsigned char *key, int key_len, unsigned char *prefix,
                         int prefix_len, unsigned char *data, int data_len,
                         unsigned char *output, int len);

extern int fPRF(unsigned char *key, int key_len, const unsigned char *prefix,
                          int prefix_len, unsigned char *data, int data_len,
                          unsigned char *output, int len);

extern void hmac_sha1(unsigned char *text, int text_len, unsigned char *key,
                                int key_len, unsigned char *digest);

extern void hmac_md5(unsigned char *text, int text_len, unsigned char *key,
                               int key_len, unsigned char *digest);

#endif /* _PRF_H_ */
