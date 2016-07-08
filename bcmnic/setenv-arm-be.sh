##
# You need to modify the exports below for your platform.
##

###################################################################
# setting the path for cross compiler
###################################################################
export LINUXVER=3.14.13
export KERNELDIR=/projects/hnd/tools/Linux/BCG
export CROSSTOOL=${KERNELDIR}/stbgcc-4.8-1.0/bin
export PATH=${CROSSTOOL}:$PATH
export LINUXDIR=${KERNELDIR}/stblinux-3.14-1.2/linux
export ROOTDIR=${KERNELDIR}/rootfs-3.14-1.2/rootfs
#export EXTERNAL_OPENSSL=0
#export EXTERNAL_OPENSSL_BASE=${KERNELDIR}/openssl
export LIBUSB_PATH=${ROOTDIR}/lib/libusb
export TARGETDIR=${LINUXVER}

###################################################################
# USBSHIM=1 if kernel is greater than or equal to 2.6.18
# USBSHIM=0 if kernel is less than 2.6.18 or using DBUS kernel object.
# USBSHIM=0 if bcmdbus is used.
###################################################################
export USBSHIM=0

###################################################################
# Machine and Architecture specifics
# TARGETMACH=armbe is BE
# TARGETMACH=arm is LE
###################################################################
export TARGETMACH=armeb
export TARGETARCH=arm
export TARGETENV=linuxarm
export CC=arm-linux-gcc
export STRIP=arm-linux-strip
export CROSS_COMPILE=${CROSSTOOL}/arm-linux-

###################################################################
# DO NOT MODIFY BELOW THIS LINE
###################################################################
source ./setenv.sh

