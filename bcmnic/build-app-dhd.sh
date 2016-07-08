#!/bin/bash


make -C src/dhd/exe CC=${CC} STRIP=${STRIP} $1

if [ "$1" == clean ] ; then
	exit
fi

mkdir -p ${TARGETDIR}

if [ ${TARGETARCH} == x86 ] ; then
	cp -v src/dhd/exe/dhd ${TARGETDIR}/dhd
	cp -v src/dhd/exe/dhd ${TARGETDIR}/dhd-${TARGETARCH}
elif [ ${TARGETARCH} == mips ] ; then
	cp -v src/dhd/exe/dhdmips ${TARGETDIR}/dhd
	cp -v src/dhd/exe/dhdmips ${TARGETDIR}/dhd-${TARGETARCH}
elif [ ${TARGETARCH} == arm ] ; then
	cp -v src/dhd/exe/dhdarm ${TARGETDIR}/dhd
	cp -v src/dhd/exe/dhdarm ${TARGETDIR}/dhd-${TARGETARCH}
else
	echo "UNKNOWN TARGETARCH == ${TARGETARCH}"
fi
