#
# This script is called by p2papp after tearing down a connection between
# the two peers.  This script de-initializes the network interface for the
# connection.
#
# Copyright (C) 2018, Broadcom Corporation
# All Rights Reserved.
# 
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
# $Id: p2papp_disconnected_uclibc.sh,v 1.12 2010/06/23 02:05:11 Exp $
#

role=$1
interface=$2

echo "=== Kill existing udhcpd:"
if [ -f /var/run/udhcpd-p2p.pid ]
then
   pid=`cat /var/run/udhcpd-p2p.pid`
   echo "kill -TERM $pid"
   kill -TERM $pid
fi


echo "=== Kill existing dhclient:"
if [ -f /var/run/udhcpc-wl0.1.pid ]
then
   pid=`cat /var/run/udhcpc-p2p.pid`
   echo "kill -TERM $pid"
   kill -TERM $pid
fi


echo "/sbin/ifconfig $interface 0.0.0.0"
/sbin/ifconfig $interface 0.0.0.0
