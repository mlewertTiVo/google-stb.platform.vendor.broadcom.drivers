#!/bin/bash

make -C dhd/src/wl/exe ASD=0 CC=${CC} STRIP=${STRIP} $1

if [ "$1" != clean ] ; then
mkdir -p ${TARGETDIR}
if [ ${TARGETARCH} == x86 ] ; then
	cp -pv dhd/src/wl/exe/wl ${TARGETDIR}/wl
	cp -pv dhd/src/wl/exe/wl ${TARGETDIR}/wl-x86
elif [ ${TARGETARCH} == mips ] ; then
	cp -pv dhd/src/wl/exe/wlmips ${TARGETDIR}/wl
	cp -pv dhd/src/wl/exe/wlmips ${TARGETDIR}/wl-${TARGETMACH}
elif [ ${TARGETARCH} == arm ] ; then
	cp -pv dhd/src/wl/exe/wlarm ${ANDROID_TOP}/${BCM_VENDOR_STB_ROOT}/bcm_platform/brcm_dhd/tools/wl
else
	echo "UNKNOWN TARGETARCH == ${TARGETARCH}"
fi
else
	if [ "${TARGETDIR}" != "" ]; then
		rm -rfv ${TARGETDIR};
	fi
fi
