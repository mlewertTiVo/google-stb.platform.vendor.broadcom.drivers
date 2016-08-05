#
# This script is called by p2papp after a connection is established between
# the two peers.  This script:
# - sets up the network interface for the connection
# - verifies the connection by doing a ping and a file transfer.
# When this script exits, p2papp will tear down the connection.
#
# Copyright (C) 2016, Broadcom Corporation
# All Rights Reserved.
# 
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
# $Id: p2papp_connected_uclibc.sh,v 1.28 2010/07/31 02:20:55 Exp $
#

role=$1
interface_name=$2
enable_ping=$3

echo "Entering p2papp_connected_uclibc.sh"


echo "=== Kill existing udhcpd:"
if [ -f /var/run/udhcpd-p2p.pid ]
then
   pid=`cat /var/run/udhcpd-p2p.pid`
   echo "kill -TERM $pid"
   kill -TERM $pid
fi

echo "=== Kill existing udhcpc:"
if [ -f /var/run/udhcpc-p2p.pid ]
then
   pid=`cat /var/run/udhcpc-p2p.pid`
   echo "kill -TERM $pid"
   kill -TERM $pid
fi


echo "=== Clearing dhcpd leases"
dhcpd_leases_file="/tmp/udhcpd-p2p.leases"
if [ -f ${dhcpd_leases_file} ]
then   
   echo "cp -f ${dhcpd_leases_file} ${dhcpd_leases_file}.bak"
   cp -f ${dhcpd_leases_file} ${dhcpd_leases_file}.bak
   echo "rm -f ${dhcpd_leases_file}"
   rm -f ${dhcpd_leases_file}
fi
if [ -f ${dhcpd_leases_file}~ ]
then
   echo "rm -f ${dhcpd_leases_file}~"
   rm -f ${dhcpd_leases_file}~
fi

#echo === Clearing dhclient leases
#dhclient_leases_file="/var/lib/dhclient/dhclient.leases"
#if [ -a ${dhclient_leases_file} ]
#then   
#   cp -f ${dhclient_leases_file} ${dhclient_leases_file}.bak
#   echo rm -f ${dhclient_leases_file}
#   rm -f ${dhclient_leases_file}
#fi


#
# Actions for a standlone Group Owner 
#
if [ "$role" == "standalone_go" ]; then
   echo "=== Running DHCP Server"
   
   echo "/sbin/ifconfig $interface_name"
   /sbin/ifconfig $interface_name
   
   echo "touch /tmp/udhcpd-p2p.leases"
   touch /tmp/udhcpd-p2p.leases
   
   echo "/usr/sbin/udhcpd ./udhcpd-p2p.conf"
   /usr/sbin/udhcpd ./udhcpd-p2p.conf
fi

#
# Actions for the AP peer in a P2P connection
#
if [ "$role" == "ap" ]; then
   # On the AP peer, our IP address is statically assigned by the P2P Library
   # to the IP address required by the DHCP server.
   echo ""
   echo "=== Running DHCP Server"
   echo ""
   
   echo "touch /tmp/udhcpd-p2p.leases"
   touch /tmp/udhcpd-p2p.leases
   
   echo "/usr/sbin/udhcpd ./udhcpd-p2p.conf"
   /usr/sbin/udhcpd ./udhcpd-p2p.conf
fi

#
# Actions for the STA peer in a P2P connection
#
if [ "$role" == "sta" ]; then
   echo "/sbin/ifconfig $interface_name up"
   /sbin/ifconfig $interface_name up

   # wait for DHCP server
   sleep 3
   echo ""
   echo "=== Running DHCP Client"
   echo ""
   echo "/usr/sbin/udhcpc -i $interface_name -p /var/run/udhcpc-p2p.pid -s ./udhcpc-p2p.script"
   /usr/sbin/udhcpc -i $interface_name -p /var/run/udhcpc-p2p.pid -s ./udhcpc-p2p.script
   
   echo "=== DCHP Client obtained IP address: "
   /sbin/ifconfig $interface_name


   if [ "$enable_ping" == "1" ]; then
      echo ""
      echo "=== Check that we can ping the peer:"
      echo ""
      echo "ping -c 3 192.168.16.1"
      ping -c 3 192.168.16.1
   fi   
fi
