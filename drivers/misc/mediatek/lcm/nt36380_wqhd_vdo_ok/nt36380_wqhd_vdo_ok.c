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


static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;


#define SET_RESET_PIN(v)        (lcm_util.set_reset_pin((v)))
#define MDELAY(n)               (lcm_util.mdelay(n))
#define UDELAY(n)               (lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
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

#define set_gpio_lcd_enp(cmd) \
		lcm_util.set_gpio_lcd_enp_bias(cmd)

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
#endif



#define LCM_DSI_CMD_MODE				0
#define FRAME_WIDTH					(1440)
#define FRAME_HEIGHT					(2560)

#define REGFLAG_DELAY	0xFFFC
#define REGFLAG_UDELAY	0xFFFB
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

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{0x4F, 1, {0x01} },
	{REGFLAG_DELAY, 120, {} }
};


static struct LCM_setting_table init_setting[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0xBA, 1, {0x01} },
	{0xBB, 1, {0x03} },
	{0xFF, 1, {0xD0} },
	{0xFB, 1, {0x01} },
	{0x97, 1, {0x04} },
	{0xC8, 1, {0x40} },
	{0xC9, 1, {0x44} },
	{0xFF, 1, {0x10} },
	{0x11, 0, {} },
	{0x51, 1, {0xFF} },
	{0x53, 1, {0x24} },
	{0x29, 0, {} },
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


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.IsCphy = 1;
	params->dsi.switch_mode = CMD_MODE;

	params->dsi.switch_mode_enable = 0;

	params->dsi.LANE_NUM = LCM_TWO_LANE;

	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size = 256;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 10;
	params->dsi.vertical_frontporch = 10;
	params->dsi.vertical_active_line = FRAME_HEIGHT;


	params->dsi.horizontal_backporch = 384;
	params->dsi.horizontal_frontporch = 384;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.horizontal_sync_active = 26;

	params->dsi.lane_swap_en = 0;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x53;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x24;


	params->dsi.LPX = 0x08;
	params->dsi.HS_PRPR = 0x06;
	params->dsi.HS_ZERO = 0x40;
	params->dsi.HS_TRAIL = 0x20;

	params->dsi.TA_GO = 0x20;
	params->dsi.TA_SURE = 0x0C;
	params->dsi.TA_GET = 0x28;
	params->dsi.DA_HS_EXIT = 0x10;

	params->dsi.CONT_DET = 0x00;
	params->dsi.CLK_ZERO = 0x2C;
	params->dsi.CLK_TRAIL = 0x0A;

	params->dsi.CLK_HS_PRPR = 0x07;
	params->dsi.CLK_HS_POST = 0x16;
	params->dsi.CLK_HS_EXIT = 0x10;

	params->dsi.lane_swap_en = 1;
	params->dsi.PHY_SEL0 = 0x65432101;
	params->dsi.PHY_SEL1 = 0x24210987;
	params->dsi.PHY_SEL2 = 0x68543102;
	params->dsi.PHY_SEL3 = 0x00000007;

	params->dsi.PLL_CLOCK = 500;

}

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_init_power(void)
{
}

static void lcm_suspend_power(void)
{
}

static void lcm_resume_power(void)
{
}

static void lcm_suspend(void)
{
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(10);
}


static unsigned int lcm_esd_check(void)
{
	return FALSE;
}

static void lcm_setbacklight(unsigned int level)
{
}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
	return 0;
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
}

static void *lcm_switch_mode(int mode)
{
	return NULL;
}

static void lcm_init(void)
{
	SET_RESET_PIN(0);

	SET_RESET_PIN(1);
	MDELAY(1000);
	SET_RESET_PIN(0);
	MDELAY(1000);

	SET_RESET_PIN(1);
	MDELAY(10);

	push_table(NULL, init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_resume(void)
{
	lcm_init();
}

static unsigned int lcm_compare_id(void)
{
	return 1;
}

LCM_DRIVER nt36380_wqhd_vdo_lcm_drv = {
	.name = "nt36380_wqhd_vdo_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.esd_check = lcm_esd_check,
	.set_backlight = lcm_setbacklight,
	.ata_check = lcm_ata_check,
	.update = lcm_update,
	.switch_mode = lcm_switch_mode,
};

