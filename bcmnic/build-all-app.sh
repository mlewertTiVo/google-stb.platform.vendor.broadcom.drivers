#!/bin/bash

./build-app-bcmdl.sh
./build-app-p2p.sh
./build-app-sigma.sh
./build-app-wl.sh
./build-app-wps.sh

# if this is a DHD release then build the DHD app
if [ -d ./src/dhd/ ]; then
	./build-app-dhd.sh
fi



