LOCAL_PATH := $(call my-dir)

#########################
ifneq ($(HW_WIFI_SUPPORT), n)
ifeq ($(HW_WIFI_NIC_SUPPORT), y)

# Build WL Utility
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
   ./sys/src/wl/exe/wlu.c \
   ./sys/src/wl/exe/wlu_common.c \
   ./sys/src/wl/exe/wlu_linux.c \
   ./sys/src/wl/exe/wlu_cmd.c \
   ./sys/src/wl/exe/wlu_iov.c \
   ./sys/src/wl/exe/wlu_client_shared.c \
   ./sys/src/wl/exe/wlu_pipe_linux.c \
   ./sys/src/wl/exe/wlu_pipe.c \
   ./sys/src/wl/exe/wlu_rates_matrix.c \
   ./sys/src/wl/exe/wluc_phy.c \
   ./sys/src/wl/exe/wluc_wnm.c \
   ./sys/src/wl/exe/wluc_cac.c \
   ./sys/src/wl/exe/wluc_relmcast.c \
   ./sys/src/wl/exe/wluc_rrm.c \
   ./sys/src/wl/exe/wluc_wowl.c \
   ./sys/src/wl/exe/wluc_pkt_filter.c \
   ./sys/src/wl/exe/wluc_mfp.c \
   ./sys/src/wl/exe/wluc_ota_test.c \
   ./sys/src/wl/exe/wluc_bssload.c \
   ./sys/src/wl/exe/wluc_stf.c \
   ./sys/src/wl/exe/wluc_offloads.c \
   ./sys/src/wl/exe/wluc_tpc.c \
   ./sys/src/wl/exe/wluc_toe.c \
   ./sys/src/wl/exe/wluc_arpoe.c \
   ./sys/src/wl/exe/wluc_keep_alive.c \
   ./sys/src/wl/exe/wluc_ap.c \
   ./sys/src/wl/exe/wluc_ampdu.c \
   ./sys/src/wl/exe/wluc_ampdu_cmn.c \
   ./sys/src/wl/exe/wluc_bmac.c \
   ./sys/src/wl/exe/wluc_ht.c \
   ./sys/src/wl/exe/wluc_wds.c \
   ./sys/src/wl/exe/wluc_keymgmt.c \
   ./sys/src/wl/exe/wluc_scan.c \
   ./sys/src/wl/exe/wluc_obss.c \
   ./sys/src/wl/exe/wluc_prot_obss.c \
   ./sys/src/wl/exe/wluc_lq.c \
   ./sys/src/wl/exe/wluc_seq_cmds.c \
   ./sys/src/wl/exe/wluc_btcx.c \
   ./sys/src/wl/exe/wluc_led.c \
   ./sys/src/wl/exe/wluc_interfere.c \
   ./sys/src/wl/exe/wluc_ltecx.c \
   ./sys/src/wl/exe/wluc_btcdyn.c \
   ./sys/src/wl/exe/wluc_nan.c \
   ./sys/src/wl/exe/wluc_extlog.c \
   ./sys/src/wl/exe/wluc_ndoe.c \
   ./sys/src/wl/exe/wluc_p2po.c \
   ./sys/src/wl/exe/wluc_anqpo.c \
   ./sys/src/wl/exe/wluc_bdo.c \
   ./sys/src/wl/exe/wluc_tko.c \
   ./sys/src/wl/exe/wluc_pfn.c \
   ./sys/src/wl/exe/wluc_tbow.c \
   ./sys/src/wl/exe/wluc_p2p.c \
   ./sys/src/wl/exe/wluc_tdls.c \
   ./sys/src/wl/exe/wluc_traffic_mgmt.c \
   ./sys/src/wl/exe/wluc_proxd.c \
   ./sys/src/wl/exe/wluc_randmac.c \
   ./sys/src/wl/exe/wluc_msch.c \
   ./sys/src/shared/bcmutils.c \
   ./sys/src/shared/bcmwifi/src/bcmwifi_channels.c \
   ./sys/src/shared/miniopt.c \
   ./sys/src/shared/bcm_app_utils.c \
   ./sys/src/shared/bcmxtlv.c \
   ./sys/src/shared/bcmbloom.c \
   ./sys/src/wl/ppr/src/wlc_ppr.c

LOCAL_MODULE := wl

LOCAL_C_INCLUDES += \
   $(LOCAL_PATH)/./sys/src/include \
   $(LOCAL_PATH)/./sys/src/wl/sys \
   $(LOCAL_PATH)/./sys/components/shared \
   $(LOCAL_PATH)/./sys/src/shared/bcmwifi/include \
   $(LOCAL_PATH)/./sys/src/wl/ppr/include

LOCAL_CFLAGS := -g -Wall -Wextra \
   -DTARGETENV_android -DTARGETOS_unix -DTARGETARCH_armv7l \
   -D_ARM_ -DD11AC_IOTYPES -DPPR_API -DSR_DEBUG -DWLCNT -DWIFI_ACT_FRAME -DWLEXTLOG \
   -DWLP2P -DWLMCHAN -DWLTDLS -DWLNDOE -DWLP2PO -DWLANQPO -DWLBDO -DWLTKO \
   -DTRAFFIC_MGMT -DWL_PROXDETECT -DWL_RANDMAC -DWL_MSCH -DWL11ULB \
   -DBT_WIFI_HANDOVER -DWLWNM -DWLBSSLOAD_REPORT \
   -DWL_NAN -DWL_NAN_PD_P2P -DWL_BTCDYN -DWL_MPF \
   -DWLPFN -DWLPFN_AUTO_CONNECT -DLINUX \
   -DRWL_SERIAL -DRWL_SOCKET -DRWL_DONGLE -DRWL_WIFI 

LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_SHARED_LIBRARIES :=
LOCAL_MODULE_TAGS := debug eng
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

include $(BUILD_EXECUTABLE)

endif
endif

