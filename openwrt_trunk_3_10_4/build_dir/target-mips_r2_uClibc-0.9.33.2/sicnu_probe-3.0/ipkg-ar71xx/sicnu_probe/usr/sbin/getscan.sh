#!/bin/sh

path2_4=/tmp/allget

#if it does not scan all channels
alls=$(cat /etc/cscan | tail -n 2 | head -n 1 )
alli=$(cat /etc/cscan | head -n 1)

#get the ap interface
ifap=$(cat /etc/cmcs | head -n 1)
#2.4G scan results process
if [ -f ${path2_4} ]
then
cp ${path2_4}  /tmp/allgets2_4_tmp
cat /tmp/myMAC24 > /tmp/pesGet24

#if it does not scan all channels
if [ $alls=="01" ] || [ $alli=="none" ]
then
iw dev ${ifap} station dump | grep "^Station" | awk '{print $2}' > /tmp/linkmac 
iw dev ${ifap} station dump | grep "signal avg" | awk '{print $3}' > /tmp/linkrssi
awk 'NR==FNR{a[i]=$0;i++}NR>FNR{print a[j]" "$0;j++}' /tmp/linkmac /tmp/linkrssi >> /tmp/pesGet24
fi

awk 'BEGIN{FS=", "} {if(($6<0)&&($6>-90)&&($6!="")) print $1,$2,$6}' /tmp/allgets2_4_tmp > /tmp/sel1

#APs get 
awk '{if($1=="BEACON" || $1=="PROBRP") print $2}' /tmp/sel1 | sort | uniq > /tmp/apsGet24 
awk '{print $2,$3}' /tmp/sel1 | sed  '/^[f][f]\:[f][f]\:[f][f]\:[f][f]\:/d' | sed  '/^[0][0]\:[0][0]\:[0][0]\:[0][0]\:/d' | sed  '/^[0]\.[0]\.[0]\./d' | sort -k 1 > /tmp/sel1_tmp
mv /tmp/sel1_tmp  /tmp/sel1

for LINE in `cat /tmp/apsGet24`
do
	#get the newest scaned phone data
	sed -i "/^${LINE}/d" /tmp/sel1
done

sed -i '/^$/d' /tmp/sel1

#only PEs and just get the uniq mac to group
awk '{print $1}' /tmp/sel1 | uniq -w17 > /tmp/sel1_tmp

:> /tmp/rssi
for LINE in `cat /tmp/sel1_tmp`
do
	cat /tmp/sel1 | awk 'BEGIN{sum=0;num=0}{if($1=="'$LINE'"){sum+=$2;num+=1}}END{print sum/num}' | awk -F. '{print $1}' >> /tmp/rssi
done
awk 'NR==FNR{a[i]=$0;i++}NR>FNR{print a[j]" "$0;j++}' /tmp/sel1_tmp /tmp/rssi >> /tmp/pesGet24
rm -rf ${path2_4}
fi
exit
