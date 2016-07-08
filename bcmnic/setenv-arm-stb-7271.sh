##
# You need to modify the exports below for your platform.
##

###################################################################
# setting the path for cross compiler
###################################################################
export LINUXVER=4.1.19-1.0
export LINUX_VERSION=4_1_19
export KERNELDIR=/projects/hnd/tools/linux/BCG
export CROSSTOOL=${KERNELDIR}/stbgcc-4.8-1.5/bin
export PATH=${CROSSTOOL}:$PATH
export PATH=${TOOLCHAIN}/bin:$PATH
export LINUXDIR=${KERNELDIR}/stblinux-4.1-1.0/linux
export ROOTDIR=${KERNELDIR}/stblinux-4.1-1.0/rootfs
#export LINUX=${KERNELDIR}/stblinux-3.14-1.8-netfilter/linux
#export ROOTDIR=${KERNELDIR}/rootfs-3.14-1.8-netfilter/rootfs
#export EXTERNAL_OPENSSL=0
#export EXTERNAL_OPENSSL_BASE=${KERNELDIR}/openssl
export LIBUSB_PATH=${ROOTDIR}/lib/libusb
export TARGETDIR=${LINUXVER}

###################################################################
# STBLINUX=0; not use for STB
# STBLINUX=1; use for STB
###################################################################
export STBLINUX=1

###################################################################
# USBSHIM=0 if bcmdbus is used.  This is used for STB LINUX
###################################################################
export USBSHIM=0

###################################################################
# Machine and Architecture specifics
# TARGETMACH=armeb is BE
# TARGETMACH=arm is LE
###################################################################
export TARGETMACH=arm
export TARGETARCH=arm
export TARGETENV=linuxarm
export CC=arm-linux-gcc
export STRIP=arm-linux-strip
export CROSS_COMPILE=${CROSSTOOL}/arm-linux-

