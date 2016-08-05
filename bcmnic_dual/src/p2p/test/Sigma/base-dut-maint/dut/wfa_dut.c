/****************************************************************************
 *  (c) Copyright 2007 Wi-Fi Alliance.  All Rights Reserved
 *
 *
 *  LICENSE
 *
 * License is granted only to Wi-Fi Alliance members and designated
 * contractors (“Authorized Licensees”).  Authorized Licensees are granted
 * the non-exclusive, worldwide, limited right to use, copy, import, export
 * and distribute this software:
 * (i) solely for noncommercial applications and solely for testing Wi-Fi
 * equipment; and
 * (ii) solely for the purpose of embedding the software into Authorized
 * Licensee’s proprietary equipment and software products for distribution to
 * its customers under a license with at least the same restrictions as
 * contained in this License, including, without limitation, the disclaimer of
 * warranty and limitation of liability, below.  The distribution rights
 * granted in clause (ii), above, include distribution to third party
 * companies who will redistribute the Authorized Licensee’s product to their
 * customers with or without such third party’s private label. Other than
 * expressly granted herein, this License is not transferable or sublicensable,
 * and it does not extend to and may not be used with non-Wi-Fi applications. 
 * Wi-Fi Alliance reserves all rights not expressly granted herein. 
 *
 * Except as specifically set forth above, commercial derivative works of
 * this software or applications that use the Wi-Fi scripts generated by this
 * software are NOT AUTHORIZED without specific prior written permission from
 * Wi-Fi Alliance. Non-Commercial derivative works of this software for
 * internal use are authorized and are limited by the same restrictions;
 * provided, however, that the Authorized Licensee shall provide Wi-Fi Alliance
 * with a copy of such derivative works under a perpetual, payment-free license
 * to use, modify, and distribute such derivative works for purposes of testing
 * Wi-Fi equipment.
 * Neither the name of the author nor "Wi-Fi Alliance" may be used to endorse
 * or promote products that are derived from or that use this software without
 * specific prior written permission from Wi-Fi Alliance.
 *
 * THIS SOFTWARE IS PROVIDED BY WI-FI ALLIANCE "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE,
 * ARE DISCLAIMED. IN NO EVENT SHALL WI-FI ALLIANCE BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, THE COST OF PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************
 */

/*
 * File: wfa_dut.c - The main program for DUT agent.
 *       This is the top level of traffic control. It initializes a local TCP
 *       socket for command and control link and waits for a connect request
 *       from a Control Agent. Once the the connection is established, it
 *       will process the commands from the Control Agent. For details, please
 *       reference the architecture documents.
 *
 * Revision History:
 *
 *
 */

#include <pthread.h>
#include <signal.h>
#include <time.h>

#include "wfa_portall.h"
#include "wfa_debug.h"
#include "wfa_main.h"
#include "wfa_types.h"
#include "wfa_dut.h"
#include "wfa_sock.h"
#include "wfa_tlv.h"
#include "wfa_tg.h"
#include "wfa_miscs.h"
#include "wfa_agt.h"
#include "wfa_rsp.h"
#include "wfa_wmmps.h"

#include <p2p_app.h>

/* Global flags for synchronizing the TG functions */
int        gtimeOut = 0;        /* timeout value for select call in usec */

#ifdef WFA_WMM_PS_EXT
extern BOOL gtgWmmPS;
extern unsigned long psTxMsg[512];
extern unsigned long psRxMsg[512];
extern wfaWmmPS_t wmmps_info;
extern int  psSockfd;

extern struct apts_msg *apts_msgs;
extern void BUILD_APTS_MSG(int msg, unsigned long *txbuf);
extern int wfaWmmPowerSaveProcess(int sockfd);
extern void wfaSetDUTPwrMgmt(int);
extern void wfaTGSetPrio(int, int);
#endif /* WFA_WMM_PS_EXT */

extern int adj_latency;           /* adjust sleep time due to latency */

char       gnetIf[WFA_BUFF_32];        /* specify the interface to use */

extern BYTE   *trafficBuf, *respBuf;
/* stream table */
extern tgStream_t gStreams[];         /* streams' buffers             */

/* the agent local Socket, Agent Control socket and baseline test socket*/
int   gagtSockfd = -1;
extern int btSockfd;


/* the WMM traffic streams socket fds - Socket Handler table */
extern int tgSockfds[];

extern     xcCommandFuncPtr gWfaCmdFuncTbl[]; /* command process functions */
extern     char gCmdStr[];
extern     tgStream_t *findStreamProfile(int);
extern     int clock_drift_ps;

dutCmdResponse_t gGenericResp;
unsigned short wfa_defined_debug = WFA_DEBUG_ERR | WFA_DEBUG_WARNING | WFA_DEBUG_INFO;
unsigned short dfd_lvl = WFA_DEBUG_DEFAULT | WFA_DEBUG_ERR | WFA_DEBUG_INFO;

/*
 * Thread Synchronize flags
 */
tgWMM_t wmm_thr[WFA_THREADS_NUM];

extern void *wfa_wmm_thread(void *thr_param);
extern void *wfa_wmmps_thread();

extern double gtgPktRTDelay;

int gxcSockfd = -1;

#define DEBUG 0

extern int wfa_estimate_timer_latency();
extern void wfa_dut_init(BYTE **tBuf, BYTE **rBuf, BYTE **paBuf, BYTE **cBuf, struct timeval **timerp);

int
main(int argc, char **argv)
{
    int	      nfds, maxfdn1 = -1, nbytes = 0, cmdLen = 0, isExit = 1;
    int       respLen;
    WORD      locPortNo = 0;   /* local control port number                  */
    fd_set    sockSet;         /* Set of socket descriptors for select()     */
    BYTE      *xcCmdBuf=NULL, *parmsVal=NULL;
    struct timeval *toutvalp=NULL, *tovalp; /* Timeout for select()           */
    WORD      xcCmdTag;
    struct sockfds fds;

    tgThrData_t tdata[WFA_THREADS_NUM];
    int i = 0;
    pthread_attr_t ptAttr;
    int ptPolicy;
    int reset=0;

    struct sched_param ptSchedParam;

    if (argc < 3)              /* Test for correct number of arguments */
    {
        DPRINT_ERR(WFA_ERR, "Usage:  %s <command interface> <Local Control Port> \n", argv[0]);
        exit(1);
    }
#ifdef WFA_PC_CONSOLE
    else if(argc > 3)
    {
                FILE *logfile;
                int fd;
                logfile = fopen(argv[3],"a");
                if(logfile != NULL)
                {
                        fd = fileno(logfile);
                        DPRINT_INFO(WFA_OUT,"redirecting the output to %s\n",argv[3]);
                        dup2(fd,1);
                        dup2(fd,2);
                }
                else
                {
                        DPRINT_ERR(WFA_ERR, "Cant open the log file continuing without redirecting\n");
                }
                printf("Output starts\n");
    }
#endif

    if(isString(argv[1]) == FALSE)
    {
        DPRINT_ERR(WFA_ERR, "incorrect network interface\n");
        exit(1);
    }

    strncpy(gnetIf, argv[1], 31);

    if(isNumber(argv[2]) == FALSE)
    {
        DPRINT_ERR(WFA_ERR, "incorrect port number\n");
        exit(1);
    }

    locPortNo = atoi(argv[2]);

    adj_latency = wfa_estimate_timer_latency() + 4000; /* four more mini */

    /* allocate the traffic stream table */
    wfa_dut_init(&trafficBuf, &respBuf, &parmsVal, &xcCmdBuf, &toutvalp);

    /* 4create listening TCP socket */
    gagtSockfd = wfaCreateTCPServSock(locPortNo);
    if(gagtSockfd == -1)
    {
       DPRINT_ERR(WFA_ERR, "Failed to open socket\n");
       exit(1);
    }

    pthread_attr_init(&ptAttr);

    ptSchedParam.sched_priority = 10;
    pthread_attr_setschedparam(&ptAttr, &ptSchedParam);
    pthread_attr_getschedpolicy(&ptAttr, &ptPolicy);
    pthread_attr_setschedpolicy(&ptAttr, SCHED_RR);
    pthread_attr_getschedpolicy(&ptAttr, &ptPolicy);

    /*
     * Create multiple threads for WMM Stream processing.
     */
    for(i = 0; i< WFA_THREADS_NUM; i++)
    {
        tdata[i].tid = i;
        pthread_mutex_init(&wmm_thr[i].thr_flag_mutex, NULL);
        pthread_cond_init(&wmm_thr[i].thr_flag_cond, NULL);
        wmm_thr[i].thr_id = pthread_create(&wmm_thr[i].thr,
                       &ptAttr, wfa_wmm_thread, &tdata[i]);
    }

    for(i = 0; i < WFA_MAX_TRAFFIC_STREAMS; i++)
       tgSockfds[i] = -1;


    {
        char buf[128];
        char *s;
        //strncpy(buf, "bcmp2papp --noevtloop --pin 12345670 ", sizeof(buf));
        strncpy(buf, "bcmp2papp --noevtloop --add_services 10 ", sizeof(buf));
        FILE *fp;

        s = &buf[strlen(buf)];

        /* Parse p2p_opts.txt for command line options for P2P application. */
        fp = fopen("./p2p_opts.txt", "r");
        if (fp == NULL)
        {
            printf("Using default command line options for bcmp2papp.\n");
        }
        else
        {
            if (fgets(s, sizeof(buf) - strlen(buf), fp) == NULL)
            {
                printf("Error reading contents of p2p_opts.txt\n");
            }
            else
            {
                if (buf[strlen(buf)-1] == '\n')
                {
                    buf[strlen(buf)-1] = '\0';
                }
            }
			fclose(fp);
        }

        /* Init BRCM P2P library. */
        printf("'%s'\n", buf);
        if (0 != bcmp2p_main_str(buf))
		isExit = 0;
    }

    maxfdn1 = gagtSockfd + 1;
    while (isExit) {
        fds.agtfd = &gagtSockfd;
        fds.cafd = &gxcSockfd;
        fds.tgfd = &btSockfd;
        fds.wmmfds = tgSockfds;
#ifdef WFA_WMM_PS_EXT
        fds.psfd = &psSockfd;
#endif

        wfaSetSockFiDesc(&sockSet, &maxfdn1, &fds);

        /*
         * The timer will be set for transaction traffic if no echo is back
         * The timeout from the select call force to send a new packet
         */
        tovalp = NULL;
        if(gtimeOut != 0)
        {
          /* timeout is set to usec */
          tovalp = wfaSetTimer(0, gtimeOut*1000, toutvalp);
        }

        #define STDIN_READ_TIMEOUT_USEC 130000
        toutvalp->tv_sec = 0;
        toutvalp->tv_usec = STDIN_READ_TIMEOUT_USEC;
        tovalp = toutvalp;

        nfds = 0;
	if ( (nfds = select(maxfdn1, &sockSet, NULL, NULL, tovalp)) < 0)
        {
	     if (errno == EINTR)
	  	  continue;		/* back to for() */
	     else
 		  DPRINT_WARNING(WFA_WNG, "select error: %i", errno);
	}

        if(nfds == 0)
        {
#ifdef WFA_WMM_PS_EXT
            /*
             * For WMM-Power Save
             * periodically send HELLO to Console for initial setup.
             */
            if(gtgWmmPS != 0 && psSockfd != -1)
            {
                wfaSetDUTPwrMgmt(0);
                wfaTGSetPrio(psSockfd, 0);
                BUILD_APTS_MSG(APTS_HELLO, psTxMsg);
                wfaTrafficSendTo(psSockfd, (char *)psTxMsg, sizeof(psTxMsg), (struct sockaddr *) &wmmps_info.psToAddr);

                wmmps_info.sta_state = 0;
                wmmps_info.wait_state = WFA_WAIT_STAUT_00;
                continue;
            }
#endif /* WFA_WMM_PS_EXT */

            /* Run P2P event processing state machine. */
            if (bcmp2p_event_process(1) == BCMP2P_ERROR)
            {
                p2papp_shutdown();
                DPRINT_ERR(WFA_ERR, "bcmp2papp exit.\n");
                break;
            }
        }

	if (FD_ISSET(gagtSockfd, &sockSet))
        {
            /* Incoming connection request */
            gxcSockfd = wfaAcceptTCPConn(gagtSockfd);
            if(gxcSockfd == -1)
            {
               DPRINT_ERR(WFA_ERR, "Failed to open control link socket\n");
               exit(1);
            }
	}

        /* Control Link port event*/
        if(gxcSockfd >= 0 && FD_ISSET(gxcSockfd, &sockSet))
        {
            memset(xcCmdBuf, 0, WFA_BUFF_1K);  /* reset the buffer */
            nbytes = wfaCtrlRecv(gxcSockfd, xcCmdBuf);

            if(nbytes <=0)
            {
                /* errors at the port, close it */
                shutdown(gxcSockfd, SHUT_WR);
                close(gxcSockfd);
                gxcSockfd = -1;
            }
            else
            {
               /* command received */
               wfaDecodeTLV(xcCmdBuf, nbytes, &xcCmdTag, &cmdLen, parmsVal);
               p2papi_log(BCMP2P_LOG_MED, 1, "message xcCmdTag=%u\n", xcCmdTag);
			   DPRINT_INFO(WFA_OUT, "message xcCmdTag=%u\n", xcCmdTag);

               memset(respBuf, 0, WFA_RESP_BUF_SZ);
               respLen = 0;

               /* reset two commond storages used by control functions */
               memset(gCmdStr, 0, WFA_CMD_STR_SZ);
               memset(&gGenericResp, 0, sizeof(dutCmdResponse_t));

               /* command process function defined in wfa_ca.c and wfa_tg.c */
			   if (xcCmdTag == WFA_GENERIC_TLV) {
					/* to use single local parser instead of adding a new one for each function in wfa_ca.c */
					reset = WfaGeneric(cmdLen, parmsVal, &respLen, (BYTE *)respBuf);
			   }
               else if((xcCmdTag != 0 && xcCmdTag > WFA_STA_NEW_COMMANDS_START && xcCmdTag < WFA_STA_NEW_COMMANDS_END) &&
	                      gWfaCmdFuncTbl[xcCmdTag - WFA_STA_NEW_COMMANDS_START + (WFA_STA_COMMANDS_END - 1)] != NULL)
               {
	           /* since the new commands are expanded to new block */
			   gWfaCmdFuncTbl[xcCmdTag - WFA_STA_NEW_COMMANDS_START + (WFA_STA_COMMANDS_END - 1)](cmdLen, parmsVal, &respLen, (BYTE *)respBuf);
			   DPRINT_INFO(WFA_OUT, "1message xcCmdTag=%u\n", xcCmdTag);
          }
          else if((xcCmdTag != 0 && xcCmdTag < WFA_STA_COMMANDS_END) && gWfaCmdFuncTbl[xcCmdTag] != NULL)
          {
		       /* commands in the old block */
				gWfaCmdFuncTbl[xcCmdTag](cmdLen, parmsVal, &respLen, (BYTE *)respBuf);
				DPRINT_INFO(WFA_OUT, "2message xcCmdTag=%u\n", xcCmdTag);
     	  }
          else
          {       // no command defined
		        gWfaCmdFuncTbl[0](cmdLen, parmsVal, &respLen, (BYTE *)respBuf);
	            DPRINT_INFO(WFA_OUT, "3message xcCmdTag=%u\n", xcCmdTag);
	       }

               // gWfaCmdFuncTbl[xcCmdTag](cmdLen, parmsVal, &respLen, (BYTE *)respBuf);
           if(gxcSockfd != -1)
                     if(wfaCtrlSend(gxcSockfd, (BYTE *)respBuf, respLen)!=respLen)
                     {
               DPRINT_WARNING(WFA_WNG, "wfa-wfaCtrlSend Error\n");
                }
            }
	}

#ifdef WFA_WMM_PS_EXT
        /*
         * Check if there is from Console
         */
        if(psSockfd != -1 && FD_ISSET(psSockfd, &sockSet))
        {
            wfaWmmPowerSaveProcess(psSockfd);
            continue;
        }
#endif /* WFA_WMM_PS_EXT */

    }

    //DPRINT_ERR(WFA_ERR, "bcmp2papp exit.\n");

        /*
     * necessarily free all mallocs for flat memory real-time systems
         */
    wFREE(trafficBuf);
    wFREE(toutvalp);
    wFREE(respBuf);
    wFREE(xcCmdBuf);
    wFREE(parmsVal);

     /* Close sockets */
    //DPRINT_ERR(WFA_ERR, "bcmp2papp exit.\n");
    wSHUTDOWN(gagtSockfd, SHUT_RDWR);
    wSHUTDOWN(gxcSockfd, SHUT_RDWR);
    wSHUTDOWN(btSockfd, SHUT_RDWR);

    wCLOSE(gagtSockfd);
    wCLOSE(gxcSockfd);
    wCLOSE(btSockfd);

    //DPRINT_ERR(WFA_ERR, "bcmp2papp exit.\n");
    for(i= 0; i< WFA_MAX_TRAFFIC_STREAMS; i++)
    {
       //DPRINT_ERR(WFA_ERR, "bcmp2papp exit.\n");
       if(tgSockfds[i] != -1)
            {
         wCLOSE(tgSockfds[i]);
         tgSockfds[i] = -1;
            }
        }

    return 0;
}
