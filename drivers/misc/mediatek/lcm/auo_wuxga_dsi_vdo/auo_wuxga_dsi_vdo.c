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

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
/*#include <mach/mt_pm_ldo.h>*/
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>
#endif
#endif
#ifdef CONFIG_MTK_LEGACY
#include <cust_gpio_usage.h>
#endif
#ifndef CONFIG_FPGA_EARLY_PORTING
#if defined(CONFIG_MTK_LEGACY)
#include <cust_i2c.h>
#endif
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

static LCM_UTIL_FUNCS lcm_util;

#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define set_gpio_lcd_enp(cmd) \
		lcm_util.set_gpio_lcd_enp_bias(cmd)

#define LCM_DSI_CMD_MODE	0
#define FRAME_WIDTH		(1200)
#define FRAME_HEIGHT		(1920)

#ifndef CONFIG_FPGA_EARLY_PORTING
#define GPIO_PWR_EN GPIO_LCD_BIAS_ENP_PIN
#endif

#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY		0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD

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

static struct LCM_setting_table init_setting[] = {
	{ 0x8f, 1, { 0xa5 } },
	{ REGFLAG_DELAY, 5, {} },
	{ 0x01, 1, { 0x00 } },
	{ REGFLAG_DELAY, 20, {} },
	{ 0x8f, 1, { 0xa5 } },
	{ REGFLAG_DELAY, 5, {} },
	{ 0x83, 1, { 0xaa } },
	{ 0x84, 1, { 0x11 } },
	{ 0xa9, 1, { 0x4b } },
	{ 0x83, 1, { 0x00 } },
	{ 0x84, 1, { 0x00 } },
	{ 0x8f, 1, { 0x00 } },
	/* Setting ending by predefined flag */
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(void *cmdq, struct LCM_setting_table *table,
	unsigned int count, unsigned char force_update)
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
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

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

	params->dsi.mode = SYNC_EVENT_VDO_MODE;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Video mode setting */
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 1;
	params->dsi.vertical_backporch = 25;	/* 3;//4 */
	params->dsi.vertical_frontporch = 35;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 1;	/* 12;//20; */
	params->dsi.horizontal_backporch = 60;	/* 32;//48; //10 OK */
	params->dsi.horizontal_frontporch = 80;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.PLL_CLOCK = 480;
	params->dsi.cont_clock = 1;
}

static void lcm_init(void)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef CONFIG_MTK_LEGACY
	mt_set_gpio_mode(GPIO_PWR_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_PWR_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_PWR_EN, GPIO_OUT_ONE);
#else
	set_gpio_lcd_enp(1);
#endif
#endif
	MDELAY(10);
	push_table(0, init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef CONFIG_MTK_LEGACY
	mt_set_gpio_mode(GPIO_PWR_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_PWR_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_PWR_EN, GPIO_OUT_ZERO);
#else
	set_gpio_lcd_enp(0);
#endif
#endif
}

static void lcm_resume(void)
{
	lcm_init();
}

LCM_DRIVER auo_wuxga_dsi_vdo_lcm_drv = {
	.name = "auo_wuxga_dsi_vdo_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
