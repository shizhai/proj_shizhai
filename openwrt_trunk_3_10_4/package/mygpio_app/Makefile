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
PKG_NAME:=mygpio_app
PKG_RELEASE:=1

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
endef

#Uncomment portion below for Kacmikaze and delete DESCRIPTION variable above
define Package/$(PKG_NAME)/description
	the software of OpenWrt test gpio controled by hand
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

#Specify where and how to install the program.Since we only have on file,
#the smartHome executable,install it by copying it to the /bin directory on
#the router.The $(1) variable represents the root directory on the router running
#OpenWrt.The $(INSTALL_DIR) variable contains a command to prepare the install
#directory if it does not already exit.Likewise $(INSTALL_BIN) contains the 
#command to copy the binary file from its current location (in our case the build
#directory) to the install directory
define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
