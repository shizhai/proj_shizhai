#!/bin/sh

ip=$1
mac=$2
gateway=$3
dns=$4
mask=$5

uci get network.lan.ipaddr=${ip}
uci get network.lan.macaddr=${mac}
uci get network.lan.gateway=${gateway}
uci get network.lan.dns=${dns}
uci get network.lan.network=${mask}

/etc/init.d/network restart

exit
