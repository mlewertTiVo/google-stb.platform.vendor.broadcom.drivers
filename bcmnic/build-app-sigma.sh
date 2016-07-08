#!/bin/bash


SIGMADIR=src/p2p/test/Sigma/base-dut-maint
make -C ${SIGMADIR} $1

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


cp -v ${SIGMADIR}/dut/${OBJDIR}/wfa_dut ${TARGETDIR}
cp -v ${SIGMADIR}/ca/${OBJDIR}/wfa_ca ${TARGETDIR}
cp -v ${SIGMADIR}/cli/${OBJDIR}/ca_cli ${TARGETDIR}
cp -v ${SIGMADIR}/dut/${OBJDIR}/wfa_dut ${TARGETDIR}/wfa_dut-${TARGETARCH}
cp -v ${SIGMADIR}/ca/${OBJDIR}/wfa_ca ${TARGETDIR}/wfa_ca-${TARGETARCH}
cp -v ${SIGMADIR}/cli/${OBJDIR}/ca_cli ${TARGETDIR}/ca_cli-${TARGETARCH}

