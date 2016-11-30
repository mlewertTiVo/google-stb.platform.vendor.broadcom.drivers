# Helper makefile for building Broadcom wl device driver
# This file maps wl driver feature flags (import) to WLFLAGS and WLFILES_SRC (export).
#
# Copyright (C) 2016, Broadcom. All Rights Reserved.
# 
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
#
# <<Broadcom-WL-IPTag/Open:>>
#
# $Id: wl.mk 655293 2016-08-18 20:59:38Z $


ifdef UCODE_RAM
UCODE_RAM_DIR = components/ucode/dot11_releases/trunk/$(UCODE_RAM)/ram
else
UCODE_RAM_DIR = src/wl/sys
endif

ifeq ($(UCODE_IN_ROM),1)
UCODE_ROM_DIR = components/ucode/dot11_releases/trunk/$(UCODE_ROM)/rom
endif


ifeq ($(NO_BCMDBG_ASSERT), 1)
	WLFLAGS += -DNO_BCMDBG_ASSERT
endif

# debug/internal
ifeq ($(DEBUG),1)
	WLFLAGS += -DBCMDBG -DWLTEST -DRWL_WIFI -DWIFI_ACT_FRAME -DWLRWL -DWL_EXPORT_CURPOWER
	WLRWL = 1
else ifeq ($(WLDEBUG),1)
	BCMUTILS = 1
	OSLLX = 1
	WLFLAGS += -DBCMDBG -DWLTEST -DWIFI_ACT_FRAME -DWL_EXPORT_CURPOWER
else
endif

#ifdef BCMWDF
ifeq ($(BCMWDF),1)
	WLFLAGS += -DBCMWDF
endif
#endif


ifeq ($(WLAIRDBG),1)
	WLFLAGS += -DWLAIRDBG
	WLFILES_SRC += src/wl/sys/wlc_airdbg.c
endif

# hotspot AP
ifeq ($(HSPOT),1)
	WLBSSLOAD = 1
	L2_FILTER = 1
	WLDLS = 1
	WLWNM = 1
	WIFI_ACT_FRAME = 1
	WL11U = 1
	WLPROBRESP_SW = 1
	WLOSEN = 1
endif

WLFLAGS += -DPPR_API

ifdef BCMPHYCORENUM
	WLFLAGS += -DBCMPHYCORENUM=$(BCMPHYCORENUM)
endif

ifeq ($(WLATF),1)
	WLATF := 1
	WLFLAGS += -DWLATF
endif

ifeq ($(WLATF_DONGLE),1)
	WLATF := 1
	WLFLAGS += -DWLATF_DONGLE
endif



#ifdef BCM_DMA_CT
ifeq ($(BCM_DMA_CT),1)
	WLFLAGS += -DBCM_DMA_CT
endif
#endif

#ifdef BCM_DMA_CT_DISABLED
ifeq ($(BCM_DMA_CT_DISABLED),1)
	WLFLAGS += -DBCM_DMA_CT_DISABLED
endif
#endif

#ifdef BCM_DMA_INDIRECT
ifeq ($(BCM_DMA_INDIRECT),1)
	WLFLAGS += -DBCM_DMA_INDIRECT
endif
#endif

ifeq ($(DBG_HEAPCHECK),1)
	WLFLAGS += -DBCMDBG_HEAPCHECK
endif

#ifdef BCMSPLITRX
ifeq ($(BCMSPLITRX),1)
	WLFILES_SRC	+= src/wl/sys/wlc_pktfetch.c
endif
#endif

#ifdef BCMDBG_TXSTUCK
ifeq ($(BCMDBG_TXSTUCK),1)
	# Debug for: wl dump txstuck;
        WLFLAGS += -DBCMDBG_TXSTUCK
endif
#endif

#ifdef PKTQ_LOG
ifeq ($(PKTQ_LOG),1)
	WLFLAGS += -DPKTQ_LOG
	ifeq ($(SCB_BS_DATA),1)
		WLFILES_SRC += src/wl/sys/wlc_bs_data.c
		WLFLAGS += -DSCB_BS_DATA
	endif
endif
#endif

ifeq ($(PSPRETEND),1)
	WLFLAGS += -DPSPRETEND
	WLFLAGS += -DWL_CS_PKTRETRY
	WLFLAGS += -DWL_CS_RESTRICT_RELEASE
	WLFILES_SRC += src/wl/sys/wlc_pspretend.c
	WLFILES_SRC += src/wl/sys/wlc_csrestrict.c
endif


## wl driver common

ifeq ($(GTK_RESET),1)
	WLFLAGS += -DGTK_RESET
endif

ifeq ($(USBAP),1)
	WLFLAGS += -DUSBAP
	WLFLAGS += -DEHCI_FASTPATH_TX -DEHCI_FASTPATH_RX
endif

ifeq ($(PROPRIETARY_11N_RATES),1)
	WLFLAGS += -DWLPROPRIETARY_11N_RATES
endif

ifeq ($(WLRSDB),1)
	WLFLAGS += -DWLRSDB -DSHARED_OSL_CMN
	WL_OBJ_REGISTRY := 1
	ifeq ($(WL_RSDB_DISABLED),1)
		WLFLAGS += -DWLRSDB_DISABLED -DNUMD11CORES=1 -DRSDB_MODE_MIMO
	endif
	# RSDB Needs multiqueue for txfifo sync
	ifndef WLMULTIQUEUE
		WLMULTIQUEUE := 1
	endif
	ifeq ($(WLRAPSTA),1)
		WLFLAGS += -DWL_RESTRICTED_APSTA
		ifeq ($(WLRAPSTA_DISABLED),1)
			WLFLAGS += -DWL_RESTRICTED_APSTA_DISABLED
		endif
	endif
	ifeq ($(RSDB_POLICY_MGR),1)
		WLFLAGS += -DWLRSDB_POLICY_MGR
		WLFILES_SRC += src/wl/sys/wlc_rsdb_policymgr.c
	endif
	ifeq ($(RSDB_POLICY_MGR_DISABLED),1)
		WLFLAGS += -DWLRSDB_POLICY_MGR_DISABLED
	endif
	ifeq ($(BCMDBG_RSDB),1)
		WLFLAGS += -DBCMDBG_RSDB
	endif
endif
ifeq ($(WL_OBJ_REGISTRY),1)
	WLFLAGS += -DWL_OBJ_REGISTRY
	ifeq ($(WL_NO_BSSCFG_SHARE),1)
		WLFLAGS += -DWL_NO_BSSCFG_SHARE
	endif
endif

#ifdef WL
ifeq ($(WL),1)
	WLFILES_SRC += src/shared/bcmwifi/src/bcmwifi_channels.c
	WLFILES_SRC += src/shared/bcmevent.c
	WLFILES_SRC += src/shared/bcm_mpool.c
	WLFILES_SRC += src/shared/bcm_notif.c
	WLFILES_SRC += src/shared/bcm_objregistry.c
	WLFILES_SRC += src/shared/bcmiov.c
	WLFILES_SRC += src/wl/sys/wlc_alloc.c
	WLFILES_SRC += src/wl/sys/wlc_intr.c
	WLFILES_SRC += src/wl/sys/wlc_hw.c
	WLFILES_SRC += $(D11SHMCDIR)/d11shm.c
	ifeq ($(WLPFN),1)
		WLFLAGS += -DWLPFN
		WLFILES_SRC += src/wl/sys/wl_pfn.c
	endif
	WLFILES_SRC += src/shared/qmath.c
	WLFILES_SRC += $(UCODE_RAM_DIR)/d11ucode_gt15.c
	WLFILES_SRC += $(UCODE_RAM_DIR)/d11ucode_ge24.c
	WLFILES_SRC += src/wl/sys/d11reglist.c
	WLFILES_SRC += src/wl/ppr/src/wlc_ppr.c
	WLFILES_SRC += src/wl/sys/wlc_phy_shim.c
	WLFILES_SRC += src/wl/sys/wlc_bmac.c
	WLFILES_SRC += src/wl/sys/wlc_ucinit.c
	WLFILES_SRC += src/wl/sys/wlc_rate_def.c
	WLFILES_SRC += src/wl/sys/wlc_vasip.c

#ifdef BCM_DMA_CT
	ifeq ($(BCM_DMA_CT),1)
		WLFILES_SRC += $(UCODE_RAM_DIR)/d11ucode_mu.c
	endif
#endif

	ifneq ($(MINIAP),1)
		WLFILES_SRC += $(UCODE_RAM_DIR)/d11ucode_ge40.c
		WLFILES_SRC += src/wl/sys/d11vasip_code.c
	endif

	WLFILES_SRC += src/wl/sys/wlc.c
	WLFILES_SRC += src/wl/sys/wlc_tx.c
	WLFILES_SRC += src/wl/sys/wlc_txmod.c
	WLFILES_SRC += src/wl/sys/wlc_rx.c
	WLFILES_SRC += src/wl/sys/wlc_frag.c
	WLFILES_SRC += src/wl/sys/wlc_cubby.c
	WLFILES_SRC += src/wl/sys/wlc_qoscfg.c
	WLFILES_SRC += src/wl/sys/wlc_flow_ctx.c
	ifeq ($(ATE),1)
		WLFILES_SRC += src/wl/ate/src/wlu_ate.c
		WLFILES_SRC += src/shared/miniopt.c
		WLFILES_SRC += src/wl/sys/wl_ate.c
	endif
	WLFILES_SRC += src/wl/sys/wlc_dbg.c
	WLFILES_SRC += src/wl/sys/wlc_dump.c
	WLFILES_SRC += src/wl/sys/wlc_macdbg.c
	WLFILES_SRC += src/wl/sys/wlc_txtime.c
	WLFILES_SRC += src/wl/sys/wlc_mux.c
	WLFILES_SRC += src/wl/sys/wlc_mux_utils.c
	WLFILES_SRC += src/wl/sys/wlc_scbq.c
	WLFILES_SRC += src/wl/sys/wlc_ie_misc_hndlrs.c
	WLFILES_SRC += src/wl/sys/wlc_addrmatch.c
	WLFILES_SRC += src/wl/sys/wlc_utils.c
	WLFILES_SRC += src/wl/sys/wlc_prot.c
	WLFILES_SRC += src/wl/sys/wlc_prot_g.c
	WLFILES_SRC += src/wl/sys/wlc_prot_n.c
	WLFILES_SRC += src/wl/sys/wlc_assoc.c
	WLFILES_SRC += src/wl/sys/wlc_txc.c
	WLFILES_SRC += src/wl/sys/wlc_pcb.c
	WLFILES_SRC += src/wl/sys/wlc_rate.c
	WLFILES_SRC += src/wl/sys/wlc_rate_def.c
	WLFILES_SRC += src/wl/sys/wlc_stf.c
	WLFILES_SRC += src/wl/sys/wlc_lq.c
	WLFILES_SRC += src/wl/sys/wlc_log.c
	ifeq ($(SRHWVSDB),1)
		WLFILES_SRC += src/wl/sys/wlc_srvsdb.c
	endif
	ifeq ($(WL_PROT_OBSS),1)
		WLFLAGS += -DWL_PROT_OBSS
		WLFILES_SRC += src/wl/sys/wlc_obss_util.c
		WLFILES_SRC += src/wl/sys/wlc_prot_obss.c
	endif

	ifeq ($(WL_OBSS_DYNBW),1)
		WL_MODESW = 1
		WLFLAGS += -DWL_OBSS_DYNBW
		WLFILES_SRC += src/wl/sys/wlc_obss_util.c
		WLFILES_SRC += src/wl/sys/wlc_obss_dynbw.c
	endif

	ifeq ($(WL_BCNTRIM),1)
		WLFLAGS += -DWL_BCNTRIM
		WLFILES_SRC += src/wl/sys/wlc_bcntrim.c
	endif
	WLFILES_SRC += src/wl/sys/wlc_pm.c
	WLFILES_SRC += src/wl/sys/wlc_btcx.c
	WLFILES_SRC += src/wl/sys/wlc_stamon.c
	WLFILES_SRC += src/wl/sys/wlc_monitor.c
ifeq ($(WLNDIS),1)
	ifeq ($(WLXP), 1)
		WLFLAGS += -DWLC_NDIS5IOVAR
	endif
	WLFILES_SRC += src/wl/sys/wlc_ndis_iovar.c
endif
	ifneq ($(WLWSEC),0)
		WLFLAGS += -DWLWSEC
	endif
	WLFILES_SRC += src/wl/sys/wlc_scb.c
	WLFILES_SRC += src/wl/sys/wlc_rate_sel.c
	WLFILES_SRC += src/wl/sys/wlc_scb_ratesel.c
	WLFILES_SRC += src/wl/sys/wlc_macfltr.c
	ifeq ($(WL_PROXDETECT),1)
		WLFLAGS += -DWL_PROXDETECT
		ifeq ($(WLDEBUG),1)
			WLFLAGS += -DTOF_DEBUG -DTOF_DEBUG_TIME
		endif
		ifeq ($(TOF_DBG),1)
			WLFLAGS += -DTOF_DBG
		endif
		WLFILES_PROXD = wlc_pdsvc.c wlc_tof.c wlc_fft.c
		ifeq ($(WL_TOF),1)
			WLFLAGS += -DWL_TOF
			WLFILES_PROXD += wlc_pdtof.c
		ifeq ($(WL_NAN),1)
				WLFILES_PROXD += wlc_pdnan.c
			endif
		endif
		ifeq ($(WL_FTM), 1)
			WLFLAGS += -DWL_FTM -DWL_FTM_MSCH
			ifeq ($(WL_FTM_11K),1)
				WLFLAGS += -DWL_FTM_11K
			endif
			ifeq ($(WL_FTM_TSF_SYNC),1)
				WLFLAGS += -DWL_FTM_TSF_SYNC
			endif

            WLFILES_FTM_BCMCRYPTO = hmac_sha256.c sha256.c
			WLFILES_PROXD += pdftm.c pdftmevt.c pdftmiov.c pdftmproto.c \
				pdftmsn.c pdftmsched.c pdburst.c pdftmutil.c pdftmdbg.c \
				pdftmvs.c pdftmrange.c pdftmmsch.c \
				pdftmsec.c
			ifeq ($(WL_NAN),1)
				WLFILES_PROXD += pdftmnan.c
			endif
		endif
		WLFILES_SRC += $(addprefix src/wl/proxd/src/, $(WLFILES_PROXD))
		WLFILES_SRC += $(addprefix src/bcmcrypto/, $(WLFILES_FTM_BCMCRYPTO))
	endif
	ifeq ($(WL_NAN),1)
		WLFLAGS += -DWL_NAN
		ifeq ($(WL_NAN_PD_P2P),1)
			WLFLAGS += -DWL_NAN_PD_P2P
		endif
		WLFILES_SRC += src/shared/bcmbloom.c
		WLFILES_SRC += src/wl/nan/src/nan_cmn.c
		WLFILES_SRC += src/wl/nan/src/nan_iov.c
		WLFILES_SRC += src/wl/nan/src/nan_mac.c
		ifeq ($(WL_NAN_DISC), 1)
			WLFILES_SRC += src/wl/nan/src/nan_disc.c
			WLFLAGS += -DWL_NAN_DISC
		endif
		WLFILES_SRC += src/wl/nan/src/nan_dbg.c
		WLFILES_SRC += src/wl/sys/wlc_tsmap.c
		WLFILES_SRC += src/wl/nan/src/nan_fam.c
		WLFILES_SRC += src/wl/nan/src/nan_sched.c
		WLFILES_SRC += src/wl/nan/src/nan_data.c
		WLFILES_SRC += src/wl/nan/src/nan_mgmt.c
		WLFILES_SRC += src/wl/nan/src/nan_ndp.c
		WLFILES_SRC += src/wl/nan/src/nan_ndl.c
		WLFILES_SRC += src/wl/nan/src/nan_util.c
	endif
ifeq ($(WL_RANDMAC),1)
	WLFLAGS += -DWL_RANDMAC
	WLFILES_SRC += src/wl/randmac/src/wlc_randmac.c
endif
#ifdef WL_LPC
	ifeq ($(WL_LPC),1)
		WLFLAGS += -DWL_LPC
		WLFILES_SRC += src/wl/sys/wlc_power_sel.c
		WLFILES_SRC += src/wl/sys/wlc_scb_powersel.c
	else
		ifeq ($(LP_P2P_SOFTAP),1)
			WLFLAGS += -DLP_P2P_SOFTAP
		endif
	endif
#ifdef WL_LPC_DEBUG
	ifeq ($(WL_LPC_DEBUG),1)
		WLFLAGS += -DWL_LPC_DEBUG
	endif
#endif
#endif
	ifeq ($(WL_RELMCAST),1)
		WLFLAGS += -DWL_RELMCAST -DIBSS_RMC
		WLFILES_SRC += src/wl/rel_mcast/src/wlc_relmcast.c
	endif
	WLFILES_SRC += src/wl/sys/wlc_antsel.c
	WLFILES_SRC += src/wl/sys/wlc_bsscfg.c
	WLFILES_SRC += src/wl/sys/wlc_bsscfg_psq.c
	WLFILES_SRC += src/wl/sys/wlc_bsscfg_viel.c
	WLFILES_SRC += src/wl/sys/wlc_vndr_ie_list.c
	WLFILES_SRC += src/wl/sys/wlc_scan.c
	WLFILES_SRC += src/wl/sys/wlc_scan_utils.c
	WLFILES_SRC += src/wl/sys/wlc_msch.c
	WLFILES_SRC += src/wl/sys/wlc_mschutil.c
	WLFILES_SRC += src/wl/sys/wlc_chanctxt.c
	WLFILES_SRC += src/wl/sys/wlc_rm.c
	WLFILES_SRC += src/wl/sys/wlc_tso.c
	WLFILES_SRC += src/wl/sys/wlc_pmkid.c
	WLFILES_SRC += src/wl/sys/wlc_pktc.c
	WLFILES_SRC += src/wl/sys/wlc_rspec.c
	WLFILES_SRC += src/wl/sys/wlc_perf_utils.c
	WLFILES_SRC += src/wl/sys/wlc_act_frame.c
	ifeq ($(WL11N),1)
		WLFLAGS += -DWL11N
		WLFILES_SRC += src/wl/sys/wlc_ht.c
		ifeq ($(WL11N_20MHZONLY),1)
			WLFLAGS += -DWL11N_20MHZONLY
		endif
		ifeq ($(WL11N_SINGLESTREAM),1)
			WLFLAGS += -DWL11N_SINGLESTREAM
		endif
	endif
	ifeq ($(WL11AC),1)
		WLFLAGS += -DWL11AC
		WLFILES_SRC += src/wl/sys/wlc_vht.c
		ifeq ($(WL11AC_160),1)
			WLFLAGS += -DWL11AC_160
		endif
		ifeq ($(WL11AC_80P80),1)
			WLFLAGS += -DWL11AC_80P80
		endif
		WL_MODESW ?= 1
		ifeq ($(WLTXBF),1)
			WLFLAGS += -DWL_BEAMFORMING
			WLFILES_SRC += src/wl/sys/wlc_txbf.c
			ifeq ($(TXBF_MORE_LINKS),1)
				WLFLAGS += -DTXBF_MORE_LINKS
			endif
		endif
		ifeq ($(WLOLPC),1)
			WLFLAGS += -DWLOLPC
			WLFILES_SRC += src/wl/olpc/src/wlc_olpc_engine.c
		endif
	endif
	ifeq ($(WL11AX),1)
		WLFLAGS += -DWL11AX
		ifeq ($(HE_MAC_SW_DEV),1)
			WLFLAGS += -DHE_MAC_SW_DEV
		endif
		WLTWT ?= 1
		WLFILES_SRC += src/wl/sys/wlc_he.c
	endif
	ifeq ($(WLTWT),1)
		WLFLAGS += -DWLTWT
		WLFILES_SRC += src/wl/sys/wlc_twt.c
	endif
	ifeq ($(WL_MODESW),1)
		WLFLAGS += -DWL_MODESW
		WLFILES_SRC += src/wl/sys/wlc_modesw.c
		WLFLAGS += -DPHYCAL_CACHING
		ifeq ($(DEBUG),1)
			WLFLAGS += -DWL_MODESW_TIMECAL
		endif
	endif
	ifeq ($(WL_MU_RX),1)
		WLFLAGS += -DWL_MU_RX
		WLFILES_SRC += src/wl/sys/wlc_murx.c
	endif
		WLFILES_SRC += src/wl/sys/wlc_airtime.c
#ifdef WL11H
	ifeq ($(WL11H),1)
		WLFLAGS += -DWL11H
		WLFILES_SRC += src/wl/sys/wlc_11h.c
		WLFLAGS += -DWLCSA
		WLFILES_SRC += src/wl/sys/wlc_csa.c
		WLFLAGS += -DWLQUIET
		WLFILES_SRC += src/wl/sys/wlc_quiet.c
	endif
#endif /* WL11H */

#ifdef WL_PM_MUTE_TX
	ifeq ($(WL_PM_MUTE_TX),1)
		WLFLAGS += -DWL_PM_MUTE_TX
		WLFILES_SRC += src/wl/sys/wlc_pm_mute_tx.c
	endif
#endif /* WL_PM_MUTE_TX */

#ifdef WL_FRAGDUR
	ifeq ($(WL_FRAGDUR),1)
		WLFLAGS += -DWL_FRAGDUR
		WLFILES_SRC += src/wl/sys/wlc_fragdur.c
	endif
#endif /* WL_FRAGDUR */

	# tpc module is shared by 11h tpc and wl tx power control */
	WLTPC ?= 1
	ifeq ($(WLTPC),1)
		WLFLAGS += -DWLTPC
		WLFILES_SRC += src/wl/sys/wlc_tpc.c
#ifdef WL_AP_TPC
		ifeq ($(WL_AP_TPC),1)
			WLFLAGS += -DWL_AP_TPC
		endif
#endif
		ifeq ($(WL_CHANSPEC_TXPWR_MAX),1)
			WLFLAGS += -DWL_CHANSPEC_TXPWR_MAX
		endif
	endif
	ifeq ($(WLC_DISABLE_DFS_RADAR_SUPPORT),1)
		WLFLAGS += -DWLC_DISABLE_DFS_RADAR_SUPPORT
	else
		ifeq ($(DBAND),1)
			WLFILES_SRC += src/wl/sys/wlc_dfs.c
			ifeq ($(BGDFS),1)
				WLFLAGS += -DBGDFS
			endif
		endif
	endif
#ifdef WL11D
	ifeq ($(WL11D),1)
		WLFLAGS += -DWL11D
		WLFILES_SRC += src/wl/sys/wlc_11d.c
	endif
#endif /* WL11D */
	# cntry module is shared by 11h/11d and wl channel */
	WLCNTRY := 1
	ifeq ($(WLCNTRY),1)
		WLFLAGS += -DWLCNTRY
		WLFILES_SRC += src/wl/sys/wlc_cntry.c
	endif
	WLFILES_SRC += src/wl/sys/wlc_event.c
	WLFILES_SRC += src/wl/sys/wlc_event_utils.c
	WLFILES_SRC += src/wl/sys/wlc_msfblob.c
	WLFILES_SRC += src/wl/sys/wlc_channel.c
#ifdef WL_SARLIMIT
	ifeq ($(WL_SARLIMIT),1)
		# Note that wlc_sar_tbl.c is dynamically generated. Its whereabouts
		# shall be assigned to WLC_SAR_TBL_DIR, its generation recipe shall
		# established by WLAN_GenSarTblRule macro, defined in WLAN_Common.mk
		WLFILES_SRC += $(WLC_SAR_TBL_DIR)/wlc_sar_tbl.c
	endif
#endif
	WLFILES_SRC += components/clm-api/src/wlc_clm.c
	WLFILES_SRC += components/clm-api/src/wlc_clm_data.c
#ifdef WLC_TXCAL
	ifeq ($(WLC_TXCAL),1)
		WLFLAGS += -DWLC_TXCAL
		WLFILES_SRC += src/wl/sys/wlc_calload.c
	endif
#endif
	WLFILES_SRC += src/shared/bcmwpa.c
#ifndef LINUX_CRYPTO
	ifneq ($(LINUX_CRYPTO),1)
		WLFILES_SRC += src/bcmcrypto/rc4.c
		WLFILES_SRC += src/bcmcrypto/tkhash.c
		WLFILES_SRC += src/bcmcrypto/tkmic.c
		WLFILES_SRC += src/bcmcrypto/wep.c
	endif
#endif /* LINUX_CRYPTO */
#ifdef WLEXTLOG
ifeq ($(WLEXTLOG),1)
	WLFLAGS += -DWLEXTLOG
	WLFILES_SRC += src/wl/sys/wlc_extlog.c
endif
#endif
#ifdef WLSCANCACHE
ifeq ($(WLSCANCACHE),1)
	WLFLAGS += -DWLSCANCACHE
	WLFILES_SRC += src/wl/sys/wlc_scandb.c
endif
#endif
	WLFILES_SRC += src/wl/sys/wlc_hrt.c
	WLFILES_SRC += src/wl/sys/wlc_ie_helper.c
	WLFILES_SRC += src/wl/sys/wlc_ie_mgmt_lib.c
	WLFILES_SRC += src/wl/sys/wlc_ie_mgmt_vs.c
	WLFILES_SRC += src/wl/sys/wlc_ie_mgmt.c
	WLFILES_SRC += src/wl/sys/wlc_ie_reg.c
ifeq ($(IEM_TEST),1)
	WLFLAGS += -DIEM_TEST
	WLFILES_SRC += src/wl/sys/wlc_ie_mgmt_test.c
endif
	WLFILES_SRC += src/wl/sys/wlc_akm_ie.c
	WLFILES_SRC += src/wl/sys/wlc_obss.c
	WLFILES_SRC += src/wl/sys/wlc_hs20.c
	WLFILES_SRC += src/wl/sys/wlc_iocv.c
	#iovar/ioctl registration
	WLFILES_SRC += src/wl/iocv/src/wlc_iocv_reg.c
	WLFILES_SRC += src/wl/iocv/src/wlc_iocv_low.c
	WLFILES_SRC += src/wl/iocv/src/wlc_iocv_high.c
	WLFILES_SRC += src/wl/iocv/src/wlc_iocv_cmd.c
	#named dump callback registration
	WLFILES_SRC += src/wl/dump/src/wlc_dump_reg.c
	#BMAC iovar/ioctl
	WLFILES_SRC += src/wl/sys/wlc_bmac_iocv.c
	WLFILES_SRC += src/wl/sys/wlc_misc.c
	WLFILES_SRC += src/wl/sys/wlc_txs.c
	WLFILES_SRC += src/wl/sys/wlc_test.c
ifeq ($(WLRSDB),1)
	WLFILES_SRC += src/wl/sys/wlc_rsdb.c
	WLFILES_SRC += src/shared/si_d11rsdb_utils.c
	WLFILES_SRC += src/shared/ai_d11rsdb_utils.c
endif
ifeq ($(WL_STF_ARBITRATOR),1)
	 WLFILES_SRC += src/wl/sys/wlc_stf_arb.c
endif
ifeq ($(WL_MIMOPS)$(WL_STF_ARBITRATOR),11)
	 WLFILES_SRC += src/wl/sys/wlc_stf_mimops.c
endif
ifeq ($(WL_OBJ_REGISTRY),1)
	WLFILES_SRC += src/wl/sys/wlc_objregistry.c
endif
endif
#endif /* WL */

#ifdef BCMDBUS
ifeq ($(BCMDBUS),1)
	WLFLAGS += -DBCMDBUS
	ifneq ($(BCM_EXTERNAL_MODULE),1)
		WLFILES_SRC += src/shared/dbus.c
	endif

	ifeq ($(BCMTRXV2),1)
		WLFLAGS += -DBCMTRXV2
	endif

	ifeq ($(BCMSDIO),1)
		WLFILES_SRC += src/shared/siutils.c
		WLFILES_SRC += src/shared/bcmotp.c
		WLFILES_SRC += src/shared/sbutils.c
		WLFILES_SRC += src/shared/aiutils.c
		WLFILES_SRC += src/shared/hndpmu.c
	endif

	ifeq ($(WLLX),1)
		ifeq ($(BCMSDIO),1)
			WLFILES_SRC += src/shared/dbus_sdh.c
			WLFILES_SRC += src/shared/dbus_sdh_linux.c
		else
			ifneq ($(BCM_EXTERNAL_MODULE),1)
				WLFILES_SRC += src/shared/dbus_usb.c
				WLFILES_SRC += src/shared/dbus_usb_linux.c
			endif
		endif
	else
		ifeq ($(WLNDIS),1)
			ifeq ($(BCMSDIO),1)
				ifneq ($(EXTSTA),)
					WLFILES_SRC += src/shared/dbus_sdio.c
					WLFILES_SRC += src/shared/dbus_sdio_ndis.c
				else
					WLFILES_SRC += src/shared/dbus_sdh.c
					WLFILES_SRC += src/shared/dbus_sdh_ndis.c
				endif
			else
				WLFILES_SRC += src/shared/dbus_usb.c
				WLFILES_SRC += src/shared/dbus_usb_ndis.c
			endif
		endif
	endif
endif
#endif

ifeq ($(BCM_DNGL_EMBEDIMAGE),1)
	WLFLAGS += -DBCM_DNGL_EMBEDIMAGE
endif

# For USBAP to select which images to embed
ifeq ($(EMBED_IMAGE_4319usb),1)
	WLFLAGS += -DEMBED_IMAGE_4319usb
endif
ifeq ($(EMBED_IMAGE_4319sd),1)
	WLFLAGS += -DEMBED_IMAGE_4319sd
endif
ifeq ($(EMBED_IMAGE_4322),1)
	WLFLAGS += -DEMBED_IMAGE_4322
endif

ifeq ($(EMBED_IMAGE_43236b),1)
	WLFLAGS += -DEMBED_IMAGE_43236b
endif
ifeq ($(EMBED_IMAGE_43526b),1)
	WLFLAGS += -DEMBED_IMAGE_43526b
endif
ifeq ($(EMBED_IMAGE_43556b1),1)
	WLFLAGS += -DEMBED_IMAGE_43556b1
endif
ifeq ($(EMBED_IMAGE_4325sd),1)
	WLFLAGS += -DEMBED_IMAGE_4325sd
endif

## wl OSL
#ifdef WLVX
ifeq ($(WLVX),1)
	WLFILES_SRC += src/wl/sys/wl_vx.c
	WLFILES_SRC += src/shared/bcmstdlib.c
	WLFLAGS += -DSEC_TXC_ENABLED
endif
#endif

#ifdef WLBSD
ifeq ($(WLBSD),1)
	WLFILES_SRC += src/wl/sys/wl_bsd.c
endif
#endif

#ifdef WLLX
ifeq ($(WLLX),1)
        WLFILES_SRC += src/wl/sys/wl_linux.c
endif
#endif

#ifdef WLLXIW
ifeq ($(WLLXIW),1)
	WLFILES_SRC += src/wl/sys/wl_iw.c
	WLFLAGS += -DWLLXIW
endif
#endif

ifeq ($(BCM_STA_CFG80211),1)
	WLFILES_SRC += src/wl/sys/wl_cfg80211.c
	WLFILES_SRC += src/wl/sys/wl_cfgp2p.c
	WLFILES_SRC += src/wl/sys/wldev_common.c
	WLFILES_SRC += src/wl/sys/wl_linux_mon.c
	WLFILES_SRC += src/wl/sys/wl_cfg_btcoex.c
	WLFILES_SRC += src/wl/sys/wl_cfgvendor.c
endif

ifeq ($(BCM_ANDROID),1)
	WLFILES_SRC += src/wl/sys/wl_android.c
endif

#ifdef WLCFE
ifeq ($(WLCFE),1)
	WLFILES_SRC += src/wl/sys/wl_cfe.c
endif
#endif

#ifdef WLRTE
ifeq ($(WLRTE),1)
	WLFILES_SRC += src/wl/sys/wl_rte.c
#	#ifdef PROP_TXSTATUS
	ifeq ($(PROP_TXSTATUS),1)
		WLFLAGS += -DPROP_TXSTATUS
		WLFILES_SRC += src/wl/sys/wlc_wlfc.c
	endif
#	#endif  ## PROP_TXSTATUS
endif
#endif

ifeq ($(BCMECICOEX),1)
	WLFLAGS += -DBCMECICOEX
endif

ifeq ($(BCMLTECOEX),1)
	WLFILES_SRC += src/wl/sys/wlc_ltecx.c
	WLFLAGS += -DBCMLTECOEX
endif


ifeq ($(TRAFFIC_MGMT),1)
	WLFLAGS += -DTRAFFIC_MGMT
	WLFILES_SRC += src/wl/sys/wlc_traffic_mgmt.c

	ifeq ($(TRAFFIC_SHAPING),1)
		WLFLAGS += -DTRAFFIC_SHAPING
	endif

	ifeq ($(TRAFFIC_MGMT_RSSI_POLICY),1)
		WLFLAGS += -DTRAFFIC_MGMT_RSSI_POLICY=$(AP)
	endif

	ifeq ($(WLINTFERSTAT),1)
		WLFLAGS += -DWLINTFERSTAT
	endif

	ifeq ($(TRAFFIC_MGMT_DWM),1)
		WLFLAGS += -DTRAFFIC_MGMT_DWM
	endif
endif

ifeq ($(WLSTAPRIO),1)
	WLFLAGS += -DWL_STAPRIO
	WLFILES_SRC += src/wl/sys/wlc_staprio.c
endif


ifdef EXT_STA_DONGLE
$(error EXT_STA_DONGLE should be replaced by NDISFW & EXTSTA)
endif

#ifdef NDISFW
ifeq ($(NDISFW),1)
	ifeq ($(EXTSTA),1)
		# DONGLE vista needs WL_MONITOR to pass RTM
		WLFLAGS += -DEXT_STA
		WLFLAGS += -DWL_MONITOR
		WLFLAGS += -DWLCNTSCB
		WLFLAGS += -DIBSS_PEER_GROUP_KEY
		WLFLAGS += -DIBSS_PEER_DISCOVERY_EVENT
		WLFLAGS += -DIBSS_PEER_MGMT
	endif
	WLFILES_SRC += src/wl/sys/wlc_ndis_iovar.c
endif
#endif

ifeq ($(ADV_PS_POLL),1)
	WLFLAGS += -DADV_PS_POLL
endif

ifeq ($(GTKOE),1)
	WLFLAGS += -DGTKOE
	WLFILES_SRC += src/wl/sys/wl_gtkrefresh.c
	 ifeq ($(BCMULP),1)
                WLFILES_SRC += src/wl/sys/wl_gtkrefresh_ulp.c
        endif
	WLFILES_SRC += src/bcmcrypto/aes.c
        WLFILES_SRC += src/bcmcrypto/aeskeywrap.c
        WLFILES_SRC += src/bcmcrypto/hmac.c
        WLFILES_SRC += src/bcmcrypto/prf.c
        WLFILES_SRC += src/bcmcrypto/sha1.c
        ifeq ($(WLFBT),1)
                WLFILES_SRC += src/bcmcrypto/hmac_sha256.c
                WLFILES_SRC += src/bcmcrypto/sha256.c
        endif
        # NetBSD 2.0 has MD5 and AES built in
        ifneq ($(OSLBSD),1)
                WLFILES_SRC += src/bcmcrypto/md5.c
                WLFILES_SRC += src/bcmcrypto/rijndael-alg-fst.c
        endif
        WLFILES_SRC += src/bcmcrypto/passhash.c
endif


## wl features
# D11CONF, D11CONF2, and D11CONF3 --  bit mask of supported d11 core revs
ifneq ($(D11CONF),)
	WLFLAGS += -DD11CONF=$(D11CONF)
endif
ifneq ($(D11CONF2),)
	WLFLAGS += -DD11CONF2=$(D11CONF2)
endif
ifneq ($(D11CONF3),)
	WLFLAGS += -DD11CONF3=$(D11CONF3)
endif

# D11CONF_MINOR --  bit mask of supported d11 core minor revs
ifneq ($(D11CONF_MINOR),)
	WLFLAGS += -DD11CONF_MINOR=$(D11CONF_MINOR)
endif

# ACCONF and ACCONF2 -- 0 is remove from code, else bit mask of supported acphy revs
ifneq ($(ACCONF),)
	WLFLAGS += -DACCONF=$(ACCONF)
endif
ifneq ($(ACCONF2),)
	WLFLAGS += -DACCONF2=$(ACCONF2)
endif

# NCONF -- 0 is remove from code, else bit mask of supported nphy revs
ifneq ($(NCONF),)
	WLFLAGS += -DNCONF=$(NCONF)
endif

# HTCONF -- 0 is remove from code, else bit mask of supported htphy revs
ifneq ($(HTCONF),)
	WLFLAGS += -DHTCONF=$(HTCONF)
endif

# ACONF -- 0 is remove from code, else bit mask of supported aphy revs
ifneq ($(ACONF),)
	WLFLAGS += -DACONF=$(ACONF)
endif

# GCONF -- 0 is remove from code, else bit mask of supported gphy revs
ifneq ($(GCONF),)
	WLFLAGS += -DGCONF=$(GCONF)
endif

# LPCONF -- 0 is remove from code, else bit mask of supported lpphy revs
ifneq ($(LPCONF),)
	WLFLAGS += -DLPCONF=$(LPCONF)
endif

# SSLPNCONF -- 0 is remove from code, else bit mask of supported sslpnphy revs
ifneq ($(SSLPNCONF),)
	WLFLAGS += -DSSLPNCONF=$(SSLPNCONF)
endif


#ifdef SOFTAP
ifeq ($(SOFTAP),1)
	WLFLAGS += -DSOFTAP
endif
#endif

#ifdef AP
# ap
ifeq ($(AP),1)
	WLFILES_SRC += src/wl/sys/wlc_ap.c
	WLFILES_SRC += src/wl/sys/wlc_apps.c
	WLFLAGS += -DAP

	ifeq ($(MBSS),1)
		WLFILES_SRC += src/wl/sys/wlc_mbss.c
		WLFLAGS += -DMBSS
	endif

	ifeq ($(WDS),1)
		WLFILES_SRC += src/wl/sys/wlc_wds.c
		WLFLAGS += -DWDS
	endif

	ifeq ($(DWDS),1)
		WLFLAGS += -DDWDS
	endif

	# Channel Select
	ifeq ($(APCS),1)
		WLFILES_SRC += src/wl/sys/wlc_apcs.c
		WLFLAGS += -DAPCS
	endif

	# WME_PER_AC_TX_PARAMS
	ifeq ($(WME_PER_AC_TX_PARAMS),1)
		WLFLAGS += -DWME_PER_AC_TX_PARAMS
	endif

	# WME_PER_AC_TUNING
	ifeq ($(WME_PER_AC_TUNING),1)
		WLFLAGS += -DWME_PER_AC_TUNING
	endif

endif
#endif

#ifdef STA
# sta
ifeq ($(STA),1)
	WLFLAGS += -DSTA
	WLFILES_SRC += src/wl/sys/wlc_sta.c
endif
#endif

#ifdef APSTA
# apsta
ifeq ($(APSTA),1)
	WLFLAGS += -DAPSTA
endif
# apsta
#endif

#ifdef WL_NATOE
# NAT Offload
ifeq ($(WL_NATOE),1)
       WLFLAGS += -DWL_NATOE
       BCMFRWDPOOLREORG=1
       BCMPOOLRECLAIM=1
       WLFILES_SRC += src/wl/natoe/src/natoe.c
       WLFILES_SRC += src/wl/natoe/src/natoe_iovar.c
       WLFILES_SRC += src/wl/natoe/src/natoe_arp.c
endif
#endif
#ifdef BCMFRWDPOOLREORG
ifeq ($(BCMFRWDPOOLREORG),1)
	WLFLAGS += -DBCMFRWDPOOLREORG
	WLFILES_SRC += src/shared/hnd_poolreorg.c
	WLFILES_SRC += src/wl/sys/wlc_poolreorg.c
endif
#endif
#ifdef BCMPOOLRECLAIM
ifeq ($(BCMPOOLRECLAIM),1)
	WLFLAGS += -DBCMPOOLRECLAIM
endif
#endif

#ifdef WET
# wet
ifeq ($(WET),1)
	WLFLAGS += -DWET
	WLFILES_SRC += src/wl/sys/wlc_wet.c
endif
#endif

#ifdef RXCHAIN_PWRSAVE
ifeq ($(RXCHAIN_PWRSAVE), 1)
	WLFLAGS += -DRXCHAIN_PWRSAVE
endif
#endif

#ifdef RADIO_PWRSAVE
ifeq ($(RADIO_PWRSAVE), 1)
	WLFLAGS += -DRADIO_PWRSAVE
endif
#endif

#ifdef WMF
ifeq ($(WMF), 1)
	WLFILES_SRC += src/wl/sys/wlc_wmf.c
	WLFLAGS += -DWMF
endif
ifeq ($(IGMP_UCQUERY), 1)
	WLFLAGS += -DWL_IGMP_UCQUERY
endif
ifeq ($(UCAST_UPNP), 1)
	WLFLAGS += -DWL_UCAST_UPNP
endif
#endif

#ifdef MCAST_REGEN
ifeq ($(MCAST_REGEN), 1)
	WLFLAGS += -DMCAST_REGEN
endif
#endif

#ifdef  ROUTER_COMA
ifeq ($(ROUTER_COMA), 1)
	WLFILES_SRC += src/shared/hndmips.c
	WLFILES_SRC += src/shared/hndchipc.c
	WLFLAGS += -DROUTER_COMA
endif
#endif


#ifdef WLOVERTHRUSTER
ifeq ($(WLOVERTHRUSTER), 1)
	WLFLAGS += -DWLOVERTHRUSTER
endif
#endif

#ifdef MAC_SPOOF
# mac spoof
ifeq ($(MAC_SPOOF),1)
	WLFLAGS += -DMAC_SPOOF
endif
#endif

#ifdef PSTA
# Proxy STA
ifeq ($(PSTA),1)
	WLFILES_SRC += src/shared/bcm_psta.c
	WLFILES_SRC += src/wl/sys/wlc_psta.c
	WLFLAGS += -DPSTA
endif
#endif

#ifdef DPSTA
# Dualband Proxy STA
ifeq ($(STA),1)
	ifeq ($(DPSTA),1)
		WLFLAGS += -DDPSTA
	endif
endif
#endif

# Router IBSS Security Support
ifeq ($(ROUTER_SECURE_IBSS),1)
         WLFLAGS += -DIBSS_PEER_GROUP_KEY
         WLFLAGS += -DIBSS_PSK
         WLFLAGS += -DIBSS_PEER_MGMT
         WLFLAGS += -DIBSS_PEER_DISCOVERY_EVENT
endif

#ifdef WLLED
# led
ifeq ($(WLLED),1)
	WLFLAGS += -DWLLED
	WLFILES_SRC += src/wl/sys/wlc_led.c
endif
#endif

#ifdef WL_MONITOR
# MONITOR
ifeq ($(WL_MONITOR),1)
	WLFLAGS += -DWL_MONITOR
	WLFLAGS += -DWL_NEW_RXSTS
	ifeq ($(WL_MONITOR_DISABLED),1)
		WLFLAGS += -DWL_MONITOR_DISABLED
	endif
	ifneq ($(FULLDNGLBLD),1)
		WLFILES_SRC += src/shared/bcmwifi/src/bcmwifi_monitor.c
		WLFILES_SRC += src/shared/bcmwifi/src/bcmwifi_radiotap.c
	endif
endif
#endif

#ifdef WL_RADIOTAP
ifeq ($(WL_RADIOTAP),1)
	WLFLAGS += -DWL_RADIOTAP
	WLFILES_SRC += src/shared/bcmwifi/src/bcmwifi_radiotap.c
endif
#endif

#ifdef WL_STA_MONITOR
ifeq ($(WL_STA_MONITOR),1)
	WLFLAGS += -DWL_STA_MONITOR
endif
#endif

#ifdef ACKSUPR_MAC_FILTER
ifeq ($(ACKSUPR_MAC_FILTER),1)
        WLFLAGS += -DACKSUPR_MAC_FILTER
endif
#endif

#ifdef WL_PROMISC
# PROMISC
ifeq ($(PROMISC),1)
	WLFLAGS += -DWL_PROMISC
endif
#endif

ifeq ($(WL_ALL_PASSIVE),1)
	WLFLAGS += -DWL_ALL_PASSIVE
ifdef WL_ALL_PASSIVE_MODE
	WLFLAGS += -DWL_ALL_PASSIVE_MODE=$(WL_ALL_PASSIVE_MODE)
endif
endif

#ifdef ND_ALL_PASSIVE
ifeq ($(ND_ALL_PASSIVE),1)
	WLFLAGS += -DND_ALL_PASSIVE
endif
#endif

#ifdef WME
# WME
ifeq ($(WME),1)
	WLFLAGS += -DWME
endif
#endif

#ifdef WLPIO
# WLPIO
ifeq ($(WLPIO),1)
	WLFLAGS += -DWLPIO
	WLFILES_SRC += src/wl/sys/wlc_pio.c
endif
#endif

#ifdef WL11H
# 11H
ifeq ($(WL11H),1)
	WLFLAGS += -DWL11H
endif
#endif

#ifdef WL11D
# 11D
ifeq ($(WL11D),1)
	WLFLAGS += -DWL11D
endif
#endif

#ifdef WL11U
# 11U
ifeq ($(WL11U),1)
	L2_FILTER := 1
	WLFLAGS += -DWL11U
	WLFILES_SRC += src/wl/sys/wlc_11u.c
endif
#endif

#ifdef WLPROBRESP_SW
# WLPROBRESP_SW
ifeq ($(WLPROBRESP_SW),1)
	WLFLAGS += -DWLPROBRESP_SW
	WLFILES_SRC += src/wl/sys/wlc_probresp.c
ifeq ($(WLPROBRESP_MAC_FILTER),1)
	WLFLAGS += -DWLPROBRESP_MAC_FILTER
	WLFILES_SRC += src/wl/sys/wlc_probresp_mac_filter.c
endif
endif
#endif

ifeq ($(WLAUTHRESP_MAC_FILTER),1)
	WLFLAGS += -DWLAUTHRESP_MAC_FILTER
endif

#ifdef DBAND
# DBAND
ifeq ($(DBAND),1)
	WLFLAGS += -DDBAND
endif
#endif

#ifdef WLRM
# WLRM
ifeq ($(WLRM),1)
	WLFLAGS += -DWLRM
endif
#endif

#ifdef WLCQ
# WLCQ
ifeq ($(WLCQ),1)
	WLFLAGS += -DWLCQ
endif
#endif

#ifdef WLCNT
# WLCNT
ifeq ($(WLCNT),1)
	WLFLAGS += -DWLCNT
	WLFILES_SRC += src/shared/bcm_app_utils.c
ifndef DELTASTATS
	DELTASTATS := 1
endif
endif
#endif

# DELTASTATS
ifeq ($(DELTASTATS),1)
	WLFLAGS += -DDELTASTATS
endif

#ifdef WLCHANIM
# WLCHANIM
ifeq ($(WLCHANIM),1)
	WLFLAGS += -DWLCHANIM -DWLCHANIM_V2
endif
#endif

#ifdef WLCNTSCB
# WLCNTSCB
ifeq ($(WLCNTSCB),1)
	WLFLAGS += -DWLCNTSCB
endif
#endif
ifeq ($(WL_OKC),1)
	WLFLAGS += -DWL_OKC
	WLFILES_SRC += src/wl/sys/wlc_okc.c
	WLFILES_SRC += src/bcmcrypto/prf.c
	WLFILES_SRC += src/bcmcrypto/sha1.c
endif
ifeq ($(WLRCC),1)
	WLFLAGS += -DWLRCC
	ifneq ($(WL_OKC),1)
		WLFILES_SRC += src/wl/sys/wlc_okc.c
	endif
endif
ifeq ($(WLABT),1)
	WLFLAGS += -DWLABT
endif

#ifdef WLCOEX
# WLCOEX
ifeq ($(WLCOEX),1)
	WLFLAGS += -DWLCOEX
endif
#endif

#ifdef WLOSEN
# hotspot OSEN
ifeq ($(WLOSEN),1)
	WLFLAGS += -DWLOSEN
endif
#endif

## wl security
# external linux supplicant
#ifdef LINUX_CRYPTO
ifeq ($(LINUX_CRYPTO),1)
	WLFLAGS += -DLINUX_CRYPTO
endif
#endif

#ifdef WLFBT
ifeq ($(WLFBT),1)
	WLFLAGS += -DWLFBT
	WLFLAGS += -DBCMINTSUP
	WLFILES_SRC += src/wl/sys/wlc_fbt.c
	ifeq ($(STA),1)
		# external supplicant: need some crypto files for fbt
		ifneq ($(BCMSUP_PSK),1)
			WLFILES_SRC += src/wl/sys/wlc_sup.c
			WLFILES_SRC += src/wl/sys/wlc_wpapsk.c
			WLFILES_SRC += src/bcmcrypto/aes.c
			WLFILES_SRC += src/bcmcrypto/aeskeywrap.c
			WLFILES_SRC += src/bcmcrypto/prf.c
			WLFILES_SRC += src/bcmcrypto/sha1.c
			WLFILES_SRC += src/bcmcrypto/hmac.c
			WLFILES_SRC += src/bcmcrypto/hmac_sha256.c
			WLFILES_SRC += src/bcmcrypto/sha256.c
			WLFILES_SRC += src/bcmcrypto/passhash.c
			MD5 := 1
		endif
	endif
	WLCAC := 1
endif
#endif

ifeq ($(WL_ASSOC_MGR),1)
	WLFLAGS += -DWL_ASSOC_MGR
	WLFILES_SRC += src/wl/sys/wlc_assoc_mgr.c
endif

#ifdef BCMSUP_PSK
# in-driver supplicant
ifeq ($(BCMSUP_PSK),1)
	PSK_COMMON := 1
	ifeq ($(BCMCCX),1)
		WLFILES_SRC += src/wl/sys/wlc_sup_ccx.c
	endif
	WLFLAGS += -DBCMSUP_PSK -DBCMINTSUP
	WLFILES_SRC += src/wl/sys/wlc_sup.c
endif
#endif

#ifdef BCMAUTH_PSK
# in-driver authenticator
ifeq ($(BCMAUTH_PSK),1)
	PSK_COMMON := 1
	WLFLAGS += -DBCMAUTH_PSK
	WLFILES_SRC += src/wl/sys/wlc_auth.c
endif
#endif

# common files for both idsup & authenticator
ifeq ($(PSK_COMMON),1)
	WLFILES_SRC += src/wl/sys/wlc_wpapsk.c
	WLFILES_SRC += src/bcmcrypto/aes.c
	WLFILES_SRC += src/bcmcrypto/aeskeywrap.c
	WLFILES_SRC += src/bcmcrypto/hmac.c
	WLFILES_SRC += src/bcmcrypto/prf.c
	WLFILES_SRC += src/bcmcrypto/sha1.c
	ifeq ($(WLFBT),1)
		WLFILES_SRC += src/bcmcrypto/hmac_sha256.c
		WLFILES_SRC += src/bcmcrypto/sha256.c
	endif
	WLFILES_SRC += src/bcmcrypto/passhash.c
	MD5 := 1
	RIJNDAEL := 1
endif



#ifdef WLCAC
ifeq ($(WLCAC),1)
	WLFLAGS += -DWLCAC
	WLFILES_SRC += src/wl/sys/wlc_cac.c
endif
#endif

#ifdef WLTDLS
ifeq ($(WLTDLS), 1)
	WLFILES_SRC += src/bcmcrypto/sha256.c
	WLFILES_SRC += src/bcmcrypto/hmac_sha256.c
endif
#endif

#ifdef MFP
# Management Frame Protection
ifeq ($(MFP),1)
	WLFLAGS += -DMFP
	WLFILES_SRC += src/bcmcrypto/aes.c
	WLFILES_SRC += src/bcmcrypto/sha256.c
	WLFILES_SRC += src/bcmcrypto/hmac_sha256.c
	WLFILES_SRC += src/bcmcrypto/prf.c
	WLFILES_SRC += src/bcmcrypto/sha1.c
	WLFILES_SRC += src/wl/sys/wlc_mfp.c
	# BSD has AES built in
	ifneq ($(BSD),1)
		WLFILES_SRC += src/bcmcrypto/rijndael-alg-fst.c
	endif
	ifeq ($(MFP_TEST),1)
		WLFLAGS += -DMFP_TEST
		WLFILES_SRC += src/wl/sys/wlc_mfp_test.c
	endif
	BCMCCMP := 1
endif
#endif

#ifdef BCMCCMP
# Soft AES CCMP
ifeq ($(BCMCCMP),1)
	WLFLAGS += -DBCMCCMP
	WLFILES_SRC += src/bcmcrypto/aes.c
	# BSD has AES built in
	ifneq ($(BSD),1)
		WLFILES_SRC += src/bcmcrypto/rijndael-alg-fst.c
	endif
endif
#endif



# NetBSD 2.0 has MD5 and AES built in
ifneq ($(OSLBSD),1)
	ifeq ($(MD5),1)
		WLFILES_SRC += src/bcmcrypto/md5.c
	endif
	ifeq ($(RIJNDAEL),1)
		WLFILES_SRC += src/bcmcrypto/rijndael-alg-fst.c
	endif
endif

#ifdef WIFI_ACT_FRAME
# WIFI_ACT_FRAME
ifeq ($(WIFI_ACT_FRAME),1)
	WLFLAGS += -DWIFI_ACT_FRAME
endif
#endif

# BCMDMA32
ifeq ($(BCMDMA32),1)
	WLFLAGS += -DBCMDMA32
endif

ifeq ($(BCMDMA64OSL),1)
	WLFLAGS += -DBCMDMA64OSL
endif

ifeq ($(BCMDMASGLISTOSL),1)
	WLFLAGS += -DBCMDMASGLISTOSL
endif

## wl over jtag

#ifdef WLAMSDU
ifeq ($(WLAMSDU),1)
	WLFLAGS += -DWLAMSDU
	WLFILES_SRC += src/wl/sys/wlc_amsdu.c

	ifeq ($(BCMDBG_AMSDU),1)
		# Debug for: wl dump amsdu
		WLFLAGS += -DBCMDBG_AMSDU
	endif
endif
#endif

ifeq ($(BCMDBG_WLMAC),1)
	WLFLAGS += -DWL_MACDBG
endif

#ifdef WLAMSDU_TX
ifeq ($(WLAMSDU_TX),1)
	WLFLAGS += -DWLAMSDU_TX
	WLFILES_SRC += src/wl/sys/wlc_amsdu.c
endif
#endif

#ifdef WLAMSDU_SWDEAGG
ifeq ($(WLAMSDU_SWDEAGG),1)
	WLFLAGS += -DWLAMSDU_SWDEAGG
endif
#endif

#ifdef WLNAR
ifeq ($(WLNAR),1)
	WLFILES_SRC += src/wl/sys/wlc_nar.c
	WLFLAGS += -DWLNAR
endif
#endif

#ifdef WLAMPDU
ifeq ($(WLAMPDU),1)
	WLFLAGS += -DWLAMPDU
	WLFILES_SRC += src/wl/sys/wlc_ampdu.c
	WLFILES_SRC += src/wl/sys/wlc_ampdu_rx.c
	WLFILES_SRC += src/wl/sys/wlc_ampdu_cmn.c
	ifeq ($(WLAMPDU_UCODE),1)
		WLFLAGS += -DWLAMPDU_UCODE -DWLAMPDU_MAC
	endif
	ifeq ($(WLAMPDU_HW),1)
		WLFLAGS += -DWLAMPDU_HW -DWLAMPDU_MAC
	endif
	ifeq ($(WLAMPDU_UCODE_ONLY),1)
		WLFLAGS += -DWLAMPDU_UCODE_ONLY
	endif
	ifeq ($(WLAMPDU_AQM),1)
		WLFLAGS += -DWLAMPDU_AQM -DWLAMPDU_MAC
	endif
	ifeq ($(WLAMPDU_PRECEDENCE),1)
		WLFLAGS += -DWLAMPDU_PRECEDENCE
	endif
	ifeq ($(WL_EXPORT_AMPDU_RETRY),1)
		WLFLAGS += -DWL_EXPORT_AMPDU_RETRY
	endif

	ifeq ($(BCMDBG_AMPDU),1)
		# Debug for: wl dump ampdu; wl ampdu_clear_dump
		WLFLAGS += -DBCMDBG_AMPDU
	endif
endif
#endif

#ifdef WOWL
ifeq ($(WOWL),1)
	WLFLAGS += -DWOWL
	ifeq ($(BCMULP),1)
		WLFILES_SRC += $(UCODE_RAM_DIR)/d11ucode_ulp.c
	else
		WLFILES_SRC += $(UCODE_RAM_DIR)/d11ucode_wowl.c
	endif
	WLFILES_SRC += $(UCODE_RAM_DIR)/d11ucode_p2p.c
	WLFILES_SRC += $(UCODE_RAM_DIR)/d11ucode_ge40.c
	WLFILES_SRC += $(UCODE_RAM_DIR)/d11ucode_ge24.c
	WLFILES_SRC += src/wl/sys/wlc_wowl.c
	WLFILES_SRC += src/wl/sys/wowlaestbls.c
endif
#endif

# ucode in rom
ifeq ($(UCODE_IN_ROM),1)
		WLFLAGS += -DUCODE_IN_ROM_SUPPORT
		ifneq ($(ULP_DS1ROM_DS0RAM),1)
			WLFILES_SRC += $(UCODE_ROM_DIR)/d11ucode_p2p_upatch.c
		else # ULP_DS1ROM_DS0RAM
				WLFLAGS += -DULP_DS1ROM_DS0RAM
		endif # ULP_DS1ROM_DS0RAM
		WLFILES_SRC += $(UCODE_ROM_DIR)/d11ucode_ulp_upatch.c
endif # UCODE_IN_ROM

#ifdef WOWLPF
ifeq ($(WOWLPF),1)
	ifneq ($(PACKET_FILTER)$(PACKET_FILTER2)$(PACKET_FILTER2_DEBUG),)
		WLFLAGS += -DWOWLPF
		WLFILES_SRC += src/wl/sys/wlc_wowlpf.c
		#ifdef SECURE_WOWL
		ifeq ($(SECURE_WOWL),1)
			WLFLAGS += -DSECURE_WOWL
		endif
		#endif
		#ifdef SS_WOWL
		ifeq ($(SS_WOWL),1)
			WLFLAGS += -DSS_WOWL
		endif
		#endif
	endif
endif
#endif

#ifdef WL_ASSOC_RECREATE
ifeq ($(WL_ASSOC_RECREATE),1)
	ifneq ($(WL_ASSOC_RECREATE_DISABLE),1)
		ifeq ($(STA),1)
			WLFLAGS += -DWL_ASSOC_RECREATE
		endif
	endif
endif
#endif

ifeq ($(ATE),1)
	WLFLAGS += -DATE_BUILD=1
	WLFILES_SRC += src/wl/sys/wl_ate.c
endif

#ifdef WLTDLS
ifeq ($(TDLS_TESTBED), 1)
	WLFLAGS += -DTDLS_TESTBED
endif
ifeq ($(WLTDLS), 1)
	WLFLAGS += -DWLTDLS
	WLFLAGS += -DWLTDLS_SEND_PTI_RETRY
	WLFLAGS += -DIEEE2012_TDLSSEPC
	WLFILES_SRC += src/wl/sys/wlc_tdls.c
endif
ifeq ($(BE_TDLS),1)
	WLFLAGS += -DBE_TDLS
endif
ifeq ($(BE_TDLS_DISABLED),1)
	WLFLAGS += -DBE_TDLS_DISABLED
endif
#endif

#ifdef WLDLS
ifeq ($(WLDLS), 1)
	WLFLAGS += -DWLDLS
	WLFILES_SRC += src/wl/sys/wlc_dls.c
endif
#endif

#ifdef WLBSSLOAD
# WLBSSLOAD
ifeq ($(WLBSSLOAD),1)
	WLFLAGS += -DWLBSSLOAD
	WLFILES_SRC += src/wl/sys/wlc_bssload.c
endif
#endif

#ifdef L2_FILTER
ifeq ($(L2_FILTER),1)
	WLFLAGS += -DL2_FILTER
	ifeq ($(L2_FILTER_STA),1)
		WLFLAGS += -DL2_FILTER_STA
    endif
	WLFILES_SRC += src/wl/sys/wlc_l2_filter.c
	WLFILES_SRC += src/shared/bcm_l2_filter.c
endif
#endif

# FCC power limit control on ch12/13.
ifeq ($(FCC_PWR_LIMIT_2G),1)
	ifneq ($(FCC_PWR_LIMIT_2G_DISABLED),1)
		WLFLAGS += -DFCC_PWR_LIMIT_2G
	endif
endif

#ifdef WLP2P
ifeq ($(WLP2P),1)
	WLFLAGS += -DWLP2P
	WLFILES_SRC += src/wl/sys/wlc_p2p.c
	WLFLAGS += -DWL_BSSCFG_TX_SUPR -DWIFI_ACT_FRAME
	WLMCNX := 1
ifndef WLMCHAN
	WLMCHAN := 1
endif
# WFDS support
ifeq ($(WLWFDS),1)
	WLFLAGS += -DWLWFDS
endif
endif
#endif /* WLP2P */

#ifdef BCMCOEXNOA
ifeq ($(BCMCOEXNOA),1)
	WLFLAGS += -DBCMCOEXNOA
	WLFILES_SRC += src/wl/sys/wlc_cxnoa.c
	WLFLAGS += -DWL_BSSCFG_TX_SUPR -DWIFI_ACT_FRAME
	WLMCNX := 1
endif
#endif /* BCMCOEXNOA */

ifeq ($(WLFCTS),1)
	# PROP_TXSTATUS Timestamp feature
	WLFLAGS += -DWLFCTS
endif

ifeq ($(SRSCAN),1)
	WLFLAGS += -DWLMSG_SRSCAN
endif

#ifdef WL_RXEARLYRC
ifeq ($(WL_RXEARLYRC),1)
	WLFLAGS += -DWL_RXEARLYRC
endif
#endif

#ifdef SRHWVSDB
ifeq ($(SRHWVSDB),1)
	WLFLAGS += -DSRHWVSDB
endif
ifeq ($(PHY_WLSRVSDB),1)
	# cpp define WLSRVSDB is used in this branch by PHY code only
	WLFLAGS += -DWLSRVSDB
endif
#endif /* SRHWVSDB */

#ifdef WLMCHAN
ifeq ($(WLMCHAN),1)
	WLMCNX := 1
endif
#endif

#ifdef WLMESH
ifeq ($(WLMESH),1)
	WLFLAGS += -DWLMESH
	WLFILES_SRC += src/wl/mesh/src/wlc_mesh.c
	WLFILES_SRC += src/wl/mesh/src/wlc_mesh_peer.c
	WLFILES_SRC += src/wl/mesh/src/wlc_mesh_route.c
	WLFILES_SRC += src/wl/mesh/src/wlc_mesh_iovar.c
	ifeq ($(WLMSG_MESH),1)
		WLFLAGS += -DWLMSG_MESH
	endif
	WLMCNX := 1
endif
#endif

WLP2P_UCODE ?= 0
# multiple connection
ifeq ($(WLMCNX),1)
	WLP2P_UCODE := 1
	WLFLAGS += -DWLMCNX
	WLFILES_SRC += src/wl/sys/wlc_mcnx.c
	WLFILES_SRC += src/wl/sys/wlc_tbtt.c
endif

ifeq ($(WLP2P_UCODE_ONLY),1)
	WLFLAGS += -DWLP2P_UCODE_ONLY
#	WLP2P_UCODE := 1
endif

ifeq ($(WLP2P_UCODE),1)
	WLFLAGS += -DWLP2P_UCODE
	WLFILES_SRC += $(UCODE_RAM_DIR)/d11ucode_p2p.c
endif

#ifdef WLMCHAN
ifeq ($(WLMCHAN),1)
	WLFLAGS += -DWLMCHAN
	WLFLAGS += -DWLTXPWR_CACHE
	WLFLAGS += -DWLTXPWR_CACHE_PHY_ONLY
	WLFILES_SRC += src/wl/sys/wlc_mchan.c
ifndef WLMULTIQUEUE
	WLMULTIQUEUE := 1
endif
endif
#endif /* WLMCHAN */

ifeq ($(and $(WLMCHAN),$(WLRSDB),$(WL_MODESW),$(WLASDB)),1)
	WLFLAGS += -DWL_ASDB
	WLFILES_SRC_HI += src/wl/sys/wlc_asdb.c
endif


ifeq ($(WL_DUALMAC_RSDB), 1)
	WLFLAGS += -DWL_DUALMAC_RSDB
endif

ifneq ($(WL_NO_IOVF_RSDB_SET), 1)
	WLFLAGS += -DWL_IOVF_RSDB_SET
endif

# BSSID monitor
ifeq ($(WLBMON),1)
	WLFLAGS += -DWLBMON
endif
# WAR linux hybrid build nit
ifeq ($(WL),1)
	WLFILES_SRC += src/wl/sys/wlc_bmon.c
endif

ifeq ($(WL_DUALNIC_RSDB),1)
	WLFLAGS += -DWL_DUALNIC_RSDB
endif

ifeq ($(RSDB_DVT), 1)
	WLFLAGS += -DWLRSDB_DVT
endif

ifeq ($(CCA_STATS),1)
	WLFLAGS += -DCCA_STATS
	WLFILES_SRC += src/wl/sys/wlc_cca.c
ifeq ($(ISID_STATS),1)
	WLFLAGS += -DISID_STATS
	WLFILES_SRC += src/wl/sys/wlc_interfere.c
endif
endif

#ifdef WLRWL
ifeq ($(WLRWL),1)
	WLFLAGS += -DWLRWL
	WLFILES_SRC += src/wl/sys/wlc_rwl.c
endif
#endif

ifeq ($(D0_COALESCING),1)
	WLFLAGS += -DD0_COALESCING
	WLFILES_SRC += src/wl/sys/wl_d0_filter.c
	WLMCHAN := 1
endif

ifneq ($(WLNDIS_DHD),1)
	ifneq ($(WLNDIS),1)
		ifeq ($(AP)$(STA),11)
		ifeq ($(WLBTAMP),1)
			WLFLAGS += -DWLBTAMP
			WLFILES_SRC += src/wl/sys/wlc_bta.c
			ifeq ($(WLBTWUSB),1)
				WLFLAGS += -DWLBTWUSB
				WLFILES_SRC += src/wl/sys/bt_int_stub.c
			endif
		endif
		endif
	endif
endif

#ifdef WLMEDIA
ifeq ($(WLMEDIA),1)
	WLFLAGS += -DWLMEDIA_EN
	WLFLAGS += -DWLMEDIA_RATESTATS
	WLFLAGS += -DWLMEDIA_MULTIQUEUE
	WLFLAGS += -DWLMEDIA_TXFIFO_FLUSH_SCB
	WLFLAGS += -DWLMEDIA_AMPDUSTATS
	WLFLAGS += -DWLMEDIA_TXFAILEVENT
	WLFLAGS += -DWLMEDIA_LQSTATS
	WLFLAGS += -DWLMEDIA_CALDBG
	WLFLAGS += -DWLMEDIA_EXT
	WLFLAGS += -DWLMEDIA_TXFILTER_OVERRIDE
	WLFLAGS += -DWLMEDIA_TSF
	WLFLAGS += -DWLMEDIA_PEAKRATE
endif
#endif

#ifdef WLPKTDLYSTAT
ifeq ($(WLPKTDLYSTAT),1)
	WLFLAGS += -DWLPKTDLYSTAT
endif
#endif

#ifdef WLPKTDLYSTAT_IND
ifeq ($(WLPKTDLYSTAT_IND),1)
	WLFLAGS += -DWLPKTDLYSTAT_IND
endif
#endif

# Clear restricted channels upon receiving bcn/prbresp frames
#ifdef WLNON_CLEARING_RESTRICTED_CHANNEL_BEHAVIOR
ifeq ($(WLNON_CLEARING_RESTRICTED_CHANNEL_BEHAVIOR),1)
	WLFLAGS += -DWLNON_CLEARING_RESTRICTED_CHANNEL_BEHAVIOR
endif
#endif /* WLNON_CLEARING_RESTRICTED_CHANNEL_BEHAVIOR */

ifeq ($(WLFMC),1)
	WLFLAGS += -DWLFMC
endif

## --- which buses

# silicon backplane

#ifdef BCMSIBUS
ifeq ($(BCMSIBUS),1)
	WLFLAGS += -DBCMBUSTYPE=SI_BUS
	BCMPCI = 0
endif
#endif

ifeq ($(SOCI_SB),1)
	WLFLAGS += -DBCMCHIPTYPE=SOCI_SB
else
	ifeq ($(SOCI_AI),1)
		WLFLAGS += -DBCMCHIPTYPE=SOCI_AI
	endif
endif

#ifdef BCMPCMCIA
ifeq ($(BCMPCMCIA),1)
	WLFLAGS += -DBCMPCMCIA
endif
#endif


# AP/ROUTER with SDSTD
ifeq ($(WLAPSDSTD),1)
	WLFILES_SRC += src/shared/nvramstubs.c
	WLFILES_SRC += src/shared/bcmsrom.c
endif

## --- basic shared files

#ifdef HNDDMA
ifeq ($(HNDDMA),1)
	WLFILES_SRC += src/shared/hnddma_tx.c
	WLFILES_SRC += src/shared/hnddma_rx.c
	WLFILES_SRC += src/shared/hnddma.c
endif
#endif

#ifdef HNDGCI
ifeq ($(HNDGCI),1)
	WLFLAGS += -DHNDGCI
	WLFILES_SRC += src/shared/hndgci.c
endif
#endif

#ifdef BCMUTILS
ifeq ($(BCMUTILS),1)
	WLFILES_SRC += src/shared/bcmutils.c
	WLFILES_SRC += src/shared/bcmxtlv.c
	WLFILES_SRC += src/shared/hnd_pktpool.c
	WLFILES_SRC += src/shared/hnd_pktq.c
endif
#endif


# Use LHL Timer
ifeq ($(USE_LHL_TIMER),1)
	WLFLAGS += -DUSE_LHL_TIMER
endif

#ifdef BCMSROM
ifeq ($(BCMSROM),1)
	ifeq ($(BCMSDIO),1)
		WLFILES_SRC += src/shared/bcmsrom.c
		WLFILES_SRC += src/shared/bcmotp.c
	endif
	WLFILES_SRC += src/shared/bcmsrom.c
	WLFILES_SRC += src/shared/bcmotp.c
endif
#endif

#ifdef BCMOTP
ifeq ($(BCMOTP),1)
	WLFILES_SRC += src/shared/bcmotp.c
	WLFLAGS += -DBCMNVRAMR

	ifeq ($(BCMOTPSROM),1)
		WLFLAGS += -DBCMOTPSROM
	endif
endif
#endif

#ifdef SIUTILS
ifeq ($(SIUTILS),1)
	WLFILES_SRC += src/shared/siutils.c
	WLFILES_SRC += src/shared/sbutils.c
	WLFILES_SRC += src/shared/aiutils.c
	WLFILES_SRC += src/shared/hndpmu.c
	WLFILES_SRC += src/shared/hndlhl.c
	ifneq ($(BCMPCI), 0)
		WLFILES_SRC += src/shared/nicpci.c
		WLFILES_SRC += src/shared/pcie_core.c
	endif
endif
#endif /* SIUTILS */

#ifdef SBMIPS
ifeq ($(SBMIPS),1)
	WLFLAGS += -DBCMMIPS
	WLFILES_SRC += src/shared/hndmips.c
	WLFILES_SRC += src/shared/hndchipc.c
endif
#endif

#ifdef SBPCI
ifeq ($(SBPCI),1)
	WLFILES_SRC += src/shared/hndpci.c
endif
#endif

#ifdef SFLASH
ifeq ($(SFLASH),1)
	WLFILES_SRC += src/shared/sflash.c
endif
#endif

#ifdef FLASHUTL
ifeq ($(FLASHUTL),1)
	WLFILES_SRC += src/shared/flashutl.c
endif
#endif

## --- shared OSL
#ifdef OSLLX
# linux osl
ifeq ($(OSLLX),1)
	WLFILES_SRC += src/shared/linux_osl.c
	WLFILES_SRC += src/shared/linux_pkt.c
endif
#endif

#ifdef OSLVX
# vx osl
ifeq ($(OSLVX),1)
	WLFILES_SRC += src/shared/vx_osl.c
	WLFILES_SRC += src/shared/bcmallocache.c
endif
#endif

#ifdef OSLBSD
# bsd osl
ifeq ($(OSLBSD),1)
	WLFILES_SRC += src/shared/bsd_osl.c
	WLFILES_SRC += src/shared/nvramstubs.c
endif
#endif

#ifdef OSLCFE
ifeq ($(OSLCFE),1)
	WLFILES_SRC += src/shared/cfe_osl.c
endif
#endif

#ifdef OSLRTE
ifeq ($(OSLRTE),1)
	WLFILES_SRC += src/shared/rte_osl.c
	ifeq ($(SRMEM),1)
		WLFLAGS += -DSRMEM
		WLFILES_SRC += src/shared/hndsrmem.c
	endif
endif
#endif

#ifdef OSLNDIS
ifeq ($(OSLNDIS),1)
	ifeq ($(WLXP), 1)
		WLFILES_SRC += components/ndis/xp/src/ndshared5.c
	else
		WLFILES_SRC += components/ndis/src/ndshared.c
	endif
	WLFILES_SRC += src/shared/ndis_osl.c
endif
#endif

ifeq ($(NVRAM),1)
	WLFILES_SRC += src/dongle/rte/test/nvram.c
	WLFILES_SRC += src/dongle/rte/sim/nvram.c
	WLFILES_SRC += src/shared/nvram.c
endif

ifeq ($(NVRAMVX),1)
	WLFILES_SRC += src/shared/nvram_rw.c
endif

#ifdef BCMNVRAMR
ifeq ($(BCMNVRAMR),1)
	WLFILES_SRC += src/shared/nvram_ro.c
	WLFILES_SRC += src/shared/sflash.c
	WLFILES_SRC += src/shared/bcmotp.c
	WLFLAGS += -DBCMNVRAMR
endif
#else /* !BCMNVRAMR */
ifneq ($(BCMNVRAMR),1)
	ifeq ($(WLLXNOMIPSEL),1)
		ifneq ($(WLUMK),1)
			WLFILES_SRC += src/shared/nvramstubs.c
		endif
	else
		ifeq ($(WLNDIS),1)
			WLFILES_SRC += src/shared/nvramstubs.c
		else
			ifeq ($(BCMNVRAMW),1)
				WLFILES_SRC += src/shared/nvram_ro.c
				WLFILES_SRC += src/shared/sflash.c
			endif
		endif
	endif
	ifeq ($(BCMNVRAMW),1)
		WLFILES_SRC += src/shared/bcmotp.c
		WLFLAGS += -DBCMNVRAMW
	endif
endif
#endif /* !BCMNVRAMR */

# Define one OTP mechanism, or none to support all dynamically
ifeq ($(BCMHNDOTP),1)
	WLFLAGS += -DBCMHNDOTP
endif
ifeq ($(BCMIPXOTP),1)
	WLFLAGS += -DBCMIPXOTP
endif


#ifdef WLDIAG
ifeq ($(WLDIAG),1)
	WLFLAGS += -DWLDIAG
	WLFILES_SRC += src/wl/sys/wlc_diag.c
endif
#endif



#ifdef WLPFN
ifeq ($(WLPFN),1)
	WLFLAGS += -DWLPFN
	WLFILES_SRC += src/wl/sys/wl_pfn.c
	ifeq ($(WLPFN_AUTO_CONNECT),1)
		WLFLAGS += -DWLPFN_AUTO_CONNECT
	endif
endif
#endif /* WLPFN */

#ifdef GSCAN
ifeq ($(GSCAN),1)
	WLFLAGS += -DGSCAN
endif
#endif /* GSCAN */

#ifdef WL_PWRSTATS
ifeq ($(WL_PWRSTATS),1)
	WLFLAGS += -DWL_PWRSTATS
	WLFILES_SRC += src/wl/sys/wlc_pwrstats.c
endif
#endif

# Excess PMwake
#ifdef WL_EXCESS_PMWAKE
ifeq ($(WL_EXCESS_PMWAKE),1)
	WLFLAGS += -DWL_EXCESS_PMWAKE
endif
#endif

#ifdef TOE
ifeq ($(TOE),1)
	WLFLAGS += -DTOE
	WLFILES_SRC += src/wl/sys/wl_toe.c
endif
#endif

#ifdef ARPOE
ifeq ($(ARPOE),1)
	WLFLAGS += -DARPOE
	WLFILES_SRC += src/wl/sys/wl_arpoe.c
endif
#endif

#ifdef MDNS
ifeq ($(MDNS),1)
	WLFLAGS += -DMDNS
	WLFILES_SRC += src/wl/sys/wl_mdns_main.c
	WLFILES_SRC += src/wl/sys/wl_mdns_common.c
endif
#endif

#ifdef P2PO
ifeq ($(P2PO),1)
	WLFLAGS += -DP2PO
	WLFLAGS += -DWL_EVENTQ
	WLFLAGS += -DBCM_DECODE_NO_ANQP
	WLFLAGS += -DBCM_DECODE_NO_HOTSPOT_ANQP
	WLFLAGS += -DWLWFDS
	WLFILES_SRC += src/wl/sys/wl_p2po.c
	WLFILES_SRC += src/wl/sys/wl_p2po_disc.c
	WLFILES_SRC += src/wl/sys/wl_gas.c
	WLFILES_SRC += src/wl/sys/wl_tmr.c
	WLFILES_SRC += src/wl/sys/bcm_p2p_disc.c
	WLFILES_SRC += src/wl/sys/wl_eventq.c
	WLFILES_SRC += src/wl/gas/src/bcm_gas.c
	WLFILES_SRC += src/wl/encode/src/bcm_decode.c
	WLFILES_SRC += src/wl/encode/src/bcm_decode_gas.c
	WLFILES_SRC += src/wl/encode/src/bcm_decode_ie.c
	WLFILES_SRC += src/wl/encode/src/bcm_decode_anqp.c
	WLFILES_SRC += src/wl/encode/src/bcm_decode_hspot_anqp.c
	WLFILES_SRC += src/wl/encode/src/bcm_decode_p2p.c
	WLFILES_SRC += src/wl/encode/src/bcm_encode.c
	WLFILES_SRC += src/wl/encode/src/bcm_encode_gas.c
	WLFILES_SRC += src/wl/encode/src/bcm_encode_ie.c
	WLFILES_SRC += src/wl/encode/src/bcm_encode_anqp.c
	WLFILES_SRC += src/wl/encode/src/bcm_encode_hspot_anqp.c
endif
#endif

#ifdef P2POELO
ifeq ($(P2POELO),1)
	WLFLAGS += -DP2POELO
	WLFILES_SRC += src/wl/sys/wl_p2po.c
	WLFILES_SRC += src/wl/sys/wl_p2po_disc.c
	WLFILES_SRC += src/wl/sys/wl_tmr.c
	WLFILES_SRC += src/wl/sys/bcm_p2p_disc.c
endif
#endif

#ifdef ANQPO
ifeq ($(ANQPO),1)
	WLFLAGS += -DANQPO
	WLFLAGS += -DWL_EVENTQ
	WLFLAGS += -DBCM_DECODE_NO_ANQP
	WLFLAGS += -DBCM_DECODE_NO_HOTSPOT_ANQP
	WLFILES_SRC += src/wl/sys/wl_anqpo.c
	WLFILES_SRC += src/wl/sys/wl_gas.c
	WLFILES_SRC += src/wl/sys/wl_tmr.c
	WLFILES_SRC += src/wl/sys/wl_eventq.c
	WLFILES_SRC += src/wl/gas/src/bcm_gas.c
	WLFILES_SRC += src/wl/encode/src/bcm_decode.c
	WLFILES_SRC += src/wl/encode/src/bcm_decode_gas.c
	WLFILES_SRC += src/wl/encode/src/bcm_decode_ie.c
	WLFILES_SRC += src/wl/encode/src/bcm_decode_p2p.c
	WLFILES_SRC += src/wl/encode/src/bcm_encode.c
	WLFILES_SRC += src/wl/encode/src/bcm_encode_gas.c
	WLFILES_SRC += src/wl/encode/src/bcm_encode_ie.c
endif
#endif

#ifdef WLC_TSYNC
ifeq ($(WLC_TSYNC), 1)
	WLFLAGS += -DWLC_TSYNC
	WLFILES_SRC += src/wl/sys/wlc_tsync.c
endif
#endif

#ifdef BDO
ifeq ($(BDO),1)
	WLFLAGS += -DBDO
	WLFILES_SRC += src/wl/sys/wl_bdo.c
	WLFILES_SRC += src/wl/sys/wl_mdns_main.c
	WLFILES_SRC += src/wl/sys/wl_mdns_common.c
endif
#endif

#ifdef TKO
ifeq ($(TKO),1)
	WLFLAGS += -DTKO
	WLFILES_SRC += src/wl/sys/wl_tko.c
endif
#endif

#ifdef NWOE
ifeq ($(NWOE),1)
	WLFLAGS += -DNWOE
	WLFILES_SRC += src/wl/sys/wl_nwoe.c
	WLFILES_SRC += src/wl/lwip/src/core/def.c
	WLFILES_SRC += src/wl/lwip/src/core/dns.c
	WLFILES_SRC += src/wl/lwip/src/core/mem.c
	WLFILES_SRC += src/wl/lwip/src/core/netif.c
	WLFILES_SRC += src/wl/lwip/src/core/raw.c
	WLFILES_SRC += src/wl/lwip/src/core/stats.c
	WLFILES_SRC += src/wl/lwip/src/core/tcp.c
	WLFILES_SRC += src/wl/lwip/src/core/tcp_out.c
	WLFILES_SRC += src/wl/lwip/src/core/udp.c
	WLFILES_SRC += src/wl/lwip/src/core/dhcp.c
	WLFILES_SRC += src/wl/lwip/src/core/init.c
	WLFILES_SRC += src/wl/lwip/src/core/memp.c
	WLFILES_SRC += src/wl/lwip/src/core/pbuf.c
	WLFILES_SRC += src/wl/lwip/src/core/sys.c
	WLFILES_SRC += src/wl/lwip/src/core/tcp_in.c
	WLFILES_SRC += src/wl/lwip/src/core/timers.c
	WLFILES_SRC += src/wl/lwip/src/netif/etharp.c
	WLFILES_SRC += src/wl/lwip/src/core/ipv4/autoip.c
	WLFILES_SRC += src/wl/lwip/src/core/ipv4/icmp.c
	WLFILES_SRC += src/wl/lwip/src/core/ipv4/igmp.c
	WLFILES_SRC += src/wl/lwip/src/core/ipv4/inet.c
	WLFILES_SRC += src/wl/lwip/src/core/ipv4/inet_chksum.c
	WLFILES_SRC += src/wl/lwip/src/core/ipv4/ip_addr.c
	WLFILES_SRC += src/wl/lwip/src/core/ipv4/ip.c
	WLFILES_SRC += src/wl/lwip/src/core/ipv4/ip_frag.c
endif
#endif

#ifdef WLNDOE
ifeq ($(WLNDOE),1)
	WLFLAGS += -DWLNDOE -DWLNDOE_RA
	WLFILES_SRC += src/wl/sys/wl_ndoe.c
endif
#endif

#ifdef WL_BWTE
ifeq ($(WL_BWTE),1)
        WLFLAGS += -DWL_BWTE
        WLFILES_SRC += src/wl/sys/wlc_bwte.c
endif
#endif

#ifdef WL_TBOW
ifeq ($(WL_TBOW),1)
	WLFLAGS += -DWL_TBOW
	WLFILES_SRC += src/wl/sys/wlc_tbow.c
	WLFILES_SRC += src/wl/sys/wlc_homanager.c
endif
#endif

#msch
ifeq ($(WL_MSCH_PROFILER),1)
	WLFLAGS += -DMSCH_PROFILER
	WLFLAGS += -DMSCH_TESTING
	WLFILES_SRC += src/wl/sys/wlc_msch_profiler.c
endif

#ifdef WL_SHIF
ifeq ($(WL_SHIF),1)
	WLFLAGS += -DWL_SHIF
	WLFILES_SRC += src/wl/sys/wl_shub.c
endif
#endif


#ifdef PCOEM_LINUXSTA
ifeq ($(PCOEM_LINUXSTA),1)
	WLFLAGS += -DPCOEM_LINUXSTA
endif
#endif

#ifdef LINUXSTA_PS
ifeq ($(LINUXSTA_PS),1)
	WLFLAGS += -DLINUXSTA_PS
endif
#endif

#ifdef OPENSRC_IOV_IOCTL
ifeq ($(OPENSRC_IOV_IOCTL),1)
	WLFLAGS += -DOPENSRC_IOV_IOCTL
endif
#endif

ifeq ($(PACKET_FILTER),1)
	WLFILES_SRC += src/wl/sys/wlc_pkt_filter.c
	WLFLAGS += -DPACKET_FILTER
	ifeq ($(PACKET_FILTER2),1)
		WLFLAGS += -DPACKET_FILTER2
		ifeq ($(PACKET_FILTER2_DEBUG),1)
			WLFLAGS += -DPF2_DEBUG
		endif
		ifeq ($(PACKET_FILTER6),1)
			WLFLAGS += -DPACKET_FILTER6
		endif
	endif
endif

ifeq ($(SEQ_CMDS),1)
	WLFLAGS += -DSEQ_CMDS
	WLFILES_SRC += src/wl/sys/wlc_seq_cmds.c
endif

ifeq ($(OTA_TEST),1)
	WLFLAGS += -DWLOTA_EN
	WLFILES_SRC += src/wl/sys/wlc_ota_test.c
endif

ifeq ($(WLSWDIV),1)
	WLFLAGS += -DWLC_SW_DIVERSITY
	WLFILES_SRC += src/wl/sys/wlc_swdiv.c
endif
ifeq ($(WL_SW_DIVERSITY_DISABLED),1)
	WLFLAGS += -DWLC_SW_DIVERSITY_DISABLED
endif

ifeq ($(WLC_TXPWRCAP),1)
	WLFLAGS += -DWLC_TXPWRCAP
endif

ifeq ($(RECEIVE_THROTTLE),1)
	WLFLAGS += -DWL_PM2_RCV_DUR_LIMIT
endif

ifeq ($(DEBUG),1)
	ifeq ($(ASYNC_TSTAMPED_LOGS),1)
		WLFLAGS += -DBCMTSTAMPEDLOGS
	endif
endif

ifeq ($(WL11K_AP),1)
	WLFLAGS += -DWL11K_AP
	WL11K := 1
endif
ifeq ($(WL11K),1)
	WLFLAGS += -DWL11K
	WLFILES_SRC += src/wl/sys/wlc_rrm.c
endif

ifeq ($(WLWNM_AP),1)
	WLWNM := 1
	WLFLAGS += -DWLWNM_AP
endif

ifeq ($(WLWNM_BRCM),1)
	WLWNM := 1
	WLFLAGS += -DWLWNM_BRCM
endif

ifeq ($(WLWNM),1)
	WLFLAGS += -DWLWNM
	ifeq ($(STA),1)
		ifneq ($(WLWNM_AP),1)
			KEEP_ALIVE = 1
		endif
	endif
	WLFILES_SRC += src/wl/sys/wlc_wnm.c
	WLFILES_SRC += src/shared/bcm_l2_filter.c
endif

# BCMULP
ifeq ($(BCMULP),1)
	FCBS := 1
	WLFLAGS += -DBCMULP
	WLFLAGS += -DWOWL_OS_OFFLOADS
	WLFILES_SRC += src/wl/sys/wlc_ulp.c
	WLFILES_SRC += src/shared/ulp.c
	WLFILES_SRC += src/shared/hndmem.c
endif

ifeq ($(FCBS),1)
	WLFLAGS += -DBCMFCBS
	WLFILES_SRC += src/shared/fcbs.c
	WLFILES_SRC += src/shared/fcbsutils.c
	WLFILES_SRC += fcbs_metadata.c
	WLFILES_SRC += fcbs_ram_data.c
	WLFILES_SRC += src/shared/hndd11.c
	ifeq ($(BCMULP),1)
		WLFILES_SRC += src/shared/fcbsdata_pmu.c
		WLFILES_SRC += src/wl/sys/wlc_fcbsdata_d11.c
	endif
endif

ifeq ($(WNM_BSSTRANS_EXT),1)
	WLFLAGS += -DWNM_BSSTRANS_EXT
endif

ifeq ($(WL_MBO),1)
	WLFLAGS += -DWL_MBO
	WLFILES_SRC += src/wl/sys/wlc_mbo.c
endif

# Debug feature file
ifeq ($(WL_STATS), 1)
	WLFLAGS += -DWL_STATS
	WLFILES_SRC += src/wl/sys/wlc_stats.c
endif

ifeq ($(DBG_LINKSTAT), 1)
	WLFILES_SRC += src/wl/sys/wlc_linkstats.c
endif

ifeq ($(WL_LINKSTAT), 1)
	WLFLAGS += -DWL_LINKSTAT
	WLFILES_SRC += src/wl/sys/wlc_linkstats.c
endif

ifeq ($(KEEP_ALIVE),1)
	WLFLAGS += -DKEEP_ALIVE
	WLFILES_SRC += src/wl/sys/wl_keep_alive.c
endif

# wl patch code
ifneq ($(WLPATCHFILE), )
	WLFLAGS += -DWLC_PATCH
	WLFILES_SRC += $(WLPATCHFILE)
endif

ifeq ($(WLC_PATCH_IOCTL),1)
	WLFLAGS += -DWLC_PATCH_IOCTL
endif


ifeq ($(SAMPLE_COLLECT),1)
	WLFLAGS += -DSAMPLE_COLLECT
endif

ifeq ($(SMF_STATS),1)
	WLFLAGS += -DSMF_STATS
	WLFILES_SRC += src/wl/sys/wlc_smfs.c
endif

#ifdef PHYMON
ifeq ($(PHYMON),1)
	WLFLAGS += -DPHYMON
endif
#endif

ifeq ($(BCM_EXTERNAL_MODULE),1)
	WLFLAGS += -DLINUX_EXTERNAL_MODULE_DBUS
endif

#ifdef BCM_DCS
ifeq ($(BCM_DCS),1)
	WLFLAGS += -DBCM_DCS
endif
#endif

ifeq ($(WLMCHAN), 1)
	ifneq ($(WLMCHAN_DISABLED),1)
		WLFLAGS += -DWLTXPWR_CACHE
		WLFLAGS += -DWLTXPWR_CACHE_PHY_ONLY
	endif
endif

ifeq ($(WL_THREAD),1)
	WLFLAGS += -DWL_THREAD
endif

ifneq ($(WL_THREADNICE),)
	WLFLAGS += -DWL_THREADNICE=$(WL_THREADNICE)
endif

ifeq ($(USBOS_THREAD),1)
	WLFLAGS += -DUSBOS_THREAD
endif
ifeq ($(WL_NVRAM_FILE),1)
	WLFLAGS += -DWL_NVRAM_FILE
endif

ifeq ($(WL_FW_DECOMP),1)
	WLFLAGS += -DWL_FW_DECOMP
	WLFILES_SRC += src/shared/zlib/adler32.c
	WLFILES_SRC += src/shared/zlib/inffast.c
	WLFILES_SRC += src/shared/zlib/inflate.c
	WLFILES_SRC += src/shared/zlib/infcodes.c
	WLFILES_SRC += src/shared/zlib/infblock.c
	WLFILES_SRC += src/shared/zlib/inftrees.c
	WLFILES_SRC += src/shared/zlib/infutil.c
	WLFILES_SRC += src/shared/zlib/zutil.c
	WLFILES_SRC += src/shared/zlib/crc32.c
endif

ifeq ($(WL_WOWL_MEDIA),1)
	WLFLAGS += -DWL_WOWL_MEDIA
endif

ifeq ($(WL_USB_ZLP_PAD),1)
	WLFLAGS += -DWL_USB_ZLP_PAD
endif

ifeq ($(WL_URB_ZPKT),1)
	WLFLAGS += -DWL_URB_ZPKT
endif

ifeq ($(WL_LTR),1)
	WLFLAGS += -DWL_LTR
	WLFILES_SRC += src/wl/sys/wlc_ltr.c
endif

ifeq ($(BCM_BACKPLANE_TIMEOUT),1)
	WLFLAGS += -DBCM_BACKPLANE_TIMEOUT
endif

ifeq ($(UCODE_TOT_COREREV_66),1)
	WLFLAGS += -DUCODE_TOT_COREREV_66
endif
ifeq ($(WL_PRE_AC_DELAY_NOISE_INT),1)
	WLFLAGS += -DWL_PRE_AC_DELAY_NOISE_INT
endif

#ifdef SAVERESTORE
ifeq ($(SAVERESTORE),1)
	WLFLAGS += -DSAVERESTORE
	SR_ESSENTIALS := 1
endif

ifeq ($(SRFAST),1)
	WLFLAGS += -DSRFAST
endif

# minimal functionality required to operate SR engine. Examples: sr binary download, sr enable.
ifeq ($(SR_ESSENTIALS),1)
	WLFLAGS += -DSR_ESSENTIALS
	WLFILES_SRC += src/shared/sr_array.c
	WLFILES_SRC += src/shared/saverestore.c
endif
#endif

# Enable SR Power Control
ifeq ($(BCMSRPWR),1)
	WLFLAGS += -DBCMSRPWR
endif

#ifdef BCM_FD_AGGR
ifeq ($(BCM_FD_AGGR),1)
    WLFLAGS += -DBCM_FD_AGGR
	WLFILES_SRC += src/shared/bcm_rpc_tp_rte.c
endif
#endif

#ifdef SR_DEBUG
ifeq ($(SR_DEBUG),1)
	WLFLAGS += -DSR_DEBUG
endif
#endif

#ifdef BCM_REQUEST_FW
ifeq ($(BCM_REQUEST_FW), 1)
	WLFLAGS += -DBCM_REQUEST_FW
endif
#endif

#ifdef BCM_UCODE_FILES
ifeq ($(BCM_UCODE_FILES), 1)
        WLFLAGS += -DBCM_UCODE_FILES
endif
#endif

# HW CSO support (D11 rev40 feature)
ifeq ($(WLCSO),1)
	WLFLAGS += -DWLCSO
endif

# add a flag to indicate the split to linux kernels
WLFLAGS += -DPHY_HAL

# compile only 1x1 ACPHY related code
ifeq ($(ACPHY_1X1_ONLY),1)
WLFLAGS += -DACPHY_1X1_ONLY
endif

#ifdef WET_TUNNEL
ifeq ($(WET_TUNNEL),1)
	WLFLAGS += -DWET_TUNNEL
	WLFILES_SRC += src/wl/sys/wlc_wet_tunnel.c
endif
#endif

#ifdef WLDURATION
ifeq ($(WLDURATION),1)
	WLFLAGS += -DWLDURATION
	WLFILES_SRC += src/wl/sys/wlc_duration.c
endif
#endif

# Memory optimization. Use functions instead of macros for bit operations.
ifeq ($(BCMUTILS_BIT_MACROS_USE_FUNCS),1)
	WLFLAGS += -DBCMUTILS_BIT_MACROS_USE_FUNCS
endif

# Disable MCS32 in 40MHz
ifeq ($(DISABLE_MCS32_IN_40MHZ),1)
	WLFLAGS += -DDISABLE_MCS32_IN_40MHZ
endif

#ifdef WLPM_BCNRX
ifeq ($(WLPM_BCNRX),1)
	WLFLAGS += -DWLPM_BCNRX
endif
#endif

#ifdef WLSCAN_PS
ifeq ($(WLSCAN_PS),1)
	WLFLAGS += -DWLSCAN_PS
endif
#endif

#Disable aggregation for Voice traffic
#ifdef WL_DISABLE_VO_AGG
ifeq ($(WL_DISABLE_VO_AGG),1)
	WLFLAGS += -DWL_DISABLE_VO_AGG
endif
#endif

#ifdef TINY_PKTJOIN
ifeq ($(TINY_PKTJOIN),1)
	WLFLAGS += -DTINY_PKTJOIN
endif
#endif

#ifdef WL_RXEARLYRC
ifeq ($(WL_RXEARLYRC),1)
	WLFLAGS += -DWL_RXEARLYRC
endif
#endif

#ifdef WLMCHAN
#ifdef PROP_TXSTATUS
ifeq ($(WLMCHAN),1)
ifeq ($(PROP_TXSTATUS),1)
	WLFLAGS += -DROBUST_DISASSOC_TX
		WLFLAGS += -DWLMCHANPRECLOSE
		WLFLAGS += -DBBPLL_PARR
	endif
endif
#endif  ## PROP_TXSTATUS
#endif  ## WLMCHAN

#ifdef WLRXOV
ifeq ($(WLRXOV),1)
	WLFLAGS += -DWLRXOV
endif
#endif

ifeq ($(PKTC),1)
	WLFLAGS += -DPKTC
endif

ifeq ($(PKTC_DONGLE),1)
	WLFLAGS += -DPKTC_DONGLE
endif

#ifdef BCMPKTPOOL
# Packet Pool
ifeq ($(BCMPKTPOOL),1)
	WLFLAGS += -DBCMPKTPOOL
endif	# BCMPKTPOOL
#endif

#ifdef WLAIBSS
ifeq ($(WLAIBSS), 1)
        WLFLAGS += -DWLAIBSS
        WLFILES_SRC += src/wl/sys/wlc_aibss.c
endif
#endif

#ifdef WLTSFSYNC
ifeq ($(WLTSFSYNC), 1)
        WLFLAGS += -DWLTSFSYNC
endif
#endif

#ifdef WLIPFO
ifeq ($(WLIPFO), 1)
        WLFLAGS += -DWLIPFO
        WLFILES_SRC += src/wl/sys/wlc_ipfo.c
endif
#endif

#ifdef WL11ULB
ifeq ($(WL11ULB), 1)
        WLFLAGS += -DWL11ULB
        WLFILES_SRC += src/wl/sys/wlc_ulb.c
endif
#endif

#ifdef WL_FRWD_REORDER
ifeq ($(WL_FRWD_REORDER), 1)
        WLFLAGS += -DWL_FRWD_REORDER
endif
#endif

ifeq ($(WLBSSLOAD_REPORT),1)
	WLFLAGS += -DWLBSSLOAD_REPORT
	WLFILES_SRC += src/wl/sys/wlc_bssload.c
endif

# the existing PKTC_DONGLE once this additional feature is proven to be stable
ifeq ($(PKTC_TX_DONGLE),1)
	WLFLAGS += -DPKTC_TX_DONGLE
endif

# Secure WiFi through NFC
ifeq ($(WLNFC),1)
	WLFLAGS += -DWLNFC
	WLFILES_SRC += src/wl/sys/wl_nfc.c
endif

ifeq ($(BCM_NFCIF),1)
	WLFLAGS += -DBCM_NFCIF_ENABLED
	WLFLAGS += -DDISABLE_CONSOLE_UART_OUTPUT
	WLFILES_SRC += src/shared/bcm_nfcif.c
endif

ifeq ($(AMPDU_HOSTREORDER), 1)
	WLFLAGS += -DBRCMAPIVTW=128 -DWLAMPDU_HOSTREORDER
endif

# Append bss_info_t to selected events
ifeq ($(EVDATA_BSSINFO), 1)
	WLFLAGS += -DWL_EVDATA_BSSINFO
endif

# code optimisation for non dual phy chips
ifeq ($(DUAL_PHY_CHIP), 1)
	WLFLAGS += -DDUAL_PHY
endif

ifndef WLCFGDIR
	ifdef WLCFG_DIR
		WLCFGDIR=$(WLCFG_DIR)
	endif
endif
ifndef WLCFGDIR
$(error WLCFGDIR is not defined)
endif

# add keymgmt
ifeq ($(WLWSEC),1)
include $(WLCFGDIR)/keymgmt.mk
WLFILES_SRC += $(KEYMGMT_SRC_FILES)
endif

ifeq ($(WLROAMPROF),1)
	WLFLAGS += -DWLROAMPROF
endif

ifeq ($(SCAN_SUMEVT),1)
	WLFLAGS += -DWLSCAN_SUMEVT
endif

#ifdef TXQ_MUX
ifeq ($(TXQ_MUX),1)
	WLFLAGS += -DTXQ_MUX
endif
#endif

# PHY modules
ifeq ($(WL),1)
include $(WLCFGDIR)/phy.mk
WLFILES_SRC += $(PHY_SRC)
WLFLAGS += $(PHY_FLAGS)
endif

# enabling secure DMA feature
ifeq ($(BCM_SECURE_DMA),1)
	WLFLAGS += -DBCM_SECURE_DMA
endif

# ARMV7L
ifeq ($(ARMV7L),1)
	ifeq ($(STBLINUX),1)
		WLFLAGS += -DSTBLINUX
		WLFLAGS += -DSTB
		ifneq ($(BCM_SECURE_DMA),1)
			WLFLAGS += -DBCM47XX
		endif
		ifeq ($(BCM_SECURE_DMA),1)
			WLFILES_SRC += src/shared/stbutils.c
		endif
		WLFLAGS += -DBCMEXTNVM
	endif
endif

# ARMV8 (for 64 bit)
ifeq ($(ARMV8),1)
	ifeq ($(STBLINUX),1)
		WLFLAGS += -DSTBLINUX
		WLFLAGS += -DSTB
		WLFLAGS += -DBCMEXTNVM
	endif
endif

# BCM7271
ifeq ($(BCM7271),1)
	WLFLAGS += -DBCM7271
#	WLFLAGS += -DPLATFORM_INTEGRATED_WIFI
#	WLFLAGS += -DSTB_SOC_WIFI
	WLFILES_SRC += src/wl/sys/wl_linux_bcm7xxx.c
	WLFILES_SRC += src/wl/sys/wl_bcm7xxx_dbg.c
	ifeq ($(BDBG_DEBUG_BUILD),1)
		WLFLAGS += -DBDBG_DEBUG_BUILD=1
	else
		WLFLAGS += -DBDBG_DEBUG_BUILD=0
	endif
endif

ifeq ($(STBLINUX),1)
	WLFLAGS += -DSTBLINUX
endif

ifeq ($(WL_AUTH_SHARED_OPEN),1)
	WLFLAGS += -DWL_AUTH_SHARED_OPEN
endif

ifeq ($(PHY_EPAPD_NVRAMCTL),1)
	WLFLAGS += -DEPAPD_SUPPORT_NVRAMCTL=1
endif

# Dynamic bt coex(auto mode sw,desese, pwr ctrl. etc) -
ifeq ($(WL_BTCDYN),1)
	WLFLAGS += -DWL_BTCDYN
endif

ifeq ($(WLC_MACDBG_FRAMEID_TRACE),1)
	WLFLAGS += -DWLC_MACDBG_FRAMEID_TRACE
endif

ifeq ($(WLC_RXFIFO_CNT_ENABLE),1)
	WLFLAGS += -DWLC_RXFIFO_CNT_ENABLE
endif

# Datapath Debuggability, Health Check
ifeq ($(WL_DATAPATH_HC),1)
	EXTRA_DFLAGS += -DWL_DATAPATH_HC

        EXTRA_DFLAGS += -DWL_RX_DMA_STALL_CHECK

        EXTRA_DFLAGS += -DWL_TX_STALL
        EXTRA_DFLAGS += -DWLC_TXSTALL_FULL_STATS

        EXTRA_DFLAGS += -DWL_TXQ_STALL

        EXTRA_DFLAGS += -DWL_RX_STALL
endif

ifeq ($(WLOCL),1)
	EXTRA_DFLAGS += -DOCL
endif

#Arbitartor 
ifeq ($(WL_STF_ARBITRATOR),1)
    EXTRA_DFLAGS += -DWL_STF_ARBITRATOR
endif

# MIMOPS 
ifeq ($(WL_MIMOPS), 1)
    WLFLAGS += -DWL_MIMOPS
    EXTRA_DFLAGS += -DWL_MIMOPS_CFG
endif

# MIMO SISO stats
ifeq ($(WL_MIMO_SISO_STATS), 1)
    EXTRA_DFLAGS += -DWL_MIMO_SISO_STATS
endif

# Datapath Debuggability, option to prevent ASSERT on check
ifeq ($(WL_DATAPATH_HC_NOASSERT),1)
	EXTRA_DFLAGS += -DWL_DATAPATH_HC_NOASSERT
endif

ifeq ($(WL_DATAPATH_LOG_DUMP),1)
	WLFLAGS += -DWL_DATAPATH_LOG_DUMP
endif

# SmartAmpdu (TX DTS suppression)
ifeq ($(WL_DTS),1)
	WLFLAGS += -DWL_DTS
endif

# cache based datapath offload
ifeq ($(WLCXO),1)
	WLFLAGS += -DWLCXO
	WLFLAGS_CXO_DATA := -DWLCXO -DWL11N -DWL11AC
	#always support AMPDU (AQM) and AMSDU in CXO datapath
	WLFLAGS_CXO_DATA += -DWLAMPDU -DWLAMPDU_AQM -DWLAMPDU_MAC -DWLAMSDU -DWLAMSDU_TX

	ifeq ($(WLCXO_DUMP),1)
		WLFLAGS += -DWLCXO_DUMP
		WLFLAGS_CXO_DATA += -DWLCXO_DUMP
	endif

	ifeq ($(WLCXCNT),1)
		WLFLAGS += -DWLCXCNT
		WLFLAGS_CXO_DATA += -DWLCXCNT
	endif

	ifeq ($(WLCXO_CTRL),1)
		WLFLAGS += -DWLCXO_CTRL
		WLFILES_SRC_CXO_CTRL = src/wl/sys/wlc_cxo_ctrl.c
		ifeq ($(WLCXO_SIM),1)
			WLFLAGS += -DWLCXO_SIM
		endif
	endif

	ifeq ($(WLCXO_DATA),1)
		WLFLAGS_CXO_DATA += -DWLCXO_DATA
		ifeq ($(WLCXO_SIM),1)
			WLFLAGS_CXO_DATA += -DWLCXO_SIM
		endif
	endif

	ifeq ($(WLCXO_IPC),1)
		WLFLAGS += -DWLCXO_IPC
		WLFLAGS_CXO_DATA += -DWLCXO_IPC
	endif

	ifeq ($(WLCXO_CTRL_CXO_PKT),1)
		WLFLAGS += -DWLCXO_CXO_PKT
		WLFILES_SRC_CXO_CTRL += src/shared/cxo_pkt.c
	endif

	ifeq ($(WLCXO_DATA_CXO_PKT),1)
		WLFLAGS_CXO_DATA += -DWLCXO_CXO_PKT
		ifneq ($(WLCXO_CTRL_CXO_PKT),1)
			WLFILES_SRC_CXO_DATA += src/shared/cxo_pkt.c
		endif
	endif

	ifeq ($(WLCXO_CTRL_CXO_OSL),1)
		WLFLAGS += -DWLCXO_CXO_OSL
		WLFILES_SRC_CXO_CTRL += src/shared/cxo_osl.c
	endif

	ifeq ($(WLCXO_DATA_CXO_OSL),1)
		WLFLAGS_CXO_DATA += -DWLCXO_CXO_OSL
		ifneq ($(WLCXO_CTRL_CXO_OSL),1)
			WLFILES_SRC_CXO_DATA += src/shared/cxo_osl.c
		endif
	endif

	ifeq ($(WLCXO_CTRL_LX_PKT),1)
		WLFLAGS += -DWLCXO_LX_PKT
		WLFILES_SRC_CXO_CTRL += src/shared/linux_pkt.c
	endif

	ifeq ($(WLCXO_DATA_LX_PKT),1)
		WLFLAGS_CXO_DATA += -DWLCXO_LX_PKT
		ifneq ($(WLCXO_CTRL_LX_PKT),1)
			WLFILES_SRC_CXO_DATA += src/shared/linux_pkt.c
		endif
	endif

	ifeq ($(WLCXO_CTRL_LX_OSL),1)
		WLFLAGS += -DWLCXO_LX_OSL
		WLFILES_SRC_CXO_CTRL += src/shared/linux_osl.c
	endif

	ifeq ($(WLCXO_DATA_LX_OSL),1)
		WLFLAGS_CXO_DATA += -DWLCXO_LX_OSL
		ifneq ($(WLCXO_CTRL_LX_OSL),1)
			WLFILES_SRC_CXO_DATA += src/shared/linux_osl.c
		endif
	endif

	ifeq ($(WLCXO_CTRL_RTE_OSL),1)
		WLFLAGS += -DWLCXO_RTE_OSL
		WLFILES_SRC_CXO_CTRL += src/shared/rte_osl.c
	endif

	ifeq ($(WLCXO_DATA_RTE_OSL),1)
		WLFLAGS_CXO_DATA += -DWLCXO_RTE_OSL
		ifneq ($(WLCXO_CTRL_RTE_OSL),1)
			WLFILES_SRC_CXO_DATA += src/shared/rte_osl.c
		endif
	endif

	ifeq ($(WLCXO_CTRL_HND_PKT),1)
		WLFLAGS += -DWLCXO_HND_PKT
	endif

	ifeq ($(WLCXO_DATA_HND_PKT),1)
		WLFLAGS_CXO_DATA += -DWLCXO_HND_PKT
	endif

	WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo.c
	WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_ampdu.c
	WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_ampdu_rx.c
	WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_amsdu.c
	WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_amsdu_rx.c
	WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_bmac.c
	WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_bmac_pmq.c
	WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_intr.c
	WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_rsc.c
	WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_tsc.c
	WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_rx.c

	WLFILES_SRC_CXO_DATA += src/shared/bcmutils_cxo_data.c
	WLFILES_SRC_CXO_DATA += src/shared/hnd_pktq_cxo_data.c
	WLFILES_SRC_CXO_DATA += src/shared/hnddma_cxo_data_tx.c
	WLFILES_SRC_CXO_DATA += src/shared/hnddma_cxo_data_rx.c

	WLFILES_SRC_CXO_CTRL += src/wl/sys/wlc_cxo_ctrl_rsc.c
	WLFILES_SRC_CXO_CTRL += src/wl/sys/wlc_cxo_ctrl_tsc.c
endif
ifeq ($(WLCXO_DATA),1)
	ifeq ($(PSPRETEND),1)
		WLFLAGS_CXO_DATA += -DWLCXO_DATA_PPS
		ifeq ($(WLCXO_DATA_OVR_PSPRETEND), 1)
			WLFLAGS_CXO_DATA += -DPSPRETEND
		endif
	endif
	ifeq ($(WLOVERTHRUSTER), 1)
		ifeq ($(WLCXO_DATA_OVR_WLOVERTHRUSTER), 1)
			WLFLAGS_CXO_DATA += -DWLOVERTHRUSTER
		endif
	endif
	ifeq ($(PSTA),1)
		ifeq ($(WLCXO_DATA_OVR_PSTA), 1)
			WLFLAGS_CXO_DATA += -DPSTA
		endif
	endif
	ifeq ($(WLPIO),1)
		ifeq ($(WLCXO_DATA_OVR_WLPIO), 1)
			WLFLAGS_CXO_DATA += -DWLPIO
		endif
	endif
	ifeq ($(WLCNT),1)
		ifeq ($(WLCXO_DATA_OVR_WLCNT), 1)
			WLFLAGS_CXO_DATA += -DWLCNT
		endif
	endif
	ifeq ($(WLCNTSCB),1)
		ifeq ($(WLCXO_DATA_OVR_WLCNTSCB), 1)
			WLFLAGS_CXO_DATA += -DWLCNTSCB
		endif
	endif
	ifeq ($(WLNAR),1)
		ifeq ($(WLCXO_DATA_OVR_WLNAR), 1)
			WLFLAGS_CXO_DATA += -DWLNAR
		endif
	endif
	ifeq ($(WLPKTDLYSTAT),1)
		ifeq ($(WLCXO_DATA_OVR_WLPKTDLYSTAT), 1)
			WLFLAGS_CXO_DATA += -DWLPKTDLYSTAT
		endif
	endif
	ifeq ($(WLWNM_AP),1)
		ifeq ($(WLCXO_DATA_OVR_WLWNM_AP), 1)
			WLFLAGS_CXO_DATA += -DWLWNM_AP
		endif
	endif
	ifeq ($(WLCXO_DATA_OVR_PHY_HAL), 1)
		WLFLAGS_CXO_DATA += -DPHY_HAL
	endif
	ifeq ($(PKTC_DONGLE),1)
		ifeq ($(WLCXO_DATA_OVR_PKTC_DONGLE), 1)
			WLFLAGS_CXO_DATA += -DPKTC_DONGLE
		endif
	endif
	ifeq ($(BCMPKTPOOL),1)
		ifeq ($(WLCXO_DATA_OVR_BCMPKTPOOL), 1)
			WLFLAGS_CXO_DATA += -DBCMPKTPOOL
		endif
	endif
	ifeq ($(PKTQ_LOG),1)
		ifeq ($(WLCXO_DATA_OVR_PKTQ_LOG), 1)
			WLFLAGS_CXO_DATA += -DPKTQ_LOG
		endif
	endif

	ifeq ($(AP),1)
		WLFLAGS_CXO_DATA += -DAP
		ifeq ($(MBSS),1)
			WLFLAGS_CXO_DATA += -DMBSS
		endif
	endif
	ifeq ($(APSTA),1)
		WLFLAGS_CXO_DATA += -DAPSTA
	endif
	ifeq ($(STA),1)
		WLFLAGS_CXO_DATA += -DSTA
	endif
	ifeq ($(WME),1)
		WLFLAGS_CXO_DATA += -DWME
	endif
	ifneq ($(WLWSEC),0)
		WLFLAGS_CXO_DATA += -DWLWSEC
	endif
	ifeq ($(WLATF),1)
		WLFLAGS_CXO_DATA += -DWLATF
	endif
	ifeq ($(PKTC),1)
		WLFLAGS_CXO_DATA += -DPKTC
	endif
	ifeq ($(PKTC_TX_DONGLE),1)
		WLFLAGS_CXO_DATA += -DPKTC_TX_DONGLE
	endif
	ifeq ($(WLRTE),1)
		ifeq ($(PROP_TXSTATUS),1)
			WLFLAGS_CXO_DATA += -DPROP_TXSTATUS
		endif
	endif
	ifeq ($(DEBUG),1)
		WLFLAGS_CXO_DATA += -DBCMDBG -DWLTEST -DRWL_WIFI
	else ifeq ($(WLDEBUG),1)
		WLFLAGS_CXO_DATA += -DBCMDBG -DWLTEST
	endif
	ifeq ($(BCMDBG_MEM),1)
		WLFLAGS_CXO_DATA += -DBCMDBG_MEM
	endif
	#ifdef BCMDBG_TRAP
		WLFLAGS_CXO_DATA += -DBCMDBG_TRAP
	#endif
	ifneq ($(NO_BCMINTERNAL), 1)
		WLFLAGS_CXO_DATA += -DBCMINTERNAL -DUNRELEASEDCHIP
	endif
	ifeq ($(BCMDBG_PKT),1)
		WLFLAGS_CXO_DATA += -DBCMDBG_PKT
	endif
endif

# Override WLFILES_SRC to use WLFILES_SRC_CXO_DATA for CXO data only driver.
ifeq ($(WLCXO_CTRL),0)
	ifeq ($(WLCXO_DATA),1)
		# CXO DATA only ('offload') driver
		WLCXO_DATA_ONLY := 1

		ifeq ($(WLCXO_IPC),1)
			# Include cxo ipc
			ifeq ($(WLCXO_SIM),1)
				WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_ipc.c
				ifeq ($(WLCXO_LX_PKT),1)
					WLFILES_SRC_CXO_DATA += src/shared/linux_pkt.c
				endif
			else
				WLFILES_SRC_CXO_DATA += src/wl/sys/wlc_cxo_rpc.c
				WLFILES_SRC_CXO_DATA += src/shared/bcm_xdr.c
				WLFILES_SRC_CXO_DATA += src/shared/bcm_rpc.c
				WLFILES_SRC_CXO_DATA += src/shared/bcm_rpc_tp_mb.c
				ifeq ($(WLCXO_LX_OSL),1)
					WLFILES_SRC_CXO_DATA += src/shared/linux_rpc_osl.c
				endif
				ifeq ($(WLCXO_CXO_OSL),1)
					WLFILES_SRC_CXO_DATA += src/shared/cxo_rpc_osl.c
				endif
			endif
		endif
		ifeq ($(WLCXO_DATA_LX_OSL),1)
			WLFILES_SRC_CXO_DATA += src/wl/sys/wl_cxo.c
		endif
		ifeq ($(WLCXO_DATA_RTE_OSL),1)
			WLFILES_SRC_CXO_DATA += src/wl/sys/wl_cxo_rte.c
		endif

		WLFILES_SRC	:= $(WLFILES_SRC_CXO_DATA)
		WLFLAGS		:= $(WLFLAGS_CXO_DATA)
	endif
endif
ifeq ($(WLCXO_CTRL),1)
	ifeq ($(WLCXO_DATA),0)
		# CXO CTRL only ('host') driver
		ifeq ($(WLCXO_IPC),1)
			ifeq ($(WLCXO_SIM),1)
				WLFILES_SRC_CXO_CTRL += src/wl/sys/wlc_cxo_ipc.c
			else
				WLFILES_SRC_CXO_CTRL += src/wl/sys/wlc_cxo_rpc.c
				WLFILES_SRC_CXO_CTRL += src/shared/bcm_xdr.c
				WLFILES_SRC_CXO_CTRL += src/shared/bcm_rpc.c
				WLFILES_SRC_CXO_CTRL += src/shared/bcm_rpc_tp_mb.c
				ifeq ($(WLCXO_LX_OSL),1)
					WLFILES_SRC_CXO_CTRL += src/shared/linux_rpc_osl.c
				endif
				ifeq ($(WLCXO_CXO_OSL),1)
					WLFILES_SRC_CXO_CTRL += src/shared/cxo_rpc_osl.c
				endif
			endif
		endif
		WLFILES_SRC	+= $(WLFILES_SRC_CXO_CTRL)
	endif

	ifeq ($(WLCXO_DATA),1)
		# CXO FULL driver (CTRL+DATA)
		WLFLAGS			+= -DWLCXO_FULL
		WLFLAGS_CXO_DATA	+= -DWLCXO_FULL
		ifeq ($(WLCXO_IPC),0)
			ifeq ($(WLCXO_DATA_LX_OSL),1)
				WLFILES_SRC_CXO_DATA += src/wl/sys/wl_cxo.c
			endif
			ifeq ($(WLCXO_DATA_RTE_OSL),1)
				WLFILES_SRC_CXO_DATA += src/wl/sys/wl_cxo_rte.c
			endif
			WLFILES_SRC	+= $(WLFILES_SRC_CXO_CTRL) $(WLFILES_SRC_CXO_DATA)
		endif
	endif
endif

ifneq ($(SEC_ENHC),1)
		WLFLAGS			+= -DSEC_ENHANCEMENT
endif
# Sorting has two benefits: it uniqifies the list, which may have
# gotten some double entries above, and it makes for prettier and
# more predictable log output.
WLFILES_SRC := $(sort $(WLFILES_SRC))
# Legacy WLFILES pathless definition, please use new src relative path
# in make files.
WLFILES := $(sort $(notdir $(WLFILES_SRC)))
WLFILES_CXO_DATA := $(sort $(notdir $(WLFILES_SRC_CXO_DATA)))
