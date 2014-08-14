#!/bin/sh
#mv /dev/console /www/cgi-bin/
rm -rf /www/list
sed 's/UTC/CST-8/' /etc/config/system > /system
mv /system /etc/config/system
sed 's/ttyATH0/ttyS0/' /proc/cmdline > /root/cmdline
mount -n --bind -o ro /root/cmdline /proc/cmdline
