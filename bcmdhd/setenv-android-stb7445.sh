##
# You need to modify the exports below for your platform.
##

###################################################################
# setting the path for cross compiler
###################################################################
export KERNELDIR=${ANDROID_TOP}/kernel/private/bcm-97xxx
export LINUXDIR=${KERNELDIR}/linux${LOCAL_LINUX_VERSION}
export ROOTDIR=${KERNELDIR}/rootfs

KVER=`make --no-print-directory -C ${LINUXDIR} kernelversion`
KCC=`grep "CONFIG_CROSS_COMPILE" ${LINUX_OUT_1ST_ARCH}/.config | cut -d"\"" -f2`
export LINUXVER=${KVER}

#export EXTERNAL_OPENSSL=0
#export EXTERNAL_OPENSSL_BASE=${KERNELDIR}/openssl
export LIBUSB_PATH=${ROOTDIR}/lib/libusb
export TARGETDIR=driver

###################################################################
# USBSHIM=1 if kernel is greater than or equal to 2.6.18
# USBSHIM=0 if kernel is less than 2.6.18 or using DBUS kernel object.
###################################################################
export USBSHIM=0

###################################################################
# Machine and Architecture specifics
###################################################################
export TARGETMACH=armle
export TARGETARCH=arm
export TARGETENV=linuxarm
export CC=${KCC}gcc
export STRIP=${KCC}strip
export CROSS_COMPILE=${KCC}


###################################################################
# DO NOT MODIFY BELOW THIS LINE
###################################################################
source ./setenv.sh

