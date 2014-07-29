#!/bin/sh
#test before modify this for adapt
ifname=$(cat /etc/cmcs | head -n 1)
ch=$(uci get wireless.@wifi-device[${ifname:4}].channel)
mcsv=$(cat /etc/cmcs | tail -n 1)

echo "the interface is: ${ifname} & the channel value is: ${ch}"

if [ $ch -ge 48 ]
then
	iw dev ${ifname} set bitrates mcs-5 ${mcsv}
	echo "the mcs of 5G value is: ${mcsv}"
else
	iw dev ${ifname} set bitrates mcs-2.4 ${mcsv}
	echo "the mcs of 2.4G value is: ${mcsv}"
fi
