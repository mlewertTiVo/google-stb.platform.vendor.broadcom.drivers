#!/bin/bash


if [ "$1" == "" ] ; then
	echo ""
	echo "************************************************"
	echo "USAGE:   build-drv.sh <TARGET>"
	echo ""
	echo "USB BMAC Disconnect Targets"	
	echo "eg: build-drv.sh clean"
	echo "eg: build-drv.sh media-high"
	echo "eg: build-drv.sh media-high-p2p"
	echo "eg: build-drv.sh media-high-p2p-mchan"
	echo "eg: build-drv.sh media-high-p2p-mchan-cfg80211"
	echo "eg: build-drv.sh media-high-p2p-mchan-cfg80211-debug"
	echo "eg: build-drv.sh media-high-p2p-mchan-tdls"
	echo "eg: build-drv.sh media-high-p2p-mchan-tdls-cfg80211"
	echo ""
	echo "USB BMAC Request Firmware Targets"	
	echo "eg: build-drv.sh media-high-reqfw"
	echo "eg: build-drv.sh media-high-reqfw-p2p"
	echo "eg: build-drv.sh media-high-reqfw-p2p-mchan"
	echo "eg: build-drv.sh media-high-reqfw-p2p-mchan-cfg80211"
	echo "eg: build-drv.sh media-high-reqfw-p2p-mchan-cfg80211-debug"
	echo "eg: build-drv.sh media-high-reqfw-p2p-mchan-tdls"
	echo "eg: build-drv.sh media-high-reqfw-p2p-mchan-tdls-cfg80211"
	echo ""
	echo "USB BMAC Disconnect Manufacture Targets"	
	echo "eg: build-drv.sh media-high-mfgtest"
	echo ""
	echo "PCIe NIC Targets"	
	echo "eg: build-drv.sh apdef-stadef"
	echo "eg: build-drv.sh apdef-stadef-p2p"
	echo "eg: build-drv.sh apdef-stadef-p2p-mchan"
	echo "eg: build-drv.sh apdef-stadef-p2p-mchan-tdls"
	echo "eg: build-drv.sh apdef-stadef-p2p-mchan-tdls-cfg80211"
	echo ""
	echo "PCIe NIC Manufacture Targets"	
	echo "eg: build-drv.sh apdef-stadef-mfgtest"
	echo ""
	exit
elif [ "$1" == clean ] ; then
	rm -vrf ./src/wl/linux/obj-*
	exit
else
	export BUILDCFG="$1"
fi

######################################################################
# DO NOT MODIFY BELOW THIS LINE
######################################################################
if [ "${TARGETARCH}" == x86 ] ; then
	export BUILDCFG=${BUILDCFG}
	export STBLINUX=0
elif [ "${TARGETARCH}" == mips ] ; then
	if [ "${TARGETMACH}" == mipseb ] ; then 
		export BUILDCFG=mipseb-mips-${BUILDCFG}
		export STBLINUX=1
	elif [ "${TARGETMACH}" == mipsel ] ; then
		export BUILDCFG=mipsel-mips-${BUILDCFG}
		export STBLINUX=1
	else
		echo "TARGETARCH==${TARGETARCH}"
		echo "TARGETMACH==${TARGETMACH} undefined"
		exit
	fi
elif [ "${TARGETARCH}" == armv7l ]; then
    export BUILDCFG=armv7l-${BUILDCFG}
    export STBLINUX=1
elif [ "${TARGETARCH}" == arm_le ]; then
    export BUILDCFG=armle-${BUILDCFG}
    export STBLINUX=1
elif [ "${TARGETARCH}" == arm ]; then
    export BUILDCFG=armv7l-${BUILDCFG}
    export STBLINUX=1
else
    echo "TARGETARCH==${TARGETARCH}"
    echo "TARGETMACH==${TARGETMACH} undefined"
    exit
fi

#export BRAND=linux-external-wl
export BRAND=linux-external-media
export BUILDARG="$2"



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

if [ "${USBSHIM}" == "1" ] ; then
	make -C ./src/linuxdev LINUXVER=${LINUXVER} TARGETARCH=${TARGETARCH} V=1
	cp -v ./src/linuxdev/obj-bcm_usbshim-${LINUXVER}-${TARGETARCH}/bcm_usbshim.ko ${TARGETDIR}
	cp -v ./src/linuxdev/obj-bcm_usbshim-${LINUXVER}-${TARGETARCH}/bcm_usbshim.ko ${TARGETDIR}/bcm_usbshim-${TARGETMACH}.ko
fi

make -C ./src/wl/linux ${BUILDCFG} FIRMWARE="${FIRMWARE}"  LINUXVER=${LINUXVER} USBSHIM=${USBSHIM} V=1 ${BUILDARG}
if [ "${USBSHIM}" != "1" ] ; then
	cp  -v ./src/wl/linux/obj-${BUILDCFG}-${LINUXVER}/bcm_dbus.ko ${TARGETDIR}
	cp -v ./src/wl/linux/obj-${BUILDCFG}-${LINUXVER}/bcm_dbus.ko ${TARGETDIR}/bcm_dbus-${BUILDCFG}.ko
fi
cp  -v ./src/wl/linux/obj-${BUILDCFG}-${LINUXVER}/wl.ko ${TARGETDIR}
cp  -v ./src/wl/linux/obj-${BUILDCFG}-${LINUXVER}/wl.ko ${TARGETDIR}/${CHIPVER}-${BUILDCFG}-wl.ko

