/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define LOG_TAG "LCM"

#include "lcm_drv.h"

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_notice("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define LCM_ID_NT35695 (0xf5)

static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define MDELAY(n)       (lcm_util.mdelay(n))
#define UDELAY(n)       (lcm_util.udelay(n))


/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif

#endif

#ifndef BUILD_LK
static unsigned int GPIO_LCD_PWR_EN;
static unsigned int GPIO_LCD_PWR2_EN;

void lcm_request_gpio_control(struct device *dev)
{
	GPIO_LCD_PWR_EN = of_get_named_gpio(dev->of_node, "gpio_lcd_pwr_en", 0);

	gpio_request(GPIO_LCD_PWR_EN, "GPIO_LCD_PWR_EN");
	pr_notice("[KE/LCM] GPIO_LCD_PWR_EN = 0x%x\n", GPIO_LCD_PWR_EN);

	GPIO_LCD_PWR2_EN = of_get_named_gpio(dev->of_node, "gpio_lcd_pwr2_en", 0);

	gpio_request(GPIO_LCD_PWR2_EN, "GPIO_LCD_PWR2_EN");
	pr_notice("[KE/LCM] GPIO_LCD_PWR2_EN = 0x%x\n", GPIO_LCD_PWR2_EN);
}

static int lcm_driver_probe(struct device *dev, void const *data)
{
	lcm_request_gpio_control(dev);

	return 0;
}

static const struct of_device_id lcm_platform_of_match[] = {
	{
		.compatible = "tpv,otm1901a",
		.data = 0,
	}, {
		/* sentinel */
	}
};

MODULE_DEVICE_TABLE(of, platform_of_match);

static int lcm_platform_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;

	id = of_match_node(lcm_platform_of_match, pdev->dev.of_node);
	if (!id)
		return -ENODEV;

	return lcm_driver_probe(&pdev->dev, id->data);
}

static struct platform_driver lcm_driver = {
	.probe = lcm_platform_probe,
	.driver = {
		.name = "otm1901a_fhd_dsi_vdo_tpv",
		.owner = THIS_MODULE,
		.of_match_table = lcm_platform_of_match,
	},
};

static int __init lcm_init(void)
{
	if (platform_driver_register(&lcm_driver)) {
		pr_notice("LCM: failed to register this driver!\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit lcm_exit(void)
{
	platform_driver_unregister(&lcm_driver);
}

late_initcall(lcm_init);
module_exit(lcm_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("LCM display subsystem driver");
MODULE_LICENSE("GPL");
#endif

/* static unsigned char lcd_id_pins_value = 0xFF; */
static const unsigned char LCD_MODULE_ID = 0x01;
/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */
#define LCM_DSI_CMD_MODE                                    0
#define FRAME_WIDTH                                     (1080)
#define FRAME_HEIGHT                                    (1920)
#define GPIO_OUT_ONE  1
#define GPIO_OUT_ZERO 0

/* GPIO158 panel 1.8V */
#ifdef GPIO_LCM_PWR_EN
#define GPIO_LCD_PWR_EN	GPIO_LCM_PWR_EN
#else
#define GPIO_LCD_PWR_EN	0xffffffff
#endif

/* GPIO159 panel 2.8V */
#ifdef GPIO_LCM_PWR2_EN
#define GPIO_LCD_PWR2_EN	GPIO_LCM_PWR2_EN
#else
#define GPIO_LCD_PWR2_EN	0xffffffff
#endif

#define REGFLAG_DELAY	0xFFFC
#define REGFLAG_UDELAY	0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{0x4F, 1, {0x01} },
	{REGFLAG_DELAY, 120, {} }
};

static struct LCM_setting_table init_setting[] = {
	{0xFF, 1, {0x24} },
	{0xFB, 1, {0x01} },
	{0xC5, 1, {0x31} },


	{0xFF, 1, {0x20} },
	{0x00, 1, {0x01} },
	{0x01, 1, {0x55} },
	{0x02, 1, {0x45} },
	{0x03, 1, {0x55} },
	{0x05, 1, {0x40} },/* VGH=2xAVDD, VGL=2xAVEE */
	{0x06, 1, {0x99} },
	{0x07, 1, {0x9E} },
	{0x08, 1, {0x0C} },
	{0x0B, 1, {0x87} },
	{0x0C, 1, {0x87} },
	{0x0E, 1, {0xAB} },
	{0x0F, 1, {0xA9} },
	{0x11, 1, {0x0D} },/* VCOM */
	{0x12, 1, {0x10} },/* VCOM */
	{0x13, 1, {0x03} },
	{0x14, 1, {0x4A} },
	{0x15, 1, {0x12} },
	{0x16, 1, {0x12} },
	{0x30, 1, {0x01} },
	{0x58, 1, {0x00} },
	{0x59, 1, {0x00} },
	{0x5A, 1, {0x02} },
	{0x5B, 1, {0x00} },
	{0x5C, 1, {0x00} },
	{0x5D, 1, {0x00} },
	{0x5E, 1, {0x02} },
	{0x5F, 1, {0x02} },
	{0x72, 1, {0x31} },
	{0xFB, 1, {0x01} },

	{0xFF, 1, {0x24} },
	{0x00, 1, {0x01} },
	{0x01, 1, {0x0B} },
	{0x02, 1, {0x0C} },
	{0x03, 1, {0x03} },
	{0x04, 1, {0x05} },
	{0x05, 1, {0x1C} },
	{0x06, 1, {0x10} },
	{0x07, 1, {0x00} },
	{0x08, 1, {0x1C} },
	{0x09, 1, {0x00} },
	{0x0A, 1, {0x00} },
	{0x0B, 1, {0x00} },
	{0x0C, 1, {0x00} },
	{0x0D, 1, {0x13} },
	{0x0E, 1, {0x15} },
	{0x0F, 1, {0x17} },
	{0x10, 1, {0x01} },
	{0x11, 1, {0x0B} },
	{0x12, 1, {0x0C} },
	{0x13, 1, {0x04} },
	{0x14, 1, {0x06} },
	{0x15, 1, {0x1C} },
	{0x16, 1, {0x10} },
	{0x17, 1, {0x00} },
	{0x18, 1, {0x1C} },
	{0x19, 1, {0x00} },
	{0x1A, 1, {0x00} },
	{0x1B, 1, {0x00} },
	{0x1C, 1, {0x00} },
	{0x1D, 1, {0x13} },
	{0x1E, 1, {0x15} },
	{0x1F, 1, {0x17} },
	{0x20, 1, {0x00} },/* STV */
	{0x21, 1, {0x03} },
	{0x22, 1, {0x01} },
	{0x23, 1, {0x4A} },
	{0x24, 1, {0x4A} },
	{0x25, 1, {0x6D} },
	{0x26, 1, {0x40} },
	{0x27, 1, {0x40} },
	{0x32, 1, {0x7B} },
	{0x33, 1, {0x00} },
	{0x34, 1, {0x01} },
	{0x35, 1, {0x8E} },
	{0x39, 1, {0x01} },
	{0x3A, 1, {0x8E} },

	{0xBD, 1, {0x20} },/* VEND */
	{0xB6, 1, {0x22} },
	{0xB7, 1, {0x24} },
	{0xB8, 1, {0x07} },
	{0xB9, 1, {0x07} },
	{0xC1, 1, {0x6D} },
	{0xC2, 1, {0x00} }, /* disable Vblank protection for low fps power saving (for vdo mode)*/
	{0xC4, 1, {0x24} },/* updated */

	{0xBE, 1, {0x07} },
	{0xBF, 1, {0x07} },
	{0x29, 1, {0xD8} },/* UD */
	{0x2A, 1, {0x2A} },

	{0x5B, 1, {0x43} },/* CTRL */
	{0x5C, 1, {0x00} },
	{0x5F, 1, {0x73} },
	{0x60, 1, {0x73} },
	{0x63, 1, {0x22} },
	{0x64, 1, {0x00} },
	{0x67, 1, {0x08} },
	{0x68, 1, {0x04} },

	{0x7A, 1, {0x80} },/* MUX */
	{0x7B, 1, {0x91} },
	{0x7C, 1, {0xD8} },
	{0x7D, 1, {0x60} },
	{0x74, 1, {0x09} },
	{0x7E, 1, {0x09} },
	{0x75, 1, {0x21} },
	{0x7F, 1, {0x21} },
	{0x76, 1, {0x05} },
	{0x81, 1, {0x05} },
	{0x77, 1, {0x04} },
	{0x82, 1, {0x04} },

	{0x93, 1, {0x06} },/* FP,BP */
	{0x94, 1, {0x06} },
	{0xB3, 1, {0x00} },
	{0xB4, 1, {0x00} },
	{0xB5, 1, {0x00} },

	{0x78, 1, {0x00} },/* SOURCE EQ */
	{0x79, 1, {0x00} },
	{0x80, 1, {0x00} },
	{0x83, 1, {0x00} },
	{0x84, 1, {0x04} },


	{0x8A, 1, {0x33} },/* /Inversion Type// pixel column driving */
	{0x8B, 1, {0xF0} },
	{0x9B, 1, {0x0F} },
	{0xC6, 1, {0x09} },
	{0xFB, 1, {0x01} },
	{0xEC, 1, {0x00} },


	{0xFF, 1, {0x20} },
	{0xFB, 1, {0x01} },
	{0x75, 1, {0x00} },
	{0x76, 1, {0x49} },
	{0x77, 1, {0x00} },
	{0x78, 1, {0x78} },
	{0x79, 1, {0x00} },
	{0x7A, 1, {0xA4} },
	{0x7B, 1, {0x00} },
	{0x7C, 1, {0xC2} },
	{0x7D, 1, {0x00} },
	{0x7E, 1, {0xDA} },
	{0x7F, 1, {0x00} },
	{0x80, 1, {0xED} },
	{0x81, 1, {0x00} },
	{0x82, 1, {0xFE} },
	{0x83, 1, {0x01} },
	{0x84, 1, {0x0E} },
	{0x85, 1, {0x01} },
	{0x86, 1, {0x1B} },
	{0x87, 1, {0x01} },
	{0x88, 1, {0x48} },
	{0x89, 1, {0x01} },
	{0x8A, 1, {0x6C} },
	{0x8B, 1, {0x01} },
	{0x8C, 1, {0xA2} },
	{0x8D, 1, {0x01} },
	{0x8E, 1, {0xCD} },
	{0x8F, 1, {0x02} },
	{0x90, 1, {0x0F} },
	{0x91, 1, {0x02} },
	{0x92, 1, {0x42} },
	{0x93, 1, {0x02} },
	{0x94, 1, {0x43} },
	{0x95, 1, {0x02} },
	{0x96, 1, {0x71} },
	{0x97, 1, {0x02} },
	{0x98, 1, {0xA3} },
	{0x99, 1, {0x02} },
	{0x9A, 1, {0xC5} },
	{0x9B, 1, {0x02} },
	{0x9C, 1, {0xF3} },
	{0x9D, 1, {0x03} },
	{0x9E, 1, {0x12} },
	{0x9F, 1, {0x03} },
	{0xA0, 1, {0x3A} },
	{0xA2, 1, {0x03} },
	{0xA3, 1, {0x46} },
	{0xA4, 1, {0x03} },
	{0xA5, 1, {0x52} },
	{0xA6, 1, {0x03} },
	{0xA7, 1, {0x60} },
	{0xA9, 1, {0x03} },
	{0xAA, 1, {0x6E} },
	{0xAB, 1, {0x03} },
	{0xAC, 1, {0x7D} },
	{0xAD, 1, {0x03} },
	{0xAE, 1, {0x8B} },
	{0xAF, 1, {0x03} },
	{0xB0, 1, {0x91} },
	{0xB1, 1, {0x03} },
	{0xB2, 1, {0xCF} },
	{0xB3, 1, {0x00} },/* RN GAMMA SETTING */
	{0xB4, 1, {0x49} },
	{0xB5, 1, {0x00} },
	{0xB6, 1, {0x78} },
	{0xB7, 1, {0x00} },
	{0xB8, 1, {0xA4} },
	{0xB9, 1, {0x00} },
	{0xBA, 1, {0xC2} },
	{0xBB, 1, {0x00} },
	{0xBC, 1, {0xDA} },
	{0xBD, 1, {0x00} },
	{0xBE, 1, {0xED} },
	{0xBF, 1, {0x00} },
	{0xC0, 1, {0xFE} },
	{0xC1, 1, {0x01} },
	{0xC2, 1, {0x0E} },
	{0xC3, 1, {0x01} },
	{0xC4, 1, {0x1B} },
	{0xC5, 1, {0x01} },
	{0xC6, 1, {0x48} },
	{0xC7, 1, {0x01} },
	{0xC8, 1, {0x6C} },
	{0xC9, 1, {0x01} },
	{0xCA, 1, {0xA2} },
	{0xCB, 1, {0x01} },
	{0xCC, 1, {0xCD} },
	{0xCD, 1, {0x02} },
	{0xCE, 1, {0x0F} },
	{0xCF, 1, {0x02} },
	{0xD0, 1, {0x42} },
	{0xD1, 1, {0x02} },
	{0xD2, 1, {0x43} },
	{0xD3, 1, {0x02} },
	{0xD4, 1, {0x71} },
	{0xD5, 1, {0x02} },
	{0xD6, 1, {0xA3} },
	{0xD7, 1, {0x02} },
	{0xD8, 1, {0xC5} },
	{0xD9, 1, {0x02} },
	{0xDA, 1, {0xF3} },
	{0xDB, 1, {0x03} },
	{0xDC, 1, {0x12} },
	{0xDD, 1, {0x03} },
	{0xDE, 1, {0x3A} },
	{0xDF, 1, {0x03} },
	{0xE0, 1, {0x46} },
	{0xE1, 1, {0x03} },
	{0xE2, 1, {0x52} },
	{0xE3, 1, {0x03} },
	{0xE4, 1, {0x60} },
	{0xE5, 1, {0x03} },
	{0xE6, 1, {0x6E} },
	{0xE7, 1, {0x03} },
	{0xE8, 1, {0x7D} },
	{0xE9, 1, {0x03} },
	{0xEA, 1, {0x8B} },
	{0xEB, 1, {0x03} },
	{0xEC, 1, {0x91} },
	{0xED, 1, {0x03} },
	{0xEE, 1, {0xCF} },

	{0xEF, 1, {0x00} },/* GP GAMMA SETTING */
	{0xF0, 1, {0x49} },
	{0xF1, 1, {0x00} },
	{0xF2, 1, {0x78} },
	{0xF3, 1, {0x00} },
	{0xF4, 1, {0xA4} },
	{0xF5, 1, {0x00} },
	{0xF6, 1, {0xC2} },
	{0xF7, 1, {0x00} },
	{0xF8, 1, {0xDA} },
	{0xF9, 1, {0x00} },
	{0xFA, 1, {0xED} },

	{0xFF, 1, {0x21} },/* CMD2 PAGE1 */
	{0xFB, 1, {0x01} },

	{0x00, 1, {0x00} },
	{0x01, 1, {0xFE} },
	{0x02, 1, {0x01} },
	{0x03, 1, {0x0E} },
	{0x04, 1, {0x01} },
	{0x05, 1, {0x1B} },
	{0x06, 1, {0x01} },
	{0x07, 1, {0x48} },
	{0x08, 1, {0x01} },
	{0x09, 1, {0x6C} },
	{0x0A, 1, {0x01} },
	{0x0B, 1, {0xA2} },
	{0x0C, 1, {0x01} },
	{0x0D, 1, {0xCD} },
	{0x0E, 1, {0x02} },
	{0x0F, 1, {0x0F} },
	{0x10, 1, {0x02} },
	{0x11, 1, {0x42} },
	{0x12, 1, {0x02} },
	{0x13, 1, {0x43} },
	{0x14, 1, {0x02} },
	{0x15, 1, {0x71} },
	{0x16, 1, {0x02} },
	{0x17, 1, {0xA3} },
	{0x18, 1, {0x02} },
	{0x19, 1, {0xC5} },
	{0x1A, 1, {0x02} },
	{0x1B, 1, {0xF3} },
	{0x1C, 1, {0x03} },
	{0x1D, 1, {0x12} },
	{0x1E, 1, {0x03} },
	{0x1F, 1, {0x3A} },
	{0x20, 1, {0x03} },
	{0x21, 1, {0x46} },
	{0x22, 1, {0x03} },
	{0x23, 1, {0x52} },
	{0x24, 1, {0x03} },
	{0x25, 1, {0x60} },
	{0x26, 1, {0x03} },
	{0x27, 1, {0x6E} },
	{0x28, 1, {0x03} },
	{0x29, 1, {0x7D} },
	{0x2A, 1, {0x03} },
	{0x2B, 1, {0x8B} },
	{0x2D, 1, {0x03} },
	{0x2F, 1, {0x91} },
	{0x30, 1, {0x03} },
	{0x31, 1, {0xCF} },
	{0x32, 1, {0x00} },
	{0x33, 1, {0x49} },
	{0x34, 1, {0x00} },
	{0x35, 1, {0x78} },
	{0x36, 1, {0x00} },
	{0x37, 1, {0xA4} },
	{0x38, 1, {0x00} },
	{0x39, 1, {0xC2} },
	{0x3A, 1, {0x00} },
	{0x3B, 1, {0xDA} },
	{0x3D, 1, {0x00} },
	{0x3F, 1, {0xED} },
	{0x40, 1, {0x00} },
	{0x41, 1, {0xFE} },
	{0x42, 1, {0x01} },
	{0x43, 1, {0x0E} },
	{0x44, 1, {0x01} },
	{0x45, 1, {0x1B} },
	{0x46, 1, {0x01} },
	{0x47, 1, {0x48} },
	{0x48, 1, {0x01} },
	{0x49, 1, {0x6C} },
	{0x4A, 1, {0x01} },
	{0x4B, 1, {0xA2} },
	{0x4C, 1, {0x01} },
	{0x4D, 1, {0xCD} },
	{0x4E, 1, {0x02} },
	{0x4F, 1, {0x0F} },
	{0x50, 1, {0x02} },
	{0x51, 1, {0x42} },
	{0x52, 1, {0x02} },
	{0x53, 1, {0x43} },
	{0x54, 1, {0x02} },
	{0x55, 1, {0x71} },
	{0x56, 1, {0x02} },
	{0x58, 1, {0xA3} },
	{0x59, 1, {0x02} },
	{0x5A, 1, {0xC5} },
	{0x5B, 1, {0x02} },
	{0x5C, 1, {0xF3} },
	{0x5D, 1, {0x03} },
	{0x5E, 1, {0x12} },
	{0x5F, 1, {0x03} },
	{0x60, 1, {0x3A} },
	{0x61, 1, {0x03} },
	{0x62, 1, {0x46} },
	{0x63, 1, {0x03} },
	{0x64, 1, {0x52} },
	{0x65, 1, {0x03} },
	{0x66, 1, {0x60} },
	{0x67, 1, {0x03} },
	{0x68, 1, {0x6E} },
	{0x69, 1, {0x03} },
	{0x6A, 1, {0x7D} },
	{0x6B, 1, {0x03} },
	{0x6C, 1, {0x8B} },
	{0x6D, 1, {0x03} },
	{0x6E, 1, {0x91} },
	{0x6F, 1, {0x03} },
	{0x70, 1, {0xCF} },
	{0x71, 1, {0x00} },/* BP GAMMA SETTING */
	{0x72, 1, {0x49} },
	{0x73, 1, {0x00} },
	{0x74, 1, {0x78} },
	{0x75, 1, {0x00} },
	{0x76, 1, {0xA4} },
	{0x77, 1, {0x00} },
	{0x78, 1, {0xC2} },
	{0x79, 1, {0x00} },
	{0x7A, 1, {0xDA} },
	{0x7B, 1, {0x00} },
	{0x7C, 1, {0xED} },
	{0x7D, 1, {0x00} },
	{0x7E, 1, {0xFE} },
	{0x7F, 1, {0x01} },
	{0x80, 1, {0x0E} },
	{0x81, 1, {0x01} },
	{0x82, 1, {0x1B} },
	{0x83, 1, {0x01} },
	{0x84, 1, {0x48} },
	{0x85, 1, {0x01} },
	{0x86, 1, {0x6C} },
	{0x87, 1, {0x01} },
	{0x88, 1, {0xA2} },
	{0x89, 1, {0x01} },
	{0x8A, 1, {0xCD} },
	{0x8B, 1, {0x02} },
	{0x8C, 1, {0x0F} },
	{0x8D, 1, {0x02} },
	{0x8E, 1, {0x42} },
	{0x8F, 1, {0x02} },
	{0x90, 1, {0x43} },
	{0x91, 1, {0x02} },
	{0x92, 1, {0x71} },
	{0x93, 1, {0x02} },
	{0x94, 1, {0xA3} },
	{0x95, 1, {0x02} },
	{0x96, 1, {0xC5} },
	{0x97, 1, {0x02} },
	{0x98, 1, {0xF3} },
	{0x99, 1, {0x03} },
	{0x9A, 1, {0x12} },
	{0x9B, 1, {0x03} },
	{0x9C, 1, {0x3A} },
	{0x9D, 1, {0x03} },
	{0x9E, 1, {0x46} },
	{0x9F, 1, {0x03} },
	{0xA0, 1, {0x52} },
	{0xA2, 1, {0x03} },
	{0xA3, 1, {0x60} },
	{0xA4, 1, {0x03} },
	{0xA5, 1, {0x6E} },
	{0xA6, 1, {0x03} },
	{0xA7, 1, {0x7D} },
	{0xA9, 1, {0x03} },
	{0xAA, 1, {0x8B} },
	{0xAB, 1, {0x03} },
	{0xAC, 1, {0x91} },
	{0xAD, 1, {0x03} },
	{0xAE, 1, {0xCF} },
	{0xAF, 1, {0x00} },
	{0xB0, 1, {0x49} },
	{0xB1, 1, {0x00} },
	{0xB2, 1, {0x78} },
	{0xB3, 1, {0x00} },
	{0xB4, 1, {0xA4} },
	{0xB5, 1, {0x00} },
	{0xB6, 1, {0xC2} },
	{0xB7, 1, {0x00} },
	{0xB8, 1, {0xDA} },
	{0xB9, 1, {0x00} },
	{0xBA, 1, {0xED} },
	{0xBB, 1, {0x00} },
	{0xBC, 1, {0xFE} },
	{0xBD, 1, {0x01} },
	{0xBE, 1, {0x0E} },
	{0xBF, 1, {0x01} },
	{0xC0, 1, {0x1B} },
	{0xC1, 1, {0x01} },
	{0xC2, 1, {0x48} },
	{0xC3, 1, {0x01} },
	{0xC4, 1, {0x6C} },
	{0xC5, 1, {0x01} },
	{0xC6, 1, {0xA2} },
	{0xC7, 1, {0x01} },
	{0xC8, 1, {0xCD} },
	{0xC9, 1, {0x02} },
	{0xCA, 1, {0x0F} },
	{0xCB, 1, {0x02} },
	{0xCC, 1, {0x42} },
	{0xCD, 1, {0x02} },
	{0xCE, 1, {0x43} },
	{0xCF, 1, {0x02} },
	{0xD0, 1, {0x71} },
	{0xD1, 1, {0x02} },
	{0xD2, 1, {0xA3} },
	{0xD3, 1, {0x02} },
	{0xD4, 1, {0xC5} },
	{0xD5, 1, {0x02} },
	{0xD6, 1, {0xF3} },
	{0xD7, 1, {0x03} },
	{0xD8, 1, {0x12} },
	{0xD9, 1, {0x03} },
	{0xDA, 1, {0x3A} },
	{0xDB, 1, {0x03} },
	{0xDC, 1, {0x46} },
	{0xDD, 1, {0x03} },
	{0xDE, 1, {0x52} },
	{0xDF, 1, {0x03} },
	{0xE0, 1, {0x60} },
	{0xE1, 1, {0x03} },
	{0xE2, 1, {0x6E} },
	{0xE3, 1, {0x03} },
	{0xE4, 1, {0x7D} },
	{0xE5, 1, {0x03} },
	{0xE6, 1, {0x8B} },
	{0xE7, 1, {0x03} },
	{0xE8, 1, {0x91} },
	{0xE9, 1, {0x03} },
	{0xEA, 1, {0xCF} },

	{0xFF, 1, {0x10} }, /* Return  To CMD1 */
	{REGFLAG_UDELAY, 1, {} },

	{0x35, 1, {0x00} },
	{0x3B, 3, {0x03, 0x0a, 0x0a} },
	{0x44, 2, {0x07, 0x78} }, /* set TE event @ line 0x778(1912) for partial update */
	/* ///////////////////CABC SETTING///////// */
	{0x51, 1, {0x00} },
	{0x5E, 1, {0x00} },
	{0x53, 1, {0x24} },
	{0x55, 1, {0x00} },

	/* don't reload cmd1 setting from MTP when exit sleep.(or C9 will be overwritten) */
	{0xFB, 1, {0x01} },
	/* set partial update option */
	{0xC9, 11, {0x49, 0x02, 0x05, 0x00, 0x0F, 0x06, 0x67, 0x03, 0x2E, 0x10, 0xF0} },
#if (LCM_DSI_CMD_MODE)
	{0xBB, 1, {0x10} },/* 0x03:video mode  0x10:command mode */
#else
	{0xBB, 1, {0x03} },
#endif
	/*{REGFLAG_DELAY, 200, {} },*/
	{0x11, 0, {} },

	{REGFLAG_DELAY, 120, {} },
	{0x29, 0, {} },
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {
		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

/* --------------------------------------------------------------------------- */
/* LCM Driver Implementations */
/* --------------------------------------------------------------------------- */

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
#endif
	params->dsi.switch_mode_enable = 0;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 8;
	params->dsi.vertical_frontporch = 20;
	params->dsi.vertical_frontporch_for_low_power = 620;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch = 20;
	params->dsi.horizontal_frontporch = 40;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	/* params->dsi.ssc_disable                                                   = 1; */
#ifndef MACH_FPGA
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 420;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 440;	/* this value must be in MTK suggested table */
#endif
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif
	params->dsi.CLK_HS_POST = 36;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x53;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x24;
}

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
#ifdef BUILD_LK
	mt_set_gpio_mode(GPIO, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO, output);
#else
	gpio_set_value(GPIO, output);
#endif
}

static void lcm_init_power(void)
{
#ifdef BUILD_LK
	printf("[LK/LCM] lcm_init_power() enter\n");

	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	MDELAY(20);

	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ONE);
	MDELAY(20);

	SET_RESET_PIN(0);
	MDELAY(5);

	SET_RESET_PIN(1);
	MDELAY(20);
#else
	pr_notice("[Kernel/LCM] lcm_init_power() enter\n");
#endif
}

static void lcm_suspend_power(void)
{
	SET_RESET_PIN(0);
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ZERO);
	MDELAY(20);

	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
}

static void lcm_resume_power(void)
{
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	MDELAY(20);

	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ONE);
	MDELAY(20);

	SET_RESET_PIN(1);
	MDELAY(20);
}

static void lcm_init_lcm(void)
{
	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_resume(void)
{
	lcm_init_lcm();
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}
#if 0
static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0, version_id = 0;
	unsigned char buffer[2];
	unsigned int array[16];

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);

	SET_RESET_PIN(1);
	MDELAY(20);

	array[0] = 0x00023700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xF4, buffer, 2);
	id = buffer[0];     /* we only need ID */

	read_reg_v2(0xDB, buffer, 1);
	version_id = buffer[0];

	LCM_LOGI("%s,nt35695_id=0x%08x,version_id=0x%x\n", __func__, id, version_id);

	if (id == LCM_ID_NT35695 && version_id == 0x81)
		return 1;
	else
		return 0;
}

/* return TRUE: need recovery */
/* return FALSE: No need recovery */
static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char buffer[3];
	int array[4];

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x53, buffer, 1);

	if (buffer[0] != 0x24) {
		LCM_LOGI("[LCM ERROR] [0x53]=0x%02x\n", buffer[0]);
		return TRUE;
	}
	LCM_LOGI("[LCM NORMAL] [0x53]=0x%02x\n", buffer[0]);
	return FALSE;
#else
	return FALSE;
#endif
}
#endif
static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	unsigned int x0 = FRAME_WIDTH / 4;
	unsigned int x1 = FRAME_WIDTH * 3 / 4;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);

	unsigned int data_array[3];
	unsigned char read_buf[4];

	LCM_LOGI("ATA check size = 0x%x,0x%x,0x%x,0x%x\n", x0_MSB, x0_LSB, x1_MSB, x1_LSB);

	data_array[0] = 0x0005390A; /* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043700; /* read id return two byte,version and id */
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);

	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)
		&& (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
		ret = 1;
	else
		ret = 0;

	x0 = 0;
	x1 = FRAME_WIDTH - 1;

	x0_MSB = ((x0 >> 8) & 0xFF);
	x0_LSB = (x0 & 0xFF);
	x1_MSB = ((x1 >> 8) & 0xFF);
	x1_LSB = (x1 & 0xFF);

	data_array[0] = 0x0005390A; /* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	return ret;
#else
	return 0;
#endif
}

LCM_DRIVER otm1901a_fhd_dsi_vdo_tpv_lcm_drv = {
	.name = "otm1901a_fhd_dsi_vdo_tpv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
/*	.compare_id = lcm_compare_id, */
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
/*	.esd_check = lcm_esd_check, */
	.ata_check = lcm_ata_check,
	.update = lcm_update,
};
