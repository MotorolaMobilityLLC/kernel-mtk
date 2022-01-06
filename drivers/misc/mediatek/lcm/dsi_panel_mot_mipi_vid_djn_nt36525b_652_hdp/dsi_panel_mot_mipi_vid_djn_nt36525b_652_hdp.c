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

#include "mtkfb_params.h"

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_info("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define LCM_ID_0			0x0D
#define LCM_ID_1 			0x01
#define LCM_ID_2	 		0x25
#define LCM_ID_3 			0x94

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

#define LCM_BL_BITS_11			1
#if LCM_BL_BITS_11
#define LCM_BL_MAX_BRIGHTENSS		1638
#else
#define LCM_BL_MAX_BRIGHTENSS		3276
#endif

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

extern void lcm_set_bias_pin_disable(unsigned int delay);
extern void lcm_set_bias_pin_enable(unsigned int value,unsigned int delay);
extern void lcm_set_bias_init(unsigned int value,unsigned int delay);

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{REGFLAG_DELAY, 20, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 80, {} }

};

static struct LCM_setting_table init_setting_vdo[] = {
	/*
	{0xFF,1, {0x20}},
	{0xFB,1, {0x01}},
	{0x03,1, {0x54}},
	{0x05,1, {0xB1}},
	{0x07,1, {0x6E}},
	{0x08,1, {0xCB}},
	{0x0E,1, {0x8C}},
	{0x0F,1, {0x69}},
	{0x69,1, {0xA9}},
	{0x88,1, {0x00}},
	{0x95,1, {0xD7}},
	{0x96,1, {0xD7}},
	{0xFF,1, {0x20}},
	{0xFB,1, {0x01}},
	{0xB0,16, {0x00,0x00,0x00,0x26,0x00,0x52,0x00,0x73,0x00,0x8F,0x00,0xA6,0x00,0xBB,0x00,0xCE}},
	{0xB1,16, {0x00,0xDE,0x01,0x14,0x01,0x3B,0x01,0x78,0x01,0xA2,0x01,0xE4,0x02,0x16,0x02,0x18}},
	{0xB2,16, {0x02,0x49,0x02,0x80,0x02,0xA9,0x02,0xDC,0x03,0x00,0x03,0x2D,0x03,0x3C,0x03,0x4B}},
	{0xB3,12, {0x03,0x5D,0x03,0x73,0x03,0x90,0x03,0xB7,0x03,0xD6,0x03,0xD7}},
	{0xB4,16, {0x00,0x00,0x00,0x26,0x00,0x50,0x00,0x73,0x00,0x8F,0x00,0xA6,0x00,0xBB,0x00,0xCE}},
	{0xB5,16, {0x00,0xDE,0x01,0x14,0x01,0x3B,0x01,0x78,0x01,0xA2,0x01,0xE4,0x02,0x16,0x02,0x18}},
	{0xB6,16, {0x02,0x49,0x02,0x80,0x02,0xA9,0x02,0xDC,0x03,0x00,0x03,0x2D,0x03,0x3C,0x03,0x4B}},
	{0xB7,12, {0x03,0x5D,0x03,0x73,0x03,0x90,0x03,0xB7,0x03,0xD6,0x03,0xD7}},
	{0xB8,16, {0x00,0x00,0x00,0x26,0x00,0x51,0x00,0x73,0x00,0x8F,0x00,0xA6,0x00,0xBB,0x00,0xCE}},
	{0xB9,16, {0x00,0xDE,0x01,0x14,0x01,0x3B,0x01,0x78,0x01,0xA2,0x01,0xE4,0x02,0x16,0x02,0x18}},
	{0xBA,16, {0x02,0x49,0x02,0x80,0x02,0xA9,0x02,0xDC,0x03,0x00,0x03,0x2D,0x03,0x3C,0x03,0x4B}},
	{0xBB,12, {0x03,0x5D,0x03,0x73,0x03,0x90,0x03,0xB7,0x03,0xD6,0x03,0xD7}},
	{0xFF,1, {0x21}},
	{0xFB,1, {0x01}},
	{0xB0,16, {0x00,0x00,0x00,0x1E,0x00,0x4A,0x00,0x6B,0x00,0x87,0x00,0x9E,0x00,0xB3,0x00,0xC6}},
	{0xB1,16, {0x00,0xD6,0x01,0x0C,0x01,0x33,0x01,0x70,0x01,0x9A,0x01,0xDC,0x02,0x0E,0x02,0x10}},
	{0xB2,16, {0x02,0x41,0x02,0x78,0x02,0xA1,0x02,0xD4,0x02,0xF8,0x03,0x25,0x03,0x34,0x03,0x43}},
	{0xB3,12, {0x03,0x55,0x03,0x6B,0x03,0x88,0x03,0xAF,0x03,0xCE,0x03,0xD7}},
	{0xB4,16, {0x00,0x00,0x00,0x1E,0x00,0x48,0x00,0x6B,0x00,0x87,0x00,0x9E,0x00,0xB3,0x00,0xC6}},
	{0xB5,16, {0x00,0xD6,0x01,0x0C,0x01,0x33,0x01,0x70,0x01,0x9A,0x01,0xDC,0x02,0x0E,0x02,0x10}},
	{0xB6,16, {0x02,0x41,0x02,0x78,0x02,0xA1,0x02,0xD4,0x02,0xF8,0x03,0x25,0x03,0x34,0x03,0x43}},
	{0xB7,12, {0x03,0x55,0x03,0x6B,0x03,0x88,0x03,0xAF,0x03,0xCE,0x03,0xD7}},
	{0xB8,16, {0x00,0x00,0x00,0x1E,0x00,0x49,0x00,0x6B,0x00,0x87,0x00,0x9E,0x00,0xB3,0x00,0xC6}},
	{0xB9,16, {0x00,0xD6,0x01,0x0C,0x01,0x33,0x01,0x70,0x01,0x9A,0x01,0xDC,0x02,0x0E,0x02,0x10}},
	{0xBA,16, {0x02,0x41,0x02,0x78,0x02,0xA1,0x02,0xD4,0x02,0xF8,0x03,0x25,0x03,0x34,0x03,0x43}},
	{0xBB,12, {0x03,0x55,0x03,0x6B,0x03,0x88,0x03,0xAF,0x03,0xCE,0x03,0xD7}},
	{0xFF,1, {0x23}},
	{0xFB,1, {0x01}},
	{0x00,1, {0x60}},
	{0x07,1, {0x00}},
	{0x08,1, {0x01}},
	{0x09,1, {0x80}},
	{0x12,1, {0xB4}},
	{0x15,1, {0xE9}},
	{0x16,1, {0x0B}},
	{0x10,1, {0x40}},
	{0x11,1, {0x00}},
	{0x19,1, {0x00}},
	{0x1A,1, {0x04}},
	{0x1B,1, {0x08}},
	{0x1C,1, {0x0C}},
	{0x1D,1, {0x10}},
	{0x1E,1, {0x14}},
	{0x1F,1, {0x18}},
	{0x20,1, {0x1C}},
	{0x21,1, {0x20}},
	{0x22,1, {0x24}},
	{0x23,1, {0x28}},
	{0x24,1, {0x2C}},
	{0x25,1, {0x30}},
	{0x26,1, {0x34}},
	{0x27,1, {0x38}},
	{0x28,1, {0x3C}},
	{0x29,1, {0x90}},
	{0x2A,1, {0x20}},
	{0x2B,1, {0xA0}},
	{0x30,1, {0xFF}},
	{0x31,1, {0xFF}},
	{0x32,1, {0xFE}},
	{0x33,1, {0xF8}},
	{0x34,1, {0xF6}},
	{0x35,1, {0xF2}},
	{0x36,1, {0xED}},
	{0x37,1, {0xEC}},
	{0x38,1, {0xE9}},
	{0x39,1, {0xE6}},
	{0x3A,1, {0xE2}},
	{0x3B,1, {0xDE}},
	{0x3D,1, {0xDA}},
	{0x3F,1, {0xD6}},
	{0x40,1, {0xD2}},
	{0x41,1, {0xCE}},
	{0x45,1, {0xFF}},
	{0x46,1, {0xFD}},
	{0x47,1, {0xF9}},
	{0x48,1, {0xF2}},
	{0x49,1, {0xEB}},
	{0x4A,1, {0xE2}},
	{0x4B,1, {0xD8}},
	{0x4C,1, {0xCE}},
	{0x4D,1, {0xC3}},
	{0x4E,1, {0xB9}},
	{0x4F,1, {0xB0}},
	{0x50,1, {0xA8}},
	{0x51,1, {0xA1}},
	{0x52,1, {0x9A}},
	{0x53,1, {0x97}},
	{0x54,1, {0x95}},
	{0x58,1, {0xFF}},
	{0x59,1, {0xFB}},
	{0x5A,1, {0xF7}},
	{0x5B,1, {0xEE}},
	{0x5C,1, {0xE6}},
	{0x5D,1, {0xDC}},
	{0x5E,1, {0xD6}},
	{0x5F,1, {0xCE}},
	{0x60,1, {0xC3}},
	{0x61,1, {0xC0}},
	{0x62,1, {0xBC}},
	{0x63,1, {0xB8}},
	{0x64,1, {0xB4}},
	{0x65,1, {0xB0}},
	{0x66,1, {0xAC}},
	{0x67,1, {0xA8}},
	{0xFF,1, {0x24}},
	{0xFB,1, {0x01}},
	{0x00,1, {0x05}},
	{0x01,1, {0x00}},
	{0x02,1, {0x1F}},
	{0x03,1, {0x1E}},
	{0x04,1, {0x20}},
	{0x05,1, {0x00}},
	{0x06,1, {0x00}},
	{0x07,1, {0x00}},
	{0x08,1, {0x00}},
	{0x09,1, {0x0F}},
	{0x0A,1, {0x0E}},
	{0x0B,1, {0x00}},
	{0x0C,1, {0x00}},
	{0x0D,1, {0x0D}},
	{0x0E,1, {0x0C}},
	{0x0F,1, {0x04}},
	{0x10,1, {0x00}},
	{0x11,1, {0x00}},
	{0x12,1, {0x00}},
	{0x13,1, {0x00}},
	{0x14,1, {0x00}},
	{0x15,1, {0x00}},
	{0x16,1, {0x05}},
	{0x17,1, {0x00}},
	{0x18,1, {0x1F}},
	{0x19,1, {0x1E}},
	{0x1A,1, {0x20}},
	{0x1B,1, {0x00}},
	{0x1C,1, {0x00}},
	{0x1D,1, {0x00}},
	{0x1E,1, {0x00}},
	{0x1F,1, {0x0F}},
	{0x20,1, {0x0E}},
	{0x21,1, {0x00}},
	{0x22,1, {0x00}},
	{0x23,1, {0x0D}},
	{0x24,1, {0x0C}},
	{0x25,1, {0x04}},
	{0x26,1, {0x00}},
	{0x27,1, {0x00}},
	{0x28,1, {0x00}},
	{0x29,1, {0x00}},
	{0x2A,1, {0x00}},
	{0x2B,1, {0x00}},
	{0x2F,1, {0x06}},
	{0x30,1, {0x30}},
	{0x33,1, {0x30}},
	{0x34,1, {0x06}},
	{0x37,1, {0x74}},
	{0x3A,1, {0x98}},
	{0x3B,1, {0xA6}},
	{0x3D,1, {0x52}},
	{0x4D,1, {0x12}},
	{0x4E,1, {0x34}},
	{0x51,1, {0x43}},
	{0x52,1, {0x21}},
	{0x55,1, {0x42}},
	{0x56,1, {0x34}},
	{0x5A,1, {0x98}},
	{0x5B,1, {0xA6}},
	{0x5C,1, {0x00}},
	{0x5D,1, {0x00}},
	{0x5E,1, {0x08}},
	{0x60,1, {0x80}},
	{0x61,1, {0x90}},
	{0x64,1, {0x11}},
	{0x92,1, {0xB3}},
	{0x93,1, {0x0A}},
	{0x94,1, {0x0C}},
	{0xAB,1, {0x00}},
	{0xAD,1, {0x00}},
	{0xB0,1, {0x05}},
	{0xB1,1, {0xAF}},
	{0xFF,1, {0x25}},
	{0xFB,1, {0x01}},
	{0x0A,1, {0x82}},
	{0x0B,1, {0x25}},
	{0x0C,1, {0x01}},
	{0x17,1, {0x82}},
	{0x18,1, {0x06}},
	{0x19,1, {0x0F}},
	{0xFF,1, {0x26}},
	{0xFB,1, {0x01}},
	{0x00,1, {0xA0}},
	{0xFF,1, {0x27}},
	{0xFB,1, {0x01}},
	{0x13,1, {0x00}},
	{0x15,1, {0xB4}},
	{0x1F,1, {0x55}},
	{0x26,1, {0x0F}},
	{0xC0,1, {0x18}},
	{0xC1,1, {0xF2}},
	{0xC2,1, {0x00}},
	{0xC3,1, {0x00}},
	{0xC4,1, {0xF2}},
	{0xC5,1, {0x00}},
	{0xC6,1, {0x00}},
	{0xFF,1, {0x10}},
	{0xFB,1, {0x01}},
	{0xBA,1, {0x03}},
*/
	{0xFF,1, {0x10}},
	{0xFB,1, {0x01}},
	{0x68,2, {0x03,0x01}},
	{0x51,2, {0x00,0x00}},
	{0x53,1, {0x2C}},
	{0x55,1, {0x01}},
	{0x35,1, {0x00}},
	{0x11,1, {0x00}},
	{REGFLAG_DELAY,150,{}},
	{0x29,1, {0x00}},
	{REGFLAG_DELAY,50,{}}
};

#if LCM_BL_BITS_11
static struct LCM_setting_table bl_level[] = {
	{ 0x51, 0x02, {0x06, 0x66} }
};
#else
//12bit
static struct LCM_setting_table bl_level[] = {
	{ 0x51, 0x02, {0x0C, 0xCC} }
};
#endif

static struct LCM_setting_table lcm_cabc_setting_ui[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0x55, 1, {0x01} },
};

static struct LCM_setting_table lcm_cabc_setting_mv[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0x55, 1, {0x03} },
};

static struct LCM_setting_table lcm_cabc_setting_disable[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0x55, 1, {0x00} },
};

struct LCM_cabc_table {
	int cmd_num;
	struct LCM_setting_table *cabc_cmds;
};

//Make sure the seq keep consitent with definition of cabc_mode, otherwise it need remap
static struct LCM_cabc_table lcm_cabc_settings[] = {
	{ARRAY_SIZE(lcm_cabc_setting_ui), lcm_cabc_setting_ui},
	{ARRAY_SIZE(lcm_cabc_setting_mv), lcm_cabc_setting_mv},
	{ARRAY_SIZE(lcm_cabc_setting_disable), lcm_cabc_setting_disable},
};

#if LCM_BL_BITS_11
static struct LCM_setting_table lcm_hbm_setting[] = {
	{0x51, 2, {0x06, 0x66} },	//80% PWM
	{0x51, 2, {0x0F, 0XFF} },	//100% PWM
};
#else
static struct LCM_setting_table lcm_hbm_setting[] = {
	{0x51, 2, {0x0C, 0XCC} },	//80% PWM
	{0x51, 2, {0x0F, 0XFF} },	//100% PWM
};
#endif

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
	params->dsi.vertical_backporch = 252;
	params->dsi.vertical_frontporch = 10;
	//params->dsi.vertical_frontporch_for_low_power = 540;/*disable dynamic frame rate*/
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 64;
	params->dsi.horizontal_frontporch = 68;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_range = 4;
	params->dsi.ssc_disable = 1;
	/*params->dsi.ssc_disable = 1;*/
#ifndef FPGA_EARLY_PORTING
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 270;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 304;	/* this value must be in MTK suggested table */
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
	LCM_LOGI("[LCM] lcm_init_power\n");
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_PWR_EN_OUT1);
	MDELAY(1);
	lcm_set_bias_init(15,1);
	MDELAY(10);
}

static void lcm_suspend_power(void)
{
	LCM_LOGI("[LCM] lcm_suspend_power\n");
	lcm_set_bias_pin_disable(1);
	MDELAY(1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_PWR_EN_OUT0);
	LCM_LOGI("[LCM] lcm suspend power down.\n");
}

static void lcm_resume_power(void)
{
	LCM_LOGI("[LCM] lcm_resume_power\n");
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_PWR_EN_OUT1);
	MDELAY(1);
	lcm_set_bias_pin_enable(15,1);
	MDELAY(10);
}

static void lcm_init(void)
{
	LCM_LOGI("[LCM] lcm_init\n");
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
	MDELAY(1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(50);
	push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	LCM_LOGI("[LCM] lcm_suspend\n");

	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
	MDELAY(10);
}

static void lcm_resume(void)
{
	LCM_LOGI("[LCM] lcm_resume\n");
	lcm_init();
}

static unsigned int lcm_compare_id(void)
{
	unsigned char buffer[4];
	unsigned int array[16];
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
	MDELAY(1);
	disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
	MDELAY(50);

	array[0] = 0x00043700;	// read id return four byte
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xA1, buffer, 4);

	LCM_LOGI("%s,djn_nt36525b id=0x%02x%02x%02x%02x\n", __func__,buffer[0], buffer[1],buffer[2],buffer[3]);

	if (LCM_ID_0 == buffer[0] && LCM_ID_1 == buffer[1] && LCM_ID_2 == buffer[2] && LCM_ID_3 == buffer[3])
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

#if !LCM_BL_BITS_11
static unsigned int lcm_get_max_brightness(void)
{
        LCM_LOGD("%s: return max_brightness:%d\n", __func__, LCM_BL_MAX_BRIGHTENSS);
        return LCM_BL_MAX_BRIGHTENSS;
}
#endif

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	unsigned int bl_lvl;

	if (level > LCM_BL_MAX_BRIGHTENSS) {
		LCM_LOGI("%s: djn_nt36525b: level%d: exceed max bl:%d\n", __func__, level, LCM_BL_MAX_BRIGHTENSS);
	}
	bl_lvl = level;
	LCM_LOGI("%s,djn_nt36525b backlight: level = %d,bl_lvl=%d\n", __func__, level,bl_lvl);

#if LCM_BL_BITS_11
    //11bit
	bl_level[0].para_list[0] = (bl_lvl&0x700)>>8;
#else
	//12 bit
	bl_level[0].para_list[0] = (bl_lvl&0xF00)>>8;
#endif
	bl_level[0].para_list[1] = (bl_lvl&0xFF);
	LCM_LOGI("%s,djn_nt36525b: para_list[0]=%x,para_list[1]=%x\n",__func__,bl_level[0].para_list[0],bl_level[0].para_list[1]);

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

static void lcm_set_cmdq(void *handle, unsigned int *lcm_cmd,
		unsigned int *lcm_count, unsigned int *lcm_value)
{
	pr_info("%s,djn_nt36525b cmd:%d, value = %d\n", __func__, *lcm_cmd, *lcm_value);

	switch(*lcm_cmd) {
		case PARAM_HBM:
			push_table(handle, &lcm_hbm_setting[*lcm_value], 1, 1);
			break;
		case PARAM_CABC:
			if (*lcm_value >= CABC_MODE_NUM) {
				pr_info("%s: invalid CABC mode:%d out of CABC_MODE_NUM:", *lcm_value, CABC_MODE_NUM);
			}
			else {
				unsigned int cmd_num = lcm_cabc_settings[*lcm_value].cmd_num;
				pr_info("%s: handle PARAM_CABC, mode=%d, cmd_num=%d", __func__, *lcm_value, cmd_num);
				push_table(handle, lcm_cabc_settings[*lcm_value].cabc_cmds, cmd_num, 1);
			}
			break;
		default:
			pr_err("%s,djn_nt36525b cmd:%d, unsupport\n", __func__, *lcm_cmd);
			break;
	}

	pr_info("%s,ili7806s cmd:%d, value = %d done\n", __func__, *lcm_cmd, *lcm_value);

}

#ifdef CONFIG_LCM_NOTIFIY_SUPPORT
static bool lcm_set_recovery_notify(void)
{
	char tp_info[] = "nvt";

	//return TRUE if TP need lcm notify
	//NVT touch recover need enable lcm notify
	if (strstr(tp_info, "nvt")) {
		pr_info("%s: djn_nt: return TRUE\n", __func__);
		return TRUE;
	}

	return false;
}
#endif

struct LCM_DRIVER mipi_mot_vid_djn_nt36525b_hdp_652_lcm_drv = {
	.name = "mipi_mot_vid_djn_nt36525b_hdp_652",
	.supplier = "djn",
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
#if !LCM_BL_BITS_11
	.get_max_brightness = lcm_get_max_brightness,
#endif
	.ata_check = lcm_ata_check,
#ifdef CONFIG_LCM_NOTIFIY_SUPPORT
	.set_lcm_notify = lcm_set_recovery_notify,
#endif
	.set_lcm_cmd = lcm_set_cmdq,
#if (LCM_DSI_CMD_MODE)
	.validate_roi = lcm_validate_roi,
#endif

};
