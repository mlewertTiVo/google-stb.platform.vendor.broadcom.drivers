#!/bin/bash


make -C src/wl/exe ASD=0 CC=${CC} STRIP=${STRIP} $1

if [ "$1" == clean ] ; then
	exit
fi

mkdir -p ${TARGETDIR}


if [ ${TARGETARCH} == x86 ] ; then
	cp -v src/wl/exe/wl ${TARGETDIR}/wl
	cp -v src/wl/exe/wl ${TARGETDIR}/wl-${TARGETARCH}
elif [ ${TARGETARCH} == mips ] ; then
	cp -v src/wl/exe/wlmips ${TARGETDIR}/wl
	cp -v src/wl/exe/wlmips ${TARGETDIR}/wl-${TARGETARCH}
elif [ ${TARGETARCH} == arm ] ; then
	cp -v src/wl/exe/wlarm ${TARGETDIR}/wl
	cp -v src/wl/exe/wlarm ${TARGETDIR}/wl-${TARGETARCH}	
else
	echo "UNKNOWN TARGETARCH == ${TARGETARCH}"
fi
