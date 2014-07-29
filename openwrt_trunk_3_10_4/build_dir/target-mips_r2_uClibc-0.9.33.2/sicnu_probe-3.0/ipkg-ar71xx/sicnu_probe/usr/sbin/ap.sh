#!/bin/sh
onff=$1
ifname=$2
mac=$3
cha=$4
htm=$5
pow=$6
hwm=$7
ssid=$8
enc=$9
key=${10}

uci set wireless.@wifi-device[${ifname:4}].disable=${onff}
uci set wireless.@wifi-device[${ifname:4}].macaddr=${mac}
uci set wireless.@wifi-device[${ifname:4}].channel=${cha}
uci set wireless.@wifi-device[${ifname:4}].htmode=${htm}
uci set wireless.@wifi-device[${ifname:4}].txpower=${pow}
uci set wireless.@wifi-device[${ifname:4}].hwmode=${hwm}
uci set wireless.@wifi-iface[${ifname:4}].ssid=${ssid}
uci set wireless.@wifi-iface[${ifname:4}].encryption=${enc}
if [[ ${enc} != "none" ]]
then
	uci set wireless.@wifi-iface[${ifname:4}].key=${key}
fi

/etc/init.d/network restart
exit
