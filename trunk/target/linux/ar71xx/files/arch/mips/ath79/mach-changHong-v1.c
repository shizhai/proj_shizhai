/*
 *  TP-LINK TL-WR841N/ND v8/TL-MR3420 v2 board support
 *
 *  Copyright (C) 2012 Gabor Juhos <juhosg@openwrt.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/platform_device.h>

#include <asm/mach-ath79/ath79.h>
#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define CHANGHONG_V1_GPIO_LED_WLAN	13
#define CHANGHONG_V1_GPIO_LED_QSS	15
#define CHANGHONG_V1_GPIO_LED_WAN	18
#define CHANGHONG_V1_GPIO_LED_LAN1	19
#define CHANGHONG_V1_GPIO_LED_LAN2       20
#define CHANGHONG_V1_GPIO_LED_LAN3       21
#define CHANGHONG_V1_GPIO_LED_LAN4       12

#define CHANGHONG_V1_GPIO_LED_SYSTEM	14

#define CHANGHONG_V1_GPIO_BTN_RESET	17
#define CHANGHONG_V1_GPIO_SW_RFKILL	16	/* WPS for MR3420 v2 */

#define CHANGHONG_V1_KEYS_POLL_INTERVAL	20	/* msecs */
#define CHANGHONG_V1_KEYS_DEBOUNCE_INTERVAL (3 * CHANGHONG_V1_KEYS_POLL_INTERVAL)

static const char *changHong_v1_part_probes[] = {
	"tp-link",
	NULL,
};

static struct flash_platform_data changHong_v1_flash_data = {
	.part_probes	= changHong_v1_part_probes,
};

static struct gpio_led changHong_v1_leds_gpio[] __initdata = {
	{
		.name		= "tp-link:green:lan1",
		.gpio		= CHANGHONG_V1_GPIO_LED_LAN1,
		.active_low	= 1,
	}, {
                .name           = "tp-link:green:lan2",
                .gpio           = CHANGHONG_V1_GPIO_LED_LAN2,
                .active_low     = 1,
        }, {
                .name           = "tp-link:green:lan3",
                .gpio           = CHANGHONG_V1_GPIO_LED_LAN3,
                .active_low     = 1,
        }, {
                .name           = "tp-link:green:lan4",
                .gpio           = CHANGHONG_V1_GPIO_LED_LAN4,
                .active_low     = 1,
        }, {
		.name		= "tp-link:green:qss",
		.gpio		= CHANGHONG_V1_GPIO_LED_QSS,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:system",
		.gpio		= CHANGHONG_V1_GPIO_LED_SYSTEM,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:wan",
		.gpio		= CHANGHONG_V1_GPIO_LED_WAN,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:wlan",
		.gpio		= CHANGHONG_V1_GPIO_LED_WLAN,
		.active_low	= 1,
	},/* {
		.name		= "tp-link:green:3g",
		.gpio		= TL_MR3420V2_GPIO_LED_3G,
		.active_low	= 1,
	},*/
};

static struct gpio_keys_button changHong_v1_gpio_keys[] __initdata = {
	{
		.desc		= "Reset button",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = CHANGHONG_V1_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= CHANGHONG_V1_GPIO_BTN_RESET,
		.active_low	= 1,
	}, {
		.desc		= "RFKILL switch",
		.type		= EV_SW,
		.code		= KEY_RFKILL,
		.debounce_interval = CHANGHONG_V1_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= CHANGHONG_V1_GPIO_SW_RFKILL,
		.active_low	= 0,
	}
};

static void __init tl_ap123_setup(void)
{
	u8 *mac = (u8 *) KSEG1ADDR(0x1f01fc00);
	u8 *ee = (u8 *) KSEG1ADDR(0x1fff1000);

	/* Disable JTAG, enabling GPIOs 0-4 */
	ath79_gpio_function_enable(AR934X_GPIO_FUNC_JTAG_DISABLE);

	ath79_register_m25p80(&changHong_v1_flash_data);

	ath79_setup_ar934x_eth_cfg(AR934X_ETH_CFG_SW_PHY_SWAP);

	ath79_register_mdio(1, 0x0);

	ath79_init_mac(ath79_eth0_data.mac_addr, mac, -1);
	ath79_init_mac(ath79_eth1_data.mac_addr, mac, 0);

	/* GMAC0 is connected to the PHY0 of the internal switch */
	ath79_switch_data.phy4_mii_en = 1;
	ath79_switch_data.phy_poll_mask = BIT(0);
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_MII;
	ath79_eth0_data.phy_mask = BIT(0);
	ath79_eth0_data.mii_bus_dev = &ath79_mdio1_device.dev;
	ath79_register_eth(0);

	/* GMAC1 is connected to the internal switch */
	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
	ath79_register_eth(1);

	ath79_register_wmac(ee, mac);
}

static void __init changHong_v1_setup(void)
{
	tl_ap123_setup();

	ath79_register_leds_gpio(-1, ARRAY_SIZE(changHong_v1_leds_gpio) - 1,
				 changHong_v1_leds_gpio);

	ath79_register_gpio_keys_polled(1, CHANGHONG_V1_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(changHong_v1_gpio_keys),
					changHong_v1_gpio_keys);
}

MIPS_MACHINE(ATH79_MACH_CHANGHONG_V1, "CHANGHONG-V1", "CHANGHONG v1 WIFI BOARD", changHong_v1_setup);

