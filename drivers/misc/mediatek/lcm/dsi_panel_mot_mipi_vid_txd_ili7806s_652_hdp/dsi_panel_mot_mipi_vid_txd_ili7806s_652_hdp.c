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
#include "disp_dts_gpio.h"
#endif

#ifndef MACH_FPGA
#include <lcm_pmic.h>
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_info("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define LCM_ID_0			0x10
#define LCM_ID_1 			0x05
#define LCM_ID_2	 		0x06
#define LCM_ID_3 			0x92

static const unsigned int BL_MIN_LEVEL = 20;
static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))


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

/* static unsigned char lcd_id_pins_value = 0xFF; */
static const unsigned char LCD_MODULE_ID = 0x01;
#define LCM_DSI_CMD_MODE	0
#define FRAME_WIDTH			(720)
#define FRAME_HEIGHT		(1600)

#define LCM_PHYSICAL_WIDTH	(67930)
#define LCM_PHYSICAL_HEIGHT	(150960)


#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY		0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

//static struct LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

extern void lcm_reset_pin(unsigned int mode);
extern int lcm_power_disable(unsigned int delay);
extern int lcm_power_enable(unsigned int value,unsigned int delay);

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{ 0xFF, 0x03, {0x78, 0x07, 0x00} },
	{0x28, 0, {} },
	{REGFLAG_DELAY, 20, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} }

};

static struct LCM_setting_table init_setting_vdo[] = {
	{0xFF,3,{0x78,0x07,0x01}},
	{0x00,1,{0x42}},
	{0x01,1,{0x51}},
	{0x02,1,{0x00}},
	{0x03,1,{0x9F}},
	{0x04,1,{0x00}},
	{0x05,1,{0x00}},
	{0x06,1,{0x00}},
	{0x07,1,{0x00}},
	{0x08,1,{0x89}},
	{0x09,1,{0x0A}},
	{0x0a,1,{0x30}},
	{0x0B,1,{0x00}},
	{0x0C,1,{0x04}},
	{0x0E,1,{0x04}},
	{0xFF,3,{0x78,0x07,0x11}},
	{0x00,1,{0x04}},
	{0x01,1,{0x04}},
	{0x18,1,{0x5A}},
	{0x19,1,{0x00}},
	{0x1A,1,{0x5A}},
	{0x1B,1,{0x6A}},
	{0x1C,1,{0x64}},
	{0x1D,1,{0x00}},
	{0xFF,3,{0x78,0x07,0x0E}},
	{0x02,1,{0x0D}},
	{0xFF,3,{0x78,0x07,0x01}},
	{0x31,1,{0x07}},
	{0x32,1,{0x07}},
	{0x33,1,{0x07}},
	{0x34,1,{0x07}},
	{0x35,1,{0x07}},
	{0x36,1,{0x07}},
	{0x37,1,{0x07}},
	{0x38,1,{0x40}},
	{0x39,1,{0x40}},
	{0x3a,1,{0x02}},
	{0x3b,1,{0x24}},
	{0x3c,1,{0x00}},
	{0x3d,1,{0x00}},
	{0x3e,1,{0x30}},
	{0x3f,1,{0x2F}},
	{0x40,1,{0x2E}},
	{0x41,1,{0x10}},
	{0x42,1,{0x12}},
	{0x43,1,{0x2D}},
	{0x44,1,{0x41}},
	{0x45,1,{0x41}},
	{0x46,1,{0x01}},
	{0x47,1,{0x24}},
	{0x48,1,{0x08}},
	{0x49,1,{0x07}},
	{0x4a,1,{0x07}},
	{0x4b,1,{0x07}},
	{0x4c,1,{0x07}},
	{0x4d,1,{0x07}},
	{0x4e,1,{0x07}},
	{0x4f,1,{0x07}},
	{0x50,1,{0x40}},
	{0x51,1,{0x40}},
	{0x52,1,{0x02}},
	{0x53,1,{0x24}},
	{0x54,1,{0x00}},
	{0x55,1,{0x00}},
	{0x56,1,{0x30}},
	{0x57,1,{0x2F}},
	{0x58,1,{0x2E}},
	{0x59,1,{0x11}},
	{0x5a,1,{0x13}},
	{0x5b,1,{0x2D}},
	{0x5c,1,{0x41}},
	{0x5d,1,{0x41}},
	{0x5e,1,{0x01}},
	{0x5f,1,{0x24}},
	{0x60,1,{0x09}},
	{0x61,1,{0x07}},
	{0x62,1,{0x07}},
	{0x63,1,{0x07}},
	{0x64,1,{0x07}},
	{0x65,1,{0x07}},
	{0x66,1,{0x07}},
	{0x67,1,{0x07}},
	{0x68,1,{0x40}},
	{0x69,1,{0x40}},
	{0x6a,1,{0x02}},
	{0x6b,1,{0x24}},
	{0x6c,1,{0x01}},
	{0x6d,1,{0x01}},
	{0x6e,1,{0x30}},
	{0x6f,1,{0x2F}},
	{0x70,1,{0x2E}},
	{0x71,1,{0x13}},
	{0x72,1,{0x11}},
	{0x73,1,{0x2D}},
	{0x74,1,{0x41}},
	{0x75,1,{0x41}},
	{0x76,1,{0x24}},
	{0x77,1,{0x00}},
	{0x78,1,{0x09}},
	{0x79,1,{0x07}},
	{0x7a,1,{0x07}},
	{0x7b,1,{0x07}},
	{0x7c,1,{0x07}},
	{0x7d,1,{0x07}},
	{0x7e,1,{0x07}},
	{0x7f,1,{0x07}},
	{0x80,1,{0x40}},
	{0x81,1,{0x40}},
	{0x82,1,{0x02}},
	{0x83,1,{0x24}},
	{0x84,1,{0x01}},
	{0x85,1,{0x01}},
	{0x86,1,{0x30}},
	{0x87,1,{0x2F}},
	{0x88,1,{0x2E}},
	{0x89,1,{0x12}},
	{0x8a,1,{0x10}},
	{0x8b,1,{0x2D}},
	{0x8c,1,{0x41}},
	{0x8d,1,{0x41}},
	{0x8e,1,{0x24}},
	{0x8f,1,{0x00}},
	{0x90,1,{0x08}},
	{0x92,1,{0x10}},
	{0xA2,1,{0x4C}},
	{0xA3,1,{0x49}},
	{0xA7,1,{0x10}},
	{0xAE,1,{0x00}},
	{0xB0,1,{0x20}},
	{0xB4,1,{0x05}},
	{0xC0,1,{0x0C}},
	{0xC6,1,{0x64}},
	{0xC7,1,{0x00}},
	{0xD1,1,{0x02}},
	{0xD2,1,{0x10}},
	{0xD4,1,{0x14}},
	{0xD5,1,{0x01}},
	{0xD7,1,{0x80}},
	{0xD8,1,{0x40}},
	{0xD9,1,{0x04}},
	{0xDF,1,{0x4A}},
	{0xDE,1,{0x03}},
	{0xDD,1,{0x08}},
	{0xE0,1,{0x00}},
	{0xE1,1,{0x28}},
	{0xE2,1,{0x06}},
	{0xE3,1,{0x31}},
	{0xE4,1,{0x02}},
	{0xE5,1,{0x6D}},
	{0xE6,1,{0x12}},
	{0xE7,1,{0x0C}},
	{0xED,1,{0x56}},
	{0xF4,1,{0x54}},
	{0xFF,3,{0x78,0x07,0x02}},
	{0x01,1,{0xD5}},
	{0x47,1,{0x00}},
	{0x4F,1,{0x01}},
	{0x6B,1,{0x11}},
	{0xFF,3,{0x78,0x07,0x12}},
	{0x10,1,{0x10}},
	{0x12,1,{0x08}},
	{0x13,1,{0x4F}},
	{0x16,1,{0x08}},
	{0x3A,1,{0x05}},
	{0xC0,1,{0xA1}},
	{0xC2,1,{0x12}},
	{0xC3,1,{0x20}},
	{0xFF,3,{0x78,0x07,0x04}},
	{0xBD,1,{0x01}},
	{0xFF,3,{0x78,0x07,0x04}},
	{0xB7,1,{0xCF}},
	{0xB8,1,{0x45}},
	{0xBA,1,{0x74}},
	{0xFF,3,{0x78,0x07,0x05}},
	{0x61,1,{0xCB}},
	{0xFF,3,{0x78,0x07,0x07}},
	{0x07,1,{0x4C}},
	{0xFF,3,{0x78,0x07,0x05}},
	{0x1C,1,{0x85}},
	{0x72,1,{0x56}},
	{0x74,1,{0x56}},
	{0x76,1,{0x51}},
	{0x7A,1,{0x51}},
	{0x7B,1,{0x88}},
	{0x7C,1,{0x88}},
	{0xAE,1,{0x29}},
	{0xB1,1,{0x39}},
	{0x46,1,{0x58}},
	{0x47,1,{0x78}},
	{0xB5,1,{0x58}},
	{0xB7,1,{0x78}},
	{0xC9,1,{0x90}},
	{0x56,1,{0xFF}},
	{0xFF,3,{0x78,0x07,0x06}},
	{0xC0,1,{0x40}},
	{0xC1,1,{0x16}},
	{0xC2,1,{0xFA}},
	{0xC3,1,{0x06}},
	{0x96,1,{0x50}},
	{0xD6,1,{0x55}},
	{0xCD,1,{0x68}},
	{0x13,1,{0x13}},
	{0xB4,1,{0xDC}},
	{0xB5,1,{0x23}},
	{0xFF,3,{0x78,0x07,0x08}},
	{0xE0,31,{0x00,0x00,0x1A,0x46,0x00,0x84,0xB1,0xD6,0x15,0x0F,0x3B,0x7D,0x25,0xAD,0xF8,0x33,0x2A,0x6B,0xAB,0xD3,0x3F,0x06,0x2B,0x53,0x3F,0x6F,0x90,0xBE,0x0F,0xD7,0xD9}},
	{0xE1,31,{0x00,0x00,0x1A,0x46,0x00,0x84,0xB1,0xD6,0x15,0x0F,0x3B,0x7D,0x25,0xAD,0xF8,0x33,0x2A,0x6B,0xAB,0xD3,0x3F,0x06,0x2B,0x53,0x3F,0x6F,0x90,0xBE,0x0F,0xD7,0xD9}},
	{0xFF,3,{0x78,0x07,0x0B}},
	{0xC0,1,{0x88}},
	{0xC1,1,{0x1F}},
	{0xC2,1,{0x06}},
	{0xC3,1,{0x06}},
	{0xC4,1,{0xCB}},
	{0xC5,1,{0xCB}},
	{0xD2,1,{0x45}},
	{0xD3,1,{0x0A}},
	{0xD4,1,{0x04}},
	{0xD5,1,{0x04}},
	{0xD6,1,{0x7E}},
	{0xD7,1,{0x7E}},
	{0xAB,1,{0xE0}},
	{0xFF,3,{0x78,0x07,0x0C}},
	{0x00,1,{0x27}},
	{0x01,1,{0x9D}},
	{0x02,1,{0x27}},
	{0x03,1,{0x9E}},
	{0x04,1,{0x27}},
	{0x05,1,{0x9E}},
	{0x06,1,{0x27}},
	{0x07,1,{0xA5}},
	{0x08,1,{0x27}},
	{0x09,1,{0xA3}},
	{0x0A,1,{0x27}},
	{0x0B,1,{0xA0}},
	{0x0C,1,{0x27}},
	{0x0D,1,{0xA1}},
	{0x0E,1,{0x27}},
	{0x0F,1,{0xA4}},
	{0x10,1,{0x27}},
	{0x11,1,{0xA7}},
	{0x12,1,{0x27}},
	{0x13,1,{0xA6}},
	{0x14,1,{0x27}},
	{0x15,1,{0xA2}},
	{0xFF,3,{0x78,0x07,0x0E}},
	{0x00,1,{0xA3}},
	{0x04,1,{0x00}},
	{0x20,1,{0x07}},
	{0x27,1,{0x00}},
	{0x29,1,{0xC8}},
	{0x25,1,{0x0B}},
	{0x26,1,{0x0C}},
	{0x2D,1,{0x21}},
	{0x30,1,{0x04}},
	{0x21,1,{0x24}},
	{0x23,1,{0x44}},
	{0xB0,1,{0x21}},
	{0xC0,1,{0x12}},
	{0x05,1,{0x20}},
	{0x2B,1,{0x0A}},
	{0xFF,3,{0x78,0x07,0x1E}},
	{0xA2,1,{0x00}},
	{0xAD,1,{0x02}},
	{0x00,1,{0x91}},
	{0x08,1,{0x91}},
	{0x09,1,{0x91}},
	{0x0A,1,{0x10}},
	{0x0C,1,{0x08}},
	{0x0D,1,{0x4F}},
	{0xFF,3,{0x78,0x07,0x00}},
	{0x51,0x02,{0x00,0x00}},
	//{0x68,0x02,{0x02,0x01}},
	{0x53,0x01,{0x2C}},
	{0x55,0x01,{0x00}},
	{0x11,0x01,{0x00}},
	{REGFLAG_DELAY,120,{}},
	{0x29,0x01,{0x00}},
	{REGFLAG_DELAY,20,{}},
	{0x35,0x01,{0x00}}
};

static struct LCM_setting_table bl_level[] = {
	{ 0xFF, 0x03, {0x78, 0x07, 0x00} },
	{ 0x51, 0x02, {0x0F, 0xFF} },
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


static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}


static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
	//lcm_dsi_mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
#endif
	LCM_LOGI("lcm_get_params lcm_dsi_mode %d\n", lcm_dsi_mode);
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
	params->dsi.vertical_backporch = 16;
	params->dsi.vertical_frontporch = 32;
	//params->dsi.vertical_frontporch_for_low_power = 540;/*disable dynamic frame rate*/
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 12;
	params->dsi.horizontal_backporch = 120;
	params->dsi.horizontal_frontporch = 116;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_range = 4;
	params->dsi.ssc_disable = 1;
	/*params->dsi.ssc_disable = 1;*/
#ifndef CONFIG_FPGA_EARLY_PORTING
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 270;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 310;	/* this value must be in MTK suggested table */
#endif
	//params->dsi.PLL_CK_CMD = 220;
	//params->dsi.PLL_CK_VDO = 255;
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
	params->round_corner_en = 0;
	params->corner_pattern_width = 720;
	params->corner_pattern_height = 32;
#endif
}

static void lcm_init_power(void)
{
	pr_debug("lcm_init_power\n");
	lcm_power_enable(18,10);
}

static void lcm_suspend_power(void)
{
	pr_debug("lcm_suspend_power\n");
	lcm_reset_pin(0);
	MDELAY(2);
	pr_debug("[LCM] lcm suspend power down.\n");
}

static void lcm_resume_power(void)
{
	pr_debug("lcm_resume_power\n");
	lcm_init_power();
}

static void lcm_init(void)
{
	pr_debug("[LCM] lcm_init\n");
	lcm_reset_pin(1);
	MDELAY(10);
	lcm_reset_pin(0);
	MDELAY(1);
	lcm_reset_pin(1);
	MDELAY(20);
	push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table), 1);

}

static void lcm_suspend(void)
{
	pr_debug("[LCM] lcm_suspend\n");

	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(10);

}

static void lcm_resume(void)
{
	pr_debug("[LCM] lcm_resume\n");
	lcm_init();
}

static unsigned int lcm_compare_id(void)
{
	return 1;
	/*
	unsigned char buffer[4];
	unsigned int array[16];

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(20);

	array[0] = 0x00043700;	// read id return four byte
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xA1, buffer, 4);

	LCM_LOGI("%s,ili7806s_id=0x%02x%02x%02x%02x\n", __func__,buffer[0], buffer[1],buffer[2],buffer[3]);

	if (LCM_ID_0 == buffer[0] && LCM_ID_1 == buffer[1] && LCM_ID_2 == buffer[2] && LCM_ID_3 == buffer[3])
		return 1;
	else
		return 0;
*/
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

	read_reg_v2(0x0A, buffer, 1);

	if (buffer[0] != 0x9c) {
		LCM_LOGI("[LCM ERROR] [0x53]=0x%02x\n", buffer[0]);
		return TRUE;
	}
	LCM_LOGI("[LCM NORMAL] [0x53]=0x%02x\n", buffer[0]);
	return FALSE;
#else
	return FALSE;
#endif

}

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
	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043700;	/* read id return two byte,version and id */
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

	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	return ret;
#else
	return 0;
#endif
}

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
		// set 12bit
		unsigned int bl_lvl;
		bl_lvl =(4095 * level)/255;
		LCM_LOGI("%s,ili7806s backlight: level = %d,bl_lvl=%d\n", __func__, level,bl_lvl);
		//for 12bit
		bl_level[1].para_list[0] = (bl_lvl&0xF00)>>8;
		bl_level[1].para_list[1] = (bl_lvl&0xFF);
		LCM_LOGI("%s,ili7806s_tcl : para_list[0]=%x,para_list[1]=%x\n",__func__,bl_level[1].para_list[0],bl_level[1].para_list[1]);

		push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

/*
static void *lcm_switch_mode(int mode)
{
#ifndef BUILD_LK
	// customization: 1. V2C config 2 values, C2V config 1 value; 2. config mode control register
	if (mode == 0) {	// V2C
		lcm_switch_mode_cmd.mode = CMD_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;	// mode control addr
		lcm_switch_mode_cmd.val[0] = 0x13;	// enabel GRAM firstly, ensure writing one frame to GRAM
		lcm_switch_mode_cmd.val[1] = 0x10;	// disable video mode secondly
	} else {		// C2V
		lcm_switch_mode_cmd.mode = SYNC_PULSE_VDO_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;
		lcm_switch_mode_cmd.val[0] = 0x03;	// disable GRAM and enable video mode
	}
	return (void *)(&lcm_switch_mode_cmd);
#else
	return NULL;
#endif
}
*/

#if (LCM_DSI_CMD_MODE)

/* partial update restrictions:
 * 1. roi width must be 1080 (full lcm width)
 * 2. vertical start (y) must be multiple of 16
 * 3. vertical height (h) must be multiple of 16
 */
static void lcm_validate_roi(int *x, int *y, int *width, int *height)
{
	unsigned int y1 = *y;
	unsigned int y2 = *height + y1 - 1;
	unsigned int x1, w, h;

	x1 = 0;
	w = FRAME_WIDTH;

	y1 = round_down(y1, 16);
	h = y2 - y1 + 1;

	/* in some cases, roi maybe empty. In this case we need to use minimu roi */
	if (h < 16)
		h = 16;

	h = round_up(h, 16);

	/* check height again */
	if (y1 >= FRAME_HEIGHT || y1 + h > FRAME_HEIGHT) {
		/* assign full screen roi */
		LCM_LOGD("%s calc error,assign full roi:y=%d,h=%d\n", __func__, *y, *height);
		y1 = 0;
		h = FRAME_HEIGHT;
	}

	/*LCM_LOGD("lcm_validate_roi (%d,%d,%d,%d) to (%d,%d,%d,%d)\n",*/
	/*	*x, *y, *width, *height, x1, y1, w, h);*/

	*x = x1;
	*width = w;
	*y = y1;
	*height = h;
}
#endif

/*
static struct LCM_setting_table set_cabc[] = {
	{ 0xFF, 0x03, {0x78, 0x07, 0x00} },
	{ 0x55, 0x01, {0x02} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};

static int cabc_status = 0;
static void lcm_set_cabc_cmdq(void *handle, unsigned int enable)
{
	pr_err("[ili7806s] cabc set to %d\n", enable);
	if (enable==1){
		set_cabc[1].para_list[0]=0x02;
		push_table(handle, set_cabc, sizeof(set_cabc) / sizeof(struct LCM_setting_table), 1);
		pr_err("[ili7806s] cabc set enable, set_cabc[0x%2x]=%x \n",set_cabc[1].cmd,set_cabc[1].para_list[0]);
	}else if (enable == 0){
		set_cabc[1].para_list[0]=0x00;
		push_table(handle, set_cabc, sizeof(set_cabc) / sizeof(struct LCM_setting_table), 1);
		pr_err("[ili7806s] cabc set disable, set_cabc[0x%2x]=%x \n",set_cabc[1].cmd,set_cabc[1].para_list[0]);
	}
	cabc_status = enable;
}

static void lcm_get_cabc_status(int *status)
{
	pr_err("[ili7806s] cabc get to %d\n", cabc_status);
	*status = cabc_status;
}
*/

struct LCM_DRIVER mipi_mot_vid_txd_ili7806s_hdp_652_lcm_drv = {
	.name = "mipi_mot_vid_txd_ili7806s_hdp_652",
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
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = lcm_ata_check,
//	.set_cabc_cmdq = lcm_set_cabc_cmdq,
//	.get_cabc_status = lcm_get_cabc_status,
#if (LCM_DSI_CMD_MODE)
	.validate_roi = lcm_validate_roi,
#endif

};
