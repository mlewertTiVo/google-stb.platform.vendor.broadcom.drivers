#!/bin/bash
#Driver load instructions:
#=========================
#insmod dhd.ko
#dhd -i eth download pcie-ag-splitrx.bin bcm943570pcie_p105.txt
#ifconfig eth up
#wl up
#Tune linux TCP Parameters with following commands to get maximum peak TPUT:
#===========================================================================
#Fedore 15
#=========
sysctl -w net.ipv4.tcp_congestion_control=bic
echo 4194304 > /proc/sys/net/core/rmem_max
echo 4194304 > /proc/sys/net/core/wmem_max
echo "4096 4194304 4194304" > /proc/sys/net/ipv4/tcp_rmem
echo "4096 4194304 4194304" > /proc/sys/net/ipv4/tcp_wmem
#Fedora 19
#=========
#echo "1310720" > /proc/sys/net/ipv4/tcp_limit_output_bytes
#sysctl -w net.ipv4.tcp_congestion_control=bic
#echo 4194304 > /proc/sys/net/core/rmem_max
#echo 4194304 > /proc/sys/net/core/wmem_max
#echo "4096 4194304 4194304" > /proc/sys/net/ipv4/tcp_rmem
#echo "4096 4194304 4194304" > /proc/sys/net/ipv4/tcp_wmem


