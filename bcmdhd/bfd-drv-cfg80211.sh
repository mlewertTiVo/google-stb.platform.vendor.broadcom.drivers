#!/bin/bash

if [ "${CHIPVER}" == 43236b ] || [ "${CHIPVER}" == 43238b ] ; then
	export BUILDCFG_COMMON=dhd-cdc-usb-android-media-cfg80211-noproptxstatus
elif [ "${CHIPVER}" == 43242a1 ] ; then
	export BUILDCFG_COMMON=dhd-cdc-usb-reqfw-android-media-cfg80211-comp
elif [ "${CHIPVER}" == 43569a0 ] || [ "${CHIPVER}" == 43569a2 ] ; then
	export BUILDCFG_COMMON=dhd-cdc-usb-reqfw-android-media-cfg80211-comp-mfp
elif [ "${CHIPVER}" == 43570a2 ] || [ "${CHIPVER}" == 43570a0 ] || \
     [ "${CHIPVER}" == 43602a1 ] || [ "${CHIPVER}" == 4365b1 ] || \
     [ "${CHIPVER}" == 4366c0 ] ; then
	if [ "${ANDROID_ENABLE_DHD_SECDMA}" == y ] ; then
		export BUILDCFG_COMMON=dhd-msgbuf-pciefd-reqfw-android-media-cfg80211-mfp-secdma
	else
		export BUILDCFG_COMMON=dhd-msgbuf-pciefd-reqfw-android-media-cfg80211-mfp
	fi
else
	export BUILDCFG_COMMON=dhd-cdc-usb-android-media-cfg80211
fi

if [ "${TARGETARCH}" == x86 ] ; then
	export BUILDCFG=${BUILDCFG_COMMON}
	export STBLINUX=0
elif [ "${TARGETARCH}" == mips ] ; then
	if [ "${TARGETMACH}" == mipseb ] ; then 
		export BUILDCFG=${BUILDCFG_COMMON}-be
		export STBLINUX=1
		export CROSS_COMPILE=mips-linux-	
	elif [ "${TARGETMACH}" == mipsel ] ; then
		export BUILDCFG=${BUILDCFG_COMMON}
		export STBLINUX=1
		export CROSS_COMPILE=mipsel-linux-	
	else
		echo "TARGETARCH==${TARGETARCH}"
		echo "TARGETMACH==${TARGETMACH} undefined"
		exit
	fi
elif [ "${TARGETARCH}" == arm ] ; then
	if [ "${TARGETMACH}" == armeb ] ; then 
		export BUILDCFG=${BUILDCFG_COMMON}-be
		export STBLINUX=1
		export CROSS_COMPILE=arm-linux- 
	elif [ "${TARGETMACH}" == armle ] ; then
		export BUILDCFG=${BUILDCFG_COMMON}-stb-armv7l-debug
		export STBLINUX=1
		export CROSS_COMPILE=arm-linux- 
	else
		echo "TARGETARCH==${TARGETARCH}"
		echo "TARGETMACH==${TARGETMACH} undefined"
		exit
	fi
elif [ "${TARGETARCH}" == aarch64 ] ; then
	export BUILDCFG=${BUILDCFG_COMMON}-stb-armv8-debug
	export STBLINUX=1
	export CROSS_COMPILE=aarch64-linux-
else
	echo "TARGETARCH==${TARGETARCH} undefined"
	exit
fi

export WLTEST=0
export DNGL_IMAGE_NAME="NOT NEEDED"

if [ "$1" == clean ] ; then
	echo "clean DHD driver build"
	rm -vrf ./dhd/src/dhd/linux/dhd-*
	rm -rf ${TARGETDIR}
	exit
else
	export BUILDARG="$1"
fi


echo ""
echo "************************************************"
echo "      DNGL_IMAGE_NAME = ${DNGL_IMAGE_NAME}      "
echo "      BUILDCFG = "${BUILDCFG}"                  "
echo "      BUILDARG = "${BUILDARG}"                  "
echo "************************************************"
echo ""
sleep 3

if [ ! -d ${TARGETDIR} ]; then
mkdir ${TARGETDIR}
fi

if [ ! -d ./src/dhd/linux/${BUILDCFG}-${LINUXVER} ]; then
mkdir ./dhd/src/dhd/linux/${BUILDCFG}-${LINUXVER}
fi

if [ ! -f ./dhd/src/include/epivers.h ]; then
cd ./dhd/src/include
make
cd -
fi

make -C ./dhd/src/dhd/linux ${BUILDCFG} WLTEST=${WLTEST} WLLXIW=${WLLXIW} LINUXVER=${LINUXVER} ${BUILDARG} CC=${CC} STRIP=${STRIP} V=${VERBOSE}

if [ "$?" -eq 0 ]; then
	if [ -f ./dhd/src/dhd/linux/${BUILDCFG}-${LINUXVER}/dhd.ko ]; then
	cp  -v ./dhd/src/dhd/linux/${BUILDCFG}-${LINUXVER}/dhd.ko ${TARGETDIR}
	cp  -v ./dhd/src/dhd/linux/${BUILDCFG}-${LINUXVER}/dhd.ko ${TARGETDIR}/${CHIPVER}-${BUILDCFG}-dhd.ko
	fi

	if [ -f ./dhd/src/dhd/linux/${BUILDCFG}-${LINUXVER}/bcmdhd.ko ]; then
	cp  -v ./dhd/src/dhd/linux/${BUILDCFG}-${LINUXVER}/bcmdhd.ko ${TARGETDIR}
	cp  -v ./dhd/src/dhd/linux/${BUILDCFG}-${LINUXVER}/bcmdhd.ko ${TARGETDIR}/${CHIPVER}-${BUILDCFG}-bcmdhd.ko
	fi
else
	echo "failed to build bcmdhd"
	rm -f ${TARGETDIR}/*.ko
	exit -1
fi
