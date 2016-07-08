#!/bin/bash



if [ "$1" == "" ] ; then
	echo ""
	echo "************************************************"
	echo "USAGE:   build-drv-fd-mfg.sh <TARGET>"
	echo ""
	echo "eg: build-drv-fd-mfg.sh clean"
	echo ""
	echo "PCIE Manufacture Targets"	
	echo "eg: build-drv-fd-mfg.sh dhd-msgbuf-pciefd-mfp"
	echo "eg: build-drv-fd-mfg.sh dhd-msgbuf-pciefd-mfp-secdma"
	echo "eg: build-drv-fd-mfg.sh dhd-msgbuf-pciefd-reqfw-mfp"
	echo "eg: build-drv-fd-mfg.sh dhd-msgbuf-pciefd-reqfw-mfp-secdma"
	echo ""
	echo "USB Manufacture Targets"	
	echo "eg: build-drv-fd-mfg.sh dhd-cdc-usb-comp-gpl"
	echo "eg: build-drv-fd-mfg.sh dhd-cdc-usb-reqfw-comp-gpl"
	echo ""
	exit
elif [ "$1" == clean ] ; then
	rm -rf ./src/dhd/linux/dhd-*
	exit
else
	export BUILDCFG="$1"
fi

export BRAND=linux-mfgtest-media
export BUILDARG="$2"


######################################################################
# DO NOT MODIFY BELOW THIS LINE
######################################################################
if [ "${TARGETARCH}" == x86 ] ; then
	export BUILDCFG=${BUILDCFG}
	export STBLINUX=0
elif [ "${TARGETARCH}" == mips ] ; then
	if [ "${TARGETMACH}" == mipseb ] ; then 
		export BUILDCFG=${BUILDCFG}-mips
	elif [ "${TARGETMACH}" == mipsel ] ; then
		export BUILDCFG=${BUILDCFG}-mips
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


# use -debug for manufaturing driver and it must be the last option of the build cfg
export BUILDCFG=${BUILDCFG}-debug


echo ""
echo "************************************************"
echo "      BUILDCFG = "${BUILDCFG}"                  "
echo "      BUILDARG = "${BUILDARG}"                  "
echo "************************************************"
echo ""
sleep 3

mkdir -p ${TARGETDIR}
mkdir -p ./src/dhd/linux/${BUILDCFG}-${LINUXVER}

if [ ! -f ./src/include/epivers.h ]; then
	cd ./src/include
	make
	cd -
fi


make -C ./src/dhd/linux ${BUILDCFG} WLTEST=${WLTEST} WLLXIW=${WLLXIW} LINUXVER=${LINUXVER} ${BUILDARG} CC=${CC} STRIP=${STRIP}

if [ -f ./src/dhd/linux/${BUILDCFG}-${LINUXVER}/dhd.ko ]; then
	cp  -v ./src/dhd/linux/${BUILDCFG}-${LINUXVER}/dhd.ko ${TARGETDIR}
	cp  -v ./src/dhd/linux/${BUILDCFG}-${LINUXVER}/dhd.ko ${TARGETDIR}/${BUILDCFG}-dhd.ko
fi

if [ -f ./src/dhd/linux/${BUILDCFG}-${LINUXVER}/bcmdhd.ko ]; then
	cp  -v ./src/dhd/linux/${BUILDCFG}-${LINUXVER}/bcmdhd.ko ${TARGETDIR}
	cp  -v ./src/dhd/linux/${BUILDCFG}-${LINUXVER}/bcmdhd.ko ${TARGETDIR}/${BUILDCFG}-bcmdhd.ko
fi

