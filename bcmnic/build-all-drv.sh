#!/bin/bash

####################################
# if this is a DHD release
####################################
if [ -d ./src/dhd ]; then

	####################################
	# BUILD PCIE FD DRIVERS 
	####################################
	build-drv-fd.sh dhd-msgbuf-pciefd-mfp
	build-drv-fd.sh dhd-msgbuf-pciefd-mfp-secdma
	build-drv-fd.sh dhd-msgbuf-pciefd-reqfw-mfp
	build-drv-fd.sh dhd-msgbuf-pciefd-reqfw-mfp-secdma
	build-drv-fd.sh dhd-msgbuf-pciefd-android-media-cfg80211-mfp
	build-drv-fd.sh dhd-msgbuf-pciefd-android-media-cfg80211-mfp-secdma
	build-drv-fd.sh dhd-msgbuf-pciefd-reqfw-android-media-cfg80211-mfp
	build-drv-fd.sh dhd-msgbuf-pciefd-reqfw-android-media-cfg80211-mfp-secdma

	####################################
	# BUILD USB FD DRIVERS
	####################################
	build-drv-fd.sh dhd-cdc-usb-comp-mfp-gpl
	build-drv-fd.sh dhd-cdc-usb-reqfw-comp-mfp-gpl
	build-drv-fd.sh dhd-cdc-usb-android-media-cfg80211-comp-mfp
	build-drv-fd.sh dhd-cdc-usb-reqfw-android-media-cfg80211-comp-mfp

	####################################
	# BUILD PCIE FD MANUFACTURE DRIVERS
	####################################
	build-drv-fd-mfg.sh dhd-msgbuf-pciefd-mfp
	build-drv-fd-mfg.sh dhd-msgbuf-pciefd-reqfw-mfp
	
	####################################
	# BUILD USB FD MANUFACTURE DRIVERS
	####################################
	build-drv-fd-mfg.sh dhd-cdc-usb-comp-gpl
	build-drv-fd-mfg.sh dhd-cdc-usb-reqfw-comp-gpl

else

	####################################
	# BUILD BMAC DRIVERS
	####################################
	build-drv-bmac.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-high-stb
	build-drv-bmac.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-high-reqfw-stb

	####################################
	# BUILD NIC DRIVERS
	####################################
	build-drv-nic.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-stb
	build-drv-nic.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-secdma-stb
	build-drv-nic.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-extnvm-stb
	build-drv-nic.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-extnvm-secdma-stb

	####################################
	# BUILD BMAC MANUFACTURE DRIVERS
	####################################
	#build-drv-bmac-mfg.sh apdef-stadef-high-mfgtest-debug-stb

	####################################
	# BUILD NIC MANUFACTURE DRIVERS
	####################################
	#build-drv-nic-mfg.sh apdef-stadef-mfgtest-debug-stb

fi

