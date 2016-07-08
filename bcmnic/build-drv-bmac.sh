#!/bin/bash



if [ "$1" == "" ] ; then
	echo ""
	echo "************************************************"
	echo "USAGE:   build-drv-bmac.sh <TARGET>"
	echo ""
	echo "eg: build-drv-bmac.sh clean"
	echo ""
	echo "eg: build-drv-bmac.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-high-stb"
	echo "eg: build-drv-bmac.sh apdef-stadef-p2p-mchan-tdls-wowl-mfp-high-reqfw-stb"
	echo ""
	exit
elif [ "$1" == clean ] ; then
	rm -rf ./src/wl/linux/obj-*
	rm -rf ./src/linuxdev/obj-*
	exit
else
	export BUILDCFG="$1"
fi

export BRAND=linux-external-media
export BUILDARG="$2"


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


make -C ./src/wl/linux ${BUILDCFG} FIRMWARE="${FIRMWARE}"  LINUXVER=${LINUXVER} USBSHIM=${USBSHIM} ${BUILDARG}

if [ "${USBSHIM}" != "1" ] ; then
	cp -v ./src/wl/linux/obj-${BUILDCFG}-${LINUXVER}/bcm_dbus.ko ${TARGETDIR}
	cp -v ./src/wl/linux/obj-${BUILDCFG}-${LINUXVER}/bcm_dbus.ko ${TARGETDIR}/bcm_dbus-${TARGETARCH}.ko
fi

cp  -v ./src/wl/linux/obj-${BUILDCFG}-${LINUXVER}/wl.ko ${TARGETDIR}
cp  -v ./src/wl/linux/obj-${BUILDCFG}-${LINUXVER}/wl.ko ${TARGETDIR}/${BUILDCFG}-wl.ko

