/*
 * RPC OSL linux port
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: fbsd_rpc_osl.c 467150 Tue Oct 28 04:03:38 PDT 2014 nidalk $
 */
#if !defined(BCM_FD_AGGR)
#error "SPLIT"
#endif

#ifdef BCMDRIVER
#include <sys/param.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/condvar.h>
#include <sys/syscallsubr.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmendian.h>
#include <osl.h>
#include <bcmutils.h>

#include <rpc_osl.h>

struct rpc_osl {
	osl_t      *osh;
	struct cv   rpc_osl_wait;
	int         rpc_osl_wait_chan;
	struct mtx  lock;
	bool        wakeup;
};

rpc_osl_t *
rpc_osl_attach(osl_t *osh)
{
	rpc_osl_t *rpc_osh;

	if ((rpc_osh = (rpc_osl_t *)MALLOC(osh, sizeof(rpc_osl_t))) == NULL)
		return NULL;
	cv_init(&rpc_osh->rpc_osl_wait, "rpc_osl_wait");
	MTX_INIT(&rpc_osh->lock, "rpclock", NULL, MTX_DEF);

	/* set the wakeup flag to TRUE, so we are ready
	   to detect a call return even before we call rpc_osl_wait.
	 */
	rpc_osh->wakeup = TRUE;
	rpc_osh->osh = osh;

	return rpc_osh;
}

void
rpc_osl_detach(rpc_osl_t *rpc_osh)
{
	if (!rpc_osh)
		return;
	cv_destroy(&rpc_osh->rpc_osl_wait);
	MTX_DESTROY(&rpc_osh->lock);
	MFREE(rpc_osh->osh, rpc_osh, sizeof(rpc_osl_t));
}

void
rpc_osl_wake(rpc_osl_t *rpc_osh)
{
	/* Wake up if someone's waiting */
	if (rpc_osh->wakeup == TRUE) {
		/* this is the only place where this flag is set to FALSE
		   It will be reset to TRUE in rpc_osl_wait.
		 */
		rpc_osh->wakeup = FALSE;
		wakeup((void *)&rpc_osh->rpc_osl_wait_chan);
	}
}

int
rpc_osl_wait(rpc_osl_t *rpc_osh, uint ms, bool *ptimedout)
{
	int ret = 0;
	struct timeval tv;

	tv.tv_sec  = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;

	ret = tsleep((void*)&rpc_osh->rpc_osl_wait_chan, 0,
				"rpc_osl_wait", tvtohz(&tv));
	/* 0 ret => timeout */
	if (ret == 0) {
		if (ptimedout)
			*ptimedout = TRUE;
	} else if (ret < 0)
		return ret;

	RPC_OSL_LOCK(rpc_osh);
	/* set the flag to be ready for next return  */
	rpc_osh->wakeup = TRUE;
	RPC_OSL_UNLOCK(rpc_osh);

	return 0;
}

void
rpc_osl_lock(rpc_osl_t *rpc_osh)
{
	MTX_LOCK(&rpc_osh->lock);
}

void
rpc_osl_unlock(rpc_osl_t *rpc_osh)
{
	MTX_UNLOCK(&rpc_osh->lock);
}

#else /* !BCMDRIVER */

#include <typedefs.h>
#include <osl.h>
#include <rpc_osl.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

struct rpc_osl {
	osl_t *osh;
	pthread_cond_t wait; /* To block awaiting init frame */
	pthread_mutex_t	lock;
	bool wakeup;
};

rpc_osl_t *
rpc_osl_attach(osl_t *osh)
{
	rpc_osl_t *rpc_osh;

	if ((rpc_osh = (rpc_osl_t *)MALLOC(osh, sizeof(rpc_osl_t))) == NULL)
		return NULL;

	rpc_osh->osh = osh;
	pthread_mutex_init(&rpc_osh->lock, NULL);
	pthread_cond_init(&rpc_osh->wait, NULL);
	rpc_osh->wakeup = FALSE;

	return rpc_osh;
}

void
rpc_osl_detach(rpc_osl_t *rpc_osh)
{
	if (rpc_osh) {
		MFREE(rpc_osh->osh, rpc_osh, sizeof(rpc_osl_t));
	}
}

int
rpc_osl_wait(rpc_osl_t *rpc_osh, uint ms, bool *ptimedout)
{
	struct timespec timeout;
	int ret;

	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += ms/1000;
	ms = ms - ((ms/1000) * 1000);
	timeout.tv_nsec += ms * 1000 * 1000;

	RPC_OSL_LOCK(rpc_osh);
	rpc_osh->wakeup = FALSE;
	while (rpc_osh->wakeup == FALSE) {
		ret = pthread_cond_timedwait(&rpc_osh->wait, &rpc_osh->lock, &timeout);
		/* check for timedout instead of wait condition */
		if (ret == ETIMEDOUT) {
			if (ptimedout)
				*ptimedout = TRUE;
			rpc_osh->wakeup = TRUE;
		} else if (ret)
			break;	/* some other error (e.g. interrupt) */
	}
	RPC_OSL_UNLOCK(rpc_osh);

	return ret;
}
void
rpc_osl_wake(rpc_osl_t *rpc_osh)
{
	rpc_osh->wakeup = TRUE;
	pthread_cond_signal(&rpc_osh->wait);
}

void
rpc_osl_lock(rpc_osl_t *rpc_osh)
{
	pthread_mutex_lock(&rpc_osh->lock);
}

void
rpc_osl_unlock(rpc_osl_t *rpc_osh)
{
	pthread_mutex_unlock(&rpc_osh->lock);
}

#endif /* BCMDRIVER */
