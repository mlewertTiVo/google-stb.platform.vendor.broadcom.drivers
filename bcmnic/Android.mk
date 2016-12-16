LOCAL_PATH := $(call my-dir)

#########################
ifneq ($(HW_WIFI_SUPPORT), n)
ifeq ($(HW_WIFI_NIC_SUPPORT), y)

# Build WL Utility
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
   ./src/wl/exe/wlu.c \
   ./src/wl/exe/wlu_common.c \
   ./src/wl/exe/wlu_linux.c \
   ./src/wl/exe/wlu_cmd.c \
   ./src/wl/exe/wlu_iov.c \
   ./src/wl/exe/wlu_client_shared.c \
   ./src/wl/exe/wlu_pipe_linux.c \
   ./src/wl/exe/wlu_pipe.c \
   ./src/wl/exe/wlu_rates_matrix.c \
   ./src/wl/exe/wluc_phy.c \
   ./src/wl/exe/wluc_wnm.c \
   ./src/wl/exe/wluc_cac.c \
   ./src/wl/exe/wluc_relmcast.c \
   ./src/wl/exe/wluc_rrm.c \
   ./src/wl/exe/wluc_wowl.c \
   ./src/wl/exe/wluc_pkt_filter.c \
   ./src/wl/exe/wluc_mfp.c \
   ./src/wl/exe/wluc_ota_test.c \
   ./src/wl/exe/wluc_bssload.c \
   ./src/wl/exe/wluc_stf.c \
   ./src/wl/exe/wluc_offloads.c \
   ./src/wl/exe/wluc_tpc.c \
   ./src/wl/exe/wluc_toe.c \
   ./src/wl/exe/wluc_arpoe.c \
   ./src/wl/exe/wluc_keep_alive.c \
   ./src/wl/exe/wluc_ap.c \
   ./src/wl/exe/wluc_ampdu.c \
   ./src/wl/exe/wluc_ampdu_cmn.c \
   ./src/wl/exe/wluc_bmac.c \
   ./src/wl/exe/wluc_ht.c \
   ./src/wl/exe/wluc_wds.c \
   ./src/wl/exe/wluc_keymgmt.c \
   ./src/wl/exe/wluc_scan.c \
   ./src/wl/exe/wluc_obss.c \
   ./src/wl/exe/wluc_prot_obss.c \
   ./src/wl/exe/wluc_lq.c \
   ./src/wl/exe/wluc_seq_cmds.c \
   ./src/wl/exe/wluc_btcx.c \
   ./src/wl/exe/wluc_led.c \
   ./src/wl/exe/wluc_interfere.c \
   ./src/wl/exe/wluc_ltecx.c \
   ./src/wl/exe/wluc_btcdyn.c \
   ./src/wl/exe/wluc_nan.c \
   ./src/wl/exe/wluc_extlog.c \
   ./src/wl/exe/wluc_ndoe.c \
   ./src/wl/exe/wluc_p2po.c \
   ./src/wl/exe/wluc_anqpo.c \
   ./src/wl/exe/wluc_bdo.c \
   ./src/wl/exe/wluc_tko.c \
   ./src/wl/exe/wluc_pfn.c \
   ./src/wl/exe/wluc_tbow.c \
   ./src/wl/exe/wluc_p2p.c \
   ./src/wl/exe/wluc_tdls.c \
   ./src/wl/exe/wluc_traffic_mgmt.c \
   ./src/wl/exe/wluc_proxd.c \
   ./src/wl/exe/wluc_randmac.c \
   ./src/wl/exe/wluc_msch.c \
   ./src/wl/exe/wluc_he.c \
   ./src/wl/exe/wlu_avail_utils.c \
   ./src/wl/exe/wlu_subcounters.c \
   ./src/shared/bcmutils.c \
   ./src/shared/bcmwifi/src/bcmwifi_channels.c \
   ./src/shared/miniopt.c \
   ./src/shared/bcm_app_utils.c \
   ./src/shared/bcmxtlv.c \
   ./src/shared/bcmbloom.c \
   ./src/wl/ppr/src/wlc_ppr.c \
   ./src/bcmcrypto/sha256.c \

LOCAL_MODULE := wl

LOCAL_C_INCLUDES += \
   $(LOCAL_PATH)/./src/include \
   $(LOCAL_PATH)/./src/wl/sys \
   $(LOCAL_PATH)/./components/shared \
   $(LOCAL_PATH)/./src/shared/bcmwifi/include \
   $(LOCAL_PATH)/./src/wl/ppr/include

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

