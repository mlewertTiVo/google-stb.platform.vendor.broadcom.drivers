/* hmac_sha256.h
 * Code copied from openssl distribution and
 * Modified just enough so that compiles and runs standalone
 *
 * Copyright (C) 1999-2015, Broadcom Corporation
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
 * $Id: hmac_sha256.h 523133 2014-12-27 05:50:30Z $
 */
/* ====================================================================
 * Copyright (c) 2004 The OpenSSL Project.  All rights reserved
 * according to the OpenSSL license [found in ../../LICENSE].
 * ====================================================================
 */
void hmac_sha256(const void *key, int key_len,
                 const unsigned char *text, size_t text_len,
                 unsigned char *digest,
                 unsigned int *digest_len);
void hmac_sha256_n(const void *key, int key_len,
                   const unsigned char *text, size_t text_len,
                   unsigned char *digest,
                   unsigned int digest_len);
void sha256(const unsigned char *text, size_t text_len, unsigned char *digest,
            unsigned int digest_len);
int
KDF(unsigned char *key, int key_len, const unsigned char *prefix,
              int prefix_len, unsigned char *data, int data_len,
              unsigned char *output, int len);
