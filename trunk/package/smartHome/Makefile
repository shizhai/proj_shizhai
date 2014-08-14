#####################################################
#Openwrt Makefile for smartHome program
#
#Most of the variables usred here are defined
#in the include directives below.We just need to  
#specify a basic description of the package,
#where to buil our program,where to find the 
#source files,and where to install the compiled 
#program on the router
#
#
#
#Be very careful of sapacing in this file
#Indents should be tabs,not spaces,and there
#should be no trailing whitespace in lines that 
#are not commneted
#
#####################################################

include $(TOPDIR)/rules.mk

#Name and release number of this package
PKG_NAME:=smartHome
PKG_RELEASE:=2

#This specifiles the directory where we're goning to build the program.
#The root build directory,$(BUILD_DIR),is by default the build_mipsel
#directory in your OpenWrt SDK directory
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
include $(INCLUDE_DIR)/package.mk

#Specify package information for this program.
#the variables defined there should be self explanatory.
#if you are running Kamikaze,delete the DESCRIPTION
#variable below and uncommnet the Kamikaze define 
#directive for the description below
define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=$(PKG_NAME) Program
	DEPENDS:=+libpthread
endef

#Uncomment portion below for Kacmikaze and delete DESCRIPTION variable above
define Package/$(PKG_NAME)/description
	the software of OpenWrt Router to manage light
endef

#Specify what needs to be done to prepare for building the package.
#In our case,we need to copy the source files to the build direcory.
#This is NOT the default.The default uses the PKG_SOURCE_URL and the 
#PKG_SOURCE which is not defined here to download the source from the web.
#In order to just build a simple program that we have just written,it is 
#much easier to do it this way.
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/preinst
#!/bin/sh
#set the config of environment
rm -rf /www/list
sed 's/UTC/CST-8/' /etc/config/system > /system
mv /system /etc/config/system
sed 's/::askconsole/#::askconsole/' /etc/inittab > /inittab
mv /inittab /etc/inittab
sed 's/ttyATH0/ttyS0/' /proc/cmdline > /root/cmdline
mount -n --bind -o ro /root/cmdline /proc/cmdline
#create the /etc/config/files
#create networko
echo "config interface 'loopback'" > /etc/config/networko
echo "	option ifname 'lo'" >> /etc/config/networko
echo "	option proto 'static'" >> /etc/config/networko
echo "	option ipaddr '127.0.0.1'" >> /etc/config/networko
echo "	option netmask '255.0.0.0'" >> /etc/config/networko

echo "config interface 'wan'" >> /etc/config/networko
echo "	option ifname 'eth0'" >> /etc/config/networko
echo "	option proto 'dhcp'" >> /etc/config/networko

echo "config interface 'lan'" >> /etc/config/networko
echo "	option _orig_ifname 'wlan0'" >> /etc/config/networko
echo "	option _orig_bridge 'false'" >> /etc/config/networko
echo "	option proto 'static'" >> /etc/config/networko
echo "	option ipaddr '192.168.1.1'" >> /etc/config/networko
echo "	option netmask '255.255.255.0'" >> /etc/config/networko

#create networkb
echo "config interface 'loopback'" > /etc/config/networkb
echo "	option ifname 'lo'" >> /etc/config/networkb
echo "	option proto 'static'" >> /etc/config/networkb
echo "	option ipaddr '127.0.0.1'" >> /etc/config/networkb
echo "	option netmask '255.0.0.0'" >> /etc/config/networkb

echo "config globals 'globals'" >> /etc/config/networkb
echo "	option ula_prefix 'fd92:0f58:53b4::/48'" >> /etc/config/networkb

echo "config interface 'lan'" >> /etc/config/networkb
echo "	option type 'bridge'" >> /etc/config/networkb
echo "	option proto 'static'" >> /etc/config/networkb
echo "	option ipaddr '192.168.1.1'" >> /etc/config/networkb
echo "	option netmask '255.255.255.0'" >> /etc/config/networkb
echo "	option ip6assign '60'" >> /etc/config/networkb

echo "config interface 'wan'" >> /etc/config/networkb
echo "	option ifname 'eth0'" >> /etc/config/networkb
echo "	option macaddr '00:21:fe:11:22:33'" >> /etc/config/networkb
echo "	option proto 'dhcp'" >> /etc/config/networkb

echo "config interface 'wwan'" >> /etc/config/networkb
echo "	option proto 'dhcp'" >> /etc/config/networkb

#create wirelesso

echo "config wifi-device 'radio0'" > /etc/config/wirelesso
echo "	option type 'mac80211'" >> /etc/config/wirelesso
echo "	option channel '11'">> /etc/config/wirelesso
echo "	option hwmode '11ng'">> /etc/config/wirelesso
echo "	option path 'platform/ar933x_wmac'">> /etc/config/wirelesso
echo "	option htmode 'HT20'">> /etc/config/wirelesso
echo "	list ht_capab 'SHORT-GI-20'">> /etc/config/wirelesso
echo "	list ht_capab 'SHORT-GI-40'">> /etc/config/wirelesso
echo "	list ht_capab 'RX-STBC1'">> /etc/config/wirelesso
echo "	list ht_capab 'DSSS_CCK-40'">> /etc/config/wirelesso
echo "	option txpower '27'">> /etc/config/wirelesso
echo "	option country 'US'">> /etc/config/wirelesso

echo "config wifi-iface">> /etc/config/wirelesso
echo "	option network 'lan'">> /etc/config/wirelesso
echo "	option ssid 'openwrt'">> /etc/config/wirelesso
echo "	option device 'radio0'">> /etc/config/wirelesso
echo "	option mode 'ap'">> /etc/config/wirelesso
echo "	option encryption 'none'">> /etc/config/wirelesso

#create wirelessb
echo "config wifi-device 'radio0'"> /etc/config/wirelessb
echo "	option type 'mac80211'">> /etc/config/wirelessb
echo "	option channel '11'">> /etc/config/wirelessb
echo "	option hwmode '11ng'">> /etc/config/wirelessb
echo "	option path 'platform/ar933x_wmac'">> /etc/config/wirelessb
echo "	option htmode 'HT20'">> /etc/config/wirelessb
echo "	list ht_capab 'SHORT-GI-20'">> /etc/config/wirelessb
echo "	list ht_capab 'SHORT-GI-40'">> /etc/config/wirelessb
echo "	list ht_capab 'RX-STBC1'">> /etc/config/wirelessb
echo "	list ht_capab 'DSSS_CCK-40'">> /etc/config/wirelessb
echo "	option disabled '0'">> /etc/config/wirelessb
echo "	option txpower '27'">> /etc/config/wirelessb
echo "	option country 'US'">> /etc/config/wirelessb

echo "config wifi-iface">> /etc/config/wirelessb
echo "	option network 'wwan'">> /etc/config/wirelessb
echo "	option ssid 'OpenWrtAP'">> /etc/config/wirelessb
echo "	option encryption 'psk'">> /etc/config/wirelessb
echo "	option device 'radio0'">> /etc/config/wirelessb
echo "	option mode 'sta'">> /etc/config/wirelessb
echo "	option bssid '14:CF:92:3C:1A:5C'">> /etc/config/wirelessb
echo "	option key '2013SERVER_305'">> /etc/config/wirelessb

#create restart my program
#echo "::respawn:/usr/sbin/smartHome">>/etc/inittab

endef

define Package/$(PKG_NAME)/postrm
#!/bin/sh
#reset the pre environment
rm -rf /www/list
rm -rf /www/cgi-bin/*.cfg
rm -rf /www/cgi-bin/wifi_list
rm -rf /www/cgi-bin/is*

sed 's/#::askconsole/::askconsole/' /etc/inittab > /inittab
mv /inittab /etc/inittab

sed 's/ttyS0/ttyATH0/' /proc/cmdline > /root/cmdline
mount -n --bind -o ro /root/cmdline /proc/cmdline

#restore restart my program
#sed '4d' /etc/inittab > /etc/inittab1
#mv /etc/inittab1 /etc/inittab

#netconfig file reset 
cp -f /etc/config/networko /etc/config/network
cp -f /etc/config/wirelesso /etc/config/wireless
rm -rf /etc/config/networko
rm -rf /etc/config/networkb
rm -rf /etc/config/wirelesso
rm -rf /etc/config/wirelessb
endef

#Specify where and how to install the program.Since we only have on file,
#the smartHome executable,install it by copying it to the /bin directory on
#the router.The $(1) variable represents the root directory on the router running
#OpenWrt.The $(INSTALL_DIR) variable contains a command to prepare the install
#directory if it does not already exit.Likewise $(INSTALL_BIN) contains the 
#command to copy the binary file from its current location (in our case the build
#directory) to the install directory
define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	#$(CP) /lib/libpthread.so.0 $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
