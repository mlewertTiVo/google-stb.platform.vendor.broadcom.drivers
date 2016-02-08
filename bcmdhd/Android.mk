LOCAL_PATH := $(call my-dir)

#########################
ifneq ($(ANDROID_ENABLE_DHD), n)

# Build DHD Utility
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
   dhd/src/dhd/exe/dhdu.c \
   dhd/src/dhd/exe/dhdu_linux.c \
   dhd/src/dhd/exe/ucode_download.c \
   dhd/src/wl/exe/wlu_client_shared.c \
   dhd/src/wl/exe/wlu_pipe_linux.c \
   dhd/src/wl/exe/wlu_pipe.c \
   dhd/src/wl/exe/wlu_common.c \
   dhd/src/shared/bcmutils.c \
   dhd/src/shared/bcmxtlv.c \
   dhd/src/shared/miniopt.c \
   dhd/src/shared/bcm_app_utils.c \
   dhd/src/shared/bcmwifi/src/bcmwifi_channels.c


LOCAL_MODULE := dhd_tool

LOCAL_C_INCLUDES += \
   $(LOCAL_PATH)/dhd/src/include \
   $(LOCAL_PATH)/dhd/src/dhd/exe \
   $(LOCAL_PATH)/dhd/src/wl/exe \
   $(LOCAL_PATH)/dhd/components/shared \
   $(LOCAL_PATH)/dhd/src/shared/bcmwifi/include \

LOCAL_CFLAGS := -g -W -Wall \
   -DTARGETENV_android -DTARGETENV_linuxarm -DTARGETOS_unix -DTARGETARCH_arm -D_ARM_ -DIL_BIGENDIAN \
   -DBCMWPA2 -DSDTEST -DBCMSPI -DWLCNT -DBCMDONGLEHOST -DWLBTAMP -DWLPFN -DLINUX \
   -DRWL_SOCKET -DRWL_WIFI -DRWL_DONGLE

LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_MODULE_TAGS := debug eng
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

include $(BUILD_EXECUTABLE)

# Build WL Utility
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
   dhd/src/wl/exe/wlu.c \
   dhd/src/wl/exe/wlu_common.c \
   dhd/src/wl/exe/wlu_linux.c \
   dhd/src/wl/exe/wlu_cmd.c \
   dhd/src/wl/exe/wlu_iov.c \
   dhd/src/wl/exe/wlu_client_shared.c \
   dhd/src/wl/exe/wlu_pipe_linux.c \
   dhd/src/wl/exe/wlu_pipe.c \
   dhd/src/wl/exe/wlu_rates_matrix.c \
   dhd/src/wl/exe/wluc_phy.c \
   dhd/src/wl/exe/wluc_wnm.c \
   dhd/src/wl/exe/wluc_cac.c \
   dhd/src/wl/exe/wluc_relmcast.c \
   dhd/src/wl/exe/wluc_rrm.c \
   dhd/src/wl/exe/wluc_wowl.c \
   dhd/src/wl/exe/wluc_pkt_filter.c \
   dhd/src/wl/exe/wluc_mfp.c \
   dhd/src/wl/exe/wluc_ota_test.c \
   dhd/src/wl/exe/wluc_bssload.c \
   dhd/src/wl/exe/wluc_stf.c \
   dhd/src/wl/exe/wluc_offloads.c \
   dhd/src/wl/exe/wluc_tpc.c \
   dhd/src/wl/exe/wluc_toe.c \
   dhd/src/wl/exe/wluc_arpoe.c \
   dhd/src/wl/exe/wluc_keep_alive.c \
   dhd/src/wl/exe/wluc_ap.c \
   dhd/src/wl/exe/wluc_ampdu.c \
   dhd/src/wl/exe/wluc_ampdu_cmn.c \
   dhd/src/wl/exe/wluc_bmac.c \
   dhd/src/wl/exe/wluc_ht.c \
   dhd/src/wl/exe/wluc_wds.c \
   dhd/src/wl/exe/wluc_keymgmt.c \
   dhd/src/wl/exe/wluc_scan.c \
   dhd/src/wl/exe/wluc_obss.c \
   dhd/src/wl/exe/wluc_prot_obss.c \
   dhd/src/wl/exe/wluc_lq.c \
   dhd/src/wl/exe/wluc_seq_cmds.c \
   dhd/src/wl/exe/wluc_btcx.c \
   dhd/src/wl/exe/wluc_led.c \
   dhd/src/wl/exe/wluc_interfere.c \
   dhd/src/wl/exe/wluc_ltecx.c \
   dhd/src/wl/exe/wluc_btcdyn.c \
   dhd/src/wl/exe/wluc_nan.c \
   dhd/src/wl/exe/wluc_extlog.c \
   dhd/src/wl/exe/wluc_ndoe.c \
   dhd/src/wl/exe/wluc_p2po.c \
   dhd/src/wl/exe/wluc_anqpo.c \
   dhd/src/wl/exe/wluc_pfn.c \
   dhd/src/wl/exe/wluc_tbow.c \
   dhd/src/wl/exe/wluc_p2p.c \
   dhd/src/wl/exe/wluc_tdls.c \
   dhd/src/wl/exe/wluc_traffic_mgmt.c \
   dhd/src/wl/exe/wluc_proxd.c \
   dhd/src/shared/bcmutils.c \
   dhd/src/shared/bcmwifi/src/bcmwifi_channels.c \
   dhd/src/shared/miniopt.c \
   dhd/src/shared/bcm_app_utils.c \
   dhd/src/shared/bcmxtlv.c \
   dhd/src/shared/bcmbloom.c \
   dhd/src/wl/ppr/src/wlc_ppr.c

LOCAL_MODULE := wl

LOCAL_C_INCLUDES += \
   $(LOCAL_PATH)/dhd/src/include \
   $(LOCAL_PATH)/dhd/src/dhd/exe \
   $(LOCAL_PATH)/dhd/src/wl/exe \
   $(LOCAL_PATH)/dhd/src/wl/sys \
   $(LOCAL_PATH)/dhd/src/wl/phy \
   $(LOCAL_PATH)/dhd/components/shared \
   $(LOCAL_PATH)/dhd/src/shared/bcmwifi/include \
   $(LOCAL_PATH)/dhd/src/wl/ppr/include

LOCAL_CFLAGS := -g -Wall -Wextra \
   -DTARGETENV_android -DTARGETENV_linuxarm -DTARGETOS_unix -DTARGETARCH_arm \
   -D_ARM_ -DIL_BIGENDIAN -DWLPFN -DWLPFN_AUTO_CONNECT -DLINUX -DWLC_HIGH \
   -DD11AC_IOTYPES -DPPR_API -DSR_DEBUG -DWLCNT -DWIFI_ACT_FRAME -DWLEXTLOG \
   -DWLP2P -DWLMCHAN -DWLTDLS -DWLNDOE -DWLP2PO -DWLANQPO -DTRAFFIC_MGMT \
   -DWL_PROXDETECT -DWL11ULB -DBT_WIFI_HANDOVER -DWLWNM -DWLBSSLOAD_REPORT \
   -DWL_NAN -DWL_BTCDYN -DRWL_SERIAL

LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_SHARED_LIBRARIES :=
LOCAL_MODULE_TAGS := debug eng
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

include $(BUILD_EXECUTABLE)

endif
