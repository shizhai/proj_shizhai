#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/ZHONGWANG
	NAME:=ZHONG WANG V1 WIFI BOARD
	PACKAGES:=kmod-usb-core kmod-usb2 kmod-ledtrig-usbdev
endef

define Profile/ZHONGWANG/Description
	Package set optimized for the ZHONG WANG V1.
endef
$(eval $(call Profile,ZHONGWANG))
