#!/bin/bash


./build-app-bcmdl.sh clean
./build-app-p2p.sh clean
./build-app-sigma.sh clean
./build-app-wl.sh clean
./build-app-wps.sh clean


echo ""
echo "------------------------------------------"
echo "cleaning p2p lib and sigma test"
echo "------------------------------------------"
rm -rf ./src/p2p/p2plib/linux/obj*
rm -rf ./src/p2p/p2plib/linux/sampleapp/obj*
rm -rf ./src/p2p/test/Sigma/base-dut-maint/ca/obj*
rm -rf ./src/p2p/test/Sigma/base-dut-maint/cli/obj*
rm -rf ./src/p2p/test/Sigma/base-dut-maint/dut/obj*
rm -rf ./src/p2p/test/Sigma/base-dut-maint/lib/obj*
rm -rf ./src/p2p/test/Sigma/base-dut-maint/scripts/obj*

echo ""
echo "------------------------------------------"
echo "cleaning bmcdl"
echo "------------------------------------------"
rm -rf ./src/usbdev/usbdl/arm*
rm -rf ./src/usbdev/usbdl/mips*

echo ""
echo "------------------------------------------"
echo "cleaning wfd_capd"
echo "------------------------------------------"
rm -rf ./src/apps/wfd_capd/linux/wfd_capd_ie/obj*

echo ""
echo "------------------------------------------"
echo "cleaning wl"
echo "------------------------------------------"
rm -rf src/wl/exe/dongle_noasd*
rm -rf src/wl/exe/serial_noasd*
rm -rf src/wl/exe/socket_noasd*
rm -rf src/wl/exe/wifi_noasd*
rm -rf src/wl/exe/obj*
rm -f src/wl/exe/wlarm*
rm -f src/wl/exe/wlmips*


echo ""
echo "------------------------------------------"
echo "cleaning bwl"
echo "------------------------------------------"
#make -C src/apps/bwl/linux clean

echo ""
echo "------------------------------------------"
echo "cleaning wps"
echo "------------------------------------------"
rm -rf ./src/wps/wpscli/linux/obj*
rm -rf ./src/wps/wpsapi/linux/release*
rm -rf ./src/wps/linux/enr/arm*
rm -rf ./src/wps/linux/enr/mips*


# if this is a DHD release
if [ -d ./src/dhd ]; then
	./build-app-dhd.sh clean
	./build-drv-fd.sh clean
	./build-drv-fd-mfg.sh clean
	rm -rf ./src/dhd/exe/mips*
	rm -rf ./src/dhd/exe/arm*
	rm -rf ./src/dhd/exe/dhdmips*
	rm -rf ./src/dhd/exe/dhdarm*
	rm -rf ./src/dhd/linux/cdc-usb-*
	rm -rf ./src/dhd/linux/msgbuf-pciefd-*
else
	./build-drv-bmac.sh clean
	./build-drv-nic.sh clean
fi


