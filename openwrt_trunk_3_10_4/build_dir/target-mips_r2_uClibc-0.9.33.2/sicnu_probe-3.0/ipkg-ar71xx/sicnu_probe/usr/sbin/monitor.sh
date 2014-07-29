#! /bin/sh
#系统启动时候已经生成/tmp/wlan-interfaces
rm -rf /tmp/allget*
isWifi=$1
vinter=mon0
interface=$2
channel=$3

#2.4G 扫描

if [ ${isWifi} -eq 1 ] #open wifi, single channel scan
then
	uci set wireless.@wifi-device[${interface:4}].disabled=0
	uci set wireless.@wifi-device[${interface:4}].channel=${channel}
	uci commit
	#wifi up
	/etc/init.d/network restart
	iw dev ${interface} interface add ${vinter} type monitor;
	echo "add mon0 and no down ${interface}, that mean single channel scan ..."
else	#multi scan
	uci set wireless.@wifi-device[${interface:4}].disabled=0
	uci commit
	/etc/init.d/network restart
	ifconfig ${interface} up
	iw dev ${interface} interface add ${vinter} type monitor;
	ifconfig ${interface} down
	echo "add mon0 and down ${interface}, that mean muti channel scan ..."
fi
ifconfig ${vinter} up

exit
