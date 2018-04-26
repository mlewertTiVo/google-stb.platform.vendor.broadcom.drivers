/*
 * random.c
 * Copyright (C) 2018, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: random.c 523198 2014-12-29 04:39:42Z $
 */
#include <stdio.h>
#if defined(__linux__)
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <fcntl.h>
#include <linux/if_packet.h>
#endif /* __linux__ */

#include <assert.h>
#include <typedefs.h>
#include <bcmcrypto/bn.h>

#if defined(__linux__)
void linux_random(uint8 *rand, int len);
#endif /* __linux__ */

void RAND_bytes(unsigned char *buf, int num)
{
#if defined(__linux__)
	linux_random(buf, num);
#endif /* __linux__ */
}

#if defined(__linux__)
void RAND_linux_init()
{
	BN_register_RAND(linux_random);
}

#ifndef	RANDOM_READ_TRY_MAX
#define RANDOM_READ_TRY_MAX	10
#endif

void linux_random(uint8 *rand, int len)
{
	static int dev_random_fd = -1;
	int status;
	int i;

	if (dev_random_fd == -1)
		dev_random_fd = open("/dev/urandom", O_RDONLY|O_NONBLOCK);

	assert(dev_random_fd != -1);

	for (i = 0; i < RANDOM_READ_TRY_MAX; i++) {
		status = read(dev_random_fd, rand, len);
		if (status == -1) {
			if (errno == EINTR)
				continue;

			assert(status != -1);
		}

		return;
	}

	assert(i != RANDOM_READ_TRY_MAX);
}
#endif /* __linux__ */
