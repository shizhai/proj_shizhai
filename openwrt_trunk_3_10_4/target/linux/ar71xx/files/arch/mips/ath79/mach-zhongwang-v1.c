/*
 *  TP-LINK TL-ZHONGWANG_V1 board support
 *
 *  Copyright (C) 2012 Gabor Juhos <juhosg@openwrt.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/pci.h>
#include <linux/phy.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/ath9k_platform.h>
#include <linux/ar8216_platform.h>

#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "dev-ap9x-pci.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-spi.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define ZHONGWANG_V1_GPIO_LED_WLAN		13
#define ZHONGWANG_V1_GPIO_LED_SYSTEM	12

#define ZHONGWANG_V1_GPIO_BTN_WPS		16
#define ZHONGWANG_V1_GPIO_BTN_RFKILL		17


#define ZHONGWANG_V1_KEYS_POLL_INTERVAL	20	/* msecs */
#define ZHONGWANG_V1_KEYS_DEBOUNCE_INTERVAL	(3 * ZHONGWANG_V1_KEYS_POLL_INTERVAL)

#define ZHONGWANG_V1_MAC0_OFFSET		0
#define ZHONGWANG_V1_WMAC_CALDATA_OFFSET	0x1000

static const char *zhongwang_v1_part_probes[] = {
	"tp-link",
	NULL,
};

static struct flash_platform_data zhongwang_v1_flash_data = {
	.part_probes	= zhongwang_v1_part_probes,
};

static struct gpio_led zhongwang_v1_leds_gpio[] __initdata = {
	{
		.name		= "zhongwang:green:wlan",
		.gpio		= ZHONGWANG_V1_GPIO_LED_WLAN,
		.active_low	= 1,
	},
	{
		.name		= "zhongwang:green:system",
		.gpio		= ZHONGWANG_V1_GPIO_LED_SYSTEM,
		.active_low	= 1,
	},
};

static struct gpio_keys_button zhongwang_v1_gpio_keys[] __initdata = {
	{
		.desc		= "QSS button",
		.type		= EV_KEY,
		.code		= KEY_WPS_BUTTON,
		.debounce_interval = ZHONGWANG_V1_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= ZHONGWANG_V1_GPIO_BTN_WPS,
		.active_low	= 1,
	},
	{
		.desc		= "RFKILL switch",
		.type		= EV_SW,
		.code		= KEY_RFKILL,
		.debounce_interval = ZHONGWANG_V1_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= ZHONGWANG_V1_GPIO_BTN_RFKILL,
	},
};

static void __init zhongwang_v1_setup(void)
{
	u8 *mac = (u8 *) KSEG1ADDR(0x1f01fc00);
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);
	u8 tmpmac[ETH_ALEN];

	ath79_register_m25p80(&zhongwang_v1_flash_data);
	
	ath79_register_leds_gpio(-1, ARRAY_SIZE(zhongwang_v1_leds_gpio),
				 zhongwang_v1_leds_gpio);
	ath79_register_gpio_keys_polled(-1, ZHONGWANG_V1_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(zhongwang_v1_gpio_keys),
					zhongwang_v1_gpio_keys);

	ath79_init_mac(tmpmac, mac, -1);
	ath79_register_wmac(art + ZHONGWANG_V1_WMAC_CALDATA_OFFSET, tmpmac);

	ath79_init_mac(tmpmac, mac, 0);

	ath79_setup_ar934x_eth_cfg(AR934X_ETH_CFG_SW_PHY_SWAP);
	
	ath79_register_mdio(1, 0x0);

	ath79_init_mac(ath79_eth1_data.mac_addr, mac, -2);
	/* eth0  is connected to the internal switch  */

	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
	ath79_eth1_data.speed = SPEED_1000;
	ath79_eth1_data.duplex = DUPLEX_FULL;
	ath79_register_eth(1);
	
	/*gpio_request_one(ZHONGWANG_V1_GPIO_USB1_POWER,
			 GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_FIXED,
			 "USB1 power");
	gpio_request_one(ZHONGWANG_V1_GPIO_USB2_POWER,
			 GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_FIXED,
			 "USB2 power");*/
	 ath79_register_usb();
}

MIPS_MACHINE(ATH79_MACH_ZHONGWANG_V1, "ZHONGWANG-V1",
	     "ZHONG WANG V1 WIFI BOARD",
	     zhongwang_v1_setup);
