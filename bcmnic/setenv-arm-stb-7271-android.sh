##
# You need to modify the exports below for your platform.
##

###################################################################
# setting the path for cross compiler
###################################################################
export LINUXVER=4.1.20-1.0pre
export LINUX_VERSION=4_1_20
export KERNELDIR=/home/ytpark/projects/android/STB/stblinux-4.1.20-1.0pre
export CROSSTOOL=/home/ytpark/projects/android/STB/stbgcc-4.8-1.5/bin
export PATH=${CROSSTOOL}:$PATH
export PATH=${TOOLCHAIN}/bin:$PATH
export LINUXDIR=${KERNELDIR}/linux
export ROOTDIR=${KERNELDIR}/rootfs
export LIBUSB_PATH=${ROOTDIR}/lib/libusb
export TARGETDIR=driver

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

###################################################################
# DO NOT MODIFY BELOW THIS LINE
###################################################################
#source ./setenv.sh

