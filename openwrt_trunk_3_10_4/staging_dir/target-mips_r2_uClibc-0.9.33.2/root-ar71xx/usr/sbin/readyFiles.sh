#!/bin/sh
rm -rf /tmp/testPrint
#echo 0 > /tmp/horst-finish
ifconfig eth0 | awk 'BEGIN{getline;print $5}' > /tmp/myMAC24
#cp /etc/wlan-interfaces /tmp/wlans-for-monitor

