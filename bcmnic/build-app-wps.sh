#!/bin/bash

cd src/wps/common/include/
make
cd -

cd src/wps/linux/enr/
make -f wps_enr_app.mk CC=${CC} STRIP=${STRIP} EXTERNAL_OPENSSL=${EXTERNAL_OPENSSL} BCM_WPS_IOTYPECOMPAT=1 $1
cd -

cd src/wps/wpsapi/linux
make CC=${CC} STRIP=${STRIP} EXTERNAL_OPENSSL=${EXTERNAL_OPENSSL} BCM_WPS_IOTYPECOMPAT=1 $1
cd -

cd src/wps/wpscli/linux
make CC=${CC} STRIP=${STRIP} EXTERNAL_OPENSSL=${EXTERNAL_OPENSSL} BCM_WPS_IOTYPECOMPAT=1 $1
cd -

if [ "$1" == clean ] ; then
	exit
fi

mkdir -p ${TARGETDIR}


cp -v src/wps/linux/enr/${CC}/wpsenr ${TARGETDIR}
cp -v src/wps/linux/enr/${CC}/wpsenr ${TARGETDIR}/wpsenr-${TARGETARCH}

cp -v src/wps/linux/enr/${CC}/wpsreg ${TARGETDIR}
cp -v src/wps/linux/enr/${CC}/wpsreg ${TARGETDIR}/wpsreg-${TARGETARCH}

cp -v src/wps/wpsapi/linux/release/${TARGETARCH}/wpsapitester ${TARGETDIR}
cp -v src/wps/wpsapi/linux/release/${TARGETARCH}/wpsapitester ${TARGETDIR}/wpsapitester-${TARGETARCH}

cp -v src/wps/wpscli/linux/obj/${TARGETARCH}-debug/wpscli_test_cmd ${TARGETDIR}
cp -v src/wps/wpscli/linux/obj/${TARGETARCH}-debug/wpscli_test_cmd ${TARGETDIR}/wpscli_test_cmd-${TARGETARCH}

