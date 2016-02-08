#!/bin/bash


echo ""
echo "------------------------------------------"
echo "cleaning bcmdl"
echo "------------------------------------------"
make -C dhd/src/usbdev/usbdl clean

echo ""
echo "------------------------------------------"
echo "cleaning wl exe"
echo "------------------------------------------"
make -C dhd/src/wl/exe clean

echo ""
echo "------------------------------------------"
echo "cleaning wl sys"
echo "------------------------------------------"
rm -vrf ./dhd/src/dhd/linux/dhd-*

echo ""
echo "------------------------------------------"
echo "cleaning usb shim"
echo "------------------------------------------"
rm -vrf ./dhd/src/linuxdev/obj-*



