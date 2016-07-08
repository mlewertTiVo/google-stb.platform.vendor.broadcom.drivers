#!/bin/bash


P2PDIR=src/p2p/p2plib/linux/sampleapp
make -C ${P2PDIR} BCM_P2P_IOTYPECOMPAT=1 $1


if [ "$1" == clean ] ; then
	exit
fi

if [ ${TARGETARCH} == x86 ] ; then
	OBJDIR=obj/${TARGETARCH}-debug-intsec
elif [ ${TARGETARCH} == mips ] ; then
	OBJDIR=obj/${TARGETARCH}-debug-intsec
elif [ ${TARGETARCH} == arm ]; then
	OBJDIR=obj/${TARGETARCH}-debug-intsec
else
	echo "TARGETARCH==${TARGETARCH}"
	echo "TARGETMACH==${TARGETMACH} undefined"
	exit
fi



mkdir -p ${TARGETDIR}

cp -v ${P2PDIR}/${OBJDIR}/bcmp2papp ${TARGETDIR}
cp -v ${P2PDIR}/${OBJDIR}/bcmp2papp ${TARGETDIR}/bcmp2papp-${TARGETARCH}

