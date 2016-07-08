##
# You need to modify the exports below for your platform.
##

###################################################################
# setting the path for cross compiler
###################################################################
export LINUXVER=3.3.8
export KERNELDIR=/projects/hnd/tools/Linux/BCG
export CROSSTOOL=${KERNELDIR}/stbgcc-4.5.4-2.8/bin
export PATH=${CROSSTOOL}:$PATH
export LINUXDIR=${KERNELDIR}/stblinux-3.3-3.3/linux
export ROOTDIR=${KERNELDIR}/rootfs-3.3-3.3/uclinux-rootfs
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
# TARGETMACH=mipseb is BE
# TARGETMACH=mipsel is LE
###################################################################
export TARGETMACH=mipseb
export TARGETARCH=mips
export TARGETENV=linuxmips_be
export CC=mips-linux-gcc
export STRIP=mips-linux-strip
export CROSS_COMPILE=${CROSSTOOL}/mips-linux-	


###################################################################
# DO NOT MODIFY BELOW THIS LINE
###################################################################
source ./setenv.sh

