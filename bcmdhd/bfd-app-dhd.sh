#!/bin/bash

make -C dhd/src/dhd/exe CC=${CC} STRIP=${STRIP} $1

if [ "$1" != clean ] ; then
mkdir -p ${TARGETDIR}
if [ ${TARGETARCH} == x86 ] ; then
	cp -pv src/dhd/exe/dhd ${TARGETDIR}/dhd
	cp -pv src/dhd/exe/dhd ${TARGETDIR}/dhd-x86
elif [ ${TARGETARCH} == mips ] ; then
	cp -pv src/dhd/exe/dhdmips ${TARGETDIR}/dhd
	cp -pv src/dhd/exe/dhdmips ${TARGETDIR}/dhd-${TARGETMACH}
elif [ ${TARGETARCH} == arm ] ; then
	cp -pv dhd/src/dhd/exe/dhdarm ${ANDROID_TOP}/${BCM_VENDOR_STB_ROOT}/bcm_platform/brcm_dhd/tools/dhd
else
	echo "UNKNOWN TARGETARCH == ${TARGETARCH}"
fi
else
	if [ "${TARGETDIR}" != "" ]; then
		rm -rfv ${TARGETDIR};
	fi
fi
