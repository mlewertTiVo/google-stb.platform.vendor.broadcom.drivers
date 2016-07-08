#!/bin/bash


make -C src/usbdev/usbdl CC=${CC} STRIP=${STRIP} $1

if [ "$1" == clean ] ; then
	exit
fi

mkdir -p ${TARGETDIR}
cp -v src/usbdev/usbdl/${TARGETARCH}/bcmdl ${TARGETDIR}
cp -v src/usbdev/usbdl/${TARGETARCH}/bcmdl ${TARGETDIR}/bcmdl-${TARGETARCH}

