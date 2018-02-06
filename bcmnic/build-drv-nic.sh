#!/bin/bash


if [ "$1" == "" ] ; then
	echo ""
	echo "************************************************"
	echo "USAGE:   build-drv-nic.sh <TARGET>"
	echo ""
	echo "eg: build-drv-nic.sh clean"
	echo ""
	echo "PCIe NIC Targets"	
	echo "eg: build-drv-nic.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-stb"
	echo "eg: build-drv-nic.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-secdma-stb"
	echo "eg: build-drv-nic.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-extnvm-stb"
	echo "eg: build-drv-nic.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-extnvm-secdma-stb"
	echo ""
	echo "PCIe NIC Manufacture Targets"	
	echo "eg: build-drv-nic.sh apdef-stadef-mfgtest-debug-stb"
	echo ""
	exit
elif [ "$1" == clean ] ; then
	rm -rf ./src/wl/linux/obj-*
	exit
else
	export BUILDCFG="$1"
fi

export BRAND=linux-external-media
export BUILDARG="$2"
export SHORTER_PATH?=0

######################################################################
# DO NOT MODIFY BELOW THIS LINE
######################################################################
if [ "${TARGETARCH}" == x86 ] ; then
	export BUILDCFG=${BUILDCFG}
	export STBLINUX=0
elif [ "${TARGETARCH}" == mips ] ; then
	if [ "${TARGETMACH}" == mipseb ] ; then 
		export BUILDCFG=${BUILDCFG}-mipseb-mips
	elif [ "${TARGETMACH}" == mipsel ] ; then
		export BUILDCFG=${BUILDCFG}-mipsel-mips
	else
		echo "TARGETARCH==${TARGETARCH}"
		echo "TARGETMACH==${TARGETMACH} undefined"
		exit
	fi
elif [ "${TARGETARCH}" == arm ]; then
	if [ "${TARGETMACH}" == arm ] ; then 
		export BUILDCFG=${BUILDCFG}-armv7l
	elif [ "${TARGETMACH}" == armeb ] ; then
		export BUILDCFG=${BUILDCFG}-armeb
	else
		echo "TARGETARCH==${TARGETARCH}"
		echo "TARGETMACH==${TARGETMACH} undefined"
		exit
	fi
elif [ "${TARGETARCH}" == arm64 ]; then
	if [ "${TARGETMACH}" == arm ] ; then
		export BUILDCFG=${BUILDCFG}-armv8
	elif [ "${TARGETMACH}" == armeb ] ; then
		export BUILDCFG=${BUILDCFG}-armeb
	else
		echo "TARGETARCH==${TARGETARCH}"
		echo "TARGETMACH==${TARGETMACH} undefined"
		exit
	fi
else
	echo "TARGETARCH==${TARGETARCH}"
	echo "TARGETMACH==${TARGETMACH} undefined"
	exit
fi



echo ""
echo "************************************************"
echo "      FIRMWARE = ${FIRMWARE}                    "
echo "      BUILDCFG = ${BUILDCFG}                    "
echo "      BUILDARG = "${BUILDARG}"                  "
echo "************************************************"
echo ""
sleep 3

mkdir -p ${TARGETDIR}

if [ ! -f ./src/include/epivers.h ]; then
	cd ./src/include
	make
	cd -
fi


make -C ./src/wl/linux ${BUILDCFG} FIRMWARE="${FIRMWARE}"  LINUXVER=${LINUXVER} ${BUILDARG}

if [ "${SHORTER_PATH}" == "1" ]; then
	cp  -v ./src/wl/linux/obj-${BCHP_VER}-${LINUXVER}/wl.ko ${TARGETDIR}
	cp  -v ./src/wl/linux/obj-${BCHP_VER}-${LINUXVER}/wl.ko ${TARGETDIR}/${BUILDCFG}-wl.ko
	if [ -e ./src/wlplat/obj-${BCHP_VER}-${LINUXVER}/wlplat.ko ]; then
		cp  -v ./src/wlplat/obj-${BCHP_VER}-${LINUXVER}/wlplat.ko ${TARGETDIR}
		cp  -v ./src/wlplat/obj-${BCHP_VER}-${LINUXVER}/wlplat.ko ${TARGETDIR}/${BUILDCFG}-wlplat.ko
	fi
else
	cp  -v ./src/wl/linux/obj-${BUILDCFG}-${LINUXVER}/wl.ko ${TARGETDIR}
	cp  -v ./src/wl/linux/obj-${BUILDCFG}-${LINUXVER}/wl.ko ${TARGETDIR}/${BUILDCFG}-wl.ko
	if [ -e ./src/wlplat/obj-${BCHP_VER}-${LINUXVER}/wlplat.ko ]; then
		cp  -v ./src/wlplat/obj-${BUILDCFG}-${LINUXVER}/wlplat.ko ${TARGETDIR}
		cp  -v ./src/wlplat/obj-${BUILDCFG}-${LINUXVER}/wlplat.ko ${TARGETDIR}/${BUILDCFG}-wlplat.ko
	fi
fi
